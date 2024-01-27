#pragma once

class ServerRespawnManager
{
	struct ObjectToRespawn
	{
		u16 objectID;
		u32 secondsToRespawn;
		u32 secondsElapsed;
		shared_str section;
		shared_str customData;
		Fvector pos;
	};

	UINT_PTR m_RespawnTimerHandle = { 0 };
	xr_vector<ObjectToRespawn> m_ObjectsToRespawn;
	using RespObjIt = decltype(m_ObjectsToRespawn)::iterator;

	RespObjIt FindObject(u16 objectID);

public:
	ServerRespawnManager();
	~ServerRespawnManager();

	void RegisterToRespawn(CSE_Abstract* object);
	bool RegisteredForRespawn(u16 objectID);
	void MarkReadyForRespawn(u16 objectID);
	void CleanRespawnList();
	void OnTimer();
};