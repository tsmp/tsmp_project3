#include "stdafx.h"
#include "xrserver.h"
#include "xrserver_objects.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "..\..\TSMP3_Build_Config.h"

void ReplaceOwnershipHeader(NET_Packet &P)
{
    // Способ очень грубый, но на данный момент иного выбора нет. Заранее приношу извинения
    u16 NewType = GE_OWNERSHIP_TAKE;
    CopyMemory(&P.B.data[6], &NewType, 2);
}

void xrServer::Process_event_ownership(NET_Packet &P, ClientID const &sender, u32 time, u16 ID, BOOL bForced)
{
    const u32 MODE = net_flags(TRUE, TRUE, FALSE, TRUE);

    u16 parentId = ID, entityId;
    P.r_u16(entityId);
    CSE_Abstract *parentEntity = game->get_entity_from_eid(parentId);
    CSE_Abstract *entity = game->get_entity_from_eid(entityId);

#ifdef MP_LOGGING
    Msg("--- SV: Process ownership take: parent [%d][%s], item [%d][%s]", parentId,
        parentEntity ? parentEntity->name_replace() : "null_parent", entityId, entity ? entity->name() : "null_entity");
#endif // MP_LOGGING

    if (!parentEntity)
    {
        Msg("! ERROR on ownership: parent not found. parent_id = [%d], entity_id = [%d], frame = [%d].", parentId,
            entityId, Device.CurrentFrameNumber);
        return;
    }

    if (!entity)
    {
        Msg("! ERROR on ownership: entity not found. parent_id = [%d], entity_id = [%d], frame = [%d].",
            parentId, entityId, Device.CurrentFrameNumber);
        return;
    }

    if (0xffff != entity->ID_Parent)
    {
        Msg("! WARNING: ownership: (entity already has parent) new_parent %s id_parent %s id_entity %s [%d]", ent_name_safe(entity->ID_Parent).c_str(), ent_name_safe(parentId).c_str(), ent_name_safe(entityId).c_str(), Device.CurrentFrameNumber);
        return;
    }

    xrClientData *clientParent = parentEntity->owner;
    xrClientData *clientSender = ID_to_client(sender);

    // Доверяем при передаче владения только серверному клиенту, клиенту новому владельцу если он подбирает вещь, инвентарному ящику
    if (GetServerClient() != clientSender && clientParent != clientSender && !smart_cast<CSE_InventoryBox*>(parentEntity) && !smart_cast<CSE_ALifeHumanStalker*>(parentEntity))
    {
        Msg("! WARNING: ownership: transfer called by client [%s] for item [%s], not by server!", clientSender->ps->name, ent_name_safe(entityId).c_str());
        return;
    }

    CSE_ALifeCreatureAbstract *alife_entity = smart_cast<CSE_ALifeCreatureAbstract *>(parentEntity);

    if (alife_entity && !alife_entity->g_Alive() && !smart_cast<CSE_ALifeHumanStalker*>(alife_entity))
    {
        Msg("! WARNING: dead player [%d] tries to take item [%d]", parentId, entityId);
        return;
    }

    // Game allows ownership of entity
    if (game->OnTouch(parentId, entityId, bForced))
    {
        // Rebuild parentness
        entity->ID_Parent = parentId;
        parentEntity->children.push_back(entityId);

        if (bForced)
            ReplaceOwnershipHeader(P);

        // Signal to everyone (including sender)
        SendBroadcast(BroadcastCID, P, MODE);
    }
}
