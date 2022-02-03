#pragma once
#include "game_sv_mp.h"
#include "xrServer.h"
#include "xrMessages.h"

class game_sv_Freeplay : public game_sv_mp
{
	using inherited = game_sv_mp;

public:

	game_sv_Freeplay();
	virtual ~game_sv_Freeplay();

	virtual LPCSTR type_name() const override { return "freeplay"; };
	virtual void Create(shared_str &options) override;
	virtual void Update() override;
	virtual void OnEvent(NET_Packet &P, u16 type, u32 time, ClientID sender) override;
	
	virtual void OnPlayerReady(ClientID id) override;
	virtual void OnPlayerConnect(ClientID id_who) override;
	virtual void OnPlayerConnectFinished(ClientID id_who) override;
	virtual void OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID) override;
	virtual void OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState *ps_killed, KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract *pWeaponA);

	void SetStartMoney(ClientID &id_who);
	void SpawnItemsForActor(CSE_Abstract* pE, game_PlayerState* ps);
};
