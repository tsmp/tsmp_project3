#include "stdafx.h"
#include "psy_dog_aura.h"
#include "psy_dog.h"
#include "../../../actor.h"
#include "../../../actor_memory.h"
#include "../../../visual_memory_manager.h"
#include "../../../level.h"

CPPEffectorPsyDogAura::CPPEffectorPsyDogAura(const SPPInfo &ppi, u32 time_to_fade)
	: inherited(ppi)
{
	m_time_to_fade = time_to_fade;
	m_effector_state = eStateFadeIn;
	m_time_state_started = Device.dwTimeGlobal;
}

void CPPEffectorPsyDogAura::switch_off()
{
	m_effector_state = eStateFadeOut;
	m_time_state_started = Device.dwTimeGlobal;
}

BOOL CPPEffectorPsyDogAura::update()
{
	// update factor
	if (m_effector_state == eStatePermanent)
	{
		m_factor = 1.f;
	}
	else
	{
		m_factor = float(Device.dwTimeGlobal - m_time_state_started) / float(m_time_to_fade);
		if (m_effector_state == eStateFadeOut)
			m_factor = 1 - m_factor;

		if (m_factor > 1)
		{
			m_effector_state = eStatePermanent;
			m_factor = 1.f;
		}
		else if (m_factor < 0)
		{
			return FALSE;
		}
	}
	return TRUE;
}

void CPsyDogAura::reinit()
{
	m_time_actor_saw_phantom = 0;
	m_time_phantom_saw_actor = 0;

	m_actor = smart_cast<CActor *>(Level().CurrentEntity());
	VERIFY(m_actor);
}

void CPsyDogAura::update_schedule()
{
	if (!m_object->g_Alive())
		return;

	m_time_phantom_saw_actor = 0;

	// Check memory of actor and check memory of phantoms
	const auto &visibleObjects = m_actor->memory().visual().objects();

	for (const auto visibleObj: visibleObjects)
	{
		const CGameObject *obj = visibleObj.m_object;

		if (smart_cast<const CPsyDogPhantom*>(obj))
		{
			if (m_actor->memory().visual().visible_now(obj))
				m_time_actor_saw_phantom = time();
		}
	}

	// Check memory and enemy manager of phantoms whether they see actor
	const auto &phantoms = m_object->m_storage;

	for(CPsyDogPhantom* phantom: phantoms)
	{
		if (phantom->EnemyMan.get_enemy() == m_actor)
			m_time_phantom_saw_actor = time();
		else
		{
			const auto &phantomEnemies = phantom->EnemyMemory.get_memory();

			for(auto &[entity, monsterEnemy]: phantomEnemies)
			{
				if (entity == m_actor)
				{
					m_time_phantom_saw_actor = monsterEnemy.time;
					break;
				}
			}
		}

		if (m_time_phantom_saw_actor == time())
			break;
	}

	bool needToBeActive = (m_time_actor_saw_phantom + 2000 > time()) || (m_time_phantom_saw_actor != 0);

	if (active())
	{
		if (!needToBeActive)
		{
			m_effector->switch_off();
			m_effector = 0;
		}
	}
	else
	{
		if (needToBeActive)
		{
			// Create effector
			m_effector = xr_new<CPPEffectorPsyDogAura>(m_state, 5000);
			Actor()->Cameras().AddPPEffector(m_effector);
		}
	}
}

void CPsyDogAura::on_death()
{
	if (active())
	{
		m_effector->switch_off();
		m_effector = 0;
	}
}
