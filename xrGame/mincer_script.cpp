#include "pch_script.h"
#include "mincer.h"

#pragma optimize("s", on)
void CMincer::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CMincer, CGameObject>("CMincer")
			 .def(constructor<>())];
}
