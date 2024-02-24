#include "pch_script.h"
#include "MosquitoBald.h"

#pragma optimize("s", on)
void CMosquitoBald::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CMosquitoBald, CGameObject>("CMosquitoBald")
			 .def(constructor<>())];
}
