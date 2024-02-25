#include "stdafx.h"
#include "xrServer.h"
#include "game_sv_single.h"
#include "alife_simulator.h"
#include "xrserver_objects.h"
#include "game_base.h"
#include "game_cl_base.h"
#include "ai_space.h"
#include "alife_object_registry.h"
#include "xrServerRespawnManager.h"

extern bool IsGameTypeSingle();
extern bool IsGameTypeCoop();

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

void xrServer::Process_event_destroy(NET_Packet &P, ClientID const &sender, u32 time, u16 ID, NET_Packet *pEPack)
{
	const u32 sendFlags = net_flags(TRUE, TRUE);
	u16 destID = ID;
	CSE_Abstract *entityToDestroy = game->get_entity_from_eid(destID);

	if (!entityToDestroy)
	{
		Msg("!SV:ge_destroy: [%d] not found on server", destID);
		return;
	}

	if (!IsGameTypeSingle() && !IsGameTypeCoop())
		m_RespawnerMP.MarkReadyForRespawn(destID);

	R_ASSERT(entityToDestroy);
	xrClientData* clientOwner = entityToDestroy->owner;
	R_ASSERT(clientOwner);
	xrClientData *clientSender = ID_to_client(sender);
	R_ASSERT(clientSender);

	if (clientOwner != clientSender && clientSender != GetServerClient())
	{
		CSE_Abstract *parent = game->get_entity_from_eid(entityToDestroy->ID_Parent);
		LPCSTR parentName = parent ? parent->name() : "unknown";
		Msg("! ERROR: client [%s] sent destroy for object which owner is [%s]", clientSender->ps->getName(), clientOwner->ps->getName());
		Msg("object id [%u], name [%s], parent id [%u], name [%s]", destID, entityToDestroy->name(), entityToDestroy->ID_Parent, parentName);
		return;
	}

	const u16 parentID = entityToDestroy->ID_Parent;

#ifdef MP_LOGGING
	Msg("--- SV: Process destroy: parent [%d] item [%d][%s]", parentID, destID, entityToDestroy->name());
#endif //#ifdef MP_LOGGING

	NET_Packet tempPacket;
	NET_Packet *pEventPack = pEPack;

	if (!pEventPack)
	{
		pEventPack = &tempPacket;
		pEventPack->w_begin(M_EVENT_PACK);
	}

	if (!entityToDestroy->children.empty())
	{
		while (!entityToDestroy->children.empty())
		{
			Process_event_destroy(P, sender, time, *entityToDestroy->children.begin(), pEventPack);

			if ((pEventPack->B.count + 100) > NET_PacketSizeLimit)
			{
				SendBroadcast(BroadcastCID, *pEventPack, sendFlags);
				pEventPack->w_begin(M_EVENT_PACK);
			}
		}
	}

	const u16 invalidID = static_cast<u16>(-1);

	if (parentID == invalidID && !pEventPack)
		SendBroadcast(BroadcastCID, P, sendFlags);
	else
	{
		NET_Packet tmpP;

		if (parentID != invalidID && Process_event_reject(P, sender, time, parentID, ID, false))
		{
			game->u_EventGen(tmpP, GE_OWNERSHIP_REJECT, parentID);
			tmpP.w_u16(destID);
			tmpP.w_u8(1);

			pEventPack->w_u8(u8(tmpP.B.count));
			pEventPack->w(&tmpP.B.data, tmpP.B.count);
		}

		game->u_EventGen(tmpP, GE_DESTROY, destID);

		pEventPack->w_u8(u8(tmpP.B.count));
		pEventPack->w(&tmpP.B.data, tmpP.B.count);
	}

	if (!pEPack && pEventPack)
		SendBroadcast(BroadcastCID, *pEventPack, sendFlags);

	// Everything OK, so perform entity-destroy
	if (entityToDestroy->m_bALifeControl && ai().get_alife())
	{
		const game_sv_GameState* pGame = smart_cast<game_sv_GameState*>(game);
		
		R_ASSERT(pGame);
		R_ASSERT(pGame->HasAlifeSimulator());

		if (ai().alife().objects().object(destID, true))
			pGame->alife().release(entityToDestroy, false);
	}

	if (game)
		game->OnDestroyObject(entityToDestroy->ID);

	entity_Destroy(entityToDestroy);
}
