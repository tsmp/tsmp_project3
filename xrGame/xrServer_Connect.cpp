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
	game = nullptr;	

	CLASS_ID clsid = game_GameState::getCLASS_ID(gameType, true);
	game = smart_cast<game_sv_GameState *>(NEW_INSTANCE(clsid));

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
	return IPureServer::Connect(*session_name);
}

IClient *xrServer::new_client(SClientConnectData *cl_data)
{
	IClient *CL = client_Find_Get(cl_data->clientID);
	VERIFY(CL);

	// copy entity
	CL->ID = cl_data->clientID;
	CL->process_id = cl_data->process_id;
	
	string64 new_name;
	strcpy_s(new_name, cl_data->name);
	new_name[63] = '\0';
	Msg("- Connecting player - %s", new_name);

	int nameLength = strlen(new_name);
	
	for (int i = 0; i < nameLength; i++)
	{
		if (new_name[i] == '%')
		{
			new_name[i] = '_';
			Msg("! WARNING: player tried to use %% in nickname!");
		}
	}

	CL->name._set(new_name);

	if (!HasProtected() && game->NewPlayerName_Exists(CL, new_name))
	{
		game->NewPlayerName_Generate(CL, new_name);
		game->NewPlayerName_Replace(CL, new_name);
	}
	CL->name._set(new_name);
	CL->pass._set(cl_data->pass);

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
	CheckClientGameSpyCDKey(CL);
	CheckClientBuildVersion(CL);
	CL->m_guid[0] = 0;
}
