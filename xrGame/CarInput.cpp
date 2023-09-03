#include "stdafx.h"
#pragma hdrstop

#ifdef DEBUG
#include "ode_include.h"
#include "StatGraph.h"
#include "PHDebug.h"
#endif

#include "alife_space.h"
#include "hit.h"
#include "phdestroyable.h"
#include "car.h"
#include "actor.h"
#include "cameralook.h"
#include "camerafirsteye.h"
#include "script_entity_action.h"
#include "xr_level_controller.h"
#include "..\include\xrRender\Kinematics.h"
#include "level.h"
#include "CarWeapon.h"
#include "Console.h"

void CCar::OnMouseMove(int dx, int dy)
{
	if (!IsMyCar() && !GetScriptControl())
		return;

	CCameraBase *C = active_camera;
	float scale = (C->f_fov / g_fov) * psMouseSens * psMouseSensScale / 50.f;
	
	if (dx)
	{
		float d = float(dx) * scale;
		C->Move((d < 0) ? kLEFT : kRIGHT, _abs(d));
	}

	if (dy)
	{
		float d = ((psMouseInvert.test(1)) ? -1 : 1) * float(dy) * scale * 3.f / 4.f;
		C->Move((d > 0) ? kUP : kDOWN, _abs(d));
	}
}

bool CCar::bfAssignMovement(CScriptEntityAction *tpEntityAction)
{
	if (tpEntityAction->m_tMovementAction.m_bCompleted)
		return (false);

	u32 l_tInput = tpEntityAction->m_tMovementAction.m_tInputKeys;

	vfProcessInputKey(kFWD, !!(l_tInput & CScriptMovementAction::eInputKeyForward));
	vfProcessInputKey(kBACK, !!(l_tInput & CScriptMovementAction::eInputKeyBack));
	vfProcessInputKey(kL_STRAFE, !!(l_tInput & CScriptMovementAction::eInputKeyLeft));
	vfProcessInputKey(kR_STRAFE, !!(l_tInput & CScriptMovementAction::eInputKeyRight));
	vfProcessInputKey(kACCEL, !!(l_tInput & CScriptMovementAction::eInputKeyShiftUp));
	vfProcessInputKey(kCROUCH, !!(l_tInput & CScriptMovementAction::eInputKeyShiftDown));
	vfProcessInputKey(kJUMP, !!(l_tInput & CScriptMovementAction::eInputKeyBreaks));

	if (!!(l_tInput & CScriptMovementAction::eInputKeyEngineOn))
		StartEngine();

	if (!!(l_tInput & CScriptMovementAction::eInputKeyEngineOff))
		StopEngine();

	return (true);
}

bool CCar::bfAssignObject(CScriptEntityAction *tpEntityAction)
{
	CScriptObjectAction &l_tObjectAction = tpEntityAction->m_tObjectAction;
	if (l_tObjectAction.m_bCompleted || !xr_strlen(l_tObjectAction.m_caBoneName))
		return ((l_tObjectAction.m_bCompleted = true) == false);

	s16 l_sBoneID = smart_cast<IKinematics *>(Visual())->LL_BoneID(l_tObjectAction.m_caBoneName);
	if (is_Door(l_sBoneID))
	{
		switch (l_tObjectAction.m_tGoalType)
		{
		case MonsterSpace::eObjectActionActivate:
		{
			if (!DoorOpen(l_sBoneID))
				return ((l_tObjectAction.m_bCompleted = true) == false);
			break;
		}
		case MonsterSpace::eObjectActionDeactivate:
		{
			if (!DoorClose(l_sBoneID))
				return ((l_tObjectAction.m_bCompleted = true) == false);
			break;
		}
		case MonsterSpace::eObjectActionUse:
		{
			if (!DoorSwitch(l_sBoneID))
				return ((l_tObjectAction.m_bCompleted = true) == false);
			break;
		}
		default:
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		return (false);
	}

	SCarLight *light = NULL;
	if (m_lights.findLight(l_sBoneID, light))
	{
		switch (l_tObjectAction.m_tGoalType)
		{
		case MonsterSpace::eObjectActionActivate:
		{
			light->TurnOn();
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		case MonsterSpace::eObjectActionDeactivate:
		{
			light->TurnOff();
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		case MonsterSpace::eObjectActionUse:
		{
			light->Switch();
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		default:
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
	}

	if (t_lights.findTailLight(l_sBoneID, light))
	{
		switch (l_tObjectAction.m_tGoalType)
		{
		case MonsterSpace::eObjectActionActivate:
		{
			light->TurnOn();
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		case MonsterSpace::eObjectActionDeactivate:
		{
			light->TurnOff();
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		case MonsterSpace::eObjectActionUse:
		{
			light->Switch();
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		default:
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
	}

	if (e_lights.findExhaustLight(l_sBoneID, light))
	{
		switch (l_tObjectAction.m_tGoalType)
		{
		case MonsterSpace::eObjectActionActivate:
		{
			light->TurnOn();
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		case MonsterSpace::eObjectActionDeactivate:
		{
			light->TurnOff();
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		case MonsterSpace::eObjectActionUse:
		{
			light->Switch();
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		default:
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
	}

	if (n_lights.findNitroLight(l_sBoneID, light))
	{
		switch (l_tObjectAction.m_tGoalType)
		{
		case MonsterSpace::eObjectActionActivate:
		{
			light->TurnOn();
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		case MonsterSpace::eObjectActionDeactivate:
		{
			light->TurnOff();
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		case MonsterSpace::eObjectActionUse:
		{
			light->Switch();
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
		default:
			return ((l_tObjectAction.m_bCompleted = true) == false);
		}
	}


	return (false);
}

void CCar::vfProcessInputKey(int iCommand, bool bPressed)
{
	if (bPressed)
		OnKeyboardPress(iCommand);
	else
		OnKeyboardRelease(iCommand);
}

void CCar::OnKeyboardPress(int cmd)
{
	if (!IsMyCar() && !GetScriptControl())
		return;

	switch (cmd)
	{
	case kCAM_1:
		OnCameraChange(ectFirst);
		break;
	case kCAM_2:
		OnCameraChange(ectChase);
		break;
	case kCAM_3:
		OnCameraChange(ectFree);
		break;
	case kACCEL:
		TransmissionUp();
		break;
	case kCROUCH:
		TransmissionDown();
		break;
	case kFWD:
		PressForward();
		t_lights.TurnOffTailLights();
		break;
	case kBACK:
		PressBack();
		if (m_current_transmission_num > 1) {
			NET_Packet PBACKBRAKES;
			CGameObject::u_EventGen(PBACKBRAKES, GE_CAR_BRAKES, ID());
			CGameObject::u_EventSend(PBACKBRAKES);
			PlayTireSmoke();
			NET_Packet PBACKTSON;
			CGameObject::u_EventGen(PBACKTSON, GE_CAR_TIRESMOKE_ON, ID());
			CGameObject::u_EventSend(PBACKTSON);
			b_tire_smoke_active = true;
			b_brakes_activated = true;
		}

		if (m_lights.IsLightTurnedOn()) {
			t_lights.TurnOnTailLights();
			NET_Packet PBACKON;
			CGameObject::u_EventGen(PBACKON, GE_CAR_TAIL_ON, ID());
			CGameObject::u_EventSend(PBACKON);
		}
		break;
	case kCarBeep:
	{
		NET_Packet PBEEP;
		CGameObject::u_EventGen(PBEEP, GE_CAR_BEEP, ID());
		CGameObject::u_EventSend(PBEEP);
		break;
	}
	case kR_STRAFE:
		PressRight();
		if (OwnerActor())
			OwnerActor()->steer_Vehicle(1);
		break;
	case kL_STRAFE:
		PressLeft();
		if (OwnerActor())
			OwnerActor()->steer_Vehicle(-1);
		break;
	case kJUMP: 
		PressBreaks();
		NET_Packet PJUMPBRAKES;
		CGameObject::u_EventGen(PJUMPBRAKES, GE_CAR_BRAKES, ID());
		CGameObject::u_EventSend(PJUMPBRAKES);

		if (!b_tire_smoke_active && !b_brakes_activated && b_engine_on && m_current_rpm > m_min_rpm * 1.03) {
			PlayTireSmoke();
			NET_Packet PJUMPTSON;
			CGameObject::u_EventGen(PJUMPTSON, GE_CAR_TIRESMOKE_ON, ID());
			CGameObject::u_EventSend(PJUMPTSON);
			b_tire_smoke_active = true;
			b_brakes_activated = true;
		}

		if (m_lights.IsLightTurnedOn()) {
			t_lights.TurnOnTailLights();
			NET_Packet PJUMPON;
			CGameObject::u_EventGen(PJUMPON, GE_CAR_TAIL_ON, ID());
			CGameObject::u_EventSend(PJUMPON);
		}
		break;
	case kENGINE:
		SwitchEngine();
		break;
	case kTORCH:
		m_lights.SwitchHeadLights();
		break;
	case kUSE:
		break;
	case kNITRO:
		if (b_engine_on && m_nitro_capacity > 0) {
			Console->Execute("fov 85");
			n_lights.TurnOnNitroLights();
		}
		NET_Packet PNITROON;
		CGameObject::u_EventGen(PNITROON, GE_CAR_NITRO_ON, ID());
		CGameObject::u_EventSend(PNITROON);
		break;
	};
}

void CCar::OnKeyboardRelease(int cmd)
{
	if (!IsMyCar() && !GetScriptControl())
		return;

	switch (cmd)
	{
	case kACCEL:
		break;
	case kFWD:
		ReleaseForward();
		break;
	case kBACK:
		ReleaseBack();
		t_lights.TurnOffTailLights();
		NET_Packet PBACKOFF;
		CGameObject::u_EventGen(PBACKOFF, GE_CAR_TAIL_OFF, ID());
		CGameObject::u_EventSend(PBACKOFF);

		b_tire_smoke_active = false;
		b_brakes_activated = false;

		StopTireSmoke();
		NET_Packet PBACKTSOFF;
		CGameObject::u_EventGen(PBACKTSOFF, GE_CAR_TIRESMOKE_OFF, ID());
		CGameObject::u_EventSend(PBACKTSOFF);
		break;
	case kL_STRAFE:
		ReleaseLeft();
		if (OwnerActor())
			OwnerActor()->steer_Vehicle(0);
		break;
	case kR_STRAFE:
		ReleaseRight();
		if (OwnerActor())
			OwnerActor()->steer_Vehicle(0);
		break;
	case kJUMP:
		ReleaseBreaks();
		t_lights.TurnOffTailLights();
		NET_Packet PJUMPOFF;
		CGameObject::u_EventGen(PJUMPOFF, GE_CAR_TAIL_OFF, ID());
		CGameObject::u_EventSend(PJUMPOFF);

		b_tire_smoke_active = false;
		b_brakes_activated = false;

		StopTireSmoke();
		NET_Packet PJUMPTSOFF;
		CGameObject::u_EventGen(PJUMPTSOFF, GE_CAR_TIRESMOKE_OFF, ID());
		CGameObject::u_EventSend(PJUMPTSOFF);
		break;
	case kNITRO:
		Console->Execute("fov 70");
		n_lights.TurnOffNitroLights();
		NET_Packet PNITROOFF;
		CGameObject::u_EventGen(PNITROOFF, GE_CAR_NITRO_OFF, ID());
		CGameObject::u_EventSend(PNITROOFF);
		break;
	};
}

void CCar::OnKeyboardHold(int cmd)
{
	if (!IsMyCar())
		return;

	switch (cmd)
	{
	case kCAM_ZOOM_IN:
	case kCAM_ZOOM_OUT:
	case kUP:
	case kDOWN:
	case kLEFT:
	case kRIGHT:
		active_camera->Move(cmd);
		break;
	}
}
void CCar::Action(int id, u32 flags)
{
	if (m_car_weapon)
		m_car_weapon->Action(id, flags);
}

void CCar::SetParam(int id, Fvector2 val)
{
	if (m_car_weapon)
		m_car_weapon->SetParam(id, val);
}

void CCar::SetParam(int id, Fvector val)
{
	if (m_car_weapon)
		m_car_weapon->SetParam(id, val);
}

bool CCar::WpnCanHit()
{
	if (m_car_weapon)
		return m_car_weapon->AllowFire();
	return false;
}

float CCar::FireDirDiff()
{
	if (m_car_weapon)
		return m_car_weapon->FireDirDiff();
	return 0.0f;
}
#include "script_game_object.h"
#include "car_memory.h"
#include "visual_memory_manager.h"

bool CCar::isObjectVisible(CScriptGameObject *O_)
{
	if (m_memory)
	{
		return m_memory->visual().visible_now(&O_->object());
	}
	else
	{

		if (!O_)
		{
			Msg("Attempt to call CCar::isObjectVisible method wihth passed NULL parameter");
			return false;
		}
		CObject *O = &O_->object();
		Fvector dir_to_object;
		Fvector to_point;
		O->Center(to_point);

		Fvector from_point;
		Center(from_point);

		if (HasWeapon())
		{
			from_point.y = XFORM().c.y + m_car_weapon->_height();
		}

		dir_to_object.sub(to_point, from_point).normalize_safe();
		float ray_length = from_point.distance_to(to_point);

		BOOL res = Level().ObjectSpace.RayTest(from_point, dir_to_object, ray_length, collide::rqtStatic, NULL, NULL);
		return (0 == res);
	}
}

bool CCar::HasWeapon()
{
	return (m_car_weapon != NULL);
}

Fvector CCar::CurrentVel()
{
	Fvector lin_vel;
	m_pPhysicsShell->get_LinearVel(lin_vel);

	return lin_vel;
}
