#include "stdafx.h"
#include "UIGameBaseMP.h"
#include "ui/UIVoteStatusWnd.h"
#include "ui/xrUIXmlParser.h"

CUIGameBaseMP::CUIGameBaseMP(): m_voteStatusWnd(nullptr)
{}

CUIGameBaseMP::~CUIGameBaseMP()
{
	xr_delete(m_voteStatusWnd);
}

void CUIGameBaseMP::SetVoteMessage(const char* str)
{
	if (!str)
	{
		xr_delete(m_voteStatusWnd);
		return;
	}

	if (!m_voteStatusWnd)
	{
		CUIXml uiXml;
		uiXml.Init(CONFIG_PATH, UI_PATH, "ui_game_dm.xml");
		m_voteStatusWnd = xr_new<UIVoteStatusWnd>();
		m_voteStatusWnd->InitFromXML(uiXml);
	}

	m_voteStatusWnd->Show(true);
	m_voteStatusWnd->SetVoteMsg(str);
}

void CUIGameBaseMP::SetVoteTimeResultMsg(const char* str)
{
	if (m_voteStatusWnd)
		m_voteStatusWnd->SetVoteTimeResultMsg(str);
}

void CUIGameBaseMP::OnFrame()
{
	inherited::OnFrame();

	if (m_voteStatusWnd && m_voteStatusWnd->IsShown())
		m_voteStatusWnd->Update();
}

void CUIGameBaseMP::Render()
{
	inherited::Render();

	if (m_voteStatusWnd && m_voteStatusWnd->IsShown())
		m_voteStatusWnd->Draw();
}
