#include "stdafx.h"
#include "xrApplication.h"
#include "GameFont.h"
#include "Console.h"
#include "igame_level.h"
#include "igame_persistent.h"
#include "std_classes.h"
#include "ispatial.h"
#include "..\TSMP3_Build_Config.h"

struct _SoundProcessor : public pureFrame
{
	virtual void OnFrame()
	{
		Device.Statistic->Sound.Begin();
		::Sound->update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);
		Device.Statistic->Sound.End();
	}
} SoundProcessor;

LPCSTR _GetFontTexName(LPCSTR section)
{
	static char* tex_names[] = { "texture800", "texture", "texture1600" };
	int def_idx = 1; //default 1024x768
	int idx = def_idx;

	u32 h = Device.dwHeight;

#pragma TODO("TSMP: check why in cs here is 1024 instead of 900")
	if (h <= 600)
		idx = 0;
	else if (h <= 900)
		idx = 1;
	else
		idx = 2;

	while (idx >= 0)
	{
		if (pSettings->line_exist(section, tex_names[idx]))
			return pSettings->r_string(section, tex_names[idx]);
		--idx;
	}

	return pSettings->r_string(section, tex_names[def_idx]);
}

void _InitializeFont(CGameFont*& F, LPCSTR section, u32 flags)
{
	LPCSTR font_tex_name = _GetFontTexName(section);
	R_ASSERT(font_tex_name);

	if (!F)
	{
		F = xr_new<CGameFont>("font", font_tex_name, flags);
		Device.seqRender.Add(F, REG_PRIORITY_LOW - 1000);
	}
	else
		F->Initialize("font", font_tex_name);

	if (pSettings->line_exist(section, "size"))
	{
		float sz = pSettings->r_float(section, "size");

		if (flags & CGameFont::fsDeviceIndependent)
			F->SetHeightI(sz);
		else
			F->SetHeight(sz);
	}

	if (pSettings->line_exist(section, "interval"))
		F->SetInterval(pSettings->r_fvector2(section, "interval"));
}

CApplication::CApplication()
{
	ll_dwReference = 0;
	max_load_stage = 0;

	// events
	eQuit = Engine.Event.Handler_Attach("KERNEL:quit", this);
	eStart = Engine.Event.Handler_Attach("KERNEL:start", this);
	eStartLoad = Engine.Event.Handler_Attach("KERNEL:load", this);
	eDisconnect = Engine.Event.Handler_Attach("KERNEL:disconnect", this);

#pragma TODO("TSMP: check why in cs here added eConsole")

	// levels
	Level_Current = 0;
	Level_Scan();

	// Font
	pFontSystem = NULL;

	// Register us
	Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1000);

	if (psDeviceFlags.test(mtSound))
		Device.seqFrameMT.Add(&SoundProcessor);
	else
		Device.seqFrame.Add(&SoundProcessor);

	Console->Show();

	// App Title
	app_title[0] = '\0';
}

CApplication::~CApplication()
{
	Console->Hide();

	// font
	Device.seqRender.Remove(pFontSystem);
	xr_delete(pFontSystem);

	Device.seqFrameMT.Remove(&SoundProcessor);
	Device.seqFrame.Remove(&SoundProcessor);
	Device.seqFrame.Remove(this);

	// events
	Engine.Event.Handler_Detach(eDisconnect, this);
	Engine.Event.Handler_Detach(eStartLoad, this);
	Engine.Event.Handler_Detach(eStart, this);
	Engine.Event.Handler_Detach(eQuit, this);
}

extern CRenderDevice Device;

void CApplication::OnEvent(EVENT E, u64 P1, u64 P2)
{
	if (E == eQuit)
	{
		PostQuitMessage(0);

		for (u32 i = 0; i < Levels.size(); i++)
		{
			xr_free(Levels[i].folder);
			xr_free(Levels[i].name);
		}
	}
	else if (E == eStart)
	{
		LPSTR op_server = LPSTR(P1);
		LPSTR op_client = LPSTR(P2);
		R_ASSERT(0 == g_pGameLevel);
		R_ASSERT(0 != g_pGamePersistent);

#ifdef NO_SINGLE
		Console->Execute("main_menu on");
		if (op_server == NULL ||
			strstr(op_server, "/deathmatch") ||
			strstr(op_server, "/teamdeathmatch") ||
			strstr(op_server, "/artefacthunt"))
#endif
		{
			Console->Execute("main_menu off");
			Console->Hide();
			Device.Reset(false);

			g_pGamePersistent->PreStart(op_server);

			g_pGameLevel = (IGame_Level*)NEW_INSTANCE(CLSID_GAME_LEVEL);
			pApp->LoadBegin();
			g_pGamePersistent->Start(op_server);
			g_pGameLevel->net_Start(op_server, op_client);
			pApp->LoadEnd();
		}

		xr_free(op_server);
		xr_free(op_client);
	}
	else if (E == eDisconnect)
	{
		if (g_pGameLevel)
		{
			Console->Hide();
			g_pGameLevel->net_Stop();
			DEL_INSTANCE(g_pGameLevel);
			Console->Show();

			if ((FALSE == Engine.Event.Peek("KERNEL:quit")) && (FALSE == Engine.Event.Peek("KERNEL:start")))
			{
				Console->Execute("main_menu off");
				Console->Execute("main_menu on");
			}
		}

		R_ASSERT(0 != g_pGamePersistent);
		g_pGamePersistent->Disconnect();
	}
}

static CTimer phase_timer;
extern ENGINE_API BOOL g_appLoaded = FALSE;

void CApplication::LoadBegin()
{
	ll_dwReference++;
	if (1 == ll_dwReference)
	{
		g_appLoaded = FALSE;

#ifndef DEDICATED_SERVER
		_InitializeFont(pFontSystem, "ui_font_graffiti19_russian", 0);

		m_pRender->LoadBegin();
#endif
		phase_timer.Start();
		load_stage = 0;
	}
}

void CApplication::LoadEnd()
{
	ll_dwReference--;
	if (0 == ll_dwReference)
	{
		Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
		Msg("* phase cmem: %d K", Memory.mem_usage() / 1024);
		Console->Execute("stat_memory");
		g_appLoaded = TRUE;
	}
}

void CApplication::destroy_loading_shaders()
{
	m_pRender->destroy_loading_shaders();
}

void CApplication::LoadDraw()
{
	if (g_appLoaded)
		return;

	Device.CurrentFrameNumber += 1;

	if (!Device.Begin())
		return;

	if (g_dedicated_server)
		Console->OnRender();
	else
		load_draw_internal();

	Device.End();
}

void CApplication::LoadTitleInt(LPCSTR str)
{
	load_stage++;

	VERIFY(ll_dwReference);
	VERIFY(str && xr_strlen(str) < 256);
	strcpy_s(app_title, str);
	Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
	phase_timer.Start();
	Msg("* phase cmem: %d K", Memory.mem_usage() / 1024);
	Log(app_title);

	if (g_pGamePersistent->GameType() == 1 && strstr(Core.Params, "alife"))
		max_load_stage = 17;
	else
		max_load_stage = 14;

	LoadDraw();
}

// Sequential
void CApplication::OnFrame()
{
	Engine.Event.OnFrame();
	g_SpatialSpace->update();
	g_SpatialSpacePhysic->update();

	if (g_pGameLevel)
		g_pGameLevel->SoundEvent_Dispatch();
}

void CApplication::Level_Append(LPCSTR folder)
{
	string_path N1, N2, N3, N4;
	strconcat(sizeof(N1), N1, folder, "level");
	strconcat(sizeof(N2), N2, folder, "level.ltx");
	strconcat(sizeof(N3), N3, folder, "level.geom");
	strconcat(sizeof(N4), N4, folder, "level.cform");

	if (
		FS.exist("$game_levels$", N1) &&
		FS.exist("$game_levels$", N2) &&
		FS.exist("$game_levels$", N3) &&
		FS.exist("$game_levels$", N4))
	{
		sLevelInfo LI;
		LI.folder = xr_strdup(folder);
		LI.name = 0;
		Levels.push_back(LI);
	}
}

void CApplication::Level_Scan()
{
#pragma todo("container is created in stack!")
	xr_vector<char*>* folder = FS.file_list_open("$game_levels$", FS_ListFolders | FS_RootOnly);
	R_ASSERT(folder && folder->size());

	for (u32 i = 0; i < folder->size(); i++)
		Level_Append((*folder)[i]);

	FS.file_list_close(folder);

#ifdef DEBUG
	folder = FS.file_list_open("$game_levels$", "$debug$\\", FS_ListFolders | FS_RootOnly);
	if (folder)
	{
		string_path tmp_path;
		for (u32 i = 0; i < folder->size(); i++)
		{
			strconcat(sizeof(tmp_path), tmp_path, "$debug$\\", (*folder)[i]);
			Level_Append(tmp_path);
		}

		FS.file_list_close(folder);
	}
#endif
}

void CApplication::Level_Set(u32 L)
{
	if (L >= Levels.size())
		return;

	Level_Current = L;
	FS.get_path("$level$")->_set(Levels[L].folder);

	string_path temp;
	string_path temp2;
	strconcat(sizeof(temp), temp, "intro\\intro_", Levels[L].folder);
	temp[xr_strlen(temp) - 1] = 0;

	if (FS.exist(temp2, "$game_textures$", temp, ".dds"))
		m_pRender->setLevelLogo(temp);
	else
		m_pRender->setLevelLogo("intro\\intro_no_start_picture");
}

int CApplication::Level_ID(LPCSTR name)
{
	char buffer[256];
	strconcat(sizeof(buffer), buffer, name, "\\");

	for (u32 I = 0; I < Levels.size(); I++)
	{
		if (0 == stricmp(buffer, Levels[I].folder))
			return int(I);
	}

	return -1;
}

void CApplication::load_draw_internal()
{
	m_pRender->load_draw_internal(*this);
}
