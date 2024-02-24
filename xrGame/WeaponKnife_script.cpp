#include "pch_script.h"
#include "WeaponKnife.h"

#pragma optimize("s", on)
void CWeaponKnife::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CWeaponKnife, CGameObject>("CWeaponKnife")
			 .def(constructor<>())];
}
