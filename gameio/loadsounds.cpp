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

CHashTable soundNames [2];
int soundOffset [2][MAX_SOUND_FILES];
static tSoundFile sounds [2][MAX_SOUND_FILES];
static int nSoundFilesNew = 0;

//------------------------------------------------------------------------------
/*
 * reads a tPIGSoundHeader structure from a CFile
 */
void PIGSoundHeaderRead (tPIGSoundHeader *dsh, CFile& cf)
{
cf.Read (dsh->name, 8, 1);
dsh->length = cf.ReadInt ();
dsh->data_length = cf.ReadInt ();
dsh->offset = cf.ReadInt ();
}

//------------------------------------------------------------------------------

void PiggyInitSound (void)
{
memset (gameData.pig.sound.sounds, 0, sizeof (gameData.pig.sound.sounds));
}

//------------------------------------------------------------------------------

int PiggyRegisterSound (CDigiSound *soundP, char *szFileName, int nInFile)
{
	int i = gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data];

Assert (i < MAX_SOUND_FILES);
strncpy (sounds [gameStates.app.bD1Data][i].name, szFileName, 12);
soundNames [gameStates.app.bD1Data].Insert (sounds [gameStates.app.bD1Data][i].name, i);
gameData.pig.sound.soundP [i] = *soundP;
if (!nInFile)
	soundOffset [gameStates.app.bD1Data][i] = 0;
if (!nInFile)
	nSoundFilesNew++;
(gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data])++;
return i;
}

//------------------------------------------------------------------------------

int PiggyFindSound (const char * name)     
{
	int i;

i = soundNames [gameStates.app.bD1Data].Search (name);
if (i < 0)
	return 255;
return i;
}

//------------------------------------------------------------------------------

void FreeSoundReplacements (void)
{
	CDigiSound	*dsP;
	int			i, j;

PrintLog ("unloading custom sounds\n");
for (i = 0; i < 2; i++)
	for (j = 0, dsP = gameData.pig.sound.sounds [i].Buffer (); j < MAX_SOUND_FILES; j++, dsP++)
		if (dsP->bDTX) {
			dsP->data [1].Destroy ();
			dsP->bDTX = 0;
			}
}

//------------------------------------------------------------------------------

int LoadSoundReplacements (const char *pszFilename)
{
	CFile					cf;
	char					szId [4];
	int					nSounds;
	int					i, l;
	int					b11K = (gameOpts->sound.digiSampleRate == SAMPLE_RATE_11K);
	ubyte					j;
	tPIGSoundHeader	dsh;
	CDigiSound			*dsP;
	size_t				nHeaderOffs, nDataOffs;
	char					szFilename [SHORT_FILENAME_LEN];

if (gameOpts->sound.bHires)
	return -1;
//first, free up data allocated for old bitmaps
PrintLog ("   loading replacement sounds\n");
FreeSoundReplacements ();
CFile::ChangeFilenameExtension (szFilename, pszFilename, ".dtx");
if (!cf.Open (szFilename, gameFolders.szDataDir, "rb", 0))
	return -1;
if (cf.Read (szId, 1, sizeof (szId)) != sizeof (szId)) {
	cf.Close ();
	return -1;
	}
if (strncmp (szId, "DTX2", sizeof (szId))) {
	cf.Close ();
	return -1;
	}
nSounds = cf.ReadInt ();
if (!b11K)
	nSounds *= 2;
nHeaderOffs = cf.Tell ();
nDataOffs = nHeaderOffs + nSounds * sizeof (tPIGSoundHeader);
for (i = b11K ? 0 : nSounds / 2; i < nSounds; i++) {
	cf.Seek ((long) (nHeaderOffs + i * sizeof (tPIGSoundHeader)), SEEK_SET);
	PIGSoundHeaderRead (&dsh, cf);
	j = PiggyFindSound (dsh.name);
	if (j == 255)
		continue;
	dsP = gameData.pig.sound.sounds [gameStates.app.bD1Mission] + j;
	l = dsh.length;
	if (!dsP->data [1].Create (l))
		continue;
	cf.Seek ((long) (nDataOffs + dsh.offset), SEEK_SET);
	if (cf.Read (dsP->data [1].Buffer (), 1, l) != (uint) l) {
		cf.Close ();
		dsP->data [1].Destroy ();
		return -1;
		}
	dsP->bDTX = 1;
	dsP->nLength [1] = l;
	}
cf.Close ();
return 1;
}

//------------------------------------------------------------------------------

int LoadHiresSound (CDigiSound *soundP, char *pszSoundName)
{
	CFile			cf;
	char			szSoundFile [FILENAME_LEN];

if (!gameOpts->sound.bHires)
	return 0;
sprintf (szSoundFile, "%s.wav", pszSoundName);
if (!cf.Open (szSoundFile, gameFolders.szSoundDir [gameOpts->sound.bHires - 1], "rb", 0))
	return 0;
if (0 >= (soundP->nLength [0] = cf.Length ())) {
	cf.Close ();
	return 0;
	}
if (!soundP->data [0].Create (soundP->nLength [0])) {
	cf.Close ();
	return 0;
	}
if (cf.Read (soundP->data [0].Buffer (), soundP->nLength [0], 1) != 1) {
	cf.Close ();
	return 0;
	}
cf.Close ();
soundP->bHires = 1;
return 1;
}

//------------------------------------------------------------------------------

int LoadSounds (CFile& cf, int nSoundNum, int nSoundStart)
{
	tPIGSoundHeader	sndh;
	CDigiSound			sound;
	int					i;
	int					sbytes = 0;
	int 					nHeaderSize = nSoundNum * sizeof (tPIGSoundHeader);
	char					szSoundName [16];


/*---*/PrintLog ("      Loading sound data (%d sounds)\n", nSoundNum);
cf.Seek (nSoundStart, SEEK_SET);
memset (&sound, 0, sizeof (sound));
#if USE_OPENAL
memset (&sound.buffer, 0xFF, sizeof (sound.buffer));
#endif
for (i = 0; i < nSoundNum; i++) {
	PIGSoundHeaderRead (&sndh, cf);
	//size -= sizeof (tPIGSoundHeader);
	memcpy (szSoundName, sndh.name, 8);
	szSoundName [8] = 0;
	if (LoadHiresSound (&sound, szSoundName)) 
		sbytes += sound.nLength [0];
	else {
		sound.bHires = 0;
		sound.nLength [0] = sndh.length;
		sound.data [0].Create (sndh.offset + nHeaderSize + nSoundStart);
		soundOffset [gameStates.app.bD1Data][gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data]] = sndh.offset + nHeaderSize + nSoundStart;
		sbytes += sndh.length;
		}
	PiggyRegisterSound (&sound, szSoundName, 1);
	}
if (!(gameData.pig.sound.data [gameStates.app.bD1Data].Create (sbytes + 16))) {
	Error ("Not enough memory to load sounds\n");
	return 0;
	}
#if TRACE			
console.printf (CON_VERBOSE, "\nBitmapNum: %d KB   Sounds [gameStates.app.bD1Data]: %d KB\n", 
				bitmapCacheSize / 1024, sbytes / 1024);
#endif
return 1;
}

//------------------------------------------------------------------------------

int ReadSoundFile (void)
{
	CFile		cf;
	int		snd_id,sndVersion;
	int		nSoundNum;
	int		nSoundStart;
	int		size, length;

if (!cf.Open (reinterpret_cast<char*> (DEFAULT_SNDFILE), gameFolders.szDataDir, "rb", 0))
	return 0;

//make sure soundfile is valid nType file & is up-to-date
snd_id = cf.ReadInt ();
sndVersion = cf.ReadInt ();
if (snd_id != SNDFILE_ID || sndVersion != SNDFILE_VERSION) {
	cf.Close ();						//out of date sound file
	return 0;
	}

nSoundNum = cf.ReadInt ();
nSoundStart = cf.Tell ();
size = cf.Length () - nSoundStart;
length = size;
//	PiggyReadSounds ();
LoadSounds (cf, nSoundNum, nSoundStart);
cf.Close ();
return 1;
}

//------------------------------------------------------------------------------

int PiggySoundIsNeeded (int nSound)
{
	int i;

if (!gameStates.sound.digi.bLoMem)
	return 1;
for (i = 0; i < MAX_SOUNDS; i++) {
	if ((AltSounds [gameStates.app.bD1Data][i] < 255) && (Sounds [gameStates.app.bD1Data][AltSounds [gameStates.app.bD1Data][i]] == nSound))
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

#if USE_OPENAL

int PiggyBufferSound (CDigiSound *soundP)
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
	CFile			cf;
	ubyte			*ptr;
	int			i, j, sbytes;
	CDigiSound*	soundP = gameData.pig.sound.soundP.Buffer ();

ptr = gameData.pig.sound.data [gameStates.app.bD1Data].Buffer ();
sbytes = 0;
if (!cf.Open (gameStates.app.bD1Mission ? reinterpret_cast<char*> ("descent.pig") : reinterpret_cast<char*> (DEFAULT_SNDFILE), 
				  gameFolders.szDataDir, "rb", 0))
	return;
for (i = 0, j = gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data]; i < j; i++, soundP++) {
	if (soundOffset [gameStates.app.bD1Data][i] > 0) {
		if (PiggySoundIsNeeded (i)) {
			cf.Seek (soundOffset [gameStates.app.bD1Data][i], SEEK_SET);
			soundP->data [0].SetBuffer (ptr, true, soundP->nLength [0]);
			ptr += soundP->nLength [0];
			sbytes += soundP->nLength [0];
			cf.Read (soundP->data [0].Buffer (), soundP->nLength [0], 1);
#if USE_OPENAL
			PiggyBufferSound (soundP);
#endif
			}
		else
			soundP->data [0].SetBuffer (reinterpret_cast<ubyte*> (-1), true);
		}
	}
cf.Close ();
#if TRACE			
	console.printf (CON_VERBOSE, "\nActual Sound usage: %d KB\n", sbytes/1024);
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
return ((nSound < 0) || (nSound >= MAX_ADDON_SOUND_FILES)) ? reinterpret_cast<char*> ("") : addonSounds [nSound].szSoundFile;
}

//------------------------------------------------------------------------------

Mix_Chunk *LoadAddonSound (const char *pszSoundFile, ubyte *bBuiltIn)
{
	Mix_Chunk	*chunkP;
	char			szWAV [FILENAME_LEN];
	int			i;
	CFile			cf;

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
if (!(cf.Extract (pszSoundFile, gameFolders.szDataDir, 0, "d2x-temp.wav") ||
		cf.Extract (pszSoundFile, gameFolders.szSoundDir [gameOpts->sound.bHires - 1], 0, "d2x-temp.wav")))
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
