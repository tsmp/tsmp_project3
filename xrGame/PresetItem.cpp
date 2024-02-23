#include "StdAfx.h"
#include "PresetItem.h"
#include "..\xrNetwork\NET_utils.h"

void PresetItem::Serialize(NET_Packet& p) const
{
	p.w_u16(ItemID);
	p.w_u8(Addons);
}

void PresetItem::Deserialize(NET_Packet& p)
{	
	p.r_u16(ItemID);
	p.r_u8(Addons);
}
