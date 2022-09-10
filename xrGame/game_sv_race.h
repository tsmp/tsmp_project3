#pragma once
#include "game_sv_mp.h"

class ClientID;

class game_sv_Race : public game_sv_mp
{
	using inherited = game_sv_mp;

	shared_str m_PlayerSkin;
	u32 m_WinnerFinishTime;
	u16 m_WinnerId;
	u8 m_CurrentRpoint;
	u8 m_MaxPlayers;

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
	virtual bool SwitchPhaseOnRoundStart() override { return false; }

private:
	void SpawnPlayerInCar(ClientID const &playerId);
	CSE_Abstract* SpawnCar();
	void AssignRPoint(CSE_Abstract* E);

	void OnGKill(NET_Packet &P);
	void OnBaseEnter(NET_Packet &P);

	void UpdatePending();
	void UpdateRaceStart();
	void UpdateScores();
	void UpdateInProgress();

	void LoadRaceSettings();
};
