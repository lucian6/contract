#ifndef __HEADER_VNUM_HELPER__
#define	__HEADER_VNUM_HELPER__
#include "CommonDefines.h"

class CItemVnumHelper
{
public:

	static	const bool	IsPhoenix(DWORD vnum)				{ return 53001 == vnum; }

	static	const bool	IsRamadanMoonRing(DWORD vnum)		{ return 71135 == vnum; }

	static	const bool	IsHalloweenCandy(DWORD vnum)		{ return 71136 == vnum; }

	static	const bool	IsHappinessRing(DWORD vnum)		{ return 71143 == vnum; }

	static	const bool	IsLovePendant(DWORD vnum)		{ return 71145 == vnum; }

#if defined(__BL_67_ATTR__)
	static DWORD Get67MaterialVnum(int type, int subtype)
	{

		if (type == ITEM_WEAPON)
		{
			return 39078;
		}
		else if (type == ITEM_ARMOR)
		{
			if (subtype == ARMOR_BODY || subtype == ARMOR_SHIELD || subtype == ARMOR_HEAD)
				return 39079;
			else if (subtype == ARMOR_EAR || subtype == ARMOR_FOOTS || subtype == ARMOR_NECK || subtype == ARMOR_WRIST)
				return 39080;
		}
		return 0;
	}
#endif

	static const bool IsDragonSoul(DWORD vnum)
	{
		return (vnum >= 110000 && vnum <= 165400);
	}

#if defined(__EXTENDED_BLEND_AFFECT__)
	/// Extended Blend
	static const bool IsExtendedBlend(DWORD vnum)
	{
		switch (vnum)
		{
			// INFINITE_DEWS
		case 50830:
		case 950821:
		case 950822:
		case 950823:
		case 950824:
		case 950825:
		case 950826:
		// END_OF_INFINITE_DEWS

		// DRAGON_GOD_MEDALS
		case 939017:
		case 939018:
		case 939019:
		case 939020:
			// END_OF_DRAGON_GOD_MEDALS

			// CRITICAL_AND_PENETRATION
		case 939024:
		case 939025:
			// END_OF_CRITICAL_AND_PENETRATION

			// ATTACK_AND_MOVE_SPEED
		case 27102:
			// END_OF_ATTACK_AND_MOVE_SPEED
		case 95219:
		case 95220:
			// BLESSED WATER
			return true;
		default:
			return false;
		}
	}
#endif


};

class CMobVnumHelper
{
public:

	static	bool	IsPhoenix(DWORD vnum)				{ return 34001 == vnum; }
	static	bool	IsIcePhoenix(DWORD vnum)				{ return 34003 == vnum; }

	static	bool	IsPetUsingPetSystem(DWORD vnum)	{ return (IsPhoenix(vnum) || IsReindeerYoung(vnum)) || IsIcePhoenix(vnum); }

	static	bool	IsReindeerYoung(DWORD vnum)	{ return 34002 == vnum; }

	static	bool	IsRamadanBlackHorse(DWORD vnum)		{ return 20119 == vnum || 20219 == vnum || 22022 == vnum; }
};

class CVnumHelper
{
};

#endif	//__HEADER_VNUM_HELPER__
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
