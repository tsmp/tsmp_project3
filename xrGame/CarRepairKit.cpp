#include "stdAfx.h"
#include "CarRepairKit.h"
#include "Actor.h"
#include "Car.h"

void CCarRepairKit::UseBy(CEntityAlive* user)
{
	if (auto act = smart_cast<CActor*>(user))
	{
		if (auto car = smart_cast<CCar*>(act->Holder()))		
			car->Repair();		
	}

	if (m_iPortionsNum > 0)
		--(m_iPortionsNum);
	else
		m_iPortionsNum = 0;
}
