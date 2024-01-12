#include "stdafx.h"
#include "maintenance.h"
#include "cmd.h"
#include "char.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"


const std::vector<std::pair<long, long>> lobbyPositions = {
	{368200, 1457400},
	{373600, 1460600},
	{376600, 1451300},
	{391200, 1452500},
	{395400, 1464300},
	{402100, 1461900},
	{402000, 1446800},
	{384100, 1475300},
	{369500, 1470500},
	{374400, 1479700},
	{404500, 1476300},
};

static const std::map<BYTE, std::vector<std::pair<long, long>>> empirePositions = {
	{
		1,//red-empire
		{
			{459500, 953700},
			{484100, 929000},
			{488135, 955100},
			{483300, 968600},
		}
	},
	{
		2,//yellow-empire
		{
			{51400, 166600},
			{64000, 152000},
			{64200, 184500},
			{77600, 166200},
		}
	},
	{
		3,//blue-empire
		{
			{957300, 255300},
			{975200, 268800},
			{963800, 285400},
			{943400, 275100},
		}
	},
};

CMaintenanceManager::CMaintenanceManager(){}
CMaintenanceManager::~CMaintenanceManager(){}

void CMaintenanceManager::ReloadData()
{
	const BYTE subHeader = MAINTENANCE_GD_RELOAD;
	db_clientdesc->DBPacket(HEADER_GD_MAINTENANCE, 0, &subHeader, sizeof(BYTE));
}

void ComparePacket(TPacketGCMaintenance& packet, const TMainteance* eventPtr)
{
	packet.header = HEADER_GC_MAINTENANCE;
	packet.maintenance.iStartTime = eventPtr->iStartTime - time(0);
	packet.maintenance.iEndTime = eventPtr->iEndTime - time(0);
	packet.maintenance.mode = eventPtr->mode;
	packet.maintenance.status = eventPtr->status;
	strlcpy(packet.maintenance.reason, eventPtr->reason, sizeof(packet.maintenance.reason));
}

void CMaintenanceManager::SetMainteanceData(const char* szPacketData)
{
	m_data.clear();
	const BYTE dataCount = *(BYTE*)szPacketData;
	szPacketData += sizeof(BYTE);
	const bool isFromGameMaster = *(bool*)szPacketData;
	szPacketData += sizeof(bool);
	for (BYTE j = 0; j < dataCount;++j)
	{
		const TMainteance& data = *(TMainteance*)szPacketData;
		szPacketData += sizeof(TMainteance);
		m_data.emplace_back(data);
	}
	if (isFromGameMaster)
	{
		const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();

		//CLEAR
		TPacketGCMaintenance packet;
		memset(&packet, 0, sizeof(packet));
		packet.header = HEADER_GC_MAINTENANCE;
		packet.maintenance.mode = GAME_MODE_NORMAL;
		for (const auto& desc : c_ref_set)
		{
			if(desc->GetPhase() == PHASE_GAME)
				desc->Packet(&packet, sizeof(packet));
		}

		const TMainteance* currentModePtr = GetCurrentMode();
		if (currentModePtr)
		{
			ComparePacket(packet, currentModePtr);

			if (currentModePtr->mode == GAME_MODE_LOBBY)
			{
				for (const auto& desc : c_ref_set)
				{
					LPCHARACTER ch = desc->GetCharacter();
					if (!ch)
						continue;
					if (ch->GetMapIndex() != MAINTENANCE_LOBBY_MAP_IDX && !ch->IsGM())
					{
						const BYTE randomPos = lobbyPositions.size() <= 1 ? 0 : number(0, lobbyPositions.size() - 1);
						ch->WarpSet(lobbyPositions[randomPos].first, lobbyPositions[randomPos].second);
					}
					else
						desc->Packet(&packet, sizeof(packet));
				}
			}
			else if (currentModePtr->mode == GAME_MODE_MAINTENANCE)
			{
				for (const auto& desc : c_ref_set)
				{
					if (desc->GetPhase() == PHASE_GAME)
					{
						desc->Packet(&packet, sizeof(packet));
						LPCHARACTER ch = desc->GetCharacter();
						if (ch && !ch->IsGM() && !desc->IsTester())
							desc->DelayedDisconnect(5);
					}
				}
			}
		}
		else
		{
			const TMainteance* eventPtr = GetCurrentMode(GAME_MODE_MAINTENANCE);
			if (eventPtr)
			{
				ComparePacket(packet, eventPtr);
				for (const auto& desc : c_ref_set)
				{
					if (desc->GetPhase() == PHASE_GAME)
						desc->Packet(&packet, sizeof(packet));
				}
			}

			const TMainteance* eventPtrEx = GetCurrentMode(GAME_MODE_LOBBY);
			if (eventPtrEx)
			{
				ComparePacket(packet, eventPtrEx);
				for (const auto& desc : c_ref_set)
				{
					if (desc->GetPhase() == PHASE_GAME)
						desc->Packet(&packet, sizeof(packet));
				}
			}
		}
	}
}

void CMaintenanceManager::CheckMaintenance(LPDESC desc)
{
	LPCHARACTER ch = desc->GetCharacter();
	const TMainteance* currentModePtr = GetCurrentMode();
	if (currentModePtr)
	{
		TPacketGCMaintenance packet;
		ComparePacket(packet, currentModePtr);
		if (currentModePtr->mode == GAME_MODE_LOBBY)
		{
			if (desc->GetPhase() == PHASE_GAME)
			{
				if (ch && ch->GetMapIndex() != MAINTENANCE_LOBBY_MAP_IDX && !ch->IsGM())
				{
					const BYTE randomPos = lobbyPositions.size() <= 1 ? 0 : number(0, lobbyPositions.size() - 1);
					ch->WarpSet(lobbyPositions[randomPos].first, lobbyPositions[randomPos].second);
					return;
				}
				desc->Packet(&packet, sizeof(packet));
			}
			return;
		}
		else if (currentModePtr->mode == GAME_MODE_MAINTENANCE)
		{
			if (desc->GetPhase() == PHASE_GAME)
			{
				desc->Packet(&packet, sizeof(packet));
				if(ch && !ch->IsGM() && !desc->IsTester())
					desc->DelayedDisconnect(5);
			}
			return;
		}
	}
	else
	{
		const TMainteance* eventPtrMaintenance = GetCurrentMode(GAME_MODE_MAINTENANCE);
		if (eventPtrMaintenance)
		{
			TPacketGCMaintenance packet;
			ComparePacket(packet, eventPtrMaintenance);
			desc->Packet(&packet, sizeof(packet));
		}

		const TMainteance* eventPtrLobby = GetCurrentMode(GAME_MODE_LOBBY);
		if (eventPtrLobby)
		{
			TPacketGCMaintenance packet;
			ComparePacket(packet, eventPtrLobby);
			desc->Packet(&packet, sizeof(packet));
		}
	}
}

void CMaintenanceManager::SetStatus(const WORD id, const bool status)
{
	TMainteance* mainEvent = NULL;
	
	for (WORD j = 0; j < m_data.size(); ++j)
	{
		TMainteance& data = m_data[j];
		if (data.id == id)
		{
			mainEvent = &m_data[j];
			data.status = status;
			break;
		}
	}

	if (mainEvent)
	{
		if (mainEvent->mode == GAME_MODE_MAINTENANCE)
		{
			TPacketGCMaintenance packet;
			ComparePacket(packet, mainEvent);
			if (status)
			{
				const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
				for (const auto& desc : c_ref_set)
				{
					if (desc->GetPhase() == PHASE_GAME)
					{
						LPCHARACTER ch = desc->GetCharacter();
						if (ch && !ch->IsGM() && !desc->IsTester())
							desc->DelayedDisconnect(number(1, 5));
						else
							desc->Packet(&packet, sizeof(packet));
					}
				}
			}
			else
			{
				packet.maintenance.mode = 0;
				const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
				for (const auto& desc : c_ref_set)
				{
					if (desc->GetPhase() == PHASE_GAME)
						desc->Packet(&packet, sizeof(packet));
				}
			}
		}
		else if (mainEvent->mode == GAME_MODE_LOBBY)
		{
			const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
			TPacketGCMaintenance packet;
			ComparePacket(packet, mainEvent);
			if (status)
			{
				for (const auto& desc : c_ref_set)
				{
					LPCHARACTER ch = desc->GetCharacter();
					if (!ch)
						continue;
					if (ch->GetMapIndex() != MAINTENANCE_LOBBY_MAP_IDX && !ch->IsGM())
					{
						const BYTE randomPos = lobbyPositions.size() <= 1 ? 0 : number(0, lobbyPositions.size() - 1);
						ch->WarpSet(lobbyPositions[randomPos].first, lobbyPositions[randomPos].second);
					}
					else
						desc->Packet(&packet, sizeof(packet));
				}
			}
			else
			{
				for (const auto& desc : c_ref_set)
				{
					LPCHARACTER ch = desc->GetCharacter();
					if (ch && ch->GetMapIndex() == MAINTENANCE_LOBBY_MAP_IDX && !ch->IsGM())
					{
						const itertype(empirePositions) it = empirePositions.find(ch->GetEmpire());
						if (it != empirePositions.end())
						{
							const BYTE randomPos = it->second.size() <= 1 ? 0 : number(0, it->second.size() - 1);
							ch->WarpSet(it->second[randomPos].first, it->second[randomPos].second);
						}
					}
				}
			}
		}
	}
}
BYTE CMaintenanceManager::GetGameMode()
{
	const TMainteance* eventPtr = GetCurrentMode();
	return eventPtr ? eventPtr->mode : 0;
}
const TMainteance* CMaintenanceManager::GetCurrentMode(BYTE modeIdx)
{
	for (WORD j = 0; j < m_data.size(); ++j)
	{
		if ((modeIdx != GAME_MODE_NORMAL && m_data[j].mode == modeIdx) || m_data[j].status)
			return &m_data[j];
	}
	return NULL;
}
