#pragma once
#include "game_cl_mp.h"

class CUIGameDM;

class game_cl_Race : public game_cl_mp
{
private:
	typedef game_cl_mp inherited;
	CUIGameDM* m_game_ui;

	void LoadTeamBaseParticles();

public:

	game_cl_Race();
	virtual ~game_cl_Race();

	virtual CUIGameCustom* createGameUI();
	virtual void shedule_Update(u32 dt);
	virtual void Init() override;

	virtual bool OnKeyboardPress(int key);
	virtual bool OnKeyboardRelease(int key);
};
