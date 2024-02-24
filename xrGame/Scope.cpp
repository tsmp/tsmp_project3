#include "pch_script.h"
#include "scope.h"

CScope::CScope()
{
}

CScope::~CScope()
{
}

#pragma optimize("s", on)
void CScope::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CScope, CGameObject>("CScope")
			 .def(constructor<>())];
}
