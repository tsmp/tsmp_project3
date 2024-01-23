#include "stdAfx.h"
#include "HeliCall.h"
#include "Actor.h"
#include "HeliKillerMode.h"

void CHeliCall::UseBy(CEntityAlive* user)
{
	if (!m_Called && OnServer())
	{
		if (auto playerState = Level().Server->game->get_eid(user->ID()))
		{
			m_Called = true;
			Msg("- Player [%s] called heli!", playerState->name);
			HeliKillerMode::SpawnHeli(playerState);
		}
		else
			Msg("! can not call heli!");
	}

	if (m_iPortionsNum > 0)
		--m_iPortionsNum;
	else
		m_iPortionsNum = 0;
}
