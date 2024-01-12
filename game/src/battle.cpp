#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "battle.h"
#include "item.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "vector.h"
#include "packet.h"
#include "pvp.h"
#include "profiler.h"
#include "guild.h"
#include "affect.h"
#include "unique_item.h"
#include "lua_incl.h"
#include "arena.h"
#include "castle.h"
#include "sectree.h"
#include "ani.h"
#include "locale_service.h"
#include "../../common/CommonDefines.h"
#ifdef ENABLE_ULTIMATE_REGEN
#include "new_mob_timer.h"
#endif
#include "log.h"
#if defined(__SHIP_DEFENSE__)
#	include "ShipDefense.h"
#endif

int battle_hit(LPCHARACTER ch, LPCHARACTER victim, int & iRetDam);

bool battle_distance_valid_by_xy(long x, long y, long tx, long ty)
{
	long distance = DISTANCE_APPROX(x - tx, y - ty);

	if (distance > 170)
		return false;

	return true;
}

bool battle_distance_valid(LPCHARACTER ch, LPCHARACTER victim)
{
	return battle_distance_valid_by_xy(ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY());
}

bool timed_event_cancel(LPCHARACTER ch)
{
	if (ch->m_pkTimedEvent)
	{
		event_cancel(&ch->m_pkTimedEvent);
		return true;
	}

	return false;
}

#ifdef NEW_ICEDAMAGE_SYSTEM
bool battle_is_icedamage(LPCHARACTER pAttacker, LPCHARACTER pVictim)
{
	if (pAttacker && pAttacker->IsPC())
	{
		DWORD race = pAttacker->GetRaceNum();
		const DWORD tmp_dwNDRFlag = pVictim->GetNoDamageRaceFlag();
		if (tmp_dwNDRFlag &&
			(race < MAIN_RACE_MAX_NUM) &&
			(IS_SET(tmp_dwNDRFlag, 1<<race))
		)
		{
			return false;
		}
		const std::set<DWORD> & tmp_setNDAFlag = pVictim->GetNoDamageAffectFlag();
		if (tmp_setNDAFlag.size())
		{
			for (std::set<DWORD>::iterator it = tmp_setNDAFlag.begin(); it != tmp_setNDAFlag.end(); ++it)
			{
				if (!pAttacker->IsAffectFlag(*it))
				{
					return false;
				}
			}
		}
	}
	return true;
}
#endif

bool battle_is_attackable(LPCHARACTER ch, LPCHARACTER victim)
{
	if (victim->IsDead())
		return false;

	{
		SECTREE	*sectree = NULL;

		sectree	= ch->GetSectree();
		if (sectree && sectree->IsAttr(ch->GetX(), ch->GetY(), ATTR_BANPK))
			return false;

		sectree = victim->GetSectree();
		if (sectree && sectree->IsAttr(victim->GetX(), victim->GetY(), ATTR_BANPK))
			return false;
	}
#ifdef NEW_ICEDAMAGE_SYSTEM
	if (!battle_is_icedamage(ch, victim))
		return false;
#endif
	if (ch->IsStun() || ch->IsDead())
		return false;

#ifdef ENABLE_ULTIMATE_REGEN
	if (!CNewMobTimer::Instance().CheckDamage(ch, victim))
		return false;
#endif

	if (ch->IsPC() && victim->IsPC())
	{
		CGuild* g1 = ch->GetGuild();
		CGuild* g2 = victim->GetGuild();

		if (g1 && g2)
		{
			if (g1->UnderWar(g2->GetID()))
				return true;
		}
	}

	if (IS_CASTLE_MAP(ch->GetMapIndex()) && false==castle_can_attack(ch, victim))
			return false;

	if (CArenaManager::instance().CanAttack(ch, victim) == true)
		return true;

#if defined(__SHIP_DEFENSE__)
	if (ch->IsNPC() && victim->GetRaceNum() == 20434)
		return true;

	if (ch->GetRaceNum() == 3963 || ch->GetRaceNum() == 3964)
		return false;

	if (victim->GetRaceNum() == 3963 || victim->GetRaceNum() == 3964)
		return false;
#endif

	return CPVPManager::instance().CanAttack(ch, victim);
}

int battle_melee_attack(LPCHARACTER ch, LPCHARACTER victim)
{
	if (test_server&&ch->IsPC())
		sys_log(0, "battle_melee_attack : [%s] attack to [%s]", ch->GetName(), victim->GetName());

	if (!victim || ch == victim)
		return BATTLE_NONE;

	if (test_server&&ch->IsPC())
		sys_log(0, "battle_melee_attack : [%s] attack to [%s]", ch->GetName(), victim->GetName());

	if (!battle_is_attackable(ch, victim))
		return BATTLE_NONE;

	if (test_server&&ch->IsPC())
		sys_log(0, "battle_melee_attack : [%s] attack to [%s]", ch->GetName(), victim->GetName());

	int distance = DISTANCE_APPROX(ch->GetX() - victim->GetX(), ch->GetY() - victim->GetY());

	if (!victim->IsBuilding())
	{
		int max = 300;

		if (false == ch->IsPC())
		{
			max = (int) (ch->GetMobAttackRange(victim->GetRaceNum()) * 1.15f);
		}
		else
		{
			if (false == victim->IsPC() && BATTLE_TYPE_MELEE == victim->GetMobBattleType())
				max = MAX(300, (int) (victim->GetMobAttackRange(victim->GetRaceNum()) * 1.15f));
			else if (victim->IsPC() == true)
				max += 30;
		}

#if defined(__SHIP_DEFENSE__)
		// PC melee attack on HUGE_RACE Hydra
		if (ch->IsPC() && CShipDefenseManager::Instance().IsHydra(victim->GetRaceNum()))
			max += 600;
#endif

		if (distance > max)
		{
			if (test_server)
				sys_log(0, "VICTIM_FAR: %s distance: %d max: %d", ch->GetName(), distance, max);

			return BATTLE_NONE;
		}
	}

	if (timed_event_cancel(ch))
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("전투가 시작 되어 취소 되었습니다."));

	if (timed_event_cancel(victim))
		victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("전투가 시작 되어 취소 되었습니다."));

	ch->SetPosition(POS_FIGHTING);
	ch->SetVictim(victim);

	const PIXEL_POSITION & vpos = victim->GetXYZ();
	ch->SetRotationToXY(vpos.x, vpos.y);

	int dam;
	int ret = battle_hit(ch, victim, dam);
	return (ret);
}

void battle_end_ex(LPCHARACTER ch)
{
	if (ch->IsPosition(POS_FIGHTING))
		ch->SetPosition(POS_STANDING);
}

void battle_end(LPCHARACTER ch)
{
	battle_end_ex(ch);
}

// AG = Attack Grade
// AL = Attack Limit
int CalcBattleDamage(int iDam, int iAttackerLev, int iVictimLev)
{
	if (iDam < 3)
		iDam = number(1, 5);

	//return CALCULATE_DAMAGE_LVDELTA(iAttackerLev, iVictimLev, iDam);
	return iDam;
}

int CalcMagicDamageWithValue(int iDam, LPCHARACTER pkAttacker, LPCHARACTER pkVictim)
{
	return CalcBattleDamage(iDam, pkAttacker->GetLevel(), pkVictim->GetLevel());
}

int CalcMagicDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim)
{
	int iDam = 0;

	if (pkAttacker->IsNPC())
	{
		iDam = CalcMeleeDamage(pkAttacker, pkVictim, false, false);
	}

	iDam += pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);

	return CalcMagicDamageWithValue(iDam, pkAttacker, pkVictim);
}

float CalcAttackRating(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, bool bIgnoreTargetRating)
{
	int iARSrc;
	int iERSrc;

	{
		int attacker_dx = pkAttacker->GetPolymorphPoint(POINT_DX);
		int attacker_lv = pkAttacker->GetLevel();

		int victim_dx = pkVictim->GetPolymorphPoint(POINT_DX);
		int victim_lv = pkAttacker->GetLevel();

		iARSrc = MIN(90, (attacker_dx * 4	+ attacker_lv * 2) / 6);
		iERSrc = MIN(90, (victim_dx	  * 4	+ victim_lv   * 2) / 6);
	}

	float fAR = ((float) iARSrc + 210.0f) / 300.0f;

	if (bIgnoreTargetRating)
		return fAR;

	float fER = ((float) (iERSrc * 2 + 5) / (iERSrc + 95)) * 3.0f / 10.0f;

	return fAR - fER;
}

int SumAttResBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int attBonusID, int resistBonusID)
{
	return pkAttacker->GetPoint(attBonusID) - pkVictim->GetPoint(resistBonusID);
}

int SumBreakBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int attBonusID, int resistBonusID)
{
	if (!pkVictim->IsPC())
		return 100 - pkVictim->GetPoint(resistBonusID);
	return 100 + pkAttacker->GetPoint(attBonusID) - pkVictim->GetPoint(resistBonusID);
}

int CalcDamBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int iDam)
{
	LPITEM pkWeapon = pkAttacker->GetWear(WEAR_WEAPON);
	if (pkWeapon)
	{
		switch (pkWeapon->GetSubType())
		{
			case WEAPON_SWORD:
				iDam = iDam * SumBreakBonus(pkAttacker, pkVictim, POINT_ATTBONUS_SWORD, POINT_RESIST_SWORD) / 100;
				break;

			case WEAPON_TWO_HANDED:
				iDam = iDam * SumBreakBonus(pkAttacker, pkVictim, POINT_ATTBONUS_TWOHAND, POINT_RESIST_TWOHAND) / 100;
				break;

			case WEAPON_DAGGER:
				iDam = iDam * SumBreakBonus(pkAttacker, pkVictim, POINT_ATTBONUS_DAGGER, POINT_RESIST_DAGGER) / 100;
				break;

			case WEAPON_BELL:
				iDam = iDam * SumBreakBonus(pkAttacker, pkVictim, POINT_ATTBONUS_BELL, POINT_RESIST_BELL) / 100;
				break;

			case WEAPON_FAN:
				iDam = iDam * SumBreakBonus(pkAttacker, pkVictim, POINT_ATTBONUS_FAN, POINT_RESIST_FAN) / 100;
				break;

			#ifdef ENABLE_WOLFMAN_CHARACTER
			case WEAPON_CLAW:
				iDam = iDam * SumBreakBonus(pkAttacker, pkVictim, POINT_ATTBONUS_CLAW, POINT_RESIST_CLAW) / 100;
				#if defined(ENABLE_WOLFMAN_CHARACTER) && defined(USE_ITEM_CLAW_AS_DAGGER)
				iDam = iDam * SumBreakBonus(pkAttacker, pkVictim, POINT_ATTBONUS_DAGGER, POINT_RESIST_DAGGER) / 100;
				#endif
				break;
			#endif
		}
	}
	return iDam;
}

int CalcAttBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int iAtk)
{
	if (!pkVictim->IsPC())
		iAtk += pkAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_ATTACK_BONUS);

	if (!pkAttacker->IsPC())
	{
		int iReduceDamagePct = pkVictim->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_TRANSFER_DAMAGE);
		iAtk = iAtk * (100 + iReduceDamagePct) / 100;
	}

	if (pkAttacker->IsNPC() && pkVictim->IsPC())
	{
		iAtk = (iAtk * CHARACTER_MANAGER::instance().GetMobDamageRate(pkAttacker)) / 100;
	}

	if (pkVictim->IsNPC())
	{
		if (pkVictim->IsRaceFlag(RACE_FLAG_ANIMAL))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ANIMAL)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_UNDEAD))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_UNDEAD)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_DEVIL))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_DEVIL)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_HUMAN))
			iAtk += (iAtk  * SumAttResBonus(pkAttacker, pkVictim, POINT_ATTBONUS_HUMAN, POINT_RESIST_HUMAN)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_ORC))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ORC)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_MILGYO))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_MILGYO)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_INSECT))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_INSECT)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_FIRE))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_FIRE)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_ICE))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ICE)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_DESERT))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_DESERT)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_TREE))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_TREE)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_CZ))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_CZ)) / 100;

		if (pkVictim->IsStone())
			iAtk += (iAtk * 1.8 * pkAttacker->GetPoint(POINT_ATTBONUS_STONE)) / 100;

		if (CHARACTER_MANAGER::instance().IsDungeonMap(pkVictim, pkAttacker))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_DUNGEON)) / 100;

		if (pkVictim->GetMobRank() == MOB_RANK_BOSS)
			iAtk += (iAtk * (1.6 * pkAttacker->GetPoint(POINT_ATTBONUS_BOSS))) / 100;

		iAtk += (iAtk * (1.5 * pkAttacker->GetPoint(POINT_CRITICAL_DAMAGE_PVM))) /100;

		if (pkVictim->IsStone())
			iAtk += (iAtk * 0.7 * pkAttacker->GetPoint(POINT_ATTBONUS_MONSTER)) / 100;
		else
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_MONSTER)) / 100;
	}
	else if (pkVictim->IsPC())
	{
		if ((pkAttacker->IsNPC() || !pkAttacker->IsStone() || pkAttacker->GetMobRank() != MOB_RANK_BOSS))
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_MONSTER)) / 100;

		if (pkAttacker->GetMobRank() == MOB_RANK_BOSS)
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_BOSS)) / 100;

		iAtk += (iAtk * SumAttResBonus(pkAttacker, pkVictim, POINT_ATTBONUS_HUMAN, POINT_RESIST_HUMAN)) / 100;
		
		switch (pkVictim->GetJob())
		{
			case JOB_WARRIOR:
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_WARRIOR)) / 100;
				break;

			case JOB_ASSASSIN:
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ASSASSIN)) / 100;
				break;

			case JOB_SURA:
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_SURA)) / 100;
				break;

			case JOB_SHAMAN:
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_SHAMAN)) / 100;
				break;
#ifdef ENABLE_WOLFMAN_CHARACTER
			case JOB_WOLFMAN:
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_WOLFMAN)) / 100;
				break;
#endif
		}	
	}

	if (pkAttacker->IsPC())
	{
		switch (pkAttacker->GetJob())
		{
			case JOB_WARRIOR:
				iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_WARRIOR)) / 100;
				break;

			case JOB_ASSASSIN:
				iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_ASSASSIN)) / 100;
				break;

			case JOB_SURA:
				iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_SURA)) / 100;
				break;

			case JOB_SHAMAN:
				iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_SHAMAN)) / 100;
				break;

			#ifdef ENABLE_WOLFMAN_CHARACTER
			case JOB_WOLFMAN:
				iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_WOLFMAN)) / 100;
				break;
			#endif
		}
		if (pkVictim->IsPC())
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_RACES)) / 100;
	}

	if (pkAttacker->IsNPC() && pkVictim->IsPC())
	{
		if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_ELEC) || pkAttacker->GetMobElement(MOB_ELEMENT_ELECT))
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_ELEC)) / 100;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_FIRE) || pkAttacker->GetMobElement(MOB_ELEMENT_FIRE))
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_FIRE)) / 100;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_ICE) || pkAttacker->GetMobElement(MOB_ELEMENT_ICE))
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_ICE)) / 100;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_WIND) || pkAttacker->GetMobElement(MOB_ELEMENT_WIND))
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_WIND)) / 100;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_EARTH) || pkAttacker->GetMobElement(MOB_ELEMENT_EARTH))
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_EARTH)) / 100;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_DARK) || pkAttacker->GetMobElement(MOB_ELEMENT_DARK))
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_DARK)) / 100;
	}

	if (pkAttacker->IsPC() && pkVictim->IsNPC())
	{
		if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_ELEC) || pkVictim->GetMobElement(MOB_ELEMENT_ELECT))
			iAtk += ((iAtk * pkAttacker->GetPoint(POINT_ENCHANT_ELECT)) / 100) - ((((iAtk * pkAttacker->GetPoint(POINT_ENCHANT_ELECT)) / 100) * pkVictim->GetMobElement(MOB_ELEMENT_ELECT)) / 100);
		else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_FIRE) || pkVictim->GetMobElement(MOB_ELEMENT_FIRE))
			iAtk += ((iAtk * pkAttacker->GetPoint(POINT_ENCHANT_FIRE)) / 100) - ((((iAtk * pkAttacker->GetPoint(POINT_ENCHANT_FIRE)) / 100) * pkVictim->GetMobElement(MOB_ELEMENT_FIRE)) / 100);
		else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_ICE) || pkVictim->GetMobElement(MOB_ELEMENT_ICE))
			iAtk += ((iAtk * pkAttacker->GetPoint(POINT_ENCHANT_ICE)) / 100) - ((((iAtk * pkAttacker->GetPoint(POINT_ENCHANT_ICE)) / 100) * pkVictim->GetMobElement(MOB_ELEMENT_ICE)) / 100);
		else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_WIND) || pkVictim->GetMobElement(MOB_ELEMENT_WIND))
			iAtk += ((iAtk * pkAttacker->GetPoint(POINT_ENCHANT_WIND)) / 100) - ((((iAtk * pkAttacker->GetPoint(POINT_ENCHANT_WIND)) / 100) * pkVictim->GetMobElement(MOB_ELEMENT_WIND)) / 100);
		else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_EARTH) || pkVictim->GetMobElement(MOB_ELEMENT_EARTH))
			iAtk += ((iAtk * pkAttacker->GetPoint(POINT_ENCHANT_EARTH)) / 100) - ((((iAtk * pkAttacker->GetPoint(POINT_ENCHANT_EARTH)) / 100) * pkVictim->GetMobElement(MOB_ELEMENT_EARTH)) / 100);
		else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_DARK) || pkVictim->GetMobElement(MOB_ELEMENT_DARK))
			iAtk += ((iAtk * pkAttacker->GetPoint(POINT_ENCHANT_DARK)) / 100) - ((((iAtk * pkAttacker->GetPoint(POINT_ENCHANT_DARK)) / 100) * pkVictim->GetMobElement(MOB_ELEMENT_DARK)) / 100);
	}

	return iAtk;
}

void Item_GetDamage(LPITEM pkItem, int* pdamMin, int* pdamMax)
{
	*pdamMin = 0;
	*pdamMax = 1;

	if (!pkItem)
		return;

	switch (pkItem->GetType())
	{
		case ITEM_ROD:
		case ITEM_PICK:
			return;
	}

	if (pkItem->GetType() != ITEM_WEAPON)
		sys_err("Item_GetDamage - !ITEM_WEAPON vnum=%d, type=%d", pkItem->GetOriginalVnum(), pkItem->GetType());

	*pdamMin = pkItem->GetValue(3);
	*pdamMax = pkItem->GetValue(4);
}

int CalcMeleeDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, bool bIgnoreDefense, bool bIgnoreTargetRating)
{
	LPITEM pWeapon = pkAttacker->GetWear(WEAR_WEAPON);
	bool bPolymorphed = pkAttacker->IsPolymorphed();

	if (pWeapon && !(bPolymorphed && !pkAttacker->IsPolyMaintainStat()))
	{
		if (pWeapon->GetType() != ITEM_WEAPON)
			return 0;

		switch (pWeapon->GetSubType())
		{
			case WEAPON_SWORD:
			case WEAPON_DAGGER:
			case WEAPON_TWO_HANDED:
			case WEAPON_BELL:
			case WEAPON_FAN:
			case WEAPON_MOUNT_SPEAR:
#ifdef ENABLE_WOLFMAN_CHARACTER
			case WEAPON_CLAW:
#endif
				break;

			case WEAPON_BOW:
				sys_err("CalcMeleeDamage should not handle bows (name: %s)", pkAttacker->GetName());
				return 0;

			default:
				return 0;
		}
	}

	int iDam = 0;
	float fAR = CalcAttackRating(pkAttacker, pkVictim, bIgnoreTargetRating);
	int iDamMin = 0, iDamMax = 0;

	// TESTSERVER_SHOW_ATTACKINFO
	int DEBUG_iDamCur = 0;
	int DEBUG_iDamBonus = 0;
	// END_OF_TESTSERVER_SHOW_ATTACKINFO

	if (bPolymorphed && !pkAttacker->IsPolyMaintainStat())
	{
		// MONKEY_ROD_ATTACK_BUG_FIX
		Item_GetDamage(pWeapon, &iDamMin, &iDamMax);
		// END_OF_MONKEY_ROD_ATTACK_BUG_FIX

		DWORD dwMobVnum = pkAttacker->GetPolymorphVnum();
		const CMob * pMob = CMobManager::instance().Get(dwMobVnum);

		if (pMob)
		{
			int iPower = pkAttacker->GetPolymorphPower();
			iDamMin += pMob->m_table.dwDamageRange[0] * iPower / 100;
			iDamMax += pMob->m_table.dwDamageRange[1] * iPower / 100;
		}
	}
	else if (pWeapon)
	{
		// MONKEY_ROD_ATTACK_BUG_FIX
		Item_GetDamage(pWeapon, &iDamMin, &iDamMax);
		// END_OF_MONKEY_ROD_ATTACK_BUG_FIX
	}
	else if (pkAttacker->IsNPC())
	{
		iDamMin = pkAttacker->GetMobDamageMin();
		iDamMax = pkAttacker->GetMobDamageMax();
	}

	iDam = number(iDamMin, iDamMax) * 2;

	// TESTSERVER_SHOW_ATTACKINFO
	DEBUG_iDamCur = iDam;
	// END_OF_TESTSERVER_SHOW_ATTACKINFO

	int iAtk = 0;

	// level must be ignored when multiply by fAR, so subtract it before calculation.
	iAtk = pkAttacker->GetPoint(POINT_ATT_GRADE) + iDam - (pkAttacker->GetLevel() * 2);
	iAtk = (int) (iAtk * fAR);
	iAtk += pkAttacker->GetLevel() * 2; // and add again

	if (pWeapon)
	{
		iAtk += pWeapon->GetValue(5) * 2;

		// 2004.11.12.myevan.TESTSERVER_SHOW_ATTACKINFO
		DEBUG_iDamBonus = pWeapon->GetValue(5) * 2;
		///////////////////////////////////////////////
	}

	iAtk += pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS); // party attacker role bonus
	iAtk = (int) (iAtk * (100 + (pkAttacker->GetPoint(POINT_ATT_BONUS) + pkAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100);

	iAtk = CalcAttBonus(pkAttacker, pkVictim, iAtk);

	int iDef = 0;

	if (!bIgnoreDefense)
	{
		iDef = (pkVictim->GetPoint(POINT_DEF_GRADE) * (100 + pkVictim->GetPoint(POINT_DEF_BONUS)) / 100);

		if (!pkAttacker->IsPC())
			iDef += pkVictim->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_DEFENSE_BONUS);
	}

	if (pkAttacker->IsNPC())
		iAtk = (int) (iAtk * pkAttacker->GetMobDamageMultiply());

	iDam = MAX(0, iAtk - iDef);

	if (test_server)
	{
		int DEBUG_iLV = pkAttacker->GetLevel()*2;
		int DEBUG_iST = int((pkAttacker->GetPoint(POINT_ATT_GRADE) - DEBUG_iLV) * fAR);
		int DEBUG_iPT = pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);
		int DEBUG_iWP = 0;
		int DEBUG_iPureAtk = 0;
		int DEBUG_iPureDam = 0;
		char szRB[32] = "";
		char szGradeAtkBonus[32] = "";

		DEBUG_iWP = int(DEBUG_iDamCur * fAR);
		DEBUG_iPureAtk = DEBUG_iLV + DEBUG_iST + DEBUG_iWP+DEBUG_iDamBonus;
		DEBUG_iPureDam = iAtk - iDef;

		if (pkAttacker->IsNPC())
		{
			snprintf(szGradeAtkBonus, sizeof(szGradeAtkBonus), "=%d*%.1f", DEBUG_iPureAtk, pkAttacker->GetMobDamageMultiply());
			DEBUG_iPureAtk = int(DEBUG_iPureAtk * pkAttacker->GetMobDamageMultiply());
		}

		if (DEBUG_iDamBonus != 0)
			snprintf(szRB, sizeof(szRB), "+RB(%d)", DEBUG_iDamBonus);

		char szPT[32] = "";

		if (DEBUG_iPT != 0)
			snprintf(szPT, sizeof(szPT), ", PT=%d", DEBUG_iPT);

		char szUnknownAtk[32] = "";

		if (iAtk != DEBUG_iPureAtk)
			snprintf(szUnknownAtk, sizeof(szUnknownAtk), "+?(%d)", iAtk-DEBUG_iPureAtk);

		char szUnknownDam[32] = "";

		if (iDam != DEBUG_iPureDam)
			snprintf(szUnknownDam, sizeof(szUnknownDam), "+?(%d)", iDam-DEBUG_iPureDam);

		char szMeleeAttack[128];

		snprintf(szMeleeAttack, sizeof(szMeleeAttack),
				"%s(%d)-%s(%d)=%d%s, ATK=LV(%d)+ST(%d)+WP(%d)%s%s%s, AR=%.3g%s",
				pkAttacker->GetName(),
				iAtk,
				pkVictim->GetName(),
				iDef,
				iDam,
				szUnknownDam,
				DEBUG_iLV,
				DEBUG_iST,
				DEBUG_iWP,
				szRB,
				szUnknownAtk,
				szGradeAtkBonus,
				fAR,
				szPT);

		pkAttacker->ChatPacket(CHAT_TYPE_TALKING, "%s", szMeleeAttack);
		pkVictim->ChatPacket(CHAT_TYPE_TALKING, "%s", szMeleeAttack);
	}

	return CalcBattleDamage(iDam, pkAttacker->GetLevel(), pkVictim->GetLevel());
}

int CalcArrowDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, LPITEM pkBow, LPITEM pkArrow, bool bIgnoreDefense)
{
	if (!pkBow || pkBow->GetType() != ITEM_WEAPON || pkBow->GetSubType() != WEAPON_BOW)
		return 0;

	if (!pkArrow)
		return 0;

	int iDist = (int) (DISTANCE_SQRT(pkAttacker->GetX() - pkVictim->GetX(), pkAttacker->GetY() - pkVictim->GetY()));
	//int iGap = (iDist / 100) - 5 - pkBow->GetValue(5) - pkAttacker->GetPoint(POINT_BOW_DISTANCE);
	int iGap = (iDist / 100) - 5 - pkAttacker->GetPoint(POINT_BOW_DISTANCE);
	int iPercent = 100 - (iGap * 5);

#ifdef ENABLE_QUIVER_SYSTEM
	if (pkArrow->GetSubType() == WEAPON_QUIVER)
		iPercent = 100;
#endif

	if (iPercent <= 0)
		return 0;
	else if (iPercent > 100)
		iPercent = 100;

	int iDam = 0;

	float fAR = CalcAttackRating(pkAttacker, pkVictim, false);
	iDam = number(pkBow->GetValue(3), pkBow->GetValue(4)) * 2 + pkArrow->GetValue(3);
	int iAtk;

	// level must be ignored when multiply by fAR, so subtract it before calculation.
	iAtk = pkAttacker->GetPoint(POINT_ATT_GRADE) + iDam - (pkAttacker->GetLevel() * 2);
	iAtk = (int) (iAtk * fAR);
	iAtk += pkAttacker->GetLevel() * 2; // and add again

	// Refine Grade
	iAtk += pkBow->GetValue(5) * 2;

	iAtk += pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);
	iAtk = (int) (iAtk * (100 + (pkAttacker->GetPoint(POINT_ATT_BONUS) + pkAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100);

	iAtk = CalcAttBonus(pkAttacker, pkVictim, iAtk);

	int iDef = 0;

	if (!bIgnoreDefense)
		iDef = (pkVictim->GetPoint(POINT_DEF_GRADE) * (100 + pkAttacker->GetPoint(POINT_DEF_BONUS)) / 100);

	if (pkAttacker->IsNPC())
		iAtk = (int) (iAtk * pkAttacker->GetMobDamageMultiply());

	iDam = MAX(0, iAtk - iDef);

	int iPureDam = iDam;

	iPureDam = (iPureDam * iPercent) / 100;

	if (test_server)
	{
		pkAttacker->ChatPacket(CHAT_TYPE_INFO, "ARROW %s -> %s, DAM %d DIST %d GAP %d %% %d",
				pkAttacker->GetName(),
				pkVictim->GetName(),
				iPureDam,
				iDist, iGap, iPercent);
	}

	return iPureDam;
	//return iDam;
}

void NormalAttackAffect(LPCHARACTER pkAttacker, LPCHARACTER pkVictim)
{
	if (pkAttacker->GetPoint(POINT_POISON_PCT) && !pkVictim->IsAffectFlag(AFF_POISON))
	{
		if (number(1, 100) <= pkAttacker->GetPoint(POINT_POISON_PCT))
			pkVictim->AttackedByPoison(pkAttacker);
	}
#ifdef ENABLE_WOLFMAN_CHARACTER
	if (pkAttacker->GetPoint(POINT_BLEEDING_PCT) && !pkVictim->IsAffectFlag(AFF_BLEEDING))
	{
		if (number(1, 100) <= pkAttacker->GetPoint(POINT_BLEEDING_PCT))
			pkVictim->AttackedByBleeding(pkAttacker);
	}
#endif
	int iStunDuration = 2;
	if (pkAttacker->IsPC() && !pkVictim->IsPC())
		iStunDuration = 4;

	AttackAffect(pkAttacker, pkVictim, POINT_STUN_PCT, IMMUNE_STUN,  AFFECT_STUN, POINT_NONE,        0, AFF_STUN, iStunDuration, "STUN");
	AttackAffect(pkAttacker, pkVictim, POINT_SLOW_PCT, IMMUNE_SLOW,  AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, 20,		"SLOW");
}

int battle_hit(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int & iRetDam)
{
	if (test_server)
		sys_log(0, "battle_hit : [%s] attack to [%s] : dam :%d type :%d", pkAttacker->GetName(), pkVictim->GetName(), iRetDam);

	int iDam = CalcMeleeDamage(pkAttacker, pkVictim);

	if (iDam <= 0)
		return (BATTLE_DAMAGE);

	NormalAttackAffect(pkAttacker, pkVictim);

	iDam = CalcDamBonus(pkAttacker, pkVictim, iDam);

	float attMul = pkAttacker->GetAttMul();
	float tempIDam = iDam;
	iDam = attMul * tempIDam + 0.5f;

	iRetDam = iDam;

	if (pkVictim->Damage(pkAttacker, iDam, DAMAGE_TYPE_NORMAL))
		return (BATTLE_DEAD);

	return (BATTLE_DAMAGE);
}

DWORD GET_ATTACK_SPEED(LPCHARACTER ch)
{
	if (NULL == ch)
		return 1000;

	LPITEM item = ch->GetWear(WEAR_WEAPON);
	DWORD default_bonus = SPEEDHACK_LIMIT_BONUS;
	DWORD riding_bonus = 0;

	if (ch->IsRiding())
	{
		riding_bonus = 50;
	}

	DWORD ani_speed = ani_attack_speed(ch);
	DWORD real_speed = (ani_speed * 100) / (default_bonus + ch->GetPoint(POINT_ATT_SPEED) + riding_bonus);

	// if (item && item->GetSubType() == WEAPON_DAGGER)
	// 	real_speed /= 2;
#ifdef ENABLE_WOLFMAN_CHARACTER
	if (item && item->GetSubType() == WEAPON_CLAW)
		real_speed /= 2;
#endif

	return real_speed;
}

void SET_ATTACK_TIME(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time)
{
	if (NULL == ch || NULL == victim)
		return;

	if (!ch->IsPC())
		return;

	ch->m_kAttackLog.dwVID = victim->GetVID();
	ch->m_kAttackLog.dwTime = current_time;
}

void SET_ATTACKED_TIME(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time)
{
	if (NULL == ch || NULL == victim)
		return;

	if (!ch->IsPC())
		return;

	victim->m_AttackedLog.dwPID			= ch->GetPlayerID();
	victim->m_AttackedLog.dwAttackedTime= current_time;
}

bool IS_SPEED_HACK(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time)
{
	if (!ch->IsPC())
		return false;

	if(!gHackCheckEnable) return false;

	DWORD speed = GET_ATTACK_SPEED(ch) * 0.97;

	if (ch->m_kAttackLog.dwVID == victim->GetVID())
	{
		if (current_time - ch->m_kAttackLog.dwTime < speed)
		{
			INCREASE_SPEED_HACK_COUNT(ch);

			SET_ATTACK_TIME(ch, victim, current_time);
			SET_ATTACKED_TIME(ch, victim, current_time);
			LogManager::instance().SpeedHackLog(ch->GetPlayerID(), ch->GetMapIndex(), ch->GetX(), ch->GetY(), GET_SPEED_HACK_COUNT(ch));

			return true;
		}
	}

	SET_ATTACK_TIME(ch, victim, current_time);

	if (victim->m_AttackedLog.dwPID == ch->GetPlayerID())
	{
		if (current_time - victim->m_AttackedLog.dwAttackedTime < speed)
		{
			INCREASE_SPEED_HACK_COUNT(ch);

			SET_ATTACKED_TIME(ch, victim, current_time);
			LogManager::instance().SpeedHackLog(ch->GetPlayerID(), ch->GetMapIndex(), ch->GetX(), ch->GetY(), GET_SPEED_HACK_COUNT(ch));
			return true;
		}
	}

	SET_ATTACKED_TIME(ch, victim, current_time);
	return false;
}
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
