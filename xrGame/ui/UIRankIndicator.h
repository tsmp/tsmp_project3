#pragma once
#include "UIWindow.h"

class CUIStatic;
class CUIXml;
class CUIStatic;

class CUIRankIndicator : public CUIWindow
{
	xr_vector<CUIStatic*> m_ranks;
	u8 m_current;
	u8 max_rank;

public:
	CUIRankIndicator(u8 ranksCount);
	virtual ~CUIRankIndicator();
	void InitFromXml(CUIXml &xml_doc);
	void SetRank(u8 team, u8 rank);
};
