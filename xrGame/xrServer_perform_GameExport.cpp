#include "stdafx.h"
#include "xrserver.h"
#include "xrmessages.h"

void xrServer::Perform_game_export()
{
	u32 mode = net_flags(TRUE, TRUE);

	ForEachClientDoSender([&](IClient* client)
	{
		NET_Packet P;
		xrClientData* CL = static_cast<xrClientData*>(client);

		if (!CL->net_Accepted)
			return;

		P.w_begin(M_SV_CONFIG_GAME);
		game->net_Export_State(P, client->ID);
		SendTo(client->ID, P, mode);
	});

	game->sv_force_sync = FALSE;
}

void xrServer::Export_game_type(IClient *CL)
{
	NET_Packet P;
	u32 mode = net_flags(TRUE, TRUE);

	P.w_begin(M_SV_CONFIG_NEW_CLIENT);
	P.w_stringZ(game->type_name());
	SendTo(CL->ID, P, mode);
}
