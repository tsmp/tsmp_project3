#include "stdafx.h"
#pragma hdrstop

#ifdef DEBUG
#include "ode_include.h"
#include "StatGraph.h"
#include "PHDebug.h"
#include "phworld.h"
#endif

#include "alife_space.h"
#include "hit.h"
#include "PHDestroyable.h"
#include "car.h"
#include "actor.h"
#include "cameralook.h"
#include "camerafirsteye.h"
#include "level.h"
#include "cameramanager.h"

BOOL show_arms_on_steering_wheel = FALSE;
int g_car_camera_view = 0;

bool CCar::HUDView() const
{
	if (show_arms_on_steering_wheel)
		return false;

	return active_camera->tag == ectFirst;
}

void CCar::cam_Update(float dt, float fov)
{
	DEBUG_VERIFY(!ph_world->Processing());
	Fvector P, Da;
	Da.set(0, 0, 0);

	switch (active_camera->tag)
	{
	case ectFirst:
		XFORM().transform_tiny(P, m_camera_position);
		// rotate head
		if (OwnerActor())
			OwnerActor()->Orientation().yaw = -active_camera->yaw;
		if (OwnerActor())
			OwnerActor()->Orientation().pitch = -active_camera->pitch;
		break;

	case ectChase:
		XFORM().transform_tiny(P, m_camera_position_lookat);
		break;

	case ectFree:
		XFORM().transform_tiny(P, m_camera_position_free);
		break;
	}

	active_camera->f_fov = fov;
	active_camera->Update(P, Da);
	Level().Cameras().Update(active_camera);
}

void CCar::OnCameraChange(int type)
{
	if (Owner())
		Owner()->setVisible(type != ectFirst || show_arms_on_steering_wheel);

	if (active_camera && active_camera->tag == type)
		return;

	active_camera = camera[type];

	if (ectFree == type)
	{
		Fvector xyz;
		XFORM().getXYZi(xyz);
		active_camera->yaw = xyz.y;
	}

	g_car_camera_view = type;
}
