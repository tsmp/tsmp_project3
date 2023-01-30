#include "stdafx.h"
#include "xrserver.h"
#include "xrmessages.h"
#include "xrServerRespawnManager.h"

#if 1 //def DEBUG
#define USE_DESIGNER_KEY
#endif

#ifdef USE_DESIGNER_KEY
#include "xrServer_Objects_ALife_Monsters.h"
#endif


extern ObjectRespawnClass ServerRespawnManager;

void xrServer::SLS_Default()
{
	if (game->custom_sls_default())
	{
		game->sls_default();
		return;
	}

#ifdef USE_DESIGNER_KEY
	bool _designer = !!strstr(Core.Params, "-designer");
	CSE_ALifeCreatureActor *_actor = 0;
#endif

	ServerRespawnManager.DestroyRespawner(); // очищаем список респавнера

	string_path fn_spawn;
	if (FS.exist(fn_spawn, "$level$", "level.spawn"))
	{
		IReader *SP = FS.r_open(fn_spawn);
		NET_Packet P;
		u32 S_id;
		for (IReader *S = SP->open_chunk_iterator(S_id); S; S = SP->open_chunk_iterator(S_id, S))
		{
			P.B.count = S->length();
			S->r(P.B.data, P.B.count);

			u16 ID;
			P.r_begin(ID);
			R_ASSERT(M_SPAWN == ID);
			ClientID clientID;
			clientID.set(0);

			CSE_Abstract* entity = Process_spawn(P, clientID);
			if (entity)
			{
				string str = entity->s_name.c_str();

				// TODO: уточнить, какие секции можно добавл€ть в список дл€ респавна. ќЅя«ј“≈Ћ№Ќќ !!! ѕока перенес также, как и в хрее. 

				if (str.find("zone") != string::npos || str.find("mp_actor") != string::npos || str.find("spec") != string::npos || str.find("ligh") != string::npos
					|| str.find("spec") != string::npos || str.find("phys") != string::npos || str.find("m_ph") != string::npos || str.find("scri") != string::npos)
					continue;

				ServerRespawnManager.AddObject(
					entity->s_name.c_str(),
					entity->ID,
					entity->RespawnTime,
					entity->o_Position.x,
					entity->o_Position.y,
					entity->o_Position.z);
			}


#ifdef USE_DESIGNER_KEY
			if (_designer)
			{
				CSE_ALifeCreatureActor *actor = smart_cast<CSE_ALifeCreatureActor *>(entity);
				if (actor)
					_actor = actor;
			}
#endif
		}
		FS.r_close(SP);
	}

#ifdef USE_DESIGNER_KEY
	if (!_designer)
		return;

	if (_actor)
		return;

	_actor = smart_cast<CSE_ALifeCreatureActor *>(entity_Create("actor"));
	_actor->o_Position = Fvector().set(0.f, 0.f, 0.f);
	_actor->set_name_replace("designer");
	_actor->s_flags.flags |= M_SPAWN_OBJECT_ASPLAYER;
	NET_Packet packet;
	packet.w_begin(M_SPAWN);
	_actor->Spawn_Write(packet, TRUE);

	u16 id;
	packet.r_begin(id);
	R_ASSERT(id == M_SPAWN);
	ClientID clientID;
	clientID.set(0);
	Process_spawn(packet, clientID);
#endif
}
