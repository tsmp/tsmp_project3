////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_surge_manager.cpp
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator surge manager
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "alife_surge_manager.h"
#include "alife_object_registry.h"
#include "alife_spawn_registry.h"
#include "alife_time_manager.h"
#include "alife_graph_registry.h"
#include "alife_schedule_registry.h"
#include "alife_simulator_header.h"
#include "ai_space.h"
#include "ef_storage.h"
#include "ef_pattern.h"
#include "graph_engine.h"
#include "xrserver.h"
#include "alife_human_brain.h"

using namespace ALife;

void CALifeSurgeManager::spawn_new_spawns()
{
	auto simulator = ai().m_alife_simulator;
	auto &spawns = simulator->spawns().spawns();

	for (u16 spawnID: m_temp_spawns)
	{
		auto abstractObj = &spawns.vertex(spawnID)->data()->object();
		auto spawn = smart_cast<CSE_ALifeDynamicObject*>(abstractObj);
		VERIFY3(spawn, abstractObj->name(), abstractObj->name_replace());

#ifdef DEBUG
		CTimer timer;
		timer.Start();
#endif

		CSE_ALifeDynamicObject* object = nullptr;
		simulator->create(object, spawn, spawnID);

#ifdef DEBUG
		if (psAI_Flags.test(aiALife))
		{
			auto &gameGraph = ai().game_graph();
			auto levelName = *gameGraph.header().level(gameGraph.vertex(spawn->m_tGraphID)->level_id()).name();
			Msg("LSS : SURGE : SPAWN : [%s],[%s], level %s, time %f ms", *spawn->s_name, spawn->name_replace(), levelName, timer.GetElapsed_ms());
		}
#endif
	}
}

void CALifeSurgeManager::fill_spawned_objects()
{
	m_temp_spawned_objects.clear();

	auto simulator = ai().m_alife_simulator;
	auto &alifeObjects = simulator->objects().objects();
	auto &spawns = simulator->spawns().spawns();

	for (auto& pair : alifeObjects)
	{
		const u16 spawnID = pair.second->m_tSpawnID;

		if (spawns.vertex(spawnID))
			m_temp_spawned_objects.push_back(spawnID);
	}
}

void CALifeSurgeManager::spawn_new_objects()
{
	fill_spawned_objects();
	auto simulator = ai().m_alife_simulator;
	simulator->spawns().fill_new_spawns(m_temp_spawns, simulator->time_manager().game_time(), m_temp_spawned_objects);
	spawn_new_spawns();
	VERIFY(simulator->graph().actor());
}
