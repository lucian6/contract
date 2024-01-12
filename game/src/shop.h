#ifndef __INC_METIN_II_GAME_SHOP_H__
#define __INC_METIN_II_GAME_SHOP_H__

enum
{
	SHOP_MAX_DISTANCE = 1000
};

class CGrid;

/* ---------------------------------------------------------------------------------- */
class CShop
{
	public:
		typedef struct shop_item
		{
			DWORD	vnum;
			DWORD	price;
#if defined(__SHOPEX_RENEWAL__)
			DWORD price1;
			DWORD price2;
			DWORD price3;
			DWORD price4;
#endif
			DWORD	count;

			LPITEM	pkItem;
			int		itemid;

#if defined(__SHOPEX_RENEWAL__)
			long alSockets[ITEM_SOCKET_MAX_NUM];
			TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
			DWORD price_type;
			DWORD price_vnum;
			DWORD price_vnum1;
			DWORD price_vnum2;
			DWORD price_vnum3;
			DWORD price_vnum4;
#endif

			shop_item()
			{
				vnum = 0;
				price = 0;
#if defined(__SHOPEX_RENEWAL__)
				price1 = 0;
				price2 = 0;
				price3 = 0;
				price4 = 0;
#endif
				count = 0;
				itemid = 0;
				pkItem = NULL;
#if defined(__SHOPEX_RENEWAL__)
				price_type = 1;
				price_vnum = 0;
				price_vnum1 = 0;
				price_vnum2 = 0;
				price_vnum3 = 0;
				price_vnum4 = 0;
				memset(&alSockets, 0, sizeof(alSockets));
				memset(&aAttr, 0, sizeof(aAttr));
#endif

			}
		} SHOP_ITEM;

		CShop();
		virtual ~CShop(); // @fixme139 (+virtual)

		bool			Create(DWORD dwVnum, DWORD dwNPCVnum, TShopItemTable * pItemTable);
		void			SetShopItems(TShopItemTable * pItemTable, BYTE bItemCount);

		virtual void	SetPCShop(LPCHARACTER ch);
		virtual bool	IsPCShop()	{ return m_pkPC ? true : false; }
#if defined(__SHOPEX_RENEWAL__)
		virtual bool IsShopEx() const { return false; };
#endif

		virtual bool	AddGuest(LPCHARACTER ch,DWORD owner_vid, bool bOtherEmpire);
		void			RemoveGuest(LPCHARACTER ch);
		virtual int		Buy(LPCHARACTER ch, BYTE pos, int dwCount = 1);
		void			BroadcastUpdateItem(BYTE pos);
		int				GetNumberByVnum(DWORD dwVnum);
		virtual bool	IsSellingItem(DWORD itemID);

		DWORD	GetVnum() { return m_dwVnum; }
		DWORD	GetNPCVnum() { return m_dwNPCVnum; }

	protected:
		void	Broadcast(const void * data, int bytes);

	protected:
		DWORD				m_dwVnum;
		DWORD				m_dwNPCVnum;

		CGrid *				m_pGrid;

		typedef TR1_NS::unordered_map<LPCHARACTER, bool> GuestMapType;
		GuestMapType m_map_guest;
		std::vector<SHOP_ITEM>		m_itemVector;

		LPCHARACTER			m_pkPC;
};

#endif
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
