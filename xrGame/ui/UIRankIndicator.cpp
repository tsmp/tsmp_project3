#include "stdafx.h"
#include "UIRankIndicator.h"
#include "UIXmlInit.h"
#include "UIStatic.h"

CUIRankIndicator::CUIRankIndicator(u8 ranksCount) : max_rank(ranksCount * 2), m_current(static_cast<u8>(-1))
{
	m_ranks.resize(max_rank);
}

CUIRankIndicator::~CUIRankIndicator()
{
	for (CUIStatic* stat : m_ranks)
		xr_delete(stat);
}

void CUIRankIndicator::InitFromXml(CUIXml &xml_doc)
{
	CUIXmlInit::InitWindow(xml_doc, "rank_wnd", 0, this);
	string256 str;

	for (u8 i = 0; i < max_rank; ++i)
	{
		auto s = m_ranks.emplace_back(xr_new<CUIStatic>());
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

	if (m_current != u8(-1))
		DetachChild(m_ranks[m_current]);

	m_current = rank;
	AttachChild(m_ranks[m_current]);
}
