#include "stdafx.h"

#ifdef __BUFFI_SUPPORT__
#include "buffi.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "skill.h"

//Some Config
constexpr bool SAVE_WITH_ITEM_ID = false; // IF FALSE WILL SAVE WITH OWNER PID!
constexpr int MAX_BUFFI_COUNT = 1;

BUFFI::BUFFI(DWORD playerID, DWORD itemID) : szName(""), m_dwOwnerPID(playerID), m_dwVID(0), m_dwItemPID(itemID), m_dwSkillNextTime(0){}
BUFFI::~BUFFI(){}

LPITEM BUFFI::GetItem()
{
	LPITEM item = ITEM_MANAGER::Instance().Find(m_dwItemPID);
	return item ? item : NULL;
}
LPCHARACTER BUFFI::GetOwner()
{
	LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(m_dwOwnerPID);
	return ch ? ch : NULL;
}
LPCHARACTER BUFFI::GetBuffi()
{
	LPCHARACTER ch = CHARACTER_MANAGER::Instance().Find(m_dwVID);
	return ch ? ch : NULL;
}
void BUFFI::LoadSpecialName()
{
	szName = "";

	char filename[124];
	snprintf(filename, sizeof(filename), "data/buffi_name/%u", SAVE_WITH_ITEM_ID ? m_dwItemPID : m_dwOwnerPID);
	FILE* fp;
	if ((fp = fopen(filename, "r")) != NULL)
	{
		char	one_line[256];
		while (fgets(one_line, 256, fp))
		{
			if (strlen(one_line) > 0)
			{
				szName = one_line;
				break;
			}
		}
		fclose(fp);
	}
}

void BUFFI::SetBuffiName(const char* szNewName)
{
	szName = szNewName;

	char filename[124];
	snprintf(filename, sizeof(filename), "data/buffi_name/%u", SAVE_WITH_ITEM_ID ? m_dwItemPID : m_dwOwnerPID);
	FILE* fp = fopen(filename, "w+");
	if (fp)
	{
		fprintf(fp, "%s", szNewName);
		fclose(fp);
	}
	Summon(true, false);
}

void BUFFI::CheckSupportWear(bool bIsCreating)
{
	LPCHARACTER owner = GetOwner();
	if (!owner)
		return;

	LPCHARACTER buffi = GetBuffi();
	if (!buffi)
		return;

	buffi->SetPart(PART_WEAPON, owner->GetWear(WEAR_BUFFI_WEAPON) ? owner->GetWear(WEAR_BUFFI_WEAPON)->GetVnum() : 0);
	buffi->SetPart(PART_ACCE, owner->GetWear(WEAR_BUFFI_SASH) ? owner->GetWear(WEAR_BUFFI_SASH)->GetVnum() : 0);
	buffi->SetPart(PART_HAIR, owner->GetWear(WEAR_BUFFI_HEAD) ? owner->GetWear(WEAR_BUFFI_HEAD)->GetValue(3) : 0);
	buffi->SetPart(PART_MAIN, owner->GetWear(WEAR_BUFFI_BODY) ? owner->GetWear(WEAR_BUFFI_BODY)->GetVnum() : 41463);
	
	if (!bIsCreating)
		buffi->UpdatePacket();
}
bool BUFFI::Summon(bool bSummonType, bool bFromClick)
{
	if (bSummonType)
	{
		LPCHARACTER owner = GetOwner();
		if (!owner)
			return false;

		if (owner->IsObserverMode())
			return false;

		LPITEM item = GetItem();
		if (!item)
			return false;
		LPCHARACTER buffi = GetBuffi();
		if (buffi)
		{
			M2_DESTROY_CHARACTER(buffi);
			buffi = NULL;
			m_dwVID = 0;
		}

		LoadSpecialName();

		const long x = owner->GetX() + number(-100, 100), y = owner->GetY() + number(-100, 100), z = owner->GetZ();
		buffi = CHARACTER_MANAGER::instance().SpawnMob(65000, owner->GetMapIndex(), x, y, z, false, (int)(owner->GetRotation() + 180), false);
		if (!buffi)
		{
			sys_err("Buffi can't spawn, something went wrong please check buffi item value0 in item proto..");
			return false;
		}
		m_dwVID = buffi->GetVID();
		item->SetSocket(1, 1);
		buffi->SetOwner(owner);
		buffi->SetBuffi(true);
		buffi->SetEmpire(owner->GetEmpire());

		std::string name(szName != "" ? szName.c_str() : owner->GetName());
		if (szName == "")
			name += "'s Buffi";

		buffi->SetName(name.c_str());
		CheckSupportWear(true);

		if (!buffi->Show(owner->GetMapIndex(), x, y, z))
		{
			sys_err("Buffi can't show, something went wrong please check buffi summon code!");
			return false;
		}
		CheckOwnerSupport();
	}
	else
	{
		LPCHARACTER buffi = GetBuffi();
		if (buffi)
		{
			buffi->SetOwner(NULL);
			M2_DESTROY_CHARACTER(buffi);
		}

		if (bFromClick)
		{
			LPITEM item = GetItem();
			if (item)
				item->SetSocket(1, 0);
		}

		m_dwVID = 0;
	}
	return true;
}
void BUFFI::CheckOwnerSupport()
{
	const DWORD now = get_dword_time();

	if (m_dwSkillNextTime > now) { 
		return;
	}

	m_dwSkillNextTime = now + 3000;

	LPCHARACTER owner = GetOwner();

	if (!owner) {
		return;
	}

	if (!owner->IsLoadedAffect()) {
		return;
	}

	LPCHARACTER buffi = GetBuffi();

	if (!buffi){
		return;
	}

	if (owner->GetSkillGroup() == 0) {
		return;
	}

	if (owner->IsDead())
	{
		return;
	}

	constexpr DWORD skilList[][4] = { 
		{164, SKILL_HOSIN, AFF_BUFFI_HOSIN, AFF_HOSIN},
		{165, SKILL_GICHEON, AFF_BUFFI_GICHEON, AFF_GICHEON},
		{166, SKILL_KWAESOK, AFF_BUFFI_KWAESOK, AFF_KWAESOK},
		{167, SKILL_JEUNGRYEOK, AFF_BUFFI_JEUNGRYEOK,AFF_JEUNGRYEOK},
	};
	
	for (DWORD j = 0; j < 4; ++j)
	{
		const DWORD skillIdx = skilList[j][0];
		
		if (szName == "" && (skillIdx == 166 || skillIdx == 167))
			continue;

		if (owner->GetSkillLevel(skillIdx) == 0) {
			owner->SetSkillLevel(skillIdx, 20);
			owner->SkillLevelPacket();
			return;
		}

		if (owner->IsAffectFlag(skilList[j][2]) || owner->IsAffectFlag(skilList[j][3]) || !owner->GetSkillLevel(skillIdx))
			continue;

		buffi->ComputeSkill(skillIdx, owner, owner->GetSkillLevel(skillIdx));
		buffi->SendBuffiSkillAffect(owner->GetVID(), skilList[j][1], owner->GetSkillLevel(skillIdx));
		return;
	}
}

BUFFI_MANAGER::BUFFI_MANAGER(){}
BUFFI_MANAGER::~BUFFI_MANAGER() {}
BUFFI* BUFFI_MANAGER::GetBuffi(DWORD dwOwnerID, DWORD dwVID)
{
	auto it = m_mapBuffiList.find(dwOwnerID);
	if (it != m_mapBuffiList.end())
	{
		for (auto& [itemID, buffi] : it->second)
		{
			if (buffi.CheckVID(dwVID) || !dwVID)
				return &buffi;
		}
	}
	return NULL;
}

void BUFFI_MANAGER::CompareLoginStatus(bool isLogin, LPCHARACTER ch)
{
	if (isLogin)
	{
		for (WORD i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = ch->GetInventoryItem(i);
			
			if (!item)
				continue;

			if (item->GetType() == ITEM_BUFFI && item->GetSubType() == BUFFI_SCROLL)
			{
				if (item->GetSocket(1) == 1)
				{
					Summon(true, item);
					return;
				}
			}
		}
	}
	else
	{
		auto it = m_mapBuffiList.find(ch->GetPlayerID());
		if (it != m_mapBuffiList.end())
		{
			for (auto& [itemID, buffi] : it->second)
				buffi.Summon(false, false);
			m_mapBuffiList.erase(ch->GetPlayerID());
		}
	}
}
int BUFFI_MANAGER::SummonCount(LPCHARACTER ch)
{
	int count = 0;
	auto it = m_mapBuffiList.find(ch->GetPlayerID());
	if (it != m_mapBuffiList.end())
	{
		for (auto& [itemID, buffi] : it->second)
			count += 1;
	}
	return count;
}

void BUFFI_MANAGER::Summon(bool bSummonType, LPITEM item)
{
	if (!item)
		return;
	LPCHARACTER owner = item->GetOwner();

	if (!owner)
		return;

	bool anti_war_buffi = false;

	if (owner->GetRealMapIndex() == 110 || owner->GetRealMapIndex() == 111)
	{
		owner->ChatPacket(CHAT_TYPE_INFO, "You can't summon Buffi in a War.");
		anti_war_buffi = true;
	}

	const DWORD ownerPID = owner->GetPlayerID();
	const DWORD itemID = item->GetID();

	if (bSummonType && owner->IsObserverMode() == false && anti_war_buffi == false)
	{
		auto it = m_mapBuffiList.find(ownerPID);
		if (it == m_mapBuffiList.end())
		{
			std::map<DWORD, BUFFI> m_map;
			m_mapBuffiList.emplace(ownerPID, m_map);
			it = m_mapBuffiList.find(ownerPID);
		}

		auto itBuffi = it->second.find(itemID);
		if (itBuffi == it->second.end())
		{
			if (it->second.size() >= MAX_BUFFI_COUNT)
			{
				owner->ChatPacket(CHAT_TYPE_INFO, "You already summoned buffi. Please deactive other and try again.");
				return;
			}
			BUFFI  newBuffi(ownerPID, itemID);
			if (newBuffi.Summon(true, true))
				it->second.emplace(itemID, newBuffi);
		}
		else
			itBuffi->second.Summon(true, true);
	}
	else
	{
		auto it = m_mapBuffiList.find(ownerPID);
		if (it != m_mapBuffiList.end())
		{
			auto itBuffi = it->second.find(itemID);
			if (itBuffi != it->second.end())
			{
				itBuffi->second.Summon(false, true);
				it->second.erase(itemID);
				return;
			}
		}
		if (item->GetSocket(1) == 1)
			item->SetSocket(1, 0);
	}
}
#endif



