#pragma once
#include "game_sv_teamdeathmatch.h"

class ClientID;

class game_sv_Carfight : public game_sv_TeamDeathmatch
{
	using inherited = game_sv_TeamDeathmatch;
public:

	game_sv_Carfight();
	virtual ~game_sv_Carfight() = default;

	virtual LPCSTR type_name() const override { return "carfight"; };
	virtual void RespawnPlayer(ClientID const& id_who, bool NoSpectator) override;

	void AllowDeadBodyRemove(u16 gameID, bool changeOwner = true) override;

private:
	bool SpawnPlayerInCar(ClientID const& playerId);
	CSE_Abstract* SpawnCar(Fvector pos, Fvector angle);
};
