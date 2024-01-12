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

//#define ENABLE_SHOP_BLACKLIST
/* ------------------------------------------------------------------------------------ */
CShop::CShop()
	: m_dwVnum(0), m_dwNPCVnum(0), m_pkPC(NULL)
{
	m_pGrid = M2_NEW CGrid(5, 9);
}

CShop::~CShop()
{
	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_END;
	pack.size		= sizeof(TPacketGCShop);

	Broadcast(&pack, sizeof(pack));

	GuestMapType::iterator it;

	it = m_map_guest.begin();

	while (it != m_map_guest.end())
	{
		LPCHARACTER ch = it->first;
		ch->SetShop(NULL);
		++it;
	}

	M2_DELETE(m_pGrid);
}

void CShop::SetPCShop(LPCHARACTER ch)
{
	m_pkPC = ch;
}

bool CShop::Create(DWORD dwVnum, DWORD dwNPCVnum, TShopItemTable * pTable)
{
	/*
	   if (NULL == CMobManager::instance().Get(dwNPCVnum))
	   {
	   sys_err("No such a npc by vnum %d", dwNPCVnum);
	   return false;
	   }
	 */
	sys_log(0, "SHOP #%d (Shopkeeper %d)", dwVnum, dwNPCVnum);

	m_dwVnum = dwVnum;
	m_dwNPCVnum = dwNPCVnum;

	BYTE bItemCount;

	for (bItemCount = 0; bItemCount < SHOP_HOST_ITEM_MAX_NUM; ++bItemCount)
		if (0 == (pTable + bItemCount)->vnum)
			break;

	SetShopItems(pTable, bItemCount);
	return true;
}

void CShop::SetShopItems(TShopItemTable * pTable, BYTE bItemCount)
{
	if (bItemCount > SHOP_HOST_ITEM_MAX_NUM)
		return;

	m_pGrid->Clear();

	m_itemVector.resize(SHOP_HOST_ITEM_MAX_NUM);
	memset(&m_itemVector[0], 0, sizeof(SHOP_ITEM) * m_itemVector.size());

	for (int i = 0; i < bItemCount; ++i)
	{
		LPITEM pkItem = NULL;
		const TItemTable * item_table;

		if (m_pkPC)
		{
			pkItem = m_pkPC->GetItem(pTable->pos);

			if (!pkItem)
			{
				sys_err("cannot find item on pos (%d, %d) (name: %s)", pTable->pos.window_type, pTable->pos.cell, m_pkPC->GetName());
				continue;
			}

			item_table = pkItem->GetProto();
		}
		else
		{
			if (!pTable->vnum)
				continue;

			item_table = ITEM_MANAGER::instance().GetTable(pTable->vnum);
		}

		if (!item_table)
		{
			sys_err("Shop: no item table by item vnum #%d", pTable->vnum);
			continue;
		}

		int iPos;

		if (IsPCShop())
		{
			sys_log(0, "MyShop: use position %d", pTable->display_pos);
			iPos = pTable->display_pos;
		}
		else
			iPos = m_pGrid->FindBlank(1, item_table->bSize);

		if (iPos < 0)
		{
			sys_err("not enough shop window");
			continue;
		}

		if (!m_pGrid->IsEmpty(iPos, 1, item_table->bSize))
		{
			if (IsPCShop())
			{
				sys_err("not empty position for pc shop %s[%d]", m_pkPC->GetName(), m_pkPC->GetPlayerID());
			}
			else
			{
				sys_err("not empty position for npc shop");
			}
			continue;
		}

		m_pGrid->Put(iPos, 1, item_table->bSize);

		SHOP_ITEM & item = m_itemVector[iPos];

		item.pkItem = pkItem;
		item.itemid = 0;

		if (item.pkItem)
		{
			item.vnum = pkItem->GetVnum();
			item.count = pkItem->GetCount();
			item.price = pTable->price;
			item.price1 = pTable->price1;
			item.price2 = pTable->price2;
			item.price3 = pTable->price3;
			item.price4 = pTable->price4;
			item.itemid	= pkItem->GetID();
		}
		else
		{
			item.vnum = pTable->vnum;
			item.count = pTable->count;

			if (IS_SET(item_table->dwFlags, ITEM_FLAG_COUNT_PER_1GOLD))
			{
				if (item_table->dwGold == 0)
					item.price = item.count;
				else
					item.price = item.count / item_table->dwGold;
			}
			else
				item.price = item_table->dwGold * item.count;
		}

		char name[36];
		snprintf(name, sizeof(name), "%-20s(#%-5d) (x %d)", item_table->szName, (int) item.vnum, item.count);

		sys_log(0, "SHOP_ITEM: %-36s PRICE %-5d", name, item.price);
		++pTable;
	}
}

int CShop::Buy(LPCHARACTER ch, BYTE pos, int dwCount)
{
	int iCount = dwCount;

	if (pos >= m_itemVector.size())
	{
		sys_log(0, "Shop::Buy : invalid position %d : %s", pos, ch->GetName());
		return SHOP_SUBHEADER_GC_INVALID_POS;
	}

	sys_log(0, "Shop::Buy : name %s pos %d", ch->GetName(), pos);

	GuestMapType::iterator it = m_map_guest.find(ch);

	if (it == m_map_guest.end())
		return SHOP_SUBHEADER_GC_END;

	SHOP_ITEM& r_item = m_itemVector[pos];

	if (r_item.price <= 0)
	{
		LogManager::instance().HackLog("SHOP_BUY_GOLD_OVERFLOW", ch);
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
	}

	LPITEM pkSelectedItem = ITEM_MANAGER::instance().Find(r_item.itemid);

	if (IsPCShop())
	{
		if (!pkSelectedItem)
		{
			sys_log(0, "Shop::Buy : Critical: This user seems to be a hacker : invalid pcshop item : BuyerPID:%d SellerPID:%d",
					ch->GetPlayerID(),
					m_pkPC->GetPlayerID());

			return SHOP_SUBHEADER_GC_SOLD_OUT; // @fixme132 false to SHOP_SUBHEADER_GC_SOLD_OUT
		}

		if ((pkSelectedItem->GetOwner() != m_pkPC))
		{
			sys_log(0, "Shop::Buy : Critical: This user seems to be a hacker : invalid pcshop item : BuyerPID:%d SellerPID:%d",
					ch->GetPlayerID(),
					m_pkPC->GetPlayerID());

			return SHOP_SUBHEADER_GC_SOLD_OUT; // @fixme132 false to SHOP_SUBHEADER_GC_SOLD_OUT
		}
	}

	for (int i = 0; i < iCount; i++)
	{
		long long dwPrice = r_item.price;

		if (ch->GetGold() < dwPrice)
		{
			sys_log(1, "Shop::Buy : Not enough money : %s has %d, price %d", ch->GetName(), ch->GetGold(), dwPrice);
			return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
		}

		LPITEM item;

		if (m_pkPC)
			item = r_item.pkItem;
		else
		{
			item = ITEM_MANAGER::instance().CreateItem(r_item.vnum, r_item.count);
		}

		if (!item)
			return SHOP_SUBHEADER_GC_SOLD_OUT;

	#ifdef ENABLE_SHOP_BLACKLIST
		if (!m_pkPC)
		{
			if (quest::CQuestManager::instance().GetEventFlag("hivalue_item_sell") == 0)
			{
				if (item->GetVnum() == 70024 || item->GetVnum() == 70035)
				{
					return SHOP_SUBHEADER_GC_END;
				}
			}
		}
	#endif

		int iEmptyPos;
		int iInvType = 0;
		if (item->IsDragonSoul())
		{
			iEmptyPos = ch->GetEmptyDragonSoulInventory(item);
			iInvType = DRAGON_SOUL_INVENTORY;
		}
	#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		else if (item->IsSkillBook())
		{
			iEmptyPos = ch->GetEmptySkillBookInventory(item->GetSize());
			iInvType = SKILL_BOOK_INVENTORY;
		}
		else if (item->IsUpgradeItem())
		{
			iEmptyPos = ch->GetEmptyUpgradeItemsInventory(item->GetSize());
			iInvType = UPGRADE_ITEMS_INVENTORY;
		}
		else if (item->IsStone())
		{
			iEmptyPos = ch->GetEmptyStoneInventory(item->GetSize());
			iInvType = STONE_INVENTORY;
		}
		else if (item->IsGiftBox())
		{
			iEmptyPos = ch->GetEmptyGiftBoxInventory(item->GetSize());
			iInvType = GIFT_BOX_INVENTORY;
		}
		else if (item->IsSwitch())
		{
			iEmptyPos = ch->GetEmptySwitchInventory(item->GetSize());
			iInvType = SWITCH_INVENTORY;
		}
	#endif
		else
		{
			iEmptyPos = ch->GetEmptyInventory(item->GetSize());
			iInvType = INVENTORY;
		}

		if (iEmptyPos < 0)
		{
			if (m_pkPC)
			{
				sys_log(1, "Shop::Buy at PC Shop : Inventory full : %s size %d", ch->GetName(), item->GetSize());
				return SHOP_SUBHEADER_GC_INVENTORY_FULL;
			}
			else
			{
				sys_log(1, "Shop::Buy : Inventory full : %s size %d", ch->GetName(), item->GetSize());
				M2_DESTROY_ITEM(item);
				return SHOP_SUBHEADER_GC_INVENTORY_FULL;
			}
		}

		ch->PointChange(POINT_GOLD, -dwPrice, false);
		CHARACTER_MANAGER::Instance().DoMission(ch, MISSION_SPEND_MONEY, dwPrice, 0);

		DWORD dwTax = 0;
		int iVal = 0;

		{
			iVal = quest::CQuestManager::instance().GetEventFlag("personal_shop");

			if (0 < iVal)
			{
				if (iVal > 100)
					iVal = 100;

				dwTax = dwPrice * iVal / 100;
				dwPrice = dwPrice - dwTax;
			}
			else
			{
				iVal = 0;
				dwTax = 0;
			}
		}

		if (!m_pkPC)
		{
			CMonarch::instance().SendtoDBAddMoney(dwTax, ch->GetEmpire(), ch);
		}

		if (m_pkPC)
		{
			m_pkPC->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), 255);

			{
				char buf[512];

				item->RemoveFromCharacter();
				if (item->IsDragonSoul())
				{
					item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
				}
	#if defined(__SPECIAL_INVENTORY_SYSTEM__)
				else if (item->IsSkillBook())
				{
					item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
				}
				else if (item->IsUpgradeItem())
				{
					item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
				}
				else if (item->IsStone())
				{
					item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
				}
				else if (item->IsGiftBox())
				{
					item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
				}

				else if (item->IsSwitch())
				{
					item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
				}

	#endif
				else
				{
					item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
				}

				if (item->GetVnum() == 51010)
					CHARACTER_MANAGER::Instance().SetRewardData(REWARD_CRYSTAL,ch->GetName(), true);
				
				ITEM_MANAGER::instance().FlushDelayedSave(item);

				snprintf(buf, sizeof(buf), "%s %u(%s) %u %u", item->GetName(), m_pkPC->GetPlayerID(), m_pkPC->GetName(), dwPrice, item->GetCount());
				LogManager::instance().ItemLog(ch, item, "SHOP_BUY", buf);

				snprintf(buf, sizeof(buf), "%s %u(%s) %u %u", item->GetName(), ch->GetPlayerID(), ch->GetName(), dwPrice, item->GetCount());
				LogManager::instance().ItemLog(m_pkPC, item, "SHOP_SELL", buf);
			}

			r_item.pkItem = NULL;
			BroadcastUpdateItem(pos);

			m_pkPC->PointChange(POINT_GOLD, (int64_t)dwPrice, false);

			if (iVal > 0)
				m_pkPC->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("판매금액의 %d %% 가 세금으로 나가게됩니다"), iVal);

			CMonarch::instance().SendtoDBAddMoney(dwTax, m_pkPC->GetEmpire(), m_pkPC);
		}
		else
		{
			if (item->IsDragonSoul())
				item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
			else
			{
				WORD bCount = item->GetCount();

				if (IS_SET(item->GetFlag(), ITEM_FLAG_STACKABLE) && !item->IsEquipped())
				{
					switch (iInvType)
					{
						case SKILL_BOOK_INVENTORY:
							{
								for (int i = SKILL_BOOK_INVENTORY_SLOT_START; i < SKILL_BOOK_INVENTORY_SLOT_END; ++i)
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
							}
							break;
						case UPGRADE_ITEMS_INVENTORY:
							{
								for (int i = UPGRADE_ITEMS_INVENTORY_SLOT_START; i < UPGRADE_ITEMS_INVENTORY_SLOT_END; ++i)
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
							}
							break;
						case STONE_INVENTORY:
							{
								for (int i = STONE_INVENTORY_SLOT_START; i < STONE_INVENTORY_SLOT_END; ++i)
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
							}
							break;
						case GIFT_BOX_INVENTORY:
							{
								for (int i = GIFT_BOX_INVENTORY_SLOT_START; i < GIFT_BOX_INVENTORY_SLOT_END; ++i)
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
							}
							break;
						case SWITCH_INVENTORY:
							{
								for (int i = SWITCH_INVENTORY_SLOT_START; i < SWITCH_INVENTORY_SLOT_END; ++i)
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
							}
							break;
						case INVENTORY:
							{
								for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
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
							}
							break;						
					}

					item->SetCount(bCount);
				}

				if (bCount > 0)
					item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
				else
					M2_DESTROY_ITEM(item);
			}

			ITEM_MANAGER::instance().FlushDelayedSave(item);

			DBManager::instance().SendMoneyLog(MONEY_LOG_SHOP, item->GetVnum(), -dwPrice);
		}
		if (item)
			sys_log(0, "SHOP: BUY: name %s %s(x %d):%u price %u count %s", ch->GetName(), item->GetName(), item->GetCount(), item->GetID(), dwPrice, iCount);
	}

    ch->Save();

    return (SHOP_SUBHEADER_GC_OK);
}

bool CShop::AddGuest(LPCHARACTER ch, DWORD owner_vid, bool bOtherEmpire)
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
	pack.subheader	= SHOP_SUBHEADER_GC_START;

	TPacketGCShopStart pack2;

	memset(&pack2, 0, sizeof(pack2));
	pack2.owner_vid = owner_vid;

	for (DWORD i = 0; i < m_itemVector.size() && i < SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		const SHOP_ITEM & item = m_itemVector[i];
		//END_HIVALUE_ITEM_EVENT
		if (m_pkPC && !item.pkItem)
			continue;

		pack2.items[i].vnum = item.vnum;

		pack2.items[i].price = item.price;
#if defined(__SHOPEX_RENEWAL__)
		pack2.items[i].price1 = item.price1 > 0 ? item.price1 : 0;
		pack2.items[i].price2 = item.price2 > 0 ? item.price2 : 0;
		pack2.items[i].price3 = item.price3 > 0 ? item.price3 : 0;
		pack2.items[i].price4 = item.price4 > 0 ? item.price4 : 0;
#endif

		pack2.items[i].count = item.count;
#if defined(__SHOPEX_RENEWAL__)
		pack2.items[i].price_type = item.price_type;
		pack2.items[i].price_vnum = item.price_vnum;
		pack2.items[i].price_vnum1 = item.price_vnum1 > 0 ? item.price_vnum1 : 0;
		pack2.items[i].price_vnum2 = item.price_vnum2 > 0 ? item.price_vnum2 : 0;
		pack2.items[i].price_vnum3 = item.price_vnum3 > 0 ? item.price_vnum3 : 0;
		pack2.items[i].price_vnum4 = item.price_vnum4 > 0 ? item.price_vnum4 : 0;
#endif

		if (item.pkItem)
		{
			thecore_memcpy(pack2.items[i].alSockets, item.pkItem->GetSockets(), sizeof(pack2.items[i].alSockets));
			thecore_memcpy(pack2.items[i].aAttr, item.pkItem->GetAttributes(), sizeof(pack2.items[i].aAttr));
		}
	}

	pack.size = sizeof(pack) + sizeof(pack2);

	ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCShop));
	ch->GetDesc()->Packet(&pack2, sizeof(TPacketGCShopStart));
	return true;
}

void CShop::RemoveGuest(LPCHARACTER ch)
{
	if (ch->GetShop() != this)
		return;

	m_map_guest.erase(ch);
	ch->SetShop(NULL);

	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_END;
	pack.size		= sizeof(TPacketGCShop);

	ch->GetDesc()->Packet(&pack, sizeof(pack));
}

void CShop::Broadcast(const void * data, int bytes)
{
	sys_log(1, "Shop::Broadcast %p %d", data, bytes);

	GuestMapType::iterator it;

	it = m_map_guest.begin();

	while (it != m_map_guest.end())
	{
		LPCHARACTER ch = it->first;

		if (ch->GetDesc())
			ch->GetDesc()->Packet(data, bytes);

		++it;
	}
}

void CShop::BroadcastUpdateItem(BYTE pos)
{
	TPacketGCShop pack;
	TPacketGCShopUpdateItem pack2;

	TEMP_BUFFER	buf;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_UPDATE_ITEM;
	pack.size		= sizeof(pack) + sizeof(pack2);

	pack2.pos		= pos;

	if (m_pkPC && !m_itemVector[pos].pkItem)
		pack2.item.vnum = 0;
	else
	{
		pack2.item.vnum	= m_itemVector[pos].vnum;
		if (m_itemVector[pos].pkItem)
		{
			thecore_memcpy(pack2.item.alSockets, m_itemVector[pos].pkItem->GetSockets(), sizeof(pack2.item.alSockets));
			thecore_memcpy(pack2.item.aAttr, m_itemVector[pos].pkItem->GetAttributes(), sizeof(pack2.item.aAttr));
		}
		else
		{
			memset(pack2.item.alSockets, 0, sizeof(pack2.item.alSockets));
			memset(pack2.item.aAttr, 0, sizeof(pack2.item.aAttr));
		}
	}

	pack2.item.price	= m_itemVector[pos].price;
	pack2.item.count	= m_itemVector[pos].count;

	buf.write(&pack, sizeof(pack));
	buf.write(&pack2, sizeof(pack2));

	Broadcast(buf.read_peek(), buf.size());
}

int CShop::GetNumberByVnum(DWORD dwVnum)
{
	int itemNumber = 0;

	for (DWORD i = 0; i < m_itemVector.size() && i < SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		const SHOP_ITEM & item = m_itemVector[i];

		if (item.vnum == dwVnum)
		{
			itemNumber += item.count;
		}
	}

	return itemNumber;
}

bool CShop::IsSellingItem(DWORD itemID)
{
	bool isSelling = false;

	for (DWORD i = 0; i < m_itemVector.size() && i < SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		if ((unsigned int)(m_itemVector[i].itemid) == itemID)
		{
			isSelling = true;
			break;
		}
	}

	return isSelling;
}
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
