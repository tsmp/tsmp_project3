#include "StdAfx.h"
#include "game_sv_Carfight.h"
#include "xrserver_objects_alife_monsters.h"
#include "xrServer.h"
#include "Level.h"

game_sv_Carfight::game_sv_Carfight()
{
	m_type = GAME_CARFIGHT;
}

void game_sv_Carfight::CleanClientData(xrClientData* xrCData)
{
	DestroyCarOfPlayer(xrCData->ps);
}

CSE_Abstract* game_sv_Carfight::SpawnCar(Fvector pos, Fvector angle)
{
	CSE_Abstract* E = nullptr;
	E = spawn_begin("m_car");
	E->s_flags.assign(M_SPAWN_OBJECT_LOCAL);

	E->o_Position.set(pos);
	E->o_Angle.set(angle);

	CSE_Visual* pV = smart_cast<CSE_Visual*>(E);
	pV->set_visual("physics\\vehicles\\btr\\veh_btr_u_01.ogf");

	CSE_Abstract* spawned = spawn_end(E, m_server->GetServerClient()->ID);
	signal_Syncronize();
	return spawned;
}

void game_sv_Carfight::DestroyCarOfPlayer(game_PlayerState* ps)
{
	if (ps->CarID == u16(-1))
		return;

	NET_Packet P;
	u_EventGen(P, GE_DESTROY, ps->CarID);
	Level().Send(P);
	ps->CarID = u16(-1);
}

bool game_sv_Carfight::SpawnPlayerInCar(ClientID const& playerId)
{
	xrClientData* xrCData = m_server->ID_to_client(playerId);
	if (!xrCData || !xrCData->ps)
		return false;

#pragma TODO("TSMP: Убрать костыль :)")
	CSE_ALifeCreatureActor fakeAct("mp_actor");
	fakeAct.s_team = xrCData->ps->team;
	assign_RP(&fakeAct, xrCData->ps);

	CSE_Abstract* car = SpawnCar(fakeAct.position(), fakeAct.angle());
	R_ASSERT(smart_cast<CSE_ALifeCar*>(car));

	xrCData->ps->resetFlag(GAME_PLAYER_FLAG_SPECTATOR);
	xrCData->ps->CarID = car->ID;

	xrCData->ps->skin = u8(::Random.randI((int)TeamList[xrCData->ps->team].aSkins.size()));
	SpawnPlayer(playerId, "mp_actor", car->ID);
	return true;
}

#pragma TODO("TSMP: переписать без дублирования этого кода в базовом классе")
void game_sv_Carfight::RespawnPlayer(ClientID const& id_who, bool NoSpectator)
{
	xrClientData* xrCData = m_server->ID_to_client(id_who);
	if (!xrCData || !xrCData->owner)
		return;

	CSE_Abstract* pOwner = xrCData->owner;
	CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(pOwner);
	CSE_Spectator* pS = smart_cast<CSE_Spectator*>(pOwner);

	if (pA)
	{
		AllowDeadBodyRemove(xrCData->ps->GameID);
	}

	if (pA && !NoSpectator)
		SpawnPlayer(id_who, "spectator");
	else
	{
		if (pOwner->owner != m_server->GetServerClient())
			pOwner->owner = (xrClientData*)m_server->GetServerClient();

		//remove spectator entity
		if (pS)
		{
			NET_Packet P;
			u_EventGen(P, GE_DESTROY, pS->ID);
			Level().Send(P, net_flags(TRUE, TRUE));
		}

		SpawnPlayerInCar(id_who);
	}
}
