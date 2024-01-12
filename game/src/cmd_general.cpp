#include "stdafx.h"
#ifdef __FreeBSD__
#include <md5.h>
#else
#include "../../libthecore/include/xmd5.h"
#endif

#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "motion.h"
#include "packet.h"
#include "affect.h"
#include "pvp.h"
#include "start_position.h"
#include "party.h"
#include "guild_manager.h"
#include "p2p.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "war_map.h"
#include "questmanager.h"
#include "item_manager.h"
#include "monarch.h"
#include "mob_manager.h"
#include "item.h"
#include "arena.h"
#include "buffer_manager.h"
#include "unique_item.h"
#include "threeway_war.h"
#include "log.h"
#include "../../common/VnumHelper.h"
#include "../../common/PulseManager.h"
#include "shop_manager.h"
#ifdef __MAINTENANCE__
#include "maintenance.h"
#endif
#include "dungeon.h"
#include "blend_item.h"

ACMD(do_auto_dews)
{
	if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::MultipleBuy, std::chrono::seconds(5)))
		return ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to wait %d s! Try again later."), 5);

	if (ch->IsObserverMode())
		return;

	if (ch->IsDead() || ch->IsStun())
		return;

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	int quest_flag = ch->GetQuestFlag("use_auto_dews.active");
	if (quest_flag == 0)
	{
		ch->SetQuestFlag("use_auto_dews.active", 1);
		quest_flag = 1;
	}

	for (int i; i < INVENTORY_MAX_NUM; i++)
	{
		auto item = ch->GetInventoryItem(i);

		if (item && item->IsBlendItem() && !item->IsExtendedBlend(item->GetVnum()))
		{
			if (item->GetVnum() == 27874)
				continue;

			long is_active = item->GetSocket(3);

			if (quest_flag == 2 && is_active == 1)
				continue;

			if (quest_flag == 1 && is_active == 0)
				continue;

			if (Blend_Item_find(item->GetVnum()))
			{
				int		affect_type		= AFFECT_BLEND;
				int		apply_type		= aApplyInfo[item->GetSocket(0)].bPointType;
				int		apply_value		= item->GetSocket(1);
				int		apply_duration	= item->GetSocket(2) <= 0 ? INFINITE_AFFECT_DURATION : item->GetSocket(2);
	
				if (quest_flag == 2)
				{
					switch(item->GetVnum())
					{
						case 50830:
						{
							if (ch->FindAffect(AFFECT_BLEND_MONSTERS))
								continue;
						};
						break;
						case 50821:
						{
							if (ch->FindAffect(AFFECT_BLEND_POTION_1))
								continue;
						};
						break;
						case 50822:
						{
							if (ch->FindAffect(AFFECT_BLEND_POTION_2))
								continue;
						};
						break;
						case 50823:
						{
							if (ch->FindAffect(AFFECT_BLEND_POTION_3))
								continue;
						};
						break;
						case 50824:
						{
							if (ch->FindAffect(AFFECT_BLEND_POTION_4))
								continue;
						};
						break;
						case 50825:
						{
							if (ch->FindAffect(AFFECT_BLEND_POTION_5))
								continue;
						};
						break;
						case 50826:
						{
							if (ch->FindAffect(AFFECT_BLEND_POTION_6))
								continue;
						};
						break;
						case 39017:
						case 71027:
						{
							if (ch->FindAffect(AFFECT_DRAGON_GOD_1))
								continue;
						};
						break;
						case 39018:
						case 71028:
						{
							if (ch->FindAffect(AFFECT_DRAGON_GOD_2))
								continue;
						};
						break;
						case 39019:
						case 71029:
						{
							if (ch->FindAffect(AFFECT_DRAGON_GOD_3))
								continue;
						};
						break;
						case 39020:
						case 71030:
						{
							if (ch->FindAffect(AFFECT_DRAGON_GOD_4))
								continue;
						};
						break;
						case 39024:
						case 71044:
						{
							if (ch->FindAffect(AFFECT_CRITICAL))
								continue;
						};
						break;
						case 39025:
						case 71045:
						{
							if (ch->FindAffect(AFFECT_PENETRATE))
								continue;
						};
						break;
						case 50817:
						{
							if (ch->FindAffect(AFFECT_ATT_WATER))
								continue;
						};
						break;
						case 50818:
						{
							if (ch->FindAffect(AFFECT_DEF_WATER))
								continue;
						};
						break;
						case 27863:
						{
							if (ch->FindAffect(AFFECT_FISH1))
								continue;
						};
						break;
						case 27864:
						{
							if (ch->FindAffect(AFFECT_FISH2))
								continue;
						};
						break;
						case 27865:
						{
							if (ch->FindAffect(AFFECT_FISH3))
								continue;
						};
						break;
						case 27866:
						{
							if (ch->FindAffect(AFFECT_FISH4))
								continue;
						};
						break;
						case 27867:
						{
							if (ch->FindAffect(AFFECT_FISH5))
								continue;
						};
						break;
						case 27868:
						{
							if (ch->FindAffect(AFFECT_FISH6))
								continue;
						};
						break;
						case 27869:
						{
							if (ch->FindAffect(AFFECT_FISH7))
								continue;
						};
						break;
						case 27870:
						{
							if (ch->FindAffect(AFFECT_FISH8))
								continue;
						};
						break;
						case 27871:
						{
							if (ch->FindAffect(AFFECT_FISH9))
								continue;
						};
						break;
						case 27872:
						{
							if (ch->FindAffect(AFFECT_FISH10))
								continue;
						};
						break;
						case 27873:
						{
							if (ch->FindAffect(AFFECT_FISH11))
								continue;
						};
						break;
						case 27875:
						{
							if (ch->FindAffect(AFFECT_FISH12))
								continue;
						};
						break;
						case 27883:
						{
							if (ch->FindAffect(AFFECT_FISH13))
								continue;
						};
						break;
						case 27878:
						{
							if (ch->FindAffect(AFFECT_FISH14))
								continue;
						};
						break;
					}
				}

				ch->UseItemEx(item);	
			}
		}
	}

	if (quest_flag == 2)
		ch->SetQuestFlag("use_auto_dews.active", 1);
	else
		ch->SetQuestFlag("use_auto_dews.active", 2);
}

#ifdef __SAVE_BLOCK_ATTR__
ACMD(do_save_block_attr)
{
	if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::MultipleBuy, std::chrono::milliseconds(3000)))
		return ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to wait %d s! Try again later."), 3);

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());
	if (pkSectreeMap)
	{
		LPSECTREE tree = pkSectreeMap->Find(ch->GetX(), ch->GetY());
		if (tree)
		{
			const DWORD dwAttr = tree->GetAttribute(ch->GetX(), ch->GetY());
			if (!IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT | ATTR_WATER))
				return;
		}
	}
	const long lastX = ch->GetXLast(), lastY = ch->GetYLast(),lastZ = ch->GetZLast();
	if(lastX != 0 && lastX != 0)
		ch->Show(ch->GetMapIndex(), lastX, lastY, lastZ);
}
#endif

ACMD(do_multiple_buy)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 3) { return; }

	if (ch->PreventTradeWindow(WND_SHOPOWNER | WND_SHOP, true))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	BYTE pos;
	str_to_number(pos, vecArgs[1].c_str());

	int count;
	str_to_number(count, vecArgs[2].c_str());

	if (!count)
		return;

	if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::MultipleBuy, std::chrono::milliseconds(2000)))
		return ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("퀘스트를 로드하는 중입니다. 잠시만 기다려 주십시오."));;

	if (count > 50)
		return;

	CShopManager::instance().Buy(ch, pos, count);
}

ACMD(do_sell)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 3) { return; }

	if (ch->PreventTradeWindow(WND_SHOPOWNER | WND_SHOP, true))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	WORD pos;
	str_to_number(pos, vecArgs[1].c_str());

	DWORD count;
	str_to_number(count, vecArgs[2].c_str());

	if (!count)
		return;

	CShopManager::instance().Sell(ch, pos, count);
}

ACMD(do_single_bonus_affect)
{
	int skilllevel = ch->GetSkillLevel(SKILL_LEADERSHIP);

	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 1) { return; }

 	TPassiveSkillStats affectTable;

	if (ch->GetProtectTime("leadership") > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to wait %d s! Try again later."), ch->GetProtectTime("leadership") - get_global_time());
		return;
	}
	if (ch->GetGMLevel() != GM_IMPLEMENTOR)
		ch->SetProtectTime("leadership", get_global_time() + 5);

	CAffect* affect = ch->FindAffect(AFFECT_PARTY);
	if (affect)
		ch->RemoveAffect(affect);

	if (vecArgs[1] == "a")
	{
		CAffect* affectValue = ch->FindAffect(AFFECT_PARTY, PassiveSkillStats[skilllevel].bonus1type);
		if (affectValue)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This affect is already active."));

		ch->AddAffect(AFFECT_PARTY, PassiveSkillStats[skilllevel].bonus1type, PassiveSkillStats[skilllevel].bonus1value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your new leadership bonus is active now."));	
		if (test_server)
			ch->ChatPacket(CHAT_TYPE_INFO, "New Leader Bonus: BONUS TYPE %d BONUS VALUE %d TIME %d LEVEL %d", PassiveSkillStats[skilllevel].bonus1type, PassiveSkillStats[skilllevel].bonus1value, INFINITE_AFFECT_DURATION, skilllevel);
	}
	else if (vecArgs[1] == "b")
	{
		CAffect* affectValue = ch->FindAffect(AFFECT_PARTY, PassiveSkillStats[skilllevel].bonus2type);
		if (affectValue)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This affect is already active."));
			return;
		}

		ch->AddAffect(AFFECT_PARTY, PassiveSkillStats[skilllevel].bonus2type, PassiveSkillStats[skilllevel].bonus2value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your new leadership bonus is active now."));
		if (test_server)
			ch->ChatPacket(CHAT_TYPE_INFO, "New Leader Bonus: BONUS TYPE %d BONUS VALUE %d TIME %d LEVEL %d", PassiveSkillStats[skilllevel].bonus2type, PassiveSkillStats[skilllevel].bonus2value, INFINITE_AFFECT_DURATION, skilllevel);
	}
	else if (vecArgs[1] == "c")
	{
		CAffect* affectValue = ch->FindAffect(AFFECT_PARTY, PassiveSkillStats[skilllevel].bonus3type);
		if (affectValue)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This affect is already active."));
			return;
		}
			
		ch->AddAffect(AFFECT_PARTY, PassiveSkillStats[skilllevel].bonus3type, PassiveSkillStats[skilllevel].bonus3value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your new leadership bonus is active now."));
		if (test_server)
			ch->ChatPacket(CHAT_TYPE_INFO, "New Leader Bonus: BONUS TYPE %d BONUS VALUE %d TIME %d LEVEL %d", PassiveSkillStats[skilllevel].bonus3type, PassiveSkillStats[skilllevel].bonus3value, INFINITE_AFFECT_DURATION, skilllevel);
	}
	else if (vecArgs[1] == "d")
	{
		CAffect* affectValue = ch->FindAffect(AFFECT_PARTY, PassiveSkillStats[skilllevel].bonus4type);
		if (affectValue)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This affect is already active."));
			return;
		}

		ch->AddAffect(AFFECT_PARTY, PassiveSkillStats[skilllevel].bonus4type, PassiveSkillStats[skilllevel].bonus4value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your new leadership bonus is active now."));
		if (test_server)
			ch->ChatPacket(CHAT_TYPE_INFO, "New Leader Bonus: BONUS TYPE %d BONUS VALUE %d TIME %d LEVEL %d", PassiveSkillStats[skilllevel].bonus4type, PassiveSkillStats[skilllevel].bonus4value, INFINITE_AFFECT_DURATION, skilllevel);

	}
	else if (vecArgs[1] == "e")
	{
		CAffect* affectValue = ch->FindAffect(AFFECT_PARTY, PassiveSkillStats[skilllevel].bonus5type);
		if (affectValue)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This affect is already active."));
			return;
		}

		ch->AddAffect(AFFECT_PARTY, PassiveSkillStats[skilllevel].bonus5type, PassiveSkillStats[skilllevel].bonus5value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your new leadership bonus is active now."));
		if (test_server)
			ch->ChatPacket(CHAT_TYPE_INFO, "New Leader Bonus: BONUS TYPE %d BONUS VALUE %d TIME %d LEVEL %d", PassiveSkillStats[skilllevel].bonus5type, PassiveSkillStats[skilllevel].bonus5value, INFINITE_AFFECT_DURATION, skilllevel);

	}
	else if (vecArgs[1] == "f")
	{	
		CAffect* affectValue = ch->FindAffect(AFFECT_PARTY, PassiveSkillStats[skilllevel].bonus6type);
		if (affectValue)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This affect is already active."));
			return;
		}

		ch->AddAffect(AFFECT_PARTY, PassiveSkillStats[skilllevel].bonus6type, PassiveSkillStats[skilllevel].bonus6value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your new leadership bonus is active now."));
		if (test_server)
			ch->ChatPacket(CHAT_TYPE_INFO, "New Leader Bonus: BONUS TYPE %d BONUS VALUE %d TIME %d LEVEL %d", PassiveSkillStats[skilllevel].bonus6type, PassiveSkillStats[skilllevel].bonus6value, INFINITE_AFFECT_DURATION, skilllevel);
	}
}

ACMD(do_use_item_stack)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 3) { 
		if (test_server)
			ch->ChatPacket(CHAT_TYPE_INFO, "/use_item_stack cell count");
		return; 
	}

	int cell = 0;
	str_to_number(cell, vecArgs[1].c_str());

	int count = 1;
	str_to_number(count, vecArgs[2].c_str());

	if (count > 100)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Maximum amount is 100 at the same time."));
		return;
	}

	if (ch->GetProtectTime("do_use_item_stack") > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to wait %d s! Try again later."), ch->GetProtectTime("do_use_item_stack") - get_global_time());
		return;
	}
	ch->SetProtectTime("do_use_item_stack", get_global_time() + 1);

	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		for (int i = 0; i < count; i++)
		{
			ch->UseItem(TItemPos (INVENTORY, cell));
		}
	}	
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템이 없어서 착용할 수 없어."));
}

ACMD(do_user_horse_ride)
{
#ifdef __RENEWAL_MOUNT__
	if (ch->MountBlockMap())
		return;
	time_t now = get_global_time();
	if (ch->GetProtectTime("mount.ride") > now)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("684 %d"), ch->GetProtectTime("mount.ride") - now);
		return;
	}
	ch->SetProtectTime("mount.ride", now + 1);
#endif

	if (ch->IsObserverMode())
		return;

	if (ch->IsDead() || ch->IsStun())
		return;

	if (ch->IsHorseRiding() == false)
	{
		if (ch->GetMountVnum())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 탈것을 이용중입니다."));
			return;
		}

		if (ch->GetHorse() == NULL)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 먼저 소환해주세요."));
			return;
		}

		ch->StartRiding();
	}
	else
	{
		ch->StopRiding();
	}
}

ACMD(do_anti_exp)
{
	if (!ch)
		return;
		
	if (ch->GetProtectTime("anti_exp") > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to wait %d s! Try again later."), ch->GetProtectTime("anti_exp") - get_global_time());
		return;
	}
	ch->SetProtectTime("anti_exp", get_global_time() + 5);

	if (!ch->HasFrozenEXP())
	{
		ch->FreezeEXP();
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have frozen your experience."));
	}
	else
	{
		ch->UnfreezeEXP();
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have unfrozen your experience."));
	}
}

ACMD(do_unstack_time)
{
	const char *line;
	char arg1[256], arg2[256], arg3[256];
	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Wrong command use.");
		return;
	}

	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	int timeToConvert = 0;
	WORD cell = 0;
	WORD destCell = 0;

	str_to_number(cell, arg1);
	str_to_number(timeToConvert, arg2);
	str_to_number(destCell, arg3);

	LPITEM item = ch->GetInventoryItem(cell);

	if (timeToConvert == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cant use this on this item!"));
		return;
	}
	
	if (item == NULL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No item found.");
		return;
	}

	if (item->GetType() != ITEM_BLEND)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cant use this on this item!"));
		return;
	}

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	if (item->isLocked())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cant use this on this item!"));
		return;
	}

	if (item->GetSocket(3) == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cant use this on this item!"));
		return;
	}

	if (item->IsExtendedBlend(item->GetVnum()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cant use this on this item!"));
		return;
	}

	int iEmptyPosition = ch->GetEmptyInventoryFromIndex(destCell, item->GetSize());
	if (iEmptyPosition == -1)
		return;

	int originalTime = item->GetSocket(2);
	
	int resultTime = originalTime - timeToConvert;
	if (resultTime < 60)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cant use this on this item!"));
		return;
	}

	auto newItem = ITEM_MANAGER::Instance().CreateItem(item->GetVnum(), 1);
	item->SetSocket(2, resultTime);
	newItem->SetSocket(0, item->GetSocket(0));
	newItem->SetSocket(1, item->GetSocket(1));
	newItem->SetSocket(2, timeToConvert);

	newItem->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPosition));
}

ACMD(do_split_items) //SPLIT ITEMS
{
	if (!ch)
		return;
	
	const char *line;
	char arg1[256], arg2[256], arg3[256];
	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));
	
	if (!*arg1 || !*arg2 || !*arg3)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Wrong command use.");
		return;
	}
	
	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}
	
	DWORD count = 0;
	WORD cell = 0;
	WORD destCell = 0;
	
	str_to_number(cell, arg1);
	str_to_number(count, arg2);
	str_to_number(destCell, arg3);
	
	LPITEM item = ch->GetInventoryItem(cell);
	if (item != NULL)
	{
		DWORD itemCount = item->GetCount();
		while (itemCount > 0)
		{
			if (count > itemCount)
				count = itemCount;

			int iEmptyPosition;

			if (destCell >= SKILL_BOOK_INVENTORY_SLOT_START && destCell < SKILL_BOOK_INVENTORY_SLOT_END)
			{
				iEmptyPosition = ch->GetEmptySkillBookInventoryFromIndex(destCell, item->GetSize());
			}
			else if (destCell >= UPGRADE_ITEMS_INVENTORY_SLOT_START && destCell < UPGRADE_ITEMS_INVENTORY_SLOT_END)
			{
				iEmptyPosition = ch->GetEmptyUpgradeInventoryFromIndex(destCell, item->GetSize());
			}
			else if (destCell >= STONE_INVENTORY_SLOT_START && destCell < STONE_INVENTORY_SLOT_END)
			{
				iEmptyPosition = ch->GetEmptyStoneInventoryFromIndex(destCell, item->GetSize());
			}
			else if (destCell >= GIFT_BOX_INVENTORY_SLOT_START && destCell < GIFT_BOX_INVENTORY_SLOT_END)
			{
				iEmptyPosition = ch->GetEmptyGiftBoxInventoryFromIndex(destCell, item->GetSize());
			}
			else if (destCell >= SWITCH_INVENTORY_SLOT_START && destCell < SWITCH_INVENTORY_SLOT_END)
			{
				iEmptyPosition = ch->GetEmptySwitchInventoryFromIndex(destCell, item->GetSize());
			}
			else if (destCell >= COSTUME_INVENTORY_SLOT_START && destCell < COSTUME_INVENTORY_SLOT_END)
			{
				iEmptyPosition = ch->GetEmptyCostumeInventoryFromIndex(destCell, item->GetSize());
			}
			else {
				iEmptyPosition = ch->GetEmptyInventoryFromIndex(destCell, item->GetSize());
			}

			if (iEmptyPosition == -1)
				break;
			
			itemCount -= count;
			ch->MoveItem(TItemPos(INVENTORY, cell), TItemPos(INVENTORY, iEmptyPosition), count);
		}
	}
}

#ifdef ENABLE_WIKI
ACMD(do_wiki_server)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "warp")
	{
		if (vecArgs.size() < 4) { return; }
		long x, y;
		if (!str_to_number(x, vecArgs[2].c_str()) || !str_to_number(y, vecArgs[3].c_str()))
			return;
		const std::vector<std::pair<long, long>> m_safeWarpLocations = {
			{599400, 756300},
		};
		if (std::find(m_safeWarpLocations.begin(), m_safeWarpLocations.end(), std::make_pair(x, y)) == m_safeWarpLocations.end())
			return;
		ch->WarpSet(x, y);
	}
}
#endif

ACMD(do_user_horse_back)
{
#ifdef __RENEWAL_MOUNT__
	if (ch->MountBlockMap())
		return;
#endif

	if (ch->GetHorse() != NULL)
	{
		ch->HorseSummon(false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 돌려보냈습니다."));
	}
	else if (ch->IsHorseRiding() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말에서 먼저 내려야 합니다."));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 먼저 소환해주세요."));
	}
}

ACMD(do_user_horse_feed)
{
#ifdef __RENEWAL_MOUNT__
	if (ch->MountBlockMap())
		return;
#endif

	if (ch->GetMyShop())
		return;

	if (ch->GetHorse() == NULL)
	{
		if (ch->IsHorseRiding() == false)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 먼저 소환해주세요."));
		else
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 탄 상태에서는 먹이를 줄 수 없습니다."));
		return;
	}

	DWORD dwFood = ch->GetHorseGrade() + 50054 - 1;
	BYTE bLocale = ch->GetLanguage();

	if (ch->CountSpecifyItem(dwFood) > 0)
	{
		ch->RemoveSpecifyItem(dwFood, 1);
		ch->FeedHorse();
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말에게 %s%s 주었습니다."),
			LC_ITEM_NAME(dwFood, bLocale),
			g_iUseLocale ? "" : under_han(LC_ITEM_NAME(dwFood, bLocale)) ? LC_TEXT("을") : LC_TEXT("를"));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 아이템이 필요합니다"), LC_ITEM_NAME(dwFood, bLocale));
	}
}

#ifdef __DUNGEON_INFO__
ACMD(do_dungeon_info)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "rank")
	{
		if (vecArgs.size() < 4) { return; }
		DWORD mobIdx, rankIdx;
		if (!str_to_number(mobIdx, vecArgs[2].c_str()) || !str_to_number(rankIdx, vecArgs[3].c_str()))
			return;
		CHARACTER_MANAGER::Instance().SendDungeonRank(ch, mobIdx, rankIdx);
	}
	else if (vecArgs[1] == "test_cooldown" && ch->IsGM())
	{
		ch->SetQuestFlag("jotun.cooldown", time(0) + 30);
		ch->SetQuestFlag("hydra.cooldown", time(0) + 60);
		ch->SetQuestFlag("ship_defense.cooldown", time(0) + 90);
		ch->SendDungeonCooldown(0);
	}
	else if (vecArgs[1] == "update" && ch->IsGM())
		ch->SendDungeonCooldown(0);
}
#endif

#define MAX_REASON_LEN		128

EVENTINFO(TimedEventInfo)
{
	DynamicCharacterPtr ch;
	int		subcmd;
	int		 	left_second;
	char		szReason[MAX_REASON_LEN];

	TimedEventInfo()
	: ch()
	, subcmd( 0 )
	, left_second( 0 )
	{
		::memset( szReason, 0, MAX_REASON_LEN );
	}
};

struct SendDisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetCharacter())
		{
			if (d->GetCharacter()->GetGMLevel() == GM_PLAYER)
				d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "quit Shutdown(SendDisconnectFunc)");
		}
	}
};

struct DisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetType() == DESC_TYPE_CONNECTOR)
			return;

		if (d->IsPhase(PHASE_P2P))
			return;

		if (d->GetCharacter())
			d->GetCharacter()->Disconnect("Shutdown(DisconnectFunc)");

		d->SetPhase(PHASE_CLOSE);
	}
};

EVENTINFO(shutdown_event_data)
{
	int seconds;

	shutdown_event_data()
	: seconds( 0 )
	{
	}
};

EVENTFUNC(shutdown_event)
{
	shutdown_event_data* info = dynamic_cast<shutdown_event_data*>( event->info );

	if ( info == NULL )
	{
		sys_err( "shutdown_event> <Factor> Null pointer" );
		return 0;
	}

	int * pSec = & (info->seconds);

	if (*pSec < 0)
	{
		sys_log(0, "shutdown_event sec %d", *pSec);

		if (--*pSec == -10)
		{
			const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), DisconnectFunc());
			return passes_per_sec;
		}
		else if (*pSec < -10)
			return 0;

		return passes_per_sec;
	}
	else if (*pSec == 0)
	{
		const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_set_desc.begin(), c_set_desc.end(), SendDisconnectFunc());
		g_bNoMoreClient = true;
		--*pSec;
		return passes_per_sec;
	}
	else
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "Server shut down in %d seconds.", *pSec);
		SendNotice(buf);

		--*pSec;
		return passes_per_sec;
	}
}

void Shutdown(int iSec)
{
	if (g_bNoMoreClient)
	{
		thecore_shutdown();
		return;
	}

	CWarMapManager::instance().OnShutdown();

	char buf[64];
	snprintf(buf, sizeof(buf), "Server will close in %d seconds.", iSec);

	SendNotice(buf);

	shutdown_event_data* info = AllocEventInfo<shutdown_event_data>();
	info->seconds = iSec;

	event_create(shutdown_event, info, 1);
}

ACMD(do_shutdown)
{
	sys_err("Accept shutdown command from %s.", (ch) ? ch->GetName() : "NONAME");

	TPacketGGShutdown p;
	p.bHeader = HEADER_GG_SHUTDOWN;
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShutdown));

	Shutdown(10);
}

EVENTFUNC(timed_event)
{
	TimedEventInfo * info = dynamic_cast<TimedEventInfo *>( event->info );

	if ( info == NULL )
	{
		sys_err( "timed_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}
	LPDESC d = ch->GetDesc();

	if (info->left_second <= 0)
	{
		ch->m_pkTimedEvent = NULL;

		switch (info->subcmd)
		{
			case SCMD_LOGOUT:
			case SCMD_QUIT:
			case SCMD_PHASE_SELECT:
				{
					TPacketNeedLoginLogInfo acc_info;
					acc_info.dwPlayerID = ch->GetDesc()->GetAccountTable().id;
					acc_info.bLanguage = ch->GetDesc()->GetAccountTable().bLanguage;

					db_clientdesc->DBPacket( HEADER_GD_VALID_LOGOUT, 0, &acc_info, sizeof(acc_info) );

					LogManager::instance().DetailLoginLog( false, ch );
				}
				break;
		}

		switch (info->subcmd)
		{
			case SCMD_LOGOUT:
				if (d)
					d->SetPhase(PHASE_CLOSE);
				break;

			case SCMD_QUIT:
				ch->ChatPacket(CHAT_TYPE_COMMAND, "quit");
				if (d) // @fixme197
					d->DelayedDisconnect(1);
				break;

			case SCMD_PHASE_SELECT:
				ch->Disconnect("timed_event - SCMD_PHASE_SELECT");
				if (d)
					d->SetPhase(PHASE_SELECT);
				break;
		}

		return 0;
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d초 남았습니다."), info->left_second);
		--info->left_second;
	}

	return PASSES_PER_SEC(1);
}

ACMD(do_cmd)
{
	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("취소 되었습니다."));
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

#ifdef __MAINTENANCE__
	if (CMaintenanceManager::Instance().GetGameMode()== GAME_MODE_LOBBY && !ch->IsGM())
		return;
#endif

	switch (subcmd)
	{
		case SCMD_LOGOUT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("로그인 화면으로 돌아 갑니다. 잠시만 기다리세요."));
			break;

		case SCMD_QUIT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("게임을 종료 합니다. 잠시만 기다리세요."));
			break;

		case SCMD_PHASE_SELECT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("캐릭터를 전환 합니다. 잠시만 기다리세요."));
			break;
	}

	int nExitLimitTime = 10;

	if (ch->IsHack(false, true, nExitLimitTime) &&
		false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()) &&
	   	(!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		return;
	}

	switch (subcmd)
	{
		case SCMD_LOGOUT:
		case SCMD_QUIT:
		case SCMD_PHASE_SELECT:
			{
				TimedEventInfo* info = AllocEventInfo<TimedEventInfo>();

				{
					if (ch->IsPosition(POS_FIGHTING))
						info->left_second = 10;
					else
						info->left_second = 3;
				}

				info->ch		= ch;
				info->subcmd		= subcmd;
				strlcpy(info->szReason, argument, sizeof(info->szReason));

				ch->m_pkTimedEvent	= event_create(timed_event, info, 1);
			}
			break;
	}
}

ACMD(do_mount)
{
}

ACMD(do_fishing)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	ch->SetRotation(atof(arg1));
	ch->fishing();
}

ACMD(do_console)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ConsoleEnable");
}

ACMD(do_restart)
{
	if (false == ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
		ch->StartRecoveryEvent();
		return;
	}

	if (NULL == ch->m_pkDeadEvent)
		return;

	int iTimeToDead = (event_time(ch->m_pkDeadEvent) / passes_per_sec);

	if (subcmd != SCMD_RESTART_TOWN && (!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		if (!test_server)
		{
			#define eFRS_HERESEC	170

			if (iTimeToDead > eFRS_HERESEC)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아직 재시작 할 수 없습니다. (%d초 남음)"), iTimeToDead - eFRS_HERESEC);
				return;
			}
		}
	}

	//PREVENT_HACK

	if (subcmd == SCMD_RESTART_TOWN)
	{
		#define eFRS_TOWNSEC	173

		if (iTimeToDead > eFRS_TOWNSEC)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아직 마을에서 재시작 할 수 없습니다. (%d 초 남음)"), iTimeToDead - eFRS_TOWNSEC);
			return;
		}
	}
	//END_PREVENT_HACK

	ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");

	ch->GetDesc()->SetPhase(PHASE_GAME);
	ch->SetPosition(POS_STANDING);
	ch->StartRecoveryEvent();

	//FORKED_LOAD

	if (1 == quest::CQuestManager::instance().GetEventFlag("threeway_war"))
	{
		if (subcmd == SCMD_RESTART_TOWN || subcmd == SCMD_RESTART_HERE)
		{
			if (true == CThreeWayWar::instance().IsThreeWayWarMapIndex(ch->GetMapIndex()) &&
					false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));

				ch->ReviveInvisible(5);
				ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());

				return;
			}

			if (true == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				if (CThreeWayWar::instance().GetReviveTokenForPlayer(ch->GetPlayerID()) <= 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("성지에서 부활 기회를 모두 잃었습니다! 마을로 이동합니다!"));
					ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
				}
				else
				{
					ch->Show(ch->GetMapIndex(), GetSungziStartX(ch->GetEmpire()), GetSungziStartY(ch->GetEmpire()));
				}

				ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				ch->ReviveInvisible(5);

				return;
			}
		}
	}
	//END_FORKED_LOAD

	if (ch->GetDungeon())
		ch->GetDungeon()->UseRevive(ch);

	if (ch->GetWarMap() && !ch->IsObserverMode())
	{
		CWarMap * pMap = ch->GetWarMap();
		DWORD dwGuildOpponent = pMap ? pMap->GetGuildOpponent(ch) : 0;

		if (dwGuildOpponent)
		{
			switch (subcmd)
			{
				case SCMD_RESTART_TOWN:
					sys_log(0, "do_restart: restart town");
					PIXEL_POSITION pos;

					if (CWarMapManager::instance().GetStartPosition(ch->GetMapIndex(), ch->GetGuild()->GetID() < dwGuildOpponent ? 0 : 1, pos))
						ch->Show(ch->GetMapIndex(), pos.x, pos.y);
					else
						ch->ExitToSavedLocation();

					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
					break;

				case SCMD_RESTART_HERE:
					sys_log(0, "do_restart: restart here");
					ch->RestartAtSamePos();
					//ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY());
					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
					break;
			}

			return;
		}
	}
	switch (subcmd)
	{
		case SCMD_RESTART_TOWN:
			sys_log(0, "do_restart: restart town");
			PIXEL_POSITION pos;

			if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
				ch->WarpSet(pos.x, pos.y);
			else if (ch->GetDungeon())
				ch->ExitToSavedLocation();
			else
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
			ch->PointChange(POINT_HP, 50 - ch->GetHP());
			ch->DeathPenalty(1);
			break;

		case SCMD_RESTART_HERE:
			sys_log(0, "do_restart: restart here");
			ch->RestartAtSamePos();
			//ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY());
			ch->PointChange(POINT_HP, 50 - ch->GetHP());
			ch->DeathPenalty(0);
			ch->ReviveInvisible(5);
			break;
	}
}

#define MAX_STAT g_iStatusPointSetMaxValue

ACMD(do_stat_reset)
{
	ch->PointChange(POINT_STAT_RESET_COUNT, 12 - ch->GetPoint(POINT_STAT_RESET_COUNT));
}

ACMD(do_stat_minus)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑 중에는 능력을 올릴 수 없습니다."));
		return;
	}

	if (ch->GetPoint(POINT_STAT_RESET_COUNT) <= 0)
		return;

	if (!strcmp(arg1, "st"))
	{
		if (ch->GetRealPoint(POINT_ST) <= JobInitialPoints[ch->GetJob()].st)
			return;

		ch->SetRealPoint(POINT_ST, ch->GetRealPoint(POINT_ST) - 1);
		ch->SetPoint(POINT_ST, ch->GetPoint(POINT_ST) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_ST, 0);
	}
	else if (!strcmp(arg1, "dx"))
	{
		if (ch->GetRealPoint(POINT_DX) <= JobInitialPoints[ch->GetJob()].dx)
			return;

		ch->SetRealPoint(POINT_DX, ch->GetRealPoint(POINT_DX) - 1);
		ch->SetPoint(POINT_DX, ch->GetPoint(POINT_DX) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_DX, 0);
	}
	else if (!strcmp(arg1, "ht"))
	{
		if (ch->GetRealPoint(POINT_HT) <= JobInitialPoints[ch->GetJob()].ht)
			return;

		ch->SetRealPoint(POINT_HT, ch->GetRealPoint(POINT_HT) - 1);
		ch->SetPoint(POINT_HT, ch->GetPoint(POINT_HT) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_HT, 0);
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (!strcmp(arg1, "iq"))
	{
		if (ch->GetRealPoint(POINT_IQ) <= JobInitialPoints[ch->GetJob()].iq)
			return;

		ch->SetRealPoint(POINT_IQ, ch->GetRealPoint(POINT_IQ) - 1);
		ch->SetPoint(POINT_IQ, ch->GetPoint(POINT_IQ) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_IQ, 0);
		ch->PointChange(POINT_MAX_SP, 0);
	}
	else
		return;

	ch->PointChange(POINT_STAT, +1);
	ch->PointChange(POINT_STAT_RESET_COUNT, -1);
	ch->ComputePoints();
}

ACMD(do_stat_val)
{
	char	arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	int val = 0;
	str_to_number(val, arg2);
	
	if (!*arg1 || val <= 0)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change your state as long as you are transformed."));
		return;
	}

	if (ch->GetPoint(POINT_STAT) <= 0)
		return;

	BYTE idx = 0;
	
	if (!strcmp(arg1, "st"))
		idx = POINT_ST;
	else if (!strcmp(arg1, "dx"))
		idx = POINT_DX;
	else if (!strcmp(arg1, "ht"))
		idx = POINT_HT;
	else if (!strcmp(arg1, "iq"))
		idx = POINT_IQ;
	else
		return;

	if (ch->GetRealPoint(idx) >= MAX_STAT)
		return;
	
	if (val > ch->GetPoint(POINT_STAT))
		val = ch->GetPoint(POINT_STAT);
	
	if (ch->GetRealPoint(idx) + val > MAX_STAT)
		val = MAX_STAT - ch->GetRealPoint(idx);

	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + val);
	ch->SetPoint(idx, ch->GetPoint(idx) + val);
	ch->ComputePoints();
	ch->PointChange(idx, 0);

	if (idx == POINT_IQ)
		ch->PointChange(POINT_MAX_HP, 0);
	else if (idx == POINT_HT)
		ch->PointChange(POINT_MAX_SP, 0);

	ch->PointChange(POINT_STAT, -val);
	ch->ComputePoints();
}

ACMD(do_stat)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑 중에는 능력을 올릴 수 없습니다."));
		return;
	}

	if (ch->GetPoint(POINT_STAT) <= 0)
		return;

	BYTE idx = 0;

	if (!strcmp(arg1, "st"))
		idx = POINT_ST;
	else if (!strcmp(arg1, "dx"))
		idx = POINT_DX;
	else if (!strcmp(arg1, "ht"))
		idx = POINT_HT;
	else if (!strcmp(arg1, "iq"))
		idx = POINT_IQ;
	else
		return;

	// ch->ChatPacket(CHAT_TYPE_INFO, "%s GRP(%d) idx(%u), MAX_STAT(%d), expr(%d)", __FUNCTION__, ch->GetRealPoint(idx), idx, MAX_STAT, ch->GetRealPoint(idx) >= MAX_STAT);
	if (ch->GetRealPoint(idx) >= MAX_STAT)
		return;

	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + 1);
	ch->SetPoint(idx, ch->GetPoint(idx) + 1);
	ch->ComputePoints();
	ch->PointChange(idx, 0);

	if (idx == POINT_IQ)
	{
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (idx == POINT_HT)
	{
		ch->PointChange(POINT_MAX_SP, 0);
	}

	ch->PointChange(POINT_STAT, -1);
	ch->ComputePoints();
}

ACMD(do_pvp)
{
	if (ch->GetArena() != NULL || CArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(vid);

	if (!pkVictim)
		return;

	if (pkVictim->IsNPC())
		return;

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	if (MessengerManager::instance().IsBlocked(ch->GetName(), pkVictim->GetName()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), pkVictim->GetName());
		return;
	}
	else if (MessengerManager::instance().IsBlocked(pkVictim->GetName(), ch->GetName()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s has blocked you."), pkVictim->GetName());
		return;
	}
#endif

	if (pkVictim->GetArena() != NULL)
	{
		pkVictim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 대련중입니다."));
		return;
	}

	CPVPManager::instance().Insert(ch, pkVictim);
}

ACMD(do_guildskillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch->GetGuild())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드에 속해있지 않습니다."));
		return;
	}

	CGuild* g = ch->GetGuild();
	TGuildMember* gm = g->GetMember(ch->GetPlayerID());
	if (gm->grade == GUILD_LEADER_GRADE)
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		g->SkillLevelUp(vnum);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드 스킬 레벨을 변경할 권한이 없습니다."));
	}
}

ACMD(do_skillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vnum = 0;
	str_to_number(vnum, arg1);

	if (true == ch->CanUseSkill(vnum))
	{
		ch->SkillLevelUp(vnum);
	}
	else
	{
		switch(vnum)
		{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:

			case SKILL_7_A_ANTI_TANHWAN:
			case SKILL_7_B_ANTI_AMSEOP:
			case SKILL_7_C_ANTI_SWAERYUNG:
			case SKILL_7_D_ANTI_YONGBI:

			case SKILL_8_A_ANTI_GIGONGCHAM:
			case SKILL_8_B_ANTI_YEONSA:
			case SKILL_8_C_ANTI_MAHWAN:
			case SKILL_8_D_ANTI_BYEURAK:

			case SKILL_ADD_HP:
			case SKILL_RESIST_PENETRATE:
				ch->SkillLevelUp(vnum);
				break;
		}
	}
}

//
//
ACMD(do_safebox_close)
{
	ch->CloseSafebox();
}

//
//
ACMD(do_safebox_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	ch->ReqSafeboxLoad(arg1);
}

ACMD(do_safebox_change_password)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || strlen(arg1)>6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 잘못된 암호를 입력하셨습니다."));
		return;
	}

	if (!*arg2 || strlen(arg2)>6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 잘못된 암호를 입력하셨습니다."));
		return;
	}

	TSafeboxChangePasswordPacket p;

	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szOldPassword, arg1, sizeof(p.szOldPassword));
	strlcpy(p.szNewPassword, arg2, sizeof(p.szNewPassword));

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_CHANGE_PASSWORD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || strlen(arg1) > 6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 잘못된 암호를 입력하셨습니다."));
		return;
	}

	int iPulse = thecore_pulse();

	if (ch->GetMall())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고가 이미 열려있습니다."));
		return;
	}

	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	if (iPulse - ch->GetMallLoadTime() < passes_per_sec * 10)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고를 닫은지 10초 안에는 열 수 없습니다."));
		return;
	}

	ch->SetMallLoadTime(iPulse);

	TSafeboxLoadPacket p;
	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szLogin, ch->GetDesc()->GetAccountTable().login, sizeof(p.szLogin));
	strlcpy(p.szPassword, arg1, sizeof(p.szPassword));

	db_clientdesc->DBPacket(HEADER_GD_MALL_LOAD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_close)
{
	if (ch->GetMall())
	{
		ch->SetMallLoadTime(thecore_pulse());
		ch->CloseMall();
		ch->Save();
	}
}

ACMD(do_ungroup)
{
	if (!ch->GetParty())
		return;

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던젼내에서는 파티를 나갈 수 없습니다."));
		return;
	}

	LPPARTY pParty = ch->GetParty();

	if (pParty->IsPartyInAnyDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던젼내에서는 파티를 나갈 수 없습니다."));
		return;
	}

	if (pParty->GetMemberCount() == 2)
	{
		// party disband
		CPartyManager::instance().DeleteParty(pParty);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티에서 나가셨습니다."));
		//pParty->SendPartyRemoveOneToAll(ch);
		pParty->Quit(ch->GetPlayerID());
		//pParty->SendPartyRemoveAllToOne(ch);
	}
}

ACMD(do_close_shop)
{
	if (ch->GetMyShop())
	{
		ch->CloseMyShop();
		return;
	}
}

ACMD(do_set_walk_mode)
{
	ch->SetNowWalking(true);
	ch->SetWalking(true);
}

ACMD(do_set_run_mode)
{
	ch->SetNowWalking(false);
	ch->SetWalking(false);
}

#ifdef ENABLE_SHOP_SEARCH_SYSTEM
#include "offlineshop_manager.h"
ACMD(do_shop_search)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }

	else if (vecArgs[1] == "search")
	{
		if (vecArgs.size() < 3) { return; }
		COfflineShopManager::Instance().StartSearch(ch, vecArgs[2].c_str());
	}
	else if (vecArgs[1] == "page")
	{
		if (vecArgs.size() < 3) { return; }
		int iPageIdx;
		if (!str_to_number(iPageIdx, vecArgs[2].c_str()))
			return;
		COfflineShopManager::Instance().SendPlayerSearch(ch, iPageIdx);
	}
}
#endif

ACMD(do_war)
{
	CGuild * g = ch->GetGuild();

	if (!g)
		return;

	if (g->UnderAnyWar())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 이미 다른 전쟁에 참전 중 입니다."));
		return;
	}

	char arg1[256], arg2[256];
	DWORD type = GUILD_WAR_TYPE_FIELD; //fixme102 base int modded uint
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
		return;

	if (*arg2)
	{
		str_to_number(type, arg2);

		if (type >= GUILD_WAR_TYPE_MAX_NUM)
			type = GUILD_WAR_TYPE_FIELD;
	}

	DWORD gm_pid = g->GetMasterPID();

	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전에 대한 권한이 없습니다."));
		return;
	}

	CGuild * opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 그런 길드가 없습니다."));
		return;
	}

	switch (g->GetGuildWarState(opp_g->GetID()))
	{
		case GUILD_WAR_NONE:
			{
				if (opp_g->UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드가 이미 전쟁 중 입니다."));
					return;
				}

				int iWarPrice = KOR_aGuildWarInfo[type].iWarPrice;

				if (g->GetGuildMoney() < iWarPrice)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 전비가 부족하여 길드전을 할 수 없습니다."));
					return;
				}

				if (opp_g->GetGuildMoney() < iWarPrice)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드의 전비가 부족하여 길드전을 할 수 없습니다."));
					return;
				}
			}
			break;

		case GUILD_WAR_SEND_DECLARE:
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 선전포고 중인 길드입니다."));
				return;
			}
			break;

		case GUILD_WAR_RECV_DECLARE:
			{
				if (opp_g->UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드가 이미 전쟁 중 입니다."));
					g->RequestRefuseWar(opp_g->GetID());
					return;
				}
			}
			break;

		case GUILD_WAR_RESERVE:
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 이미 전쟁이 예약된 길드 입니다."));
				return;
			}
			break;

		case GUILD_WAR_END:
			return;

		default:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 이미 전쟁 중인 길드입니다."));
			g->RequestRefuseWar(opp_g->GetID());
			return;
	}

	if (!g->CanStartWar(type))
	{
		if (g->GetLadderPoint() == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 레더 점수가 모자라서 길드전을 할 수 없습니다."));
			sys_log(0, "GuildWar.StartError.NEED_LADDER_POINT");
		}
		else if (g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전을 하기 위해선 최소한 %d명이 있어야 합니다."), GUILD_WAR_MIN_MEMBER_COUNT);
			sys_log(0, "GuildWar.StartError.NEED_MINIMUM_MEMBER[%d]", GUILD_WAR_MIN_MEMBER_COUNT);
		}
		else
		{
			sys_log(0, "GuildWar.StartError.UNKNOWN_ERROR");
		}
		return;
	}

	if (!opp_g->CanStartWar(GUILD_WAR_TYPE_FIELD))
	{
		if (opp_g->GetLadderPoint() == 0)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드의 레더 점수가 모자라서 길드전을 할 수 없습니다."));
		else if (opp_g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드의 길드원 수가 부족하여 길드전을 할 수 없습니다."));
		return;
	}

	do
	{
		if (g->GetMasterCharacter() != NULL)
			break;

		CCI *pCCI = P2P_MANAGER::instance().FindByPID(g->GetMasterPID());

		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드의 길드장이 접속중이 아닙니다."));
		g->RequestRefuseWar(opp_g->GetID());
		return;

	} while (false);

	do
	{
		if (opp_g->GetMasterCharacter() != NULL)
			break;

		CCI *pCCI = P2P_MANAGER::instance().FindByPID(opp_g->GetMasterPID());

		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드의 길드장이 접속중이 아닙니다."));
		g->RequestRefuseWar(opp_g->GetID());
		return;

	} while (false);

	g->RequestDeclareWar(opp_g->GetID(), type);
}

ACMD(do_nowar)
{
	CGuild* g = ch->GetGuild();
	if (!g)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD gm_pid = g->GetMasterPID();

	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전에 대한 권한이 없습니다."));
		return;
	}

	CGuild* opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 그런 길드가 없습니다."));
		return;
	}

	g->RequestRefuseWar(opp_g->GetID());
}

ACMD(do_detaillog)
{
	ch->DetailLog();
}

ACMD(do_monsterlog)
{
	ch->ToggleMonsterLog();
}

ACMD(do_pkmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	BYTE mode = 0;
	str_to_number(mode, arg1);

	if (mode == PK_MODE_PROTECT)
		return;

	if (ch->GetLevel() < PK_PROTECT_LEVEL && mode != 0)
		return;

	ch->SetPKMode(mode);
}

ACMD(do_messenger_auth)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	char answer = LOWER(*arg1);
	// @fixme130 AuthToAdd void -> bool
	bool bIsDenied = answer != 'y';
	bool bIsAdded = MessengerManager::instance().AuthToAdd(ch->GetName(), arg2, bIsDenied); // DENY
	if (bIsAdded && bIsDenied)
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg2);

		if (tch)
			tch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s declined the invitation."), ch->GetName());
#ifdef CROSS_CHANNEL_FRIEND_REQUEST
		else
		{
			CCI* pkCCI = P2P_MANAGER::Instance().Find(arg2);
			if (pkCCI)
			{
				LPDESC pkDesc = pkCCI->pkDesc;
				pkDesc->SetRelay(arg2);
				pkDesc->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s declined the invitation."), ch->GetName());
				pkDesc->SetRelay("");
			}
		}
#endif
	}
}

ACMD(do_setblockmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		BYTE flag = 0;
		str_to_number(flag, arg1);
		ch->SetBlockMode(flag);
	}
}

ACMD(do_unmount)
{
	if (true == ch->UnEquipSpecialRideUniqueItem())
	{
		ch->RemoveAffect(AFFECT_MOUNT);
		ch->RemoveAffect(AFFECT_MOUNT_BONUS);

		if (ch->IsHorseRiding())
		{
			ch->StopRiding();
		}
	}
	else
	{
		ch->ChatPacket( CHAT_TYPE_INFO, LC_TEXT("인벤토리가 꽉 차서 내릴 수 없습니다."));
	}
}

ACMD(do_observer_exit)
{
	if (ch->IsObserverMode())
	{
		if (ch->GetWarMap())
			ch->SetWarMap(NULL);

		if (ch->GetArena() != NULL || ch->GetArenaObserverMode() == true)
		{
			ch->SetArenaObserverMode(false);

			if (ch->GetArena() != NULL)
				ch->GetArena()->RemoveObserver(ch->GetPlayerID());

			ch->SetArena(NULL);
			ch->WarpSet(ARENA_RETURN_POINT_X(ch->GetEmpire()), ARENA_RETURN_POINT_Y(ch->GetEmpire()));
		}
		else
		{
			ch->ExitToSavedLocation();
		}
		ch->SetObserverMode(false);
	}
}

ACMD(do_view_equip)
{
	if (ch->GetGMLevel() <= GM_PLAYER)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD vid = 0;
		str_to_number(vid, arg1);
		LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

		if (!tch)
			return;

		if (!tch->IsPC())
			return;


#if defined(__MESSENGER_BLOCK_SYSTEM__)
		if (MessengerManager::instance().IsBlocked(ch->GetName(), tch->GetName()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), tch->GetName());
			return;
		}
		else if (MessengerManager::instance().IsBlocked(tch->GetName(), ch->GetName()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s has blocked you."), tch->GetName());
			return;
		}
#endif

		tch->SendEquipment(ch);
	}
}

ACMD(do_party_request)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	if (ch->GetParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 파티에 속해 있으므로 가입신청을 할 수 없습니다."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		if (!ch->RequestToParty(tch))
			ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

ACMD(do_party_request_accept)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->AcceptToParty(tch);
}

ACMD(do_party_request_deny)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->DenyToParty(tch);
}

ACMD(do_monarch_warpto)
{
	if (!CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주만이 사용 가능한 기능입니다"));
		return;
	}

	if (!ch->IsMCOK(CHARACTER::MI_WARP))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d 초간 쿨타임이 적용중입니다."), ch->GetMCLTime(CHARACTER::MI_WARP));
		return;
	}

	const int WarpPrice = 10000;

	if (!CMonarch::instance().IsMoneyOk(WarpPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("국고에 돈이 부족합니다. 현재 : %u 필요금액 : %u"), NationMoney, WarpPrice);
		return;
	}

	int x = 0, y = 0;
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("사용법: warpto <character name>"));
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bEmpire != ch->GetEmpire())
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("타제국 유저에게는 이동할수 없습니다"));
				return;
			}

			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("해당 유저는 %d 채널에 있습니다. (현재 채널 %d)"), pkCCI->bChannel, g_bChannel);
				return;
			}
			if (!IsMonarchWarpZone(pkCCI->lMapIndex))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 이동할 수 없습니다."));
				return;
			}

			PIXEL_POSITION pos;

			if (!SECTREE_MANAGER::instance().GetCenterPositionOfMap(pkCCI->lMapIndex, pos))
				ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map (index %d)", pkCCI->lMapIndex);
			else
			{
				//ch->ChatPacket(CHAT_TYPE_INFO, "You warp to (%d, %d)", pos.x, pos.y);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 에게로 이동합니다"), arg1);
				ch->WarpSet(pos.x, pos.y);

				CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

				ch->SetMC(CHARACTER::MI_WARP);
			}
		}
		else if (NULL == CHARACTER_MANAGER::instance().FindPC(arg1))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no one by that name");
		}

		return;
	}
	else
	{
		if (tch->GetEmpire() != ch->GetEmpire())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("타제국 유저에게는 이동할수 없습니다"));
			return;
		}
		if (!IsMonarchWarpZone(tch->GetMapIndex()))
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 이동할 수 없습니다."));
			return;
		}
		x = tch->GetX();
		y = tch->GetY();
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 에게로 이동합니다"), arg1);
	ch->WarpSet(x, y);
	ch->Stop();

	CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

	ch->SetMC(CHARACTER::MI_WARP);
}

ACMD(do_monarch_transfer)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("사용법: transfer <name>"));
		return;
	}

	if (!CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주만이 사용 가능한 기능입니다"));
		return;
	}

	if (!ch->IsMCOK(CHARACTER::MI_TRANSFER))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d 초간 쿨타임이 적용중입니다."), ch->GetMCLTime(CHARACTER::MI_TRANSFER));
		return;
	}

	const int WarpPrice = 10000;

	if (!CMonarch::instance().IsMoneyOk(WarpPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("국고에 돈이 부족합니다. 현재 : %u 필요금액 : %u"), NationMoney, WarpPrice);
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bEmpire != ch->GetEmpire())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 제국 유저는 소환할 수 없습니다."));
				return;
			}
			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 님은 %d 채널에 접속 중 입니다. (현재 채널: %d)"), arg1, pkCCI->bChannel, g_bChannel);
				return;
			}
			if (!IsMonarchWarpZone(pkCCI->lMapIndex))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 이동할 수 없습니다."));
				return;
			}
			if (!IsMonarchWarpZone(ch->GetMapIndex()))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 소환할 수 없습니다."));
				return;
			}

			TPacketGGTransfer pgg;

			pgg.bHeader = HEADER_GG_TRANSFER;
			strlcpy(pgg.szName, arg1, sizeof(pgg.szName));
			pgg.lX = ch->GetX();
			pgg.lY = ch->GetY();

			P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGTransfer));
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 님을 소환하였습니다."), arg1);

			CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

			ch->SetMC(CHARACTER::MI_TRANSFER);
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("입력하신 이름을 가진 사용자가 없습니다."));
		}

		return;
	}

	if (ch == tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("자신을 소환할 수 없습니다."));
		return;
	}

	if (tch->GetEmpire() != ch->GetEmpire())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 제국 유저는 소환할 수 없습니다."));
		return;
	}
	if (!IsMonarchWarpZone(tch->GetMapIndex()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 이동할 수 없습니다."));
		return;
	}
	if (!IsMonarchWarpZone(ch->GetMapIndex()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 소환할 수 없습니다."));
		return;
	}

	//tch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
	tch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());

	CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

	ch->SetMC(CHARACTER::MI_TRANSFER);
}

ACMD(do_monarch_info)
{
	if (CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("나의 군주 정보"));
		TMonarchInfo * p = CMonarch::instance().GetMonarch();
		for (int n = 1; n < 4; ++n)
		{
			if (n == ch->GetEmpire())
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[%s군주] : %s  보유금액 %lld "), EMPIRE_NAME(n), p->name[n], p->money[n]);
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[%s군주] : %s  "), EMPIRE_NAME(n), p->name[n]);

		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주 정보"));
		TMonarchInfo * p = CMonarch::instance().GetMonarch();
		for (int n = 1; n < 4; ++n)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[%s군주] : %s  "), EMPIRE_NAME(n), p->name[n]);

		}
	}
}

ACMD(do_elect)
{
	db_clientdesc->DBPacketHeader(HEADER_GD_COME_TO_VOTE, ch->GetDesc()->GetHandle(), 0);
}

// LUA_ADD_GOTO_INFO
struct GotoInfo
{
	std::string 	st_name;

	BYTE 	empire;
	int 	mapIndex;
	DWORD 	x, y;

	GotoInfo()
	{
		st_name 	= "";
		empire 		= 0;
		mapIndex 	= 0;

		x = 0;
		y = 0;
	}

	GotoInfo(const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void operator = (const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void __copy__(const GotoInfo& c_src)
	{
		st_name 	= c_src.st_name;
		empire 		= c_src.empire;
		mapIndex 	= c_src.mapIndex;

		x = c_src.x;
		y = c_src.y;
	}
};

ACMD(do_monarch_tax)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: monarch_tax <1-50>");
		return;
	}

	if (!ch->IsMonarch())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주만이 사용할수 있는 기능입니다"));
		return;
	}

	int tax = 0;
	str_to_number(tax,  arg1);

	if (tax < 1 || tax > 50)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1-50 사이의 수치를 선택해주세요"));

	quest::CQuestManager::instance().SetEventFlag("trade_tax", tax);

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("세금이 %d %로 설정되었습니다"));

	char szMsg[1024];

	snprintf(szMsg, sizeof(szMsg), "군주의 명으로 세금이 %d %% 로 변경되었습니다", tax);
	BroadcastNotice(szMsg);

	snprintf(szMsg, sizeof(szMsg), "앞으로는 거래 금액의 %d %% 가 국고로 들어가게됩니다.", tax);
	BroadcastNotice(szMsg);

	ch->SetMC(CHARACTER::MI_TAX);
}

static const DWORD cs_dwMonarchMobVnums[] =
{
	191,
	192,
	193,
	194,
	391,
	392,
	393,
	394,
	491,
	492,
	493,
	494,
	591,
	691,
	791,
	1304,
	1901,
	2091,
	2191,
	2206,
	0,
};

#ifdef RENEWAL_PICKUP_AFFECT
ACMD(do_pickup_affect)
{
	
	CAffect* affect = ch->FindAffect(AFFECT_PICKUP_ENABLE);
	if (!affect)
	{
		affect = ch->FindAffect(AFFECT_PICKUP_DEACTIVE);
		if (!affect)
			return;
	}
	const int64_t duration = affect->lDuration - time(0);
	const bool isActive = affect->dwType == AFFECT_PICKUP_ENABLE;
	ch->RemoveAffect(affect);
	if (duration <= 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Affect is removed.");
		return;
	}
	ch->AddAffect(isActive ? AFFECT_PICKUP_DEACTIVE : AFFECT_PICKUP_ENABLE, POINT_NONE, 0, AFF_NONE, duration, 0, 0, false);
}
#endif

ACMD(do_monarch_mob)
{
	char arg1[256];
	LPCHARACTER	tch;

	one_argument(argument, arg1, sizeof(arg1));

	if (!ch->IsMonarch())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주만이 사용할수 있는 기능입니다"));
		return;
	}

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mmob <mob name>");
		return;
	}

#ifdef ENABLE_MONARCH_MOB_CMD_MAP_CHECK // @warme006
	BYTE pcEmpire = ch->GetEmpire();
	BYTE mapEmpire = SECTREE_MANAGER::instance().GetEmpireFromMapIndex(ch->GetMapIndex());
	if (mapEmpire != pcEmpire && mapEmpire != 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("자국 영토에서만 사용할 수 있는 기능입니다"));
		return;
	}
#endif

	const int SummonPrice = 5000000;

	if (!ch->IsMCOK(CHARACTER::MI_SUMMON))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d 초간 쿨타임이 적용중입니다."), ch->GetMCLTime(CHARACTER::MI_SUMMON));
		return;
	}

	if (!CMonarch::instance().IsMoneyOk(SummonPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("국고에 돈이 부족합니다. 현재 : %u 필요금액 : %u"), NationMoney, SummonPrice);
		return;
	}

	const CMob * pkMob;
	DWORD vnum = 0;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	DWORD count;

	for (count = 0; cs_dwMonarchMobVnums[count] != 0; ++count)
		if (cs_dwMonarchMobVnums[count] == vnum)
			break;

	if (0 == cs_dwMonarchMobVnums[count])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소환할수 없는 몬스터 입니다. 소환가능한 몬스터는 홈페이지를 참조하세요"));
		return;
	}

	tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum,
			ch->GetMapIndex(),
			ch->GetX() - number(200, 750),
			ch->GetY() - number(200, 750),
			ch->GetX() + number(200, 750),
			ch->GetY() + number(200, 750),
			true,
			pkMob->m_table.bType == CHAR_TYPE_STONE,
			true);

	if (tch)
	{
		CMonarch::instance().SendtoDBDecMoney(SummonPrice, ch->GetEmpire(), ch);

		ch->SetMC(CHARACTER::MI_SUMMON);
	}
}

static const char* FN_point_string(int apply_number)
{
	switch (apply_number)
	{
		case POINT_MAX_HP:	return LC_TEXT("최대 생명력 +%d");
		case POINT_MAX_SP:	return LC_TEXT("최대 정신력 +%d");
		case POINT_HT:		return LC_TEXT("체력 +%d");
		case POINT_IQ:		return LC_TEXT("지능 +%d");
		case POINT_ST:		return LC_TEXT("근력 +%d");
		case POINT_DX:		return LC_TEXT("민첩 +%d");
		case POINT_ATT_SPEED:	return LC_TEXT("공격속도 +%d");
		case POINT_MOV_SPEED:	return LC_TEXT("이동속도 %d");
		case POINT_CASTING_SPEED:	return LC_TEXT("쿨타임 -%d");
		case POINT_HP_REGEN:	return LC_TEXT("생명력 회복 +%d");
		case POINT_SP_REGEN:	return LC_TEXT("정신력 회복 +%d");
		case POINT_POISON_PCT:	return LC_TEXT("독공격 %d");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_BLEEDING_PCT:	return LC_TEXT("독공격 %d");
#endif
		case POINT_STUN_PCT:	return LC_TEXT("스턴 +%d");
		case POINT_SLOW_PCT:	return LC_TEXT("슬로우 +%d");
		case POINT_CRITICAL_PCT:	return LC_TEXT("%d%% 확률로 치명타 공격");
		case POINT_RESIST_CRITICAL:	return LC_TEXT("상대의 치명타 확률 %d%% 감소");
		case POINT_PENETRATE_PCT:	return LC_TEXT("%d%% 확률로 관통 공격");
		case POINT_RESIST_PENETRATE: return LC_TEXT("상대의 관통 공격 확률 %d%% 감소");
		case POINT_ATTBONUS_HUMAN:	return LC_TEXT("인간류 몬스터 타격치 +%d%%");
		case POINT_ATTBONUS_ANIMAL:	return LC_TEXT("동물류 몬스터 타격치 +%d%%");
		case POINT_ATTBONUS_ORC:	return LC_TEXT("웅귀족 타격치 +%d%%");
		case POINT_ATTBONUS_MILGYO:	return LC_TEXT("밀교류 타격치 +%d%%");
		case POINT_ATTBONUS_UNDEAD:	return LC_TEXT("시체류 타격치 +%d%%");
		case POINT_ATTBONUS_DEVIL:	return LC_TEXT("악마류 타격치 +%d%%");
		case POINT_STEAL_HP:		return LC_TEXT("타격치 %d%% 를 생명력으로 흡수");
		case POINT_STEAL_SP:		return LC_TEXT("타력치 %d%% 를 정신력으로 흡수");
		case POINT_MANA_BURN_PCT:	return LC_TEXT("%d%% 확률로 타격시 상대 전신력 소모");
		case POINT_DAMAGE_SP_RECOVER:	return LC_TEXT("%d%% 확률로 피해시 정신력 회복");
		case POINT_BLOCK:			return LC_TEXT("물리타격시 블럭 확률 %d%%");
		case POINT_DODGE:			return LC_TEXT("활 공격 회피 확률 %d%%");
		case POINT_RESIST_SWORD:	return LC_TEXT("한손검 방어 %d%%");
		case POINT_RESIST_TWOHAND:	return LC_TEXT("양손검 방어 %d%%");
		case POINT_RESIST_DAGGER:	return LC_TEXT("두손검 방어 %d%%");
		case POINT_RESIST_BELL:		return LC_TEXT("방울 방어 %d%%");
		case POINT_RESIST_FAN:		return LC_TEXT("부채 방어 %d%%");
		case POINT_RESIST_BOW:		return LC_TEXT("활공격 저항 %d%%");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_CLAW:		return LC_TEXT("두손검 방어 %d%%");
#endif
		case POINT_RESIST_FIRE:		return LC_TEXT("화염 저항 %d%%");
		case POINT_RESIST_ELEC:		return LC_TEXT("전기 저항 %d%%");
		case POINT_RESIST_MAGIC:	return LC_TEXT("마법 저항 %d%%");
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
		case POINT_RESIST_MAGIC_REDUCTION:	return LC_TEXT("마법 저항 %d%%");
#endif
		case POINT_RESIST_WIND:		return LC_TEXT("바람 저항 %d%%");
		case POINT_RESIST_ICE:		return LC_TEXT("냉기 저항 %d%%");
		case POINT_RESIST_EARTH:	return LC_TEXT("대지 저항 %d%%");
		case POINT_RESIST_DARK:		return LC_TEXT("어둠 저항 %d%%");
		case POINT_REFLECT_MELEE:	return LC_TEXT("직접 타격치 반사 확률 : %d%%");
		case POINT_REFLECT_CURSE:	return LC_TEXT("저주 되돌리기 확률 %d%%");
		case POINT_POISON_REDUCE:	return LC_TEXT("독 저항 %d%%");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_BLEEDING_REDUCE:	return LC_TEXT("독 저항 %d%%");
#endif
		case POINT_KILL_SP_RECOVER:	return LC_TEXT("%d%% 확률로 적퇴치시 정신력 회복");
		case POINT_EXP_DOUBLE_BONUS:	return LC_TEXT("%d%% 확률로 적퇴치시 경험치 추가 상승");
		case POINT_GOLD_DOUBLE_BONUS:	return LC_TEXT("%d%% 확률로 적퇴치시 돈 2배 드롭");
		case POINT_ITEM_DROP_BONUS:	return LC_TEXT("%d%% 확률로 적퇴치시 아이템 2배 드롭");
		case POINT_POTION_BONUS:	return LC_TEXT("물약 사용시 %d%% 성능 증가");
		case POINT_KILL_HP_RECOVERY:	return LC_TEXT("%d%% 확률로 적퇴치시 생명력 회복");
		case POINT_ATT_GRADE_BONUS:	return LC_TEXT("공격력 +%d");
		case POINT_DEF_GRADE_BONUS:	return LC_TEXT("방어력 +%d");
		case POINT_MAGIC_ATT_GRADE:	return LC_TEXT("마법 공격력 +%d");
		case POINT_MAGIC_DEF_GRADE:	return LC_TEXT("마법 방어력 +%d");
		case POINT_MAX_STAMINA:	return LC_TEXT("최대 지구력 +%d");
		case POINT_ATTBONUS_WARRIOR:	return LC_TEXT("무사에게 강함 +%d%%");
		case POINT_ATTBONUS_ASSASSIN:	return LC_TEXT("자객에게 강함 +%d%%");
		case POINT_ATTBONUS_SURA:		return LC_TEXT("수라에게 강함 +%d%%");
		case POINT_ATTBONUS_SHAMAN:		return LC_TEXT("무당에게 강함 +%d%%");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_ATTBONUS_WOLFMAN:	return LC_TEXT("무당에게 강함 +%d%%");
#endif
		case POINT_ATTBONUS_MONSTER:	return LC_TEXT("몬스터에게 강함 +%d%%");
		case POINT_MALL_ATTBONUS:		return LC_TEXT("공격력 +%d%%");
		case POINT_MALL_DEFBONUS:		return LC_TEXT("방어력 +%d%%");
		case POINT_MALL_EXPBONUS:		return LC_TEXT("경험치 %d%%");
		case POINT_MALL_ITEMBONUS:		return LC_TEXT("아이템 드롭율 %d배"); // @fixme180 float to int
		case POINT_MALL_GOLDBONUS:		return LC_TEXT("돈 드롭율 %d배"); // @fixme180 float to int
		case POINT_MAX_HP_PCT:			return LC_TEXT("최대 생명력 +%d%%");
		case POINT_MAX_SP_PCT:			return LC_TEXT("최대 정신력 +%d%%");
		case POINT_SKILL_DAMAGE_BONUS:	return LC_TEXT("스킬 데미지 %d%%");
		case POINT_NORMAL_HIT_DAMAGE_BONUS:	return LC_TEXT("평타 데미지 %d%%");
		case POINT_SKILL_DEFEND_BONUS:		return LC_TEXT("스킬 데미지 저항 %d%%");
		case POINT_NORMAL_HIT_DEFEND_BONUS:	return LC_TEXT("평타 데미지 저항 %d%%");
		case POINT_RESIST_WARRIOR:	return LC_TEXT("무사공격에 %d%% 저항");
		case POINT_RESIST_ASSASSIN:	return LC_TEXT("자객공격에 %d%% 저항");
		case POINT_RESIST_SURA:		return LC_TEXT("수라공격에 %d%% 저항");
		case POINT_RESIST_SHAMAN:	return LC_TEXT("무당공격에 %d%% 저항");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_WOLFMAN:	return LC_TEXT("무당공격에 %d%% 저항");
#endif
		default:					return "UNK_ID %d%%"; // @fixme180
	}
}

static bool FN_hair_affect_string(LPCHARACTER ch, char *buf, size_t bufsiz)
{
	if (NULL == ch || NULL == buf)
		return false;

	CAffect* aff = NULL;
	time_t expire = 0;
	struct tm ltm;
	int	year, mon, day;
	int	offset = 0;

	aff = ch->FindAffect(AFFECT_HAIR);

	if (NULL == aff)
		return false;

	expire = ch->GetQuestFlag("hair.limit_time");

	if (expire < get_global_time())
		return false;

	// set apply string
	offset = snprintf(buf, bufsiz, FN_point_string(aff->bApplyOn), aff->lApplyValue);

	if (offset < 0 || offset >= (int) bufsiz)
		offset = bufsiz - 1;

	localtime_r(&expire, &ltm);

	year	= ltm.tm_year + 1900;
	mon		= ltm.tm_mon + 1;
	day		= ltm.tm_mday;

	snprintf(buf + offset, bufsiz - offset, LC_TEXT(" (만료일 : %d년 %d월 %d일)"), year, mon, day);

	return true;
}

ACMD(do_costume)
{
	char buf[1024]; // @warme015
	const size_t bufferSize = sizeof(buf);

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	CItem* pBody = ch->GetWear(WEAR_COSTUME_BODY);
	CItem* pHair = ch->GetWear(WEAR_COSTUME_HAIR);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	CItem* pMount = ch->GetWear(WEAR_COSTUME_MOUNT);
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	CItem* pAcce = ch->GetWear(WEAR_COSTUME_ACCE);
#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	CItem* pWeapon = ch->GetWear(WEAR_COSTUME_WEAPON);
#endif

	ch->ChatPacket(CHAT_TYPE_INFO, "COSTUME status:");

	if (pHair)
	{
		const char* itemName = pHair->GetLocaleName(ch->GetLanguage());
		ch->ChatPacket(CHAT_TYPE_INFO, "  HAIR : %s", itemName);

		for (int i = 0; i < pHair->GetAttributeCount(); ++i)
		{
			const TPlayerItemAttribute& attr = pHair->GetAttribute(i);
			if (0 < attr.bType)
			{
				snprintf(buf, bufferSize, FN_point_string(attr.bType), attr.sValue);
				ch->ChatPacket(CHAT_TYPE_INFO, "	 %s", buf);
			}
		}

		if (pHair->IsEquipped() && arg1[0] == 'h')
			ch->UnequipItem(pHair);
	}

	if (pBody)
	{
		const char* itemName = pBody->GetLocaleName(ch->GetLanguage());
		ch->ChatPacket(CHAT_TYPE_INFO, "  BODY : %s", itemName);

		if (pBody->IsEquipped() && arg1[0] == 'b')
			ch->UnequipItem(pBody);
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (pMount)
	{
		const char* itemName = pMount->GetLocaleName(ch->GetLanguage());
		ch->ChatPacket(CHAT_TYPE_INFO, "  MOUNT : %s", itemName);

		if (pMount->IsEquipped() && arg1[0] == 'm')
			ch->UnequipItem(pMount);
	}
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if (pAcce)
	{
		const char* itemName = pAcce->GetLocaleName(ch->GetLanguage());
		ch->ChatPacket(CHAT_TYPE_INFO, "  ACCE : %s", itemName);

		if (pAcce->IsEquipped() && arg1[0] == 'a')
			ch->UnequipItem(pAcce);
	}
#endif

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (pWeapon)
	{
		const char* itemName = pWeapon->GetLocaleName(ch->GetLanguage());
		ch->ChatPacket(CHAT_TYPE_INFO, "  WEAPON : %s", itemName);

		if (pWeapon->IsEquipped() && arg1[0] == 'w')
			ch->UnequipItem(pWeapon);
	}
#endif
}

ACMD(do_hair)
{
	char buf[256];

	if (false == FN_hair_affect_string(ch, buf, sizeof(buf)))
		return;

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
}

ACMD(do_inventory)
{
	int	index = 0;
	int	count		= 1;

	char arg1[256];
	char arg2[256];

	LPITEM	item;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: inventory <start_index> <count>");
		return;
	}

	if (!*arg2)
	{
		index = 0;
		str_to_number(count, arg1);
	}
	else
	{
		str_to_number(index, arg1); index = MIN(index, INVENTORY_MAX_NUM);
		str_to_number(count, arg2); count = MIN(count, INVENTORY_MAX_NUM);
	}

	for (int i = 0; i < count; ++i)
	{
		if (index >= INVENTORY_MAX_NUM)
			break;

		item = ch->GetInventoryItem(index);

		ch->ChatPacket(CHAT_TYPE_INFO, "inventory [%d] = %s",
						index, item ? item->GetLocaleName(ch->GetLanguage()) : "<NONE>");
		++index;
	}
}

//gift notify quest command
ACMD(do_gift)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "gift");
}


#ifdef __SASH_SKIN__
ACMD(do_sash_skin)
{
	if (ch->GetExchange() || !ch->CanHandleItem())
		return;

	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	if (vecArgs[1] == "upgrade")
	{
		if (vecArgs.size() < 5) { return; }
		BYTE window_type, improve_full;
		WORD slotIdx;
		if (!str_to_number(window_type, vecArgs[2].c_str()) || !str_to_number(slotIdx, vecArgs[3].c_str()) || !str_to_number(improve_full, vecArgs[4].c_str()))
			return;

		if (improve_full)
		{
			BYTE totalRefineCount = 0;
			while (1)
			{
				LPITEM dreamItem = ch->GetItem(TItemPos(window_type, slotIdx));
				if (!dreamItem)
					return;
				if (ch->UpgradeDreamSoul(dreamItem, false))
					totalRefineCount += 1;
				else
					break;
			}
			if (totalRefineCount)
				ch->ChatPacket(CHAT_TYPE_INFO, "Total refine count: %u", totalRefineCount);
		}
		else
		{
			LPITEM dreamItem = ch->GetItem(TItemPos(window_type, slotIdx));
			if (!dreamItem)
				return;
			ch->UpgradeDreamSoul(dreamItem);
		}
	}
	else if (vecArgs[1] == "combine")
	{
		if (vecArgs.size() < 6) { return; }

		BYTE mainWindowType, targetWindowType;
		WORD mainSlotIdx, targetSlotIdx;
		if (!str_to_number(mainWindowType, vecArgs[2].c_str()) || !str_to_number(mainSlotIdx, vecArgs[3].c_str()) || !str_to_number(targetWindowType, vecArgs[4].c_str()) || !str_to_number(targetSlotIdx, vecArgs[5].c_str()))
			return;

		LPITEM mainItem = ch->GetItem(TItemPos(mainWindowType, mainSlotIdx));
		if (!mainItem)
			return;
		else if (mainItem->IsEquipped())
			return;
		else if (!mainItem->IsCostumeSashSkin())
			return;
		else if (mainItem->GetValue(0) >= 3)
			return;

		LPITEM targetItem = ch->GetItem(TItemPos(targetWindowType, targetSlotIdx));
		if (!targetItem)
			return;
		else if (targetItem->IsEquipped())
			return;
		else if (!targetItem->IsCostumeSashSkin())
			return;
		else if (mainItem->GetValue(0) >= 4)
			return;

		if (mainItem == targetItem)
			return;

		//#if this false will be checking same item index
		constexpr bool CHECK_SAME_GRADE_SASH_SKIN = true;
		if (CHECK_SAME_GRADE_SASH_SKIN)
		{
			if (mainItem->GetValue(0) != targetItem->GetValue(0))
				return;
		}
		else
		{
			if (mainItem->GetVnum() != targetItem->GetVnum())
				return;
		}

		const auto it = m_CombinePrices.find(mainItem->GetValue(0));
		if (it != m_CombinePrices.end())
		{
			if (ch->GetGold() < it->second)
				return;
			ch->PointChange(POINT_GOLD, -static_cast<int64_t>(it->second));
		}

		targetItem->SetCount(0);

		if (mainItem->GetValue(1) > number(1, 100)) //success
		{
			LPITEM newItem = ITEM_MANAGER::Instance().CreateItem(mainItem->GetVnum() + 1);
			if (newItem)
			{
				newItem->SetSockets(mainItem->GetSockets());
				newItem->SetAttributes(mainItem->GetAttributes());
				mainItem->SetCount(0);
				newItem->AddToCharacter(ch, TItemPos(mainWindowType, mainSlotIdx));
				newItem->Save();
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Sash skin combine sucessfully"));
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Something went wrong for create new item!"));
			}
		}
		else
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Sash skin combine failed"));
		ch->ChatPacket(CHAT_TYPE_COMMAND, "SashSkinRefresh %u %u", mainWindowType, mainSlotIdx);
	}
}
#endif

#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
ACMD(do_cube)
{

	const char *line;
	char arg1[256], arg2[256], arg3[256];
	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
	{
		return;
	}

	switch (LOWER(arg1[0]))
	{
		case 'o':	// open
			Cube_open(ch);
			break;

		default:
			return;
	}
}
#else
ACMD(do_cube)
{
	if (!ch->CanDoCube())
		return;

	dev_log(LOG_DEB0, "CUBE COMMAND <%s>: %s", ch->GetName(), argument);
	int cube_index = 0, inven_index = 0;
	const char *line;

	int inven_type = 0;
	
	char arg1[256], arg2[256], arg3[256];
	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
	{
		// print usage
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: cube open");
		ch->ChatPacket(CHAT_TYPE_INFO, "	   cube close");
		ch->ChatPacket(CHAT_TYPE_INFO, "	   cube add <inveltory_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "	   cube delete <cube_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "	   cube list");
		ch->ChatPacket(CHAT_TYPE_INFO, "	   cube cancel");
		ch->ChatPacket(CHAT_TYPE_INFO, "	   cube make [all]");
		return;
	}

	const std::string& strArg1 = std::string(arg1);

	if (strArg1 == "r_info")
	{
		if (0 == arg2[0])
			Cube_request_result_list(ch);
		else
		{
			if (isdigit(*arg2))
			{
				int listIndex = 0, requestCount = 1;
				str_to_number(listIndex, arg2);

				if (0 != arg3[0] && isdigit(*arg3))
					str_to_number(requestCount, arg3);

				Cube_request_material_info(ch, listIndex, requestCount);
			}
		}

		return;
	}

	switch (LOWER(arg1[0]))
	{
		case 'o':	// open
			Cube_open(ch);
			break;

		case 'c':	// close
			Cube_close(ch);
			break;

		case 'l':	// list
			Cube_show_list(ch);
			break;

		case 'a':	// add cue_index inven_index
			{
				if (0 == arg2[0] || !isdigit(*arg2) || 0 == arg3[0] || !isdigit(*arg3))
					return;

				str_to_number(cube_index, arg2);
				str_to_number(inven_index, arg3);
				Cube_add_item (ch, cube_index, inven_index);
			}
			break;

		case 'd':	// delete
			{
				if (0 == arg2[0] || !isdigit(*arg2))
					return;

				str_to_number(cube_index, arg2);
				Cube_delete_item (ch, cube_index);
			}
			break;

		case 'm':	// make
			if (0 != arg2[0])
			{
				while (true == Cube_make(ch))
					dev_log (LOG_DEB0, "cube make success");
			}
			else
				Cube_make(ch);
			break;

		default:
			return;
	}
}
#endif

#if defined(__MINI_GAME_OKEY__)
ACMD(do_cards)
{
	const char* line;
	char arg1[256], arg2[256];
	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	switch (LOWER(arg1[0]))
	{
	case 'o': // open
		if (isdigit(*arg2))
		{
			DWORD safemode;
			str_to_number(safemode, arg2);
			ch->Cards_open(safemode);
		}
		break;

	case 'p': // open
		ch->Cards_pullout();
		break;

	case 'e': // open
		ch->CardsEnd();
		break;

	case 'd': // open
		if (isdigit(*arg2))
		{
			DWORD destroy_index;
			str_to_number(destroy_index, arg2);
			ch->CardsDestroy(destroy_index);
		}
		break;

	case 'a': // open
		if (isdigit(*arg2))
		{
			DWORD accpet_index;
			str_to_number(accpet_index, arg2);
			ch->CardsAccept(accpet_index);
		}
		break;

	case 'r': // open
		if (isdigit(*arg2))
		{
			DWORD restore_index;
			str_to_number(restore_index, arg2);
			ch->CardsRestore(restore_index);
		}
		break;

	default:
		return;
	}
}
#endif

#if defined(__GEM_MARKET_SYSTEM__)
ACMD(do_gem)
{
	char arg1[255];
	char arg2[255];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (0 == arg1[0])
		return;

	const std::string& strArg1 = std::string(arg1);

	if (strArg1 == "craft")
	{
		if (0 == arg2[0])
			return;

		int slot = atoi(arg2);
		ch->CraftGemItems(slot);

	}
	else if (strArg1 == "market")
	{

		if (0 == arg2[0])
			return;

		int slot = atoi(arg2);
		ch->MarketGemItems(slot);

	}
	else if (strArg1 == "refresh")
	{
		ch->RefreshGemItems();
	}
}
#endif


ACMD(do_in_game_mall)
{	
	if (!ch->GetDesc())
		return;

	char buf[512+1];
	char sas[33];
	MD5_CTX ctx;
	const char sas_key[] = "Cp8NHh";

	snprintf(buf, sizeof(buf), "%u%u%s", ch->GetPlayerID(), ch->GetAID(), sas_key);

	MD5Init(&ctx);
	MD5Update(&ctx, (const unsigned char *) buf, strlen(buf));
#ifdef __FreeBSD__
	MD5End(&ctx, sas);
#else
	static const char hex[] = "0123456789abcdef";
	unsigned char digest[16];
	MD5Final(digest, &ctx);
	int i;
	for (i = 0; i < 16; ++i)
	{
		sas[i+i] = hex[digest[i] >> 4];
		sas[i+i+1] = hex[digest[i] & 0x0f];
	}
	sas[i+i] = '\0';
#endif

	ch->ChatPacket(CHAT_TYPE_COMMAND, "mall https://www.%s/shop?pid=%u&lang=%s&sid=%d&sas=%s",g_strWebMallURL.c_str(),ch->GetPlayerID(), GetLocaleCodeName(ch->GetLanguage()), g_server_id, sas);
}

ACMD(do_dice)
{
	char arg1[256], arg2[256];
	int start = 1, end = 100;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1 && *arg2)
	{
		start = atoi(arg1);
		end = atoi(arg2);
	}
	else if (*arg1 && !*arg2)
	{
		start = 1;
		end = atoi(arg1);
	}

	end = MAX(start, end);
	start = MIN(start, end);

	int n = number(start, end);

#ifdef ENABLE_DICE_SYSTEM
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_DICE_INFO, LC_TEXT("%s님이 주사위를 굴려 %d가 나왔습니다. (%d-%d)"), ch->GetName(), n, start, end);
	else
		ch->ChatPacket(CHAT_TYPE_DICE_INFO, LC_TEXT("당신이 주사위를 굴려 %d가 나왔습니다. (%d-%d)"), n, start, end);
#else
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_INFO, LC_TEXT("%s님이 주사위를 굴려 %d가 나왔습니다. (%d-%d)"), ch->GetName(), n, start, end);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("당신이 주사위를 굴려 %d가 나왔습니다. (%d-%d)"), n, start, end);
#endif
}

#ifdef ENABLE_NEWSTUFF
ACMD(do_click_safebox)
{
	if (!ch)
		return;

	if (ch->PreventTradeWindow(WND_SAFEBOX, true))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to close the other windows in order to open the safebox."));
		return;
	}

	if ((ch->GetGMLevel() <= GM_PLAYER) && (ch->GetDungeon() || ch->GetWarMap()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot open the safebox in dungeon or at war."));
		return;
	}

	if (ch->GetSafeboxSize() < 0)
		ch->SetSafeboxSize(3);

	ch->SetSafeboxOpenPosition();
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeSafeboxPassword");
}
ACMD(do_force_logout)
{
	LPDESC pDesc=DESC_MANAGER::instance().FindByCharacterName(ch->GetName());
	if (!pDesc)
		return;
	pDesc->DelayedDisconnect(0);
}
#endif

ACMD(do_click_mall)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeMallPassword");
}

ACMD(do_ride)
{
	sys_log(1, "[DO_RIDE] start");
	if (ch->IsDead() || ch->IsStun())
		return;

#ifdef __RENEWAL_MOUNT__
	if (ch->MountBlockMap())
		return;
	time_t now = get_global_time();
	if (ch->GetProtectTime("mount.ride") > now)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("684 %d"), ch->GetProtectTime("mount.ride") - now);
		return;
	}
	ch->SetProtectTime("mount.ride", now + 2);
#endif

	{
		if (ch->IsHorseRiding())
		{
			sys_log(1, "[DO_RIDE] stop riding");
			ch->StopRiding();
			return;
		}

		if (ch->GetMountVnum())
		{
			sys_log(1, "[DO_RIDE] unmount");
			do_unmount(ch, NULL, 0, 0);
			return;
		}
	}

#ifdef __RENEWAL_MOUNT__
	if (ch->GetWear(WEAR_COSTUME_MOUNT))
	{
		ch->StartRiding();
		return;
	}
	else
#endif
	{
		if (ch->GetHorse() != NULL)
		{
			sys_log(1, "[DO_RIDE] start riding");
			ch->StartRiding();
			return;
		}

		for (BYTE i=0; i<INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = ch->GetInventoryItem(i);
			if (NULL == item)
				continue;
#ifdef __RENEWAL_MOUNT__
			else if (item->IsCostumeMountItem())
			{
				ch->EquipItem(item);
				return;
			}
#endif
			else if (item->IsRideItem())
			{
				ch->EquipItem(item);
				return;
			}
		}
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 먼저 소환해주세요."));
}


#if defined(__HIDE_COSTUME_SYSTEM__)
ACMD(do_hide_costume)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
		return;

	bool hidden = true;
	BYTE bPartPos = 0;
	BYTE bHidden = 0;

	str_to_number(bPartPos, arg1);

	if (*arg2)
	{
		str_to_number(bHidden, arg2);

		if (bHidden == 0)
			hidden = false;
	}

	if (ch->IsDead())
		return;

	bool bAttacking = (get_dword_time() - ch->GetLastAttackTime()) < 1500;
	bool bMoving = (get_dword_time() - ch->GetLastMoveTime()) < 1500;
	bool bDelayedCMD = false;

	if (ch->IsStateMove() || bAttacking || bMoving)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to stand still to hide your costume."));
		return;
	}

	if (bDelayedCMD)
	{
		int iPulse = thecore_pulse();
		if (iPulse - ch->GetHideCostumePulse() < passes_per_sec * 3)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to wait 3 seconds before you can hide your costume again."));
			return;
		}
		ch->SetHideCostumePulse(iPulse);
	}

	if (bPartPos == 1)
		ch->SetBodyCostumeHidden(hidden);
	else if (bPartPos == 2)
		ch->SetHairCostumeHidden(hidden);
	else if (bPartPos == 3)
		ch->SetAcceCostumeHidden(hidden);
	else if (bPartPos == 4)
		ch->SetWeaponCostumeHidden(hidden);
	else
		return;

	ch->UpdatePacket();
}
#endif

#if defined(__SORT_INVENTORY_ITEMS__) && defined(__SPECIAL_INVENTORY_SYSTEM__)
ACMD(do_sort_special_inventory)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	BYTE type = 0;
	str_to_number(type, arg1);

	ch->SortSpecialInventoryItems(type);
}
#endif

#ifdef ENABLE_MOVE_CHANNEL
ACMD(DoChangeChannel)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;

	WORD channel = 0;
	str_to_number(channel, arg1);
	if (!channel)
		return;

	ch->ChangeChannel(channel);
}
#endif

#ifdef ENABLE_MULTI_FARM_BLOCK
ACMD(do_multi_farm)
{
	if (!ch->GetDesc())
		return;
	if (ch->GetProtectTime("multi-farm") > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need be slow! You can try after %d second."), ch->GetProtectTime("multi-farm") - get_global_time());
		return;
	}
	ch->SetProtectTime("multi-farm", get_global_time() + 10);
	CHARACTER_MANAGER::Instance().CheckMultiFarmAccount(ch->GetDesc()->GetHostName(), ch->GetPlayerID(),ch->GetName(), !ch->GetRewardStatus());
}
#endif

#ifdef __RENEWAL_BRAVE_CAPE__
ACMD(do_brave_cape)
{
	ch->GetBraveCapeCMDCompare(argument);
}
#endif

#ifdef RENEWAL_MISSION_BOOKS
ACMD(do_missionbooks)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "load")
	{
		ch->SendMissionData();
	}
	else if (vecArgs[1] == "delete")
	{
		if (vecArgs.size() < 3) { return; }
		WORD missionID;
		str_to_number(missionID, vecArgs[2].c_str());
		ch->DeleteBookMission(missionID);
	}
	else if (vecArgs[1] == "reward")
	{
		if (vecArgs.size() < 3) { return; }
		WORD missionID;
		str_to_number(missionID, vecArgs[2].c_str());
		ch->RewardMissionBook(missionID);
	}
}
#endif

#ifdef ENABLE_EVENT_MANAGER
ACMD(do_event_manager)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "info")
	{
		CHARACTER_MANAGER::Instance().SendDataPlayer(ch);
	}
	else if (vecArgs[1] == "remove")
	{
		if (!ch->IsGM())
			return;

		if (vecArgs.size() < 3) { 
			
			ch->ChatPacket(CHAT_TYPE_INFO, "put the event index!!");
			return; 
		}

		BYTE removeIndex;
		str_to_number(removeIndex, vecArgs[2].c_str());

		if(CHARACTER_MANAGER::Instance().CloseEventManuel(removeIndex))
			ch->ChatPacket(CHAT_TYPE_INFO, "successfuly remove!");
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "dont has any event!");
	}
	else if (vecArgs[1] == "update")
	{
		if (!ch->IsGM())
			return;
		const BYTE subHeader = EVENT_MANAGER_UPDATE;
		//db_clientdesc->DBPacketHeader(HEADER_GD_EVENT_MANAGER, 0, sizeof(BYTE));
		//db_clientdesc->Packet(&subHeader, sizeof(BYTE));
		db_clientdesc->DBPacket(HEADER_GD_EVENT_MANAGER, 0, &subHeader, sizeof(BYTE));

		ch->ChatPacket(CHAT_TYPE_INFO, "successfully update!");
	}
}
#endif

#ifdef ENABLE_REWARD_SYSTEM
ACMD(do_update_reward_data)
{
	CHARACTER_MANAGER::Instance().SendRewardInfo(ch);
}
#endif

#ifdef ENABLE_BATTLE_PASS
ACMD(do_battle_pass)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "info")
	{
		ch->SetProtectTime("battlePassOpen", 1);
		sys_log(0, "do_battle_pass: step 1");
		CHARACTER_MANAGER::Instance().LoadBattlePassData(ch);
		sys_log(0, "do_battle_pass: step 2");
	}
	else if (vecArgs[1] == "reward")
		CHARACTER_MANAGER::Instance().CheckBattlePassReward(ch);
}
#endif

ACMD(do_zodiac_treasure)
{
	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	if (ch->GetMapIndex() != 366)
		return;

	DWORD insigniaItems[][2] = {
		{33001,1},
		{33002,1},
		{33003,1},
		{33004,1},
		{33005,1},
		{33006,1},
		{33007,1},
		{33008,1},
		{33009,1},
		{33010,1},
		{33011,1},
		{33012,1},
	};

	for (BYTE j = 0; j < _countof(insigniaItems); ++j)
	{
		if (ch->CountSpecifyItem(insigniaItems[j][0]) < insigniaItems[j][1])
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_QUEST(773, ch->GetLanguage()));
			return;
		}
	}
	for (BYTE j= 0; j < _countof(insigniaItems); ++j)
		ch->RemoveSpecifyItem(insigniaItems[j][0], insigniaItems[j][1]);
	ch->AutoGiveItem(33028, 1);
}

#ifdef ENABLE_WHEEL_OF_FORTUNE
ACMD(do_wheel_of_fortune)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }


	else if (vecArgs[1] == "start")
	{
		if (ch->CountSpecifyItem(506025) <= 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("775"));
			return;
		}
		else if (ch->GetProtectTime("WheelWorking") != 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("wheelwork"));
			return;
		}

		std::map<std::pair<long, long>, int> m_normal_item = {
			 {{30179,	1},	55},
			 {{30319,	1},	55},
			 {{30613,	1},	50},
			 {{31181,	1},	50},
			 {{71095,	1},	55},
			 {{71174,	1},	55},
			 {{67011,	1},	35},
			 {{55111,	1},	50},
			 {{55104,	1},	50},
			 {{55118,	1},	50},
			 {{55110,	1},	55},
			 {{55103,	1},	55},
			 {{55117,	1},	55},
			 {{72320,	5},	40},
			 {{55009,	1},	20},
			 {{70031,	1},	20},
			 {{30610,	1},	20},
			 {{18900,	1},	45},
			 {{351211,	1},	15},
			 {{50934,	1},	50},
			 {{50934,	5},	35},
			 {{72346,	1},	5},
			 {{55032,	1},	5},
			 {{71035,	1},	15},
			 {{63019,	1},	15},
			 {{401012,	1}, 2},
			 {{402020,	1}, 2},
			 {{402021,	1}, 2},
			 {{402019,	1}, 2},
			 {{403017,	1}, 2},
			 {{403018,	1}, 2},
			 {{403019,	1}, 2}
		};

		std::vector<std::pair<long, long>> m_send_items;

		while (true)
		{
			for (auto it = m_normal_item.begin(); it != m_normal_item.end(); ++it)
			{
				int randomEx = number(0,4);
				if (randomEx == 4)
				{
					int random = number(0,100);
					if (it->second >= random)
					{
						auto itFind = std::find(m_send_items.begin(), m_send_items.end(), it->first);
						if (itFind == m_send_items.end())
						{
							m_send_items.emplace_back(it->first.first, it->first.second);
							if (m_send_items.size() >= 10)
								break;
						}
					}
				}
			}
			if (m_send_items.size() >= 10)
				break;
		}

		std::string cmd_wheel = "";
		if (m_send_items.size())
		{
			for (auto it = m_send_items.begin(); it != m_send_items.end(); ++it)
			{
				cmd_wheel += std::to_string(it->first);
				cmd_wheel += "|";
				cmd_wheel += std::to_string(it->second);
				cmd_wheel += "#";
			}
		}

		int luckyWheel = number(0, 9);
		if(luckyWheel == 0)
			if(number(0,1) == 0)
				luckyWheel = number(0, 9);

		ch->SetProtectTime("WheelLuckyIndex", luckyWheel);
		ch->SetProtectTime("WheelLuckyItemVnum", m_send_items[luckyWheel].first);
		ch->SetProtectTime("WheelLuckyItemCount", m_send_items[luckyWheel].second);

		ch->SetProtectTime("WheelWorking", 1);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "SetItemData %s", cmd_wheel.c_str());
		ch->ChatPacket(CHAT_TYPE_COMMAND, "OnSetWhell %d", luckyWheel);
	}
	else if (vecArgs[1] == "done")
	{
		if (ch->GetProtectTime("WheelWorking") == 0)
			return;

		if (ch->CountSpecifyItem(506025) <= 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("775"));
			return;
		}
		ch->RemoveSpecifyItem(506025, 1);

		ch->AutoGiveItem(ch->GetProtectTime("WheelLuckyItemVnum"), ch->GetProtectTime("WheelLuckyItemCount"));
		ch->ChatPacket(CHAT_TYPE_COMMAND, "GetGiftData %d %d", ch->GetProtectTime("WheelLuckyItemVnum"), ch->GetProtectTime("WheelLuckyItemCount"));
		ch->SetProtectTime("WheelLuckyIndex", 0);
		ch->SetProtectTime("WheelLuckyItemVnum", 0);
		ch->SetProtectTime("WheelLuckyItemCount", 0);
		ch->SetProtectTime("WheelWorking", 0);
	}
}
#endif

ACMD(do_extend_inventory)
{
	if (!ch)
		return;

	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	char arg1[512];
	const char* rest = one_argument(argument, arg1, sizeof(arg1));
	switch (arg1[0])
	{
		case 'a':
		{
			ch->Update_Inventory(0);
		}
		break;
		case 'b':
		{
			ch->Update_Inventory(1);
		}
		break;
		case 'c':
		{
			ch->Update_Inventory(2);
		}
		break;
		case 'd':
		{
			ch->Update_Inventory(3);
		}
		break;
		case 'e':
		{
			ch->Update_Inventory(4);
		}
		break;
		case 'f':
		{
			ch->Update_Inventory(5);
		}
		break;
		case 'g':
		{
			ch->Update_Inventory(6);
		}
		break;
	}
}

#ifdef ENABLE_BIYOLOG
#include "shop_manager.h"
ACMD(do_open_shop)
{
	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }

	DWORD shopVnum;
	str_to_number(shopVnum, vecArgs[1].c_str());
	
	if (shopVnum < 1000 && !ch->IsGM())
		return;
		
	CShopManager::Instance().StartShopping(ch, NULL, shopVnum);
}

ACMD(do_bio)
{
	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	if (ch->GetLevel() < 30)
		return;

	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }

	int level = ch->GetQuestFlag("bio.level");

	if (level >= bio_max)
		return;
	else if (level < 1)
		return;
	else if (ch->GetLevel() < bio_data[level][0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("710"), bio_data[level][0]);
		return;
	}

	int count = ch->GetQuestFlag("bio.count");
	int time = ch->GetQuestFlag("bio.time");

	if (vecArgs[1] == "mission")
	{
		if (vecArgs.size() < 4) { return; }
		BYTE isOzut = 0;
		BYTE isUnutkanlik = 0;

		str_to_number(isOzut, vecArgs[2].c_str());
		str_to_number(isUnutkanlik, vecArgs[3].c_str());

		if (count < bio_data[level][2])
		{
			if (ch->CountSpecifyItem(bio_data[level][1]) < 1)
			{
				TItemTable* p = ITEM_MANAGER::instance().GetTable(bio_data[level][1]);
				if(p)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("711 %s"), LC_ITEM_NAME(bio_data[level][1], ch->GetLanguage()));
				return;
			}

			//bool isUnutkanlikDrink = false;

			if (isUnutkanlik)
			{
				if (ch->CountSpecifyItem(bio_data[level][16]) > 0 && time > get_global_time())
				{
					ch->RemoveSpecifyItem(bio_data[level][16], 1);
					//isUnutkanlikDrink = true;
				}
				else
				{
					if (time > get_global_time())
					{
						TItemTable* p = ITEM_MANAGER::instance().GetTable(bio_data[level][16]);
						if(p)
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("712 %s"), LC_ITEM_NAME(bio_data[level][1], ch->GetLanguage()));
						return;
					}
				}
			}
			else
			{
				if (time > get_global_time())
				{
					TItemTable* p = ITEM_MANAGER::instance().GetTable(bio_data[level][16]);
					if(p)
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("712 %s"), LC_ITEM_NAME(bio_data[level][1], ch->GetLanguage()));
					return;
				}
			}


			bool isOzutDrink = false;
			if (isOzut)
			{
				if (ch->CountSpecifyItem(bio_data[level][15]) > 0)
				{
					ch->RemoveSpecifyItem(bio_data[level][15], 1);
					isOzutDrink = true;
				}
			}
			
			int prob = isOzutDrink ? bio_data[level][4] + 30 : bio_data[level][4];
			ch->RemoveSpecifyItem(bio_data[level][1], 1);

			if (prob >= number(1, 100))
			{
				count += 1;
				time = get_global_time() + bio_data[level][3];
				ch->SetQuestFlag("bio.count", count);
				ch->SetQuestFlag("bio.time", time);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biodata %d %d %d", level, count, time);

				TItemTable* p = ITEM_MANAGER::instance().GetTable(bio_data[level][1]);
				if(p)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("713 %s"), LC_ITEM_NAME(bio_data[level][1], ch->GetLanguage()));
			}
			else
			{
				time = get_global_time() + bio_data[level][3];
				ch->SetQuestFlag("bio.time", time);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "biodata %d %d %d", level, count, time);
				
				TItemTable* p = ITEM_MANAGER::instance().GetTable(bio_data[level][1]);
				if(p)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("714 %s"), LC_ITEM_NAME(bio_data[level][1], ch->GetLanguage()));
			}

			if (bio_data[level][5] != 0)
			{
				if (count == bio_data[level][2])
					ch->ChatPacket(CHAT_TYPE_COMMAND, "biostone %d", level);
				return;
			}
		}

		if (bio_data[level][5] != 0)
		{
			if (count == bio_data[level][2])
			{
				if (ch->CountSpecifyItem(bio_data[level][5]) < 1)
				{
					TItemTable* p = ITEM_MANAGER::instance().GetTable(bio_data[level][5]);
					if(p)
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("715 %s"), LC_ITEM_NAME(bio_data[level][1], ch->GetLanguage()));
					return;
				}
				else
				{
					ch->RemoveSpecifyItem(bio_data[level][5], 1);
					ch->SetQuestFlag("bio.count", count+1);

					if (bio_data[level][14] == 0)
					{
						ch->SetQuestFlag("bio.count", 0);
						ch->SetQuestFlag("bio.level", level + 1);
						ch->SetQuestFlag("bio.time", 0);

						if (bio_data[level][6] != 0)
						{
							long value = bio_data[level][7];
							CAffect* affect = ch->FindAffect(AFFECT_COLLECT, bio_data[level][6]);
							if (affect) {
								value += affect->lApplyValue;
								ch->RemoveAffect(affect);
							}
							ch->AddAffect(AFFECT_COLLECT, bio_data[level][6], value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("717"));
						}

						if (bio_data[level][8] != 0)
						{
							long value = bio_data[level][9];
							CAffect* affect = ch->FindAffect(AFFECT_COLLECT, bio_data[level][8]);
							if (affect) {
								value += affect->lApplyValue;
								ch->RemoveAffect(affect);
							}
							ch->AddAffect(AFFECT_COLLECT, bio_data[level][8], value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("717"));
						}

						if (bio_data[level][10] != 0)
						{
							long value = bio_data[level][11];
							CAffect* affect = ch->FindAffect(AFFECT_COLLECT, bio_data[level][10]);
							if (affect) {
								value += affect->lApplyValue;
								ch->RemoveAffect(affect);
							}
							ch->AddAffect(AFFECT_COLLECT, bio_data[level][10], value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("717"));
						}

						if (bio_data[level][12] != 0)
						{
							long value = bio_data[level][13];
							CAffect* affect = ch->FindAffect(AFFECT_COLLECT, bio_data[level][12]);
							if (affect) {
								value += affect->lApplyValue;
								ch->RemoveAffect(affect);
							}
								
							ch->AddAffect(AFFECT_COLLECT, bio_data[level][12], value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("717"));
						}

						int newLevel = level + 1;
						if (newLevel >= bio_max)
						{
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("716"));
							ch->ChatPacket(CHAT_TYPE_COMMAND, "bioempty");
							return;
						}
						ch->ChatPacket(CHAT_TYPE_COMMAND, "biodata %d %d %d", newLevel,0,0);
					}
					else
					{
						ch->ChatPacket(CHAT_TYPE_COMMAND, "bioodul %d", level);
						return;
					}
				}
			}
			else if (count == bio_data[level][2]+1)
				return;
		}
		else
		{
			if (count == bio_data[level][2])
			{
				if (bio_data[level][14] == 0)
				{
					ch->SetQuestFlag("bio.count", 0);
					ch->SetQuestFlag("bio.level", level + 1);
					ch->SetQuestFlag("bio.time", 0);

					if (bio_data[level][6] != 0)
					{
						long value = bio_data[level][7];
						CAffect* affect = ch->FindAffect(AFFECT_COLLECT, bio_data[level][6]);
						if (affect) {
							value += affect->lApplyValue;
							ch->RemoveAffect(affect);
						}
						ch->AddAffect(AFFECT_COLLECT, bio_data[level][6], value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);

						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("717"));
					}

					if (bio_data[level][8] != 0)
					{
						long value = bio_data[level][9];
						CAffect* affect = ch->FindAffect(AFFECT_COLLECT, bio_data[level][8]);
						if (affect) {
							value += affect->lApplyValue;
							ch->RemoveAffect(affect);
						}
						ch->AddAffect(AFFECT_COLLECT, bio_data[level][8], value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);

						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("717"));
					}

					if (bio_data[level][10] != 0)
					{
						long value = bio_data[level][11];
						CAffect* affect = ch->FindAffect(AFFECT_COLLECT, bio_data[level][10]);
						if (affect) {
							value += affect->lApplyValue;
							ch->RemoveAffect(affect);
						}
						ch->AddAffect(AFFECT_COLLECT, bio_data[level][10], value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);

						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("717"));
					}

					if (bio_data[level][12] != 0)
					{
						long value = bio_data[level][13];
						CAffect* affect = ch->FindAffect(AFFECT_COLLECT, bio_data[level][12]);
						if (affect) {
							value += affect->lApplyValue;
							ch->RemoveAffect(affect);
						}
						ch->AddAffect(AFFECT_COLLECT, bio_data[level][12], value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);

						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("717"));
					}

					int newLevel = level + 1;
					if (newLevel >= bio_max)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("716"));
						ch->ChatPacket(CHAT_TYPE_COMMAND, "bioempty");
						return;
					}
					ch->ChatPacket(CHAT_TYPE_COMMAND, "biodata %d %d %d", newLevel, 0, 0);
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_COMMAND, "bioodul %d", level);
					return;
				}
			}
		}
	}
	else if (vecArgs[1] == "gift")
	{
		if (vecArgs.size() < 3) { return; }
		BYTE index = 0;
		str_to_number(index, vecArgs[2].c_str());

		if (index > 2)
			return;

		if (bio_data[level][5] != 0)
		{
			if (count != bio_data[level][2] + 1)
				return;
		}
		else
		{
			if (count != bio_data[level][2])
				return;
		}

		ch->SetQuestFlag("bio.count", 0);
		ch->SetQuestFlag("bio.level", level + 1);
		ch->SetQuestFlag("bio.time", 0);

		if (bio_data[level][6] != 0 && index == 0)
		{
			long value = bio_data[level][7];
			CAffect* affect = ch->FindAffect(AFFECT_COLLECT, bio_data[level][6]);
			if (affect) {
				value += affect->lApplyValue;
				ch->RemoveAffect(affect);
			}
			ch->AddAffect(AFFECT_COLLECT, bio_data[level][6], value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);

			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("717"));
		}

		if (bio_data[level][8] != 0 && index == 1)
		{
			long value = bio_data[level][9];
			CAffect* affect = ch->FindAffect(AFFECT_COLLECT, bio_data[level][8]);
			if (affect) {
				value += affect->lApplyValue;
				ch->RemoveAffect(affect);
			}
			ch->AddAffect(AFFECT_COLLECT, bio_data[level][8], value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);

			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("717"));
		}

		if (bio_data[level][10] != 0 && index == 2)
		{
			long value = bio_data[level][11];
			CAffect* affect = ch->FindAffect(AFFECT_COLLECT, bio_data[level][10]);
			if (affect) {
				value += affect->lApplyValue;
				ch->RemoveAffect(affect);
			}
			ch->AddAffect(AFFECT_COLLECT, bio_data[level][10], value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);

			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("717"));
		}

		char flag[100];
		sprintf(flag, "bio.bonus%d", level);
		ch->SetQuestFlag(flag, index + 1);

		int newLevel = level + 1;
		if (newLevel >= bio_max)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("716"));
			ch->ChatPacket(CHAT_TYPE_COMMAND, "bioempty");
			CHARACTER_MANAGER::Instance().SetRewardData(REWARD_BIOLOG, ch->GetName(), true);
			return;
		}

		ch->ChatPacket(CHAT_TYPE_COMMAND, "biodata %d %d %d", newLevel, 0, 0);
	}
}
#endif

ACMD(do_vote4buff)
{
	// split args
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 1) { return; }

	// spam function
	if (ch->GetProtectTime("vote4buff") > get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to wait %d s! Try again later."), ch->GetProtectTime("vote4buff") - get_global_time());
		return;
	}
	ch->SetProtectTime("vote4buff", get_global_time() + 10);

	if (ch->PreventTradeWindow(WND_ALL))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	// check for bonus
	DWORD bonusAffect;
	str_to_number(bonusAffect, vecArgs[1].c_str());

	if (bonusAffect < 1)
		return;

	// check for affect
	if (ch->FindAffect(AFFECT_SINGLE_BONUS_URIEL))
	{
		return ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This affect is already active."));
	}

	// check for hwid
    char szQueryHWID[256];
    snprintf(szQueryHWID, sizeof(szQueryHWID), "SELECT hwid FROM account.hwid WHERE account_id = '%d'", ch->GetDesc()->GetAccountTable().id);
    std::unique_ptr<SQLMsg> pMsgHWID(DBManager::instance().DirectQuery(szQueryHWID));

    if (pMsgHWID->Get()->uiNumRows == 0)
    {
        ch->ChatPacket(CHAT_TYPE_INFO, "Failed to retrieve HWID.");
        return;
    }
	auto rowHWID = mysql_fetch_row(pMsgHWID->Get()->pSQLResult);
    const char* szHWID = rowHWID[0];

	// check for vote
    char szQueryVote[512];
    snprintf(szQueryVote, sizeof(szQueryVote), "SELECT UNIX_TIMESTAMP(time) FROM account.vote4buff WHERE hwid = '%s'", szHWID);
    std::unique_ptr<SQLMsg> pMsgVote(DBManager::instance().DirectQuery(szQueryVote));

    if (pMsgVote->Get()->uiNumRows == 0)
    {
        ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성 추가에 실패하였습니다."));
        return;
    }
    auto rowVote = mysql_fetch_row(pMsgVote->Get()->pSQLResult);
    long dbVoteTime;
    str_to_number(dbVoteTime, rowVote[0]);

	// duration
   long duration = dbVoteTime + 24*60*60;
	if (duration <= get_global_time())
		return ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성 추가에 실패하였습니다."));

	// give bonus
	if (bonusAffect == 1)
	{
		ch->AddAffect(AFFECT_SINGLE_BONUS_URIEL, POINT_MALL_EXPBONUS, 20, 0, duration-dbVoteTime, 0, false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Succesfully added affect."));
	}
	else if (bonusAffect == 2)
	{
		ch->AddAffect(AFFECT_SINGLE_BONUS_URIEL, POINT_ATTBONUS_MONSTER, 10, 0, duration-dbVoteTime, 0, false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Succesfully added affect."));
	}
	else if (bonusAffect == 3)
	{
		ch->AddAffect(AFFECT_SINGLE_BONUS_URIEL, POINT_ITEM_DROP_BONUS, 20, 0, duration-dbVoteTime, 0, false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Succesfully added affect."));
	}
}

#if defined(ENABLE_RENEWAL_AFFECT_SHOWER)
ACMD(do_remove_affect)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	DWORD affect = 0;
	str_to_number(affect, vecArgs[1].c_str());
	if (AFFECT_POLYMORPH == affect)
	{
		if (!ch->IsPolymorphed())
			return;
		ch->SetPolymorph(0);
	}

	if (AFFECT_SINGLE_BONUS_URIEL == affect
		|| AFFECT_SINGLE_BONUS_URIEL_DELAY == affect
		|| AFFECT_CRYSTAL == affect
		|| AFFECT_MULTI_FARM_PREMIUM == affect
		|| AFFECT_DECORATION == affect
		|| AFFECT_BLEND_POTION_1 == affect
		|| AFFECT_BLEND_POTION_2 == affect
		|| AFFECT_BLEND_POTION_3 == affect
		|| AFFECT_BLEND_POTION_4 == affect
		|| AFFECT_BLEND_POTION_5 == affect
		|| AFFECT_BLEND_POTION_6 == affect
		|| AFFECT_DRAGON_GOD_1 == affect
		|| AFFECT_DRAGON_GOD_2 == affect
		|| AFFECT_DRAGON_GOD_3 == affect
		|| AFFECT_DRAGON_GOD_4 == affect
		|| AFFECT_CRITICAL == affect
		|| AFFECT_PENETRATE == affect
		|| AFFECT_ATT_WATER == affect
		|| AFFECT_DEF_WATER == affect
		|| AFFECT_FISH1 == affect
		|| AFFECT_FISH2 == affect
		|| AFFECT_FISH3 == affect
		|| AFFECT_FISH4 == affect
		|| AFFECT_FISH5 == affect
		|| AFFECT_FISH6 == affect
		|| AFFECT_FISH7 == affect
		|| AFFECT_FISH8 == affect
		|| AFFECT_FISH9 == affect
		|| AFFECT_FISH10 == affect
		|| AFFECT_FISH11 == affect
		|| AFFECT_FISH12 == affect
		|| AFFECT_FISH13 == affect
		|| AFFECT_FISH14 == affect
		|| AFFECT_BLEND_MONSTERS == affect
		|| AFFECT_ATTACK_SPEED == affect
		|| AFFECT_MOVE_SPEED == affect
		|| AFFECT_BRAVE_CAPE == affect
		|| AFFECT_PICKUP_ENABLE == affect
		|| AFFECT_PICKUP_DEACTIVE == affect
		|| AFFECT_CRYSTAL == affect
		|| AFFECT_POISON == affect
		|| AFFECT_STUN == affect
		|| AFFECT_SLOW == affect
		|| AFFECT_DUNGEON_READY == affect
		|| AFFECT_DUNGEON_UNIQUE == affect
		|| AFFECT_FIRE == affect
		|| AFFECT_BLOCK_CHAT == affect
		|| AFFECT_SINGLE_BONUS_URIEL == affect
		|| 66 == affect
	)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This affect can't be removed."));
		return;
	}

	if(ch->FindAffect(affect))
		ch->RemoveAffect(affect);
}
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
#include "PetSystem.h"
ACMD(do_change_name)
{
	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 3) { return; }
	if (vecArgs.size() > 12) { return; }
	int scrollPos = -1;
	int petPos = -1;
	str_to_number(scrollPos, vecArgs[1].c_str());
	str_to_number(petPos, vecArgs[2].c_str());
	if (scrollPos == -1 || petPos == -1 || vecArgs[3].empty() || vecArgs[3].size() < 3 || ch->GetGold() < 100000)
		return;
	LPITEM scroll = ch->GetInventoryItem(scrollPos);
	if (scroll)
	{
		//if ()
		{
			LPITEM pet = ch->GetInventoryItem(petPos);
			if (pet)
			{
				if (pet->IsEquipped())
					return;
				if (pet->GetType() == ITEM_PET && pet->GetSubType() == PET_LEVELABLE && scroll->GetVnum() == 55008)
				{
					ch->SetPoint(POINT_GOLD, -100000);
					scroll->SetCount(scroll->GetCount() - 1);
					//str_replace(vecArgs[3], ";", " ");
					if (!check_name(vecArgs[3].c_str()))
						return;
					ch->SetNewName(vecArgs[3].c_str());
					pet->UpdatePacket();
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("688"));
				}
			}
		}
	}
}
ACMD(do_pet_egg)
{
	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	if (ch->GetGold() < 100000)
		return;

	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	if (vecArgs.size() > 12) { return; }
	int petPos = -1;
	str_to_number(petPos, vecArgs[1].c_str());

	LPITEM item = ch->GetInventoryItem(petPos);
	if (!item)
		return;
	else if (!(item->GetType() == ITEM_PET && item->GetSubType() == PET_EGG))
		return;
	else if (!check_name(vecArgs[2].c_str()))
		return;

	LPITEM petItem = ch->AutoGiveItem(item->GetValue(0));
	if (!petItem)
		return;

	item->SetCount(item->GetCount()-1);
	ch->SetNewName(vecArgs[2].c_str());
	petItem->SetSocket(0,time(0)+60*60*24*7);
	petItem->SetSocket(1, 1);
	for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
		petItem->SetForceAttribute(j, 99, 0);
	petItem->Save();
	petItem->UpdatePacket();
}

#ifdef __BACK_DUNGEON__
ACMD(do_back_dungeon)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	DWORD mapIdx;
	if (!str_to_number(mapIdx, vecArgs[1].c_str()))
		return;

	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	CHARACTER_MANAGER::Instance().CheckBackDungeon(ch, BACK_DUNGEON_WARP, mapIdx);
}
#endif


ACMD(do_pet_evolve)
{
	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	LPITEM item = ch->GetWear(WEAR_PET);
	if(!item)
		return;
	long level = item->GetSocket(1);
	if (level != 40 && level != 75 && level != 100)
		return;
	CPetSystem* pet = ch->GetPetSystem();
	if (!pet)
		return;
	LPPET petActor = pet->GetNewPet();
	if (!petActor)
		return;
	petActor->IncreaseEvolve();
}
ACMD(do_pet_add_slot)
{
	if (ch->PreventTradeWindow(WND_ALL)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows and wait a few seconds, before using this."));
		return;
	}

	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	int skillSlot = -1;
	str_to_number(skillSlot, vecArgs[1].c_str());
	if (skillSlot >= 0 && skillSlot <= 14)
	{
		CPetSystem* pet = ch->GetPetSystem();
		if (!pet)
			return;
		LPPET petActor = pet->GetNewPet();
		if (!petActor)
			return;
		LPITEM item = petActor->GetSummonItem();
		if (!item)
			return;
		if (item->GetSocket(1) < 115)
			return;

		int nextIndex = -1;
		for (int j = 3; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
		{
			if (item->GetAttributeType(j) == 99) {
				nextIndex = j;
				break;
			}
		}

		if (nextIndex < 0)
			return;

		DWORD slotItems[][2] = {
			{55031,1},
		};
		for (BYTE j=0;j<_countof(slotItems);++j)
		{
			if (ch->CountSpecifyItem(slotItems[j][0]) < slotItems[j][1])
			{
				TItemTable* p = ITEM_MANAGER::instance().GetTable(slotItems[j][0]);
				if(p)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("730 %s %d"), LC_ITEM_NAME(slotItems[j][0], ch->GetLanguage()), slotItems[j][1]);
				return;
			}
		}
		for (BYTE j = 0; j < _countof(slotItems); ++j)
			ch->RemoveSpecifyItem(slotItems[j][0],slotItems[j][1]);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("733"));
		petActor->PointChange(POINT_PET_SKILL_INDEX_1 + nextIndex, 0);
	}
}
#endif

#ifdef __BUFFI_SUPPORT__
ACMD(do_buffi_costume_page)
{
	char arg[256];
	one_argument(argument, arg, sizeof(arg));
	if (!*arg)
		return;
	int iPageIdx;
	if (!str_to_number(iPageIdx, arg))
		return;
	ch->SetLookingBuffiPage(iPageIdx == 1 ? true : false);
}
#endif

//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
