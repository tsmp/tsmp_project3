#include "pch_script.h"
#include "tushkano.h"

#pragma optimize("s", on)
void CTushkano::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CTushkano, CGameObject>("CTushkano")
			 .def(constructor<>())];
}
