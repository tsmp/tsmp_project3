#include "stdafx.h"
#include "controller_psy_hit.h"
#include "../BaseMonster/base_monster.h"
#include "controller.h"
#include "../control_animation_base.h"
#include "../control_direction_base.h"
#include "../control_movement_base.h"
#include "../../../level.h"
#include "../../../actor.h"
#include "../../../ActorEffector.h"
#include "CameraBase.h"
#include "../../../CharacterPhysicsSupport.h"
#include "../../../level_debug.h"
#include "../../../clsid_game.h"
#include "..\include\xrRender\animation_motion.h"

bool IsActor(const CEntityAlive* entity)
{
	return entity->CLS_ID == CLSID_OBJECT_ACTOR;
}

const CEntityAlive* CControllerPsyHit::GetEnemy()
{
	return m_object->EnemyMan.get_enemy();
}

void CControllerPsyHit::SendEventMP(u16 idTo, PsyHitMpSyncStages stage)
{
	u8 ustage = static_cast<u8>(stage);

	NET_Packet packet;
	CGameObject::u_EventGen(packet, GE_CONTROLLER_PSY_FIRE, m_object->ID());
	packet.w_u16(idTo);
	packet.w_u8(ustage);
	Level().Server->SendBroadcast(BroadcastCID, packet, net_flags(TRUE, TRUE));
}

void CControllerPsyHit::load(LPCSTR section)
{
	m_min_tube_dist = pSettings->r_float(section, "tube_min_dist");
}

void CControllerPsyHit::reinit()
{
	inherited::reinit();

	IKinematicsAnimated *skel = smart_cast<IKinematicsAnimated *>(m_object->Visual());
	m_stage[0] = skel->ID_Cycle_Safe("psy_attack_0");
	VERIFY(m_stage[0]);
	m_stage[1] = skel->ID_Cycle_Safe("psy_attack_1");
	VERIFY(m_stage[1]);
	m_stage[2] = skel->ID_Cycle_Safe("psy_attack_2");
	VERIFY(m_stage[2]);
	m_stage[3] = skel->ID_Cycle_Safe("psy_attack_3");
	VERIFY(m_stage[3]);
	m_current_index = 0;

	m_sound_state = eNone;
}

bool CControllerPsyHit::check_start_conditions()
{
	if (is_active())
		return false;
	if (m_man->is_captured_pure())
		return false;

	const CEntityAlive* enemy = GetEnemy();

	if (!enemy || !enemy->g_Alive() || !m_object->EnemyMan.see_enemy_now())
		return false;

	return true;
}

void CControllerPsyHit::activate()
{
	m_man->capture_pure(this);
	m_man->subscribe(this, ControlCom::eventAnimationEnd);

	m_man->path_stop(this);
	m_man->move_stop(this);

	const CEntityAlive* enemy = GetEnemy();
	
	if (!enemy)
	{
		enemyId = static_cast<u16>(-1);
		enemyIsActor = false;
		return;
	}		

	enemyId = enemy->ID();
	enemyIsActor = IsActor(enemy);

	// set direction
	SControlDirectionData *ctrl_dir = (SControlDirectionData *)m_man->data(this, ControlCom::eControlDir);
	VERIFY(ctrl_dir);
	ctrl_dir->heading.target_speed = 3.f;
	ctrl_dir->heading.target_angle = m_man->direction().angle_to_target(enemy->Position());

	m_current_index = 0;
	play_anim();

	m_blocked = false;

	if (!enemyIsActor)
		return;

	if (IsGameTypeSingle())
		set_sound_state(ePrepare);
	else
		SendEventMP(enemy->ID(), PsyHitMpSyncStages::Prepare);
}

void CControllerPsyHit::deactivate()
{
	m_man->release_pure(this);
	m_man->unsubscribe(this, ControlCom::eventAnimationEnd);

	if (!m_blocked)
		return;

	if (!enemyIsActor)
		return;

	if (IsGameTypeSingle())
	{
		NET_Packet P;
		CGameObject::u_EventGen(P, GEG_PLAYER_WEAPON_HIDE_STATE, Actor()->ID());
		P.w_u32(INV_STATE_BLOCK_ALL);
		P.w_u8(u8(false));
		CGameObject::u_EventSend(P);

		set_sound_state(eNone);
	}
	else
		SendEventMP(enemyId, PsyHitMpSyncStages::Deactivate);
}

void CControllerPsyHit::on_event(ControlCom::EEventType type, ControlCom::IEventData *data)
{
	if (type == ControlCom::eventAnimationEnd)
	{
		if (m_current_index < 3)
		{
			m_current_index++;
			play_anim();

			switch (m_current_index)
			{
			case 1:
				death_glide_start();
				break;
			case 2:
				hit();
				break;
			case 3:
				death_glide_end();
				break;
			}
		}
		else
		{
			m_man->deactivate(this);
			return;
		}
	}
}

void CControllerPsyHit::play_anim()
{
	SControlAnimationData *ctrl_anim = (SControlAnimationData *)m_man->data(this, ControlCom::eControlAnimation);
	VERIFY(ctrl_anim);

	ctrl_anim->global.motion = m_stage[m_current_index];
	ctrl_anim->global.actual = false;
}

bool CControllerPsyHit::check_conditions_final()
{
	if (!m_object->g_Alive())
		return false;

	const CEntityAlive* enemy = GetEnemy();

	if (!enemy || !enemy->g_Alive())
		return false;

	if (!m_object->EnemyMan.see_enemy_now())
		return false;

	return true;
}

void CControllerPsyHit::death_glide_start()
{
	if (OnServer())
	{
		if (!check_conditions_final())
		{
			m_man->deactivate(this);
			return;
		}

		if (enemyIsActor)
		{
			if (IsGameTypeSingle())
			{
				NET_Packet P;
				CGameObject::u_EventGen(P, GEG_PLAYER_WEAPON_HIDE_STATE, enemyId);
				P.w_u32(INV_STATE_BLOCK_ALL);
				P.w_u8(u8(true));
				CGameObject::u_EventSend(P);
			}
			else
				SendEventMP(enemyId, PsyHitMpSyncStages::GlideStart);
		}

		m_blocked = true;
	}

	if (OnClient() || (IsGameTypeSingle() && enemyIsActor))
	{
		// Start effector
		CEffectorCam *ce = Actor()->Cameras().GetCamEffector(eCEControllerPsyHit);
		VERIFY(!ce);

		Fvector src_pos = Actor()->cam_Active()->vPosition;
		Fvector target_pos = m_object->Position();
		target_pos.y += 1.2f;

		Fvector dir;
		dir.sub(target_pos, src_pos);

		float dist = dir.magnitude();
		dir.normalize();

		target_pos.mad(src_pos, dir, dist - 4.8f);

		Actor()->Cameras().AddCamEffector(xr_new<CControllerPsyHitCamEffector>(eCEControllerPsyHit, src_pos, target_pos, m_man->animation().motion_time(m_stage[1], m_object->Visual())));
		smart_cast<CController*>(m_object)->draw_fire_particles();

		dir.sub(src_pos, target_pos);
		dir.normalize();
		float h, p;
		dir.getHP(h, p);
		dir.setHP(h, p + PI_DIV_3);
		Actor()->character_physics_support()->movement()->ApplyImpulse(dir, Actor()->GetMass() * 530.f);

		set_sound_state(eStart);
	}

	if (!OnServer())
		return;

	if (const CEntityAlive* enemy = GetEnemy())
	{
		// set direction
		SControlDirectionData* ctrl_dir = (SControlDirectionData*)m_man->data(this, ControlCom::eControlDir);
		VERIFY(ctrl_dir);
		ctrl_dir->heading.target_speed = 3.f;
		ctrl_dir->heading.target_angle = m_man->direction().angle_to_target(enemy->Position());
	}
}

void CControllerPsyHit::death_glide_end()
{
	stop();
	CController* monster = smart_cast<CController*>(m_object);

	if (OnClient() || IsGameTypeSingle())
	{
		monster->draw_fire_particles();

		monster->m_sound_tube_hit_left.play_at_pos(Actor(), Fvector().set(-1.f, 0.f, 1.f), sm_2D);
		monster->m_sound_tube_hit_right.play_at_pos(Actor(), Fvector().set(1.f, 0.f, 1.f), sm_2D);
	}

	if (!OnServer())
		return;

	if (const CEntityAlive* enemy = GetEnemy())
		m_object->Hit_Wound(enemy, monster->m_tube_damage, Fvector().set(0.0f, 1.0f, 0.0f), 0.0f);
}

void CControllerPsyHit::update_frame()
{
}

void CControllerPsyHit::set_sound_state(ESoundState state)
{
	CController *monster = smart_cast<CController *>(m_object);
	if (state == ePrepare)
	{
		monster->m_sound_tube_prepare.play_at_pos(Actor(), Fvector().set(0.f, 0.f, 0.f), sm_2D);
	}
	else if (state == eStart)
	{
		if (monster->m_sound_tube_prepare._feedback())
			monster->m_sound_tube_prepare.stop();

		monster->m_sound_tube_start.play_at_pos(Actor(), Fvector().set(0.f, 0.f, 0.f), sm_2D);
		monster->m_sound_tube_pull.play_at_pos(Actor(), Fvector().set(0.f, 0.f, 0.f), sm_2D);
	}
	else if (state == eHit)
	{
		if (monster->m_sound_tube_start._feedback())
			monster->m_sound_tube_start.stop();
		if (monster->m_sound_tube_pull._feedback())
			monster->m_sound_tube_pull.stop();

		//monster->m_sound_tube_hit_left.play_at_pos(Actor(), Fvector().set(-1.f, 0.f, 1.f), sm_2D);
		//monster->m_sound_tube_hit_right.play_at_pos(Actor(), Fvector().set(1.f, 0.f, 1.f), sm_2D);
	}
	else if (state == eNone)
	{
		if (monster->m_sound_tube_start._feedback())
			monster->m_sound_tube_start.stop();
		if (monster->m_sound_tube_pull._feedback())
			monster->m_sound_tube_pull.stop();
		if (monster->m_sound_tube_prepare._feedback())
			monster->m_sound_tube_prepare.stop();
	}

	m_sound_state = state;
}

void CControllerPsyHit::hit()
{
	if (!enemyIsActor)
		return;

	if (IsGameTypeSingle())
		set_sound_state(eHit);
	else
		SendEventMP(enemyId, PsyHitMpSyncStages::Hit);
}

void CControllerPsyHit::stop()
{
	if (OnClient() || IsGameTypeSingle())
	{
		// Stop camera effector
		if (CEffectorCam* ce = Actor()->Cameras().GetCamEffector(eCEControllerPsyHit))
			Actor()->Cameras().RemoveCamEffector(eCEControllerPsyHit);
	}

	if (!IsGameTypeSingle() && OnServer() && enemyIsActor)
		SendEventMP(enemyId, PsyHitMpSyncStages::Stop);
}

void CControllerPsyHit::on_death()
{
	if (!is_active())
		return;

	stop();
	m_man->deactivate(this);
}
