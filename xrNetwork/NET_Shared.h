#pragma once

#include "net_utils.h"
#include "net_messages.h"

#ifdef XR_NETWORK_EXPORTS
#define XRNETWORK_API __declspec(dllexport)
#else
#define XRNETWORK_API __declspec(dllimport)
#pragma comment(lib, "xrNetwork")
#endif

#include "net_compressor.h"

XRNETWORK_API extern ClientID BroadcastCID;

XRNETWORK_API extern Flags32 psNET_Flags;
XRNETWORK_API extern int psNET_ClientUpdate;
XRNETWORK_API extern int get_psNET_ClientUpdate();
XRNETWORK_API extern int psNET_ClientPending;
XRNETWORK_API extern char psNET_Name[];
XRNETWORK_API extern int psNET_ServerUpdate;
XRNETWORK_API extern int get_psNET_ServerUpdate();
XRNETWORK_API extern int psNET_ServerPending;

XRNETWORK_API extern BOOL psNET_direct_connect;

enum
{
	NETFLAG_MINIMIZEUPDATES = (1 << 0),
	NETFLAG_DBG_DUMPSIZE = (1 << 1),
	NETFLAG_LOG_SV_PACKETS = (1 << 2),
	NETFLAG_LOG_CL_PACKETS = (1 << 3),
};

IC u32 TimeGlobal(CTimer *timer) { return timer->GetElapsed_ms(); }
IC u32 TimerAsync(CTimer *timer) { return TimeGlobal(timer); }

class XRNETWORK_API IClientStatistic
{
	DPN_CONNECTION_INFO ci_last;
	u32 mps_recive, mps_receive_base;
	u32 mps_send, mps_send_base;
	u32 dwBaseTime;
	CTimer *device_timer;

public:
	IClientStatistic(CTimer *timer)
	{
		ZeroMemory(this, sizeof(*this));
		device_timer = timer;
		dwBaseTime = TimeGlobal(device_timer);
	}

	void Update(DPN_CONNECTION_INFO &CI);

	IC u32 getPing() { return ci_last.dwRoundTripLatencyMS; }
	IC u32 getBPS() { return ci_last.dwThroughputBPS; }
	IC u32 getDroppedCount() { return ci_last.dwPacketsDropped; }
	IC u32 getRetriedCount() { return ci_last.dwPacketsRetried; }
	IC u32 getMPS_Receive() { return mps_recive; }
	IC u32 getMPS_Send() { return mps_send; }

	IC void Clear()
	{
		CTimer *timer = device_timer;
		ZeroMemory(this, sizeof(*this));
		device_timer = timer;
		dwBaseTime = TimeGlobal(device_timer);
	}

	//-----------------------------------------------------------------------
	u32 dwTimesBlocked;
	u32 dwBytesSended;
	u32 dwBytesPerSec;
};
