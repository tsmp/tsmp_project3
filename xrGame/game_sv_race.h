#pragma once
#include "game_sv_mp.h"

class ClientID;

class game_sv_Race : public game_sv_mp
{
	using inherited = game_sv_mp;
	u16 m_WinnerId;

public:

	game_sv_Race();
	virtual ~game_sv_Race();

	virtual LPCSTR type_name() const override { return "race"; };
	virtual void Create(shared_str& options) override;
	virtual void Update() override;
	virtual void OnEvent(NET_Packet& P, u16 type, u32 time, ClientID sender) override;

	virtual void OnPlayerReady(ClientID id) override;
	virtual void OnPlayerConnect(ClientID id_who) override;
	virtual void OnPlayerConnectFinished(ClientID id_who) override;
	virtual void OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID) override;

private:
	void SpawnPlayerInCar(ClientID &playerId);
	CSE_Abstract* SpawnCar();
	void AssignRPoint(CSE_Abstract* E);

	void OnGKill(NET_Packet &P);
	void OnBaseEnter(NET_Packet &P);
};
