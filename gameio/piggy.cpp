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
#include "piggy.h"

//#define NO_DUMP_SOUNDS        1   //if set, dump bitmaps but not sounds

#ifdef _DEBUG
#	define PIGGY_MEM_QUOTA	8 //4
#else
#	define PIGGY_MEM_QUOTA	8
#endif

short *d1_tmap_nums = NULL;

ubyte bogus_data [4096*4096];
grsBitmap bogusBitmap;
ubyte bogusBitmap_initialized=0;
tDigiSound bogusSound;

#define RLE_REMAP_MAX_INCREASE 132 /* is enough for d1 pc registered */

static int bLowMemory = 0;
static int bMustWriteHamFile = 0;
static int nBitmapFilesNew = 0;

static hashtable bitmapNames [2];
size_t bitmapCacheUsed = 0;
size_t bitmapCacheSize = 0;
int bitmapCacheNext [2] = {0, 0};
int bitmapOffsets [2][MAX_BITMAP_FILES];
#ifndef _WIN32
static ubyte *bitmapBits [2] = {NULL, NULL};
#endif
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


#define PIGBITMAPHEADER_D1_SIZE 17 // no wh_extra

int ReadHamFile ();
int ReadSoundFile ();
int RequestCD (void);

CFILE cfPiggy [2] = {{NULL, 0, 0, 0}, {NULL, 0, 0, 0}};

char szCurrentPigFile [2][SHORT_FILENAME_LEN] = {"",""};

int bPigFileInitialized = 0;

ubyte bBigPig = 0;

extern char szLastPalettePig [];
extern char CDROM_dir [];


//------------------------------------------------------------------------------
/*
 * reads a tPIGBitmapHeader structure from a CFILE
 */
void PIGBitmapHeaderRead (tPIGBitmapHeader *dbh, CFILE *cfP)
{
CFRead (dbh->name, 8, 1, cfP);
dbh->dflags = CFReadByte (cfP);
dbh->width = CFReadByte (cfP);
dbh->height = CFReadByte (cfP);
dbh->wh_extra = CFReadByte (cfP);
dbh->flags = CFReadByte (cfP);
dbh->bmAvgColor = CFReadByte (cfP);
dbh->offset = CFReadInt (cfP);
}

//------------------------------------------------------------------------------
/*
 * reads a descent 1 tPIGBitmapHeader structure from a CFILE
 */
void PIGBitmapHeaderD1Read (tPIGBitmapHeader *dbh, CFILE *cfP)
{
CFRead (dbh->name, 8, 1, cfP);
dbh->dflags = CFReadByte (cfP);
dbh->width = CFReadByte (cfP);
dbh->height = CFReadByte (cfP);
dbh->wh_extra = 0;
dbh->flags = CFReadByte (cfP);
dbh->bmAvgColor = CFReadByte (cfP);
dbh->offset = CFReadInt (cfP);
}

//------------------------------------------------------------------------------

tBitmapIndex PiggyRegisterBitmap (grsBitmap *bmP, const char *name, int bInFile)
{
	tBitmapIndex temp;
	Assert (gameData.pig.tex.nBitmaps [gameStates.app.bD1Data] < MAX_BITMAP_FILES);

if (strstr (name, "door13"))
	name = name;
temp.index = gameData.pig.tex.nBitmaps [gameStates.app.bD1Data];
if (!bInFile) {
#ifdef EDITOR
	if (FindArg ("-macdata"))
		swap_0_255 (bmP);
#endif
	if (!bBigPig)  
		gr_bitmap_rle_compress (bmP);
	nBitmapFilesNew++;
	}
strncpy (gameData.pig.tex.pBitmapFiles [gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]].name, name, 12);
hashtable_insert (bitmapNames + gameStates.app.bD1Mission, 
						gameData.pig.tex.pBitmapFiles[gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]].name, 
						gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]);
gameData.pig.tex.pBitmaps [gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]] = *bmP;
if (!bInFile) {
	bitmapOffsets [gameStates.app.bD1Data][gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]] = 0;
	gameData.pig.tex.bitmapFlags [gameStates.app.bD1Data][gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]] = bmP->bmProps.flags;
	}
gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]++;
return temp;
}

//------------------------------------------------------------------------------

tBitmapIndex PiggyFindBitmap (const char * pszName, int bD1Data)   
{
	tBitmapIndex	bi;
	int				i;
	const char		*t;
	char				name [FILENAME_LEN], alias [FILENAME_LEN];

bi.index = 0;
strcpy (name, pszName);
if ((t = strchr (pszName, '#')))
	name [t - pszName] = '\0';
for (i = 0; i < gameData.pig.tex.nAliases; i++)
	if (!stricmp (name, gameData.pig.tex.aliases [i].alias_name)) {
		if (t) {	//this is a frame of an animated texture, so add the frame number
			_splitpath (gameData.pig.tex.aliases [i].file_name, NULL, NULL, alias, NULL);
			strcat (alias, "#");
			strcat (alias, t + 1);
			pszName = alias;
			}
		else
			pszName = gameData.pig.tex.aliases [i].file_name; 
		break;
		}
i = hashtable_search (bitmapNames + bD1Data, pszName);
Assert (i != 0);
bi.index = i;
return bi;
}

//------------------------------------------------------------------------------

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
//copies a pigfile from the CD to the current dir
//retuns file handle of new pig
int CopyPigFileFromCD (CFILE *cfP, char *filename)
{
	char name [80];
	FFS ffs;
	int ret;

	return CFOpen (cfP, filename, gameFolders.szDataDir, "rb", 0);
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

	return CFOpen (cfP, filename, gameFolders.szDataDir, "rb", 0);
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
#	ifdef _DEBUG
	gameStates.render.nMaxTextureQuality = 3;
#	else
	if (bitmapCacheSize > 1024 * 1024 * 1024)
		gameStates.render.nMaxTextureQuality = 3;
	else if (bitmapCacheSize > 256 * 1024 * 1024)
		gameStates.render.nMaxTextureQuality = 2;
	else
		gameStates.render.nMaxTextureQuality = 1;
#	endif
	}
#else
gameStates.render.nMaxTextureQuality = 3;
bitmapCacheSize = PIGGY_BUFFER_SIZE;
if (bLowMemory)
	bitmapCacheSize = PIGGY_SMALL_BUFFER_SIZE;
// the following loop will scale bitmap memory down by 20% for each iteration
// until memory could be allocated, and then once more to leave enough memory
// for other parts of the program
for (;;) {
	if ((bitmapBits [0] = (ubyte *) D2_ALLOC (bitmapCacheSize))) {
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
	char					szName [16];
	char					szNameRead [16];
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
	int pigVersion = CFReadInt (cfPiggy + gameStates.app.bD1Data);
	if (pig_id != PIGFILE_ID || pigVersion != PIGFILE_VERSION) {
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
	memcpy (szNameRead, bmh.name, 8);
	szNameRead [8] = 0;
	if (bmh.dflags & DBM_FLAG_ABM)        
		sprintf (szName, "%s#%d", szNameRead, bmh.dflags & DBM_NUM_FRAMES);
	else
		strcpy (szName, szNameRead);
	memset (&bmTemp, 0, sizeof (grsBitmap));
	bmTemp.bmProps.w = bmTemp.bmProps.rowSize = bmh.width + ((short) (bmh.wh_extra & 0x0f) << 8);
	bmTemp.bmProps.h = bmh.height + ((short) (bmh.wh_extra & 0xf0) << 4);
	bmTemp.bmProps.flags |= BM_FLAG_PAGED_OUT;
	bmTemp.bmAvgColor = bmh.bmAvgColor;
	gameData.pig.tex.bitmapFlags [0][i+1] = bmh.flags & BM_FLAGS_TO_COPY;
	bitmapOffsets [0][i+1] = bmh.offset + nDataStart;
	Assert ((i+1) == gameData.pig.tex.nBitmaps [0]);
	PiggyRegisterBitmap (&bmTemp, szName, 1);
	}
bPigFileInitialized = 1;
}

//------------------------------------------------------------------------------

#ifdef EDITOR
#	define MAX_BITMAPS_PER_BRUSH 30

int ComputeAvgPixel (grsBitmap *newBmp);
#endif

//reads in a new pigfile (for new palette)
//returns the size of all the bitmap data
void PiggyNewPigFile (char *pigname)
{
	int i;
	char szName [16];
	char szNameRead [16];
	grsBitmap bmTemp;
	tPIGBitmapHeader bmh;
	int nHeaderSize, nBitmapNum, nDataSize, nDataStart;
	int must_rewrite_pig = 0;

	strlwr (pigname);

	//rename pigfile for shareware
	if (!stricmp (DEFAULT_PIGFILE, DEFAULT_PIGFILE_SHAREWARE) && !CFExist (pigname, gameFolders.szDataDir,0))
		pigname = (char *) DEFAULT_PIGFILE_SHAREWARE;

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
		int pig_id,pigVersion;

		pig_id = CFReadInt (cfPiggy);
		pigVersion = CFReadInt (cfPiggy);
		if (pig_id != PIGFILE_ID || pigVersion != PIGFILE_VERSION) {
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
			memcpy (szNameRead, bmh.name, 8);
			szNameRead [8] = 0;
			if (bmh.dflags & DBM_FLAG_ABM)        
				sprintf (szName, "%s#%d", szNameRead, bmh.dflags & DBM_NUM_FRAMES);
			else
				strcpy (szName, szNameRead);
			//Make sure name matches
			if (strcmp (szName, gameData.pig.tex.bitmapFiles [0][i].name)) {
				//Int3 ();       //this pig is out of date.  Delete it
				must_rewrite_pig=1;
			}
			strcpy (gameData.pig.tex.bitmapFiles [0][i].name,szName);
			memset (&bmTemp, 0, sizeof (grsBitmap));
			bmTemp.bmProps.w = bmTemp.bmProps.rowSize = bmh.width + ((short) (bmh.wh_extra&0x0f)<<8);
			bmTemp.bmProps.h = bmh.height + ((short) (bmh.wh_extra & 0xf0)<<4);
			bmTemp.bmProps.flags |= BM_FLAG_PAGED_OUT;
			bmTemp.bmAvgColor = bmh.bmAvgColor;
			bmTemp.bmTexBuf = (ubyte *) D2_ALLOC (bmTemp.bmProps.w * bmTemp.bmProps.h);
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
				for (fnum = 0; fnum < nframes; fnum++) {
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
					if (!bBigPig) gr_bitmap_rle_compress (bm [fnum]);

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
				if (!bBigPig)  gr_bitmap_rle_compress (newBm);
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
		PiggyNewPigFile (pigname);             //read in just-generated pig
	}
	#endif  //ifdef EDITOR
}

//------------------------------------------------------------------------------

int ReadHamFile (void)
{
	CFILE cfHAM;
#if 1
	char szD1PigFileName [FILENAME_LEN];
#endif
	int nHAMId;
	int nSoundOffset = 0;

	if (!CFOpen (&cfHAM, (char *) DEFAULT_HAMFILE, gameFolders.szDataDir, "rb", 0)) {
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
/*---*/PrintLog ("      Loading bitmap index translation table\n");
		CFRead (gameData.pig.tex.bitmapXlat, sizeof (ushort)*MAX_BITMAP_FILES, 1, &cfHAM);
		// no swap here?
		//for (i = 0; i < MAX_BITMAP_FILES; i++) {
			//gameData.pig.tex.bitmapXlat [i] = INTEL_SHORT (gameData.pig.tex.bitmapXlat [i]);
			////printf ("gameData.pig.tex.bitmapXlat [%d] = %d\n", i, gameData.pig.tex.bitmapXlat [i]);
		//}
	}
	#endif

	if (gameData.pig.tex.nHamFileVersion < 3) {
		CFSeek (&cfHAM, nSoundOffset, SEEK_SET);
		int nSoundNum = CFReadInt (&cfHAM);
		int nSoundStart = CFTell (&cfHAM);
/*---*/PrintLog ("      Loading %d sounds\n", nSoundNum);
		LoadSounds (&cfHAM, nSoundNum, nSoundStart);
		}

	CFClose (&cfHAM);
#if 1
/*---*/PrintLog ("      Looking for Descent 1 data files\n");
		strcpy (szD1PigFileName, "descent.pig");
		if (!cfPiggy [1].file)
			CFOpen (cfPiggy + 1, szD1PigFileName, gameFolders.szDataDir, "rb", 0);
		if (cfPiggy [1].file) {
			gameStates.app.bHaveD1Data = 1;
/*---*/PrintLog ("      Loading Descent 1 data\n");
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

int PiggyInit (void)
{
	int bHamOk = 0, bSoundOk = 0;
	int i;

/*---*/PrintLog ("   Initializing hash tables\n");
hashtable_init (bitmapNames, MAX_BITMAP_FILES);
hashtable_init (bitmapNames + 1, D1_MAX_BITMAP_FILES);
hashtable_init (soundNames, MAX_SOUND_FILES);
hashtable_init (soundNames + 1, MAX_SOUND_FILES);

/*---*/PrintLog ("   Initializing sound data (%d sounds)\n", MAX_SOUND_FILES);
for (i=0; i<MAX_SOUND_FILES; i++)	{
	gameData.pig.sound.sounds [0][i].nLength [0] =
	gameData.pig.sound.sounds [0][i].nLength [1] = 0;
	gameData.pig.sound.sounds [0][i].data [0] =
	gameData.pig.sound.sounds [0][i].data [1] = NULL;
	soundOffset [0][i] = 0;
}
/*---*/PrintLog ("   Initializing bitmap index (%d indices)\n", MAX_BITMAP_FILES);
for (i = 0; i < MAX_BITMAP_FILES; i++)     
	gameData.pig.tex.bitmapXlat [i] = i;

if (!bogusBitmap_initialized) {
	int i;
	ubyte c;
/*---*/PrintLog ("   Initializing placeholder bitmap\n");
	bogusBitmap_initialized = 1;
	memset (&bogusBitmap, 0, sizeof (grsBitmap));
	bogusBitmap.bmProps.w = 
	bogusBitmap.bmProps.h = 
	bogusBitmap.bmProps.rowSize = 64;
	bogusBitmap.bmTexBuf = bogus_data;
	bogusBitmap.bmPalette = gamePalette;
	c = GrFindClosestColor (gamePalette, 0, 0, 63);
	memset (bogus_data, c, 4096);
	c = GrFindClosestColor (gamePalette, 63, 0, 0);
	// Make a big red X !
	for (i=0; i<1024; i++) {
		bogus_data [i * 1024 + i] = c;
		bogus_data [i * 1024 + (1023 - i)] = c;
		}
	PiggyRegisterBitmap (&bogusBitmap, "bogus", 1);
	bogusSound.nLength [0] = 1024*1024;
	bogusSound.data [0] = bogus_data;
	bitmapOffsets [0][0] =
	bitmapOffsets [1][0] = 0;
}

if (FindArg ("-bigpig"))
	bBigPig = 1;

if (FindArg ("-lowmem"))
	bLowMemory = 1;

if (FindArg ("-nolowmem"))
	bLowMemory = 0;

if (bLowMemory)
	gameStates.sound.digi.bLoMem = 1;
/*---*/PrintLog ("   Loading game data\n");
#if 1 //def EDITOR //need for d1 mission briefings
PiggyInitPigFile ((char *) DEFAULT_PIGFILE);
#endif
/*---*/PrintLog ("   Loading main ham file\n");
bSoundOk = bHamOk = ReadHamFile ();

if (gameData.pig.tex.nHamFileVersion >= 3) {
/*---*/PrintLog ("   Loading sound file\n");
	bSoundOk = ReadSoundFile ();
	}
if (gameStates.app.bFixModels)
	gameStates.app.bFixModels = gameStates.app.bDemoData ? 0 : LoadRobotReplacements ("d2x-xl", 0, 1) > 0;
LoadTextureBrightness ("descent2", gameData.pig.tex.defaultBrightness [0]);
LoadTextureBrightness ("descent", gameData.pig.tex.defaultBrightness [1]);
LoadTextureColors ("descent2", gameData.render.color.defaultTextures [0]);
LoadTextureColors ("descent", gameData.render.color.defaultTextures [1]);
atexit (PiggyClose);
return (bHamOk && bSoundOk);               //read ok
}

//------------------------------------------------------------------------------

const char * szCriticalErrors [13] = { 
	"Write Protected", "Unknown Unit", "Drive Not Ready", "Unknown Command", "CRC Error",
	"Bad struct length", "Seek Error", "Unknown media nType", "Sector not found", "Printer out of paper", 
	"Write Fault",	"Read fault", "General Failure"
	};

void PiggyCriticalError (void)
{
	gsrCanvas * save_canv;
	grsFont * save_font;
	int i;
	save_canv = grdCurCanv;
	save_font = grdCurCanv->cvFont;
	GrPaletteStepLoad (NULL);
	i = ExecMessageBox ("Disk Error", NULL, 2, "Retry", "Exit", "%s\non drive %c:", szCriticalErrors [descent_critical_errcode&0xf], (descent_critical_deverror&0xf)+'A');
	if (i == 1)
		exit (1);
	GrSetCurrentCanvas (save_canv);
	grdCurCanv->cvFont = save_font;
}

//------------------------------------------------------------------------------

int IsMacDataFile (CFILE *cfP, int bD1)
{
if (cfP == cfPiggy + bD1)
	switch (CFLength (cfP, 0)) {
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

void PiggyLoadLevelData (void)
{
PrintLog ("   loading level textures\n");
PiggyBitmapPageOutAll (0);
PagingTouchAll ();
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

void swap_0_255 (grsBitmap *bmP)
{
	int i;
	ubyte	*p;

for (i = bmP->bmProps.h * bmP->bmProps.w, p = bmP->bmTexBuf; i; i--, p++) {
	if (!*p)
		*p = 255;
	else if (*p == 255)
		*p = 0;
	}
}

//------------------------------------------------------------------------------

void PiggyBitmapReadD1 (
	grsBitmap			*bmP, /* read into this bmP */
   CFILE					*cfPiggy, /* read from this file */
	int					nBmDataOffs, /* specific to file */
   tPIGBitmapHeader	*bmh, /* header info for bmP */
   ubyte					**pNextBmP, /* where to write it (if 0, use (ubyte *) D2_ALLOC) */
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
	bmP->bmTexBuf = (ubyte *) D2_ALLOC (bmP->bmProps.h * bmP->bmProps.rowSize);
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
	PrintLog ("unloading D1 texture ids\n");
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
	CFILE *cfP, int *pSoundNum, int *pBmHdrOffs, int *pBmDataOffs, int *pBitmapNum, int bReadTMapNums)
{

#	define D1_PIG_LOAD_FAILED "Failed loading " D1_PIGFILE

	int	nPigDataStart,
			nHeaderSize,
			nBmHdrOffs, 
			nBmDataOffs,
			nSoundNum, 
			nBitmapNum;

switch (CFLength (cfP, 0)) {
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
		nPigDataStart = CFReadInt (cfP);
		if (bReadTMapNums)
			BMReadD1TMapNums (cfP); 
		break;
	}
CFSeek (cfP, nPigDataStart, SEEK_SET);
nBitmapNum = CFReadInt (cfP);
nSoundNum = CFReadInt (cfP);
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

void LoadD1BitmapReplacements (void)
{
	tPIGBitmapHeader	bmh;
	char					szNameRead [16];
	int					i, nBmHdrOffs, nBmDataOffs, nSoundNum, nBitmapNum;

if (cfPiggy [1].file)
	CFSeek (cfPiggy + 1, 0, SEEK_SET);
else if (!CFOpen (cfPiggy + 1, D1_PIGFILE, gameFolders.szDataDir, "rb", 0)) {
	Warning (D1_PIG_LOAD_FAILED);
	return;
	}
//first, free up data allocated for old bitmaps
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
	PiggyRegisterBitmap (&bogusBitmap, "bogus", 1);
	for (i = 0; i < nBitmapNum; i++) {
		PIGBitmapHeaderD1Read (&bmh, cfPiggy + 1);
		memcpy (szNameRead, bmh.name, 8);
		szNameRead [8] = 0;
		memset (&bmTemp, 0, sizeof (grsBitmap));
		if (bmh.dflags & DBM_FLAG_ABM)        
			sprintf (bmTemp.szName, "%s#%d", szNameRead, bmh.dflags & DBM_NUM_FRAMES);
		else
			strcpy (bmTemp.szName, szNameRead);
#ifdef _DEBUG
		if (strstr (bmTemp.szName, "door13"))
			i = i;
#endif
		bmTemp.bmProps.w = bmTemp.bmProps.rowSize = bmh.width + ((short) (bmh.wh_extra&0x0f)<<8);
		bmTemp.bmProps.h = bmh.height + ((short) (bmh.wh_extra&0xf0)<<4);
		bmTemp.bmProps.flags = bmh.flags | BM_FLAG_PAGED_OUT;
		bmTemp.bmAvgColor = bmh.bmAvgColor;
		bmTemp.bmTexBuf = NULL; //(ubyte *) D2_ALLOC (bmTemp.bmProps.w * bmTemp.bmProps.h);
		bmTemp.bmBPP = 1;
		bitmapCacheUsed += bmTemp.bmProps.h * bmTemp.bmProps.w;
		gameData.pig.tex.bitmapFlags [1][i+1] = bmh.flags & BM_FLAGS_TO_COPY;
		bitmapOffsets [1][i+1] = bmh.offset + nBmDataOffs;
		Assert ((i+1) == gameData.pig.tex.nBitmaps [1]);
		PiggyRegisterBitmap (&bmTemp, bmTemp.szName, 1);
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
tBitmapIndex ReadExtraBitmapD1Pig (const char *name)
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
void BitmapIndexRead (tBitmapIndex *bi, CFILE *cfP)
{
bi->index = CFReadShort (cfP);
}

//------------------------------------------------------------------------------
/*
 * reads n tBitmapIndex structs from a CFILE
 */
int BitmapIndexReadN (tBitmapIndex *pbi, int n, CFILE *cfP)
{
	int		i;

for (i = 0; i < n; i++)
	pbi [i].index = CFReadShort (cfP);
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

grsBitmap *PiggyLoadBitmap (const char *pszFile)
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

void _CDECL_ PiggyClose (void)
{
	int			i, j;
	tDigiSound	*dsP;

PrintLog ("unloading textures\n");
PiggyCloseFile ();
PrintLog ("unloading sounds\n");
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
//eof
