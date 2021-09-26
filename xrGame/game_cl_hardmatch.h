#pragma once
#include "game_cl_deathmatch.h"

class game_cl_Hardmatch : public game_cl_Deathmatch
{
protected:
	virtual BOOL CanCallBuyMenu() override { return FALSE; };
};
