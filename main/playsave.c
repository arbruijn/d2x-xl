/* $Id: playsave.c,v 1.16 2003/11/26 12:26:33 btb Exp $ */
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
#include <string.h>
#include <ctype.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#ifndef _WIN32_WCE
#include <errno.h>
#endif

#include "error.h"

#include "pa_enabl.h"
#include "inferno.h"
#include "strutil.h"
#include "game.h"
#include "gameseq.h"
#include "player.h"
#include "playsave.h"
#include "joy.h"
#include "kconfig.h"
#include "digi.h"
#include "newmenu.h"
#include "joydefs.h"
#include "palette.h"
#include "multi.h"
#include "menu.h"
#include "config.h"
#include "text.h"
#include "mono.h"
#include "state.h"
#include "gauges.h"
#include "screens.h"
#include "powerup.h"
#include "makesig.h"
#include "byteswap.h"
#include "escort.h"
#include "network.h"
#include "weapon.h"
#include "autodl.h"
#include "args.h"

//------------------------------------------------------------------------------

#define SAVE_FILE_ID			MAKE_SIG('D','P','L','R')

#if defined(_WIN32_WCE)
# define errno -1
# define ENOENT -1
# define strerror(x) "Unknown Error"
#endif

int get_lifetime_checksum (int a,int b);

typedef struct hli {
	char	shortname[9];
	ubyte	level_num;
} hli;

short nHighestLevels;

hli highestLevels [MAX_MISSIONS];

#define COMPATIBLE_PLAYER_FILE_VERSION    17
#define D2W95_PLAYER_FILE_VERSION			24
#define D2XW32_PLAYER_FILE_VERSION			45		// first flawless D2XW32 tPlayer file version
#define PLAYER_FILE_VERSION					145	//increment this every time the tPlayer file changes

//version 5  ->  6: added new highest level information
//version 6  ->  7: stripped out the old saved_game array.
//version 7  ->  8: added reticle flag, & window size
//version 8  ->  9: removed player_struct_version
//version 9  -> 10: added default display mode
//version 10 -> 11: added all toggles in toggle menu
//version 11 -> 12: added weapon ordering
//version 12 -> 13: added more keys
//version 13 -> 14: took out marker key
//version 14 -> 15: added guided in big window
//version 15 -> 16: added small windows in cockpit
//version 16 -> 17: ??
//version 17 -> 18: save guidebot name
//version 18 -> 19: added automap-highres flag
//version 19 -> 20: added KConfig data for windows joysticks
//version 20 -> 21: save seperate config types for DOS & Windows
//version 21 -> 22: save lifetime netstats 
//version 22 -> 23: ??
//version 23 -> 24: add name of joystick for windows version.
//version 24 -> 25: add d2x keys array


extern void InitWeaponOrdering();

//------------------------------------------------------------------------------

int new_player_config()
{
	int nitems;
	int i,j,choice;
	tMenuItem m[8];
   int mct=CONTROL_MAX_TYPES;
 
 	 mct--;
   InitWeaponOrdering ();		//setup default weapon priorities 

RetrySelection:
		memset (m, 0, sizeof (m));
		for (i=0; i<mct; i++ )	{
			m[i].nType = NM_TYPE_MENU; m[i].text = CONTROL_TEXT(i); m[i].key = -1;
		}
		nitems = i;
		m[0].text = TXT_CONTROL_KEYBOARD;
	
			choice = gameConfig.nControlType;				// Assume keyboard
		#ifndef APPLE_DEMO
			i = ExecMenu1( NULL, TXT_CHOOSE_INPUT, i, m, NULL, &choice );
		#else
			choice = 0;
		#endif
		
		if ( i < 0 )
			return 0;

	for (i = 0; i < CONTROL_MAX_TYPES; i++)
		for (j = 0; j < MAX_CONTROLS; j++)
			controlSettings.custom [i][j] = controlSettings.defaults [i][j];
	//added on 2/4/99 by Victor Rachels for new keys
	for(i = 0; i < MAX_D2X_CONTROLS; i++)
		controlSettings.d2xCustom[i] = controlSettings.d2xDefaults[i];
	//end this section addition - VR
	KCSetControls();

	gameConfig.nControlType = choice;

	if ( gameConfig.nControlType==CONTROL_THRUSTMASTER_FCS)	{
		i = ExecMessageBox( TXT_IMPORTANT_NOTE, NULL, 2, "Choose another", TXT_OK, TXT_FCS );
		if (i==0) 
			goto RetrySelection;
	}
	
	if ( (gameConfig.nControlType>0) && 	(gameConfig.nControlType<5))	{
		joydefs_calibrate();
	}
	
playerDefaultDifficulty = 1;
gameOptions [0].gameplay.bAutoLeveling = gameOpts->gameplay.bDefaultLeveling = 1;
nHighestLevels = 1;
highestLevels[0].shortname[0] = 0;			//no name for mission 0
highestLevels[0].level_num = 1;				//was highest level in old struct
gameOpts->input.joySensitivity [0] =
gameOpts->input.joySensitivity [1] =
gameOpts->input.joySensitivity [2] =
gameOpts->input.joySensitivity [3] = 8;
gameOpts->input.mouseSensitivity [0] =
gameOpts->input.mouseSensitivity [1] =
gameOpts->input.mouseSensitivity [2] = 8;
Cockpit_3dView[0]=CV_NONE;
Cockpit_3dView[1]=CV_NONE;

// Default taunt macros
strcpy(multiData.msg.szMacro[0], TXT_GET_ALONG);
strcpy(multiData.msg.szMacro[1], TXT_GOT_PRESENT);
strcpy(multiData.msg.szMacro[2], TXT_HANKERING);
strcpy(multiData.msg.szMacro[3], TXT_URANUS);
networkData.nNetLifeKills=0; 
networkData.nNetLifeKilled=0;	
#if 0
InitGameOptions (0);
InitArgs (0, NULL);
InitGameOptions (1);
#endif
return 1;
}

//------------------------------------------------------------------------------

//this length must match the value in escort.c
#define GUIDEBOT_NAME_LEN 9
WIN(extern char win95_current_joyname[]);

ubyte controlType_dos,controlType_win;

//read in the tPlayer's saved games.  returns errno (0 == no error)
int ReadPlayerFile(int bOnlyWindowSizes)
{
	char	filename[FILENAME_LEN];
	CFILE *fp;
	int	errno_ret = EZERO;
	int	id, h, i, j;
	short player_file_version;
	int	rewrite_it=0;
	int	swap = 0;
	int	gameOptsSize, nMaxControls;

Assert(gameData.multi.nLocalPlayer>=0 && gameData.multi.nLocalPlayer<MAX_PLAYERS);

sprintf(filename, "%.8s.plr", gameData.multi.players[gameData.multi.nLocalPlayer].callsign);
if (!(fp = CFOpen(filename, gameFolders.szProfDir, "rb", 0))) {
	LogErr ("   couldn't read tPlayer file '%s'\n", filename);
	return errno;
	}
id = CFReadInt(fp);
// SWAPINT added here because old versions of d2x
// used the wrong byte order.
if (nCFileError || (id!=SAVE_FILE_ID && id!=SWAPINT(SAVE_FILE_ID))) {
	ExecMessageBox(TXT_ERROR, NULL, 1, TXT_OK, "Invalid tPlayer file");
	CFClose(fp);
	return -1;
	}

player_file_version = CFReadShort(fp);
if (player_file_version > 255) // bigendian fp?
	swap = 1;
nMaxControls = (player_file_version < 108) ? 60 : MAX_CONTROLS;
if (swap)
	player_file_version = SWAPSHORT(player_file_version);

if ((player_file_version < COMPATIBLE_PLAYER_FILE_VERSION) ||
	 ((player_file_version > D2W95_PLAYER_FILE_VERSION) && 
	  (player_file_version < D2XW32_PLAYER_FILE_VERSION))) {
	ExecMessageBox(TXT_ERROR, NULL, 1, TXT_OK, TXT_ERROR_PLR_VERSION);
	CFClose(fp);
	return -1;
	}

{
	short w, h;
	w = CFReadShort(fp);
	h = CFReadShort(fp);
	if (!gameStates.gfx.bOverride) {
		Game_window_w = w;
		Game_window_h = h;
		if (swap) {
			Game_window_w = SWAPSHORT (Game_window_w);
			Game_window_h = SWAPSHORT (Game_window_h);
			}
		}
}
if (bOnlyWindowSizes) {
	CFClose (fp);
	return 0;
	}

playerDefaultDifficulty = CFReadByte (fp);
gameOpts->gameplay.bDefaultLeveling = CFReadByte (fp);
gameOpts->render.cockpit.bReticle = CFReadByte (fp);
gameStates.render.cockpit.nMode = CFReadByte (fp);
{
	unsigned char m = CFReadByte (fp);
	if (!gameStates.gfx.bOverride)
		gameStates.video.nDefaultDisplayMode = m;
}
gameOpts->render.cockpit.bMissileView = CFReadByte (fp);
gameOpts->gameplay.bHeadlightOn = CFReadByte (fp);
gameOptions [0].render.cockpit.bGuidedInMainView = CFReadByte (fp);

if (player_file_version >= 19)
	gameOptions [0].render.bAutomapAlwaysHires = CFReadByte (fp);
gameOptions [0].gameplay.bAutoLeveling = gameOpts->gameplay.bDefaultLeveling;

//read new highest level info

nHighestLevels = CFReadShort(fp);
if (swap)
	nHighestLevels = SWAPSHORT(nHighestLevels);
Assert(nHighestLevels <= MAX_MISSIONS);

if (CFRead(highestLevels, sizeof(hli), nHighestLevels, fp) != (size_t) nHighestLevels) {
	errno_ret = errno;
	CFClose(fp);
	return errno_ret;
	}

//read taunt macros
{
	int len = MAX_MESSAGE_LEN;

	for (i = 0; i < 4; i++)
		if (CFRead(multiData.msg.szMacro[i], len, 1, fp) != 1)
			{errno_ret			= errno; break;}
}

//read KConfig data
{
	int nControlTypes = (player_file_version<20)?7:CONTROL_MAX_TYPES;

	if (CFRead (controlSettings.custom, nMaxControls * nControlTypes, 1, fp ) != 1)
		errno_ret = errno;
	else if (CFRead((ubyte *)&controlType_dos, sizeof (ubyte), 1, fp ) != 1)
		errno_ret = errno;
	else if (player_file_version >= 21 && CFRead ((ubyte *) &controlType_win, sizeof (ubyte), 1, fp ) != 1)
		errno_ret = errno;
	else if (CFRead (gameOptions [0].input.joySensitivity, sizeof (ubyte), 1, fp ) != 1)
		errno_ret = errno;
	gameConfig.nControlType = controlType_dos;
	for (i=0;i<11;i++) {
		primaryOrder[i]   = CFReadByte (fp);
		secondaryOrder[i] = CFReadByte (fp);
		}
	ValidatePrios (primaryOrder, defaultPrimaryOrder, MAX_PRIMARY_WEAPONS);
	ValidatePrios (secondaryOrder, defaultSecondaryOrder, MAX_SECONDARY_WEAPONS);
	if (player_file_version>=16) {
		Cockpit_3dView[0] = CFReadInt(fp);
		Cockpit_3dView[1] = CFReadInt(fp);
		if (swap) {
			Cockpit_3dView[0] = SWAPINT(Cockpit_3dView[0]);
			Cockpit_3dView[1] = SWAPINT(Cockpit_3dView[1]);
			}
		}
}

if (player_file_version>=22) {
	networkData.nNetLifeKills = CFReadInt(fp);
	networkData.nNetLifeKilled = CFReadInt(fp);
	if (swap) {
		networkData.nNetLifeKills = SWAPINT(networkData.nNetLifeKills);
		networkData.nNetLifeKilled = SWAPINT(networkData.nNetLifeKilled);
		}
	}
else
	{
	networkData.nNetLifeKills=0; 
	networkData.nNetLifeKilled=0;
	}

if (player_file_version>=23) {
  i = CFReadInt(fp);
  if (swap)
	  i = SWAPINT(i);
#if TRACE				
	con_printf (CON_DEBUG,"Reading: lifetime checksum is %d\n",i);
#endif
	if (i!=get_lifetime_checksum (networkData.nNetLifeKills,networkData.nNetLifeKilled)) {
		networkData.nNetLifeKills=0; networkData.nNetLifeKilled=0;
 		ExecMessageBox(NULL, NULL, 1, "Shame on me", "Trying to cheat eh?");
		rewrite_it=1;
		}
	}

//read guidebot name
if (player_file_version >= 18)
	CFReadString(gameData.escort.szName, GUIDEBOT_NAME_LEN, fp);
else
	strcpy(gameData.escort.szName, "GUIDE-BOT");
gameData.escort.szName [sizeof (gameData.escort.szName) - 1] = '\0';
for (i = 0; i < sizeof (gameData.escort.szName); i++) {
	if (!gameData.escort.szName [i])
		break;
	if (!isprint (gameData.escort.szName [i])) {
		strcpy(gameData.escort.szName, "GUIDE-BOT");
		break;
		}
	}
strcpy(gameData.escort.szRealName, gameData.escort.szName);

{
	char buf[128];

	if (player_file_version >= D2W95_PLAYER_FILE_VERSION) 
		CFReadString(buf, 127, fp);      // Just read it in fpr DPS.
}

if (player_file_version >= 25)
	CFRead(controlSettings.d2xCustom, MAX_D2X_CONTROLS, 1, fp);
else
	for(i=0; i < MAX_D2X_CONTROLS; i++)
		controlSettings.d2xCustom[i] = controlSettings.d2xDefaults[i];

// read D2X-XL stuff
if (player_file_version >= 97)
	gameOptsSize = CFReadInt (fp);
for (j = 0; j < 2; j++) {
	if (j && (gameOptsSize > sizeof (tGameOptions)))
		CFSeek (fp, gameOptsSize - sizeof (tGameOptions), SEEK_CUR);
	if (!j) {
		if (player_file_version >= 26)
			extraGameInfo [0].bFixedRespawns = (int) CFReadByte (fp);
		if (player_file_version >= 27)
			/*extraGameInfo [0].bFriendlyFire = (int)*/ CFReadByte (fp);
		}
	if (player_file_version >= 28) {
		gameOptions [j].render.nMaxFPS = (int) CFReadByte (fp);
		if (gameOptions [j].render.nMaxFPS < 0) {
			gameOptions [j].render.nMaxFPS += 256;
			if (gameOptions [j].render.nMaxFPS < 0)
				gameOptions [j].render.nMaxFPS = 250;
			}
		if (!j)
			extraGameInfo [0].nSpawnDelay = (int) CFReadByte (fp) * 1000;
		}
	if (player_file_version >= 29)
		gameOptions [j].input.joyDeadZones [0] = (int) CFReadByte (fp);
	if (player_file_version >= 30)
		gameOptions [j].render.cockpit.nWindowSize = (int) CFReadByte (fp);
	if (player_file_version >= 31)
		gameOptions [j].render.cockpit.nWindowPos = (int) CFReadByte (fp);
	if (player_file_version >= 32)
		gameOptions [j].render.cockpit.nWindowZoom = (int) CFReadByte (fp);
	if (!j && (player_file_version >= 33))
		extraGameInfo [0].bPowerupsOnRadar = (int) CFReadByte (fp);
	if (player_file_version >= 34) {
		gameOptions [j].input.keyRampScale = (int) CFReadByte (fp);
		if (gameOptions [j].input.keyRampScale < 10)
			gameOptions [j].input.keyRampScale = 10;
		else if (gameOptions [j].input.keyRampScale > 100)
			gameOptions [j].input.keyRampScale = 100;
		}
	if (player_file_version >= 35)
		gameOptions [j].render.color.bAmbientLight = (int) CFReadByte (fp);
	if (player_file_version >= 36)
		gameOptions [j].ogl.bSetGammaRamp = (int) CFReadByte (fp);
	if (!j && (player_file_version >= 37))
		extraGameInfo [0].nZoomMode = (int) CFReadByte (fp);
	if (player_file_version >= 38)
		for (i = 0; i < 3; i++)
			gameOptions [j].input.bRampKeys [i] = (int) CFReadByte (fp);
	if (!j && (player_file_version >= 39))
#if 0
		extraGameInfo [0].bEnhancedCTF = (int) CFReadByte (fp);
#else
		CFReadByte (fp);
#endif
	if (!j && (player_file_version >= 40))
		extraGameInfo [0].bRobotsHitRobots = (int) CFReadByte (fp);
	if (player_file_version >= 41)
		gameOptions [j].gameplay.nAutoSelectWeapon = (int) CFReadByte (fp);
	if (!j && (player_file_version >= 42))
		extraGameInfo [0].bAutoDownload = (int) CFReadByte (fp);
	if (player_file_version >= 43)
		gameOptions [j].render.color.bGunLight = (int) CFReadByte (fp);
	if (player_file_version >= 44)
		gameStates.multi.bUseTracker = (int) CFReadByte (fp);
	if (player_file_version >= 45)
		gameOptions [j].gameplay.bFastRespawn = (int) CFReadByte (fp);
	if (!j && (player_file_version >= 46))
		extraGameInfo [0].bDualMissileLaunch = (int) CFReadByte (fp);
	if (player_file_version >= 47) {
		gameOptions [j].gameplay.nPlayerDifficultyLevel = (int) CFReadByte (fp);
		gameOptions [j].gameplay.nPlayerDifficultyLevel = NMCLAMP (gameOptions [j].gameplay.nPlayerDifficultyLevel, 0, 4);
		}
	if (player_file_version >= 48)
		gameOptions [j].render.bTransparentEffects = (int) CFReadByte (fp);
	if (!j && (player_file_version >= 49))
		extraGameInfo [0].bRobotsOnRadar = (int) CFReadByte (fp);
	if (player_file_version >= 50)
		gameOptions [j].render.bAllSegs = (int) CFReadByte (fp);
	if (!j && (player_file_version >= 51))
		extraGameInfo [0].grWallTransparency = (int) CFReadByte (fp);
	if (player_file_version >= 52)
		gameOptions [j].input.mouseSensitivity [0] = (int) CFReadByte (fp);
	if (player_file_version >= 53) {
		gameOptions [j].multi.bUseMacros = (int) CFReadByte (fp);
		if (!j)
			extraGameInfo [0].bWiggle = (int) CFReadByte (fp);
		}
	if (player_file_version >= 54)
		gameOptions [j].movies.nQuality = (int) CFReadByte (fp);
	if (player_file_version >= 55)
		gameOptions [j].render.color.bWalls = (int) CFReadByte (fp);
	if (player_file_version >= 56)
		gameOptions [j].input.bLinearJoySens = (int) CFReadByte (fp);
	if (!j) {
		if (player_file_version >= 57)
			extraGameInfo [0].nSpeedBoost = (int) CFReadByte (fp);
		if (player_file_version >= 58) {
			extraGameInfo [0].bDropAllMissiles = (int) CFReadByte (fp);
			extraGameInfo [0].bImmortalPowerups = (int) CFReadByte (fp);
			}
		if (player_file_version >= 59)
			extraGameInfo [0].bUseCameras = (int) CFReadByte (fp);
		}
	if (player_file_version >= 60) {
		gameOptions [j].render.cameras.bFitToWall = (int) CFReadByte (fp);
		gameOptions [j].render.cameras.nFPS = (int) CFReadByte (fp);
		if (!j)
			extraGameInfo [0].nFusionPowerMod = (int) CFReadByte (fp);
		}
	if (player_file_version >= 62)
		gameOptions [j].render.color.bUseLightMaps = (int) CFReadByte (fp);
	if (player_file_version >= 63)
		gameOptions [j].render.cockpit.bHUD = (int) CFReadByte (fp);
	if (player_file_version >= 64)
		gameOptions [j].render.color.nLightMapRange = (int) CFReadByte (fp);
	if (!j) {
		if (player_file_version >= 65)
			extraGameInfo [0].bMouseLook = (int) CFReadByte (fp);
		if	(player_file_version >= 66)
			extraGameInfo [0].bMultiBosses = (int) CFReadByte (fp);
		}	
	if (player_file_version >= 67)
		gameOptions [j].app.nVersionFilter = (int) CFReadByte (fp);
	if (!j) {
		if (player_file_version >= 68)
			extraGameInfo [0].bSmartWeaponSwitch = (int) CFReadByte (fp);
		if (player_file_version >= 69)
			extraGameInfo [0].bFluidPhysics = (int) CFReadByte (fp);
		}
	if (player_file_version >= 70) {
		gameOptions [j].render.nQuality = (int) CFReadByte (fp);
		SetRenderQuality ();
		}
	if (player_file_version >= 71)
		gameOptions [j].movies.bSubTitles = (int) CFReadByte (fp);
	if (player_file_version >= 72)
		gameOptions [j].render.textures.nQuality = (int) CFReadByte (fp);
	if (player_file_version >= 73)
		gameOptions [j].render.cameras.nSpeed = CFReadInt(fp);
	if (!j && (player_file_version >= 74))
		extraGameInfo [0].nWeaponDropMode = CFReadByte (fp);
	if (player_file_version >= 75)
		gameOptions [j].menus.bSmartFileSearch = CFReadInt (fp);
	if (player_file_version >= 76) {
		int h = CFReadInt (fp);
		SetDlTimeout (h);
		}
	if (!j && (player_file_version >= 79)) {
		extraGameInfo [0].entropy.nCaptureVirusLimit = CFReadByte (fp);
		extraGameInfo [0].entropy.nCaptureTimeLimit = CFReadByte (fp);
		extraGameInfo [0].entropy.nMaxVirusCapacity = CFReadByte (fp);
		extraGameInfo [0].entropy.nBumpVirusCapacity = CFReadByte (fp);
		extraGameInfo [0].entropy.nBashVirusCapacity = CFReadByte (fp);
		extraGameInfo [0].entropy.nVirusGenTime = CFReadByte (fp);
		extraGameInfo [0].entropy.nVirusLifespan = CFReadByte (fp);
		extraGameInfo [0].entropy.nVirusStability = CFReadByte (fp); 
		extraGameInfo [0].entropy.nEnergyFillRate = CFReadShort(fp);
		extraGameInfo [0].entropy.nShieldFillRate = CFReadShort(fp);
		extraGameInfo [0].entropy.nShieldDamageRate = CFReadShort(fp);
		extraGameInfo [0].entropy.bRevertRooms = CFReadByte (fp);
		extraGameInfo [0].entropy.bDoConquerWarning = CFReadByte (fp);
		extraGameInfo [0].entropy.nOverrideTextures = CFReadByte (fp);
		extraGameInfo [0].entropy.bBrightenRooms = CFReadByte (fp);
		extraGameInfo [0].entropy.bPlayerHandicap = CFReadByte (fp);

		mpParams.nLevel = CFReadByte (fp);
		mpParams.nGameType = CFReadByte (fp);
		mpParams.nGameMode = CFReadByte (fp);
		mpParams.nGameAccess = CFReadByte (fp);
		mpParams.bShowPlayersOnAutomap = CFReadByte (fp);
		mpParams.nDifficulty = CFReadByte (fp);
		mpParams.nWeaponFilter = CFReadInt(fp);
		mpParams.nReactorLife = CFReadInt(fp);
		mpParams.nMaxTime = CFReadByte (fp);
		mpParams.nKillGoal = CFReadByte (fp);
		mpParams.bInvul = CFReadByte (fp);
		mpParams.bMarkerView = CFReadByte (fp);
		mpParams.bAlwaysBright = CFReadByte (fp);
		mpParams.bBrightPlayers = CFReadByte (fp);
		mpParams.bShowAllNames = CFReadByte (fp);
		mpParams.bShortPackets = CFReadByte (fp);
		mpParams.nPPS = CFReadByte (fp);
		mpParams.udpClientPort = CFReadInt(fp);
		CFRead(mpParams.szServerIpAddr, 16, 1, fp);
		}
	if (player_file_version >= 80)
		gameOptions [j].render.bOptimize = (int) CFReadByte (fp);
	if (!j && (player_file_version >= 81)) {
		extraGameInfo [1].bRotateLevels = (int) CFReadByte (fp);
		extraGameInfo [1].bDisableReactor = (int) CFReadByte (fp);
		}
	if (player_file_version >= 82) {
		for (i = 0; i < 4; i++)
			gameOptions [j].input.joyDeadZones [i] = (int) CFReadByte (fp);
		for (i = 0; i < 4; i++)
			gameOptions [j].input.joySensitivity [i] = CFReadByte (fp);
		gameOptions [j].input.bSyncJoyAxes = (int) CFReadByte (fp);
		}
	if (!j && (player_file_version >= 83)) 
		extraGameInfo [1].bDualMissileLaunch = (int) CFReadByte (fp);
	if (player_file_version >= 84) {
		if (!j)
			extraGameInfo [1].bMouseLook = (int) CFReadByte (fp);
		for (i = 0; i < 3; i++)
			gameOptions [j].input.mouseSensitivity [i] = (int) CFReadByte (fp);
		gameOptions [j].input.bSyncMouseAxes = (int) CFReadByte (fp);
		gameOptions [j].render.color.bMix = (int) CFReadByte (fp);
		gameOptions [j].render.color.bCap = (int) CFReadByte (fp);
		}
	if (!j && (player_file_version >= 85))
		extraGameInfo [0].nWeaponIcons = (ubyte) CFReadByte (fp);
	if (player_file_version >= 86)
		gameOptions [j].render.weaponIcons.bSmall = (ubyte) CFReadByte (fp);
	if (!j && (player_file_version >= 87))
		extraGameInfo [0].bAutoBalanceTeams = CFReadByte (fp);
	if (player_file_version >= 88) {
		gameOptions [j].movies.bResize = (int) CFReadByte (fp);
		gameOptions [j].menus.bShowLevelVersion = (int) CFReadByte (fp);
		}
	if (player_file_version >= 89)
		gameOptions [j].render.weaponIcons.nSort = (ubyte) CFReadByte (fp);
	if (player_file_version >= 90)
		gameOptions [j].render.weaponIcons.bShowAmmo = (ubyte) CFReadByte (fp);
	if (player_file_version >= 91) {
		gameOptions [j].input.bUseMouse = (ubyte) CFReadByte (fp);
		gameOptions [j].input.bUseJoystick = (ubyte) CFReadByte (fp);
		gameOptions [j].input.bUseHotKeys = (ubyte) CFReadByte (fp);
		}
	if (!j) {
		if (player_file_version >= 92)
			extraGameInfo [0].bSafeUDP = (char) CFReadByte (fp);
		if (player_file_version >= 93)
			CFRead(mpParams.szServerIpAddr + 16, 6, 1, fp);
		}
	if (player_file_version >= 95) {
		gameOptions [j].render.cockpit.bTextGauges = (int) CFReadByte (fp);
		gameOptions [j].render.cockpit.bScaleGauges = (int) CFReadByte (fp);
		}
	if (player_file_version >= 96) {
		gameOptions [j].render.weaponIcons.bEquipment = (int) CFReadByte (fp);
		gameOptions [j].render.weaponIcons.alpha = (ubyte) CFReadByte (fp);
		}
	if (player_file_version >= 97)
		gameOptions [j].render.cockpit.bFlashGauges = (int) CFReadByte (fp);
	if (!j && (player_file_version >= 98))
		for (i = 0; i < 2; i++) 
			if (!(extraGameInfo [i].bFastPitch = (int) CFReadByte (fp)))
				extraGameInfo [i].bFastPitch = 2;
	if (!j && (player_file_version >= 99))
		gameStates.multi.nConnection = CFReadInt (fp);
	if (player_file_version >= 100) {
		if (!j)
			extraGameInfo [0].bUseSmoke = (int) CFReadByte (fp);
		gameOptions [j].render.smoke.nDens [0] = CFReadInt (fp);
		gameOptions [j].render.smoke.nSize [0] = CFReadInt (fp);
		NMCLAMP (gameOptions [j].render.smoke.nDens [0], 0, 4);
		NMCLAMP (gameOptions [j].render.smoke.nSize [0], 0, 3);
		gameOptions [j].render.smoke.bPlayers = (int) CFReadByte (fp);
		gameOptions [j].render.smoke.bRobots = (int) CFReadByte (fp);
		gameOptions [j].render.smoke.bMissiles = (int) CFReadByte (fp);
		}
	if (player_file_version >= 101) {
		if (!j) {
			extraGameInfo [0].bDamageExplosions = (int) CFReadByte (fp);
			extraGameInfo [0].bThrusterFlames = (int) CFReadByte (fp);
			}
		}
	if (player_file_version >= 102)
		gameOptions [j].render.smoke.bCollisions = (int) CFReadByte (fp);
	if (player_file_version >= 103)
		gameOptions [j].gameplay.bShieldWarning = (int) CFReadByte (fp);
	if (player_file_version >= 104)
		gameOptions [j].app.bExpertMode = (int) CFReadByte (fp);
	if (player_file_version >= 105) {
		if (!j)
#if SHADOWS
			extraGameInfo [0].bShadows = CFReadByte (fp);
		gameOptions [j].render.shadows.nLights = CFReadInt (fp);
		NMCLAMP (gameOptions [j].render.shadows.nLights, 0, 8);
#else
			CFReadByte (fp);
		CFReadInt (fp);
#endif
		}
	if (player_file_version >= 106)
		if (!j)
			gameStates.ogl.nContrast = CFReadInt (fp);
	if (player_file_version >= 107)
		if (!j)
			extraGameInfo [0].bRenderShield = CFReadByte (fp);
	if (player_file_version >= 108)
		gameOptions [j].gameplay.bInventory = (int) CFReadByte (fp);
	if (player_file_version >= 109)
		gameOptions [j].input.bJoyMouse = CFReadInt (fp);
	if (player_file_version >= 110)
		if (!j) {
			int	w, h;
			w = CFReadInt (fp);
			h = CFReadInt (fp);
			if (!gameStates.gfx.bOverride && (gameStates.video.nDefaultDisplayMode == NUM_DISPLAY_MODES))
				SetCustomDisplayMode (w, h);
			}
	if (player_file_version >= 111)
		gameOptions [j].render.cockpit.bMouseIndicator = CFReadInt (fp);
	if (player_file_version >= 112)
		extraGameInfo [j].bTeleporterCams = CFReadInt (fp);
	if (player_file_version >= 113)
		gameOptions [j].render.cockpit.bSplitHUDMsgs = CFReadInt (fp);
	if (player_file_version >= 114) {
		gameOptions [j].input.joyDeadZones [4] = (int) CFReadByte (fp);
		gameOptions [j].input.joySensitivity [4] = CFReadByte (fp);
		}
	if (player_file_version >= 115)
		if (!j) {
			tMonsterballForce *pf = extraGameInfo [0].monsterball.forces;
			extraGameInfo [0].monsterball.nBonus = CFReadByte (fp);
			for (h = 0; h < MAX_MONSTERBALL_FORCES; h++, pf++) {
				pf->nWeaponId = CFReadByte (fp);
				pf->nForce = CFReadByte (fp);
				}
			}
	if (player_file_version >= 116)
		if (!j)
			extraGameInfo [0].monsterball.nSizeMod = CFReadByte (fp);
	if (player_file_version >= 117) {
		gameOptions [j].render.bDynLighting = CFReadInt (fp);
		gameOptions [j].ogl.bLightObjects = CFReadInt (fp);
		gameOptions [j].ogl.nMaxLights = CFReadInt (fp);
		}
	if (player_file_version >= 118)
		extraGameInfo [j].bDarkness = CFReadByte (fp);
	if (player_file_version >= 119) {
		extraGameInfo [j].bTeamDoors = CFReadByte (fp);
		extraGameInfo [j].bEnableCheats = CFReadByte (fp);
		}
	if (player_file_version >= 120) {
		extraGameInfo [j].bTargetIndicators = CFReadByte (fp);
		extraGameInfo [j].bDamageIndicators = CFReadByte (fp);
		}
	if (player_file_version >= 121) {
		extraGameInfo [j].bCloakedIndicators = CFReadByte (fp);
		extraGameInfo [j].bFriendlyIndicators = CFReadByte (fp);
		extraGameInfo [j].bHeadLights = CFReadByte (fp);
		extraGameInfo [j].bPowerupLights = CFReadByte (fp);
		extraGameInfo [j].nSpotSize = CFReadByte (fp);
		extraGameInfo [j].nSpotStrength = extraGameInfo [j].nSpotSize;
		}
	if (player_file_version >= 122)
		extraGameInfo [j].bTowFlags = CFReadByte (fp);
	if (player_file_version >= 123)
		gameOptions [j].render.smoke.bDecreaseLag = CFReadByte (fp);
	if (player_file_version >= 124)
		gameOptions [j].render.bAutoTransparency = CFReadByte (fp);
	if (!j) {
		if (player_file_version >= 125)
			extraGameInfo [j].bUseHitAngles = CFReadByte (fp);
		if (player_file_version >= 126)
			extraGameInfo [j].bLightTrails = CFReadByte (fp);
		}
	if (player_file_version >= 126) {
		gameOptions [j].render.smoke.bSyncSizes = CFReadInt (fp);
		for (i = 1; i < 4; i++) {
			gameOptions [j].render.smoke.nDens [i] = CFReadInt (fp);
			gameOptions [j].render.smoke.nSize [i] = CFReadInt (fp);
			}
		}
	if (!j) {
		if (player_file_version >= 127) {
			extraGameInfo [j].bTracers = CFReadByte (fp);
			extraGameInfo [j].bShockwaves = CFReadByte (fp);
			}
		}
	if (player_file_version >= 128)
		gameOptions [j].render.smoke.bDisperse = (int) CFReadByte (fp);
	if (player_file_version >= 129)
		extraGameInfo [j].bHitIndicators = (int) CFReadByte (fp);
	if (player_file_version >= 130)
		for (i = 0; i < 4; i++)
			gameOptions [j].render.smoke.nLife [i] = CFReadInt (fp);
	if (player_file_version >= 131) {
		gameOptions [j].render.shadows.bRobots = (int) CFReadByte (fp);
		gameOptions [j].render.shadows.bMissiles = (int) CFReadByte (fp);
		gameOptions [j].render.shadows.bReactors = (int) CFReadByte (fp);
		}
	if (player_file_version >= 132)
		gameOptions [j].render.shadows.bPlayers = (int) CFReadByte (fp);
	if (player_file_version >= 133)
		gameOptions [j].render.shadows.bFast = (int) CFReadByte (fp);
	if (player_file_version >= 134)
		gameOptions [j].render.shadows.nReach = (int) CFReadByte (fp);
	if (player_file_version >= 135)
		gameOptions [j].render.shadows.nClip = (int) CFReadByte (fp);
	if (player_file_version >= 136) {
		gameOptions [j].render.powerups.b3D = (int) CFReadByte (fp);
		gameOptions [j].render.powerups.nSpin = (int) CFReadByte (fp);
		}
	if (player_file_version >= 137)
		gameOptions [j].gameplay.bIdleAnims = (int) CFReadByte (fp);
	if (player_file_version >= 138)
		if (!j)
			gameStates.app.nDifficultyLevel = (int) CFReadByte (fp);
	if (player_file_version >= 139)
		gameOptions [j].demo.bOldFormat = (int) CFReadByte (fp);
	if (player_file_version >= 140)
		gameOptions [j].render.cockpit.bObjectTally = (int) CFReadByte (fp);
	if (player_file_version >= 141)
		if (!j)
			extraGameInfo [0].nLightRange = CFReadByte (fp);
	if (player_file_version >= 142)
		if (j)
			extraGameInfo [j].bCompetition = CFReadByte (fp);
	if (player_file_version >= 143)
		extraGameInfo [j].bFlickerLights = CFReadByte (fp);
	if (player_file_version >= 144)
		if (!j)
			extraGameInfo [j].bSmokeGrenades = CFReadByte (fp);
	if (player_file_version >= 144)
		if (!j)
			extraGameInfo [j].nMaxSmokeGrenades = CFReadByte (fp);
	}
mpParams.bDarkness = extraGameInfo [1].bDarkness;
mpParams.bTeamDoors = extraGameInfo [1].bTeamDoors;
mpParams.bEnableCheats = extraGameInfo [1].bEnableCheats;
if (errno_ret == EZERO)
	KCSetControls();
if (CFClose(fp)) 
	if (errno_ret == EZERO)
		errno_ret = errno;
extraGameInfo [1].bDisableReactor = 0;
if (rewrite_it)
	WritePlayerFile();
return errno_ret;
}

//------------------------------------------------------------------------------

//finds entry for this level in table.  if not found, returns ptr to 
//empty entry.  If no empty entries, takes over last one 
int FindHLIEntry()
{
	int i;

	for (i=0;i<nHighestLevels;i++)
		if (!stricmp(highestLevels[i].shortname,gameData.missions.list[gameData.missions.nCurrentMission].filename))
			break;

	if (i==nHighestLevels) {		//not found.  create entry

		if (i==MAX_MISSIONS)
			i--;		//take last entry
		else
			nHighestLevels++;

		strcpy(highestLevels[i].shortname,gameData.missions.list[gameData.missions.nCurrentMission].filename);
		highestLevels[i].level_num			= 0;
	}

	return i;
}

//------------------------------------------------------------------------------
//set a new highest level for tPlayer for this mission
void SetHighestLevel(int levelnum)
{
	int ret,i;

if ((ret=ReadPlayerFile(0)) != EZERO)
	if (ret != ENOENT)		//if file doesn't exist, that's ok
		return;
i = FindHLIEntry();
if (levelnum > highestLevels[i].level_num)
	highestLevels[i].level_num	= levelnum;
WritePlayerFile();
}

//------------------------------------------------------------------------------
//gets the tPlayer's highest level from the file for this mission
int GetHighestLevel(void)
{
	int i;
	int highest_saturnLevel = 0;

ReadPlayerFile(0);
#ifndef SATURN
if (strlen (gameData.missions.list [gameData.missions.nCurrentMission].filename) == 0)	{
	for (i = 0; i < nHighestLevels; i++)
		if (!stricmp(highestLevels [i].shortname, "DESTSAT")) 	//	Destination Saturn.
		 	highest_saturnLevel	= highestLevels[i].level_num; 
}
#endif
i = highestLevels [FindHLIEntry()].level_num;
if (highest_saturnLevel > i)
   i = highest_saturnLevel;
return i;
}

//------------------------------------------------------------------------------

//write out tPlayer's saved games.  returns errno (0 == no error)
int WritePlayerFile()
{
	char filename[FILENAME_LEN];		// because of ":gameData.multi.players:" path
	CFILE *fp;
	int errno_ret, h, i, j;

//	#ifdef APPLE_DEMO		// no saving of tPlayer files in Apple OEM version
//	return 0;
//	#endif

	errno_ret = WriteConfigFile();

	sprintf(filename,"%s.plr",gameData.multi.players[gameData.multi.nLocalPlayer].callsign);
	fp = CFOpen(filename, gameFolders.szProfDir, "wb", 0);

#if 0
	//check filename
	if (fp && isatty(fileno (fp)) {

		//if the callsign is the name of a tty device, prepend a char

		fclose(fp);
		sprintf(filename,"$%.7s.plr",gameData.multi.players[gameData.multi.nLocalPlayer].callsign);
		fp			= fopen(filename,"wb");
	}
#endif

	if (!fp)
              return errno;

	errno_ret			= EZERO;

	//Write out tPlayer's info
	CFWriteInt(SAVE_FILE_ID, fp);
	CFWriteShort(PLAYER_FILE_VERSION, fp);

	CFWriteShort((short) Game_window_w, fp);
	CFWriteShort((short) Game_window_h, fp);

	CFWriteByte ((sbyte) playerDefaultDifficulty, fp);
	CFWriteByte ((sbyte) gameOptions [0].gameplay.bAutoLeveling, fp);
	CFWriteByte ((sbyte) gameOptions [0].render.cockpit.bReticle, fp);
	CFWriteByte ((sbyte) ((gameStates.render.cockpit.nModeSave != -1)?gameStates.render.cockpit.nModeSave:gameStates.render.cockpit.nMode), fp);   //if have saved mode, write it instead of letterbox/rear view
	CFWriteByte ((sbyte) gameStates.video.nDefaultDisplayMode, fp);
	CFWriteByte ((sbyte) gameOptions [0].render.cockpit.bMissileView, fp);
	CFWriteByte ((sbyte) gameOptions [0].gameplay.bHeadlightOn, fp);
	CFWriteByte ((sbyte) gameOptions [0].render.cockpit.bGuidedInMainView, fp);
	CFWriteByte ((sbyte) gameOptions [0].render.bAutomapAlwaysHires, fp);

	//write higest level info
	Assert(nHighestLevels <= MAX_MISSIONS);
	CFWriteShort(nHighestLevels, fp);
	if ((CFWrite(highestLevels, sizeof(hli), nHighestLevels, fp) != nHighestLevels))
	{
		errno_ret			= errno;
		CFClose(fp);
		return errno_ret;
	}

	if ((CFWrite(multiData.msg.szMacro, MAX_MESSAGE_LEN, 4, fp) != 4))
	{
		errno_ret			= errno;
		CFClose(fp);
		return errno_ret;
	}

	//write KConfig info
	{

		controlType_dos = gameConfig.nControlType;
		if (CFWrite(controlSettings.custom, MAX_CONTROLS * CONTROL_MAX_TYPES, 1, fp ) != 1)
			errno_ret=errno;
		else if (CFWrite(&controlType_dos, sizeof(ubyte), 1, fp) != 1)
			errno_ret=errno;
		else if (CFWrite(&controlType_win, sizeof(ubyte), 1, fp ) != 1)
			errno_ret=errno;
		else if (CFWrite(gameOptions [0].input.joySensitivity, sizeof(ubyte), 1, fp) != 1)
			errno_ret=errno;

		for (i=0;i<11;i++)
		{
			CFWrite(&primaryOrder[i], sizeof(ubyte), 1, fp);
			CFWrite(&secondaryOrder[i], sizeof(ubyte), 1, fp);
		}

		CFWriteInt(Cockpit_3dView[0], fp);
		CFWriteInt(Cockpit_3dView[1], fp);

		CFWriteInt(networkData.nNetLifeKills, fp);
		CFWriteInt(networkData.nNetLifeKilled, fp);
		i=get_lifetime_checksum (networkData.nNetLifeKills, networkData.nNetLifeKilled);
#if TRACE				
		con_printf (CON_DEBUG,"Writing: Lifetime checksum is %d\n",i);
#endif
		CFWriteInt(i,fp);
	}

	//write guidebot name
	CFWriteString(gameData.escort.szRealName, fp);
	{
		char buf[128];
		strcpy(buf, "DOS joystick");
		CFWriteString(buf, fp);  // Write out current joystick for player.
	}

	CFWrite(controlSettings.d2xCustom, MAX_D2X_CONTROLS, 1, fp);
// write D2X-XL stuff
CFWriteInt (sizeof (tGameOptions), fp);
for (j = 0; j < 2; j++) {
	if (!j) {
		CFWriteByte (extraGameInfo [0].bFixedRespawns, fp);
		CFWriteByte (extraGameInfo [0].bFriendlyFire, fp);
		}
	CFWriteByte ((sbyte) gameOptions [j].render.nMaxFPS, fp);
	if (!j)
		CFWriteByte ((sbyte) (extraGameInfo [0].nSpawnDelay / 1000), fp);
	CFWriteByte ((sbyte) gameOptions [j].input.joyDeadZones [0], fp);
	CFWriteByte ((sbyte) gameOptions [j].render.cockpit.nWindowSize, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.cockpit.nWindowPos, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.cockpit.nWindowZoom, fp);
	if (!j)
		CFWriteByte ((sbyte) extraGameInfo [0].bPowerupsOnRadar, fp);
	CFWriteByte ((sbyte) gameOptions [j].input.keyRampScale, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.color.bAmbientLight, fp);
	CFWriteByte ((sbyte) gameOptions [j].ogl.bSetGammaRamp, fp);
	if (!j)
		CFWriteByte ((sbyte) extraGameInfo [0].nZoomMode, fp);
	for (i = 0; i < 3; i++)
		CFWriteByte ((sbyte) gameOptions [j].input.bRampKeys [i], fp);
	if (!j) {
		CFWriteByte ((sbyte) extraGameInfo [0].bEnhancedCTF, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].bRobotsHitRobots, fp);
		}
	CFWriteByte ((sbyte) gameOptions [j].gameplay.nAutoSelectWeapon, fp);
	if (!j)
		CFWriteByte ((sbyte) extraGameInfo [0].bAutoDownload, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.color.bGunLight, fp);
	CFWriteByte ((sbyte) gameStates.multi.bUseTracker, fp);
	CFWriteByte ((sbyte) gameOptions [j].gameplay.bFastRespawn, fp);
	if (!j)
		CFWriteByte ((sbyte) extraGameInfo [0].bDualMissileLaunch, fp);
	CFWriteByte ((sbyte) gameOptions [j].gameplay.nPlayerDifficultyLevel, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.bTransparentEffects, fp);
	if (!j)
		CFWriteByte ((sbyte) extraGameInfo [0].bRobotsOnRadar, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.bAllSegs, fp);
	if (!j)
		CFWriteByte ((sbyte) extraGameInfo [0].grWallTransparency, fp);
	CFWriteByte ((sbyte) gameOptions [j].input.mouseSensitivity, fp);
	CFWriteByte ((sbyte) gameOptions [j].multi.bUseMacros, fp);
	if (!j)
		CFWriteByte ((sbyte) extraGameInfo [0].bWiggle, fp);
	CFWriteByte ((sbyte) gameOptions [j].movies.nQuality, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.color.bWalls, fp);
	CFWriteByte ((sbyte) gameOptions [j].input.bLinearJoySens, fp);
	if (!j) {
		CFWriteByte ((sbyte) extraGameInfo [0].nSpeedBoost, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].bDropAllMissiles, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].bImmortalPowerups, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].bUseCameras, fp);
		}
	CFWriteByte ((sbyte) gameOptions [j].render.cameras.bFitToWall, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.cameras.nFPS, fp);
	if (!j)
		CFWriteByte ((sbyte) extraGameInfo [0].nFusionPowerMod, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.color.bUseLightMaps, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.cockpit.bHUD, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.color.nLightMapRange, fp);
	if (!j) {
		CFWriteByte ((sbyte) extraGameInfo [0].bMouseLook, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].bMultiBosses, fp);
		}
	CFWriteByte ((sbyte) gameOptions [j].app.nVersionFilter, fp);
	if (!j) {
		CFWriteByte ((sbyte) extraGameInfo [0].bSmartWeaponSwitch, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].bFluidPhysics, fp);
		}
	CFWriteByte ((sbyte) gameOptions [j].render.nQuality, fp);
	CFWriteByte ((sbyte) gameOptions [j].movies.bSubTitles, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.textures.nQuality, fp);
	CFWriteInt (gameOptions [j].render.cameras.nSpeed, fp);
	if (!j)
		CFWriteByte (extraGameInfo [0].nWeaponDropMode, fp);
	CFWriteInt (gameOptions [j].menus.bSmartFileSearch, fp);
	CFWriteInt (GetDlTimeout (), fp);
	if (!j) {
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.nCaptureVirusLimit, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.nCaptureTimeLimit, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.nMaxVirusCapacity, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.nBumpVirusCapacity, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.nBashVirusCapacity, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.nVirusGenTime, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.nVirusLifespan, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.nVirusStability, fp); 
		CFWriteShort((short) extraGameInfo [0].entropy.nEnergyFillRate, fp);
		CFWriteShort((short) extraGameInfo [0].entropy.nShieldFillRate, fp);
		CFWriteShort((short) extraGameInfo [0].entropy.nShieldDamageRate, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.bRevertRooms, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.bDoConquerWarning, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.nOverrideTextures, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.bBrightenRooms, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].entropy.bPlayerHandicap, fp);

		CFWriteByte ((sbyte) mpParams.nLevel, fp);
		CFWriteByte ((sbyte) mpParams.nGameType, fp);
		CFWriteByte ((sbyte) mpParams.nGameMode, fp);
		CFWriteByte ((sbyte) mpParams.nGameAccess, fp);
		CFWriteByte ((sbyte) mpParams.bShowPlayersOnAutomap, fp);
		CFWriteByte ((sbyte) mpParams.nDifficulty, fp);
		CFWriteInt(mpParams.nWeaponFilter, fp);
		CFWriteInt(mpParams.nReactorLife, fp);
		CFWriteByte ((sbyte) mpParams.nMaxTime, fp);
		CFWriteByte ((sbyte) mpParams.nKillGoal, fp);
		CFWriteByte ((sbyte) mpParams.bInvul, fp);
		CFWriteByte ((sbyte) mpParams.bMarkerView, fp);
		CFWriteByte ((sbyte) mpParams.bAlwaysBright, fp);
		CFWriteByte ((sbyte) mpParams.bBrightPlayers, fp);
		CFWriteByte ((sbyte) mpParams.bShowAllNames, fp);
		CFWriteByte ((sbyte) mpParams.bShortPackets, fp);
		CFWriteByte ((sbyte) mpParams.nPPS, fp);
		CFWriteInt(mpParams.udpClientPort, fp);
		CFWrite(mpParams.szServerIpAddr, 16, 1, fp);
		}
	CFWriteByte ((sbyte) gameOptions [j].render.bOptimize, fp);
	if (!j) {
		CFWriteByte ((sbyte) extraGameInfo [1].bRotateLevels, fp);
		CFWriteByte ((sbyte) extraGameInfo [1].bDisableReactor, fp);
		}
	for (i = 0; i < 4; i++)
		CFWriteByte ((sbyte) gameOptions [0].input.joyDeadZones [i], fp);
	for (i = 0; i < 4; i++)
		CFWriteByte ((sbyte) gameOptions [0].input.joySensitivity [i], fp);
	CFWriteByte ((sbyte) gameOptions [j].input.bSyncJoyAxes, fp);
	if (!j) {
		CFWriteByte ((sbyte) extraGameInfo [1].bDualMissileLaunch, fp);
		CFWriteByte ((sbyte) extraGameInfo [1].bMouseLook, fp);
		}
	for (i = 0; i < 3; i++)
		CFWriteByte ((sbyte) gameOptions [0].input.mouseSensitivity [i], fp);
	CFWriteByte ((sbyte) gameOptions [j].input.bSyncMouseAxes, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.color.bMix, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.color.bCap, fp);
	if (!j)
		CFWriteByte ((sbyte) extraGameInfo [0].nWeaponIcons, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.weaponIcons.bSmall, fp);
	if (!j)
		CFWriteByte ((sbyte) extraGameInfo [0].bAutoBalanceTeams, fp);
	CFWriteByte ((sbyte) gameOptions [j].movies.bResize, fp);
	CFWriteByte ((sbyte) gameOptions [j].menus.bShowLevelVersion, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.weaponIcons.nSort, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.weaponIcons.bShowAmmo, fp);
	CFWriteByte ((sbyte) gameOptions [j].input.bUseMouse, fp);
	CFWriteByte ((sbyte) gameOptions [j].input.bUseJoystick, fp);
	CFWriteByte ((sbyte) gameOptions [j].input.bUseHotKeys, fp);
	if (!j) {
		CFWriteByte ((sbyte) extraGameInfo [0].bSafeUDP, fp);
		CFWrite(mpParams.szServerIpAddr + 16, 6, 1, fp);
		}
	CFWriteByte ((sbyte) gameOptions [j].render.cockpit.bTextGauges, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.cockpit.bScaleGauges, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.weaponIcons.bEquipment, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.weaponIcons.alpha, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.cockpit.bFlashGauges, fp);
	if (!j) {
		for (i = 0; i < 2; i++)
			CFWriteByte ((sbyte) extraGameInfo [i].bFastPitch, fp);
		CFWriteInt (gameStates.multi.nConnection, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].bUseSmoke, fp);
		}
	CFWriteInt (gameOptions [j].render.smoke.nDens [0], fp);
	CFWriteInt (gameOptions [j].render.smoke.nSize [0], fp);
	CFWriteByte ((sbyte) gameOptions [j].render.smoke.bPlayers, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.smoke.bRobots, fp);
	CFWriteByte ((sbyte) gameOptions [j].render.smoke.bMissiles, fp);
	if (!j) {
		CFWriteByte ((sbyte) extraGameInfo [0].bDamageExplosions, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].bThrusterFlames, fp);
		}
	CFWriteByte ((sbyte) gameOptions [j].render.smoke.bCollisions, fp);
	CFWriteByte ((sbyte) gameOptions [j].gameplay.bShieldWarning, fp);
	CFWriteByte ((sbyte) gameOptions [j].app.bExpertMode, fp);
	if (!j)
		CFWriteByte ((sbyte) extraGameInfo [0].bShadows, fp);
	CFWriteInt (gameOptions [j].render.shadows.nLights, fp);
	if (!j) {
		CFWriteInt (gameStates.ogl.nContrast, fp);
		CFWriteByte ((sbyte) extraGameInfo [0].bRenderShield, fp);
		}
	CFWriteByte ((sbyte) gameOptions [j].gameplay.bInventory, fp);
	CFWriteInt (gameOptions [j].input.bJoyMouse, fp);
	if (!j) {
		CFWriteInt (displayModeInfo [NUM_DISPLAY_MODES].w, fp);
		CFWriteInt (displayModeInfo [NUM_DISPLAY_MODES].h, fp);
		}
	CFWriteInt (gameOptions [j].render.cockpit.bMouseIndicator, fp);
	CFWriteInt (extraGameInfo [j].bTeleporterCams, fp);
	CFWriteInt (gameOptions [j].render.cockpit.bSplitHUDMsgs, fp);
	CFWriteByte (gameOptions [j].input.joyDeadZones [4], fp);
	CFWriteByte (gameOptions [j].input.joySensitivity [4], fp);
	if (!j) {
		tMonsterballForce *pf = extraGameInfo [0].monsterball.forces;
		CFWriteByte (extraGameInfo [0].monsterball.nBonus, fp);
		for (h = 0; h < MAX_MONSTERBALL_FORCES; h++, pf++) {
			CFWriteByte (pf->nWeaponId, fp);
			CFWriteByte (pf->nForce, fp);
			}
		}
	if (!j)
		CFWriteByte (extraGameInfo [0].monsterball.nSizeMod, fp);
	CFWriteInt (gameOptions [j].render.bDynLighting, fp);
	CFWriteInt (gameOptions [j].ogl.bLightObjects, fp);
	CFWriteInt (gameOptions [j].ogl.nMaxLights, fp);
	CFWriteByte (extraGameInfo [j].bDarkness, fp);
	CFWriteByte (extraGameInfo [j].bTeamDoors, fp);
	CFWriteByte (extraGameInfo [j].bEnableCheats, fp);
	CFWriteByte (extraGameInfo [j].bTargetIndicators, fp);
	CFWriteByte (extraGameInfo [j].bDamageIndicators, fp);
	CFWriteByte (extraGameInfo [j].bFriendlyIndicators, fp);
	CFWriteByte (extraGameInfo [j].bCloakedIndicators, fp);
	CFWriteByte (extraGameInfo [j].bHeadLights, fp);
	CFWriteByte (extraGameInfo [j].bPowerupLights, fp);
	CFWriteByte (extraGameInfo [j].nSpotSize, fp);
	CFWriteByte (extraGameInfo [j].bTowFlags, fp);
	CFWriteByte (gameOptions [j].render.smoke.bDecreaseLag, fp);
	CFWriteByte (gameOptions [j].render.bAutoTransparency, fp);
	if (!j) {
		CFWriteByte (extraGameInfo [j].bUseHitAngles, fp);
		CFWriteByte (extraGameInfo [j].bLightTrails, fp);
		}
	CFWriteInt (gameOptions [j].render.smoke.bSyncSizes, fp);
	for (i = 1; i < 4; i++) {
		CFWriteInt (gameOptions [j].render.smoke.nDens [i], fp);
		CFWriteInt (gameOptions [j].render.smoke.nSize [i], fp);
		}
	if (!j) {
		CFWriteByte (extraGameInfo [j].bTracers, fp);
		CFWriteByte (extraGameInfo [j].bShockwaves, fp);
		}
	CFWriteByte (gameOptions [j].render.smoke.bDisperse, fp);
	CFWriteByte (extraGameInfo [j].bHitIndicators, fp);
	for (i = 0; i < 4; i++)
		CFWriteInt (gameOptions [j].render.smoke.nLife [i], fp);
	CFWriteByte (gameOptions [j].render.shadows.bRobots, fp);
	CFWriteByte (gameOptions [j].render.shadows.bMissiles, fp);
	CFWriteByte (gameOptions [j].render.shadows.bReactors, fp);
	CFWriteByte (gameOptions [j].render.shadows.bPlayers, fp);
	CFWriteByte (gameOptions [j].render.shadows.bFast, fp);
	CFWriteByte (gameOptions [j].render.shadows.nReach, fp);
	CFWriteByte (gameOptions [j].render.shadows.nClip, fp);
	CFWriteByte (gameOptions [j].render.powerups.b3D, fp);
	CFWriteByte (gameOptions [j].render.powerups.nSpin, fp);
	CFWriteByte (gameOptions [j].gameplay.bIdleAnims, fp);
	if (!j)
		CFWriteByte (gameStates.app.nDifficultyLevel, fp);
	CFWriteByte (gameOptions [j].demo.bOldFormat, fp);
	CFWriteByte (gameOptions [j].render.cockpit.bObjectTally, fp);
	if (!j)
		CFWriteByte (extraGameInfo [0].nLightRange, fp);
	if (j)
		CFWriteByte (extraGameInfo [j].bCompetition, fp);
	CFWriteByte (extraGameInfo [j].bFlickerLights, fp);
	if (!j) {
		CFWriteByte (extraGameInfo [j].bSmokeGrenades, fp);
		CFWriteByte (extraGameInfo [j].nMaxSmokeGrenades, fp);
		}
// end of D2X-XL stuff
	}

	if (CFClose (fp))
		errno_ret = errno;

	if (errno_ret != EZERO) {
		CFDelete(filename, gameFolders.szProfDir);         //delete bogus fp
		ExecMessageBox(TXT_ERROR, NULL, 1, TXT_OK, "%s\n\n%s",TXT_ERROR_WRITING_PLR, strerror(errno_ret));
	}

	return errno_ret;

}

//update the tPlayer's highest level.  returns errno (0 == no error)
int update_player_file()
{
	int ret;

	if ((ret=ReadPlayerFile(0)) != EZERO)
		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return ret;

	return WritePlayerFile();
}

int get_lifetime_checksum (int a,int b)
 {
  int num;

  // confusing enough to beat amateur disassemblers? Lets hope so

  num=(a<<8 ^ b);
  num^=(a | b);
  num*=num>>2;
  return (num);
 }
  

//------------------------------------------------------------------------------

