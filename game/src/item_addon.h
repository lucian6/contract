#ifndef __ITEM_ADDON_H
#define __ITEM_ADDON_H

class CItemAddonManager : public singleton<CItemAddonManager>
{
	public:
		CItemAddonManager();
		virtual ~CItemAddonManager();

		void ApplyAddonTo(int iAddonType, LPITEM pItem);
};

#endif
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
