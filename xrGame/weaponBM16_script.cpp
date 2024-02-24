#include "pch_script.h"
#include "WeaponBM16.h"

#pragma optimize("s", on)
void CWeaponBM16::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CWeaponBM16, CGameObject>("CWeaponBM16")
			 .def(constructor<>())];
}
