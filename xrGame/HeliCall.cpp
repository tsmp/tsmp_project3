#include "stdAfx.h"
#include "HeliCall.h"
#include "Actor.h"
#include "HeliKillerMode.h"

void CHeliCall::UseBy(CEntityAlive* user)
{
	if (!m_Called && OnServer())
	{
		if (auto usedClient = Level().Server->game->get_client(user->ID()))
		{
			m_Called = true;
			Msg("- Player [%s] called heli!", usedClient->ps->name);
			HeliKillerMode::SpawnHeli(usedClient);
		}
		else
			Msg("! can not call heli!");
	}

	if (m_iPortionsNum > 0)
		--m_iPortionsNum;
	else
		m_iPortionsNum = 0;
}
