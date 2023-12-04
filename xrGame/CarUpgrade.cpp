#include "stdafx.h"
#include "Actor.h"
#include "CarUpgrade.h"
#include "Car.h"
#include "CarWeapon.h"

void CCarUpgrade::Load(LPCSTR section)
{
	inherited::Load(section);
	ReadUpgrades(section);
}

void CCarUpgrade::OnH_A_Chield()
{
	auto act = smart_cast<CActor*>(H_Parent());
	R_ASSERT(act);
	auto car = smart_cast<CCar*>(act->Holder());
	R_ASSERT(car);
	ApplyUpgrades(car);
}

void CCarUpgrade::ReadUpgrades(LPCSTR section)
{
	m_EnginePowerCoeff = READ_IF_EXISTS(pSettings, r_float, section, "engine_power_coeff", 1.f);
	m_GunHitPowerCoeff = READ_IF_EXISTS(pSettings, r_float, section, "gun_hit_power_coeff", 1.f);
	m_ExplosionImmunityCoeff = READ_IF_EXISTS(pSettings, r_float, section, "explosion_immunity_coeff", 1.f);
	m_FireWoundImmunityCoeff = READ_IF_EXISTS(pSettings, r_float, section, "fire_wound_immunity_coeff", 1.f);
}

void CCarUpgrade::ApplyUpgrades(CCar* car)
{
	//car->ChangeEnginePower(car->GetEnginePower() * m_EnginePowerCoeff);
	car->m_HitTypeK[ALife::eHitTypeExplosion] *= m_ExplosionImmunityCoeff;
	car->m_HitTypeK[ALife::eHitTypeFireWound] *= m_FireWoundImmunityCoeff;
	
	if(car->HasWeapon())
		car->m_car_weapon->SetHitPower(car->m_car_weapon->GetHitPower() * m_GunHitPowerCoeff);
}
