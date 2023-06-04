#pragma once

#include "game_base.h"
#include "alife_space.h"
#include "script_export_space.h"
#include "../../xrNetwork/client_id.h"
#include "game_sv_base_console_vars.h"
#include "alife_simulator.h"

enum ERoundEnd_Result
{
	eRoundEnd_Finish = u32(0),
	eRoundEnd_GameRestarted,
	eRoundEnd_GameRestartedFast,
	eRoundEnd_TimeLimit,
	eRoundEnd_FragLimit,
	eRoundEnd_ArtrefactLimit,
	eRoundEnd_Force = u32(-1)
};

class xrClientData;
class CSE_Abstract;
class xrServer;
// [OLES] Policy:
// it_*		- means ordinal number of player
// id_*		- means player-id
// eid_*	- means entity-id
class GameEventQueue;

class game_sv_GameState : public game_GameState
{
	typedef game_GameState inherited;

protected:	
	xrServer *m_server;
	GameEventQueue *m_event_queue;
	CALifeSimulator* m_alife_simulator;

	//Events
	virtual void OnEvent(NET_Packet &tNetPacket, u16 type, u32 time, ClientID const &sender);

	virtual void ReadOptions(shared_str &options);
	virtual void ConsoleCommands_Create();
	virtual void ConsoleCommands_Clear();

	DEF_DEQUE(MAP_ROTATION_LIST, xr_string);
	bool m_bMapRotation;
	bool m_bMapNeedRotation;
	bool m_bMapSwitched;
	bool m_bFastRestart;

	MAP_ROTATION_LIST m_pMapRotation_List;

private:
	void UpdateClientPing(xrClientData *client);

public:
#define TEAM_COUNT 4

	bool NewPlayerName_Exists(void *pClient, LPCSTR NewName);
	void NewPlayerName_Generate(void *pClient, LPSTR NewPlayerName);
	void NewPlayerName_Replace(void *pClient, LPCSTR NewPlayerName);

	BOOL sv_force_sync;
	float rpoints_MinDist[TEAM_COUNT];
	xr_vector<RPoint> rpoints[TEAM_COUNT];
	DEF_VECTOR(RPRef, RPoint *);
	RPRef rpointsBlocked;

	ERoundEnd_Result round_end_reason;

	virtual void SaveMapList();
	virtual bool HasMapRotation() { return m_bMapRotation; };

public:
	virtual void OnPlayerConnect(ClientID const &id_who);
	virtual void OnPlayerDisconnect(ClientID const &id_who, LPSTR Name, u16 GameID);
	virtual void OnPlayerReady(ClientID const &id_who){};
	virtual void OnPlayerEnteredGame(ClientID const &id_who){};
	virtual void OnPlayerConnectFinished(ClientID const &id_who){};
	virtual void OnPlayerFire(ClientID const &id_who, NET_Packet &P){};
	void GenerateGameMessage(NET_Packet &P);

	virtual void OnRoundStart(); // старт раунда
	virtual void OnRoundEnd();	 //	round_end_reason			// конец раунда

	virtual void MapRotation_AddMap(LPCSTR MapName);
	virtual void MapRotation_ListMaps();
	virtual bool OnNextMap() { return false; }
	virtual void OnPrevMap() {}
	virtual bool SwitchToNextMap() { return m_bMapNeedRotation; };
	bool HasAlifeSimulator() const { return !!m_alife_simulator; };

	virtual BOOL IsVotingEnabled();
	virtual BOOL IsVotingEnabled(u16 flag);
	virtual bool IsVotingActive() { return false; };
	virtual void SetVotingActive(bool Active){};
	virtual void OnVoteStart(LPCSTR VoteCommand, ClientID const &sender){};
	virtual void OnVoteStop(){};

public:
	game_sv_GameState();
	virtual ~game_sv_GameState();
	// Main accessors
	virtual game_PlayerState *get_eid(u16 id);
	virtual xrClientData *get_client(u16 id); //if exist
	virtual game_PlayerState *get_id(ClientID const &id);

	virtual LPCSTR get_name_id(ClientID const &id);
	LPCSTR get_player_name_id(ClientID const &id);
	virtual u16 get_id_2_eid(ClientID const &id);
	virtual u32 get_players_count();
	CSE_Abstract *get_entity_from_eid(u16 id);
	RPoint getRP(u16 team_idx, u32 rp_idx);
	u32 getRPcount(u16 team_idx);
	// Signals
	virtual void signal_Syncronize();
	virtual void assign_RP(CSE_Abstract *E, game_PlayerState *ps_who);
	virtual bool IsPointFreezed(RPoint *rp);
	virtual void SetPointFreezed(RPoint *rp);

#ifdef DEBUG
	virtual void OnRender();
#endif

	virtual void OnSwitchPhase(u32 old_phase, u32 new_phase);
	CSE_Abstract *spawn_begin(LPCSTR N);
	CSE_Abstract *spawn_end(CSE_Abstract *E, ClientID const &id);
	CSE_Abstract *SpawnObject(const char* section, Fvector &pos, shared_str &pCustomData);

	// Utilities
	float get_option_f(LPCSTR lst, LPCSTR name, float def = 0.0f);
	s32 get_option_i(LPCSTR lst, LPCSTR name, s32 def = 0);
	string64 &get_option_s(LPCSTR lst, LPCSTR name, LPCSTR def = 0);
	virtual u32 get_alive_count(u32 team);
	void u_EventGen(NET_Packet &P, u16 type, u16 dest);
	void u_EventSend(NET_Packet &P, u32 dwFlags = DPNSEND_GUARANTEED);

	// Events
	virtual BOOL OnPreCreate(CSE_Abstract *E) { return TRUE; };
	virtual void OnCreate(u16 id_who){};
	virtual void OnPostCreate(u16 id_who){};
	virtual BOOL OnTouch(u16 eid_who, u16 eid_target, BOOL bForced = FALSE) = 0; // TRUE=allow ownership, FALSE=denied
	virtual void OnDetach(u16 eid_who, u16 eid_target) = 0;
	virtual void OnDestroyObject(u16 eid_who){};

	virtual void OnHit(u16 id_hitter, u16 id_hitted, NET_Packet &P);			   //кто-то получил Hit
	virtual void OnPlayerHitPlayer(u16 id_hitter, u16 id_hitted, NET_Packet &P){}; //игрок получил Hit
	virtual void OnPlayerHitCreature(game_PlayerState* psHitter, CSE_Abstract* pWeaponA) {}

	// Main
	virtual void Create(shared_str &options);
	virtual void Update();
	virtual void net_Export_State(NET_Packet &P, ClientID const &id_to);				// full state
	virtual void net_Export_Update(NET_Packet &P, ClientID const &id_to, ClientID const &id); // just incremental update for specific client
	virtual void net_Export_GameTime(NET_Packet &P);							// update GameTime only for remote clients

	virtual bool change_level(NET_Packet &net_packet, ClientID const &sender);
	virtual void save_game(NET_Packet &net_packet, ClientID const &sender);
	virtual bool load_game(NET_Packet &net_packet, ClientID const &sender);
	virtual void switch_distance(NET_Packet &net_packet, ClientID const &sender);

	void SendPatrolPaths(const ClientID& idTo);
	void AddDelayedEvent(NET_Packet &tNetPacket, u16 type, u32 time, ClientID const &sender);
	void ProcessDelayedEvent();
	virtual BOOL isFriendlyFireEnabled() { return FALSE; };
	virtual BOOL CanHaveFriendlyFire() = 0;
	virtual void teleport_object(NET_Packet &packet, u16 id);
	virtual void add_restriction(NET_Packet &packet, u16 id);
	virtual void remove_restriction(NET_Packet &packet, u16 id);
	virtual void remove_all_restrictions(NET_Packet &packet, u16 id);
	virtual void sls_default(){};
	virtual shared_str level_name(const shared_str &server_options) const;
	virtual void on_death(CSE_Abstract *e_dest, CSE_Abstract *e_src);

	virtual void DumpOnlineStatistic(){};

	virtual bool custom_sls_default()
	{
		return !!m_alife_simulator;
	};

	IC xrServer& server() const
	{
		VERIFY(m_server);
		return (*m_server);
	}

	IC CALifeSimulator& alife() const
	{
		VERIFY(m_alife_simulator);
		return (*m_alife_simulator);
	}

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(game_sv_GameState)
#undef script_type_list
#define script_type_list save_type_list(game_sv_GameState)