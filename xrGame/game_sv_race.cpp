#include "StdAfx.h"
#include "game_sv_race.h"
#include "xrserver_objects_alife_monsters.h"
#include "xrServer.h"

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

void game_sv_Race::OnEvent(NET_Packet& P, u16 type, u32 time, ClientID sender)
{
	if (type == GAME_EVENT_PLAYER_KILL) // g_kill
	{
		u16 ID = P.r_u16();

		if (xrClientData* l_pC = get_client(ID))
			KillPlayer(l_pC->ID, l_pC->ps->GameID);

		return;
	}

	inherited::OnEvent(P, type, time, sender);
}

void game_sv_Race::OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID)
{
	inherited::OnPlayerDisconnect(id_who, Name, GameID);
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

		RespawnPlayer(id, false);
		pOwner = xrCData->owner;
	}
}
