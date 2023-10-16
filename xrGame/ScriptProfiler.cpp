#include "stdafx.h"
#include "ScriptProfiler.h"
#include "script_engine.h"

int script_profile_interval_ms = 10;

void ScriptProfiler::LuajutProfileCallback(void* data, lua_State* L, int samples, int vmstate)
{
	/*
	* p — Preserve the full path for module names. Otherwise only the file name is used.
	* f — Dump the function name if it can be derived. Otherwise use module:line.
	* F — Ditto, but dump module:name.
	* l — Dump module:line.
	* Z — Zap the following characters for the last dumped frame.
	* All other characters are added verbatim to the output string.
	*/
	const char* format = "f l \t";
	const int maxFrames = 15;
	size_t stackStrLen = 0;
	const char* stack = luaJIT_profile_dumpstack(L, format, maxFrames, &stackStrLen);
	ScriptProfiler::m_Stacks.emplace_back(xr_string(stack, stackStrLen));
}

void ScriptProfiler::SetScriptEngine(CScriptEngine* engine)
{
	if (!engine)
		StopProfile();

	m_Engine = engine;
}

void ScriptProfiler::OnScriptsReinit()
{
	if (m_ProfilingEnabled)
		InstallCallback();
}

void ScriptProfiler::StartProfile()
{
	if (m_ProfilingEnabled)
	{
		Msg("! script profiling is already enabled!");
		return;
	}

	InstallCallback();
	m_ProfilingEnabled = true;
	Msg("- started profiling");
}

void ScriptProfiler::StopProfile()
{
	if (!m_ProfilingEnabled)
		return;

	if (m_Engine)
	{
		auto luaVM = m_Engine->lua();
		luaJIT_profile_stop(luaVM);
	}

	m_ProfilingEnabled = false;
	SaveProfile();
	m_Stacks.clear();
}

void ScriptProfiler::InstallCallback()
{
	if (m_Engine)
	{
		auto luaVM = m_Engine->lua();
		const std::string settings = "fi" + std::to_string(script_profile_interval_ms);
		luaJIT_profile_start(luaVM, settings.c_str(), LuajutProfileCallback, nullptr);
	}
}

void ScriptProfiler::SaveProfile()
{
	string_path profileFullName;

	string64 timeStamp;
	timestamp(timeStamp);

	strcpy(profileFullName, timeStamp);
	strcat(profileFullName, ".stluaprof");

	FS.update_path(profileFullName, "$app_data_root$", profileFullName);
	bool writeAllowed = true;

	if (FS.exist(profileFullName))
		writeAllowed = SetFileAttributes(profileFullName, FILE_ATTRIBUTE_NORMAL);

	if (!writeAllowed)
	{
		Msg("! ERROR: Could not save profile to [%s]!", profileFullName);
		return;
	}

	IWriter* F = FS.w_open(profileFullName);
	int idx = 0;

	for (const xr_string& stack : m_Stacks)	
		F->w_printf("%s\r\n", stack.c_str());

	FS.w_close(F);
	Msg("- profile saved to [%s]", profileFullName);
}
