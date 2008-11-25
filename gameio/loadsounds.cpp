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
#include <ctype.h>

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

int RequestCD (void);
int ReadSoundFile ();

extern char CDROM_dir [];

tHashTable soundNames [2];
int soundOffset [2][MAX_SOUND_FILES];
static tSoundFile sounds [2][MAX_SOUND_FILES];
static int nSoundFilesNew = 0;

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
HashTableInsert (&soundNames [gameStates.app.bD1Data], 
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

int PiggyFindSound (const char * name)     
{
	int i;

i = HashTableSearch (&soundNames [gameStates.app.bD1Data], name);
if (i < 0)
	return 255;
return i;
}

//------------------------------------------------------------------------------

void FreeSoundReplacements (void)
{
	tDigiSound	*dsP;
	int			i, j;

PrintLog ("unloading custom sounds\n");
for (i = 0; i < 2; i++)
	for (j = 0, dsP = gameData.pig.sound.sounds [i]; j < MAX_SOUND_FILES; j++, dsP++)
		if (dsP->bDTX) {
			D2_FREE (dsP->data [1]);
			dsP->bDTX = 0;
			}
}

//------------------------------------------------------------------------------

int LoadSoundReplacements (const char *pszFilename)
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
PrintLog ("   loading replacement sounds\n");
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
	if (!(dsP->data [1] = (ubyte *) (ubyte *) D2_ALLOC (l)))
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
if (!(soundP->data [0] = (ubyte *) (ubyte *) D2_ALLOC (soundP->nLength [0]))) {
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


/*---*/PrintLog ("      Loading sound data (%d sounds)\n", nSoundNum);
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
if (!(gameData.pig.sound.data [gameStates.app.bD1Data] = (ubyte *) D2_ALLOC (sbytes + 16))) {
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

int ReadSoundFile (void)
{
	CFILE cfSound;
	int snd_id,sndVersion;
	int nSoundNum;
	int nSoundStart;
	int size, length;

CFOpen (&cfSound, (char *) DEFAULT_SNDFILE, gameFolders.szDataDir, "rb", 0);

if (&cfSound == NULL)
	return 0;

//make sure soundfile is valid nType file & is up-to-date
snd_id = CFReadInt (&cfSound);
sndVersion = CFReadInt (&cfSound);
if (snd_id != SNDFILE_ID || sndVersion != SNDFILE_VERSION) {
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

int PiggySoundIsNeeded (int nSound)
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
if (!CFOpen (&cf, gameStates.app.bD1Mission ? (char *) "descent.pig" : (char *) DEFAULT_SNDFILE, gameFolders.szDataDir, "rb", 0))
	return;
for (i = 0, j = gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data]; i < j; i++, soundP++) {
	if (soundOffset [gameStates.app.bD1Data][i] > 0) {
		if (PiggySoundIsNeeded (i)) {
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

#if USE_SDL_MIXER

#include "SDL_mixer.h"

typedef struct tAddonSound {
	Mix_Chunk		*chunkP;
	/*const*/ char		szSoundFile [FILENAME_LEN];
} tAddonSound;

tAddonSound addonSounds [MAX_ADDON_SOUND_FILES] = {
	{NULL, "00:missileflight-small.wav"},
	{NULL, "01:missileflight-big.wav"},
	{NULL, "02:vulcan-firing.wav"},
	{NULL, "03:gauss-firing.wav"},
	{NULL, "04:gatling-speedup.wav"},
	{NULL, "05:flareburning.wav"},
	{NULL, "06:lowping.wav"},
	{NULL, "07:highping.wav"},
	{NULL, "08:lightng.wav"},
	{NULL, "09:slowdown.wav"},
	{NULL, "10:speedup.wav"},
	{NULL, "11:airbubbles.wav"}
	};

//------------------------------------------------------------------------------

const char *AddonSoundName (int nSound)
{
return ((nSound < 0) || (nSound >= MAX_ADDON_SOUND_FILES)) ? (char *) "" : addonSounds [nSound].szSoundFile;
}

//------------------------------------------------------------------------------

Mix_Chunk *LoadAddonSound (const char *pszSoundFile, ubyte *bBuiltIn)
{
	Mix_Chunk	*chunkP;
	char			szWAV [FILENAME_LEN];
	int			i;

if (!::isdigit (*pszSoundFile))
	i = -1;
else {
	i = atoi (pszSoundFile);
	if (i >= MAX_ADDON_SOUND_FILES)
		return NULL;
	*bBuiltIn = 1;
	if ((chunkP = addonSounds [i].chunkP))
		return chunkP;
	pszSoundFile += 3;
	}
if (!(CFExtract (pszSoundFile, gameFolders.szDataDir, 0, "d2x-temp.wav") ||
		CFExtract (pszSoundFile, gameFolders.szSoundDir [gameOpts->sound.bHires - 1], 0, "d2x-temp.wav")))
	return NULL;
sprintf (szWAV, "%s%sd2x-temp.wav", gameFolders.szCacheDir, *gameFolders.szCacheDir ? "/" : "");
if (!(chunkP = Mix_LoadWAV (szWAV)))
	return NULL;
if (i >= 0)
	addonSounds [i].chunkP = chunkP;
*bBuiltIn = (i >= 0);
return chunkP;
}

//------------------------------------------------------------------------------

void FreeAddonSounds (void)
{
for (int i = 0; i < MAX_ADDON_SOUND_FILES; i++) {
	if (addonSounds [i].chunkP) {
		Mix_FreeChunk (addonSounds [i].chunkP);
		addonSounds [i].chunkP = NULL;
		}
	}
}

#endif

//------------------------------------------------------------------------------
//eof
