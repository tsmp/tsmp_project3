#include "pch_script.h"
#include "cat.h"

#pragma optimize("s", on)
void CCat::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CCat, CGameObject>("CCat")
			 .def(constructor<>())];
}
