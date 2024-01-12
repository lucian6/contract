#ifndef __INC_LOCALE_H__
#define __INC_LOCALE_H__

#include "../../common/length.h"

extern "C"
{
	extern int g_iUseLocale;
	extern const char* GetLocaleCodeName(BYTE bLocale);
#define LC_CODE(locale) GetLocaleCodeName(locale)

	void LoadLocaleString(const char* c_szFileName, BYTE bLocale = LOCALE_YMIR);
	const char* FindLocaleString(const char* c_szString, BYTE bLocale = LOCALE_YMIR);
#define LC_TEXT(string) FindLocaleString(string)
#define LC_STRING(string, locale) FindLocaleString(string, locale)

	// Locale Quest (locale_quest.txt)
	void LoadLocaleQuest(const char* c_szFileName, BYTE bLocale);
	const char* FindLocaleQuest(DWORD dwVNum, BYTE bLocale = LOCALE_YMIR);
#define LC_QUEST(vnum, locale) FindLocaleQuest(vnum, locale)

	// Locale Item (item_names.txt)
	void LoadLocaleItemName(const char* c_szFileName, BYTE bLocale);
	const char* FindLocaleItemName(DWORD dwVNum, BYTE bLocale = LOCALE_YMIR);
#define LC_ITEM_NAME(vnum, locale) FindLocaleItemName(vnum, locale)

	// Locale Mob (mob_names.txt)
	void LoadLocaleMobName(const char* c_szFileName, BYTE bLocale);
	const char* FindLocaleMobName(DWORD dwVNum, BYTE bLocale = LOCALE_YMIR);
#define LC_MOB_NAME(vnum, locale) FindLocaleMobName(vnum, locale)

	// Locale Skill (skill_names.txt)
	void LoadLocaleSkillName(const char* c_szFileName, BYTE bLocale);
	const char* FindLocaleSkillName(DWORD dwVNum, BYTE bLocale = LOCALE_YMIR);
#define LC_SKILL_NAME(vnum, locale) FindLocaleSkillName(vnum, locale)

	// Locale Quiz (locale_quiz.txt)
	void LoadLocaleQuiz(const char* c_szFileName, BYTE bLocale);
	const char* FindLocaleQuiz(DWORD dwVNum, BYTE bLocale = LOCALE_YMIR);
#define LC_QUIZ(vnum, locale) FindLocaleQuiz(vnum, locale)
};

#endif // __INC_LOCALE_H__
