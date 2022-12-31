#include "pch_script.h"
#include "physicobject.h"
#include "PhysicsShell.h"
#include "Physics.h"
#include "xrserver_objects_alife.h"
#include "..\include\xrRender\Kinematics.h"
#include "..\include\xrRender\KinematicsAnimated.h"
#include "xr_collide_form.h"
#include "game_object_space.h"
#include "Level.h"
#include "PHSynchronize.h"

#ifdef ANIMATED_PHYSICS_OBJECT_SUPPORT
#include "PhysicsShellAnimator.h"
#endif

CPhysicObject::CPhysicObject(void): m_net_updateData(0)
{
	m_type = epotBox;
	m_mass = 10.f;
	m_collision_hit_callback = NULL;
}

CPhysicObject::~CPhysicObject(void)
{
	xr_delete(m_net_updateData);
}

BOOL CPhysicObject::net_Spawn(CSE_Abstract *DC)
{
	CSE_Abstract *e = (CSE_Abstract *)(DC);
	CSE_ALifeObjectPhysic *po = smart_cast<CSE_ALifeObjectPhysic *>(e);
	R_ASSERT(po);
	m_type = EPOType(po->type);
	m_mass = po->mass;
	m_collision_hit_callback = NULL;
	inherited::net_Spawn(DC);
	xr_delete(collidable.model);
	switch (m_type)
	{
	case epotBox:
	case epotFixedChain:
	case epotFreeChain:
	case epotSkeleton:
		collidable.model = xr_new<CCF_Skeleton>(this);
		break;

	default:
		NODEFAULT;
	}

	CPHSkeleton::Spawn(e);
	setVisible(TRUE);
	setEnabled(TRUE);

	if (!PPhysicsShell()->isBreakable() && !CScriptBinder::object() && !CPHSkeleton::IsRemoving())
		SheduleUnregister();

#ifdef ANIMATED_PHYSICS_OBJECT_SUPPORT
	if (PPhysicsShell()->Animated())
	{
		processing_activate();
	}
#endif

	m_just_after_spawn = true;
	m_activated = false;

	if (DC->s_flags.is(M_SPAWN_UPDATE)) 
	{
		NET_Packet temp;
		temp.B.count = 0;
		DC->UPDATE_Write(temp);

		if (temp.B.count > 0)
		{
			temp.r_seek(0);
			net_Import(temp);
		}
	}

	return TRUE;
}

void CPhysicObject::SpawnInitPhysics(CSE_Abstract *D)
{
	CreatePhysicsShell(D);
	RunStartupAnim(D);
}
void CPhysicObject::RunStartupAnim(CSE_Abstract *D)
{
	if (Visual() && smart_cast<IKinematics *>(Visual()))
	{
		//		CSE_PHSkeleton	*po	= smart_cast<CSE_PHSkeleton*>(D);
		IKinematicsAnimated *PKinematicsAnimated = NULL;
		R_ASSERT(Visual() && smart_cast<IKinematics *>(Visual()));
		PKinematicsAnimated = smart_cast<IKinematicsAnimated *>(Visual());
		if (PKinematicsAnimated)
		{
			CSE_Visual *visual = smart_cast<CSE_Visual *>(D);
			R_ASSERT(visual);
			R_ASSERT2(*visual->startup_animation, "no startup animation");
			PKinematicsAnimated->PlayCycle(*visual->startup_animation);
		}
		smart_cast<IKinematics *>(Visual())->CalculateBones_Invalidate();
		smart_cast<IKinematics *>(Visual())->CalculateBones();
	}
}
void CPhysicObject::net_Destroy()
{
#ifdef ANIMATED_PHYSICS_OBJECT_SUPPORT
	if (PPhysicsShell()->Animated())
	{
		processing_deactivate();
	}
#endif

	inherited::net_Destroy();
	CPHSkeleton::RespawnInit();
}

void CPhysicObject::net_Save(NET_Packet &P)
{
	inherited::net_Save(P);
	CPHSkeleton::SaveNetState(P);
}
void CPhysicObject::CreatePhysicsShell(CSE_Abstract *e)
{
	CSE_ALifeObjectPhysic *po = smart_cast<CSE_ALifeObjectPhysic *>(e);
	CreateBody(po);
}

void CPhysicObject::CreateSkeleton(CSE_ALifeObjectPhysic *po)
{
	if (m_pPhysicsShell)
		return;
	if (!Visual())
		return;
	LPCSTR fixed_bones = *po->fixed_bones;
	m_pPhysicsShell = P_build_Shell(this, !po->_flags.test(CSE_PHSkeleton::flActive), fixed_bones);
	ApplySpawnIniToPhysicShell(&po->spawn_ini(), m_pPhysicsShell, fixed_bones[0] != '\0');
	ApplySpawnIniToPhysicShell(smart_cast<IKinematics *>(Visual())->LL_UserData(), m_pPhysicsShell, fixed_bones[0] != '\0');
}

void CPhysicObject::Load(LPCSTR section)
{
	inherited::Load(section);
	CPHSkeleton::Load(section);
}

void CPhysicObject::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
	CPHSkeleton::Update(dt);
}
void CPhysicObject::UpdateCL()
{
	inherited::UpdateCL();

#ifdef ANIMATED_PHYSICS_OBJECT_SUPPORT
	//Если наш физический объект анимированный, то
	//двигаем объект за анимацией
	if (m_pPhysicsShell->PPhysicsShellAnimator())
	{
		m_pPhysicsShell->PPhysicsShellAnimator()->OnFrame();
	}
#endif

	if (!IsGameTypeSingle())	
		Interpolate();	

	PHObjectPositionUpdate();
}

void CPhysicObject::PHObjectPositionUpdate()
{
	if (!m_pPhysicsShell)
		return;

	if (m_type == epotBox)
	{
		m_pPhysicsShell->Update();
		XFORM().set(m_pPhysicsShell->mXFORM);
	}
	else
		m_pPhysicsShell->InterpolateGlobalTransform(&XFORM());
}

void CPhysicObject::AddElement(CPhysicsElement *root_e, int id)
{
	IKinematics *K = smart_cast<IKinematics *>(Visual());

	CPhysicsElement *E = P_create_Element();
	CBoneInstance &B = K->LL_GetBoneInstance(u16(id));
	E->mXFORM.set(K->LL_GetTransform(u16(id)));
	Fobb bb = K->LL_GetBox(u16(id));

	if (bb.m_halfsize.magnitude() < 0.05f)
	{
		bb.m_halfsize.add(0.05f);
	}
	E->add_Box(bb);
	E->setMass(10.f);
	E->set_ParentElement(root_e);
	B.set_callback(bctPhysics, m_pPhysicsShell->GetBonesCallback(), E);
	m_pPhysicsShell->add_Element(E);
	if (!(m_type == epotFreeChain && root_e == 0))
	{
		CPhysicsJoint *J = P_create_Joint(CPhysicsJoint::full_control, root_e, E);
		J->SetAnchorVsSecondElement(0, 0, 0);
		J->SetAxisDirVsSecondElement(1, 0, 0, 0);
		J->SetAxisDirVsSecondElement(0, 1, 0, 2);
		J->SetLimits(-M_PI / 2, M_PI / 2, 0);
		J->SetLimits(-M_PI / 2, M_PI / 2, 1);
		J->SetLimits(-M_PI / 2, M_PI / 2, 2);
		m_pPhysicsShell->add_Joint(J);
	}

	CBoneData &BD = K->LL_GetData(u16(id));
	for (vecBonesIt it = BD.children.begin(); BD.children.end() != it; ++it)
	{
		AddElement(E, (*it)->GetSelfID());
	}
}

void CPhysicObject::CreateBody(CSE_ALifeObjectPhysic *po)
{

	if (m_pPhysicsShell)
		return;
	IKinematics *pKinematics = smart_cast<IKinematics *>(Visual());
	switch (m_type)
	{
	case epotBox:
	{
		m_pPhysicsShell = P_build_SimpleShell(this, m_mass, !po->_flags.test(CSE_ALifeObjectPhysic::flActive));
	}
	break;
	case epotFixedChain:
	case epotFreeChain:
	{
		m_pPhysicsShell = P_create_Shell();
		m_pPhysicsShell->set_Kinematics(pKinematics);
		AddElement(0, pKinematics->LL_GetBoneRoot());
		m_pPhysicsShell->setMass1(m_mass);
	}
	break;

	case epotSkeleton:
	{
		//pKinematics->LL_SetBoneRoot(0);
		CreateSkeleton(po);
	}
	break;

	default:
	{
	}
	break;
	}

	m_pPhysicsShell->mXFORM.set(XFORM());
	m_pPhysicsShell->SetAirResistance(0.001f, 0.02f);
	if (pKinematics)
	{

		SAllDDOParams disable_params;
		disable_params.Load(pKinematics->LL_UserData());
		m_pPhysicsShell->set_DisableParams(disable_params);
	}
	//m_pPhysicsShell->SetAirResistance(0.002f, 0.3f);
}

BOOL CPhysicObject::net_SaveRelevant()
{
	return TRUE; //!m_flags.test(CSE_ALifeObjectPhysic::flSpawnCopy);
}

BOOL CPhysicObject::UsedAI_Locations()
{
	return (FALSE);
}

void CPhysicObject::InitServerObject(CSE_Abstract *D)
{
	CPHSkeleton::InitServerObject(D);
	CSE_ALifeObjectPhysic *l_tpALifePhysicObject = smart_cast<CSE_ALifeObjectPhysic *>(D);
	if (!l_tpALifePhysicObject)
		return;
	l_tpALifePhysicObject->type = u32(m_type);
}
SCollisionHitCallback *CPhysicObject::get_collision_hit_callback()
{
	return m_collision_hit_callback;
}
bool CPhysicObject::set_collision_hit_callback(SCollisionHitCallback *cc)
{
	if (!cc)
	{
		m_collision_hit_callback = NULL;
		return true;
	}
	if (PPhysicsShell())
	{
		VERIFY2(cc->m_collision_hit_callback != 0, "No callback function");
		m_collision_hit_callback = cc;
		return true;
	}
	else
		return false;
}

// network synchronization ----------------------------
net_updatePhData* CPhysicObject::NetSync()
{
	if (!m_net_updateData)
		m_net_updateData = xr_new<net_updatePhData>();

	return m_net_updateData;
}

void CPhysicObject::net_Export(NET_Packet& P)
{
	if (this->H_Parent() || IsGameTypeSingle())
	{
		P.w_u8(0);
		return;
	}

	CPHSynchronize* pSyncObj = nullptr;
	SPHNetState	State;
	pSyncObj = this->PHGetSyncItem(0);
	R_ASSERT(pSyncObj);
	pSyncObj->get_State(State);

	mask_num_items num_items;
	num_items.mask = 0;
	u16	temp = this->PHGetSyncItemsNumber();
	R_ASSERT(temp < (u16(1) << 5));
	num_items.num_items = u8(temp);

	if (State.enabled)
		num_items.mask |= CSE_ALifeObjectPhysic::inventory_item_state_enabled;

	if (fis_zero(State.angular_vel.square_magnitude()))
		num_items.mask |= CSE_ALifeObjectPhysic::inventory_item_angular_null;

	if (fis_zero(State.linear_vel.square_magnitude()))
		num_items.mask |= CSE_ALifeObjectPhysic::inventory_item_linear_null;

	P.w_u8(num_items.common);
	net_Export_PH_Params(P, State, num_items);

	if (PPhysicsShell()->isEnabled())	
		P.w_u8(1);	//not freezed	
	else	
		P.w_u8(0);  //freezed	
}

void CPhysicObject::net_Export_PH_Params(NET_Packet &P, SPHNetState &State, mask_num_items &num_items)
{
	P.w_vec3(State.force);
	P.w_vec3(State.torque);
	P.w_vec3(State.position);

	float magnitude = _sqrt(State.quaternion.magnitude());

	if (fis_zero(magnitude)) 
	{
		magnitude = 1;
		State.quaternion.x = 0.f;
		State.quaternion.y = 0.f;
		State.quaternion.z = 1.f;
		State.quaternion.w = 0.f;
	}
	
	P.w_float(State.quaternion.x);
	P.w_float(State.quaternion.y);
	P.w_float(State.quaternion.z);
	P.w_float(State.quaternion.w);

	if (!(num_items.mask & CSE_ALifeObjectPhysic::inventory_item_angular_null)) 
	{
		P.w_float(State.angular_vel.x);
		P.w_float(State.angular_vel.y);
		P.w_float(State.angular_vel.z);
	}

	if (!(num_items.mask & CSE_ALifeObjectPhysic::inventory_item_linear_null)) 
	{
		P.w_float(State.linear_vel.x);
		P.w_float(State.linear_vel.y);
		P.w_float(State.linear_vel.z);
	}
}

void CPhysicObject::net_Import(NET_Packet &P)
{
	u8 NumItems = 0;
	NumItems = P.r_u8();

	if (!NumItems)
		return;

	CSE_ALifeObjectPhysic::mask_num_items num_items;
	num_items.common = NumItems;
	NumItems = num_items.num_items;

	net_update_PItem N;
	N.dwTimeStamp = Device.dwTimeGlobal;
	net_Import_PH_Params(P, N, num_items);
	P.r_u8();	// freezed or not..

	if (this->cast_game_object()->Local())	
		return;	
		
	Level().AddObject_To_Objects4CrPr(this);
	net_updatePhData* p = NetSync();
	p->NET_IItem.push_back(N);

	while (p->NET_IItem.size() > 2)	
		p->NET_IItem.pop_front();
	
	if (!m_activated)
	{
		//Msg("- Activating object [%d][%s] before interpolation starts", ID(), Name());
		processing_activate();
		m_activated = true;
	}
};

void CPhysicObject::net_Import_PH_Params(NET_Packet &P, net_update_PItem &N, mask_num_items &num_items)
{
	P.r_vec3(N.State.force);
	P.r_vec3(N.State.torque);
	P.r_vec3(N.State.position);

	P.r_float(N.State.quaternion.x);
	P.r_float(N.State.quaternion.y);
	P.r_float(N.State.quaternion.z);
	P.r_float(N.State.quaternion.w);

	N.State.enabled = num_items.mask & CSE_ALifeObjectPhysic::inventory_item_state_enabled;

	if (!(num_items.mask & CSE_ALifeObjectPhysic::inventory_item_angular_null)) 
	{
		N.State.angular_vel.x = P.r_float();
		N.State.angular_vel.y = P.r_float();
		N.State.angular_vel.z = P.r_float();
	}
	else
		N.State.angular_vel.set(0.f, 0.f, 0.f);

	if (!(num_items.mask & CSE_ALifeObjectPhysic::inventory_item_linear_null)) 
	{
		N.State.linear_vel.x = P.r_float();
		N.State.linear_vel.y = P.r_float();
		N.State.linear_vel.z = P.r_float();
	}
	else
		N.State.linear_vel.set(0.f, 0.f, 0.f);

#pragma TODO("TSMP: проверить нужны ли эти previous")
	N.State.previous_position = N.State.position;
	N.State.previous_quaternion = N.State.quaternion;
}

void CPhysicObject::PH_A_CrPr()
{
	if (!m_just_after_spawn)
		return;

	VERIFY(Visual());
	IKinematics* K = ((IRenderVisual*)Visual())->dcast_PKinematics();
	VERIFY(K);

	if (!PPhysicsShell())
		return;

	if (!PPhysicsShell()->isFullActive())
	{
		K->CalculateBones_Invalidate();
		K->CalculateBones(TRUE);
	}

	PPhysicsShell()->GetGlobalTransformDynamic(&XFORM());
	K->CalculateBones_Invalidate();
	K->CalculateBones(TRUE);

	spatial_move();
	m_just_after_spawn = false;

	VERIFY(!OnServer());

	PPhysicsShell()->get_ElementByStoreOrder(0)->Fix();
	PPhysicsShell()->SetIgnoreStatic();	
}

void CPhysicObject::CalculateInterpolationParams()
{
	if (this->m_pPhysicsShell)
		this->m_pPhysicsShell->NetInterpolationModeON();
}

void CPhysicObject::Interpolate()
{
	net_updatePhData* p = NetSync();
	CPHSynchronize* pSyncObj = this->PHGetSyncItem(0);

	if (H_Parent() || !getVisible() || !m_pPhysicsShell || OnServer() || p->NET_IItem.empty())
		return;

	//simple linear interpolation...
	SPHNetState newState = p->NET_IItem.front().State;

	if (p->NET_IItem.size() >= 2)
	{
		float ret_interpolate = interpolate_states(p->NET_IItem.front(), p->NET_IItem.back(), newState);

		if (ret_interpolate >= 1.f)
		{
			p->NET_IItem.pop_front();

			if (m_activated)
			{
				//Msg("- Deactivating object [%d][%s] after interpolation finish", ID(), Name());
				processing_deactivate();
				m_activated = false;
			}
		}
	}

	pSyncObj->set_State(newState);	
}

float CPhysicObject::interpolate_states(net_update_PItem const &first, net_update_PItem const &last, SPHNetState &current)
{
	float ret_val = 0.f;
	u32 CurTime = Device.dwTimeGlobal;

	if (CurTime == last.dwTimeStamp)
		return 0.f;

	float factor = float(CurTime - last.dwTimeStamp) / float(last.dwTimeStamp - first.dwTimeStamp);
	ret_val = factor;

	if (factor > 1.f)	
		factor = 1.f;	
	else if (factor < 0.f)	
		factor = 0.f;	

	current.position.x = first.State.position.x + (factor * (last.State.position.x - first.State.position.x));
	current.position.y = first.State.position.y + (factor * (last.State.position.y - first.State.position.y));
	current.position.z = first.State.position.z + (factor * (last.State.position.z - first.State.position.z));
	current.previous_position = current.position;

#pragma TODO("TSMP: проверить нужны ли эти previous")
	current.quaternion.slerp(first.State.quaternion, last.State.quaternion, factor);
	current.previous_quaternion = current.quaternion;
	return ret_val;
}
