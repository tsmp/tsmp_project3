// CDedicatedSrvConsole, used on dedicated server instead of CConsole

#pragma once
#include "XR_IOConsole.h"
#include "IGame_Level.h"

class ENGINE_API CDedicatedSrvConsole :
	public CConsole
{
	using inherited = CConsole;

private:
	HWND hMainWnd;
	HWND hConsoleWnd;
	HWND hLogWnd;

	HFONT m_hLogWndFont;
	HFONT m_hPrevFont;
	HBRUSH m_hBackGroundBrush;

	HDC m_hDC_LogWnd;
	HDC m_hDC_LogWnd_BackBuffer;
	HBITMAP m_hBB_BM, m_hOld_BM;

	bool m_bNeedUpdate;
	u32	m_dwLastUpdateTime;

	CServerInfo server_info;
	bool m_bScrollLog;
	u32	m_dwStartLine;
	u32 m_last_time;

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
