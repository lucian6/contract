#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "log.h"

#include "char.h"
#include "desc.h"
#include "item.h"

static char	__escape_hint[1024];

LogManager::LogManager() : m_bIsConnect(false)
{
}

LogManager::~LogManager()
{
}

bool LogManager::Connect(const char * host, const int port, const char * user, const char * pwd, const char * db)
{
	if (m_sql.Setup(host, user, pwd, db, g_stLocale.c_str(), false, port))
		m_bIsConnect = true;

	return m_bIsConnect;
}

void LogManager::Query(const char * c_pszFormat, ...)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);

	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

	if (test_server)
		sys_log(0, "LOG: %s", szQuery);

	m_sql.AsyncQuery(szQuery);
}

bool LogManager::IsConnected()
{
	return m_bIsConnect;
}

size_t LogManager::EscapeString(char* dst, size_t dstSize, const char *src, size_t srcSize)
{
	return m_sql.EscapeString(dst, dstSize, src, srcSize);
}

void LogManager::ItemLog(DWORD dwPID, DWORD x, DWORD y, DWORD dwItemID, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, DWORD dwVnum, DWORD dwChannel)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHint, strlen(c_pszHint));

	Query("INSERT INTO item%s (player_id, time, x, y, channel, ip, item_id, item_vnum, what, hint) VALUES(%u, NOW(), %u, %u, %u, '%s', %u, %u, '%s', '%s')", get_table_postfix(), dwPID, x, y, dwChannel, c_pszIP, dwItemID, dwVnum, c_pszText, c_pszHint);
}

void LogManager::ItemLog(LPCHARACTER ch, LPITEM item, const char * c_pszText, const char * c_pszHint)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	if (NULL == ch || NULL == item)
	{
		sys_err("character or item nil (ch %p item %p text %s)", get_pointer(ch), get_pointer(item), c_pszText);
		return;
	}

	ItemLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), item->GetID(),
			NULL == c_pszText ? "" : c_pszText,
		   	NULL == c_pszHint ? "" : c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "",
		   	item->GetOriginalVnum(), g_bChannel);
}

void LogManager::ItemLog(LPCHARACTER ch, int itemID, int itemVnum, const char * c_pszText, const char * c_pszHint)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	ItemLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), itemID, c_pszText, c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "", itemVnum, g_bChannel);
}

#if defined(__MINI_GAME_OKEY__)
void LogManager::OkeyEventLog(int dwPID, const char* c_pszText, int points)
{
	Query("INSERT INTO okey_event%s (pid, name, points) VALUES(%d, '%s', %d)", get_table_postfix(), dwPID, c_pszText, points);
}
#endif

void LogManager::LoginLog(bool isLogin, DWORD dwAccountID, DWORD dwPID, BYTE bLevel, BYTE bJob, DWORD dwPlayTime)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	Query("INSERT INTO loginlog%s (type, time, channel, account_id, pid, level, job, playtime) VALUES (%s, NOW(), %d, %u, %u, %d, %d, %u)",
			get_table_postfix(), isLogin ? "'LOGIN'" : "'LOGOUT'", g_bChannel, dwAccountID, dwPID, bLevel, bJob, dwPlayTime);
}

void LogManager::MoneyLog(BYTE type, DWORD vnum, int gold)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MAX);
	if (type == MONEY_LOG_RESERVED || type >= MONEY_LOG_TYPE_MAX_NUM)
	{
		sys_err("TYPE ERROR: type %d vnum %u gold %d", type, vnum, gold);
		return;
	}

	Query("INSERT INTO money_log%s VALUES (NOW(), %d, %d, %d)", get_table_postfix(), type, vnum, gold);
}

void LogManager::HackLog(const char * c_pszHackName, const char * c_pszLogin, const char * c_pszName, const char * c_pszIP)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MID);
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHackName, strlen(c_pszHackName));

	Query("INSERT INTO hack_log (time, login, name, ip, server, why) VALUES(NOW(), '%s', '%s', '%s', '%s', '%s')", c_pszLogin, c_pszName, c_pszIP, g_stHostname.c_str(), __escape_hint);
}

void LogManager::HackLog(const char * c_pszHackName, LPCHARACTER ch)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MID);
	if (ch->GetDesc())
	{
		HackLog(c_pszHackName,
				ch->GetDesc()->GetAccountTable().login,
				ch->GetName(),
				ch->GetDesc()->GetHostName());
	}
}

void LogManager::SpeedHackLog(DWORD pid, DWORD map_index, DWORD x, DWORD y, int hack_count)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MID);
	Query("INSERT INTO speed_hack%s (player_id, time, map_index, x, y, channel, hack_count) "
			"VALUES(%u, NOW(), %u, %u, %u, %u, %d)",
			get_table_postfix(), pid, map_index, x, y, g_bChannel, hack_count);
}

void LogManager::ChangeNameLog(DWORD pid, const char *old_name, const char *new_name, const char *ip)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	Query("INSERT INTO change_name%s (pid, old_name, new_name, time, ip) "
			"VALUES(%u, '%s', '%s', NOW(), '%s') ",
			get_table_postfix(), pid, old_name, new_name, ip);
}

void LogManager::GMCommandLog(DWORD dwPID, const char* szName, const char* szIP, BYTE byChannel, const char* szCommand)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), szCommand, strlen(szCommand));

	Query("INSERT INTO command_log%s (userid, server, ip, port, username, command, date ) "
			"VALUES(%u, 999, '%s', %u, '%s', '%s', NOW()) ",
			get_table_postfix(), dwPID, szIP, byChannel, szName, __escape_hint);
}

void LogManager::LevelLog(LPCHARACTER pChar, unsigned int level, unsigned int playhour)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	DWORD aid = 0;

	if (NULL != pChar->GetDesc())
	{
		aid = pChar->GetDesc()->GetAccountTable().id;
	}

	Query("REPLACE INTO levellog%s (name, level, time, account_id, pid, playtime) VALUES('%s', %u, NOW(), %u, %u, %d)",
			get_table_postfix(), pChar->GetName(), level, aid, pChar->GetPlayerID(), playhour);
}

void LogManager::BootLog(const char * c_pszHostName, BYTE bChannel)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	Query("INSERT INTO bootlog (time, hostname, channel) VALUES(NOW(), '%s', %d)",
			c_pszHostName, bChannel);
}

void LogManager::DetailLoginLog(bool isLogin, LPCHARACTER ch)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MID);
	if (NULL == ch->GetDesc())
		return;

	if (true == isLogin)
	{
		Query("INSERT INTO loginlog2(type, is_gm, login_time, channel, account_id, pid, ip, client_version) "
				"VALUES('INVALID', %s, NOW(), %d, %u, %u, inet_aton('%s'), '%s')",
				ch->IsGM() == true ? "'Y'" : "'N'",
				g_bChannel,
				ch->GetDesc()->GetAccountTable().id,
				ch->GetPlayerID(),
				ch->GetDesc()->GetHostName(),
				ch->GetDesc()->GetClientVersion());
	}
	else
	{
		Query("SET @i = (SELECT MAX(id) FROM loginlog2 WHERE account_id=%u AND pid=%u)",
				ch->GetDesc()->GetAccountTable().id,
				ch->GetPlayerID());

		Query("UPDATE loginlog2 SET type='VALID', logout_time=NOW(), playtime=TIMEDIFF(logout_time,login_time) WHERE id=@i");
	}
}

void LogManager::ChatLog(DWORD where, DWORD who_id, const char* who_name, DWORD whom_id, const char* whom_name, const char* type, const char* msg, const char* ip)
{
	Query("INSERT INTO `chat_log%s` (`where`, `who_id`, `who_name`, `whom_id`, `whom_name`, `type`, `msg`, `when`, `ip`) "
		"VALUES (%u, %u, '%s', %u, '%s', '%s', '%s', NOW(), '%s');",
		get_table_postfix(),
		where, who_id, who_name, whom_id, whom_name, type, msg, ip);
}
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
