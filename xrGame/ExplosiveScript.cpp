#include "pch_script.h"
#include "Explosive.h"

#pragma optimize("s", on)
void CExplosive::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CExplosive>("explosive")
			 .def("explode", (&CExplosive::Explode))];
}
