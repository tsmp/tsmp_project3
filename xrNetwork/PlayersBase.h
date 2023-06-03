#pragma once
class IClient;
class IPureServer;

XRNETWORK_API void CheckPlayerBannedInBase(IClient* cl, IPureServer *serv);
XRNETWORK_API void GenPlayerUID(IClient* cl, IPureServer *serv);
