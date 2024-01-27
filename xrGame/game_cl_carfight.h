#pragma once
#include "game_cl_teamdeathmatch.h"

class game_cl_Carfight : public game_cl_TeamDeathmatch
{
	using inherited = game_cl_TeamDeathmatch;
public:
	game_cl_Carfight();
	~game_cl_Carfight() override = default;

	void OnSpawn(CObject* pObj) override;
};
