#include "stdafx.h"
#include "game_sv_single.h"
#include "alife_graph_registry.h"
#include "alife_time_manager.h"
#include "../xrNetwork/net_utils.h"
#include "object_broker.h"
#include "gamepersistent.h"
#include "xrServer.h"
#include "xrApplication.h"

game_sv_Single::game_sv_Single()
{
	m_type = GAME_SINGLE;
}

void game_sv_Single::Create(shared_str &options)
{
	inherited::Create(options);
	switch_Phase(GAME_PHASE_INPROGRESS);
}

ALife::_TIME_ID game_sv_Single::GetGameTime()
{
	if (ai().get_alife() && ai().alife().initialized())
		return ai().alife().time_manager().game_time();

	return inherited::GetGameTime();
}

float game_sv_Single::GetGameTimeFactor()
{
	if (ai().get_alife() && ai().alife().initialized())
		return ai().alife().time_manager().time_factor();

	return inherited::GetGameTimeFactor();
}

void game_sv_Single::SetGameTimeFactor(const float fTimeFactor)
{
	if (ai().get_alife() && ai().alife().initialized())
		return alife().time_manager().set_time_factor(fTimeFactor);

	return inherited::SetGameTimeFactor(fTimeFactor);
}

bool game_sv_Single::change_level(NET_Packet &net_packet, ClientID const &sender)
{
	if (ai().get_alife())
		return alife().change_level(net_packet);

	return true;
}

void game_sv_Single::save_game(NET_Packet &net_packet, ClientID const &sender)
{
	if (ai().get_alife())
		alife().save(net_packet);
}

bool game_sv_Single::load_game(NET_Packet &net_packet, ClientID const &sender)
{
	if (!ai().get_alife())
		return inherited::load_game(net_packet, sender);

	shared_str game_name;
	net_packet.r_stringZ(game_name);
	return (alife().load_game(*game_name, true));
}

void game_sv_Single::switch_distance(NET_Packet &net_packet, ClientID const &sender)
{
	if (ai().get_alife())
		alife().set_switch_distance(net_packet.r_float());
}

void game_sv_Single::sls_default()
{
	alife().update_switch();
}

shared_str game_sv_Single::level_name(const shared_str &server_options) const
{
	if (!ai().get_alife())
		return inherited::level_name(server_options);

	return alife().level_name();
}

void game_sv_Single::restart_simulator(LPCSTR saved_game_name)
{
	shared_str &options = *alife().server_command_line();

	delete_data(m_alife_simulator);
	server().clear_ids();

	strcpy(g_pGamePersistent->m_game_params.m_game_or_spawn, saved_game_name);
	strcpy(g_pGamePersistent->m_game_params.m_new_or_load, "load");

	pApp->LoadBegin();
	m_alife_simulator = xr_new<CALifeSimulator>(&server(), &options);
	g_pGamePersistent->LoadTitle("st_client_synchronising");
	Device.PreCache(30);
	pApp->LoadEnd();
}
