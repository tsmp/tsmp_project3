#include "stdafx.h"
#include "xrserver.h"
#include "xrserver_objects.h"
#include "..\..\TSMP3_Build_Config.h"

bool xrServer::Process_event_reject(NET_Packet &P, const ClientID const &sender, const u32 time, const u16 id_parent,
                                    const u16 id_entity, bool send_message)
{
    // Parse message
    CSE_Abstract *e_parent = game->get_entity_from_eid(id_parent);
    CSE_Abstract *e_entity = game->get_entity_from_eid(id_entity);

#ifdef DEBUG
    Msg("sv reject. id_parent %s id_entity %s [%d]", ent_name_safe(id_parent).c_str(), ent_name_safe(id_entity).c_str(), Device.CurrentFrameNumber);
#endif

    if (!e_entity)
    {
        Msg("! ERROR on rejecting: entity not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent,
            id_entity, Device.CurrentFrameNumber);

        return false;
    }

    if (!e_parent)
    {
        Msg("! ERROR on rejecting: parent not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent,
            id_entity, Device.CurrentFrameNumber);

        return false;
    }

#ifdef MP_LOGGING
    Msg("--- SV: Process reject: parent[%d][%s], item[%d][%s]", id_parent, e_parent->name_replace(), id_entity,
        e_entity->name());
#endif // MP_LOGGING

    xr_vector<u16> &C = e_parent->children;
    xr_vector<u16>::iterator c = std::find(C.begin(), C.end(), id_entity);

    if (c == C.end())
    {
        Msg("! WARNING: SV: can't find children [%s][%d] of parent [%s][%d]", e_entity->s_name.c_str(), id_entity,
            e_parent->name_replace(), e_parent);

        return false;
    }

    if (0xffff == e_entity->ID_Parent)
    {
        Msg("! ERROR: can't detach independant object. entity[%s][%d], parent[%s][%d], section[%s]",
            e_entity->name_replace(), id_entity, e_parent->name_replace(), id_parent, e_entity->s_name.c_str());

        return false;
    }

    // Rebuild parentness

    if (e_entity->ID_Parent != id_parent)
    {
        Msg("! ERROR: e_entity->ID_Parent = [%d]  parent = [%d][%s]  entity_id = [%d]  frame = [%d]",
            e_entity->ID_Parent, id_parent, e_parent->name_replace(), id_entity, Device.CurrentFrameNumber);

        CSE_Abstract *e_parent_from_entity = game->get_entity_from_eid(e_entity->ID_Parent);

        if (!e_parent_from_entity)
            Msg("! ERROR: cant find parent from entity");
        else
            Msg("! ERROR: parent from entity is [%s][%d]", e_parent_from_entity->name_replace(), e_entity->ID_Parent);
    }

    game->OnDetach(id_parent, id_entity);

    e_entity->ID_Parent = 0xffff;
    C.erase(c);

    // Signal to everyone (including sender)
    if (send_message)
    {
        DWORD MODE = net_flags(TRUE, TRUE, FALSE, TRUE);
        SendBroadcast(BroadcastCID, P, MODE);
    }

    return true;
}
