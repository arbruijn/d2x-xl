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
#include "rwops.h"

//#define NO_DUMP_SOUNDS        1   //if set, dump bitmaps but not sounds

int32_t RequestCD (void);

extern char CDROM_dir [];

CHashTable soundNames [2];
static tSoundFile sounds [2][MAX_SOUND_FILES];
static int32_t nSoundFilesNew = 0;

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
memset (gameData.pigData.sound.sounds, 0, sizeof (gameData.pigData.sound.sounds));
}

//------------------------------------------------------------------------------

int32_t PiggyRegisterSound (char *szFileName, int32_t bFromFile, bool bCustom)
{
	int32_t i = gameData.pigData.sound.nSoundFiles [gameStates.app.bD1Mission];

Assert (i < MAX_SOUND_FILES);
strncpy (sounds [gameStates.app.bD1Mission][i].name, szFileName, 12);
soundNames [gameStates.app.bD1Mission].Insert (sounds [gameStates.app.bD1Mission][i].name, i);
if (!bFromFile)
	nSoundFilesNew++;
(gameData.pigData.sound.nSoundFiles [gameStates.app.bD1Mission])++;
return i;
}

//------------------------------------------------------------------------------

int32_t PiggyFindSound (const char * name)
{
	int32_t i = soundNames [gameStates.app.bD1Mission].Search (name);
if (i < 0)
	return -1;
return i;
}

//------------------------------------------------------------------------------

void FreeSoundReplacements (void)
{
for (int32_t i = 0; i < 2; i++) {
	CSoundSample* pSample = gameData.pigData.sound.sounds [i].Buffer ();
	for (int32_t j = 0; j < MAX_SOUND_FILES; j++, pSample++) {
		if (pSample->bCustom) {
			pSample->data [1].Destroy ();
			pSample->bCustom = 0;
			}
		}
	}
}

//------------------------------------------------------------------------------

int32_t LoadSoundReplacements (const char *pszFilename)
{
	CFile					cf;
	char					szId [4];
	int32_t				nLoadedSounds;
	int32_t				i, j, l;
	int32_t				b11K = (gameOpts->sound.audioSampleRate == SAMPLE_RATE_11K);
	tPIGSoundHeader	dsh;
	CSoundSample*		pSample;
	size_t				nHeaderOffs, nDataOffs;
	char					szFilename [FILENAME_LEN];

if (gameOpts->UseHiresSound ())
	return -1;
//first, free up data allocated for old bitmaps
PrintLog (1, "loading replacement sounds\n");
//FreeSoundReplacements ();
CFile::ChangeFilenameExtension (szFilename, pszFilename, ".dtx");
if (!cf.Open (szFilename, gameFolders.game.szData [0], "rb", 0)) {
	PrintLog (-1);
	return -1;
	}
if (cf.Read (szId, 1, sizeof (szId)) != sizeof (szId)) {
	cf.Close ();
	PrintLog (-1);
	return -1;
	}
if (strncmp (szId, "DTX2", sizeof (szId))) {
	cf.Close ();
	PrintLog (-1);
	return -1;
	}
nLoadedSounds = cf.ReadInt ();
if (!b11K)
	nLoadedSounds *= 2;
nHeaderOffs = cf.Tell ();
nDataOffs = nHeaderOffs + nLoadedSounds * sizeof (tPIGSoundHeader);
for (i = b11K ? 0 : nLoadedSounds / 2; i < nLoadedSounds; i++) {
	cf.Seek ((long) (nHeaderOffs + i * sizeof (tPIGSoundHeader)), SEEK_SET);
	PIGSoundHeaderRead (&dsh, cf);
	j = PiggyFindSound (dsh.name);
	if (j < 0)
		continue;
	pSample = gameData.pigData.sound.sounds [gameStates.app.bD1Mission] + j;
	l = dsh.length;
	if (!pSample->data [1].Create (l))
		continue;
	cf.Seek ((long) (nDataOffs + dsh.offset), SEEK_SET);
	if (pSample->data [1].Read (cf, l) != (size_t) l) {
		cf.Close ();
		pSample->data [1].Destroy ();
		return -1;
		}
	pSample->bCustom = 1;
	pSample->nLength [1] = l;
	}
cf.Close ();
PrintLog (-1);
return 1;
}

//------------------------------------------------------------------------------

int32_t LoadHiresSound (CSoundSample* pSound, char* pszSoundName, bool bCustom)
{
	CFile			cf;
	char			szSoundFile [FILENAME_LEN];

sprintf (szSoundFile, "%s.wav", pszSoundName);
if (!((*gameFolders.mods.szSounds [1] && cf.Open (szSoundFile, gameFolders.mods.szSounds [1], "rb", 0)) ||
	   (*gameFolders.mods.szSounds [0] && cf.Open (szSoundFile, gameFolders.mods.szSounds [0], "rb", 0)) ||
	   ((/*bCustom ||*/ gameOpts->UseHiresSound ()) && cf.Open (szSoundFile, gameFolders.game.szSounds [HIRES_SOUND_FOLDER (gameStates.sound.bD1Sound)], "rb", 0))))
	return 0;
if (0 >= (pSound->nLength [0] = (int32_t) cf.Length ())) {
	cf.Close ();
	return 0;
	}
if (!pSound->data [0].Create (pSound->nLength [0], "CSoundSample::data [0]")) {
	cf.Close ();
	return 0;
	}
if (pSound->data [0].Read (cf, pSound->nLength [0]) != uint32_t (pSound->nLength [0])) {
	pSound->data [0].Destroy ();
	pSound->nLength [0] = 0;
	cf.Close ();
	return 0;
	}
cf.Close ();
pSound->bHires = 1;
return 1;
}

//------------------------------------------------------------------------------

int32_t SetupSounds (CFile& cf, int32_t nSounds, int32_t nSoundStart, bool bCustom, bool bUseLowRes)
{
	tPIGSoundHeader	sndh;
	CSoundSample*		pSound;
	int32_t				i, j;
	int32_t 				nHeaderSize = nSounds * sizeof (tPIGSoundHeader);
	char					szSoundName [16];
	int32_t				nLoadedSounds [2] = {0, 0};

/*---*/PrintLog (1, "Loading sound data (%d sounds)\n", nSounds);
cf.Seek (nSoundStart, SEEK_SET);
#if USE_OPENAL
memset (&sound.buffer, 0xFF, sizeof (sound.buffer));
#endif
if (!bCustom) {
	gameData.pigData.sound.nSoundFiles [gameStates.app.bD1Mission] = 0;
	soundNames [gameStates.app.bD1Mission].Destroy ();
	soundNames [gameStates.app.bD1Mission].Create (MAX_SOUND_FILES);
	}
for (i = 0; i < nSounds; i++) {
	PIGSoundHeaderRead (&sndh, cf);
	memcpy (szSoundName, sndh.name, 8);
	szSoundName [8] = 0;
#if 0
	PrintLog (0, "%3d: %s\n", i, szSoundName);
#endif
	if (0 > (j = bCustom ? PiggyFindSound (szSoundName) : i))
		continue;
	pSound = &gameData.pigData.sound.pSound [j];
	if (!bUseLowRes && LoadHiresSound (pSound, szSoundName, bCustom)) {
		nLoadedSounds [1]++;
		pSound->nOffset [0] = 0;
		}
	else {
		if (!bUseLowRes)
			PrintLog (0, "Couldn't find hires sound %s.wav.\n", szSoundName);
		pSound->bHires = 0;
		pSound->nLength [0] = sndh.length;
		pSound->data [0].Create (sndh.length);
		pSound->nOffset [0] = sndh.offset + nHeaderSize + nSoundStart;
		nLoadedSounds [0]++;
		}
	memcpy (pSound->szName, szSoundName, sizeof (pSound->szName));
	pSound->bCustom = 0;
	if (!bCustom)
		PiggyRegisterSound (szSoundName, 1, bCustom);
	}
PrintLog (-1);
return (nLoadedSounds [1] << 16) | nLoadedSounds [0];
}

//------------------------------------------------------------------------------

int32_t PiggySoundIsNeeded (int32_t nSound)
{
#if 1
return 1;
#else
	int32_t i;

if (!gameStates.sound.audio.bLoMem)
	return 1;
for (i = 0; i < MAX_SOUNDS; i++) {
	if ((AltSounds [gameStates.app.bD1Mission][i] < 255) && (Sounds [gameStates.app.bD1Mission][AltSounds [gameStates.app.bD1Mission][i]] == nSound))
		return 1;
	}
return 0;
#endif
}

//------------------------------------------------------------------------------

#if USE_OPENAL

int32_t PiggyBufferSound (CSoundSample *pSound)
{
if (!gameOpts->sound.bUseOpenAL)
	return 0;
if (pSound->buffer != 0xFFFFFFFF) {
	alDeleteBuffers (1, &pSound->buffer);
	pSound->buffer = 0xFFFFFFFF;
	}
alGenBuffers (1, &pSound->buffer);
if (alGetError () != AL_NO_ERROR) {
	pSound->buffer = 0xFFFFFFFF;
	gameOpts->sound.bUseOpenAL = 0;
	return 0;
	}
alBufferData (pSound->buffer, AL_FORMAT_MONO8, pSound->data [0], pSound->nLength [0], gameOpts->sound.audioSampleRate);
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
	CSoundSample*	pSound = &gameData.pigData.sound.pSound [0];

for (int32_t i = gameData.pigData.sound.nSoundFiles [gameStates.app.bD1Mission]; i; i--, pSound++) {
	if (pSound->nOffset [bCustom] > 0) {
		//if (PiggySoundIsNeeded (i))
			{
			cf.Seek (pSound->nOffset [bCustom], SEEK_SET);
			pSound->data [bCustom].Read (cf, pSound->nLength [bCustom]);
#if USE_OPENAL
			PiggyBufferSound (pSound);
#endif
			}
		}
	}
}


//------------------------------------------------------------------------------

void UnloadSounds (int32_t bD1)
{
PrintLog (1, "unloading sounds\n");
CSoundSample* pSound = gameData.pigData.sound.sounds [bD1].Buffer ();
for (int32_t j = 0; j < MAX_SOUND_FILES; j++, pSound++)
	if (pSound->bHires) {
		pSound->data [0].Destroy ();
		pSound->bHires = 0;
		}
	else if (pSound->bCustom) {
		pSound->data [1].Destroy ();
		pSound->bCustom = 0;
		}
soundNames [bD1].Destroy ();
}

//------------------------------------------------------------------------------

bool VerifyHiresSound (bool bCustom, int32_t nLoadedSounds)
{
if (bCustom) {
	if (!(nLoadedSounds & 0xFFFF)) {	// -> all default sounds are hires
		gameOpts->sound.bHires [0] = gameOpts->sound.bHires [1];
		}
	}
else if (gameOpts->sound.bHires [0]) { 
	if (nLoadedSounds & 0xFFFF) {	// -> not all default sounds are hires
		gameOpts->sound.bHires [0] = 0;
		audio.Reset ();
		return false;
		}
	}
return true;
}

//------------------------------------------------------------------------------

int32_t LoadD2Sounds (bool bCustom)
{
if (!(gameData.pigData.sound.nType || bCustom))
	return 1;

	CFile			cf;
	int32_t		sndId, sndVersion;
	int32_t		nSounds;
	int32_t		nLoadedSounds = 0;
	bool			bUseLowRes = false;
	char			szFile [FILENAME_LEN];
	char			* pszFile, * pszFolder;

if (gameStates.app.bNostalgia)
	gameOpts->sound.bHires [0] = 0;
#if 0
else if (gameOpts->sound.bHires [0] < 0)
	gameOpts->sound.bHires [0] = 0;
else
	gameOpts->sound.bHires [0] = gameOpts->sound.bHires [1];
#endif
if (bCustom) {
	if (!*gameFolders.mods.szName)
		return 0;
	sprintf (szFile, "%s%s", gameFolders.mods.szName, (gameOpts->sound.audioSampleRate >= SAMPLE_RATE_22K) ? ".s22" : ".s11");
	pszFile = szFile;
	pszFolder = gameFolders.mods.szCurrent;
	if (!(bUseLowRes = cf.Exist (pszFile, pszFolder, 0) != 0)) {
		pszFile = DefaultSoundFile ();
		pszFolder = gameFolders.game.szData [0];
		sprintf (gameFolders.mods.szSounds [1], "%s%s", gameFolders.mods.szSounds [0], LevelFolder (missionManager.nCurrentLevel));
		}
	}
else {
	pszFile = DefaultSoundFile ();
	pszFolder = gameFolders.game.szData [0];
	bUseLowRes = !gameOpts->UseHiresSound ();
	}

if (!cf.Open (pszFile, pszFolder, "rb", 0)) {
	PrintLog (0, "Couldn't open sound file '%s%s'\n", pszFolder ? pszFolder : "", pszFile ? pszFile : "");
	return 0;
	}

if (gameStates.app.bDemoData) 
	gameOpts->sound.soundSampleRate = SAMPLE_RATE_11K;
else {
	//make sure soundfile is valid nType file & is up-to-date
	sndId = cf.ReadInt ();
	sndVersion = cf.ReadInt ();
	if ((sndId != SNDFILE_ID) || (sndVersion != SNDFILE_VERSION)) {
		cf.Close ();						//out of date sound file
		PrintLog (0, "Invalid sound file '%s%s'\n", pszFolder ? pszFolder : "", pszFile ? pszFile : "");
		return 0;
		}
	nSounds = cf.ReadInt ();

	nLoadedSounds = SetupSounds (cf, nSounds, (int32_t) cf.Tell (), bCustom, bUseLowRes);

#if 1
	if (!VerifyHiresSound (bCustom, nLoadedSounds))
		return LoadD2Sounds (bCustom);
#else
	if (bCustom)
		gameOpts->sound.bHires [0] = (nLoadedSounds & 0xffff) ? 0 : 2;
	else if (gameOpts->sound.bHires [0] && ((nLoadedSounds >> 16) == 0))
		gameOpts->sound.bHires [0] = gameOpts->sound.bHires [1] = 0;
#endif
	}

LoadSounds (cf, false);
cf.Close ();
return nLoadedSounds != 0;
}

//------------------------------------------------------------------------------

bool LoadD1Sounds (bool bCustom)
{
	int32_t	nBmHdrOffs, nBmDataOffs, nSounds, nBitmaps;

if (cfPiggy [1].File ())
	cfPiggy [1].Seek (0, SEEK_SET);
else if (!cfPiggy [1].Open (D1_PIGFILE, gameFolders.game.szData [0], "rb", 0)) {
	Warning (D1_PIG_LOAD_FAILED);
	return false;
	}
LoadD1PigHeader (cfPiggy [1], &nSounds, &nBmHdrOffs, &nBmDataOffs, &nBitmaps, 1);
if (gameStates.app.bD1Mission && gameStates.app.bHaveD1Data) {
	gameStates.app.bD1Data = 1;
	SetDataVersion (1);
	SetD1Sound ();
	if ((gameData.pigData.sound.nType != 1) || gameStates.app.bCustomSounds || bCustom) {
		if (bCustom)
			sprintf (gameFolders.mods.szSounds [1], "%s%s", gameFolders.mods.szSounds [0], LevelFolder (missionManager.nCurrentLevel));
		int32_t nLoadedSounds = SetupSounds (cfPiggy [1], nSounds, nBmHdrOffs + nBitmaps * PIGBITMAPHEADER_D1_SIZE, bCustom, false);
		if (!VerifyHiresSound (bCustom, nLoadedSounds))
			return LoadD1Sounds (bCustom);
		LoadSounds (cfPiggy [1]);
		gameData.pigData.sound.nType = 1;
		if (gameStates.sound.bD1Sound)
			gameOpts->sound.soundSampleRate = SAMPLE_RATE_11K;
		}
	}
return nSounds > 0;
}

//------------------------------------------------------------------------------

#if USE_SDL_MIXER

#	ifdef __macosx__
#		include <SDL_mixer/SDL_mixer.h>
#	else
#		include <SDL_mixer.h>
#	endif

tAddonSound addonSounds [MAX_ADDON_SOUND_FILES] = {
	 {NULL, "00:missileflight-small.wav"},
	 {NULL, "01:missileflight-big.wav"},
	 {NULL, "02:vulcan-firing.wav"},
	 {NULL, "03:gauss-firing.wav"},
	 {NULL, "04:gatling-speedup.wav"},
	 {NULL, "05:flareburning.wav"},
	 {NULL, "06:lowping.wav"},
	 {NULL, "07:highping.wav"},
	 {NULL, "08:lightning.wav"},
	 {NULL, "09:slowdown.wav"},
	 {NULL, "10:speedup.wav"},
	 {NULL, "11:airbubbles.wav"},
	 {NULL, "12:zoom1.wav"},
	 {NULL, "13:zoom2.wav"},
	 {NULL, "14:headlight.wav"},
	 {NULL, "15:boiling-lava.wav"},
	 {NULL, "16:busy-machine.wav"},
	 {NULL, "17:computer.wav"},
	 {NULL, "18:deep-hum.wav"},
	 {NULL, "19:dripping-water.wav"},
	 {NULL, "20:dripping-water-2.wav"},
	 {NULL, "21:dripping-water-3.wav"},
	 {NULL, "22:earthquake.wav"},
	 {NULL, "23:energy.wav"},
	 {NULL, "24:falling-rocks.wav"},
	 {NULL, "25:falling-rocks-2.wav"},
	 {NULL, "26:fast-fan.wav"},
	 {NULL, "27:flowing-lava.wav"},
	 {NULL, "28:flowing-water.wav"},
	 {NULL, "29:insects.wav"},
	 {NULL, "30:machine-gear.wav"},
	 {NULL, "31:mighty-machine.wav"},
	 {NULL, "32:static-buzz.wav"},
	 {NULL, "33:steam.wav"},
	 {NULL, "34:teleporter.wav"},
	 {NULL, "35:waterfall.wav"},
	 {NULL, "36:fire.wav"},
	 {NULL, "37:jet-engine.wav"},
	 {NULL, "38:nuke-explosion-short.wav"},
	 {NULL, "39:scrape.wav"},
	 {NULL, "40:scrape2.wav"},
	 {NULL, "41:scrape3.wav"},
	 {NULL, "42:scrape4.wav"}
	};

//------------------------------------------------------------------------------

const char *AddonSoundName (int32_t nSound)
{
return ((nSound < 0) || (nSound >= MAX_ADDON_SOUND_FILES)) ? "" : addonSounds [nSound].szSoundFile;
}

//------------------------------------------------------------------------------

int32_t FindAddonSound (const char* pszSoundFile)
{
for (int32_t i = 0; i < MAX_ADDON_SOUND_FILES; i++)
	if (strstr (addonSounds [i].szSoundFile + 3, pszSoundFile) ==  addonSounds [i].szSoundFile + 3)
		return i;
return -1;
}

//------------------------------------------------------------------------------

Mix_Chunk *LoadAddonSound (const char *pszSoundFile, uint8_t *bBuiltIn)
{
	Mix_Chunk*		pChunk;
	//char				szWAV [FILENAME_LEN];
	const char*		pszFolder, * pszFile;
	int32_t				i;
	CFile				cf;

if (!::isdigit (*pszSoundFile))
	i = FindAddonSound (pszSoundFile);
else {
	i = atoi (pszSoundFile);
	if (i >= MAX_ADDON_SOUND_FILES)
		return NULL;
	}
if (i >= 0) {
	if (bBuiltIn)
		*bBuiltIn = 1;
	if ((pChunk = addonSounds [i].pChunk))
		return pChunk;
	pszSoundFile = addonSounds [i].szSoundFile + 3;
	}
if (!gameStates.app.bReadOnly && cf.Extract (pszSoundFile, gameFolders.game.szData [0], 0, "d2x-temp.wav")) {
	pszFolder = gameFolders.var.szCache;
	pszFile = "d2x-temp.wav";
	}
else {
	if (cf.Exist (pszSoundFile, gameFolders.mods.szSounds [1], 0))
		pszFolder = gameFolders.mods.szSounds [1];
	else if (cf.Exist (pszSoundFile, gameFolders.mods.szSounds [0], 0))
		pszFolder = gameFolders.mods.szSounds [0];
	else if (cf.Exist (pszSoundFile, gameFolders.game.szSounds [4], 0))
		pszFolder = gameFolders.game.szSounds [4];
	else
		return NULL;
	pszFile = const_cast<char*>(pszSoundFile);
	}
//sprintf (szWAV, "%s%s", pszFolder, pszFile);
if (!(pChunk = Mix_LoadWAV_RW (CFileOpenRWOps (pszFile, pszFolder), 1)))
	return NULL;
if (i >= 0)
	addonSounds [i].pChunk = pChunk;
if (bBuiltIn)
	*bBuiltIn = (i >= 0);
return pChunk;
}

//------------------------------------------------------------------------------

void LoadAddonSounds (void)
{
for (int32_t i = 0; i < int32_t (sizeofa (addonSounds)); i++)
	LoadAddonSound (AddonSoundName (i));
}

//------------------------------------------------------------------------------

void FreeAddonSounds (void)
{
for (int32_t i = 0; i < MAX_ADDON_SOUND_FILES; i++) {
	if (addonSounds [i].pChunk) {
		Mix_FreeChunk (addonSounds [i].pChunk);
		addonSounds [i].pChunk = NULL;
		}
	}
}

#endif

//------------------------------------------------------------------------------
//eof
