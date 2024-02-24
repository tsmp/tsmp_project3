#include "pch_script.h"
#include "torch.h"

#pragma optimize("s", on)
void CTorch::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CTorch, CGameObject>("CTorch")
			 .def(constructor<>())];
}
