#include "stdafx.h"
#include "xrServer.h"
#include "game_sv_single.h"
#include "alife_simulator.h"
#include "xrserver_objects.h"
#include "game_base.h"
#include "game_cl_base.h"
#include "ai_space.h"
#include "alife_object_registry.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "game_sv_deathmatch.h"
#include "Level.h"
#include "Actor.h"
#include "ai_object_location.h"
#include "..\TSMP3_Build_Config.h"
#include "InfoPortion.h"

void xrServer::Process_event(NET_Packet &P, ClientID const &sender)
{
#ifdef SLOW_VERIFY_ENTITIES
	VERIFY(verify_entities());
#endif

	u32 timestamp;
	u16 type;
	u16 destination;
	u32 MODE = net_flags(TRUE, TRUE);

	// correct timestamp with server-unique-time (note: direct message correction)
	P.r_u32(timestamp);

	// read generic info
	P.r_u16(type);
	P.r_u16(destination);

	CSE_Abstract *receiver = game->get_entity_from_eid(destination);
	if (receiver)
	{
		R_ASSERT(receiver->owner);
		receiver->OnEvent(P, type, timestamp, sender);
	};

	switch (type)
	{
	case GE_GAME_EVENT:
	{
		u16 game_event_type;
		P.r_u16(game_event_type);
		game->AddDelayedEvent(P, game_event_type, timestamp, sender);
	}
	break;
		
	case GE_WPN_STATE_CHANGE:
	case GE_ZONE_STATE_CHANGE:
	case GEG_PLAYER_PLAY_HEADSHOT_PARTICLE:
	case GEG_PLAYER_ATTACH_HOLDER:
	case GEG_PLAYER_DETACH_HOLDER:
	case GEG_PLAYER_ACTIVATEARTEFACT:
	case GEG_PLAYER_ITEM2SLOT:
	case GEG_PLAYER_ITEM2BELT:
	case GEG_PLAYER_ITEM2RUCK:
	case GE_GRENADE_EXPLODE:
	case GE_CAR_BEEP:
	case GE_BLOODSUCKER_PREDATOR_CHANGE:
	case GE_CONTROLLER_PSY_FIRE:
		SendBroadcast(BroadcastCID, P, MODE);
		break;

	case GE_INFO_TRANSFER:
	{
		u16 id;
		shared_str info_id;
		u8 add_info;

		u32 idPos = P.r_tell() - sizeof(u16); // позиция id отправителя
		P.r_u16(id); // отправитель
		P.r_stringZ(info_id);
		P.r_u8(add_info); // добавление или убирание информации

		CInfoPortion info_portion;
		info_portion.Load(info_id);

		if(!info_portion.NeedMpSync())
			SendBroadcast(BroadcastCID, P, MODE);
		else
		{
			// глобальный поршень, надо разослать всем акторам
			xr_vector<u16> actors;

			ForEachClientDo([&](IClient* client)
			{
				auto cl = static_cast<xrClientData*>(client);

				if (!cl->ps)
					return;

				u16 gameId = cl->ps->GameID;
					
				if (smart_cast<CActor*>(Level().Objects.net_Find(gameId)))
					actors.push_back(gameId);
			});

			// server
			actors.push_back(Actor()->ID());

			for (u16 actor : actors)
			{
				P.w_seek(idPos, &actor, sizeof(u16));
				SendBroadcast(BroadcastCID, P, MODE);
			}
		}

		break;
	}

	case GE_INV_ACTION:
	{
		xrClientData *CL = ID_to_client(sender);
		if (CL)
			CL->net_Ready = TRUE;
		if (SV_Client)
			SendTo(SV_Client->ID, P, net_flags(TRUE, TRUE));
	}
	break;
	case GE_RESPAWN:
	{
		CSE_Abstract *E = receiver;
		if (E)
		{
			R_ASSERT(E->s_flags.is(M_SPAWN_OBJECT_PHANTOM));

			svs_respawn R;
			R.timestamp = timestamp + E->RespawnTime * 1000;
			R.phantom = destination;
			q_respawn.insert(R);
		}
	}
	break;
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
	{
		Process_event_ownership(P, sender, timestamp, destination);
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case GE_OWNERSHIP_TAKE_MP_FORCED:
	{
		Process_event_ownership(P, sender, timestamp, destination, TRUE);
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
	case GE_LAUNCH_ROCKET:
	{
		Process_event_reject(P, sender, timestamp, destination, P.r_u16());
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case GE_DESTROY:
	{
		Process_event_destroy(P, sender, timestamp, destination, NULL);
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case GE_TRANSFER_AMMO:
	{
		u16 id_entity;
		P.r_u16(id_entity);
		CSE_Abstract *e_parent = receiver;							   // кто забирает (для своих нужд)
		CSE_Abstract *e_entity = game->get_entity_from_eid(id_entity); // кто отдает
		if (!e_entity)
			break;
		if (0xffff != e_entity->ID_Parent)
			break; // this item already taken
		xrClientData *c_parent = e_parent->owner;
		xrClientData *c_from = ID_to_client(sender);
		R_ASSERT(c_from == c_parent); // assure client ownership of event

		// Signal to everyone (including sender)
		SendBroadcast(BroadcastCID, P, MODE);

		// Perfrom real destroy
		entity_Destroy(e_entity);
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case GE_HIT:
	case GE_HIT_STATISTIC:
	{
		P.r_pos -= 2;
		if (type == GE_HIT_STATISTIC)
		{
			P.B.count -= 4;
			P.w_u32(sender.value());
		};
		game->AddDelayedEvent(P, GAME_EVENT_ON_HIT, 0, ClientID());
	}
	break;
	case GE_ASSIGN_KILLER:
	{
		u16 id_src;
		P.r_u16(id_src);

		CSE_Abstract *e_dest = receiver; // кто умер
		// this is possible when hit event is sent before destroy event
		if (!e_dest)
			break;

		CSE_ALifeCreatureAbstract *creature = smart_cast<CSE_ALifeCreatureAbstract *>(e_dest);
		if (creature)
			creature->m_killer_id = id_src;

		//		Msg							("[%d][%s] killed [%d][%s]",id_src,id_src==u16(-1) ? "UNKNOWN" : game->get_entity_from_eid(id_src)->name_replace(),id_dest,e_dest->name_replace());

		break;
	}
	case GE_CHANGE_VISUAL:
	{
		CSE_Visual *visual = smart_cast<CSE_Visual *>(receiver);
		VERIFY(visual);
		string256 tmp;
		P.r_stringZ(tmp);
		visual->set_visual(tmp);
	}
	break;
	case GE_DIE:
	{
		// Parse message
		u16 id_dest = destination, id_src;
		P.r_u16(id_src);

		xrClientData *l_pC = ID_to_client(sender);
		VERIFY(game && l_pC);
		if ((game->Type() != GAME_SINGLE) && l_pC && l_pC->owner)
		{
			Msg("* [%2d] killed by [%2d] - sended by [%s:%2d]", id_dest, id_src, l_pC->name.c_str(), l_pC->owner->ID);
		}

		CSE_Abstract *e_dest = receiver; // кто умер
		// this is possible when hit event is sent before destroy event
		if (!e_dest)
			break;

		if (game->Type() != GAME_SINGLE)
			Msg("* [%2d] is [%s:%s]", id_dest, *e_dest->s_name, e_dest->name_replace());

		CSE_Abstract *e_src = game->get_entity_from_eid(id_src); // кто убил
		if (!e_src)
		{
			xrClientData *C = game->get_client(id_src);
			if (C)
				e_src = C->owner;
		};
		VERIFY(e_src);
		//			R_ASSERT2			(e_dest && e_src, "Killer or/and being killed are offline or not exist at all :(");
		if (game->Type() != GAME_SINGLE)
			Msg("* [%2d] is [%s:%s]", id_src, *e_src->s_name, e_src->name_replace());

		game->on_death(e_dest, e_src);
		xrClientData *c_src = e_src->owner; // клиент, чей юнит убил

		if (c_src->owner->ID == id_src)
		{
			// Main unit
			P.w_begin(M_EVENT);
			P.w_u32(timestamp);
			P.w_u16(type);
			P.w_u16(destination);
			P.w_u16(id_src);
			P.w_clientID(c_src->ID);
		}

		SendBroadcast(BroadcastCID, P, MODE);

		if (game->Type() == GAME_SINGLE)
		{
			P.w_begin(M_EVENT);
			P.w_u32(timestamp);
			P.w_u16(GE_KILL_SOMEONE);
			P.w_u16(id_src);
			P.w_u16(destination);
			SendTo(c_src->ID, P, net_flags(TRUE, TRUE));
		}
		
		DEBUG_VERIFY(verify_entities());
	}
	break;
	case GE_ADDON_ATTACH:
	case GE_ADDON_DETACH:
	case GE_CHANGE_POS:
	{
		SendTo(SV_Client->ID, P, net_flags(TRUE, TRUE));
	}
	break;
	case GEG_PLAYER_WEAPON_HIDE_STATE:
	{
		SendTo(SV_Client->ID, P, net_flags(TRUE, TRUE));

#ifdef SLOW_VERIFY_ENTITIES
		VERIFY(verify_entities());
#endif
	}
	break;

	case GEG_PLAYER_ACTIVATE_SLOT:
	case GEG_PLAYER_ITEM_EAT:
	{
		SendTo(SV_Client->ID, P, net_flags(TRUE, TRUE));

		if (!IsGameTypeSingle() && type == GEG_PLAYER_ITEM_EAT)
			SendTo(sender, P, net_flags(TRUE, TRUE));

#ifdef SLOW_VERIFY_ENTITIES
		VERIFY(verify_entities());
#endif
	}
	break;

	case GEG_PLAYER_ITEM_SELL:
		break;

	case GE_TELEPORT_OBJECT:
	{
		game->teleport_object(P, destination);
	}
	break;
	case GE_ADD_RESTRICTION:
	{
		game->add_restriction(P, destination);
	}
	break;
	case GE_REMOVE_RESTRICTION:
	{
		game->remove_restriction(P, destination);
	}
	break;
	case GE_REMOVE_ALL_RESTRICTIONS:
	{
		game->remove_all_restrictions(P, destination);
	}
	break;
	case GE_MONEY:
	{
		if (IsGameTypeSingle())
		{
			if(CSE_ALifeTraderAbstract *pTa = smart_cast<CSE_ALifeTraderAbstract*>(receiver))
				pTa->m_dwMoney = P.r_u32();
			break;
		}

		if (CSE_ALifeTraderAbstract* pTa = smart_cast<CSE_ALifeTraderAbstract*>(receiver))
		{
			xrClientData* l_pC = ID_to_client(sender);

			if (l_pC && l_pC->owner == receiver)
			{
				pTa->m_dwMoney = P.r_u32();
				l_pC->ps->money_for_round = pTa->m_dwMoney;
				break;
			}
		}

		SendBroadcast(BroadcastCID, P, MODE); // Signal to everyone (including sender)
	}
	break;

	case GE_FREEZE_OBJECT:
		break;

	case GE_CLIENT_SPAWN:
	{
		Fvector3 pos;
		shared_str name_sect;

		P.r_vec3(pos);
		P.r_stringZ(name_sect);

#ifdef PUBLIC_BUILD
		if (!receiver->owner->m_admin_rights.m_has_admin_rights)
		{
			Msg("! Attempt to spawn object from player without admin rights! Section: %s, player name: %s", name_sect.c_str(), receiver->name_replace());
		
			NET_Packet P_answ;
			P_answ.w_begin(M_REMOTE_CONTROL_CMD);
			P_answ.w_stringZ("You dont have admin rights!");
			SendTo(sender, P_answ, net_flags(TRUE, TRUE));
			break;
		}
#endif // PUBLIC_BUILD
				
		if (game_sv_mp* svGame = smart_cast<game_sv_mp*>(Level().Server->game))
			svGame->SpawnObject(name_sect.c_str(), pos, shared_str(nullptr));
	}
	break;

	default:
		R_ASSERT2(0, "Game Event not implemented!!!");
		break;
	}
}
