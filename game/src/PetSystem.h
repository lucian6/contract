#pragma once

class CItem;
class CHARACTER;

class CPetActor
{
public:
	friend class CPetSystem;
	CPetActor(BYTE petType=0);
	virtual ~CPetActor();

	DWORD			Summon(LPITEM summonItem, const char* petName, DWORD mobVnum);
	void			Unsummon(bool isReal = true);

	void			GiveBuffSkill();
	void			ClearBuffSkill();

	BYTE			GetType() { return m_petType; }
	void			SetType(const BYTE type) { m_petType = type; }

	bool			IsSummoned();

	LPCHARACTER		GetPet();
	LPITEM			GetSummonItem();

#ifdef ENABLE_NEW_PET_SYSTEM
	bool			PointChange(BYTE byType, int amount, bool bBroadcast = false);
	
	BYTE			GetMaxLevelByEvolution(BYTE evolution);
	DWORD			GetNextExp();

	void			IncreaseEvolve();
	
	bool			IncreaseBonus(const BYTE bonusType, const BYTE bonusStep);

	bool			IncreaseSkill(const BYTE skillIndex);
	BYTE			CheckSkillIndex(LPITEM summonItem, BYTE skillIndex);
	BYTE			CheckEmptyIndex(LPITEM summonItem);

	long			GetApplyValue(BYTE byIndex) const { return m_aApplies[byIndex].lValue; };
	void			SetApplyValue(BYTE byIndex, long lValue) { m_aApplies[byIndex].lValue = lValue; };

	BYTE			GetSkillBookIndex(BYTE byIndex) const { return m_aSkills[byIndex].bType; };
	void			SetSkillBookIndex(BYTE byIndex, BYTE byBookIndex) { m_aSkills[byIndex].bType = byBookIndex; };

	short			GetSkillApplyLevel(BYTE byIndex) const { return m_aSkills[byIndex].sLevel; };
	void			SetSkillApplyLevel(BYTE byIndex, short sValue) { m_aSkills[byIndex].sLevel = sValue; };

	long			GetAge() const { return m_lAge; };
	void			SetAge(long lAge) { m_lAge = lAge; };
#endif

protected:
	BYTE			m_petType;
	DWORD			m_dwSummonItemID;
	DWORD			m_pkPetVID;
#ifdef ENABLE_NEW_PET_SYSTEM
	BYTE			m_byEvolution;
	long			m_lDuration;
	long			m_lMaxDuration;
	long			m_lAge;
	TPetApply		m_aApplies[PET_APPLY_MAX_NUM];
	TPetSkill		m_aSkills[PET_SKILLS_MAX_NUM];
#endif
};

class CPetSystem
{
public:
	CPetSystem(LPCHARACTER owner);
	virtual ~CPetSystem();

	LPPET		GetByVID(DWORD vid) const;
	LPPET		GetByID(DWORD itemID) const;

	void		Destroy();
	size_t		CountSummoned() const;

	LPPET		Summon(DWORD mobVnum, LPITEM pSummonItem, const char* petName, BYTE petType);
	void		HandlePetCostumeItem();
#ifdef __SKIN_SYSTEM__
	LPPET		GetPet();
#endif

	void		DeletePet(DWORD itemID);
	void		DeletePet(LPPET petActor);
	void		ChangeGmPetStatus();

#ifdef ENABLE_NEW_PET_SYSTEM
	void		PointChange(BYTE byType, int amount, bool bBroadcast = false);
	void		HandleNewPetItem();
	LPPET		GetNewPet();
#endif
private:
	std::unordered_map<DWORD, LPPET>	m_petActorMap;
	LPCHARACTER		m_pkOwner;
};
