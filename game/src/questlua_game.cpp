#include "stdafx.h"
#include "questlua.h"
#include "questmanager.h"
#include "desc_client.h"
#include "char.h"
#include "item_manager.h"
#include "item.h"
#include "cmd.h"
#include "packet.h"

#ifdef ENABLE_DICE_SYSTEM
#include "party.h"
#endif

#ifdef ENABLE_EVENT_MANAGER
#include "char_manager.h"
#endif

#include "maintenance.h"

#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

extern ACMD(do_in_game_mall);

namespace quest
{
	ALUA(game_set_event_flag)
	{
		CQuestManager & q = CQuestManager::instance();

		if (lua_isstring(L,1) && lua_isnumber(L, 2))
			q.RequestSetEventFlag(lua_tostring(L,1), (int)lua_tonumber(L,2));

		return 0;
	}

	ALUA(game_get_event_flag)
	{
		CQuestManager& q = CQuestManager::instance();

		if (lua_isstring(L,1))
			lua_pushnumber(L, q.GetEventFlag(lua_tostring(L,1)));
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	ALUA(game_request_make_guild)
	{
		CQuestManager& q = CQuestManager::instance();
		LPDESC d = q.GetCurrentCharacterPtr()->GetDesc();
		if (d)
		{
			BYTE header = HEADER_GC_REQUEST_MAKE_GUILD;
			d->Packet(&header, 1);
		}
		return 0;
	}

	ALUA(game_get_safebox_level)
	{
		CQuestManager& q = CQuestManager::instance();
		lua_pushnumber(L, q.GetCurrentCharacterPtr()->GetSafeboxSize()/SAFEBOX_PAGE_SIZE);
		return 1;
	}

	ALUA(game_set_safebox_level)
	{
		CQuestManager& q = CQuestManager::instance();

		//q.GetCurrentCharacterPtr()->ChangeSafeboxSize(3*(int)lua_tonumber(L,-1));
		TSafeboxChangeSizePacket p;
		p.dwID = q.GetCurrentCharacterPtr()->GetDesc()->GetAccountTable().id;
		p.bSize = (int)lua_tonumber(L,-1);
		db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_CHANGE_SIZE,  q.GetCurrentCharacterPtr()->GetDesc()->GetHandle(), &p, sizeof(p));

		q.GetCurrentCharacterPtr()->SetSafeboxSize(SAFEBOX_PAGE_SIZE * (int)lua_tonumber(L,-1));
		return 0;
	}

	ALUA(game_open_safebox)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		ch->SetSafeboxOpenPosition();
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeSafeboxPassword");
		return 0;
	}

	ALUA(game_open_mall)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		ch->SetSafeboxOpenPosition();
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeMallPassword");
		return 0;
	}

	ALUA(game_drop_item)
	{
		// Syntax: pc.give_item2(50050, 1)

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		DWORD item_vnum = (DWORD) lua_tonumber(L, 1);
		int count = (int) lua_tonumber(L, 2);
		long x = ch->GetX();
		long y = ch->GetY();

		LPITEM item = ITEM_MANAGER::instance().CreateItem(item_vnum, count);

		if (!item)
		{
			sys_err("cannot create item vnum %d count %d", item_vnum, count);
			return 0;
		}

		PIXEL_POSITION pos;
		pos.x = x + number(-200, 200);
		pos.y = y + number(-200, 200);

		item->AddToGround(ch->GetMapIndex(), pos);
		item->StartDestroyEvent();
#ifdef RENEWAL_PICKUP_AFFECT
		if (ch->CanPickupDirectly())
			ch->PickupItem(item->GetVID());
#endif

		return 0;
	}

	ALUA(game_drop_item_with_ownership)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		LPITEM item = NULL;
		switch (lua_gettop(L))
		{
		case 1:
			item = ITEM_MANAGER::instance().CreateItem((DWORD) lua_tonumber(L, 1));
			break;
		case 2:
		case 3:
			item = ITEM_MANAGER::instance().CreateItem((DWORD) lua_tonumber(L, 1), (int) lua_tonumber(L, 2));
			break;
		default:
			return 0;
		}

		if ( item == NULL )
		{
			return 0;
		}

		int sec = (int) lua_tonumber(L, 3);

		PIXEL_POSITION pos;
		pos.x = ch->GetX() + number(-200, 200);
		pos.y = ch->GetY() + number(-200, 200);

		item->AddToGround(ch->GetMapIndex(), pos);
		item->StartDestroyEvent();
		item->SetOwnership(ch);
#ifdef RENEWAL_PICKUP_AFFECT
		if (ch->CanPickupDirectly())
			ch->PickupItem(item->GetVID());
#endif
		return 0;
	}

#ifdef ENABLE_DICE_SYSTEM
	ALUA(game_drop_item_with_ownership_and_dice)
	{
		LPITEM item = NULL;
		switch (lua_gettop(L))
		{
		case 1:
			item = ITEM_MANAGER::instance().CreateItem((DWORD) lua_tonumber(L, 1));
			break;
		case 2:
		case 3:
			item = ITEM_MANAGER::instance().CreateItem((DWORD) lua_tonumber(L, 1), (int) lua_tonumber(L, 2));
			break;
		default:
			return 0;
		}

		if ( item == NULL )
		{
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch->GetParty())
		{
			FPartyDropDiceRoll f(item, ch);
			f.Process(NULL);
		}

		if (lua_isnumber(L, 3))
		{
			int sec = (int) lua_tonumber(L, 3);
			if (sec <= 0)
			{
				item->SetOwnership( ch );
			}
			else
			{
				item->SetOwnership( ch, sec );
			}
		}
		else
			item->SetOwnership( ch );

		PIXEL_POSITION pos;
		pos.x = ch->GetX() + number(-200, 200);
		pos.y = ch->GetY() + number(-200, 200);

		item->AddToGround(ch->GetMapIndex(), pos);
		item->StartDestroyEvent();
#ifdef RENEWAL_PICKUP_AFFECT
		if (ch->CanPickupDirectly())
			ch->PickupItem(item->GetVID());
#endif
		return 0;
	}
#endif

	ALUA(game_web_mall)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			do_in_game_mall(ch, const_cast<char*>(""), 0, 0);
		}
		return 0;
	}

#if defined(__BL_67_ATTR__)
	ALUA(game_open_67_attr)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (ch)
			ch->Open67Attr();
		
		return 0;
	}
#endif

#ifdef ENABLE_EVENT_MANAGER
	ALUA(game_check_event)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			lua_pushboolean(L, false);
			return 1;
		}
		auto it = CHARACTER_MANAGER::instance().CheckEventIsActive(lua_tonumber(L, 1), lua_tonumber(L, 2));
		lua_pushboolean(L, it != NULL);
		return 1;
	}

	ALUA(game_check_event_expiration_value)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			lua_pushnumber(L, 0);
			return 1;
		}
		auto it = CHARACTER_MANAGER::instance().CheckEventIsActive(lua_tonumber(L, 1), lua_tonumber(L, 2));
		lua_pushnumber(L, it->value[2]);
		return 1;
	}
#endif

#ifdef ENABLE_REWARD_SYSTEM
	ALUA(game_set_reward)
	{
		CQuestManager& qMgr = CQuestManager::instance();
		LPCHARACTER pkChar = qMgr.GetCurrentCharacterPtr();
		if (pkChar)
		{
			if (!lua_isnumber(L, 1))
				return 0;
			CHARACTER_MANAGER::instance().SetRewardData(lua_tonumber(L, 1), pkChar->GetName(), true);
		}
		return 0;
	}
#endif

#if defined(__GEM_MARKET_SYSTEM__)
	ALUA(game_open_gem_c)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		ch->ChatPacket(CHAT_TYPE_COMMAND, "OpenGuiGem");
		return 0;
	}

	ALUA(game_open_gem_m)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (ch->CheckItemsFull() == false)
		{
			//ch->SetGemState("gem_refresh_time", init_gemTime() + (60*60*5));
			ch->RefreshGemItems();
			ch->UpdateItemsGemMarker();
			ch->InfoGemMarker();
		}
		else
		{
			ch->RefreshGemItems();
			ch->InfoGemMarker();
		}

		ch->ChatPacket(CHAT_TYPE_COMMAND, "OpenGuiGemMarket");

		return 0;
	}
#endif

#if defined(__BL_MOVE_COSTUME_ATTR__)
	ALUA(game_open_item_comb)
	{
		const LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch)
			ch->OpenItemComb();

		return 0;
	}
#endif

	ALUA(game_check_maintenance)
	{
		const BYTE gameMode = CMaintenanceManager::Instance().GetGameMode();
		if (!gameMode)
			lua_pushboolean(L, false);

		if (gameMode == GAME_MODE_MAINTENANCE || gameMode == GAME_MODE_LOBBY)
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);

		return 1;
	}

	void RegisterGameFunctionTable()
	{
		luaL_reg game_functions[] =
		{
			{ "get_safebox_level",			game_get_safebox_level			},
			{ "request_make_guild",			game_request_make_guild			},
			{ "set_safebox_level",			game_set_safebox_level			},
			{ "open_safebox",				game_open_safebox				},
			{ "open_mall",					game_open_mall					},
			{ "get_event_flag",				game_get_event_flag				},
			{ "set_event_flag",				game_set_event_flag				},
			{ "drop_item",					game_drop_item					},
			{ "drop_item_with_ownership",	game_drop_item_with_ownership	},
#ifdef ENABLE_DICE_SYSTEM
			{ "drop_item_with_ownership_and_dice",	game_drop_item_with_ownership_and_dice	},
#endif
			{ "open_web_mall",				game_web_mall					},
#ifdef ENABLE_EVENT_MANAGER
			{ "check_event",		game_check_event			},
			{ "check_event_expiration_value",	game_check_event_expiration_value },
#endif
#if defined(__BL_MOVE_COSTUME_ATTR__)
			{ "open_item_comb",				game_open_item_comb				},
#endif
#if defined(__BL_67_ATTR__)
			{ "open_67_attr",				game_open_67_attr				},
#endif

#ifdef ENABLE_REWARD_SYSTEM
			{ "set_reward",		game_set_reward			},
#endif
#if defined(__GEM_MARKET_SYSTEM__)
			{ "open_gem", game_open_gem_c },
			{ "open_gem_market", game_open_gem_m },
#endif
			{ "check_maintenance", game_check_maintenance },
			{ NULL,					NULL				}
		};

		CQuestManager::instance().AddLuaFunctionTable("game", game_functions);
	}
}
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
