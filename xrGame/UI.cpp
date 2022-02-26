#include "stdafx.h"
#include "UI.h"
#include "Console.h"
#include "Entity.h"
#include "HUDManager.h"
#include "UIGameSP.h"
#include "actor.h"
#include "level.h"
#include "game_cl_base.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIMessagesWindow.h"
#include "ui/UIPdaWnd.h"
#include "inventory.h"
#include "huditem.h"

CUI::CUI(CHUDManager *p)
{
	UIMainIngameWnd = xr_new<CUIMainIngameWnd>();
	UIMainIngameWnd->Init();
	m_pMessagesWnd = xr_new<CUIMessagesWindow>();

	m_Parent = p;
	pUIGame = nullptr;

	ShowGameIndicators();
	ShowCrosshair();
}

CUI::~CUI()
{
	xr_delete(m_pMessagesWnd);
	xr_delete(pUIGame);
	xr_delete(UIMainIngameWnd);
}

void CUI::Load(CUIGameCustom *pGameUI)
{
	if (pGameUI)
	{
		pGameUI->SetClGame(&Game());
		m_pMessagesWnd->SetChatOwner(&Game());
		return;
	}
	pUIGame = Game().createGameUI();
	R_ASSERT(pUIGame);
}

void CUI::UnLoad()
{
	xr_delete(pUIGame);
	pUIGame = Game().createGameUI();
	R_ASSERT(pUIGame);
}

void CUI::UIOnFrame()
{
	CEntity *m_Actor = smart_cast<CEntity *>(Level().CurrentEntity());

	//update windows
	if (m_Actor && GameIndicatorsShown() && psHUD_Flags.is(HUD_DRAW | HUD_DRAW_RT))		
		UIMainIngameWnd->Update();			

	// out GAME-style depend information
	if (GameIndicatorsShown() && pUIGame)
		pUIGame->OnFrame();	

	m_pMessagesWnd->Update();
}

bool CUI::Render()
{
	if (GameIndicatorsShown() && pUIGame)
		pUIGame->Render();	

	CEntity *pEntity = smart_cast<CEntity *>(Level().CurrentEntity());
	if (pEntity)
	{
		CActor *pActor = smart_cast<CActor *>(pEntity);
		if (pActor)
		{
			PIItem item = pActor->inventory().ActiveItem();
			if (item && pActor->HUDview() && smart_cast<CHudItem *>(item))
				(smart_cast<CHudItem *>(item))->OnDrawUI();
		}

		if (GameIndicatorsShown() && psHUD_Flags.is(HUD_DRAW | HUD_DRAW_RT))
		{
			UIMainIngameWnd->Draw();
			m_pMessagesWnd->Draw();
		}
		else
		{ //hack - draw messagess wnd in scope mode
			if (CUIGameCustom* pGameUI = HUD().GetUI()->UIGame())			
			{
				if (!pGameUI->PdaMenu->GetVisible())
					m_pMessagesWnd->Draw();
			}
			else
				m_pMessagesWnd->Draw();
		}
	}
	else
		m_pMessagesWnd->Draw();

	DoRenderDialogs();
	return false;
}

bool CUI::IR_OnMouseWheel(int direction)
{
	if (MainInputReceiver())
	{
		if (MainInputReceiver()->IR_OnMouseWheel(direction))
			return true;
	}

	if (pUIGame && pUIGame->IR_OnMouseWheel(direction))
		return true;

	if (MainInputReceiver())
		return true;

	return false;
}

bool CUI::IR_OnKeyboardHold(int dik)
{
	if (MainInputReceiver())
	{
		if (MainInputReceiver()->IR_OnKeyboardHold(dik))
			return true;
	}

	if (MainInputReceiver())
		return true;

	return false;
}

bool CUI::IR_OnKeyboardPress(int dik)
{
	if (MainInputReceiver())
	{
		if (MainInputReceiver()->IR_OnKeyboardPress(dik))
			return true;
	}

	if (UIMainIngameWnd->OnKeyboardPress(dik))
		return true;

	if (pUIGame && pUIGame->IR_OnKeyboardPress(dik))
		return true;

	if (MainInputReceiver())
		return true;

	return false;
}

bool CUI::IR_OnKeyboardRelease(int dik)
{
	if (MainInputReceiver())
	{
		if (MainInputReceiver()->IR_OnKeyboardRelease(dik))
			return true;
	}

	if (pUIGame && pUIGame->IR_OnKeyboardRelease(dik))
		return true;

	if (MainInputReceiver())
		return true;

	return false;
}

bool CUI::IR_OnMouseMove(int dx, int dy)
{
	if (MainInputReceiver())
	{
		if (MainInputReceiver()->IR_OnMouseMove(dx, dy))
			return true;
	}

	if (pUIGame && pUIGame->IR_OnMouseMove(dx, dy))
		return true;

	if (MainInputReceiver())
		return true;

	return false;
}

SDrawStaticStruct *CUI::AddInfoMessage(LPCSTR message)
{
	SDrawStaticStruct *ss = pUIGame->GetCustomStatic(message);
	if (!ss)
	{
		ss = pUIGame->AddCustomStatic(message, true);
	}
	return ss;
}

void CUI::ShowGameIndicators()
{
	m_bShowGameIndicators = true;
}

void CUI::HideGameIndicators()
{
	m_bShowGameIndicators = false;
}

void CUI::ShowCrosshair()
{
	psHUD_Flags.set(HUD_CROSSHAIR_RT, TRUE);
}

void CUI::HideCrosshair()
{
	psHUD_Flags.set(HUD_CROSSHAIR_RT, FALSE);
}

bool CUI::CrosshairShown()
{
	return !!psHUD_Flags.test(HUD_CROSSHAIR_RT);
}

void CUI::OnConnected()
{
	UIMainIngameWnd->OnConnected();
}
