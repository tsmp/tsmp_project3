#include "stdafx.h"

#ifdef DEBUG
#include "ode_include.h"
#include "StatGraph.h"
#include "PHDebug.h"
#endif

#include "alife_space.h"
#include "hit.h"
#include "PHDestroyable.h"
#include "car.h"
#include "..\include\xrRender\Kinematics.h"
#include "PHWorld.h"

extern CPHWorld* ph_world;

CCar::STireSmoke::~STireSmoke()
{
	CParticlesObject::Destroy(p_pgobject);
}

void CCar::STireSmoke::Init()
{
	VERIFY(!ph_world->Processing());
	pelement = (bone_map.find(bone_id))->second.element;
	IKinematics* K = smart_cast<IKinematics*>(pcar->Visual());
	CBoneData& bone_data = K->LL_GetData(u16(bone_id));
	transform.set(bone_data.bind_transform);
	p_pgobject = CParticlesObject::Create(*pcar->m_tire_smoke_particles, FALSE);
	Fvector zero_vector;
	zero_vector.set(0.f, 0.f, 0.f);
	p_pgobject->UpdateParent(pcar->XFORM(), zero_vector);
}

void CCar::STireSmoke::Update()
{
	VERIFY(!ph_world->Processing());
	Fmatrix global_transform;
	pelement->InterpolateGlobalTransform(&global_transform);
	global_transform.mulB_43(transform);
	dVector3 res;
	Fvector res_vel;
	dBodyGetPointVel(pelement->get_body(), global_transform.c.x, global_transform.c.y, global_transform.c.z, res);
	CopyMemory(&res_vel, res, sizeof(Fvector));
	p_pgobject->UpdateParent(global_transform, res_vel);
}

void CCar::STireSmoke::Clear()
{
	CParticlesObject::Destroy(p_pgobject);
}

void CCar::STireSmoke::Play()
{
	VERIFY(!ph_world->Processing());
	p_pgobject->Play();
	Update();
}

void CCar::STireSmoke::Stop()
{
	VERIFY(!ph_world->Processing());
	p_pgobject->Stop();
}