#ifndef __INC_METIN_II_GAME_DRAGON_SOUL_H__
#define __INC_METIN_II_GAME_DRAGON_SOUL_H__

#include "../../common/length.h"

class CHARACTER;
class CItem;

class DragonSoulTable;

class DSManager : public singleton<DSManager>
{
public:
	DSManager();
	~DSManager();
	bool	ReadDragonSoulTableFile(const char * c_pszFileName);

	void	GetDragonSoulInfo(DWORD dwVnum, OUT BYTE& bType, OUT BYTE& bGrade, OUT BYTE& bStep, OUT BYTE& bRefine) const;
	WORD	GetBasePosition(const LPITEM pItem) const;
	bool	IsValidCellForThisItem(const LPITEM pItem, const TItemPos& Cell) const;
	int		GetDuration(const LPITEM pItem) const;

	bool	CheckDragonSoulGrade(DWORD vnum)
	{
		switch (vnum)
		{
			case 115400:
			case 115410:
			case 115420:
			case 115430:
			case 115440:
			case 115450:
			case 115460:
			case 125400:
			case 125410:
			case 125420:
			case 125430:
			case 125440:
			case 125450:
			case 125460:
			case 135400:
			case 135410:
			case 135420:
			case 135430:
			case 135440:
			case 135450:
			case 135460:
			case 145400:
			case 145410:
			case 145420:
			case 145430:
			case 145440:
			case 145450:
			case 145460:
			case 155400:
			case 155410:
			case 155420:
			case 155430:
			case 155440:
			case 155450:
			case 155460:
			case 165400:
			case 165410:
			case 165420:
			case 165430:
			case 165440:
			case 165450:
			case 165460:
				return true;
		}
		return false;
	}

	bool	ExtractDragonHeart(LPCHARACTER ch, LPITEM pItem, LPITEM pExtractor = NULL);

	bool	PullOut(LPCHARACTER ch, TItemPos DestCell, IN OUT LPITEM& pItem, LPITEM pExtractor = NULL);

	bool	DoRefineGrade(LPCHARACTER ch, TItemPos (&aItemPoses)[DRAGON_SOUL_REFINE_GRID_SIZE]);
	bool	DoRefineStep(LPCHARACTER ch, TItemPos (&aItemPoses)[DRAGON_SOUL_REFINE_GRID_SIZE]);
	bool	DoRefineStrength(LPCHARACTER ch, TItemPos (&aItemPoses)[DRAGON_SOUL_REFINE_GRID_SIZE]);
#if defined(__DS_CHANGE_ATTR__)
	bool	DoChangeAttribute(LPCHARACTER lpCh, TItemPos(&arItemPos)[DRAGON_SOUL_REFINE_GRID_SIZE]);
#endif

	bool	DragonSoulItemInitialize(LPITEM pItem);

	bool	IsTimeLeftDragonSoul(LPITEM pItem) const;
	int		LeftTime(LPITEM pItem) const;
	bool	ActivateDragonSoul(LPITEM pItem);
	bool	DeactivateDragonSoul(LPITEM pItem, bool bSkipRefreshOwnerActiveState = false);
	bool	IsActiveDragonSoul(LPITEM pItem) const;

#ifdef ENABLE_DS_SET
	float	GetWeight(DWORD dwVnum);
	int		GetApplyCount(DWORD dwVnum);
	int		GetBasicApplyValue(DWORD dwVnum, int iType, bool bAttr = false);
	int		GetAdditionalApplyValue(DWORD dwVnum, int iType, bool bAttr = false);
#endif

private:
	void	SendRefineResultPacket(LPCHARACTER ch, BYTE bSubHeader, const TItemPos& pos);

	void	RefreshDragonSoulState(LPCHARACTER ch);

	DWORD	MakeDragonSoulVnum(BYTE bType, BYTE grade, BYTE step, BYTE refine);
	bool	PutAttributes(LPITEM pDS);
	bool	RefreshItemAttributes(LPITEM pItem);

	DragonSoulTable*	m_pTable;
};

#endif
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
