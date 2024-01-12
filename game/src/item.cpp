#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "desc.h"
#include "sectree_manager.h"
#include "packet.h"
#include "protocol.h"
#include "log.h"
#include "skill.h"
#include "unique_item.h"
#include "profiler.h"
#include "marriage.h"
#include "item_addon.h"
#include "locale_service.h"
#include "item.h"
#include "item_manager.h"
#include "affect.h"
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"
#include "../../common/VnumHelper.h"
#include "../../common/CommonDefines.h"
#ifdef __BUFFI_SUPPORT__
#include "buffi.h"
#endif
#include "questmanager.h"

CItem::CItem(DWORD dwVnum)
	: m_dwVnum(dwVnum), m_bWindow(0), m_dwID(0), m_bEquipped(false), m_dwVID(0), m_wCell(0), m_dwCount(0), m_lFlag(0), m_dwLastOwnerPID(0),
	m_bExchanging(false), m_pkDestroyEvent(NULL), m_pkExpireEvent(NULL), m_pkUniqueExpireEvent(NULL),
	m_pkTimerBasedOnWearExpireEvent(NULL), m_pkRealTimeExpireEvent(NULL),
   	m_pkAccessorySocketExpireEvent(NULL), m_pkOwnershipEvent(NULL), m_dwOwnershipPID(0), m_bSkipSave(false), m_isLocked(false),
	m_dwMaskVnum(0), m_dwSIGVnum (0)
#if defined(__EXTENDED_BLEND_AFFECT__)
	, m_pkBlendUseEvent(NULL)
#endif
{
	memset( &m_alSockets, 0, sizeof(m_alSockets) );
	memset( &m_aAttr, 0, sizeof(m_aAttr) );
}

CItem::~CItem()
{
	Destroy();
}

void CItem::Initialize()
{
	CEntity::Initialize(ENTITY_ITEM);

	m_bWindow = RESERVED_WINDOW;
	m_pOwner = NULL;
	m_dwID = 0;
	m_bEquipped = false;
	m_dwVID = m_wCell = m_dwCount = m_lFlag = 0;
	m_pProto = NULL;
	m_bExchanging = false;
	memset(&m_alSockets, 0, sizeof(m_alSockets));
	memset(&m_aAttr, 0, sizeof(m_aAttr));
	m_bUpdateStatus = false;

	m_pkDestroyEvent = NULL;
	m_pkOwnershipEvent = NULL;
	m_dwOwnershipPID = 0;
	m_pkUniqueExpireEvent = NULL;
	m_pkTimerBasedOnWearExpireEvent = NULL;
	m_pkRealTimeExpireEvent = NULL;
#if defined(__EXTENDED_BLEND_AFFECT__)
	m_pkBlendUseEvent = NULL;
#endif
	m_pkAccessorySocketExpireEvent = NULL;

	m_bSkipSave = false;
	m_dwLastOwnerPID = 0;
}

void CItem::Destroy()
{
	event_cancel(&m_pkDestroyEvent);
	event_cancel(&m_pkOwnershipEvent);
	event_cancel(&m_pkUniqueExpireEvent);
	event_cancel(&m_pkTimerBasedOnWearExpireEvent);
	event_cancel(&m_pkRealTimeExpireEvent);
	event_cancel(&m_pkAccessorySocketExpireEvent);

#if defined(__EXTENDED_BLEND_AFFECT__)
	event_cancel(&m_pkBlendUseEvent);
#endif

	CEntity::Destroy();

	if (GetSectree())
		GetSectree()->RemoveEntity(this);
}

EVENTFUNC(item_destroy_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "item_destroy_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;

	if (pkItem->GetOwner())
		sys_err("item_destroy_event: Owner exist. (item %s owner %s)", pkItem->GetName(), pkItem->GetOwner()->GetName());

	pkItem->SetDestroyEvent(NULL);
	M2_DESTROY_ITEM(pkItem);
	return 0;
}

void CItem::SetDestroyEvent(LPEVENT pkEvent)
{
	m_pkDestroyEvent = pkEvent;
}

void CItem::StartDestroyEvent(int iSec)
{
	if (m_pkDestroyEvent)
		return;

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetDestroyEvent(event_create(item_destroy_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::EncodeInsertPacket(LPENTITY ent)
{
	LPDESC d;

	if (!(d = ent->GetDesc()))
		return;

	const PIXEL_POSITION & c_pos = GetXYZ();

	struct packet_item_ground_add pack;

	pack.bHeader	= HEADER_GC_ITEM_GROUND_ADD;
	pack.x		= c_pos.x;
	pack.y		= c_pos.y;
	pack.z		= c_pos.z;
	pack.dwVnum		= GetVnum();
	pack.dwVID		= m_dwVID;
	//pack.count	= m_dwCount;

#ifdef __ITEM_GROUND_ONLY_OWNER__
	LPCHARACTER ch = d->GetCharacter();
	if (!ch)
		return;
	else if (m_dwOwnershipPID != 0 && m_dwOwnershipPID != ch->GetPlayerID())
		return;
#endif

	d->Packet(pack);

	if (m_pkOwnershipEvent != NULL)
	{
		item_event_info * info = dynamic_cast<item_event_info *>(m_pkOwnershipEvent->info);

		if ( info == NULL )
		{
			sys_err( "CItem::EncodeInsertPacket> <Factor> Null pointer" );
			return;
		}

		TPacketGCItemOwnership p;

		p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
		p.dwVID = m_dwVID;
		strlcpy(p.szName, info->szOwnerName, sizeof(p.szName));

		d->Packet(p);
	}
}

void CItem::EncodeRemovePacket(LPENTITY ent)
{
	LPDESC d;

	if (!(d = ent->GetDesc()))
		return;

	struct packet_item_ground_del pack;

	pack.bHeader	= HEADER_GC_ITEM_GROUND_DEL;
	pack.dwVID		= m_dwVID;

	d->Packet(pack);
	sys_log(2, "Item::EncodeRemovePacket %s to %s", GetName(), ((LPCHARACTER) ent)->GetName());
}

void CItem::SetProto(const TItemTable * table)
{
	assert(table != NULL);
	m_pProto = table;
	SetFlag(m_pProto->dwFlags);
}

void CItem::UsePacketEncode(LPCHARACTER ch, LPCHARACTER victim, struct packet_item_use *packet)
{
	if (!GetVnum())
		return;

	packet->header 	= HEADER_GC_ITEM_USE;
	packet->ch_vid 	= ch->GetVID();
	packet->victim_vid 	= victim->GetVID();
	packet->Cell = TItemPos(GetWindow(), m_wCell);
	packet->vnum	= GetVnum();
}

void CItem::RemoveFlag(long bit)
{
	REMOVE_BIT(m_lFlag, bit);
}

void CItem::AddFlag(long bit)
{
	SET_BIT(m_lFlag, bit);
}

void CItem::UpdatePacket()
{
	if (!m_pOwner || !m_pOwner->GetDesc())
		return;
		
#ifdef ENABLE_SWITCHBOT
	if (GetUpdateStatus())
		return;
#endif

	TPacketGCItemUpdate pack;

	pack.header = HEADER_GC_ITEM_UPDATE;
	pack.Cell = TItemPos(GetWindow(), m_wCell);
	pack.count	= m_dwCount;

	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		pack.alSockets[i] = m_alSockets[i];

	thecore_memcpy(pack.aAttr, GetAttributes(), sizeof(pack.aAttr));

	sys_log(2, "UpdatePacket %s -> %s", GetName(), m_pOwner->GetName());
	m_pOwner->GetDesc()->Packet(pack);
}

DWORD CItem::GetCount()
{
	if (GetType() == ITEM_ELK) return MIN(m_dwCount, INT_MAX);
	else
	{
		return MIN(m_dwCount, g_bItemCountLimit);
	}
}

bool CItem::SetCount(DWORD count)
{
	if (GetType() == ITEM_ELK)
	{
		m_dwCount = MIN(count, INT_MAX);
	}
	else
	{
		m_dwCount = MIN(count, g_bItemCountLimit);
	}

	if (count == 0 && m_pOwner)
	{
		if (GetSubType() == USE_ABILITY_UP || GetSubType() == USE_POTION || GetVnum() == 70020)
		{
			LPCHARACTER pOwner = GetOwner();
			WORD wCell = GetCell();

			RemoveFromCharacter();

			if (!IsDragonSoul())
			{
				LPITEM pItem = pOwner->FindSpecifyItem(GetVnum());

				if (NULL != pItem)
				{
					pOwner->ChainQuickslotItem(pItem, QUICKSLOT_TYPE_ITEM, wCell);
				}
				else
				{
					pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, wCell, 255);
				}
			}

			M2_DESTROY_ITEM(this);
		}
		else
		{
			if (!IsDragonSoul())
			{
				m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, m_wCell, 255);
			}
			M2_DESTROY_ITEM(RemoveFromCharacter());
		}

		return false;
	}
	
	UpdatePacket();

	Save();
	return true;
}

LPITEM CItem::RemoveFromCharacter()
{
	if (!m_pOwner)
	{
		sys_err("Item::RemoveFromCharacter owner null");
		return (this);
	}

	LPCHARACTER pOwner = m_pOwner;

	if (m_bEquipped)
	{
		Unequip();

		SetWindow(RESERVED_WINDOW);
		Save();
		return (this);
	}
	else
	{
		if (GetWindow() != SAFEBOX && GetWindow() != MALL)
		{
			if (IsDragonSoul())
			{
				if (m_wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
					sys_err("CItem::RemoveFromCharacter: pos >= DRAGON_SOUL_INVENTORY_MAX_NUM");
				else
					pOwner->SetItem(TItemPos(m_bWindow, m_wCell), NULL);
			}
#ifdef ENABLE_SWITCHBOT
			else if (m_bWindow == SWITCHBOT)
			{
				if (m_wCell >= SWITCHBOT_SLOT_COUNT)
				{
					sys_err("CItem::RemoveFromCharacter: pos >= SWITCHBOT_SLOT_COUNT");
				}
				else
				{
					pOwner->SetItem(TItemPos(SWITCHBOT, m_wCell), NULL);
				}
			}
#endif
			else
			{
				TItemPos cell(INVENTORY, m_wCell);

				if (false == cell.IsDefaultInventoryPosition() && false == cell.IsBeltInventoryPosition()
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
					&& false == cell.IsSkillBookInventoryPosition() && false == cell.IsUpgradeItemsInventoryPosition() && false == cell.IsStoneInventoryPosition() && false == cell.IsGiftBoxInventoryPosition()
					 && false == cell.IsSwitchInventoryPosition() && false == cell.IsCostumeInventoryPosition()
#endif
				)
					sys_err("CItem::RemoveFromCharacter: Invalid Item Position");
				else
				{
					pOwner->SetItem(cell, NULL);
				}
			}
		}

		m_pOwner = NULL;
		m_wCell = 0;

		SetWindow(RESERVED_WINDOW);
		Save();
		return (this);
	}
}

#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
bool CItem::AddToCharacter(LPCHARACTER ch, TItemPos Cell, bool bHighlight)
#else
bool CItem::AddToCharacter(LPCHARACTER ch, TItemPos Cell)
#endif
{
	assert(GetSectree() == NULL);
	assert(m_pOwner == NULL);

	WORD pos = Cell.cell;
	BYTE window_type = Cell.window_type;

	if (INVENTORY == window_type)
	{
		if (m_wCell >= INVENTORY_MAX_NUM && BELT_INVENTORY_SLOT_START > m_wCell)
		{
			sys_err("CItem::AddToCharacter: cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
	else if (DRAGON_SOUL_INVENTORY == window_type)
	{
		if (m_wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CItem::AddToCharacter: cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
#ifdef ENABLE_SWITCHBOT
	else if (SWITCHBOT == window_type)
	{
		if (m_wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CItem::AddToCharacter:switchbot cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
#endif

	bool highlight = this->GetLastOwnerPID() != ch->GetPlayerID() ? true : false;

	if (ch->GetDesc())
		m_dwLastOwnerPID = ch->GetPlayerID();

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if ((GetType() == ITEM_COSTUME) && (GetSubType() == COSTUME_ACCE) && (GetSocket(ACCE_ABSORPTION_SOCKET) == 0))
	{
		long lVal = GetValue(ACCE_GRADE_VALUE_FIELD);
		switch (lVal)
		{
		case 2:
			lVal = ACCE_GRADE_2_ABS;
			break;
		case 3:
			lVal = ACCE_GRADE_3_ABS;
			break;
		case 4:
			lVal = number(ACCE_GRADE_4_ABS_MIN, ACCE_GRADE_4_ABS_MAX_COMB);
			break;
		default:
			lVal = ACCE_GRADE_1_ABS;
			break;
		}
		SetSocket(ACCE_ABSORPTION_SOCKET, lVal);
	}
#endif

	event_cancel(&m_pkDestroyEvent);

#if defined(__BL_ENABLE_PICKUP_ITEM_EFFECT__)
	ch->SetItem(TItemPos(window_type, pos), this, highlight );
#else
	ch->SetItem(TItemPos(window_type, pos), this);
#endif
	m_pOwner = ch;

	Save();
	return true;
}

LPITEM CItem::RemoveFromGround()
{
	if (GetSectree())
	{
		SetOwnership(NULL);

		GetSectree()->RemoveEntity(this);

		ViewCleanup();

		Save();
	}

	return (this);
}

bool CItem::AddToGround(long lMapIndex, const PIXEL_POSITION & pos, bool skipOwnerCheck)
{
	if (0 == lMapIndex)
	{
		sys_err("wrong map index argument: %d", lMapIndex);
		return false;
	}

	if (GetSectree())
	{
		sys_err("sectree already assigned");
		return false;
	}

	if (!skipOwnerCheck && m_pOwner)
	{
		sys_err("owner pointer not null");
		return false;
	}

	LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, pos.x, pos.y);

	if (!tree)
	{
		sys_err("cannot find sectree by %dx%d", pos.x, pos.y);
		return false;
	}

	SetWindow(GROUND);
	SetXYZ(pos.x, pos.y, pos.z);
	tree->InsertEntity(this);
	UpdateSectree();
	Save();
	return true;
}

bool CItem::AddToGround2(long lMapIndex, const PIXEL_POSITION & pos, bool skipOwnerCheck, LPCHARACTER chr, int sec)
{
	SetOwnership(chr, sec);

	if (0 == lMapIndex)
	{
		sys_err("wrong map index argument: %d", lMapIndex);
		return false;
	}

	if (GetSectree())
	{
		sys_err("sectree already assigned");
		return false;
	}

	if (!skipOwnerCheck && m_pOwner)
	{
		sys_err("owner pointer not null");
		return false;
	}

	LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, pos.x, pos.y);

	if (!tree)
	{
		sys_err("cannot find sectree by %dx%d", pos.x, pos.y);
		return false;
	}

	SetWindow(GROUND);
	SetXYZ(pos.x, pos.y, pos.z);
	tree->InsertEntity(this);
	UpdateSectree();
	Save();
	return true;
}
bool CItem::DistanceValid(LPCHARACTER ch)
{
	if (!GetSectree())
		return false;

	int iDist = DISTANCE_APPROX(GetX() - ch->GetX(), GetY() - ch->GetY());

	if (ch->CanPickupDirectly() && iDist <= 2000)
		return true;
		
	if (iDist > 600) // @fixme173 300 to 600
		return false;

	return true;
}

bool CItem::CanUsedBy(LPCHARACTER ch)
{
#ifdef __BUFFI_SUPPORT__
	if ((ch->IsLookingBuffiPage() && GetType() == ITEM_COSTUME && GetSubType() == COSTUME_WEAPON))
	{
		if (GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
			return false;
		return true;
	}
#endif

	// Anti flag check
	switch (ch->GetJob())
	{
		case JOB_WARRIOR:
			if (GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (GetAntiFlag() & ITEM_ANTIFLAG_SURA)
				return false;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			if (GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN)
				return false;
			break;
#endif
	}

	return true;
}

int CItem::FindEquipCell(LPCHARACTER ch, int iCandidateCell)
{
	if ((0 == GetWearFlag() || ITEM_TOTEM == GetType()) && ITEM_COSTUME != GetType() && ITEM_DS != GetType() && ITEM_SPECIAL_DS != GetType() && ITEM_RING != GetType() && ITEM_BELT != GetType()
#ifdef ENABLE_NEW_PET_SYSTEM
		&& !IsNewPetItem() && !IsSkinPet() && !IsSkinMount() && !IsAcceSkin() && !IsPendantSoul() && !IsPendant()
#endif
	)
		return -1;

	if (GetType() == ITEM_DS || GetType() == ITEM_SPECIAL_DS)
	{
		if (iCandidateCell < 0)
		{
			return WEAR_MAX_NUM + GetSubType();
		}
		else
		{
			for (int i = 0; i < DRAGON_SOUL_DECK_MAX_NUM; i++)
			{
				if (WEAR_MAX_NUM + i * DS_SLOT_MAX + GetSubType() == iCandidateCell)
				{
					return iCandidateCell;
				}
			}
			return -1;
		}
	}


#ifdef __PENDANT__
	else if (GetType() == ITEM_ARMOR && GetSubType() >= ARMOR_PENDANT_FIRE && GetSubType() <= ARMOR_PENDANT_SOUL)
	{
		switch (GetSubType())
		{
			case ARMOR_PENDANT_FIRE:
				return WEAR_PENDANT_FIRE;
			case ARMOR_PENDANT_ICE:
				return WEAR_PENDANT_ICE;
			case ARMOR_PENDANT_THUNDER:
				return WEAR_PENDANT_THUNDER;
			case ARMOR_PENDANT_WIND:
				return WEAR_PENDANT_WIND;
			case ARMOR_PENDANT_DARK:
				return WEAR_PENDANT_DARK;
			case ARMOR_PENDANT_EARTH:
				return WEAR_PENDANT_EARTH;
			case ARMOR_PENDANT_SOUL:
				if (GetVnum() == 10790)
					return WEAR_PENDANT_SOUL_FIRST;
				else if (GetVnum() == 10791)
					return WEAR_PENDANT_SOUL_SECOND;
		}
		return -1;
	}
#endif

	else if (GetType() == ITEM_COSTUME)
	{
#ifdef __BUFFI_SUPPORT__
		if (ch && ch->IsLookingBuffiPage())
		{
			//not teach these method
			if (GetSubType() == COSTUME_BODY)
			{
				if (IS_SET(GetAntiFlag(), ITEM_ANTIFLAG_FEMALE))
					return -1;
				return WEAR_BUFFI_BODY;
			}
			else if (GetSubType() == COSTUME_HAIR)
			{
				if (IS_SET(GetAntiFlag(), ITEM_ANTIFLAG_FEMALE))
					return -1;
				return WEAR_BUFFI_HEAD;
			}
			else if (GetSubType() == COSTUME_WEAPON)
				return WEAR_BUFFI_WEAPON;
			else if (GetSubType() == COSTUME_ACCE)
				return WEAR_BUFFI_SASH;
			else if (GetSubType() == COSTUME_ACCE_SKIN)
				return WEAR_BUFFI_SASH;
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			else if (GetSubType() == COSTUME_MOUNT)
				return WEAR_COSTUME_MOUNT;
#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
			else if (GetSubType() == COSTUME_WEAPON)
				return WEAR_COSTUME_WEAPON;
#endif
			else if (GetSubType() == COSTUME_PET)
				return WEAR_NORMAL_PET;
				
			else if (GetSubType() == COSTUME_ACCE_SKIN)
				return WEAR_COSTUME_ACCE_SKIN;
			
			else if (GetSubType() == COSTUME_MOUNT_SKIN)
				return WEAR_COSTUME_MOUNT_SKIN;

			else if (GetSubType() == COSTUME_NORMAL_PET_SKIN)
				return WEAR_COSTUME_NORMAL_PET_SKIN;
		}
		else
#endif
		{
			if (GetSubType() == COSTUME_BODY)
				return WEAR_COSTUME_BODY;
			else if (GetSubType() == COSTUME_HAIR)
				return WEAR_COSTUME_HAIR;
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			else if (GetSubType() == COSTUME_MOUNT)
				return WEAR_COSTUME_MOUNT;
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
			else if (GetSubType() == COSTUME_ACCE)
				return WEAR_COSTUME_ACCE;
#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
			else if (GetSubType() == COSTUME_WEAPON)
				return WEAR_COSTUME_WEAPON;
#endif
			else if (GetSubType() == COSTUME_PET)
				return WEAR_NORMAL_PET;
				
			else if (GetSubType() == COSTUME_ACCE_SKIN)
				return WEAR_COSTUME_ACCE_SKIN;
			
			else if (GetSubType() == COSTUME_MOUNT_SKIN)
				return WEAR_COSTUME_MOUNT_SKIN;

			else if (GetSubType() == COSTUME_NORMAL_PET_SKIN)
				return WEAR_COSTUME_NORMAL_PET_SKIN;
		}
	}
	else if (GetType() == ITEM_BELT)
		return WEAR_BELT;


	#ifdef ENABLE_GLOVE_SYSTEM
	else if (GetType() == ITEM_ARMOR && GetSubType() == ARMOR_GLOVE) // (GetWearFlag() & WEARABLE_GLOVE)
		return WEAR_GLOVE;
	#endif
#ifdef ENABLE_NEW_PET_SYSTEM
	else if (IsNewPetItem())
		return WEAR_PET;
#endif

	if (GetWearFlag() & WEARABLE_BODY)
		return WEAR_BODY;
	else if (GetWearFlag() & WEARABLE_WEAPON)
		return WEAR_WEAPON;

	if (GetWearFlag() & WEARABLE_HEAD)
		return WEAR_HEAD;

	else if (GetWearFlag() & WEARABLE_FOOTS)
		return WEAR_FOOTS;

	else if (GetWearFlag() & WEARABLE_WRIST)
		return WEAR_WRIST;

	else if (GetWearFlag() & WEARABLE_SHIELD)
		return WEAR_SHIELD;

	else if (GetWearFlag() & WEARABLE_NECK)
		return WEAR_NECK;

	else if (GetWearFlag() & WEARABLE_EAR)
		return WEAR_EAR;

	else if (GetWearFlag() & WEARABLE_ARROW)
		return WEAR_ARROW;

	else if (GetWearFlag() & WEARABLE_UNIQUE)
	{
		if (ch->GetWear(WEAR_UNIQUE1))
			return WEAR_UNIQUE2;
		else
			return WEAR_UNIQUE1;
	}

	else if (GetWearFlag() & WEARABLE_RING1)
		return WEAR_RING1;

	else if (GetWearFlag() & WEARABLE_RING2)
		return WEAR_RING2;

	else if (GetWearFlag() & WEARABLE_BUFFI_1)
		return WEAR_BUFFI_RING_0;
	
	else if (GetWearFlag() & WEARABLE_BUFFI_2)
		return WEAR_BUFFI_RING_1;

	else if (GetWearFlag() & WEARABLE_BUFFI_3)
		return WEAR_BUFFI_RING_2;

	else if (GetWearFlag() & WEARABLE_ABILITY)
	{
		if (!ch->GetWear(WEAR_ABILITY1))
		{
			return WEAR_ABILITY1;
		}
		else if (!ch->GetWear(WEAR_ABILITY2))
		{
			return WEAR_ABILITY2;
		}
		else if (!ch->GetWear(WEAR_ABILITY3))
		{
			return WEAR_ABILITY3;
		}
		else if (!ch->GetWear(WEAR_ABILITY4))
		{
			return WEAR_ABILITY4;
		}
		else if (!ch->GetWear(WEAR_ABILITY5))
		{
			return WEAR_ABILITY5;
		}
		else if (!ch->GetWear(WEAR_ABILITY6))
		{
			return WEAR_ABILITY6;
		}
		else if (!ch->GetWear(WEAR_ABILITY7))
		{
			return WEAR_ABILITY7;
		}
		else if (!ch->GetWear(WEAR_ABILITY8))
		{
			return WEAR_ABILITY8;
		}
		else
		{
			return -1;
		}
	}
	return -1;
}

#ifdef ENABLE_NEW_PET_SYSTEM
int FindPetBonusLevel(long value)
{
	const WORD levelRanges[] = { 349, 324, 299, 274, 249, 224, 204, 184, 164, 144, 124, 109, 94, 79, 64, 49, 39, 29, 19, 9 };
	for (WORD j = 0; j < _countof(levelRanges); ++j)
	{
		if (value > levelRanges[j])
			return _countof(levelRanges) - j;
	}
	return 0;
}
#endif

void CItem::ModifyPoints(bool bAdd)
{
#ifdef ENABLE_NEW_PET_SYSTEM
	if (GetType() == ITEM_PET && GetSubType() == PET_LEVELABLE)
	{
		int bonusTypes[][2] = { {APPLY_MAX_HP,4000},{APPLY_ATTBONUS_MONSTER,20},{APPLY_CRITICAL_DAMAGE_PVM,10} };
		for (BYTE i = 0; i < 3; ++i)
		{
			const long bonusLevel = FindPetBonusLevel(GetSocket(POINT_PET_BONUS_1+i));
			if (bonusLevel == 0)
				continue;
			int value = bonusLevel == 20 ? bonusTypes[i][1]:bonusLevel*(float(bonusTypes[i][1])/20.0);
			m_pOwner->ApplyPoint(bonusTypes[i][0], bAdd ? value : -value);
		}

		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
		{
			BYTE skillType = GetAttributeType(j);
			if (skillType == 0 || skillType == 99)
				continue;
			auto it = petSkillBonus.find(skillType);
			if (it != petSkillBonus.end())
			{
				BYTE skillLevel = GetAttributeValue(j);
				for (BYTE i = 0; i < it->second.size(); ++i)
				{
					auto bonus = it->second[i];
					int value = skillLevel * (float(bonus.second) / 20.0);
					m_pOwner->ApplyPoint(bonus.first, bAdd ? value : -value);
				}
			}
		}
		return;
	}
#endif

#ifdef __PENDANT__
	if (IsPendantSoul())
	{
		for (BYTE i = WEAR_PENDANT_FIRE; i <= WEAR_PENDANT_EARTH; ++i)
		{
			LPITEM item = m_pOwner->GetWear(i);
			if (!item)
				return;
			if (item->GetRefineLevel() != 25)
				return;
		}
	}
#endif

#ifdef __BUFFI_SUPPORT__
	if (IsEquipped() && GetCell()-INVENTORY_MAX_NUM >= WEAR_BUFFI_BODY && GetCell()-INVENTORY_MAX_NUM < WEAR_BUFFI_RING_0)
		return;
#endif

#ifdef __RENEWAL_MOUNT__
	if (IsCostumeMountItem() && !m_pOwner->IsRiding())
		return;
#endif

	int accessoryGrade;

	if (!IsAccessoryForSocket())
	{
		if (m_pProto->bType == ITEM_WEAPON || m_pProto->bType == ITEM_ARMOR)
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				DWORD dwVnum;

				if ((dwVnum = GetSocket(i)) <= 2)
					continue;

				TItemTable * p = ITEM_MANAGER::instance().GetTable(dwVnum);

				if (!p)
				{
					if (GetType() != ITEM_WEAPON)
						sys_err("cannot find table by vnum %u", dwVnum);
					continue;
				}

				if (ITEM_METIN == p->bType)
				{
					//m_pOwner->ApplyPoint(p->alValues[0], bAdd ? p->alValues[1] : -p->alValues[1]);
					for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
					{
						if (p->aApplies[i].bType == APPLY_NONE)
							continue;

						if (p->aApplies[i].bType == APPLY_SKILL)
							m_pOwner->ApplyPoint(p->aApplies[i].bType, bAdd ? p->aApplies[i].lValue : p->aApplies[i].lValue ^ 0x00800000);
						else
							m_pOwner->ApplyPoint(p->aApplies[i].bType, bAdd ? p->aApplies[i].lValue : -p->aApplies[i].lValue);
					}
				}
			}
		}

		accessoryGrade = 0;
	}
	else
	{
		accessoryGrade = MIN(GetAccessorySocketGrade(), ITEM_ACCESSORY_SOCKET_MAX_NUM);
	}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if (IsCostumeAcce() && GetSocket(ACCE_ABSORBED_SOCKET))
	{
		TItemTable * pkItemAbsorbed = ITEM_MANAGER::instance().GetTable(GetSocket(ACCE_ABSORBED_SOCKET));
		if (pkItemAbsorbed)
		{
			if ((pkItemAbsorbed->bType == ITEM_ARMOR) && (pkItemAbsorbed->bSubType == ARMOR_BODY))
			{
				long lDefGrade = pkItemAbsorbed->alValues[1] + long(pkItemAbsorbed->alValues[5] * 2);
				double dValue = lDefGrade * GetSocket(ACCE_ABSORPTION_SOCKET);
				dValue = (double)dValue / 100;
				lDefGrade = (long)dValue;
				if ((pkItemAbsorbed->alValues[1] > 0 && (lDefGrade <= 0)) || (pkItemAbsorbed->alValues[5] > 0 && (lDefGrade < 1)))
					lDefGrade += 1;
				else if ((pkItemAbsorbed->alValues[1] > 0) || (pkItemAbsorbed->alValues[5] > 0))
					lDefGrade += 1;

				m_pOwner->ApplyPoint(APPLY_DEF_GRADE_BONUS, bAdd ? lDefGrade : -lDefGrade);

				long lDefMagicBonus = pkItemAbsorbed->alValues[0];
				dValue = lDefMagicBonus * GetSocket(ACCE_ABSORPTION_SOCKET);
				dValue = (double)dValue / 100;
				lDefMagicBonus = (long)dValue;
				if ((pkItemAbsorbed->alValues[0] > 0) && (lDefMagicBonus < 1))
					lDefMagicBonus += 1;
				else if (pkItemAbsorbed->alValues[0] > 0)
					lDefMagicBonus += 1;

				m_pOwner->ApplyPoint(APPLY_MAGIC_DEF_GRADE, bAdd ? lDefMagicBonus : -lDefMagicBonus);
			}
			else if (pkItemAbsorbed->bType == ITEM_WEAPON)
			{
				long lAttGrade = pkItemAbsorbed->alValues[4] + pkItemAbsorbed->alValues[5];
				if (pkItemAbsorbed->alValues[3] > pkItemAbsorbed->alValues[4])
					lAttGrade = pkItemAbsorbed->alValues[3] + pkItemAbsorbed->alValues[5];

				double dValue = lAttGrade * GetSocket(ACCE_ABSORPTION_SOCKET);
				dValue = (double)dValue / 100;
				lAttGrade = (long)dValue;
				if (((pkItemAbsorbed->alValues[3] > 0) && (lAttGrade < 1)) || ((pkItemAbsorbed->alValues[4] > 0) && (lAttGrade < 1)))
					lAttGrade += 1;
				else if ((pkItemAbsorbed->alValues[3] > 0) || (pkItemAbsorbed->alValues[4] > 0))
					lAttGrade += 1;

				m_pOwner->ApplyPoint(APPLY_ATT_GRADE_BONUS, bAdd ? lAttGrade : -lAttGrade);

				long lAttMagicGrade = pkItemAbsorbed->alValues[2] + pkItemAbsorbed->alValues[5];
				if (pkItemAbsorbed->alValues[1] > pkItemAbsorbed->alValues[2])
					lAttMagicGrade = pkItemAbsorbed->alValues[1] + pkItemAbsorbed->alValues[5];

				dValue = lAttMagicGrade * GetSocket(ACCE_ABSORPTION_SOCKET);
				dValue = (double)dValue / 100;
				lAttMagicGrade = (long)dValue;
				if (((pkItemAbsorbed->alValues[1] > 0) && (lAttMagicGrade < 1)) || ((pkItemAbsorbed->alValues[2] > 0) && (lAttMagicGrade < 1)))
					lAttMagicGrade += 1;
				else if ((pkItemAbsorbed->alValues[1] > 0) || (pkItemAbsorbed->alValues[2] > 0))
					lAttMagicGrade += 1;

				m_pOwner->ApplyPoint(APPLY_MAGIC_ATT_GRADE, bAdd ? lAttMagicGrade : -lAttMagicGrade);
			}
		}
	}
#endif

	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
	{
		if (m_pProto->aApplies[i].bType == APPLY_NONE && !IsCostumeAcce())
			continue;

		BYTE bType = m_pProto->aApplies[i].bType;
		long lValue = m_pProto->aApplies[i].lValue;

		if (IsCostumeAcce())
		{
		
			TItemTable* pkItemAbsorbed = ITEM_MANAGER::instance().GetTable(GetSocket(ACCE_ABSORBED_SOCKET));
			if (pkItemAbsorbed)
			{
				if (pkItemAbsorbed->aApplies[i].bType == APPLY_NONE)
					continue;

				bType = pkItemAbsorbed->aApplies[i].bType;
				lValue = pkItemAbsorbed->aApplies[i].lValue;
				if (lValue < 0)
					continue;

				double dValue = lValue * GetSocket(ACCE_ABSORPTION_SOCKET);
				dValue = (double)dValue / 100;
				lValue = (long)dValue;

				if ((pkItemAbsorbed->aApplies[i].lValue > 0) && (lValue <= 0))
					lValue += 1;

				if (m_pProto->aApplies[i].bType == APPLY_SKILL)
				{
					m_pOwner->ApplyPoint(bType, bAdd ? lValue : lValue ^ 0x00800000);
				}
				else
				{
					if (0 != accessoryGrade)
						lValue += MAX(accessoryGrade, lValue * aiAccessorySocketEffectivePct[accessoryGrade] / 100);

					m_pOwner->ApplyPoint(bType, bAdd ? lValue : -lValue);
				}
			}
			else
				continue;
		}
		
		if (m_pProto->aApplies[i].bType == APPLY_SKILL)
		{
			m_pOwner->ApplyPoint(m_pProto->aApplies[i].bType, bAdd ? lValue : lValue ^ 0x00800000);
		}
		else
		{
			if (0 != accessoryGrade)
				lValue += MAX(accessoryGrade, lValue * aiAccessorySocketEffectivePct[accessoryGrade] / 100);

			m_pOwner->ApplyPoint(m_pProto->aApplies[i].bType, bAdd ? lValue : -lValue);
		}
	}

	if (true == CItemVnumHelper::IsRamadanMoonRing(GetVnum()) || true == CItemVnumHelper::IsHalloweenCandy(GetVnum())
		|| true == CItemVnumHelper::IsHappinessRing(GetVnum()) || true == CItemVnumHelper::IsLovePendant(GetVnum()))
	{
		// Do not anything.
	}
	else
	{
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			if (GetAttributeType(i))
			{
				const TPlayerItemAttribute &ia = GetAttribute(i);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
				long sValue = ia.sValue;

				if (GetType() == ITEM_COSTUME && GetSubType() == COSTUME_ACCE)
				{
					double dValue = sValue * GetSocket(ACCE_ABSORPTION_SOCKET);
					dValue = (double)dValue / 100;
					sValue = (long)dValue;
					if ((ia.sValue > 0) && (sValue <= 0))
						sValue += 1;
				}
#endif
#ifdef ENABLE_DS_SET
				if ((IsDragonSoul()) && (m_pOwner->FindAffect(AFFECT_DS_SET)))
					sValue += i < DSManager::instance().GetApplyCount(GetVnum()) ? DSManager::instance().GetBasicApplyValue(GetVnum(), ia.bType, true) : DSManager::instance().GetAdditionalApplyValue(GetVnum(), ia.bType, true);
				
				if (ia.bType == APPLY_SKILL)
					m_pOwner->ApplyPoint(ia.bType, bAdd ? sValue : sValue ^ 0x00800000);
				else
					m_pOwner->ApplyPoint(ia.bType, bAdd ? sValue : -sValue);
#else
				if (ia.bType == APPLY_SKILL)
					m_pOwner->ApplyPoint(ia.bType, bAdd ? ia.sValue : ia.sValue ^ 0x00800000);
				else
					m_pOwner->ApplyPoint(ia.bType, bAdd ? ia.sValue : -ia.sValue);
#endif
			}
		}
	}

	switch (m_pProto->bType)
	{
		case ITEM_PICK:
		case ITEM_ROD:
			{
				if (bAdd)
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, GetVnum());
				}
				else
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, 0);
				}
			}
			break;

		case ITEM_WEAPON:
			{
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
				if (0 != m_pOwner->GetWear(WEAR_COSTUME_WEAPON))
					break;
#endif

				if (bAdd)
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, GetVnum());
				}
				else
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, 0);
				}
			}
			break;

		case ITEM_ARMOR:
			{
				if (0 != m_pOwner->GetWear(WEAR_COSTUME_BODY))
					break;

				if (GetSubType() == ARMOR_BODY || GetSubType() == ARMOR_HEAD || GetSubType() == ARMOR_FOOTS || GetSubType() == ARMOR_SHIELD)
				{
					if (bAdd)
					{
						if (GetProto()->bSubType == ARMOR_BODY)
							m_pOwner->SetPart(PART_MAIN, GetVnum());
					}
					else
					{
						if (GetProto()->bSubType == ARMOR_BODY)
							m_pOwner->SetPart(PART_MAIN, m_pOwner->GetOriginalPart(PART_MAIN));
					}
				}
			}
			break;

		case ITEM_COSTUME:
			{
				DWORD toSetValue = this->GetVnum();
				EParts toSetPart = PART_MAX_NUM;

				if (GetSubType() == COSTUME_BODY)
				{
					toSetPart = PART_MAIN;

					if (false == bAdd)
					{
						const CItem* pArmor = m_pOwner->GetWear(WEAR_BODY);
						toSetValue = (NULL != pArmor) ? pArmor->GetVnum() : m_pOwner->GetOriginalPart(PART_MAIN);
					}

				}

				else if (GetSubType() == COSTUME_HAIR)
				{
					toSetPart = PART_HAIR;
					toSetValue = (true == bAdd) ? this->GetValue(3) : 0;
				}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
				else if (GetSubType() == COSTUME_MOUNT)
				{
					// not need to do a thing in here
				}
#endif

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
				else if (GetSubType() == COSTUME_WEAPON)
				{
					toSetPart = PART_WEAPON;
					if (false == bAdd)
					{
						const CItem* pWeapon = m_pOwner->GetWear(WEAR_WEAPON);
						toSetValue = (NULL != pWeapon) ? pWeapon->GetVnum() : 0;
					}
				}
#endif

#ifdef __SKIN_SYSTEM__
				else if (GetSubType() == COSTUME_ACCE)
				{
					toSetPart = PART_ACCE;
					toSetValue = bAdd ? (m_pOwner->GetWear(WEAR_COSTUME_ACCE_SKIN) ? m_pOwner->GetWear(WEAR_COSTUME_ACCE_SKIN)->GetAcceVnum() : GetAcceVnum()) : 0;
				}
				else if (GetSubType() == COSTUME_ACCE_SKIN)
				{
					toSetPart = PART_ACCE;
					toSetValue = !bAdd ? (m_pOwner->GetWear(WEAR_COSTUME_ACCE) ? m_pOwner->GetWear(WEAR_COSTUME_ACCE)->GetAcceVnum() : 0) : GetAcceVnum();
				}
#endif
				if (PART_MAX_NUM != toSetPart)
				{
					m_pOwner->SetPart((BYTE)toSetPart, toSetValue);
					m_pOwner->UpdatePacket();
				}
			}
			break;
		case ITEM_UNIQUE:
			{
				if (0 != GetSIGVnum())
				{
					const CSpecialItemGroup* pItemGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(GetSIGVnum());
					if (NULL == pItemGroup)
						break;
					DWORD dwAttrVnum = pItemGroup->GetAttrVnum(GetVnum());
					const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(dwAttrVnum);
					if (NULL == pAttrGroup)
						break;
					for (itertype (pAttrGroup->m_vecAttrs) it = pAttrGroup->m_vecAttrs.begin(); it != pAttrGroup->m_vecAttrs.end(); it++)
					{
						m_pOwner->ApplyPoint(it->apply_type, bAdd ? it->apply_value : -it->apply_value);
					}
				}
			}
			break;
	}
}

#ifdef __SASH_SKIN__
bool CItem::IsDreamSoul()
{
	return (GetVnum() >= 67001 && GetVnum() <= 67010) ? true : false;
}
bool CItem::IsDreamSoulEnchant()
{
	return GetVnum() == 67011 ? true : false;
}
bool CItem::IsCostumeSashSkin()
{
	return  (GetType() == ITEM_COSTUME && GetSubType() == COSTUME_ACCE_SKIN) ? true : false;
}
BYTE CItem::GetDreamSoulIndex()
{
	const BYTE firstAttr = GetAttributeType(0);
	if (firstAttr)
	{
		for (const auto& [dreamIdx, dreamData] : m_dreamSoulData)
			if (dreamData.apply_idx == firstAttr)
				return dreamIdx;
	}
	return 0;
}
bool CItem::AddDreamSoul(LPITEM dreamSoulItem)
{
	long dreamSoulIdx = dreamSoulItem->GetValue(0);
	if (!dreamSoulIdx || dreamSoulIdx > 10)
		return false;
	const BYTE dreamSoulIsPvM = dreamSoulIdx <= 5 ? true : false;
	if (dreamSoulIdx > 5)
		dreamSoulIdx -= 5;
	//0-null:1-pvm:2-pvp
	const BYTE sashSkinDreamType = (GetDreamSoulIndex() == 0) ? 0 : (GetDreamSoulIndex() <= 5 ? 1 : 2);
	const int attrCount = GetAttributeCount();

	if (!(sashSkinDreamType == 0 || (sashSkinDreamType == 1 && dreamSoulIsPvM) || (sashSkinDreamType == 2 && !dreamSoulIsPvM)))
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't insert this item on it!"));
		return false;
	}

	if (attrCount > GetValue(0) && GetValue(0) != 3)
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to upgrade your sash costume in order to add more bonuses."));
		return false;	
	}

	if (attrCount != dreamSoulIdx - 1)
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Please add right dream soul index."));
		return false;
	}
	
	std::vector<std::pair<WORD, BYTE>> m_vec;
	GetSashSkinBonusValues(dreamSoulItem->GetAttributeType(0), m_vec);
	if(!m_vec.size())
		return false;

	SetForceAttribute(attrCount, dreamSoulItem->GetAttributeType(0), m_vec[0].first);

	return true;
}

void CItem::GetSashSkinBonusValues(BYTE bApply, std::vector<std::pair<WORD, BYTE>>& m_vec)
{
	m_vec.clear();
	for(auto it = m_dreamSoulData.begin(); it != m_dreamSoulData.end(); ++it)
	{
		if(it->second.apply_idx == bApply)
		{
			for(BYTE i=0;i<it->second.vecBonusData.size();++i)
				m_vec.emplace_back(it->second.vecBonusData[i].first, it->second.vecBonusData[i].second);
		}
	}
}

bool CItem::GiveDreamSoulBonus()
{
	if (!IsDreamSoul())
		return false;
	const auto it = m_dreamSoulData.find(GetValue(0));
	if (it == m_dreamSoulData.end())
		return false;
	const long currentVal = GetAttributeValue(0);
	ClearAttribute();

	int idx = 0;
	int lowestValue = it->second.vecBonusData[0].first;
	
	for (int i = 1; i < it->second.vecBonusData.size(); i++) {
	    if (it->second.vecBonusData[i].first < lowestValue) {
	        lowestValue = it->second.vecBonusData[i].first;
	        idx = i;
	    }
	}
	SetForceAttribute(0, it->second.apply_idx, lowestValue);

	return true;
}

bool CItem::ChangeDreamSoulBonus()
{
	if (!IsCostumeSashSkin())
		return false;

	std::vector<std::pair<WORD, BYTE>> m_vec;
	for (BYTE i=0; i < GetAttributeCount(); ++i)
	{
		GetSashSkinBonusValues(GetAttributeType(i), m_vec);
		const long currentVal = GetAttributeValue(i);
		if(m_vec.size())
		{
			BYTE idx = 0;
			while (1)
			{
				idx = m_vec.size() == 1 ? 0 : number(0, m_vec.size() - 1);
				if (m_vec[idx].first == currentVal)//check_for_not_same_Value_for_next
					continue;
				else if (number(1, 100) < m_vec[idx].second)//check_lucky
					break;
			}
			SetForceAttribute(i, GetAttributeType(i), m_vec[idx].first);
		}
	}
	return GetAttributeCount() != 0 ? true : false;
}
#endif

bool CItem::IsEquipable() const
{
	switch (this->GetType())
	{
	case ITEM_COSTUME:
	case ITEM_ARMOR:
	case ITEM_WEAPON:
	case ITEM_ROD:
	case ITEM_PICK:
	case ITEM_UNIQUE:
	case ITEM_DS:
	case ITEM_SPECIAL_DS:
	case ITEM_RING:
	case ITEM_BELT:
#ifdef ENABLE_NEW_PET_SYSTEM
	case ITEM_PET:
#endif
		return true;
	}

	return false;
}

#define ENABLE_IMMUNE_FIX
// return false on error state
bool CItem::EquipTo(LPCHARACTER ch, WORD bWearCell)
{
	if (!ch)
	{
		sys_err("EquipTo: nil character");
		return false;
	}

	if (IsDragonSoul())
	{
		if (bWearCell < WEAR_MAX_NUM || bWearCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
		{
			sys_err("EquipTo: invalid dragon soul cell (this: #%d %s wearflag: %d cell: %d)", GetOriginalVnum(), GetName(), GetSubType(), bWearCell - WEAR_MAX_NUM);
			return false;
		}
	}
	else
	{
		if (bWearCell >= WEAR_MAX_NUM)
		{
			sys_log(0, "EquipTo: invalid wear cell (this: #%d %s wearflag: %d cell: %d)", GetOriginalVnum(), GetName(), GetWearFlag(), bWearCell);
			return false;
		}
	}

	if (ch->GetWear(bWearCell))
	{
		sys_err("EquipTo: item already exist (this: #%d %s cell: %d %s)", GetOriginalVnum(), GetName(), bWearCell, ch->GetWear(bWearCell)->GetName());
		return false;
	}

	if (GetOwner())
		RemoveFromCharacter();

	ch->SetWear(bWearCell, this);

	m_pOwner = ch;
	m_bEquipped = true;
	m_wCell	= INVENTORY_MAX_NUM + bWearCell;

#ifndef ENABLE_IMMUNE_FIX
	DWORD dwImmuneFlag = 0;

	for (int i = 0; i < WEAR_MAX_NUM; ++i)
	{
		if (m_pOwner->GetWear(i))
		{
			// m_pOwner->ChatPacket(CHAT_TYPE_INFO, "unequip immuneflag(%u)", m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag); // always 0
			SET_BIT(dwImmuneFlag, m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag);
		}
	}

	m_pOwner->SetImmuneFlag(dwImmuneFlag);
#endif

#ifdef __BUFFI_SUPPORT__
	if (bWearCell >= WEAR_BUFFI_BODY && bWearCell < WEAR_BUFFI_RING_0 && BUFFI_MANAGER::Instance().SummonCount(m_pOwner))
	{
		BUFFI* buffi = BUFFI_MANAGER::Instance().GetBuffi(m_pOwner->GetPlayerID(), 0);
		if (buffi)
			buffi->CheckSupportWear(false);
	}
#endif

	if (IsDragonSoul())
	{
		DSManager::instance().ActivateDragonSoul(this);
	}
	else
	{
		ModifyPoints(true);
		StartUniqueExpireEvent();
		if (-1 != GetProto()->cLimitTimerBasedOnWearIndex)
			StartTimerBasedOnWearExpireEvent();

		// ACCESSORY_REFINE
		StartAccessorySocketExpireEvent();
		// END_OF_ACCESSORY_REFINE
	}

#ifdef ENABLE_NEW_PET_SYSTEM
	if (GetOwner())
	{
		if (IsCostumePetItem()
#ifdef __SKIN_SYSTEM__
		|| IsSkinPet()
#endif
		)
		{
			CPetSystem* petSystem = GetOwner()->GetPetSystem();
			if (petSystem)
				petSystem->HandlePetCostumeItem();
		}
		if (IsNewPetItem())
		{
			CPetSystem* petSystem = GetOwner()->GetPetSystem();
			if (petSystem)
				petSystem->HandleNewPetItem();
		}
	}
#endif

	ch->BuffOnAttr_AddBuffsFromItem(this);

	m_pOwner->ComputeBattlePoints();
	m_pOwner->UpdatePacket();

#ifdef ENABLE_HIGHLIGHT_NEW_ITEM
	SetLastOwnerPID(m_pOwner->GetPlayerID());
#endif
	Save();

	return (true);
}

bool CItem::Unequip()
{
	if (!m_pOwner || GetCell() < INVENTORY_MAX_NUM)
	{
		// ITEM_OWNER_INVALID_PTR_BUG
		sys_err("%s %u m_pOwner %p, GetCell %d",
				GetName(), GetID(), get_pointer(m_pOwner), GetCell());
		// END_OF_ITEM_OWNER_INVALID_PTR_BUG
		return false;
	}

	if (this != m_pOwner->GetWear(GetCell() - INVENTORY_MAX_NUM))
	{
		sys_err("m_pOwner->GetWear() != this");
		return false;
	}

	if (IsRideItem())
		ClearMountAttributeAndAffect();

#ifdef ENABLE_NEW_PET_SYSTEM
	if (GetOwner())
	{
		if (IsCostumePetItem() || IsNewPetItem()
#ifdef __SKIN_SYSTEM__
		|| IsSkinPet()
#endif	
		)
		{
			CPetSystem* petSystem = GetOwner()->GetPetSystem();
			if (petSystem)
				petSystem->DeletePet(GetID());
		}
	}
#endif

	if (IsDragonSoul())
	{
		DSManager::instance().DeactivateDragonSoul(this);
	}
	else
	{
		ModifyPoints(false);
	}

	StopUniqueExpireEvent();

	if (-1 != GetProto()->cLimitTimerBasedOnWearIndex)
		StopTimerBasedOnWearExpireEvent();

	// ACCESSORY_REFINE
	StopAccessorySocketExpireEvent();
	// END_OF_ACCESSORY_REFINE

	m_pOwner->BuffOnAttr_RemoveBuffsFromItem(this);

	m_pOwner->SetWear(GetCell() - INVENTORY_MAX_NUM, NULL);
	
#ifdef __BUFFI_SUPPORT__
	if (GetCell() - INVENTORY_MAX_NUM >= WEAR_BUFFI_BODY && GetCell() - INVENTORY_MAX_NUM < WEAR_BUFFI_RING_0 && BUFFI_MANAGER::Instance().SummonCount(m_pOwner))
	{
		BUFFI* buffi = BUFFI_MANAGER::Instance().GetBuffi(m_pOwner->GetPlayerID(), 0);
		if (buffi)
			buffi->CheckSupportWear(false);
	}
#endif

#ifndef ENABLE_IMMUNE_FIX
	DWORD dwImmuneFlag = 0;

	for (int i = 0; i < WEAR_MAX_NUM; ++i)
	{
		if (m_pOwner->GetWear(i))
		{
			// m_pOwner->ChatPacket(CHAT_TYPE_INFO, "unequip immuneflag(%u)", m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag); // always 0
			SET_BIT(dwImmuneFlag, m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag);
		}
	}

	m_pOwner->SetImmuneFlag(dwImmuneFlag);
#endif


#ifdef __PENDANT__
	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(m_pOwner->GetPlayerID());

	if (IsPendant() && pPC)
		m_pOwner->ComputePoints();
	else
	{
		m_pOwner->ComputeBattlePoints();
		m_pOwner->UpdatePacket();
	}
	
#else
	m_pOwner->ComputeBattlePoints();
	m_pOwner->UpdatePacket();
#endif

	m_pOwner = NULL;
	m_wCell = 0;
	m_bEquipped	= false;

	return true;
}

#ifdef __PENDANT__
bool CItem::IsPendantSoul()
{
	switch (GetVnum())
	{
		case 10790:
		case 10791:
			return true;
	}
	return false;
}

bool CItem::IsPendant()
{
	if (GetType() == ITEM_ARMOR)
	{
		switch (GetSubType())
		{
			case ARMOR_PENDANT_FIRE:
			case ARMOR_PENDANT_ICE:
			case ARMOR_PENDANT_THUNDER:
			case ARMOR_PENDANT_WIND:
			case ARMOR_PENDANT_DARK:
			case ARMOR_PENDANT_EARTH:
				return true;
		}
	}
	return false;
}
#endif

bool CItem::IsZodiacItem()
{
	switch (GetVnum())
	{
		case 300:
		case 301:
		case 302:
		case 303:
		case 304:
		case 305:
		case 306:
		case 307:
		case 308:
		case 309:
		case 310:
		case 311:
		case 312:
		case 313:
		case 314:
		case 315:
		case 316:
		case 317:
		case 318:
		case 319:
		case 1180:
		case 1181:
		case 1182:
		case 1183:
		case 1184:
		case 1185:
		case 1186:
		case 1187:
		case 1188:
		case 1189:
		case 5160:
		case 5161:
		case 5162:
		case 5163:
		case 5164:
		case 5165:
		case 5166:
		case 5167:
		case 5168:
		case 5169:
		case 2200:
		case 2201:
		case 2202:
		case 2203:
		case 2204:
		case 2205:
		case 2206:
		case 2207:
		case 2208:
		case 2209:
		case 3220:
		case 3221:
		case 3222:
		case 3223:
		case 3224:
		case 3225:
		case 3226:
		case 3227:
		case 3228:
		case 3229:
		case 7300:
		case 7301:
		case 7302:
		case 7303:
		case 7304:
		case 7305:
		case 7306:
		case 7307:
		case 7308:
		case 7309:
		// case 19290: ARMURI
		// case 19291:
		// case 19292:
		// case 19293:
		// case 19294:
		// case 19295:
		// case 19296:
		// case 19297:
		// case 19298:
		// case 19299:
		// case 19490:
		// case 19491:
		// case 19492:
		// case 19493:
		// case 19494:
		// case 19495:
		// case 19496:
		// case 19497:
		// case 19498:
		// case 19499:
		// case 19690:
		// case 19691:
		// case 19692:
		// case 19693:
		// case 19694:
		// case 19695:
		// case 19696:
		// case 19697:
		// case 19698:
		// case 19699:
		// case 19890:
		// case 19891:
		// case 19892:
		// case 19893:
		// case 19894:
		// case 19895:
		// case 19896:
		// case 19897:
		// case 19898:
		// case 19899:
			return true;
	}
	return false;
}

long CItem::GetValue(DWORD idx)
{
	assert(idx < ITEM_VALUES_MAX_NUM);
	return GetProto()->alValues[idx];
}

void CItem::SetExchanging(bool bOn)
{
	m_bExchanging = bOn;
}

void CItem::Save()
{
	if (m_bSkipSave)
		return;

	ITEM_MANAGER::instance().DelayedSave(this);
}

bool CItem::CreateSocket(BYTE bSlot, BYTE bGold)
{
	assert(bSlot < ITEM_SOCKET_MAX_NUM);

	if (m_alSockets[bSlot] != 0)
	{
		sys_err("Item::CreateSocket : socket already exist %s %d", GetName(), bSlot);
		return false;
	}

	if (bGold)
		m_alSockets[bSlot] = 2;
	else
		m_alSockets[bSlot] = 1;

	UpdatePacket();

	Save();
	return true;
}

void CItem::SetSockets(const long * c_al)
{
	thecore_memcpy(m_alSockets, c_al, sizeof(m_alSockets));
	Save();
}

void CItem::SetSocket(int i, long v, bool bLog)
{
	assert(i < ITEM_SOCKET_MAX_NUM);
	m_alSockets[i] = v;
	UpdatePacket();
	Save();
}

long long CItem::GetGold()
{
	if (IS_SET(GetFlag(), ITEM_FLAG_COUNT_PER_1GOLD))
	{
		if (GetProto()->dwGold == 0)
			return GetCount();
		else
			return GetCount() / GetProto()->dwGold;
	}
	else
		return GetProto()->dwGold;
}

long long CItem::GetShopBuyPrice()
{
	return GetProto()->dwShopBuyPrice;
}

bool CItem::IsOwnership(LPCHARACTER ch)
{
	if (!m_pkOwnershipEvent)
		return true;

	return m_dwOwnershipPID == ch->GetPlayerID() ? true : false;
}

EVENTFUNC(ownership_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if (!info)
	{
		sys_err( "ownership_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;

	pkItem->SetOwnershipEvent(NULL);

#ifdef __ITEM_GROUND_ONLY_OWNER__
	pkItem->ViewReencode();
#else
	TPacketGCItemOwnership p;
	p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
	p.dwVID = pkItem->GetVID();
	p.szName[0] = '\0';

	pkItem->PacketAround(&p, sizeof(p));
#endif
	return 0;
}

void CItem::SetOwnershipEvent(LPEVENT pkEvent)
{
	m_pkOwnershipEvent = std::move(pkEvent); // fix
#ifdef __ITEM_GROUND_ONLY_OWNER__
	if (!m_pkOwnershipEvent)
		m_dwOwnershipPID = 0;
#endif
}

void CItem::SetOwnership(LPCHARACTER ch, int iSec)
{
	if (!ch)
	{
		if (m_pkOwnershipEvent)
		{
			event_cancel(&m_pkOwnershipEvent);
			m_dwOwnershipPID = 0;

			TPacketGCItemOwnership p;

			p.bHeader	= HEADER_GC_ITEM_OWNERSHIP;
			p.dwVID	= m_dwVID;
			p.szName[0]	= '\0';

			PacketAround(&p, sizeof(p));
		}
		return;
	}

	if (m_pkOwnershipEvent)
		return;

	if (iSec <= 10)
		iSec = 30;

	m_dwOwnershipPID = ch->GetPlayerID();

	item_event_info* info = AllocEventInfo<item_event_info>();
	strlcpy(info->szOwnerName, ch->GetName(), sizeof(info->szOwnerName));
	info->item = this;

	SetOwnershipEvent(event_create(ownership_event, info, PASSES_PER_SEC(iSec)));

	TPacketGCItemOwnership p;

	p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
	p.dwVID = m_dwVID;
	strlcpy(p.szName, ch->GetName(), sizeof(p.szName));

	PacketAround(&p, sizeof(p));
}

int CItem::GetSocketCount()
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
	{
		if (GetSocket(i) == 0)
			return i;
	}
	return ITEM_SOCKET_MAX_NUM;
}

bool CItem::AddSocket()
{
	int count = GetSocketCount();
	if (count == ITEM_SOCKET_MAX_NUM)
		return false;
	m_alSockets[count] = 1;
	return true;
}

void CItem::AlterToSocketItem(int iSocketCount)
{
	if (iSocketCount >= ITEM_SOCKET_MAX_NUM)
	{
		sys_log(0, "Invalid Socket Count %d, set to maximum", ITEM_SOCKET_MAX_NUM);
		iSocketCount = ITEM_SOCKET_MAX_NUM;
	}

	for (int i = 0; i < iSocketCount; ++i)
		SetSocket(i, 1);
}

void CItem::AlterToMagicItem()
{
	int idx = GetAttributeSetIndex();

	if (idx < 0)
		return;

	// Appearance Second Third
	// Weapon 50		20	 5
	// Armor  30		10	 2
	// Acc	20		10	 1

	int iSecondPct;
	int iThirdPct;

	switch (GetType())
	{
		case ITEM_WEAPON:
			iSecondPct = 20;
			iThirdPct = 5;
			break;

		case ITEM_ARMOR:
		case ITEM_COSTUME:
			if (GetSubType() == ARMOR_BODY)
			{
				iSecondPct = 10;
				iThirdPct = 2;
			}
			else
			{
				iSecondPct = 10;
				iThirdPct = 1;
			}
			break;

		default:
			return;
	}

	PutAttribute(aiItemMagicAttributePercentHigh);

	if (number(1, 100) <= iSecondPct)
		PutAttribute(aiItemMagicAttributePercentLow);

	if (number(1, 100) <= iThirdPct)
		PutAttribute(aiItemMagicAttributePercentLow);
}

DWORD CItem::GetRefineFromVnum()
{
	return ITEM_MANAGER::instance().GetRefineFromVnum(GetVnum());
}

int CItem::GetRefineLevel()
{
	const char* name = GetBaseName();
	char* p = const_cast<char*>(strrchr(name, '+'));

	if (!p)
		return 0;

	int	rtn = 0;
	str_to_number(rtn, p+1);

	const char* locale_name = GetName();
	p = const_cast<char*>(strrchr(locale_name, '+'));

	if (p)
	{
		int	locale_rtn = 0;
		str_to_number(locale_rtn, p+1);
		if (locale_rtn != rtn)
		{
			sys_err("refine_level_based_on_NAME(%d) is not equal to refine_level_based_on_LOCALE_NAME(%d).", rtn, locale_rtn);
		}
	}

	return rtn;
}

const char* CItem::GetLocaleName(BYTE bLocale)
{
	if (bLocale == LOCALE_MAX_NUM)
	{
		if (m_pOwner != NULL)
			bLocale = m_pOwner->GetLanguage();
		else
		{
			if (GetDesc())
				bLocale = GetDesc()->GetLanguage();
			else
				bLocale = LOCALE_DEFAULT;
		}
	}
	return m_pProto ? LC_ITEM_NAME(GetVnum(), bLocale) : NULL;
}

const char* CItem::GetName()
{
	BYTE bLocale;
	
	if (m_pOwner != NULL)
		bLocale = m_pOwner->GetLanguage();
	else
	{
		if (GetDesc())
			bLocale = GetDesc()->GetLanguage();
		else
			bLocale = LOCALE_DEFAULT;
	}
	return m_pProto ? LC_ITEM_NAME(GetVnum(), bLocale) : NULL;
}

bool CItem::IsPolymorphItem()
{
	return GetType() == ITEM_POLYMORPH;
}

EVENTFUNC(unique_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if (!info)
	{
		sys_err( "unique_expire_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;

	if (pkItem->GetValue(2) == 0)
	{
		if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) <= 1)
		{
			sys_log(0, "UNIQUE_ITEM: expire %s %u", pkItem->GetName(), pkItem->GetID());
			pkItem->SetUniqueExpireEvent(NULL);
			ITEM_MANAGER::instance().RemoveItem(pkItem, "UNIQUE_EXPIRE");
			return 0;
		}
		else
		{
			pkItem->SetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME, pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - 1);
			return PASSES_PER_SEC(60);
		}
	}
	else
	{
		time_t cur = get_global_time();

		if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) <= cur)
		{
			pkItem->SetUniqueExpireEvent(NULL);
			ITEM_MANAGER::instance().RemoveItem(pkItem, "UNIQUE_EXPIRE");
			return 0;
		}
		else
		{
			// by rtsummit
			if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - cur < 600)
				return PASSES_PER_SEC(pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - cur);
			else
				return PASSES_PER_SEC(600);
		}
	}
}

EVENTFUNC(timer_based_on_wear_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if (!info)
	{
		sys_err( "expire_event <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;
	int remain_time = pkItem->GetSocket(ITEM_SOCKET_REMAIN_SEC) - processing_time/passes_per_sec;
	if (remain_time <= 0)
	{
		sys_log(0, "ITEM EXPIRED : expired %s %u", pkItem->GetName(), pkItem->GetID());
		pkItem->SetTimerBasedOnWearExpireEvent(NULL);
		pkItem->SetSocket(ITEM_SOCKET_REMAIN_SEC, 0);

		if (pkItem->IsDragonSoul())
		{
			DSManager::instance().DeactivateDragonSoul(pkItem);
		}
		else
		{
			ITEM_MANAGER::instance().RemoveItem(pkItem, "TIMER_BASED_ON_WEAR_EXPIRE");
		}
		return 0;
	}
	pkItem->SetSocket(ITEM_SOCKET_REMAIN_SEC, remain_time);
	return PASSES_PER_SEC (MIN (60, remain_time));
}

void CItem::SetUniqueExpireEvent(LPEVENT pkEvent)
{
	m_pkUniqueExpireEvent = pkEvent;
}

void CItem::SetTimerBasedOnWearExpireEvent(LPEVENT pkEvent)
{
	m_pkTimerBasedOnWearExpireEvent = pkEvent;
}

EVENTFUNC(real_time_expire_event)
{
	const item_vid_event_info* info = reinterpret_cast<const item_vid_event_info*>(event->info);

	if (!info)
		return 0;

	const LPITEM item = ITEM_MANAGER::instance().FindByVID( info->item_vid );

	if (!item)
		return 0;

	LPCHARACTER owner = item->GetOwner();

	if (!owner)
		return 0;

	const time_t current = get_global_time();

	if (current > item->GetSocket(0))
	{
		if (item->IsNewPetItem()) {
			if (item->IsEquipped()) {
				owner->UnequipItem(item);
				return 0;
			}
			else {
				return 0;
			} 
		}

		if (item->GetVnum() && item->IsRideItem()) // @fixme152
			item->ClearMountAttributeAndAffect();

		if (item->IsCostumePetItem())
		{
			LPITEM petSkin = owner->GetWear(WEAR_COSTUME_NORMAL_PET_SKIN);
			if (petSkin)
				owner->UnequipItem(petSkin);
		}

#ifdef __RENEWAL_MOUNT__
		// normal mount expire
		if (item->IsCostumeMountItem() && item->IsEquipped())
		{
			owner->StopRiding();
			owner->HorseSummon(false);

			LPITEM mountSkin = owner->GetWear(WEAR_COSTUME_MOUNT_SKIN);
			if (mountSkin)
				owner->UnequipItem(mountSkin);
		}

		// skin mount expire
		if (item->IsSkinMount() && item->IsEquipped())
		{
			owner->StopRiding();
			owner->HorseSummon(false);
			LPITEM mount = owner->GetWear(WEAR_COSTUME_MOUNT);
			if (mount)
			{
				owner->HorseSummon(true, true, mount->GetValue(3));
				owner->StartRiding();
			}
		}
#endif
		bool skinPet = false;
		if (item->IsSkinPet() && item->IsEquipped())
			skinPet = true;

		ITEM_MANAGER::instance().RemoveItem(item, "REAL_TIME_EXPIRE");

		if (skinPet)
		{
			CPetSystem* petSystem = owner->GetPetSystem();
			if (petSystem)
				petSystem->HandlePetCostumeItem();
			skinPet = false;
		}
	}

	return PASSES_PER_SEC(1);
}

void CItem::StartRealTimeExpireEvent()
{
	if (m_pkRealTimeExpireEvent)
		return;

	if (IsNewPetItem())
	{
		item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
		info->item_vid = GetVID();

		m_pkRealTimeExpireEvent = event_create(real_time_expire_event, info, PASSES_PER_SEC(1));
		return;
	}

	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType || LIMIT_REAL_TIME_START_FIRST_USE == GetProto()->aLimits[i].bType)
		{
			item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
			info->item_vid = GetVID();

			m_pkRealTimeExpireEvent = event_create( real_time_expire_event, info, PASSES_PER_SEC(1));

			sys_log(0, "REAL_TIME_EXPIRE: StartRealTimeExpireEvent");

			return;
		}
	}
}

#if defined(__EXTENDED_BLEND_AFFECT__)
EVENTFUNC(blend_use_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("expire_event <Factor> Null pointer");
		return 0;
	}

	LPITEM pkItem = info->item;
	int remain_time = pkItem->GetSocket(2) - processing_time / passes_per_sec;
	if (remain_time <= 0)
	{
		pkItem->SetSocket(2, 0);
		pkItem->SetSocket(3, false);
		pkItem->Lock(false);
		ITEM_MANAGER::instance().RemoveItem(pkItem, "BLEND_USE_EXPIRE");
		return 0;
	}

	pkItem->SetSocket(2, remain_time);
	return PASSES_PER_SEC(MIN(1, remain_time));
}

void CItem::StartBlendExpireEvent()
{
	if (m_pkBlendUseEvent)
		return;

	if (IsRealTimeItem())
		return;

	if (!IsBlendItem())
		return;

	if (IsExtendedBlend(GetVnum()))
		return;

	int iSec = GetSocket(2);

	if (iSec >= 8640000)
		return;

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	m_pkBlendUseEvent = event_create(blend_use_expire_event, info, PASSES_PER_SEC(1));
}

void CItem::StopBlendExpireEvent()
{
	if (!m_pkBlendUseEvent)
		return;

	if (IsExtendedBlend(GetVnum()))
		return;

	int remain_time = GetSocket(2) - event_processing_time(m_pkBlendUseEvent) / passes_per_sec;

	SetSocket(2, remain_time);
	event_cancel(&m_pkBlendUseEvent);

	ITEM_MANAGER::instance().SaveSingleItem(this);
}
#endif

bool CItem::IsRealTimeItem()
{
	if(!GetProto())
		return false;
	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType)
			return true;
	}
	return false;
}

void CItem::StartUniqueExpireEvent()
{
	if (GetType() != ITEM_UNIQUE)
		return;

	if (m_pkUniqueExpireEvent)
		return;

	if (IsRealTimeItem())
		return;

	// HARD CODING
	if (GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		m_pOwner->ShowAlignment(false);

	int iSec = GetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME);

	if (iSec == 0)
		iSec = 60;
	else
		iSec = MIN(iSec, 60);

	SetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME, 0);

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetUniqueExpireEvent(event_create(unique_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StartTimerBasedOnWearExpireEvent()
{
	if (m_pkTimerBasedOnWearExpireEvent)
		return;

	if (IsRealTimeItem())
		return;

	if (-1 == GetProto()->cLimitTimerBasedOnWearIndex)
		return;

	int iSec = GetSocket(0);

	if (0 != iSec)
	{
		iSec %= 60;
		if (0 == iSec)
			iSec = 60;
	}

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetTimerBasedOnWearExpireEvent(event_create(timer_based_on_wear_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StopUniqueExpireEvent()
{
	if (!m_pkUniqueExpireEvent)
		return;

	if (GetValue(2) != 0)
		return;

	// HARD CODING
	if (GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		m_pOwner->ShowAlignment(true);

	SetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME, event_time(m_pkUniqueExpireEvent) / passes_per_sec);
	event_cancel(&m_pkUniqueExpireEvent);

	ITEM_MANAGER::instance().SaveSingleItem(this);
}

void CItem::StopTimerBasedOnWearExpireEvent()
{
	if (!m_pkTimerBasedOnWearExpireEvent)
		return;

	int remain_time = GetSocket(ITEM_SOCKET_REMAIN_SEC) - event_processing_time(m_pkTimerBasedOnWearExpireEvent) / passes_per_sec;

	SetSocket(ITEM_SOCKET_REMAIN_SEC, remain_time);
	event_cancel(&m_pkTimerBasedOnWearExpireEvent);

	ITEM_MANAGER::instance().SaveSingleItem(this);
}

void CItem::ApplyAddon(int iAddonType)
{
	CItemAddonManager::instance().ApplyAddonTo(iAddonType, this);
}

int CItem::GetSpecialGroup() const
{
	return ITEM_MANAGER::instance().GetSpecialGroupFromItem(GetVnum());
}

bool CItem::IsAccessoryForSocket()
{
	return (m_pProto->bType == ITEM_ARMOR && (m_pProto->bSubType == ARMOR_WRIST || m_pProto->bSubType == ARMOR_NECK || m_pProto->bSubType == ARMOR_EAR)) ||
		(m_pProto->bType == ITEM_BELT);
}

#ifdef __PERMA_ACCESSORY__
void CItem::CheckDownGradeTime()
{
	for (BYTE j = 0; j < 3; ++j)
	{
		if (GetSocket(j) > 0)
			SetSocket(j, -2);
	}
	for (int j = 2; j >= 0; --j)
	{
		if (GetSocket(j) == -2)
		{
			SetSocket(j, aiAccessorySocketDegradeTime[GetAccessorySocketGrade()]);
			break;
		}
	}
}

long getMinTime(long first, long second, long third)
{
	std::vector<long> getMini{first,second,third};
	std::sort(getMini.begin(), getMini.end()); 
	for(size_t i = 0;i < getMini.size();i++)
	{
		if(getMini[i] > 0)
			return getMini[i];
	}
	return 0;
}
DWORD CItem::GetAccessorySocketItemVnum(LPITEM item)
{
	DWORD ACCESSORY_MATERIAL_LIST[] = { 50623, 50624, 50625, 50626, 50627, 50628, 50629, 50630, 50631, 50632, 50633, 50634, 50635, 50636, 50637, 50638, 50639 };

	DWORD JewelAccessoryInfos[][4] = {
		// jewel		wrist	neck	ear
		{50634,	14420,	16220,	17220},
		{ 50635,	14500,	16500,	17500 },
		{ 50636,	14520,	16520,	17520 },
		{ 50637,	14540,	16540,	17540 },
		{ 50638,	14560,	16560,	17560 },
		{ 50639, 14570, 16570, 950150 },
		{ 50640, 950120, 950130, 950140 },
	};

	DWORD vnum = item->GetVnum();
	DWORD ret = vnum;
	DWORD item_base = (vnum / 10) * 10;
	for (size_t j = 0; j < (sizeof(JewelAccessoryInfos) / sizeof(JewelAccessoryInfos[0])); j++)
	{
		if (item->GetSubType() == ARMOR_WRIST)
		{
			if (JewelAccessoryInfos[j][1] == item_base)
				return JewelAccessoryInfos[j][0];
		}
		else if (item->GetSubType() == ARMOR_NECK)
		{
			if (JewelAccessoryInfos[j][2] == item_base)
				return JewelAccessoryInfos[j][0];
		}
		else if (item->GetSubType() == ARMOR_EAR)
		{
			if (JewelAccessoryInfos[j][3] == item_base)
				return JewelAccessoryInfos[j][0];
		}
	}

	if (vnum >= 16210 && vnum <= 16219)
		return 50625;

	if (item->GetSubType() == ARMOR_WRIST)
	{
		ret -= 14000;
	}
	else if (item->GetSubType() == ARMOR_NECK)
	{
		ret -= 16000;
	}
	else if (item->GetSubType() == ARMOR_EAR)
	{
		ret -= 17000;
	}

	DWORD type = ret / 20;

	if (type < 0 || type >= (sizeof(ACCESSORY_MATERIAL_LIST) / sizeof(ACCESSORY_MATERIAL_LIST[0])))
	{
		type = (ret - 170) / 20;
		if (type < 0 || type >= (sizeof(ACCESSORY_MATERIAL_LIST) / sizeof(ACCESSORY_MATERIAL_LIST[0])))
			return 0;
	}

	return ACCESSORY_MATERIAL_LIST[type];
}

void CItem::SetAccessorySocketGrade(bool isPerma)
{
	for (BYTE j = 0; j < 3; ++j)
	{
		if (GetSocket(j) == -1)
		{
			SetSocket(j, isPerma ? -3 : -2);
			break;
		}
	}
	GetOwner()->SetStatistics(STAT_TYPE_ADD_SOCKET, 1);
	CheckDownGradeTime();
}
#else
void CItem::SetAccessorySocketGrade(int iGrade)
{
	SetSocket(0, MINMAX(0, iGrade, GetAccessorySocketMaxGrade()));
	int iDownTime = aiAccessorySocketDegradeTime[GetAccessorySocketGrade()];
	SetAccessorySocketDownGradeTime(iDownTime);
}
#endif

#ifdef __PERMA_ACCESSORY__
void CItem::SetAccessorySocketMaxGrade(int iMaxGrade)
{
	for (BYTE j = 0; j < 3; ++j)
	{
		if (GetSocket(j) == 0)
		{
			SetSocket(j, -1);
			break;
		}
	}
}
#else
void CItem::SetAccessorySocketMaxGrade(int iMaxGrade)
{
	SetSocket(1, MINMAX(0, iMaxGrade, ITEM_ACCESSORY_SOCKET_MAX_NUM));
}
#endif

#ifdef __PERMA_ACCESSORY__
void CItem::SetAccessorySocketDownGradeTime(DWORD time)
{
	for (int j = 2; j >= 0; --j)
	{
		if (GetSocket(j) > 0)
		{
			SetSocket(j, time);
			break;
		}
	}
	if (test_server && GetOwner())
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s에서 소켓 빠질때까지 남은 시간 %d"), GetLocaleName(GetOwner()->GetLanguage()), time);

}
#else
void CItem::SetAccessorySocketDownGradeTime(DWORD time)
{
	SetSocket(2, time);
	if (test_server && GetOwner())
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s에서 소켓 빠질때까지 남은 시간 %d"), GetName(), time);

}
#endif

EVENTFUNC(accessory_socket_expire_event)
{
	item_vid_event_info* info = dynamic_cast<item_vid_event_info*>( event->info );

	if (info == NULL)
	{
		sys_err( "accessory_socket_expire_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM item = ITEM_MANAGER::instance().FindByVID(info->item_vid);


	if (item->GetAccessorySocketDownGradeTime() <= 1)
	{
degrade:
		item->SetAccessorySocketExpireEvent(NULL);
		item->AccessorySocketDegrade();
		return 0;
	}
	else
	{
		int iTime = item->GetAccessorySocketDownGradeTime() - 60;

		if (iTime <= 1)
			goto degrade;

		item->SetAccessorySocketDownGradeTime(iTime);

		if (iTime > 60)
			return PASSES_PER_SEC(60);
		else
			return PASSES_PER_SEC(iTime);
	}
}

void CItem::StartAccessorySocketExpireEvent()
{
	if (!IsAccessoryForSocket())
		return;

	if (m_pkAccessorySocketExpireEvent)
		return;

	if (GetAccessorySocketMaxGrade() == 0)
		return;

	if (GetAccessorySocketGrade() == 0)
		return;

#ifdef __PERMA_ACCESSORY__
	for (BYTE j = 0; j < 3; ++j)
	{
		if (GetSocket(j) > 0)
			break;
		if (j == 2)
			return;
	}
#endif

	int iSec = GetAccessorySocketDownGradeTime();
	SetAccessorySocketExpireEvent(NULL);

	if (iSec <= 1)
		iSec = 5;
	else
		iSec = MIN(iSec, 60);

	item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
	info->item_vid = GetVID();

	SetAccessorySocketExpireEvent(event_create(accessory_socket_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StopAccessorySocketExpireEvent()
{
	if (!m_pkAccessorySocketExpireEvent)
		return;

	if (!IsAccessoryForSocket())
		return;

	int new_time = GetAccessorySocketDownGradeTime() - (60 - event_time(m_pkAccessorySocketExpireEvent) / passes_per_sec);

	event_cancel(&m_pkAccessorySocketExpireEvent);

	if (new_time <= 1)
	{
		AccessorySocketDegrade();
	}
	else
	{
		SetAccessorySocketDownGradeTime(new_time);
	}
}

bool CItem::IsRideItem()
{
	return IsOldMountItem() || IsNewMountItem(); // @fixme152
}

bool CItem::IsOldMountItem()
{
	if (ITEM_UNIQUE == GetType() && UNIQUE_SPECIAL_RIDE == GetSubType())
		return true;
	if (ITEM_UNIQUE == GetType() && UNIQUE_SPECIAL_MOUNT_RIDE == GetSubType())
		return true;
	return false;
}

bool CItem::IsNewMountItem()
{
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (ITEM_COSTUME == GetType() && COSTUME_MOUNT == GetSubType())
		return true;
#endif
	return false;
}

bool CItem::IsRamadanRing()
{
	if (GetVnum() == UNIQUE_ITEM_RAMADAN_RING)
		return true;
	return false;
}

void CItem::ClearMountAttributeAndAffect()
{
	LPCHARACTER ch = GetOwner();
	if (!ch) // @fixme186
		return;

	ch->RemoveAffect(AFFECT_MOUNT);
	ch->RemoveAffect(AFFECT_MOUNT_BONUS);

	ch->MountVnum(0);

	ch->PointChange(POINT_ST, 0);
	ch->PointChange(POINT_DX, 0);
	ch->PointChange(POINT_HT, 0);
	ch->PointChange(POINT_IQ, 0);
}

void CItem::SetAccessorySocketExpireEvent(LPEVENT pkEvent)
{
	m_pkAccessorySocketExpireEvent = pkEvent;
}


void CItem::AccessorySocketDegrade()
{
	if (GetAccessorySocketGrade() > 0)
	{
		LPCHARACTER ch = GetOwner();

		if (ch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s에 박혀있던 보석이 사라집니다."), GetLocaleName(ch->GetLanguage()));
		}

#ifdef __PERMA_ACCESSORY__
		ModifyPoints(false);
		for (int j = 2; j >= 0; --j)
		{
			if (GetSocket(j) > 0)
			{
				if (j == 0)
				{
					if (GetSocket(1) == -2 || GetSocket(1) == -3)
					{
						if (GetSocket(2) == -2 || GetSocket(2) == -3)
						{
							SetSocket(0, GetSocket(1));
							SetSocket(1, GetSocket(2));
							SetSocket(2, -1);
						}
						else
						{
							SetSocket(0, GetSocket(1));
							SetSocket(1, -1);
						}
					}
					else
						SetSocket(j, -1);
				}
				else if (j == 1)
				{
					if (GetSocket(2) == -2 || GetSocket(2) == -3)
					{
						SetSocket(1, GetSocket(2));
						SetSocket(2, -1);
					}
					else
						SetSocket(j, -1);
				}
				else
					SetSocket(j, -1);
				break;
			}
		}
		ModifyPoints(true);
		CheckDownGradeTime();
		StartAccessorySocketExpireEvent();
#else
		ModifyPoints(false);
		SetAccessorySocketGrade(GetAccessorySocketGrade()-1);
		ModifyPoints(true);
#endif

	}
}

static const bool CanPutIntoRing(LPITEM ring, LPITEM item)
{
	//const DWORD vnum = item->GetVnum();
	return false;
}

bool CItem::CanPutInto(LPITEM item)
{
	if (item->GetType() == ITEM_BELT)
		return this->GetSubType() == USE_PUT_INTO_BELT_SOCKET;

	else if (item->GetType() == ITEM_RING)
		return CanPutIntoRing(item, this);

	else if (item->GetType() != ITEM_ARMOR)
		return false;

	DWORD accessoryIdx = GetVnum();
#ifdef __PERMA_ACCESSORY__
	if (accessoryIdx >= 55622)
		accessoryIdx -= 5000;
#endif

	DWORD vnum = item->GetVnum();

	struct JewelAccessoryInfo
	{
		DWORD jewel;
		DWORD wrist;
		DWORD neck;
		DWORD ear;
	};
	const static JewelAccessoryInfo infos[] = {
		{ 50634, 14420, 16220, 17220 },
		{ 50635, 14500, 16500, 17500 },
		{ 50636, 14520, 16520, 17520 },
		{ 50637, 14540, 16540, 17540 },
		{ 50638, 14560, 16560, 17560 },
		{ 50639, 14570, 16570, 950150 },
		{ 50640, 950120, 950130, 950140 },
	};

	DWORD item_type = (item->GetVnum() / 10) * 10;
	for (int i = 0; i < sizeof(infos) / sizeof(infos[0]); i++)
	{
		const JewelAccessoryInfo& info = infos[i];
		switch (item->GetSubType())
		{
		case ARMOR_WRIST:
			if (info.wrist == item_type)
				return info.jewel == accessoryIdx ? true : false;
			break;
		case ARMOR_NECK:
			if (info.neck == item_type)
				return info.jewel == accessoryIdx ? true : false;
			break;
		case ARMOR_EAR:
			if (info.ear == item_type)
				return info.jewel == accessoryIdx ? true : false;
			break;
		}
	}
	if (item->GetSubType() == ARMOR_WRIST)
		vnum -= 14000;
	else if (item->GetSubType() == ARMOR_NECK)
		vnum -= 16000;
	else if (item->GetSubType() == ARMOR_EAR)
		vnum -= 17000;
	else
		return false;

	DWORD type = vnum / 20;

	if (type < 0 || type > 11)
	{
		type = (vnum - 170) / 20;

		if (50623 + type != accessoryIdx)
			return false;
		else
			return true;
	}
	else if (item->GetVnum() >= 16210 && item->GetVnum() <= 16219)
	{
		if (50625 != accessoryIdx)
			return false;
		else
			return true;
	}
	else if (item->GetVnum() >= 16230 && item->GetVnum() <= 16239)
	{
		if (50626 != accessoryIdx)
			return false;
		else
			return true;
	}

	return 50623 + type == accessoryIdx;
}

bool CItem::CheckItemUseLevel(int nLevel)
{
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (this->m_pProto->aLimits[i].bType == LIMIT_LEVEL)
		{
			if (this->m_pProto->aLimits[i].lValue > nLevel) return false;
			else return true;
		}
	}
	return true;
}

long CItem::FindApplyValue(BYTE bApplyType)
{
	if (m_pProto == NULL)
		return 0;

	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
	{
		if (m_pProto->aApplies[i].bType == bApplyType)
			return m_pProto->aApplies[i].lValue;
	}

	return 0;
}

long CItem::FindApplyValueQuest(BYTE type)
{
	if (m_pProto == NULL)
		return 0;

	if (type >= ITEM_APPLY_MAX_NUM)
		return 0;

	return m_pProto->aApplies[type].lValue;
}

BYTE CItem::FindApplyTypeQuest(BYTE type)
{
	if (m_pProto == NULL)
		return 0;

	if (type >= ITEM_APPLY_MAX_NUM)
		return 0;

	return m_pProto->aApplies[type].bType;
}

bool CItem::IsNormalHairStyle()
{
	switch (GetVnum())
	{
		case 73005:
		case 73006:
		case 73007:
		case 73008:
		case 73009:
		case 73010:
		case 73011:
		case 73012:
		case 73251:
		case 73252:
		case 73253:
		case 73254:
		case 73255:
		case 73256:
		case 73257:
		case 73258:
		case 73259:
		case 73260:
		case 73261:
		case 73262:
		case 73501:
		case 73502:
		case 73503:
		case 73504:
		case 73505:
		case 73506:
		case 73507:
		case 73508:
		case 73509:
		case 73510:
		case 73511:
		case 73512:
		case 73751:
		case 73752:
		case 73753:
		case 73754:
		case 73755:
		case 73756:
		case 73757:
		case 73758:
		case 73759:
		case 73760:
		case 73761:
		case 73762:
			return true;
	}
	return false;
}

void CItem::CopySocketTo(LPITEM pItem)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		pItem->m_alSockets[i] = m_alSockets[i];
	}
}

#ifdef __PERMA_ACCESSORY__
int CItem::GetAccessorySocketGrade()
{
	for (int j = 2; j >= 0; --j)
	{
		const long socketData = GetSocket(j);
		if (socketData == -2 || socketData == -3 || socketData > 0)
			return j + 1;
	}
	return 0;
}
#else
int CItem::GetAccessorySocketGrade()
{
	return MINMAX(0, GetSocket(0), GetAccessorySocketMaxGrade());
}
#endif

#ifdef __PERMA_ACCESSORY__
int CItem::GetAccessorySocketMaxGrade()
{
	for (int j = 2; j >= 0; --j)
	{
		const long socketData = GetSocket(j);
		if (socketData == -1 || socketData == -2 || socketData == -3 || socketData > 0)
			return j + 1;
	}
	return 0;
}
#else
int CItem::GetAccessorySocketMaxGrade()
{
	return MINMAX(0, GetSocket(1), ITEM_ACCESSORY_SOCKET_MAX_NUM);
}
#endif

#ifdef __PERMA_ACCESSORY__
int CItem::GetAccessorySocketDownGradeTime()
{
	for (int j = 2; j >= 0; --j)
	{
		const long socketData = GetSocket(j);
		if (socketData > 0)
			return socketData;
	}
	return aiAccessorySocketDegradeTime[GetAccessorySocketGrade()];
}
#else
int CItem::GetAccessorySocketDownGradeTime()
{
	return MINMAX(0, GetSocket(2), aiAccessorySocketDegradeTime[GetAccessorySocketGrade()]);
}
#endif

void CItem::AttrLog()
{
	const char * pszIP = NULL;

	if (GetOwner() && GetOwner()->GetDesc())
		pszIP = GetOwner()->GetDesc()->GetHostName();
}

int CItem::GetLevelLimit()
{
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (this->m_pProto->aLimits[i].bType == LIMIT_LEVEL)
		{
			return this->m_pProto->aLimits[i].lValue;
		}
	}
	return 0;
}

int CItem::GetRealUseLimit()
{
	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++) {
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType || LIMIT_REAL_TIME_START_FIRST_USE == GetProto()->aLimits[i].bType)
			return this->m_pProto->aLimits[i].lValue;
	}
	return 0;
}

bool CItem::OnAfterCreatedItem()
{

#if defined(__EXTENDED_BLEND_AFFECT__)
	if (IsBlendItem())
	{
		if (GetSocket(3) == true)
		{
			Lock(true);
			StartBlendExpireEvent();
		}
	}
#endif

	if (-1 != this->GetProto()->cLimitRealTimeFirstUseIndex)
	{
		if (0 != GetSocket(1))
		{
			StartRealTimeExpireEvent();
		}
	}

	return true;
}

bool CItem::IsDragonSoul()
{
	return GetType() == ITEM_DS;
}

bool CItem::IsRemoveAfterUnequip()
{
	switch (GetVnum())
	{
		case 72001:
		case 72002:
		case 72003:
		case 72004:
		case 72006:
		case 70043:
			return true;
	}
	return false;
}

int CItem::GiveMoreTime_Per(float fPercent)
{
	if (IsDragonSoul())
	{
		DWORD duration = DSManager::instance().GetDuration(this);
		DWORD remain_sec = GetSocket(ITEM_SOCKET_REMAIN_SEC);
		DWORD given_time = fPercent * duration / 100u;
		if (remain_sec == duration)
			return false;
		if ((given_time + remain_sec) >= duration)
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, duration);
			return duration - remain_sec;
		}
		else
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, given_time + remain_sec);
			return given_time;
		}
	}

	else
		return 0;
}

int CItem::GiveMoreTime_Fix(DWORD dwTime)
{
	if (IsDragonSoul())
	{
		DWORD duration = DSManager::instance().GetDuration(this);
		DWORD remain_sec = GetSocket(ITEM_SOCKET_REMAIN_SEC);
		if (remain_sec == duration)
			return false;
		if ((dwTime + remain_sec) >= duration)
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, duration);
			return duration - remain_sec;
		}
		else
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, dwTime + remain_sec);
			return dwTime;
		}
	}

	else
		return 0;
}

int	CItem::GetDuration()
{
	if(!GetProto())
		return -1;

	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType)
			return GetProto()->aLimits[i].lValue;
	}

	if (GetProto()->cLimitTimerBasedOnWearIndex >= 0)
	{
		BYTE cLTBOWI = GetProto()->cLimitTimerBasedOnWearIndex;
		return GetProto()->aLimits[cLTBOWI].lValue;
	}

	return -1;
}

bool CItem::IsSameSpecialGroup(const LPITEM item) const
{
	if (this->GetVnum() == item->GetVnum())
		return true;

	if (GetSpecialGroup() && (item->GetSpecialGroup() == GetSpecialGroup()))
		return true;

	return false;
}

#ifdef __RENEWAL_MOUNT__
bool CItem::IsCostumeMountItem()
{
	return GetType() == ITEM_COSTUME && GetSubType() == COSTUME_MOUNT;
}
#endif

bool CItem::CannotBeDestroyed()
{
	switch (GetVnum())
	{
		// pet
		case 50317:
		case 30645:
		case 55401:
		case 55402:
		case 55403:
		case 55404:
		case 55405:
		case 55406:
		case 55407:
		case 55408:
		case 55409:
		case 55410:
		case 55411:
		// insotitori
		case 55701:
		case 55702:
		case 55703:
		case 55704:
		case 55705:
		case 55706:
		case 55707:
		case 55708:
		case 55709:
		case 55710:
		case 55711:
		// ucenic
		case 50187:
		// permise din temnite
		case 30139:
		case 50084:
		case 30304:
		case 30312:
		case 30327:
		case 31170:
		case 30333:
		case 30332:
		case 33025:
		// beta event:
		case 80020:
			return true;
	}
	return false;
}

#ifdef ENABLE_NEW_PET_SYSTEM
bool CItem::IsCostumePetItem()
{
	return GetType() == ITEM_COSTUME && GetSubType() == COSTUME_PET;
}
bool CItem::IsNewPetItem()
{
	return GetType() == ITEM_PET && GetSubType() == PET_LEVELABLE;
}

bool CItem::IsSkinPet()
{
	return GetType() == ITEM_COSTUME && GetSubType() == COSTUME_NORMAL_PET_SKIN;
}
bool CItem::IsSkinMount()
{
	return GetType() == ITEM_COSTUME && GetSubType() == COSTUME_MOUNT_SKIN;
}

#endif

#ifdef ENABLE_SPECIAL_COSTUME_ATTR
bool CItem::CanBonusCostume()
{
	if (GetType() != ITEM_COSTUME)
		return false;

	switch (GetSubType())
	{
		case COSTUME_BODY:
		case COSTUME_HAIR:
		case COSTUME_WEAPON:
			return true;
	}
	return false;
}
#endif

#ifdef __RENEWAL_CRYSTAL__
void CItem::SetCrystalBonus()
{
	if (!IsCrystal())
		return;
	const BYTE LEVEL_RANGE = 5;
	ClearAttribute();
	const BYTE refineLevel = GetVnum() - 51010;
	const std::vector<std::pair<BYTE, int>> m_AffectList = {
		{APPLY_ATTBONUS_MONSTER, 10},
		{APPLY_MAX_HP, 2000},
		{APPLY_ATTBONUS_HUMAN, 10},
		{APPLY_ATTBONUS_STONE, 10},
		{APPLY_ATTBONUS_ANIMAL, 10},
	};
	for (int i = 0; i < m_AffectList.size(); ++i)
	{
		const int bonusStartLevel = 1 + (i * LEVEL_RANGE);
		if (refineLevel >= bonusStartLevel)
		{
			const int bonusLevel = refineLevel >= bonusStartLevel + (LEVEL_RANGE-1) ? LEVEL_RANGE : LEVEL_RANGE - ((bonusStartLevel + (LEVEL_RANGE-1)) - refineLevel);
			SetAttribute(i, m_AffectList[i].first, bonusLevel * (m_AffectList[i].second/LEVEL_RANGE));
		}
	}
	SetSocket(1, 60 * 60 * 12);// start time!
}
bool CItem::IsCrystal()
{
	return (GetVnum() >= 51010 && GetVnum() <= 51035) ? true : false;
}
#endif

#ifdef __SKIN_SYSTEM__
bool CItem::IsAcce()
{
	return GetType() == ITEM_COSTUME && GetSubType() == COSTUME_ACCE;
}
bool CItem::IsAcceSkin()
{
	return GetType() == ITEM_COSTUME && GetSubType() == COSTUME_ACCE_SKIN;
}
#endif

#if defined(__SPECIAL_INVENTORY_SYSTEM__)
bool CItem::IsSkillBook()
{

	switch (GetVnum())
	{
		case 951040:
		case 951042:
		case 951043:
		case 951044:
		case 951045:
		case 55010:
		case 55011:
		case 55012:
		case 55013:
		case 55014:
		case 55015:
		case 55016:
		case 55017:
		case 55018:
		case 55019:
		case 55020:
		case 55021:
		case 55022:
		case 55023:
		case 55024:
		case 55025:
		case 55026:
		case 55009:
		case 71001:
		case 71094:
		case 50033:
		case 50513:
		case 952005:
		case 50301:
		case 50302:
		case 50303:
		case 50314:
		case 50315:
		case 50316:
		case 951050:
		case 951051:
		case 951052:
		case 951053:
		case 951054:
		case 951055:
		case 951056:
		case 951057:
		case 951058:
		case 70102:
			return true;
	}

	return GetType() == ITEM_SKILLBOOK;
}

bool CItem::IsUpgradeItem()
{
	if (GetType() == ITEM_MATERIAL && GetSubType() == MATERIAL_LEATHER)
		return true;

	switch (GetVnum())
	{
		case 351201:
		case 351202:
		case 351200:
			return false;
		case 951016:
		case 71025:
		case 70035:
		case 27987:
		case 27990:
		case 951021:
		case 951022:
		case 951023:
		case 951030:
		case 951031:
		case 951032:
		case 951033:
		case 951034:
		case 951035:
		case 951036:
		case 951037:
		case 30270:
		case 50252:
		case 27992:
		case 27993:
		case 27994:
		case 80019:
			return true;
	}

	return false;
}

bool CItem::IsStone()
{
	if (GetVnum() == 27999)
		return true;

	return GetType() == ITEM_METIN;
}

bool CItem::IsCostume()
{
	switch (GetSubType())
	{
		case COSTUME_MOUNT:
		case COSTUME_PET:
		case COSTUME_ACCE:
			return false;
	}

	switch (GetVnum())
	{
		case 41726:
		case 41728:
		case 45412:
		case 45414:
		case 40143:
		case 40146:
		case 40144:
		case 40145:
		case 40147:
		case 40148:
			return false;
	}
	return GetType() == ITEM_COSTUME;
}

bool CItem::IsSwitch()
{
	if (GetVnum() == 410010 || GetVnum() == 67011)
		return true;
	if (GetType() == ITEM_USE && ( GetSubType() == USE_CHANGE_ATTRIBUTE ||
			GetSubType() == USE_CHANGE_ATTRIBUTE2 ||
			GetSubType() == USE_ADD_ATTRIBUTE ||
			GetSubType() == USE_ADD_ATTRIBUTE2))
		return true;

	return false;
}

bool CItem::IsGiftBox()
{
	switch (GetVnum())
	{
		case 951040:
		case 951042:
		case 951043:
		case 951044:
		case 951045:
		case 410010:
		case 55009:
		case 50033:
			return false;
	}

#if defined(__GACHA_SYSTEM__)
	return GetType() == ITEM_GIFTBOX || GetType() == ITEM_GACHA;
#else
	return GetType() == ITEM_GIFTBOX;
#endif
}
#endif

//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
