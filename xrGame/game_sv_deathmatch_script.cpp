#include "pch_script.h"
#include "game_sv_deathmatch.h"
#include "xrServer_script_macroses.h"
#include "xrserver.h"

template <typename T>
struct CWrapperBase : public T, public luabind::wrap_base
{
	typedef T inherited;
	typedef CWrapperBase<T> self_type;
	DEFINE_LUA_WRAPPER_CONST_METHOD_0(type_name, LPCSTR)
};

#pragma optimize("s", on)
void game_sv_Deathmatch::script_register(lua_State *L)
{
	using namespace luabind;

	typedef CWrapperBase<game_sv_Deathmatch> WrapType;
	module(L)
		[luabind::class_<game_sv_Deathmatch, WrapType, game_sv_GameState>("game_sv_Deathmatch")
			 .def(constructor<>())
			 .def("GetTeamData", &game_sv_Deathmatch::GetTeamData)

			 .def("type_name", &WrapType::type_name, &WrapType::type_name_static)
	];
}
