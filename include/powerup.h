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

#ifndef _POWERUP_H
#define _POWERUP_H

#include "vclip.h"

#define POW_EXTRA_LIFE				0
#define POW_ENERGY					1
#define POW_SHIELD_BOOST			2
#define POW_LASER						3

#define POW_KEY_BLUE					4
#define POW_KEY_RED					5
#define POW_KEY_GOLD					6

#define POW_HOARD_ORB				7       // use unused slot
#define POW_MONSTERBALL				8

#define POW_CONCUSSION_1        	10
#define POW_CONCUSSION_4        	11      // 4-pack MUST follow single missile

#define POW_QUADLASER           	12

#define POW_VULCAN       			13
#define POW_SPREADFIRE   			14
#define POW_PLASMA       			15
#define POW_FUSION       			16
#define POW_PROXMINE    			17
#define POW_HOMINGMSL_1       	18
#define POW_HOMINGMSL_4       	19      // 4-pack MUST follow single missile
#define POW_SMARTMSL    			20
#define POW_MEGAMSL         		21
#define POW_VULCAN_AMMO         	22
#define POW_CLOAK               	23
#define POW_TURBO               	24
#define POW_INVUL				     	25
#define POW_MEGAWOW             	27

#define POW_GAUSS        			28
#define POW_HELIX        			29
#define POW_PHOENIX      			30
#define POW_OMEGA        			31

#define POW_SUPERLASER         	32
#define POW_FULL_MAP            	33
#define POW_CONVERTER           	34
#define POW_AMMORACK           	35
#define POW_AFTERBURNER         	36
#define POW_HEADLIGHT           	37

#define POW_FLASHMSL_1         	38
#define POW_FLASHMSL_4         	39      // 4-pack MUST follow single missile
#define POW_GUIDEDMSL_1    		40
#define POW_GUIDEDMSL_4    		41      // 4-pack MUST follow single missile
#define POW_SMARTMINE          	42

#define POW_MERCURYMSL_1   		43
#define POW_MERCURYMSL_4   		44      // 4-pack MUST follow single missile
#define POW_EARTHSHAKER 			45

#define POW_BLUEFLAG           	46
#define POW_REDFLAG            	47

#define POW_SLOWMOTION				48
#define POW_BULLETTIME				49

#define MAX_POWERUP_TYPES_D2		48
#define MAX_POWERUP_TYPES			50
#define D2X_POWERUP_TYPES			100
#define MAX_POWERUP_TYPES_D1  	29

#define POWERUP_ADDON_COUNT		(MAX_POWERUP_TYPES - MAX_POWERUP_TYPES_D2)

#if 0//_DEBUG
#	define	POW_ENTROPY_VIRUS		POW_SHIELD_BOOST
#else
#	define	POW_ENTROPY_VIRUS		POW_HOARD_ORB
#endif

#define VULCAN_AMMO_MAX             (392*4)
#define VULCAN_WEAPON_AMMO_AMOUNT   196
#define VULCAN_AMMO_AMOUNT          (49*2)

#define GAUSS_WEAPON_AMMO_AMOUNT    392


#define POWERUP_NAME_LENGTH 16      // Length of a robot or powerup name.
extern char Powerup_names[MAX_POWERUP_TYPES][POWERUP_NAME_LENGTH];

extern int Headlight_active_default;    // is headlight on when picked up?

typedef struct tPowerupTypeInfo {
	int nClipIndex;
	int hitSound;
	fix size;       // 3d size of longest dimension
	fix light;      // amount of light cast by this powerup, set in bitmaps.tbl
} __pack__ tPowerupTypeInfo;

extern int N_powerupTypes;
extern tPowerupTypeInfo powerupInfo[MAX_POWERUP_TYPES];

void InitPowerupTables (void);

void DrawPowerup(CObject *objP);

//returns true if powerup consumed
int DoPowerup(CObject *objP, int nPlayer);

//process (animate) a powerup for one frame
void DoPowerupFrame(CObject *objP);
void UpdatePowerupClip (tVideoClip *vcP, tVClipInfo *vciP, int nObject);
void UpdateFlagClips (void);

// Diminish shields and energy towards max in case they exceeded it.
void diminish_towards_max(void);

void DoMegaWowPowerup(int quantity);

void _CDECL_ PowerupBasic(int redadd, int greenadd, int blueadd, int score, const char *format, ...);

/*
 * reads n tPowerupTypeInfo structs from a CFILE
 */
int ReadPowerupTypeInfos (tPowerupTypeInfo *pti, int n, CFile& cf);

int ApplyCloak (int bForce, int nPlayer);
int ApplyInvul (int bForce, int nPlayer);

char PowerupToWeapon (short nPowerup, int *nType);
char PowerupToWeaponCount (short nPowerup);
char PowerupClass (short nPowerup);
char PowerupToObject (short nPowerup);
short PowerupToModel (short nPowerup);
short WeaponToModel (short nWeapon);
short PowerupsOnShips (int nPowerup);
void SpawnLeftoverPowerups (short nObject);
void CheckInventory (void);

int PickupEnergyBoost (CObject *objP, int nPlayer);
int PickupEquipment (CObject *objP, int nEquipment, const char *pszHave, const char *pszGot, int nPlayer);

#define	PowerupsInMine(_nPowerup) \
			((gameStates.multi.nGameType == UDP_GAME) ? \
			 (gameData.multiplayer.powerupsInMine [_nPowerup] + PowerupsOnShips (_nPowerup)) : \
			 gameData.multiplayer.powerupsInMine [_nPowerup])

#define	TooManyPowerups(_nPowerup) \
			IsMultiGame && PowerupClass (_nPowerup) && \
			(PowerupsInMine (_nPowerup) >= gameData.multiplayer.maxPowerupsAllowed [_nPowerup])

extern const char *pszPowerup [MAX_POWERUP_TYPES];
extern ubyte powerupType [MAX_POWERUP_TYPES];
extern void * pickupHandler [MAX_POWERUP_TYPES];

#define POWERUP_IS_UNDEFINED	-1
#define POWERUP_IS_GUN			0
#define POWERUP_IS_MISSILE		1
#define POWERUP_IS_EQUIPMENT	2
#define POWERUP_IS_KEY			3
#define POWERUP_IS_FLAG			4

//------------------------------------------------------------------------------

static inline int IsEnergyPowerup (int nId)
{
return (nId == POW_EXTRA_LIFE) || (nId == POW_ENERGY) || (nId == POW_SHIELD_BOOST) ||
		 (nId == POW_HOARD_ORB) || (nId == POW_CLOAK) || (nId == POW_INVUL);
}

//------------------------------------------------------------------------------

#endif /* _POWERUP_H */
