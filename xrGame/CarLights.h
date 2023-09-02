#pragma once
#include "Render.h"

class CCarLights;
class CCar;

struct SCarLight
{
	ref_light light_render;
	ref_glow glow_render;
	u16 bone_id;
	CCarLights *m_holder;
	SCarLight();
	~SCarLight();
	void Switch();
	void TurnOn();
	void TurnOff();
	bool isOn();
	void Init(CCarLights *holder);
	void Update();
	void ParseDefinitions(LPCSTR section);
};

DEFINE_VECTOR(SCarLight *, LIGHTS_STORAGE, LIGHTS_I)
class CCarLights
{
public:
	void ParseDefinitions();
	void ParseDefinitionsTail();
	void ParseDefinitionsExhaust();
	void ParseDefinitionsNitro();
	void Init(CCar *pcar);
	void Update();
	CCar *PCar() { return m_pcar; }
	void SwitchHeadLights();
	void TurnOnHeadLights();
	void TurnOffHeadLights();
	void SwitchTailLights();
	void TurnOnTailLights();
	void TurnOffTailLights();
	void SwitchExhaustLights();
	void TurnOnExhaustLights();
	void TurnOffExhaustLights();
	void SwitchNitroLights();
	void TurnOnNitroLights();
	void TurnOffNitroLights();
	bool IsLightTurnedOn();
	bool IsTailLightTurnedOn();
	bool IsExhaustLightTurnedOn();
	bool IsNitroLightTurnedOn();
	bool IsLight(u16 bone_id);
	bool findLight(u16 bone_id, SCarLight *&light);
	bool findTailLight(u16 bone_id, SCarLight *&light);
	bool findExhaustLight(u16 bone_id, SCarLight *&light);
	bool findNitroLight(u16 bone_id, SCarLight *&light);
	CCarLights();
	~CCarLights();

protected:
	struct SFindLightPredicate
	{
		const SCarLight *m_light;

		SFindLightPredicate(const SCarLight* light) : m_light(light)
		{
		}

		bool operator()(const SCarLight *light) const
		{
			return light->bone_id == m_light->bone_id;
		}
	};

	struct SFindTailLightPredicate
	{
		const SCarLight *t_light;

		SFindTailLightPredicate(const SCarLight* light) : t_light(light)
		{
		}

		bool operator()(const SCarLight *light) const
		{
			return light->bone_id == t_light->bone_id;
		}
	};

	struct SFindExhaustLightPredicate
	{
		const SCarLight *e_light;

		SFindExhaustLightPredicate(const SCarLight* light) : e_light(light)
		{
		}

		bool operator()(const SCarLight *light) const
		{
			return light->bone_id == e_light->bone_id;
		}
	};

	struct SFindNitroLightPredicate
	{
		const SCarLight *n_light;

		SFindNitroLightPredicate(const SCarLight* light) : n_light(light)
		{
		}

		bool operator()(const SCarLight *light) const
		{
			return light->bone_id == n_light->bone_id;
		}
	};

	LIGHTS_STORAGE m_lights;
	LIGHTS_STORAGE t_lights;
	LIGHTS_STORAGE e_lights;
	LIGHTS_STORAGE n_lights;
	CCar *m_pcar;
};
