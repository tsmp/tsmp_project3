#include "StdAfx.h"
#include "game_sv_freeplay.h"
#include "xrserver_objects_alife_monsters.h"
#include "ui/UIBuyWndShared.h"

ENGINE_API bool g_dedicated_server;

const std::vector<std::string> FreeplayDefaultRandomItems
{
	"mp_wpn_pm","mp_wpn_pb"
};

const std::vector<std::string> FreeplayDefaultSkins
{
	"stalker_killer_antigas"
};

const std::vector<std::string> FreeplayDefaultPersistentItems
{
	"mp_medkit","mp_wpn_knife", "mp_device_torch","mp_ammo_9x18_fmj","mp_ammo_9x18_fmj","mp_ammo_9x18_fmj"
};

const char* freeplaySkinsSection = "freeplay_skins";
const char* freeplayRandomItemsSection = "freeplay_random_weapons";
const char* freeplayPersistentItemsSection = "freeplay_persistent_items";

void game_sv_Freeplay::LoadSettings()
{
	if (pSettings->section_exist(freeplayRandomItemsSection))
	{
		CInifile::Sect &sect = pSettings->r_section(freeplayRandomItemsSection);

		for (CInifile::SectCIt I = sect.Data.begin(); I != sect.Data.end(); I++)
		{
			const CInifile::Item& item = *I;
			m_RandomItems.push_back(item.first.c_str());
		}

		Msg("- Loaded %u persistent items for freeplay", m_RandomItems.size());
	}

	if (m_RandomItems.empty())
	{
		Msg("- Cant find freeplay random weapons list, using default");
		m_RandomItems = FreeplayDefaultRandomItems;
	}

	if (pSettings->section_exist(freeplaySkinsSection))
	{
		CInifile::Sect& sect = pSettings->r_section(freeplaySkinsSection);

		for (CInifile::SectCIt I = sect.Data.begin(); I != sect.Data.end(); I++)
		{
			const CInifile::Item& item = *I;
			m_Skins.push_back(item.first.c_str());
		}

		Msg("- Loaded %u skins for freeplay", m_Skins.size());
	}

	if (m_Skins.empty())
	{
		Msg("- Cant find freeplay skins list, using default");
		m_Skins = FreeplayDefaultSkins;
	}

	if (pSettings->section_exist(freeplayPersistentItemsSection))
	{
		CInifile::Sect& sect = pSettings->r_section(freeplayPersistentItemsSection);

		for (CInifile::SectCIt I = sect.Data.begin(); I != sect.Data.end(); I++)
		{
			const CInifile::Item& item = *I;
			u32 count = 1;

			if (const char* ch = item.second.c_str())
				count = pSettings->r_u32(freeplayPersistentItemsSection, item.first.c_str());

			for (int i = 0; i < count; i++)
				m_PersistentItems.push_back(item.first.c_str());
		}

		Msg("- Loaded %u persistent items for freeplay", m_PersistentItems.size());
	}

	if (m_PersistentItems.empty())
	{
		Msg("- Cant find freeplay persistent items list, using default");
		m_PersistentItems = FreeplayDefaultPersistentItems;
	}
}

game_sv_Freeplay::game_sv_Freeplay() 
{
	m_phase = GAME_PHASE_NONE;
	m_type = GAME_FREEPLAY;	
}

game_sv_Freeplay::~game_sv_Freeplay() {}

void game_sv_Freeplay::Create(shared_str &options)
{
	inherited::Create(options);
	LoadSettings();

	TeamStruct NewTeam;
	for (std::string& skin : m_Skins)
		NewTeam.aSkins.push_back(skin.c_str());
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

void game_sv_Freeplay::SetStartMoney(ClientID const &id_who)
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

void game_sv_Freeplay::OnPlayerConnect(ClientID const &id_who)
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

void game_sv_Freeplay::OnPlayerConnectFinished(ClientID const &id_who)
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

void game_sv_Freeplay::OnEvent(NET_Packet &P, u16 type, u32 time, ClientID const &sender)
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

void game_sv_Freeplay::OnPlayerDisconnect(ClientID const &id_who, LPSTR Name, u16 GameID)
{
	inherited::OnPlayerDisconnect(id_who, Name, GameID);
}

void game_sv_Freeplay::OnPlayerReady(ClientID const &id)
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

	SpawnWeapon4Actor(pA->ID, m_RandomItems[m_ItemsRnd.randI(m_RandomItems.size())].c_str(), 0);

	for (std::string& itemName : m_PersistentItems)
		SpawnWeapon4Actor(pA->ID, itemName.c_str(), 0);
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
