#include "stdafx.h"
#include "xrServer.h"
#include "game_sv_single.h"
#include "game_sv_deathmatch.h"
#include "game_sv_teamdeathmatch.h"
#include "game_sv_artefacthunt.h"
#include "xrMessages.h"
#include "game_cl_artefacthunt.h"
#include "game_cl_single.h"
#include "MainMenu.h"
#include "../xrNetwork/PlayersBase.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#include "xrGameSpyServer.h"
#pragma warning(pop)

extern void fz_download_mod(xrServer *server, ClientID const &ID);

xrServer::EConnect xrServer::Connect(shared_str &session_name)
{
#ifdef DEBUG
	Msg("* sv_Connect: %s", *session_name);
#endif

	// Parse options and create game
	if (0 == strchr(*session_name, '/'))
		return ErrConnect;

	string1024 options;
	R_ASSERT2(xr_strlen(session_name) <= sizeof(options), "session_name too BIIIGGG!!!");
	strcpy(options, strchr(*session_name, '/') + 1);

	LPCSTR gameType = g_pGamePersistent->m_game_params.m_game_type;
	CLASS_ID clsid = game_GameState::getCLASS_ID(gameType, true);
	game = smart_cast<game_sv_GameState*>(NEW_INSTANCE(clsid));

	// Options
	if (!game)
		return ErrConnect;

	if (game->Type() != GAME_SINGLE)
	{
		m_file_transfers = xr_new<file_transfer::server_site>();
		initialize_screenshot_proxies();
	}

#ifdef DEBUG
	Msg("* Created server_game %s", game->type_name());
#endif

	game->Create(session_name);

	if (IsGameTypeCoop())
	{
		// Replace save name to level name
		const char* cmdNoSaveName = strstr(session_name.c_str(), "/");
		R_ASSERT(cmdNoSaveName);
		std::string params = cmdNoSaveName;
		params = level_name(session_name).c_str() + params;
		return IPureServer::Connect(params.c_str());
	}

	return IPureServer::Connect(*session_name);
}

IClient *xrServer::new_client(SClientConnectData *clConData)
{
	IClient *CL = client_Find_Get(clConData->clientID);
	VERIFY(CL);

	// copy entity
	CL->ID = clConData->clientID;
	CL->process_id = clConData->process_id;

	string64 newName;
	strcpy_s(newName, clConData->name);
	newName[sizeof(newName) - 1] = '\0';
	Msg("- Connecting player - %s, ip: %s", newName, CL->m_cAddress.to_string().c_str());
	
	for (u32 i = 0, len = xr_strlen(newName); i < len; i++)
	{
		if (newName[i] == '%')
		{
			newName[i] = '_';
			Msg("! WARNING: player tried to use %% in nickname!");
		}
	}

	CL->name._set(newName);

	if (!HasProtected() && game->NewPlayerName_Exists(CL, newName))
	{
		game->NewPlayerName_Generate(CL, newName);
		game->NewPlayerName_Replace(CL, newName);
	}

	CL->name._set(newName);
	CL->pass._set(clConData->pass);

	NET_Packet P;
	P.B.count = 0;
	P.r_pos = 0;

	game->AddDelayedEvent(P, GAME_EVENT_CREATE_CLIENT, 0, CL->ID);	

	if (IsGameTypeSingle())
		Update();

	return CL;
}

void xrServer::AttachNewClient(IClient *CL)
{
	if(!IsGameTypeSingle())
		fz_download_mod(this, CL->ID);

	MSYS_CONFIG msgConfig;
	msgConfig.sign1 = 0x12071980;
	msgConfig.sign2 = 0x26111975;
	msgConfig.is_battleye = 0;

	if (psNET_direct_connect) //single_game
	{
		SV_Client = CL;
		CL->flags.bLocal = 1;
		SendTo_LL(SV_Client->ID, &msgConfig, sizeof(msgConfig), net_flags(TRUE, TRUE, TRUE, TRUE));
	}
	else
	{
		SendTo_LL(CL->ID, &msgConfig, sizeof(msgConfig), net_flags(TRUE, TRUE, TRUE, TRUE));
		Server_Client_Check(CL);
	}

	if (!IsGameTypeSingle())
	{
		int clCnt = g_dedicated_server ? GetClientsCount() - 1 : GetClientsCount();
		
		if (clCnt > smart_cast<xrGameSpyServer*>(this)->m_iMaxPlayers)
		{
			SendConnectResult(CL, 0, 0, "Server is full!");
			return;
		}
	}

	CL->flags.bVerified = FALSE;

	if (CL != SV_Client)
	{
		CheckClientGameSpyCDKey(CL);
		CheckClientBuildVersion(CL);
		CheckClientUID(CL);
	}
	else
		OnConnectionVerificationStepComplete(CL);

	CL->m_guid[0] = 0;
}
