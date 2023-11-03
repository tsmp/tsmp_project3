#pragma once
#include "iinputreceiver.h"

extern char const* const ioc_prompt;
extern char const* const ch_cursor;

struct TipString
{
	shared_str text;
	int HL_start; // Highlight
	int HL_finish;

	TipString()
	{
		text._set("");
		HL_start = 0;
		HL_finish = 0;
	}

	TipString(shared_str const& tips_text, int start_pos, int finish_pos)
	{
		text._set(tips_text);
		HL_start = start_pos;
		HL_finish = finish_pos;
	}

	TipString(LPCSTR tips_text, int start_pos, int finish_pos)
	{
		text._set(tips_text);
		HL_start = start_pos;
		HL_finish = finish_pos;
	}

	TipString(shared_str const& tips_text)
	{
		text._set(tips_text);
		HL_start = 0;
		HL_finish = 0;
	}

	IC bool operator== (shared_str const& tips_text)
	{
		return (text == tips_text);
	}
};

namespace text_editor
{
	class line_editor;
	class line_edit_control;
};

//refs
class ENGINE_API CGameFont;
class ENGINE_API IConsole_Command;


//public pureScreenResolutionChanged

class ENGINE_API CConsole : 
	public IInputReceiver,
	public pureRender,
	public pureFrame
{
public:
	//t-defs
	struct str_pred
	{
		IC bool operator()(const char *x, const char *y) const
		{
			return xr_strcmp(x, y) < 0;
		}
	};

	using vecCMD = xr_map<LPCSTR, IConsole_Command *, str_pred>;
	using vecCMD_IT = vecCMD::iterator;
	using vecCMD_CIT = vecCMD::const_iterator;
	using Callback = fastdelegate::FastDelegate0<void>;
	using vecHistory = xr_vector<shared_str>;
	using vecTips = xr_vector<shared_str>;
	using vecTipsEx = xr_vector<TipString>;

	enum
	{
		CONSOLE_BUF_SIZE = 1024
	};

	enum 
	{ 
		VIEW_TIPS_COUNT = 14, 
		MAX_TIPS_COUNT = 220 
	};

protected:
	int scroll_delta;

	CGameFont* pFont;
	CGameFont* pFont2;

	bool m_disable_tips;
	POINT m_mouse_pos;

private:
	vecHistory m_cmd_history;
	u32 m_cmd_history_max;
	int	m_cmd_history_idx;
	shared_str m_last_cmd;

	vecTips m_temp_tips;
	vecTipsEx m_tips;
	u32 m_tips_mode;
	shared_str m_cur_cmd;
	int m_select_tip;
	int m_start_tip;
	u32 m_prev_length_str;
	static LPCSTR radmin_cmd_name;
	bool m_HasRaPrefix;

public:

	CConsole();
	virtual ~CConsole();

	virtual void Initialize();
	virtual void Destroy();

	virtual void OnRender();
	virtual void OnFrame();
	virtual void OnScreenResolutionChanged();

	string64 ConfigFile;
	bool bVisible;
	vecCMD Commands;

	void AddCommand(IConsole_Command *);
	void RemoveCommand(IConsole_Command *);
	//void Reset();

	void Show();
	void Hide();

	void Execute(LPCSTR cmd);
	void ExecuteScript(LPCSTR name);
	void ExecuteCommand(LPCSTR cmd_str, bool record_cmd = true);
	void SelectCommand();

	void RegisterScreenshotCallback(int screenshotDik);

	// get
	bool GetBool(LPCSTR cmd, bool &val);
	float GetFloat(LPCSTR cmd, float &val, float &min, float &max);
	char *GetString(LPCSTR cmd);
	int GetInteger(LPCSTR cmd, int &val, int &min, int &max);
	char *GetToken(LPCSTR cmd);
	xr_token *GetXRToken(LPCSTR cmd);

protected:

	text_editor::line_editor* m_editor;
	text_editor::line_edit_control& ec();

	enum class Console_mark // (int)=char
	{
		no_mark = ' ',
		mark0 = '~',  // €рко желтый
		mark1 = '!',  // €рко красный, ошибка
		mark2 = '@',  // €рко синий, консоль
		mark3 = '#',  // сине-зеленый
		mark4 = '$',  // розовый
		mark5 = '%',  // тускло-белый
		mark6 = '^',  // тускло-зеленый
		mark7 = '&',  // тускло-желтый
		mark8 = '*',  // серый
		mark9 = '-',  // зеленый, ок
		mark10 = '+', // темновато-голубовато-зеленый
		mark11 = '=', // ооочень тускло-желтый
		mark12 = '/'  // розовато-синий
	};

	bool is_mark(Console_mark type);
	u32 get_mark_color(Console_mark type);

	void DrawBackgrounds(bool bGame);
	void DrawRect(Frect const& r, u32 color);

	void OutFont(LPCSTR text, float& pos_y);
	void Register_callbacks();
	u32 GetRadminCMDOffset(const char* cmdStr);

protected:
	void xr_stdcall Prev_log();
	void xr_stdcall Next_log();
	void xr_stdcall Begin_log();
	void xr_stdcall End_log();

	void xr_stdcall Find_cmd();
	void xr_stdcall Find_cmd_back();
	void xr_stdcall Prev_cmd();
	void xr_stdcall Next_cmd();

	void xr_stdcall Execute_cmd();
	void xr_stdcall Show_cmd();
	void xr_stdcall Hide_cmd();
	void xr_stdcall GamePause();

	void xr_stdcall Prev_tip();
	void xr_stdcall Next_tip();

	void xr_stdcall Begin_tips();
	void xr_stdcall End_tips();
	void xr_stdcall PageUp_tips();
	void xr_stdcall PageDown_tips();
	void xr_stdcall Hide_cmd_esc();

	void xr_stdcall ScreenshotCmd();

protected:
	void add_cmd_history(shared_str const& str);
	void next_cmd_history_idx();
	void prev_cmd_history_idx();
	void reset_cmd_history_idx();

	void next_selected_tip();
	void check_next_selected_tip();
	void prev_selected_tip();
	void check_prev_selected_tip();
	void reset_selected_tip();

	IConsole_Command* find_next_cmd(LPCSTR in_str, shared_str& out_str);
	void AddFilteredCommands(LPCSTR inStr, vecTipsEx& outVec);

	void update_tips();
	void select_for_filter(LPCSTR filter_str, vecTips& in_v, vecTipsEx& out_v);
};

ENGINE_API extern CConsole *Console;
