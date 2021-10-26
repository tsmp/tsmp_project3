#include "stdafx.h"
#include "../Console.h"
#include "../Console_commands.h"
#include "level.h"
#include "xrServer.h"
#include "game_cl_base.h"
#include "actor.h"
#include "xrServer_Object_base.h"
#include "RegistryFuncs.h"
#include "gamepersistent.h"
#include "MainMenu.h"
#include "UIGameCustom.h"
#include "game_sv_deathmatch.h"
#include "game_sv_artefacthunt.h"
#include "date_time.h"
#include "game_cl_base_weapon_usage_statistic.h"

extern float g_cl_lvInterp;
extern int g_cl_InterpolationType; //0 - Linear, 1 - BSpline, 2 - HSpline
extern u32 g_cl_InterpolationMaxPoints;
extern string64 gsCDKey;
extern u32 g_dwMaxCorpses;
extern float g_fTimeFactor;
extern BOOL g_b_COD_PickUpMode;
extern int g_iWeaponRemove;
extern int g_iCorpseRemove;
extern BOOL g_bCollectStatisticData;
//extern	BOOL	g_bStatisticSaveAuto	;
extern BOOL g_SV_Disable_Auth_Check;
xr_token game_types[];

extern int g_sv_mp_iDumpStatsPeriod;
extern BOOL g_SV_Force_Artefact_Spawn;
extern int g_Dump_Update_Write;
extern int g_Dump_Update_Read;
extern u32 g_sv_base_dwRPointFreezeTime;
extern int g_sv_base_iVotingEnabled;
extern BOOL g_sv_mp_bSpectator_FreeFly;
extern BOOL g_sv_mp_bSpectator_FirstEye;
extern BOOL g_sv_mp_bSpectator_LookAt;
extern BOOL g_sv_mp_bSpectator_FreeLook;
extern BOOL g_sv_mp_bSpectator_TeamCamera;
extern BOOL g_sv_mp_bCountParticipants;
extern float g_sv_mp_fVoteQuota;
extern float g_sv_mp_fVoteTime;
extern u32 g_sv_dm_dwForceRespawn;
extern s32 g_sv_dm_dwFragLimit;
extern s32 g_sv_dm_dwTimeLimit;
extern BOOL g_sv_dm_bDamageBlockIndicators;
extern u32 g_sv_dm_dwDamageBlockTime;
extern BOOL g_sv_dm_bAnomaliesEnabled;
extern u32 g_sv_dm_dwAnomalySetLengthTime;
extern BOOL g_sv_dm_bPDAHunt;
extern u32 g_sv_dm_dwWarmUp_MaxTime;
extern BOOL g_sv_dm_bDMIgnore_Money_OnBuy;
extern BOOL g_sv_tdm_bAutoTeamBalance;
extern BOOL g_sv_tdm_bAutoTeamSwap;
extern BOOL g_sv_tdm_bFriendlyIndicators;
extern BOOL g_sv_tdm_bFriendlyNames;
extern BOOL g_sv_tdm_bFriendlyFireEnabled;
extern BOOL g_bLeaveTDemo;
extern int g_sv_tdm_iTeamKillLimit;
extern int g_sv_tdm_bTeamKillPunishment;
extern u32 g_sv_ah_dwArtefactRespawnDelta;
extern int g_sv_ah_dwArtefactsNum;
extern u32 g_sv_ah_dwArtefactStayTime;
extern int g_sv_ah_iReinforcementTime;
extern BOOL g_sv_ah_bBearerCantSprint;
extern BOOL g_sv_ah_bShildedBases;
extern BOOL g_sv_ah_bAfReturnPlayersToBases;
extern u32 g_dwDemoDeltaFrame;
extern u32 g_sv_dwMaxClientPing;
extern int g_be_message_out;

extern int g_sv_Skip_Winner_Waiting;
extern int g_sv_Wait_For_Players_Ready;
extern int G_DELAYED_ROUND_TIME;
extern int g_sv_Pending_Wait_Time;
extern int g_sv_Client_Reconnect_Time;
int g_dwEventDelay = 0;

void XRNETWORK_API DumpNetCompressorStats(bool brief);
BOOL XRNETWORK_API g_net_compressor_enabled;
BOOL XRNETWORK_API g_net_compressor_gather_stats;

static xrClientData* GetCommandInitiator(LPCSTR commandLine)
{
	LPCSTR tmp_str = strrchr(commandLine, ' ');

	if (!tmp_str)
		tmp_str = commandLine;

	LPCSTR clientidstr = strstr(tmp_str, RadminIdPrefix);

	if (!clientidstr)
		return nullptr;

	clientidstr += sizeof(RadminIdPrefix) + 1;
	u32 client_id = static_cast<u32>(strtoul(clientidstr, NULL, 10));

	ClientID tmp_id;
	tmp_id.set(client_id);

	if (g_pGameLevel && Level().Server)
		return Level().Server->ID_to_client(tmp_id);
}

ENGINE_API const char* RadminIdPrefix;

class CCC_Restart : public IConsole_Command
{
public:
	CCC_Restart(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;
		if (Level().Server)
		{
			Level().Server->game->round_end_reason = eRoundEnd_GameRestarted;
			Level().Server->game->OnRoundEnd();
		}
	}
	virtual void Info(TInfo &I) { strcpy(I, "restart game"); }
};

class CCC_RestartFast : public IConsole_Command
{
public:
	CCC_RestartFast(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;
		if (Level().Server)
		{
			Level().Server->game->round_end_reason = eRoundEnd_GameRestartedFast;
			Level().Server->game->OnRoundEnd();
		}
	}
	virtual void Info(TInfo &I) { strcpy(I, "restart game fast"); }
};

class CCC_Kill : public IConsole_Command
{
public:
	CCC_Kill(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (IsGameTypeSingle())
			return;

		if (Game().local_player &&
			Game().local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
			return;

		CObject *l_pObj = Level().CurrentControlEntity();
		CActor *l_pPlayer = smart_cast<CActor *>(l_pObj);
		if (l_pPlayer)
		{
			NET_Packet P;
			l_pPlayer->u_EventGen(P, GE_GAME_EVENT, l_pPlayer->ID());
			P.w_u16(GAME_EVENT_PLAYER_KILL);
			P.w_u16(u16(l_pPlayer->ID()));
			l_pPlayer->u_EventSend(P);
		}
	}
	virtual void Info(TInfo &I) { strcpy(I, "player kill"); }
};

class CCC_Net_CL_Resync : public IConsole_Command
{
public:
	CCC_Net_CL_Resync(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		Level().net_Syncronize();
	}
	virtual void Info(TInfo &I) { strcpy(I, "resyncronize client"); }
};

class CCC_Net_CL_ClearStats : public IConsole_Command
{
public:
	CCC_Net_CL_ClearStats(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		Level().ClearStatistic();
	}
	virtual void Info(TInfo &I) { strcpy(I, "clear client net statistic"); }
};

class CCC_Net_SV_ClearStats : public IConsole_Command
{
public:
	CCC_Net_SV_ClearStats(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		Level().Server->ClearStatistic();
	}
	virtual void Info(TInfo &I) { strcpy(I, "clear server net statistic"); }
};

#ifdef DEBUG
class CCC_Dbg_NumObjects : public IConsole_Command
{
public:
	CCC_Dbg_NumObjects(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{

		u32 SVObjNum = (OnServer()) ? Level().Server->GetEntitiesNum() : 0;
		xr_vector<u16> SObjID;

		for (u32 i = 0; i < SVObjNum; i++)
		{
			CSE_Abstract *pEntity = Level().Server->GetEntity(i);
			SObjID.push_back(pEntity->ID);
		}

		std::sort(SObjID.begin(), SObjID.end());

		u32 CLObjNum = Level().Objects.o_count();
		xr_vector<u16> CObjID;

		for (u32 i = 0; i < CLObjNum; i++)		
			CObjID.push_back(Level().Objects.o_get_by_iterator(i)->ID());
		
		std::sort(CObjID.begin(), CObjID.end());

		Msg("Client Objects : %d", CLObjNum);
		Msg("Server Objects : %d", SVObjNum);

		for (u32 CO = 0; CO < _max(CLObjNum, SVObjNum); CO++)
		{
			if (CO < CLObjNum && CO < SVObjNum)
			{
				CSE_Abstract *pEntity = Level().Server->ID_to_entity(SObjID[CO]);
				CObject *pObj = Level().Objects.net_Find(CObjID[CO]);
				char color = (pObj->ID() == pEntity->ID) ? '-' : '!';

				Msg("%c%4d: Client - %20s[%5d] <===> Server - %s [%d]", color, CO + 1,
					*(pObj->cNameSect()), pObj->ID(),
					pEntity->s_name.c_str(), pEntity->ID);
			}
			else
			{
				if (CO < CLObjNum)
				{
					CObject *pObj = Level().Objects.net_Find(CObjID[CO]);
					Msg("! %2d: Client - %s [%d] <===> Server - -----------------", CO + 1,
						*(pObj->cNameSect()), pObj->ID());
				}
				else
				{
					CSE_Abstract *pEntity = Level().Server->ID_to_entity(SObjID[CO]);
					Msg("! %2d: Client - ----- <===> Server - %s [%d]", CO + 1,
						pEntity->s_name.c_str(), pEntity->ID);
				}
			}
		};

		Msg("Client Objects : %d", CLObjNum);
		Msg("Server Objects : %d", SVObjNum);
	}
	virtual void Info(TInfo &I) { strcpy(I, "dbg Num Objects"); }
};
#endif // DEBUG

class CCC_GSCDKey : public CCC_String
{
public:
	CCC_GSCDKey(LPCSTR N, LPSTR V, int _size) : CCC_String(N, V, _size) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR arguments)
	{
		CCC_String::Execute(arguments);

		WriteRegistry_StrValue(REGISTRY_VALUE_GSCDKEY, value);

		if (g_pGamePersistent && MainMenu())
			MainMenu()->ValidateCDKey();
	}
	virtual void Save(IWriter *F){};
};

class CCC_KickPlayerByName : public IConsole_Command
{
public:
	CCC_KickPlayerByName(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;

		if (!xr_strlen(args))
			return;
		if (strchr(args, '/'))
		{
			Msg("!  '/' is not allowed in names!");
			return;
		}
		string4096 PlayerName = "";
		if (xr_strlen(args) > 17)
		{
			strncpy(PlayerName, args, 17);
			PlayerName[17] = 0;
		}
		else
			strcpy(PlayerName, args);

		xr_strlwr(PlayerName);

		Level().Server->clients_Lock();
		u32 cnt = Level().Server->game->get_players_count();
		u32 it;
		for (it = 0; it < cnt; it++)
		{
			xrClientData *l_pC = (xrClientData *)Level().Server->client_Get(it);
			if (l_pC)
			{
				string64 _low_name;
				strcpy(_low_name, l_pC->ps->getName());
				xr_strlwr(_low_name);

				if (!xr_strcmp(_low_name, PlayerName))
				{
					if (Level().Server->GetServerClient() != l_pC)
					{
						Msg("Disconnecting : %s", l_pC->ps->getName());
						Level().Server->DisconnectClient(l_pC);
						break;
					}
					else
						Msg("! Can't disconnect server's client");
				}
			}
		};
		if (it == cnt)
		{
			Msg("! No such player found : %s", PlayerName);
		}
		Level().Server->clients_Unlock();
	};

	virtual void Info(TInfo &I) { strcpy(I, "Kick Player by name"); }
};

class CCC_MakeScreenshot : public IConsole_Command 
{
public:
	CCC_MakeScreenshot(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; bForRadminsOnly = true; bHidden = true; };

	virtual void Execute(LPCSTR args_)
	{
		if (!g_pGameLevel || !Level().Server || !Level().Server->game)
			return;

		u32 len = xr_strlen(args_);
		if ((len == 0) || (len >= 32))
			return;

		ClientID idCL;
		u32 tmpId;

		if (sscanf_s(args_, "%u", &tmpId) != 1)
		{
			Msg("! ERROR: bad command parameters.");
			Msg("Make screenshot. Format: \"make_screenshot <player session id | \'%s\'> <ban_time_in_sec>\".");
			Msg("To receive list of players ids see sv_listplayers");
			return;
		}

		idCL.set(tmpId);		
		xrClientData* clientAdmin = GetCommandInitiator(args_);

		if (!clientAdmin)
		{
			Msg("! ERROR: only radmin can make screenshots ...");
			return;
		}

		Level().Server->MakeScreenshot(clientAdmin->ID, idCL);
	}

	virtual void Info(TInfo& I)
	{
		strcpy(I,"Make screenshot. Format: \"make_screenshot <player session id | \'%s\'> <ban_time_in_sec>\". To receive list of players ids see sv_listplayers");
	}

}; //class CCC_MakeScreenshot

class CCC_ScreenshotAllPlayers : public IConsole_Command 
{
public:
	CCC_ScreenshotAllPlayers(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; bForRadminsOnly = true; bHidden = true; };

	virtual void Execute(LPCSTR args_)
	{
		if (!g_pGameLevel || !Level().Server) 
			return;
		
		xrClientData* radmin = GetCommandInitiator(args_);

		if (!radmin)
		{
			Msg("! ERROR: only radmin can make screenshots (use \"ra login\")");
			return;
		}

		Level().Server->clients_Lock();
		u32 cnt = Level().Server->game->get_players_count();
		u32 it;

		for (it = 0; it < cnt; it++)
		{
			xrClientData* l_pC = (xrClientData*)Level().Server->client_Get(it);
			Level().Server->MakeScreenshot(radmin->ID, l_pC->ID);
		}

		Level().Server->clients_Unlock();
	}

	virtual void Info(TInfo& I)
	{
		strcpy(I, "Make screenshot of each player in the game. Format: \"screenshot_all");
	}

}; //class CCC_ScreenshotAllPlayers

class CCC_BanPlayerByName : public IConsole_Command
{
public:
	CCC_BanPlayerByName(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args_)
	{
		if (!g_pGameLevel || !Level().Server || !Level().Server->game)
			return;
		string4096 buff;
		strcpy(buff, args_);
		u32 len = xr_strlen(buff);

		if (0 == len)
			return;

		string1024 digits;
		LPSTR p = buff + len - 1;
		while (isdigit(*p))
		{
			if (p == buff)
				break;
			--p;
		}
		R_ASSERT(p >= buff);
		strcpy(digits, p);
		*p = 0;
		if (!xr_strlen(buff))
		{
			Msg("incorrect parameter passed. bad name.");
			return;
		}
		u32 ban_time = atol(digits);
		if (ban_time == 0)
		{
			Msg("incorrect parameters passed.  name and time required");
			return;
		}
		string4096 PlayerName = "";
		if (xr_strlen(buff) > 17)
		{

			strncpy(PlayerName, buff, 17);
			PlayerName[17] = 0;
		}
		else
			strcpy(PlayerName, buff);

		xr_strlwr(PlayerName);

		Level().Server->clients_Lock();
		u32 cnt = Level().Server->game->get_players_count();
		u32 it;
		for (it = 0; it < cnt; it++)
		{
			xrClientData *l_pC = (xrClientData *)Level().Server->client_Get(it);
			if (l_pC)
			{
				string64 _low_name;
				strcpy(_low_name, l_pC->ps->getName());
				xr_strlwr(_low_name);

				if (!xr_strcmp(_low_name, PlayerName))
				{
					if (Level().Server->GetServerClient() != l_pC)
					{
						Msg("Disconnecting and Banning: %s", l_pC->ps->getName());
						Level().Server->BanClient(l_pC, ban_time);
						Level().Server->DisconnectClient(l_pC);
						break;
					}
					else
					{
						Msg("! Can't disconnect server's client");
						break;
					}
				}
			}
		};
		if (it == cnt)
			Msg("! No such player found : %s", PlayerName);

		Level().Server->clients_Unlock();
	};

	virtual void Info(TInfo &I) { strcpy(I, "Ban Player by Name"); }
};

class CCC_BanPlayerByIP : public IConsole_Command
{
public:
	CCC_BanPlayerByIP(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args_)
	{
		if (!g_pGameLevel || !Level().Server)
			return;
		//-----------
		string4096 buff;
		strcpy(buff, args_);
		u32 len = xr_strlen(buff);

		if (0 == len)
			return;

		string1024 digits;
		LPSTR p = buff + len - 1;
		while (isdigit(*p))
		{
			if (p == buff)
				break;
			--p;
		}
		R_ASSERT(p >= buff);
		strcpy(digits, p);
		*p = 0;
		if (!xr_strlen(buff))
		{
			Msg("incorrect parameter passed. bad IP address.");
			return;
		}
		u32 ban_time = atol(digits);
		if (ban_time == 0)
		{
			Msg("incorrect parameters passed.  IP and time required");
			return;
		}

		string1024 s_ip_addr;
		strcpy(s_ip_addr, buff);
		//-----------

		ip_address Address;
		Address.set(s_ip_addr);
		Level().Server->clients_Lock();
		Msg("Disconnecting and Banning: %s", Address.to_string().c_str());
		Level().Server->BanAddress(Address, ban_time);
		Level().Server->DisconnectAddress(Address);
		Level().Server->clients_Unlock();
	};

	virtual void Info(TInfo &I) { strcpy(I, "Ban Player by IP"); }
};

class CCC_BanPlayerByHW : public IConsole_Command
{
public:
	CCC_BanPlayerByHW(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args_)
	{
		if (!g_pGameLevel || !Level().Server)
			return;

		unsigned short s1, s2, s3, s4, s5;
		std::string hwstr = args_;
		int delimiters[4] = { 0 };
		int delimsCounter = 0;

		for (int i = 0; i < hwstr.size(); i++)
		{
			if (hwstr[i] == '-')
			{
				delimiters[delimsCounter] = i;
				delimsCounter++;
			}
		}

		if (delimsCounter != 4)
		{
			Msg("! invalid hwid");
			return;
		}

		std::string str1(hwstr.begin(), hwstr.end() - hwstr.size() + delimiters[0]);
		s1 = atoi(str1.c_str());

		std::string str2(hwstr.begin() + delimiters[0] + 1, hwstr.end() - hwstr.size() + delimiters[1]);
		s2 = atoi(str2.c_str());

		std::string str3(hwstr.begin() + delimiters[1] + 1, hwstr.end() - hwstr.size() + delimiters[2]);
		s3 = atoi(str3.c_str());

		std::string str4(hwstr.begin() + delimiters[2] + 1, hwstr.end() - hwstr.size() + delimiters[3]);
		s4 = atoi(str4.c_str());

		std::string str5(hwstr.begin() + delimiters[3] + 1, hwstr.end());
		s5 = atoi(str5.c_str());

		Level().Server->clients_Lock();
		u32 cnt = Level().Server->game->get_players_count();

		HWID hwid(s1, s2, s3, s4, s5);
		Level().Server->BanClientHW(hwid);
		
		for (u32 it = 0; it < cnt; it++)
		{
			xrClientData* l_pC = (xrClientData*)Level().Server->client_Get(it);

			if (l_pC && l_pC->m_HWID == hwid)
			{
				if (Level().Server->GetServerClient() != l_pC)
				{	
						Level().Server->DisconnectClient(l_pC);
						break;
				}
				else
				{
					Msg("! Can't disconnect server's client");
					break;
				}
			}
		}

		Level().Server->clients_Unlock();
	};

	virtual void Info(TInfo& I) { strcpy(I, "Ban Player by IP"); }
};

class CCC_UnBanPlayerByHW : public IConsole_Command
{
public:
	CCC_UnBanPlayerByHW(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args_)
	{
		if (!g_pGameLevel || !Level().Server)
			return;

		unsigned short s1, s2, s3, s4, s5;
		std::string hwstr = args_;
		int delimiters[4] = { 0 };
		int delimsCounter = 0;

		for (int i = 0; i < hwstr.size(); i++)
		{
			if (hwstr[i] == '-')
			{
				delimiters[delimsCounter] = i;
				delimsCounter++;
			}
		}

		if (delimsCounter != 4)
		{
			Msg("! invalid hwid");
			return;
		}

		std::string str1(hwstr.begin(), hwstr.end() - hwstr.size() + delimiters[0]);
		s1 = atoi(str1.c_str());

		std::string str2(hwstr.begin() + delimiters[0] + 1, hwstr.end() - hwstr.size() + delimiters[1]);
		s2 = atoi(str2.c_str());

		std::string str3(hwstr.begin() + delimiters[1] + 1, hwstr.end() - hwstr.size() + delimiters[2]);
		s3 = atoi(str3.c_str());

		std::string str4(hwstr.begin() + delimiters[2] + 1, hwstr.end() - hwstr.size() + delimiters[3]);
		s4 = atoi(str4.c_str());

		std::string str5(hwstr.begin() + delimiters[3] + 1, hwstr.end());
		s5 = atoi(str5.c_str());

		HWID hwid(s1, s2, s3, s4, s5);
		Level().Server->UnBanHW(hwid);	
	};

	virtual void Info(TInfo& I) { strcpy(I, "Ban Player by IP"); }
};

class CCC_UnBanPlayerByIP : public IConsole_Command
{
public:
	CCC_UnBanPlayerByIP(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args)
	{
		if (!g_pGameLevel || !Level().Server)
			return;

		if (!xr_strlen(args))
			return;

		ip_address Address;
		Address.set(args);
		Level().Server->clients_Lock();
		Level().Server->UnBanAddress(Address);
		Level().Server->clients_Unlock();
	};

	virtual void Info(TInfo &I) { strcpy(I, "UnBan Player by IP"); }
};

class CCC_ListPlayers : public IConsole_Command
{
public:
	CCC_ListPlayers(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;

		Msg("------------------------");

		Level().Server->clients_Lock();
		u32	cnt = Level().Server->game->get_players_count();

		Msg("- Total Players : %d", cnt);

		for (u32 it = 0; it < cnt; it++)
		{
			xrClientData* l_pC = (xrClientData*)Level().Server->client_Get(it);
			ip_address Address;

			if (!l_pC)
				continue;		

			Level().Server->GetClientAddress(l_pC->ID, Address, nullptr);

			Msg("%d (name: %s), (session_id: %u), (hash: @), (ip: %s), (ping: %u);"
				, it + 1
				, l_pC->ps->getName()
				, l_pC->ID.value()
				, Address.to_string().c_str()
				, l_pC->ps->ping);
		}

		Level().Server->clients_Unlock();
		Msg("------------------------");
	};

	virtual void Info(TInfo& I) { strcpy(I, "List Players"); }
};

class CCC_ListPlayersHW : public IConsole_Command
{
public:
	CCC_ListPlayersHW(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;

		Msg("------------------------");

		Level().Server->clients_Lock();
		u32	cnt = Level().Server->game->get_players_count();

		Msg("- Total Players : %d", cnt);

		for (u32 it = 0; it < cnt; it++)
		{
			xrClientData* l_pC = (xrClientData*)Level().Server->client_Get(it);
			ip_address Address;

			if (!l_pC)
				continue;

			Level().Server->GetClientAddress(l_pC->ID, Address, nullptr);

			Msg("%d (name: %s), (session_id: %u), (hash: @), (ip: %s), (ping: %u);"
				, it + 1
				, l_pC->ps->getName()
				, l_pC->ID.value()
				, Address.to_string().c_str()
				, l_pC->ps->ping);

			Msg("His hwid: %hu-%hu-%hu-%hu-%hu", l_pC->m_HWID.s1, l_pC->m_HWID.s2, l_pC->m_HWID.s3, l_pC->m_HWID.s4, l_pC->m_HWID.s5);
		}

		Level().Server->clients_Unlock();
		Msg("------------------------");
	};

	virtual void Info(TInfo& I) { strcpy(I, "List Players"); }
};

class CCC_ListPlayersOld : public IConsole_Command
{
public:
	CCC_ListPlayersOld(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;

		Msg("------------------------");

		Level().Server->clients_Lock();
		u32 cnt = Level().Server->game->get_players_count();
		
		Msg("- Total Players : %d", cnt);

		for (u32 it = 0; it < cnt; it++)
		{
			xrClientData *l_pC = (xrClientData *)Level().Server->client_Get(it);

			if (!l_pC)
				continue;

			ip_address Address;
			DWORD dwPort = 0;

			Level().Server->GetClientAddress(l_pC->ID, Address, &dwPort);

			Msg("%d : %s - %s port[%u] ping[%u]", it + 1, l_pC->ps->getName(),
				Address.to_string().c_str(),
				dwPort,
				l_pC->ps->ping);
		}

		Level().Server->clients_Unlock();
		Msg("------------------------");
	};

	virtual void Info(TInfo &I) { strcpy(I, "List Players Original"); }
};

class CCC_ListPlayers_Banned : public IConsole_Command
{
public:
	CCC_ListPlayers_Banned(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;
		Msg("------------------------");
		Level().Server->Print_Banned_Addreses();
		Msg("------------------------");
	};

	virtual void Info(TInfo &I) { strcpy(I, "List of Banned Players"); }
};
class CCC_ChangeLevelGameType : public IConsole_Command
{
public:
	CCC_ChangeLevelGameType(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;

		string256 LevelName;
		LevelName[0] = 0;
		string256 GameType;
		GameType[0] = 0;

		sscanf(args, "%s %s", LevelName, GameType);

		if (!xr_strcmp(GameType, "dm"))
			sprintf_s(GameType, "deathmatch");
		else if (!xr_strcmp(GameType, "tdm"))
			sprintf_s(GameType, "teamdeathmatch");
		else if (!xr_strcmp(GameType, "artifacthunt"))
			sprintf_s(GameType, "artefacthunt");
		else if (!xr_strcmp(GameType, "ah"))
			sprintf_s(GameType, "artefacthunt");
		else if (!xr_strcmp(GameType, "hm"))
			sprintf_s(GameType, "hardmatch");

		if (xr_strcmp(GameType, "deathmatch"))
			if (xr_strcmp(GameType, "teamdeathmatch"))
				if (xr_strcmp(GameType, "artefacthunt"))
					if (xr_strcmp(GameType, "hardmatch"))
					{
						Msg("! Unknown gametype - %s", GameType);
						return;
					};

		s32 GameTypeID = 0;

		if (!xr_strcmp(GameType, "deathmatch"))
			GameTypeID = GAME_DEATHMATCH;
		else if (!xr_strcmp(GameType, "teamdeathmatch"))
			GameTypeID = GAME_TEAMDEATHMATCH;
		else if (!xr_strcmp(GameType, "artefacthunt"))
			GameTypeID = GAME_ARTEFACTHUNT;
		else if (!xr_strcmp(GameType, "hardmatch"))
			GameTypeID = GAME_HARDMATCH;

		const SGameTypeMaps &M = gMapListHelper.GetMapListFor((EGameTypes)GameTypeID);
		u32 cnt = M.m_map_names.size();
		bool bMapFound = false;

		for (u32 i = 0; i < cnt; ++i)
		{
			const shared_str &_map_name = M.m_map_names[i];
			if (0 == xr_strcmp(_map_name.c_str(), LevelName))
			{
				bMapFound = true;
				break;
			}
		}

		if (!bMapFound)
		{
			Msg("! Level [%s] not registered for [%s]!", LevelName, GameType);
#ifdef NDEBUG
			return;
#endif
		}

		NET_Packet P;
		P.w_begin(M_CHANGE_LEVEL_GAME);
		P.w_stringZ(LevelName);
		P.w_stringZ(GameType);
		Level().Send(P);
	};

	virtual void Info(TInfo &I) { strcpy(I, "Changing level and game type"); }
};

class CCC_ChangeGameType : public CCC_ChangeLevelGameType
{
public:
	CCC_ChangeGameType(LPCSTR N) : CCC_ChangeLevelGameType(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args)
	{

		if (!OnServer())
			return;

		string256 GameType;
		GameType[0] = 0;
		sscanf(args, "%s", GameType);

		string1024 argsNew;
		sprintf_s(argsNew, "%s %s", Level().name().c_str(), GameType);

		CCC_ChangeLevelGameType::Execute((LPCSTR)argsNew);
	};

	virtual void fill_tips(vecTips& tips, u32 mode)
	{
		if (g_pGameLevel && Level().Server && OnServer() && Level().Server->game)
		{
			u32 curType = Level().Server->game->Type();

			for (int k = GAME_DEATHMATCH; game_types[k].name; k++)
			{
				TStatus str;

				if (curType == game_types[k].id)
					sprintf_s(str, sizeof(str), "%s  (current game type)", game_types[k].name);
				else
					sprintf_s(str, sizeof(str), "%s", game_types[k].name);
				
				tips.push_back(str);				
			}
		}

		IConsole_Command::fill_tips(tips, mode);
	}

	virtual void Info(TInfo &I) { strcpy(I, "Changing Game Type"); };
};

class CCC_ChangeLevel : public CCC_ChangeLevelGameType
{
public:
	CCC_ChangeLevel(LPCSTR N) : CCC_ChangeLevelGameType(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;

		string256 LevelName;
		LevelName[0] = 0;
		sscanf(args, "%s", LevelName);

		string1024 argsNew;
		sprintf_s(argsNew, "%s %s", LevelName, Level().Server->game->type_name());

		CCC_ChangeLevelGameType::Execute((LPCSTR)argsNew);
	};

	virtual void Info(TInfo &I) { strcpy(I, "Changing Game Type"); }
};

class CCC_AddMap : public IConsole_Command
{
public:
	CCC_AddMap(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args)
	{
		char MapName[256] = {0};
		sscanf(args, "%s", MapName);
		if (!g_pGameLevel || !Level().Server || !Level().Server->game)
			return;
		Level().Server->game->MapRotation_AddMap(MapName);
	};

	virtual void Info(TInfo &I)
	{
		strcpy(I, "Adds map to map rotation list");
	}
};

class CCC_ListMaps : public IConsole_Command
{
public:
	CCC_ListMaps(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!g_pGameLevel || !Level().Server || !Level().Server->game)
			return;
		Level().Server->game->MapRotation_ListMaps();
	};

	virtual void Info(TInfo &I) { strcpy(I, "List maps in map rotation list"); }
};

class CCC_NextMap : public IConsole_Command
{
public:
	CCC_NextMap(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;

		Level().Server->game->OnNextMap();
	};

	virtual void Info(TInfo &I) { strcpy(I, "Switch to Next Map in map rotation list"); }
};

class CCC_PrevMap : public IConsole_Command
{
public:
	CCC_PrevMap(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;

		Level().Server->game->OnPrevMap();
	};

	virtual void Info(TInfo &I) { strcpy(I, "Switch to Previous Map in map rotation list"); }
};

class CCC_AnomalySet : public IConsole_Command
{
public:
	CCC_AnomalySet(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;

		game_sv_Deathmatch *gameDM = smart_cast<game_sv_Deathmatch *>(Level().Server->game);
		if (!gameDM)
			return;

		string256 AnomalySet;
		sscanf(args, "%s", AnomalySet);
		gameDM->StartAnomalies(atol(AnomalySet));
	};

	virtual void Info(TInfo &I) { strcpy(I, "Activating pointed Anomaly set"); }
};

class CCC_Vote_Start : public IConsole_Command
{
public:
	CCC_Vote_Start(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args)
	{
		if (IsGameTypeSingle())
		{
			Msg("! Only for multiplayer games!");
			return;
		}

		if (!Game().IsVotingEnabled())
		{
			Msg("! Voting is disabled by server!");
			return;
		}
		if (Game().IsVotingActive())
		{
			Msg("! There is voting already!");
			return;
		}

		if (Game().Phase() != GAME_PHASE_INPROGRESS)
		{
			Msg("! Voting is allowed only when game is in progress!");
			return;
		};

		Game().SendStartVoteMessage(args);
	};

	virtual void Info(TInfo &I) { strcpy(I, "Starts Voting"); };
};

class CCC_Vote_Stop : public IConsole_Command
{
public:
	CCC_Vote_Stop(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;

		if (IsGameTypeSingle())
		{
			Msg("! Only for multiplayer games!");
			return;
		}

		if (!Level().Server->game->IsVotingEnabled())
		{
			Msg("! Voting is disabled by server!");
			return;
		}

		if (!Level().Server->game->IsVotingActive())
		{
			Msg("! Currently there is no active voting!");
			return;
		}

		if (Level().Server->game->Phase() != GAME_PHASE_INPROGRESS)
		{
			Msg("! Voting is allowed only when game is in progress!");
			return;
		};

		Level().Server->game->OnVoteStop();
	};

	virtual void Info(TInfo &I) { strcpy(I, "Stops Current Voting"); };
};

class CCC_Vote_Yes : public IConsole_Command
{
public:
	CCC_Vote_Yes(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (IsGameTypeSingle())
		{
			Msg("! Only for multiplayer games!");
			return;
		}

		if (!Game().IsVotingEnabled())
		{
			Msg("! Voting is disabled by server!");
			return;
		}

		if (!Game().IsVotingActive())
		{
			Msg("! Currently there is no active voting!");
			return;
		}

		if (Game().Phase() != GAME_PHASE_INPROGRESS)
		{
			Msg("! Voting is allowed only when game is in progress!");
			return;
		};

		Game().SendVoteYesMessage();
	};

	virtual void Info(TInfo &I) { strcpy(I, "Vote Yes"); };
};

class CCC_Vote_No : public IConsole_Command
{
public:
	CCC_Vote_No(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (IsGameTypeSingle())
		{
			Msg("! Only for multiplayer games!");
			return;
		}

		if (!Game().IsVotingEnabled())
		{
			Msg("! Voting is disabled by server!");
			return;
		}

		if (!Game().IsVotingActive())
		{
			Msg("! Currently there is no active voting!");
			return;
		}

		if (Game().Phase() != GAME_PHASE_INPROGRESS)
		{
			Msg("! Voting is allowed only when game is in progress!");
			return;
		};

		Game().SendVoteNoMessage();
	};

	virtual void Info(TInfo &I) { strcpy(I, "Vote No"); };
};

class CCC_StartTimeEnvironment : public IConsole_Command
{
public:
	CCC_StartTimeEnvironment(LPCSTR N) : IConsole_Command(N){};
	virtual void Execute(LPCSTR args)
	{
		u32 year = 1, month = 1, day = 1, hours = 0, mins = 0, secs = 0, milisecs = 0;

		sscanf(args, "%d:%d:%d.%d", &hours, &mins, &secs, &milisecs);
		u64 NewTime = generate_time(year, month, day, hours, mins, secs, milisecs);

		if (!g_pGameLevel)
			return;

		if (!Level().Server)
			return;

		if (!Level().Server->game)
			return;

		Level().Server->game->SetEnvironmentGameTimeFactor(NewTime, g_fTimeFactor);
		Level().Server->game->SetGameTimeFactor(NewTime, g_fTimeFactor);
	}
};
class CCC_SetWeather : public IConsole_Command
{
public:
	CCC_SetWeather(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args)
	{
		if (!g_pGamePersistent)
			return;
		if (!OnServer())
			return;

		string256 weather_name;
		weather_name[0] = 0;
		sscanf(args, "%s", weather_name);
		if (!weather_name[0])
			return;
		g_pGamePersistent->Environment().SetWeather(weather_name);
	};

	virtual void Info(TInfo &I) { strcpy(I, "Set new weather"); }
};

class CCC_SaveStatistic : public IConsole_Command
{
public:
	CCC_SaveStatistic(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		Game().m_WeaponUsageStatistic->SaveData();
	}
	virtual void Info(TInfo &I) { strcpy(I, "saving statistic data"); }
};

class CCC_AuthCheck : public CCC_Integer
{
public:
	CCC_AuthCheck(LPCSTR N, int *V, int _min = 0, int _max = 999) : CCC_Integer(N, V, _min, _max){};
	virtual void Save(IWriter *F){};
};

class CCC_ReturnToBase : public IConsole_Command
{
public:
	CCC_ReturnToBase(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;
		if (GameID() != GAME_ARTEFACTHUNT)
			return;

		game_sv_ArtefactHunt *g = smart_cast<game_sv_ArtefactHunt *>(Level().Server->game);
		g->MoveAllAlivePlayers();
	}
};

class CCC_GetServerAddress : public IConsole_Command
{
public:
	CCC_GetServerAddress(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		ip_address Address;
		DWORD dwPort = 0;

		Level().GetServerAddress(Address, &dwPort);

		Msg("Server Address - %s:%i", Address.to_string().c_str(), dwPort);
	};

	virtual void Info(TInfo &I) { strcpy(I, "List Players"); }
};

class CCC_StartTeamMoney : public IConsole_Command
{
public:
	CCC_StartTeamMoney(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;

		game_sv_mp *pGameMP = smart_cast<game_sv_Deathmatch *>(Level().Server->game);
		if (!pGameMP)
			return;

		string128 Team = "";
		s32 TeamMoney = 0;
		sscanf(args, "%s %i", Team, &TeamMoney);

		if (!Team[0])
		{
			Msg("- --------------------");
			Msg("Teams start money:");
			u32 TeamCount = pGameMP->GetTeamCount();
			for (u32 i = 0; i < TeamCount; i++)
			{
				TeamStruct *pTS = pGameMP->GetTeamData(i);
				if (!pTS)
					continue;
				Msg("Team %d: %d", i, pTS->m_iM_Start);
			}
			Msg("- --------------------");
			return;
		}
		else
		{
			u32 TeamID = 0;
			s32 TeamStartMoney = 0;
			int cnt = sscanf(args, "%i %i", &TeamID, &TeamStartMoney);
			if (cnt != 2)
			{
				Msg("invalid args. (int int) expected");
				return;
			}
			TeamStruct *pTS = pGameMP->GetTeamData(TeamID);
			if (pTS)
				pTS->m_iM_Start = TeamStartMoney;
		}
	};

	virtual void Info(TInfo &I) { strcpy(I, "Set Start Team Money"); }
};
class CCC_SV_Integer : public CCC_Integer
{
public:
	CCC_SV_Integer(LPCSTR N, int *V, int _min = 0, int _max = 999) : CCC_Integer(N, V, _min, _max){};

	virtual void Execute(LPCSTR args)
	{
		CCC_Integer::Execute(args);

		if (!g_pGameLevel || !Level().Server || !Level().Server->game)
			return;

		Level().Server->game->signal_Syncronize();
	}
};

class CCC_SV_Float : public CCC_Float
{
public:
	CCC_SV_Float(LPCSTR N, float *V, float _min = 0, float _max = 1) : CCC_Float(N, V, _min, _max){};

	virtual void Execute(LPCSTR args)
	{
		CCC_Float::Execute(args);
		if (!g_pGameLevel || !Level().Server || !Level().Server->game)
			return;
		Level().Server->game->signal_Syncronize();
	}
};
class CCC_RadminCmd : public IConsole_Command
{
public:
	CCC_RadminCmd(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR arguments)
	{
		if (IsGameTypeSingle())
			return;

		if (strstr(arguments, "login") == arguments)
		{
			string256 user;
			string256 pass;
			if (2 == sscanf(arguments + xr_strlen("login") + 1, "%s %s", user, pass))
			{
				NET_Packet P;
				P.w_begin(M_REMOTE_CONTROL_AUTH);
				P.w_stringZ(user);
				P.w_stringZ(pass);

				Level().Send(P, net_flags(TRUE, TRUE));
			}
			else
				Msg("2 args(user pass) needed");
		}
		else if (strstr(arguments, "logout") == arguments)
		{
			NET_Packet P;
			P.w_begin(M_REMOTE_CONTROL_AUTH);
			P.w_stringZ("logoff");

			Level().Send(P, net_flags(TRUE, TRUE));
		} //logoff
		else
		{
			NET_Packet P;
			P.w_begin(M_REMOTE_CONTROL_CMD);
			P.w_stringZ(arguments);

			Level().Send(P, net_flags(TRUE, TRUE));
		}
	}
	virtual void Save(IWriter *F){};
};

class CCC_SwapTeams : public IConsole_Command
{
public:
	CCC_SwapTeams(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;
		if (Level().Server && Level().Server->game)
		{
			game_sv_ArtefactHunt *pGame = smart_cast<game_sv_ArtefactHunt *>(Level().Server->game);
			if (pGame)
			{
				pGame->SwapTeams();
				Level().Server->game->round_end_reason = eRoundEnd_GameRestartedFast;
				Level().Server->game->OnRoundEnd();
			}
		}
	}
	virtual void Info(TInfo &I) { strcpy(I, "swap teams for artefacthunt game"); }
};

class CCC_Name : public IConsole_Command
{
public:
	CCC_Name(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Status(TStatus &S)
	{
		S[0] = 0;
		if (IsGameTypeSingle())
			return;
		if (!(&Level()))
			return;
		if (!(&Game()))
			return;
		if (!Game().local_player || !Game().local_player->name)
			return;
		sprintf_s(S, "is \"%s\" ", Game().local_player->name);
	}

	virtual void Save(IWriter *F) {}

	virtual void Execute(LPCSTR args)
	{
		if (IsGameTypeSingle())
			return;
		if (!(&Level()))
			return;
		if (!(&Game()))
			return;
		if (!Game().local_player)
			return;

		if (!xr_strlen(args))
			return;
		if (strchr(args, '/'))
		{
			Msg("!  '/' is not allowed in names!");
			return;
		}
		string4096 NewName = "";
		if (xr_strlen(args) > 17)
		{
			strncpy(NewName, args, 17);
			NewName[17] = 0;
		}
		else
			strcpy(NewName, args);

		NET_Packet P;
		Game().u_EventGen(P, GE_GAME_EVENT, Game().local_player->GameID);
		P.w_u16(GAME_EVENT_PLAYER_NAME);
		P.w_stringZ(NewName);
		Game().u_EventSend(P);
	}

	virtual void Info(TInfo &I) { strcpy(I, "player name"); }
};

class CCC_SvStatus : public IConsole_Command
{
public:
	CCC_SvStatus(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;
		if (Level().Server && Level().Server->game)
		{
			Console->Execute("cfg_load all_server_settings");
		}
	}
	virtual void Info(TInfo &I) { strcpy(I, "Shows current server settings"); }
};

class CCC_SvChat : public IConsole_Command
{
public:
	CCC_SvChat(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;
		if (Level().Server && Level().Server->game)
		{
			game_sv_mp *game = smart_cast<game_sv_mp *>(Level().Server->game);
			if (game)
				game->SvSendChatMessage(args);
		}
	}
};

class CCC_SvEventMsg : public IConsole_Command
{
public:
	CCC_SvEventMsg(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };

	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;

		if (!Level().Server || !Level().Server->game)
			return;
		
		game_sv_mp* game = smart_cast<game_sv_mp*>(Level().Server->game);

		if (game)
		{
			NET_Packet netP;
			game->GenerateGameMessage(netP);

			netP.w_u32(GAME_EVENT_SERVER_STRING_MESSAGE);
			netP.w_stringZ(args);

			game->u_EventSend(netP);
		}
	}
};

#include "Actor.h"
#include "ai_object_location.h"

Fvector vec{0, 0, 0};
u16 game_vertex_id = 0;
u32 level_vertex_id = 0;

class CCC_SvSpawn : public IConsole_Command
{
public:
	CCC_SvSpawn(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };

	virtual void fill_tips(vecTips &tips, u32 mode)
	{
		for (auto sect : pSettings->sections())
		{
			if ((sect->line_exist("description") && !sect->line_exist("value") && !sect->line_exist("scheme_index"))
				|| sect->line_exist("species"))
				tips.push_back(sect->Name);
		}
	}

	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;
		
		if (!pSettings->section_exist(args))
		{
			Msg("! cant find section %s", args);
			return;
		}		

		if (Level().Server && Level().Server->game)
		{
			if (game_vertex_id == 0 && level_vertex_id == 0)
			{
				if (!Actor())
				{
					Msg("! error, cant get actor");
					return;
				}

				game_vertex_id = Actor()->ai_location().game_vertex_id();
				level_vertex_id = Actor()->ai_location().level_vertex_id();
			}

			game_sv_Deathmatch *game = smart_cast<game_sv_Deathmatch *>(Level().Server->game);
			if (game)
				game->alife().spawn_item(args, vec, level_vertex_id, game_vertex_id, ALife::_OBJECT_ID(-1));
		}
	}
};

class CCC_ClSpawn : public IConsole_Command
{
public:
	CCC_ClSpawn(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };

	virtual void fill_tips(vecTips &tips, u32 mode)
	{
		for (auto sect : pSettings->sections())
		{
			if ((sect->line_exist("description") && !sect->line_exist("value") && !sect->line_exist("scheme_index"))
				|| sect->line_exist("species"))
				tips.push_back(sect->Name);
		}
	}

	virtual void Execute(LPCSTR args)
	{
		if (!g_pGameLevel) 
			return;

		if (!Level().CurrentControlEntity()) 
			return;

		if (!pSettings->section_exist(args))
		{
			Msg("! Section [%s] isn`t exist...", args);
			return;
		}

		Fvector result;

		if (g_dedicated_server)		
			result = { 0,0,0 };
		else
		{
			collide::rq_result RQ;
			//RayPick(начало луча, направление, максимальная дистанция, тип коллизии, тип столкновения, игнорируемый объект)
			BOOL HasPick = Level().ObjectSpace.RayPick(Device.vCameraPosition, Device.vCameraDirection, 1000.0f, collide::rqtBoth, RQ, Level().CurrentControlEntity());

			if (!HasPick)
			{
				Msg("! Cant pick the place to spawn object");
				return;
			}

			result = Device.vCameraPosition; //начальная позиция
			result.add(Fvector(Device.vCameraDirection).mul(RQ.range)); //умножаем диреккцию столкновения на дистанцию до столкновения добавляем в вектор начальной позиции
			// result == место куда смотрит камера
		}

		if (OnServer())
		{			
			if (game_sv_Deathmatch* tpGame = smart_cast<game_sv_Deathmatch*>(Level().Server->game))
				tpGame->alife().spawn_item(args, result, Actor()->ai_location().level_vertex_id(), Actor()->ai_location().game_vertex_id(), ALife::_OBJECT_ID(-1));
		}
		else
		{
			NET_Packet P;
			CGameObject::u_EventGen(P, GE_CLIENT_SPAWN, Level().CurrentControlEntity()->ID());

			P.w_vec3(result);
			P.w_stringZ(args);

			CGameObject::u_EventSend(P);
		}		
	}
};

extern int fz_downloader_enabled;
extern std::string fz_downloader_mod_name;
extern std::string fz_downloader_reconnect_ip;

class CCC_fz_reconnect_ip : public IConsole_Command
{
public:
	CCC_fz_reconnect_ip(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args_)
	{
		if (!xr_strlen(args_))
		{
			Msg("- fz_downloader_reconnect_ip %s", fz_downloader_reconnect_ip.c_str());
			return;
		}

		fz_downloader_reconnect_ip = args_;
	}

	virtual void Info(TInfo &I) { strcpy(I, "Set reconnect IP for clients who use downloader"); }
};

class CCC_fz_mod_name : public IConsole_Command
{
public:
	CCC_fz_mod_name(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args_)
	{
		if (!xr_strlen(args_))
		{
			Msg("- fz_downloader_mod_name %s", fz_downloader_mod_name.c_str());
			return;
		}

		fz_downloader_mod_name = args_;
	}

	virtual void Info(TInfo &I) { strcpy(I, "Set mod name for downloader"); }
};

class CCC_MpStatistics : public IConsole_Command
{
public:
	CCC_MpStatistics(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!OnServer())
			return;
		if (Level().Server && Level().Server->game)
		{
			Level().Server->game->DumpOnlineStatistic();
		}
	}
	virtual void Info(TInfo &I) { strcpy(I, "Shows current server settings"); }
};

class CCC_CompressorStatus : public IConsole_Command
{
public:
	CCC_CompressorStatus(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR args)
	{
		if (strstr(args, "info_full"))
			DumpNetCompressorStats(false);
		else if (strstr(args, "info"))
			DumpNetCompressorStats(true);
		else
			InvalidSyntax();
	}
	virtual void Info(TInfo &I) { strcpy(I, "valid arguments is [info info_full on off]"); }
};

class CCC_SV_SetMoneyCount : public IConsole_Command
{
public:
	CCC_SV_SetMoneyCount(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };

	virtual void Execute(LPCSTR args_)
	{
		if (!g_pGameLevel || !Level().Server)
			return;

		string128 buff;
		strcpy(buff, args_);
		u32 len = xr_strlen(buff);

		if (!len)
			return;

		string128 digits;
		LPSTR p = buff + len - 1;

		while (isdigit(*p))
		{
			if (p == buff)
				break;

			--p;
		}

		R_ASSERT(p >= buff);
		strcpy(digits, p);

		*p = 0;

		if (!xr_strlen(buff))
		{
			Msg("incorrect parameter passed. bad money count");
			return;
		}

		u32 money_count = atol(digits);

		if (!money_count)
		{
			Msg("incorrect parameters passed.  ID and count required");
			return;
		}

		string128 s_id;
		strcpy(s_id, buff);
		u32 id = static_cast<u32>(atoll(s_id));

		if (!id)
		{
			Msg("invalid id");
			return;
		}

		Level().Server->clients_Lock();
		u32	cnt = Level().Server->game->get_players_count();

		for (u32 it = 0; it < cnt; it++)
		{
			xrClientData* l_pC = (xrClientData*)Level().Server->client_Get(it);

			if (!l_pC)
				continue;

			if (l_pC->ID.value() == id)
			{
				u64 temp = (u64)money_count;

				if (temp > (u64)MAXLONG)
					Msg("! cant set too much money");
				else
					l_pC->ps->money_for_round = (s32)money_count;

				Level().Server->clients_Unlock();
				return;
			}
		}

		Level().Server->clients_Unlock();
		Msg("! client with this id not found");
	};

	virtual void Info(TInfo& I) { strcpy(I, "Set money count by ClientID"); }
};

class CCC_SV_RankUp : public IConsole_Command
{
public:
	CCC_SV_RankUp(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };

	virtual void Execute(LPCSTR args_)
	{
		if (!g_pGameLevel || !Level().Server)
			return;

		string128 s_id;
		strcpy(s_id, args_);
		u32 id = static_cast<u32>(atoll(s_id));

		if (!id)
		{
			Msg("invalid id");
			return;
		}	

		Level().Server->clients_Lock();
		u32	cnt = Level().Server->game->get_players_count();

		for (u32 it = 0; it < cnt; it++)
		{
			xrClientData* l_pC = (xrClientData*)Level().Server->client_Get(it);

			if (!l_pC)
				continue;

			if (l_pC->ID.value() == id)
			{
				if (l_pC->ps->rank < 4)
					l_pC->ps->rank++;
				else
					Msg("! cant increase rank");

				Level().Server->clients_Unlock();
				return;
			}
		}

		Level().Server->clients_Unlock();
		Msg("! client with this id not found");
	};

	virtual void Info(TInfo& I) { strcpy(I, "Increase player rank by ClientID"); }
};

class CCC_SV_RankDown : public IConsole_Command
{
public:
	CCC_SV_RankDown(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };

	virtual void Execute(LPCSTR args_)
	{
		if (!g_pGameLevel || !Level().Server)
			return;

		string128 s_id;
		strcpy(s_id, args_);
		u32 id = static_cast<u32>(atoll(s_id));

		if (!id)
		{
			Msg("invalid id");
			return;
		}

		Level().Server->clients_Lock();
		u32	cnt = Level().Server->game->get_players_count();

		for (u32 it = 0; it < cnt; it++)
		{
			xrClientData* l_pC = (xrClientData*)Level().Server->client_Get(it);

			if (!l_pC)
				continue;

			if (l_pC->ID.value() == id)
			{
				if (l_pC->ps->rank > 0)
					l_pC->ps->rank--;
				else
					Msg("! cant decrease rank");

				Level().Server->clients_Unlock();
				return;
			}
		}

		Level().Server->clients_Unlock();
		Msg("! client with this id not found");
	};

	virtual void Info(TInfo& I) { strcpy(I, "Decrease player rank bu ClientID"); }
};

class CCC_SV_MuteChat : public IConsole_Command
{
public:
	CCC_SV_MuteChat(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };

	virtual void Execute(LPCSTR args_)
	{
		if (!g_pGameLevel || !Level().Server)
			return;

		string128 s_id;
		strcpy(s_id, args_);
		u32 id = static_cast<u32>(atoll(s_id));

		if (!id)
		{
			Msg("invalid id");
			return;
		}

		Level().Server->clients_Lock();
		u32	cnt = Level().Server->game->get_players_count();

		for (u32 it = 0; it < cnt; it++)
		{
			xrClientData* l_pC = (xrClientData*)Level().Server->client_Get(it);

			if (!l_pC)
				continue;

			if (l_pC->ID.value() == id)
			{
				l_pC->bMutedChat = true;
				Level().Server->clients_Unlock();
				return;
			}
		}

		Level().Server->clients_Unlock();
		Msg("! client with this id not found");
	};

	virtual void Info(TInfo& I) { strcpy(I, "Mute chat for player by ClientID"); }
};

class CCC_SV_UnMuteChat : public IConsole_Command
{
public:
	CCC_SV_UnMuteChat(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };

	virtual void Execute(LPCSTR args_)
	{
		if (!g_pGameLevel || !Level().Server)
			return;

		string128 s_id;
		strcpy(s_id, args_);
		u32 id = static_cast<u32>(atoll(s_id));

		if (!id)
		{
			Msg("invalid id");
			return;
		}

		Level().Server->clients_Lock();
		u32	cnt = Level().Server->game->get_players_count();

		for (u32 it = 0; it < cnt; it++)
		{
			xrClientData* l_pC = (xrClientData*)Level().Server->client_Get(it);

			if (!l_pC)
				continue;

			if (l_pC->ID.value() == id)
			{
				l_pC->bMutedChat = false;
				Level().Server->clients_Unlock();
				return;
			}
		}

		Level().Server->clients_Unlock();
		Msg("! client with this id not found");
	};

	virtual void Info(TInfo& I) { strcpy(I, "Unmute chat for player by ClientID"); }
};

std::string server_name = "";
class CCC_SV_SERVERNAME : public IConsole_Command
{
	public:
	CCC_SV_SERVERNAME(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args_)
	{
		if (!xr_strlen(args_))
		{
			Msg("- set default server name");
			server_name = "";
			return;
		}
		server_name = args_;
	}
	virtual void Info(TInfo& I) { strcpy(I, "set server name"); }
};

void register_mp_console_commands()
{
	CMD1(CCC_Restart, "g_restart");
	CMD1(CCC_RestartFast, "g_restart_fast");
	CMD1(CCC_Kill, "g_kill");

	// Net Interpolation
	CMD4(CCC_Float, "net_cl_interpolation", &g_cl_lvInterp, -1, 1);
	CMD4(CCC_Integer, "net_cl_icurvetype", &g_cl_InterpolationType, 0, 2);
	CMD4(CCC_Integer, "net_cl_icurvesize", (int *)&g_cl_InterpolationMaxPoints, 0, 2000);

	CMD1(CCC_Net_CL_Resync, "net_cl_resync");
	CMD1(CCC_Net_CL_ClearStats, "net_cl_clearstats");
	CMD1(CCC_Net_SV_ClearStats, "net_sv_clearstats");

	// Network
#ifdef DEBUG
	CMD4(CCC_Integer, "net_cl_update_rate", &psNET_ClientUpdate, 20, 100);
	CMD4(CCC_Integer, "net_cl_pending_lim", &psNET_ClientPending, 0, 10);
#endif
	CMD4(CCC_Integer, "net_sv_update_rate", &psNET_ServerUpdate, 1, 1000000);
	CMD4(CCC_Integer, "net_sv_pending_lim", &psNET_ServerPending, 0, 10);
	CMD4(CCC_Integer, "net_sv_gpmode", &psNET_GuaranteedPacketMode, 0, 2);
	CMD3(CCC_Mask, "net_sv_log_data", &psNET_Flags, NETFLAG_LOG_SV_PACKETS);
	CMD3(CCC_Mask, "net_cl_log_data", &psNET_Flags, NETFLAG_LOG_CL_PACKETS);
#ifdef DEBUG
	CMD3(CCC_Mask, "net_dump_size", &psNET_Flags, NETFLAG_DBG_DUMPSIZE);
	CMD1(CCC_Dbg_NumObjects, "net_dbg_objects");
#endif // DEBUG
	CMD3(CCC_GSCDKey, "cdkey", gsCDKey, sizeof(gsCDKey));
	CMD4(CCC_Integer, "g_eventdelay", &g_dwEventDelay, 0, 1000);
	CMD4(CCC_Integer, "g_corpsenum", (int *)&g_dwMaxCorpses, 0, 100);

	CMD1(CCC_KickPlayerByName, "sv_kick");
	CMD1(CCC_MakeScreenshot, "make_screenshot");
	CMD1(CCC_ScreenshotAllPlayers, "screenshot_all");

	CMD1(CCC_BanPlayerByHW, "sv_banplayer_hw");
	CMD1(CCC_UnBanPlayerByHW, "sv_unbanplayer_hw");
	CMD1(CCC_BanPlayerByName, "sv_banplayer");
	CMD1(CCC_BanPlayerByIP, "sv_banplayer_ip");

	CMD1(CCC_UnBanPlayerByIP, "sv_unbanplayer_ip");

	CMD1(CCC_ListPlayers, "sv_listplayers");
	CMD1(CCC_ListPlayersHW, "sv_listplayers_hw");
	CMD1(CCC_ListPlayersOld, "sv_listplayers_old");
	CMD1(CCC_ListPlayers_Banned, "sv_listplayers_banned");

	CMD1(CCC_ChangeGameType, "sv_changegametype");
	CMD1(CCC_ChangeLevel, "sv_changelevel");
	CMD1(CCC_ChangeLevelGameType, "sv_changelevelgametype");

	CMD1(CCC_AddMap, "sv_addmap");
	CMD1(CCC_ListMaps, "sv_listmaps");
	CMD1(CCC_NextMap, "sv_nextmap");
	CMD1(CCC_PrevMap, "sv_prevmap");
	CMD1(CCC_AnomalySet, "sv_nextanomalyset");

	CMD1(CCC_Vote_Start, "cl_votestart");
	CMD1(CCC_Vote_Stop, "sv_votestop");
	CMD1(CCC_Vote_Yes, "cl_voteyes");
	CMD1(CCC_Vote_No, "cl_voteno");

	CMD1(CCC_StartTimeEnvironment, "sv_setenvtime");

	CMD1(CCC_SetWeather, "sv_setweather");

	CMD4(CCC_Integer, "cl_cod_pickup_mode", &g_b_COD_PickUpMode, 0, 1);

	CMD4(CCC_Integer, "sv_remove_weapon", &g_iWeaponRemove, -1, 1);
	CMD4(CCC_Integer, "sv_remove_corpse", &g_iCorpseRemove, -1, 1);

	CMD4(CCC_Integer, "sv_statistic_collect", &g_bCollectStatisticData, 0, 1);
	CMD1(CCC_SaveStatistic, "sv_statistic_save");
	//	CMD4(CCC_Integer,		"sv_statistic_save_auto", &g_bStatisticSaveAuto, 0, 1);

	CMD4(CCC_AuthCheck, "sv_no_auth_check", &g_SV_Disable_Auth_Check, 0, 1);

	CMD4(CCC_Integer, "sv_artefact_spawn_force", &g_SV_Force_Artefact_Spawn, 0, 1);

	CMD4(CCC_Integer, "net_dbg_dump_update_write", &g_Dump_Update_Write, 0, 1);
	CMD4(CCC_Integer, "net_dbg_dump_update_read", &g_Dump_Update_Read, 0, 1);

	CMD1(CCC_ReturnToBase, "sv_return_to_base");
	CMD1(CCC_GetServerAddress, "get_server_address");

#ifdef DEBUG
	CMD4(CCC_Integer, "cl_leave_tdemo", &g_bLeaveTDemo, 0, 1);

	CMD4(CCC_Integer, "sv_skip_winner_waiting", &g_sv_Skip_Winner_Waiting, 0, 1);

	CMD4(CCC_Integer, "sv_wait_for_players_ready", &g_sv_Wait_For_Players_Ready, 0, 1);
#endif
	CMD1(CCC_StartTeamMoney, "sv_startteammoney");

	CMD4(CCC_Integer, "sv_hail_to_winner_time", &G_DELAYED_ROUND_TIME, 0, 60);

	//. CMD4(CCC_Integer,		"sv_pending_wait_time",		&g_sv_Pending_Wait_Time, 0, 60000);

	CMD4(CCC_Integer, "sv_client_reconnect_time", &g_sv_Client_Reconnect_Time, 0, 1000000000);

	CMD4(CCC_SV_Integer, "sv_rpoint_freeze_time", (int *)&g_sv_base_dwRPointFreezeTime, 0, 60000);
	CMD4(CCC_SV_Integer, "sv_vote_enabled", &g_sv_base_iVotingEnabled, 0, 0x00FF);

	CMD4(CCC_SV_Integer, "sv_spectr_freefly", (int *)&g_sv_mp_bSpectator_FreeFly, 0, 1);
	CMD4(CCC_SV_Integer, "sv_spectr_firsteye", (int *)&g_sv_mp_bSpectator_FirstEye, 0, 1);
	CMD4(CCC_SV_Integer, "sv_spectr_lookat", (int *)&g_sv_mp_bSpectator_LookAt, 0, 1);
	CMD4(CCC_SV_Integer, "sv_spectr_freelook", (int *)&g_sv_mp_bSpectator_FreeLook, 0, 1);
	CMD4(CCC_SV_Integer, "sv_spectr_teamcamera", (int *)&g_sv_mp_bSpectator_TeamCamera, 0, 1);

	CMD4(CCC_SV_Integer, "sv_vote_participants", (int *)&g_sv_mp_bCountParticipants, 0, 1);
	CMD4(CCC_SV_Float, "sv_vote_quota", &g_sv_mp_fVoteQuota, 0.0f, 1.0f);
	CMD4(CCC_SV_Float, "sv_vote_time", &g_sv_mp_fVoteTime, 0.5f, 10.0f);

	CMD4(CCC_SV_Integer, "sv_forcerespawn", (int *)&g_sv_dm_dwForceRespawn, 0, 3600); //sec
	CMD4(CCC_SV_Integer, "sv_fraglimit", &g_sv_dm_dwFragLimit, 0, 1000000);
	CMD4(CCC_SV_Integer, "sv_timelimit", &g_sv_dm_dwTimeLimit, 0, 1000000); //min
	CMD4(CCC_SV_Integer, "sv_dmgblockindicator", (int *)&g_sv_dm_bDamageBlockIndicators, 0, 1);
	CMD4(CCC_SV_Integer, "sv_dmgblocktime", (int *)&g_sv_dm_dwDamageBlockTime, 0, 600); //sec
	CMD4(CCC_SV_Integer, "sv_anomalies_enabled", (int *)&g_sv_dm_bAnomaliesEnabled, 0, 1);
	CMD4(CCC_SV_Integer, "sv_anomalies_length", (int *)&g_sv_dm_dwAnomalySetLengthTime, 0, 180); //min
	CMD4(CCC_SV_Integer, "sv_pda_hunt", (int *)&g_sv_dm_bPDAHunt, 0, 1);
	CMD4(CCC_SV_Integer, "sv_warm_up", (int *)&g_sv_dm_dwWarmUp_MaxTime, 0, 1000000); //sec

	CMD4(CCC_Integer, "sv_max_ping_limit", (int *)&g_sv_dwMaxClientPing, 1, 2000);

	CMD4(CCC_SV_Integer, "sv_auto_team_balance", (int *)&g_sv_tdm_bAutoTeamBalance, 0, 1);
	CMD4(CCC_SV_Integer, "sv_auto_team_swap", (int *)&g_sv_tdm_bAutoTeamSwap, 0, 1);
	CMD4(CCC_SV_Integer, "sv_friendly_indicators", (int *)&g_sv_tdm_bFriendlyIndicators, 0, 1);
	CMD4(CCC_SV_Integer, "sv_friendly_names", (int *)&g_sv_tdm_bFriendlyNames, 0, 1);
	CMD4(CCC_SV_Integer, "sv_friendly_fire", (int*)&g_sv_tdm_bFriendlyFireEnabled, 0, 1);
	CMD4(CCC_SV_Integer, "sv_teamkill_limit", &g_sv_tdm_iTeamKillLimit, 0, 100);
	CMD4(CCC_SV_Integer, "sv_teamkill_punish", (int *)&g_sv_tdm_bTeamKillPunishment, 0, 1);

	CMD4(CCC_SV_Integer, "sv_artefact_respawn_delta", (int *)&g_sv_ah_dwArtefactRespawnDelta, 0, 600); //sec
	CMD4(CCC_SV_Integer, "sv_artefacts_count", (int *)&g_sv_ah_dwArtefactsNum, 1, 1000000);
	CMD4(CCC_SV_Integer, "sv_artefact_stay_time", (int *)&g_sv_ah_dwArtefactStayTime, 0, 180);	 //min
	CMD4(CCC_SV_Integer, "sv_reinforcement_time", (int *)&g_sv_ah_iReinforcementTime, -1, 3600); //sec
	CMD4(CCC_SV_Integer, "sv_bearercantsprint", (int *)&g_sv_ah_bBearerCantSprint, 0, 1);
	CMD4(CCC_SV_Integer, "sv_shieldedbases", (int *)&g_sv_ah_bShildedBases, 0, 1);
	CMD4(CCC_SV_Integer, "sv_returnplayers", (int *)&g_sv_ah_bAfReturnPlayersToBases, 0, 1);
	CMD1(CCC_SwapTeams, "g_swapteams");
#ifdef DEBUG
	CMD4(CCC_SV_Integer, "sv_demo_delta_frame", (int *)&g_dwDemoDeltaFrame, 0, 100);
	CMD4(CCC_SV_Integer, "sv_ignore_money_on_buy", (int *)&g_sv_dm_bDMIgnore_Money_OnBuy, 0, 1);
#endif

	CMD1(CCC_RadminCmd, "ra");
	CMD1(CCC_Name, "name");
	CMD1(CCC_SvStatus, "sv_status");
	CMD1(CCC_SvChat, "chat");
	CMD1(CCC_SvEventMsg, "event_msg");

	CMD1(CCC_ClSpawn, "cl_spawn");
	CMD1(CCC_SvSpawn, "sv_spawn");
	CMD4(CCC_SV_Float, "sv_spawn_x", &vec.x, -1000000.f, 1000000.0f);
	CMD4(CCC_SV_Float, "sv_spawn_y", &vec.y, -1000000.f, 1000000.0f);
	CMD4(CCC_SV_Float, "sv_spawn_z", &vec.z, -1000000.f, 1000000.0f);

	CMD4(CCC_Integer, "fz_downloader_enabled", (int *)&fz_downloader_enabled, 0, 1);
	CMD1(CCC_fz_reconnect_ip, "fz_downloader_reconnect_ip");
	CMD1(CCC_fz_mod_name, "fz_downloader_mod_name");

	CMD1(CCC_CompressorStatus, "net_compressor_status");
	CMD4(CCC_SV_Integer, "net_compressor_enabled", (int *)&g_net_compressor_enabled, 0, 1);
	CMD4(CCC_SV_Integer, "net_compressor_gather_stats", (int *)&g_net_compressor_gather_stats, 0, 1);
	CMD1(CCC_MpStatistics, "sv_dump_online_statistics");
	CMD4(CCC_SV_Integer, "sv_dump_online_statistics_period", (int *)&g_sv_mp_iDumpStatsPeriod, 0, 60); //min

	CMD1(CCC_SV_MuteChat, "sv_mute_chat");
	CMD1(CCC_SV_UnMuteChat, "sv_unmute_chat");
	CMD1(CCC_SV_RankUp, "sv_rank_up");
	CMD1(CCC_SV_RankDown, "sv_rank_down");
	CMD1(CCC_SV_SetMoneyCount, "sv_set_money");
	CMD1(CCC_SV_SERVERNAME, "sv_servername");
}