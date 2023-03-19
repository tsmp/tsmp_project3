#pragma once
#include "game_sv_mp.h"

class ClientID;

class game_sv_Race : public game_sv_mp
{
	using inherited = game_sv_mp;

	std::vector<std::string> m_AvailableCars;
	shared_str m_PlayerSkin;
	CRandom m_CarRandom;
	u32 m_WinnerFinishTime;
	u16 m_WinnerId;
	u8 m_CurrentRpoint;
	u8 m_CurrentRoundCar;
	u8 m_MaxPlayers;
	u8 m_CurrentRoad;

public:

	game_sv_Race();
	virtual ~game_sv_Race();

	virtual LPCSTR type_name() const override { return "race"; };
	virtual void Create(shared_str &options) override;
	virtual void Update() override;

	virtual void OnEvent(NET_Packet &P, u16 type, u32 time, ClientID const &sender) override;
	virtual void OnRoundStart() override;
	virtual void OnRoundEnd() override;

	virtual void OnPlayerReady(ClientID const &id) override;
	virtual void OnPlayerConnect(ClientID const &id_who) override;
	virtual void OnPlayerConnectFinished(ClientID const &id_who) override;
	virtual void OnPlayerDisconnect(ClientID const &id_who, LPSTR Name, u16 GameID) override;
	virtual void OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed, KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA) override;

	virtual bool SwitchPhaseOnRoundStart() override { return false; }
	virtual void net_Export_State(NET_Packet &P, ClientID const &id_to) override;

	virtual BOOL OnPreCreate(CSE_Abstract* E) override;

private:
	void SpawnPlayerInCar(ClientID const &playerId);
	CSE_Abstract* SpawnCar(u32 rpoint);
	void AssignRPoint(CSE_Abstract* E, u32 rpoint);

	void OnGKill(NET_Packet &P);
	void OnBaseEnter(NET_Packet &P);

	void UpdatePending();
	void UpdateRaceStart();
	void UpdateScores();
	void UpdateInProgress();

	u32 GetRpointIdx(game_PlayerState* ps);
	void LoadRaceSettings();
	void SelectRoad();
};
