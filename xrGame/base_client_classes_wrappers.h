////////////////////////////////////////////////////////////////////////////
//	Module 		: base_client_classes_wrappers.h
//	Created 	: 20.12.2004
//  Modified 	: 20.12.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay base client classes wrappers
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"
#include "base_client_classes.h"
#include "engineapi.h"
#include "ispatial.h"
#include "isheduled.h"
#include "irenderable.h"
#include "icollidable.h"
#include "xr_object.h"
#include "entity.h"
#include "ai_space.h"
#include "script_engine.h"
#include <typelist.h>
#include <hierarchygenerators.h>
#include "xrServer_Object_Base.h"

template <typename _1, typename _2>
struct heritage
{
	template <typename _type, typename _base>
	struct linear_registrator : public _base, public _type
	{
	};

	template <typename _type>
	struct linear_registrator<_type, Loki::EmptyType> : public _type
	{
	};

	typedef Loki::Typelist<_1, Loki::Typelist<_2, Loki::NullType>> tl;
	typedef typename Loki::TL::Erase<tl, Loki::EmptyType>::Result pure_tl;
	typedef typename Loki::GenLinearHierarchy<pure_tl, linear_registrator>::LinBase result;
};

template <typename base, typename luabind_base = Loki::EmptyType>
class DLL_PureWrapper : public heritage<base, luabind_base>::result
{
public:
	IC DLL_PureWrapper(){};
	virtual ~DLL_PureWrapper(){};

	virtual DLL_Pure *_construct()
	{
		return (call_member<DLL_Pure *>(this, "_construct"));
	}

	static DLL_Pure *_construct_static(base *self)
	{
		return (self->base::_construct());
	}
};

typedef DLL_PureWrapper<DLL_Pure, luabind::wrap_base> CDLL_PureWrapper;

template <typename base, typename luabind_base = Loki::EmptyType>
class ISheduledWrapper : public heritage<base, luabind_base>::result
{
public:
	IC ISheduledWrapper(){};
	virtual ~ISheduledWrapper(){};

	virtual float shedule_Scale()
	{
		return 1;
		// return (call_member<float>(this,"shedule_Scale"));
	}

	virtual void shedule_Update(u32 dt)
	{
		base::shedule_Update(dt);
		// call_member<void>(this,"shedule_Update");
	}
};

typedef ISheduledWrapper<ISheduled, luabind::wrap_base> CISheduledWrapper;

template <typename base, typename luabind_base = Loki::EmptyType>
class IRenderableWrapper : public heritage<base, luabind_base>::result
{
public:
	IC IRenderableWrapper() = default;
	virtual ~IRenderableWrapper() = default;
};

typedef IRenderableWrapper<IRenderable, luabind::wrap_base> CIRenderableWrapper;
typedef DLL_PureWrapper<CGameObject, luabind::wrap_base> CGameObjectDLL_Pure;
typedef ISheduledWrapper<CGameObjectDLL_Pure> CGameObjectISheduled;
typedef IRenderableWrapper<CGameObjectISheduled> CGameObjectIRenderable;

class CGameObjectWrapper : public CGameObjectIRenderable
{
public:
	IC CGameObjectWrapper(){};
	virtual ~CGameObjectWrapper(){};
	virtual bool use(CGameObject *who_use)
	{
		return call<bool>("use", who_use);
	}

	static bool use_static(CGameObject *self, CGameObject *who_use)
	{
		return self->CGameObject::use(who_use);
	}

	virtual void net_Import(NET_Packet &packet)
	{
		call<void>("net_Import", &packet);
	}

	static void net_Import_static(CGameObject *self, NET_Packet *packet)
	{
		self->CGameObject::net_Import(*packet);
	}

	virtual void net_Export(NET_Packet &packet)
	{
		call<void>("net_Export", &packet);
	}

	static void net_Export_static(CGameObject *self, NET_Packet *packet)
	{
		self->CGameObject::net_Export(*packet);
	}

	virtual BOOL net_Spawn(CSE_Abstract *data)
	{
		return (luabind::call_member<bool>(this, "net_Spawn", data));
	}

	static bool net_Spawn_static(CGameObject *self, CSE_Abstract *abstract)
	{
		return (!!self->CGameObject::net_Spawn(abstract));
	}
};

class CEntityWrapper : public CEntity, public luabind::wrap_base
{
public:
	IC CEntityWrapper() {}
	virtual ~CEntityWrapper() {}

	virtual void HitSignal(float P, Fvector &local_dir, CObject *who, s16 element)
	{
		luabind::call_member<void>(this, "HitSignal", P, local_dir, who, element);
	}

	static void HitSignal_static(CEntity *self, float P, Fvector &local_dir, CObject *who, s16 element)
	{
		ai().script_engine().script_log(eLuaMessageTypeError, "You are trying to call a pure virtual function CEntity::HitSignal!");
	}

	virtual void HitImpulse(float P, Fvector &vWorldDir, Fvector &vLocalDir)
	{
		luabind::call_member<void>(this, "HitImpulse", P, vWorldDir, vLocalDir);
	}

	static void HitImpulse_static(float P, Fvector &vWorldDir, Fvector &vLocalDir)
	{
		ai().script_engine().script_log(eLuaMessageTypeError, "You are trying to call a pure virtual function CEntity::HitImpulse!");
	}
};
