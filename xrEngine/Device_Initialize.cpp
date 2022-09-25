#include "pch_xrengine.h"
#include "resource.h"

extern LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern const char* TryToGetNewWindowText();

void CRenderDevice::Initialize()
{
    Log("Initializing Engine...");
    m_GlobalTimer.Start();
    TimerMM.Start();

    // Unless a substitute hWnd has been specified, create a window to render into
    if (!m_hWnd)
    {
        const char *wndClassName = "_XRAY_";

        HINSTANCE hInstance = static_cast<HINSTANCE>(GetModuleHandle(0));
        HICON icon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
        HCURSOR cursor = LoadCursor(NULL, IDC_ARROW);
        HBRUSH brush = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
             
		WNDCLASS wndClass = { 0, WndProc, 0, 0, hInstance, icon, cursor, brush, nullptr, wndClassName };
		RegisterClass(&wndClass);

        u32 wndStyle = WS_BORDER | WS_DLGFRAME;

        // Set the window's initial width
        RECT rc{ 0,0,640,480 };
        AdjustWindowRect(&rc, wndStyle, FALSE);

        u32 width = rc.right - rc.left;
        u32 height = rc.bottom - rc.top;

        const char* wndText;
        
        if (const char* ptr = TryToGetNewWindowText())
            wndText = ptr;
        else
            wndText = "S.T.A.L.K.E.R.: Shadow Of Chernobyl";

        // Create the render window
        m_hWnd = CreateWindow(wndClassName, wndText, wndStyle, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, hInstance, 0);
    }

    // Command line
    char *lpCmdLine = Core.Params;
    if (strstr(lpCmdLine, "-gpu_sw") != NULL)
        HW.Caps.bForceGPU_SW = TRUE;
    else
        HW.Caps.bForceGPU_SW = FALSE;
    if (strstr(lpCmdLine, "-gpu_nopure") != NULL)
        HW.Caps.bForceGPU_NonPure = TRUE;
    else
        HW.Caps.bForceGPU_NonPure = FALSE;
    if (strstr(lpCmdLine, "-gpu_ref") != NULL)
        HW.Caps.bForceGPU_REF = TRUE;
    else
        HW.Caps.bForceGPU_REF = FALSE;
}
