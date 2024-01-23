#include "stdafx.h"
#include "game_cl_deathmatch.h"
#include "level.h"
#include "actor.h"
#include "inventory.h"
#include "xrServer_Objects_ALife_Items.h"
#include "weapon.h"
#include "xr_level_controller.h"
#include "eatable_item_object.h"
#include "Missile.h"
#include "clsid_game.h"

void game_cl_Deathmatch::OnBuyMenu_Ok()
{
	if (!m_bBuyEnabled || !local_player)
		return;

	CGameObject *pPlayer = smart_cast<CGameObject *>(Level().CurrentEntity());
	
	if (!pPlayer)
		return;	

	NET_Packet P;	
	pPlayer->u_EventGen(P, GE_GAME_EVENT, pPlayer->ID());
	P.w_u16(GAME_EVENT_PLAYER_BUY_FINISHED);	
	preset_items &preset = pCurBuyMenu->GetPreset(_preset_idx_last);

	if (preset.empty())
	{
		CActor* pActor = smart_cast<CActor*>(pPlayer);

		if (!pActor || !pActor->g_Alive())
			preset = pCurBuyMenu->GetPreset(_preset_idx_origin);
	}

	PRESET_ITEMS tmpItems;

	for (u32 i = 0, cnt = preset.size(); i < cnt; ++i)
	{
		const _preset_item &presetItem = preset[i];

		for (u32 idx = 0; idx < presetItem.count; ++idx)
		{
			PresetItem item;
			pCurBuyMenu->GetPresetItemByName(presetItem.sect_name, item);
			u8 Addons = presetItem.addon_state;
			tmpItems.emplace_back(PresetItem(Addons, item.GetItemID()));
		}
	}

	// Принудительно добавляем нож
	if (Type() != GAME_CARFIGHT)
	{
		PresetItem knifeItem;
		pCurBuyMenu->GetPresetItemByName("mp_wpn_knife", knifeItem);
		tmpItems.push_back(knifeItem);
	}

	if (pCurBuyMenu->IsIgnoreMoneyAndRank())	
		P.w_s32(0);	
	else
	{
		s32 MoneyDiff = pCurBuyMenu->GetPresetCost(_preset_idx_origin) - pCurBuyMenu->GetPresetCost(_preset_idx_last);
		P.w_s32(MoneyDiff);
	}

	P.w_u8(static_cast<u8>(tmpItems.size()));

	for (const auto &item: tmpItems)	
		item.Serialize(P);	
	
	if (m_bMenuCalledFromReady)
		OnKeyboardPress(kJUMP);	

	pPlayer->u_EventSend(P);
}

void game_cl_Deathmatch::OnBuyMenu_DefaultItems()
{
	SetBuyMenuItems(&PlayerDefItems, TRUE);
}

void game_cl_Deathmatch::SetBuyMenuItems(PRESET_ITEMS *pItems, BOOL OnlyPreset)
{
	if(!local_player || pCurBuyMenu->IsShown())
		return;

	pCurBuyMenu->ResetItems();
	pCurBuyMenu->SetupPlayerItemsBegin();
	
	if (CActor* pCurActor = smart_cast<CActor*>(Level().Objects.net_Find(local_player->GameID)))
	{
		using ItemToFunPtr = void (IBuyWnd::*)(const shared_str& sectionName, u8 addons);
		const CInventory &inventory = pCurActor->inventory();

		auto CheckItem = [&](const PIItem pItem, ItemToFunPtr itemToFunc)
		{
			if (!pItem || pItem->IsInvalid() || pItem->object().CLS_ID == CLSID_OBJECT_W_KNIFE)
				return;

			if (!pSettings->line_exist(GetBaseCostSect(), pItem->object().cNameSect()))
				return;
			
			u8 Addons = 0;

			if (CWeapon* pWeapon = smart_cast<CWeapon*>(pItem))
				Addons = pWeapon->GetAddonsState();

			if (CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(pItem))
			{
				if (pAmmo->m_boxCurr != pAmmo->m_boxSize)
					return;
			}

			(pCurBuyMenu->*itemToFunc)(pItem->object().cNameSect(), Addons);			
		};

		//проверяем слоты
		const TISlotArr &slots = inventory.m_slots;
		for(const CInventorySlot &slot: slots)
			CheckItem(slot.m_pIItem, &IBuyWnd::ItemToSlot);

		//проверяем пояс
		const TIItemContainer &belt = inventory.m_belt;
		for (const PIItem pItem : belt)
			CheckItem(pItem, &IBuyWnd::ItemToBelt);

		//проверяем ruck
		const TIItemContainer &ruck = pCurActor->inventory().m_ruck;
		for (const PIItem pItem : ruck)
			CheckItem(pItem, &IBuyWnd::ItemToRuck);
	}
	else
	{
		PresetItem knifeItem;
		pCurBuyMenu->GetPresetItemByName("mp_wpn_knife", knifeItem);
		const PRESET_ITEMS &presetItems = *pItems;

		for(const PresetItem &presetItem: presetItems)
		{
			if (presetItem.GetItemID() == knifeItem.GetItemID())
				continue;

			pCurBuyMenu->ItemToSlot(pCurBuyMenu->GetNameByPresetItem(PresetItem(0, presetItem.GetItemID())), presetItem.GetSlot());
		}
	}

	pCurBuyMenu->SetMoneyAmount(local_player->money_for_round);
	pCurBuyMenu->SetupPlayerItemsEnd();
	pCurBuyMenu->CheckBuyAvailabilityInSlots();
}

void game_cl_Deathmatch::LoadTeamDefaultPresetItems(const shared_str &caSection, IBuyWnd *pBuyMenu, PRESET_ITEMS *pPresetItems)
{
	if (!pSettings->line_exist(caSection, "default_items"))
		return;

	if (!pBuyMenu || !pPresetItems)
		return;

	pPresetItems->clear();	
	string4096 DefItems;

	// Читаем данные этого поля
	std::strcpy(DefItems, pSettings->r_string(caSection, "default_items"));
	u32 count = _GetItemCount(DefItems);

	// теперь каждое имя оружия, разделенные запятыми, заносим в массив
	for (u32 i = 0; i < count; ++i)
	{
		string256 ItemName;
		_GetItem(DefItems, i, ItemName);
		
		PresetItem itm;
		pBuyMenu->GetPresetItemByName(ItemName, itm);

		if (!itm.IsValid())
			continue;

		pPresetItems->emplace_back(PresetItem(0, itm.GetItemID()));
	}
}

void game_cl_Deathmatch::LoadDefItemsForRank(IBuyWnd *pBuyMenu)
{
	if (!pBuyMenu)
		return;

	LoadPlayerDefItems(getTeamSection(local_player->team), pBuyMenu);
	
	for (int i = 1; i <= local_player->rank; i++)
	{
		char tmp[5];
		string16 RankStr;		
		strconcat(sizeof(RankStr), RankStr, "rank_", itoa(i, tmp, 10));

		if (!pSettings->section_exist(RankStr))
			continue;

		for (PresetItem &defItem: PlayerDefItems)
		{
			const shared_str &ItemName = pBuyMenu->GetNameByPresetItem(defItem);

			if (!ItemName.size())
				continue;

			string256 ItemStr;
			strconcat(sizeof(ItemStr), ItemStr, "def_item_repl_", ItemName.c_str());
			if (!pSettings->line_exist(RankStr, ItemStr))
				continue;

			string256 NewItemStr;
			strcpy_s(NewItemStr, sizeof(NewItemStr), pSettings->r_string(RankStr, ItemStr));

			PresetItem item;
			pBuyMenu->GetPresetItemByName(NewItemStr, item);

			if (!item.IsValid())
				continue;

			defItem.set(0, item.GetItemID());
		}
	}

	for (const PresetItem &defItem : PlayerDefItems)
	{
		const shared_str &ItemName = pBuyMenu->GetNameByPresetItem(defItem);

		if (!ItemName.size())
			continue;

		if (!xr_strcmp(*ItemName, "mp_wpn_knife"))
			continue;

		if (!pSettings->line_exist(ItemName, "ammo_class"))
			continue;

		string1024 wpnAmmos, BaseAmmoName;
		std::strcpy(wpnAmmos, pSettings->r_string(ItemName, "ammo_class"));
		_GetItem(wpnAmmos, 0, BaseAmmoName);

		PresetItem item;
		pBuyMenu->GetPresetItemByName(BaseAmmoName, item);

		if (!item.IsValid())
			continue;

		if (GameID() == GAME_ARTEFACTHUNT)
		{
			PlayerDefItems.emplace_back(PresetItem(0, item.GetItemID()));
			PlayerDefItems.emplace_back(PresetItem(0, item.GetItemID()));
		}
	}

	if (pCurBuyMenu->IsShown())
		return;

	pCurBuyMenu->ResetItems();
	pCurBuyMenu->SetupDefaultItemsBegin();

	PresetItem knifeItem;
	pCurBuyMenu->GetPresetItemByName("mp_wpn_knife", knifeItem);

	for (const PresetItem &defItem : PlayerDefItems)
	{
		if (defItem.GetItemID() == knifeItem.GetItemID())
			continue;

		pCurBuyMenu->ItemToSlot(pCurBuyMenu->GetNameByPresetItem(PresetItem(0, defItem.GetItemID())), defItem.GetSlot());
	}

	pCurBuyMenu->SetupDefaultItemsEnd();
}

void game_cl_Deathmatch::OnMoneyChanged()
{
	if (!pCurBuyMenu || !pCurBuyMenu->IsShown())
		return;

	pCurBuyMenu->SetMoneyAmount(local_player->money_for_round);
	pCurBuyMenu->CheckBuyAvailabilityInSlots();	
}
