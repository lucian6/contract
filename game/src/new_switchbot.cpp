#include "stdafx.h"

#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#include "desc.h"
#include "item.h"
#include "item_manager.h"
#include "char_manager.h"
#include "buffer_manager.h"
#include "char.h"
#include "config.h"
#include "p2p.h"

const float c_fSpeed = 0.18f;

bool ValidPosition(DWORD wCell)
{
	return wCell < SWITCHBOT_SLOT_COUNT_REAL;
}

bool SwitchbotHelper::IsValidItem(LPITEM pkItem)
{
	if (!pkItem)
		return false;
	switch (pkItem->GetType())
	{
	case ITEM_WEAPON:
		return true;

	case ITEM_ARMOR:
		switch (pkItem->GetSubType())
		{
		case ARMOR_BODY:
		case ARMOR_HEAD:
		case ARMOR_SHIELD:
		case ARMOR_WRIST:
		case ARMOR_FOOTS:
		case ARMOR_NECK:
		case ARMOR_EAR:
		case ARMOR_PENDANT_SOUL:
			return true;
		}
	case ITEM_COSTUME:
		switch (pkItem->GetSubType())
		{
		case COSTUME_ACCE_SKIN:
			return true;
		}
	default:
		return false;
	}
}

CSwitchbot::CSwitchbot()
{
	m_pkSwitchEvent = nullptr;
	m_table = {};
	m_isWarping = false;
}

CSwitchbot::~CSwitchbot()
{
	if (m_pkSwitchEvent)
		event_cancel(&m_pkSwitchEvent);

	m_pkSwitchEvent = nullptr;
	m_table = {};
	m_isWarping = false;
}

void CSwitchbot::RegisterItem(WORD wCell, DWORD item_id)
{
	if (!ValidPosition(wCell))
		return;
	m_table.items[wCell] = item_id;
}

void CSwitchbot::UnregisterItem(WORD wCell)
{
	if (!ValidPosition(wCell))
		return;
	m_table.items[wCell] = 0;
	m_table.active[wCell] = false;
	m_table.finished[wCell] = false;
	memset(&m_table.alternatives[wCell], 0, sizeof(m_table.alternatives[wCell]));
}

void CSwitchbot::SetAttributes(BYTE slot, const std::vector<TSwitchbotAttributeAlternativeTable> vec_alternatives)
{
	if (!ValidPosition(slot))
		return;

	for (BYTE alternative = 0; alternative < SWITCHBOT_ALTERNATIVE_COUNT; ++alternative)
	{
		for (BYTE attrIdx = 0; attrIdx < MAX_NORM_ATTR_NUM; ++attrIdx)
		{
			m_table.alternatives[slot][alternative].attributes[attrIdx].bType = vec_alternatives[alternative].attributes[attrIdx].bType;
			m_table.alternatives[slot][alternative].attributes[attrIdx].sValue = vec_alternatives[alternative].attributes[attrIdx].sValue;
		}
	}
}

void CSwitchbot::SetActive(BYTE slot, bool active)
{
	if (!ValidPosition(slot))
		return;
	m_table.active[slot] = active;
	m_table.finished[slot] = false;
	LPITEM pkItem = ITEM_MANAGER::Instance().Find(m_table.items[slot]);
	if (pkItem)
		pkItem->SetUpdateStatus(active);

	LPCHARACTER owner = pkItem->GetOwner();
}

EVENTINFO(TSwitchbotEventInfo)
{
	CSwitchbot* pkSwitchbot;
	TSwitchbotEventInfo() : pkSwitchbot(nullptr)
	{}
};

EVENTFUNC(switchbot_event)
{
	TSwitchbotEventInfo* info = dynamic_cast<TSwitchbotEventInfo*>(event->info);
	if (info == NULL)
	{
		sys_err("switchbot_event> <Factor> Info Null pointer");
		return 0;
	}
	if (!info->pkSwitchbot)
	{
		sys_err("switchbot_event> <Factor> Switchbot Null pointer");
		return 0;
	}
	info->pkSwitchbot->SwitchItems();

	return PASSES_PER_SEC(c_fSpeed);
}

void CSwitchbot::Start()
{
	TSwitchbotEventInfo* info = AllocEventInfo<TSwitchbotEventInfo>();
	info->pkSwitchbot = this;

	m_pkSwitchEvent = event_create(switchbot_event, info, c_fSpeed);

	CSwitchbotManager::Instance().SendSwitchbotUpdate(m_table.player_id);
}

void CSwitchbot::Stop()
{
	if (m_pkSwitchEvent)
	{
		event_cancel(&m_pkSwitchEvent);
		m_pkSwitchEvent = nullptr;
	}

	memset(&m_table.active, 0, sizeof(m_table.active));

	CSwitchbotManager::Instance().SendSwitchbotUpdate(m_table.player_id);
	for (BYTE bSlot = 0; bSlot < SWITCHBOT_SLOT_COUNT; ++bSlot)
	{
		LPITEM pkItem = ITEM_MANAGER::Instance().Find(m_table.items[bSlot]);
		if (!pkItem)
			continue;

		pkItem->SetUpdateStatus(false);
		LPCHARACTER pkOwner = pkItem->GetOwner();
		if (!pkOwner)
			return;
		
		SendItemUpdate(pkOwner, bSlot, pkItem);
	}
}

void CSwitchbot::SendSingleUpdate(BYTE slot)
{
	LPITEM pkItem = ITEM_MANAGER::Instance().Find(m_table.items[slot]);
	if (!pkItem)
		return;

	LPCHARACTER pkOwner = pkItem->GetOwner();
	if (!pkOwner)
		return;

	SendItemUpdate(pkOwner, slot, pkItem);
	CSwitchbotManager::Instance().SendSwitchbotUpdate(m_table.player_id);
}

void CSwitchbot::Pause()
{
	if (m_pkSwitchEvent)
	{
		event_cancel(&m_pkSwitchEvent);
		m_pkSwitchEvent = nullptr;
	}
	for (BYTE bSlot = 0; bSlot < SWITCHBOT_SLOT_COUNT; ++bSlot)
	{
		LPITEM pkItem = ITEM_MANAGER::Instance().Find(m_table.items[bSlot]);
		if (!pkItem)
			continue;
		LPCHARACTER pkOwner = pkItem->GetOwner();
		if (!pkOwner)
			return;
		SendItemUpdate(pkOwner, bSlot, pkItem);
	}
}

bool CSwitchbot::IsActive(BYTE slot)
{
	if (!ValidPosition(slot))
		return false;
	return m_table.active[slot];
}

bool CSwitchbot::HasActiveSlots()
{
	for (const auto& it: m_table.active)
	{
		if (it)
			return true;
	}
	return false;
}

bool CSwitchbot::IsSwitching()
{
	return m_pkSwitchEvent != nullptr;
}

void CSwitchbot::SwitchItems()
{
	for (BYTE bSlot = 0; bSlot < SWITCHBOT_SLOT_COUNT_REAL; ++bSlot)
	{
		if (!m_table.active[bSlot])
			continue;
		m_table.finished[bSlot] = false;
		const DWORD item_id = m_table.items[bSlot];
		LPITEM pkItem = ITEM_MANAGER::Instance().Find(item_id);
		if (!pkItem)
			continue;
		LPCHARACTER pkOwner = pkItem->GetOwner();
		if (!pkOwner)
			return;

		if (IsWarping())
			continue;

		if (pkOwner->PreventTradeWindow(WND_EXCHANGE, false/*except*/))
			continue;

		if (CheckItem(pkItem, bSlot))
		{
			LPDESC desc = pkOwner->GetDesc();
			if (desc)
			{
				char buf[255];
				const int iLen =snprintf(buf, sizeof(buf), LC_STRING("864 %s %d", pkOwner->GetLanguage()), pkItem->GetLocaleName(pkOwner->GetLanguage()), bSlot + 1);

				TPacketGCWhisper pack;
				pack.bHeader = HEADER_GC_WHISPER;
				pack.bType = WHISPER_TYPE_GM;
				pack.wSize = sizeof(TPacketGCWhisper) + iLen;
				strlcpy(pack.szNameFrom, "[SYSTEM]", sizeof(pack.szNameFrom));
				pkOwner->GetDesc()->BufferedPacket(&pack, sizeof(pack));
				pkOwner->GetDesc()->Packet(buf, iLen);
			}

			SetActive(bSlot, false);
			m_table.finished[bSlot] = true;
			if (!HasActiveSlots())
				Stop();
			else
				CSwitchbotManager::Instance().SendSwitchbotUpdate(m_table.player_id);

			SendItemUpdate(pkOwner, bSlot, pkItem);
		}
		else
		{
			bool stop = true;
			DWORD itemVnumSelected = 0;
			if (SWITCHBOT_PRICE_TYPE == 1)
			{
				for (const auto& itemVnum : c_arSwitchingItems)
				{
					if (itemVnum == 67011)
					{
						if (pkItem->IsCostumeSashSkin())
						{
							bool bCanUse = true;
							if (false == bCanUse)
								continue;
							if (pkOwner->CountSpecifyItem(itemVnum) >= SWITCHBOT_PRICE_AMOUNT)
							{
								itemVnumSelected = itemVnum;
								stop = false;
							}
						}
					}
					else if (itemVnum == 350006)
					{
						if (pkItem->IsPendantSoul())
						{
							bool bCanUse = true;
							if (false == bCanUse)
								continue;
							if (pkOwner->CountSpecifyItem(itemVnum) >= SWITCHBOT_PRICE_AMOUNT)
							{
								itemVnumSelected = itemVnum;
								stop = false;
							}
						}
					}
					//CHECK_LIMITED_ITEM START
					else if (itemVnum == 951002)
					{
						if (pkItem->IsZodiacItem())
						{
							bool bCanUse = true;
							if (false == bCanUse)
								continue;
							if (pkOwner->CountSpecifyItem(itemVnum) >= SWITCHBOT_PRICE_AMOUNT)
							{
								itemVnumSelected = itemVnum;
								stop = false;
							}
						}
					}
					else if (itemVnum == 71151 || itemVnum == 76023)
					{
						bool bCanUse = true;
						for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
						{
							if (pkItem->GetLimitType(i) == LIMIT_LEVEL && pkItem->GetLimitValue(i) > 40)
							{
								bCanUse = false;
								break;
							}
						}
						if (false == bCanUse)
							continue;
						if (pkOwner->CountSpecifyItem(itemVnum) >= SWITCHBOT_PRICE_AMOUNT)
						{
							itemVnumSelected = itemVnum;
							stop = false;
						}
					}
					else if (itemVnum == 71084)
					{
						if (false == pkItem->IsZodiacItem() && false == pkItem->IsPendantSoul() && false == pkItem->IsCostumeSashSkin())
						{
							bool bCanUse = true;
							if (false == bCanUse)
								continue;
							if (pkOwner->CountSpecifyItem(itemVnum) >= SWITCHBOT_PRICE_AMOUNT)
							{
								itemVnumSelected = itemVnum;
								stop = false;
							}
						}
					}
					else
					{
						if (pkOwner->CountSpecifyItem(itemVnum) >= SWITCHBOT_PRICE_AMOUNT)
						{
							itemVnumSelected = itemVnum;
							stop = false;
						}
					}
					//CHECK_LIMITED_ITEM END
				}
			}
			else if (SWITCHBOT_PRICE_TYPE == 2)
			{
				if (pkOwner->GetGold() >= SWITCHBOT_PRICE_AMOUNT)
					stop = false;
			}

			if (stop)
			{
				if (false == pkItem->IsZodiacItem() && false == pkItem->IsPendantSoul() && false == pkItem->IsCostumeSashSkin())
				{
					for (const auto& itemVnum : c_arSwitchingItems)
					{
						if (itemVnum == 71084)
						{
							std::vector<DWORD> m_BonusChestIdxList = {410010};
							for (WORD i= SWITCH_INVENTORY_SLOT_START; i < SWITCH_INVENTORY_SLOT_END;++i)
							{
								LPITEM item = pkOwner->GetInventoryItem(i);
								if(item)
								{
									if(std::find(m_BonusChestIdxList.begin(), m_BonusChestIdxList.end(), item->GetVnum()) != m_BonusChestIdxList.end())
									{
										pkOwner->UseItemEx(item);
										return;
									}
								}
							}
						}
					}
				}

				SetActive(bSlot, false);
				m_table.finished[bSlot] = false;

				if (!HasActiveSlots())
				{
					Stop();
				}
				else
				{
					SendItemUpdate(pkOwner, bSlot, pkItem);
					CSwitchbotManager::Instance().SendSwitchbotUpdate(m_table.player_id);
				}

				if (SWITCHBOT_PRICE_TYPE == 1)
					pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Switchbot stopped. Out of switchers."));
				else
					pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Switchbot stopped. Not enough money."));

				return;
			}

			if (SWITCHBOT_PRICE_TYPE == 1)
			{
				for (const auto& itemVnum : c_arSwitchingItems)
				{
					if (itemVnumSelected != itemVnum)
						continue;

					LPITEM pkItem = pkOwner->FindSpecifyItem(itemVnum);
					if (pkItem)
					{
						pkItem->SetCount(pkItem->GetCount() - SWITCHBOT_PRICE_AMOUNT);
						break;
					}
				}
				if (pkItem->IsCostumeSashSkin())
					pkItem->ChangeDreamSoulBonus();
				else
					pkItem->ChangeAttribute();
			}
		}
	}
}

bool CSwitchbot::CheckItem(LPITEM pkItem, BYTE slot)
{
	if (!ValidPosition(slot) || !pkItem)
		return false;

	bool checked = false;

	for (const auto& alternative : m_table.alternatives[slot])
	{
		if (!alternative.IsConfigured())
			continue;
		BYTE configuredAttrCount = 0;
		BYTE correctAttrCount = 0;

		for (const auto& destAttr : alternative.attributes)
		{
			if (!destAttr.bType || !destAttr.sValue)
				continue;
			++configuredAttrCount;
			for (BYTE attrIdx = 0; attrIdx < MAX_NORM_ATTR_NUM; ++attrIdx)
			{
				const TPlayerItemAttribute& curAttr = pkItem->GetAttribute(attrIdx);
				if (curAttr.bType != destAttr.bType || curAttr.sValue < destAttr.sValue)
					continue;
				++correctAttrCount;
				break;
			}
		}
		checked = true;
		if (configuredAttrCount == correctAttrCount)
			return true;
	}
	if (!checked)
		return true;
	return false;
}

void CSwitchbot::SendItemUpdate(LPCHARACTER ch, BYTE slot, LPITEM item)
{
	LPDESC desc = ch->GetDesc();
	if (!desc)
		return;

	if (IsWarping())
		return;

	TPacketGCSwitchbot pack;
	pack.header = HEADER_GC_SWITCHBOT;
	pack.subheader = SUBHEADER_GC_SWITCHBOT_UPDATE_ITEM;
	pack.size = sizeof(TPacketGCSwitchbot) + sizeof(TSwitchbotUpdateItem);

	TSwitchbotUpdateItem update = {};
	update.slot = slot;
	update.vnum = item->GetVnum();
	update.count = item->GetCount();
	thecore_memcpy(update.alSockets, item->GetSockets(), sizeof(update.alSockets));
	thecore_memcpy(update.aAttr, item->GetAttributes(), sizeof(update.aAttr));

	TEMP_BUFFER tmp_buf;
	tmp_buf.write(&pack, sizeof(pack));
	tmp_buf.write(&update, sizeof(update));
	desc->Packet(tmp_buf.read_peek(), tmp_buf.size());

}

void CSwitchbotManager::RegisterItem(DWORD player_id, DWORD item_id, WORD wCell)
{
	if (!ValidPosition(wCell))
		return;
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
	{
		pkSwitchbot = new CSwitchbot();
		pkSwitchbot->SetPlayerId(player_id);
		m_map_Switchbots.insert(std::make_pair(player_id, pkSwitchbot));
	}
	if (pkSwitchbot->IsWarping())
		return;
	pkSwitchbot->RegisterItem(wCell, item_id);
	SendSwitchbotUpdate(player_id);
}

void CSwitchbotManager::UnregisterItem(DWORD player_id, WORD wCell)
{
	if (!ValidPosition(wCell))
		return;
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
		return;
	if (pkSwitchbot->IsWarping())
		return;
	pkSwitchbot->UnregisterItem(wCell);
	SendSwitchbotUpdate(player_id);
}

void CSwitchbotManager::Start(DWORD player_id, BYTE slot, const std::vector<TSwitchbotAttributeAlternativeTable> vec_alternatives)
{
	if (!ValidPosition(slot))
		return;

	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
	{
		sys_err("No Switchbot found for player_id %d slot %d", player_id, slot);
		return;
	}

	if (pkSwitchbot->IsActive(slot))
	{
		sys_err("Switchbot slot %d already running for player_id %d", slot, player_id);
		return;
	}

	pkSwitchbot->SetActive(slot, true);
	pkSwitchbot->SetAttributes(slot, vec_alternatives);
	if (pkSwitchbot->HasActiveSlots() && !pkSwitchbot->IsSwitching())
		pkSwitchbot->Start();
	else
		SendSwitchbotUpdate(player_id);
}

void CSwitchbotManager::Stop(DWORD player_id, BYTE slot)
{
	if (!ValidPosition(slot))
		return;
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
	{
		sys_err("No Switchbot found for player_id %d slot %d", player_id, slot);
		return;
	}

	if (!pkSwitchbot->IsActive(slot))
	{
		sys_err("Switchbot slot %d is not running for player_id %d", slot, player_id);
		return;
	}
	pkSwitchbot->SetActive(slot, false);
	if (!pkSwitchbot->HasActiveSlots() && pkSwitchbot->IsSwitching())
		pkSwitchbot->Stop();
	else
		pkSwitchbot->SendSingleUpdate(slot);
}

bool CSwitchbotManager::IsActive(DWORD player_id, BYTE slot)
{
	if (!ValidPosition(slot))
		return false;
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
		return false;
	return pkSwitchbot->IsActive(slot);
}

bool CSwitchbotManager::IsWarping(DWORD player_id)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
		return false;
	return pkSwitchbot->IsWarping();
}

void CSwitchbotManager::SetIsWarping(DWORD player_id, bool warping)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
		return;
	pkSwitchbot->SetIsWarping(warping);
}

CSwitchbot* CSwitchbotManager::FindSwitchbot(DWORD player_id)
{
	const auto& it = m_map_Switchbots.find(player_id);
	if (it == m_map_Switchbots.end())
		return nullptr;

	return it->second;
}

void CSwitchbotManager::P2PSendSwitchbot(DWORD player_id, WORD wTargetPort)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
		return;
	pkSwitchbot->Pause();
	m_map_Switchbots.erase(player_id);
	TPacketGGSwitchbot pack;
	pack.wPort = wTargetPort;
	pack.table = pkSwitchbot->GetTable();
	P2P_MANAGER::Instance().Send(&pack, sizeof(pack));
}

void CSwitchbotManager::P2PReceiveSwitchbot(TSwitchbotTable table)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(table.player_id);
	if (!pkSwitchbot)
	{
		pkSwitchbot = new CSwitchbot();
		m_map_Switchbots.insert(std::make_pair(table.player_id, pkSwitchbot));
	}

	pkSwitchbot->SetTable(table);
}

void CSwitchbotManager::SendItemAttributeInformations(LPCHARACTER ch)
{
	if (!ch)
		return;
	LPDESC desc = ch->GetDesc();
	if (!desc)
		return;
	TPacketGCSwitchbot pack;
	pack.header = HEADER_GC_SWITCHBOT;
	pack.subheader = SUBHEADER_GC_SWITCHBOT_SEND_ATTRIBUTE_INFORMATION;
	pack.size = sizeof(TPacketGCSwitchbot);
	TEMP_BUFFER buf;
	for (BYTE bAttributeSet = 0; bAttributeSet < ATTRIBUTE_SET_MAX_NUM; ++bAttributeSet)
	{
		TSwitchbottAttributeTable table = {};
		table.attribute_set = bAttributeSet;
		if (bAttributeSet == ATTRIBUTE_SET_ACCE_SKIN)
		{
			table.apply_num = APPLY_ATTBONUS_MONSTER;
			table.max_value = 10;
			buf.write(&table, sizeof(table));
			table.apply_num = APPLY_ATTBONUS_BOSS;
			table.max_value = 10;
			buf.write(&table, sizeof(table));
			table.apply_num = APPLY_ATTBONUS_DUNGEON;
			table.max_value = 10;
			buf.write(&table, sizeof(table));
			
			table.apply_num = APPLY_ATTBONUS_STONE;
			table.max_value = 10;
			buf.write(&table, sizeof(table));
			
			table.apply_num = APPLY_NORMAL_HIT_DAMAGE_BONUS;
			table.max_value = 8;
			buf.write(&table, sizeof(table));
			
			table.apply_num = APPLY_MAX_HP;
			table.max_value = 2000;
			buf.write(&table, sizeof(table));
			
			table.apply_num = APPLY_ANTI_CRITICAL_PCT;
			table.max_value = 10;
			buf.write(&table, sizeof(table));
			
			table.apply_num = APPLY_SKILL_DEFEND_BONUS;
			table.max_value = 10;
			buf.write(&table, sizeof(table));
			
			table.apply_num = APPLY_ATTBONUS_HUMAN;
			table.max_value = 10;
			buf.write(&table, sizeof(table));
			
			table.apply_num = APPLY_RESIST_HUMAN;
			table.max_value = 10;
			buf.write(&table, sizeof(table));
		}
		else
		{
			for (int iApplyNum = 0; iApplyNum < MAX_APPLY_NUM; ++iApplyNum)
			{
				const TItemAttrTable& r = g_map_itemAttr[iApplyNum];
				const BYTE max = r.bMaxLevelBySet[bAttributeSet];
				if (max > 0)
				{
					table.apply_num = iApplyNum;
					table.max_value = r.lValues[max - 1];
					buf.write(&table, sizeof(table));
				}
			}
		}
	}
	if (buf.size())
	{
		pack.size += buf.size();
		desc->BufferedPacket(&pack, sizeof(pack));
		desc->Packet(buf.read_peek(), buf.size());
	}
	else
		desc->Packet(&pack, sizeof(pack));
}

void CSwitchbotManager::SendSwitchbotUpdate(DWORD player_id)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
		return;
	LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(player_id);
	if (!ch)
		return;
	LPDESC d = ch->GetDesc();
	if (!d)
		return;
	const TSwitchbotTable table = pkSwitchbot->GetTable();
	TPacketGCSwitchbot pack;
	pack.header = HEADER_GC_SWITCHBOT;
	pack.subheader = SUBHEADER_GC_SWITCHBOT_UPDATE;
	pack.size = sizeof(TPacketGCSwitchbot) + sizeof(TSwitchbotTable);
	d->BufferedPacket(&pack, sizeof(pack));
	d->Packet(&table, sizeof(table));
}

void CSwitchbotManager::EnterGame(LPCHARACTER ch)
{
	SendItemAttributeInformations(ch);
	SetIsWarping(ch->GetPlayerID(), false);
	SendSwitchbotUpdate(ch->GetPlayerID());
	CSwitchbot* pkSwitchbot = FindSwitchbot(ch->GetPlayerID());
	if (pkSwitchbot && pkSwitchbot->HasActiveSlots() && !pkSwitchbot->IsSwitching())
		pkSwitchbot->Start();
}

#endif
