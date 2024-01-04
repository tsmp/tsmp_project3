#pragma once
#include "UIGameBaseMP.h"

class game_cl_GameState;
class game_cl_Freeplay;
class CUIInventoryWnd;

class CUIGameFP : public CUIGameBaseMP
{
public:

	CUIGameFP();
	virtual ~CUIGameFP();

	virtual void SetClGame(game_cl_GameState* g) override;
	virtual void Init() override;
	virtual void Render() override;
	virtual void OnFrame() override;
	virtual void ReInitShownUI() override;

	virtual void reset_ui() override;
	virtual void HideShownDialogs() override;

	void SetPressJumpMsgCaption(LPCSTR text);

	void ShowPlayersList(bool bShow);
	CUIInventoryWnd* m_pInventoryMenu;

	virtual bool IR_OnKeyboardPress(int dik);

protected:
	
	shared_str m_pressjump_caption;
	game_cl_Freeplay *m_game;
	CUIWindow *m_pPlayerLists;
	using inherited = CUIGameBaseMP;
};
