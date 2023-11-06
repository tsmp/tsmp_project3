// xrServer.cpp: implementation of the xrServer class.
//
//////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer.h"
#include "xrMessages.h"
#include "xrServer_Objects_ALife_All.h"
#include "level.h"
#include "game_cl_base.h"
#include "ai_space.h"
#include "IGame_Persistent.h"
#include "screenshot_server.h"

#include "Console.h"
#include "ui/UIInventoryUtilities.h"
#include <functional>

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#include "../xrNetwork/PlayersBase.h"
#include "game_sv_mp.h"
#pragma warning(pop)

ENGINE_API const char* RadminIdPrefix;
static const int maxMessageLength = 140;

#pragma TODO("ѕосмотреть на MakeUpdatePackets в чн, возможно сделать так же")

xrClientData::xrClientData() : IClient(Device.GetTimerGlobal())
{
	ps = Level().Server->game->createPlayerState();
	ps->clear();
	ps->m_online_time = Level().timeServer();

	Clear();
}

void xrClientData::Clear()
{
	owner = nullptr;
	net_Ready = FALSE;
	net_Accepted = FALSE;
	net_PassUpdates = TRUE;
	m_ping_warn.m_maxPingWarnings = 0;
	m_ping_warn.m_dwLastMaxPingWarningTime = 0;
	m_admin_rights.m_has_admin_rights = FALSE;
	bMutedChat = false;
	verificationStepsCompleted = 0;
}

xrClientData::~xrClientData()
{
	xr_delete(ps);
}

xrServer::xrServer() : IPureServer(Device.GetTimerGlobal(), g_dedicated_server)
{
	m_file_transfers = nullptr;
	m_iCurUpdatePacket = 0;
	m_aUpdatePackets.push_back(NET_Packet());
	m_aDelayedPackets.clear();
}

extern void UnloadSysmsgsDLL();

xrServer::~xrServer()
{
	UnloadSysmsgsDLL();
	IClient *tmpClient = nullptr;

	do
	{
		client_Destroy(tmpClient);
		tmpClient = net_players.GetFirstClient(false);
	} 
	while (tmpClient);

	do
	{
		client_Destroy(tmpClient);
		tmpClient = net_players.GetFirstClient(true);
	} 
	while (tmpClient);

	m_aUpdatePackets.clear();
	m_aDelayedPackets.clear();
	entities.clear();
}

bool xrServer::HasBattlEye()
{
	return false;
}

CSE_Abstract *xrServer::ID_to_entity(u16 ID)
{
	// #pragma todo("??? to all : ID_to_entity - must be replaced to 'game->entity_from_eid()'")
	if (0xffff == ID)
		return 0;
	xrS_entities::iterator I = entities.find(ID);
	if (entities.end() != I)
		return I->second;
	else
		return 0;
}

IClient *xrServer::client_Create()
{
	return xr_new<xrClientData>();
}

void xrServer::client_Replicate() {}

IClient *xrServer::client_Find_Get(ClientID const &ID)
{
	ip_address cAddress;
	DWORD dwPort = 0;

	if (!psNET_direct_connect)
		GetClientAddress(ID, cAddress, &dwPort);
	else
		cAddress.set("127.0.0.1");

	if (!psNET_direct_connect)
	{
		IClient* disconnectedClient = net_players.FindAndEraseDisconnectedClient([&cAddress](IClient* client)
		{
			return client->m_cAddress == cAddress;
		});

		if (disconnectedClient)
		{
			disconnectedClient->m_dwPort = dwPort;
			disconnectedClient->flags.bReconnect = TRUE;
			disconnectedClient->server = this;
			net_players.AddNewClient(disconnectedClient);
			Msg("# Player found");

			auto data = static_cast<xrClientData*>(disconnectedClient);
			data->ps->m_online_time = Level().timeServer();

			data->ps->m_StatsBeforeDisconnect = data->ps->m_Stats;
			data->ps->m_WpnHits.clear();
			data->ps->m_WpnIdx.clear();

			return disconnectedClient;
		}
	}

	IClient *newCL = client_Create();
	newCL->ID = ID;

	if (!psNET_direct_connect)
	{
		newCL->m_cAddress = cAddress;
		newCL->m_dwPort = dwPort;
	}

	newCL->server = this;
	net_players.AddNewClient(newCL);

	Msg("# Player not found. New player created.");
	return newCL;
};

void xrServer::ReportClientStats(IClient *CL)
{
	PlayerGameStats stats{ 0 };
	const game_PlayerState* state = static_cast<xrClientData*>(CL)->ps;
	CollectedStatistic clStat = state->m_Stats - state->m_StatsBeforeDisconnect;

	stats.fragsCnt = clStat.m_iRivalKills;
	stats.fragsNpc = clStat.m_iAiKills;
	stats.headShots = clStat.m_iHeadshots;
	stats.deathsCnt = clStat.m_iDeaths;
	stats.artsCnt = clStat.af_count;
	stats.maxFragsOneLife = clStat.m_iKillsInRowMax;
	stats.timeInGame = (Level().timeServer() - state->m_online_time) / 60000; // minutes

	if (auto gameMp = smart_cast<game_sv_mp*>(game))
		gameMp->GetPlayerWpnStats(state, stats.weaponNames, stats.hitsCount);

	ReportPlayerStats(CL, stats);
}

INT g_sv_Client_Reconnect_Time = 0;

void xrServer::client_Destroy(IClient* C)
{
	if (!C)
		return;

	auto predicate = std::bind(std::equal_to<IClient*>(), std::placeholders::_1, C);

	if(IClient *erasedDisconnectedCL = net_players.FindAndEraseDisconnectedClient(predicate))
		xr_delete(erasedDisconnectedCL);
	
	IClient *aliveClient = net_players.FindAndEraseClient(predicate);

	if (!aliveClient)
		return;

	CSE_Abstract *pOwner = static_cast<xrClientData*>(aliveClient)->owner;
		
	if (CSE_Spectator *pS = smart_cast<CSE_Spectator*>(pOwner))
	{
		NET_Packet P;
		P.w_begin(M_EVENT);
		P.w_u32(Level().timeServer());
		P.w_u16(GE_DESTROY);
		P.w_u16(pS->ID);
		Level().Send(P, net_flags(TRUE, TRUE));
	}

	DelayedPacket pp;
	pp.SenderID = aliveClient->ID;

	do
	{
		auto it = std::find(m_aDelayedPackets.begin(), m_aDelayedPackets.end(), pp);

		if (it != m_aDelayedPackets.end())
		{
			m_aDelayedPackets.erase(it);
			Msg("removing packet from delayed event storage");
		}
		else
			break;
	} 
	while (true);

	if(game)
		game->CleanClientData(static_cast<xrClientData*>(aliveClient));

#pragma TODO("—делать как в чн/зп")
	//if (pOwner)
	//{
	//	game->CleanDelayedEventFor(pOwner->ID);
	//}

	if (!g_sv_Client_Reconnect_Time)
		xr_delete(aliveClient);
	else
	{
		aliveClient->dwTime_LastUpdate = Device.dwTimeGlobal;
		net_players.AddNewDisconnectedClient(aliveClient);
		static_cast<xrClientData*>(aliveClient)->Clear();
	}
}

int g_Dump_Update_Write = 0;

#ifdef DEBUG
INT g_sv_SendUpdate = 0;
#endif

void xrServer::Update()
{
	NET_Packet Packet;
	DEBUG_VERIFY(verify_entities());

	ProceedDelayedPackets();
	// game update
	game->ProcessDelayedEvent();
	game->Update();

	// spawn queue
	u32 svT = Device.TimerAsync();
	while (!(q_respawn.empty() || (svT < q_respawn.begin()->timestamp)))
	{
		// get
		svs_respawn R = *q_respawn.begin();
		q_respawn.erase(q_respawn.begin());

		//
		CSE_Abstract *E = ID_to_entity(R.phantom);
		E->Spawn_Write(Packet, FALSE);
		u16 ID;
		Packet.r_begin(ID);
		R_ASSERT(M_SPAWN == ID);
		ClientID clientID;
		clientID.set(0xffff);
		Process_spawn(Packet, clientID);
	}

	SendUpdatesToAll();

	if (game->sv_force_sync)
		Perform_game_export();

	DEBUG_VERIFY(verify_entities());

	//Remove any of long time disconnected players
	IClient* tmpClient = nullptr;

	do
	{
		client_Destroy(tmpClient);
		tmpClient = net_players.GetFoundDisconnectedClient([](IClient* client)
		{
			if (client->dwTime_LastUpdate + (g_sv_Client_Reconnect_Time * 60000) < Device.dwTimeGlobal)
				return true;

			return false;
		});
	} 
	while (tmpClient);

	PerformCheckClientsForMaxPing();
	Flush_Clients_Buffers();

	if (Device.CurrentFrameNumber % 1000 == 0) // once per 1000 frames
		BannedListUpdate();
}

void xrServer::SendUpdateTo(IClient* client)
{
	xrClientData* xrClient = static_cast<xrClientData*>(client);

	if (!xrClient->net_Ready)
		return;

	if (!HasBandwidth(xrClient)
#ifdef DEBUG
		&& !g_sv_SendUpdate
#endif
		)
		return;

	NET_Packet playersStateUpdatePacket;
	playersStateUpdatePacket.w_begin(M_UPDATE);
	game->net_Export_Update(playersStateUpdatePacket, xrClient->ID);

#pragma TODO("check why in cs there is no bLocal check with return")

	if (xrClient->flags.bLocal) // this is server client;
	{
		SendTo(xrClient->ID, playersStateUpdatePacket, net_flags(FALSE, TRUE));
		return;
	}

	if (!IsUpdatePacketsReady())
		MakeUpdatePackets(xrClient, playersStateUpdatePacket.B.count);

	WriteAtUpdateBegining(playersStateUpdatePacket);

	// Send relevant entities to client
	SendUpdatePacketsToClient(xrClient);
}

bool xrServer::IsUpdatePacketsReady()
{
	return m_aUpdatePackets[0].B.count != 0;
}

// ¬ начале первого пакета с обновлением должен идти стейт текушего игрока. —ейчас в m_aUpdatePackets стейт от другого игрока.
// ѕерезапишем в начало поверх него стейт от текущего игрока из P. Ѕлаго размеры стейтов у всех одинаковые.
void xrServer::WriteAtUpdateBegining(NET_Packet const& P)
{
	m_aUpdatePackets[0].w_seek(0, P.B.data, P.B.count);
}

void xrServer::MakeUpdatePackets(xrClientData* xr_client, u32 writeOffset)
{
	NET_Packet* pCurUpdatePacket = &(m_aUpdatePackets[0]);
	pCurUpdatePacket->B.count = writeOffset;

	if (g_Dump_Update_Write)
	{
		if (xr_client->ps)
			Msg("---- UPDATE_Write to %s --- ", xr_client->ps->getName());
		else
			Msg("---- UPDATE_Write to %s --- ", *(xr_client->name));
	}

	// all entities
	for (auto& pair : entities)
	{
		CSE_Abstract& entity = *pair.second;

		if (!entity.owner || !entity.net_Ready || entity.s_flags.is(M_SPAWN_OBJECT_PHANTOM) || !entity.Net_Relevant())
			continue;

		NET_Packet tmpPacket;
		tmpPacket.B.count = 0;

		// write specific data
		u32 position;
		tmpPacket.w_u16(entity.ID);
		tmpPacket.w_chunk_open8(position);
		entity.UPDATE_Write(tmpPacket);
		u32 ObjectSize = u32(tmpPacket.w_tell() - position) - sizeof(u8);
		tmpPacket.w_chunk_close8(position);

		if (!ObjectSize)
			continue;
#ifdef DEBUG
		if (g_Dump_Update_Write)
			Msg("* %s : %d", Test.name(), ObjectSize);
#endif

		if (pCurUpdatePacket->B.count + tmpPacket.B.count >= NET_PacketSizeLimit)
		{
			m_iCurUpdatePacket++;

			if (m_aUpdatePackets.size() == m_iCurUpdatePacket)
				m_aUpdatePackets.push_back(NET_Packet());

			pCurUpdatePacket = &(m_aUpdatePackets[m_iCurUpdatePacket]);
			pCurUpdatePacket->w_begin(M_UPDATE_OBJECTS);
		}

		pCurUpdatePacket->w(tmpPacket.B.data, tmpPacket.B.count);
	}
}

void xrServer::SendUpdatePacketsToClient(xrClientData* xr_client)
{
	if (g_Dump_Update_Write)
		Msg("----------------------- ");

	for (u32 p = 0; p <= m_iCurUpdatePacket; p++)
	{
		NET_Packet &ToSend = m_aUpdatePackets[p];

		if (ToSend.B.count > 2)
		{
			if (g_Dump_Update_Write && xr_client->ps != NULL)
			{
				Msg("- Server Update[%d] to Client[%s]  : %d",
					*((u16 *)ToSend.B.data),
					xr_client->ps->getName(),
					ToSend.B.count);
			}

			SendTo(xr_client->ID, ToSend, net_flags(FALSE, TRUE));
		}
	}
}

void xrServer::SendUpdatesToAll()
{
	m_iCurUpdatePacket = 0;
	m_aUpdatePackets[0].B.count = 0;

	ForEachClientDoSender([this](IClient* cl)
	{
		SendUpdateTo(cl);
	});

	if (m_file_transfers)
	{
		m_file_transfers->update_transfer();
		m_file_transfers->stop_obsolete_receivers();
	}

#ifdef DEBUG
	g_sv_SendUpdate = 0;
#endif

	if (game->sv_force_sync)
		Perform_game_export();

	DEBUG_VERIFY(verify_entities());
}

xr_vector<shared_str> _tmp_log;
void console_log_cb(LPCSTR text)
{
	_tmp_log.push_back(text);
}

void xrServer::OnRadminCommand(xrClientData* CL, NET_Packet &P, ClientID const &sender)
{
	NET_Packet answerP;

	if (!CL->m_admin_rights.m_has_admin_rights)
	{
		answerP.w_begin(M_REMOTE_CONTROL_CMD);
		answerP.w_stringZ("you dont have admin rights");
		SendTo(sender, answerP, net_flags(TRUE, TRUE));
		return;
	}

	string128 buffer;
	P.r_stringZ(buffer);

	string128 command;
	sprintf_s(command, 128, "%s %s%u", buffer, RadminIdPrefix, sender.value());

	Msg("# radmin %s is running command: %s", CL->m_admin_rights.m_Login.c_str(), buffer);
	_tmp_log.clear();

	SetLogCB(console_log_cb);
	Console->Execute(command);
	SetLogCB(nullptr);

	for (u32 i = 0; i < _tmp_log.size(); ++i)
	{
		answerP.w_begin(M_REMOTE_CONTROL_CMD);
		answerP.w_stringZ(_tmp_log[i]);
		SendTo(sender, answerP, net_flags(TRUE, TRUE));
	}
}

u32 xrServer::OnDelayedMessage(NET_Packet &P, ClientID const &sender) // Non-Zero means broadcasting with "flags" as returned
{
	if (g_pGameLevel && Level().IsDemoSave())
		Level().Demo_StoreServerData(P.B.data, P.B.count);

	u16 type;
	P.r_begin(type);

	DEBUG_VERIFY(verify_entities());
	xrClientData *CL = ID_to_client(sender);
	R_ASSERT2(CL, make_string("packet type [%d]", type).c_str());

	switch (type)
	{
	case M_CLIENT_REQUEST_CONNECTION_DATA:	
		OnCL_Connected(CL);
		break;

	case M_REMOTE_CONTROL_CMD:
		OnRadminCommand(CL, P, sender);
		break;

	case M_FILE_TRANSFER:	
		m_file_transfers->on_message(&P, sender);
		break;
	}

	DEBUG_VERIFY(verify_entities());
	return 0;
}

#pragma TODO("ѕосмотреть зачем в чн OnMessageSync")
extern float g_fCatchObjectTime;

u32 xrServer::OnMessage(NET_Packet &P, ClientID const &sender) // Non-Zero means broadcasting with "flags" as returned
{
	if (g_pGameLevel && Level().IsDemoSave())
		Level().Demo_StoreServerData(P.B.data, P.B.count);
	u16 type;
	P.r_begin(type);

	DEBUG_VERIFY(verify_entities());
	xrClientData *CL = ID_to_client(sender);

	switch (type)
	{
	case M_UPDATE:
	{
		Process_update(P, sender); // No broadcast
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_SPAWN:
	{
		if (CL->flags.bLocal)
			Process_spawn(P, sender);

		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_EVENT:
	{
		Process_event(P, sender);
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_FILE_TRANSFER:
	{
		AddDelayedPacket(P, sender);
	}break;
	case M_EVENT_PACK:
	{
		NET_Packet tmpP;
		while (!P.r_eof())
		{
			tmpP.B.count = P.r_u8();
			P.r(&tmpP.B.data, tmpP.B.count);

			OnMessage(tmpP, sender);
		};
	}
	break;
	case M_CL_UPDATE:
	{
		xrClientData *CL = ID_to_client(sender);
		if (!CL)
			break;
		CL->net_Ready = TRUE;

		if (!CL->net_PassUpdates)
			break;
	
		u32 ClientPing = CL->stats.getPing();
		P.w_seek(P.r_tell() + 2, &ClientPing, 4);
		
		if (SV_Client)
			SendTo(SV_Client->ID, P, net_flags(TRUE, TRUE));
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_MOVE_PLAYERS_RESPOND:
	{
		xrClientData *CL = ID_to_client(sender);
		if (!CL)
			break;
		CL->net_Ready = TRUE;
		CL->net_PassUpdates = TRUE;
	}
	break;
	case M_CL_INPUT:
	{
		xrClientData *CL = ID_to_client(sender);
		if (CL)
			CL->net_Ready = TRUE;
		if (SV_Client)
			SendTo(SV_Client->ID, P, net_flags(TRUE, TRUE));
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_GAMEMESSAGE:
	{
		SendBroadcast(BroadcastCID, P, net_flags(TRUE, TRUE));
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_CLIENTREADY:
	{
		xrClientData *CL = ID_to_client(sender);
		if (CL)
		{
			CL->net_Ready = TRUE;
			CL->ps->DeathTime = Device.dwTimeGlobal;
			game->OnPlayerConnectFinished(sender);
			CL->ps->setName(CL->name.c_str());
		};
		game->signal_Syncronize();
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_SWITCH_DISTANCE:
	{
		game->switch_distance(P, sender);
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_CHANGE_LEVEL:
	{
		if (game->change_level(P, sender))
		{
			SendBroadcast(BroadcastCID, P, net_flags(TRUE, TRUE));
		}
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_SAVE_GAME:
	{
		game->save_game(P, sender);
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_LOAD_GAME:
	{
		game->load_game(P, sender);
		SendBroadcast(BroadcastCID, P, net_flags(TRUE, TRUE));
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_RELOAD_GAME:
	{
		SendBroadcast(BroadcastCID, P, net_flags(TRUE, TRUE));
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_SAVE_PACKET:
	{
		Process_save(P, sender);
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case M_CLIENT_REQUEST_CONNECTION_DATA:
	{
		AddDelayedPacket(P, sender);
	}
	break;
	case M_CHAT_MESSAGE:
	{
		xrClientData *l_pC = ID_to_client(sender);
		OnChatMessage(&P, l_pC);
	}
	break;
	case M_CHANGE_LEVEL_GAME:
	{
		ClientID CID;
		CID.set(0xffffffff);
		SendBroadcast(CID, P, net_flags(TRUE, TRUE));
	}
	break;
	case M_CL_AUTH:
	{
		game->AddDelayedEvent(P, GAME_EVENT_PLAYER_AUTH, 0, sender);
	}
	break;

	case M_STATISTIC_UPDATE:
	{
		if (SV_Client)
			SendBroadcast(SV_Client->ID, P, net_flags(TRUE, TRUE));
		else
			SendBroadcast(BroadcastCID, P, net_flags(TRUE, TRUE));
	}
	break;

	case M_UID_RESPOND:
		game->AddDelayedEvent(P, GAME_EVENT_PLAYER_AUTH_UID, 0, sender);
		break;

	case M_STATISTIC_UPDATE_RESPOND:
	{
		if (SV_Client)
			SendTo(SV_Client->ID, P, net_flags(TRUE, TRUE));
	}
	break;
	case M_PLAYER_FIRE:
	{
		if (game)
			game->OnPlayerFire(sender, P);
	}
	break;
	case M_REMOTE_CONTROL_AUTH:
	{
		string512 reason;
		shared_str user;
		shared_str pass;
		P.r_stringZ(user);

		if (0 == stricmp(user.c_str(), "logoff"))
		{
			CL->m_admin_rights.m_has_admin_rights = FALSE;
			strcpy(reason, "logged off");
			Msg("# Remote administrator [%s] logged off.", CL->m_admin_rights.m_Login.c_str());
		}
		else
		{
			P.r_stringZ(pass);
			bool res = CheckAdminRights(user, pass, reason);

			if (res)
			{
				CL->m_admin_rights.m_has_admin_rights = TRUE;
				CL->m_admin_rights.m_dwLoginTime = Device.dwTimeGlobal;
				CL->m_admin_rights.m_Login = user;
				Msg("# Player [%s] logged as remote administrator with login [%s].", CL->ps->getName(), user.c_str());
			}
			else
				Msg("! Player [%s] tried to login as remote administrator with login [%s]. Access denied.", CL->ps->getName(), user.c_str());
		}

		NET_Packet P_answ;
		P_answ.w_begin(M_REMOTE_CONTROL_CMD);
		P_answ.w_stringZ(reason);
		SendTo(CL->ID, P_answ, net_flags(TRUE, TRUE));
	}
	break;

	case M_REMOTE_CONTROL_CMD:
	{
		AddDelayedPacket(P, sender);
	}
	break;

	case M_BATTLEYE:
		break;

	case M_VOICE_MESSAGE:	
		OnVoiceMessage(P, sender);
		break;
	}

	DEBUG_VERIFY(verify_entities());
	return IPureServer::OnMessage(P, sender);
}

bool xrServer::CheckAdminRights(const shared_str &user, const shared_str &pass, string512 reason)
{
	bool res = false;
	string_path fn;
	FS.update_path(fn, "$app_data_root$", "radmins.ltx");
	if (FS.exist(fn))
	{
		CInifile ini(fn);
		if (ini.line_exist("radmins", user.c_str()))
		{
			if (ini.r_string("radmins", user.c_str()) == pass)
			{
				strcpy(reason, "Access permitted.");
				res = true;
			}
			else
			{
				strcpy(reason, "Access denied. Wrong password.");
			}
		}
		else
			strcpy(reason, "Access denied. No such user.");
	}
	else
		strcpy(reason, "Access denied.");

	return res;
}

void xrServer::SendTo_LL(ClientID const &ID, void *data, u32 size, u32 dwFlags, u32 dwTimeout)
{
	if (SV_Client && SV_Client->ID == ID)
	{
		// optimize local traffic
		Level().OnMessage(data, size);
	}
	else
	{
		IClient *pClient = ID_to_client(ID);
		if (!pClient || !pClient->flags.bConnected)
			return;

		IPureServer::SendTo_Buf(ID, data, size, dwFlags, dwTimeout);
	}
}

CSE_Abstract *xrServer::entity_Create(LPCSTR name)
{
	return F_entity_Create(name);
}

void xrServer::entity_Destroy(CSE_Abstract *&P)
{
#ifdef DEBUG
	Msg("xrServer::entity_Destroy : [%d][%s][%s]", P->ID, P->name(), P->name_replace());
#endif
	R_ASSERT(P);
	entities.erase(P->ID);
	m_tID_Generator.vfFreeID(P->ID, Device.TimerAsync());

	if (P->owner && P->owner->owner == P)
		P->owner->owner = NULL;

	P->owner = NULL;
	if (!ai().get_alife() || !P->m_bALifeControl)
	{
		F_entity_Destroy(P);
	}
}

void xrServer::Server_Client_Check(IClient *CL)
{
	if (SV_Client && SV_Client->ID == CL->ID)
	{
		if (!CL->flags.bConnected)
			SV_Client = nullptr;		
		return;
	}

	if (SV_Client && SV_Client->ID != CL->ID)	
		return;	

	if (!CL->flags.bConnected)	
		return;
	
	if (CL->process_id == GetCurrentProcessId())
	{
		CL->flags.bLocal = 1;
		SV_Client = (xrClientData *)CL;
		Msg("New SV client %s", SV_Client->name.c_str());
	}
	else
		CL->flags.bLocal = 0;
}

bool xrServer::OnCL_QueryHost()
{
	if (game->Type() == GAME_SINGLE)
		return false;
	return (GetClientsCount() != 0);
};

CSE_Abstract *xrServer::GetEntity(u32 Num)
{
	xrS_entities::iterator I = entities.begin(), E = entities.end();
	for (u32 C = 0; I != E; ++I, ++C)
	{
		if (C == Num)
			return I->second;
	};
	return NULL;
}

void xrServer::initialize_screenshot_proxies()
{
	for (int i = 0; i < sizeof(m_screenshot_proxies) / sizeof(clientdata_proxy*); ++i)
	{
		m_screenshot_proxies[i] = xr_new<clientdata_proxy>(m_file_transfers);
	}
}

void xrServer::deinitialize_screenshot_proxies()
{
	for (int i = 0; i < sizeof(m_screenshot_proxies) / sizeof(clientdata_proxy*); ++i)
	{
		xr_delete(m_screenshot_proxies[i]);
	}
}

void xrServer::MakeScreenshot(ClientID const& admin_id, ClientID const& cheater_id)
{
	if ((cheater_id == SV_Client->ID) && g_dedicated_server)	
		return;
	
	for (int i = 0; i < sizeof(m_screenshot_proxies) / sizeof(clientdata_proxy*); ++i)
	{
		if (!m_screenshot_proxies[i]->is_active())
		{
			m_screenshot_proxies[i]->make_screenshot(admin_id, cheater_id);
			Msg("* admin [%d] is making screeshot of client [%d]", admin_id, cheater_id);
			return;
		}
	}

	Msg("! ERROR: SV: not enough file transfer proxies for downloading screenshot, please try later ...");
}

void xrServer::OnChatMessage(NET_Packet *P, xrClientData *CL)
{
	if (!CL->net_Ready || CL->bMutedChat)
		return;

#pragma TODO("TSMP: если будет скучно можно тут провер€ть еще им€ и команду :)")
	string128 playerName;
	s16 senderTeam = P->r_s16();	
	P->r_stringZ(playerName);

	char* pChatMessage = reinterpret_cast<char*>(&P->B.data[P->r_pos]);	
	int chatMessageLength = static_cast<int>(strnlen_s(pChatMessage, maxMessageLength));

	if (chatMessageLength == maxMessageLength)
	{
		Msg("! WARNING: player [%s] sent chat message with length more than limit!", playerName);
		return;
	}

	for (int i = 0; i < chatMessageLength; i++)
	{
		if (pChatMessage[i] == '%')
		{
			pChatMessage[i] = '_';
			Msg("! WARNING: player [%s] tried to use %% in chat!", playerName);
		}
	}

	ForEachClientDoSender([&](IClient *client)
	{
		xrClientData* xrClient = static_cast<xrClientData*>(client);
		game_PlayerState* ps = xrClient->ps;

		if (!ps)
			return;

		if (!xrClient->net_Ready)
			return;

		if (senderTeam != 0 && ps->team != senderTeam && client != GetServerClient())
			return;

		SendTo(client->ID, *P);
	});
}

void xrServer::OnVoiceMessage(NET_Packet& P, ClientID sender)
{
	const xrClientData* pClientSender = ID_to_client(sender);

	if (!pClientSender || !pClientSender->net_Ready)
		return;
	
	const game_PlayerState* psSender = pClientSender->ps;
	
	if (!psSender || !pClientSender->owner)
		return;

	//Msg("VoiceMessage size: %u", P.B.count);

	const float distance = P.r_u8();
	const float voiceDistanceSqr = distance * distance;
	const Fvector senderPos = pClientSender->owner->Position();

	ForEachClientDo([&](IClient* client)
	{
		if (g_dedicated_server && client == GetServerClient())
			return;

		const auto CL = static_cast<xrClientData*>(client);
		if (!CL || !CL->net_Ready || !CL->owner || CL->ID == pClientSender->ID)
			return;

		const game_PlayerState* ps = CL->ps;
		if (!ps || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
			return;

		const float distanceSqr = CL->owner->Position().distance_to_sqr(senderPos);
		if (fis_zero(voiceDistanceSqr) || distanceSqr <= voiceDistanceSqr)
			SendTo(CL->ID, P, net_flags(FALSE, TRUE, TRUE, TRUE));
	});
};

#ifdef DEBUG

static BOOL _ve_initialized = FALSE;
static BOOL _ve_use = TRUE;

bool xrServer::verify_entities() const
{
	if (!_ve_initialized)
	{
		_ve_initialized = TRUE;
		if (strstr(Core.Params, "-~ve"))
			_ve_use = FALSE;
	}
	if (!_ve_use)
		return true;

	xrS_entities::const_iterator I = entities.begin();
	xrS_entities::const_iterator E = entities.end();
	for (; I != E; ++I)
	{
		VERIFY2((*I).first != 0xffff, "SERVER : Invalid entity id as a map key - 0xffff");
		VERIFY2((*I).second, "SERVER : Null entity object in the map");
		VERIFY3((*I).first == (*I).second->ID, "SERVER : ID mismatch - map key doesn't correspond to the real entity ID", (*I).second->name_replace());
		verify_entity((*I).second);
	}
	return (true);
}

void xrServer::verify_entity(const CSE_Abstract *entity) const
{
	VERIFY(entity->m_wVersion != 0);
	if (entity->ID_Parent != 0xffff)
	{
		xrS_entities::const_iterator J = entities.find(entity->ID_Parent);
		VERIFY3(J != entities.end(), "SERVER : Cannot find parent in the map", entity->name_replace());
		VERIFY3((*J).second, "SERVER : Null entity object in the map", entity->name_replace());
		VERIFY3((*J).first == (*J).second->ID, "SERVER : ID mismatch - map key doesn't correspond to the real entity ID", (*J).second->name_replace());
		VERIFY3(std::find((*J).second->children.begin(), (*J).second->children.end(), entity->ID) != (*J).second->children.end(), "SERVER : Parent/Children relationship mismatch - Object has parent, but corresponding parent doesn't have children", (*J).second->name_replace());
	}

	xr_vector<u16>::const_iterator I = entity->children.begin();
	xr_vector<u16>::const_iterator E = entity->children.end();
	for (; I != E; ++I)
	{
		VERIFY3(*I != 0xffff, "SERVER : Invalid entity children id - 0xffff", entity->name_replace());
		xrS_entities::const_iterator J = entities.find(*I);
		VERIFY3(J != entities.end(), "SERVER : Cannot find children in the map", entity->name_replace());
		VERIFY3((*J).second, "SERVER : Null entity object in the map", entity->name_replace());
		VERIFY3((*J).first == (*J).second->ID, "SERVER : ID mismatch - map key doesn't correspond to the real entity ID", (*J).second->name_replace());
		VERIFY3((*J).second->ID_Parent == entity->ID, "SERVER : Parent/Children relationship mismatch - Object has children, but children doesn't have parent", (*J).second->name_replace());
	}
}

#endif // DEBUG

shared_str xrServer::level_name(const shared_str &server_options) const
{
	return (game->level_name(server_options));
}

void xrServer::create_direct_client()
{
	SClientConnectData cl_data;
	cl_data.clientID.set(1);
	strcpy_s(cl_data.name, "single_player");
	cl_data.process_id = GetCurrentProcessId();

	new_client(&cl_data);
}

void xrServer::ProceedDelayedPackets()
{
	DelayedPackestCS.Enter();
	while (!m_aDelayedPackets.empty())
	{
		DelayedPacket &DPacket = *m_aDelayedPackets.begin();
		OnDelayedMessage(DPacket.Packet, DPacket.SenderID);
		//		OnMessage(DPacket.Packet, DPacket.SenderID);
		m_aDelayedPackets.pop_front();
	}
	DelayedPackestCS.Leave();
};

void xrServer::AddDelayedPacket(NET_Packet &Packet, ClientID const &Sender)
{
	DelayedPackestCS.Enter();

	m_aDelayedPackets.push_back(DelayedPacket());
	DelayedPacket *NewPacket = &(m_aDelayedPackets.back());
	NewPacket->SenderID = Sender;
	CopyMemory(&(NewPacket->Packet), &Packet, sizeof(NET_Packet));

	DelayedPackestCS.Leave();
}

u32 g_sv_dwMaxClientPing = 2000;
u32 g_sv_time_for_ping_check = 15000; // 15 sec
u8 g_sv_maxPingWarningsCount = 5;

void xrServer::PerformCheckClientsForMaxPing()
{
	ForEachClientDoSender([&](IClient* client)
	{
		xrClientData* Client = static_cast<xrClientData*>(client);
		game_PlayerState* ps = Client->ps;
		if (!ps)
			return;

		if (ps->ping <= g_sv_dwMaxClientPing)
			return;

		if (Client->m_ping_warn.m_dwLastMaxPingWarningTime + g_sv_time_for_ping_check >= Device.dwTimeGlobal)
			return;

		++Client->m_ping_warn.m_maxPingWarnings;
		Client->m_ping_warn.m_dwLastMaxPingWarningTime = Device.dwTimeGlobal;

		if (Client->m_ping_warn.m_maxPingWarnings >= g_sv_maxPingWarningsCount)				 
			Level().Server->DisconnectClient(Client); //kick				
		else
		{ 
			NET_Packet P;
			P.w_begin(M_CLIENT_WARN);
			P.w_u8(1); // 1 means max-ping-warning
			P.w_u16(ps->ping);
			P.w_u8(Client->m_ping_warn.m_maxPingWarnings);
			P.w_u8(g_sv_maxPingWarningsCount);
			SendTo(Client->ID, P, net_flags(FALSE, TRUE));
		}			
	});
}

extern s32 g_sv_dm_dwFragLimit;
extern s32 g_sv_ah_dwArtefactsNum;
extern s32 g_sv_dm_dwTimeLimit;
extern int g_sv_ah_iReinforcementTime;

xr_token game_types[];
void xrServer::GetServerInfo(CServerInfo *si)
{
	string32 tmp;
	string256 tmp256;

	si->AddItem("Server port", itoa(GetPort(), tmp, 10), RGB(128, 128, 255));
	LPCSTR time = InventoryUtilities::GetTimeAsString(Device.dwTimeGlobal, InventoryUtilities::etpTimeToSecondsAndDay).c_str();

	int iFps = -1;
	if (Device.fTimeDelta > EPS_S)
	{
		float fps = 1.f / Device.fTimeDelta;
		iFps = fps;
	}

	si->AddItem("Server FPS: ", itoa(iFps, tmp, 10), RGB(255, 228, 0));

	si->AddItem("Uptime", time, RGB(255, 228, 0));

	strcpy_s(tmp256, get_token_name(game_types, game->Type()));
	if (game->Type() == GAME_DEATHMATCH || game->Type() == GAME_TEAMDEATHMATCH)
	{
		strcat_s(tmp256, " [");
		strcat_s(tmp256, itoa(g_sv_dm_dwFragLimit, tmp, 10));
		strcat_s(tmp256, "] ");
	}
	else if (game->Type() == GAME_ARTEFACTHUNT)
	{
		strcat_s(tmp256, " [");
		strcat_s(tmp256, itoa(g_sv_ah_dwArtefactsNum, tmp, 10));
		strcat_s(tmp256, "] ");
		g_sv_ah_iReinforcementTime;
	}

	//if ( g_sv_dm_dwTimeLimit > 0 )
	{
		strcat_s(tmp256, " time limit [");
		strcat_s(tmp256, itoa(g_sv_dm_dwTimeLimit, tmp, 10));
		strcat_s(tmp256, "] ");
	}
	if (game->Type() == GAME_ARTEFACTHUNT)
	{
		strcat_s(tmp256, " RT [");
		strcat_s(tmp256, itoa(g_sv_ah_iReinforcementTime, tmp, 10));
		strcat_s(tmp256, "]");
	}
	si->AddItem("Game type", tmp256, RGB(128, 255, 255));
}

IClient *xrServer::GetClientByName(const char* name)
{
	string64 playerName;
	strncpy_s(playerName, sizeof(playerName), name, sizeof(playerName) - 1);
	xr_strlwr(playerName);

	return FindClient([&playerName](IClient* client)
	{
		auto cl = static_cast<xrClientData*>(client);
		return !xr_strcmp(playerName, cl->ps->getName());
	});
}
