#pragma once

#include "inventory_item_object.h"

class CMPPlayersBag : public CInventoryItemObject
{
	using inherited = CInventoryItemObject;

public:
	CMPPlayersBag(void);
	virtual ~CMPPlayersBag(void);
	virtual bool NeedToDestroyObject() const;

	virtual void OnEvent(NET_Packet &P, u16 type);
	virtual void Load(LPCSTR section) override;

	ALife::_TIME_ID m_RemoveTime;
};