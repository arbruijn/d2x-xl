
/* $Id: endlevel.c, v 1.20 2003/10/12 09:38:48 btb Exp $ */
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

/*
 *
 * Code for rendering external scenes
 *
 * Old Log:
 * Revision 1.5  1995/10/31  10:24:09  allender
 * shareware stuff
 *
 * Revision 1.4  1995/09/14  16:33:54  allender
 * fixed function return values for those that didn't
 * have them...thanks matt!
 *
 * Revision 1.3  1995/07/28  15:36:26  allender
 * reverse inverse sqrt change
 *
 * Revision 1.2  1995/07/28  15:17:40  allender
 * inverse magnitude fixup
 *
 * Revision 1.1  1995/05/16  15:24:32  allender
 * Initial revision
 *
 * Revision 2.2  1995/03/21  14:40:14  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.1  1995/03/20  18:15:50  john
 * Added code to not store the normals in the tSegment structure.
 *
 * Revision 2.0  1995/02/27  11:30:42  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.80  1995/02/22  13:24:45  john
 * Removed the vecmat anonymous unions.
 *
 * Revision 1.79  1995/02/11  12:41:54  john
 * Added new song method, with FM bank switching..
 *
 * Revision 1.78  1995/02/08  11:37:41  mike
 * Check for failures in call to CreateObject.
 *
 * Revision 1.77  1995/02/05  22:09:49  matt
 * Switch out of rear view when starting endlevel sequence
 *
 * Revision 1.76  1995/01/30  18:08:28  rob
 * Add palette fade out before ending level on special missions.
 *
 * Revision 1.75  1995/01/29  16:19:19  rob
 * Fixed endlevel for custom missions.
 *
 * Revision 1.74  1995/01/26  12:18:10  rob
 * Changed calling convention of NetworkDoFrame.
 *
 * Revision 1.73  1995/01/21  16:50:03  matt
 * Made endlevel work with new mission stuff
 *
 * Revision 1.72  1994/12/20  18:22:51  john
 * Added code to support non-looping songs, and put
 * it in for endlevel and credits.
 *
 * Revision 1.71  1994/12/15  12:23:58  matt
 * Added check for failure to create camera tObject
 *
 * Revision 1.70  1994/12/15  03:05:28  matt
 * Added error checking for NULL return from ObjectCreateExplosion ()
 *
 * Revision 1.69  1994/12/12  21:41:38  matt
 * Don't start endlevel if OF_SHOULD_BE_DEAD is set for player
 *
 * Revision 1.68  1994/12/12  15:44:54  rob
 * Rolled back a change to endlevel_start that caused more bugs than
 * it fixed.
 *
 * Revision 1.67  1994/12/12  12:08:33  rob
 * IF a player is dead upon entering the tunnel, make them not dead.  Not perfect solution
 * but avoids some last-minute weirdness we want to fix.  This should be revisited in new
 * versions if possible!
 *
 * Revision 1.66  1994/12/11  22:02:13  allender
 * made endlevel data loading work with .txb encoded format (made with
 * compbit -i level0?.end -o level0?.txb)
 *
 * Revision 1.65  1994/12/11  20:32:47  matt
 * Made camera transition happen 1/3 of the way through exit tunnel
 *
 * Revision 1.64  1994/12/08  20:56:27  john
 * More cfile stuff.
 *
 * Revision 1.63  1994/12/07  17:00:52  rob
 * Trying to fix homing tone warning when in exit tunnel.
 *
 * Revision 1.62  1994/12/06  13:24:47  matt
 * Made exit model come out of bitmaps.tbl
 *
 * Revision 1.61  1994/12/06  12:06:22  matt
 * Fixed/cleaned up satellite (planet/sun) code
 *
 * Revision 1.60  1994/12/05  13:37:12  adam
 * removed slew-mode
 *
 * Revision 1.59  1994/12/05  12:49:37  matt
 * Made satellite a rod (instead of a plane old non-rotating bitmap), and
 * made the size settable in the .end file
 *
 * Revision 1.58  1994/12/04  21:40:00  matt
 * Added explosion sounds
 *
 * Revision 1.57  1994/12/04  18:31:41  matt
 * Wasn't coding planet position, causing it to disappear sometimes
 *
 * Revision 1.56  1994/12/04  14:30:26  john
 * Added hooks for music..
 *
 * Revision 1.55  1994/12/04  13:53:52  matt
 * Added code to make camera off-centered during lookback
 *
 * Revision 1.54  1994/12/04  12:30:18  matt
 * Fixed slew for short sequence
 *
 * Revision 1.53  1994/12/03  19:28:10  matt
 * Added alternate model for exit model after mine destruction
 *
 * Revision 1.52  1994/12/03  00:17:23  matt
 * Made endlevel sequence cut off early
 * Made exit model and bit explosion always plot last (after all terrain)
 *
 * Revision 1.51  1994/12/01  20:15:43  yuan
 * Localization.
 *
 * Revision 1.50  1994/11/30  23:27:35  adam
 * mucked around carelessly
 *
 * Revision 1.49  1994/11/28  21:50:37  mike
 * optimizations.
 *
 * Revision 1.48  1994/11/28  00:12:05  allender
 * took out demo code that was in at one time to record endlevel sequence.
 * We are _not_ recording endlevel sequence
 *
 * Revision 1.47  1994/11/27  23:35:54  allender
 * pause demo recording when starting endlevel sequence.  on demo playback, 
 * don't do endlevel at all.
 *
 * Revision 1.46  1994/11/27  23:13:59  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.45  1994/11/26  23:17:29  matt
 * When camera leaves mine, bank it so it's level with the ground
 *
 * Revision 1.44  1994/11/23  16:52:13  rob
 * Ended netgame endlevel sequence a bit earlier.
 *
 * Revision 1.43  1994/11/22  19:20:46  rob
 * Modem support for secret levels.
 *
 * Revision 1.42  1994/11/22  12:11:03  rob
 * Fixed bug - file handle left open in LoadEndLevelData.
 *
 * Revision 1.41  1994/11/21  17:29:22  matt
 * Cleaned up sequencing & game saving for secret levels
 *
 * Revision 1.40  1994/11/19  15:14:54  mike
 * remove unused code and data
 *
 * Revision 1.39  1994/11/19  12:41:32  matt
 * Added system to read endlevel data from file, and to make it work
 * with any exit tunnel.
 *
 * Revision 1.38  1994/11/17  15:02:24  mike
 * support new tSegment validation functions.
 *
 * Revision 1.37  1994/11/17  13:04:45  allender
 * backout out newdemo changes
 *
 * Revision 1.35  1994/11/16  14:52:33  rob
 * Commented out SLEW_ON on Matt's direction.
 * Changed something to fix demo recording.
 *
 * Revision 1.34  1994/11/16  11:49:29  matt
 * Added code to rotate terrain to match mine
 *
 * Revision 1.33  1994/11/14  17:54:54  allender
 * on exit sequence during demo recording, force player exited from mine
 * packet to all other network players
 *
 * Revision 1.32  1994/11/10  21:27:42  matt
 * Took out //printf's
 *
 * Revision 1.31  1994/11/10  14:02:24  matt
 * Hacked in support for player ships with different textures
 *
 * Revision 1.30  1994/11/09  10:31:33  matt
 * Don't create explosions if can't find seg to create them in
 *
 * Revision 1.29  1994/11/05  17:22:37  john
 * Fixed lots of sequencing problems with newdemo stuff.
 *
 * Revision 1.28  1994/11/03  11:10:39  matt
 * Fixed chase angles code
 * Maybe other things, too.
 *
 * Revision 1.27  1994/10/30  20:09:21  matt
 * For endlevel: added big explosion at tunnel exit; made lights in tunnel
 * go out; made more explosions on walls.
 *
 * Revision 1.26  1994/10/28  16:37:50  allender
 * stop demo recording when endlevel sequence activated
 *
 * Revision 1.25  1994/10/27  21:15:21  matt
 * Added explosions in mine chasing player
 *
 * Revision 1.24  1994/10/27  01:03:57  matt
 * Fixed several small bugs in flythrough
 *
 * Revision 1.23  1994/10/22  01:32:30  matt
 * Don't start endlevel sequence if player dead
 *
 * Revision 1.22  1994/10/22  00:08:06  matt
 * Fixed up problems with bonus & game sequencing
 * Player doesn't get credit for hostages unless he gets them out alive
 *
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: endlevel.c, v 1.20 2003/10/12 09:38:48 btb Exp $";
#endif

//#define SLEW_ON 1

//#define _MARK_ON

#include <stdlib.h>


#include <stdio.h>
#include <string.h>
#include <cType.h> // for isspace

#include "inferno.h"
#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "3d.h"
#include "error.h"
#include "palette.h"
#include "iff.h"
#include "mono.h"
#include "texmap.h"
#include "fvi.h"
#include "u_mem.h"
#include "sounds.h"

#include "endlevel.h"
#include "object.h"
#include "game.h"
#include "screens.h"
#include "gauges.h"
#include "wall.h"
#include "terrain.h"
#include "polyobj.h"
#include "bm.h"
#include "gameseq.h"
#include "newdemo.h"
#include "multi.h"
#include "vclip.h"
#include "fireball.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "text.h"
#include "digi.h"
#include "cfile.h"
#include "compbit.h"
#include "songs.h"
#include "movie.h"
#include "render.h"
#include "gameseg.h"

//------------------------------------------------------------------------------

typedef struct flythrough_data {
	tObject		*objP;
	vmsAngVec	angles;			//orientation in angles
	vmsVector	step;				//how far in a second
	vmsVector	angstep;			//rotation per second
	fix			speed;			//how fast tObject is moving
	vmsVector 	headvec;			//where we want to be pointing
	int			firstTime;		//flag for if first time through
	fix			offset_frac;	//how far off-center as portion of way
	fix			offsetDist;	//how far currently off-center
} flythrough_data;

//endlevel sequence states

#define EL_OFF				0		//not in endlevel
#define EL_FLYTHROUGH	1		//auto-flythrough in tunnel
#define EL_LOOKBACK		2		//looking back at player
#define EL_OUTSIDE		3		//flying outside for a while
#define EL_STOPPED		4		//stopped, watching explosion
#define EL_PANNING		5		//panning around, watching player
#define EL_CHASING		6		//chasing player to station

#define SHORT_SEQUENCE	1		//if defined, end sequnce when panning starts
//#define STATION_ENABLED	1		//if defined, load & use space station model

#define FLY_SPEED i2f (50)

void DoEndLevelFlyThrough (int n);
void DrawStars ();
int find_exit_side (tObject *objP);
void generate_starfield ();
void StartEndLevelFlyThrough (int n, tObject *objP, fix speed);
void StartRenderedEndLevelSequence ();

#ifdef D2_OEM
char movie_table [2][] = {
	{'a', 'a', 'a', 'a', 'd', 'd', 'd', 'd'}, 
	{'f', 'f', 'f', 'a', 'a', 'b', 'b', 'c'}
};
#else
char movie_table [2][30] = {	
	{'a', 'b', 'c', 
#ifndef SHAREWARE
	'a', 
	'd', 'f', 'd', 'f', 
	'g', 'h', 'i', 'g', 
	'j', 'k', 'l', 'j', 
	'm', 'o', 'm', 'o', 
	'p', 'q', 'p', 'q', 
	0, 0, 0, 0, 0, 0
#endif
	}, 
	{'f', 'f', 'f', 'a', 'a', 'b', 'b', 'c', 'c', 
	 'c', 'g', 'f', 'g', 'g', 'f', 'g', 'f', 'g', 
	 'f', 'f', 'f', 'g', 'f', 'g', 'd', 'd', 'f', 
	 'e', 'e', 'e'
	}
};
#endif

#define N_MOVIES (sizeof (movie_table) / (2 * sizeof (*movie_table)))

#ifndef SHAREWARE
#ifdef D2_OEM
char movie_table_secret [2][2] = {{'a', 'd'}, {'e', 'e'}};
#else
char movie_table_secret [2][6] = {{'a', 'd', 'g', 'j', 'm', 'p'}, {'e', 'e', 'e', 'e', 'e', 'e'}};
#endif
#define N_MOVIES_SECRET (sizeof (movie_table_secret) / (2 * sizeof (*movie_table_secret)))
#else
#define N_MOVIES_SECRET 0
#endif


#define FLY_ACCEL i2f (5)

extern int ELFindConnectedSide (int seg0, int seg1);

//------------------------------------------------------------------------------

void InitEndLevelData (void)
{
gameData.endLevel.station.vPos.x = 0xf8c4 << 10;
gameData.endLevel.station.vPos.y = 0x3c1c << 12;
gameData.endLevel.station.vPos.z = 0x0372 << 10;
}

//------------------------------------------------------------------------------
//find delta between two angles
inline fixang DeltaAng (fixang a, fixang b)
{
fixang delta0, delta1;

return (abs (delta0 = a - b) < abs (delta1 = b - a)) ? delta0 : delta1;
}

//------------------------------------------------------------------------------
//return though which tSide of seg0 is seg1
int ELFindConnectedSide (int seg0, int seg1)
{
	tSegment *Seg=&gameData.segs.segments [seg0];
	int i;

	for (i=MAX_SIDES_PER_SEGMENT;i--;) if (Seg->children [i]==seg1) return i;

	return -1;
}

//------------------------------------------------------------------------------
extern int Kmatrix_nomovie_message;

#if defined (D2_OEM) || defined (COMPILATION)
#define MOVIE_REQUIRED 0
#else
#define MOVIE_REQUIRED 1
#endif

//returns movie played status.  see movie.h
int StartEndLevelMovie ()
{
	char movie_name [SHORT_FILENAME_LEN];
	int r;

strcpy (movie_name, gameStates.app.bD1Mission ? "exita.mve" : "esa.mve");
if (!IS_D2_OEM)
	if (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel)
		return 1;   //don't play movie
if (gameData.missions.nCurrentLevel > 0)
	if (gameStates.app.bD1Mission)
		movie_name [4] = movie_table [1][gameData.missions.nCurrentLevel-1];
	else
		movie_name [2] = movie_table [0][gameData.missions.nCurrentLevel-1];
else if (gameStates.app.bD1Mission) {
	movie_name [4] = movie_table [1][26 - gameData.missions.nCurrentLevel];
	}
else {
#ifndef SHAREWARE
	return 0;       //no escapes for secret level
#else
	Error ("Invalid level number <%d>", gameData.missions.nCurrentLevel);
#endif
}
#ifndef SHAREWARE
r = PlayMovie (movie_name, (gameData.app.nGameMode & GM_MULTI) ? 0 : MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
#else
return 0;	// movie not played for shareware
#endif
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	SetScreenMode (SCREEN_GAME);
#ifdef NETWORK
if (r==MOVIE_NOT_PLAYED && (gameData.app.nGameMode & GM_MULTI))
	Kmatrix_nomovie_message=1;
else
 	Kmatrix_nomovie_message=0;
#endif
return (r);
}

//------------------------------------------------------------------------------

void _CDECL_ FreeEndLevelData (void)
{
LogErr ("unloading endlevel data\n");
if (gameData.endLevel.terrain.bmInstance.bm_texBuf) {
	OglFreeBmTexture (&gameData.endLevel.terrain.bmInstance);
	d_free (gameData.endLevel.terrain.bmInstance.bm_texBuf);
	}
if (gameData.endLevel.satellite.bmInstance.bm_texBuf) {
	OglFreeBmTexture (&gameData.endLevel.satellite.bmInstance);
	d_free (gameData.endLevel.satellite.bmInstance.bm_texBuf);
	}
}

//-----------------------------------------------------------------------------

void InitEndLevel ()
{
	//##gameData.endLevel.satellite.bmP = bm_load ("earth.bbm");
	//##gameData.endLevel.terrain.bmP = bm_load ("moon.bbm");
	//##
	//##LoadTerrain ("matt5b.bbm");		//load bitmap as height array
	//##//LoadTerrain ("ttest2.bbm");		//load bitmap as height array

#ifdef STATION_ENABLED
	gameData.endLevel.station.bmP = bm_load ("steel3.bbm");
	gameData.endLevel.station.bmList [0] = &gameData.endLevel.station.bmP;

	gameData.endLevel.station.nModel = LoadPolygonModel ("station.pof", 1, gameData.endLevel.station.bmList, NULL);
#endif

//!!	exit_bitmap = bm_load ("steel1.bbm");
//!!	exit_bitmap_list [0] = &exit_bitmap;

//!!	gameData.endLevel.exit.nModel = LoadPolygonModel ("exit01.pof", 1, exit_bitmap_list, NULL);
//!!	gameData.endLevel.exit.nDestroyedModel = LoadPolygonModel ("exit01d.pof", 1, exit_bitmap_list, NULL);

generate_starfield ();
atexit (FreeEndLevelData);
gameData.endLevel.terrain.bmInstance.bm_texBuf = 
gameData.endLevel.satellite.bmInstance.bm_texBuf = NULL;
}

//------------------------------------------------------------------------------

tObject external_explosion;

vmsAngVec exit_angles={-0xa00, 0, 0};

vmsMatrix surface_orient;

extern char szLastPaletteLoaded [];

void StartEndLevelSequence (int bSecret)
{
	int	i;
	int nMoviePlayed = MOVIE_NOT_PLAYED;

if (gameData.demo.nState == ND_STATE_RECORDING)		// stop demo recording
	gameData.demo.nState = ND_STATE_PAUSED;
if (gameData.demo.nState == ND_STATE_PLAYBACK) {		// don't do this if in playback mode
	if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) ||
			((gameData.missions.nCurrentMission == gameData.missions.nD1BuiltinMission) && 
			gameStates.app.bHaveExtraMovies))
		StartEndLevelMovie ();
	strcpy (szLastPaletteLoaded, "");		//force palette load next time
	return;
	}
if (gameStates.app.bPlayerIsDead || gameData.objs.console->flags&OF_SHOULD_BE_DEAD)
	return;				//don't start if dead!
//	Dematerialize Buddy!
for (i = 0; i <= gameData.objs.nLastObject; i++)
	if (gameData.objs.objects [i].nType == OBJ_ROBOT)
		if (gameData.bots.pInfo [gameData.objs.objects [i].id].companion) {
			ObjectCreateExplosion (gameData.objs.objects [i].nSegment, &gameData.objs.objects [i].pos, F1_0*7/2, VCLIP_POWERUP_DISAPPEARANCE);
			gameData.objs.objects [i].flags |= OF_SHOULD_BE_DEAD;
		}
gameData.multi.players [gameData.multi.nLocalPlayer].homingObjectDist = -F1_0; // Turn off homing sound.
ResetRearView ();		//turn off rear view if set
#ifdef NETWORK
if (gameData.app.nGameMode & GM_MULTI) {
	MultiSendEndLevelStart (0);
	NetworkDoFrame (1, 1);
	}
#endif
if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) ||
		((gameData.missions.nCurrentMission == gameData.missions.nD1BuiltinMission) && 
		gameStates.app.bHaveExtraMovies)) {
	// only play movie for built-in mission
	if (!(gameData.app.nGameMode & GM_MULTI))
		nMoviePlayed = StartEndLevelMovie ();
	}
if ((nMoviePlayed == MOVIE_NOT_PLAYED) && gameStates.app.bEndLevelDataLoaded) {   //don't have movie.  Do rendered sequence, if available
	int bExitModelsLoaded = (gameData.pig.tex.nHamFileVersion < 3) ? 1 : LoadExitModels ();
#ifndef WINDOWS
	if (bExitModelsLoaded) {
		StartRenderedEndLevelSequence ();
		return;
		}
#endif
	}
//don't have movie or rendered sequence, fade out
GrPaletteFadeOut (NULL, 32, 0);
if (!bSecret)
	PlayerFinishedLevel (0);		//done with level
}

//------------------------------------------------------------------------------

void StartRenderedEndLevelSequence ()
{
	int last_segnum, exit_side, tunnel_length;
	int nSegment, old_segnum, entry_side, i;

//count segments in exit tunnel
old_segnum = gameData.objs.console->nSegment;
exit_side = find_exit_side (gameData.objs.console);
nSegment = gameData.segs.segments [old_segnum].children [exit_side];
tunnel_length = 0;
do {
	entry_side = ELFindConnectedSide (nSegment, old_segnum);
	exit_side = sideOpposite [entry_side];
	old_segnum = nSegment;
	nSegment = gameData.segs.segments [nSegment].children [exit_side];
	tunnel_length++;
	} while (nSegment >= 0);
if (nSegment != -2) {
	PlayerFinishedLevel (0);		//don't do special sequence
	return;
	}
last_segnum = old_segnum;
//now pick transition nSegment 1/3 of the way in
old_segnum = gameData.objs.console->nSegment;
exit_side = find_exit_side (gameData.objs.console);
nSegment = gameData.segs.segments [old_segnum].children [exit_side];
i=tunnel_length/3;
while (i--) {
	entry_side = ELFindConnectedSide (nSegment, old_segnum);
	exit_side = sideOpposite [entry_side];
	old_segnum = nSegment;
	nSegment = gameData.segs.segments [nSegment].children [exit_side];
	}
gameData.endLevel.exit.nTransitSegNum = nSegment;
gameStates.render.cockpit.nModeSave = gameStates.render.cockpit.nMode;
#ifdef NETWORK
if (gameData.app.nGameMode & GM_MULTI) {
	MultiSendEndLevelStart (0);
	NetworkDoFrame (1, 1);
	}
#endif
Assert (last_segnum == gameData.endLevel.exit.nSegNum);
if (gameData.missions.list [gameData.missions.nCurrentMission].descent_version == 1)
	SongsPlaySong (SONG_ENDLEVEL, 0);
gameStates.app.bEndLevelSequence = EL_FLYTHROUGH;
gameData.objs.console->movementType = MT_NONE;			//movement handled by flythrough
gameData.objs.console->controlType = CT_NONE;
gameStates.app.bGameSuspended |= SUSP_ROBOTS;          //robots don't move
gameData.endLevel.xCurFlightSpeed = gameData.endLevel.xDesiredFlightSpeed = FLY_SPEED;
StartEndLevelFlyThrough (0, gameData.objs.console, gameData.endLevel.xCurFlightSpeed);		//initialize
HUDInitMessage (TXT_EXIT_SEQUENCE);
gameStates.render.bOutsideMine = gameStates.render.bExtExplPlaying = 0;
gameStates.render.nFlashScale = f1_0;
gameStates.gameplay.bMineDestroyed = 0;
}

//------------------------------------------------------------------------------

extern flythrough_data flyObjects [];

extern tObject *slew_obj;

vmsAngVec player_angles, player_dest_angles;
vmsAngVec camera_desired_angles, camera_cur_angles;

#define CHASE_TURN_RATE (0x4000/4)		//max turn per second

//returns bitmask of which angles are at dest. bits 0, 1, 2 = p, b, h
int ChaseAngles (vmsAngVec *cur_angles, vmsAngVec *desired_angles)
{
	vmsAngVec	delta_angs, alt_angles, alt_delta_angs;
	fix			total_delta, altTotal_delta;
	fix			frame_turn;
	int			mask = 0;

delta_angs.p = desired_angles->p - cur_angles->p;
delta_angs.h = desired_angles->h - cur_angles->h;
delta_angs.b = desired_angles->b - cur_angles->b;
total_delta = abs (delta_angs.p) + abs (delta_angs.b) + abs (delta_angs.h);

alt_angles.p = f1_0/2 - cur_angles->p;
alt_angles.b = cur_angles->b + f1_0/2;
alt_angles.h = cur_angles->h + f1_0/2;

alt_delta_angs.p = desired_angles->p - alt_angles.p;
alt_delta_angs.h = desired_angles->h - alt_angles.h;
alt_delta_angs.b = desired_angles->b - alt_angles.b;
//alt_delta_angs.b = 0;

altTotal_delta = abs (alt_delta_angs.p) + abs (alt_delta_angs.b) + abs (alt_delta_angs.h);

////printf ("Total delta = %x, alt total_delta = %x\n", total_delta, altTotal_delta);

if (altTotal_delta < total_delta) {
	////printf ("FLIPPING ANGLES!\n");
	*cur_angles = alt_angles;
	delta_angs = alt_delta_angs;
}

frame_turn = FixMul (gameData.time.xFrame, CHASE_TURN_RATE);

if (abs (delta_angs.p) < frame_turn) {
	cur_angles->p = desired_angles->p;
	mask |= 1;
	}
else
	if (delta_angs.p > 0)
		cur_angles->p += frame_turn;
	else
		cur_angles->p -= frame_turn;

if (abs (delta_angs.b) < frame_turn) {
	cur_angles->b = desired_angles->b;
	mask |= 2;
	}
else
	if (delta_angs.b > 0)
		cur_angles->b += frame_turn;
	else
		cur_angles->b -= frame_turn;
if (abs (delta_angs.h) < frame_turn) {
	cur_angles->h = desired_angles->h;
	mask |= 4;
	}
else
	if (delta_angs.h > 0)
		cur_angles->h += frame_turn;
	else
		cur_angles->h -= frame_turn;
return mask;
}

//------------------------------------------------------------------------------

void StopEndLevelSequence ()
{
	gameStates.render.nInterpolationMethod = 0;

GrPaletteFadeOut (NULL, 32, 0);
SelectCockpit (gameStates.render.cockpit.nModeSave);
gameStates.app.bEndLevelSequence = EL_OFF;
PlayerFinishedLevel (0);
}

//------------------------------------------------------------------------------

#define VCLIP_BIG_PLAYER_EXPLOSION	58

//--unused-- vmsVector upvec = {0, f1_0, 0};

//find the angle between the player's heading & the station
inline void GetAnglesToObject (vmsAngVec *av, vmsVector *targ_pos, vmsVector *cur_pos)
{
	vmsVector tv;

VmVecSub (&tv, targ_pos, cur_pos);
VmExtractAnglesVector (av, &tv);
}

//------------------------------------------------------------------------------

void DoEndLevelFrame ()
{
	static fix timer;
	static fix bank_rate;
	vmsVector save_last_pos;
	static fix explosion_wait1=0;
	static fix explosion_wait2=0;
	static fix ext_expl_halflife;

save_last_pos = gameData.objs.console->last_pos;	//don't let move code change this
MoveAllObjects ();
gameData.objs.console->last_pos = save_last_pos;

if (gameStates.render.bExtExplPlaying) {
	external_explosion.lifeleft -= gameData.time.xFrame;
	DoExplosionSequence (&external_explosion);
	if (external_explosion.lifeleft < ext_expl_halflife)
		gameStates.gameplay.bMineDestroyed = 1;
	if (external_explosion.flags & OF_SHOULD_BE_DEAD)
		gameStates.render.bExtExplPlaying = 0;
	}

if (gameData.endLevel.xCurFlightSpeed != gameData.endLevel.xDesiredFlightSpeed) {
	fix delta = gameData.endLevel.xDesiredFlightSpeed - gameData.endLevel.xCurFlightSpeed;
	fix frame_accel = FixMul (gameData.time.xFrame, FLY_ACCEL);

	if (abs (delta) < frame_accel)
		gameData.endLevel.xCurFlightSpeed = gameData.endLevel.xDesiredFlightSpeed;
	else
		if (delta > 0)
			gameData.endLevel.xCurFlightSpeed += frame_accel;
		else
			gameData.endLevel.xCurFlightSpeed -= frame_accel;
}

//do big explosions
if (!gameStates.render.bOutsideMine) {
	if (gameStates.app.bEndLevelSequence==EL_OUTSIDE) {
		vmsVector tvec;

		VmVecSub (&tvec, &gameData.objs.console->pos, &gameData.endLevel.exit.vSideExit);
		if (VmVecDot (&tvec, &gameData.endLevel.exit.mOrient.fvec) > 0) {
			tObject *tObjP;
			gameStates.render.bOutsideMine = 1;
			tObjP = ObjectCreateExplosion (gameData.endLevel.exit.nSegNum, &gameData.endLevel.exit.vSideExit, i2f (50), VCLIP_BIG_PLAYER_EXPLOSION);
			if (tObjP) {
				external_explosion = *tObjP;
				tObjP->flags |= OF_SHOULD_BE_DEAD;
				gameStates.render.nFlashScale = 0;	//kill lights in mine
				ext_expl_halflife = tObjP->lifeleft;
				gameStates.render.bExtExplPlaying = 1;
				}
			DigiLinkSoundToPos (SOUND_BIG_ENDLEVEL_EXPLOSION, gameData.endLevel.exit.nSegNum, 0, &gameData.endLevel.exit.vSideExit, 0, i2f (3)/4);
			}
		}

	//do explosions chasing player
	if ((explosion_wait1-=gameData.time.xFrame) < 0) {
		vmsVector	tpnt;
		short			nSegment;
		tObject		*expl;
		static int	sound_count;

		VmVecScaleAdd (&tpnt, &gameData.objs.console->pos, &gameData.objs.console->orient.fvec, -gameData.objs.console->size*5);
		VmVecScaleInc (&tpnt, &gameData.objs.console->orient.rvec, (d_rand ()-RAND_MAX/2)*15);
		VmVecScaleInc (&tpnt, &gameData.objs.console->orient.uvec, (d_rand ()-RAND_MAX/2)*15);
		nSegment = FindSegByPoint (&tpnt, gameData.objs.console->nSegment);
		if (nSegment != -1) {
			expl = ObjectCreateExplosion (nSegment, &tpnt, i2f (20), VCLIP_BIG_PLAYER_EXPLOSION);
			if (d_rand ()<10000 || ++sound_count==7) {		//pseudo-random
				DigiLinkSoundToPos (SOUND_TUNNEL_EXPLOSION, nSegment, 0, &tpnt, 0, F1_0);
				sound_count=0;
				}
			}
		explosion_wait1 = 0x2000 + d_rand ()/4;
		}
	}

//do little explosions on walls
if (gameStates.app.bEndLevelSequence >= EL_FLYTHROUGH && gameStates.app.bEndLevelSequence < EL_OUTSIDE)
	if ((explosion_wait2-=gameData.time.xFrame) < 0) {
		vmsVector tpnt;
		fvi_query fq;
		fvi_info hit_data;
		//create little explosion on wall
		VmVecCopyScale (&tpnt, &gameData.objs.console->orient.rvec, (d_rand ()-RAND_MAX/2)*100);
		VmVecScaleInc (&tpnt, &gameData.objs.console->orient.uvec, (d_rand ()-RAND_MAX/2)*100);
		VmVecInc (&tpnt, &gameData.objs.console->pos);
		if (gameStates.app.bEndLevelSequence == EL_FLYTHROUGH)
			VmVecScaleInc (&tpnt, &gameData.objs.console->orient.fvec, d_rand ()*200);
		else
			VmVecScaleInc (&tpnt, &gameData.objs.console->orient.fvec, d_rand ()*60);
		//find hit point on wall
		fq.p0					= &gameData.objs.console->pos;
		fq.p1					= &tpnt;
		fq.startSeg			= gameData.objs.console->nSegment;
		fq.rad				= 0;
		fq.thisObjNum		= 0;
		fq.ignoreObjList	= NULL;
		fq.flags				= 0;
		FindVectorIntersection (&fq, &hit_data);
		if ((hit_data.hit.nType == HIT_WALL) && (hit_data.hit.nSegment != -1))
			ObjectCreateExplosion ((short) hit_data.hit.nSegment, &hit_data.hit.vPoint, i2f (3)+d_rand ()*6, VCLIP_SMALL_EXPLOSION);
		explosion_wait2 = (0xa00 + d_rand ()/8)/2;
		}

switch (gameStates.app.bEndLevelSequence) {
	case EL_OFF:
		return;

	case EL_FLYTHROUGH: {
		DoEndLevelFlyThrough (0);
		if (gameData.objs.console->nSegment == gameData.endLevel.exit.nTransitSegNum) {
			if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) &&
					(StartEndLevelMovie () != MOVIE_NOT_PLAYED))
				StopEndLevelSequence ();
			else {
				int nObject;
				gameStates.app.bEndLevelSequence = EL_LOOKBACK;
				nObject = CreateObject (OBJ_CAMERA, 0, -1, 
												gameData.objs.console->nSegment, 
												&gameData.objs.console->pos, 
												&gameData.objs.console->orient, 0, 
												CT_NONE, MT_NONE, RT_NONE, 1);
				if (nObject == -1) { //can't get tObject, so abort
#if TRACE
					con_printf (1, "Can't get tObject for endlevel sequence.  Aborting endlevel sequence.\n");
#endif
					StopEndLevelSequence ();
					return;
				}
				gameData.objs.viewer = gameData.objs.endLevelCamera = gameData.objs.objects + nObject;
				SelectCockpit (CM_LETTERBOX);
				gameOpts->render.cockpit.bHUD = 0;	//will be restored by reading plr file when loading next level
				flyObjects [1] = flyObjects [0];
				flyObjects [1].objP = gameData.objs.endLevelCamera;
				flyObjects [1].speed = (5*gameData.endLevel.xCurFlightSpeed)/4;
				flyObjects [1].offset_frac = 0x4000;
				VmVecScaleInc (&gameData.objs.endLevelCamera->pos, &gameData.objs.endLevelCamera->orient.fvec, i2f (7));
				timer=0x20000;
				}
			}
		break;
		}


	case EL_LOOKBACK: {
		DoEndLevelFlyThrough (0);
		DoEndLevelFlyThrough (1);
		if (timer>0) {
			timer -= gameData.time.xFrame;
			if (timer < 0)		//reduce speed
				flyObjects [1].speed = flyObjects [0].speed;
			}
		if (gameData.objs.endLevelCamera->nSegment == gameData.endLevel.exit.nSegNum) {
			vmsAngVec cam_angles, exit_seg_angles;
			gameStates.app.bEndLevelSequence = EL_OUTSIDE;
			timer = i2f (2);
			VmVecNegate (&gameData.objs.endLevelCamera->orient.fvec);
			VmVecNegate (&gameData.objs.endLevelCamera->orient.rvec);
			VmExtractAnglesMatrix (&cam_angles, &gameData.objs.endLevelCamera->orient);
			VmExtractAnglesMatrix (&exit_seg_angles, &gameData.endLevel.exit.mOrient);
			bank_rate = (-exit_seg_angles.b - cam_angles.b)/2;
			gameData.objs.console->controlType = gameData.objs.endLevelCamera->controlType = CT_NONE;
#ifdef SLEW_ON
slew_obj = gameData.objs.endLevelCamera;
#endif
			}
		break;
		}

	case EL_OUTSIDE: {
#ifndef SLEW_ON
		vmsAngVec cam_angles;
#endif
		VmVecScaleInc (&gameData.objs.console->pos, &gameData.objs.console->orient.fvec, FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed));
#ifndef SLEW_ON
		VmVecScaleInc (&gameData.objs.endLevelCamera->pos, &gameData.objs.endLevelCamera->orient.fvec, FixMul (gameData.time.xFrame, -2*gameData.endLevel.xCurFlightSpeed));
		VmVecScaleInc (&gameData.objs.endLevelCamera->pos, &gameData.objs.endLevelCamera->orient.uvec, FixMul (gameData.time.xFrame, -gameData.endLevel.xCurFlightSpeed/10));
		VmExtractAnglesMatrix (&cam_angles, &gameData.objs.endLevelCamera->orient);
		cam_angles.b += FixMul (bank_rate, gameData.time.xFrame);
		VmAngles2Matrix (&gameData.objs.endLevelCamera->orient, &cam_angles);
#endif
		timer -= gameData.time.xFrame;
		if (timer < 0) {
			gameStates.app.bEndLevelSequence = EL_STOPPED;
			VmExtractAnglesMatrix (&player_angles, &gameData.objs.console->orient);
			timer = i2f (3);
			}
		break;
		}

	case EL_STOPPED: {
		GetAnglesToObject (&player_dest_angles, &gameData.endLevel.station.vPos, &gameData.objs.console->pos);
		ChaseAngles (&player_angles, &player_dest_angles);
		VmAngles2Matrix (&gameData.objs.console->orient, &player_angles);
		VmVecScaleInc (&gameData.objs.console->pos, &gameData.objs.console->orient.fvec, FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed));
		timer -= gameData.time.xFrame;
		if (timer < 0) {
#ifdef SLEW_ON
			slew_obj = gameData.objs.endLevelCamera;
			DoSlewMovement (gameData.objs.endLevelCamera, 1, 1);
			timer += gameData.time.xFrame;		//make time stop
			break;
#else
#	ifdef SHORT_SEQUENCE
			StopEndLevelSequence ();
#	else
			gameStates.app.bEndLevelSequence = EL_PANNING;
			VmExtractAnglesMatrix (&camera_cur_angles, &gameData.objs.endLevelCamera->orient);
			timer = i2f (3);
			if (gameData.app.nGameMode & GM_MULTI) { // try to skip part of the seq if multiplayer
				StopEndLevelSequence ();
				return;
				}
#	endif		//SHORT_SEQUENCE
#endif		//SLEW_ON
			}
		break;
		}
	#ifndef SHORT_SEQUENCE
	case EL_PANNING: {
#ifndef SLEW_ON
		int mask;
#endif
		GetAnglesToObject (&player_dest_angles, &gameData.endLevel.station.vPos, &gameData.objs.console->pos);
		ChaseAngles (&player_angles, &player_dest_angles);
		VmAngles2Matrix (&gameData.objs.console->orient, &player_angles);
		VmVecScaleInc (&gameData.objs.console->pos, &gameData.objs.console->orient.fvec, FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed);

#ifdef SLEW_ON
		DoSlewMovement (gameData.objs.endLevelCamera, 1, 1);
#else
		GetAnglesToObject (&camera_desired_angles, &gameData.objs.console->pos, &gameData.objs.endLevelCamera->pos);
		mask = ChaseAngles (&camera_cur_angles, &camera_desired_angles);
		VmAngles2Matrix (&gameData.objs.endLevelCamera->orient, &camera_cur_angles);
		if ((mask&5) == 5) {
			vmsVector tvec;
			gameStates.app.bEndLevelSequence = EL_CHASING;
			VmVecNormalizedDirQuick (&tvec, &gameData.endLevel.station.vPos, &gameData.objs.console->pos);
			VmVector2Matrix (&gameData.objs.console->orient, &tvec, &surface_orient.uvec, NULL);
			gameData.endLevel.xDesiredFlightSpeed *= 2;
			}
#endif
		break;
	}

	case EL_CHASING: 
		{
		fix d, speed_scale;

#ifdef SLEW_ON
		DoSlewMovement (gameData.objs.endLevelCamera, 1, 1);
#endif
		GetAnglesToObject (&camera_desired_angles, &gameData.objs.console->pos, &gameData.objs.endLevelCamera->pos);
		ChaseAngles (&camera_cur_angles, &camera_desired_angles);
#ifndef SLEW_ON
		VmAngles2Matrix (&gameData.objs.endLevelCamera->orient, &camera_cur_angles);
#endif
		d = VmVecDistQuick (&gameData.objs.console->pos, &gameData.objs.endLevelCamera->pos);
		speed_scale = FixDiv (d, i2f (0x20);
		if (d<f1_0) d=f1_0;
		GetAnglesToObject (&player_dest_angles, &gameData.endLevel.station.vPos, &gameData.objs.console->pos);
		ChaseAngles (&player_angles, &player_dest_angles);
		VmAngles2Matrix (&gameData.objs.console->orient, &player_angles);
		VmVecScaleInc (&gameData.objs.console->pos, &gameData.objs.console->orient.fvec, FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed);
#ifndef SLEW_ON
		VmVecScaleInc (&gameData.objs.endLevelCamera->pos, &gameData.objs.endLevelCamera->orient.fvec, FixMul (gameData.time.xFrame, FixMul (speed_scale, gameData.endLevel.xCurFlightSpeed));
		if (VmVecDist (&gameData.objs.console->pos, &gameData.endLevel.station.vPos) < i2f (10))
			StopEndLevelSequence ();
#endif
		break;
		}
#endif		//ifdef SHORT_SEQUENCE
	}
}

//------------------------------------------------------------------------------

#define MIN_D 0x100

//find which tSide to fly out of
int find_exit_side (tObject *objP)
{
	int i;
	vmsVector prefvec, segcenter, sidevec;
	fix best_val=-f2_0;
	int best_side;
	tSegment *pseg = &gameData.segs.segments [objP->nSegment];

	//find exit tSide

	VmVecNormalizedDirQuick (&prefvec, &objP->pos, &objP->last_pos);

	COMPUTE_SEGMENT_CENTER (&segcenter, pseg);

	best_side=-1;
	for (i=MAX_SIDES_PER_SEGMENT;--i >= 0;) {
		fix d;

		if (pseg->children [i]!=-1) {

			COMPUTE_SIDE_CENTER (&sidevec, pseg, i);
			VmVecNormalizedDirQuick (&sidevec, &sidevec, &segcenter);
			d = VmVecDotProd (&sidevec, &prefvec);

			if (labs (d) < MIN_D) d=0;

			if (d > best_val) {best_val=d; best_side=i;}

		}
	}

	Assert (best_side!=-1);

	return best_side;
}

//------------------------------------------------------------------------------

extern fix nRenderZoom;							//the player's zoom factor

extern vmsVector viewerEye;	//valid during render

void DrawExitModel ()
{
	vmsVector model_pos;
	int f=15, u=0;	//21;

VmVecScaleAdd (&model_pos, &gameData.endLevel.exit.vMineExit, &gameData.endLevel.exit.mOrient.fvec, i2f (f));
VmVecScaleInc (&model_pos, &gameData.endLevel.exit.mOrient.uvec, i2f (u));
gameStates.app.bD1Model = gameStates.app.bD1Mission && gameStates.app.bD1Data;
DrawPolygonModel (NULL, &model_pos, &gameData.endLevel.exit.mOrient, NULL, 
						 (gameStates.gameplay.bMineDestroyed) ? gameData.endLevel.exit.nDestroyedModel : gameData.endLevel.exit.nModel, 
						 0, f1_0, NULL, NULL, NULL);
gameStates.app.bD1Model = 0;
}

//------------------------------------------------------------------------------

int exit_point_bmx, exit_point_bmy;

fix satellite_size = i2f (400);

#define SATELLITE_DIST		i2f (1024)
#define SATELLITE_WIDTH		satellite_size
#define SATELLITE_HEIGHT	 ((satellite_size*9)/4)		// ((satellite_size*5)/2)

void RenderExternalScene (fix eye_offset)
{
	vmsVector delta;
	g3s_point p, top_pnt;

viewerEye = gameData.objs.viewer->pos;
if (eye_offset)
	VmVecScaleInc (&viewerEye, &gameData.objs.viewer->orient.rvec, eye_offset);
G3SetViewMatrix (&gameData.objs.viewer->pos, &gameData.objs.viewer->orient, nRenderZoom);
GrClearCanvas (BLACK_RGBA);
G3StartInstanceMatrix (&vmdZeroVector, &surface_orient);
DrawStars ();
G3DoneInstance ();
//draw satellite
G3TransformAndEncodePoint (&p, &gameData.endLevel.satellite.vPos);
G3RotateDeltaVec (&delta, &gameData.endLevel.satellite.vUp);
G3AddDeltaVec (&top_pnt, &p, &delta);
if (!(p.p3_codes & CC_BEHIND)) {
	int save_im = gameStates.render.nInterpolationMethod;
	if (!(p.p3Flags & PF_OVERFLOW)) {
		gameStates.render.nInterpolationMethod = 0;
		G3DrawRodTexPoly (gameData.endLevel.satellite.bmP, &p, SATELLITE_WIDTH, &top_pnt, SATELLITE_WIDTH, f1_0);
		gameStates.render.nInterpolationMethod = save_im;
		}
	}
#ifdef STATION_ENABLED
DrawPolygonModel (NULL, &gameData.endLevel.station.vPos, &vmdIdentityMatrix, NULL, gameData.endLevel.station.nModel, 0, f1_0, NULL, NULL);
#endif
RenderTerrain (&gameData.endLevel.exit.vGroundExit, exit_point_bmx, exit_point_bmy);
DrawExitModel ();
if (gameStates.render.bExtExplPlaying)
	DrawFireball (&external_explosion);
gameStates.render.nLighting=0;
RenderObject (gameData.objs.console, 0);
gameStates.render.nLighting=1;
}

//------------------------------------------------------------------------------

#define MAX_STARS 500

vmsVector stars [MAX_STARS];

void generate_starfield ()
{
	int i;

for (i = 0; i < MAX_STARS; i++) {
	stars [i].x = (d_rand () - RAND_MAX/2) << 14;
	stars [i].z = (d_rand () - RAND_MAX/2) << 14;
	stars [i].y = (d_rand ()/2) << 14;
	}
}

//------------------------------------------------------------------------------

void DrawStars ()
{
	int i;
	int intensity=31;
	g3s_point p;

for (i = 0; i < MAX_STARS; i++) {
	if ((i&63) == 0) {
		GrSetColorRGBi (RGBA_PAL (intensity, intensity, intensity));
		intensity-=3;
		}
	G3RotateDeltaVec (&p.p3_vec, &stars [i]);
	G3EncodePoint (&p);
	if (p.p3_codes == 0) {
		p.p3Flags &= ~PF_PROJECTED;
		G3ProjectPoint (&p);
		gr_pixel (f2i (p.p3_sx), f2i (p.p3_sy));
		}
	}
}

//------------------------------------------------------------------------------

void RenderEndLevelMine (fix eye_offset, int window_num)
{
	short start_seg_num;

viewerEye = gameData.objs.viewer->pos;
if (gameData.objs.viewer->nType == OBJ_PLAYER)
	VmVecScaleInc (&viewerEye, &gameData.objs.viewer->orient.fvec, (gameData.objs.viewer->size*3)/4);
if (eye_offset)
	VmVecScaleInc (&viewerEye, &gameData.objs.viewer->orient.rvec, eye_offset);
#ifdef EDITOR
if (gameStates.app.nFunctionMode==FMODE_EDITOR)
	viewerEye = gameData.objs.viewer->pos;
#endif
if (gameStates.app.bEndLevelSequence >= EL_OUTSIDE) {
	start_seg_num = gameData.endLevel.exit.nSegNum;
	}
else {
	start_seg_num = FindSegByPoint (&viewerEye, gameData.objs.viewer->nSegment);
	if (start_seg_num==-1)
		start_seg_num = gameData.objs.viewer->nSegment;
	}
if (gameStates.app.bEndLevelSequence == EL_LOOKBACK) {
	vmsMatrix headm, viewm;
	vmsAngVec angles = {0, 0, 0x7fff};
	VmAngles2Matrix (&headm, &angles);
	VmMatMul (&viewm, &gameData.objs.viewer->orient, &headm);
	G3SetViewMatrix (&viewerEye, &viewm, nRenderZoom);
	}
else
	G3SetViewMatrix (&viewerEye, &gameData.objs.viewer->orient, nRenderZoom);
RenderMine (start_seg_num, eye_offset, window_num);
}

//-----------------------------------------------------------------------------

void RenderEndLevelFrame (fix eye_offset, int window_num)
{
G3StartFrame (0, !window_num);
if (gameStates.app.bEndLevelSequence < EL_OUTSIDE)
	RenderEndLevelMine (eye_offset, window_num);
else if (!window_num)
	RenderExternalScene (eye_offset);
G3EndFrame ();
}

//------------------------------------------------------------------------------
///////////////////////// copy of flythrough code for endlevel

#define MAX_FLY_OBJECTS 2

flythrough_data flyObjects [MAX_FLY_OBJECTS];

flythrough_data *flydata;

int ELFindConnectedSide (int seg0, int seg1);

fixang DeltaAng (fixang a, fixang b);
fixang interp_angle (fixang dest, fixang src, fixang step);

#define DEFAULT_SPEED i2f (16)

#define MIN_D 0x100

//if speed is zero, use default speed
void StartEndLevelFlyThrough (int n, tObject *objP, fix speed)
{
flydata = flyObjects + n;
flydata->objP = objP;
flydata->firstTime = 1;
flydata->speed = speed?speed:DEFAULT_SPEED;
flydata->offset_frac = 0;
}

//------------------------------------------------------------------------------

static vmsAngVec *angvec_add2_scale (vmsAngVec *dest, vmsVector *src, fix s)
{
dest->p += FixMul (src->x, s);
dest->b += FixMul (src->z, s);
dest->h += FixMul (src->y, s);
return dest;
}

//-----------------------------------------------------------------------------

#define MAX_ANGSTEP	0x4000		//max turn per second

#define MAX_SLIDE_PER_SEGMENT 0x10000

void DoEndLevelFlyThrough (int n)
{
	tObject *objP;
	tSegment *pseg;
	int old_player_seg;

flydata = flyObjects + n;
objP = flydata->objP;
old_player_seg = objP->nSegment;

//move the player for this frame

if (!flydata->firstTime) {
	VmVecScaleInc (&objP->pos, &flydata->step, gameData.time.xFrame);
	angvec_add2_scale (&flydata->angles, &flydata->angstep, gameData.time.xFrame);
	VmAngles2Matrix (&objP->orient, &flydata->angles);
	}
//check new player seg
UpdateObjectSeg (objP);
pseg = &gameData.segs.segments [objP->nSegment];
if (flydata->firstTime || objP->nSegment != old_player_seg) {		//moved into new seg
	vmsVector curcenter, nextcenter;
	fix step_size, segTime;
	short entry_side, exit_side = -1;//what sides we entry and leave through
	vmsVector dest_point;		//where we are heading (center of exit_side)
	vmsAngVec dest_angles;		//where we want to be pointing
	vmsMatrix dest_orient;
	int up_side=0;

	entry_side=0;
	//find new exit tSide
	if (!flydata->firstTime) {
		entry_side = ELFindConnectedSide (objP->nSegment, old_player_seg);
		exit_side = sideOpposite [entry_side];
		}
	if (flydata->firstTime || entry_side==-1 || pseg->children [exit_side]==-1)
		exit_side = find_exit_side (objP);
	{										//find closest tSide to align to
		fix d, largest_d=-f1_0;
		int i;

		for (i = 0; i < 6; i++) {
#ifdef COMPACT_SEGS
			vmsVector v1;
			GetSideNormal (pseg, i, 0, &v1);
			d = VmVecDot (&v1, &flydata->objP->orient.uvec);
#else
			d = VmVecDot (&pseg->sides [i].normals [0], &flydata->objP->orient.uvec);
#endif
			if (d > largest_d) {largest_d = d; up_side=i;}
			}
		}
	//update target point & angles
	COMPUTE_SIDE_CENTER (&dest_point, pseg, exit_side);
	//update target point and movement points
	//offset tObject sideways
	if (flydata->offset_frac) {
		int s0=-1, s1=0, i;
		vmsVector s0p, s1p;
		fix dist;

		for (i = 0; i < 6; i++)
			if (i!=entry_side && i!=exit_side && i!=up_side && i!=sideOpposite [up_side]) {
				if (s0==-1)
					s0 = i;
				else
					s1 = i;
				}
		COMPUTE_SIDE_CENTER (&s0p, pseg, s0);
		COMPUTE_SIDE_CENTER (&s1p, pseg, s1);
		dist = FixMul (VmVecDist (&s0p, &s1p), flydata->offset_frac);
		if (dist-flydata->offsetDist > MAX_SLIDE_PER_SEGMENT)
			dist = flydata->offsetDist + MAX_SLIDE_PER_SEGMENT;
		flydata->offsetDist = dist;
		VmVecScaleInc (&dest_point, &objP->orient.rvec, dist);
		}
	VmVecSub (&flydata->step, &dest_point, &objP->pos);
	step_size = VmVecNormalizeQuick (&flydata->step);
	VmVecScale (&flydata->step, flydata->speed);
	COMPUTE_SEGMENT_CENTER (&curcenter, pseg);
	COMPUTE_SEGMENT_CENTER_I (&nextcenter, pseg->children [exit_side]);
	VmVecSub (&flydata->headvec, &nextcenter, &curcenter);
#ifdef COMPACT_SEGS
	{
	vmsVector _v1;
	GetSideNormal (pseg, up_side, 0, &_v1);
	VmVector2Matrix (&dest_orient, &flydata->headvec, &_v1, NULL);
	}
#else
	VmVector2Matrix (&dest_orient, &flydata->headvec, &pseg->sides [up_side].normals [0], NULL);
#endif
	VmExtractAnglesMatrix (&dest_angles, &dest_orient);
	if (flydata->firstTime)
		VmExtractAnglesMatrix (&flydata->angles, &objP->orient);
	segTime = FixDiv (step_size, flydata->speed);	//how long through seg
	if (segTime) {
		flydata->angstep.x = max (-MAX_ANGSTEP, min (MAX_ANGSTEP, FixDiv (DeltaAng (flydata->angles.p, dest_angles.p), segTime)));
		flydata->angstep.z = max (-MAX_ANGSTEP, min (MAX_ANGSTEP, FixDiv (DeltaAng (flydata->angles.b, dest_angles.b), segTime)));
		flydata->angstep.y = max (-MAX_ANGSTEP, min (MAX_ANGSTEP, FixDiv (DeltaAng (flydata->angles.h, dest_angles.h), segTime)));
		}
	else {
		flydata->angles = dest_angles;
		flydata->angstep.x = flydata->angstep.y = flydata->angstep.z = 0;
		}
	}
flydata->firstTime=0;
}

//------------------------------------------------------------------------------

#define JOY_NULL 15
#define ROT_SPEED 8		//rate of rotation while key held down
#define VEL_SPEED (15)	//rate of acceleration while key held down

extern short old_joy_x, old_joy_y;	//position last time around

#include "key.h"
#include "joy.h"

#ifdef SLEW_ON		//this is a special routine for slewing around external scene
int DoSlewMovement (tObject *objP, int check_keys, int check_joy)
{
	int moved = 0;
	vmsVector svel, movement;				//scaled velocity (per this frame)
	vmsMatrix rotmat, new_pm;
	int joy_x, joy_y, btns;
	int joyx_moved, joyy_moved;
	vmsAngVec rotang;

if (keyd_pressed [KEY_PAD5])
	VmVecZero (&objP->physInfo.velocity);

if (check_keys) {
	objP->physInfo.velocity.x += VEL_SPEED * (KeyDownTime (KEY_PAD9) - KeyDownTime (KEY_PAD7);
	objP->physInfo.velocity.y += VEL_SPEED * (KeyDownTime (KEY_PADMINUS) - KeyDownTime (KEY_PADPLUS);
	objP->physInfo.velocity.z += VEL_SPEED * (KeyDownTime (KEY_PAD8) - KeyDownTime (KEY_PAD2);

	rotang.pitch = (KeyDownTime (KEY_LBRACKET) - KeyDownTime (KEY_RBRACKET))/ROT_SPEED;
	rotang.bank  = (KeyDownTime (KEY_PAD1) - KeyDownTime (KEY_PAD3))/ROT_SPEED;
	rotang.head  = (KeyDownTime (KEY_PAD6) - KeyDownTime (KEY_PAD4))/ROT_SPEED;
	}
	else
		rotang.pitch = rotang.bank  = rotang.head  = 0;
//check for joystick movement
if (check_joy && joy_present)	{
	joy_get_pos (&joy_x, &joy_y);
	btns=joy_get_btns ();
	joyx_moved = (abs (joy_x - old_joy_x)>JOY_NULL);
	joyy_moved = (abs (joy_y - old_joy_y)>JOY_NULL);
	if (abs (joy_x) < JOY_NULL) joy_x = 0;
	if (abs (joy_y) < JOY_NULL) joy_y = 0;
	if (btns)
		if (!rotang.pitch) rotang.pitch = FixMul (-joy_y * 512, gameData.time.xFrame); else;
	else
		if (joyy_moved) objP->physInfo.velocity.z = -joy_y * 8192;
	if (!rotang.head) rotang.head = FixMul (joy_x * 512, gameData.time.xFrame);
	if (joyx_moved) 
		old_joy_x = joy_x;
	if (joyy_moved) 
		old_joy_y = joy_y;
	}
moved = rotang.pitch | rotang.bank | rotang.head;
VmAngles2Matrix (&rotmat, &rotang);
VmMatMul (&new_pm, &objP->orient, &rotmat);
objP->orient = new_pm;
VmTransposeMatrix (&new_pm);		//make those columns rows
moved |= objP->physInfo.velocity.x | objP->physInfo.velocity.y | objP->physInfo.velocity.z;
svel = objP->physInfo.velocity;
VmVecScale (&svel, gameData.time.xFrame);		//movement in this frame
VmVecRotate (&movement, &svel, &new_pm);
VmVecInc (&objP->pos, &movement);
moved |= (movement.x || movement.y || movement.z);
return moved;
}
#endif

//-----------------------------------------------------------------------------

#define LINE_LEN	80
#define NUM_VARS	8

#define STATION_DIST	i2f (1024)

int ConvertExt (char *dest, char *ext)
{
	char *t = strchr (dest, '.');

if (t && (t-dest <= 8)) {
	t [1] = ext [0];			
	t [2] = ext [1];			
	t [3] = ext [2];	
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

extern char szAutoMission [255];
//called for each level to load & setup the exit sequence
void LoadEndLevelData (int level_num)
{
	char filename [13];
	char line [LINE_LEN], *p;
	CFILE *ifile;
	int var, nSegment, nSide;
	int exit_side=0, i;
	int bHaveBinary = 0;

gameStates.app.bEndLevelDataLoaded = 0;		//not loaded yet
if (!gameOpts->movies.nLevel)
	return;

try_again:

if (gameStates.app.bAutoRunMission)
	strcpy (filename, szAutoMission);
else if (level_num<0)		//secret level
	strcpy (filename, gameData.missions.szSecretLevelNames [-level_num-1]);
else					//normal level
	strcpy (filename, gameData.missions.szLevelNames [level_num-1]);
if (!ConvertExt (filename, "end"))
	Error ("Error converting filename\n'<%s>'\nfor endlevel data\n", filename);
ifile = CFOpen (filename, gameFolders.szDataDir, "rb", gameStates.app.bD1Mission);
if (!ifile) {
	ConvertExt (filename, "txb");
	ifile = CFOpen (filename, gameFolders.szDataDir, "rb", gameStates.app.bD1Mission);
	if (!ifile) {
		if (level_num==1) {
#if TRACE
			con_printf (CON_DEBUG, "Cannot load file text\nof binary version of\n'<%s>'\n", filename);
#endif
			gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
			return;
			}
		else {
			level_num = 1;
			goto try_again;
			}
		}
	bHaveBinary = 1;
	}

//ok...this parser is pretty simple.  It ignores comments, but
//everything else must be in the right place
var = 0;
LogErr ("      parsing endlevel description\n");
while (CFGetS (line, LINE_LEN, ifile)) {
	if (bHaveBinary) {
		int l = (int) strlen (line) - 1;
		for (i = 0; i < l; i++)
			line [i] = EncodeRotateLeft ((char) (EncodeRotateLeft (line [i]) ^ BITMAP_TBL_XOR));
		p = line;
		}
	if ((p=strchr (line, ';')))
		*p = 0;		//cut off comment
	for (p=line+strlen (line)-1;p>line && isspace (*p);*p--=0);
	for (p=line;isspace (*p);p++);
	if (!*p)		//empty line
		continue;
	switch (var) {
		case 0: {						//ground terrain
			int iff_error;

			LogErr ("         loading terrain bitmap\n");
			if (gameData.endLevel.terrain.bmInstance.bm_texBuf) {
				OglFreeBmTexture (&gameData.endLevel.terrain.bmInstance);
				d_free (gameData.endLevel.terrain.bmInstance.bm_texBuf);
				}
			Assert (gameData.endLevel.terrain.bmInstance.bm_texBuf == NULL);
			iff_error = iff_read_bitmap (p, &gameData.endLevel.terrain.bmInstance, BM_LINEAR);
			if (iff_error != IFF_NO_ERROR) {
#ifdef _DEBUG
				Warning (TXT_EXIT_TERRAIN, p, iff_errormsg (iff_error));
#endif
				gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
				return;
			}
			gameData.endLevel.terrain.bmP = &gameData.endLevel.terrain.bmInstance;
			//GrRemapBitmapGood (gameData.endLevel.terrain.bmP, NULL, iff_transparent_color, -1);
			break;
		}

		case 1:							//height map
			LogErr ("         loading endlevel terrain\n");
			LoadTerrain (p);
			break;

		case 2:
			LogErr ("         loading exit point\n");
			sscanf (p, "%d, %d", &exit_point_bmx, &exit_point_bmy);
			break;

		case 3:							//exit heading
			LogErr ("         loading exit angle\n");
			exit_angles.h = i2f (atoi (p))/360;
			break;

		case 4: {						//planet bitmap
			int iff_error;

			LogErr ("         loading satellite bitmap\n");
			if (gameData.endLevel.satellite.bmInstance.bm_texBuf) {
				OglFreeBmTexture (&gameData.endLevel.satellite.bmInstance);
				d_free (gameData.endLevel.satellite.bmInstance.bm_texBuf);
				}
			iff_error = iff_read_bitmap (p, &gameData.endLevel.satellite.bmInstance, BM_LINEAR);
			if (iff_error != IFF_NO_ERROR) {
				Warning (TXT_SATELLITE, p, iff_errormsg (iff_error));
				gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
				return;
			}
			gameData.endLevel.satellite.bmP = &gameData.endLevel.satellite.bmInstance;
			//GrRemapBitmapGood (gameData.endLevel.satellite.bmP, NULL, iff_transparent_color, -1);
			break;
		}

		case 5:							//earth pos
		case 7: {						//station pos
			vmsMatrix tm;
			vmsAngVec ta;
			int pitch, head;

			LogErr ("         loading satellite and station position\n");
			sscanf (p, "%d, %d", &head, &pitch);
			ta.h = i2f (head)/360;
			ta.p = -i2f (pitch)/360;
			ta.b = 0;
			VmAngles2Matrix (&tm, &ta);
			if (var==5)
				gameData.endLevel.satellite.vPos = tm.fvec;
				//VmVecCopyScale (&gameData.endLevel.satellite.vPos, &tm.fvec, SATELLITE_DIST);
			else
				gameData.endLevel.station.vPos = tm.fvec;
			break;
		}

		case 6:						//planet size
			LogErr ("         loading satellite size\n");
			satellite_size = i2f (atoi (p));
			break;
	}
	var++;
}

Assert (var == NUM_VARS);
// OK, now the data is loaded.  Initialize everything
//find the exit sequence by searching all segments for a tSide with
//children == -2
for (nSegment=0, gameData.endLevel.exit.nSegNum=-1;gameData.endLevel.exit.nSegNum==-1 && nSegment<=gameData.segs.nLastSegment;nSegment++)
	for (nSide=0;nSide<6;nSide++)
		if (gameData.segs.segments [nSegment].children [nSide] == -2) {
			gameData.endLevel.exit.nSegNum = nSegment;
			exit_side = nSide;
			break;
			}

Assert (gameData.endLevel.exit.nSegNum!=-1);
LogErr ("      computing endlevel element orientation\n");
COMPUTE_SEGMENT_CENTER_I (&gameData.endLevel.exit.vMineExit, gameData.endLevel.exit.nSegNum);
ExtractOrientFromSegment (&gameData.endLevel.exit.mOrient, &gameData.segs.segments [gameData.endLevel.exit.nSegNum]);
COMPUTE_SIDE_CENTER_I (&gameData.endLevel.exit.vSideExit, gameData.endLevel.exit.nSegNum, exit_side);
VmVecScaleAdd (&gameData.endLevel.exit.vGroundExit, &gameData.endLevel.exit.vMineExit, &gameData.endLevel.exit.mOrient.uvec, -i2f (20));
//compute orientation of surface
{
	vmsVector tv;
	vmsMatrix exit_orient, tm;

	VmAngles2Matrix (&exit_orient, &exit_angles);
	VmTransposeMatrix (&exit_orient);
	VmMatMul (&surface_orient, &gameData.endLevel.exit.mOrient, &exit_orient);
	VmCopyTransposeMatrix (&tm, &surface_orient);
	VmVecRotate (&tv, &gameData.endLevel.station.vPos, &tm);
	VmVecScaleAdd (&gameData.endLevel.station.vPos, &gameData.endLevel.exit.vMineExit, &tv, STATION_DIST);
	VmVecRotate (&tv, &gameData.endLevel.satellite.vPos, &tm);
	VmVecScaleAdd (&gameData.endLevel.satellite.vPos, &gameData.endLevel.exit.vMineExit, &tv, SATELLITE_DIST);
	VmVector2Matrix (&tm, &tv, &surface_orient.uvec, NULL);
	VmVecCopyScale (&gameData.endLevel.satellite.vUp, &tm.uvec, SATELLITE_HEIGHT);
	}
CFClose (ifile);
gameStates.app.bEndLevelDataLoaded = 1;
}

//------------------------------------------------------------------------------
//eof
