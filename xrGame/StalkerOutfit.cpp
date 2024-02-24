#include "pch_script.h"
#include "StalkerOutfit.h"

CStalkerOutfit::CStalkerOutfit()
{
}

CStalkerOutfit::~CStalkerOutfit()
{
}

#pragma optimize("s", on)
void CStalkerOutfit::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CStalkerOutfit, CGameObject>("CStalkerOutfit")
			 .def(constructor<>())];
}
