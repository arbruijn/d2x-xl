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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "descent.h"
#include "kconfig.h"
#include "u_mem.h"
#include "joy.h"
#include "args.h"
#include "findfile.h"
#include "crypt.h"
#include "strutil.h"
#include "midi.h"
#include "songs.h"
#include "error.h"

static const char* pszDigiVolume = "DigiVolume";
static const char* pszMidiVolume = "MidiVolume";
static const char* pszRedbookEnabled = "RedbookEnabled";
static const char* pszRedbookVolume = "RedbookVolume";
static const char* pszDetailLevel = "DetailLevel";
static const char* pszGammaLevel = "GammaLevel";
static const char* pszStereoRev = "StereoReverse";
static const char* pszJoystickMin = "JoystickMin";
static const char* pszJoystickMax = "JoystickMax";
static const char* pszJoystickCen = "JoystickCen";
static const char* pszLastPlayer = "LastPlayer";
static const char* pszLastMission = "LastMission";
static const char* pszVrType = "VRType";
static const char* pszVrResolution = "VR_resolution";
static const char* pszVrTracking = "VR_tracking";
static const char* pszHiresMovies = "Hires Movies";
static const char* pszD2XVersion = "D2XVersion";
static const char* pszMPStatus = "MPStatus";

int digi_driver_board_16;
int digi_driver_dma_16;

void InitCustomDetails(void);

tGameConfig gameConfig;

int*	mpStatus = NULL;
bool (*mpActivate) (void) = NULL;
int (*mpCheck) (int) = NULL;
void (*mpNotify) (void) = NULL;

class CHashList {
	public:
		int				nHashs;
		CArray<uint>	hashs;
	public:
		CHashList () { nHashs = 0; }
};

CHashList hashList;

//------------------------------------------------------------------------------

void SetNostalgia (int nLevel)
{
gameStates.app.bNostalgia = (nLevel < 0) ? 0 : (nLevel > 3) ? 3 : nLevel;
gameStates.app.iNostalgia = (gameStates.app.bNostalgia > 0);
gameOpts = gameOptions + gameStates.app.iNostalgia;
}

//------------------------------------------------------------------------------

int CfgCountHashs (char *pszFilter, char *pszFolder)
{
	FFS	ffs;
	char	szTag [FILENAME_LEN];

hashList.nHashs = 0;
sprintf (szTag, "%s/%s", pszFolder, pszFilter);
if (!FFF (szTag, &ffs, 0)) {
	hashList.nHashs++;
	while (!FFN (&ffs, 0))
		hashList.nHashs++;
	}
FFC (&ffs);
return hashList.nHashs;
}

// ----------------------------------------------------------------------------

void InitGameConfig (void)
{
*gameConfig.szLastPlayer = '\0';
*gameConfig.szLastMission = '\0';
gameConfig.vrType = 0;
gameConfig.vrResolution = 0;
gameConfig.vrTracking = 0;
gameConfig.nDigiType = 0;
gameConfig.nDigiDMA = 0;
gameConfig.nMidiType = 0;
gameConfig.nDigiVolume = 8;
gameConfig.nMidiVolume = 8;
gameConfig.nRedbookVolume = 8;
gameConfig.nControlType = 0;
gameConfig.bReverseChannels = 0;
gameConfig.nVersion = 0;
}

// ----------------------------------------------------------------------------
//gameOpts->movies.bHires might be changed by -nohighres, so save a "real" copy of it
int bHiresMoviesSave;
int bRedbookEnabledSave;

#define MAX_JOY_AXIS

int ReadConfigFile (void)
{
	CFile			cf;
	char			line [80], *token, *value, *ptr;
	ubyte			gamma;
	tJoyAxisCal	cal [7];
	int			i;

strcpy (gameConfig.szLastPlayer, "");
memset (cal, 0, sizeof (cal));
JoySetCalVals (cal, sizeofa (cal));
gameConfig.nDigiVolume = 8;
gameConfig.nMidiVolume = 8;
gameConfig.nRedbookVolume = 8;
gameConfig.nControlType = 0;
gameConfig.bReverseChannels = 0;
if (mpActivate && mpStatus)
	*mpStatus = mpActivate () ? -1 : 0;

//set these here in case no cfg file
bHiresMoviesSave = gameOpts->movies.bHires;
bRedbookEnabledSave = redbook.Enabled ();

if (!cf.Open ("descent.cfg", gameFolders.szConfigDir, "rt", 0))
	return 1;
while (!cf.EoF ()) {
	memset (line, 0, 80);
	cf.GetS (line, 80);
	ptr = line;
	while (::isspace (*ptr))
		ptr++;
	if (*ptr != '\0') {
		token = strtok (ptr, "=");
		value = strtok (NULL, "=");
		if (!(value && token)) {
			PrintLog ("configuration file (descent.cfg) looks messed up.\n");
			continue;
			}
		if (value [strlen (value) - 1] == '\n')
			value [strlen (value) - 1] = 0;
		if (!strcmp (token, pszDigiVolume))
			gameConfig.nDigiVolume = (ubyte) strtol (value, NULL, 10);
		else if (!strcmp (token, pszMidiVolume))
			gameConfig.nMidiVolume = (ubyte) strtol (value, NULL, 10);
		else if (!strcmp (token, pszRedbookEnabled))
			redbook.Enable (bRedbookEnabledSave = strtol (value, NULL, 10));
		else if (!strcmp (token, pszRedbookVolume))
			gameConfig.nRedbookVolume = (ubyte) strtol (value, NULL, 10);
		else if (!strcmp (token, pszStereoRev))
			gameConfig.bReverseChannels = (ubyte) strtol (value, NULL, 10);
		else if (!strcmp (token, pszGammaLevel)) {
			gamma = (ubyte) strtol (value, NULL, 10);
			paletteManager.SetGamma (gamma);
			}
		else if (!strcmp (token, pszDetailLevel)) {
			gameStates.app.nDetailLevel = strtol (value, NULL, 10);
			if (gameStates.app.nDetailLevel == NUM_DETAIL_LEVELS-1) {
				int count,dummy,oc,od,wd,wrd,da,sc;
				count = sscanf (value, "%d,%d,%d,%d,%d,%d,%d\n", &dummy, &oc, &od, &wd, &wrd, &da, &sc);
				if (count == 7) {
					gameStates.render.detail.nObjectComplexity = oc;
					gameStates.render.detail.nObjectDetail = od;
					gameStates.render.detail.nWallDetail = wd;
					gameStates.render.detail.nWallRenderDepth = wrd;
					gameStates.render.detail.nDebrisAmount = da;
					gameStates.sound.nSoundChannels = sc;
					InitCustomDetails ();
					}
				}
			}
		else if (!strcmp (token, pszJoystickMin))
			sscanf (value, "%d,%d,%d,%d", &cal [0].nMin, &cal [1].nMin, &cal [2].nMin, &cal [3].nMin);
		else if (!strcmp (token, pszJoystickMax))
			sscanf (value, "%d,%d,%d,%d", &cal [0].nMax, &cal [1].nMax, &cal [2].nMax, &cal [3].nMax);
		else if (!strcmp (token, pszJoystickCen))
			sscanf (value, "%d,%d,%d,%d", &cal [0].nCenter, &cal [1].nCenter, &cal [2].nCenter, &cal [3].nCenter);
		else if (!strcmp (token, pszLastPlayer)) {
			char * p;
			strncpy (gameConfig.szLastPlayer, value, CALLSIGN_LEN);
			p = strchr (gameConfig.szLastPlayer, '\n');
			if (p) *p = 0;
		}
		else if (!strcmp (token, pszLastMission)) {
			int j = MsnHasGameVer (value) ? 4 : 0;
			strncpy (gameConfig.szLastMission, value + j, MISSION_NAME_LEN);
			char *p = strchr (gameConfig.szLastMission, '\n');
			if (p)
				*p = 0;
			}
		else if (!strcmp (token, pszVrType))
			gameConfig.vrType = strtol (value, NULL, 10);
		else if (!strcmp (token, pszVrResolution))
			gameConfig.vrResolution = strtol (value, NULL, 10);
		else if (!strcmp (token, pszVrTracking))
			gameConfig.vrTracking = strtol (value, NULL, 10);
		else if (!strcmp (token, pszD2XVersion))
			gameConfig.nVersion = strtoul (value, NULL, 10);
		else if (!strcmp (token, pszMPStatus)) {
			if (mpStatus)
				*mpStatus = (*mpStatus < 0) ? strtoul (value, NULL, 10) : 0;
			}
		else if (!strcmp (token, pszHiresMovies) && gameStates.app.bNostalgia)
			bHiresMoviesSave = gameOpts->movies.bHires = strtol (value, NULL, 10);
	}
}
cf.Close ();

i = FindArg ("-volume");
if (i > 0) {
	i = atoi (pszArgList [i + 1]);
	if (i < 0)
		i = 0;
	else if (i > 100)
		i = 100;
	gameConfig.nDigiVolume =
	gameConfig.nMidiVolume =
	gameConfig.nRedbookVolume = (i * 8) / 100;
	}

if (gameConfig.nDigiVolume > 8)
	gameConfig.nDigiVolume = 8;
if (gameConfig.nMidiVolume > 8)
	gameConfig.nMidiVolume = 8;
if (gameConfig.nRedbookVolume > 8)
	gameConfig.nRedbookVolume = 8;
audio.SetVolumes ((gameConfig.nDigiVolume * 32768) / 8, (gameConfig.nMidiVolume * 128) / 8);
if (cf.Open ("descentw.cfg", gameFolders.szConfigDir, "rt", 0)) {
	while (!cf.EoF ()) {
		memset (line, 0, 80);
		cf.GetS (line, 80);
		ptr = line;
		while (::isspace(*ptr))
			ptr++;
		if (*ptr != '\0') {
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
			if (!(value  && token)) {
				PrintLog ("configuration file (descentw.cfg) looks messed up.\n");
				continue;
				}
			if (value [strlen(value)-1] == '\n')
				value [strlen(value)-1] = 0;
			if (!strcmp (token, pszJoystickMin))
				sscanf (value, "%d,%d,%d,%d,%d,%d,%d",
						  &cal [0].nMin, &cal [1].nMin, &cal [2].nMin, &cal [3].nMin, &cal [4].nMin, &cal [5].nMin, &cal [6].nMin);
			else if (!strcmp (token, pszJoystickMax))
				sscanf (value, "%d,%d,%d,%d,%d,%d,%d",
						  &cal [0].nMax, &cal [1].nMax, &cal [2].nMax, &cal [3].nMax, &cal [4].nMax, &cal [5].nMax, &cal [6].nMax);
			else if (!strcmp (token, pszJoystickCen))
				sscanf (value, "%d,%d,%d,%d,%d,%d,%d",
						  &cal [0].nCenter, &cal [1].nCenter, &cal [2].nCenter, &cal [3].nCenter, &cal [4].nCenter, &cal [5].nCenter, &cal [6].nCenter);
			}
		}
	cf.Close ();
	}
JoySetCalVals (cal, sizeofa (cal));
#if 0 //DBG
if (mpStatus)
	*mpStatus = -1;
#endif
return 0;
}

// ----------------------------------------------------------------------------

int WriteConfigFile (void)
{
	CFile cf;
	char str [256];
	int i, j;
	tJoyAxisCal cal [JOY_MAX_AXES];
	ubyte gamma = paletteManager.GetGamma ();

console.printf (CON_VERBOSE, "writing config file ...\n");
console.printf (CON_VERBOSE, "   getting joystick calibration values ...\n");
JoyGetCalVals(cal, sizeofa (cal));

if (!cf.Open ("descent.cfg", gameFolders.szConfigDir, "wt", 0))
	return 1;
sprintf (str, "%s=%u\n", pszD2XVersion, D2X_IVER);
cf.PutS (str);
if (mpActivate && mpActivate ()) {
	sprintf (str, "%s=1\n", pszMPStatus);
	cf.PutS (str);
	}
sprintf (str, "%s=%d\n", pszDigiVolume, gameConfig.nDigiVolume);
cf.PutS (str);
sprintf (str, "%s=%d\n", pszMidiVolume, gameConfig.nMidiVolume);
cf.PutS (str);
sprintf (str, "%s=%d\n", pszRedbookEnabled, FindArg ("-noredbook")?bRedbookEnabledSave:redbook.Enabled ());
cf.PutS (str);
sprintf (str, "%s=%d\n", pszRedbookVolume, gameConfig.nRedbookVolume);
cf.PutS (str);
sprintf (str, "%s=%d\n", pszStereoRev, gameConfig.bReverseChannels);
cf.PutS (str);
sprintf (str, "%s=%d\n", pszGammaLevel, gamma);
cf.PutS (str);
if (gameStates.app.nDetailLevel == NUM_DETAIL_LEVELS-1)
	sprintf (str, "%s=%d,%d,%d,%d,%d,%d,%d\n",
				pszDetailLevel,
				gameStates.app.nDetailLevel,
				gameStates.render.detail.nObjectComplexity,
				gameStates.render.detail.nObjectDetail,
				gameStates.render.detail.nWallDetail,
				gameStates.render.detail.nWallRenderDepth,
				gameStates.render.detail.nDebrisAmount,
				gameStates.sound.nSoundChannels);
else
	sprintf (str, "%s=%d\n", pszDetailLevel, gameStates.app.nDetailLevel);
cf.PutS (str);

sprintf (str, "%s=%d,%d,%d,%d\n", pszJoystickMin, cal [0].nMin, cal [1].nMin, cal [2].nMin, cal [3].nMin);
cf.PutS (str);
sprintf (str, "%s=%d,%d,%d,%d\n", pszJoystickCen, cal [0].nCenter, cal [1].nCenter, cal [2].nCenter, cal [3].nCenter);
cf.PutS (str);
sprintf (str, "%s=%d,%d,%d,%d\n", pszJoystickMax, cal [0].nMax, cal [1].nMax, cal [2].nMax, cal [3].nMax);
cf.PutS (str);

sprintf (str, "%s=%s\n", pszLastPlayer, LOCALPLAYER.callsign);
cf.PutS (str);
for (i = 0; gameConfig.szLastMission [i]; i++)
	if (!isprint (gameConfig.szLastMission [i])) {
		*gameConfig.szLastMission = '\0';
		break;
		}
j = MsnHasGameVer (gameConfig.szLastMission) ? 4 : 0;
sprintf (str, "%s=%s\n", pszLastMission, gameConfig.szLastMission + j);
cf.PutS (str);
sprintf (str, "%s=%d\n", pszVrType, gameConfig.vrType);
cf.PutS (str);
sprintf (str, "%s=%d\n", pszVrResolution, gameConfig.vrResolution);
cf.PutS (str);
sprintf (str, "%s=%d\n", pszVrTracking, gameConfig.vrTracking);
cf.PutS (str);
sprintf (str, "%s=%d\n", pszHiresMovies, (FindArg ("-nohires") || FindArg ("-nohighres") || FindArg ("-lowresmovies"))
			? bHiresMoviesSave
			: gameOpts->movies.bHires);
cf.PutS (str);
cf.Close ();
return 0;
}

// ----------------------------------------------------------------------------
//eof

