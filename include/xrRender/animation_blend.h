#pragma once
#include "animation_motion.h"

// TSMP: бленд из тч, чн вариант не перенесен

//*** Run-time Blend definition *******************************************************************
class CBlend
{
public:
	enum ECurvature
	{
		eFREE_SLOT = 0,
		eAccrue,
		eFalloff,
		eFORCEDWORD = u32(-1)
	};

public:
	float blendAmount;
	float timeCurrent;
	float timeTotal;
	MotionID motionID;
	u16 bone_or_part; // startup parameters
	u8 channel;
	ECurvature blend;
	float blendAccrue;	// increasing
	float blendFalloff; // decreasing
	float blendPower;
	float speed;

	BOOL playing; // TSMP: можно оптимизировать эти 12 байт BOOL-ов в однобайтовый флаг
	BOOL stop_at_end;
	BOOL fall_at_end;
	PlayCallback Callback;
	void* CallbackParam;

	u32 dwFrame;

	u32 mem_usage() { return sizeof(*this); }
	IC void update_play(float dt);
	IC bool update_falloff(float dt);
	IC bool update(float dt);
};

IC bool UpdateFalloffBlend(CBlend& B, float dt)
{
	B.blendAmount -= dt * B.blendFalloff * B.blendPower;
	return B.blendAmount <= 0;
}

//returns true if play time out
IC bool UpdatePlayBlend(CBlend& B, float dt)
{
	B.blendAmount += dt * B.blendAccrue * B.blendPower;

	if (B.blendAmount > B.blendPower)
		B.blendAmount = B.blendPower;

	if (B.stop_at_end && (B.timeCurrent > (B.timeTotal - SAMPLE_SPF)))
	{
		B.timeCurrent = B.timeTotal - SAMPLE_SPF; // stop@end - time frozen at the end
		if (B.playing && B.Callback)
			B.Callback(&B); // callback only once
		B.playing = FALSE;

		return true;
	}

	return false;
}

IC void CBlend::update_play(float dt)
{
	CBlend& B = *this;

	if (UpdatePlayBlend(B, dt))
	{
		if (B.fall_at_end)
		{
			B.blend = CBlend::eFalloff;
			B.blendFalloff = 2.f;
		}
	}
}

IC bool CBlend::update_falloff(float dt)
{
	return UpdateFalloffBlend(*this, dt);
}

IC bool CBlend::update(float dt)
{
	switch (blend)
	{
	case eAccrue:
		update_play(dt);
		break;
	case eFalloff:
		if (update_falloff(dt))
			return true;
		break;
	default:
		NODEFAULT;
	}
	return false;
}

class IBlendDestroyCallback
{
public:
	virtual void BlendDestroy(CBlend &blend) = 0;
};