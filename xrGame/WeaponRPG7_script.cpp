#include "pch_script.h"
#include "WeaponRPG7.h"

#pragma optimize("s", on)
void CWeaponRPG7::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CWeaponRPG7, CGameObject>("CWeaponRPG7")
			 .def(constructor<>())];
}
