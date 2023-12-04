#pragma once
#include "game_sv_deathmatch.h"
#include "xrServer_Objects_ALife_Monsters.h"

class game_sv_Hardmatch : public game_sv_Deathmatch
{
	CRandom m_WeaponsRandom;
	std::vector<xr_string> m_RandomWeapons;
	std::vector<xr_string> m_PersistentItems;

public:
	game_sv_Hardmatch();

	virtual LPCSTR type_name() const override { return "hardmatch"; };

	virtual void SpawnWeaponsForActor(CSE_Abstract* pE, game_PlayerState* ps) override;
};
