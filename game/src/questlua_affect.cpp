#include "stdafx.h"
#include "config.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "char.h"
#include "affect.h"
#include "db.h"

namespace quest
{
	ALUA(affect_add)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		BYTE applyOn = (BYTE)lua_tonumber(L, 1);

		if (applyOn >= MAX_APPLY_NUM || applyOn < 1)
		{
			sys_err("apply is out of range : %d", applyOn);
			return 0;
		}

		if (ch->FindAffect(AFFECT_QUEST_START_IDX, applyOn))
			return 0;

		long value = (long)lua_tonumber(L, 2);
		long duration = (long)lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_QUEST_START_IDX, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	ALUA(affect_add_real)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) )
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		int affectOn = (int)lua_tonumber(L, 1);

		if (affectOn >= 999 || affectOn < 1)
		{
			sys_err("affectOn is out of range : %d", affectOn);
			return 0;
		}

		BYTE applyOn = (BYTE)lua_tonumber(L, 2);

		if (applyOn >= MAX_APPLY_NUM || applyOn < 0)
		{
			sys_err("apply is out of range : %d", applyOn);
			return 0;
		}

		if (ch->FindAffect(affectOn, applyOn))
			ch->RemoveAffect(affectOn);

		long value = (long)lua_tonumber(L, 3);
		long duration = (long)lua_tonumber(L, 4);

		ch->AddAffect(affectOn, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	ALUA(affect_remove)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		quest::PC* pcPtr = q.GetCurrentPC();
		if (!pcPtr)
			return 0;

		int iType;

		if (lua_isnumber(L, 1))
		{
			iType = (int)lua_tonumber(L, 1);

			if (iType == 0)
				iType = pcPtr->GetCurrentQuestIndex() + AFFECT_QUEST_START_IDX;
		}
		else
			iType = pcPtr->GetCurrentQuestIndex() + AFFECT_QUEST_START_IDX;

		ch->RemoveAffect(iType);
		return 0;
	}

	ALUA(affect_remove_bad)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;
		ch->RemoveBadAffect();
		return 0;
	}

	ALUA(affect_remove_good)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;
		ch->RemoveGoodAffect();
		return 0;
	}

	ALUA(affect_add_hair)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		BYTE applyOn = (BYTE)lua_tonumber(L, 1);

		if (applyOn >= MAX_APPLY_NUM || applyOn < 1)
		{
			sys_err("apply is out of range : %d", applyOn);
			return 0;
		}

		long value = (long)lua_tonumber(L, 2);
		long duration = (long)lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_HAIR, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	ALUA(affect_remove_hair)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		CAffect* pkAff = ch->FindAffect(AFFECT_HAIR);

		if (pkAff != NULL)
		{
			lua_pushnumber(L, pkAff->lDuration);
			ch->RemoveAffect(pkAff);
		}
		else
			lua_pushnumber(L, 0);
		return 1;
	}

	ALUA(affect_get_apply_on)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		DWORD affectType = lua_tonumber(L, 1);

		CAffect* pkAff = ch->FindAffect(affectType);

		if (pkAff != NULL)
			lua_pushnumber(L, pkAff->bApplyOn);
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	ALUA(affect_add_collect)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		BYTE applyOn = (BYTE)lua_tonumber(L, 1);

		if (applyOn >= MAX_APPLY_NUM || applyOn < 1)
		{
			sys_err("apply is out of range : %d", applyOn);
			return 0;
		}

		long value = (long)lua_tonumber(L, 2);
		long duration = (long)lua_tonumber(L, 3);

		CAffect* affect = ch->FindAffect(AFFECT_COLLECT, aApplyInfo[applyOn].bPointType);
		if (affect) {
			value += affect->lApplyValue;
			ch->RemoveAffect(affect);
		}

		ch->AddAffect(AFFECT_COLLECT, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	ALUA(affect_add_collect_point)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		BYTE point_type = (BYTE)lua_tonumber(L, 1);
		if (point_type >= POINT_MAX_NUM || point_type < 1)
		{
			sys_err("point is out of range : %d", point_type);
			return 0;
		}
		long value = (long)lua_tonumber(L, 2);
		long duration = (long)lua_tonumber(L, 3);
		CAffect* affect = ch->FindAffect(AFFECT_COLLECT, point_type);
		if (affect) {
			value += affect->lApplyValue;
			ch->RemoveAffect(affect);
		}
		ch->AddAffect(AFFECT_COLLECT, point_type, value, 0, duration, 0, false);
		return 0;
	}

	ALUA(affect_remove_collect)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch != NULL)
		{
			BYTE bApply = (BYTE)lua_tonumber(L, 1);

			if (bApply >= MAX_APPLY_NUM) return 0;

			bApply = aApplyInfo[bApply].bPointType;
			long value = (long)lua_tonumber(L, 2);

			const std::list<CAffect*>& rList = ch->GetAffectContainer();
			const CAffect* pAffect = NULL;

			for (auto iter = rList.begin(); iter != rList.end(); ++iter)
			{
				pAffect = *iter;

				if (pAffect->dwType == AFFECT_COLLECT)
				{
					if (pAffect->bApplyOn == bApply && pAffect->lApplyValue == value)
					{
						break;
					}
				}

				pAffect = NULL;
			}

			if (pAffect != NULL)
			{
				ch->RemoveAffect(const_cast<CAffect*>(pAffect));
			}
		}

		return 0;
	}

	ALUA(affect_remove_all_collect)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch != NULL)
			ch->RemoveAffect(AFFECT_COLLECT);
		return 0;
	}

	ALUA(affect_str_to_apply)
	{
		if (!lua_isstring(L, 1))
			return 0;
		lua_pushnumber(L, FN_get_apply_type(lua_tostring(L,1)));
		return 1;
	}

	ALUA(affect_remove_aff_value)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch != NULL)
		{
			BYTE bApply = (BYTE)lua_tonumber(L, 1);
			if (bApply >= MAX_APPLY_NUM)
				return 0;

			long value = 0;

			CAffect* affect = ch->FindAffect(AFFECT_COLLECT, aApplyInfo[bApply].bPointType);
			if(affect != NULL)
			{
				value = affect->lApplyValue-lua_tonumber(L, 2);
				ch->RemoveAffect(affect);
				ch->AddAffect(AFFECT_COLLECT, aApplyInfo[bApply].bPointType , value, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);
			}
		}
		return 0;
	}

	void RegisterAffectFunctionTable()
	{
		luaL_reg affect_functions[] =
		{
			{ "add",		affect_add		},
			{ "AddAffect",	affect_add_real },
			{ "remove",		affect_remove		},
			{ "remove_bad",	affect_remove_bad	},
			{ "remove_good",	affect_remove_good	},
			{ "add_hair",		affect_add_hair		},
			{ "remove_hair",	affect_remove_hair		},
			{ "add_collect",		affect_add_collect		},
			{ "add_collect_point",		affect_add_collect_point		},
			{ "remove_collect",		affect_remove_collect	},
			{ "remove_all_collect",	affect_remove_all_collect	},
			{ "get_apply_on",	affect_get_apply_on },

			{ "str_to_apply",	affect_str_to_apply },
			{ "remove_aff_value",	affect_remove_aff_value },

			{ NULL,		NULL			}
		};

		CQuestManager::instance().AddLuaFunctionTable("affect", affect_functions);
	}
};
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
