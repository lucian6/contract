#include "stdafx.h"

#include <stack>

#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "skill.h"
#include "start_position.h"
#include "mob_manager.h"
#include "db.h"
#include "log.h"
#include "vector.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "fishing.h"
#include "party.h"
#include "dungeon.h"
#include "refine.h"
#include "unique_item.h"
#include "war_map.h"
#include "xmas_event.h"
#include "marriage.h"
#include "monarch.h"
#include "polymorph.h"
#include "blend_item.h"
#include "castle.h"
#include "arena.h"
#include "threeway_war.h"

#include "safebox.h"
#include "shop.h"

#ifdef ENABLE_NEWSTUFF
#include "pvp.h"
#include "../../common/PulseManager.h"
#endif

#include "../../common/VnumHelper.h"
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"
#include "../../common/CommonDefines.h"
#include "PetSystem.h"

#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
#include "PetSystem.h"
#endif
#ifdef __BUFFI_SUPPORT__
#include "buffi.h"
#endif
#ifdef __MAINTENANCE__
#include "maintenance.h"
#endif

#define ENABLE_EFFECT_EXTRAPOT
#define ENABLE_BOOKS_STACKFIX
#define ENABLE_ITEM_RARE_ATTR_LEVEL_PCT

enum {ITEM_BROKEN_METIN_VNUM = 28960};

// CHANGE_ITEM_ATTRIBUTES
const char CHARACTER::msc_szLastChangeItemAttrFlag[] = "Item.LastChangeItemAttr";
// END_OF_CHANGE_ITEM_ATTRIBUTES
const BYTE g_aBuffOnAttrPoints[] = { POINT_ENERGY, POINT_COSTUME_ATTR_BONUS };

struct FFindStone
{
	std::map<DWORD, LPCHARACTER> m_mapStone;

	void operator()(LPENTITY pEnt)
	{
		if (pEnt->IsType(ENTITY_CHARACTER) == true)
		{
			LPCHARACTER pChar = (LPCHARACTER)pEnt;

			if (pChar->IsStone() == true)
			{
				m_mapStone[(DWORD)pChar->GetVID()] = pChar;
			}
		}
	}
};

static bool IS_MONKEY_DUNGEON(int map_index)
{
	switch (map_index)
	{
		case 5:
		case 25:
		case 45:
		case 108:
		case 109:
			return true;;
	}

	return false;
}

bool IS_SUMMONABLE_ZONE(int map_index)
{
	if (IS_MONKEY_DUNGEON(map_index))
		return false;
	if (IS_CASTLE_MAP(map_index))
		return false;

	switch (map_index)
	{
		case 66 :
		case 71 :
		case 72 :
		case 73 :
		case 193 :
#if 0
		case 184 :
		case 185 :
		case 186 :
		case 187 :
		case 188 :
		case 189 :
#endif

		case 216 :
		case 217 :
		case 208 :

		case 113 :
			return false;
	}

	if (map_index > 10000) return false;

	return true;
}

bool IS_BOTARYABLE_ZONE(int nMapIndex)
{
	if (!g_bEnableBootaryCheck) return true;

	switch (nMapIndex)
	{
		case 1 :
		case 3 :
		case 21 :
		case 23 :
		case 41 :
		case 43 :
			return true;
	}

	return false;
}

static bool FN_check_item_socket(LPITEM item)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		if (item->GetSocket(i) != item->GetProto()->alSockets[i])
			return false;
	}

	return true;
}

static void FN_copy_item_socket(LPITEM dest, LPITEM src)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		dest->SetSocket(i, src->GetSocket(i));
	}
}
static bool FN_check_item_sex(LPCHARACTER ch, LPITEM item)
{
#ifdef __BUFFI_SUPPORT__
	if ((ch->IsLookingBuffiPage() && item->GetType() == ITEM_COSTUME && (item->GetSubType() == COSTUME_BODY || item->GetSubType() == COSTUME_HAIR)))
	{
		if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_FEMALE))
			return false;
		else
			return true;
	}
#endif

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_MALE))
	{
		if (SEX_MALE==GET_SEX(ch))
			return false;
	}

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_FEMALE))
	{
		if (SEX_FEMALE==GET_SEX(ch))
			return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// ITEM HANDLING
/////////////////////////////////////////////////////////////////////////////
bool CHARACTER::CanHandleItem(bool bSkipCheckRefine, bool bSkipObserver)
{
	if (!bSkipObserver)
		if (m_bIsObserver)
			return false;

#ifdef __MAINTENANCE__
	if (CMaintenanceManager::Instance().GetGameMode() == GAME_MODE_LOBBY && !IsGM() && !this->GetDesc()->IsTester())
		return false;
#endif

	if (GetMyShop())
		return false;

	if (!bSkipCheckRefine)
		if (m_bUnderRefine)
			return false;

	if (IsCubeOpen() || NULL != DragonSoul_RefineWindow_GetOpener())
		return false;

	if (IsWarping())
		return false;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if ((m_bAcceCombination) || (m_bAcceAbsorption))
		return false;
#endif

	if (Is67AttrOpen())
		return false;

	return true;
}

#if defined(__SPECIAL_INVENTORY_SYSTEM__)
LPITEM CHARACTER::GetSkillBookInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}

LPITEM CHARACTER::GetUpgradeItemsInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}

LPITEM CHARACTER::GetStoneInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}

LPITEM CHARACTER::GetGiftBoxInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}

LPITEM CHARACTER::GetSwitchInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}

LPITEM CHARACTER::GetCostumeInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}
#endif

LPITEM CHARACTER::GetInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}

LPITEM CHARACTER::GetItem(TItemPos Cell) const
{
	if (!m_PlayerSlots)
		return nullptr;

	if (!IsValidItemPosition(Cell))
		return NULL;

	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	switch (window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX) 
		{
			sys_err("CHARACTER::GetInventoryItem: invalid item cell %d", wCell);
			return NULL;
		}
		return m_PlayerSlots->pItems[wCell];
	case DRAGON_SOUL_INVENTORY:
		if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid DS item cell %d", wCell);
			return NULL;
		}
		return m_PlayerSlots->pDSItems[wCell];
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid switchbot item cell %d", wCell);
			return NULL;
		}
		return m_PlayerSlots->pSwitchbotItems[wCell];
#endif
	default:
		return NULL;
	}
	return NULL;
}

#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
void CHARACTER::SetItem(TItemPos Cell, LPITEM pItem, bool bHighlight)
#else
void CHARACTER::SetItem(TItemPos Cell, LPITEM pItem)
#endif
{
	if (!m_PlayerSlots)
		return;

	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	if ((unsigned long)((CItem*)pItem) == 0xff || (unsigned long)((CItem*)pItem) == 0xffffffff)
	{
		sys_err("!!! FATAL ERROR !!! item == 0xff (char: %s cell: %u)", GetName(), wCell);
		core_dump();
		return;
	}

	if (pItem && pItem->GetOwner())
	{
		assert(!"GetOwner exist");
		return;
	}

	switch(window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		{
			if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
			{
				sys_err("CHARACTER::SetItem: invalid item cell %d", wCell);
				return;
			}

			LPITEM pOld = m_PlayerSlots->pItems[wCell];

			if (pOld)
			{
				if (wCell < INVENTORY_AND_EQUIP_SLOT_MAX)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= INVENTORY_AND_EQUIP_SLOT_MAX)
							continue;

						if (m_PlayerSlots->pItems[p] && m_PlayerSlots->pItems[p] != pOld)
							continue;

						m_PlayerSlots->bItemGrid[p] = 0;
					}
				}
				else
					m_PlayerSlots->bItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell < INVENTORY_AND_EQUIP_SLOT_MAX)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= INVENTORY_AND_EQUIP_SLOT_MAX)
							continue;

						m_PlayerSlots->bItemGrid[p] = wCell + 1;
					}
				}
				else
					m_PlayerSlots->bItemGrid[wCell] = wCell + 1;
			}

			m_PlayerSlots->pItems[wCell] = pItem;
		}
		break;

	case DRAGON_SOUL_INVENTORY:
		{
			LPITEM pOld = m_PlayerSlots->pDSItems[wCell];

			if (pOld)
			{
				if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						if (m_PlayerSlots->pDSItems[p] && m_PlayerSlots->pDSItems[p] != pOld)
							continue;

						m_PlayerSlots->wDSItemGrid[p] = 0;
					}
				}
				else
					m_PlayerSlots->wDSItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid DS item cell %d", wCell);
					return;
				}

				if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						m_PlayerSlots->wDSItemGrid[p] = wCell + 1;
					}
				}
				else
					m_PlayerSlots->wDSItemGrid[wCell] = wCell + 1;
			}

			m_PlayerSlots->pDSItems[wCell] = pItem;
		}
		break;
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		{
			LPITEM pOld = m_PlayerSlots->pSwitchbotItems[wCell];
			if (pItem && pOld)
			{
				return;
			}

			if (wCell >= SWITCHBOT_SLOT_COUNT)
			{
				sys_err("CHARACTER::SetItem: invalid switchbot item cell %d", wCell);
				return;
			}

			if (pItem)
			{
				CSwitchbotManager::Instance().RegisterItem(GetPlayerID(), pItem->GetID(), wCell);
			}
			else
			{
				CSwitchbotManager::Instance().UnregisterItem(GetPlayerID(), wCell);
			}

			m_PlayerSlots->pSwitchbotItems[wCell] = pItem;
		}
		break;
#endif
	default:
		sys_err ("Invalid Inventory type %d", window_type);
		return;
	}

	if (GetDesc())
	{
		if (pItem)
		{
			TPacketGCItemSet pack;
			pack.header = HEADER_GC_ITEM_SET;
			pack.Cell = Cell;

			pack.count = pItem->GetCount();
			pack.vnum = pItem->GetVnum();
			pack.flags = pItem->GetFlag();
			pack.anti_flags	= pItem->GetAntiFlag();
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			pack.highlight = bHighlight;
#else
			pack.highlight = (Cell.window_type == DRAGON_SOUL_INVENTORY);
#endif

			thecore_memcpy(pack.alSockets, pItem->GetSockets(), sizeof(pack.alSockets));
			thecore_memcpy(pack.aAttr, pItem->GetAttributes(), sizeof(pack.aAttr));

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemSet));
		}
		else
		{
			TPacketGCItemDelDeprecated pack;
			pack.header = HEADER_GC_ITEM_DEL;
			pack.Cell = Cell;
			pack.count = 0;
			pack.vnum = 0;
			memset(pack.alSockets, 0, sizeof(pack.alSockets));
			memset(pack.aAttr, 0, sizeof(pack.aAttr));

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemDelDeprecated));
		}
	}

	if (pItem)
	{
		pItem->SetCell(this, wCell);
		switch (window_type)
		{
		case INVENTORY:
		case EQUIPMENT:
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
			if ((wCell < Inventory_Size()) || 
			(SKILL_BOOK_INVENTORY_SLOT_START <= wCell && SKILL_BOOK_INVENTORY_SLOT_END > wCell) || 
			(UPGRADE_ITEMS_INVENTORY_SLOT_START <= wCell && UPGRADE_ITEMS_INVENTORY_SLOT_END > wCell) || 
			(STONE_INVENTORY_SLOT_START <= wCell && STONE_INVENTORY_SLOT_END > wCell) || 
			(GIFT_BOX_INVENTORY_SLOT_START <= wCell && GIFT_BOX_INVENTORY_SLOT_END > wCell)|| 
 			(SWITCH_INVENTORY_SLOT_START <= wCell && SWITCH_INVENTORY_SLOT_END > wCell)|| 
			(COSTUME_INVENTORY_SLOT_START <= wCell && COSTUME_INVENTORY_SLOT_END > wCell)
			)
#else
			if ((wCell < INVENTORY_MAX_NUM) || (BELT_INVENTORY_SLOT_START <= wCell && BELT_INVENTORY_SLOT_END > wCell))
#endif
				pItem->SetWindow(INVENTORY);
			else
				pItem->SetWindow(EQUIPMENT);
			break;
#ifdef ENABLE_SWITCHBOT
		case SWITCHBOT:
			pItem->SetWindow(SWITCHBOT);
			break;
#endif		
		case DRAGON_SOUL_INVENTORY:
			pItem->SetWindow(DRAGON_SOUL_INVENTORY);
			break;
		}
	}
}

LPITEM CHARACTER::GetWear(WORD bCell) const
{
	if (!m_PlayerSlots)
		return nullptr;

	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_log(0, "CHARACTER::GetWear: invalid wear cell %d", bCell);
		return NULL;
	}

	return m_PlayerSlots->pItems[INVENTORY_MAX_NUM + bCell];
}

void CHARACTER::SetWear(WORD bCell, LPITEM item)
{
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::SetItem: invalid item cell %d", bCell);
		return;
	}

#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
	SetItem(TItemPos(INVENTORY, INVENTORY_MAX_NUM + bCell), item, false);
#else
	SetItem(TItemPos (INVENTORY, INVENTORY_MAX_NUM + bCell), item);
#endif
}

void CHARACTER::ClearItem()
{
	int		i;
	LPITEM	item;

	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = GetInventoryItem(i)))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);

			SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
		}
	}
#ifdef ENABLE_SWITCHBOT
	for (i = 0; i < SWITCHBOT_SLOT_COUNT; ++i)
	{
		if ((item = GetItem(TItemPos(SWITCHBOT, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#endif
	for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
}

bool CHARACTER::IsEmptyItemGrid(TItemPos Cell, BYTE bSize, int iExceptionCell) const
{
	if (!m_PlayerSlots)
		return false;

	switch (Cell.window_type)
	{
	case INVENTORY:
		{
			WORD bCell = Cell.cell;

			++iExceptionCell;

			if (Cell.IsBeltInventoryPosition())
			{
				LPITEM beltItem = GetWear(WEAR_BELT);

				if (NULL == beltItem)
					return false;

				if (false == CBeltInventoryHelper::IsAvailableCell(bCell - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
					return false;

				if (m_PlayerSlots->bItemGrid[bCell])
				{
					if (m_PlayerSlots->bItemGrid[bCell] == iExceptionCell)
						return true;

					return false;
				}

				if (bSize == 1)
					return true;

			}
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
			else if (Cell.IsSkillBookInventoryPosition())
			{
				if (bCell < SKILL_BOOK_INVENTORY_SLOT_START)
					return false;

				if (bCell > SKILL_BOOK_INVENTORY_SLOT_START + Skill_Inventory_Size())
					return false;

				if (m_PlayerSlots->bItemGrid[bCell] == (UINT)iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = bCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

					do
					{
						UINT p = bCell + (5 * j);

						if (p >= Skill_Inventory_Size())
							return false;

						if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
							return false;

						if (m_PlayerSlots->bItemGrid[p])
							if (m_PlayerSlots->bItemGrid[p] != iExceptionCell)
								return false;
					} while (++j < bSize);

					return true;
				}
			}
			else if (Cell.IsUpgradeItemsInventoryPosition())
			{
				if (bCell < UPGRADE_ITEMS_INVENTORY_SLOT_START)
					return false;

				if (bCell > UPGRADE_ITEMS_INVENTORY_SLOT_START + Material_Inventory_Size())
					return false;

				if (m_PlayerSlots->bItemGrid[bCell] == (UINT)iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = bCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

					do
					{
						UINT p = bCell + (5 * j);

						if (p >= Material_Inventory_Size())
							return false;

						if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
							return false;

						if (m_PlayerSlots->bItemGrid[p])
							if (m_PlayerSlots->bItemGrid[p] != iExceptionCell)
								return false;
					} while (++j < bSize);

					return true;
				}
			}
			else if (Cell.IsStoneInventoryPosition())
			{
				if (bCell < STONE_INVENTORY_SLOT_START)
					return false;

				if (bCell > STONE_INVENTORY_SLOT_START + Stone_Inventory_Size())
					return false;

				if (m_PlayerSlots->bItemGrid[bCell] == (UINT)iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = bCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

					do
					{
						UINT p = bCell + (5 * j);

						if (p >= Stone_Inventory_Size())
							return false;

						if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
							return false;

						if (m_PlayerSlots->bItemGrid[p])
							if (m_PlayerSlots->bItemGrid[p] != iExceptionCell)
								return false;
					} while (++j < bSize);

					return true;
				}
			}
			else if (Cell.IsGiftBoxInventoryPosition())
			{
				if (bCell < GIFT_BOX_INVENTORY_SLOT_START)
					return false;

				if (bCell > GIFT_BOX_INVENTORY_SLOT_START + Chest_Inventory_Size())
					return false;

				if ( m_PlayerSlots->bItemGrid[bCell] == (UINT)iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = bCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

					do
					{
						UINT p = bCell + (5 * j);

						if (p >= Chest_Inventory_Size())
							return false;

						if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
							return false;

						if (m_PlayerSlots->bItemGrid[p])
							if (m_PlayerSlots->bItemGrid[p] != iExceptionCell)
								return false;
					} while (++j < bSize);

					return true;
				}
			}
			else if (Cell.IsSwitchInventoryPosition())
			{
				if (bCell < SWITCH_INVENTORY_SLOT_START)
					return false;

				if (bCell > SWITCH_INVENTORY_SLOT_START + Enchant_Inventory_Size())
					return false;

				if (m_PlayerSlots->bItemGrid[bCell] == (UINT)iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = bCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

					do
					{
						UINT p = bCell + (5 * j);

						if (p >= Enchant_Inventory_Size())
							return false;

						if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
							return false;

						if (m_PlayerSlots->bItemGrid[p])
							if (m_PlayerSlots->bItemGrid[p] != iExceptionCell)
								return false;
					} while (++j < bSize);

					return true;
				}
			}
			else if (Cell.IsCostumeInventoryPosition())
			{
				if (bCell < COSTUME_INVENTORY_SLOT_START || bCell > (COSTUME_INVENTORY_SLOT_START + Costume_Inventory_Size()))
					return false;

				if (m_PlayerSlots->bItemGrid[bCell])
				{
					if (m_PlayerSlots->bItemGrid[bCell] == iExceptionCell)
					{
						if (bSize == 1)
							return true;

						const BYTE bLocalIdx = bCell - COSTUME_INVENTORY_SLOT_START;

						int j = 1;
						const BYTE bPage = bLocalIdx / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

						do
						{
							const WORD p = bCell + (5 * j);

							const BYTE bTargetLocalIdx = p - COSTUME_INVENTORY_SLOT_START;

							if (bTargetLocalIdx >= Costume_Inventory_Size())
								return false;

							if (bTargetLocalIdx / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
								return false;

							if (m_PlayerSlots->bItemGrid[p])
								if (m_PlayerSlots->bItemGrid[p] != iExceptionCell)
									return false;
						}
						while (++j < bSize);

						return true;
					}
					else
						return false;
				}
	
				if (1 == bSize)
					return true;
				else
				{
					const BYTE bLocalIdx = bCell - COSTUME_INVENTORY_SLOT_START;
					
					int j = 1;
					const BYTE bPage = bLocalIdx / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

					do
					{
						WORD p = bCell + (5 * j);
						const BYTE bTargetLocalIdx = p - COSTUME_INVENTORY_SLOT_START;
						if (bTargetLocalIdx >= Costume_Inventory_Size())
							return false;
						if (bTargetLocalIdx/ (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
							return false;
						if (m_PlayerSlots->bItemGrid[p])
							if (m_PlayerSlots->bItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
			}
#endif

			else if (bCell >= Inventory_Size())
				return false;

			if (m_PlayerSlots->bItemGrid[bCell])
			{
				if (m_PlayerSlots->bItemGrid[bCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = bCell / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT);

					do
					{
						BYTE p = bCell + (5 * j);

						if (p >= Inventory_Size())
							return false;

						if (p / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT) != bPage)
							return false;

						if (m_PlayerSlots->bItemGrid[p])
							if (m_PlayerSlots->bItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;
				BYTE bPage = bCell / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT);

				do
				{
					BYTE p = bCell + (5 * j);

					if (p >= Inventory_Size())
						return false;

					if (p / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_PlayerSlots->bItemGrid[p])
						if (m_PlayerSlots->bItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		{
		WORD wCell = Cell.cell;
		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			return false;
		}

		if (m_PlayerSlots->pSwitchbotItems[wCell])
		{
			return false;
		}

		return true;
		}
#endif
	case DRAGON_SOUL_INVENTORY:
		{
			WORD wCell = Cell.cell;
			if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

			iExceptionCell++;

			if (m_PlayerSlots->wDSItemGrid[wCell])
			{
				if (m_PlayerSlots->wDSItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							return false;

						if (m_PlayerSlots->wDSItemGrid[p])
							if (m_PlayerSlots->wDSItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

					if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
						return false;

					if (m_PlayerSlots->wDSItemGrid[p]) // @fixme159 (from bItemGrid)
						if (m_PlayerSlots->wDSItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
	}
	return false;
}

int CHARACTER::GetEmptyInventory(BYTE size) const
{
	for ( int i = 0; i < Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), size))
			return i;
	return -1;
}

int CHARACTER::GetEmptyInventoryFromIndex(WORD index, BYTE itemSize) const //SPLIT ITEMS
{
	if (index > Inventory_Size())
		return -1;
	
	for (WORD i = index; i < Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), itemSize))
			return i;
	return -1;
}

// SPECIAL STORAGE !!!

int CHARACTER::GetEmptySkillBookInventoryFromIndex(WORD index, BYTE itemSize) const //SPLIT ITEMS
{
	for (WORD i = SKILL_BOOK_INVENTORY_SLOT_START; i < SKILL_BOOK_INVENTORY_SLOT_START+Skill_Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), itemSize))
			return i;
	return -1;
}

int CHARACTER::GetEmptyUpgradeInventoryFromIndex(WORD index, BYTE itemSize) const //SPLIT ITEMS
{
	for (WORD i = UPGRADE_ITEMS_INVENTORY_SLOT_START; i < UPGRADE_ITEMS_INVENTORY_SLOT_START+Material_Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), itemSize))
			return i;
	return -1;
}

int CHARACTER::GetEmptyStoneInventoryFromIndex(WORD index, BYTE itemSize) const //SPLIT ITEMS
{
	for (WORD i = STONE_INVENTORY_SLOT_START; i < STONE_INVENTORY_SLOT_START+Stone_Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), itemSize))
			return i;
	return -1;
}

int CHARACTER::GetEmptyGiftBoxInventoryFromIndex(WORD index, BYTE itemSize) const //SPLIT ITEMS
{
	for (WORD i = GIFT_BOX_INVENTORY_SLOT_START; i < GIFT_BOX_INVENTORY_SLOT_START+Chest_Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), itemSize))
			return i;
	return -1;
}

int CHARACTER::GetEmptySwitchInventoryFromIndex(WORD index, BYTE itemSize) const //SPLIT ITEMS
{
	for (WORD i = SWITCH_INVENTORY_SLOT_START; i < SWITCH_INVENTORY_SLOT_START+Enchant_Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), itemSize))
			return i;
	return -1;
}

int CHARACTER::GetEmptyCostumeInventoryFromIndex(WORD index, BYTE itemSize) const //SPLIT ITEMS
{
	for (WORD i = COSTUME_INVENTORY_SLOT_START; i < COSTUME_INVENTORY_SLOT_START+Costume_Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), itemSize))
			return i;
	return -1;
}



// END OF SPECIAL STORAGE !!
int CHARACTER::GetEmptyDragonSoulInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsDragonSoul())
		return -1;
	if (!DragonSoul_IsQualified())
	{
		return -1;
	}
	BYTE bSize = pItem->GetSize();
	WORD wBaseCell = DSManager::instance().GetBasePosition(pItem);

	if (WORD_MAX == wBaseCell)
		return -1;

	for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
		if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, i + wBaseCell), bSize))
			return i + wBaseCell;

	return -1;
}

#if defined(__SPECIAL_INVENTORY_SYSTEM__)
int CHARACTER::GetEmptySkillBookInventory(BYTE size) const
{
	for (int i = SKILL_BOOK_INVENTORY_SLOT_START; i < SKILL_BOOK_INVENTORY_SLOT_START+Skill_Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos(INVENTORY, i), size))
			return i;

	return -1;
}

int CHARACTER::GetEmptyUpgradeItemsInventory(BYTE size) const
{
	for (int i = UPGRADE_ITEMS_INVENTORY_SLOT_START; i < UPGRADE_ITEMS_INVENTORY_SLOT_START+Material_Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos(INVENTORY, i), size))
			return i;

	return -1;
}

int CHARACTER::GetEmptyStoneInventory(BYTE size) const
{
	for (int i = STONE_INVENTORY_SLOT_START; i < STONE_INVENTORY_SLOT_START+Stone_Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos(INVENTORY, i), size))
			return i;

	return -1;
}

int CHARACTER::GetEmptyGiftBoxInventory(BYTE size) const
{
	for (int i = GIFT_BOX_INVENTORY_SLOT_START; i < GIFT_BOX_INVENTORY_SLOT_START+Chest_Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos(INVENTORY, i), size))
			return i;

	return -1;
}

int CHARACTER::GetEmptySwitchInventory(BYTE size) const
{
	for (int i = SWITCH_INVENTORY_SLOT_START; i < SWITCH_INVENTORY_SLOT_START+Enchant_Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos(INVENTORY, i), size))
			return i;

	return -1;
}

int CHARACTER::GetEmptyCostumeInventory(BYTE size) const
{
	for (int i = COSTUME_INVENTORY_SLOT_START; i < COSTUME_INVENTORY_SLOT_START+Costume_Inventory_Size(); ++i)
		if (IsEmptyItemGrid(TItemPos(INVENTORY, i), size))
			return i;

	return -1;
}
#endif

void CHARACTER::CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const
{
	vDragonSoulItemGrid.resize(DRAGON_SOUL_INVENTORY_MAX_NUM);

	std::copy(m_PlayerSlots->wDSItemGrid, m_PlayerSlots->wDSItemGrid + DRAGON_SOUL_INVENTORY_MAX_NUM, vDragonSoulItemGrid.begin());
}

int CHARACTER::CountEmptyInventory() const
{
	int	count = 0;

	for (int i = 0; i < Inventory_Size(); ++i)
		if (GetInventoryItem(i))
			count += GetInventoryItem(i)->GetSize();

	return (Inventory_Size() - count);
}

void TransformRefineItem(LPITEM pkOldItem, LPITEM pkNewItem)
{
	// ACCESSORY_REFINE
	if (pkOldItem->IsAccessoryForSocket())
	{
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			pkNewItem->SetSocket(i, pkOldItem->GetSocket(i));
		}
		//pkNewItem->StartAccessorySocketExpireEvent();
	}
	// END_OF_ACCESSORY_REFINE
	else
	{
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			if (!pkOldItem->GetSocket(i))
				break;
			else
				pkNewItem->SetSocket(i, 1);
		}

		int slot = 0;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			long socket = pkOldItem->GetSocket(i);

			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				pkNewItem->SetSocket(slot++, socket);
		}

	}

	pkOldItem->CopyAttributeTo(pkNewItem);
}

void NotifyRefineSuccess(LPCHARACTER ch, LPITEM item, const char* way)
{
	if (NULL != ch && item != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineSuceeded");
	}
}

void NotifyRefineFail(LPCHARACTER ch, LPITEM item, const char* way, int success = 0)
{
	if (NULL != ch && NULL != item)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineFailed");
	}
}

#ifdef ENABLE_REFINE_MSG_ADD
void NotifyRefineFailType(const LPCHARACTER pkChr, const LPITEM pkItem, const BYTE bType, const std::string stRefineType, const BYTE bSuccess = 0)
{
	if (pkChr && pkItem)
	{
		pkChr->ChatPacket(CHAT_TYPE_COMMAND, "RefineFailedType %d", bType);
	}
}
#endif

void CHARACTER::SetRefineNPC(LPCHARACTER ch)
{
	if ( ch != NULL )
	{
		m_dwRefineNPCVID = ch->GetVID();
	}
	else
	{
		m_dwRefineNPCVID = 0;
	}
}

bool CHARACTER::DoRefine(LPITEM item, bool bMoneyOnly)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	if (quest::CQuestManager::instance().GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::instance().GetEventFlag("update_refine_time") + (60 * 5))
		{
			sys_log(0, "can't refine %d %s", GetPlayerID(), GetName());
			return false;
		}
	}

	const TRefineTable * prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());

	if (!prt)
		return false;

	DWORD result_vnum = item->GetRefinedVnum();

	// REFINE_COST
	int cost = ComputeRefineFee(prt->cost);

	int RefineChance = GetQuestFlag("main_quest_lv7.refine_chance");

	if (RefineChance > 0)
	{
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("무료 개량 기회는 20 이하의 무기만 가능합니다"));
			return false;
		}

		cost = 0;
		SetQuestFlag("main_quest_lv7.refine_chance", RefineChance - 1);
	}
	// END_OF_REFINE_COST

	if (result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("더 이상 개량할 수 없습니다."));
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
		return false;

	TItemTable * pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefine NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 아이템은 개량할 수 없습니다."));
		return false;
	}

	// REFINE_COST
	if (GetGold() < cost)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("개량을 하기 위한 돈이 부족합니다."));
		return false;
	}

	if (!bMoneyOnly && !RefineChance)
	{
		for (int i = 0; i < prt->material_count; ++i)
		{
			DWORD material_count;

			if (prt->materials[i].vnum == item->GetVnum())
				material_count = prt->materials[i].count + 1;
			else
				material_count = prt->materials[i].count;

			if (CountSpecifyItem(prt->materials[i].vnum, item->GetCell()) < prt->materials[i].count)
			{
				if (test_server)
				{
					ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
				}

				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("개량을 하기 위한 재료가 부족합니다."));
				return false;
			}
		}

		for (int i = 0; i < prt->material_count; ++i)
			RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count, item->GetCell());
	}

	int prob = number(1, 100);

	if (IsRefineThroughGuild() || bMoneyOnly)
		prob -= 10;

	// END_OF_REFINE_COST

#ifdef ENABLE_BATTLE_PASS
	CHARACTER_MANAGER::Instance().DoMission(this, MISSION_CRAFT_ITEM, 1, item->GetVnum());
#endif

	if (prob <= prt->prob)
	{
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE_SUCCESS", pkNewItem->GetName());

			WORD bCell = item->GetCell();

			// DETAIL_REFINE_LOG
			NotifyRefineSuccess(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");
			// END_OF_DETAIL_REFINE_LOG

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

			sys_log(0, "Refine Success %d", cost);
			pkNewItem->AttrLog();
			//PointChange(POINT_GOLD, -cost);
			sys_log(0, "PayPee %d", cost);
			PayRefineFee(cost);
			sys_log(0, "PayPee End %d", cost);
		}
		else
		{
			// DETAIL_REFINE_LOG

			sys_err("cannot create item %u", result_vnum);
#ifdef ENABLE_REFINE_MSG_ADD
			NotifyRefineFailType(this, item, REFINE_FAIL_KEEP_GRADE, IsRefineThroughGuild() ? "GUILD" : "POWER");
#else
			NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
#endif
			LogManager::instance().ItemLog(this, item, "REFINE_FAIL", item->GetName());
			// END_OF_DETAIL_REFINE_LOG
		}
	}
	else
	{
		DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
#ifdef ENABLE_REFINE_MSG_ADD
		NotifyRefineFailType(this, item, REFINE_FAIL_DEL_ITEM, IsRefineThroughGuild() ? "GUILD" : "POWER");
#else
		NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
#endif
		LogManager::instance().ItemLog(this, item, "REFINE_FAIL", item->GetName());
		item->AttrLog();
		ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");

		//PointChange(POINT_GOLD, -cost);
		PayRefineFee(cost);
	}

	return true;
}

enum enum_RefineScrolls
{
	CHUKBOK_SCROLL = 0,
	HYUNIRON_CHN   = 1,
	YONGSIN_SCROLL = 2,
	MUSIN_SCROLL   = 3,
	YAGONG_SCROLL  = 4,
	MEMO_SCROLL	   = 5,
	BDRAGON_SCROLL	= 6,
#ifdef __RENEWAL_CRYSTAL__
	CRYSTAL_FIRST_SCROLL = 7,
	CRYSTAL_SECOND_SCROLL = 8,
#endif
};

bool CHARACTER::DoRefineWithScroll(LPITEM item)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	ClearRefineMode();

	if (quest::CQuestManager::instance().GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::instance().GetEventFlag("update_refine_time") + (60 * 5))
		{
			sys_log(0, "can't refine %d %s", GetPlayerID(), GetName());
			return false;
		}
	}

	const TRefineTable * prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());

	if (!prt)
		return false;

	LPITEM pkItemScroll;

	if (m_iRefineAdditionalCell < 0)
		return false;

	pkItemScroll = GetInventoryItem(m_iRefineAdditionalCell);

	if (!pkItemScroll)
		return false;

	if (!(pkItemScroll->GetType() == ITEM_USE && pkItemScroll->GetSubType() == USE_TUNING))
		return false;

	if (pkItemScroll->GetVnum() == item->GetVnum())
		return false;

	DWORD result_vnum = item->GetRefinedVnum();
	DWORD result_fail_vnum = item->GetRefineFromVnum();

	if (result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("더 이상 개량할 수 없습니다."));
		return false;
	}

	// NEW REFINE INFO

	// if (pkItemScroll->GetVnum() == 25040)
	// {
	// 	if ((item->GetVnum()>=300 && item->GetVnum()<= 319) || (item->GetVnum()>=1180 && item->GetVnum()<= 1189) || (item->GetVnum()>=2200 && item->GetVnum()<= 2209) || (item->GetVnum()>=3220 && item->GetVnum()<= 3229) || (item->GetVnum()>=5160 && item->GetVnum()<= 5169) || (item->GetVnum()>=7300 && item->GetVnum()<= 7309) || (item->GetVnum()>=19290 && item->GetVnum()<= 19299) || (item->GetVnum()>=19490 && item->GetVnum()<= 19499) || (item->GetVnum()>=19690 && item->GetVnum()<= 19699) || (item->GetVnum()>=19890 && item->GetVnum()<= 19899))
	// 	{
	// 		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This scroll cannot be used on Zodiac Items."));
	// 		return false;
	// 	}
	// }

	if (pkItemScroll->GetVnum() == 71021)
	{
		if ((item->GetVnum()>=300 && item->GetVnum()<= 319) || (item->GetVnum()>=1180 && item->GetVnum()<= 1189) || (item->GetVnum()>=2200 && item->GetVnum()<= 2209) || (item->GetVnum()>=3220 && item->GetVnum()<= 3229) || (item->GetVnum()>=5160 && item->GetVnum()<= 5169) || (item->GetVnum()>=7300 && item->GetVnum()<= 7309) || (item->GetVnum()>=19290 && item->GetVnum()<= 19299) || (item->GetVnum()>=19490 && item->GetVnum()<= 19499) || (item->GetVnum()>=19690 && item->GetVnum()<= 19699) || (item->GetVnum()>=19890 && item->GetVnum()<= 19899))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This scroll cannot be used on Zodiac Items."));
			return false;
		}

		if ((item->GetLevelLimit() >= 75)) {
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This scroll cannot be used on items with level higher than 75."));
			return false;
		}

		if (item->GetRefineLevel() > 4) {
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This scroll cannot be used on items with Refine Level above +4."));
			return false;
		}

	}

	if (pkItemScroll->GetVnum() == 71032)
	{
		if ((item->GetVnum()>=300 && item->GetVnum()<= 319) || (item->GetVnum()>=1180 && item->GetVnum()<= 1189) || (item->GetVnum()>=2200 && item->GetVnum()<= 2209) || (item->GetVnum()>=3220 && item->GetVnum()<= 3229) || (item->GetVnum()>=5160 && item->GetVnum()<= 5169) || (item->GetVnum()>=7300 && item->GetVnum()<= 7309) || (item->GetVnum()>=19290 && item->GetVnum()<= 19299) || (item->GetVnum()>=19490 && item->GetVnum()<= 19499) || (item->GetVnum()>=19690 && item->GetVnum()<= 19699) || (item->GetVnum()>=19890 && item->GetVnum()<= 19899))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This scroll cannot be used on Zodiac Items."));
			return false;
		}
	}

	if (pkItemScroll->GetVnum() == 351220)
	{
		if ((item->GetVnum()>=300 && item->GetVnum()<= 319) || (item->GetVnum()>=1180 && item->GetVnum()<= 1189) || (item->GetVnum()>=2200 && item->GetVnum()<= 2209) || (item->GetVnum()>=3220 && item->GetVnum()<= 3229) || (item->GetVnum()>=5160 && item->GetVnum()<= 5169) || (item->GetVnum()>=7300 && item->GetVnum()<= 7309) || (item->GetVnum()>=19290 && item->GetVnum()<= 19299) || (item->GetVnum()>=19490 && item->GetVnum()<= 19499) || (item->GetVnum()>=19690 && item->GetVnum()<= 19699) || (item->GetVnum()>=19890 && item->GetVnum()<= 19899))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This scroll cannot be used on Zodiac Items."));
			return false;
		}

		if ((item->GetLevelLimit() > 75)) {
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This scroll cannot be used on items with level higher than 75."));
			return false;
		}
	}

	// if (pkItemScroll->GetVnum() == 70039)
	// {
	// 	if ((item->GetVnum()>=300 && item->GetVnum()<= 319) || (item->GetVnum()>=1180 && item->GetVnum()<= 1189) || (item->GetVnum()>=2200 && item->GetVnum()<= 2209) || (item->GetVnum()>=3220 && item->GetVnum()<= 3229) || (item->GetVnum()>=5160 && item->GetVnum()<= 5169) || (item->GetVnum()>=7300 && item->GetVnum()<= 7309) || (item->GetVnum()>=19290 && item->GetVnum()<= 19299) || (item->GetVnum()>=19490 && item->GetVnum()<= 19499) || (item->GetVnum()>=19690 && item->GetVnum()<= 19699) || (item->GetVnum()>=19890 && item->GetVnum()<= 19899))
	// 	{
	// 		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This scroll cannot be used on Zodiac Items."));
	// 		return false;
	// 	}
	// }

	// if (pkItemScroll->GetVnum() == 25041)
	// {
	// 	if ((item->GetVnum()>=300 && item->GetVnum()<= 319) || (item->GetVnum()>=1180 && item->GetVnum()<= 1189) || (item->GetVnum()>=2200 && item->GetVnum()<= 2209) || (item->GetVnum()>=3220 && item->GetVnum()<= 3229) || (item->GetVnum()>=5160 && item->GetVnum()<= 5169) || (item->GetVnum()>=7300 && item->GetVnum()<= 7309) || (item->GetVnum()>=19290 && item->GetVnum()<= 19299) || (item->GetVnum()>=19490 && item->GetVnum()<= 19499) || (item->GetVnum()>=19690 && item->GetVnum()<= 19699) || (item->GetVnum()>=19890 && item->GetVnum()<= 19899))
	// 	{
	// 		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This scroll cannot be used on Zodiac Items."));
	// 		return false;
	// 	}
	// }
	
	if (pkItemScroll->GetVnum() == 25042)
	{
		if ((item->GetVnum()>=300 && item->GetVnum()<= 319) || (item->GetVnum()>=1180 && item->GetVnum()<= 1189) || (item->GetVnum()>=2200 && item->GetVnum()<= 2209) || (item->GetVnum()>=3220 && item->GetVnum()<= 3229) || (item->GetVnum()>=5160 && item->GetVnum()<= 5169) || (item->GetVnum()>=7300 && item->GetVnum()<= 7309) || (item->GetVnum()>=19290 && item->GetVnum()<= 19299) || (item->GetVnum()>=19490 && item->GetVnum()<= 19499) || (item->GetVnum()>=19690 && item->GetVnum()<= 19699) || (item->GetVnum()>=19890 && item->GetVnum()<= 19899))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This scroll cannot be used on Zodiac Items."));
			return false;
		}
	}

	if (pkItemScroll->GetVnum() == 25043)
	{
		if (!((item->GetVnum()>=300 && item->GetVnum()<= 319) || (item->GetVnum()>=1180 && item->GetVnum()<= 1189) || (item->GetVnum()>=2200 && item->GetVnum()<= 2209) || (item->GetVnum()>=3220 && item->GetVnum()<= 3229) || (item->GetVnum()>=5160 && item->GetVnum()<= 5169) || (item->GetVnum()>=7300 && item->GetVnum()<= 7309) || (item->GetVnum()>=19290 && item->GetVnum()<= 19299) || (item->GetVnum()>=19490 && item->GetVnum()<= 19499) || (item->GetVnum()>=19690 && item->GetVnum()<= 19699) || (item->GetVnum()>=19890 && item->GetVnum()<= 19899)))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This scroll cannot be used on normal items, just Zodiac."));
			return false;
		}
	}

	// NEW REFINE INFO

	TItemTable * pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefineWithScroll NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 아이템은 개량할 수 없습니다."));
		return false;
	}

#ifdef __RENEWAL_CRYSTAL__
	const bool isFirstCrystalRange = (item->GetVnum() >= 51010 && item->GetVnum() <= 51029) ? true : false;
	const bool isSecondCrystalRange = (item->GetVnum() >= 51030 && item->GetVnum() <= 51035) ? true : false;
	if (
		(pkItemScroll->GetValue(0) == CRYSTAL_FIRST_SCROLL && !isFirstCrystalRange) || (pkItemScroll->GetValue(0) != CRYSTAL_FIRST_SCROLL && isFirstCrystalRange) ||
		(pkItemScroll->GetValue(0) == CRYSTAL_SECOND_SCROLL && !isSecondCrystalRange) || (pkItemScroll->GetValue(0) != CRYSTAL_SECOND_SCROLL && isSecondCrystalRange)
		)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item cannot be improved with this combination."));
		return false;
	}
	if (item->GetSocket(0) && (isFirstCrystalRange || isSecondCrystalRange))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use on this active energy crystal."));
		return false;
	}
#endif

	if (GetGold() < prt->cost)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("개량을 하기 위한 돈이 부족합니다."));
		return false;
	}

	for (int i = 0; i < prt->material_count; ++i)
	{
		DWORD material_count;

		if (prt->materials[i].vnum == item->GetVnum())
			material_count = prt->materials[i].count + 1;
		else
			material_count = prt->materials[i].count;

		if (CountSpecifyItem(prt->materials[i].vnum, item->GetCell()) < prt->materials[i].count)
		{
			if (test_server)
			{
				ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
			}

			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("개량을 하기 위한 재료가 부족합니다."));
			return false;
		}
	}

	for (int i = 0; i < prt->material_count; ++i)
		RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count, item->GetCell());

	int prob = number(1, 100);
	int success_prob = prt->prob;
	bool bDestroyWhenFail = false;

	const char* szRefineType = "SCROLL";

	if (test_server)
	{
		ChatPacket(CHAT_TYPE_INFO, "[Only Test] Success_Prob %d, RefineLevel %d ", success_prob, item->GetRefineLevel());
	}

#ifdef __RENEWAL_CRYSTAL__
	if (pkItemScroll->GetValue(0) == CRYSTAL_FIRST_SCROLL || pkItemScroll->GetValue(0) == CRYSTAL_SECOND_SCROLL )
	{
		szRefineType = "CRYSTAL_SCROLL";
		bDestroyWhenFail = true;
	}
#endif

	// NEW REFINE INFO

	if (pkItemScroll->GetVnum() == 71021) {
		success_prob = 100;
	}

	if (pkItemScroll->GetVnum() == 71032) {
		success_prob += 10;
		// redus cu 1
	}

	if (pkItemScroll->GetVnum() == 351220) {
		success_prob += 10;
		// redus cu 1
	}

	if (pkItemScroll->GetVnum() == 70039) {
		success_prob += 15;
		// redus cu 1
	}

	if (pkItemScroll->GetVnum() == 25041) {
		success_prob = prt->prob;
		bDestroyWhenFail = true;
	}

	if (pkItemScroll->GetVnum() == 25042) {
		success_prob += 10;
		bDestroyWhenFail = true;
	}

	if (pkItemScroll->GetVnum() == 25043) {
		success_prob += 10;
		bDestroyWhenFail = true;
	}

#ifdef ENABLE_BATTLE_PASS
	CHARACTER_MANAGER::Instance().DoMission(this, MISSION_CRAFT_ITEM, 1, item->GetVnum());
	CHARACTER_MANAGER::Instance().DoMission(this, MISSION_REFINE_ITEM, 1, pkItemScroll->GetVnum());
#endif
	pkItemScroll->SetCount(pkItemScroll->GetCount() - 1);

	if (prob <= success_prob)
	{
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE_SUCCESS", pkNewItem->GetName());

			WORD bCell = item->GetCell();

			NotifyRefineSuccess(this, item, szRefineType);
			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			pkNewItem->AttrLog();
			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost);
		}
		else
		{
			sys_err("cannot create item %u", result_vnum);
#ifdef ENABLE_REFINE_MSG_ADD
			NotifyRefineFailType(this, item, REFINE_FAIL_KEEP_GRADE, szRefineType);
#else
			NotifyRefineFail(this, item, szRefineType);
#endif
			LogManager::instance().ItemLog(this, item, "REFINE_FAIL", item->GetName());
		}
	}
	else if (!bDestroyWhenFail && result_fail_vnum)
	{
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_fail_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, item, "REFINE_FAIL", item->GetName());

			WORD bCell = item->GetCell();

			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost);
#ifdef ENABLE_REFINE_MSG_ADD
			NotifyRefineFailType(this, item, REFINE_FAIL_GRADE_DOWN, szRefineType, -1);
#else
			NotifyRefineFail(this, item, szRefineType, -1);
#endif
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

			pkNewItem->AttrLog();

			PayRefineFee(prt->cost);
		}
		else
		{
			sys_err("cannot create item %u", result_fail_vnum);
			NotifyRefineFail(this, item, szRefineType);
			LogManager::instance().ItemLog(this, item, "REFINE_FAIL", item->GetName());
		}
	}
	else
	{
#ifdef ENABLE_REFINE_MSG_ADD
		NotifyRefineFailType(this, item, REFINE_FAIL_KEEP_GRADE, szRefineType);
#else
		NotifyRefineFail(this, item, szRefineType);
#endif
		LogManager::instance().ItemLog(this, item, "REFINE_FAIL", item->GetName());

		PayRefineFee(prt->cost);
	}

	return true;
}

bool CHARACTER::RefineInformation(WORD bCell, BYTE bType, int iAdditionalCell, int RefinementType)
{
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	if (bCell > INVENTORY_MAX_NUM + SPECIAL_INVENTORY_MAX_PAGE_NUM)
		return false;
#else
	if (bCell > INVENTORY_MAX_NUM)
		return false;
#endif

	LPITEM item = GetInventoryItem(bCell);

	if (!item)
		return false;

	// REFINE_COST
	if (bType == REFINE_TYPE_MONEY_ONLY && !GetQuestFlag("dungeon_devil_tower.can_refine"))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("사귀 타워 완료 보상은 한번까지 사용가능합니다."));
		return false;
	}
	// END_OF_REFINE_COST

	TPacketGCRefineInformation p;

	p.header = HEADER_GC_REFINE_INFORMATION;
	p.pos = bCell;
	p.src_vnum = item->GetVnum();
	p.result_vnum = item->GetRefinedVnum();
	p.type = bType;
	p.refinement_type = RefinementType;

	if (p.result_vnum == 0)
	{
		sys_err("RefineInformation p.result_vnum == 0");
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 아이템은 개량할 수 없습니다."));
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
	{
		if (bType == 0)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 아이템은 이 방식으로는 개량할 수 없습니다."));
			return false;
		}
		else
		{
			LPITEM itemScroll = GetInventoryItem(iAdditionalCell);
			if (!itemScroll || item->GetVnum() == itemScroll->GetVnum())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("같은 개량서를 합칠 수는 없습니다."));
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("축복의 서와 현철을 합칠 수 있습니다."));
				return false;
			}
		}
	}

	CRefineManager & rm = CRefineManager::instance();

	const TRefineTable* prt = rm.GetRefineRecipe(item->GetRefineSet());

	if (!prt)
	{
		sys_err("RefineInformation NOT GET REFINE SET %d", item->GetRefineSet());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 아이템은 개량할 수 없습니다."));
		return false;
	}

	// REFINE_COST

	//MAIN_QUEST_LV7
	if (GetQuestFlag("main_quest_lv7.refine_chance") > 0)
	{
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("무료 개량 기회는 20 이하의 무기만 가능합니다"));
			return false;
		}
		p.cost = 0;
	}
	else
		p.cost = ComputeRefineFee(prt->cost);

	//END_MAIN_QUEST_LV7
	p.prob = prt->prob;
	if (bType == REFINE_TYPE_MONEY_ONLY)
	{
		p.material_count = 0;
		memset(p.materials, 0, sizeof(p.materials));
	}
	else
	{
		p.material_count = prt->material_count;
		thecore_memcpy(&p.materials, prt->materials, sizeof(prt->materials));
	}
	// END_OF_REFINE_COST

	GetDesc()->Packet(&p, sizeof(TPacketGCRefineInformation));

	SetRefineMode(iAdditionalCell);
	return true;
}

bool CHARACTER::RefineItem(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
		return false;

	int RefinementType = 0;

#ifdef NEW_REFINE_INFO
	if (pkItem->GetVnum() == 25040)
		RefinementType = 1;
	else if (pkItem->GetVnum() == 71021)
		RefinementType = 2;
	else if (pkItem->GetVnum() == 71032)
		RefinementType = 3;
	else if (pkItem->GetVnum() == 351220)
		RefinementType = 4;
	else if (pkItem->GetVnum() == 70039)
		RefinementType = 5;
	else if (pkItem->GetVnum() == 25041)
		RefinementType = 6;
	else if (pkItem->GetVnum() == 25042)
		RefinementType = 7;
	else if (pkItem->GetVnum() == 25043)
		RefinementType = 8;
#endif

	if (pkItem->GetSubType() == USE_TUNING)
	{
		// MUSIN_SCROLL
		if (pkItem->GetValue(0) == MUSIN_SCROLL)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_MUSIN, pkItem->GetCell(), RefinementType);
		// END_OF_MUSIN_SCROLL
		else if (pkItem->GetValue(0) == HYUNIRON_CHN)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_HYUNIRON, pkItem->GetCell(), RefinementType);
		else if (pkItem->GetValue(0) == BDRAGON_SCROLL)
		{
			if (pkTarget->GetRefineSet() != 702) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_BDRAGON, pkItem->GetCell(), RefinementType);
		}
		else
		{
			if (pkTarget->GetRefineSet() == 501) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell(), RefinementType);
		}
	}
	else if (pkItem->GetSubType() == USE_DETACHMENT && IS_SET(pkTarget->GetFlag(), ITEM_FLAG_REFINEABLE))
	{

		bool bHasMetinStone = false;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		{
			long socket = pkTarget->GetSocket(i);
			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
			{
				bHasMetinStone = true;
				break;
			}
		}

		if (bHasMetinStone)
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				long socket = pkTarget->GetSocket(i);
				if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				{
					AutoGiveItem(socket);
					//TItemTable* pTable = ITEM_MANAGER::instance().GetTable(pkTarget->GetSocket(i));
					//pkTarget->SetSocket(i, pTable->alValues[2]);

					pkTarget->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
				}
			}
			pkItem->SetCount(pkItem->GetCount() - 1);
			return true;
		}
		else
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("빼낼 수 있는 메틴석이 없습니다."));
			return false;
		}
	}

	return false;
}

EVENTFUNC(kill_campfire_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "kill_campfire_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}
	ch->m_pkMiningEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

bool CHARACTER::GiveRecallItem(LPITEM item)
{
	int idx = GetMapIndex();
	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("기억해 둘 수 없는 위치 입니다."));
		return false;
	}

	int pos;

	if (item->GetCount() == 1)
	{
		item->SetSocket(0, GetX());
		item->SetSocket(1, GetY());
	}
	else if ((pos = GetEmptyInventory(item->GetSize())) != -1)
	{
		LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), 1);

		if (NULL != item2)
		{
			item2->SetSocket(0, GetX());
			item2->SetSocket(1, GetY());
			item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

			item->SetCount(item->GetCount() - 1);
		}
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지품에 빈 공간이 없습니다."));
		return false;
	}

	return true;
}

void CHARACTER::ProcessRecallItem(LPITEM item)
{
	int idx;

	if ((idx = SECTREE_MANAGER::instance().GetMapIndex(item->GetSocket(0), item->GetSocket(1))) == 0)
		return;

	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;

		case 301:
		case 302:
		case 303:
		case 304:
			if( GetLevel() < 90 )
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템의 레벨 제한보다 레벨이 낮습니다."));
				return;
			}
			else
				break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("기억된 위치가 타제국에 속해 있어서 귀환할 수 없습니다."));
		item->SetSocket(0, 0);
		item->SetSocket(1, 0);
	}
	else
	{
		sys_log(1, "Recall: %s %d %d -> %d %d", GetName(), GetX(), GetY(), item->GetSocket(0), item->GetSocket(1));
		WarpSet(item->GetSocket(0), item->GetSocket(1));
		item->SetCount(item->GetCount() - 1);
	}
}

void CHARACTER::__OpenPrivateShop()
{
#ifdef ENABLE_OPEN_SHOP_WITH_ARMOR
	ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
#else
	unsigned bodyPart = GetPart(PART_MAIN);
	switch (bodyPart)
	{
		case 0:
		case 1:
		case 2:
			ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
			break;
		default:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("갑옷을 벗어야 개인 상점을 열 수 있습니다."));
			break;
	}
#endif
}

// MYSHOP_PRICE_LIST
void CHARACTER::SendMyShopPriceListCmd(DWORD dwItemVnum, DWORD dwItemPrice)
{
	char szLine[256];
	snprintf(szLine, sizeof(szLine), "MyShopPriceList %u %u", dwItemVnum, dwItemPrice);
	ChatPacket(CHAT_TYPE_COMMAND, szLine);
	sys_log(0, szLine);
}

//

//
void CHARACTER::UseSilkBotaryReal(const TPacketMyshopPricelistHeader* p)
{
	const TItemPriceInfo* pInfo = (const TItemPriceInfo*)(p + 1);

	if (!p->byCount)

		SendMyShopPriceListCmd(1, 0);
	else {
		for (int idx = 0; idx < p->byCount; idx++)
			SendMyShopPriceListCmd(pInfo[ idx ].dwVnum, pInfo[ idx ].dwPrice);
	}

	__OpenPrivateShop();
}

//

//
void CHARACTER::UseSilkBotary(void)
{
	if (m_bNoOpenedShop) {
		DWORD dwPlayerID = GetPlayerID();
		db_clientdesc->DBPacket(HEADER_GD_MYSHOP_PRICELIST_REQ, GetDesc()->GetHandle(), &dwPlayerID, sizeof(DWORD));
		m_bNoOpenedShop = false;
	} else {
		__OpenPrivateShop();
	}
}
// END_OF_MYSHOP_PRICE_LIST

#if defined(__EXTENDED_BLEND_AFFECT__)
bool CHARACTER::UseExtendedBlendAffect(LPITEM item, int affect_type, int apply_type, int apply_value, int apply_duration)
{
	bool bStatus = item->GetSocket(3);

	if (FindAffect(affect_type, apply_type))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
		return false;
	}

	if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, apply_type) || FindAffect(AFFECT_MALL, apply_type))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
		return false;
	}

	switch (item->GetVnum())
	{
		case 50830:
		{
			if (FindAffect(AFFECT_BLEND_MONSTERS))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_BLEND_MONSTERS);
				return false;
			}
			AddAffect(AFFECT_BLEND_MONSTERS, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
			// DEWS
		case 50821:
		case 950821:
		{
			if (FindAffect(AFFECT_BLEND_POTION_1))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_BLEND_POTION_1);
				return false;
			}
			AddAffect(AFFECT_BLEND_POTION_1, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 50822:
		case 950822:
		{
			if (FindAffect(AFFECT_BLEND_POTION_2))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_BLEND_POTION_2);
				return false;
			}
			AddAffect(AFFECT_BLEND_POTION_2, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 50823:
		case 950823:
		{
			if (FindAffect(AFFECT_BLEND_POTION_3))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_BLEND_POTION_3);
				return false;
			}
			AddAffect(AFFECT_BLEND_POTION_3, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 50824:
		case 950824:
		{
			if (FindAffect(AFFECT_BLEND_POTION_4))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_BLEND_POTION_4);
				return false;
			}
			if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_RESIST_MAGIC))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_BLEND_POTION_4, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 50825:
		case 950825:
		{
			if (FindAffect(AFFECT_BLEND_POTION_5))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_BLEND_POTION_5);
				return false;
			}
			AddAffect(AFFECT_BLEND_POTION_5, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 950826:
		{
			if (FindAffect(AFFECT_BLEND_POTION_6))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_BLEND_POTION_6);
				return false;
			}
			AddAffect(AFFECT_BLEND_POTION_6, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		// END_OF_DEWS
	
		// DRAGON_GOD_MEDALS
		case 39017:
		case 71027:
		case 939017:
		{
			if (FindAffect(AFFECT_DRAGON_GOD_1))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_DRAGON_GOD_1);
				return false;
			}
	
			AddAffect(AFFECT_DRAGON_GOD_1, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 39018:
		case 71028:
		case 939018:
		{
			if (FindAffect(AFFECT_DRAGON_GOD_2))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_DRAGON_GOD_2);
				return false;
			}
			AddAffect(AFFECT_DRAGON_GOD_2, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 39019:
		case 71029:
		case 939019:
		{
			if (FindAffect(AFFECT_DRAGON_GOD_3))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_DRAGON_GOD_3);
				return false;
			}
			AddAffect(AFFECT_DRAGON_GOD_3, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 39020:
		case 71030:
		case 939020:
		{
			if (FindAffect(AFFECT_DRAGON_GOD_4))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_DRAGON_GOD_4);
				return false;
			}
			AddAffect(AFFECT_DRAGON_GOD_4, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		// END_OF_DRAGON_GOD_MEDALS
	
		// CRITICAL_AND_PENETRATION
		case 39024:
		case 71044:
		case 939024:
		{
			if (FindAffect(AFFECT_CRITICAL))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_CRITICAL);
				return false;
			}
			AddAffect(AFFECT_CRITICAL, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 39025:
		case 71045:
		case 939025:
		{
			if (FindAffect(AFFECT_PENETRATE))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_PENETRATE);
				return false;
			}
			AddAffect(AFFECT_PENETRATE, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		// END_OF_CRITICAL_AND_PENETRATION
	
		// ATTACK_AND_MOVE_SPEED
		case 27102:
		{
			if (FindAffect(AFFECT_ATTACK_SPEED))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_ATTACK_SPEED);
				return false;
			}
	
			if (FindAffect(AFFECT_ATT_SPEED))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_ATTACK_SPEED, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
	
		// BLESSED WATER
		case 50817:
		case 95219:
		{
			if (FindAffect(AFFECT_ATT_WATER))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_ATT_WATER);
				return false;
			}
	
			if (FindAffect(AFFECT_ATT_WATER))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_ATT_WATER, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 50818:
		case 95220:
		{
			if (FindAffect(AFFECT_DEF_WATER))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_DEF_WATER);
				return false;
			}
	
			if (FindAffect(AFFECT_DEF_WATER))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_DEF_WATER, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
	
		/// NEW FISH
		case 27863:
		{
			if (FindAffect(AFFECT_FISH1))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH1);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH1))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH1, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27864:
		{
			if (FindAffect(AFFECT_FISH2))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH2);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH2))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH2, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27865:
		{
			if (FindAffect(AFFECT_FISH3))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH3);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH3))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH3, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27866:
		{
			if (FindAffect(AFFECT_FISH4))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH4);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH4))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH4, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27867:
		{
			if (FindAffect(AFFECT_FISH5))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH5);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH5))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH5, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27868:
		{
			if (FindAffect(AFFECT_FISH6))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH6);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH6))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH6, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27869:
		{
			if (FindAffect(AFFECT_FISH7))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH7);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH7))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH7, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27870:
		{
			if (FindAffect(AFFECT_FISH8))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH8);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH8))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH8, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27871:
		{
			if (FindAffect(AFFECT_FISH9))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH9);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH9))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH9, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27872:
		{
			if (FindAffect(AFFECT_FISH10))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH10);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH10))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH10, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27873:
		{
			if (FindAffect(AFFECT_FISH11))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH11);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH11))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH11, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27875:
		{
			if (FindAffect(AFFECT_FISH12))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH12);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH12))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH12, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27883:
		{
			if (FindAffect(AFFECT_FISH13))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH13);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH13))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH13, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		case 27878:
		{
			if (FindAffect(AFFECT_FISH14))
			{
				if (!bStatus)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					return false;
				}
				RemoveAffect(AFFECT_FISH14);
				return false;
			}
	
			if (FindAffect(AFFECT_FISH14))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				return false;
			}
	
			AddAffect(AFFECT_FISH14, apply_type, apply_value, 0, apply_duration, 0, true);
		}
		break;
		// END OF NEW FISH
	}

	return true;
}
#endif

#if defined(__BLEND_AFFECT__)
bool CHARACTER::SetBlendAffect(LPITEM item)
{
	switch (item->GetVnum())
	{
		case 50830:
			AddAffect(AFFECT_BLEND_MONSTERS, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		// DEWS
		case 50821:
		case 950821:
			AddAffect(AFFECT_BLEND_POTION_1, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		
		case 50822:
		case 950822:
			AddAffect(AFFECT_BLEND_POTION_2, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		
		case 50823:
		case 950823:
			AddAffect(AFFECT_BLEND_POTION_3, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		
		case 50824:
		case 950824:
			AddAffect(AFFECT_BLEND_POTION_4, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		
		case 50825:
		case 950825:
			AddAffect(AFFECT_BLEND_POTION_3, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		
		case 950826:
			AddAffect(AFFECT_BLEND_POTION_6, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
			// END_OF_DEWS
		
			// DRAGON_GOD_MEDALS
		case 39017:
		case 71027:
		case 939017:
			AddAffect(AFFECT_DRAGON_GOD_1, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 39018:
		case 71028:
		case 939018:
			AddAffect(AFFECT_DRAGON_GOD_2, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 39019:
		case 71029:
		case 939019:
			AddAffect(AFFECT_DRAGON_GOD_3, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 39020:
		case 71030:
		case 939020:
			AddAffect(AFFECT_DRAGON_GOD_4, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
			// END_OF_DRAGON_GOD_MEDALS
		
			// CRITICAL_AND_PENETRATION
		case 39024:
		case 71044:
		case 939024:
			AddAffect(AFFECT_CRITICAL, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		
		case 39025:
		case 71045:
		case 939025:
			AddAffect(AFFECT_PENETRATE, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
			// END_OF_CRITICAL_AND_PENETRATION
		
			// ATTACK_AND_MOVE_SPEED
		case 27102:
			AddAffect(AFFECT_ATTACK_SPEED, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
			// END_OF_ATTACK_AND_MOVE_SPEED
		
		case 50817:
		case 95219:
			AddAffect(AFFECT_ATT_WATER, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		
		case 50818:
		case 95220:
			AddAffect(AFFECT_DEF_WATER, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;

		// FISH

		case 27863:
			AddAffect(AFFECT_FISH1, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27864:
			AddAffect(AFFECT_FISH2, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27865:
			AddAffect(AFFECT_FISH3, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27866:
			AddAffect(AFFECT_FISH4, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27867:
			AddAffect(AFFECT_FISH5, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27868:
			AddAffect(AFFECT_FISH6, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27869:
			AddAffect(AFFECT_FISH7, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27870:
			AddAffect(AFFECT_FISH8, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27871:
			AddAffect(AFFECT_FISH9, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27872:
			AddAffect(AFFECT_FISH10, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27873:
			AddAffect(AFFECT_FISH11, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27874:
			if (!PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::Fishes, std::chrono::milliseconds(5000)))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to wait %d s! Try again later."), 5);
				return false;
			}
			AddAffect(AFFECT_FISH12, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27883:
			AddAffect(AFFECT_FISH13, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		case 27878:
			AddAffect(AFFECT_FISH14, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
			break;
		// END FISH

		default:
			return false;
	}

	return true;
}
#endif


int CalculateConsume(LPCHARACTER ch)
{
	static const int WARP_NEED_LIFE_PERCENT	= 30;
	static const int WARP_MIN_LIFE_PERCENT	= 10;
	// CONSUME_LIFE_WHEN_USE_WARP_ITEM
	int consumeLife = 0;
	{
		// CheckNeedLifeForWarp
		const int curLife		= ch->GetHP();
		const int needPercent	= WARP_NEED_LIFE_PERCENT;
		const int needLife = ch->GetMaxHP() * needPercent / 100;
		if (curLife < needLife)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("남은 생명력 양이 모자라 사용할 수 없습니다."));
			return -1;
		}

		consumeLife = needLife;

		const int minPercent	= WARP_MIN_LIFE_PERCENT;
		const int minLife	= ch->GetMaxHP() * minPercent / 100;
		if (curLife - needLife < minLife)
			consumeLife = curLife - minLife;

		if (consumeLife < 0)
			consumeLife = 0;
	}
	// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM
	return consumeLife;
}

int CalculateConsumeSP(LPCHARACTER lpChar)
{
	static const int NEED_WARP_SP_PERCENT = 30;

	const int curSP = lpChar->GetSP();
	const int needSP = lpChar->GetMaxSP() * NEED_WARP_SP_PERCENT / 100;

	if (curSP < needSP)
	{
		lpChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("남은 정신력 양이 모자라 사용할 수 없습니다."));
		return -1;
	}

	return needSP;
}

// #define ENABLE_FIREWORK_STUN
#define ENABLE_ADDSTONE_FAILURE
bool CHARACTER::UseItemEx(LPITEM item, TItemPos DestCell)
{
	int iLimitRealtimeStartFirstUseFlagIndex = -1;
	//int iLimitTimerBasedOnWearFlagIndex = -1;

#ifdef __MAINTENANCE__
	if (CMaintenanceManager::Instance().GetGameMode() == GAME_MODE_LOBBY && !IsGM() && !this->GetDesc()->IsTester())
		return false;
#endif

	WORD wDestCell = DestCell.cell;
	BYTE bDestInven = DestCell.window_type;
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limitValue = item->GetProto()->aLimits[i].lValue;

		switch (item->GetProto()->aLimits[i].bType)
		{
			case LIMIT_LEVEL:
				if (GetLevel() < limitValue)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템의 레벨 제한보다 레벨이 낮습니다."));
					return false;
				}
				break;

			case LIMIT_REAL_TIME_START_FIRST_USE:
				iLimitRealtimeStartFirstUseFlagIndex = i;
				break;

			case LIMIT_TIMER_BASED_ON_WEAR:
				//iLimitTimerBasedOnWearFlagIndex = i;
				break;
		}
	}

	if (test_server)
	{
		sys_log(0, "USE_ITEM %d %s, Inven %d, Cell %d, DestInven %d, DestCell %d, ItemType %d, SubType %d", item->GetVnum(), item->GetName(), item->GetWindow(), item->GetCell(), bDestInven, wDestCell, item->GetType(), item->GetSubType());
	}

	if ( CArenaManager::instance().IsLimitedItem( GetMapIndex(), item->GetVnum() ) == true )
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련 중에는 이용할 수 없는 물품입니다."));
		return false;
	}

	// @fixme402 (IsLoadedAffect to block affect hacking)
	if (!IsLoadedAffect())
	{
		ChatPacket(CHAT_TYPE_INFO, "Affects are not loaded yet!");
		return false;
	}

	// @fixme141 BEGIN
	if (TItemPos(item->GetWindow(), item->GetCell()).IsBeltInventoryPosition())
	{
		LPITEM beltItem = GetWear(WEAR_BELT);

		if (NULL == beltItem)
		{
			ChatPacket(CHAT_TYPE_INFO, "<Belt> You can't use this item if you have no equipped belt.");
			return false;
		}

		if (false == CBeltInventoryHelper::IsAvailableCell(item->GetCell() - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
		{
			ChatPacket(CHAT_TYPE_INFO, "<Belt> You can't use this item if you don't upgrade your belt.");
			return false;
		}
	}
	// @fixme141 END

	switch (item->GetType())
	{
		case ITEM_HAIR:
			return ItemProcess_Hair(item, wDestCell);

		case ITEM_POLYMORPH:
			return ItemProcess_Polymorph(item);

#ifdef __BUFFI_SUPPORT__
		case ITEM_BUFFI:
			switch (item->GetSubType())
			{
				case BUFFI_NAME_SCROLL:
				{
					if (item->GetSIGVnum() == 0)
						quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
					else
						quest::CQuestManager::instance().SIGUse(GetPlayerID(), item->GetSIGVnum(), item, false);
				}
				break;
				case BUFFI_SCROLL:
				{
					const int now = get_global_time();
					if (GetProtectTime("buffi_next_level") > now)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need little bit slow. You need wait %d second."), GetProtectTime("buffi_next_level") - now);
						return false;
					}
					SetProtectTime("buffi_next_level", now + 1);
					BUFFI_MANAGER::Instance().Summon(!item->GetSocket(1), item);
				}
				break;
			}
			break;
#endif

		case ITEM_QUEST:
#ifdef ENABLE_QUEST_DND_EVENT
			if (IS_SET(item->GetFlag(), ITEM_FLAG_APPLICABLE))
			{
				LPITEM item2;

				if (!GetItem(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					return false;

				quest::CQuestManager::instance().DND(GetPlayerID(), item, item2, false);
				return true;
			}
#endif

#ifdef METINSTONES_QUEUE
			switch(item->GetVnum())
			{
				case QUEUE_1:
				case QUEUE_2:
				case QUEUE_3:
				{
					if (item->isLocked() || item->IsExchanging())
						return false;

					if (FindAffect(AFFECT_AUTO_METIN_FARM)) {
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You already have this affect!"));
						return false;
					}

					if (!item->GetSocket(0) && item->GetVnum() != QUEUE_2)
					{
						ChatPacket(CHAT_TYPE_INFO, "[SYSTEM] Contact game master. This item not has time data!");
						return false;
					}

					if (item->GetVnum() == QUEUE_1)
						AddAffect(AFFECT_AUTO_METIN_FARM, POINT_NONE, 0, AFF_NONE, item->GetSocket(0), 0, false);
					else if(item->GetVnum() == QUEUE_2)
						AddAffect(AFFECT_AUTO_METIN_FARM, POINT_NONE, 0, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);
					else if(item->GetVnum() == QUEUE_3)
						AddAffect(AFFECT_AUTO_METIN_FARM, POINT_NONE, 0, AFF_NONE, item->GetSocket(0), 0, false);

					item->SetCount(item->GetCount() - 1);
					return true;
				}
				break;

				case STONE_THIEF_GLOVES:
				case BOSS_THIEF_GLOVES:
				{
					if (item->isLocked() || item->IsExchanging())
						return false;

					if (item->GetVnum() == STONE_THIEF_GLOVES && FindAffect(AFFECT_THIEF_GLOVES_STONES)) {
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You already have this affect!"));
						return false;
					}

					if (item->GetVnum() == BOSS_THIEF_GLOVES && FindAffect(AFFECT_THIEF_GLOVES_BOSSES)) {
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You already have this affect!"));
						return false;
					}

					if (item->GetVnum() == STONE_THIEF_GLOVES)
						AddAffect(AFFECT_THIEF_GLOVES_STONES, POINT_NONE, 0, AFF_NONE, 604800, 0, false);
					else if(item->GetVnum() == BOSS_THIEF_GLOVES)
						AddAffect(AFFECT_THIEF_GLOVES_BOSSES, POINT_NONE, 0, AFF_NONE, 604800, 0, false);

					item->SetCount(item->GetCount() - 1);
					return true;
				}
				break;
			}
#endif
#ifdef __SASH_SKIN__
			if (item->IsDreamSoul())
			{
				LPITEM item2 = GetItem(DestCell);
				if (item2)
				{
					if (item2->IsCostumeSashSkin())
					{

						if (item2->AddDreamSoul(item))
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Successfuly soul added."));
							item->SetCount(0);
							return true;
						}
					}
					else
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't add this item on this"));
				}
				return false;
			}
			else if (item->IsDreamSoulEnchant())
			{
				LPITEM item2 = GetItem(DestCell);
				if (item2)
				{
					if (item2->IsCostumeSashSkin())
					{
						if (item2->ChangeDreamSoulBonus())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Successfuly attribute changed."));
							item->SetCount(item->GetCount() - 1);
							return true;
						}
					}
					else
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't use on this."));
				}
				return false;
			}
#endif

#ifdef __RENEWAL_CRYSTAL__
			if (item->GetVnum() >= 51005 && item->GetVnum() <= 51006)
			{
				LPITEM item2 = GetItem(DestCell);
				if (item2 != NULL)
				{
					const int now = time(0);
					if (GetProtectTime("crystal.last_time_add") > now)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to wait some seconds."));
						return false;
					}
					SetProtectTime("crystal.last_time_add", now + 2);

					if (!item2->IsCrystal())
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can use only on crystal."));
						return false;
					}
					else if (item2->GetSocket(0))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Crystal item need deactive for adding time."));
						return false;
					}

					// new script method
					int leftTime = item2->GetSocket(1);
					if (leftTime < 0)
						leftTime = 0;

					if (leftTime >= (60*60*24)-(60*3))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to wait %d s! Try again later."), (leftTime - ((60*60*24)-(60*3))));
						return false;
					}

					int newLeftTime = ((leftTime + (60*60*item->GetValue(0))) > (60*60*24)) ? 60*60*24 : (leftTime + 60*60*item->GetValue(0));

					item2->SetSocket(1, newLeftTime);
					item->SetCount(item->GetCount() - 1);
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d초 만큼 충전되었습니다."), newLeftTime);
					return true;
				}
				return false;
			}
			else if (item->IsCrystal())
			{
				const int now = time(0);
				if (GetProtectTime("crystal.last_click") > now)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to wait some seconds."));
					return false;
				}
				SetProtectTime("crystal.last_click", now + 3);

				const bool newStatus = item->GetSocket(0) ? false : (item->GetSocket(1) <= 0 ? false : true);

				if (!newStatus)
				{
					if (!item->GetSocket(0))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need add time for you can use."));
						return false;
					}
					while (true)
					{
						CAffect* affect = FindAffect(AFFECT_CRYSTAL);
						if (!affect)
							break;
						RemoveAffect(affect);
					}
					item->SetSocket(0, newStatus);
					item->SetSocket(1, (item->GetSocket(1) - now) > 0 ? item->GetSocket(1) - now : 0);
				}
				else
				{
					int lastTime = item->GetSocket(1);
					if(lastTime > 60)
						lastTime -= 60;
					// ChatPacket(1, "lastTime: %d", lastTime);
					if(lastTime <= 0)
					{
						item->SetSocket(0, 0);
						item->SetSocket(1, 0);
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Not has time!"));
						return false;
					}
					while (true)
					{
						CAffect* affect = FindAffect(AFFECT_CRYSTAL);
						if (!affect)
							break;
						RemoveAffect(affect);
					}
					for (BYTE i = 0; i < Inventory_Size(); ++i)
					{
						LPITEM invItem = GetInventoryItem(i);
						if (invItem && invItem->IsCrystal() && invItem->GetSocket(0))
						{
							invItem->SetSocket(1, (invItem->GetSocket(1) - now) > 0 ? invItem->GetSocket(1) - now : 0);
							invItem->SetSocket(0, 0);;
						}
					}
					item->SetSocket(0, newStatus);
					item->SetSocket(1, now + lastTime);


					AddAffect(AFFECT_CRYSTAL, APPLY_NONE, 0, AFF_NONE, lastTime, 0, false);

					for (BYTE i = 0; i < 5; ++i)
					{
						if (item->GetAttributeType(i) != APPLY_NONE)
							AddAffect(AFFECT_CRYSTAL, aApplyInfo[item->GetAttributeType(i)].bPointType, item->GetAttributeValue(i), AFF_NONE, lastTime, 0, false);
					}
				}
			}
#endif
			if (GetArena() != NULL || IsObserverMode() == true)
			{
				if (item->GetVnum() == 50051 || item->GetVnum() == 50052 || item->GetVnum() == 50053)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련 중에는 이용할 수 없는 물품입니다."));
					return false;
				}
			}
#ifdef RENEWAL_MISSION_BOOKS
			if ((item->GetVnum() >= 50307 && item->GetVnum() <= 50310) || (item->GetVnum() == 50317))
			{
				if ((item->GetVnum() == 50307) && GetLevel() > 90)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can open this quest only if you have level between 1-90 (included 90)."));
					return false;
				}

				if (((item->GetVnum() == 50308) && GetLevel() < 90) && ((item->GetVnum() == 50308) && GetLevel() > 105) )
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can open this quest only if you have level between 90-105 (included 105)."));
					return false;
				}

				if (((item->GetVnum() == 50309) && GetLevel() < 105) && ((item->GetVnum() == 50309) && GetLevel() > 115) )
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can open this quest only if you have level between 105-115 (included 115)."));
					return false;
				}

				if (((item->GetVnum() == 50310) && GetLevel() < 115) && ((item->GetVnum() == 50310) && GetLevel() > 120) )
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can open this quest only if you have level between 115-120 (included 120)."));
					return false;
				}

				CHARACTER_MANAGER::Instance().GiveNewMission(item, this);
				return true;
			}
#endif
#ifdef RENEWAL_PICKUP_AFFECT
			if (item->GetVnum() == ITEM_AUTO_PICK_UP0 || item->GetVnum() == ITEM_AUTO_PICK_UP1 || item->GetVnum() == ITEM_AUTO_PICK_UP2)
			{
				if (item->isLocked() || item->IsExchanging())
					return false;

				CAffect* affect = FindAffect(AFFECT_PICKUP_ENABLE);
				if (!affect)
				{
					affect = FindAffect(AFFECT_PICKUP_DEACTIVE);
					if (affect)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You already have the Auto Loot affect."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You already have the Auto Loot affect."));
					return false;
				}

				if (!item->GetSocket(0) && item->GetVnum() != ITEM_AUTO_PICK_UP1)
				{
					ChatPacket(CHAT_TYPE_INFO, "[SYSTEM] Contact game master. This item not has time data!");
					return false;
				}

				if (item->GetVnum() == ITEM_AUTO_PICK_UP0)
					AddAffect(AFFECT_PICKUP_ENABLE, POINT_NONE, 0, AFF_NONE, item->GetSocket(0), 0, false);
				else if(item->GetVnum() == ITEM_AUTO_PICK_UP1)
					AddAffect(AFFECT_PICKUP_ENABLE, POINT_NONE, 0, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);
				else if(item->GetVnum() == ITEM_AUTO_PICK_UP2)
					AddAffect(AFFECT_PICKUP_ENABLE, POINT_NONE, 0, AFF_NONE, item->GetSocket(0), 0, false);

				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Succesfully added affect."));
				item->SetCount(item->GetCount()-1);
				return true;
			}
#endif
			if (!IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE | ITEM_FLAG_QUEST_USE_MULTIPLE))
			{
				if (item->GetSIGVnum() == 0)
					quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
				else
					quest::CQuestManager::instance().SIGUse(GetPlayerID(), item->GetSIGVnum(), item, false);
			}
			break;

		case ITEM_CAMPFIRE:
			{
				float fx, fy;
				GetDeltaByDegree(GetRotation(), 100.0f, &fx, &fy);

				LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), (long)(GetX()+fx), (long)(GetY()+fy));

				if (!tree)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("모닥불을 피울 수 없는 지점입니다."));
					return false;
				}

				if (tree->IsAttr((long)(GetX()+fx), (long)(GetY()+fy), ATTR_WATER))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("물 속에 모닥불을 피울 수 없습니다."));
					return false;
				}

				LPCHARACTER campfire = CHARACTER_MANAGER::instance().SpawnMob(fishing::CAMPFIRE_MOB, GetMapIndex(), (long)(GetX()+fx), (long)(GetY()+fy), 0, false, number(0, 359));

				char_event_info* info = AllocEventInfo<char_event_info>();

				info->ch = campfire;

				campfire->m_pkMiningEvent = event_create(kill_campfire_event, info, PASSES_PER_SEC(40));

				item->SetCount(item->GetCount() - 1);
			}
			break;

		case ITEM_UNIQUE:
			{

				int affect_type = AFFECT_EXP_BONUS_EURO_FREE;
				int apply_type = aApplyInfo[item->GetValue(0)].bPointType;
				int apply_value = item->GetValue(2);
				int apply_duration = item->GetValue(1);

				switch (item->GetSubType())
				{
					case USE_ABILITY_UP:
						{
							switch (item->GetValue(0))
							{
								case APPLY_MOV_SPEED:
								{
#if defined(__EXTENDED_BLEND_AFFECT__)
									if (FindAffect(AFFECT_MOVE_SPEED))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
#endif
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;
								}
								case APPLY_ATT_SPEED:
								{
#if defined(__EXTENDED_BLEND_AFFECT__)
									if (FindAffect(AFFECT_BLEND_POTION_3))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}

									if (FindAffect(AFFECT_ATTACK_SPEED))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
#endif
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;
								}

								case APPLY_STR:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_DEX:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CON:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_INT:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CAST_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CRITICAL_PCT:
								{
#if defined(__EXTENDED_BLEND_AFFECT__)
									if (FindAffect(AFFECT_BLEND_POTION_1))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}

									if (FindAffect(AFFECT_CRITICAL))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
#endif
									AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, true, true);
									break;
								}

								case APPLY_PENETRATE_PCT:
								{
#if defined(__EXTENDED_BLEND_AFFECT__)
									if (FindAffect(AFFECT_BLEND_POTION_2))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
									if (FindAffect(AFFECT_PENETRATE))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
#endif
									AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, true, true);
									break;
								}

								case APPLY_RESIST_MAGIC:
								{
#if defined(__EXTENDED_BLEND_AFFECT__)
									if (FindAffect(AFFECT_BLEND_POTION_4))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
#endif
									AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, true, true);
									break;
								}

								case APPLY_ATT_GRADE_BONUS:
								{
#if defined(__EXTENDED_BLEND_AFFECT__)
									if (FindAffect(AFFECT_BLEND_POTION_5) || FindAffect(AFFECT_ATT_WATER))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
#endif
									AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, true, true);
									break;
								}

								case APPLY_DEF_GRADE_BONUS:
								{
#if defined(__EXTENDED_BLEND_AFFECT__)
									if (FindAffect(AFFECT_BLEND_POTION_6) || FindAffect(AFFECT_DEF_WATER))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
#endif
									AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, true, true);
									break;
								}
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);item->SetCount(item->GetCount() - 1);
						break;

					default:
						{
#ifdef ENABLE_MULTI_FARM_BLOCK
							if (item->GetVnum() >= 55610 && item->GetVnum() <= 55615)
							{
								if (FindAffect(AFFECT_MULTI_FARM_PREMIUM))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have already this affect!"));
									return false;
								}
								else
								{
									AddAffect(AFFECT_MULTI_FARM_PREMIUM, POINT_NONE, item->GetValue(1), AFF_NONE, item->GetValue(0), 0, false, false);
									item->SetCount(item->GetCount() - 1);
									CHARACTER_MANAGER::Instance().CheckMultiFarmAccount(GetDesc()->GetHostName(), GetPlayerID(), GetName(), GetRewardStatus());
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Affect succesfully added on your character!"));
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("If you want use this affect this character need active drop status!"));
								}
							}
#endif
							if (item->GetSubType() == USE_SPECIAL)
							{
								sys_log(0, "ITEM_UNIQUE: USE_SPECIAL %u", item->GetVnum());

								switch (item->GetVnum())
								{
									case 71049:
										if (g_bEnableBootaryCheck)
										{
											if (IS_BOTARYABLE_ZONE(GetMapIndex()) == true)
											{
												UseSilkBotary();
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("개인 상점을 열 수 없는 지역입니다"));
											}
										}
										else
										{
											UseSilkBotary();
										}
										break;
								}
							}
							else
							{
								if (!item->IsEquipped())
								{
					#ifdef ENABLE_ANTI_CMD_FLOOD
									if (!PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::EquipItems, std::chrono::milliseconds(500)))
										return false;
					#endif	
									EquipItem(item);
								}

								else
								{
					#ifdef ENABLE_ANTI_CMD_FLOOD
									if (!PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::EquipItems, std::chrono::milliseconds(500)))
										return false;
					#endif
									UnequipItem(item);
								}
							}
						}
						break;
				}
			}
			break;

		case ITEM_COSTUME:
		case ITEM_WEAPON:
		case ITEM_ARMOR:
		case ITEM_ROD:
		case ITEM_RING:
		case ITEM_BELT:
			// MINING
		case ITEM_PICK:
			// END_OF_MINING
			if (!item->IsEquipped())
			{
#ifdef ENABLE_ANTI_CMD_FLOOD
				if (!PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::EquipItems, std::chrono::milliseconds(500)))
					return false;
#endif	
				EquipItem(item);
			}
				
			else
			{
#ifdef ENABLE_ANTI_CMD_FLOOD
				if (!PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::EquipItems, std::chrono::milliseconds(500)))
					return false;
#endif
				UnequipItem(item);
			}
			break;

		case ITEM_DS:
			{
				if (!item->IsEquipped())
					return false;
				return DSManager::instance().PullOut(this, NPOS, item);
			}
			break;
		case ITEM_SPECIAL_DS:
			if (!item->IsEquipped())
			{
#ifdef ENABLE_ANTI_CMD_FLOOD
				if (!PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::EquipItems, std::chrono::milliseconds(500)))
					return false;
#endif	
				EquipItem(item);
			}
				
			else
			{
#ifdef ENABLE_ANTI_CMD_FLOOD
				if (!PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::EquipItems, std::chrono::milliseconds(500)))
					return false;
#endif
				UnequipItem(item);
			}

#ifdef ENABLE_NEW_PET_SYSTEM
		case ITEM_PET:
		{
			switch (item->GetSubType())
			{
			case PET_LEVELABLE:
			{
				time_t now = get_global_time();
				if (GetProtectTime("newpet.ride") > now)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("684 %d"), GetProtectTime("newpet.ride") - now);
					return false;
				}
				SetProtectTime("newpet.ride", now + 1);
		
				if (time(0) > item->GetSocket(POINT_PET_DURATION))
				{
					TItemTable* p = ITEM_MANAGER::instance().GetTable(55001);
					if(p)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("719 %s"), LC_ITEM_NAME(55001, GetLanguage()));
					if (item->IsEquipped())
						UnequipItem(item);
					return false;
				}
		
				if (!item->IsEquipped())
				{
	#ifdef ENABLE_ANTI_CMD_FLOOD
					if (!PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::EquipItems, std::chrono::milliseconds(500)))
						return false;
	#endif	
					EquipItem(item);
				}

				else
				{
	#ifdef ENABLE_ANTI_CMD_FLOOD
					if (!PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::EquipItems, std::chrono::milliseconds(500)))
						return false;
	#endif
					UnequipItem(item);
				}
			}
			break;
		
			case PET_PROTEIN:
			{
				LPITEM item2 = GetItem(DestCell);
				if (item2 != NULL)
				{
					if (!(item2->GetType() == ITEM_PET && item2->GetSubType() == PET_LEVELABLE))
						return false;
					long oldTime = item2->GetSocket(POINT_PET_DURATION) - time(0);

					if (oldTime < 0)
						oldTime = 0;

					if (oldTime >= (60*60*24*7))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("732"));
						return false;
					}
					else
					{
						long newTime = 60*60*24*7;
						item2->SetSocket(POINT_PET_DURATION, time(0) + newTime);
						item->SetCount(item->GetCount() - 1);
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("720 %d"), 7);
					}
				}
				else
				{
					CPetSystem* pet = GetPetSystem();
					if (pet)
					{
						LPPET petActor = pet->GetNewPet();
						if (petActor) {
							if (petActor->PointChange(POINT_PET_DURATION, 60*60*24*7))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("720 %d"), 7);
								item->SetCount(item->GetCount() - 1);
							}
							else
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("732"));
								return false;
							}
						}
						else
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("721"));
					}
				}
			}
			break;

			case PET_DEW:
			{
				LPITEM item2 = GetItem(DestCell);
				if (item2 != NULL)
				{
					if (!(item2->GetType() == ITEM_PET && item2->GetSubType() == PET_LEVELABLE))
						return false;

					BYTE bonusType = item->GetValue(0);
					BYTE bonusStep = item->GetValue(1);

					DWORD petLevel = item2->GetSocket(POINT_PET_LEVEL);
					long bonusLevel = item2->GetSocket(POINT_PET_BONUS_1 + bonusType);

					if (bonusStep == 1)
					{
						if (bonusLevel >= 0 && bonusLevel <= 49)
						{
							item2->SetSocket(POINT_PET_BONUS_1 + bonusType, bonusLevel + 1);
							item->SetCount(item->GetCount() - 1);
							return true;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("722"));
							return false;
						}
					}
					else if (bonusStep == 2)
					{
						if (petLevel < 40)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("718 %d"), 40);
							return false;
						}

						if (bonusLevel >= 50 && bonusLevel <= 124)
						{
							item2->SetSocket(POINT_PET_BONUS_1 + bonusType, bonusLevel + 1);
							item->SetCount(item->GetCount() - 1);
							return true;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("722"));
							return false;
						}

					}
					else if (bonusStep == 3)
					{
						if (petLevel < 75)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("718 %d"), 75);
							return false;
						}

						if (bonusLevel >= 125 && bonusLevel <= 224)
						{
							item2->SetSocket(POINT_PET_BONUS_1 + bonusType, bonusLevel + 1);
							item->SetCount(item->GetCount() - 1);
							return true;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("722"));
							return false;
						}
					}
					else if (bonusStep == 4)
					{
						if (petLevel < 100)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("718 %d"), 100);
							return false;
						}

						if (bonusLevel >= 225 && bonusLevel <= 349)
						{
							item2->SetSocket(POINT_PET_BONUS_1 + bonusType, bonusLevel + 1);
							item->SetCount(item->GetCount() - 1);
							return true;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("722"));
							return false;
						}
					}
				}
				else
				{
					CPetSystem* pet = GetPetSystem();
					if (pet)
					{
						LPPET petActor = pet->GetNewPet();
						if (petActor) {
							if (petActor->IncreaseBonus(item->GetValue(0), item->GetValue(1)))
								item->SetCount(item->GetCount() - 1);
						}
						else
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("721"));
					}
				}
			}
			break;
		
			case PET_SKILL:
			{
				LPITEM item2 = GetItem(DestCell);
				if (item2 != NULL)
				{
					if (!(item2->GetType() == ITEM_PET && item2->GetSubType() == PET_LEVELABLE))
						return false;
		
					BYTE skillIndex = item->GetValue(0);
		
					BYTE bySlotIndex = 99;
					for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
					{
						if (item2->GetAttributeType(j) == skillIndex) {
							bySlotIndex = j;
							break;
						}
					}
					if (bySlotIndex == 99)
					{
						BYTE emptyIndex = 99;
						for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
						{
							if (item2->GetAttributeType(j) == 0) {
								emptyIndex = j;
								break;
							}
						}
						if (emptyIndex == 99)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("723"));
							return false;
						}
		
						item2->SetForceAttribute(emptyIndex, skillIndex, 0);
						TItemTable* p = ITEM_MANAGER::instance().GetTable(55009+skillIndex);
						if(p)
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("724 %s"), item->GetLocaleName(GetLanguage()));
						item->SetCount(item->GetCount() - 1);
						return true;
					}
					else
					{
						BYTE type = item2->GetAttributeType(bySlotIndex);
						long value = item2->GetAttributeValue(bySlotIndex);
						if (value > 9)
						{
							TItemTable* p = ITEM_MANAGER::instance().GetTable(55009+skillIndex);
							if(p)
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("725 %s"), item->GetLocaleName(GetLanguage()));
							return false;
						}
						item2->SetForceAttribute(bySlotIndex, type, value + 1);
						TItemTable* p = ITEM_MANAGER::instance().GetTable(55009+skillIndex);
						if(p)
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("726 %s"), item->GetLocaleName(GetLanguage()));
						item->SetCount(item->GetCount() - 1);
					}
				}
				else
				{
					CPetSystem* pet = GetPetSystem();
					if (pet)
					{
						LPPET petActor = pet->GetNewPet();
						if (petActor) {
							if (petActor->IncreaseSkill(item->GetValue(0)))
								item->SetCount(item->GetCount() - 1);
						}
						else
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("721"));
					}
				}
			}
			break;
		
			case PET_SKILL_DEL_BOOK:
			{
				WORD index = DestCell.cell;
				if (index >= 0 && index <= 15)
				{
					CPetSystem* pet = GetPetSystem();
					if (pet)
					{
						LPPET petActor = pet->GetNewPet();
						if (petActor) {
							LPITEM petItem = petActor->GetSummonItem();
							if (!petItem)
								return false;
							BYTE type = petItem->GetAttributeType(index);
							if (type >= 1 && type <= 25)
							{
								petActor->PointChange(POINT_PET_SKILL_INDEX_1 + index, 0);
								petActor->PointChange(POINT_PET_SKILL_LEVEL_1 + index, 0);
								TItemTable* p = ITEM_MANAGER::instance().GetTable(55009+type);
								if(p)
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("727 %s"), item->GetLocaleName(GetLanguage()));
								item->SetCount(item->GetCount() - 1);
							}
						}
						else
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("721"));
					}
				}
			}
			break;
			case PET_EXPFOOD_PER:
			{
				CPetSystem* pet = GetPetSystem();
				if (pet)
				{
					LPPET petActor = pet->GetNewPet();
					if (petActor) {
						float realExp = (float(petActor->GetNextExp()) / 100.0) * 2.0;
						if (petActor->PointChange(POINT_PET_EXP, int(realExp)))
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("734"));
							item->SetCount(item->GetCount() - 1);
						}
						else
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("735"));
					}
					else
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("721"));
				}
			}
			break;
			case PET_SKILL_ALL_DEL_BOOK:
			{
				LPITEM item2 = GetItem(DestCell);
				if (item2 != NULL)
				{
					if (!(item2->GetType() == ITEM_PET && item2->GetSubType() == PET_LEVELABLE))
						return false;
					bool isHaveSkill = false;
					for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
					{
						BYTE type = item2->GetAttributeType(j);
						if (type != 0 && type != 99)
						{
							isHaveSkill = true;
							break;
						}
					}
					if (!isHaveSkill)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("728"));
						return false;
					}
					else
					{
						for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
						{
							BYTE type = item2->GetAttributeType(j);
							if (type != 0 && type != 99)
								item2->SetForceAttribute(j, 0, 0);
						}
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("729"));
						item->SetCount(item->GetCount() - 1);
					}
				}
				else
				{
					CPetSystem* pet = GetPetSystem();
					if (pet)
					{
						LPPET petActor = pet->GetNewPet();
						if (petActor)
						{
							LPITEM petItem = petActor->GetSummonItem();
							if (!petItem)
								return false;
							bool isHaveSkill = false;
							for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
							{
								BYTE type = petItem->GetAttributeType(j);
								if (type != 0 && type != 99)
								{
									isHaveSkill = true;
									break;
								}
							}
							if (!isHaveSkill)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("728"));
								return false;
							}
							else
							{
								for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
								{
									BYTE type = petItem->GetAttributeType(j);
									if (type != 0 && type != 99)
										petItem->SetForceAttribute(j, 0, 0);
								}
								ChatPacket(CHAT_TYPE_COMMAND, "UpdatePet %d", POINT_PET_SKILL_INDEX_1);
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("729"));
								item->SetCount(item->GetCount() - 1);
							}
						}
						else
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("721"));
					}
				}
			}
			break;
			}
		}
		break;
#endif

		case ITEM_FISH:
			{
				if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련 중에는 이용할 수 없는 물품입니다."));
					return false;
				}

				if (item->GetSubType() == FISH_ALIVE)
					fishing::UseFish(this, item);
			}
			break;

		case ITEM_TREASURE_BOX:
			{
				return false;

			}
			break;

		case ITEM_TREASURE_KEY:
			{
				LPITEM item2;

				if (!GetItem(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					return false;

				if (item2->GetType() != ITEM_TREASURE_BOX)
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("열쇠로 여는 물건이 아닌것 같다."));
					return false;
				}

				if (item->GetValue(0) == item2->GetValue(0))
				{
					DWORD dwBoxVnum = item2->GetVnum();
					std::vector <DWORD> dwVnums;
					std::vector <DWORD> dwCounts;
					std::vector <LPITEM> item_gets(0);
					int count = 0;

					if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
					{
#ifdef ENABLE_BATTLE_PASS
						CHARACTER_MANAGER::Instance().DoMission(this, MISSION_DESTROY_ITEM, item->GetCount(), item->GetVnum());
#endif
						ITEM_MANAGER::instance().RemoveItem(item);
						ITEM_MANAGER::instance().RemoveItem(item2);

						for (int i = 0; i < count; i++){
							switch (dwVnums[i])
							{
								case CSpecialItemGroup::GOLD:
									#if defined(__CHATTING_WINDOW_RENEWAL__)
									ChatPacket(CHAT_TYPE_MONEY_INFO, LC_TEXT("돈 %d 냥을 획득했습니다."), dwCounts[i]);
									#else
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("돈 %d 냥을 획득했습니다."), dwCounts[i]);
									#endif				
									break;
								case CSpecialItemGroup::EXP:
									#if defined(__CHATTING_WINDOW_RENEWAL__)
									ChatPacket(CHAT_TYPE_EXP_INFO, LC_TEXT("상자에서 부터 신비한 빛이 나옵니다."));
									ChatPacket(CHAT_TYPE_EXP_INFO, LC_TEXT("%d의 경험치를 획득했습니다."), dwCounts[i]);
									#else
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 부터 신비한 빛이 나옵니다."));
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d의 경험치를 획득했습니다."), dwCounts[i]);
									#endif
									break;
								case CSpecialItemGroup::MOB:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 몬스터가 나타났습니다!"));
									break;
								case CSpecialItemGroup::SLOW:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 나온 빨간 연기를 들이마시자 움직이는 속도가 느려졌습니다!"));
									break;
								case CSpecialItemGroup::DRAIN_HP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자가 갑자기 폭발하였습니다! 생명력이 감소했습니다."));
									break;
								case CSpecialItemGroup::POISON:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 나온 녹색 연기를 들이마시자 독이 온몸으로 퍼집니다!"));
									break;
#ifdef ENABLE_WOLFMAN_CHARACTER
								case CSpecialItemGroup::BLEEDING:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 나온 녹색 연기를 들이마시자 독이 온몸으로 퍼집니다!"));
									break;
#endif
								case CSpecialItemGroup::MOB_GROUP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 몬스터가 나타났습니다!"));
									break;
								default:
									if (item_gets[i])
									{
										if (dwCounts[i] > 1)
											ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("상자에서 %s 가 %d 개 나왔습니다."), item_gets[i]->GetLocaleName(GetLanguage()), dwCounts[i]);
										else
											ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("상자에서 %s 가 나왔습니다."), item_gets[i]->GetLocaleName(GetLanguage()));
									}
							}
						}
					}
					else
					{
						ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("열쇠가 맞지 않는 것 같다."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("열쇠가 맞지 않는 것 같다."));
					return false;
				}
			}
			break;

		case ITEM_GIFTBOX:
			OpenChestItem(item);
			break;

		case ITEM_SKILLFORGET:
			{
				if (!item->GetSocket(0))
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					return false;
				}

				DWORD dwVnum = item->GetSocket(0);

				if (SkillLevelDown(dwVnum))
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("스킬 레벨을 내리는데 성공하였습니다."));
				}
				else
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("스킬 레벨을 내릴 수 없습니다."));
			}
			break;

		case ITEM_SKILLBOOK:
			{
				if (IsPolymorphed())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("변신중에는 책을 읽을수 없습니다."));
					return false;
				}

				DWORD dwVnum = 0;

				if (item->GetVnum() == 50300)
				{
					dwVnum = item->GetSocket(0);
				}
				else
				{
					dwVnum = item->GetValue(0);
				}

				if (0 == dwVnum)
				{
					ITEM_MANAGER::instance().RemoveItem(item);

					return false;
				}

				if (true == LearnSkillByBook(dwVnum))
				{
#ifdef ENABLE_BOOKS_STACKFIX
					item->SetCount(item->GetCount() - 1);
#else
					ITEM_MANAGER::instance().RemoveItem(item);
#endif

					int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

					if (distribution_test_server)
						iReadDelay /= 3;

					SetSkillNextReadTime(dwVnum, get_global_time() + iReadDelay);
				}
			}
			break;

		case ITEM_USE:
			{
				if (item->GetVnum() > 50800 && item->GetVnum() <= 50820)
				{
					if (test_server)
						sys_log (0, "ADD addtional effect : vnum(%d) subtype(%d)", item->GetOriginalVnum(), item->GetSubType());

					int affect_type = AFFECT_EXP_BONUS_EURO_FREE;
					int apply_type = aApplyInfo[item->GetValue(0)].bPointType;
					int apply_value = item->GetValue(2);
					int apply_duration = item->GetValue(1);

					switch (item->GetSubType())
					{
						case USE_ABILITY_UP:
							if (FindAffect(affect_type, apply_type))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
								return false;
							}

							{
								switch (item->GetValue(0))
								{
									case APPLY_MOV_SPEED:
										AddAffect(affect_type, apply_type, apply_value, AFF_MOV_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case APPLY_ATT_SPEED:
										AddAffect(affect_type, apply_type, apply_value, AFF_ATT_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case APPLY_STR:
									case APPLY_DEX:
									case APPLY_CON:
									case APPLY_INT:
									case APPLY_CAST_SPEED:
									case APPLY_RESIST_MAGIC:
									case APPLY_ATT_GRADE_BONUS:
									case APPLY_DEF_GRADE_BONUS:
										AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, true, true);
										break;
								}
							}

							if (GetDungeon())
								GetDungeon()->UsePotion(this);

							if (GetWarMap())
								GetWarMap()->UsePotion(this, item);

							item->SetCount(item->GetCount() - 1);
							break;

					case USE_AFFECT:
						{
#if defined(__EXTENDED_BLEND_AFFECT__)
							for (int i = AFFECT_DRAGON_GOD_1; i <= AFFECT_DRAGON_GOD_4; ++i)
							{
								if (FindAffect(i, aApplyInfo[item->GetValue(1)].bPointType))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
									return false;
								}
							}
#endif
							if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[item->GetValue(1)].bPointType))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
							}
							else
							{
								AddAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false, true);
								item->SetCount(item->GetCount() - 1);
							}
						}
						break;

					case USE_POTION_NODELAY:
						{
							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("사용 제한량을 초과하였습니다."));
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
										return false;
										break;
								}
							}

							bool used = false;

							if (item->GetValue(0) != 0)
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0)
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0)
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("월병 또는 종자 를 사용하였습니다"));
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;
					}

					return true;
				}

				if (item->GetVnum() >= 27863 && item->GetVnum() <= 27883)
				{
					if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련 중에는 이용할 수 없는 물품입니다."));
						return false;
					}
				}

				if (test_server)
				{
					 sys_log (0, "USE_ITEM %s Type %d SubType %d vnum %d", item->GetName(), item->GetType(), item->GetSubType(), item->GetOriginalVnum());
				}

				switch (item->GetSubType())
				{
					case USE_TIME_CHARGE_PER:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (NULL == pDestItem)
							{
								return false;
							}

							if (pDestItem->IsDragonSoul())
							{
								int ret;
								char buf[128];
								ret = pDestItem->GiveMoreTime_Per((float)item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
								if (ret > 0)
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "Inc %ds by item{VN:%d SOC%d:%ld}", ret, item->GetVnum(), ITEM_SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "Inc %ds by item{VN:%d VAL%d:%ld}", ret, item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d초 만큼 충전되었습니다."), ret);
									item->SetCount(item->GetCount() - 1);
									return true;
								}
								else
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "No change by item{VN:%d SOC%d:%ld}", item->GetVnum(), ITEM_SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "No change by item{VN:%d VAL%d:%ld}", item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("충전할 수 없습니다."));
									return false;
								}
							}
							else
								return false;
						}
						break;
					case USE_TIME_CHARGE_FIX:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (NULL == pDestItem)
							{
								return false;
							}

							if (pDestItem->IsDragonSoul())
							{
								int ret = pDestItem->GiveMoreTime_Fix(item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
								char buf[128];
								if (ret)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d초 만큼 충전되었습니다."), ret);
									item->SetCount(item->GetCount() - 1);
									return true;
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("충전할 수 없습니다."));
									return false;
								}
							}
							else
								return false;
						}
						break;
					case USE_SPECIAL:

						switch (item->GetVnum())
						{
							case ITEM_NOG_POCKET:
								{
									if (FindAffect(AFFECT_NOG_ABILITY))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case ITEM_RAMADAN_CANDY:
								{
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_RAMADAN_ABILITY))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
									// @fixme147 END
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;
							case ITEM_MARRIAGE_RING:
								{
									marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());
									if (pMarriage)
									{
										if (pMarriage->ch1 != NULL)
										{
											if (CArenaManager::instance().IsArenaMap(pMarriage->ch1->GetMapIndex()) == true)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련 중에는 이용할 수 없는 물품입니다."));
												break;
											}
										}

										if (pMarriage->ch2 != NULL)
										{
											if (CArenaManager::instance().IsArenaMap(pMarriage->ch2->GetMapIndex()) == true)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련 중에는 이용할 수 없는 물품입니다."));
												break;
											}
										}

										int consumeSP = CalculateConsumeSP(this);

										if (consumeSP < 0)
											return false;

										PointChange(POINT_SP, -consumeSP, false);

										WarpToPID(pMarriage->GetOther(GetPlayerID()));
									}
									else
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("결혼 상태가 아니면 결혼반지를 사용할 수 없습니다."));
								}
								break;
							case 501007:
							case 501008:
							case 501009:
								if (item->isLocked() || item->IsExchanging())
									return false;
#ifdef ENABLE_OFFLINESHOP_SYSTEM
								if(FindAffect(AFFECT_DECORATION))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("OFFLINE_SHOP_ALREADY_HAVE_DECORATION"));
									return false;
								}
								else
								{
									if (!item->GetSocket(0) && item->GetVnum() != 501008)
									{
										ChatPacket(CHAT_TYPE_INFO, "[SYSTEM] Contact game master. This item not has time data!");
										return false;
									}

									if (item->GetVnum() == 501007)
									{
										AddAffect(AFFECT_DECORATION, POINT_NONE, 0, AFF_NONE, item->GetSocket(0), 0, false);
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("OFFLINE_SHOP_DECORATION_DAY %d"),item->GetValue(0));
									}
									else if(item->GetVnum() == 501008)
									{
										AddAffect(AFFECT_DECORATION, POINT_NONE, 0, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Succesfully added affect."));
									}
									else if(item->GetVnum() == 501009)
									{
										AddAffect(AFFECT_DECORATION, POINT_NONE, 0, AFF_NONE, item->GetSocket(0), 0, false);
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("OFFLINE_SHOP_DECORATION_DAY %d"),item->GetValue(0));
									}
									item->SetCount(item->GetCount()-1);
								}
								break;
#endif

							case UNIQUE_ITEM_CAPE_OF_COURAGE:
							case 70057:
							{
#if defined(__REQUEST_MONEY_FOR_BRAVERY_CAPE__)
								if (GetLevel() > 30 && GetLevel() < 61) {
									if (GetGold() < LOW_COST_CAPE) {
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough money."));
										return false;
									}
								}
								if (GetLevel() > 60 && GetLevel() < 91) {
									if (GetGold() < MEDIUM_COST_CAPE) {
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough money."));
										return false;
									}
								}
								if (GetLevel() > 90 && GetLevel() < 121) {
									if (GetGold() < HIGH_COST_CAPE) {
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough money."));
										return false;
									}
								}
#endif						
								AggregateMonster();
								AttractRanger();
#if defined(__REQUEST_MONEY_FOR_BRAVERY_CAPE__)
								if (GetLevel() > 30 && GetLevel() < 61) {
									PointChange(POINT_GOLD, -LOW_COST_CAPE, true);
								}
								if (GetLevel() > 60 && GetLevel() < 91) {
									PointChange(POINT_GOLD, -MEDIUM_COST_CAPE, true);
								}
								if (GetLevel() > 90 && GetLevel() < 121) {
									PointChange(POINT_GOLD, -HIGH_COST_CAPE, true);
								}
#endif
							}
							break;
#ifdef __RENEWAL_BRAVE_CAPE__
							case BRAVE_CAPE_1:
							case BRAVE_CAPE_2:
							case BRAVE_CAPE_3:
							{
								if (item->isLocked() || item->IsExchanging())
									return false;

								if (FindAffect(AFFECT_BRAVE_CAPE))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You already have this affect!"));
									return false;
								}

								if (!item->GetSocket(0) && item->GetVnum() != BRAVE_CAPE_2)
								{
									ChatPacket(CHAT_TYPE_INFO, "[SYSTEM] Contact game master. This item not has time data!");
									return false;
								}

								if (item->GetVnum() == BRAVE_CAPE_1)
									AddAffect(AFFECT_BRAVE_CAPE, POINT_NONE, 0, AFF_NONE, item->GetSocket(0), 0, false);
								else if(item->GetVnum() == BRAVE_CAPE_2)
									AddAffect(AFFECT_BRAVE_CAPE, POINT_NONE, 0, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);
								else if(item->GetVnum() == BRAVE_CAPE_3)
									AddAffect(AFFECT_BRAVE_CAPE, POINT_NONE, 0, AFF_NONE, item->GetSocket(0), 0, false);
								item->SetCount(item->GetCount() - 1);
								return true;
							}
							break;
							
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
							case ACCE_REVERSAL_VNUM_1:
							case ACCE_REVERSAL_VNUM_2:
							{
								LPITEM item2;
								if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
									return false;

								if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
									return false;

								if (!CleanAcceAttr(item, item2))
									return false;
								item->SetCount(item->GetCount()-1);
								break;
							}
#endif

							case UNIQUE_ITEM_WHITE_FLAG:
								ForgetMyAttacker();
								item->SetCount(item->GetCount()-1);
								break;

							case UNIQUE_ITEM_TREASURE_BOX:
								break;

							case 30093:
							case 30094:
							case 30095:
							case 30096:

								{
									const int MAX_BAG_INFO = 26;
									static struct LuckyBagInfo
									{
										DWORD count;
										int prob;
										DWORD vnum;
									} b1[MAX_BAG_INFO] =
									{
										{ 1000,	302,	1 },
										{ 10,	150,	27002 },
										{ 10,	75,	27003 },
										{ 10,	100,	27005 },
										{ 10,	50,	27006 },
										{ 10,	80,	27001 },
										{ 10,	50,	27002 },
										{ 10,	80,	27004 },
										{ 10,	50,	27005 },
										{ 1,	10,	50300 },
										{ 1,	6,	92 },
										{ 1,	2,	132 },
										{ 1,	6,	1052 },
										{ 1,	2,	1092 },
										{ 1,	6,	2082 },
										{ 1,	2,	2122 },
										{ 1,	6,	3082 },
										{ 1,	2,	3122 },
										{ 1,	6,	5052 },
										{ 1,	2,	5082 },
										{ 1,	6,	7082 },
										{ 1,	2,	7122 },
										{ 1,	1,	11282 },
										{ 1,	1,	11482 },
										{ 1,	1,	11682 },
										{ 1,	1,	11882 },
									};

									LuckyBagInfo * bi = NULL;
									bi = b1;

									int pct = number(1, 1000);

									int i;
									for (i=0;i<MAX_BAG_INFO;i++)
									{
										if (pct <= bi[i].prob)
											break;
										pct -= bi[i].prob;
									}
									if (i>=MAX_BAG_INFO)
										return false;

									if (bi[i].vnum == 50300)
									{
										GiveRandomSkillBook();
									}
									else if (bi[i].vnum == 1)
									{
										PointChange(POINT_GOLD, 1000, true);
									}
									else
									{
										AutoGiveItem(bi[i].vnum, bi[i].count);
									}
									ITEM_MANAGER::instance().RemoveItem(item);
								}
								break;

							case 50004:
								{
									if (item->GetSocket(0))
									{
										item->SetSocket(0, item->GetSocket(0) + 1);
									}
									else
									{
										int iMapIndex = GetMapIndex();

										PIXEL_POSITION pos;

										if (SECTREE_MANAGER::instance().GetRandomLocation(iMapIndex, pos, 700))
										{
											item->SetSocket(0, 1);
											item->SetSocket(1, pos.x);
											item->SetSocket(2, pos.y);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 곳에선 이벤트용 감지기가 동작하지 않는것 같습니다."));
											return false;
										}
									}

									int dist = 0;
									float distance = (DISTANCE_SQRT(GetX()-item->GetSocket(1), GetY()-item->GetSocket(2)));

									if (distance < 1000.0f)
									{	
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이벤트용 감지기가 신비로운 빛을 내며 사라집니다."));

										struct TEventStoneInfo
										{
											DWORD dwVnum;
											int count;
											int prob;
										};
										const int EVENT_STONE_MAX_INFO = 15;
										TEventStoneInfo info_10[EVENT_STONE_MAX_INFO] =
										{
											{ 27001, 10,  8 },
											{ 27004, 10,  6 },
											{ 27002, 10, 12 },
											{ 27005, 10, 12 },
											{ 27100,  1,  9 },
											{ 27103,  1,  9 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 12 },

											{ 25040,  1,  4 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1,  0 },

											{ 50001,  1,  0 },
											{ 50003,  1,  1 },
										};
										TEventStoneInfo info_7[EVENT_STONE_MAX_INFO] =
										{
											{ 27001, 10,  1 },
											{ 27004, 10,  1 },
											{ 27004, 10,  9 },
											{ 27005, 10,  9 },
											{ 27100,  1,  5 },
											{ 27103,  1,  5 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 14 },

											{ 25040,  1,  5 },

											{ 27410,  1,  5 },
											{ 27600,  1,  5 },
											{ 25100,  1,  5 },

											{ 50001,  1,  0 },
											{ 50003,  1,  5 },

										};
										TEventStoneInfo info_4[EVENT_STONE_MAX_INFO] =
										{
											{ 27001, 10,  0 },
											{ 27004, 10,  0 },
											{ 27002, 10,  0 },
											{ 27005, 10,  0 },
											{ 27100,  1,  0 },
											{ 27103,  1,  0 },
											{ 27101,  1,  0 },
											{ 27104,  1,  0 },
											{ 27999,  1, 25 },

											{ 25040,  1,  0 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1, 15 },

											{ 50001,  1, 10 },
											{ 50003,  1, 50 },

										};

										{
											TEventStoneInfo* info;
											if (item->GetSocket(0) <= 4)
												info = info_4;
											else if (item->GetSocket(0) <= 7)
												info = info_7;
											else
												info = info_10;

											int prob = number(1, 100);

											for (int i = 0; i < EVENT_STONE_MAX_INFO; ++i)
											{
												if (!info[i].prob)
													continue;

												if (prob <= info[i].prob)
												{
													AutoGiveItem(info[i].dwVnum, info[i].count);
													break;
												}
												prob -= info[i].prob;
											}
										}

										char chatbuf[CHAT_MAX_LEN + 1];
										int len = snprintf(chatbuf, sizeof(chatbuf), "StoneDetect %u 0 0", (DWORD)GetVID());

										if (len < 0 || len >= (int) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;

										TPacketGCChat pack_chat;
										pack_chat.header	= HEADER_GC_CHAT;
										pack_chat.size		= sizeof(TPacketGCChat) + len;
										pack_chat.type		= CHAT_TYPE_COMMAND;
										pack_chat.id		= 0;
										pack_chat.bEmpire	= GetDesc()->GetEmpire();
										//pack_chat.id	= vid;

										TEMP_BUFFER buf;
										buf.write(&pack_chat, sizeof(TPacketGCChat));
										buf.write(chatbuf, len);

										PacketAround(buf.read_peek(), buf.size());

										ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 1");
										return true;
									}
									else if (distance < 20000)
										dist = 1;
									else if (distance < 70000)
										dist = 2;
									else
										dist = 3;

									const int STONE_DETECT_MAX_TRY = 10;
									if (item->GetSocket(0) >= STONE_DETECT_MAX_TRY)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이벤트용 감지기가 흔적도 없이 사라집니다."));
										ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 0");
										AutoGiveItem(27002);
										return true;
									}

									if (dist)
									{
										char chatbuf[CHAT_MAX_LEN + 1];
										int len = snprintf(chatbuf, sizeof(chatbuf),
												"StoneDetect %u %d %d",
											   	(DWORD)GetVID(), dist, (int)GetDegreeFromPositionXY(GetX(), item->GetSocket(2), item->GetSocket(1), GetY()));

										if (len < 0 || len >= (int) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;

										TPacketGCChat pack_chat;
										pack_chat.header	= HEADER_GC_CHAT;
										pack_chat.size		= sizeof(TPacketGCChat) + len;
										pack_chat.type		= CHAT_TYPE_COMMAND;
										pack_chat.id		= 0;
										pack_chat.bEmpire	= GetDesc()->GetEmpire();
										//pack_chat.id		= vid;

										TEMP_BUFFER buf;
										buf.write(&pack_chat, sizeof(TPacketGCChat));
										buf.write(chatbuf, len);

										PacketAround(buf.read_peek(), buf.size());
									}

								}
								break;

							case 27989:
							case 76006:
								{
									LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());

									if (pMap != NULL)
									{
										item->SetSocket(0, item->GetSocket(0) + 1);

										FFindStone f;

										// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
										pMap->for_each(f);

										if (f.m_mapStone.size() > 0)
										{
											std::map<DWORD, LPCHARACTER>::iterator stone = f.m_mapStone.begin();

											DWORD max = UINT_MAX;
											LPCHARACTER pTarget = stone->second;

											while (stone != f.m_mapStone.end())
											{
												DWORD dist = (DWORD)DISTANCE_SQRT(GetX()-stone->second->GetX(), GetY()-stone->second->GetY());

												if (dist != 0 && max > dist)
												{
													max = dist;
													pTarget = stone->second;
												}
												stone++;
											}

											if (pTarget != NULL)
											{
												int val = 3;

												if (max < 10000) val = 2;
												else if (max < 70000) val = 1;

												ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u %d %d", (DWORD)GetVID(), val,
														(int)GetDegreeFromPositionXY(GetX(), pTarget->GetY(), pTarget->GetX(), GetY()));
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("감지기를 작용하였으나 감지되는 영석이 없습니다."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("감지기를 작용하였으나 감지되는 영석이 없습니다."));
										}

										if (item->GetSocket(0) >= 6)
										{
											ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u 0 0", (DWORD)GetVID());
											ITEM_MANAGER::instance().RemoveItem(item);
										}
									}
									break;
								}
								break;

							case 27996:
								item->SetCount(item->GetCount() - 1);
								AttackedByPoison(NULL); // @warme008
								break;

							case 27987:

								{
									item->SetCount(item->GetCount() - 1);

									int r = number(1, 100);

									if (r <= 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("조개에서 돌조각이 나왔습니다."));
										AutoGiveItem(27990);
									}
									else
									{
										const int prob_table_gb2312[] =
										{
											95, 97, 99
										};

										const int * prob_table = prob_table_gb2312;

										if (r <= prob_table[0])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("조개가 흔적도 없이 사라집니다."));
										}
										else if (r <= prob_table[1])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("조개에서 백진주가 나왔습니다."));
											AutoGiveItem(27992);
										}
										else if (r <= prob_table[2])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("조개에서 청진주가 나왔습니다."));
											AutoGiveItem(27993);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("조개에서 피진주가 나왔습니다."));
											AutoGiveItem(27994);
										}
									}
								}
								break;

							case 71013:
								CreateFly(number(FLY_FIREWORK1, FLY_FIREWORK6), this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50100:
							case 50101:
							case 50102:
							case 50103:
							case 50104:
							case 50105:
							case 50106:
								CreateFly(item->GetVnum() - 50100 + FLY_FIREWORK1, this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50200:
								if (g_bEnableBootaryCheck)
								{
									if (IS_BOTARYABLE_ZONE(GetMapIndex()) == true)
									{
										__OpenPrivateShop();
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("개인 상점을 열 수 없는 지역입니다"));
									}
								}
								else
								{
									__OpenPrivateShop();
								}
								break;

							case fishing::FISH_MIND_PILL_VNUM:
								AddAffect(AFFECT_FISH_MIND_PILL, POINT_NONE, 0, AFF_FISH_MIND, 20*60, 0, true);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50301:
							case 50302:
							case 50303:
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑 중에는 능력을 올릴 수 없습니다."));
										return false;
									}

									int lv = GetSkillLevel(SKILL_LEADERSHIP);

									if (lv < item->GetValue(0))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 책은 너무 어려워 이해하기가 힘듭니다."));
										return false;
									}

									if (lv >= item->GetValue(1))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 책은 아무리 봐도 도움이 될 것 같지 않습니다."));
										return false;
									}

									if (LearnSkillByBook(SKILL_LEADERSHIP))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(SKILL_LEADERSHIP, get_global_time() + iReadDelay);
									}
								}
								break;


							case 951050:
							case 951051:
							case 951052:
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑 중에는 능력을 올릴 수 없습니다."));
										return false;
									}

									int lv = GetSkillLevel(171);

									if (lv < item->GetValue(0))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 책은 너무 어려워 이해하기가 힘듭니다."));
										return false;
									}

									if (lv >= item->GetValue(1))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 책은 아무리 봐도 도움이 될 것 같지 않습니다."));
										return false;
									}

									if (LearnSkillByBook(171))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(171, get_global_time() + iReadDelay);
									}
								}
								break;

							case 951053:
							case 951054:
							case 951055:
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑 중에는 능력을 올릴 수 없습니다."));
										return false;
									}

									int lv = GetSkillLevel(168);

									if (lv < item->GetValue(0))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 책은 너무 어려워 이해하기가 힘듭니다."));
										return false;
									}

									if (lv >= item->GetValue(1))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 책은 아무리 봐도 도움이 될 것 같지 않습니다."));
										return false;
									}

									if (LearnSkillByBook(168))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(168, get_global_time() + iReadDelay);
									}
								}
								break;

							case 951056:
							case 951057:
							case 951058:
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑 중에는 능력을 올릴 수 없습니다."));
										return false;
									}

									int lv = GetSkillLevel(169);

									if (lv < item->GetValue(0))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 책은 너무 어려워 이해하기가 힘듭니다."));
										return false;
									}

									if (lv >= item->GetValue(1))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 책은 아무리 봐도 도움이 될 것 같지 않습니다."));
										return false;
									}

									if (LearnSkillByBook(169))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(169, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50304:
							case 50305:
							case 50306:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("변신중에는 책을 읽을수 없습니다."));
										return false;

									}
									if (GetSkillLevel(SKILL_COMBO) == 0 && GetLevel() < 30)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("레벨 30이 되기 전에는 습득할 수 있을 것 같지 않습니다."));
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) == 1 && GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("레벨 50이 되기 전에는 습득할 수 있을 것 같지 않습니다."));
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) >= 2)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("연계기는 더이상 수련할 수 없습니다."));
										return false;
									}

									int iPct = item->GetValue(0);

									if (LearnSkillByBook(SKILL_COMBO, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(SKILL_COMBO, get_global_time() + iReadDelay);
									}
								}
								break;
							case 50311:
							case 50312:
							case 50313:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("변신중에는 책을 읽을수 없습니다."));
										return false;

									}
									DWORD dwSkillVnum = item->GetValue(0);
									int iPct = MINMAX(0, item->GetValue(1), 100);
									if (GetSkillLevel(dwSkillVnum)>=20 || dwSkillVnum-SKILL_LANGUAGE1+1 == GetEmpire())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 완벽하게 알아들을 수 있는 언어이다."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50061 :
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("변신중에는 책을 읽을수 없습니다."));
										return false;

									}
									DWORD dwSkillVnum = item->GetValue(0);
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum) >= 10)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("더 이상 수련할 수 없습니다."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50314: case 50315: case 50316:
							case 50323: case 50324:
							case 50325: case 50326:
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑 중에는 능력을 올릴 수 없습니다."));
										return false;
									}

									int iSkillLevelLowLimit = item->GetValue(0);
									int iSkillLevelHighLimit = item->GetValue(1);
									int iPct = MINMAX(0, item->GetValue(2), 100);
									int iLevelLimit = item->GetValue(3);
									DWORD dwSkillVnum = 0;

									switch (item->GetVnum())
									{
										case 50314: case 50315: case 50316:
											dwSkillVnum = SKILL_POLYMORPH;
											break;

										case 50323: case 50324:
											dwSkillVnum = SKILL_ADD_HP;
											break;

										case 50325: case 50326:
											dwSkillVnum = SKILL_RESIST_PENETRATE;
											break;

										default:
											return false;
									}

									if (0 == dwSkillVnum)
										return false;

									if (GetLevel() < iLevelLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 책을 읽으려면 레벨을 더 올려야 합니다."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= 40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("더 이상 수련할 수 없습니다."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) < iSkillLevelLowLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 책은 너무 어려워 이해하기가 힘듭니다."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= iSkillLevelHighLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 책으로는 더 이상 수련할 수 없습니다."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50902:
							case 50903:
							case 50904:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("변신중에는 책을 읽을수 없습니다."));
										return false;

									}
									DWORD dwSkillVnum = SKILL_CREATE;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("더 이상 수련할 수 없습니다."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);

										if (test_server)
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Success to learn skill ");
										}
									}
									else
									{
										if (test_server)
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Failed to learn skill ");
										}
									}
								}
								break;

								// MINING
							case ITEM_MINING_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("변신중에는 책을 읽을수 없습니다."));
										return false;

									}
									DWORD dwSkillVnum = SKILL_MINING;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("더 이상 수련할 수 없습니다."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;
								// END_OF_MINING

							case ITEM_HORSE_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("변신중에는 책을 읽을수 없습니다."));
										return false;

									}
									DWORD dwSkillVnum = SKILL_HORSE;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아직 승마 스킬을 수련할 수 있는 레벨이 아닙니다."));
										return false;
									}

									if (!test_server && get_global_time() < GetSkillNextReadTime(dwSkillVnum))
									{
										if (FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
										{
											RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("주안술서를 통해 주화입마에서 빠져나왔습니다."));
										}
										else
										{
											SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
											return false;
										}
									}

									if (GetPoint(POINT_HORSE_SKILL) >= 20 ||
											GetSkillLevel(SKILL_HORSE_WILDATTACK) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60 ||
											GetSkillLevel(SKILL_HORSE_WILDATTACK_RANGE) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("더 이상 승마 수련서를 읽을 수 없습니다."));
										return false;
									}

									if (number(1, 100) <= iPct)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("승마 수련서를 읽어 승마 스킬 포인트를 얻었습니다."));
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("얻은 포인트로는 승마 스킬의 레벨을 올릴 수 있습니다."));
										PointChange(POINT_HORSE_SKILL, 1);

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										if (!test_server)
											SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("승마 수련서 이해에 실패하였습니다."));
									}
#ifdef ENABLE_BOOKS_STACKFIX
									item->SetCount(item->GetCount() - 1);
#else
									ITEM_MANAGER::instance().RemoveItem(item);
#endif
								}
								break;

							case 70102:
							case 70103:
								{
									if (GetAlignment() >= 0)
										return false;

									int delta = MIN(-GetAlignment(), item->GetValue(0));

									sys_log(0, "%s ALIGNMENT ITEM %d", GetName(), delta);

									UpdateAlignment(delta);
									item->SetCount(item->GetCount() - 1);

									if (delta / 10 > 0)
									{
										ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("마음이 맑아지는군. 가슴을 짓누르던 무언가가 좀 가벼워진 느낌이야."));
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("선악치가 %d 증가하였습니다."), delta/10);
									}
								}
								break;

							case 71107:
							case 39032: // @fixme169 mythical peach alternative vnum
								{
									int val = item->GetValue(0);
									int interval = item->GetValue(1);
									quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID());
									if (!pPC) // @fixme169 missing check
										return false;
									int last_use_time = pPC->GetFlag("mythical_peach.last_use_time");

									if (get_global_time() - last_use_time < interval * 60 * 60)
									{
										if (test_server == false)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아직 사용할 수 없습니다."));
											return false;
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("테스트 서버 시간제한 통과"));
										}
									}

									if (GetAlignment() == 200000)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("선악치를 더 이상 올릴 수 없습니다."));
										return false;
									}

									if (200000 - GetAlignment() < val * 10)
									{
										val = (200000 - GetAlignment()) / 10;
									}

									int old_alignment = GetAlignment() / 10;

									UpdateAlignment(val*10);

									item->SetCount(item->GetCount()-1);
									pPC->SetFlag("mythical_peach.last_use_time", get_global_time());

									ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("마음이 맑아지는군. 가슴을 짓누르던 무언가가 좀 가벼워진 느낌이야."));
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("선악치가 %d 증가하였습니다."), val);
								}
								break;

							case 71109:
							case 72719:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
										return false;

									if (item2->GetSocketCount() == 0)
										return false;

									switch( item2->GetType())
									{
										case ITEM_WEAPON:
											break;
										case ITEM_ARMOR:
											switch (item2->GetSubType())
											{
												case ARMOR_EAR:
												case ARMOR_WRIST:
												case ARMOR_NECK:
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("빼낼 영석이 없습니다"));
													return false;
											}
											break;

										default:
											return false;
									}

									std::stack<long> socket;

									for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										socket.push(item2->GetSocket(i));

									int idx = ITEM_SOCKET_MAX_NUM - 1;

									while (socket.size() > 0)
									{
										if (socket.top() > 2 && socket.top() != ITEM_BROKEN_METIN_VNUM)
											break;

										idx--;
										socket.pop();
									}

									if (socket.size() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("빼낼 영석이 없습니다"));
										return false;
									}

									LPITEM pItemReward = AutoGiveItem(socket.top());

									if (pItemReward != NULL)
									{
										item2->SetSocket(idx, 1);
										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case 70201:
							case 70202:
							case 70203:
							case 70204:
							case 70205:
							case 70206:
								{
									// NEW_HAIR_STYLE_ADD
									if (GetPart(PART_HAIR) >= 1001)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("현재 헤어스타일에서는 염색과 탈색이 불가능합니다."));
									}
									// END_NEW_HAIR_STYLE_ADD
									else
									{
										quest::CQuestManager& q = quest::CQuestManager::instance();
										quest::PC* pPC = q.GetPC(GetPlayerID());

										if (pPC)
										{
											int last_dye_level = pPC->GetFlag("dyeing_hair.last_dye_level");

											if (last_dye_level == 0 ||
													last_dye_level+3 <= GetLevel() ||
													item->GetVnum() == 70201)
											{
												SetPart(PART_HAIR, item->GetVnum() - 70201);

												if (item->GetVnum() == 70201)
													pPC->SetFlag("dyeing_hair.last_dye_level", 0);
												else
													pPC->SetFlag("dyeing_hair.last_dye_level", GetLevel());

												item->SetCount(item->GetCount() - 1);
												UpdatePacket();
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d 레벨이 되어야 다시 염색하실 수 있습니다."), last_dye_level+3);
											}
										}
									}
								}
								break;

							case ITEM_NEW_YEAR_GREETING_VNUM:
								{
									DWORD dwBoxVnum = ITEM_NEW_YEAR_GREETING_VNUM;
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets;
									int count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
									{
										for (int i = 0; i < count; i++)
										{
											if (dwVnums[i] == CSpecialItemGroup::GOLD)
												#if defined(__CHATTING_WINDOW_RENEWAL__)
												ChatPacket(CHAT_TYPE_MONEY_INFO, LC_TEXT("돈 %d 냥을 획득했습니다."), dwCounts[i]);
												#else
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("돈 %d 냥을 획득했습니다."), dwCounts[i]);
												#endif
										}

										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case ITEM_VALENTINE_ROSE:
							case ITEM_VALENTINE_CHOCOLATE:
								{
									DWORD dwBoxVnum = item->GetVnum();
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int count = 0;

									if (((item->GetVnum() == ITEM_VALENTINE_ROSE) && (SEX_MALE==GET_SEX(this))) ||
										((item->GetVnum() == ITEM_VALENTINE_CHOCOLATE) && (SEX_FEMALE==GET_SEX(this))))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("성별이 맞지않아 이 아이템을 열 수 없습니다."));
										return false;
									}

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
									{
										item->SetCount(item->GetCount()-1);
									}
								}
								break;

							case ITEM_WHITEDAY_CANDY:
							case ITEM_WHITEDAY_ROSE:
								{
									DWORD dwBoxVnum = item->GetVnum();
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int count = 0;

									if (((item->GetVnum() == ITEM_WHITEDAY_CANDY) && (SEX_MALE==GET_SEX(this))) ||
										((item->GetVnum() == ITEM_WHITEDAY_ROSE) && (SEX_FEMALE==GET_SEX(this))))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("성별이 맞지않아 이 아이템을 열 수 없습니다."));
										return false;
									}

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
									{
										item->SetCount(item->GetCount()-1);
									}	
					
								}
								break;

							case 50011:
								{
									DWORD dwBoxVnum = 50011;
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
									{
										for (int i = 0; i < count; i++)
										{
											char buf[50 + 1];
											snprintf(buf, sizeof(buf), "%u %u", dwVnums[i], dwCounts[i]);

											//ITEM_MANAGER::instance().RemoveItem(item);
											item->SetCount(item->GetCount() - 1);

											switch (dwVnums[i])
											{
											case CSpecialItemGroup::GOLD:
												#if defined(__CHATTING_WINDOW_RENEWAL__)
												ChatPacket(CHAT_TYPE_MONEY_INFO, LC_TEXT("돈 %d 냥을 획득했습니다."), dwCounts[i]);
												#else
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("돈 %d 냥을 획득했습니다."), dwCounts[i]);
												#endif
												break;

											case CSpecialItemGroup::EXP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 부터 신비한 빛이 나옵니다."));
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d의 경험치를 획득했습니다."), dwCounts[i]);
												break;

											case CSpecialItemGroup::MOB:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 몬스터가 나타났습니다!"));
												break;

											case CSpecialItemGroup::SLOW:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 나온 빨간 연기를 들이마시자 움직이는 속도가 느려졌습니다!"));
												break;

											case CSpecialItemGroup::DRAIN_HP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자가 갑자기 폭발하였습니다! 생명력이 감소했습니다."));
												break;

											case CSpecialItemGroup::POISON:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 나온 녹색 연기를 들이마시자 독이 온몸으로 퍼집니다!"));
												break;
#ifdef ENABLE_WOLFMAN_CHARACTER
											case CSpecialItemGroup::BLEEDING:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 나온 녹색 연기를 들이마시자 독이 온몸으로 퍼집니다!"));
												break;
#endif
											case CSpecialItemGroup::MOB_GROUP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 몬스터가 나타났습니다!"));
												break;

											default:
												if (item_gets[i])
												{
													#if defined(__CHATTING_WINDOW_RENEWAL__)
													if (dwCounts[i] > 1)
														ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("상자에서 %s 가 %d 개 나왔습니다."), item_gets[i]->GetLocaleName(GetLanguage()), dwCounts[i]);
													else
														ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("상자에서 %s 가 나왔습니다."), item_gets[i]->GetLocaleName(GetLanguage()));
													#else
													if (dwCounts[i] > 1)
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 %s 가 %d 개 나왔습니다."), item_gets[i]->GetName(), dwCounts[i]);
													else
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 %s 가 나왔습니다."), item_gets[i]->GetName());
													#endif
												}
												break;
											}
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("아무것도 얻을 수 없었습니다."));
										return false;
									}
								}
								break;

							case ITEM_GIVE_STAT_RESET_COUNT_VNUM:
								{
									//PointChange(POINT_GOLD, -iCost);
									PointChange(POINT_STAT_RESET_COUNT, 1);
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50107:
								{
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련 중에는 이용할 수 없는 물품입니다."));
										return false;
									}

									EffectPacket(SE_CHINA_FIREWORK);
#ifdef ENABLE_FIREWORK_STUN

									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
#endif
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50108:
								{
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련 중에는 이용할 수 없는 물품입니다."));
										return false;
									}

									EffectPacket(SE_SPIN_TOP);
#ifdef ENABLE_FIREWORK_STUN

									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
#endif
									item->SetCount(item->GetCount()-1);
								}
								break;

							case ITEM_WONSO_BEAN_VNUM:
								PointChange(POINT_HP, GetMaxHP() - GetHP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_SUGAR_VNUM:
								PointChange(POINT_SP, GetMaxSP() - GetSP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_FRUIT_VNUM:
								PointChange(POINT_STAMINA, GetMaxStamina()-GetStamina());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_ELK_VNUM:
								{
									int iGold = item->GetSocket(0);
									ITEM_MANAGER::instance().RemoveItem(item);

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("돈 %d 냥을 획득했습니다."), iGold);
									PointChange(POINT_GOLD, iGold);
								}
								break;

							case 70021:
								{
									int HealPrice = quest::CQuestManager::instance().GetEventFlag("MonarchHealGold");
									if (HealPrice == 0)
										HealPrice = 2000000;

									if (CMonarch::instance().HealMyEmpire(this, HealPrice))
									{
										char szNotice[256];
										snprintf(szNotice, sizeof(szNotice), LC_TEXT("군주의 축복으로 이지역 %s 유저는 HP,SP가 모두 채워집니다."), EMPIRE_NAME(GetEmpire()));
										SendNoticeMap(szNotice, GetMapIndex(), false);

										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주의 축복을 사용하였습니다."));
									}
								}
								break;

							case 27995:
								{
								}
								break;

							case 71092 :
								{
									if (m_pkChrTarget != NULL)
									{
										if (m_pkChrTarget->IsPolymorphed())
										{
											m_pkChrTarget->SetPolymorph(0);
											m_pkChrTarget->RemoveAffect(AFFECT_POLYMORPH);
										}
									}
									else
									{
										if (IsPolymorphed())
										{
											SetPolymorph(0);
											RemoveAffect(AFFECT_POLYMORPH);
										}
									}
								}
								break;

#if !defined(__BL_67_ATTR__)
							case 71051 : // 진재가
								{
									// 유럽, 싱가폴, 베트남 진재가 사용금지
									if (LC_IsEurope() || LC_IsSingapore() || LC_IsVietnam())
										return false;

									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetInventoryItem(wDestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경할 수 없는 아이템입니다."));
										return false;
									}

									if (item2->AddRareAttribute() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("성공적으로 속성이 추가 되었습니다"));

										int iAddedIdx = item2->GetRareAttrCount() + 4;
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										LogManager::instance().ItemLog(
												GetPlayerID(),
												item2->GetAttributeType(iAddedIdx),
												item2->GetAttributeValue(iAddedIdx),
												item->GetID(),
												"ADD_RARE_ATTR",
												buf,
												GetDesc()->GetHostName(),
												item->GetOriginalVnum());

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("더 이상 이 아이템으로 속성을 추가할 수 없습니다"));
									}
								}
								break;
#endif

#if !defined(__BL_67_ATTR__)
							case 71052 : // 진재경
								{
									// 유럽, 싱가폴, 베트남 진재가 사용금지
									if (LC_IsEurope() || LC_IsSingapore() || LC_IsVietnam())
										return false;

									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경할 수 없는 아이템입니다."));
										return false;
									}

									if (item2->ChangeRareAttribute() == true)
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_RARE_ATTR", buf);

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("변경 시킬 속성이 없습니다"));
									}
								}
								break;
#endif
							case ITEM_AUTO_HP_RECOVERY_S:
							case ITEM_AUTO_HP_RECOVERY_M:
							case ITEM_AUTO_HP_RECOVERY_L:
							case ITEM_AUTO_HP_RECOVERY_X:
							case ITEM_AUTO_SP_RECOVERY_S:
							case ITEM_AUTO_SP_RECOVERY_M:
							case ITEM_AUTO_SP_RECOVERY_L:
							case ITEM_AUTO_SP_RECOVERY_X:

							case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
							case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
							case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
							case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
							case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
							case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
								{
									if (!PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::UsePotions, std::chrono::milliseconds(5000)))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to wait %d s! Try again later."), 5);
										return false;
									}

									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
										return false;
									}

									EAffectTypes type = AFFECT_NONE;
									bool isSpecialPotion = false;

									switch (item->GetVnum())
									{
										case ITEM_AUTO_HP_RECOVERY_X:
											isSpecialPotion = false;

										case ITEM_AUTO_HP_RECOVERY_S:
										case ITEM_AUTO_HP_RECOVERY_M:
										case ITEM_AUTO_HP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
											type = AFFECT_AUTO_HP_RECOVERY;
											break;

										case ITEM_AUTO_SP_RECOVERY_X:
											isSpecialPotion = false;

										case ITEM_AUTO_SP_RECOVERY_S:
										case ITEM_AUTO_SP_RECOVERY_M:
										case ITEM_AUTO_SP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
											type = AFFECT_AUTO_SP_RECOVERY;
											break;
									}

									if (AFFECT_NONE == type)
										break;

									if (item->GetCount() > 1)
									{
										int pos = GetEmptyInventory(item->GetSize());

										if (-1 == pos)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지품에 빈 공간이 없습니다."));
											break;
										}

										item->SetCount( item->GetCount() - 1 );

										LPITEM item2 = ITEM_MANAGER::instance().CreateItem( item->GetVnum(), 1 );
										item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

										if (item->GetSocket(1) != 0)
										{
											item2->SetSocket(1, item->GetSocket(1));
										}

										item = item2;
									}

									CAffect* pAffect = FindAffect( type );

									if (NULL == pAffect)
									{
										EPointTypes bonus = POINT_NONE;

										if (true == isSpecialPotion)
										{
											if (type == AFFECT_AUTO_HP_RECOVERY)
											{
												bonus = POINT_MAX_HP_PCT;
											}
											else if (type == AFFECT_AUTO_SP_RECOVERY)
											{
												bonus = POINT_MAX_SP_PCT;
											}
										}

										AddAffect( type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

										item->Lock(true);
										item->SetSocket(0, true);

										AutoRecoveryItemProcess( type );
									}
									else
									{
										if (item->GetID() == pAffect->dwFlag)
										{
											RemoveAffect( pAffect );

											item->Lock(false);
											item->SetSocket(0, false);
										}
										else
										{
											LPITEM old = FindItemByID( pAffect->dwFlag );

											if (NULL != old)
											{
												old->Lock(false);
												old->SetSocket(0, false);
											}

											RemoveAffect( pAffect );

											EPointTypes bonus = POINT_NONE;

											if (true == isSpecialPotion)
											{
												if (type == AFFECT_AUTO_HP_RECOVERY)
												{
													bonus = POINT_MAX_HP_PCT;
												}
												else if (type == AFFECT_AUTO_SP_RECOVERY)
												{
													bonus = POINT_MAX_SP_PCT;
												}
											}

											AddAffect( type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

											item->Lock(true);
											item->SetSocket(0, true);

											AutoRecoveryItemProcess( type );
										}
									}
								}
								break;
						}
						break;

					case USE_CLEAR:
						{
							switch (item->GetVnum())
							{
#ifdef ENABLE_WOLFMAN_CHARACTER
								case 27124: // Bandage
									RemoveBleeding();
									break;
#endif
								case 27874: // Grilled Perch
									if (!PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::Fishes, std::chrono::milliseconds(5000)))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to wait %d s! Try again later."), 5);
										return false;
									}
									RemoveBadAffect();
									break;
									
								default:
									RemoveBadAffect();
									break;
							}
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_INVISIBILITY:
						{
							if (item->GetVnum() == 70026)
							{
								quest::CQuestManager& q = quest::CQuestManager::instance();
								quest::PC* pPC = q.GetPC(GetPlayerID());

								if (pPC != NULL)
								{
									int last_use_time = pPC->GetFlag("mirror_of_disapper.last_use_time");

									if (get_global_time() - last_use_time < 10*60)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아직 사용할 수 없습니다."));
										return false;
									}

									pPC->SetFlag("mirror_of_disapper.last_use_time", get_global_time());
								}
							}

							AddAffect(AFFECT_INVISIBILITY, POINT_NONE, 0, AFF_INVISIBILITY, 300, 0, true);
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_POTION_NODELAY:
						{
							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("사용 제한량을 초과하였습니다."));
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
										return false;
								}
							}

							bool used = false;

							if (item->GetValue(0) != 0)
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0)
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0)
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("월병 또는 종자 를 사용하였습니다"));
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;

					case USE_POTION:
						if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
						{
							if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
								return false;
							}

							switch (item->GetVnum())
							{
								case 27001 :
								case 27002 :
								case 27003 :
								case 27004 :
								case 27005 :
								case 27006 :
									if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
									{
										if (m_nPotionLimit <= 0)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("사용 제한량을 초과하였습니다."));
											return false;
										}
									}
									break;

								default :
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
									return false;
							}
						}

						if (item->GetValue(1) != 0)
						{
							if (GetPoint(POINT_SP_RECOVERY) + GetSP() >= GetMaxSP())
							{
								return false;
							}

							PointChange(POINT_SP_RECOVERY, item->GetValue(1) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							StartAffectEvent();
							EffectPacket(SE_SPUP_BLUE);
						}

						if (item->GetValue(0) != 0)
						{
							if (GetPoint(POINT_HP_RECOVERY) + GetHP() >= GetMaxHP())
							{
								return false;
							}

							PointChange(POINT_HP_RECOVERY, item->GetValue(0) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							StartAffectEvent();
							EffectPacket(SE_HPUP_RED);
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						m_nPotionLimit--;
						break;

					case USE_POTION_CONTINUE:
						{
							if (item->GetValue(0) != 0)
							{
								AddAffect(AFFECT_HP_RECOVER_CONTINUE, POINT_HP_RECOVER_CONTINUE, item->GetValue(0), 0, item->GetValue(2), 0, true);
							}
							else if (item->GetValue(1) != 0)
							{
								AddAffect(AFFECT_SP_RECOVER_CONTINUE, POINT_SP_RECOVER_CONTINUE, item->GetValue(1), 0, item->GetValue(2), 0, true);
							}
							else
								return false;
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					case USE_ABILITY_UP:
						{
							switch (item->GetValue(0))
							{
								case APPLY_MOV_SPEED:
#if defined(__EXTENDED_BLEND_AFFECT__)
									if (FindAffect(AFFECT_MOVE_SPEED))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
#endif
									if (FindAffect(AFFECT_MOV_SPEED))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
									AddAffect(AFFECT_MOV_SPEED, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true);
#ifdef ENABLE_EFFECT_EXTRAPOT
									EffectPacket(SE_DXUP_PURPLE);
#endif
									break;

								case APPLY_ATT_SPEED:
#if defined(__EXTENDED_BLEND_AFFECT__)
									if (FindAffect(AFFECT_ATTACK_SPEED))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
#endif

									if (FindAffect(AFFECT_ATT_SPEED))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
										return false;
									}
									AddAffect(AFFECT_ATT_SPEED, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true);
#ifdef ENABLE_EFFECT_EXTRAPOT
									EffectPacket(SE_SPEEDUP_GREEN);
#endif
									break;

								case APPLY_STR:
									AddAffect(AFFECT_STR, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_DEX:
									AddAffect(AFFECT_DEX, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_CON:
									AddAffect(AFFECT_CON, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_INT:
									AddAffect(AFFECT_INT, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_CAST_SPEED:
									AddAffect(AFFECT_CAST_SPEED, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_ATT_GRADE_BONUS:
									AddAffect(AFFECT_ATT_GRADE, POINT_ATT_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_DEF_GRADE_BONUS:
									AddAffect(AFFECT_DEF_GRADE, POINT_DEF_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					case USE_TALISMAN:
						{
							const int TOWN_PORTAL	= 1;
							const int MEMORY_PORTAL = 2;

							if (GetMapIndex() == 200 || GetMapIndex() == 113)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("현재 위치에서 사용할 수 없습니다."));
								return false;
							}

							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련 중에는 이용할 수 없는 물품입니다."));
								return false;
							}

							if (m_pkWarpEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이동할 준비가 되어있음으로 귀환부를 사용할수 없습니다"));
								return false;
							}

							// CONSUME_LIFE_WHEN_USE_WARP_ITEM
							int consumeLife = CalculateConsume(this);

							if (consumeLife < 0)
								return false;
							// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

							if (item->GetValue(0) == TOWN_PORTAL)
							{
								if (item->GetSocket(0) == 0)
								{
									if (!GetDungeon())
										if (!GiveRecallItem(item))
											return false;

									PIXEL_POSITION posWarp;

									if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp))
									{
										// CONSUME_LIFE_WHEN_USE_WARP_ITEM
										PointChange(POINT_HP, -consumeLife, false);
										// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

										WarpSet(posWarp.x, posWarp.y);
									}
									else
									{
										sys_err("CHARACTER::UseItem : cannot find spawn position (name %s, %d x %d)", GetName(), GetX(), GetY());
									}
								}
								else
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("원래 위치로 복귀"));

									ProcessRecallItem(item);
								}
							}
							else if (item->GetValue(0) == MEMORY_PORTAL)
							{
								if (item->GetSocket(0) == 0)
								{
									if (GetDungeon())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("던전 안에서는 %s%s 사용할 수 없습니다."), item->GetLocaleName(GetLanguage()),
											g_iUseLocale ? "" : (under_han(item->GetLocaleName(GetLanguage())) ? LC_TEXT("을") : LC_TEXT("를")));
										return false;
									}

									if (!GiveRecallItem(item))
										return false;
								}
								else
								{
									// CONSUME_LIFE_WHEN_USE_WARP_ITEM
									PointChange(POINT_HP, -consumeLife, false);
									// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

									ProcessRecallItem(item);
								}
							}
						}
						break;

					case USE_TUNING:
					case USE_DETACHMENT:
						{
							LPITEM item2;

							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
								return false;

							if (PreventTradeWindow(WND_ALL))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("창고,거래창등이 열린 상태에서는 개량을 할수가 없습니다"));
								return false;
							}

							if (item2->GetVnum() >= 28330 && item2->GetVnum() <= 28343)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("+3 영석은 이 아이템으로 개량할 수 없습니다"));
								return false;
							}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
							if (item->GetValue(0) == ACCE_CLEAN_ATTR_VALUE0 && item->GetVnum() != 51003 || item->GetVnum() == ACCE_REVERSAL_VNUM_1 || item->GetVnum() == ACCE_REVERSAL_VNUM_2)
							{
								if (!CleanAcceAttr(item, item2))
									return false;
									
								item->SetCount(item->GetCount()-1);
								return true;
							}
#endif

							if (item2->GetVnum() >= 28430 && item2->GetVnum() <= 28443)
							{
								if (item->GetVnum() == 71056)
								{
									RefineItem(item, item2);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("영석은 이 아이템으로 개량할 수 없습니다"));
								}
							}
							else
							{
								RefineItem(item, item2);
							}
						}
						break;

					case USE_CHANGE_COSTUME_ATTR:
					case USE_RESET_COSTUME_ATTR:
						{
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}

							if (ITEM_COSTUME != item2->GetType())
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경할 수 없는 아이템입니다."));
								return false;
							}

							if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
								return false;

							if (item2->GetAttributeSetIndex() == -1)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경할 수 없는 아이템입니다."));
								return false;
							}

							if (item2->GetAttributeCount() == 0)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("변경할 속성이 없습니다."));
								return false;
							}

							switch (item->GetSubType())
							{
								case USE_CHANGE_COSTUME_ATTR:
									item2->ChangeAttribute();
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
									}
									break;
								case USE_RESET_COSTUME_ATTR:
									item2->ClearAttribute();
									item2->AlterToMagicItem();
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
									}
									break;
							}

							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경하였습니다."));

							item->SetCount(item->GetCount() - 1);
							break;
						}

						//  ACCESSORY_REFINE & ADD/CHANGE_ATTRIBUTES
					case USE_PUT_INTO_BELT_SOCKET:
					case USE_PUT_INTO_RING_SOCKET:
					case USE_PUT_INTO_ACCESSORY_SOCKET:
					case USE_ADD_ACCESSORY_SOCKET:
					case USE_CLEAN_SOCKET:
					case USE_CHANGE_ATTRIBUTE:
					case USE_CHANGE_ATTRIBUTE2 :
					case USE_ADD_ATTRIBUTE:
					case USE_ADD_ATTRIBUTE2:
						{
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}

#if defined(__BL_67_ATTR__)
							if ((ITEM_COSTUME == item2->GetType() && item->GetVnum() < 401001) || (ITEM_COSTUME == item2->GetType() && item->GetVnum() > 403019))
#else
							if (ITEM_COSTUME == item2->GetType())
#endif
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경할 수 없는 아이템입니다."));
								return false;
							}

							if (item2->IsExchanging() || item2->IsEquipped())
								return false;

							switch (item->GetSubType())
							{
								case USE_CLEAN_SOCKET:
									{
										int i;
										for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										{
											if (item2->GetSocket(i) == ITEM_BROKEN_METIN_VNUM)
												break;
										}

										if (i == ITEM_SOCKET_MAX_NUM)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("청소할 석이 박혀있지 않습니다."));
											return false;
										}

										int j = 0;

										for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										{
											if (item2->GetSocket(i) != ITEM_BROKEN_METIN_VNUM && item2->GetSocket(i) != 0)
												item2->SetSocket(j++, item2->GetSocket(i));
										}

										for (; j < ITEM_SOCKET_MAX_NUM; ++j)
										{
											if (item2->GetSocket(j) > 0)
												item2->SetSocket(j, 1);
										}

										{
											char buf[21];
											snprintf(buf, sizeof(buf), "%u", item2->GetID());
										}

										item->SetCount(item->GetCount() - 1);

									}
									break;

								case USE_CHANGE_ATTRIBUTE :
#ifdef ENABLE_SPECIAL_COSTUME_ATTR
									{
										const DWORD bonusItemVnum = item->GetVnum();
										bool itsCostumeAttr = (bonusItemVnum >= 401001 && bonusItemVnum <= 403019);

										// daca e ranfosare ?i nu poate bonusa costumele
										if ((itsCostumeAttr == true && !item2->CanBonusCostume()))
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't use on this item!"));
											return false;
										}

										// Remove last attr
										if (bonusItemVnum == 71304)
										{
											bool isChange = false;
											for (int j = MAX_NORM_ATTR_NUM - 1; j >= 0; --j)
											{
												if (item2->GetAttributeType(j) != APPLY_NONE)
												{
													isChange = true;
													item->SetCount(item->GetCount() - 1);
													item2->SetForceAttribute(j, 0, 0);
													break;
												}
											}
											if (isChange)
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Successfully removed last attr!"));
											else
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Dont have any attr in item!"));
											return true;
										}
										// Remove all attr
										else if (bonusItemVnum == 71305)
										{
											bool isChange = false;
											for (int j = 0; j < MAX_NORM_ATTR_NUM; ++j)
											{
												if (item2->GetAttributeType(j) != APPLY_NONE)
												{
													isChange = true;
													item2->SetForceAttribute(j, 0, 0);
												}
											}
											if (isChange)
											{
												item->SetCount(item->GetCount() - 1);
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Successfully removed all attr!"));
											}
											else
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Dont have any attr in item!"));
											return true;
										}
										// Boosters
										else if (bonusItemVnum >= 401001 && bonusItemVnum <= 403019)
										{
											const BYTE applyType = item->GetValue(0);
											const long applyValue = item->GetValue(1);
											if (applyType == APPLY_NONE)
											{
												ChatPacket(CHAT_TYPE_INFO, "Something wrong please contact with game master!");
												return false;
											}

											if (item2->FindApplyTypeQuest(0) > 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item already has bonuses !"));
												return false;
											}

											if (item2->IsNormalHairStyle())
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cant use this on this item!"));
												return false;
											}

											// skin inceput
											if (item2->GetVnum() >= 40143 && item2->GetVnum() <= 40148)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item already has bonuses !"));
												return false;
											}

											// skin gaya
											if (item2->GetVnum() >= 40129 && item2->GetVnum() <= 40141)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item already has bonuses !"));
												return false;
											}

											// costum inceput
											if (item2->GetVnum() >= 41726 && item2->GetVnum() <= 41728)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item already has bonuses !"));
												return false;
											}

											// freza inceput
											if (item2->GetVnum() >= 45412 && item2->GetVnum() <= 45414)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item already has bonuses !"));
												return false;
											}

											// costum gaya
											if (item2->GetVnum() >= 41775 && item2->GetVnum() <= 41777)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item already has bonuses !"));
												return false;
											}

											// freza gaya
											if (item2->GetVnum() >= 45511 && item2->GetVnum() <= 45513)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item already has bonuses !"));
												return false;
											}
											

											if (item2->GetSubType() != COSTUME_BODY && item->GetVnum() >= 401001 && item->GetVnum() <= 401012)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cant use this on this item!"));
												return false;
											}

											if (item2->GetSubType() != COSTUME_WEAPON && item->GetVnum() >= 402001 && item->GetVnum() <= 402021)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cant use this on this item!"));
												return false;
											}


											if (item2->GetSubType() != COSTUME_HAIR && item->GetVnum() >= 403001 && item->GetVnum() <= 403019)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cant use this on this item!"));
												return false;
											}

											const BYTE maxAttrCount = item2->GetSubType() == COSTUME_BODY ? SPECIAL_ATTR_COSTUME_BODY_LIMIT : (item2->GetSubType() == COSTUME_HAIR ? SPECIAL_ATTR_COSTUME_HAIR_LIMIT : (item2->GetSubType() == COSTUME_WEAPON ? SPECIAL_ATTR_COSTUME_WEAPON_LIMIT : 5));
											if (item2->GetAttributeCount() >= maxAttrCount)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item has max attr count!"));
												return false;
											}
											else if(item2->HasAttr(applyType))
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This affect already have in this item!"));
												return false;
											}
											item2->AddAttribute(applyType, applyValue);
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Successfully added!"));
											item->SetCount(item->GetCount() - 1);
											return true;
										}
										else
										{
											if (item2->IsPendantSoul() && false == item->IsPendantAttribute())
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can use only talisman enchantments on this item."));
												return false;	
											}

											if (item2->IsZodiacItem() && false == item->IsZodiacAttribute())
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can use only zodiac enchantments on this item."));
												return false;	
											}

											if ((false == item2->IsPendantSoul() && true == item->IsPendantAttribute()) || (false == item2->IsZodiacItem() && true == item->IsZodiacAttribute()))
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to use normal enchantments on this item."));
												return false;
											}

											if (item2->GetAttributeSetIndex() == -1)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경할 수 없는 아이템입니다."));
												return false;
											}

											if (item2->GetAttributeCount() == 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("변경할 속성이 없습니다."));
												return false;
											}

											if ((GM_PLAYER == GetGMLevel()) && (false == test_server) && (g_dwItemBonusChangeTime > 0))
											{
												DWORD dwChangeItemAttrCycle = g_dwItemBonusChangeTime;

												quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID());

												if (pPC)
												{
													DWORD dwNowSec = get_global_time();

													DWORD dwLastChangeItemAttrSec = pPC->GetFlag(msc_szLastChangeItemAttrFlag);

													if (dwLastChangeItemAttrSec + dwChangeItemAttrCycle > dwNowSec)
													{
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 바꾼지 %d분 이내에는 다시 변경할 수 없습니다.(%d 분 남음)"),
																dwChangeItemAttrCycle, dwChangeItemAttrCycle - (dwNowSec - dwLastChangeItemAttrSec));
														return false;
													}

													pPC->SetFlag(msc_szLastChangeItemAttrFlag, dwNowSec);
												}
											}

											if (item->GetSubType() == USE_CHANGE_ATTRIBUTE2)
											{
												int aiChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
												{
													0, 0, 30, 40, 3
												};

												item2->ChangeAttribute(aiChangeProb);
											}
											else if (item->GetVnum() == 76014)
											{
												int aiChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
												{
													0, 10, 50, 39, 1
												};

												item2->ChangeAttribute(aiChangeProb);
											}

											else
											{
												if (item->GetVnum() == 71151 || item->GetVnum() == 76023)
												{
													if ((item2->GetType() == ITEM_WEAPON)
														|| (item2->GetType() == ITEM_ARMOR))
													{
														bool bCanUse = true;
														for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
														{
															if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
															{
																bCanUse = false;
																break;
															}
														}
														if (false == bCanUse)
														{
															ChatPacket(CHAT_TYPE_INFO, LC_TEXT("적용 레벨보다 높아 사용이 불가능합니다."));
															break;
														}
													}
													else
													{
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT("무기와 갑옷에만 사용 가능합니다."));
														break;
													}
												}
												item2->ChangeAttribute();
											}

											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경하였습니다."));

											item->SetCount(item->GetCount() - 1);
										}
									}
									break;
#endif
#if defined(__BL_67_ATTR__)
								case USE_CHANGE_ATTRIBUTE2:
									if (item2->GetAttributeSetIndex() == -1 || item2->GetRareAttrCount() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경할 수 없는 아이템입니다."));
										return false;
									}

									if (item2->IsEquipped())
										return false;

									if (item->GetVnum() == SMALL_ORISON && ((number(1, 100) <= 10) == true) && item2->ChangeRareAttribute2() == true)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경하였습니다."));
									else if (item->GetVnum() == MEDIUM_ORISON && ((number(1, 100) <= 40) == true) && item2->ChangeRareAttribute2() == true)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경하였습니다."));
									else if (item->GetVnum() == LARGE_ORISON && ((number(1, 100) <= 70) == true) && item2->ChangeRareAttribute2() == true)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경하였습니다."));
									else if (item->GetVnum() == ORISON && item2->ChangeRareAttribute2() == true)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경하였습니다."));
									else
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성 추가에 실패하였습니다."));

									item->SetCount(item->GetCount() - 1);
									break;
#endif

								case USE_ADD_ATTRIBUTE :
									if (item2->IsPendantSoul() && false == item->IsPendantAttribute())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can use only talisman enchantments on this item."));
										return false;	
									}

									if ((false == item2->IsPendantSoul() && true == item->IsPendantAttribute()) || (false == item2->IsZodiacItem() && true == item->IsZodiacAttribute()))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to use normal enchantments on this item."));
										return false;
									}

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경할 수 없는 아이템입니다."));
										return false;
									}

									if ((item2->GetAttributeCount() < 4 && (item2->IsPendantSoul() || item2->IsZodiacItem())) || (item2->GetAttributeCount() < 5 && (!item2->IsPendantSoul() || !item2->IsZodiacItem())) ) // 4 ==> 5 dezactivare marmura 
									{
										if (item->GetVnum() == 71152 || item->GetVnum() == 76024)
										{
											if ((item2->GetType() == ITEM_WEAPON)
												|| (item2->GetType() == ITEM_ARMOR))
											{
												bool bCanUse = true;
												for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
													{
														bCanUse = false;
														break;
													}
												}
												if (false == bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("적용 레벨보다 높아 사용이 불가능합니다."));
													break;
												}
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("무기와 갑옷에만 사용 가능합니다."));
												break;
											}
										}
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->IsPendantSoul() && (number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()]))
										{
											item2->AddAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성 추가에 성공하였습니다."));

											int iAddedIdx = item2->GetAttributeCount() - 1;
										}
										else if (item2->IsPendant() == false)
										{
											if (item->GetVnum() == 71085)
											{
												int attrCount = item2->GetAttributeCount();

												for (int i = attrCount; i < MAX_NORM_ATTR_NUM; ++i) 
													item2->AddAttribute();

												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성 추가에 성공하였습니다."));

												int iAddedIdx = item2->GetAttributeCount() - 1;
											}
											else
											{
												item2->AddAttribute();
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성 추가에 성공하였습니다."));

												int iAddedIdx = item2->GetAttributeCount() - 1;
											}

										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성 추가에 실패하였습니다."));
										}

										if (item2->IsPendantSoul())
											item->SetCount(item->GetCount() - 1);
									}
									else
									{
										// Nou tooltip
										// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("더이상 이 아이템을 이용하여 속성을 추가할 수 없습니다."));

										if (item2->IsPendantSoul())
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("더 이상 이 아이템을 이용하여 속성을 추가할 수 없습니다."));
										}
										else
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Go to Seon-Hae to discover how to add more bonuses."));
									}
									break;

								case USE_ADD_ATTRIBUTE2 :

									if (item2->IsPendantSoul() && false == item->IsPendantAttribute())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can use only talisman enchantments on this item."));
										return false;	
									}

									if ((false == item2->IsPendantSoul() && true == item->IsPendantAttribute()) || (false == item2->IsZodiacItem() && true == item->IsZodiacAttribute()))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to use normal enchantments on this item."));
										return false;
									}

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성을 변경할 수 없는 아이템입니다."));
										return false;
									}

									if (item2->GetAttributeCount() == 4)
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
										{
											item2->AddAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성 추가에 성공하였습니다."));

											int iAddedIdx = item2->GetAttributeCount() - 1;
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("속성 추가에 실패하였습니다."));
										}

										item->SetCount(item->GetCount() - 1);
									}
									else if (item2->GetAttributeCount() == 5)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("더 이상 이 아이템을 이용하여 속성을 추가할 수 없습니다."));
									}
									else if (item2->GetAttributeCount() < 4)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("먼저 재가비서를 이용하여 속성을 추가시켜 주세요."));
									}
									else
									{
										// wtf ?!
										sys_err("ADD_ATTRIBUTE2 : Item has wrong AttributeCount(%d)", item2->GetAttributeCount());
									}
									break;

								case USE_ADD_ACCESSORY_SOCKET:
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->IsAccessoryForSocket())
										{
											if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
											{
#ifdef ENABLE_ADDSTONE_FAILURE
												if (number(1, 100) <= 50)
#else
												if (1)
#endif
												{
													item2->SetAccessorySocketMaxGrade(item2->GetAccessorySocketMaxGrade() + 1);
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소켓이 성공적으로 추가되었습니다."));
												}
												else
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소켓 추가에 실패하였습니다."));
												}

												item->SetCount(item->GetCount() - 1);
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 액세서리에는 더이상 소켓을 추가할 공간이 없습니다."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 아이템으로 소켓을 추가할 수 없는 아이템입니다."));
										}
									}
									break;

								case USE_PUT_INTO_BELT_SOCKET:
								case USE_PUT_INTO_ACCESSORY_SOCKET:
#ifdef __PERMA_ACCESSORY__
									if (item->GetVnum() == 55621 && (0 != item2->GetAccessorySocketMaxGrade()))
									{
										if (item2->GetAccessorySocketGrade() > 0 && !item2->IsEquipped())
										{
											const DWORD accessoryItemIdx = item->GetAccessorySocketItemVnum(item2);
											if (accessoryItemIdx == 0)
												return false;
											BYTE accessoryCount = 0;
					
											for (int j = 2; j >= 0; --j)
											{
												if (item2->GetSocket(j) == -3 || item2->GetSocket(j) == -2 || item2->GetSocket(j) > 0)
												{
													if (item2->GetSocket(j) == -3)
														accessoryCount += 1;
					
													if (j == 0)
													{
														if (item2->GetSocket(1) != 0)
														{
															if (item2->GetSocket(2) != 0)
															{
																item2->SetSocket(0, item2->GetSocket(1));
																item2->SetSocket(1, item2->GetSocket(2));
																item2->SetSocket(2, -1);
															}
															else
															{
																item2->SetSocket(0, item2->GetSocket(1));
																item2->SetSocket(1, -1);
															}
														}
														else
															item2->SetSocket(j, -1);
													}
													else if (j == 1)
													{
														if (item2->GetSocket(2) != 0)
														{
															item2->SetSocket(1, item2->GetSocket(2));
															item2->SetSocket(2, -1);
														}
														else
															item2->SetSocket(j, -1);
													}
													else
														item2->SetSocket(j, -1);
												}
											}
					
											if (accessoryCount)
												AutoGiveItem(accessoryItemIdx + 5000, accessoryCount);
												
											item->SetCount(item->GetCount() - 1);
											ChatPacket(CHAT_TYPE_INFO, "Process successfully. You get back %d perma accessory", accessoryCount);
										}
										return true;
									}
#endif
									if (item2->IsAccessoryForSocket() && item->CanPutInto(item2))
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->GetAccessorySocketGrade() < item2->GetAccessorySocketMaxGrade())
										{

#ifdef __PERMA_ACCESSORY__
											if (number(1, 100) <= (item->GetValue(0) == 99 ? 100 : aiAccessorySocketPutPct[item2->GetAccessorySocketGrade()]))
#else
											if (number(1, 100) <= aiAccessorySocketPutPct[item2->GetAccessorySocketGrade()])
#endif										
											{
#ifdef __PERMA_ACCESSORY__
												item2->SetAccessorySocketGrade((item->GetValue(0) == 99) ?  true : false);
#else
												item2->SetAccessorySocketGrade(item2->GetAccessorySocketGrade() + 1);
#endif
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("장착에 성공하였습니다."));
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("장착에 실패하였습니다."));
											}

											item->SetCount(item->GetCount() - 1);
										}
										else
										{
											if (item2->GetAccessorySocketMaxGrade() == 0)
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("먼저 다이아몬드로 악세서리에 소켓을 추가해야합니다."));
											else if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 액세서리에는 더이상 장착할 소켓이 없습니다."));
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다이아몬드로 소켓을 추가해야합니다."));
											}
											else
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 액세서리에는 더이상 보석을 장착할 수 없습니다."));
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 아이템을 장착할 수 없습니다."));
									}
									break;
							}
							if (item2->IsEquipped())
							{
								BuffOnAttr_AddBuffsFromItem(item2);
							}
						}
						break;
						//  END_OF_ACCESSORY_REFINE & END_OF_ADD_ATTRIBUTES & END_OF_CHANGE_ATTRIBUTES

					case USE_BAIT:
						{
							if (m_pkFishingEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("낚시 중에 미끼를 갈아끼울 수 없습니다."));
								return false;
							}

							LPITEM weapon = GetWear(WEAR_WEAPON);

							if (!weapon || weapon->GetType() != ITEM_ROD)
								return false;

							if (weapon->GetSocket(2))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 꽂혀있던 미끼를 빼고 %s를 끼웁니다."), item->GetLocaleName(GetLanguage()));
							}
							else
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("낚시대에 %s를 미끼로 끼웁니다."), item->GetLocaleName(GetLanguage()));
							}

							weapon->SetSocket(2, item->GetValue(0));
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_MOVE:
					case USE_TREASURE_BOX:
					case USE_MONEYBAG:
						break;

					case USE_AFFECT:
					{
// #if defined(__EXTENDED_BLEND_AFFECT__)
// 						for (int i = AFFECT_ENERGY; i <= AFFECT_PENETRATE; ++i)
// 						{
// 							if (FindAffect(i, aApplyInfo[item->GetValue(1)].bPointType))
// 							{
// 								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
// 								return false;
// 							}
// 						}
// #endif
						if (FindAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType))
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
						}
						else
						{
							AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false);
							item->SetCount(item->GetCount() - 1);
						}
					}
					break;


					case USE_CREATE_STONE:
						AutoGiveItem(number(28000, 28013));
						item->SetCount(item->GetCount() - 1);
						break;

					case USE_RECIPE :
						{
							LPITEM pSource1 = FindSpecifyItem(item->GetValue(1));
							DWORD dwSourceCount1 = item->GetValue(2);

							LPITEM pSource2 = FindSpecifyItem(item->GetValue(3));
							DWORD dwSourceCount2 = item->GetValue(4);

							if (dwSourceCount1 != 0)
							{
								if (pSource1 == NULL)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("물약 조합을 위한 재료가 부족합니다."));
									return false;
								}
							}

							if (dwSourceCount2 != 0)
							{
								if (pSource2 == NULL)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("물약 조합을 위한 재료가 부족합니다."));
									return false;
								}
							}

							if (pSource1 != NULL)
							{
								if (pSource1->GetCount() < dwSourceCount1)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("재료(%s)가 부족합니다."), pSource1->GetLocaleName(GetLanguage()));
									return false;
								}

								pSource1->SetCount(pSource1->GetCount() - dwSourceCount1);
							}

							if (pSource2 != NULL)
							{
								if (pSource2->GetCount() < dwSourceCount2)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("재료(%s)가 부족합니다."), pSource2->GetLocaleName(GetLanguage()));
									return false;
								}

								pSource2->SetCount(pSource2->GetCount() - dwSourceCount2);
							}

							LPITEM pBottle = FindSpecifyItem(50901);

							if (!pBottle || pBottle->GetCount() < 1)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("빈 병이 모자릅니다."));
								return false;
							}

							pBottle->SetCount(pBottle->GetCount() - 1);

							if (number(1, 100) > item->GetValue(5))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("물약 제조에 실패했습니다."));
								return false;
							}

							AutoGiveItem(item->GetValue(0));
						}
						break;
				}
			}
			break;

		case ITEM_METIN:
			{
				LPITEM item2;

				if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					return false;

				if (item2->GetType() == ITEM_PICK) return false;
				if (item2->GetType() == ITEM_ROD) return false;

				int i;

				for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				{
					DWORD dwVnum;

					if ((dwVnum = item2->GetSocket(i)) <= 2)
						continue;

					TItemTable * p = ITEM_MANAGER::instance().GetTable(dwVnum);

					if (!p)
						continue;

					if (item->GetValue(5) == p->alValues[5])
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("같은 종류의 메틴석은 여러개 부착할 수 없습니다."));
						return false;
					}
				}

				if (item2->GetType() == ITEM_ARMOR)
				{
					if (!IS_SET(item->GetWearFlag(), WEARABLE_BODY) || !IS_SET(item2->GetWearFlag(), WEARABLE_BODY))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 메틴석은 장비에 부착할 수 없습니다."));
						return false;
					}
				}
				else if (item2->GetType() == ITEM_WEAPON)
				{
					if (!IS_SET(item->GetWearFlag(), WEARABLE_WEAPON))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 메틴석은 무기에 부착할 수 없습니다."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("부착할 수 있는 슬롯이 없습니다."));
					return false;
				}

				for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
					if (item2->GetSocket(i) >= 1 && item2->GetSocket(i) <= 2 && item2->GetSocket(i) >= item->GetValue(2))
					{
#ifdef ENABLE_BATTLE_PASS
						CHARACTER_MANAGER::Instance().DoMission(this, MISSION_SPRITE_STONE, 1, item->GetVnum());
#endif
#ifdef ENABLE_ADDSTONE_FAILURE
						if (number(1, 100) <= 30)
#else
						if (1)
#endif
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("메틴석 부착에 성공하였습니다."));
							item2->SetSocket(i, item->GetVnum());
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("메틴석 부착에 실패하였습니다."));
							item2->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
						}

						item->SetCount(item->GetCount() - 1);
						break;
					}

				if (i == ITEM_SOCKET_MAX_NUM)
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("부착할 수 있는 슬롯이 없습니다."));
			}
			break;

		case ITEM_AUTOUSE:
		case ITEM_MATERIAL:
		case ITEM_SPECIAL:
		case ITEM_TOOL:
		case ITEM_LOTTERY:
			break;

		case ITEM_TOTEM:
			{
				if (!item->IsEquipped())
					EquipItem(item);
			}
			break;

		case ITEM_BLEND:
			sys_log(0,"ITEM_BLEND!!");
			if (Blend_Item_find(item->GetVnum()))
			{
				int		affect_type		= AFFECT_BLEND;
				int		apply_type		= aApplyInfo[item->GetSocket(0)].bPointType;
				int		apply_value		= item->GetSocket(1);
				int		apply_duration	= item->GetSocket(2) <= 0 ? INFINITE_AFFECT_DURATION : item->GetSocket(2);

#if defined(__EXTENDED_BLEND_AFFECT__)
				if ((apply_duration != 0) && UseExtendedBlendAffect(item, affect_type, apply_type, apply_value, apply_duration))
				{
					item->Lock(true);
					item->SetSocket(3, true);
					item->StartBlendExpireEvent();
				}
				else
				{
					item->Lock(false);
					item->SetSocket(3, false);
					item->StopBlendExpireEvent();
				}
				break;
#endif

				if (FindAffect(affect_type, apply_type))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
				}
				else
				{
					if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_RESIST_MAGIC))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 효과가 걸려 있습니다."));
					}
					else
					{
#if defined(__BLEND_AFFECT__)
						if (SetBlendAffect(item))
						{
							AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, false);
							item->SetCount(item->GetCount() - 1);
						}
#else
						AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, false);
						item->SetCount(item->GetCount() - 1);
#endif
					}
				}
			}
			break;
		case ITEM_EXTRACT:
			{
				LPITEM pDestItem = GetItem(DestCell);
				if (NULL == pDestItem)
				{
					return false;
				}
				switch (item->GetSubType())
				{
				case EXTRACT_DRAGON_SOUL:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().PullOut(this, NPOS, pDestItem, item);
					}
					return false;
				case EXTRACT_DRAGON_HEART:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().ExtractDragonHeart(this, pDestItem, item);
					}
					return false;
				default:
					return false;
				}
			}
			break;

		case ITEM_GACHA:
			{
				DWORD dwBoxVnum = item->GetVnum();
				std::vector <DWORD> dwVnums;
				std::vector <DWORD> dwCounts;
				std::vector <LPITEM> item_gets(0);
				int count = 0;

				if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
				{
					if(item->GetSocket(0) > 1)
						item->SetSocket(0, item->GetSocket(0) - 1);
					else
						ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (ITEM_GACHA)");
				}
			}
			break;

		case ITEM_NONE:
			sys_err("Item type NONE %s", item->GetName());
			break;

		default:
			sys_log(0, "UseItemEx: Unknown type %s %d", item->GetName(), item->GetType());
			return false;
	}

	if (item->IsNewPetItem())
		item->StartRealTimeExpireEvent();

	if (-1 != iLimitRealtimeStartFirstUseFlagIndex && item->IsEquipped() == true)
	{
		if (0 == item->GetSocket(1))
		{
			long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[iLimitRealtimeStartFirstUseFlagIndex].lValue;

			if (0 == duration)
				duration = 60 * 60 * 24 * 7;

			item->SetSocket(0, time(0) + duration);
			item->StartRealTimeExpireEvent();
		}

		if (false == item->IsEquipped() && item->GetSocket(1) == 0)
			item->SetSocket(1, item->GetSocket(1) + 1);
	}

	return true;
}

int g_nPortalLimitTime = 10;

int CHARACTER::CalculateItemPos(LPITEM item)
{
	int iEmptyPos = -1;
	if (item->IsDragonSoul())
		iEmptyPos = GetEmptyDragonSoulInventory(item);
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	else if (item->IsSkillBook())
		iEmptyPos = GetEmptySkillBookInventory(item->GetSize());
	else if (item->IsUpgradeItem())
		iEmptyPos = GetEmptyUpgradeItemsInventory(item->GetSize());
	else if (item->IsStone())
		iEmptyPos = GetEmptyStoneInventory(item->GetSize());
	else if (item->IsGiftBox())
		iEmptyPos = GetEmptyGiftBoxInventory(item->GetSize());
	else if (item->IsSwitch())
		iEmptyPos = GetEmptySwitchInventory(item->GetSize());
#endif
	else
		iEmptyPos = GetEmptyInventory(item->GetSize());
	return iEmptyPos;
}

bool CHARACTER::UseItemStack(TItemPos Cell, int count)
{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	LPITEM item;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

#ifdef __PENDANT__
	if (item->IsPendantSoul())
		return false;
#endif

	sys_log(0, "%s: USE_ITEM %s (inven %d, cell: %d)", GetName(), item->GetName(), window_type, wCell);

	if (item->IsExchanging())
		return false;

#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition())
	{
		CSwitchbot* pkSwitchbot = CSwitchbotManager::Instance().FindSwitchbot(GetPlayerID());
		if (pkSwitchbot && pkSwitchbot->IsActive(Cell.cell))
		{
			return false;
		}

		int iEmptyCell = GetEmptyInventory(item->GetSize());

		if (iEmptyCell == -1)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot remove item from switchbot. Inventory is full."));
			return false;
		}

		MoveItem(Cell, TItemPos(INVENTORY, iEmptyCell), item->GetCount());
		return true;
	}
#endif

	if (!item->CanUsedBy(this))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군직이 맞지않아 이 아이템을 사용할 수 없습니다."));
		return false;
	}

	if (IsStun())
		return false;

	if (false == FN_check_item_sex(this, item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("성별이 맞지않아 이 아이템을 사용할 수 없습니다."));
		return false;
	}

	//PREVENT_TRADE_WINDOW
	if (IS_SUMMON_ITEM(item->GetVnum()))
	{
		if (false == IS_SUMMONABLE_ZONE(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("사용할수 없습니다."));
			return false;
		}

		if (CThreeWayWar::instance().IsThreeWayWarMapIndex(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("삼거리 전투 참가중에는 귀환부,귀환기억부를 사용할수 없습니다."));
			return false;
		}
		int iPulse = thecore_pulse();

		if (iPulse - GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("창고를 연후 %d초 이내에는 귀환부,귀환기억부를 사용할 수 없습니다."), g_nPortalLimitTime);

			if (test_server)
				ChatPacket(CHAT_TYPE_INFO, "[TestOnly]Pulse %d LoadTime %d PASS %d", iPulse, GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
			return false;
		}

		if (PreventTradeWindow(WND_ALL))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래창,창고 등을 연 상태에서는 귀환부,귀환기억부 를 사용할수 없습니다."));
			return false;
		}

		//PREVENT_REFINE_HACK

		{
			if (iPulse - GetRefineTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 개량후 %d초 이내에는 귀환부,귀환기억부를 사용할 수 없습니다."), g_nPortalLimitTime);
				return false;
			}
		}
		//END_PREVENT_REFINE_HACK

		//PREVENT_ITEM_COPY
		{
			if (iPulse - GetMyShopTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("개인상점 사용후 %d초 이내에는 귀환부,귀환기억부를 사용할 수 없습니다."), g_nPortalLimitTime);
				return false;
			}

		}
		//END_PREVENT_ITEM_COPY

		if (item->GetVnum() != 70302)
		{
			PIXEL_POSITION posWarp;

			int x = 0;
			int y = 0;

			double nDist = 0;
			const double nDistant = 5000.0;

			if (item->GetVnum() == 22010)
			{
				x = item->GetSocket(0) - GetX();
				y = item->GetSocket(1) - GetY();
			}

			else if (item->GetVnum() == 22000)
			{
				SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp);

				if (item->GetSocket(0) == 0)
				{
					x = posWarp.x - GetX();
					y = posWarp.y - GetY();
				}
				else
				{
					x = item->GetSocket(0) - GetX();
					y = item->GetSocket(1) - GetY();
				}
			}

			nDist = sqrt(pow((float)x,2) + pow((float)y,2));

			if (nDistant > nDist)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이동 되어질 위치와 너무 가까워 귀환부를 사용할수 없습니다."));
				if (test_server)
					ChatPacket(CHAT_TYPE_INFO, "PossibleDistant %f nNowDist %f", nDistant,nDist);
				return false;
			}
		}

		//PREVENT_PORTAL_AFTER_EXCHANGE

		if (iPulse - GetExchangeTime()  < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래 후 %d초 이내에는 귀환부,귀환기억부등을 사용할 수 없습니다."), g_nPortalLimitTime);
			return false;
		}
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

	}

	if ((item->GetVnum() == 50200) || (item->GetVnum() == 71049))
	{
		if (PreventTradeWindow(WND_ALL))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래창,창고 등을 연 상태에서는 보따리,비단보따리를 사용할수 없습니다."));
			return false;
		}

	}
	//END_PREVENT_TRADE_WINDOW

	// @fixme150 BEGIN
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item if you're using quests"));
		return false;
	}
	// @fixme150 END

	if (IS_SET(item->GetFlag(), ITEM_FLAG_LOG))
	{
		DWORD vid = item->GetVID();
		DWORD oldCount = item->GetCount();
		DWORD vnum = item->GetVnum();

		char hint[ITEM_NAME_MAX_LEN + 32 + 1];
		int len = snprintf(hint, sizeof(hint) - 32, "%s", item->GetName());

		if (len < 0 || len >= (int) sizeof(hint) - 32)
			len = (sizeof(hint) - 32) - 1;

		bool ret = UseItemEx(item, NPOS);

		return (ret);
	}
	else
		return UseItemEx(item, NPOS);
}

bool CHARACTER::UseItem(TItemPos Cell, TItemPos DestCell)
{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	LPITEM item;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	sys_log(0, "%s: USE_ITEM %s (inven %d, cell: %d)", GetName(), item->GetName(), window_type, wCell);

	if (item->IsExchanging())
		return false;

#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition())
	{
		CSwitchbot* pkSwitchbot = CSwitchbotManager::Instance().FindSwitchbot(GetPlayerID());
		if (pkSwitchbot && pkSwitchbot->IsActive(Cell.cell))
		{
			return false;
		}

		int iEmptyCell = GetEmptyInventory(item->GetSize());

		if (iEmptyCell == -1)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot remove item from switchbot. Inventory is full."));
			return false;
		}

		MoveItem(Cell, TItemPos(INVENTORY, iEmptyCell), item->GetCount());
		return true;
	}
#endif

	if (!item->CanUsedBy(this))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군직이 맞지않아 이 아이템을 사용할 수 없습니다."));
		return false;
	}

	if (IsStun())
		return false;

	if (false == FN_check_item_sex(this, item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("성별이 맞지않아 이 아이템을 사용할 수 없습니다."));
		return false;
	}

	//PREVENT_TRADE_WINDOW
	if (IS_SUMMON_ITEM(item->GetVnum()))
	{
		if (false == IS_SUMMONABLE_ZONE(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("사용할수 없습니다."));
			return false;
		}

		if (CThreeWayWar::instance().IsThreeWayWarMapIndex(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("삼거리 전투 참가중에는 귀환부,귀환기억부를 사용할수 없습니다."));
			return false;
		}
		int iPulse = thecore_pulse();

		if (iPulse - GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("창고를 연후 %d초 이내에는 귀환부,귀환기억부를 사용할 수 없습니다."), g_nPortalLimitTime);

			if (test_server)
				ChatPacket(CHAT_TYPE_INFO, "[TestOnly]Pulse %d LoadTime %d PASS %d", iPulse, GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
			return false;
		}

		if (PreventTradeWindow(WND_ALL))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래창,창고 등을 연 상태에서는 귀환부,귀환기억부 를 사용할수 없습니다."));
			return false;
		}

		//PREVENT_REFINE_HACK

		{
			if (iPulse - GetRefineTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 개량후 %d초 이내에는 귀환부,귀환기억부를 사용할 수 없습니다."), g_nPortalLimitTime);
				return false;
			}
		}
		//END_PREVENT_REFINE_HACK

		//PREVENT_ITEM_COPY
		{
			if (iPulse - GetMyShopTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("개인상점 사용후 %d초 이내에는 귀환부,귀환기억부를 사용할 수 없습니다."), g_nPortalLimitTime);
				return false;
			}

		}
		//END_PREVENT_ITEM_COPY

		if (item->GetVnum() != 70302)
		{
			PIXEL_POSITION posWarp;

			int x = 0;
			int y = 0;

			double nDist = 0;
			const double nDistant = 5000.0;

			if (item->GetVnum() == 22010)
			{
				x = item->GetSocket(0) - GetX();
				y = item->GetSocket(1) - GetY();
			}

			else if (item->GetVnum() == 22000)
			{
				SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp);

				if (item->GetSocket(0) == 0)
				{
					x = posWarp.x - GetX();
					y = posWarp.y - GetY();
				}
				else
				{
					x = item->GetSocket(0) - GetX();
					y = item->GetSocket(1) - GetY();
				}
			}

			nDist = sqrt(pow((float)x,2) + pow((float)y,2));

			if (nDistant > nDist)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이동 되어질 위치와 너무 가까워 귀환부를 사용할수 없습니다."));
				if (test_server)
					ChatPacket(CHAT_TYPE_INFO, "PossibleDistant %f nNowDist %f", nDistant,nDist);
				return false;
			}
		}

		//PREVENT_PORTAL_AFTER_EXCHANGE

		if (iPulse - GetExchangeTime()  < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래 후 %d초 이내에는 귀환부,귀환기억부등을 사용할 수 없습니다."), g_nPortalLimitTime);
			return false;
		}
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

	}

	if ((item->GetVnum() == 50200) || (item->GetVnum() == 71049))
	{
		if (PreventTradeWindow(WND_ALL))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래창,창고 등을 연 상태에서는 보따리,비단보따리를 사용할수 없습니다."));
			return false;
		}

	}
	//END_PREVENT_TRADE_WINDOW

	// @fixme150 BEGIN
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item if you're using quests"));
		return false;
	}
	// @fixme150 END

	if (IS_SET(item->GetFlag(), ITEM_FLAG_LOG))
	{
		DWORD vid = item->GetVID();
		DWORD oldCount = item->GetCount();
		DWORD vnum = item->GetVnum();

		char hint[ITEM_NAME_MAX_LEN + 32 + 1];
		int len = snprintf(hint, sizeof(hint) - 32, "%s", item->GetName());

		if (len < 0 || len >= (int) sizeof(hint) - 32)
			len = (sizeof(hint) - 32) - 1;

		bool ret = UseItemEx(item, DestCell);
		return (ret);
	}
	else
		return UseItemEx(item, DestCell);
}

bool CHARACTER::DestroyItem(TItemPos Cell)
{
	LPITEM item = NULL;
	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("강화창을 연 상태에서는 아이템을 옮길 수 없습니다."));
		return false;
	}
	if (IsDead())
		return false;
		
	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (!item)
		return false;

	if (item->IsExchanging())
		return false;
		
	if (item->CannotBeDestroyed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't destroy this item."));
		return false;
	}

	if (true == item->isLocked())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't destroy this item."));
		return false;
	}

	if ((item->GetType() == ITEM_BUFFI && item->GetSubType() == BUFFI_SCROLL) && item->GetSocket(1) == 1)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Deactivate the Buffi Seal first."));
		return false;
	}
	if ((item->GetVnum() >= 51010 && item->GetVnum() <= 51030) && item->GetSocket(0) == 1)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Deactivate the Energy Crystal first."));
		return false;
	}
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
		return false;

	if (item->GetCount() <= 0)
		return false;
		
	SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, 255);
	ITEM_MANAGER::instance().RemoveItem(item, "DESTROY_CMD");
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You deleted %s."), item->GetLocaleName(GetLanguage()));
	return true;
}

bool CHARACTER::DropItem(TItemPos Cell, DWORD bCount)
{
	LPITEM item = NULL;

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("강화창을 연 상태에서는 아이템을 옮길 수 없습니다."));
		return false;
	}

	if (PreventTradeWindow(WND_EXCHANGE, false/*except*/))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't drop an item while exchange."));
		return false;
	}

#ifdef ENABLE_NEWSTUFF
	if (g_ItemDropTimeLimitValue && !PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::ItemDrop, std::chrono::milliseconds(g_ItemDropTimeLimitValue)))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("퀘스트를 로드하는 중입니다. 잠시만 기다려 주십시오."));
		return false;
	}
#endif
	if (IsDead())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (true == item->isLocked())
		return false;

	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP | ITEM_ANTIFLAG_GIVE) || item->GetSocket(4) == 99)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("버릴 수 없는 아이템입니다."));
		return false;
	}

	if ((item->GetType() == ITEM_BUFFI && item->GetSubType() == BUFFI_SCROLL) && item->GetSocket(1) == 1)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Deactivate the Buffi Seal first."));
		return false;
	}

	if ((item->GetVnum() >= 51010 && item->GetVnum() <= 51030) && item->GetSocket(0) == 1)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Deactivate the Energy Crystal first."));
		return false;
	}

	if (bCount == 0 || bCount > item->GetCount())
		bCount = item->GetCount();

	SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, 255);

	LPITEM pkItemToDrop;

	if (bCount == item->GetCount())
	{
		item->RemoveFromCharacter();
		pkItemToDrop = item;
	}
	else
	{
		if (bCount == 0)
		{
			if (test_server)
				sys_log(0, "[DROP_ITEM] drop item count == 0");
			return false;
		}

		item->SetCount(item->GetCount() - bCount);
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		pkItemToDrop = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), bCount);

		// copy item socket -- by mhh
		FN_copy_item_socket(pkItemToDrop, item);

		char szBuf[51 + 1];
		snprintf(szBuf, sizeof(szBuf), "%u %u", pkItemToDrop->GetID(), pkItemToDrop->GetCount());
	}

	PIXEL_POSITION pxPos = GetXYZ();

	if (pkItemToDrop->AddToGround2(GetMapIndex(), pxPos, false, this, g_aiItemDestroyTime[ITEM_DESTROY_TIME_DROPITEM]))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("떨어진 아이템은 3분 후 사라집니다."));
#ifdef ENABLE_NEWSTUFF
		pkItemToDrop->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_DROPITEM]);
#else
		pkItemToDrop->StartDestroyEvent();
#endif
		ITEM_MANAGER::instance().FlushDelayedSave(pkItemToDrop);
		char szHint[32 + 1];
		snprintf(szHint, sizeof(szHint), "%s %u %u", pkItemToDrop->GetName(), pkItemToDrop->GetCount(), pkItemToDrop->GetOriginalVnum());
		LogManager::instance().ItemLog(this, pkItemToDrop, "DROP_TO_GROUND", szHint);
		//Motion(MOTION_PICKUP);
	}

	return true;
}

bool CHARACTER::DropGold(long long gold)
{
	if (gold <= 0 || gold > GetGold())
		return false;

	if (!CanHandleItem())
		return false;

#ifdef ENABLE_NEWSTUFF
	if (g_GoldDropTimeLimitValue && !PulseManager::Instance().IncreaseClock(GetPlayerID(), ePulse::BoxOpening, std::chrono::milliseconds(g_GoldDropTimeLimitValue)))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("퀘스트를 로드하는 중입니다. 잠시만 기다려 주십시오."));
		return false;
	}
#endif

	LPITEM item = ITEM_MANAGER::instance().CreateItem(1, gold);

	if (item)
	{
		PIXEL_POSITION pos = GetXYZ();

		if (item->AddToGround(GetMapIndex(), pos))
		{
			//Motion(MOTION_PICKUP);
			PointChange(POINT_GOLD, -gold, true);

#ifdef ENABLE_NEWSTUFF
			item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_DROPGOLD]);
#else
			item->StartDestroyEvent();
#endif
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("떨어진 아이템은 %d분 후 사라집니다."), 150/60);
		}

		Save();
		return true;
	}

	return false;
}

bool CHARACTER::MoveItem(TItemPos Cell, TItemPos DestCell, DWORD count)
{
	if (Cell.IsSamePosition(DestCell)) // @fixme196 (check same slot n same window aliases)
		return false;

	if (!IsValidItemPosition(Cell))
		return false;

	LPITEM item = NULL;
	if (!(item = GetItem(Cell)))
		return false;

	if (item->IsRemoveAfterUnequip() && item->IsEquipped())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Press right click on the item to remove."));
		return false;
	}

	if (item->IsExchanging())
		return false;

	if (item->GetCount() < count)
		return false;

	if (Cell.IsDefaultInventoryPosition() && DestCell.IsDefaultInventoryPosition() && DestCell.cell >= Inventory_Size())
		return false;

	if (INVENTORY == Cell.window_type && Cell.cell >= Inventory_Size() && IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

	if (true == item->isLocked())
		return false;

	if (!IsValidItemPosition(DestCell))
	{
		return false;
	}

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("강화창을 연 상태에서는 아이템을 옮길 수 없습니다."));
		return false;
	}

#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition())
	{
		if (CSwitchbotManager::Instance().IsActive(GetPlayerID(), Cell.cell))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot move active switchbot item."));
			return false;
		}
	
		if (DestCell.IsEquipPosition())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Transfer type not allowed."));
			return false;
		}
	}

	if (DestCell.IsSwitchbotPosition())
	{
		if (item->IsEquipped())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Transfer type not allowed."));
			return false;
		}
		
		if (!SwitchbotHelper::IsValidItem(item))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Invalid item type for switchbot."));
			return false;
		}
	}
#endif
	
#ifdef __RENEWAL_MOUNT__
	if (DestCell.IsDefaultInventoryPosition() && Cell.IsEquipPosition())
	{
		if (item->IsCostumeMountItem() && GetHorse())
		{
			HorseSummon(false);
		}
#ifdef __SKIN_SYSTEM__
		if (item->IsSkinMount() && GetHorse() && GetWear(WEAR_COSTUME_MOUNT))
		{
			HorseSummon(false);
			LPITEM mount = GetWear(WEAR_COSTUME_MOUNT);
			HorseSummon(true, false, mount->GetValue(3));
		}
#endif
	}
#endif

	if (DestCell.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 아이템은 벨트 인벤토리로 옮길 수 없습니다."));
		return false;
	}

#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	if (DestCell.IsSkillBookInventoryPosition() && (item->IsEquipped() || Cell.IsBeltInventoryPosition()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return false;
	}

	if (DestCell.IsUpgradeItemsInventoryPosition() && (item->IsEquipped() || Cell.IsBeltInventoryPosition()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return false;
	}

	if (DestCell.IsStoneInventoryPosition() && (item->IsEquipped() || Cell.IsBeltInventoryPosition()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return false;
	}

	if (DestCell.IsGiftBoxInventoryPosition() && (item->IsEquipped() || Cell.IsBeltInventoryPosition()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return false;
	}

	if ((DestCell.IsSwitchInventoryPosition() && item->IsEquipped() || Cell.IsBeltInventoryPosition()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return false;
	}

	if (DestCell.IsCostumeInventoryPosition() && (item->IsEquipped() || Cell.IsBeltInventoryPosition()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return false;
	}

	if ((Cell.IsSkillBookInventoryPosition() && !DestCell.IsSkillBookInventoryPosition() && !DestCell.IsDefaultInventoryPosition()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return false;
	}

	if (Cell.IsUpgradeItemsInventoryPosition() && !DestCell.IsUpgradeItemsInventoryPosition() && !DestCell.IsDefaultInventoryPosition())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return false;
	}

	if (Cell.IsStoneInventoryPosition() && !DestCell.IsStoneInventoryPosition() && !DestCell.IsDefaultInventoryPosition())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return false;
	}

	if (Cell.IsGiftBoxInventoryPosition() && !DestCell.IsGiftBoxInventoryPosition() && !DestCell.IsDefaultInventoryPosition())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return false;
	}

	if (Cell.IsSwitchInventoryPosition() && !DestCell.IsSwitchInventoryPosition() && !DestCell.IsDefaultInventoryPosition())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return false;
	}

	if (Cell.IsCostumeInventoryPosition() && !DestCell.IsCostumeInventoryPosition() && !DestCell.IsDefaultInventoryPosition())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return false;
	}

	if (Cell.IsDefaultInventoryPosition() && DestCell.IsSkillBookInventoryPosition())
	{
		if (!item->IsSkillBook())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
			return false;
		}
	}

	if (Cell.IsDefaultInventoryPosition() && DestCell.IsUpgradeItemsInventoryPosition())
	{
		if (!item->IsUpgradeItem())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
			return false;
		}
	}

	if (Cell.IsDefaultInventoryPosition() && DestCell.IsStoneInventoryPosition())
	{
		if (!item->IsStone())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
			return false;
		}
	}

	if (Cell.IsDefaultInventoryPosition() && DestCell.IsGiftBoxInventoryPosition())
	{
		if (!item->IsGiftBox())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
			return false;
		}
	}
	if (Cell.IsDefaultInventoryPosition() && DestCell.IsSwitchInventoryPosition())
	{
		if (!item->IsSwitch())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
			return false;
		}
	}

	if (Cell.IsDefaultInventoryPosition() && DestCell.IsCostumeInventoryPosition())
	{
		if (!item->IsCostume())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
			return false;
		}
	}
#endif

	if (Cell.IsEquipPosition())
	{
		if (!CanUnequipNow(item))
			return false;

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		int iWearCell = item->FindEquipCell(this);
		if (iWearCell == WEAR_WEAPON)
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !UnequipItem(costumeWeapon))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot unequip the costume weapon. Not enough space."));
				return false;
			}

			if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
				return UnequipItem(item);
		}
#endif
	}

#ifdef __TIME_STACKEBLE__
	LPITEM itemNext = NULL;
	if (DestCell.IsDefaultInventoryPosition() && Cell.IsDefaultInventoryPosition() && ITEM_FLAG_TIME_STACKEBLE & item->GetFlag()  && (itemNext = GetItem(DestCell)) && itemNext->GetCount() == 1 && item->GetCount() == 1)
	{
		if (ITEM_FLAG_TIME_STACKEBLE & itemNext->GetFlag())
		{
			if (itemNext->GetVnum() == item->GetVnum())
			{
				if (itemNext->GetType() == ITEM_BLEND)
				{
					if (itemNext->GetSocket(1) == item->GetSocket(1))
					{
						itemNext->SetSocket(2, itemNext->GetSocket(2) + item->GetSocket(2));
						item->SetCount(item->GetCount() - 1);
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Item time combined successfully."));
						return true;
					}
					else
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can combine only with same bonus value."));
						return false;
					}
				}
				// here... for other types...
			}
		}
	}
#endif

	if (DestCell.IsEquipPosition())
	{
		if (GetItem(DestCell))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 장비를 착용하고 있습니다."));

			return false;
		}

		EquipItem(item, DestCell.cell - INVENTORY_MAX_NUM);
	}
	else
	{
		if (item->IsDragonSoul())
		{
			if (item->IsEquipped())
			{
				return DSManager::instance().PullOut(this, DestCell, item);
			}
			else
			{
				if (DestCell.window_type != DRAGON_SOUL_INVENTORY)
				{
					return false;
				}

				if (!DSManager::instance().IsValidCellForThisItem(item, DestCell))
					return false;
			}
		}

		else if (DRAGON_SOUL_INVENTORY == DestCell.window_type)
			return false;

		LPITEM item2;

		if ((item2 = GetItem(DestCell)) && item != item2 && item2->IsStackable() && !IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_STACK) && item2->GetVnum() == item->GetVnum())
		{
#if defined(__EXTENDED_BLEND_AFFECT__)
			if (item->IsBlendItem() && item2->IsBlendItem() && !IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
			{
				if (count == 0)
					count = item->GetCount();

				for (int i = 0; i < 2; ++i)
					// 0 - apply_type
					// 1 - apply_value
					if (item2->GetSocket(i) != item->GetSocket(i))
						return false;

				item2->SetSocket(2, item2->GetSocket(2) + item->GetSocket(2));
				item->SetCount(item->GetCount() - count);
				return true;
			}
#endif

			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				if (item2->GetSocket(i) != item->GetSocket(i))
					return false;

			if (count == 0)
				count = item->GetCount();

			sys_log(0, "%s: ITEM_STACK %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetLocaleName(GetLanguage()), Cell.window_type, Cell.cell,
				DestCell.window_type, DestCell.cell, count);

			count = MIN(g_bItemCountLimit - item2->GetCount(), count);

			item->SetCount(item->GetCount() - count);
			item2->SetCount(item2->GetCount() + count);
			return true;
		}

		if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
		{
			return false;
		}

		if (count == 0 || count >= item->GetCount() || !item->IsStackable() || IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			sys_log(0, "%s: ITEM_MOVE %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetLocaleName(GetLanguage()), Cell.window_type, Cell.cell,
				DestCell.window_type, DestCell.cell, count);

			item->RemoveFromCharacter();
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			SetItem(DestCell, item, false);
#else
			SetItem(DestCell, item);
#endif
			if (INVENTORY == Cell.window_type && INVENTORY == DestCell.window_type)
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, DestCell.cell);
		}
		else if (count < item->GetCount())
		{
			sys_log(0, "%s: ITEM_SPLIT %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell,
				DestCell.window_type, DestCell.cell, count);

			item->SetCount(item->GetCount() - count);
			LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), count);

			// copy socket -- by mhh
			FN_copy_item_socket(item2, item);

#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item2->AddToCharacter(this, DestCell, false);
#else
			item2->AddToCharacter(this, DestCell);
#endif
		}

		if (item->IsSkinPet());
		{
			CPetSystem* petSystem = GetPetSystem();
			if (petSystem)
				petSystem->HandlePetCostumeItem();
		}
	}

	return true;
}

namespace NPartyPickupDistribute
{
	struct FFindOwnership
	{
		LPITEM item;
		LPCHARACTER owner;

		FFindOwnership(LPITEM item)
			: item(item), owner(NULL)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (item->IsOwnership(ch))
				owner = ch;
		}
	};

	struct FCountNearMember
	{
		int		total;
		int		x, y;

		FCountNearMember(LPCHARACTER center )
			: total(0), x(center->GetX()), y(center->GetY())
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				total += 1;
		}
	};

	struct FMoneyDistributor
	{
		int		total;
		LPCHARACTER	c;
		int		x, y;
		int		iMoney;

		FMoneyDistributor(LPCHARACTER center, int iMoney)
			: total(0), c(center), x(center->GetX()), y(center->GetY()), iMoney(iMoney)
		{
		}

		void operator ()(LPCHARACTER ch)
		{
			if (ch!=c)
				if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				{
					ch->PointChange(POINT_GOLD, iMoney, true);
				}
		}
	};

#if defined(__GEM_SYSTEM__)
	struct FGemDistributor
	{
		int total;
		LPCHARACTER c;
		int x, y;
		int iGem;

		FGemDistributor(LPCHARACTER center, int iGem)
			: total(0), c(center), x(center->GetX()), y(center->GetY()), iGem(iGem)
		{
		}

		void operator ()(LPCHARACTER ch)
		{
			if (ch != c)
				if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				{
					ch->PointChange(POINT_GEM, iGem, true);
				}
		}
	};
#endif

}

void CHARACTER::GiveGold(long long iAmount)
{
	if (iAmount <= 0)
		return;

	sys_log(0, "GIVE_GOLD: %s %lld", GetName(), iAmount);

	if (GetParty())
	{
		LPPARTY pParty = GetParty();

		long long dwTotal = iAmount;
		long long dwMyAmount = dwTotal;

		NPartyPickupDistribute::FCountNearMember funcCountNearMember(this);
		pParty->ForEachOnlineMember(funcCountNearMember);

		if (funcCountNearMember.total > 1)
		{
			long long dwShare = dwTotal / funcCountNearMember.total;
			dwMyAmount -= dwShare * (funcCountNearMember.total - 1);

			NPartyPickupDistribute::FMoneyDistributor funcMoneyDist(this, dwShare);

			pParty->ForEachOnlineMember(funcMoneyDist);
		}

		PointChange(POINT_GOLD, dwMyAmount, true);
		CHARACTER_MANAGER::Instance().DoMission(this, MISSION_EARN_MONEY, dwMyAmount, 0);
	}
	else
	{
		PointChange(POINT_GOLD, iAmount, true);
		CHARACTER_MANAGER::Instance().DoMission(this, MISSION_EARN_MONEY, iAmount, 0);
	}
}

#if defined(__GEM_SYSTEM__)
void CHARACTER::GiveGem(int iAmount)
{
	if (iAmount <= 0)
		return;

	sys_log(0, "GIVE_GEM: %s %lld", GetName(), iAmount);

	if (GetParty())
	{
		LPPARTY pParty = GetParty();

		DWORD dwTotal = iAmount;
		DWORD dwMyAmount = dwTotal;

		NPartyPickupDistribute::FCountNearMember funcCountNearMember(this);
		pParty->ForEachOnlineMember(funcCountNearMember);

		if (funcCountNearMember.total > 1)
		{
			DWORD dwShare = dwTotal / funcCountNearMember.total;
			dwMyAmount -= dwShare * (funcCountNearMember.total - 1);

			NPartyPickupDistribute::FGemDistributor funcGemDist(this, dwShare);

			pParty->ForEachOnlineMember(funcGemDist);
		}

		PointChange(POINT_GEM, dwMyAmount, true);
	}
	else
	{
		PointChange(POINT_GEM, iAmount, true);
	}
}
#endif

bool CHARACTER::PickupItem(DWORD dwVID)
{
	LPITEM item = ITEM_MANAGER::instance().FindByVID(dwVID);

	if (IsObserverMode())
		return false;

	if (PreventTradeWindow(WND_EXCHANGE, false))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Close all windows first."));
		return false;
	}

	if (!item || !item->GetSectree())
		return false;

	if (item->DistanceValid(this))
	{
		// @fixme150 BEGIN
		if (item->GetType() == ITEM_QUEST)
		{
			if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot pickup this item if you're using quests"));
				return false;
			}
		}
		// @fixme150 END

		if (item->IsOwnership(this))
		{
			if (item->GetType() == ITEM_ELK)
			{
				GiveGold(item->GetCount());
				item->RemoveFromGround();

				M2_DESTROY_ITEM(item);

				Save();
			}

			else
			{
#if defined(__EXTENDED_BLEND_AFFECT__)
				if (item->IsBlendItem() && !item->IsExtendedBlend(item->GetVnum()) && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD wCount = item->GetCount();

					for (int i = 0; i < Inventory_Size(); ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->IsBlendItem() && item->IsBlendItem())
						{
							bool bSameBlend = true;
							for (int j = 0; j < 2; ++j)
							{
								if (item2->GetSocket(j) != item->GetSocket(j))
								{
									bSameBlend = false;
									break;
								}
							}

							if (bSameBlend)
							{
								item2->SetSocket(2, item2->GetSocket(2) + item->GetSocket(2));
								item->SetCount(item->GetCount() - wCount);
								item->RemoveFromGround();

								M2_DESTROY_ITEM(item);
								return true;
							}
						}
					}
				}
				if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
#else
				if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
#endif
				{
					DWORD bCount = item->GetCount();

					for (int i = 0; i < Inventory_Size(); ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

							DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
								#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetName());
								#endif
								M2_DESTROY_ITEM(item);
								if (item2->GetType() == ITEM_QUEST)
									quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(bCount);
				}

#if defined(__SPECIAL_INVENTORY_SYSTEM__)

				if (item->IsSkillBook() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD wCount = item->GetCount();

					for (int i = SKILL_BOOK_INVENTORY_SLOT_START; i < SKILL_BOOK_INVENTORY_SLOT_START+Skill_Inventory_Size(); ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							if (item2->GetCount() == ITEM_MAX_COUNT)
								continue;
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

							DWORD wCount2 = MIN(ITEM_MAX_COUNT - item2->GetCount(), wCount);
							wCount -= wCount2;

							item2->SetCount(item2->GetCount() + wCount2);

							if (wCount == 0)
							{
#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item2->GetLocaleName(GetLanguage()));
#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item2->GetLocaleName(GetLanguage()));
#endif
								M2_DESTROY_ITEM(item);

								//if (item2->GetType() == ITEM_QUEST)
								quest::CQuestManager::instance().PickupItem(GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(wCount);
				}
				if (item->IsUpgradeItem() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD wCount = item->GetCount();

					for (int i = UPGRADE_ITEMS_INVENTORY_SLOT_START; i < UPGRADE_ITEMS_INVENTORY_SLOT_START+Material_Inventory_Size(); ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							if (item2->GetCount() == ITEM_MAX_COUNT)
								continue;
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

							DWORD wCount2 = MIN(ITEM_MAX_COUNT - item2->GetCount(), wCount);
							wCount -= wCount2;

							item2->SetCount(item2->GetCount() + wCount2);

							if (wCount == 0)
							{
#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item2->GetLocaleName(GetLanguage()));
#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item2->GetLocaleName(GetLanguage()));
#endif
								M2_DESTROY_ITEM(item);

								//if (item2->GetType() == ITEM_QUEST)
								quest::CQuestManager::instance().PickupItem(GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(wCount);
				}
				if (item->IsStone() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD wCount = item->GetCount();

					for (int i = STONE_INVENTORY_SLOT_START; i < STONE_INVENTORY_SLOT_START+Stone_Inventory_Size(); ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							if (item2->GetCount() == ITEM_MAX_COUNT)
								continue;
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

							DWORD wCount2 = MIN(ITEM_MAX_COUNT - item2->GetCount(), wCount);
							wCount -= wCount2;

							item2->SetCount(item2->GetCount() + wCount2);

							if (wCount == 0)
							{
#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item2->GetLocaleName(GetLanguage()));
#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item2->GetLocaleName(GetLanguage()));
#endif
								M2_DESTROY_ITEM(item);

								//if (item2->GetType() == ITEM_QUEST)
								quest::CQuestManager::instance().PickupItem(GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(wCount);
				}
				if (item->IsGiftBox() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD wCount = item->GetCount();

					for (int i = GIFT_BOX_INVENTORY_SLOT_START; i < GIFT_BOX_INVENTORY_SLOT_START+Chest_Inventory_Size(); ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							if (item2->GetCount() == ITEM_MAX_COUNT)
								continue;
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

							DWORD wCount2 = MIN(ITEM_MAX_COUNT - item2->GetCount(), wCount);
							wCount -= wCount2;

							item2->SetCount(item2->GetCount() + wCount2);

							if (wCount == 0)
							{
#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item2->GetLocaleName(GetLanguage()));
#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item2->GetLocaleName(GetLanguage()));
#endif
								M2_DESTROY_ITEM(item);

								//if (item2->GetType() == ITEM_QUEST)
								quest::CQuestManager::instance().PickupItem(GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(wCount);
				}
				if (item->IsSwitch() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD wCount = item->GetCount();

					for (int i = SWITCH_INVENTORY_SLOT_START; i < SWITCH_INVENTORY_SLOT_START+Enchant_Inventory_Size(); ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							if (item2->GetCount() == ITEM_MAX_COUNT)
								continue;

							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

							DWORD wCount2 = MIN(ITEM_MAX_COUNT - item2->GetCount(), wCount);
							wCount -= wCount2;

							item2->SetCount(item2->GetCount() + wCount2);

							if (wCount == 0)
							{
#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item2->GetLocaleName(GetLanguage()));
#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item2->GetLocaleName(GetLanguage()));
#endif
								M2_DESTROY_ITEM(item);

								//if (item2->GetType() == ITEM_QUEST)
								quest::CQuestManager::instance().PickupItem(GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(wCount);
				}
				if (item->IsCostume() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD wCount = item->GetCount();

					for (int i = COSTUME_INVENTORY_SLOT_START; i < COSTUME_INVENTORY_SLOT_START+Costume_Inventory_Size(); ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							if (item2->GetCount() == ITEM_MAX_COUNT)
								continue;

							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

							DWORD wCount2 = MIN(ITEM_MAX_COUNT - item2->GetCount(), wCount);
							wCount -= wCount2;

							item2->SetCount(item2->GetCount() + wCount2);

							if (wCount == 0)
							{
#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item2->GetLocaleName(GetLanguage()));
#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item2->GetLocaleName(GetLanguage()));
#endif
								M2_DESTROY_ITEM(item);

								//if (item2->GetType() == ITEM_QUEST)
								quest::CQuestManager::instance().PickupItem(GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(wCount);
				}
#endif

				int iEmptyCell;
				if (item->IsDragonSoul())
				{
					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						sys_log(0, "No empty ds inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
				else if (item->IsSkillBook())
				{
					if ((iEmptyCell = GetEmptySkillBookInventory(item->GetSize())) == -1)
					{
						sys_log(0, "No empty skill book inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}
				else if (item->IsUpgradeItem())
				{
					if ((iEmptyCell = GetEmptyUpgradeItemsInventory(item->GetSize())) == -1)
					{
						sys_log(0, "No empty refine inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}
				else if (item->IsStone())
				{
					if ((iEmptyCell = GetEmptyStoneInventory(item->GetSize())) == -1)
					{
						sys_log(0, "No empty stone inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}
				else if (item->IsGiftBox())
				{
					if ((iEmptyCell = GetEmptyGiftBoxInventory(item->GetSize())) == -1)
					{
						sys_log(0, "No empty gift box inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}

				else if (item->IsSwitch())
				{
					if ((iEmptyCell = GetEmptySwitchInventory(item->GetSize())) == -1)
					{
						sys_log(0, "No empty switch inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}
#endif
				else
				{
					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						sys_log(0, "No empty inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}

				item->RemoveFromGround();

				if (item->IsDragonSoul())
					item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
				else if (item->IsSkillBook())
				{
					int iOpenCount = GetQuestFlag("special_inventory.open_count");
					if (iOpenCount < 3)
					{
						ChatPacket(CHAT_TYPE_COMMAND, "OpenSpecialInventoryWindow %d", 0);
						SetQuestFlag("special_inventory.open_count", iOpenCount + 1);
					}
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
				}
				else if (item->IsUpgradeItem())
				{
					int iOpenCount = GetQuestFlag("special_inventory.open_count");
					if (iOpenCount < 3)
					{
						ChatPacket(CHAT_TYPE_COMMAND, "OpenSpecialInventoryWindow %d", 1);
						SetQuestFlag("special_inventory.open_count", iOpenCount + 1);
					}
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
				}
				else if (item->IsStone())
				{
					int iOpenCount = GetQuestFlag("special_inventory.open_count");
					if (iOpenCount < 3)
					{
						ChatPacket(CHAT_TYPE_COMMAND, "OpenSpecialInventoryWindow %d", 2);
						SetQuestFlag("special_inventory.open_count", iOpenCount + 1);
					}
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
				}
				else if (item->IsGiftBox())
				{
					int iOpenCount = GetQuestFlag("special_inventory.open_count");
					if (iOpenCount < 3)
					{
						ChatPacket(CHAT_TYPE_COMMAND, "OpenSpecialInventoryWindow %d", 3);
						SetQuestFlag("special_inventory.open_count", iOpenCount + 1);
					}
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
				}
				else if (item->IsSwitch())
				{
					int iOpenCount = GetQuestFlag("special_inventory.open_count");
					if (iOpenCount < 3)
					{
						ChatPacket(CHAT_TYPE_COMMAND, "OpenSpecialInventoryWindow %d", 4);
						SetQuestFlag("special_inventory.open_count", iOpenCount + 1);
					}
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
				}
#endif
				else
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));

				char szHint[32+1];
				snprintf(szHint, sizeof(szHint), "%s %u %u", item->GetName(), item->GetCount(), item->GetOriginalVnum());
				LogManager::instance().ItemLog(this, item, "GET_FROM_GROUND", szHint);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));

				if (item->GetType() == ITEM_QUEST)
					quest::CQuestManager::instance().PickupItem (GetPlayerID(), item);
			}

			//Motion(MOTION_PICKUP);
			return true;
		}
		else if (!IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_DROP) && GetParty() || item->GetSocket(4) == 99)
		{
			NPartyPickupDistribute::FFindOwnership funcFindOwnership(item);

			GetParty()->ForEachOnlineMember(funcFindOwnership);

			LPCHARACTER owner = funcFindOwnership.owner;
			// @fixme115
			if (!owner)
				return false;

			int iEmptyCell;

			if (item->IsDragonSoul())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyDragonSoulInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}
			}
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
			else if (item->IsSkillBook())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptySkillBookInventory(item->GetSize())) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptySkillBookInventory(item->GetSize())) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}
			}
			else if (item->IsUpgradeItem())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyUpgradeItemsInventory(item->GetSize())) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyUpgradeItemsInventory(item->GetSize())) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}
			}
			else if (item->IsStone())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyStoneInventory(item->GetSize())) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyStoneInventory(item->GetSize())) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}
			}
			else if (item->IsGiftBox())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyGiftBoxInventory(item->GetSize())) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyGiftBoxInventory(item->GetSize())) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}
			}
			else if (item->IsSwitch())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptySwitchInventory(item->GetSize())) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptySwitchInventory(item->GetSize())) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}
			}
#endif
			else
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyInventory(item->GetSize())) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지하고 있는 아이템이 너무 많습니다."));
						return false;
					}
				}
			}

			item->RemoveFromGround();

			if (item->IsDragonSoul())
				item->AddToCharacter(owner, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
			else if (item->IsSkillBook())
				item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));
			else if (item->IsUpgradeItem())
				item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));
			else if (item->IsStone())
				item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));
			else if (item->IsGiftBox())
				item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));
			else if (item->IsSwitch())
				item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));
#endif
			else
				item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));

			char szHint[32+1];
			snprintf(szHint, sizeof(szHint), "%s %u %u", item->GetName(), item->GetCount(), item->GetOriginalVnum());
			LogManager::instance().ItemLog(owner, item, "GET_FROM_GROUND", szHint);

			if (owner == this)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
			else
			{
				owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s 님으로부터 %s"), GetName(), item->GetLocaleName(GetLanguage()));
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 전달: %s 님에게 %s"), owner->GetName(), item->GetLocaleName(GetLanguage()));
			}

			if (item->GetType() == ITEM_QUEST)
				quest::CQuestManager::instance().PickupItem (owner->GetPlayerID(), item);

			return true;
		}
	}

	return false;
}

bool CHARACTER::SwapItem(WORD bCell, WORD bDestCell)
{
	if (!CanHandleItem())
		return false;

	TItemPos srcCell(INVENTORY, bCell), destCell(INVENTORY, bDestCell);

	//if (bCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM || bDestCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM)
	if (srcCell.IsDragonSoulEquipPosition() || destCell.IsDragonSoulEquipPosition())
		return false;

	if (bCell == bDestCell)
		return false;

	if (srcCell.IsEquipPosition() && destCell.IsEquipPosition())
		return false;

	LPITEM item1, item2;

	if (srcCell.IsEquipPosition())
	{
		item1 = GetInventoryItem(bDestCell);
		item2 = GetInventoryItem(bCell);
	}
	else
	{
		item1 = GetInventoryItem(bCell);
		item2 = GetInventoryItem(bDestCell);
	}

	if (!item1 || !item2)
		return false;

	if ((bCell >= COSTUME_INVENTORY_SLOT_START) && (bCell <= COSTUME_INVENTORY_SLOT_END) && (bDestCell >= INVENTORY_MAX_NUM) && (bDestCell <= INVENTORY_MAX_NUM + WEAR_MAX_NUM))
	{
		if ((item1->GetType() == ITEM_COSTUME && item2->GetType() == ITEM_ARMOR) || (item1->GetType() == ITEM_ARMOR && item2->GetType() == ITEM_COSTUME))
			return false;
	}

	if (item1 == item2)
	{
		sys_log(0, "[WARNING][WARNING][HACK USER!] : %s %d %d", m_stName.c_str(), bCell, bDestCell);
		return false;
	}

	if (!IsEmptyItemGrid(TItemPos (INVENTORY, item1->GetCell()), item2->GetSize(), item1->GetCell()))
		return false;

	if (TItemPos(EQUIPMENT, item2->GetCell()).IsEquipPosition())
	{
		WORD bEquipCell = item2->GetCell() - INVENTORY_MAX_NUM;
		WORD bInvenCell = item1->GetCell();

		if (item2->IsDragonSoul() || item2->GetType() == ITEM_BELT) // @fixme117
		{
			if (false == CanUnequipNow(item2) || false == CanEquipNow(item1))
				return false;
		}

		if (bEquipCell != item1->FindEquipCell(this))
			return false;

		item2->RemoveFromCharacter();

		if (item2->GetType() == ITEM_WEAPON)
		{
			if (IsAffectFlag(AFF_GWIGUM))
				RemoveAffect(SKILL_GWIGEOM);

			if (IsAffectFlag(AFF_GEOMGYEONG))
				RemoveAffect(SKILL_GEOMKYUNG);
		}

		if (item1->EquipTo(this, bEquipCell))
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item2->AddToCharacter(this, TItemPos(INVENTORY, bInvenCell), false);
#else
			item2->AddToCharacter(this, TItemPos(INVENTORY, bInvenCell));
#endif
		else
			sys_err("SwapItem cannot %s! item1 %s", item2->GetLocaleName(GetLanguage()), item1->GetLocaleName(GetLanguage()));
	}
	else
	{
		WORD bCell1 = item1->GetCell();
		WORD bCell2 = item2->GetCell();

		item1->RemoveFromCharacter();
		item2->RemoveFromCharacter();

#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
		item1->AddToCharacter(this, TItemPos(INVENTORY, bCell2), false);
		item2->AddToCharacter(this, TItemPos(INVENTORY, bCell1), false);
#else
		item1->AddToCharacter(this, TItemPos(INVENTORY, bCell2));
		item2->AddToCharacter(this, TItemPos(INVENTORY, bCell1));
#endif
	}

	return true;
}

bool CHARACTER::UnequipItem(LPITEM item)
{
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	int iWearCell = item->FindEquipCell(this);
	if (iWearCell == WEAR_WEAPON)
	{
		LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
		if (costumeWeapon && !UnequipItem(costumeWeapon))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot unequip the costume weapon. Not enough space."));
			return false;
		}
	}
#endif

	if (item->IsAcce() && GetWear(WEAR_COSTUME_ACCE_SKIN))
	{
		LPITEM costumeAcce = GetWear(WEAR_COSTUME_ACCE_SKIN);
		if (costumeAcce && !UnequipItem(costumeAcce))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot unequip the acce. You need to take down Acce Skin first."));
			return false;
		}
	}

	if (IsRiding() && (item->IsCostumeMountItem() || item->IsSkinMount()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't unequip a mount while riding."));
		return false;
	}

	if (item->IsCostumeMountItem() && GetWear(WEAR_COSTUME_MOUNT_SKIN))
	{
		auto costumeMount = GetWear(WEAR_COSTUME_MOUNT_SKIN);
		if (costumeMount && !UnequipItem(costumeMount))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to unequip first skin."));
			return false;
		}
	}

	if (item->IsCostumePetItem() && GetWear(WEAR_COSTUME_NORMAL_PET_SKIN))
	{
		auto costumePet = GetWear(WEAR_COSTUME_NORMAL_PET_SKIN);
		if (costumePet && !UnequipItem(costumePet))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to unequip first skin."));
			return false;
		}
	}

	if (false == CanUnequipNow(item))
		return false;

	int pos;
	if (item->IsDragonSoul())
		pos = GetEmptyDragonSoulInventory(item);
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	else if (item->IsSkillBook())
		pos = GetEmptySkillBookInventory(item->GetSize());
	else if (item->IsUpgradeItem())
		pos = GetEmptyUpgradeItemsInventory(item->GetSize());
	else if (item->IsStone())
		pos = GetEmptyStoneInventory(item->GetSize());
	else if (item->IsGiftBox())
		pos = GetEmptyGiftBoxInventory(item->GetSize());
	else if (item->IsSwitch())
		pos = GetEmptySwitchInventory(item->GetSize());
#endif
	else
		pos = GetEmptyInventory(item->GetSize());

	// HARD CODING
	if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		ShowAlignment(true);

#ifdef __SKIN_SYSTEM__
	if (item->IsSkinMount())
	{
		if (GetHorse())
		{
			HorseSummon(false);
			LPITEM mount = GetWear(WEAR_COSTUME_MOUNT);
			HorseSummon(true, false, mount->GetValue(3));
		}
	}
#endif

	if (item->IsCostumeMountItem())
	{
		if (GetHorse())
			HorseSummon(false);
	}

	if (item->IsRemoveAfterUnequip())
	{
		CheckMaximumPoints();
		ITEM_MANAGER::instance().RemoveItem(item);
		return true;
	}
	
	item->RemoveFromCharacter();
	if (item->IsDragonSoul())
	{
		item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, pos));
	}
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	else if (item->IsSkillBook())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
		item->AddToCharacter(this, TItemPos(INVENTORY, pos), false);
#else
		item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
	else if (item->IsUpgradeItem())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
		item->AddToCharacter(this, TItemPos(INVENTORY, pos), false);
#else
		item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
	else if (item->IsStone())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
		item->AddToCharacter(this, TItemPos(INVENTORY, pos), false);
#else
		item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
	else if (item->IsGiftBox())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
		item->AddToCharacter(this, TItemPos(INVENTORY, pos), false);
#else
		item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
	else if (item->IsSwitch())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
		item->AddToCharacter(this, TItemPos(INVENTORY, pos), false);
#else
		item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
#endif
	else
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
		item->AddToCharacter(this, TItemPos(INVENTORY, pos), false);
#else
		item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif

#ifdef __SKIN_SYSTEM__
	if (item->IsSkinPet())
	{
		CPetSystem* pet = GetPetSystem();
		if (pet)
			pet->HandlePetCostumeItem();
	}
#endif

	CheckMaximumPoints();

	return true;
}

//

//
bool CHARACTER::EquipItem(LPITEM item, int iCandidateCell)
{
	if (item->IsExchanging())
		return false;

	if (false == item->IsEquipable())
		return false;

	if (false == CanEquipNow(item))
		return false;

	int iWearCell = item->FindEquipCell(this, iCandidateCell);

	if (iWearCell < 0)
		return false;

	if (iWearCell == WEAR_BODY && IsRiding() && (item->GetVnum() >= 11901 && item->GetVnum() <= 11904))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 탄 상태에서 예복을 입을 수 없습니다."));
		return false;
	}

	if (item->GetCount() > 1) 
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("In order to be able to equip this object, you must separate it first."));
		return false;
	}

	if (iWearCell != WEAR_ARROW && IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑 중에는 착용중인 장비를 변경할 수 없습니다."));
		return false;
	}

	if (FN_check_item_sex(this, item) == false)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("성별이 맞지않아 이 아이템을 사용할 수 없습니다."));
		return false;
	}

	if(item->IsRideItem() && IsRiding())
	{
		if (g_NoMountAtGuildWar && GetWarMap())
		{
			if (IsRiding())
				StopRiding();
			return false;
		}
	}

	DWORD dwCurTime = get_dword_time();

	if (iWearCell != WEAR_ARROW
		&& (dwCurTime - GetLastAttackTime() <= 1500 || dwCurTime - m_dwLastSkillTime <= 1500))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("가만히 있을 때만 착용할 수 있습니다."));
		return false;
	}

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (iWearCell == WEAR_WEAPON)
	{
		if (item->GetType() == ITEM_WEAPON)
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && costumeWeapon->GetValue(3) != item->GetSubType() && !UnequipItem(costumeWeapon))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot unequip the costume weapon. Not enough space."));
				return false;
			}
		}
		else //fishrod/pickaxe
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !UnequipItem(costumeWeapon))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot unequip the costume weapon. Not enough space."));
				return false;
			}
		}
	}
	else if (iWearCell == WEAR_COSTUME_WEAPON)
	{
#ifdef __BUFFI_SUPPORT__
		if (IsLookingBuffiPage())
		{
			if (item->GetValue(3) != WEAPON_BELL && item->GetValue(3) != WEAPON_FAN)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't wear a weapon costume which has a different type than your weapon."));
				return false;
			}
			return true;
		}
#endif
		if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_WEAPON)
		{
			LPITEM pkWeapon = GetWear(WEAR_WEAPON);
			if (!pkWeapon || pkWeapon->GetType() != ITEM_WEAPON || item->GetValue(3) != pkWeapon->GetSubType())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot equip the costume weapon. Wrong equipped weapon."));
				return false;
			}
		}
	}
#endif

	if (item->IsDragonSoul())
	{
		if(GetInventoryItem(INVENTORY_MAX_NUM + iWearCell))
		{
			ChatPacket(CHAT_TYPE_INFO, "이미 같은 종류의 용혼석을 착용하고 있습니다.");
			return false;
		}

		if (!item->EquipTo(this, iWearCell))
		{
			return false;
		}
	}
	else
	{
		if (GetWear(iWearCell) && !IS_SET(GetWear(iWearCell)->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		{
			if (item->GetWearFlag() == WEARABLE_ABILITY)
				return false;

			if (false == SwapItem(item->GetCell(), INVENTORY_MAX_NUM + iWearCell))
			{
				return false;
			}
		}
		else
		{
			WORD bOldCell = item->GetCell();

			if (item->EquipTo(this, iWearCell))
			{
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, bOldCell, iWearCell);
			}
		}
	}

	if (true == item->IsEquipped())
	{
		if (IS_SET(item->GetFlag(), ITEM_FLAG_CONFIRM_WHEN_USE) && item->GetSocket(5) == 0)
			item->SetSocket(5, 1, false);

		if (-1 != item->GetProto()->cLimitRealTimeFirstUseIndex)
		{
			if (0 == item->GetSocket(1))
			{
				long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[(unsigned char)(item->GetProto()->cLimitRealTimeFirstUseIndex)].lValue;

				if (0 == duration)
					duration = 60 * 60 * 24 * 7;

				item->SetSocket(0, time(0) + duration);
				item->StartRealTimeExpireEvent();
			}

			item->SetSocket(1, item->GetSocket(1) + 1);
		}

		if (item->IsNewPetItem())
			item->StartRealTimeExpireEvent();

		if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
			ShowAlignment(false);

		const DWORD& dwVnum = item->GetVnum();

		if (true == CItemVnumHelper::IsRamadanMoonRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_RAMADAN_RING);
		}

		else if (true == CItemVnumHelper::IsHalloweenCandy(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HALLOWEEN_CANDY);
		}

		else if (true == CItemVnumHelper::IsHappinessRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HAPPINESS_RING);
		}

		else if (true == CItemVnumHelper::IsLovePendant(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_LOVE_PENDANT);
		}
		else if (ITEM_UNIQUE == item->GetType() && 0 != item->GetSIGVnum())
		{
			const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(item->GetSIGVnum());
			if (NULL != pGroup)
			{
				const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(pGroup->GetAttrVnum(item->GetVnum()));
				if (NULL != pAttrGroup)
				{
					const std::string& std = pAttrGroup->m_stEffectFileName;
					SpecificEffectPacket(std.c_str());
				}
			}
		}
		#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		else if ((item->GetType() == ITEM_COSTUME) && (item->GetSubType() == COSTUME_ACCE))
			this->EffectPacket(SE_EFFECT_ACCE_EQUIP);
		#endif

		if (item->IsOldMountItem()) // @fixme152
			quest::CQuestManager::instance().SIGUse(GetPlayerID(), quest::QUEST_NO_NPC, item, false);
	}

#ifdef __RENEWAL_MOUNT__
	if (item->IsCostumeMountItem())
	{
		if (IsRiding())
		{
			LPITEM costumeItem = GetWear(WEAR_COSTUME_MOUNT_SKIN);
			if (!costumeItem)
			{
				StopRiding();

				if (GetHorse())
					HorseSummon(false);

				if (costumeItem)
					HorseSummon(true, false, costumeItem->GetValue(3));
				else
					HorseSummon(true, false, item->GetValue(3));
				
				StartRiding();
			}
			else
			{
				StopRiding();
				if (GetHorse())
					HorseSummon(false);
				HorseSummon(true, false, costumeItem->GetValue(3));
				StartRiding();
			}
		}
		else
		{
			if (GetHorse())
				HorseSummon(false);
			LPITEM costumeItem = GetWear(WEAR_COSTUME_MOUNT_SKIN);
			if (costumeItem)
				HorseSummon(true, false, costumeItem->GetValue(3));
			else
				HorseSummon(true, false, item->GetValue(3));
		}
	}
#endif
#ifdef __SKIN_SYSTEM__
	if (item->IsSkinMount())
	{
		if (IsRiding())
		{
			StopRiding();
			if (GetHorse())
				HorseSummon(false);
			HorseSummon(true, true, item->GetValue(3));
			StartRiding();
		}
		else
		{
			if (GetHorse())
				HorseSummon(false);
			HorseSummon(true, true, item->GetValue(3));
		}
	}
#endif

	return true;
}

void CHARACTER::BuffOnAttr_AddBuffsFromItem(LPITEM pItem)
{
	for (size_t i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->AddBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem)
{
	for (size_t i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->RemoveBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_ClearAll()
{
	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin(); it != m_map_buff_on_attrs.end(); it++)
	{
		CBuffOnAttributes* pBuff = it->second;
		if (pBuff)
		{
			pBuff->Initialize();
		}
	}
}

void CHARACTER::BuffOnAttr_ValueChange(BYTE bType, BYTE bOldValue, BYTE bNewValue)
{
	TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(bType);

	if (0 == bNewValue)
	{
		if (m_map_buff_on_attrs.end() == it)
			return;
		else
			it->second->Off();
	}
	else if(0 == bOldValue)
	{
		CBuffOnAttributes* pBuff = NULL;
		if (m_map_buff_on_attrs.end() == it)
		{
			switch (bType)
			{
			case POINT_ENERGY:
				{
					static BYTE abSlot[] = { WEAR_BODY, WEAR_HEAD, WEAR_FOOTS, WEAR_WRIST, WEAR_WEAPON, WEAR_NECK, WEAR_EAR, WEAR_SHIELD };
					static std::vector <BYTE> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			case POINT_COSTUME_ATTR_BONUS:
				{
					static BYTE abSlot[] = {
						WEAR_COSTUME_BODY,
						WEAR_COSTUME_HAIR,
						#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
						WEAR_COSTUME_MOUNT,
						#endif
						#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
						WEAR_COSTUME_WEAPON,
						#endif
					};
					static std::vector <BYTE> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			default:
				break;
			}
			m_map_buff_on_attrs.emplace(bType, pBuff);

		}
		else
			pBuff = it->second;
		if (pBuff != NULL)
			pBuff->On(bNewValue);
	}
	else
	{
		assert (m_map_buff_on_attrs.end() != it);
		it->second->ChangeBuffValue(bNewValue);
	}
}

LPITEM CHARACTER::FindSpecifyItem(DWORD vnum) const
{
	for (int i = 0; i < Inventory_Size(); ++i)
	{
		if (GetInventoryItem(i) && GetInventoryItem(i)->GetVnum() == vnum)
			return GetInventoryItem(i);
	}

	for (int i = SKILL_BOOK_INVENTORY_SLOT_START; i < COSTUME_INVENTORY_SLOT_END; ++i)
	{
		if (GetInventoryItem(i) && GetInventoryItem(i)->GetVnum() == vnum)
			return GetInventoryItem(i);
	}

	return NULL;
}

LPITEM CHARACTER::FindItemByID(DWORD id) const
{
	for (int i=0 ; i < Inventory_Size() ; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	for (int i=BELT_INVENTORY_SLOT_START; i < BELT_INVENTORY_SLOT_END ; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	for (int i = SKILL_BOOK_INVENTORY_SLOT_START; i < COSTUME_INVENTORY_SLOT_END; ++i)
	{
		if (GetInventoryItem(i) && GetInventoryItem(i)->GetVnum() == id)
			return GetInventoryItem(i);
	}

	return NULL;
}

int CHARACTER::CountSpecifyItem(DWORD vnum, int iExceptionCell) const
{
	int	count = 0;
	LPITEM item;

	for (int i = 0; i < Inventory_Size(); ++i)
	{
		if(i == iExceptionCell)
			continue;

		item = GetInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
			{
				continue;
			}
			else
			{
				count += item->GetCount();
			}
		}
	}

#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	for (int i = SKILL_BOOK_INVENTORY_SLOT_START; i < COSTUME_INVENTORY_SLOT_END; ++i)
	{
		item = GetInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			// 개인 상점에 등록된 물건이면 넘어간다.
			if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
			{
				continue;
			}
			else
			{
				count += item->GetCount();
			}
		}
	}
#endif

	return count;
}

void CHARACTER::RemoveSpecifyItem(DWORD vnum, DWORD count, int iExceptionCell)
{
	if (0 == count)
		return;

	for (UINT i = 0; i < Inventory_Size(); ++i)
	{
		if(i == iExceptionCell)
			continue;

		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetVnum() != vnum)
			continue;

		if(m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}

#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	for (UINT i = SKILL_BOOK_INVENTORY_SLOT_START; i < COSTUME_INVENTORY_SLOT_END; ++i)
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetVnum() != vnum)
			continue;

		//개인 상점에 등록된 물건이면 넘어간다. (개인 상점에서 판매될때 이 부분으로 들어올 경우 문제!)
		if (m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}
#endif

	if (count)
		sys_log(0, "CHARACTER::RemoveSpecifyItem cannot remove enough item vnum %u, still remain %d", vnum, count);
}

int CHARACTER::CountSpecifyTypeItem(BYTE type) const
{
	int	count = 0;

	for (int i = 0; i < Inventory_Size(); ++i)
	{
		LPITEM pItem = GetInventoryItem(i);
		if (pItem != NULL && pItem->GetType() == type)
		{
			count += pItem->GetCount();
		}
	}

	return count;
}

void CHARACTER::RemoveSpecifyTypeItem(BYTE type, DWORD count)
{
	if (0 == count)
		return;

	for (UINT i = 0; i < Inventory_Size(); ++i)
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetType() != type)
			continue;

		if(m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}

#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	for (UINT i = SKILL_BOOK_INVENTORY_SLOT_START; i < COSTUME_INVENTORY_SLOT_END; ++i)
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetType() != type)
			continue;

		// 개인 상점에 등록된 물건이면 넘어간다. (개인 상점에서 판매될때 이 부분으로 들어올 경우 문제!)
		if (m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}
#endif

}

void CHARACTER::AutoGiveItemType(LPITEM item, BYTE type, bool longOwnerShip)
{
	if (NULL == item)
	{
		sys_err ("NULL point.");
		return;
	}
	if (item->GetOwner())
	{
		sys_err ("item %d 's owner exists!",item->GetID());
		return;
	}

	int cell;
	if (type == DRAGON_SOUL_INVENTORY)
	{
		cell = GetEmptyDragonSoulInventory(item);
	}
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	else if (type == SKILL_BOOK_INVENTORY)
	{
		cell = GetEmptySkillBookInventory(item->GetSize());
	}
	else if (type == UPGRADE_ITEMS_INVENTORY)
	{
		cell = GetEmptyUpgradeItemsInventory(item->GetSize());
	}
	else if (type == STONE_INVENTORY)
	{
		cell = GetEmptyStoneInventory(item->GetSize());
	}
	else if (type == GIFT_BOX_INVENTORY)
	{
		cell = GetEmptyGiftBoxInventory(item->GetSize());
	}
	else if (type == SWITCH_INVENTORY)
	{
		cell = GetEmptySwitchInventory(item->GetSize());
	}
	else if (type == COSTUME_INVENTORY)
	{
		cell = GetEmptyCostumeInventory(item->GetSize());
	}
#endif
	else if (type == INVENTORY)
	{
		cell = GetEmptyInventory (item->GetSize());
	}

	if (cell != -1)
	{
		if (type == DRAGON_SOUL_INVENTORY)
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, cell));
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		else if (type == SKILL_BOOK_INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
		else if (type == UPGRADE_ITEMS_INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
		else if (type == STONE_INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
		else if (type == GIFT_BOX_INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
		else if (type == SWITCH_INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
		else if (type == COSTUME_INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
#endif
		else if (type == INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = (DWORD)cell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround2(GetMapIndex(), GetXYZ(), false, this, 300);
#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
#else
		item->StartDestroyEvent();
#endif
	}
}

void CHARACTER::AutoGiveItem(LPITEM item, bool longOwnerShip)
{
	if (NULL == item)
	{
		sys_err ("NULL point.");
		return;
	}
	if (item->GetOwner())
	{
		sys_err ("item %d 's owner exists!",item->GetID());
		return;
	}

	int cell;
	if (item->IsDragonSoul())
	{
		cell = GetEmptyDragonSoulInventory(item);
	}
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	else if (item->IsSkillBook())
	{
		cell = GetEmptySkillBookInventory(item->GetSize());
	}
	else if (item->IsUpgradeItem())
	{
		cell = GetEmptyUpgradeItemsInventory(item->GetSize());
	}
	else if (item->IsStone())
	{
		cell = GetEmptyStoneInventory(item->GetSize());
	}
	else if (item->IsGiftBox())
	{
		cell = GetEmptyGiftBoxInventory(item->GetSize());
	}
	else if (item->IsSwitch())
	{
		cell = GetEmptySwitchInventory(item->GetSize());
	}
	else if (item->IsCostume())
	{
		cell = GetEmptyCostumeInventory(item->GetSize());
	}
#endif
	else
	{
		cell = GetEmptyInventory (item->GetSize());
	}

	if (cell != -1)
	{
		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, cell));
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		else if (item->IsSkillBook())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
		else if (item->IsUpgradeItem())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
		else if (item->IsStone())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
		else if (item->IsGiftBox())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
		else if (item->IsSwitch())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
		else if (item->IsCostume())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif
#endif
		else
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, cell), false);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = (DWORD)cell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround2(GetMapIndex(), GetXYZ(), false, this, 300);
#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
#else
		item->StartDestroyEvent();
#endif
	}
}

LPITEM CHARACTER::AutoGiveItem(DWORD dwItemVnum, DWORD bCount, int iRarePct, bool bMsg)
{
	TItemTable * p = ITEM_MANAGER::instance().GetTable(dwItemVnum);

	if (!p)
		return NULL;

	DBManager::instance().SendMoneyLog(MONEY_LOG_DROP, dwItemVnum, bCount);

	if (p->dwFlags & ITEM_FLAG_STACKABLE && p->bType != ITEM_BLEND)
	{
		for (int i = 0; i < Inventory_Size(); ++i)
		{
			LPITEM item = GetInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (item->GetCount() == ITEM_MAX_COUNT)
					continue;

				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				DWORD bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2);

				if (bCount == 0)
				{
					if (bMsg)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));

					return item;
				}
			}
		}
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		for (int i = SKILL_BOOK_INVENTORY_SLOT_START; i < SKILL_BOOK_INVENTORY_SLOT_END; ++i)
		{
			LPITEM item = GetSkillBookInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (item->GetCount() == ITEM_MAX_COUNT)
					continue;

				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				WORD wCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
				bCount -= wCount2;

				item->SetCount(item->GetCount() + wCount2);

				if (bCount == 0)
				{
					if (bMsg)
#if defined(__CHATTING_WINDOW_RENEWAL__)
						ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
#else
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
#endif

					return item;
				}
			}
		}

		for (int i = UPGRADE_ITEMS_INVENTORY_SLOT_START; i < UPGRADE_ITEMS_INVENTORY_SLOT_END; ++i)
		{
			LPITEM item = GetUpgradeItemsInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (item->GetCount() == ITEM_MAX_COUNT)
					continue;
					
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				WORD wCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
				bCount -= wCount2;

				item->SetCount(item->GetCount() + wCount2);

				if (bCount == 0)
				{
					if (bMsg)
#if defined(__CHATTING_WINDOW_RENEWAL__)
						ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
#else
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
#endif

					return item;
				}
			}
		}

		for (int i = STONE_INVENTORY_SLOT_START; i < STONE_INVENTORY_SLOT_END; ++i)
		{
			LPITEM item = GetStoneInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (item->GetCount() == ITEM_MAX_COUNT)
					continue;
					
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				WORD wCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
				bCount -= wCount2;

				item->SetCount(item->GetCount() + wCount2);

				if (bCount == 0)
				{
					if (bMsg)
#if defined(__CHATTING_WINDOW_RENEWAL__)
						ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
#else
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
#endif

					return item;
				}
			}
		}

		for (int i = GIFT_BOX_INVENTORY_SLOT_START; i < GIFT_BOX_INVENTORY_SLOT_END; ++i)
		{
			LPITEM item = GetGiftBoxInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (item->GetCount() == ITEM_MAX_COUNT)
					continue;
					
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				WORD wCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
				bCount -= wCount2;

				item->SetCount(item->GetCount() + wCount2);

				if (bCount == 0)
				{
					if (bMsg)
#if defined(__CHATTING_WINDOW_RENEWAL__)
						ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
#else
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
#endif

					return item;
				}
			}
		}
#endif

		for (int i = SWITCH_INVENTORY_SLOT_START; i < SWITCH_INVENTORY_SLOT_END; ++i)
		{
			LPITEM item = GetSwitchInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (item->GetCount() == ITEM_MAX_COUNT)
					continue;
					
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				WORD wCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
				bCount -= wCount2;

				item->SetCount(item->GetCount() + wCount2);

				if (bCount == 0)
				{
					if (bMsg)
#if defined(__CHATTING_WINDOW_RENEWAL__)
						ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
#else
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
#endif

					return item;
				}
			}
		}

		for (int i = COSTUME_INVENTORY_SLOT_START; i < COSTUME_INVENTORY_SLOT_END; ++i)
		{
			LPITEM item = GetCostumeInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (item->GetCount() == ITEM_MAX_COUNT)
					continue;
					
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				WORD wCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
				bCount -= wCount2;

				item->SetCount(item->GetCount() + wCount2);

				if (bCount == 0)
				{
					if (bMsg)
#if defined(__CHATTING_WINDOW_RENEWAL__)
						ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
#else
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
#endif

					return item;
				}
			}
		}
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwItemVnum, bCount, 0, true);

	if (!item)
	{
		sys_err("cannot create item by vnum %u (name: %s)", dwItemVnum, GetName());
		return NULL;
	}

#if defined(__EXTENDED_BLEND_AFFECT__)
	if (item->GetType() == ITEM_BLEND && !item->IsExtendedBlend(item->GetVnum()))
#else
	if (item->GetType() == ITEM_BLEND)
#endif
	{
		for (int i=0; i < Inventory_Size(); i++)
		{
			LPITEM inv_item = GetInventoryItem(i);

			if (inv_item == NULL) continue;

			if (inv_item->GetType() == ITEM_BLEND)
			{
				if (inv_item->GetVnum() == item->GetVnum())
				{
					if (inv_item->GetSocket(0) == item->GetSocket(0) && inv_item->GetSocket(1) == item->GetSocket(1) && inv_item->GetCount() < g_bItemCountLimit)
					{
#if defined(__EXTENDED_BLEND_AFFECT__)
						inv_item->SetSocket(2, inv_item->GetSocket(2) + item->GetSocket(2));
#else
						inv_item->SetCount(inv_item->GetCount() + item->GetCount());
#endif
						return inv_item;
					}
				}
			}
		}
	}

	int iEmptyCell;
	if (item->IsDragonSoul())
	{
		iEmptyCell = GetEmptyDragonSoulInventory(item);
	}
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	else if (item->IsSkillBook())
		iEmptyCell = GetEmptySkillBookInventory(item->GetSize());
	else if (item->IsUpgradeItem())
		iEmptyCell = GetEmptyUpgradeItemsInventory(item->GetSize());
	else if (item->IsStone())
		iEmptyCell = GetEmptyStoneInventory(item->GetSize());
	else if (item->IsGiftBox())
		iEmptyCell = GetEmptyGiftBoxInventory(item->GetSize());
	else if (item->IsSwitch())
		iEmptyCell = GetEmptySwitchInventory(item->GetSize());
	else if (item->IsCostume())
		iEmptyCell = GetEmptyCostumeInventory(item->GetSize());
#endif
	else
		iEmptyCell = GetEmptyInventory(item->GetSize());

	if (iEmptyCell != -1)
	{
		if (bMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));

		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		else if (item->IsSkillBook())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell), iEmptyCell);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
		else if (item->IsUpgradeItem())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell), iEmptyCell);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
		else if (item->IsStone())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell), iEmptyCell);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
		else if (item->IsGiftBox())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell), iEmptyCell);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
		else if (item->IsSwitch())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell), iEmptyCell);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
		else if (item->IsCostume())
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell), iEmptyCell);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
#endif
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = (DWORD)iEmptyCell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround2(GetMapIndex(), GetXYZ(), false, this, 300);
#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
#else
		item->StartDestroyEvent();
#endif
	}

	sys_log(0,
		"7: %d %d", dwItemVnum, bCount);
	return item;
}

LPITEM CHARACTER::AutoGiveItemType(DWORD dwItemVnum, BYTE type, DWORD bCount, int iRarePct, bool bMsg)
{
	TItemTable * p = ITEM_MANAGER::instance().GetTable(dwItemVnum);

	if (!p)
		return NULL;

	DBManager::instance().SendMoneyLog(MONEY_LOG_DROP, dwItemVnum, bCount);

	if (p->dwFlags & ITEM_FLAG_STACKABLE && p->bType != ITEM_BLEND)
	{	
		switch (type)
		{
			case SKILL_BOOK_INVENTORY:
			{
				for (int i = SKILL_BOOK_INVENTORY_SLOT_START; i < SKILL_BOOK_INVENTORY_SLOT_END; ++i)
				{
					LPITEM item = GetSkillBookInventoryItem(i);

					if (!item)
						continue;

					if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
					{
						if (item->GetCount() == ITEM_MAX_COUNT)
							continue;

						if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
						{
							if (bCount < p->alValues[1])
								bCount = p->alValues[1];
						}

						WORD wCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
						bCount -= wCount2;

						item->SetCount(item->GetCount() + wCount2);

						if (bCount == 0)
						{
							if (bMsg)
		#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#endif

							return item;
						}
					}
				}
			}
			break;

			case UPGRADE_ITEMS_INVENTORY:
			{
				for (int i = UPGRADE_ITEMS_INVENTORY_SLOT_START; i < UPGRADE_ITEMS_INVENTORY_SLOT_END; ++i)
				{
					LPITEM item = GetUpgradeItemsInventoryItem(i);

					if (!item)
						continue;

					if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
					{
						if (item->GetCount() == ITEM_MAX_COUNT)
							continue;

						if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
						{
							if (bCount < p->alValues[1])
								bCount = p->alValues[1];
						}

						WORD wCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
						bCount -= wCount2;

						item->SetCount(item->GetCount() + wCount2);

						if (bCount == 0)
						{
							if (bMsg)
		#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#endif

							return item;
						}
					}
				}
			}
			break;

			case STONE_INVENTORY:
			{
				for (int i = STONE_INVENTORY_SLOT_START; i < STONE_INVENTORY_SLOT_END; ++i)
				{
					LPITEM item = GetStoneInventoryItem(i);
		
					if (!item)
						continue;
		
					if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
					{
						if (item->GetCount() == ITEM_MAX_COUNT)
							continue;

						if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
						{
							if (bCount < p->alValues[1])
								bCount = p->alValues[1];
						}
		
						WORD wCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
						bCount -= wCount2;
		
						item->SetCount(item->GetCount() + wCount2);
		
						if (bCount == 0)
						{
							if (bMsg)
		#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#endif
		
							return item;
						}
					}
				}
			}
			break;

			case GIFT_BOX_INVENTORY:
			{
				for (int i = GIFT_BOX_INVENTORY_SLOT_START; i < GIFT_BOX_INVENTORY_SLOT_END; ++i)
				{
					LPITEM item = GetGiftBoxInventoryItem(i);

					if (!item)
						continue;

					if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
					{
						if (item->GetCount() == ITEM_MAX_COUNT)
							continue;

						if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
						{
							if (bCount < p->alValues[1])
								bCount = p->alValues[1];
						}

						WORD wCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
						bCount -= wCount2;

						item->SetCount(item->GetCount() + wCount2);

						if (bCount == 0)
						{
							if (bMsg)
		#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#endif

							return item;
						}
					}
				}
			}
			break;

			case SWITCH_INVENTORY:
			{
				for (int i = SWITCH_INVENTORY_SLOT_START; i < SWITCH_INVENTORY_SLOT_END; ++i)
				{
					LPITEM item = GetCostumeInventoryItem(i);
		
					if (!item)
						continue;
		
					if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
					{
						if (item->GetCount() == ITEM_MAX_COUNT)
							continue;

						if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
						{
							if (bCount < p->alValues[1])
								bCount = p->alValues[1];
						}
		
						WORD wCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
						bCount -= wCount2;
		
						item->SetCount(item->GetCount() + wCount2);
		
						if (bCount == 0)
						{
							if (bMsg)
		#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#endif
		
							return item;
						}
					}
				}
			}
			break;

			case COSTUME_INVENTORY:
			{
				for (int i = COSTUME_INVENTORY_SLOT_START; i < COSTUME_INVENTORY_SLOT_END; ++i)
				{
					LPITEM item = GetCostumeInventoryItem(i);
		
					if (!item)
						continue;
		
					if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
					{
						if (item->GetCount() == ITEM_MAX_COUNT)
							continue;

						if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
						{
							if (bCount < p->alValues[1])
								bCount = p->alValues[1];
						}
		
						WORD wCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
						bCount -= wCount2;
		
						item->SetCount(item->GetCount() + wCount2);
		
						if (bCount == 0)
						{
							if (bMsg)
		#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#endif
		
							return item;
						}
					}
				}
			}
			break;

			case INVENTORY:
			{
				for (int i = 0; i < Inventory_Size(); ++i)
				{
					LPITEM item = GetInventoryItem(i);

					if (!item)
						continue;

					if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
					{
						if (item->GetCount() == ITEM_MAX_COUNT)
							continue;

						if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
						{
							if (bCount < p->alValues[1])
								bCount = p->alValues[1];
						}

						DWORD bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
						bCount -= bCount2;

						item->SetCount(item->GetCount() + bCount2);

						if (bCount == 0)
						{
							if (bMsg)
		#if defined(__CHATTING_WINDOW_RENEWAL__)
								ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));
		#endif
							return item;
						}
					}
				}
			}
			break;
		}
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwItemVnum, bCount, 0, true);

	if (!item)
	{
		sys_err("cannot create item by vnum %u (name: %s)", dwItemVnum, GetName());
		return NULL;
	}

#if defined(__EXTENDED_BLEND_AFFECT__)
	if (item->GetType() == ITEM_BLEND && !item->IsExtendedBlend(item->GetVnum()))
#else
	if (item->GetType() == ITEM_BLEND)
#endif
	{
		for (int i=0; i < Inventory_Size(); i++)
		{
			LPITEM inv_item = GetInventoryItem(i);

			if (inv_item == NULL) continue;

			if (inv_item->GetType() == ITEM_BLEND)
			{
				if (inv_item->GetVnum() == item->GetVnum())
				{
					if (inv_item->GetSocket(0) == item->GetSocket(0) &&
							inv_item->GetSocket(1) == item->GetSocket(1) &&
#if !defined(__EXTENDED_BLEND_AFFECT__)
							inv_item->GetSocket(2) == item->GetSocket(2) &&
#endif
							inv_item->GetCount() < g_bItemCountLimit)
					{
#if defined(__EXTENDED_BLEND_AFFECT__)
						inv_item->SetSocket(2, inv_item->GetSocket(2) + item->GetSocket(2));
#else
						inv_item->SetCount(inv_item->GetCount() + item->GetCount());
#endif
						return inv_item;
					}
				}
			}
		}
	}

	int iEmptyCell;

	if (type == DRAGON_SOUL_INVENTORY)
		iEmptyCell = GetEmptyDragonSoulInventory(item);
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	else if (type == SKILL_BOOK_INVENTORY)
		iEmptyCell = GetEmptySkillBookInventory(item->GetSize());
	else if (type == UPGRADE_ITEMS_INVENTORY)
		iEmptyCell = GetEmptyUpgradeItemsInventory(item->GetSize());
	else if (type == STONE_INVENTORY)
		iEmptyCell = GetEmptyStoneInventory(item->GetSize());
	else if (type == GIFT_BOX_INVENTORY)
		iEmptyCell = GetEmptyGiftBoxInventory(item->GetSize());
	else if (type == SWITCH_INVENTORY)
		iEmptyCell = GetEmptySwitchInventory(item->GetSize());
	else if (type == COSTUME_INVENTORY)
		iEmptyCell = GetEmptyCostumeInventory(item->GetSize());
#endif
	else if (type == INVENTORY)
		iEmptyCell = GetEmptyInventory(item->GetSize());

	if (iEmptyCell != -1)
	{
		if (bMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템 획득: %s"), item->GetLocaleName(GetLanguage()));

		if (type == DRAGON_SOUL_INVENTORY)
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		else if (type == SKILL_BOOK_INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell), iEmptyCell);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
		else if (type == UPGRADE_ITEMS_INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell), iEmptyCell);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
		else if (type == STONE_INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell), iEmptyCell);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
		else if (type == GIFT_BOX_INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell), iEmptyCell);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
		else if (type == SWITCH_INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell), iEmptyCell);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
		else if (type == COSTUME_INVENTORY)
#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell), iEmptyCell);
#else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
#endif
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = (DWORD)iEmptyCell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround2(GetMapIndex(), GetXYZ(), false, this, 300);
#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
#else
		item->StartDestroyEvent();
#endif
	}

	sys_log(0,
		"7: %d %d", dwItemVnum, bCount);
	return item;
}

#ifdef RENEWAL_PICKUP_AFFECT
bool CHARACTER::CanPickupDirectly()
{
	return FindAffect(AFFECT_PICKUP_ENABLE) != NULL;
}
#endif

bool CHARACTER::GiveItem(LPCHARACTER victim, TItemPos Cell)
{
	if (!CanHandleItem())
		return false;

	// @fixme150 BEGIN
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot take this item if you're using quests"));
		return false;
	}
	// @fixme150 END

	LPITEM item = GetItem(Cell);

	if (item && !item->IsExchanging())
	{
		if (victim->CanReceiveItem(this, item))
		{
			victim->ReceiveItem(this, item);
			return true;
		}
	}

	return false;
}

bool CHARACTER::CanReceiveItem(LPCHARACTER from, LPITEM item) const
{
	if (IsPC())
		return false;

	// TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX
	if (DISTANCE_APPROX(GetX() - from->GetX(), GetY() - from->GetY()) > 2000)
		return false;
	// END_OF_TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX

	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH &&
					(item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				return true;
			break;

		case fishing::FISHER_MOB:
			if (item->GetType() == ITEM_ROD)
				return true;
			break;

			// BUILDING_NPC
		case BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
			if (item->GetType() == ITEM_WEAPON &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
			if (item->GetType() == ITEM_ARMOR &&
					(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ACCESSORY_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
			if (item->GetType() == ITEM_ARMOR &&
					!(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;
			// END_OF_BUILDING_NPC

		case BLACKSMITH_MOB:
			if (item->GetRefinedVnum() && item->GetRefineSet() < 500 && !item->IsCrystal())
			{
				return true;
			}
			else
			{
				return false;
			}

		case BLACKSMITH2_MOB:
			if (item->GetRefineSet() >= 500)
			{
				return true;
			}
			else
			{
				return false;
			}

		case ALCHEMIST_MOB:
			if (item->GetRefinedVnum())
				return true;
			break;

		case 20101:
		case 20102:
		case 20103:

			if (item->GetVnum() == ITEM_REVIVE_HORSE_1)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("죽지 않은 말에게 선초를 먹일 수 없습니다."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("죽은 말에게 사료를 먹일 수 없습니다."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20104:
		case 20105:
		case 20106:

			if (item->GetVnum() == ITEM_REVIVE_HORSE_2)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("죽지 않은 말에게 선초를 먹일 수 없습니다."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("죽은 말에게 사료를 먹일 수 없습니다."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20107:
		case 20108:
		case 20109:

			if (item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("죽지 않은 말에게 선초를 먹일 수 없습니다."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("죽은 말에게 사료를 먹일 수 없습니다."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				return false;
			}
			break;
	}

	//if (IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_GIVE))
	{
		return true;
	}

	return false;
}

#ifdef ENABLE_CHEST_OPEN_RENEWAL
bool CHARACTER::OpenChestItem(TItemPos pos, BYTE bOpenCount)
{
	LPITEM item;
	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(pos) || !(item = GetItem(pos)))
		return false;
	
	if (item->GetType() != ITEM_GIFTBOX)
		return false;

	if (item->IsExchanging())
		return false;
	
	if (IsStun())
		return false;

	if (bOpenCount > 250)
		bOpenCount = 250;
	
	return OpenChestItem(item, bOpenCount);
}

bool CHARACTER::OpenChestItem(LPITEM item, BYTE bOpenCount)
{
	if (!item)
		return false;
	
	if (!CanHandleItem())
		return false;
	
	if (item->IsExchanging())
		return false;
	
	if (IsStun())
		return false;
	
	DWORD dwGroupNum = item->GetVnum();
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(dwGroupNum);
	if (!pGroup)
	{
		sys_err("cannot find special item group %d", dwGroupNum);
		return false;
	}
	
	if (item->isLocked())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 이 아이템은 넣을 수 없습니다."));
		return false;
	}
	
	BYTE loopCount = MINMAX(1, MIN(bOpenCount, item->GetCount()), 250);
	if (loopCount > item->GetCount())
	{
		sys_err("loopCount > item->GetCount() %d", dwGroupNum);
		return false;
	}
	
	item->Lock(true);
	if (test_server)
		ChatPacket(CHAT_TYPE_INFO, "Opening %d times of %d", loopCount, dwGroupNum);
	
	// umap of all itemVnums|Counts given at the end
	std::unordered_map<DWORD, long long> wGivenItems;
	for (auto oCount = 0; oCount < loopCount; oCount++)
	{
		std::vector<int> itemIndexes;
		int itemIndexesCount = pGroup->GetMultiIndex(itemIndexes);
		
		for (auto index = 0; index < itemIndexesCount; index++)
		{
			DWORD dwVnum = pGroup->GetVnum(itemIndexes[index]);
			DWORD dwCount = pGroup->GetCount(itemIndexes[index]);
			
			long long llItemCount = wGivenItems.count(dwVnum) > 0 ? wGivenItems[dwVnum] + dwCount : dwCount;
			wGivenItems[dwVnum] = llItemCount;
		}
	}
	
	if (wGivenItems.size() < 1)
	{
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("아무것도 얻을 수 없었습니다."));
		if (item)
			item->Lock(false);
		return false;
	}

	item->Lock(false);
	item->SetCount(MAX(0, item->GetCount() - loopCount));
	
	for (auto& info : wGivenItems)
	{
		DWORD dwItemVnum = info.first;
		long long llItemCount = info.second;
		
		LPITEM pItem;
		switch (dwItemVnum)
		{
			case CSpecialItemGroup::GOLD:
			{
				PointChange(POINT_GOLD, llItemCount);
				ChatPacket(CHAT_TYPE_MONEY_INFO, LC_TEXT("돈 %d 냥을 획득했습니다."), (int)llItemCount);
			}
			break;
			
			case CSpecialItemGroup::EXP:
			{
				PointChange(POINT_EXP, llItemCount);
				ChatPacket(CHAT_TYPE_EXP_INFO, LC_TEXT("상자에서 부터 신비한 빛이 나옵니다."));
				ChatPacket(CHAT_TYPE_EXP_INFO, LC_TEXT("%d의 경험치를 획득했습니다."), (int)llItemCount);
			}
			break;
			
			case CSpecialItemGroup::MOB:
			{
				int x = GetX() + number(-500, 500);
				int y = GetY() + number(-500, 500);
				
				LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(llItemCount, GetMapIndex(), x, y, 0, true, -1);
				if (ch)
					ch->SetAggressive();
				
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 몬스터가 나타났습니다!"));
			}
			break;
			
			case CSpecialItemGroup::SLOW:
			{
				sys_log(0, "CSpecialItemGroup::SLOW %d", -(int)llItemCount);
				AddAffect(AFFECT_SLOW, POINT_MOV_SPEED, -(int)llItemCount, AFF_SLOW, 300, 0, true);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 나온 빨간 연기를 들이마시자 움직이는 속도가 느려졌습니다!"));
			}
			break;
			
			case CSpecialItemGroup::DRAIN_HP:
			{
				int iDropHP = GetMaxHP()*llItemCount/100;
				iDropHP = MIN(iDropHP, GetHP()-1);
				PointChange(POINT_HP, -iDropHP);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자가 갑자기 폭발하였습니다! 생명력이 감소했습니다."));
			}
			break;
			
			case CSpecialItemGroup::POISON:
			{
				AttackedByPoison(NULL);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 나온 녹색 연기를 들이마시자 독이 온몸으로 퍼집니다!"));
			}
			break;
			
#ifdef ENABLE_WOLFMAN_CHARACTER
			case CSpecialItemGroup::BLEEDING:
			{
				AttackedByBleeding(NULL);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 나온 녹색 연기를 들이마시자 독이 온몸으로 퍼집니다!"));
			}
			break;
#endif
			case CSpecialItemGroup::MOB_GROUP:
			{
				int sx = GetX() - number(300, 500);
				int sy = GetY() - number(300, 500);
				int ex = GetX() + number(300, 500);
				int ey = GetY() + number(300, 500);
				CHARACTER_MANAGER::instance().SpawnGroup(llItemCount, GetMapIndex(), sx, sy, ex, ey, NULL, true);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 몬스터가 나타났습니다!"));
			}
			break;
			
			default:
			{
				if (llItemCount > g_bItemCountLimit)
				{
					for(auto itemIndex = llItemCount; itemIndex > 0; itemIndex -= g_bItemCountLimit)
					{
						WORD wCount = MIN(itemIndex, g_bItemCountLimit);
						pItem = AutoGiveItem(dwItemVnum, wCount, -1, false);
					}
				}
				else if ((pItem = AutoGiveItem(dwItemVnum, llItemCount, -1, false)))
				{
					if (!pItem->IsStackable() && llItemCount > 1)
					{
						for (auto itemIndex = 0; itemIndex < (llItemCount - 1); itemIndex++)
							pItem = AutoGiveItem(dwItemVnum, 1, -1, false);
					}
				}
				
				if (pItem)
				{
					if (llItemCount > 1)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 %s 가 %d 개 나왔습니다."), pItem->GetLocaleName(GetLanguage()), (int)llItemCount);
					else
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상자에서 %s 가 나왔습니다."), pItem->GetLocaleName(GetLanguage()));
				}
			}
			break;
		}

		SetStatistics(STAT_TYPE_CHESTS, loopCount);
	}
	
	return true;
}
#endif
void CHARACTER::ReceiveItem(LPCHARACTER from, LPITEM item)
{
	if (IsPC())
		return;

	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH && (item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				fishing::Grill(from, item);
			else
			{
				// TAKE_ITEM_BUG_FIX
				from->SetQuestNPCID(GetVID());
				// END_OF_TAKE_ITEM_BUG_FIX
				quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			}
			break;

			// DEVILTOWER_NPC
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
			if (item->GetRefinedVnum() != 0 && item->GetRefineSet() != 0 && item->GetRefineSet() < 500)
			{
				from->SetRefineNPC(this);
				from->RefineInformation(item->GetCell(), REFINE_TYPE_MONEY_ONLY);
			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 아이템은 개량할 수 없습니다."));
			}
			break;
			// END_OF_DEVILTOWER_NPC

		case BLACKSMITH_MOB:
		case BLACKSMITH2_MOB:
		case BLACKSMITH_WEAPON_MOB:
		case BLACKSMITH_ARMOR_MOB:
		case BLACKSMITH_ACCESSORY_MOB:
			if (item->GetRefinedVnum() && !item->IsCrystal())
			{
				from->SetRefineNPC(this);
				from->RefineInformation(item->GetCell(), REFINE_TYPE_NORMAL);
			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 아이템은 개량할 수 없습니다."));
			}
			break;

		case 20101:
		case 20102:
		case 20103:
		case 20104:
		case 20105:
		case 20106:
		case 20107:
		case 20108:
		case 20109:
			if (item->GetVnum() == ITEM_REVIVE_HORSE_1 ||
					item->GetVnum() == ITEM_REVIVE_HORSE_2 ||
					item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				from->ReviveHorse();
				item->SetCount(item->GetCount()-1);
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말에게 선초를 주었습니다."));
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 ||
					item->GetVnum() == ITEM_HORSE_FOOD_2 ||
					item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				from->FeedHorse();
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말에게 사료를 주었습니다."));
				item->SetCount(item->GetCount()-1);
				EffectPacket(SE_HPUP_RED);
			}
			break;

		default:
			sys_log(0, "TakeItem %s %d %s", from->GetName(), GetRaceNum(), item->GetName());
			from->SetQuestNPCID(GetVID());
			quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			break;
	}
}

bool CHARACTER::IsEquipUniqueItem(DWORD dwItemVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	if (dwItemVnum == UNIQUE_ITEM_RING_OF_LANGUAGE)
		return IsEquipUniqueItem(UNIQUE_ITEM_RING_OF_LANGUAGE_SAMPLE);

	return false;
}

// CHECK_UNIQUE_GROUP
bool CHARACTER::IsEquipUniqueGroup(DWORD dwGroupVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetSpecialGroup() == (int) dwGroupVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetSpecialGroup() == (int) dwGroupVnum)
			return true;
	}

	return false;
}
// END_OF_CHECK_UNIQUE_GROUP

void CHARACTER::SetRefineMode(int iAdditionalCell)
{
	m_iRefineAdditionalCell = iAdditionalCell;
	SetUnderRefine(true);
}

void CHARACTER::ClearRefineMode()
{
	SetUnderRefine(false);
	SetRefineNPC( NULL );
}

bool CHARACTER::GiveItemFromSpecialItemGroup(DWORD dwGroupNum, std::vector<DWORD> &dwItemVnums,
											std::vector<DWORD> &dwItemCounts, std::vector <LPITEM> &item_gets, int &count)
{
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(dwGroupNum);

	if (!pGroup)
	{
		sys_err("cannot find special item group %d", dwGroupNum);
		return false;
	}

	std::vector <int> idxes;
	int n = pGroup->GetMultiIndex(idxes);

	bool bSuccess;

	for (int i = 0; i < n; i++)
	{
		bSuccess = false;
		int idx = idxes[i];
		DWORD dwVnum = pGroup->GetVnum(idx);
		DWORD dwCount = pGroup->GetCount(idx);
		int	iRarePct = pGroup->GetRarePct(idx);
		LPITEM item_get = NULL;
		switch (dwVnum)
		{
			case CSpecialItemGroup::GOLD:
				PointChange(POINT_GOLD, (int)dwCount);

				bSuccess = true;
				break;
			case CSpecialItemGroup::EXP:
				{
					PointChange(POINT_EXP, dwCount);

					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::MOB:
				{
					sys_log(0, "CSpecialItemGroup::MOB %d", dwCount);
					int x = GetX() + number(-500, 500);
					int y = GetY() + number(-500, 500);

					LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(dwCount, GetMapIndex(), x, y, 0, true, -1);
					if (ch)
						ch->SetAggressive();
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::SLOW:
				{
					sys_log(0, "CSpecialItemGroup::SLOW %d", -(int)dwCount);
					AddAffect(AFFECT_SLOW, POINT_MOV_SPEED, -(int)dwCount, AFF_SLOW, 300, 0, true);
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::DRAIN_HP:
				{
					int iDropHP = GetMaxHP()*dwCount/100;
					sys_log(0, "CSpecialItemGroup::DRAIN_HP %d", -iDropHP);
					iDropHP = MIN(iDropHP, GetHP()-1);
					sys_log(0, "CSpecialItemGroup::DRAIN_HP %d", -iDropHP);
					PointChange(POINT_HP, -iDropHP);
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::POISON:
				{
					AttackedByPoison(NULL);
					bSuccess = true;
				}
				break;
#ifdef ENABLE_WOLFMAN_CHARACTER
			case CSpecialItemGroup::BLEEDING:
				{
					AttackedByBleeding(NULL);
					bSuccess = true;
				}
				break;
#endif
			case CSpecialItemGroup::MOB_GROUP:
				{
					int sx = GetX() - number(300, 500);
					int sy = GetY() - number(300, 500);
					int ex = GetX() + number(300, 500);
					int ey = GetY() + number(300, 500);
					CHARACTER_MANAGER::instance().SpawnGroup(dwCount, GetMapIndex(), sx, sy, ex, ey, NULL, true);

					bSuccess = true;
				}
				break;
			default:
				{
					item_get = AutoGiveItem(dwVnum, dwCount, iRarePct);

					if (item_get)
					{
						bSuccess = true;
					}
				}
				break;
		}

		if (bSuccess)
		{
			dwItemVnums.emplace_back(dwVnum);
			dwItemCounts.emplace_back(dwCount);
			item_gets.emplace_back(item_get);
			count++;

		}
		else
		{
			return false;
		}
	}
	return bSuccess;
}

// NEW_HAIR_STYLE_ADD
bool CHARACTER::ItemProcess_Hair(LPITEM item, int iDestCell)
{
	if (item->CheckItemUseLevel(GetLevel()) == false)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아직 이 머리를 사용할 수 없는 레벨입니다."));
		return false;
	}

	DWORD hair = item->GetVnum();

	switch (GetJob())
	{
		case JOB_WARRIOR :
			hair -= 72000;
			break;

		case JOB_ASSASSIN :
			hair -= 71250;
			break;

		case JOB_SURA :
			hair -= 70500;
			break;

		case JOB_SHAMAN :
			hair -= 69750;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			break;
#endif
		default :
			return false;
			break;
	}

	if (hair == GetPart(PART_HAIR))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("동일한 머리 스타일로는 교체할 수 없습니다."));
		return true;
	}

	item->SetCount(item->GetCount() - 1);

	SetPart(PART_HAIR, hair);
	UpdatePacket();

	return true;
}
// END_NEW_HAIR_STYLE_ADD

bool CHARACTER::ItemProcess_Polymorph(LPITEM item)
{
	if (IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 둔갑중인 상태입니다."));
		return false;
	}

	if (true == IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑할 수 없는 상태입니다."));
		return false;
	}

	DWORD dwVnum = item->GetSocket(0);

	if (dwVnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("잘못된 둔갑 아이템입니다."));
		if (!CHARACTER_MANAGER::Instance().CheckTestDungeon(this))
			item->SetCount(item->GetCount()-1);
		return false;
	}

	const CMob* pMob = CMobManager::instance().Get(dwVnum);

	if (pMob == NULL)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("잘못된 둔갑 아이템입니다."));
		if (!CHARACTER_MANAGER::Instance().CheckTestDungeon(this))
			item->SetCount(item->GetCount()-1);
		return false;
	}

	switch (item->GetVnum())
	{
		case 70104 :
		case 70105 :
		case 70106 :
		case 70107 :
		case 71093 :
			{
				sys_log(0, "USE_POLYMORPH_BALL PID(%d) vnum(%d)", GetPlayerID(), dwVnum);

				int iPolymorphLevelLimit = MAX(0, 20 - GetLevel() * 3 / 10);
				if (pMob->m_table.bLevel >= GetLevel() + iPolymorphLevelLimit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("나보다 너무 높은 레벨의 몬스터로는 변신 할 수 없습니다."));
					return false;
				}

				int iDuration = GetSkillLevel(POLYMORPH_SKILL_ID) == 0 ? 5 : (5 + (5 + GetSkillLevel(POLYMORPH_SKILL_ID)/40 * 25));
				iDuration *= 60;

				DWORD dwBonus = 0;

				dwBonus = (2 + GetSkillLevel(POLYMORPH_SKILL_ID)/40) * 100;

				AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, dwVnum, AFF_POLYMORPH, iDuration, 0, true);
				AddAffect(AFFECT_POLYMORPH, POINT_ATT_BONUS, dwBonus, AFF_POLYMORPH, iDuration, 0, false);

				if (!CHARACTER_MANAGER::Instance().CheckTestDungeon(this))
					item->SetCount(item->GetCount()-1);
			}
			break;

		case 50322:
			{
				sys_log(0, "USE_POLYMORPH_BOOK: %s(%u) vnum(%u)", GetName(), GetPlayerID(), dwVnum);

				if (CPolymorphUtils::instance().PolymorphCharacter(this, item, pMob) == true)
				{
					CPolymorphUtils::instance().UpdateBookPracticeGrade(this, item);
				}
				else
				{
				}
			}
			break;

		default :
			sys_err("POLYMORPH invalid item passed PID(%d) vnum(%d)", GetPlayerID(), item->GetOriginalVnum());
			return false;
	}

	return true;
}

bool CHARACTER::CanDoCube() const
{
	if (m_bIsObserver)	return false;
	if (GetShop())		return false;
	if (GetMyShop())	return false;
	if (m_bUnderRefine)	return false;
	if (IsWarping())	return false;

	return true;
}

bool CHARACTER::UnEquipSpecialRideUniqueItem()
{
	LPITEM Unique1 = GetWear(WEAR_UNIQUE1);
	LPITEM Unique2 = GetWear(WEAR_UNIQUE2);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	LPITEM MountCostume = GetWear(WEAR_COSTUME_MOUNT);
#endif

	if( NULL != Unique1 )
	{
		if( UNIQUE_GROUP_SPECIAL_RIDE == Unique1->GetSpecialGroup() )
		{
			return UnequipItem(Unique1);
		}
	}

	if( NULL != Unique2 )
	{
		if( UNIQUE_GROUP_SPECIAL_RIDE == Unique2->GetSpecialGroup() )
		{
			return UnequipItem(Unique2);
		}
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (MountCostume)
		return UnequipItem(MountCostume);
#endif

	return true;
}

void CHARACTER::AutoRecoveryItemProcess(const EAffectTypes type)
{
	if (true == IsDead() || true == IsStun())
		return;

	if (false == IsPC())
		return;

	if (AFFECT_AUTO_HP_RECOVERY != type && AFFECT_AUTO_SP_RECOVERY != type)
		return;

	if (NULL != FindAffect(AFFECT_STUN))
		return;

	{
		const DWORD stunSkills[] = { SKILL_TANHWAN, SKILL_GEOMPUNG, SKILL_BYEURAK, SKILL_GIGUNG };

		for (size_t i=0 ; i < sizeof(stunSkills)/sizeof(DWORD) ; ++i)
		{
			const CAffect* p = FindAffect(stunSkills[i]);

			if (NULL != p && AFF_STUN == p->dwFlag)
				return;
		}
	}

	const CAffect* pAffect = FindAffect(type);
	const size_t idx_of_amount_of_used = 1;
	const size_t idx_of_amount_of_full = 2;

	if (NULL != pAffect)
	{
		LPITEM pItem = FindItemByID(pAffect->dwFlag);

		if (NULL != pItem && true == pItem->GetSocket(0))
		{
			if (!CArenaManager::instance().IsArenaMap(GetMapIndex()))
			{
				const long amount_of_full = pItem->GetSocket(idx_of_amount_of_full);

				int32_t amount = 0;

				if (AFFECT_AUTO_HP_RECOVERY == type)
				{
					amount = GetMaxHP() - (GetHP() + GetPoint(POINT_HP_RECOVERY));
				}
				else if (AFFECT_AUTO_SP_RECOVERY == type)
				{
					amount = GetMaxSP() - (GetSP() + GetPoint(POINT_SP_RECOVERY));
				}

				if (amount > 0)
				{
					if (AFFECT_AUTO_HP_RECOVERY == type)
					{
						PointChange( POINT_HP_RECOVERY, amount );
						EffectPacket( SE_AUTO_HPUP );
					}
					else if (AFFECT_AUTO_SP_RECOVERY == type)
					{
						PointChange( POINT_SP_RECOVERY, amount );
						EffectPacket( SE_AUTO_SPUP );
					}
				}
			}
			else
			{
				pItem->Lock(false);
				pItem->SetSocket(0, false);
				RemoveAffect( const_cast<CAffect*>(pAffect) );
			}
		}
		else
		{
			RemoveAffect( const_cast<CAffect*>(pAffect) );
		}
	}
}

bool CHARACTER::IsValidItemPosition(TItemPos Pos) const
{
	BYTE window_type = Pos.window_type;
	WORD cell = Pos.cell;

	switch (window_type)
	{
	case RESERVED_WINDOW:
		return false;

	case INVENTORY:
	case EQUIPMENT:
		return cell < (INVENTORY_AND_EQUIP_SLOT_MAX);	
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	case SKILL_BOOK_INVENTORY:
		return (cell < SKILL_BOOK_INVENTORY_SLOT_END && cell > SKILL_BOOK_INVENTORY_SLOT_START);
	case UPGRADE_ITEMS_INVENTORY:
		return (cell < UPGRADE_ITEMS_INVENTORY_SLOT_END && cell > UPGRADE_ITEMS_INVENTORY_SLOT_START);
	case STONE_INVENTORY:
		return (cell < STONE_INVENTORY_SLOT_END && cell > STONE_INVENTORY_SLOT_START);
	case GIFT_BOX_INVENTORY:
		return (cell < GIFT_BOX_INVENTORY_SLOT_END && cell > GIFT_BOX_INVENTORY_SLOT_START);
	case SWITCH_INVENTORY:
		return (cell < SWITCH_INVENTORY_SLOT_END && cell > SWITCH_INVENTORY_SLOT_START);
	case COSTUME_INVENTORY:
		return (cell < COSTUME_INVENTORY_SLOT_END && cell > COSTUME_INVENTORY_SLOT_START);
#endif

	case DRAGON_SOUL_INVENTORY:
		return cell < (DRAGON_SOUL_INVENTORY_MAX_NUM);

	case SAFEBOX:
		if (NULL != m_pkSafebox)
			return m_pkSafebox->IsValidPosition(cell);
		else
			return false;

	case MALL:
		if (NULL != m_pkMall)
			return m_pkMall->IsValidPosition(cell);
		else
			return false;
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		return cell < SWITCHBOT_SLOT_COUNT;
#endif
	default:
		return false;
	}
}

#define VERIFY_MSG(exp, msg)  \
	if (true == (exp)) { \
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(msg)); \
			return false; \
	}

bool CHARACTER::CanEquipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{
	const TItemTable* itemTable = item->GetProto();

#ifdef __BUFFI_SUPPORT__
	if ((IsLookingBuffiPage() && item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_WEAPON))
	{
		if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
			return false;
	}
	else
#endif
	{
		switch (GetJob())
		{
		case JOB_WARRIOR:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SURA)
				return false;
			break;

#ifdef ENABLE_WOLFMAN
		case JOB_WOLFMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN)
			{
				return false;
			}
			break;
#endif
		}
	}


	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limit = itemTable->aLimits[i].lValue;
		switch (itemTable->aLimits[i].bType)
		{
			case LIMIT_LEVEL:
				if (GetLevel() < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("레벨이 낮아 착용할 수 없습니다."));
					return false;
				}
				break;

			case LIMIT_STR:
				if (GetPoint(POINT_ST) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("근력이 낮아 착용할 수 없습니다."));
					return false;
				}
				break;

			case LIMIT_INT:
				if (GetPoint(POINT_IQ) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("지능이 낮아 착용할 수 없습니다."));
					return false;
				}
				break;

			case LIMIT_DEX:
				if (GetPoint(POINT_DX) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("민첩이 낮아 착용할 수 없습니다."));
					return false;
				}
				break;

			case LIMIT_CON:
				if (GetPoint(POINT_HT) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("체력이 낮아 착용할 수 없습니다."));
					return false;
				}
				break;
		}
	}

#ifdef __SKIN_SYSTEM__
	if (item->IsAcceSkin())
	{
		if (!GetWear(WEAR_COSTUME_ACCE))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't wear skin without acce."));
			return false;
		}
	}
#endif

	if (item->GetVnum() == 72002 || item->GetVnum() == 72001 || item->GetVnum() == 72003)
	{
		auto unique1 = GetWear(WEAR_UNIQUE1);
		auto unique2 = GetWear(WEAR_UNIQUE2);

		if (unique1 && (unique1->GetVnum() == 72002 || unique1->GetVnum() == 72001 || unique1->GetVnum() == 72003))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("같은 종류의 유니크 아이템 두 개를 동시에 장착할 수 없습니다."));
			return false;
		}
		if (unique2 && (unique2->GetVnum() == 72002 || unique2->GetVnum() == 72001 || unique2->GetVnum() == 72003))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("같은 종류의 유니크 아이템 두 개를 동시에 장착할 수 없습니다."));
			return false;
		}
	}

	if (item->GetVnum() == 70043 || item->GetVnum() == 72004 || item->GetVnum() == 72006)
	{
		auto unique1 = GetWear(WEAR_UNIQUE1);
		auto unique2 = GetWear(WEAR_UNIQUE2);

		if (unique1 && (unique1->GetVnum() == 70043 || unique1->GetVnum() == 72004 || unique1->GetVnum() == 72006))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("같은 종류의 유니크 아이템 두 개를 동시에 장착할 수 없습니다."));
			return false;
		}
		if (unique2 && (unique2->GetVnum() == 70043 || unique2->GetVnum() == 72004 || unique2->GetVnum() == 72006))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("같은 종류의 유니크 아이템 두 개를 동시에 장착할 수 없습니다."));
			return false;
		}
	}

#ifdef __RENEWAL_MOUNT__
	// if (item->IsCostumeMountItem())
	// {
	// 	if (IsRiding())
	// 	{
	// 		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't equip a mount skin while riding."));
	// 		return false;
	// 	}
	// }
#ifdef __SKIN_SYSTEM__
	if (item->IsSkinMount())
	{
		if (!GetWear(WEAR_COSTUME_MOUNT))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need a mount before equipping a mount costume."));
			return false;
		}
		// if (IsRiding())
		// {
		// 	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't equip a skin while riding."));
		// 	return false;
		// }
	}
	if (item->IsSkinPet())
	{
		if (!GetWear(WEAR_NORMAL_PET))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't wear a skin without pet!"));
			return false;
		}
	}
#endif
#endif

	if (item->GetWearFlag() & WEARABLE_UNIQUE)
	{
		if ((GetWear(WEAR_UNIQUE1) && GetWear(WEAR_UNIQUE1)->IsSameSpecialGroup(item)) ||
			(GetWear(WEAR_UNIQUE2) && GetWear(WEAR_UNIQUE2)->IsSameSpecialGroup(item)))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("같은 종류의 유니크 아이템 두 개를 동시에 장착할 수 없습니다."));
			return false;
		}

		if (marriage::CManager::instance().IsMarriageUniqueItem(item->GetVnum()) &&
			!marriage::CManager::instance().IsMarried(GetPlayerID()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("결혼하지 않은 상태에서 예물을 착용할 수 없습니다."));
			return false;
		}

	}

#ifdef ENABLE_DS_SET
	if ((DragonSoul_IsDeckActivated()) && (item->IsDragonSoul())) {
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("DS_DEACTIVATE_REQ"));
		return false;
	}
#endif

	return true;
}

bool IS_SUMMON_ITEM(int vnum) //< 2020.08.08.Owsap - Check function externally.
{
	switch (vnum)
	{
	case 22000:
	case 22010:
	case 22011:
	case 22020:
	case ITEM_MARRIAGE_RING:
		return true;
	}

	return false;
}

bool CHARACTER::CanUnequipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{
	if (ITEM_BELT == item->GetType())
		VERIFY_MSG(CBeltInventoryHelper::IsExistItemInBeltInventory(this), "벨트 인벤토리에 아이템이 존재하면 해제할 수 없습니다.");

	if (IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

#ifdef __SKIN_SYSTEM__
	if (item->IsSkinMount() && IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't unequip a mount while riding."));
		return false;
	}
	if (item->IsCostumeMountItem() && GetWear(WEAR_COSTUME_MOUNT_SKIN))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to unequip first skin."));
		return false;
	}
	if (item->IsAcce() && GetWear(WEAR_COSTUME_ACCE_SKIN))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to unequip acce skin first."));
		return false;
	}
	if (item->IsCostumePetItem() && GetWear(WEAR_COSTUME_NORMAL_PET_SKIN))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to unequip first skin."));
		return false;
	}
#endif

	if ((item->GetType() == ITEM_ARMOR && destCell.IsCostumeInventoryPosition()) || 
		(item->GetType() == ITEM_WEAPON && destCell.IsCostumeInventoryPosition()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't add armours and weapons to the special inventory."));
		return false;
	}

#ifdef __RENEWAL_MOUNT__
	if (item->IsCostumeMountItem())
	{
		if(IsRiding())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't unequip a mount while riding."));
			return false;
		}
	}
#endif

#ifdef ENABLE_DS_SET
	if ((DragonSoul_IsDeckActivated()) && (item->IsDragonSoul())) {
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("DS_DEACTIVATE_REQ"));
		return false;
	}
#endif

	if (item->GetType() == ITEM_WEAPON)
	{
		if (IsAffectFlag(AFF_GWIGUM))
			RemoveAffect(SKILL_GWIGEOM);

		if (IsAffectFlag(AFF_GEOMGYEONG))
			RemoveAffect(SKILL_GEOMKYUNG);
	}

	{
		int pos = -1;

		if (item->IsDragonSoul())
			pos = GetEmptyDragonSoulInventory(item);
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		else if (item->IsSkillBook())
			pos = GetEmptySkillBookInventory(item->GetSize());
		else if (item->IsUpgradeItem())
			pos = GetEmptyUpgradeItemsInventory(item->GetSize());
		else if (item->IsStone())
			pos = GetEmptyStoneInventory(item->GetSize());
		else if (item->IsGiftBox())
			pos = GetEmptyGiftBoxInventory(item->GetSize());
		else if (item->IsSwitch())
			pos = GetEmptySwitchInventory(item->GetSize());
		else if (item->IsCostume())
			pos = GetEmptyCostumeInventory(item->GetSize());
#endif
		else
			pos = GetEmptyInventory(item->GetSize());

		VERIFY_MSG( -1 == pos, "소지품에 빈 공간이 없습니다." );
	}

	return true;
}

#if defined(__BL_MOVE_COSTUME_ATTR__)
void CHARACTER::OpenItemComb()
{
	if (PreventTradeWindow(WND_ITEM_COMB, true))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to close other windows."));
		return;
	}
	
	const LPCHARACTER npc = GetQuestNPC();
	if (npc == NULL)
	{
		sys_err("Item Combination NPC is NULL (ch: %s)", GetName());
		return;
	}

	SetItemCombNpc(npc);
	ChatPacket(CHAT_TYPE_COMMAND, "ShowItemCombinationDialog");
}

void CHARACTER::ItemCombination(const short MediumIndex, const short BaseIndex, const short MaterialIndex)
{
	if (IsItemComb() == false)
		return;
	
	const LPITEM MediumItem		= GetItem(TItemPos(INVENTORY, MediumIndex));
	const LPITEM BaseItem		= GetItem(TItemPos(INVENTORY, BaseIndex));
	const LPITEM MaterialItem	= GetItem(TItemPos(INVENTORY, MaterialIndex));

	if (MediumItem == NULL || BaseItem == NULL || MaterialItem == NULL)
		return;

	if (BaseItem->IsEquipped() || MaterialItem->IsEquipped())
		return;

	if (BaseItem->GetSubType() == ECostumeSubTypes::COSTUME_ACCE_SKIN || MaterialItem->GetSubType() == ECostumeSubTypes::COSTUME_ACCE_SKIN)
		return;

	if ((BaseItem->GetType() == EItemTypes::ITEM_COSTUME && BaseItem->GetSubType() == ECostumeSubTypes::COSTUME_ACCE) ||
		(MaterialItem->GetType() == EItemTypes::ITEM_COSTUME && MaterialItem->GetSubType() == ECostumeSubTypes::COSTUME_ACCE)
	)
	{
		if (MediumItem->GetVnum() != 70070)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have the required item for this type of combination."));
			return;
		}
		
		long sashGradeBase = BaseItem->GetValue(0);
		long sashAbsBase = BaseItem->GetSocket(0);
		long sashGradeMaterial = MaterialItem->GetValue(0);
		long sashAbsMaterial = MaterialItem->GetSocket(0);

		if (sashGradeBase != sashGradeMaterial)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The sashes must be the same grade in order to move the bonuses."));
			return;
		}
		
		if (sashAbsBase != sashAbsMaterial)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The sashes must have the same absortion in order to move the bonuses."));
			return;
		}
	}
	else if ((BaseItem->GetType() == EItemTypes::ITEM_COSTUME && BaseItem->GetSubType() != ECostumeSubTypes::COSTUME_ACCE) ||
		(MaterialItem->GetType() == EItemTypes::ITEM_COSTUME && MaterialItem->GetSubType() != ECostumeSubTypes::COSTUME_ACCE)
	)
	{
		if (MediumItem->GetVnum() != 70065)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have the required item for this type of combination."));
			return;
		}

		if (BaseItem->FindApplyTypeQuest(0) > 0 || MaterialItem->FindApplyTypeQuest(0) > 0)
			return;

		if (MaterialItem->GetAttributeCount() < 1)
			return;
	}

	if (BaseItem->GetSubType() != MaterialItem->GetSubType())
		return;

	BaseItem->SetAttributes(MaterialItem->GetAttributes());
	if (MaterialItem->GetSubType() == ECostumeSubTypes::COSTUME_ACCE)
		BaseItem->SetSockets(MaterialItem->GetSockets());
	BaseItem->UpdatePacket();

	ITEM_MANAGER::instance().RemoveItem(MaterialItem, "REMOVE (Item Combination)");
	MediumItem->SetCount(MediumItem->GetCount() - 1);
}
#endif
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
