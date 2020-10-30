#include "stdafx.h"
#include "DedicatedSrvConsole.h"

extern const char *ioc_prompt;
int g_svTextConsoleUpdateRate = 1;

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
	hMainWnd = nullptr;
	hConsoleWnd = nullptr;
	hLogWnd = nullptr;
	hLogWndFont = nullptr;

	bScrollLog = false;
	bNeedUpdate = false;

	LastUpdateTime = 0;
	LastStatisticsUpdate = 0;
}

CDedicatedSrvConsole::~CDedicatedSrvConsole()
{
	hMainWnd = nullptr;
}

void CDedicatedSrvConsole::CreateConsoleWnd()
{
	HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);

	RECT rct;
	GetClientRect(hMainWnd, &rct);

	INT lX = rct.left;
	INT lY = rct.top;
	INT lWidth = rct.right - rct.left;
	INT lHeight = rct.bottom - rct.top;

	const char *wndClassName = "TEXT_CONSOLE";

	// Register the windows class
	WNDCLASS wndClass =
		{
			0, DefWindowProc, 0, 0, hInstance, NULL, LoadCursor(hInstance, IDC_ARROW), GetStockBrush(GRAY_BRUSH), NULL, wndClassName};

	RegisterClass(&wndClass);

	// Set the window's initial style
	u32 dwWindowStyle = WS_OVERLAPPED | WS_CHILD | WS_VISIBLE;

	// Create the render window
	hConsoleWnd = CreateWindow(wndClassName, "XRAY Text Console", dwWindowStyle, lX, lY, lWidth, lHeight, hMainWnd, 0, hInstance, 0L);

	R_ASSERT2(hConsoleWnd, "Unable to Create TextConsole Window!");
}

void CDedicatedSrvConsole::CreateLogWnd()
{
	HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);

	RECT rct;
	GetClientRect(hConsoleWnd, &rct);

	INT lX = rct.left;
	INT lY = rct.top;
	INT lWidth = rct.right - rct.left;
	INT lHeight = rct.bottom - rct.top;

	const char *wndClassName = "TEXT_CONSOLE_LOG_WND";

	// Register the windows class
	WNDCLASS wndClass =
		{
			0, TextConsole_LogWndProc, 0, 0, hInstance, NULL, LoadCursor(NULL, IDC_ARROW), GetStockBrush(BLACK_BRUSH), NULL, wndClassName};

	RegisterClass(&wndClass);

	// Set the window's initial style
	u32 dwWindowStyle = WS_OVERLAPPED | WS_CHILD | WS_VISIBLE; // | WS_CLIPSIBLINGS;

	// Create the render window
	hLogWnd = CreateWindow(wndClassName,
						   "XRAY Text Console Log", dwWindowStyle, lX, lY, lWidth, lHeight, hConsoleWnd, 0, hInstance, 0L);

	R_ASSERT2(hLogWnd, "Unable to Create TextConsole Window!");

	ShowWindow(hLogWnd, SW_SHOW);
	UpdateWindow(hLogWnd);

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

	hLogWndFont = CreateFontIndirect(&lf);
	R_ASSERT2(hLogWndFont, "Unable to Create Font for Log Window");

	hdcLogWnd = GetDC(hLogWnd); // DC - Device Context
	R_ASSERT2(hdcLogWnd, "Unable to Get DC for Log Window!");

	hdcBackBuffer = CreateCompatibleDC(hdcLogWnd);
	R_ASSERT2(hdcBackBuffer, "Unable to Create Compatible DC for Log Window!");

	hBitmap = CreateCompatibleBitmap(hdcLogWnd, lWidth, lHeight);
	R_ASSERT2(hBitmap, "Unable to Create Compatible Bitmap for Log Window!");

	hOldBitmap = (HBITMAP)SelectObject(hdcBackBuffer, hBitmap);
	hPrevFont = (HFONT)SelectObject(hdcBackBuffer, hLogWndFont);

	SetBkColor(hdcBackBuffer, RGB(0, 0, 0));

	hBackGroundBrush = GetStockBrush(BLACK_BRUSH);
}

void CDedicatedSrvConsole::Initialize()
{
	CConsole::Initialize();

	hMainWnd = Device.m_hWnd;

	CreateConsoleWnd();
	CreateLogWnd();

	ShowWindow(hConsoleWnd, SW_SHOW);
	UpdateWindow(hConsoleWnd);

	server_info.ResetData();
	LastStatisticsUpdate = Device.dwTimeGlobal;
}

void CDedicatedSrvConsole::Destroy()
{
	CConsole::Destroy();

	SelectObject(hdcBackBuffer, hPrevFont);
	SelectObject(hdcBackBuffer, hOldBitmap);

	if (hBitmap)
		DeleteObject(hBitmap);
	if (hOldBitmap)
		DeleteObject(hOldBitmap);
	if (hLogWndFont)
		DeleteObject(hLogWndFont);
	if (hPrevFont)
		DeleteObject(hPrevFont);
	if (hBackGroundBrush)
		DeleteObject(hBackGroundBrush);

	ReleaseDC(hLogWnd, hdcBackBuffer);
	ReleaseDC(hLogWnd, hdcLogWnd);

	DestroyWindow(hLogWnd);
	DestroyWindow(hConsoleWnd);
}

void CDedicatedSrvConsole::OnPaint()
{
	RECT rect;
	PAINTSTRUCT pstruct;

	BeginPaint(hLogWnd, &pstruct);

	if (bNeedUpdate)
	{
		LastUpdateTime = Device.dwTimeGlobal;
		bNeedUpdate = false;

		GetClientRect(hLogWnd, &rect);
		DrawLog(hdcBackBuffer, &rect);
	}
	else
		rect = pstruct.rcPaint;

	// BitBlt выполняет передачу битовых блоков данных о цвете, соответствующих прямоугольнику
	// пикселей из заданного исходного контекста устройства в целевой контекст устройства

	BitBlt(hdcLogWnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hdcBackBuffer, rect.left, rect.top,
		   SRCCOPY);

	EndPaint(hLogWnd, &pstruct);
}

void CDedicatedSrvConsole::DrawLog(HDC hDC, RECT *pRect)
{
	TEXTMETRIC tm;
	GetTextMetrics(hDC, &tm);

	RECT wRC = *pRect;
	GetClientRect(hLogWnd, &wRC);

	FillRect(hDC, &wRC, hBackGroundBrush);

	INT Height = wRC.bottom - wRC.top;
	wRC = *pRect;
	int y_top_max = (int)(0.3f * Height);

	char buf[MAX_LEN + 5];
	strcpy_s(buf, ioc_prompt);
	strcat(buf, editor);
	//if (bCursor)
	strcat(buf, "|");

	SetTextColor(hDC, RGB(128, 128, 255));
	TextOut(hDC, 0, Height - tm.tmHeight, buf, xr_strlen(buf));

	int YPos = Height - tm.tmHeight - tm.tmHeight;

	static int last_log_size;

	if (scroll_delta)
		scroll_delta += (LogFile->size() - last_log_size);

	int startPos = LogFile->size() - 1 - scroll_delta;

	for (int i = startPos; i >= 0; i--)
	{
		YPos -= tm.tmHeight;

		if (YPos < y_top_max)
			break;

		LPCSTR Str = *(*LogFile)[i];

		if (!Str)
			continue;

		Console_mark cm = (Console_mark)Str[0];
		COLORREF c2 = (COLORREF)bgr2rgb(get_mark_color(cm));
		SetTextColor(hDC, c2);

		u8 b = (is_mark(cm)) ? 2 : 0;
		LPCSTR pOut = Str + b;

		BOOL res = TextOut(hDC, 10, YPos, pOut, xr_strlen(pOut));
		R_ASSERT(res);
	}

	last_log_size = LogFile->size();

	// update statistic every 0,8 sec
	if (g_pGameLevel && (Device.dwTimeGlobal - LastStatisticsUpdate > 800))
	{
		LastStatisticsUpdate = Device.dwTimeGlobal;

		server_info.ResetData();
		g_pGameLevel->GetLevelInfo(&server_info);
	}

	YPos = 5;

	for (u32 i = 0; i < server_info.Size(); ++i)
	{
		SetTextColor(hDC, server_info[i].color);
		TextOut(hDC, 10, YPos, server_info[i].name, xr_strlen(server_info[i].name));
		YPos += tm.tmHeight;

		if (YPos > y_top_max)
			break;
	}
}

void CDedicatedSrvConsole::IR_OnKeyboardPress(int dik)
{
	bScrollLog = true;
	CConsole::IR_OnKeyboardPress(dik);
}

void CDedicatedSrvConsole::IR_OnKeyboardHold(int dik)
{
	bScrollLog = true;
	CConsole::IR_OnKeyboardHold(dik);
}

void CDedicatedSrvConsole::IR_OnKeyboardRelease(int dik)
{
	bScrollLog = false;
	CConsole::IR_OnKeyboardRelease(dik);
}

void CDedicatedSrvConsole::OnFrame()
{
	inherited::OnFrame();

	if (bScrollLog)
		bNeedUpdate = true;

	if (!bNeedUpdate && (LastUpdateTime + 1000 / g_svTextConsoleUpdateRate) > Device.dwTimeGlobal)
		return;

	InvalidateRect(hConsoleWnd, NULL, FALSE);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	bNeedUpdate = true;
}
