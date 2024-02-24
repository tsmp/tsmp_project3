#include "pch_script.h"
#include "f1.h"

#pragma optimize("s", on)
void CF1::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CF1, CGameObject>("CF1")
			 .def(constructor<>())];
}
