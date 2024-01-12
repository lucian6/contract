#include "stdafx.h"

#if defined(__GEM_MARKET_SYSTEM__)
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "item.h"
#include "item_manager.h"
#include "locale_service.h"
#include "questmanager.h"

const char* txt_gem[] = {
	"[Gaya] You do not have enough Gaya.",
	"[Gaya] An error occurred.",
	"[Gaya] You need %d of %s.",
	"[Gaya] You do not have enough Yang.",
	"[Gaya] Amount of Gaya: %d.",
	"[Gaya] Cutting successful.",
	"[Gaya] Cutting failed."
};

void CHARACTER::LoadGemSystem()
{
	FILE* fp;
	char one_line[256];
	int value1, value2, value3;
	const char* delim = " \t\r\n";
	char* v, * token_string;
	char file_name[256 + 1];

	Gem_Load_Values gem_values = { 0, 0, 0 };

	snprintf(file_name, sizeof(file_name), "%s/gem.txt", LocaleService_GetBasePath().c_str());
	fp = fopen(file_name, "r");

	while (fgets(one_line, 256, fp))
	{
		value1 = value2 = value3 = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delim);
		if (NULL == token_string)
			continue;

		if ((v = strtok(NULL, delim)))
			str_to_number(value1, v);

		if ((v = strtok(NULL, delim)))
			str_to_number(value2, v);

		if ((v = strtok(NULL, delim)))
			str_to_number(value3, v);

		TOKEN("ITEM")
		{
			gem_values.items = value1;
			gem_values.gem = value2;
			gem_values.count = value3;
			load_gem_items.push_back(gem_values);
		}
		else TOKEN("GLIMMERSTONE") { load_gem_values.glimmerstone = value1; }
		else TOKEN("GEM_EXPANSION") { load_gem_values.gem_expansion = value1; }
		else TOKEN("GEM_REFRESH") { load_gem_values.gem_refresh = value1; }
		else TOKEN("GLIMMERSTONE_COUNT") { load_gem_values.glimmerstone_count = value1; }
		else TOKEN("GRADE_STONE") { load_gem_values.grade_stone = value1; }
		else TOKEN("GIVE_GEM") { load_gem_values.give_gem = value1; }
		else TOKEN("PROB_GEM") { load_gem_values.prob_gem = value1; }
		else TOKEN("COST_GEM_YANG") { load_gem_values.cost_gem_yang = value1; }
		else TOKEN("GEM_EXPANSION_COUNT") { load_gem_values.gem_expansion_count = value1; }
		else TOKEN("GEM_REFRESH_COUNT") { load_gem_values.gem_refresh_count = value1; }
	}

	fclose(fp);
}

bool CHARACTER::CheckItemsFull()
{
	FILE* fp;
	char file_name[256 + 1];

	snprintf(file_name, sizeof(file_name), "%s/gem/%s_gem_info.txt", LocaleService_GetBasePath().c_str(), GetName());

	if ((fp = fopen(file_name, "r")) == 0)
		return false;
	else
		return true;
}

void CHARACTER::ClearGemMarket()
{
	info_items.clear();
	info_slots.clear();
}

void CHARACTER::InfoGemMarker()
{
	ClearGemMarket();
	UpdateItemsGemMarker0();
	ChatPacket(CHAT_TYPE_COMMAND, "GemMarketClear");

	if (info_items.empty())
		return;

	for (int i = 0; i < info_items.size(); ++i)
	{
		ChatPacket(CHAT_TYPE_COMMAND, "GemMarketItems %d %d %d",
			info_items[i].value_1,
			info_items[i].value_2,
			info_items[i].value_3);
	}
}

bool CHARACTER::CheckSlotGemMarket(int slot)
{
	return true;
}

void CHARACTER::BuyItemsGemMarket(int slot)
{
	if (GetGem() >= info_items[slot].value_2)
	{
		PointChange(POINT_GEM, -info_items[slot].value_2);
		AutoGiveItem(info_items[slot].value_1, info_items[slot].value_3, -1, true);
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, txt_gem[0]);
		return;
	}
}

int myrandom(int i) { return std::rand() % i; }

void CHARACTER::RefreshItemsGemMarket()
{
	FILE* fileID;
	char file_name[256 + 1];

	snprintf(file_name, sizeof(file_name), "%s/gem/%s_gem_info.txt", LocaleService_GetBasePath().c_str(), GetName());
	fileID = fopen(file_name, "w");

	std::vector <int> itemID;
	itemID.clear();

	for (int i = 0; i < 25; i++)
	{
		bool check;
		int itemIndex;

		do
		{
			itemIndex = number(1, load_gem_items.size() - 1);
			check = true;

			for (int j = 0; j < i; j++)
			{
				if (std::find(itemID.begin(), itemID.end(), load_gem_items[itemIndex].items) != itemID.end())
				{
					check = false;
					break;
				}
			}
		} while (!check);

		itemID.push_back(load_gem_items[itemIndex].items);

		fprintf(fileID, "Item	%d	%d	%d\n",
			load_gem_items[itemIndex].items,
			load_gem_items[itemIndex].gem,
			load_gem_items[itemIndex].count);
	}
	fclose(fileID);
}

void CHARACTER::UpdateSlotGemMarket(int slot)
{
	FILE* fileID;
	char file_name[256 + 1];

	snprintf(file_name, sizeof(file_name), "%s/gem/%s_gem_info.txt", LocaleService_GetBasePath().c_str(), GetName());
	fileID = fopen(file_name, "w");

	for (int i = 0; i < 25; ++i)
	{
		fprintf(fileID, "Item	%d	%d	%d\n",
			info_items[i].value_1,
			info_items[i].value_2,
			info_items[i].value_3);
	}

	if (slot == 3)
		info_slots[0].value_1 = 1;
	else if (slot == 4)
		info_slots[0].value_2 = 1;
	else if (slot == 5)
		info_slots[0].value_3 = 1;
	else if (slot == 6)
		info_slots[0].value_4 = 1;
	else if (slot == 7)
		info_slots[0].value_5 = 1;
	else if (slot == 8)
		info_slots[0].value_6 = 1;

	fclose(fileID);
	InfoGemMarker();
}

void CHARACTER::UpdateItemsGemMarker0()
{
	FILE* fp;
	char one_line[256];
	int value1, value2, value3, value4, value5, value6;
	const char* delim = " \t\r\n";
	char* v, * token_string;
	char file_name[256 + 1];

	Gem_Shop_Values market_gem_values_0 = { 0, 0, 0 };

	snprintf(file_name, sizeof(file_name), "%s/gem/%s_gem_info.txt", LocaleService_GetBasePath().c_str(), GetName());
	fp = fopen(file_name, "r");

	while (fgets(one_line, 256, fp))
	{
		value1 = value2 = value3 = value4 = value5 = value6 = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delim);
		if (NULL == token_string)
			continue;

		if ((v = strtok(NULL, delim)))
			str_to_number(value1, v);

		if ((v = strtok(NULL, delim)))
			str_to_number(value2, v);

		if ((v = strtok(NULL, delim)))
			str_to_number(value3, v);

		if ((v = strtok(NULL, delim)))
			str_to_number(value4, v);

		if ((v = strtok(NULL, delim)))
			str_to_number(value5, v);

		if ((v = strtok(NULL, delim)))
			str_to_number(value6, v);

		TOKEN("Item")
		{
			market_gem_values_0.value_1 = value1;
			market_gem_values_0.value_2 = value2;
			market_gem_values_0.value_3 = value3;
			info_items.push_back(market_gem_values_0);
		}
	}

	fclose(fp);
}

void CHARACTER::UpdateItemsGemMarker()
{
	FILE* fileID;
	char file_name[256 + 1];

	snprintf(file_name, sizeof(file_name), "%s/gem/%s_gem_info.txt", LocaleService_GetBasePath().c_str(), GetName());
	fileID = fopen(file_name, "a");

	std::vector <int> itemID;
	itemID.clear();

	for (int i = 0; i < 25; ++i)
	{
		bool check;
		int itemIndex;

		do
		{
			itemIndex = number(1, load_gem_items.size() - 1);
			check = true;

			for (int j = 0; j < i; j++)
			{
				if (std::find(itemID.begin(), itemID.end(), load_gem_items[itemIndex].items) != itemID.end())
				{
					check = false;
					break;
				}
			}
		} while (!check);

		itemID.push_back(load_gem_items[itemIndex].items);

		fprintf(fileID, "Item	%d	%d	%d\n",
			load_gem_items[itemIndex].items,
			load_gem_items[itemIndex].gem,
			load_gem_items[itemIndex].count);
	}

	fclose(fileID);
}

void CHARACTER::CraftGemItems(int slot)
{
	LPITEM item = GetItem(TItemPos(INVENTORY, slot));

	int ID_Glimmerstone = load_gem_values.glimmerstone;
	int Count_Glimmerstone = load_gem_values.glimmerstone_count;
	int Grade_Stone = load_gem_values.grade_stone;
	int gem_points = load_gem_values.give_gem;
	int Cost_Gem_Yang = load_gem_values.cost_gem_yang;

	LPITEM item_glimmerstone = ITEM_MANAGER::instance().CreateItem(ID_Glimmerstone, Count_Glimmerstone, 0, true);

	if (NULL == item) //Null item
	{
		return;
	}

	if (item->GetType() != ITEM_METIN || item->GetRefineLevel() > Grade_Stone)
	{
		ChatPacket(CHAT_TYPE_INFO, txt_gem[1]);
		return;
	}

	if (CountSpecifyItem(ID_Glimmerstone) < Count_Glimmerstone)
	{
		ChatPacket(CHAT_TYPE_INFO, txt_gem[2], Count_Glimmerstone, item_glimmerstone->GetLocaleName(GetLanguage()));
		return;
	}

	if (GetGold() < Cost_Gem_Yang)
	{
		ChatPacket(CHAT_TYPE_INFO, txt_gem[3]);
		return;
	}

	PointChange(POINT_GEM, gem_points);
	ChatPacket(CHAT_TYPE_INFO, txt_gem[4], gem_points);
	ChatPacket(CHAT_TYPE_INFO, txt_gem[5]);


	ChatPacket(CHAT_TYPE_COMMAND, "GemCheck");
	RemoveSpecifyItem(ID_Glimmerstone, Count_Glimmerstone);
	PointChange(POINT_GOLD, -Cost_Gem_Yang);

	item->SetCount(item->GetCount() - 1);
}

void CHARACTER::MarketGemItems(int slot)
{
	if (CheckItemsFull() == false)
	{
		ChatPacket(CHAT_TYPE_INFO, txt_gem[1]);
		return;
	}

	if (slot > 24)
	{
		ChatPacket(CHAT_TYPE_INFO, txt_gem[1]);
		return;
	}

	int ID_GemMarketExpansion = load_gem_values.gem_expansion;
	LPITEM item_gemrexpansion = ITEM_MANAGER::instance().CreateItem(ID_GemMarketExpansion, load_gem_values.gem_expansion_count, 0, true);

	if (slot >= 3)
	{
		if (CheckSlotGemMarket(slot) == false)
		{
			if (CountSpecifyItem(ID_GemMarketExpansion) >= load_gem_values.gem_expansion_count)
			{
				RemoveSpecifyItem(ID_GemMarketExpansion, load_gem_values.gem_expansion_count);
				UpdateSlotGemMarket(slot);
				return;
			}
			else
			{
				ChatPacket(CHAT_TYPE_INFO, txt_gem[2], load_gem_values.gem_expansion_count, item_gemrexpansion->GetLocaleName(GetLanguage()));
				return;
			}
		}
		else
		{
			BuyItemsGemMarket(slot);
			return;
		}
	}
	else
	{
		BuyItemsGemMarket(slot);
		return;
	}
	return;
}

void CHARACTER::RefreshGemItems()
{
	if (CheckItemsFull() == false)
	{
		ChatPacket(CHAT_TYPE_INFO, txt_gem[1]);
		return;
	}

	int ID_GemMarketRefresh = load_gem_values.gem_refresh;
	LPITEM item_gemrefresh = ITEM_MANAGER::instance().CreateItem(ID_GemMarketRefresh, load_gem_values.gem_refresh_count, 0, true);

	if (CountSpecifyItem(ID_GemMarketRefresh) < load_gem_values.gem_refresh_count)
	{
		ChatPacket(CHAT_TYPE_INFO, txt_gem[2], load_gem_values.gem_refresh_count, item_gemrefresh->GetLocaleName(GetLanguage()));
		return;
	}

	RemoveSpecifyItem(ID_GemMarketRefresh, load_gem_values.gem_refresh_count);
	RefreshItemsGemMarket();
	InfoGemMarker();
}

int CHARACTER::GetGemState(const std::string& state) const
{
	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());
	return pPC->GetFlag(state);

	//return quest::CQuestManager::instance().GetEventFlag(state);
}

void CHARACTER::SetGemState(const std::string& state, int szValue)
{
	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());
	return pPC->SetFlag(state, szValue);

	//return quest::CQuestManager::instance().SetEventFlag(state, szValue);
}

#endif
