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

#include "digi.h"
#include "sounds.h"
#include "cfile.h"
#include "tga.h"
#include "hash.h"

//------------------------------------------------------------------------------

#define DEFAULT_PIGFILE_REGISTERED      "groupa.pig"
#define DEFAULT_PIGFILE_SHAREWARE       "d2demo.pig"
#define DEFAULT_HAMFILE_REGISTERED      "descent2.ham"
#define DEFAULT_HAMFILE_SHAREWARE       "d2demo.ham"

#define DEFAULT_PIGFILE \
		  (gameStates.app.bDemoData ? DEFAULT_PIGFILE_SHAREWARE : DEFAULT_PIGFILE_REGISTERED)
#define DEFAULT_HAMFILE \
		  (gameStates.app.bDemoData ? DEFAULT_HAMFILE_SHAREWARE : DEFAULT_HAMFILE_REGISTERED)

#define DEFAULT_SNDFILE \
		  ((gameData.pig.tex.nHamFileVersion < 3) ? DEFAULT_HAMFILE_SHAREWARE : \
			(gameOpts->sound.digiSampleRate == SAMPLE_RATE_22K) ? "descent2.s22" : "descent2.s11")

#define PIGFILE_ID					MAKE_SIG ('G','I','P','P') //PPIG
#define PIGFILE_VERSION				2

#define HAMFILE_ID					MAKE_SIG ('!','M','A','H') //HAM!
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

#define MAX_ADDON_BITMAP_FILES	2

#define BM_ADDON_SLOWMOTION		0
#define BM_ADDON_BULLETTIME		1

#define MAX_ADDON_SOUND_FILES		12
#define SND_ADDON_MISSILE_SMALL	0
#define SND_ADDON_MISSILE_BIG		1
#define SND_ADDON_VULCAN			2
#define SND_ADDON_GAUSS				3
#define SND_ADDON_GATLING_SPIN	4
#define SND_ADDON_FLARE				5
#define SND_ADDON_LOW_SHIELDS1	6
#define SND_ADDON_LOW_SHIELDS2	7
#define SND_ADDON_LIGHTNING		8
#define SND_ADDON_SLOWDOWN			9
#define SND_ADDON_SPEEDUP			10
#define SND_ADDON_AIRBUBBLES		11

//------------------------------------------------------------------------------

typedef struct tRGBA {
	ubyte	r,g,b,a;
} tRGBA;

typedef struct tABGR {
	ubyte	a, b, g, r;
} tABGR;

typedef struct tBGRA {
	ubyte	b, g, r, a;
} tBGRA;

typedef struct tARGB {
	ubyte	a, r, g, b;
} tARGB;

typedef struct alias {
	char aliasname [FILENAME_LEN];
	char filename [FILENAME_LEN];
} alias;

typedef struct tPIGBitmapHeader {
	char name [8];
	ubyte dflags;           // bits 0-5 anim frame num, bit 6 abm flag
	ubyte width;            // low 8 bits here, 4 more bits in wh_extra
	ubyte height;           // low 8 bits here, 4 more bits in wh_extra
	ubyte wh_extra;         // bits 0-3 width, bits 4-7 height
	ubyte flags;
	ubyte bmAvgColor;
	int offset;
} __pack__ tPIGBitmapHeader;

typedef struct tPIGBitmapHeaderD1 {
	char name [8];
	ubyte dflags;           // bits 0-5 anim frame num, bit 6 abm flag
	ubyte width;            // low 8 bits here, 4 more bits in wh_extra
	ubyte height;           // low 8 bits here, 4 more bits in wh_extra
	ubyte flags;
	ubyte bmAvgColor;
	int offset;
} __pack__ tPIGBitmapHeaderD1;

#define PIGBITMAPHEADER_D1_SIZE 17 // no wh_extra

typedef struct tPIGSoundHeader {
	char name [8];
	int length;
	int data_length;
	int offset;
} __pack__ tPIGSoundHeader;

// an index into the bitmap collection of the piggy file
typedef struct tBitmapIndex {
	ushort index;
} __pack__ tBitmapIndex;

typedef struct tBitmapFile {
	char    name [15];
} tBitmapFile;

typedef struct tSoundFile {
	char    name [15];
} tSoundFile;

//------------------------------------------------------------------------------

int PiggyInit (void);
int PiggyInitMemory (void);
void PiggyInitPigFile (char *filename);
void _CDECL_ PiggyClose(void);
void PiggyDumpAll (void);
tBitmapIndex PiggyRegisterBitmap( grsBitmap * bmp, const char * name, int in_file );
int PiggyRegisterSound( tDigiSound * snd, char * name, int in_file );
tBitmapIndex PiggyFindBitmap (const char * name, int bD1Data );
int PiggyFindSound (const char * name);
int LoadSoundReplacements (const char *pszFileName);
void FreeSoundReplacements (void);
void LoadTextureColors (const char *pszLevelName, tFaceColor *colorP);
int LoadModelData (void);
int SaveModelData (void);

void piggy_read_bitmap_data (grsBitmap * bmp);
void piggy_readSound_data (tDigiSound *snd);

void PiggyLoadLevelData ();

#ifdef PIGGY_USE_PAGING
int PiggyBitmapPageIn (int bmi, int bD1);
int PageInBitmap (grsBitmap *bmP, const char *bmName, int nIndex, int bD1);
void PiggyBitmapPageOutAll(int bAll);
#endif

void PiggyReadSounds();

//reads in a new pigfile (for new palette)
//returns the size of all the bitmap data
void piggy_new_pigfile (const char *pigname);

//loads custom bitmaps for current level
void LoadBitmapReplacements (const char *level_name);
//if descent.pig exists, loads descent 1 texture bitmaps
void LoadD1BitmapReplacements();

#if 0
#define BitmapIndexRead(bi, fp) CFRead(bi, sizeof(tBitmapIndex), 1, fp)
#define BitmapIndexReadN(bi, n, fp) CFRead(bi, sizeof(tBitmapIndex), n, fp)
#else
/*
 * reads a tBitmapIndex structure from a CFILE
 */
void BitmapIndexRead (tBitmapIndex *bi, CFile& cf);

/*
 * reads n tBitmapIndex structs from a CFILE
 */
int BitmapIndexReadN (tBitmapIndex *bi, int n, CFile& cf);
#endif // FAST_FILE_IO

/*
 * Find and load the named bitmap from descent.pig
 */
tBitmapIndex ReadExtraBitmapD1Pig (const char *name);

void PIGBitmapHeaderRead (tPIGBitmapHeader *dbh, CFile& cf);
void PIGBitmapHeaderD1Read (tPIGBitmapHeader *dbh, CFile& cf);

grsBitmap *PiggyLoadBitmap (const char *pszFile);
void PiggyFreeBitmap (grsBitmap *bmP, int i, int bD1);
int CreateSuperTranspMasks (grsBitmap *bmP);

int PiggyFreeHiresAnimation (grsBitmap *bmP, int bD1);
void PiggyFreeHiresAnimations (void);

ubyte *LoadD1Palette (void);
void UseBitmapCache (grsBitmap *bmP, int nSize);
int IsAnimatedTexture (short nTexture);

int LoadSounds (CFile& fpSound, int nSoundNum, int nSoundStart);

int IsMacDataFile (CFile* cfP, int bD1);

void PiggyCriticalError (void);

void swap_0_255 (grsBitmap *bmP);

//------------------------------------------------------------------------------

extern CFile cfPiggy [2];

extern int nDescentCriticalError;
extern unsigned descent_critical_deverror;
extern unsigned descent_critical_errcode;

extern size_t bitmapCacheUsed;
extern size_t bitmapCacheSize;
extern int bitmapCacheNext [2];
extern int bitmapOffsets [2][MAX_BITMAP_FILES];

extern ubyte *d1Palette;

extern int Pigfile_initialized;
extern int bUseHiresTextures, bD1Data;
extern size_t bitmapCacheUsed;
extern size_t bitmapCacheSize;
extern const char *szAddonTextures [MAX_ADDON_BITMAP_FILES];

extern tHashTable soundNames [2];
extern int soundOffset [2][MAX_SOUND_FILES];

#if USE_SDL_MIXER

#	include <SDL_mixer.h>

const char *AddonSoundName (int nSound);
Mix_Chunk *LoadAddonSound (const char *pszSoundFile, ubyte *bBuiltIn);
void FreeAddonSounds (void);
#else
#	define AddonSoundName(_nSound)	NULL
#	define FreeAddonSounds()
#endif

#define	PageInAddonBitmap(bmi) \
			{if (((bmi) < 0) && ((bmi) >= -MAX_ADDON_BITMAP_FILES)) \
			 PageInBitmap (gameData.pig.tex.addonBitmaps - (bmi) - 1, szAddonTextures [-(bmi) - 1], (bmi), 0);}

#define BM_ADDON(_i)	(gameData.pig.tex.addonBitmaps + (_i))

//------------------------------------------------------------------------------

#endif //_PIGGY_H
