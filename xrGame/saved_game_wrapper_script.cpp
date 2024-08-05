////////////////////////////////////////////////////////////////////////////
//	Module 		: saved_game_wrapper_script.cpp
//	Created 	: 21.02.2006
//  Modified 	: 21.02.2006
//	Author		: Dmitriy Iassenev
//	Description : saved game wrapper class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "saved_game_wrapper.h"
#include "ai_space.h"
#include "game_graph.h"
#include "xr_time.h"
#include "game_cl_coop.h"

xrTime CSavedGameWrapper__game_time(const CSavedGameWrapper *self)
{
	return (xrTime(self->game_time()));
}

LPCSTR CSavedGameWrapper__modified_date(const CSavedGameWrapper* self)
{
	return self->m_ModifiedDate.c_str();
}

LPCSTR CSavedGameWrapper__file_name(const CSavedGameWrapper* self)
{
	return self->m_SaveName.c_str();
}

LPCSTR CSavedGameWrapper__level_name(const CSavedGameWrapper *self)
{
	if (self->m_LevelName.empty())
		return (*ai().game_graph().header().level(self->level_id()).name());
	else
		return self->m_LevelName.c_str();
}

struct SavedGamesList
{
public:
	xr_vector<CSavedGameWrapper> m_SavesList;
	u32 Size() { return m_SavesList.size(); }
	CSavedGameWrapper GetAt(u32 idx) { return m_SavesList[idx]; }
};

SavedGamesList GetCoopSaves()
{
	if (auto coopCL = smart_cast<game_cl_Coop*>(Level().game))
	{
		if (coopCL->m_ActualSavedGames)
			return { coopCL->m_SavedGames };
	}

	return {};
}

#pragma optimize("s", on)
void CSavedGameWrapper::script_register(lua_State *L)
{
	using namespace luabind;

	module(L)
		[class_<CSavedGameWrapper>("CSavedGameWrapper")
			 .def(constructor<LPCSTR>())
			 .def("game_time", &CSavedGameWrapper__game_time)
			 .def("modified_date", &CSavedGameWrapper__modified_date)
			 .def("file_name", &CSavedGameWrapper__file_name)
			 .def("level_name", &CSavedGameWrapper__level_name)
			 .def("level_id", &CSavedGameWrapper::level_id)
			 .def("actor_health", &CSavedGameWrapper::actor_health),

		def("valid_saved_game", (bool (*)(LPCSTR))(&valid_saved_game)),
		def("GetCoopSaves", GetCoopSaves),

		class_<SavedGamesList>("SavedGamesList")
			.def("Size", &SavedGamesList::Size)
			.def("GetAt", &SavedGamesList::GetAt)];
}
