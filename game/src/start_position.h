#ifndef __START_POSITION_H
#define __START_POSITION_H

#include "locale_service.h"
#include "maintenance.h"

extern char g_nation_name[4][32];
extern DWORD g_start_position[4][2];
extern long g_start_map[4];
extern DWORD g_create_position[4][2];
extern DWORD g_create_position_canada[4][2];
extern DWORD g_create_position_a[4][2];
extern DWORD g_create_position_b[4][2];
extern DWORD g_create_position_c[4][2];
extern DWORD g_create_position_lobby[11][2];
extern DWORD arena_return_position[4][2];

inline const char* EMPIRE_NAME( BYTE e)
{
	return LC_TEXT(g_nation_name[e]);
}

inline DWORD EMPIRE_START_MAP(BYTE e)
{
	return g_start_map[e];
}

inline DWORD EMPIRE_START_X(BYTE e)
{
	if (1 <= e && e <= 3)
		return g_start_position[e][0];

	return 0;
}

inline DWORD EMPIRE_START_Y(BYTE e)
{
	if (1 <= e && e <= 3)
		return g_start_position[e][1];

	return 0;
}

inline DWORD ARENA_RETURN_POINT_X(BYTE e)
{
	if (1 <= e && e <= 3)
		return arena_return_position[e][0];

	return 0;
}

inline DWORD ARENA_RETURN_POINT_Y(BYTE e)
{
	if (1 <= e && e <= 3)
		return arena_return_position[e][1];

	return 0;
}

inline DWORD CREATE_START_X(BYTE e, int random)
{
	if (CMaintenanceManager::Instance().GetGameMode() == GAME_MODE_LOBBY)
	{
		return g_create_position_lobby[random-1][0];
	}
	else
	{
		if (e == 1)
			return g_create_position_a[random-1][0];
		else if (e == 2)
			return g_create_position_b[random-1][0];
		else if (e == 3)
			return g_create_position_c[random-1][0];
	}

	return 0;
}

inline DWORD CREATE_START_Y(BYTE e, int random)
{
	if (CMaintenanceManager::Instance().GetGameMode() == GAME_MODE_LOBBY)
	{
		return g_create_position_lobby[random-1][1];
	}
	else
	{
		if (e == 1)
			return g_create_position_a[random-1][1];
		else if (e == 2)
			return g_create_position_b[random-1][1];
		else if (e == 3)
			return g_create_position_c[random-1][1];
	}

	return 0;
}

#endif
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
