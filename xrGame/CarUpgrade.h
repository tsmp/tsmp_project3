#pragma once
#include "inventory_item_object.h"

class CCar;

class CCarUpgrade : public CInventoryItemObject
{
	using inherited = CInventoryItemObject;

public:
	virtual void Load(LPCSTR section) override;
	virtual void OnH_A_Chield() override;

	void ApplyUpgrades(CCar* car);
	void ReadUpgrades(LPCSTR section);

private:
	float m_EnginePowerCoeff = 1.f;
	float m_GunHitPowerCoeff = 1.f;
	float m_ExplosionImmunityCoeff = 1.f;
	float m_FireWoundImmunityCoeff = 1.f;
};
