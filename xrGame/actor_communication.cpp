#include "pch_script.h"
#include "actor.h"
#include "UIGameSP.h"
#include "UI.h"
#include "PDA.h"
#include "HUDManager.h"
#include "level.h"
#include "string_table.h"
#include "PhraseDialog.h"
#include "character_info.h"
#include "relation_registry.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_registry_container.h"
#include "script_game_object.h"
#include "game_cl_base.h"
#include "xrServer.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_registry_wrappers.h"
#include "map_manager.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIPdaWnd.h"
#include "ui/UIDiaryWnd.h"
#include "ui/UITalkWnd.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "encyclopedia_article.h"
#include "GameTaskManager.h"
#include "GameTaskdefs.h"
#include "infoportion.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "ai/trader/ai_trader.h"

ENGINE_API bool g_dedicated_server;

void CActor::AddEncyclopediaArticle(const CInfoPortion *info_portion) const
{
	VERIFY(info_portion);
	ARTICLE_VECTOR &article_vector = encyclopedia_registry->registry().objects();

	ARTICLE_VECTOR::iterator last_end = article_vector.end();
	ARTICLE_VECTOR::iterator B = article_vector.begin();
	ARTICLE_VECTOR::iterator E = last_end;

	for (ARTICLE_ID_VECTOR::const_iterator it = info_portion->ArticlesDisable().begin();
		 it != info_portion->ArticlesDisable().end(); it++)
	{
		FindArticleByIDPred pred(*it);
		last_end = std::remove_if(B, last_end, pred);
	}
	article_vector.erase(last_end, E);

	for (ARTICLE_ID_VECTOR::const_iterator it = info_portion->Articles().begin();
		 it != info_portion->Articles().end(); it++)
	{
		FindArticleByIDPred pred(*it);
		if (std::find_if(article_vector.begin(), article_vector.end(), pred) != article_vector.end())
			continue;

		CEncyclopediaArticle article;

		article.Load(*it);

		article_vector.push_back(ARTICLE_DATA(*it, Level().GetGameTime(), article.data()->articleType));
		LPCSTR g, n;
		int _atype = article.data()->articleType;
		g = *(article.data()->group);
		n = *(article.data()->name);
		callback(GameObject::eArticleInfo)(lua_game_object(), g, n, _atype);

		if (HUD().GetUI())
		{
			pda_section::part p = pda_section::encyclopedia;

			switch (article.data()->articleType)
			{
			case ARTICLE_DATA::eEncyclopediaArticle:
				p = pda_section::encyclopedia;
				break;
			case ARTICLE_DATA::eJournalArticle:
				p = pda_section::journal;
				break;
			case ARTICLE_DATA::eInfoArticle:
				p = pda_section::info;
				break;
			case ARTICLE_DATA::eTaskArticle:
				p = pda_section::quests;
				break;
			default:
				NODEFAULT;
			};
			
			CUIGameCustom* pGameUI = HUD().GetUI()->UIGame();
			pGameUI->PdaMenu->PdaContentsChanged(p);
		}
	}
}

void CActor::AddGameTask(const CInfoPortion *info_portion) const
{
	VERIFY(info_portion);

	if (info_portion->GameTasks().empty())
		return;

	for (TASK_ID_VECTOR::const_iterator it = info_portion->GameTasks().begin();
		 it != info_portion->GameTasks().end(); it++)
	{
		GameTaskManager().GiveGameTaskToActor(*it, 0);
	}
}

void CActor::AddGameNews(GAME_NEWS_DATA &news_data)
{
	GAME_NEWS_VECTOR &news_vector = game_news_registry->registry().objects();
	news_data.receive_time = Level().GetGameTime();
	news_vector.push_back(news_data);

	if (HUD().GetUI())
	{
		HUD().GetUI()->UIMainIngameWnd->ReceiveNews(&news_data);
		
		if (CUIGameCustom* pGameUI = HUD().GetUI()->UIGame())		
			pGameUI->PdaMenu->PdaContentsChanged(pda_section::news);
	}
}

bool CActor::OnReceiveInfo(shared_str info_id) const
{
	if (!CInventoryOwner::OnReceiveInfo(info_id))
		return false;

#ifdef DEBUG
	Msg("- actor: %s received: [%s]", Name(), info_id.c_str());
#endif

	callback(GameObject::eInventoryInfo)(lua_game_object(), *info_id);

	if (Actor() != this)
		return true;

	CInfoPortion info_portion;
	info_portion.Load(info_id);

	AddEncyclopediaArticle(&info_portion);
	AddGameTask(&info_portion);

	if (!HUD().GetUI())
		return false;

	CUIGameCustom* pGameUi = smart_cast<CUIGameCustom*>(HUD().GetUI()->UIGame());
	if (!pGameUi)
		return false;

	if (pGameUi->TalkMenu->IsShown())	
		pGameUi->TalkMenu->NeedUpdateQuestions();	

	return true;
}

void CActor::OnDisableInfo(shared_str info_id) const
{
	CInventoryOwner::OnDisableInfo(info_id);

	if (Actor() != this)
		return;

	if (!HUD().GetUI())
		return;

	CUIGameCustom* pGameUi = smart_cast<CUIGameCustom*>(HUD().GetUI()->UIGame());
	if (!pGameUi)
		return;

	if (pGameUi->TalkMenu->IsShown())
		pGameUi->TalkMenu->NeedUpdateQuestions();
}

void CActor::ReceivePhrase(DIALOG_SHARED_PTR &phrase_dialog)
{
	CUIGameCustom* pGameUi = smart_cast<CUIGameCustom*>(HUD().GetUI()->UIGame());
	if (!pGameUi)
		return;

	if (pGameUi->TalkMenu->IsShown())
		pGameUi->TalkMenu->NeedUpdateQuestions();

	CPhraseDialogManager::ReceivePhrase(phrase_dialog);
}

void CActor::UpdateAvailableDialogs(CPhraseDialogManager *partner)
{
	m_AvailableDialogs.clear();
	m_CheckedDialogs.clear();

	if (CInventoryOwner::m_known_info_registry->registry().objects_ptr())
	{
		for (KNOWN_INFO_VECTOR::const_iterator it = CInventoryOwner::m_known_info_registry->registry().objects_ptr()->begin();
			 CInventoryOwner::m_known_info_registry->registry().objects_ptr()->end() != it; ++it)
		{
			//���������� ������� ���������� � ������� �� ��������
			CInfoPortion info_portion;
			info_portion.Load((*it).info_id);

			for (u32 i = 0; i < info_portion.DialogNames().size(); i++)
				AddAvailableDialog(*info_portion.DialogNames()[i], partner);
		}
	}

	//�������� ��������� ������ �����������
	CInventoryOwner *pInvOwnerPartner = smart_cast<CInventoryOwner *>(partner);
	VERIFY(pInvOwnerPartner);

	for (u32 i = 0; i < pInvOwnerPartner->CharacterInfo().ActorDialogs().size(); i++)
		AddAvailableDialog(pInvOwnerPartner->CharacterInfo().ActorDialogs()[i], partner);

	CPhraseDialogManager::UpdateAvailableDialogs(partner);
}

void CActor::TryToTalk()
{
	VERIFY(m_pPersonWeLookingAt);

	if (!IsTalking())
	{
		RunTalkDialog(m_pPersonWeLookingAt);
	}
}

void CActor::RunTalkDialog(CInventoryOwner *talk_partner)
{
	//���������� ���������� � ����
	if (talk_partner->OfferTalk(this))
	{
		StartTalk(talk_partner);

		if (CUIGameCustom* pGameUI = smart_cast<CUIGameCustom*>(HUD().GetUI()->UIGame()))
		{
			if (pGameUI->MainInputReceiver())
				Game().StartStopMenu(pGameUI->MainInputReceiver(), true);

			pGameUI->StartTalk();
		}
	}
}

void CActor::StartTalk(CInventoryOwner *talk_partner)
{
	CGameObject *GO = smart_cast<CGameObject *>(talk_partner);
	VERIFY(GO);
	//�������� ���������� � ��������
	//.	UpdateContact(GO->ID());

	CInventoryOwner::StartTalk(talk_partner);
}

void CActor::NewPdaContact(CInventoryOwner *pInvOwner)
{
	if (g_dedicated_server)
		return;

	bool b_alive = !!(smart_cast<CEntityAlive *>(pInvOwner))->g_Alive();
	HUD().GetUI()->UIMainIngameWnd->AnimateContacts(b_alive);

	Level().MapManager().AddRelationLocation(pInvOwner);

	if (HUD().GetUI())
	{
		if (CUIGameCustom* pGameUI = HUD().GetUI()->UIGame())
			pGameUI->PdaMenu->PdaContentsChanged(pda_section::contacts);
	}
}

void CActor::LostPdaContact(CInventoryOwner *pInvOwner)
{	
	if (CGameObject* GO = smart_cast<CGameObject*>(pInvOwner))
	{
		for (int t = ALife::eRelationTypeFriend; t < ALife::eRelationTypeLast; ++t)
		{
			ALife::ERelationType tt = (ALife::ERelationType)t;
			Level().MapManager().RemoveMapLocation(RELATION_REGISTRY().GetSpotName(tt), GO->ID());
		}

		Level().MapManager().RemoveMapLocation("deadbody_location", GO->ID());
	}

	if (HUD().GetUI())
	{		
		if (CUIGameCustom* pGameUI = HUD().GetUI()->UIGame())		
			pGameUI->PdaMenu->PdaContentsChanged(pda_section::contacts);		
	}
}

void CActor::AddGameNews_deffered(GAME_NEWS_DATA &news_data, u32 delay)
{
	GAME_NEWS_DATA *d = xr_new<GAME_NEWS_DATA>(news_data);
	//*d = news_data;
	m_defferedMessages.push_back(SDefNewsMsg());
	m_defferedMessages.back().news_data = d;
	m_defferedMessages.back().time = Device.dwTimeGlobal + delay;
	std::sort(m_defferedMessages.begin(), m_defferedMessages.end());
}
void CActor::UpdateDefferedMessages()
{
	while (m_defferedMessages.size())
	{
		SDefNewsMsg &M = m_defferedMessages.back();
		if (M.time <= Device.dwTimeGlobal)
		{
			AddGameNews(*M.news_data);
			xr_delete(M.news_data);
			m_defferedMessages.pop_back();
		}
		else
			break;
	}
}

bool CActor::OnDialogSoundHandlerStart(CInventoryOwner *inv_owner, LPCSTR phrase)
{
	CAI_Trader *trader = smart_cast<CAI_Trader *>(inv_owner);
	if (!trader)
		return false;

	trader->dialog_sound_start(phrase);
	return true;
}
bool CActor::OnDialogSoundHandlerStop(CInventoryOwner *inv_owner)
{
	CAI_Trader *trader = smart_cast<CAI_Trader *>(inv_owner);
	if (!trader)
		return false;

	trader->dialog_sound_stop();
	return true;
}
