#include "stdafx.h"
#include "xrServer.h"
#include "xrserver_objects.h"
#include "game_base.h"
#include "Level.h"
#include "..\TSMP3_Build_Config.h"
#include "xrServerRespawnManager.h"
#include <vector>
#include "xrserver.h"
#include "game_sv_base.h"
using namespace std;

UINT_PTR RespawnTimer = 0;
vector <ObjectRespawnClass> xr_add_object;

void ObjectRespawnClass::AddObject(shared_str &pSection, shared_str& pCustomData, u16 pID, int pTimeRespawn, Fvector& XYZ)
{
    if (pTimeRespawn != 0)
    {
        ObjectRespawnClass add;
        xr_add_object.push_back(add);

        for (vector<ObjectRespawnClass>::iterator respawn = xr_add_object.begin(); respawn != xr_add_object.end(); respawn++)
        {
            if (respawn->section.size() != 0)
                continue;

            respawn->custom_data = pCustomData;
            respawn->section = pSection;
            respawn->id_object = pID;
            respawn->time_respawn = pTimeRespawn;
            respawn->spawn_position = Fvector().set(XYZ);
            respawn->time_tick = 0;
        }
    }
}

void ObjectRespawnClass::CheckRespawnObjects()
{
    for (vector<ObjectRespawnClass>::iterator respawn = xr_add_object.begin(); respawn != xr_add_object.end(); respawn++)
    {
        if (respawn->id_object == 0)
        {
            respawn->time_tick++;
            if (respawn->time_respawn <= respawn->time_tick)
            {
                respawn->time_tick = 0;

                if (CSE_Abstract* resp = Level().Server->game->SpawnObject(respawn->section.c_str(), respawn->spawn_position))
                {
                    respawn->id_object = resp->ID;
                    Msg("- Object spawned: [%s], id: [%d], X: [%3.2f], Y: [%3.2f], Z: [%3.2f], Time respawn: [%u]",
                        respawn->section.c_str(), respawn->id_object, respawn->spawn_position.x, respawn->spawn_position.y, respawn->spawn_position.z, respawn->time_respawn);
                }
            }
        }
    }
}

int ObjectRespawnClass::DestroyRespawnID(u16 id)
{
    for (vector<ObjectRespawnClass>::iterator respawn = xr_add_object.begin(); respawn != xr_add_object.end(); respawn++)
    {
        if (respawn->id_object == id)
        {
            respawn->id_object = 0;
            return id;
        }
    }
    return 0;
}

void CALLBACK TimerProc(HWND, UINT, UINT, DWORD)
{
    ObjectRespawnClass::CheckRespawnObjects();
}

void ObjectRespawnClass::DestroyRespawner()
{
    xr_add_object.clear();
    if (RespawnTimer) 
    {
        KillTimer(NULL, RespawnTimer);
    }
    RespawnTimer = SetTimer(NULL, 0, 1000, &TimerProc);
}
