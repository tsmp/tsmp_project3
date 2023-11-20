#pragma once

#include "eatable_item_object.h"

class CCarRepairKit : public CEatableItemObject
{
public:
	virtual void UseBy(CEntityAlive* user) override;
};
