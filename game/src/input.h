#ifndef __INC_METIN_II_GAME_INPUT_PROCESSOR__
#define __INC_METIN_II_GAME_INPUT_PROCESSOR__

#include "packet_info.h"

enum
{
	INPROC_CLOSE,
	INPROC_HANDSHAKE,
	INPROC_LOGIN,
	INPROC_MAIN,
	INPROC_DEAD,
	INPROC_DB,
	INPROC_UDP,
	INPROC_P2P,
	INPROC_AUTH,
};

void LoginFailure(LPDESC d, const char * c_pszStatus);

enum EHWIDStatus
{
	HWID_STATUS_UNTOUCHED,
	HWID_STATUS_CHANGED,
	HWID_STATUS_BLOCKED,
};

std::tuple<BYTE /* Status */, BYTE, /* PID1 */ BYTE, /* PID2 */ BYTE, /* PID3 */ BYTE, /* PID4 */ BYTE /* PID5 */> GetHWIDStatus(DWORD dwAID, const char* c_szHWID, bool bCheckBlock = false);
BYTE IsHWIDBlocked(const char* c_szHWID);

extern void SendShout(const char * szText, BYTE bEmpire);
#include <vector>
extern std::vector<char> cachedChannelStatus;

class CInputProcessor
{
	public:
		CInputProcessor();
		virtual ~CInputProcessor() {};

		virtual bool Process(LPDESC d, const void * c_pvOrig, int iBytes, int & r_iBytesProceed);
		virtual BYTE GetType() = 0;

		void BindPacketInfo(CPacketInfo * pPacketInfo);
		void Pong(LPDESC d);
		void Handshake(LPDESC d, const char * c_pData);
		void Version(LPCHARACTER ch, const char* c_pData);

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData) = 0;

		CPacketInfo * m_pPacketInfo;
		int	m_iBufferLeft;

		CPacketInfoCG 	m_packetInfoCG;
};

class CInputClose : public CInputProcessor
{
	public:
		virtual BYTE	GetType() { return INPROC_CLOSE; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData) { return m_iBufferLeft; }
};

class CInputHandshake : public CInputProcessor
{
	public:
		CInputHandshake();
		virtual ~CInputHandshake();

		virtual BYTE	GetType() { return INPROC_HANDSHAKE; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

	protected:
		void		GuildMarkLogin(LPDESC d, const char* c_pData);

		CPacketInfo *	m_pMainPacketInfo;
};

class CInputLogin : public CInputProcessor
{
	public:
		virtual BYTE	GetType() { return INPROC_LOGIN; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

	protected:
		void		Login(LPDESC d, const char * data);
		void		LoginByKey(LPDESC d, const char * data);

		void		CharacterSelect(LPDESC d, const char * data);
		void		CharacterCreate(LPDESC d, const char * data);
		void		CharacterDelete(LPDESC d, const char * data);
		void		CharacterPinCode(LPDESC d, const char* c_pData);

		void		Entergame(LPDESC d, const char * data);
		void		Empire(LPDESC d, const char * c_pData);
		void		GuildMarkCRCList(LPDESC d, const char* c_pData);
		// MARK_BUG_FIX
		void		GuildMarkIDXList(LPDESC d, const char* c_pData);
		// END_OF_MARK_BUG_FIX
		void		GuildMarkUpload(LPDESC d, const char* c_pData);
		int			GuildSymbolUpload(LPDESC d, const char* c_pData, size_t uiBytes);
		void		GuildSymbolCRC(LPDESC d, const char* c_pData);
		void		ChangeName(LPDESC d, const char * data);
};

class CInputMain : public CInputProcessor
{
	public:
		virtual BYTE	GetType() { return INPROC_MAIN; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

	protected:
		void		Attack(LPCHARACTER ch, const BYTE header, const char* data);

		int			Whisper(LPCHARACTER ch, const char * data, size_t uiBytes);
		int			Chat(LPCHARACTER ch, const char * data, size_t uiBytes);
		void		ItemUse(LPCHARACTER ch, const char * data);
		void		ItemDrop(LPCHARACTER ch, const char * data);
		void		ItemDrop2(LPCHARACTER ch, const char * data);
#ifdef ENABLE_CHEST_OPEN_RENEWAL
		void		ChestOpen(LPCHARACTER ch, const char * data);
#endif
		void		ItemDestroy(LPCHARACTER ch, const char * data);
		void		ItemMove(LPCHARACTER ch, const char * data);
		void		ItemPickup(LPCHARACTER ch, const char * data);
		void		ItemToItem(LPCHARACTER ch, const char * pcData);
		void		QuickslotAdd(LPCHARACTER ch, const char * data);
		void		QuickslotDelete(LPCHARACTER ch, const char * data);
		void		QuickslotSwap(LPCHARACTER ch, const char * data);
		int			Shop(LPCHARACTER ch, const char * data, size_t uiBytes);

		void		OnClick(LPCHARACTER ch, const char * data);
		void		Exchange(LPCHARACTER ch, const char * data);
		void		Position(LPCHARACTER ch, const char * data);
		void		Move(LPCHARACTER ch, const char * data);
		int			SyncPosition(LPCHARACTER ch, const char * data, size_t uiBytes);
		void		FlyTarget(LPCHARACTER ch, const char * pcData, BYTE bHeader);
		void		UseSkill(LPCHARACTER ch, const char * pcData);

#if defined(__SKILL_COLOR_SYSTEM__)
		void		SetSkillColor(LPCHARACTER ch, const char* pcData);
#endif
		void		ScriptAnswer(LPCHARACTER ch, const void * pvData);
		void		ScriptButton(LPCHARACTER ch, const void * pvData);
		void		ScriptSelectItem(LPCHARACTER ch, const void * pvData);

		void		QuestInputString(LPCHARACTER ch, const void * pvData);
		void		QuestConfirm(LPCHARACTER ch, const void* pvData);
		void		Target(LPCHARACTER ch, const char * pcData);
		void		Warp(LPCHARACTER ch, const char * pcData);
		void		SafeboxCheckin(LPCHARACTER ch, const char * c_pData);
		void		SafeboxCheckout(LPCHARACTER ch, const char * c_pData, bool bMall);
		void		SafeboxItemMove(LPCHARACTER ch, const char * data);
		int			Messenger(LPCHARACTER ch, const char* c_pData, size_t uiBytes);

#if defined(__BL_67_ATTR__)
		void		Attr67(LPCHARACTER ch, const char* data);
		void		Attr67Close(LPCHARACTER ch, const char* data);
#endif
#if defined(__BL_MOVE_COSTUME_ATTR__)
		void		ItemCombination(LPCHARACTER ch, const char* c_pData);
		void		ItemCombinationCancel(LPCHARACTER ch, const char* c_pData);
#endif
		void 		PartyInvite(LPCHARACTER ch, const char * c_pData);
		void 		PartyInviteAnswer(LPCHARACTER ch, const char * c_pData);
		void		PartyRemove(LPCHARACTER ch, const char * c_pData);
		void		PartySetState(LPCHARACTER ch, const char * c_pData);
		void		PartyUseSkill(LPCHARACTER ch, const char * c_pData);
		void		PartyParameter(LPCHARACTER ch, const char * c_pData);

		int			Guild(LPCHARACTER ch, const char * data, size_t uiBytes);
		void		AnswerMakeGuild(LPCHARACTER ch, const char* c_pData);

		void		Fishing(LPCHARACTER ch, const char* c_pData);
		void		ItemGive(LPCHARACTER ch, const char* c_pData);
		void		Hack(LPCHARACTER ch, const char * c_pData);
		int			MyShop(LPCHARACTER ch, const char * c_pData, size_t uiBytes);
#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
		void 		CubeRenewalSend(LPCHARACTER ch, const char* data);
#endif
		void		Refine(LPCHARACTER ch, const char* c_pData);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		void		Acce(LPCHARACTER pkChar, const char* c_pData);
#endif
#ifdef ENABLE_OFFLINESHOP_SYSTEM
		int			OfflineShop(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			MyOfflineShop(LPCHARACTER ch, const char* c_pData, size_t uiBytes);
#endif
		void		Roulette(LPCHARACTER ch, const char* c_pData);
#ifdef __SEND_TARGET_INFO__
		void		TargetInfoLoad(LPCHARACTER ch, const char* c_pData);
#endif
#ifdef ENABLE_SWITCHBOT
		int			Switchbot(LPCHARACTER ch, const char* data, size_t uiBytes);
#endif
		void		ChangeLanguage(LPCHARACTER ch, BYTE bLanguage);
		void		WhisperDetails(LPCHARACTER ch, const char* c_pData);

};

class CInputDead : public CInputMain
{
	public:
		virtual BYTE	GetType() { return INPROC_DEAD; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);
};

class CInputDB : public CInputProcessor
{
public:
	virtual bool Process(LPDESC d, const void * c_pvOrig, int iBytes, int & r_iBytesProceed);
	virtual BYTE GetType() { return INPROC_DB; }

protected:
	virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

protected:
	void		MapLocations(const char * c_pData);
	void		LoginSuccess(DWORD dwHandle, const char *data);
	void		PlayerCreateFailure(LPDESC d, BYTE bType);
	void		PlayerDeleteSuccess(LPDESC d, const char * data);
	void		PlayerDeleteFail(LPDESC d);
	void		PlayerLoad(LPDESC d, const char* data);
	void		PlayerCreateSuccess(LPDESC d, const char * data);
	void		Boot(const char* data);
	void		QuestLoad(LPDESC d, const char * c_pData);
	void		SafeboxLoad(LPDESC d, const char * c_pData);
	void		SafeboxChangeSize(LPDESC d, const char * c_pData);
	void		SafeboxWrongPassword(LPDESC d);
	void		SafeboxChangePasswordAnswer(LPDESC d, const char* c_pData);
	void		MallLoad(LPDESC d, const char * c_pData);
	void		EmpireSelect(LPDESC d, const char * c_pData);
	void		P2P(const char * c_pData);
	void		ItemLoad(LPDESC d, const char * c_pData);
	void		AffectLoad(LPDESC d, const char * c_pData);
#if defined(__SKILL_COLOR_SYSTEM__)
	void		SkillColorLoad(LPDESC d, const char* c_pData);
#endif
	void		GuildLoad(const char * c_pData);
	void		GuildSkillUpdate(const char* c_pData);
	void		GuildSkillRecharge();
	void		GuildExpUpdate(const char* c_pData);
	void		GuildAddMember(const char* c_pData);
	void		GuildRemoveMember(const char* c_pData);
	void		GuildChangeGrade(const char* c_pData);
	void		GuildChangeMemberData(const char* c_pData);
	void		GuildDisband(const char* c_pData);
	void		GuildLadder(const char* c_pData);
	void		GuildWar(const char* c_pData);
	void		GuildWarScore(const char* c_pData);
	void		GuildSkillUsableChange(const char* c_pData);
	void		GuildMoneyChange(const char* c_pData);
	void		GuildWithdrawMoney(const char* c_pData);
	void		GuildWarReserveAdd(TGuildWarReserve * p);
	void		GuildWarReserveUpdate(TGuildWarReserve * p);
	void		GuildWarReserveDelete(DWORD dwID);
	void		GuildWarBet(TPacketGDGuildWarBet * p);
	void		GuildChangeMaster(TPacketChangeGuildMaster* p);

	void		LoginAlready(LPDESC d, const char * c_pData);

	void		PartyCreate(const char* c_pData);
	void		PartyDelete(const char* c_pData);
	void		PartyAdd(const char* c_pData);
	void		PartyRemove(const char* c_pData);
	void		PartyStateChange(const char* c_pData);
	void		PartySetMemberLevel(const char* c_pData);

	void		Time(const char * c_pData);

	void		ReloadProto(const char * c_pData);
	void		ChangeName(LPDESC d, const char * data);

	void		AuthLogin(LPDESC d, const char * c_pData);
	void		ItemAward(const char * c_pData);

	void		ChangeEmpirePriv(const char* c_pData);
	void		ChangeGuildPriv(const char* c_pData);
	void		ChangeCharacterPriv(const char* c_pData);

	void		MoneyLog(const char* c_pData);

	void		SetEventFlag(const char* c_pData);

	void		CreateObject(const char * c_pData);
	void		DeleteObject(const char * c_pData);
	void		UpdateLand(const char * c_pData);

	void		Notice(const char* c_pData);
	void		BigNotice(const char* c_pData);

	void		MarriageAdd(TPacketMarriageAdd * p);
	void		MarriageUpdate(TPacketMarriageUpdate * p);
	void		MarriageRemove(TPacketMarriageRemove * p);

	void		WeddingRequest(TPacketWeddingRequest* p);
	void		WeddingReady(TPacketWeddingReady* p);
	void		WeddingStart(TPacketWeddingStart* p);
	void		WeddingEnd(TPacketWeddingEnd* p);

	void		TakeMonarchMoney(LPDESC d, const char * data );
	void		AddMonarchMoney(LPDESC d, const char * data );
	void		DecMonarchMoney(LPDESC d, const char * data );
	void		SetMonarch( LPDESC d, const char * data );

	void		ChangeMonarchLord(TPacketChangeMonarchLordACK* data);
	void		UpdateMonarchInfo(TMonarchInfo* data);

	// MYSHOP_PRICE_LIST

	void		MyshopPricelistRes( LPDESC d, const TPacketMyshopPricelistHeader* p );
	// END_OF_MYSHOP_PRICE_LIST

	//RELOAD_ADMIN
	void		ReloadAdmin( const char * c_pData );
#ifdef __MAINTENANCE__
	void		Maintenance(const char* c_pData);
#endif
	//END_RELOAD_ADMIN
#ifdef ENABLE_EVENT_MANAGER
	void		EventManager(const char* c_pData);
#endif

	void		DetailLog(const TPacketNeedLoginLogInfo* info);

	void		ItemAwardInformer(TPacketItemAwardInfromer* data);
#if defined(BL_OFFLINE_MESSAGE)
	void		ReadOfflineMessages(LPDESC desc, const char* pcData);
#endif

	void		RespondChannelStatus(LPDESC desc, const char* pcData);
	protected:
		DWORD		m_dwHandle;
};

class CInputUDP : public CInputProcessor
{
	public:
		CInputUDP();
		virtual bool Process(LPDESC d, const void * c_pvOrig, int iBytes, int & r_iBytesProceed);

		virtual BYTE GetType() { return INPROC_UDP; }
		void		SetSockAddr(struct sockaddr_in & rSockAddr) { m_SockAddr = rSockAddr; };

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

	protected:
		void		Handshake(LPDESC lpDesc, const char * c_pData);
		void		StateChecker(const char * c_pData);

	protected:
		struct sockaddr_in	m_SockAddr;
		CPacketInfoUDP 		m_packetInfoUDP;
};

class CInputP2P : public CInputProcessor
{
	public:
		CInputP2P();
		virtual BYTE	GetType() { return INPROC_P2P; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

	public:
		void		Setup(LPDESC d, const char * c_pData);
		void		Login(LPDESC d, const char * c_pData);
		void		Logout(LPDESC d, const char * c_pData);
		int			Relay(LPDESC d, const char * c_pData, size_t uiBytes);
		int			Notice(LPDESC d, const char* c_pData, size_t uiBytes);
		int			BigNotice(LPDESC d, const char* c_pData, size_t uiBytes);
		int			MonarchNotice(LPDESC d, const char* c_pData, size_t uiBytes);
		int			MonarchTransfer(LPDESC d, const char * c_pData);
		int			Guild(LPDESC d, const char* c_pData, size_t uiBytes);
		void		Shout(const char * c_pData);
		void		Disconnect(const char * c_pData);
		void		MessengerAdd(const char * c_pData);
		void		MessengerRemove(const char * c_pData);
#if defined(__MESSENGER_BLOCK_SYSTEM__)
		void		MessengerBlockAdd(const char* c_pData);
		void		MessengerBlockRemove(const char* c_pData);
#endif
		void		FindPosition(LPDESC d, const char* c_pData);
		void		WarpCharacter(const char* c_pData);
		void		GuildWarZoneMapIndex(const char* c_pData);
		void		Transfer(const char * c_pData);
		void		XmasWarpSanta(const char * c_pData);
		void		XmasWarpSantaReply(const char * c_pData);
		void		LoginPing(LPDESC d, const char * c_pData);
		void		BlockChat(const char * c_pData);
		void		IamAwake(LPDESC d, const char * c_pData);

#ifdef ENABLE_ULTIMATE_REGEN
		void		NewRegen(const char* c_pData);
#endif
#ifdef CROSS_CHANNEL_FRIEND_REQUEST
		void		MessengerRequestAdd(const char* c_pData);
#endif
#ifdef ENABLE_MULTI_FARM_BLOCK
		void		MultiFarm(const char* c_pData);
#endif
#ifdef ENABLE_REWARD_SYSTEM
		void		RewardInfo(const char* c_pData);
#endif
#ifdef ENABLE_SWITCHBOT
		void		Switchbot(LPDESC d, const char* c_pData);
#endif
		void		LocaleNotice(const char* c_pData);

	protected:
		CPacketInfoGG 	m_packetInfoGG;
};

class CInputAuth : public CInputProcessor
{
	public:
		CInputAuth();
		virtual BYTE GetType() { return INPROC_AUTH; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

	public:
		void		Login(LPDESC d, const char * c_pData);
};

#endif /* __INC_METIN_II_GAME_INPUT_PROCESSOR__ */
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
