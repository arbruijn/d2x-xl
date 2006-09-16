/* $Id: fuelcen.c,v 1.8 2003/10/04 03:30:27 btb Exp $ */
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
 * Functions for refueling centers.
 *
 * Old Log:
 * Revision 1.2  1995/10/31  10:23:40  allender
 * shareware stuff
 *
 * Revision 1.1  1995/05/16  15:24:50  allender
 * Initial revision
 *
 * Revision 2.3  1995/03/21  14:38:40  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.2  1995/03/06  15:23:09  john
 * New screen techniques.
 *
 * Revision 2.1  1995/02/27  13:13:26  john
 * Removed floating point.
 *
 * Revision 2.0  1995/02/27  11:27:20  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.159  1995/02/22  13:48:10  allender
 * remove anonymous unions in object structure
 *
 * Revision 1.158  1995/02/08  11:37:48  mike
 * Check for failures in call to CreateObject.
 *
 * Revision 1.157  1995/02/07  20:39:39  mike
 * fix toasters in multiplayer
 *
 *
 * Revision 1.156  1995/02/02  18:40:10  john
 * Fixed bug with full screen cockpit flashing non-white.
 *
 * Revision 1.155  1995/01/28  15:27:22  yuan
 * Make sure fuelcen nums are valid.
 *
 * Revision 1.154  1995/01/03  14:26:23  rob
 * Better ifdef for robot centers.
 *
 * Revision 1.153  1995/01/03  11:27:49  rob
 * Added include of fuelcen.c
 *
 * Revision 1.152  1995/01/03  09:47:22  john
 * Some ifdef SHAREWARE lines.
 *
 * Revision 1.151  1995/01/02  21:02:07  rob
 * added matcen support for coop/multirobot.
 *
 * Revision 1.150  1994/12/15  18:31:22  mike
 * fix confusing precedence problems.
 *
 * Revision 1.149  1994/12/15  13:04:22  mike
 * Replace gameData.multi.players[gameData.multi.nLocalPlayer].time_total references with gameData.app.xGameTime.
 *
 * Revision 1.148  1994/12/15  03:05:18  matt
 * Added error checking for NULL return from ObjectCreateExplosion()
 *
 * Revision 1.147  1994/12/13  19:49:12  rob
 * Made the fuelcen noise quieter.
 *
 * Revision 1.146  1994/12/13  12:03:18  john
 * Made the warning sirens not start until after "desccruction
 * secquence activated voice".
 *
 * Revision 1.145  1994/12/12  17:18:30  mike
 * make warning siren louder.
 *
 * Revision 1.144  1994/12/11  23:18:04  john
 * Added -nomusic.
 * Added gameData.app.xRealFrameTime.
 * Put in a pause when sound initialization error.
 * Made controlcen countdown and framerate use gameData.app.xRealFrameTime.
 *
 * Revision 1.143  1994/12/11  14:10:16  mike
 * louder sounds.
 *
 * Revision 1.142  1994/12/06  11:33:19  yuan
 * Fixed bug with fueling when above 100.
 *
 * Revision 1.141  1994/12/05  23:37:14  matt
 * Took out calls to warning() function
 *
 * Revision 1.140  1994/12/05  23:19:18  yuan
 * Fixed fuel center refuelers..
 *
 * Revision 1.139  1994/12/03  12:48:12  mike
 * diminish rocking due to control center destruction.
 *
 * Revision 1.138  1994/12/02  23:30:32  mike
 * fix bumpiness after toasting control center.
 *
 * Revision 1.137  1994/12/02  22:48:14  mike
 * rock the ship after toasting the control center!
 *
 * Revision 1.136  1994/12/02  17:12:11  rob
 * Fixed countdown sounds.
 *
 * Revision 1.135  1994/11/29  20:59:43  rob
 * Don't run out of fuel in net games (don't want to sync it between machines)
 *
 * Revision 1.134  1994/11/29  19:10:57  john
 * Took out debugging con_printf.
 *
 * Revision 1.133  1994/11/29  13:19:40  john
 * Made voice for "destruction actived in t-"
 * be at 12.75 secs.
 *
 * Revision 1.132  1994/11/29  12:19:46  john
 * MAde the "Mine desctruction will commence"
 * voice play at 12.5 secs.
 *
 * Revision 1.131  1994/11/29  12:12:54  adam
 * *** empty log message ***
 *
 * Revision 1.130  1994/11/28  21:04:26  rob
 * Added code to cast noise when player refuels.
 *
 * Revision 1.129  1994/11/27  23:15:04  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.128  1994/11/21  16:27:51  mike
 * debug code for morphing.
 *
 * Revision 1.127  1994/11/21  12:33:50  matt
 * For control center explosions, use small fireball, not pseudo-random vclip
 *
 * Revision 1.126  1994/11/20  22:12:15  mike
 * Fix bug in initializing materialization centers.
 *
 * Revision 1.125  1994/11/19  15:18:22  mike
 * rip out unused code and data.
 *
 * Revision 1.124  1994/11/08  12:18:59  mike
 * Initialize Fuelcen_seconds_left.
 *
 * Revision 1.123  1994/10/30  14:12:33  mike
 * rip out repair center stuff
 *
 * Revision 1.122  1994/10/28  14:42:45  john
 * Added sound volumes to all sound calls.
 *
 * Revision 1.121  1994/10/16  12:44:02  mike
 * Make time to exit mine after control center destruction diff level dependent.
 *
 * Revision 1.120  1994/10/09  22:03:26  mike
 * Adapt to new create_n_segment_path parameters.
 *
 * Revision 1.119  1994/10/06  14:52:42  mike
 * Remove last of ability to damage fuel centers.
 *
 * Revision 1.118  1994/10/06  14:08:45  matt
 * Made morph flash effect get orientation from segment
 *
 * Revision 1.117  1994/10/05  16:09:03  mike
 * Put debugging code into matcen/fuelcen synchronization problem.
 *
 * Revision 1.116  1994/10/04  15:32:41  john
 * Took out the old PLAY_SOUND??? code and replaced it
 * with direct calls into digi_link_??? so that all sounds
 * can be made 3d.
 *
 * Revision 1.115  1994/10/03  23:37:57  mike
 * Clean up this mess of confusion to the point where maybe matcens actually work.
 *
 * Revision 1.114  1994/10/03  13:34:40  matt
 * Added new (and hopefully better) game sequencing functions
 *
 * Revision 1.113  1994/09/30  14:41:57  matt
 * Fixed bug as per Mike's instructions
 *
 * Revision 1.112  1994/09/30  00:37:33  mike
 * Balance materialization centers.
 *
 * Revision 1.111  1994/09/28  23:12:52  matt
 * Macroized palette flash system
 *
 * Revision 1.110  1994/09/27  15:42:31  mike
 * Add names of Specials.
 *
 * Revision 1.109  1994/09/27  00:02:23  mike
 * Yet more materialization center stuff.
 *
 * Revision 1.108  1994/09/26  11:26:23  mike
 * Balance materialization centers.
 *
 * Revision 1.107  1994/09/25  23:40:47  matt
 * Changed the object load & save code to read/write the structure fields one
 * at a time (rather than the whole structure at once).  This mean that the
 * object structure can be changed without breaking the load/save functions.
 * As a result of this change, the local_object data can be and has been
 * incorporated into the object array.  Also, timeleft is now a property
 * of all gameData.objs.objects, and the object structure has been otherwise cleaned up.
 *
 * Revision 1.106  1994/09/25  15:55:58  mike
 * Balance materialization centers, make them emit light, make them re-triggerable after awhile.
 *
 * Revision 1.105  1994/09/24  17:42:33  mike
 * Making materialization centers be activated by triggers and balancing them.
 *
 * Revision 1.104  1994/09/24  14:16:06  mike
 * Support new network constants.
 *
 * Revision 1.103  1994/09/20  19:14:40  john
 * Massaged the sound system; used a better formula for determining
 * which l/r balance, also, put in Mike's stuff that searches for a connection
 * between the 2 sounds' segments, stopping for closed doors, etc.
 *
 * Revision 1.102  1994/09/17  01:40:51  matt
 * Added status bar/sizable window mode, and in the process revamped the
 * whole cockpit mode system.
 *
 * Revision 1.101  1994/08/31  20:57:25  matt
 * Cleaned up endlevel/death code
 *
 * Revision 1.100  1994/08/30  17:54:20  mike
 * Slow down rate of creation of gameData.objs.objects by materialization centers.
 *
 * Revision 1.99  1994/08/29  11:47:01  john
 * Added warning if no control centers in mine.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: fuelcen.c,v 1.8 2003/10/04 03:30:27 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "inferno.h"
#include "fuelcen.h"
#include "gameseg.h"
#include "game.h"		// For gameData.app.xFrameTime
#include "error.h"
#include "mono.h"
#include "gauges.h"
#include "vclip.h"
#include "fireball.h"
#include "robot.h"
#include "powerup.h"

#include "wall.h"
#include "sounds.h"
#include "morph.h"
#include "3d.h"
#include "bm.h"
#include "polyobj.h"
#include "ai.h"
#include "gamemine.h"
#include "gamesave.h"
#include "player.h"
#include "collide.h"
#include "laser.h"
#ifdef NETWORK
#include "network.h"
#include "multi.h"
#endif
#include "multibot.h"
#include "escort.h"

// The max number of fuel stations per mine.

// Every time a robot is created in the morphing code, it decreases capacity of the morpher
// by this amount... when capacity gets to 0, no more morphers...
#define MATCEN_HP_DEFAULT			F1_0*500; // Hitpoints
#define MATCEN_INTERVAL_DEFAULT	F1_0*5;	//  5 seconds

#ifdef EDITOR
char	Special_names[MAX_CENTER_TYPES][11] = {
	"NOTHING   ",
	"FUELCEN   ",
	"REPAIRCEN ",
	"CONTROLCEN",
	"ROBOTMAKER",
	"GOAL_RED",
	"GOAL_BLUE",
};
#endif

//------------------------------------------------------------
// Resets all fuel center info
void fuelcen_reset()
{
	int i;

	gameData.matCens.nFuelCenters = 0;
	for(i=0; i<MAX_SEGMENTS; i++ )
		gameData.segs.segment2s[i].special = SEGMENT_IS_NOTHING;

	gameData.matCens.nRobotCenters = 0;

}

#ifndef NDEBUG		//this is sometimes called by people from the debugger
void reset_all_robot_centers()
{
	int i;

	// Remove all materialization centers
	for (i=0; i<gameData.segs.nSegments; i++)
		if (gameData.segs.segment2s[i].special == SEGMENT_IS_ROBOTMAKER) {
			gameData.segs.segment2s[i].special = SEGMENT_IS_NOTHING;
			gameData.segs.segment2s[i].matcen_num = -1;
		}
}
#endif

//------------------------------------------------------------
// Turns a segment into a fully charged up fuel center...
void fuelcen_create( segment *segP, int oldType)
{
	segment2	*seg2p = gameData.segs.segment2s + (SEG_IDX (segP));

	int	station_type = seg2p->special;
	int	i;
	
	switch( station_type )	{
	case SEGMENT_IS_NOTHING:
	case SEGMENT_IS_GOAL_BLUE:
	case SEGMENT_IS_GOAL_RED:
	case SEGMENT_IS_TEAM_BLUE:
	case SEGMENT_IS_TEAM_RED:
	case SEGMENT_IS_WATER:
	case SEGMENT_IS_LAVA:
	case SEGMENT_IS_SPEEDBOOST:
	case SEGMENT_IS_BLOCKED:
	case SEGMENT_IS_NODAMAGE:
	case SEGMENT_IS_SKYBOX:
		return;
	case SEGMENT_IS_FUELCEN:
	case SEGMENT_IS_REPAIRCEN:
	case SEGMENT_IS_CONTROLCEN:
	case SEGMENT_IS_ROBOTMAKER:
		break;
	default:
		Error( "Segment %d has invalid\nstation type %d in fuelcen.c\n", SEG_IDX (segP), station_type );
	}

	Assert((seg2p != NULL));
	if ( seg2p == NULL ) 
		return;

	switch (oldType) {
		case SEGMENT_IS_FUELCEN:
		case SEGMENT_IS_REPAIRCEN:
		case SEGMENT_IS_ROBOTMAKER:
			i = seg2p->value;
			break;
		default:
			Assert( gameData.matCens.nFuelCenters < MAX_FUEL_CENTERS );
			i = gameData.matCens.nFuelCenters;
		}

	seg2p->value = i;
	gameData.matCens.fuelCenters[i].Type = station_type;
	gameData.matCens.origStationTypes [i] = (oldType == station_type) ? SEGMENT_IS_NOTHING : oldType;
	gameData.matCens.fuelCenters[i].MaxCapacity = gameData.matCens.xFuelMaxAmount;
	gameData.matCens.fuelCenters[i].Capacity = gameData.matCens.fuelCenters[i].MaxCapacity;
	gameData.matCens.fuelCenters[i].segnum = SEG2_IDX (seg2p);
	gameData.matCens.fuelCenters[i].Timer = -1;
	gameData.matCens.fuelCenters[i].Flag = 0;
//	gameData.matCens.fuelCenters[i].NextRobotType = -1;
//	gameData.matCens.fuelCenters[i].last_created_obj=NULL;
//	gameData.matCens.fuelCenters[i].last_created_sig = -1;
	COMPUTE_SEGMENT_CENTER(&gameData.matCens.fuelCenters[i].Center, segP);
	if (oldType == SEGMENT_IS_NOTHING)
		gameData.matCens.nFuelCenters++;
	else if (oldType == SEGMENT_IS_ROBOTMAKER) {
		gameData.matCens.origStationTypes [i] = SEGMENT_IS_NOTHING;
		i = seg2p->matcen_num;
		if (i < --gameData.matCens.nRobotCenters)
			memcpy (gameData.matCens.robotCenters + i, gameData.matCens.robotCenters + i + 1, (gameData.matCens.nRobotCenters - i) * sizeof (fuelcen_info));
		}
//	if (station_type == SEGMENT_IS_ROBOTMAKER)
//		gameData.matCens.fuelCenters[gameData.matCens.nFuelCenters].Capacity = i2f(gameStates.app.nDifficultyLevel + 3);
}

//------------------------------------------------------------
// Adds a matcen that already is a special type into the gameData.matCens.fuelCenters array.
// This function is separate from other fuelcens because we don't want values reset.
void matcen_create( segment *segP, int oldType)
{
	segment2	*seg2p = &gameData.segs.segment2s[SEG_IDX (segP)];
	int	station_type = seg2p->special;
	int	i;

	Assert( (seg2p != NULL) );
	Assert(station_type == SEGMENT_IS_ROBOTMAKER);
	if ( seg2p == NULL ) return;

	Assert( gameData.matCens.nFuelCenters > -1 );
	switch (oldType) {
		case SEGMENT_IS_FUELCEN:
		case SEGMENT_IS_REPAIRCEN:
		case SEGMENT_IS_ROBOTMAKER:
			i = seg2p->value;
			break;
		default:
			Assert( gameData.matCens.nFuelCenters < MAX_FUEL_CENTERS );
			i = gameData.matCens.nFuelCenters;
		}
	seg2p->value = i;
	gameData.matCens.fuelCenters[i].Type = station_type;
	gameData.matCens.origStationTypes [i] = (oldType == station_type) ? SEGMENT_IS_NOTHING : oldType;
	gameData.matCens.fuelCenters[i].Capacity = i2f(gameStates.app.nDifficultyLevel + 3);
	gameData.matCens.fuelCenters[i].MaxCapacity = gameData.matCens.fuelCenters[i].Capacity;
	gameData.matCens.fuelCenters[i].segnum = SEG2_IDX (seg2p);
	gameData.matCens.fuelCenters[i].Timer = -1;
	gameData.matCens.fuelCenters[i].Flag = 0;
//	gameData.matCens.fuelCenters[i].NextRobotType = -1;
//	gameData.matCens.fuelCenters[i].last_created_obj=NULL;
//	gameData.matCens.fuelCenters[i].last_created_sig = -1;
	COMPUTE_SEGMENT_CENTER_I(&gameData.matCens.fuelCenters[i].Center, seg2p-gameData.segs.segment2s);
	seg2p->matcen_num = gameData.matCens.nRobotCenters;
	gameData.matCens.robotCenters[gameData.matCens.nRobotCenters].hit_points = MATCEN_HP_DEFAULT;
	gameData.matCens.robotCenters[gameData.matCens.nRobotCenters].interval = MATCEN_INTERVAL_DEFAULT;
	gameData.matCens.robotCenters[gameData.matCens.nRobotCenters].segnum = SEG2_IDX (seg2p);
	if (oldType == SEGMENT_IS_NOTHING)
		gameData.matCens.robotCenters[gameData.matCens.nRobotCenters].fuelcen_num = gameData.matCens.nFuelCenters;
	gameData.matCens.nRobotCenters++;
	gameData.matCens.nFuelCenters++;
}

//------------------------------------------------------------
// Adds a segment that already is a special type into the gameData.matCens.fuelCenters array.
void fuelcen_activate( segment * segP, int station_type )
{
	segment2	*seg2p = &gameData.segs.segment2s[SEG_IDX (segP)];

	seg2p->special = station_type;
	if (seg2p->special == SEGMENT_IS_ROBOTMAKER)
		matcen_create( segP, SEGMENT_IS_NOTHING);
	else
		fuelcen_create( segP, SEGMENT_IS_NOTHING);
}

//	The lower this number is, the more quickly the center can be re-triggered.
//	If it's too low, it can mean all the robots won't be put out, but for about 5
//	robots, that's not real likely.
#define	MATCEN_LIFE (i2f(30-2*gameStates.app.nDifficultyLevel))

//------------------------------------------------------------
//	Trigger (enable) the materialization center in segment segnum
void trigger_matcen(short segnum)
{
	// -- segment		*segP = &gameData.segs.segments[segnum];
	segment2		*seg2p = &gameData.segs.segment2s[segnum];
	vms_vector	pos, delta;
	fuelcen_info	*robotcen;
	int			objnum;

#if TRACE
	con_printf (CON_DEBUG, "Trigger matcen, segment %i\n", segnum);
#endif
	Assert(seg2p->special == SEGMENT_IS_ROBOTMAKER);
	Assert(seg2p->matcen_num < gameData.matCens.nFuelCenters);
	Assert((seg2p->matcen_num >= 0) && (seg2p->matcen_num <= gameData.segs.nLastSegment));

	robotcen = gameData.matCens.fuelCenters + gameData.matCens.robotCenters [seg2p->matcen_num].fuelcen_num;

	if (robotcen->Enabled == 1)
		return;

	if (!robotcen->Lives)
		return;

	//	MK: 11/18/95, At insane, matcens work forever!
	if (gameStates.app.nDifficultyLevel+1 < NDL)
		robotcen->Lives--;

	robotcen->Timer = F1_0*1000;	//	Make sure the first robot gets emitted right away.
	robotcen->Enabled = 1;			//	Say this center is enabled, it can create robots.
	robotcen->Capacity = i2f(gameStates.app.nDifficultyLevel + 3);
	robotcen->Disable_time = MATCEN_LIFE;

	//	Create a bright object in the segment.
	pos = robotcen->Center;
	VmVecSub(&delta, &gameData.segs.vertices[gameData.segs.segments[segnum].verts[0]], &robotcen->Center);
	VmVecScaleInc(&pos, &delta, F1_0/2);
	objnum = CreateObject( OBJ_LIGHT, 0, -1, segnum, &pos, NULL, 0, CT_LIGHT, MT_NONE, RT_NONE );
	if (objnum != -1) {
		gameData.objs.objects[objnum].lifeleft = MATCEN_LIFE;
		gameData.objs.objects[objnum].ctype.light_info.intensity = i2f(8);	//	Light cast by a fuelcen.
	} else {
#if TRACE
		con_printf (1, "Can't create invisible flare for matcen.\n");
#endif
		Int3();
	}
}

#ifdef EDITOR
//------------------------------------------------------------
// Takes away a segment's fuel center properties.
//	Deletes the segment point entry in the fuelcen_info list.
void fuelcen_delete( segment * segP )
{
	segment2	*seg2p = &gameData.segs.segment2s[SEG_IDX (segP)];
	int i, j;

Restart: ;

	seg2p->special = 0;

	for (i=0; i<gameData.matCens.nFuelCenters; i++ )	{
		if ( gameData.matCens.fuelCenters[i].segnum == SEG_IDX (segP) )	{

			// If Robot maker is deleted, fix gameData.segs.segments and gameData.matCens.robotCenters.
			if (gameData.matCens.fuelCenters[i].Type == SEGMENT_IS_ROBOTMAKER) {
				gameData.matCens.nRobotCenters--;
				Assert(gameData.matCens.nRobotCenters >= 0);

				for (j=seg2p->matcen_num; j<gameData.matCens.nRobotCenters; j++)
					gameData.matCens.robotCenters[j] = gameData.matCens.robotCenters[j+1];

				for (j=0; j<gameData.matCens.nFuelCenters; j++) {
					if ( gameData.matCens.fuelCenters[j].Type == SEGMENT_IS_ROBOTMAKER )
						if ( gameData.segs.segment2s[gameData.matCens.fuelCenters[j].segnum].matcen_num > seg2p->matcen_num )
							gameData.segs.segment2s[gameData.matCens.fuelCenters[j].segnum].matcen_num--;
				}
			}

			//fix gameData.matCens.robotCenters so they point to correct fuelcenter
			for (j=0; j<gameData.matCens.nRobotCenters; j++ )
				if (gameData.matCens.robotCenters[j].fuelcen_num > i)		//this robotcenter's fuelcen is changing
					gameData.matCens.robotCenters[j].fuelcen_num--;

			gameData.matCens.nFuelCenters--;
			Assert(gameData.matCens.nFuelCenters >= 0);
			for (j=i; j<gameData.matCens.nFuelCenters; j++ )	{
				gameData.matCens.fuelCenters[j] = gameData.matCens.fuelCenters[j+1];
				gameData.segs.segment2s[gameData.matCens.fuelCenters[j].segnum].value = j;
			}
			goto Restart;
		}
	}
}
#endif

#define	ROBOT_GEN_TIME (i2f(5))

object * create_morph_robot(segment *segP, vms_vector *vObjPosP, ubyte object_id)
{
	short		objnum;
	object	*objP;
	ubyte		default_behavior;

	gameData.multi.players[gameData.multi.nLocalPlayer].num_robots_level++;
	gameData.multi.players[gameData.multi.nLocalPlayer].num_robots_total++;

	objnum = CreateObject ((ubyte) OBJ_ROBOT, object_id, -1, SEG_IDX (segP), vObjPosP,
				&vmd_identity_matrix, gameData.models.polyModels [gameData.bots.pInfo[object_id].model_num].rad,
				(ubyte) CT_AI, (ubyte) MT_PHYSICS, (ubyte) RT_POLYOBJ);

	if ( objnum < 0 ) {
#if TRACE
		con_printf (1, "Can't create morph robot.  Aborting morph.\n");
#endif
		Int3();
		return NULL;
	}

	objP = gameData.objs.objects + objnum;
	//Set polygon-object-specific data
	objP->rtype.pobj_info.model_num = gameData.bots.pInfo[objP->id].model_num;
	objP->rtype.pobj_info.subobj_flags = 0;
	//set Physics info
	objP->mtype.phys_info.mass = gameData.bots.pInfo[objP->id].mass;
	objP->mtype.phys_info.drag = gameData.bots.pInfo[objP->id].drag;
	objP->mtype.phys_info.flags |= (PF_LEVELLING);
	objP->shields = gameData.bots.pInfo[objP->id].strength;
	default_behavior = gameData.bots.pInfo[objP->id].behavior;
	InitAIObject(OBJ_IDX (objP), default_behavior, -1 );		//	Note, -1 = segment this robot goes to to hide, should probably be something useful
	create_n_segment_path(objP, 6, -1);		//	Create a 6 segment path from creation point.
	gameData.ai.localInfo[objnum].mode = AIBehaviorToMode(default_behavior);
	return objP;
}

int Num_extry_robots = 15;

#ifndef NDEBUG
int	FrameCount_last_msg = 0;
#endif

//	----------------------------------------------------------------------------------------------------------

void robotmaker_proc (fuelcen_info * robotcen)
{
	fix			dist_to_player;
	vms_vector	cur_object_loc; //, direction;
	int			matcen_num, segnum, objnum;
	object		*objP;
	fix			top_time;
	vms_vector	direction;
#if 0//def _DEBUG
	int			bMakeVirus = 1;
#else
	int			bMakeVirus = (gameData.app.nGameMode & GM_ENTROPY) != 0;
#endif
	ubyte			vclip = bMakeVirus ? VCLIP_POWERUP_DISAPPEARANCE : VCLIP_MORPHING_ROBOT;

	if (bMakeVirus) {
#if 1//def RELEASE
		if (gameStates.entropy.bExitSequence || (gameData.segs.xSegments [robotcen->segnum].owner <= 0))
			return;
#endif
		}
	else {
		if (!robotcen->Enabled)
			return;
		if (robotcen->Disable_time > 0) {
			robotcen->Disable_time -= gameData.app.xFrameTime;
			if (robotcen->Disable_time <= 0) {
#if TRACE
				con_printf (CON_DEBUG, "Robot center #%i gets disabled due to time running out.\n", 
								FUELCEN_IDX (robotcen));
#endif
				robotcen->Enabled = 0;
			}
		}
	}

	//	No robot making in multiplayer mode.
#ifdef NETWORK
#ifndef SHAREWARE
	if (!bMakeVirus && (gameData.app.nGameMode & GM_MULTI) && (!(gameData.app.nGameMode & GM_MULTI_ROBOTS) || !NetworkIAmMaster()))
		return;
#else
	if (gameData.app.nGameMode & GM_MULTI)
		return;
#endif
#endif

	// Wait until transmorgafier has capacity to make a robot...
	if (!bMakeVirus && (robotcen->Capacity <= 0)) {
		return;
	}

	matcen_num = gameData.segs.segment2s[robotcen->segnum].matcen_num;

	if ( matcen_num == -1 ) {
#if TRACE
		con_printf (CON_DEBUG, "Non-functional robotcen at %d\n", robotcen->segnum);
#endif
		return;
	}

	if (!bMakeVirus && 
		 gameData.matCens.robotCenters[matcen_num].robot_flags[0]==0 && 
		 gameData.matCens.robotCenters[matcen_num].robot_flags[1]==0) {
		return;
	}

	// Wait until we have a d_free slot for this puppy...
   //	  <<<<<<<<<<<<<<<< Num robots in mine >>>>>>>>>>>>>>>>>>>>>>>>>>    <<<<<<<<<<<< Max robots in mine >>>>>>>>>>>>>>>
	if ((gameData.multi.players[gameData.multi.nLocalPlayer].num_robots_level - gameData.multi.players[gameData.multi.nLocalPlayer].num_kills_level) >= 
		 (Gamesave_num_org_robots + Num_extry_robots )) {
		#ifndef NDEBUG
		if (gameData.app.nFrameCount > FrameCount_last_msg + 20) {
#if TRACE
			con_printf (CON_DEBUG, "Cannot morph until you kill one!\n");
#endif
			FrameCount_last_msg = gameData.app.nFrameCount;
		}
		#endif
		return;
	}

	robotcen->Timer += gameData.app.xFrameTime;

	switch(robotcen->Flag)	{
	case 0:		// Wait until next robot can generate
		if (gameData.app.nGameMode & GM_MULTI) {
			top_time = bMakeVirus ? i2f (extraGameInfo [1].entropy.nVirusGenTime) : ROBOT_GEN_TIME;	
//			if (bMakeVirus)
//				top_time *= 2;
			}
		else {
			dist_to_player = VmVecDistQuick( &gameData.objs.console->pos, &robotcen->Center );
			top_time = dist_to_player/64 + d_rand() * 2 + F1_0*2;
			if ( top_time > ROBOT_GEN_TIME )
				top_time = ROBOT_GEN_TIME + d_rand();
			if ( top_time < F1_0*2 )
				top_time = F1_0*3/2 + d_rand()*2;
			}
		if (robotcen->Timer > top_time )	{
			int	count=0;
			int	i, my_station_num = FUELCEN_IDX (robotcen);

			//	Make sure this robotmaker hasn't put out its max without having any of them killed.
			if (bMakeVirus) {
				object *objP;
				int objnum = gameData.segs.segments [robotcen->segnum].objects;

				while (objnum >= 0) {
					objP = gameData.objs.objects + objnum;
					if ((objP->type == OBJ_POWERUP) && (objP->id == POW_ENTROPY_VIRUS)) {
						robotcen->Timer = 0;
						return;
						}
					objnum = objP->next;
					}
				}
			else {
				for (i=0; i<=gameData.objs.nLastObject; i++)
					if (gameData.objs.objects[i].type == OBJ_ROBOT)
						if ((gameData.objs.objects[i].matcen_creator^0x80) == my_station_num)
							count++;
				if (count > gameStates.app.nDifficultyLevel + 3) {
#if TRACE
					con_printf (CON_DEBUG, "Cannot morph: center %i has already put out %i robots.\n", my_station_num, count);
#endif
					robotcen->Timer /= 2;
					return;
					}
				}

			//	Whack on any robot or player in the matcen segment.
			count=0;
			segnum = robotcen->segnum;
#if 1//def _DEBUG
		if (!bMakeVirus)
#endif
			for (objnum=gameData.segs.segments[segnum].objects;objnum!=-1;objnum=gameData.objs.objects[objnum].next)	{
				count++;
				if ( count > MAX_OBJECTS )	{
#if TRACE
					con_printf (CON_DEBUG, "Object list in segment %d is circular.", segnum );
#endif
					Int3();
					return;
				}
				if (gameData.objs.objects[objnum].type == OBJ_ROBOT) {
					CollideRobotAndMatCen (gameData.objs.objects + objnum);
					robotcen->Timer = top_time / 2;
					return;
					}
				else if (gameData.objs.objects[objnum].type == OBJ_PLAYER ) {
					CollidePlayerAndMatCen (gameData.objs.objects + objnum);
					robotcen->Timer = top_time / 2;
					return;
				}
			}

			COMPUTE_SEGMENT_CENTER_I(&cur_object_loc, robotcen->segnum);
			// HACK!!! The 10 under here should be something equal to the 1/2 the size of the segment.
			objP = ObjectCreateExplosion((short) robotcen->segnum, &cur_object_loc, i2f(10), vclip);

			if (objP)
				ExtractOrientFromSegment (&objP->orient,gameData.segs.segments + robotcen->segnum);

			if (gameData.eff.vClips [0][vclip].sound_num > -1) {
				DigiLinkSoundToPos (gameData.eff.vClips [0][vclip].sound_num, (short) robotcen->segnum,
												0, &cur_object_loc, 0, F1_0);
			}
			robotcen->Flag	= 1;
			robotcen->Timer = 0;
		}
		break;

	case 1:			// Wait until 1/2 second after VCLIP started.
		if (robotcen->Timer > (gameData.eff.vClips [0][vclip].xTotalTime / 2))	{
			if (!bMakeVirus)
				robotcen->Capacity -= gameData.matCens.xEnergyToCreateOneRobot;
			robotcen->Flag = 0;
			robotcen->Timer = 0;
			COMPUTE_SEGMENT_CENTER_I (&cur_object_loc, robotcen->segnum);

			// If this is the first materialization, set to valid robot.
			if (bMakeVirus ||
				 gameData.matCens.robotCenters [matcen_num].robot_flags[0] != 0 || 
				 gameData.matCens.robotCenters [matcen_num].robot_flags[1] != 0) {
				ubyte	type;
				uint	flags;
				sbyte legal_types[64];   // 64 bits, the width of robot_flags[].
				int	num_types, robot_index, i;

				if (!bMakeVirus) {
					num_types = 0;
					for (i=0;i<2;i++) {
						robot_index = i*32;
						flags = gameData.matCens.robotCenters[matcen_num].robot_flags[i];
						while (flags) {
							if (flags & 1)
								legal_types[num_types++] = robot_index;
							flags >>= 1;
							robot_index++;
							}
						}

					if (num_types == 1)
						type = legal_types[0];
					else
						type = legal_types [(d_rand() * num_types) / 32768];
#if TRACE
					con_printf (CON_DEBUG, "Morph: (type = %i) (seg = %i) (capacity = %08x)\n", type, robotcen->segnum, robotcen->Capacity);
#endif
					}
				if (bMakeVirus) {
					int objnum = CreateObject(OBJ_POWERUP, POW_ENTROPY_VIRUS, -1, (short) robotcen->segnum, &cur_object_loc, &vmd_identity_matrix, 
													gameData.objs.pwrUp.info [POW_ENTROPY_VIRUS].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);
					if (objnum >= 0) {
						objP = gameData.objs.objects + objnum;
						if (gameData.app.nGameMode & GM_MULTI)
							multiData.create.nObjNums[multiData.create.nLoc++] = objnum;
						objP->rtype.vclip_info.nClipIndex = gameData.objs.pwrUp.info[objP->id].nClipIndex;
						objP->rtype.vclip_info.xFrameTime = gameData.eff.vClips [0][objP->rtype.vclip_info.nClipIndex].xFrameTime;
						objP->rtype.vclip_info.nCurFrame = 0;
						objP->matcen_creator = gameData.segs.xSegments [robotcen->segnum].owner;
#if 1//def _DEBUG
						objP->lifeleft = IMMORTAL_TIME;
#endif
						}
					}
				else {
					objP = create_morph_robot(gameData.segs.segments + robotcen->segnum, &cur_object_loc, type);
					if (objP == NULL) {
#if TRACE
						con_printf (CON_DEBUG, "Warning: create_morph_robot returned NULL (no gameData.objs.objects left?)\n");
#endif
						}
					else {
#ifndef SHAREWARE
#ifdef NETWORK
						if (gameData.app.nGameMode & GM_MULTI)
							MultiSendCreateRobot(FUELCEN_IDX (robotcen), OBJ_IDX (objP), type);
#endif
#endif
						objP->matcen_creator = (FUELCEN_IDX (robotcen)) | 0x80;
						// Make object faces player...
						VmVecSub( &direction, &gameData.objs.console->pos,&objP->pos );
						VmVector2Matrix( &objP->orient, &direction, &objP->orient.uvec, NULL);
						MorphStart( objP );
						//robotcen->last_created_obj = obj;
						//robotcen->last_created_sig = robotcen->last_created_objP->signature;
						}
					}
				}
			}
		break;

	default:
		robotcen->Flag = 0;
		robotcen->Timer = 0;
	}
}


//-------------------------------------------------------------
// Called once per frame, replenishes fuel supply.
void FuelcenUpdateAll()
{
	int i;
	fix xAmountToReplenish = fixmul(gameData.app.xFrameTime,gameData.matCens.xFuelRefillSpeed);

	for (i=0; i<gameData.matCens.nFuelCenters; i++ )	{
		if ( gameData.matCens.fuelCenters[i].Type == SEGMENT_IS_ROBOTMAKER )	{
			if (! (gameStates.app.bGameSuspended & SUSP_ROBOTS))
				robotmaker_proc( &gameData.matCens.fuelCenters[i] );
		} else if ( gameData.matCens.fuelCenters[i].Type == SEGMENT_IS_CONTROLCEN )	{
			//controlcen_proc( &gameData.matCens.fuelCenters[i] );
	
		} else if ( (gameData.matCens.fuelCenters[i].MaxCapacity > 0) && (gameData.matCens.playerSegP!=gameData.segs.segments + gameData.matCens.fuelCenters[i].segnum) )	{
			if ( gameData.matCens.fuelCenters[i].Capacity < gameData.matCens.fuelCenters[i].MaxCapacity )	{
 				gameData.matCens.fuelCenters[i].Capacity += xAmountToReplenish;
				if ( gameData.matCens.fuelCenters[i].Capacity >= gameData.matCens.fuelCenters[i].MaxCapacity )		{
					gameData.matCens.fuelCenters[i].Capacity = gameData.matCens.fuelCenters[i].MaxCapacity;
					//gauge_message( "Fuel center is fully recharged!    " );
				}
			}
		}
	}
}

//--unused-- //-------------------------------------------------------------
//--unused-- // replenishes all fuel supplies.
//--unused-- void fuelcen_replenish_all()
//--unused-- {
//--unused-- 	int i;
//--unused--
//--unused-- 	for (i=0; i<gameData.matCens.nFuelCenters; i++ )	{
//--unused-- 		gameData.matCens.fuelCenters[i].Capacity = gameData.matCens.fuelCenters[i].MaxCapacity;
//--unused-- 	}
//--unused--
//--unused-- }

#define FUELCEN_SOUND_DELAY (f1_0/4)		//play every half second

//-------------------------------------------------------------

fix hostile_room_damage_shields (segment *segP, fix MaxAmountCanGive)
{
	static fix last_play_time=0;
	fix amount;
	xsegment	*xsegP;

if (!(gameData.app.nGameMode & GM_ENTROPY))
	return 0;
Assert(segP != NULL);
gameData.matCens.playerSegP = segP;
if (!segP)
	return 0;
xsegP = gameData.segs.xSegments + SEG_IDX (segP);
if ((xsegP->owner < 1) || (xsegP->owner == GetTeam (gameData.multi.nLocalPlayer) + 1))
	return 0;
amount = fixmul (gameData.app.xFrameTime, gameData.matCens.xFuelGiveAmount * extraGameInfo [1].entropy.nShieldDamageRate / 25);
if (amount > MaxAmountCanGive )
	amount = MaxAmountCanGive;
if (last_play_time > gameData.app.xGameTime)
	last_play_time = 0;
if (gameData.app.xGameTime > last_play_time+FUELCEN_SOUND_DELAY) {
	DigiPlaySample(SOUND_PLAYER_GOT_HIT, F1_0/2 );
#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendPlaySound(SOUND_PLAYER_GOT_HIT, F1_0/2);
#endif
	last_play_time = gameData.app.xGameTime;
	}
return amount;
}

//-------------------------------------------------------------

fix fuelcen_give_fuel(segment *segP, fix MaxAmountCanTake )
{
	short		segnum = SEG_IDX (segP);
	segment2	*seg2p = gameData.segs.segment2s + segnum;
	xsegment	*xsegp = gameData.segs.xSegments + segnum;

	static fix last_play_time=0;

	Assert( segP != NULL );
	gameData.matCens.playerSegP = segP;
	if ((gameData.app.nGameMode & GM_ENTROPY) && ((xsegp->owner < 0) || ((xsegp->owner > 0) && (xsegp->owner != GetTeam (gameData.multi.nLocalPlayer) + 1))))
		return 0;
	if ( (segP) && (seg2p->special==SEGMENT_IS_FUELCEN) )	{
		fix amount;
		DetectEscortGoalAccomplished(-4);	//	UGLY! Hack! -4 means went through fuelcen.

//		if (gameData.matCens.fuelCenters[segP->value].MaxCapacity<=0)	{
//			HUDInitMessage( "Fuelcenter %d is destroyed.", segP->value );
//			return 0;
//		}

//		if (gameData.matCens.fuelCenters[segP->value].Capacity<=0)	{
//			HUDInitMessage( "Fuelcenter %d is empty.", segP->value );
//			return 0;
//		}

		if (MaxAmountCanTake <= 0)	{
			return 0;
		}
		if (gameData.app.nGameMode & GM_ENTROPY)
			amount = fixmul (gameData.app.xFrameTime, 
								  gameData.matCens.xFuelGiveAmount * 
								  extraGameInfo [IsMultiGame].entropy.nEnergyFillRate / 25);
		else
			amount = fixmul (gameData.app.xFrameTime, gameData.matCens.xFuelGiveAmount);
		if (amount > MaxAmountCanTake )
			amount = MaxAmountCanTake;
		if (last_play_time > gameData.app.xGameTime)
			last_play_time = 0;
		if (gameData.app.xGameTime > last_play_time+FUELCEN_SOUND_DELAY) {
			DigiPlaySample( SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2 );
#ifdef NETWORK
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendPlaySound(SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2);
#endif
			last_play_time = gameData.app.xGameTime;
		}


		//HUDInitMessage( "Fuelcen %d has %d/%d fuel", segP->value,f2i(gameData.matCens.fuelCenters[segP->value].Capacity),f2i(gameData.matCens.fuelCenters[segP->value].MaxCapacity) );
		return amount;

	} else {
		return 0;
	}
}

//-------------------------------------------------------------
// DM/050904
// Repair centers
// use same values as fuel centers
fix repaircen_give_shields(segment *segP, fix MaxAmountCanTake )
{
	short		segnum = SEG_IDX (segP);
	segment2	*seg2p = gameData.segs.segment2s + segnum;
	xsegment	*xsegp = gameData.segs.xSegments + segnum;
	static fix last_play_time=0;
	fix amount;

if (gameOpts->legacy.bFuelCens)
	return 0;
Assert (segP != NULL);
if (!segP)
	return 0;
gameData.matCens.playerSegP = segP;
if ((gameData.app.nGameMode & GM_ENTROPY) && ((xsegp->owner < 0) || ((xsegp->owner > 0) && (xsegp->owner != GetTeam (gameData.multi.nLocalPlayer) + 1))))
	return 0;
if (seg2p->special != SEGMENT_IS_REPAIRCEN)
	return 0;
//		DetectEscortGoalAccomplished(-4);	//	UGLY! Hack! -4 means went through fuelcen.
//		if (gameData.matCens.fuelCenters[segP->value].MaxCapacity<=0)	{
//			HUDInitMessage( "Repaircenter %d is destroyed.", segP->value );
//			return 0;
//		}
//		if (gameData.matCens.fuelCenters[segP->value].Capacity<=0)	{
//			HUDInitMessage( "Repaircenter %d is empty.", segP->value );
//			return 0;
//		}
if (MaxAmountCanTake <= 0 )	{
	return 0;
}
amount = fixmul (gameData.app.xFrameTime, gameData.matCens.xFuelGiveAmount * extraGameInfo [IsMultiGame].entropy.nShieldFillRate / 25);
if (amount > MaxAmountCanTake )
	amount = MaxAmountCanTake;
if (last_play_time > gameData.app.xGameTime)
	last_play_time = 0;
if (gameData.app.xGameTime > last_play_time+FUELCEN_SOUND_DELAY) {
	DigiPlaySample( SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2 );
#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendPlaySound(SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2);
#endif
	last_play_time = gameData.app.xGameTime;
	}
return amount;
}

//--unused-- //-----------------------------------------------------------
//--unused-- // Damages a fuel center
//--unused-- void fuelcen_damage(segment *segP, fix damage )
//--unused-- {
//--unused-- 	//int i;
//--unused-- 	// int	station_num = segP->value;
//--unused--
//--unused-- 	Assert( segP != NULL );
//--unused-- 	if ( segP == NULL ) return;
//--unused--
//--unused-- 	switch( segP->special )	{
//--unused-- 	case SEGMENT_IS_NOTHING:
//--unused-- 		return;
//--unused-- 	case SEGMENT_IS_ROBOTMAKER:
//--unused-- //--		// Robotmaker hit by laser
//--unused-- //--		if (gameData.matCens.fuelCenters[station_num].MaxCapacity<=0 )	{
//--unused-- //--			// Shooting a already destroyed materializer
//--unused-- //--		} else {
//--unused-- //--			gameData.matCens.fuelCenters[station_num].MaxCapacity -= damage;
//--unused-- //--			if (gameData.matCens.fuelCenters[station_num].Capacity > gameData.matCens.fuelCenters[station_num].MaxCapacity )	{
//--unused-- //--				gameData.matCens.fuelCenters[station_num].Capacity = gameData.matCens.fuelCenters[station_num].MaxCapacity;
//--unused-- //--			}
//--unused-- //--			if (gameData.matCens.fuelCenters[station_num].MaxCapacity <= 0 )	{
//--unused-- //--				gameData.matCens.fuelCenters[station_num].MaxCapacity = 0;
//--unused-- //--				// Robotmaker dead
//--unused-- //--				for (i=0; i<6; i++ )
//--unused-- //--					segP->sides[i].tmap_num2 = 0;
//--unused-- //--			}
//--unused-- //--		}
//--unused-- 		break;
//--unused-- 	case SEGMENT_IS_FUELCEN:	
//--unused-- //--		DigiPlaySample( SOUND_REFUEL_STATION_HIT );
//--unused-- //--		if (gameData.matCens.fuelCenters[station_num].MaxCapacity>0 )	{
//--unused-- //--			gameData.matCens.fuelCenters[station_num].MaxCapacity -= damage;
//--unused-- //--			if (gameData.matCens.fuelCenters[station_num].Capacity > gameData.matCens.fuelCenters[station_num].MaxCapacity )	{
//--unused-- //--				gameData.matCens.fuelCenters[station_num].Capacity = gameData.matCens.fuelCenters[station_num].MaxCapacity;
//--unused-- //--			}
//--unused-- //--			if (gameData.matCens.fuelCenters[station_num].MaxCapacity <= 0 )	{
//--unused-- //--				gameData.matCens.fuelCenters[station_num].MaxCapacity = 0;
//--unused-- //--				DigiPlaySample( SOUND_REFUEL_STATION_DESTROYED );
//--unused-- //--			}
//--unused-- //--		} else {
//--unused-- //--			gameData.matCens.fuelCenters[station_num].MaxCapacity = 0;
//--unused-- //--		}
//--unused-- //--		HUDInitMessage( "Fuelcenter %d damaged", station_num );
//--unused-- 		break;
//--unused-- 	case SEGMENT_IS_REPAIRCEN:
//--unused-- 		break;
//--unused-- 	case SEGMENT_IS_CONTROLCEN:
//--unused-- 		break;
//--unused-- 	default:
//--unused-- 		Error( "Invalid type in fuelcen.c" );
//--unused-- 	}
//--unused-- }

//--unused-- //	----------------------------------------------------------------------------------------------------------
//--unused-- fixang my_delta_ang(fixang a,fixang b)
//--unused-- {
//--unused-- 	fixang delta0,delta1;
//--unused--
//--unused-- 	return (abs(delta0 = a - b) < abs(delta1 = b - a)) ? delta0 : delta1;
//--unused--
//--unused-- }

//--unused-- //	----------------------------------------------------------------------------------------------------------
//--unused-- //return though which side of seg0 is seg1
//--unused-- int john_find_connect_side(int seg0,int seg1)
//--unused-- {
//--unused-- 	segment *Seg=&gameData.segs.segments[seg0];
//--unused-- 	int i;
//--unused--
//--unused-- 	for (i=MAX_SIDES_PER_SEGMENT;i--;) if (Seg->children[i]==seg1) return i;
//--unused--
//--unused-- 	return -1;
//--unused-- }

//	----------------------------------------------------------------------------------------------------------
//--unused-- vms_angvec start_angles, delta_angles, goal_angles;
//--unused-- vms_vector start_pos, delta_pos, goal_pos;
//--unused-- int FuelStationSeg;
//--unused-- fix current_time,delta_time;
//--unused-- int next_side, side_index;
//--unused-- int * sidelist;

//--repair-- int Repairing;
//--repair-- vms_vector repair_save_uvec;		//the player's upvec when enter repaircen
//--repair-- object *RepairObj=NULL;		//which object getting repaired
//--repair-- int disable_repair_center=0;
//--repair-- fix repair_rate;
//--repair-- #define FULL_REPAIR_RATE i2f(10)

//--unused-- ubyte save_control_type,save_movement_type;

//--unused-- int SideOrderBack[] = {WFRONT, WRIGHT, WTOP, WLEFT, WBOTTOM, WBACK};
//--unused-- int SideOrderFront[] =  {WBACK, WLEFT, WTOP, WRIGHT, WBOTTOM, WFRONT};
//--unused-- int SideOrderLeft[] =  { WRIGHT, WBACK, WTOP, WFRONT, WBOTTOM, WLEFT };
//--unused-- int SideOrderRight[] =  { WLEFT, WFRONT, WTOP, WBACK, WBOTTOM, WRIGHT };
//--unused-- int SideOrderTop[] =  { WBOTTOM, WLEFT, WBACK, WRIGHT, WFRONT, WTOP };
//--unused-- int SideOrderBottom[] =  { WTOP, WLEFT, WFRONT, WRIGHT, WBACK, WBOTTOM };

//--unused-- int SideUpVector[] = {WBOTTOM, WFRONT, WBOTTOM, WFRONT, WBOTTOM, WBOTTOM };

//--repair-- //	----------------------------------------------------------------------------------------------------------
//--repair-- void refuel_calc_deltas(object *objP, int next_side, int repair_seg)
//--repair-- {
//--repair-- 	vms_vector nextcenter, headfvec, *headuvec;
//--repair-- 	vms_matrix goal_orient;
//--repair--
//--repair-- 	// Find time for this movement
//--repair-- 	delta_time = F1_0;		// one second...
//--repair-- 		
//--repair-- 	// Find start and goal position
//--repair-- 	start_pos = objP->pos;
//--repair-- 	
//--repair-- 	// Find delta position to get to goal position
//--repair-- 	COMPUTE_SEGMENT_CENTER(&goal_pos,&gameData.segs.segments[repair_seg]);
//--repair-- 	VmVecSub( &delta_pos,&goal_pos,&start_pos);
//--repair-- 	
//--repair-- 	// Find start angles
//--repair-- 	//angles_from_vector(&start_angles,&objP->orient.fvec);
//--repair-- 	VmExtractAnglesMatrix(&start_angles,&objP->orient);
//--repair-- 	
//--repair-- 	// Find delta angles to get to goal orientation
//--repair-- 	med_compute_center_point_on_side(&nextcenter,&gameData.segs.segments[repair_seg],next_side);
//--repair-- 	VmVecSub(&headfvec,&nextcenter,&goal_pos);
//--repair--
//--repair-- 	if (next_side == 5)						//last side
//--repair-- 		headuvec = &repair_save_uvec;
//--repair-- 	else
//--repair-- 		headuvec = &gameData.segs.segments[repair_seg].sides[SideUpVector[next_side]].normals[0];
//--repair--
//--repair-- 	VmVector2Matrix(&goal_orient,&headfvec,headuvec,NULL);
//--repair-- 	VmExtractAnglesMatrix(&goal_angles,&goal_orient);
//--repair-- 	delta_angles.p = my_delta_ang(start_angles.p,goal_angles.p);
//--repair-- 	delta_angles.b = my_delta_ang(start_angles.b,goal_angles.b);
//--repair-- 	delta_angles.h = my_delta_ang(start_angles.h,goal_angles.h);
//--repair-- 	current_time = 0;
//--repair-- 	Repairing = 0;
//--repair-- }
//--repair--
//--repair-- //	----------------------------------------------------------------------------------------------------------
//--repair-- //if repairing, cut it short
//--repair-- abort_repair_center()
//--repair-- {
//--repair-- 	if (!RepairObj || side_index==5)
//--repair-- 		return;
//--repair--
//--repair-- 	current_time = 0;
//--repair-- 	side_index = 5;
//--repair-- 	next_side = sidelist[side_index];
//--repair-- 	refuel_calc_deltas(RepairObj, next_side, FuelStationSeg);
//--repair-- }
//--repair--
//--repair-- //	----------------------------------------------------------------------------------------------------------
//--repair-- void repair_ship_damage()
//--repair-- {
//--repair-- }
//--repair--
//--repair-- //	----------------------------------------------------------------------------------------------------------
//--repair-- int refuel_do_repair_effect( object * objP, int first_time, int repair_seg )	{
//--repair--
//--repair-- 	objP->mtype.phys_info.velocity.x = 0;				
//--repair-- 	objP->mtype.phys_info.velocity.y = 0;				
//--repair-- 	objP->mtype.phys_info.velocity.z = 0;				
//--repair--
//--repair-- 	if (first_time)	{
//--repair-- 		int entry_side;
//--repair-- 		current_time = 0;
//--repair--
//--repair-- 		DigiPlaySample( SOUND_REPAIR_STATION_PLAYER_ENTERING, F1_0 );
//--repair--
//--repair-- 		entry_side = john_find_connect_side(repair_seg,objP->segnum );
//--repair-- 		Assert( entry_side > -1 );
//--repair--
//--repair-- 		switch( entry_side )	{
//--repair-- 		case WBACK: sidelist = SideOrderBack; break;
//--repair-- 		case WFRONT: sidelist = SideOrderFront; break;
//--repair-- 		case WLEFT: sidelist = SideOrderLeft; break;
//--repair-- 		case WRIGHT: sidelist = SideOrderRight; break;
//--repair-- 		case WTOP: sidelist = SideOrderTop; break;
//--repair-- 		case WBOTTOM: sidelist = SideOrderBottom; break;
//--repair-- 		}
//--repair-- 		side_index = 0;
//--repair-- 		next_side = sidelist[side_index];
//--repair--
//--repair-- 		refuel_calc_deltas(objP,next_side, repair_seg);
//--repair-- 	}
//--repair--
//--repair-- 	//update shields
//--repair-- 	if (gameData.multi.players[gameData.multi.nLocalPlayer].shields < MAX_SHIELDS) {	//if above max, don't mess with it
//--repair--
//--repair-- 		gameData.multi.players[gameData.multi.nLocalPlayer].shields += fixmul(gameData.app.xFrameTime,repair_rate);
//--repair--
//--repair-- 		if (gameData.multi.players[gameData.multi.nLocalPlayer].shields > MAX_SHIELDS)
//--repair-- 			gameData.multi.players[gameData.multi.nLocalPlayer].shields = MAX_SHIELDS;
//--repair-- 	}
//--repair--
//--repair-- 	current_time += gameData.app.xFrameTime;
//--repair--
//--repair-- 	if (current_time >= delta_time )	{
//--repair-- 		vms_angvec av;
//--repair-- 		objP->pos = goal_pos;
//--repair-- 		av	= goal_angles;
//--repair-- 		VmAngles2Matrix(&objP->orient,&av);
//--repair--
//--repair-- 		if (side_index >= 5 )	
//--repair-- 			return 1;		// Done being repaired...
//--repair--
//--repair-- 		if (Repairing==0)		{
//--repair-- 			//DigiPlaySample( SOUND_REPAIR_STATION_FIXING );
//--repair-- 			Repairing=1;
//--repair--
//--repair-- 			switch( next_side )	{
//--repair-- 			case 0:	DigiPlaySample( SOUND_REPAIR_STATION_FIXING_1,F1_0 ); break;
//--repair-- 			case 1:	DigiPlaySample( SOUND_REPAIR_STATION_FIXING_2,F1_0 ); break;
//--repair-- 			case 2:	DigiPlaySample( SOUND_REPAIR_STATION_FIXING_3,F1_0 ); break;
//--repair-- 			case 3:	DigiPlaySample( SOUND_REPAIR_STATION_FIXING_4,F1_0 ); break;
//--repair-- 			case 4:	DigiPlaySample( SOUND_REPAIR_STATION_FIXING_1,F1_0 ); break;
//--repair-- 			case 5:	DigiPlaySample( SOUND_REPAIR_STATION_FIXING_2,F1_0 ); break;
//--repair-- 			}
//--repair-- 		
//--repair-- 			repair_ship_damage();
//--repair--
//--repair-- 		}
//--repair--
//--repair-- 		if (current_time >= (delta_time+(F1_0/2)) )	{
//--repair-- 			current_time = 0;
//--repair-- 			// Find next side...
//--repair-- 			side_index++;
//--repair-- 			if (side_index >= 6 ) return 1;
//--repair-- 			next_side = sidelist[side_index];
//--repair-- 	
//--repair-- 			refuel_calc_deltas(objP, next_side, repair_seg);
//--repair-- 		}
//--repair--
//--repair-- 	} else {
//--repair-- 		fix factor, p,b,h;	
//--repair-- 		vms_angvec av;
//--repair--
//--repair-- 		factor = fixdiv( current_time,delta_time );
//--repair--
//--repair-- 		// Find object's current position
//--repair-- 		objP->pos = delta_pos;
//--repair-- 		VmVecScale( &objP->pos, factor );
//--repair-- 		VmVecInc( &objP->pos, &start_pos );
//--repair-- 			
//--repair-- 		// Find object's current orientation
//--repair-- 		p	= fixmul(delta_angles.p,factor);
//--repair-- 		b	= fixmul(delta_angles.b,factor);
//--repair-- 		h	= fixmul(delta_angles.h,factor);
//--repair-- 		av.p = (fixang)p + start_angles.p;
//--repair-- 		av.b = (fixang)b + start_angles.b;
//--repair-- 		av.h = (fixang)h + start_angles.h;
//--repair-- 		VmAngles2Matrix(&objP->orient,&av);
//--repair--
//--repair-- 	}
//--repair--
//--repair-- 	UpdateObjectSeg(objP);		//update segment
//--repair--
//--repair-- 	return 0;
//--repair-- }
//--repair--
//--repair-- //	----------------------------------------------------------------------------------------------------------
//--repair-- //do the repair center for this frame
//--repair-- void do_repair_sequence(object *objP)
//--repair-- {
//--repair-- 	Assert(obj == RepairObj);
//--repair--
//--repair-- 	if (refuel_do_repair_effect( objP, 0, FuelStationSeg )) {
//--repair-- 		if (gameData.multi.players[gameData.multi.nLocalPlayer].shields < MAX_SHIELDS)
//--repair-- 			gameData.multi.players[gameData.multi.nLocalPlayer].shields = MAX_SHIELDS;
//--repair-- 		objP->control_type = save_control_type;
//--repair-- 		objP->movement_type = save_movement_type;
//--repair-- 		disable_repair_center=1;
//--repair-- 		RepairObj = NULL;
//--repair--
//--repair--
//--repair-- 		//the two lines below will spit the player out of the rapair center,
//--repair-- 		//but what happen is that the ship just bangs into the door
//--repair-- 		//if (objP->movement_type == MT_PHYSICS)
//--repair-- 		//	VmVecCopyScale(&objP->mtype.phys_info.velocity,&objP->orient.fvec,i2f(200);
//--repair-- 	}
//--repair--
//--repair-- }
//--repair--
//--repair-- //	----------------------------------------------------------------------------------------------------------
//--repair-- //see if we should start the repair center
//--repair-- void check_start_repair_center(object *objP)
//--repair-- {
//--repair-- 	if (RepairObj != NULL) return;		//already in repair center
//--repair--
//--repair-- 	if (Lsegments[objP->segnum].special_type & SS_REPAIR_CENTER) {
//--repair--
//--repair-- 		if (!disable_repair_center) {
//--repair-- 			//have just entered repair center
//--repair--
//--repair-- 			RepairObj = obj;
//--repair-- 			repair_save_uvec = objP->orient.uvec;
//--repair--
//--repair-- 			repair_rate = fixmuldiv(FULL_REPAIR_RATE,(MAX_SHIELDS - gameData.multi.players[gameData.multi.nLocalPlayer].shields),MAX_SHIELDS);
//--repair--
//--repair-- 			save_control_type = objP->control_type;
//--repair-- 			save_movement_type = objP->movement_type;
//--repair--
//--repair-- 			objP->control_type = CT_REPAIRCEN;
//--repair-- 			objP->movement_type = MT_NONE;
//--repair--
//--repair-- 			FuelStationSeg	= Lsegments[objP->segnum].special_segment;
//--repair-- 			Assert(FuelStationSeg != -1);
//--repair--
//--repair-- 			if (refuel_do_repair_effect( objP, 1, FuelStationSeg )) {
//--repair-- 				Int3();		//can this happen?
//--repair-- 				objP->control_type = CT_FLYING;
//--repair-- 				objP->movement_type = MT_PHYSICS;
//--repair-- 			}
//--repair-- 		}
//--repair-- 	}
//--repair-- 	else
//--repair-- 		disable_repair_center=0;
//--repair--
//--repair-- }

//	--------------------------------------------------------------------------------------------
void disable_matcens(void)
{
	int	i;

	for (i=0; i<gameData.matCens.nRobotCenters; i++) {
		gameData.matCens.fuelCenters[i].Enabled = 0;
		gameData.matCens.fuelCenters[i].Disable_time = 0;
	}
}

//	--------------------------------------------------------------------------------------------
//	Initialize all materialization centers.
//	Give them all the right number of lives.
void InitAllMatCens(void)
{
	int	i;

	for (i=0; i<gameData.matCens.nFuelCenters; i++)
		if (gameData.matCens.fuelCenters[i].Type == SEGMENT_IS_ROBOTMAKER) {
			gameData.matCens.fuelCenters[i].Lives = 3;
			gameData.matCens.fuelCenters[i].Enabled = 0;
			gameData.matCens.fuelCenters[i].Disable_time = 0;
#ifndef NDEBUG
{
			//	Make sure this fuelcen is pointed at by a matcen.
			int	j;
			for (j=0; j<gameData.matCens.nRobotCenters; j++) {
				if (gameData.matCens.robotCenters[j].fuelcen_num == i)
					break;
			}
			Assert(j != gameData.matCens.nRobotCenters);
}
#endif

		}

#ifndef NDEBUG
	//	Make sure all matcens point at a fuelcen
	for (i=0; i<gameData.matCens.nRobotCenters; i++) {
		int	fuelcen_num = gameData.matCens.robotCenters[i].fuelcen_num;

		Assert(fuelcen_num < gameData.matCens.nFuelCenters);
		Assert(gameData.matCens.fuelCenters[fuelcen_num].Type == SEGMENT_IS_ROBOTMAKER);
	}
#endif

}

//-----------------------------------------------------------------------------

short flag_goal_list [MAX_SEGMENTS];
short flag_goal_roots [2] = {-1, -1};
short blue_flag_goals = -1;

// create linked lists of all segments with special type blue and red goal

void GatherFlagGoals (void)
{
	int			i, j;
	segment2		*seg2P = gameData.segs.segment2s;

memset (flag_goal_list, 0xff, sizeof (flag_goal_list));
for (i = 0; i <= gameData.segs.nLastSegment; i++, seg2P++) {
	if (seg2P->special == SEGMENT_IS_GOAL_BLUE)
		j = 0;
	else if (seg2P->special == SEGMENT_IS_GOAL_RED)
		j = 1;
	else
		continue;
	flag_goal_list [i] = flag_goal_roots [j];
	flag_goal_roots [j] = i;
	}
}

//--------------------------------------------------------------------

#ifdef NETWORK
extern void MultiSendCaptureBonus (char);

int flag_at_home (int nFlagId)
{
	int		i, j;
	object	*objP;

for (i = flag_goal_roots [nFlagId - POW_FLAG_BLUE]; i >= 0; i = flag_goal_list [i])
	for (j = gameData.segs.segments [i].objects; j >= 0; j = objP->next) {
		objP = gameData.objs.objects + j;
		if ((objP->type == OBJ_POWERUP) && (objP->id == nFlagId))
			return 1;
		}
return 0;
}

//--------------------------------------------------------------------

int CheckFlagDrop (segment2 *pSeg, int nTeamId, int nFlagId, int nGoalId)
{
if (pSeg->special != nGoalId)
	return 0;
if (GetTeam (gameData.multi.nLocalPlayer) != nTeamId)
	return 0;
if (!(gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_FLAG))
	return 0;
if (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bEnhancedCTF && !flag_at_home ((nFlagId == POW_FLAG_BLUE) ? POW_FLAG_RED : POW_FLAG_BLUE))
	return 0;
MultiSendCaptureBonus ((char) gameData.multi.nLocalPlayer);
gameData.multi.players[gameData.multi.nLocalPlayer].flags &=(~(PLAYER_FLAGS_FLAG));
MaybeDropNetPowerup (-1, nFlagId, FORCE_DROP);
return 1;
}

//--------------------------------------------------------------------

void fuelcen_check_for_goal(segment *segP)
{
	segment2	*seg2p = &gameData.segs.segment2s[SEG_IDX (segP)];

	Assert( segP != NULL );
	Assert (gameData.app.nGameMode & GM_CAPTURE);

#if 1
CheckFlagDrop (seg2p, TEAM_BLUE, POW_FLAG_RED, SEGMENT_IS_GOAL_BLUE);
CheckFlagDrop (seg2p, TEAM_RED, POW_FLAG_BLUE, SEGMENT_IS_GOAL_RED);
#else
if (seg2p->special==SEGMENT_IS_GOAL_BLUE)	{
	if ((GetTeam(gameData.multi.nLocalPlayer)==TEAM_BLUE) && 
		 (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_FLAG))
		  flag_at_home (POW_FLAG_BLUE)) {		
		{
#if TRACE
		con_printf (CON_DEBUG,"In goal segment BLUE\n");
#endif
		MultiSendCaptureBonus (gameData.multi.nLocalPlayer);
		gameData.multi.players[gameData.multi.nLocalPlayer].flags &=(~(PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_FLAG_RED, FORCE_DROP);
		}
	}
else if ( seg2p->special==SEGMENT_IS_GOAL_RED) {
	if ((GetTeam(gameData.multi.nLocalPlayer)==TEAM_RED) && 
		 (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_FLAG)) &&
		 flag_at_home (POW_FLAG_RED)) {		
#if TRACE
		con_printf (CON_DEBUG,"In goal segment RED\n");
#endif
		MultiSendCaptureBonus (gameData.multi.nLocalPlayer);
		gameData.multi.players[gameData.multi.nLocalPlayer].flags &=(~(PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_FLAG_BLUE, FORCE_DROP);
		}
	}
#endif
}

//--------------------------------------------------------------------

void fuelcen_check_for_hoard_goal(segment *segP)
{
	segment2	*seg2p = &gameData.segs.segment2s[SEG_IDX (segP)];

	Assert( segP != NULL );
	Assert (gameData.app.nGameMode & GM_HOARD);

   if (gameStates.app.bPlayerIsDead)
		return;

	if (seg2p->special==SEGMENT_IS_GOAL_BLUE || seg2p->special==SEGMENT_IS_GOAL_RED  )	
	{
		if (gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[PROXIMITY_INDEX])
		{
#if TRACE
				con_printf (CON_DEBUG,"In orb goal!\n");
#endif
				multi_send_orb_bonus ((char) gameData.multi.nLocalPlayer);
				gameData.multi.players[gameData.multi.nLocalPlayer].flags &=(~(PLAYER_FLAGS_FLAG));
				gameData.multi.players[gameData.multi.nLocalPlayer].secondary_ammo[PROXIMITY_INDEX]=0;
      }
	}

}

#endif

//--------------------------------------------------------------------
#ifndef FAST_FILE_IO
/*
 * reads an old_matcen_info structure from a CFILE
 */
void old_matcen_info_read(old_matcen_info *mi, CFILE *fp)
{
	mi->robot_flags = CFReadInt(fp);
	mi->hit_points = CFReadFix(fp);
	mi->interval = CFReadFix(fp);
	mi->segnum = CFReadShort(fp);
	mi->fuelcen_num = CFReadShort(fp);
}

/*
 * reads a matcen_info structure from a CFILE
 */
void matcen_info_read(matcen_info *mi, CFILE *fp)
{
	mi->robot_flags[0] = CFReadInt(fp);
	mi->robot_flags[1] = CFReadInt(fp);
	mi->hit_points = CFReadFix(fp);
	mi->interval = CFReadFix(fp);
	mi->segnum = CFReadShort(fp);
	mi->fuelcen_num = CFReadShort(fp);
}
#endif
