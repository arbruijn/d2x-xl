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

/*
 *
 * Interface to piggy functions.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:01:04  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:31:21  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.10  1995/02/03  17:08:29  john
 * Changed sound stuff to allow low memory usage.
 * Also, changed so that Sounds [gameStates.app.bD1Mission] isn't an array of digiSounds, it
 * is a ubyte pointing into GameSounds [gameStates.app.bD1Mission], this way the digi.c code that
 * locks sounds won't accidentally unlock a sound that is already playing, but
 * since it's Sounds [gameStates.app.bD1Mission][soundno] is different, it would erroneously be unlocked.
 *
 * Revision 1.9  1995/01/24  14:33:49  john
 * *** empty log message ***
 *
 * Revision 1.8  1995/01/24  14:32:35  john
 * Took out paging in code.
 *
 * Revision 1.7  1995/01/23  12:30:17  john
 * Made debug code that mprintf what bitmap gets paged in.
 *
 * Revision 1.6  1995/01/17  14:11:37  john
 * Added function that is called after level loaded.
 *
 * Revision 1.5  1995/01/14  19:16:58  john
 * First version of new bitmap paging code.
 *
 * Revision 1.4  1994/10/27  18:51:57  john
 * Added -piglet option that only loads needed textures for a
 * mine.  Only saved ~1MB, and code still doesn't free textures
 * before you load a new mine.
 *
 * Revision 1.3  1994/06/08  14:20:47  john
 * Made piggy dump before going into game.
 *
 * Revision 1.2  1994/05/06  13:02:40  john
 * Added piggy stuff; worked on supertransparency
 *
 * Revision 1.1  1994/05/06  11:47:46  john
 * Initial revision
 *
 *
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
tBitmapIndex PiggyRegisterBitmap( grs_bitmap * bmp, char * name, int in_file );
int piggy_registerSound( digiSound * snd, char * name, int in_file );
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

int ShrinkTGA (grs_bitmap *bm, int xFactor, int yFactor, int bRealloc, int nColorBytes);
int ReadTGAHeader (CFILE *fp, tTgaHeader *ph, grs_bitmap *pb);
int ReadTGAImage (CFILE *fp, tTgaHeader *ph, grs_bitmap *pb, int alpha, 
						double brightness, int bGrayScale, int bRedBlueFlip);
int LoadTGA (CFILE *fp, grs_bitmap *pb, int alpha, double brightness, 
				 int bGrayScale, int bRedBlueFlip);
int ReadTGA (char *pszFile, char *pszFolder, grs_bitmap *pb, int alpha, 
				 double brightness, int bGrayScale, int bRedBlueFlip);

extern int Pigfile_initialized;

void piggy_read_bitmap_data(grs_bitmap * bmp);
void piggy_readSound_data(digiSound *snd);

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

grs_bitmap *PiggyLoadBitmap (char *pszFile);
void PiggyFreeBitmap (grs_bitmap *bmP, int i, int bD1);
int CreateSuperTranspMasks (grs_bitmap *bmP);

int PiggyFreeHiresAnimation (grs_bitmap *bmP, int bD1);
void PiggyFreeHiresAnimations (void);

ubyte *LoadD1Palette (void);

extern int bUseHiresTextures, bD1Data;
extern unsigned int bitmapCacheUsed;
extern unsigned int bitmapCacheSize;

#endif //_PIGGY_H
