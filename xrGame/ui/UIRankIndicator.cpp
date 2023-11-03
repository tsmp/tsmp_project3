#include "stdafx.h"
#include "UIRankIndicator.h"
#include "UIXmlInit.h"
#include "UIStatic.h"
#include "../TSMP3_Build_Config.h"

CUIRankIndicator::CUIRankIndicator()
{
	m_current = u8(-1);
}

CUIRankIndicator::~CUIRankIndicator()
{
#ifdef NEW_RANKS

	for (u8 i = 0; i < max_rank; ++i)
		xr_delete(&m_ranks[i]);
#else
	for (u8 i = 0; i < max_rank; ++i)
		xr_delete(m_ranks[i]);
#endif
}

void CUIRankIndicator::InitFromXml(CUIXml &xml_doc)
{
	CUIXmlInit::InitWindow(xml_doc, "rank_wnd", 0, this);
	string256 str;
	for (u8 i = 0; i < max_rank; ++i)
	{
#ifdef NEW_RANKS
		CUIStatic *s = &m_ranks[i];
#else
		CUIStatic *&s = m_ranks[i];
#endif
		s = xr_new<CUIStatic>();
		sprintf_s(str, "rank_wnd:rank_%d", i);
		CUIXmlInit::InitStatic(xml_doc, str, 0, s);
	}
	CUIStatic *back = xr_new<CUIStatic>();
	back->SetAutoDelete(true);
	CUIXmlInit::InitStatic(xml_doc, "rank_wnd:background", 0, back);
	AttachChild(back);
}

void CUIRankIndicator::SetRank(u8 team, u8 rank)
{
	rank += team * (max_rank / 2);
	if (m_current == rank)
		return;
#ifdef NEW_RANKS
	if (m_current != u8(-1))
		DetachChild(&m_ranks[m_current]);

	m_current = rank;
	AttachChild(&m_ranks[m_current]);
#else
	if (m_current != u8(-1))
		DetachChild(m_ranks[m_current]);

	m_current = rank;
	AttachChild(m_ranks[m_current]);
#endif
}
