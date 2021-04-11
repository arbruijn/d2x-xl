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
#	pragma pack(push)
#	pragma pack(8)
#	include <windows.h>
#	pragma pack(pop)
#endif

#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "strutil.h"
#include "descent.h"
#include "gr.h"
#include "u_mem.h"
#include "iff.h"
#include "mono.h"
#include "error.h"
#include "sounds.h"
#include "songs.h"
#include "loadgamedata.h"
#include "bmread.h"
#include "hash.h"
#include "args.h"
#include "palette.h"
#include "gamefont.h"
#include "rle.h"
#include "screens.h"
#include "loadgeometry.h"
#include "textures.h"
#include "texmerge.h"
#include "paging.h"
#include "game.h"
#include "text.h"
#include "cfile.h"
#include "menu.h"
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

#if DBG
#	define PIGGY_MEM_QUOTA	8 //4
#else
#	define PIGGY_MEM_QUOTA	8
#endif

int16_t *d1_tmap_nums = NULL;

CBitmap bogusBitmap;
CSoundSample bogusSound;

#define RLE_REMAP_MAX_INCREASE 132 /* is enough for d1 pc registered */

static int32_t bMustWriteHamFile = 0;
static int32_t nBitmapFilesNew = 0;

static CHashTable bitmapNames [2];
size_t bitmapCacheUsed = 0;
size_t bitmapCacheSize = 0;
int32_t bitmapCacheNext [2] = {0, 0};
int32_t bitmapOffsets [2][MAX_BITMAP_FILES];
#ifndef _WIN32
static uint8_t *bitmapBits [2] = {NULL, NULL};
#endif
uint8_t d1ColorMap [256];

#if DBG
#	define PIGGY_BUFFER_SIZE ((uint32_t) (512*1024*1024))
#else
#	define PIGGY_BUFFER_SIZE ((uint32_t) 0x7fffffff)
#endif
#define PIGGY_SMALL_BUFFER_SIZE (16*1024*1024)

#define DBM_FLAG_ABM    64 // animated bitmap
#define DBM_NUM_FRAMES  63

#define BM_FLAGS_TO_COPY (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT | BM_FLAG_NO_LIGHTING | BM_FLAG_RLE | BM_FLAG_RLE_BIG)


#define PIGBITMAPHEADER_D1_SIZE 17 // no wh_extra

CFile cfPiggy [2];

char szCurrentPigFile [2][SHORT_FILENAME_LEN] = {"",""};

int32_t bPigFileInitialized = 0;

uint8_t bBigPig = 0;

//------------------------------------------------------------------------------

static char szPigFiles [2][11] = {"groupa.pig", "d2demo.pig"};

char* DefaultPigFile (int32_t bDemoData)
{
return szPigFiles [(bDemoData < 0) ? gameStates.app.bDemoData : bDemoData];
}

//------------------------------------------------------------------------------

static char szHamFiles [2][13] = {"descent2.ham", "d2demo.ham"};

char* DefaultHamFile (void)
{
return szHamFiles [gameStates.app.bDemoData];
}

//------------------------------------------------------------------------------

static char szSndFiles [3][13] = {"descent2.s11", "descent2.s22", "d2demo.ham"};

char* DefaultSoundFile (void)
{
if (gameData.pigData.tex.nHamFileVersion < 3) {
	gameOpts->sound.soundSampleRate = SAMPLE_RATE_11K;
	return szSndFiles [2];
	}
if (gameOpts->sound.audioSampleRate == SAMPLE_RATE_11K) {
	gameOpts->sound.soundSampleRate = SAMPLE_RATE_11K;
	return szSndFiles [0];
	}
gameOpts->sound.soundSampleRate = SAMPLE_RATE_22K;
return szSndFiles [1];
}

//------------------------------------------------------------------------------
/*
 * reads a tPIGBitmapHeader structure from a CFile
 */
void PIGBitmapHeaderRead (tPIGBitmapHeader *dbh, CFile& cf)
{
cf.Read (dbh->name, 8, 1);
dbh->dflags = cf.ReadByte ();
dbh->width = cf.ReadByte ();
dbh->height = cf.ReadByte ();
dbh->wh_extra = cf.ReadByte ();
dbh->flags = cf.ReadByte ();
dbh->avgColor = cf.ReadByte ();
dbh->offset = cf.ReadInt ();
}

//------------------------------------------------------------------------------
/*
 * reads a descent 1 tPIGBitmapHeader structure from a CFile
 */
void PIGBitmapHeaderD1Read (tPIGBitmapHeader *dbh, CFile& cf)
{
cf.Read (dbh->name, 8, 1);
dbh->dflags = cf.ReadByte ();
dbh->width = cf.ReadByte ();
dbh->height = cf.ReadByte ();
dbh->wh_extra = 0;
dbh->flags = cf.ReadByte ();
dbh->avgColor = cf.ReadByte ();
dbh->offset = cf.ReadInt ();
}

//------------------------------------------------------------------------------

tBitmapIndex PiggyRegisterBitmap (CBitmap *pBm, const char *name, int32_t bInFile)
{
	tBitmapIndex bmi;
	Assert (gameData.pigData.tex.nBitmaps [gameStates.app.bD1Data] < MAX_BITMAP_FILES);

#if DBG
if (strstr (name, "door13"))
	BRP;
#endif
bmi.index = gameData.pigData.tex.nBitmaps [gameStates.app.bD1Data];
if (!bInFile) {
	nBitmapFilesNew++;
	}
int32_t i = gameData.pigData.tex.nBitmaps [gameStates.app.bD1Data];
strncpy (gameData.pigData.tex.pBitmapFile [i].name, name, 12);
bitmapNames [gameStates.app.bD1Data].Insert (gameData.pigData.tex.pBitmapFile [i].name, i);
pBm->Clone (gameData.pigData.tex.pBitmap [i]);
pBm->SetBuffer (NULL);	//avoid automatic destruction trying to delete the same buffer twice
if (!bInFile) {
	bitmapOffsets [gameStates.app.bD1Data][i] = 0;
	gameData.pigData.tex.bitmapFlags [gameStates.app.bD1Data][i] = pBm->Flags ();
	}
gameData.pigData.tex.nBitmaps [gameStates.app.bD1Data]++;
return bmi;
}

//------------------------------------------------------------------------------

tBitmapIndex PiggyFindBitmap (const char * pszName, int32_t bD1Data)
{
	tBitmapIndex	bi;
	int32_t				i;
	const char		*t;
	char				name [FILENAME_LEN], alias [FILENAME_LEN];

bi.index = 0;
strcpy (name, pszName);
if ((t = strchr (pszName, '#')))
	name [t - pszName] = '\0';
for (i = 0; i < gameData.pigData.tex.nAliases; i++)
	if (!stricmp (name, gameData.pigData.tex.aliases [i].aliasname)) {
		if (t) {	//this is a frame of an animated texture, so add the frame number
			_splitpath (gameData.pigData.tex.aliases [i].filename, NULL, NULL, alias, NULL);
			strcat (alias, "#");
			strcat (alias, t + 1);
			pszName = alias;
			}
		else
			pszName = gameData.pigData.tex.aliases [i].filename;
		break;
		}
i = bitmapNames [bD1Data].Search (pszName);
Assert (i != 0);
bi.index = uint16_t (i);
return bi;
}

//------------------------------------------------------------------------------

void PiggyCloseFile (void)
{
	int32_t	i;

for (i = 0; i < 2; i++)
	if (cfPiggy [i].File ()) {
		cfPiggy [i].Close ();
		szCurrentPigFile [i][0] = 0;
		}
}

//------------------------------------------------------------------------------
//copies a pigfile from the CD to the current dir
//retuns file handle of new pig
int32_t CopyPigFileFromCD (CFile& cf, char *filename)
{
return cf.Open (filename, gameFolders.game.szData [0], "rb", 0);
#if 0
	char name [80];
	FFS ffs;
	int32_t ret;

messageBox.Show ("Copying bitmap data from CD...");
//paletteManager.ResumeEffect ();    //I don't think this line is really needed
//First, delete all PIG files currently in the directory
if (!FFF ("*.pig", &ffs, 0)) {
	do {
		cf.Delete (ffs.name, "");
	} while (!FFN  (&ffs, 0));
	FFC (&ffs);
}
//Now, copy over new pig
redbook.Stop ();           //so we can read off the cd
//new code to unarj file
strcpy (name, CDROM_dir);
strcat (name, "descent2.sow");
do {
//	ret = unarj_specific_file (name,filename,filename);
// DPH:FIXME
	ret = !EXIT_SUCCESS;
	if (ret != EXIT_SUCCESS) {
		//delete file, so we don't leave partial file
		cf.Delete (filename, "");
		if (RequestCD () == -1)
			//NOTE LINK TO ABOVE IF
			Error ("Cannot load file <%s> from CD", filename);
		}
	} while (ret != EXIT_SUCCESS);
mb.Clear ();
return cfPiggy [gameStates.app.bD1Data].Open (filename, gameFolders.game.szData [0], "rb", 0);
#endif
}

//------------------------------------------------------------------------------

int32_t PiggyInitMemory (void)
{
	static	int32_t bMemInited = 0;

if (bMemInited)
	return 1;
if (gameStates.app.bUseSwapFile) {
	bitmapCacheSize = 0xFFFFFFFF;
	gameStates.render.nMaxTextureQuality = 3;
	return bMemInited = 1;
	}
#if defined (_WIN32)
 {
	MEMORYSTATUS	memStat;
	GlobalMemoryStatus (&memStat);
	bitmapCacheSize = (int32_t) (memStat.dwAvailPhys / 10) * PIGGY_MEM_QUOTA;
#	if DBG
	gameStates.render.nMaxTextureQuality = 3;
#	else
	if (bitmapCacheSize > 1024 * 1024 * 1024)
		gameStates.render.nMaxTextureQuality = 3;
	else if (bitmapCacheSize > 256 * 1024 * 1024)
		gameStates.render.nMaxTextureQuality = 2;
	else
		gameStates.render.nMaxTextureQuality = 1;
#	endif
	PrintLog (0, "maximum permissible texture quality: %d\n", gameStates.render.nMaxTextureQuality);
	}
#else
gameStates.render.nMaxTextureQuality = 3;
bitmapCacheSize = PIGGY_BUFFER_SIZE;
// the following loop will scale bitmap memory down by 20% for each iteration
// until memory could be allocated, and then once more to leave enough memory
// for other parts of the program
for (;;) {
	if ((bitmapBits [0] = NEW uint8_t (bitmapCacheSize))) {
		delete[] bitmapBits [0];
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
	CFile					*pFile = cfPiggy;
	char					szName [20];
	char					szNameRead [16];
	int32_t				nHeaderSize, nBitmapNum, nDataStart, i;
	//int32_t					bD1 = gameStates.app.bD1Data;
	bool					bRegister = filename != NULL;
	bool					bReload = true;
	CBitmap				bm;
	tPIGBitmapHeader	bmh;

	static char szPigName [FILENAME_LEN] = {'\0'};

if (filename) {
	if (stricmp (szPigName, filename))
		strcpy (szPigName, filename);
	else if (pFile->File ())
		return;
	else
		bReload = false;
	}
else if (!*szPigName || pFile->File ())
	return;
if (bRegister)
	PiggyCloseFile ();             //close old pig if still open
//rename pigfile for shareware
strlwr (szPigName);
if (stricmp (szPigName, DefaultPigFile (1)) && !CFile::Exist (szPigName, gameFolders.game.szData [0], 0))
	strcpy (szPigName, DefaultPigFile (1));
if (!pFile->Open (szPigName, gameFolders.game.szData [0], "rb", 0)) {
	if (!CopyPigFileFromCD (*pFile, szPigName))
		return;
	}
if (!bReload)
	return;

int32_t pig_id = pFile->ReadInt ();
int32_t pigVersion = pFile->ReadInt ();
if (pig_id != PIGFILE_ID || pigVersion != PIGFILE_VERSION) {
	pFile->Close ();              //out of date pig
	return;
	}
strncpy (szCurrentPigFile [0], szPigName, sizeof (szCurrentPigFile [0]));
nBitmapNum = pFile->ReadInt ();
nHeaderSize = nBitmapNum * sizeof (tPIGBitmapHeader);
nDataStart = nHeaderSize + (int32_t) pFile->Tell ();
if (bRegister)
	gameData.pigData.tex.nBitmaps [0] = 1;
SetDataVersion (0);
for (i = 0; i < nBitmapNum; i++) {
	PIGBitmapHeaderRead (&bmh, *pFile);
	memcpy (szNameRead, bmh.name, 8);
	szNameRead [8] = 0;
	if (bmh.dflags & DBM_FLAG_ABM)
		sprintf (szName, "%s#%d", szNameRead, bmh.dflags & DBM_NUM_FRAMES);
	else
		strcpy (szName, szNameRead);
	bm.Reset ();
	bm.SetWidth (bmh.width + ((int16_t) (bmh.wh_extra & 0x0f) << 8));
	bm.SetHeight (bmh.height + ((int16_t) (bmh.wh_extra & 0xf0) << 4));
	bm.SetBPP (1);
	bm.SetFlags (BM_FLAG_PAGED_OUT);
	bm.SetAvgColorIndex (bmh.avgColor);
	gameData.pigData.tex.bitmapFlags [0][i+1] = bmh.flags & BM_FLAGS_TO_COPY;
	bitmapOffsets [0][i+1] = bmh.offset + nDataStart;
	if (bRegister) {
		Assert ((i+1) == gameData.pigData.tex.nBitmaps [0]);
		PiggyRegisterBitmap (&bm, szName, 1);
		}
	}
bPigFileInitialized = 1;
}

//------------------------------------------------------------------------------

int32_t ReadHamFile (int32_t nType) // 0: default, 1: level, 2: mod
{
	CFile		cf;
#if 1
	char		szD1PigFileName [FILENAME_LEN];
#endif
	int32_t	nHAMId;
	int32_t	nSoundOffset = 0;
	char		szFile [FILENAME_LEN];
	char*		pszFile, * pszFolder;

if (!nType) {
	pszFile = DefaultHamFile ();
	pszFolder = gameFolders.game.szData [0];
	}
if (nType == 1) {
	sprintf (szFile, "\x01%s", DefaultHamFile ());
	pszFile = szFile;
	pszFolder = gameFolders.game.szData [0];
	}
else if (nType == 2) {
	if (!*gameFolders.mods.szName)
		return 0;
	sprintf (szFile, "%s.ham", gameFolders.mods.szName);
	pszFile = szFile;
	pszFolder = gameFolders.mods.szCurrent;
	}

if (!cf.Open (pszFile, pszFolder, "rb", 0)) {
	bMustWriteHamFile = (nType == 0);
	return 0;
	}
//make sure ham is valid nType file & is up-to-date
nHAMId = cf.ReadInt ();
gameData.pigData.tex.nHamFileVersion = cf.ReadInt ();
if (nHAMId != HAMFILE_ID)
	Error ("Cannot open ham file %s\n", DefaultHamFile ());
if (gameData.pigData.tex.nHamFileVersion < 3) // hamfile contains sound info
	nSoundOffset = cf.ReadInt ();
BMReadAll (cf, nType == 0);
/*---*/PrintLog (0, "Loading bitmap index translation table\n");
	gameData.pigData.tex.bitmapXlat.Read (cf, MAX_BITMAP_FILES);
if (gameData.pigData.tex.nHamFileVersion < 3) {
	cf.Seek (nSoundOffset, SEEK_SET);
	int32_t nSoundNum = cf.ReadInt ();
	int32_t nSoundStart = (int32_t) cf.Tell ();
/*---*/PrintLog (1, "Loading %d sounds\n", nSoundNum);
	SetupSounds (cf, nSoundNum, nSoundStart);
	PrintLog (-1);
	}
cf.Close ();
if (!nType) {
	/*---*/PrintLog (0, "Looking for Descent 1 data files\n");
	strcpy (szD1PigFileName, "descent.pig");
	if (cfPiggy [1].File ())
		cfPiggy [1].Seek (0, SEEK_SET);
	else
		cfPiggy [1].Open (szD1PigFileName, gameFolders.game.szData [0], "rb", 0);
	if (cfPiggy [1].File ()) {
		gameStates.app.bHaveD1Data = 1;
		BMReadGameDataD1 (cfPiggy [1]);
		PrintLog (-1);
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t PiggyInit (void)
{
	int32_t bHamOk = 0, bSoundOk = 0;
	int32_t i;

PrintLog (1);
/*---*/PrintLog (0, "Initializing hash tables\n");
bitmapNames [0].Create (MAX_BITMAP_FILES);
bitmapNames [1].Create (D1_MAX_BITMAP_FILES);
soundNames [0].Create (MAX_SOUND_FILES);
soundNames [1].Create (MAX_SOUND_FILES);

#if 0
/*---*/PrintLog (1, "Initializing sound data (%d sounds)\n", MAX_SOUND_FILES);
for (i = 0; i < MAX_SOUND_FILES; i++)
	soundOffset [0][i] = 0;
#endif

/*---*/PrintLog (0, "Initializing bitmap index (%d indices)\n", MAX_BITMAP_FILES);
for (i = 0; i < MAX_BITMAP_FILES; i++)
	gameData.pigData.tex.bitmapXlat [i] = i;

if (!bogusBitmap.FrameSize ()) {
	int32_t i;
	uint8_t c;
/*---*/PrintLog (0, "Initializing placeholder bitmap\n");
	bogusBitmap.Setup (0, 64, 64, 1, "Bogus Bitmap");
	bogusBitmap.SetPalette (paletteManager.Game ());
	c = paletteManager.Game ()->ClosestColor (0, 0, 63);
	memset (bogusBitmap.Buffer (), c, 4096);
	c = paletteManager.Game ()->ClosestColor (63, 0, 0);
	// Make a big red X !
	for (i = 0; i < 64; i++) {
		bogusBitmap [i * 64 + i] = c;
		bogusBitmap [i * 64 + (64 - i)] = c;
		}
	PiggyRegisterBitmap (&bogusBitmap, "bogus", 1);
	bogusSound.nLength [0] = 1024*1024;
	bogusSound.data [0].ShareBuffer (bogusBitmap);
	bitmapOffsets [0][0] =
	bitmapOffsets [1][0] = 0;
}

if (FindArg ("-bigpig"))
	bBigPig = 1;

/*---*/PrintLog (1, "Loading game data\n");
PiggyInitPigFile (DefaultPigFile (-1));
PrintLog (-1);
/*---*/PrintLog (1, "Loading main ham file\n");
bSoundOk = bHamOk = ReadHamFile ();
PrintLog (-1);
gameData.pigData.sound.nType = -1; //none loaded
if (gameData.pigData.tex.nHamFileVersion >= 3) {
/*---*/PrintLog (1, "Loading sound file\n");
	bSoundOk = LoadD2Sounds ();
	PrintLog (-1);
	}
//if (gameStates.app.bFixModels)
#if 1 //!DBG
gameStates.app.bAltModels =
gameStates.app.bFixModels = gameStates.app.bDemoData ? 0 : LoadRobotReplacements ("d2x-xl", NULL, 0, 1) > 0;
#endif
LoadTextureBrightness ("descent2", gameData.pigData.tex.defaultBrightness [0].Buffer ());
LoadTextureBrightness ("descent", gameData.pigData.tex.defaultBrightness [1].Buffer ());
LoadTextureColors ("descent2", gameData.renderData.color.defaultTextures [0].Buffer ());
LoadTextureColors ("descent", gameData.renderData.color.defaultTextures [1].Buffer ());
atexit (PiggyClose);
PrintLog (-1);
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
gameData.renderData.screen.Activate ("PiggyCriticalError");
int32_t i = InfoBox ("Disk Error", (pMenuCallback) NULL, BG_STANDARD, 2, "Retry", "Exit", "%s\non drive %c:", 
							szCriticalErrors [descent_critical_errcode & 0xf], (descent_critical_deverror & 0xf) + 'A');
gameData.renderData.screen.Deactivate ();
if (i == 1)
	exit (1);
}

//------------------------------------------------------------------------------

int32_t IsMacDataFile (CFile *pFile, int32_t bD1)
{
if (pFile == cfPiggy + bD1) {
	switch (pFile->Length ()) {
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

		default:
			return gameStates.app.bMacData;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int32_t ReadBitmap (CFile* pFile, CBitmap* pBm, int32_t nSize, bool bD1);

int32_t PiggyBitmapReadD1 (
	CFile					&cf,
	CBitmap*				pBm, /* read into this pBm */
	int32_t				nBmDataOffs, /* specific to file */
   tPIGBitmapHeader*	bmh, /* header info for pBm */
   uint8_t**			pNextBmP, /* where to write it (if 0, use reinterpret_cast<uint8_t*> (D2_ALLOC) */
   uint8_t*				colorMap) /* how to translate pBm's colors */
{
memset (pBm, 0, sizeof (CBitmap));
pBm->SetWidth (bmh->width + ((int16_t) (bmh->wh_extra&0x0f)<<8));
pBm->SetHeight (bmh->height + ((int16_t) (bmh->wh_extra&0xf0)<<4));
pBm->SetBPP (1);
pBm->SetAvgColorIndex (bmh->avgColor);
pBm->AddFlags (bmh->flags & BM_FLAGS_TO_COPY);
pBm->CreateBuffer ();

cf.Seek (nBmDataOffs + bmh->offset, SEEK_SET);

#if 1

if (ReadBitmap (&cf, pBm, pBm->FrameSize (), 1) < 0)
	return 0;

#else

int32_t zSize;

if (bmh->flags & BM_FLAG_RLE) 
	zSize = cf.ReadInt () - 4;
else
	zSize = pBm->Width () * pBm->Width ();

if (pNextBmP) {
	pBm->SetBuffer (*pNextBmP);
	*pNextBmP += zSize;
	}
else {
	if (pBm->CreateBuffer ())
		UseBitmapCache (pBm, (int32_t) pBm->FrameSize ());
	else
		return 0;
	}
pBm->Read (cf, zSize);
int32_t bSwapTranspColor = 0;
switch (cf.Length ()) {
	case D1_MAC_PIGSIZE:
	case D1_MAC_SHARE_PIGSIZE:
		if (bmh->flags & BM_FLAG_RLE)
			bSwapTranspColor = 1;
		else
			pBm->SwapTransparencyColor ();
		}
if (bmh->flags & BM_FLAG_RLE)
	pBm->RLEExpand (NULL, bSwapTranspColor);

#endif

pBm->SetPalette (paletteManager.D1 (), TRANSPARENCY_COLOR, -1);
return 1;
}

//------------------------------------------------------------------------------
#define D1_MAX_TEXTURES 800
#define D1_MAX_TMAP_NUM 1630 // 1621 in descent.pig Mac registered

/* the inverse of the gameData.pigData.tex.bmIndex array, for descent 1.
 * "gameData.pigData.tex.bmIndex" looks up a d2 bitmap index given a d2 nBaseTex
 * "d1_tmap_nums" looks up a d1 nBaseTex given a d1 bitmap. "-1" means "None"
 */
void _CDECL_ FreeD1TMapNums (void)
{
if (d1_tmap_nums) {
	PrintLog (0, "unloading D1 texture ids\n");
	delete[] d1_tmap_nums;
	d1_tmap_nums = NULL;
	}
}

//------------------------------------------------------------------------------

void BMReadD1TMapNums (CFile& cf)
{
	int32_t i, d1_index;

	FreeD1TMapNums ();
	cf.Seek (8, SEEK_SET);
	d1_tmap_nums = NEW int16_t [D1_MAX_TMAP_NUM];
	for (i = 0; i < D1_MAX_TMAP_NUM; i++)
		d1_tmap_nums [i] = -1;
	for (i = 0; i < D1_MAX_TEXTURES; i++) {
		d1_index = cf.ReadShort ();
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
int16_t D2IndexForD1Index (int16_t d1_index)
{
	int16_t d1_tmap_num = d1_tmap_nums ? d1_tmap_nums [d1_index] : -1;

	Assert (d1_index >= 0 && d1_index < D1_MAX_TMAP_NUM);
	if ((d1_tmap_num == -1) || !d1_tmap_num_unique (d1_tmap_num))
  		return -1;
	return gameData.pigData.tex.bmIndex [0][ConvertD1Texture (d1_tmap_num, 0)].index;
}

//------------------------------------------------------------------------------

int32_t LoadD1PigHeader (CFile& cf, int32_t *soundNumP, int32_t *pBmHdrOffs, int32_t *pBmDataOffs, int32_t *pBitmapNum, int32_t bReadTMapNums)
{
	int32_t	nPigDataStart,
			nHeaderSize,
			nBmHdrOffs,
			nBmDataOffs,
			nSoundNum,
			nBitmapNum;

switch (cf.Length ()) {
	case D1_SHARE_BIG_PIGSIZE:
	case D1_SHARE_10_PIGSIZE:
	case D1_SHARE_PIGSIZE:
	case D1_10_BIG_PIGSIZE:
	case D1_10_PIGSIZE:
		nPigDataStart = 0;
		Warning ("%s %s. %s", TXT_LOAD_FAILED, D1_PIGFILE, TXT_D1_SUPPORT);
		return 0;
	default:
		Warning ("%s %s", TXT_UNKNOWN_SIZE, D1_PIGFILE);
		Int3 ();
		// fall through
	case D1_PIGSIZE:
	case D1_OEM_PIGSIZE:
	case D1_MAC_PIGSIZE:
	case D1_MAC_SHARE_PIGSIZE:
		nPigDataStart = cf.ReadInt ();
		if (bReadTMapNums)
			BMReadD1TMapNums (cf);
		break;
	}
cf.Seek (nPigDataStart, SEEK_SET);
nBitmapNum = cf.ReadInt ();
nSoundNum = int32_t (cf.ReadShort ());
cf.Seek (sizeof (int16_t), SEEK_CUR);	//skip another 2 bytes
nHeaderSize = nBitmapNum * PIGBITMAPHEADER_D1_SIZE + nSoundNum * sizeof (tPIGSoundHeader);
nBmHdrOffs = nPigDataStart + 2 * sizeof (int32_t);
nBmDataOffs = nBmHdrOffs + nHeaderSize;
if (soundNumP)
	*soundNumP = nSoundNum;
*pBmHdrOffs = nBmHdrOffs;
*pBmDataOffs = nBmDataOffs;
*pBitmapNum = nBitmapNum;
return 1;
}

//------------------------------------------------------------------------------

#define D1_BITMAPS_SIZE (128 * 1024 * 1024)

//------------------------------------------------------------------------------

void LoadD1Textures (void)
{
	tPIGBitmapHeader	bmh;
	CBitmap				bm;
	char					szName [20];
	char					szNameRead [16];
	int32_t				i, nBmHdrOffs, nBmDataOffs, nSounds, nBitmaps;

PiggyInitPigFile (const_cast<char*> ("groupa.pig"));
SetDataVersion (1);
if (cfPiggy [1].File ())
	cfPiggy [1].Seek (0, SEEK_SET);
else if (!cfPiggy [1].Open (D1_PIGFILE, gameFolders.game.szData [0], "rb", 0)) {
	Warning (D1_PIG_LOAD_FAILED);
	return;
	}
//first, free up data allocated for old bitmaps
paletteManager.LoadD1 ();

if (!LoadD1PigHeader (cfPiggy [1], &nSounds, &nBmHdrOffs, &nBmDataOffs, &nBitmaps, 1)) {
	PrintLog (0, "Warning Failed to load Descent 1 textures\n");
	return;
	}
gameStates.app.bD1Data = 1;
if (gameStates.app.bD1Mission && gameStates.app.bHaveD1Data && !gameStates.app.bHaveD1Textures) {
	cfPiggy [1].Seek (nBmHdrOffs, SEEK_SET);
	gameData.pigData.tex.nBitmaps [1] = 0;
	PiggyRegisterBitmap (&bogusBitmap, "bogus", 1);
	for (i = 0; i < nBitmaps; i++) {
		PIGBitmapHeaderD1Read (&bmh, cfPiggy [1]);
		memcpy (szNameRead, bmh.name, 8);
		szNameRead [8] = 0;

		memset (&bm, 0, sizeof (bm)); // important, because this texture will be cloned and must not contain any pointers to itself
		if (bmh.dflags & DBM_FLAG_ABM) {
			sprintf (szName, "%s#%d", szNameRead, bmh.dflags & DBM_NUM_FRAMES);
			}
		else
			strcpy (szName, szNameRead);
#if DBG
		if (strstr (bm.Name (), "door13"))
			BRP;
#endif
		bm.SetWidth (bmh.width + ((int16_t) (bmh.wh_extra & 0x0f) << 8));
		bm.SetHeight (bmh.height + ((int16_t) (bmh.wh_extra & 0xf0) << 4));
		bm.SetBPP (1);
		bm.SetFlags (bmh.flags | BM_FLAG_PAGED_OUT);
		bm.SetAvgColorIndex (bmh.avgColor);
		bm.SetBuffer (NULL);
		bm.SetBPP (1);
		bitmapCacheUsed += bm.FrameSize ();
		gameData.pigData.tex.bitmapFlags [1][i+1] = bmh.flags & BM_FLAGS_TO_COPY;
		bitmapOffsets [1][i+1] = bmh.offset + nBmDataOffs;
		Assert ((i+1) == gameData.pigData.tex.nBitmaps [1]);
		PiggyRegisterBitmap (&bm, szName, 1);
		}
	gameStates.app.bHaveD1Textures = 1;
	}
paletteManager.SetLastPig ("");  //force pig re-load
TexMergeFlush ();       //for re-merging with new textures
}

//------------------------------------------------------------------------------

/*
 * Find and load the named bitmap from descent.pig
 * similar to LoadExitModelIFF
 */
tBitmapIndex ReadExtraBitmapD1Pig (const char *name)
{
	CFile					cf;
	tPIGBitmapHeader	bmh;
	int32_t				i, nBmHdrOffs, nBmDataOffs, nBitmapNum;
	tBitmapIndex		bmi;
	CBitmap*				newBm = gameData.pigData.tex.bitmaps [1] + gameData.pigData.tex.nExtraBitmaps;

bmi.index = 0;
if (!cf.Open (D1_PIGFILE, gameFolders.game.szData [0], "rb", 0)) {
	Warning (D1_PIG_LOAD_FAILED);
	return bmi;
	}
if (!gameStates.app.bHaveD1Data)
	paletteManager.LoadD1 ();
if (!LoadD1PigHeader (cf, NULL, &nBmHdrOffs, &nBmDataOffs, &nBitmapNum, 0))
	return bmi;
for (i = 0; i < nBitmapNum; i++) {
	PIGBitmapHeaderD1Read (&bmh, cf);
	if (!strnicmp (bmh.name, name, 8))
		break;
	}
if (i >= nBitmapNum) {
#if TRACE
	console.printf (CON_DBG, "could not find bitmap %s\n", name);
#endif
	return bmi;
	}
bmi.index = (uint16_t) PiggyBitmapReadD1 (cf, newBm, nBmDataOffs, &bmh, 0, d1ColorMap);
cf.Close ();
if (bmi.index) {
	newBm->SetAvgColorIndex (0);
	bmi.index = gameData.pigData.tex.nExtraBitmaps;
	gameData.pigData.tex.pBitmap [gameData.pigData.tex.nExtraBitmaps++] = *newBm;
	}
return bmi;
}

//------------------------------------------------------------------------------

/*
 * reads a tBitmapIndex structure from a CFile
 */
void ReadBitmapIndex (tBitmapIndex *bi, CFile& cf)
{
bi->index = cf.ReadShort ();
}

//------------------------------------------------------------------------------
/*
 * reads n tBitmapIndex structs from a CFile
 */
int32_t ReadBitmapIndices (CArray<tBitmapIndex>& bi, int32_t n, CFile& cf, int32_t o)
{
	int32_t		i;

for (i = 0; i < n; i++)
	bi [i + o].index = cf.ReadShort ();
return i;
}

//------------------------------------------------------------------------------

typedef struct tBitmapFileHeader {
	int16_t	bfType;
	uint32_t bfSize;
	int16_t bfReserved1;
	int16_t bfReserved2;
	uint32_t bfOffBits;
} tBitmapFileHeader;

typedef struct tBitmapInfoHeader {
	uint32_t biSize;
	uint32_t biWidth;
	uint32_t biHeight;
	int16_t biPlanes;
	int16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	uint32_t biXPelsPerMeter;
	uint32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} tBitmapInfoHeader;

CBitmap *PiggyLoadBitmap (const char *pszFile)
{
	CFile					cf;
	CBitmap				*pBm;
	tBitmapFileHeader	bfh;
	tBitmapInfoHeader	bih;

if (!cf.Open (pszFile, gameFolders.game.szData [0], "rb", 0))
	return NULL;

bfh.bfType = cf.ReadShort ();
bfh.bfSize = (uint32_t) cf.ReadInt ();
bfh.bfReserved1 = cf.ReadShort ();
bfh.bfReserved2 = cf.ReadShort ();
bfh.bfOffBits = (uint32_t) cf.ReadInt ();

bih.biSize = (uint32_t) cf.ReadInt ();
bih.biWidth = (uint32_t) cf.ReadInt ();
bih.biHeight = (uint32_t) cf.ReadInt ();
bih.biPlanes = cf.ReadShort ();
bih.biBitCount = cf.ReadShort ();
bih.biCompression = (uint32_t) cf.ReadInt ();
bih.biSizeImage = (uint32_t) cf.ReadInt ();
bih.biXPelsPerMeter = (uint32_t) cf.ReadInt ();
bih.biYPelsPerMeter = (uint32_t) cf.ReadInt ();
bih.biClrUsed = (uint32_t) cf.ReadInt ();
bih.biClrImportant = (uint32_t) cf.ReadInt ();

if (!(pBm = CBitmap::Create (0, bih.biWidth, bih.biHeight, 1))) {
	cf.Close ();
	return NULL;
	}
pBm->SetName (pszFile);
cf.Seek (bfh.bfOffBits, SEEK_SET);
if (pBm->Read (cf, bih.biWidth * bih.biHeight) != bih.biWidth * bih.biHeight) {
	delete pBm;
	return NULL;
	}
cf.Close ();
return pBm;
}

//------------------------------------------------------------------------------

void _CDECL_ PiggyClose (void)
{
PrintLog (1, "unloading sounds and textures\n");
PiggyCloseFile ();
PrintLog (-1);
for (int32_t i = 0; i < 2; i++) {
	UnloadSounds (i);
	bitmapNames [i].Destroy ();
	}
PrintLog (-1);
}

//------------------------------------------------------------------------------
//eof
