#include "stdafx.h"
#ifdef ENABLE_OFFLINESHOP_SYSTEM
#include "ClientManager.h"

void CClientManager::OfflineShopAssert(TOfflineShop* p){}

bool CClientManager::InitializeOfflineShop()
{
	MYSQL_ROW row;
	//for (size_t j = 1; j <= channelcount; j++)
	{
		MYSQL_ROW row_item;
		char szQuery[QUERY_MAX_LEN];
		std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery("SELECT * FROM player.offline_shop_npc"));
		if (pMsg && pMsg->Get()->uiNumRows != 0)
		{
			while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			{
				TOfflineShop* offlineshop = new TOfflineShop;

				memset(offlineshop, 0, sizeof(TOfflineShop));
				memset(&offlineshop->items, 0, sizeof(OFFLINE_SHOP_ITEM)*OFFLINE_SHOP_HOST_ITEM_MAX_NUM);
				memset(&offlineshop->log, 0, sizeof(ShopLog)*OFFLINE_SHOP_HOST_ITEM_MAX_NUM);


				str_to_number(offlineshop->owner_id, row[0]);
				strlcpy(offlineshop->owner_name, row[1], sizeof(offlineshop->owner_name));
				strlcpy(offlineshop->sign, row[2], sizeof(offlineshop->sign));
				str_to_number(offlineshop->x, row[3]);
				str_to_number(offlineshop->y, row[4]);
				str_to_number(offlineshop->z, row[5]);
				str_to_number(offlineshop->mapindex, row[6]);
				str_to_number(offlineshop->type, row[7]);
				str_to_number(offlineshop->channel, row[8]);
				str_to_number(offlineshop->slotflag, row[9]);
				str_to_number(offlineshop->time, row[10]);

				snprintf(szQuery, sizeof(szQuery), "SELECT shop_money FROM player.player WHERE id = %u", offlineshop->owner_id);
				std::unique_ptr<SQLMsg> pMsgPrice(CDBManager::instance().DirectQuery(szQuery));
				if (pMsgPrice->Get()->uiNumRows != 0)
				{
					MYSQL_ROW price = mysql_fetch_row(pMsgPrice->Get()->pSQLResult);
					str_to_number(offlineshop->price, price[0]);
				}
				else
					offlineshop->price = 0;

				int iLen = snprintf(szQuery, sizeof(szQuery), "SELECT id,owner_id,pos,count,vnum");
				for (BYTE j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
					iLen += snprintf(szQuery+ iLen, sizeof(szQuery)- iLen, ",socket%d",j);
				for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
					iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ",attrtype%d,attrvalue%d", j,j);
				iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ",price,status,ownername,buyername");

				iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, " FROM player.offline_shop_item WHERE owner_id = %u", offlineshop->owner_id);
				std::unique_ptr<SQLMsg> pMsg2(CDBManager::instance().DirectQuery(szQuery));
				int cur = 0;
				BYTE pos = 0;
				if (pMsg2 && pMsg2->Get()->uiNumRows != 0)
				{
					while (NULL != (row_item = mysql_fetch_row(pMsg2->Get()->pSQLResult)))
					{
						cur = 0;
						str_to_number(pos, row_item[2]);
						if (pos > OFFLINE_SHOP_HOST_ITEM_MAX_NUM)
							continue;
						OFFLINE_SHOP_ITEM& item = offlineshop->items[pos];
						str_to_number(item.id, row_item[cur++]);
						str_to_number(item.owner_id, row_item[cur++]);
						str_to_number(item.pos, row_item[cur++]);
						str_to_number(item.count, row_item[cur++]);
						str_to_number(item.vnum, row_item[cur++]);
						for (int j = 0; j < ITEM_SOCKET_MAX_NUM; j++)
							str_to_number(item.alSockets[j], row_item[cur++]);
						for (int j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++)
						{
							str_to_number(item.aAttr[j].bType, row_item[cur++]);
							str_to_number(item.aAttr[j].sValue, row_item[cur++]);
						}
						str_to_number(item.price, row_item[cur++]);
						str_to_number(item.status, row_item[cur++]);
						strlcpy(item.szOwnerName, row_item[cur++], sizeof(item.szOwnerName));
						strlcpy(item.szBuyerName, row_item[cur++], sizeof(item.szBuyerName));
					}
				}


				bool status = true;
				// for (DWORD i = 0; i < OFFLINE_SHOP_HOST_ITEM_MAX_NUM; ++i)
				// {
				// 	if (offlineshop->items[i].vnum != 0 && offlineshop->items[i].status == 0)
				// 	{
				// 		status = true;
				// 		break;
				// 	}
				// }

				if (!status)
				{
					char szQuery[1024];
					snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.offline_shop_npc WHERE owner_id = %u", offlineshop->owner_id);
					std::unique_ptr<SQLMsg> pMsg2(CDBManager::instance().DirectQuery(szQuery));
					snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.offline_shop_item WHERE owner_id = %u", offlineshop->owner_id);
					std::unique_ptr<SQLMsg> pMsg1(CDBManager::instance().DirectQuery(szQuery));
					delete offlineshop;
					continue;
				}


				int i = 0;
				snprintf(szQuery, sizeof(szQuery), "SELECT target, time, itemvnum, itemcount, price FROM player.offline_shop_log WHERE owner_id = %u", offlineshop->owner_id);
				std::unique_ptr<SQLMsg> pMsg5(CDBManager::instance().DirectQuery(szQuery));
				if (pMsg5 && pMsg5->Get()->uiNumRows != 0)
				{
					MYSQL_ROW log_row = NULL;
					while ((log_row = mysql_fetch_row(pMsg5->Get()->pSQLResult)))
					{
						strlcpy(offlineshop->log[i].name, log_row[0], sizeof(offlineshop->log[i].name));
						strlcpy(offlineshop->log[i].date, log_row[1], sizeof(offlineshop->log[i].date));
						str_to_number(offlineshop->log[i].itemVnum, log_row[2]);
						str_to_number(offlineshop->log[i].itemCount, log_row[3]);
						str_to_number(offlineshop->log[i].price, log_row[4]);
						++i;
					}
				}

				sys_log(0, "shop insert: owner_id: %u, owner_name: %s, sign: %s,x: %ld,y: %ld,mapIndex: %u, channel: %d",
					offlineshop->owner_id, offlineshop->owner_name, offlineshop->sign, offlineshop->x, offlineshop->y, offlineshop->mapindex, offlineshop->channel);

				m_Offlineshop.insert(std::make_pair(offlineshop->owner_id, offlineshop));
			}
		}
	}
	return true;
}

void CClientManager::CreateOfflineShop(TOfflineShop* p)
{
	if (!p)
		return;
	auto it = m_Offlineshop.find(p->owner_id);
	if (it != m_Offlineshop.end())
	{
		sys_err("wtf, this guy have offlineshop???????????? owner id %d name %s", p->owner_id, p->owner_name);
		OfflineShopAssert(p);// save player items...
		return;
	}

	int iLen = 0, iUpdateLen = 0;
	char szQuery[QUERY_MAX_LEN], szColumns[QUERY_MAX_LEN], szValues[QUERY_MAX_LEN];
	TOfflineShop* offlineshop = new TOfflineShop;
	thecore_memcpy(offlineshop, p, sizeof(TOfflineShop));
	m_Offlineshop.insert(std::make_pair(offlineshop->owner_id, offlineshop));

	snprintf(szQuery, sizeof(szQuery), "SELECT shop_money FROM player.player WHERE id = %u", offlineshop->owner_id);
	std::unique_ptr<SQLMsg> pMsgPrice(CDBManager::instance().DirectQuery(szQuery));
	if (pMsgPrice->Get()->uiNumRows != 0)
	{
		MYSQL_ROW price = mysql_fetch_row(pMsgPrice->Get()->pSQLResult);
		str_to_number(offlineshop->price, price[0]);
	}
	else
		offlineshop->price = 0;

	snprintf(szQuery, sizeof(szQuery), "INSERT INTO player.offline_shop_npc(owner_id, owner_name, sign, x, y, z, mapindex, type, channel, slotflag, time) VALUES(%u, '%s', '%s', %ld, %ld, %ld, %u, %d, %d, %lld, %d)",
		offlineshop->owner_id, offlineshop->owner_name, offlineshop->sign, offlineshop->x, offlineshop->y, offlineshop->z, offlineshop->mapindex, offlineshop->type, offlineshop->channel, offlineshop->slotflag, offlineshop->time);
	std::unique_ptr<SQLMsg> pMsg(CDBManager::Instance().DirectQuery(szQuery));
	for (DWORD i = 0; i < OFFLINE_SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		OFFLINE_SHOP_ITEM& item = p->items[i];
		if (item.vnum == 0)
			continue;

		iLen = snprintf(szColumns, sizeof(szColumns), "id,owner_id,pos,count,price,vnum");
		iUpdateLen = snprintf(szValues, sizeof(szValues), "%u,%u,%d,%d,%lld,%u", item.id, item.owner_id, item.pos, item.count, item.price, item.vnum);
		for (BYTE j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
		{
			iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen, ",socket%d",j);
			iUpdateLen += snprintf(szValues + iUpdateLen, sizeof(szValues) - iUpdateLen, ",%ld", item.alSockets[j]);
		}
		
		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
		{
			iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen, ",attrtype%d,attrvalue%d", j,j);
			iUpdateLen += snprintf(szValues + iUpdateLen, sizeof(szValues) - iUpdateLen, ",%d,%d", item.aAttr[j].bType, item.aAttr[j].sValue);
		}

		iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen, ",ownername");
		iUpdateLen += snprintf(szValues + iUpdateLen, sizeof(szValues) - iUpdateLen, ",'%s'", item.szOwnerName);

		snprintf(szQuery, sizeof(szQuery), "INSERT INTO player.offline_shop_item (%s) VALUES (%s)", szColumns, szValues);
		std::unique_ptr<SQLMsg> pMsg2(CDBManager::instance().DirectQuery(szQuery));
	}

	memset(&offlineshop->log, 0, sizeof(ShopLog) * OFFLINE_SHOP_HOST_ITEM_MAX_NUM);
	int i = 0;
	snprintf(szQuery, sizeof(szQuery), "SELECT target, time, itemvnum, itemcount, price FROM player.offline_shop_log WHERE owner_id = %u", offlineshop->owner_id);
	std::unique_ptr<SQLMsg> pMsg5(CDBManager::instance().DirectQuery(szQuery));
	if (pMsg5->Get()->uiNumRows != 0)
	{
		MYSQL_ROW log_row = NULL;
		while ((log_row = mysql_fetch_row(pMsg5->Get()->pSQLResult)))
		{
			strlcpy(offlineshop->log[i].name, log_row[0], sizeof(offlineshop->log[i].name));
			strlcpy(offlineshop->log[i].date, log_row[1], sizeof(offlineshop->log[i].date));
			str_to_number(offlineshop->log[i].itemVnum, log_row[2]);
			str_to_number(offlineshop->log[i].itemCount, log_row[3]);
			str_to_number(offlineshop->log[i].price, log_row[4]);
			++i;
		}
	}

	//int firstOffshopTime = time(0)+(60*60*24);
	//if(offlineshop->time < firstOffshopTime)
	//{
	//	offlineshop->time = firstOffshopTime;
	//	sys_err("shop insert: negative offshop time.... pid: %d name: %s ",offlineshop->owner_id, offlineshop->owner_name);
	//}

	sys_log(0, "shop insert: owner_id: %u, owner_name: %s, sign: %s,x: %ld,y: %ld,mapIndex: %u, channel: %d", 
	offlineshop->owner_id, offlineshop->owner_name, offlineshop->sign, offlineshop->x, offlineshop->y, offlineshop->mapindex, offlineshop->channel);


	shop_create n;
	n.subheader = CREATE_OFFLINESHOP;
	thecore_memcpy(&n.offlineshop, offlineshop, sizeof(n.offlineshop));

	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_create));//, 0, peer);
}

void CClientManager::DeleteOfflineShop(DWORD ownerid)
{
	auto it = m_Offlineshop.find(ownerid);
	if (it == m_Offlineshop.end())
		return;
	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.offline_shop_npc WHERE owner_id = %u", ownerid);
	std::unique_ptr<SQLMsg> pMsg2(CDBManager::instance().DirectQuery(szQuery));

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.offline_shop_item WHERE owner_id = %u and status = 1", ownerid);
	std::unique_ptr<SQLMsg> pMsg1(CDBManager::instance().DirectQuery(szQuery));

	delete it->second;
	it->second = NULL;
	m_Offlineshop.erase(it);
	
	sys_log(0, "DeleteOfflineShop %d ",ownerid);

	shop_owner n;
	n.subheader = DESTROY_OFFLINESHOP;
	n.owner_id = ownerid;
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_owner));
}

void CClientManager::RemoveAllItem(DWORD ownerID)
{
	auto it = m_Offlineshop.find(ownerID);
	if (it == m_Offlineshop.end())
		return;
	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.offline_shop_item WHERE owner_id = %u", ownerID);
	std::unique_ptr<SQLMsg> pMsg2(CDBManager::instance().DirectQuery(szQuery));
	memset(&it->second->items, 0, sizeof(it->second->items));

	remove_all n;
	n.subheader = REMOVE_ALL_ITEM;
	n.ownerID = ownerID;
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(remove_all));
}
void CClientManager::RemoveItem(OFFLINE_SHOP_ITEM* item)
{
	if (!item)
		return;
	auto it = m_Offlineshop.find(item->owner_id);
	if (it == m_Offlineshop.end())
		// todo player removes items...
		return;
	if (it->second->items[item->pos].vnum == 0)
		return;

	
	shop_item n;
	n.subheader = REMOVE_ITEM;
	//thecore_memcpy(&n.item, item, sizeof(n.item));
	thecore_memcpy(&n.item, &it->second->items[item->pos], sizeof(n.item));
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_item));

	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.offline_shop_item WHERE id = %u", item->id);
	std::unique_ptr<SQLMsg> pMsg2(CDBManager::instance().DirectQuery(szQuery));
	memset(&it->second->items[item->pos], 0, sizeof(OFFLINE_SHOP_ITEM));
	sys_log(0, "RemoveItem %d -> vnum %d count %d pos %d price %lld status %d buyername %s ", item->owner_id, item->vnum, item->count, item->pos, item->price, item->status, item->szBuyerName);
}

void CClientManager::AddItem(OFFLINE_SHOP_ITEM* item)
{
	if (!item)
		return;
	auto it = m_Offlineshop.find(item->owner_id);
	if (it == m_Offlineshop.end())
		// todo player removes items...
		return;
	thecore_memcpy(&it->second->items[item->pos], item, sizeof(OFFLINE_SHOP_ITEM));
	OFFLINE_SHOP_ITEM& newitem = it->second->items[item->pos];
	int iLen = 0, iUpdateLen = 0;
	char szQuery[QUERY_MAX_LEN], szColumns[QUERY_MAX_LEN], szValues[QUERY_MAX_LEN];

	iLen = snprintf(szColumns, sizeof(szColumns), "id,owner_id,pos,count,price,vnum");
	iUpdateLen = snprintf(szValues, sizeof(szValues), "%u,%u,%d,%d,%lld,%u", newitem.id, newitem.owner_id, newitem.pos, newitem.count, newitem.price, newitem.vnum);

	for (BYTE j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
	{
		iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen, ",socket%d",j);
		iUpdateLen += snprintf(szValues + iUpdateLen, sizeof(szValues) - iUpdateLen, ",%ld", newitem.alSockets[j]);
	}

	for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
	{
		iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen,",attrtype%d,attrvalue%d",j,j);
		iUpdateLen += snprintf(szValues + iUpdateLen, sizeof(szValues) - iUpdateLen, ",%d,%d", newitem.aAttr[j].bType, newitem.aAttr[j].sValue);
	}

	iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen, ",ownername");
	iUpdateLen += snprintf(szValues + iUpdateLen, sizeof(szValues) - iUpdateLen, ",'%s'", newitem.szOwnerName);
	snprintf(szQuery, sizeof(szQuery), "INSERT INTO player.offline_shop_item (%s) VALUES (%s)", szColumns, szValues);
	std::unique_ptr<SQLMsg> pMsg2(CDBManager::instance().DirectQuery(szQuery));

	sys_log(0, "AddItem %d -> vnum %d count %d pos %d price %lld ", newitem.owner_id, newitem.vnum, newitem.count, newitem.pos, newitem.price);

	shop_item n;
	n.subheader = ADD_ITEM;
	thecore_memcpy(&n.item, item, sizeof(n.item));
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_item));
	
}

void CClientManager::BuyItem(TOfflineShopBuy* buyItem)
{
	if (!buyItem)
		return;

	auto it = m_Offlineshop.find(buyItem->item.owner_id);
	if (it == m_Offlineshop.end())
		return;

	OFFLINE_SHOP_ITEM& item = it->second->items[buyItem->item.pos];
	if (item.vnum == 0 || item.status != 0)
		return;

	const long long calculateFeePrice = (100 - SHOP_FEE) * (buyItem->item.price / 100);

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "UPDATE player.player SET shop_money = shop_money + %lld WHERE id = %u", calculateFeePrice, buyItem->item.owner_id);

	std::unique_ptr<SQLMsg> pMsg1(CDBManager::instance().DirectQuery(szQuery));
	it->second->price += calculateFeePrice;
	item.status = 1;
	strlcpy(item.szBuyerName, buyItem->customer_name, sizeof(item.szBuyerName));

	memset(&buyItem->log, 0, sizeof(buyItem->log));
	buyItem->log_index = -1;

	for (DWORD i = 0; i < OFFLINE_SHOP_HOST_ITEM_MAX_NUM; i++)
	{
		ShopLog& log = it->second->log[i];
		if (log.price != 0)
			continue;

		time_t     now = time(0);
		struct tm  tstruct;
		char       buf[80];
		tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "%X-%d.%m.%Y", &tstruct);

		strlcpy(log.date, buf, sizeof(log.date));
		log.itemCount = item.count;
		log.itemVnum = item.vnum;
		strlcpy(log.name, buyItem->customer_name, sizeof(log.name));
		log.price = calculateFeePrice;

		buyItem->log_index = i;
		thecore_memcpy(&buyItem->log,&log, sizeof(buyItem->log));

		int iLen = snprintf(szQuery, sizeof(szQuery), "INSERT INTO player.offline_shop_log (owner_id,target,time,itemvnum,itemcount,price,item_id,pos");

		for (BYTE j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
			iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ",socket%d",j);
		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
			iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ",attrtype%d,attrvalue%d", j,j);
		iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ") VALUES(%u,'%s','%s',%d,%d,%lld,%u,%d", item.owner_id, buyItem->customer_name, buf, item.vnum, item.count, calculateFeePrice,item.id, item.pos);

		for (BYTE j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
			iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ",%ld", item.alSockets[j]);
		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
			iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ",%d,%d", item.aAttr[j].bType, item.aAttr[j].sValue);

		iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ")");

		std::unique_ptr<SQLMsg> pMsg2(CDBManager::instance().DirectQuery(szQuery));
		break;
	}

#ifdef AFTER_BUY_REMOVE_DIRECTLY
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.offline_shop_item WHERE id = %u", buyItem->item.id);
	memset(&item, 0, sizeof(item));
#else
	snprintf(szQuery, sizeof(szQuery), "UPDATE player.offline_shop_item SET status = 1, buyername = '%s' WHERE id = %u and owner_id = %u", buyItem->customer_name, buyItem->item.id, buyItem->item.owner_id);
	thecore_memcpy(&buyItem->item, &item, sizeof(buyItem->item));
#endif
	std::unique_ptr<SQLMsg> pMsg2(CDBManager::instance().DirectQuery(szQuery));
	sys_log(0, "BuyItem %d -> buyer name %s vnum %d count %d pos %d price %lld ", buyItem->item.owner_id, buyItem->customer_name, buyItem->item.vnum, buyItem->item.count, buyItem->item.pos, buyItem->item.price);

	shop_buy n;
	n.subheader = BUY_ITEM;
	thecore_memcpy(&n.buyItem, buyItem, sizeof(n.buyItem));
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_buy));
}

void CClientManager::WithdrawMoney(DWORD ch)
{
	auto it = m_Offlineshop.find(ch);
	if (it == m_Offlineshop.end())
		// todo player removes price...
		return;
	if (it->second->price == 0)
		return;
	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "UPDATE player.player SET shop_money = 0 WHERE id = '%u'",ch);
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));

	sys_log(0, "WithdrawMoney %d -> %lld ", ch, it->second->price);
	it->second->price = 0;

	shop_owner n;
	n.subheader = WITHDRAW_MONEY;
	n.owner_id = ch;
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_owner));
}

void CClientManager::ChangeTitle(TOfflineShopChangeTitle* p)
{
	if (!p)
		return;
	auto it = m_Offlineshop.find(p->owner_id);
	if (it == m_Offlineshop.end())
		// todo player removes price...
		return;
	strlcpy(it->second->sign, p->sign, sizeof(it->second->sign));
	char szQuery[1024 + 1];
	snprintf(szQuery, sizeof(szQuery), "UPDATE player.offline_shop_npc SET sign = '%s' WHERE owner_id = '%d'", p->sign, p->owner_id);
	std::unique_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));
	sys_log(0, "ChangeTitle %d -> %s ", p->owner_id, p->sign);


	shop_title n;
	n.subheader = CHANGE_TITLE;
	thecore_memcpy(&n.title, p, sizeof(n.title));
	ForwardPacket(HEADER_DG_OFFLINESHOP,&n, sizeof(shop_title));
}

void CClientManager::ClearLog(DWORD ch)
{
	auto it = m_Offlineshop.find(ch);
	if (it == m_Offlineshop.end())
		return;
	memset(&it->second->log, 0, sizeof(it->second->log));
	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.offline_shop_log WHERE owner_id = %u", it->second->owner_id);
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));
	sys_log(0, "ClearLog %d ", ch);

	shop_owner n;
	n.subheader = CLEAR_LOG;
	n.owner_id = ch;
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_owner));
}

void CClientManager::ChangeDecoration(TShopDecoration* ch)
{
	if (!ch)
		return;
	auto it = m_Offlineshop.find(ch->owner_id);
	if (it == m_Offlineshop.end())
		return;
	it->second->type = 30000 + ch->vnum;
	strlcpy(it->second->sign, ch->sign, sizeof(it->second->sign));

	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "UPDATE player.offline_shop_npc SET type = %d, sign = '%s' WHERE owner_id = %u", it->second->type, it->second->sign,it->first);
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));
	sys_log(0, "ChangeDecoration %d -> %d %s ", ch->owner_id, 30000 + ch->vnum, ch->sign);

	shop_decoration n;
	n.subheader = CHANGE_DECORATION;
	thecore_memcpy(&n.decoration, ch, sizeof(n.decoration));
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_decoration));
}

void CClientManager::OpenSlot(TOfflineShopOpenSlot* ch)
{
	if (!ch)
		return;
	auto it = m_Offlineshop.find(ch->owner_id);
	if (it == m_Offlineshop.end())
		return;
	it->second->slotflag = ch->flag;
	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "UPDATE player.offline_shop_npc SET slotflag = %lld WHERE owner_id = %u", ch->flag,ch->owner_id);
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));
	sys_log(0, "OpenSlot %d -> %lld ", ch->owner_id, ch->flag);

	shop_slot n;
	n.subheader = CHANGE_OPEN_SLOT;
	thecore_memcpy(&n.ch, ch, sizeof(n.ch));
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_slot));
	
}

void CClientManager::TimeDone(DWORD ch)
{
	auto it = m_Offlineshop.find(ch);
	if (it == m_Offlineshop.end())
		return;
	sys_log(0, "TimeDone %d ", ch);

	bool status = true;
	for (DWORD i = 0; i < OFFLINE_SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		if (it->second->items[i].vnum != 0 && it->second->items[i].status == 0) {
			status = false;
			break;
		}
	}
	if (status)
	{
		char szQuery[1024];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.offline_shop_npc WHERE owner_id = %u", ch);
		std::unique_ptr<SQLMsg> pMsg2(CDBManager::instance().DirectQuery(szQuery));
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.offline_shop_item WHERE owner_id = %u", ch);
		std::unique_ptr<SQLMsg> pMsg1(CDBManager::instance().DirectQuery(szQuery));
		delete it->second;
		it->second = NULL;
		m_Offlineshop.erase(it);

		sys_log(0, "TimeDone: %d shop closed. 0 item.", ch);
	}
	else
		sys_log(0, "TimeDone: %d ", ch);

	shop_owner n;
	n.subheader = TIME_DONE;
	n.owner_id = ch;
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_owner));
}

void CClientManager::GetBackItem(DWORD ch)
{
	auto it = m_Offlineshop.find(ch);
	if (it == m_Offlineshop.end())
		return;

	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.offline_shop_npc WHERE owner_id = %u", ch);
	std::unique_ptr<SQLMsg> pMsg2(CDBManager::instance().DirectQuery(szQuery));

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.offline_shop_item WHERE owner_id = %u", ch);
	std::unique_ptr<SQLMsg> pMsg1(CDBManager::instance().DirectQuery(szQuery));

	shop_back n;
	n.subheader = GET_BACK_ITEM;
	n.back.owner_id = ch;
	thecore_memcpy(&n.back.items, &it->second->items, sizeof(n.back.items));
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_back));

	delete it->second;
	it->second = NULL;
	m_Offlineshop.erase(it);
	sys_log(0, "GetBackItem %d ", ch);
}

void CClientManager::ChangePrice(DWORD ownerID, WORD bPos, long long itemPrice)
{
	auto it = m_Offlineshop.find(ownerID);
	if (it == m_Offlineshop.end())
		return;
	if (it->second->items[bPos].vnum == 0 || it->second->items[bPos].price == itemPrice)
		return;

	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "UPDATE player.offline_shop_npc SET price = %lld WHERE id = %u", itemPrice, it->second->items[bPos].id);
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));

	sys_log(0, "change price %u - pos: %u %lld -> %lld", ownerID, bPos, it->second->items[bPos].price, itemPrice);
	it->second->items[bPos].price = itemPrice;
	change_price n;
	n.subheader = CHANGE_PRICE;
	n.ownerID = ownerID;
	n.bPos = bPos;
	n.itemPrice = itemPrice;
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(change_price));
}
void CClientManager::MoveItem(DWORD ownerID, WORD slotPos, WORD targetPos)
{
	auto it = m_Offlineshop.find(ownerID);
	if (it == m_Offlineshop.end())
		return;
	if (it->second->items[slotPos].vnum == 0 || it->second->items[targetPos].vnum != 0)
		return;

	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "UPDATE player.offline_shop_item SET pos = %u WHERE id = %u AND pos = %u", targetPos, it->second->items[slotPos].id, slotPos);
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));

	thecore_memcpy(&it->second->items[targetPos], &it->second->items[slotPos], sizeof(it->second->items[targetPos]));
	memset(&it->second->items[slotPos], 0, sizeof(it->second->items[slotPos]));

	it->second->items[targetPos].pos = targetPos;

	move_item n;
	n.subheader = MOVE_ITEM;
	n.ownerID = ownerID;
	n.slotPos = slotPos;
	n.targetPos = targetPos;
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(move_item));

	sys_log(0, "moveitem %u - pos: %u -> %u", ownerID, slotPos, targetPos);
}

void CClientManager::AddTime(DWORD ch)
{
	auto it = m_Offlineshop.find(ch);
	if (it == m_Offlineshop.end())
		return;

	it->second->time = time(0) + SHOP_TIME_ADD_TIME;

	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "UPDATE player.offline_shop_npc SET time = %d WHERE owner_id = %u", it->second->time, it->second->owner_id);
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));

	shop_owner n;
	n.subheader = ADD_TIME;
	n.owner_id = ch;
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_owner));
	sys_log(0, "AddTime %d ", ch);
}

void CClientManager::AddTimePremium(DWORD ch)
{
	auto it = m_Offlineshop.find(ch);
	if (it == m_Offlineshop.end())
		return;

	it->second->time = time(0) + SHOP_TIME_ADD_TIME_PREMIUM;

	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "UPDATE player.offline_shop_npc SET time = %d WHERE owner_id = %u", it->second->time, it->second->owner_id);
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));

	shop_owner n;
	n.subheader = ADD_TIME_PREMIUM;
	n.owner_id = ch;
	ForwardPacket(HEADER_DG_OFFLINESHOP, &n, sizeof(shop_owner));
	sys_log(0, "AddTime Premium %d ", ch);
}

void CClientManager::RecvPackets(const char* data)
{
	BYTE sub_header = (BYTE)*data;
	switch (sub_header)
	{
		case CREATE_OFFLINESHOP:
		{
			shop_create* p = (shop_create*)data;
			if (p)
				CreateOfflineShop(&p->offlineshop);
		}
		break;
		case DESTROY_OFFLINESHOP:
		{
			shop_owner* p = (shop_owner*)data;
			if (p)
				DeleteOfflineShop(p->owner_id);
		}
		break;
		case ADD_ITEM:
		{
			shop_item* p = (shop_item*)data;
			if (p)
				AddItem(&p->item);
		}
		break;
		case REMOVE_ITEM:
		{
			shop_item* p = (shop_item*)data;
			if (p)
				RemoveItem(&p->item);
		}
		break;
		case BUY_ITEM:
		{
			shop_buy* p = (shop_buy*)data;
			if (p)
				BuyItem(&p->buyItem);
		}
		break;
		case CHANGE_TITLE:
		{
			shop_title* p = (shop_title*)data;
			if (p)
				ChangeTitle(&p->title);
		}
		break;

		case WITHDRAW_MONEY:
		{
			shop_owner* p = (shop_owner*)data;
			if (p)
				WithdrawMoney(p->owner_id);
		}
		break;

		case CLEAR_LOG:
		{
			shop_owner* p = (shop_owner*)data;
			if (p)
				ClearLog(p->owner_id);
		}
		break;

		case CHANGE_DECORATION:
		{
			shop_decoration* p = (shop_decoration*)data;
			if (p)
				ChangeDecoration(&p->decoration);
		}
		break;

		case CHANGE_OPEN_SLOT:
		{
			shop_slot* p = (shop_slot*)data;
			if (p)
				OpenSlot(&p->ch);
		}
		break;

		case TIME_DONE:
		{
			shop_owner* p = (shop_owner*)data;
			if (p)
				TimeDone(p->owner_id);
		}
		break;

		case GET_BACK_ITEM:
		{
			shop_owner* p = (shop_owner*)data;
			if (p)
				GetBackItem(p->owner_id);
		}
		break;

		case ADD_TIME:
		{
			shop_owner* p = (shop_owner*)data;
			if (p)
				AddTime(p->owner_id);
		}
		break;
		case ADD_TIME_PREMIUM:
		{
			shop_owner* p = (shop_owner*)data;
			if (p)
				AddTimePremium(p->owner_id);
		}
		break;
		case MOVE_ITEM:
		{
			move_item* p = (move_item*)data;
			if (p)
				MoveItem(p->ownerID, p->slotPos, p->targetPos);
		}
		break;

		case CHANGE_PRICE:
		{
			change_price* p = (change_price*)data;
			if (p)
				ChangePrice(p->ownerID, p->bPos, p->itemPrice);
		}
		break;

		case REMOVE_ALL_ITEM:
		{
			remove_all* p = (remove_all*)data;
			if (p)
				RemoveAllItem(p->ownerID);
		}
		break;
		default:
			sys_err("unkown offlineshop subheader %d", sub_header);
	}
}
#endif

