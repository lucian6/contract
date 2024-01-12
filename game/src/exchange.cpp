#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "char.h"
#include "item.h"
#include "item_manager.h"
#include "packet.h"
#include "log.h"
#include "db.h"
#include "locale_service.h"
#include "../../common/length.h"
#include "exchange.h"
#include "DragonSoul.h"
#include "questmanager.h" // @fixme150
#if defined(__MESSENGER_BLOCK_SYSTEM__)
#include "messenger_manager.h"
#endif

void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, long long arg1, TItemPos arg2, DWORD arg3, void * pvData = NULL);

void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, long long arg1, TItemPos arg2, DWORD arg3, void * pvData)
{
	if (!ch->GetDesc())
		return;

	struct packet_exchange pack_exchg;

	pack_exchg.header 		= HEADER_GC_EXCHANGE;
	pack_exchg.sub_header 	= sub_header;
	pack_exchg.is_me		= is_me;
	pack_exchg.arg1		= arg1;
	pack_exchg.arg2		= arg2;
	pack_exchg.arg3		= arg3;

	if (sub_header == EXCHANGE_SUBHEADER_GC_ITEM_ADD && pvData)
	{
#ifdef WJ_ENABLE_TRADABLE_ICON
		pack_exchg.arg4 = TItemPos(((LPITEM) pvData)->GetWindow(), ((LPITEM) pvData)->GetCell());
#endif
		thecore_memcpy(&pack_exchg.alSockets, ((LPITEM) pvData)->GetSockets(), sizeof(pack_exchg.alSockets));
		thecore_memcpy(&pack_exchg.aAttr, ((LPITEM) pvData)->GetAttributes(), sizeof(pack_exchg.aAttr));
	}
	else
	{
#ifdef WJ_ENABLE_TRADABLE_ICON
		pack_exchg.arg4 = TItemPos(RESERVED_WINDOW, 0);
#endif
		memset(&pack_exchg.alSockets, 0, sizeof(pack_exchg.alSockets));
		memset(&pack_exchg.aAttr, 0, sizeof(pack_exchg.aAttr));
	}

	ch->GetDesc()->Packet(&pack_exchg, sizeof(pack_exchg));
	
	if (sub_header == EXCHANGE_SUBHEADER_GC_ITEM_ADD)
		ch->ChatPacket(CHAT_TYPE_COMMAND, "AppendExchangeAddItemChat %lld %d %d", arg1, arg3, is_me);

	if (sub_header == EXCHANGE_SUBHEADER_GC_GOLD_ADD)
		ch->ChatPacket(CHAT_TYPE_COMMAND, "AppendExchangeAddGoldChat %lld %d", arg1, is_me);
}

bool CHARACTER::ExchangeStart(LPCHARACTER victim)
{
	if (this == victim)
		return false;

	if (IsObserverMode())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("관전 상태에서는 교환을 할 수 없습니다."));
		return false;
	}

	if (victim->IsNPC())
		return false;

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	if (MessengerManager::instance().IsBlocked(GetName(), victim->GetName()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), victim->GetName());
		return false;
	}
	else if (MessengerManager::instance().IsBlocked(victim->GetName(), GetName()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s has blocked you."), victim->GetName());
		return false;
	}
#endif

	// PREVENT_TRADE_WINDOW
	if (PreventTradeWindow(WND_EXCHANGE, true/*except*/))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래창이 열려있을경우 거래를 할수 없습니다." ));
		return false;
	}

	if (victim->PreventTradeWindow(WND_EXCHANGE, true/*except*/))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래창이 열려있을경우 거래를 할수 없습니다." ));
		return false;
	}
	// END_PREVENT_TRADE_WINDOW
	
	int iDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	if (iDist >= EXCHANGE_MAX_DISTANCE)
		return false;

	if (GetExchange())
		return false;

	if (victim->GetExchange())
	{
		exchange_packet(this, EXCHANGE_SUBHEADER_GC_ALREADY, 0, 0, NPOS, 0);
		return false;
	}

	if (victim->IsBlockMode(BLOCK_EXCHANGE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 교환 거부 상태입니다."));
		return false;
	}

	SetExchange(M2_NEW CExchange(this));
	victim->SetExchange(M2_NEW CExchange(victim));

	victim->GetExchange()->SetCompany(GetExchange());
	GetExchange()->SetCompany(victim->GetExchange());

	SetExchangeTime();
	victim->SetExchangeTime();

	exchange_packet(victim, EXCHANGE_SUBHEADER_GC_START, 0, GetVID(), NPOS, 0);
	exchange_packet(this, EXCHANGE_SUBHEADER_GC_START, 0, victim->GetVID(), NPOS, 0);

	return true;
}

CExchange::CExchange(LPCHARACTER pOwner)
{
	m_pCompany = NULL;

	m_bAccept = false;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		m_apItems[i] = NULL;
		m_aItemPos[i] = NPOS;
		m_abItemDisplayPos[i] = 0;
	}

	m_lGold = 0;

	m_pOwner = pOwner;
	pOwner->SetExchange(this);

	m_pGrid = M2_NEW CGrid(6,4);
}

CExchange::~CExchange()
{
	M2_DELETE(m_pGrid);
}

#if defined(ITEM_CHECKINOUT_UPDATE)
int CExchange::GetEmptyExchange(BYTE size)
{
	for (unsigned int i = 0; m_pGrid && i < m_pGrid->GetSize(); i++)
		if (m_pGrid->IsEmpty(i, 1, size))
			return i;

	return -1;
}
#endif

#if defined(ITEM_CHECKINOUT_UPDATE)
bool CExchange::AddItem(TItemPos item_pos, BYTE display_pos, bool SelectPosAuto)
#else
bool CExchange::AddItem(TItemPos item_pos, BYTE display_pos)
#endif
{
	assert(m_pOwner != NULL && GetCompany());

	if (!item_pos.IsValidItemPosition())
		return false;

	if (item_pos.IsEquipPosition())
		return false;

	LPITEM item;

	if (!(item = m_pOwner->GetItem(item_pos)))
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE) || item->GetSocket(4) == 99)
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템을 건네줄 수 없습니다."));
		return false;
	}

	if ((item->GetType() == ITEM_BUFFI && item->GetSubType() == BUFFI_SCROLL) && item->GetSocket(1) == 1)
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Deactivate the Buffi Seal first."));
		return false;
	}

	if ((item->GetVnum() >= 51010 && item->GetVnum() <= 51030) && item->GetSocket(0) == 1)
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Deactivate the Energy Crystal first."));
		return false;
	}


	if (true == item->isLocked())
	{
		return false;
	}

	if (item->IsExchanging())
	{
		sys_log(0, "EXCHANGE under exchanging");
		return false;
	}

#if defined(ITEM_CHECKINOUT_UPDATE)
	if (SelectPosAuto)
	{
		int AutoPos = GetEmptyExchange(item->GetSize());
		if (AutoPos == -1)
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "<EXCHANGE> You don't have enough space.");
			return false;
		}
		display_pos = AutoPos;
	}
#endif

	if (!m_pGrid->IsEmpty(display_pos, 1, item->GetSize()))
	{
		sys_log(0, "EXCHANGE not empty item_pos %d %d %d", display_pos, 1, item->GetSize());
		return false;
	}

	Accept(false);
	GetCompany()->Accept(false);

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			continue;

		m_apItems[i]		= item;
		m_aItemPos[i]		= item_pos;
		m_abItemDisplayPos[i]	= display_pos;
		m_pGrid->Put(display_pos, 1, item->GetSize());

		item->SetExchanging(true);

		exchange_packet(m_pOwner,
				EXCHANGE_SUBHEADER_GC_ITEM_ADD,
				true,
				item->GetVnum(),
				TItemPos(RESERVED_WINDOW, display_pos),
				item->GetCount(),
				item);

		exchange_packet(GetCompany()->GetOwner(),
				EXCHANGE_SUBHEADER_GC_ITEM_ADD,
				false,
				item->GetVnum(),
				TItemPos(RESERVED_WINDOW, display_pos),
				item->GetCount(),
				item);

		sys_log(0, "EXCHANGE AddItem success %s pos(%d, %d) %d", item->GetName(), item_pos.window_type, item_pos.cell, display_pos);

		return true;
	}

	return false;
}

bool CExchange::RemoveItem(BYTE pos)
{
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return false;

	if (!m_apItems[pos])
		return false;

	TItemPos PosOfInventory = m_aItemPos[pos];
	m_apItems[pos]->SetExchanging(false);

	m_pGrid->Get(m_abItemDisplayPos[pos], 1, m_apItems[pos]->GetSize());

	exchange_packet(GetOwner(),	EXCHANGE_SUBHEADER_GC_ITEM_DEL, true, pos, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_ITEM_DEL, false, pos, PosOfInventory, 0);

	Accept(false);
	GetCompany()->Accept(false);

	m_apItems[pos]	    = NULL;
	m_aItemPos[pos]	    = NPOS;
	m_abItemDisplayPos[pos] = 0;
	return true;
}

bool CExchange::AddGold(long long gold)
{
	if (gold <= 0)
		return false;

	if (GetOwner()->GetGold() < gold)
	{
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_LESS_GOLD, 0, 0, NPOS, 0);
		return false;
	}

	if (m_lGold > 0)
		return false;

	Accept(false);
	GetCompany()->Accept(false);

	m_lGold = gold;

	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_GOLD_ADD, true, m_lGold, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_GOLD_ADD, false, m_lGold, NPOS, 0);
	return true;
}

bool CExchange::Check(int * piItemCount)
{
	if (GetOwner()->GetGold() < m_lGold)
		return false;

	int item_count = 0;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!m_apItems[i])
			continue;

		if (!m_aItemPos[i].IsValidItemPosition())
			return false;

		if (m_apItems[i] != GetOwner()->GetItem(m_aItemPos[i]))
			return false;

		++item_count;
	}

	*piItemCount = item_count;
	return true;
}

bool CExchange::CheckSpace()
{
	static CGrid s_grid_inventory1(5, 9);
	s_grid_inventory1.Clear();
	static CGrid s_grid_inventory2(5, 9);
	s_grid_inventory2.Clear();
	static CGrid s_grid_inventory3(5, 9);
	s_grid_inventory3.Clear();
	static CGrid s_grid_inventory4(5, 9);
	s_grid_inventory4.Clear();

#ifdef __SPECIAL_INVENTORY_SYSTEM__
	static CGrid s_grid_book1(5, 9);
	s_grid_book1.Clear();
	static CGrid s_grid_book2(5, 9);
	s_grid_book2.Clear();
	static CGrid s_grid_book3(5, 9);
	s_grid_book3.Clear();
	static CGrid s_grid_book4(5, 9);
	s_grid_book4.Clear();
	static CGrid s_grid_book5(5, 9);
	s_grid_book5.Clear();

	static CGrid s_grid_upgrade1(5, 9);
	s_grid_upgrade1.Clear();
	static CGrid s_grid_upgrade2(5, 9);
	s_grid_upgrade2.Clear();
	static CGrid s_grid_upgrade3(5, 9);
	s_grid_upgrade3.Clear();
	static CGrid s_grid_upgrade4(5, 9);
	s_grid_upgrade4.Clear();
	static CGrid s_grid_upgrade5(5, 9);
	s_grid_upgrade5.Clear();

	static CGrid s_grid_stone1(5, 9);
	s_grid_stone1.Clear();
	static CGrid s_grid_stone2(5, 9);
	s_grid_stone2.Clear();
	static CGrid s_grid_stone3(5, 9);
	s_grid_stone3.Clear();
	static CGrid s_grid_stone4(5, 9);
	s_grid_stone4.Clear();
	static CGrid s_grid_stone5(5, 9);
	s_grid_stone5.Clear();

	static CGrid s_grid_gift1(5, 9);
	s_grid_gift1.Clear();
	static CGrid s_grid_gift2(5, 9);
	s_grid_gift2.Clear();
	static CGrid s_grid_gift3(5, 9);
	s_grid_gift3.Clear();
	static CGrid s_grid_gift4(5, 9);
	s_grid_gift4.Clear();
	static CGrid s_grid_gift5(5, 9);
	s_grid_gift5.Clear();

	static CGrid s_grid_switch1(5, 9);
	s_grid_switch1.Clear();
	static CGrid s_grid_switch2(5, 9);
	s_grid_switch2.Clear();
	static CGrid s_grid_switch3(5, 9);
	s_grid_switch3.Clear();
	static CGrid s_grid_switch4(5, 9);
	s_grid_switch4.Clear();
	static CGrid s_grid_switch5(5, 9);
	s_grid_switch5.Clear();

	// static CGrid s_grid_costume1(5, 9);
	// s_grid_costume1.Clear();
	// static CGrid s_grid_costume2(5, 9);
	// s_grid_costume2.Clear();
	// static CGrid s_grid_costume3(5, 9);
	// s_grid_costume3.Clear();
	// static CGrid s_grid_costume4(5, 9);
	// s_grid_costume4.Clear();
	// static CGrid s_grid_costume5(5, 9);
	// s_grid_costume5.Clear();
#endif

	static CGrid s_grid_inventory[4] = { s_grid_inventory1,s_grid_inventory2,s_grid_inventory3,s_grid_inventory4 };
	static CGrid s_grid_book[5] = { s_grid_book1,s_grid_book2,s_grid_book3,s_grid_book4 ,s_grid_book5 };
	static CGrid s_grid_upgrade[5] = { s_grid_upgrade1,s_grid_upgrade2,s_grid_upgrade3,s_grid_upgrade4 ,s_grid_upgrade5 };
	static CGrid s_grid_stone[5] = { s_grid_stone1,s_grid_stone2,s_grid_stone3,s_grid_stone4 ,s_grid_stone5 };
	static CGrid s_grid_gift[5] = { s_grid_gift1,s_grid_gift2,s_grid_gift3,s_grid_gift4 ,s_grid_gift5 };
	static CGrid s_grid_switch[5] = { s_grid_switch1,s_grid_switch2,s_grid_switch3,s_grid_switch4 ,s_grid_switch5 };
	// static CGrid s_grid_costume[5] = { s_grid_costume1,s_grid_costume2,s_grid_costume3,s_grid_costume4 ,s_grid_costume5 };

	LPCHARACTER	victim = GetCompany()->GetOwner();
	LPITEM item;

	for (DWORD j = 0; j < 5; j++)
	{
		for (DWORD i = 45*j; i < 45+(45*j); ++i)
		{
			if (j != 4)
			{
				if(i < victim->Inventory_Size())
				{
					if ((item = victim->GetInventoryItem(i)) != NULL)
						s_grid_inventory[j].Put(i - ((180 / 4) * j), 1, item->GetSize());
				}
				else
					s_grid_inventory[j].Put(i - ((180 / 4) * j), 1, 1);
			}

#ifdef __SPECIAL_INVENTORY_SYSTEM__
			if(i + SKILL_BOOK_INVENTORY_SLOT_START < SKILL_BOOK_INVENTORY_SLOT_START + victim->Skill_Inventory_Size())
			{
				if ((item = victim->GetSkillBookInventoryItem(SKILL_BOOK_INVENTORY_SLOT_START + i)) != NULL)
					s_grid_book[j].Put(i - ((225 / 5) * j), 1, item->GetSize());
			}
			else
				s_grid_book[j].Put(i - ((225 / 5) * j), 1, 1);

			if(i + UPGRADE_ITEMS_INVENTORY_SLOT_START < UPGRADE_ITEMS_INVENTORY_SLOT_START + victim->Material_Inventory_Size())
			{
				if ((item = victim->GetUpgradeItemsInventoryItem(UPGRADE_ITEMS_INVENTORY_SLOT_START + i)) != NULL)
					s_grid_upgrade[j].Put(i - ((225 / 5) * j), 1, item->GetSize());
			}
			else
				s_grid_upgrade[j].Put(i - ((225 / 5) * j), 1, 1);

			if(i + STONE_INVENTORY_SLOT_START < STONE_INVENTORY_SLOT_START + victim->Stone_Inventory_Size())
			{
				if ((item = victim->GetStoneInventoryItem(STONE_INVENTORY_SLOT_START + i)) != NULL)
					s_grid_stone[j].Put(i - ((225 / 5) * j), 1, item->GetSize());
			}
			else
				s_grid_stone[j].Put(i - ((225 / 5) * j), 1, 1);

			if(i + GIFT_BOX_INVENTORY_SLOT_START < GIFT_BOX_INVENTORY_SLOT_START + victim->Chest_Inventory_Size())
			{
				if ((item = victim->GetGiftBoxInventoryItem(GIFT_BOX_INVENTORY_SLOT_START + i)) != NULL)
					s_grid_gift[j].Put(i - ((225 / 5) * j), 1, item->GetSize());
			}
			else
				s_grid_gift[j].Put(i - ((225 / 5) * j), 1, 1);

			if(i + SWITCH_INVENTORY_SLOT_START < SWITCH_INVENTORY_SLOT_START + victim->Enchant_Inventory_Size())
			{
				if ((item = victim->GetSwitchInventoryItem(SWITCH_INVENTORY_SLOT_START + i)) != NULL)
					s_grid_switch[j].Put(i - ((225 / 5) * j), 1, item->GetSize());
			}
			else
				s_grid_switch[j].Put(i - ((225 / 5) * j), 1, 1);

			// if(i + COSTUME_INVENTORY_SLOT_START < COSTUME_INVENTORY_SLOT_START + victim->Costume_Inventory_Size())
			// {
			// 	if ((item = victim->GetCostumeInventoryItem(COSTUME_INVENTORY_SLOT_START + i)) != NULL)
			// 		s_grid_costume[j].Put(i - ((225 / 5) * j), 1, item->GetSize());
			// }
			// else
			// 	s_grid_costume[j].Put(i - ((225 / 5) * j), 1, 1);
#endif
		}
	}
	

	static std::vector <WORD> s_vDSGrid(DRAGON_SOUL_INVENTORY_MAX_NUM);

	bool bDSInitialized = false;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (item->IsDragonSoul())
		{
			if (!victim->DragonSoul_IsQualified())
			{
				return false;
			}

			if (!bDSInitialized)
			{
				bDSInitialized = true;
				victim->CopyDragonSoulItemGrid(s_vDSGrid);
			}

			bool bExistEmptySpace = false;
			WORD wBasePos = DSManager::instance().GetBasePosition(item);
			if (wBasePos >= DRAGON_SOUL_INVENTORY_MAX_NUM)
			{
				return false;
			}

			for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; i++)
			{
				WORD wPos = wBasePos + i;
				if (0 == s_vDSGrid[wPos])
				{
					bool bEmpty = true;
					for (int j = 1; j < item->GetSize(); j++)
					{
						if (s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM])
						{
							bEmpty = false;
							break;
						}
					}
					if (bEmpty)
					{
						for (int j = 0; j < item->GetSize(); j++)
						{
							s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM] = wPos + 1;
						}
						bExistEmptySpace = true;
						break;
					}
				}
				if (bExistEmptySpace)
					break;
			}
			if (!bExistEmptySpace)
			{
				return false;
			}
		}
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		else if (item->IsSkillBook())
		{
			bool isStatus = true;
			for (DWORD j = 0; j < 5; j++)
			{
				const int iPos = s_grid_book[j].FindBlank(1, item->GetSize());
				if (iPos >= 0)
				{
					s_grid_book[j].Put(iPos, 1, item->GetSize());
					isStatus = false;
					break;
				}
			}
			if(isStatus)
				return false;
		}
		else if (item->IsUpgradeItem())
		{
			bool isStatus = true;
			for (DWORD j = 0; j < 5; j++)
			{
				const int iPos = s_grid_upgrade[j].FindBlank(1, item->GetSize());
				if (iPos >= 0)
				{
					s_grid_upgrade[j].Put(iPos, 1, item->GetSize());
					isStatus = false;
					break;
				}
			}
			if(isStatus)
				return false;
		}
		else if (item->IsStone())
		{
			bool isStatus = true;
			for (DWORD j = 0; j < 5; j++)
			{
				const int iPos = s_grid_stone[j].FindBlank(1, item->GetSize());
				if (iPos >= 0)
				{
					s_grid_stone[j].Put(iPos, 1, item->GetSize());
					isStatus = false;
					break;
				}
			}
			if(isStatus)
				return false;
		}
		else if (item->IsGiftBox())
		{
			bool isStatus = true;
			for (DWORD j = 0; j < 5; j++)
			{
				const int iPos = s_grid_gift[j].FindBlank(1, item->GetSize());
				if (iPos >= 0)
				{
					s_grid_gift[j].Put(iPos, 1, item->GetSize());
					isStatus = false;
					break;
				}
			}
			if(isStatus)
				return false;
		}
		else if (item->IsSwitch())
		{
			bool isStatus = true;
			for (DWORD j = 0; j < 5; j++)
			{
				const int iPos = s_grid_switch[j].FindBlank(1, item->GetSize());
				if (iPos >= 0)
				{
					s_grid_switch[j].Put(iPos, 1, item->GetSize());
					isStatus = false;
					break;
				}
			}
			if(isStatus)
				return false;
		}
		// else if (item->IsCostume())
		// {
		// 	bool isStatus = true;
		// 	for (DWORD j = 0; j < 5; j++)
		// 	{
		// 		const int iPos = s_grid_costume[j].FindBlank(1, item->GetSize());
		// 		if (iPos >= 0)
		// 		{
		// 			s_grid_costume[j].Put(iPos, 1, item->GetSize());
		// 			isStatus = false;
		// 			break;
		// 		}
		// 	}
		// 	if(isStatus)
		// 		return false;
		// }
#endif
		else
		{
			bool isStatus = true;
			for (DWORD j = 0; j < 4; j++)
			{
				const int iPos = s_grid_inventory[j].FindBlank(1, item->GetSize());
				if (iPos >= 0)
				{
					s_grid_inventory[j].Put(iPos, 1, item->GetSize());
					isStatus = false;
					break;
				}
			}
			if(isStatus)
				return false;
		}
	}

	return true;
}

bool CExchange::Done()
{
	int		empty_pos, i;
	LPITEM	item;

	LPCHARACTER	victim = GetCompany()->GetOwner();

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (item->IsDragonSoul())
			empty_pos = victim->GetEmptyDragonSoulInventory(item);
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		else if (item->IsSkillBook())
			empty_pos = victim->GetEmptySkillBookInventory(item->GetSize());
		else if (item->IsUpgradeItem())
			empty_pos = victim->GetEmptyUpgradeItemsInventory(item->GetSize());
		else if (item->IsStone())
			empty_pos = victim->GetEmptyStoneInventory(item->GetSize());
		else if (item->IsGiftBox())
			empty_pos = victim->GetEmptyGiftBoxInventory(item->GetSize());
		else if (item->IsSwitch())
			empty_pos = victim->GetEmptySwitchInventory(item->GetSize());
		else if (item->IsCostume())
			empty_pos = victim->GetEmptyCostumeInventory(item->GetSize());
#endif
		else
			empty_pos = victim->GetEmptyInventory(item->GetSize());

		if (empty_pos < 0)
		{
			sys_err("Exchange::Done : Cannot find blank position in inventory %s <-> %s item %s",
					m_pOwner->GetName(), victim->GetName(), item->GetName());
			continue;
		}

		assert(empty_pos >= 0);

		m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), 255);

		item->RemoveFromCharacter();
		if (item->IsDragonSoul())
			item->AddToCharacter(victim, TItemPos(DRAGON_SOUL_INVENTORY, empty_pos));
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		else if (item->IsSkillBook())
			item->AddToCharacter(victim, TItemPos(INVENTORY, empty_pos));
		else if (item->IsUpgradeItem())
			item->AddToCharacter(victim, TItemPos(INVENTORY, empty_pos));
		else if (item->IsStone())
			item->AddToCharacter(victim, TItemPos(INVENTORY, empty_pos));
		else if (item->IsGiftBox())
			item->AddToCharacter(victim, TItemPos(INVENTORY, empty_pos));
		else if (item->IsSwitch())
			item->AddToCharacter(victim, TItemPos(INVENTORY, empty_pos));
		else if (item->IsCostume())
			item->AddToCharacter(victim, TItemPos(INVENTORY, empty_pos));
#endif
		else
			item->AddToCharacter(victim, TItemPos(INVENTORY, empty_pos));
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		item->SetExchanging(false);
		{
			char exchange_buf[51];

			snprintf(exchange_buf, sizeof(exchange_buf), "%s %u %u", item->GetName(), GetOwner()->GetPlayerID(), item->GetCount());
			LogManager::instance().ItemLog(victim, item, "EXCHANGE_TAKE", exchange_buf);

			snprintf(exchange_buf, sizeof(exchange_buf), "%s %u %u", item->GetName(), victim->GetPlayerID(), item->GetCount());
			LogManager::instance().ItemLog(GetOwner(), item, "EXCHANGE_GIVE", exchange_buf);
		}

		m_apItems[i] = NULL;
	}

	if (m_lGold)
	{
		GetOwner()->PointChange(POINT_GOLD, -m_lGold, true);
		victim->PointChange(POINT_GOLD, m_lGold, true);

		if (m_lGold > 1000)
		{
			// char exchange_buf[51];
			// snprintf(exchange_buf, sizeof(exchange_buf), "%u %s", GetOwner()->GetPlayerID(), GetOwner()->GetName());
			// LogManager::instance().CharLog(victim, m_lGold, "EXCHANGE_GOLD_TAKE", exchange_buf);

			// snprintf(exchange_buf, sizeof(exchange_buf), "%u %s", victim->GetPlayerID(), victim->GetName());
			// LogManager::instance().CharLog(GetOwner(), m_lGold, "EXCHANGE_GOLD_GIVE", exchange_buf);
		}
	}

	m_pGrid->Clear();
	return true;
}

bool CExchange::Accept(bool bAccept)
{
	if (m_bAccept == bAccept)
		return true;

	m_bAccept = bAccept;

	if (m_bAccept && GetCompany()->m_bAccept)
	{
		int	iItemCount;

		LPCHARACTER victim = GetCompany()->GetOwner();

		//PREVENT_PORTAL_AFTER_EXCHANGE
		GetOwner()->SetExchangeTime();
		victim->SetExchangeTime();
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

		// @fixme150 BEGIN
		if (quest::CQuestManager::instance().GetPCForce(GetOwner()->GetPlayerID())->IsRunning() == true)
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if you're using quests"));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if the other part using quests"));
			goto EXCHANGE_END;
		}
		else if (quest::CQuestManager::instance().GetPCForce(victim->GetPlayerID())->IsRunning() == true)
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if you're using quests"));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade if the other part using quests"));
			goto EXCHANGE_END;
		}
		// @fixme150 END

		if (!Check(&iItemCount))
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("돈이 부족하거나 아이템이 제자리에 없습니다."));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 돈이 부족하거나 아이템이 제자리에 없습니다."));
			goto EXCHANGE_END;
		}

		if (!CheckSpace())
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 소지품에 빈 공간이 없습니다."));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지품에 빈 공간이 없습니다."));
			goto EXCHANGE_END;
		}

		if (!GetCompany()->Check(&iItemCount))
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("돈이 부족하거나 아이템이 제자리에 없습니다."));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 돈이 부족하거나 아이템이 제자리에 없습니다."));
			goto EXCHANGE_END;
		}

		if (!GetCompany()->CheckSpace())
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 소지품에 빈 공간이 없습니다."));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지품에 빈 공간이 없습니다."));
			goto EXCHANGE_END;
		}

		if (db_clientdesc->GetSocket() == INVALID_SOCKET)
		{
			sys_err("Cannot use exchange feature while DB cache connection is dead.");
			victim->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			goto EXCHANGE_END;
		}

		if (Done())
		{
			if (m_lGold)
				GetOwner()->Save();

			if (GetCompany()->Done())
			{
				if (GetCompany()->m_lGold)
					victim->Save();

				// INTERNATIONAL_VERSION
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 님과의 교환이 성사 되었습니다."), victim->GetName());
				victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 님과의 교환이 성사 되었습니다."), GetOwner()->GetName());
				// END_OF_INTERNATIONAL_VERSION
				GetOwner()->SetStatistics(STAT_TYPE_TRADE, 1);
				victim->SetStatistics(STAT_TYPE_TRADE, 1);
			}
		}

EXCHANGE_END:
		Cancel();
		return false;
	}
	else
	{
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_ACCEPT, true, m_bAccept, NPOS, 0);
		exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_ACCEPT, false, m_bAccept, NPOS, 0);
		return true;
	}
}

void CExchange::Cancel()
{
	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_END, 0, 0, NPOS, 0);
	GetOwner()->SetExchange(NULL);

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			m_apItems[i]->SetExchanging(false);
	}

	if (GetCompany())
	{
		GetCompany()->SetCompany(NULL);
		GetCompany()->Cancel();
	}

	M2_DELETE(this);
}
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
