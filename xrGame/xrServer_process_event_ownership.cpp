#include "stdafx.h"
#include "xrserver.h"
#include "xrserver_objects.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "..\..\TSMP3_Build_Config.h"

void ReplaceOwnershipHeader(NET_Packet &P)
{
    //способ очень грубый, но на данный момент иного выбора нет. «аранее приношу извинени€
    u16 NewType = GE_OWNERSHIP_TAKE;
    CopyMemory(&P.B.data[6], &NewType, 2);
};

void xrServer::Process_event_ownership(NET_Packet &P, ClientID sender, u32 time, u16 ID, BOOL bForced)
{
    u32 MODE = net_flags(TRUE, TRUE, FALSE, TRUE);

    u16 id_parent = ID, id_entity;
    P.r_u16(id_entity);
    CSE_Abstract *e_parent = game->get_entity_from_eid(id_parent);
    CSE_Abstract *e_entity = game->get_entity_from_eid(id_entity);

#ifdef MP_LOGGING
    Msg("--- SV: Process ownership take: parent [%d][%s], item [%d][%s]", id_parent,
        e_parent ? e_parent->name_replace() : "null_parent", id_entity, e_entity ? e_entity->name() : "null_entity");
#endif // MP_LOGGING

    if (!e_parent)
    {
        Msg("! ERROR on ownership: parent not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent,
            id_entity, Device.CurrentFrameNumber);
        return;
    }

    if (!e_entity)
    {
        Msg("! ERROR on ownership: entity not found. parent_id = [%d], entity_id = [%d], frame = [%d].",
            id_parent, id_entity, Device.CurrentFrameNumber);
        return;
    }

    if (0xffff != e_entity->ID_Parent)
    {
        Msg("! WARNING: ownership: (entity already has parent) new_parent %s id_parent %s id_entity %s [%d]", ent_name_safe(e_entity->ID_Parent).c_str(), ent_name_safe(id_parent).c_str(), ent_name_safe(id_entity).c_str(), Device.CurrentFrameNumber);
        return;
    }

    xrClientData *c_parent = e_parent->owner;
    xrClientData *c_entity = e_entity->owner;
    xrClientData *c_from = ID_to_client(sender);

    // ƒовер€ем при передаче владени€ только серверному клиенту, клиенту новому владельцу если он подбирает вещь, инвентарному €щику
    if (GetServerClient() != c_from && c_parent != c_from && !smart_cast<CSE_InventoryBox*>(e_parent) && !smart_cast<CSE_ALifeHumanStalker*>(e_parent))
    {
        Msg("! WARNING: ownership: transfer called by client [%s] for item [%s], not by server!",c_from->ps->name, ent_name_safe(id_entity).c_str());
        return;
    }

    CSE_ALifeCreatureAbstract *alife_entity = smart_cast<CSE_ALifeCreatureAbstract *>(e_parent);

    if (alife_entity && !alife_entity->g_Alive() && !smart_cast<CSE_ALifeHumanStalker*>(alife_entity))
    {
        Msg("! WARNING: dead player [%d] tries to take item [%d]", id_parent, id_entity);
        return;
    }

    // Game allows ownership of entity
    if (game->OnTouch(id_parent, id_entity, bForced))
    {
        // Perform migration if needed
        if (c_parent != c_entity)
            PerformMigration(e_entity, c_entity, c_parent);

        // Rebuild parentness
        e_entity->ID_Parent = id_parent;
        e_parent->children.push_back(id_entity);

        if (bForced)
            ReplaceOwnershipHeader(P);

        // Signal to everyone (including sender)
        SendBroadcast(BroadcastCID, P, MODE);
    }
}
