#include "stdafx.h"
#include "xrServer.h"
#include "xrserver_objects.h"
#include "Level.h"
#include "xrServerRespawnManager.h"

UINT_PTR RespawnTimer = 0;
xr_vector <ObjectRespawnClass> xr_add_object;

void ObjectRespawnClass::AddObject(shared_str &pSection, shared_str& pCustomData, u16 pID, int pTimeRespawn, Fvector& XYZ)
{
    if (pTimeRespawn != 0)
    {
        ObjectRespawnClass add;
        add.custom_data = pCustomData;
        add.section = pSection;
        add.id_object = pID;
        add.time_respawn = pTimeRespawn;
        add.spawn_position = Fvector().set(XYZ);
        add.time_tick = 0;
        xr_add_object.push_back(add);
    }
}

void ObjectRespawnClass::CheckRespawnObjects()
{
    if (!OnServer() || IsGameTypeSingle())
        return;

    for (auto& respawn : xr_add_object)
    {
        if (respawn.id_object == 0)
        {
            respawn.time_tick++;
            if (respawn.time_respawn <= respawn.time_tick)
            {
                respawn.time_tick = 0;

                if (CSE_Abstract* resp = Level().Server->game->SpawnObject(respawn.section.c_str(), respawn.spawn_position, respawn.custom_data))
                {
                    respawn.id_object = resp->ID;
                    Msg("- Object spawned: [%s], id: [%d], X: [%3.2f], Y: [%3.2f], Z: [%3.2f], Time respawn: [%u]",
                        respawn.section.c_str(), respawn.id_object, respawn.spawn_position.x, respawn.spawn_position.y, respawn.spawn_position.z, respawn.time_respawn);
                }
            }
        }
    }
}


u16 ObjectRespawnClass::DestroyRespawnID(u16 id)
{
    for (auto& respawn : xr_add_object)
    {
        if (respawn.id_object == id)
        {
            respawn.id_object = 0;
            return id;
        }
    }
    return 0;
}

u16 ObjectRespawnClass::GetRespawnObjectID(u16 id)
{
    for (auto& respawn : xr_add_object)
    {
        if (respawn.id_object == id)
            return id;
    }
    return 0;
}

void CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD)
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