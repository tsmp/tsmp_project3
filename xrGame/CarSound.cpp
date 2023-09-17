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

extern CPHWorld *ph_world;
float state_N_start;
float state_N_end;
float state_1_start;
float state_1_end;
float state_2_start;
float state_2_end;
float state_3_start;
float state_3_end;
float state_4_start;
float state_4_end;
float state_5_start;
float state_5_end;

CCar::SCarSound::SCarSound(CCar *car)
{
	volume = 1.f;
	pcar = car;
	relative_pos.set(0.f, 0.5f, -1.f);
}

CCar::SCarSound::~SCarSound()
{
}

void CCar::SCarSound::Init()
{
	CInifile *ini = smart_cast<IKinematics *>(pcar->Visual())->LL_UserData();

	if (ini->section_exist("car_sound") && ini->line_exist("car_sound", "snd_volume"))
	{
		volume = ini->r_float("car_sound", "snd_volume");

		snd_engine.create(ini->r_string("car_sound", "snd_name"), st_Effect, sg_SourceType);
		snd_engine_1.create(READ_IF_EXISTS(ini, r_string, "car_sound", "engine_1", "car\\skyline\\skyline_run"), st_Effect, sg_SourceType);
		snd_engine_2.create(READ_IF_EXISTS(ini, r_string, "car_sound", "engine_2", "car\\skyline\\skyline_run"), st_Effect, sg_SourceType);
		snd_engine_3.create(READ_IF_EXISTS(ini, r_string, "car_sound", "engine_3", "car\\skyline\\skyline_run"), st_Effect, sg_SourceType);
		snd_engine_4.create(READ_IF_EXISTS(ini, r_string, "car_sound", "engine_4", "car\\skyline\\skyline_run"), st_Effect, sg_SourceType);
		snd_engine_5.create(READ_IF_EXISTS(ini, r_string, "car_sound", "engine_5", "car\\skyline\\skyline_run"), st_Effect, sg_SourceType);
		snd_skid_1.create(READ_IF_EXISTS(ini, r_string, "car_sound", "skid_1", "car\\car_skid_1"), st_Effect, sg_SourceType);
		snd_skid_2.create(READ_IF_EXISTS(ini, r_string, "car_sound", "skid_2", "car\\car_skid_2"), st_Effect, sg_SourceType);
		snd_skid_3.create(READ_IF_EXISTS(ini, r_string, "car_sound", "skid_3", "car\\car_skid_3"), st_Effect, sg_SourceType);
		snd_engine_idle.create(READ_IF_EXISTS(ini, r_string, "car_sound", "engine_idle", "car\\car_idle"), st_Effect, sg_SourceType);
		snd_engine_start.create(READ_IF_EXISTS(ini, r_string, "car_sound", "engine_start", "car\\test_car_start"), st_Effect, sg_SourceType);
		snd_engine_stop.create(READ_IF_EXISTS(ini, r_string, "car_sound", "engine_stop", "car\\test_car_stop"), st_Effect, sg_SourceType);
		snd_beep.create(READ_IF_EXISTS(ini, r_string, "car_sound", "beep", "car\\klakson"), st_Effect, sg_SourceType);
		snd_brakes.create(READ_IF_EXISTS(ini, r_string, "car_sound", "car_brakes", "ambient\\silence"), st_Effect, sg_SourceType);
		snd_nitro_start.create(READ_IF_EXISTS(ini, r_string, "car_sound", "nitro_start", "car\\nitro_start"), st_Effect, sg_SourceType);
		snd_nitro_loop.create(READ_IF_EXISTS(ini, r_string, "car_sound", "nitro_loop", "car\\nitro_loop"), st_Effect, sg_SourceType);
		snd_nitro_end.create(READ_IF_EXISTS(ini, r_string, "car_sound", "nitro_end", "car\\nitro_end"), st_Effect, sg_SourceType);

		float fengine_start_delay = READ_IF_EXISTS(ini, r_float, "car_sound", "engine_sound_start_dellay", 0.25f);
		engine_start_delay = iFloor((snd_engine_start._handle() ? snd_engine_start._handle()->length_ms() : 1.f) * fengine_start_delay);
		
		if (ini->line_exist("car_sound", "relative_pos"))		
			relative_pos.set(ini->r_fvector3("car_sound", "relative_pos"));
		
		if (ini->line_exist("car_sound", "transmission_switch"))		
			snd_transmission.create(ini->r_string("car_sound", "transmission_switch"), st_Effect, sg_SourceType);		
	}
	else	
		Msg("! Car doesn't contain sound params");
	
	eCarSound = sndOff;

	state_N_start = pcar->m_min_rpm;
	state_N_end = (pcar->m_max_rpm - pcar->m_min_rpm) * 0.1 + pcar->m_min_rpm;
	state_1_start = (pcar->m_max_rpm - pcar->m_min_rpm) * 0.1 + pcar->m_min_rpm;
	state_1_end = (pcar->m_max_rpm - pcar->m_min_rpm) * 0.28 + pcar->m_min_rpm;
	state_2_start = (pcar->m_max_rpm - pcar->m_min_rpm) * 0.28 + pcar->m_min_rpm;
	state_2_end = (pcar->m_max_rpm - pcar->m_min_rpm) * 0.46 + pcar->m_min_rpm;
	state_3_start = (pcar->m_max_rpm - pcar->m_min_rpm) * 0.46 + pcar->m_min_rpm;
	state_3_end = (pcar->m_max_rpm - pcar->m_min_rpm) * 0.64 + pcar->m_min_rpm;
	state_4_start = (pcar->m_max_rpm - pcar->m_min_rpm) * 0.64 + pcar->m_min_rpm;
	state_4_end = (pcar->m_max_rpm - pcar->m_min_rpm) * 0.82 + pcar->m_min_rpm;
	state_5_start = (pcar->m_max_rpm - pcar->m_min_rpm) * 0.82 + pcar->m_min_rpm;
	state_5_end = pcar->m_max_rpm;
}

void CCar::SCarSound::SetSoundPosition(ref_sound &snd)
{
	VERIFY(!ph_world->Processing());
	if (snd._feedback())
	{
		Fvector pos;
		pcar->XFORM().transform_tiny(pos, relative_pos);
		snd.set_position(pos);
	}
}

void CCar::SCarSound::UpdateStarting()
{
	VERIFY(!ph_world->Processing());
	SetSoundPosition(snd_engine_start);

	if (snd_engine._feedback())	
		UpdateDrive();	
	else
	{
		if (time_state_start + engine_start_delay < Device.dwTimeGlobal)
		{
			snd_engine_idle.play(pcar, sm_Looped);
			snd_engine_1.play(pcar, sm_Looped);
			snd_engine_1.set_volume(0);
			snd_engine_2.play(pcar, sm_Looped);
			snd_engine_2.set_volume(0);
			snd_engine_3.play(pcar, sm_Looped);
			snd_engine_3.set_volume(0);
			snd_engine_4.play(pcar, sm_Looped);
			snd_engine_4.set_volume(0);
			snd_engine_5.play(pcar, sm_Looped);
			snd_engine_5.set_volume(0);
			snd_nitro_loop.play(pcar, sm_Looped);
			snd_nitro_loop.set_volume(0);
			snd_skid_1.play(pcar, sm_Looped);
			snd_skid_1.set_volume(0);
			snd_skid_2.play(pcar, sm_Looped);
			snd_skid_2.set_volume(0);
			snd_skid_3.play(pcar, sm_Looped);
			snd_skid_3.set_volume(0);
			UpdateDrive();
		}
	}

	if (!snd_engine_start._feedback())
		Drive();
}

void CCar::SCarSound::UpdateStoping()
{
	VERIFY(!ph_world->Processing());
	SetSoundPosition(snd_engine_stop);

	if (!snd_engine_stop._feedback())
		SwitchOff();
}

void CCar::SCarSound::UpdateStalling()
{
	SetSoundPosition(snd_engine_stop);

	if (!snd_engine_stop._feedback())
		SwitchOff();
}

void CCar::SCarSound::UpdateDrive()
{
	VERIFY(!ph_world->Processing());
	float scale = 0.5f + 0.5f * pcar->m_current_rpm / pcar->m_torque_rpm;
	clamp(scale, 0.5f, 1.25f);

	snd_engine_idle.set_frequency(scale);
	snd_engine_1.set_frequency(scale);
	snd_engine_2.set_frequency(scale);
	snd_engine_3.set_frequency(scale);
	snd_engine_4.set_frequency(scale);
	snd_engine_5.set_frequency(scale);
	snd_nitro_loop.set_frequency(1.f);
	snd_skid_1.set_frequency(1.f);
	snd_skid_2.set_frequency(1.f);
	snd_skid_3.set_frequency(1.f);

	if (pcar->m_current_rpm < state_N_end) {
		snd_engine_idle.set_volume(1.f);
		snd_engine_1.set_volume(0.25);
		snd_engine_2.set_volume(0);
		snd_engine_3.set_volume(0);
		snd_engine_4.set_volume(0);
		snd_engine_5.set_volume(0);
	}
	else if (pcar->m_current_rpm >= state_1_start && pcar->m_current_rpm < state_1_end) {
		snd_engine_idle.set_volume(0.25);
		snd_engine_1.set_volume(1.f);
		snd_engine_2.set_volume(0.25);
		snd_engine_3.set_volume(0);
		snd_engine_4.set_volume(0);
		snd_engine_5.set_volume(0);
	}
	else if (pcar->m_current_rpm >= state_2_start && pcar->m_current_rpm < state_2_end) {
		snd_engine_idle.set_volume(0);
		snd_engine_1.set_volume(0.25);
		snd_engine_2.set_volume(1.f);
		snd_engine_3.set_volume(0.25);
		snd_engine_4.set_volume(0);
		snd_engine_5.set_volume(0);
	}
	else if (pcar->m_current_rpm >= state_3_start && pcar->m_current_rpm < state_3_end) {
		snd_engine_idle.set_volume(0);
		snd_engine_1.set_volume(0);
		snd_engine_2.set_volume(0.25);
		snd_engine_3.set_volume(1.f);
		snd_engine_4.set_volume(0.25);
		snd_engine_5.set_volume(0);
	}
	else if (pcar->m_current_rpm >= state_4_start && pcar->m_current_rpm < state_4_end) {
		snd_engine_idle.set_volume(0);
		snd_engine_1.set_volume(0);
		snd_engine_2.set_volume(0);
		snd_engine_3.set_volume(0.25);
		snd_engine_4.set_volume(1.f);
		snd_engine_5.set_volume(0.25);
	}
	else if (pcar->m_current_rpm >= state_5_start && pcar->m_current_rpm < state_5_end) {
		snd_engine_idle.set_volume(0);
		snd_engine_1.set_volume(0);
		snd_engine_2.set_volume(0);
		snd_engine_3.set_volume(0);
		snd_engine_4.set_volume(0.25);
		snd_engine_5.set_volume(1.f);
	}

	if (pcar->b_engine_on == false) {
		snd_engine_idle.set_volume(0);
		snd_engine_1.set_volume(0);
		snd_engine_2.set_volume(0);
		snd_engine_3.set_volume(0);
		snd_engine_4.set_volume(0);
		snd_engine_5.set_volume(0);
	}

	SetSoundPosition(snd_engine_idle);
	SetSoundPosition(snd_engine_1);
	SetSoundPosition(snd_engine_2);
	SetSoundPosition(snd_engine_3);
	SetSoundPosition(snd_engine_4);
	SetSoundPosition(snd_engine_5);
	SetSoundPosition(snd_nitro_loop);
	SetSoundPosition(snd_skid_1);
	SetSoundPosition(snd_skid_2);
	SetSoundPosition(snd_skid_3);
}

void CCar::SCarSound::SwitchState(ESoundState new_state)
{
	eCarSound = new_state;
	time_state_start = Device.dwTimeGlobal;
}

void CCar::SCarSound::Update()
{
	VERIFY(!ph_world->Processing());
	if (eCarSound == sndOff)
		return;

	if (snd_beep._feedback())
		SetSoundPosition(snd_beep);

	//if (snd_brakes._feedback())
	//	SetSoundPosition(snd_brakes);

	switch (eCarSound)
	{
	case sndStarting:
		UpdateStarting();
		break;
	case sndDrive:
		UpdateDrive();
		break;
	case sndStalling:
		UpdateStalling();
		break;
	case sndStoping:
		UpdateStalling();
		break;
	}
}

void CCar::SCarSound::SwitchOn()
{
	pcar->processing_activate();
}

void CCar::SCarSound::Destroy()
{
	SwitchOff();
	snd_engine_1.destroy();
	snd_engine_2.destroy();
	snd_engine_3.destroy();
	snd_engine_4.destroy();
	snd_engine_5.destroy();
	snd_transmission.destroy();
	snd_engine_stop.destroy();
	snd_engine_start.destroy();
	snd_beep.destroy();
	//snd_brakes.destroy();
	snd_skid_1.destroy();
	snd_skid_2.destroy();
	snd_skid_3.destroy();
}

void CCar::SCarSound::SwitchOff()
{
	eCarSound = sndOff;
	pcar->processing_deactivate();
}

void CCar::SCarSound::Start()
{
	VERIFY(!ph_world->Processing());
	if (eCarSound == sndOff)
		SwitchOn();
	SwitchState(sndStarting);
	snd_engine_start.play(pcar);
	SetSoundPosition(snd_engine_start);
}

void CCar::SCarSound::Stall()
{
	VERIFY(!ph_world->Processing());
	if (eCarSound == sndOff)
		return;
	SwitchState(sndStalling);
	snd_engine.stop_deffered();
	snd_engine_stop.play(pcar);
	SetSoundPosition(snd_engine_stop);
}

void CCar::SCarSound::Stop()
{
	VERIFY(!ph_world->Processing());
	if (eCarSound == sndOff)
		return;
	SwitchState(sndStoping);
	snd_engine_1.stop_deffered();
	snd_engine_2.stop_deffered();
	snd_engine_3.stop_deffered();
	snd_engine_4.stop_deffered();
	snd_engine_5.stop_deffered();
	snd_nitro_loop.stop_deffered();
	snd_skid_1.stop_deffered();
	snd_skid_2.stop_deffered();
	snd_skid_3.stop_deffered();
	snd_engine_stop.play(pcar);
	SetSoundPosition(snd_engine_stop);
}

void CCar::SCarSound::Drive()
{
	VERIFY(!ph_world->Processing());
	if (eCarSound == sndOff)
		SwitchOn();
	SwitchState(sndDrive);
	if (!snd_engine._feedback())
		snd_engine_1.play(pcar, sm_Looped);
		snd_engine_2.play(pcar, sm_Looped);
		snd_engine_3.play(pcar, sm_Looped);
		snd_engine_4.play(pcar, sm_Looped);
		snd_engine_5.play(pcar, sm_Looped);
		snd_skid_1.play(pcar, sm_Looped);
		snd_skid_2.play(pcar, sm_Looped);
		snd_skid_3.play(pcar, sm_Looped);
		snd_nitro_loop.play(pcar, sm_Looped);
	SetSoundPosition(snd_engine);
}

void CCar::SCarSound::TransmissionSwitch()
{
	VERIFY(!ph_world->Processing());
	if (snd_transmission._handle() && eCarSound != sndOff)
	{
		snd_transmission.play(pcar);
		SetSoundPosition(snd_transmission);
	}
}

void CCar::SCarSound::Beep()
{
	if (snd_beep._handle())
	{
		snd_beep.play(pcar);
		SetSoundPosition(snd_beep);
	}
}

void CCar::SCarSound::Brakes()
{
	//if (snd_brakes._handle())
	//{
	//	snd_brakes.play(pcar);
	//	SetSoundPosition(snd_brakes);
	//}
}

void CCar::SCarSound::NitroStart()
{
	if (snd_nitro_start._handle())
	{
		snd_nitro_start.play(pcar);
		SetSoundPosition(snd_nitro_start);
	}
}

void CCar::SCarSound::NitroEnd()
{
	if (snd_nitro_end._handle())
	{
		snd_nitro_end.play(pcar);
		SetSoundPosition(snd_nitro_end);
	}
}
