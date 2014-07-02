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

#ifndef _PIGGY_H
#define _PIGGY_H

#include "audio.h"
#include "sounds.h"
#include "cfile.h"
#include "tga.h"
#include "hash.h"

//------------------------------------------------------------------------------

#define PIGFILE_ID					MAKE_SIG ('G','I','P','P') //PPIG
#define PIGFILE_VERSION				2

#define HAMFILE_ID					MAKE_SIG ('!','M','A','H') //HAM!
#define DEMO_HAMFILE_VERSION		2
#define HAMFILE_VERSION				3
//version 1 -> 2:  save marker_model_num
//version 2 -> 3:  removed sound files

#define SNDFILE_ID					MAKE_SIG ('D','N','S','D') //DSND
#define SNDFILE_VERSION				1

#define MAC_ALIEN1_PIGSIZE			5013035
#define MAC_ALIEN2_PIGSIZE			4909916
#define MAC_FIRE_PIGSIZE			4969035
#define MAC_GROUPA_PIGSIZE			4929684 // also used for mac shareware
#define MAC_ICE_PIGSIZE				4923425
#define MAC_WATER_PIGSIZE			4832403	

#define MAX_BITMAP_FILES			2620 // Upped for CD Enhanced
#define D1_MAX_BITMAP_FILES		1555 // Upped for CD Enhanced
#define MAX_WALL_TEXTURES			910
#define MAX_SOUND_FILES				MAX_SOUNDS

#define D1_MAX_TEXTURES				800
#define D1_MAX_TMAP_NUM				1630 // 1621 in descent.pig Mac registered

#define D1_PIGFILE              "descent.pig"

#define D1_SHARE_BIG_PIGSIZE    5092871 // v1.0 - 1.4 before RLE compression
#define D1_SHARE_10_PIGSIZE     2529454 // v1.0 - 1.2
#define D1_SHARE_PIGSIZE        2509799 // v1.4
#define D1_10_BIG_PIGSIZE       7640220 // v1.0 before RLE compression
#define D1_10_PIGSIZE           4520145 // v1.0
#define D1_PIGSIZE              4920305 // v1.4 - 1.5 (Incl. OEM v1.4a)
#define D1_OEM_PIGSIZE          5039735 // v1.0
#define D1_MAC_PIGSIZE          3975533
#define D1_MAC_SHARE_PIGSIZE    2714487

#define MAX_ALIASES					20

#define MAX_ADDON_BITMAP_FILES	22

#define BM_ADDON_SLOWMOTION		0
#define BM_ADDON_BULLETTIME		1

#define MAX_ADDON_SOUND_FILES			39
#define SND_ADDON_MISSILE_SMALL		0
#define SND_ADDON_MISSILE_BIG			1
#define SND_ADDON_VULCAN				2
#define SND_ADDON_GAUSS					3
#define SND_ADDON_GATLING_SPIN		4
#define SND_ADDON_FLARE					5
#define SND_ADDON_LOW_SHIELDS1		6
#define SND_ADDON_LOW_SHIELDS2		7
#define SND_ADDON_LIGHTNING			8
#define SND_ADDON_SLOWDOWN				9
#define SND_ADDON_SPEEDUP				10
#define SND_ADDON_AIRBUBBLES			11
#define SND_ADDON_ZOOM1					12
#define SND_ADDON_ZOOM2					13
#define SND_ADDON_HEADLIGHT			14
#define SND_ADDON_BOILING_LAVA		15
#define SND_ADDON_BUSY_MACHINE		16
#define SND_ADDON_COMPUTER				17
#define SND_ADDON_DEEP_HUM				18
#define SND_ADDON_DRIPPING_WATER		19
#define SND_ADDON_DRIPPING_WATER2	20
#define SND_ADDON_DRIPPING_WATER3	21
#define SND_ADDON_EARTHQUAKE			22
#define SND_ADDON_ENERGY				23
#define SND_ADDON_FALLING_ROCKS		24
#define SND_ADDON_FALLING_ROCKS2		25
#define SND_ADDON_FAST_FAN				26
#define SND_ADDON_FLOWING_LAVA		27
#define SND_ADDON_FLOWING_WATER		28
#define SND_ADDON_INSECTS				29
#define SND_ADDON_MACHINE_GEAR		30
#define SND_ADDON_MIGHTY_MACHINE		31
#define SND_ADDON_STATIC_BUZZ			32
#define SND_ADDON_STEAM					33
#define SND_ADDON_TELEPORTER			34
#define SND_ADDON_WATERFALL			35
#define SND_ADDON_FIRE					36
#define SND_ADDON_JET_ENGINE			37
#define SND_ADDON_NUKE_EXPLOSION		38

//------------------------------------------------------------------------------

typedef struct tRGBA {
	uint8_t	r,g,b,a;
} __pack__ tRGBA;

typedef struct tABGR {
	uint8_t	a, b, g, r;
} __pack__ tABGR;

typedef struct tBGRA {
	uint8_t	b, g, r, a;
} __pack__ tBGRA;

typedef struct tARGB {
	uint8_t	a, r, g, b;
} __pack__ tARGB;

typedef struct alias {
	char aliasname [FILENAME_LEN];
	char filename [FILENAME_LEN];
} alias;

typedef struct tPIGBitmapHeader {
	char name [8];
	uint8_t dflags;           // bits 0-5 anim frame num, bit 6 abm flag
	uint8_t width;            // low 8 bits here, 4 more bits in wh_extra
	uint8_t height;           // low 8 bits here, 4 more bits in wh_extra
	uint8_t wh_extra;         // bits 0-3 width, bits 4-7 height
	uint8_t flags;
	uint8_t avgColor;
	int32_t offset;
} __pack__ tPIGBitmapHeader;

typedef struct tPIGBitmapHeaderD1 {
	char name [8];
	uint8_t dflags;           // bits 0-5 anim frame num, bit 6 abm flag
	uint8_t width;            // low 8 bits here, 4 more bits in wh_extra
	uint8_t height;           // low 8 bits here, 4 more bits in wh_extra
	uint8_t flags;
	uint8_t avgColor;
	int32_t offset;
} __pack__ tPIGBitmapHeaderD1;

#define PIGBITMAPHEADER_D1_SIZE 17 // no wh_extra

typedef struct tPIGSoundHeader {
	char name [8];
	int32_t length;
	int32_t data_length;
	int32_t offset;
} __pack__ tPIGSoundHeader;

// an index into the bitmap collection of the piggy file
typedef struct tBitmapIndex {
	uint16_t index;
} __pack__ tBitmapIndex;

typedef struct tBitmapFile {
	char    name [15];
} __pack__ tBitmapFile;

typedef struct tSoundFile {
	char    name [15];
} __pack__ tSoundFile;

//------------------------------------------------------------------------------

int32_t PiggyInit (void);
int32_t PiggyInitMemory (void);
void PiggyInitPigFile (char *filename);
void _CDECL_ PiggyClose(void);
void PiggyDumpAll (void);
tBitmapIndex PiggyRegisterBitmap( CBitmap * bmp, const char * name, int32_t in_file );
int32_t PiggyRegisterSound (char * name, int32_t bInFile, bool bCustom = false);
tBitmapIndex PiggyFindBitmap (const char * name, int32_t bD1Data );
int32_t PiggyFindSound (const char * name);
int32_t LoadSoundReplacements (const char *pszFileName);
void FreeSoundReplacements (void);
void LoadTextureColors (const char *pszLevelName, CFaceColor *colorP);
int32_t LoadModelData (void);
int32_t SaveModelData (void);

void piggy_read_bitmap_data (CBitmap * bmp);
void piggy_readSound_data (CSoundSample *snd);

int32_t ReadHiresBitmap (CBitmap* bmP, const char* bmName, int32_t nIndex, int32_t bD1);
int32_t PiggyBitmapPageIn (int32_t bmi, int32_t bD1, bool bHires = false);
int32_t PageInBitmap (CBitmap *bmP, const char *bmName, int32_t nIndex, int32_t bD1, bool bHires = false);
void UnloadTextures (void);
void ResetTextureFlags (void);

void LoadSounds (CFile& cf, bool bCustom = false);

//reads in a new pigfile (for new palette)
//returns the size of all the bitmap data
void piggy_new_pigfile (const char *pigname);

//loads custom bitmaps for current level
void LoadReplacementBitmaps (const char *level_name);
//if descent.pig exists, loads descent 1 texture bitmaps
void LoadD1Textures (void);

/*
 * reads a tBitmapIndex structure from a CFILE
 */
void ReadBitmapIndex (tBitmapIndex *bi, CFile& cf);

/*
 * reads n tBitmapIndex structs from a CFILE
 */
int32_t ReadBitmapIndices (CArray<tBitmapIndex>& bi, int32_t n, CFile& cf, int32_t o = 0);

/*
 * Find and load the named bitmap from descent.pig
 */
tBitmapIndex ReadExtraBitmapD1Pig (const char *name);

void PIGBitmapHeaderRead (tPIGBitmapHeader *dbh, CFile& cf);
void PIGBitmapHeaderD1Read (tPIGBitmapHeader *dbh, CFile& cf);

CBitmap *PiggyLoadBitmap (const char *pszFile);
int32_t CreateSuperTranspMasks (CBitmap *bmP);
void UnloadHiresAnimations (void);

CPalette* LoadD1Palette (void);
void UseBitmapCache (CBitmap *bmP, int32_t nSize);
int32_t IsAnimatedTexture (int16_t nTexture);

int32_t SetupSounds (CFile& fpSound, int32_t nSoundNum, int32_t nSoundStart, bool bCustom = false, bool bUseLowRes = true);

int32_t IsMacDataFile (CFile* cfP, int32_t bD1);

void PiggyCriticalError (void);

void swap_0_255 (CBitmap *bmP);

#define HIRES_SOUND_FOLDER(_bD1)		((_bD1) ? 3 : gameOpts->UseHiresSound ())

#define D1_PIG_LOAD_FAILED "Failed loading " D1_PIGFILE


//------------------------------------------------------------------------------

extern CFile cfPiggy [2];

extern int32_t nDescentCriticalError;
extern uint32_t descent_critical_deverror;
extern uint32_t descent_critical_errcode;

extern size_t bitmapCacheUsed;
extern size_t bitmapCacheSize;
extern int32_t bitmapCacheNext [2];
extern int32_t bitmapOffsets [2][MAX_BITMAP_FILES];

extern uint8_t *d1Palette;

extern int32_t Pigfile_initialized;
extern int32_t bUseHiresTextures, bD1Data;
extern size_t bitmapCacheUsed;
extern size_t bitmapCacheSize;
extern const char *szAddonTextures [MAX_ADDON_BITMAP_FILES];

extern CHashTable soundNames [2];
//extern int32_t soundOffset [2][MAX_SOUND_FILES];

#if USE_SDL_MIXER

#	ifdef __macosx__
#		include <SDL_mixer/SDL_mixer.h>
#	else
#		include <SDL_mixer.h>
#	endif

const char *AddonSoundName (int32_t nSound);
Mix_Chunk *LoadAddonSound (const char *pszSoundFile, uint8_t* bBuiltIn = NULL);
void LoadAddonSounds (void);
void FreeAddonSounds (void);
#else
#	define AddonSoundName(_nSound)	NULL
#	define LoadAddonSounds()
#	define FreeAddonSounds()
#endif

#define BM_ADDON(_i)					(&gameData.pig.tex.addonBitmaps [_i])
#define BM_ADDON_RETICLE_GREEN	2
#define BM_ADDON_RETICLE_RED		12

void PiggyCloseFile (void);

void PageInAddonBitmap (int32_t bmi);
bool BitmapLoaded (int32_t bmi, int32_t bD1);
void LoadTexture (int32_t bmi, int32_t nFrame, int32_t bD1, bool bHires = false);
void LoadGameBackground (void);

char* DefaultPigFile (int32_t bDemoData = 0);
char* DefaultHamFile (void);
char* DefaultSoundFile (void);

int32_t LoadD1PigHeader (CFile& cf, int32_t *pSoundNum, int32_t *pBmHdrOffs, int32_t *pBmDataOffs, int32_t *pBitmapNum, int32_t bReadTMapNums);

//------------------------------------------------------------------------------

typedef struct tAddonSound {
	Mix_Chunk		*chunkP;
	/*const*/ char		szSoundFile [FILENAME_LEN];
} tAddonSound;

extern tAddonSound addonSounds [MAX_ADDON_SOUND_FILES];

//------------------------------------------------------------------------------

#endif //_PIGGY_H
