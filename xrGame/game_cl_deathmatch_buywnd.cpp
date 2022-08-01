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

s16 game_cl_Deathmatch::GetBuyMenuItemIndex(u8 Addons, u8 ItemID)
{
	return (s16(Addons) << 0x08) | s16(ItemID);
}

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
			u8 SlotID = 0;
			u8 ItemID = 0;
			pCurBuyMenu->GetWeaponIndexByName(presetItem.sect_name, SlotID, ItemID);
			u8 Addons = presetItem.addon_state;
			s16 ID = GetBuyMenuItemIndex(Addons, ItemID);
			tmpItems.push_back(ID);
		}
	}

	//принудительно добавляем нож
	u8 SectID;
	u8 ItemID;
	pCurBuyMenu->GetWeaponIndexByName("mp_wpn_knife", SectID, ItemID);
	tmpItems.push_back(GetBuyMenuItemIndex(SectID, ItemID));

	if (pCurBuyMenu->IsIgnoreMoneyAndRank())	
		P.w_s32(0);	
	else
	{
		s32 MoneyDiff = pCurBuyMenu->GetPresetCost(_preset_idx_origin) - pCurBuyMenu->GetPresetCost(_preset_idx_last);
		P.w_s32(MoneyDiff);
	}

	P.w_u8(static_cast<u8>(tmpItems.size()));

	for (const auto &item: tmpItems)	
		P.w_s16(item.BigID);	
	
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
		u8 KnifeSlot, KnifeIndex;
		pCurBuyMenu->GetWeaponIndexByName("mp_wpn_knife", KnifeSlot, KnifeIndex);
		const PRESET_ITEMS &presetItems = *pItems;

		for(const PresetItem &presetItem: presetItems)
		{
			if (presetItem.ItemID == KnifeIndex)
				continue;

			pCurBuyMenu->ItemToSlot(pCurBuyMenu->GetWeaponNameByIndex(0, presetItem.ItemID), presetItem.SlotID);
		}
	}

	pCurBuyMenu->SetMoneyAmount(local_player->money_for_round);
	pCurBuyMenu->SetupPlayerItemsEnd();
	pCurBuyMenu->CheckBuyAvailabilityInSlots();
}

void game_cl_Deathmatch::CheckItem(PIItem pItem, PRESET_ITEMS *pPresetItems, BOOL OnlyPreset)
{
	R_ASSERT(pItem);
	R_ASSERT(pPresetItems);

	if (pItem->IsInvalid())
		return;

	u8 SlotID, ItemID;
	pCurBuyMenu->GetWeaponIndexByName(*pItem->object().cNameSect(), SlotID, ItemID);
	
	if (SlotID == 0xff || ItemID == 0xff)
		return;

	s16 BigID = GetBuyMenuItemIndex(SlotID, ItemID);	

	if (CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(pItem))
	{
		if (pAmmo->m_boxCurr != pAmmo->m_boxSize)
			return;
	}

	auto PresetItemIt = std::find(pPresetItems->begin(), pPresetItems->end(), BigID);
	if (OnlyPreset)
	{
		if (PresetItemIt == pPresetItems->end())
			return;
	}

	if (SlotID == PISTOL_SLOT)
	{
		auto DefPistolIt = std::find(PlayerDefItems.begin(), PlayerDefItems.end(), BigID);
		if (DefPistolIt != PlayerDefItems.end() && PresetItemIt == pPresetItems->end())
			return;
	}

	s16 DesiredAddons = 0;
	pCurBuyMenu->SectionToSlot(SlotID, ItemID, true);
		
	if (PresetItemIt != pPresetItems->end())
	{
		DesiredAddons = (*PresetItemIt).ItemID >> 5;
		pPresetItems->erase(PresetItemIt);
	}

	CWeapon* pWeapon = smart_cast<CWeapon*>(pItem);

	if (!pWeapon)
		return;

	using GetAddonNameFuncPtr = const shared_str& (CWeapon::*)();
	using IsAddonAttachedFuncPtr = bool (CWeapon::*)();
	using WpnAddonState = CSE_ALifeItemWeapon::EWeaponAddonState;

	auto CheckAddon = [&](GetAddonNameFuncPtr GetAddonName, IsAddonAttachedFuncPtr IsAddonAttached, WpnAddonState AddonState)
	{
		pCurBuyMenu->GetWeaponIndexByName(*(pWeapon->*GetAddonName)(), SlotID, ItemID);

		if (SlotID == 0xff || ItemID == 0xff)
			return;

		if ((pWeapon->*IsAddonAttached)())
		{
			if ((DesiredAddons & AddonState) || !OnlyPreset)
				pCurBuyMenu->AddonToSlot(AddonState, pWeapon->GetSlot(), true);
		}
		else
		{
			if (DesiredAddons & AddonState)
				pCurBuyMenu->AddonToSlot(CSE_ALifeItemWeapon::eWeaponAddonScope, pWeapon->GetSlot(), false);
		}
	};

	CheckAddon(&CWeapon::GetScopeName, &CWeapon::ScopeAttachable, WpnAddonState::eWeaponAddonScope);
	CheckAddon(&CWeapon::GetSilencerName, &CWeapon::SilencerAttachable, WpnAddonState::eWeaponAddonSilencer);
	CheckAddon(&CWeapon::GetGrenadeLauncherName, &CWeapon::GrenadeLauncherAttachable, WpnAddonState::eWeaponAddonGrenadeLauncher);	
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
		u8 SlotID, ItemID;
		pBuyMenu->GetWeaponIndexByName(ItemName, SlotID, ItemID);

		if (SlotID == 0xff || ItemID == 0xff)
			continue;

		s16 ID = GetBuyMenuItemIndex(0, ItemID);
		pPresetItems->push_back(ID);
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

		for (PresetItem &pDefItem: PlayerDefItems)
		{
			const shared_str &ItemName = pBuyMenu->GetWeaponNameByIndex(pDefItem.SlotID, pDefItem.ItemID);

			if (!ItemName.size())
				continue;

			string256 ItemStr;
			strconcat(sizeof(ItemStr), ItemStr, "def_item_repl_", ItemName.c_str());
			if (!pSettings->line_exist(RankStr, ItemStr))
				continue;

			string256 NewItemStr;
			strcpy_s(NewItemStr, sizeof(NewItemStr), pSettings->r_string(RankStr, ItemStr));

			u8 SlotID, ItemID;
			pBuyMenu->GetWeaponIndexByName(NewItemStr, SlotID, ItemID);
			if (SlotID == 0xff || ItemID == 0xff)
				continue;

			s16 ID = GetBuyMenuItemIndex(0, ItemID);
			pDefItem.set(ID);
		}
	}

	for (const PresetItem& pDefItem :PlayerDefItems)
	{
		const shared_str &ItemName = pBuyMenu->GetWeaponNameByIndex(pDefItem.SlotID, pDefItem.ItemID);

		if (!ItemName.size())
			continue;

		if (!xr_strcmp(*ItemName, "mp_wpn_knife"))
			continue;

		if (!pSettings->line_exist(ItemName, "ammo_class"))
			continue;

		string1024 wpnAmmos, BaseAmmoName;
		std::strcpy(wpnAmmos, pSettings->r_string(ItemName, "ammo_class"));
		_GetItem(wpnAmmos, 0, BaseAmmoName);

		u8 SlotID, ItemID;
		pBuyMenu->GetWeaponIndexByName(BaseAmmoName, SlotID, ItemID);

		if (SlotID == 0xff || ItemID == 0xff)
			continue;

		s16 ID = GetBuyMenuItemIndex(0, ItemID);

		if (GameID() == GAME_ARTEFACTHUNT)
		{
			PlayerDefItems.push_back(ID);
			PlayerDefItems.push_back(ID);
		}
	}

	if (pCurBuyMenu->IsShown())
		return;

	pCurBuyMenu->ResetItems();
	pCurBuyMenu->SetupDefaultItemsBegin();

	u8 KnifeSlot, KnifeIndex;
	pCurBuyMenu->GetWeaponIndexByName("mp_wpn_knife", KnifeSlot, KnifeIndex);

	for (const PresetItem& pDefItem : PlayerDefItems)
	{
		if (pDefItem.ItemID == KnifeIndex)
			continue;

		pCurBuyMenu->ItemToSlot(pCurBuyMenu->GetWeaponNameByIndex(0, pDefItem.ItemID), pDefItem.SlotID);
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
