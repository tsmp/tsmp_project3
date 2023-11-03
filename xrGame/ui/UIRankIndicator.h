#pragma once

#include "UIWindow.h"
#include "UIStatic.h"
#include "../TSMP3_Build_Config.h"

class CUIStatic;
class CUIXml;
class CUIStatic;
class CUIRankIndicator : public CUIWindow
{
#ifdef NEW_RANKS
	u32 max_rank_in_team = pSettings->r_u32("rank_extra_data", "max_rank_in_team");
	CUIStatic* m_ranks = new CUIStatic[max_rank_in_team];
#else
	enum
	{
		max_rank = 10,
	};

	CUIStatic* m_ranks[max_rank];
#endif
	u8 m_current;

public:
	CUIRankIndicator();
	virtual ~CUIRankIndicator();
	void InitFromXml(CUIXml &xml_doc);
	void SetRank(u8 team, u8 rank);

#ifdef NEW_RANKS
private:
	u32 max_rank = pSettings->r_u32("rank_extra_data", "max_rank_in_team");
#endif
};
