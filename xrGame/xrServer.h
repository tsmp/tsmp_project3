#pragma once
#include "../../xrNetwork/net_server.h"
#include "game_sv_base.h"
#include "id_generator.h"
#include "filetransfer/file_transfer.h"
#include "xrServerRespawnManager.h"

#ifdef DEBUG
//. #define SLOW_VERIFY_ENTITIES
#endif

class CSE_Abstract;
class clientdata_proxy;

const u32 NET_Latency = 50; // time in (ms)

// t-defs
typedef xr_map<u16, CSE_Abstract *> xrS_entities;

class xrClientData : public IClient
{
public:
	CSE_Abstract *owner;
	BOOL net_Ready;
	BOOL net_Accepted;

	BOOL net_PassUpdates;
	u32 net_LastMoveUpdateTime;
	bool bMutedChat;

	game_PlayerState *ps;

	struct
	{
		u8 m_maxPingWarnings;
		u32 m_dwLastMaxPingWarningTime;
	} m_ping_warn;

	struct
	{
		BOOL m_has_admin_rights;
		u32 m_dwLoginTime;
		shared_str m_Login;
	} m_admin_rights;

	struct
	{
		u32 m_LastCheckMinute{ 0 };
		int m_UsageCount{ 0 };
		u32 m_BlockedUntil{ 0 };
	} m_SpeechMessages;

	xrClientData();
	virtual ~xrClientData();
	virtual void Clear();
	LPCSTR GetIP() {
		static xr_string str = m_cAddress.to_string();
		return str.c_str();
	}
};

// main
struct svs_respawn
{
	u32 timestamp;
	u16 phantom;
};

IC bool operator<(const svs_respawn &A, const svs_respawn &B) { return A.timestamp < B.timestamp; }

class xrServer : public IPureServer
{
private:
	xrS_entities entities;
	xr_multiset<svs_respawn> q_respawn;

	u16 m_iCurUpdatePacket;
	xr_vector<NET_Packet> m_aUpdatePackets;

	file_transfer::server_site* m_file_transfers;
	clientdata_proxy* m_screenshot_proxies[/*MAX_PLAYERS_COUNT*/ 32];

	bool IsUpdatePacketsReady();
	void MakeUpdatePackets(xrClientData* xr_client, u32 writeOffset);
	void WriteAtUpdateBegining(NET_Packet const& P);
	void SendUpdatePacketsToClient(xrClientData* xr_client);

	struct DelayedPacket
	{
		ClientID SenderID;
		NET_Packet Packet;
		bool operator==(const DelayedPacket &other)
		{
			return SenderID == other.SenderID;
		}
	};

	xrCriticalSection DelayedPackestCS;
	xr_deque<DelayedPacket> m_aDelayedPackets;
	void ProceedDelayedPackets();
	void AddDelayedPacket(NET_Packet &Packet, ClientID const &Sender);
	u32 OnDelayedMessage(NET_Packet &P, ClientID const &sender); // Non-Zero means broadcasting with "flags" as returned

	void OnRadminCommand(xrClientData *CL, NET_Packet& P, ClientID const &sender);
	void SendUpdatesToAll();
	void SendUpdateTo(IClient* client);

private:
	typedef CID_Generator<
		u32,	 // time identifier type
		u8,		 // compressed id type
		u16,	 // id type
		u8,		 // block id type
		u16,	 // chunk id type
		0,		 // min value
		u16(-2), // max value
		256,	 // block size
		u16(-1)	 // invalid id
		>
		id_generator_type;

private:
	id_generator_type m_tID_Generator;

protected:
	void Server_Client_Check(IClient *CL);
	void PerformCheckClientsForMaxPing();
	void ReportClientStats(IClient* CL);

public:
	game_sv_GameState *game;
	ServerRespawnManager m_RespawnerMP;

	void Export_game_type(IClient *CL);
	void Perform_game_export();

	IC void clear_ids()
	{
		m_tID_Generator = id_generator_type();
	}

	IC u16 PerformIDgen(u16 ID)
	{
		return (m_tID_Generator.tfGetID(ID));
	}

	IC void FreeID(u16 ID, u32 time)
	{
		return (m_tID_Generator.vfFreeID(ID, time));
	}

	void Perform_connect_spawn(CSE_Abstract *E, xrClientData *to, NET_Packet &P);
	void Perform_transfer(NET_Packet &PR, NET_Packet &PT, CSE_Abstract *what, CSE_Abstract *from, CSE_Abstract *to);
	void Perform_reject(CSE_Abstract *what, CSE_Abstract *from, int delta);
	void Perform_destroy(CSE_Abstract *tpSE_Abstract, u32 mode);

	CSE_Abstract *Process_spawn(NET_Packet &P, ClientID const &sender, BOOL bSpawnWithClientsMainEntityAsParent = FALSE, CSE_Abstract *tpExistedEntity = 0);
	void Process_update(NET_Packet &P, ClientID const &sender);
	void Process_save(NET_Packet &P, ClientID const &sender);
	void Process_event(NET_Packet &P, ClientID const &sender);
	void Process_event_ownership(NET_Packet &P, ClientID const &sender, u32 time, u16 ID, BOOL bForced = FALSE);
	bool Process_event_reject(NET_Packet &P, ClientID const &sender, const u32 time, const u16 id_parent, const u16 id_entity, bool send_message = true);
	void Process_event_destroy(NET_Packet &P, ClientID const &sender, u32 time, u16 ID, NET_Packet *pEPack);

	void SendConnectResult(IClient *CL, u8 res, u8 res1, char *ResultStr);

	void AttachNewClient(IClient *CL);
	virtual void OnBuildVersionRespond(IClient *CL, NET_Packet &P);
	void OnRespondUID(IClient* CL, NET_Packet& P);
	void OnPlayersBaseVerifyRespond(IClient* CL, bool banned) override;
	void OnPlayersBaseGenUID(IClient* CL, u32 uid) override;

	void MakeScreenshot(ClientID const &admin_id, ClientID const &cheater_id);
	void initialize_screenshot_proxies();
	void deinitialize_screenshot_proxies();

protected:
	bool CheckAdminRights(const shared_str &user, const shared_str &pass, string512 reason);
	virtual IClient *new_client(SClientConnectData *cl_data);

	virtual bool Check_ServerAccess(IClient *CL, string512 &reason) { return true; }

	virtual void CheckClientGameSpyCDKey(IClient* CL) {};

	void CheckClientBuildVersion(IClient *CL);
	void CheckClientUID(IClient *CL);
	void OnConnectionVerificationStepComplete(IClient *CL);

	void SendConnectionData(IClient *CL);
	void OnChatMessage(NET_Packet *P, xrClientData *CL);
	void OnVoiceMessage(NET_Packet& P, ClientID sender);

public:
	// constr / destr
	xrServer();
	virtual ~xrServer();

	// extended functionality
	virtual u32 OnMessage(NET_Packet &P, ClientID const &sender); // Non-Zero means broadcasting with "flags" as returned
	virtual void OnCL_Connected(IClient *CL);
	virtual void OnCL_Disconnected(IClient *CL);
	virtual bool OnCL_QueryHost();
	virtual void SendTo_LL(ClientID const &ID, void *data, u32 size, u32 dwFlags = DPNSEND_GUARANTEED, u32 dwTimeout = 0);

	virtual IClient *client_Create();			   // create client info
	virtual void client_Replicate();			   // replicate current state to client
	virtual IClient *client_Find_Get(ClientID const &ID); // Find earlier disconnected client
	virtual void client_Destroy(IClient *C);	   // destroy client info

	// utilities
	CSE_Abstract *entity_Create(LPCSTR name);
	void entity_Destroy(CSE_Abstract *&P);
	u32 GetEntitiesNum() { return xr_narrow_cast<u32>(entities.size()); };
	CSE_Abstract *GetEntity(u32 Num);

	xrClientData *ID_to_client(ClientID const &ID, bool ScanAll = false) { return (xrClientData *)(IPureServer::ID_to_client(ID, ScanAll)); }
	CSE_Abstract *ID_to_entity(u16 ID);

	// main
	virtual EConnect Connect(shared_str &session_name);
	virtual void Disconnect();
	virtual void Update();
	void SLS_Default();
	void SLS_Clear();
	void SLS_Save(IWriter &fs);
	void SLS_Load(IReader &fs);
	shared_str level_name(const shared_str &server_options) const;

	void create_direct_client();
	BOOL IsDedicated() const { return m_bDedicated; };

	virtual void Assign_ServerType(string512 &res){};
	virtual bool HasPassword() { return false; }
	virtual bool HasProtected() { return false; }
	bool HasBattlEye();

	virtual void GetServerInfo(CServerInfo *si);
	IClient* GetClientByName(const char* name);

public:
	xr_string ent_name_safe(u16 eid);
#ifdef DEBUG
	bool verify_entities() const;
	void verify_entity(const CSE_Abstract *entity) const;
#endif
};
