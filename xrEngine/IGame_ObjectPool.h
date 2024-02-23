#pragma once

// refs
class ENGINE_API CObject;

class ENGINE_API IGame_ObjectPool
{
	xr_vector<CObject*> m_PrefetchObjects;

public:
	void prefetch();
	void clear();

	CObject *create(LPCSTR name);
	void destroy(CObject *O);

	IGame_ObjectPool() = default;
	~IGame_ObjectPool();
};
