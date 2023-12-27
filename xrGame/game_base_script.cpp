#include "pch_script.h"
#include "game_base.h"
#include "xrServer_script_macroses.h"
#include "../../xrNetwork/client_id.h"

using namespace luabind;

template <typename T>
struct CWrapperBase : public T, public luabind::wrap_base
{
	typedef T inherited;
	typedef CWrapperBase<T> self_type;

	DEFINE_LUA_WRAPPER_METHOD_R2P1_V1(net_Export, NET_Packet)
	DEFINE_LUA_WRAPPER_METHOD_R2P1_V1(net_Import, NET_Packet)
	DEFINE_LUA_WRAPPER_METHOD_V0(clear)
};

#pragma optimize("s", on)
void game_PlayerState::script_register(lua_State *L)
{
	typedef CWrapperBase<game_PlayerState> WrapType;
	typedef game_PlayerState BaseType;

	module(L)
		[luabind::class_<game_PlayerState, WrapType>("game_PlayerState")
			 .def(constructor<>())
			 .def_readonly("team", &BaseType::team)
			 .def_readonly("flags", &BaseType::flags__)
			 .def_readonly("ping", &BaseType::ping)
			 .def_readonly("GameID", &BaseType::GameID)
			 .def_readonly("skin", &BaseType::skin)
			 .def_readonly("RespawnTime", &BaseType::RespawnTime)
			 .def_readonly("m_iSelfKills", &BaseType::m_iSelfKills)
			 .def_readonly("m_iTeamKills", &BaseType::m_iTeamKills)
			 .def_readonly("m_iKillsInRowCurr", &BaseType::m_iKillsInRowCurr)
			 .def_readonly("money_for_round", &BaseType::money_for_round)
			 .def_readonly("money_for_round_team2", &BaseType::money_for_round_team2)
			 .def_readonly("experience_Real", &BaseType::experience_Real)
			 .def_readonly("experience_New", &BaseType::experience_New)
			 .def_readonly("experience_D", &BaseType::experience_D)
			 .def_readonly("rank", &BaseType::rank)
			 .def_readonly("DeathTime", &BaseType::DeathTime)
			 .def_readonly("m_online_time", &BaseType::m_online_time)
			 .def("frags", &BaseType::frags)
			 .def("rivalfrags", &BaseType::rivalfrags)
			 .def("deaths", &BaseType::deaths)
			 .def("aifrags", &BaseType::aifrags)
			 .def("headshots", &BaseType::headshots)
			 .def("maxkillstreak", &BaseType::maxkillstreak)
			 .def("afcount", &BaseType::afcount)

			 .def_readwrite("pItemList", &BaseType::pItemList)
			 .def_readwrite("LastBuyAcount", &BaseType::LastBuyAcount)
			 .def("testFlag", &BaseType::testFlag)
			 .def("setFlag", &BaseType::setFlag)
			 .def("resetFlag", &BaseType::resetFlag)
			 .def("getName", &BaseType::getName)
			 .def("setName", &BaseType::setName)
			 .def("clear", &BaseType::clear, &WrapType::clear_static)
			 .def("net_Export", &BaseType::net_Export, &WrapType::net_Export_static)
			 .def("net_Import", &BaseType::net_Import, &WrapType::net_Import_static)
	];
}

void game_GameState::script_register(lua_State *L)
{

	module(L)
		[luabind::class_<game_GameState, DLL_Pure>("game_GameState")
			 .def(constructor<>())

			 .def_readwrite("type", &game_GameState::m_type)
			 .def_readonly("round", &game_GameState::m_round)
			 .def_readonly("start_time", &game_GameState::m_start_time)

			 .def("Type", &game_GameState::Type)
			 .def("Phase", &game_GameState::Phase)
			 .def("Round", &game_GameState::Round)
			 .def("StartTime", &game_GameState::StartTime)];
}
