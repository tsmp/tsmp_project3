#pragma once

#include "game_sv_base.h"

class xrServer;
class CALifeSimulator;

class game_sv_Single : public game_sv_GameState
{
private:
	typedef game_sv_GameState inherited;

public:
	game_sv_Single();
	virtual ~game_sv_Single() = default;

	virtual LPCSTR type_name() const { return "single"; };
	virtual void Create(shared_str &options);

	// Main
	virtual ALife::_TIME_ID GetGameTime();
	virtual float GetGameTimeFactor();
	virtual void SetGameTimeFactor(const float fTimeFactor);

	virtual void SetEnvironmentGameTimeFactor(const float fTimeFactor) {};

	virtual bool change_level(NET_Packet &net_packet, ClientID const &sender);
	virtual void save_game(NET_Packet &net_packet, ClientID const &sender);
	virtual bool load_game(NET_Packet &net_packet, ClientID const &sender);
	virtual void switch_distance(NET_Packet &net_packet, ClientID const &sender);
	virtual BOOL CanHaveFriendlyFire() { return FALSE; }
	virtual void sls_default();
	virtual shared_str level_name(const shared_str &server_options) const;
	void restart_simulator(LPCSTR saved_game_name);
};
