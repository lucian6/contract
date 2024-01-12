#pragma once
#include "../../common/tables.h"

extern const std::vector<std::pair<long, long>> lobbyPositions;


class CMaintenanceManager : public singleton<CMaintenanceManager>
{
public:
	CMaintenanceManager();
	~CMaintenanceManager();
	void ReloadData();
	void SetMainteanceData(const char* szPacketData);
	void SetStatus(const WORD id, const bool status);
	void CheckMaintenance(LPDESC desc);
	BYTE GetGameMode();
	const TMainteance* GetCurrentMode(BYTE modeIdx = GAME_MODE_NORMAL);

protected:
	std::vector<TMainteance>	m_data;
};
