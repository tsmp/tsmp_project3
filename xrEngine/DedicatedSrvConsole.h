// CDedicatedSrvConsole, used on dedicated server instead of CConsole

#pragma once
#include "Console.h"
#include "IGame_Level.h"

class ENGINE_API CDedicatedSrvConsole final :
	public CConsole
{
	using inherited = CConsole;

private:
	HWND hMainWnd;
	HWND hConsoleWnd;
	HWND hLogWnd;

	HFONT hLogWndFont;
	HFONT hPrevFont;
	HBRUSH hBackGroundBrush;

	HDC hdcLogWnd;
	HDC hdcBackBuffer;

	HBITMAP hBitmap;
	HBITMAP hOldBitmap;

	bool bNeedUpdate;
	u32	LastUpdateTime;

	CServerInfo server_info;
	bool bScrollLog;
	
	u32 LastStatisticsUpdate;

private:
	void CreateConsoleWnd();
	void CreateLogWnd();
	void DrawLog(HDC hDC, RECT* pRect);

public:
	CDedicatedSrvConsole();
	virtual ~CDedicatedSrvConsole();

	virtual	void Initialize();
	virtual	void Destroy();

	void OnPaint();

	virtual void OnRender() {};
	virtual void OnFrame();

	virtual void IR_OnKeyboardPress(int dik);
	virtual void IR_OnKeyboardHold(int dik);
	virtual void IR_OnKeyboardRelease(int dik);
};
