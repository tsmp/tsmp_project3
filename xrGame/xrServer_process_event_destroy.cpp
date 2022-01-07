#include "stdafx.h"
#include "xrServer.h"
#include "game_sv_single.h"
#include "alife_simulator.h"
#include "xrserver_objects.h"
#include "game_base.h"
#include "game_cl_base.h"
#include "ai_space.h"
#include "alife_object_registry.h"

xr_string xrServer::ent_name_safe(u16 eid)
{
	string1024 buff;
	CSE_Abstract *e_dest = game->get_entity_from_eid(eid);
	if (e_dest)
		sprintf(buff, "[%d][%s:%s]", eid, e_dest->name(), e_dest->name_replace());
	else
		sprintf(buff, "[%d][%s]", eid, "NOTFOUND");

	return buff;
}

void xrServer::Process_event_destroy(NET_Packet &P, ClientID sender, u32 time, u16 ID, NET_Packet *pEPack)
{
	u32 MODE = net_flags(TRUE, TRUE);
	// Parse message
	u16 id_dest = ID;

	CSE_Abstract *e_dest = game->get_entity_from_eid(id_dest); // кто должен быть уничтожен
	if (!e_dest)
	{
		Msg("!SV:ge_destroy: [%d] not found on server", id_dest);
		return;
	};

	R_ASSERT(e_dest);
	xrClientData *c_dest = e_dest->owner; // клиент, чей юнит
	R_ASSERT(c_dest);
	xrClientData *c_from = ID_to_client(sender); // клиент, кто прислал
	R_ASSERT(c_from);

	if (c_dest != c_from && c_from != GetServerClient())
	{
		CSE_Abstract *parent = game->get_entity_from_eid(e_dest->ID_Parent);
		LPCSTR parentName = parent ? parent->name() : "unknown";
		Msg("! ERROR: client [%s] sent destroy for object which owner is [%s]", c_from->ps->getName(), c_dest->ps->getName());
		Msg("object id [%u], name [%s], parent id [%u], name [%s]", id_dest, e_dest->name(), e_dest->ID_Parent, parentName);
		return;
	}

	u16 parent_id = e_dest->ID_Parent;

#ifdef MP_LOGGING
	Msg("--- SV: Process destroy: parent [%d] item [%d][%s]", parent_id, id_dest, e_dest->name());
#endif //#ifdef MP_LOGGING

	NET_Packet P2, *pEventPack = pEPack;
	P2.w_begin(M_EVENT_PACK);
	//---------------------------------------------
	// check if we have children
	if (!e_dest->children.empty())
	{
		if (!pEventPack)
			pEventPack = &P2;

		while (!e_dest->children.empty()) 
		{
			Process_event_destroy(P, sender, time, *e_dest->children.begin(), pEventPack);
			if ((pEventPack->B.count + 100) > NET_PacketSizeLimit)
			{
				SendBroadcast(BroadcastCID, *pEventPack, MODE);
				pEventPack->w_begin(M_EVENT_PACK);
			}
		}
	};

	if (0xffff == parent_id && NULL == pEventPack)
	{
		SendBroadcast(BroadcastCID, P, MODE);
	}
	else
	{
		NET_Packet tmpP;
		if (0xffff != parent_id && Process_event_reject(P, sender, time, parent_id, ID, false))
		{
			game->u_EventGen(tmpP, GE_OWNERSHIP_REJECT, parent_id);
			tmpP.w_u16(id_dest);
			tmpP.w_u8(1);

			if (!pEventPack)
				pEventPack = &P2;

			pEventPack->w_u8(u8(tmpP.B.count));
			pEventPack->w(&tmpP.B.data, tmpP.B.count);
		};

		game->u_EventGen(tmpP, GE_DESTROY, id_dest);

		pEventPack->w_u8(u8(tmpP.B.count));
		pEventPack->w(&tmpP.B.data, tmpP.B.count);
	};

	if (NULL == pEPack && NULL != pEventPack)
	{
		SendBroadcast(BroadcastCID, *pEventPack, MODE);
	}

	// Everything OK, so perform entity-destroy
	if (e_dest->m_bALifeControl && ai().get_alife())
	{
		const game_sv_GameState* pGame = smart_cast<game_sv_GameState*>(game);
		
		R_ASSERT(pGame);
		R_ASSERT(pGame->HasAlifeSimulator());

		if (ai().alife().objects().object(id_dest, true))
			pGame->alife().release(e_dest, false);
	}

	if (game)
		game->OnDestroyObject(e_dest->ID);

	entity_Destroy(e_dest);
}
