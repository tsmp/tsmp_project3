#include "pch_script.h"
#include "WeaponFORT.h"

CWeaponFORT::CWeaponFORT() : CWeaponPistol("FORT")
{
}

CWeaponFORT::~CWeaponFORT()
{
}

#pragma optimize("s", on)
void CWeaponFORT::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CWeaponFORT, CGameObject>("CWeaponFORT")
			 .def(constructor<>())];
}
