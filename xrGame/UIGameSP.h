#pragma once
#include "uigamecustom.h"
#include "ui/UIDialogWnd.h"
#include "../../xrNetwork/net_utils.h"
#include "game_graph_space.h"

class CUIInventoryWnd;
class CUITradeWnd;
class CInventory;

class game_cl_GameState;
class CChangeLevelWnd;
class CUIMessageBox;
class CInventoryBox;
class CInventoryOwner;

class CUIGameSP : public CUIGameCustom
{
private:
	game_cl_GameState*m_game;
	typedef CUIGameCustom inherited;

public:
	CUIGameSP();
	virtual ~CUIGameSP();

	virtual void reset_ui();
	virtual void shedule_Update(u32 dt);
	virtual void SetClGame(game_cl_GameState *g);
	virtual bool IR_OnKeyboardPress(int dik);
	virtual bool IR_OnKeyboardRelease(int dik);

	virtual void ReInitShownUI();
	void ChangeLevel(GameGraph::_GRAPH_ID game_vert_id, u32 level_vert_id, Fvector pos, Fvector ang, Fvector pos2, Fvector ang2, bool b);

	virtual void HideShownDialogs();

	bool IsAnyWndActive();

	CUIInventoryWnd *InventoryMenu;
	CChangeLevelWnd *UIChangeLevelWnd;
};

class CChangeLevelWnd : public CUIDialogWnd
{
	CUIMessageBox *m_messageBox;
	typedef CUIDialogWnd inherited;
	void OnCancel();
	void OnOk();

public:
	GameGraph::_GRAPH_ID m_game_vertex_id;
	u32 m_level_vertex_id;
	Fvector m_position;
	Fvector m_angles;
	Fvector m_position_cancel;
	Fvector m_angles_cancel;
	bool m_b_position_cancel;

	CChangeLevelWnd();
	virtual ~CChangeLevelWnd(){};
	virtual void SendMessage(CUIWindow *pWnd, s16 msg, void *pData);
	virtual bool WorkInPause() const { return true; }
	virtual void Show();
	virtual void Hide();
	virtual bool OnKeyboard(int dik, EUIMessages keyboard_action);
};
