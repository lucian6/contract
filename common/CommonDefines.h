#ifndef __INC_METIN2_COMMON_DEFINES_H__
#define __INC_METIN2_COMMON_DEFINES_H__
#pragma once
//////////////////////////////////////////////////////////////////////////
// ### Standard Features ###
#define _IMPROVED_PACKET_ENCRYPTION_			// Improved packet encrypt
#ifdef _IMPROVED_PACKET_ENCRYPTION_
	#define USE_IMPROVED_PACKET_DECRYPTED_BUFFER
#endif
#define __UDP_BLOCK__							// UD Port Block

// ### END Standard Features ###
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// ### New Features ###
#define ENABLE_D_NJGUILD						// Join Guild Quest
#define ENABLE_FULL_NOTICE						// Full Notice Announcement
#define ENABLE_NEWSTUFF							// New Stuff
#define ENABLE_PORT_SECURITY					// Port Security
#define ENABLE_BELT_INVENTORY_EX				// Belt Inventory Extention
#define ENABLE_CMD_WARP_IN_DUNGEON				// Warp in specific Dungeon
// #define ENABLE_SEQUENCE_SYSTEM					// Sequence Packet System
#define ENABLE_PLAYER_PER_ACCOUNT5				// 5 characters per account
#define ENABLE_DICE_SYSTEM						// Dice function
#define ENABLE_EXTEND_INVEN_SYSTEM				// Extend inventory 4 pages
#define ENABLE_MOUNT_COSTUME_SYSTEM				// Normal Mount Costume
#define ENABLE_WEAPON_COSTUME_SYSTEM			// Weapon Skin System
#define ENABLE_QUEST_DIE_EVENT					// Self death quest event
#define ENABLE_QUEST_BOOT_EVENT					// Boot quest event
#define ENABLE_QUEST_DND_EVENT					// DND quest event
#define ENABLE_SKILL_FLAG_PARTY					// Skill flag party
#define ENABLE_NO_DSS_QUALIFICATION				// Without DSS Qualification
#define ENABLE_NO_SELL_PRICE_DIVIDED_BY_5		// New recalculation for prices
#define ENABLE_TAX_CHANGES						// Change taxes via define
#if defined(ENABLE_TAX_CHANGES)
	#define NEW_TAX_VARIABLE 4				// New tax for the bought items in normal shops
#endif
#define ENABLE_CHECK_SELL_PRICE					// Check the sell price for overflows
#define ENABLE_GOTO_LAG_FIX						// GoTo Lag fix 
#define ENABLE_PENDANT_SYSTEM					// Talisman System
#define ENABLE_GLOVE_SYSTEM						// Glove System
#define ENABLE_MOVE_CHANNEL						// Change the Channel
#define ENABLE_QUIVER_SYSTEM					// New Quiver System

enum eCommonDefines {
	MAP_ALLOW_LIMIT = 32,						// Map Limit per Channel. Default : 32
};

#define ENABLE_WOLFMAN_CHARACTER
#ifdef ENABLE_WOLFMAN_CHARACTER
#define DISABLE_WOLFMAN_ON_CREATE
#define USE_MOB_BLEEDING_AS_POISON
#define USE_MOB_CLAW_AS_DAGGER
#define USE_WOLFMAN_STONES
#define USE_WOLFMAN_BOOKS
#endif

#define ENABLE_MAGIC_REDUCTION_SYSTEM		// Magic Reduction Recalculation
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
#endif

// ### END New Features ###
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// ### Start of Hex Features ###

#define ENABLE_YMIR_REGEN_FIX					// New Regen Function ( Memory Usage Reduction ) 
#define ENABLE_ENTITY_PRELOADING				// Load all entities in specific map ( Reduce Frame Spikes )

#define ENABLE_OFFLINESHOP_SYSTEM				// Define Offline Shop System
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	#define AFTER_BUY_REMOVE_DIRECTLY			// Fast Item Purge After Buying
	#define ENABLE_SHOP_SEARCH_SYSTEM			// Enable Search Shop System
#endif

#define ENABLE_WIKI								// State for Wikipedia System
#define __CLIENT_VERSION__						// Check client version for players
#define ENABLE_MULTI_FARM_BLOCK					// Multi farm block for more than 3 characters

#define __RENEWAL_BRAVE_CAPE__					// Automatic bravery cape for players
#define __REQUEST_MONEY_FOR_BRAVERY_CAPE__		// Request money for each use of Bravery Cape ( 0-30 Free ; 31-60 1.000yang ; 61-90 2.000 yang ; 91-120 3.000 yang )
#if defined (__REQUEST_MONEY_FOR_BRAVERY_CAPE__)
	#define LOW_COST_CAPE			1000
	#define MEDIUM_COST_CAPE		2000
	#define HIGH_COST_CAPE			3000
#endif

#define ENABLE_DUNGEON_BOSS_ICON_IN_MAP			// Position and Respawn Info for Bosses on Minimap
#define RENEWAL_PICKUP_AFFECT					// Auto pickup function for Premium Users
#define RENEWAL_MISSION_BOOKS					// New Mission Book Window with Random Quests for the Players

#define ENABLE_EVENT_MANAGER					// Event Handler
#define ENABLE_REWARD_SYSTEM					// Reward System
#define ENABLE_BATTLE_PASS						// Battle Pass System
#define ENABLE_WHEEL_OF_FORTUNE					// System for Wheel of Random Items

#define ENABLE_BIYOLOG							// New Biolog System
#define ENABLE_RENEWAL_AFFECT_SHOWER			// Renewal New Affects Categories

#define ENABLE_DS_GRADE_MYTH					// New Mythic Alchemy
#define ENABLE_DS_SET							// When the wheel is complete you get additional bonuses
#define DRAGON_SOUL_REFINEMENT_FROM_WINDOW		// Open Refinement and Change Window from Dragon Soul Inventory

#define __VIEW_TARGET_PLAYER_HP__				// View Target Player HP Amount
#define __VIEW_TARGET_DECIMAL_HP__				// View Target Decimal HP
#define CROSS_CHANNEL_FRIEND_REQUEST			// You can send a friend request now from a different channel

#define ENABLE_CUBE_RENEWAL_WORLDARD			// Official Cube System
#define ENABLE_CUBE_ATTR_SOCKET					// Official Cube System Fix
#define __SEND_TARGET_INFO__					// Info Drop Details for each Mob on the Target

#define ENABLE_SWITCHBOT						// Enable Switchbot System
#define __QUEST_RENEWAL__						// Quest Page Renewal

#define NEW_REFINE_INFO							// New Scrolls For Refinement
#define ENABLE_REFINE_MSG_ADD					// Refine Fail New Message
#define __BL_PARTY_POSITION__					// Party Member Position

#define __ITEM_APPLY4__							// Extended Apply Bonus (4)
#define __NEW_BONUSES__							// New Bonuses (AttBoss, ResMob, ResBoss, CritPVM, ResRace, AttStone)
#define NEW_PROTO								// New Proto modifications

#define __RENEWAL_MOUNT__						// New mount system
#define ENABLE_NEW_PET_SYSTEM					// New Pet System
#define __BUFFI_SUPPORT__						// New Buffi System
#define ENABLE_SPECIAL_COSTUME_ATTR				// Enchant for Costumes
#define __RENEWAL_CRYSTAL__						// New Energy Crystal
#define __PERMA_ACCESSORY__						// Permanent Buff for Accesories

#define __BLEND_AFFECT__						// Blend Affects with Icons
#define __EXTENDED_BLEND_AFFECT__				// Extended Blend Item Affect
#define __TIME_STACKEBLE__						// Stack items for bonuses
#define __BL_67_ATTR__							// 6 & 7 official bonuses
#define __SKIN_SYSTEM__							// Skin system for pet & mount

#define __GEM_SYSTEM__							// Gem (Gaya) System
#if defined(__GEM_SYSTEM__)
#	define __GEM_MARKET_SYSTEM__				// Gem (Gaya) Market System
#endif

#define __BL_EVENT_STONE_SHAPE_CHANGE__

#define __SPECIAL_INVENTORY_SYSTEM__			// Special Inventory System
#define __DUNGEON_INFO__						// New Dungeon Tracking Info DracaryS
#define __PENDANT__								// Cool Pendants
#define __ELEMENT_ADD__							// Element System

#define __SORT_INVENTORY_ITEMS__				// Sort Inventory Items
#define ITEM_CHECKINOUT_UPDATE					// Check item place
#define __BACK_DUNGEON__						// Back in Dungeon
#define __BL_MOVE_COSTUME_ATTR__				// Move Costume Attributes
#define __HIDE_COSTUME_SYSTEM__					// Hide costumes on button on Inventory
#define __ANTI_EXP_RING__						// Anti Experience Ring

#define __SHOPEX_RENEWAL__						// Shop Renewal / New funcs for price and windows
#if defined(__SHOPEX_RENEWAL__)					
#	define __SHOPEX_TAB4__						// ShopEx 4 Tabs
#endif
#define __MULTI_LANGUAGE_SYSTEM__
#define __CHATTING_WINDOW_RENEWAL__ 			// New Chat options
#define WJ_ENABLE_TRADABLE_ICON					// New Inventory Slotmark

#define NEW_ITEM_DROP_RATE 250					// New drop chance rate
#define ENABLE_ULTIMATE_REGEN					// New regen functions
#define ENABLE_AGGREGATE_MONSTER_EFFECT			// Aggregate Effect
#define __BL_ENABLE_PICKUP_ITEM_EFFECT__		// Pickup Item Effect
#define __MAINTENANCE__							// Maintenance System

#define __MESSENGER_BLOCK_SYSTEM__				// Messenger Block System
#define __MESSENGER_GM__						// Messenger GM List
#define __DS_CHANGE_ATTR__						// Change Attr bonuses
#define RENEWAL_DEAD_PACKET						// New dead packet
#define BL_SORT_LASTPLAYTIME					// Sort by last time
#define __IMPROVED_LOGOUT_POINTS__				// Improved logout points
#define __IMPROVED_HANDSHAKE_PROCESS__			// Improved handshake process
#define __SASH_SKIN__							// New sash skins

#define __SHIP_DEFENSE__						// Ship Defense (Hydra Dungeon)
#define ENABLE_UNIQUE_IMMORTALITY				// Unique immortality function for quest
#define _ENABLE_IPBAN_							// Ban Player IP
#define ENABLE_CHEST_OPEN_RENEWAL				// Open multiple chests
#define BL_OFFLINE_MESSAGE						// Offline messages

#define NEW_STATISTICS							// Made by Hex with love.
#define ENABLE_GUILD_TOKEN_AUTH					// New guild icon exploit fix.
#define METINSTONES_QUEUE						// Metinstones queue system.
#define __ITEM_GROUND_ONLY_OWNER__
#define __SAVE_BLOCK_ATTR__

#define __MINI_GAME_OKEY__

// ### End of Hex Features ###
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// ### Ex Features ###
#define DISABLE_STOP_RIDING_WHEN_DIE 			//	if DISABLE_TOP_RIDING_WHEN_DIE is defined, the player doesn't lose the horse after dying
#define ENABLE_ACCE_COSTUME_SYSTEM 				//fixed version
#define USE_ACCE_ABSORB_WITH_NO_NEGATIVE_BONUS //enable only positive bonus in acce absorb
#define ENABLE_HIGHLIGHT_NEW_ITEM 				//if you want to see highlighted a new item when dropped or when exchanged
#define ENABLE_KILL_EVENT_FIX 					//if you want to fix the 0 exp problem about the when kill lua event (recommended)
// #define ENABLE_SYSLOG_PACKET_SENT 			// debug purposes

#define ENABLE_EXTEND_ITEM_AWARD 				//slight adjustement
#ifdef ENABLE_EXTEND_ITEM_AWARD
	// #define USE_ITEM_AWARD_CHECK_ATTRIBUTES 	//it prevents bonuses higher than item_attr lvl1-lvl5 min-max range limit
#endif

// ### END Ex Features ###
//////////////////////////////////////////////////////////////////////////

#endif
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
