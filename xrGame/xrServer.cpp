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
#pragma warning(pop)

ENGINE_API const char* RadminIdPrefix;

#pragma TODO("Посмотреть на MakeUpdatePackets в чн, возможно сделать так же")

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

	struct ClientDestroyer
	{
		static bool true_generator(IClient*)
		{
			return true;
		}
	};

	IClient* tmp_client = net_players.GetFoundClient(&ClientDestroyer::true_generator);
	while (tmp_client)
	{
		client_Destroy(tmp_client);
		tmp_client = net_players.GetFoundClient(&ClientDestroyer::true_generator);
	}

	tmp_client = net_players.GetFoundDisconnectedClient(&ClientDestroyer::true_generator);
	while (tmp_client)
	{
		client_Destroy(tmp_client);
		tmp_client = net_players.GetFoundDisconnectedClient(&ClientDestroyer::true_generator);
	}
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

IClient *xrServer::client_Find_Get(ClientID ID)
{
	struct AddressSearcherPredicate
	{
		ip_address m_cAddress;
		bool operator()(IClient* client)
		{
			return client->m_cAddress == m_cAddress;
		};
	};
	AddressSearcherPredicate	search_predicate;

	ip_address cAddress;
	DWORD dwPort = 0;

	if (!psNET_direct_connect)
		GetClientAddress(ID, search_predicate.m_cAddress, &dwPort);
	else
		search_predicate.m_cAddress.set("127.0.0.1");

	if (!psNET_direct_connect)
	{
		IClient* disconnected_client = net_players.FindAndEraseDisconnectedClient(search_predicate);
		if (disconnected_client)
		{
			disconnected_client->m_dwPort = dwPort;
			disconnected_client->flags.bReconnect = TRUE;
			disconnected_client->server = this;
			net_players.AddNewClient(disconnected_client);
			Msg("# Player found");
			return disconnected_client;
		}
	};

	IClient *newCL = client_Create();
	newCL->ID = ID;
	if (!psNET_direct_connect)
	{
		newCL->m_cAddress = search_predicate.m_cAddress;
		newCL->m_dwPort = dwPort;
	}

	newCL->server = this;
	net_players.AddNewClient(newCL);

	Msg("# Player not found. New player created.");
	return newCL;
};

INT g_sv_Client_Reconnect_Time = 0;

void xrServer::client_Destroy(IClient* C)
{
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
		SendBroadcast(C->ID, P, net_flags(TRUE, TRUE));
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

#pragma TODO("Сделать как в чн/зп если это еще нужно")
	//if (pOwner)
	//{
	//	game->CleanDelayedEventFor(pOwner->ID);
	//}

	if (!g_sv_Client_Reconnect_Time || !aliveClient->flags.bVerified)	
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

	struct LongTimeClient
	{
		static bool Searher(IClient* client)
		{
			if (client->dwTime_LastUpdate + (g_sv_Client_Reconnect_Time * 60000) < Device.dwTimeGlobal)
				return true;
			return false;
		}
	};

	IClient* tmp_client = net_players.GetFoundDisconnectedClient(LongTimeClient::Searher);

	while (tmp_client)
	{
		client_Destroy(tmp_client);
		tmp_client = net_players.GetFoundDisconnectedClient(
			LongTimeClient::Searher);
	}

	PerformCheckClientsForMaxPing();
	Flush_Clients_Buffers();

	if (0 == (Device.CurrentFrameNumber % 100)) //once per 100 frames
	{
		UpdateBannedList();
	}
}

void xrServer::SendUpdatesToAll()
{
	m_iCurUpdatePacket = 0;
	NET_Packet *pCurUpdatePacket = &(m_aUpdatePackets[0]);
	pCurUpdatePacket->B.count = 0;
	u32 position;

#pragma TODO("Сделать как в чн/зп, вынести в функцию")

	ForEachClientDoSender([this, &position, &pCurUpdatePacket](IClient *client)
	{
		xrClientData* xr_client = static_cast<xrClientData*>(client);

		if (!xr_client->net_Ready)
			return;

		if (!HasBandwidth(xr_client)
#ifdef DEBUG
			&& !g_sv_SendUpdate
#endif
		)
			return;

		// Send relevant entities to client
		NET_Packet Packet;
		u16 PacketType = M_UPDATE;
		Packet.w_begin(PacketType);
		// GameUpdate
		game->net_Export_Update(Packet, xr_client->ID, xr_client->ID);
		game->net_Export_GameTime(Packet);

		if (xr_client->flags.bLocal) //this is server client;
		{
			SendTo(xr_client->ID, Packet, net_flags(FALSE, TRUE));
			return;
		}

		if (m_aUpdatePackets[0].B.count != 0) //not a first client in update cycle
		{
			m_aUpdatePackets[0].w_seek(0, Packet.B.data, Packet.B.count);
		}
		else
		{
			m_aUpdatePackets[0].w(Packet.B.data, Packet.B.count);

			if (g_Dump_Update_Write)
			{
				if (xr_client->ps)
					Msg("---- UPDATE_Write to %s --- ", xr_client->ps->getName());
				else
					Msg("---- UPDATE_Write to %s --- ", *(xr_client->name));
			}

			NET_Packet tmpPacket;

			xrS_entities::iterator I = entities.begin();
			xrS_entities::iterator E = entities.end();
			for (; I != E; ++I)
			{ //all entities
				CSE_Abstract &Test = *(I->second);

				if (0 == Test.owner)
					continue;
				if (!Test.net_Ready)
					continue;
				if (Test.s_flags.is(M_SPAWN_OBJECT_PHANTOM))
					continue; // Surely: phantom
				if (!Test.Net_Relevant())
					continue;

				tmpPacket.B.count = 0;
				// write specific data
				{
					tmpPacket.w_u16(Test.ID);
					tmpPacket.w_chunk_open8(position);
					Test.UPDATE_Write(tmpPacket);
					u32 ObjectSize = u32(tmpPacket.w_tell() - position) - sizeof(u8);
					tmpPacket.w_chunk_close8(position);

					if (ObjectSize == 0)
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

						PacketType = M_UPDATE_OBJECTS;
						pCurUpdatePacket = &(m_aUpdatePackets[m_iCurUpdatePacket]);
						pCurUpdatePacket->w_begin(PacketType);
					}
					pCurUpdatePacket->w(tmpPacket.B.data, tmpPacket.B.count);
				} //all entities
			}
		}

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
	});

#ifdef DEBUG
	g_sv_SendUpdate = 0;
#endif

	if (m_file_transfers)
	{
		m_file_transfers->update_transfer();
		m_file_transfers->stop_obsolete_receivers();
	}

	if (game->sv_force_sync)
		Perform_game_export();

	DEBUG_VERIFY(verify_entities());
}

xr_vector<shared_str> _tmp_log;
void console_log_cb(LPCSTR text)
{
	_tmp_log.push_back(text);
}

u32 xrServer::OnDelayedMessage(NET_Packet &P, ClientID &sender) // Non-Zero means broadcasting with "flags" as returned
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
	{
		OnCL_Connected(CL);
	}
	break;
	case M_REMOTE_CONTROL_CMD:
	{
		if (CL->m_admin_rights.m_has_admin_rights)
		{
			string128 buffer;
			P.r_stringZ(buffer);

			string128 command;
			sprintf_s(command, 128, "%s %s%u", buffer, RadminIdPrefix, sender.value());

			Msg("# radmin %s is running command: %s", CL->m_admin_rights.m_Login.c_str(), buffer);
			_tmp_log.clear();

			SetLogCB(console_log_cb);			
			Console->Execute(command);
			SetLogCB(nullptr);
			
			NET_Packet P_answ;

			for (u32 i = 0; i < _tmp_log.size(); ++i)
			{
				P_answ.w_begin(M_REMOTE_CONTROL_CMD);
				P_answ.w_stringZ(_tmp_log[i]);
				SendTo(sender, P_answ, net_flags(TRUE, TRUE));
			}
		}
		else
		{
			NET_Packet P_answ;
			P_answ.w_begin(M_REMOTE_CONTROL_CMD);
			P_answ.w_stringZ("you dont have admin rights");
			SendTo(sender, P_answ, net_flags(TRUE, TRUE));
		}
	}
	break;
	case M_FILE_TRANSFER:
	{
		m_file_transfers->on_message(&P, sender);
	}break;
	}

	DEBUG_VERIFY(verify_entities());
	return 0;
}

#pragma TODO("Посмотреть зачем в чн OnMessageSync")

extern float g_fCatchObjectTime;
u32 xrServer::OnMessage(NET_Packet &P, ClientID sender) // Non-Zero means broadcasting with "flags" as returned
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

	case M_HW_RESPOND:
		game->AddDelayedEvent(P, GAME_EVENT_PLAYER_AUTH_HW, 0, sender);
		break;

	case M_STATISTIC_UPDATE:
	{
		if (SV_Client)
			SendBroadcast(SV_Client->ID, P, net_flags(TRUE, TRUE));
		else
			SendBroadcast(BroadcastCID, P, net_flags(TRUE, TRUE));
	}
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

void xrServer::SendTo_LL(ClientID ID, void *data, u32 size, u32 dwFlags, u32 dwTimeout)
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

	struct MessageSenderController
	{
		xrServer* m_owner;
		s16	m_team;
		game_PlayerState* m_sender_ps;
		NET_Packet* m_packet;
		MessageSenderController(xrServer* owner) : m_owner(owner)
		{}

		void operator()(IClient* client)
		{
			xrClientData* xr_client = static_cast<xrClientData*>(client);
			game_PlayerState* ps = xr_client->ps;
			if (!ps)
				return;
			if (!xr_client->net_Ready)
				return;
			if (m_team != -1 && ps->team != m_team)
				return;
			if (m_sender_ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD) &&
				!ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
			{
				return;
			}
			m_owner->SendTo(client->ID, *m_packet);
		}
	};

	MessageSenderController	mesenger(this);
	mesenger.m_team = P->r_s16();
	mesenger.m_sender_ps = CL->ps;
	mesenger.m_packet = P;
	ForEachClientDoSender(mesenger);
		
#pragma TODO("Починить!")
	//s16 teamReceiver = P->r_s16();
	//string128 playerName;
	//P->r_stringZ(playerName);
	//
	//char* pChatMessage = reinterpret_cast<char*>(&P->B.data[P->r_pos]);
	//int chatMessageLength = strlen(pChatMessage);

	//for (int i = 0; i < chatMessageLength; i++)
	//{
	//	if (pChatMessage[i] == '%')
	//	{
	//		pChatMessage[i] = '_';
	//		Msg("! WARNING: player [%s] tried to use %% in chat!", playerName);
	//	}
	//}
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

void xrServer::AddDelayedPacket(NET_Packet &Packet, ClientID &Sender)
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
	struct MaxPingClientDisconnector
	{
		xrServer* m_owner;
		MaxPingClientDisconnector(xrServer* owner) :
			m_owner(owner)
		{}
		void operator()(IClient* client)
		{
			xrClientData* Client = static_cast<xrClientData*>(client);
			game_PlayerState* ps = Client->ps;
			if (!ps)
				return;

			if (ps->ping > g_sv_dwMaxClientPing &&
				Client->m_ping_warn.m_dwLastMaxPingWarningTime + g_sv_time_for_ping_check < Device.dwTimeGlobal)
			{
				++Client->m_ping_warn.m_maxPingWarnings;
				Client->m_ping_warn.m_dwLastMaxPingWarningTime = Device.dwTimeGlobal;

				if (Client->m_ping_warn.m_maxPingWarnings >= g_sv_maxPingWarningsCount)
				{  //kick
					Level().Server->DisconnectClient(Client);
				}
				else
				{ //send warning
					NET_Packet		P;
					P.w_begin(M_CLIENT_WARN);
					P.w_u8(1); // 1 means max-ping-warning
					P.w_u16(ps->ping);
					P.w_u8(Client->m_ping_warn.m_maxPingWarnings);
					P.w_u8(g_sv_maxPingWarningsCount);
					m_owner->SendTo(Client->ID, P, net_flags(FALSE, TRUE));
				}
			}
		}
	};

	MaxPingClientDisconnector temp_functor(this);
	ForEachClientDoSender(temp_functor);
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
