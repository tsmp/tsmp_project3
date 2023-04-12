// File: x_ray.cpp
//
// Programmers:
//	Oles		- Oles Shishkovtsov
//	AlexMX		- Alexander Maksimchuk

#include "pch_xrengine.h"
#include <process.h>
#include "igame_persistent.h"
#include "xr_input.h"
#include "std_classes.h"
#include "ispatial.h"
#include "resource.h"
#include "LightAnimLibrary.h"
#include "DedicatedSrvConsole.h"
#include "..\TSMP3_Build_Config.h"
#include "Console_commands.h"
#include "xrApplication.h"

// global variables
ENGINE_API CApplication* pApp = nullptr;

ENGINE_API string512 g_sLaunchOnExit_params;
ENGINE_API string512 g_sLaunchOnExit_app;

ENGINE_API bool g_dedicated_server = false;
ENGINE_API CInifile *pGameIni = nullptr;

// computing build id
XRCORE_API LPCSTR build_date;
XRCORE_API u32 build_id;

static LPSTR month_id[12] = 
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static int days_in_month[12] = 
{
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int start_day = 31;	  // 31
static int start_month = 1;	  // January
static int start_year = 1999; // 1999

void compute_build_id()
{
	build_date = __DATE__;

	int days;
	int months = 0;
	int years;
	string16 month;
	string256 buffer {0};
	strcpy_s(buffer, __DATE__);
	sscanf(buffer, "%s %d %d", month, &days, &years);

	for (int i = 0; i < 12; i++)
	{
		if (_stricmp(month_id[i], month))
			continue;

		months = i;
		break;
	}

	build_id = (years - start_year) * 365 + days - start_day;

	for (int i = 0; i < months; ++i)
		build_id += days_in_month[i];

	for (int i = 0; i < start_month - 1; ++i)
		build_id -= days_in_month[i];
}

// Класс для проверки наличия нескольких запущенных копий игры
class GameInstancesChecker
{
	HANDLE m_PresenceMutex;
	const char* m_MutexName = "STALKER-SoC";

public:

#ifdef NO_MULTI_INSTANCES // Запрещено запускать несколько экземпляров игры

	bool IsAnotherGameInstanceLaunched()
	{
		HANDLE presenceMutex = OpenMutex(READ_CONTROL, FALSE, m_MutexName);

		// Мьютекс с таким именем уже существует в системе
		if (presenceMutex != NULL)
		{
			CloseHandle(presenceMutex);
			return true;
		}

		return false;
	}

	void RegisterThisInstanceLaunched()
	{
		m_PresenceMutex = CreateMutex(NULL, FALSE, m_MutexName);
	}

	void UnregisterThisInstanceLaunched()
	{
		CloseHandle(m_PresenceMutex);
	}

#else // Можно запускать несколько экземпляров

	bool IsAnotherGameInstanceLaunched() { return false; }
	void RegisterThisInstanceLaunched() {}
	void UnregisterThisInstanceLaunched() {}

#endif // NO_MULTI_INSTANCES
};

extern void TryLoadXrCustomResDll();
extern void TryToChangeLogoImageToCustom(HWND logoWindow);

namespace Logo
{
	static HWND logoWindow = nullptr;

	void Create()
	{
#ifdef DEBUG
		bool isTopMost = false;
#else
		bool isTopMost = !IsDebuggerPresent();
#endif

		HWND topMost = isTopMost ? HWND_TOPMOST : HWND_NOTOPMOST;
		logoWindow = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_STARTUP), 0, 0);

		UINT flags = SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW;
		SetWindowPos(logoWindow, topMost, 0, 0, 0, 0, flags);				
		TryToChangeLogoImageToCustom(logoWindow);
	}

	void Destroy()
	{
		DestroyWindow(logoWindow);
		logoWindow = nullptr;
	}
}

void InitializeSettings()
{
	string_path fname;
	FS.update_path(fname, "$game_config$", "system.ltx");
	pSettings = xr_new<CInifile>(fname, TRUE);
	CHECK_OR_EXIT(!pSettings->sections().empty(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));

	FS.update_path(fname, "$game_config$", "game.ltx");
	pGameIni = xr_new<CInifile>(fname, TRUE);
	CHECK_OR_EXIT(!pGameIni->sections().empty(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
}

void LaunchOnExit()
{
	char* _args[3];
	// check for need to execute something external
	if (xr_strlen(g_sLaunchOnExit_app))
	{
		string4096 ModuleFileName = "";
		GetModuleFileName(NULL, ModuleFileName, 4096);

		string4096 ModuleFilePath = "";
		char* ModuleName = NULL;
		GetFullPathName(ModuleFileName, 4096, ModuleFilePath, &ModuleName);
		ModuleName[0] = 0;
		strcat(ModuleFilePath, g_sLaunchOnExit_app);
		_args[0] = g_sLaunchOnExit_app;
		_args[1] = g_sLaunchOnExit_params;
		_args[2] = NULL;

		_spawnv(_P_NOWAIT, _args[0], _args); //, _envvar);
	}
}

void InitializeConsole()
{
#ifdef DEDICATED_SERVER	
		Console = xr_new<CDedicatedSrvConsole>();	
#else
		Console = xr_new<CConsole>();	
#endif

	Console->Initialize();
	strcpy_s(Console->ConfigFile, "user.ltx");

	if (strstr(Core.Params, "-ltx "))
	{
		string64 c_name;
		sscanf(strstr(Core.Params, "-ltx ") + 5, "%[^ ] ", c_name);
		strcpy_s(Console->ConfigFile, c_name);
	}
}

void InitInput()
{
	BOOL bCaptureInput = !strstr(Core.Params, "-i");

	if (g_dedicated_server)
		bCaptureInput = FALSE;

	pInput = xr_new<CInput>(bCaptureInput);
}

void destroyInput()
{
	xr_delete(pInput);
}

void InitSound()
{
	CSound_manager_interface::_create(u64(Device.m_hWnd));
}

void destroySound()
{
	CSound_manager_interface::_destroy();
}

void destroySettings()
{
	xr_delete(pSettings);
	xr_delete(pGameIni);
}

void destroyConsole()
{
	Console->Execute("cfg_save");
	Console->Destroy();
	xr_delete(Console);
}

void destroyEngine()
{
	Device.Destroy();
	Engine.Destroy();
}

void execUserScript()
{
	// Execute script

	Console->Execute("unbindall");
	Console->ExecuteScript(Console->ConfigFile);
}

void InitializeCore(char* lpCmdLine)
{
	LPCSTR fsgame_ltx_name = "-fsltx ";
	string_path fsgame = "";

	if (strstr(lpCmdLine, fsgame_ltx_name))
	{
		int sz = xr_strlen(fsgame_ltx_name);
		sscanf(strstr(lpCmdLine, fsgame_ltx_name) + sz, "%[^ ] ", fsgame);
	}

	compute_build_id();
	Core._initialize("xray", NULL, TRUE, fsgame[0] ? fsgame : NULL);
}

// Фунция для тупых требований THQ и тупых американских пользователей
BOOL IsOutOfVirtualMemory()
{
#define VIRT_ERROR_SIZE 256
#define VIRT_MESSAGE_SIZE 512

	MEMORYSTATUSEX statex;
	DWORD dwPageFileInMB = 0;
	DWORD dwPhysMemInMB = 0;
	HINSTANCE hApp = 0;
	char pszError[VIRT_ERROR_SIZE];
	char pszMessage[VIRT_MESSAGE_SIZE];

	ZeroMemory(&statex, sizeof(MEMORYSTATUSEX));
	statex.dwLength = sizeof(MEMORYSTATUSEX);

	if (!GlobalMemoryStatusEx(&statex))
		return 0;

	dwPageFileInMB = (DWORD)(statex.ullTotalPageFile / (1024 * 1024));
	dwPhysMemInMB = (DWORD)(statex.ullTotalPhys / (1024 * 1024));

	// Довольно отфонарное условие
	if ((dwPhysMemInMB > 500) && ((dwPageFileInMB + dwPhysMemInMB) > 2500))
		return 0;

	hApp = GetModuleHandle(NULL);

	if (!LoadString(hApp, RC_VIRT_MEM_ERROR, pszError, VIRT_ERROR_SIZE))
		return 0;

	if (!LoadString(hApp, RC_VIRT_MEM_TEXT, pszMessage, VIRT_MESSAGE_SIZE))
		return 0;

	MessageBox(NULL, pszMessage, pszError, MB_OK | MB_ICONHAND);
	return 1;
}

void InitializeApplication()
{
	ShowWindow(Device.m_hWnd, SW_SHOWNORMAL);
	Device.Create();
	LALib.OnCreate();

	pApp = xr_new<CApplication>();
	g_pGamePersistent = (IGame_Persistent*)NEW_INSTANCE(CLSID_GAME_PERSISTANT);
	g_SpatialSpace = xr_new<ISpatial_DB>();
	g_SpatialSpacePhysic = xr_new<ISpatial_DB>();
}

void EngineInitialize(char* lpCmdLine)
{
	Logo::Create();	
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	
	g_sLaunchOnExit_app[0] = g_sLaunchOnExit_params[0] = '\0';

	InitializeCore(lpCmdLine);
	InitializeSettings();

	FPU::m24r();	
	Engine.Initialize();
	Device.Initialize();

	InitInput();
	InitializeConsole();

	if (strstr(Core.Params, "-r2a"))
		Console->Execute("renderer renderer_r2a");
	else if (strstr(Core.Params, "-r2"))
		Console->Execute("renderer renderer_r2");
	else
	{
		CCC_LoadCFG_custom* pTmp = xr_new<CCC_LoadCFG_custom>("renderer ");
		pTmp->Execute(Console->ConfigFile);
		xr_delete(pTmp);
	}
	
	Engine.External.Initialize();
	Console->Execute("stat_memory");

	execUserScript();
	InitSound();

	// ...command line for auto start
	if(LPCSTR pStartup = strstr(Core.Params, "-start "))
		Console->Execute(pStartup + 1);
	
	if(LPCSTR pStartup = strstr(Core.Params, "-load "))
		Console->Execute(pStartup + 1);

	InitializeApplication();
	Logo::Destroy();
}

void EngineRoutine()
{
	// Main cycle
	Memory.mem_usage();
	Device.Run();
}

void EngineDestroy()
{
	// Destroy APP
	xr_delete(g_SpatialSpacePhysic);
	xr_delete(g_SpatialSpace);
	DEL_INSTANCE(g_pGamePersistent);
	xr_delete(pApp);
	Engine.Event.Dump();

	// Destroying
	destroySound();
	destroyInput();
	destroySettings();

	LALib.OnDestroy();

	destroyConsole();
	destroyEngine();

	Core._destroy();
	LaunchOnExit();
}

int APIENTRY WinMainImplementation(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow)
{
	TryLoadXrCustomResDll();

#ifdef DEDICATED_SERVER
	g_dedicated_server = true;
#else

	if ((strstr(lpCmdLine, "-skipmemcheck") == NULL) && IsOutOfVirtualMemory())
		return 0;

	GameInstancesChecker gameInstances;

	if (gameInstances.IsAnotherGameInstanceLaunched())
		return 0;

	gameInstances.RegisterThisInstanceLaunched();

#endif // DEDICATED_SERVER
		
	EngineInitialize(lpCmdLine);
	EngineRoutine();
	EngineDestroy();

#ifndef DEDICATED_SERVER
	gameInstances.UnregisterThisInstanceLaunched();
#endif // DEDICATED_SERVER

	return 0;
}

int stack_overflow_exception_filter(int exception_code)
{
	if (exception_code == EXCEPTION_STACK_OVERFLOW)
	{
		// Do not call _resetstkoflw here, because
		// at this point, the stack is not yet unwound.
		// Instead, signal that the handler (the __except block)
		// is to be executed.
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
		return EXCEPTION_CONTINUE_SEARCH;
}

int APIENTRY WinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 char *lpCmdLine,
					 int nCmdShow)
{
#ifdef DEDICATED_SERVER
	Debug._initialize(true);
#else  
	Debug._initialize(false);
#endif

	__try
	{
		WinMainImplementation(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}
	__except (stack_overflow_exception_filter(GetExceptionCode()))
	{
		_resetstkoflw();
		FATAL("stack overflow");
	}

	return 0;
}
