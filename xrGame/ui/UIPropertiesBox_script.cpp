#include "pch_script.h"
#include "UIPropertiesBox.h"

#pragma optimize("s", on)
void CUIPropertiesBox::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CUIPropertiesBox, CUIFrameWindow>("CUIPropertiesBox")
			 .def(constructor<>())
			 .def("RemoveItem", &CUIPropertiesBox::RemoveItemByTAG)
			 .def("RemoveAll", &CUIPropertiesBox::RemoveAll)
			 .def("Show", (void (CUIPropertiesBox::*)(int, int)) & CUIPropertiesBox::Show)
			 .def("Hide", &CUIPropertiesBox::Hide)
			 .def("AutoUpdateSize", &CUIPropertiesBox::AutoUpdateSize)
			 .def("AddItem", &CUIPropertiesBox::AddItem_script)
	];
}
