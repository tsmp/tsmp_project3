#include "pch_script.h"
#include "HairsZone.h"

#pragma optimize("s", on)
void CHairsZone::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CHairsZone, CGameObject>("CHairsZone")
			 .def(constructor<>())];
}
