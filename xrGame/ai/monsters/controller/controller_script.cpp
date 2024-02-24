#include "pch_script.h"
#include "controller.h"

#pragma optimize("s", on)
void CController::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CController, CGameObject>("CController")
			 .def(constructor<>())];
}
