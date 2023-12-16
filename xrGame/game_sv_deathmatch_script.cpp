#include "pch_script.h"
#include "game_sv_deathmatch.h"
#include "xrServer_script_macroses.h"
#include "xrserver.h"
#include "Level.h"

using namespace luabind;

template <typename T>
struct CWrapperBase : public T, public luabind::wrap_base
{
	typedef T inherited;
	typedef CWrapperBase<T> self_type;
	DEFINE_LUA_WRAPPER_CONST_METHOD_0(type_name, LPCSTR)
};

game_sv_Deathmatch* GetDMGame()
{
	return smart_cast<game_sv_Deathmatch*>(Level().Server->game);
}

void game_sv_Deathmatch::SetScriptTeamScore(u32 idx, int val)
{
	VERIFY((idx >= 0) && (idx < m_TeamsScores.size()));
	m_TeamsScores[idx] = val;
	signal_Syncronize();
}

#pragma optimize("s", on)
void game_sv_Deathmatch::script_register(lua_State *L)
{
	typedef CWrapperBase<game_sv_Deathmatch> WrapType;
	module(L)
		[def("get_dmgame", &GetDMGame),
		luabind::class_<game_sv_Deathmatch, WrapType, game_sv_GameState>("game_sv_Deathmatch")
			 .def(constructor<>())
			 .def("GetTeamData", &game_sv_Deathmatch::GetTeamData)
			.def("GetTeamScore", &game_sv_Deathmatch::GetTeamScore)
			.def("SetTeamScore", &SetScriptTeamScore)

			 .def("type_name", &WrapType::type_name, &WrapType::type_name_static)
	];
}
