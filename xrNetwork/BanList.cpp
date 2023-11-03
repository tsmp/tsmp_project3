#include "stdafx.h"
#include "NET_Server.h"

// BannedClient

bool IsIpAdress(const char* str)
{
	int pointsCount = 0;

	for (int i = 0; str[i] != '\0'; i++)
		if (str[i] == '.')
			pointsCount++;

	return pointsCount == 3;
}

void BannedClient::Load(CInifile& ini, const shared_str& sect)
{
	const char* data = sect.c_str();

	if (IsIpAdress(data))
		IpAddr.set(data);
	else
		uid = static_cast<u32>(atoll(data));

	tm tmBanned;
	const shared_str& timeToStr = ini.r_string(sect, "time_to");

	int res_t = sscanf(timeToStr.c_str(),
		"%02d.%02d.%d_%02d:%02d:%02d",
		&tmBanned.tm_mday,
		&tmBanned.tm_mon,
		&tmBanned.tm_year,
		&tmBanned.tm_hour,
		&tmBanned.tm_min,
		&tmBanned.tm_sec);

	VERIFY(res_t == 6);
	tmBanned.tm_mon -= 1;
	tmBanned.tm_year -= 1900;

	BanTime = mktime(&tmBanned);
	Msg("- loaded banned client %s to %s", IpAddr.to_string().c_str(), BannedTimeTo().c_str());
}

void BannedClient::Save(CInifile& ini) const
{
	if (uid)
		ini.w_string(std::to_string(uid).c_str(), "time_to", BannedTimeTo().c_str());
	else
		ini.w_string(IpAddr.to_string().c_str(), "time_to", BannedTimeTo().c_str());
}

xr_string BannedClient::BannedTimeTo() const
{
	string256 res;
	tm* tmBanned = _localtime64(&BanTime);

	sprintf_s(res, sizeof(res),
		"%02d.%02d.%d_%02d:%02d:%02d",
		tmBanned->tm_mday,
		tmBanned->tm_mon + 1,
		tmBanned->tm_year + 1900,
		tmBanned->tm_hour,
		tmBanned->tm_min,
		tmBanned->tm_sec);

	return res;
}

// CBanList

const char* BanListFilename = "banned_list.ltx";

void CBanList::BanAddress(const ip_address& address, u32 banTimeSec)
{
	if(IsAddressBanned(address))
	{
		Msg("This address is already banned");
		return;
	}

	if (address.to_string() == "0.0.0.0")
	{
		Msg("! WARNING: attempt to ban server ip!");
		return;
	}

	BannedClient newClient;
	newClient.IpAddr = address;
	time(&newClient.BanTime);
	newClient.BanTime += banTimeSec;

	m_Banned.emplace_back(std::move(newClient));
	Save();
}

void CBanList::UnbanAddress(const ip_address& address)
{
	auto it = std::find_if(m_Banned.begin(), m_Banned.end(), [&](const BannedClient& cl)
	{
		return cl.IpAddr == address;
	});

	if (it != m_Banned.end())
	{
		Msg("Unbanning %s", address.to_string().c_str());
		m_Banned.erase(it);
		Save();
	}
	else
		Msg("! ERROR: Can't find address %s in ban list.", address.to_string().c_str());
}

void CBanList::BanUid(u32 uid)
{
	auto it = std::find_if(m_Banned.begin(), m_Banned.end(), [&](const BannedClient& cl)
	{
		return cl.uid == uid;
	});

	if (it != m_Banned.end())
	{
		Msg("This uid is already banned");
		return;
	}

	if (!uid)
	{
		Msg("! WARNING: attempt to ban invalid uid!");
		return;
	}

	BannedClient newClient;
	newClient.uid = uid;
	time(&newClient.BanTime);
	newClient.BanTime += 999999999;

	m_Banned.emplace_back(std::move(newClient));
	Save();
}

void CBanList::UnbanUid(u32 uid)
{
	auto it = std::find_if(m_Banned.begin(), m_Banned.end(), [&](const BannedClient& cl)
	{
		return cl.uid == uid;
	});

	if (it != m_Banned.end())
	{
		Msg("Unbanning %u", uid);
		m_Banned.erase(it);
		Save();
	}
	else
		Msg("! ERROR: Can't find uid %u in ban list.", uid);
}

void CBanList::Print()
{
	Msg("- Banned list");

	for (const BannedClient &pBClient : m_Banned)
		Msg("- %s to %s", pBClient.IpAddr.to_string().c_str(), pBClient.BannedTimeTo().c_str());
}

void CBanList::Load()
{
	string_path temp;
	FS.update_path(temp, "$app_data_root$", BanListFilename);

	CInifile ini(temp);
	CInifile::Root& root = ini.sections();

	for (CInifile::Sect* section : root)
	{
		BannedClient cl;
		cl.Load(ini, section->Name);
		m_Banned.emplace_back(std::move(cl));
	}
}

void CBanList::Save()
{
	string_path path;
	FS.update_path(path, "$app_data_root$", BanListFilename);
	CInifile ini(path, FALSE, FALSE, TRUE);

	for (const BannedClient& cl : m_Banned)
		cl.Save(ini);
}

void CBanList::Update()
{
	if (m_Banned.empty())
		return;

	std::sort(m_Banned.begin(), m_Banned.end(), [](const BannedClient& cl1, const BannedClient& cl2)
	{
		return cl1.BanTime > cl2.BanTime;
	});

	time_t curTime;
	time(&curTime);
	const BannedClient &lastBanned = m_Banned.back();

	if (lastBanned.BanTime < curTime)	
		UnbanAddress(lastBanned.IpAddr);	
}

bool CBanList::IsAddressBanned(const ip_address& address)
{
	auto it = std::find_if(m_Banned.begin(), m_Banned.end(), [&](const BannedClient& cl)
	{
		return cl.IpAddr == address;
	});

	return it != m_Banned.end();
}

bool CBanList::IsUidBanned(u32 uid)
{
	auto it = std::find_if(m_Banned.begin(), m_Banned.end(), [&](const BannedClient& cl)
	{
		return cl.uid == uid;
	});

	return it != m_Banned.end();
}
