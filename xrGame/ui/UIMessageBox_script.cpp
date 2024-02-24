#include "pch_script.h"
#include "UIMessageBox.h"
#include "UIMessageBoxEx.h"

#pragma optimize("s", on)
void CUIMessageBox::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CUIMessageBox, CUIStatic>("CUIMessageBox")
			 .def(constructor<>())
			 .def("Init", &CUIMessageBox::Init)
			 .def("SetText", &CUIMessageBox::SetText)
			 .def("GetHost", &CUIMessageBox::GetHost)
			 .def("GetPassword", &CUIMessageBox::GetPassword),

		 class_<CUIMessageBoxEx, CUIDialogWnd>("CUIMessageBoxEx")
			 .def(constructor<>())
			 .def("Init", &CUIMessageBoxEx::Init)
			 .def("SetText", &CUIMessageBoxEx::SetText)
			 .def("GetHost", &CUIMessageBoxEx::GetHost)
			 .def("GetPassword", &CUIMessageBoxEx::GetPassword)
	];
}
