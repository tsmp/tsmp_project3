#include "StdAfx.h"
#include "game_cl_Coop.h"
#include "clsid_game.h"
#include "UIGameSP.h"
#include "Actor.h"
#include "GametaskManager.h"
#include "alife_registry_wrappers.h"

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
