#include "stdafx.h"
#include "NET_Common.h"
#include "net_client.h"
#include "net_server.h"
#include "net_messages.h"
#include "NET_Log.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#include "dxerr9.h"

extern void gen_auth_code();

static INetLog *pClNetLog = nullptr;
void IC InitClNetLog(u32 time)
{
	string_path LogPath;
	strcpy(LogPath, "net_cl_log");

	__try
	{
		if (FS.path_exist("$logs$"))
			FS.update_path(LogPath, "$logs$", LogPath);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		string_path temp;
		strcpy(temp, LogPath);
		strcpy(LogPath, "logs/");
		strcat(LogPath, temp);
	}

	string_path log_file_name;
	strconcat(sizeof(log_file_name), log_file_name, LogPath, ".log");

	pClNetLog = xr_new<INetLog>(log_file_name, time);
}

static u32 LastTimeCreate = 0;

const u32 BASE_PORT_LAN_SV = 5445;
const u32 BASE_PORT_LAN_CL = 5446;
const u32 BASE_PORT = 0;
const u32 END_PORT = 65535;
const u32 MaxPasswordLen = 256;

// {0218FA8B-515B-4bf2-9A5F-2F079D1759F3}
static const GUID NET_GUID = { 0x218fa8b, 0x515b, 0x4bf2, {0x9a, 0x5f, 0x2f, 0x7, 0x9d, 0x17, 0x59, 0xf3} };

// {8D3F9E5E-A3BD-475b-9E49-B0E77139143C}
static const GUID CLSID_NETWORKSIMULATOR_DP8SP_TCPIP = { 0x8d3f9e5e, 0xa3bd, 0x475b, {0x9e, 0x49, 0xb0, 0xe7, 0x71, 0x39, 0x14, 0x3c} };

XRNETWORK_API Flags32 psNET_Flags = { 0 };
XRNETWORK_API int psNET_ClientUpdate = 30; // FPS
XRNETWORK_API int psNET_ClientPending = 2;
XRNETWORK_API BOOL psNET_direct_connect = FALSE;

INetQueue::INetQueue()
#ifdef PROFILE_CRITICAL_SECTIONS
	: cs(MUTEX_PROFILE_ID(INetQueue))
#endif // PROFILE_CRITICAL_SECTIONS
{
	unused.reserve(128);

	for (int i = 0; i < 16; i++)
		unused.push_back(xr_new<NET_Packet>());
}

INetQueue::~INetQueue()
{
	cs.Enter();

	for (NET_Packet* unusedPacket : unused)
		xr_delete(unusedPacket);

	for (NET_Packet* readyPacket : ready)
		xr_delete(readyPacket);

	cs.Leave();
}

void INetQueue::CreateCommit(NET_Packet *P)
{
	cs.Enter();
	ready.push_back(P);
	cs.Leave();
}

NET_Packet *INetQueue::CreateGet()
{
	NET_Packet *P = nullptr;
	cs.Enter();

	if (unused.empty())
	{
		P = xr_new<NET_Packet>();
		LastTimeCreate = GetTickCount();
	}
	else
	{
		P = unused.back();
		unused.pop_back();
	}

	cs.Leave();
	return P;
}

NET_Packet *INetQueue::Retreive()
{
	NET_Packet *P = nullptr;
	cs.Enter();

	if (!ready.empty())
		P = ready.front();
	else
	{
		u32 tmp_time = GetTickCount() - 60000;
		u32 size = unused.size();

		if ((LastTimeCreate < tmp_time) && (size > 32))
		{
			xr_delete(unused.back());
			unused.pop_back();
		}
	}

	cs.Leave();
	return P;
}

void INetQueue::Release()
{
	cs.Enter();
	VERIFY(!ready.empty());

	u32 tmp_time = GetTickCount() - 60000;
	u32 size = unused.size();

	if ((LastTimeCreate < tmp_time) && (size > 32))	
		xr_delete(ready.front());	
	else
		unused.push_back(ready.front());

	ready.pop_front();
	cs.Leave();
}

class XRNETWORK_API syncQueue
{
	static const u32 syncQueueSize = 512;
	u32 table[syncQueueSize];
	u32 write;
	u32 count;

public:
	syncQueue() { clear(); }

	IC void push(u32 value)
	{
		table[write++] = value;

		if (write == syncQueueSize)
			write = 0;

		if (count <= syncQueueSize)
			count++;
	}

	IC u32 *begin() { return table; }
	IC u32 *end() { return table + count; }
	IC u32 size() { return count; }

	IC void clear()
	{
		write = 0;
		count = 0;
	}

	static const int syncSamples = 256;
} net_DeltaArray;

void IPureClient::_SendTo_LL(const void *data, u32 size, u32 flags, u32 timeout)
{
	IPureClient::SendTo_LL(const_cast<void *>(data), size, flags, timeout);
}

// Internal system message
void IPureClient::ReceiveSysMessage(const MSYS_PING* cfg, u32 dataSize)
{
	switch (dataSize)
	{
		case sizeof(MSYS_PING):
		{
			// It is reverted(server) ping
			u32 time = TimerAsync(device_timer);
			u32 ping = time - (cfg->dwTime_ClientSend);
			u32 delta = cfg->dwTime_Server + ping / 2 - time;

			net_DeltaArray.push(delta);
			Sync_Average();
		}
		break;

		case sizeof(MSYS_CONFIG):
			net_Connected = EnmConnectionCompleted;
			break;

		default:
			Msg("! Unknown system message");
	}
}

void IPureClient::_Recieve(const void *data, u32 data_size, u32 /*param*/)
{
	const MSYS_PING *cfg = reinterpret_cast< const MSYS_PING*>(data);

	if ((data_size > 2 * sizeof(u32)) && (cfg->sign1 == 0x12071980) && (cfg->sign2 == 0x26111975))
	{
		ReceiveSysMessage(cfg, data_size);
		return;
	}

	if (net_Connected != EnmConnectionCompleted)
		return;

	// one of the messages - decompress it
	if (psNET_Flags.test(NETFLAG_LOG_CL_PACKETS))
	{
		if (!pClNetLog)
			InitClNetLog(timeServer());

		if (pClNetLog)
			pClNetLog->LogData(timeServer(), const_cast<void*>(data), data_size, TRUE);
	}

	OnMessage(const_cast<void*>(data), data_size);
}

IPureClient::IPureClient(CTimer *timer) : net_Statistic(timer)
#ifdef PROFILE_CRITICAL_SECTIONS
										  , net_csEnumeration(MUTEX_PROFILE_ID(IPureClient::net_csEnumeration))
#endif // PROFILE_CRITICAL_SECTIONS
{
	NET = nullptr;
	net_Address_server = nullptr;
	net_Address_device = nullptr;
	device_timer = timer;
	net_TimeDelta_User = 0;
	net_Time_LastUpdate = 0;
	net_TimeDelta = 0;
	net_TimeDelta_Calculated = 0;

	pClNetLog = nullptr; //xr_new<INetLog>("logs\\net_cl_log.log", timeServer());
}

IPureClient::~IPureClient()
{
	xr_delete(pClNetLog);
	pClNetLog = nullptr;
	psNET_direct_connect = FALSE;
}

static HRESULT WINAPI DPMessageHandler(PVOID pvUserContext, DWORD dwMessageType, PVOID pMessage)
{
	IPureClient* Cl = reinterpret_cast<IPureClient*>(pvUserContext);
	return Cl->net_Handler(dwMessageType, pMessage);
}

namespace ParseParams
{
	std::string ServerName(const char* cmdLine)
	{
		string256 server_name = "";

		if (strchr(cmdLine, '/'))
			strncpy(server_name, cmdLine, strchr(cmdLine, '/') - cmdLine);
		if (strchr(server_name, '/'))
			*strchr(server_name, '/') = '\0';

		return std::string(server_name);
	}

	std::string PlayerName(const char* cmdLine)
	{
		string64 user_name_str = "";

		if (strstr(cmdLine, "name="))
		{
			const char* NM = strstr(cmdLine, "name=") + 5;

			if (strchr(NM, '/'))
				strncpy(user_name_str, NM, strchr(NM, '/') - NM);
			else
				strcpy(user_name_str, NM);
		}

		return std::string(user_name_str);
	}

	std::string PlayerPass(const char* cmdLine)
	{
		string64 user_pass = "";

		if (strstr(cmdLine, "pass="))
		{
			const char* UP = strstr(cmdLine, "pass=") + 5;

			if (strchr(UP, '/'))
				strncpy(user_pass, UP, strchr(UP, '/') - UP);
			else
				strcpy(user_pass, UP);
		}

		return std::string(user_pass);
	}

	std::string Password(const char* cmdLine)
	{
		string64 password_str = "";

		if (strstr(cmdLine, "psw="))
		{
			const char* PSW = strstr(cmdLine, "psw=") + 4;

			if (strchr(PSW, '/'))
				strncpy(password_str, PSW, strchr(PSW, '/') - PSW);
			else
				strcpy(password_str, PSW);
		}

		return std::string(password_str);
	}

	u32 PortSv(const char* cmdLine)
	{
		u32 result = BASE_PORT_LAN_SV;

		if (strstr(cmdLine, "port="))
		{
			string64 portstr;
			strcpy(portstr, strstr(cmdLine, "port=") + 5);

			if (strchr(portstr, '/'))
				*strchr(portstr, '/') = 0;

			result = static_cast<u32>(atol(portstr));
			clamp(result, BASE_PORT, END_PORT);
		}

		return result;
	}

	u32 PortCl(const char* cmdLine)
	{
		u32 result = static_cast<u32>(-1);

		if (strstr(cmdLine, "portcl="))
		{
			string64 portstr;
			strcpy(portstr, strstr(cmdLine, "portcl=") + 7);

			if (strchr(portstr, '/'))
				*strchr(portstr, '/') = 0;

			result = static_cast<u32>(atol(portstr));
			clamp(result, BASE_PORT, END_PORT);
		}

		return result;
	}
}

bool IPureClient::ConnectLocal(LPCSTR options, DPN_APPLICATION_DESC &dpAppDesc)
{	
	u32 basePortCl = ParseParams::PortCl(options);
	bool portWasSet = true;

	if (basePortCl == static_cast<u32>(-1))
	{
		portWasSet = false;
		basePortCl = BASE_PORT_LAN_CL;
	}

	std::string password = ParseParams::Password(options);
	WCHAR SessionPasswordUNICODE[MaxPasswordLen];

	if (!password.empty())
	{
		CHK_DX(MultiByteToWideChar(CP_ACP, 0, password.c_str(), -1, SessionPasswordUNICODE, MaxPasswordLen));
		dpAppDesc.dwFlags |= DPNSESSION_REQUIREPASSWORD;
		dpAppDesc.pwszPassword = SessionPasswordUNICODE;
	}

	HRESULT res = S_FALSE;

	for (u32 port = basePortCl; port < basePortCl + 100; port++)
	{
		R_CHK(net_Address_device->AddComponent(DPNA_KEY_PORT, &port, sizeof(port), DPNA_DATATYPE_DWORD));
		res = NET->Connect(&dpAppDesc, net_Address_server, net_Address_device, NULL, NULL, NULL, 0, NULL, NULL, DPNCONNECT_SYNC);

		if (res == S_OK)
		{
			Msg("- IPureClient : created on port %d!", port);
			break;
		}
				
		if (portWasSet)
		{
			Msg("! IPureClient : port %d is BUSY!", port);
			return false;
		}
#ifdef DEBUG
		else
			Msg("! IPureClient : port %d is BUSY!", port);
#endif			
	}

	if (res != S_OK)
		return false;

	// Create ONE node
	HOST_NODE NODE;
	ZeroMemory(&NODE, sizeof(HOST_NODE));

	// Copy the Host Address
	R_CHK(net_Address_server->Duplicate(&NODE.pHostAddress));

	// Retreive session name
	char desc[4096];
	ZeroMemory(desc, sizeof(desc));
	DPN_APPLICATION_DESC* dpServerDesc = reinterpret_cast<DPN_APPLICATION_DESC*>(desc);
	DWORD dpServerDescSize = sizeof(desc);
	dpServerDesc->dwSize = sizeof(DPN_APPLICATION_DESC);
	R_CHK(NET->GetApplicationDesc(dpServerDesc, &dpServerDescSize, 0));

	if (dpServerDesc->pwszSessionName)
	{
		string4096 dpSessionName;
		R_CHK(WideCharToMultiByte(CP_ACP, 0, dpServerDesc->pwszSessionName, -1, dpSessionName, sizeof(dpSessionName), 0, 0));
		NODE.dpSessionName = &dpSessionName[0];
	}

	net_Hosts.push_back(NODE);
	return true;
}

bool IPureClient::ConnectGlobal(LPCSTR options, DPN_APPLICATION_DESC& dpAppDesc)
{
	string64 EnumData;
	EnumData[0] = '\0';
	strcat(EnumData, "ToConnect");
	DWORD EnumSize = xr_strlen(EnumData) + 1;

	// We now have the host address so lets enum
	u32 basePortCl = ParseParams::PortCl(options);
	bool portWasSet = true;

	if (basePortCl == static_cast<u32>(-1))
	{
		portWasSet = false;
		basePortCl = BASE_PORT_LAN_CL;
	}

	for (u32 port = basePortCl; port < basePortCl + 100; port++)
	{
		R_CHK(net_Address_device->AddComponent(DPNA_KEY_PORT, &port, sizeof(port), DPNA_DATATYPE_DWORD));
		HRESULT res = NET->EnumHosts(&dpAppDesc, net_Address_server, net_Address_device, EnumData, EnumSize, 10, 1000, 1000, NULL, NULL, DPNENUMHOSTS_SYNC);

		if (res == S_OK)
		{
			Msg("- IPureClient : created on port %d!", port);
			break;
		}

		switch (res)
		{
		case DPNERR_INVALIDHOSTADDRESS:
			OnInvalidHost();
			return false;
			break;

		case DPNERR_SESSIONFULL:
			OnSessionFull();
			return false;
			break;
		}

		if (portWasSet)
		{
			Msg("! IPureClient : port %d is BUSY!", port);
			return false;
		}
#ifdef DEBUG
		else
			Msg("! IPureClient : port %d is BUSY!", port);

		string1024 tmp_err = "";
		DXTRACE_ERR(tmp_err, res);
#endif
	}

	// Connection	
	if (net_Hosts.empty())
	{
		OnInvalidHost();
		return false;
	}

	WCHAR SessionPasswordUNICODE[MaxPasswordLen];
	std::string password = ParseParams::Password(options);

	if (!password.empty())
	{
		CHK_DX(MultiByteToWideChar(CP_ACP, 0, password.c_str(), -1, SessionPasswordUNICODE, MaxPasswordLen));
		dpAppDesc.dwFlags |= DPNSESSION_REQUIREPASSWORD;
		dpAppDesc.pwszPassword = SessionPasswordUNICODE;
	}

	net_csEnumeration.Enter();

	// real connect
	for (const HOST_NODE &node : net_Hosts)	
		Msg("* HOST: %s\n", *node.dpSessionName);
	
	IDirectPlay8Address* pHostAddress = nullptr;
	R_CHK(net_Hosts.front().pHostAddress->Duplicate(&pHostAddress));

	HRESULT res = NET->Connect(&dpAppDesc, 	pHostAddress, net_Address_device, 	NULL, 	NULL, NULL,
		0, NULL, NULL, DPNCONNECT_SYNC);
	
	net_csEnumeration.Leave();
	_RELEASE(pHostAddress);

#ifdef DEBUG
	string1024 tmp_err = "";
	DXTRACE_ERR(tmp_err, res);
#endif

	switch (res)
	{
	case DPNERR_INVALIDPASSWORD:	
		OnInvalidPassword();
		break;

	case DPNERR_SESSIONFULL:	
		OnSessionFull();
		break;

	case DPNERR_CANTCREATEPLAYER:	
		Msg("! Error: Can\'t create player");
		break;
	}

	return res == S_OK;
}

bool IPureClient::Connect(LPCSTR options)
{
	R_ASSERT(options);
	net_Disconnected = false;
	net_TimeDelta = 0;

	if (!psNET_direct_connect && !strstr(options, "localhost"))
		gen_auth_code();

	if (psNET_direct_connect)		
		return true;	

	net_Connected = EnmConnectionWait;
	net_Syncronised = false;
	
	// Create the IDirectPlay8Client object.
	HRESULT CoCreateInstanceRes = CoCreateInstance(CLSID_DirectPlay8Client, nullptr, CLSCTX_INPROC_SERVER,
		IID_IDirectPlay8Client, reinterpret_cast<LPVOID*>(&NET));

	if (CoCreateInstanceRes != S_OK)
	{
		string256 tmp = "";
		DXTRACE_ERR(tmp, CoCreateInstanceRes);
		CHK_DX(CoCreateInstanceRes);
	}

	// Initialize IDirectPlay8Client object.
#ifdef DEBUG
	R_CHK(NET->Initialize(this, DPMessageHandler, 0));
#else
	R_CHK(NET->Initialize(this, DPMessageHandler, DPNINITIALIZE_DISABLEPARAMVAL));
#endif

	bool bSimulator = strstr(Core.Params, "-netsim");

	// Create our IDirectPlay8Address Device Address, --- Set the SP for our Device Address
	net_Address_device = nullptr;
	R_CHK(CoCreateInstance(CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address, reinterpret_cast<LPVOID*>(&net_Address_device)));
	R_CHK(net_Address_device->SetSP(bSimulator ? &CLSID_NETWORKSIMULATOR_DP8SP_TCPIP : &CLSID_DP8SP_TCPIP));

	// Create our IDirectPlay8Address Server Address, --- Set the SP for our Server Address
	static const u32 MaxServerNameLen = 256;
	WCHAR ServerNameUNICODE[MaxServerNameLen];
	std::string serverName = ParseParams::ServerName(options);
	R_CHK(MultiByteToWideChar(CP_ACP, 0, serverName.c_str(), -1, ServerNameUNICODE, MaxServerNameLen));

	int svPort = ParseParams::PortSv(options);
	net_Address_server = nullptr;

	R_CHK(CoCreateInstance(CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address, reinterpret_cast<LPVOID*>(&net_Address_server)));
	R_CHK(net_Address_server->SetSP(bSimulator ? &CLSID_NETWORKSIMULATOR_DP8SP_TCPIP : &CLSID_DP8SP_TCPIP));
	R_CHK(net_Address_server->AddComponent(DPNA_KEY_HOSTNAME, ServerNameUNICODE, 2 * u32(wcslen(ServerNameUNICODE) + 1), DPNA_DATATYPE_STRING));
	R_CHK(net_Address_server->AddComponent(DPNA_KEY_PORT, &svPort, sizeof(svPort), DPNA_DATATYPE_DWORD));

	// Now set up the Application Description
	DPN_APPLICATION_DESC dpAppDesc;
	ZeroMemory(&dpAppDesc, sizeof(DPN_APPLICATION_DESC));
	dpAppDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
	dpAppDesc.guidApplication = NET_GUID;

	// Setup client info
	static const int MaxClientNameLen = 256;
	WCHAR ClientNameUNICODE[MaxClientNameLen];
	std::string clientName = ParseParams::PlayerName(options);
	R_CHK(MultiByteToWideChar(CP_ACP, 0, clientName.c_str(), -1, ClientNameUNICODE, MaxClientNameLen));

	{
		DPN_PLAYER_INFO Pinfo;
		ZeroMemory(&Pinfo, sizeof(Pinfo));
		Pinfo.dwSize = sizeof(Pinfo);
		Pinfo.dwInfoFlags = DPNINFO_NAME | DPNINFO_DATA;
		Pinfo.pwszName = ClientNameUNICODE;

		SClientConnectData cl_data;
		cl_data.process_id = GetCurrentProcessId();
		strcpy_s(cl_data.name, clientName.c_str());
		strcpy_s(cl_data.pass, ParseParams::PlayerPass(options).c_str());

		Pinfo.pvData = &cl_data;
		Pinfo.dwDataSize = sizeof(cl_data);

		R_CHK(NET->SetClientInfo(&Pinfo, 0, 0, DPNSETCLIENTINFO_SYNC));
	}

	if (serverName == "localhost")
		return ConnectLocal(options, dpAppDesc);
	else
		return ConnectGlobal(options, dpAppDesc);
}

void IPureClient::Disconnect()
{
	if (NET)
		NET->Close(0);

	// Clean up Host _list_
	net_csEnumeration.Enter();

	for(HOST_NODE &node: net_Hosts)
		_RELEASE(node.pHostAddress);

	net_Hosts.clear();
	net_csEnumeration.Leave();

	// Release interfaces
	_SHOW_REF("cl_netADR_Server", net_Address_server);
	_RELEASE(net_Address_server);
	_SHOW_REF("cl_netADR_Device", net_Address_device);
	_RELEASE(net_Address_device);
	_SHOW_REF("cl_netCORE", NET);
	_RELEASE(NET);

	net_Connected = EnmConnectionWait;
	net_Syncronised = false;
}

HRESULT IPureClient::net_Handler(u32 dwMessageType, PVOID pMessage)
{
	const char* msg = nullptr;

	switch (dwMessageType)
	{
	case DPN_MSGID_ENUM_HOSTS_RESPONSE:
	{
		PDPNMSG_ENUM_HOSTS_RESPONSE pEnumHostsResponseMsg = reinterpret_cast<PDPNMSG_ENUM_HOSTS_RESPONSE>(pMessage);
		const DPN_APPLICATION_DESC* pDesc = pEnumHostsResponseMsg->pApplicationDescription;

		// Insert each host response if it isn't already present
		net_csEnumeration.Enter();
		bool bHostRegistered = false;

		for (const HOST_NODE &node : net_Hosts)
		{		
			if (pDesc->guidInstance == node.dpAppDesc.guidInstance)
			{
				// This host is already in the list
				bHostRegistered = true;
				break;
			}
		}

		if (!bHostRegistered)
		{
			// This host session is not in the list then so insert it.
			HOST_NODE NODE;
			ZeroMemory(&NODE, sizeof(HOST_NODE));

			// Copy the Host Address
			R_CHK(pEnumHostsResponseMsg->pAddressSender->Duplicate(&NODE.pHostAddress));
			CopyMemory(&NODE.dpAppDesc, pDesc, sizeof(DPN_APPLICATION_DESC));

			// Null out all the pointers we aren't copying
			NODE.dpAppDesc.pwszSessionName = nullptr;
			NODE.dpAppDesc.pwszPassword = nullptr;
			NODE.dpAppDesc.pvReservedData = nullptr;
			NODE.dpAppDesc.dwReservedDataSize = 0;
			NODE.dpAppDesc.pvApplicationReservedData = nullptr;
			NODE.dpAppDesc.dwApplicationReservedDataSize = 0;

			if (pDesc->pwszSessionName)
			{
				string4096 dpSessionName;
				R_CHK(WideCharToMultiByte(CP_ACP, 0, pDesc->pwszSessionName, -1, dpSessionName, sizeof(dpSessionName), 0, 0));
				NODE.dpSessionName = (char *)(&dpSessionName[0]);
			}

			net_Hosts.push_back(NODE);
		}
		net_csEnumeration.Leave();
	}
	break;

	case DPN_MSGID_RECEIVE:
	{
		PDPNMSG_RECEIVE pMsg = reinterpret_cast<PDPNMSG_RECEIVE>(pMessage);
		MultipacketReciever::RecievePacket(pMsg->pReceiveData, pMsg->dwReceiveDataSize);
	}
	break;

	case DPN_MSGID_TERMINATE_SESSION:
	{
		PDPNMSG_TERMINATE_SESSION pMsg = reinterpret_cast<PDPNMSG_TERMINATE_SESSION>(pMessage);
		char *data = reinterpret_cast<char*>(pMsg->pvTerminateData);
		u32 size = pMsg->dwTerminateDataSize;
		HRESULT m_hResultCode = pMsg->hResultCode;

		net_Disconnected = true;
		msg = "DPN_MSGID_TERMINATE_SESSION";

		if (size)		
			OnSessionTerminate(data);
		else		
			OnSessionTerminate(::Debug.error2string(m_hResultCode));
	}
	break;

#if 1
	case DPN_MSGID_ADD_PLAYER_TO_GROUP:
		msg = "DPN_MSGID_ADD_PLAYER_TO_GROUP";
		break;

	case DPN_MSGID_ASYNC_OP_COMPLETE:
		msg = "DPN_MSGID_ASYNC_OP_COMPLETE";
		break;

	case DPN_MSGID_CLIENT_INFO:
		msg = "DPN_MSGID_CLIENT_INFO";
		break;

	case DPN_MSGID_CONNECT_COMPLETE:
	{
		PDPNMSG_CONNECT_COMPLETE pMsg = reinterpret_cast<PDPNMSG_CONNECT_COMPLETE>(pMessage);

#ifdef DEBUG		
		if (pMsg->hResultCode != S_OK)
		{
			string1024 tmp = "";
			DXTRACE_ERR(tmp, pMsg->hResultCode);
		}
#endif
		if (pMsg->dwApplicationReplyDataSize)
		{
			string256 ResStr = "";
			strncpy(ResStr, reinterpret_cast<char*>(pMsg->pvApplicationReplyData), pMsg->dwApplicationReplyDataSize);
			Msg("Connection result : %s", ResStr);
		}
		else
			msg = "DPN_MSGID_CONNECT_COMPLETE";
	}
	break;

	case DPN_MSGID_CREATE_GROUP:
		msg = "DPN_MSGID_CREATE_GROUP";
		break;

	case DPN_MSGID_CREATE_PLAYER:
		msg = "DPN_MSGID_CREATE_PLAYER";
		break;

	case DPN_MSGID_DESTROY_GROUP:
		msg = "DPN_MSGID_DESTROY_GROUP";
		break;

	case DPN_MSGID_DESTROY_PLAYER:
		msg = "DPN_MSGID_DESTROY_PLAYER";
		break;

	case DPN_MSGID_ENUM_HOSTS_QUERY:
		msg = "DPN_MSGID_ENUM_HOSTS_QUERY";
		break;

	case DPN_MSGID_GROUP_INFO:
		msg = "DPN_MSGID_GROUP_INFO";
		break;

	case DPN_MSGID_HOST_MIGRATE:
		msg = "DPN_MSGID_HOST_MIGRATE";
		break;
	case DPN_MSGID_INDICATE_CONNECT:
		msg = "DPN_MSGID_INDICATE_CONNECT";
		break;

	case DPN_MSGID_INDICATED_CONNECT_ABORTED:
		msg = "DPN_MSGID_INDICATED_CONNECT_ABORTED";
		break;

	case DPN_MSGID_PEER_INFO:
		msg = "DPN_MSGID_PEER_INFO";
		break;

	case DPN_MSGID_REMOVE_PLAYER_FROM_GROUP:
		msg = "DPN_MSGID_REMOVE_PLAYER_FROM_GROUP";
		break;

	case DPN_MSGID_RETURN_BUFFER:
		msg = "DPN_MSGID_RETURN_BUFFER";
		break;

	case DPN_MSGID_SEND_COMPLETE:
		msg = "DPN_MSGID_SEND_COMPLETE";
		break;

	case DPN_MSGID_SERVER_INFO:
		msg = "DPN_MSGID_SERVER_INFO";
		break;

	default:
		msg = "???";
		break;
	}
#endif

	//if(msg)
	//	Msg("! ************************************ : %s", msg);
	
	return S_OK;
}

void IPureClient::OnMessage(void *data, u32 size)
{
	// One of the messages - decompress it
	NET_Packet *P = net_Queue.CreateGet();

	P->construct(data, size);
	P->timeReceive = timeServer_Async();

	u16 tmp_type;
	P->r_begin(tmp_type);
	net_Queue.CreateCommit(P);
}

void IPureClient::timeServer_Correct(u32 sv_time, u32 cl_time)
{
	u32 ping = net_Statistic.getPing();
	u32 delta = sv_time + ping / 2 - cl_time;
	net_DeltaArray.push(delta);
	Sync_Average();
}

void IPureClient::SendTo_LL(void *data, u32 size, u32 dwFlags, u32 dwTimeout)
{
	if (net_Disconnected)
		return;

	if (psNET_Flags.test(NETFLAG_LOG_CL_PACKETS))
	{
		if (!pClNetLog)
			InitClNetLog(timeServer());
		if (pClNetLog)
			pClNetLog->LogData(timeServer(), data, size);
	}

	DPN_BUFFER_DESC desc;
	desc.dwBufferSize = size;
	desc.pBufferData = reinterpret_cast<BYTE*>(data);
	net_Statistic.dwBytesSended += size;

	// verify
	VERIFY(desc.dwBufferSize);
	VERIFY(desc.pBufferData);
	VERIFY(NET);
	DPNHANDLE hAsync = 0;

	HRESULT hr = NET->Send(&desc, 1, dwTimeout, 0, &hAsync, dwFlags | DPNSEND_COALESCE);
	if(FAILED(hr))
	{
		Msg("! ERROR: Failed to send net-packet, reason: %s", ::Debug.error2string(hr));
		string1024 tmp = "";
		DXTRACE_ERR(tmp, hr);
	}
}

void IPureClient::Send(NET_Packet &packet, u32 dwFlags, u32 dwTimeout)
{
	MultipacketSender::SendPacket(packet.B.data, packet.B.count, dwFlags, dwTimeout);
}

void IPureClient::Flush_Send_Buffer()
{
	MultipacketSender::FlushSendBuffer(0);
}

bool IPureClient::net_HasBandwidth()
{
	u32 dwTime = TimeGlobal(device_timer);
	u32 dwInterval = 0;

	if (net_Disconnected)
		return false;

	if (psNET_ClientUpdate)
		dwInterval = 1000 / psNET_ClientUpdate;

	if (psNET_Flags.test(NETFLAG_MINIMIZEUPDATES))
		dwInterval = 1000; // approx 3 times per second

	if (psNET_direct_connect)
	{
		if (psNET_ClientUpdate && (dwTime - net_Time_LastUpdate) > dwInterval)
		{
			net_Time_LastUpdate = dwTime;
			return true;
		}
		
		return false;
	}

	if (!psNET_ClientUpdate || (dwTime - net_Time_LastUpdate) <= dwInterval)
		return false;

	R_ASSERT(NET);

	// check queue for "empty" state
	DWORD dwPending = 0;

	if (FAILED(NET->GetSendQueueInfo(&dwPending, 0, 0)))
		return false;

	if (dwPending > u32(psNET_ClientPending))
	{
		net_Statistic.dwTimesBlocked++;
		return false;
	}

	UpdateStatistic();
	net_Time_LastUpdate = dwTime;
	return true;
}

void IPureClient::UpdateStatistic()
{
	// Query network statistic for this client
	DPN_CONNECTION_INFO CI;
	ZeroMemory(&CI, sizeof(CI));
	CI.dwSize = sizeof(CI);

	if(SUCCEEDED(NET->GetConnectionInfo(&CI, 0)))
		net_Statistic.Update(CI);
}

void IPureClient::Sync_Thread()
{
	// Ping server
	net_DeltaArray.clear();
	R_ASSERT(NET);

	while(NET && !net_Disconnected)
	{
		// Waiting for queue empty state
		if (net_Syncronised)
			break;
		else
		{
			DWORD dwPending = 0;
			do
			{
				R_CHK(NET->GetSendQueueInfo(&dwPending, 0, 0));
				Sleep(1);
			} while (dwPending);
		}

		// Construct message
		MSYS_PING clPing;
		clPing.sign1 = 0x12071980;
		clPing.sign2 = 0x26111975;
		clPing.dwTime_ClientSend = TimerAsync(device_timer);

		// Send it
		__try
		{
			DPN_BUFFER_DESC desc;
			DPNHANDLE hAsync = 0;
			desc.dwBufferSize = sizeof(clPing);
			desc.pBufferData = LPBYTE(&clPing);

			if (!NET || net_Disconnected)
				break;

			if (FAILED(NET->Send(&desc, 1, 0, 0, &hAsync, net_flags(FALSE, FALSE, TRUE))))
			{
				Msg("* CLIENT: SyncThread: EXIT. (failed to send - disconnected?)");
				break;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			Msg("* CLIENT: SyncThread: EXIT. (failed to send - disconnected?)");
			break;
		}

		if (net_Syncronised)
			continue;

		// Waiting for reply-packet to arrive
		u32 old_size = net_DeltaArray.size();
		u32 timeBegin = TimerAsync(device_timer);

		while ((net_DeltaArray.size() == old_size) && (TimerAsync(device_timer) - timeBegin < 5000))
			Sleep(1);

		if (net_DeltaArray.size() >= syncQueue::syncSamples)
		{
			net_Syncronised = true;
			net_TimeDelta = net_TimeDelta_Calculated;
		}
	}
}

void IPureClient::Sync_Average()
{
	// Analyze results
	s64 summary_delta = 0;
	s32 size = net_DeltaArray.size();

	for (const u32 &item : net_DeltaArray)
		summary_delta += static_cast<int>(item);

	s64 frac = s64(summary_delta) % s64(size);

	if (frac < 0)
		frac = -frac;

	summary_delta /= s64(size);

	if (frac > s64(size / 2))
		summary_delta += (summary_delta < 0) ? -1 : 1;

	net_TimeDelta_Calculated = s32(summary_delta);
	net_TimeDelta = (net_TimeDelta * 5 + net_TimeDelta_Calculated) / 6;
}

void sync_thread(void *P)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	IPureClient* C = reinterpret_cast<IPureClient*>(P);
	C->Sync_Thread();
}

void IPureClient::net_Syncronize()
{
	net_Syncronised = false;
	net_DeltaArray.clear();
	thread_spawn(sync_thread, "network-time-sync", 0, this);
}

void IPureClient::ClearStatistic()
{
	net_Statistic.Clear();
}

bool IPureClient::net_IsSyncronised()
{
	return net_Syncronised;
}

#include <WINSOCK2.H>
#include <Ws2tcpip.h>

bool IPureClient::GetServerAddress(ip_address &pAddress, DWORD *pPort)
{
	*pPort = 0;

	if (!net_Address_server)
		return false;

	WCHAR wstrHostname[2048] = {0};
	DWORD dwHostNameSize = sizeof(wstrHostname);
	DWORD dwHostNameDataType = DPNA_DATATYPE_STRING;
	CHK_DX(net_Address_server->GetComponentByName(DPNA_KEY_HOSTNAME, wstrHostname, &dwHostNameSize, &dwHostNameDataType));

	string2048 HostName;
	CHK_DX(WideCharToMultiByte(CP_ACP, 0, wstrHostname, -1, HostName, sizeof(HostName), 0, 0));

	hostent *pHostEnt = gethostbyname(HostName);
	char *localIP;

	localIP = inet_ntoa(*reinterpret_cast<struct in_addr*>(*pHostEnt->h_addr_list));
	pHostEnt = gethostbyname(pHostEnt->h_name);
	localIP = inet_ntoa(*reinterpret_cast<struct in_addr*>(*pHostEnt->h_addr_list));
	pAddress.set(localIP);

	DWORD dwPort = 0;
	DWORD dwPortSize = sizeof(dwPort);
	DWORD dwPortDataType = DPNA_DATATYPE_DWORD;

	CHK_DX(net_Address_server->GetComponentByName(DPNA_KEY_PORT, &dwPort, &dwPortSize, &dwPortDataType));
	*pPort = dwPort;

	return true;
}
