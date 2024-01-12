#include "stdafx.h"

#include "../../common/length.h"
#include "../../common/VnumHelper.h"
#include "CsvReader.h"

#include "locale_service.h"

int g_iUseLocale = 0;
static const std::string c_stDefaultString = "NoName";

using SToken = struct SToken
{
	DWORD dwVNum;
	std::string stString;
};

using LocaleStringMapType = std::map<std::string, std::string>;
using LocaleMapType = std::map<DWORD, std::string>;

LocaleStringMapType gLocaleStringMapType[LOCALE_MAX_NUM]; // Support for old "Ymir" LC_TEXT.
LocaleMapType gLocaleQuestMapType[LOCALE_MAX_NUM];
LocaleMapType gLocaleItemNameMapType[LOCALE_MAX_NUM];
LocaleMapType gLocaleMobNameMapType[LOCALE_MAX_NUM];
LocaleMapType gLocaleSkillNameMapType[LOCALE_MAX_NUM];
LocaleMapType gLocaleQuizMapType[LOCALE_MAX_NUM];

const char* GetLocaleCodeName(BYTE bLocale)
{
	switch (bLocale)
	{
	case LOCALE_YMIR: return "kr"; // Korean
	case LOCALE_EN: return "en"; // English
	case LOCALE_PT: return "pt"; // Portuguese
	case LOCALE_ES: return "es"; // Spanish
	case LOCALE_FR: return "fr"; // French
	case LOCALE_GR: return "gr"; // Greek
	case LOCALE_DE: return "de"; // German
	case LOCALE_RO: return "ro"; // Romanian
	case LOCALE_PL: return "pl"; // Polish
	case LOCALE_IT: return "it"; // Italian
	case LOCALE_CZ: return "cz"; // Czech
	case LOCALE_HU: return "hu"; // Hungarian
	case LOCALE_TR: return "tr"; // Turkish
	default: return "en"; // English
	}
}

///////////////////////////////////////////////////////////////////////////////
// Locale String (locale_quiz.txt)
//
#define NUM_LOCALES 2
void AddLocaleStrings(const char** c_szStrings, BYTE bLocale)
{
	LocaleStringMapType::iterator it = gLocaleStringMapType[bLocale].find(c_szStrings[0]);
	if (it == gLocaleStringMapType[bLocale].end())
		gLocaleStringMapType[bLocale].emplace(std::make_pair(c_szStrings[0], c_szStrings[1]));
}
const char* FindEndQuote(const char* c_szFileName)
{
	const char* c_szTmp = c_szFileName;
	BYTE bQuote = 0;

	while (*c_szTmp)
	{
		if (bQuote && *c_szTmp == '\\' && *(c_szTmp + 1))
		{
			// \ ?? ??? " ? ????.
			switch (*(c_szTmp + 1))
			{
			case '"':
				c_szTmp += 2;
				continue;
			}
		}
		else if (*c_szTmp == '"')
		{
			bQuote = ~bQuote;
		}
		else if (!bQuote && *c_szTmp == ';')
			return c_szTmp;

		c_szTmp++;
	}

	return nullptr;
}
char* ConvertLocaleStrings(const char* c_szSrc, int iLen)
{
	const char* c_szTmp = nullptr;
	char* pBuf = nullptr, * pDest = nullptr;

	int iStart = 0;
	char szLastChar = 0;

	int i = 0, j = 0;

	if (!iLen)
		return NULL;

	pBuf = M2_NEW char[iLen + 1];

	for (j = i = 0, c_szTmp = c_szSrc, pDest = pBuf; i < iLen; i++, c_szTmp++)
	{
		if (*c_szTmp == '"')
		{
			if (szLastChar != '\\')
				iStart = ~iStart;
			else
				goto ENCODE;
		}
		else if (*c_szTmp == ';')
		{
			if (szLastChar != '\\' && !iStart)
				break;
			else
				goto ENCODE;
		}
		else if (iStart)
		{
		ENCODE:
			if (*c_szTmp == '\\' && *(c_szTmp + 1) == 'n')
			{
				*pDest++ = '\n';
				c_szTmp++;
				szLastChar = '\n';
			}
			else
			{
				*pDest++ = *c_szTmp;
				szLastChar = *c_szTmp;
			}
			j++;
		}
	}

	if (!j)
	{
		M2_DELETE_ARRAY(pBuf);
		return nullptr;
	}

	*pDest = '\0';
	return pBuf;
}
void LoadLocaleString(const char* c_szFileName, BYTE bLocale)
{
	FILE* pFile = nullptr;
	if (nullptr == (pFile = fopen(c_szFileName, "rb")))
	{
		fprintf(stderr, "cannot init locale file %s\n", c_szFileName);
		return;
	}

	fseek(pFile, 0L, SEEK_END);
	long i = ftell(pFile);
	fseek(pFile, 0L, SEEK_SET);

	i++;

	char* pBuf = new char[i];
	memset(pBuf, 0, i);

	fread(pBuf, i - 1, sizeof(char), pFile);
	fclose(pFile);

	if (!pBuf)
	{
		fprintf(stderr, "locale_read: no file %s\n", c_szFileName);
		return;
	}

	gLocaleStringMapType[bLocale].clear(); // Support for resetting the map type.

	char* szStrings[NUM_LOCALES];
	const char* c_szTmp;
	const char* c_szEnd;

	c_szTmp = pBuf;
	do
	{
		for (i = 0; i < NUM_LOCALES; i++)
			szStrings[i] = nullptr;

		if (*c_szTmp == '"')
		{
			for (i = 0; i < NUM_LOCALES; i++)
			{
				if (!(c_szEnd = FindEndQuote(c_szTmp)))
					break;

				szStrings[i] = ConvertLocaleStrings(c_szTmp, c_szEnd - c_szTmp);
				c_szTmp = ++c_szEnd;

				while (*c_szTmp == '\n' || *c_szTmp == '\r' || *c_szTmp == ' ') c_szTmp++;

				if (i + 1 == NUM_LOCALES)
					break;

				if (*c_szTmp != '"')
				{
					fprintf(stderr, "locale_init: invalid format filename %s\n", c_szFileName);
					break;
				}
			}

			if (szStrings[0] == nullptr || szStrings[1] == nullptr)
				break;

			AddLocaleStrings((const char**)szStrings, bLocale);

			for (i = 0; i < NUM_LOCALES; i++)
				if (szStrings[i])
					M2_DELETE_ARRAY(szStrings[i]);
		}
		else
		{
			c_szTmp = strchr(c_szTmp, '\n');
			if (c_szTmp)
				c_szTmp++;
		}
	} while (c_szTmp && *c_szTmp);
	delete[](pBuf);
}
const char* FindLocaleString(const char* c_szString, BYTE bLocale)
{
	if (!*c_szString || bLocale == LOCALE_YMIR)
		return c_szString;

	LocaleStringMapType::iterator it = gLocaleStringMapType[bLocale].find(c_szString);
	if (it != gLocaleStringMapType[bLocale].end())
		return it->second.c_str();
	return c_szString;
}
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Locale Quest (locale_quest.txt)
//
void LoadLocaleQuest(const char* c_szFileName, BYTE bLocale)
{
	FILE* pFile = nullptr;
	if (nullptr == (pFile = fopen(c_szFileName, "r")))
		return;

	//gLocaleQuestMapType[bLocale].clear(); // Support for resetting the map type.

	static SToken sToken = { 0, "" };
	BYTE bToken = 0;

	char szLine[BUFSIZ] = { 0 }, * pBuf = nullptr;
	while (fgets(szLine, BUFSIZ, pFile))
	{
		// Tokenise the read line, using "\" delimiter*/
		pBuf = strtok(szLine, "\t");
		sToken.dwVNum = std::atol(pBuf);

		while (nullptr != (pBuf = strtok(nullptr, "\n")))
		{
			bToken++;
			// Store the tokens as per structure members , where (i == 0) is first member and so on...
			if (bToken == 1)
				sToken.stString = pBuf;
		}
		sToken.stString = sToken.stString + " "; // add empty space at the end of the string to avoid encoding errors.

		gLocaleQuestMapType[bLocale].emplace(std::make_pair(sToken.dwVNum, sToken.stString));

		sToken.stString.clear(); // Clear last string.
		bToken = 0; // Reset token value of iToken.
	}
	fclose(pFile);
}
const char* FindLocaleQuest(DWORD dwVNum, BYTE bLocale)
{
	LocaleMapType::iterator it = gLocaleQuestMapType[bLocale].find(dwVNum);
	if (it != gLocaleQuestMapType[bLocale].end())
		return it->second.c_str();
	return c_stDefaultString.c_str();
}
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Locale Item (item_names.txt)
//
void LoadLocaleItemName(const char* c_szFileName, BYTE bLocale)
{
	cCsvTable CsvTable;
	if (!CsvTable.Load(c_szFileName, '\t'))
	{
		fprintf(stderr, "%s couldn't be loaded or its format is incorrect.\n", c_szFileName);
		return;
	}

	CsvTable.Next();

	DWORD dwVNum = 0;
	while (CsvTable.Next())
	{
		if (CsvTable.ColCount() < 2)
			continue;

		dwVNum = atol(CsvTable.AsStringByIndex(0));
		if (CItemVnumHelper::IsDragonSoul(dwVNum) == true)
		{
			for (DWORD dwDSVNum = dwVNum; dwDSVNum < (dwVNum + 100); ++dwDSVNum)
				gLocaleItemNameMapType[bLocale].emplace(std::make_pair(dwDSVNum, CsvTable.AsStringByIndex(1)));
		}
		else
			gLocaleItemNameMapType[bLocale].emplace(std::make_pair(dwVNum, CsvTable.AsStringByIndex(1)));
	}

	CsvTable.Destroy();
}
const char* FindLocaleItemName(DWORD dwVNum, BYTE bLocale)
{
	LocaleMapType::iterator it = gLocaleItemNameMapType[bLocale].find(dwVNum);
	if (it != gLocaleItemNameMapType[bLocale].end())
		return it->second.c_str();
	return c_stDefaultString.c_str();
}
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Locale Mob (mob_names.txt)
//
void LoadLocaleMobName(const char* c_szFileName, BYTE bLocale)
{
	cCsvTable CsvTable;
	if (!CsvTable.Load(c_szFileName, '\t'))
	{
		fprintf(stderr, "%s couldn't be loaded or its format is incorrect.\n", c_szFileName);
		return;
	}

	CsvTable.Next();

	while (CsvTable.Next())
	{
		if (CsvTable.ColCount() < 2)
			continue;

		gLocaleMobNameMapType[bLocale].emplace(std::make_pair(atol(CsvTable.AsStringByIndex(0)), CsvTable.AsStringByIndex(1)));
	}

	CsvTable.Destroy();
}
const char* FindLocaleMobName(DWORD dwVNum, BYTE bLocale)
{
	LocaleMapType::iterator it = gLocaleMobNameMapType[bLocale].find(dwVNum);
	if (it != gLocaleMobNameMapType[bLocale].end())
		return it->second.c_str();
	return c_stDefaultString.c_str();
}
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Locale Skill (skill_names.txt)
//
void LoadLocaleSkillName(const char* c_szFileName, BYTE bLocale)
{
	cCsvTable CsvTable;
	if (!CsvTable.Load(c_szFileName, '\t'))
	{
		fprintf(stderr, "%s couldn't be loaded or its format is incorrect.\n", c_szFileName);
		return;
	}

	while (CsvTable.Next())
	{
		if (CsvTable.ColCount() < 2)
			continue;

		gLocaleSkillNameMapType[bLocale].emplace(std::make_pair(atoi(CsvTable.AsStringByIndex(0)), CsvTable.AsStringByIndex(1)));
	}

	CsvTable.Destroy();
}
const char* FindLocaleSkillName(DWORD dwVNum, BYTE bLocale)
{
	LocaleMapType::iterator it = gLocaleSkillNameMapType[bLocale].find(dwVNum);
	if (it != gLocaleSkillNameMapType[bLocale].end())
		return it->second.c_str();
	return c_stDefaultString.c_str();
}
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Locale Quiz (locale_quiz.txt)
//
void LoadLocaleQuiz(const char* c_szFileName, BYTE bLocale)
{
	FILE* pFile = nullptr;
	if (nullptr == (pFile = fopen(c_szFileName, "r")))
		return;

	gLocaleQuizMapType[bLocale].clear(); // Support for resting the map type.

	static SToken sToken = { 0, "" };
	BYTE bToken = 0;

	char szLine[BUFSIZ] = { 0 }, * pBuf = nullptr;
	while (fgets(szLine, BUFSIZ, pFile))
	{
		// Tokenise the read line, using "\" delimiter*/
		pBuf = strtok(szLine, "\t");

		while (nullptr != (pBuf = strtok(nullptr, "\t\n")))
		{
			bToken++;
			// Store the tokens as per structure members , where (i == 0) is first member and so on...
			if (bToken == 1)
				sToken.dwVNum = std::atol(pBuf);
			else if (bToken == 2)
				sToken.stString = pBuf;
		}
		sToken.stString = sToken.stString + " "; // add empty space at the end of the string to avoid encoding errors.

		gLocaleQuizMapType[bLocale].emplace(std::make_pair(sToken.dwVNum, sToken.stString));

		sToken.stString.clear(); // Clear last string.
		bToken = 0; // Reset token value of iToken.
	}
	fclose(pFile);
}
const char* FindLocaleQuiz(DWORD dwVNum, BYTE bLocale)
{
	LocaleMapType::iterator it = gLocaleQuizMapType[bLocale].find(dwVNum);
	if (it != gLocaleQuizMapType[bLocale].end())
		return it->second.c_str();
	return c_stDefaultString.c_str();
}
//
///////////////////////////////////////////////////////////////////////////////
