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
#include "piggy.h"
#include "gamemine.h"
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

//#define NO_DUMP_SOUNDS        1   //if set, dump bitmaps but not sounds

int RequestCD (void);

extern char CDROM_dir [];

CHashTable soundNames [2];
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

int PiggyRegisterSound (char *szFileName, int nInFile, bool bCustom)
{
	int i = gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data];

Assert (i < MAX_SOUND_FILES);
strncpy (sounds [gameStates.app.bD1Data][i].name, szFileName, 12);
soundNames [gameStates.app.bD1Data].Insert (sounds [gameStates.app.bD1Data][i].name, i);
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
		if (dsP->bCustom) {
			dsP->data [1].Destroy ();
			dsP->bCustom = 0;
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

if (gameOpts->sound.bHires [0])
	return -1;
//first, free up data allocated for old bitmaps
PrintLog ("   loading replacement sounds\n");
//FreeSoundReplacements ();
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
	if (dsP->data [1].Read (cf, l) != (size_t) l) {
		cf.Close ();
		dsP->data [1].Destroy ();
		return -1;
		}
	dsP->bCustom = 1;
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

if (!gameOpts->sound.bHires [0])
	return 0;
sprintf (szSoundFile, "%s.wav", pszSoundName);
if (!(cf.Open (szSoundFile, gameFolders.szSoundDir [2], "rb", 0) ||
	   cf.Open (szSoundFile, gameFolders.szSoundDir [gameOpts->sound.bHires [0] - 1], "rb", 0)))
	return 0;
if (0 >= (soundP->nLength [0] = cf.Length ())) {
	cf.Close ();
	return 0;
	}
if (!soundP->data [0].Create (soundP->nLength [0])) {
	cf.Close ();
	return 0;
	}
if (soundP->data [0].Read (cf, soundP->nLength [0]) != soundP->nLength [0]) {
	soundP->data [0].Destroy ();
	soundP->nLength [0] = 0;
	cf.Close ();
	return 0;
	}
cf.Close ();
soundP->bHires = 1;
return 1;
}

//------------------------------------------------------------------------------

int SetupSounds (CFile& cf, int nSoundNum, int nSoundStart, bool bCustom, bool bUseLowRes)
{
	tPIGSoundHeader	sndh;
	CDigiSound*			soundP;
	int					i, j;
	int 					nHeaderSize = nSoundNum * sizeof (tPIGSoundHeader);
	char					szSoundName [16];
	int					nSounds [2] = {0, 0};

/*---*/PrintLog ("      Loading sound data (%d sounds)\n", nSoundNum);
cf.Seek (nSoundStart, SEEK_SET);
#if USE_OPENAL
memset (&sound.buffer, 0xFF, sizeof (sound.buffer));
#endif
if (!bCustom) {
	gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data] = 0;
	soundNames [gameStates.app.bD1Data].Destroy ();
	soundNames [gameStates.app.bD1Data].Create (MAX_SOUND_FILES);
	}
for (i = 0; i < nSoundNum; i++) {
	PIGSoundHeaderRead (&sndh, cf);
	memcpy (szSoundName, sndh.name, 8);
	szSoundName [8] = 0;
	if (0 > (j = bCustom ? PiggyFindSound (szSoundName) : i))
		continue;
	soundP = &gameData.pig.sound.soundP [j];
	if (!bUseLowRes && LoadHiresSound (soundP, szSoundName))
		nSounds [1]++;
	else {
		soundP->bHires = 0;
		soundP->nLength [0] = sndh.length;
		soundP->data [0].Create (sndh.length);
		soundP->nOffset [0] = sndh.offset + nHeaderSize + nSoundStart;
		nSounds [0]++;
		}
	soundP->bCustom = 0;
	if (!bCustom)
		PiggyRegisterSound (szSoundName, 1, bCustom);
	}
return (nSounds [1] << 16) | nSounds [0];
}

//------------------------------------------------------------------------------

int PiggySoundIsNeeded (int nSound)
{
#if 1
return 1;
#else
	int i;

if (!gameStates.sound.audio.bLoMem)
	return 1;
for (i = 0; i < MAX_SOUNDS; i++) {
	if ((AltSounds [gameStates.app.bD1Data][i] < 255) && (Sounds [gameStates.app.bD1Data][AltSounds [gameStates.app.bD1Data][i]] == nSound))
		return 1;
	}
return 0;
#endif
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

void LoadSounds (CFile& cf, bool bCustom)
{
	CDigiSound*	soundP = &gameData.pig.sound.soundP [0];

for (int i = gameData.pig.sound.nSoundFiles [gameStates.app.bD1Data]; i; i--, soundP++) {
	if (soundP->nOffset [bCustom] > 0) {
		//if (PiggySoundIsNeeded (i)) 
			{
			cf.Seek (soundP->nOffset [bCustom], SEEK_SET);
			soundP->data [bCustom].Read (cf, soundP->nLength [bCustom]);
#if USE_OPENAL
			PiggyBufferSound (soundP);
#endif
			}
		}
	}
}

//------------------------------------------------------------------------------

int ReadSoundFile (bool bCustom)
{
	CFile		cf;
	int		snd_id,sndVersion;
	int		nSoundNum;
	int		nSounds;
	bool		bUseLowRes;
	char		szFile [FILENAME_LEN];
	char*		pszFile, * pszFolder;

if (gameStates.app.bNostalgia)
	gameOpts->sound.bHires [0] = 0;
else
	gameOpts->sound.bHires [0] = gameOpts->sound.bHires [1];
if (bCustom) {
	if (!*gameFolders.szModName)
		return 0;
	sprintf (szFile, "%s%s", gameFolders.szModName, (gameOpts->sound.digiSampleRate == SAMPLE_RATE_22K) ? ".s22" : ".s11");
	pszFile = szFile;
	pszFolder = gameFolders.szModDir [1];
	if (!(bUseLowRes = cf.Exist (pszFile, pszFolder, 0) != 0)) {
		pszFile = DefaultSoundFile ();
		pszFolder = gameFolders.szDataDir;
		}
	}
else {
	pszFile = DefaultSoundFile ();
	pszFolder = gameFolders.szDataDir;
	}
	
if (!cf.Open (pszFile, pszFolder, "rb", 0))
	return 0;

//make sure soundfile is valid nType file & is up-to-date
snd_id = cf.ReadInt ();
sndVersion = cf.ReadInt ();
if (snd_id != SNDFILE_ID || sndVersion != SNDFILE_VERSION) {
	cf.Close ();						//out of date sound file
	return 0;
	}
nSoundNum = cf.ReadInt ();

nSounds = SetupSounds (cf, nSoundNum, cf.Tell (), bCustom, bUseLowRes);
if (bCustom)
	gameOpts->sound.bHires [0] = (nSounds & 0xffff) ? 0 : 2;

LoadSounds (cf, false);
cf.Close ();
return nSounds != 0;
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
return ((nSound < 0) || (nSound >= MAX_ADDON_SOUND_FILES)) ? "" : addonSounds [nSound].szSoundFile;
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
		cf.Extract (pszSoundFile, gameFolders.szSoundDir [gameOpts->sound.bHires [0] - 1], 0, "d2x-temp.wav")))
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
