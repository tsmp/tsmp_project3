#include "pch_script.h"
#include "chimera.h"

#pragma optimize("s", on)
void CChimera::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CChimera, CGameObject>("CChimera")
			 .def(constructor<>())];
}
