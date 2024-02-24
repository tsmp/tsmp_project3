#include "pch_script.h"
#include "zombie.h"

#pragma optimize("s", on)
void CZombie::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CZombie, CGameObject>("CZombie")
			 .def(constructor<>())];
}
