/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _WEAPON_H
#define _WEAPON_H

#include "gr.h"
#include "game.h"
#include "piggy.h"
#include "cfile.h"

// weapon info flags
#define WIF_PLACABLE        1   // can be placed by level designer

class __pack__ CWeaponInfo {
	public:
		int8_t   renderType;         // How to draw 0=laser, 1=blob, 2=CObject
		int8_t   persistent;         // 0 = dies when it hits something, 1 = continues (eg, fusion cannon)
		int16_t	nModel;             // Model num if rendertype==2.
		int16_t  nInnerModel;        // Model num of inner part if rendertype==2.

		int8_t   nFlashAnimation;     // What tAnimationInfo to use for muzzle flash
		int8_t   nRobotHitAnimation;  // What tAnimationInfo for impact with robot
		int16_t  flashSound;         // What sound to play when fired

		int8_t   nWallHitAnimation;  // What tAnimationInfo for impact with CWall
		int8_t   fireCount;          // Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fireCount shots will be fired.
		int16_t  nRobotHitSound;    // What sound for impact with robot

		int8_t   nAmmoUsage;         // How many units of ammunition it uses.
		int8_t   nAnimationIndex;        // Vclip to render for the weapon, itself.
		int16_t  nWallHitSound;     // What sound for impact with CWall

		int8_t   destructible;       // If !0, this weapon can be destroyed by another weapon.
		int8_t   matter;             // Flag: set if this CObject is matter (as opposed to energy)
		int8_t   bounce;             // 1==always bounces, 2=bounces twice
		int8_t   homingFlag;         // Set if this weapon can home in on a target.

		uint8_t  speedvar;           // allowed variance in speed below average, /128: 64 = 50% meaning if speed = 100, can be 50..100

		uint8_t  flags;              // see values above

		int8_t   flash;              // Flash effect
		int8_t   nAfterburnerSize;   // Size of blobs in I2X (1)/16 units, specify in bitmaps.tbl as floating point.  Player afterburner size = 2.5.

		/* not present in shareware datafiles */
		int8_t   children;           // ID of weapon to drop if this contains children.  -1 means no children.

		fix	xEnergyUsage;         // How much fuel is consumed to fire this weapon.
		fix	xFireWait;            // Time until this weapon can be fired again.

		/* not present in shareware datafiles */
		fix xMultiDamageScale;      // Scale damage by this amount when applying to player in multiplayer.  I2X (1) means no change.

		tBitmapIndex bitmap;        // Pointer to bitmap if rendertype==0 or 1.

		fix blob_size;              // Size of blob if blob nType
		fix xFlashSize;             // How big to draw the flash
		fix xImpactSize;            // How big of an impact
		fix strength [DIFFICULTY_LEVEL_COUNT];         // How much damage it can inflict
		fix speed [DIFFICULTY_LEVEL_COUNT];            // How fast it can move, difficulty level based.
		fix mass;                   // How much mass it has
		fix drag;                   // How much drag it has
		fix thrust;                 // How much thrust it has
		fix poLenToWidthRatio;      // For polyobjects, the ratio of len/width. (10 maybe?)
		fix light;                  // Amount of light this weapon casts.
		fix lifetime;               // Lifetime in seconds of this weapon.
		fix xDamageRadius;          // Radius of damage caused by weapon, used for missiles (not lasers) to apply to damage to things it did not hit
	//-- unused--   fix damage_force;           // Force of damage caused by weapon, used for missiles (not lasers) to apply to damage to things it did not hit
	// damage_force was a real mess.  Wasn't DifficultyLevel based, and was being applied instead of weapon's actual strength.  Now use 2*strength instead. --MK, 01/19/95
		tBitmapIndex    picture;    // a picture of the weapon for the cockpit
		/* not present in shareware datafiles */
		tBitmapIndex    hiresPicture;  // a hires picture of the above

	public:
		void Read (CFile& cf, int32_t nFileVersion);
	};

class __pack__ CD1WeaponInfo {
	public:
		int8_t	renderType;				// How to draw 0=laser, 1=blob, 2=CObject
		int8_t	nModel;					// Model num if rendertype==2.
		int8_t	nInnerModel;			// Model num of inner part if rendertype==2.
		int8_t	persistent;					//	0 = dies when it hits something, 1 = continues (eg, fusion cannon)

		int8_t	nFlashAnimation;				// What tAnimationInfo to use for muzzle flash
		int16_t	flashSound;				// What sound to play when fired
		int8_t	nRobotHitAnimation;			// What tAnimationInfo for impact with robot
		int16_t	nRobotHitSound;			// What sound for impact with robot

		int8_t	nWallHitAnimation;			// What tAnimationInfo for impact with CWall
		int16_t	nWallHitSound;			// What sound for impact with CWall
		int8_t	fireCount;					//	Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fireCount shots will be fired.
		int8_t	nAmmoUsage;					//	How many units of ammunition it uses.

		int8_t	nAnimationIndex;				//	Vclip to render for the weapon, itself.
		int8_t	destructible;				//	If !0, this weapon can be destroyed by another weapon.
		int8_t	matter;						//	Flag: set if this CObject is matter (as opposed to energy)
		int8_t	bounce;						//	Flag: set if this CObject bounces off walls

		int8_t	homingFlag;				//	Set if this weapon can home in on a target.
		int8_t	dum1, dum2, dum3;

		fix	xEnergyUsage;				//	How much fuel is consumed to fire this weapon.
		fix	xFireWait;					//	Time until this weapon can be fired again.

		tBitmapIndex bitmap;				// Pointer to bitmap if rendertype==0 or 1.

		fix	blob_size;					// Size of blob if blob nType
		fix	xFlashSize;					// How big to draw the flash
		fix	xImpactSize;				// How big of an impact
		fix	strength [DIFFICULTY_LEVEL_COUNT];				// How much damage it can inflict
		fix	speed [DIFFICULTY_LEVEL_COUNT];					// How fast it can move, difficulty level based.
		fix	mass;							// How much mass it has
		fix	drag;							// How much drag it has
		fix	thrust;						//	How much thrust it has
		fix	poLenToWidthRatio;	// For polyobjects, the ratio of len/width. (10 maybe?)
		fix	light;						//	Amount of light this weapon casts.
		fix	lifetime;					//	Lifetime in seconds of this weapon.
		fix	xDamageRadius;				//	Radius of damage caused by weapon, used for missiles (not lasers) to apply to damage to things it did not hit
	//-- unused--	fix	damage_force;				//	Force of damage caused by weapon, used for missiles (not lasers) to apply to damage to things it did not hit
	// damage_force was a real mess.  Wasn't DifficultyLevel based, and was being applied instead of weapon's actual strength.  Now use 2*strength instead. --MK, 01/19/95
		tBitmapIndex	picture;				// a picture of the weapon for the cockpit
	};

class __pack__ CD2D1WeaponInfo {
	public:
		int8_t	persistent;					//	0 = dies when it hits something, 1 = continues (eg, fusion cannon)
		int8_t	fireCount;					//	Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fireCount shots will be fired.
		int8_t	nAmmoUsage;					//	How many units of ammunition it uses.
		int8_t	destructible;				//	If !0, this weapon can be destroyed by another weapon.
		int8_t	matter;						//	Flag: set if this CObject is matter (as opposed to energy)
		int8_t	bounce;						//	Flag: set if this CObject bounces off walls
		int8_t	homingFlag;				//	Set if this weapon can home in on a target.
		fix	xEnergyUsage;				//	How much fuel is consumed to fire this weapon.
		fix	xFireWait;					//	Time until this weapon can be fired again.
		fix	strength [DIFFICULTY_LEVEL_COUNT];				// How much damage it can inflict
		fix	speed [DIFFICULTY_LEVEL_COUNT];					// How fast it can move, difficulty level based.
		fix	mass;							// How much mass it has
		fix	drag;							// How much drag it has
		fix	thrust;						//	How much thrust it has
		fix	light;						//	Amount of light this weapon casts.
		fix	lifetime;					//	Lifetime in seconds of this weapon.
		fix	xDamageRadius;				//	Radius of damage caused by weapon, used for missiles (not lasers) to apply to damage to things it did not hit
	};

#define WI_persistent(_i)		(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].persistent : gameData.weapons.info [_i].persistent)
#define WI_fireCount(_i)		(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].fireCount : gameData.weapons.info [_i].fireCount)
#define WI_ammo_usage(_i)		(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].nAmmoUsage : gameData.weapons.info [_i].nAmmoUsage)
#define WI_destructible(_i)	(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].destructible : gameData.weapons.info [_i].destructible)
#define WI_matter(_i)			(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].matter : gameData.weapons.info [_i].matter)
#define WI_bounce(_i)			(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].bounce : gameData.weapons.info [_i].bounce)
#define WI_homingFlag(_i)		(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].homingFlag : gameData.weapons.info [_i].homingFlag)
#define WI_set_homingFlag(_i,_v)	 { if (gameStates.app.bD1Mission) weaponInfoD2D1 [_i].homingFlag = (_v); else gameData.weapons.info [_i].homingFlag = (_v); }
#define WI_energy_usage(_i)	(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].xEnergyUsage : gameData.weapons.info [_i].xEnergyUsage)
#define WI_fire_wait(_i)		(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].xFireWait : gameData.weapons.info [_i].xFireWait)
#define WI_strength(_i,_j)		(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].strength [_j] : gameData.weapons.info [_i].strength [_j])
#define WI_speed(_i,_j)			(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].speed [_j] : gameData.weapons.info [_i].speed [_j])
#define WI_mass(_i)				(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].mass : gameData.weapons.info [_i].mass)
#define WI_drag(_i)				(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].drag : gameData.weapons.info [_i].drag)
#define WIThrust(_i)				(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].thrust : gameData.weapons.info [_i].thrust)
#define WI_light(_i)				(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].light : gameData.weapons.info [_i].light)
#define WI_lifetime(_i)			(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].lifetime : gameData.weapons.info [_i].lifetime)
#define WI_damage_radius(_i)	(gameStates.app.bD1Mission ? weaponInfoD2D1 [_i].xDamageRadius : gameData.weapons.info [_i].xDamageRadius)

#define REARM_TIME                  (I2X (1))

#define WEAPON_DEFAULT_LIFETIME     I2X (12)   // Lifetime of an CObject if a bozo forgets to define it.

#define WEAPON_TYPE_WEAK_LASER      0
#define WEAPON_TYPE_STRONG_LASER    1
#define WEAPON_TYPE_CANNON_BALL     2
#define WEAPON_TYPEMSL         3

#define MAX_WEAPON_TYPES            70
#define D1_MAX_WEAPON_TYPES         30

#define WEAPON_RENDER_NONE          -1
#define WEAPON_RENDER_LASER         0
#define WEAPON_RENDER_BLOB          1
#define WEAPON_RENDER_POLYMODEL     2
#define WEAPON_RENDER_VCLIP         3

#define MAX_PRIMARY_WEAPONS         10
#define MAX_SECONDARY_WEAPONS       10
#define MAX_PRIMSEC_WEAPONS         10

#define MAX_D1_PRIMARY_WEAPONS      5
#define MAX_D1_SECONDARY_WEAPONS    5
//given a weapon index, return the flag value
#define  HAS_FLAG(index)  (1<<(index))

// Weapon flags, if player->weaponFlags & WEAPON_FLAG is set, then the player has this weapon
#define HAS_LASER_FLAG      HAS_FLAG(LASER_INDEX)
#define HAS_VULCAN_FLAG     HAS_FLAG(VULCAN_INDEX)
#define HAS_SPREADFIRE_FLAG HAS_FLAG(SPREADFIRE_INDEX)
#define HAS_PLASMA_FLAG     HAS_FLAG(PLASMA_INDEX)
#define HAS_FUSION_FLAG     HAS_FLAG(FUSION_INDEX)

#define HAS_CONCUSSION_FLAG HAS_FLAG(CONCUSSION_INDEX)
#define HAS_HOMING_FLAG     HAS_FLAG(HOMING_INDEX)
#define HAS_PROXIMITY_FLAG  HAS_FLAG(PROXIMITY_INDEX)
#define HAS_SMART_FLAG      HAS_FLAG(SMART_INDEX)
#define HAS_MEGA_FLAG       HAS_FLAG(MEGA_INDEX)

#define CLASS_PRIMARY       0
#define CLASS_SECONDARY     1

#define LASER_INDEX         0
#define VULCAN_INDEX        1
#define SPREADFIRE_INDEX    2
#define PLASMA_INDEX        3
#define FUSION_INDEX        4
#define SUPER_LASER_INDEX   5
#define GAUSS_INDEX         6
#define HELIX_INDEX         7
#define PHOENIX_INDEX       8
#define OMEGA_INDEX         9

#define CONCUSSION_INDEX    0
#define HOMING_INDEX        1
#define PROXMINE_INDEX		 2
#define SMART_INDEX         3
#define MEGA_INDEX          4
#define FLASHMSL_INDEX      5
#define GUIDED_INDEX        6
#define SMARTMINE_INDEX     7
#define MERCURY_INDEX		 8
#define EARTHSHAKER_INDEX   9

#define SUPER_WEAPON        5

#define VULCAN_AMMO_SCALE   0xcc163 //(0x198300/2)      //multiply ammo by this before displaying

#define NUM_SMART_CHILDREN  6   // Number of smart children created by default.

extern CD2D1WeaponInfo weaponInfoD2D1 [D1_MAX_WEAPON_TYPES];
extern void DoSelectWeapon(int32_t nWeapon, int32_t secondaryFlag);
extern void ShowWeaponStatus(void);

extern uint8_t primaryWeaponToWeaponInfo [MAX_PRIMARY_WEAPONS];
extern uint8_t secondaryWeaponToWeaponInfo [MAX_SECONDARY_WEAPONS];

//for each Secondary weapon, which gun it fires out of
extern uint8_t secondaryWeaponToGunNum [MAX_SECONDARY_WEAPONS];

//for each primary weapon, what kind of powerup gives weapon
extern uint8_t primaryWeaponToPowerup [MAX_SECONDARY_WEAPONS];

//for each Secondary weapon, what kind of powerup gives weapon
extern uint8_t secondaryWeaponToPowerup [2][MAX_SECONDARY_WEAPONS];

//flags whether the last time we use this weapon, it was the 'super' version
extern uint8_t bLastWeaponWasSuper [2][MAX_PRIMARY_WEAPONS];

extern void AutoSelectWeapon(int32_t weaponType, int32_t auto_select);        //parm is primary or secondary
extern void SelectWeapon (int32_t nWeapon, int32_t secondaryFlag, int32_t bPrintMessage, int32_t bWaitForRearm);

extern char 	*pszShortPrimaryWeaponNames [];
extern char 	*pszShortSecondaryWeaponNames [];
extern char 	*pszPrimaryWeaponNames [];
extern char 	*pszSecondaryWeaponNames [];
extern int32_t  	nMaxPrimaryAmmo [MAX_PRIMARY_WEAPONS];
extern uint8_t   nMaxSecondaryAmmo [MAX_SECONDARY_WEAPONS];

#define HAS_WEAPON_FLAG 1
#define HAS_ENERGY_FLAG 2
#define HAS_AMMO_FLAG       4
#define  HAS_ALL (HAS_WEAPON_FLAG | HAS_ENERGY_FLAG | HAS_AMMO_FLAG)

//-----------------------------------------------------------------------------
// Return:
// Bits set:
//      HAS_WEAPON_FLAG
//      HAS_ENERGY_FLAG
//      HAS_AMMO_FLAG
//      HAS_SUPER_FLAG
int32_t PlayerHasWeapon (int32_t nWeapon, int32_t secondaryFlag, int32_t nPlayer, int32_t bAll);

//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
int32_t PickupSecondary (CObject *pObj, int32_t nWeaponIndex, int32_t nAmount, int32_t nPlayer);

// max. amount of ammo of type 'nWeapon' player can carry depending on equipment (ammo rack) and game settings
int32_t MaxSecondaryAmmo (int32_t nWeapon);

//called when a primary weapon is picked up
//returns true if actually picked up
int32_t PickupPrimary (int32_t nWeaponIndex, int32_t nPlayer);

//called when ammo (for the vulcan cannon) is picked up
int32_t PickupAmmo (int32_t classFlag, int32_t nWeaponIndex, int32_t ammoCount, int32_t nPlayer);

int32_t PickupLaser (CObject *pObj, int32_t nId, int32_t nPlayer);
int32_t PickupSuperLaser (CObject *pObj, int32_t nId, int32_t nPlayer);
int32_t PickupQuadLaser (CObject *pObj, int32_t nId, int32_t nPlayer);
int32_t PickupGun (CObject *pObj, int32_t nId, int32_t nPlayer);
int32_t PickupGatlingGun (CObject *pObj, int32_t nId, int32_t nPlayer);
int32_t PickupVulcanAmmo (CObject *pObj, int32_t nPlayer);

int32_t AttemptToStealItem (CObject *objp, int32_t player_num);

//this function is for when the player intentionally drops a powerup
int32_t SpitPowerup (CObject *spitter, uint8_t id, int32_t seed = -1);

#define SMEGA_ID    40

void RockTheMineFrame (void);
void ShakerRockStuff (CFixVector* vPos = NULL);
void InitShakerDetonates (void);
void tactile_set_button_jolt (void);

/*
 * reads n CWeaponInfo structs from a CFILE
 */
int32_t ReadWeaponInfos (int32_t nOffset, int32_t nCount, CFile& cf, int32_t fileVersion, bool bDefault);

extern uint8_t nWeaponOrder [2][11];
extern uint8_t nDefaultWeaponOrder [2][11];

#define primaryOrder		(nWeaponOrder [0])
#define secondaryOrder	(nWeaponOrder [1])

#define defaultPrimaryOrder	(nDefaultWeaponOrder [0])
#define defaultSecondaryOrder	(nDefaultWeaponOrder [1])

int32_t AllowedToFireGun (void);
int32_t AllowedToFireFlare (void);
int32_t AllowedToFireMissile (int32_t nPlayer, int32_t bCheckSegment);

void DropCurrentWeapon (void);
void DropSecondaryWeapon (int32_t nWeapon, int32_t nAmount = 1, int32_t bSilent = 0);

void ValidatePrios (uint8_t *order, uint8_t *defaultOrder, int32_t n);
void SetLastSuperWeaponStates (void);
void ToggleBomb (void);
void CyclePrimary ();
void CycleSecondary ();
void SetWeaponState (int8_t nPrimary, int8_t bPrimFire, int8_t nSecondary, int8_t bSecFire);
int32_t GetPlayerGun (int32_t nPlayer, int32_t *bFiring);
int32_t GetPlayerMissile (int32_t nPlayer, int32_t *bFiring);

void InitSeismicDisturbances (void);
int32_t POrderList (int32_t nWeapon);
int32_t SOrderList (int32_t nWeapon);
int32_t CheckToUsePrimary (int32_t nWeaponIndex);
void DoWeaponStuff (void);
int32_t ArmedBomb (void);

#endif
