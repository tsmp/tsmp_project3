#pragma once
#include "game_cl_mp.h"

class CUIGameFP;

class game_cl_Freeplay : public game_cl_mp
{
private:
	typedef game_cl_mp inherited;
	CUIGameFP* m_game_ui;
	 
public:

	game_cl_Freeplay();
	virtual ~game_cl_Freeplay();

	virtual CUIGameCustom* createGameUI();
	virtual void shedule_Update(u32 dt);

	virtual bool OnKeyboardPress(int key);
	virtual bool OnKeyboardRelease(int key);
};
