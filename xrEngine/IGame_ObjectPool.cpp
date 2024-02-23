#include "stdafx.h"
#include "IGame_Persistent.h"
#include "igame_objectpool.h"
#include "xr_object.h"

IGame_ObjectPool::~IGame_ObjectPool()
{
	R_ASSERT(m_PrefetchObjects.empty());
}

xr_string GetSectionName()
{
	string256 gameTypeName;
	strcpy(gameTypeName, g_pGamePersistent->m_game_params.m_game_type);

	if (!strcmp(gameTypeName, "carfight"))
		strcpy(gameTypeName, "teamdeathmatch");

	string256 section;
	const char* SectionNamePrefix = "prefetch_objects_";
	strconcat(sizeof(section), section, SectionNamePrefix, gameTypeName);

	if (!pSettings->section_exist(section)) // hack for new gametypes
		strconcat(sizeof(section), section, SectionNamePrefix, "deathmatch");

	return section;
}

void IGame_ObjectPool::prefetch()
{
	R_ASSERT(m_PrefetchObjects.empty());
	::Render->model_Logging(FALSE);
	auto &sectionData = pSettings->r_section(GetSectionName().c_str()).Data;
	m_PrefetchObjects.reserve(sectionData.size());

	for (auto &item: sectionData)
		m_PrefetchObjects.push_back(create(item.first.c_str()));

	::Render->model_Logging(TRUE);
}

void IGame_ObjectPool::clear()
{
	for (CObject *obj: m_PrefetchObjects)
		xr_delete(obj);

	m_PrefetchObjects.clear();
}

CObject *IGame_ObjectPool::create(LPCSTR name)
{
	CLASS_ID CLS = pSettings->r_clsid(name, "class");
	CObject *O = reinterpret_cast<CObject*>(NEW_INSTANCE(CLS));
	O->Load(name);
	return O;
}

void IGame_ObjectPool::destroy(CObject *O)
{
	xr_delete(O);
}
