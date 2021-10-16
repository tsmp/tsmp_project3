// CDedicatedSrvConsole, used on dedicated server instead of CConsole

#pragma once
#include "Console.h"
#include "IGame_Level.h"

class ENGINE_API CDedicatedSrvConsole final : public CConsole
{
	using inherited = CConsole;

private:
	u32 m_LastStatisticUpdateTime;
	CServerInfo m_server_info;

	HWND m_hMainWnd;
	HWND m_hConsoleWnd;
	HWND m_hLogWnd;

	HFONT m_hLogWndFont;
	HFONT m_hPrevFont;
	HBRUSH m_hBackGroundBrush;

	HDC m_hdcLogWnd;
	HDC m_hdcBackBuffer;

	HBITMAP m_hBitmap;
	HBITMAP m_hOldBitmap;

public:
	CDedicatedSrvConsole();
	virtual ~CDedicatedSrvConsole();

	virtual void Initialize() final override;
	virtual void Destroy() final override;

	virtual void OnRender() final override {};
	virtual void OnFrame() final override;
	void OnPaint();

private:
	void CreateConsoleWnd();
	void CreateLogWnd();
	void DrawLog(HDC hDC, RECT* pRect);
};
