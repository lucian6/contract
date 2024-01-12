#ifndef __INC_METIN_II_GAME_CONSTANTS_H__
#define __INC_METIN_II_GAME_CONSTANTS_H__

#include "../../common/tables.h"

typedef struct SMobRankStat
{
	int iGoldPercent;
} TMobRankStat;

typedef struct SMobStat
{
	BYTE	byLevel;
	WORD	HP;
	DWORD	dwExp;
	WORD	wDefGrade;
} TMobStat;

typedef struct SBattleTypeStat
{
	int		AttGradeBias;
	int		DefGradeBias;
	int		MagicAttGradeBias;
	int		MagicDefGradeBias;
} TBattleTypeStat;

typedef struct SJobInitialPoints
{
	int		st, ht, dx, iq;
	int		max_hp, max_sp;
	int		hp_per_ht, sp_per_iq;
	int		hp_per_lv_begin, hp_per_lv_end;
	int		sp_per_lv_begin, sp_per_lv_end;
	int		max_stamina;
	int		stamina_per_con;
	int		stamina_per_lv_begin, stamina_per_lv_end;
} TJobInitialPoints;

typedef struct SPassiveSkillStats
{
	int	bonusLevel;
	int bonus1type;
	long bonus1value;
	int bonus2type;
	long bonus2value;
	int bonus3type;
	long bonus3value;
	int bonus4type;
	long bonus4value;
	int bonus5type;
	long bonus5value;
	int bonus6type;
	long bonus6value;
} TPassiveSkillStats;

typedef struct __coord
{
	int		x, y;
} Coord;

typedef struct SApplyInfo
{
	BYTE	bPointType;                          // APPLY -> POINT
} TApplyInfo;

enum {
	FORTUNE_BIG_LUCK,
	FORTUNE_LUCK,
	FORTUNE_SMALL_LUCK,
	FORTUNE_NORMAL,
	FORTUNE_SMALL_BAD_LUCK,
	FORTUNE_BAD_LUCK,
	FORTUNE_BIG_BAD_LUCK,
	FORTUNE_MAX_NUM,
};

const int STONE_INFO_MAX_NUM = 12;
const int STONE_LEVEL_MAX_NUM = 5;

struct SStoneDropInfo
{
	DWORD dwMobVnum;
	int iDropPct;
	int iLevelPct[STONE_LEVEL_MAX_NUM+1];
};

inline bool operator < (const SStoneDropInfo& l, DWORD r)
{
	return l.dwMobVnum < r;
}

inline bool operator < (DWORD l, const SStoneDropInfo& r)
{
	return l < r.dwMobVnum;
}

inline bool operator < (const SStoneDropInfo& l, const SStoneDropInfo& r)
{
	return l.dwMobVnum < r.dwMobVnum;
}

#ifdef ENABLE_BIYOLOG
extern const DWORD bio_data[][17];
extern const BYTE bio_max;
extern BYTE pointToApply(BYTE p);
#endif

extern const TApplyInfo		aApplyInfo[MAX_APPLY_NUM];
extern const TMobRankStat       MobRankStats[MOB_RANK_MAX_NUM];

extern TBattleTypeStat		BattleTypeStats[BATTLE_TYPE_MAX_NUM];

extern const DWORD		party_exp_distribute_table[PLAYER_EXP_TABLE_MAX + 1];

extern const DWORD		exp_table_common[PLAYER_MAX_LEVEL_CONST + 1];

extern const DWORD*		exp_table;
extern const DWORD		exp_table_pet[PET_MAX_LEVEL_CONST+1];

extern const DWORD		guild_exp_table[GUILD_MAX_LEVEL + 1];
extern const DWORD		guild_exp_table2[GUILD_MAX_LEVEL + 1];

#define MAX_EXP_DELTA_OF_LEV	41
#define PERCENT_LVDELTA(me, victim) aiPercentByDeltaLev_euckr[MINMAX(0, (victim + 20) - me, MAX_EXP_DELTA_OF_LEV - 1)]
#define PERCENT_LVDELTA_BOSS(me, victim) aiPercentByDeltaLevForBoss_euckr[MINMAX(0, (victim + 20) - me, MAX_EXP_DELTA_OF_LEV - 1)]
#define CALCULATE_VALUE_LVDELTA(me, victim, val) ((val * PERCENT_LVDELTA(me, victim)) / 100)
extern const int		aiPercentByDeltaLev_euckr[MAX_EXP_DELTA_OF_LEV];
extern const int		aiPercentByDeltaLevForBoss_euckr[MAX_EXP_DELTA_OF_LEV];
extern const int *		aiPercentByDeltaLev;
extern const int *		aiPercentByDeltaLevForBoss;

#define ARROUND_COORD_MAX_NUM	161
extern Coord			aArroundCoords[ARROUND_COORD_MAX_NUM];
extern TJobInitialPoints	JobInitialPoints[JOB_MAX_NUM];
extern TPassiveSkillStats	PassiveSkillStats[41];
extern const int		aiMobEnchantApplyIdx[MOB_ENCHANTS_MAX_NUM];
extern const int		aiMobResistsApplyIdx[MOB_RESISTS_MAX_NUM];

extern const int		aSkillAttackAffectProbByRank[MOB_RANK_MAX_NUM];

extern const int aiItemMagicAttributePercentHigh[ITEM_ATTRIBUTE_MAX_LEVEL];
extern const int aiItemMagicAttributePercentLow[ITEM_ATTRIBUTE_MAX_LEVEL];

extern const int aiItemAttributeAddPercent[ITEM_ATTRIBUTE_MAX_NUM];

extern const int aiWeaponSocketQty[WEAPON_NUM_TYPES];
extern const int aiArmorSocketQty[ARMOR_NUM_TYPES];
extern const int aiSocketPercentByQty[5][4];

extern const int aiExpLossPercents[PLAYER_EXP_TABLE_MAX + 1];

extern const int * aiSkillPowerByLevel;
extern const int aiSkillPowerByLevel_euckr[SKILL_MAX_LEVEL + 1];

extern const int aiPolymorphPowerByLevel[SKILL_MAX_LEVEL + 1];
#ifdef __DUNGEON_INFO__
extern const std::map<DWORD, std::pair<std::pair<BYTE, BYTE>, std::string>> m_mapDungeonList;
#endif

extern const int aiSkillBookCountForLevelUp[10];
extern const int aiGrandMasterSkillBookCountForLevelUp[10];
extern const int aiGrandMasterSkillBookMinCount[10];
extern const int aiGrandMasterSkillBookMaxCount[10];
extern const int CHN_aiPartyBonusExpPercentByMemberCount[9];
extern const int KOR_aiPartyBonusExpPercentByMemberCount[9];
extern const int KOR_aiUniqueItemPartyBonusExpPercentByMemberCount[9];

typedef std::map<DWORD, TItemAttrTable> TItemAttrMap;
extern TItemAttrMap g_map_itemAttr;
extern TItemAttrMap g_map_itemRare;

extern const int * aiChainLightningCountBySkillLevel;
extern const int aiChainLightningCountBySkillLevel_euckr[SKILL_MAX_LEVEL + 1];

extern const char * c_apszEmpireNames[EMPIRE_MAX_NUM];
extern const char * c_apszPrivNames[MAX_PRIV_NUM];
extern const SStoneDropInfo aStoneDrop[STONE_INFO_MAX_NUM];

#ifdef ENABLE_NEW_PET_SYSTEM
extern std::map<BYTE, std::vector<std::pair<DWORD, WORD>>> petEvolutionItems;
extern std::map<BYTE, std::vector<std::pair<BYTE, long>>> petSkillBonus;
extern BYTE petEvolutionLimits[PET_MAX_EVOLUTION_CONST + 1];
#endif

typedef struct SGuildWarInfo
{
	long lMapIndex;
	int iWarPrice;
	int iWinnerPotionRewardPctToWinner;
	int iLoserPotionRewardPctToWinner;
	int iInitialScore;
	int iEndScore;
} TGuildWarInfo;

extern TGuildWarInfo KOR_aGuildWarInfo[GUILD_WAR_TYPE_MAX_NUM];

// ACCESSORY_REFINE
enum
{
	ITEM_ACCESSORY_SOCKET_MAX_NUM = 3
};

extern const int aiAccessorySocketAddPct[ITEM_ACCESSORY_SOCKET_MAX_NUM];
extern const int aiAccessorySocketEffectivePct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1];
extern const int aiAccessorySocketDegradeTime[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1];
extern const int aiAccessorySocketPutPct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1];
long FN_get_apply_type(const char *apply_type_string);

#ifdef __SASH_SKIN__
struct dream_soul_bonus
{
	BYTE apply_idx;
	DWORD soulItemIndx;
	std::vector<std::pair<WORD, BYTE>> vecBonusData;
	dream_soul_bonus(BYTE _apply_idx, DWORD _soulItemIndx, std::vector<std::pair<WORD, BYTE>> _vecBonusData) : apply_idx(_apply_idx), soulItemIndx(_soulItemIndx), vecBonusData(_vecBonusData) {}
};
struct dream_soul
{
	DWORD nextItem;
	DWORD cost;
	BYTE lucky;
	std::vector<std::pair<DWORD, WORD>> needItems;
	dream_soul(const DWORD _nextItem, const DWORD _cost, const BYTE _lucky, const std::vector<std::pair<DWORD, WORD>> _needItems) : nextItem(_nextItem), cost(_cost), lucky(_lucky), needItems(_needItems) {}
};
const extern std::map<BYTE, long> m_CombinePrices;
const extern std::map<DWORD, dream_soul> dreamSoulRefineData;
const extern std::map<BYTE, dream_soul_bonus> m_dreamSoulData;
#endif

// END_OF_ACCESSORY_REFINE

long FN_get_apply_type(const char *apply_type_string);
#endif
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
