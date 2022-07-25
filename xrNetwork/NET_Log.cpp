#include "stdafx.h"
#include "net_log.h"
//---------------------------------------------------------
string64 PacketName[] = {
	"M_UPDATE", // DUAL: Update state
	"M_SPAWN",	// DUAL: Spawning, full state

	"M_SV_CONFIG_NEW_CLIENT",
	"M_SV_CONFIG_GAME",
	"M_SV_CONFIG_FINISHED",

	"M_MIGRATE_DEACTIVATE", // TO:   Changing server, just deactivate
	"M_MIGRATE_ACTIVATE",	// TO:   Changing server", full state

	"M_CHAT", // DUAL:

	"M_EVENT",	  // Game Event
	"M_CL_INPUT", // Client Input Data
	//----------- for E3 -----------------------------
	"M_CL_UPDATE",
	"M_UPDATE_OBJECTS",
	//-------------------------------------------------
	"M_CLIENTREADY", // Client has finished to load level and are ready to play

	"M_CHANGE_LEVEL", // changing level
	"M_LOAD_GAME",
	"M_RELOAD_GAME",
	"M_SAVE_GAME",
	"M_SAVE_PACKET",

	"M_SWITCH_DISTANCE",
	"M_GAMEMESSAGE", // Game Message
	"M_EVENT_PACK",	 // Pack of M_EVENT

	//-----------------------------------------------------
	"M_GAMESPY_CDKEY_VALIDATION_CHALLENGE",
	"M_GAMESPY_CDKEY_VALIDATION_CHALLENGE_RESPOND",
	"M_CLIENT_CONNECT_RESULT",
	"M_CLIENT_REQUEST_CONNECTION_DATA",

	"M_CHAT_MESSAGE",
	"M_CLIENT_WARN",
	"M_CHANGE_LEVEL_GAME",
	//-----------------------------------------------------
	"M_CL_PING_CHALLENGE",
	"M_CL_PING_CHALLENGE_RESPOND",
	//-----------------------------------------------------
	"M_AUTH_CHALLENGE",
	"M_CL_AUTH",
	"M_BULLET_CHECK_RESPOND",
	//-----------------------------------------------------
	"M_STATISTIC_UPDATE",
	"M_STATISTIC_UPDATE_RESPOND",
	//-----------------------------------------------------
	"M_PLAYER_FIRE",
	//-----------------------------------------------------
	"M_MOVE_PLAYERS",
	"M_MOVE_PLAYERS_RESPOND",
	"M_CHANGE_SELF_NAME",
	"M_REMOTE_CONTROL_AUTH",
	"M_REMOTE_CONTROL_CMD",
	"M_BATTLEYE",
	"M_MAP_SYNC",
	"M_HW_CHALLENGE",
	"M_HW_RESPOND",
	"M_FILE_TRANSFER",

	"MSG_FORCEDWORD"
};
constexpr size_t PacketTypeSize = sizeof(PacketName) / sizeof(PacketName[0]);

INetLog::INetLog(LPCSTR sFileName, u32 dwStartTime)
#ifdef PROFILE_CRITICAL_SECTIONS
	: m_cs(MUTEX_PROFILE_ID(NET_Log))
#endif // PROFILE_CRITICAL_SECTIONS
{
	strcpy(m_cFileName, sFileName);

	m_pLogFile = nullptr;
	m_pLogFile = FS.w_open(m_cFileName);//fopen(sFileName, "wb");
	m_dwStartTime = 0; //dwStartTime;
}

INetLog::~INetLog()
{
	FlushLog();
	if (m_pLogFile)
		FS.w_close(m_pLogFile);

	m_pLogFile = nullptr;
}

void INetLog::FlushLog()
{
	if (m_pLogFile)
	{
		char buff[100] {0};
		for (xr_vector<SLogPacket>::iterator it = m_aLogPackets.begin(); it != m_aLogPackets.end(); it++)
		{
			SLogPacket *pLPacket = &(*it);
			if (pLPacket->m_u16Type >= PacketTypeSize)
			{
				sprintf(buff, "%s %10d %10d %10d", pLPacket->m_bIsIn ? "In: " : "Out:", pLPacket->m_u32Time, pLPacket->m_u16Type, pLPacket->m_u32Size);
			}
			else
			{
				sprintf(buff, "%s %10d %10s %10d", pLPacket->m_bIsIn ? "In: " : "Out:", pLPacket->m_u32Time, PacketName[pLPacket->m_u16Type], pLPacket->m_u32Size);
			}

			m_pLogFile->w_string(buff);
		};
	};

	m_aLogPackets.clear();
}

void INetLog::LogPacket(u32 Time, NET_Packet *pPacket, bool IsIn)
{
	if (!pPacket)
		return;

	m_cs.Enter();

	SLogPacket NewPacket;

	NewPacket.m_u16Type = *((u16 *)&pPacket->B.data);
	if (NewPacket.m_u16Type >= PacketTypeSize)
	{
		u32 timestamp = 0;
		pPacket->r_u32(timestamp);

		u16 type;
		pPacket->r_u16(type);

		if (!(type >= PacketTypeSize))
			NewPacket.m_u16Type = type;
	}

	NewPacket.m_u32Size = pPacket->B.count;
	NewPacket.m_u32Time = Time - m_dwStartTime;
	NewPacket.m_bIsIn = IsIn;

	m_aLogPackets.push_back(NewPacket);
	if (m_aLogPackets.size() > 100)
		FlushLog();

	m_cs.Leave();
};

void INetLog::LogData(u32 Time, void *data, u32 size, bool IsIn)
{
	if (!data)
		return;

	m_cs.Enter();

	SLogPacket NewPacket;

	NewPacket.m_u16Type = *((u16 *)data);
	if (NewPacket.m_u16Type >= PacketTypeSize)
	{
		u16 type = *((u16*)&(((char*)data)[sizeof(u32)]));

		if (!(type >= PacketTypeSize))
			NewPacket.m_u16Type = type;
	}

	NewPacket.m_u32Size = size;
	NewPacket.m_u32Time = Time - m_dwStartTime;
	NewPacket.m_bIsIn = IsIn;

	m_aLogPackets.push_back(NewPacket);
	if (m_aLogPackets.size() > 100)
		FlushLog();

	m_cs.Leave();
}