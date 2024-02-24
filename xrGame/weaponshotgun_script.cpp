#include "pch_script.h"
#include "WeaponShotgun.h"

#pragma optimize("s",on)
void CWeaponShotgun::script_register	(lua_State *L)
{
	using namespace luabind;

	module(L)
	[
		class_<CWeaponShotgun,CGameObject>("CWeaponShotgun")
			.def(constructor<>())
	];
}
