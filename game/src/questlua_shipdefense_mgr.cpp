/**
* Filename: questlua_shipdefense_mgr.cpp
* Title: Ship Defense
* Description: Quest Ship Defense Manager
* Author: Owsap
* Date: 2022.08.05
*
* Discord: Owsap#7928
* Skype: owsap.
*
* Web: https://owsap.dev/
* GitHub: https://github.com/Owsap
**/

#include "stdafx.h"

#if defined(__SHIP_DEFENSE__)

#include "questlua.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "ShipDefense.h"
#include "char.h"
#include "char_manager.h"

namespace quest
{
	int ship_defense_mgr_create(lua_State* L)
	{
		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		if (!rkShipDefenseMgr.Create(c_lpChar))
		{
			sys_err("Failed to create ship defense instance.");
			return 0;
		}

		return 0;
	}


	int ship_defense_mgr_set_test(lua_State* L)
	{
		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		rkShipDefenseMgr.SetTest(c_lpChar, lua_toboolean(L, 1));
		return 0;
	}


	int ship_defense_mgr_start(lua_State* L)
	{
		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		if (rkShipDefenseMgr.IsRunning(c_lpChar))
		{
			return 0;
		}

		if (!rkShipDefenseMgr.Start(c_lpChar))
		{
			sys_err("Cannot start ship defense, instance not created.");
			return 0;
		}

		return 0;
	}

	int ship_defense_mgr_start_test(lua_State* L)
	{
		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		if (rkShipDefenseMgr.IsRunning(c_lpChar))
		{
			return 0;
		}

		if (!rkShipDefenseMgr.StartTest(c_lpChar))
		{
			sys_err("Cannot start ship defense, instance not created.");
			return 0;
		}

		return 0;
	}
	int ship_defense_mgr_join(lua_State* L)
	{
		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		rkShipDefenseMgr.Join(c_lpChar);
		return 0;
	}

	int ship_defense_mgr_leave(lua_State* L)
	{
		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		rkShipDefenseMgr.Leave(c_lpChar);
		return 0;
	}

	int ship_defense_mgr_land(lua_State* L)
	{
		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		rkShipDefenseMgr.Land(c_lpChar);
		return 0;
	}

	int ship_defense_mgr_is_created(lua_State* L)
	{
		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		lua_pushboolean(L, rkShipDefenseMgr.IsCreated(c_lpChar));
		return 1;
	}

	int ship_defense_mgr_is_running(lua_State* L)
	{
		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		lua_pushboolean(L, rkShipDefenseMgr.IsRunning(c_lpChar));
		return 1;
	}

	int ship_defense_mgr_need_party(lua_State* L)
	{
		lua_pushboolean(L, ShipDefense::NEED_PARTY);
		return 1;
	}

	int ship_defense_mgr_need_ticket(lua_State* L)
	{
		lua_pushboolean(L, ShipDefense::NEED_TICKET);
		return 1;
	}

	int ship_defense_mgr_spawn_wood_repair(lua_State* L)
	{
		lua_pushboolean(L, ShipDefense::SPAWN_WOOD_REPAIR);
		return 1;
	}

	int ship_defense_mgr_require_cooldown(lua_State* L)
	{
		lua_pushboolean(L, ShipDefense::REQUIRE_COOLDOWN);
		return 1;
	}

	int ship_defense_mgr_set_alliance_hp_pct(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("Wrong argument.");
			return 0;
		}

		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		const BYTE c_byPct = static_cast<BYTE>(lua_tonumber(L, 1));

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		rkShipDefenseMgr.SetAllianceHPPct(c_lpChar, c_byPct);
		return 0;
	}

	ALUA(ship_increase_completed)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("Wrong argument.");
			return 0;
		}

		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		rkShipDefenseMgr.IncreaseCompleted(c_lpChar);

		return 0;
	}

	ALUA(ship_set_fastest)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("Wrong argument.");
			return 0;
		}

		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		rkShipDefenseMgr.SetFastest(c_lpChar, (int)lua_tonumber(L, 2));

		return 0;

	}

	ALUA(ship_update_info)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("Wrong argument.");
			return 0;
		}

		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		rkShipDefenseMgr.UpdateInfo(c_lpChar);

		return 0;
	}

	ALUA(ship_is_test)
	{
		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
			return 0;

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::Instance();
		return rkShipDefenseMgr.GetTest(c_lpChar);
	}

	int ship_defense_mgr_can_join(lua_State* L)
	{
		const LPCHARACTER c_lpChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (c_lpChar == nullptr)
		{
			lua_pushboolean(L, false);
			return 1;
		}

		CShipDefenseManager& rkShipDefenseMgr = CShipDefenseManager::instance();
		lua_pushboolean(L, rkShipDefenseMgr.CanJoin(c_lpChar));
		return 1;
	}

	void RegisterShipDefenseManagerFunctionTable()
	{
		luaL_reg functions[] =
		{
			{ "create", ship_defense_mgr_create },
			{ "start", ship_defense_mgr_start },
			{ "start_test", ship_defense_mgr_start_test },
			{ "set_test", ship_defense_mgr_set_test },
			{ "join", ship_defense_mgr_join },
			{ "leave", ship_defense_mgr_leave },
			{ "land", ship_defense_mgr_land },
			{ "is_created", ship_defense_mgr_is_created },
			{ "is_running", ship_defense_mgr_is_running },
			{ "need_party", ship_defense_mgr_need_party },
			{ "need_ticket", ship_defense_mgr_need_ticket },
			{ "spawn_wood_repair", ship_defense_mgr_spawn_wood_repair },
			{ "require_cooldown", ship_defense_mgr_require_cooldown },
			{ "set_alliance_hp_pct", ship_defense_mgr_set_alliance_hp_pct },
			{ "can_join", ship_defense_mgr_can_join},
			{ "increase_completed", ship_increase_completed },
			{ "is_test", ship_is_test },
			{ "set_fastest", ship_set_fastest },
			{ "update_info", ship_update_info },
			{ nullptr, nullptr }
		};

		CQuestManager::instance().AddLuaFunctionTable("ship_defense_mgr", functions);
	}
}
#endif
