#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "party.h"
#include "regen.h"
#include "p2p.h"
#include "dungeon.h"
#include "db.h"
#include "config.h"
#include "xmas_event.h"
#include "questmanager.h"
#include "questlua.h"
#include "locale_service.h"
#include "shutdown_manager.h"
#include "../../common/CommonDefines.h"
#if defined(__SHIP_DEFENSE__)
#	include "ShipDefense.h"
#endif

#if (!defined(__GNUC__) || defined(__clang__)) && !defined(CXX11_ENABLED)
#	include <boost/bind.hpp>

#elif defined(CXX11_ENABLED)
#	include <functional>
	template <typename T>
	decltype(std::bind(&T::second, std::placeholders::_1)) select2nd()
	{
		return std::bind(&T::second, std::placeholders::_1);
	}
#endif

CHARACTER_MANAGER::CHARACTER_MANAGER() :
	m_iVIDCount(0),
	m_pkChrSelectedStone(NULL),
	m_bUsePendingDestroy(false)
{
	RegisterRaceNum(xmas::MOB_XMAS_FIRWORK_SELLER_VNUM);
	RegisterRaceNum(xmas::MOB_SANTA_VNUM);
	RegisterRaceNum(xmas::MOB_XMAS_TREE_VNUM);

	m_iMobItemRate = 100;
	m_iMobDamageRate = 100;
	m_iMobGoldAmountRate = 100;
	m_iMobGoldDropRate = 100;
	m_iMobExpRate = 100;

	m_iMobItemRatePremium = 100;
	m_iMobGoldAmountRatePremium = 100;
	m_iMobGoldDropRatePremium = 100;
	m_iMobExpRatePremium = 100;

	m_iUserDamageRate = 100;
	m_iUserDamageRatePremium = 100;
#ifdef ENABLE_BATTLE_PASS
	m_BattlePassData.clear();
#endif
#ifdef ENABLE_REWARD_SYSTEM
	m_rewardData.clear();
#endif
}

CHARACTER_MANAGER::~CHARACTER_MANAGER()
{
	Destroy();
}

void CHARACTER_MANAGER::Destroy()
{
#ifdef ENABLE_BATTLE_PASS
	m_BattlePassData.clear();
#endif
#ifdef ENABLE_REWARD_SYSTEM
	m_rewardData.clear();
#endif
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.begin();
	while (it != m_map_pkChrByVID.end()) {
		LPCHARACTER ch = it->second;
		M2_DESTROY_CHARACTER(ch); // m_map_pkChrByVID is changed here
		it = m_map_pkChrByVID.begin();
	}
}

void CHARACTER_MANAGER::GracefulShutdown()
{
	NAME_MAP::iterator it = m_map_pkPCChr.begin();

	while (it != m_map_pkPCChr.end())
		(it++)->second->Disconnect("GracefulShutdown");
}

DWORD CHARACTER_MANAGER::AllocVID()
{
	++m_iVIDCount;
	return m_iVIDCount;
}

LPCHARACTER CHARACTER_MANAGER::CreateCharacter(const char * name, DWORD dwPID)
{
	DWORD dwVID = AllocVID();

#ifdef M2_USE_POOL
	LPCHARACTER ch = pool_.Construct();
#else
	LPCHARACTER ch = M2_NEW CHARACTER;
#endif
	ch->Create(name, dwVID, dwPID ? true : false);

	m_map_pkChrByVID.emplace(dwVID, ch);

	if (dwPID)
	{
		char szName[CHARACTER_NAME_MAX_LEN + 1];
		str_lower(name, szName, sizeof(szName));

		m_map_pkPCChr.emplace(szName, ch);
		m_map_pkChrByPID.emplace(dwPID, ch);
	}

	return (ch);
}

#ifndef DEBUG_ALLOC
void CHARACTER_MANAGER::DestroyCharacter(LPCHARACTER ch)
#else
void CHARACTER_MANAGER::DestroyCharacter(LPCHARACTER ch, const char* file, size_t line)
#endif
{
	if (!ch)
		return;

	// <Factor> Check whether it has been already deleted or not.
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.find(ch->GetVID());
	if (it == m_map_pkChrByVID.end()) {
		sys_err("[CHARACTER_MANAGER::DestroyCharacter] <Factor> %d not found", (long)(ch->GetVID()));
		return; // prevent duplicated destrunction
	}

	if (ch->IsNPC() && !ch->IsPet() && !ch->IsHorse() && ch->GetRider() == NULL)
	{
		if (ch->GetDungeon())
		{
			ch->GetDungeon()->DeadCharacter(ch);
		}
	}

#if defined(__SHIP_DEFENSE__)
	// Delete monsters from the ship defense.
	if (ch->IsPC() == false)
		CShipDefenseManager::Instance().OnKill(ch);
#endif

	if (m_bUsePendingDestroy)
	{
		m_set_pkChrPendingDestroy.emplace(ch);
		return;
	}

	m_map_pkChrByVID.erase(it);

	if (true == ch->IsPC())
	{
		char szName[CHARACTER_NAME_MAX_LEN + 1];

		str_lower(ch->GetName(), szName, sizeof(szName));

		NAME_MAP::iterator it = m_map_pkPCChr.find(szName);

		if (m_map_pkPCChr.end() != it)
			m_map_pkPCChr.erase(it);
	}

	if (0 != ch->GetPlayerID())
	{
		itertype(m_map_pkChrByPID) it = m_map_pkChrByPID.find(ch->GetPlayerID());

		if (m_map_pkChrByPID.end() != it)
		{
			m_map_pkChrByPID.erase(it);
		}
	}

	UnregisterRaceNumMap(ch);

	RemoveFromStateList(ch);

#ifdef M2_USE_POOL
	pool_.Destroy(ch);
#else
#ifndef DEBUG_ALLOC
	M2_DELETE(ch);
#else
	M2_DELETE_EX(ch, file, line);
#endif
#endif
}

LPCHARACTER CHARACTER_MANAGER::Find(DWORD dwVID)
{
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.find(dwVID);

	if (m_map_pkChrByVID.end() == it)
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && dwVID != (DWORD)found->GetVID()) {
		sys_err("[CHARACTER_MANAGER::Find] <Factor> %u != %u", dwVID, (DWORD)found->GetVID());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::Find(const VID & vid)
{
	LPCHARACTER tch = Find((DWORD) vid);

	if (!tch || tch->GetVID() != vid)
		return NULL;

	return tch;
}

LPCHARACTER CHARACTER_MANAGER::FindByPID(DWORD dwPID)
{
	itertype(m_map_pkChrByPID) it = m_map_pkChrByPID.find(dwPID);

	if (m_map_pkChrByPID.end() == it)
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && dwPID != found->GetPlayerID()) {
		sys_err("[CHARACTER_MANAGER::FindByPID] <Factor> %u != %u", dwPID, found->GetPlayerID());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::FindPC(const char * name)
{
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	str_lower(name, szName, sizeof(szName));
	NAME_MAP::iterator it = m_map_pkPCChr.find(szName);

	if (it == m_map_pkPCChr.end())
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && strncasecmp(szName, found->GetName(), CHARACTER_NAME_MAX_LEN) != 0) {
		sys_err("[CHARACTER_MANAGER::FindPC] <Factor> %s != %s", name, found->GetName());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::SpawnMobRandomPosition(DWORD dwVnum, long lMapIndex, bool is_aggressive)
{
	{
		if (dwVnum == 5001 && !quest::CQuestManager::instance().GetEventFlag("japan_regen"))
		{
			sys_log(1, "WAEGU[5001] regen disabled.");
			return NULL;
		}
	}

	{
		if (dwVnum == 5002 && !quest::CQuestManager::instance().GetEventFlag("newyear_mob"))
		{
			sys_log(1, "HAETAE (new-year-mob) [5002] regen disabled.");
			return NULL;
		}
	}

	{
		if (dwVnum == 5004 && !quest::CQuestManager::instance().GetEventFlag("independence_day"))
		{
			sys_log(1, "INDEPENDECE DAY [5004] regen disabled.");
			return NULL;
		}
	}

	const CMob * pkMob = CMobManager::instance().Get(dwVnum);

	if (!pkMob)
	{
		sys_err("no mob data for vnum %u", dwVnum);
		return NULL;
	}

	if (!map_allow_find(lMapIndex))
	{
		sys_err("not allowed map %u", lMapIndex);
		return NULL;
	}

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
	if (pkSectreeMap == NULL) {
		return NULL;
	}

	int i;
	long x, y;
	for (i=0; i<2000; i++)
	{
		x = number(1, (pkSectreeMap->m_setting.iWidth / 100)  - 1) * 100 + pkSectreeMap->m_setting.iBaseX;
		y = number(1, (pkSectreeMap->m_setting.iHeight / 100) - 1) * 100 + pkSectreeMap->m_setting.iBaseY;
		//LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);
		LPSECTREE tree = pkSectreeMap->Find(x, y);

		if (!tree)
			continue;

		DWORD dwAttr = tree->GetAttribute(x, y);

		if (IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT))
			continue;

		if (IS_SET(dwAttr, ATTR_BANPK))
			continue;

		break;
	}

	if (i == 2000)
	{
		sys_err("cannot find valid location");
		return NULL;
	}

	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!sectree)
	{
		sys_log(0, "SpawnMobRandomPosition: cannot create monster at non-exist sectree %d x %d (map %d)", x, y, lMapIndex);
		return NULL;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pkMob->m_table.szLocaleName);

	if (!ch)
	{
		sys_log(0, "SpawnMobRandomPosition: cannot create new character");
		return NULL;
	}

	ch->SetProto(pkMob);

	// if mob is npc with no empire assigned, assign to empire of map
	if (pkMob->m_table.bType == CHAR_TYPE_NPC)
		if (ch->GetEmpire() == 0)
			ch->SetEmpire(SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex));

	ch->SetRotation(number(0, 360));
	if (is_aggressive) //@fixme195
		ch->SetAggressive();

	if (!ch->Show(lMapIndex, x, y, 0, false))
	{
		M2_DESTROY_CHARACTER(ch);
		sys_err(0, "SpawnMobRandomPosition: cannot show monster");
		return NULL;
	}

	char buf[512+1];
	long local_x = x - pkSectreeMap->m_setting.iBaseX;
	long local_y = y - pkSectreeMap->m_setting.iBaseY;
	snprintf(buf, sizeof(buf), "spawn %s[%d] random position at %ld %ld %ld %ld (time: %d)", ch->GetName(), dwVnum, x, y, local_x, local_y, get_global_time());

	if (test_server)
		SendNotice(buf);

	sys_log(0, buf);
	return (ch);
}

LPCHARACTER CHARACTER_MANAGER::SpawnMob(DWORD dwVnum, long lMapIndex, long x, long y, long z, bool bSpawnMotion, int iRot, bool bShow)
{
	const CMob * pkMob = CMobManager::instance().Get(dwVnum);
	if (!pkMob)
	{
		sys_err("SpawnMob: no mob data for vnum %u %ld", dwVnum, lMapIndex);
		return NULL;
	}

	if (!(pkMob->m_table.bType == CHAR_TYPE_NPC || pkMob->m_table.bType == CHAR_TYPE_PET || pkMob->m_table.bType == CHAR_TYPE_HORSE || pkMob->m_table.bType == CHAR_TYPE_WARP || pkMob->m_table.bType == CHAR_TYPE_GOTO) || mining::IsVeinOfOre (dwVnum))
	{
		LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

		if (!tree)
		{
			sys_log(0, "no sectree for spawn at %d %d mobvnum %d mapindex %d", x, y, dwVnum, lMapIndex);
			return NULL;
		}

		DWORD dwAttr = tree->GetAttribute(x, y);

		bool is_set = false;

		if ( mining::IsVeinOfOre (dwVnum) ) is_set = IS_SET(dwAttr, ATTR_BLOCK);
		else is_set = IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT);

		if ( is_set )
		{
			// SPAWN_BLOCK_LOG
			static bool s_isLog=quest::CQuestManager::instance().GetEventFlag("spawn_block_log");
			static DWORD s_nextTime=get_global_time()+10000;

			DWORD curTime=get_global_time();

			if (curTime>s_nextTime)
			{
				s_nextTime=curTime;
				s_isLog=quest::CQuestManager::instance().GetEventFlag("spawn_block_log");

			}

			if (s_isLog)
				sys_log(0, "SpawnMob: BLOCKED position for spawn %s %u at %d %d (attr %u)", pkMob->m_table.szName, dwVnum, x, y, dwAttr);
			// END_OF_SPAWN_BLOCK_LOG
			return NULL;
		}

		if (IS_SET(dwAttr, ATTR_BANPK))
		{
			sys_log(0, "SpawnMob: BAN_PK position for mob spawn %s %u at %d %d", pkMob->m_table.szName, dwVnum, x, y);
			return NULL;
		}
	}

	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!sectree)
	{
		sys_log(0, "SpawnMob: cannot create monster at non-exist sectree %d x %d (map %d)", x, y, lMapIndex);
		return NULL;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pkMob->m_table.szLocaleName);

	if (!ch)
	{
		sys_log(0, "SpawnMob: cannot create new character");
		return NULL;
	}

	if (iRot == -1)
		iRot = number(0, 360);

	ch->SetProto(pkMob);

	// if mob is npc with no empire assigned, assign to empire of map
	if (pkMob->m_table.bType == CHAR_TYPE_NPC)
		if (ch->GetEmpire() == 0)
			ch->SetEmpire(SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex));

	ch->SetRotation(iRot);

	if (bShow && !ch->Show(lMapIndex, x, y, z, bSpawnMotion))
	{
		M2_DESTROY_CHARACTER(ch);
		sys_log(0, "SpawnMob: cannot show monster");
		return NULL;
	}

#ifdef ENABLE_ENTITY_PRELOADING
	//@Amun: or be specific, like if(ch->ispet, mount, whatever)
	if (!ch->IsPC() && !ch->IsGoto() && !ch->IsWarp() && !ch->IsDoor())
		SECTREE_MANAGER::Instance().ExtendPreloadedEntitiesMap(lMapIndex, pkMob->m_table.dwVnum);
#endif

	return (ch);
}

LPCHARACTER CHARACTER_MANAGER::SpawnMobRange(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, bool bIsException, bool bSpawnMotion, bool bAggressive )
{
	const CMob * pkMob = CMobManager::instance().Get(dwVnum);

	if (!pkMob)
		return NULL;

	if (pkMob->m_table.bType == CHAR_TYPE_STONE)
		bSpawnMotion = true;

	int i = 16;

	while (i--)
	{
		int x = number(sx, ex);
		int y = number(sy, ey);
		/*
		   if (bIsException)
		   if (is_regen_exception(x, y))
		   continue;
		 */
		LPCHARACTER ch = SpawnMob(dwVnum, lMapIndex, x, y, 0, bSpawnMotion);

		if (ch)
		{
			sys_log(1, "MOB_SPAWN: %s(%d) %dx%d", ch->GetName(), (DWORD) ch->GetVID(), ch->GetX(), ch->GetY());
			if (bAggressive)
				ch->SetAggressive();
			return (ch);
		}
	}

	return NULL;
}

void CHARACTER_MANAGER::SelectStone(LPCHARACTER pkChr)
{
	m_pkChrSelectedStone = pkChr;
}

bool CHARACTER_MANAGER::SpawnMoveGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, int tx, int ty, LPREGEN pkRegen, bool bAggressive_)
{
	CMobGroup * pkGroup = CMobManager::Instance().GetGroup(dwVnum);

	if (!pkGroup)
	{
		sys_err("NOT_EXIST_GROUP_VNUM(%u) Map(%u) ", dwVnum, lMapIndex);
		return false;
	}

	LPCHARACTER pkChrMaster = NULL;
	LPPARTY pkParty = NULL;

	const std::vector<DWORD> & c_rdwMembers = pkGroup->GetMemberVector();

	bool bSpawnedByStone = false;
	bool bAggressive = bAggressive_;

	if (m_pkChrSelectedStone)
	{
		bSpawnedByStone = true;
		if (m_pkChrSelectedStone->GetDungeon())
			bAggressive = true;
	}

	for (DWORD i = 0; i < c_rdwMembers.size(); ++i)
	{
		LPCHARACTER tch = SpawnMobRange(c_rdwMembers[i], lMapIndex, sx, sy, ex, ey, true, bSpawnedByStone);

		if (!tch)
		{
			if (i == 0)
				return false;

			continue;
		}

		sx = tch->GetX() - number(300, 500);
		sy = tch->GetY() - number(300, 500);
		ex = tch->GetX() + number(300, 500);
		ey = tch->GetY() + number(300, 500);

		if (m_pkChrSelectedStone)
			tch->SetStone(m_pkChrSelectedStone);
		else if (pkParty)
		{
			pkParty->Join(tch->GetVID());
			pkParty->Link(tch);
		}
		else if (!pkChrMaster)
		{
			pkChrMaster = tch;
			pkChrMaster->SetRegen(pkRegen);

			pkParty = CPartyManager::instance().CreateParty(pkChrMaster);
		}
		if (bAggressive)
			tch->SetAggressive();

		if (tch->Goto(tx, ty))
			tch->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	}

	return true;
}

bool CHARACTER_MANAGER::SpawnGroupGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen, bool bAggressive_, LPDUNGEON pDungeon)
{
	const DWORD dwGroupID = CMobManager::Instance().GetGroupFromGroupGroup(dwVnum);

	if( dwGroupID != 0 )
	{
		return SpawnGroup(dwGroupID, lMapIndex, sx, sy, ex, ey, pkRegen, bAggressive_, pDungeon);
	}
	else
	{
		sys_err( "NOT_EXIST_GROUP_GROUP_VNUM(%u) MAP(%ld)", dwVnum, lMapIndex );
		return false;
	}
}

LPCHARACTER CHARACTER_MANAGER::SpawnGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen, bool bAggressive_, LPDUNGEON pDungeon)
{
	CMobGroup * pkGroup = CMobManager::Instance().GetGroup(dwVnum);

	if (!pkGroup)
	{
		if (dwVnum > 0)
			sys_err("NOT_EXIST_GROUP_VNUM(%u) Map(%u) ", dwVnum, lMapIndex);
		return NULL;
	}

	LPCHARACTER pkChrMaster = NULL;
	LPPARTY pkParty = NULL;

	const std::vector<DWORD> & c_rdwMembers = pkGroup->GetMemberVector();

	bool bSpawnedByStone = false;
	bool bAggressive = bAggressive_;

	if (m_pkChrSelectedStone)
	{
		bSpawnedByStone = true;

		if (m_pkChrSelectedStone->GetDungeon())
			bAggressive = true;
	}

	LPCHARACTER chLeader = NULL;

	for (DWORD i = 0; i < c_rdwMembers.size(); ++i)
	{
		LPCHARACTER tch = SpawnMobRange(c_rdwMembers[i], lMapIndex, sx, sy, ex, ey, true, bSpawnedByStone);

		if (!tch)
		{
			if (i == 0)
				return NULL;

			continue;
		}

		if (i == 0)
			chLeader = tch;

		tch->SetDungeon(pDungeon);

		sx = tch->GetX() - number(300, 500);
		sy = tch->GetY() - number(300, 500);
		ex = tch->GetX() + number(300, 500);
		ey = tch->GetY() + number(300, 500);

		if (m_pkChrSelectedStone)
			tch->SetStone(m_pkChrSelectedStone);
		else if (pkParty)
		{
			pkParty->Join(tch->GetVID());
			pkParty->Link(tch);
		}
		else if (!pkChrMaster)
		{
			pkChrMaster = tch;
			pkChrMaster->SetRegen(pkRegen);

			pkParty = CPartyManager::instance().CreateParty(pkChrMaster);
		}

		if (bAggressive)
			tch->SetAggressive();
	}

	return chLeader;
}

struct FuncUpdateAndResetChatCounter
{
	void operator () (LPCHARACTER ch)
	{
		ch->ResetChatCounter();
		ch->CFSM::Update();
	}
};

void CHARACTER_MANAGER::Update(int iPulse)
{
	using namespace std;
#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
	using namespace __gnu_cxx;
#endif

	BeginPendingDestroy();

	{
		if (!m_map_pkPCChr.empty())
		{
			CHARACTER_VECTOR v;
			v.reserve(m_map_pkPCChr.size());
#if (defined(__GNUC__) && !defined(__clang__)) || defined(CXX11_ENABLED)
			transform(m_map_pkPCChr.begin(), m_map_pkPCChr.end(), back_inserter(v), select2nd<NAME_MAP::value_type>());
#else
			transform(m_map_pkPCChr.begin(), m_map_pkPCChr.end(), back_inserter(v), boost::bind(&NAME_MAP::value_type::second, _1));
#endif

			if (0 == (iPulse % PASSES_PER_SEC(5)))
			{
				FuncUpdateAndResetChatCounter f;
				for_each(v.begin(), v.end(), f);
			}
			else
			{
				for_each(v.begin(), v.end(), msl::bind2nd(std::mem_fn(&CHARACTER::UpdateCharacter), iPulse));
			}
		}

	}

	{
		if (!m_set_pkChrState.empty())
		{
			CHARACTER_VECTOR v;
			v.reserve(m_set_pkChrState.size());
#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
			transform(m_set_pkChrState.begin(), m_set_pkChrState.end(), back_inserter(v), identity<CHARACTER_SET::value_type>());
#else
			v.insert(v.end(), m_set_pkChrState.begin(), m_set_pkChrState.end());
#endif
			for_each(v.begin(), v.end(), msl::bind2nd(std::mem_fn(&CHARACTER::UpdateStateMachine), iPulse));
		}
	}

	{
		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(xmas::MOB_SANTA_VNUM, i))
		{
			for_each(i.begin(), i.end(),
					msl::bind2nd(std::mem_fn(&CHARACTER::UpdateStateMachine), iPulse));
		}
	}

	if (0 == (iPulse % PASSES_PER_SEC(3600)))
	{
		for (itertype(m_map_dwMobKillCount) it = m_map_dwMobKillCount.begin(); it != m_map_dwMobKillCount.end(); ++it)
			DBManager::instance().SendMoneyLog(MONEY_LOG_MONSTER_KILL, it->first, it->second);

		m_map_dwMobKillCount.clear();
	}

	if (test_server && 0 == (iPulse % PASSES_PER_SEC(60)))
		sys_log(0, "CHARACTER COUNT vid %zu pid %zu", m_map_pkChrByVID.size(), m_map_pkChrByPID.size());

	FlushPendingDestroy();

	// ShutdownManager Update
	CShutdownManager::Instance().Update();
}

void CHARACTER_MANAGER::ProcessDelayedSave()
{
	CHARACTER_SET::iterator it = m_set_pkChrForDelayedSave.begin();

	while (it != m_set_pkChrForDelayedSave.end())
	{
		LPCHARACTER pkChr = *it++;
		pkChr->SaveReal();
	}

	m_set_pkChrForDelayedSave.clear();
}

bool CHARACTER_MANAGER::AddToStateList(LPCHARACTER ch)
{
	assert(ch != NULL);

	CHARACTER_SET::iterator it = m_set_pkChrState.find(ch);

	if (it == m_set_pkChrState.end())
	{
		m_set_pkChrState.emplace(ch);
		return true;
	}

	return false;
}

void CHARACTER_MANAGER::RemoveFromStateList(LPCHARACTER ch)
{
	CHARACTER_SET::iterator it = m_set_pkChrState.find(ch);

	if (it != m_set_pkChrState.end())
	{
		//sys_log(0, "RemoveFromStateList %p", ch);
		m_set_pkChrState.erase(it);
	}
}

void CHARACTER_MANAGER::DelayedSave(LPCHARACTER ch)
{
	m_set_pkChrForDelayedSave.emplace(ch);
}

bool CHARACTER_MANAGER::FlushDelayedSave(LPCHARACTER ch)
{
	CHARACTER_SET::iterator it = m_set_pkChrForDelayedSave.find(ch);

	if (it == m_set_pkChrForDelayedSave.end())
		return false;

	m_set_pkChrForDelayedSave.erase(it);
	ch->SaveReal();
	return true;
}

void CHARACTER_MANAGER::RegisterForMonsterLog(LPCHARACTER ch)
{
	m_set_pkChrMonsterLog.emplace(ch);
}

void CHARACTER_MANAGER::UnregisterForMonsterLog(LPCHARACTER ch)
{
	m_set_pkChrMonsterLog.erase(ch);
}

void CHARACTER_MANAGER::PacketMonsterLog(LPCHARACTER ch, const void* buf, int size)
{
	itertype(m_set_pkChrMonsterLog) it;

	for (it = m_set_pkChrMonsterLog.begin(); it!=m_set_pkChrMonsterLog.end();++it)
	{
		LPCHARACTER c = *it;

		if (ch && DISTANCE_APPROX(c->GetX()-ch->GetX(), c->GetY()-ch->GetY())>6000)
			continue;

		LPDESC d = c->GetDesc();

		if (d)
			d->Packet(buf, size);
	}
}

void CHARACTER_MANAGER::KillLog(DWORD dwVnum)
{
	const DWORD SEND_LIMIT = 10000;

	itertype(m_map_dwMobKillCount) it = m_map_dwMobKillCount.find(dwVnum);

	if (it == m_map_dwMobKillCount.end())
		m_map_dwMobKillCount.emplace(dwVnum, 1);
	else
	{
		++it->second;

		if (it->second > SEND_LIMIT)
		{
			DBManager::instance().SendMoneyLog(MONEY_LOG_MONSTER_KILL, it->first, it->second);
			m_map_dwMobKillCount.erase(it);
		}
	}
}

void CHARACTER_MANAGER::RegisterRaceNum(DWORD dwVnum)
{
	m_set_dwRegisteredRaceNum.emplace(dwVnum);
}

void CHARACTER_MANAGER::RegisterRaceNumMap(LPCHARACTER ch)
{
	DWORD dwVnum = ch->GetRaceNum();

	if (m_set_dwRegisteredRaceNum.find(dwVnum) != m_set_dwRegisteredRaceNum.end())
	{
		sys_log(0, "RegisterRaceNumMap %s %u", ch->GetName(), dwVnum);
		m_map_pkChrByRaceNum[dwVnum].emplace(ch);
	}
}

void CHARACTER_MANAGER::UnregisterRaceNumMap(LPCHARACTER ch)
{
	DWORD dwVnum = ch->GetRaceNum();

	itertype(m_map_pkChrByRaceNum) it = m_map_pkChrByRaceNum.find(dwVnum);

	if (it != m_map_pkChrByRaceNum.end())
		it->second.erase(ch);
}

bool CHARACTER_MANAGER::GetCharactersByRaceNum(DWORD dwRaceNum, CharacterVectorInteractor & i)
{
	std::map<DWORD, CHARACTER_SET>::iterator it = m_map_pkChrByRaceNum.find(dwRaceNum);

	if (it == m_map_pkChrByRaceNum.end())
		return false;

	i = it->second;
	return true;
}

#define FIND_JOB_WARRIOR_0	(1 << 3)
#define FIND_JOB_WARRIOR_1	(1 << 4)
#define FIND_JOB_WARRIOR_2	(1 << 5)
#define FIND_JOB_WARRIOR	(FIND_JOB_WARRIOR_0 | FIND_JOB_WARRIOR_1 | FIND_JOB_WARRIOR_2)
#define FIND_JOB_ASSASSIN_0	(1 << 6)
#define FIND_JOB_ASSASSIN_1	(1 << 7)
#define FIND_JOB_ASSASSIN_2	(1 << 8)
#define FIND_JOB_ASSASSIN	(FIND_JOB_ASSASSIN_0 | FIND_JOB_ASSASSIN_1 | FIND_JOB_ASSASSIN_2)
#define FIND_JOB_SURA_0		(1 << 9)
#define FIND_JOB_SURA_1		(1 << 10)
#define FIND_JOB_SURA_2		(1 << 11)
#define FIND_JOB_SURA		(FIND_JOB_SURA_0 | FIND_JOB_SURA_1 | FIND_JOB_SURA_2)
#define FIND_JOB_SHAMAN_0	(1 << 12)
#define FIND_JOB_SHAMAN_1	(1 << 13)
#define FIND_JOB_SHAMAN_2	(1 << 14)
#define FIND_JOB_SHAMAN		(FIND_JOB_SHAMAN_0 | FIND_JOB_SHAMAN_1 | FIND_JOB_SHAMAN_2)
#ifdef ENABLE_WOLFMAN_CHARACTER
#define FIND_JOB_WOLFMAN_0	(1 << 15)
#define FIND_JOB_WOLFMAN_1	(1 << 16)
#define FIND_JOB_WOLFMAN_2	(1 << 17)
#define FIND_JOB_WOLFMAN		(FIND_JOB_WOLFMAN_0 | FIND_JOB_WOLFMAN_1 | FIND_JOB_WOLFMAN_2)
#endif

//
// (job+1)*3+(skill_group)
//
LPCHARACTER CHARACTER_MANAGER::FindSpecifyPC(unsigned int uiJobFlag, long lMapIndex, LPCHARACTER except, int iMinLevel, int iMaxLevel)
{
	LPCHARACTER chFind = NULL;
	itertype(m_map_pkChrByPID) it;
	int n = 0;

	for (it = m_map_pkChrByPID.begin(); it != m_map_pkChrByPID.end(); ++it)
	{
		LPCHARACTER ch = it->second;

		if (ch == except)
			continue;

		if (ch->GetLevel() < iMinLevel)
			continue;

		if (ch->GetLevel() > iMaxLevel)
			continue;

		if (ch->GetMapIndex() != lMapIndex)
			continue;

		if (uiJobFlag)
		{
			unsigned int uiChrJob = (1 << ((ch->GetJob() + 1) * 3 + ch->GetSkillGroup()));

			if (!IS_SET(uiJobFlag, uiChrJob))
				continue;
		}

		if (!chFind || number(1, ++n) == 1)
			chFind = ch;
	}

	return chFind;
}

int CHARACTER_MANAGER::GetMobItemRate(LPCHARACTER ch)
{
	//PREVENT_TOXICATION_FOR_CHINA
	if (g_bChinaIntoxicationCheck)
	{
		if ( ch->IsOverTime( OT_3HOUR ) )
		{
			if (ch && ch->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0)
				return m_iMobItemRatePremium/2;
			return m_iMobItemRate/2;
		}
		else if ( ch->IsOverTime( OT_5HOUR ) )
		{
			return 0;
		}
	}
	//END_PREVENT_TOXICATION_FOR_CHINA
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0)
		return m_iMobItemRatePremium;
	return m_iMobItemRate;
}

int CHARACTER_MANAGER::GetMobDamageRate(LPCHARACTER ch)
{
	return m_iMobDamageRate;
}

bool CHARACTER_MANAGER::IsDungeonMap(LPCHARACTER victim, LPCHARACTER attacker)
{
	if (!victim || !attacker)
		return false;

	if (victim->GetRealMapIndex() == 66 ||
		victim->GetRealMapIndex() == 216 ||
		victim->GetRealMapIndex() == 354 ||
		victim->GetRealMapIndex() == 365 ||
		victim->GetRealMapIndex() == 208 ||
		victim->GetRealMapIndex() == 351 ||
		victim->GetRealMapIndex() == 360 ||
		victim->GetRealMapIndex() == 352 ||
		victim->GetRealMapIndex() == 364 ||
		victim->GetRealMapIndex() == 358 ||
		victim->GetRealMapIndex() == 363 ||
		victim->GetRealMapIndex() == 362 ||
		victim->GetRealMapIndex() == 367
	)
		return true;

	return false;
}

int CHARACTER_MANAGER::GetMobGoldAmountRate(LPCHARACTER ch)
{
	if ( !ch )
		return m_iMobGoldAmountRate;

	//PREVENT_TOXICATION_FOR_CHINA
	if (g_bChinaIntoxicationCheck)
	{
		if ( ch->IsOverTime( OT_3HOUR ) )
		{
			if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
				return m_iMobGoldAmountRatePremium/2;
			return m_iMobGoldAmountRate/2;
		}
		else if ( ch->IsOverTime( OT_5HOUR ) )
		{
			return 0;
		}
	}
	//END_PREVENT_TOXICATION_FOR_CHINA
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		return m_iMobGoldAmountRatePremium;
	return m_iMobGoldAmountRate;
}

int CHARACTER_MANAGER::GetMobGoldDropRate(LPCHARACTER ch)
{
	if ( !ch )
		return m_iMobGoldDropRate;

	//PREVENT_TOXICATION_FOR_CHINA
	if (g_bChinaIntoxicationCheck)
	{
		if ( ch->IsOverTime( OT_3HOUR ) )
		{
			if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
				return m_iMobGoldDropRatePremium/2;
			return m_iMobGoldDropRate/2;
		}
		else if ( ch->IsOverTime( OT_5HOUR ) )
		{
			return 0;
		}
	}
	//END_PREVENT_TOXICATION_FOR_CHINA
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		return m_iMobGoldDropRatePremium;
	return m_iMobGoldDropRate;
}

int CHARACTER_MANAGER::GetMobExpRate(LPCHARACTER ch)
{
	if ( !ch )
		return m_iMobExpRate;

	//PREVENT_TOXICATION_FOR_CHINA
	if (g_bChinaIntoxicationCheck)
	{
		if ( ch->IsOverTime( OT_3HOUR ) )
		{
			if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
				return m_iMobExpRatePremium/2;
			return m_iMobExpRate/2;
		}
		else if ( ch->IsOverTime( OT_5HOUR ) )
		{
			return 0;
		}
	}
	//END_PREVENT_TOXICATION_FOR_CHINA
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		return m_iMobExpRatePremium;
	return m_iMobExpRate;
}

int	CHARACTER_MANAGER::GetUserDamageRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iUserDamageRate;

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		return m_iUserDamageRatePremium;

	return m_iUserDamageRate;
}

void CHARACTER_MANAGER::SendScriptToMap(long lMapIndex, const std::string & s)
{
	LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);

	if (NULL == pSecMap)
		return;

	struct packet_script p;

	p.header = HEADER_GC_SCRIPT;
	p.skin = 1;
	p.src_size = s.size();

	quest::FSendPacket f;
	p.size = p.src_size + sizeof(struct packet_script);
	f.buf.write(&p, sizeof(struct packet_script));
	f.buf.write(&s[0], s.size());

	pSecMap->for_each(f);
}

bool CHARACTER_MANAGER::BeginPendingDestroy()
{
	if (m_bUsePendingDestroy)
		return false;

	m_bUsePendingDestroy = true;
	return true;
}

void CHARACTER_MANAGER::FlushPendingDestroy()
{
	using namespace std;

	m_bUsePendingDestroy = false;

	if (!m_set_pkChrPendingDestroy.empty())
	{
		sys_log(0, "FlushPendingDestroy size %d", m_set_pkChrPendingDestroy.size());

		CHARACTER_SET::iterator it = m_set_pkChrPendingDestroy.begin(),
			end = m_set_pkChrPendingDestroy.end();
		for ( ; it != end; ++it) {
			M2_DESTROY_CHARACTER(*it);
		}

		m_set_pkChrPendingDestroy.clear();
	}
}

CharacterVectorInteractor::CharacterVectorInteractor(const CHARACTER_SET & r)
{
	using namespace std;
#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
	using namespace __gnu_cxx;
#endif

	reserve(r.size());
#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
	transform(r.begin(), r.end(), back_inserter(*this), identity<CHARACTER_SET::value_type>());
#else
	insert(end(), r.begin(), r.end());
#endif

	if (CHARACTER_MANAGER::instance().BeginPendingDestroy())
		m_bMyBegin = true;
}

CharacterVectorInteractor::~CharacterVectorInteractor()
{
	if (m_bMyBegin)
		CHARACTER_MANAGER::instance().FlushPendingDestroy();
}

void CHARACTER_MANAGER::CheckEnergyCrystal(LPCHARACTER ch)
{
	for (WORD i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		int j = i;

		LPITEM item = ch->GetInventoryItem(i);
		const int now = time(0);

		if (!item)
			continue;

		if (item->IsCrystal())
		{
			if (item->GetSocket(0) == 1)
			{
				CAffect* affect = ch->FindAffect(AFFECT_CRYSTAL);
				if (!affect)
					return;
					
				ch->RemoveAffect(affect);

				item->SetSocket(0, 0);
				item->SetSocket(1, (item->GetSocket(1) - now) > 0 ? item->GetSocket(1) - now : 0);
				ch->SetQuestFlag("crystal.cell", j);
				return;
			}
			else if (item->GetSocket(0) == 0 && ch->GetQuestFlag("crystal.cell") == j)
			{
				int lastTime = item->GetSocket(1);

				if (lastTime > 60)
					lastTime -= 60;

				if(lastTime <= 0)
				{
					item->SetSocket(0, 0);
					item->SetSocket(1, 0);
					return;
				}
				
				item->SetSocket(0, 1);
				item->SetSocket(1, now + lastTime);

				ch->AddAffect(AFFECT_CRYSTAL, APPLY_NONE, 0, AFF_NONE, lastTime, 0, false);

				for (BYTE i = 0; i < 5; ++i)
				{
					if (item->GetAttributeType(i) != APPLY_NONE)
						ch->AddAffect(AFFECT_CRYSTAL, aApplyInfo[item->GetAttributeType(i)].bPointType, item->GetAttributeValue(i), AFF_NONE, lastTime, 0, false);
				}
				ch->SetQuestFlag("crystal.cell", -1);
			}
		}
	}
}

#ifdef ENABLE_MULTI_FARM_BLOCK
void CHARACTER_MANAGER::CheckMultiFarmAccounts(const char* szIP)
{
	auto it = m_mapmultiFarm.find(szIP);
	if (it != m_mapmultiFarm.end())
	{
		auto itVec = it->second.begin();
		while(itVec != it->second.end())
		{
			LPCHARACTER ch = FindByPID(itVec->playerID);
			CCI* chP2P = P2P_MANAGER::Instance().FindByPID(itVec->playerID);
			if(!ch && !chP2P)
				itVec = it->second.erase(itVec);
			else
				++itVec;
		}
		if(!it->second.size())
			m_mapmultiFarm.erase(szIP);
	}
}

void CHARACTER_MANAGER::RemoveMultiFarm(const char* szIP, const DWORD playerID, const bool isP2P)
{
	if (!isP2P)
	{
		TPacketGGMultiFarm p;
		p.header = HEADER_GG_MULTI_FARM;
		p.subHeader = MULTI_FARM_REMOVE;
		p.playerID = playerID;
		strlcpy(p.playerIP, szIP, sizeof(p.playerIP));
		P2P_MANAGER::Instance().Send(&p, sizeof(TPacketGGMultiFarm));
	}
	
	auto it = m_mapmultiFarm.find(szIP);
	if (it != m_mapmultiFarm.end())
	{
		for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
		{
			if (itVec->playerID == playerID)
			{
				it->second.erase(itVec);
				break;
			}
		}
		if (!it->second.size())
			m_mapmultiFarm.erase(szIP);
	}
	std::map<DWORD, std::pair<std::string, bool>> m_mapNames;
	// const int infarmPlayerCount = GetMultiFarmCount(szIP, m_mapNames);
	for (auto it = m_mapNames.begin(); it != m_mapNames.end(); ++it)
	{
		LPCHARACTER newCh = FindByPID(it->first);
		if (newCh)
		{
			newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiFarmAffect %d 0", newCh->GetRewardStatus());
			for (auto itEx = m_mapNames.begin(); itEx != m_mapNames.end(); ++itEx)
			{
				if (itEx->second.second)
					newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiFarmPlayer %s", itEx->second.first.c_str());
			}
		}
	}
}
void CHARACTER_MANAGER::SetMultiFarm(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, const BYTE affectType, const int affectTime)
{
	const auto it = m_mapmultiFarm.find(szIP);
	if (it != m_mapmultiFarm.end())
	{
		for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
		{
			if (itVec->playerID == playerID)
			{
				itVec->farmStatus = bStatus;
				itVec->affectType = affectType;
				itVec->affectTime = affectTime;
				return;
			}
		}
		it->second.emplace_back(TMultiFarm(playerID, playerName, bStatus, affectType, affectTime));
	}
	else
	{
		std::vector<TMultiFarm> m_vecFarmList;
		m_vecFarmList.emplace_back(TMultiFarm(playerID, playerName, bStatus, affectType, affectTime));
		m_mapmultiFarm.emplace(szIP, m_vecFarmList);
	}
}
int CHARACTER_MANAGER::GetMultiFarmCount(const char* playerIP, std::map<DWORD, std::pair<std::string, bool>>& m_mapNames)
{
	int accCount = 0;
	bool affectTimeHas = false;
	BYTE affectType = 0;
	const auto it = m_mapmultiFarm.find(playerIP);
	if (it != m_mapmultiFarm.end())
	{
		for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
		{
			if (itVec->farmStatus)
				accCount++;
			if (itVec->affectTime > get_global_time())
				affectTimeHas = true;
			if (itVec->affectType > affectType)
				affectType = itVec->affectType;
			m_mapNames.emplace(itVec->playerID, std::make_pair(itVec->playerName, itVec->farmStatus));
		}
	}

	if (affectTimeHas && affectType > 0)
		accCount -= affectType;
	if (accCount < 0)
		accCount = 0;

	return accCount;
}
void CHARACTER_MANAGER::CheckMultiFarmAccount(const char* szIP, const DWORD playerID, const char* playerName, const bool bStatus, BYTE affectType, int affectDuration, bool isP2P)
{
	CheckMultiFarmAccounts(szIP);

	LPCHARACTER ch = FindByPID(playerID);
	if (ch)
	{
		affectDuration = ch->FindAffect(AFFECT_MULTI_FARM_PREMIUM) ? get_global_time() + ch->FindAffect(AFFECT_MULTI_FARM_PREMIUM)->lDuration : 0;
		affectType = ch->FindAffect(AFFECT_MULTI_FARM_PREMIUM) ? ch->FindAffect(AFFECT_MULTI_FARM_PREMIUM)->lApplyValue : 0;
	}

	std::map<DWORD, std::pair<std::string, bool>> m_mapNames;
	int farmPlayerCount = GetMultiFarmCount(szIP, m_mapNames);
	if (bStatus)
	{
		if (farmPlayerCount >= 2)
		{
			CheckMultiFarmAccount(szIP, playerID, playerName, false);
			return;
		}
	}

	if (!isP2P)
	{
		TPacketGGMultiFarm p;
		p.header = HEADER_GG_MULTI_FARM;
		p.subHeader = MULTI_FARM_SET;
		p.playerID = playerID;
		strlcpy(p.playerIP, szIP, sizeof(p.playerIP));
		strlcpy(p.playerName, playerName, sizeof(p.playerIP));
		p.farmStatus = bStatus;
		p.affectType = affectType;
		p.affectTime = affectDuration;
		P2P_MANAGER::Instance().Send(&p, sizeof(TPacketGGMultiFarm));
	}

	SetMultiFarm(szIP, playerID, playerName, bStatus, affectType, affectDuration);
	if (ch)
	{
		ch->SetQuestFlag("multi_farm.last_status", bStatus);
		ch->SetRewardStatus(bStatus);
	}

	m_mapNames.clear();
	farmPlayerCount = GetMultiFarmCount(szIP, m_mapNames);
	
	for (auto it = m_mapNames.begin(); it != m_mapNames.end(); ++it)
	{
		LPCHARACTER newCh = FindByPID(it->first);
		if (newCh)
		{
			newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiFarmAffect %d %d", newCh->GetRewardStatus(), newCh == ch ? true : false);
			for (auto itEx = m_mapNames.begin(); itEx != m_mapNames.end(); ++itEx)
			{
				if (itEx->second.second)
					newCh->ChatPacket(CHAT_TYPE_COMMAND, "UpdateMultiFarmPlayer %s", itEx->second.first.c_str());
			}
		}
	}
}
#endif

#ifdef RENEWAL_MISSION_BOOKS
void CHARACTER_MANAGER::GiveNewMission(LPITEM missionBook, LPCHARACTER ch)
{
	if (!missionBook || !ch)
		return;
	const DWORD missionBookItem = missionBook->GetVnum();
	if (ch->MissionCount() >= MISSION_BOOK_MAX )
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't read new mission book."));
		return;
	}
	std::vector<WORD> m_vecCanGetMissions;
#if __cplusplus < 199711L
	for (itertype(m_mapMissionData) it = m_mapMissionData.begin(); it != m_mapMissionData.end(); ++it)
#else
	for (auto it = m_mapMissionData.begin(); it != m_mapMissionData.end(); ++it)
#endif
	{
		if (it->second.missionItemIndex == missionBookItem)
		{
			if (!ch->IsMissionHas(it->second.id))
#if __cplusplus < 199711L
				m_vecCanGetMissions.push_back(it->second.id);
#else
				m_vecCanGetMissions.emplace_back(it->second.id);
#endif
		}
	}
	const int luckyIndex = m_vecCanGetMissions.size() > 1 ? number(0, m_vecCanGetMissions.size() - 1) : 0;
	if (luckyIndex + 1 > m_vecCanGetMissions.size())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't get more mission on this book!"));
		return;
	}
	ch->GiveNewMission(m_vecCanGetMissions[luckyIndex]);
	missionBook->SetCount(missionBook->GetCount() - 1);
}
const TMissionBookData* CHARACTER_MANAGER::GetMissionData(WORD id)
{
#if __cplusplus < 199711L
	const itertype(m_mapMissionData) it = m_mapMissionData.find(id);
#else
	const auto it = m_mapMissionData.find(id);
#endif
	if (it != m_mapMissionData.end())
		return &it->second;
	return NULL;
}
void CHARACTER_MANAGER::LoadMissionBook()
{
	m_mapMissionData.clear();
	char filename[124];
	snprintf(filename, sizeof(filename), "data/missionbook/data");
	FILE* fp;
	if ((fp = fopen(filename, "r")) != NULL)
	{
		char	one_line[256];
		TMissionBookData mission;
		while (fgets(one_line, 256, fp))
		{
			std::vector<std::string> m_vec;
			split_argument(one_line, m_vec);
			if (!m_vec.size())
				continue;

			if (m_vec[0].find("#") != std::string::npos)
				continue;

			if (m_vec[0].find("start") != std::string::npos)
			{
				memset(&mission, 0, sizeof(mission));
				memset(&mission.gold, 0, sizeof(mission.gold));
				memset(&mission.exp, 0, sizeof(mission.exp));
				memset(&mission.rewardItems, 0, sizeof(mission.rewardItems));
				memset(&mission.rewardCount, 0, sizeof(mission.rewardCount));
			}
			else if (m_vec[0] == "id")
			{
				if (m_vec.size() != 2)
					continue;
				str_to_number(mission.id, m_vec[1].c_str());
			}
			else if (m_vec[0] == "book")
			{
				if (m_vec.size() != 2)
					continue;
				str_to_number(mission.missionItemIndex, m_vec[1].c_str());
			}
			else if (m_vec[0] == "type")
			{
				if (m_vec.size() != 2)
					continue;
				else if (m_vec[1].find("monster") != std::string::npos)
					mission.type = MISSION_BOOK_TYPE_MONSTER;
				else if (m_vec[1].find("metin") != std::string::npos)
					mission.type = MISSION_BOOK_TYPE_METINSTONE;
				else if (m_vec[1].find("boss") != std::string::npos)
					mission.type = MISSION_BOOK_TYPE_BOSS;
			}
			else if (m_vec[0] == "levelrange")
			{
				if (m_vec.size() != 2)
					continue;
				str_to_number(mission.levelRange, m_vec[1].c_str());
			}
			else if (m_vec[0] == "subtype")
			{
				if (m_vec.size() != 2)
					continue;
				if (m_vec[1] != "all")
					str_to_number(mission.subtype, m_vec[1].c_str());
			}
			else if (m_vec[0] == "max")
			{
				if (m_vec.size() != 2)
					continue;
				str_to_number(mission.max, m_vec[1].c_str());
			}
			else if (m_vec[0] == "timer")
			{
				if (m_vec.size() != 2)
					continue;
				str_to_number(mission.maxTime, m_vec[1].c_str());
			}
			else if (m_vec[0] == "gold")
			{
				if (m_vec.size() != 3)
					continue;
				str_to_number(mission.gold[0], m_vec[1].c_str());
				str_to_number(mission.gold[1], m_vec[2].c_str());
			}
			else if (m_vec[0] == "exp")
			{
				if (m_vec.size() != 3)
					continue;
				str_to_number(mission.exp[0], m_vec[1].c_str());
				str_to_number(mission.exp[1], m_vec[2].c_str());
			}
			else if (m_vec[0] == "reward")
			{
				if (m_vec.size() != 3)
					continue;
				for (BYTE j = 0; j < 6; ++j)
				{
					if (mission.rewardItems[j] == 0)
					{
						str_to_number(mission.rewardItems[j], m_vec[1].c_str());
						str_to_number(mission.rewardCount[j], m_vec[2].c_str());
						break;
					}
				}
			}
			if (m_vec[0].find("end") != std::string::npos)
			{
				if (mission.id != 0)
#if __cplusplus < 199711L
					m_mapMissionData.insert(std::make_pair(mission.id, mission));
#else
					m_mapMissionData.emplace(mission.id, mission);
#endif
				memset(&mission, 0, sizeof(mission));
				memset(&mission.gold, 0, sizeof(mission.gold));
				memset(&mission.exp, 0, sizeof(mission.exp));
				memset(&mission.rewardItems, 0, sizeof(mission.rewardItems));
				memset(&mission.rewardCount, 0, sizeof(mission.rewardCount));
			}

		}
	}
}
#endif

#ifdef ENABLE_EVENT_MANAGER
#include "item_manager.h"
#include "item.h"
#include "desc_client.h"
#include "desc_manager.h"
void CHARACTER_MANAGER::ClearEventData()
{
	m_eventData.clear();
}
void CHARACTER_MANAGER::CheckBonusEvent(LPCHARACTER ch)
{
	const TEventManagerData* eventPtr = CheckEventIsActive(BONUS_EVENT, ch->GetEmpire());
	if (eventPtr)
		ch->ApplyPoint(eventPtr->value[0], eventPtr->value[1]);
}
const TEventManagerData* CHARACTER_MANAGER::CheckEventIsActive(BYTE eventIndex, BYTE empireIndex)
{
	for (const auto& [dayIndex, dayVector] : m_eventData)
	{
		for (const auto& eventData : dayVector)
		{
			if (eventData.eventIndex == eventIndex)
			{
				if (eventData.channelFlag != 0)
					if (eventData.channelFlag != g_bChannel)
						continue;
				if (eventData.empireFlag != 0 && empireIndex != 0)
					if (eventData.empireFlag != empireIndex)
						continue;

				if(eventData.eventStatus == true)
					return &eventData;
			}
		}
	}
	return NULL;
}
void CHARACTER_MANAGER::CheckEventForDrop(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM>& vec_item)
{
	const BYTE killerEmpire = pkKiller->GetEmpire();
	const TEventManagerData* eventPtr = NULL;
	LPITEM rewardItem = NULL;

	if (pkChr->IsStone())
	{
		if (pkKiller->FindAffect(AFFECT_THIEF_GLOVES_STONES) && number(1,100) <= 10)
		{
			std::vector<LPITEM> m_cache;
			for (const auto& vItem : vec_item)
			{
				rewardItem = ITEM_MANAGER::Instance().CreateItem(vItem->GetVnum(), vItem->GetCount(), 0, true);
				if (rewardItem) m_cache.emplace_back(rewardItem);
			}
			for (const auto& rItem : m_cache)
				vec_item.emplace_back(rItem);
		}
		eventPtr = CheckEventIsActive(DOUBLE_METIN_LOOT_EVENT, killerEmpire);
		if (eventPtr)
		{
			std::vector<LPITEM> m_cache;
			for (const auto& vItem : vec_item)
			{
				rewardItem = ITEM_MANAGER::Instance().CreateItem(vItem->GetVnum(), vItem->GetCount(), 0, true);
				if (rewardItem) m_cache.emplace_back(rewardItem);
			}
			for (const auto& rItem : m_cache)
				vec_item.emplace_back(rItem);
		}
	}

	if (pkChr->GetMobRank() == MOB_RANK_BOSS)
	{
		if (pkKiller->FindAffect(AFFECT_THIEF_GLOVES_BOSSES) && number(1,100) <= 5)
		{
			std::vector<LPITEM> m_cache;
			for (const auto& vItem : vec_item)
			{
				rewardItem = ITEM_MANAGER::Instance().CreateItem(vItem->GetVnum(), vItem->GetCount(), 0, true);
				if (rewardItem) m_cache.emplace_back(rewardItem);
			}
			for (const auto& rItem : m_cache)
				vec_item.emplace_back(rItem);
		}
		eventPtr = CheckEventIsActive(DOUBLE_BOSS_LOOT_EVENT, killerEmpire);
		if (eventPtr)
		{
			std::vector<LPITEM> m_cache;
			for (const auto& vItem : vec_item)
			{
				rewardItem = ITEM_MANAGER::Instance().CreateItem(vItem->GetVnum(), vItem->GetCount(), 0, true);
				if (rewardItem) m_cache.emplace_back(rewardItem);
			}
			for (const auto& rItem : m_cache)
				vec_item.emplace_back(rItem);
		}
	}

	// wheel of fortune start
	eventPtr = CheckEventIsActive(WHEEL_OF_FORTUNE_EVENT, killerEmpire);
	if (eventPtr)
	{
		const int expiration_date = eventPtr->value[2];
		if (pkChr->GetMobRank() == MOB_RANK_KING && pkChr->GetLevel() < 70)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(506024, 1, 0, true);
			if (item) 
			{
				item->SetSocket(0, expiration_date, false);
				vec_item.emplace_back(item);
			}
		}
		else if (pkChr->GetMobRank() == MOB_RANK_KING && pkChr->GetLevel() >= 70 && pkChr->GetLevel() < 90)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(506024, 2, 0, true);
			if (item) 
			{
				item->SetSocket(0, expiration_date, false);
				vec_item.emplace_back(item);
			}
		}
		else if (pkChr->GetMobRank() == MOB_RANK_KING && pkChr->GetLevel() >= 90)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(506024, 4, 0, true);
			if (item) 
			{
				item->SetSocket(0, expiration_date, false);
				vec_item.emplace_back(item);
			}
		}
		else if (pkChr->IsBetaBoss())
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(506024, 100, 0, true);
			if (item) 
			{
				item->SetSocket(0, expiration_date, false);
				vec_item.emplace_back(item);
			}
		}
		else if (pkChr->GetRaceNum() == 693 || pkChr->GetRaceNum() == 1093 || pkChr->GetRaceNum() == 2092 )
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(506024, 50, 0, true);
			if (item) 
			{
				item->SetSocket(0, expiration_date, false);
				vec_item.emplace_back(item);
			}
		}
		else if (pkChr->GetRaceNum() == 2493 || pkChr->GetRaceNum() == 2598 )
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(506024, 100, 0, true);
			if (item) 
			{
				item->SetSocket(0, expiration_date, false);
				vec_item.emplace_back(item);
			}
		}
		else if (pkChr->GetRaceNum() == 6192 || pkChr->GetRaceNum() == 6191 || pkChr->GetRaceNum() == 6091 || pkChr->GetRaceNum() == 4469 || pkChr->GetRaceNum() == 3965)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(506025, 1, 0, true);
			if (item) 
			{
				item->SetSocket(0, expiration_date, false);
				vec_item.emplace_back(item);
			}
		}
		else if (pkChr->GetRaceNum() == 6500 || pkChr->GetRaceNum() == 6501)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(506025, 1, 0, true);
			if (item) 
			{
				item->SetSocket(0, expiration_date, false);
				vec_item.emplace_back(item);
			}
			LPITEM item2 = ITEM_MANAGER::Instance().CreateItem(506024, 100, 0, true);
			if (item2) 
			{
				item2->SetSocket(0, expiration_date, false);
				vec_item.emplace_back(item2);
			}
		}
		else if (pkChr->IsZodiacBoss())
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(506025, 2, 0, true);
			if (item) 
			{
				item->SetSocket(0, expiration_date, false);
				vec_item.emplace_back(item);
			}
		}
	}
	// wheel of fortune end

	eventPtr = CheckEventIsActive(MOONLIGHT_EVENT, killerEmpire);
	if (eventPtr)
	{
		const int prob = number(1, 10000);
		const int success_prob = eventPtr->value[3];
		const int expiration_date = eventPtr->value[2];
		if (prob < success_prob)
		{
			// If your moonlight item vnum is different change 50011!
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(50011, 1, 0, true);
			if (item) 
			{
				item->SetSocket(0, expiration_date, false);
				vec_item.emplace_back(item);
			}
		}
	}

	eventPtr = CheckEventIsActive(OKEY_EVENT, killerEmpire);
	if (eventPtr)
	{
		const int prob = number(1, 10000);
		const int success_prob = eventPtr->value[3];
		const int expiration_date = eventPtr->value[2];
		if (prob < success_prob)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(79505, 1, 0, true);
			if (item)
			{
				item->SetSocket(0, expiration_date, false);
				vec_item.emplace_back(item);
			}
		}
	}

	eventPtr = CheckEventIsActive(GOLD_MANNY_EVENT, killerEmpire);
	if (eventPtr)
	{
		if (pkChr->IsStone())
		{
			const int prob = number(1, 10000);
			const int success_prob = eventPtr->value[3];

			if (prob < success_prob)
			{
				if (quest::CQuestManager::instance().GetEventFlag("legendary_manny_count") > 0)
				{
					LPITEM item = ITEM_MANAGER::Instance().CreateItem(79505, 1, 0, true);
					if (item)
					{
						vec_item.emplace_back(item);
						quest::CQuestManager::instance().RequestSetEventFlag("legendary_manny_count", (quest::CQuestManager::instance().GetEventFlag("legendary_manny_count") - 1));
					}
				}
			}
		}
	}
}

void CHARACTER_MANAGER::CompareEventSendData(TEMP_BUFFER* buf)
{
	const BYTE dayCount = m_eventData.size();
	const BYTE subIndex = EVENT_MANAGER_LOAD;
	const int cur_Time = time(NULL);
	TPacketGCEventManager p;
	p.header = HEADER_GC_EVENT_MANAGER;
	p.size = sizeof(TPacketGCEventManager) + sizeof(BYTE)+sizeof(BYTE)+sizeof(int);
	for (const auto& [dayIndex, dayData] : m_eventData)
	{
		const BYTE dayEventCount = dayData.size();
		p.size += sizeof(BYTE) + sizeof(BYTE) + (dayEventCount * sizeof(TEventManagerData));
	}
	buf->write(&p, sizeof(TPacketGCEventManager));
	buf->write(&subIndex, sizeof(BYTE));
	buf->write(&dayCount, sizeof(BYTE));
	buf->write(&cur_Time, sizeof(int));
	for (const auto& [dayIndex, dayData] : m_eventData)
	{
		const BYTE dayEventCount = dayData.size();
		buf->write(&dayIndex, sizeof(BYTE));
		buf->write(&dayEventCount, sizeof(BYTE));
		if (dayEventCount > 0)
			buf->write(dayData.data(), dayEventCount * sizeof(TEventManagerData));
	}
}
void CHARACTER_MANAGER::UpdateAllPlayerEventData()
{
	TEMP_BUFFER buf;
	CompareEventSendData(&buf);
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	for (const auto& desc : c_ref_set)
	{
		if (!desc->GetCharacter())
			continue;
		desc->Packet(buf.read_peek(), buf.size());
	}
}
void CHARACTER_MANAGER::SendDataPlayer(LPCHARACTER ch)
{
	auto desc = ch->GetDesc();
	if (!desc)
		return;
	TEMP_BUFFER buf;
	CompareEventSendData(&buf);
	desc->Packet(buf.read_peek(), buf.size());
}
bool CHARACTER_MANAGER::CloseEventManuel(BYTE eventIndex)
{
	auto eventPtr = CheckEventIsActive(eventIndex);
	if (eventPtr != NULL)
	{
		const BYTE subHeader = EVENT_MANAGER_REMOVE_EVENT;
		db_clientdesc->DBPacketHeader(HEADER_GD_EVENT_MANAGER, 0, sizeof(BYTE)+sizeof(WORD));
		db_clientdesc->Packet(&subHeader, sizeof(BYTE));
		db_clientdesc->Packet(&eventPtr->eventID, sizeof(WORD));
		return true;
	}
	return false;
}
void CHARACTER_MANAGER::SetEventStatus(const WORD eventID, const bool eventStatus, const int endTime, const char* endTimeText)
{
	//eventStatus - 0-deactive  // 1-active

	TEventManagerData* eventData = NULL;
	for (auto it = m_eventData.begin(); it != m_eventData.end(); ++it)
	{
		if (it->second.size())
		{
			for (DWORD j = 0; j < it->second.size(); ++j)
			{
				TEventManagerData& pData = it->second[j];
				if (pData.eventID == eventID)
				{
					eventData = &pData;
					break;
				}
			}
		}
	}
	if (eventData == NULL)
		return;
	eventData->eventStatus = eventStatus;
	eventData->endTime = endTime;
	strlcpy(eventData->endTimeText, endTimeText, sizeof(eventData->endTimeText));

	// Auto open&close notice
	const std::map<BYTE, std::pair<std::string, std::string>> m_eventText = {
		{BONUS_EVENT,{"761","762"}},
		{DOUBLE_BOSS_LOOT_EVENT,{"763","764"}},
		{DOUBLE_METIN_LOOT_EVENT,{"765","766"}},
		{DUNGEON_TICKET_LOOT_EVENT,{"771","772"}},
		{MOONLIGHT_EVENT,{"773","774"}},
	};

	const auto it = m_eventText.find(eventData->eventIndex);
	if (it != m_eventText.end())
	{
		if (eventStatus)
			SendNotice(it->second.first.c_str());
		else
			SendNotice(it->second.second.c_str());
	}

	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();

	// Bonus event update status
	if (eventData->eventIndex == BONUS_EVENT)
	{
		for (const auto& desc : c_ref_set)
		{
			LPCHARACTER ch = desc->GetCharacter();
			if (!ch)
				continue;
			if (eventData->empireFlag != 0)
				if (eventData->empireFlag != ch->GetEmpire())
					continue;
			if (eventData->channelFlag != 0)
				if (eventData->channelFlag != g_bChannel)
					return;
			if (!eventStatus)
			{
				const long value = eventData->value[1];
				ch->ApplyPoint(eventData->value[0], -value);
			}
			ch->ComputePoints();
		}
	}

	const int now = time(0);
	const BYTE subIndex = EVENT_MANAGER_EVENT_STATUS;
	
	TPacketGCEventManager p;
	p.header = HEADER_GC_EVENT_MANAGER;
	p.size = sizeof(TPacketGCEventManager)+sizeof(BYTE)+sizeof(WORD)+sizeof(bool)+sizeof(int)+sizeof(int)+sizeof(eventData->endTimeText);
	
	TEMP_BUFFER buf;
	buf.write(&p, sizeof(TPacketGCEventManager));
	buf.write(&subIndex, sizeof(BYTE));
	buf.write(&eventData->eventID, sizeof(WORD));
	buf.write(&eventData->eventStatus, sizeof(bool));
	buf.write(&eventData->endTime, sizeof(int));
	buf.write(&eventData->endTimeText, sizeof(eventData->endTimeText));
	buf.write(&now, sizeof(int));

	for (const auto& desc : c_ref_set)
	{
		if (!desc->GetCharacter())
			continue;
		desc->Packet(buf.read_peek(), buf.size());
	}

}
void CHARACTER_MANAGER::SetEventData(BYTE dayIndex, const std::vector<TEventManagerData>& m_data)
{
	const auto it = m_eventData.find(dayIndex);
	if (it == m_eventData.end())
		m_eventData.emplace(dayIndex, m_data);
	else
	{
		it->second.clear();
		for (const auto& newEvents : m_data)
			it->second.emplace_back(newEvents);
	}
}
#endif

#ifdef ENABLE_REWARD_SYSTEM
const char* GetRewardIndexToString(BYTE rewardIndex)
{
	std::map<BYTE, std::string> rewardNames = {
		{REWARD_120,"REWARD_120"},
		{REWARD_PET_115,"REWARD_PET_115"},
		{REWARD_LEGENDARY_SKILL,"REWARD_LEGENDARY_SKILL"},
		{REWARD_LEGENDARY_SKILL_SET,"REWARD_LEGENDARY_SKILL_SET"},
		{REWARD_HYDRA,"REWARD_HYDRA"},
		{REWARD_MELEY,"REWARD_MELEY"},
		{REWARD_AKZADUR,"REWARD_AKZADUR"},
		{REWARD_EKZEKIEL,"REWARD_EKZEKIEL"},
		{REWARD_AVERAGE,"REWARD_AVERAGE"},
		{REWARD_INVEN_SLOT,"REWARD_INVEN_SLOT"},
		{REWARD_OFFLINE_SLOT,"REWARD_OFFLINE_SLOT"},
		{REWARD_BATTLEPASS,"REWARD_BATTLEPASS"},
		{REWARD_ACCE,"REWARD_ACCE"},
		{REWARD_LEADERSHIP,"REWARD_LEADERSHIP"},
		{REWARD_CRYSTAL,"REWARD_CRYSTAL"},
		{REWARD_BUFFI,"REWARD_BUFFI"},
		{REWARD_BIOLOG,"REWARD_BIOLOG"},
		{REWARD_ELEMENT_PAGE,"REWARD_ELEMENT_PAGE"},
		{REWARD_FIRST_WEAPON_ZODIAC_CRAFT,"REWARD_FIRST_WEAPON_ZODIAC_CRAFT"},
		{FIRST_AVERAGE_ZODIAC_50,"FIRST_AVERAGE_ZODIAC_50"},

	};
	auto it = rewardNames.find(rewardIndex);
	if (it != rewardNames.end())
		return it->second.c_str();
	return 0;
}
BYTE GetRewardIndex(const char* szRewardName)
{
	std::map<std::string, BYTE> rewardNames = {
		{"REWARD_120",REWARD_120},
		{"REWARD_PET_115",REWARD_PET_115},
		{"REWARD_LEGENDARY_SKILL",REWARD_LEGENDARY_SKILL},
		{"REWARD_LEGENDARY_SKILL_SET",REWARD_LEGENDARY_SKILL_SET},
		{"REWARD_HYDRA",REWARD_HYDRA},
		{"REWARD_MELEY",REWARD_MELEY},
		{"REWARD_AKZADUR",REWARD_AKZADUR},
		{"REWARD_EKZEKIEL",REWARD_EKZEKIEL},
		{"REWARD_AVERAGE",REWARD_AVERAGE},
		{"REWARD_INVEN_SLOT",REWARD_INVEN_SLOT},
		{"REWARD_OFFLINE_SLOT",REWARD_OFFLINE_SLOT},
		{"REWARD_BATTLEPASS",REWARD_BATTLEPASS},
		{"REWARD_ACCE",REWARD_ACCE},
		{"REWARD_LEADERSHIP",REWARD_LEADERSHIP},
		{"REWARD_CRYSTAL",REWARD_CRYSTAL},
		{"REWARD_BUFFI",REWARD_BUFFI},
		{"REWARD_BIOLOG",REWARD_BIOLOG},
		{"REWARD_ELEMENT_PAGE",REWARD_ELEMENT_PAGE},
		{"REWARD_FIRST_WEAPON_ZODIAC_CRAFT",REWARD_FIRST_WEAPON_ZODIAC_CRAFT},
		{"FIRST_AVERAGE_ZODIAC_50",FIRST_AVERAGE_ZODIAC_50},
	};
	
	auto it = rewardNames.find(szRewardName);
	if (it != rewardNames.end())
		return it->second;
	return 0;
}
void CHARACTER_MANAGER::LoadRewardData()
{
	m_rewardData.clear();

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "SELECT rewardIndex, lc_text, playerName, itemVnum0, itemCount0, itemVnum1, itemCount1, itemVnum2, itemCount2 FROM player.reward_table");
	std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szQuery));

	if (pMsg->Get()->uiNumRows != 0)
	{
		MYSQL_ROW row = NULL;
		for (int n = 0; (row = mysql_fetch_row(pMsg->Get()->pSQLResult)) != NULL; ++n)
		{
			int col = 0;
			TRewardInfo p;
			p.m_rewardItems.clear();

			char rewardName[50];
			DWORD rewardIndex;
			strlcpy(rewardName, row[col++], sizeof(rewardName));
			rewardIndex = GetRewardIndex(rewardName);

			strlcpy(p.lc_text, row[col++], sizeof(p.lc_text));
			strlcpy(p.playerName, row[col++], sizeof(p.playerName));
			for (BYTE j = 0; j < 3; ++j)
			{
				DWORD itemVnum, itemCount;
				str_to_number(itemVnum, row[col++]);
				str_to_number(itemCount, row[col++]);
				p.m_rewardItems.emplace_back(itemVnum, itemCount);
			}
			m_rewardData.emplace(rewardIndex, p);
		}
	}
}
bool CHARACTER_MANAGER::IsRewardEmpty(BYTE rewardIndex)
{
	auto it = m_rewardData.find(rewardIndex);
	if (it != m_rewardData.end())
	{
		if (strcmp(it->second.playerName, "") == 0)
			return true;
	}
	return false;
}
void CHARACTER_MANAGER::SendRewardInfo(LPCHARACTER ch)
{
	ch->SetProtectTime("RewardInfo", 1);
	std::string cmd="";//12
	if (m_rewardData.size())
	{
		for (auto it = m_rewardData.begin(); it != m_rewardData.end(); ++it)
		{
			if (strlen(it->second.playerName) > 1)
			{
				char text[60];
				snprintf(text, sizeof(text), "%d|%s#", it->first, it->second.playerName);
				cmd += text;
			}
			if (strlen(cmd.c_str()) + 12 > CHAT_MAX_LEN - 30)
			{
				ch->ChatPacket(CHAT_TYPE_COMMAND, "RewardInfo %s", cmd.c_str());
				cmd = "";
			}
		}
		if (strlen(cmd.c_str()) > 1)
			ch->ChatPacket(CHAT_TYPE_COMMAND, "RewardInfo %s", cmd.c_str());
	}
}

struct RewardForEach
{
	void operator() (LPDESC d)
	{
		LPCHARACTER ch = d->GetCharacter();
		if (ch == NULL)
			return;
		else if (ch->GetProtectTime("RewardInfo") != 1)
			return;
		CHARACTER_MANAGER::Instance().SendRewardInfo(ch);
	}
};
void CHARACTER_MANAGER::SetRewardData(BYTE rewardIndex, const char* playerName, bool isP2P)
{
	if (!IsRewardEmpty(rewardIndex))
		return;

	auto it = m_rewardData.find(rewardIndex);
	if (it == m_rewardData.end())
		return;

	TRewardInfo& rewardInfo = it->second;
	LPCHARACTER ch = FindPC(playerName);

	if (ch && ch->IsGM() && isP2P)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Sorry, but staff can't take reward >.<");
		return;
	}

	strlcpy(rewardInfo.playerName, playerName, sizeof(rewardInfo.playerName));

	if (isP2P)
	{		

		if (ch)
		{
			for (DWORD j = 0; j < rewardInfo.m_rewardItems.size(); ++j)
				ch->AutoGiveItem(rewardInfo.m_rewardItems[j].first, rewardInfo.m_rewardItems[j].second);
		}

		TPacketGGRewardInfo p;
		p.bHeader = HEADER_GG_REWARD_INFO;
		p.rewardIndex = rewardIndex;
		strlcpy(p.playerName, playerName, sizeof(p.playerName));
		P2P_MANAGER::Instance().Send(&p, sizeof(p));

		char szQuery[4096];
		snprintf(szQuery, sizeof(szQuery), "UPDATE player.reward_table SET playerName = '%s' WHERE lc_text = '%s'", playerName, rewardInfo.lc_text);
		auto pMsg(DBManager::instance().DirectQuery(szQuery));
	}
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), RewardForEach());
}
#endif

#ifdef ENABLE_BATTLE_PASS
BYTE GetBattlePassMissionIndex(const char* szMissionName)
{
	std::map<std::string, BYTE> eventNames = {
		{"MISSION_NONE",MISSION_NONE},
		{"MISSION_BOSS",MISSION_BOSS},
		{"MISSION_CATCH_FISH",MISSION_CATCH_FISH},
		{"MISSION_CRAFT_ITEM",MISSION_CRAFT_ITEM},
		{"MISSION_CRAFT_GAYA",MISSION_CRAFT_GAYA},
		{"MISSION_DESTROY_ITEM",MISSION_DESTROY_ITEM},
		{"MISSION_DUNGEON",MISSION_DUNGEON},
		{"MISSION_EARN_MONEY",MISSION_EARN_MONEY},
		{"MISSION_FEED_PET",MISSION_FEED_PET},
		{"MISSION_LEVEL_UP",MISSION_LEVEL_UP},
		{"MISSION_MONSTER",MISSION_MONSTER},
		{"MISSION_MOUNT_TIME",MISSION_MOUNT_TIME},
		{"MISSION_OPEN_OFFLINESHOP",MISSION_OPEN_OFFLINESHOP},
		{"MISSION_PLAYTIME",MISSION_PLAYTIME},
		{"MISSION_REFINE_ITEM",MISSION_REFINE_ITEM},
		{"MISSION_REFINE_ALCHEMY",MISSION_REFINE_ALCHEMY},
		{"MISSION_SASH",MISSION_SASH},
		{"MISSION_SELL_ITEM",MISSION_SELL_ITEM},
		{"MISSION_SPEND_MONEY",MISSION_SPEND_MONEY},
		{"MISSION_SPRITE_STONE",MISSION_SPRITE_STONE},
		{"MISSION_STONE",MISSION_STONE},
		{"MISSION_USE_EMOTICON",MISSION_USE_EMOTICON},
		{"MISSION_WHISPER",MISSION_WHISPER},
		{"MISSION_SHOUT_CHAT",MISSION_SHOUT_CHAT},
		{"MISSION_KILLPLAYER",MISSION_KILLPLAYER},
	};
	auto it = eventNames.find(szMissionName);
	if (it != eventNames.end())
		return it->second;
	return MISSION_NONE;
}

void CHARACTER_MANAGER::DoMission(LPCHARACTER ch, BYTE missionIndex, long long value, DWORD subValue)
{
	if (!ch->IsBattlePassActive())
		return;
	else if (!IsBattlePassMissionHave(missionIndex))
		return;

	DWORD missionSubValue = GetMissionSubValue(missionIndex);
	if (missionSubValue != 0)
		if (subValue != missionSubValue)
			return;

	long long missionMaxValue = GetMissionMaxValue(missionIndex);
	long long selfValue = ch->GetBattlePassMissonValue(missionIndex);
	if (selfValue >= missionMaxValue)
		return;

	long long newValue = selfValue + std::abs(value);
	if (newValue >= missionMaxValue)
	{
		newValue = missionMaxValue;
		ch->SetBattlePassMissonValue(missionIndex, newValue);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("776"));
		GiveLowMissionReward(ch, missionIndex);
		if (CheckBattlePassDone(ch))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("777"));
		}
	}
	else
		ch->SetBattlePassMissonValue(missionIndex, newValue);
	if (ch->GetProtectTime("battlePassOpen") == 1)
		ch->ChatPacket(CHAT_TYPE_COMMAND, "BattlePassSetMission %d %lld", missionIndex, newValue);
}

bool CHARACTER_MANAGER::IsBattlePassMissionHave(BYTE missionIndex)
{
	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);
	auto it = m_BattlePassData.find(vKey.tm_mon);
	if (it != m_BattlePassData.end())
	{
		if (it->second.second.missionData.find(missionIndex) != it->second.second.missionData.end())
			return true;
	}
	return false;
}
void CHARACTER_MANAGER::GiveLowMissionReward(LPCHARACTER ch, BYTE missionIndex)
{
	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);
	auto it = m_BattlePassData.find(vKey.tm_mon);
	if (it != m_BattlePassData.end())
	{
		auto lowReward = it->second.second.subReward.find(missionIndex);
		if (lowReward != it->second.second.subReward.end())
		{
			for (DWORD j = 0; j < lowReward->second.size(); ++j)
				ch->AutoGiveItem(lowReward->second[j].first, lowReward->second[j].second);
		}
	}
}
long long CHARACTER_MANAGER::GetMissionMaxValue(BYTE missionIndex)
{
	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);
	auto it = m_BattlePassData.find(vKey.tm_mon);
	if (it != m_BattlePassData.end())
	{
		auto itEx = it->second.second.missionData.find(missionIndex);
		if (itEx != it->second.second.missionData.end())
			return itEx->second.first;
	}
	return 0;
}
DWORD CHARACTER_MANAGER::GetMissionSubValue(BYTE missionIndex)
{
	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);
	auto it = m_BattlePassData.find(vKey.tm_mon);
	if (it != m_BattlePassData.end())
	{
		auto itEx = it->second.second.missionData.find(missionIndex);
		if (itEx != it->second.second.missionData.end())
			return itEx->second.second;
	}
	return 0;
}
bool CHARACTER_MANAGER::CheckBattlePassDone(LPCHARACTER ch)
{
	if (!ch->IsBattlePassActive())
		return false;
	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);
	auto it = m_BattlePassData.find(vKey.tm_mon);
	if (it != m_BattlePassData.end())
	{
		const auto missionData = it->second.second.missionData;
		if (missionData.size())
		{
			for (auto itMission = missionData.begin(); itMission != missionData.end(); ++itMission)
			{
				if (ch->GetBattlePassMissonValue(itMission->first) < itMission->second.first)
					return false;
			}
		}
	}
	return true;
}
void CHARACTER_MANAGER::CheckBattlePassReward(LPCHARACTER ch)
{
	if(!CheckBattlePassDone(ch))
		return;
	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);
	auto it = m_BattlePassData.find(vKey.tm_mon);
	if (it != m_BattlePassData.end())
	{
		const auto rewardData = it->second.second.rewardData;
		if (rewardData.size())
		{
			for (auto itReward = rewardData.begin(); itReward != rewardData.end(); ++itReward)
				ch->AutoGiveItem(itReward->first, itReward->second);
			SetRewardData(REWARD_BATTLEPASS, ch->GetName(), true);
			ch->SetStatistics(STAT_TYPE_BATTLEPASS, 1);
		}
	}
	ch->ClearBattlePassData();
	ch->SetQuestFlag("battlePass.status",2);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "BattlePassSetStatusEx 2");
}
void CHARACTER_MANAGER::LoadBattlePassData(LPCHARACTER ch)
{
	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);
	auto it = m_BattlePassData.find(vKey.tm_mon);
	if(it != m_BattlePassData.end())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "BattlePassClear");
		std::string rewardText = "";
		const auto missionData = it->second.second.missionData;
		if (missionData.size())
		{
			for (auto itMission = missionData.begin(); itMission != missionData.end(); ++itMission)
			{
				const auto subReward = it->second.second.subReward;
				auto rewardList = subReward.find(itMission->first);
				if (rewardList != subReward.end())
				{
					const auto itRewardList = rewardList->second;
					if (itRewardList.size() == 0)
						rewardText = "Empty";
					else
					{
						for (BYTE j = 0; j < itRewardList.size(); ++j)
						{
							rewardText += std::to_string(itRewardList[j].first);
							rewardText += "|";
							rewardText += std::to_string(itRewardList[j].second);
							rewardText += "#";
						}
					}
				}

				int vecList = 0;
				const auto missinvecIndexList = it->second.second.missionIndex;
				auto missinvecIndex = missinvecIndexList.find(itMission->first);
				if (missinvecIndex != missinvecIndexList.end())
					vecList = missinvecIndex->second;

				sys_log(0,"BattlePassAppendMission %u %s %lld %lld %u %d", itMission->first, rewardText.c_str(), ch->GetBattlePassMissonValue(itMission->first), itMission->second.first, itMission->second.second, vecList);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "BattlePassAppendMission %u %s %lld %lld %u %d", itMission->first, rewardText.c_str(), ch->GetBattlePassMissonValue(itMission->first), itMission->second.first, itMission->second.second, vecList);

				rewardText = "";
			}
		}
		const auto rewardData = it->second.second.rewardData;
		for (DWORD j = 0; j < rewardData.size(); ++j)
		{
			rewardText += std::to_string(rewardData[j].first);
			rewardText += "|";
			rewardText += std::to_string(rewardData[j].second);
			rewardText += "#";
		}
		ch->ChatPacket(CHAT_TYPE_COMMAND, "BattlePassSetStatus %d %d %s", ch->GetBattlePassStatus(), it->second.first-time(0), rewardText.c_str());
	}
}
void CHARACTER_MANAGER::InitializeBattlePass()
{
	m_BattlePassData.clear();
	std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT * FROM player.battle_pass"));
	if (pMsg->Get()->uiNumRows != 0)
	{
		time_t cur_Time = time(NULL);
		struct tm vKey = *localtime(&cur_Time);
		MYSQL_ROW row = NULL;
		
		for (int n = 0; (row = mysql_fetch_row(pMsg->Get()->pSQLResult)) != NULL; ++n)
		{
			int c = 0;
			int monthIndex;
			str_to_number(monthIndex, row[c++]);

			int year = vKey.tm_year+1900;
			int month = monthIndex;
			if (month >= 12)
			{
				year += 1;
				month = 0;
			}

			struct tm t;
			t.tm_year = year - 1900;
			t.tm_mon = month;
			t.tm_mday = 1;
			t.tm_isdst = 0;
			t.tm_hour = 0;
			t.tm_min = 0;
			t.tm_sec = 0;
			time_t lastTime = mktime(&t);

			//sys_err("year %d month %d lasttime %d realTime %d", year, month, lastTime, lastTime-time(0));

			BattlePassData p;
			for (DWORD j = 0; j < BATTLE_MISSION_MAX; ++j)
			{
				char missionName[100];
				strlcpy(missionName, row[c++], sizeof(missionName));
				BYTE missionIndex = GetBattlePassMissionIndex(missionName);
				DWORD missionMaxValue, missionSubValue;
				str_to_number(missionMaxValue, row[c++]);
				str_to_number(missionSubValue, row[c++]);

				std::vector<std::pair<DWORD, DWORD>> m_vec;
				m_vec.clear();
				for (DWORD j = 0; j < BATTLE_SUB_REWARD; ++j)
				{
					DWORD itemVnum, itemCount;
					str_to_number(itemVnum, row[c++]);
					str_to_number(itemCount, row[c++]);
					if (itemVnum > 0)
						m_vec.emplace_back(itemVnum, itemCount);
				}

				if (missionIndex > 0)
				{
					auto it = p.missionData.find(missionIndex);
					if (it == p.missionData.end())
					{
						p.subReward.emplace(missionIndex, m_vec);
						p.missionIndex.emplace(missionIndex, j + 1);
						p.missionData.emplace(missionIndex, std::make_pair(missionMaxValue, missionSubValue));
					}
					else
						sys_err("battlepass: duplicate mission in same month! monthIndex %d missionIndex %d", monthIndex, missionIndex);
				}
			}
			for (DWORD j = 0; j < BATTLE_REWARD_MAX; ++j)
			{
				DWORD itemVnum, itemCount;
				str_to_number(itemVnum, row[c++]);
				str_to_number(itemCount, row[c++]);
				if (itemVnum != 0 && itemCount != 0)
					p.rewardData.emplace_back(itemVnum, itemCount);
			}
			m_BattlePassData.emplace(monthIndex-1, std::make_pair(lastTime,p));
		}
	}
}
#endif

#ifdef __DUNGEON_INFO__
bool sortByTime(const TDungeonRank& a, const TDungeonRank& b){return a.value < b.value;}
bool sortByVal(const TDungeonRank& a, const TDungeonRank& b){return a.value > b.value;}
void CHARACTER_MANAGER::SendDungeonRank(LPCHARACTER ch, DWORD mobIdx, BYTE rankIdx)
{
	const auto it = m_mapDungeonList.find(mobIdx);
	if (it == m_mapDungeonList.end())
		return;

	bool reLoad = false;

	auto itMob = m_mapDungeonRank.find(mobIdx);
	if (itMob == m_mapDungeonRank.end())
	{
		std::map<BYTE, std::pair<std::vector<TDungeonRank>, int>> m_data;
		m_mapDungeonRank.emplace(mobIdx, m_data);
		itMob = m_mapDungeonRank.find(mobIdx);
		reLoad = true;
	}

	auto itRank = itMob->second.find(rankIdx);
	if (itRank == itMob->second.end())
	{
		reLoad = true;
		std::pair<std::vector<TDungeonRank>, int> m_vec;
		itMob->second.emplace(rankIdx, m_vec);
		itRank = itMob->second.find(rankIdx);
	}

	if (!reLoad && itRank->second.second < time(0))
		reLoad = true;


	if (reLoad)
	{
		itRank->second.first.clear();
		char szQuery[1024];
		if(rankIdx == 0)
			snprintf(szQuery, sizeof(szQuery), "SELECT dwPID, lValue FROM player.quest WHERE szName = 'dungeon' and szState = '%u_completed' and lValue > 0 ORDER BY lValue DESC LIMIT 10", mobIdx);
		else if(rankIdx == 1)
			snprintf(szQuery, sizeof(szQuery), "SELECT dwPID, lValue FROM player.quest WHERE szName = 'dungeon' and szState = '%u_fastest' and lValue > 0 ORDER BY lValue ASC LIMIT 10", mobIdx);
		else if (rankIdx == 2)
			snprintf(szQuery, sizeof(szQuery), "SELECT dwPID, lValue FROM player.quest WHERE szName = 'dungeon' and szState = '%u_damage' and lValue > 0 ORDER BY lValue DESC LIMIT 10", mobIdx);

		std::unique_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery(szQuery));
		if (pMsg->Get()->pSQLResult)
		{
			MYSQL_ROW mRow;
			while (NULL != (mRow = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			{
				TDungeonRank dungeonRank;
				memset(&dungeonRank, 0, sizeof(dungeonRank));
				DWORD playerID;
				str_to_number(playerID, mRow[0]);
				str_to_number(dungeonRank.value, mRow[1]);
				snprintf(szQuery, sizeof(szQuery), "SELECT name, level FROM player.player WHERE id = %d", playerID);
				std::unique_ptr<SQLMsg> pMsg2(DBManager::instance().DirectQuery(szQuery));
				MYSQL_ROW  playerRow = mysql_fetch_row(pMsg2->Get()->pSQLResult);
				if (playerRow)
				{
					strlcpy(dungeonRank.name, playerRow[0], sizeof(dungeonRank.name));
					str_to_number(dungeonRank.level, playerRow[1]);
				}
				itRank->second.first.emplace_back(dungeonRank);
			}
			if (itRank->second.first.size())
			{
				if(rankIdx == 1)
					std::sort(itRank->second.first.begin(), itRank->second.first.end(), sortByTime);
				else
					std::sort(itRank->second.first.begin(), itRank->second.first.end(), sortByVal);
			}
		}
		itRank->second.second = time(0) + (60 * 5);
	}

	std::string cmd("");

	for (BYTE j = 0; j < itRank->second.first.size(); ++j)
	{
		const auto& rank = itRank->second.first[j];

		cmd += rank.name;
		cmd += "|";
		cmd += std::to_string(rank.value);
		cmd += "|";
		cmd += std::to_string(rank.level);
		cmd += "#";
	}
	if (cmd == "")
		cmd = "-";
	ch->ChatPacket(CHAT_TYPE_COMMAND, "dungeon_log_info %u %u %s", mobIdx, rankIdx, cmd.c_str());
}
#endif

bool CHARACTER_MANAGER::CheckMeleyStatues(LPCHARACTER ch)
{
	if (!ch)
		return false;

	const DWORD dungeonIdx = ch->GetMapIndex();

	LPDUNGEON dungeon = CDungeonManager::Instance().FindByMapIndex(dungeonIdx);
	if (!dungeon)
		return false;

	if (dungeon->GetFlag("stage1_active") == 1)
		return true;

	if (dungeon->GetFlag("stage2_active") == 1)
		return true;

	if (dungeon->GetFlag("stage3_active") == 1)
		return true;

	if (dungeon->GetFlag("stage4_active") == 1)
		return true;

	return false;
}

int CHARACTER_MANAGER::CheckMeleyStatuesHP(LPCHARACTER ch, int hp)
{
	if (!ch)
		return 0;

	const DWORD dungeonIdx = ch->GetMapIndex();

	LPDUNGEON dungeon = CDungeonManager::Instance().FindByMapIndex(dungeonIdx);
	if (!dungeon)
		return 0;

	if (dungeon->GetFlag("stage1_active") == 1 && hp >= hp*0.75)
		return hp*0.75;

	if (dungeon->GetFlag("stage2_active") == 1 && hp >= hp*0.50)
		return hp*0.50;

	if (dungeon->GetFlag("stage3_active") == 1 && hp >= hp*0.25)
		return hp*0.25;

	if (dungeon->GetFlag("stage4_active") == 1 && hp >= hp*0.01)
		return hp*0.01;

	return 0;
}

bool CHARACTER_MANAGER::CheckTestDungeon(LPCHARACTER ch)
{
	if (!ch)
		return false;

	const DWORD dungeonIdx = ch->GetMapIndex();

	if (ch->GetQuestFlag("ship_defense.test_dungeon") == 1 && ch->GetMapIndex() >= 3580000 && ch->GetMapIndex() <= 3589900)
		return true;

	if (dungeonIdx < 10000)
		return false;

	LPDUNGEON dungeon = CDungeonManager::Instance().FindByMapIndex(dungeonIdx);
	if (!dungeon)
		return false;

	if (dungeonIdx != dungeon->GetMapIndex())
		return false;

	if (dungeon->GetFlag("dungeon_test") != 1)
		return false;

	return true;
}

#ifdef __BACK_DUNGEON__
bool CHARACTER_MANAGER::CheckBackDungeon(LPCHARACTER ch, BYTE checkType, DWORD mapIdx)
{
	if (!ch && checkType != BACK_DUNGEON_REMOVE)
		return false;

	// std::map<DWORD, std::map<std::pair<DWORD, WORD>, std::vector<DWORD>>> m_mapbackDungeon;

	if (checkType == BACK_DUNGEON_SAVE)
	{
		const DWORD dungeonIdx = ch->GetMapIndex();
		if (dungeonIdx < 10000)
			return false;
		LPDUNGEON dungeon = CDungeonManager::Instance().FindByMapIndex(dungeonIdx);
		if (!dungeon)
			return false;
		if (dungeon->GetFlag("can_back_dungeon") != 1)
			return false;
		const DWORD mapIdx = dungeonIdx / 10000;
		auto it = m_mapbackDungeon.find(mapIdx);
		if (it == m_mapbackDungeon.end())
		{
			std::map<std::pair<DWORD, WORD>, std::vector<DWORD>> m_mapList;
			m_mapbackDungeon.emplace(mapIdx, m_mapList);
			it = m_mapbackDungeon.find(mapIdx);
		}
		auto itDungeon = it->second.find(std::make_pair(dungeonIdx, mother_port));
		if (itDungeon == it->second.end())
		{
			std::vector<DWORD> m_pidList;
			it->second.emplace(std::make_pair(dungeonIdx, mother_port), m_pidList);
			itDungeon = it->second.find(std::make_pair(dungeonIdx, mother_port));
		}
		if(std::find(itDungeon->second.begin(), itDungeon->second.end(),ch->GetPlayerID()) == itDungeon->second.end())
			itDungeon->second.emplace_back(ch->GetPlayerID());
	}
	else if (checkType == BACK_DUNGEON_REMOVE)
	{
		if (mapIdx < 10000)
			return false;

		auto it = m_mapbackDungeon.find(mapIdx / 10000);
		if (it != m_mapbackDungeon.end())
		{
			for (auto& [dungeonData, playerList] : it->second)
			{
				if (mapIdx == dungeonData.first)
				{
					it->second.erase(dungeonData);
					return true;
				}
			}
		}
	}
	else if (checkType == BACK_DUNGEON_CHECK || checkType == BACK_DUNGEON_WARP)
	{
		const DWORD curretMapIdx = ch->GetMapIndex();
		if (curretMapIdx > 10000)
			return false;

		const std::map<DWORD, std::vector<DWORD>> m_baseToDungeonIdx = {
			{
				65, // milgyo
				{
					66,//devil-tower
					216,//devilcatacomb
				},
			},
			{
				64, // threeway
				{
					354, // ork
				},
			},
			{
				217, // sd3
				{
					365, // sd3
				},
			},
			{
				73, // v4
				{
					208, // beran
				},
			},
			{
				62,// flame
				{
					351,// flame dungeon
					361,// meley
				},
			},
			{
				61, // snow
				{
					352,// snow dungeon
				},
			},
			{
				356, // dawnmistwood
				{
					364,// jotun
				},
			},
			{
				301, // cape
				{
					358,// ship defense
				},
			},
			{
				355, // akzadur
				{
					363,// akzadur
					362,// nafaroth
				},
			},
			{
				366, // zodiac
				{
					367,// zodiac ingame
				},
			}
		};

		const auto itBase = m_baseToDungeonIdx.find(curretMapIdx);
		if (itBase != m_baseToDungeonIdx.end())
		{
			for (const auto& dungeonIdx : itBase->second)
			{
				auto it = m_mapbackDungeon.find(dungeonIdx);
				if (it != m_mapbackDungeon.end())
				{
					for (auto& [dungeonData, playerList] : it->second)
					{
						LPDUNGEON dungeon = CDungeonManager::Instance().FindByMapIndex(dungeonData.first);
						if (std::find(playerList.begin(), playerList.end(), ch->GetPlayerID()) == playerList.end())
							continue;
						else if (!dungeon)
							continue;
						else if (dungeon->GetFlag("can_back_dungeon") != 1)
							continue;
						if (checkType == BACK_DUNGEON_WARP)
						{
							if (mapIdx == dungeonData.first)
							{
								if (ch->IsRiding())
									ch->StopRiding();
									
								ch->WarpSet(dungeon->GetFlag("LAST_X"), dungeon->GetFlag("LAST_Y"), dungeonData.first);
								return true;
							}
						}
						else
							ch->ChatPacket(CHAT_TYPE_COMMAND, "ReJoinDungeonDialog %d", dungeonData.first);
					}
				}
			}
		}
	}
	return false;
}
#endif


//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
