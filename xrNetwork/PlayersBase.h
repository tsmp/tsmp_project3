#pragma once
class IClient;
class IPureServer;

struct PlayerGameStats
{
	u32 fragsCnt;
	u32 deathsCnt;
	u32 headShots;
	u32 artsCnt;
	u32 fragsNpc;
	u32 maxFragsOneLife;
	u32 timeInGame;

	xr_vector<shared_str> weaponNames;
	xr_vector<u32> hitsCount;
};

XRNETWORK_API void CheckPlayerBannedInBase(IClient* cl, IPureServer *serv);
XRNETWORK_API void GenPlayerUID(IClient* cl, IPureServer *serv);
XRNETWORK_API void ReportPlayerStats(IClient* cl, const PlayerGameStats &stats);
