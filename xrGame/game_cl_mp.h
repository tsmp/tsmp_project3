#pragma once

#include "game_cl_base.h"
#include "script_export_space.h"
#include "game_cl_mp_snd_messages.h"
#include "../../xrSound/Sound.h"
#include "ui/UISpeechMenu.h"
#include "Spectator.h"

class CUISpeechMenu;
class CUIMessageBoxEx;
class CUIGameBaseMP;
class CVoiceChat;

struct SND_Message
{
	ref_sound pSound;
	u32 priority;
	u32 SoundID;
	u32 LastStarted;
	bool operator==(u32 ID) { return SoundID == ID; }
	void Load(u32 ID, u32 prior, LPCSTR name)
	{
		SoundID = ID;
		priority = prior;
		pSound.create(name, st_Effect, sg_SourceType);
		LastStarted = 0;
	}
	~SND_Message()
	{
		SoundID = 0;
		priority = 0;
		pSound.destroy();
	}
};

struct cl_TeamStruct
{
	shared_str caSection; // имя секции комманды
	//-----------------------------------
	ui_shader IndicatorShader;
	ui_shader InvincibleShader;

	Fvector IndicatorPos;
	float Indicator_r1;
	float Indicator_r2;
};

DEF_DEQUE(CL_TEAM_DATA_LIST, cl_TeamStruct);

struct cl_Message_Sound
{
	ref_sound mSound_Voice;
	ref_sound mSound_Radio;
};

DEF_VECTOR(TEAMSOUND, cl_Message_Sound);

struct cl_Menu_Message
{
	shared_str pMessage;
	DEF_VECTOR(SOUND_VARIANTS, TEAMSOUND);
	SOUND_VARIANTS aVariants;
};

struct cl_MessageMenu
{
	CUISpeechMenu *m_pSpeechMenu;
	DEF_VECTOR(MENUMESSAGES, cl_Menu_Message);
	MENUMESSAGES m_aMessages;

	bool operator==(CUISpeechMenu *pMenu) { return pMenu == m_pSpeechMenu; }
};

struct Bonus_Struct
{
	shared_str BonusName;
	shared_str BonusTypeName;
	shared_str MoneyStr;
	int Money;
	ui_shader IconShader;
	xr_vector<Frect> IconRects;
	Bonus_Struct()
	{
		BonusTypeName = "";
		BonusName = "";
		MoneyStr = "";
		Money = 0;
		IconRects.clear();
	}
	~Bonus_Struct()
	{
		IconRects.clear();
	}

	bool operator==(LPCSTR TypeName) { return !xr_strcmp(BonusTypeName.c_str(), TypeName); }
};

class CUIVotingCategory;
class CUIVote;
class CUIMessageBoxEx;

#include "screenshot_manager.h"
#include "screenshot_server.h"

class game_cl_mp : public game_cl_GameState
{
	typedef game_cl_GameState inherited;

protected:
	CL_TEAM_DATA_LIST TeamList;

	DEF_VECTOR(SNDMESSAGES, SND_Message);
	SNDMESSAGES m_pSndMessages;
	bool m_bJustRestarted;
	DEF_VECTOR(SNDMESSAGESINPLAY, SND_Message *);
	SNDMESSAGESINPLAY m_pSndMessagesInPlay;

	DEF_VECTOR(BONUSES, Bonus_Struct);
	BONUSES m_pBonusList;

	bool m_bVotingActive;
	CUIVotingCategory *m_pVoteStartWindow;
	CUIVote *m_pVoteRespondWindow;
	CUIMessageBoxEx *m_pMessageBox;
	BOOL m_bSpectatorSelected;
	u32 m_dwVoteEndTime;

	virtual void LoadTeamData(const shared_str &TeamName);
	virtual void ChatSayTeam(const shared_str &phrase);
	virtual void ChatSayAll(const shared_str &phrase);
	virtual void OnChatMessage(NET_Packet *P);
	virtual void OnWarnMessage(NET_Packet *P);
	virtual void OnRadminMessage(u16 type, NET_Packet *P);
	virtual void OnVoiceMessage(NET_Packet* P) override;

	virtual void UpdateMapLocations(){};

	ui_shader m_EquipmentIconsShader;
	ui_shader m_RankIconsShader;

	virtual const ui_shader& GetEquipmentIconsShader();
	virtual const ui_shader& GetKillEventIconsShader();
	virtual const ui_shader& GetRadiationIconsShader();
	virtual const ui_shader& GetBloodLossIconsShader();
	virtual const ui_shader& GetRankIconsShader();

	virtual void OnPlayerKilled(NET_Packet &P);

	virtual bool NeedToSendReady_Actor(int key, game_PlayerState *ps);
	virtual bool NeedToSendReady_Spectator(int key, game_PlayerState *ps);

	virtual void LoadSndMessage(LPCSTR caSection, LPCSTR caLine, u32 ID);
	virtual void LoadSndMessages();
	virtual void PlaySndMessage(u32 ID);
	virtual void UpdateSndMessages();

	u8 m_u8SpectatorModes;
	bool m_bSpectator_FreeFly;
	bool m_bSpectator_FirstEye;
	bool m_bSpectator_LookAt;
	bool m_bSpectator_FreeLook;
	bool m_bSpectator_TeamCamera;

	screenshot_manager ss_manager;

	virtual void LoadBonuses();

	u8* buffer_for_compress;
	u32	buffer_for_compress_size;
	CMemoryWriter upload_memory_writer;
	void reinit_compress_buffer(u32 need_size);
	void deinit_compress_buffer();

public:
	game_cl_mp();
	virtual ~game_cl_mp();

	void xr_stdcall OnBuySpawn(CUIWindow *pWnd, void *p);
	virtual void TranslateGameMessage(u32 msg, NET_Packet &P);
	virtual void CommonMessageOut(LPCSTR msg);

	virtual bool OnKeyboardPress(int key);
	virtual bool OnKeyboardRelease(int key);

	virtual bool CanBeReady();
	virtual CUIGameCustom *createGameUI();
	virtual void shedule_Update(u32 dt);

	//// VOTING
	virtual bool IsVotingActive() { return m_bVotingActive; };
	virtual void SetVotingActive(bool Active) { m_bVotingActive = Active; };
	virtual void SendStartVoteMessage(LPCSTR args);
	virtual void SendVoteYesMessage();
	virtual void SendVoteNoMessage();
	void VotingBegin();
	void Vote();
	void OnCantVoteMsg(LPCSTR Text);
	virtual void OnVoteStart(NET_Packet &P);
	virtual void OnVoteStop(NET_Packet &P);
	virtual void OnVoteEnd(NET_Packet &P);
	virtual void OnPlayerChangeName(NET_Packet &P);
	virtual void OnPlayerVoted(game_PlayerState *ps);
	virtual void OnSpectatorSelect();
	virtual void OnSkinMenuBack(){};
	virtual void OnTeamMenuBack(){};
	virtual void OnTeamMenu_Cancel(){};
	virtual void OnMapInfoAccept(){};
	virtual void OnSkinMenu_Ok(){};
	virtual void OnSkinMenu_Cancel(){};
	virtual void OnBuySpawnMenu_Ok(){};
	virtual void OnSellItemsFromRuck(){};

	virtual void OnGameMenuRespond(NET_Packet &P);
	virtual void OnGameMenuRespond_Spectator(NET_Packet &P){};
	virtual void OnGameMenuRespond_ChangeTeam(NET_Packet &P){};
	virtual void OnGameMenuRespond_ChangeSkin(NET_Packet &P){};
	virtual void OnGameRoundStarted();

	virtual void OnSwitchPhase(u32 old_phase, u32 new_phase);
	virtual void net_import_update(NET_Packet &P);
	virtual void net_import_state(NET_Packet &P);
	virtual void OnRankChanged(u8 OldRank);
	virtual void OnTeamChanged(){};
	virtual void OnMoneyChanged(){};
	virtual void OnEventMoneyChanged(NET_Packet &P);

	virtual void OnSwitchPhase_InProgress();

	virtual u8 GetTeamCount() { return 0; };
	virtual s16 ModifyTeam(s16 Team) { return Team; };
	virtual CUIGameBaseMP* GetMpGameUI() { return nullptr; }

	void draw_downloads(bool draw);
	virtual bool Is_Spectator_TeamCamera_Allowed() { return m_bSpectator_TeamCamera; };
	virtual bool Is_Spectator_Camera_Allowed(CSpectator::EActorCameras Camera);

	void decompress_and_save_screenshot(LPCSTR file_name, u8* data, u32 data_size, u32 file_size);

	void __stdcall SendCollectedData(u8 const* buffer, u32 buffer_size, u32 uncompressed_size);
	void PrepareToReceiveFile(ClientID const& from_client, shared_str const& client_session_id, clientdata_event_t response_event);
	
	struct fr_callback_binder
	{
		file_transfer::filereceiver_node* m_frnode;
		shared_str m_file_name;
		clientdata_event_t m_response_type;
		bool m_active;
		u32	m_downloaded_size;
		u32	m_max_size;
		game_cl_mp* m_owner;
		CMemoryWriter m_writer;
		fr_callback_binder() : m_frnode(nullptr), m_active(false) {};
		void __stdcall receiving_file_callback(file_transfer::receiving_status_t status, u32 bytes_received, u32 data_size);
		//void __stdcall		receiving_serverinfo_callback(file_transfer::receiving_status_t status, u32 bytes_received, u32 data_size);
	};

	void draw_all_active_binder_states();
	fr_callback_binder m_client_receiver_cbs[/*MAX_PLAYERS_COUNT*/ 32];
	fr_callback_binder* get_receiver_cb_binder();
	void __stdcall sending_screenshot_callback(file_transfer::sending_status_t status, u32 bytes_sent, u32 data_size);

private:
	CVoiceChat* m_pVoiceChat = nullptr;

#include "game_cl_mp_messages_menu.h"

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(game_cl_mp)
#undef script_type_list
#define script_type_list save_type_list(game_cl_mp)
