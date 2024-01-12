#include "stdafx.h"

#ifdef ENABLE_OFFLINESHOP_SYSTEM
#ifdef ENABLE_SHOP_SEARCH_SYSTEM
#include "utils.h"
#include "config.h"
#include "desc.h"
#include "char.h"
#include "item_manager.h"
#include "packet.h"
#include "offline_shop.h"
#include "offlineshop_manager.h"
#include "buffer_manager.h"
#include "DragonSoul.h"
#include "char_manager.h"
#include "p2p.h"
#include "desc.h"
#include "desc_manager.h"
#include "desc_client.h"

void replace(std::string& str, const std::string& from, const std::string& to) {
	while (true)
	{
		const size_t start_pos = str.find(from);
		if (start_pos == std::string::npos)
			break;
		str.replace(start_pos, from.length(), to);
	}
}

bool COfflineShopManager::CompareNames(const std::string searchedInput, DWORD itemIdx, bool exactSearch, BYTE playerLanguage)
{
	const auto it = m_mapItemNames.find(playerLanguage);
	if (it != m_mapItemNames.end())
	{
		const auto itItem = it->second.find(itemIdx);
		if (itItem != it->second.end())
			return exactSearch ? (itItem->second == searchedInput) : (itItem->second.find(searchedInput.c_str()) != std::string::npos);
	}
	return false;
}

void COfflineShopManager::LoadSearchLanguage()
{
	const std::map<BYTE, std::string> m_vecLang = {
		{LOCALE_EN, "en"},
		{LOCALE_PT, "pt"},
		{LOCALE_ES, "es"},
		{LOCALE_FR, "fr"},
		{LOCALE_GR, "gr"},
		{LOCALE_DE, "de"},
		{LOCALE_RO, "ro"},
		{LOCALE_PL, "pl"},
		{LOCALE_IT, "it"},
		{LOCALE_CZ, "cz"},
		{LOCALE_HU, "hu"},
		{LOCALE_TR, "tr"},
	};
	m_mapItemNames.clear();
	std::map<DWORD, std::string> m_ItemNames;
	for (auto it = m_vecLang.begin(); it != m_vecLang.end(); ++it)
	{
		char filename[40];
		snprintf(filename, sizeof(filename), "locale/germany/country/%s/item_names.txt", it->second.c_str());
		char	one_line[256];
		FILE* fp = fopen(filename, "r");
		if (fp != NULL)
		{
			m_ItemNames.clear();
			while (fgets(one_line, 256, fp))
			{
				std::vector<std::string> m_vec;
				split_argument(one_line, m_vec, "\t");
				if (m_vec.size() < 2)
					continue;
				else if (m_vec[0].length() < 1 || m_vec[1].length() < 1)
					continue;
				DWORD itemIdx;
				if (!str_to_number(itemIdx, m_vec[0].c_str()))
					continue;

				std::string itemName(m_vec[1].c_str());
				replace(itemName, "\n", "");
				std::transform(itemName.begin(), itemName.end(), itemName.begin(), [](unsigned char c) { return std::tolower(c); });
				m_ItemNames.emplace(itemIdx, itemName);
			}
			fclose(fp);
			if (m_ItemNames.size())
				m_mapItemNames.emplace(it->first, m_ItemNames);
		}
	}
}


#include "desc_manager.h"

int GetRefineLevel(const char* name)
{
	char* p = const_cast<char*>(strrchr(name, '+'));
	if (!p)
		return 0;
	int	rtn = 0;
	str_to_number(rtn, p + 1);
	return rtn;
}

void COfflineShopManager::CompareFilter(searchFilter& filter, const char* szCommand)
{
	std::vector<std::string> vecArgs;
	split_argument(szCommand, vecArgs, "$");
	const bool printInfo = false;
	if(printInfo)
		sys_err("");
	// type^0^-1$checkbox^0^0$input^hello½world½worl$combo^0^0^0^0^0^0^0^0^$editline^?^?^?^?^?^?^?^?^$attr^6^2000^-1^0^-1^0^-1^0^-1^0^$
	for (BYTE j = 0; j < vecArgs.size(); ++j)
	{
		std::vector<std::string> vec;
		split_argument(vecArgs[j].c_str(), vec, "^");
		if (vec.size() <= 1)
			continue;
		std::string filterType(vec[0]);
		replace(filterType, " ", "");
		if (printInfo)
			sys_err("%s - vecSize: %d", filterType.c_str(), vec.size());
		if (filterType == "type")
		{
			if (vec.size() < 3) continue;
			if (!str_to_number(filter.searchType, vec[1].c_str()) || !str_to_number(filter.searchSubType, vec[2].c_str()))
				continue;
			if (printInfo)
				sys_err("searchType: %d searchSubType: %d", filter.searchType, filter.searchSubType);
		}
		else if (filterType == "checkbox")
		{
			if (vec.size() < 3) continue;
			BYTE exactSearch, playerSearch;
			if (!str_to_number(exactSearch, vec[1].c_str()) || !str_to_number(playerSearch, vec[2].c_str()))
				continue;
			filter.exactSearch = exactSearch ? true : false;
			filter.playerSearch = playerSearch ? true : false;
			if (printInfo)
				sys_err("exactSearch: %d playerSearch: %d", filter.exactSearch, filter.playerSearch);
		}
		else if (filterType == "input")
		{
			if (vec.size() < 2) continue;
			std::string newString(vec[1].c_str());
			if (!newString.length())
				continue;
			replace(newString, "½", " ");
			replace(newString, "Â", "");
			filter.searchInput = newString;
			if (printInfo)
				sys_err("searchInput: %s searchInputNEW: %s", filter.searchInput.c_str(), vec[1].c_str());
		}
		else if (filterType == "race")
		{
			if (vec.size() < 5) continue;
			for (BYTE x = 0; x < 4; ++x)
			{
				if (!str_to_number(filter.race[x], vec[1 + x].c_str()))
					continue;
				if (printInfo)
					sys_err("race%d: %d", x, filter.race[x]);
			}
		}
		else if (filterType == "sex")
		{
			if (vec.size() < 3) continue;
			for (BYTE x = 0; x < 2; ++x)
			{
				if (!str_to_number(filter.gender[x], vec[1 + x].c_str()))
					continue;
				if (printInfo)
					sys_err("sex%d: %d", x, filter.gender[x]);
			}
		}
		else if (filterType == "combo")
		{
			if (vec.size() < 9) continue;
			if (!str_to_number(filter.minRefine, vec[1].c_str())
				|| !str_to_number(filter.maxRefine, vec[2].c_str())
				|| !str_to_number(filter.alchemyClass, vec[3].c_str())
				|| !str_to_number(filter.alchemyLevel, vec[4].c_str())
				|| !str_to_number(filter.costumeType, vec[5].c_str())
				|| !str_to_number(filter.costumeRarity, vec[6].c_str())
				|| !str_to_number(filter.sashType, vec[7].c_str())
				|| !str_to_number(filter.timeType, vec[8].c_str())
				)
				continue;
			if (printInfo)
				sys_err("minRefine: %d maxRefine: %d alchemyClass: %d alchemyLevel: %d costumeType: %d costumeRarity: %d sashType: %d timeType: %d",
				filter.minRefine, filter.maxRefine, filter.alchemyClass, filter.alchemyLevel, filter.costumeType, filter.costumeRarity, filter.sashType, filter.timeType);
		}
		else if (filterType == "editline")
		{
			if (vec.size() < 9) continue;
			if (!str_to_number(filter.minCount, vec[1].c_str()))
				filter.minCount = 0;
			if (!str_to_number(filter.maxCount, vec[2].c_str()))
				filter.maxCount = 0;
			if (!str_to_number(filter.minAverage, vec[3].c_str()))
				filter.minAverage = 0;
			if (!str_to_number(filter.minSkill, vec[4].c_str()))
				filter.minSkill = 0;
			if (!str_to_number(filter.minAbs, vec[5].c_str()))
				filter.minAbs = 0;
			if (!str_to_number(filter.maxAbs, vec[6].c_str()))
				filter.maxAbs = 0;
			if (!str_to_number(filter.minLevel, vec[7].c_str()))
				filter.minLevel = 0;
			if (!str_to_number(filter.maxLevel, vec[8].c_str()))
				filter.maxLevel = 0;
			if (printInfo)
				sys_err("minCount: %d maxCount: %d minAverage: %d minSkill: %d minAbs: %d maxAbs: %d minLevel: %d maxLevel: %d",
				filter.minCount, filter.maxCount, filter.minAverage, filter.minSkill, filter.minAbs, filter.maxAbs, filter.minLevel, filter.maxLevel);
		}
		else if (filterType == "attr")
		{
			if (vec.size() < 11) continue;
			for (BYTE x = 0; x < 5; ++x)
			{
				int attrType, attrValue;
				if (!str_to_number(attrType, vec[1 + (x * 2)].c_str()) || !str_to_number(attrValue, vec[1 + 1 + (x * 2)].c_str())) continue;
				if (attrType > 0 && attrType <= 255 && attrValue > 0 && attrValue < SHRT_MAX)
				{
					for (BYTE r = 0; r < 5; ++r)
					{
						if (filter.aAttr[r].bType == 0)
						{
							filter.aAttr[r].bType = attrType;
							filter.aAttr[r].sValue = attrValue;
							if (printInfo)
								sys_err("attr%d: %d value: %d", r, attrType, attrValue);
							break;
						}
					}
				}
			}
		}
	}
}

bool IsRaceCheckItem(const DWORD itemIdx, const BYTE itemType, const BYTE itemSubType)
{
	switch (itemType)
	{
	case ITEM_WEAPON:
	case ITEM_ARMOR:
	case ITEM_COSTUME:
		return true;
	}
	return false;
}

bool IsGenderCheckItem(const DWORD itemIdx, const BYTE itemType, const BYTE itemSubType)
{
	switch (itemType)
	{
	case ITEM_COSTUME:
		return true;
	}
	return false;
}


bool COfflineShopManager::CheckFilter(const searchFilter* filter, const OFFLINE_SHOP_ITEM* item, const BYTE playerLang)
{
	const DWORD itemIdx = item->vnum;
	
	if (itemIdx == 0 || item->status != 0)
		return false;
	
	if (IsRealTimeItem(item))
	{
		RemoveItemTimeDone(item);
		return false;
	}

	if (item->count < filter->minCount || (filter->maxCount != 0 && item->count > filter->maxCount))
		return false;
	
	if (!filter->playerSearch && filter->searchInput.length())
	{
		if (!CompareNames(filter->searchInput, itemIdx, filter->exactSearch, playerLang))
			return false;
	}
	
	const TItemTable* table = ITEM_MANAGER::instance().GetTable(itemIdx);
	if (!table)
		return false;
	
	const BYTE itemType = table->bType;
	const BYTE itemSubType = table->bSubType;

	if (filter->searchType > 0)
	{
		if (GetItemCategory(table->dwVnum, itemType, itemSubType) != std::make_pair(filter->searchType, filter->searchSubType))
			return false;
	}

	if (filter->minRefine != 0)
	{
		const int itemRefineLevel = GetRefineLevel(table->szLocaleName);
		if (itemRefineLevel < filter->minRefine)
			return false;
	}

	if (filter->maxRefine != 0)
	{
		const int itemRefineLevel = GetRefineLevel(table->szLocaleName);
		if (itemRefineLevel > filter->maxRefine)
			return false;
	}

	if (filter->minAverage > 0)
	{
		if (item->aAttr[0].bType == APPLY_NORMAL_HIT_DAMAGE_BONUS)
		{
			if (item->aAttr[0].sValue < filter->minAverage)
				return false;
		}
		else
			return false;
	}
	if (filter->minSkill > 0)
	{
		if (item->aAttr[0].bType == APPLY_SKILL_DAMAGE_BONUS)
		{
			if (item->aAttr[0].sValue < filter->minSkill)
				return false;
		}
		else if (item->aAttr[1].bType == APPLY_SKILL_DAMAGE_BONUS)
		{
			if (item->aAttr[1].sValue < filter->minSkill)
				return false;
		}
		else
			return false;
	}

	//Race
	if (IsRaceCheckItem(itemIdx, itemType, itemSubType) && (filter->race[0] || filter->race[1] || filter->race[2] || filter->race[3]))
	{
		for (BYTE j = 0; j < JOB_MAX_NUM; ++j)
		{
			if (!(table->dwAntiFlags & (1 << (2+j))) && !filter->race[j])
				return false;
		}
	}
	
	//Gender
	if (IsGenderCheckItem(itemIdx, itemType, itemSubType) && (filter->gender[0] || filter->gender[1]))
	{
		for (BYTE j = 0; j < 2; ++j)
		{
			if (!(table->dwAntiFlags & (1 << (0 + j))) && !filter->gender[j])
				return false;
		}
	}

	if (itemType == ITEM_DS && (filter->alchemyClass != 0 || filter->alchemyLevel != 0))
	{
		BYTE ds_type, grade_idx, step_idx, strength_idx;
		DSManager::Instance().GetDragonSoulInfo(itemIdx, ds_type, grade_idx, step_idx, strength_idx);
		if (filter->alchemyClass != 0 && grade_idx != filter->alchemyClass - 1)
			return false;
		if (filter->alchemyLevel != 0 && strength_idx != filter->alchemyLevel - 1)
			return false;
	}

	if (itemType == ITEM_COSTUME)
	{
		if (filter->costumeType > 0)
		{
			// constexpr BYTE costumeList[6] = { COSTUME_BODY, COSTUME_WEAPON, COSTUME_HAIR, COSTUME_ACCE, COSTUME_PET, COSTUME_MOUNT };
			constexpr BYTE costumeList[6] = { COSTUME_BODY, COSTUME_WEAPON, COSTUME_HAIR, COSTUME_ACCE_SKIN, COSTUME_NORMAL_PET_SKIN, COSTUME_MOUNT_SKIN };
			if (costumeList[(5) < (filter->costumeType - 1) ? (5) : (filter->costumeType - 1)] != itemSubType)
				return false;
		}

		if (filter->costumeRarity > 0)
		{
			//put your costume rarity method!
		}

		if (itemSubType == COSTUME_ACCE)
		{
			if (filter->sashType > 0)
			{
				//put acce 1 grade itemIdx like example in list.
				const std::vector<DWORD> m_acceItemIdxList = { 85001, 85005, 85011, 85015, 85021, 86001, 86005, 86011, 86015, 86021, 86025, 86031, 86035, 86041, 86045, 86051, 86055, 86061 };
				BYTE gradeLevel = 0;
				for (const auto& acceIdx : m_acceItemIdxList)
				{
					if (itemIdx >= acceIdx && itemIdx <= acceIdx + 3)
					{
						gradeLevel = itemIdx - (acceIdx - 1);
						break;
					}
				}
				if (filter->sashType - 1 != gradeLevel)
					return false;
			}

			if (item->alSockets[0] < filter->minAbs || (filter->maxAbs != 0 && item->alSockets[0] > filter->maxAbs))
				return false;
		}
		
	}
	

	if (filter->timeType > 0)
	{
		if (filter->timeType == 1)
		{
			//put perma check method!
		}
		else if (filter->timeType == 2)
		{
			for (int i = 0; i < ITEM_LIMIT_MAX_NUM; i++)
			{
				if (LIMIT_REAL_TIME == table->aLimits[i].bType || LIMIT_TIMER_BASED_ON_WEAR == table->aLimits[i].bType)
					break;
				if (i + 1 == ITEM_LIMIT_MAX_NUM)
					return false;
			}
		}
	}

	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; i++)
	{
		if (LIMIT_LEVEL == table->aLimits[i].bType)
		{
			if ((filter->minLevel != 0 && table->aLimits[i].lValue < filter->minLevel) || (filter->maxLevel != 0 && table->aLimits[i].lValue > filter->maxLevel))
				return false;
			break;
		}
	}

	for (int i = 0; i < 5; ++i)
	{
		if (filter->aAttr[i].bType == 0)
			break;
		bool isFailed = true;
		for (int x = 0; x < 5; ++x)
		{
			if (item->aAttr[x].bType == 0)
				break;
			else if (aApplyInfo[item->aAttr[x].bType].bPointType  == filter->aAttr[i].bType && item->aAttr[x].sValue >= filter->aAttr[i].sValue)
			{
				isFailed = false;
				break;
			}
		}
		if(isFailed)
			return false;
	}
	return true;
}

void COfflineShopManager::RemoveItemTimeDone(const OFFLINE_SHOP_ITEM* item)
{
	LPOFFLINESHOP pkOfflineShop = FindOfflineShopPID(item->owner_id);
	if (!pkOfflineShop)
		return;
	shop_item n;
	n.subheader = REMOVE_ITEM;
	thecore_memcpy(&n.item, item, sizeof(n.item));
	db_clientdesc->DBPacket(HEADER_GD_OFFLINESHOP, 0, &n, sizeof(shop_item));
}

bool COfflineShopManager::IsRealTimeItem(const OFFLINE_SHOP_ITEM* item)
{
	const TItemTable* item_table = ITEM_MANAGER::instance().GetTable(item->vnum);
	if (!item_table)
		return false;

	if (item_table->bType == ITEM_UNIQUE)
	{
		if (item->alSockets[7] > 0)
		{
			switch (item->vnum)
			{
				case 72001:
				case 72002:
				case 72003:
				case 72004:
				case 72006:
				case 70043:
					return false;
			}

			if (get_global_time() >= item->alSockets[0])
				return true;
		}
	}

	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; i++)
	{
		if (LIMIT_REAL_TIME_START_FIRST_USE == item_table->aLimits[i].bType)
		{
			if (item->alSockets[0] > 0)
			{
				if (get_global_time() >= item->alSockets[0])
					return true;
			};
			break;
		}
		else if (LIMIT_REAL_TIME == item_table->aLimits[i].bType)
		{
			if(get_global_time() >= item->alSockets[0])
				return true;
			break;
		}
	}
	return false;
}

void COfflineShopManager::StartSearch(LPCHARACTER ch, const char* szCommand)
{
	if (!ch || strlen(szCommand) <= 4)
		return;
	searchFilter filter;
	CompareFilter(filter, szCommand);

	const BYTE playerLanguage = ch->GetLanguage();
	std::vector<DWORD> m_SearchItems;

	if (filter.playerSearch && filter.searchInput.length())
	{
		const auto it = m_Map_pkOfflineShopByName.find(filter.searchInput.c_str());
		if (it != m_Map_pkOfflineShopByName.end())
		{
			const LPOFFLINESHOP pkOfflineShop = it->second;
			if (!pkOfflineShop->IsClosed())
			{
				for (WORD j = 0; j < OFFLINE_SHOP_HOST_ITEM_MAX_NUM; ++j)
				{
					const OFFLINE_SHOP_ITEM* item = &pkOfflineShop->m_data.items[j];
					if (!CheckFilter(&filter, item, playerLanguage))
						continue;
					m_SearchItems.emplace_back(item->id);
				}
			}
		}
	}
	else
	{
		if(filter.searchInput.length())// if locale name is arabian carefully for make string lower.!
			std::transform(filter.searchInput.begin(), filter.searchInput.end(), filter.searchInput.begin(), [](unsigned char c) { return std::tolower(c); });

		std::map<DWORD, OFFLINE_SHOP_ITEM*>::iterator itStart = filter.searchType > 0 ? m_itemMap.end() : m_itemMap.begin(), itEnd = m_itemMap.end();
		if (filter.searchType > 0)
		{
			const std::pair<int, int> searchPair(filter.searchType , filter.searchSubType);
			auto itCategory = m_itemCategoryMap.find(searchPair);
			if (itCategory != m_itemCategoryMap.end())
			{
				itStart = itCategory->second.begin();
				itEnd = itCategory->second.end();
			}
		}
		for (; itStart != itEnd; ++itStart)
		{
			if (!CheckFilter(&filter, itStart->second, playerLanguage))
				continue;
			m_SearchItems.emplace_back(itStart->second->id);
		}
	}
	ch->SetLookingSearch(m_SearchItems);
	SendPlayerSearch(ch, 1);
}

void COfflineShopManager::SendPlayerSearch(LPCHARACTER ch, int pageIdx)
{
	if (!ch)
		return;
	if (!ch->GetDesc())
		return;

	int totalPageCount = ch->GetTotalPageCount();
	if (totalPageCount < 1)
		totalPageCount = 1;

	if (pageIdx > totalPageCount)
		pageIdx = totalPageCount;

	if (pageIdx < 1)
		pageIdx = 1;

	std::vector<DWORD> m_SearchItems;
	ch->GetPageItems(pageIdx, m_SearchItems);

	const int itemCount = m_SearchItems.size();
	

	TEMP_BUFFER buf;

	TPacketGCShopSearchItemSet pack;
	pack.header = HEADER_GC_SHOPSEARCH_SET;
	pack.size = sizeof(TPacketGCShopSearchItemSet) + sizeof(pageIdx) + sizeof(totalPageCount) + sizeof(itemCount) + (itemCount * sizeof(TOfflineShopItem));

	buf.write(&pack, sizeof(TPacketGCShopSearchItemSet));
	buf.write(&pageIdx, sizeof(pageIdx));
	buf.write(&totalPageCount, sizeof(totalPageCount));
	buf.write(&itemCount, sizeof(itemCount));
	
	for (int i = 0; i < m_SearchItems.size(); ++i)
	{
		const OFFLINE_SHOP_ITEM* item = NULL;

		auto it = m_itemMap.find(m_SearchItems[i]);
		if (it != m_itemMap.end())
			item = it->second;

		TOfflineShopItem p;
		if (item)
		{
			p.id = item->id;
			p.vnum = item->vnum;
			p.price = item->price;
			p.count = item->count;
			p.display_pos = item->pos;
			p.owner_id = item->owner_id;
			p.status = 0;
			thecore_memcpy(&p.alSockets, &item->alSockets, sizeof(p.alSockets));
			thecore_memcpy(&p.aAttr, &item->aAttr, sizeof(p.aAttr));
			strlcpy(p.szBuyerName, item->szOwnerName, sizeof(p.szBuyerName));
			p.ownerStatus = CHARACTER_MANAGER::Instance().FindByPID(item->owner_id) ? true : (P2P_MANAGER::Instance().FindByPID(item->owner_id) ? true : false);
			p.averagePrice = GetAveragePrice(p.vnum);
		}
		else
			memset(&p, 0, sizeof(TOfflineShopItem));
		buf.write(&p, sizeof(TOfflineShopItem));
	}
	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}


std::pair<int, int> COfflineShopManager::GetItemCategory(DWORD itemIdx, BYTE itemType, BYTE itemSubType)
{
	if (itemType == ITEM_WEAPON)
	{
		if (itemSubType == WEAPON_SWORD)
			return { 1, 0 };
		else if (itemSubType == WEAPON_TWO_HANDED)
			return { 1, 1 };
		else if (itemSubType == WEAPON_DAGGER)
			return { 1, 2 };
		else if (itemSubType == WEAPON_BOW)
			return { 1, 3 };
		else if (itemSubType == WEAPON_BELL)
			return { 1, 4 };
		else if (itemSubType == WEAPON_FAN)
			return { 1, 5 };
	}
	else if (itemType == ITEM_BELT)
		return { 2, 6 };
	else if (itemType == ITEM_ARMOR)
	{
		if (itemSubType == ARMOR_BODY)
			return { 2, 0 };
		else if (itemSubType == ARMOR_SHIELD)
			return { 2, 1 };
		else if (itemSubType == ARMOR_EAR)
			return { 2, 2 };
		else if (itemSubType == ARMOR_NECK)
			return { 2, 3 };
		else if (itemSubType == ARMOR_WRIST)
			return { 2, 4 };
		else if (itemSubType == ARMOR_FOOTS)
			return { 2, 5 };
		else if (itemSubType == ARMOR_HEAD)
			return { 2, 7 };
		else if (itemSubType == ARMOR_PENDANT_FIRE)
			return { 2, 8 };
		else if (itemSubType == ARMOR_PENDANT_ICE)
			return { 2, 8 };
		else if (itemSubType == ARMOR_PENDANT_WIND)
			return { 2, 8 };
		else if (itemSubType == ARMOR_PENDANT_THUNDER)
			return { 2, 8 };
		else if (itemSubType == ARMOR_PENDANT_EARTH)
			return { 2, 8 };
		else if (itemSubType == ARMOR_PENDANT_DARK)
			return { 2, 8 };
		else if (itemSubType == ARMOR_PENDANT_SOUL)
			return { 2, 8 };
	}
	else if (itemType == ITEM_DS)
		return { 3, -1 };
	else if (itemType == ITEM_COSTUME)
	{
		if (itemSubType == COSTUME_BODY)
			return { 4, 0 };
		else if (itemSubType == COSTUME_HAIR)
			return { 4, 1 };
		else if (itemSubType == COSTUME_WEAPON)
		{
			const TItemTable* table = ITEM_MANAGER::instance().GetTable(itemIdx);
			if (table)
			{
				const long weaponType = table->alValues[3];
				if(weaponType == WEAPON_SWORD)
					return { 4, 2 };
				else if (weaponType == WEAPON_TWO_HANDED)
					return { 4, 3 };
				else if (weaponType == WEAPON_FAN)
					return { 4, 4 };
				else if (weaponType == WEAPON_BELL)
					return { 4, 5 };
				else if (weaponType == WEAPON_DAGGER)
					return { 4, 6 };
				else if (weaponType == WEAPON_BOW)
					return { 4, 7 };
			}
		}
		else if (itemSubType == COSTUME_ACCE)
			return { 5, 0 };
		else if (itemSubType == COSTUME_ACCE_SKIN)
			return { 5, 1 };
		else if (itemSubType == COSTUME_MOUNT)
			return { 10, 0 };
		else if (itemSubType == COSTUME_MOUNT_SKIN)
			return { 10, 1 };
		else if (itemSubType == COSTUME_PET)
			return { 11, 1 };
		else if (itemSubType == COSTUME_NORMAL_PET_SKIN)
			return { 11, 2 };
	}

	else if (itemType == COSTUME_PET && itemSubType == PET_LEVELABLE)
		return { 11, 0 };

	// reinforcements
	else if (itemIdx >= 401001 && itemIdx <= 403019)
		return { 4, 8 };

	// passive books 
	else if (itemIdx >= 50301 && itemIdx <= 50303)
		return { 7, 1 };

	else if (itemIdx >= 50314 && itemIdx <= 50316)
		return { 7, 1 };

	else if (itemIdx >= 951050 && itemIdx <= 951058)
		return { 7, 1 };

	// pet books
	else if (itemIdx >= 55010 && itemIdx <= 55028)
		return { 7, 2 };

	// normal books
	else if (itemType == ITEM_SKILLBOOK)
		return { 7, 0 };

	switch (itemIdx)
	{
		case 67001:
		case 67002:
		case 67003:
		case 67004:
		case 67005:
		case 67006:
		case 67007:
		case 67008:
		case 67009:
		case 67010:
		case 67011:
		case 39046:
		case 90000:
		case 70070:
		case 55032:
			return { 5, 2 };
		case 51010:
		case 51011:
		case 51012:
		case 51013:
		case 51014:
		case 51015:
		case 51016:
		case 51017:
		case 51018:
		case 51019:
		case 51020:
		case 51021:
		case 51022:
		case 51023:
		case 51024:
		case 51025:
		case 51026:
		case 51027:
		case 51028:
		case 51029:
		case 51030:
			return { 6, 0 };
		case 51003:
		case 51005:
		case 51006:
			return { 6, 1 };
		case 50821:
		case 50822:
		case 50823:
		case 50824:
		case 50825:
		case 50826:
		case 950821:
		case 950822:
		case 950823:
		case 950824:
		case 950825:
		case 950826:
			return { 8, 0 };
		case 39017:
		case 39018:
		case 39019:
		case 39020:
		case 39024:
		case 39025:
		case 939017:
		case 939018:
		case 939019:
		case 939020:
		case 939024:
		case 939025:	
		case 50817:
		case 50818:
		case 95219:
		case 95220:
		case 50830:
		case 55621:
		case 50621:
		case 18900:
		case 18901:
			return { 8, 2 };
		case 50623:
		case 50624:
		case 50625:
		case 50626:
		case 50627:
		case 50628:
		case 50629:
		case 50630:
		case 50631:
		case 50632:
		case 50633:
		case 50634:
		case 50635:
		case 50636:
		case 50637:
		case 50638:
		case 50639:
		case 50640:
		case 55623:
		case 55624:
		case 55625:
		case 55626:
		case 55627:
		case 55628:
		case 55629:
		case 55630:
		case 55631:
		case 55632:
		case 55633:
		case 55634:
		case 55635:
		case 55636:
		case 55637:
		case 55638:
		case 55639:
		case 55640:
			return { 8, 3 };
		case 27802:
		case 27863:
		case 27864:
		case 27865:
		case 27866:
		case 27867:
		case 27868:
		case 27870:
		case 27871:
		case 27872:
		case 27873:
		case 27874:
		case 27875:
		case 27878:
		case 27883:
			return { 8, 1 };
		case 351220:
		case 71032:
		case 70039:
		case 25041:
		case 25042:
		case 25043:
		case 72346:
		case 72351:
		case 351210:
		case 351211:
			return { 9, 1 };
		case 30120:
		case 30179:
		case 30319:
		case 30320:
		case 30324:
		case 30613:
		case 31181:
		case 70448:
		case 70449:
		case 71095:
		case 71174:
		case 72328:
			return { 9, 2 };
		case 71035:
		case 63019:
		case 30006:
		case 30015:
		case 30047:
		case 30050:
		case 30165:
		case 30166:
		case 30167:
		case 30168:
		case 30251:
		case 30252:
		case 70028:
			return { 9, 3 };
		case 94144:
		case 94146:
		case 94148:
			return { 11, 3 };
		case 94145:
		case 94147:
		case 94149:
			return { 10, 2 };
		case 951063:
		case 71073:
		case 71071:
		case 71070:
		case 71072:
			return { 12, 0 };
		case 83011:
		case 83012:
		case 83015:
		case 83008:
		case 83014:
		case 83009:
			return { 12, 1};
		case 501008:
		case 501005:
		case 501002:
		case 506051:
		case 501007:
		case 501004:
		case 501001:
		case 506050:
		case 506053:
		case 506054:
		case 952007:
		case 952006:
		case 503012:
			return { 12, 2 };
	}

	if (itemType == ITEM_GIFTBOX)
		return { 9, 0 };

	return { 0, 0 };
}

void COfflineShopManager::ClearItem(DWORD id)
{
	const auto it = m_itemMap.find(id);
	if (it != m_itemMap.end())
	{
		const TItemTable* table = ITEM_MANAGER::instance().GetTable(it->second->vnum);
		if (table)
		{
			const std::pair<int, int> itemCategory = GetItemCategory(table->dwVnum, table->bType, table->bSubType);
			const auto itItemMap = m_itemCategoryMap.find(itemCategory);
			if (itItemMap != m_itemCategoryMap.end())
			{
				if (itItemMap->second.find(id) != itItemMap->second.end())
					itItemMap->second.erase(id);
			}
		}
		M2_DELETE(it->second);
		m_itemMap.erase(id);
		const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
		if (c_ref_set.size())
		{
			for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				auto desc = *it;
				if (desc)
				{
					LPCHARACTER ch = desc->GetCharacter();
					if (ch)
						ch->IsLookingSearchItem(id, true, true);
				}
			}
		}
	}
}

void COfflineShopManager::InsertItem(OFFLINE_SHOP_ITEM* p)
{
	if (p->vnum == 0 || p->status != 0)
		return;

	const auto it = m_itemMap.find(p->id);
	if (it != m_itemMap.end()){
		sys_err("wtf have 2 id in game? item id %u",p->id);
		return;
	}
	SetAveragePrice(p->vnum, p->price / p->count);

	OFFLINE_SHOP_ITEM* item = new OFFLINE_SHOP_ITEM;
	thecore_memcpy(item, p, sizeof(OFFLINE_SHOP_ITEM));

	const TItemTable* table = ITEM_MANAGER::instance().GetTable(p->vnum);
	if (table)
	{
		const std::pair<int, int> itemCategory = GetItemCategory(table->dwVnum, table->bType, table->bSubType);
		if (itemCategory.first != 0)
		{
			auto itItemMap = m_itemCategoryMap.find(itemCategory);
			if (itItemMap != m_itemCategoryMap.end())
			{
				if (itItemMap->second.find(p->id) == itItemMap->second.end())
					itItemMap->second.emplace(p->id, item);
			}
			else
			{
				std::map<DWORD, OFFLINE_SHOP_ITEM*> m_map;
				m_map.emplace(p->id, item);
				m_itemCategoryMap.emplace(itemCategory, m_map);
			}
		}
	}
	m_itemMap.insert(std::make_pair(p->id, item));
}
#endif
#endif
