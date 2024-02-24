#include "pch_script.h"
#include "UIProgressBar.h"

#pragma optimize("s", on)
void CUIProgressBar::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CUIProgressBar, CUIWindow>("CUIProgressBar")
			 .def(constructor<>())
			 .def("SetProgressPos", &CUIProgressBar::SetProgressPos)
			 .def("GetProgressPos", &CUIProgressBar::GetProgressPos)

			 .def("GetRange_min", &CUIProgressBar::GetRange_min)
			 .def("GetRange_max", &CUIProgressBar::GetRange_max)

	];
}
