#include "StdAfx.h"
#include "game_sv_race.h"
#include "xrserver_objects_alife_monsters.h"
#include "xrServer.h"
#include "..\xrNetwork\client_id.h"
#include "Level.h"

ENGINE_API bool g_dedicated_server;

game_sv_Race::game_sv_Race()
{
	m_phase = GAME_PHASE_NONE;
	m_type = GAME_FREEPLAY;
}

game_sv_Race::~game_sv_Race() {}

void game_sv_Race::Create(shared_str& options)
{
	inherited::Create(options);

	TeamStruct NewTeam;
	NewTeam.aSkins.push_back("stalker_killer_antigas");
	TeamList.push_back(NewTeam);

	switch_Phase(GAME_PHASE_PENDING);
}

void game_sv_Race::Update()
{
	inherited::Update();

	switch (Phase())
	{
	case GAME_PHASE_PENDING:
		OnRoundStart();
		break;
	}
}

void game_sv_Race::OnPlayerConnect(ClientID id_who)
{
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
	ps_who->resetFlag(GAME_PLAYER_FLAG_SKIP);

	if (g_dedicated_server && xrCData == m_server->GetServerClient())
	{
		ps_who->setFlag(GAME_PLAYER_FLAG_SKIP);
		return;
	}

	SetPlayersDefItems(ps_who);
}

void game_sv_Race::OnPlayerConnectFinished(ClientID id_who)
{
	xrClientData* xrCData = m_server->ID_to_client(id_who);
	SpawnPlayer(id_who, "spectator");

	if (xrCData)
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
		m_WinnerId = playerId;
		switch_Phase(GAME_PHASE_PLAYER_SCORES);

		if (auto eActor = smart_cast<CSE_ALifeCreatureActor*>(m_server->ID_to_entity(playerId)))
		{
			if (auto ps = eActor->owner->ps)
			{
				std::string str = ps->getName();
				str += " wins!";
				SvSendChatMessage(str.c_str());
			}
		}		
	}
}

void game_sv_Race::OnEvent(NET_Packet& P, u16 type, u32 time, ClientID sender)
{
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
	inherited::OnRoundStart();
	m_WinnerId = u16(-1);

	// Respawn all players
	m_server->ForEachClientDoSender([this](IClient* cl)
	{
		xrClientData* l_pC = dynamic_cast<xrClientData*>(cl);

		if (!l_pC || !l_pC->net_Ready || !l_pC->ps)
			return;

		game_PlayerState* ps = l_pC->ps;
		ps->clear();
		SpawnPlayer(l_pC->ID, "spectator");
	});
}

void game_sv_Race::OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID)
{
	inherited::OnPlayerDisconnect(id_who, Name, GameID);
}

void game_sv_Race::AssignRPoint(CSE_Abstract* E)
{
	R_ASSERT(E);
	const xr_vector<RPoint> &rp = rpoints[0];

	RPoint r;
	u32 ID = 0;

	r = rp[ID];
	E->o_Position.set(r.P);
	E->o_Angle.set(r.A);
}

CSE_Abstract* game_sv_Race::SpawnCar()
{
	CSE_Abstract* E = nullptr;
	E = spawn_begin("m_car");
	E->s_flags.assign(M_SPAWN_OBJECT_LOCAL); // flags
	AssignRPoint(E);

	CSE_Visual* pV = smart_cast<CSE_Visual*>(E);
	pV->set_visual("physics\\vehicles\\kamaz\\veh_kamaz_u_01.ogf");

	CSE_Abstract* spawned = spawn_end(E, m_server->GetServerClient()->ID);
	signal_Syncronize();
	return spawned;
}

void game_sv_Race::SpawnPlayerInCar(ClientID &playerId)
{
	CSE_Abstract* car = SpawnCar();
	R_ASSERT(smart_cast<CSE_ALifeCar*>(car));

	xrClientData* xrCData = m_server->ID_to_client(playerId);
	if (!xrCData || !xrCData->owner)
		return;

	CSE_Abstract* pOwner = xrCData->owner;
	CSE_Spectator* pS = smart_cast<CSE_Spectator*>(pOwner);
	R_ASSERT(pS);

	if (pOwner->owner != m_server->GetServerClient())
		pOwner->owner = (xrClientData*)m_server->GetServerClient();

	//remove spectator entity
	NET_Packet P;
	u_EventGen(P, GE_DESTROY, pS->ID);
	Level().Send(P, net_flags(TRUE, TRUE));

	SpawnPlayer(playerId, "mp_actor", car->ID);
}

void game_sv_Race::OnPlayerReady(ClientID id)
{
	if (m_phase == GAME_PHASE_INPROGRESS)
	{
		xrClientData* xrCData = m_server->ID_to_client(id);
		game_PlayerState* ps = get_id(id);
		if (ps->IsSkip())
			return;

		if (!(ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)))
			return;

		xrClientData* xrSCData = (xrClientData*)m_server->GetServerClient();
		CSE_Abstract* pOwner = xrCData->owner;

		SpawnPlayerInCar(id);
		pOwner = xrCData->owner;
	}
}
