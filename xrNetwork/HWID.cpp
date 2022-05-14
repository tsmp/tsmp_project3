#include "stdafx.h"
#include "HWID.h"

#pragma warning(push)
#pragma warning(disable:4244)

HWID::HWID()
{
	std::fill(std::begin(s), std::end(s), 0);
}

#pragma warning(pop)

bool HWID::operator==(HWID &other)
{
	return (s[0] == other.s[0]
		&& s[1] == other.s[1]);
	//|| (s[2] == other.s[2] // работает неверно
	//	&& s[3] == other.s[3]
	//	&& s[4] == other.s[4]);
}

bool HWID::operator!()
{
	return std::all_of(s, s + ElementsCnt, [&](u16 x) { return !x; });
}

HWID &HWID::operator=(const HWID &other)
{
	if (this == &other)
		return *this;

	std::copy(std::begin(other.s), std::end(other.s), std::begin(s));
	return *this;
}

std::string HWID::ToString()
{
	std::string result;

	auto AddNumber = [&](u16 num)
	{
		char ch[32];
		sprintf(ch, "%hu", num);

		if (!result.empty())
			result += "-";

		result += ch;
	};

	for (u16 number : s)
		AddNumber(number);

	return result;
}

bool HWID::ParseFromString(const char* str, HWID &result)
{
	std::string hwstr = str;

	int numbersStart[ElementsCnt] = { 0 };
	int startsCnt = 1;
	u32 len = hwstr.length();

	for (u32 i = 0; i < len; i++)
	{
		if (hwstr[i] == '-')
		{
			numbersStart[startsCnt] = i;
			startsCnt++;
		}
	}

	if (startsCnt != ElementsCnt)
		return false;

	for (int i = 1; i <= ElementsCnt; i++)
	{
		int start = numbersStart[i - 1];
		int end = numbersStart[i];

		if (i != 1)
			start++;

		if (i == ElementsCnt)
			end = len;

		std::string num(hwstr.begin() + start, hwstr.end() - len + end);
		result.s[i - 1] = static_cast<u16>(atoi(num.c_str()));
	}

	return true;
}

void HWID::BuildFromCore(const char* arr, HWID &result)
{
	for (int i = 0; i < ElementsCnt; i++)
		memcpy(&result.s[i], &arr[i * 2], sizeof(u16));

	//Msg("My hwid: %s", result.ToString().c_str());
}

void HWID::NetSerialize(NET_Packet &Packet)
{
	Packet.w(s, ElementsCnt * sizeof(u16));
}

void HWID::NetDeserialize(NET_Packet &Packet)
{
	Packet.r(s, ElementsCnt * sizeof(u16));
}
