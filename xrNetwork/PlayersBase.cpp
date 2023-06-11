#include "stdafx.h"
#include "PlayersBase.h"
#include "NET_Server.h"
#include <WinInet.h>
#include <thread>
#include "..\TSMP3_Build_Config.h"

#pragma comment(lib, "Wininet.lib")

const int MaxResponseLength = 1024;
const int MaxRequestLength = 4096;
const char *BaseUrl = "http://194.147.90.72:8080/";
static std::string SessionId;

extern std::string UrlEncode(const std::string &str);

bool SendRequest(const char* request, char* responseBuffer)
{
    char url[MaxRequestLength];
	R_ASSERT((strlen(BaseUrl) + strlen(request)) < MaxRequestLength);
    strcpy_s(url, BaseUrl);
    strcat_s(url, request);

    HINTERNET hInetSession = InternetOpen(0, INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
    HINTERNET hURL = InternetOpenUrl(hInetSession, url, 0, 0, 0, 0);
    
    DWORD dwBytesRead = 0;
    BOOL bResult = InternetReadFile(hURL, responseBuffer, MaxResponseLength, &dwBytesRead);
    responseBuffer[dwBytesRead] = '\0';

    Msg("- sent request [%s] %s", request, bResult ? "succeded" : "failed");
    if (bResult) Msg("- get response [%s]", responseBuffer);

    InternetCloseHandle(hURL);
    InternetCloseHandle(hInetSession);

    return bResult;
}

int CalculateKey(std::string srvName, std::string srvVer)
{
    auto Hash = [](std::string str)
    {
        int hash = 7;

        for (u32 i = 0; i < str.size(); i++)
        {
            int val = static_cast<unsigned char>(str[i]);
            //Msg("%d", val);
            hash = hash * 31 + val;
        }

        return hash;
    };

    std::string strToHash = srvName + srvVer + "PlayersBase3218";
    return Hash(strToHash);
}

void InitSession(IPureServer* serv)
{
    if (!SessionId.empty())
        return;

    std::string srvName = serv->GetServerName();
    std::string mapName = serv->GetMapName();
    std::string srvVer = TSMP_VERSION;
    std::string key = std::to_string(CalculateKey(srvName, srvVer));
	std::string request = "PlayersBase/v1/StartSession2?srv=" + UrlEncode(srvName) +
		"&key=" + UrlEncode(key) +
		"&ver=" + UrlEncode(srvVer) +
		"&map=" + UrlEncode(mapName);

    char response[MaxResponseLength];

    if (!SendRequest(request.c_str(), response))
        return;

    static const std::string SessionIdHeader = "id: ";
    std::string responseStr = response;

    if (responseStr.find(SessionIdHeader) == std::string::npos)
        return;

    std::string temp(responseStr.begin() + SessionIdHeader.size(), responseStr.end());
    SessionId = temp;
}

bool IsBanned(IClient* CL)
{
    if (SessionId.empty())
        return false;

    std::string ip = CL->m_cAddress.to_string().c_str();
    std::string name = CL->name.c_str();

	std::string request = "PlayersBase/v1/IsBanned2?ip=" + UrlEncode(ip) +
		"&name=" + UrlEncode(name) +
		"&key=" + UrlEncode(SessionId) +
		"&uid=" + UrlEncode(std::to_string(CL->UID));

    char response[MaxResponseLength];

    if (!SendRequest(request.c_str(), response))
        return false;

    if (response[0] == '1')
        return true;

    return false;
}

u32 GenUid()
{
	if (SessionId.empty())
		return 0;

	std::string request = "PlayersStats/v1/GenPlayerID?key=" + UrlEncode(SessionId);
	char response[MaxResponseLength];

	if (!SendRequest(request.c_str(), response))
		return 0;

	return std::stoul(response);
}

std::string SerializeStats(const PlayerGameStats& stats)
{
	std::string statsStr;
	statsStr += "frags:" + std::to_string(stats.fragsCnt);
	statsStr += "fragsNpc:" + std::to_string(stats.fragsNpc);
	statsStr += "headshot:" + std::to_string(stats.headShots);
	statsStr += "deaths:" + std::to_string(stats.deathsCnt);
	statsStr += "arts:" + std::to_string(stats.artsCnt);
	statsStr += "maxFrags:" + std::to_string(stats.maxFragsOneLife);
	statsStr += "time:" + std::to_string(stats.timeInGame);
	statsStr += "weaponCnt:" + std::to_string(stats.hitsCount.size());

	for (u32 i = 0, cnt = stats.hitsCount.size(); i < cnt; i++)
	{
		statsStr += "wpn:" + std::string(stats.weaponNames[i].c_str());
		statsStr += "hits:" + std::to_string(stats.hitsCount[i]);
	}

#ifndef PUBLIC_BUILD
	Msg("- Serialized stats:");
	Msg(statsStr.c_str());
#endif
	return statsStr;
}

void ReportPlayerStatsThread(IClient* cl, const PlayerGameStats& stats)
{
	if (SessionId.empty())
		return;

	std::string request = "PlayersStats/v1/ReportPlayerStats?key=" + UrlEncode(SessionId) +
		"&id=" + std::to_string(cl->UID) +
		"&stats=" + UrlEncode(SerializeStats(stats));

	char response[MaxResponseLength];
	SendRequest(request.c_str(), response);
}

#pragma TODO("TSMP: remake without creating new thread always")

void CheckPlayerBannedInBase(IClient* cl, IPureServer* serv)
{
    std::thread thread([](IClient*client, IPureServer* srv)
    {
        InitSession(srv); 
        srv->OnPlayersBaseVerifyRespond(client, IsBanned(client));
    }, cl, serv);

   thread.detach();
}

void GenPlayerUID(IClient* cl, IPureServer* serv)
{
	std::thread thread([](IClient* client, IPureServer* srv)
	{
		InitSession(srv);
		srv->OnPlayersBaseGenUID(client, GenUid());
	}, cl, serv);

	thread.detach();
}

void ReportPlayerStats(IClient* cl, const PlayerGameStats &stats)
{
	std::thread thread(ReportPlayerStatsThread, cl, stats);
	thread.detach();
}
