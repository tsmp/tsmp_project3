#include "StdAfx.h"
#include "game_sv_hardmatch.h"

const std::vector<std::string> HardMatchDefaultRandomWeapons
{
	"wpn_winchester_m1", "wpn_spas12_m1", "wpn_ak74u_m1", "wpn_groza_m1", "wpn_sig_m1", "wpn_l85_m2", "wpn_l85_m1", "wpn_abakan_m2",
	"wpn_abakan_m1", "wpn_sig_m2", "wpn_ak74_m1", "wpn_mp5_m2", "wpn_mp5_m1", "wpn_lr300_m1",  "wpn_val_m1", "wpn_svd_m1",
	"mp_wpn_toz34", "mp_wpn_mp5",  "mp_wpn_abakan", "mp_wpn_ak74", "mp_wpn_ak74u", "mp_wpn_fn2000", "mp_wpn_g36",
	"mp_wpn_gauss", "mp_wpn_groza", "mp_wpn_l85", "mp_wpn_lr300", "mp_wpn_sig550","mp_wpn_svd", "mp_wpn_svu",
	"mp_wpn_vintorez", "mp_wpn_wincheaster1300", "mp_wpn_bm16", "mp_wpn_spas12"
};

const std::vector<std::string> HardMatchDefaultPersistentItems
{
	"mp_medkit", "mp_medkit", "mp_medkit", "mp_device_torch"
};

const char* hardmatchRandomWpnSection = "hardmatch_random_weapons";
const char* hardmatchPersistentItemsSection = "hardmatch_persistent_items";

game_sv_Hardmatch::game_sv_Hardmatch()
{
	m_type = GAME_HARDMATCH;

	if (pSettings->section_exist(hardmatchRandomWpnSection))
	{
		CInifile::Sect& sect = pSettings->r_section(hardmatchRandomWpnSection);

		for (CInifile::SectCIt I = sect.Data.begin(); I != sect.Data.end(); I++)
		{
			const CInifile::Item &item = *I;
			m_RandomWeapons.push_back(item.first.c_str());
		}

		Msg("- Loaded %u persistent items for hardmatch", m_RandomWeapons.size());
	}

	if (m_RandomWeapons.empty())
	{
		Msg("- Cant find hardmatch random weapons list, using default");
		m_RandomWeapons = HardMatchDefaultRandomWeapons;
	}

	if (pSettings->section_exist(hardmatchPersistentItemsSection))
	{
		CInifile::Sect& sect = pSettings->r_section(hardmatchPersistentItemsSection);

		for (CInifile::SectCIt I = sect.Data.begin(); I != sect.Data.end(); I++)
		{
			const CInifile::Item& item = *I;
			u32 count = 1;

			if (const char* ch = item.second.c_str())
				count = pSettings->r_u32(hardmatchPersistentItemsSection, item.first.c_str());

			for (int i = 0; i < count; i++)
				m_PersistentItems.push_back(item.first.c_str());
		}

		Msg("- Loaded %u persistent items for hardmatch", m_PersistentItems.size());
	}

	if (m_PersistentItems.empty())
	{
		Msg("- Cant find hardmatch persistent items list, using default");
		m_PersistentItems = HardMatchDefaultPersistentItems;
	}
}

void game_sv_Hardmatch::SpawnWeaponsForActor(CSE_Abstract* pE, game_PlayerState* ps)
{
	CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(pE);

	if (!pA)
		return;

	SpawnWeapon4Actor(pA->ID, m_RandomWeapons[m_WeaponsRandom.randI(static_cast<u32>(m_RandomWeapons.size()))].c_str(), 0);

	for(std::string &itemName:m_PersistentItems)
		SpawnWeapon4Actor(pA->ID, itemName.c_str(), 0);
}
