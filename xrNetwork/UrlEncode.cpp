#include "pch_xrnetwork.h"

static bool IsUnreserved(unsigned char in)
{
	switch (in)
	{
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	case 'a': case 'b': case 'c': case 'd': case 'e':
	case 'f': case 'g': case 'h': case 'i': case 'j':
	case 'k': case 'l': case 'm': case 'n': case 'o':
	case 'p': case 'q': case 'r': case 's': case 't':
	case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E':
	case 'F': case 'G': case 'H': case 'I': case 'J':
	case 'K': case 'L': case 'M': case 'N': case 'O':
	case 'P': case 'Q': case 'R': case 'S': case 'T':
	case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
	case '-': case '.': case '_': case '~':
		return true;
	default:
		break;
	}
	return false;
}

std::string Encode(const std::string &str)
{
	std::string res;
	res.reserve(str.size() + 1);

	for (int i = 0, sourceStrLen = str.size(); i < sourceStrLen; i++)
	{
		unsigned char in = str[i];
			
		if (IsUnreserved(in)) //just copy		
			res += in;		
		else // encode
		{
			char ch[4];
			snprintf(ch, 4, "%%%02X", in);
			res += ch;
		}
	}

	return res;
}

std::string toUtf8(const std::wstring &str)
{
	std::string res;
	int len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.length(), 0, 0, 0, 0);
	
	if (len > 0)
	{
		res.resize(len);
		WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.length(), &res[0], len, 0, 0);
	}

	return res;
}

std::wstring ToUnicode(const std::string &str)
{
	int cnt = str.size();
	std::wstring res;
	res.resize(cnt);

	MultiByteToWideChar(CP_ACP, 0, str.data(), cnt, res.data(), cnt);
	return res;
}

std::string UrlEncode(const std::string &str)
{
	std::wstring strW = ToUnicode(str);
	std::string utf8str = toUtf8(strW);
	return Encode(utf8str.c_str());
}
