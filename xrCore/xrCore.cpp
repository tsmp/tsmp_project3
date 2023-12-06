#include "stdafx.h"
#include <mmsystem.h>
#include <objbase.h>
#include "xrCore.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "lzo.lib")

#ifdef DEBUG
#include <malloc.h>
#endif // DEBUG

XRCORE_API xrCore Core;
XRCORE_API u32 build_id;
XRCORE_API LPCSTR build_date;

namespace CPU
{
	extern void Detect();
};

static u32 init_counter = 0;

void xrCore::_initialize(LPCSTR _ApplicationName, LogCallback cb, BOOL init_fs, LPCSTR fs_fname)
{
	strcpy_s(ApplicationName, _ApplicationName);
	if (0 == init_counter)
	{

		// Init COM so we can use CoCreateInstance
		//		HRESULT co_res =
		CoInitializeEx(NULL, COINIT_MULTITHREADED);

		strcpy_s(Params, sizeof(Params), GetCommandLine());
		_strlwr_s(Params, sizeof(Params));

		string_path fn, dr, di;

		// application path
		GetModuleFileName(GetModuleHandle(MODULE_NAME), fn, sizeof(fn));
		_splitpath(fn, dr, di, 0, 0);
		strconcat(sizeof(ApplicationPath), ApplicationPath, dr, di);

		// working path
		if (strstr(Params, "-wf"))
		{
			string_path c_name;
			sscanf(strstr(Core.Params, "-wf ") + 4, "%[^ ] ", c_name);
			SetCurrentDirectory(c_name);
		}
		GetCurrentDirectory(sizeof(WorkingPath), WorkingPath);

		// User/Comp Name
		DWORD sz_user = sizeof(UserName);
		GetUserName(UserName, &sz_user);

		DWORD sz_comp = sizeof(CompName);
		GetComputerName(CompName, &sz_comp);

		// Mathematics & PSI detection
		CPU::Detect();

		Memory._initialize(strstr(Params, "-mem_debug") ? TRUE : FALSE);

		DUMP_PHASE;

		InitLog();
		_initialize_cpu();
		rtc_initialize();

		xr_FS = xr_new<CLocatorAPI>();
		xr_EFS = xr_new<EFS_Utils>();
	}

	if (init_fs)
	{
		u32 flags = 0;
		if (0 != strstr(Params, "-build"))
			flags |= CLocatorAPI::flBuildCopy;
		if (0 != strstr(Params, "-ebuild"))
			flags |= CLocatorAPI::flBuildCopy | CLocatorAPI::flEBuildCopy;
#ifdef DEBUG
		if (strstr(Params, "-cache"))
			flags |= CLocatorAPI::flCacheFiles;
		else
			flags &= ~CLocatorAPI::flCacheFiles;
#endif // DEBUG

		flags |= CLocatorAPI::flScanAppRoot;

#ifndef ELocatorAPIH
		if (0 != strstr(Params, "-file_activity"))
			flags |= CLocatorAPI::flDumpFileActivity;
#endif

		FS._initialize(flags, 0, fs_fname);
		Msg("'xrCore' build %d, %s", build_id, build_date);

#ifdef _WIN64
		Msg("Architecture: x64 \n");
#else
		Msg("Architecture: x86 \n");
#endif

		EFS._initialize();
#ifdef DEBUG

		Msg("CRT heap 0x%08x", _get_heap_handle());
		Msg("Process heap 0x%08x", GetProcessHeap());

#endif // DEBUG
	}
	SetLogCB(cb);
	init_counter++;
}

void xrCore::_destroy()
{
	--init_counter;
	if (0 == init_counter)
	{
		FS._destroy();
		EFS._destroy();
		xr_delete(xr_FS);
		xr_delete(xr_EFS);

		Memory._destroy();
	}
}

#ifndef DEBUG
constexpr auto IS_DEBUG = 0;
#else
constexpr auto IS_DEBUG = 1;
#endif

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID lpvReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		_clear87();

#ifndef _WIN64 // unsupported on x64
		_control87(_PC_53, MCW_PC);
#endif

		_control87(_RC_CHOP, MCW_RC);
		_control87(_RC_NEAR, MCW_RC);
		_control87(_MCW_EM, MCW_EM);

		auto console_cmd = GetCommandLine();
		if(strstr(console_cmd, "-dbg_console") || IS_DEBUG)
		{
			if(AllocConsole())
			{
				(void)freopen("CONIN$", "r", stdin);
				(void)freopen("CONOUT$", "w", stderr);
				(void)freopen("CONOUT$", "w", stdout);
			}
		}
	}
	break;
	case DLL_THREAD_ATTACH:
		timeBeginPeriod(1);
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
#ifdef USE_MEMORY_MONITOR
		memory_monitor::flush_each_time(true);
#endif // USE_MEMORY_MONITOR
		break;
	}
	return TRUE;
}
