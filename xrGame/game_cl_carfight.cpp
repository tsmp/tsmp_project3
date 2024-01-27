#include "StdAfx.h"
#include "game_cl_Carfight.h"
#include "Actor.h"
#include "Car.h"

extern int g_car_camera_view;

game_cl_Carfight::game_cl_Carfight()
{
	// set 3rd view for car
	g_car_camera_view = 2;
}

void game_cl_Carfight::OnSpawn(CObject* pObj)
{
	inherited::OnSpawn(pObj);
	const auto actor = Actor();

	if (pObj && actor && actor->ID() == pObj->ID())
	{
		if (CCar* car = smart_cast<CCar*>(Actor()->Holder()))
			car->StartEngine();
	}
}
