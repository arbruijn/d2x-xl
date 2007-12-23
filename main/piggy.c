/* $Id: piggy.c,v 1.51 2004/01/08 19:02:53 schaffner Exp $ */
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


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: piggy.c,v 1.51 2004/01/08 19:02:53 schaffner Exp $";
#endif

#ifdef _WIN32
#	include <windows.h>
#endif

#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "strutil.h"
#include "inferno.h"
#include "gr.h"
#include "u_mem.h"
#include "iff.h"
#include "mono.h"
#include "error.h"
#include "sounds.h"
#include "songs.h"
#include "bm.h"
#include "bmread.h"
#include "hash.h"
#include "args.h"
#include "palette.h"
#include "gamefont.h"
#include "rle.h"
#include "screens.h"
#include "piggy.h"
#include "gamemine.h"
#include "textures.h"
#include "texmerge.h"
#include "paging.h"
#include "game.h"
#include "text.h"
#include "cfile.h"
#include "newmenu.h"
#include "byteswap.h"
#include "findfile.h"
#include "makesig.h"
#include "effects.h"
#include "wall.h"
#include "weapon.h"
#include "error.h"
#include "grdef.h"
#include "gamepal.h"

//#define NO_DUMP_SOUNDS        1   //if set, dump bitmaps but not sounds

#ifdef _DEBUG
#	define PIGGY_MEM_QUOTA	4
#else
#	define PIGGY_MEM_QUOTA	8
#endif

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

#define MAC_ALIEN1_PIGSIZE      5013035
#define MAC_ALIEN2_PIGSIZE      4909916
#define MAC_FIRE_PIGSIZE        4969035
#define MAC_GROUPA_PIGSIZE      4929684 // also used for mac shareware
#define MAC_ICE_PIGSIZE         4923425
#define MAC_WATER_PIGSIZE       4832403

short *d1_tmap_nums = NULL;

static short d2OpaqueDoors [] = {
	440, 451, 463, 477, /*483,*/ 488, 
	500, 508, 523, 550, 556, 564, 572, 579, 585, 593, 536, 
	600, 608, 615, 628, 635, 642, 649, 664, /*672,*/ 687, 
	702, 717, 725, 731, 738, 745, 754, 763, 772, 780, 790,
	806, 817, 827, 838, 849, /*858,*/ 863, 871, 886,
	901,
	-1};

ubyte bogus_data [4096*4096];
grsBitmap bogus_bitmap;
ubyte bogus_bitmap_initialized=0;
tDigiSound bogusSound;

#define RLE_REMAP_MAX_INCREASE 132 /* is enough for d1 pc registered */

static int bLowMemory = 0;
static int bMustWriteHamFile = 0;
static int nBitmapFilesNew = 0;
static int nSoundFilesNew = 0;

static hashtable soundNames [2];
static tSoundFile sounds [2][MAX_SOUND_FILES];
static int soundOffset [2][MAX_SOUND_FILES];

static hashtable bitmapNames [2];
size_t bitmapCacheUsed = 0;
size_t bitmapCacheSize = 0;
static int bitmapCacheNext [2] = {0, 0};
static int bitmapOffsets [2][MAX_BITMAP_FILES];

static ubyte *bitmapBits [2] = {NULL, NULL};

ubyte d1ColorMap [256];
ubyte *d1Palette = NULL;

#ifdef _DEBUG
#	define PIGGY_BUFFER_SIZE ((unsigned int) (512*1024*1024))
#else
#	define PIGGY_BUFFER_SIZE ((unsigned int) 0x7fffffff)
#endif
#define PIGGY_SMALL_BUFFER_SIZE (16*1024*1024)		// size of buffer when bLowMemory is set

#define DBM_FLAG_ABM    64 // animated bitmap
#define DBM_NUM_FRAMES  63

#define BM_FLAGS_TO_COPY (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT \
                         | BM_FLAG_NO_LIGHTING | BM_FLAG_RLE | BM_FLAG_RLE_BIG)

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

int ReadHamFile ();
int ReadSoundFile ();
void ChangeFilenameExtension (char *dest, char *src, char *new_ext);
extern char szLastPalettePig [];

//------------------------------------------------------------------------------

#ifdef _DEBUG
typedef struct tTrackedBitmaps {
	grsBitmap	*bmP;
	int			nSize;
} tTrackedBitmaps;

tTrackedBitmaps	trackedBitmaps [1000000];
int					nTrackedBitmaps = 0;
#endif

void UseBitmapCache (grsBitmap *bmP, int nSize)
{
bitmapCacheUsed += nSize;
if (0 > bitmapCacheUsed)
	bitmapCacheUsed = 0;
else if (0 > (int) bitmapCacheUsed)
	bitmapCacheUsed = 0;
#ifdef _DEBUG
if (nSize < 0) {
	int i;
	for (i = 0; i < nTrackedBitmaps; i++)
		if (trackedBitmaps [i].bmP == bmP) {
			//CBP (trackedBitmaps [i].nSize != -nSize);
			if (i < --nTrackedBitmaps)
				trackedBitmaps [i] = trackedBitmaps [nTrackedBitmaps];
			trackedBitmaps [nTrackedBitmaps].bmP = NULL;
			trackedBitmaps [nTrackedBitmaps].nSize = 0;
			break;
			}
	}
else {
	trackedBitmaps [nTrackedBitmaps].bmP = bmP;
	trackedBitmaps [nTrackedBitmaps++].nSize = nSize;
	}
#endif
}

//------------------------------------------------------------------------------

int IsOpaqueDoor (int i)
{
	short	*p;

for (p = d2OpaqueDoors; *p >= 0; p++)
	if (i == gameData.pig.tex.pBmIndex [*p].index)
		return 1;
return 0;
}

//------------------------------------------------------------------------------

#if TEXTURE_COMPRESSION

int SaveS3TC (grsBitmap *bmP, char *pszFolder, char *pszFilename)
{
	CFILE		cf;
	char		szFilename [FILENAME_LEN], szFolder [FILENAME_LEN];

if (!bmP->bmCompressed)
	return 0;
CFSplitPath (pszFilename, szFolder, szFilename + 1, NULL);
strcat (szFilename + 1, ".s3tc");
*szFilename = '\x02';	//don't search lib (hog) files
if (*szFolder)
	pszFolder = szFolder;
if (CFExist (szFilename, pszFolder, 0))
	return 1;
if (!CFOpen (&cf, szFilename + 1, pszFolder, "wb", 0))
	return 0;
if ((CFWrite (&bmP->bmProps.w, sizeof (bmP->bmProps.w), 1, &cf) != 1) ||
	 (CFWrite (&bmP->bmProps.h, sizeof (bmP->bmProps.h), 1, &cf) != 1) ||
	 (CFWrite (&bmP->bmFormat, sizeof (bmP->bmFormat), 1, &cf) != 1) ||
    (CFWrite (&bmP->bmBufSize, sizeof (bmP->bmBufSize), 1, &cf) != 1) ||
    (CFWrite (bmP->bmTexBuf, bmP->bmBufSize, 1, &cf) != 1)) {
	CFClose (&cf);
	return 0;
	}
return !CFClose (&cf);
}

//------------------------------------------------------------------------------

int ReadS3TC (grsBitmap *bmP, char *pszFolder, char *pszFilename)
{
	CFILE		cf;
	char		szFilename [FILENAME_LEN], szFolder [FILENAME_LEN];

if (!gameStates.ogl.bHaveTexCompression)
	return 0;
CFSplitPath (pszFilename, szFolder, szFilename, NULL);
if (!*szFilename)
	CFSplitPath (pszFilename, szFolder, szFilename, NULL);
strcat (szFilename, ".s3tc");
if (*szFolder)
	pszFolder = szFolder;
if (!CFOpen (&cf, szFilename, pszFolder, "rb", 0))
	return 0;
if ((CFRead (&bmP->bmProps.w, sizeof (bmP->bmProps.w), 1, &cf) != 1) ||
	 (CFRead (&bmP->bmProps.h, sizeof (bmP->bmProps.h), 1, &cf) != 1) ||
	 (CFRead (&bmP->bmFormat, sizeof (bmP->bmFormat), 1, &cf) != 1) ||
	 (CFRead (&bmP->bmBufSize, sizeof (bmP->bmBufSize), 1, &cf) != 1)) {
	CFClose (&cf);
	return 0;
	}
if (!(bmP->bmTexBuf = (ubyte *) D2_ALLOC (bmP->bmBufSize))) {
	CFClose (&cf);
	return 0;
	}
if (CFRead (bmP->bmTexBuf, bmP->bmBufSize, 1, &cf) != 1) {
	CFClose (&cf);
	return 0;
	}
CFClose (&cf);
bmP->bmCompressed = 1;
return 1;
}

#endif

//------------------------------------------------------------------------------

#if 0//def FAST_FILE_IO /*disabled for a reason!*/

#define PIGBitmapHeaderRead(dbh, &cf) CFRead (dbh, sizeof (tPIGBitmapHeader), 1, &cf)
#define PIGSoundHeaderRead(dsh, &cf) CFRead (dsh, sizeof (tPIGSoundHeader), 1, &cf)

#else

//------------------------------------------------------------------------------
/*
 * reads a tPIGBitmapHeader structure from a CFILE
 */
void PIGBitmapHeaderRead (tPIGBitmapHeader *dbh, CFILE *cfp)
{
CFRead (dbh->name, 8, 1, cfp);
dbh->dflags = CFReadByte (cfp);
dbh->width = CFReadByte (cfp);
dbh->height = CFReadByte (cfp);
dbh->wh_extra = CFReadByte (cfp);
dbh->flags = CFReadByte (cfp);
dbh->bmAvgColor = CFReadByte (cfp);
dbh->offset = CFReadInt (cfp);
}

//------------------------------------------------------------------------------
/*
 * reads a tPIGSoundHeader structure from a CFILE
 */
void PIGSoundHeaderRead (tPIGSoundHeader *dsh, CFILE *cfp)
{
CFRead (dsh->name, 8, 1, cfp);
dsh->length = CFReadInt (cfp);
dsh->data_length = CFReadInt (cfp);
dsh->offset = CFReadInt (cfp);
}
#endif // FAST_FILE_IO

//------------------------------------------------------------------------------
/*
 * reads a descent 1 tPIGBitmapHeader structure from a CFILE
 */
void PIGBitmapHeaderD1Read (tPIGBitmapHeader *dbh, CFILE *pcf)
{
CFRead (dbh->name, 8, 1, pcf);
dbh->dflags = CFReadByte (pcf);
dbh->width = CFReadByte (pcf);
dbh->height = CFReadByte (pcf);
dbh->wh_extra = 0;
dbh->flags = CFReadByte (pcf);
dbh->bmAvgColor = CFReadByte (pcf);
dbh->offset = CFReadInt (pcf);
}

//------------------------------------------------------------------------------

ubyte BigPig = 0;

int PiggyIsSubstitutableBitmap (char * name, char * subst_name);

#ifdef EDITOR
void PiggyWritePigFile (char *filename);
static void write_int (int i,FILE *file);
#endif

void swap_0_255 (grsBitmap *bmp)
{
	int i;
	ubyte	*p;

for (i = bmp->bmProps.h * bmp->bmProps.w, p = bmp->bmTexBuf; i; i--, p++) {
	if (!*p)
		*p = 255;
	else if (*p == 255)
		*p = 0;
	}
}

//------------------------------------------------------------------------------

tBitmapIndex PiggyRegisterBitmap (grsBitmap *bmP, char *name, int in_file)
{
	tBitmapIndex temp;
	Assert (gameData.pig.tex.nBitmaps [gameStates.app.bD1Data] < MAX_BITMAP_FILES);

temp.index = gameData.pig.tex.nBitmaps [gameStates.app.bD1Data];
if (!in_file) {
#ifdef EDITOR
	if (FindArg ("-macdata"))
		swap_0_255 (bmP);
#endif
	if (!BigPig)  
		gr_bitmap_rle_compress (bmP);
	nBitmapFilesNew++;
	}
strncpy (gameData.pig.tex.pBitmapFiles [gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]].name, name, 12);
hashtable_insert (bitmapNames + gameStates.app.bD1Mission, gameData.pig.tex.pBitmapFiles[gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]].name, gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]);
gameData.pig.tex.pBitmaps [gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]] = *bmP;
if (!in_file) {
	bitmapOffsets [gameStates.app.bD1Data][gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]] = 0;
	gameData.pig.tex.bitmapFlags [gameStates.app.bD1Data][gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]] = bmP->bmProps.flags;
	}
gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]++;
return temp;
}

//------------------------------------------------------------------------------

void PiggyInitSound (void)
{
memset (gameData.pig.sound.sounds, 0, sizeof (gameData.pig.sound.sounds));
}

//------------------------------------------------------------------------------

int PiggyRegisterSound (tDigiSound *soundP, char *szFileName, int nInFile)
{
	int i;

Assert (gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data] < MAX_SOUND_FILES);
strncpy (sounds [gameStates.app.bD1Data][gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data]].name, szFileName, 12);
hashtable_insert (&soundNames [gameStates.app.bD1Data], 
						sounds [gameStates.app.bD1Data][gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data]].name, 
						gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data]);
gameData.pig.sound.pSounds [gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data]] = *soundP;
if (!nInFile)
	soundOffset [gameStates.app.bD1Data][gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data]] = 0;
i = gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data];
if (!nInFile)
	nSoundFilesNew++;
gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data]++;
return i;
}

//------------------------------------------------------------------------------

tBitmapIndex PiggyFindBitmap (char * name, int bD1Data)   
{
	tBitmapIndex bmp;
	int i;
	char *t;

	bmp.index = 0;

	if ((t=strchr (name,'#'))!=NULL)
		*t=0;

	for (i=0;i<gameData.pig.tex.nAliases;i++)
		if (stricmp (name,gameData.pig.tex.aliases [i].alias_name)==0) {
			if (t) {                //extra stuff for ABMs
				static char temp [SHORT_FILENAME_LEN];
				_splitpath (gameData.pig.tex.aliases [i].file_name, NULL, NULL, temp, NULL);
				name = temp;
				strcat (name,"#");
				strcat (name,t+1);
			}
			else
				name=gameData.pig.tex.aliases [i].file_name; 
			break;
		}

	if (t)
		*t = '#';

	i = hashtable_search (bitmapNames + bD1Data, name);
	Assert (i != 0);
	if (i < 0)
		return bmp;

	bmp.index = i;
	return bmp;
}

//------------------------------------------------------------------------------

int PiggyFindSound (char * name)     
{
	int i;

i = hashtable_search (&soundNames [gameStates.app.bD1Data], name);
if (i < 0)
	return 255;
return i;
}

//------------------------------------------------------------------------------

CFILE cfPiggy [2] = {{NULL, 0, 0, 0}, {NULL, 0, 0, 0}};

char szCurrentPigFile [2][SHORT_FILENAME_LEN] = {"",""};

void PiggyCloseFile (void)
{
	int	i;

for (i = 0; i < 2; i++)
	if (cfPiggy [i].file) {
		CFClose (cfPiggy + i);
		szCurrentPigFile [i][0] = 0;
		}
}

//------------------------------------------------------------------------------

int bPigFileInitialized=0;

#define PIGFILE_ID              MAKE_SIG ('G','I','P','P') //PPIG
#define PIGFILE_VERSION         2

extern char CDROM_dir [];

int RequestCD (void);



//------------------------------------------------------------------------------
//copies a pigfile from the CD to the current dir
//retuns file handle of new pig
int CopyPigFileFromCD (CFILE *cfp, char *filename)
{
	char name [80];
	FFS ffs;
	int ret;

	return CFOpen (cfp, filename, gameFolders.szDataDir, "rb", 0);
	ShowBoxedMessage ("Copying bitmap data from CD...");
	GrPaletteStepLoad (NULL);    //I don't think this line is really needed

	//First, delete all PIG files currently in the directory

	if (!FFF ("*.pig", &ffs, 0)) {
		do {
			CFDelete (ffs.name, "");
		} while (!FFN  (&ffs, 0));
		FFC (&ffs);
	}

	//Now, copy over new pig

	SongsStopRedbook ();           //so we can read off the cd

	//new code to unarj file
	strcpy (name,CDROM_dir);
	strcat (name,"descent2.sow");

	do {
//		ret = unarj_specific_file (name,filename,filename);
// DPH:FIXME

		ret = !EXIT_SUCCESS;

		if (ret != EXIT_SUCCESS) {

			//delete file, so we don't leave partial file
			CFDelete (filename, "");

			if (RequestCD () == -1)
				//NOTE LINK TO ABOVE IF
				Error ("Cannot load file <%s> from CD",filename);
		}

	} while (ret != EXIT_SUCCESS);

	return CFOpen (cfp, filename, gameFolders.szDataDir, "rb", 0);
}

//------------------------------------------------------------------------------

int PiggyInitMemory (void)
{
	static	int bMemInited = 0;

if (bMemInited)
	return 1;
if (gameStates.app.bUseSwapFile) {
	bitmapCacheSize = 0xFFFFFFFF;
	return bMemInited = 1;
	}
#ifdef EDITOR
bitmapCacheSize = nDataSize + (nDataSize / 10);   //extra mem for new bitmaps
Assert (bitmapCacheSize > 0);
#elif defined (WIN32)
	{
	MEMORYSTATUS	memStat;
	GlobalMemoryStatus (&memStat);
	bitmapCacheSize = (int) (memStat.dwAvailPhys / 10) * PIGGY_MEM_QUOTA;
	}
#else
bitmapCacheSize = PIGGY_BUFFER_SIZE;
if (bLowMemory)
	bitmapCacheSize = PIGGY_SMALL_BUFFER_SIZE;
// the following loop will scale bitmap memory down by 20% for each iteration
// until memory could be allocated, and then once more to leave enough memory
// for other parts of the program
for (;;) {
	if ((bitmapBits [0] = D2_ALLOC (bitmapCacheSize))) {
		D2_FREE (bitmapBits [0]);
		break;
		}
	bitmapCacheSize = (bitmapCacheSize / 10) * PIGGY_MEM_QUOTA;
	if (bitmapCacheSize < PIGGY_SMALL_BUFFER_SIZE) {
		Error ("Not enough memory to load D2 bitmaps\n");
		return 0;
		break;
		}
	}
#endif
return bMemInited = 1;
}

//------------------------------------------------------------------------------
//initialize a pigfile, reading headers
//returns the size of all the bitmap data
void PiggyInitPigFile (char *filename)
{
	char					temp_name [16];
	char					temp_name_read [16];
	char					szPigName [FILENAME_LEN];
	int					nHeaderSize, nBitmapNum, nDataSize, nDataStart, i;
	grsBitmap			bmTemp;
	tPIGBitmapHeader	bmh;

PiggyCloseFile ();             //close old pig if still open
strcpy (szPigName, filename);
//rename pigfile for shareware
if (!stricmp (DEFAULT_PIGFILE, DEFAULT_PIGFILE_SHAREWARE) && 
	 !CFExist (szPigName, gameFolders.szDataDir, 0))
	strcpy (szPigName, DEFAULT_PIGFILE_SHAREWARE);
strlwr (szPigName);
if (!CFOpen (cfPiggy + gameStates.app.bD1Data, szPigName, gameFolders.szDataDir, "rb", 0)) {
#ifdef EDITOR
	return;         //if editor, ok to not have pig, because we'll build one
#else
	CopyPigFileFromCD (cfPiggy + gameStates.app.bD1Data, szPigName);
#endif
	}
if (cfPiggy [gameStates.app.bD1Data].file) {                        //make sure pig is valid nType file & is up-to-date
	int pig_id = CFReadInt (cfPiggy + gameStates.app.bD1Data);
	int pig_version = CFReadInt (cfPiggy + gameStates.app.bD1Data);
	if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION) {
		CFClose (cfPiggy + gameStates.app.bD1Data);              //out of date pig
		}
	}
if (!cfPiggy [gameStates.app.bD1Data].file) {
#ifndef EDITOR
	Error ("Cannot load required file <%s>", szPigName);
#endif
	return;
	}
strncpy (szCurrentPigFile [0], szPigName, sizeof (szCurrentPigFile [0]));
nBitmapNum = CFReadInt (cfPiggy + gameStates.app.bD1Data);
nHeaderSize = nBitmapNum * sizeof (tPIGBitmapHeader);
nDataStart = nHeaderSize + CFTell (cfPiggy + gameStates.app.bD1Data);
nDataSize = CFLength (cfPiggy + gameStates.app.bD1Data, 0) - nDataStart;
gameData.pig.tex.nBitmaps [0] = 1;
for (i = 0; i < nBitmapNum; i++) {
	PIGBitmapHeaderRead (&bmh, cfPiggy + gameStates.app.bD1Data);
	memcpy (temp_name_read, bmh.name, 8);
	temp_name_read [8] = 0;
	if (bmh.dflags & DBM_FLAG_ABM)        
		sprintf (temp_name, "%s#%d", temp_name_read, bmh.dflags & DBM_NUM_FRAMES);
	else
		strcpy (temp_name, temp_name_read);
	memset (&bmTemp, 0, sizeof (grsBitmap));
	bmTemp.bmProps.w = bmTemp.bmProps.rowSize = bmh.width + ((short) (bmh.wh_extra & 0x0f) << 8);
	bmTemp.bmProps.h = bmh.height + ((short) (bmh.wh_extra & 0xf0) << 4);
	bmTemp.bmProps.flags |= BM_FLAG_PAGED_OUT;
	bmTemp.bmAvgColor = bmh.bmAvgColor;
	gameData.pig.tex.bitmapFlags [0][i+1] = bmh.flags & BM_FLAGS_TO_COPY;
	bitmapOffsets [0][i+1] = bmh.offset + nDataStart;
	Assert ((i+1) == gameData.pig.tex.nBitmaps [0]);
	PiggyRegisterBitmap (&bmTemp, temp_name, 1);
	}
bPigFileInitialized = 1;
}

//------------------------------------------------------------------------------

void FreeSoundReplacements (void)
{
	tDigiSound	*dsP;
	int			i, j;

LogErr ("unloading custom sounds\n");
for (i = 0; i < 2; i++)
	for (j = 0, dsP = gameData.pig.sound.sounds [i]; j < MAX_SOUND_FILES; j++, dsP++)
		if (dsP->bDTX) {
			D2_FREE (dsP->data [1]);
			dsP->bDTX = 0;
			}
}

//------------------------------------------------------------------------------

int LoadSoundReplacements (char *pszFilename)
{
	CFILE					cf;
	char					szId [4];
	int					nSounds;
	int					i, l;
	int					b11K = (gameOpts->sound.digiSampleRate == SAMPLE_RATE_11K);
	ubyte					j;
	tPIGSoundHeader	dsh;
	tDigiSound			*dsP;
	size_t				nHeaderOffs, nDataOffs;
	char					szFilename [SHORT_FILENAME_LEN];

if (gameOpts->sound.bHires)
	return -1;
//first, D2_FREE up data allocated for old bitmaps
LogErr ("   loading replacement sounds\n");
FreeSoundReplacements ();
ChangeFilenameExtension (szFilename, pszFilename, ".dtx");
if (!CFOpen (&cf, szFilename, gameFolders.szDataDir, "rb", 0))
	return -1;
if (CFRead (szId, 1, sizeof (szId), &cf) != sizeof (szId)) {
	CFClose (&cf);
	return -1;
	}
if (strncmp (szId, "DTX2", sizeof (szId))) {
	CFClose (&cf);
	return -1;
	}
nSounds = CFReadInt (&cf);
if (!b11K)
	nSounds *= 2;
nHeaderOffs = CFTell (&cf);
nDataOffs = nHeaderOffs + nSounds * sizeof (tPIGSoundHeader);
for (i = b11K ? 0 : nSounds / 2; i < nSounds; i++) {
	CFSeek (&cf, (long) (nHeaderOffs + i * sizeof (tPIGSoundHeader)), SEEK_SET);
	PIGSoundHeaderRead (&dsh, &cf);
	j = PiggyFindSound (dsh.name);
	if (j == 255)
		continue;
	dsP = gameData.pig.sound.sounds [gameStates.app.bD1Mission] + j;
	l = dsh.length;
	if (!(dsP->data [1] = (ubyte *) D2_ALLOC (l)))
		continue;
	CFSeek (&cf, (long) (nDataOffs + dsh.offset), SEEK_SET);
	if (CFRead (dsP->data [1], 1, l, &cf) != (unsigned int) l) {
		CFClose (&cf);
		D2_FREE (dsP->data [1]);
		return -1;
		}
	dsP->bDTX = 1;
	dsP->nLength [1] = l;
	}
CFClose (&cf);
return 1;
}

//------------------------------------------------------------------------------

#define SHORT_FILENAME_LEN 13
#define MAX_BITMAPS_PER_BRUSH 30

extern int ComputeAvgPixel (grsBitmap *new);

//reads in a new pigfile (for new palette)
//returns the size of all the bitmap data
void piggy_new_pigfile (char *pigname)
{
	int i;
	char temp_name [16];
	char temp_name_read [16];
	grsBitmap bmTemp;
	tPIGBitmapHeader bmh;
	int nHeaderSize, nBitmapNum, nDataSize, nDataStart;
	int must_rewrite_pig = 0;

	strlwr (pigname);

	//rename pigfile for shareware
	if (stricmp (DEFAULT_PIGFILE, DEFAULT_PIGFILE_SHAREWARE) == 0 && !CFExist (pigname, gameFolders.szDataDir,0))
		pigname = DEFAULT_PIGFILE_SHAREWARE;

	if (strnicmp (szCurrentPigFile [0], pigname, sizeof (szCurrentPigFile)) == 0) // no need to reload: no bitmaps were altered
		return;

	if (!bPigFileInitialized) {                     //have we ever opened a pigfile?
		PiggyInitPigFile (pigname);            //..no, so do initialization stuff
		return;
	}
	else
		PiggyCloseFile ();             //close old pig if still open
	bitmapCacheNext [0] = 0;            //D2_FREE up cache
	strncpy (szCurrentPigFile[0],pigname,sizeof (szCurrentPigFile));
	CFOpen (cfPiggy, pigname, gameFolders.szDataDir, "rb", 0);

	#ifndef EDITOR
	if (!cfPiggy [0].file)
		CopyPigFileFromCD (cfPiggy, pigname);
	#endif
	if (cfPiggy [0].file) {  //make sure pig is valid nType file & is up-to-date
		int pig_id,pig_version;

		pig_id = CFReadInt (cfPiggy);
		pig_version = CFReadInt (cfPiggy);
		if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION) {
			CFClose (cfPiggy);              //out of date pig
		}
	}

#ifndef EDITOR
	if (!cfPiggy [0].file)
		Error ("Cannot open correct version of <%s>", pigname);
#endif
	if (cfPiggy [0].file) {
		nBitmapNum = CFReadInt (cfPiggy);
		nHeaderSize = nBitmapNum * sizeof (tPIGBitmapHeader);
		nDataStart = nHeaderSize + CFTell (cfPiggy);
		nDataSize = CFLength (cfPiggy, 0) - nDataStart;
		for (i = 1; i <= nBitmapNum; i++) {
			PIGBitmapHeaderRead (&bmh, cfPiggy);
			memcpy (temp_name_read, bmh.name, 8);
			temp_name_read [8] = 0;
			if (bmh.dflags & DBM_FLAG_ABM)        
				sprintf (temp_name, "%s#%d", temp_name_read, bmh.dflags & DBM_NUM_FRAMES);
			else
				strcpy (temp_name, temp_name_read);
			//Make sure name matches
			if (strcmp (temp_name,gameData.pig.tex.bitmapFiles [0][i].name)) {
				//Int3 ();       //this pig is out of date.  Delete it
				must_rewrite_pig=1;
			}
			strcpy (gameData.pig.tex.bitmapFiles [0][i].name,temp_name);
			memset (&bmTemp, 0, sizeof (grsBitmap));
			bmTemp.bmProps.w = bmTemp.bmProps.rowSize = bmh.width + ((short) (bmh.wh_extra&0x0f)<<8);
			bmTemp.bmProps.h = bmh.height + ((short) (bmh.wh_extra & 0xf0)<<4);
			bmTemp.bmProps.flags |= BM_FLAG_PAGED_OUT;
			bmTemp.bmAvgColor = bmh.bmAvgColor;
			bmTemp.bmTexBuf = D2_ALLOC (bmTemp.bmProps.w * bmTemp.bmProps.h);
			bitmapCacheUsed += bmTemp.bmProps.w * bmTemp.bmProps.h;
			gameData.pig.tex.bitmapFlags [0][i] = bmh.flags & BM_FLAGS_TO_COPY;
			bitmapOffsets [0][i] = bmh.offset + nDataStart;
			gameData.pig.tex.bitmaps [0][i] = bmTemp;
		}
	}
	else
		nBitmapNum = 0;          //no pigfile, so no bitmaps

#ifndef EDITOR
	Assert (nBitmapNum == gameData.pig.tex.nBitmaps [0]-1);
#else
	if (must_rewrite_pig || (nBitmapNum < gameData.pig.tex.nBitmaps [0]-1)) {
		int size;
		//re-read the bitmaps that aren't in this pig
		for (i = nBitmapNum + 1; i < gameData.pig.tex.nBitmaps [0]; i++) {
			char *p;
			p = strchr (gameData.pig.tex.bitmapFiles [0][i].name, '#');
			if (p) {   // this is an ABM == animated bitmap
				char abmname [SHORT_FILENAME_LEN];
				int fnum;
				grsBitmap * bm [MAX_BITMAPS_PER_BRUSH];
				int iff_error;          //reference parm to avoid warning message
				ubyte newpal [768];
				char basename [SHORT_FILENAME_LEN];
				int nframes;
				strcpy (basename, gameData.pig.tex.bitmapFiles [0][i].name);
				basename [p-gameData.pig.tex.bitmapFiles [0][i].name] = 0;  //cut off "#nn" part
				sprintf (abmname, "%s.abm", basename);
				iff_error = iff_read_animbrush (abmname,bm,MAX_BITMAPS_PER_BRUSH,&nframes,newpal);
				if (iff_error != IFF_NO_ERROR) {
#if TRACE			
					con_printf (1,"File %s - IFF error: %s",abmname,iff_errormsg (iff_error));
#endif
					Error ("File %s - IFF error: %s",abmname,iff_errormsg (iff_error);
				}
				for (fnum=0;fnum<nframes; fnum++) {
					char tempname [20];
					int SuperX;
					sprintf (tempname, "%s#%d", basename, fnum);
					//SuperX = (gameData.pig.tex.bitmaps [i+fnum].bmProps.flags&BM_FLAG_SUPER_TRANSPARENT)?254:-1;
					SuperX = (gameData.pig.tex.bitmapFlags [i + fnum] & BM_FLAG_SUPER_TRANSPARENT) ? 254 : -1;
					//above makes assumption that supertransparent color is 254
					if (iff_has_transparency)
						GrRemapBitmapGood (bm [fnum], newpal, iff_transparent_color, SuperX);
					else
						GrRemapBitmapGood (bm [fnum], newpal, -1, SuperX);
					bm [fnum]->bmAvgColor = ComputeAvgPixel (bm [fnum]);

#ifdef EDITOR
					if (FindArg ("-macdata"))
						swap_0_255 (bm [fnum]);
#endif
					if (!BigPig) gr_bitmap_rle_compress (bm [fnum]);

					if (bm [fnum]->bmProps.flags & BM_FLAG_RLE)
						size = * ((int *) bm [fnum]->bmTexBuf);
					else
						size = bm [fnum]->bmProps.w * bm [fnum]->bmProps.h;
					gameData.pig.tex.bitmaps [0][i+fnum] = *bm [fnum];
					D2_FREE (bm [fnum]);
				}
				i += nframes - 1;         //filled in multiple bitmaps
			}
			else {          //this is a BBM
				grsBitmap * newBm;
				ubyte newpal [256*3];
				int iff_error;
				char bbmname [SHORT_FILENAME_LEN];
				int SuperX;
				MALLOC (newBm, grsBitmap, 1);
				sprintf (bbmname, "%s.bbm", gameData.pig.tex.bitmapFiles [0][i].name);
				iff_error = iff_read_bitmap (bbmname,newBm,BM_LINEAR,newpal);
				newBm->bmHandle=0;
				if (iff_error != IFF_NO_ERROR) {
#if TRACE			
					con_printf (1, "File %s - IFF error: %s",bbmname,iff_errormsg (iff_error));
#endif
					Error ("File %s - IFF error: %s",bbmname,iff_errormsg (iff_error);
				}
				SuperX = (gameData.pig.tex.bitmapFlags [i]&BM_FLAG_SUPER_TRANSPARENT)?254:-1;
				//above makes assumption that supertransparent color is 254
				if (iff_has_transparency)
					GrRemapBitmapGood (newBm, newpal, iff_transparent_color, SuperX);
				else
					GrRemapBitmapGood (newBm, newpal, -1, SuperX);
				newBm->bmAvgColor = ComputeAvgPixel (newBm);
#ifdef EDITOR
				if (FindArg ("-macdata"))
					swap_0_255 (newBm);
#endif
				if (!BigPig)  gr_bitmap_rle_compress (newBm);
				if (newBm->bmProps.flags & BM_FLAG_RLE)
					size = * ((int *) newBm->bmTexBuf);
				else
					size = newBm->bmProps.w * newBm->bmProps.h;
				gameData.pig.tex.bitmaps [0][i] = *newBm;
				D2_FREE (newBm);
			}
		}

		//@@Dont' do these things which are done when writing
		//@@for (i=0; i < gameData.pig.tex.nBitmaps; i++) {
		//@@    tBitmapIndex bi;
		//@@    bi.index = i;
		//@@    PIGGY_PAGE_IN (bi);
		//@@}
		//@@
		//@@PiggyCloseFile ();
		PiggyWritePigFile (pigname);
		szCurrentPigFile [0] = 0;                 //say no pig, to force reload
		piggy_new_pigfile (pigname);             //read in just-generated pig
	}
	#endif  //ifdef EDITOR
}

//------------------------------------------------------------------------------

int LoadHiresSound (tDigiSound *soundP, char *pszSoundName)
{
	CFILE			cf;
	char			szSoundFile [FILENAME_LEN];

if (!gameOpts->sound.bHires)
	return 0;
sprintf (szSoundFile, "%s.wav", pszSoundName);
if (!CFOpen (&cf, szSoundFile, gameFolders.szSoundDir [gameOpts->sound.bHires - 1], "rb", 0))
	return 0;
if (0 >= (soundP->nLength [0] = CFLength (&cf, 0))) {
	CFClose (&cf);
	return 0;
	}
if (!(soundP->data [0] = (ubyte *) D2_ALLOC (soundP->nLength [0]))) {
	CFClose (&cf);
	return 0;
	}
if (CFRead (soundP->data [0], soundP->nLength [0], 1, &cf) != 1) {
	CFClose (&cf);
	return 0;
	}
CFClose (&cf);
soundP->bHires = 1;
return 1;
}

//------------------------------------------------------------------------------

int LoadSounds (CFILE *fpSound, int nSoundNum, int nSoundStart)
{
	tPIGSoundHeader	sndh;
	tDigiSound			sound;
	int					i;
	int					sbytes = 0;
	int 					nHeaderSize = nSoundNum * sizeof (tPIGSoundHeader);
	char					szSoundName [16];


/*---*/LogErr ("      Loading sound data (%d sounds)\n", nSoundNum);
CFSeek (fpSound, nSoundStart, SEEK_SET);
memset (&sound, 0, sizeof (sound));
#if USE_OPENAL
memset (&sound.buffer, 0xFF, sizeof (sound.buffer));
#endif
for (i = 0; i < nSoundNum; i++) {
	PIGSoundHeaderRead (&sndh, fpSound);
	//size -= sizeof (tPIGSoundHeader);
	memcpy (szSoundName, sndh.name, 8);
	szSoundName [8] = 0;
	if (LoadHiresSound (&sound, szSoundName)) 
		sbytes += sound.nLength [0];
	else {
		sound.bHires = 0;
		sound.nLength [0] = sndh.length;
		sound.data [0] = (ubyte *) (size_t) (sndh.offset + nHeaderSize + nSoundStart);
		soundOffset [gameStates.app.bD1Data][gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data]] = sndh.offset + nHeaderSize + nSoundStart;
		sbytes += sndh.length;
		}
	PiggyRegisterSound (&sound, szSoundName, 1);
	}
if (!(gameData.pig.sound.data [gameStates.app.bD1Data] = D2_ALLOC (sbytes + 16))) {
	Error ("Not enough memory to load sounds\n");
	return 0;
	}
#if TRACE			
con_printf (CON_VERBOSE, "\nBitmapNum: %d KB   Sounds [gameStates.app.bD1Data]: %d KB\n", 
				bitmapCacheSize / 1024, sbytes / 1024);
#endif
return 1;
}

//------------------------------------------------------------------------------

#define HAMFILE_ID              MAKE_SIG ('!','M','A','H') //HAM!
#define HAMFILE_VERSION 3
//version 1 -> 2:  save marker_model_num
//version 2 -> 3:  removed sound files

#define SNDFILE_ID              MAKE_SIG ('D','N','S','D') //DSND
#define SNDFILE_VERSION 1

int ReadHamFile ()
{
	CFILE cfHAM;
#if 1
	char szD1PigFileName [FILENAME_LEN];
#endif
	int nHAMId;
	int nSoundOffset = 0;

	if (!CFOpen (&cfHAM, DEFAULT_HAMFILE, gameFolders.szDataDir, "rb", 0)) {
		bMustWriteHamFile = 1;
		return 0;
		}
	//make sure ham is valid nType file & is up-to-date
	nHAMId = CFReadInt (&cfHAM);
	gameData.pig.tex.nHamFileVersion = CFReadInt (&cfHAM);
	if (nHAMId != HAMFILE_ID)
		Error ("Cannot open ham file %s\n", DEFAULT_HAMFILE);
#if 0
	if (nHAMId != HAMFILE_ID || gameData.pig.tex.nHamFileVersion != HAMFILE_VERSION) {
		bMustWriteHamFile = 1;
		CFClose (&cfHAM);						//out of date ham
		return 0;
	}
#endif
	if (gameData.pig.tex.nHamFileVersion < 3) // hamfile contains sound info
		nSoundOffset = CFReadInt (&cfHAM);
	#ifndef EDITOR
	{
		//int i;
		BMReadAll (&cfHAM);
/*---*/LogErr ("      Loading bitmap index translation table\n");
		CFRead (gameData.pig.tex.bitmapXlat, sizeof (ushort)*MAX_BITMAP_FILES, 1, &cfHAM);
		// no swap here?
		//for (i = 0; i < MAX_BITMAP_FILES; i++) {
			//gameData.pig.tex.bitmapXlat [i] = INTEL_SHORT (gameData.pig.tex.bitmapXlat [i]);
			////printf ("gameData.pig.tex.bitmapXlat [%d] = %d\n", i, gameData.pig.tex.bitmapXlat [i]);
		//}
	}
	#endif

	if (gameData.pig.tex.nHamFileVersion < 3) {
		int nSoundNum;
		int nSoundStart;

		CFSeek (&cfHAM, nSoundOffset, SEEK_SET);
		nSoundNum = CFReadInt (&cfHAM);
		nSoundStart = CFTell (&cfHAM);
/*---*/LogErr ("      Loading %d sounds\n", nSoundNum);
		LoadSounds (&cfHAM, nSoundNum, nSoundStart);
	}

	CFClose (&cfHAM);
#if 1
/*---*/LogErr ("      Looking for Descent 1 data files\n");
		strcpy (szD1PigFileName, "descent.pig");
		if (!cfPiggy [1].file)
			CFOpen (cfPiggy + 1, szD1PigFileName, gameFolders.szDataDir, "rb", 0);
		if (cfPiggy [1].file) {
			gameStates.app.bHaveD1Data = 1;
/*---*/LogErr ("      Loading Descent 1 data\n");
			BMReadGameDataD1 (cfPiggy + 1);
			//CFClose (cfPiggy);
			}
#else
		strcpy (szD1PigFileName, "descent.pig");
		CFOpen (cfPiggy + 1, szD1PigFileName, gameFolders.szDataDir, "rb", 0);
		if (cfPiggy [1].file) {
			BMReadWeaponInfoD1 (cfPiggy + 1);
			CFClose (cfPiggy + 1);
			}
		else {
			int	i;

			for (i = 0; i < D1_MAX_WEAPON_TYPES; i++)
				memcpy (gameData.weapons.infoD1 [i].strength, 
						  gameData.weapons.info [i].strength, 
						  sizeof (gameData.weapons.infoD1 [i].strength));
			}
#endif

	return 1;

}

//------------------------------------------------------------------------------

int ReadSoundFile ()
{
	CFILE cfSound;
	int snd_id,snd_version;
	int nSoundNum;
	int nSoundStart;
	int size, length;

	CFOpen (&cfSound, DEFAULT_SNDFILE, gameFolders.szDataDir, "rb", 0);

	if (&cfSound == NULL)
		return 0;

	//make sure soundfile is valid nType file & is up-to-date
	snd_id = CFReadInt (&cfSound);
	snd_version = CFReadInt (&cfSound);
	if (snd_id != SNDFILE_ID || snd_version != SNDFILE_VERSION) {
		CFClose (&cfSound);						//out of date sound file
		return 0;
	}

	nSoundNum = CFReadInt (&cfSound);
	nSoundStart = CFTell (&cfSound);
	size = CFLength (&cfSound,0) - nSoundStart;
	length = size;
//	PiggyReadSounds (&cfSound);
	LoadSounds (&cfSound, nSoundNum, nSoundStart);
	CFClose (&cfSound);
	return 1;
}

//------------------------------------------------------------------------------

int PiggyInit (void)
{
	int ham_ok=0,snd_ok=0;
	int i;

/*---*/LogErr ("   Initializing hash tables\n");
	hashtable_init (bitmapNames, MAX_BITMAP_FILES);
	hashtable_init (bitmapNames + 1, D1_MAX_BITMAP_FILES);
	hashtable_init (soundNames, MAX_SOUND_FILES);
	hashtable_init (soundNames + 1, MAX_SOUND_FILES);

/*---*/LogErr ("   Initializing sound data (%d sounds)\n", MAX_SOUND_FILES);
	for (i=0; i<MAX_SOUND_FILES; i++)	{
		gameData.pig.sound.sounds [0][i].nLength [0] =
		gameData.pig.sound.sounds [0][i].nLength [1] = 0;
		gameData.pig.sound.sounds [0][i].data [0] =
		gameData.pig.sound.sounds [0][i].data [1] = NULL;
		soundOffset [0][i] = 0;
	}
/*---*/LogErr ("   Initializing bitmap index (%d indices)\n", MAX_BITMAP_FILES);
	for (i = 0; i < MAX_BITMAP_FILES; i++)     
		gameData.pig.tex.bitmapXlat [i] = i;

	if (!bogus_bitmap_initialized) {
		int i;
		ubyte c;
/*---*/LogErr ("   Initializing placeholder bitmap\n");
		bogus_bitmap_initialized = 1;
		memset (&bogus_bitmap, 0, sizeof (grsBitmap));
		bogus_bitmap.bmProps.w = 
		bogus_bitmap.bmProps.h = 
		bogus_bitmap.bmProps.rowSize = 64;
		bogus_bitmap.bmTexBuf = bogus_data;
		bogus_bitmap.bmPalette = gamePalette;
		c = GrFindClosestColor (gamePalette, 0, 0, 63);
		memset (bogus_data, c, 4096);
		c = GrFindClosestColor (gamePalette, 63, 0, 0);
		// Make a big red X !
		for (i=0; i<1024; i++) {
			bogus_data [i * 1024 + i] = c;
			bogus_data [i * 1024 + (1023 - i)] = c;
			}
		PiggyRegisterBitmap (&bogus_bitmap, "bogus", 1);
		bogusSound.nLength [0] = 1024*1024;
		bogusSound.data [0] = bogus_data;
		bitmapOffsets [0][0] =
		bitmapOffsets [1][0] = 0;
	}

	if (FindArg ("-bigpig"))
		BigPig = 1;

	if (FindArg ("-lowmem"))
		bLowMemory = 1;

	if (FindArg ("-nolowmem"))
		bLowMemory = 0;

	if (bLowMemory)
		gameStates.sound.digi.bLoMem = 1;
#if 0
	WIN (DDGRLOCK (dd_grd_curcanv));
		GrSetCurFont (SMALL_FONT);
		GrSetFontColorRGBi (DKGRAY_RGBA, 1, 0, 0);
		GrPrintF (0x8000, grdCurCanv->cv_h-20, "%s...", TXT_LOADING_DATA);
		GrUpdate (0);
	WIN (DDGRUNLOCK (dd_grd_curcanv));
#endif
/*---*/LogErr ("   Loading game data\n");
#if 1 //def EDITOR //need for d1 mission briefings
	PiggyInitPigFile (DEFAULT_PIGFILE);
#endif
/*---*/LogErr ("   Loading main ham file\n");
	snd_ok = ham_ok = ReadHamFile ();

	if (gameData.pig.tex.nHamFileVersion >= 3) {
/*---*/LogErr ("   Loading sound file\n");
		snd_ok = ReadSoundFile ();
		}
	if (gameStates.app.bFixModels)
		gameStates.app.bFixModels = gameStates.app.bDemoData ? 0 : LoadRobotReplacements ("d2x-xl", 0, 1) > 0;
	atexit (PiggyClose);
	return (ham_ok && snd_ok);               //read ok
}

//------------------------------------------------------------------------------

int PiggyIsNeeded (int nSound)
{
	int i;

if (!gameStates.sound.digi.bLoMem)
	return 1;
for (i = 0; i < MAX_SOUNDS; i++) {
	if ((AltSounds [gameStates.app.bD1Data][i] < 255) && (Sounds [gameStates.app.bD1Data] [AltSounds [gameStates.app.bD1Data][i]] == nSound))
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

#if USE_OPENAL

int PiggyBufferSound (tDigiSound *soundP)
{
if (!gameOpts->sound.bUseOpenAL)
	return 0;
if (soundP->buffer != 0xFFFFFFFF) {
	alDeleteBuffers (1, &soundP->buffer);
	soundP->buffer = 0xFFFFFFFF;
	}
alGenBuffers (1, &soundP->buffer);
if (alGetError () != AL_NO_ERROR) {
	soundP->buffer = 0xFFFFFFFF;
	gameOpts->sound.bUseOpenAL = 0;
	return 0;
	}
alBufferData (soundP->buffer, AL_FORMAT_MONO8, soundP->data [0], soundP->nLength [0], gameOpts->sound.digiSampleRate);
if (alGetError () != AL_NO_ERROR) {
	gameOpts->sound.bUseOpenAL = 0;
	return 0;
	}
return 1;
}

#endif

//------------------------------------------------------------------------------

void PiggyReadSounds (void)
{
	CFILE			cf;
	ubyte			*ptr;
	int			i, j, sbytes;
	tDigiSound	*soundP = gameData.pig.sound.pSounds;

ptr = gameData.pig.sound.data [gameStates.app.bD1Data];
sbytes = 0;
if (!CFOpen (&cf, gameStates.app.bD1Mission ? "descent.pig" : DEFAULT_SNDFILE, gameFolders.szDataDir, "rb", 0))
	return;
for (i = 0, j = gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data]; i < j; i++, soundP++) {
	if (soundOffset [gameStates.app.bD1Data][i] > 0) {
		if (PiggyIsNeeded (i)) {
			CFSeek (&cf, soundOffset [gameStates.app.bD1Data][i], SEEK_SET);

			// Read in the sound data!!!
			soundP->data [0] = ptr;
			ptr += soundP->nLength [0];
			sbytes += soundP->nLength [0];
			CFRead (soundP->data [0], soundP->nLength [0], 1, &cf);
#if USE_OPENAL
			PiggyBufferSound (soundP);
#endif
			}
		else
			soundP->data [0] = (ubyte *) -1;
		}
	}
CFClose (&cf);
#if TRACE			
	con_printf (CON_VERBOSE, "\nActual Sound usage: %d KB\n", sbytes/1024);
#endif
}

//------------------------------------------------------------------------------

extern int nDescentCriticalError;
extern unsigned descent_critical_deverror;
extern unsigned descent_critical_errcode;

char * crit_errors [13] = { "Write Protected", "Unknown Unit", "Drive Not Ready", "Unknown Command", "CRC Error", \
"Bad struct length", "Seek Error", "Unknown media nType", "Sector not found", "Printer out of paper", "Write Fault", \
"Read fault", "General Failure" };

void PiggyCriticalError ()
{
	gsrCanvas * save_canv;
	grsFont * save_font;
	int i;
	save_canv = grdCurCanv;
	save_font = grdCurCanv->cvFont;
	GrPaletteStepLoad (NULL);
	i = ExecMessageBox ("Disk Error", NULL, 2, "Retry", "Exit", "%s\non drive %c:", crit_errors [descent_critical_errcode&0xf], (descent_critical_deverror&0xf)+'A');
	if (i == 1)
		exit (1);
	GrSetCurrentCanvas (save_canv);
	grdCurCanv->cvFont = save_font;
}

//------------------------------------------------------------------------------

int FindTextureByIndex (int nIndex)
{
	int	i, j = gameData.pig.tex.nBitmaps [gameStates.app.bD1Mission];

for (i = 0; i < j; i++)
	if (gameData.pig.tex.pBmIndex [i].index == nIndex)
		return i;
return -1;
}

//------------------------------------------------------------------------------

#define	CHANGING_TEXTURE(_eP)	\
			(((_eP)->changingWallTexture >= 0) ? \
			 (_eP)->changingWallTexture : \
			 (_eP)->changingObjectTexture)

tEffectClip *FindEffect (tEffectClip *ecP, int tNum)
{
	int				h, i, j;
	tBitmapIndex	*frameP;

if (ecP)
	i = (int) (++ecP - gameData.eff.pEffects);
else {
	ecP = gameData.eff.pEffects;
	i = 0;
	}
for (h = gameData.eff.nEffects [gameStates.app.bD1Data]; i < h; i++, ecP++) {
	for (j = ecP->vc.nFrameCount, frameP = ecP->vc.frames; j > 0; j--, frameP++)
		if (frameP->index == tNum) {
#if 0
			int t = FindTextureByIndex (tNum);
			if (t >= 0)
				ecP->changingWallTexture = t;
#endif
			return ecP;
			}
	}
return NULL;
}

//------------------------------------------------------------------------------

tVideoClip *FindVClip (int tNum)
{
	int	h, i, j;
	tVideoClip *vcP = gameData.eff.vClips [0];

for (i = gameData.eff.nClips [0]; i; i--, vcP++) {
	for (h = vcP->nFrameCount, j = 0; j < h; j++)
		if (vcP->frames [j].index == tNum) {
			return vcP;
			}
	}
return NULL;
}

//------------------------------------------------------------------------------

tWallClip *FindWallAnim (int tNum)
{
	int	h, i, j;
	tWallClip *wcP = gameData.walls.pAnims;

for (i = gameData.walls.nAnims [gameStates.app.bD1Data]; i; i--, wcP++)
	for (h = wcP->nFrameCount, j = 0; j < h; j++)
		if (gameData.pig.tex.pBmIndex [wcP->frames [j]].index == tNum)
			return wcP;
return NULL;
}

//------------------------------------------------------------------------------

inline void PiggyFreeBitmapData (grsBitmap *bmP)
{
if (bmP->bmTexBuf) {
	D2_FREE (bmP->bmTexBuf);
	UseBitmapCache (bmP, (int) -bmP->bmProps.h * (int) bmP->bmProps.rowSize);
	}
}

//------------------------------------------------------------------------------

void PiggyFreeMask (grsBitmap *bmP)
{
	grsBitmap	*bmMask;

if ((bmMask = BM_MASK (bmP))) {
	OglFreeBmTexture (bmMask);
	PiggyFreeBitmapData (bmMask);
	GrFreeBitmap (bmMask);
	BM_MASK (bmP) = NULL;
	}
}

//------------------------------------------------------------------------------

int PiggyFreeHiresFrame (grsBitmap *bmP, int bD1)
{

BM_OVERRIDE (gameData.pig.tex.bitmaps [bD1] + bmP->bmHandle) = NULL;
OglFreeBmTexture (bmP);
PiggyFreeMask (bmP);
bmP->bmType = 0;
bmP->bmTexBuf = NULL;
return 1;
}

//------------------------------------------------------------------------------

int PiggyFreeHiresAnimation (grsBitmap *bmP, int bD1)
{
	grsBitmap	*altBmP, *bmfP;
	int			i;

if (!(altBmP = BM_OVERRIDE (bmP)))
	return 0;
BM_OVERRIDE (bmP) = NULL;
if (altBmP->bmType == BM_TYPE_FRAME)
	if (!(altBmP = BM_PARENT (altBmP)))
		return 1;
if (altBmP->bmType != BM_TYPE_ALT)
	return 0;	//actually this would be an error
if ((bmfP = BM_FRAMES (altBmP)))
	for (i = BM_FRAMECOUNT (altBmP); i; i--, bmfP++)
		PiggyFreeHiresFrame (bmfP, bD1);
else
	PiggyFreeMask (altBmP);
OglFreeBmTexture (altBmP);
D2_FREE (BM_FRAMES (altBmP));
altBmP->bmData.alt.bmFrameCount = 0;
PiggyFreeBitmapData (altBmP);
altBmP->bmPalette = NULL;
altBmP->bmType = 0;
return 1;
}

//------------------------------------------------------------------------------

void PiggyFreeHiresAnimations (void)
{
	int			i, bD1;
	grsBitmap	*bmP;

for (bD1 = 0, bmP = gameData.pig.tex.bitmaps [bD1]; bD1 < 2; bD1++)
	for (i = gameData.pig.tex.nBitmaps [bD1]; i; i--, bmP++)
		PiggyFreeHiresAnimation (bmP, bD1);
}

//------------------------------------------------------------------------------

void PiggyFreeBitmap (grsBitmap *bmP, int i, int bD1)
{
if (!bmP)
	bmP = gameData.pig.tex.bitmaps [bD1] + i;
else if (i < 0)
	i = (int) (bmP - gameData.pig.tex.bitmaps [bD1]);
PiggyFreeMask (bmP);
if (!PiggyFreeHiresAnimation (bmP, 0))
	OglFreeBmTexture (bmP);
if (bitmapOffsets [bD1][i] > 0)
	bmP->bmProps.flags |= BM_FLAG_PAGED_OUT;
bmP->bmFromPog = 0;
bmP->bmPalette = NULL;
PiggyFreeBitmapData (bmP);
}

//------------------------------------------------------------------------------

void PiggyBitmapPageOutAll (int bAll)
{
	int			i, bD1;
	grsBitmap	*bmP;

#if TRACE			
con_printf (CON_VERBOSE, "Flushing piggy bitmap cache\n");
#endif
gameData.pig.tex.bPageFlushed++;
TexMergeFlush ();
RLECacheFlush ();
for (bD1 = 0; bD1 < 2; bD1++) {
	bitmapCacheNext [bD1] = 0;
	for (i = 0, bmP = gameData.pig.tex.bitmaps [bD1]; 
		  i < gameData.pig.tex.nBitmaps [bD1]; 
		  i++, bmP++) {
		if (bmP->bmTexBuf && (bitmapOffsets [bD1][i] > 0)) { // only page out bitmaps read from disk
			bmP->bmProps.flags |= BM_FLAG_PAGED_OUT;
			PiggyFreeBitmap (bmP, i, bD1);
			}
		}
	}
}

//------------------------------------------------------------------------------

int IsMacDataFile (CFILE *pcf, int bD1)
{
if (pcf == cfPiggy + bD1)
	switch (CFLength (pcf, 0)) {
		default:
			if (!FindArg ("-macdata"))
				break;
		case MAC_ALIEN1_PIGSIZE:
		case MAC_ALIEN2_PIGSIZE:
		case MAC_FIRE_PIGSIZE:
		case MAC_GROUPA_PIGSIZE:
		case MAC_ICE_PIGSIZE:
		case MAC_WATER_PIGSIZE:
			return !bD1;
		case D1_MAC_PIGSIZE:
		case D1_MAC_SHARE_PIGSIZE:
			return bD1;
		}
return 0;
}

//------------------------------------------------------------------------------

void GetFlagData (char *bmName, tBitmapIndex bmi)
{
	int			i;
	tFlagData	*pf;

if (strstr (bmName, "flag01#0") == bmName)
	i = 0;
else if (strstr (bmName, "flag02#0") == bmName)
	i = 1;
else
	return;
pf = gameData.pig.flags + i;
pf->bmi = bmi;
pf->vcP = FindVClip (bmi.index);
pf->vci.nClipIndex = gameData.objs.pwrUp.info [46 + i].nClipIndex;	//46 is the blue flag powerup
pf->vci.xFrameTime = gameData.eff.vClips [0][pf->vci.nClipIndex].xFrameTime;
pf->vci.nCurFrame = 0;
}

//------------------------------------------------------------------------------

int IsAnimatedTexture (short nTexture)
{
return (nTexture > 0) && (strchr (gameData.pig.tex.bitmapFiles [gameStates.app.bD1Mission][gameData.pig.tex.pBmIndex [nTexture].index].name, '#') != NULL);
}

//------------------------------------------------------------------------------

void PiggyBitmapPageIn (tBitmapIndex bmi, int bD1)
{
	grsBitmap		*bmP, *altBmP = NULL;
	int				i, org_i, temp, nSize, nOffset, nFrames, nShrinkFactor, 
						bRedone = 0, bTGA, bDefault = 0;
	time_t			tBase, tShrunk;
	CFILE				cf = {NULL, 0, 0, 0};
	char				fn [FILENAME_LEN], fnShrunk [FILENAME_LEN], bmName [20];
	tTgaHeader		h;

	//bD1 = gameStates.app.bD1Mission;
org_i = 0;
i = bmi.index;
nShrinkFactor = 1 << (3 - gameOpts->render.textures.nQuality);
#if 0
Assert (i >= 0);
Assert (i < MAX_BITMAP_FILES);
Assert (i < gameData.pig.tex.nBitmaps [bD1]);
Assert (bitmapCacheSize > 0);
#endif
if (i < 1) 
	return;
if (i >= MAX_BITMAP_FILES) 
	return;
if (i >= gameData.pig.tex.nBitmaps [bD1]) 
	return;
if (bitmapOffsets [bD1][i] == 0) 
	return;		// A read-from-disk bmi!!!

if (bLowMemory) {
	org_i = i;
	i = gameData.pig.tex.bitmapXlat [i];          // Xlat for low-memory settings!
	}
#if 1
bmP = BmOverride (gameData.pig.tex.bitmaps [bD1] + i, -1);
if (!bmP->bmTexBuf) {
#else
bmP = gameData.pig.tex.bitmaps [bD1] + i;
if (bmP->bmProps.flags & BM_FLAG_PAGED_OUT) {
#endif
	StopTime ();
	nSize = (int) bmP->bmProps.h * (int) bmP->bmProps.rowSize;
	strcpy (bmName, gameData.pig.tex.bitmapFiles [bD1][i].name);
	GetFlagData (bmName, bmi);
#ifdef _DEBUG
	if (strstr (bmName, "exp06#0")) {
		sprintf (fn, "%s%s%s.tga", gameFolders.szTextureDir [bD1], 
					*gameFolders.szTextureDir [bD1] ? "/" : "", bmName);
		}
	else
#endif
	sprintf (fn, "%s%s%s.tga", gameFolders.szTextureDir [bD1], 
				*gameFolders.szTextureDir [bD1] ? "/" : "", bmName);
	tBase = CFDate (fn, "", 0);
	if (tBase < 0) 
		*fnShrunk = '\0';
	else {
		sprintf (fnShrunk, "%s%s%s-%d.tga", gameFolders.szTextureCacheDir [bD1], 
					*gameFolders.szTextureDir [bD1] ? "/" : "", bmName, 512 / nShrinkFactor);
		tShrunk = CFDate (fnShrunk, "", 0);
		if (tShrunk < tBase)
			*fnShrunk = '\0';
		}
	bTGA = 0;
	bmP->bmBPP = 1;
	if (gameStates.app.bNostalgia)
		gameOpts->render.textures.bUseHires = 0;
	if (*bmName && gameOpts->render.textures.bUseHires &&
		 (!gameOpts->ogl.bGlTexMerge || gameStates.render.textures.bGlsTexMergeOk)) {
#if 0
		if (ReadS3TC (gameData.pig.tex.altBitmaps [bD1] + i, gameFolders.szTextureDir [bD1], bmName)) {
			altBmP = gameData.pig.tex.altBitmaps [bD1] + i;
			altBmP->bmType = BM_TYPE_ALT;
			BM_OVERRIDE (bmP) = altBmP;
			BM_FRAMECOUNT (altBmP) = 1;
			gameData.pig.tex.bitmapFlags [bD1][i] &= ~BM_FLAG_RLE;
			gameData.pig.tex.bitmapFlags [bD1][i] |= BM_FLAG_TGA;
			bmP = altBmP;
			altBmP = NULL;
			}
		else 
#endif
		if ((gameStates.app.bCacheTextures && (nShrinkFactor > 1) && *fnShrunk && CFOpen (&cf, fnShrunk, "", "rb", 0)) || 
			 CFOpen (&cf, fn, "", "rb", 0)) {
			LogErr ("loading hires texture '%s' (quality: %d)\n", fn, gameOpts->render.nTextureQuality);
			bTGA = 1;
			altBmP = gameData.pig.tex.altBitmaps [bD1] + i;
			altBmP->bmType = BM_TYPE_ALT;
			BM_OVERRIDE (bmP) = altBmP;
			bmP = altBmP;
			ReadTGAHeader (&cf, &h, bmP);
			nSize = (int) h.width * (int) h.height * bmP->bmBPP;
			nFrames = (h.height % h.width) ? 1 : h.height / h.width;
			BM_FRAMECOUNT (bmP) = (ubyte) nFrames;
			nOffset = CFTell (&cf);
			gameData.pig.tex.bitmapFlags [bD1][i] &= ~(BM_FLAG_RLE | BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT);
			gameData.pig.tex.bitmapFlags [bD1][i] |= BM_FLAG_TGA;
			if (bmP->bmProps.h > bmP->bmProps.w) {
				tEffectClip	*ecP = NULL;
				tWallClip *wcP;
				tVideoClip *vcP;
				while ((ecP = FindEffect (ecP, i))) {
					//e->vc.nFrameCount = nFrames;
					ecP->flags |= EF_ALTFMT;
					//ecP->vc.flags |= WCF_ALTFMT;
					}
				if (!ecP) {
					if ((wcP = FindWallAnim (i))) {
					//w->nFrameCount = nFrames;
						wcP->flags |= WCF_ALTFMT;
						}
					else if ((vcP = FindVClip (i))) {
						//v->nFrameCount = nFrames;
						vcP->flags |= WCF_ALTFMT;
						}
					else {
						LogErr ("   couldn't find animation for '%s'\n", bmName);
						}
					}
				}
			}
		}
	if (!altBmP) {
		cf = cfPiggy [bD1];
		nOffset = bitmapOffsets [bD1][i];
		bDefault = 1;
		}

reloadTextures:

	if (bRedone) {
		Error ("Not enough memory for textures.\nTry to decrease texture quality\nin the advanced render options menu.");
#ifndef _DEBUG
		return;
#endif
		}
	bRedone = 1;
	if (CFSeek (&cf, nOffset, SEEK_SET)) {
		PiggyCriticalError ();
		goto reloadTextures;
		}
#if 1//def _DEBUG
strncpy (bmP->szName, bmName, sizeof (bmP->szName));
#endif
#if TEXTURE_COMPRESSION
	if (bmP->bmCompressed)
		UseBitmapCache (bmP, bmP->bmBufSize);
	else 
#endif
		{
		bmP->bmTexBuf = D2_ALLOC (nSize);
		if (bmP->bmTexBuf) 
			UseBitmapCache (bmP, nSize);
		}
	if (!bmP->bmTexBuf || (bitmapCacheUsed > bitmapCacheSize)) {
		Int3 ();
		PiggyBitmapPageOutAll (0);
		goto reloadTextures;
		}
	bmP->bmProps.flags = gameData.pig.tex.bitmapFlags [bD1][i];
	bmP->bmHandle = i;
	if (bmP->bmProps.flags & BM_FLAG_RLE) {
		int zSize = 0;
		nDescentCriticalError = 0;
		zSize = CFReadInt (&cf);
		if (nDescentCriticalError) {
			PiggyCriticalError ();
			goto reloadTextures;
			}
		temp = (int) CFRead (bmP->bmTexBuf + 4, 1, zSize-4, &cf);
		if (nDescentCriticalError) {
			PiggyCriticalError ();
			goto reloadTextures;
			}
		zSize = rle_expand (bmP, NULL, 0);
		if (bD1)
			GrRemapBitmapGood (bmP, d1Palette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
		else
			GrRemapBitmapGood (bmP, gamePalette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
		}
	else 
#if TEXTURE_COMPRESSION
		if (!bmP->bmCompressed) 
#endif
		{
		nDescentCriticalError = 0;
		if (bDefault) {
			temp = (int) CFRead (bmP->bmTexBuf, 1, nSize, &cf);
			if (bD1)
				GrRemapBitmapGood (bmP, d1Palette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
			else
				GrRemapBitmapGood (bmP, gamePalette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
			}
		else {
			ReadTGAImage (&cf, &h, bmP, -1, 1.0, 0, 0);
			if (bmP->bmProps.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
				bmP->bmProps.flags |= BM_FLAG_SEE_THRU;
			bmP->bmType = BM_TYPE_ALT;
			if (IsOpaqueDoor (i)) {
				bmP->bmProps.flags &= ~BM_FLAG_TRANSPARENT;
				bmP->bmTransparentFrames [0] &= ~1;
				}
#if TEXTURE_COMPRESSION
			if (CompressTGA (bmP))
				SaveS3TC (bmP, gameFolders.szTextureDir [bD1], bmName);
			else 
#endif
			if ((nShrinkFactor > 1) && (bmP->bmProps.w == 512) && ShrinkTGA (bmP, nShrinkFactor, nShrinkFactor, 1)) {
				nSize /= (nShrinkFactor * nShrinkFactor);
				if (gameStates.app.bCacheTextures)
					SaveTGA (bmName, gameFolders.szTextureCacheDir [bD1], &h, bmP);
				}
			}
		if (nDescentCriticalError) {
			PiggyCriticalError ();
			goto reloadTextures;
			}
		}
#ifndef MACDATA
	if (!bTGA && IsMacDataFile (&cf, bD1))
		swap_0_255 (bmP);
#endif
	StartTime ();
	}

if (bLowMemory) {
	if (org_i != i) {
		gameData.pig.tex.bitmaps [bD1][org_i] = gameData.pig.tex.bitmaps [bD1][i];
		i = org_i;
		}
	}
gameData.pig.tex.bitmaps [bD1][i].bmProps.flags &= ~BM_FLAG_PAGED_OUT;
if (!bDefault)
	CFClose (&cf);
}

//------------------------------------------------------------------------------

void PiggyLoadLevelData ()
{
LogErr ("   loading level textures\n");
PiggyBitmapPageOutAll (0);
PagingTouchAll ();
}

//------------------------------------------------------------------------------

#ifdef EDITOR

void ChangeFilenameExt (char *dest, char *src, char *ext);

void PiggyWritePigFile (char *filename)
{
	FILE *pig_fp;
	int nBmDataOffs, data_offset;
	tPIGBitmapHeader bmh;
	int org_offset;
	char subst_name [32];
	int i, j;
	FILE *fp1,*fp2;
	char tname [SHORT_FILENAME_LEN];

	for (i = 0, j = gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]; i < j; i++) {
		tBitmapIndex bi;
		bi.index = i;
		PIGGY_PAGE_IN (bi);
	}

	PiggyCloseFile ();

	pig_fp = fopen (filename, "wb");       //open PIG file
	Assert (pig_fp!=NULL);

	write_int (PIGFILE_ID,pig_fp);
	write_int (PIGFILE_VERSION,pig_fp);

	gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]--;
	fwrite (&gameData.pig.tex.nBitmaps [gameStates.app.bD1Data], sizeof (int), 1, pig_fp);
	gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]++;

	nBmDataOffs = ftell (pig_fp);
	nBmDataOffs += (gameData.pig.tex.nBitmaps [gameStates.app.bD1Data] - 1) * sizeof (tPIGBitmapHeader);
	data_offset = nBmDataOffs;

	ChangeFilenameExt (tname,filename,"lst");
	fp1 = fopen (tname, "wt");
	ChangeFilenameExt (tname,filename,"all");
	fp2 = fopen (tname, "wt");

	for (i = 1, j = gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]; i < j; i++) {
		int *size;
		grsBitmap *bmp;

		{               
			char * p, *p1;
			p = strchr (gameData.pig.tex.bitmapFiles [i].name, '#');
			if (p) {   // this is an ABM == animated bitmap
				int n;
				p1 = p; p1++; 
				n = atoi (p1);
				*p = 0;
				if (fp2 && n==0)
					fprintf (fp2, "%s.abm\n", gameData.pig.tex.pBitmapFiles[i].name);
				memcpy (bmh.name, gameData.pig.tex.bitmapFiles [i].name, 8);
				Assert (n <= DBM_NUM_FRAMES);
				bmh.dflags = DBM_FLAG_ABM + n;
				*p = '#';
			} else {
				if (fp2)
					fprintf (fp2, "%s.bbm\n", gameData.pig.tex.pBitmapFiles[i].name);
				memcpy (bmh.name, gameData.pig.tex.pBitmapFiles[i].name, 8);
				bmh.dflags = 0;
			}
		}
		bmp = gameData.pig.tex.bitmaps + i;

		Assert (!(bmp->bmProps.flags & BM_FLAG_PAGED_OUT));

		if (fp1)
			fprintf (fp1, "BMP: %s, size %d bytes", gameData.pig.tex.pBitmapFiles[i].name, bmp->bmProps.rowSize * bmp->bmProps.h);
		org_offset = ftell (pig_fp);
		bmh.offset = data_offset - nBmDataOffs;
		fseek (pig_fp, data_offset, SEEK_SET);

		if (bmp->bmProps.flags & BM_FLAG_RLE) {
			size = (int *)bmp->bmTexBuf;
			fwrite (bmp->bmTexBuf, sizeof (ubyte), *size, pig_fp);
			data_offset += *size;
			if (fp1)
				fprintf (fp1, ", and is already compressed to %d bytes.\n", *size);
		} else {
			fwrite (bmp->bmTexBuf, sizeof (ubyte), bmp->bmProps.rowSize * bmp->bmProps.h, pig_fp);
			data_offset += bmp->bmProps.rowSize * bmp->bmProps.h;
			if (fp1)
				fprintf (fp1, ".\n");
		}
		fseek (pig_fp, org_offset, SEEK_SET);
		Assert (gameData.pig.tex.bitmaps [i].bmProps.w < 4096);
		bmh.width = (gameData.pig.tex.pBitmaps[i].bmProps.w & 0xff);
		bmh.wh_extra = ((gameData.pig.tex.pBitmaps[i].bmProps.w >> 8) & 0x0f);
		Assert (gameData.pig.tex.bitmaps [i].bmProps.h < 4096);
		bmh.height = gameData.pig.tex.bitmaps [i].bmProps.h;
		bmh.wh_extra |= ((gameData.pig.tex.pBitmaps[i].bmProps.h >> 4) & 0xf0);
		bmh.flags = gameData.pig.tex.pBitmaps[i].bmProps.flags;
		if (PiggyIsSubstitutableBitmap (gameData.pig.tex.pBitmapFiles[i].name, subst_name)) {
			tBitmapIndex other_bitmap;
			other_bitmap = PiggyFindBitmap (subst_name, gameStates.app.bD1Data);
			gameData.pig.tex.bitmapXlat [i] = other_bitmap.index;
			bmh.flags |= BM_FLAG_PAGED_OUT;
		else
			bmh.flags &= ~BM_FLAG_PAGED_OUT;
			}
		bmh.bmAvgColor=gameData.pig.tex.bitmaps [i].bmAvgColor;
		fwrite (&bmh, sizeof (tPIGBitmapHeader), 1, pig_fp);  // Mark as a bitmap
		}
	fclose (pig_fp);
#if TRACE			
	con_printf (CONDBG, " Dumped %d assorted bitmaps.\n", gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]);
#endif
	fprintf (fp1, " Dumped %d assorted bitmaps.\n", gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]);

	fclose (fp1);
	fclose (fp2);

}

//------------------------------------------------------------------------------

static void write_int (int i,FILE *file)
{
	if (fwrite (&i, sizeof (i), 1, file) != 1)
		Error ("Error reading int in gamesave.c");

}

//------------------------------------------------------------------------------

void PiggyDumpAll ()
{
	int i, xlatOffset;
	FILE * fpHAM;
	int org_offset,data_offset=0;
	tPIGSoundHeader sndh;
	int sound_data_start=0;
	FILE *fp1,*fp2;

	#ifdef NO_DUMP_SOUNDS
	gameData.pig.sound.nSoundFiles [0] = 0;
	gameData.pig.sound.nSoundFiles [1] = 0;
	nSoundFilesNew = 0;
	#endif

	if (!bMustWriteHamFile && (nBitmapFilesNew == 0) && (nSoundFilesNew == 0))
		return;

	fp1 = fopen ("ham.lst", "wt");
	fp2 = fopen ("ham.all", "wt");

	if (bMustWriteHamFile || nBitmapFilesNew) {

#if TRACE			
		con_printf (CONDBG, "Creating %s...",DEFAULT_HAMFILE);
#endif

		fpHAM = fopen (DEFAULT_HAMFILE, "wb");                       //open HAM file
		Assert (fpHAM!=NULL);

		write_int (HAMFILE_ID,fpHAM);
		write_int (HAMFILE_VERSION,fpHAM);

		bm_write_all (fpHAM);
		xlatOffset = ftell (fpHAM);
		fwrite (gameData.pig.tex.bitmapXlat, sizeof (ushort)*MAX_BITMAP_FILES, 1, fpHAM);
		//Dump bitmaps

		if (nBitmapFilesNew)
			PiggyWritePigFile (DEFAULT_PIGFILE);

		//D2_FREE up memeory used by new bitmaps
		for (i=gameData.pig.tex.nBitmaps-nBitmapFilesNew;i<gameData.pig.tex.nBitmaps;i++)
			D2_FREE (gameData.pig.tex.bitmaps [i].bmTexBuf);

		//next thing must be done after pig written
		fseek (fpHAM, xlatOffset, SEEK_SET);
		fwrite (gameData.pig.tex.bitmapXlat, sizeof (ushort)*MAX_BITMAP_FILES, 1, fpHAM);

		fclose (fpHAM);
#if TRACE			
		con_printf (CONDBG, "\n");
#endif
	}

	if (nSoundFilesNew) {

#if TRACE			
		con_printf (CONDBG, "Creating %s...",DEFAULT_HAMFILE);
#endif
		// Now dump sound file
		fpHAM = fopen (DEFAULT_SNDFILE, "wb");
		Assert (fpHAM!=NULL);

		write_int (SNDFILE_ID,fpHAM);
		write_int (SNDFILE_VERSION,fpHAM);

		fwrite (&gameData.pig.sound.nSoundFiles [0], sizeof (int), 1, fpHAM);

#if TRACE			
		con_printf (CONDBG, "\nDumping sounds...");
#endif
		sound_data_start = ftell (fpHAM);
		sound_data_start += gameData.pig.sound.nSoundFiles [0]*sizeof (tPIGSoundHeader);
		data_offset = sound_data_start;

		for (i=0; i < gameData.pig.sound.nSoundFiles [0]; i++) {
			tDigiSound *snd;

			snd = &gameData.pig.sound.sounds [0][i];
			strcpy (sndh.name, sounds [0][i].name);
			sndh.length = gameData.pig.sound.sounds [0][i].nLength;
			sndh.offset = data_offset - sound_data_start;

			org_offset = ftell (fpHAM);
			fseek (fpHAM, data_offset, SEEK_SET);

			sndh.data_length = gameData.pig.sound.sounds [0][i].nLength;
			fwrite (snd->data, sizeof (ubyte), snd->length, fpHAM);
			data_offset += snd->length;
			fseek (fpHAM, org_offset, SEEK_SET);
			fwrite (&sndh, sizeof (tPIGSoundHeader), 1, fpHAM);                    // Mark as a bitmap

			fprintf (fp1, "SND: %s, size %d bytes\n", sounds [i].name, snd->length);
			fprintf (fp2, "%s.raw\n", sounds [i].name);
		}

		fclose (fpHAM);
#if TRACE			
		con_printf (CONDBG, "\n");
#endif
	}

	fprintf (fp1, "Total sound size: %d bytes\n", data_offset-sound_data_start);
#if TRACE			
	con_printf (CONDBG, " Dumped %d assorted sounds.\n", gameData.pig.sound.nSoundFiles [0]);
#endif
	fprintf (fp1, " Dumped %d assorted sounds.\n", gameData.pig.sound.nSoundFiles [0]);

	fclose (fp1);
	fclose (fp2);

	// Never allow the game to run after building ham.
	exit (0);
}

#endif

//------------------------------------------------------------------------------

void _CDECL_ PiggyClose (void)
{
	int			i, j;
	tDigiSound	*dsP;

LogErr ("unloading textures\n");
PiggyCloseFile ();
LogErr ("unloading sounds\n");
for (i = 0; i < 2; i++) {
	for (j = 0, dsP = gameData.pig.sound.sounds [i]; j < MAX_SOUND_FILES; j++, dsP++)
		if (dsP->bHires) {
			D2_FREE (dsP->data [0]);
			dsP->bHires = 0;
			}
		else if (dsP->bDTX) {
			D2_FREE (dsP->data [1]);
			dsP->bDTX = 0;
			}
	if (gameData.pig.sound.data [i])
		D2_FREE (gameData.pig.sound.data [i]);
	hashtable_free (bitmapNames + i);
	hashtable_free (soundNames + i);
	}
}

//------------------------------------------------------------------------------

int PiggyBitmapExistsSlow (char * name)
{
	int i, j;

	for (i=0, j=gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]; i<j; i++) {
		if (!strcmp (gameData.pig.tex.pBitmapFiles[i].name, name))
			return 1;
	}
	return 0;
}

//------------------------------------------------------------------------------

#define NUM_GAUGE_BITMAPS 23
char * gauge_bitmap_names [NUM_GAUGE_BITMAPS] = {
	"gauge01", "gauge01b",
	"gauge02", "gauge02b",
	"gauge06", "gauge06b",
	"targ01", "targ01b",
	"targ02", "targ02b", 
	"targ03", "targ03b",
	"targ04", "targ04b",
	"targ05", "targ05b",
	"targ06", "targ06b",
	"gauge18", "gauge18b",
	"gauss1", "helix1",
	"phoenix1"
};

//------------------------------------------------------------------------------

int PiggyIsGaugeBitmap (char * base_name)
{
	int i;
	for (i=0; i<NUM_GAUGE_BITMAPS; i++) {
		if (!stricmp (base_name, gauge_bitmap_names [i]))      
			return 1;
	}

	return 0;       
}

//------------------------------------------------------------------------------

int PiggyIsSubstitutableBitmap (char * name, char * subst_name)
{
	int frame;
	char * p;
	char base_name [ 16 ];

	strcpy (subst_name, name);
	p = strchr (subst_name, '#');
	if (p) {
		frame = atoi (&p [1]);
		*p = 0;
		strcpy (base_name, subst_name);
		if (!PiggyIsGaugeBitmap (base_name)) {
			sprintf (subst_name, "%s#%d", base_name, frame+1);
			if (PiggyBitmapExistsSlow (subst_name)) {
				if (frame & 1) {
					sprintf (subst_name, "%s#%d", base_name, frame-1);
					return 1;
				}
			}
		}
	}
	strcpy (subst_name, name);
	return 0;
}

//------------------------------------------------------------------------------
/*
 * Functions for loading replacement textures
 *  1) From .pog files
 *  2) From descent.pig (for loading d1 levels)
 */

void _CDECL_ FreeBitmapReplacements (void)
{
}

//------------------------------------------------------------------------------

void LoadBitmapReplacements (char *pszLevelName)
{
	char			szFilename [SHORT_FILENAME_LEN];
	CFILE			cf;
	int			i, j;
	grsBitmap	bm;

	//first, D2_FREE up data allocated for old bitmaps
LogErr ("   loading replacement textures\n");
FreeBitmapReplacements ();
ChangeFilenameExtension (szFilename, pszLevelName, ".pog");
if (CFOpen (&cf, szFilename, gameFolders.szDataDir, "rb", 0)) {
	int					id, version, nBitmapNum, bTGA;
	int					bmDataSize, bmDataOffset, bmOffset;
	ushort				*indices;
	tPIGBitmapHeader	*bmh;

	id = CFReadInt (&cf);
	version = CFReadInt (&cf);
	if (id != MAKE_SIG ('G','O','P','D') || version != 1) {
		CFClose (&cf);
		return;
		}
	nBitmapNum = CFReadInt (&cf);
	MALLOC (indices, ushort, nBitmapNum);
	MALLOC (bmh, tPIGBitmapHeader, nBitmapNum);
#if 0
	CFRead (indices, nBitmapNum * sizeof (ushort), 1, &cf);
	CFRead (bmh, nBitmapNum * sizeof (tPIGBitmapHeader), 1, &cf);
#else
	for (i = 0; i < nBitmapNum; i++)
		indices [i] = CFReadShort (&cf);
	for (i = 0; i < nBitmapNum; i++)
		PIGBitmapHeaderRead (bmh + i, &cf);
#endif
	bmDataOffset = CFTell (&cf);
	bmDataSize = CFLength (&cf, 0) - bmDataOffset;

	for (i = 0; i < nBitmapNum; i++) {
		bmOffset = bmh [i].offset;
		memset (&bm, 0, sizeof (grsBitmap));
		bm.bmProps.flags |= bmh [i].flags & (BM_FLAGS_TO_COPY | BM_FLAG_TGA);
		bm.bmProps.w = bm.bmProps.rowSize = bmh [i].width + ((short) (bmh [i].wh_extra & 0x0f) << 8);
		if ((bTGA = (bm.bmProps.flags & BM_FLAG_TGA)) && (bm.bmProps.w > 256))
			bm.bmProps.h = bm.bmProps.w * bmh [i].height;
		else
			bm.bmProps.h = bmh [i].height + ((short) (bmh [i].wh_extra & 0xf0) << 4);
		bm.bmBPP = bTGA ? 4 : 1;
		if (!(bm.bmProps.w * bm.bmProps.h))
			continue;
		if (bmOffset + bm.bmProps.h * bm.bmProps.rowSize > bmDataSize)
			break;
		bm.bmAvgColor = bmh [i].bmAvgColor;
		bm.bmType = BM_TYPE_ALT;
		if (!(bm.bmTexBuf = GrAllocBitmapData (bm.bmProps.w, bm.bmProps.h, bm.bmBPP)))
			break;
		CFSeek (&cf, bmDataOffset + bmOffset, SEEK_SET);
		if (bTGA) {
			int			nFrames = bm.bmProps.h / bm.bmProps.w;
			tTgaHeader	h;

			h.width = bm.bmProps.w;
			h.height = bm.bmProps.h;
			h.bits = 32;
			if (!ReadTGAImage (&cf, &h, &bm, -1, 1.0, 0, 1)) {
				D2_FREE (bm.bmTexBuf);
				break;
				}
			bm.bmProps.rowSize *= bm.bmBPP;
			bm.bmData.alt.bmFrameCount = (ubyte) nFrames;
			if (nFrames > 1) {
				tEffectClip	*ecP = NULL;
				tWallClip *wcP;
				tVideoClip *vcP;
				while ((ecP = FindEffect (ecP, indices [i]))) {
					//e->vc.nFrameCount = nFrames;
					ecP->flags |= EF_ALTFMT | EF_FROMPOG;
					}
				if (!ecP) {
					if ((wcP = FindWallAnim (indices [i]))) {
						//w->nFrameCount = nFrames;
						wcP->flags |= WCF_ALTFMT | WCF_FROMPOG;
						}
					else if ((vcP = FindVClip (i))) {
						//v->nFrameCount = nFrames;
						vcP->flags |= WCF_ALTFMT | WCF_FROMPOG;
						}
					}
				}
			j = indices [i];
			}
		else {
			int nSize = (int) bm.bmProps.w * (int) bm.bmProps.h;
			if (nSize != (int) CFRead (bm.bmTexBuf, 1, nSize, &cf)) {
				D2_FREE (bm.bmTexBuf);
				break;
				}
			bm.bmPalette = gamePalette;
			j = indices [i];
			rle_expand (&bm, NULL, 0);
			bm.bmProps = gameData.pig.tex.pBitmaps [j].bmProps;
			GrRemapBitmapGood (&bm, gamePalette, TRANSPARENCY_COLOR, SUPER_TRANSP_COLOR);
			}
		PiggyFreeBitmap (NULL, j, 0);
		bm.bmFromPog = 1;
		bm.bmHandle = j;
		if (*gameData.pig.tex.pBitmaps [j].szName)
			sprintf (bm.szName, "[%s]", gameData.pig.tex.pBitmaps [j].szName);
		else
			sprintf (bm.szName, "POG#%04d", j);
		gameData.pig.tex.pAltBitmaps [j] = bm;
		BM_OVERRIDE (gameData.pig.tex.pBitmaps + j) = gameData.pig.tex.pAltBitmaps + j;
		{
		tRgbColorf	*c;
		grsBitmap	*bmP = gameData.pig.tex.pAltBitmaps + j;

		c = BitmapColor (bmP, NULL);
		if (c && !bmP->bmAvgColor)
			bmP->bmAvgColor = GrFindClosestColor (bmP->bmPalette, (int) c->red, (int) c->green, (int) c->blue);
		}
		UseBitmapCache (gameData.pig.tex.pAltBitmaps + j, (int) bm.bmProps.h * (int) bm.bmProps.rowSize);
		}
	D2_FREE (indices);
	D2_FREE (bmh);
	CFClose (&cf);
	szLastPalettePig [0] = 0;  //force pig re-load
	TexMergeFlush ();       //for re-merging with new textures
	}
atexit (FreeBitmapReplacements);
}

//------------------------------------------------------------------------------
/* calculate table to translate d1 bitmaps to current palette,
 * return -1 on error
 */
ubyte *LoadD1Palette (void)
{
	tPalette	palette;
	CFILE cf;
	
if (!CFOpen (&cf, D1_PALETTE, gameFolders.szDataDir, "rb", 1) || (CFLength (&cf, 0) != 9472))
	return NULL;
CFRead (palette, 256, 3, &cf);
CFClose (&cf);
palette [254] = SUPER_TRANSP_COLOR;
palette [255] = TRANSPARENCY_COLOR;
return d1Palette = AddPalette (palette);
}

//------------------------------------------------------------------------------

void PiggyBitmapReadD1 (
	grsBitmap			*bmP, /* read into this bmP */
   CFILE					*cfPiggy, /* read from this file */
	int					nBmDataOffs, /* specific to file */
   tPIGBitmapHeader	*bmh, /* header info for bmP */
   ubyte					**pNextBmP, /* where to write it (if 0, use D2_ALLOC) */
	ubyte					*palette, /* what palette the bmP has */
   ubyte					*colorMap) /* how to translate bmP's colors */
{
	int zSize, bSwap0255;

memset (bmP, 0, sizeof (grsBitmap));
bmP->bmProps.w = bmP->bmProps.rowSize = bmh->width + ((short) (bmh->wh_extra&0x0f)<<8);
bmP->bmProps.h = bmh->height + ((short) (bmh->wh_extra&0xf0)<<4);
bmP->bmAvgColor = bmh->bmAvgColor;
bmP->bmProps.flags |= bmh->flags & BM_FLAGS_TO_COPY;

CFSeek (cfPiggy , nBmDataOffs + bmh->offset, SEEK_SET);
if (bmh->flags & BM_FLAG_RLE) {
	zSize = CFReadInt (cfPiggy);
	CFSeek (cfPiggy, -4, SEEK_CUR);
	}
else
	zSize = bmP->bmProps.h * bmP->bmProps.w;

if (pNextBmP) {
	bmP->bmTexBuf = *pNextBmP;
	*pNextBmP += zSize;
	}
else {
	bmP->bmTexBuf = D2_ALLOC (bmP->bmProps.h * bmP->bmProps.rowSize);
	UseBitmapCache (bmP, (int) bmP->bmProps.h * (int) bmP->bmProps.rowSize);
	}
CFRead (bmP->bmTexBuf, 1, zSize, cfPiggy);
bSwap0255 = 0;
switch (CFLength (cfPiggy,0)) {
	case D1_MAC_PIGSIZE:
	case D1_MAC_SHARE_PIGSIZE:
		if (bmh->flags & BM_FLAG_RLE)
			bSwap0255 = 1;
		else
			swap_0_255 (bmP);
		}
if (bmh->flags & BM_FLAG_RLE)
	rle_expand (bmP, NULL, bSwap0255);
GrRemapBitmapGood (bmP, d1Palette, TRANSPARENCY_COLOR, -1);
}

//------------------------------------------------------------------------------
#define D1_MAX_TEXTURES 800
#define D1_MAX_TMAP_NUM 1630 // 1621 in descent.pig Mac registered

/* the inverse of the gameData.pig.tex.bmIndex array, for descent 1.
 * "gameData.pig.tex.bmIndex" looks up a d2 bitmap index given a d2 nBaseTex
 * "d1_tmap_nums" looks up a d1 nBaseTex given a d1 bitmap. "-1" means "None"
 */
void _CDECL_ FreeD1TMapNums (void) 
{
if (d1_tmap_nums) {
	LogErr ("unloading D1 texture ids\n");
	D2_FREE (d1_tmap_nums);
	d1_tmap_nums = NULL;
	}
}

//------------------------------------------------------------------------------

void BMReadD1TMapNums (CFILE *d1pig)
{
	int i, d1_index;

	FreeD1TMapNums ();
	CFSeek (d1pig, 8, SEEK_SET);
	MALLOC (d1_tmap_nums, short, D1_MAX_TMAP_NUM);
	for (i = 0; i < D1_MAX_TMAP_NUM; i++)
		d1_tmap_nums [i] = -1;
	for (i = 0; i < D1_MAX_TEXTURES; i++) {
		d1_index = CFReadShort (d1pig);
		Assert (d1_index >= 0 && d1_index < D1_MAX_TMAP_NUM);
		d1_tmap_nums [d1_index] = i;
	}
	atexit (FreeD1TMapNums);
}

//------------------------------------------------------------------------------
/* If the given d1_index is the index of a bitmap we have to load
 * (because it is unique to descent 1), then returns the d2_index that
 * the given d1_index replaces.
 * Returns -1 if the given d1_index is not unique to descent 1.
 */
short D2IndexForD1Index (short d1_index)
{
	short d1_tmap_num = d1_tmap_nums ? d1_tmap_nums [d1_index] : -1;

	Assert (d1_index >= 0 && d1_index < D1_MAX_TMAP_NUM);
	if ((d1_tmap_num == -1) || !d1_tmap_num_unique (d1_tmap_num))
  		return -1;
	return gameData.pig.tex.bmIndex [0][ConvertD1Texture (d1_tmap_num, 0)].index;
}

//------------------------------------------------------------------------------

void LoadD1PigHeader (
	CFILE *pcf, int *pSoundNum, int *pBmHdrOffs, int *pBmDataOffs, int *pBitmapNum, int bReadTMapNums)
{

#	define D1_PIG_LOAD_FAILED "Failed loading " D1_PIGFILE

	int	nPigDataStart,
			nHeaderSize,
			nBmHdrOffs, 
			nBmDataOffs,
			nSoundNum, 
			nBitmapNum;

switch (CFLength (pcf, 0)) {
	case D1_SHARE_BIG_PIGSIZE:
	case D1_SHARE_10_PIGSIZE:
	case D1_SHARE_PIGSIZE:
	case D1_10_BIG_PIGSIZE:
	case D1_10_PIGSIZE:
		nPigDataStart = 0;
		Warning ("%s %s. %s", TXT_LOAD_FAILED, D1_PIGFILE, TXT_D1_SUPPORT);
		return;
	default:
		Warning ("%s %s", TXT_UNKNOWN_SIZE, D1_PIGFILE);
		Int3 ();
		// fall through
	case D1_PIGSIZE:
	case D1_OEM_PIGSIZE:
	case D1_MAC_PIGSIZE:
	case D1_MAC_SHARE_PIGSIZE:
		nPigDataStart = CFReadInt (pcf);
		if (bReadTMapNums)
			BMReadD1TMapNums (pcf); 
		break;
	}
CFSeek (pcf, nPigDataStart, SEEK_SET);
nBitmapNum = CFReadInt (pcf);
nSoundNum = CFReadInt (pcf);
nHeaderSize = nBitmapNum * PIGBITMAPHEADER_D1_SIZE + nSoundNum * sizeof (tPIGSoundHeader);
nBmHdrOffs = nPigDataStart + 2 * sizeof (int);
nBmDataOffs = nBmHdrOffs + nHeaderSize;
if (pSoundNum)
	*pSoundNum = nSoundNum;
*pBmHdrOffs = nBmHdrOffs;
*pBmDataOffs = nBmDataOffs;
*pBitmapNum = nBitmapNum;
}

//------------------------------------------------------------------------------

static int bHaveD1Sounds = 0;

grsBitmap bmTemp;

#define D1_BITMAPS_SIZE (128 * 1024 * 1024)

void LoadD1BitmapReplacements ()
{
	tPIGBitmapHeader	bmh;
	char					temp_name [16];
	char					temp_name_read [16];
	int					i, nBmHdrOffs, nBmDataOffs, nSoundNum, nBitmapNum;

if (cfPiggy [1].file)
	CFSeek (cfPiggy + 1, 0, SEEK_SET);
else if (!CFOpen (cfPiggy + 1, D1_PIGFILE, gameFolders.szDataDir, "rb", 0)) {
	Warning (D1_PIG_LOAD_FAILED);
	return;
	}
//first, free up data allocated for old bitmaps
FreeBitmapReplacements ();
#ifdef _DEBUG
Assert (LoadD1Palette () != NULL);
#else
LoadD1Palette ();
#endif

LoadD1PigHeader (cfPiggy + 1, &nSoundNum, &nBmHdrOffs, &nBmDataOffs, &nBitmapNum, 1);
if (gameStates.app.bD1Mission && gameStates.app.bHaveD1Data && !gameStates.app.bHaveD1Textures) {
	gameStates.app.bD1Data = 1;
	SetDataVersion (1);
	if (!bHaveD1Sounds) {
		LoadSounds (cfPiggy + 1, nSoundNum, nBmHdrOffs + nBitmapNum * PIGBITMAPHEADER_D1_SIZE);
		PiggyReadSounds ();
		bHaveD1Sounds = 1;
		}
	CFSeek (cfPiggy + 1, nBmHdrOffs, SEEK_SET);
	gameData.pig.tex.nBitmaps [1] = 0;
	PiggyRegisterBitmap (&bogus_bitmap, "bogus", 1);
	for (i = 0; i < nBitmapNum; i++) {
		PIGBitmapHeaderD1Read (&bmh, cfPiggy + 1);
		memcpy (temp_name_read, bmh.name, 8);
		temp_name_read [8] = 0;
		if (bmh.dflags & DBM_FLAG_ABM)        
			sprintf (temp_name, "%s#%d", temp_name_read, bmh.dflags & DBM_NUM_FRAMES);
		else
			strcpy (temp_name, temp_name_read);
		memset (&bmTemp, 0, sizeof (grsBitmap));
		bmTemp.bmProps.w = bmTemp.bmProps.rowSize = bmh.width + ((short) (bmh.wh_extra&0x0f)<<8);
		bmTemp.bmProps.h = bmh.height + ((short) (bmh.wh_extra&0xf0)<<4);
		bmTemp.bmProps.flags |= BM_FLAG_PAGED_OUT;
		bmTemp.bmAvgColor = bmh.bmAvgColor;
		bmTemp.bmTexBuf = D2_ALLOC (bmTemp.bmProps.w * bmTemp.bmProps.h);
		bitmapCacheUsed += bmTemp.bmProps.h * bmTemp.bmProps.w;
		gameData.pig.tex.bitmapFlags [1][i+1] = bmh.flags & BM_FLAGS_TO_COPY;
		bitmapOffsets [1][i+1] = bmh.offset + nBmDataOffs;
		Assert ((i+1) == gameData.pig.tex.nBitmaps [1]);
		PiggyRegisterBitmap (&bmTemp, temp_name, 1);
		}
	gameStates.app.bHaveD1Textures = 1;
	}
//CFClose (cfPiggy [1]);
szLastPalettePig [0]= 0;  //force pig re-load
TexMergeFlush ();       //for re-merging with new textures
}

//------------------------------------------------------------------------------

/*
 * Find and load the named bitmap from descent.pig
 * similar to ReadExtraBitmapIFF
 */
tBitmapIndex ReadExtraBitmapD1Pig (char *name)
{
	CFILE					cfPiggy;
	tPIGBitmapHeader	bmh;
	int					i, nBmHdrOffs, nBmDataOffs, nBitmapNum;
	tBitmapIndex		bmi;
	grsBitmap			*newBm = gameData.pig.tex.bitmaps [0] + gameData.pig.tex.nExtraBitmaps;

bmi.index = 0;
if (!CFOpen (&cfPiggy, D1_PIGFILE, gameFolders.szDataDir, "rb", 0)) {
	Warning (D1_PIG_LOAD_FAILED);
	return bmi;
	}
if (!gameStates.app.bHaveD1Data) {
#ifdef _DEBUG
Assert (LoadD1Palette () != NULL);
#else
LoadD1Palette ();
#endif
}
LoadD1PigHeader (&cfPiggy, NULL, &nBmHdrOffs, &nBmDataOffs, &nBitmapNum, 0);
for (i = 0; i < nBitmapNum; i++) {
	PIGBitmapHeaderD1Read (&bmh, &cfPiggy);
	if (!strnicmp (bmh.name, name, 8))
		break;
	}
if (i >= nBitmapNum) {
#if TRACE			
	con_printf (CONDBG, "could not find bitmap %s\n", name);
#endif
	return bmi;
	}
PiggyBitmapReadD1 (newBm, &cfPiggy, nBmDataOffs, &bmh, 0, d1Palette, d1ColorMap);
CFClose (&cfPiggy);
newBm->bmAvgColor = 0;	//ComputeAvgPixel (newBm);
bmi.index = gameData.pig.tex.nExtraBitmaps;
gameData.pig.tex.pBitmaps [gameData.pig.tex.nExtraBitmaps++] = *newBm;
return bmi;
}

//------------------------------------------------------------------------------

#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads a tBitmapIndex structure from a CFILE
 */
void BitmapIndexRead (tBitmapIndex *bi, CFILE *pcf)
{
bi->index = CFReadShort (pcf);
}

//------------------------------------------------------------------------------
/*
 * reads n tBitmapIndex structs from a CFILE
 */
int BitmapIndexReadN (tBitmapIndex *pbi, int n, CFILE *pcf)
{
	int		i;

for (i = 0; i < n; i++)
	pbi [i].index = CFReadShort (pcf);
return i;
}

#endif // FAST_FILE_IO

//------------------------------------------------------------------------------

typedef struct tBitmapFileHeader {
	short	bfType;
	unsigned int bfSize;
	short bfReserved1;
	short bfReserved2;
	unsigned int bfOffBits;
} tBitmapFileHeader;

typedef struct tBitmapInfoHeader {
	unsigned int biSize;
	unsigned int biWidth;
	unsigned int biHeight;
	short biPlanes;
	short biBitCount;
	unsigned int biCompression;
	unsigned int biSizeImage;
	unsigned int biXPelsPerMeter;
	unsigned int biYPelsPerMeter;
	unsigned int biClrUsed;
	unsigned int biClrImportant;
} tBitmapInfoHeader;

grsBitmap *PiggyLoadBitmap (char *pszFile)
{
	CFILE					cf;
	grsBitmap			*bmp;
	tBitmapFileHeader	bfh;
	tBitmapInfoHeader	bih;

if (!CFOpen (&cf, pszFile, gameFolders.szDataDir, "rb", 0))
	return NULL;

bfh.bfType = CFReadShort (&cf);
bfh.bfSize = (unsigned int) CFReadInt (&cf);
bfh.bfReserved1 = CFReadShort (&cf);
bfh.bfReserved2 = CFReadShort (&cf);
bfh.bfOffBits = (unsigned int) CFReadInt (&cf);

bih.biSize = (unsigned int) CFReadInt (&cf);
bih.biWidth = (unsigned int) CFReadInt (&cf);
bih.biHeight = (unsigned int) CFReadInt (&cf);
bih.biPlanes = CFReadShort (&cf);
bih.biBitCount = CFReadShort (&cf);
bih.biCompression = (unsigned int) CFReadInt (&cf);
bih.biSizeImage = (unsigned int) CFReadInt (&cf);
bih.biXPelsPerMeter = (unsigned int) CFReadInt (&cf);
bih.biYPelsPerMeter = (unsigned int) CFReadInt (&cf);
bih.biClrUsed = (unsigned int) CFReadInt (&cf);
bih.biClrImportant = (unsigned int) CFReadInt (&cf);

if (!(bmp = GrCreateBitmap (bih.biWidth, bih.biHeight, 1))) {
	CFClose (&cf);
	return NULL;
	}
CFSeek (&cf, bfh.bfOffBits, SEEK_SET);
if (CFRead (bmp->bmTexBuf, bih.biWidth * bih.biHeight, 1, &cf) != 1) {
	GrFreeBitmap (bmp);
	return NULL;
	}
CFClose (&cf);
return bmp;
}

//------------------------------------------------------------------------------
//eof
