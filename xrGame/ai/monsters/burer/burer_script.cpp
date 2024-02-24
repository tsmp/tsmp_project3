#include "pch_script.h"
#include "burer.h"

#pragma optimize("s", on)
void CBurer::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CBurer, CGameObject>("CBurer")
			 .def(constructor<>())];
}
