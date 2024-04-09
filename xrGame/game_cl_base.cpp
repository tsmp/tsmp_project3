#include "pch_script.h"
#include "hudmanager.h"
#include "game_cl_base.h"
#include "level.h"
#include "GamePersistent.h"
#include "UIGameCustom.h"
#include "script_engine.h"
#include "xr_Level_controller.h"
#include "ui/UIMainIngameWnd.h"
#include "UI/UIGameTutorial.h"
#include "UI/UIMessagesWindow.h"
#include "string_table.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "Console.h"
#include "game_sv_mp_vote_flags.h"

struct Color
{
	u32 r;
	u32 g;
	u32 b;
	u32 a;

	u32 GetRGBA() { return color_rgba(r, g, b, a); }
	void GetStr(string32 &str)
	{
		string32 ch;
		sprintf(ch, "11[%u,%u,%u,%u]", a, r, g, b);
		ch[0] = '%';
		ch[1] = 'c';

		strcpy(str, ch);
	}
};

extern u32 Color_Teams_u32[3];
extern string32 Color_Teams[3];

game_cl_GameState::game_cl_GameState()
{
	m_WeaponUsageStatistic = xr_new<WeaponUsageStatistic>();
	m_ShowPlayerNamesOnCrosshair = false;
	m_ColorPlayersOnCrosshair = false;

	local_player = 0;
	m_game_type_name = 0;

	shedule.t_min = 5;
	shedule.t_max = 20;
	m_game_ui_custom = NULL;
	shedule_register();

	m_u16VotingEnabled = 0;
	m_bServerControlHits = true;

	Color team0;
	Color team1;
	Color team2;

	if (pSettings->section_exist("mp_team_0_color_override"))
	{
		team0.r = pSettings->r_u32("mp_team_0_color_override", "r");
		team0.g = pSettings->r_u32("mp_team_0_color_override", "g");
		team0.b = pSettings->r_u32("mp_team_0_color_override", "b");
		team0.a = pSettings->r_u32("mp_team_0_color_override", "a");
	}
	else
	{
		team0.r = 255;
		team0.g = 240;
		team0.b = 190;
		team0.a = 255;
	}

	if (pSettings->section_exist("mp_team_1_color_override"))
	{
		team1.r = pSettings->r_u32("mp_team_1_color_override", "r");
		team1.g = pSettings->r_u32("mp_team_1_color_override", "g");
		team1.b = pSettings->r_u32("mp_team_1_color_override", "b");
		team1.a = pSettings->r_u32("mp_team_1_color_override", "a");
	}
	else
	{
		team1.r = 64;
		team1.g = 255;
		team1.b = 64;
		team1.a = 255;
	}

	if (pSettings->section_exist("mp_team_2_color_override"))
	{
		team2.r = pSettings->r_u32("mp_team_2_color_override", "r");
		team2.g = pSettings->r_u32("mp_team_2_color_override", "g");
		team2.b = pSettings->r_u32("mp_team_2_color_override", "b");
		team2.a = pSettings->r_u32("mp_team_2_color_override", "a");
	}
	else
	{
		team2.r = 64;
		team2.g = 64;
		team2.b = 255;
		team2.a = 255;
	}

	Color_Teams_u32[0] = team0.GetRGBA();
	Color_Teams_u32[1] = team1.GetRGBA();
	Color_Teams_u32[2] = team2.GetRGBA();

	team0.GetStr(Color_Teams[0]);
	team1.GetStr(Color_Teams[1]);
	team2.GetStr(Color_Teams[2]);
}

game_cl_GameState::~game_cl_GameState()
{
	PLAYERS_MAP_IT I = players.begin();
	for (; I != players.end(); ++I)
		xr_delete(I->second);
	players.clear();

	shedule_unregister();

	xr_delete(m_WeaponUsageStatistic);
}

void game_cl_GameState::net_import_GameTime(NET_Packet &P)
{
	//time
	u64 GameTime;
	P.r_u64(GameTime);
	float TimeFactor;
	P.r_float(TimeFactor);

	Level().SetGameTimeFactor(GameTime, TimeFactor);

	u64 GameEnvironmentTime;
	P.r_u64(GameEnvironmentTime);
	float EnvironmentTimeFactor;
	P.r_float(EnvironmentTimeFactor);

	u64 OldTime = Level().GetEnvironmentGameTime();
	Level().SetEnvironmentGameTimeFactor(GameEnvironmentTime, EnvironmentTimeFactor);
	if (OldTime > GameEnvironmentTime)
		GamePersistent().Environment().Invalidate();
}

void game_cl_GameState::net_import_state(NET_Packet &P)
{
	// Generic
	P.r_clientID(local_svdpnid);
	P.r_u32(m_type);

	u16 ph;
	P.r_u16(ph);

	if (Phase() != ph)
		switch_Phase(ph);

	P.r_s32(m_round);
	P.r_u32(m_start_time);
	m_u16VotingEnabled = u16(P.r_u8());
	m_bServerControlHits = !!P.r_u8();
	m_WeaponUsageStatistic->SetCollectData(!!P.r_u8());
	m_ShowPlayerNamesOnCrosshair = !!P.r_u8();
	m_ColorPlayersOnCrosshair = !!P.r_u8();

	// Players
	u16 p_count;
	P.r_u16(p_count);

	PLAYERS_MAP players_new;

	/*
	players.clear	();
*/
	PLAYERS_MAP_IT I;
	for (u16 p_it = 0; p_it < p_count; ++p_it)
	{
		ClientID ID;
		P.r_clientID(ID);

		game_PlayerState *IP;
		I = players.find(ID);
		if (I != players.end())
		{
			IP = I->second;
			//***********************************************
			u16 OldFlags = IP->flags__;
			u8 OldVote = IP->m_bCurrentVoteAgreed;
			//-----------------------------------------------
			IP->net_Import(P);
			//-----------------------------------------------
			if (OldFlags != IP->flags__)
				if (Type() != GAME_SINGLE)
					OnPlayerFlagsChanged(IP);
			if (OldVote != IP->m_bCurrentVoteAgreed)
				OnPlayerVoted(IP);
			//***********************************************

			players_new.insert(mk_pair(ID, IP));
			players.erase(I);
		}
		else
		{
			IP = createPlayerState();
			IP->net_Import(P);

			if (Type() != GAME_SINGLE)
				OnPlayerFlagsChanged(IP);

			players_new.insert(mk_pair(ID, IP));
		}
		if (IP->testFlag(GAME_PLAYER_FLAG_LOCAL))
			local_player = IP;
	}

	I = players.begin();
	for (; I != players.end(); ++I)
		xr_delete(I->second);
	players.clear();

	players = players_new;

	net_import_GameTime(P);
}

void game_cl_GameState::net_import_update(NET_Packet &P)
{
	// Read
	ClientID ID;
	P.r_clientID(ID);

	// Update
	PLAYERS_MAP_IT I = players.find(ID);
	if (players.end() != I)
	{
		game_PlayerState *IP = I->second;
		//		CopyMemory	(&IP,&PS,sizeof(PS));
		//***********************************************
		u16 OldFlags = IP->flags__;
		u8 OldVote = IP->m_bCurrentVoteAgreed;
		//-----------------------------------------------
		IP->net_Import(P);
		//-----------------------------------------------
		if (OldFlags != IP->flags__)
			if (Type() != GAME_SINGLE)
				OnPlayerFlagsChanged(IP);
		if (OldVote != IP->m_bCurrentVoteAgreed)
			OnPlayerVoted(IP);
		//***********************************************
	}
	else
	{
		game_PlayerState *PS = createPlayerState();
		PS->net_Import(P);
		if (Type() != GAME_SINGLE)
			OnPlayerFlagsChanged(PS);
		xr_delete(PS);
	};

	//Syncronize GameTime
	net_import_GameTime(P);
}

void game_cl_GameState::net_signal(NET_Packet &P)
{
}

void game_cl_GameState::TranslateGameMessage(u32 msg, NET_Packet &P)
{
	CStringTable st;

	string512 Text;
	char Color_Main[] = "%c[255,192,192,192]";

	switch (msg)
	{
	case GAME_EVENT_PLAYER_CONNECTED:
	{
		string64 PlayerName;
		P.r_stringZ(PlayerName);

		sprintf_s(Text, "%s%s %s%s", Color_Teams[0], PlayerName, Color_Main, *st.translate("mp_connected"));
		CommonMessageOut(Text);

		Msg("%s connected", PlayerName);

		if (g_dedicated_server)
			Console->Execute("sv_listplayers");
	}
	break;
	case GAME_EVENT_PLAYER_DISCONNECTED:
	{
		string64 PlayerName;
		P.r_stringZ(PlayerName);

		sprintf_s(Text, "%s%s %s%s", Color_Teams[0], PlayerName, Color_Main, *st.translate("mp_disconnected"));
		CommonMessageOut(Text);
		Msg("%s disconnected", PlayerName);
	}
	break;
	case GAME_EVENT_PLAYER_ENTERED_GAME:
	{
		string64 PlayerName;
		P.r_stringZ(PlayerName);

		sprintf_s(Text, "%s%s %s%s", Color_Teams[0], PlayerName, Color_Main, *st.translate("mp_entered_game"));
		CommonMessageOut(Text);
	}
	break;
	default:
	{
		R_ASSERT2(0, "Unknown Game Message");
	}
	break;
	};
}

void game_cl_GameState::OnGameMessage(NET_Packet &P)
{
	VERIFY(this && &P);
	u32 msg;
	P.r_u32(msg);

	TranslateGameMessage(msg, P);
};

void game_cl_GameState::ForEachPlayer(luabind::functor<void> functor)
{
	for (auto& playerPair : players)
		functor(playerPair.second, this);
}

game_PlayerState *game_cl_GameState::GetPlayerByGameID(u32 GameID)
{
	PLAYERS_MAP_IT I = players.begin();
	PLAYERS_MAP_IT E = players.end();

	for (; I != E; ++I)
	{
		game_PlayerState *P = I->second;
		if (P->GameID == GameID)
			return P;
	};
	return NULL;
};

game_PlayerState *game_cl_GameState::GetPlayerByOrderID(u32 idx)
{
	PLAYERS_MAP_IT I = players.begin();
	std::advance(I, idx);
	game_PlayerState *ps = I->second;
	return ps;
}

ClientID game_cl_GameState::GetClientIDByOrderID(u32 idx)
{
	PLAYERS_MAP_IT I = players.begin();
	std::advance(I, idx);
	return I->first;
}

void game_cl_GameState::CommonMessageOut(LPCSTR msg)
{
	if (!HUD().GetUI())
		return;
	HUD().GetUI()->m_pMessagesWnd->AddLogMessage(msg);
}

float game_cl_GameState::shedule_Scale()
{
	return 1.0f;
}

void game_cl_GameState::shedule_Update(u32 dt)
{
	ISheduled::shedule_Update(dt);

	if (!m_game_ui_custom)
	{
		if (HUD().GetUI())
			m_game_ui_custom = HUD().GetUI()->UIGame();
	}
	//---------------------------------------
	switch (Phase())
	{
	case GAME_PHASE_INPROGRESS:
	{
		if (!IsGameTypeSingle())
			m_WeaponUsageStatistic->Update();
	}
	break;
	default:
	{
	}
	break;
	};
};

void game_cl_GameState::StartStopMenu(CUIDialogWnd *pDialog, bool bDoHideIndicators)
{
	HUD().GetUI()->StartStopMenu(pDialog, bDoHideIndicators);
}

void game_cl_GameState::sv_EventSend(NET_Packet &P)
{
	Level().Send(P, net_flags(TRUE, TRUE));
}

bool game_cl_GameState::IR_OnKeyboardPress(int dik)
{
	if (local_player && !local_player->IsSkip())
		return OnKeyboardPress(get_binded_action(dik));
	else
		return false;
}

bool game_cl_GameState::IR_OnKeyboardRelease(int dik)
{
	if (local_player && !local_player->IsSkip())
		return OnKeyboardRelease(get_binded_action(dik));
	else
		return false;
}

bool game_cl_GameState::IR_OnMouseMove(int dx, int dy)
{
	return false;
}

bool game_cl_GameState::IR_OnMouseWheel(int direction)
{
	return false;
}

void game_cl_GameState::u_EventSend(NET_Packet &P)
{
	Level().Send(P, net_flags(TRUE, TRUE));
}

void game_cl_GameState::OnSwitchPhase(u32 old_phase, u32 new_phase)
{
	switch (old_phase)
	{
	case GAME_PHASE_INPROGRESS:
	{
	}
	break;
	default:
	{
	}
	break;
	};

	switch (new_phase)
	{
	case GAME_PHASE_INPROGRESS:
	{
		m_WeaponUsageStatistic->Clear();
	}
	break;
	default:
	{
	}
	break;
	}
}

void game_cl_GameState::SendPickUpEvent(u16 ID_who, u16 ID_what)
{
	NET_Packet P;
	u_EventGen(P, GE_OWNERSHIP_TAKE, ID_who);
	P.w_u16(ID_what);
	u_EventSend(P);
};

void game_cl_GameState::set_type_name(LPCSTR s)
{
	m_game_type_name = s;
	if (OnClient())
	{
		strcpy(g_pGamePersistent->m_game_params.m_game_type, *m_game_type_name);
		g_pGamePersistent->OnGameStart();
	}
};
void game_cl_GameState::reset_ui()
{
	if (g_dedicated_server)
		return;

	if (!m_game_ui_custom)
		m_game_ui_custom = HUD().GetUI()->UIGame();

	m_game_ui_custom->reset_ui();

	HUD().GetUI()->UIMainIngameWnd->reset_ui();

	if (HUD().GetUI()->MainInputReceiver())
		HUD().GetUI()->StartStopMenu(HUD().GetUI()->MainInputReceiver(), true);
}

#include "ai_space.h"
#include "patrol_path_storage.h"

void game_cl_GameState::ReceivePatrolPaths(NET_Packet &P)
{
	ai().patrol_paths().NetworkImport(P);
}

void game_cl_GameState::ForEachClient(luabind::functor<void> functor)
{
	Level().Server->ForEachClientDo([&](IClient* client)
		{
			functor(static_cast<xrClientData*>(client), this);
		}
	);
}