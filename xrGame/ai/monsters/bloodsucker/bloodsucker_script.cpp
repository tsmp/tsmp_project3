#include "pch_script.h"
#include "bloodsucker.h"

#pragma optimize("s", on)
void CAI_Bloodsucker::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CAI_Bloodsucker, CGameObject>("CAI_Bloodsucker")
			 .def(constructor<>())];
}
