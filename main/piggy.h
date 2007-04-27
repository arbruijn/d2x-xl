/* $Id: piggy.h,v 1.25 2003/11/04 21:33:30 btb Exp $ */
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

#define MAX_ALIASES 20

typedef struct alias {
	char alias_name[FILENAME_LEN];
	char file_name[FILENAME_LEN];
} alias;

// an index into the bitmap collection of the piggy file
typedef struct tBitmapIndex {
	ushort index;
} __pack__ tBitmapIndex;

typedef struct BitmapFile {
	char    name [15];
} BitmapFile;

typedef struct SoundFile {
	char    name [15];
} SoundFile;


int PiggyInit();
void PiggyInitPigFile (char *filename);
void _CDECL_ PiggyClose(void);
void PiggyDumpAll();
tBitmapIndex PiggyRegisterBitmap( grsBitmap * bmp, char * name, int in_file );
int piggy_registerSound( tDigiSound * snd, char * name, int in_file );
tBitmapIndex piggy_find_bitmap( char * name, int bD1Data );
int piggy_findSound( char * name );

typedef struct {
    char  identSize;          // size of ID field that follows 18 char header (0 usually)
    char  colorMapType;      // nType of colour map 0=none, 1=has palette
    char  imageType;          // nType of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    short colorMapStart;     // first colour map entry in palette
    short colorMapLength;    // number of colours in palette
    char  colorMapBits;      // number of bits per palette entry 15,16,24,32

    short xStart;             // image x origin
    short yStart;             // image y origin
    short width;              // image width in pixels
    short height;             // image height in pixels
    char  bits;               // image bits per pixel 8,16,24,32
    char  descriptor;         // image descriptor bits (vh flip bits)
} tTgaHeader;

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

int ShrinkTGA (grsBitmap *bm, int xFactor, int yFactor, int bRealloc);
int ReadTGAHeader (CFILE *fp, tTgaHeader *ph, grsBitmap *pb);
int ReadTGAImage (CFILE *fp, tTgaHeader *ph, grsBitmap *pb, int alpha, 
						double brightness, int bGrayScale, int bRedBlueFlip);
int LoadTGA (CFILE *fp, grsBitmap *pb, int alpha, double brightness, 
				 int bGrayScale, int bRedBlueFlip);
int ReadTGA (char *pszFile, char *pszFolder, grsBitmap *pb, int alpha, 
				 double brightness, int bGrayScale, int bRedBlueFlip);
grsBitmap *CreateAndReadTGA (char *szFile);

extern int Pigfile_initialized;

void piggy_read_bitmap_data(grsBitmap * bmp);
void piggy_readSound_data(tDigiSound *snd);

void PiggyLoadLevelData();

#define MAX_BITMAP_FILES    2620 // Upped for CD Enhanced
#define D1_MAX_BITMAP_FILES 1555 // Upped for CD Enhanced
#define MAX_WALL_TEXTURES	 910
#define MAX_SOUND_FILES     MAX_SOUNDS

#ifdef PIGGY_USE_PAGING
void PiggyBitmapPageIn (tBitmapIndex bmp, int bD1);
void PiggyBitmapPageOutAll(int bAll);
#endif

void PiggyReadSounds();

//reads in a new pigfile (for new palette)
//returns the size of all the bitmap data
void piggy_new_pigfile(char *pigname);

//loads custom bitmaps for current level
void LoadBitmapReplacements(char *level_name);
//if descent.pig exists, loads descent 1 texture bitmaps
void LoadD1BitmapReplacements();

#ifdef FAST_FILE_IO
#define BitmapIndexRead(bi, fp) CFRead(bi, sizeof(tBitmapIndex), 1, fp)
#define BitmapIndexReadN(bi, n, fp) CFRead(bi, sizeof(tBitmapIndex), n, fp)
#else
/*
 * reads a tBitmapIndex structure from a CFILE
 */
void BitmapIndexRead(tBitmapIndex *bi, CFILE *fp);

/*
 * reads n tBitmapIndex structs from a CFILE
 */
int BitmapIndexReadN(tBitmapIndex *bi, int n, CFILE *fp);
#endif // FAST_FILE_IO

/*
 * Find and load the named bitmap from descent.pig
 */
tBitmapIndex ReadExtraBitmapD1Pig(char *name);

grsBitmap *PiggyLoadBitmap (char *pszFile);
void PiggyFreeBitmap (grsBitmap *bmP, int i, int bD1);
int CreateSuperTranspMasks (grsBitmap *bmP);

int PiggyFreeHiresAnimation (grsBitmap *bmP, int bD1);
void PiggyFreeHiresAnimations (void);
int SaveTGA (char *pszFile, char *pszFolder, tTgaHeader *ph, grsBitmap *bmP);

ubyte *LoadD1Palette (void);

extern int bUseHiresTextures, bD1Data;
extern unsigned int bitmapCacheUsed;
extern unsigned int bitmapCacheSize;

#endif //_PIGGY_H
