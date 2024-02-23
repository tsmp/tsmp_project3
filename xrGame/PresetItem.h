#pragma once

struct PresetItem
{
private:
	u8 Addons;
	u16 ItemID;

public:	
	PresetItem(u8 Addons, u16 Item) { set(Addons, Item); };
	PresetItem() : PresetItem(static_cast<u8>(-1), static_cast<u16>(-1)) {};

	bool operator==(const PresetItem &other)
	{
		return Addons == other.Addons && ItemID == other.ItemID;
	}

	void set(u8 addons, u16 Item)
	{
		Addons = addons;
		ItemID = Item;
	}

	void SetAddons(u8 addons)
	{
		Addons = addons;
	}

	void SetItemID(u16 item)
	{
		ItemID = item;
	}

	u8 GetAddons() const
	{
		return Addons;
	}

	u16 GetItemID() const
	{
		return ItemID;
	}

	bool IsValid() const
	{
		return Addons != static_cast<u8>(-1) && ItemID != static_cast<u16>(-1);
	}

	void Serialize(NET_Packet &p) const;
	void Deserialize(NET_Packet &p);
};

using PRESET_ITEMS = xr_vector<PresetItem>;
