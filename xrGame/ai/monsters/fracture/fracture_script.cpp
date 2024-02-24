#include "pch_script.h"
#include "fracture.h"

#pragma optimize("s", on)
void CFracture::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CFracture, CGameObject>("CFracture")
			 .def(constructor<>())];
}
