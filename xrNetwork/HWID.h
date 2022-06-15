#pragma once

class XRNETWORK_API HWID
{
private:
	static const int ElementsCnt = 5;
	u16 s[ElementsCnt];

public:
	HWID();

	bool operator==(HWID &other);
	bool operator!();
	HWID &operator=(const HWID &other);

	std::string ToString();
	static bool ParseFromString(const char* str, HWID &result);
	static void BuildFromCore(const char* arr, HWID &result);
	
	void NetSerialize(NET_Packet& Packet);
	void NetDeserialize(NET_Packet& Packet);
};
