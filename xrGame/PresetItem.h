#pragma once

struct PresetItem
{
private:
	u8 SlotID; // slot + addons
	u8 ItemID;

public:	
	PresetItem(u8 Slot, u8 Item) { set(Slot, Item); };
	PresetItem() : PresetItem(static_cast<u8>(-1), static_cast<u8>(-1)) {};

	bool operator==(const PresetItem &other)
	{
		return SlotID == other.SlotID && ItemID == other.ItemID;
	}

	void set(u8 Slot, u8 Item)
	{
		SlotID = Slot;
		ItemID = Item;
	}

	void SetSlot(u8 slot)
	{
		SlotID = slot;
	}

	void SetItem(u8 item)
	{
		ItemID = item;
	}

	u8 GetAddons() const
	{
		return SlotID >> 0x08;
	}

	u8 GetSlot() const
	{
		return SlotID;
	}

	u8 GetItemID() const
	{
		return ItemID;
	}

	bool IsValid() const
	{
		return SlotID != static_cast<u8>(-1) && ItemID != static_cast<u8>(-1);
	}

	void Serialize(NET_Packet &p) const;
	void Deserialize(NET_Packet &p);
};

using PRESET_ITEMS = xr_vector<PresetItem>;
