#include "StdAfx.h"
#include "game_sv_race.h"
#include "xrserver_objects_alife_monsters.h"
#include "xrServer.h"
#include "..\xrNetwork\client_id.h"
#include "Level.h"
#include "xrGameSpyServer.h"
#include "Actor.h"
#include "GameObject.h"
#include "holder_custom.h"

ENGINE_API bool g_dedicated_server;
extern int G_DELAYED_ROUND_TIME;
extern INT g_sv_Pending_Wait_Time;
u32 TimeBeforeRaceStart = 10000; // 10 sec
BOOL g_sv_race_invulnerability = FALSE;
u32 g_sv_race_reinforcementTime = 10; // 10 sec

game_sv_Race::game_sv_Race()
{
	m_phase = GAME_PHASE_NONE;
	m_type = GAME_RACE;
	m_WinnerId = u16(-1);
	m_CurrentRpoint = 0;
	m_WinnerFinishTime = 0;
	m_CurrentRoundCar = u8(-1);
	m_MaxPlayers = 4;
	m_CurrentRoad = -1;

	m_CarRandom.seed(static_cast<s32>(time(nullptr)));
	m_CarVisualRandom.seed(static_cast<s32>(time(nullptr)));
	LoadRaceSettings();
}

void game_sv_Race::LoadRaceSettings()
{
	string_path temp;
	m_AvailableSkins.push_back("stalker_killer_antigas");
	m_AvailableCars.push_back({ "physics\\vehicles\\kamaz\\veh_kamaz_u_01.ogf" });

	if (!FS.exist(temp, "$level$", "race.ltx"))
	{
		Msg("- race ltx not found, using default settings");
		return;
	}

	Msg("- race ltx found, using it");
	CInifile* settings = xr_new<CInifile>(temp);

	if (settings->line_exist("settings", "max_players"))
		m_MaxPlayers = settings->r_s32("settings", "max_players");

	if (settings->section_exist("player_visuals"))
	{
		const auto & visualsSect = settings->r_section("player_visuals").Data;

		if (!visualsSect.empty())
			m_AvailableSkins.clear();

		for (const auto &item: visualsSect)
			m_AvailableSkins.push_back(item.first.c_str());
	}

	LoadCarVisuals(settings);
	xr_delete(settings);
}

void game_sv_Race::LoadCarVisuals(CInifile* settings)
{
	if (!settings->section_exist("available_cars"))
		return;

	const auto& sect = settings->r_section("available_cars").Data;

	if (sect.empty())
		return;
	
	m_AvailableCars.clear();

	for (const auto& item : sect)
	{
		std::string carPath = item.first.c_str();

		auto ModelExists = [&](const std::string& path) -> bool
		{
			string_path temp;
			return FS.exist(temp, "$game_meshes$", path.c_str());
		};

		if (carPath.find(".ogf") != std::string::npos && ModelExists(carPath))
		{
			m_AvailableCars.push_back({ carPath });
			continue;
		}

		if (ModelExists(carPath + ".ogf"))
		{
			m_AvailableCars.push_back({ carPath + ".ogf" });
			continue;
		}

		static const std::string ColorPrefix = "_color_";
		u32 colorCounter = 0;
		std::vector<std::string> coloredCars;

		while (ModelExists(carPath + ColorPrefix + std::to_string(colorCounter) + ".ogf"))
		{
			coloredCars.push_back(carPath + ColorPrefix + std::to_string(colorCounter) + ".ogf");
			colorCounter++;
		}

		if (!coloredCars.empty())
		{
			m_AvailableCars.push_back(std::move(coloredCars));
			continue;
		}

		Msg("! WARNING: can not find model [%s]", carPath.c_str());
	}
}

void game_sv_Race::Create(shared_str& options)
{
	inherited::Create(options);

	TeamStruct NewTeam;
	for(const auto &skin: m_AvailableSkins)
		NewTeam.aSkins.push_back(skin.c_str());
	TeamList.push_back(NewTeam);

	switch_Phase(GAME_PHASE_PENDING);
}

void game_sv_Race::UpdatePending()
{
	if (AllPlayersReady() || (m_WinnerFinishTime && (Level().timeServer() - m_WinnerFinishTime) > u32(g_sv_Pending_Wait_Time)))
		OnRoundStart();
}

void game_sv_Race::UpdateRaceStart()
{
	if (m_start_time + TimeBeforeRaceStart <= Level().timeServer())
		switch_Phase(GAME_PHASE_INPROGRESS);
}

void game_sv_Race::UpdateScores()
{
	if (m_WinnerFinishTime + G_DELAYED_ROUND_TIME * 1000 < Level().timeServer())
	{
		round_end_reason = eRoundEnd_Finish;
		OnRoundEnd();
	}
}

u32 game_sv_Race::GetPlayerRespawnTime(const game_PlayerState* ps)
{
	return ps->DeathTime + g_sv_race_reinforcementTime * 1000;
}

void game_sv_Race::UpdateInProgress()
{
	if (g_dedicated_server && m_server->GetClientsCount() == 1)
	{
		m_WinnerFinishTime = 0;
		OnRoundEnd();
	}

	m_server->ForEachClientDoSender([&](IClient* client)
	{
		xrClientData* l_pC = static_cast<xrClientData*>(client);
		game_PlayerState* ps = l_pC->ps;

		if (!l_pC->net_Ready || ps->IsSkip())
			return;

		if (!ps->DeathTime)
			ps->DeathTime = Level().timeServer();

		if (!ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
			return;			

		if (GetPlayerRespawnTime(l_pC->ps) < Level().timeServer())
		{
			DestroyCarOfPlayer(ps);
			SpawnPlayer(l_pC->ID, "spectator");

			if (!SpawnPlayerInCar(l_pC->ID))
			{
				l_pC->ps->DeathTime = Level().timeServer(); // reset respawn time
				signal_Syncronize(); // update timer on client
			}
		}
	});
}

void game_sv_Race::Update()
{
	inherited::Update();

	switch (Phase())
	{
	case GAME_PHASE_PENDING:
		UpdatePending();
		break;

	case GAME_PHASE_RACE_START:
		UpdateRaceStart();
		break;

	case GAME_PHASE_PLAYER_SCORES:
		UpdateScores();
		break;

	case GAME_PHASE_INPROGRESS:
		UpdateInProgress();
		break;
	}
}

void game_sv_Race::OnPlayerConnect(ClientID const &id_who)
{
	smart_cast<xrGameSpyServer*>(m_server)->m_iMaxPlayers = m_MaxPlayers;
	inherited::OnPlayerConnect(id_who);
	xrClientData* xrCData = m_server->ID_to_client(id_who);
	game_PlayerState* ps_who = get_id(id_who);

	if (!xrCData->flags.bReconnect)
	{
		ps_who->clear();
		ps_who->team = 0;
		ps_who->skin = -1;
	}

	ps_who->setFlag(GAME_PLAYER_FLAG_SPECTATOR);
	ps_who->setFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
	ps_who->resetFlag(GAME_PLAYER_FLAG_SKIP);

	if (g_dedicated_server && xrCData == m_server->GetServerClient())
	{
		ps_who->setFlag(GAME_PLAYER_FLAG_SKIP);
		return;
	}

	SetPlayersDefItems(ps_who);
}

void game_sv_Race::OnPlayerConnectFinished(ClientID const &id_who)
{	
	SpawnPlayer(id_who, "spectator");

	if (xrClientData* xrCData = m_server->ID_to_client(id_who))
	{
		NET_Packet P;
		GenerateGameMessage(P);
		P.w_u32(GAME_EVENT_PLAYER_CONNECTED);
		P.w_stringZ(xrCData->name.c_str());
		u_EventSend(P);
	}
}

void game_sv_Race::OnGKill(NET_Packet &P)
{
	u16 ID = P.r_u16();

	if (xrClientData* l_pC = get_client(ID))	
		KillPlayer(l_pC->ID, l_pC->ps->GameID);	
}

void game_sv_Race::OnBaseEnter(NET_Packet &P)
{
	u16 playerId = P.r_u16();
	u8 baseTeam = P.r_u8();

	if (m_WinnerId == u16(-1))
	{
		m_WinnerFinishTime = Level().timeServer();
		m_WinnerId = playerId;
		switch_Phase(GAME_PHASE_PLAYER_SCORES);	
	}
}

void game_sv_Race::OnEvent(NET_Packet &P, u16 type, u32 time, ClientID const &sender)
{
	if (g_sv_race_invulnerability && type == GAME_EVENT_ON_HIT)
		return;

	switch (type)
	{
	case GAME_EVENT_PLAYER_KILL:
		OnGKill(P);
		break;

	case GAME_EVENT_PLAYER_ENTER_TEAM_BASE:
		OnBaseEnter(P);
		break;

	case GAME_EVENT_PLAYER_LEAVE_TEAM_BASE:
		break;

	default:
		inherited::OnEvent(P, type, time, sender);
		break;
	}	
}

void game_sv_Race::OnRoundStart()
{
	SelectRoad();
	Msg("- RACE: round start");
	inherited::OnRoundStart();
	m_WinnerId = u16(-1);
	m_CurrentRpoint = 0;
	m_CurrentRoundCar = m_CarRandom.randI(static_cast<int>(m_AvailableCars.size()));
	switch_Phase(GAME_PHASE_RACE_START);

	// Respawn all players
	m_server->ForEachClientDoSender([this](IClient* cl)
	{
		xrClientData* l_pC = dynamic_cast<xrClientData*>(cl);

		if (!l_pC || !l_pC->net_Ready || !l_pC->ps)
			return;

		game_PlayerState* ps = l_pC->ps;
		ps->clear();		
		SpawnPlayer(l_pC->ID, "spectator");
			
		if (!ps->IsSkip())
			SpawnPlayerInCar(l_pC->ID);
	});
}

void game_sv_Race::OnRoundEnd()
{
	Msg("- RACE: round end");

	m_server->ForEachClientDoSender([this](IClient* cl)
	{
		xrClientData* l_pC = dynamic_cast<xrClientData*>(cl);
		game_PlayerState* ps = l_pC->ps;
		if (!ps)
			return;
		if (ps->IsSkip())
			return;
		SpawnPlayer(l_pC->ID, "spectator");
	});
	
	inherited::OnRoundEnd();
}

void game_sv_Race::CleanClientData(xrClientData* xrCData)
{
	xrCData->ps->m_s16LastSRoint = -1;
	DestroyCarOfPlayer(xrCData->ps);
}

void game_sv_Race::OnPlayerDisconnect(ClientID const &id_who, LPSTR Name, u16 GameID)
{
	inherited::OnPlayerDisconnect(id_who, Name, GameID);
}

void game_sv_Race::AssignRPoint(CSE_Abstract* E, u16 rpoint)
{
	R_ASSERT(E);
	const xr_vector<RPoint> &rp = rpoints[m_CurrentRoad];

	RPoint r = rp[rpoint];

	E->o_Position.set(r.P);
	E->o_Angle.set(r.A);
}

CSE_Abstract* game_sv_Race::SpawnCar(u16 rpoint)
{
	CSE_Abstract* E = nullptr;
	E = spawn_begin("m_car");
	E->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
		
	AssignRPoint(E, rpoint);

	CSE_Visual* pV = smart_cast<CSE_Visual*>(E);
	const auto& carVisuals = m_AvailableCars[m_CurrentRoundCar];
	R_ASSERT2(!carVisuals.empty(), "There are no visuals for cars");
	pV->set_visual(carVisuals[m_CarVisualRandom.randI(carVisuals.size())].c_str());

	CSE_Abstract* spawned = spawn_end(E, m_server->GetServerClient()->ID);
	signal_Syncronize();
	return spawned;
}

u16 game_sv_Race::GetRpointIdx(game_PlayerState* ps)
{
	const u8 rpointsTeam = m_CurrentRoad;
	u16 rpoint = ps->m_s16LastSRoint;

	// player has associated rpoint
	if (rpoint != static_cast<u16>(-1))
		return rpoint;

	// assign next free rpoint
	if (m_CurrentRpoint < rpoints[rpointsTeam].size())
	{
		rpoint = m_CurrentRpoint;
		ps->m_s16LastSRoint = rpoint;
		m_CurrentRpoint++;
		return rpoint;
	}

	// try to find free rpoint 
	std::vector<u16> freeRpointsVec;

	for (u32 i = 0, cnt = rpoints[rpointsTeam].size(); i < cnt; i++)
		freeRpointsVec.push_back(true);

	m_server->ForEachClientDoSender([&](IClient* client)
	{
		xrClientData* l_pC = static_cast<xrClientData*>(client);
		game_PlayerState* state = l_pC->ps;

		if (!l_pC->net_Ready || !state || state->IsSkip())
			return;

		if (state->m_s16LastSRoint != static_cast<u16>(-1))
			freeRpointsVec[state->m_s16LastSRoint] = false;
	});

	for (u16 i = 0; i < freeRpointsVec.size(); i++)
	{
		if (freeRpointsVec[i])
		{
			rpoint = i;
			break;
		}
	}

	return rpoint;
}

bool game_sv_Race::SpawnPlayerInCar(ClientID const &playerId)
{
	xrClientData* xrCData = m_server->ID_to_client(playerId);
	if (!xrCData || !xrCData->owner || !xrCData->ps)
		return false;

	CSE_Abstract* pOwner = xrCData->owner;
	CSE_Spectator* pS = smart_cast<CSE_Spectator*>(pOwner);
	R_ASSERT(pS);

	const u16 rpointIdx = GetRpointIdx(xrCData->ps);

	if (rpointIdx == static_cast<u16>(-1))
	{
		Msg("! ERROR: There are no free rpoints for player [%s]!", xrCData->ps->getName());
		return false;
	}

	CSE_Abstract* car = SpawnCar(rpointIdx);
	R_ASSERT(smart_cast<CSE_ALifeCar*>(car));

	xrCData->ps->resetFlag(GAME_PLAYER_FLAG_SPECTATOR);	

	if (pOwner->owner != m_server->GetServerClient())
		pOwner->owner = (xrClientData*)m_server->GetServerClient();

	//remove spectator entity
	NET_Packet P;
	u_EventGen(P, GE_DESTROY, pS->ID);
	Level().Send(P, net_flags(TRUE, TRUE));
	xrCData->ps->CarID = car->ID;

	xrCData->ps->skin = u8(::Random.randI((int)TeamList[xrCData->ps->team].aSkins.size()));
	SpawnPlayer(playerId, "mp_actor", car->ID);
	return true;
}

void game_sv_Race::OnPlayerReady(ClientID const& id)
{
	if (m_phase != GAME_PHASE_PENDING)
		return;

	if (game_PlayerState* ps = get_id(id))
	{
		if (ps->testFlag(GAME_PLAYER_FLAG_READY))
			ps->resetFlag(GAME_PLAYER_FLAG_READY);
		else
			ps->setFlag(GAME_PLAYER_FLAG_READY);

		signal_Syncronize();
	}
}

void game_sv_Race::net_Export_State(NET_Packet &P, ClientID const &id_to)
{
	inherited::net_Export_State(P, id_to);

	auto secondsToRespawn = [&]() -> u32
	{
		if (const game_PlayerState* ps = get_id(id_to))
		{
			const u32 respTime = GetPlayerRespawnTime(ps);
			const u32 curTime = Level().timeServer();

			if (curTime < respTime)
				return respTime - curTime;
		}

		return 0;
	};

	P.w_u32(secondsToRespawn());

	if (m_phase == GAME_PHASE_PLAYER_SCORES)
		P.w_u16(m_WinnerId);
}

void game_sv_Race::OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed, KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA)
{
	if (!ps_killed)
		return;

	ps_killed->setFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
	ps_killed->m_Stats.m_iDeaths++;
	ps_killed->m_iKillsInRowCurr = 0;
	ps_killed->DeathTime = Level().timeServer();

	signal_Syncronize();
}

void game_sv_Race::SelectRoad()
{
	R_ASSERT2(!rpoints[1].empty(), "No rpoints for players found. Use rpoints and base with team 1");

	if (rpoints[2].empty() || m_CurrentRoad == static_cast<u8>(-1))
		m_CurrentRoad = 1;
	else
	{
		if (m_CurrentRoad == 1)
			m_CurrentRoad = 2;
		else
			m_CurrentRoad = 1;
	}

	Msg("- selected road %u", static_cast<u32>(m_CurrentRoad));
}

BOOL game_sv_Race::OnPreCreate(CSE_Abstract* E)
{
	if (!inherited::OnPreCreate(E))
		return FALSE;

	if (auto base  = smart_cast<CSE_ALifeTeamBaseZone*>(E))
		return base->m_team == m_CurrentRoad;

	return true;
}
