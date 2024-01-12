#ifndef __INC_METIN_II_GAME_SAFEBOX_H__
#define __INC_METIN_II_GAME_SAFEBOX_H__
#include <memory>

class CHARACTER;
class CItem;
class CGrid;

class CSafebox
{
	public:
		CSafebox(LPCHARACTER pkChrOwner, int iSize, DWORD dwGold);
		~CSafebox();
	
		bool			Add(DWORD dwPos, LPITEM pkItem);
		LPITEM			Get(DWORD dwPos);
		LPITEM			Remove(DWORD dwPos);
		void			ChangeSize(int iSize);
	
		bool			MoveItem(WORD bCell, WORD bDestCell, DWORD count);
		LPITEM			GetItem(WORD bCell);
	
		void			Save();
	
		bool			IsEmpty(DWORD dwPos, BYTE bSize);
	#if defined(ITEM_CHECKINOUT_UPDATE)
		int				GetEmptySafebox(BYTE size);
	#endif
		bool			IsValidPosition(DWORD dwPos);
	
		void			SetWindowMode(BYTE bWindowMode);
		unsigned int	GetGridTotalSize() const;
	
	protected:
		void		__Destroy();
	
		LPCHARACTER	m_pkChrOwner;
		LPITEM		m_pkItems[SAFEBOX_MAX_NUM];
		int			m_iSize;
		long		m_lGold;
		BYTE		m_bWindowMode;
		std::vector<std::shared_ptr<CGrid>> v_Grid;
};

#endif
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
