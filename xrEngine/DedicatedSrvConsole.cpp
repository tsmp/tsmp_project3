#include "stdafx.h"
#include "DedicatedSrvConsole.h"

extern const char* ioc_prompt;
int g_svTextConsoleUpdateRate = 1;

LRESULT CALLBACK TextConsole_LogWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND:
		return (LRESULT)1; // Say we handled it.

	case WM_PAINT:
	{
		CDedicatedSrvConsole* pConsole = (CDedicatedSrvConsole*)Console;
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
	m_hLogWndFont = nullptr;

	m_bScrollLog = false;
	m_dwStartLine = 0;

	m_bNeedUpdate = false;
	m_dwLastUpdateTime = 0;
	m_last_time = Device.dwTimeGlobal;
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

	const char* wndClassName = "TEXT_CONSOLE";

	// Register the windows class
	WNDCLASS wndClass = 
	{ 
		0
		, DefWindowProc
		, 0
		, 0
		, hInstance
		, NULL
		, LoadCursor(hInstance, IDC_ARROW)
		, GetStockBrush(GRAY_BRUSH)
		, NULL
		, wndClassName };

	RegisterClass(&wndClass);

	// Set the window's initial style
	u32 dwWindowStyle = WS_OVERLAPPED | WS_CHILD | WS_VISIBLE;

	// Create the render window
	hConsoleWnd = CreateWindow(wndClassName
		, "XRAY Text Console"
		, dwWindowStyle
		, lX
		, lY
		, lWidth
		, lHeight
		, hMainWnd
		, 0
		, hInstance
		, 0L);

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

	const char* wndClassName = "TEXT_CONSOLE_LOG_WND";

	// Register the windows class
	WNDCLASS wndClass = 
	{ 
		0
		, TextConsole_LogWndProc
		, 0
		, 0
		, hInstance
		, NULL
		, LoadCursor(NULL, IDC_ARROW)
		, GetStockBrush(BLACK_BRUSH)
		, NULL
		, wndClassName
	};

	RegisterClass(&wndClass);

	// Set the window's initial style
	u32 dwWindowStyle = WS_OVERLAPPED | WS_CHILD | WS_VISIBLE;// | WS_CLIPSIBLINGS;

	// Create the render window
	hLogWnd = CreateWindow(wndClassName,
		"XRAY Text Console Log"
		, dwWindowStyle
		, lX
		, lY
		, lWidth
		, lHeight
		, hConsoleWnd
		, 0
		, hInstance
		, 0L);

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

	m_hLogWndFont = CreateFontIndirect(&lf);
	R_ASSERT2(m_hLogWndFont, "Unable to Create Font for Log Window");

	m_hDC_LogWnd = GetDC(hLogWnd); // DC - Device Context
	R_ASSERT2(m_hDC_LogWnd, "Unable to Get DC for Log Window!");

	m_hDC_LogWnd_BackBuffer = CreateCompatibleDC(m_hDC_LogWnd);
	R_ASSERT2(m_hDC_LogWnd_BackBuffer, "Unable to Create Compatible DC for Log Window!");

	m_hBB_BM = CreateCompatibleBitmap(m_hDC_LogWnd, lWidth, lHeight);
	R_ASSERT2(m_hBB_BM, "Unable to Create Compatible Bitmap for Log Window!");

	m_hOld_BM = (HBITMAP)SelectObject(m_hDC_LogWnd_BackBuffer, m_hBB_BM);
	m_hPrevFont = (HFONT)SelectObject(m_hDC_LogWnd_BackBuffer, m_hLogWndFont);

	SetBkColor(m_hDC_LogWnd_BackBuffer, RGB(0, 0, 0));

	m_hBackGroundBrush = GetStockBrush(BLACK_BRUSH);
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
	m_last_time = Device.dwTimeGlobal;
}

void CDedicatedSrvConsole::Destroy()
{
	CConsole::Destroy();

	SelectObject(m_hDC_LogWnd_BackBuffer, m_hPrevFont);
	SelectObject(m_hDC_LogWnd_BackBuffer, m_hOld_BM);

	if (m_hBB_BM) DeleteObject(m_hBB_BM);
	if (m_hOld_BM) DeleteObject(m_hOld_BM);
	if (m_hLogWndFont) DeleteObject(m_hLogWndFont);
	if (m_hPrevFont) DeleteObject(m_hPrevFont);
	if (m_hBackGroundBrush) DeleteObject(m_hBackGroundBrush);

	ReleaseDC(hLogWnd, m_hDC_LogWnd_BackBuffer);
	ReleaseDC(hLogWnd, m_hDC_LogWnd);

	DestroyWindow(hLogWnd);
	DestroyWindow(hConsoleWnd);
}

void CDedicatedSrvConsole::OnPaint()
{
	RECT rect;
	PAINTSTRUCT pstruct;

	BeginPaint(hLogWnd, &pstruct);

	if (m_bNeedUpdate)
	{
		m_dwLastUpdateTime = Device.dwTimeGlobal;
		m_bNeedUpdate = false;

		GetClientRect(hLogWnd, &rect);
		DrawLog(m_hDC_LogWnd_BackBuffer, &rect);
	}
	else
		rect = pstruct.rcPaint;

	// BitBlt выполняет передачу битовых блоков данных о цвете, соответствующих прямоугольнику 
	// пикселей из заданного исходного контекста устройства в целевой контекст устройства
	
	BitBlt(m_hDC_LogWnd
		, rect.left
		, rect.top
		, rect.right - rect.left
		, rect.bottom - rect.top
		, m_hDC_LogWnd_BackBuffer
		, rect.left
		, rect.top,
		SRCCOPY);

	EndPaint(hLogWnd, &pstruct);
}

void CDedicatedSrvConsole::DrawLog(HDC hDC, RECT* pRect)
{
	TEXTMETRIC tm;
	GetTextMetrics(hDC, &tm);

	RECT wRC = *pRect;
	GetClientRect(hLogWnd, &wRC);

	FillRect(hDC, &wRC, m_hBackGroundBrush);

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

	for (int i = LogFile->size() - 1 - scroll_delta; i >= 0; i--)
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

	// update statistic every 0,8 sec
	if (g_pGameLevel && (Device.dwTimeGlobal - m_last_time > 800))
	{
		m_last_time = Device.dwTimeGlobal;

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
	m_bScrollLog = true;
	CConsole::IR_OnKeyboardPress(dik);
}

void CDedicatedSrvConsole::IR_OnKeyboardHold(int dik)
{
	m_bScrollLog = true;
	CConsole::IR_OnKeyboardHold(dik);
}

void CDedicatedSrvConsole::IR_OnKeyboardRelease(int dik)
{
	m_bScrollLog = false;
	CConsole::IR_OnKeyboardRelease(dik);
}

void CDedicatedSrvConsole::OnFrame()
{
	inherited::OnFrame();

	if (m_bScrollLog)
		m_bNeedUpdate = true;

	if (!m_bNeedUpdate && (m_dwLastUpdateTime + 1000 / g_svTextConsoleUpdateRate) > Device.dwTimeGlobal)
		return;

	InvalidateRect(hConsoleWnd, NULL, FALSE);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	m_bNeedUpdate = true;
}
