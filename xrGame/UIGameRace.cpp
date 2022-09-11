#include "StdAfx.h"
#include "UIGameRace.h"
#include "game_cl_race.h"
#include "ui/UIXmlInit.h"
#include "ui/UIFrags.h"

CUIGameRace::CUIGameRace() : m_game(nullptr)
{
	m_pPlayerLists = xr_new<CUIWindow>();
}

CUIGameRace::~CUIGameRace()
{
	xr_delete(m_pPlayerLists);
}

void CUIGameRace::SetClGame(game_cl_GameState* g)
{
	m_game = smart_cast<game_cl_Race*>(g);
	R_ASSERT(m_game);
}

void CUIGameRace::Init()
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

void CUIGameRace::ShowPlayersList(bool bShow)
{
	if (bShow)
		AddDialogToRender(m_pPlayerLists);
	else
		RemoveDialogToRender(m_pPlayerLists);
}
