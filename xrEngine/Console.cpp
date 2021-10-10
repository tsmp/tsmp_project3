// Console.cpp: implementation of the CConsole class.

#include "stdafx.h"
#include "igame_level.h"
#include "igame_persistent.h"
#include "Console.h"
#include "xr_input.h"
#include "Console_commands.h"
#include "GameFont.h"
#include "Render.h"
#include "line_editor/line_editor.h"

static u32 const default_font_color = color_rgba(250, 250, 250, 250);

constexpr auto LDIST = 0.05f;

ENGINE_API CConsole *Console = nullptr;
const char *ioc_prompt = ">>> ";
const char* ch_cursor = "_"; //"|";

text_editor::line_edit_control& CConsole::ec()
{
	return m_editor->control();
}

void CConsole::AddCommand(IConsole_Command *C)
{
	Commands[C->Name()] = C;
}

void CConsole::RemoveCommand(IConsole_Command *C)
{
	vecCMD_IT it = Commands.find(C->Name());

	if (Commands.end() != it)
		Commands.erase(it);
}

CConsole::CConsole()
{
	m_editor = xr_new<text_editor::line_editor>((u32)CONSOLE_BUF_SIZE);
	m_cmd_history_max = 64;
	Register_callbacks();
}

void CConsole::Initialize()
{
	pFont = nullptr;
	pFont2 = nullptr;
	m_pRender = nullptr;

	m_mouse_pos.x = 0;
	m_mouse_pos.y = 0;
	m_last_cmd = nullptr;

	m_cmd_history.reserve(m_cmd_history_max + 2);
	m_cmd_history.clear();
	reset_cmd_history_idx();

	// Commands
	extern void CCC_Register();
	CCC_Register();
}

CConsole::~CConsole()
{
	xr_delete(m_editor);
	Destroy();
}

void CConsole::Destroy()
{
	xr_delete(pFont);
	xr_delete(pFont2);

	if (m_pRender)
	{
		xr_delete(m_pRender);
		m_pRender = nullptr;
	}

	Commands.clear();
}

void CConsole::OnFrame()
{
	m_editor->on_frame();
}

void CConsole::OutFont(LPCSTR text, float& pos_y)
{
	float str_length = pFont->SizeOf_(text);
	float scr_width = 1.98f * Device.fWidth_2;

	if (str_length > scr_width) //1024.0f
	{
		float f = 0.0f;
		int sz = 0;
		int ln = 0;
		PSTR one_line = (PSTR)_alloca((CONSOLE_BUF_SIZE + 1) * sizeof(char));

		while (text[sz] && (ln + sz < CONSOLE_BUF_SIZE - 5))// перенос строк
		{
			one_line[ln + sz] = text[sz];
			one_line[ln + sz + 1] = 0;

			float t = pFont->SizeOf_(one_line + ln);

			if (t > scr_width)
			{
				OutFont(text + sz + 1, pos_y);
				pos_y -= LDIST;
				pFont->OutI(-1.0f, pos_y, "%s", one_line + ln);
				ln = sz + 1;
				f = 0.0f;
			}
			else			
				f = t;			

			++sz;
		}
	}
	else	
		pFont->OutI(-1.0f, pos_y, "%s", text);	
}

void CConsole::OnRender()
{
	if (!bVisible)	
		return;
	
	if (!pFont)
	{
		pFont = xr_new<CGameFont>("hud_font_di", CGameFont::fsDeviceIndependent);
		pFont->SetHeightI(0.025f);
	}

	if (!pFont2)
	{
		pFont2 = xr_new<CGameFont>("hud_font_di2", CGameFont::fsDeviceIndependent);
		pFont2->SetHeightI(0.025f);
	}

	if (!m_pRender)	
		m_pRender = Render->CreateConsoleRender();	

	bool bGame = false;

	if ((g_pGameLevel && g_pGameLevel->bReady) ||
		(g_pGamePersistent && g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive()))
	{
		bGame = true;
	}

	if (g_dedicated_server)	
		bGame = false;
	
	m_pRender->OnRender(bGame); //*** Shadow

	float fMaxY;
	float dwMaxY = (float)Device.dwHeight;

	if (bGame)
	{
		fMaxY = 0.0f;
		dwMaxY /= 2;
	}
	else	
		fMaxY = 1.0f;
	
	float ypos = fMaxY - LDIST * 1.1f;
	float scr_x = 1.0f / Device.fWidth_2;

	float scr_width = 1.9f * Device.fWidth_2;
	float ioc_d = pFont->SizeOf_(ioc_prompt);

	LPCSTR s_cursor = ec().str_before_cursor();
	LPCSTR s_b_mark = ec().str_before_mark();
	LPCSTR s_mark = ec().str_mark();
	LPCSTR s_mark_a = ec().str_after_mark();

	float str_length = ioc_d + pFont->SizeOf_(s_cursor);
	float out_pos = 0.0f;

	if (str_length > scr_width)
	{
		out_pos -= (str_length - scr_width);
		str_length = scr_width;
	}

	pFont->SetColor(color_rgba(228, 228, 255, 255));
	pFont->OutI(-1.0f + out_pos * scr_x, ypos, "%s", ioc_prompt);

	pFont->SetColor(color_rgba(138, 138, 245, 255));
	pFont2->SetColor(color_rgba(138, 138, 245, 255));

	out_pos += ioc_d;
	pFont->OutI(-1.0f + out_pos * scr_x, ypos, "%s", s_b_mark);		out_pos += pFont->SizeOf_(s_b_mark);
	pFont2->OutI(-1.0f + out_pos * scr_x, ypos, "%s", s_mark);		out_pos += pFont2->SizeOf_(s_mark);
	pFont->OutI(-1.0f + out_pos * scr_x, ypos, "%s", s_mark_a);

	if (ec().cursor_view())
	{
		pFont->SetColor(color_rgba(255, 255, 255, 255));
		pFont->OutI(-1.0f + str_length * scr_x, ypos, "%s", ch_cursor);
	}

	ypos -= LDIST;

	for (int i = LogFile->size() - 1 - scroll_delta; i >= 0; --i)
	{
		ypos -= LDIST;

		if (ypos < -1.0f)		
			break;
		
		LPCSTR ls = ((*LogFile)[i]).c_str();

		if (!ls)		
			continue;
		
		Console_mark cm = (Console_mark)ls[0];
		pFont->SetColor(get_mark_color(cm));
		u8 b = (is_mark(cm)) ? 2 : 0;
		OutFont(ls + b, ypos);
	}

	pFont->SetColor(color_rgba(205, 205, 25, 105));
	u32 log_line = LogFile->size() - 1;
	string16 q;
	itoa(log_line, q, 10);
	u32 qn = xr_strlen(q);
	pFont->OutI(0.95f - 0.02f * qn, fMaxY - 2.0f * LDIST, "[%d]", log_line);

	pFont->OnRender();
	pFont2->OnRender();
}

void CConsole::ExecuteCommand(LPCSTR cmd_str, bool record_cmd)
{
	u32  str_size = xr_strlen(cmd_str);
	PSTR edt = (PSTR)_alloca((str_size + 1) * sizeof(char));
	PSTR first = (PSTR)_alloca((str_size + 1) * sizeof(char));
	PSTR last = (PSTR)_alloca((str_size + 1) * sizeof(char));

	strcpy_s(edt, str_size + 1, cmd_str);
	edt[str_size] = 0;

	scroll_delta = 0;
	reset_cmd_history_idx();

	text_editor::remove_spaces(edt);
	if (edt[0] == 0)
	{
		return;
	}
	if (record_cmd)
	{
		char c[2];
		c[0] = static_cast<char>(Console_mark::mark2);
		c[1] = '\0';

		if (m_last_cmd.c_str() == 0 || xr_strcmp(m_last_cmd, edt) != 0)
		{
			Log(c, edt);
			add_cmd_history(edt);
			m_last_cmd = edt;
		}
	}
	text_editor::split_cmd(first, last, edt);

	// search
	vecCMD_IT it = Commands.find(first);
	if (it != Commands.end())
	{
		IConsole_Command* cc = it->second;
		if (cc && cc->bEnabled)
		{
			if (cc->bLowerCaseArgs)
			{
				strlwr(last);
			}
			if (last[0] == 0)
			{
				if (cc->bEmptyArgsHandled)
				{
					cc->Execute(last);
				}
				else
				{
					IConsole_Command::TStatus stat;
					cc->Status(stat);
					Msg("- %s %s", cc->Name(), stat);
				}
			}
			else
			{
				cc->Execute(last);
			}
		}
		else
		{
			Log("! Command disabled.");
		}
	}
	else
	{
		first[CONSOLE_BUF_SIZE - 21] = 0;
		Log("! Unknown command: ", first);
	}

	if (record_cmd)
	{
		ec().clear_states();
	}
}

void CConsole::Show()
{
	if (bVisible)	
		return;
	
	bVisible = true;
	GetCursorPos(&m_mouse_pos);

	ec().clear_states();
	scroll_delta = 0;
	reset_cmd_history_idx();

	m_editor->IR_Capture();
	Device.seqRender.Add(this, 1);
	Device.seqFrame.Add(this);
}

void CConsole::Hide()
{
	if (!bVisible)
		return;

	if (g_pGamePersistent && g_dedicated_server)
		return;

	//if (pInput->get_exclusive_mode())	
	//	SetCursorPos(m_mouse_pos.x, m_mouse_pos.y);
	

	bVisible = false;
	Device.seqFrame.Remove(this);
	Device.seqRender.Remove(this);
	m_editor->IR_Release();
}

void CConsole::SelectCommand()
{
	if (m_cmd_history.empty())
	{
		return;
	}
	VERIFY(0 <= m_cmd_history_idx && m_cmd_history_idx < (int)m_cmd_history.size());

	xr_vector<shared_str>::reverse_iterator	it_rb = m_cmd_history.rbegin() + m_cmd_history_idx;
	ec().set_edit((*it_rb).c_str());
}

void CConsole::Execute(LPCSTR cmd)
{
	ExecuteCommand(cmd, false);
}

void CConsole::ExecuteScript(LPCSTR str)
{
	u32  str_size = xr_strlen(str);
	PSTR buf = (PSTR)_alloca((str_size + 10) * sizeof(char));
	strcpy_s(buf, str_size + 10, "cfg_load ");
	strcat_s(buf, str_size + 10, str);
	Execute(buf);
}

void CConsole::add_cmd_history(shared_str const& str)
{
	if (str.size() == 0)
	{
		return;
	}
	m_cmd_history.push_back(str);
	if (m_cmd_history.size() > m_cmd_history_max)
	{
		m_cmd_history.erase(m_cmd_history.begin());
	}
}

void CConsole::next_cmd_history_idx()
{
	--m_cmd_history_idx;
	if (m_cmd_history_idx < 0)
	{
		m_cmd_history_idx = 0;
	}
}

void CConsole::prev_cmd_history_idx()
{
	++m_cmd_history_idx;
	if (m_cmd_history_idx >= (int)m_cmd_history.size())
	{
		m_cmd_history_idx = m_cmd_history.size() - 1;
	}
}

void CConsole::reset_cmd_history_idx()
{
	m_cmd_history_idx = -1;
}

u32 CConsole::get_mark_color(Console_mark type)
{
	u32 color = default_font_color;

	switch (type)
	{
	case Console_mark::mark0:
		color = color_rgba(255, 255, 0, 255);
		break;
	case Console_mark::mark1:
		color = color_rgba(255, 0, 0, 255);
		break;
	case Console_mark::mark2:
		color = color_rgba(100, 100, 255, 255);
		break;
	case Console_mark::mark3:
		color = color_rgba(0, 222, 205, 155);
		break;
	case Console_mark::mark4:
		color = color_rgba(255, 0, 255, 255);
		break;
	case Console_mark::mark5:
		color = color_rgba(155, 55, 170, 155);
		break;
	case Console_mark::mark6:
		color = color_rgba(25, 200, 50, 255);
		break;
	case Console_mark::mark7:
		color = color_rgba(255, 255, 0, 255);
		break;
	case Console_mark::mark8:
		color = color_rgba(128, 128, 128, 255);
		break;
	case Console_mark::mark9:
		color = color_rgba(0, 255, 0, 255);
		break;
	case Console_mark::mark10:
		color = color_rgba(55, 155, 140, 255);
		break;
	case Console_mark::mark11:
		color = color_rgba(205, 205, 105, 255);
		break;
	case Console_mark::mark12:
		color = color_rgba(128, 128, 250, 255);
		break;
	case Console_mark::no_mark:
	default:
		break;
	}

	return color;
}

bool CConsole::is_mark(Console_mark type)
{
	switch (type)
	{
	case Console_mark::mark0:
	case Console_mark::mark1:
	case Console_mark::mark2:
	case Console_mark::mark3:
	case Console_mark::mark4:
	case Console_mark::mark5:
	case Console_mark::mark6:
	case Console_mark::mark7:
	case Console_mark::mark8:
	case Console_mark::mark9:
	case Console_mark::mark10:
	case Console_mark::mark11:
	case Console_mark::mark12:
		return true;
		break;
	}

	return false;
}
