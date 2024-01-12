#ifndef __INC_TABLES_H__
#define __INC_TABLES_H__

#include "length.h"
#include "item_length.h"
#include "CommonDefines.h"

typedef	DWORD IDENT;

enum
{
	HEADER_GD_LOGIN				= 1,
	HEADER_GD_LOGOUT			= 2,

	HEADER_GD_PLAYER_LOAD		= 3,
	HEADER_GD_PLAYER_SAVE		= 4,
	HEADER_GD_PLAYER_CREATE		= 5,
	HEADER_GD_PLAYER_DELETE		= 6,

	HEADER_GD_LOGIN_KEY			= 7,
	// 8 empty
	HEADER_GD_BOOT				= 9,
	HEADER_GD_PLAYER_COUNT		= 10,
	HEADER_GD_QUEST_SAVE		= 11,
	HEADER_GD_SAFEBOX_LOAD		= 12,
	HEADER_GD_SAFEBOX_SAVE		= 13,
	HEADER_GD_SAFEBOX_CHANGE_SIZE	= 14,
	HEADER_GD_EMPIRE_SELECT		= 15,

	HEADER_GD_SAFEBOX_CHANGE_PASSWORD		= 16,
	HEADER_GD_SAFEBOX_CHANGE_PASSWORD_SECOND	= 17, // Not really a packet, used internal
	HEADER_GD_DIRECT_ENTER		= 18,

	HEADER_GD_GUILD_SKILL_UPDATE	= 19,
	HEADER_GD_GUILD_EXP_UPDATE		= 20,
	HEADER_GD_GUILD_ADD_MEMBER		= 21,
	HEADER_GD_GUILD_REMOVE_MEMBER	= 22,
	HEADER_GD_GUILD_CHANGE_GRADE	= 23,
	HEADER_GD_GUILD_CHANGE_MEMBER_DATA	= 24,
	HEADER_GD_GUILD_DISBAND		= 25,
	HEADER_GD_GUILD_WAR			= 26,
	HEADER_GD_GUILD_WAR_SCORE		= 27,
	HEADER_GD_GUILD_CREATE		= 28,

	HEADER_GD_ITEM_SAVE			= 30,
	HEADER_GD_ITEM_DESTROY		= 31,

	HEADER_GD_ADD_AFFECT		= 32,
	HEADER_GD_REMOVE_AFFECT		= 33,

	HEADER_GD_HIGHSCORE_REGISTER	= 34,
	HEADER_GD_ITEM_FLUSH		= 35,

	HEADER_GD_PARTY_CREATE		= 36,
	HEADER_GD_PARTY_DELETE		= 37,
	HEADER_GD_PARTY_ADD			= 38,
	HEADER_GD_PARTY_REMOVE		= 39,
	HEADER_GD_PARTY_STATE_CHANGE	= 40,
	HEADER_GD_PARTY_HEAL_USE		= 41,

	HEADER_GD_FLUSH_CACHE		= 42,
	HEADER_GD_RELOAD_PROTO		= 43,

	HEADER_GD_CHANGE_NAME		= 44,

	HEADER_GD_GUILD_CHANGE_LADDER_POINT	= 46,
	HEADER_GD_GUILD_USE_SKILL		= 47,

	HEADER_GD_REQUEST_EMPIRE_PRIV	= 48,
	HEADER_GD_REQUEST_GUILD_PRIV	= 49,

	HEADER_GD_MONEY_LOG				= 50,

	HEADER_GD_GUILD_DEPOSIT_MONEY				= 51,
	HEADER_GD_GUILD_WITHDRAW_MONEY				= 52,
	HEADER_GD_GUILD_WITHDRAW_MONEY_GIVE_REPLY	= 53,

	HEADER_GD_REQUEST_CHARACTER_PRIV	= 54,

	HEADER_GD_SET_EVENT_FLAG			= 55,

	HEADER_GD_PARTY_SET_MEMBER_LEVEL	= 56,

	HEADER_GD_GUILD_WAR_BET		= 57,

	HEADER_GD_CREATE_OBJECT		= 60,
	HEADER_GD_DELETE_OBJECT		= 61,
	HEADER_GD_UPDATE_LAND		= 62,

	HEADER_GD_MARRIAGE_ADD		= 70,
	HEADER_GD_MARRIAGE_UPDATE	= 71,
	HEADER_GD_MARRIAGE_REMOVE	= 72,

	HEADER_GD_WEDDING_REQUEST	= 73,
	HEADER_GD_WEDDING_READY		= 74,
	HEADER_GD_WEDDING_END		= 75,

	HEADER_GD_AUTH_LOGIN		= 100,
	HEADER_GD_LOGIN_BY_KEY		= 101,
	HEADER_GD_MALL_LOAD			= 107,

	HEADER_GD_MYSHOP_PRICELIST_UPDATE	= 108,
	HEADER_GD_MYSHOP_PRICELIST_REQ		= 109,

	HEADER_GD_BLOCK_CHAT				= 110,

	HEADER_GD_RELOAD_ADMIN			= 115,
	HEADER_GD_BREAK_MARRIAGE		= 116,
	HEADER_GD_ELECT_MONARCH			= 117,
	HEADER_GD_CANDIDACY				= 118,
	HEADER_GD_ADD_MONARCH_MONEY		= 119,
	HEADER_GD_TAKE_MONARCH_MONEY	= 120,
	HEADER_GD_COME_TO_VOTE			= 121,
	HEADER_GD_RMCANDIDACY			= 122,
	HEADER_GD_SETMONARCH			= 123,
	HEADER_GD_RMMONARCH				= 124,
	HEADER_GD_DEC_MONARCH_MONEY		= 125,

	HEADER_GD_CHANGE_MONARCH_LORD	= 126,

	HEADER_GD_REQ_CHANGE_GUILD_MASTER	= 129,

	HEADER_GD_REQ_SPARE_ITEM_ID_RANGE	= 130,

	HEADER_GD_UPDATE_HORSE_NAME		= 131,
	HEADER_GD_REQ_HORSE_NAME		= 132,

	HEADER_GD_DC					= 133,

	HEADER_GD_VALID_LOGOUT			= 134,

	HEADER_GD_REQUEST_CHARGE_CASH	= 137,

	HEADER_GD_DELETE_AWARDID		= 138,	// delete gift notify icon

	HEADER_GD_UPDATE_CHANNELSTATUS	= 139,
	HEADER_GD_REQUEST_CHANNELSTATUS	= 140,
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	HEADER_GD_OFFLINESHOP			= 141,
#endif
#if defined(BL_OFFLINE_MESSAGE)
	HEADER_GD_REQUEST_OFFLINE_MESSAGES = 142,
	HEADER_GD_SEND_OFFLINE_MESSAGE = 143,
#endif
#if defined(__SKILL_COLOR_SYSTEM__)
	HEADER_GD_SKILL_COLOR_SAVE		= 145,
#endif
	HEADER_GD_REQUEST_CHANGE_LANGUAGE = 146,
#ifdef ENABLE_EVENT_MANAGER
	HEADER_GD_EVENT_MANAGER			= 212,
#endif
	HEADER_GD_SETUP			= 0xff,

	///////////////////////////////////////////////
	HEADER_DG_NOTICE			= 1,

	HEADER_DG_LOGIN_SUCCESS			= 30,
	HEADER_DG_LOGIN_NOT_EXIST		= 31,
	HEADER_DG_LOGIN_WRONG_PASSWD	= 33,
	HEADER_DG_LOGIN_ALREADY			= 34,

	HEADER_DG_PLAYER_LOAD_SUCCESS	= 35,
	HEADER_DG_PLAYER_LOAD_FAILED	= 36,
	HEADER_DG_PLAYER_CREATE_SUCCESS	= 37,
	HEADER_DG_PLAYER_CREATE_ALREADY	= 38,
	HEADER_DG_PLAYER_CREATE_FAILED	= 39,
	HEADER_DG_PLAYER_DELETE_SUCCESS	= 40,
	HEADER_DG_PLAYER_DELETE_FAILED	= 41,

	HEADER_DG_ITEM_LOAD			= 42,

	HEADER_DG_BOOT				= 43,
	HEADER_DG_QUEST_LOAD		= 44,

	HEADER_DG_SAFEBOX_LOAD					= 45,
	HEADER_DG_SAFEBOX_CHANGE_SIZE			= 46,
	HEADER_DG_SAFEBOX_WRONG_PASSWORD		= 47,
	HEADER_DG_SAFEBOX_CHANGE_PASSWORD_ANSWER = 48,

	HEADER_DG_EMPIRE_SELECT		= 49,

	HEADER_DG_AFFECT_LOAD		= 50,
	HEADER_DG_MALL_LOAD			= 51,

	HEADER_DG_DIRECT_ENTER		= 55,

	HEADER_DG_GUILD_SKILL_UPDATE	= 56,
	HEADER_DG_GUILD_SKILL_RECHARGE	= 57,
	HEADER_DG_GUILD_EXP_UPDATE		= 58,

	HEADER_DG_PARTY_CREATE		= 59,
	HEADER_DG_PARTY_DELETE		= 60,
	HEADER_DG_PARTY_ADD			= 61,
	HEADER_DG_PARTY_REMOVE		= 62,
	HEADER_DG_PARTY_STATE_CHANGE	= 63,
	HEADER_DG_PARTY_HEAL_USE		= 64,
	HEADER_DG_PARTY_SET_MEMBER_LEVEL	= 65,

	HEADER_DG_TIME			= 90,
	HEADER_DG_ITEM_ID_RANGE		= 91,

	HEADER_DG_GUILD_ADD_MEMBER		= 92,
	HEADER_DG_GUILD_REMOVE_MEMBER	= 93,
	HEADER_DG_GUILD_CHANGE_GRADE	= 94,
	HEADER_DG_GUILD_CHANGE_MEMBER_DATA	= 95,
	HEADER_DG_GUILD_DISBAND		= 96,
	HEADER_DG_GUILD_WAR			= 97,
	HEADER_DG_GUILD_WAR_SCORE		= 98,
	HEADER_DG_GUILD_TIME_UPDATE		= 99,
	HEADER_DG_GUILD_LOAD		= 100,
	HEADER_DG_GUILD_LADDER		= 101,
	HEADER_DG_GUILD_SKILL_USABLE_CHANGE	= 102,
	HEADER_DG_GUILD_MONEY_CHANGE	= 103,
	HEADER_DG_GUILD_WITHDRAW_MONEY_GIVE	= 104,

	HEADER_DG_SET_EVENT_FLAG		= 105,

	HEADER_DG_GUILD_WAR_RESERVE_ADD	= 106,
	HEADER_DG_GUILD_WAR_RESERVE_DEL	= 107,
	HEADER_DG_GUILD_WAR_BET		= 108,
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	HEADER_DG_OFFLINESHOP		= 109,
#endif
	HEADER_DG_RELOAD_PROTO		= 120,
	HEADER_DG_CHANGE_NAME		= 121,

	HEADER_DG_AUTH_LOGIN		= 122,

	HEADER_DG_CHANGE_EMPIRE_PRIV	= 124,
	HEADER_DG_CHANGE_GUILD_PRIV		= 125,

	HEADER_DG_MONEY_LOG			= 126,

	HEADER_DG_CHANGE_CHARACTER_PRIV	= 127,

	HEADER_DG_CREATE_OBJECT		= 140,
	HEADER_DG_DELETE_OBJECT		= 141,
	HEADER_DG_UPDATE_LAND		= 142,

	HEADER_DG_MARRIAGE_ADD		= 150,
	HEADER_DG_MARRIAGE_UPDATE		= 151,
	HEADER_DG_MARRIAGE_REMOVE		= 152,

	HEADER_DG_WEDDING_REQUEST		= 153,
	HEADER_DG_WEDDING_READY		= 154,
	HEADER_DG_WEDDING_START		= 155,
	HEADER_DG_WEDDING_END		= 156,

	HEADER_DG_MYSHOP_PRICELIST_RES	= 157,
	HEADER_DG_RELOAD_ADMIN = 158,
	HEADER_DG_BREAK_MARRIAGE = 159,
	HEADER_DG_ELECT_MONARCH			= 160,
	HEADER_DG_CANDIDACY				= 161,
	HEADER_DG_ADD_MONARCH_MONEY		= 162,
	HEADER_DG_TAKE_MONARCH_MONEY	= 163,
	HEADER_DG_COME_TO_VOTE			= 164,
	HEADER_DG_RMCANDIDACY			= 165,
	HEADER_DG_SETMONARCH			= 166,
	HEADER_DG_RMMONARCH			= 167,
	HEADER_DG_DEC_MONARCH_MONEY = 168,

	HEADER_DG_CHANGE_MONARCH_LORD_ACK = 169,
	HEADER_DG_UPDATE_MONARCH_INFO	= 170,

	HEADER_DG_ACK_CHANGE_GUILD_MASTER = 173,

	HEADER_DG_ACK_SPARE_ITEM_ID_RANGE = 174,

	HEADER_DG_UPDATE_HORSE_NAME 	= 175,
	HEADER_DG_ACK_HORSE_NAME		= 176,

	HEADER_DG_NEED_LOGIN_LOG		= 177,

	HEADER_DG_RESULT_CHARGE_CASH	= 179,
	HEADER_DG_ITEMAWARD_INFORMER	= 180,	//gift notify
	HEADER_DG_RESPOND_CHANNELSTATUS	= 181,
#if defined(BL_OFFLINE_MESSAGE)
	HEADER_DG_RESPOND_OFFLINE_MESSAGES = 182,
#endif
#if defined(__SKILL_COLOR_SYSTEM__)
	HEADER_DG_SKILL_COLOR_LOAD		= 186,
#endif
#ifdef ENABLE_EVENT_MANAGER
	HEADER_DG_EVENT_MANAGER			= 212,
#endif
#ifdef __MAINTENANCE__
	HEADER_DG_MAINTENANCE			= 213,
	HEADER_GD_MAINTENANCE			= 213,
#endif
	HEADER_DG_MAP_LOCATIONS			= 0xfe,
	HEADER_DG_P2P					= 0xff,
};

/* ----------------------------------------------
 * table
 * ----------------------------------------------
 */

/* game Server -> DB Server */
#pragma pack(1)
enum ERequestChargeType
{
	ERequestCharge_Cash = 0,
	ERequestCharge_Mileage,
};

#if defined(BL_OFFLINE_MESSAGE)
typedef struct
{
	char 	szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGDReadOfflineMessage;

typedef struct
{
	char	szFrom[CHARACTER_NAME_MAX_LEN + 1];
	char	szMessage[CHAT_MAX_LEN + 1];
} TPacketDGReadOfflineMessage;

typedef struct
{
	char	szFrom[CHARACTER_NAME_MAX_LEN + 1];
	char	szTo[CHARACTER_NAME_MAX_LEN + 1];
	char	szMessage[CHAT_MAX_LEN + 1];
} TPacketGDSendOfflineMessage;
#endif

typedef struct SRequestChargeCash
{
	DWORD		dwAID;		// id(primary key) - Account Table
	DWORD		dwAmount;
	ERequestChargeType	eChargeType;
} TRequestChargeCash;

typedef struct SSimplePlayer
{
	DWORD		dwID;
	char		szName[CHARACTER_NAME_MAX_LEN + 1];
	BYTE		byJob;
	BYTE		byLevel;
	DWORD		dwPlayMinutes;
	BYTE		byST, byHT, byDX, byIQ;
	DWORD		wMainPart; // @fixme502
	BYTE		bChangeName;
	DWORD		wHairPart; // @fixme502
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	DWORD		wAccePart;
#endif
	BYTE		bDummy[4];
	long		x, y;
	long		lAddr;
	WORD		wPort;
	BYTE		skill_group;
#if defined(BL_SORT_LASTPLAYTIME)
	DWORD		dwLastPlayTime;
#endif
	char szPinCode[PIN_CODE_LENGTH + 1];
} TSimplePlayer;

typedef struct SAccountTable
{
	DWORD		id;
	char		login[LOGIN_MAX_LEN + 1];
	char		passwd[PASSWD_MAX_LEN + 1];
	char		social_id[SOCIAL_ID_MAX_LEN + 1];
	char		status[ACCOUNT_STATUS_MAX_LEN + 1];
	BYTE		bEmpire;	
	char hwid[HWID_MAX_NUM + 1];
	BYTE		bLanguage;
	TSimplePlayer	players[PLAYER_PER_ACCOUNT];
#ifdef __MAINTENANCE__
	bool isTester;
#endif
} TAccountTable;

typedef struct SPacketDGCreateSuccess
{
	BYTE		bAccountCharacterIndex;
	TSimplePlayer	player;
} TPacketDGCreateSuccess;

typedef struct TPlayerItemAttribute
{
	BYTE	bType;
	short	sValue;
} TPlayerItemAttribute;

typedef struct SPlayerItem
{
	DWORD	id;
	BYTE	window;
	WORD	pos;
	DWORD	count;

	DWORD	vnum;
	long	alSockets[ITEM_SOCKET_MAX_NUM];

	TPlayerItemAttribute    aAttr[ITEM_ATTRIBUTE_MAX_NUM];

	DWORD	owner;
} TPlayerItem;

typedef struct SQuickslot
{
	DWORD	type;
	DWORD	pos;
} TQuickslot;

typedef struct SPlayerSkill
{
	BYTE	bMasterType;
	BYTE	bLevel;
	time_t	tNextRead;
} TPlayerSkill;

struct	THorseInfo
{
	BYTE	bLevel;
	BYTE	bRiding;
	short	sStamina;
	short	sHealth;
	DWORD	dwHorseHealthDropTime;
};

#ifdef NEW_STATISTICS
enum
{
	STAT_TYPE_REFINE,
	STAT_TYPE_CRAFT,
	STAT_TYPE_FISH,
	STAT_TYPE_DUNGEONS,
	STAT_TYPE_SHOUT,
	STAT_TYPE_ADD_SOCKET,
	STAT_TYPE_CHESTS,
	STAT_TYPE_TRADE,
	STAT_TYPE_MONSTERS,
	STAT_TYPE_BOSSES,
	STAT_TYPE_STONES,
	STAT_TYPE_PC_A,
	STAT_TYPE_PC_B,
	STAT_TYPE_PC_C,
	STAT_TYPE_DUEL,
	STAT_TYPE_DEAD,
	STAT_TYPE_DMG_STONES,
	STAT_TYPE_DMG_HIT_PC,
	STAT_TYPE_DMG_SKILL_PC,
	STAT_TYPE_GUILD_WAR,
	STAT_TYPE_GUILD_KILLS,
	STAT_TYPE_GUILD_FLAGS,
	STAT_TYPE_BATTLEPASS,
	STAT_TYPE_SKILLBOOKS,
	STAT_TYPE_HUNTING_MISSIONS,

	STAT_TYPE_MAX,
};
#endif

#ifdef ENABLE_BATTLE_PASS
enum
{
	MISSION_NONE,
	MISSION_BOSS,
	MISSION_CATCH_FISH,
	MISSION_CRAFT_ITEM,
	MISSION_CRAFT_GAYA,
	MISSION_DESTROY_ITEM,
	MISSION_DUNGEON,
	MISSION_EARN_MONEY,
	MISSION_FEED_PET,
	MISSION_LEVEL_UP,
	MISSION_MONSTER,
	MISSION_MOUNT_TIME,
	MISSION_OPEN_OFFLINESHOP,
	MISSION_PLAYTIME,
	MISSION_REFINE_ITEM,
	MISSION_REFINE_ALCHEMY,
	MISSION_SASH,
	MISSION_SELL_ITEM,
	MISSION_SPEND_MONEY,
	MISSION_SPRITE_STONE,
	MISSION_STONE,
	MISSION_USE_EMOTICON,
	MISSION_WHISPER,
	MISSION_SHOUT_CHAT,
	MISSION_KILLPLAYER,
	MISSION_MAX,

	BATTLE_MISSION_MAX = 10,
	BATTLE_SUB_REWARD = 3,
	BATTLE_REWARD_MAX = 6,
};
#endif

typedef struct SPlayerTable
{
	DWORD	id;

	char	name[CHARACTER_NAME_MAX_LEN + 1];
	char	ip[IP_ADDRESS_LENGTH + 1];

	WORD	job;
	BYTE	voice;

	BYTE	level;
	BYTE	level_step;
	short	st, ht, dx, iq;

	DWORD	exp;
	long long	gold;

#if defined(__GEM_SYSTEM__)
	INT		gem;
#endif

	BYTE	dir;
	INT		x, y, z;
	INT		lMapIndex;

	long	lExitX, lExitY;
	long	lExitMapIndex;

	// @fixme400
	int		hp;
	int		sp;

	short	sRandomHP;
	short	sRandomSP;

	int         playtime;

	short	stat_point;
	short	skill_point;
	short	sub_skill_point;
	short	horse_skill_point;

	TPlayerSkill skills[SKILL_MAX_NUM];

	TQuickslot  quickslot[QUICKSLOT_MAX_NUM];

	BYTE	part_base;
	DWORD	parts[PART_MAX_NUM]; // @fixme502

	short	stamina;

	BYTE	skill_group;
	long	lAlignment;

	short	stat_reset_count;

	THorseInfo	horse;

	DWORD	logoff_interval;

	int		aiPremiumTimes[PREMIUM_MAX_NUM];
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	long long	shopFlag;
#endif
#ifdef ENABLE_BATTLE_PASS
	long long	battlePassData[MISSION_MAX];
#endif
#ifdef NEW_STATISTICS
	int64_t	statsData[STAT_TYPE_MAX];
#endif
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	int		iNormalInventory;
	int		iSkillbookInventory;
	int		iMaterialInventory;
	int		iStoneInventory;
	int		iChestInventory;
	int		iEnchantInventory;
	int		iCostumeInventory;
#endif
	char pin[PIN_CODE_LENGTH + 1];
} TPlayerTable;

typedef struct SMobSkillLevel
{
	DWORD	dwVnum;
	BYTE	bLevel;
} TMobSkillLevel;

typedef struct SEntityTable
{
	DWORD dwVnum;
} TEntityTable;

typedef struct SMobTable : public SEntityTable
{
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	char	szLocaleName[CHARACTER_NAME_MAX_LEN + 1];

	BYTE	bType;			// Monster, NPC
	BYTE	bRank;			// PAWN, KNIGHT, KING
	BYTE	bBattleType;		// MELEE, etc..
	BYTE	bLevel;			// Level
	BYTE	bSize;

	DWORD	dwGoldMin;
	DWORD	dwGoldMax;
	DWORD	dwExp;
	DWORD	dwMaxHP;
	BYTE	bRegenCycle;
	BYTE	bRegenPercent;
	WORD	wDef;

	DWORD	dwAIFlag;
	DWORD	dwRaceFlag;
	DWORD	dwImmuneFlag;

	BYTE	bStr, bDex, bCon, bInt;
	DWORD	dwDamageRange[2];

	short	sAttackSpeed;
	short	sMovingSpeed;
	BYTE	bAggresiveHPPct;
	WORD	wAggressiveSight;
	WORD	wAttackRange;

	char	cEnchants[MOB_ENCHANTS_MAX_NUM];
	char	cResists[MOB_RESISTS_MAX_NUM];
	char	cElements[MOB_ELEMENT_MAX_NUM];

	DWORD	dwResurrectionVnum;
	DWORD	dwDropItemVnum;

	BYTE	bMountCapacity;
	BYTE	bOnClickType;

	BYTE	bEmpire;
	char	szFolder[64 + 1];

	float	fDamMultiply;

	DWORD	dwSummonVnum;
	DWORD	dwDrainSP;
	DWORD	dwMobColor;
	DWORD	dwPolymorphItemVnum;

	TMobSkillLevel Skills[MOB_SKILL_MAX_NUM];

	BYTE	bBerserkPoint;
	BYTE	bStoneSkinPoint;
	BYTE	bGodSpeedPoint;
	BYTE	bDeathBlowPoint;
	BYTE	bRevivePoint;
} TMobTable;

typedef struct SSkillTable
{
	DWORD	dwVnum;
	char	szName[32 + 1];
	BYTE	bType;
	BYTE	bMaxLevel;
	DWORD	dwSplashRange;

	char	szPointOn[64];
	char	szPointPoly[100 + 1];
	char	szSPCostPoly[100 + 1];
	char	szDurationPoly[100 + 1];
	char	szDurationSPCostPoly[100 + 1];
	char	szCooldownPoly[100 + 1];
	char	szMasterBonusPoly[100 + 1];
	//char	szAttackGradePoly[100 + 1];
	char	szGrandMasterAddSPCostPoly[100 + 1];
	DWORD	dwFlag;
	DWORD	dwAffectFlag;

	// Data for secondary skill
	char 	szPointOn2[64];
	char 	szPointPoly2[100 + 1];
	char 	szDurationPoly2[100 + 1];
	DWORD 	dwAffectFlag2;

	// Data for grand master point
	char 	szPointOn3[64];
	char 	szPointPoly3[100 + 1];
	char 	szDurationPoly3[100 + 1];

	BYTE	bLevelStep;
	BYTE	bLevelLimit;
	DWORD	preSkillVnum;
	BYTE	preSkillLevel;

	long	lMaxHit;
	char	szSplashAroundDamageAdjustPoly[100 + 1];

	BYTE	bSkillAttrType;

	DWORD	dwTargetRange;
} TSkillTable;

typedef struct SShopItemTable
{
	DWORD		vnum;
	DWORD		count;

    TItemPos	pos;
	DWORD		price;
#if defined(__SHOPEX_RENEWAL__)
	DWORD		price1;
	DWORD		price2;
	DWORD		price3;
	DWORD		price4;
#endif
	BYTE		display_pos;
#if defined(__SHOPEX_RENEWAL__)
	long alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
	DWORD price_type;
	DWORD price_vnum;
	DWORD price_vnum1;
	DWORD price_vnum2;
	DWORD price_vnum3;
	DWORD price_vnum4;
	SShopItemTable() : price_type(1), price_vnum(0), price_vnum1(0), price_vnum2(0), price_vnum3(0), price_vnum4(0)
	{
		memset(&alSockets, 0, sizeof(alSockets));
		memset(&aAttr, 0, sizeof(aAttr));
	}
#endif
} TShopItemTable;

typedef struct SShopTable
{
	DWORD		dwVnum;
	DWORD		dwNPCVnum;

	DWORD		byItemCount;
	TShopItemTable	items[SHOP_HOST_ITEM_MAX_NUM];
} TShopTable;


#if defined(__SHOPEX_RENEWAL__)
enum STableExTypes// : decltype(TShopItemTable::price_type)
{
	EX_GOLD = 1,
	EX_SECONDARY,
	EX_ITEM,
	EX_EXP,
	EX_MAX
};
#endif

#define QUEST_NAME_MAX_LEN	32
#define QUEST_STATE_MAX_LEN	64

typedef struct SQuestTable
{
	DWORD		dwPID;
	char		szName[QUEST_NAME_MAX_LEN + 1];
	char		szState[QUEST_STATE_MAX_LEN + 1];
	long		lValue;
} TQuestTable;

typedef struct SItemLimit
{
	BYTE	bType;
	long	lValue;
} TItemLimit;

typedef struct SItemApply
{
	BYTE	bType;
	long	lValue;
} TItemApply;

typedef struct SItemTable : public SEntityTable
{
	DWORD		dwVnumRange;
	char        szName[ITEM_NAME_MAX_LEN + 1];
	char	szLocaleName[ITEM_NAME_MAX_LEN + 1];
	BYTE	bType;
	BYTE	bSubType;

	BYTE        bWeight;
	BYTE	bSize;

	DWORD	dwAntiFlags;
	DWORD	dwFlags;
	DWORD	dwWearFlags;
	DWORD	dwImmuneFlag;

	DWORD       dwGold;
	DWORD       dwShopBuyPrice;

	TItemLimit	aLimits[ITEM_LIMIT_MAX_NUM];
	TItemApply	aApplies[ITEM_APPLY_MAX_NUM];
	long        alValues[ITEM_VALUES_MAX_NUM];
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	DWORD	dwRefinedVnum;
	WORD	wRefineSet;
	BYTE	bAlterToMagicItemPct;
	BYTE	bSpecular;
	BYTE	bGainSocketPct;

	short int	sAddonType;

	char		cLimitRealTimeFirstUseIndex;
	char		cLimitTimerBasedOnWearIndex;
} TItemTable;

struct TItemAttrTable
{
	TItemAttrTable() :
		dwApplyIndex(0),
		dwProb(0)
	{
		szApply[0] = 0;
		memset(&lValues, 0, sizeof(lValues));
		memset(&bMaxLevelBySet, 0, sizeof(bMaxLevelBySet));
	}

	char    szApply[APPLY_NAME_MAX_LEN + 1];
	DWORD   dwApplyIndex;
	DWORD   dwProb;
	long    lValues[ITEM_ATTRIBUTE_MAX_LEVEL];
	BYTE    bMaxLevelBySet[ATTRIBUTE_SET_MAX_NUM];
};

typedef struct SConnectTable
{
	char	login[LOGIN_MAX_LEN + 1];
	IDENT	ident;
} TConnectTable;

typedef struct SLoginPacket
{
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
} TLoginPacket;

typedef struct SPlayerLoadPacket
{
	DWORD	account_id;
	DWORD	player_id;
	BYTE	account_index;
} TPlayerLoadPacket;

typedef struct SPlayerCreatePacket
{
	char		login[LOGIN_MAX_LEN + 1];
	char		passwd[PASSWD_MAX_LEN + 1];
	DWORD		account_id;
	BYTE		account_index;
	TPlayerTable	player_table;
} TPlayerCreatePacket;

typedef struct SPlayerDeletePacket
{
	char	login[LOGIN_MAX_LEN + 1];
	DWORD	player_id;
	BYTE	account_index;
	//char	name[CHARACTER_NAME_MAX_LEN + 1];
	char	private_code[8];
} TPlayerDeletePacket;

typedef struct SLogoutPacket
{
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
} TLogoutPacket;

typedef struct SPlayerCountPacket
{
	DWORD	dwCount;
} TPlayerCountPacket;

#define SAFEBOX_MAX_NUM			135
#define SAFEBOX_PASSWORD_MAX_LEN	6

typedef struct SSafeboxTable
{
	DWORD	dwID;
	BYTE	bSize;
	DWORD	dwGold;
	WORD	wItemCount;
} TSafeboxTable;

typedef struct SSafeboxChangeSizePacket
{
	DWORD	dwID;
	BYTE	bSize;
} TSafeboxChangeSizePacket;

typedef struct SSafeboxLoadPacket
{
	DWORD	dwID;
	char	szLogin[LOGIN_MAX_LEN + 1];
	char	szPassword[SAFEBOX_PASSWORD_MAX_LEN + 1];
} TSafeboxLoadPacket;

typedef struct SSafeboxChangePasswordPacket
{
	DWORD	dwID;
	char	szOldPassword[SAFEBOX_PASSWORD_MAX_LEN + 1];
	char	szNewPassword[SAFEBOX_PASSWORD_MAX_LEN + 1];
} TSafeboxChangePasswordPacket;

typedef struct SSafeboxChangePasswordPacketAnswer
{
	BYTE	flag;
} TSafeboxChangePasswordPacketAnswer;

typedef struct SEmpireSelectPacket
{
	DWORD	dwAccountID;
	BYTE	bEmpire;
} TEmpireSelectPacket;

typedef struct SPacketGDSetup
{
	char	szPublicIP[16];	// Public IP which listen to users
	BYTE	bChannel;
	WORD	wListenPort;
	WORD	wP2PPort;
	long	alMaps[MAP_ALLOW_LIMIT];
	DWORD	dwLoginCount;
	BYTE	bAuthServer;
} TPacketGDSetup;

typedef struct SPacketDGMapLocations
{
	BYTE	bCount;
} TPacketDGMapLocations;

typedef struct SMapLocation
{
	long	alMaps[MAP_ALLOW_LIMIT];
	char	szHost[MAX_HOST_LENGTH + 1];
	WORD	wPort;
#ifdef ENABLE_MOVE_CHANNEL
	BYTE	channel;
#endif
} TMapLocation;

typedef struct SPacketDGP2P
{
	char	szHost[MAX_HOST_LENGTH + 1];
	WORD	wPort;
	BYTE	bChannel;
} TPacketDGP2P;

typedef struct SPacketGDDirectEnter
{
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
	BYTE	index;
} TPacketGDDirectEnter;

typedef struct SPacketDGDirectEnter
{
	TAccountTable accountTable;
	TPlayerTable playerTable;
} TPacketDGDirectEnter;

typedef struct SPacketGuildSkillUpdate
{
	DWORD guild_id;
	int amount;
	BYTE skill_levels[12];
	BYTE skill_point;
	BYTE save;
} TPacketGuildSkillUpdate;

typedef struct SPacketGuildExpUpdate
{
	DWORD guild_id;
	int amount;
} TPacketGuildExpUpdate;

typedef struct SPacketGuildChangeMemberData
{
	DWORD guild_id;
	DWORD pid;
	DWORD offer;
	BYTE level;
	BYTE grade;
} TPacketGuildChangeMemberData;

typedef struct SPacketDGLoginAlready
{
	char	szLogin[LOGIN_MAX_LEN + 1];
} TPacketDGLoginAlready;

typedef struct TPacketAffectElement
{
	DWORD	dwType;
	BYTE	bApplyOn;
	long	lApplyValue;
	DWORD	dwFlag;
	long	lDuration;
	long	lSPCost;
} TPacketAffectElement;

typedef struct SPacketGDAddAffect
{
	DWORD			dwPID;
	TPacketAffectElement	elem;
} TPacketGDAddAffect;

typedef struct SPacketGDRemoveAffect
{
	DWORD	dwPID;
	DWORD	dwType;
	BYTE	bApplyOn;
} TPacketGDRemoveAffect;

typedef struct SPacketGDHighscore
{
	DWORD	dwPID;
	long	lValue;
	char	cDir;
	char	szBoard[21];
} TPacketGDHighscore;

typedef struct SPacketPartyCreate
{
	DWORD	dwLeaderPID;
} TPacketPartyCreate;

typedef struct SPacketPartyDelete
{
	DWORD	dwLeaderPID;
} TPacketPartyDelete;

typedef struct SPacketPartyAdd
{
	DWORD	dwLeaderPID;
	DWORD	dwPID;
	BYTE	bState;
} TPacketPartyAdd;

typedef struct SPacketPartyRemove
{
	DWORD	dwLeaderPID;
	DWORD	dwPID;
} TPacketPartyRemove;

typedef struct SPacketPartyStateChange
{
	DWORD	dwLeaderPID;
	DWORD	dwPID;
	BYTE	bRole;
	BYTE	bFlag;
} TPacketPartyStateChange;

typedef struct SPacketPartySetMemberLevel
{
	DWORD	dwLeaderPID;
	DWORD	dwPID;
	BYTE	bLevel;
} TPacketPartySetMemberLevel;

typedef struct SPacketGDBoot
{
    DWORD	dwItemIDRange[2];
	char	szIP[16];
} TPacketGDBoot;

typedef struct SPacketGuild
{
	DWORD	dwGuild;
	DWORD	dwInfo;
} TPacketGuild;

typedef struct SPacketGDGuildAddMember
{
	DWORD	dwPID;
	DWORD	dwGuild;
	BYTE	bGrade;
} TPacketGDGuildAddMember;

typedef struct SPacketDGGuildMember
{
	DWORD	dwPID;
	DWORD	dwGuild;
	BYTE	bGrade;
	BYTE	isGeneral;
	BYTE	bJob;
	BYTE	bLevel;
	DWORD	dwOffer;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketDGGuildMember;

typedef struct SPacketGuildWar
{
	BYTE	bType;
	BYTE	bWar;
	DWORD	dwGuildFrom;
	DWORD	dwGuildTo;
	long	lWarPrice;
	long	lInitialScore;
} TPacketGuildWar;

typedef struct SPacketGuildWarScore
{
	DWORD dwGuildGainPoint;
	DWORD dwGuildOpponent;
	long lScore;
	long lBetScore;
} TPacketGuildWarScore;

typedef struct SRefineMaterial
{
	DWORD vnum;
	int count;
} TRefineMaterial;

typedef struct SRefineTable
{
	//DWORD src_vnum;
	//DWORD result_vnum;
	DWORD id;
	BYTE material_count;
	int cost;
	int prob;
	TRefineMaterial materials[REFINE_MATERIAL_MAX_NUM];
} TRefineTable;

typedef struct SBanwordTable
{
	char szWord[BANWORD_MAX_LEN + 1];
} TBanwordTable;

typedef struct SPacketGDChangeName
{
	DWORD pid;
	char name[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGDChangeName;

typedef struct SPacketDGChangeName
{
	DWORD pid;
	char name[CHARACTER_NAME_MAX_LEN + 1];
} TPacketDGChangeName;

typedef struct SPacketGuildLadder
{
	DWORD dwGuild;
	long lLadderPoint;
	long lWin;
	long lDraw;
	long lLoss;
} TPacketGuildLadder;

typedef struct SPacketGuildLadderPoint
{
	DWORD dwGuild;
	long lChange;
} TPacketGuildLadderPoint;

typedef struct SPacketGuildUseSkill
{
	DWORD dwGuild;
	DWORD dwSkillVnum;
	DWORD dwCooltime;
} TPacketGuildUseSkill;

typedef struct SPacketGuildSkillUsableChange
{
	DWORD dwGuild;
	DWORD dwSkillVnum;
	BYTE bUsable;
} TPacketGuildSkillUsableChange;

typedef struct SPacketGDLoginKey
{
	DWORD dwAccountID;
	DWORD dwLoginKey;
} TPacketGDLoginKey;

typedef struct SPacketGDAuthLogin
{
	DWORD	dwID;
	DWORD	dwLoginKey;
	char	szLogin[LOGIN_MAX_LEN + 1];
	char	szSocialID[SOCIAL_ID_MAX_LEN + 1];
	DWORD	adwClientKey[4];
	int		iPremiumTimes[PREMIUM_MAX_NUM];
	char szHWID[HWID_MAX_NUM + 1];
	BYTE	bLanguage;
#ifdef __MAINTENANCE__
	bool isTester;
#endif
} TPacketGDAuthLogin;

typedef struct SPacketGDLoginByKey
{
	char	szLogin[LOGIN_MAX_LEN + 1];
	DWORD	dwLoginKey;
	DWORD	adwClientKey[4];
	char	szIP[MAX_HOST_LENGTH + 1];
} TPacketGDLoginByKey;

typedef struct SPacketGiveGuildPriv
{
	BYTE type;
	int value;
	DWORD guild_id;
	time_t duration_sec;
} TPacketGiveGuildPriv;
typedef struct SPacketGiveEmpirePriv
{
	BYTE type;
	int value;
	BYTE empire;
	time_t duration_sec;
} TPacketGiveEmpirePriv;
typedef struct SPacketGiveCharacterPriv
{
	BYTE type;
	int value;
	DWORD pid;
} TPacketGiveCharacterPriv;
typedef struct SPacketRemoveGuildPriv
{
	BYTE type;
	DWORD guild_id;
} TPacketRemoveGuildPriv;
typedef struct SPacketRemoveEmpirePriv
{
	BYTE type;
	BYTE empire;
} TPacketRemoveEmpirePriv;

#if defined(__SKILL_COLOR_SYSTEM__)
typedef struct
{
	DWORD dwPlayerID;
	DWORD dwSkillColor[ESkillColorLength::MAX_SKILL_COUNT + ESkillColorLength::MAX_BUFF_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
} TSkillColor;
#endif

typedef struct SRequestChangeLanguage
{
	DWORD dwAID;
	BYTE bLanguage;
} TRequestChangeLanguage;

typedef struct SRequestChangePin
{
	char pinCode[PIN_CODE_LENGTH + 1];
} TRequestChangePin;

typedef struct SPacketDGChangeCharacterPriv
{
	BYTE type;
	int value;
	DWORD pid;
	BYTE bLog;
} TPacketDGChangeCharacterPriv;

typedef struct SPacketDGChangeGuildPriv
{
	BYTE type;
	int value;
	DWORD guild_id;
	BYTE bLog;
	time_t end_time_sec;
} TPacketDGChangeGuildPriv;

typedef struct SPacketDGChangeEmpirePriv
{
	BYTE type;
	int value;
	BYTE empire;
	BYTE bLog;
	time_t end_time_sec;
} TPacketDGChangeEmpirePriv;

typedef struct SPacketMoneyLog
{
	BYTE type;
	DWORD vnum;
	long long gold;
} TPacketMoneyLog;

typedef struct SPacketGDGuildMoney
{
	DWORD dwGuild;
	INT iGold;
} TPacketGDGuildMoney;

typedef struct SPacketDGGuildMoneyChange
{
	DWORD dwGuild;
	INT iTotalGold;
} TPacketDGGuildMoneyChange;

typedef struct SPacketDGGuildMoneyWithdraw
{
	DWORD dwGuild;
	INT iChangeGold;
} TPacketDGGuildMoneyWithdraw;

typedef struct SPacketGDGuildMoneyWithdrawGiveReply
{
	DWORD dwGuild;
	INT iChangeGold;
	BYTE bGiveSuccess;
} TPacketGDGuildMoneyWithdrawGiveReply;

typedef struct SPacketSetEventFlag
{
	char	szFlagName[EVENT_FLAG_NAME_MAX_LEN + 1];
	long	lValue;
} TPacketSetEventFlag;

typedef struct SPacketLoginOnSetup
{
	DWORD   dwID;
	char    szLogin[LOGIN_MAX_LEN + 1];
	char    szSocialID[SOCIAL_ID_MAX_LEN + 1];
	char    szHost[MAX_HOST_LENGTH + 1];
	DWORD   dwLoginKey;
	DWORD   adwClientKey[4];
	char szHWID[HWID_MAX_NUM + 1];
	BYTE	bLanguage;
#ifdef __MAINTENANCE__
	bool isTester;
#endif
} TPacketLoginOnSetup;

typedef struct SPacketGDCreateObject
{
	DWORD	dwVnum;
	DWORD	dwLandID;
	INT		lMapIndex;
	INT	 	x, y;
	float	xRot;
	float	yRot;
	float	zRot;
} TPacketGDCreateObject;

typedef struct SGuildReserve
{
	DWORD       dwID;
	DWORD       dwGuildFrom;
	DWORD       dwGuildTo;
	DWORD       dwTime;
	BYTE        bType;
	long        lWarPrice;
	long        lInitialScore;
	bool        bStarted;
	DWORD	dwBetFrom;
	DWORD	dwBetTo;
	long	lPowerFrom;
	long	lPowerTo;
	long	lHandicap;
} TGuildWarReserve;

typedef struct SPacketGDGuildWarBet
{
	DWORD	dwWarID;
	char	szLogin[LOGIN_MAX_LEN + 1];
	DWORD	dwGold;
	DWORD	dwGuild;
} TPacketGDGuildWarBet;

// Marriage

typedef struct SPacketMarriageAdd
{
	DWORD dwPID1;
	DWORD dwPID2;
	time_t tMarryTime;
	char szName1[CHARACTER_NAME_MAX_LEN + 1];
	char szName2[CHARACTER_NAME_MAX_LEN + 1];
} TPacketMarriageAdd;

typedef struct SPacketMarriageUpdate
{
	DWORD dwPID1;
	DWORD dwPID2;
	INT  iLovePoint;
	BYTE  byMarried;
} TPacketMarriageUpdate;

typedef struct SPacketMarriageRemove
{
	DWORD dwPID1;
	DWORD dwPID2;
} TPacketMarriageRemove;

typedef struct SPacketWeddingRequest
{
	DWORD dwPID1;
	DWORD dwPID2;
} TPacketWeddingRequest;

typedef struct SPacketWeddingReady
{
	DWORD dwPID1;
	DWORD dwPID2;
	DWORD dwMapIndex;
} TPacketWeddingReady;

typedef struct SPacketWeddingStart
{
	DWORD dwPID1;
	DWORD dwPID2;
} TPacketWeddingStart;

typedef struct SPacketWeddingEnd
{
	DWORD dwPID1;
	DWORD dwPID2;
} TPacketWeddingEnd;

typedef struct SPacketMyshopPricelistHeader
{
	DWORD	dwOwnerID;
	BYTE	byCount;
} TPacketMyshopPricelistHeader;

typedef struct SItemPriceInfo
{
	DWORD	dwVnum;
	DWORD	dwPrice;
} TItemPriceInfo;

typedef struct SItemPriceListTable
{
	DWORD	dwOwnerID;
	BYTE	byCount;

	TItemPriceInfo	aPriceInfo[SHOP_PRICELIST_MAX_NUM];
} TItemPriceListTable;

typedef struct SPacketBlockChat
{
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	long lDuration;
} TPacketBlockChat;

//ADMIN_MANAGER
typedef struct TAdminInfo
{
	int m_ID;
	char m_szAccount[32];
	char m_szName[32];
	char m_szContactIP[16];
	char m_szServerIP[16];
	int m_Authority;
} tAdminInfo;
//END_ADMIN_MANAGER

//BOOT_LOCALIZATION
struct tLocale
{
	char szValue[32];
	char szKey[32];
};
//BOOT_LOCALIZATION

//RELOAD_ADMIN
typedef struct SPacketReloadAdmin
{
	char szIP[16];
} TPacketReloadAdmin;
//END_RELOAD_ADMIN

typedef struct TMonarchInfo
{
	DWORD pid[4];
	int64_t money[4];
	char name[4][32];
	char date[4][32];
} MonarchInfo;

typedef struct TMonarchElectionInfo
{
	DWORD pid;
	DWORD selectedpid;
	char date[32];
} MonarchElectionInfo;

typedef struct tMonarchCandidacy
{
	DWORD pid;
	char name[32];
	char date[32];
} MonarchCandidacy;

typedef struct tChangeMonarchLord
{
	BYTE bEmpire;
	DWORD dwPID;
} TPacketChangeMonarchLord;

typedef struct tChangeMonarchLordACK
{
	BYTE bEmpire;
	DWORD dwPID;
	char szName[32];
	char szDate[32];
} TPacketChangeMonarchLordACK;

typedef struct tChangeGuildMaster
{
	DWORD dwGuildID;
	DWORD idFrom;
	DWORD idTo;
} TPacketChangeGuildMaster;

typedef struct tItemIDRange
{
	DWORD dwMin;
	DWORD dwMax;
	DWORD dwUsableItemIDMin;
} TItemIDRangeTable;

typedef struct tUpdateHorseName
{
	DWORD dwPlayerID;
	char szHorseName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketUpdateHorseName;

typedef struct tDC
{
	char	login[LOGIN_MAX_LEN + 1];
} TPacketDC;

typedef struct tNeedLoginLogInfo
{
	DWORD dwPlayerID;
	BYTE bLanguage;
#ifdef __MAINTENANCE__
	bool isTester;
#endif
} TPacketNeedLoginLogInfo;

typedef struct tItemAwardInformer
{
	char	login[LOGIN_MAX_LEN + 1];
	char	command[20];
	unsigned int vnum;
} TPacketItemAwardInfromer;

typedef struct tDeleteAwardID
{
	DWORD dwID;
} TPacketDeleteAwardID;

typedef struct SChannelStatus
{
	WORD nPort; // backward change for client @fixme024
	BYTE bStatus;
} TChannelStatus;

#if defined(__MESSENGER_BLOCK_SYSTEM__)
enum MessengerBlock
{
	MESSENGER_BLOCK,
	MESSENGER_FRIEND
};
#endif

#ifdef ENABLE_SWITCHBOT
struct TSwitchbotAttributeAlternativeTable
{
	TPlayerItemAttribute attributes[MAX_NORM_ATTR_NUM];

	bool IsConfigured() const
	{
		for (const auto& it : attributes)
		{
			if (it.bType && it.sValue)
			{
				return true;
			}
		}

		return false;
	}
};

struct TSwitchbotTable
{
	DWORD player_id;
	bool active[SWITCHBOT_SLOT_COUNT];
	bool finished[SWITCHBOT_SLOT_COUNT];
	DWORD items[SWITCHBOT_SLOT_COUNT];
	TSwitchbotAttributeAlternativeTable alternatives[SWITCHBOT_SLOT_COUNT][SWITCHBOT_ALTERNATIVE_COUNT];

	TSwitchbotTable() : player_id(0)
	{
		memset(&items, 0, sizeof(items));
		memset(&alternatives, 0, sizeof(alternatives));
		memset(&active, false, sizeof(active));
		memset(&finished, false, sizeof(finished));
	}
};

struct TSwitchbottAttributeTable
{
	BYTE attribute_set;
	int apply_num;
	long max_value;
};
#endif

#ifdef ENABLE_OFFLINESHOP_SYSTEM
typedef struct offline_shop_item
{
	DWORD		id;
	DWORD		owner_id;
	BYTE		pos;
	int			count;
	long long	price;
	DWORD		vnum;
	long		alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute	aAttr[ITEM_ATTRIBUTE_MAX_NUM];
	BYTE		status;
	char 		szBuyerName[CHARACTER_NAME_MAX_LEN+1];
	char 		szOwnerName[CHARACTER_NAME_MAX_LEN+1];
} OFFLINE_SHOP_ITEM;
typedef struct ShopLog {
	char	name[CHARACTER_NAME_MAX_LEN+1];
	char	date[25];
	DWORD	itemVnum;
	int	itemCount;
	long long	price;
}TShopLog;
typedef struct SShopDecoration {
	char	sign[SHOP_SIGN_MAX_LEN + 1];
	DWORD	owner_id;
	BYTE	vnum;
	BYTE	type;
}TShopDecoration;
typedef struct SGDOfflineShopCreate
{
	DWORD	owner_id;
	char	owner_name[CHARACTER_NAME_MAX_LEN+1];
	char	sign[SHOP_SIGN_MAX_LEN + 1];
	long	x;
	long	y;
	long	z;
	DWORD	mapindex;
	DWORD	type;
	BYTE	channel;
	long long	slotflag;
	OFFLINE_SHOP_ITEM items[OFFLINE_SHOP_HOST_ITEM_MAX_NUM];
	long long	price;
	ShopLog		log[OFFLINE_SHOP_HOST_ITEM_MAX_NUM];
	int		time;
} TOfflineShop;
typedef struct SOfflineShopBuy
{
	OFFLINE_SHOP_ITEM	item;
	DWORD	customer_id;
	char	customer_name[CHARACTER_NAME_MAX_LEN + 1];
	ShopLog	log;
	int		log_index;
}TOfflineShopBuy;
typedef struct SOfflineShopChangeTitle
{
	DWORD	owner_id;
	char	sign[SHOP_SIGN_MAX_LEN + 1];
}TOfflineShopChangeTitle;
typedef struct SOfflineShopItemTable
{
	DWORD		vnum;
	int			count;
	TItemPos	pos;
	long long		price;
	BYTE		display_pos;
} TOfflineShopItemTable;
typedef struct SOfflineShopAddItem
{
	TItemPos	bPos;
	BYTE		bDisplayPos;
	long long		lPrice;
} TOfflineShopAddItem;
typedef struct SOfflineShopOpenSlot
{
	DWORD	owner_id;
	long long	flag;
} TOfflineShopOpenSlot;
typedef struct SOfflineShopBackItem
{
	DWORD	owner_id;
	OFFLINE_SHOP_ITEM items[OFFLINE_SHOP_HOST_ITEM_MAX_NUM];
} TOfflineShopBackItem;
enum {
	// db
	CREATE_OFFLINESHOP,
	DESTROY_OFFLINESHOP,
	REMOVE_ITEM,
	ADD_ITEM,
	BUY_ITEM,
	WITHDRAW_MONEY,
	CHANGE_TITLE,
	CLEAR_LOG,
	CHANGE_DECORATION,
	CHANGE_OPEN_SLOT,
	TIME_DONE,
	GET_BACK_ITEM,
	ADD_TIME,
	ADD_TIME_PREMIUM,
	MOVE_ITEM,
	REMOVE_ALL_ITEM,
	CHANGE_PRICE,

};
struct move_item {
	BYTE subheader;
	DWORD ownerID;
	WORD slotPos;
	WORD targetPos;
};
struct remove_all {
	BYTE subheader;
	DWORD ownerID;
};
struct change_price {
	BYTE subheader;
	DWORD ownerID;
	WORD bPos;
	long long itemPrice;
};
struct shop_decoration {
	BYTE subheader;
	TShopDecoration decoration;
};
struct shop_create {
	BYTE subheader;
	TOfflineShop offlineshop;
};
struct shop_buy {
	BYTE subheader;
	TOfflineShopBuy buyItem;
};
struct shop_title {
	BYTE subheader;
	TOfflineShopChangeTitle title;
};
struct shop_owner {
	BYTE subheader;
	DWORD owner_id;
};
struct shop_item {
	BYTE subheader;
	OFFLINE_SHOP_ITEM item;
};
struct shop_slot {
	BYTE subheader;
	TOfflineShopOpenSlot ch;
};
struct shop_back {
	BYTE subheader;
	TOfflineShopBackItem back;
};
#endif

#ifdef ENABLE_MULTI_FARM_BLOCK
typedef struct SMultiFarm
{
	DWORD	playerID;
	bool	farmStatus;
	BYTE	affectType;
	int		affectTime;
	char	playerName[CHARACTER_NAME_MAX_LEN+1];
	SMultiFarm(DWORD id_, const char* playerName_, bool status_, BYTE type_, int time_) : playerID(id_), farmStatus(status_), affectType(type_), affectTime(time_){
		strlcpy(playerName, playerName_, sizeof(playerName));
	}
}TMultiFarm;
#endif

#ifdef RENEWAL_MISSION_BOOKS
typedef struct SMissionBookData
{
	WORD	id;
	DWORD	missionItemIndex;
	BYTE type;
	DWORD subtype;
	BYTE levelRange;
	long long max;
	int maxTime;
	long long gold[2];
	DWORD exp[2];
	DWORD rewardItems[6];
	WORD rewardCount[6];
} TMissionBookData;
typedef struct SMissionBook
{
	long long value;
	int	leftTime;
	bool reward;
	int endTime;
}TMissionBook;
enum
{
	MISSION_BOOK_MAX = 10,

	MISSION_BOOK_TYPE_MONSTER = 0,
	MISSION_BOOK_TYPE_METINSTONE = 1,
	MISSION_BOOK_TYPE_BOSS = 2,

};
#endif

#ifdef ENABLE_EVENT_MANAGER
typedef struct event_struct_
{
	WORD	eventID;
	BYTE	eventIndex;
	int		startTime;
	int		endTime;
	BYTE	empireFlag;
	BYTE	channelFlag;
	DWORD	value[4];
	bool	eventStatus;
	bool	eventTypeOnlyStart;
	char	startTimeText[25];
	char	endTimeText[25];
}TEventManagerData;
enum
{
	EVENT_MANAGER_LOAD,
	EVENT_MANAGER_EVENT_STATUS,
	EVENT_MANAGER_REMOVE_EVENT,
	EVENT_MANAGER_UPDATE,

	EXP_EVENT = 1,
	ITEM_DROP_EVENT,
	YANG_DROP_EVENT,
	BONUS_EVENT,
	DOUBLE_BOSS_LOOT_EVENT,
	DOUBLE_METIN_LOOT_EVENT,
	DUNGEON_TICKET_LOOT_EVENT,
	MOONLIGHT_EVENT,
	WHEEL_OF_FORTUNE_EVENT,
	HALF_TIMER_DUNGEONS_EVENT,
	OKEY_EVENT,
	GOLD_MANNY_EVENT,
};
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
enum EPetOptions
{
	EPetOption_Followable = 1 << 0,
	EPetOption_Mountable = 1 << 1,
	EPetOption_Summonable = 1 << 2,
	EPetOption_Combatable = 1 << 3,
};
enum m_petMisc {
	NORMAL_PET,
	NORMAL_LEVEL_PET,
};
enum EPetPointTypes
{
	POINT_PET_DURATION = 0,
	POINT_PET_LEVEL = 1,
	POINT_PET_EXP = 2,
	POINT_PET_EVOLVE = 3,
	POINT_PET_SKILLCOUNT = 4,

	POINT_PET_BONUS_1 = 5,
	POINT_PET_BONUS_2 = 6,
	POINT_PET_BONUS_3 = 7,
	

	POINT_PET_SKILL_INDEX_1 = 100,
	POINT_PET_SKILL_INDEX_2,
	POINT_PET_SKILL_INDEX_3,
	POINT_PET_SKILL_INDEX_4,
	POINT_PET_SKILL_INDEX_5,
	POINT_PET_SKILL_INDEX_6,
	POINT_PET_SKILL_INDEX_7,
	POINT_PET_SKILL_INDEX_8,
	POINT_PET_SKILL_INDEX_9,
	POINT_PET_SKILL_INDEX_10,
	POINT_PET_SKILL_INDEX_11,
	POINT_PET_SKILL_INDEX_12,
	POINT_PET_SKILL_INDEX_13,
	POINT_PET_SKILL_INDEX_14,
	POINT_PET_SKILL_INDEX_15 = 114,

	POINT_PET_SKILL_LEVEL_1 = 150,
	POINT_PET_SKILL_LEVEL_2,
	POINT_PET_SKILL_LEVEL_3,
	POINT_PET_SKILL_LEVEL_4,
	POINT_PET_SKILL_LEVEL_5,
	POINT_PET_SKILL_LEVEL_6,
	POINT_PET_SKILL_LEVEL_7,
	POINT_PET_SKILL_LEVEL_8,
	POINT_PET_SKILL_LEVEL_9,
	POINT_PET_SKILL_LEVEL_10,
	POINT_PET_SKILL_LEVEL_11,
	POINT_PET_SKILL_LEVEL_12,
	POINT_PET_SKILL_LEVEL_13,
	POINT_PET_SKILL_LEVEL_14,
	POINT_PET_SKILL_LEVEL_15 = 164,
};
typedef struct SPetApply
{
	BYTE bType;
	long lValue;
} TPetApply;

typedef struct SPetSkill
{
	BYTE bType;
	short sLevel;
} TPetSkill;
#endif

#ifdef __DUNGEON_INFO__
typedef struct SDungeonRank
{
	char name[CHARACTER_NAME_MAX_LEN + 1];
	BYTE level;
	int	value;
}TDungeonRank;
#endif

#ifdef __MAINTENANCE__
typedef struct SMainteance
{
	WORD	id;
	BYTE	mode;
	int		iStartTime, iEndTime;
	char	reason[256 + 1];
	bool	status;
} TMainteance;
#endif

#pragma pack()
#endif
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
