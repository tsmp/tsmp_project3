#include "stdafx.h"
#include "xrServer.h"
#include "game_sv_single.h"
#include "xrserver_objects.h"
#include "game_base.h"
#include "game_cl_base.h"
#include "game_sv_deathmatch.h"
#include "Level.h"
#include "..\TSMP3_Build_Config.h"
#include <iostream>
#include <string>
#include "xrServerRespawnManager.h"
#include <vector>
#include "xrserver.h"
#include "game_sv_base.h"

using namespace std;

#define RESPAWN_TIMER 11111

int RespawnTimer = 0;

ObjectRespawnClass ServerRespawnManager;

vector <ObjectRespawnClass> xr_add_object;


void ObjectRespawnClass::AddObject(string _section, int _id, int _time_respawn, float _pos_x, float _pos_y, float _pos_z)
{
    if (_time_respawn != 0)
    {
        ObjectRespawnClass add;
        xr_add_object.push_back(add);

        for (vector<ObjectRespawnClass>::iterator respawn = xr_add_object.begin(); respawn != xr_add_object.end(); respawn++)
        {
            if (respawn->section.length() != 0)
                continue;

            respawn->section = _section;
            respawn->id_object = _id;
            respawn->time_respawn = _time_respawn;
            respawn->pos_x = _pos_x;
            respawn->pos_y = _pos_y;
            respawn->pos_z = _pos_z;
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
                CSE_Abstract* E = NULL;
                E = Level().Server->game->spawn_begin(respawn->section.c_str());
                E->s_flags.assign(M_SPAWN_OBJECT_LOCAL); // flags
                E->o_Position.x = respawn->pos_x;
                E->o_Position.y = respawn->pos_y;
                E->o_Position.z = respawn->pos_z;
                E = Level().Server->game->spawn_end(E, Level().Server->GetServerClient()->ID);

                respawn->id_object = E->ID;

                Msg("- Object spawned: [%s], id: [%d], X: [%3.2f], Y: [%3.2f], Z: [%3.2f], Time respawn: [%d]",
                    respawn->section.c_str(), respawn->id_object, respawn->pos_x, respawn->pos_y, respawn->pos_z, respawn->time_respawn);
            }
        }
    }
}

int ObjectRespawnClass::DestroyRespawnID(int id)
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
    ServerRespawnManager.CheckRespawnObjects();
}


// переделать таймер, но пока будет так :) 
void ObjectRespawnClass::DestroyRespawner()
{
    xr_add_object.clear();

    if (RespawnTimer) 
    {
        KillTimer(NULL, RespawnTimer);
    }

    RespawnTimer = SetTimer(NULL, 0, 1000, &TimerProc);
}
