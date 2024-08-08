#include "stdafx.h"
#include "HeliKillerMode.h"
#include "game_sv_base.h"
#include "Level.h"
#include "xrServer_Objects_ALife.h"
#include "patrol_path.h"
#include "ai_space.h"
#include "patrol_path_storage.h"
#include "Actor.h"
#include "helicopter.h"

#pragma warning(push)
#pragma warning(disable : 4995) // deprecation warning
#include <random>
#pragma warning(pop)

void SendStatusMessage(const char* msg)
{
	if (auto svGame = Level().Server->game)
	{
		NET_Packet netP;
		svGame->GenerateGameMessage(netP);

		netP.w_u32(GAME_EVENT_SERVER_STRING_MESSAGE);
		netP.w_stringZ(msg);

		svGame->SvEventSend(netP);
	}
}

void HeliKillerMode::SpawnHeli(xrClientData* callerData)
{
	auto caller = callerData->ps;
	R_ASSERT(caller);

	std::string customData =
		"[logic]\n\
		active = heli_move\n\
		[heli_move]\n\
		combat_ignore = false \n\
		combat_use_mgun = false \n\
		combat_use_rocket = true \n\
		combat_velocity = 30 \n\
		path_move = heli_point\n\
		";

	u32 teamToKill = caller->team == 1 ? 2 : 1;
	customData += "combat_enemy = team" + std::to_string(teamToKill);	

	const CPatrolPath* patrolPath = ai().patrol_paths().path("heli_point");
	const CPatrolPoint* pt = &patrolPath->vertex(0)->data();

	auto svGame = Level().Server->game;
	CSE_Abstract* E = svGame->spawn_begin("helicopter");
	E->s_flags.assign(M_SPAWN_OBJECT_LOCAL);

	E->o_Position.set(pt->position());
	E->o_Angle.set(zero_vel);

	CSE_Visual* pV = smart_cast<CSE_Visual*>(E);
	pV->set_visual("physics\\vehicles\\mi24\\veh_mi24_u_01.ogf");

	auto hel = smart_cast<CSE_ALifeHelicopter*>(E);
	hel->startup_animation = "idle";
	hel->engine_sound = "alexmx\\helicopter.ogg";

	E->m_ini_string = customData.c_str();
	CSE_Abstract* spawned = svGame->spawn_end(E, Level().Server->GetServerClient()->ID);
	smart_cast<CSE_ALifeHelicopter*>(spawned)->CallerClID = callerData->ID;
	svGame->signal_Syncronize();

	string256 msg;
	sprintf(msg, "%%c[green] Игрок [%s] вызвал вертолет!", caller->name);
	SendStatusMessage(msg);
}

u16 HeliKillerMode::GetEnemyId()
{
	std::vector<u16> availablePlayers;

	Level().Server->ForEachClientDo([&](IClient* client)
	{
		xrClientData* cldata = static_cast<xrClientData*>(client);

		if (game_PlayerState* ps = cldata->ps)
		{
			if (ps->team != m_EnemyTeam)
				return;

			if (smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID)))
				availablePlayers.push_back(ps->GameID);
		}
	});

	if (m_FirstEnemy)
	{
		m_EnabledTime = Level().timeServer();
		m_FirstEnemy = false;
	}
	else
	{
		if (Level().timeServer() > m_EnabledTime + 1000 * 60 * 2) // disable after 2 minutes work
		{
			m_Finished = true;
			Msg("- Helicopter killer mode deactivated on timer");
			SendStatusMessage("%c[green] У вертолета закончились боеприпасы");
		}
	}

	if (!availablePlayers.empty())
	{
		static std::random_device rd;
		static std::mt19937 g(rd());
		std::shuffle(availablePlayers.begin(), availablePlayers.end(), g);
		return availablePlayers[0];
	}

	return static_cast<u16>(-1);
}

void HeliKillerMode::SetHeliId(u16 id)
{
	m_HeliId = id;
}

void HeliKillerMode::SetEnemyTeam(u32 team) 
{
	Msg("- Helicopter killer mode activated on team [%d]", team);
	m_Enabled = true;
	m_EnemyTeam = team;
}

bool HeliKillerMode::Enabled() const
{
	return m_Enabled;
}

bool HeliKillerMode::Finished()
{
	if (Level().timeServer() > m_EnabledTime + 1000 * 60 * 3) // kill him after 3 minutes work
		KillHeli();	

	return m_Finished;
}

void HeliKillerMode::KillHeli()
{
	Msg("- Helicopter killer mode destroying heli");
	NET_Packet P;
	Game().u_EventGen(P, GE_DESTROY, m_HeliId);
	Game().u_EventSend(P);
}
