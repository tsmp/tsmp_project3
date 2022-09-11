#include "StdAfx.h"
#include "game_cl_race.h"
#include "UIGameRace.h"
#include "clsid_game.h"
#include "actor.h"
#include "ui/UIInventoryWnd.h"
#include "string_table.h"
#include "LevelGameDef.h"
#include "Car.h"
#include "GameObject.h"

CStringTable g_St;

extern ENGINE_API bool g_dedicated_server;
extern u32 TimeBeforeRaceStart;
u32 GoMessageShowTime = 5000; // 5 sec

game_cl_Race::game_cl_Race() : m_game_ui(nullptr), m_WinnerId(static_cast<u16>(-1)), m_WinnerMessageSet(false)
{
	LoadSounds();
}

game_cl_Race::~game_cl_Race() {}

CUIGameCustom* game_cl_Race::createGameUI()
{
	inherited::createGameUI();
	CLASS_ID clsid = CLSID_GAME_UI_RACE;
	m_game_ui = smart_cast<CUIGameRace*>(NEW_INSTANCE(clsid));
	R_ASSERT(m_game_ui);
	m_game_ui->SetClGame(this);
	m_game_ui->Init();
	return m_game_ui;
}

void game_cl_Race::LoadSounds()
{
	LoadSndMessage("race_snd_messages", "go", ID_RACE_GO);

	if (m_pSndMessages.back().SoundID != ID_RACE_GO)
	{
		m_pSndMessages.push_back(SND_Message());
		m_pSndMessages.back().Load(ID_RACE_GO, 50, "characters_voice\\human_01\\stalker\\fight\\attack\\attack_4.ogg");
	}
}

void game_cl_Race::UpdateRaceStart()
{
	if (!m_game_ui)
		return;

	m_game_ui->ShowPlayersList(false);

	if (m_start_time + TimeBeforeRaceStart <= Level().timeServer())
		return;

	static u32 LastTimeRemains = 0;

	u32 timeBeforeStart = m_start_time + TimeBeforeRaceStart - Level().timeServer();
	u32 secRemains = timeBeforeStart / 1000;

	if (LastTimeRemains != secRemains)
	{
		string256 caption;

		if (secRemains >= 1)		
			sprintf_s(caption, "%s %u", *g_St.translate("mp_time2start"), secRemains);
		else
			sprintf_s(caption, "%s", *g_St.translate("mp_ready"));

		m_game_ui->SetCountdownCaption(caption);

		if (secRemains == 6)
		{
			if (CActor* act = Actor())
				if (CCar* car = smart_cast<CCar*>(act->Holder()))
					car->StartEngine();
		}

		if (secRemains > 0 && secRemains <= 5)
			PlaySndMessage(ID_COUNTDOWN_1 + secRemains - 1);
	}

	LastTimeRemains = secRemains;
}

void game_cl_Race::UpdateRaceInProgress()
{
	if (!m_game_ui)
		return;
	
	m_game_ui->ShowPlayersList(false);

	if (m_start_time + GoMessageShowTime > Level().timeServer())	
		m_game_ui->SetCountdownCaption(*g_St.translate("mp_go"));	
	else
		m_game_ui->SetCountdownCaption("");	
}

void game_cl_Race::UpdateRaceScores()
{
	if (!m_game_ui)
		return;

	m_game_ui->SetCountdownCaption("");

	if (!m_WinnerMessageSet && m_WinnerId != -1)
	{
		if (CGameObject* obj = smart_cast<CGameObject*>(Level().Objects.net_Find(m_WinnerId)))
		{
			string128 tmp;
			sprintf_s(tmp, *g_St.translate("mp_player_wins"), obj->cName().c_str());
			m_game_ui->SetRoundResultCaption(tmp);
			m_WinnerMessageSet = true;

			if(local_player->GameID == m_WinnerId)
				PlaySndMessage(ID_YOU_WON);
		}
	}
}

void game_cl_Race::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);

	if (g_dedicated_server)
		return;

	switch (Phase())
	{
	case GAME_PHASE_PENDING:
	{
		if (m_game_ui)
		{
			m_game_ui->ShowPlayersList(true);
			m_game_ui->SetCountdownCaption("");
			m_game_ui->SetRoundResultCaption("");
		}
	}
	break;

	case GAME_PHASE_PLAYER_SCORES:
		UpdateRaceScores();
		break;

	case GAME_PHASE_RACE_START:
		UpdateRaceStart();
		break;

	case GAME_PHASE_INPROGRESS:
		UpdateRaceInProgress();
		break;
	}
}

void game_cl_Race::LoadTeamBaseParticles()
{
	string_path fn_game;
	if (!FS.exist(fn_game, "$level$", "level.game"))
		return;

	IReader* F = FS.r_open(fn_game);

	// Load TeamBase particles
	if (IReader* O = F->open_chunk(RPOINT_CHUNK))
	{
		for (int id = 0; O->find_chunk(id); ++id)
		{
			RPoint R;
			u8 RP_team, RP_type, RP_GameType;

			O->r_fvector3(R.P);
			O->r_fvector3(R.A);

			RP_team = O->r_u8();
			VERIFY(RP_team >= 0 && RP_team < 4);

			RP_type = O->r_u8();
			RP_GameType = O->r_u8();
			O->r_u8();

			if (RP_GameType != rpgtGameAny && RP_GameType != rpgtGameDeathmatch)
				continue;

			if (RP_type != rptTeamBaseParticle)
				continue;

			string256 ParticleStr;
			sprintf_s(ParticleStr, "teambase_particle_%d", RP_team);

			if (pSettings->line_exist("artefacthunt_gamedata", ParticleStr))
			{
				Fmatrix transform;
				transform.identity();
				transform.setXYZ(R.A);
				transform.translate_over(R.P);
				CParticlesObject* pStaticParticles = CParticlesObject::Create(pSettings->r_string("artefacthunt_gamedata", ParticleStr), FALSE, false);
				pStaticParticles->UpdateParent(transform, zero_vel);
				pStaticParticles->Play();
				Level().m_StaticParticles.push_back(pStaticParticles);
			}
		}

		O->close();
	}

	FS.r_close(F);
}

void game_cl_Race::Init()
{
	LoadTeamBaseParticles();
}

bool game_cl_Race::OnKeyboardPress(int key)
{
	if (key == kSCORES)
	{
		m_game_ui->ShowPlayersList(true);
		return true;
	}

	return inherited::OnKeyboardPress(key);
}

bool game_cl_Race::OnKeyboardRelease(int key)
{
	if (key == kSCORES && Phase() != GAME_PHASE_PENDING)
		m_game_ui->ShowPlayersList(false);

	return inherited::OnKeyboardRelease(key);
}

void game_cl_Race::OnSwitchPhase(u32 oldPhase, u32 newPhase)
{
	m_WinnerMessageSet = false;
	inherited::OnSwitchPhase(oldPhase, newPhase);

	if (oldPhase == GAME_PHASE_RACE_START && newPhase == GAME_PHASE_INPROGRESS)
		PlaySndMessage(ID_RACE_GO);
}

void game_cl_Race::net_import_state(NET_Packet &P)
{
	inherited::net_import_state(P);

	if (m_phase == GAME_PHASE_PLAYER_SCORES)
		P.r_u16(m_WinnerId);
}
