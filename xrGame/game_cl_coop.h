#pragma once
#include "game_cl_mp.h"

class CUIGameDM;

class game_cl_Coop : public game_cl_mp
{
private:
	typedef game_cl_mp inherited;
	CUIGameDM* m_game_ui;

public:

	game_cl_Coop();
	virtual ~game_cl_Coop();

	virtual CUIGameCustom* createGameUI();
	virtual void shedule_Update(u32 dt);

	virtual bool OnKeyboardPress(int key);
	virtual bool OnKeyboardRelease(int key);
};
