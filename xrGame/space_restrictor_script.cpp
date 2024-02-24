#include "pch_script.h"
#include "space_restrictor.h"

#pragma optimize("s", on)
void CSpaceRestrictor::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CSpaceRestrictor, CGameObject>("CSpaceRestrictor")
			 .def(constructor<>())];
}
