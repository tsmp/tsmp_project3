#include "StdAfx.h"
#include "game_sv_Coop.h"
#include "xrserver_objects_alife_monsters.h"
#include "xrServer.h"
#include "ai_space.h"
#include "Actor.h"
#include "GametaskManager.h"
#include "alife_registry_wrappers.h"
#include "saved_game_wrapper.h"

ENGINE_API bool g_dedicated_server;
extern shared_str g_active_task_id;
extern u16 g_active_task_objective_id;

game_sv_Coop::game_sv_Coop()
{
	m_phase = GAME_PHASE_NONE;
	m_type = GAME_COOP;
}

game_sv_Coop::~game_sv_Coop() {}

void game_sv_Coop::Create(shared_str& options)
{
	inherited::Create(options);
	switch_Phase(GAME_PHASE_PENDING);
}

void game_sv_Coop::Update()
{
	inherited::Update();

	switch (Phase())
	{
	case GAME_PHASE_PENDING:
		OnRoundStart();
		break;
	}
}

void game_sv_Coop::OnPlayerConnect(const ClientID& id_who)
{
	inherited::OnPlayerConnect(id_who);
	xrClientData* xrCData = m_server->ID_to_client(id_who);
	game_PlayerState* ps_who = get_id(id_who);

	if (!xrCData->flags.bReconnect)
	{
		ps_who->clear();
		ps_who->team = 0;
		ps_who->skin = -1;
	}

	ps_who->setFlag(GAME_PLAYER_FLAG_SPECTATOR);
	ps_who->resetFlag(GAME_PLAYER_FLAG_SKIP);

	if (g_dedicated_server && xrCData == m_server->GetServerClient())
	{
		ps_who->setFlag(GAME_PLAYER_FLAG_SKIP);
		return;
	}

	SetPlayersDefItems(ps_who);
}

void game_sv_Coop::OnPlayerConnectFinished(const ClientID& id_who)
{
	xrClientData* xrCData = m_server->ID_to_client(id_who);

	if (!xrCData->owner)
		SpawnPlayer(id_who, "spectator");
	else
	{
		SendInfoPortions(id_who);
		SendTasks(id_who);
	}

	if (xrCData)
	{
		NET_Packet P;
		GenerateGameMessage(P);
		P.w_u32(GAME_EVENT_PLAYER_CONNECTED);
		P.w_stringZ(xrCData->name.c_str());
		SvEventSend(P);
	}
}

void game_sv_Coop::OnEvent(NET_Packet& P, u16 type, u32 time, const ClientID& sender)
{
	if (type == GAME_EVENT_PLAYER_KILL) // g_kill
	{
		u16 ID = P.r_u16();

		if (xrClientData* l_pC = get_client(ID))
			KillPlayer(l_pC->ID, l_pC->ps->GameID);

		return;
	}

	inherited::OnEvent(P, type, time, sender);
}

void game_sv_Coop::OnPlayerDisconnect(const ClientID& id_who, LPSTR Name, u16 GameID)
{
	inherited::OnPlayerDisconnect(id_who, Name, GameID);
}

void game_sv_Coop::OnPlayerReady(const ClientID& id)
{
	if (m_phase == GAME_PHASE_INPROGRESS)
	{
		xrClientData* xrCData = m_server->ID_to_client(id);
		game_PlayerState* ps = get_id(id);
		if (ps->IsSkip())
			return;

		if (!(ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)))
			return;

		xrClientData* xrSCData = static_cast<xrClientData*>(m_server->GetServerClient());
		CSE_Abstract* pOwner = xrCData->owner;

		RespawnPlayer(id, false);
		pOwner = xrCData->owner;
		SpawnDefaultItemsForPlayer(pOwner->ID);
	}
}

shared_str game_sv_Coop::level_name(const shared_str& server_options) const
{
	if (!ai().get_alife())
		return (inherited::level_name(server_options));

	return (alife().level_name());
}

void game_sv_Coop::SetSkin(CSE_Abstract* E, u16 Team, u16 ID)
{
	if (auto visual = smart_cast<CSE_Visual*>(E))
		visual->set_visual("actors\\hero\\stalker_novice.ogf");
}

bool game_sv_Coop::AssignOwnershipToConnectingClient(CSE_Abstract* E, xrClientData* CL)
{
	auto act = smart_cast<CSE_ALifeCreatureActor*>(E);

	if (!act || act->owner != server().GetServerClient())
		return false;

	return CL->name == act->name_replace();
}

void game_sv_Coop::SendTasks(const ClientID& target)
{
	auto& tasks = Actor()->GameTaskManager().GameTasks();

	for (auto& task : tasks)
	{
		CMemoryWriter taskSerialized;
		task.save(taskSerialized);

		NET_Packet packet;
		packet.w_begin(M_TASKS_SYNC);
		packet.w_u8(1); // task data
		packet.w_stringZ(task.task_id);
		packet.w_u32(taskSerialized.size());
		packet.w(taskSerialized.pointer(), taskSerialized.size());
		server().SendTo(target, packet);
	}

	NET_Packet packet;
	packet.w_begin(M_TASKS_SYNC);
	packet.w_u8(0);
	packet.w_stringZ(g_active_task_id);
	packet.w_u16(g_active_task_objective_id);
	server().SendTo(target, packet);
}

void game_sv_Coop::SendInfoPortions(const ClientID& target)
{
	auto &infoportions = Actor()->m_known_info_registry->registry().objects();

	for (auto &portion: infoportions)
	{
		NET_Packet packet;
		packet.w_begin(M_INFOPORTIONS_SYNC);
		packet.w_stringZ(portion.info_id);
		packet.w_u64(portion.receive_time);
		server().SendTo(target, packet);
	}
}

void game_sv_Coop::SpawnDefaultItemsForPlayer(u16 actorId)
{
	static const LPCSTR itemsToSpawn[] =
	{
		"wpn_binoc",
		"detector_simple",
		"novice_outfit",
		"device_torch",
		"device_pda",
		"wpn_knife"
	};

	for (LPCSTR itemSection : itemsToSpawn)
		SpawnWeapon4Actor(actorId, itemSection, 0);
}

void game_sv_Coop::OnPlayerRequestSavedGames(ClientID const& sender)
{
	const LPCSTR path = "$game_saves$";

	FS_Path* P = FS.get_path(path);
	P->m_Flags.set(FS_Path::flNeedRescan, TRUE);
	FS.m_Flags.set(CLocatorAPI::flNeedCheck, TRUE);
	FS.rescan_pathes();

	FS_FileSet files;
	FS.file_list(files, path, FS_ListFiles | FS_RootOnly, "*.sav");
	FS.m_Flags.set(CLocatorAPI::flNeedCheck, FALSE);

	struct SaveFile
	{
		xr_string fileName;
		xr_string levelName;
		u64 gameTime;
		u32 modifiedTime;

		bool operator <(const SaveFile& rhs)
		{
			return this->modifiedTime > rhs.modifiedTime;
		}

		u32 size()
		{
			return static_cast<u32>(fileName.size() + 1 + levelName.size() + 1 + sizeof(gameTime) + sizeof(modifiedTime));
		}
	};

	xr_vector<SaveFile> saves;
	saves.reserve(files.size());

	for (auto& file : files)
	{
		if (!file.name.empty())
		{
			extern LPCSTR CSavedGameWrapper__level_name(const CSavedGameWrapper* self);

			const auto fileName = EFS.ExtractFileName(file.name.c_str());
			Msg("* reading save %s", fileName.c_str());
			CSavedGameWrapper wrapper(fileName.c_str());
			auto &save = saves.emplace_back();

			save.fileName = fileName;
			save.gameTime = wrapper.game_time();
			save.levelName = CSavedGameWrapper__level_name(&wrapper);
			save.modifiedTime = static_cast<u32>(file.time_write);
		}
	}

	std::sort(saves.begin(), saves.end());

	NET_Packet responsePacket;
	responsePacket.w_begin(M_RESPOND_SAVED_GAMES);
	responsePacket.w_u8(1); // first

	for (u32 i = 0, cnt = saves.size(); i < cnt; i++)
	{
		auto it = saves[i];

		if (responsePacket.B.count + it.size() >= NET_PacketSizeLimit)
		{
			server().SendTo(sender, responsePacket, net_flags(TRUE, TRUE));
			responsePacket.w_begin(M_RESPOND_SAVED_GAMES);
			responsePacket.w_u8(0); // not first
		}

		responsePacket.w_stringZ(it.fileName.c_str());
		responsePacket.w_stringZ(it.levelName.c_str());
		responsePacket.w_u32(it.modifiedTime);
		responsePacket.w_u64(it.gameTime);
		responsePacket.w_u8(i + 1 == cnt); // last or not
	}

	server().SendTo(sender, responsePacket, net_flags(TRUE, TRUE));
}
