/* $Id: config.c,v 1.10 2003/10/04 20:03:11 btb Exp $ */
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
#include "game.h"
#include "menu.h"
#include "movie.h"
#include "digi.h"
#include "kconfig.h"
#include "palette.h"
#include "joy.h"
#include "songs.h"
#include "args.h"
#include "player.h"
#include "mission.h"
#include "mono.h"
#include "error.h"
#include "pa_enabl.h"

#ifdef RCS
static char rcsid[] = "$Id: config.c,v 1.10 2003/10/04 20:03:11 btb Exp $";
#endif

static char *pszDigiVolume = "DigiVolume";
static char *pszMidiVolume = "MidiVolume";
static char *pszRedbookEnabled = "RedbookEnabled";
static char *pszRedbookVolume = "RedbookVolume";
static char *pszDetailLevel = "DetailLevel";
static char *pszGammaLevel = "GammaLevel";
static char *pszStereoRev = "StereoReverse";
static char *pszJoystickMin = "JoystickMin";
static char *pszJoystickMax = "JoystickMax";
static char *pszJoystickCen = "JoystickCen";
static char *pszLastPlayer = "LastPlayer";
static char *pszLastMission = "LastMission";
static char *pszVrType = "VRType";
static char *pszVrResolution = "VR_resolution";
static char *pszVrTracking = "VR_tracking";
static char *pszHiresMovies = "Hires Movies";

int digi_driver_board_16;
int digi_driver_dma_16;

void InitCustomDetails(void);

tGameConfig gameConfig;

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

#define CL_MC0 0xF8F
#define CL_MC1 0xF8D
/*
void CrystalLakeWriteMCP( ushort mc_addr, ubyte mc_data )
{
	_disable();
	outp( CL_MC0, 0xE2 );				// Write password
	outp( mc_addr, mc_data );		// Write data
	_enable();
}

ubyte CrystalLakeReadMCP( ushort mc_addr )
{
	ubyte value;
	_disable();
	outp( CL_MC0, 0xE2 );		// Write password
	value = inp( mc_addr );		// Read data
	_enable();
	return value;
}

void CrystalLakeSetSB()
{
	ubyte tmp;
	tmp = CrystalLakeReadMCP( CL_MC1 );
	tmp &= 0x7F;
	CrystalLakeWriteMCP( CL_MC1, tmp );
}

void CrystalLakeSetWSS()
{
	ubyte tmp;
	tmp = CrystalLakeReadMCP( CL_MC1 );
	tmp |= 0x80;
	CrystalLakeWriteMCP( CL_MC1, tmp );
}
*/

// ----------------------------------------------------------------------------
//gameOpts->movies.bHires might be changed by -nohighres, so save a "real" copy of it
int bHiresMoviesSave;
int bRedbookEnabledSave;

int ReadConfigFile()
{
	CFILE *infile;
	char line[80], *token, *value, *ptr;
	ubyte gamma;
	int joy_axis_min[7];
	int joy_axis_center[7];
	int joy_axis_max[7];
	int i;

	strcpy( gameConfig.szLastPlayer, "" );

	joy_axis_min[0] = joy_axis_min[1] = joy_axis_min[2] = joy_axis_min[3] = 0;
	joy_axis_max[0] = joy_axis_max[1] = joy_axis_max[2] = joy_axis_max[3] = 0;
	joy_axis_center[0] = joy_axis_center[1] = joy_axis_center[2] = joy_axis_center[3] = 0;
	JoySetCalVals(joy_axis_min, joy_axis_center, joy_axis_max);
	gameConfig.nDigiVolume = 8;
	gameConfig.nMidiVolume = 8;
	gameConfig.nRedbookVolume = 8;
	gameConfig.nControlType = 0;
	gameConfig.bReverseChannels = 0;

	//set these here in case no cfg file
	bHiresMoviesSave = gameOpts->movies.bHires;
	bRedbookEnabledSave = gameStates.sound.bRedbookEnabled;

	infile = CFOpen("descent.cfg", gameFolders.szConfigDir, "rt", 0);
	if (infile == NULL) {
		WIN(CheckMovieAttributes());
		return 1;
	}
	while (!CFEoF(infile)) {
		memset(line, 0, 80);
		CFGetS(line, 80, infile);
		ptr = &(line[0]);
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') {
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
			if (!(value && token)) {
				LogErr ("configuration file (descent.cfg) looks messed up.\n");
				continue;
				}
			if (value[strlen(value)-1] == '\n')
				value[strlen(value)-1] = 0;
/*			if (!strcmp(token, digi_dev8_str))
				digi_driver_board = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_dev16_str))
				digi_driver_board_16 = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_port_str))
				digi_driver_port = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_irq_str))
				digi_driver_irq = strtol(value, NULL, 10);
			else if (!strcmp(token, digi_dma8_str))
				digi_driver_dma = strtol(value, NULL, 10);
			else if (!strcmp(token, digi_dma16_str))
				digi_driver_dma_16 = strtol(value, NULL, 10);
			else*/ if (!strcmp(token, pszDigiVolume))
				gameConfig.nDigiVolume = (ubyte) strtol(value, NULL, 10);
			else/* if (!strcmp(token, midi_dev_str))
				digi_midiType = strtol(value, NULL, 16);
			else if (!strcmp(token, midi_port_str))
				digi_midi_port = strtol(value, NULL, 16);
			else*/ if (!strcmp(token, pszMidiVolume))
				gameConfig.nMidiVolume = (ubyte) strtol(value, NULL, 10);
			else if (!strcmp(token, pszRedbookEnabled))
				gameStates.sound.bRedbookEnabled = bRedbookEnabledSave = strtol(value, NULL, 10);
			else if (!strcmp(token, pszRedbookVolume))
				gameConfig.nRedbookVolume = (ubyte) strtol(value, NULL, 10);
			else if (!strcmp(token, pszStereoRev))
				gameConfig.bReverseChannels = (ubyte) strtol(value, NULL, 10);
			else if (!strcmp(token, pszGammaLevel)) {
				gamma = (ubyte) strtol(value, NULL, 10);
				GrSetPaletteGamma( gamma );
			}
			else if (!strcmp(token, pszDetailLevel)) {
				gameStates.app.nDetailLevel = strtol(value, NULL, 10);
				if (gameStates.app.nDetailLevel == NUM_DETAIL_LEVELS-1) {
					int count,dummy,oc,od,wd,wrd,da,sc;

					count = sscanf (value, "%d,%d,%d,%d,%d,%d,%d\n",&dummy,&oc,&od,&wd,&wrd,&da,&sc);

					if (count == 7) {
						gameStates.render.detail.nObjectComplexity = oc;
						gameStates.render.detail.nObjectDetail = od;
						gameStates.render.detail.nWallDetail = wd;
						gameStates.render.detail.nWallRenderDepth = wrd;
						gameStates.render.detail.nDebrisAmount = da;
						gameStates.sound.nMaxSoundChannels = sc;
						InitCustomDetails();
					}
				  #ifdef PA_3DFX_VOODOO   // Set to highest detail because you can't change em
					   gameStates.render.detail.nObjectComplexity=gameStates.render.detail.nObjectDetail=gameStates.render.detail.nWallDetail=
						gameStates.render.detail.nWallRenderDepth=gameStates.render.detail.nDebrisAmount=SoundChannels = NUM_DETAIL_LEVELS-1;
						gameStates.app.nDetailLevel=NUM_DETAIL_LEVELS-1;
						InitCustomDetails();
					#endif
				}
			}
			else if (!strcmp(token, pszJoystickMin))	{
				sscanf( value, "%d,%d,%d,%d", &joy_axis_min[0], &joy_axis_min[1], &joy_axis_min[2], &joy_axis_min[3] );
			}
			else if (!strcmp(token, pszJoystickMax))	{
				sscanf( value, "%d,%d,%d,%d", &joy_axis_max[0], &joy_axis_max[1], &joy_axis_max[2], &joy_axis_max[3] );
			}
			else if (!strcmp(token, pszJoystickCen))	{
				sscanf( value, "%d,%d,%d,%d", &joy_axis_center[0], &joy_axis_center[1], &joy_axis_center[2], &joy_axis_center[3] );
			}
			else if (!strcmp(token, pszLastPlayer))	{
				char * p;
				strncpy( gameConfig.szLastPlayer, value, CALLSIGN_LEN );
				p = strchr( gameConfig.szLastPlayer, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, pszLastMission))	{
				char * p;
				strncpy( gameConfig.szLastMission, value, MISSION_NAME_LEN );
				p = strchr( gameConfig.szLastMission, '\n');
				if ( p ) *p = 0;
			} else if (!strcmp(token, pszVrType)) {
				gameConfig.vrType = strtol(value, NULL, 10);
			} else if (!strcmp(token, pszVrResolution)) {
				gameConfig.vrResolution = strtol(value, NULL, 10);
			} else if (!strcmp(token, pszVrTracking)) {
				gameConfig.vrTracking = strtol(value, NULL, 10);
			} else if (!strcmp(token, pszHiresMovies)) {
				bHiresMoviesSave = gameOpts->movies.bHires = strtol(value, NULL, 10);
			}
		}
	}
	CFClose(infile);
	JoySetCalVals(joy_axis_min, joy_axis_center, joy_axis_max);
	i = FindArg( "-volume" );

	if ( i > 0 )	{
		i = atoi( Args[i+1] );
		if ( i < 0 ) i = 0;
		if ( i > 100 ) i = 100;
		gameConfig.nDigiVolume = (i*8)/100;
		gameConfig.nMidiVolume = (i*8)/100;
		gameConfig.nRedbookVolume = (i*8)/100;
	}

	if ( gameConfig.nDigiVolume > 8 ) gameConfig.nDigiVolume = 8;
	if ( gameConfig.nMidiVolume > 8 ) gameConfig.nMidiVolume = 8;
	if ( gameConfig.nRedbookVolume > 8 ) gameConfig.nRedbookVolume = 8;

	DigiMidiVolume( (gameConfig.nDigiVolume*32768)/8, (gameConfig.nMidiVolume*128)/8 );
/*
	//printf( "DigiDeviceID: 0x%x\n", digi_driver_board );
	//printf( "DigiPort: 0x%x\n", digi_driver_port		);
	//printf( "DigiIrq: 0x%x\n",  digi_driver_irq		);
	//printf( "DigiDma: 0x%x\n",	digi_driver_dma	);
	//printf( "MidiDeviceID: 0x%x\n", digi_midiType	);
	//printf( "MidiPort: 0x%x\n", digi_midi_port		);
  	KeyGetChar();
*/

	/*gameConfig.nMidiType = digi_midiType;
	gameConfig.nDigiType = digi_driver_board;
	gameConfig.nDigiDMA = digi_driver_dma;*/

#if 0
	if (digi_driver_board_16 > 0 && !FindArg("-no16bit") && digi_driver_board_16 != _GUS_16_ST) {
		digi_driver_board = digi_driver_board_16;
		digi_driver_dma = digi_driver_dma_16;
	}

	// HACK!!!
	//Hack to make some cards look like others, such as
	//the Crytal Lake look like Microsoft Sound System
	if ( digi_driver_board == _CRYSTAL_LAKE_8_ST )	{
		ubyte tmp;
		tmp = CrystalLakeReadMCP( CL_MC1 );
		if ( !(tmp & 0x80) )
			atexit( CrystalLakeSetSB );		// Restore to SB when done.
	 	CrystalLakeSetWSS();
		digi_driver_board = _MICROSOFT_8_ST;
	} else if ( digi_driver_board == _CRYSTAL_LAKE_16_ST )	{
		ubyte tmp;
		tmp = CrystalLakeReadMCP( CL_MC1 );
		if ( !(tmp & 0x80) )
			atexit( CrystalLakeSetSB );		// Restore to SB when done.
	 	CrystalLakeSetWSS();
		digi_driver_board = _MICROSOFT_16_ST;
	} else if ( digi_driver_board == _AWE32_8_ST )	{
		digi_driver_board = _SB16_8_ST;
	} else if ( digi_driver_board == _AWE32_16_ST )	{
		digi_driver_board = _SB16_16_ST;
	} else
		digi_driver_board		= digi_driver_board;
#else
	infile = CFOpen("descentw.cfg", gameFolders.szConfigDir, "rt", 0);
	if (infile) {
		while (!CFEoF(infile))
		{
			memset(line, 0, 80);
			CFGetS(line, 80, infile);
			ptr = &(line[0]);
			while (isspace(*ptr))
				ptr++;
			if (*ptr != '\0') {
				token = strtok(ptr, "=");
				value = strtok(NULL, "=");
				if (!(value && token)) {
					LogErr ("configuration file (descentw.cfg) looks messed up.\n");
					continue;
					}
				if (value[strlen(value)-1] == '\n')
					value[strlen(value)-1] = 0;
				if (!strcmp(token, pszJoystickMin))	{
					sscanf( value, "%d,%d,%d,%d,%d,%d,%d", &joy_axis_min[0], &joy_axis_min[1], &joy_axis_min[2], &joy_axis_min[3], &joy_axis_min[4], &joy_axis_min[5], &joy_axis_min[6] );
				}
				else if (!strcmp(token, pszJoystickMax))	{
					sscanf( value, "%d,%d,%d,%d,%d,%d,%d", &joy_axis_max[0], &joy_axis_max[1], &joy_axis_max[2], &joy_axis_max[3], &joy_axis_max[4], &joy_axis_max[5], &joy_axis_max[6] );
				}
				else if (!strcmp(token, pszJoystickCen))	{
					sscanf( value, "%d,%d,%d,%d,%d,%d,%d", &joy_axis_center[0], &joy_axis_center[1], &joy_axis_center[2], &joy_axis_center[3], &joy_axis_center[4], &joy_axis_center[5], &joy_axis_center[6] );
				}
			}
		}
		CFClose(infile);
	}
#endif

	return 0;
}

// ----------------------------------------------------------------------------

int WriteConfigFile()
{
	CFILE *infile;
	char str[256];
	int i;
	int joy_axis_min[JOY_MAX_AXES];
	int joy_axis_center[JOY_MAX_AXES];
	int joy_axis_max[JOY_MAX_AXES];
	ubyte gamma = GrGetPaletteGamma();
con_printf (CON_VERBOSE, "writing config file ...\n");
con_printf (CON_VERBOSE, "   getting joystick calibration values ...\n");
	JoyGetCalVals(joy_axis_min, joy_axis_center, joy_axis_max);

	infile = CFOpen("descent.cfg", gameFolders.szConfigDir, "wt", 0);
	if (infile == NULL) {
		return 1;
	}
	/*sprintf (str, "%s=0x%x\n", digi_dev8_str, gameConfig.nDigiType);
	CFPutS(str, infile);
	sprintf (str, "%s=0x%x\n", digi_dev16_str, digi_driver_board_16);
	CFPutS(str, infile);
	sprintf (str, "%s=0x%x\n", digi_port_str, digi_driver_port);
	CFPutS(str, infile);
	sprintf (str, "%s=%d\n", digi_irq_str, digi_driver_irq);
	CFPutS(str, infile);
	sprintf (str, "%s=%d\n", digi_dma8_str, gameConfig.nDigiDMA);
	CFPutS(str, infile);
	sprintf (str, "%s=%d\n", digi_dma16_str, digi_driver_dma_16);
	CFPutS(str, infile);*/
	sprintf (str, "%s=%d\n", pszDigiVolume, gameConfig.nDigiVolume);
	CFPutS(str, infile);
	/*sprintf (str, "%s=0x%x\n", midi_dev_str, gameConfig.nMidiType);
	CFPutS(str, infile);
	sprintf (str, "%s=0x%x\n", midi_port_str, digi_midi_port);
	CFPutS(str, infile);*/
	sprintf (str, "%s=%d\n", pszMidiVolume, gameConfig.nMidiVolume);
	CFPutS(str, infile);
	sprintf (str, "%s=%d\n", pszRedbookEnabled, FindArg("-noredbook")?bRedbookEnabledSave:gameStates.sound.bRedbookEnabled);
	CFPutS(str, infile);
	sprintf (str, "%s=%d\n", pszRedbookVolume, gameConfig.nRedbookVolume);
	CFPutS(str, infile);
	sprintf (str, "%s=%d\n", pszStereoRev, gameConfig.bReverseChannels);
	CFPutS(str, infile);
	sprintf (str, "%s=%d\n", pszGammaLevel, gamma);
	CFPutS(str, infile);
	if (gameStates.app.nDetailLevel == NUM_DETAIL_LEVELS-1)
		sprintf (str, "%s=%d,%d,%d,%d,%d,%d,%d\n", 
					pszDetailLevel, 
					gameStates.app.nDetailLevel,
					gameStates.render.detail.nObjectComplexity,
					gameStates.render.detail.nObjectDetail,
					gameStates.render.detail.nWallDetail,
					gameStates.render.detail.nWallRenderDepth,
					gameStates.render.detail.nDebrisAmount,
					gameStates.sound.nMaxSoundChannels);
	else
		sprintf (str, "%s=%d\n", pszDetailLevel, gameStates.app.nDetailLevel);
	CFPutS(str, infile);

	sprintf (str, "%s=%d,%d,%d,%d\n", pszJoystickMin, joy_axis_min[0], joy_axis_min[1], joy_axis_min[2], joy_axis_min[3] );
	CFPutS(str, infile);
	sprintf (str, "%s=%d,%d,%d,%d\n", pszJoystickCen, joy_axis_center[0], joy_axis_center[1], joy_axis_center[2], joy_axis_center[3] );
	CFPutS(str, infile);
	sprintf (str, "%s=%d,%d,%d,%d\n", pszJoystickMax, joy_axis_max[0], joy_axis_max[1], joy_axis_max[2], joy_axis_max[3] );
	CFPutS(str, infile);

	sprintf (str, "%s=%s\n", pszLastPlayer, LOCALPLAYER.callsign );
	CFPutS(str, infile);
	for (i = 0; gameConfig.szLastMission [i]; i++)
		if (!isprint (gameConfig.szLastMission [i])) {
			*gameConfig.szLastMission = '\0';
			break;
			}
	sprintf (str, "%s=%s\n", pszLastMission, gameConfig.szLastMission );
	CFPutS(str, infile);
	sprintf (str, "%s=%d\n", pszVrType, gameConfig.vrType );
	CFPutS(str, infile);
	sprintf (str, "%s=%d\n", pszVrResolution, gameConfig.vrResolution );
	CFPutS(str, infile);
	sprintf (str, "%s=%d\n", pszVrTracking, gameConfig.vrTracking );
	CFPutS(str, infile);
	sprintf (str, "%s=%d\n", pszHiresMovies, (FindArg("-nohires") || FindArg("-nohighres") || FindArg("-lowresmovies"))?bHiresMoviesSave:gameOpts->movies.bHires);
	CFPutS(str, infile);

	CFClose(infile);

	return 0;
}	

// ----------------------------------------------------------------------------
//eof

