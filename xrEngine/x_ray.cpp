// File: x_ray.cpp
//
// Programmers:
//	Oles		- Oles Shishkovtsov
//	AlexMX		- Alexander Maksimchuk

#include "stdafx.h"
#include "igame_level.h"
#include "igame_persistent.h"

#include "xr_input.h"
#include "Console.h"
#include "x_ray.h"
#include "std_classes.h"
#include "GameFont.h"
#include "resource.h"
#include "LightAnimLibrary.h"
#include "ispatial.h"
#include "DedicatedSrvConsole.h"
#include <process.h>

#include "..\TSMP3_Build_Config.h"
#include "Console_commands.h"

#define TRIVIAL_ENCRYPTOR_DECODER
#include "trivial_encryptor.h"
#include <debugapi.h>

// global variables
ENGINE_API CApplication* pApp = nullptr;

ENGINE_API string512 g_sLaunchOnExit_params;
ENGINE_API string512 g_sLaunchOnExit_app;

typedef void DUMMY_STUFF(const void*, const u32&, void*);
XRCORE_API DUMMY_STUFF* g_temporary_stuff;

ENGINE_API bool g_dedicated_server = false;

ENGINE_API CInifile *pGameIni = nullptr;
int max_load_stage = 0;

// computing build id
XRCORE_API LPCSTR build_date;
XRCORE_API u32 build_id;

static LPSTR month_id[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static int days_in_month[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int start_day = 31;	  // 31
static int start_month = 1;	  // January
static int start_year = 1999; // 1999

void compute_build_id()
{
	build_date = __DATE__;

	int days;
	int months = 0;
	int years;
	string16 month;
	string256 buffer;
	strcpy_s(buffer, __DATE__);
	sscanf(buffer, "%s %d %d", month, &days, &years);

	for (int i = 0; i < 12; i++)
	{
		if (_stricmp(month_id[i], month))
			continue;

		months = i;
		break;
	}

	build_id = (years - start_year) * 365 + days - start_day;

	for (int i = 0; i < months; ++i)
		build_id += days_in_month[i];

	for (int i = 0; i < start_month - 1; ++i)
		build_id -= days_in_month[i];
}

// Класс для проверки наличия нескольких запущенных копий игры
class GameInstancesChecker
{
	HANDLE m_PresenceMutex;
	const char* m_MutexName = "STALKER-SoC";

public:

#ifdef NO_MULTI_INSTANCES // Запрещено запускать несколько экземпляров игры

	bool IsAnotherGameInstanceLaunched()
	{
		HANDLE presenceMutex = OpenMutex(READ_CONTROL, FALSE, m_MutexName);

		// Мьютекс с таким именем уже существует в системе
		if (presenceMutex != NULL)
		{
			CloseHandle(presenceMutex);
			return true;
		}

		return false;
	}

	void RegisterThisInstanceLaunched()
	{
		m_PresenceMutex = CreateMutex(NULL, FALSE, m_MutexName);
	}

	void UnregisterThisInstanceLaunched()
	{
		CloseHandle(m_PresenceMutex);
	}

#else // Можно запускать несколько экземпляров

	bool IsAnotherGameInstanceLaunched() { return false; }
	void RegisterThisInstanceLaunched() {}
	void UnregisterThisInstanceLaunched() {}

#endif // NO_MULTI_INSTANCES
};

struct _SoundProcessor : public pureFrame
{
	virtual void OnFrame()
	{
		Device.Statistic->Sound.Begin();
		::Sound->update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);
		Device.Statistic->Sound.End();
	}
} SoundProcessor;

extern void TryLoadXrCustomResDll();
extern void TryToChangeLogoImageToCustom(HWND logoWindow);

namespace Logo
{
	static HWND logoWindow = nullptr;

	void Create()
	{
#ifdef DEBUG
		bool isTopMost = false;
#else
		bool isTopMost = !IsDebuggerPresent();
#endif

		HWND topMost = isTopMost ? HWND_TOPMOST : HWND_NOTOPMOST;
		logoWindow = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_STARTUP), 0, 0);

		UINT flags = SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW;
		SetWindowPos(logoWindow, topMost, 0, 0, 0, 0, flags);				
		TryToChangeLogoImageToCustom(logoWindow);
	}

	void Destroy()
	{
		DestroyWindow(logoWindow);
		logoWindow = nullptr;
	}
}

void InitializeSettings()
{
	string_path fname;
	FS.update_path(fname, "$game_config$", "system.ltx");
	pSettings = xr_new<CInifile>(fname, TRUE);
	CHECK_OR_EXIT(!pSettings->sections().empty(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));

	FS.update_path(fname, "$game_config$", "game.ltx");
	pGameIni = xr_new<CInifile>(fname, TRUE);
	CHECK_OR_EXIT(!pGameIni->sections().empty(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
}

void LaunchOnExit()
{
	char* _args[3];
	// check for need to execute something external
	if (xr_strlen(g_sLaunchOnExit_app))
	{
		string4096 ModuleFileName = "";
		GetModuleFileName(NULL, ModuleFileName, 4096);

		string4096 ModuleFilePath = "";
		char* ModuleName = NULL;
		GetFullPathName(ModuleFileName, 4096, ModuleFilePath, &ModuleName);
		ModuleName[0] = 0;
		strcat(ModuleFilePath, g_sLaunchOnExit_app);
		_args[0] = g_sLaunchOnExit_app;
		_args[1] = g_sLaunchOnExit_params;
		_args[2] = NULL;

		_spawnv(_P_NOWAIT, _args[0], _args); //, _envvar);
	}
}

void InitializeConsole()
{
#ifdef DEDICATED_SERVER	
		Console = xr_new<CDedicatedSrvConsole>();	
#else
		Console = xr_new<CConsole>();	
#endif

	Console->Initialize();
	strcpy_s(Console->ConfigFile, "user.ltx");

	if (strstr(Core.Params, "-ltx "))
	{
		string64 c_name;
		sscanf(strstr(Core.Params, "-ltx ") + 5, "%[^ ] ", c_name);
		strcpy_s(Console->ConfigFile, c_name);
	}
}

void InitInput()
{
	BOOL bCaptureInput = !strstr(Core.Params, "-i");

	if (g_dedicated_server)
		bCaptureInput = FALSE;

	pInput = xr_new<CInput>(bCaptureInput);
}

void destroyInput()
{
	xr_delete(pInput);
}

void InitSound()
{
	CSound_manager_interface::_create(u64(Device.m_hWnd));
}

void destroySound()
{
	CSound_manager_interface::_destroy();
}

void destroySettings()
{
	xr_delete(pSettings);
	xr_delete(pGameIni);
}

void destroyConsole()
{
	Console->Destroy();
	xr_delete(Console);
}

void destroyEngine()
{
	Device.Destroy();
	Engine.Destroy();
}

void execUserScript()
{
	// Execute script

	Console->Execute("unbindall");
	Console->ExecuteScript(Console->ConfigFile);
}

void InitializeCore(char* lpCmdLine)
{
	LPCSTR fsgame_ltx_name = "-fsltx ";
	string_path fsgame = "";

	if (strstr(lpCmdLine, fsgame_ltx_name))
	{
		int sz = xr_strlen(fsgame_ltx_name);
		sscanf(strstr(lpCmdLine, fsgame_ltx_name) + sz, "%[^ ] ", fsgame);
	}

	g_temporary_stuff = &trivial_encryptor::decode;

	compute_build_id();
	Core._initialize("xray", NULL, TRUE, fsgame[0] ? fsgame : NULL);
}

// Фунция для тупых требований THQ и тупых американских пользователей
BOOL IsOutOfVirtualMemory()
{
#define VIRT_ERROR_SIZE 256
#define VIRT_MESSAGE_SIZE 512

	MEMORYSTATUSEX statex;
	DWORD dwPageFileInMB = 0;
	DWORD dwPhysMemInMB = 0;
	HINSTANCE hApp = 0;
	char pszError[VIRT_ERROR_SIZE];
	char pszMessage[VIRT_MESSAGE_SIZE];

	ZeroMemory(&statex, sizeof(MEMORYSTATUSEX));
	statex.dwLength = sizeof(MEMORYSTATUSEX);

	if (!GlobalMemoryStatusEx(&statex))
		return 0;

	dwPageFileInMB = (DWORD)(statex.ullTotalPageFile / (1024 * 1024));
	dwPhysMemInMB = (DWORD)(statex.ullTotalPhys / (1024 * 1024));

	// Довольно отфонарное условие
	if ((dwPhysMemInMB > 500) && ((dwPageFileInMB + dwPhysMemInMB) > 2500))
		return 0;

	hApp = GetModuleHandle(NULL);

	if (!LoadString(hApp, RC_VIRT_MEM_ERROR, pszError, VIRT_ERROR_SIZE))
		return 0;

	if (!LoadString(hApp, RC_VIRT_MEM_TEXT, pszMessage, VIRT_MESSAGE_SIZE))
		return 0;

	MessageBox(NULL, pszMessage, pszError, MB_OK | MB_ICONHAND);
	return 1;
}

void InitializeApplication()
{
	ShowWindow(Device.m_hWnd, SW_SHOWNORMAL);
	Device.Create();
	LALib.OnCreate();

	pApp = xr_new<CApplication>();
	g_pGamePersistent = (IGame_Persistent*)NEW_INSTANCE(CLSID_GAME_PERSISTANT);
	g_SpatialSpace = xr_new<ISpatial_DB>();
	g_SpatialSpacePhysic = xr_new<ISpatial_DB>();
}

void EngineInitialize(char* lpCmdLine)
{
	Logo::Create();	
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	
	g_sLaunchOnExit_app[0] = g_sLaunchOnExit_params[0] = '\0';

	InitializeCore(lpCmdLine);
	InitializeSettings();

	FPU::m24r();	
	Engine.Initialize();
	Device.Initialize();

	InitializeConsole();

	if (strstr(Core.Params, "-r2a"))
		Console->Execute("renderer renderer_r2a");
	else if (strstr(Core.Params, "-r2"))
		Console->Execute("renderer renderer_r2");
	else
	{
		CCC_LoadCFG_custom* pTmp = xr_new<CCC_LoadCFG_custom>("renderer ");
		pTmp->Execute(Console->ConfigFile);
		xr_delete(pTmp);
	}

	InitInput();
	Engine.External.Initialize();
	Console->Execute("stat_memory");

	execUserScript();
	InitSound();

	// ...command line for auto start
	if(LPCSTR pStartup = strstr(Core.Params, "-start "))
		Console->Execute(pStartup + 1);
	
	if(LPCSTR pStartup = strstr(Core.Params, "-load "))
		Console->Execute(pStartup + 1);

	InitializeApplication();
	Logo::Destroy();
}

void EngineRoutine()
{
	// Main cycle
	Memory.mem_usage();
	Device.Run();
}

void EngineDestroy()
{
	// Destroy APP
	xr_delete(g_SpatialSpacePhysic);
	xr_delete(g_SpatialSpace);
	DEL_INSTANCE(g_pGamePersistent);
	xr_delete(pApp);
	Engine.Event.Dump();

	// Destroying
	destroySound();
	destroyInput();
	destroySettings();

	LALib.OnDestroy();

	destroyConsole();
	destroyEngine();

	Core._destroy();
	LaunchOnExit();
}

int APIENTRY WinMainImplementation(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow)
{
	TryLoadXrCustomResDll();

#ifdef DEDICATED_SERVER
	g_dedicated_server = true;
#else

	if ((strstr(lpCmdLine, "-skipmemcheck") == NULL) && IsOutOfVirtualMemory())
		return 0;

	GameInstancesChecker gameInstances;

	if (gameInstances.IsAnotherGameInstanceLaunched())
		return 0;

	gameInstances.RegisterThisInstanceLaunched();

#endif // DEDICATED_SERVER
		
	EngineInitialize(lpCmdLine);
	EngineRoutine();
	EngineDestroy();

#ifndef DEDICATED_SERVER
	gameInstances.UnregisterThisInstanceLaunched();
#endif // DEDICATED_SERVER

	return 0;
}

int stack_overflow_exception_filter(int exception_code)
{
	if (exception_code == EXCEPTION_STACK_OVERFLOW)
	{
		// Do not call _resetstkoflw here, because
		// at this point, the stack is not yet unwound.
		// Instead, signal that the handler (the __except block)
		// is to be executed.
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
		return EXCEPTION_CONTINUE_SEARCH;
}

int APIENTRY WinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 char *lpCmdLine,
					 int nCmdShow)
{
#if defined(DEDICATED_SERVER) || defined(SAVE_ERROR_REPORTS)
	Debug._initialize(true);
#else  
	Debug._initialize(false);
#endif

	__try
	{
		WinMainImplementation(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}
	__except (stack_overflow_exception_filter(GetExceptionCode()))
	{
		_resetstkoflw();
		FATAL("stack overflow");
	}

	return 0;
}

LPCSTR _GetFontTexName(LPCSTR section)
{
	static char *tex_names[] = {"texture800", "texture", "texture1600"};
	int def_idx = 1; //default 1024x768
	int idx = def_idx;

	u32 h = Device.dwHeight;

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

void _InitializeFont(CGameFont *&F, LPCSTR section, u32 flags)
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

	// events
	eQuit = Engine.Event.Handler_Attach("KERNEL:quit", this);
	eStart = Engine.Event.Handler_Attach("KERNEL:start", this);
	eStartLoad = Engine.Event.Handler_Attach("KERNEL:load", this);
	eDisconnect = Engine.Event.Handler_Attach("KERNEL:disconnect", this);

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

			g_pGameLevel = (IGame_Level *)NEW_INSTANCE(CLSID_GAME_LEVEL);
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

		ll_hGeom.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.QuadIB);
		sh_progress.create("hud\\default", "ui\\ui_load");
		ll_hGeom2.create(FVF::F_TL, RCache.Vertex.Buffer(), NULL);
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
	hLevelLogo.destroy();
	sh_progress.destroy();
}

u32 calc_progress_color(u32, u32, int, int);

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
	xr_vector<char *> *folder = FS.file_list_open("$game_levels$", FS_ListFolders | FS_RootOnly);
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
		hLevelLogo.create("font", temp);
	else
		hLevelLogo.create("font", "intro\\intro_no_start_picture");
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

#pragma optimize("g", off)
void CApplication::load_draw_internal()
{
	if (!sh_progress)
	{
		CHK_DX(HW.pDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1, 0));
		return;
	}
	// Draw logo
	u32 Offset;
	u32 C = 0xffffffff;
	u32 _w = Device.dwWidth;
	u32 _h = Device.dwHeight;
	FVF::TL *pv = NULL;

	//progress
	float bw = 1024.0f;
	float bh = 768.0f;
	Fvector2 k;
	k.set(float(_w) / bw, float(_h) / bh);

	RCache.set_Shader(sh_progress);
	CTexture *T = RCache.get_ActiveTexture(0);
	Fvector2 tsz;
	tsz.set((float)T->get_Width(), (float)T->get_Height());
	Frect back_text_coords;
	Frect back_coords;
	Fvector2 back_size;

	//progress background
	static float offs = -0.5f;

	back_size.set(1024, 768);
	back_text_coords.lt.set(0, 0);
	back_text_coords.rb.add(back_text_coords.lt, back_size);
	back_coords.lt.set(offs, offs);
	back_coords.rb.add(back_coords.lt, back_size);

	back_coords.lt.mul(k);
	back_coords.rb.mul(k);

	back_text_coords.lt.x /= tsz.x;
	back_text_coords.lt.y /= tsz.y;
	back_text_coords.rb.x /= tsz.x;
	back_text_coords.rb.y /= tsz.y;
	pv = (FVF::TL *)RCache.Vertex.Lock(4, ll_hGeom.stride(), Offset);
	pv->set(back_coords.lt.x, back_coords.rb.y, C, back_text_coords.lt.x, back_text_coords.rb.y);
	pv++;
	pv->set(back_coords.lt.x, back_coords.lt.y, C, back_text_coords.lt.x, back_text_coords.lt.y);
	pv++;
	pv->set(back_coords.rb.x, back_coords.rb.y, C, back_text_coords.rb.x, back_text_coords.rb.y);
	pv++;
	pv->set(back_coords.rb.x, back_coords.lt.y, C, back_text_coords.rb.x, back_text_coords.lt.y);
	pv++;
	RCache.Vertex.Unlock(4, ll_hGeom.stride());

	RCache.set_Geometry(ll_hGeom);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

	//progress bar
	back_size.set(268, 37);
	back_text_coords.lt.set(0, 768);
	back_text_coords.rb.add(back_text_coords.lt, back_size);
	back_coords.lt.set(379, 726);
	back_coords.rb.add(back_coords.lt, back_size);

	back_coords.lt.mul(k);
	back_coords.rb.mul(k);

	back_text_coords.lt.x /= tsz.x;
	back_text_coords.lt.y /= tsz.y;
	back_text_coords.rb.x /= tsz.x;
	back_text_coords.rb.y /= tsz.y;

	u32 v_cnt = 40;
	pv = (FVF::TL *)RCache.Vertex.Lock(2 * (v_cnt + 1), ll_hGeom2.stride(), Offset);
	FVF::TL *_pv = pv;
	float pos_delta = back_coords.width() / v_cnt;
	float tc_delta = back_text_coords.width() / v_cnt;
	u32 clr = C;

	for (u32 idx = 0; idx < v_cnt + 1; ++idx)
	{
		clr = calc_progress_color(idx, v_cnt, load_stage, max_load_stage);
		pv->set(back_coords.lt.x + pos_delta * idx + offs, back_coords.rb.y + offs, 0 + EPS_S, 1, clr, back_text_coords.lt.x + tc_delta * idx, back_text_coords.rb.y);
		pv++;
		pv->set(back_coords.lt.x + pos_delta * idx + offs, back_coords.lt.y + offs, 0 + EPS_S, 1, clr, back_text_coords.lt.x + tc_delta * idx, back_text_coords.lt.y);
		pv++;
	}
	VERIFY(u32(pv - _pv) == 2 * (v_cnt + 1));
	RCache.Vertex.Unlock(2 * (v_cnt + 1), ll_hGeom2.stride());

	RCache.set_Geometry(ll_hGeom2);
	RCache.Render(D3DPT_TRIANGLESTRIP, Offset, 2 * v_cnt);

	// Draw title
	VERIFY(pFontSystem);
	pFontSystem->Clear();
	pFontSystem->SetColor(color_rgba(157, 140, 120, 255));
	pFontSystem->SetAligment(CGameFont::alCenter);
	pFontSystem->OutI(0.f, 0.815f, app_title);
	pFontSystem->OnRender();

	//draw level-specific screenshot
	if (hLevelLogo)
	{
		Frect r;
		r.lt.set(257, 369);
		r.lt.x += offs;
		r.lt.y += offs;
		r.rb.add(r.lt, Fvector2().set(512, 256));
		r.lt.mul(k);
		r.rb.mul(k);
		pv = (FVF::TL *)RCache.Vertex.Lock(4, ll_hGeom.stride(), Offset);
		pv->set(r.lt.x, r.rb.y, C, 0, 1);
		pv++;
		pv->set(r.lt.x, r.lt.y, C, 0, 0);
		pv++;
		pv->set(r.rb.x, r.rb.y, C, 1, 1);
		pv++;
		pv->set(r.rb.x, r.lt.y, C, 1, 0);
		pv++;
		RCache.Vertex.Unlock(4, ll_hGeom.stride());

		RCache.set_Shader(hLevelLogo);
		RCache.set_Geometry(ll_hGeom);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}

u32 calc_progress_color(u32 idx, u32 total, int stage, int max_stage)
{
	if (idx > (total / 2))
		idx = total - idx;

	float kk = (float(stage + 1) / float(max_stage)) * (total / 2.0f);
	float f = 1 / (exp((float(idx) - kk) * 0.5f) + 1.0f);

	return color_argb_f(f, 1.0f, 1.0f, 1.0f);
}
