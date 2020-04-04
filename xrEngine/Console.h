// XR_IOConsole.h: interface for the CConsole class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include "iinputreceiver.h"

//refs
class ENGINE_API CGameFont;
class ENGINE_API IConsole_Command;

class ENGINE_API CConsole : public IInputReceiver,
							public pureRender,
							public pureFrame
{
public:
	//t-defs
	struct str_pred : public std::binary_function<char*, char*, bool> 
	{	
		IC bool operator()(const char* x, const char* y) const
		{	
			return xr_strcmp(x,y)<0;
		}
	};
	
	using vecCMD = xr_map<LPCSTR,IConsole_Command*,str_pred>;
	using vecCMD_IT = vecCMD::iterator;
	
	enum { MAX_LEN = 1024 };
private:
	u32 last_mm_timer;
	float cur_time;
	float rep_time;
	float fAccel;
	
	int	cmd_delta;
	int	old_cmd_delta;
	
	char* editor_last;
	bool bShift;
	bool bCtrl;
	
	bool bRepeat;
	bool RecordCommands;
protected:
	int	scroll_delta;
	char editor[MAX_LEN];
	char command[MAX_LEN];
	bool bCursor;
	bool bRussian;

	CGameFont* pFont;

	enum class Console_mark // (int)=char
	{
		no_mark = ' ',
		mark0 = '~',	// €рко желтый
		mark1 = '!',	// €рко красный, ошибка
		mark2 = '@',	// €рко синий, консоль
		mark3 = '#',	// сине-зеленый
		mark4 = '$',	// розовый
		mark5 = '%',	// тускло-белый
		mark6 = '^',	// тускло-зеленый
		mark7 = '&',	// тускло-желтый
		mark8 = '*',	// серый
		mark9 = '-',	// зеленый, ок
		mark10 = '+',	// темновато-голубовато-зеленый
		mark11 = '=',	// ооочень тускло-желтый
		mark12 = '/'	// розовато-синий
	};

	bool is_mark(Console_mark type);
	u32 get_mark_color(Console_mark type);

public:
	virtual ~CConsole(){};

	string64 ConfigFile;
	bool bVisible;
	vecCMD Commands;

	void AddCommand(IConsole_Command*);
	void RemoveCommand(IConsole_Command*);
	void Reset();

	void Show();
	void Hide();

	void Execute(LPCSTR cmd);
	void ExecuteScript(LPCSTR name);
	void ExecuteCommand();

	// get
	bool GetBool(LPCSTR cmd, bool &val);
	float GetFloat(LPCSTR cmd, float& val, float& min, float& max);
	char* GetString(LPCSTR cmd);
	int	GetInteger(LPCSTR cmd, int& val, int& min, int& max);
	char* GetToken(LPCSTR cmd);
	xr_token* GetXRToken(LPCSTR cmd);

	void SelectCommand();

	// keyboard
	void OnPressKey(int dik, BOOL bHold=false);
	virtual void IR_OnKeyboardPress(int dik);
	virtual void IR_OnKeyboardHold(int dik);
	virtual void IR_OnKeyboardRelease(int dik);

	// render & onmove
	virtual void OnRender();
	virtual void OnFrame();

	virtual	void Initialize();
	virtual void Destroy();
};

ENGINE_API extern CConsole* Console;
