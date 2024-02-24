#include "pch_script.h"
#include "WeaponHPSA.h"

#pragma optimize("s", on)
void CWeaponHPSA::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CWeaponHPSA, CGameObject>("CWeaponHPSA")
			 .def(constructor<>())];
}
