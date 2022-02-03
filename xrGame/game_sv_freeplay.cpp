#include "StdAfx.h"
#include "game_sv_freeplay.h"
#include "xrserver_objects_alife_monsters.h"
#include "ui/UIBuyWndShared.h"

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

	TeamStruct NewTeam;
	NewTeam.aSkins.push_back("stalker_killer_antigas");
	TeamList.push_back(NewTeam);

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

void game_sv_Freeplay::OnEvent(NET_Packet &P, u16 type, u32 time, ClientID sender)
{
	if(type == GAME_EVENT_PLAYER_KILL) // g_kill
	{
		u16 ID = P.r_u16();

		if(xrClientData* l_pC = get_client(ID))
			KillPlayer(l_pC->ID, l_pC->ps->GameID);

		return;
	}

	inherited::OnEvent(P, type, time, sender);
}

void game_sv_Freeplay::OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID)
{
	inherited::OnPlayerDisconnect(id_who, Name, GameID);
}

void game_sv_Freeplay::OnPlayerReady(ClientID id)
{
	if(m_phase == GAME_PHASE_INPROGRESS)
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
				
		if (CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(pOwner))		
			SpawnItemsForActor(pOwner, ps);
	}
}

void game_sv_Freeplay::SpawnItemsForActor(CSE_Abstract* pE, game_PlayerState* ps)
{
	CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(pE);
	R_ASSERT2(pA, "Owner not a Actor");
	if (!pA)
		return;

	if (!(ps->team < s16(TeamList.size())))
		return;

	SpawnWeapon4Actor(pA->ID, "mp_wpn_pm", 0);
	SpawnWeapon4Actor(pA->ID, "mp_wpn_abakan", 0);
	SpawnWeapon4Actor(pA->ID, "mp_medkit", 0);

	for (u32 i = 0; i < ps->pItemList.size(); i++)
	{
		u16 ItemID = ps->pItemList[i];
		SpawnWeapon4Actor(pA->ID, *m_strWeaponsData->GetItemName(ItemID & 0x00FF), u8((ItemID & 0xFF00) >> 0x08));
	}

	Player_AddMoney(ps, ps->LastBuyAcount);
}

void game_sv_Freeplay::OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed, KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA)
{
	ps_killed->setFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
	ps_killed->m_iDeaths++;
	ps_killed->m_iKillsInRowCurr = 0;
	ps_killed->DeathTime = Device.dwTimeGlobal;

	if (!ps_killer)	
		ps_killed->m_iSelfKills++;	

	SetPlayersDefItems(ps_killed);
	signal_Syncronize();
}
