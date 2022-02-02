#include "StdAfx.h"
#include "game_cl_freeplay.h"
#include "UIGameDM.h"
#include "clsid_game.h"
#include "actor.h"

game_cl_Freeplay::game_cl_Freeplay() {}
game_cl_Freeplay::~game_cl_Freeplay() {}

CUIGameCustom* game_cl_Freeplay::createGameUI()
{
	inherited::createGameUI();
	CLASS_ID clsid = CLSID_GAME_UI_DEATHMATCH;
	m_game_ui = smart_cast<CUIGameDM*>(NEW_INSTANCE(clsid));
	R_ASSERT(m_game_ui);
	m_game_ui->SetClGame(this);
	m_game_ui->Init();
	return m_game_ui;
}

void game_cl_Freeplay::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
}

bool game_cl_Freeplay::OnKeyboardPress(int key)
{
	return inherited::OnKeyboardPress(key);
}

bool game_cl_Freeplay::OnKeyboardRelease(int key)
{
	return inherited::OnKeyboardRelease(key);
}
