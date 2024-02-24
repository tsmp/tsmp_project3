#include "pch_script.h"
#include "weapongroza.h"
#include "WeaponHUD.h"

CWeaponGroza::CWeaponGroza(void) : CWeaponMagazinedWGrenade("GROZA", SOUND_TYPE_WEAPON_SUBMACHINEGUN)
{
	m_weight = 1.5f;
	m_slot = 2;
}

CWeaponGroza::~CWeaponGroza(void)
{
}

#pragma optimize("s", on)
void CWeaponGroza::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CWeaponGroza, CGameObject>("CWeaponGroza")
			 .def(constructor<>())];
}
