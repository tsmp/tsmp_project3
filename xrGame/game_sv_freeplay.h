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


};
