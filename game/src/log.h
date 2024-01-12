#ifndef __INC_LOG_MANAGER_H__
#define __INC_LOG_MANAGER_H__

#include "../../libsql/AsyncSQL.h"
#include "any_function.h"
#include "locale_service.h"

// #ifdef ENABLE_NEWSTUFF
enum log_level {LOG_LEVEL_NONE=0, LOG_LEVEL_MIN=1, LOG_LEVEL_MID=2, LOG_LEVEL_MAX=3};
// #endif

#ifdef ENABLE_NEWSTUFF
#define LOG_LEVEL_CHECK_N_RET(x) { if (g_iDbLogLevel < x) return; }
#else
#define LOG_LEVEL_CHECK_N_RET(x) { }
#endif

#ifdef ENABLE_NEWSTUFF
#define LOG_LEVEL_CHECK(x, fnc)	\
	{\
		if (g_iDbLogLevel >= (x))\
			fnc;\
	}
#else
#define LOG_LEVEL_CHECK(x, fnc)	{ fnc; }
#endif

enum GOLDBAR_HOW
{
	PERSONAL_SHOP_BUY	= 1 ,
	PERSONAL_SHOP_SELL	= 2 ,
	SHOP_BUY			= 3 ,
	SHOP_SELL			= 4 ,
	EXCHANGE_TAKE		= 5 ,
	EXCHANGE_GIVE		= 6 ,
	QUEST				= 7 ,
};

class LogManager : public singleton<LogManager>
{
	public:
		LogManager();
		virtual ~LogManager();

		bool		IsConnected();

		bool		Connect(const char * host, const int port, const char * user, const char * pwd, const char * db);

		void		ItemLog(DWORD dwPID, DWORD x, DWORD y, DWORD dwItemID, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, DWORD dwVnum, DWORD dwChannel);
		void		ItemLog(LPCHARACTER ch, LPITEM item, const char * c_pszText, const char * c_pszHint);
		void		ItemLog(LPCHARACTER ch, int itemID, int itemVnum, const char * c_pszText, const char * c_pszHint);

#if defined(__MINI_GAME_OKEY__)
		void		OkeyEventLog(int dwPID, const char* c_pszText, int points);
#endif

		void		LoginLog(bool isLogin, DWORD dwAccountID, DWORD dwPID, BYTE bLevel, BYTE bJob, DWORD dwPlayTime);
		void		MoneyLog(BYTE type, DWORD vnum, int gold);
		void		HackLog(const char * c_pszHackName, const char * c_pszLogin, const char * c_pszName, const char * c_pszIP);
		void		HackLog(const char * c_pszHackName, LPCHARACTER ch);
		void		GMCommandLog(DWORD dwPID, const char * szName, const char * szIP, BYTE byChannel, const char * szCommand);
		void		SpeedHackLog(DWORD pid, DWORD map_index, DWORD x, DWORD y, int hack_count);
		void		ChangeNameLog(DWORD pid, const char * old_name, const char * new_name, const char * ip);
		void		LevelLog(LPCHARACTER pChar, unsigned int level, unsigned int playhour);
		void		BootLog(const char * c_pszHostName, BYTE bChannel);
		void		DetailLoginLog(bool isLogin, LPCHARACTER ch);
		void		ChatLog(DWORD where, DWORD who_id, const char* who_name, DWORD whom_id, const char* whom_name, const char* type, const char* msg, const char* ip);

		size_t EscapeString(char* dst, size_t dstSize, const char *src, size_t srcSize);
	private:
		void		Query(const char * c_pszFormat, ...);

		CAsyncSQL	m_sql;
		bool		m_bIsConnect;
};

#endif
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
