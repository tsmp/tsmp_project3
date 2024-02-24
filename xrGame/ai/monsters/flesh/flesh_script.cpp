#include "pch_script.h"
#include "flesh.h"

#pragma optimize("s", on)
void CAI_Flesh::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CAI_Flesh, CGameObject>("CAI_Flesh")
			 .def(constructor<>())];
}
