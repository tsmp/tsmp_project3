#include "pch_script.h"
#include "weaponsvu.h"

CWeaponSVU::CWeaponSVU(void) : CWeaponCustomPistol("SVU")
{
}

CWeaponSVU::~CWeaponSVU(void)
{
}

#pragma optimize("s", on)
void CWeaponSVU::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CWeaponSVU, CGameObject>("CWeaponSVU")
			 .def(constructor<>())];
}
