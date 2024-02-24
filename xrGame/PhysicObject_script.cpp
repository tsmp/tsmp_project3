#include "pch_script.h"
#include "PhysicObject.h"

#pragma optimize("s", on)
void CPhysicObject::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CPhysicObject, CGameObject>("CPhysicObject")
			 .def(constructor<>())];
}
