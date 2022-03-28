#include "StdAfx.h"
#include "UIGameFP.h"
#include "game_cl_freeplay.h"
#include "ui/UIXmlInit.h"
#include "ui/UIFrags.h"
#include "ui/UIInventoryWnd.h"
#include "ui/UICarBodyWnd.h"
#include "ui/UITalkWnd.h"
#include "ui/UIMultiTextStatic.h"

const u32 NORMAL_MSG_COLOR = 0xffffffff;

#define DI2PX(x) float(iFloor((x + 1) * float(UI_BASE_WIDTH) * 0.5f))
#define DI2PY(y) float(iFloor((y + 1) * float(UI_BASE_HEIGHT) * 0.5f))
#define SZ(x) x *UI_BASE_WIDTH

CUIGameFP::CUIGameFP(): m_game(nullptr)
{
	m_pPlayerLists = xr_new<CUIWindow>();
	m_pInventoryMenu = xr_new<CUIInventoryWnd>();

	m_pressjump_caption = "pressjump";
	GameCaptions()->addCustomMessage(m_pressjump_caption, DI2PX(0.0f), DI2PY(0.9f), SZ(0.02f), HUD().Font().pFontGraffiti19Russian, CGameFont::alCenter, NORMAL_MSG_COLOR, "");
}

CUIGameFP::~CUIGameFP() 
{
	xr_delete(m_pPlayerLists);
	xr_delete(m_pInventoryMenu);
}

void CUIGameFP::SetClGame(game_cl_GameState* g) 
{
	m_game = smart_cast<game_cl_Freeplay*>(g);
	R_ASSERT(m_game);
}

void CUIGameFP::Init() 
{
	inherited::Init();
	
	CUIXml xml_doc;
	bool xml_result = xml_doc.Init(CONFIG_PATH, UI_PATH, "stats.xml");
	R_ASSERT2(xml_result, "xml file not found");

	CUIFrags* pFragList = xr_new<CUIFrags>();
	pFragList->SetAutoDelete(true);
	pFragList->Init(xml_doc, "stats_wnd", "frag_wnd_dm");

	Frect FrameRect = pFragList->GetWndRect();
	float FrameW = FrameRect.right - FrameRect.left;
	float FrameH = FrameRect.bottom - FrameRect.top;
	pFragList->SetWndPos((UI_BASE_WIDTH - FrameW) / 2.0f, (UI_BASE_HEIGHT - FrameH) / 2.0f);
	m_pPlayerLists->AttachChild(pFragList);
}

void CUIGameFP::Render() 
{
	inherited::Render();
}

void CUIGameFP::OnFrame()
{
	inherited::OnFrame();
}

void CUIGameFP::ReInitShownUI()
{
	if (m_pInventoryMenu && m_pInventoryMenu->IsShown())
		m_pInventoryMenu->InitInventory();
	else if (m_pUICarBodyMenu->IsShown())
		m_pUICarBodyMenu->UpdateLists_delayed();
}

void CUIGameFP::HideShownDialogs()
{
	CUIDialogWnd* mir = MainInputReceiver();

	if (!mir)
		return;

	if (mir == m_pUICarBodyMenu
		|| mir == TalkMenu)
		mir->GetHolder()->StartStopMenu(mir, true);
}

void CUIGameFP::ShowPlayersList(bool bShow)
{
	if (bShow)
		AddDialogToRender(m_pPlayerLists);
	else
		RemoveDialogToRender(m_pPlayerLists);
}

void CUIGameFP::reset_ui()
{
	inherited::reset_ui();
	m_pInventoryMenu->Reset();
}

void CUIGameFP::SetPressJumpMsgCaption(LPCSTR str)
{
	GameCaptions()->setCaption(m_pressjump_caption, str, NORMAL_MSG_COLOR, true);
}
