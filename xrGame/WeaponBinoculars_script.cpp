#include "pch_script.h"
#include "weaponbinoculars.h"

void CWeaponBinoculars::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CWeaponBinoculars, CGameObject>("CWeaponBinoculars")
			 .def(constructor<>())];
}
