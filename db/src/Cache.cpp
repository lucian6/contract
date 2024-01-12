#include "stdafx.h"
#include "Cache.h"

#include "QID.h"
#include "ClientManager.h"
#include "Main.h"
#include <fmt/fmt.h>

extern CPacketInfo g_item_info;
extern int g_iPlayerCacheFlushSeconds;
extern int g_iItemCacheFlushSeconds;
extern int g_test_server;
// MYSHOP_PRICE_LIST
extern int g_iItemPriceListTableCacheFlushSeconds;
// END_OF_MYSHOP_PRICE_LIST
//
extern int g_item_count;

CItemCache::CItemCache()
{
	m_expireTime = MIN(1800, g_iItemCacheFlushSeconds);
}

CItemCache::~CItemCache()
{
}

// fixme
// by rtsummit
void CItemCache::Delete()
{
	if (m_data.vnum == 0)
		return;

	//char szQuery[QUERY_MAX_LEN];
	//szQuery[QUERY_MAX_LEN] = '\0';
	if (g_test_server)
		sys_log(0, "ItemCache::Delete : DELETE %u", m_data.id);

	m_data.vnum = 0;
	m_bNeedQuery = true;
	m_lastUpdateTime = time(0);
	OnFlush();

	//m_bNeedQuery = false;
}

void CItemCache::OnFlush()
{
	if (m_data.vnum == 0)
	{
		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM item%s WHERE id=%u", GetTablePostfix(), m_data.id);
		CDBManager::instance().ReturnQuery(szQuery, QID_ITEM_DESTROY, 0, NULL);

		if (g_test_server)
			sys_log(0, "ItemCache::Flush : DELETE %u %s", m_data.id, szQuery);
	}
	else
	{
		TPlayerItem *p = &m_data;
		const auto setQuery = fmt::format(FMT_COMPILE("id={}, owner_id={}, window={}, pos={}, count={}, vnum={}, socket0={}, socket1={}, socket2={}, socket3={}, socket4={}, socket5={}, socket6={}, socket7={}, "
														"attrtype0={}, attrvalue0={}, "
														"attrtype1={}, attrvalue1={}, "
														"attrtype2={}, attrvalue2={}, "
														"attrtype3={}, attrvalue3={}, "
														"attrtype4={}, attrvalue4={}, "
														"attrtype5={}, attrvalue5={}, "
														"attrtype6={}, attrvalue6={}, "
														"attrtype7={}, attrvalue7={}, "
														"attrtype8={}, attrvalue8={}, "
														"attrtype9={}, attrvalue9={}, "
														"attrtype10={}, attrvalue10={}, "
														"attrtype11={}, attrvalue11={}, "
														"attrtype12={}, attrvalue12={}, "
														"attrtype13={}, attrvalue13={}, "
														"attrtype14={}, attrvalue14={} ")
														, p->id,
														p->owner,
														p->window,
														p->pos,
														p->count,
														p->vnum,
														p->alSockets[0],
														p->alSockets[1],
														p->alSockets[2],
														p->alSockets[3],
														p->alSockets[4],
														p->alSockets[5],
														p->alSockets[6],
														p->alSockets[7],
														p->aAttr[0].bType, p->aAttr[0].sValue,
														p->aAttr[1].bType, p->aAttr[1].sValue,
														p->aAttr[2].bType, p->aAttr[2].sValue,
														p->aAttr[3].bType, p->aAttr[3].sValue,
														p->aAttr[4].bType, p->aAttr[4].sValue,
														p->aAttr[5].bType, p->aAttr[5].sValue,
														p->aAttr[6].bType, p->aAttr[6].sValue,
														p->aAttr[7].bType, p->aAttr[7].sValue,
														p->aAttr[8].bType, p->aAttr[8].sValue,
														p->aAttr[9].bType, p->aAttr[9].sValue,
														p->aAttr[10].bType, p->aAttr[10].sValue,
														p->aAttr[11].bType, p->aAttr[11].sValue,
														p->aAttr[12].bType, p->aAttr[12].sValue,
														p->aAttr[13].bType, p->aAttr[13].sValue,
														p->aAttr[14].bType, p->aAttr[14].sValue
		); // @fixme205
		
		const auto itemQuery = fmt::format(FMT_COMPILE("INSERT INTO item{} SET {} ON DUPLICATE KEY UPDATE {}"),
														GetTablePostfix(), setQuery, setQuery);

		if (g_test_server)
			sys_log(0, "ItemCache::Flush :REPLACE  (%s)", itemQuery.c_str());

		CDBManager::instance().ReturnQuery(itemQuery.c_str(), QID_ITEM_SAVE, 0, NULL);

		++g_item_count;
	}

	m_bNeedQuery = false;
}

#if defined(__SKILL_COLOR_SYSTEM__)
//
// CSKillColorCache
//

extern int g_iSkillColorCacheFlushSeconds;

CSKillColorCache::CSKillColorCache()
{
	m_expireTime = MIN(1800, g_iSkillColorCacheFlushSeconds);
}

CSKillColorCache::~CSKillColorCache()
{
}

void CSKillColorCache::OnFlush()
{
	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery),
		"REPLACE INTO skill_color%s (player_id"
		// Skill Slots
		", skillSlot1_Col1, skillSlot1_Col2, skillSlot1_Col3, skillSlot1_Col4, skillSlot1_Col5"
		", skillSlot2_Col1, skillSlot2_Col2, skillSlot2_Col3, skillSlot2_Col4, skillSlot2_Col5"
		", skillSlot3_Col1, skillSlot3_Col2, skillSlot3_Col3, skillSlot3_Col4, skillSlot3_Col5"
		", skillSlot4_Col1, skillSlot4_Col2, skillSlot4_Col3, skillSlot4_Col4, skillSlot4_Col5"
		", skillSlot5_Col1, skillSlot5_Col2, skillSlot5_Col3, skillSlot5_Col4, skillSlot5_Col5"
		", skillSlot6_Col1, skillSlot6_Col2, skillSlot6_Col3, skillSlot6_Col4, skillSlot6_Col5"
		", skillSlot7_Col1, skillSlot7_Col2, skillSlot7_Col3, skillSlot7_Col4, skillSlot7_Col5"
		", skillSlot8_Col1, skillSlot8_Col2, skillSlot8_Col3, skillSlot8_Col4, skillSlot8_Col5"
		", skillSlot9_Col1, skillSlot9_Col2, skillSlot9_Col3, skillSlot9_Col4, skillSlot9_Col5"
		", skillSlot10_Col1, skillSlot10_Col2, skillSlot10_Col3, skillSlot10_Col4, skillSlot10_Col5"
		// Buff Skills
		", buffSkill1_Col1, buffSkill1_Col2, buffSkill1_Col3, buffSkill1_Col4, buffSkill1_Col5"
		", buffSkill2_Col1, buffSkill2_Col2, buffSkill2_Col3, buffSkill2_Col4, buffSkill2_Col5"
		", buffSkill3_Col1, buffSkill3_Col2, buffSkill3_Col3, buffSkill3_Col4, buffSkill3_Col5"
		", buffSkill4_Col1, buffSkill4_Col2, buffSkill4_Col3, buffSkill4_Col4, buffSkill4_Col5"
		", buffSkill5_Col1, buffSkill5_Col2, buffSkill5_Col3, buffSkill5_Col4, buffSkill5_Col5"
		", buffSkill6_Col1, buffSkill6_Col2, buffSkill6_Col3, buffSkill6_Col4, buffSkill6_Col5"

		") "
		"VALUES (%d"
		// Skill Slots <Slot, Col1, Col2, Col3, Col4, Col5>
		", %d, %d, %d, %d, %d" // Skill Slot 1
		", %d, %d, %d, %d, %d" // Skill Slot 2
		", %d, %d, %d, %d, %d" // Skill Slot 3
		", %d, %d, %d, %d, %d" // Skill Slot 4
		", %d, %d, %d, %d, %d" // Skill Slot 5
		", %d, %d, %d, %d, %d" // Skill Slot 6 (End of Skills)

		", %d, %d, %d, %d, %d" // Skill Slot 7 (Unused)
		", %d, %d, %d, %d, %d" // Skill Slot 8 (Unused)
		", %d, %d, %d, %d, %d" // Skill Slot 9 (Conqueror)
		", %d, %d, %d, %d, %d" // Skill Slot 10 (End of Skills)
		// Buff Skills
		", %d, %d, %d, %d, %d" // Buff Skill 11 (Shaman Group 1)
		", %d, %d, %d, %d, %d" // Buff Skill 12 (Shaman Group 1)
		", %d, %d, %d, %d, %d" // Buff Skill 13 (Shaman Group 1)
		", %d, %d, %d, %d, %d" // Buff Skill 14 (Shaman Group 2)
		", %d, %d, %d, %d, %d" // Buff Skill 15 (Shaman Group 2)
		", %d, %d, %d, %d, %d" // Buff Skill 16 (Wolfman)

		")", GetTablePostfix(), m_data.dwPlayerID
		, m_data.dwSkillColor[0][0], m_data.dwSkillColor[0][1], m_data.dwSkillColor[0][2], m_data.dwSkillColor[0][3], m_data.dwSkillColor[0][4]
		, m_data.dwSkillColor[1][0], m_data.dwSkillColor[1][1], m_data.dwSkillColor[1][2], m_data.dwSkillColor[1][3], m_data.dwSkillColor[1][4]
		, m_data.dwSkillColor[2][0], m_data.dwSkillColor[2][1], m_data.dwSkillColor[2][2], m_data.dwSkillColor[2][3], m_data.dwSkillColor[2][4]
		, m_data.dwSkillColor[3][0], m_data.dwSkillColor[3][1], m_data.dwSkillColor[3][2], m_data.dwSkillColor[3][3], m_data.dwSkillColor[3][4]
		, m_data.dwSkillColor[4][0], m_data.dwSkillColor[4][1], m_data.dwSkillColor[4][2], m_data.dwSkillColor[4][3], m_data.dwSkillColor[4][4]
		, m_data.dwSkillColor[5][0], m_data.dwSkillColor[5][1], m_data.dwSkillColor[5][2], m_data.dwSkillColor[5][3], m_data.dwSkillColor[5][4]

		, m_data.dwSkillColor[6][0], m_data.dwSkillColor[6][1], m_data.dwSkillColor[6][2], m_data.dwSkillColor[6][3], m_data.dwSkillColor[6][4]
		, m_data.dwSkillColor[7][0], m_data.dwSkillColor[7][1], m_data.dwSkillColor[7][2], m_data.dwSkillColor[7][3], m_data.dwSkillColor[7][4]
		, m_data.dwSkillColor[8][0], m_data.dwSkillColor[8][1], m_data.dwSkillColor[8][2], m_data.dwSkillColor[8][3], m_data.dwSkillColor[8][4]
		, m_data.dwSkillColor[9][0], m_data.dwSkillColor[9][1], m_data.dwSkillColor[9][2], m_data.dwSkillColor[9][3], m_data.dwSkillColor[9][4]
		, m_data.dwSkillColor[10][0], m_data.dwSkillColor[10][1], m_data.dwSkillColor[10][2], m_data.dwSkillColor[10][3], m_data.dwSkillColor[10][4]
		, m_data.dwSkillColor[11][0], m_data.dwSkillColor[11][1], m_data.dwSkillColor[11][2], m_data.dwSkillColor[11][3], m_data.dwSkillColor[11][4]
		, m_data.dwSkillColor[12][0], m_data.dwSkillColor[12][1], m_data.dwSkillColor[12][2], m_data.dwSkillColor[12][3], m_data.dwSkillColor[12][4]
		, m_data.dwSkillColor[13][0], m_data.dwSkillColor[13][1], m_data.dwSkillColor[13][2], m_data.dwSkillColor[13][3], m_data.dwSkillColor[13][4]
		, m_data.dwSkillColor[14][0], m_data.dwSkillColor[14][1], m_data.dwSkillColor[14][2], m_data.dwSkillColor[14][3], m_data.dwSkillColor[14][4]
		, m_data.dwSkillColor[15][0], m_data.dwSkillColor[15][1], m_data.dwSkillColor[15][2], m_data.dwSkillColor[15][3], m_data.dwSkillColor[15][4]

	);

	CDBManager::instance().ReturnQuery(szQuery, QID_SKILL_COLOR_SAVE, 0, NULL);

	if (g_test_server)
		sys_log(0, "SkillColorCache::Flush :REPLACE %u (%s)", m_data.dwPlayerID, szQuery);

	m_bNeedQuery = false;
}
#endif

//
// CPlayerTableCache
//
CPlayerTableCache::CPlayerTableCache()
{
	m_expireTime = MIN(1800, g_iPlayerCacheFlushSeconds);
}

CPlayerTableCache::~CPlayerTableCache()
{
}

void CPlayerTableCache::OnFlush()
{
	if (g_test_server)
		sys_log(0, "PlayerTableCache::Flush : %s", m_data.name);

	char szQuery[QUERY_MAX_LEN];
	CreatePlayerSaveQuery(szQuery, sizeof(szQuery), &m_data);
	CDBManager::instance().ReturnQuery(szQuery, QID_PLAYER_SAVE, 0, NULL);
}

// MYSHOP_PRICE_LIST
//
// CItemPriceListTableCache class implementation
//

const int CItemPriceListTableCache::s_nMinFlushSec = 1800;

CItemPriceListTableCache::CItemPriceListTableCache()
{
	m_expireTime = MIN(s_nMinFlushSec, g_iItemPriceListTableCacheFlushSeconds);
}

void CItemPriceListTableCache::UpdateList(const TItemPriceListTable* pUpdateList)
{
	std::vector<TItemPriceInfo> tmpvec;

	for (uint idx = 0; idx < m_data.byCount; ++idx)
	{
		const TItemPriceInfo* pos = pUpdateList->aPriceInfo;
		for (; pos != pUpdateList->aPriceInfo + pUpdateList->byCount && m_data.aPriceInfo[idx].dwVnum != pos->dwVnum; ++pos)
			;

		if (pos == pUpdateList->aPriceInfo + pUpdateList->byCount)
			tmpvec.emplace_back(m_data.aPriceInfo[idx]);
	}

	if (pUpdateList->byCount > SHOP_PRICELIST_MAX_NUM)
	{
		sys_err("Count overflow!");
		return;
	}

	m_data.byCount = pUpdateList->byCount;

	thecore_memcpy(m_data.aPriceInfo, pUpdateList->aPriceInfo, sizeof(TItemPriceInfo) * pUpdateList->byCount);

	int nDeletedNum;

	if (pUpdateList->byCount < SHOP_PRICELIST_MAX_NUM)
	{
		size_t sizeAddOldDataSize = SHOP_PRICELIST_MAX_NUM - pUpdateList->byCount;

		if (tmpvec.size() < sizeAddOldDataSize)
			sizeAddOldDataSize = tmpvec.size();
		if (tmpvec.size() != 0)
		{
			thecore_memcpy(m_data.aPriceInfo + pUpdateList->byCount, &tmpvec[0], sizeof(TItemPriceInfo) * sizeAddOldDataSize);
			m_data.byCount += sizeAddOldDataSize;
		}
		nDeletedNum = tmpvec.size() - sizeAddOldDataSize;
	}
	else
		nDeletedNum = tmpvec.size();

	m_bNeedQuery = true;

	sys_log(0,
			"ItemPriceListTableCache::UpdateList : OwnerID[%u] Update [%u] Items, Delete [%u] Items, Total [%u] Items",
			m_data.dwOwnerID, pUpdateList->byCount, nDeletedNum, m_data.byCount);
}

void CItemPriceListTableCache::OnFlush()
{
	char szQuery[QUERY_MAX_LEN];

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM myshop_pricelist%s WHERE owner_id = %u", GetTablePostfix(), m_data.dwOwnerID);
	CDBManager::instance().ReturnQuery(szQuery, QID_ITEMPRICE_DESTROY, 0, NULL);

	for (int idx = 0; idx < m_data.byCount; ++idx)
	{
		snprintf(szQuery, sizeof(szQuery),
				"REPLACE myshop_pricelist%s(owner_id, item_vnum, price) VALUES(%u, %u, %u)", // @fixme204 (INSERT INTO -> REPLACE)
				GetTablePostfix(), m_data.dwOwnerID, m_data.aPriceInfo[idx].dwVnum, m_data.aPriceInfo[idx].dwPrice);
		CDBManager::instance().ReturnQuery(szQuery, QID_ITEMPRICE_SAVE, 0, NULL);
	}

	sys_log(0, "ItemPriceListTableCache::Flush : OwnerID[%u] Update [%u]Items", m_data.dwOwnerID, m_data.byCount);

	m_bNeedQuery = false;
}

CItemPriceListTableCache::~CItemPriceListTableCache()
{
}

// END_OF_MYSHOP_PRICE_LIST
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
