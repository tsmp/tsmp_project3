#include "stdafx.h"
#include "actor.h"
#include "customdetector.h"
#include "uigamesp.h"
#include "hudmanager.h"
#include "weapon.h"
#include "artifact.h"
#include "scope.h"
#include "silencer.h"
#include "grenadelauncher.h"
#include "inventory.h"
#include "level.h"
#include "xr_level_controller.h"
#include "FoodItem.h"
#include "ActorCondition.h"
#include "Grenade.h"

#include "CameraLook.h"
#include "CameraFirstEye.h"
#include "holder_custom.h"
#include "ui/uiinventoryWnd.h"
#include "game_base_space.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif
IC BOOL BE(BOOL A, BOOL B)
{
	bool a = !!A;
	bool b = !!B;
	return a == b;
}

void CActor::OnEvent(NET_Packet &P, u16 type)
{
	inherited::OnEvent(P, type);
	CInventoryOwner::OnEvent(P, type);

	u16 id;
	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
	{
		P.r_u16(id);
		CObject *Obj = Level().Objects.net_Find(id);

		if (!Obj)
		{
			Msg("! Error: No object to take/buy [%d]", id);
			break;
		}

		CGameObject *_GO = smart_cast<CGameObject *>(Obj);

		if (!IsGameTypeSingle() && !g_Alive())
		{
			Msg("! WARNING: dead player [%d][%s] can't take items [%d][%s]", ID(), Name(), _GO->ID(),
				_GO->cNameSect().c_str());
			break;
		}

		if (CFoodItem *pFood = smart_cast<CFoodItem *>(Obj))
			pFood->m_eItemPlace = eItemPlaceRuck;

		if (inventory().CanTakeItem(smart_cast<CInventoryItem *>(_GO)))
		{
			Obj->H_SetParent(smart_cast<CObject *>(this));
			inventory().Take(_GO, false, true);

			CUIGameSP *pGameSP = NULL;
			CUI *ui = HUD().GetUI();

			if (ui && ui->UIGame())
			{
				pGameSP = smart_cast<CUIGameSP *>(HUD().GetUI()->UIGame());

				if (Level().CurrentViewEntity() == this)
					HUD().GetUI()->UIGame()->ReInitShownUI();
			}

			//добавить отсоединенный аддон в инвентарь
			if (pGameSP && pGameSP->MainInputReceiver() == pGameSP->InventoryMenu)				
				pGameSP->InventoryMenu->AddItemToBag(smart_cast<CInventoryItem *>(Obj));			

			SelectBestWeapon(Obj);
		}
		else
		{
			if (IsGameTypeSingle())
			{
				NET_Packet P2;
				u_EventGen(P2, GE_OWNERSHIP_REJECT, ID());
				P2.w_u16(u16(Obj->ID()));
				u_EventSend(P2);
			}
			else
			{
				Msg("! ERROR: Actor [%d][%s]  tries to drop on take [%d][%s]", ID(), Name(), _GO->ID(),
					_GO->cNameSect().c_str());
			}
		}
	}
	break;

	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
	{
		P.r_u16(id);
		CObject *Obj = Level().Objects.net_Find(id);

		if (!Obj)
		{
			Msg("! Error: No object to reject/sell [%d]", id);
			break;
		}

		bool just_before_destroy = !P.r_eof() && P.r_u8();
		Obj->SetTmpPreDestroy(just_before_destroy);

		CGameObject *GO = smart_cast<CGameObject *>(Obj);

#ifdef MP_LOGGING
		string64 act;
		xr_strcpy(act, (type == GE_TRADE_SELL) ? "sells" : "rejects");
		Msg("--- Actor [%d][%s]  %s  [%d][%s]", ID(), Name(), act, GO->ID(), GO->cNameSect().c_str());
#endif // MP_LOGGING

		if (!GO->H_Parent())
		{
			Msg("! ERROR: Actor [%d][%s] tries to reject item [%d][%s] that has no parent", ID(), Name(), GO->ID(),
				GO->cNameSect().c_str());
			break;
		}

		if (GO->H_Parent()->ID() != ID())
		{
			CActor *real_parent = smart_cast<CActor *>(GO->H_Parent());
			Msg("! ERROR: Actor [%d][%s] tries to drop not own item [%d][%s], his parent is [%d][%s]", ID(), Name(),
				GO->ID(), GO->cNameSect().c_str(), real_parent->ID(), real_parent->Name());
			break;
		}

		if (inventory().DropItem(smart_cast<CGameObject *>(Obj)) && !Obj->getDestroy())
		{
			Obj->H_SetParent(0, just_before_destroy);
			Level().m_feel_deny.feel_touch_deny(Obj, 1000);
		}

		SelectBestWeapon(Obj);

		if (Level().CurrentViewEntity() == this && HUD().GetUI() && HUD().GetUI()->UIGame())
			HUD().GetUI()->UIGame()->ReInitShownUI();
	}
	break;

	case GE_INV_ACTION:
	{
		s32 cmd;
		P.r_s32(cmd);
		u32 flags;
		P.r_u32(flags);
		s32 ZoomRndSeed = P.r_s32();
		s32 ShotRndSeed = P.r_s32();

		if (!IsGameTypeSingle() && !g_Alive())
		{
			Msg("! WARNING: dead player tries to rize inventory action");
			break;
		}

		if (flags & CMD_START)
		{
			if (cmd == kWPN_ZOOM)
				SetZoomRndSeed(ZoomRndSeed);

			if (cmd == kWPN_FIRE)
				SetShotRndSeed(ShotRndSeed);

			IR_OnKeyboardPress(cmd);
		}
		else
			IR_OnKeyboardRelease(cmd);
	}
	break;

	case GEG_PLAYER_ITEM2SLOT:
	case GEG_PLAYER_ITEM2BELT:
	case GEG_PLAYER_ITEM2RUCK:
	case GEG_PLAYER_ITEM_EAT:
	case GEG_PLAYER_ACTIVATEARTEFACT:
	{
		P.r_u16(id);
		CObject *Obj = Level().Objects.net_Find(id);
		CGameObject* GO = smart_cast<CGameObject*>(Obj);

		if (!Obj)
		{
			Msg("! ERROR: GEG_PLAYER_ITEM_USE: Object not found. object_id = [%d]", id);
			break;
		}

		if (!IsGameTypeSingle() && !g_Alive())
		{
			Msg("! WARNING: dead player [%d][%s] can't use items [%d][%s]", ID(), Name(), Obj->ID(),
				Obj->cNameSect().c_str());
			break;
		}

		if (Obj->getDestroy())
		{
#ifdef DEBUG
			Msg("! something to destroyed object - %s[%d]0x%X", *Obj->cName(), id, smart_cast<CInventoryItem *>(Obj));
#endif
			break;
		}

		if (!GO->H_Parent())
		{
			Msg("! ERROR: Actor [%d][%s] tries to manipulate with item [%d][%s] that has no parent", ID(), Name(), GO->ID(),
				GO->cNameSect().c_str());
			break;
		}

		if (GO->H_Parent()->ID() != ID())
		{
			CActor* real_parent = smart_cast<CActor*>(GO->H_Parent());
			Msg("! ERROR: Actor [%d][%s] tries to manipulate with item he doesnt own [%d][%s], his parent is [%d][%s]", ID(), Name(),
				GO->ID(), GO->cNameSect().c_str(), real_parent->ID(), real_parent->Name());
			break;
		}

		switch (type)
		{
		case GEG_PLAYER_ITEM2SLOT:
			inventory().Slot(smart_cast<CInventoryItem *>(Obj));
			break;
		case GEG_PLAYER_ITEM2BELT:
			inventory().Belt(smart_cast<CInventoryItem *>(Obj));
			break;
		case GEG_PLAYER_ITEM2RUCK:
			inventory().Ruck(smart_cast<CInventoryItem *>(Obj));
			break;
		case GEG_PLAYER_ITEM_EAT:
			inventory().Eat(smart_cast<CInventoryItem *>(Obj));
			break;
		case GEG_PLAYER_ACTIVATEARTEFACT:
		{
			CArtefact *pArtefact = smart_cast<CArtefact *>(Obj);

			if (!pArtefact)
			{
				Msg("! ERROR: GEG_PLAYER_ACTIVATEARTEFACT: Artefact not found. artefact_id = [%d], Player[%s]", id, this->Name());
				break;
			}

			pArtefact->ActivateArtefact();
		}
		break;
		}
	}
	break;
	case GEG_PLAYER_ACTIVATE_SLOT:
	{
		u32 slot_id;
		P.r_u32(slot_id);

		inventory().Activate(slot_id);
	}
	break;

	case GEG_PLAYER_WEAPON_HIDE_STATE:
	{
		u32 State = P.r_u32();
		BOOL Set = !!P.r_u8();
		inventory().SetSlotsBlocked((u16)State, !!Set);
	}
	break;

	case GE_MOVE_ACTOR:
	{
		Fvector NewPos, NewRot;
		P.r_vec3(NewPos);
		P.r_vec3(NewRot);

		MoveActor(NewPos, NewRot);
	}
	break;

	case GE_ACTOR_MAX_POWER:
	{
		conditions().MaxPower();
		conditions().ClearWounds();
		ClearBloodWounds();
	}
	break;

	case GEG_PLAYER_ATTACH_HOLDER:
	{
		u32 id = P.r_u32();
		CObject *O = Level().Objects.net_Find(id);

		if (!O)
		{
			Msg("! Error: No object to attach holder [%d]", id);
			break;
		}
		VERIFY(m_holder == NULL);
		CHolderCustom *holder = smart_cast<CHolderCustom *>(O);
		if (!holder->Engaged())
			use_Holder(holder);
	}
	break;

	case GEG_PLAYER_DETACH_HOLDER:
	{
		if (!m_holder)
			break;

		u32 id = P.r_u32();
		CGameObject *GO = smart_cast<CGameObject *>(m_holder);
		VERIFY(id == GO->ID());
		use_Holder(NULL);
	}
	break;

	case GEG_PLAYER_PLAY_HEADSHOT_PARTICLE:	
		OnPlayHeadShotParticle(P);
		break;
	}
}

void CActor::MoveActor(Fvector NewPos, Fvector NewDir)
{
	Fmatrix M = XFORM();
	M.translate(NewPos);
	r_model_yaw = NewDir.y;
	r_torso.yaw = NewDir.y;
	r_torso.pitch = -NewDir.x;
	unaffected_r_torso.yaw = r_torso.yaw;
	unaffected_r_torso.pitch = r_torso.pitch;
	unaffected_r_torso.roll = 0; //r_torso.roll;

	r_torso_tgt_roll = 0;
	cam_Active()->Set(-unaffected_r_torso.yaw, unaffected_r_torso.pitch, unaffected_r_torso.roll);
	ForceTransform(M);

	m_bInInterpolation = false;
}