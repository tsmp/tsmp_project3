#include "StdAfx.h"
#include "game_cl_Coop.h"
#include "clsid_game.h"
#include "UIGameSP.h"

game_cl_Coop::game_cl_Coop() : m_game_ui(nullptr) {}
game_cl_Coop::~game_cl_Coop() {}

CUIGameCustom* game_cl_Coop::createGameUI()
{
	CLASS_ID clsid = CLSID_GAME_UI_SINGLE;
	CUIGameSP* pUIGame = smart_cast<CUIGameSP*>(NEW_INSTANCE(clsid));
	R_ASSERT(pUIGame);
	pUIGame->SetClGame(this);
	pUIGame->Init();
	m_game_ui = pUIGame;
	return pUIGame;
}

void game_cl_Coop::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
}
