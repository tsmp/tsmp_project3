#pragma once
#include "game_sv_mp.h"

class game_sv_Coop : public game_sv_mp
{
	using inherited = game_sv_mp;

protected:
	virtual void SetSkin(CSE_Abstract* E, u16 Team, u16 ID) override;

public:

	game_sv_Coop();
	virtual ~game_sv_Coop();

	virtual LPCSTR type_name() const override { return "coop"; };
	virtual void Create(shared_str& options) override;
	virtual void Update() override;
	virtual void OnEvent(NET_Packet& P, u16 type, u32 time, const ClientID& sender) override;

	virtual void OnPlayerReady(const ClientID& id) override;
	virtual void OnPlayerConnect(const ClientID& id_who) override;
	virtual void OnPlayerConnectFinished(const ClientID& id_who) override;
	virtual void OnPlayerDisconnect(const ClientID&, LPSTR Name, u16 GameID) override;

	virtual bool AssignOwnershipToConnectingClient(CSE_Abstract* E, xrClientData* CL) override;
	virtual shared_str level_name(const shared_str& server_options) const override;

	virtual void OnPlayerRequestSavedGames(ClientID const& sender) override;

	void SendTasks(const ClientID& target);
	void SendInfoPortions(const ClientID& target);
	void SpawnDefaultItemsForPlayer(u16 actorId);
};
