#pragma once

#include "UIGameBaseMP.h"
#include "ui/KillMessageStruct.h"

#include "game_cl_mp.h"

class CUIDMPlayerList;
class CUIDMStatisticWnd;
class CUISkinSelectorWnd;
class game_cl_Deathmatch;
class CUIMoneyIndicator;
class CUIRankIndicator;
class UIVoteStatusWnd;

class CUIInventoryWnd;
class CUIMapDesc;
class CInventoryBox;

class CUIGameDM : public CUIGameBaseMP
{
private:
	//game_cl_Deathmatch *m_game;
	game_cl_mp *m_game;
	using inherited = CUIGameBaseMP;
	bool m_IsHardmatch;

public:
	CUIInventoryWnd *m_pInventoryMenu;
	CUIMapDesc *m_pMapDesc;

protected:
	enum
	{
		flShowFragList = (1 << 1),
		fl_force_dword = u32(-1)
	};

	CUIWindow *m_pFragLists;
	CUIWindow *m_pPlayerLists;
	CUIWindow *m_pStatisticWnds;

	shared_str m_time_caption;
	shared_str m_spectrmode_caption;

	shared_str m_spectator_caption;
	shared_str m_pressjump_caption;
	shared_str m_pressbuy_caption;

	shared_str m_round_result_caption;
	shared_str m_force_respawn_time_caption;
	shared_str m_demo_play_caption;
	shared_str m_warm_up_caption;
	
	CUIMoneyIndicator* m_pMoneyIndicator;
	CUIRankIndicator* m_pRankIndicator;
	CUIStatic* m_pFragLimitIndicator;

	virtual void ClearLists();

public:
	CUIGameDM();
	virtual ~CUIGameDM();

	virtual void SetClGame(game_cl_GameState *g);
	virtual void Init();
	virtual void Render();
	virtual void OnFrame();

	void SetRank(s16 team, u8 rank);

	void ChangeTotalMoneyIndicator(LPCSTR newMoneyString);
	void DisplayMoneyChange(LPCSTR deltaMoney);
	void DisplayMoneyBonus(KillMessageStruct bonus);
	virtual void SetFraglimit(int local_frags, int fraglimit);

	void SetTimeMsgCaption(LPCSTR str);
	void SetSpectrModeMsgCaption(LPCSTR str);

	void SetSpectatorMsgCaption(LPCSTR str);
	void SetPressJumpMsgCaption(LPCSTR str);
	void SetPressBuyMsgCaption(LPCSTR str);
	void SetRoundResultCaption(LPCSTR str);
	void SetForceRespawnTimeCaption(LPCSTR str);
	void SetDemoPlayCaption(LPCSTR str);
	void SetWarmUpCaption(LPCSTR str);

	virtual bool IR_OnKeyboardPress(int dik);
	virtual bool IR_OnKeyboardRelease(int dik);

	virtual void HideShownDialogs() override;

	void ShowFragList(bool bShow);
	void ShowPlayersList(bool bShow);
	void ShowStatistic(bool bShow);
	virtual void ReInitShownUI();
	virtual void reset_ui();
};
