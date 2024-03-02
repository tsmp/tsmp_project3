#include "stdafx.h"
#include "xrServer.h"
#include "LevelGameDef.h"
#include "script_process.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "script_engine.h"
#include "script_engine_space.h"
#include "level.h"
#include "xrserver.h"
#include "ai_space.h"
#include "game_sv_event_queue.h"
#include "Console.h"
#include "Console_commands.h"
#include "string_table.h"
#include "object_broker.h"
#include "debug_renderer.h"

#include "Actor.h"
#include "ai_object_location.h"
#include "clsid_game.h"
#include "alife_object_registry.h"
#include "alife_graph_registry.h"
#include "IGame_Persistent.h"
#include "xrApplication.h"

ENGINE_API bool g_dedicated_server;

#define MAPROT_LIST_NAME "maprot_list.ltx"
string_path MAPROT_LIST = "";
BOOL net_sv_control_hit = FALSE;
BOOL g_bCollectStatisticData = FALSE;
BOOL g_sv_crosshair_players_names = FALSE;
BOOL g_sv_crosshair_color_players = FALSE;

u32 g_sv_base_dwRPointFreezeTime = 0;
int g_sv_base_iVotingEnabled = 0x00ff;

xr_token round_end_result_str[] =
{
	{"Finish", eRoundEnd_Finish},
	{"Game restarted", eRoundEnd_GameRestarted},
	{"Game restarted fast", eRoundEnd_GameRestartedFast},
	{"Time limit", eRoundEnd_TimeLimit},
	{"Frag limit", eRoundEnd_FragLimit},
	{"Artefact limit", eRoundEnd_ArtrefactLimit},
	{"Unknown", eRoundEnd_Force},
	{0, 0}
};

game_PlayerState *game_sv_GameState::get_id(ClientID const &id)
{
	if (xrClientData* C = m_server->ID_to_client(id))
		return C->ps;

	return nullptr;
}

LPCSTR game_sv_GameState::get_name_id(ClientID const &id)
{
	if (xrClientData* C = m_server->ID_to_client(id))
		return *C->name;

	return nullptr;
}

LPCSTR game_sv_GameState::get_player_name_id(ClientID const &id)
{
	if (xrClientData* xrCData = m_server->ID_to_client(id))
		return xrCData->name.c_str();

	return "unknown";
}

u32 game_sv_GameState::get_players_count()
{
	return m_server->GetClientsCount();
}

u16 game_sv_GameState::get_id_2_eid(ClientID const &id)
{
	xrClientData *C = (xrClientData *)m_server->ID_to_client(id);

	if (!C)
		return 0xffff;

	CSE_Abstract *E = C->owner;

	if (!E)
		return 0xffff;

	return E->ID;
}

game_PlayerState *game_sv_GameState::get_eid(u16 id) //if exist
{
	if (xrClientData* data = get_client(id))
		return data->ps;

	return nullptr;
}

xrClientData *game_sv_GameState::get_client(u16 id) //if exist
{
	CSE_Abstract *entity = get_entity_from_eid(id);

	if (entity && entity->owner && entity->owner->ps && entity->owner->ps->GameID == id)
		return entity->owner;

	IClient* cl = m_server->FindClient([&](IClient* client)
	{
		xrClientData* cldata = static_cast<xrClientData*>(client);
		game_PlayerState* ps = cldata->ps;

		if (!ps)
			return false;

		return ps->HasOldID(id);
	});

	if (!cl)
		return nullptr;

	return static_cast<xrClientData*>(cl);
}

CSE_Abstract *game_sv_GameState::get_entity_from_eid(u16 id)
{
	return m_server->ID_to_entity(id);
}

// Utilities
u32 game_sv_GameState::get_alive_count(u32 team)
{
	u32 cnt = get_players_count();
	u32 alive = 0;

	m_server->ForEachClientDo([&alive,&team](IClient* client)
	{
		xrClientData* cldata = static_cast<xrClientData*>(client);
		game_PlayerState* ps = cldata->ps;

		if (!ps)
			return;

		if (u32(ps->team) == team)
			alive += (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)) ? 0 : 1;
	});

	return alive;
}

s32 game_sv_GameState::get_option_i(LPCSTR lst, LPCSTR name, s32 def)
{
	string64 op;
	strconcat(sizeof(op), op, "/", name, "=");
	if (strstr(lst, op))
		return atoi(strstr(lst, op) + xr_strlen(op));
	else
		return def;
}

float game_sv_GameState::get_option_f(LPCSTR lst, LPCSTR name, float def)
{
	string64 op;
	strconcat(sizeof(op), op, "/", name, "=");
	LPCSTR found = strstr(lst, op);

	if (found)
	{
		float val;
		int cnt = sscanf(found + xr_strlen(op), "%f", &val);
		VERIFY(cnt == 1);
		return val;
		//.		return atoi	(strstr(lst,op)+xr_strlen(op));
	}
	else
		return def;
}

string64 &game_sv_GameState::get_option_s(LPCSTR lst, LPCSTR name, LPCSTR def)
{
	static string64 ret;

	string64 op;
	strconcat(sizeof(op), op, "/", name, "=");
	LPCSTR start = strstr(lst, op);
	if (start)
	{
		LPCSTR begin = start + xr_strlen(op);
		sscanf(begin, "%[^/]", ret);
	}
	else
	{
		if (def)
			strcpy(ret, def);
		else
			ret[0] = 0;
	}
	return ret;
}
void game_sv_GameState::signal_Syncronize()
{
	sv_force_sync = TRUE;
}

// Network
void game_sv_GameState::net_Export_State(NET_Packet &P, ClientID const &to)
{
	// Generic
	P.w_clientID(to);
	P.w_s32(m_type);
	P.w_u16(m_phase);
	P.w_s32(m_round);
	P.w_u32(m_start_time);
	P.w_u8(u8(g_sv_base_iVotingEnabled & 0xff));
	P.w_u8(u8(net_sv_control_hit));
	P.w_u8(u8(g_bCollectStatisticData));
	P.w_u8(u8(g_sv_crosshair_players_names));
	P.w_u8(u8(g_sv_crosshair_color_players));

	// Players
	const u32 playersCountPos = P.B.count;
	P.w_u16(0); // players count fake

	u16 playersCount = 0;
	game_PlayerState *Base = get_id(to);

	m_server->ForEachClientDo([&](IClient* client)
	{
		string64 p_name;
		xrClientData *C = static_cast<xrClientData*>(client);
		game_PlayerState *A = C->ps;

		if (!C->net_Ready || (A->IsSkip() && C->ID != to))
			return;

		if(CSE_Abstract *C_e = C->owner)
			strcpy(p_name, C_e->name_replace());
		else
			strcpy(p_name, "Unknown");	

		A->setName(p_name);
		u16 tmp_flags = A->flags__;

		if (Base == A)
			A->setFlag(GAME_PLAYER_FLAG_LOCAL);

		ClientID clientID = C->ID;
		P.w_clientID(clientID);
		A->net_Export(P, TRUE);
		A->flags__ = tmp_flags;
		playersCount++;
	});

	P.w_seek(playersCountPos, &playersCount, sizeof(playersCount)); // players count real
	net_Export_GameTime(P);
}

void game_sv_GameState::net_Export_Update(NET_Packet &P, ClientID const &idTo)
{
	game_PlayerState* ps = get_id(idTo);
	R_ASSERT2(ps, "Can not get player state in net_Export_Update");

	// ���� ����� ����� ������, �������� �����������. ��������� ��� ���� �����������, ����� �� ������������ ���� � ���.
	u16 flagsBackup = ps->flags__;
	ps->setFlag(GAME_PLAYER_FLAG_LOCAL);

	P.w_clientID(idTo);
	ps->net_Export(P);

	// ������ ���� �����������, ���� �������������
	ps->flags__ = flagsBackup;

	net_Export_GameTime(P);
}

void game_sv_GameState::net_Export_GameTime(NET_Packet &P)
{
	// Syncronize GameTime
	P.w_u64(GetGameTime());
	P.w_float(GetGameTimeFactor());

	// Syncronize EnvironmentGameTime
	P.w_u64(GetEnvironmentGameTime());
	P.w_float(GetEnvironmentGameTimeFactor());
}

void game_sv_GameState::OnPlayerConnect(ClientID const &id_who)
{
	signal_Syncronize();
}

void game_sv_GameState::OnPlayerDisconnect(ClientID const &id_who, LPSTR, u16)
{
	signal_Syncronize();
}

bool IsCompatibleSpawnGameType(u8 spawnGameType, u32 currentGameType)
{
	if (spawnGameType == rpgtGameDeathmatch && currentGameType == GAME_DEATHMATCH)
		return true;

	if (spawnGameType == rpgtGameTeamDeathmatch && (currentGameType == GAME_TEAMDEATHMATCH || currentGameType == GAME_CARFIGHT))
		return true;

	if (spawnGameType == rpgtGameArtefactHunt && currentGameType == GAME_ARTEFACTHUNT)
		return true;

	// Freeplay, hardmatch, race use deathmatch rpoints
	if (spawnGameType == rpgtGameDeathmatch && currentGameType == GAME_FREEPLAY)
		return true;

	if (spawnGameType == rpgtGameDeathmatch && currentGameType == GAME_HARDMATCH)
		return true;

	if (spawnGameType == rpgtGameDeathmatch && currentGameType == GAME_RACE)
		return true;

	return false;
}

static float rpoints_Dist[TEAM_COUNT] = {1000.f, 1000.f, 1000.f, 1000.f};
void game_sv_GameState::Create(shared_str &options)
{
	string_path fn_game;
	if (FS.exist(fn_game, "$level$", "level.game"))
	{
		IReader *F = FS.r_open(fn_game);
		IReader *O = 0;

		// Load RPoints
		if (0 != (O = F->open_chunk(RPOINT_CHUNK)))
		{
			for (int id = 0; O->find_chunk(id); ++id)
			{
				RPoint R;
				u8 team;
				u8 type;
				u8 GameType;

				O->r_fvector3(R.P);
				O->r_fvector3(R.A);
				team = O->r_u8();
				VERIFY(team >= 0 && team < 4);
				type = O->r_u8();
				GameType = O->r_u8();
				O->r_u8();

				if (GameType != rpgtGameAny && !IsCompatibleSpawnGameType(GameType, Type()))
					continue;				

				switch (type)
				{
				case rptActorSpawn:
				{
					rpoints[team].push_back(R);
					for (int i = 0; i < int(rpoints[team].size()) - 1; i++)
					{
						RPoint rp = rpoints[team][i];
						float dist = R.P.distance_to_xz(rp.P) / 2;
						if (dist < rpoints_MinDist[team])
							rpoints_MinDist[team] = dist;
						dist = R.P.distance_to(rp.P) / 2;
						if (dist < rpoints_Dist[team])
							rpoints_Dist[team] = dist;
					};
				}
				break;
				};
			};
			O->close();
		}

		FS.r_close(F);
	}

	// loading scripts
	ai().script_engine().remove_script_process(ScriptEngine::eScriptProcessorGame);
	string_path S;
	FS.update_path(S, "$game_config$", "script.ltx");
	CInifile* l_tpIniFile = xr_new<CInifile>(S);
	R_ASSERT(l_tpIniFile);

	if (l_tpIniFile->section_exist(type_name()))
	{
		if (l_tpIniFile->r_string(type_name(), "script"))
			ai().script_engine().add_script_process(ScriptEngine::eScriptProcessorGame, xr_new<CScriptProcess>("game", l_tpIniFile->r_string(type_name(), "script")));
		else
			ai().script_engine().add_script_process(ScriptEngine::eScriptProcessorGame, xr_new<CScriptProcess>("game", ""));
	}

	xr_delete(l_tpIniFile);
	ConsoleCommands_Create();
	LPCSTR svcfg_ltx_name = "-svcfg ";

	if (strstr(Core.Params, svcfg_ltx_name))
	{
		string_path svcfg_name = "";
		int sz = xr_strlen(svcfg_ltx_name);
		sscanf(strstr(Core.Params, svcfg_ltx_name) + sz, "%[^ ] ", svcfg_name);
		Console->ExecuteScript(svcfg_name);		
	}
	
	ReadOptions(options);

	if (strstr(*options, "/alife"))
		m_alife_simulator = xr_new<CALifeSimulator>(&server(), &options);
}

void game_sv_GameState::ReadOptions(shared_str &options)
{
	g_sv_base_dwRPointFreezeTime = get_option_i(*options, "rpfrz", g_sv_base_dwRPointFreezeTime / 1000) * 1000;
	FS.update_path(MAPROT_LIST, "$app_data_root$", MAPROT_LIST_NAME);

	if (FS.exist(MAPROT_LIST))
		Console->ExecuteScript(MAPROT_LIST);

	g_sv_base_iVotingEnabled = get_option_i(*options, "vote", (g_sv_base_iVotingEnabled));

	// Convert old vote param
	if (g_sv_base_iVotingEnabled != 0)
	{
		if (g_sv_base_iVotingEnabled == 1)
			g_sv_base_iVotingEnabled = 0x00ff;
	}
}

static bool g_bConsoleCommandsCreated_SV_Base = false;

void game_sv_GameState::assign_RP(CSE_Abstract *E, game_PlayerState *ps_who)
{
	VERIFY(E);

	u8 l_uc_team = u8(-1);
	CSE_Spectator *tpSpectator = smart_cast<CSE_Spectator *>(E);

	if (tpSpectator)
		l_uc_team = tpSpectator->g_team();
	else
	{
		CSE_ALifeCreatureAbstract *tpTeamed = smart_cast<CSE_ALifeCreatureAbstract *>(E);
		if (tpTeamed)
			l_uc_team = tpTeamed->g_team();
		else
			R_ASSERT2(tpTeamed, "Non-teamed object is assigning to respawn point!");
	}

	xr_vector<RPoint> &rp = rpoints[l_uc_team];
	xr_vector<u32> xrp;

	for (u32 i = 0; i < rp.size(); i++)
	{
		if (rp[i].TimeToUnfreeze < Level().timeServer())
			xrp.push_back(i);
	}
	u32 rpoint = 0;

	if (xrp.size() && !tpSpectator)	
		rpoint = xrp[::Random.randI((int)xrp.size())];	
	else
	{
		if (!tpSpectator)
		{
			for (u32 i = 0; i < rp.size(); i++)
				rp[i].TimeToUnfreeze = 0;
		}

		rpoint = ::Random.randI((int)rp.size());
	}

	RPoint &r = rp[rpoint];

	if (!tpSpectator)	
		r.TimeToUnfreeze = Level().timeServer() + g_sv_base_dwRPointFreezeTime;

	E->o_Position.set(r.P);
	E->o_Angle.set(r.A);
}

bool game_sv_GameState::IsPointFreezed(RPoint *rp)
{
	return rp->TimeToUnfreeze > Level().timeServer();
}

void game_sv_GameState::SetPointFreezed(RPoint *rp)
{
	R_ASSERT(rp);
	rp->TimeToUnfreeze = Level().timeServer() + g_sv_base_dwRPointFreezeTime;
}

CSE_Abstract *game_sv_GameState::spawn_begin(LPCSTR N)
{
	CSE_Abstract *A = F_entity_Create(N);
	R_ASSERT(A);			  // create SE
	A->s_name = N;			  // ltx-def
	A->s_gameid = u8(m_type); // game-type
	A->s_RP = 0xFE;			  // use supplied
	A->ID = 0xffff;			  // server must generate ID
	A->ID_Parent = 0xffff;	  // no-parent
	A->ID_Phantom = 0xffff;	  // no-phantom
	A->RespawnTime = 0;		  // no-respawn
	return A;
}

CSE_Abstract *game_sv_GameState::spawn_end(CSE_Abstract *E, ClientID const &id)
{
	NET_Packet P;
	u16 skip_header;

	E->Spawn_Write(P, TRUE);
	P.r_begin(skip_header);
	CSE_Abstract *N = m_server->Process_spawn(P, id);
	F_entity_Destroy(E);

	return N;
}

CSE_Abstract* game_sv_GameState::SpawnObject(const char* section, const Fvector& pos, const char* customData, ALife::_OBJECT_ID parent)
{
	if (!pSettings->section_exist(section))
	{
		Msg("! Section [%s] doesn`t exist", section);
		return nullptr;
	}

	if (HasAlifeSimulator())
	{
		if (!Actor())
		{
			Msg("! there is no actor!");
			return nullptr;
		}

		CSE_Abstract* spawnedObject = alife().spawn_item(section, pos, Actor()->ai_location().level_vertex_id(), Actor()->ai_location().game_vertex_id(), parent);

		if (parent != ALife::_OBJECT_ID(-1))
		{
			const auto parentObj = ai().alife().objects().object(parent, true);

			if (parentObj && parentObj->m_bOnline)
			{
				NET_Packet tNetPacket;
				server().FreeID(spawnedObject->ID, 0);
				alife().server().Process_spawn(tNetPacket, m_server->GetServerClient()->ID, FALSE, spawnedObject);
			}
		}

		if (!customData)
			return spawnedObject;

		if (auto alifeObject = smart_cast<CSE_ALifeCreatureAbstract*>(spawnedObject))		
			alifeObject->m_ini_string = customData;
		
		return spawnedObject;
	}
	else
	{
		R_ASSERT(parent == ALife::_OBJECT_ID(-1)); // not implemented
		CSE_Abstract* E = spawn_begin(section);

		if (!E)
			return nullptr;

		E->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
		E->o_Position.set(pos);
		E->o_Angle.set(0.f, 0.f, 0.f);
		E->m_ini_string = customData;

		E = spawn_end(E, m_server->GetServerClient()->ID);
		signal_Syncronize();
		return E;
	}
}

void game_sv_GameState::GenerateGameMessage(NET_Packet &P)
{
	P.w_begin(M_GAMEMESSAGE);
}

void game_sv_GameState::u_EventSend(NET_Packet &P, u32 dwFlags)
{
	m_server->SendBroadcast(BroadcastCID, P, dwFlags);
}

void game_sv_GameState::UpdateClientPing(xrClientData *client)
{
	u16 actualPing = u16(client->stats.getPing());

	bool bNeedToUpdate = !client->ps->lastPingUpdateTime // first update
						 || !actualPing || actualPing != client->ps->ping || client == m_server->GetServerClient();

	if (bNeedToUpdate)
	{
		client->ps->ping = actualPing;
		client->ps->lastPingUpdateTime = Level().timeServer();
	}
	else
	{
		// ����� 30 ��� ����������� ������ �����������, ���� 25 ��� �� ����������� ���� - �� �� ������ ����� �������
		// ���� ����� ������ ��������, � ���� ���������� � ���������� ��������� ����� ���������� �����, � ����� ������
		// �� ������� ������
		if ((client->ps->lastPingUpdateTime + 40000) < Level().timeServer())
			return;

		if ((client->ps->lastPingUpdateTime + 25000) < Level().timeServer())
		{
			Msg("! WARNING client [%s] crashed? Server time [%u], last ping [%u], update time [%u]", client->ps->getName(), Level().timeServer(), client->ps->ping, client->ps->lastPingUpdateTime);

			client->ps->lastPingUpdateTime = Level().timeServer() + 10000;
		}
	}
}

void game_sv_GameState::Update()
{
	m_server->ForEachClientDo([&](IClient* client)
	{
		xrClientData* C = static_cast<xrClientData*>(client);
		UpdateClientPing(C);
	});

	if (Level().game)
	{
		if (CScriptProcess* script_process = ai().script_engine().script_process(ScriptEngine::eScriptProcessorGame))
			script_process->update();
	}
}

game_sv_GameState::game_sv_GameState()
{
	VERIFY(g_pGameLevel);
	m_server = Level().Server;
	m_event_queue = xr_new<GameEventQueue>();

	m_bMapRotation = false;
	m_bMapSwitched = false;
	m_bMapNeedRotation = false;
	m_bFastRestart = false;
	m_pMapRotation_List.clear();
	m_alife_simulator = nullptr;

	for (int i = 0; i < TEAM_COUNT; i++)
		rpoints_MinDist[i] = 1000.0f;
}

game_sv_GameState::~game_sv_GameState()
{
	ai().script_engine().remove_script_process(ScriptEngine::eScriptProcessorGame);

	xr_delete(m_event_queue);
	SaveMapList();

	m_pMapRotation_List.clear();
	ConsoleCommands_Clear();

	if (m_alife_simulator)
		delete_data(m_alife_simulator);
}

bool game_sv_GameState::change_level(NET_Packet &net_packet, ClientID const &sender)
{
	return true;
}

void game_sv_GameState::save_game(NET_Packet& net_packet, ClientID const& sender)
{
	if (ai().get_alife())
		alife().save(net_packet);
}

bool game_sv_GameState::load_game(NET_Packet& net_packet, ClientID const& sender)
{
	if (!ai().get_alife())
		return true;

	shared_str game_name;
	net_packet.r_stringZ(game_name);
	return (alife().load_game(*game_name, true));
}

void game_sv_GameState::restart_simulator(LPCSTR saved_game_name)
{
	shared_str& options = *alife().server_command_line();

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

void game_sv_GameState::OnCreate(u16 idWho)
{
	if (!ai().get_alife())
		return;

	CSE_Abstract* entity = get_entity_from_eid(idWho);
	VERIFY(entity);

	if (!entity->m_bALifeControl)
		return;

	auto alifeObject = smart_cast<CSE_ALifeObject*>(entity);
	if (!alifeObject)
		return;

	alifeObject->m_bOnline = true;

	if (alifeObject->ID_Parent != 0xffff)
	{ 
		if (auto parent = ai().alife().objects().object(alifeObject->ID_Parent, true))
		{			
			if (auto trader = smart_cast<CSE_ALifeTraderAbstract*>(parent))
				alife().create(alifeObject);
			else
				alifeObject->m_bALifeControl = false;
		}
		else
			alifeObject->m_bALifeControl = false;
	}
	else
		alife().create(alifeObject);
}

BOOL game_sv_GameState::OnTouch(u16 eidWho, u16 eidWhat, BOOL bForced)
{
	CSE_Abstract* eWho = get_entity_from_eid(eidWho);
	VERIFY(eWho);
	CSE_Abstract* eWhat = get_entity_from_eid(eidWhat);
	VERIFY(eWhat);

	if (ai().get_alife())
	{
		auto alifeInvItem = smart_cast<CSE_ALifeInventoryItem*>(eWhat);
		auto dynObject = smart_cast<CSE_ALifeDynamicObject*>(eWho);

		if (
			alifeInvItem &&
			dynObject &&
			ai().alife().graph().level().object(alifeInvItem->base()->ID, true) &&
			ai().alife().objects().object(eWho->ID, true) &&
			ai().alife().objects().object(eWhat->ID, true))
			alife().graph().attach(*eWho, alifeInvItem, dynObject->m_tGraphID, false, false);
#ifdef DEBUG
		else if (psAI_Flags.test(aiALife))
		{
			Msg("Cannot attach object [%s][%s][%d] to object [%s][%s][%d]", eWhat->name_replace(), *eWhat->s_name, eWhat->ID, eWho->name_replace(), *eWho->s_name, eWho->ID);
		}
#endif
	}

	return TRUE;
}

void game_sv_GameState::OnDetach(u16 eidWho, u16 eidWhat)
{
	if (!ai().get_alife())
		return;

	CSE_Abstract* eWho = get_entity_from_eid(eidWho);
	VERIFY(eWho);
	CSE_Abstract* eWhat = get_entity_from_eid(eidWhat);
	VERIFY(eWhat);

	auto alifeInvItem = smart_cast<CSE_ALifeInventoryItem*>(eWhat);
	if (!alifeInvItem)
		return;

	auto dynObject = smart_cast<CSE_ALifeDynamicObject*>(eWho);
	if (!dynObject)
		return;

	if (
		ai().alife().objects().object(eWho->ID, true) &&
		!ai().alife().graph().level().object(alifeInvItem->base()->ID, true) &&
		ai().alife().objects().object(eWhat->ID, true))
		alife().graph().detach(*eWho, alifeInvItem, dynObject->m_tGraphID, false, false);
	else
	{
		if (!ai().alife().objects().object(eWhat->ID, true))
		{
			u16 id = alifeInvItem->base()->ID_Parent;
			alifeInvItem->base()->ID_Parent = 0xffff;

			CSE_ALifeDynamicObject* dynamic_object = smart_cast<CSE_ALifeDynamicObject*>(eWhat);
			VERIFY(dynamic_object);
			dynamic_object->m_tNodeID = dynObject->m_tNodeID;
			dynamic_object->m_tGraphID = dynObject->m_tGraphID;
			dynamic_object->m_bALifeControl = true;
			dynamic_object->m_bOnline = true;
			alife().create(dynamic_object);
			alifeInvItem->base()->ID_Parent = id;
		}
#ifdef DEBUG
		else if (psAI_Flags.test(aiALife))
		{
			Msg("Cannot detach object [%s][%s][%d] from object [%s][%s][%d]", alifeInvItem->base()->name_replace(), *alifeInvItem->base()->s_name, alifeInvItem->base()->ID, dynObject->base()->name_replace(), dynObject->base()->s_name, dynObject->ID);
		}
#endif
	}
}

void game_sv_GameState::OnHit(u16 id_hitter, u16 id_hitted, NET_Packet &P)
{
	CSE_Abstract *e_hitter = get_entity_from_eid(id_hitter);
	CSE_Abstract *e_hitted = get_entity_from_eid(id_hitted);
	if (!e_hitter || !e_hitted)
		return;

	if (auto hittedCreature = smart_cast<CSE_ALifeCreatureAbstract*>(e_hitted))
	{
		if (!hittedCreature->g_Alive())
			return;

		if (game_PlayerState* psHitter = get_eid(id_hitter))
		{
			const u32 readPos = P.r_pos;
			SHit HitS;
			HitS.Read_Packet(P);
			P.r_pos = readPos;

			if (CSE_Abstract* pWeaponA = get_entity_from_eid(HitS.weaponID))
				OnPlayerHitCreature(psHitter, pWeaponA);
		}

		if(e_hitted->m_tClassID == CLSID_OBJECT_ACTOR)
			OnPlayerHitPlayer(id_hitter, id_hitted, P);
	}
}

void game_sv_GameState::OnEvent(NET_Packet &tNetPacket, u16 type, u32 time, ClientID const &sender)
{
	switch (type)
	{
	case GAME_EVENT_PLAYER_CONNECTED:
	{
		ClientID ID;
		tNetPacket.r_clientID(ID);
		OnPlayerConnect(ID);
	}
	break;

	case GAME_EVENT_PLAYER_DISCONNECTED:
	{
		ClientID ID;
		tNetPacket.r_clientID(ID);
		string1024 PlayerName;
		tNetPacket.r_stringZ(PlayerName);
		u16 GameID = tNetPacket.r_u16();
		OnPlayerDisconnect(ID, PlayerName, GameID);
	}
	break;

	case GAME_EVENT_PLAYER_KILLED:
		break;

	case GAME_EVENT_ON_HIT:
	{
		u16 idHitted = tNetPacket.r_u16();
		u16 idHitter = tNetPacket.r_u16();
		CSE_Abstract *hitterEntity = get_entity_from_eid(idHitter);

		if (!hitterEntity) // added by andy because of Phantom does not have server entity
		{
			if (IsGameTypeSingle())
				break;

			game_PlayerState *ps = get_eid(idHitter);

			if (!ps)
				break;

			idHitter = ps->GameID;

			// TSMP: �����, �������� ��� ����� ���������, id � ������ �� �������. �� � ������, ��������� ��� ���� ����� id
			// ������� ��� � ������, ����� �� ���� ������� ��� ������������
			u32 packetSize = tNetPacket.B.count;
			tNetPacket.B.count = tNetPacket.r_pos - sizeof(u16);
			tNetPacket.w_u16(idHitter);
			tNetPacket.B.count = packetSize;
		}

		OnHit(idHitter, idHitted, tNetPacket);
		m_server->SendBroadcast(BroadcastCID, tNetPacket, net_flags(TRUE, TRUE));
	}
	break;

	case GAME_EVENT_CREATE_CLIENT:
	{
		IClient *CL = (IClient *)m_server->ID_to_client(sender);
		
		if (!CL)		
			break;
		
		CL->flags.bConnected = TRUE;
		m_server->AttachNewClient(CL);
	}
	break;

	case GAME_EVENT_PLAYER_AUTH:
	{
		IClient* CL = m_server->ID_to_client(sender);
		m_server->OnBuildVersionRespond(CL, tNetPacket);
	}
	break;

	case GAME_EVENT_PLAYER_AUTH_UID:
	{
		IClient* CL = m_server->ID_to_client(sender);
		m_server->OnRespondUID(CL, tNetPacket);
	}
	break;

	default:
	{
		string16 tmp;
		R_ASSERT3(0, "Game Event not implemented!!!", itoa(type, tmp, 10));
	}
	}
}

bool game_sv_GameState::NewPlayerName_Exists(void *pClient, LPCSTR NewName)
{
	if (!pClient || !NewName)
		return false;

	IClient *CL = (IClient *)pClient;

	if (!CL->name || xr_strlen(CL->name.c_str()) == 0)
		return false;

	CStringTable st;

	if (st.HasTranslation(NewName))
		return true;

	auto cl = m_server->FindClient([&](IClient* client)
	{
		IClient* pIC = static_cast<xrClientData*>(client);

		if (!pIC || pIC == CL)
			return false;

		string64 xName;
		strcpy(xName, pIC->name.c_str());
		return !xr_strcmp(NewName, xName);
	});

	return !!cl;
}

void game_sv_GameState::NewPlayerName_Generate(void *pClient, LPSTR NewPlayerName)
{
	if (!pClient || !NewPlayerName)
		return;
	NewPlayerName[21] = 0;
	for (int i = 1; i < 100; ++i)
	{
		string64 NewXName;
		sprintf_s(NewXName, "%s_%d", NewPlayerName, i);
		if (!NewPlayerName_Exists(pClient, NewXName))
		{
			strcpy(NewPlayerName, NewXName);
			return;
		}
	}
}

void game_sv_GameState::NewPlayerName_Replace(void *pClient, LPCSTR NewPlayerName)
{
	if (!pClient || !NewPlayerName)
		return;
	IClient *CL = (IClient *)pClient;
	if (!CL->name || xr_strlen(CL->name.c_str()) == 0)
		return;

	CL->name._set(NewPlayerName);

	//---------------------------------------------------------
	NET_Packet P;
	P.w_begin(M_CHANGE_SELF_NAME);
	P.w_stringZ(NewPlayerName);
	m_server->SendTo(CL->ID, P);
}

void game_sv_GameState::OnSwitchPhase(u32 old_phase, u32 new_phase)
{
	inherited::OnSwitchPhase(old_phase, new_phase);
	signal_Syncronize();
}

void game_sv_GameState::AddDelayedEvent(NET_Packet &tNetPacket, u16 type, u32 time, ClientID const &sender)
{
	m_event_queue->Create(tNetPacket, type, time, sender);
}

void game_sv_GameState::ProcessDelayedEvent()
{
	GameEvent *ge = NULL;
	while ((ge = m_event_queue->Retreive()) != 0)
	{
		OnEvent(ge->P, ge->type, ge->time, ge->sender);
		m_event_queue->Release();
	}
}

u32 game_sv_GameState::getRPcount(u16 team_idx)
{
	if (!(team_idx < TEAM_COUNT))
		return 0;
	else
		return rpoints[team_idx].size();
}

RPoint game_sv_GameState::getRP(u16 team_idx, u32 rp_idx)
{
	if ((team_idx < TEAM_COUNT) && (rp_idx < rpoints[team_idx].size()))
		return rpoints[team_idx][rp_idx];
	else
		return RPoint();
}

void game_sv_GameState::teleport_object(NET_Packet& netPacket, u16 id)
{
	if (!ai().get_alife())
		return;

	GameGraph::_GRAPH_ID gameVertexId;
	u32 levelVertexId;
	Fvector position;

	netPacket.r(&gameVertexId, sizeof(gameVertexId));
	netPacket.r(&levelVertexId, sizeof(levelVertexId));
	netPacket.r_vec3(position);

	alife().teleport_object(id, gameVertexId, levelVertexId, position);
}

void game_sv_GameState::add_restriction(NET_Packet& packet, u16 id)
{
	if (!ai().get_alife())
		return;

	ALife::_OBJECT_ID restrictionId;
	packet.r(&restrictionId, sizeof(restrictionId));

	RestrictionSpace::ERestrictorTypes restrictionType;
	packet.r(&restrictionType, sizeof(restrictionType));

	alife().add_restriction(id, restrictionId, restrictionType);
}

void game_sv_GameState::remove_restriction(NET_Packet& packet, u16 id)
{
	if (!ai().get_alife())
		return;

	ALife::_OBJECT_ID restrictionId;
	packet.r(&restrictionId, sizeof(restrictionId));

	RestrictionSpace::ERestrictorTypes restrictionType;
	packet.r(&restrictionType, sizeof(restrictionType));

	alife().remove_restriction(id, restrictionId, restrictionType);
}

void game_sv_GameState::remove_all_restrictions(NET_Packet& packet, u16 id)
{
	if (!ai().get_alife())
		return;

	RestrictionSpace::ERestrictorTypes restrictionType;
	packet.r(&restrictionType, sizeof(restrictionType));

	alife().remove_all_restrictions(id, restrictionType);
}

void game_sv_GameState::MapRotation_AddMap(LPCSTR MapName)
{
	m_pMapRotation_List.push_back(MapName);
	if (m_pMapRotation_List.size() > 1)
		m_bMapRotation = true;
	else
		m_bMapRotation = false;
}

void game_sv_GameState::MapRotation_ListMaps()
{
	if (m_pMapRotation_List.empty())
	{
		Msg("- Currently there are no any maps in list.");
		return;
	}
	CStringTable st;
	Msg("- ----------- Maps ---------------");
	for (u32 i = 0; i < m_pMapRotation_List.size(); i++)
	{
		xr_string MapName = m_pMapRotation_List[i];
		if (i == 0)
			Msg("~   %d. %s (%s) (current)", i + 1, *st.translate(MapName.c_str()), MapName.c_str());
		else
			Msg("  %d. %s (%s)", i + 1, *st.translate(MapName.c_str()), MapName.c_str());
	}
	Msg("- --------------------------------");
};

void game_sv_GameState::OnRoundStart()
{
	m_bMapNeedRotation = false;
	m_bFastRestart = false;

	for (int t = 0; t < TEAM_COUNT; t++)
	{
		for (u32 i = 0; i < rpoints[t].size(); i++)
		{
			RPoint rp = rpoints[t][i];
			rp.Blocked = false;
		}
	};
	rpointsBlocked.clear();
} // ����� ������

void game_sv_GameState::OnRoundEnd()
{
	if (round_end_reason == eRoundEnd_GameRestarted || round_end_reason == eRoundEnd_GameRestartedFast)
	{
		m_bMapNeedRotation = false;
	}
	else
	{
		m_bMapNeedRotation = true;
	}

	m_bFastRestart = false;
	if (round_end_reason == eRoundEnd_GameRestartedFast)
	{
		m_bFastRestart = true;
	}
} // ����� ������

void game_sv_GameState::SaveMapList()
{
	if (0 == MAPROT_LIST[0])
		return;
	if (m_pMapRotation_List.empty())
		return;
	IWriter *fs = FS.w_open(MAPROT_LIST);
	while (m_pMapRotation_List.size())
	{
		xr_string MapName = m_pMapRotation_List.front();
		m_pMapRotation_List.pop_front();

		fs->w_printf("sv_addmap %s\n", MapName.c_str());
	};
	FS.w_close(fs);
};

shared_str game_sv_GameState::level_name(const shared_str &server_options) const
{
	string64 l_name = "";
	VERIFY(_GetItemCount(*server_options, '/'));
	return (_GetItem(*server_options, 0, l_name, '/'));
}

void game_sv_GameState::on_death(CSE_Abstract *eDest, CSE_Abstract *eSrc)
{
	auto creature = smart_cast<CSE_ALifeCreatureAbstract*>(eDest);
	if (!creature)
		return;

	VERIFY(creature->m_killer_id == ALife::_OBJECT_ID(-1));
	creature->m_killer_id = eSrc->ID;

	if (ai().get_alife())
		alife().on_death(eDest, eSrc);
}

//  [7/5/2005]
#ifdef DEBUG
extern Flags32 dbg_net_Draw_Flags;

void game_sv_GameState::OnRender()
{
	Fmatrix T;
	T.identity();
	Fvector V0, V1;
	u32 TeamColors[TEAM_COUNT] = {D3DCOLOR_XRGB(255, 0, 0), D3DCOLOR_XRGB(0, 255, 0), D3DCOLOR_XRGB(0, 0, 255), D3DCOLOR_XRGB(255, 255, 0)};
	//	u32 TeamColorsDist[TEAM_COUNT] = {color_argb(128, 255, 0, 0), color_argb(128, 0, 255, 0), color_argb(128, 0, 0, 255), color_argb(128, 255, 255, 0)};

	if (dbg_net_Draw_Flags.test(1 << 9))
	{
		for (int t = 0; t < TEAM_COUNT; t++)
		{
			for (u32 i = 0; i < rpoints[t].size(); i++)
			{
				RPoint rp = rpoints[t][i];
				V1 = V0 = rp.P;
				V1.y += 1.0f;

				T.identity();
				Level().debug_renderer().draw_line(Fidentity, V0, V1, TeamColors[t]);

				bool Blocked = false;

				m_server->ForEachClientDo([&](IClient* client)
				{
					if (Blocked)
						return;

					xrClientData* C = static_cast<xrClientData*>(client);
					game_PlayerState *PS = C->ps;
					if (!PS)
						return;
					if (PS->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
						return;
					CObject *pPlayer = Level().Objects.net_Find(PS->GameID);
					if (!pPlayer)
						return;

					if (rp.P.distance_to(pPlayer->Position()) <= 0.4f)					
						Blocked = true;					
				});

				if (rp.Blocked)
					continue;

				float r = .3f;
				T.identity();
				T.scale(r, r, r);
				T.translate_add(rp.P);
				Level().debug_renderer().draw_ellipse(T, TeamColors[t]);
				/*
				r = rpoints_MinDist[t];
				T.identity();
				T.scale(r, r, r);
				T.translate_add(rp.P);
				Level().debug_renderer().draw_ellipse(T, TeamColorsDist[t]);

				r = rpoints_Dist[t];
				T.identity();
				T.scale(r, r, r);
				T.translate_add(rp.P);
				Level().debug_renderer().draw_ellipse(T, TeamColorsDist[t]);
*/
			}
		}
	};

	if (dbg_net_Draw_Flags.test(1 << 0))
	{
		m_server->ForEachClientDo([&](IClient* client)
		{
			xrClientData* C = static_cast<xrClientData*>(client);
			game_PlayerState* PS = C->ps;
			if (!PS)
				return;
			if (PS->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
				return;
			CObject *pPlayer = Level().Objects.net_Find(PS->GameID);
			if (!pPlayer)
				return;

			float r = .4f;
			T.identity();
			T.scale(r, r, r);
			T.translate_add(pPlayer->Position());
			Level().debug_renderer().draw_ellipse(T, TeamColors[PS->team]);
		});
	}
};
#endif
//  [7/5/2005]

BOOL game_sv_GameState::IsVotingEnabled() { return g_sv_base_iVotingEnabled != 0; };
BOOL game_sv_GameState::IsVotingEnabled(u16 flag) { return (g_sv_base_iVotingEnabled & flag) != 0; };

bool game_sv_GameState::AssignOwnershipToConnectingClient(CSE_Abstract* E, xrClientData* CL)
{
	return !E->owner;
}

#include "patrol_path_storage.h"

void game_sv_GameState::SendPatrolPaths(const ClientID &clTo)
{
	NET_Packet P;
	P.w_begin(M_TRANSFER_PATROL_PATHS);
	ai().patrol_paths().NetworkExport(P);
	u32 mode = net_flags(TRUE, TRUE);
	server().SendTo(clTo, P, mode);
}
