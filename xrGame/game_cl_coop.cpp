#include "StdAfx.h"
#include "game_cl_Coop.h"
#include "clsid_game.h"
#include "UIGameSP.h"
#include "Actor.h"
#include "GametaskManager.h"
#include "alife_registry_wrappers.h"
#include "string_table.h"
#include "HUDManager.h"
#include "ui/UIStatic.h"

extern shared_str g_active_task_id;
extern u16 g_active_task_objective_id;

game_cl_Coop::game_cl_Coop() : m_game_ui(nullptr) {}
game_cl_Coop::~game_cl_Coop() {}

CUIGameCustom* game_cl_Coop::createGameUI()
{
	CLASS_ID clsid = CLSID_GAME_UI_SINGLE;
	CUIGameSP* pUIGame = smart_cast<CUIGameSP*>(NEW_INSTANCE(clsid));
	R_ASSERT(pUIGame);
	pUIGame->SetClGame(this);
	pUIGame->Init();
	m_game_ui = pUIGame;
	return pUIGame;
}

void game_cl_Coop::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
}

void game_cl_Coop::OnTasksSync(NET_Packet* packet)
{
	if (packet->r_u8()) // task data
	{
		auto& tasks = Actor()->GameTaskManager().GameTasks();

		shared_str taskId;
		packet->r_stringZ(taskId);
		SGameTaskKey* task = nullptr;

		auto it = std::find_if(tasks.begin(), tasks.end(), [&taskId](const SGameTaskKey& key)
		{
			return taskId == key.task_id;
		});

		if (it != tasks.end())
			task = &(*it);

		if (!task)
			task = &tasks.emplace_back();

		const u32 size = packet->r_u32();
		auto reader = IReader(&packet->B.data[packet->r_pos], size);
		task->load(reader);
	}
	else
	{
		packet->r_stringZ(g_active_task_id);
		packet->r_u16(g_active_task_objective_id);
		Actor()->GameTaskManager().initialize(Actor()->ID());
	}
}

void game_cl_Coop::OnPortionsSync(NET_Packet* packet)
{
	auto& infoportions = Actor()->m_known_info_registry->registry().objects();

	shared_str portionID;
	packet->r_stringZ(portionID);
	u64 time = packet->r_u64();

	auto it = std::find_if(infoportions.begin(), infoportions.end(), [&portionID](const INFO_DATA& data)
	{
		return portionID == data.info_id;
	});

	if (it != infoportions.end())
	{
		it->receive_time = time;
		return;
	}

	auto &newPortion = infoportions.emplace_back();
	newPortion.info_id = portionID;
	newPortion.receive_time = time;
}

void game_cl_Coop::OnSavedGamesSync(NET_Packet* P)
{
	if (P->r_u8())
		m_SavedGames.clear();

	string64 dateFormatBuf;

	auto FormatDate = [&dateFormatBuf](u32 date) -> LPCSTR
	{
		struct tm* newtime;
		time_t t = date;
		newtime = localtime(&t);

		sprintf_s(dateFormatBuf, "[%02d:%02d:%4d %02d:%02d]",
			newtime->tm_mday,
			newtime->tm_mon + 1,
			newtime->tm_year + 1900,
			newtime->tm_hour,
			newtime->tm_min);
		return dateFormatBuf;
	};

	while (!P->r_eof())
	{
		shared_str fileName, levelName;
		P->r_stringZ(fileName);
		P->r_stringZ(levelName);

		u32 modifiedDate;
		P->r_u32(modifiedDate);

		u64 gameTime;
		P->r_u64(gameTime);

		m_SavedGames.emplace_back(fileName.c_str(), levelName.c_str(), gameTime, FormatDate(modifiedDate));
		m_ActualSavedGames = !!P->r_u8();
	}
}

void game_cl_Coop::OnGameSavedNotify(NET_Packet* P)
{
	if (!g_dedicated_server)
	{
		shared_str saveName;
		P->r_stringZ(saveName);

		auto customStatic = HUD().GetUI()->UIGame()->AddCustomStatic("game_saved", true);
		customStatic->m_endTime = Device.fTimeGlobal + 3.0f; // 3sec
		string_path save_name;
		strconcat(sizeof(save_name), save_name, *CStringTable().translate("st_game_saved"), ": ", saveName.c_str());
		customStatic->wnd()->SetText(save_name);
	}
}
