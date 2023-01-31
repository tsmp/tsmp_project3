#pragma once
class ObjectRespawnClass
{
public:

	// Вызывать в таймере - находим объекты, у которых нулевой айдишник с просроченым временем и спавним его.
	static void CheckRespawnObjects();

	// В xrServer::Process_event_destroy
	static int DestroyRespawnID(u16 id);

	// добавить объект в респавн xrServer::SLS_Default()
	static void AddObject(shared_str &pSection, u16 pID, int pTimeRespawn, Fvector &XYZ);

	// вызвать перед вызовом чтения level.spawn xrServer::SLS_Default()
	static void DestroyRespawner();

	u16 id_object;
	int time_tick;
	u16 time_respawn;
	Fvector spawn_position;
	shared_str section;
};