#include "StdAfx.h"
#include "game_cl_race.h"
#include "UIGameDM.h"
#include "clsid_game.h"
#include "actor.h"
#include "ui/UIInventoryWnd.h"
#include "string_table.h"

game_cl_Race::game_cl_Race() : m_game_ui(nullptr) {}
game_cl_Race::~game_cl_Race() {}

CUIGameCustom* game_cl_Race::createGameUI()
{
	inherited::createGameUI();
	CLASS_ID clsid = CLSID_GAME_UI_DEATHMATCH;
	m_game_ui = smart_cast<CUIGameDM*>(NEW_INSTANCE(clsid));
	R_ASSERT(m_game_ui);
	m_game_ui->SetClGame(this);
	m_game_ui->Init();
	return m_game_ui;
}

void game_cl_Race::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
}

bool game_cl_Race::OnKeyboardPress(int key)
{
	if (key == kSCORES)
	{
		m_game_ui->ShowPlayersList(true);
		return true;
	}

	return inherited::OnKeyboardPress(key);
}

bool game_cl_Race::OnKeyboardRelease(int key)
{
	if (key == kSCORES)
		m_game_ui->ShowPlayersList(false);

	return inherited::OnKeyboardRelease(key);
}
