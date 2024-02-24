////////////////////////////////////////////////////////////////////////////
//	Module 		: base_client_classes_script.cpp
//	Created 	: 20.12.2004
//  Modified 	: 20.12.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay base client classes script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "base_client_classes.h"
#include "base_client_classes_wrappers.h"
#include "feel_sound.h"
#include "..\Include\xrRender\RenderVisual.h"
#include "..\include\xrRender\KinematicsAnimated.h"
#include "ai/stalker/ai_stalker.h"
#include "../xrNetwork/net_utils.h"

#pragma optimize("s", on)
void DLL_PureScript::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<DLL_Pure, CDLL_PureWrapper>("DLL_Pure")
			 .def(constructor<>())
			 .def("_construct", &DLL_Pure::_construct, &CDLL_PureWrapper::_construct_static)];
}

void ISheduledScript::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<ISheduled, CISheduledWrapper>("ISheduled")
	];
}

void IRenderableScript::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<IRenderable, CIRenderableWrapper>("IRenderable")
	];
}

void ICollidableScript::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<ICollidable>("ICollidable")
			 .def(constructor<>())];
}

void CObjectScript::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[
			class_<CGameObject, bases<DLL_Pure, ISheduled, ICollidable, IRenderable>, CGameObjectWrapper>("CGameObject")
				.def(constructor<>())
				.def("_construct", &CGameObject::_construct, &CGameObjectWrapper::_construct_static)
				.def("Visual", &CGameObject::Visual)
		
				.def("net_Export", &CGameObject::net_Export, &CGameObjectWrapper::net_Export_static)
				.def("net_Import", &CGameObject::net_Import, &CGameObjectWrapper::net_Import_static)
				.def("net_Spawn", &CGameObject::net_Spawn, &CGameObjectWrapper::net_Spawn_static)

				.def("use", &CGameObject::use, &CGameObjectWrapper::use_static)

				.def("getVisible", &CGameObject::getVisible)
				.def("getEnabled", &CGameObject::getEnabled)
	];
}

void IRender_VisualScript::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<IRenderVisual>("IRender_Visual")
			 .def("dcast_PKinematicsAnimated", &IRenderVisual::dcast_PKinematicsAnimated)];
}

void CKinematicsAnimated_PlayCycle(IKinematicsAnimated *sa, LPCSTR anim)
{
	sa->PlayCycle(anim);
}

void CKinematicsAnimatedScript::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<IKinematicsAnimated>("IKinematicsAnimated")
			 .def("PlayCycle", &CKinematicsAnimated_PlayCycle)];
}

void CBlendScript::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CBlend>("CBlend")
	];
}
