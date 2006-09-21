/* $Id: gameseq.c,v 1.33 2003/11/26 12:26:30 btb Exp $ */
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
 * Routines for EndGame, EndLevel, etc.
 *
 * Old Log:
 * Revision 1.1  1995/12/05  16:02:05  allender
 * Initial revision
 *
 * Revision 1.14  1995/11/03  12:55:30  allender
 * shareware changes
 *
 * Revision 1.13  1995/10/31  10:23:07  allender
 * shareware stuff
 *
 * Revision 1.12  1995/10/18  18:25:02  allender
 * call AutoSelectWeapon after initing ammo since that may
 * change the secondary weapon status
 *
 * Revision 1.11  1995/10/17  13:17:11  allender
 * added closebox when entering pilot name
 *
 * Revision 1.10  1995/09/24  10:56:59  allender
 * new players must be looked for in gameData.multi.players directory
 *
 * Revision 1.9  1995/09/18  08:08:08  allender
 * remove netgame binding if at endgame
 *
 * Revision 1.8  1995/09/14  14:13:01  allender
 * initplayerobject have void return
 *
 * Revision 1.7  1995/08/31  12:54:42  allender
 * try and fix bug
 *
 * Revision 1.6  1995/08/26  16:25:40  allender
 * put return values on needed functions
 *
 * Revision 1.5  1995/08/14  09:26:28  allender
 * added byteswap header files
 *
 * Revision 1.4  1995/08/01  13:57:42  allender
 * macified player file stuff -- players stored in seperate folder
 *
 * Revision 1.3  1995/06/08  12:54:37  allender
 * new function for calculating a segment based checksum since the old way
 * is byte order dependent
 *
 * Revision 1.2  1995/06/02  07:42:10  allender
 * removed duplicate extern for NetworkEndLevelPoll2
 *
 * Revision 1.1  1995/05/16  15:25:56  allender
 * Initial revision
 *
 * Revision 2.10  1995/12/19  15:48:25  john
 * Made screen reset when loading new level.
 *
 * Revision 2.9  1995/07/07  16:47:52  john
 * Fixed bug with reactor time..
 *
 * Revision 2.8  1995/06/15  12:14:18  john
 * Made end game, win game and title sequences all go
 * on after 5 minutes automatically.
 *
 * Revision 2.7  1995/05/26  16:16:25  john
 * Split SATURN into define's for requiring cd, using cd, etc.
 * Also started adding all the Rockwell stuff.
 *
 * Revision 2.6  1995/03/24  13:11:20  john
 * Added save game during briefing screens.
 *
 * Revision 2.5  1995/03/23  17:56:20  allender
 * added code to record old laser level and weapons when player gets
 * new ship
 *
 * Revision 2.4  1995/03/21  08:39:14  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.3  1995/03/15  14:33:33  john
 * Added code to force the Descent CD-rom in the drive.
 *
 * Revision 2.2  1995/03/06  16:47:26  mike
 * destination saturn
 *
 * Revision 2.1  1995/03/06  15:23:23  john
 * New screen techniques.
 *
 * Revision 2.0  1995/02/27  11:28:53  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.310  1995/02/14  10:48:09  mike
 * zero bonus if you are a cheater.
 *
 * Revision 1.309  1995/02/11  19:17:08  rob
 * Fixed bug in laser fire rate after demo playback.
 *
 * Revision 1.308  1995/02/11  14:34:08  rob
 * Added include of netmisc.c
 *
 * Revision 1.307  1995/02/11  14:29:04  rob
 * Fixes for invul. controlcen.
 *
 * Revision 1.306  1995/02/11  13:47:00  mike
 * fix cheats.
 *
 * Revision 1.305  1995/02/11  13:10:52  rob
 * Fixed end of anarchy mission problems.
 *
 * Revision 1.304  1995/02/11  12:46:12  mike
 * initialize gameStates.app.cheats.bRobotsFiring, part of AHIMSA cheat.
 *
 * Revision 1.303  1995/02/11  12:42:03  john
 * Added new song method, with FM bank switching..
 *
 * Revision 1.302  1995/02/10  17:39:29  matt
 * Changed secret exit message to be centered
 *
 * Revision 1.301  1995/02/10  16:17:33  mike
 * init Last_level_path_shown.
 *
 * Revision 1.300  1995/02/09  22:18:22  john
 * Took out between level saves.
 *
 * Revision 1.299  1995/02/09  12:11:42  rob
 * Get rid of high scores thing for multiplayer games.
 *
 * Revision 1.298  1995/02/08  20:34:24  rob
 * Took briefing screens back OUT of coop games (per Interplay request)
 *
 * Revision 1.297  1995/02/08  19:20:09  rob
 * Moved checksum calc.
 *
 * Revision 1.296  1995/02/05  14:39:24  rob
 * Changed object mapping to be more efficient.
 *
 * Revision 1.295  1995/02/02  19:05:38  john
 * Made end level menu for 27 not overwrite descent title..
 *
 * Revision 1.294  1995/02/02  16:36:42  adam
 * *** empty log message ***
 *
 * Revision 1.293  1995/02/02  15:58:02  john
 * Added turbo mode cheat.
 *
 * Revision 1.292  1995/02/02  15:29:34  matt
 * Changed & localized secret level text
 *
 * Revision 1.291  1995/02/02  10:50:03  adam
 * messed with secret level message
 *
 * Revision 1.290  1995/02/02  01:20:28  adam
 * changed endgame song temporarily.
 *
 * Revision 1.289  1995/02/01  23:19:43  rob
 * Fixed up endlevel stuff for multiplayer.
 * Put in palette fades around areas that didn't have them before.
 *
 * Revision 1.288  1995/02/01  17:12:34  mike
 * Make score come after endgame screens.
 *
 * Revision 1.287  1995/01/30  18:34:30  rob
 * Put briefing screens back into coop games.
 *
 * Revision 1.286  1995/01/27  13:07:59  rob
 * Removed erroneous warning message.
 *
 * Revision 1.285  1995/01/27  11:47:43  rob
 * Removed new secret level menu from multiplayer games.
 *
 * Revision 1.284  1995/01/26  22:11:11  mike
 * Purple chromo-blaster (ie, fusion cannon) spruce up (chromification)
 *
 * Revision 1.283  1995/01/26  16:55:13  rob
 * Removed ship bonus from cooperative endgame.
 *
 * Revision 1.282  1995/01/26  16:45:24  mike
 * Add autofire fusion cannon stuff.
 *
 * Revision 1.281  1995/01/26  14:44:44  rob
 * Removed unnecessary #ifdefs around mprintfs.
 * Changed gameData.multi.nPlayerPositions to be independant of networkData.nMaxPlayers to
 * accomodate 4-player robo-archy games with 8 start positions.
 *
 * Revision 1.280  1995/01/26  12:19:01  rob
 * Changed NetworkDoFrame call.
 *
 * Revision 1.279  1995/01/26  00:35:03  matt
 * Changed numbering convention for HMP files for levels
 *
 * Revision 1.278  1995/01/25  16:07:59  matt
 * Added message (prototype) when going to secret level
 *
 * Revision 1.277  1995/01/22  18:57:23  matt
 * Made player highest level work with missions
 *
 * Revision 1.276  1995/01/21  23:13:08  matt
 * Made high scores with (not work, really) with loaded missions
 * Don't give player high score when quit game
 *
 * Revision 1.275  1995/01/21  17:17:39  john
 * *** empty log message ***
 *
 * Revision 1.274  1995/01/21  17:15:38  john
 * Added include for state.h
 *
 * Revision 1.273  1995/01/21  16:21:14  matt
 * Fixed bugs in secret level sequencing
 *
 * Revision 1.272  1995/01/20  22:47:29  matt
 * Mission system implemented, though imcompletely
 *
 * Revision 1.271  1995/01/19  17:00:48  john
 * Made save game work between levels.
 *
 * Revision 1.270  1995/01/17  17:49:10  rob
 * Added key syncing for coop.
 *
 * Revision 1.269  1995/01/17  14:27:37  john
 * y
 *
 * Revision 1.268  1995/01/17  13:36:33  john
 * Moved pig loading into StartNewLevelSub.
 *
 * Revision 1.267  1995/01/16  16:53:55  john
 * Added code to save cheat state during save game.
 *
 * Revision 1.266  1995/01/15  19:42:10  matt
 * Ripped out hostage faces for registered version
 *
 * Revision 1.265  1995/01/15  16:55:06  john
 * Improved mine texture parsing.
 *
 * Revision 1.264  1995/01/15  11:56:24  john
 * Working version of paging.
 *
 * Revision 1.263  1995/01/14  19:16:40  john
 * First version of new bitmap paging code.
 *
 * Revision 1.262  1995/01/13  17:38:58  yuan
 * Removed Int3 () for number players check.
 *
 * Revision 1.261  1995/01/12  12:09:52  yuan
 * Added coop object capability.
 *
 * Revision 1.260  1995/01/05  17:16:08  yuan
 * Removed Int3s.
 *
 * Revision 1.259  1995/01/05  11:34:29  john
 * Took out endlevel save stuff for registered.
 *
 * Revision 1.258  1995/01/04  19:00:16  rob
 * Added some debugging for two bugs.
 *
 * Revision 1.257  1995/01/04  13:18:18  john
 * Added cool 6 game save.
 *
 * Revision 1.256  1995/01/04  08:46:18  rob
 * JOHN CHECKED IN FOR ROB !!!
 *
 * Revision 1.255  1995/01/02  20:07:35  rob
 * Added score syncing.
 * Get rid of endlevel score for coop games (put it back in elsewhere)
 *
 * Revision 1.254  1995/01/02  16:17:43  mike
 * init super boss.
 *
 * Revision 1.253  1994/12/21  21:08:47  rob
 * fixed a bug in coop player ship positions.
 *
 * Revision 1.252  1994/12/21  12:57:08  rob
 * Handle additional player ships in mines.
 *
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
char gameseq_rcsid [] = "$Id: gameseq.c,v 1.33 2003/11/26 12:26:30 btb Exp $";
#endif

#ifdef WINDOWS
#include "desw.h"
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <time.h>

#ifdef OGL
#include "ogl_init.h"
#endif

#include "pa_enabl.h"                   //$$POLY_ACC
#include "console.h"
#include "inferno.h"
#include "game.h"
#include "player.h"
#include "key.h"
#include "object.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "cameras.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "fuelcen.h"
#include "switch.h"
#include "digi.h"
#include "gamesave.h"
#include "scores.h"
#include "ibitblt.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "titles.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "gamefont.h"
#include "newmenu.h"
#include "endlevel.h"
#ifdef NETWORK
#  include "multi.h"
#  include "network.h"
#  include "netmisc.h"
#  include "modem.h"
#endif
#include "playsave.h"
#include "ctype.h"
#include "fireball.h"
#include "kconfig.h"
#include "config.h"
#include "robot.h"
#include "automap.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "text.h"
#include "cfile.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "mission.h"
#include "state.h"
#include "songs.h"
#include "gamepal.h"
#include "movie.h"
#include "controls.h"
#include "credits.h"
#include "gamemine.h"
#include "lightmap.h"
#include "polyobj.h"
#include "movie.h"
#include "particles.h"

#if defined (POLY_ACC)
#include "poly_acc.h"
#endif
#if defined (TACTILE)
 #include "tactile.h"
#endif

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "strutil.h"
#include "rle.h"
#include "input.h"

//------------------------------------------------------------------------------

void ShowLevelIntro (int nLevel);
int StartNewLevelSecret (int nLevel, int bPageInTextures);
void InitPlayerPosition (int random_flag);
void ResetMovementPath ();
void LoadStars (bkg *bg, int bRedraw);
void ReturningToLevelMessage (void);
void AdvancingToLevelMessage (void);
void DoEndGame (void);
void AdvanceLevel (int bSecret, int bFromSecret);
void FilterObjectsFromLevel ();

// From allender -- you'll find these defines in state.c and cntrlcen.c
// since I couldn't think of a good place to put them and i wanted to
// fix this stuff fast!  Sorry about that...

#define SECRETB_FILENAME	"secret.sgb"
#define SECRETC_FILENAME	"secret.sgc"

//gameData.missions.nCurrentLevel starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 means not a real level loaded
// Global variables telling what sort of game we have

void GrRemapMonoFonts ();
void SetFunctionMode (int);
void InitHoardData ();

extern fix ThisLevelTime;

// Extern from game.c to fix a bug in the cockpit!

extern int last_drawn_cockpit [2];
extern int nLastLevelPathCreated;
extern int nTimeLastMoved;

//	HUDClearMessages external, declared in gauges.h
#ifndef _GAUGES_H
extern void HUDClearMessages (); // From hud.c
#endif

//	Extra prototypes declared for the sake of LINT
void InitPlayerStatsNewShip (void);
void copy_defaults_to_robot_all (void);

extern int nDescentCriticalError;

extern int nLastMsgYCrd;

//--------------------------------------------------------------------

void verify_console_object ()
{
	Assert (gameData.multi.nLocalPlayer > -1);
	Assert (gameData.multi.players [gameData.multi.nLocalPlayer].objnum > -1);
	gameData.objs.console = gameData.objs.objects + gameData.multi.players [gameData.multi.nLocalPlayer].objnum;
	Assert (gameData.objs.console->type == OBJ_PLAYER);
	Assert (gameData.objs.console->id==gameData.multi.nLocalPlayer);
}

//------------------------------------------------------------------------------

int count_number_of_robots ()
{
	int robot_count;
	int i;

	robot_count = 0;
	for (i=0;i<=gameData.objs.nLastObject;i++) {
		if (gameData.objs.objects [i].type == OBJ_ROBOT)
			robot_count++;
	}

	return robot_count;
}

//------------------------------------------------------------------------------

int count_number_of_hostages ()
{
	int count;
	int i;

	count = 0;
	for (i=0;i<=gameData.objs.nLastObject;i++) {
		if (gameData.objs.objects [i].type == OBJ_HOSTAGE)
			count++;
	}

	return count;
}

//------------------------------------------------------------------------------
//added 10/12/95: delete buddy bot if coop game.  Probably doesn't really belong here. -MT
void GameSeqInitNetworkPlayers ()
{
	int		i, j, t,
				segNum, segType, 
				playerObjs [MAX_PLAYERS], startSegs [MAX_PLAYERS],
				nPlayers;
	object	*objP;

	// Initialize network player start locations and object numbers

memset (gameStates.multi.bPlayerIsTyping, 0, sizeof (gameStates.multi.bPlayerIsTyping));
nPlayers = 0;
j = 0;
for (i = 0, objP = gameData.objs.objects;i <= gameData.objs.nLastObject; i++, objP++) {
	t = objP->type;
	if ((t == OBJ_PLAYER) || (t == OBJ_GHOST) || (t == OBJ_COOP)) {
		if ((gameData.app.nGameMode & GM_MULTI_COOP) ? (j && (t != OBJ_COOP)) : (t == OBJ_COOP))
			ReleaseObject ((short) i);
		else {
			playerObjs [nPlayers] = i;
			startSegs [nPlayers] = gameData.objs.objects [i].segnum;
			nPlayers++;
			}
		j++;
		}
	else if (t == OBJ_ROBOT) {
		if (gameData.bots.pInfo [objP->id].companion && (gameData.app.nGameMode & GM_MULTI))
			ReleaseObject ((short) i);		//kill the buddy in netgames
		}
	}

// the following code takes care of team players being assigned the proper start locations
// in enhanced CTF
for (i = 0; i < nPlayers; i++) {
// find a player object that resides in a segment of proper type for the current
// player start info 
	for (j = 0; j < nPlayers; j++) {
		segNum = startSegs [j];
		if (segNum < 0)
			continue;
		segType = (gameData.app.nGameMode & GM_MULTI_COOP) ? gameData.segs.segment2s [segNum].special : SEGMENT_IS_NOTHING;
#if 0			
		switch (segType) {
			case SEGMENT_IS_GOAL_RED:
			case SEGMENT_IS_TEAM_RED:
				if (i < nPlayers / 2) // (GetTeam (i) != TEAM_RED)
					continue;
				gameData.segs.segment2s [segNum].special = SEGMENT_IS_NOTHING;
				break;
			case SEGMENT_IS_GOAL_BLUE:
			case SEGMENT_IS_TEAM_BLUE:
				if (i >= nPlayers / 2) //GetTeam (i) != TEAM_BLUE)
					continue;
				gameData.segs.segment2s [segNum].special = SEGMENT_IS_NOTHING;
				break;
			default:
				break;
			}
#endif			
		objP = gameData.objs.objects + playerObjs [j];
		objP->type = OBJ_PLAYER;
		gameData.multi.playerInit [i].pos = objP->pos;
		gameData.multi.playerInit [i].orient = objP->orient;
		gameData.multi.playerInit [i].segnum = objP->segnum;
		gameData.multi.playerInit [i].segtype = segType;
		gameData.multi.players [i].objnum = playerObjs [j];
		objP->id = i;
		startSegs [j] = -1;
		break;
		}
	}
gameData.objs.viewer = gameData.objs.console = gameData.objs.objects; // + gameData.multi.players [gameData.multi.nLocalPlayer].objnum;
gameData.multi.nPlayerPositions = nPlayers;

#ifndef NDEBUG
if (( (gameData.app.nGameMode & GM_MULTI_COOP) && (gameData.multi.nPlayerPositions != 4)) ||
	  (!(gameData.app.nGameMode & GM_MULTI_COOP) && (gameData.multi.nPlayerPositions != 8)))
{
#if TRACE		
	//con_printf (CON_VERBOSE, "--NOT ENOUGH MULTIPLAYER POSITIONS IN THIS MINE!--\n");
#endif
	//Int3 (); // Not enough positions!!
}
#endif
#ifdef NETWORK
if (IS_D2_OEM && (gameData.app.nGameMode & GM_MULTI) && gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission && gameData.missions.nCurrentLevel==8) {
	for (i=0;i<nPlayers;i++)
		if (gameData.multi.players [i].connected && !(netPlayers.players [i].version_minor & 0xF0)) {
			ExecMessageBox ("Warning!",NULL,1,TXT_OK,"This special version of Descent II\nwill disconnect after this level.\nPlease purchase the full version\nto experience all the levels!");	
			return;
			}
	}

if (IS_MAC_SHARE && (gameData.app.nGameMode & GM_MULTI) && gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission && gameData.missions.nCurrentLevel == 4) {
	for (i = 0; i < nPlayers; i++)
		if (gameData.multi.players [i].connected && !(netPlayers.players [i].version_minor & 0xF0)) {
			ExecMessageBox ("Warning!",NULL , 1 ,TXT_OK, "This shareware version of Descent II\nwill disconnect after this level.\nPlease purchase the full version\nto experience all the levels!");
			return;
			}
	}
#endif // NETWORK
}

//------------------------------------------------------------------------------

void gameseq_remove_unused_players ()
{
	int i;

	// 'Remove' the unused players

#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI)
	{
		for (i=0; i < gameData.multi.nPlayerPositions; i++)
		{
			if ((!gameData.multi.players [i].connected) || (i >= gameData.multi.nPlayers))
			{
			MultiMakePlayerGhost (i);
			}
		}
	}
	else
#endif
	{		// Note link to above if!!!
		for (i=1; i < gameData.multi.nPlayerPositions; i++)
		{
			ReleaseObject ((short) gameData.multi.players [i].objnum);
		}
	}
}

fix xStartingShields=INITIAL_SHIELDS;

// Setup player for new game
void InitPlayerStatsGame ()
{
	gameData.multi.players [gameData.multi.nLocalPlayer].score = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].last_score = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].lives = INITIAL_LIVES;
	gameData.multi.players [gameData.multi.nLocalPlayer].level = 1;

	gameData.multi.players [gameData.multi.nLocalPlayer].time_level = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].time_total = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].hours_level = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].hours_total = 0;

	gameData.multi.players [gameData.multi.nLocalPlayer].energy = INITIAL_ENERGY;
	gameData.multi.players [gameData.multi.nLocalPlayer].shields = xStartingShields;
	gameData.multi.players [gameData.multi.nLocalPlayer].killer_objnum = -1;

	gameData.multi.players [gameData.multi.nLocalPlayer].net_killed_total = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].net_kills_total = 0;

	gameData.multi.players [gameData.multi.nLocalPlayer].num_kills_level = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].num_kills_total = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].num_robots_level = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].num_robots_total = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].KillGoalCount = 0;
	
	gameData.multi.players [gameData.multi.nLocalPlayer].hostages_rescued_total = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].hostages_level = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].hostages_total = 0;

	gameData.multi.players [gameData.multi.nLocalPlayer].laser_level = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].flags = 0;
	
	InitPlayerStatsNewShip ();

	gameStates.app.bFirstSecretVisit = 1;
}

//------------------------------------------------------------------------------

void init_ammo_and_energy (void)
{
	if (gameData.multi.players [gameData.multi.nLocalPlayer].energy < INITIAL_ENERGY)
		gameData.multi.players [gameData.multi.nLocalPlayer].energy = INITIAL_ENERGY;
	if (gameData.multi.players [gameData.multi.nLocalPlayer].shields < xStartingShields)
		gameData.multi.players [gameData.multi.nLocalPlayer].shields = xStartingShields;

//	for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
//		if (gameData.multi.players [gameData.multi.nLocalPlayer].primary_ammo [i] < Default_primary_ammo_level [i])
//			gameData.multi.players [gameData.multi.nLocalPlayer].primary_ammo [i] = Default_primary_ammo_level [i];

//	for (i=0; i<MAX_SECONDARY_WEAPONS; i++)
//		if (gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [i] < Default_secondary_ammo_level [i])
//			gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [i] = Default_secondary_ammo_level [i];
	if (gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [0] < 2 + NDL - gameStates.app.nDifficultyLevel)
		gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [0] = 2 + NDL - gameStates.app.nDifficultyLevel;
}

extern	ubyte	Last_afterburner_state;

// Setup player for new level (After completion of previous level)
void init_player_stats_level (int bSecret)
{
	// int	i;
gameData.multi.players [gameData.multi.nLocalPlayer].last_score = gameData.multi.players [gameData.multi.nLocalPlayer].score;
gameData.multi.players [gameData.multi.nLocalPlayer].level = gameData.missions.nCurrentLevel;
#ifdef NETWORK
if (!networkData.bRejoined) {
#endif
	gameData.multi.players [gameData.multi.nLocalPlayer].time_level = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].hours_level = 0;
#ifdef NETWORK
	}
#endif
gameData.multi.players [gameData.multi.nLocalPlayer].killer_objnum = -1;
gameData.multi.players [gameData.multi.nLocalPlayer].num_kills_level = 0;
gameData.multi.players [gameData.multi.nLocalPlayer].num_robots_level = count_number_of_robots ();
gameData.multi.players [gameData.multi.nLocalPlayer].num_robots_total += gameData.multi.players [gameData.multi.nLocalPlayer].num_robots_level;
gameData.multi.players [gameData.multi.nLocalPlayer].hostages_level = count_number_of_hostages ();
gameData.multi.players [gameData.multi.nLocalPlayer].hostages_total += gameData.multi.players [gameData.multi.nLocalPlayer].hostages_level;
gameData.multi.players [gameData.multi.nLocalPlayer].hostages_on_board = 0;
if (!bSecret) {
	init_ammo_and_energy ();
	gameData.multi.players [gameData.multi.nLocalPlayer].flags &=  
		~ (PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_MAP_ALL | KEY_BLUE | KEY_RED | KEY_GOLD);
	gameData.multi.players [gameData.multi.nLocalPlayer].cloak_time = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].invulnerable_time = 0;
	if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP))
		gameData.multi.players [gameData.multi.nLocalPlayer].flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);
	}
else if (gameStates.app.bD1Mission)
	init_ammo_and_energy ();
gameStates.app.bPlayerIsDead = 0; // Added by RH
gameData.multi.players [gameData.multi.nLocalPlayer].homing_object_dist = -F1_0; // Added by RH
Last_laser_fired_time = xNextLaserFireTime = gameData.app.xGameTime; // added by RH, solved demo playback bug
Controls.afterburner_state = 0;
Last_afterburner_state = 0;
DigiKillSoundLinkedToObject (gameData.multi.players [gameData.multi.nLocalPlayer].objnum);
InitGauges ();
#ifdef TACTILE
if (TactileStick)
	tactile_set_button_jolt ();
#endif
gameData.objs.missileViewer = NULL;
}

//------------------------------------------------------------------------------

extern	void init_ai_for_ship (void);

// Setup player for a brand-new ship
void InitPlayerStatsNewShip ()
{
	int	i;

	if (gameData.demo.nState == ND_STATE_RECORDING) {
		NDRecordLaserLevel (gameData.multi.players [gameData.multi.nLocalPlayer].laser_level, 0);
		NDRecordPlayerWeapon (0, 0);
		NDRecordPlayerWeapon (1, 0);
	}

	gameData.multi.players [gameData.multi.nLocalPlayer].energy = INITIAL_ENERGY;
	gameData.multi.players [gameData.multi.nLocalPlayer].shields = xStartingShields;
	gameData.multi.players [gameData.multi.nLocalPlayer].laser_level = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].killer_objnum = -1;
	gameData.multi.players [gameData.multi.nLocalPlayer].hostages_on_board = 0;

	xAfterburnerCharge = 0;

	for (i=0; i<MAX_PRIMARY_WEAPONS; i++) {
		gameData.multi.players [gameData.multi.nLocalPlayer].primary_ammo [i] = 0;
		bLastPrimaryWasSuper [i] = 0;
	}

	for (i=1; i<MAX_SECONDARY_WEAPONS; i++) {
		gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [i] = 0;
		bLastSecondaryWasSuper [i] = 0;
	}
	gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [0] = 2 + NDL - gameStates.app.nDifficultyLevel;
	gameData.multi.players [gameData.multi.nLocalPlayer].primary_weapon_flags = HAS_LASER_FLAG;
	gameData.multi.players [gameData.multi.nLocalPlayer].secondary_weapon_flags = HAS_CONCUSSION_FLAG;
	gameData.weapons.nOverridden = 0;
	gameData.weapons.nPrimary = 0;
	gameData.weapons.nSecondary = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].flags &= ~ (	PLAYER_FLAGS_QUAD_LASERS |
												PLAYER_FLAGS_AFTERBURNER |
												PLAYER_FLAGS_CLOAKED |
												PLAYER_FLAGS_INVULNERABLE |
												PLAYER_FLAGS_MAP_ALL |
												PLAYER_FLAGS_CONVERTER |
												PLAYER_FLAGS_AMMO_RACK |
												PLAYER_FLAGS_HEADLIGHT |
												PLAYER_FLAGS_HEADLIGHT_ON |
												PLAYER_FLAGS_FLAG);
	gameData.multi.players [gameData.multi.nLocalPlayer].cloak_time = 0;
	gameData.multi.players [gameData.multi.nLocalPlayer].invulnerable_time = 0;
	gameStates.app.bPlayerIsDead = 0;		//player no longer dead
	gameData.multi.players [gameData.multi.nLocalPlayer].homing_object_dist = -F1_0; // Added by RH
	Controls.afterburner_state = 0;
	Last_afterburner_state = 0;
	DigiKillSoundLinkedToObject (gameData.multi.players [gameData.multi.nLocalPlayer].objnum);
	gameData.objs.missileViewer=NULL;		///reset missile camera if out there
   #ifdef TACTILE
		if (TactileStick)
		 {
		  tactile_set_button_jolt ();
		 }
	#endif
	init_ai_for_ship ();
}

//------------------------------------------------------------------------------

extern void InitStuckObjects (void);

#ifdef EDITOR

extern int gameData.segs.bHaveSlideSegs;

//reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_on_level ()
{
	GameSeqInitNetworkPlayers ();
	init_player_stats_level (0);
	gameData.objs.viewer = gameData.objs.console;
	gameData.objs.console = gameData.objs.viewer = gameData.objs.objects + gameData.multi.players [gameData.multi.nLocalPlayer].objnum;
	gameData.objs.console->id=gameData.multi.nLocalPlayer;
	gameData.objs.console->control_type = CT_FLYING;
	gameData.objs.console->movement_type = MT_PHYSICS;
	gameStates.app.bGameSuspended = 0;
	verify_console_object ();
	gameData.reactor.bDestroyed = 0;
	if (gameData.demo.nState != ND_STATE_PLAYBACK)
		gameseq_remove_unused_players ();
	InitCockpit ();
	InitRobotsForLevel ();
	InitAIObjects ();
	MorphInit ();
	InitAllMatCens ();
	InitPlayerStatsNewShip ();
	InitReactorForLevel ();
	AutomapClearVisited ();
	InitStuckObjects ();
	InitThiefForLevel ();

	gameData.segs.bHaveSlideSegs = 0;
}
#endif

//------------------------------------------------------------------------------

//do whatever needs to be done when a player dies in multiplayer
void DoGameOver ()
{
//	ExecMessageBox (TXT_GAME_OVER, 1, TXT_OK, "");

	if (gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission)
		scores_maybe_add_player (0);

	SetFunctionMode (FMODE_MENU);
	gameData.app.nGameMode = GM_GAME_OVER;
	longjmp (gameExitPoint, 0);		// Exit out of game loop

}

//------------------------------------------------------------------------------

extern void do_save_game_menu ();

//update various information about the player
void UpdatePlayerStats ()
{
gameData.multi.players [gameData.multi.nLocalPlayer].time_level += gameData.app.xFrameTime;	//the never-ending march of time...
if (gameData.multi.players [gameData.multi.nLocalPlayer].time_level > i2f (3600))	{
	gameData.multi.players [gameData.multi.nLocalPlayer].time_level -= i2f (3600);
	gameData.multi.players [gameData.multi.nLocalPlayer].hours_level++;
	}
gameData.multi.players [gameData.multi.nLocalPlayer].time_total += gameData.app.xFrameTime;	//the never-ending march of time...
if (gameData.multi.players [gameData.multi.nLocalPlayer].time_total > i2f (3600))	{
	gameData.multi.players [gameData.multi.nLocalPlayer].time_total -= i2f (3600);
	gameData.multi.players [gameData.multi.nLocalPlayer].hours_total++;
	}
}

//------------------------------------------------------------------------------

//go through this level and start any eclip sounds
void SetSoundSources ()
{
	short segnum,sidenum;
	segment *seg;
	vms_vector pnt;
	int csegnum;

gameOpts->sound.bD1Sound = gameStates.app.bD1Mission && gameStates.app.bHaveD1Data && gameOpts->sound.bUseD1Sounds;
DigiInitSounds ();		//clear old sounds
gameStates.sound.bDontStartObjects = 1;
for (seg = gameData.segs.segments, segnum = 0; segnum <= gameData.segs.nLastSegment; seg++, segnum++)
	for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {
		int	tm, ec;
		short	sn;

		if (!(WALL_IS_DOORWAY (seg,sidenum, NULL) & WID_RENDER_FLAG))
			continue;
		if (tm = seg->sides [sidenum].tmap_num2)
			ec = gameData.pig.tex.pTMapInfo [tm&0x3fff].eclip_num;
		else
			ec = -1;
		if (ec < 0)
			ec = gameData.pig.tex.pTMapInfo [seg->sides [sidenum].tmap_num].eclip_num;
		if (ec < 0)
			continue;
		if ((sn = gameData.eff.pEffects [ec].sound_num) == -1)
			continue;
		csegnum = seg->children [sidenum];

		//check for sound on other side of wall.  Don't add on
		//both walls if sound travels through wall.  If sound
		//does travel through wall, add sound for lower-numbered
		//segment.

		if (IS_CHILD (csegnum) && (csegnum < segnum) &&
			 (WALL_IS_DOORWAY (seg, sidenum, NULL) & (WID_FLY_FLAG | WID_RENDPAST_FLAG))) {
			segment *csegp = gameData.segs.segments+seg->children [sidenum];
			int csidenum = FindConnectedSide (seg, csegp);
			if (csegp->sides [csidenum].tmap_num2 == seg->sides [sidenum].tmap_num2)
				continue;		//skip this one
			}
		COMPUTE_SIDE_CENTER (&pnt,seg,sidenum);
		DigiLinkSoundToPos (sn, segnum, sidenum, &pnt, 1, F1_0/2);
		}
gameOpts->sound.bD1Sound = 0;
gameStates.sound.bDontStartObjects = 0;
}


//------------------------------------------------------------------------------

//fix flash_dist=i2f (1);
fix flash_dist=fl2f (.9);
//create flash for player appearance
void CreatePlayerAppearanceEffect (object *playerObjP)
{
	vms_vector pos;
	object *effectObjP;

#ifndef NDEBUG
	{
	int objnum = OBJ_IDX (playerObjP);
	if ((objnum < 0) || (objnum > gameData.objs.nLastObject))
		Int3 (); // See Rob, trying to track down weird network bug
	}
#endif
if (playerObjP == gameData.objs.viewer)
	VmVecScaleAdd (&pos, &playerObjP->pos, &playerObjP->orient.fvec, fixmul (playerObjP->size,flash_dist));
else
	pos = playerObjP->pos;
effectObjP = ObjectCreateExplosion (playerObjP->segnum, &pos, playerObjP->size, VCLIP_PLAYER_APPEARANCE);
if (effectObjP) {
	effectObjP->orient = playerObjP->orient;
	if (gameData.eff.vClips [0] [VCLIP_PLAYER_APPEARANCE].sound_num > -1)
		DigiLinkSoundToObject (gameData.eff.vClips [0] [VCLIP_PLAYER_APPEARANCE].sound_num, OBJ_IDX (effectObjP), 0, F1_0);
	}
}

//------------------------------------------------------------------------------

//
// New Game sequencing functions
//

//pairs of chars describing ranges
char playername_allowed_chars [] = "azAZ09__--";

int MakeNewPlayerFile (int allow_abort)
{
	int x;
	char filename [FILENAME_LEN];
	newmenu_item m;
	char text [CALLSIGN_LEN+1]="";
#if 0
	FILE *fp;
#endif

strncpy (text, gameData.multi.players [gameData.multi.nLocalPlayer].callsign,CALLSIGN_LEN);

try_again:

memset (&m, 0, sizeof (m));
m.type=NM_TYPE_INPUT; 
m.text_len = 8; 
m.text = text;

nmAllowedChars = playername_allowed_chars;
x = ExecMenu (NULL, TXT_ENTER_PILOT_NAME, 1, &m, NULL, NULL);
nmAllowedChars = NULL;
if (x < 0) {
	if (allow_abort) return 0;
	goto try_again;
	}
if (text [0]==0)	//null string
	goto try_again;
sprintf (filename, "%s.plr", text);

if (CFExist (filename,gameFolders.szProfDir,0)) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, "%s '%s' %s", TXT_PLAYER, text, TXT_ALREADY_EXISTS);
	goto try_again;
	}
if (!new_player_config ())
	goto try_again;			// They hit Esc during New player config
strncpy (gameData.multi.players [gameData.multi.nLocalPlayer].callsign, text, CALLSIGN_LEN);
WritePlayerFile ();
return 1;
}

//------------------------------------------------------------------------------

#ifdef WINDOWS
#undef TXT_SELECT_PILOT
#define TXT_SELECT_PILOT "Select pilot\n<Ctrl-D> or Right-click\nto delete"
#endif

//Inputs the player's name, without putting up the background screen
int SelectPlayer ()
{
	static int bStartup = 1;
	int 	i,j, bAutoPlr;
	char 	filename [FILENAME_LEN];
	char	filespec [FILENAME_LEN];
	int 	allow_abort_flag = !bStartup;

if (gameData.multi.players [gameData.multi.nLocalPlayer].callsign [0] == 0)	{
	//---------------------------------------------------------------------
	// Set default config options in case there is no config file
	// kc_keyboard, kc_joystick, kc_mouse are statically defined.
	gameOpts->input.joySensitivity [0] =
	gameOpts->input.joySensitivity [1] =
	gameOpts->input.joySensitivity [2] =
	gameOpts->input.joySensitivity [3] = 8;
	gameOpts->input.mouseSensitivity [0] =
	gameOpts->input.mouseSensitivity [1] =
	gameOpts->input.mouseSensitivity [2] = 8;
	gameConfig.nControlType =CONTROL_NONE;
	for (i=0; i<CONTROL_MAX_TYPES; i++)
		for (j=0; j<MAX_CONTROLS; j++)
			controlSettings.custom [i] [j] = controlSettings.defaults [i] [j];
	KCSetControls ();
	//----------------------------------------------------------------

	// Read the last player's name from config file, not lastplr.txt
	strncpy (gameData.multi.players [gameData.multi.nLocalPlayer].callsign, gameConfig.szLastPlayer, CALLSIGN_LEN);
	if (gameConfig.szLastPlayer [0]==0)
		allow_abort_flag = 0;
	}
if (bAutoPlr = gameData.multi.autoNG.bValid)
	strncpy (filename, gameData.multi.autoNG.szPlayer, 8);
else if (bAutoPlr = bStartup && (i = FindArg ("-player")) && *Args [++i])
	strncpy (filename, Args [i], 8);
if (bAutoPlr) {
	char *psz;
	strlwr (filename);
	if (!(psz = strchr (filename, '.')))
		for (psz = filename; psz - filename < 8; psz++)
			if (!*psz || isspace (*psz))
				break;
		*psz = '\0';
	goto got_player;
	}

do_menu_again:

bStartup = 0;
sprintf (filespec, "%s%s*.plr", gameFolders.szProfDir, *gameFolders.szProfDir ? "/" : ""); 
if (!ExecMenuFileSelector (TXT_SELECT_PILOT, filespec, filename, allow_abort_flag))	{
	if (allow_abort_flag) {
		return 0;
		}
	goto do_menu_again; //return 0;		// They hit Esc in file selector
	}
got_player:
bStartup = 0;
if (filename [0] == '<')	{
	// They selected 'create new pilot'
	if (!MakeNewPlayerFile (1))
		//return 0;		// They hit Esc during enter name stage
		goto do_menu_again;
	}
else
	strncpy (gameData.multi.players [gameData.multi.nLocalPlayer].callsign, filename, CALLSIGN_LEN);
if (ReadPlayerFile (0) != EZERO)
	goto do_menu_again;
KCSetControls ();
gameOpts->gameplay.bAutoLeveling = gameOpts->gameplay.bDefaultLeveling;
SetDisplayMode (gameStates.video.nDefaultDisplayMode);
WriteConfigFile ();		// Update lastplr
D2SetCaption ();
return 1;
}


int LoadRobotReplacements (char *pszLevelName, int bAddBots);
int ReadHamFile ();
int network_verify_objects (int nRemoteObjNum, int nLocalObjs);

//------------------------------------------------------------------------------
//load a level off disk. level numbers start at 1.  Secret levels are -1,-2,-3

extern char szAutoMission [255];

int LoadLevel (int nLevel, int bPageInTextures)
{
	char		*pszLevelName;
	player	save_player;
	int		nRooms, load_ret;

/*---*/LogErr ("Loading level...\n");
gameStates.app.bGameRunning = 0;
#if 1
/*---*/LogErr ("   unloading textures\n");
PiggyBitmapPageOutAll (0);
/*---*/LogErr ("   unloading cambot\n");
UnloadCamBot ();
/*---*/LogErr ("   unloading additional models\n");
BMFreeExtraModels ();
/*---*/LogErr ("   unloading additional model textures\n");
BMFreeExtraObjBitmaps ();
/*---*/LogErr ("   unloading additional model textures\n");
PiggyFreeHiresAnimations ();
/*---*/LogErr ("   restoring default robot settings\n");
RestoreDefaultRobots ();
if (gameData.bots.bReplacementsLoaded) {
	/*---*/LogErr ("   loading default robot settings\n");
	ReadHamFile ();		//load original data
	gameData.bots.bReplacementsLoaded = 0;
	}
if (gameData.missions.nEnhancedMission) {
	char t [FILENAME_LEN];

	sprintf (t,"%s.ham", gameStates.app.szCurrentMissionFile);
	/*---*/LogErr ("   reading additional robots\n");
	if (!BMReadExtraRobots (t, gameFolders.szMissionDirs [0], gameData.missions.nEnhancedMission))
		BMReadExtraRobots ("d2x.ham", gameFolders.szMissionDir, gameData.missions.nEnhancedMission);
	strncpy (t, gameStates.app.szCurrentMissionFile, 6);
	strcat (t, "-l.mvl");
	/*---*/LogErr ("   initializing additional robot movies\n");
	InitExtraRobotMovie (t);
	}
#endif
/*---*/LogErr ("   Destroying camera objects\n");
DestroyCameras ();
/*---*/LogErr ("   Destroying smoke objects\n");
DestroyAllSmoke ();
/*---*/LogErr ("   Initializing smoke manager\n");
InitObjectSmoke ();
save_player = gameData.multi.players [gameData.multi.nLocalPlayer];	
Assert (gameStates.app.bAutoRunMission || ((nLevel <= gameData.missions.nLastLevel)  && (nLevel >= gameData.missions.nLastSecretLevel)  && (nLevel != 0)));
pszLevelName = gameStates.app.bAutoRunMission ? szAutoMission : (nLevel < 0) ? gameData.missions.szSecretLevelNames [-nLevel-1] : gameData.missions.szLevelNames [nLevel-1];
strlwr (pszLevelName);
/*---*/LogErr ("   loading level '%s'\n", pszLevelName);
#ifdef WINDOWS
	DDGrSetCurrentCanvas (NULL);
	dd_gr_clear_canvas (0);
#else
	GrSetCurrentCanvas (NULL);
	GrClearCanvas (BLACK_RGBA);		//so palette switching is less obvious
#endif
nLastMsgYCrd = -1;		//so we don't restore backgound under msg
/*---*/LogErr ("   loading palette\n");
#if 1 //defined (POLY_ACC) || defined (OGL)
GrPaletteStepLoad (NULL);
 //LoadPalette ("groupa.256", NULL, 0, 0, 1);		//don't change screen
ShowBoxedMessage (TXT_LOADING);
#else
ShowBoxedMessage (TXT_LOADING);
GrPaletteStepLoad (NULL);
#endif
/*---*/LogErr ("   loading level data\n");
gameStates.app.bD1Mission = gameStates.app.bAutoRunMission ? (strstr (szAutoMission, "rdl") != NULL) :
				 (gameData.missions.list [gameData.missions.nCurrentMission].descent_version == 1);
memset (gameData.segs.xSegments, 0xff, sizeof (gameData.segs.xSegments));
load_ret = LoadLevelSub (pszLevelName);		//actually load the data from disk!
if (load_ret) {
	/*---*/LogErr ("Couldn't load '%s' (%d)\n", pszLevelName, load_ret);
	Warning (TXT_LOAD_ERROR, pszLevelName);
	return 0;
	}
gameData.missions.nCurrentLevel=nLevel;
gamePalette = LoadPalette (szCurrentLevelPalette, pszLevelName, 1, 1, 1);		//don't change screen
InitGaugeCanvases ();
ResetPogEffects ();
if (gameStates.app.bD1Mission) {
	/*---*/LogErr ("   loading Descent 1 textures\n");
	LoadD1BitmapReplacements ();
	if (bPageInTextures)
		PiggyLoadLevelData ();
	}
else {
	if (bPageInTextures)
		PiggyLoadLevelData ();
	LoadBitmapReplacements (pszLevelName);
	}
/*---*/LogErr ("   loading texture brightness info\n");
LoadTextureBrightness (pszLevelName);
/*---*/LogErr ("   loading endlevel data\n");
LoadEndLevelData (nLevel);
/*---*/LogErr ("   loading cambot\n");
gameData.bots.nCamBotId = LoadRobotReplacements ("cambot.hxm", 1) ? gameData.bots.nTypes [0] - 1 : -1;
gameData.bots.nCamBotModel = gameData.models.nPolyModels - 1;
/*---*/LogErr ("   loading replacement robots\n");
LoadRobotReplacements (pszLevelName, 0);
/*---*/LogErr ("   initializing cambot\n");
InitCamBots (0);
#ifdef NETWORK
networkData.nMySegsCheckSum = NetMiscCalcCheckSum (gameData.segs.segments, sizeof (segment)* (gameData.segs.nLastSegment+1));
ResetNetworkObjects ();
ResetChildObjects ();
ResetMovementPath ();
/*---*/LogErr ("   counting entropy rooms\n");
nRooms = CountRooms ();
if (gameData.app.nGameMode & GM_ENTROPY) {
	if (!nRooms) {
		Warning (TXT_NO_ENTROPY);
		gameData.app.nGameMode &= ~GM_ENTROPY;
		gameData.app.nGameMode |= GM_TEAM;
		}
	}
else if ((gameData.app.nGameMode & (GM_CAPTURE | GM_HOARD)) || 
			((gameData.app.nGameMode & GM_MONSTERBALL) == GM_MONSTERBALL)) {
/*---*/LogErr ("   gathering CTF+ flag goals\n");
	if (GatherFlagGoals () != 3) {
		Warning (TXT_NO_CTF);
		gameData.app.nGameMode &= ~GM_CAPTURE;
		gameData.app.nGameMode |= GM_TEAM;
		}
	}
memset (gameData.render.lights.segDeltas, 0, sizeof (gameData.render.lights.segDeltas));
/*---*/LogErr ("   initializing door animations\n");
InitDoorAnims ();
#endif
gameData.multi.players [gameData.multi.nLocalPlayer] = save_player;
gameData.hoard.nMonsterballSeg = -1;
/*---*/LogErr ("   initializing sound sources\n");
SetSoundSources ();
PlayLevelSong (gameData.missions.nCurrentLevel);
ClearBoxedMessage ();		//remove message before new palette loaded
GrPaletteStepLoad (NULL);		//actually load the palette
#ifdef OGL
/*---*/LogErr ("   rebuilding OpenGL texture data\n");
/*---*/LogErr ("      rebuilding effects\n");
RebuildGfxFx (1, 1);
ResetPingStats ();
gameStates.gameplay.nDirSteps = 0;
gameStates.render.bAllVisited = 0;
gameStates.render.bViewDist = 1;
gameStates.render.bHaveSkyBox = -1;
gameStates.app.cheats.nUnlockLevel = 0;
if (!gameStates.render.bHaveStencilBuffer)
	extraGameInfo [0].bShadows = 0;
#endif
//	WIN (HideCursorW ();
D2SetCaption ();
return 1;
}

//------------------------------------------------------------------------------

//sets up gameData.multi.nLocalPlayer & gameData.objs.console
void InitMultiPlayerObject ()
{
Assert ((gameData.multi.nLocalPlayer >= 0) && (gameData.multi.nLocalPlayer < MAX_PLAYERS));
if (gameData.multi.nLocalPlayer != 0)	{
	gameData.multi.players [0] = gameData.multi.players [gameData.multi.nLocalPlayer];
	gameData.multi.nLocalPlayer = 0;
	}
gameData.multi.players [gameData.multi.nLocalPlayer].objnum = 0;
gameData.multi.players [gameData.multi.nLocalPlayer].nInvuls =
gameData.multi.players [gameData.multi.nLocalPlayer].nCloaks = 0;
gameData.objs.console = gameData.objs.objects + gameData.multi.players [gameData.multi.nLocalPlayer].objnum;
gameData.objs.console->type = OBJ_PLAYER;
gameData.objs.console->id = gameData.multi.nLocalPlayer;
gameData.objs.console->control_type	= CT_FLYING;
gameData.objs.console->movement_type = MT_PHYSICS;
gameStates.entropy.nTimeLastMoved = -1;
}

//------------------------------------------------------------------------------

extern void GameDisableCheats ();
extern void TurnCheatsOff ();
extern void InitSeismicDisturbances (void);

//starts a new game on the given level
int StartNewGame (int nStartLevel)
{
	int result;

gameData.app.nGameMode = GM_NORMAL;
SetFunctionMode (FMODE_GAME);
gameData.missions.nNextLevel = 0;
InitMultiPlayerObject ();				//make sure player's object set up
InitPlayerStatsGame ();		//clear all stats
gameData.multi.nPlayers = 1;
#ifdef NETWORK
networkData.bNewGame = 0;
#endif
if (nStartLevel < 0)
	result = StartNewLevelSecret (nStartLevel, 0);
else
	result = StartNewLevel (nStartLevel, 0);
if (result) {
	gameData.multi.players [gameData.multi.nLocalPlayer].starting_level = nStartLevel;		// Mark where they started
	GameDisableCheats ();
	InitSeismicDisturbances ();
	}
return result;
}

//@@//starts a resumed game loaded from disk
//@@void ResumeSavedGame (int nStartLevel)
//@@{
//@@	gameData.app.nGameMode = GM_NORMAL;
//@@	SetFunctionMode (FMODE_GAME);
//@@
//@@	gameData.multi.nPlayers = 1;
//@@	networkData.bNewGame = 0;
//@@
//@@	InitPlayerObject ();				//make sure player's object set up
//@@
//@@	StartNewLevel (nStartLevel, 0);
//@@
//@@	GameDisableCheats ();
//@@}

//------------------------------------------------------------------------------

#ifndef _NETWORK_H
extern int NetworkEndLevelPoll2 (int nitems, newmenu_item * menus, int * key, int citem); // network.c
#endif

//	Does the bonus scoring.
//	Call with dead_flag = 1 if player died, but deserves some portion of bonus (only skill points), anyway.
void DoEndLevelScoreGlitz (int network)
{
	int level_points, skill_points, energy_points, shield_points, hostage_points;
	int	all_hostage_points;
	int	endgame_points;
	char	all_hostage_text [64];
	char	endgame_text [64];
	#define N_GLITZITEMS 11
	char				m_str [N_GLITZITEMS] [30];
	newmenu_item	m [N_GLITZITEMS+1];
	int				i,c;
	char				title [128];
	int				is_last_level;
	int				mine_level;

	SetScreenMode (SCREEN_MENU);		//go into menu mode
	if (gameStates.app.bHaveExtraData)
		songs_play_song (SONG_INTER, 0);
	
   #ifdef TACTILE
		if (TactileStick)
		  ClearForces ();
	#endif

	//	Compute level player is on, deal with secret levels (negative numbers)
mine_level = gameData.multi.players [gameData.multi.nLocalPlayer].level;
if (mine_level < 0)
	mine_level *= - (gameData.missions.nLastLevel/gameData.missions.nSecretLevels);
level_points = gameData.multi.players [gameData.multi.nLocalPlayer].score-gameData.multi.players [gameData.multi.nLocalPlayer].last_score;
if (!gameStates.app.cheats.bEnabled) {
	if (gameStates.app.nDifficultyLevel > 1) {
		skill_points = level_points* (gameStates.app.nDifficultyLevel)/4;
		skill_points -= skill_points % 100;
		}
	else
		skill_points = 0;
	shield_points = f2i (gameData.multi.players [gameData.multi.nLocalPlayer].shields) * 5 * mine_level;
	energy_points = f2i (gameData.multi.players [gameData.multi.nLocalPlayer].energy) * 2 * mine_level;
	hostage_points = gameData.multi.players [gameData.multi.nLocalPlayer].hostages_on_board * 500 * (gameStates.app.nDifficultyLevel+1);
	shield_points -= shield_points % 50;
	energy_points -= energy_points % 50;
	}
else {
	skill_points = 0;
	shield_points = 0;
	energy_points = 0;
	hostage_points = 0;
	}
all_hostage_text [0] = 0;
endgame_text [0] = 0;
if (!gameStates.app.cheats.bEnabled && (gameData.multi.players [gameData.multi.nLocalPlayer].hostages_on_board == gameData.multi.players [gameData.multi.nLocalPlayer].hostages_level)) {
	all_hostage_points = gameData.multi.players [gameData.multi.nLocalPlayer].hostages_on_board * 1000 * (gameStates.app.nDifficultyLevel+1);
	sprintf (all_hostage_text, "%s%i\n", TXT_FULL_RESCUE_BONUS, all_hostage_points);
	}
else
	all_hostage_points = 0;
if (!gameStates.app.cheats.bEnabled && !(gameData.app.nGameMode & GM_MULTI) && (gameData.multi.players [gameData.multi.nLocalPlayer].lives) && (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel)) {		//player has finished the game!
	endgame_points = gameData.multi.players [gameData.multi.nLocalPlayer].lives * 10000;
	sprintf (endgame_text, "%s%i\n", TXT_SHIP_BONUS, endgame_points);
	is_last_level=1;
	}
else
	endgame_points = is_last_level = 0;
AddBonusPointsToScore (skill_points + energy_points + shield_points + hostage_points + all_hostage_points + endgame_points);
c = 0;
sprintf (m_str [c++], "%s%i", TXT_SHIELD_BONUS, shield_points);		// Return at start to lower menu...
sprintf (m_str [c++], "%s%i", TXT_ENERGY_BONUS, energy_points);
sprintf (m_str [c++], "%s%i", TXT_HOSTAGE_BONUS, hostage_points);
sprintf (m_str [c++], "%s%i", TXT_SKILL_BONUS, skill_points);
sprintf (m_str [c++], "%s", all_hostage_text);
if (!(gameData.app.nGameMode & GM_MULTI) && (gameData.multi.players [gameData.multi.nLocalPlayer].lives) && (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel))
	sprintf (m_str [c++], "%s", endgame_text);
sprintf (m_str [c++], "%s%i\n", TXT_TOTAL_BONUS, shield_points+energy_points+hostage_points+skill_points+all_hostage_points+endgame_points);
sprintf (m_str [c++], "%s%i", TXT_TOTAL_SCORE, gameData.multi.players [gameData.multi.nLocalPlayer].score);
#ifdef WINDOWS
sprintf (m_str [c++], "");
sprintf (m_str [c++], "         Done");
#endif
memset (m, 0, sizeof (m));
for (i=0; i<c; i++) {
	m [i].type = NM_TYPE_TEXT;
	m [i].text = m_str [i];
	}
#ifdef WINDOWS
m [c-1].type = NM_TYPE_MENU;
#endif
// m [c].type = NM_TYPE_MENU;	m [c++].text = "Ok";
sprintf (title,
			"%s%s %d %s\n%s %s",
			gameOpts->menus.nStyle ? "" : is_last_level ? "\n\n\n":"\n",
			 (gameData.missions.nCurrentLevel < 0) ? TXT_SECRET_LEVEL : TXT_LEVEL, 
			 (gameData.missions.nCurrentLevel < 0) ? -gameData.missions.nCurrentLevel : gameData.missions.nCurrentLevel, 
			TXT_COMPLETE, 
			gameData.missions.szCurrentLevel, 
			TXT_DESTROYED);
Assert (c <= N_GLITZITEMS);
GrPaletteFadeOut (NULL, 32, 0);
PA_DFX (pa_alpha_always ());
#ifdef NETWORK
if (network && (gameData.app.nGameMode & GM_NETWORK))
	ExecMenu2 (NULL, title, c, m, (void (*))NetworkEndLevelPoll2, 0, STARS_BACKGROUND);
else
#endif
// NOTE LINK TO ABOVE!!!
gameStates.app.bGameRunning = 0;
ExecMenu2 (NULL, title, c, m, NULL, 0, STARS_BACKGROUND);
}

//	-----------------------------------------------------------------------------------------------------

//give the player the opportunity to save his game
void DoEndlevelMenu ()
{
//No between level saves......!!!	StateSaveAll (1);
}

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
void StartSecretLevel ()
{
Assert (!gameStates.app.bPlayerIsDead);
InitPlayerPosition (0);
verify_console_object ();
gameData.objs.console->control_type	= CT_FLYING;
gameData.objs.console->movement_type	= MT_PHYSICS;
// -- WHY? -- DisableMatCens ();
clear_transient_objects (0);		//0 means leave proximity bombs
// CreatePlayerAppearanceEffect (gameData.objs.console);
gameStates.render.bDoAppearanceEffect = 1;
ai_reset_all_paths ();
ResetTime ();
ResetRearView ();
gameData.app.fusion.xAutoFireTime = 0;
gameData.app.fusion.xCharge = 0;
gameStates.app.cheats.bRobotsFiring = 1;
gameOpts->sound.bD1Sound = gameStates.app.bD1Mission && gameStates.app.bHaveD1Data && gameOpts->sound.bUseD1Sounds;
}

//------------------------------------------------------------------------------

extern void SetPosFromReturnSegment (void);

//	Returns true if secret level has been destroyed.
int p_secret_level_destroyed (void)
{
if (gameStates.app.bFirstSecretVisit)
	return 0;		//	Never been there, can't have been destroyed.
if (CFExist (SECRETC_FILENAME,gameFolders.szSaveDir,0))
	return 0;
return 1;
}

//	-----------------------------------------------------------------------------------------------------

void DoSecretMessage (char *msg)
{
	int	old_fmode;

#if defined (POLY_ACC)
	pa_save_clut ();
	pa_update_clut (grPalette, 0, 256, 0);
#endif

old_fmode = gameStates.app.nFunctionMode;
StopTime ();
SetFunctionMode (FMODE_MENU);
ExecMessageBox (NULL, STARS_BACKGROUND, 1, TXT_OK, msg);
SetFunctionMode (old_fmode);
StartTime ();
#if defined (POLY_ACC)
pa_restore_clut ();
#endif
WIN (DEFINE_SCREEN (NULL));
}

//	-----------------------------------------------------------------------------------------------------
// called when the player is starting a new level for normal game mode and restore state
//	Need to deal with whether this is the first time coming to this level or not.  If not the
//	first time, instead of initializing various things, need to do a game restore for all the
//	robots, powerups, walls, doors, etc.
int StartNewLevelSecret (int nLevel, int bPageInTextures)
{
	newmenu_item	m [1];
  //int i;

ThisLevelTime=0;

m [0].type = NM_TYPE_TEXT;
m [0].text = " ";

last_drawn_cockpit [0] = -1;
last_drawn_cockpit [1] = -1;

if (gameData.demo.nState == ND_STATE_PAUSED)
	gameData.demo.nState = ND_STATE_RECORDING;

if (gameData.demo.nState == ND_STATE_RECORDING) {
	NDSetNewLevel (nLevel);
	NDRecordStartFrame (gameData.app.nFrameCount, gameData.app.xFrameTime);
	} 
else if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	GrPaletteFadeOut (NULL, 32, 0);
	SetScreenMode (SCREEN_MENU);		//go into menu mode
	if (gameStates.app.bFirstSecretVisit)
		DoSecretMessage (gameStates.app.bD1Mission ? TXT_ALTERNATE_EXIT : TXT_SECRET_EXIT);
	else if (CFExist (SECRETC_FILENAME,gameFolders.szSaveDir,0))
		DoSecretMessage (gameStates.app.bD1Mission ? TXT_ALTERNATE_EXIT : TXT_SECRET_EXIT);
	else {
		char	text_str [128];

		sprintf (text_str, TXT_ADVANCE_LVL, gameData.missions.nCurrentLevel+1);
		DoSecretMessage (text_str);
		}
	}
if (!LoadLevel (nLevel,bPageInTextures))
	return 0;
Assert (gameData.missions.nCurrentLevel == nLevel);	//make sure level set right
Assert (gameStates.app.nFunctionMode == FMODE_GAME);
GameSeqInitNetworkPlayers (); // Initialize the gameData.multi.players array for this level
HUDClearMessages ();
AutomapClearVisited ();
// --	init_player_stats_level ();
gameData.objs.viewer = gameData.objs.objects + gameData.multi.players [gameData.multi.nLocalPlayer].objnum;
gameseq_remove_unused_players ();
gameStates.app.bGameSuspended = 0;
gameData.reactor.bDestroyed = 0;
InitCockpit ();
ResetPaletteAdd ();

if (gameStates.app.bFirstSecretVisit || (gameData.demo.nState == ND_STATE_PLAYBACK)) {
	if (!gameStates.app.bAutoRunMission && gameStates.app.bD1Mission)
		ShowLevelIntro (nLevel);
	PlayLevelSong (gameData.missions.nCurrentLevel);
	InitRobotsForLevel ();
	InitAIObjects ();
	InitShakerDetonates ();
	MorphInit ();
	InitAllMatCens ();
	ResetSpecialEffects ();
	StartSecretLevel ();
	gameData.multi.players [gameData.multi.nLocalPlayer].flags &= ~PLAYER_FLAGS_ALL_KEYS;
	}
else {
	if (CFExist (SECRETC_FILENAME,gameFolders.szSaveDir,0)) {
		int	pw_save, sw_save;

		pw_save = gameData.weapons.nPrimary;
		sw_save = gameData.weapons.nSecondary;
		StateRestoreAll (1, 1, SECRETC_FILENAME);
		gameData.weapons.nPrimary = pw_save;
		gameData.weapons.nSecondary = sw_save;
		ResetSpecialEffects ();
		StartSecretLevel ();
		// -- No: This is only for returning to base level: SetPosFromReturnSegment ();
		}
	else {
		char	text_str [128];

		sprintf (text_str, TXT_ADVANCE_LVL, gameData.missions.nCurrentLevel+1);
		DoSecretMessage (text_str);
		return 1;

		// -- //	If file doesn't exist, it's because reactor was destroyed.
		// -- // -- StartNewLevel (gameData.missions.secretLevelTable [-gameData.missions.nCurrentLevel-1]+1, 0);
		// -- StartNewLevel (gameData.missions.secretLevelTable [-gameData.missions.nCurrentLevel-1]+1, 0);
		// -- return;
		}
	}

if (gameStates.app.bFirstSecretVisit)
	copy_defaults_to_robot_all ();
TurnCheatsOff ();
InitReactorForLevel ();
//	Say player can use FLASH cheat to mark path to exit.
nLastLevelPathCreated = -1;
gameStates.app.bFirstSecretVisit = 0;
return 1;
}

//------------------------------------------------------------------------------

//	Called from switch.c when player is on a secret level and hits exit to return to base level.
void ExitSecretLevel (void)
{
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return;
if (!gameData.reactor.bDestroyed)
	StateSaveAll (0, 2, SECRETC_FILENAME);
if (CFExist (SECRETB_FILENAME, gameFolders.szSaveDir,0)) {
	int	pw_save, sw_save;

	pw_save = gameData.weapons.nPrimary;
	sw_save = gameData.weapons.nSecondary;
	if (gameStates.app.bD1Mission)
		DoEndLevelScoreGlitz (0);
	else
		ReturningToLevelMessage ();
	StateRestoreAll (1, 1, SECRETB_FILENAME);
	gameOpts->sound.bD1Sound = gameStates.app.bD1Mission && gameStates.app.bHaveD1Data && gameOpts->sound.bUseD1Sounds;
	if (gameStates.app.bD1Mission) {
		AdvanceLevel (0, 1);			//if finished, go on to next level
#if 0
		gameData.app.bGamePaused = 1;
		ReturningToLevelMessage ();
		gameData.app.bGamePaused = 0;
#endif
		}
	SetDataVersion (-1);
	gameData.weapons.nPrimary = pw_save;
	gameData.weapons.nSecondary = sw_save;
	}
else {
	// File doesn't exist, so can't return to base level.  Advance to next one.
	if (gameData.missions.nEnteredFromLevel == gameData.missions.nLastLevel)
		DoEndGame ();
	else {
		AdvancingToLevelMessage ();
		StartNewLevel (gameData.missions.nEnteredFromLevel + 1, 0);
		}
	}
}

//------------------------------------------------------------------------------
//	Set invulnerable_time and cloak_time in player struct to preserve amount of time left to
//	be invulnerable or cloaked.
void DoCloakInvulSecretStuff (fix old_gametime)
{
if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE) {
		fix	time_used;

	time_used = old_gametime - gameData.multi.players [gameData.multi.nLocalPlayer].invulnerable_time;
	gameData.multi.players [gameData.multi.nLocalPlayer].invulnerable_time = gameData.app.xGameTime - time_used;
	}

if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
		fix	time_used;

	time_used = old_gametime - gameData.multi.players [gameData.multi.nLocalPlayer].cloak_time;
	gameData.multi.players [gameData.multi.nLocalPlayer].cloak_time = gameData.app.xGameTime - time_used;
	}
}

//------------------------------------------------------------------------------
//	Called from switch.c when player passes through secret exit.  That means he was on a non-secret level and he
//	is passing to the secret level.
//	Do a savegame.
void EnterSecretLevel (void)
{
	fix	old_gametime;
	int	i;

Assert (!(gameData.app.nGameMode & GM_MULTI));
gameData.missions.nEnteredFromLevel = gameData.missions.nCurrentLevel;
if (gameData.reactor.bDestroyed)
	DoEndLevelScoreGlitz (0);
if (gameData.demo.nState != ND_STATE_PLAYBACK)
	StateSaveAll (0, 1, NULL);	//	Not between levels (ie, save all), IS a secret level, NO filename override
//	Find secret level number to go to, stuff in gameData.missions.nNextLevel.
for (i = 0; i < -gameData.missions.nLastSecretLevel; i++)
	if (gameData.missions.secretLevelTable [i]==gameData.missions.nCurrentLevel) {
		gameData.missions.nNextLevel = - (i+1);
		break;
		} 
	else if (gameData.missions.secretLevelTable [i] > gameData.missions.nCurrentLevel) {	//	Allows multiple exits in same group.
		gameData.missions.nNextLevel = -i;
		break;
		}
if (i >= -gameData.missions.nLastSecretLevel)		//didn't find level, so must be last
	gameData.missions.nNextLevel = gameData.missions.nLastSecretLevel;
old_gametime = gameData.app.xGameTime;
StartNewLevelSecret (gameData.missions.nNextLevel, 1);
// do_cloak_invul_stuff ();
}

//------------------------------------------------------------------------------
//called when the player has finished a level
void PlayerFinishedLevel (int bSecret)
{
	Assert (!bSecret);

	//credit the player for hostages
gameData.multi.players [gameData.multi.nLocalPlayer].hostages_rescued_total += gameData.multi.players [gameData.multi.nLocalPlayer].hostages_on_board;
if (gameData.app.nGameMode & GM_NETWORK)
	gameData.multi.players [gameData.multi.nLocalPlayer].connected = 2; // Finished but did not die
last_drawn_cockpit [0] = -1;
last_drawn_cockpit [1] = -1;
if (gameData.missions.nCurrentLevel < 0)
	ExitSecretLevel ();
else
	AdvanceLevel (0, 0);				//now go on to the next one (if one)
}

//------------------------------------------------------------------------------
#if defined (D2_OEM) || defined (COMPILATION)
#define MOVIE_REQUIRED 0
#else
#define MOVIE_REQUIRED 1
#endif

#ifdef D2_OEM
#	define ENDMOVIE		"endo"
#else
#	define ENDMOVIE		"end"
#	define D1_ENDMOVIE	"final"
#endif

void show_order_form ();
extern void com_hangup (void);

//called when the player has finished the last level
void DoEndGame (void)
{
SetFunctionMode (FMODE_MENU);
if ((gameData.demo.nState == ND_STATE_RECORDING) || (gameData.demo.nState == ND_STATE_PAUSED))
	NDStopRecording ();

SetScreenMode (SCREEN_MENU);

WINDOS (
	DDGrSetCurrentCanvas (NULL),
	GrSetCurrentCanvas (NULL)
);

KeyFlush ();

if (!(gameData.app.nGameMode & GM_MULTI)) {
	if (gameData.missions.nCurrentMission == (gameStates.app.bD1Mission ? gameData.missions.nD1BuiltinMission : gameData.missions.nBuiltinMission)) {
		int played=MOVIE_NOT_PLAYED;	//default is not played

		if (!gameStates.app.bD1Mission) {
			InitSubTitles (ENDMOVIE ".tex");	
			played = PlayMovie (ENDMOVIE, MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
			CloseSubTitles ();
			}
		else if (gameStates.app.bHaveExtraMovies) {
			//InitSubTitles (ENDMOVIE ".tex");	//ingore errors
			played = PlayMovie (D1_ENDMOVIE, MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
			CloseSubTitles ();
			}
		if (!played) {
			if (IS_D2_OEM) {
				songs_play_song (SONG_TITLE, 0);
				DoBriefingScreens ("end2oem.tex",1);
				}
			else {
				songs_play_song (SONG_ENDGAME, 0);
				DoBriefingScreens (gameStates.app.bD1Mission ? "endreg.tex" : "ending2.tex", gameStates.app.bD1Mission ? 0x7e : 1);
				}
			}
		}
	else {    //not multi
		char tname [FILENAME_LEN];
		sprintf (tname,"%s.tex",gameStates.app.szCurrentMissionFile);
		DoBriefingScreens (tname,gameData.missions.nLastLevel+1);   //level past last is endgame breifing

		//try doing special credits
		sprintf (tname,"%s.ctb",gameStates.app.szCurrentMissionFile);
		credits_show (tname);
		}
	}
KeyFlush ();

#ifdef SHAREWARE
show_order_form ();
#endif

#ifdef NETWORK
if (gameData.app.nGameMode & GM_MULTI)
	MultiEndLevelScore ();
else
#endif
	// NOTE LINK TO ABOVE
	DoEndLevelScoreGlitz (0);

if (gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission && !((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP))) {
	WINDOS (
		DDGrSetCurrentCanvas (NULL),
		GrSetCurrentCanvas (NULL)
	);
	WINDOS (
		dd_gr_clear_canvas (BLACK_RGBA),
		GrClearCanvas (BLACK_RGBA)
	);
	GrPaletteStepClear ();
	//LoadPalette (D2_DEFAULT_PALETTE, NULL, 0, 1, 0);
	scores_maybe_add_player (0);
	}
SetFunctionMode (FMODE_MENU);
if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM))
	gameData.app.nGameMode |= GM_GAME_OVER;		//preserve modem setting so go back into modem menu
else
	gameData.app.nGameMode = GM_GAME_OVER;
longjmp (gameExitPoint, 0);		// Exit out of game loop
}

//------------------------------------------------------------------------------
//from which level each do you get to each secret level
//called to go to the next level (if there is one)
//if bSecret is true, advance to secret level, else next normal one
//	Return true if game over.
void AdvanceLevel (int bSecret, int bFromSecret)
{
#ifdef NETWORK
	int result;
#endif

Assert (!bSecret);
if ((!bFromSecret && gameStates.app.bD1Mission) &&
	 ((gameData.missions.nCurrentLevel != gameData.missions.nLastLevel) || 
	  extraGameInfo [IsMultiGame].bRotateLevels)) {
#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI)
		MultiEndLevelScore ();		
	else
#endif
	// NOTE LINK TO ABOVE!!!
	DoEndLevelScoreGlitz (0);		//give bonuses
	}
gameData.reactor.bDestroyed = 0;
#ifdef EDITOR
if (gameData.missions.nCurrentLevel == 0)
	return;		//not a real level
#endif
#ifdef NETWORK
if (gameData.app.nGameMode & GM_MULTI)	{
	result = MultiEndLevel (&bSecret); // Wait for other players to reach this point
	if (result) { // failed to sync
		if (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel)		//player has finished the game!
			longjmp (gameExitPoint, 0);		// Exit out of game loop
		else
			return;
		}
	}
#endif
if ((gameData.missions.nCurrentLevel == gameData.missions.nLastLevel) && 
	!extraGameInfo [IsMultiGame].bRotateLevels) //player has finished the game!
	DoEndGame ();
else {
	gameData.missions.nNextLevel = gameData.missions.nCurrentLevel + 1;		//assume go to next normal level
	if (gameData.missions.nNextLevel > gameData.missions.nLastLevel)
		gameData.missions.nNextLevel = gameData.missions.nLastLevel;
	if (!(gameData.app.nGameMode & GM_MULTI))
		DoEndlevelMenu (); // Let user save their game
	StartNewLevel (gameData.missions.nNextLevel, 0);
	}
}

//------------------------------------------------------------------------------

void LoadStars (bkg *bg, int bRedraw)
{
//@@	int pcx_error;
//@@	ubyte pal [256*3];
//@@
//@@	pcx_error = pcx_read_bitmap ("STARS.PCX",&grdCurCanv->cv_bitmap,grdCurCanv->cv_bitmap.bm_props.type,pal);
//@@	Assert (pcx_error == PCX_ERROR_NONE);
//@@
//@@	GrRemapBitmapGood (&grdCurCanv->cv_bitmap, pal, -1, -1);

WIN (DEFINE_SCREEN (STARS_BACKGROUND));
NMLoadBackground (STARS_BACKGROUND, bg, bRedraw);
starsPalette = gameData.render.pal.pCurPal;
}

//------------------------------------------------------------------------------

void DiedInMineMessage (void)
{
	// Tell the player he died in the mine, explain why
	int old_fmode;

	if (gameData.app.nGameMode & GM_MULTI)
		return;
	GrPaletteFadeOut (NULL, 32, 0);
	SetScreenMode (SCREEN_MENU);		//go into menu mode
	WINDOS (
		DDGrSetCurrentCanvas (NULL),
		GrSetCurrentCanvas (NULL)
	);
#if defined (POLY_ACC)
	pa_save_clut ();
	pa_update_clut (grPalette, 0, 256, 0);
#endif

	old_fmode = gameStates.app.nFunctionMode;
	SetFunctionMode (FMODE_MENU);
	ExecMessageBox (NULL, STARS_BACKGROUND, 1, TXT_OK, TXT_DIED_IN_MINE);
	SetFunctionMode (old_fmode);

#if defined (POLY_ACC)
	pa_restore_clut ();
#endif

	WIN (DEFINE_SCREEN (NULL));
}

//------------------------------------------------------------------------------
//	Called when player dies on secret level.
void ReturningToLevelMessage (void)
{
	char	msg [128];

	int old_fmode;

if (gameData.app.nGameMode & GM_MULTI)
	return;
StopTime ();
GrPaletteFadeOut (NULL, 32, 0);
SetScreenMode (SCREEN_MENU);		//go into menu mode
GrSetCurrentCanvas (NULL);
#if defined (POLY_ACC)
pa_save_clut ();
pa_update_clut (grPalette, 0, 256, 0);
#endif
old_fmode = gameStates.app.nFunctionMode;
SetFunctionMode (FMODE_MENU);
if (gameData.missions.nEnteredFromLevel < 0)
	sprintf (msg, TXT_SECRET_LEVEL_RETURN);
else
	sprintf (msg, TXT_RETURN_LVL, gameData.missions.nEnteredFromLevel);
ExecMessageBox (NULL, STARS_BACKGROUND, 1, TXT_OK, msg);
SetFunctionMode (old_fmode);
#if defined (POLY_ACC)
pa_restore_clut ();
#endif
StartTime ();
WIN (DEFINE_SCREEN (NULL));
}

//------------------------------------------------------------------------------
//	Called when player dies on secret level.
void AdvancingToLevelMessage (void)
{
	char	msg [128];

	int old_fmode;

	//	Only supposed to come here from a secret level.
	Assert (gameData.missions.nCurrentLevel < 0);

	if (gameData.app.nGameMode & GM_MULTI)
		return;
	GrPaletteFadeOut (NULL, 32, 0);
	SetScreenMode (SCREEN_MENU);		//go into menu mode
	GrSetCurrentCanvas (NULL);
#if defined (POLY_ACC)
	pa_save_clut ();
	pa_update_clut (grPalette, 0, 256, 0);
#endif

	old_fmode = gameStates.app.nFunctionMode;
	SetFunctionMode (FMODE_MENU);
	sprintf (msg, "Base level destroyed.\nAdvancing to level %i", gameData.missions.nEnteredFromLevel+1);
	ExecMessageBox (NULL, STARS_BACKGROUND, 1, TXT_OK, msg);
	SetFunctionMode (old_fmode);

#if defined (POLY_ACC)
	pa_restore_clut ();
#endif

	WIN (DEFINE_SCREEN (NULL));
}

//------------------------------------------------------------------------------

void DigiStopDigiSounds ();

void DoPlayerDead ()
{
gameStates.app.bGameRunning = 0;
ResetPaletteAdd ();
GrPaletteStepLoad (NULL);
//	DigiPauseDigiSounds ();		//kill any continuing sounds (eg. forcefield hum)
DigiStopDigiSounds ();		//kill any continuing sounds (eg. forcefield hum)
DeadPlayerEnd ();		//terminate death sequence (if playing)
if (gameStates.multi.bPlayerIsTyping [gameData.multi.nLocalPlayer] && (gameData.app.nGameMode & GM_MULTI))
	MultiSendMsgQuit ();
gameStates.entropy.bConquering = 0;
#ifdef EDITOR
if (gameData.app.nGameMode == GM_EDITOR) {			//test mine, not real level
	object * playerobj = gameData.objs.objects + gameData.multi.players [gameData.multi.nLocalPlayer].objnum;
	//ExecMessageBox ("You're Dead!", 1, "Continue", "Not a real game, though.");
	LoadLevelSub ("gamesave.lvl");
	InitPlayerStatsNewShip ();
	playerobjP->flags &= ~OF_SHOULD_BE_DEAD;
	StartLevel (0);
	return;
}
#endif

#ifdef NETWORK
if (gameData.app.nGameMode&GM_MULTI)
	MultiDoDeath (gameData.multi.players [gameData.multi.nLocalPlayer].objnum);
else
#endif
	{				//Note link to above else!
	if (!--gameData.multi.players [gameData.multi.nLocalPlayer].lives) {	
			DoGameOver ();
			return;
			}
		}
	if (gameData.reactor.bDestroyed || gameStates.app.bD1Mission) {
		//clear out stuff so no bonus
		gameData.multi.players [gameData.multi.nLocalPlayer].hostages_on_board = 0;
		gameData.multi.players [gameData.multi.nLocalPlayer].energy = 0;
		gameData.multi.players [gameData.multi.nLocalPlayer].shields = 0;
		gameData.multi.players [gameData.multi.nLocalPlayer].connected = 3;
		DiedInMineMessage (); // Give them some indication of what happened
		if ((gameData.missions.nCurrentLevel < 0) && !gameStates.app.bD1Mission) {
			if (CFExist (SECRETB_FILENAME, gameFolders.szSaveDir, 0)) {
				ReturningToLevelMessage ();
				StateRestoreAll (1, 2, SECRETB_FILENAME);			//	2 means you died
				SetPosFromReturnSegment ();
				gameData.multi.players [gameData.multi.nLocalPlayer].lives--; //re-lose the life, gameData.multi.players [gameData.multi.nLocalPlayer].lives got written over in restore.
				}
			else {
				AdvancingToLevelMessage ();
				StartNewLevel (gameData.missions.nEnteredFromLevel + 1, 0);
				InitPlayerStatsNewShip ();	//	New, MK, 05/29/96!, fix bug with dying in secret level, advance to next level, keep powerups!
				}
			}
		else {
			ExitSecretLevel ();
			InitPlayerStatsNewShip ();
			last_drawn_cockpit [0] = -1;
			last_drawn_cockpit [1] = -1;
			}
		}
	else if (gameData.missions.nCurrentLevel < 0) {
		if (CFExist (SECRETB_FILENAME,gameFolders.szSaveDir,0)) {
			ReturningToLevelMessage ();
			if (!gameData.reactor.bDestroyed)
				StateSaveAll (0, 2, SECRETC_FILENAME);
			StateRestoreAll (1, 2, SECRETB_FILENAME);
			SetPosFromReturnSegment ();
			gameData.multi.players [gameData.multi.nLocalPlayer].lives--;						//	re-lose the life, gameData.multi.players [gameData.multi.nLocalPlayer].lives got written over in restore.
			} 
		else {
			DiedInMineMessage (); // Give them some indication of what happened
			AdvancingToLevelMessage ();
			StartNewLevel (gameData.missions.nEnteredFromLevel+1, 0);
			InitPlayerStatsNewShip ();
			}
		}
	else if (!gameStates.entropy.bExitSequence) {
		InitPlayerStatsNewShip ();
		StartLevel (1);
		}
	DigiSyncSounds ();
}

//------------------------------------------------------------------------------

//called when the player is starting a new level for normal game mode and restore state
//	bSecret set if came from a secret level
int StartNewLevelSub (int nLevel, int bPageInTextures, int bSecret)
{
	int funcRes;

	gameStates.multi.bTryAutoDL = 1;

reloadLevel:

if (!(gameData.app.nGameMode & GM_MULTI)) {
	last_drawn_cockpit [0] = -1;
	last_drawn_cockpit [1] = -1;
	}
 gameStates.render.cockpit.bBigWindowSwitch=0;
if (gameData.demo.nState == ND_STATE_PAUSED)
	gameData.demo.nState = ND_STATE_RECORDING;
if (gameData.demo.nState == ND_STATE_RECORDING) {
	NDSetNewLevel (nLevel);
	NDRecordStartFrame (gameData.app.nFrameCount, gameData.app.xFrameTime);
	}
if (gameData.app.nGameMode & GM_MULTI)
	SetFunctionMode (FMODE_MENU); // Cheap fix to prevent problems with errror dialogs in loadlevel.
SetWarnFunc (ShowInGameWarning);
funcRes = LoadLevel (nLevel, bPageInTextures);
ClearWarnFunc (ShowInGameWarning);
if (!funcRes)
	return 0;
Assert (gameStates.app.bAutoRunMission || (gameData.missions.nCurrentLevel == nLevel));	//make sure level set right
GameSeqInitNetworkPlayers (); // Initialize the gameData.multi.players array for
#if 0//def _DEBUG										  // this level
InitHoardData ();
#endif
//	gameData.objs.viewer = gameData.objs.objects + gameData.multi.players [gameData.multi.nLocalPlayer].objnum;
if (gameData.multi.nPlayers > gameData.multi.nPlayerPositions) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, "Too many players for this level.");
	return 0;
	}
Assert (gameData.multi.nPlayers <= gameData.multi.nPlayerPositions);
	//If this assert fails, there's not enough start positions

#ifdef NETWORK
if (gameData.app.nGameMode & GM_NETWORK) {
	switch (NetworkLevelSync ()) { // After calling this, gameData.multi.nLocalPlayer is set
		case -1:
			return 0;
		case 0:
			if (gameData.multi.nLocalPlayer < 0)
				return 0;
			GrRemapMonoFonts ();
			break;
		case 1:
			networkData.nStatus = 2;
			goto reloadLevel;
		}
	}
if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM))
	if (com_level_sync ())
		return 1;
#endif

Assert (gameStates.app.nFunctionMode == FMODE_GAME);
HUDClearMessages ();
AutomapClearVisited ();
#ifdef NETWORK
if (networkData.bNewGame == 1) {
	networkData.bNewGame = 0;
	InitPlayerStatsNewShip ();
}
#endif
init_player_stats_level (bSecret);
#ifdef NETWORK
if ((gameData.app.nGameMode & GM_MULTI_COOP) && networkData.bRejoined) {
	int i;
	for (i = 0; i < gameData.multi.nPlayers; i++)
		gameData.multi.players [i].flags |= netGame.player_flags [i];
}
if (gameData.app.nGameMode & GM_MULTI)
	MultiPrepLevel (); // Removes robots from level if necessary
else
	FindMonsterball (); //will simply remove all Monsterballs
#endif
gameseq_remove_unused_players ();
gameStates.app.bGameSuspended = 0;
gameData.reactor.bDestroyed = 0;
gameStates.render.glFOV = DEFAULT_FOV;
SetScreenMode (SCREEN_GAME);
InitCockpit ();
InitRobotsForLevel ();
InitShakerDetonates ();
MorphInit ();
InitAllMatCens ();
ResetPaletteAdd ();
InitThiefForLevel ();
InitStuckObjects ();
GameFlushInputs ();		// clear out the keyboard
if (!(gameData.app.nGameMode & GM_MULTI))
	FilterObjectsFromLevel ();
TurnCheatsOff ();
if (!(gameData.app.nGameMode & GM_MULTI) && !gameStates.app.cheats.bEnabled) {
	SetHighestLevel (gameData.missions.nCurrentLevel);
	}	
else
	ReadPlayerFile (1);		//get window sizes
ResetSpecialEffects ();
#ifdef NETWORK
if (networkData.bRejoined == 1){
	networkData.bRejoined = 0;
	StartLevel (1);
	}
else {
#endif
	StartLevel (0);		// Note link to above if!
	}
copy_defaults_to_robot_all ();
InitReactorForLevel ();
InitAIObjects ();
#if 0
gameData.multi.players [gameData.multi.nLocalPlayer].nInvuls =
gameData.multi.players [gameData.multi.nLocalPlayer].nCloaks = 0;
#endif
//	Say player can use FLASH cheat to mark path to exit.
nLastLevelPathCreated = -1;
return 1;
}

//------------------------------------------------------------------------------

void BashToShield (int i,char *s)
{
#ifdef NETWORK
int id = gameData.objs.objects [i].id;
gameData.multi.powerupsInMine [id] =
gameData.multi.maxPowerupsAllowed [id] = 0;
#endif
gameData.objs.objects [i].type = OBJ_POWERUP;
gameData.objs.objects [i].id = POW_SHIELD_BOOST;
gameData.objs.objects [i].size =
	gameData.objs.pwrUp.info [POW_SHIELD_BOOST].size;
gameData.objs.objects [i].rtype.vclip_info.nClipIndex = 
	gameData.objs.pwrUp.info [POW_SHIELD_BOOST].nClipIndex;
gameData.objs.objects [i].rtype.vclip_info.xFrameTime = 
	gameData.eff.vClips [0] [gameData.objs.objects [i].rtype.vclip_info.nClipIndex].xFrameTime;
}

//------------------------------------------------------------------------------

void BashToEnergy (int i,char *s)
{
#ifdef NETWORK
int id = gameData.objs.objects [i].id;
#endif
#ifdef NETWORK
gameData.multi.powerupsInMine [id] =
gameData.multi.maxPowerupsAllowed [id] = 0;
#endif
gameData.objs.objects [i].type = OBJ_POWERUP;
gameData.objs.objects [i].id = POW_ENERGY;
gameData.objs.objects [i].size =
	gameData.objs.pwrUp.info [POW_ENERGY].size;
gameData.objs.objects [i].rtype.vclip_info.nClipIndex = 
	gameData.objs.pwrUp.info [POW_ENERGY].nClipIndex;
gameData.objs.objects [i].rtype.vclip_info.xFrameTime = 
	gameData.eff.vClips [0] [gameData.objs.objects [i].rtype.vclip_info.nClipIndex].xFrameTime;
}

//------------------------------------------------------------------------------

void FilterObjectsFromLevel ()
 {
  int i;

for (i = 0; i <= gameData.objs.nLastObject; i++) {
	if (gameData.objs.objects [i].type==OBJ_POWERUP)
		if (gameData.objs.objects [i].id==POW_FLAG_RED || gameData.objs.objects [i].id==POW_FLAG_BLUE)
			BashToShield (i,"Flag!!!!");
		}
  }

//------------------------------------------------------------------------------

struct {
	int	nLevel;
	char	movie_name [FILENAME_LEN];
} intro_movie [] = { 	{ 1,"pla"},
							{ 5,"plb"},
							{ 9,"plc"},
							{13,"pld"},
							{17,"ple"},
							{21,"plf"},
							{24,"plg"}};

#define NUM_INTRO_MOVIES (sizeof (intro_movie) / sizeof (*intro_movie))

void ShowLevelIntro (int nLevel)
{
//if shareware, show a briefing?
if (!(gameData.app.nGameMode & GM_MULTI)) {
	int i;

	if (!gameStates.app.bD1Mission && (gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission)) {
		int movie=0;
		if (IS_SHAREWARE) {
			if (nLevel==1)
				DoBriefingScreens ("brief2.tex", 1);
			}
		else if (IS_D2_OEM) {
			if (nLevel == 1 && !gameStates.movies.bIntroPlayed)
				DoBriefingScreens ("brief2o.tex", 1);
			}
		else { // full version
			if (gameStates.app.bHaveExtraMovies && (nLevel == 1)) {
				InitSubTitles ("intro.tex");
				PlayMovie ("intro.mve", MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
				CloseSubTitles ();
				}
			for (i=0;i<NUM_INTRO_MOVIES;i++) {
				if (intro_movie [i].nLevel == nLevel) {
					gameStates.video.nScreenMode = -1;
					PlayMovie (intro_movie [i].movie_name, MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
					movie=1;
					break;
					}
				}

#ifdef WINDOWS
			if (!movie) {					//must go before briefing
				dd_gr_init_screen ();
				gameStates.video.nScreenMode = -1;
				}
#endif

			if (gameStates.movies.nRobots) {
				int hires_save=gameStates.menus.bHiresAvailable;
				if (gameStates.movies.nRobots == 1) {		//lowres only
					gameStates.menus.bHiresAvailable = 0;		//pretend we can't do highres
					if (hires_save != gameStates.menus.bHiresAvailable)
						gameStates.video.nScreenMode = -1;		//force reset
					}
				DoBriefingScreens ("robot.tex",nLevel);
				gameStates.menus.bHiresAvailable = hires_save;
				}
			}
		}
	else {	//not the built-in mission.  check for add-on briefing
		if ((gameData.missions.list [gameData.missions.nCurrentMission].descent_version == 1) &&
			 (gameData.missions.nCurrentMission == gameData.missions.nD1BuiltinMission)) {
			if (gameStates.app.bHaveExtraMovies && (nLevel == 1)) {
				if (PlayMovie ("briefa.mve", MOVIE_REQUIRED, 0, gameOpts->movies.bResize) != MOVIE_ABORTED)
					PlayMovie ("briefb.mve", MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
				}			
			DoBriefingScreens (szBriefingTextFilename, nLevel);
			}
		else {
			char tname [FILENAME_LEN];
			sprintf (tname, "%s.tex", gameStates.app.szCurrentMissionFile);
			DoBriefingScreens (tname, nLevel);
			}
		}
	}
}

//	---------------------------------------------------------------------------
//	If starting a level which appears in the gameData.missions.secretLevelTable, then set gameStates.app.bFirstSecretVisit.
//	Reason: On this level, if player goes to a secret level, he will be going to a different
//	secret level than he's ever been to before.
//	Sets the global gameStates.app.bFirstSecretVisit if necessary.  Otherwise leaves it unchanged.
void MaybeSetFirstSecretVisit (int nLevel)
{
	int	i;

	for (i=0; i<gameData.missions.nSecretLevels; i++) {
		if (gameData.missions.secretLevelTable [i] == nLevel) {
			gameStates.app.bFirstSecretVisit = 1;
		}
	}
}

//------------------------------------------------------------------------------
//called when the player is starting a new level for normal game model
//	bSecret if came from a secret level
int StartNewLevel (int nLevel, int bSecret)
{
	ThisLevelTime=0;

if ((nLevel > 0) && !bSecret)
	MaybeSetFirstSecretVisit (nLevel);
if (!gameStates.app.bAutoRunMission)
	ShowLevelIntro (nLevel);
WIN (DEFINE_SCREEN (NULL));		// ALT-TAB: no restore of background.
return StartNewLevelSub (nLevel, 1, bSecret);
}

//------------------------------------------------------------------------------
//initialize the player object position & orientation (at start of game, or new ship)
void InitPlayerPosition (int random_flag)
{
	int bNewPlayer=0;

#ifdef NETWORK
	if (!((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode&GM_MULTI_COOP))) // If not deathmatch
#endif
		bNewPlayer = gameData.multi.nLocalPlayer;
#ifdef NETWORK
	else if (random_flag == 1) {
		object *pObj;
		int spawnMap [MAX_NUM_NET_PLAYERS];
		int nSpawnSegs = 0;
		int i, closest = -1, trys = 0;
		fix closest_dist = 0x7ffffff, dist;


		for (i = 0; i < gameData.multi.nPlayerPositions; i++)
			spawnMap [i] = i;

		d_srand (SDL_GetTicks ());

#ifndef NDEBUG
		if (gameData.multi.nPlayerPositions != MAX_NUM_NET_PLAYERS)
		{
#if TRACE		
			//con_printf (1, "WARNING: There are only %d start positions!\n");
#endif
			//Int3 ();
		}
#endif

		do {
			trys++;
			if (gameData.app.nGameMode & GM_TEAM) {
				if (!nSpawnSegs) {
					for (i = 0; i < gameData.multi.nPlayerPositions; i++)
						spawnMap [i] = i;
					nSpawnSegs = gameData.multi.nPlayerPositions;
					}
				if (nSpawnSegs) {		//try to find a spawn location owned by the player's team
					closest_dist = 0;
					i = d_rand () % nSpawnSegs;
					bNewPlayer = spawnMap [i];
					if (i < --nSpawnSegs)
						spawnMap [i] = spawnMap [nSpawnSegs];
					switch (gameData.multi.playerInit [bNewPlayer].segtype) {
						case SEGMENT_IS_GOAL_RED:
						case SEGMENT_IS_TEAM_RED:
							if (GetTeam (gameData.multi.nLocalPlayer) != TEAM_RED)
								continue;
							break;
						case SEGMENT_IS_GOAL_BLUE:
						case SEGMENT_IS_TEAM_BLUE:
							if (GetTeam (gameData.multi.nLocalPlayer) != TEAM_BLUE)
								continue;
							break;
						default:
							break;
						}
					}
				}
			else {
				bNewPlayer = d_rand () % gameData.multi.nPlayerPositions;
				}
			closest = -1;
			closest_dist = 0x7fffffff;
			for (i = 0; i < gameData.multi.nPlayers; i++) {
				if (i == gameData.multi.nLocalPlayer)
					continue;
				pObj = gameData.objs.objects + gameData.multi.players [i].objnum; 
				if ((pObj->type == OBJ_PLAYER))	{
					dist = FindConnectedDistance (&pObj->pos, 
															 pObj->segnum, 
															 &gameData.multi.playerInit [bNewPlayer].pos, 
															 gameData.multi.playerInit [bNewPlayer].segnum, 
															 10, WID_FLY_FLAG);	//	Used to be 5, search up to 10 segments
					if ((dist < closest_dist) && (dist >= 0))	{
						closest_dist = dist;
						closest = i;
					}
				}
			}
		} while ((closest_dist < i2f (15 * 20)) && (trys < MAX_NUM_NET_PLAYERS * 2));
	}
	else {
		goto done; // If deathmatch and not random, positions were already determined by sync packet
	}
	Assert (bNewPlayer >= 0);
	Assert (bNewPlayer < gameData.multi.nPlayerPositions);
#endif

	gameData.objs.console->pos = gameData.multi.playerInit [bNewPlayer].pos;
	gameData.objs.console->orient = gameData.multi.playerInit [bNewPlayer].orient;
 	RelinkObject (OBJ_IDX (gameData.objs.console),gameData.multi.playerInit [bNewPlayer].segnum);
#ifdef NETWORK
done:
#endif
	ResetPlayerObject ();
	ResetCruise ();
}

//------------------------------------------------------------------------------
//	Initialize default parameters for one robot, copying from gameData.bots.pInfo to *objP.
//	What about setting size!?  Where does that come from?
void copy_defaults_to_robot (object *objP)
{
	robot_info	*robptr;
	int			objid;

	Assert (objP->type == OBJ_ROBOT);
	objid = objP->id;
	Assert (objid < gameData.bots.nTypes [0]);

	robptr = &gameData.bots.pInfo [objid];

	//	Boost shield for Thief and Buddy based on level.
	objP->shields = robptr->strength;

	if ((robptr->thief) || (robptr->companion)) {
		objP->shields = (objP->shields * (abs (gameData.missions.nCurrentLevel)+7))/8;

		if (robptr->companion) {
			//	Now, scale guide-bot hits by skill level
			switch (gameStates.app.nDifficultyLevel) {
				case 0:	objP->shields = i2f (20000);	break;		//	Trainee, basically unkillable
				case 1:	objP->shields *= 3;				break;		//	Rookie, pretty dang hard
				case 2:	objP->shields *= 2;				break;		//	Hotshot, a bit tough
				default:	break;
			}
		}
	} else if (robptr->boss_flag)	//	MK, 01/16/95, make boss shields lower on lower diff levels.
		objP->shields = objP->shields/ (NDL+3) * (gameStates.app.nDifficultyLevel+4);

	//	Additional wimpification of bosses at Trainee
	if ((robptr->boss_flag) && (gameStates.app.nDifficultyLevel == 0))
		objP->shields /= 2;
}

//------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
void copy_defaults_to_robot_all ()
{
	int	i;

	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (gameData.objs.objects [i].type == OBJ_ROBOT)
			copy_defaults_to_robot (&gameData.objs.objects [i]);

}

extern void ClearStuckObjects (void);

//------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
void StartLevel (int random_flag)
{
Assert (!gameStates.app.bPlayerIsDead);
InitPlayerPosition (random_flag);
verify_console_object ();
gameData.objs.console->control_type = CT_FLYING;
gameData.objs.console->movement_type = MT_PHYSICS;
MultiSendShields ();
DisableMatCens ();
clear_transient_objects (0);		//0 means leave proximity bombs
// CreatePlayerAppearanceEffect (gameData.objs.console);
gameStates.render.bDoAppearanceEffect = 1;
#ifdef NETWORK
if (gameData.app.nGameMode & GM_MULTI) {
	if (gameData.app.nGameMode & GM_MULTI_COOP)
		MultiSendScore ();
	MultiSendPosition (gameData.multi.players [gameData.multi.nLocalPlayer].objnum);
	MultiSendReappear ();
	}		
if (gameData.app.nGameMode & GM_NETWORK)
	NetworkDoFrame (1, 1);
#endif
ai_reset_all_paths ();
AIInitBossForShip ();
ClearStuckObjects ();
#ifdef EDITOR
//	Note, this is only done if editor builtin.  Calling this from here
//	will cause it to be called after the player dies, resetting the
//	hits for the buddy and thief.  This is ok, since it will work ok
//	in a shipped version.
InitAIObjects ();
#endif
ResetTime ();
ResetRearView ();
gameData.app.fusion.xAutoFireTime = 0;
gameData.app.fusion.xCharge = 0;
gameStates.app.cheats.bRobotsFiring = 1;
gameStates.app.cheats.bD1CheatsEnabled = 0;
//DigiClose ();
gameOpts->sound.bD1Sound = gameStates.app.bD1Mission && gameStates.app.bHaveD1Data && gameOpts->sound.bUseD1Sounds;
SetDataVersion (-1);
//DigiInit ();
}

//------------------------------------------------------------------------------
//eof
