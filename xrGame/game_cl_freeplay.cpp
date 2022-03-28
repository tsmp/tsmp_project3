#include "StdAfx.h"
#include "game_cl_freeplay.h"
#include "UIGameFP.h"
#include "clsid_game.h"
#include "actor.h"
#include "ui/UIInventoryWnd.h"
#include "string_table.h"

game_cl_Freeplay::game_cl_Freeplay() : m_game_ui(nullptr) {}
game_cl_Freeplay::~game_cl_Freeplay() {}

CUIGameCustom* game_cl_Freeplay::createGameUI()
{
	inherited::createGameUI();
	CLASS_ID clsid = CLSID_GAME_UI_FREEPLAY;
	m_game_ui = smart_cast<CUIGameFP*>(NEW_INSTANCE(clsid));
	R_ASSERT(m_game_ui);
	m_game_ui->SetClGame(this);
	m_game_ui->Init();
	return m_game_ui;
}

void game_cl_Freeplay::shedule_Update(u32 dt)
{
	CStringTable st;

	if (m_game_ui && Phase() == GAME_PHASE_INPROGRESS)
	{
		if(Level().CurrentControlEntity() && Level().CurrentControlEntity()->CLS_ID != CLSID_OBJECT_ACTOR)
			m_game_ui->SetPressJumpMsgCaption(*st.translate("mp_press_jump2spawn"));
		else
			m_game_ui->SetPressJumpMsgCaption("");
	}

	inherited::shedule_Update(dt);
}

bool game_cl_Freeplay::OnKeyboardPress(int key)
{
	if (key == kSCORES)
	{
		m_game_ui->ShowPlayersList(true);
		return true;
	}

	if (kINVENTORY == key)
	{
		if (!Level().CurrentControlEntity() || Level().CurrentControlEntity()->CLS_ID != CLSID_OBJECT_ACTOR)
			return false;

		if (!m_game_ui || Phase() != GAME_PHASE_INPROGRESS)
			return false;
		
		StartStopMenu(m_game_ui->m_pInventoryMenu, true);
		return true;		
	}

	return inherited::OnKeyboardPress(key);
}

bool game_cl_Freeplay::OnKeyboardRelease(int key)
{
	if (key == kSCORES)
		m_game_ui->ShowPlayersList(false);

	return inherited::OnKeyboardRelease(key);
}
