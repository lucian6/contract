#ifdef __BUFFI_SUPPORT__
class BUFFI
{
public:
	BUFFI(DWORD playerID, DWORD itemID);
	virtual ~BUFFI();
	void	SetBuffiName(const char* szNewName);
	bool	Summon(bool bSummonType, bool bFromClick);
	void	CheckOwnerSupport();
	void	CheckSupportWear(bool bIsCreating);
	bool	CheckVID(DWORD dwVID) {return m_dwVID == dwVID ? true : false;}
private:
	void	LoadSpecialName();
	LPITEM	GetItem();
	LPCHARACTER	GetOwner();
	LPCHARACTER	GetBuffi();
protected:
	std::string	szName;
	DWORD	m_dwOwnerPID;
	DWORD	m_dwVID;
	DWORD	m_dwItemPID;
	DWORD	m_dwSkillNextTime;
};
class BUFFI_MANAGER : public singleton<BUFFI_MANAGER>
{
public:
	BUFFI_MANAGER();
	virtual ~BUFFI_MANAGER();
	void	Summon(bool bSummonType, LPITEM item);
	BUFFI* GetBuffi(DWORD dwOwnerID, DWORD dwVID);
	void	CompareLoginStatus(bool isLogin, LPCHARACTER ch);
	int		SummonCount(LPCHARACTER ch);
protected:
	std::map<DWORD, std::map<DWORD, BUFFI>> m_mapBuffiList;
};
#endif