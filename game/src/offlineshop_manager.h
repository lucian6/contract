#pragma once
#ifdef ENABLE_OFFLINESHOP_SYSTEM

class CItemAverage
{
public:
	CItemAverage(long long price) :currentAveragePrice(0) {
		memset(&examplePrices, 0, sizeof(examplePrices));
		AddItem(price);
	}
	long long GetAveragePrice() { return currentAveragePrice; }
	void AddItem(long long price)
	{
		if (price < 0)
			return;
		unsigned long long totalPrice = 0;
		if (examplePrices[0] == 0 || examplePrices[1] == 0 || examplePrices[2] == 0)
		{
			std::vector<long long> m_list;
			m_list.emplace_back(price);
			for (BYTE j = 0; j < 3; ++j)
				if (examplePrices[j] != 0)
					m_list.emplace_back(examplePrices[j]);
			if(m_list.size() > 1)
				std::sort(m_list.begin(), m_list.end());
			for (BYTE j = 0; j < m_list.size(); ++j)
			{
				examplePrices[j] = m_list[j];
				totalPrice += m_list[j];
			}
		}
		else
		{
			if (price < examplePrices[2] && price < examplePrices[1] && price < examplePrices[0])
				examplePrices[0] = price;
			else if (price > examplePrices[0] && price < examplePrices[2] && price > examplePrices[1])
				examplePrices[1] = price;
			else if (price > examplePrices[0] && price > examplePrices[1] && price > examplePrices[2])
				examplePrices[2] = price;
			for (BYTE j = 0; j < _countof(examplePrices); ++j)
				totalPrice += examplePrices[j];
		}
		//sys_err("0: %lld 1: %lld 2: %lld", examplePrices[0], examplePrices[1], examplePrices[2]);
		currentAveragePrice = totalPrice / 3;
	}
protected:
	long long	examplePrices[3];// low - normal - high
	long long	currentAveragePrice;
};

struct searchFilter
{
	int searchType, searchSubType;
	bool exactSearch, playerSearch;
	std::string searchInput;
	bool race[JOB_MAX_NUM], gender[2];
	BYTE minRefine, maxRefine, alchemyClass, alchemyLevel, costumeType, costumeRarity, sashType, timeType;
	WORD minCount, maxCount, minAverage, minSkill, minAbs, maxAbs, minLevel, maxLevel;
	TPlayerItemAttribute aAttr[5];
	searchFilter() : searchType(-1), searchSubType(-1), searchInput(""), minRefine(0), maxRefine(0), alchemyClass(0), alchemyLevel(0), costumeType(0), costumeRarity(0), sashType(0), timeType(0)
		, minCount(0), maxCount(0), minAverage(0), minSkill(0), minAbs(0), maxAbs(0), minLevel(0), maxLevel(0)
	{
		memset(&race, false, sizeof(race));
		memset(&gender, false, sizeof(gender));
		memset(&aAttr, 0, sizeof(aAttr));
	}

};


class COfflineShopManager : public singleton<COfflineShopManager>
{
	public:
		COfflineShopManager();
		~COfflineShopManager();

		bool			IsRealTimeItem(const OFFLINE_SHOP_ITEM* item);
		void			RemoveItemTimeDone(const OFFLINE_SHOP_ITEM* item);

		LPOFFLINESHOP	FindOfflineShopPID(DWORD pid);

		//void			CompareOffShopEventTime();
		void			OpenMyOfflineShop(LPCHARACTER ch, const char* c_pszSign, TOfflineShopItemTable* pTable, BYTE bItemCount, DWORD shopVnum, BYTE titleType);
		void			CreateOfflineShop(TOfflineShop* offlineshop);
		void			StopShopping(LPCHARACTER ch);
		void			OpenOfflineShop(LPCHARACTER ch);
		void			OpenOfflineShopWithVID(LPCHARACTER ch, DWORD vid);
		bool			HasOfflineShop(LPCHARACTER ch);

		void			AddItemShortcut(LPCHARACTER ch, TItemPos pos, long long itemPrice);

		void			SetAveragePrice(DWORD itemIdx, long long itemPrice);
		long long		GetAveragePrice(DWORD itemIdx);
		void			CheckAveragePrice(LPCHARACTER ch, DWORD itemIdx);

		BYTE			GetItemPos(DWORD ownerID, DWORD itemID);

		void			Buy(LPCHARACTER ch, DWORD vid, BYTE bPos, DWORD itemID);
		void			BuyItemReal(TOfflineShopBuy* item);

		void			AddItem(LPCHARACTER ch, BYTE bDisplayPos, TItemPos bPos, long long iPrice);
		void			AddItemReal(OFFLINE_SHOP_ITEM* item);
		
		void			OpenSlot(LPCHARACTER ch, BYTE bPos);
		void			OpenSlotReal(TOfflineShopOpenSlot* ch);

		void			RemoveItem(LPCHARACTER ch, BYTE bPos, DWORD itemID, BYTE bTakeAll);
		void			RemoveItemReal(OFFLINE_SHOP_ITEM* item);
		
		void			ShopLogRemove(LPCHARACTER ch);
		void			ShopLogRemoveReal(DWORD ch);

		void			ChangeDecoration(LPCHARACTER ch, TShopDecoration* data);
		void			ChangeDecorationReal(TShopDecoration* ch);

		void			WithdrawMoney(LPCHARACTER ch);
		void			WithdrawMoneyReal(DWORD ch);

		void			DestroyOfflineShop(LPCHARACTER ch);
		void			DestroyOfflineShopReal(DWORD ch);

		void			ChangeTitle(LPCHARACTER ch, const char* title);
		void			ChangeTitleReal(TOfflineShopChangeTitle* p);

		void			CloseOfflineShopForTime(LPOFFLINESHOP offlineshop);
		void			CloseOfflineShopForTimeReal(DWORD offlineshop);

		void			GetBackItem(LPCHARACTER ch);
		void			GetBackItemReal(TOfflineShopBackItem* ch);

		void			ShopAddTime(LPCHARACTER ch);
		void			ShopAddTimeReal(DWORD ch);
		void			ShopAddTimeRealPremium(DWORD ch);

		void			RecvPackets(const char * data);

		void			Teleport(LPCHARACTER ch, const char* szOwnerName);
		
		void			RemoveItemAll(LPCHARACTER ch);
		void			RemoveItemAllReal(DWORD ownerID);

		void			MoveItem(LPCHARACTER ch, WORD slotPos, WORD targetPos);
		void			MoveItemReal(DWORD ownerID, WORD slotPos, WORD targetPos);

		void			ChangePrice(LPCHARACTER ch, BYTE bPos, DWORD itemID, long long itemPrice, BYTE bAllItem);
		void			ChangePriceReal(DWORD ownerID, BYTE bPos, long long itemPrice);
#ifdef ENABLE_SHOP_SEARCH_SYSTEM
		void			LoadSearchLanguage();
		bool			CompareNames(const std::string searchedInput, DWORD itemIdx, bool exactSearch, BYTE playerLanguage);

		void			ClearItem(DWORD id);
		void			InsertItem(OFFLINE_SHOP_ITEM* p);

		void			SendPlayerSearch(LPCHARACTER ch, int pageIdx);

		void			StartSearch(LPCHARACTER ch, const char* szCommand);
		void			CompareFilter(searchFilter& filter, const char* szCommand);
		bool			CheckFilter(const searchFilter* filter, const OFFLINE_SHOP_ITEM* item, const BYTE playerLang);

		std::pair<int, int> GetItemCategory(DWORD itemIdx, BYTE itemType, BYTE itemSubType);
#endif

		std::vector<DWORD> 					m_Map_pkShopTimes;

	private:
		LPEVENT								m_pShopTimeEvent;
		std::map<DWORD, CItemAverage> m_pShopItemAveragePrice;
		std::vector<DWORD> 					m_Map_pkOfflineShopCache;
		std::map<DWORD, COfflineShop*> 		m_Map_pkOfflineShopByNPC;
		std::map<std::string, COfflineShop*> 		m_Map_pkOfflineShopByName;
#ifdef ENABLE_SHOP_SEARCH_SYSTEM
		std::map<DWORD, OFFLINE_SHOP_ITEM*>	m_itemMap;
		std::map<std::pair<int, int>, std::map<DWORD, OFFLINE_SHOP_ITEM*>> m_itemCategoryMap;
		

		std::map<BYTE, std::map<DWORD,std::string>>	m_mapItemNames;
#endif
};
#endif

