//////////////////////////////////////////////////////////////////////
// inventory_owner_info.h:	для работы с сюжетной информацией
//
//////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "InventoryOwner.h"
#include "GameObject.h"
#include "xrMessages.h"
#include "ai_space.h"
#include "ai_debug.h"
#include "alife_simulator.h"
#include "alife_registry_container.h"
#include "script_game_object.h"
#include "level.h"
#include "infoportion.h"
#include "alife_registry_wrappers.h"
#include "script_callback_ex.h"
#include "game_object_space.h"
#include "xrServer_Objects_ALife_Monsters.h"

void CInventoryOwner::OnEvent(NET_Packet &P, u16 type)
{
	switch (type)
	{
	case GE_MONEY:
	{
		m_money = P.r_u32();

		if (OnServer())
		{
			CSE_Abstract* dest = Level().Server->ID_to_entity(this->object_id());
			if (CSE_ALifeTraderAbstract* pTa = smart_cast<CSE_ALifeTraderAbstract*>(dest))
				pTa->m_dwMoney = m_money;
		}
	}
	break;

	case GE_INFO_TRANSFER:
	{
		u16 id;
		shared_str info_id;
		u8 add_info;

		P.r_u16(id);		  //отправитель
		P.r_stringZ(info_id); //номер полученной информации
		P.r_u8(add_info);	  //добавление или убирание информации

		if (add_info)
			cast_game_object()->OnReceiveInfo(info_id);
		else
			cast_game_object()->OnDisableInfo(info_id);
	}
	break;
	}
}