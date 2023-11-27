#include "stdafx.h"
#include "xrserver.h"
#include "xrmessages.h"
#include "xrserver_objects.h"

void xrServer::Perform_transfer(NET_Packet &rejectP, NET_Packet &takeP, CSE_Abstract *what, CSE_Abstract *from, CSE_Abstract *to)
{
	// Sanity check
	R_ASSERT(what && from && to);
	R_ASSERT(from != to);
	R_ASSERT(what->ID_Parent == from->ID);
	u32 time = Device.dwTimeGlobal;

	// 1. Perform migration if need it
	if (from->owner != to->owner)
		PerformMigration(what, from->owner, to->owner);
	//Log						("B");

	// 2. Detach "FROM"
	xr_vector<u16> &C = from->children;
	xr_vector<u16>::iterator c = std::find(C.begin(), C.end(), what->ID);
	R_ASSERT(C.end() != c);
	C.erase(c);

	game_GameState::u_EventGen(rejectP, GE_OWNERSHIP_REJECT, from->ID, time);
	rejectP.w_u16(what->ID);

	// 3. Attach "TO"
	what->ID_Parent = to->ID;
	to->children.push_back(what->ID);

	game_GameState::u_EventGen(takeP, GE_OWNERSHIP_TAKE, to->ID, time + 1);
	takeP.w_u16(what->ID);
}

void xrServer::Perform_reject(CSE_Abstract *what, CSE_Abstract *from, int delta)
{
	R_ASSERT(what && from);
	R_ASSERT(what->ID_Parent == from->ID);

	NET_Packet P;
	u32 time = Device.dwTimeGlobal - delta;

	game_GameState::u_EventGen(P, GE_OWNERSHIP_REJECT, from->ID, time);
	P.w_u16(what->ID);
	P.w_u8(1);

	Process_event_reject(P, BroadcastCID, time, from->ID, what->ID);
}
