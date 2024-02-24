#include "pch_script.h"
#include "pseudo_gigant.h"

#pragma optimize("s", on)
void CPseudoGigant::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CPseudoGigant, CGameObject>("CPseudoGigant")
			 .def(constructor<>())];
}
