#pragma once
#include <string>
using namespace std;

class ObjectRespawnClass
{
public:

	// Вызывать в таймере - находим объекты, у которых нулевой айдишник с просроченым временем и спавним его.
	void CheckRespawnObjects();

	// В xrServer::Process_event_destroy
	int DestroyRespawnID(int id);

	// добавить объект в респавн xrServer::SLS_Default()
	void AddObject(string _section, int _id, int _time_respawn, float _pos_x, float _pos_y, float _pos_z);

	// вызвать перед вызовом чтения level.spawn xrServer::SLS_Default()
	void DestroyRespawner();


	int id_object;
	int time_tick;
	int time_respawn;
	float pos_x;
	float pos_y;
	float pos_z;
	string section;
};

/*
	В xrServer::SLS_Default()
	1) перед чтением level spawn нужно очистить список
	ServerRespawnManager.DestroyRespawner();

	2) в самом низу меняем Process_spawn(packet, clientID); на
	CSE_Abstract* E = Process_spawn(packet, clientID);
	ServerRespawnManager.AddObject(E->s_name.c_str(), E->ID, E->RespawnTime, E->o_Position.x, E->o_Position.y, E->o_Position.z);


	1) в xrServer::Process_event_destroy
	добавляем ServerRespawnManager.DestroyRespawnID(id_dest);


	По таймеру раз в секунду или в 10 (нужно будет заменить  на     respawn->time_tick +=10) секунд вызывать функцию CheckRespawnObjects
*/
