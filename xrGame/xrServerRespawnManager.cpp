#include "stdafx.h"
#include "xrServerRespawnManager.h"
#include "xrServer.h"
#include "xrserver_objects.h"
#include "Level.h"

void CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    Level().Server->m_RespawnerMP.OnTimer();
}

ServerRespawnManager::ServerRespawnManager()
{
    if (!IsGameTypeSingle())
    {
        const UINT TimerPeriodMs = 1000;
        m_RespawnTimerHandle = SetTimer(NULL, 0, TimerPeriodMs, &TimerProc);
    }
}

ServerRespawnManager::~ServerRespawnManager()
{
    if(m_RespawnTimerHandle)
        KillTimer(NULL, m_RespawnTimerHandle);
}

void ServerRespawnManager::RegisterToRespawn(CSE_Abstract* entity)
{
    if (!entity->RespawnTime)
        return;

    ObjectToRespawn &obj = m_ObjectsToRespawn.emplace_back();
    obj.customData = entity->m_ini_string;
    obj.section = entity->s_name;
    obj.objectID = entity->ID;
    obj.secondsToRespawn = entity->RespawnTime;
    obj.pos = entity->o_Position;
    obj.secondsElapsed = 0;

    // Если время не 0, сервер ругается на dummy16
    entity->RespawnTime = 0;
}

ServerRespawnManager::RespObjIt ServerRespawnManager::FindObject(u16 objectID)
{
    return std::find_if(m_ObjectsToRespawn.begin(), m_ObjectsToRespawn.end(), [objectID](const ObjectToRespawn& obj)
    {
        return obj.objectID == objectID;
    });
}

bool ServerRespawnManager::RegisteredForRespawn(u16 objectID)
{
    return FindObject(objectID) != m_ObjectsToRespawn.end();
}

void ServerRespawnManager::MarkReadyForRespawn(u16 objectID)
{
    auto it = FindObject(objectID);

    if (it != m_ObjectsToRespawn.end())
        it->objectID = 0;
}

void ServerRespawnManager::CleanRespawnList()
{
    m_ObjectsToRespawn.clear();
}

void ServerRespawnManager::OnTimer()
{
    for (auto& obj : m_ObjectsToRespawn)
    {
        if (obj.objectID)
            continue;

        obj.secondsElapsed++;

        if (obj.secondsToRespawn > obj.secondsElapsed)
            continue;

        obj.secondsElapsed = 0;

        if (CSE_Abstract* resp = Level().Server->game->SpawnObject(obj.section.c_str(), obj.pos, obj.customData.c_str()))
        {
            obj.objectID = resp->ID;
            Msg("- Object spawned: [%s], id: [%d], X: [%3.2f], Y: [%3.2f], Z: [%3.2f], Respawn time: [%u]",
                obj.section.c_str(), obj.objectID, VPUSH(obj.pos), obj.secondsToRespawn);
        }
    }
}
