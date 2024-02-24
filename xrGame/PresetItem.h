#pragma once

struct PresetItem
{
private:
	u8 AddonFlags;
	u16 ItemIdx;

public:	
	PresetItem(u8 Addons, u16 Item) { set(Addons, Item); };
	PresetItem() : PresetItem(static_cast<u8>(-1), static_cast<u16>(-1)) {};

	bool operator==(const PresetItem &other)
	{
		return AddonFlags == other.AddonFlags && ItemIdx == other.ItemIdx;
	}

	void set(u8 addons, u16 Item)
	{
		AddonFlags = addons;
		ItemIdx = Item;
	}

	void SetAddons(u8 addons)
	{
		AddonFlags = addons;
	}

	void SetItemID(u16 item)
	{
		ItemIdx = item;
	}

	u8 GetAddons() const
	{
		return AddonFlags;
	}

	u16 GetItemID() const
	{
		return ItemIdx;
	}

	bool IsValid() const
	{
		return AddonFlags != static_cast<u8>(-1) && ItemIdx != static_cast<u16>(-1);
	}

	void Serialize(NET_Packet &p) const;
	void Deserialize(NET_Packet &p);
};

using PRESET_ITEMS = xr_vector<PresetItem>;
