#pragma once
#include "game_cl_mp.h"
#include "saved_game_wrapper.h"

class CUIGameCustom;

class game_cl_Coop : public game_cl_mp
{
private:
	typedef game_cl_mp inherited;
	CUIGameCustom* m_game_ui;

public:
	game_cl_Coop();
	virtual ~game_cl_Coop();

	virtual CUIGameCustom* createGameUI();
	virtual void shedule_Update(u32 dt);
	virtual void OnTasksSync(NET_Packet* P) override;
	virtual void OnPortionsSync(NET_Packet* P) override;
	virtual void OnSavedGamesSync(NET_Packet* P) override;
	virtual void OnGameSavedNotify(NET_Packet* P) override;

	xr_vector<CSavedGameWrapper> m_SavedGames;
	bool m_ActualSavedGames{ false };
};
