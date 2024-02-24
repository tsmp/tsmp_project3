#include "pch_script.h"
#include "snork.h"

#pragma optimize("s", on)
void CSnork::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CSnork, CGameObject>("CSnork")
			 .def(constructor<>())];
}
