#pragma once

#include "gameobject.h"
#include "physicsshellholder.h"
#include "physicsskeletonobject.h"
#include "PHSkeleton.h"
#include "script_export_space.h"
#include "xrserver_objects_alife.h"

class CSE_ALifeObjectPhysic;
class CPhysicsElement;
class CSE_ALifeObjectPhysic;
struct SPHNetState;

using mask_num_items = CSE_ALifeObjectPhysic::mask_num_items;

struct net_update_PItem
{
	u32	dwTimeStamp;
	SPHNetState	State;
};

struct net_updatePhData 
{
	xr_deque<net_update_PItem> NET_IItem;
	u32	m_dwIStartTime;
	u32	m_dwIEndTime;
};

class CPhysicObject : public CPhysicsShellHolder,
					  public CPHSkeleton
{
	using inherited = CPhysicsShellHolder;
	EPOType m_type;
	float m_mass;
	SCollisionHitCallback *m_collision_hit_callback;
	net_updatePhData* m_net_updateData;

private:
	//Creating
	void CreateBody(CSE_ALifeObjectPhysic *po);
	void CreateSkeleton(CSE_ALifeObjectPhysic *po);
	void AddElement(CPhysicsElement *root_e, int id);

public:
	CPhysicObject(void);
	virtual ~CPhysicObject(void);

	virtual	void Interpolate();
	float interpolate_states(net_update_PItem const& first, net_update_PItem const& last, SPHNetState& current);

	virtual BOOL net_Spawn(CSE_Abstract *DC);
	virtual void CreatePhysicsShell(CSE_Abstract *e);
	virtual void net_Destroy();
	virtual void Load(LPCSTR section);
	virtual void shedule_Update(u32 dt);
	virtual void UpdateCL();
	virtual void net_Save(NET_Packet &P);
	virtual BOOL net_SaveRelevant();
	virtual BOOL UsedAI_Locations();
	virtual SCollisionHitCallback *get_collision_hit_callback();
	virtual bool set_collision_hit_callback(SCollisionHitCallback *cc);

	virtual void net_Export(NET_Packet& P);
	virtual void net_Import(NET_Packet& P);

	virtual void PH_B_CrPr() {} 	// actions & operations before physic correction-prediction steps
	virtual void PH_I_CrPr() {} 	// actions & operations after correction before prediction steps
	virtual void PH_A_CrPr();		// actions & operations after phisic correction-prediction steps

protected:
	virtual void SpawnInitPhysics(CSE_Abstract *D);
	virtual void RunStartupAnim(CSE_Abstract *D);
	virtual CPhysicsShellHolder *PPhysicsShellHolder() { return PhysicsShellHolder(); }
	virtual CPHSkeleton *PHSkeleton() { return this; }
	virtual void InitServerObject(CSE_Abstract *po);
	virtual void PHObjectPositionUpdate();

	void net_Export_PH_Params(NET_Packet &P, SPHNetState &State, mask_num_items &num_items);
	void net_Import_PH_Params(NET_Packet &P, net_update_PItem &updItem, mask_num_items &num_items);		
	void CalculateInterpolationParams();
	net_updatePhData* NetSync();

	bool m_just_after_spawn;
	bool m_activated;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CPhysicObject)
#undef script_type_list
#define script_type_list save_type_list(CPhysicObject)
