#pragma once
#include "game_cl_mp.h"

class CUIGameRace;
class CStringTable;

class game_cl_Race : public game_cl_mp
{
private:
	typedef game_cl_mp inherited;
	CUIGameRace* m_game_ui;
	u16 m_WinnerId;
	bool m_WinnerMessageSet;

	void LoadTeamBaseParticles();
	void UpdateRaceStart();
	void UpdateRaceInProgress();
	void UpdateRaceScores();
	void LoadSounds();

public:

	game_cl_Race();
	virtual ~game_cl_Race();

	virtual CUIGameCustom* createGameUI() override;
	virtual void shedule_Update(u32 dt) override;
	virtual void Init() override;

	virtual bool OnKeyboardPress(int key) override;
	virtual bool OnKeyboardRelease(int key) override;
	virtual void OnSwitchPhase(u32 old_phase, u32 new_phase) override;

	virtual void net_import_state(NET_Packet &P) override;
};
