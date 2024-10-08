////////////////////////////////////////////////////////////////////////////
//	Module 		: base_monster_feel.cpp
//	Created 	: 26.05.2003
//  Modified 	: 26.05.2003
//	Author		: Serge Zhem
//	Description : Visibility and look for all the biting monsters
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "base_monster.h"
#include "../../../actor.h"
#include "../../../ActorEffector.h"
#include "../ai_monster_effector.h"
#include "../../../hudmanager.h"
#include "../../../clsid_game.h"
#include "..\include\xrRender\Kinematics.h"
#include "../../../sound_player.h"
#include "../../../level.h"
#include "../../../script_callback_ex.h"
#include "../../../script_game_object.h"
#include "../../../game_object_space.h"
#include "../../../ai_monster_space.h"
#include "../control_animation_base.h"
#include "../../../UIGameCustom.h"
#include "../../../UI/UIStatic.h"
#include "../../../ai_object_location.h"
#include "../../../profiler.h"
#include "../../../ActorEffector.h"
#include "CameraBase.h"

void CBaseMonster::feel_sound_new(CObject *who, int eType, CSound_UserDataPtr user_data, const Fvector &Position, float power)
{
	if (!g_Alive())
		return;

	// ignore my sounds
	if (this == who)
		return;

	if (user_data)
		user_data->accept(sound_user_data_visitor());

	// ignore unknown sounds
	if (eType == 0xffffffff)
		return;

	// ignore distant sounds
	Fvector center;
	Center(center);
	float dist = center.distance_to(Position);
	if (dist > db().m_max_hear_dist)
		return;

	// ignore sounds if not from enemies and not help sounds
	CEntityAlive *entity = smart_cast<CEntityAlive *>(who);

	// ignore sound if enemy drop a weapon on death
	if (!entity && ((eType & SOUND_TYPE_ITEM_HIDING) == SOUND_TYPE_ITEM_HIDING))
		return;

	if (entity && (!EnemyMan.is_enemy(entity)))
	{
		SoundMemory.check_help_sound(eType, entity->ai_location().level_vertex_id());
		return;
	}

	if ((eType & SOUND_TYPE_WEAPON_SHOOTING) == SOUND_TYPE_WEAPON_SHOOTING)
		power = 1.f;

	if (((eType & SOUND_TYPE_WEAPON_BULLET_HIT) == SOUND_TYPE_WEAPON_BULLET_HIT) && (dist < 2.f))
		HitMemory.add_hit(who, eSideFront);

	// execute callback
	sound_callback(who, eType, Position, power);

	// register in sound memory
	if (power >= db().m_fSoundThreshold)
	{
		SoundMemory.HearSound(who, eType, Position, power, Device.dwTimeGlobal);
	}
}

void CBaseMonster::HitEntity(const CEntity *pEntity, float fDamage, float impulse, Fvector &dir)
{
	if (!g_Alive())
		return;
	if (!pEntity || pEntity->getDestroy())
		return;

	if (!EnemyMan.get_enemy())
		return;

	if (EnemyMan.get_enemy() != pEntity)
		return;

	Fvector position_in_bone_space;
	position_in_bone_space.set(0.f, 0.f, 0.f);

	// ������� �� ��������� ��������� � ������� ������� ����������� ��������
	Fvector hit_dir;
	XFORM().transform_dir(hit_dir, dir);
	hit_dir.normalize();

	CEntity* pEntityNC = const_cast<CEntity*>(pEntity);
	VERIFY(pEntityNC);

	NET_Packet l_P;
	SHit HS;
	HS.GenHeader(GE_HIT, pEntityNC->ID());
	HS.whoID = (ID());
	HS.weaponID = (ID());
	HS.dir = (hit_dir);
	HS.power = (fDamage);
	HS.boneID = (smart_cast<IKinematics*>(pEntityNC->Visual())->LL_GetBoneRoot());
	HS.p_in_bone_space = (position_in_bone_space);
	HS.impulse = (impulse);
	HS.hit_type = (ALife::eHitTypeWound);
	HS.Write_Packet(l_P);
	u_EventSend(l_P);

	Morale.on_attack_success();
	m_time_last_attack_success = Device.dwTimeGlobal;
}

BOOL CBaseMonster::feel_vision_isRelevant(CObject *O)
{
	if (!g_Alive())
		return FALSE;
	if (0 == smart_cast<CEntity *>(O))
		return FALSE;

	if ((O->spatial.type & STYPE_VISIBLEFORAI) != STYPE_VISIBLEFORAI)
		return FALSE;

	// ���� ����, �� ������ �� �����
	if (m_bSleep)
		return FALSE;

	// ���� �� ���� - �� �����
	CEntityAlive *entity = smart_cast<CEntityAlive *>(O);
	if (entity && entity->g_Alive())
	{
		if (!EnemyMan.is_enemy(entity))
		{
			// ���� ����� ����� - ��������� ������� � ���� ������
			CBaseMonster *monster = smart_cast<CBaseMonster *>(entity);
			if (monster && !m_skip_transfer_enemy)
				EnemyMan.transfer_enemy(monster);
			return FALSE;
		}
	}

	return TRUE;
}

void CBaseMonster::HitSignal(float amount, Fvector &vLocalDir, CObject *who, s16 element)
{
	if (!g_Alive())
		return;

	feel_sound_new(who, SOUND_TYPE_WEAPON_SHOOTING, 0, who->Position(), 1.f);

	if (g_Alive())
		set_state_sound(MonsterSound::eMonsterSoundTakeDamage);

	if (element < 0)
		return;

	// ���������� ����������� ���� (����� || ��� || ���� || �����)
	float yaw, pitch;
	vLocalDir.getHP(yaw, pitch);

	yaw = angle_normalize(yaw);

	EHitSide hit_side = eSideFront;
	if ((yaw >= PI_DIV_4) && (yaw <= 3 * PI_DIV_4))
		hit_side = eSideLeft;
	else if ((yaw >= 3 * PI_DIV_4) && (yaw <= 5 * PI_DIV_4))
		hit_side = eSideBack;
	else if ((yaw >= 5 * PI_DIV_4) && (yaw <= 7 * PI_DIV_4))
		hit_side = eSideRight;

	anim().FX_Play(hit_side, 1.0f);

	HitMemory.add_hit(who, hit_side);

	Morale.on_hit();

	callback(GameObject::eHit)(
		lua_game_object(),
		amount,
		vLocalDir,
		smart_cast<const CGameObject *>(who)->lua_game_object(),
		element);

	// ���� ������� - �������� ��� �����
	CEntityAlive *obj = smart_cast<CEntityAlive *>(who);
	if (obj && (tfGetRelationType(obj) == ALife::eRelationTypeNeutral))
		EnemyMan.add_enemy(obj);
}

void CBaseMonster::SetAttackEffector()
{
	CActor *pA = smart_cast<CActor *>(Level().CurrentEntity());
	if (pA)
	{
		Actor()->Cameras().AddCamEffector(xr_new<CMonsterEffectorHit>(db().m_attack_effector.ce_time, db().m_attack_effector.ce_amplitude, db().m_attack_effector.ce_period_number, db().m_attack_effector.ce_power));
		Actor()->Cameras().AddPPEffector(xr_new<CMonsterEffector>(db().m_attack_effector.ppi, db().m_attack_effector.time, db().m_attack_effector.time_attack, db().m_attack_effector.time_release));
	}
}

void CBaseMonster::Hit_Psy(CObject *object, float value)
{
	NET_Packet P;
	SHit HS;
	HS.GenHeader(GE_HIT, object->ID());					 //					//	u_EventGen		(P,GE_HIT, object->ID());				//
	HS.whoID = (ID());									 // own		//	P.w_u16			(ID());									// own
	HS.weaponID = (ID());								 // own		//	P.w_u16			(ID());									// own
	HS.dir = (Fvector().set(0.f, 1.f, 0.f));			 // direction	//	P.w_dir			(Fvector().set(0.f,1.f,0.f));			// direction
	HS.power = (value);									 // hit value	//	P.w_float		(value);								// hit value
	HS.boneID = (BI_NONE);								 // bone		//	P.w_s16			(BI_NONE);								// bone
	HS.p_in_bone_space = (Fvector().set(0.f, 0.f, 0.f)); //	P.w_vec3		(Fvector().set(0.f,0.f,0.f));
	HS.impulse = (0.f);									 //	P.w_float		(0.f);
	HS.hit_type = (ALife::eHitTypeTelepatic);			 //	P.w_u16			(u16(ALife::eHitTypeTelepatic));
	HS.Write_Packet(P);
	u_EventSend(P);
}

void CBaseMonster::Hit_Wound(const CObject *object, float value, const Fvector &dir, float impulse)
{
	NET_Packet P;
	SHit HS;
	HS.GenHeader(GE_HIT, object->ID());
	HS.whoID = (ID());
	HS.weaponID = (ID());
	HS.dir = (dir);
	HS.power = (value);
	HS.boneID = (smart_cast<const IKinematics*>(object->cVisual())->LL_GetBoneRoot());
	HS.p_in_bone_space = (Fvector().set(0.f, 0.f, 0.f));
	HS.impulse = (impulse);
	HS.hit_type = (ALife::eHitTypeWound);
	HS.Write_Packet(P);
	u_EventSend(P);
}

bool CBaseMonster::critical_wound_external_conditions_suitable()
{
	if (!control().check_start_conditions(ControlCom::eControlSequencer))
		return false;

	if (!anim().IsStandCurAnim())
		return false;

	return true;
}

void CBaseMonster::critical_wounded_state_start()
{
	VERIFY(m_critical_wound_type != u32(-1));

	LPCSTR anim = 0;
	switch (m_critical_wound_type)
	{
	case critical_wound_type_head:
		anim = m_critical_wound_anim_head;
		break;
	case critical_wound_type_torso:
		anim = m_critical_wound_anim_torso;
		break;
	case critical_wound_type_legs:
		anim = m_critical_wound_anim_legs;
		break;
	}

	VERIFY(anim);
	com_man().critical_wound(anim);
}
