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
	const u32 time = Device.dwTimeGlobal;

	// Detach "FROM"
	xr_vector<u16> &childrenFrom = from->children;
	auto childFrom = std::find(childrenFrom.begin(), childrenFrom.end(), what->ID);
	R_ASSERT(childFrom != childrenFrom.end());
	childrenFrom.erase(childFrom);

	game_GameState::u_EventGen(rejectP, GE_OWNERSHIP_REJECT, from->ID, time);
	rejectP.w_u16(what->ID);

	// Attach "TO"
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
	const u32 time = Device.dwTimeGlobal - delta;

	game_GameState::u_EventGen(P, GE_OWNERSHIP_REJECT, from->ID, time);
	P.w_u16(what->ID);
	P.w_u8(1);

	Process_event_reject(P, BroadcastCID, time, from->ID, what->ID);
}
