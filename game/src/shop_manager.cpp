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
#include "shop_manager.h"
#include "group_text_parse_tree.h"
#include "shopEx.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/unordered_set.hpp>
#include "shop_manager.h"
#include <cctype>
#ifdef ENABLE_NEWSTUFF
#include "../../common/PulseManager.h"
#endif

CShopManager::CShopManager()
{
}

CShopManager::~CShopManager()
{
	Destroy();
}

bool CShopManager::Initialize(TShopTable * table, int size)
{
	if (!m_map_pkShop.empty())
		return false;

	int i;

	for (i = 0; i < size; ++i, ++table)
	{
		LPSHOP shop = M2_NEW CShop;

		if (!shop->Create(table->dwVnum, table->dwNPCVnum, table->items))
		{
			M2_DELETE(shop);
			continue;
		}

		m_map_pkShop.emplace(table->dwVnum, shop);
		m_map_pkShopByNPCVnum.emplace(table->dwNPCVnum, shop);
	}
	char szShopTableExFileName[256];

	snprintf(szShopTableExFileName, sizeof(szShopTableExFileName),
		"%s/shop_table_ex.txt", LocaleService_GetBasePath().c_str());

	return ReadShopTableEx(szShopTableExFileName);
}

void CShopManager::Destroy()
{
	TShopMap::iterator it = m_map_pkShop.begin();

	while (it != m_map_pkShop.end())
	{
		M2_DELETE(it->second);
		++it;
	}

	m_map_pkShop.clear();
}

LPSHOP CShopManager::Get(DWORD dwVnum)
{
	TShopMap::const_iterator it = m_map_pkShop.find(dwVnum);

	if (it == m_map_pkShop.end())
		return NULL;

	return (it->second);
}

LPSHOP CShopManager::GetByNPCVnum(DWORD dwVnum)
{
	TShopMap::const_iterator it = m_map_pkShopByNPCVnum.find(dwVnum);

	if (it == m_map_pkShopByNPCVnum.end())
		return NULL;

	return (it->second);
}

bool CShopManager::StartShopping(LPCHARACTER pkChr, LPCHARACTER pkChrShopKeeper, int iShopVnum)
{
	if (pkChr->IsDead()) // @fixme326
		return false;

	// this method is only for NPC
	if(pkChrShopKeeper)
	{
		if (pkChr->GetShopOwner() == pkChrShopKeeper)
			return false;
		if (pkChrShopKeeper->IsPC())
			return false;

		const long distance = DISTANCE_APPROX(pkChr->GetX() - pkChrShopKeeper->GetX(), pkChr->GetY() - pkChrShopKeeper->GetY());
		if (distance >= SHOP_MAX_DISTANCE)
		{
			sys_log(1, "SHOP: TOO_FAR: %s distance %d", pkChr->GetName(), distance);
			return false;
		}
	}
	
	// PREVENT_TRADE_WINDOW
	if (pkChr->PreventTradeWindow(WND_SHOPOWNER, true/*except*/))
	{
		pkChr->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래창이 열린상태에서는 상점거래를 할수 가 없습니다."));
		return false;
	}
	// END_PREVENT_TRADE_WINDOW

	LPSHOP pkShop;
	DWORD vid = 0;

	if (iShopVnum)
		pkShop = Get(iShopVnum);
	else
	{
		if (pkChrShopKeeper) {
			pkShop = GetByNPCVnum(pkChrShopKeeper->GetRaceNum());
			vid = pkChrShopKeeper->GetVID();
		}
	}

	if (!pkShop)
	{
		sys_log(1, "SHOP: NO SHOP");
		return false;
	}

	bool bOtherEmpire = false;
	if (pkChrShopKeeper && pkChr->GetEmpire() != pkChrShopKeeper->GetEmpire())
		bOtherEmpire = false;


	pkShop->AddGuest(pkChr, vid, bOtherEmpire);

	if(pkChrShopKeeper)
		pkChr->SetShopOwner(pkChrShopKeeper);
	sys_log(0, "SHOP: START: %s", pkChr->GetName());
	return true;
}

LPSHOP CShopManager::FindPCShop(DWORD dwVID)
{
	TShopMap::iterator it = m_map_pkShopByPC.find(dwVID);

	if (it == m_map_pkShopByPC.end())
		return NULL;

	return it->second;
}

LPSHOP CShopManager::CreatePCShop(LPCHARACTER ch, TShopItemTable * pTable, BYTE bItemCount)
{
	if (FindPCShop(ch->GetVID()))
		return NULL;

	LPSHOP pkShop = M2_NEW CShop;
	pkShop->SetPCShop(ch);
	pkShop->SetShopItems(pTable, bItemCount);

	m_map_pkShopByPC.emplace(ch->GetVID(), pkShop);
	return pkShop;
}

void CShopManager::DestroyPCShop(LPCHARACTER ch)
{
	LPSHOP pkShop = FindPCShop(ch->GetVID());

	if (!pkShop)
		return;

	//PREVENT_ITEM_COPY;
	ch->SetMyShopTime();
	//END_PREVENT_ITEM_COPY

	m_map_pkShopByPC.erase(ch->GetVID());
	M2_DELETE(pkShop);
}

void CShopManager::StopShopping(LPCHARACTER ch)
{
	LPSHOP shop;

	if (!(shop = ch->GetShop()))
		return;

	//PREVENT_ITEM_COPY;
	ch->SetMyShopTime();
	//END_PREVENT_ITEM_COPY

	shop->RemoveGuest(ch);
	sys_log(0, "SHOP: END: %s", ch->GetName());
}

void CShopManager::Buy(LPCHARACTER ch, BYTE pos, int dwCount)
{
#ifdef ENABLE_NEWSTUFF
	if (g_BuySellTimeLimitValue && !PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::BoxOpening, std::chrono::milliseconds(g_BuySellTimeLimitValue)))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("퀘스트를 로드하는 중입니다. 잠시만 기다려 주십시오."));
		return;
	}
#endif
	if (!ch->GetShop())
		return;

	if (ch->GetShopOwner())
	{
		if (DISTANCE_APPROX(ch->GetX() - ch->GetShopOwner()->GetX(), ch->GetY() - ch->GetShopOwner()->GetY()) > 2000)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상점과의 거리가 너무 멀어 물건을 살 수 없습니다."));
			return;
		}
	}

	CShop* pkShop = ch->GetShop();

	if (!pkShop->IsPCShop())
	{
		//if (pkShop->GetVnum() == 0)
		//	return;
		//const CMob* pkMob = CMobManager::instance().Get(pkShop->GetNPCVnum());
		//if (!pkMob)
		//	return;

		//if (pkMob->m_table.bType != CHAR_TYPE_NPC)
		//{
		//	return;
		//}
	}
	else
	{
	}

	if (dwCount > 50)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Log registered. Action cannot be made. (Max multiple buy: 50)");
		return;
	}

	//PREVENT_ITEM_COPY
	ch->SetMyShopTime();
	//END_PREVENT_ITEM_COPY

	int ret = pkShop->Buy(ch, pos, dwCount);

	if (SHOP_SUBHEADER_GC_OK != ret)
	{
		TPacketGCShop pack;

		pack.header	= HEADER_GC_SHOP;
		pack.subheader	= ret;
		pack.size	= sizeof(TPacketGCShop);

		ch->GetDesc()->Packet(&pack, sizeof(pack));
	}
}

void CShopManager::Sell(LPCHARACTER ch, WORD bCell, DWORD dwCount)
{
	int64_t bCount = dwCount;

#ifdef ENABLE_NEWSTUFF
	if (g_BuySellTimeLimitValue && !PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::BoxOpening, std::chrono::milliseconds(g_BuySellTimeLimitValue)))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("퀘스트를 로드하는 중입니다. 잠시만 기다려 주십시오."));
		return;
	}
#endif

	if (!ch->GetShop())
		return;

	if (!ch->CanHandleItem())
		return;

	if (ch->GetShop()->IsPCShop())
		return;

	if (!ch->GetShopOwner())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't sell in portable shops."));
		return;
	}

	if (ch->GetShopOwner())
	{
		if (DISTANCE_APPROX(ch->GetX()-ch->GetShopOwner()->GetX(), ch->GetY()-ch->GetShopOwner()->GetY())>2000)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상점과의 거리가 너무 멀어 물건을 팔 수 없습니다."));
			return;
		}
	}

	LPITEM item = ch->GetInventoryItem(bCell);

	if (!item)
		return;

	if (item->IsEquipped() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("착용 중인 아이템은 판매할 수 없습니다."));
		return;
	}

	if (true == item->isLocked())
	{
		return;
	}

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_SELL) || item->GetSocket(4) == 99)
		return;

	if ((item->GetType() == ITEM_BUFFI && item->GetSubType() == BUFFI_SCROLL) && item->GetSocket(1) == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Deactivate the Buffi Seal first."));
		return;
	}

	if ((item->GetVnum() >= 51010 && item->GetVnum() <= 51030) && item->GetSocket(0) == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Deactivate the Energy Crystal first."));
		return;
	}

	int64_t dwPrice;
	dwPrice = item->GetGold();

	if (bCount == 0 || bCount > item->GetCount())
		bCount = item->GetCount();

	if (IS_SET(item->GetFlag(), ITEM_FLAG_COUNT_PER_1GOLD))
	{
		if (dwPrice == 0)
			dwPrice = bCount;
		else
			dwPrice = bCount / dwPrice;
	}
	else
		dwPrice *= bCount;

#ifndef ENABLE_NO_SELL_PRICE_DIVIDED_BY_5
	dwPrice /= 5;
#endif

	int64_t dwTax = 0;
#if defined(ENABLE_TAX_CHANGES)
	int64_t iVal = NEW_TAX_VARIABLE;
#else
	int iVal = 3;
#endif

	dwTax = dwPrice * iVal / 100;
	dwPrice -= dwTax;

	if (test_server)
		sys_log(0, "Sell Item price id %d %s itemid %d", ch->GetPlayerID(), ch->GetName(), item->GetID());

	const int64_t nTotalMoney = static_cast<int64_t>(ch->GetGold()) + static_cast<int64_t>(dwPrice);

	if (GOLD_MAX <= nTotalMoney)
	{
		sys_err("[OVERFLOW_GOLD] id %u name %s gold %u", ch->GetPlayerID(), ch->GetName(), ch->GetGold());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("20억냥이 초과하여 물품을 팔수 없습니다."));
		return;
	}

	sys_log(0, "SHOP: SELL: %s item name: %s(x%d):%u price: %lld", ch->GetName(), item->GetName(), bCount, item->GetID(), dwPrice);

	if (iVal > 0)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("판매금액의 %d %% 가 세금으로 나가게됩니다"), iVal);

	DBManager::instance().SendMoneyLog(MONEY_LOG_SHOP, item->GetVnum(), dwPrice);

	if (bCount == item->GetCount())
		ITEM_MANAGER::instance().RemoveItem(item, "SELL");
	else
		item->SetCount(item->GetCount() - bCount);

	CMonarch::instance().SendtoDBAddMoney(dwTax, ch->GetEmpire(), ch);

#ifdef ENABLE_BATTLE_PASS
	CHARACTER_MANAGER::Instance().DoMission(ch, MISSION_SELL_ITEM, bCount, item->GetVnum());
#endif
	ch->PointChange(POINT_GOLD, static_cast<int64_t>(dwPrice), false);
}

bool CompareShopItemName(const SShopItemTable& lhs, const SShopItemTable& rhs)
{
	TItemTable* lItem = ITEM_MANAGER::instance().GetTable(lhs.vnum);
	TItemTable* rItem = ITEM_MANAGER::instance().GetTable(rhs.vnum);
	if (lItem && rItem)
		return strcmp(lItem->szLocaleName, rItem->szLocaleName) < 0;
	else
		return true;
}

#if defined(__SHOPEX_RENEWAL__)
bool CompareShopItemVnum(const SShopItemTable& lhs, const SShopItemTable& rhs)
{
	return lhs.vnum > rhs.vnum;
}

bool CompareShopItemPrice(const SShopItemTable& lhs, const SShopItemTable& rhs)
{
	return lhs.price > rhs.price;
}

bool CompareShopItemType(const SShopItemTable& lhs, const SShopItemTable& rhs)
{
	const TItemTable* lItem = ITEM_MANAGER::instance().GetTable(lhs.vnum);
	const TItemTable* rItem = ITEM_MANAGER::instance().GetTable(rhs.vnum);
	return (lItem && rItem) ? lItem->bType > rItem->bType : true;
}
#endif

bool ConvertToShopItemTable(IN CGroupNode* pNode, OUT TShopTableEx& shopTable)
{
	if (!pNode->GetValue("vnum", 0, shopTable.dwVnum))
	{
		sys_err("Group %s does not have vnum.", pNode->GetNodeName().c_str());
		return false;
	}

	if (!pNode->GetValue("name", 0, shopTable.name))
	{
		sys_err("Group %s does not have name.", pNode->GetNodeName().c_str());
		return false;
	}

	if (shopTable.name.length() >= SHOP_TAB_NAME_MAX)
	{
		sys_err("Shop name length must be less than %d. Error in Group %s, name %s", SHOP_TAB_NAME_MAX, pNode->GetNodeName().c_str(), shopTable.name.c_str());
		return false;
	}

	std::string stCoinType;
	if (!pNode->GetValue("cointype", 0, stCoinType))
	{
		stCoinType = "Gold";
	}

	if (boost::iequals(stCoinType, "Gold"))
	{
		shopTable.coinType = SHOP_COIN_TYPE_GOLD;
	}
	else if (boost::iequals(stCoinType, "SecondaryCoin"))
	{
		shopTable.coinType = SHOP_COIN_TYPE_SECONDARY_COIN;
	}
	else
	{
		sys_err("Group %s has undefine cointype(%s).", pNode->GetNodeName().c_str(), stCoinType.c_str());
		return false;
	}

	CGroupNode* pItemGroup = pNode->GetChildNode("items");
	if (!pItemGroup)
	{
		sys_err("Group %s does not have 'group items'.", pNode->GetNodeName().c_str());
		return false;
	}

	size_t itemGroupSize = pItemGroup->GetRowCount();
	std::vector <TShopItemTable> shopItems(itemGroupSize);
	if (itemGroupSize >= SHOP_HOST_ITEM_MAX_NUM)
	{
		sys_err("count(%d) of rows of group items of group %s must be smaller than %d", itemGroupSize, pNode->GetNodeName().c_str(), SHOP_HOST_ITEM_MAX_NUM);
		return false;
	}

	for (size_t i = 0; i < itemGroupSize; i++)
	{
		if (!pItemGroup->GetValue(i, "vnum", shopItems[i].vnum))
		{
			sys_err("row(%d) of group items of group %s does not have vnum column", i, pNode->GetNodeName().c_str());
			return false;
		}

		if (!pItemGroup->GetValue(i, "count", shopItems[i].count))
		{
			sys_err("row(%d) of group items of group %s does not have count column", i, pNode->GetNodeName().c_str());
			return false;
		}
		if (!pItemGroup->GetValue(i, "price", shopItems[i].price))
		{
			sys_err("row(%d) of group items of group %s does not have price column", i, pNode->GetNodeName().c_str());
			return false;
		}
#if defined(__SHOPEX_RENEWAL__)
		if (!pItemGroup->GetValue(i, "price1", shopItems[i].price1))
		{
			sys_err("row(%d) of group items of group %s does not have price column", i, pNode->GetNodeName().c_str());
			return false;
		}
		if (!pItemGroup->GetValue(i, "price2", shopItems[i].price2))
		{
			sys_err("row(%d) of group items of group %s does not have price column", i, pNode->GetNodeName().c_str());
			return false;
		}
		if (!pItemGroup->GetValue(i, "price3", shopItems[i].price3))
		{
			sys_err("row(%d) of group items of group %s does not have price column", i, pNode->GetNodeName().c_str());
			return false;
		}
		if (!pItemGroup->GetValue(i, "price4", shopItems[i].price4))
		{
			sys_err("row(%d) of group items of group %s does not have price column", i, pNode->GetNodeName().c_str());
			return false;
		}

		if (shopItems[i].price_type >= EX_MAX || shopItems[i].price_type < EX_GOLD)
		{
			sys_err("row(%d) of group items of group %s price_type is wrong!", i, pNode->GetNodeName().c_str());
			return false;
		}
		char getval[ITEM_SOCKET_MAX_NUM+ITEM_ATTRIBUTE_MAX_NUM];
		for (int j = 0; j < ITEM_SOCKET_MAX_NUM; j++)
		{
			snprintf(getval, sizeof(getval), "socket%d", j);
			if (!pItemGroup->GetValue(i, getval, shopItems[i].alSockets[j]))
			{
				sys_err("row(%d) stage %d of group items of group %s does not have socket column", i, j, pNode->GetNodeName().c_str());
				return false;
			}
		}
		for (int j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++)
		{
			snprintf(getval, sizeof(getval), "attr_type%d", j);
			if (!pItemGroup->GetValue(i, getval, shopItems[i].aAttr[j].bType))
			{
				sys_err("row(%d) stage %d of group items of group %s does not have attr_type column", i, j, pNode->GetNodeName().c_str());
				return false;
			}
			snprintf(getval, sizeof(getval), "attr_value%d", j);
			if (!pItemGroup->GetValue(i, getval, shopItems[i].aAttr[j].sValue))
			{
				sys_err("row(%d) stage %d of group items of group %s does not have attr_value column", i, j, pNode->GetNodeName().c_str());
				return false;
			}
		}
		if (pItemGroup->GetValue(i, "price_type", shopItems[i].price_type) && pItemGroup->GetValue(i, "price_vnum", shopItems[i].price_vnum) && shopItems[i].price_type == 3)
		{
			if (shopItems[i].price_vnum != 0)
			{
				if (!ITEM_MANAGER::instance().GetTable(shopItems[i].price_vnum))
				{
					sys_err("NOT GET ITEM PROTO 1 %d", shopItems[i].price_vnum);
					return false;
				}
			}
		}
		if (pItemGroup->GetValue(i, "price_vnum1", shopItems[i].price_vnum1) && shopItems[i].price_type == 3)
		{
			if (shopItems[i].price_vnum1 != 0)
			{
				if (!ITEM_MANAGER::instance().GetTable(shopItems[i].price_vnum1))
				{
					sys_err("NOT GET ITEM PROTO 1 %d", shopItems[i].price_vnum1);
					return false;
				}
			}
		}
		if (pItemGroup->GetValue(i, "price_vnum2", shopItems[i].price_vnum2) && shopItems[i].price_type == 3)
		{
			if (shopItems[i].price_vnum2 != 0)
			{
				if (!ITEM_MANAGER::instance().GetTable(shopItems[i].price_vnum2))
				{
					sys_err("NOT GET ITEM PROTO 2 %d", shopItems[i].price_vnum2);
					return false;
				}
			}
		}
		if (pItemGroup->GetValue(i, "price_vnum3", shopItems[i].price_vnum3) && shopItems[i].price_type == 3)
		{
			if (shopItems[i].price_vnum3 != 0)
			{
				if (!ITEM_MANAGER::instance().GetTable(shopItems[i].price_vnum3))
				{
					sys_err("NOT GET ITEM PROTO 3 %d", shopItems[i].price_vnum3);
					return false;
				}
			}
		}
		if (pItemGroup->GetValue(i, "price_vnum4", shopItems[i].price_vnum4) && shopItems[i].price_type == 3)
		{
			if (shopItems[i].price_vnum4 != 0)
			{
				if (!ITEM_MANAGER::instance().GetTable(shopItems[i].price_vnum4))
				{
					sys_err("NOT GET ITEM PROTO 4 %d", shopItems[i].price_vnum4);
					return false;
				}
			}
		}
#endif
	}
	std::string stSort;
	if (!pNode->GetValue("sort", 0, stSort))
	{
		stSort = "None";
	}

	if (boost::iequals(stSort, "Asc"))
	{
		std::sort(shopItems.begin(), shopItems.end(), CompareShopItemName);
	}
	else if(boost::iequals(stSort, "Desc"))
	{
		std::sort(shopItems.rbegin(), shopItems.rend(), CompareShopItemName);
	}
#if defined(__SHOPEX_RENEWAL__)
	else if (boost::iequals(stSort, "Vnum"))
		std::sort(shopItems.rbegin(), shopItems.rend(), CompareShopItemVnum);
	else if (boost::iequals(stSort, "Price"))
		std::sort(shopItems.begin(), shopItems.end(), CompareShopItemPrice);
	else if (boost::iequals(stSort, "Name"))
		std::sort(shopItems.begin(), shopItems.end(), CompareShopItemName);
	else if (boost::iequals(stSort, "Type"))
		std::sort(shopItems.begin(), shopItems.end(), CompareShopItemType);
#endif

	CGrid grid = CGrid(5, 9);
	int iPos;

	memset(&shopTable.items[0], 0, sizeof(shopTable.items));

	for (size_t i = 0; i < shopItems.size(); i++)
	{
		TItemTable * item_table = ITEM_MANAGER::instance().GetTable(shopItems[i].vnum);
		if (!item_table)
		{
			sys_err("vnum(%d) of group items of group %s does not exist", shopItems[i].vnum, pNode->GetNodeName().c_str());
			return false;
		}

		iPos = grid.FindBlank(1, item_table->bSize);

		grid.Put(iPos, 1, item_table->bSize);
		shopTable.items[iPos] = shopItems[i];
	}

	shopTable.byItemCount = shopItems.size();
	return true;
}

bool CShopManager::ReadShopTableEx(const char* stFileName)
{
	// file 유무 체크.
	// 없는 경우는 에러로 처리하지 않는다.
	FILE* fp = fopen(stFileName, "rb");
	if (NULL == fp)
		return true;
	fclose(fp);

	CGroupTextParseTreeLoader loader;
	if (!loader.Load(stFileName))
	{
		sys_err("%s Load fail.", stFileName);
		return false;
	}

	CGroupNode* pShopNPCGroup = loader.GetGroup("shopnpc");
	if (NULL == pShopNPCGroup)
	{
		sys_err("Group ShopNPC is not exist.");
		return false;
	}

	typedef std::multimap <DWORD, TShopTableEx> TMapNPCshop;
	TMapNPCshop map_npcShop;

#if defined(__SHOPEX_RENEWAL__)
	{
		boost::unordered_set<CShop*> v;
		// include unordered_set
		auto ExDelete = [&v](TShopMap& c)
		{
			for (auto it = c.begin(); !c.empty() && it != c.end();)
			{
				const auto shop = it->second;
				if (shop && shop->IsShopEx())
				{
					it = c.erase(it);
					v.emplace(shop);
				}
				else
					++it;
			}
		};
		ExDelete(m_map_pkShopByNPCVnum);
		ExDelete(m_map_pkShop);
		for (const auto& del : v)
			delete del;
	}
#endif

	for (size_t i = 0; i < pShopNPCGroup->GetRowCount(); i++)
	{
		DWORD npcVnum;
		std::string shopName;
		if (!pShopNPCGroup->GetValue(i, "npc", npcVnum) || !pShopNPCGroup->GetValue(i, "group", shopName))
		{
			sys_err("Invalid row(%d). Group ShopNPC rows must have 'npc', 'group' columns", i);
			return false;
		}
		std::transform(shopName.begin(), shopName.end(), shopName.begin(), (int(*)(int))std::tolower);
		CGroupNode* pShopGroup = loader.GetGroup(shopName.c_str());
		if (!pShopGroup)
		{
			sys_err("Group %s is not exist.", shopName.c_str());
			return false;
		}
		TShopTableEx table;
		if (!ConvertToShopItemTable(pShopGroup, table))
		{
			sys_err("Cannot read Group %s.", shopName.c_str());
			return false;
		}
		if (m_map_pkShopByNPCVnum.find(npcVnum) != m_map_pkShopByNPCVnum.end())
		{
			sys_err("%d cannot have both original shop and extended shop", npcVnum);
			return false;
		}

		map_npcShop.insert(TMapNPCshop::value_type(npcVnum, table));
	}

	for (TMapNPCshop::iterator it = map_npcShop.begin(); it != map_npcShop.end(); ++it)
	{
		DWORD npcVnum = it->first;
		TShopTableEx& table = it->second;
		if (m_map_pkShop.find(table.dwVnum) != m_map_pkShop.end())
		{
			sys_err("Shop vnum(%d) already exists", table.dwVnum);
			return false;
		}
		TShopMap::iterator shop_it = m_map_pkShopByNPCVnum.find(npcVnum);

		LPSHOPEX pkShopEx = NULL;
		if (m_map_pkShopByNPCVnum.end() == shop_it)
		{
			pkShopEx = M2_NEW CShopEx;
			pkShopEx->Create(0, npcVnum);
			m_map_pkShopByNPCVnum.insert(TShopMap::value_type(npcVnum, pkShopEx));
		}
		else
		{
			pkShopEx = dynamic_cast <CShopEx*> (shop_it->second);
			if (NULL == pkShopEx)
			{
				sys_err("WTF!!! It can't be happend. NPC(%d) Shop is not extended version.", shop_it->first);
				return false;
			}
		}

		if (pkShopEx->GetTabCount() >= SHOP_TAB_COUNT_MAX)
		{
			sys_err("ShopEx cannot have tab more than %d", SHOP_TAB_COUNT_MAX);
			return false;
		}

		if (m_map_pkShop.find(table.dwVnum) != m_map_pkShop.end())
		{
			sys_err("Shop vnum(%d) already exist.", table.dwVnum);
			return false;
		}
		m_map_pkShop.insert(TShopMap::value_type(table.dwVnum, pkShopEx));
		pkShopEx->AddShopTable(table);
	}

	return true;
}
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
