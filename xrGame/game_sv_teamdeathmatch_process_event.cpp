#include "stdafx.h"
#include "game_sv_teamdeathmatch.h"
#include "xrServer.h"
#include "xrMessages.h"

void game_sv_TeamDeathmatch::OnEvent(NET_Packet &P, u16 type, u32 time, ClientID const &sender)
{
	inherited::OnEvent(P, type, time, sender);
}
