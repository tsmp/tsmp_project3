#include "pch_script.h"
#include "ai_trader.h"

#pragma optimize("s", on)
void CAI_Trader::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CAI_Trader, CGameObject>("CAI_Trader")
			 .def(constructor<>())];
}
