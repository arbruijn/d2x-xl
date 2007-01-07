/* $Id: powerup.h,v 1.4 2003/10/10 09:36:35 btb Exp $ */
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

//#define POW_RADAR_ROBOTS        8
//#define POW_RADAR_POWERUPS      9

#define POW_CONCUSSION_1        	10
#define POW_CONCUSSION_4        	11      // 4-pack MUST follow single missile

#define POW_QUADLASER           	12

#define POW_VULCAN       			13
#define POW_SPREADFIRE   			14
#define POW_PLASMA       			15
#define POW_FUSION       			16
#define POW_PROXMINE    			17
#define POW_SMARTMSL    			20
#define POW_MEGAMSL         		21
#define POW_VULCAN_AMMO         	22
#define POW_HOMINGMSL_1       	18
#define POW_HOMINGMSL_4       	19      // 4-pack MUST follow single missile
#define POW_CLOAK               	23
#define POW_TURBO               	24
#define POW_INVULNERABILITY     	25
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

#if 0//_DEBUG
#	define	POW_ENTROPY_VIRUS		POW_SHIELD_BOOST
#else
#	define	POW_ENTROPY_VIRUS		POW_HOARD_ORB
#endif

#define VULCAN_AMMO_MAX             (392*4)
#define VULCAN_WEAPON_AMMO_AMOUNT   196
#define VULCAN_AMMO_AMOUNT          (49*2)

#define GAUSS_WEAPON_AMMO_AMOUNT    392

#define MAX_POWERUP_TYPES		50
#define D2X_POWERUP_TYPES		100
#define D1_MAX_POWERUP_TYPES  29

#define POWERUP_NAME_LENGTH 16      // Length of a robot or powerup name.
extern char Powerup_names[MAX_POWERUP_TYPES][POWERUP_NAME_LENGTH];

extern int Headlight_active_default;    // is headlight on when picked up?

typedef struct powerupType_info {
	int nClipIndex;
	int hitSound;
	fix size;       // 3d size of longest dimension
	fix light;      // amount of light cast by this powerup, set in bitmaps.tbl
} __pack__ powerupType_info;

extern int N_powerupTypes;
extern powerupType_info Powerup_info[MAX_POWERUP_TYPES];

void InitPowerupTables (void);

void DrawPowerup(tObject *objP);

//returns true if powerup consumed
int DoPowerup(tObject *objP, int nPlayer);

//process (animate) a powerup for one frame
void DoPowerupFrame(tObject *objP);
void UpdatePowerupClip (tVideoClip *vcP, tVClipInfo *vciP, int nObject);
void UpdateFlagClips (void);

// Diminish shields and energy towards max in case they exceeded it.
void diminish_towards_max(void);

void DoMegaWowPowerup(int quantity);

void _CDECL_ PowerupBasic(int redadd, int greenadd, int blueadd, int score, char *format, ...);

#ifdef FAST_FILE_IO
#define PowerupTypeInfoReadN(pti, n, fp) CFRead(pti, sizeof(powerupType_info), n, fp)
#else
/*
 * reads n powerupType_info structs from a CFILE
 */
extern int PowerupTypeInfoReadN(powerupType_info *pti, int n, CFILE *fp);
#endif
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

#define	PowerupsInMine(_nPowerup) \
			((gameStates.multi.nGameType == UDP_GAME) ? \
			 (gameData.multi.powerupsInMine [_nPowerup] + PowerupsOnShips (_nPowerup)) : \
			 gameData.multi.powerupsInMine [_nPowerup])

#define	TooManyPowerups(_nPowerup) \
			IsMultiGame && PowerupClass (_nPowerup) && \
			(PowerupsInMine (_nPowerup) >= gameData.multi.maxPowerupsAllowed [_nPowerup])

#endif /* _POWERUP_H */
