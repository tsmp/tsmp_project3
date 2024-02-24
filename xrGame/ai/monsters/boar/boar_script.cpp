#include "pch_script.h"
#include "boar.h"

#pragma optimize("s", on)
void CAI_Boar::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CAI_Boar, CGameObject>("CAI_Boar")
			 .def(constructor<>())];
}
