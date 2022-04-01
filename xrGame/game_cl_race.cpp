#include "StdAfx.h"
#include "game_cl_race.h"
#include "UIGameDM.h"
#include "clsid_game.h"
#include "actor.h"
#include "ui/UIInventoryWnd.h"
#include "string_table.h"
#include "LevelGameDef.h"

game_cl_Race::game_cl_Race() : m_game_ui(nullptr) {}
game_cl_Race::~game_cl_Race() {}

CUIGameCustom* game_cl_Race::createGameUI()
{
	inherited::createGameUI();
	CLASS_ID clsid = CLSID_GAME_UI_DEATHMATCH;
	m_game_ui = smart_cast<CUIGameDM*>(NEW_INSTANCE(clsid));
	R_ASSERT(m_game_ui);
	m_game_ui->SetClGame(this);
	m_game_ui->Init();
	return m_game_ui;
}

void game_cl_Race::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
}

void game_cl_Race::Init()
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
	if (key == kSCORES)
		m_game_ui->ShowPlayersList(false);

	return inherited::OnKeyboardRelease(key);
}
