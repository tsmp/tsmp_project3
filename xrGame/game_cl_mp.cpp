#include "stdafx.h"
#include "game_cl_mp.h"
#include "xr_level_controller.h"
#include "xrMessages.h"
#include "GameObject.h"
#include "Actor.h"
#include "level.h"
#include "hudmanager.h"
#include "ui/UIChatWnd.h"
#include "ui/UIGameLog.h"
#include "../Include/xrRender/UIShader.h"
#include "clsid_game.h"
#include <dinput.h>
#include "UIGameCustom.h"
#include "ui/UIInventoryUtilities.h"
#include "ui/UIMessagesWindow.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIMessageBoxEx.h"
#include "CustomZone.h"
#include "game_base_kill_type.h"
#include "game_base_menu_events.h"
#include "UIGameDM.h"
#include "UIGameBaseMP.h"
#include "ui/UITextureMaster.h"
#include "ui/UIVotingCategory.h"
#include "ui/UIVote.h"
#include "ui/UIMessageBoxEx.h"
#include "string_table.h"
#include "IGame_Persistent.h"
#include "MainMenu.h"
#include "screenshot_server.h"
#include "screenshot_manager.h"
#include "VoiceChat.h"

#define EQUIPMENT_ICONS "ui\\ui_mp_icon_kill"
#define KILLEVENT_ICONS "ui\\ui_hud_mp_icon_death"
#define RADIATION_ICONS "ui\\ui_mn_radiations_hard"
#define BLOODLOSS_ICONS "ui\\ui_mn_wounds_hard"
#define RANK_ICONS "ui\\ui_mp_icon_rank"

#define KILLEVENT_GRID_WIDTH 64
#define KILLEVENT_GRID_HEIGHT 64

BOOL g_draw_downloads = FALSE;

#include "game_cl_mp_snd_messages.h"
#include <PPMD/ppmd_compressor.h>

game_cl_mp::game_cl_mp()
{
	m_bVotingActive = false;
	m_pVoteStartWindow = NULL;
	m_pVoteRespondWindow = NULL;
	m_pMessageBox = NULL;

	m_pSndMessages.clear();
	LoadSndMessages();
	m_bJustRestarted = true;
	m_pSndMessagesInPlay.clear();
	m_aMessageMenus.clear();

	m_bSpectatorSelected = FALSE;
	//-------------------------------------
	m_u8SpectatorModes = 0xff;
	m_bSpectator_FreeFly = true;
	m_bSpectator_FirstEye = true;
	m_bSpectator_LookAt = true;
	m_bSpectator_FreeLook = true;
	m_bSpectator_TeamCamera = true;
	//-------------------------------------
	LoadBonuses();

	buffer_for_compress = nullptr;
	buffer_for_compress_size = 0;

	if (!g_dedicated_server)
		m_pVoiceChat = xr_new<CVoiceChat>();
	else
		m_pVoiceChat = nullptr;
};

game_cl_mp::~game_cl_mp()
{
	TeamList.clear();

	m_pSndMessagesInPlay.clear_and_free();
	m_pSndMessages.clear_and_free();
	deinit_compress_buffer();

	//	xr_delete(m_pSpeechMenu);
	DestroyMessagesMenus();

	//	xr_delete(pBuySpawnMsgBox);

	m_pBonusList.clear();

	xr_delete(m_pVoteRespondWindow);
	xr_delete(m_pVoteStartWindow);
	xr_delete(m_pMessageBox);

	xr_delete(m_pVoiceChat);
}

CUIGameCustom *game_cl_mp::createGameUI()
{
	//	m_pSpeechMenu = xr_new<CUISpeechMenu>("test_speech_section");
	HUD().GetUI()->m_pMessagesWnd->SetChatOwner(this);

	return NULL;
};

bool game_cl_mp::CanBeReady()
{
	return true;
}

bool game_cl_mp::NeedToSendReady_Actor(int key, game_PlayerState *ps)
{
	return ((GAME_PHASE_PENDING == Phase()) ||
			true == ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)) &&
		   (kWPN_FIRE == key);
}

bool game_cl_mp::NeedToSendReady_Spectator(int key, game_PlayerState *ps)
{
	return (GAME_PHASE_PENDING == Phase() && kWPN_FIRE == key) ||
		   (kJUMP == key &&
			GAME_PHASE_INPROGRESS == Phase() &&
			CanBeReady() &&
			ps->DeathTime > 1000);
}

bool game_cl_mp::OnKeyboardPress(int key)
{
	CStringTable st;
	if (kJUMP == key || kWPN_FIRE == key)
	{
		bool b_need_to_send_ready = false;

		CObject *curr = Level().CurrentControlEntity();
		if (!curr)
			return (false);

		bool is_actor = curr->CLS_ID == CLSID_OBJECT_ACTOR;
		bool is_spectator = curr->CLS_ID == CLSID_SPECTATOR;

		game_PlayerState *ps = local_player;

		if (is_actor)
		{
			b_need_to_send_ready = NeedToSendReady_Actor(key, ps);
		};
		if (is_spectator)
		{
			b_need_to_send_ready = NeedToSendReady_Spectator(key, ps);
		};
		if (b_need_to_send_ready)
		{
			CGameObject *GO = smart_cast<CGameObject *>(curr);
			NET_Packet P;
			GO->u_EventGen(P, GE_GAME_EVENT, GO->ID());
			P.w_u16(GAME_EVENT_PLAYER_READY);
			GO->u_EventSend(P);
			return true;
		}
		else
			return false;
	};

	if (key == kVoiceChat)
	{
		if (local_player && !local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
		{
			if (!m_pVoiceChat->IsStarted())
				m_pVoiceChat->Start();
		}

		return true;
	}

	if ((Phase() != GAME_PHASE_INPROGRESS) && (kQUIT != key) && (kCONSOLE != key))
		return true;

	if (Phase() == GAME_PHASE_INPROGRESS)
	{
		switch (key)
		{
		case kCHAT:
		case kCHAT_TEAM:
		{
			shared_str prefix;

			CUIChatWnd *pChatWnd = HUD().GetUI()->m_pMessagesWnd->GetChatWnd();

			if (kCHAT_TEAM == key)
			{
				prefix.sprintf("%s> ", *st.translate("st_mp_say_to_team"));

				pChatWnd->TeamChat();
			}
			else
			{
				prefix.sprintf("%s> ", *st.translate("st_mp_say_to_all"));
				pChatWnd->AllChat();
			}

			pChatWnd->SetEditBoxPrefix(prefix);

			StartStopMenu(pChatWnd, false);
			if (!pChatWnd->IsShown() && xr_strlen(pChatWnd->UIEditBox.GetText()) > 0)
			{
				shared_str phrase = pChatWnd->UIEditBox.GetText();
				//				pChatWnd->Say(phrase);
				(kCHAT == key) ? ChatSayAll(phrase) : ChatSayTeam(phrase);
				pChatWnd->UIEditBox.SetText("");
			}
			return false;
		}
		break;
		case kVOTE_BEGIN:
		{
			if (IsVotingEnabled() && !IsVotingActive())
				VotingBegin();
			else
			{
				if (!IsVotingEnabled())
					OnCantVoteMsg(*st.translate("st_mp_disabled_voting"));
				else
					OnCantVoteMsg(*st.translate("st_mp_only_one_voting"));
			};
		}
		break;
		case kVOTE:
		{
			if (IsVotingEnabled() && IsVotingActive())
				Vote();
			else
			{
				if (!IsVotingEnabled())
					OnCantVoteMsg(*st.translate("st_mp_disabled_voting"));
				else
					OnCantVoteMsg(*st.translate("st_mp_no_current_voting"));
			}
		}
		break;
		case kVOTEYES:
		{
			if (IsVotingEnabled() && IsVotingActive())
				SendVoteYesMessage();
		}
		break;
		case kVOTENO:
		{
			if (IsVotingEnabled() && IsVotingActive())
				SendVoteNoMessage();
		}
		break;
		case kSPEECH_MENU_0:
		case kSPEECH_MENU_1:
		case kSPEECH_MENU_2:
		case kSPEECH_MENU_3:
		case kSPEECH_MENU_4:
		case kSPEECH_MENU_5:
		case kSPEECH_MENU_6:
		case kSPEECH_MENU_7:
		case kSPEECH_MENU_8:
		case kSPEECH_MENU_9:
		{
			if (!local_player || local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
				break;

			u32 MenuID = key - kSPEECH_MENU_0;
			if (MenuID >= m_aMessageMenus.size())
				break;
			cl_MessageMenu *pCurMenu = &(m_aMessageMenus[MenuID]);
			HideMessageMenus();
			StartStopMenu(pCurMenu->m_pSpeechMenu, FALSE);
			return true;
		}
		break;
		}
	}

	return inherited::OnKeyboardPress(key);
}

void game_cl_mp::reinit_compress_buffer(u32 need_size)
{
	if (buffer_for_compress && (need_size <= buffer_for_compress_size))
		return;

	Msg("* reiniting compression buffer.");
	buffer_for_compress_size = need_size * 2;
	void* new_buffer = xr_realloc(buffer_for_compress, buffer_for_compress_size);
	buffer_for_compress = static_cast<u8*>(new_buffer);
}

void game_cl_mp::deinit_compress_buffer()
{
	xr_free(buffer_for_compress);
}

void game_cl_mp::decompress_and_save_screenshot(LPCSTR file_name, u8* data, u32 data_size, u32 file_size)
{
	if (!file_size)
	{
		Msg("! ERROR: file size to save is 0...");
		return;
	}

	reinit_compress_buffer(file_size);

	u32 original_size = ppmd_decompress(
		buffer_for_compress,
		buffer_for_compress_size,
		data,
		data_size
	);

	if (original_size != file_size)
	{
		Msg("! WARNING: original and downloaded file size are different !");
	}
	string_path screen_shot_path;
	FS.update_path(screen_shot_path, "$screenshots$", file_name);
	strcat(screen_shot_path, ".jpg");

	IWriter* ftosave = FS.w_open(screen_shot_path);
	if (!ftosave)
	{
		Msg("! ERROR: failed to create file [%s]", file_name);
		return;
	}
	ftosave->w(buffer_for_compress, file_size);
	FS.w_close(ftosave);
}

void game_cl_mp::draw_all_active_binder_states()
{
	//drawing download states ..
	CGameFont* F = UI()->Font()->pFontDI;
	F->SetHeightI(0.015f);
	F->OutSetI(0.1f, 0.2f);
	F->SetColor(D3DCOLOR_XRGB(0, 255, 0));

	for (u32 i = 0; i < /*MAX_PLAYERS_COUNT*/32; ++i)
	{
		if (m_client_receiver_cbs[i].m_active)
		{
			fr_callback_binder& tmp_br = m_client_receiver_cbs[i];
			F->OutNext("%s : %02u %% ", tmp_br.m_file_name.c_str(),
				int(
					(float(tmp_br.m_downloaded_size) / tmp_br.m_max_size) * 100
					)
			);
		}
	}
	F->SetColor(D3DCOLOR_XRGB(255, 0, 0));
	/*for (cheaters_collection_t::iterator i = m_detected_cheaters.begin(),
		ie = m_detected_cheaters.end(); i != ie; ++i)
	{
		F->OutNext("%s : cheater suspect ...",
			i->m_file_name.c_str());
	}

	m_detected_cheaters.erase(
		std::remove_if(
			m_detected_cheaters.begin(),
			m_detected_cheaters.end(),
			old_detected_cheater()
		),
		m_detected_cheaters.end()
	);*/
}

void __stdcall game_cl_mp::sending_screenshot_callback(file_transfer::sending_status_t status, u32 bytes_sent, u32 data_size)
{
	switch (status)
	{
	case file_transfer::sending_data:
	{
#ifdef DEBUG
		Msg("* screenshot: %d of %d bytes sent ...", bytes_sent, data_size);
#endif
	}break;
	case file_transfer::sending_aborted_by_user:
	{
		Msg("* screenshot: sending aborted by user...");
	}break;
	case file_transfer::sending_rejected_by_peer:
	{
		Msg("* screenshot: sending rejected by peer ...");
	}break;
	case file_transfer::sending_complete:
	{
#ifdef DEBUG
		Msg("* screenshot: sending complete successfully !");
#endif
	}break;
	};
}


game_cl_mp::fr_callback_binder* game_cl_mp::get_receiver_cb_binder()
{
	for (u32 i = 0; i < 32/*MAX_PLAYERS_COUNT*/; ++i)
	{
		if (!m_client_receiver_cbs[i].m_active)
		{
			return &m_client_receiver_cbs[i];
		}
	}
	return NULL;
}

void game_cl_mp::PrepareToReceiveFile(ClientID const& from_client, shared_str const& client_session_id, clientdata_event_t response_event)
{
	string_path screen_shot_fn;
	
	string128 dest_file_name;
	strcpy(dest_file_name, make_file_name(client_session_id.c_str(), screen_shot_fn));
	
	SYSTEMTIME date_time;
	GetLocalTime(&date_time);
	generate_file_name(screen_shot_fn, dest_file_name, date_time);

	fr_callback_binder* tmp_binder = get_receiver_cb_binder();
	if (!tmp_binder)
	{
		Msg("! ERROR: CL: not enough receive channels (max is 32)");
		return;
	}

	if (g_draw_downloads)
	{
		draw_downloads(true);
	}
	else
	{
		draw_downloads(false);
	}

	tmp_binder->m_file_name = screen_shot_fn;
	tmp_binder->m_owner = this;
	tmp_binder->m_active = true;
	tmp_binder->m_downloaded_size = 0;	//initial value for rendering
	tmp_binder->m_max_size = 1;			//avoiding division by zero
	tmp_binder->m_response_type = response_event;

	file_transfer::receiving_state_callback_t receiving_cb =
		fastdelegate::MakeDelegate(tmp_binder,
			&game_cl_mp::fr_callback_binder::receiving_file_callback);

	tmp_binder->m_frnode = Level().m_file_transfer->start_receive_file(
		tmp_binder->m_writer,
		from_client,
		receiving_cb
	);
	if (!tmp_binder->m_frnode)
	{
		Msg("* screenshot: receiving failed ...");
		tmp_binder->m_active = false;
	}
}

void game_cl_mp::draw_downloads(bool draw)
{
	ss_manager.set_draw_downloads(draw);
}

void __stdcall	game_cl_mp::fr_callback_binder::receiving_file_callback(
	file_transfer::receiving_status_t status,
	u32 bytes_received,
	u32 data_size)
{
	if (g_draw_downloads)
	{
		m_owner->draw_downloads(true);
	}
	else
	{
		m_owner->draw_downloads(false);
	}
	switch (status)
	{
	case file_transfer::receiving_data:
	{
		Msg("* file: %d of %d bytes received ...", bytes_received, data_size);
		m_downloaded_size = bytes_received;
		m_max_size = data_size;
	}break;
	case file_transfer::receiving_aborted_by_peer:
	{
		Msg("* file: receiving aborted by peer...");
		m_active = false;
	}break;
	case file_transfer::receiving_aborted_by_user:
	{
		Msg("* file: receiving aborted by user...");
		m_active = false;
	}break;
	case file_transfer::receiving_timeout:
	{
		Msg("* file: receiving timeout...");
		m_active = false;
	}break;
	case file_transfer::receiving_complete:
	{
		Msg("* file: download complete successfully !");
		switch (m_response_type)
		{
		case e_screenshot_response:
		{
			m_owner->decompress_and_save_screenshot(
				m_file_name.c_str(),
				m_writer.pointer(),
				m_writer.size(),
				m_frnode->get_user_param()
			);
		}break;
		case e_configs_response:
		{
			/*m_owner->decompress_and_process_config(
				m_file_name.c_str(),
				m_writer.pointer(),
				m_writer.size(),
				m_frnode->get_user_param()
			);*/
		}break;
		default:
			NODEFAULT;
		}; //switch (m_response_type)

		m_active = false;
	}break;
	};
}

void game_cl_mp::SendCollectedData(u8 const* buffer, u32 buffer_size, u32 uncompressed_size)
{
	if (!buffer_size)
	{
		Msg("! ERROR: CL: no data to send...");
		return;
	}
	file_transfer::sending_state_callback_t sending_cb =
		fastdelegate::MakeDelegate(this, &game_cl_mp::sending_screenshot_callback);

	//screenshot is compressing in screenshot manager ...
	/*reinit_compress_buffer(buffer_size);

	u32 compressed_image_size = ppmd_compress(
		buffer_for_compress,
		buffer_for_compress_size,
		buffer,
		buffer_size
	);*/

	upload_memory_writer.clear();
	upload_memory_writer.w(buffer, buffer_size);

	Level().m_file_transfer->start_transfer_file(
		upload_memory_writer.pointer(),
		upload_memory_writer.size(),
		sending_cb,
		uncompressed_size
	);
};

void game_cl_mp::VotingBegin()
{
	if (!m_pVoteStartWindow)
		m_pVoteStartWindow = xr_new<CUIVotingCategory>();

	StartStopMenu(m_pVoteStartWindow, true);
}

void game_cl_mp::Vote()
{
	if (!m_pVoteRespondWindow)
		m_pVoteRespondWindow = xr_new<CUIVote>();

	StartStopMenu(m_pVoteRespondWindow, true);
}

void game_cl_mp::OnCantVoteMsg(LPCSTR Text)
{
	if (!m_pMessageBox)
		m_pMessageBox = xr_new<CUIMessageBoxEx>();

	m_pMessageBox->Init("cant_vote");
	m_pMessageBox->SetText(Text);
	StartStopMenu(m_pMessageBox, true);
}

void game_cl_mp::OnVoiceMessage(NET_Packet* P)
{
	m_pVoiceChat->ReceiveMessage(P);
}

bool game_cl_mp::OnKeyboardRelease(int key)
{
	if (key == kVoiceChat)
	{
		m_pVoiceChat->Stop();
		return true;
	}

	return inherited::OnKeyboardRelease(key);
}

char Color_Weapon[] = "%c[255,255,1,1]";
u32 Color_Teams_u32[3];
string32 Color_Teams[3];
char Color_Main[] = "%c[255,192,192,192]";
char Color_Radiation[] = "%c[255,0,255,255]";
char Color_Neutral[] = "%c[255,255,0,255]";
u32 Color_Neutral_u32 = color_rgba(255, 0, 255, 255);
char Color_Red[] = "%c[255,255,1,1]";
char Color_Green[] = "%c[255,1,255,1]";

void game_cl_mp::TranslateGameMessage(u32 msg, NET_Packet &P)
{
	string512 Text;
	CStringTable st;

	switch (msg)
	{

	case GAME_EVENT_PLAYER_KILLED: //dm
	{
		OnPlayerKilled(P);
	}
	break;
	case GAME_EVENT_VOTE_START:
	{
		sprintf_s(Text, "%s%s", Color_Main, *st.translate("mp_voting_started_msg"));
		CommonMessageOut(Text);
		OnVoteStart(P);
	}
	break;
	case GAME_EVENT_VOTE_STOP:
	{
		sprintf_s(Text, "%s%s", Color_Main, *st.translate("mp_voting_broken"));
		CommonMessageOut(Text);
		OnVoteStop(P);
	}
	break;
	case GAME_EVENT_VOTE_END:
	{
		string512 Reason;
		P.r_stringZ(Reason);
		sprintf_s(Text, "%s%s", Color_Main, *st.translate(Reason));
		CommonMessageOut(Text);
		OnVoteEnd(P);
	}
	break;
	case GAME_EVENT_PLAYER_NAME:
	{
		OnPlayerChangeName(P);
	}
	break;
	case GAME_EVENT_SPEECH_MESSAGE:
	{
		OnSpeechMessage(P);
	}
	break;
	case GAME_EVENT_PLAYERS_MONEY_CHANGED:
	{
		OnEventMoneyChanged(P);
	}
	break;
	case GAME_EVENT_PLAYER_GAME_MENU_RESPOND:
	{
		OnGameMenuRespond(P);
	}
	break;
	case GAME_EVENT_ROUND_STARTED:
	{
		OnGameRoundStarted();
	}
	break;
	case GAME_EVENT_ROUND_END:
	{
		string64 reason;
		P.r_stringZ(reason);
	}
	break;
	case GAME_EVENT_SERVER_STRING_MESSAGE:
	{
		string1024 mess;
		P.r_stringZ(mess);
		sprintf_s(Text, "%s%s", Color_Red, *st.translate(mess));
		CommonMessageOut(Text);
	}
	break;
	case GAME_EVENT_SERVER_DIALOG_MESSAGE:
	{
		string1024 mess;
		P.r_stringZ(mess);
		Msg(mess);
		if (MainMenu() && !g_dedicated_server)
		{
			MainMenu()->OnSessionTerminate(mess);
		}
	}
	break;

	case GAME_EVENT_MAKE_DATA:
	{
		clientdata_event_t etype = static_cast<clientdata_event_t>(P.r_u8());

		if (etype == e_screenshot_request)
		{
			screenshot_manager::complete_callback_t compl_cb =
				fastdelegate::MakeDelegate(this, &game_cl_mp::SendCollectedData);
			ss_manager.make_screenshot(compl_cb);
		}
		else if (etype == e_configs_request)
		{
			//mp_anticheat::configs_dumper::complete_callback_t compl_cb =
			//	fastdelegate::MakeDelegate(this, &game_cl_mp::SendCollectedData);
			//cd_manager.dump_config(compl_cb);
		}
		else if (etype == e_screenshot_response)
		{
			ClientID tmp_client(P.r_u32());
			shared_str client_name;
			P.r_stringZ(client_name);
			PrepareToReceiveFile(tmp_client, client_name, e_screenshot_response);
		}
		else if (etype == e_configs_response)
		{
			//ClientID tmp_client(P.r_u32());
			//shared_str client_name;
			//P.r_stringZ(client_name);
			//PrepareToReceiveFile(tmp_client, client_name, e_configs_response);
		}
		else
		{
			ClientID tmp_client(P.r_u32());
			shared_str error_msg;
			P.r_stringZ(error_msg);
			Msg("! File transfer error: from client [%u]: %s", tmp_client.value(), error_msg.c_str());
		}
	}break;

	default:
		inherited::TranslateGameMessage(msg, P);
	}
}

void game_cl_mp::ChatSayAll(const shared_str &phrase)
{
	NET_Packet P;
	P.w_begin(M_CHAT_MESSAGE);
	P.w_s16(0);
	P.w_stringZ(local_player->getName());
	P.w_stringZ(phrase.c_str());
	P.w_s16(local_player->team);
	u_EventSend(P);
}

void game_cl_mp::ChatSayTeam(const shared_str &phrase)
{
	NET_Packet P;
	P.w_begin(M_CHAT_MESSAGE);
	P.w_s16(local_player->team);
	P.w_stringZ(local_player->getName());
	P.w_stringZ(phrase.c_str());
	P.w_s16(local_player->team);
	u_EventSend(P);
}

void game_cl_mp::OnWarnMessage(NET_Packet *P)
{
	u8 msg_type = P->r_u8();
	if (msg_type == 1)
	{
		u16 _ping = P->r_u16();
		u8 _cnt = P->r_u8();
		u8 _total = P->r_u8();

		if (HUD().GetUI())
		{
			string512 _buff;
			sprintf_s(_buff, "max_ping_warn_%d", _cnt);
			SDrawStaticStruct *ss = HUD().GetUI()->AddInfoMessage(_buff);

			sprintf_s(_buff, "%d ms.", _ping);
			ss->m_static->SetText(_buff);
			CUIWindow *w = ss->m_static->FindChild("auto_static_0");
			if (w)
			{
				sprintf_s(_buff, "%d/%d", _cnt, _total);
				CUIStatic *s = smart_cast<CUIStatic *>(w);
				s->SetText(_buff);
			}
		}
	}
}

void game_cl_mp::OnChatMessage(NET_Packet *P)
{
	P->r_s16();
	string256 PlayerName;
	P->r_stringZ(PlayerName);
	string256 ChatMsg;
	P->r_stringZ(ChatMsg);
	s16 team;
	P->r_s16(team);
	///#ifdef DEBUG
	CStringTable st;
	switch (team)
	{
	case 0:
		Msg("%s: %s : %s", *st.translate("mp_chat"), PlayerName, ChatMsg);
		break;
	case 1:
		Msg("- %s: %s : %s", *st.translate("mp_chat"), PlayerName, ChatMsg);
		break;
	case 2:
		Msg("~ %s: %s : %s", *st.translate("mp_chat"), PlayerName, ChatMsg);
		break;
	}

	//#endif
	if (g_dedicated_server)
		return;

	string256 colPlayerName;
	sprintf_s(colPlayerName, "%s%s:%s", Color_Teams[team], PlayerName, "%c[default]");
	if (Level().CurrentViewEntity() && HUD().GetUI())
		HUD().GetUI()->m_pMessagesWnd->AddChatMessage(ChatMsg, colPlayerName);
};

void game_cl_mp::CommonMessageOut(LPCSTR msg)
{
	if (g_dedicated_server)
		return;

	if (HUD().GetUI())
		HUD().GetUI()->m_pMessagesWnd->AddLogMessage(msg);
}

void game_cl_mp::shedule_Update(u32 dt)
{
	UpdateSndMessages();

	inherited::shedule_Update(dt);

	if (g_dedicated_server)
		return;

	switch (Phase())
	{
	case GAME_PHASE_PENDING:
	{
		CUIChatWnd *pChatWnd = HUD().GetUI()->m_pMessagesWnd->GetChatWnd();
		if (pChatWnd && pChatWnd->IsShown())
			StartStopMenu(pChatWnd, false);

		if (m_bJustRestarted)
		{
			if (Level().CurrentViewEntity())
			{
				PlaySndMessage(ID_READY);
				m_bJustRestarted = false;
			};
		}
	}
	break;
	case GAME_PHASE_INPROGRESS:
	{
		if (!local_player || local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))		
			HideMessageMenus();
		
		if (IsVotingEnabled() && IsVotingActive() && m_dwVoteEndTime >= Level().timeServer())
		{
			CStringTable st;
			u32 TimeLeft = m_dwVoteEndTime - Level().timeServer();
			string1024 VoteTimeResStr;
			u32 SecsLeft = (TimeLeft % 60000) / 1000;
			u32 MinitsLeft = (TimeLeft - SecsLeft) / 60000;

			u32 NumAgreed = 0;

			for (const auto &it : players)
			{
				if (it.second->m_bCurrentVoteAgreed == 1)
					NumAgreed++;
			}

			sprintf_s(VoteTimeResStr, st.translate("mp_timeleft").c_str(), MinitsLeft, SecsLeft, float(NumAgreed) / players.size());

			if(CUIGameBaseMP *mpUI = GetMpGameUI())
				mpUI->SetVoteTimeResultMsg(VoteTimeResStr);
		}
	}
	break;

	default:
	{
		CUIChatWnd *pChatWnd = HUD().GetUI()->m_pMessagesWnd->GetChatWnd();
		if (pChatWnd && pChatWnd->IsShown())
			StartStopMenu(pChatWnd, false);
	}
	break;
	}
	UpdateMapLocations();

	u32 cur_game_state = Phase();

	if (m_pVoteStartWindow && m_pVoteStartWindow->IsShown() && cur_game_state != GAME_PHASE_INPROGRESS)
	{
		m_pVoteStartWindow->GetHolder()->StartStopMenu(m_pVoteStartWindow, true);
	}
	if (m_pVoteRespondWindow && m_pVoteRespondWindow->IsShown() && (cur_game_state != GAME_PHASE_INPROGRESS || !IsVotingActive()))
	{
		m_pVoteRespondWindow->GetHolder()->StartStopMenu(m_pVoteRespondWindow, true);
	}

	if (m_pMessageBox && m_pMessageBox->IsShown() && cur_game_state != GAME_PHASE_INPROGRESS)
	{
		m_pMessageBox->GetHolder()->StartStopMenu(m_pMessageBox, true);
	}

	if (m_pVoiceChat)
	{
		const bool started = m_pVoiceChat->IsStarted();
		const bool is_dead = !local_player || local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);

		if (started && is_dead)		
			m_pVoiceChat->Stop();
		
		m_pVoiceChat->Update();
	}
}

void game_cl_mp::SendStartVoteMessage(LPCSTR args)
{
	if (!args)
		return;
	if (!IsVotingEnabled())
		return;
	NET_Packet P;
	Game().u_EventGen(P, GE_GAME_EVENT, Game().local_player->GameID);
	P.w_u16(GAME_EVENT_VOTE_START);
	P.w_stringZ(args);
	Game().u_EventSend(P);
};

void game_cl_mp::SendVoteYesMessage()
{
	if (!IsVotingEnabled() || !IsVotingActive())
		return;
	NET_Packet P;
	Game().u_EventGen(P, GE_GAME_EVENT, Game().local_player->GameID);
	P.w_u16(GAME_EVENT_VOTE_YES);
	Game().u_EventSend(P);
}

void game_cl_mp::SendVoteNoMessage()
{
	if (!IsVotingEnabled() || !IsVotingActive())
		return;
	NET_Packet P;
	Game().u_EventGen(P, GE_GAME_EVENT, Game().local_player->GameID);
	P.w_u16(GAME_EVENT_VOTE_NO);
	Game().u_EventSend(P);
}

const u32 MAX_VOTE_PARAMS = 5;

void game_cl_mp::OnVoteStart(NET_Packet &P)
{
	SetVotingActive(true);
	CStringTable st;

	string128 Command = "";
	string64 Player = "";
	P.r_stringZ(Command);
	P.r_stringZ(Player);

	m_dwVoteEndTime = Level().timeServer() + P.r_u32();
	CUIGameBaseMP* mpGameUi = GetMpGameUI();

	if (!mpGameUi)
		return;

	string1024 CmdName = "";
	string1024 NewCmd;
	strcpy(NewCmd, Command);
	string1024 CmdParams[MAX_VOTE_PARAMS] = { "", "", "", "", "" };
	sscanf(Command, "%s %s %s %s %s %s", CmdName, CmdParams[0], CmdParams[1], CmdParams[2], CmdParams[3], CmdParams[4]);

	if (!xr_strcmp(CmdName, "restart"))	
		sprintf_s(NewCmd, "%s", *st.translate("mp_restart"));	
	else if (!xr_strcmp(CmdName, "restart_fast"))	
		sprintf_s(NewCmd, "%s", *st.translate("mp_restart_fast"));	
	else if (!xr_strcmp(CmdName, "kick"))
	{
		sprintf_s(NewCmd, "%s %s", *st.translate("mp_kick"), CmdParams[0]);

		for (int i = 1; i < MAX_VOTE_PARAMS; i++)
		{
			if (xr_strlen(CmdParams[i]))
			{
				strcat(NewCmd, " ");
				strcat(NewCmd, CmdParams[i]);
			}
		}
	}
	else if (!xr_strcmp(CmdName, "ban"))
	{
		sprintf_s(NewCmd, "%s %s", *st.translate("mp_ban"), CmdParams[0]);

		for (int i = 1; i < MAX_VOTE_PARAMS; i++)
		{
			if (xr_strlen(CmdParams[i]))
			{
				strcat(NewCmd, " ");
				strcat(NewCmd, CmdParams[i]);
			}
		}
	}
	else if (!xr_strcmp(CmdName, "changemap"))
		sprintf_s(NewCmd, "%s %s", *st.translate("mp_change_map"), *st.translate(CmdParams[0]));
	else if (!xr_strcmp(CmdName, "changeweather"))
		sprintf_s(NewCmd, "%s %s", *st.translate("mp_change_weather"), *st.translate(CmdParams[0]));

	string1024 VoteStr;
	sprintf_s(VoteStr, *st.translate("mp_voting_started"), NewCmd, Player);

	mpGameUi->SetVoteMessage(VoteStr);
	mpGameUi->SetVoteTimeResultMsg("");

	if (!m_pVoteRespondWindow)
		m_pVoteRespondWindow = xr_new<CUIVote>();

	m_pVoteRespondWindow->SetVoting(VoteStr);
}

void game_cl_mp::OnVoteStop(NET_Packet &P)
{
	SetVotingActive(false);

	if (m_pVoteRespondWindow && m_pVoteRespondWindow->IsShown())
		StartStopMenu(m_pVoteRespondWindow, true);

	if (CUIGameBaseMP* mpGameUi = GetMpGameUI())
	{
		mpGameUi->SetVoteMessage(nullptr);
		mpGameUi->SetVoteTimeResultMsg(nullptr);
	}
}

void game_cl_mp::OnVoteEnd(NET_Packet &P)
{
	SetVotingActive(false);

	if (CUIGameBaseMP* mpGameUi = GetMpGameUI())
	{
		mpGameUi->SetVoteMessage(nullptr);
		mpGameUi->SetVoteTimeResultMsg(nullptr);
	}
}

void game_cl_mp::OnPlayerVoted(game_PlayerState *ps)
{
	if (!IsVotingActive())
		return;
	if (ps->m_bCurrentVoteAgreed == 2)
		return;

	CStringTable st;
	string1024 resStr;
	sprintf_s(resStr, "%s\"%s\" %s%s %s\"%s\"", Color_Teams[ps->team], ps->getName(), Color_Main, *st.translate("mp_voted"),
			  ps->m_bCurrentVoteAgreed ? Color_Green : Color_Red, *st.translate(ps->m_bCurrentVoteAgreed ? "mp_voted_yes" : "mp_voted_no"));
	CommonMessageOut(resStr);
}
void game_cl_mp::LoadTeamData(const shared_str &TeamName)
{
	cl_TeamStruct Team;
	Team.IndicatorPos.set(0.f, 0.f, 0.f);
	Team.Indicator_r1 = 0.f;
	Team.Indicator_r2 = 0.f;

	Team.caSection = TeamName;
	if (pSettings->section_exist(TeamName))
	{
		Team.Indicator_r1 = pSettings->r_float(TeamName, "indicator_r1");
		Team.Indicator_r2 = pSettings->r_float(TeamName, "indicator_r2");

		Team.IndicatorPos.x = pSettings->r_float(TeamName, "indicator_x");
		Team.IndicatorPos.y = pSettings->r_float(TeamName, "indicator_y");
		Team.IndicatorPos.z = pSettings->r_float(TeamName, "indicator_z");

		LPCSTR ShaderType = pSettings->r_string(TeamName, "indicator_shader");
		LPCSTR ShaderTexture = pSettings->r_string(TeamName, "indicator_texture");
		Team.IndicatorShader->create(ShaderType, ShaderTexture);

		ShaderType = pSettings->r_string(TeamName, "invincible_shader");
		ShaderTexture = pSettings->r_string(TeamName, "invincible_texture");
		Team.InvincibleShader->create(ShaderType, ShaderTexture);
	};
	TeamList.push_back(Team);
}

void game_cl_mp::OnSwitchPhase_InProgress(){};

void game_cl_mp::OnSwitchPhase(u32 old_phase, u32 new_phase)
{
	inherited::OnSwitchPhase(old_phase, new_phase);
	switch (new_phase)
	{
	case GAME_PHASE_INPROGRESS:
	case GAME_PHASE_RACE_START:
	{
		m_bSpectatorSelected = FALSE;

		if (Level().pHUD && HUD().GetUI())		
			HUD().GetUI()->ShowGameIndicators();		
	}
	break;

	case GAME_PHASE_PENDING:
	{
		m_bJustRestarted = true;
		HideMessageMenus();
	}
	break;

	case GAME_PHASE_TEAM1_SCORES:
	case GAME_PHASE_TEAM2_SCORES:
	case GAME_PHASE_TEAM1_ELIMINATED:
	case GAME_PHASE_TEAM2_ELIMINATED:
	case GAME_PHASE_TEAMS_IN_A_DRAW:
	case GAME_PHASE_PLAYER_SCORES:
		HideMessageMenus();
		break;

	default:
	{
		if (Level().pHUD && HUD().GetUI())
		{
			HUD().GetUI()->HideGameIndicators();
		};
		HideMessageMenus();
	}
	break;
	}
}

const ui_shader& game_cl_mp::GetEquipmentIconsShader()
{
	if (m_EquipmentIconsShader->inited()) 
		return m_EquipmentIconsShader;

	m_EquipmentIconsShader->create("hud\\default", EQUIPMENT_ICONS);
	return m_EquipmentIconsShader;
}

const ui_shader& game_cl_mp::GetKillEventIconsShader()
{
	return GetEquipmentIconsShader();
}

const ui_shader& game_cl_mp::GetRadiationIconsShader()
{
	return GetEquipmentIconsShader();
}

const ui_shader& game_cl_mp::GetBloodLossIconsShader()
{
	return GetEquipmentIconsShader();
}

const ui_shader& game_cl_mp::GetRankIconsShader()
{
	if (m_RankIconsShader->inited()) 
		return m_RankIconsShader;

	m_RankIconsShader->create("hud\\default", RANK_ICONS);
	return m_RankIconsShader;
}

void game_cl_mp::OnPlayerKilled(NET_Packet &P)
{
	CStringTable st;
	//-----------------------------------------------------------
	KILL_TYPE KillType = KILL_TYPE(P.r_u8());
	u16 KilledID = P.r_u16();
	u16 KillerID = P.r_u16();
	u16 WeaponID = P.r_u16();
	SPECIAL_KILL_TYPE SpecialKill = SPECIAL_KILL_TYPE(P.r_u8());
	//-----------------------------------------------------------
	CObject *pOKiller = Level().Objects.net_Find(KillerID);
	CObject *pWeapon = Level().Objects.net_Find(WeaponID);

	game_PlayerState *pPlayer = GetPlayerByGameID(KilledID);
	if (!pPlayer)
	{
		Msg("! Non existant player[%d] killed by [%d] with [%d]", KilledID, KillerID, WeaponID);
		return;
	}
	R_ASSERT(pPlayer);
	game_PlayerState *pKiller = GetPlayerByGameID(KillerID);
	//	R_ASSERT(pKiller);
	//-----------------------------------------------------------
	KillMessageStruct KMS;
	KMS.m_victim.m_name = pPlayer->name;
	KMS.m_victim.m_color = Color_Teams_u32[pPlayer->team];

	KMS.m_killer.m_name = NULL;
	KMS.m_killer.m_color = color_rgba(255, 255, 255, 255);

	switch (KillType)
	{
		//-----------------------------------------------------------
	case KT_HIT: //from hit
	{
		string1024 sWeapon = "", sSpecial = "";
		if (pWeapon)
		{
			CInventoryItem *pIItem = smart_cast<CInventoryItem *>(pWeapon);
			if (pIItem)
			{
				KMS.m_initiator.m_shader = GetEquipmentIconsShader();
				KMS.m_initiator.m_rect.x1 = pIItem->GetKillMsgXPos();
				KMS.m_initiator.m_rect.y1 = pIItem->GetKillMsgYPos();
				KMS.m_initiator.m_rect.x2 = KMS.m_initiator.m_rect.x1 + pIItem->GetKillMsgWidth();
				KMS.m_initiator.m_rect.y2 = KMS.m_initiator.m_rect.y1 + pIItem->GetKillMsgHeight();
				sprintf_s(sWeapon, "%s %s", st.translate("mp_from").c_str(), pIItem->NameShort());
			}
			else
			{
				CCustomZone *pAnomaly = smart_cast<CCustomZone *>(pWeapon);
				if (pAnomaly)
				{
					KMS.m_initiator.m_shader = GetKillEventIconsShader();
					KMS.m_initiator.m_rect.x1 = 1;
					KMS.m_initiator.m_rect.y1 = 202;
					KMS.m_initiator.m_rect.x2 = KMS.m_initiator.m_rect.x1 + 31;
					KMS.m_initiator.m_rect.y2 = KMS.m_initiator.m_rect.y1 + 30;
					sprintf_s(sWeapon, *st.translate("mp_by_anomaly"));
				}
			}
		}

		if (pKiller || pOKiller)
		{
			if (!pKiller)
			{
				CCustomZone *pAnomaly = smart_cast<CCustomZone *>(pOKiller);
				if (pAnomaly)
				{
					KMS.m_initiator.m_shader = GetKillEventIconsShader();
					KMS.m_initiator.m_rect.x1 = 1;
					KMS.m_initiator.m_rect.y1 = 202;
					KMS.m_initiator.m_rect.x2 = KMS.m_initiator.m_rect.x1 + 31;
					KMS.m_initiator.m_rect.y2 = KMS.m_initiator.m_rect.y1 + 30;
					Msg("%s killed by anomaly", *KMS.m_victim.m_name);
					break;
				}
			};

			if (pKiller)
			{
				KMS.m_killer.m_name = pKiller ? pKiller->name : *(pOKiller->cNameSect());
				KMS.m_killer.m_color = pKiller ? Color_Teams_u32[pKiller->team] : Color_Neutral_u32;
			};
		};
		//-------------------------------------------
		switch (SpecialKill)
		{
		case SKT_NONE: // not special
		{
			if (pOKiller && pOKiller == Level().CurrentViewEntity())
			{
				if (pWeapon && pWeapon->CLS_ID == CLSID_OBJECT_W_KNIFE)
					PlaySndMessage(ID_BUTCHER);
			};
		}
		break;
		case SKT_HEADSHOT: // Head Shot
		{
			BONUSES_it it = std::find(m_pBonusList.begin(), m_pBonusList.end(), "headshot");
			if (it != m_pBonusList.end() && (*it == "headshot"))
			{
				Bonus_Struct *pBS = &(*it);
				KMS.m_ext_info.m_shader = pBS->IconShader;
				KMS.m_ext_info.m_rect.x1 = pBS->IconRects[0].x1;
				KMS.m_ext_info.m_rect.y1 = pBS->IconRects[0].y1;
				KMS.m_ext_info.m_rect.x2 = pBS->IconRects[0].x1 + pBS->IconRects[0].x2;
				KMS.m_ext_info.m_rect.y2 = pBS->IconRects[0].y1 + pBS->IconRects[0].y2;
			};

			sprintf_s(sSpecial, *st.translate("mp_with_headshot"));

			if (pOKiller && pOKiller == Level().CurrentViewEntity())
				PlaySndMessage(ID_HEADSHOT);
		}
		break;
		case SKT_BACKSTAB: // BackStab
		{
			BONUSES_it it = std::find(m_pBonusList.begin(), m_pBonusList.end(), "backstab");
			if (it != m_pBonusList.end() && (*it == "backstab"))
			{
				Bonus_Struct *pBS = &(*it);
				KMS.m_ext_info.m_shader = pBS->IconShader;
				KMS.m_ext_info.m_rect.x1 = pBS->IconRects[0].x1;
				KMS.m_ext_info.m_rect.y1 = pBS->IconRects[0].y1;
				KMS.m_ext_info.m_rect.x2 = pBS->IconRects[0].x1 + pBS->IconRects[0].x2;
				KMS.m_ext_info.m_rect.y2 = pBS->IconRects[0].y1 + pBS->IconRects[0].y2;
			};

			sprintf_s(sSpecial, *st.translate("mp_with_backstab"));

			if (pOKiller && pOKiller == Level().CurrentViewEntity())
				PlaySndMessage(ID_ASSASSIN);
		}
		break;
		}
		//suicide
		if (KilledID == KillerID)
		{
			KMS.m_victim.m_name = NULL;

			KMS.m_ext_info.m_shader = GetKillEventIconsShader();
			KMS.m_ext_info.m_rect.x1 = 32;
			KMS.m_ext_info.m_rect.y1 = 202;
			KMS.m_ext_info.m_rect.x2 = KMS.m_ext_info.m_rect.x1 + 30;
			KMS.m_ext_info.m_rect.y2 = KMS.m_ext_info.m_rect.y1 + 30;
			//-------------------------------------
			Msg(sWeapon[0] ? "%s killed himself by %s" : "%s killed himself", *KMS.m_killer.m_name, sWeapon[0] ? sWeapon + 5 : "");
		}
		else
		{
			//-------------------------------------
			Msg("%s killed %s %s%s", *KMS.m_killer.m_name, *KMS.m_victim.m_name, sWeapon, sSpecial[0] ? sSpecial : "");
		}
	}
	break;
		//-----------------------------------------------------------
	case KT_BLEEDING: //from bleeding
	{
		KMS.m_initiator.m_shader = GetBloodLossIconsShader();
		KMS.m_initiator.m_rect.x1 = 238;
		KMS.m_initiator.m_rect.y1 = 31;
		KMS.m_initiator.m_rect.x2 = KMS.m_initiator.m_rect.x1 + 17;
		KMS.m_initiator.m_rect.y2 = KMS.m_initiator.m_rect.y1 + 26;

		if (!pKiller)
		{
			CCustomZone *pAnomaly = smart_cast<CCustomZone *>(pOKiller);
			if (pAnomaly)
			{
				KMS.m_ext_info.m_shader = GetKillEventIconsShader();
				KMS.m_ext_info.m_rect.x1 = 1;
				KMS.m_ext_info.m_rect.y1 = 202;
				KMS.m_ext_info.m_rect.x2 = KMS.m_ext_info.m_rect.x1 + 31;
				KMS.m_ext_info.m_rect.y2 = KMS.m_ext_info.m_rect.y1 + 30;

				Msg("%s died from bleeding, thanks to anomaly", *KMS.m_victim.m_name);
				break;
			}
		};

		if (pKiller)
		{
			KMS.m_killer.m_name = pKiller ? pKiller->name : *(pOKiller->cNameSect());
			KMS.m_killer.m_color = pKiller ? Color_Teams_u32[pKiller->team] : Color_Neutral_u32;
			//-----------------------------------------------------------------------
			Msg("%s died from bleeding, thanks to %s ", *KMS.m_victim.m_name, *KMS.m_killer.m_name);
		}
		else
		{
			//-----------------------------------------------------------------
			Msg("%s died from bleeding", *KMS.m_victim.m_name);
		};
	}
	break;
		//-----------------------------------------------------------
	case KT_RADIATION: //from radiation
	{
		KMS.m_initiator.m_shader = GetRadiationIconsShader();
		KMS.m_initiator.m_rect.x1 = 215;
		KMS.m_initiator.m_rect.y1 = 195;
		KMS.m_initiator.m_rect.x2 = KMS.m_initiator.m_rect.x1 + 24;
		KMS.m_initiator.m_rect.y2 = KMS.m_initiator.m_rect.y1 + 24;
		//---------------------------------------------------------
		Msg("%s killed by radiation", *KMS.m_victim.m_name);
	}
	break;
	default:
		break;
	}
	if (HUD().GetUI() && HUD().GetUI()->m_pMessagesWnd)
		HUD().GetUI()->m_pMessagesWnd->AddLogMessage(KMS);
};

void game_cl_mp::OnPlayerChangeName(NET_Packet &P)
{
	CStringTable st;

	u16 ObjID = P.r_u16();
	s16 Team = P.r_s16();
	string1024 OldName, NewName;
	P.r_stringZ(OldName);
	P.r_stringZ(NewName);

	string1024 resStr;
	sprintf_s(resStr, "%s\"%s\" %s%s %s\"%s\"", Color_Teams[Team], OldName, Color_Main, *st.translate("mp_is_now"), Color_Teams[Team], NewName);
	CommonMessageOut(resStr);
	//-------------------------------------------
	CObject *pObj = Level().Objects.net_Find(ObjID);
	if (pObj)
		pObj->cName_set(NewName);
}

void game_cl_mp::LoadSndMessages()
{
	LoadSndMessage("mp_snd_messages", "headshot", ID_HEADSHOT);
	LoadSndMessage("mp_snd_messages", "butcher", ID_BUTCHER);
	LoadSndMessage("mp_snd_messages", "assassin", ID_ASSASSIN);
	LoadSndMessage("mp_snd_messages", "ready", ID_READY);
	LoadSndMessage("mp_snd_messages", "match_started", ID_MATCH_STARTED);

	LoadSndMessage("dm_snd_messages", "countdown_5", ID_COUNTDOWN_5);
	LoadSndMessage("dm_snd_messages", "countdown_4", ID_COUNTDOWN_4);
	LoadSndMessage("dm_snd_messages", "countdown_3", ID_COUNTDOWN_3);
	LoadSndMessage("dm_snd_messages", "countdown_2", ID_COUNTDOWN_2);
	LoadSndMessage("dm_snd_messages", "countdown_1", ID_COUNTDOWN_1);

	LoadSndMessage("dm_snd_messages", "you_won", ID_YOU_WON);
};

void game_cl_mp::OnRankChanged(u8 OldRank)
{
	CStringTable st;
	string256 tmp;
	string1024 RankStr;
	sprintf_s(tmp, "rank_%d", local_player->rank);
	sprintf_s(RankStr, "%s : %s", *st.translate("mp_your_rank"), *st.translate(READ_IF_EXISTS(pSettings, r_string, tmp, "rank_name", "")));
	CommonMessageOut(RankStr);
#ifdef DEBUG
	Msg("- %s", RankStr);
#endif
};

void game_cl_mp::net_import_update(NET_Packet &P)
{
	u8 OldRank = u8(-1);
	s16 OldTeam = -1;
	if (local_player)
	{
		OldRank = local_player->rank;
		OldTeam = local_player->team;
	};
	//---------------------------------------------
	inherited::net_import_update(P);
	//---------------------------------------------
	if (local_player)
	{
		if (OldTeam != local_player->team)
			OnTeamChanged();
		if (OldRank != local_player->rank)
			OnRankChanged(OldRank);
	};
}

void game_cl_mp::net_import_state(NET_Packet &P)
{
	u8 OldRank = u8(-1);
	s16 OldTeam = -1;

	if (local_player)
	{
		OldRank = local_player->rank;
		OldTeam = local_player->team;
	}

	inherited::net_import_state(P);

	if (local_player)
	{
		if (OldTeam != local_player->team)
			OnTeamChanged();

		if (OldRank != local_player->rank)
			OnRankChanged(OldRank);
	}

	m_u8SpectatorModes = P.r_u8();

	m_bSpectator_FreeFly = (m_u8SpectatorModes & (1 << CSpectator::eacFreeFly)) != 0;
	m_bSpectator_FirstEye = (m_u8SpectatorModes & (1 << CSpectator::eacFirstEye)) != 0;
	m_bSpectator_LookAt = (m_u8SpectatorModes & (1 << CSpectator::eacLookAt)) != 0;
	m_bSpectator_FreeLook = (m_u8SpectatorModes & (1 << CSpectator::eacFreeLook)) != 0;
	m_bSpectator_TeamCamera = (m_u8SpectatorModes & (1 << CSpectator::eacMaxCam)) != 0;
}

bool game_cl_mp::Is_Spectator_Camera_Allowed(CSpectator::EActorCameras Camera)
{
	return (!!(m_u8SpectatorModes & (1 << Camera)));
}

void game_cl_mp::OnEventMoneyChanged(NET_Packet &P)
{
	if (!local_player)
		return;
	CUIGameDM *pUIDM = smart_cast<CUIGameDM *>(m_game_ui_custom);
	local_player->money_for_round = P.r_s32();
	OnMoneyChanged();
	{
		if (pUIDM)
		{
			string256 MoneyStr;
			itoa(local_player->money_for_round, MoneyStr, 10);
			pUIDM->ChangeTotalMoneyIndicator(MoneyStr);
		}
	}
	s32 Money_Added = P.r_s32();
	if (Money_Added != 0)
	{
		if (pUIDM)
		{
			string256 MoneyStr;
			sprintf_s(MoneyStr, (Money_Added > 0) ? "+%d" : "%d", Money_Added);
			pUIDM->DisplayMoneyChange(MoneyStr);
		}
	};
	u8 NumBonuses = P.r_u8();
	s32 TotalBonusMoney = 0;
	shared_str BonusStr = (NumBonuses > 1) ? "Your bonuses : " : ((NumBonuses == 1) ? "Your bonus : " : "");
	for (u8 i = 0; i < NumBonuses; i++)
	{
		s32 BonusMoney = P.r_s32();
		SPECIAL_KILL_TYPE BonusReason = SPECIAL_KILL_TYPE(P.r_u8());
		u8 BonusKills = (BonusReason == SKT_KIR) ? P.r_u8() : 0;
		TotalBonusMoney += BonusMoney;
		//---------------------------------------------------------
		KillMessageStruct BMS;
		string256 MoneyStr;
		if (BonusMoney >= 0)
			sprintf_s(MoneyStr, "+%d", BonusMoney);
		else
			sprintf_s(MoneyStr, "-%d", BonusMoney);
		BMS.m_victim.m_name = MoneyStr;
		BMS.m_victim.m_color = 0xff00ff00;
		u32 RectID = 0;
		//---------------------------------------------------------
		shared_str BName = "";
		switch (BonusReason)
		{
		case SKT_HEADSHOT:
		{
			BName = "headshot";
		}
		break;
		case SKT_BACKSTAB:
		{
			BName = "backstab";
		}
		break;
		case SKT_KNIFEKILL:
		{
			BName = "knife_kill";
		}
		break;
		case SKT_PDA:
		{
			BName = "pda_taken";
		}
		break;
		case SKT_KIR:
		{
			BName.sprintf("%d_kill_in_row", BonusKills);

			sprintf_s(MoneyStr, sizeof(MoneyStr), "%d", BonusKills);
			BMS.m_killer.m_name = MoneyStr;
			BMS.m_killer.m_color = 0xffff0000;
		}
		break;
		case SKT_NEWRANK:
		{
			BName = "new_rank";
			RectID = (local_player->rank + 1) * 2 + ModifyTeam(local_player->team);
		}
		break;
		};
		BONUSES_it it = std::find(m_pBonusList.begin(), m_pBonusList.end(), BName.c_str());
		if (it != m_pBonusList.end() && (*it == BName.c_str()))
		{
			Bonus_Struct *pBS = &(*it);

			BMS.m_initiator.m_shader = pBS->IconShader;
			BMS.m_initiator.m_rect.x1 = pBS->IconRects[RectID].x1;
			BMS.m_initiator.m_rect.y1 = pBS->IconRects[RectID].y1;
			BMS.m_initiator.m_rect.x2 = pBS->IconRects[RectID].x1 + pBS->IconRects[RectID].x2;
			BMS.m_initiator.m_rect.y2 = pBS->IconRects[RectID].y1 + pBS->IconRects[RectID].y2;
		};

		if (pUIDM)
			pUIDM->DisplayMoneyBonus(BMS);
	};
};

void game_cl_mp::OnSpectatorSelect()
{
	CObject *l_pObj = Level().CurrentEntity();

	CGameObject *l_pPlayer = smart_cast<CGameObject *>(l_pObj);
	if (!l_pPlayer)
		return;

	NET_Packet P;
	l_pPlayer->u_EventGen(P, GE_GAME_EVENT, l_pPlayer->ID());
	//	P.w_u16(GAME_EVENT_PLAYER_SELECT_SPECTATOR);
	P.w_u16(GAME_EVENT_PLAYER_GAME_MENU);
	P.w_u8(PLAYER_SELECT_SPECTATOR);
	l_pPlayer->u_EventSend(P);

	m_bSpectatorSelected = TRUE;
};

void game_cl_mp::OnGameMenuRespond(NET_Packet &P)
{
	u8 Respond = P.r_u8();
	switch (Respond)
	{
	case PLAYER_SELECT_SPECTATOR:
	{
		OnGameMenuRespond_Spectator(P);
	}
	break;
	case PLAYER_CHANGE_TEAM:
	{
		OnGameMenuRespond_ChangeTeam(P);
	}
	break;
	case PLAYER_CHANGE_SKIN:
	{
		OnGameMenuRespond_ChangeSkin(P);
	}
	break;
	}
};

void game_cl_mp::OnGameRoundStarted()
{
	//			sprintf_s(Text, "%sRound started !!!",Color_Main);
	string512 Text;
	CStringTable st;
	sprintf_s(Text, "%s%s", Color_Main, *st.translate("mp_match_started"));
	CommonMessageOut(Text);
	OnSwitchPhase_InProgress();
	//-------------------------------
	PlaySndMessage(ID_MATCH_STARTED);
}

void game_cl_mp::OnBuySpawn(CUIWindow *pWnd, void *p)
{
	OnBuySpawnMenu_Ok();
};

void game_cl_mp::LoadBonuses()
{
	if (!pSettings->section_exist("mp_bonus_money"))
		return;
	m_pBonusList.clear();
	u32 BonusCount = pSettings->line_count("mp_bonus_money");
	for (u32 i = 0; i < BonusCount; i++)
	{
		LPCSTR line, name;
		pSettings->r_line("mp_bonus_money", i, &name, &line);
		//-------------------------------------
		string1024 tmp0, tmp1, IconStr;
		_GetItem(line, 0, tmp0);
		_GetItem(line, 1, tmp1);
		if (strstr(name, "kill_in_row"))
		{
			sprintf_s(tmp1, "%s Kill", tmp1);
			sprintf_s(IconStr, "kill_in_row");
		}
		else
			sprintf_s(IconStr, "%s", name);
		//-------------------------------------
		Bonus_Struct NewBonus;
		NewBonus.BonusTypeName = name;
		NewBonus.BonusName = tmp1;
		NewBonus.MoneyStr = tmp0;
		NewBonus.Money = atol(tmp0);
		//-------------------------------------
		if (!strstr(name, "new_rank"))
		{
			string1024 IconShader, IconX, IconY, IconW, IconH;
			sprintf_s(IconShader, "%s_shader", IconStr);
			sprintf_s(IconX, "%s_x", IconStr);
			sprintf_s(IconY, "%s_y", IconStr);
			sprintf_s(IconW, "%s_w", IconStr);
			sprintf_s(IconH, "%s_h", IconStr);
			if (pSettings->line_exist("mp_bonus_icons", IconShader))
			{
				NewBonus.IconShader->create("hud\\default", pSettings->r_string("mp_bonus_icons", IconShader));
			}
			Frect IconRect;
			IconRect.x1 = READ_IF_EXISTS(pSettings, r_float, "mp_bonus_icons", IconX, 0);
			IconRect.y1 = READ_IF_EXISTS(pSettings, r_float, "mp_bonus_icons", IconY, 0);
			IconRect.x2 = READ_IF_EXISTS(pSettings, r_float, "mp_bonus_icons", IconW, 0);
			IconRect.y2 = READ_IF_EXISTS(pSettings, r_float, "mp_bonus_icons", IconH, 0);
			NewBonus.IconRects.push_back(IconRect);
		}
		else
		{
			LPCSTR IconShader = CUITextureMaster::GetTextureFileName("ui_hud_status_blue_01");
			NewBonus.IconShader->create("hud\\default", IconShader);

			Frect IconRect;
			for (u32 r = 1; r <= 5; r++)
			{
				string256 rankstr;

				sprintf_s(rankstr, "ui_hud_status_green_0%d", r);
				IconRect = CUITextureMaster::GetTextureRect(rankstr);
				IconRect.x2 -= IconRect.x1;
				IconRect.y2 -= IconRect.y1;
				NewBonus.IconRects.push_back(IconRect);

				sprintf_s(rankstr, "ui_hud_status_blue_0%d", r);
				IconRect = CUITextureMaster::GetTextureRect(rankstr);
				IconRect.x2 -= IconRect.x1;
				IconRect.y2 -= IconRect.y1;
				NewBonus.IconRects.push_back(IconRect);
			}
		};
		//--------------------------------------
		m_pBonusList.push_back(NewBonus);
	};
};

void game_cl_mp::OnRadminMessage(u16 type, NET_Packet *P)
{
	switch (type)
	{
	case M_REMOTE_CONTROL_CMD:
	{
		string4096 buff;
		P->r_stringZ(buff);
		Msg("# srv: %s", buff);
	}
	break;
	}
}
