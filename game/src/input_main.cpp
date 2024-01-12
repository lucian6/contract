#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "protocol.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "cmd.h"
#include "shop.h"
#include "shop_manager.h"
#include "safebox.h"
#include "regen.h"
#include "battle.h"
#include "exchange.h"
#include "questmanager.h"
#include "profiler.h"
#include "messenger_manager.h"
#include "party.h"
#include "p2p.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"
#include "log.h"
#include "banword.h"
#include "empire_text_convert.h"
#include "unique_item.h"
#include "building.h"
#include "locale_service.h"
#include "gm.h"
#include "spam.h"
#include "ani.h"
#include "motion.h"
#include "OXEvent.h"
#include "locale_service.h"
#include "DragonSoul.h"
#include "belt_inventory_helper.h" // @fixme119
#include "../../common/CommonDefines.h"
#include "../../common/PulseManager.h"

#include "input.h"
#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#endif
#include <chrono>

#define ENABLE_CHAT_COLOR_SYSTEM
#define ENABLE_CHAT_LOGGING
#define ENABLE_CHAT_SPAMLIMIT
#define ENABLE_WHISPER_CHAT_SPAMLIMIT
#define ENABLE_CHECK_GHOSTMODE

#ifdef ENABLE_CHAT_LOGGING
static char	__escape_string[1024];
static char	__escape_string2[1024];
#endif

#if defined(__MESSENGER_BLOCK_SYSTEM__)
extern void SendShout(const char* szText, const char* c_szName, BYTE bEmpire);
#else
extern void SendShout(const char* szText, BYTE bEmpire);
#endif

static int __deposit_limit()
{
	return (1000*10000);
}

#ifdef __SEND_TARGET_INFO__
void CInputMain::TargetInfoLoad(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGTargetInfoLoad* p = (TPacketCGTargetInfoLoad*)c_pData;
	TPacketGCTargetInfo pInfo;
	pInfo.header = HEADER_GC_TARGET_INFO;
	static std::vector<std::pair<int,int> > s_vec_item;
	s_vec_item.clear();

	LPCHARACTER m_pkChrTarget = CHARACTER_MANAGER::instance().Find(p->dwVID);


	if (!ch || !m_pkChrTarget)
		return;

	// if (m_pkChrTarget && (m_pkChrTarget->IsMonster() || m_pkChrTarget->IsStone()))
	// {
		// if (thecore_heart->pulse - (int) ch->GetLastTargetInfoPulse() < passes_per_sec * 3)
			// return;

		// ch->SetLastTargetInfoPulse(thecore_heart->pulse);

	if ((m_pkChrTarget->IsMonster() || m_pkChrTarget->IsStone()) && ITEM_MANAGER::instance().CreateDropItemVector(m_pkChrTarget, ch, s_vec_item))
	{
		for(std::vector<std::pair<int,int> >::const_iterator iter = s_vec_item.begin(); iter != s_vec_item.end();++iter)
		{
			pInfo.dwVID	= m_pkChrTarget->GetVID();
			pInfo.race = m_pkChrTarget->GetRaceNum();
			pInfo.dwVnum = iter->first;
			pInfo.count = iter->second;
			ch->GetDesc()->Packet(&pInfo, sizeof(TPacketGCTargetInfo));
		}
	}
	// }
}
#endif

void SendBlockChatInfo(LPCHARACTER ch, int sec)
{
	if (sec <= 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_STRING("채팅 금지 상태입니다.", ch->GetLanguage()));
		return;
	}

	long hour = sec / 3600;
	sec -= hour * 3600;

	long min = (sec / 60);
	sec -= min * 60;

	char buf[128+1];

	if (hour > 0 && min > 0)
		snprintf(buf, sizeof(buf), LC_STRING("%d 시간 %d 분 %d 초 동안 채팅금지 상태입니다", ch->GetLanguage()), hour, min, sec);
	else if (hour > 0 && min == 0)
		snprintf(buf, sizeof(buf), LC_STRING("%d 시간 %d 초 동안 채팅금지 상태입니다", ch->GetLanguage()), hour, sec);
	else if (hour == 0 && min > 0)
		snprintf(buf, sizeof(buf), LC_STRING("%d 분 %d 초 동안 채팅금지 상태입니다", ch->GetLanguage()), min, sec);
	else
		snprintf(buf, sizeof(buf), LC_STRING("%d 초 동안 채팅금지 상태입니다", ch->GetLanguage()), sec);

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
}

EVENTINFO(spam_event_info)
{
	char host[MAX_HOST_LENGTH+1];

	spam_event_info()
	{
		::memset( host, 0, MAX_HOST_LENGTH+1 );
	}
};

typedef boost::unordered_map<std::string, std::pair<unsigned int, LPEVENT> > spam_score_of_ip_t;
spam_score_of_ip_t spam_score_of_ip;

EVENTFUNC(block_chat_by_ip_event)
{
	spam_event_info* info = dynamic_cast<spam_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "block_chat_by_ip_event> <Factor> Null pointer" );
		return 0;
	}

	const char * host = info->host;

	spam_score_of_ip_t::iterator it = spam_score_of_ip.find(host);

	if (it != spam_score_of_ip.end())
	{
		it->second.first = 0;
		it->second.second = NULL;
	}

	return 0;
}

bool SpamBlockCheck(LPCHARACTER ch, const char* const buf, const size_t buflen)
{
	if (ch->GetLevel() < g_iSpamBlockMaxLevel)
	{
		spam_score_of_ip_t::iterator it = spam_score_of_ip.find(ch->GetDesc()->GetHostName());

		if (it == spam_score_of_ip.end())
		{
			spam_score_of_ip.emplace(ch->GetDesc()->GetHostName(), std::make_pair(0, (LPEVENT) NULL));
			it = spam_score_of_ip.find(ch->GetDesc()->GetHostName());
		}

		if (it->second.second)
		{
			SendBlockChatInfo(ch, event_time(it->second.second) / passes_per_sec);
			return true;
		}

		unsigned int score;
		const char * word = SpamManager::instance().GetSpamScore(buf, buflen, score);

		it->second.first += score;

		if (word)
			sys_log(0, "SPAM_SCORE: %s text: %s score: %u total: %u word: %s", ch->GetName(), buf, score, it->second.first, word);

		if (it->second.first >= g_uiSpamBlockScore)
		{
			spam_event_info* info = AllocEventInfo<spam_event_info>();
			strlcpy(info->host, ch->GetDesc()->GetHostName(), sizeof(info->host));

			it->second.second = event_create(block_chat_by_ip_event, info, PASSES_PER_SEC(g_uiSpamBlockDuration));
			sys_log(0, "SPAM_IP: %s for %u seconds", info->host, g_uiSpamBlockDuration);
			SendBlockChatInfo(ch, event_time(it->second.second) / passes_per_sec);

			return true;
		}
	}

	return false;
}

enum
{
	TEXT_TAG_PLAIN,
	TEXT_TAG_TAG, // ||
	TEXT_TAG_COLOR, // |cffffffff
	TEXT_TAG_HYPERLINK_START, // |H
	TEXT_TAG_HYPERLINK_END, // |h ex) |Hitem:1234:1:1:1|h
	TEXT_TAG_RESTORE_COLOR,
};

int GetTextTag(const char* src, int maxLen, int& tagLen, std::string& extraInfo)
{
	tagLen = 1;

	if (maxLen < 2 || *src != '|')
		return TEXT_TAG_PLAIN;

	const char* cur = ++src;

	if (*cur == '|') // ||는 |로 표시한다.
	{
		tagLen = 2;
		return TEXT_TAG_TAG;
	}
	else if (*cur == 'c') // color |cffffffffblahblah|r
	{
		tagLen = 2;
		return TEXT_TAG_COLOR;
	}
	else if (*cur == 'H') // hyperlink |Hitem:10000:0:0:0:0|h[이름]|h
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_START;
	}
	else if (*cur == 'h') // end of hyperlink
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_END;
	}

	return TEXT_TAG_PLAIN;
}

void GetTextTagInfo(const char* src, int src_len, int& hyperlinks, bool& colored)
{
	colored = false;
	hyperlinks = 0;

	int len;
	std::string extraInfo;

	for (int i = 0; i < src_len;)
	{
		int tag = GetTextTag(&src[i], src_len - i, len, extraInfo);

		if (tag == TEXT_TAG_HYPERLINK_START)
			++hyperlinks;

		if (tag == TEXT_TAG_COLOR)
			colored = true;

		i += len;
	}
}

int ProcessTextTag(LPCHARACTER ch, const char * c_pszText, size_t len)
{
	int hyperlinks;
	bool colored;

	GetTextTagInfo(c_pszText, len, hyperlinks, colored);

	if (colored == true && hyperlinks == 0)
		return 4;

#ifdef ENABLE_NEWSTUFF
	if (g_bDisablePrismNeed)
		return 0;
#endif
	int nPrismCount = ch->CountSpecifyItem(ITEM_PRISM);

	if (nPrismCount < hyperlinks)
		return 1;

	if (!ch->GetMyShop())
	{
		ch->RemoveSpecifyItem(ITEM_PRISM, hyperlinks);
		return 0;
	} else
	{
		int sellingNumber = ch->GetMyShop()->GetNumberByVnum(ITEM_PRISM);
		if(nPrismCount - sellingNumber < hyperlinks)
		{
			return 2;
		} else
		{
			ch->RemoveSpecifyItem(ITEM_PRISM, hyperlinks);
			return 0;
		}
	}

	return 4;
}

int CInputMain::Whisper(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	const TPacketCGWhisper* pinfo = reinterpret_cast<const TPacketCGWhisper*>(data);

	if (uiBytes < pinfo->wSize)
		return -1;

	int iExtraLen = pinfo->wSize - sizeof(TPacketCGWhisper);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->wSize, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

#ifdef ENABLE_WHISPER_CHAT_SPAMLIMIT
	if (ch->IncreaseChatCounter() >= 10)
	{
		ch->GetDesc()->DelayedDisconnect(0);
		return (iExtraLen);
	}
#endif

	if (ch->FindAffect(AFFECT_BLOCK_CHAT))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("채팅 금지 상태입니다."));
		return (iExtraLen);
	}

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(pinfo->szNameTo);

	if (pkChr == ch)
		return (iExtraLen);

	LPDESC pkDesc = NULL;

	BYTE bOpponentEmpire = 0;

	if (test_server)
	{
		if (!pkChr)
			sys_log(0, "Whisper to %s(%s) from %s", "Null", pinfo->szNameTo, ch->GetName());
		else
			sys_log(0, "Whisper to %s(%s) from %s", pkChr->GetName(), pinfo->szNameTo, ch->GetName());
	}

	if (ch->IsBlockMode(BLOCK_WHISPER))
	{
		if (ch->GetDesc())
		{
			TPacketGCWhisper pack;
			pack.bHeader = HEADER_GC_WHISPER;
			pack.bType = WHISPER_TYPE_SENDER_BLOCKED;
			pack.wSize = sizeof(TPacketGCWhisper);
			strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
			ch->GetDesc()->Packet(&pack, sizeof(pack));
		}
		return iExtraLen;
	}

	if (!pkChr)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(pinfo->szNameTo);

		if (pkCCI)
		{
			pkDesc = pkCCI->pkDesc;
			pkDesc->SetRelay(pinfo->szNameTo);
			bOpponentEmpire = pkCCI->bEmpire;

			if (test_server)
				sys_log(0, "Whisper to %s from %s (Channel %d Mapindex %d)", "Null", ch->GetName(), pkCCI->bChannel, pkCCI->lMapIndex);
#ifdef ENABLE_BATTLE_PASS
			CHARACTER_MANAGER::Instance().DoMission(ch, MISSION_WHISPER, 1, 0);
#endif
		}
	}
	else
	{
		pkDesc = pkChr->GetDesc();
		bOpponentEmpire = pkChr->GetEmpire();
	}

	if (!pkDesc)
	{
		if (ch->GetDesc())
		{
#if defined(BL_OFFLINE_MESSAGE)
			const BYTE bDelay = 10;
			char msg[CHAT_MAX_LEN + 1];
			
			TPacketGCWhisper pack;

			if (get_dword_time() - ch->GetLastOfflinePMTime() > bDelay * 1000)
			{
				char buf[CHAT_MAX_LEN + 1];
				strlcpy(buf, data + sizeof(TPacketCGWhisper), MIN(iExtraLen + 1, sizeof(buf)));
				const size_t buflen = strlen(buf);

				if (MessengerManager::instance().IsBlocked(ch->GetName(), pinfo->szNameTo))
				{
					snprintf(msg, sizeof(msg), LC_TEXT("Unblock %s to continue."), pinfo->szNameTo);
					int len = MIN(CHAT_MAX_LEN, strlen(msg) + 1);

					pack.bHeader = HEADER_GC_WHISPER;
					pack.wSize = sizeof(TPacketGCWhisper) + len;
					pack.bType = WHISPER_TYPE_SYSTEM;
					strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

					TEMP_BUFFER buf;

					buf.write(&pack, sizeof(TPacketGCWhisper));
					buf.write(msg, len);
					ch->GetDesc()->Packet(buf.read_peek(), buf.size());

					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), pinfo->szNameTo);
				}
				else if (MessengerManager::instance().IsBlocked(pinfo->szNameTo, ch->GetName()))
				{

					snprintf(msg, sizeof(msg), LC_TEXT("%s has blocked you."), pinfo->szNameTo);
					int len = MIN(CHAT_MAX_LEN, strlen(msg) + 1);

					pack.bHeader = HEADER_GC_WHISPER;
					pack.wSize = sizeof(TPacketGCWhisper) + len;
					pack.bType = WHISPER_TYPE_SYSTEM;
					strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

					TEMP_BUFFER buf;

					buf.write(&pack, sizeof(TPacketGCWhisper));
					buf.write(msg, len);
					ch->GetDesc()->Packet(buf.read_peek(), buf.size());

					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s has blocked you."), pinfo->szNameTo);
				}
				else
				{
					if (buflen > 0) 
					{
						ch->SendOfflineMessage(pinfo->szNameTo, buf);
						snprintf(msg, sizeof(msg), "%s %s", LC_STRING("상대방이 온라인 상태가 아닙니다.", ch->GetLanguage()), LC_STRING("문자 메세지를 전송 했습니다.", ch->GetLanguage()));
					}
					else
						return (iExtraLen);
				}
			}
			else
			{
				snprintf(msg, sizeof(msg), LC_STRING("You have to wait %d s! Try again later.", ch->GetLanguage()), bDelay);
			}

			/*Info*/

			int len = MIN(CHAT_MAX_LEN, strlen(msg) + 1);
			pack.bHeader = HEADER_GC_WHISPER;
			pack.wSize = static_cast<WORD>(sizeof(TPacketGCWhisper) + len);
			pack.bType = WHISPER_TYPE_SYSTEM;
			strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

			TEMP_BUFFER buf;
			buf.write(&pack, sizeof(TPacketGCWhisper));
			buf.write(msg, len);
			ch->GetDesc()->Packet(buf.read_peek(), buf.size());

#else
			TPacketGCWhisper pack;

			pack.bHeader = HEADER_GC_WHISPER;
			pack.bType = WHISPER_TYPE_NOT_EXIST;
			pack.wSize = sizeof(TPacketGCWhisper);
			strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
			ch->GetDesc()->Packet(&pack, sizeof(TPacketGCWhisper));
			sys_log(0, "WHISPER: no player");
#endif
		}
	}
	else
	{
		if (ch->IsBlockMode(BLOCK_WHISPER))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;
				pack.bHeader = HEADER_GC_WHISPER;
				pack.bType = WHISPER_TYPE_SENDER_BLOCKED;
				pack.wSize = sizeof(TPacketGCWhisper);
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
		}
		else if (pkChr && pkChr->IsBlockMode(BLOCK_WHISPER))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;
				pack.bHeader = HEADER_GC_WHISPER;
				pack.bType = WHISPER_TYPE_TARGET_BLOCKED;
				pack.wSize = sizeof(TPacketGCWhisper);
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
		}
#if defined(__MESSENGER_BLOCK_SYSTEM__)
		else if (pkDesc && MessengerManager::instance().IsBlocked(ch->GetName(), pinfo->szNameTo))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;

				char msg[CHAT_MAX_LEN + 1];
				snprintf(msg, sizeof(msg), LC_TEXT("Unblock %s to continue."), pinfo->szNameTo);
				int len = MIN(CHAT_MAX_LEN, strlen(msg) + 1);

				pack.bHeader = HEADER_GC_WHISPER;
				pack.wSize = sizeof(TPacketGCWhisper) + len;
				pack.bType = WHISPER_TYPE_SYSTEM;
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

				TEMP_BUFFER buf;

				buf.write(&pack, sizeof(TPacketGCWhisper));
				buf.write(msg, len);
				ch->GetDesc()->Packet(buf.read_peek(), buf.size());

				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), pinfo->szNameTo);
			}
		}
		else if (pkDesc && MessengerManager::instance().IsBlocked(pinfo->szNameTo, ch->GetName()))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;

				char msg[CHAT_MAX_LEN + 1];
				snprintf(msg, sizeof(msg), LC_TEXT("%s has blocked you."), pinfo->szNameTo);
				int len = MIN(CHAT_MAX_LEN, strlen(msg) + 1);

				pack.bHeader = HEADER_GC_WHISPER;
				pack.wSize = sizeof(TPacketGCWhisper) + len;
				pack.bType = WHISPER_TYPE_SYSTEM;
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

				TEMP_BUFFER buf;

				buf.write(&pack, sizeof(TPacketGCWhisper));
				buf.write(msg, len);
				ch->GetDesc()->Packet(buf.read_peek(), buf.size());

				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s has blocked you."), pinfo->szNameTo);
			}
		}
#endif
		else
		{
			BYTE bType = WHISPER_TYPE_NORMAL;

			char buf[CHAT_MAX_LEN + 1];
			strlcpy(buf, data + sizeof(TPacketCGWhisper), MIN(iExtraLen + 1, sizeof(buf)));
			const size_t buflen = strlen(buf);

			if (true == SpamBlockCheck(ch, buf, buflen))
			{
				if (!pkChr)
				{
					CCI * pkCCI = P2P_MANAGER::instance().Find(pinfo->szNameTo);

					if (pkCCI)
					{
						pkDesc->SetRelay("");
					}
				}
				return iExtraLen;
			}

			CBanwordManager::instance().ConvertString(buf, buflen);

			if (g_bEmpireWhisper)
				if (!ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
					if (!(pkChr && pkChr->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE)))
						if (bOpponentEmpire != ch->GetEmpire() && ch->GetEmpire() && bOpponentEmpire
								&& ch->GetGMLevel() == GM_PLAYER && gm_get_level(pinfo->szNameTo) == GM_PLAYER)

						{
							if (!pkChr)
							{
								bType = ch->GetEmpire() << 4;
							}
							else
							{
								ConvertEmpireText(ch->GetEmpire(), buf, buflen, 10 + 2 * pkChr->GetSkillPower(SKILL_LANGUAGE1 + ch->GetEmpire() - 1));
							}
						}

			int processReturn = ProcessTextTag(ch, buf, buflen);
			if (0!=processReturn)
			{
				if (ch->GetDesc())
				{
					TItemTable * pTable = ITEM_MANAGER::instance().GetTable(ITEM_PRISM);

					if (pTable)
					{
						char buf[128];
						int len;
						if (3==processReturn)
							len = snprintf(buf, sizeof(buf), LC_TEXT("사용할수 없습니다."), pTable->szLocaleName);
						else
							len = snprintf(buf, sizeof(buf), LC_TEXT("%s이 필요합니다."), pTable->szLocaleName);

						if (len < 0 || len >= (int) sizeof(buf))
							len = sizeof(buf) - 1;

						++len;

						TPacketGCWhisper pack;

						pack.bHeader = HEADER_GC_WHISPER;
						pack.bType = WHISPER_TYPE_ERROR;
						pack.wSize = sizeof(TPacketGCWhisper) + len;
						strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

						ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
						ch->GetDesc()->Packet(buf, len);

						sys_log(0, "WHISPER: not enough %s: char: %s", pTable->szLocaleName, ch->GetName());
					}
				}

				pkDesc->SetRelay("");
				return (iExtraLen);
			}

			if (ch->IsGM())
				bType = (bType & 0xF0) | WHISPER_TYPE_GM;

			if (buflen > 0)
			{
				TPacketGCWhisper pack;

				pack.bHeader = HEADER_GC_WHISPER;
				pack.wSize = sizeof(TPacketGCWhisper) + buflen;
				pack.bType = bType;
				strlcpy(pack.szNameFrom, ch->GetName(), sizeof(pack.szNameFrom));

				TEMP_BUFFER tmpbuf;

				tmpbuf.write(&pack, sizeof(pack));
				tmpbuf.write(buf, buflen);

				pkDesc->Packet(tmpbuf.read_peek(), tmpbuf.size());
#ifdef ENABLE_BATTLE_PASS
				CHARACTER_MANAGER::Instance().DoMission(ch, MISSION_WHISPER, 1, 0);
#endif
				// @warme006
				// sys_log(0, "WHISPER: %s -> %s : %s", ch->GetName(), pinfo->szNameTo, buf);
#ifdef ENABLE_CHAT_LOGGING
				if (ch->IsGM())
				{
					LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), buf, buflen);
					LogManager::instance().EscapeString(__escape_string2, sizeof(__escape_string2), pinfo->szNameTo, sizeof(pack.szNameFrom));
					LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), 0, __escape_string2, "WHISPER", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
				}
#endif
			}
		}
	}
	if(pkDesc)
		pkDesc->SetRelay("");

	return (iExtraLen);
}

struct RawPacketToCharacterFunc
{
	const void * m_buf;
	int	m_buf_len;

	RawPacketToCharacterFunc(const void * buf, int buf_len) : m_buf(buf), m_buf_len(buf_len)
	{
	}

	void operator () (LPCHARACTER c)
	{
		if (!c->GetDesc())
			return;

		c->GetDesc()->Packet(m_buf, m_buf_len);
	}
};

struct FEmpireChatPacket
{
	packet_chat& p;
	const char* orig_msg;
	int orig_len;
	char converted_msg[CHAT_MAX_LEN+1];

	BYTE bEmpire;
	int iMapIndex;
	int namelen;

	FEmpireChatPacket(packet_chat& p, const char* chat_msg, int len, BYTE bEmpire, int iMapIndex, int iNameLen)
		: p(p), orig_msg(chat_msg), orig_len(len), bEmpire(bEmpire), iMapIndex(iMapIndex), namelen(iNameLen)
	{
		memset( converted_msg, 0, sizeof(converted_msg) );
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetMapIndex() != iMapIndex)
			return;

		d->BufferedPacket(&p, sizeof(packet_chat));

		if (d->GetEmpire() == bEmpire ||
			bEmpire == 0 ||
			d->GetCharacter()->GetGMLevel() > GM_PLAYER ||
			d->GetCharacter()->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
		{
			d->Packet(orig_msg, orig_len);
		}
		else
		{
			size_t len = strlcpy(converted_msg, orig_msg, sizeof(converted_msg));

			if (len >= sizeof(converted_msg))
				len = sizeof(converted_msg) - 1;

			ConvertEmpireText(bEmpire, converted_msg + namelen, len - namelen, 10 + 2 * d->GetCharacter()->GetSkillPower(SKILL_LANGUAGE1 + bEmpire - 1));
			d->Packet(converted_msg, orig_len);
		}
	}
};

struct FYmirChatPacket
{
	packet_chat& packet;
	const char* m_szChat;
	size_t m_lenChat;
	const char* m_szName;

	int m_iMapIndex;
	BYTE m_bEmpire;
	bool m_ring;

	char m_orig_msg[CHAT_MAX_LEN+1];
	int m_len_orig_msg;
	char m_conv_msg[CHAT_MAX_LEN+1];
	int m_len_conv_msg;

	FYmirChatPacket(packet_chat& p, const char* chat, size_t len_chat, const char* name, size_t len_name, int iMapIndex, BYTE empire, bool ring)
		: packet(p),
		m_szChat(chat), m_lenChat(len_chat),
		m_szName(name),
		m_iMapIndex(iMapIndex), m_bEmpire(empire),
		m_ring(ring)
	{
		m_len_orig_msg = snprintf(m_orig_msg, sizeof(m_orig_msg), "%s : %s", m_szName, m_szChat) + 1;

		if (m_len_orig_msg < 0 || m_len_orig_msg >= (int) sizeof(m_orig_msg))
			m_len_orig_msg = sizeof(m_orig_msg) - 1;

		m_len_conv_msg = snprintf(m_conv_msg, sizeof(m_conv_msg), "??? : %s", m_szChat) + 1;

		if (m_len_conv_msg < 0 || m_len_conv_msg >= (int) sizeof(m_conv_msg))
			m_len_conv_msg = sizeof(m_conv_msg) - 1;

		ConvertEmpireText(m_bEmpire, m_conv_msg + 6, m_len_conv_msg - 6, 10);
	}

	void operator() (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetMapIndex() != m_iMapIndex)
			return;

		if (m_ring ||
			d->GetEmpire() == m_bEmpire ||
			d->GetCharacter()->GetGMLevel() > GM_PLAYER ||
			d->GetCharacter()->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
		{
			packet.size = m_len_orig_msg + sizeof(TPacketGCChat);

			d->BufferedPacket(&packet, sizeof(packet_chat));
			d->Packet(m_orig_msg, m_len_orig_msg);
		}
		else
		{
			packet.size = m_len_conv_msg + sizeof(TPacketGCChat);

			d->BufferedPacket(&packet, sizeof(packet_chat));
			d->Packet(m_conv_msg, m_len_conv_msg);
		}
	}
};

int CInputMain::Chat(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	const TPacketCGChat* pinfo = reinterpret_cast<const TPacketCGChat*>(data);

	if (uiBytes < pinfo->size)
		return -1;

	const int iExtraLen = pinfo->size - sizeof(TPacketCGChat);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->size, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

	// buf = chat - (lungimea numelui pentru link) - numele afi?tat -  
	char buf[CHAT_MAX_LEN - (CHARACTER_NAME_MAX_LEN*2 + 3) - 16 - 41 + 1];
	strlcpy(buf, data + sizeof(TPacketCGChat), MIN(iExtraLen + 1, sizeof(buf)));
	const size_t buflen = strlen(buf);

	if (buflen > 1 && *buf == '/')
	{
		interpret_command(ch, buf + 1, buflen - 1);
		return iExtraLen;
	}
#ifdef ENABLE_CHAT_SPAMLIMIT
	if (ch->IncreaseChatCounter() >= 4)
	{
		if (ch->GetChatCounter() == 10)
			ch->GetDesc()->DelayedDisconnect(0);
		return iExtraLen;
	}
#else
	if (ch->IncreaseChatCounter() >= 10)
	{
		if (ch->GetChatCounter() == 10)
		{
			sys_log(0, "CHAT_HACK: %s", ch->GetName());
			ch->GetDesc()->DelayedDisconnect(5);
		}

		return iExtraLen;
	}
#endif

	const CAffect* pAffect = ch->FindAffect(AFFECT_BLOCK_CHAT);

	if (pAffect != NULL)
	{
		SendBlockChatInfo(ch, pAffect->lDuration);
		return iExtraLen;
	}

	if (true == SpamBlockCheck(ch, buf, buflen))
	{
		return iExtraLen;
	}

	char chatbuf[CHAT_MAX_LEN + 1];
	int len;
	if (pinfo->type == CHAT_TYPE_SHOUT)
	{
		len = snprintf(chatbuf, sizeof(chatbuf), "|Hmsg:%s|h%s|h|r : %s", ch->GetName(), ch->GetName(), buf);
	}
	else
	{
		len = snprintf(chatbuf, sizeof(chatbuf), "%s : %s", ch->GetName(), buf);
	}

	CBanwordManager::instance().ConvertString(buf, buflen);

	if (len < 0 || len >= (int) sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	if (pinfo->type == CHAT_TYPE_SHOUT)
	{
		if (ch->GetLevel() < g_iShoutLimitLevel)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("외치기는 레벨 %d 이상만 사용 가능 합니다."), g_iShoutLimitLevel);
			return (iExtraLen);
		}

		if (thecore_heart->pulse - (int) ch->GetLastShoutPulse() < passes_per_sec * 15)
			return (iExtraLen);

		ch->SetLastShoutPulse(thecore_heart->pulse);

		char szLocaleName[24];
		sprintf(szLocaleName, "|Elocale_flag/%s|e", GetLocaleCodeName(ch->GetLanguage()));

		char szGlobalChat[CHAT_MAX_LEN];

		std::string strEmpireFlagToken;
		switch (ch->GetEmpire())
		{
			case 1: strEmpireFlagToken = "|Eempire_flag/shinsoo|e"; break;
			case 2: strEmpireFlagToken = "|Eempire_flag/chunjo|e"; break;
			case 3: strEmpireFlagToken = "|Eempire_flag/jinno|e"; break;
		}

		sprintf(szGlobalChat, "%s %s %s", szLocaleName, strEmpireFlagToken.c_str(), chatbuf);
		sprintf(chatbuf, "%s", szGlobalChat);

		TPacketGGShout p;

		p.bHeader = HEADER_GG_SHOUT;
		p.bEmpire = ch->GetEmpire();
		strlcpy(p.szText, chatbuf, sizeof(p.szText));

		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShout));
#ifdef ENABLE_BATTLE_PASS
		CHARACTER_MANAGER::Instance().DoMission(ch, MISSION_SHOUT_CHAT, 1, 0);
#endif
		ch->SetStatistics(STAT_TYPE_SHOUT, 1);

#if defined(__MESSENGER_BLOCK_SYSTEM__)
		SendShout(chatbuf, ch->GetName(), ch->GetEmpire());
#else
		SendShout(chatbuf, ch->GetEmpire());
#endif

		return (iExtraLen);
	}

	TPacketGCChat pack_chat;

	pack_chat.header = HEADER_GC_CHAT;
	pack_chat.size = sizeof(TPacketGCChat) + len;
	pack_chat.type = pinfo->type;
	pack_chat.id = ch->GetVID();

	switch (pinfo->type)
	{
		case CHAT_TYPE_TALKING:
			{
				const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

				if (false)
				{
					std::for_each(c_ref_set.begin(), c_ref_set.end(),
							FYmirChatPacket(pack_chat,
								buf,
								strlen(buf),
								ch->GetName(),
								strlen(ch->GetName()),
								ch->GetMapIndex(),
								ch->GetEmpire(),
								ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE)));
				}
				else
				{
					std::for_each(c_ref_set.begin(), c_ref_set.end(),
							FEmpireChatPacket(pack_chat,
								chatbuf,
								len,
								(ch->GetGMLevel() > GM_PLAYER ||
								 ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE)) ? 0 : ch->GetEmpire(),
								ch->GetMapIndex(), strlen(ch->GetName())));
#ifdef ENABLE_CHAT_LOGGING
					if (ch->IsGM())
					{
						LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), chatbuf, len);
						LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), 0, "", "NORMAL", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
					}
#endif
				}
			}
			break;

		case CHAT_TYPE_PARTY:
			{
				if (!ch->GetParty())
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("파티 중이 아닙니다."));
				else
				{
					TEMP_BUFFER tbuf;

					tbuf.write(&pack_chat, sizeof(pack_chat));
					tbuf.write(chatbuf, len);

					RawPacketToCharacterFunc f(tbuf.read_peek(), tbuf.size());
					ch->GetParty()->ForEachOnlineMember(f);
#ifdef ENABLE_CHAT_LOGGING
					if (ch->IsGM())
					{
						LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), chatbuf, len);
						LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), ch->GetParty()->GetLeaderPID(), "", "PARTY", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
					}
#endif
				}
			}
			break;

		case CHAT_TYPE_GUILD:
			{
				if (!ch->GetGuild())
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("길드에 가입하지 않았습니다."));
				else
				{
					ch->GetGuild()->Chat(chatbuf);
#ifdef ENABLE_CHAT_LOGGING
					if (ch->IsGM())
					{
						LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), chatbuf, len);
						LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), ch->GetGuild()->GetID(), ch->GetGuild()->GetName(), "GUILD", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
					}
#endif
				}
			}
			break;

		default:
			sys_err("Unknown chat type %d", pinfo->type);
			break;
	}

	return (iExtraLen);
}

void CInputMain::ItemUse(LPCHARACTER ch, const char * data)
{
	ch->UseItem(((struct command_item_use *) data)->Cell);
}

void CInputMain::ItemToItem(LPCHARACTER ch, const char * pcData)
{
	TPacketCGItemUseToItem * p = (TPacketCGItemUseToItem *) pcData;
	if (ch)
		ch->UseItem(p->Cell, p->TargetCell);
}

void CInputMain::ItemDrop(LPCHARACTER ch, const char * data)
{
	struct command_item_drop * pinfo = (struct command_item_drop *) data;

	//MONARCH_LIMIT
	//if (ch->IsMonarch())
	//	return;
	//END_MONARCH_LIMIT
	if (!ch)
		return;

	if (pinfo->gold > 0)
		ch->ChatPacket(CHAT_TYPE_INFO, "You can't drop gold.");
	else
		ch->DropItem(pinfo->Cell);
}

void CInputMain::ItemDrop2(LPCHARACTER ch, const char * data)
{
	//MONARCH_LIMIT
	//if (ch->IsMonarch())
	//	return;
	//END_MONARCH_LIMIT

	TPacketCGItemDrop2 * pinfo = (TPacketCGItemDrop2 *) data;

	if (!ch)
		return;
	if (pinfo->gold > 0)
		ch->ChatPacket(CHAT_TYPE_INFO, "You can't drop gold.");
	else
		ch->DropItem(pinfo->Cell, pinfo->count);
}

void CInputMain::ItemDestroy(LPCHARACTER ch, const char * data)
{
	struct command_item_destroy * pinfo = (struct command_item_destroy *) data;
	if (ch)
		ch->DestroyItem(pinfo->Cell);
}

#ifdef ENABLE_CHEST_OPEN_RENEWAL
void CInputMain::ChestOpen(LPCHARACTER ch, const char * data)
{
	struct OpenChestPacket * pinfo = (struct OpenChestPacket *) data;
	if (ch)
		ch->OpenChestItem(pinfo->pos, static_cast<WORD>(pinfo->wOpenCount));
}
#endif

void CInputMain::ItemMove(LPCHARACTER ch, const char * data)
{
	struct command_item_move * pinfo = (struct command_item_move *) data;

	if (ch)
		ch->MoveItem(pinfo->Cell, pinfo->CellTo, pinfo->count);
}

void CInputMain::ItemPickup(LPCHARACTER ch, const char * data)
{
	struct command_item_pickup * pinfo = (struct command_item_pickup*) data;
	if (ch)
		ch->PickupItem(pinfo->vid);
}

void CInputMain::QuickslotAdd(LPCHARACTER ch, const char * data)
{
	struct command_quickslot_add * pinfo = (struct command_quickslot_add *) data;
	ch->SetQuickslot(pinfo->pos, pinfo->slot);
}

void CInputMain::QuickslotDelete(LPCHARACTER ch, const char * data)
{
	struct command_quickslot_del * pinfo = (struct command_quickslot_del *) data;
	ch->DelQuickslot(pinfo->pos);
}

void CInputMain::QuickslotSwap(LPCHARACTER ch, const char * data)
{
	struct command_quickslot_swap * pinfo = (struct command_quickslot_swap *) data;
	ch->SwapQuickslot(pinfo->pos, pinfo->change_pos);
}

int CInputMain::Messenger(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGMessenger* p = (TPacketCGMessenger*) c_pData;

	if (uiBytes < sizeof(TPacketCGMessenger))
		return -1;

	c_pData += sizeof(TPacketCGMessenger);
	uiBytes -= sizeof(TPacketCGMessenger);

	switch (p->subheader)
	{
		case MESSENGER_SUBHEADER_CG_ADD_BY_VID:
			{
				if (uiBytes < sizeof(TPacketCGMessengerAddByVID))
					return -1;

				TPacketCGMessengerAddByVID * p2 = (TPacketCGMessengerAddByVID *) c_pData;
				LPCHARACTER ch_companion = CHARACTER_MANAGER::instance().Find(p2->vid);

				if (!ch_companion)
					return sizeof(TPacketCGMessengerAddByVID);

				if (ch->IsObserverMode())
					return sizeof(TPacketCGMessengerAddByVID);

				if (ch_companion->IsBlockMode(BLOCK_MESSENGER_INVITE))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 메신져 추가 거부 상태입니다."));
					return sizeof(TPacketCGMessengerAddByVID);
				}

				LPDESC d = ch_companion->GetDesc();

				if (!d)
					return sizeof(TPacketCGMessengerAddByVID);

				if (ch->GetGMLevel() == GM_PLAYER && ch_companion->GetGMLevel() != GM_PLAYER)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<메신져> 운영자는 메신져에 추가할 수 없습니다."));
					return sizeof(TPacketCGMessengerAddByVID);
				}

#if defined(__MESSENGER_BLOCK_SYSTEM__)
				if (MessengerManager::instance().IsBlocked(ch->GetName(), ch_companion->GetName()))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), ch_companion->GetName());
					return sizeof(TPacketCGMessengerAddByVID);
				}
				else if (MessengerManager::instance().IsBlocked(ch_companion->GetName(), ch->GetName()))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s has blocked you."), ch_companion->GetName());
					return sizeof(TPacketCGMessengerAddByVID);
				}
#endif

				if (ch->GetDesc() == d)
					return sizeof(TPacketCGMessengerAddByVID);

				MessengerManager::instance().RequestToAdd(ch, ch_companion);
				//MessengerManager::instance().AddToList(ch->GetName(), ch_companion->GetName());
			}
			return sizeof(TPacketCGMessengerAddByVID);

		case MESSENGER_SUBHEADER_CG_ADD_BY_NAME:
			{
				if (uiBytes < CHARACTER_NAME_MAX_LEN)
					return -1;

				char name[CHARACTER_NAME_MAX_LEN + 1];
				strlcpy(name, c_pData, sizeof(name));

				if (ch->GetGMLevel() == GM_PLAYER && gm_get_level(name) != GM_PLAYER)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<메신져> 운영자는 메신져에 추가할 수 없습니다."));
					return CHARACTER_NAME_MAX_LEN;
				}

				LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

				if (!tch)
#ifdef CROSS_CHANNEL_FRIEND_REQUEST
					MessengerManager::instance().P2PRequestToAdd_Stage1(ch, name);
#endif			
				else
				{
					if (tch == ch)
						return CHARACTER_NAME_MAX_LEN;

#if defined(__MESSENGER_BLOCK_SYSTEM__)
					if (MessengerManager::instance().IsBlocked(ch->GetName(), tch->GetName()))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), tch->GetName());
						return CHARACTER_NAME_MAX_LEN;
					}
					else if (MessengerManager::instance().IsBlocked(tch->GetName(), ch->GetName()))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s has blocked you."), tch->GetName());
						return CHARACTER_NAME_MAX_LEN;
					}
#endif

					if (tch->IsBlockMode(BLOCK_MESSENGER_INVITE) == true)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 메신져 추가 거부 상태입니다."));
					}
					else
					{
						MessengerManager::instance().RequestToAdd(ch, tch);
						//MessengerManager::instance().AddToList(ch->GetName(), tch->GetName());
					}
				}
			}
			return CHARACTER_NAME_MAX_LEN;

		case MESSENGER_SUBHEADER_CG_REMOVE:
			{
				if (uiBytes < CHARACTER_NAME_MAX_LEN)
					return -1;

				char char_name[CHARACTER_NAME_MAX_LEN + 1];
				strlcpy(char_name, c_pData, sizeof(char_name));
				MessengerManager::instance().RemoveFromList(ch->GetName(), char_name);
				MessengerManager::instance().RemoveFromList(char_name, ch->GetName()); // @fixme183
			}
			return CHARACTER_NAME_MAX_LEN;

#if defined(__MESSENGER_BLOCK_SYSTEM__)
		case MESSENGER_SUBHEADER_CG_BLOCK_ADD_BY_VID:
		{
			if (uiBytes < sizeof(TPacketCGMessengerAddBlockByVID))
				return -1;

			TPacketCGMessengerAddBlockByVID* p2 = (TPacketCGMessengerAddBlockByVID*)c_pData;
			LPCHARACTER ch_companion = CHARACTER_MANAGER::instance().Find(p2->vid);

			if (!ch_companion)
				return sizeof(TPacketCGMessengerAddBlockByVID);

			if (ch->IsObserverMode())
				return sizeof(TPacketCGMessengerAddBlockByVID);

			LPDESC d = ch_companion->GetDesc();

			if (!d)
				return sizeof(TPacketCGMessengerAddByVID);

			LPCHARACTER pkPartner = ch->GetMarryPartner();
			if (pkPartner)
			{
				if (ch_companion->GetName() == pkPartner->GetName())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot block your spouse."));
					return sizeof(TPacketCGMessengerAddBlockByVID);
				}
			}

			if (MessengerManager::instance().IsBlocked(ch->GetName(), ch_companion->GetName()))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Remove %s from your friends list to continue."), ch_companion->GetName());
				return sizeof(TPacketCGMessengerAddBlockByVID);
			}

			if (MessengerManager::instance().IsBlocked(ch->GetName(), ch_companion->GetName()))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s is already being blocked."), ch_companion->GetName());
				return sizeof(TPacketCGMessengerAddBlockByVID);
			}

			if (ch->GetGMLevel() == GM_PLAYER && ch_companion->GetGMLevel() != GM_PLAYER && !test_server)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot block this player."));
				return sizeof(TPacketCGMessengerAddByVID);
			}

			if (ch->GetDesc() == d)
				return sizeof(TPacketCGMessengerAddBlockByVID);

			MessengerManager::instance().AddToBlockList(ch->GetName(), ch_companion->GetName());
		}
		return sizeof(TPacketCGMessengerAddBlockByVID);

		case MESSENGER_SUBHEADER_CG_BLOCK_ADD_BY_NAME:
		{
			if (uiBytes < CHARACTER_NAME_MAX_LEN)
				return -1;

			char name[CHARACTER_NAME_MAX_LEN + 1];
			strlcpy(name, c_pData, sizeof(name));

			if (gm_get_level(name) != GM_PLAYER && !test_server)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot block this player."));
				return CHARACTER_NAME_MAX_LEN;
			}

			LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(name);
			if (pkChr == ch)
				return CHARACTER_NAME_MAX_LEN;

			LPDESC pkDesc = NULL;
			if (!pkChr)
			{
				CCI* pkCCI = P2P_MANAGER::instance().Find(name);
				if (pkCCI)
					pkDesc = pkCCI->pkDesc;
			}
			else
				pkDesc = pkChr->GetDesc();

			if (!pkDesc)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s is not online."), name);
				return CHARACTER_NAME_MAX_LEN;
			}
			else
			{
				LPCHARACTER pkPartner = ch->GetMarryPartner();
				if (pkPartner)
				{
					if (pkPartner->GetName() == name)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot block your spouse."));
						return CHARACTER_NAME_MAX_LEN;
					}
				}

				if (MessengerManager::instance().IsBlocked(ch->GetName(), name))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Remove %s from your friends list to continue."), name);
					return CHARACTER_NAME_MAX_LEN;
				}

				if (MessengerManager::instance().IsBlocked(ch->GetName(), name))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s is already being blocked."), name);
					return CHARACTER_NAME_MAX_LEN;
				}

				MessengerManager::instance().AddToBlockList(ch->GetName(), name);
			}
		}
		return CHARACTER_NAME_MAX_LEN;

		case MESSENGER_SUBHEADER_CG_BLOCK_REMOVE:
		{
			if (uiBytes < CHARACTER_NAME_MAX_LEN)
				return -1;

			char char_name[CHARACTER_NAME_MAX_LEN + 1];
			strlcpy(char_name, c_pData, sizeof(char_name));

			if (!MessengerManager::instance().IsBlocked(ch->GetName(), char_name))
				return CHARACTER_NAME_MAX_LEN;

			MessengerManager::instance().RemoveFromBlockList(ch->GetName(), char_name);
		}
		return CHARACTER_NAME_MAX_LEN;
#endif

		default:
			sys_err("CInputMain::Messenger : Unknown subheader %d : %s", p->subheader, ch->GetName());
			break;
	}

	return 0;
}

int CInputMain::Shop(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	TPacketCGShop * p = (TPacketCGShop *) data;

	if (uiBytes < sizeof(TPacketCGShop))
		return -1;

	if (test_server)
		sys_log(0, "CInputMain::Shop() ==> SubHeader %d", p->subheader);

	const char * c_pData = data + sizeof(TPacketCGShop);
	uiBytes -= sizeof(TPacketCGShop);

	switch (p->subheader)
	{
		case SHOP_SUBHEADER_CG_END:
			sys_log(1, "INPUT: %s SHOP: END", ch->GetName());
			CShopManager::instance().StopShopping(ch);
			return 0;

		case SHOP_SUBHEADER_CG_BUY:
			{
				if (uiBytes < sizeof(BYTE) + sizeof(BYTE))
					return -1;

				BYTE bPos = *(c_pData + 1);
				sys_log(1, "INPUT: %s SHOP: BUY %d", ch->GetName(), bPos);

				CShopManager::instance().Buy(ch, bPos);
				return (sizeof(BYTE) + sizeof(BYTE));
			}
		case SHOP_SUBHEADER_CG_SELL:
			{
				if (uiBytes < sizeof(WORD))
					return -1;

				WORD Pos = *c_pData;

				sys_log(0, "INPUT: %s SHOP: SELL", ch->GetName());
				CShopManager::instance().Sell(ch, Pos);
				return sizeof(WORD);
			}

		case SHOP_SUBHEADER_CG_SELL2:
			{
				if (uiBytes < sizeof(WORD) + sizeof(DWORD))
					return -1;

				WORD pos = *(c_pData++);
				DWORD count = *(c_pData);

				sys_log(0, "INPUT: %s SHOP: SELL2", ch->GetName());
				CShopManager::instance().Sell(ch, pos, count);
				return sizeof(WORD) + sizeof(DWORD);
			}

		default:
			sys_err("CInputMain::Shop : Unknown subheader %d : %s", p->subheader, ch->GetName());
			break;
	}

	return 0;
}

void CInputMain::OnClick(LPCHARACTER ch, const char * data)
{
	struct command_on_click *	pinfo = (struct command_on_click *) data;
	LPCHARACTER			victim;

	if ((victim = CHARACTER_MANAGER::instance().Find(pinfo->vid)))
		victim->OnClick(ch);
	else if (test_server)
	{
		sys_err("CInputMain::OnClick %s.Click.NOT_EXIST_VID[%d]", ch->GetName(), pinfo->vid);
	}
}

void CInputMain::Exchange(LPCHARACTER ch, const char * data)
{
	struct command_exchange * pinfo = (struct command_exchange *) data;
	LPCHARACTER	to_ch = NULL;

	if (!ch->CanHandleItem())
		return;

	int iPulse = thecore_pulse();

	if ((to_ch = CHARACTER_MANAGER::instance().Find(pinfo->arg1)))
	{
		if (iPulse - to_ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			to_ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래 후 %d초 이내에 창고를 열수 없습니다."), g_nPortalLimitTime);
			return;
		}

		if( true == to_ch->IsDead() )
		{
			return;
		}
	}

	sys_log(0, "CInputMain()::Exchange()  SubHeader %d ", pinfo->sub_header);

	if (iPulse - ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래 후 %d초 이내에 창고를 열수 없습니다."), g_nPortalLimitTime);
		return;
	}

	switch (pinfo->sub_header)
	{
		case EXCHANGE_SUBHEADER_CG_START:	// arg1 == vid of target character
			if (!ch->GetExchange())
			{
				if ((to_ch = CHARACTER_MANAGER::instance().Find(pinfo->arg1)))
				{
					//MONARCH_LIMIT

					if (iPulse - ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("창고를 연후 %d초 이내에는 거래를 할수 없습니다."), g_nPortalLimitTime);

						if (test_server)
							ch->ChatPacket(CHAT_TYPE_INFO, "[TestOnly][Safebox]Pulse %d LoadTime %d PASS %d", iPulse, ch->GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
						return;
					}

					if (iPulse - to_ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
					{
						to_ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("창고를 연후 %d초 이내에는 거래를 할수 없습니다."), g_nPortalLimitTime);

						if (test_server)
							to_ch->ChatPacket(CHAT_TYPE_INFO, "[TestOnly][Safebox]Pulse %d LoadTime %d PASS %d", iPulse, to_ch->GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
						return;
					}

					if (ch->GetGold() >= GOLD_MAX)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("액수가 20억 냥을 초과하여 거래를 할수가 없습니다.."));

						sys_err("[OVERFLOG_GOLD] START (%u) id %u name %s ", ch->GetGold(), ch->GetPlayerID(), ch->GetName());
						return;
					}

					if (to_ch->IsPC())
					{
						if (quest::CQuestManager::instance().GiveItemToPC(ch->GetPlayerID(), to_ch))
						{
							sys_log(0, "Exchange canceled by quest %s %s", ch->GetName(), to_ch->GetName());
							return;
						}
					}

					if (ch->PreventTradeWindow(WND_EXCHANGE, true/*except*/))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("창고,거래창등이 열린 상태에서는 개량을 할수가 없습니다"));
						return;
					}

					ch->ExchangeStart(to_ch);
				}
			}
			break;

		case EXCHANGE_SUBHEADER_CG_ITEM_ADD:	// arg1 == position of item, arg2 == position in exchange window
			if (ch->GetExchange())
			{
				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
#if defined(ITEM_CHECKINOUT_UPDATE)
					ch->GetExchange()->AddItem(pinfo->Pos, pinfo->arg2, pinfo->SelectPosAuto);
#else
					ch->GetExchange()->AddItem(pinfo->Pos, pinfo->arg2);
#endif
			}
			break;

		case EXCHANGE_SUBHEADER_CG_ITEM_DEL:	// arg1 == position of item
			if (ch->GetExchange())
			{
				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->RemoveItem(pinfo->arg1);
			}
			break;

		case EXCHANGE_SUBHEADER_CG_ELK_ADD:	// arg1 == amount of gold
			if (ch->GetExchange())
			{
				int64_t nTotalGold = static_cast<int64_t>(ch->GetExchange()->GetCompany()->GetOwner()->GetGold()) + static_cast<int64_t>(pinfo->arg1);

				if (GOLD_MAX <= nTotalGold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 총금액이 20억 냥을 초과하여 거래를 할수가 없습니다.."));

					sys_err("[OVERFLOW_GOLD] ELK_ADD (%u) id %u name %s ",
							ch->GetExchange()->GetCompany()->GetOwner()->GetGold(),
							ch->GetExchange()->GetCompany()->GetOwner()->GetPlayerID(),
						   	ch->GetExchange()->GetCompany()->GetOwner()->GetName());

					return;
				}

				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->AddGold(pinfo->arg1);
			}
			break;

		case EXCHANGE_SUBHEADER_CG_ACCEPT:	// arg1 == not used
			if (ch->GetExchange())
			{
				sys_log(0, "CInputMain()::Exchange() ==> ACCEPT ");
				ch->GetExchange()->Accept(true);
			}

			break;

		case EXCHANGE_SUBHEADER_CG_CANCEL:	// arg1 == not used
			if (ch->GetExchange())
				ch->GetExchange()->Cancel();
			break;
	}
}

void CInputMain::Position(LPCHARACTER ch, const char * data)
{
	struct command_position * pinfo = (struct command_position *) data;

	switch (pinfo->position)
	{
		case POSITION_GENERAL:
			ch->Standup();
			break;

		case POSITION_SITTING_CHAIR:
			ch->Sitdown(0);
			break;

		case POSITION_SITTING_GROUND:
			ch->Sitdown(1);
			break;
	}
}

static const int ComboSequenceBySkillLevel[3][8] =
{
	// 0   1   2   3   4   5   6   7
	{ 14, 15, 16, 17,  0,  0,  0,  0 },
	{ 14, 15, 16, 18, 20,  0,  0,  0 },
	{ 14, 15, 16, 18, 19, 17,  0,  0 },
};

#define COMBO_HACK_ALLOWABLE_MS	100

bool CheckComboHack(LPCHARACTER ch, BYTE bArg, DWORD dwTime, bool CheckSpeedHack)
{
	if(!gHackCheckEnable) return false;
	if(gHackCheckEnable) return false;
	//	if (IsStun() || IsDead()) return false;

	if (ch->IsStun() || ch->IsDead())
		return false;
	int ComboInterval = dwTime - ch->GetLastComboTime();
	int HackScalar = 0;
#if 0
	sys_log(0, "COMBO: %s arg:%u seq:%u delta:%d checkspeedhack:%d",
			ch->GetName(), bArg, ch->GetComboSequence(), ComboInterval - ch->GetValidComboInterval(), CheckSpeedHack);
#endif

	if (bArg == 14)
	{
		if (CheckSpeedHack && ComboInterval > 0 && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
		{
			//HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 300;

			//sys_log(0, "COMBO_HACK: 2 %s arg:%u interval:%d valid:%u atkspd:%u riding:%s",
			//		ch->GetName(),
			//		bArg,
			//		ComboInterval,
			//		ch->GetValidComboInterval(),
			//		ch->GetPoint(POINT_ATT_SPEED),
			//	    ch->IsRiding() ? "yes" : "no");
		}

		ch->SetComboSequence(1);
		ch->SetValidComboInterval((int) (ani_combo_speed(ch, 1) / (ch->GetPoint(POINT_ATT_SPEED) / 100.f)));
		ch->SetLastComboTime(dwTime);
	}
	else if (bArg > 14 && bArg < 22)
	{
		int idx = MIN(2, ch->GetComboIndex());

		if (ch->GetComboSequence() > 5)
		{
			HackScalar = 1;
			ch->SetValidComboInterval(300);
			sys_log(0, "COMBO_HACK: 5 %s combo_seq:%d", ch->GetName(), ch->GetComboSequence());
		}

		else if (bArg == 21 &&
				 idx == 2 &&
				 ch->GetComboSequence() == 5 &&
				 ch->GetJob() == JOB_ASSASSIN &&
				 ch->GetWear(WEAR_WEAPON) &&
				 ch->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
			ch->SetValidComboInterval(300);
#ifdef ENABLE_WOLFMAN_CHARACTER
		else if (bArg == 21 && idx == 2 && ch->GetComboSequence() == 5 && ch->GetJob() == JOB_WOLFMAN && ch->GetWear(WEAR_WEAPON) && ch->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_CLAW)
			ch->SetValidComboInterval(300);
#endif
		else if (ComboSequenceBySkillLevel[idx][ch->GetComboSequence()] != bArg)
		{
			HackScalar = 1;
			ch->SetValidComboInterval(300);

			sys_log(0, "COMBO_HACK: 3 %s arg:%u valid:%u combo_idx:%d combo_seq:%d",
					ch->GetName(),
					bArg,
					ComboSequenceBySkillLevel[idx][ch->GetComboSequence()],
					idx,
					ch->GetComboSequence());
		}
		else
		{
			if (CheckSpeedHack && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
			{
				HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 100;

				sys_log(0, "COMBO_HACK: 2 %s arg:%u interval:%d valid:%u atkspd:%u riding:%s",
						ch->GetName(),
						bArg,
						ComboInterval,
						ch->GetValidComboInterval(),
						ch->GetPoint(POINT_ATT_SPEED),
						ch->IsRiding() ? "yes" : "no");
			}

			//if (ch->IsHorseRiding())
			if (ch->IsRiding())
				ch->SetComboSequence(ch->GetComboSequence() == 1 ? 2 : 1);
			else
				ch->SetComboSequence(ch->GetComboSequence() + 1);

			ch->SetValidComboInterval((int) (ani_combo_speed(ch, bArg - 13) / (ch->GetPoint(POINT_ATT_SPEED) / 100.f)));
			ch->SetLastComboTime(dwTime);
		}
	}
	else if (bArg == 13)
	{
		if (CheckSpeedHack && ComboInterval > 0 && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
		{
			//HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 100;

			//sys_log(0, "COMBO_HACK: 6 %s arg:%u interval:%d valid:%u atkspd:%u",
			//		ch->GetName(),
			//		bArg,
			//		ComboInterval,
			//		ch->GetValidComboInterval(),
			//		ch->GetPoint(POINT_ATT_SPEED));
		}

		if (ch->GetRaceNum() >= MAIN_RACE_MAX_NUM)
		{
			// POLYMORPH_BUG_FIX

			// DELETEME

			float normalAttackDuration = CMotionManager::instance().GetNormalAttackDuration(ch->GetRaceNum());
			int k = (int) (normalAttackDuration / ((float) ch->GetPoint(POINT_ATT_SPEED) / 100.f) * 900.f);
			ch->SetValidComboInterval(k);
			ch->SetLastComboTime(dwTime);
			// END_OF_POLYMORPH_BUG_FIX
		}
		else
		{
			//if (ch->GetDesc()->DelayedDisconnect(number(2, 9)))
			//{
			//	LogManager::instance().HackLog("Hacker", ch);
			//	sys_log(0, "HACKER: %s arg %u", ch->GetName(), bArg);
			//}

			// by rtsummit
		}
	}
	else
	{
		if (ch->GetDesc()->DelayedDisconnect(number(2, 9)))
		{
			LogManager::instance().HackLog("Hacker", ch);
			sys_log(0, "HACKER: %s arg %u", ch->GetName(), bArg);
		}

		HackScalar = 10;
		ch->SetValidComboInterval(300);
	}

	if (HackScalar)
	{
		if (get_dword_time() - ch->GetLastMountTime() > 1500)
			ch->IncreaseComboHackCount(1 + HackScalar);

		ch->SkipComboAttackByTime(ch->GetValidComboInterval());
	}

	return HackScalar;
}

void CInputMain::Move(LPCHARACTER ch, const char * data)
{
	if (!ch->CanMove())
		return;

	struct command_move * pinfo = (struct command_move *) data;

	if (pinfo->bFunc >= FUNC_MAX_NUM && !(pinfo->bFunc & 0x80))
	{
		sys_err("invalid move type: %s", ch->GetName());
		return;
	}

	//enum EMoveFuncType
	//{
	//	FUNC_WAIT,
	//	FUNC_MOVE,
	//	FUNC_ATTACK,
	//	FUNC_COMBO,
	//	FUNC_MOB_SKILL,
	//	_FUNC_SKILL,
	//	FUNC_MAX_NUM,
	//	FUNC_SKILL = 0x80,
	//};

	{
		// #define ENABLE_TP_SPEED_CHECK
		#ifdef ENABLE_TP_SPEED_CHECK
		const float fDist = DISTANCE_SQRT((ch->GetX() - pinfo->lX) / 100, (ch->GetY() - pinfo->lY) / 100);
		if (((false == ch->IsRiding() && fDist > 25) || fDist > 60) && OXEVENT_MAP_INDEX != ch->GetMapIndex()) // @fixme106 (changed 40 to 60)
		{
			#ifdef ENABLE_HACK_TELEPORT_LOG // @warme006
			{
				const PIXEL_POSITION & warpPos = ch->GetWarpPosition();

				if (warpPos.x == 0 && warpPos.y == 0)
					LogManager::instance().HackLog("Teleport", ch);
			}
			#endif
			sys_log(0, "MOVE: %s trying to move too far (dist: %.1fm) Riding(%d)", ch->GetName(), fDist, ch->IsRiding());

			ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}
		#endif
		#ifdef ENABLE_CHECK_GHOSTMODE
		if (ch->IsPC() && ch->IsDead())
		{
			sys_log(0, "MOVE: %s trying to move as dead", ch->GetName());

			ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}
		#endif

		DWORD dwCurTime = get_dword_time();

		bool CheckSpeedHack = (false == ch->GetDesc()->IsHandshaking() && dwCurTime - ch->GetDesc()->GetClientTime() > 7000);

		if (CheckSpeedHack)
		{
			int iDelta = (int) (pinfo->dwTime - ch->GetDesc()->GetClientTime());
			int iServerDelta = (int) (dwCurTime - ch->GetDesc()->GetClientTime());

			iDelta = (int) (dwCurTime - pinfo->dwTime);

			if (iDelta >= 30000)
			{
				sys_log(0, "SPEEDHACK: slow timer name %s delta %d", ch->GetName(), iDelta);
				ch->GetDesc()->DelayedDisconnect(3);
			}

			else if (iDelta < -(iServerDelta / 50))
			{
				sys_log(0, "SPEEDHACK: DETECTED! %s (delta %d %d)", ch->GetName(), iDelta, iServerDelta);
				ch->GetDesc()->DelayedDisconnect(3);
			}
		}

		if (pinfo->bFunc == FUNC_COMBO && g_bCheckMultiHack)
		{
			CheckComboHack(ch, pinfo->bArg, pinfo->dwTime, CheckSpeedHack);
		}
	}

	if (pinfo->bFunc == FUNC_MOVE)
	{
		if (ch->GetLimitPoint(POINT_MOV_SPEED) == 0)
			return;

		ch->SetRotation(pinfo->bRot * 5);
		ch->ResetStopTime();				// ""

		ch->Goto(pinfo->lX, pinfo->lY);
	}
	else
	{
		if (pinfo->bFunc == FUNC_ATTACK || pinfo->bFunc == FUNC_COMBO)
			ch->OnMove(true);
		else if (pinfo->bFunc & FUNC_SKILL)
		{
			const int MASK_SKILL_MOTION = 0x7F;
			unsigned int motion = pinfo->bFunc & MASK_SKILL_MOTION;

			if (!ch->IsUsableSkillMotion(motion))
			{
				const char* name = ch->GetName();
				unsigned int job = ch->GetJob();
				unsigned int group = ch->GetSkillGroup();

				char szBuf[256];
				snprintf(szBuf, sizeof(szBuf), "SKILL_HACK: name=%s, job=%d, group=%d, motion=%d", name, job, group, motion);
				LogManager::instance().HackLog(szBuf, ch->GetDesc()->GetAccountTable().login, ch->GetName(), ch->GetDesc()->GetHostName());
				sys_log(0, "%s", szBuf);

				if (test_server)
				{
					ch->GetDesc()->DelayedDisconnect(number(2, 8));
					ch->ChatPacket(CHAT_TYPE_INFO, szBuf);
				}
				else
				{
					ch->GetDesc()->DelayedDisconnect(number(150, 500));
				}
			}

			ch->OnMove();
		}

		ch->SetRotation(pinfo->bRot * 5);
		ch->ResetStopTime();				// ""

		ch->Move(pinfo->lX, pinfo->lY);
		ch->Stop();
		ch->StopStaminaConsume();
	}

	TPacketGCMove pack;

	pack.bHeader      = HEADER_GC_MOVE;
	pack.bFunc        = pinfo->bFunc;
	pack.bArg         = pinfo->bArg;
	pack.bRot         = pinfo->bRot;
	pack.dwVID        = ch->GetVID();
	pack.lX           = pinfo->lX;
	pack.lY           = pinfo->lY;
	pack.dwTime       = pinfo->dwTime;
	pack.dwDuration   = (pinfo->bFunc == FUNC_MOVE) ? ch->GetCurrentMoveDuration() : 0;

	ch->PacketAround(&pack, sizeof(TPacketGCMove), ch);

	/*
	sys_log(0,
			"MOVE: %s Func:%u Arg:%u Pos:%dx%d Time:%u Dist:%.1f",
			ch->GetName(),
			pinfo->bFunc,
			pinfo->bArg,
			pinfo->lX / 100,
			pinfo->lY / 100,
			pinfo->dwTime,
			fDist);
	*/
}

void CInputMain::Attack(LPCHARACTER ch, const BYTE header, const char* data)
{
	if (NULL == ch)
		return;

	struct type_identifier
	{
		BYTE header;
		BYTE type;
	};

	const struct type_identifier* const type = reinterpret_cast<const struct type_identifier*>(data);

	if (type->type > 0)
	{
		if (false == ch->CanUseSkill(type->type))
		{
			return;
		}

		switch (type->type)
		{
			case SKILL_GEOMPUNG:
			case SKILL_SANGONG:
			case SKILL_YEONSA:
			case SKILL_KWANKYEOK:
			case SKILL_HWAJO:
			case SKILL_GIGUNG:
			case SKILL_PABEOB:
			case SKILL_MARYUNG:
			case SKILL_TUSOK:
			case SKILL_MAHWAN:
			case SKILL_BIPABU:
			case SKILL_NOEJEON:
			case SKILL_CHAIN:
			case SKILL_HORSE_WILDATTACK_RANGE:
				if (HEADER_CG_SHOOT != type->header)
				{
					if (test_server)
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Attack :name[%s] Vnum[%d] can't use skill by attack(warning)"), type->type);
					return;
				}
				break;
		}
	}

	switch (header)
	{
		case HEADER_CG_ATTACK:
			{
				if (NULL == ch->GetDesc())
					return;

				const TPacketCGAttack* const packMelee = reinterpret_cast<const TPacketCGAttack*>(data);

				ch->GetDesc()->AssembleCRCMagicCube(packMelee->bCRCMagicCubeProcPiece, packMelee->bCRCMagicCubeFilePiece);

				LPCHARACTER	victim = CHARACTER_MANAGER::instance().Find(packMelee->dwVID);

				if (NULL == victim || ch == victim)
					return;

				switch (victim->GetCharType())
				{
					case CHAR_TYPE_NPC:
					case CHAR_TYPE_WARP:
					case CHAR_TYPE_PET:
					case CHAR_TYPE_HORSE:
					case CHAR_TYPE_GOTO:
						return;
				}

				// If the attack type is Melee
				if (packMelee->bType == 0)
				{
					// Get the difference in meter between the attacker and the victim
					// A value of 1.0f is considered a point blank hit
					const float fHittingDistance = DISTANCE_SQRT((victim->GetX() - ch->GetX()) / 100, (victim->GetY() - ch->GetY()) / 100);
					// The maximum possible distance to avoid false positive is 7.0m
					// Two hands or one handed sword with Aura doesn't allow this range.
				  	// However, some rare instances showed that cranking it up to 9.0 would avoid false positive even further.
					static const float fMaximumHittingDistance = 14.0f;
					// We got one - Abort and then Log / Disconnect / Ban / Whatever?
					if (fHittingDistance > fMaximumHittingDistance)
					{
						// INSERT YOUR ACTION HERE
						return;
					}
				}

				if (packMelee->bType > 0)
				{
					if (false == ch->CheckSkillHitCount(packMelee->bType, victim->GetVID()))
					{
						return;
					}
				}

				ch->Attack(victim, packMelee->bType);
			}
			break;

		case HEADER_CG_SHOOT:
			{
				const TPacketCGShoot* const packShoot = reinterpret_cast<const TPacketCGShoot*>(data);

				ch->Shoot(packShoot->bType);
			}
			break;
	}
}

#include <chrono>

int CInputMain::SyncPosition(LPCHARACTER ch, const char * c_pcData, size_t uiBytes)
{
    const TPacketCGSyncPosition* pinfo = reinterpret_cast<const TPacketCGSyncPosition*>( c_pcData );

    if (uiBytes < pinfo->wSize)
        return -1;

    int iExtraLen = pinfo->wSize - sizeof(TPacketCGSyncPosition);

    if (iExtraLen < 0)
    {
        sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->wSize, uiBytes);
        ch->GetDesc()->SetPhase(PHASE_CLOSE);
        return -1;
    }

    if (0 != (iExtraLen % sizeof(TPacketCGSyncPositionElement)))
    {
        sys_err("invalid packet length %d (name: %s)", pinfo->wSize, ch->GetName());
        return iExtraLen;
    }

    int iCount = iExtraLen / sizeof(TPacketCGSyncPositionElement);

    if (iCount <= 0)
        return iExtraLen;

    static const int nCountLimit = 16;

    if (iCount > nCountLimit)
    {
        sys_err("Too many SyncPosition Count(%d) from Name(%s)", iCount, ch->GetName());
        iCount = nCountLimit;
    }

    TEMP_BUFFER tbuf;
    LPBUFFER lpBuf = tbuf.getptr();

    TPacketGCSyncPosition * pHeader = (TPacketGCSyncPosition *) buffer_write_peek(lpBuf);
    buffer_write_proceed(lpBuf, sizeof(TPacketGCSyncPosition));

    const TPacketCGSyncPositionElement* e = reinterpret_cast<const TPacketCGSyncPositionElement*>(c_pcData + sizeof(TPacketCGSyncPosition));

    auto now = std::chrono::steady_clock::now();

    for (int i = 0; i < iCount; ++i, ++e)
    {
        LPCHARACTER victim = CHARACTER_MANAGER::instance().Find(e->dwVID);

        if (!victim)
            continue;

        switch (victim->GetCharType())
        {
            case CHAR_TYPE_NPC:
            case CHAR_TYPE_WARP:
            case CHAR_TYPE_GOTO:
            case CHAR_TYPE_HORSE:
            case CHAR_TYPE_PET:
                continue;
        }

        if (!victim->SetSyncOwner(ch))
            continue;

        auto time_since_last_sync = std::chrono::duration_cast<std::chrono::milliseconds>(now - victim->GetLastSyncTime()).count();
		static const long g_lValidSyncInterval = 100; // 100ms

        if (time_since_last_sync < g_lValidSyncInterval)
        {
            if (victim->GetSyncHackCount() < g_iSyncHackLimitCount)
            {
                victim->SetSyncHackCount(victim->GetSyncHackCount() + 1);
                continue;
            }
            else
            {
                sys_err("Too often SyncPosition Interval(%lldms)(%s) from Name(%s)", time_since_last_sync, victim->GetName(), ch->GetName());
                ch->GetDesc()->SetPhase(PHASE_CLOSE);
                return -1;
            }
        }
        else
        {
            victim->SetSyncHackCount(0);
            victim->SetLastSyncTime(now);
        }

		const float fDist = DISTANCE_SQRT((victim->GetX() - e->lX) / 100, (victim->GetY() - e->lY) / 100);
		if (fDist > 75.0f)
		{
			LogManager::instance().HackLog("SYNC_POSITION_HACK", ch);

			sys_err("Too far SyncPosition Distance(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
				fDist, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
				e->lX, e->lY);

			ch->GetDesc()->SetPhase(PHASE_CLOSE);
			return -1;
		}

		victim->SetLastSyncTime(now);
		victim->Sync(e->lX, e->lY);
        buffer_write(lpBuf, e, sizeof(TPacketCGSyncPositionElement));
    }

    if (buffer_size(lpBuf) != sizeof(TPacketGCSyncPosition))
    {
        pHeader->bHeader = HEADER_GC_SYNC_POSITION;
        pHeader->wSize = buffer_size(lpBuf);

        ch->PacketAround(buffer_read_peek(lpBuf), buffer_size(lpBuf), ch);
    }

    return iExtraLen;
}

void CInputMain::FlyTarget(LPCHARACTER ch, const char * pcData, BYTE bHeader)
{
	TPacketCGFlyTargeting * p = (TPacketCGFlyTargeting *) pcData;
	ch->FlyTarget(p->dwTargetVID, p->x, p->y, bHeader);
}

void CInputMain::UseSkill(LPCHARACTER ch, const char * pcData)
{
	TPacketCGUseSkill * p = (TPacketCGUseSkill *) pcData;
	ch->UseSkill(p->dwVnum, CHARACTER_MANAGER::instance().Find(p->dwVID));
}

void CInputMain::ScriptButton(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptButton * p = (TPacketCGScriptButton *) c_pData;
	sys_log(0, "QUEST ScriptButton pid %d idx %u", ch->GetPlayerID(), p->idx);

	quest::PC* pc = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	if (pc && pc->IsConfirmWait())
	{
		quest::CQuestManager::instance().Confirm(ch->GetPlayerID(), quest::CONFIRM_TIMEOUT);
	}
	else if (p->idx & 0x80000000)
	{
		quest::CQuestManager::Instance().QuestInfo(ch->GetPlayerID(), p->idx & 0x7fffffff);
	}
	else
	{
		quest::CQuestManager::Instance().QuestButton(ch->GetPlayerID(), p->idx);
	}
}

void CInputMain::ScriptAnswer(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptAnswer * p = (TPacketCGScriptAnswer *) c_pData;
	sys_log(0, "QUEST ScriptAnswer pid %d answer %d", ch->GetPlayerID(), p->answer);

	if (p->answer > 250)
	{
		quest::CQuestManager::Instance().Resume(ch->GetPlayerID());
	}
	else
	{
		quest::CQuestManager::Instance().Select(ch->GetPlayerID(),  p->answer);
	}
}

// SCRIPT_SELECT_ITEM
void CInputMain::ScriptSelectItem(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptSelectItem* p = (TPacketCGScriptSelectItem*) c_pData;
	sys_log(0, "QUEST ScriptSelectItem pid %d answer %d", ch->GetPlayerID(), p->selection);
	quest::CQuestManager::Instance().SelectItem(ch->GetPlayerID(), p->selection);
}
// END_OF_SCRIPT_SELECT_ITEM

void CInputMain::QuestInputString(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGQuestInputString * p = (TPacketCGQuestInputString*) c_pData;

	char msg[65];
	strlcpy(msg, p->msg, sizeof(msg));
	sys_log(0, "QUEST InputString pid %u msg %s", ch->GetPlayerID(), msg);

	quest::CQuestManager::Instance().Input(ch->GetPlayerID(), msg);
}

void CInputMain::QuestConfirm(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGQuestConfirm* p = (TPacketCGQuestConfirm*) c_pData;
	LPCHARACTER ch_wait = CHARACTER_MANAGER::instance().FindByPID(p->requestPID);
	if (p->answer)
		p->answer = quest::CONFIRM_YES;
	sys_log(0, "QuestConfirm from %s pid %u name %s answer %d", ch->GetName(), p->requestPID, (ch_wait)?ch_wait->GetName():"", p->answer);
	if (ch_wait)
	{
		quest::CQuestManager::Instance().Confirm(ch_wait->GetPlayerID(), (quest::EQuestConfirmType) p->answer, ch->GetPlayerID());
	}
}

void CInputMain::Target(LPCHARACTER ch, const char * pcData)
{
	TPacketCGTarget * p = (TPacketCGTarget *) pcData;

	building::LPOBJECT pkObj = building::CManager::instance().FindObjectByVID(p->dwVID);

	if (pkObj)
	{
		TPacketGCTarget pckTarget{};
		pckTarget.header = HEADER_GC_TARGET;
		pckTarget.dwVID = p->dwVID;
#if defined(__SHIP_DEFENSE__)
		pckTarget.bAlliance = false;
		pckTarget.iAllianceMinHP = 0;
		pckTarget.iAllianceMaxHP = 0;
#endif
		ch->GetDesc()->Packet(&pckTarget, sizeof(TPacketGCTarget));
	}
	else
		ch->SetTarget(CHARACTER_MANAGER::instance().Find(p->dwVID));
}

void CInputMain::Warp(LPCHARACTER ch, const char * pcData)
{
	ch->WarpEnd();
}

void CInputMain::SafeboxCheckin(LPCHARACTER ch, const char * c_pData)
{
    if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::SafeboxMove, std::chrono::milliseconds(500))) {
        ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Remaining time: %.2f"), PULSEMANAGER_CLOCK_TO_SEC2(ch->GetPlayerID(), ePulse::SafeboxMove));
        return;
    }

	if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
		return;

	TPacketCGSafeboxCheckin * p = (TPacketCGSafeboxCheckin *) c_pData;

	if (!ch->CanHandleItem())
		return;

	CSafebox * pkSafebox = ch->GetSafebox();
	LPITEM pkItem = ch->GetItem(p->ItemPos);

	if (!pkSafebox || !pkItem)
		return;

#if defined(ITEM_CHECKINOUT_UPDATE)
	if (p->SelectPosAuto)
	{
		int AutoPos = pkSafebox->GetEmptySafebox(pkItem->GetSize());
		if (AutoPos == -1)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<SAFEBOX> You don't have enough space."));
			return;
		}
		p->bSafePos = AutoPos;
	}
#endif

	if (pkItem->IsEquipped() && !ch->CanUnequipNow(pkItem))
		return;

	if ((pkItem->IsCostumeMountItem() && pkItem->IsEquipped()) || (pkItem->IsSkinMount() && pkItem->IsEquipped()))
		return;

	if (p->ItemPos.IsSwitchbotPosition())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item can not be moved here (1)"));
		return;
	}

	if (pkItem->GetCell() >= ch->Inventory_Size() && IS_SET(pkItem->GetFlag(), ITEM_FLAG_IRREMOVABLE))
	{
	    ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
	    return;
	}

	if (!pkSafebox->IsEmpty(p->bSafePos, pkItem->GetSize()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 옮길 수 없는 위치입니다."));
		return;
	}

	if (pkItem->GetVnum() == UNIQUE_ITEM_SAFEBOX_EXPAND)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 이 아이템은 넣을 수 없습니다."));
		return;
	}

	if( IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_SAFEBOX) || pkItem->GetSocket(4) == 99)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 이 아이템은 넣을 수 없습니다."));
		return;
	}

	if ((pkItem->GetType() == ITEM_BUFFI && pkItem->GetSubType() == BUFFI_SCROLL) && pkItem->GetSocket(1) == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Deactivate the Buffi Seal first."));
		return;
	}

	if ((pkItem->GetVnum() >= 51010 && pkItem->GetVnum() <= 51030) && pkItem->GetSocket(0) == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Deactivate the Energy Crystal first."));
		return;
	}

	if (true == pkItem->isLocked())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 이 아이템은 넣을 수 없습니다."));
		return;
	}

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (pkItem->IsEquipped())
	{
		int iWearCell = pkItem->FindEquipCell(ch);
		if (iWearCell == WEAR_WEAPON)
		{
			LPITEM costumeWeapon = ch->GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !ch->UnequipItem(costumeWeapon))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot unequip the costume weapon. Not enough space."));
				return;
			}
		}
	}
#endif

	// @fixme140 BEGIN
	if (ITEM_BELT == pkItem->GetType() && CBeltInventoryHelper::IsExistItemInBeltInventory(ch))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("벨트 인벤토리에 아이템이 존재하면 해제할 수 없습니다."));
		return;
	}
	// @fixme140 END

	pkItem->RemoveFromCharacter();
	if (!pkItem->IsDragonSoul())
		ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, p->ItemPos.cell, 255);
	pkSafebox->Add(p->bSafePos, pkItem);

	char szHint[128];
	snprintf(szHint, sizeof(szHint), "%s %u", pkItem->GetName(), pkItem->GetCount());
	LogManager::instance().ItemLog(ch, pkItem, "SAFEBOX_PUT", szHint);
}

void CInputMain::SafeboxCheckout(LPCHARACTER ch, const char * c_pData, bool bMall)
{
    if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::SafeboxMove, std::chrono::milliseconds(500))) {
        ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Remaining time: %.2f"), PULSEMANAGER_CLOCK_TO_SEC2(ch->GetPlayerID(), ePulse::SafeboxMove));
        return;
    }

	TPacketCGSafeboxCheckout * p = (TPacketCGSafeboxCheckout *) c_pData;

	if (!ch->CanHandleItem())
		return;

	CSafebox * pkSafebox;

	if (bMall)
		pkSafebox = ch->GetMall();
	else
		pkSafebox = ch->GetSafebox();

	if (!pkSafebox)
		return;

	LPITEM pkItem = pkSafebox->Get(p->bSafePos);

	if (!pkItem)
		return;

#if defined(ITEM_CHECKINOUT_UPDATE)
	if (p->SelectPosAuto)
	{
		int AutoPos = -1;

		if (pkItem->IsDragonSoul())
			AutoPos = ch->GetEmptyDragonSoulInventory(pkItem);
	#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		else if (pkItem->IsSkillBook())
			AutoPos = ch->GetEmptySkillBookInventory(pkItem->GetSize());
		else if (pkItem->IsUpgradeItem())
			AutoPos = ch->GetEmptyUpgradeItemsInventory(pkItem->GetSize());
		else if (pkItem->IsStone())
			AutoPos = ch->GetEmptyStoneInventory(pkItem->GetSize());
		else if (pkItem->IsGiftBox())
			AutoPos = ch->GetEmptyGiftBoxInventory(pkItem->GetSize());
		else if (pkItem->IsSwitch())
			AutoPos = ch->GetEmptySwitchInventory(pkItem->GetSize());
		else if (pkItem->IsCostume())
			AutoPos = ch->GetEmptyCostumeInventory(pkItem->GetSize());
	#endif
		else
			AutoPos = ch->GetEmptyInventory(pkItem->GetSize());

		if (AutoPos == -1)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough space."));
			return;
		}
		p->ItemPos = TItemPos(pkItem->IsDragonSoul() ? DRAGON_SOUL_INVENTORY : INVENTORY, AutoPos);
	}
#endif

	if (!ch->IsEmptyItemGrid(p->ItemPos, pkItem->GetSize()))
		return;

	if (pkItem->IsDragonSoul())
	{
		if (bMall)
		{
			DSManager::instance().DragonSoulItemInitialize(pkItem);
		}

		if (DRAGON_SOUL_INVENTORY != p->ItemPos.window_type)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 옮길 수 없는 위치입니다."));
			return;
		}

		TItemPos DestPos = p->ItemPos;
		if (!DSManager::instance().IsValidCellForThisItem(pkItem, DestPos))
		{
			int iCell = ch->GetEmptyDragonSoulInventory(pkItem);
			if (iCell < 0)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 옮길 수 없는 위치입니다."));
				return ;
			}
			DestPos = TItemPos (DRAGON_SOUL_INVENTORY, iCell);
		}

		pkSafebox->Remove(p->bSafePos);
		pkItem->AddToCharacter(ch, DestPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}
	else
	{
		if (DRAGON_SOUL_INVENTORY == p->ItemPos.window_type)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 옮길 수 없는 위치입니다."));
			return;
		}
		// @fixme119
		if (p->ItemPos.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(pkItem))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 아이템은 벨트 인벤토리로 옮길 수 없습니다."));
			return;
		}

#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		if (p->ItemPos.IsSkillBookInventoryPosition() && !pkItem->IsSkillBook())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
			return;
		}
		
		if (p->ItemPos.IsUpgradeItemsInventoryPosition() && !pkItem->IsUpgradeItem())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
			return;
		}
		
		if (p->ItemPos.IsStoneInventoryPosition() && !pkItem->IsStone())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
			return;
		}
		
		if (p->ItemPos.IsGiftBoxInventoryPosition() && !pkItem->IsGiftBox())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
			return;
		}
		if (p->ItemPos.IsSwitchInventoryPosition() && !pkItem->IsSwitch())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
			return;
		}
		
		if (p->ItemPos.IsCostumeInventoryPosition() && !pkItem->IsCostume())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
			return;
		}
#endif

		pkSafebox->Remove(p->bSafePos);
		pkItem->AddToCharacter(ch, p->ItemPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}

	DWORD dwID = pkItem->GetID();
	db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_FLUSH, 0, sizeof(DWORD));
	db_clientdesc->Packet(&dwID, sizeof(DWORD));

	char szHint[128];
	snprintf(szHint, sizeof(szHint), "%s %u", pkItem->GetName(), pkItem->GetCount());
	if (bMall)
		LogManager::instance().ItemLog(ch, pkItem, "MALL_GET", szHint);
	else
		LogManager::instance().ItemLog(ch, pkItem, "SAFEBOX_GET", szHint);
}

void CInputMain::SafeboxItemMove(LPCHARACTER ch, const char * data)
{
	struct command_item_move * pinfo = (struct command_item_move *) data;

	if (!ch->CanHandleItem())
		return;

	if (!ch->GetSafebox())
		return;

	ch->GetSafebox()->MoveItem(pinfo->Cell.cell, pinfo->CellTo.cell, pinfo->count);
}

// PARTY_JOIN_BUG_FIX
void CInputMain::PartyInvite(LPCHARACTER ch, const char * c_pData)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	TPacketCGPartyInvite* p = (TPacketCGPartyInvite*) c_pData;

	LPCHARACTER pInvitee = CHARACTER_MANAGER::instance().Find(p->vid);

	if (!pInvitee || !ch->GetDesc() || !pInvitee->GetDesc())
	{
		sys_err("PARTY Cannot find invited character");
		return;
	}

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	if (MessengerManager::instance().IsBlocked(ch->GetName(), pInvitee->GetName()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), pInvitee->GetName());
		return;
	}
	else if (MessengerManager::instance().IsBlocked(pInvitee->GetName(), ch->GetName()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s has blocked you."), pInvitee->GetName());
		return;
	}
#endif

	ch->PartyInvite(pInvitee);
}

void CInputMain::PartyInviteAnswer(LPCHARACTER ch, const char * c_pData)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	TPacketCGPartyInviteAnswer * p = (TPacketCGPartyInviteAnswer*) c_pData;

	LPCHARACTER pInviter = CHARACTER_MANAGER::instance().Find(p->leader_vid);

	if (!pInviter)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티요청을 한 캐릭터를 찾을수 없습니다."));
	// @fixme301 BEGIN
	else if (!pInviter->GetParty() && pInviter->GetMapIndex() >= 10000)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Party> The party leader is inside a dungeon."));
	else if (pInviter->GetParty() && pInviter->GetParty()->IsPartyInAnyDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Party> The party can't invite people if members are inside a dungeon."));
		pInviter->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Party> The party can't invite people if members are inside a dungeon."));
	}
	// @fixme301 END
	else if (!p->accept)
		pInviter->PartyInviteDeny(ch->GetPlayerID());
	else
		pInviter->PartyInviteAccept(ch);
}
// END_OF_PARTY_JOIN_BUG_FIX

void CInputMain::PartySetState(LPCHARACTER ch, const char* c_pData)
{
	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
		return;
	}

	TPacketCGPartySetState* p = (TPacketCGPartySetState*) c_pData;

	if (!ch->GetParty())
		return;

	if (ch->GetParty()->GetLeaderPID() != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 리더만 변경할 수 있습니다."));
		return;
	}

	if (!ch->GetParty()->IsMember(p->pid))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 상태를 변경하려는 사람이 파티원이 아닙니다."));
		return;
	}

	DWORD pid = p->pid;
	sys_log(0, "PARTY SetRole pid %d to role %d state %s", pid, p->byRole, p->flag ? "on" : "off");

	switch (p->byRole)
	{
		case PARTY_ROLE_NORMAL:
			break;

		case PARTY_ROLE_ATTACKER:
		case PARTY_ROLE_TANKER:
		case PARTY_ROLE_BUFFER:
		case PARTY_ROLE_SKILL_MASTER:
		case PARTY_ROLE_HASTE:
		case PARTY_ROLE_DEFENDER:
			if (ch->GetParty()->SetRole(pid, p->byRole, p->flag))
			{
				TPacketPartyStateChange pack;
				pack.dwLeaderPID = ch->GetPlayerID();
				pack.dwPID = p->pid;
				pack.bRole = p->byRole;
				pack.bFlag = p->flag;
				db_clientdesc->DBPacket(HEADER_GD_PARTY_STATE_CHANGE, 0, &pack, sizeof(pack));
			}

			break;

		default:
			sys_err("wrong byRole in PartySetState Packet name %s state %d", ch->GetName(), p->byRole);
			break;
	}
}

void CInputMain::PartyRemove(LPCHARACTER ch, const char* c_pData)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던전 안에서는 파티에서 추방할 수 없습니다."));
		return;
	}

	TPacketCGPartyRemove* p = (TPacketCGPartyRemove*) c_pData;

	if (!ch->GetParty())
		return;

	LPPARTY pParty = ch->GetParty();
	if (pParty->GetLeaderPID() == ch->GetPlayerID())
	{
		if (ch->GetDungeon())
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던젼내에서는 파티원을 추방할 수 없습니다."));
		else
		{
			if (pParty->IsPartyInAnyDungeon()) // @fixme301
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던젼내에서는 파티를 나갈 수 없습니다."));
				return;
			}

			// leader can remove any member
			if (p->pid == ch->GetPlayerID() || pParty->GetMemberCount() == 2)
			{
				// party disband
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				LPCHARACTER B = CHARACTER_MANAGER::instance().FindByPID(p->pid);
				if (B)
				{
					//pParty->SendPartyRemoveOneToAll(B);
					B->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티에서 추방당하셨습니다."));
					//pParty->Unlink(B);
					//CPartyManager::instance().SetPartyMember(B->GetPlayerID(), NULL);
				}
				pParty->Quit(p->pid);
			}
		}
	}
	else
	{
		// otherwise, only remove itself
		if (p->pid == ch->GetPlayerID())
		{
			if (ch->GetDungeon())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던젼내에서는 파티를 나갈 수 없습니다."));
			}
			else
			{
				if (pParty->IsPartyInAnyDungeon())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던젼내에서는 파티를 나갈 수 없습니다."));
					return;
				}

				if (pParty->GetMemberCount() == 2)
				{
					// party disband
					CPartyManager::instance().DeleteParty(pParty);
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티에서 나가셨습니다."));
					//pParty->SendPartyRemoveOneToAll(ch);
					pParty->Quit(ch->GetPlayerID());
					//pParty->SendPartyRemoveAllToOne(ch);
					//CPartyManager::instance().SetPartyMember(ch->GetPlayerID(), NULL);
				}
			}
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 다른 파티원을 탈퇴시킬 수 없습니다."));
		}
	}
}

void CInputMain::AnswerMakeGuild(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGAnswerMakeGuild* p = (TPacketCGAnswerMakeGuild*) c_pData;

	if (ch->GetGold() < 200000)
		return;

	// if (get_global_time() - ch->GetQuestFlag("guild_manage.new_disband_time") <
	// 		CGuildManager::instance().GetDisbandDelay())
	// {
	// 	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 해산한 후 %d일 이내에는 길드를 만들 수 없습니다."),
	// 			quest::CQuestManager::instance().GetEventFlag("guild_disband_delay"));
	// 	return;
	// }

	// if (get_global_time() - ch->GetQuestFlag("guild_manage.new_withdraw_time") <
	// 		CGuildManager::instance().GetWithdrawDelay())
	// {
	// 	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 탈퇴한 후 %d일 이내에는 길드를 만들 수 없습니다."),
	// 			quest::CQuestManager::instance().GetEventFlag("guild_withdraw_delay"));
	// 	return;
	// }

	if (ch->GetGuild())
		return;

	CGuildManager& gm = CGuildManager::instance();

	TGuildCreateParameter cp{};

	cp.master = ch;
	strlcpy(cp.name, p->guild_name, sizeof(cp.name));

	if (cp.name[0] == 0 || !check_name(cp.name))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("적합하지 않은 길드 이름 입니다."));
		return;
	}

	DWORD dwGuildID = gm.CreateGuild(cp);

	if (dwGuildID)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> [%s] 길드가 생성되었습니다."), cp.name);

		int GuildCreateFee = 200000;

		ch->PointChange(POINT_GOLD, -GuildCreateFee);
		DBManager::instance().SendMoneyLog(MONEY_LOG_GUILD, ch->GetPlayerID(), -GuildCreateFee);

		char Log[128];
		snprintf(Log, sizeof(Log), "GUILD_NAME %s MASTER %s", cp.name, ch->GetName());

		ch->RemoveSpecifyItem(GUILD_CREATE_ITEM_VNUM, 1);
		#ifdef ENABLE_GUILD_TOKEN_AUTH
		CGuildManager::instance().GuildRelink(dwGuildID, ch);
		ch->SendGuildToken();
		#endif
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드 생성에 실패하였습니다."));
}

void CInputMain::PartyUseSkill(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGPartyUseSkill* p = (TPacketCGPartyUseSkill*) c_pData;
	if (!ch->GetParty())
		return;

	if (ch->GetPlayerID() != ch->GetParty()->GetLeaderPID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티 기술은 파티장만 사용할 수 있습니다."));
		return;
	}

	switch (p->bySkillIndex)
	{
		case PARTY_SKILL_HEAL:
			ch->GetParty()->HealParty();
			break;
		case PARTY_SKILL_WARP:
			{
				LPCHARACTER pch = CHARACTER_MANAGER::instance().Find(p->vid);
				if (pch)
					ch->GetParty()->SummonToLeader(pch->GetPlayerID());
				else
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 소환하려는 대상을 찾을 수 없습니다."));
			}
			break;
	}
}

void CInputMain::PartyParameter(LPCHARACTER ch, const char * c_pData)
{
	TPacketCGPartyParameter * p = (TPacketCGPartyParameter *) c_pData;

	if (ch->GetParty())
		ch->GetParty()->SetParameter(p->bDistributeMode);
}

size_t GetSubPacketSize(const GUILD_SUBHEADER_CG& header)
{
	switch (header)
	{
		case GUILD_SUBHEADER_CG_DEPOSIT_MONEY:				return sizeof(int);
		case GUILD_SUBHEADER_CG_WITHDRAW_MONEY:				return sizeof(int);
		case GUILD_SUBHEADER_CG_ADD_MEMBER:					return sizeof(DWORD);
		case GUILD_SUBHEADER_CG_REMOVE_MEMBER:				return sizeof(DWORD);
		case GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME:			return 10;
		case GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY:		return sizeof(BYTE) + sizeof(BYTE);
		case GUILD_SUBHEADER_CG_OFFER:						return sizeof(DWORD);
		case GUILD_SUBHEADER_CG_CHARGE_GSP:					return sizeof(int);
		case GUILD_SUBHEADER_CG_POST_COMMENT:				return 1;
		case GUILD_SUBHEADER_CG_DELETE_COMMENT:				return sizeof(DWORD);
		case GUILD_SUBHEADER_CG_REFRESH_COMMENT:			return 0;
		case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE:		return sizeof(DWORD) + sizeof(BYTE);
		case GUILD_SUBHEADER_CG_USE_SKILL:					return sizeof(TPacketCGGuildUseSkill);
		case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL:		return sizeof(DWORD) + sizeof(BYTE);
		case GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER:		return sizeof(DWORD) + sizeof(BYTE);
	}

	return 0;
}

int CInputMain::Guild(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	if (uiBytes < sizeof(TPacketCGGuild))
		return -1;

	const TPacketCGGuild* p = reinterpret_cast<const TPacketCGGuild*>(data);
	const char* c_pData = data + sizeof(TPacketCGGuild);

	uiBytes -= sizeof(TPacketCGGuild);

	const GUILD_SUBHEADER_CG SubHeader = static_cast<GUILD_SUBHEADER_CG>(p->subheader);
	const size_t SubPacketLen = GetSubPacketSize(SubHeader);

	if (uiBytes < SubPacketLen)
	{
		return -1;
	}

	CGuild* pGuild = ch->GetGuild();

	if (NULL == pGuild)
	{
		if (SubHeader != GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드에 속해있지 않습니다."));
			return SubPacketLen;
		}
	}

	switch (SubHeader)
	{
		case GUILD_SUBHEADER_CG_DEPOSIT_MONEY:
			{
				return SubPacketLen;

				const int gold = MIN(*reinterpret_cast<const int*>(c_pData), __deposit_limit());

				if (gold < 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 잘못된 금액입니다."));
					return SubPacketLen;
				}

				if (ch->GetGold() < gold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 가지고 있는 돈이 부족합니다."));
					return SubPacketLen;
				}

				pGuild->RequestDepositMoney(ch, gold);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_WITHDRAW_MONEY:
			{
				return SubPacketLen;

				const int gold = MIN(*reinterpret_cast<const int*>(c_pData), 500000);

				if (gold < 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 잘못된 금액입니다."));
					return SubPacketLen;
				}

				pGuild->RequestWithdrawMoney(ch, gold);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_ADD_MEMBER:
			{
				const DWORD vid = *reinterpret_cast<const DWORD*>(c_pData);
				LPCHARACTER newmember = CHARACTER_MANAGER::instance().Find(vid);

				if (!newmember)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 그러한 사람을 찾을 수 없습니다."));
					return SubPacketLen;
				}

				// @fixme145 BEGIN (+newmember ispc check)
				if (!ch->IsPC() || !newmember->IsPC())
					return SubPacketLen;
				// @fixme145 END

#if defined(__MESSENGER_BLOCK_SYSTEM__)
				if (MessengerManager::instance().IsBlocked(ch->GetName(), newmember->GetName()))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), newmember->GetName());
					return SubPacketLen;
				}
				else if (MessengerManager::instance().IsBlocked(newmember->GetName(), ch->GetName()))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s has blocked you."), newmember->GetName());
					return SubPacketLen;
				}
#endif

				pGuild->Invite(ch, newmember);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_REMOVE_MEMBER:
			{
				if (pGuild->UnderAnyWar() != 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전 중에는 길드원을 탈퇴시킬 수 없습니다."));
					return SubPacketLen;
				}

				const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				LPCHARACTER member = CHARACTER_MANAGER::instance().FindByPID(pid);

				if (member)
				{
					if (member->GetGuild() != pGuild)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방이 같은 길드가 아닙니다."));
						return SubPacketLen;
					}

					if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드원을 강제 탈퇴 시킬 권한이 없습니다."));
						return SubPacketLen;
					}

					member->SetQuestFlag("guild_manage.new_withdraw_time", get_global_time());
					pGuild->RequestRemoveMember(member->GetPlayerID());

					if (g_bGuildInviteLimit)
					{
						DBManager::instance().Query("REPLACE INTO guild_invite_limit VALUES(%d, %d)", pGuild->GetID(), get_global_time());
					}
				}
				else
				{
					if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드원을 강제 탈퇴 시킬 권한이 없습니다."));
						return SubPacketLen;
					}

					if (pGuild->RequestRemoveMember(pid))
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드원을 강제 탈퇴 시켰습니다."));
					else
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 그러한 사람을 찾을 수 없습니다."));
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME:
			{
				char gradename[GUILD_GRADE_NAME_MAX_LEN + 1];
				strlcpy(gradename, c_pData + 1, sizeof(gradename));

				const TGuildMember * m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (m->grade != GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 직위 이름을 변경할 권한이 없습니다."));
				}
				else if (*c_pData == GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드장의 직위 이름은 변경할 수 없습니다."));
				}
				else if (!check_name(gradename))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 적합하지 않은 직위 이름 입니다."));
				}
				else
				{
					pGuild->ChangeGradeName(*c_pData, gradename);
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY:
			{
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (m->grade != GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 직위 권한을 변경할 권한이 없습니다."));
				}
				else if (*c_pData == GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드장의 권한은 변경할 수 없습니다."));
				}
				else
				{
					pGuild->ChangeGradeAuth(*c_pData, *(c_pData + 1));
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_OFFER:
			{
				DWORD offer = *reinterpret_cast<const DWORD*>(c_pData);

				if (pGuild->GetLevel() >= GUILD_MAX_LEVEL)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드가 이미 최고 레벨입니다."));
				}
				else
				{
					offer /= 100;
					offer *= 100;

					if (pGuild->OfferExp(ch, offer))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> %u의 경험치를 투자하였습니다."), offer);
					}
					else
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 경험치 투자에 실패하였습니다."));
					}
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHARGE_GSP:
			{
				const int offer = *reinterpret_cast<const int*>(c_pData);
				const int gold = offer * 100;

				if (offer < 0 || gold < offer || gold < 0 || ch->GetGold() < gold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 돈이 부족합니다."));
					return SubPacketLen;
				}

				if (!pGuild->ChargeSP(ch, offer))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 용신력 회복에 실패하였습니다."));
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_POST_COMMENT:
			{
				const size_t length = *c_pData;

				if (length > GUILD_COMMENT_MAX_LEN)
				{
					sys_err("POST_COMMENT: %s comment too long (length: %u)", ch->GetName(), length);
					ch->GetDesc()->SetPhase(PHASE_CLOSE);
					return -1;
				}

				if (uiBytes < 1 + length)
					return -1;

				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (length && !pGuild->HasGradeAuth(m->grade, GUILD_AUTH_NOTICE) && *(c_pData + 1) == '!')
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 공지글을 작성할 권한이 없습니다."));
				}
				else
				{
					std::string str(c_pData + 1, length);
					pGuild->AddComment(ch, str);
				}

				return (1 + length);
			}

		case GUILD_SUBHEADER_CG_DELETE_COMMENT:
			{
				const DWORD comment_id = *reinterpret_cast<const DWORD*>(c_pData);

				pGuild->DeleteComment(ch, comment_id);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_REFRESH_COMMENT:
			pGuild->RefreshComment(ch);
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE:
			{
				const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
				const BYTE grade = *(c_pData + sizeof(DWORD));
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (m->grade != GUILD_LEADER_GRADE)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 직위를 변경할 권한이 없습니다."));
				else if (ch->GetPlayerID() == pid)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드장의 직위는 변경할 수 없습니다."));
				else if (grade == 1)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드장으로 직위를 변경할 수 없습니다."));
				else
					pGuild->ChangeMemberGrade(pid, grade);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_USE_SKILL:
			{
				const TPacketCGGuildUseSkill* p = reinterpret_cast<const TPacketCGGuildUseSkill*>(c_pData);

				pGuild->UseSkill(p->dwVnum, ch, p->dwPID);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL:
			{
				const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
				const BYTE is_general = *(c_pData + sizeof(DWORD));
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (m->grade != GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 장군을 지정할 권한이 없습니다."));
				}
				else
				{
					if (!pGuild->ChangeMemberGeneral(pid, is_general))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 더이상 장수를 지정할 수 없습니다."));
					}
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER:
			{
				const DWORD guild_id = *reinterpret_cast<const DWORD*>(c_pData);
				const BYTE accept = *(c_pData + sizeof(DWORD));

				CGuild * g = CGuildManager::instance().FindGuild(guild_id);

				if (g)
				{
					if (accept)
						g->InviteAccept(ch);
					else
						g->InviteDeny(ch->GetPlayerID());
				}
			}
			return SubPacketLen;

	}

	return 0;
}

#if defined(__BL_67_ATTR__)
#include "../../common/VnumHelper.h"

void CInputMain::Attr67(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCG67Attr* p = reinterpret_cast<const TPacketCG67Attr*>(c_pData);
	
	if (ch->IsDead())
		return;
	
	if (ch->PreventTradeWindow(WND_ATTR67ADD, true))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to close other windows."));
		return;
	}
	
	const LPITEM item = ch->GetInventoryItem(p->sItemPos);
	if (!item)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is no item."));
		return;
	}

	switch (item->GetType())
	{
	case ITEM_WEAPON:
	case ITEM_ARMOR:
	case ITEM_COSTUME:
		break;
	default:
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The item type is not suitable for 6-7 attr."));
		return;
	}

	if (item->IsEquipped())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot add a bonus to a equipped item."));
		return;
	}

	if (item->IsExchanging())
		return;

	const int norm_attr_count = item->GetAttributeCount();
	const int rare_attr_count = item->GetRareAttrCount();
	const int attr_set_index = item->GetAttributeSetIndex();

	if (attr_set_index == -1 || norm_attr_count < 5 || rare_attr_count >= 2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item is not suitable for 6-7 attr."));
		return;
	}

	enum
	{
		SUCCESS_PER_MATERIAL = 2,
		MATERIAL_MAX_COUNT = 10,
		SUPPORT_MAX_COUNT = 5,
	};

	if (p->bMaterialCount > MATERIAL_MAX_COUNT || p->bSupportCount > SUPPORT_MAX_COUNT)
		return;

	const DWORD dwMaterialVnum = CItemVnumHelper::Get67MaterialVnum(item->GetType(), item->GetSubType());

	if (dwMaterialVnum == 0 || p->bMaterialCount < 1 
		|| ch->CountSpecifyItem(dwMaterialVnum) < p->bMaterialCount)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough material item."));
		return;
	}
	
	BYTE success = SUCCESS_PER_MATERIAL * p->bMaterialCount;
	
	if (p->sSupportPos != -1)
	{
		const LPITEM support_item = ch->GetInventoryItem(p->sSupportPos);
		if (support_item)
		{
			if (support_item->GetCount() < p->bSupportCount)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough support item."));
				return;
			}

			BYTE uSupportSuccess = 0;
			switch (support_item->GetVnum())
			{
			case 72064:
				uSupportSuccess = 1;
				break;
			case 72065:
				uSupportSuccess = 2;
				break;
			case 72066:
				uSupportSuccess = 4;
				break;
			case 72067:
				uSupportSuccess = 10;
				break;
			default:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The support item is inappropriate."));
				return;
			}

			success += uSupportSuccess * p->bSupportCount;
			support_item->SetCount(support_item->GetCount() - p->bSupportCount);
		}
	}

	ch->RemoveSpecifyItem(dwMaterialVnum, p->bMaterialCount);
	
	const bool bAdded = (number(1, 100) <= success && item->AddSmallRareAttribute());
	ch->ChatPacket(CHAT_TYPE_INFO, "%s!", bAdded ? "Success" : "Failed");
}

void CInputMain::Attr67Close(LPCHARACTER ch, const char* c_pData)
{
	ch->Set67Attr(false);
}
#endif

void CInputMain::Fishing(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGFishing* p = (TPacketCGFishing*)c_pData;

	if (!ch->GetRewardStatus())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You cannot fish when you have the multi farm blocked.");
		return;
	}

	ch->SetRotation(p->dir * 5);
	ch->fishing();
	return;
}

#if defined(__BL_MOVE_COSTUME_ATTR__)
void CInputMain::ItemCombination(LPCHARACTER ch, const char* c_pData)
{
	auto* p = (TPacketCGItemCombination*) c_pData;

	ch->ItemCombination(p->MediumIndex, p->BaseIndex, p->MaterialIndex);
}

void CInputMain::ItemCombinationCancel(LPCHARACTER ch, const char* c_pData)
{
	auto* p = (TPacketCGItemCombinationCancel*) c_pData;

	ch->SetItemCombNpc(NULL);
}
#endif


void CInputMain::ItemGive(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGGiveItem* p = (TPacketCGGiveItem*) c_pData;
	LPCHARACTER to_ch = CHARACTER_MANAGER::instance().Find(p->dwTargetVID);

	if (to_ch)
		ch->GiveItem(to_ch, p->ItemPos);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템을 건네줄 수 없습니다."));
}

void CInputMain::Hack(LPCHARACTER ch, const char * c_pData)
{
	TPacketCGHack * p = (TPacketCGHack *) c_pData;

	char buf[sizeof(p->szBuf)];
	strlcpy(buf, p->szBuf, sizeof(buf));

	sys_err("HACK_DETECT: %s %s", ch->GetName(), buf);

	ch->GetDesc()->SetPhase(PHASE_CLOSE);
}

int CInputMain::MyShop(LPCHARACTER ch, const char * c_pData, size_t uiBytes)
{
	TPacketCGMyShop * p = (TPacketCGMyShop *) c_pData;
	int iExtraLen = p->bCount * sizeof(TShopItemTable);

	if (uiBytes < sizeof(TPacketCGMyShop) + iExtraLen)
		return -1;

	if (ch->GetGold() >= GOLD_MAX)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소유 돈이 20억냥을 넘어 거래를 핼수가 없습니다."));
		sys_log(0, "MyShop ==> OverFlow Gold id %u name %s ", ch->GetPlayerID(), ch->GetName());
		return (iExtraLen);
	}

	if (ch->IsStun() || ch->IsDead())
		return (iExtraLen);

	if (ch->PreventTradeWindow(WND_MYSHOP, true/*except*/))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래중일경우 개인상점을 열수가 없습니다."));
		return (iExtraLen);
	}

	sys_log(0, "MyShop count %d", p->bCount);
	ch->OpenMyShop(p->szSign, (TShopItemTable *) (c_pData + sizeof(TPacketCGMyShop)), p->bCount);
	return (iExtraLen);
}

void CInputMain::Refine(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGRefine* p = reinterpret_cast<const TPacketCGRefine*>(c_pData);

#ifdef ENABLE_ANTI_CMD_FLOOD
	if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::RefineItems, std::chrono::milliseconds(300)))
		return;
#endif

	if (ch->PreventTradeWindow(WND_REFINE, true/*except*/))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("창고,거래창등이 열린 상태에서는 개량을 할수가 없습니다"));
		ch->ClearRefineMode();
		return;
	}

	ch->SetStatistics(STAT_TYPE_REFINE, 1);

	if (p->type == 255)
	{
		// DoRefine Cancel
		ch->ClearRefineMode();
		return;
	}

	if (p->pos >= ch->Inventory_Size())
	{
		ch->ClearRefineMode();
		return;
	}

	LPITEM item = ch->GetInventoryItem(p->pos);

	if (!item)
	{
		ch->ClearRefineMode();
		return;
	}

	ch->SetRefineTime();

	if (p->type == REFINE_TYPE_NORMAL)
	{
		sys_log (0, "refine_type_noraml");
		ch->DoRefine(item);
	}
	else if (p->type == REFINE_TYPE_SCROLL || p->type == REFINE_TYPE_HYUNIRON || p->type == REFINE_TYPE_MUSIN || p->type == REFINE_TYPE_BDRAGON)
	{
		sys_log (0, "refine_type_scroll, ...");
		ch->DoRefineWithScroll(item);
	}
	else if (p->type == REFINE_TYPE_MONEY_ONLY)
	{
		const LPITEM item = ch->GetInventoryItem(p->pos);

		if (NULL != item)
		{
			if (500 <= item->GetRefineSet())
			{
				LogManager::instance().HackLog("DEVIL_TOWER_REFINE_HACK", ch);
			}
			else
			{
				if (ch->GetQuestFlag("dungeon_devil_tower.can_refine") > 0) // @fixme158 (allow multiple refine attempts)
				{
					if (ch->DoRefine(item, true))
						ch->SetQuestFlag("dungeon_devil_tower.can_refine", ch->GetQuestFlag("dungeon_devil_tower.can_refine") - 1);
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "사귀 타워 완료 보상은 한번까지 사용가능합니다.");
				}
			}
		}
	}

	ch->ClearRefineMode();
}

void CInputMain::ChangeLanguage(LPCHARACTER ch, BYTE bLanguage)
{
	if (!ch)
		return;

	if (!ch->GetDesc())
		return;

	if (ch->GetLanguage() == bLanguage)
		return;

	if (bLanguage > LOCALE_YMIR && bLanguage < LOCALE_MAX_NUM)
	{
		TRequestChangeLanguage packet;
		packet.dwAID = ch->GetDesc()->GetAccountTable().id;
		packet.bLanguage = bLanguage;

		db_clientdesc->DBPacketHeader(HEADER_GD_REQUEST_CHANGE_LANGUAGE, 0, sizeof(TRequestChangeLanguage));
		db_clientdesc->Packet(&packet, sizeof(packet));

		ch->ChangeLanguage(bLanguage);
	}
}

void CInputMain::WhisperDetails(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGWhisperDetails* CGWhisperDetails = (TPacketCGWhisperDetails*)c_pData;

	if (!*CGWhisperDetails->name)
		return;

	TPacketGCWhisperDetails GCWhisperDetails;
	GCWhisperDetails.header = HEADER_GC_WHISPER_DETAILS;
	strncpy(GCWhisperDetails.name, CGWhisperDetails->name, sizeof(GCWhisperDetails.name) - 1);

	BYTE bLanguage = LOCALE_DEFAULT;

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(CGWhisperDetails->name);

	if (!pkChr)
	{
		LPDESC pkDesc = NULL;
		CCI* pkCCI = P2P_MANAGER::instance().Find(CGWhisperDetails->name);

		if (pkCCI)
		{
			pkDesc = pkCCI->pkDesc;
			if (pkDesc)
				bLanguage = pkCCI->bLanguage;
		}
	}
	else
	{
		bLanguage = pkChr->GetLanguage();
	}

	GCWhisperDetails.bLanguage = bLanguage;
	ch->GetDesc()->Packet(&GCWhisperDetails, sizeof(GCWhisperDetails));
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
void CInputMain::Acce(LPCHARACTER pkChar, const char* c_pData)
{
	quest::PC * pPC = quest::CQuestManager::instance().GetPCForce(pkChar->GetPlayerID());
	if (pPC->IsRunning())
		return;

	TPacketAcce * sPacket = (TPacketAcce*) c_pData;
	switch (sPacket->subheader)
	{
	case ACCE_SUBHEADER_CG_CLOSE:
	{
		pkChar->CloseAcce();
	}
	break;
	case ACCE_SUBHEADER_CG_ADD:
	{
		pkChar->AddAcceMaterial(sPacket->tPos, sPacket->bPos);
	}
	break;
	case ACCE_SUBHEADER_CG_REMOVE:
	{
		pkChar->RemoveAcceMaterial(sPacket->bPos);
	}
	break;
	case ACCE_SUBHEADER_CG_REFINE:
	{
		pkChar->RefineAcceMaterials();
	}
	break;
	default:
		break;
	}
}
#endif

int CInputMain::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		d->SetPhase(PHASE_CLOSE); // warning te da afara
		return (0);
	}

	int iExtraLen = 0;

	if (test_server && bHeader != HEADER_CG_MOVE)
		sys_log(0, "CInputMain::Analyze() ==> Header [%d] ", bHeader);

	switch (bHeader)
	{
		case HEADER_CG_PONG:
			Pong(d);
			break;

		case HEADER_CG_TIME_SYNC:
			Handshake(d, c_pData);
			break;

		case HEADER_CG_CHAT:
			if (test_server)
			{
				char* pBuf = (char*)c_pData;
				sys_log(0, "%s", pBuf + sizeof(TPacketCGChat));
			}

			if ((iExtraLen = Chat(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_WHISPER:
			if ((iExtraLen = Whisper(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_MOVE:
			Move(ch, c_pData);
			// @fixme103 (removed CheckClientVersion since useless in here)
			break;

		case HEADER_CG_CHARACTER_POSITION:
			Position(ch, c_pData);
			break;

		case HEADER_CG_ITEM_USE:
			if (!ch->IsObserverMode())
				ItemUse(ch, c_pData);
			break;

		case HEADER_CG_ITEM_DROP:
			if (!ch->IsObserverMode())
			{
				ItemDrop(ch, c_pData);
			}
			break;

		case HEADER_CG_ITEM_DROP2:
			if (!ch->IsObserverMode())
				ItemDrop2(ch, c_pData);
			break;
			
#ifdef ENABLE_CHEST_OPEN_RENEWAL
		case HEADER_CG_CHEST_OPEN:
			if (!ch->IsObserverMode())
				ChestOpen(ch, c_pData);
			break;
#endif

		case HEADER_CG_ITEM_DESTROY:
			if (!ch->IsObserverMode())
				ItemDestroy(ch, c_pData);
			break;

		case HEADER_CG_ITEM_MOVE:
			if (!ch->IsObserverMode())
				ItemMove(ch, c_pData);
			break;

		case HEADER_CG_ITEM_PICKUP:
			if (!ch->IsObserverMode())
				ItemPickup(ch, c_pData);
			break;

		case HEADER_CG_ITEM_USE_TO_ITEM:
			if (!ch->IsObserverMode())
				ItemToItem(ch, c_pData);
			break;

		case HEADER_CG_ITEM_GIVE:
			if (!ch->IsObserverMode())
				ItemGive(ch, c_pData);
			break;

		case HEADER_CG_EXCHANGE:
			if (!ch->IsObserverMode())
				Exchange(ch, c_pData);
			break;

		case HEADER_CG_ATTACK:
		case HEADER_CG_SHOOT:
			if (!ch->IsObserverMode())
			{
				Attack(ch, bHeader, c_pData);
			}
			break;

		case HEADER_CG_USE_SKILL:
			if (!ch->IsObserverMode())
				UseSkill(ch, c_pData);
			break;

		case HEADER_CG_QUICKSLOT_ADD:
			QuickslotAdd(ch, c_pData);
			break;

		case HEADER_CG_QUICKSLOT_DEL:
			QuickslotDelete(ch, c_pData);
			break;

		case HEADER_CG_QUICKSLOT_SWAP:
			QuickslotSwap(ch, c_pData);
			break;

		case HEADER_CG_SHOP:
			if ((iExtraLen = Shop(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_MESSENGER:
			if ((iExtraLen = Messenger(ch, c_pData, m_iBufferLeft))<0)
				return -1;
			break;

		case HEADER_CG_ON_CLICK:
			OnClick(ch, c_pData);
			break;

		case HEADER_CG_SYNC_POSITION:
			if ((iExtraLen = SyncPosition(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_ADD_FLY_TARGETING:
		case HEADER_CG_FLY_TARGETING:
			FlyTarget(ch, c_pData, bHeader);
			break;

		case HEADER_CG_SCRIPT_BUTTON:
			ScriptButton(ch, c_pData);
			break;

			// SCRIPT_SELECT_ITEM
		case HEADER_CG_SCRIPT_SELECT_ITEM:
			ScriptSelectItem(ch, c_pData);
			break;
			// END_OF_SCRIPT_SELECT_ITEM

		case HEADER_CG_SCRIPT_ANSWER:
			ScriptAnswer(ch, c_pData);
			break;

		case HEADER_CG_QUEST_INPUT_STRING:
			QuestInputString(ch, c_pData);
			break;

		case HEADER_CG_QUEST_CONFIRM:
			QuestConfirm(ch, c_pData);
			break;

#if defined(__BL_67_ATTR__)
		case HEADER_CG_67_ATTR:
			Attr67(ch, c_pData);
			break;
			
		case HEADER_CG_CLOSE_67_ATTR:
			Attr67Close(ch, c_pData);
			break;
#endif

		case HEADER_CG_TARGET:
			Target(ch, c_pData);
			break;

		case HEADER_CG_WARP:
			Warp(ch, c_pData);
			break;

		case HEADER_CG_SAFEBOX_CHECKIN:
			SafeboxCheckin(ch, c_pData);
			break;

		case HEADER_CG_SAFEBOX_CHECKOUT:
			SafeboxCheckout(ch, c_pData, false);
			break;

		case HEADER_CG_SAFEBOX_ITEM_MOVE:
			SafeboxItemMove(ch, c_pData);
			break;

		case HEADER_CG_MALL_CHECKOUT:
			SafeboxCheckout(ch, c_pData, true);
			break;

#if defined(__BL_MOVE_COSTUME_ATTR__)
		case HEADER_CG_ITEM_COMBINATION:
			ItemCombination(ch, c_pData);
			break;

		case HEADER_CG_ITEM_COMBINATION_CANCEL:
			ItemCombinationCancel(ch, c_pData);
			break;
#endif

		case HEADER_CG_PARTY_INVITE:
			PartyInvite(ch, c_pData);
			break;

		case HEADER_CG_PARTY_REMOVE:
			PartyRemove(ch, c_pData);
			break;

		case HEADER_CG_PARTY_INVITE_ANSWER:
			PartyInviteAnswer(ch, c_pData);
			break;

		case HEADER_CG_PARTY_SET_STATE:
			PartySetState(ch, c_pData);
			break;

		case HEADER_CG_PARTY_USE_SKILL:
			PartyUseSkill(ch, c_pData);
			break;

		case HEADER_CG_PARTY_PARAMETER:
			PartyParameter(ch, c_pData);
			break;

		case HEADER_CG_ANSWER_MAKE_GUILD:
#ifdef ENABLE_NEWGUILDMAKE
			ch->ChatPacket(CHAT_TYPE_INFO, "<%s> AnswerMakeGuild disabled", __FUNCTION__);
#else
			AnswerMakeGuild(ch, c_pData);
#endif
			break;

		case HEADER_CG_GUILD:
			if ((iExtraLen = Guild(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_FISHING:
			Fishing(ch, c_pData);
			break;

		case HEADER_CG_HACK:
			Hack(ch, c_pData);
			break;

		case HEADER_CG_MYSHOP:
			if ((iExtraLen = MyShop(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		case HEADER_CG_ACCE:
			Acce(ch, c_pData);
			break;
#endif

		case HEADER_CG_WHISPER_DETAILS:
			WhisperDetails(ch, c_pData);
			break;

		case HEADER_CG_CHANGE_LANGUAGE:
			{
				TPacketChangeLanguage* p = reinterpret_cast <TPacketChangeLanguage*>((void*)c_pData);
				ChangeLanguage(ch, p->bLanguage);
			}
			break;

		case HEADER_CG_REFINE:
			Refine(ch, c_pData);
			break;

#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
		case HEADER_CG_CUBE_RENEWAL:
			CubeRenewalSend(ch, c_pData);
			break;
#endif

		case HEADER_CG_CLIENT_VERSION:
			Version(ch, c_pData);
			break;

#ifdef ENABLE_OFFLINESHOP_SYSTEM
		case HEADER_CG_OFFLINE_SHOP:
			if ((iExtraLen = OfflineShop(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
		case HEADER_CG_MY_OFFLINE_SHOP:
			if ((iExtraLen = MyOfflineShop(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
#endif

#ifdef __SEND_TARGET_INFO__
		case HEADER_CG_TARGET_INFO_LOAD:
			TargetInfoLoad(ch, c_pData);
			break;
#endif

#ifdef ENABLE_SWITCHBOT
		case HEADER_CG_SWITCHBOT:
			if ((iExtraLen = Switchbot(ch, c_pData, m_iBufferLeft)) < 0)
			{
				return -1;
			}
			break;
#endif

		case HEADER_CG_DRAGON_SOUL_REFINE:
			{
				TPacketCGDragonSoulRefine* p = reinterpret_cast <TPacketCGDragonSoulRefine*>((void*)c_pData);
				switch(p->bSubType)
				{
				case DS_SUB_HEADER_CLOSE:
					ch->DragonSoul_RefineWindow_Close();
					break;
				case DS_SUB_HEADER_DO_REFINE_GRADE:
					{
						DSManager::instance().DoRefineGrade(ch, p->ItemGrid);
					}
					break;
				case DS_SUB_HEADER_DO_REFINE_STEP:
					{
						DSManager::instance().DoRefineStep(ch, p->ItemGrid);
					}
					break;
				case DS_SUB_HEADER_DO_REFINE_STRENGTH:
					{
						DSManager::instance().DoRefineStrength(ch, p->ItemGrid);
					}
					break;
#if defined(__DS_CHANGE_ATTR__)
				case DS_SUB_HEADER_DO_CHANGE_ATTR:
					{
						DSManager::instance().DoChangeAttribute(ch, p->ItemGrid);
					}
					break;
#endif
				}
			}
			break;
	}
	return (iExtraLen);
}

#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
void CInputMain::CubeRenewalSend(LPCHARACTER ch, const char* data)
{
	struct packet_send_cube_renewal * pinfo = (struct packet_send_cube_renewal *) data;
	switch (pinfo->subheader)
	{
		case CUBE_RENEWAL_SUB_HEADER_MAKE_ITEM:
		{

			int index_item = pinfo->index_item;
			int count_item = pinfo->count_item;
			int index_item_improve = pinfo->index_item_improve;

			Cube_Make(ch,index_item,count_item,index_item_improve);
		}
		break;

		case CUBE_RENEWAL_SUB_HEADER_CLOSE:
		{
			Cube_close(ch);
		}
		break;
	}
}
#endif

int CInputDead::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		return 0;
	}

	int iExtraLen = 0;

	switch (bHeader)
	{
		case HEADER_CG_PONG:
			Pong(d);
			break;

		case HEADER_CG_TIME_SYNC:
			Handshake(d, c_pData);
			break;

		case HEADER_CG_CHAT:
			if ((iExtraLen = Chat(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;

			break;

		case HEADER_CG_WHISPER:
			if ((iExtraLen = Whisper(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;

			break;

		case HEADER_CG_HACK:
			Hack(ch, c_pData);
			break;

		default:
			return (0);
	}

	return (iExtraLen);
}

#if defined(__SKILL_COLOR_SYSTEM__)
void CInputMain::SetSkillColor(LPCHARACTER ch, const char* pcData)
{
	if (!ch)
		return;

#if defined(__SKILL_COLOR_NEED_ITEM__)
	int iCountNeedItem = ch->CountSpecifyItem(50512);
	if (iCountNeedItem <= 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The light of the Rainbow Stone is needed."));
		return;
	}
	ch->RemoveSpecifyItem(50512);
#endif

	TPacketCGSkillColor* CGPacket = (TPacketCGSkillColor*)pcData;

	if (CGPacket->bSkillSlot >= ESkillColorLength::MAX_SKILL_COUNT)
		return;

	DWORD dwData[ESkillColorLength::MAX_SKILL_COUNT + ESkillColorLength::MAX_BUFF_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
	memcpy(dwData, ch->GetSkillColor(), sizeof(dwData));

	dwData[CGPacket->bSkillSlot][0] = CGPacket->dwCol1;
	dwData[CGPacket->bSkillSlot][1] = CGPacket->dwCol2;
	dwData[CGPacket->bSkillSlot][2] = CGPacket->dwCol3;
	dwData[CGPacket->bSkillSlot][3] = CGPacket->dwCol4;
	dwData[CGPacket->bSkillSlot][4] = CGPacket->dwCol5;

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have changed the color of your skill."));

	ch->SetSkillColor(dwData[0]);

	TSkillColor GDPacket;
	memcpy(GDPacket.dwSkillColor, dwData, sizeof(dwData));
	GDPacket.dwPlayerID = ch->GetPlayerID();
	db_clientdesc->DBPacketHeader(HEADER_GD_SKILL_COLOR_SAVE, 0, sizeof(TSkillColor));
	db_clientdesc->Packet(&GDPacket, sizeof(TSkillColor));
}
#endif

#ifdef ENABLE_OFFLINESHOP_SYSTEM
#include "offline_shop.h"
#include "offlineshop_manager.h"
int CInputMain::MyOfflineShop(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGMyOfflineShop* p = (TPacketCGMyOfflineShop*)c_pData;
	int iExtraLen = p->bCount * sizeof(TOfflineShopItemTable);
	if (uiBytes < sizeof(TPacketCGMyOfflineShop) + iExtraLen)
		return -1;
	COfflineShopManager::instance().OpenMyOfflineShop(ch, p->szSign, (TOfflineShopItemTable*)(c_pData + sizeof(TPacketCGMyOfflineShop)), p->bCount, p->shopVnum, p->shopTitle);
	return (iExtraLen);
}
int CInputMain::OfflineShop(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	TPacketCGShop* p = (TPacketCGShop*)data;

	if (uiBytes < sizeof(TPacketCGShop))
		return -1;

	if (!ch || !ch->GetDesc())
		return -1;

	const char* c_pData = data + sizeof(TPacketCGShop);
	uiBytes -= sizeof(TPacketCGShop);
	switch (p->subheader)
	{
		case SHOP_SUBHEADER_CG_END:
			COfflineShopManager::instance().StopShopping(ch);
			break;
		case SHOP_SUBHEADER_CG_BUY:
			{
				if (uiBytes < sizeof(DWORD))
					return -1;
				const DWORD itemID = *(DWORD*)c_pData;
				COfflineShopManager::instance().Buy(ch, p->vid, p->pos, itemID);
				return sizeof(DWORD);
			}
		case SHOP_SUBHEADER_CG_DESTROY_OFFLINE_SHOP:
			COfflineShopManager::instance().DestroyOfflineShop(ch);
			break;
		case SHOP_SUBHEADER_CG_ADD_ITEM:
			{
				if (uiBytes < sizeof(TOfflineShopAddItem))
					return -1;
				TOfflineShopAddItem* pTable = (TOfflineShopAddItem*)(c_pData);
				COfflineShopManager::instance().AddItem(ch, pTable->bDisplayPos, pTable->bPos, pTable->lPrice);
				return sizeof(TOfflineShopAddItem);
			}
		case SHOP_SUBHEADER_CG_REMOVE_ITEM:
			{
				if (uiBytes < sizeof(BYTE) + sizeof(DWORD) + sizeof(BYTE))
					return -1;
				const BYTE bPos = *(BYTE*)c_pData;
				c_pData += sizeof(BYTE);
				const DWORD bItemID = *(DWORD*)c_pData;
				c_pData += sizeof(DWORD);
				const BYTE bTakeAll = *(BYTE*)c_pData;
				c_pData += sizeof(BYTE);
				COfflineShopManager::instance().RemoveItem(ch, bPos, bItemID, bTakeAll);
				return sizeof(BYTE)+sizeof(DWORD)+sizeof(BYTE);
			}
		case SHOP_SUBHEADER_CG_OPEN_SLOT:
			{
				if (uiBytes < sizeof(BYTE))
					return -1;
				const BYTE bPos = *(BYTE*)c_pData;
				COfflineShopManager::instance().OpenSlot(ch, bPos);
				return sizeof(BYTE);
			}
		case SHOP_SUBHEADER_CG_CHECK:
			COfflineShopManager::instance().HasOfflineShop(ch);
			break;
		case SHOP_SUBHEADER_CG_CHANGE_TITLE:
			COfflineShopManager::instance().ChangeTitle(ch, p->title);
			break;
		case SHOP_SUBHEADER_CG_TAKE_MONEY:
			COfflineShopManager::instance().WithdrawMoney(ch);
			break;
		case SHOP_SUBHEADER_CG_CHANGE_DECORATION:
			{
				if (uiBytes < sizeof(TShopDecoration))
					return -1;
				TShopDecoration* decoration = (TShopDecoration*)c_pData;
				COfflineShopManager::instance().ChangeDecoration(ch, decoration);
				return sizeof(TShopDecoration);
			}
			break;
		case SHOP_SUBHEADER_CG_LOG_REMOVE:
			COfflineShopManager::instance().ShopLogRemove(ch);
			break;
		case SHOP_SUBHEADER_CG_BUTTON:
			COfflineShopManager::instance().OpenOfflineShop(ch);
			break;
		case SHOP_SUBHEADER_CG_GET_ITEM:
			COfflineShopManager::instance().GetBackItem(ch);
			break;
		case SHOP_SUBHEADER_CG_ADD_TIME:
			COfflineShopManager::instance().ShopAddTime(ch);
			break;
		case SHOP_SUBHEADER_CG_OPEN_WITH_VID:
			{
				if (uiBytes < sizeof(DWORD))
					return -1;
				const DWORD bVID = *(DWORD*)c_pData;
				COfflineShopManager::instance().OpenOfflineShopWithVID(ch, bVID);
				return sizeof(DWORD);
			}
			break;
		case SHOP_SUBHEADER_CG_ADD_ITEM_SHORTCUT:
			{
				if (uiBytes < sizeof(TItemPos)+sizeof(long long))
					return -1;
				const TItemPos itemPos = *(TItemPos*)c_pData;
				c_pData += sizeof(TItemPos);
				const long long itemPrice = *(long long*)c_pData;
				c_pData += sizeof(long long);
				COfflineShopManager::instance().AddItemShortcut(ch, itemPos, itemPrice);
				return sizeof(TItemPos)+sizeof(long long);
			}
			break;
		case SHOP_SUBHEADER_CG_AVERAGE_ITEM:
			{
				if (uiBytes < sizeof(DWORD))
					return -1;
				const DWORD itemIdx = *(DWORD*)c_pData;
				c_pData += sizeof(DWORD);
				COfflineShopManager::instance().CheckAveragePrice(ch, itemIdx);
				return sizeof(DWORD);
			}
		break;
		case SHOP_SUBHEADER_CG_TELEPORT:
			{
				if (uiBytes < CHARACTER_NAME_MAX_LEN + 1)
					return -1;
				const std::string playerName(c_pData);
				COfflineShopManager::instance().Teleport(ch, playerName.c_str());
				return CHARACTER_NAME_MAX_LEN + 1;
			}
		case SHOP_SUBHEADER_CG_TAKE_OUT_ALL_ITEM:
			COfflineShopManager::instance().RemoveItemAll(ch);
			break;
		case SHOP_SUBHEADER_CG_MOVE_ITEM:
			{
				if (uiBytes < sizeof(WORD) + sizeof(WORD))
					return -1;
				const WORD slotPos = *(WORD*)c_pData;
				c_pData += sizeof(WORD);
				const WORD targetPos = *(WORD*)c_pData;
				c_pData += sizeof(WORD);
				COfflineShopManager::instance().MoveItem(ch, slotPos, targetPos);
				return sizeof(WORD) + sizeof(WORD);
			}
			break;
		case SHOP_SUBHEADER_CG_CHANGE_PRICE:
			{
				if (uiBytes < sizeof(BYTE) + sizeof(DWORD) + sizeof(long long) + sizeof(BYTE))
					return -1;
				const BYTE bPos = *(BYTE*)c_pData;
				c_pData += sizeof(BYTE);
				const DWORD itemID = *(DWORD*)c_pData;
				c_pData += sizeof(DWORD);
				const long long itemPrice = *(long long*)c_pData;
				c_pData += sizeof(long long);
				const BYTE bAllItem = *(BYTE*)c_pData;
				c_pData += sizeof(BYTE);
				COfflineShopManager::instance().ChangePrice(ch, bPos, itemID, itemPrice, bAllItem);
				return sizeof(BYTE) + sizeof(DWORD)+ sizeof(long long) + sizeof(BYTE);
			}
			break;
		default:
			sys_err("CInputMain::OfflineShop : Unknown subheader %d : %s", p->subheader, ch->GetName());
			break;
	}

	return 0;
}
#endif

#ifdef ENABLE_SWITCHBOT
int CInputMain::Switchbot(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	const TPacketCGSwitchbot* p = reinterpret_cast<const TPacketCGSwitchbot*>(data);

	if (uiBytes < sizeof(TPacketCGSwitchbot))
	{
		return -1;
	}

	const char* c_pData = data + sizeof(TPacketCGSwitchbot);
	uiBytes -= sizeof(TPacketCGSwitchbot);

	switch (p->subheader)
	{
		case SUBHEADER_CG_SWITCHBOT_START:
		{
			size_t extraLen = sizeof(TSwitchbotAttributeAlternativeTable) * SWITCHBOT_ALTERNATIVE_COUNT;
			if (uiBytes < extraLen)
			{
				return -1;
			}
	
			std::vector<TSwitchbotAttributeAlternativeTable> vec_alternatives;
	
			for (BYTE alternative = 0; alternative < SWITCHBOT_ALTERNATIVE_COUNT; ++alternative)
			{
				const TSwitchbotAttributeAlternativeTable* pAttr = reinterpret_cast<const TSwitchbotAttributeAlternativeTable*>(c_pData);
				c_pData += sizeof(TSwitchbotAttributeAlternativeTable);
	
				vec_alternatives.emplace_back(*pAttr);
			}
	
			CSwitchbotManager::Instance().Start(ch->GetPlayerID(), p->slot, vec_alternatives);
			return extraLen;
		}
	
		case SUBHEADER_CG_SWITCHBOT_STOP:
		{
			CSwitchbotManager::Instance().Stop(ch->GetPlayerID(), p->slot);
			return 0;
		}
	}

	return 0;
}
#endif

//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
