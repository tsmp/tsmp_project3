#pragma once
#include "../control_combase.h"
#include "..\include\xrRender\animation_motion.h"

class CPsyHitEffectorCam;
class CPsyHitEffectorPP;
class CEntityAlive;

enum class PsyHitMpSyncStages : u8
{
	Prepare = 0,
	Deactivate,
	GlideStart,
	Stop,
	Hit
};

class CControllerPsyHit : public CControl_ComCustom<>
{
	typedef CControl_ComCustom<> inherited;

	MotionID m_stage[4];
	u8 m_current_index;

	CPsyHitEffectorCam *m_effector_cam;
	CPsyHitEffectorPP *m_effector_pp;

	float m_min_tube_dist;

	// internal flag if weapon was hidden
	bool m_blocked;

public:
	virtual void load(LPCSTR section);
	virtual void reinit();
	virtual void update_frame();
	virtual bool check_start_conditions();
	virtual void activate();
	virtual void deactivate();

	virtual void on_event(ControlCom::EEventType, ControlCom::IEventData *);

	void on_death();

	void SendEventMP(u16 idTo, PsyHitMpSyncStages stage);

	enum ESoundState
	{
		ePrepare,
		eStart,
		ePull,
		eHit,
		eNone
	} m_sound_state;

	void set_sound_state(ESoundState state);
	void death_glide_start();
	void death_glide_end();

private:
	void stop();
	void play_anim();
	void hit();
	bool check_conditions_final();

	u16 enemyId;
	bool enemyIsActor;
	const CEntityAlive* GetEnemy();
};
