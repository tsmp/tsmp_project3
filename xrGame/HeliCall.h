#pragma once
#include "eatable_item_object.h"

class CHeliCall : public CEatableItemObject
{
public:
	virtual void UseBy(CEntityAlive* user) override;

private:
	bool m_Called = false;
};
