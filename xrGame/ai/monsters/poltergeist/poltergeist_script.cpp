#include "pch_script.h"
#include "poltergeist.h"

#pragma optimize("s", on)
void CPoltergeist::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CPoltergeist, CGameObject>("CPoltergeist")
			 .def(constructor<>())];
}
