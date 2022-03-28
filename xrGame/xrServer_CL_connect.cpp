#include "stdafx.h"
#include "xrserver.h"
#include "xrmessages.h"
#include "hudmanager.h"
#include "xrserver_objects.h"
#include "Level.h"
#include "../xrNetwork/PlayersBase.h"

xr_vector<u16> g_perform_spawn_ids;

void xrServer::Perform_connect_spawn(CSE_Abstract *E, xrClientData *CL, NET_Packet &P)
{
	xr_vector<u16>::iterator it = std::find(g_perform_spawn_ids.begin(), g_perform_spawn_ids.end(), E->ID);
	if (it != g_perform_spawn_ids.end())
		return;

	g_perform_spawn_ids.push_back(E->ID);

	if (E->net_Processed)
		return;
	if (E->s_flags.is(M_SPAWN_OBJECT_PHANTOM))
		return;

	// Connectivity order
	CSE_Abstract *Parent = ID_to_entity(E->ID_Parent);
	if (Parent)
		Perform_connect_spawn(Parent, CL, P);

	// Process
	Flags16 save = E->s_flags;
	//-------------------------------------------------
	E->s_flags.set(M_SPAWN_UPDATE, TRUE);
	if (0 == E->owner)
	{
		// PROCESS NAME; Name this entity
		if (E->s_flags.is(M_SPAWN_OBJECT_ASPLAYER))
		{
			CL->owner = E;
			E->set_name_replace(*CL->name);
		}

		// Associate
		E->owner = CL;
		E->Spawn_Write(P, TRUE);
		E->UPDATE_Write(P);
	}
	else
	{
		// Just inform
		E->Spawn_Write(P, FALSE);
		E->UPDATE_Write(P);
	}
	//-----------------------------------------------------
	E->s_flags = save;
	SendTo(CL->ID, P, net_flags(TRUE, TRUE));
	E->net_Processed = TRUE;
}

void xrServer::SendConnectionData(IClient *_CL)
{
	g_perform_spawn_ids.clear_not_free();
	xrClientData *CL = (xrClientData *)_CL;
	NET_Packet P;
	u32 mode = net_flags(TRUE, TRUE);
	// Replicate current entities on to this client
	xrS_entities::iterator I = entities.begin(), E = entities.end();
	for (; I != E; ++I)
		I->second->net_Processed = FALSE;
	for (I = entities.begin(); I != E; ++I)
		Perform_connect_spawn(I->second, CL, P);

	// Send "finished" signal
	P.w_begin(M_SV_CONFIG_FINISHED);
	SendTo(CL->ID, P, mode);
};

void xrServer::OnCL_Connected(IClient *_CL)
{
	xrClientData *CL = (xrClientData *)_CL;
	CL->net_Accepted = TRUE;

	Export_game_type(CL);
	Perform_game_export();
	SendConnectionData(CL);

	NET_Packet P;
	P.B.count = 0;
	P.w_clientID(CL->ID);
	P.r_pos = 0;
	ClientID clientID;
	clientID.set(0);
	game->AddDelayedEvent(P, GAME_EVENT_PLAYER_CONNECTED, 0, clientID);
	game->ProcessDelayedEvent();
}

void xrServer::SendConnectResult(IClient *CL, u8 res, u8 res1, char *ResultStr)
{
	NET_Packet P;
	P.w_begin(M_CLIENT_CONNECT_RESULT);
	P.w_u8(res);
	P.w_u8(res1);
	P.w_stringZ(ResultStr);

	if (SV_Client && SV_Client == CL)
		P.w_u8(1);
	else
		P.w_u8(0);

	P.w_stringZ(Level().m_caServerOptions);
	SendTo(CL->ID, P);
}

BOOL g_SV_Disable_Auth_Check = TRUE;

void xrServer::CheckClientBuildVersion(IClient *CL)
{
	if (g_SV_Disable_Auth_Check)
	{
		OnConnectionVerificationStepComplete(CL);
		return;
	}
	
	NET_Packet P;
	P.w_begin(M_AUTH_CHALLENGE);
	SendTo(CL->ID, P);
}

void xrServer::CheckClientHWID(IClient* CL)
{
	if (SV_Client && SV_Client == CL)
	{
		OnConnectionVerificationStepComplete(CL); // HWID
		OnConnectionVerificationStepComplete(CL); // PlayersBase
		return;
	}

	NET_Packet P;
	P.w_begin(M_HW_CHALLENGE);
	SendTo(CL->ID, P);
}

void xrServer::OnHardwareVerifyRespond(IClient* CL, NET_Packet& P)
{
	char ch[10];
	P.r(ch, 10);

	unsigned short hw1;
	memcpy(&hw1, &ch[0], sizeof(hw1));
	unsigned short hw2;
	memcpy(&hw2, &ch[2], sizeof(hw1));
	unsigned short hw3;
	memcpy(&hw3, &ch[4], sizeof(hw1));
	unsigned short hw4;
	memcpy(&hw4, &ch[6], sizeof(hw1));
	unsigned short hw5;
	memcpy(&hw5, &ch[8], sizeof(hw1));
	
	//Msg("His hwid: %hu-%hu-%hu-%hu-%hu", hw1, hw2, hw3, hw4, hw5);
	HWID hwid(hw1, hw2, hw3, hw4, hw5);
	CL->m_HWID = hwid;

	if (GetBannedHW(hwid))
	{
		Msg("! Player banned by hwid tried to connect");
		SendConnectResult(CL, 0, 0, "You are banned.");
		return;
	}
	
	CheckPlayerBannedInBase(CL, this);
	OnConnectionVerificationStepComplete(CL);
}

void xrServer::OnPlayersBaseVerifyRespond(IClient* CL, bool banned)
{
	if (!banned)
	{
		OnConnectionVerificationStepComplete(CL);
		return;
	}
	
	Msg("! Player banned by base tried to connect");
	SendConnectResult(CL, 0, 0, "You are banned!!!");	
}

void xrServer::OnBuildVersionRespond(IClient *CL, NET_Packet &P)
{
	u16 Type;
	P.r_begin(Type);
	u64 _our = FS.auth_get();
	u64 _him = P.r_u64();

#ifdef DEBUG
	Msg("_our = %d", _our);
	Msg("_him = %d", _him);
#endif // DEBUG

	if (_our != _him)	
		SendConnectResult(CL, 0, 0, "Data verification failed. Cheater? [3]");	
	else
	{
		bool bAccessUser = false;
		string512 res_check;

		if (!CL->flags.bLocal)		
			bAccessUser = Check_ServerAccess(CL, res_check);		

		if (CL->flags.bLocal || bAccessUser)		
			OnConnectionVerificationStepComplete(CL);
		else
		{
#pragma TODO("TSMP: Турнирный сервер не будет проверять логин/пароль если отключена проверка ресурсов!")

			Msg(res_check);
			strcat_s(res_check, "Invalid login/password. Client \"");
			strcat_s(res_check, CL->name.c_str());
			strcat_s(res_check, "\" disconnected.");

			SendConnectResult(CL, 0, 2, res_check);
		}
	}
}

void xrServer::OnConnectionVerificationStepComplete(IClient *CL)
{
	CL->verificationStepsCompleted++;

	// проверяем ключ, билд, hwid, бан в базе всего 4
	if (CL->verificationStepsCompleted == 4)
	{
		CL->flags.bVerified = TRUE;
		SendConnectResult(CL, 1, 0, "All Ok");
	}
}
