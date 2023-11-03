#include "stdafx.h"
#include "UIBuyWndShared.h"
#include "UIMPTradeWnd.h"
#include "../TSMP3_Build_Config.h"

extern LPCSTR _list_names[];

void CItemMgr::Load(const shared_str &sect_cost)
{
	CInifile::Sect &sect = pSettings->r_section(sect_cost);

	u32 idx = 0;
	for (CInifile::SectCIt it = sect.Data.begin(); it != sect.Data.end(); ++it, ++idx)
	{
		_i &val = m_items[it->first];
		val.slot_idx = u8(-1);
		int c = sscanf(it->second.c_str(), "%d,%d,%d,%d,%d", &val.cost[0], &val.cost[1], &val.cost[2], &val.cost[3], &val.cost[4]);
		VERIFY(c > 0);

#ifdef NEW_RANKS
		u32 max_rank_in_team = pSettings->r_u32("rank_extra_data", "max_rank_in_team");

		while (c < max_rank_in_team)
		{
			val.cost[c] = val.cost[c - 1];
			++c;
		}
#else
		while (c < _RANK_COUNT)
		{
			val.cost[c] = val.cost[c - 1];
			++c;
		}
#endif
	}

	for (u8 i = CUIMpTradeWnd::e_first; i < CUIMpTradeWnd::e_total_lists; ++i)
	{
		const shared_str &buff = pSettings->r_string("buy_menu_items_place", _list_names[i]);

		u32 cnt = _GetItemCount(buff.c_str());
		string1024 _one;

		for (u32 c = 0; c < cnt; ++c)
		{
			_GetItem(buff.c_str(), c, _one);
			shared_str _one_str = _one;
			COST_MAP_IT it = m_items.find(_one_str);
			R_ASSERT(it != m_items.end());
			R_ASSERT3(it->second.slot_idx == u8(-1), "item has duplicate record in [buy_menu_items_place] section ", _one);
			it->second.slot_idx = i;
		}
	}
}

const u32 CItemMgr::GetItemCost(const shared_str &sect_name, u32 rank) const
{
	COST_MAP_CIT it = m_items.find(sect_name);
	VERIFY(it != m_items.end());
	return it->second.cost[rank];
}

const u8 CItemMgr::GetItemSlotIdx(const shared_str &sect_name) const
{
	COST_MAP_CIT it = m_items.find(sect_name);
	VERIFY(it != m_items.end());
	return it->second.slot_idx;
}

const u32 CItemMgr::GetItemIdx(const shared_str &sect_name) const
{
	COST_MAP_CIT it = m_items.find(sect_name);

	if (it == m_items.end())
	{
#ifdef DEBUG
		Msg("item not found in registry [%s]", sect_name.c_str());
#endif
		return u32(-1);
	}

	return u32(std::distance(m_items.begin(), it));
}

void CItemMgr::Dump() const
{
	COST_MAP_CIT it = m_items.begin();
	COST_MAP_CIT it_e = m_items.end();

	Msg("--CItemMgr::Dump");
	for (; it != it_e; ++it)
	{
		const _i &val = it->second;
		R_ASSERT3(it->second.slot_idx != u8(-1), "item has no record in [buy_menu_items_place] section ", it->first.c_str());
		Msg("[%s] slot=[%d] cost= %d,%d,%d,%d,%d", it->first.c_str(), val.slot_idx, val.cost[0], val.cost[1], val.cost[2], val.cost[3], val.cost[4]);
	}
}

const u32 CItemMgr::GetItemsCount() const
{
	return m_items.size();
}

const shared_str &CItemMgr::GetItemName(u32 Idx) const
{
	//R_ASSERT(Idx<m_items.size());

	if (Idx >= m_items.size())
	{
		Msg("! ERROR: cant get item index, invalid value [%u], max value [%u]", Idx, m_items.size());
		return (m_items.begin() + 1)->first;
	}

	return (m_items.begin() + Idx)->first;
}