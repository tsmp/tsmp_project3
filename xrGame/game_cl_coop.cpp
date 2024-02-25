#include "StdAfx.h"
#include "game_cl_Coop.h"
#include "UIGameDM.h"
#include "clsid_game.h"

game_cl_Coop::game_cl_Coop() : m_game_ui(nullptr) {}
game_cl_Coop::~game_cl_Coop() {}

CUIGameCustom* game_cl_Coop::createGameUI()
{
	inherited::createGameUI();
	CLASS_ID clsid = CLSID_GAME_UI_DEATHMATCH;
	m_game_ui = smart_cast<CUIGameDM*>(NEW_INSTANCE(clsid));
	R_ASSERT(m_game_ui);
	m_game_ui->SetClGame(this);
	m_game_ui->Init();
	return m_game_ui;
}

void game_cl_Coop::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
}

bool game_cl_Coop::OnKeyboardPress(int key)
{
	if (key == kSCORES)
	{
		m_game_ui->ShowPlayersList(true);
		return true;
	}

	return inherited::OnKeyboardPress(key);
}

bool game_cl_Coop::OnKeyboardRelease(int key)
{
	if (key == kSCORES)
		m_game_ui->ShowPlayersList(false);

	return inherited::OnKeyboardRelease(key);
}
