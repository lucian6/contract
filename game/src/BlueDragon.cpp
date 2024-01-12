#include "stdafx.h"
#include "config.h"

#include "BlueDragon.h"

#include "vector.h"
#include "utils.h"
#include "char.h"
#include "mob_manager.h"
#include "sectree_manager.h"
#include "battle.h"
#include "affect.h"
#include "BlueDragon_Binder.h"
#include "BlueDragon_Skill.h"
#include "packet.h"
#include "motion.h"

time_t UseBlueDragonSkill(LPCHARACTER pChar, unsigned int idx)
{
	LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap( pChar->GetMapIndex() );

	if (NULL == pSecMap)
		return 0;

	int nextUsingTime = 0;

	switch (idx)
	{
		case 0:
			{
				sys_log(0, "BlueDragon: Using Skill Breath");

				FSkillBreath f(pChar);

				pSecMap->for_each( f );

				nextUsingTime = number(BlueDragon_GetSkillFactor(3, "Skill0", "period", "min"), BlueDragon_GetSkillFactor(3, "Skill0", "period", "max"));
			}
			break;

		case 1:
			{
				sys_log(0, "BlueDragon: Using Skill Weak Breath");

				FSkillWeakBreath f(pChar);

				pSecMap->for_each( f );

				nextUsingTime = number(BlueDragon_GetSkillFactor(3, "Skill1", "period", "min"), BlueDragon_GetSkillFactor(3, "Skill1", "period", "max"));
			}
			break;

		case 2:
			{
				sys_log(0, "BlueDragon: Using Skill EarthQuake");

				FSkillEarthQuake f(pChar);

				pSecMap->for_each( f );

				nextUsingTime = number(BlueDragon_GetSkillFactor(3, "Skill2", "period", "min"), BlueDragon_GetSkillFactor(3, "Skill2", "period", "max"));

				if (NULL != f.pFarthestChar)
				{
					pChar->BeginFight( f.pFarthestChar );
				}
			}
			break;

		default:
			sys_err("BlueDragon: Wrong Skill Index: %d", idx);
			return 0;
	}

	int addPct = BlueDragon_GetRangeFactor("hp_period", pChar->GetHPPct());

	nextUsingTime += (nextUsingTime * addPct) / 100;

	return nextUsingTime;
}

int BlueDragon_StateBattle(LPCHARACTER pChar)
{
	if (pChar->GetHPPct() > 98)
		return PASSES_PER_SEC(1);

	const int SkillCount = 3;
	int SkillPriority[SkillCount];
	static time_t timeSkillCanUseTime[SkillCount];

	if (pChar->GetHPPct() > 76)
	{
		SkillPriority[0] = 1;
		SkillPriority[1] = 0;
		SkillPriority[2] = 2;
	}
	else if (pChar->GetHPPct() > 31)
	{
		SkillPriority[0] = 0;
		SkillPriority[1] = 1;
		SkillPriority[2] = 2;
	}
	else
	{
		SkillPriority[0] = 0;
		SkillPriority[1] = 2;
		SkillPriority[2] = 1;
	}

	time_t timeNow = static_cast<time_t>(get_dword_time());

	for (int i=0 ; i < SkillCount ; ++i)
	{
		const int SkillIndex = SkillPriority[i];

		if (timeSkillCanUseTime[SkillIndex] < timeNow)
		{
			int SkillUsingDuration =
				static_cast<int>(CMotionManager::instance().GetMotionDuration( pChar->GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, MOTION_SPECIAL_1 + SkillIndex) ));

			timeSkillCanUseTime[SkillIndex] = timeNow + (UseBlueDragonSkill( pChar, SkillIndex ) * 1000) + SkillUsingDuration + 3000;

			pChar->SendMovePacket(FUNC_MOB_SKILL, SkillIndex, pChar->GetX(), pChar->GetY(), 0, timeNow);

			return 0 == SkillUsingDuration ? PASSES_PER_SEC(1) : PASSES_PER_SEC(SkillUsingDuration);
		}
	}

	return PASSES_PER_SEC(1);
}

int BlueDragon_Damage(LPCHARACTER me, LPCHARACTER pAttacker, int dam)
{
	return dam;
}
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
