#include "StdAfx.h"
#include "UIGameRace.h"
#include "game_cl_race.h"
#include "ui/UIXmlInit.h"
#include "ui/UIFrags.h"
#include "ui/UIMultiTextStatic.h"
#include "HUDManager.h"

const u32 CountdownColor = 0xff00ff00;
const u32 RoundResultColor = 0xfff0fff0;

#define DI2PX(x) float(iFloor((x + 1) * float(UI_BASE_WIDTH) * 0.5f))
#define DI2PY(y) float(iFloor((y + 1) * float(UI_BASE_HEIGHT) * 0.5f))
#define SZ(x) x *UI_BASE_WIDTH

CUIGameRace::CUIGameRace() : m_game(nullptr)
{
	m_pPlayerLists = xr_new<CUIWindow>();
	m_pFragLists = xr_new<CUIWindow>();

	m_CountdownCaption = "countdown";
	GameCaptions()->addCustomMessage(m_CountdownCaption, DI2PX(0.0f), DI2PY(-0.75f), SZ(0.05f), HUD().Font().pFontGraffiti19Russian, CGameFont::alCenter, CountdownColor, "");

	m_RoundResultCaption = "round_res";
	GameCaptions()->addCustomMessage(m_RoundResultCaption, DI2PX(0.0f), DI2PY(-0.1f), SZ(0.03f), HUD().Font().pFontGraffiti19Russian, CGameFont::alCenter, RoundResultColor, "");
}

CUIGameRace::~CUIGameRace()
{
	m_pFragLists->DetachAll();
	m_pPlayerLists->DetachAll();

	xr_delete(m_pPlayerLists);
	xr_delete(m_pFragLists);
}

void CUIGameRace::SetClGame(game_cl_GameState* g)
{
	m_game = smart_cast<game_cl_Race*>(g);
	R_ASSERT(m_game);
	m_game->SetUI(this);
}

void CUIGameRace::Init()
{
	inherited::Init();

	CUIXml xml_doc;
	bool xml_result = xml_doc.Init(CONFIG_PATH, UI_PATH, "stats.xml");
	R_ASSERT2(xml_result, "xml file not found");

	CUIFrags* pFragList = xr_new<CUIFrags>();
	CUIFrags* pPlayerList = xr_new<CUIFrags>();
	pFragList->SetAutoDelete(true);
	pPlayerList->SetAutoDelete(true);

	float ScreenW = UI_BASE_WIDTH;
	float ScreenH = UI_BASE_HEIGHT;
	pFragList->Init(xml_doc, "stats_wnd", "frag_wnd_dm");
	pPlayerList->Init(xml_doc, "players_wnd", "frag_wnd_dm");

	Frect FrameRect = pFragList->GetWndRect();
	float FrameW = FrameRect.right - FrameRect.left;
	float FrameH = FrameRect.bottom - FrameRect.top;
	pFragList->SetWndPos((ScreenW - FrameW) / 2.0f, (ScreenH - FrameH) / 2.0f);

	m_pFragLists->AttachChild(pFragList);

	FrameRect = pPlayerList->GetWndRect();
	FrameW = FrameRect.right - FrameRect.left;
	FrameH = FrameRect.bottom - FrameRect.top;
	pPlayerList->SetWndPos((ScreenW - FrameW) / 2.0f, (ScreenH - FrameH) / 2.0f);

	m_pPlayerLists->AttachChild(pPlayerList);
}

void CUIGameRace::ShowPlayersList(bool bShow)
{
	if (bShow)
		AddDialogToRender(m_pPlayerLists);
	else
		RemoveDialogToRender(m_pPlayerLists);
}

void CUIGameRace::ShowFragList(bool bShow)
{
	if (bShow)
		AddDialogToRender(m_pFragLists);
	else
		RemoveDialogToRender(m_pFragLists);
}

void CUIGameRace::SetCountdownCaption(const char* str)
{
	GameCaptions()->setCaption(m_CountdownCaption, str, CountdownColor, true);
}

void CUIGameRace::SetRoundResultCaption(const char* str)
{
	GameCaptions()->setCaption(m_RoundResultCaption, str, RoundResultColor, true);
}
