#pragma once

#include "net_shared.h"
#include "NET_Common.h"
#include "NET_PlayersMonitor.h"

struct SClientConnectData
{
	ClientID clientID;
	string64 name;
	string64 pass;
	u32 process_id;

	SClientConnectData()
	{
		name[0] = pass[0] = 0;
		process_id = 0;
	}
};

class IPureServer;

struct XRNETWORK_API ip_address
{
	union
	{
		struct
		{
			u8 a1;
			u8 a2;
			u8 a3;
			u8 a4;
		};
		u32 data;
	} m_data;
	void set(LPCSTR src_string);
	xr_string to_string() const;

	bool operator==(const ip_address &other) const
	{
		return (m_data.data == other.m_data.data) ||
			   ((m_data.a1 == other.m_data.a1) &&
				(m_data.a2 == other.m_data.a2) &&
				(m_data.a3 == other.m_data.a3) &&
				(m_data.a4 == 0));
	}
};

class XRNETWORK_API IClient : public MultipacketSender
{
public:
	struct Flags
	{
		u32 bLocal : 1;
		u32 bConnected : 1;
		u32 bReconnect : 1;
		u32 bVerified : 1;
	};

	IClient(CTimer *timer);
	virtual ~IClient() = default;

	IClientStatistic stats;
	int verificationStepsCompleted;

	ClientID ID;
	u32 UID;
	string128 m_guid;
	shared_str name;
	shared_str pass;

	Flags flags; // local/host/normal
	u32 dwTime_LastUpdate;

	ip_address m_cAddress;
	DWORD m_dwPort;
	u32 process_id;

	IPureServer *server;

private:
	virtual void _SendTo_LL(const void *data, u32 size, u32 flags, u32 timeout);
};

IC bool operator==(IClient const *pClient, ClientID const &ID) { return pClient->ID == ID; }

class XRNETWORK_API IServerStatistic
{
public:
	void clear()
	{
		bytes_out = bytes_out_real = 0;
		bytes_in = bytes_in_real = 0;

		dwBytesSended = 0;
		dwSendTime = 0;
		dwBytesPerSec = 0;
	}

	u32 bytes_out, bytes_out_real;
	u32 bytes_in, bytes_in_real;

	u32 dwBytesSended;
	u32 dwSendTime;
	u32 dwBytesPerSec;
};

class CServerInfo;

class BannedClient
{
public:
	ip_address IpAddr;
	time_t BanTime;
	u32 uid;

	BannedClient()
	{
		IpAddr.m_data.data = 0;
		BanTime = 0;
		uid = 0;
	}

	void Load(CInifile& ini, const shared_str& sect);
	void Save(CInifile& ini) const;

	xr_string BannedTimeTo() const;
};

class CBanList
{
public:
	void BanAddress(const ip_address& address, u32 banTimeSec);
	void UnbanAddress(const ip_address& address);
	void BanUid(u32 uid);
	void UnbanUid(u32 uid);
	void Print();

	void Save();
	void Load();
	void Update();

	bool IsAddressBanned(const ip_address& address);
	bool IsUidBanned(u32 uid);

private:
	xr_vector<BannedClient> m_Banned;
};

class XRNETWORK_API IPureServer : private MultipacketReciever
{
public:
	enum EConnect
	{
		ErrConnect,
		ErrBELoad,
		ErrNoLevel,
		ErrMax,
		ErrNoError = ErrMax,
	};

protected:
	shared_str connect_options;
	IDirectPlay8Server *NET;
	IDirectPlay8Address *net_Address_device;

	NET_Compressor net_Compressor;
	IClient *SV_Client;
	PlayersMonitor net_players;

	int psNET_Port;
	CBanList m_BanList;
	xrCriticalSection csMessage;

	// Statistic
	IServerStatistic stats;
	CTimer *device_timer;
	BOOL m_bDedicated;

	IClient *ID_to_client(ClientID const &ID, bool ScanAll = false);

	virtual IClient *new_client(SClientConnectData *cl_data) = 0;
	bool GetClientAddress(IDirectPlay8Address *pClientAddress, ip_address &Address, DWORD *pPort = NULL);

public:
	IPureServer(CTimer *timer, BOOL Dedicated = FALSE);
	virtual ~IPureServer();
	HRESULT net_Handler(u32 dwMessageType, PVOID pMessage);

	virtual EConnect Connect(LPCSTR session_name);
	virtual void Disconnect();

	// send
	virtual void SendTo_LL(ClientID const &ID, void *data, u32 size, u32 dwFlags = DPNSEND_GUARANTEED, u32 dwTimeout = 0);
	virtual void SendTo_Buf(ClientID const &ID, void *data, u32 size, u32 dwFlags = DPNSEND_GUARANTEED, u32 dwTimeout = 0);
	virtual void Flush_Clients_Buffers();

	virtual void OnPlayersBaseVerifyRespond(IClient* CL, bool banned) {}
	virtual void OnPlayersBaseGenUID(IClient* CL, u32 uid) = 0;
	virtual const char* GetServerName() { return ""; }
	virtual const char* GetMapName() { return ""; }

	void SendTo(ClientID const &ID, NET_Packet &P, u32 dwFlags = DPNSEND_GUARANTEED, u32 dwTimeout = 0);
	void SendBroadcast_LL(ClientID const &exclude, void *data, u32 size, u32 dwFlags = DPNSEND_GUARANTEED);
	void SendBroadcast(ClientID const &exclude, NET_Packet &P, u32 dwFlags = DPNSEND_GUARANTEED);
	
	// statistic
	const IServerStatistic *GetStatistic() { return &stats; }
	void ClearStatistic();
	void UpdateClientStatistic(IClient *C);

	// extended functionality
	virtual u32 OnMessage(NET_Packet &P, ClientID const &sender); // Non-Zero means broadcasting with "flags" as returned
	virtual void OnCL_Connected(IClient *C);
	virtual void OnCL_Disconnected(IClient *C);
	virtual bool OnCL_QueryHost() { return true; };

	virtual IClient *client_Create() = 0;		 // create client info
	virtual void client_Replicate() = 0;		 // replicate current state to client
	virtual void client_Destroy(IClient *C) = 0; // destroy client info

	BOOL HasBandwidth(IClient *C);

	IC int GetPort() { return psNET_Port; };
	bool GetClientAddress(ClientID const &ID, ip_address &Address, DWORD *pPort = NULL);

	virtual bool DisconnectClient(IClient *C); 
	virtual bool DisconnectClient(IClient *C, string512 &Reason);
	virtual bool DisconnectAddress(const ip_address &Address);

	virtual bool Check_ServerAccess(IClient *CL, string512 &reason) { return true; }
	virtual void Assign_ServerType(string512 &res){};
	virtual void GetServerInfo(CServerInfo *si){};

	u32	GetClientsCount() { return net_players.ClientsCount(); };
	IClient *GetServerClient() { return SV_Client; };

	template<typename SearchPredicate>
	IClient* FindClient(SearchPredicate predicate) { return net_players.GetFoundClient(predicate); }

	template<typename ActionFunctor>
	void ForEachClientDo(ActionFunctor action) { net_players.ForEachClientDo(action); }

	template<typename SenderFunctor>
	void ForEachClientDoSender(SenderFunctor action) 
	{
		csMessage.Enter();
		net_players.ForEachClientDo(action);
		csMessage.Leave();
	}

	template<typename ActionFunctor>
	void ForEachDisconnectedClientDo(ActionFunctor action) { net_players.ForEachDisconnectedClientDo(action); };

	IClient *GetClientByID(ClientID const &clientId) { return net_players.GetClientById(clientId, false); };
	IClient *GetDisconnectedClientByID(ClientID const &clientId) { return net_players.GetClientById(clientId, true); }

	const shared_str &GetConnectOptions() const { return connect_options; }

	// Ban
protected:
	void BannedListSave() { m_BanList.Save(); }
	void BannedListLoad() { m_BanList.Load(); }
	void BannedListUpdate() { m_BanList.Update(); }
	bool IsUidBanned(u32 uid) { return m_BanList.IsUidBanned(uid); }
public:
	void BanClient(IClient* C, u32 BanTime);
	void BanAddress(const ip_address& address, u32 banTimeSec) { m_BanList.BanAddress(address, banTimeSec); }
	void UnBanAddress(const ip_address& address) { m_BanList.UnbanAddress(address); }
	void BanUid(u32 uid) { m_BanList.BanUid(uid); };
	void UnbanUid(u32 uid) { m_BanList.UnbanUid(uid); };
	void Print_Banned_Addreses() { m_BanList.Print(); }

private:
	virtual void _Recieve(const void *data, u32 data_size, u32 param);
};
