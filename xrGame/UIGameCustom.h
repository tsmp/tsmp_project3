#pragma once

#include "script_export_space.h"
#include "object_interfaces.h"
#include "game_base_space.h"

class CUI;
class CTeamBaseZone;
class game_cl_GameState;
class CUIDialogWnd;
class CUICaption;
class CUIStatic;
class CUIWindow;
class CUIXml;
class CInventoryBox;
class CUITalkWnd;
class CInventoryOwner;
class CUICarBodyWnd;
class CUIPdaWnd;

struct SDrawStaticStruct : public IPureDestroyableObject
{
	CUIStatic *m_static;
	float m_endTime;
	shared_str m_name;

	SDrawStaticStruct();
	virtual void destroy();

	void Draw();
	void Update();

	CUIStatic *wnd() { return m_static; }
	
	bool IsActual();
	
	bool operator==(LPCSTR str)
	{
		return (m_name == str);
	}
};

using st_vec = xr_vector<SDrawStaticStruct>;

struct SGameTypeMaps
{
	shared_str m_game_type_name;
	EGameTypes m_game_type_id;
	xr_vector<shared_str> m_map_names;
};

struct SGameWeathers
{
	shared_str m_weather_name;
	shared_str m_start_time;
};

using GAME_WEATHERS = xr_vector<SGameWeathers>;

class CMapListHelper
{
	using TSTORAGE = xr_vector<SGameTypeMaps>;
	
	TSTORAGE m_storage;
	GAME_WEATHERS m_weathers;

	void Load();
	SGameTypeMaps *GetMapListInt(const shared_str &game_type);

public:
	const SGameTypeMaps &GetMapListFor(const shared_str &game_type);
	const SGameTypeMaps &GetMapListFor(const EGameTypes game_id);
	const GAME_WEATHERS &GetGameWeathers();
};

extern CMapListHelper gMapListHelper;

class CUIGameCustom : public DLL_Pure, public ISheduled
{
	typedef ISheduled inherited;

protected:

	st_vec m_custom_statics;
	u32 uFlags;
	CUICaption* m_pgameCaptions;
	CUIXml* m_msgs_xml;
	CUICarBodyWnd* m_pUICarBodyMenu;

	void SetFlag(u32 mask, BOOL flag)
	{
		if (flag)
			uFlags |= mask;
		else
			uFlags &= ~mask;
	}

	void InvertFlag(u32 mask)
	{
		if (uFlags & mask)
			uFlags &= ~mask;
		else
			uFlags |= mask;
	}

	BOOL GetFlag(u32 mask) { return uFlags & mask; }
	CUICaption *GameCaptions() { return m_pgameCaptions; }

public:
	virtual void SetClGame(game_cl_GameState *g){};

	virtual float shedule_Scale();
	virtual void shedule_Update(u32 dt);

	CUIGameCustom();
	virtual ~CUIGameCustom();

	virtual void Init(){};
	virtual void Render();
	virtual void OnFrame();
	virtual void reset_ui();

	virtual bool IR_OnKeyboardPress(int dik);
	virtual bool IR_OnKeyboardRelease(int dik);
	virtual bool IR_OnMouseMove(int dx, int dy);
	virtual bool IR_OnMouseWheel(int direction);

	void AddDialogToRender(CUIWindow *pDialog);
	void RemoveDialogToRender(CUIWindow *pDialog);

	CUIDialogWnd *MainInputReceiver();
	virtual void ReInitShownUI() = 0;
	virtual void HideShownDialogs(){};

	virtual void StartCarBody(CInventoryOwner* pOurInv, CInventoryOwner* pOthers);
	virtual void StartCarBody(CInventoryOwner* pOurInv, CInventoryBox* pBox);
	virtual void StartTalk();

	void AddCustomMessage(LPCSTR id, float x, float y, float font_size, CGameFont *pFont, u16 alignment, u32 color);
	void AddCustomMessage(LPCSTR id, float x, float y, float font_size, CGameFont *pFont, u16 alignment, u32 color, float flicker);
	void CustomMessageOut(LPCSTR id, LPCSTR msg, u32 color);
	void RemoveCustomMessage(LPCSTR id);

	SDrawStaticStruct *AddCustomStatic(LPCSTR id, bool bSingleInstance);
	SDrawStaticStruct *GetCustomStatic(LPCSTR id);
	void RemoveCustomStatic(LPCSTR id);

	virtual shared_str shedule_Name() const { return shared_str("CUIGameCustom"); };
	virtual bool shedule_Needed() { return true; };

	DECLARE_SCRIPT_REGISTER_FUNCTION

public: 
		CUITalkWnd* TalkMenu;
		CUIPdaWnd* PdaMenu;
};

add_to_type_list(CUIGameCustom)
#undef script_type_list
#define script_type_list save_type_list(CUIGameCustom)
