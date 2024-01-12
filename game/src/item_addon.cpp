#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "item.h"
#include "item_addon.h"
#ifdef ENABLE_REWARD_SYSTEM
#include "char_manager.h"
#include "char.h"
#endif

CItemAddonManager::CItemAddonManager()
{
}

CItemAddonManager::~CItemAddonManager()
{
}

void CItemAddonManager::ApplyAddonTo(int iAddonType, LPITEM pItem)
{
	if (!pItem)
	{
		sys_err("ITEM pointer null");
		return;
	}

	int iSkillBonus = MINMAX(-30, (int) (gauss_random(0, 5) + 0.5f), 30);
	int iNormalHitBonus = 0;
	if (abs(iSkillBonus) <= 20)
		iNormalHitBonus = -2 * iSkillBonus + abs(number(-8, 8) + number(-8, 8)) + number(1, 4);
	else
		iNormalHitBonus = -2 * iSkillBonus + number(1, 5);

	pItem->RemoveAttributeType(APPLY_SKILL_DAMAGE_BONUS);
	pItem->RemoveAttributeType(APPLY_NORMAL_HIT_DAMAGE_BONUS);
	pItem->AddAttribute(APPLY_NORMAL_HIT_DAMAGE_BONUS, iNormalHitBonus);
	
#ifdef ENABLE_REWARD_SYSTEM
	if(iNormalHitBonus>=60 && pItem->GetLevelLimit() == 75)
	{
		LPCHARACTER ch = pItem->GetOwner();
		if(ch)
			CHARACTER_MANAGER::Instance().SetRewardData(REWARD_AVERAGE,ch->GetName(), true);
	}
#endif

	if(iNormalHitBonus>=50 && pItem->GetLevelLimit() >= 115)
	{
		LPCHARACTER ch = pItem->GetOwner();
		if(ch)
			CHARACTER_MANAGER::Instance().SetRewardData(FIRST_AVERAGE_ZODIAC_50,ch->GetName(), true);
	}

	pItem->AddAttribute(APPLY_SKILL_DAMAGE_BONUS, iSkillBonus);
}
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
