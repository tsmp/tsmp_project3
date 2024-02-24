#pragma once

class CScriptEngine;
struct lua_State;

// sampling interval
extern int script_profile_interval_ms;

class ScriptProfiler
{
public:
	static void SetScriptEngine(CScriptEngine* engine);
	static void OnScriptsReinit();

	static void StartProfile();
	static void StopProfile();

private:
	static inline CScriptEngine* m_Engine = nullptr;
	static inline bool m_ProfilingEnabled = false;
	static inline xr_vector<xr_string> m_Stacks;

	static void LuajutProfileCallback(void* data, lua_State* L, int samples, int vmstate);
	static void InstallCallback();
	static void SaveProfile();
};
