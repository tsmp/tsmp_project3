#pragma once
#include "UIGameCustom.h"

class game_cl_GameState;
class game_cl_Race;

class CUIGameRace : public CUIGameCustom
{
public:

	CUIGameRace();
	virtual ~CUIGameRace();

	virtual void SetClGame(game_cl_GameState* g) override;
	virtual void Init() override;
	virtual void ReInitShownUI() override {};

	void ShowPlayersList(bool bShow);
	void SetCountdownCaption(const char* str);

protected:

	shared_str m_CountdownCaption;
	game_cl_Race* m_game;
	CUIWindow* m_pPlayerLists;
	using inherited = CUIGameCustom;
};
