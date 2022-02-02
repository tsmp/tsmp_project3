#include "StdAfx.h"
#include "game_sv_freeplay.h"

ENGINE_API bool g_dedicated_server;

game_sv_Freeplay::game_sv_Freeplay() 
{
	m_phase = GAME_PHASE_NONE;
	m_type = GAME_FREEPLAY;
}

game_sv_Freeplay::~game_sv_Freeplay() {}

void game_sv_Freeplay::Create(shared_str &options)
{
	inherited::Create(options);
	switch_Phase(GAME_PHASE_PENDING);
}

void game_sv_Freeplay::Update()
{
	inherited::Update();

	switch (Phase())
	{
	case GAME_PHASE_PENDING:
		OnRoundStart();
		break;
	}
}

void game_sv_Freeplay::SetStartMoney(ClientID &id_who)
{
	xrClientData* C = m_server->ID_to_client(id_who);
	if (!C || (C->ID != id_who))
		return;
	game_PlayerState* ps_who = (game_PlayerState*)C->ps;
	if (!ps_who)
		return;
	ps_who->money_for_round = 0;
	if (ps_who->team < 0)
		return;
	TeamStruct* pTeamData = GetTeamData(u8(ps_who->team));
	if (!pTeamData)
		return;
	ps_who->money_for_round = pTeamData->m_iM_Start;
}

void game_sv_Freeplay::OnPlayerConnect(ClientID id_who)
{
	inherited::OnPlayerConnect(id_who);
	xrClientData* xrCData = m_server->ID_to_client(id_who);
	game_PlayerState* ps_who = get_id(id_who);

	if (!xrCData->flags.bReconnect)
	{
		ps_who->clear();
		ps_who->team = 0;
		ps_who->skin = -1;
	};

	ps_who->setFlag(GAME_PLAYER_FLAG_SPECTATOR);
	ps_who->resetFlag(GAME_PLAYER_FLAG_SKIP);

	if (g_dedicated_server && xrCData == m_server->GetServerClient())
	{
		ps_who->setFlag(GAME_PLAYER_FLAG_SKIP);
		return;
	}

	if (!xrCData->flags.bReconnect || ps_who->money_for_round == -1)
		SetStartMoney(id_who);

	SetPlayersDefItems(ps_who);
}

void game_sv_Freeplay::OnPlayerConnectFinished(ClientID id_who)
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

void game_sv_Freeplay::OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID)
{
	inherited::OnPlayerDisconnect(id_who, Name, GameID);
}
