#include "pch_script.h"
#include "UIMapInfo.h"

#pragma optimize("s", on)
void CUIMapInfo::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CUIMapInfo, CUIWindow>("CUIMapInfo")
			 .def(constructor<>())
			 .def("Init", &CUIMapInfo::Init)
			 .def("InitMap", &CUIMapInfo::InitMap)];
}
