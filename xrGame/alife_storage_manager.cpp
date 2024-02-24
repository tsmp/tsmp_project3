////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_storage_manager.cpp
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator storage manager
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "alife_storage_manager.h"
#include "alife_simulator_header.h"
#include "alife_time_manager.h"
#include "alife_spawn_registry.h"
#include "alife_object_registry.h"
#include "alife_graph_registry.h"
#include "alife_group_registry.h"
#include "alife_registry_container.h"
#include "xrserver.h"
#include "level.h"
#include "saved_game_wrapper.h"
#include "string_table.h"
#include "igame_persistent.h"

using namespace ALife;

extern string_path g_last_saved_game;

void CALifeStorageManager::save(LPCSTR save_name, bool update_name)
{
	strcpy_s(g_last_saved_game, sizeof(g_last_saved_game), save_name);

	string_path save;
	strcpy(save, m_save_name);

	if (save_name)
		strconcat(sizeof(m_save_name), m_save_name, save_name, SAVE_EXTENSION);
	else
	{
		if (!xr_strlen(m_save_name))
		{
			Log("There is no file name specified!");
			return;
		}
	}

	u32 sourceCount;
	u32 destCount;
	void *destData;

	{
		CMemoryWriter stream;
		auto simulator = ai().m_alife_simulator;
		simulator->header().save(stream);
		simulator->time_manager().save(stream);
		simulator->spawns().save(stream);
		simulator->objects().save(stream);
		simulator->registry().save(stream);

		sourceCount = stream.tell();
		void *source_data = stream.pointer();
		destCount = rtc_csize(sourceCount);
		destData = xr_malloc(destCount);
		destCount = rtc_compress(destData, destCount, source_data, sourceCount);
	}

	string_path temp;
	FS.update_path(temp, "$game_saves$", m_save_name);
	IWriter *writer = FS.w_open(temp);
	writer->w_u32(u32(-1));
	writer->w_u32(ALIFE_VERSION);

	writer->w_u32(sourceCount);
	writer->w(destData, destCount);
	xr_free(destData);
	FS.w_close(writer);

#ifdef DEBUG
	Msg("* Game %s is successfully saved to file '%s' (%d bytes compressed to %d)", m_save_name, temp, sourceCount, destCount + 4);
#else  // DEBUG
	Msg("* Game %s is successfully saved to file '%s'", m_save_name, temp);
#endif // DEBUG

	if (!update_name)
		strcpy(m_save_name, save);
}

void CALifeStorageManager::load(void *buffer, const u32 &buffer_size, LPCSTR file_name)
{
	IReader source(buffer, buffer_size);
	auto simulator = ai().m_alife_simulator;
	simulator->header().load(source);
	simulator->time_manager().load(source);
	simulator->spawns().load(source, file_name);
	simulator->graph().on_load();
	simulator->objects().load(source);

	VERIFY(simulator->can_register_objects());
	simulator->can_register_objects(false);
	auto &simulatorObjects = simulator->objects().objects();

	for (auto &pair: simulatorObjects)
	{
		CSE_ALifeDynamicObject* object = pair.second;
		ALife::_OBJECT_ID id = object->ID;
		object->ID = simulator->server().PerformIDgen(id);
		VERIFY(id == object->ID);
		simulator->register_object(object, false);
	}

	simulator->registry().load(source);
	simulator->can_register_objects(true);

	for (auto& pair : simulatorObjects)
		pair.second->on_register();
}

bool CALifeStorageManager::load(LPCSTR saveName)
{
	CTimer timer;
	timer.Start();

	string256 save;
	strcpy(save, m_save_name);

	if (!saveName)
	{
		if (!xr_strlen(m_save_name))
			R_ASSERT2(false, "There is no file name specified!");
	}
	else
		strconcat(sizeof(m_save_name), m_save_name, saveName, SAVE_EXTENSION);

	string_path fileName;
	FS.update_path(fileName, "$game_saves$", m_save_name);

	IReader *stream;
	stream = FS.r_open(fileName);

	if (!stream)
	{
		Msg("* Cannot find saved game %s", fileName);
		strcpy(m_save_name, save);
		return false;
	}

	CHECK_OR_EXIT(CSavedGameWrapper::valid_saved_game(*stream), make_string("%s\nSaved game version mismatch or saved game is corrupted", fileName));

	string512 temp;
	strconcat(sizeof(temp), temp, CStringTable().translate("st_loading_saved_game").c_str(), " \"", saveName, SAVE_EXTENSION, "\"");
	g_pGamePersistent->LoadTitle(temp);

	auto simulator = ai().m_alife_simulator;
	simulator->unload();
	simulator->reload(m_section);

	u32 sourceCount = stream->r_u32();
	void *sourceData = xr_malloc(sourceCount);
	rtc_decompress(sourceData, sourceCount, stream->pointer(), stream->length() - 3 * sizeof(u32));
	FS.r_close(stream);
	load(sourceData, sourceCount, fileName);
	xr_free(sourceData);

	simulator->groups().on_after_game_load();
	VERIFY(simulator->graph().actor());

	Msg("* Game %s is successfully loaded from file '%s' (%.3fs)", saveName, fileName, timer.GetElapsed_sec());
	return true;
}

void CALifeStorageManager::save(NET_Packet &net_packet)
{
	prepare_objects_for_save();

	shared_str game_name;
	net_packet.r_stringZ(game_name);
	save(*game_name, !!net_packet.r_u8());
}

void CALifeStorageManager::prepare_objects_for_save()
{
	Level().ClientSend();
	Level().ClientSave();
}
