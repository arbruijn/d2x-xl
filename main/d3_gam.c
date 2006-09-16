
#include <stdio.h>
#include  oof.h 
#include  inferno.h 
#include  cfile.h 

//------------------------------------------------------------------------------

#define GAM_PAGENAME_LEN				35 
#define GAM_MAX_MODULENAME_LEN		32 
#define GAM_MAX_PLAYER_WEAPONS		21 
#define GAM_MAX_WB_GUNPOINTS		8 
#define GAM_MAX_WB_FIRING_MASKS	8 
#define GAM_MAX_PROC_ELEMENTS		8000 
#define GAM_MAX_WEAPON_SOUNDS		7
#define GAM_MAX_DSPEW_TYPES			2 
#define GAM_NUM_MOVEMENT_CLASSES	5 
#define GAM_NUM_ANIMS_PER_CLASS	24 
#define GAM_MAX_WBS_PER_OBJ			21 
#define GAM_MAX_OBJ_SOUNDS			2 
#define GAM_MAX_AI_SOUNDS			5 
#define GAM_MAX_DEATH_TYPES			4 

//------------------------------------------------------------------------------

typedef char	tGAM_string [GAM_PAGENAME_LEN];

typedef struct tGAM_vector {
	float	x, y, z;
} tGAM_vector;

typedef struct tGAM_physicsChunk {
	float			fMass;
	float			fDrag;
	float			fFullThrust;
	int			nFlags;
	float			fRotDrag;
	float			fFullRotThrust;
	int			nBounces;
	float			fInitVelocity;
	tGAM_vector	InitRotVelocity;
	float			fWiggleAmplitude;
	float			fWigglesPerSec;
	float			fCoEfficentRestitution;
	float			fHitDeathAngle;
	float			fMaxTurnRollRate;
	float			fTurnRollRatio;
} tGAM_physChunk;

typedef struct tGAM_firingMaskInfo {
	byte			nFireMasks;
	float			fFire WaitTime;
	float			fAnimationTime;
	float			fAnimationStartFrame;
	float			fAnimationFire Frame;
	float			fAnimationEndFrame;
} tGAM_firingMaskInfo;

typedef struct tGAM_weaponBatteryChunk {
	float						fEnergyUsage;
	float						fAmmoUsage;
	short						nGunpointWeaponIndex [GAM_MAX_WB_GUNPOINTS];
	tGAM_firingMaskInfo	firingMaskInfo [GAM_MAX_WB_FIRING_MASKS];
	byte						nMasksNum;
	short						nAimingGunpointIndex;
	byte						nAimingFlags;
	float						fAimingDot;
	float						fAimingDistance;
	float						fAimingXZDot;
	short						nFlags;
	byte						nQuadFireMask;
} tGAM_weaponBatteryChunk;

typedef struct tGAM_shipWeapons {
	byte							nFireFlags;
	tGAM_string					szFireSound;
	tGAM_string					szReleaseSound;
	tGAM_string					szSpewPowerup;
	int							nMaxAmmo;
	tGAM_weaponBatteryChunk	weaponBatteryInfo;
	tGAM_string					szFireSound [GAM_MAX_WB_GUNPOINTS];
	tGAM_string					szWeapon [GAM_MAX_WB_FIRING_MASKS];
} tGAM_shipWeapons;

typedef struct tGAM_ship {
	short					nVersion;
	tGAM_string			szName;
	tGAM_string			szCockpitModel;
	tGAM_string			szHUDConfig;
	tGAM_string			szModel;
	tGAM_string			szDyingModel;
	tGAM_string			szMedModel;
	tGAM_string			szLowModel;
	float					fMedLODdist;
	float					fLowLODdist;
	tGAM_physicsChunk	physicsInfo;
	float					fSize;
	float					fArmorScalar;
	int					fFlags;
	tGAM_shipWeapons	weapons [GAM_MAX_PLAYER_WEAPONS];
} tGAM_ship;

typedef struct tGAM_glow {
	float					red, green, blue, alpha;
} tGAM_glow;


#define GAM_TF_VOLATILE				1
#define GAM_TF_WATER					(1<<1)
#define GAM_TF_METAL					(1<<2)		// Shines like metal
#define GAM_TF_MARBLE				(1<<3)		// Shines like marble
#define GAM_TF_PLASTIC				(1<<4)		// Shines like plastic
#define GAM_TF_FORCEFIELD			(1<<5)
#define GAM_TF_ANIMATED				(1<<6)		// Set for animated textures (.OAF files)
#define GAM_TF_DESTROYABLE			(1<<7)
#define GAM_TF_EFFECT				(1<<8)
#define GAM_TF_HUD_COCKPIT			(1<<9)
#define GAM_TF_MINE					(1<<10)
#define GAM_TF_TERRAIN				(1<<11)
#define GAM_TF_OBJECT				(1<<12)
#define GAM_TF_TEXTURE_64			(1<<13)
#define GAM_TF_TMAP2					(1<<14)
#define GAM_TF_TEXTURE_32			(1<<15)
#define GAM_TF_FLY_THRU				(1<<16)
#define GAM_TF_PASS_THRU			(1<<17)
#define GAM_TF_PING_PONG			(1<<18)
#define GAM_TF_LIGHT					(1<<19)
#define GAM_TF_BREAKABLE			(1<<20)		// Breakable (as in glass)
#define GAM_TF_SATURATE				(1<<21)
#define GAM_TF_ALPHA					(1<<22)
#define GAM_TF_DONTUSE				(1<<23)		
#define GAM_TF_PROCEDURAL			(1<<24)		
#define GAM_TF_WATER_PROCEDURAL	(1<<25)
#define GAM_TF_FORCE_LIGHTMAP		(1<<26)
#define GAM_TF_SATURATE_LIGHTMAP	(1<<27)
#define GAM_TF_TEXTURE_256			(1<<28)
#define GAM_TF_LAVA					(1<<29)
#define GAM_TF_RUBBLE				(1<<30)
#define GAM_TF_SMOOTH_SPECULAR	(1<<31)
#define GAM_TF_TEXTURE_TYPES		(GAM_TF_MINE+GAM_TF_TERRAIN+GAM_TF_OBJECT+GAM_TF_EFFECT+GAM_TF_HUD_COCKPIT+GAM_TF_LIGHT)
#define GAM_TF_SPECULAR				(GAM_TF_METAL+GAM_TF_MARBLE|GAM_TF_PLASTIC)

typedef short	tGAM_palette [255];

typedef struct tGAM_procTexElements {
	byte	nType;
	byte	nFreq;
	byte	nSpeed;
	byte	nSize;
	byte	x1;
	byte	y1;
	byte	x2;
	byte	y2;
} tGAM_procTexElements;

typedef struct tGAM_procTexInfo {
	tGAM_palette		palette;
	byte					nHeat;
	byte					nLight;
	byte					nThickness;
	float					fEvaluationTime;
	float					fOscTime;
	byte					nOscValue;
	short					nProcElements;
	tGAM_procElements	elementInfo;
} tGAM_procTexInfo;

typedef struct tGAM_texture {
	short					nVersion;
	tGAM_string			szName;
	tGAM_string			szBitmap;
	tGAM_string			szDestroyed;
	tGAM_glow			glowInfo;
	float					fSpeed;
	float					fUSlide;
	float					fVSlide;
	float					fReflectivity;
	byte					nCoronaStyle;
	int					nDamage;
	int					nFlags;
	tGAM_procTexInfo	procTexInfo;
	tGAM_string			szSound;
	float					fSoundVol;
} tGAM_texture;
//------------------------------------------------------------------------------

int GAM_ReadFile (char *pszFile)
{
	FILE *fp;

// open the table fp
if (!fp = fopen (filename, rb))
	return 0;

while (!feof(fp)) {
	unsigned char nPageType = cfile_read_byte (fp);
	int nLength = cfile_read_int (fp);
	switch(nPageType) {
		case PAGETYPE_TEXTURE:
			GAM_ReadTexturePage (fp);
			break;
		case PAGETYPE_DOOR:
			GAM_ReadDoorPage (fp);
			break;
		case PAGETYPE_SOUND:
			GAM_ReadSoundPage (fp);
			break;
		case PAGETYPE_GENERIC:
			GAM_ReadGenericPage (fp);
			break;
		case PAGETYPE_GAMEFILE:
			GAM_ReadGamefilePage (fp);
			break;
		case PAGETYPE_SHIP:
			GAM_ReadShipPage (fp);
			break;
		case PAGETYPE_WEAPON:
			GAM_ReadWeaponPage (fp);
			break;
		// old pagetypes, no longer valid and not supported
		case PAGETYPE_ROBOT:
		case PAGETYPE_POWERUP:
		case PAGETYPE_MEGACELL:
		case PAGETYPE_UNKNOWN:
			fseek (fp,nLength - sizeof (int), SEEK_CUR);
			break;
		}
	}
fclose(fp);

//------------------------------------------------------------------------------

