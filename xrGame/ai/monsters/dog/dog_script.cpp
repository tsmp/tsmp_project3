#include "pch_script.h"
#include "dog.h"

#pragma optimize("s", on)
void CAI_Dog::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CAI_Dog, CGameObject>("CAI_Dog")
			 .def(constructor<>())];
}
