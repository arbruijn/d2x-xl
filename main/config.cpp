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

#include "inferno.h"
#include "kconfig.h"
#include "u_mem.h"
#include "joy.h"
#include "args.h"
#include "findfile.h"
#include "crypt.h"
#include "strutil.h"
#include "error.h"

static const char *pszDigiVolume = "DigiVolume";
static const char *pszMidiVolume = "MidiVolume";
static const char *pszRedbookEnabled = "RedbookEnabled";
static const char *pszRedbookVolume = "RedbookVolume";
static const char *pszDetailLevel = "DetailLevel";
static const char *pszGammaLevel = "GammaLevel";
static const char *pszStereoRev = "StereoReverse";
static const char *pszJoystickMin = "JoystickMin";
static const char *pszJoystickMax = "JoystickMax";
static const char *pszJoystickCen = "JoystickCen";
static const char *pszLastPlayer = "LastPlayer";
static const char *pszLastMission = "LastMission";
static const char *pszVrType = "VRType";
static const char *pszVrResolution = "VR_resolution";
static const char *pszVrTracking = "VR_tracking";
static const char *pszHiresMovies = "Hires Movies";
static const char *pszCfgDataHash = "ConfigDataHash";

int digi_driver_board_16;
int digi_driver_dma_16;

void InitCustomDetails(void);

tGameConfig gameConfig;

typedef struct tHashList {
	int	nHashs;
	uint	*hashs;
} tHashList;

tHashList hashList = {0, NULL};

static uint nDefaultHash = 0xba61cc0b;

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

//------------------------------------------------------------------------------

static uint cfgHashs [] = {
	3935244969, 
	1087444571, 
	3915091710, 
	3393194754, 
	3871670399, 
	3035723711, 
	586367371, 
	3391413563, 
	2405653309, 
	3915091710,
	2943260375,
	891560507,
	2042098877,
	2405671997,
	2124113385,
	1461184298,
	17673152,
	3007579330,
	3684198094,
	1260878687,
	3612316049,
	3090073029,
	1347391164,
	1032917202,
	0};

int CfgLoadHashs (char *pszFilter, char *pszFolder)
{
nDefaultHash = Crc32 (0, (unsigned char *) "m4d1", 4);
if (hashList.nHashs)
	return hashList.nHashs;
if (!CfgCountHashs (pszFilter, pszFolder))
	return 0;
hashList.hashs = (uint *) D2_ALLOC (hashList.nHashs * sizeof (int));

	FFS	ffs;
	char	szTag [FILENAME_LEN];
	int	i = 0;

sprintf (szTag, "%s/%s", pszFolder, pszFilter);
for (i = 0; i ? !FFN (&ffs, 0) : !FFF (szTag, &ffs, 0); i++) {
	ffs.name [4] = '\0';
	strlwr (ffs.name);
	strcompress (ffs.name);
	hashList.hashs [i] = Crc16 (0, (const unsigned char *) &ffs.name [0], 4);
	}
return i;
}

//------------------------------------------------------------------------------

char *CfgFileTag (void)
{
	char szMask [6] = {4, 1, 19, 10, 31, 0};
	char szKey [6] = {'.', '/', 'c', 'f', 'g', '\0'};
	static char szTag [6] = {0, 0, 0, 0, 0, 0};

for (int i = 0; i < 6; i++)
	szTag [i] = szKey [i] ^ szMask [i];
return szTag;
}

//------------------------------------------------------------------------------

int GetCfgHash (void)
{
if (!CfgLoadHashs (CfgFileTag (), gameFolders.szProfDir))
	return -1;
for (int i = 0; i < hashList.nHashs; i++)
	for (int j = 0; cfgHashs [j]; j++)
		if (hashList.hashs [i] == cfgHashs [j])
			return hashList.hashs [i];
return nDefaultHash;
}

//------------------------------------------------------------------------------

bool CfgCreateHashs (void)
{
gameConfig.cfgDataHash = GetCfgHash ();
return WriteConfigFile () == 0;
}

//------------------------------------------------------------------------------

bool HaveCfgHashs (void)
{
return (gameConfig.cfgDataHash != (uint) -1) && (gameConfig.cfgDataHash != nDefaultHash);
}

//------------------------------------------------------------------------------

bool CfgInitHashs (void)
{
if (!(HaveCfgHashs () || CfgCreateHashs ()))
	return false;
return true;
}

//------------------------------------------------------------------------------

bool CheckGameConfig (void)
{
return (nDefaultHash != gameConfig.cfgDataHash);
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
}

// ----------------------------------------------------------------------------

#if 0

#define CL_MC0 0xF8F
#define CL_MC1 0xF8D

void CrystalLakeWriteMCP(ushort mc_addr, ubyte mc_data)
{
	_disable();
	outp (CL_MC0, 0xE2);				// Write password
	outp (mc_addr, mc_data);		// Write data
	_enable();
}

ubyte CrystalLakeReadMCP(ushort mc_addr)
{
	ubyte value;
	_disable();
	outp (CL_MC0, 0xE2);		// Write password
	value = inp (mc_addr);		// Read data
	_enable();
	return value;
}

void CrystalLakeSetSB()
{
	ubyte tmp;
	tmp = CrystalLakeReadMCP(CL_MC1);
	tmp = 0x7F;
	CrystalLakeWriteMCP(CL_MC1, tmp);
}

void CrystalLakeSetWSS()
{
	ubyte tmp;
	tmp = CrystalLakeReadMCP(CL_MC1);
	tmp |= 0x80;
	CrystalLakeWriteMCP(CL_MC1, tmp);
}

#endif

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
gameConfig.cfgDataHash = (uint) -1;

//set these here in case no cfg file
bHiresMoviesSave = gameOpts->movies.bHires;
bRedbookEnabledSave = gameStates.sound.bRedbookEnabled;

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
			gameStates.sound.bRedbookEnabled = bRedbookEnabledSave = strtol (value, NULL, 10);
		else if (!strcmp (token, pszRedbookVolume))
			gameConfig.nRedbookVolume = (ubyte) strtol (value, NULL, 10);
		else if (!strcmp (token, pszStereoRev))
			gameConfig.bReverseChannels = (ubyte) strtol (value, NULL, 10);
		else if (!strcmp (token, pszGammaLevel)) {
			gamma = (ubyte) strtol (value, NULL, 10);
			GrSetPaletteGamma (gamma);
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
		else if (!strcmp (token, pszCfgDataHash))
			gameConfig.cfgDataHash = strtol (value, NULL, 10);
		else if (!strcmp (token, pszHiresMovies))
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
DigiMidiVolume ((gameConfig.nDigiVolume * 32768) / 8, (gameConfig.nMidiVolume * 128) / 8);
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
CfgInitHashs ();
return 0;
}

// ----------------------------------------------------------------------------

int WriteConfigFile (void)
{
	CFile cf;
	char str [256];
	int i, j;
	tJoyAxisCal cal [JOY_MAX_AXES];
	ubyte gamma = GrGetPaletteGamma();

con_printf (CON_VERBOSE, "writing config file ...\n");
con_printf (CON_VERBOSE, "   getting joystick calibration values ...\n");
JoyGetCalVals(cal, sizeofa (cal));

if (!cf.Open ("descent.cfg", gameFolders.szConfigDir, "wt", 0))
	return 1;
sprintf (str, "%s=%ul\n", pszCfgDataHash, gameConfig.cfgDataHash);
cf.PutS(str);
sprintf (str, "%s=%d\n", pszDigiVolume, gameConfig.nDigiVolume);
cf.PutS(str);
sprintf (str, "%s=%d\n", pszMidiVolume, gameConfig.nMidiVolume);
cf.PutS(str);
sprintf (str, "%s=%d\n", pszRedbookEnabled, FindArg("-noredbook")?bRedbookEnabledSave:gameStates.sound.bRedbookEnabled);
cf.PutS(str);
sprintf (str, "%s=%d\n", pszRedbookVolume, gameConfig.nRedbookVolume);
cf.PutS(str);
sprintf (str, "%s=%d\n", pszStereoRev, gameConfig.bReverseChannels);
cf.PutS(str);
sprintf (str, "%s=%d\n", pszGammaLevel, gamma);
cf.PutS(str);
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
cf.PutS(str);

sprintf (str, "%s=%d,%d,%d,%d\n", pszJoystickMin, cal [0].nMin, cal [1].nMin, cal [2].nMin, cal [3].nMin);
cf.PutS(str);
sprintf (str, "%s=%d,%d,%d,%d\n", pszJoystickCen, cal [0].nCenter, cal [1].nCenter, cal [2].nCenter, cal [3].nCenter);
cf.PutS(str);
sprintf (str, "%s=%d,%d,%d,%d\n", pszJoystickMax, cal [0].nMax, cal [1].nMax, cal [2].nMax, cal [3].nMax);
cf.PutS(str);

sprintf (str, "%s=%s\n", pszLastPlayer, LOCALPLAYER.callsign);
cf.PutS(str);
for (i = 0; gameConfig.szLastMission [i]; i++)
	if (!isprint (gameConfig.szLastMission [i])) {
		*gameConfig.szLastMission = '\0';
		break;
		}
j = MsnHasGameVer (gameConfig.szLastMission) ? 4 : 0;
sprintf (str, "%s=%s\n", pszLastMission, gameConfig.szLastMission + j);
cf.PutS(str);
sprintf (str, "%s=%d\n", pszVrType, gameConfig.vrType);
cf.PutS(str);
sprintf (str, "%s=%d\n", pszVrResolution, gameConfig.vrResolution);
cf.PutS(str);
sprintf (str, "%s=%d\n", pszVrTracking, gameConfig.vrTracking);
cf.PutS(str);
sprintf (str, "%s=%d\n", pszHiresMovies, (FindArg("-nohires") || FindArg("-nohighres") || FindArg("-lowresmovies"))?bHiresMoviesSave:gameOpts->movies.bHires);
cf.PutS(str);
cf.Close ();
return 0;
}	

// ----------------------------------------------------------------------------
//eof

