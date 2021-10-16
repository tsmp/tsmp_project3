#include "stdafx.h"
#include "DedicatedSrvConsole.h"
#include "line_editor/line_editor.h"

LRESULT CALLBACK TextConsole_LogWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND:
		return (LRESULT)1; // Say we handled it.

	case WM_PAINT:
	{
		CDedicatedSrvConsole *pConsole = (CDedicatedSrvConsole *)Console;
		pConsole->OnPaint();
		return (LRESULT)0; // Say we handled it.
	}
	break;

	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

CDedicatedSrvConsole::CDedicatedSrvConsole()
{
	m_hMainWnd = nullptr;
	m_hConsoleWnd = nullptr;
	m_hLogWnd = nullptr;
	m_hLogWndFont = nullptr;

	m_LastStatisticUpdateTime = Device.dwTimeGlobal;
}

CDedicatedSrvConsole::~CDedicatedSrvConsole()
{
	m_hMainWnd = nullptr;
}

void CDedicatedSrvConsole::CreateConsoleWnd()
{
	HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);

	RECT rct;
	GetClientRect(m_hMainWnd, &rct);

	INT lX = rct.left;
	INT lY = rct.top;
	INT lWidth = rct.right - rct.left;
	INT lHeight = rct.bottom - rct.top;

	const char *wndClassName = "TEXT_CONSOLE";

	// Register the windows class
	WNDCLASS wndClass =
	{
			0, DefWindowProc, 0, 0, hInstance, NULL, LoadCursor(hInstance, IDC_ARROW), GetStockBrush(GRAY_BRUSH), NULL, wndClassName
	};

	RegisterClass(&wndClass);

	// Set the window's initial style
	u32 dwWindowStyle = WS_OVERLAPPED | WS_CHILD | WS_VISIBLE;

	// Create the render window
	m_hConsoleWnd = CreateWindow(wndClassName, "XRAY Text Console", dwWindowStyle, lX, lY, lWidth, lHeight, m_hMainWnd, 0, hInstance, 0L);
	R_ASSERT2(m_hConsoleWnd, "Unable to Create TextConsole Window!");
}

void CDedicatedSrvConsole::CreateLogWnd()
{
	HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);

	RECT rct;
	GetClientRect(m_hConsoleWnd, &rct);

	INT lX = rct.left;
	INT lY = rct.top;
	INT lWidth = rct.right - rct.left;
	INT lHeight = rct.bottom - rct.top;

	const char *wndClassName = "TEXT_CONSOLE_LOG_WND";

	// Register the windows class
	WNDCLASS wndClass =
	{
			0, TextConsole_LogWndProc, 0, 0, hInstance, NULL, LoadCursor(NULL, IDC_ARROW), GetStockBrush(BLACK_BRUSH), NULL, wndClassName
	};

	RegisterClass(&wndClass);

	// Set the window's initial style
	u32 dwWindowStyle = WS_OVERLAPPED | WS_CHILD | WS_VISIBLE; // | WS_CLIPSIBLINGS;

	// Create the render window
	m_hLogWnd = CreateWindow(wndClassName,
						   "XRAY Text Console Log", dwWindowStyle, lX, lY, lWidth, lHeight, m_hConsoleWnd, 0, hInstance, 0L);

	R_ASSERT2(m_hLogWnd, "Unable to Create TextConsole Window!");

	ShowWindow(m_hLogWnd, SW_SHOW);
	UpdateWindow(m_hLogWnd);

	LOGFONT lf;
	lf.lfHeight = -12;
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = FW_NORMAL;
	lf.lfItalic = 0;
	lf.lfUnderline = 0;
	lf.lfStrikeOut = 0;
	lf.lfCharSet = RUSSIAN_CHARSET;
	lf.lfOutPrecision = OUT_STRING_PRECIS;
	lf.lfClipPrecision = CLIP_STROKE_PRECIS;
	lf.lfQuality = DRAFT_QUALITY;
	lf.lfFaceName[0] = '\0';

	if (strstr(Core.Params, "-cursive_font"))
		lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SCRIPT;
	else
		lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	m_hLogWndFont = CreateFontIndirect(&lf);
	R_ASSERT2(m_hLogWndFont, "Unable to Create Font for Log Window");

	m_hdcLogWnd = GetDC(m_hLogWnd); // DC - Device Context
	R_ASSERT2(m_hdcLogWnd, "Unable to Get DC for Log Window!");

	m_hdcBackBuffer = CreateCompatibleDC(m_hdcLogWnd);
	R_ASSERT2(m_hdcBackBuffer, "Unable to Create Compatible DC for Log Window!");

	m_hBitmap = CreateCompatibleBitmap(m_hdcLogWnd, lWidth, lHeight);
	R_ASSERT2(m_hBitmap, "Unable to Create Compatible Bitmap for Log Window!");

	m_hOldBitmap = (HBITMAP)SelectObject(m_hdcBackBuffer, m_hBitmap);
	m_hPrevFont = (HFONT)SelectObject(m_hdcBackBuffer, m_hLogWndFont);

	SetBkColor(m_hdcBackBuffer, RGB(1, 1, 1));
	m_hBackGroundBrush = GetStockBrush(BLACK_BRUSH);
}

void CDedicatedSrvConsole::Initialize()
{
	inherited::Initialize();

	m_LastStatisticUpdateTime = Device.dwTimeGlobal;
	m_hMainWnd = Device.m_hWnd;

	CreateConsoleWnd();
	CreateLogWnd();

	ShowWindow(m_hConsoleWnd, SW_SHOW);
	UpdateWindow(m_hConsoleWnd);

	m_server_info.ResetData();
}

void CDedicatedSrvConsole::Destroy()
{
	inherited::Destroy();

	SelectObject(m_hdcBackBuffer, m_hPrevFont);
	SelectObject(m_hdcBackBuffer, m_hOldBitmap);

	if (m_hBitmap)
		DeleteObject(m_hBitmap);
	if (m_hOldBitmap)
		DeleteObject(m_hOldBitmap);
	if (m_hLogWndFont)
		DeleteObject(m_hLogWndFont);
	if (m_hPrevFont)
		DeleteObject(m_hPrevFont);
	if (m_hBackGroundBrush)
		DeleteObject(m_hBackGroundBrush);

	ReleaseDC(m_hLogWnd, m_hdcBackBuffer);
	ReleaseDC(m_hLogWnd, m_hdcLogWnd);

	DestroyWindow(m_hLogWnd);
	DestroyWindow(m_hConsoleWnd);
}

void CDedicatedSrvConsole::OnPaint()
{
	RECT rect;
	PAINTSTRUCT pstruct;

	BeginPaint(m_hLogWnd, &pstruct);

	if (Device.CurrentFrameNumber % 2)
	{
		GetClientRect(m_hLogWnd, &rect);
		DrawLog(m_hdcBackBuffer, &rect);
	}
	else
		rect = pstruct.rcPaint;

	// BitBlt выполняет передачу битовых блоков данных о цвете, соответствующих прямоугольнику
	// пикселей из заданного исходного контекста устройства в целевой контекст устройства

	BitBlt(m_hdcLogWnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, m_hdcBackBuffer, rect.left, rect.top,
		   SRCCOPY);

	EndPaint(m_hLogWnd, &pstruct);
}

void CDedicatedSrvConsole::DrawLog(HDC hDC, RECT *pRect)
{
	TEXTMETRIC tm;
	GetTextMetrics(hDC, &tm);

	RECT wRC = *pRect;
	GetClientRect(m_hLogWnd, &wRC);

	FillRect(hDC, &wRC, m_hBackGroundBrush);

	int Width = wRC.right - wRC.left;
	int Height = wRC.bottom - wRC.top;

	wRC = *pRect;
	int y_top_max = (int)(0.32f * Height);

	LPCSTR s_edt = ec().str_edit();
	LPCSTR s_cur = ec().str_before_cursor();

	u32 cur_len = xr_strlen(s_cur) + xr_strlen(ch_cursor) + 1;
	PSTR buf = (PSTR)_alloca(cur_len * sizeof(char));
	strcpy_s(buf, cur_len, s_cur);
	strcat_s(buf, cur_len, ch_cursor);
	buf[cur_len - 1] = 0;

	u32 cur0_len = xr_strlen(s_cur);
	int xb = 25;

	SetTextColor(hDC, RGB(255, 255, 255));
	TextOut(hDC, xb, Height - tm.tmHeight - 1, buf, cur_len - 1);
	buf[cur0_len] = 0;
	
	SetTextColor(hDC, RGB(0, 0, 0));
	TextOut(hDC, xb, Height - tm.tmHeight - 1, buf, cur0_len);

	SetTextColor(hDC, RGB(255, 255, 255));
	TextOut(hDC, 0, Height - tm.tmHeight - 3, ioc_prompt, xr_strlen(ioc_prompt)); // ">>> "

	SetTextColor(hDC, (COLORREF)bgr2rgb(get_mark_color(Console_mark::mark11)));
	TextOut(hDC, xb, Height - tm.tmHeight - 3, s_edt, xr_strlen(s_edt));

	SetTextColor(hDC, RGB(205, 205, 225));
	u32 log_line = LogFile->size() - 1;
	string16 q, q2;
	itoa(log_line, q, 10);
	strcpy_s(q2, sizeof(q2), "[");
	strcat_s(q2, sizeof(q2), q);
	strcat_s(q2, sizeof(q2), "]");
	u32 qn = xr_strlen(q2);

	TextOut(hDC, Width - 8 * qn, Height - tm.tmHeight - tm.tmHeight, q2, qn);
	int ypos = Height - tm.tmHeight - tm.tmHeight;

	for (int i = LogFile->size() - 1 - scroll_delta; i >= 0; --i)
	{
		ypos -= tm.tmHeight;

		if (ypos < y_top_max)
			break;

		LPCSTR Str = *(*LogFile)[i];

		if (!Str)
			continue;

		Console_mark cm = (Console_mark)Str[0];
		COLORREF c2 = (COLORREF)bgr2rgb(get_mark_color(cm));
		SetTextColor(hDC, c2);

		u8 b = (is_mark(cm)) ? 2 : 0;
		LPCSTR pOut = Str + b;

		BOOL res = TextOut(hDC, 10, ypos, pOut, xr_strlen(pOut));
		R_ASSERT(res);
	}

	if (g_pGameLevel && (Device.dwTimeGlobal - m_LastStatisticUpdateTime > 500))
	{
		m_LastStatisticUpdateTime = Device.dwTimeGlobal;
		m_server_info.ResetData();
		g_pGameLevel->GetLevelInfo(&m_server_info);
	}

	ypos = 5;

	for (u32 i = 0; i < m_server_info.Size(); ++i)
	{
		SetTextColor(hDC, m_server_info[i].color);
		TextOut(hDC, 10, ypos, m_server_info[i].name, xr_strlen(m_server_info[i].name));

		ypos += tm.tmHeight;

		if (ypos > y_top_max)		
			break;		
	}
}

void CDedicatedSrvConsole::OnFrame()
{
	inherited::OnFrame();

	InvalidateRect(m_hConsoleWnd, NULL, FALSE);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
}
