#include "StdAfx.h"
#include "game_sv_hardmatch.h"

const std::vector<std::string> HardMatchWeapons
{
	"wpn_winchester_m1", "wpn_spas12_m1", "wpn_ak74u_m1", "wpn_groza_m1", "wpn_sig_m1", "wpn_l85_m2", "wpn_l85_m1", "wpn_abakan_m2",
	"wpn_abakan_m1", "wpn_sig_m2", "wpn_ak74_m1", "wpn_mp5_m2", "wpn_mp5_m1", "wpn_lr300_m1",  "wpn_val_m1", "wpn_svd_m1",
	"mp_wpn_toz34", "mp_wpn_mp5",  "mp_wpn_abakan", "mp_wpn_ak74", "mp_wpn_ak74u", "mp_wpn_fn2000", "mp_wpn_g36",
	"mp_wpn_gauss", "mp_wpn_groza", "mp_wpn_l85", "mp_wpn_lr300", "mp_wpn_sig550","mp_wpn_svd", "mp_wpn_svu",
	"mp_wpn_vintorez", "mp_wpn_wincheaster1300", "mp_wpn_bm16", "mp_wpn_spas12"
};

void game_sv_Hardmatch::SpawnWeaponsForActor(CSE_Abstract* pE, game_PlayerState* ps)
{
	CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(pE);

	if (!pA)
		return;

	SpawnWeapon4Actor(pA->ID, HardMatchWeapons[m_WeaponsRandom.randI(HardMatchWeapons.size())].c_str(), 0);
	SpawnWeapon4Actor(pA->ID, "mp_medkit", 0);
	SpawnWeapon4Actor(pA->ID, "medkit_army", 0);
	SpawnWeapon4Actor(pA->ID, "medkit_scientic", 0);
	SpawnWeapon4Actor(pA->ID, "mp_device_torch", 0);
}
