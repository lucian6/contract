#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "shop.h"
#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "monarch.h"
#include "mob_manager.h"
#include "locale_service.h"
#include "desc_client.h"
#include "shopEx.h"
#include "group_text_parse_tree.h"

bool CShopEx::Create(DWORD dwVnum, DWORD dwNPCVnum)
{
	m_dwVnum = dwVnum;
	m_dwNPCVnum = dwNPCVnum;
	return true;
}

bool CShopEx::AddShopTable(TShopTableEx& shopTable)
{
	for (itertype(m_vec_shopTabs) it = m_vec_shopTabs.begin(); it != m_vec_shopTabs.end(); it++)
	{
		const TShopTableEx& _shopTable = *it;
		if (0 != _shopTable.dwVnum && _shopTable.dwVnum == shopTable.dwVnum)
			return false;
		if (0 != _shopTable.dwNPCVnum && _shopTable.dwNPCVnum == shopTable.dwVnum)
			return false;
	}
	m_vec_shopTabs.emplace_back(shopTable);
	return true;
}

bool CShopEx::AddGuest(LPCHARACTER ch,DWORD owner_vid, bool bOtherEmpire)
{
	if (!ch)
		return false;

	if (ch->GetExchange())
		return false;

	if (ch->GetShop())
		return false;

	ch->SetShop(this);

	m_map_guest.emplace(ch, bOtherEmpire);

	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_START_EX;

	TPacketGCShopStartEx pack2;

	memset(&pack2, 0, sizeof(pack2));

	pack2.owner_vid = owner_vid;
	pack2.shop_tab_count = m_vec_shopTabs.size();

#if defined(__SHOPEX_RENEWAL__)
	char temp[8096 * 8]; // ?? 1728 * 3
#else
	char temp[8096]; // ?? 1728 * 3
#endif
	char* buf = &temp[0];
	size_t size = 0;
	for (itertype(m_vec_shopTabs) it = m_vec_shopTabs.begin(); it != m_vec_shopTabs.end(); it++)
	{
		const TShopTableEx& shop_tab = *it;
		TPacketGCShopStartEx::TSubPacketShopTab pack_tab;
		pack_tab.coin_type = shop_tab.coinType;
		memcpy(pack_tab.name, shop_tab.name.c_str(), SHOP_TAB_NAME_MAX);

		for (DWORD i = 0; i < SHOP_HOST_ITEM_MAX_NUM; ++i)
		{
			pack_tab.items[i].vnum = shop_tab.items[i].vnum;
			pack_tab.items[i].count = shop_tab.items[i].count;

			pack_tab.items[i].price = shop_tab.items[i].price;
#if defined(__SHOPEX_RENEWAL__)
			pack_tab.items[i].price1 = shop_tab.items[i].price1 > 0 ? shop_tab.items[i].price1 : 0;
			pack_tab.items[i].price2 =shop_tab.items[i].price2 > 0 ? shop_tab.items[i].price2 : 0;
			pack_tab.items[i].price3 =shop_tab.items[i].price3 > 0 ? shop_tab.items[i].price3 : 0;
			pack_tab.items[i].price4 =shop_tab.items[i].price4 > 0 ? shop_tab.items[i].price4 : 0;
#endif
#if defined(__SHOPEX_RENEWAL__)
			pack_tab.items[i].price_type = shop_tab.items[i].price_type;
			pack_tab.items[i].price_vnum = shop_tab.items[i].price_vnum;
			pack_tab.items[i].price_vnum1 = shop_tab.items[i].price_vnum1 > 0 ? shop_tab.items[i].price_vnum1 : 0;
			pack_tab.items[i].price_vnum2 = shop_tab.items[i].price_vnum2 > 0 ? shop_tab.items[i].price_vnum2 : 0;
			pack_tab.items[i].price_vnum3 = shop_tab.items[i].price_vnum3 > 0 ? shop_tab.items[i].price_vnum3 : 0;
			pack_tab.items[i].price_vnum4 = shop_tab.items[i].price_vnum4 > 0 ? shop_tab.items[i].price_vnum4 : 0;
			thecore_memcpy(pack_tab.items[i].aAttr, shop_tab.items[i].aAttr, sizeof(pack_tab.items[i].aAttr));
			thecore_memcpy(pack_tab.items[i].alSockets, shop_tab.items[i].alSockets, sizeof(pack_tab.items[i].alSockets));
#else
			memset(pack_tab.items[i].aAttr, 0, sizeof(pack_tab.items[i].aAttr));
			memset(pack_tab.items[i].alSockets, 0, sizeof(pack_tab.items[i].alSockets));
#endif
		}

		memcpy(buf, &pack_tab, sizeof(pack_tab));
		buf += sizeof(pack_tab);
		size += sizeof(pack_tab);
	}

	pack.size = sizeof(pack) + sizeof(pack2) + size;

	ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCShop));
	ch->GetDesc()->BufferedPacket(&pack2, sizeof(TPacketGCShopStartEx));
	ch->GetDesc()->Packet(temp, size);

	return true;
}

int CShopEx::Buy(LPCHARACTER ch, BYTE pos, int dwCount)
{
	BYTE tabIdx = pos / SHOP_HOST_ITEM_MAX_NUM;
	BYTE slotPos = pos % SHOP_HOST_ITEM_MAX_NUM;
	if (tabIdx >= GetTabCount())
	{
		sys_log(0, "ShopEx::Buy : invalid position %d : %s", pos, ch->GetName());
		return SHOP_SUBHEADER_GC_INVALID_POS;
	}

	sys_log(0, "ShopEx::Buy : name %s pos %d", ch->GetName(), pos);

	GuestMapType::iterator it = m_map_guest.find(ch);

	if (it == m_map_guest.end())
		return SHOP_SUBHEADER_GC_END;

	TShopTableEx& shopTab = m_vec_shopTabs[tabIdx];
	TShopItemTable& r_item = shopTab.items[slotPos];

	if (r_item.price <= 0)
	{
		LogManager::instance().HackLog("SHOP_BUY_GOLD_OVERFLOW", ch);
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
	}

	DWORD dwPrice = r_item.price;
	DWORD dwPrice1 = r_item.price1;
	DWORD dwPrice2 = r_item.price2;
	DWORD dwPrice3 = r_item.price3;
	DWORD dwPrice4 = r_item.price4;

	for (int i = 0; i < dwCount; i++)
	{
		switch (r_item.price_type)
		{
		case EX_GOLD:
			if (static_cast<int64_t>(ch->GetGold()) < dwPrice)
				return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
			break;
		case EX_SECONDARY:
			if (static_cast<INT>(ch->GetGem()) < dwPrice)
				return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY_EX;
			break;			
		case EX_ITEM:
			if (dwPrice > 0 && dwPrice1 > 0 && dwPrice2 > 0 && dwPrice3 > 0 && dwPrice4 > 0)
			{
				if ((ch->CountSpecifyItem(r_item.price_vnum) < dwPrice) ||
					(ch->CountSpecifyItem(r_item.price_vnum1) < dwPrice1) ||
					(ch->CountSpecifyItem(r_item.price_vnum2) < dwPrice2) ||
					(ch->CountSpecifyItem(r_item.price_vnum3) < dwPrice3) ||
					(ch->CountSpecifyItem(r_item.price_vnum4) < dwPrice4))
					return SHOP_SUBHEADER_GC_NOT_ENOUGH_ITEM;
			}
			else if(dwPrice > 0 && dwPrice1 > 0 && dwPrice2 > 0 && dwPrice3 > 0 && dwPrice4 == 0)
			{
				if ((ch->CountSpecifyItem(r_item.price_vnum) < dwPrice) ||
					(ch->CountSpecifyItem(r_item.price_vnum1) < dwPrice1) ||
					(ch->CountSpecifyItem(r_item.price_vnum2) < dwPrice2) ||
					(ch->CountSpecifyItem(r_item.price_vnum3) < dwPrice3))
					return SHOP_SUBHEADER_GC_NOT_ENOUGH_ITEM;
			}
			else if(dwPrice > 0 && dwPrice1 > 0 && dwPrice2 > 0 && dwPrice3 == 0 && dwPrice4 == 0)
			{
				if ((ch->CountSpecifyItem(r_item.price_vnum) < dwPrice) ||
					(ch->CountSpecifyItem(r_item.price_vnum1) < dwPrice1) ||
					(ch->CountSpecifyItem(r_item.price_vnum2) < dwPrice2))
					return SHOP_SUBHEADER_GC_NOT_ENOUGH_ITEM;
			}
			else if(dwPrice > 0 && dwPrice1 > 0 && dwPrice2 == 0 && dwPrice3 == 0 && dwPrice4 == 0)
			{
				if ((ch->CountSpecifyItem(r_item.price_vnum) < dwPrice) ||
					(ch->CountSpecifyItem(r_item.price_vnum1) < dwPrice1))
					return SHOP_SUBHEADER_GC_NOT_ENOUGH_ITEM;
			}
			else if(dwPrice > 0 && dwPrice1 == 0 && dwPrice2 == 0 && dwPrice3 == 0 && dwPrice4 == 0)
			{
				if (ch->CountSpecifyItem(r_item.price_vnum) < dwPrice)
					return SHOP_SUBHEADER_GC_NOT_ENOUGH_ITEM;
			}
			break;
		case EX_EXP:
			if (ch->GetGem() < dwPrice)
				return SHOP_SUBHEADER_GC_NOT_ENOUGH_EXP;
			break;
		}

		LPITEM item;

		item = ITEM_MANAGER::instance().CreateItem(r_item.vnum, r_item.count);

		if (!item)
			return SHOP_SUBHEADER_GC_SOLD_OUT;

		int iEmptyPos;
		if (item->IsDragonSoul())
		{
			iEmptyPos = ch->GetEmptyDragonSoulInventory(item);
		}
	#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		else if (item->IsSkillBook())
		{
			iEmptyPos = ch->GetEmptySkillBookInventory(item->GetSize());
		}
		else if (item->IsUpgradeItem())
		{
			iEmptyPos = ch->GetEmptyUpgradeItemsInventory(item->GetSize());
		}
		else if (item->IsStone())
		{
			iEmptyPos = ch->GetEmptyStoneInventory(item->GetSize());
		}
		else if (item->IsGiftBox())
		{
			iEmptyPos = ch->GetEmptyGiftBoxInventory(item->GetSize());
		}
		else if (item->IsSwitch())
		{
			iEmptyPos = ch->GetEmptySwitchInventory(item->GetSize());
		}
	#endif
		else
		{
			iEmptyPos = ch->GetEmptyInventory(item->GetSize());
		}

		if (iEmptyPos < 0)
		{
			sys_log(1, "ShopEx::Buy : Inventory full : %s size %d", ch->GetName(), item->GetSize());
			M2_DESTROY_ITEM(item);
			return SHOP_SUBHEADER_GC_INVENTORY_FULL;
		}

	#if defined(__SHOPEX_RENEWAL__)
		switch (r_item.price_type)
		{
			case EX_GOLD:
				ch->PointChange(POINT_GOLD, -static_cast<int64_t>(dwPrice), false);
				CHARACTER_MANAGER::Instance().DoMission(ch, MISSION_SPEND_MONEY, dwPrice, 0);
				break;
			case EX_SECONDARY:
				ch->PointChange(POINT_GEM, -static_cast<int64_t>(dwPrice), false);
				break;
			case EX_ITEM:
				ch->RemoveSpecifyItem(r_item.price_vnum, dwPrice);
				if (r_item.price_vnum1 > 0)
					ch->RemoveSpecifyItem(r_item.price_vnum1, dwPrice1);
				if (r_item.price_vnum2 > 0)
					ch->RemoveSpecifyItem(r_item.price_vnum2, dwPrice2);
				if (r_item.price_vnum3 > 0)
					ch->RemoveSpecifyItem(r_item.price_vnum3, dwPrice3);
				if (r_item.price_vnum4 > 0)
					ch->RemoveSpecifyItem(r_item.price_vnum4, dwPrice4);
				break;

			case EX_EXP:
				ch->PointChange(POINT_GEM, -static_cast<int64_t>(dwPrice), false);
				break;
		}
		if (!std::all_of(std::begin(r_item.aAttr), std::end(r_item.aAttr), [](const TPlayerItemAttribute& s) { return !s.bType; }))
			item->SetAttributes(r_item.aAttr);
		if (!std::all_of(std::begin(r_item.alSockets), std::end(r_item.alSockets), [](const long& s) { return !s; }))
			item->SetSockets(r_item.alSockets);
	#else
		switch (shopTab.coinType)
		{
		case SHOP_COIN_TYPE_GOLD:
			ch->PointChange(POINT_GOLD, -dwPrice, false);
			break;
		case SHOP_COIN_TYPE_SECONDARY_COIN:
			ch->RemoveSpecifyTypeItem(ITEM_SECONDARY_COIN, dwPrice);
			break;
		}
	#endif

		if (item->IsDragonSoul())
			item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
		else
		{
			WORD bCount = item->GetCount();
			
			if (IS_SET(item->GetFlag(), ITEM_FLAG_STACKABLE) && !item->IsEquipped())
			{
				for (WORD i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
				{
					LPITEM item2 = ch->GetInventoryItem(i);

					if (!item2)
						continue;

					if (item2->GetCount() == ITEM_MAX_COUNT)
						continue;

					if (item2->GetVnum() == item->GetVnum())
					{
						int j;

						for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
							if (item2->GetSocket(j) != item->GetSocket(j))
								break;

						if (j != ITEM_SOCKET_MAX_NUM)
							continue;

						WORD bCount2 = MIN(ITEM_MAX_COUNT - item2->GetCount(), bCount);
						bCount -= bCount2;
						item2->SetCount(item2->GetCount() + bCount2);

						if (bCount == 0)
							break;
					}
				}

				item->SetCount(bCount);
			}
			
			if (bCount > 0)
				item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
			else
				M2_DESTROY_ITEM(item);

			if (item && item->GetVnum() == 51010)
				CHARACTER_MANAGER::Instance().SetRewardData(REWARD_CRYSTAL,ch->GetName(), true);
		}

		ITEM_MANAGER::instance().FlushDelayedSave(item);

		DBManager::instance().SendMoneyLog(MONEY_LOG_SHOP, item->GetVnum(), -dwPrice);
	}

	ch->Save();

    return (SHOP_SUBHEADER_GC_OK);
}
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
