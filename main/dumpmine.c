/* $Id: dumpmine.c,v 1.4 2003/10/10 09:36:34 btb Exp $ */
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
 * Functions to dump text description of mine.
 * An editor-only function, called at mine load time.
 * To be read by a human to verify the correctness and completeness of a mine.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:24:16  allender
 * Initial revision
 *
 * Revision 2.1  1995/04/06  12:21:50  mike
 * Add texture map information to txm files.
 *
 * Revision 2.0  1995/02/27  11:26:41  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.24  1995/01/23  15:34:43  mike
 * New diagnostic code, levels.all stuff.
 *
 * Revision 1.23  1994/12/20  17:56:36  yuan
 * Multiplayer object capability.
 *
 * Revision 1.22  1994/11/27  23:12:19  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.21  1994/11/23  12:19:04  mike
 * move out level names, stick in gamesave.
 *
 * Revision 1.20  1994/11/21  16:54:36  mike
 * oops.
 *
 *
 * Revision 1.19  1994/11/20  22:12:55  mike
 * Lotsa new stuff in this fine debug file.
 *
 * Revision 1.18  1994/11/17  14:58:09  mike
 * moved segment validation functions from editor to main.
 *
 * Revision 1.17  1994/11/15  21:43:02  mike
 * texture usage system.
 *
 * Revision 1.16  1994/11/15  12:45:59  mike
 * debug code for dumping texture info.
 *
 * Revision 1.15  1994/11/14  20:47:50  john
 * Attempted to strip out all the code in the game
 * directory that uses any ui code.
 *
 * Revision 1.14  1994/10/14  17:33:38  mike
 * Fix error reporting for number of multiplayer gameData.objs.objects in mine.
 *
 * Revision 1.13  1994/10/14  13:37:46  mike
 * Forgot parameter in fprintf, was getting bogus number of excess keys.
 *
 * Revision 1.12  1994/10/12  08:05:33  mike
 * Detect keys contained in gameData.objs.objects for error checking (txm file).
 *
 * Revision 1.11  1994/10/10  17:02:08  mike
 * fix fix.
 *
 * Revision 1.10  1994/10/10  17:00:37  mike
 * Add checking for proper number of players.
 *
 * Revision 1.9  1994/10/03  23:37:19  mike
 * Adapt to clear and rational understanding of matcens as related to fuelcens as related to something that might work.
 *
 * Revision 1.8  1994/09/30  17:15:29  mike
 * Fix error message, was telling bogus filename.
 *
 * Revision 1.7  1994/09/30  11:50:55  mike
 * More diagnostics.
 *
 * Revision 1.6  1994/09/28  17:31:19  mike
 * More error checking.
 *
 * Revision 1.5  1994/09/28  11:14:05  mike
 * Better checking on bogus walls.
 *
 * Revision 1.4  1994/09/28  09:23:50  mike
 * Change some Error messages to Warnings.
 *
 * Revision 1.3  1994/09/27  17:08:31  mike
 * More mine validation stuff.
 *
 * Revision 1.2  1994/09/27  15:43:22  mike
 * The amazing code to tell you everything and more about our mines!
 *
 * Revision 1.1  1994/09/27  10:51:15  mike
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: dumpmine.c,v 1.4 2003/10/10 09:36:34 btb Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "pstypes.h"
#include "mono.h"
#include "key.h"
#include "gr.h"
#include "palette.h"

#include "inferno.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif
#include "error.h"
#include "object.h"
#include "wall.h"
#include "gamemine.h"
#include "robot.h"
#include "player.h"
#include "newmenu.h"
#include "textures.h"

#include "bm.h"
#include "menu.h"
#include "switch.h"
#include "fuelcen.h"
#include "powerup.h"
#include "gameseq.h"
#include "polyobj.h"
#include "gamesave.h"


#ifdef EDITOR

extern ubyte bogus_data[1024*1024];
extern grs_bitmap bogus_bitmap;

// ----------------------------------------------------------------------------
char	*object_types(int objnum)
{
	int	type = gameData.objs.objects[objnum].type;

	Assert((type >= 0) && (type < MAX_OBJECT_TYPES);
	return	&szObjectTypeNames[type];
}

// ----------------------------------------------------------------------------
char	*object_ids(int objnum)
{
	int	type = gameData.objs.objects[objnum].type;
	int	id = gameData.objs.objects[objnum].id;

	switch (type) {
		case OBJ_ROBOT:
			return &gameData.bots.names[id];
			break;
		case OBJ_POWERUP:
			return &Powerup_names[id];
			break;
	}

	return	NULL;
}

void err_printf(FILE *my_file, char * format, ... )
{
	va_list	args;
	char		message[256];

	va_start(args, format );
	vsprintf(message,format,args);
	va_end(args);
#if TRACE
	con_printf (1, "%s", message);
#endif	
	fprintf(my_file, "%s", message);
	Errors_in_mine++;
}

void warning_printf(FILE *my_file, char * format, ... )
{
	va_list	args;
	char		message[256];

	va_start(args, format );
	vsprintf(message,format,args);
	va_end(args);
#if TRACE
	con_printf (CON_DEBUG, "%s", message);
#endif	
	fprintf(my_file, "%s", message);
}

// ----------------------------------------------------------------------------
void write_exit_text(FILE *my_file)
{
	int	i, j, count;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "Exit stuff\n");

	//	---------- Find exit triggers ----------
	count=0;
	for (i=0; i<gameData.trigs.nTriggers; i++)
		if (gameData.trigs.triggers[i].type == TT_EXIT) {
			int	count2;

			fprintf(my_file, "Trigger %2i, is an exit trigger with %i links.\n", i, gameData.trigs.triggers[i].num_links);
			count++;
			if (gameData.trigs.triggers[i].num_links != 0)
				err_printf(my_file, "Error: Exit triggers must have 0 links, this one has %i links.\n", gameData.trigs.triggers[i].num_links);

			//	Find wall pointing to this trigger.
			count2 = 0;
			for (j=0; j<gameData.walls.nWalls; j++)
				if (gameData.walls.walls[j].trigger == i) {
					count2++;
					fprintf(my_file, "Exit trigger %i is in segment %i, on side %i, bound to wall %i\n", i, gameData.walls.walls[j].segnum, gameData.walls.walls[j].sidenum, j);
				}
			if (count2 == 0)
				err_printf(my_file, "Error: Trigger %i is not bound to any wall.\n", i);
			else if (count2 > 1)
				err_printf(my_file, "Error: Trigger %i is bound to %i walls.\n", i, count2);

		}

	if (count == 0)
		err_printf(my_file, "Error: No exit trigger in this mine.\n");
	else if (count != 1)
		err_printf(my_file, "Error: More than one exit trigger in this mine.\n");
	else
		fprintf(my_file, "\n");

	//	---------- Find exit doors ----------
	count = 0;
	for (i=0; i<=gameData.segs.nLastSegment; i++)
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (gameData.segs.segments[i].children[j] == -2) {
				fprintf(my_file, "Segment %3i, side %i is an exit door.\n", i, j);
				count++;
			}

	if (count == 0)
		err_printf(my_file, "Error: No external wall in this mine.\n");
	else if (count != 1) {
		// -- warning_printf(my_file, "Warning: %i external walls in this mine.\n", count);
		// -- warning_printf(my_file, "(If %i are secret exits, then no problem.)\n", count-1);
	} else
		fprintf(my_file, "\n");
}

// ----------------------------------------------------------------------------
void write_key_text(FILE *my_file)
{
	int	i;
	int	red_count, blue_count, gold_count;
	int	red_count2, blue_count2, gold_count2;
	int	blue_segnum=-1, blue_sidenum=-1, red_segnum=-1, red_sidenum=-1, gold_segnum=-1, gold_sidenum=-1;
	int	connect_side;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "Key stuff:\n");

	red_count = 0;
	blue_count = 0;
	gold_count = 0;

	for (i=0; i<gameData.walls.nWalls; i++) {
		if (gameData.walls.walls[i].keys & KEY_BLUE) {
			fprintf(my_file, "Wall %i (seg=%i, side=%i) is keyed to the blue key.\n", i, gameData.walls.walls[i].segnum, gameData.walls.walls[i].sidenum);
			if (blue_segnum == -1) {
				blue_segnum = gameData.walls.walls[i].segnum;
				blue_sidenum = gameData.walls.walls[i].sidenum;
				blue_count++;
			} else {
				connect_side = FindConnectedSide(&gameData.segs.segments[gameData.walls.walls[i].segnum], &gameData.segs.segments[blue_segnum]);
				if (connect_side != blue_sidenum) {
					warning_printf(my_file, "Warning: This blue door at seg %i, is different than the one at seg %i, side %i\n", gameData.walls.walls[i].segnum, blue_segnum, blue_sidenum);
					blue_count++;
				}
			}
		}
		if (gameData.walls.walls[i].keys & KEY_RED) {
			fprintf(my_file, "Wall %i (seg=%i, side=%i) is keyed to the red key.\n", i, gameData.walls.walls[i].segnum, gameData.walls.walls[i].sidenum);
			if (red_segnum == -1) {
				red_segnum = gameData.walls.walls[i].segnum;
				red_sidenum = gameData.walls.walls[i].sidenum;
				red_count++;
			} else {
				connect_side = FindConnectedSide(&gameData.segs.segments[gameData.walls.walls[i].segnum], &gameData.segs.segments[red_segnum]);
				if (connect_side != red_sidenum) {
					warning_printf(my_file, "Warning: This red door at seg %i, is different than the one at seg %i, side %i\n", gameData.walls.walls[i].segnum, red_segnum, red_sidenum);
					red_count++;
				}
			}
		}
		if (gameData.walls.walls[i].keys & KEY_GOLD) {
			fprintf(my_file, "Wall %i (seg=%i, side=%i) is keyed to the gold key.\n", i, gameData.walls.walls[i].segnum, gameData.walls.walls[i].sidenum);
			if (gold_segnum == -1) {
				gold_segnum = gameData.walls.walls[i].segnum;
				gold_sidenum = gameData.walls.walls[i].sidenum;
				gold_count++;
			} else {
				connect_side = FindConnectedSide(&gameData.segs.segments[gameData.walls.walls[i].segnum], &gameData.segs.segments[gold_segnum]);
				if (connect_side != gold_sidenum) {
					warning_printf(my_file, "Warning: This gold door at seg %i, is different than the one at seg %i, side %i\n", gameData.walls.walls[i].segnum, gold_segnum, gold_sidenum);
					gold_count++;
				}
			}
		}
	}

	if (blue_count > 1)
		warning_printf(my_file, "Warning: %i doors are keyed to the blue key.\n", blue_count);

	if (red_count > 1)
		warning_printf(my_file, "Warning: %i doors are keyed to the red key.\n", red_count);

	if (gold_count > 1)
		warning_printf(my_file, "Warning: %i doors are keyed to the gold key.\n", gold_count);

	red_count2 = 0;
	blue_count2 = 0;
	gold_count2 = 0;

	for (i=0; i<=gameData.objs.nLastObject; i++) {
		if (gameData.objs.objects[i].type == OBJ_POWERUP)
			if (gameData.objs.objects[i].id == POW_KEY_BLUE) {
				fprintf(my_file, "The BLUE key is object %i in segment %i\n", i, gameData.objs.objects[i].segnum);
				blue_count2++;
			}
		if (gameData.objs.objects[i].type == OBJ_POWERUP)
			if (gameData.objs.objects[i].id == POW_KEY_RED) {
				fprintf(my_file, "The RED key is object %i in segment %i\n", i, gameData.objs.objects[i].segnum);
				red_count2++;
			}
		if (gameData.objs.objects[i].type == OBJ_POWERUP)
			if (gameData.objs.objects[i].id == POW_KEY_GOLD) {
				fprintf(my_file, "The GOLD key is object %i in segment %i\n", i, gameData.objs.objects[i].segnum);
				gold_count2++;
			}

		if (gameData.objs.objects[i].contains_count) {
			if (gameData.objs.objects[i].contains_type == OBJ_POWERUP) {
				switch (gameData.objs.objects[i].contains_id) {
					case POW_KEY_BLUE:
						fprintf(my_file, "The BLUE key is contained in object %i (a %s %s) in segment %i\n", i, szObjectTypeNames[gameData.objs.objects[i].type], gameData.bots.names[gameData.objs.objects[i].id], gameData.objs.objects[i].segnum);
						blue_count2 += gameData.objs.objects[i].contains_count;
						break;
					case POW_KEY_GOLD:
						fprintf(my_file, "The GOLD key is contained in object %i (a %s %s) in segment %i\n", i, szObjectTypeNames[gameData.objs.objects[i].type], gameData.bots.names[gameData.objs.objects[i].id], gameData.objs.objects[i].segnum);
						gold_count2 += gameData.objs.objects[i].contains_count;
						break;
					case POW_KEY_RED:
						fprintf(my_file, "The RED key is contained in object %i (a %s %s) in segment %i\n", i, szObjectTypeNames[gameData.objs.objects[i].type], gameData.bots.names[gameData.objs.objects[i].id], gameData.objs.objects[i].segnum);
						red_count2 += gameData.objs.objects[i].contains_count;
						break;
					default:
						break;
				}
			}
		}
	}

	if (blue_count)
		if (blue_count2 == 0)
			err_printf(my_file, "Error: There is a door keyed to the blue key, but no blue key!\n");

	if (red_count)
		if (red_count2 == 0)
			err_printf(my_file, "Error: There is a door keyed to the red key, but no red key!\n");

	if (gold_count)
		if (gold_count2 == 0)
			err_printf(my_file, "Error: There is a door keyed to the gold key, but no gold key!\n");

	if (blue_count2 > 1)
		err_printf(my_file, "Error: There are %i blue keys!\n", blue_count2);

	if (red_count2 > 1)
		err_printf(my_file, "Error: There are %i red keys!\n", red_count2);

	if (gold_count2 > 1)
		err_printf(my_file, "Error: There are %i gold keys!\n", gold_count2);
}

// ----------------------------------------------------------------------------
void write_control_center_text(FILE *my_file)
{
	int	i, count, objnum, count2;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "Control Center stuff:\n");

	count = 0;
	for (i=0; i<=gameData.segs.nLastSegment; i++)
		if (gameData.segs.segment2s[i].special == SEGMENT_IS_CONTROLCEN) {
			count++;
			fprintf(my_file, "Segment %3i is a control center.\n", i);
			objnum = gameData.segs.segments[i].objects;
			count2 = 0;
			while (objnum != -1) {
				if (gameData.objs.objects[objnum].type == OBJ_CNTRLCEN)
					count2++;
				objnum = gameData.objs.objects[objnum].next;
			}
			if (count2 == 0)
				fprintf(my_file, "No control center object in control center segment.\n");
			else if (count2 != 1)
				fprintf(my_file, "%i control center gameData.objs.objects in control center segment.\n", count2);
		}

	if (count == 0)
		err_printf(my_file, "Error: No control center in this mine.\n");
	else if (count != 1)
		err_printf(my_file, "Error: More than one control center in this mine.\n");
}

// ----------------------------------------------------------------------------
void write_fuelcen_text(FILE *my_file)
{
	int	i;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "Fuel Center stuff: (Note: This means fuel, repair, materialize, control centers!)\n");

	for (i=0; i<gameData.matCens.nFuelCenters; i++) {
		fprintf(my_file, "Fuelcenter %i: Type=%i (%s), segment = %3i\n", i, gameData.matCens.fuelCenters[i].Type, Special_names[gameData.matCens.fuelCenters[i].Type], gameData.matCens.fuelCenters[i].segnum);
		if (gameData.segs.segment2s[gameData.matCens.fuelCenters[i].segnum].special != gameData.matCens.fuelCenters[i].Type)
			err_printf(my_file, "Error: Conflicting data: Segment %i has special type %i (%s), expected to be %i\n", gameData.matCens.fuelCenters[i].segnum, gameData.segs.segment2s[gameData.matCens.fuelCenters[i].segnum].special, Special_names[gameData.segs.segment2s[gameData.matCens.fuelCenters[i].segnum].special], gameData.matCens.fuelCenters[i].Type);
	}
}

// ----------------------------------------------------------------------------
void write_segment_text(FILE *my_file)
{
	int	i, objnum;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "Segment stuff:\n");

	for (i=0; i<=gameData.segs.nLastSegment; i++) {

		fprintf(my_file, "Segment %4i: ", i);
		if (gameData.segs.segment2s[i].special != 0)
			fprintf(my_file, "special = %3i (%s), value = %3i ", gameData.segs.segment2s[i].special, Special_names[gameData.segs.segment2s[i].special], gameData.segs.segment2s[i].value);

		if (gameData.segs.segment2s[i].matcen_num != -1)
			fprintf(my_file, "matcen = %3i, ", gameData.segs.segment2s[i].matcen_num);
		
		fprintf(my_file, "\n");
	}

	for (i=0; i<=gameData.segs.nLastSegment; i++) {
		int	depth;

		objnum = gameData.segs.segments[i].objects;
		fprintf(my_file, "Segment %4i: ", i);
		depth=0;
		if (objnum != -1) {
			fprintf(my_file, "gameData.objs.objects: ");
			while (objnum != -1) {
				fprintf(my_file, "[%8s %8s %3i] ", object_types(objnum), object_ids(objnum), objnum);
				objnum = gameData.objs.objects[objnum].next;
				if (depth++ > 30) {
					fprintf(my_file, "\nAborted after %i links\n", depth);
					break;
				}
			}
		}
		fprintf(my_file, "\n");
	}
}

// ----------------------------------------------------------------------------
// This routine is bogus.  It assumes that all centers are matcens,
// which is not true.  The setting of segnum is bogus.
void write_matcen_text(FILE *my_file)
{
	int	i, j, k;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "Materialization centers:\n");
	for (i=0; i<gameData.matCens.nRobotCenters; i++) {
		int	trigger_count=0, segnum, fuelcen_num;

		fprintf(my_file, "fuelcen_info[%02i].Segment = %04i  ", i, gameData.matCens.fuelCenters[i].segnum);
		fprintf(my_file, "Segment2[%04i].matcen_num = %02i  ", gameData.matCens.fuelCenters[i].segnum, gameData.segs.segment2s[gameData.matCens.fuelCenters[i].segnum].matcen_num);

		fuelcen_num = gameData.matCens.robotCenters[i].fuelcen_num;
		if (gameData.matCens.fuelCenters[fuelcen_num].Type != SEGMENT_IS_ROBOTMAKER)
			err_printf(my_file, "Error: Matcen %i corresponds to gameData.matCens.fuelCenters %i, which has type %i (%s).\n", i, fuelcen_num, gameData.matCens.fuelCenters[fuelcen_num].Type, Special_names[gameData.matCens.fuelCenters[fuelcen_num].Type]);

		segnum = gameData.matCens.fuelCenters[fuelcen_num].segnum;

		//	Find trigger for this materialization center.
		for (j=0; j<gameData.trigs.nTriggers; j++) {
			if (gameData.trigs.triggers[j].type == TT_MATCEN) {
				for (k=0; k<gameData.trigs.triggers[j].num_links; k++)
					if (gameData.trigs.triggers[j].seg[k] == segnum) {
						fprintf(my_file, "Trigger = %2i  ", j );
						trigger_count++;
					}
			}
		}
		fprintf(my_file, "\n");

		if (trigger_count == 0)
			err_printf(my_file, "Error: Matcen %i in segment %i has no trigger!\n", i, segnum);

	}
}

// ----------------------------------------------------------------------------
void write_wall_text(FILE *my_file)
{
	int	i, j;
	byte	wall_flags[MAX_WALLS];
	segment *segp;
	side *sidep;
	wall	*wallP;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "gameData.walls.walls:\n");
	for (i=0, wallP = gameData.walls.walls; i<gameData.walls.nWalls; i++, wallP++) {
		int	segnum, sidenum;

		fprintf(my_file, 
			"Wall %03i: seg=%3i, side=%2i, linked_wall=%3i, type=%s, flags=%4x, hps=%3i, trigger=%2i, clip_num=%2i, keys=%2i, state=%i\n", i,
			wallP->segnum, 
			wallP->sidenum, 
			wallP->linked_wall, 
			pszWallNames[wallP->type], 
			wallP->flags, wallP->hps >> 16, 
			wallP->trigger, 
			wallP->clip_num, 
			wallP->keys, 
			wallP->state);

		if (wallP->trigger >= gameData.trigs.nTriggers)
			fprintf(my_file, "Wall %03d points to invalid trigger %d\n",i,wallP->trigger);

		segnum = wallP->segnum;
		sidenum = wallP->sidenum;

		if (WallNumI (segnum, sidenum) != i)
			err_printf(my_file, "Error: Wall %i points at segment %i, side %i, but that segment doesn't point back (it's wall_num = %i)\n", 
							i, segnum, sidenum, WallNumI (segnum, sidenum));
	}

	for (i=0; i<MAX_WALLS; i++)
		wall_flags[i] = 0;

	for (i=0, segp = gameData.segs.segments; i<=gameData.segs.nLastSegment; i++, segp++) {
		for (j=0, sidep = segp->sides; j<MAX_SIDES_PER_SEGMENT; j++, sidep++) {
			short wall_num = WallNumP (sideP);
			if (IS_WALL (wall_num))
				if (wall_flags[wall_num])
					err_printf(my_file, "Error: Wall %i appears in two or more segments, including segment %i, side %i.\n", sidep->wall_num, i, j);
				else
					wall_flags[wall_num] = 1;
		}
	}

}

//typedef struct trigger {
//	byte		type;
//	short		flags;
//	fix		value;
//	fix		time;
//	byte		link_num;
//	short 	num_links;
//	short 	seg[MAX_WALLS_PER_LINK];
//	short		side[MAX_WALLS_PER_LINK];
//	} trigger;

// ----------------------------------------------------------------------------
void write_player_text(FILE *my_file)
{
	int	i, num_players=0;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "gameData.multi.players:\n");
	for (i=0; i<=gameData.objs.nLastObject; i++) {
		if (gameData.objs.objects[i].type == OBJ_PLAYER) {
			num_players++;
			fprintf(my_file, "Player %2i is object #%3i in segment #%3i.\n", gameData.objs.objects[i].id, i, gameData.objs.objects[i].segnum);
		}
	}

	if (num_players != MAX_PLAYERS)
		err_printf(my_file, "Error: %i player gameData.objs.objects.  %i are required.\n", num_players, MAX_PLAYERS);
	if (num_players > MAX_MULTI_PLAYERS)
		err_printf(my_file, "Error: %i player gameData.objs.objects.  %i are required.\n", num_players, MAX_PLAYERS);
}

// ----------------------------------------------------------------------------
void write_trigger_text(FILE *my_file)
{
	int	i, j, w;
	wall	*wallP;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "gameData.trigs.triggers:\n");
	for (i=0; i<gameData.trigs.nTriggers; i++) {
		fprintf(my_file, "Trigger %03i: type=%02x flags=%04x, value=%08x, time=%8x, num_links=%i ", i,
			gameData.trigs.triggers[i].type, gameData.trigs.triggers[i].flags, gameData.trigs.triggers[i].value, gameData.trigs.triggers[i].time, gameData.trigs.triggers[i].num_links);

		for (j=0; j<gameData.trigs.triggers[i].num_links; j++)
			fprintf(my_file, "[%03i:%i] ", gameData.trigs.triggers[i].seg[j], gameData.trigs.triggers[i].side[j]);

		//	Find which wall this trigger is connected to.
		for (w=gameData.walls.nWalls, wallP = gameData.walls.walls; w; w--, wallP++)
			if (wallP->trigger == i)
				break;

		if (w == gameData.walls.nWalls)
			err_printf(my_file, "\nError: Trigger %i is not connected to any wall, so it can never be triggered.\n", i);
		else
			fprintf(my_file, "Attached to seg:side = %i:%i, wall %i\n", 
				wallP->segnum, wallP->sidenum, WallNumI (wallP->segnum, wallP->sidenum));

	}

}

// ----------------------------------------------------------------------------
void write_game_text_file(char *filename)
{
	char	my_filename[128];
	int	namelen;
	FILE	* my_file;

	Errors_in_mine = 0;

	namelen = (int) strlen(filename);

	Assert (namelen > 4);

	Assert (filename[namelen-4] == '.');

	strcpy(my_filename, filename);
	strcpy( &my_filename[namelen-4], ".txm");

	my_file = fopen( my_filename, "wt" );
	if (!my_file)	{
		char  ErrorMessage[200];

		sprintf( ErrorMessage, "ERROR: Unable to open %s\nErrno = %i", my_file, errno );
		StopTime();
		GrPaletteStepLoad (NULL);
		ExecMessageBox( NULL, 1, "Ok", ErrorMessage );
		StartTime();

		return;
	}

	dump_used_textures_level(my_file, 0);
	say_totals(my_file, gameData.segs.szLevelFilename);

	fprintf(my_file, "\nNumber of segments:   %4i\n", gameData.segs.nLastSegment+1);
	fprintf(my_file, "Number of gameData.objs.objects:    %4i\n", gameData.objs.nLastObject+1);
	fprintf(my_file, "Number of walls:      %4i\n", gameData.walls.nWalls);
	fprintf(my_file, "Number of open doors: %4i\n", gameData.walls.nOpenDoors);
	fprintf(my_file, "Number of triggers:   %4i\n", gameData.trigs.nTriggers);
	fprintf(my_file, "Number of matcens:    %4i\n", gameData.matCens.nRobotCenters);
	fprintf(my_file, "\n");

	write_segment_text(my_file);

	write_fuelcen_text(my_file);

	write_matcen_text(my_file);

	write_player_text(my_file);

	write_wall_text(my_file);

	write_trigger_text(my_file);

	write_exit_text(my_file);

	//	---------- Find control center segment ----------
	write_control_center_text(my_file);

	//	---------- Show keyed walls ----------
	write_key_text(my_file);

{ int r;
	r = fclose(my_file);
#if TRACE
	con_printf (1, "Close value = %i\n", r);
#endif
	if (r)
		Int3();
}
}

// -- //	---------------
// -- //	Note: This only works for a loaded level because the gameData.objs.objects array must be valid.
// -- void determine_used_textures_robots(int *tmap_buf)
// -- {
// -- 	int	i, objnum;
// -- 	polymodel	*po;
// --
// -- 	Assert(gameData.models.nPolyModels);
// --
// -- 	for (objnum=0; objnum <= gameData.objs.nLastObject; objnum++) {
// -- 		int	model_num;
// --
// -- 		if (gameData.objs.objects[objnum].render_type == RT_POLYOBJ) {
// -- 			model_num = gameData.objs.objects[objnum].rtype.pobj_info.model_num;
// --
// -- 			po=&gameData.models.polyModels[model_num];
// --
// -- 			for (i=0;i<po->n_textures;i++)	{
// -- 				int	tli;
// --
// -- 				tli = gameData.pig.tex.objBmIndex[gameData.pig.tex.pObjBmIndex[po->first_texture+i]];
// -- 				Assert((tli>=0) && (tli<= Num_tmaps);
// -- 				tmap_buf[tli]++;
// -- 			}
// -- 		}
// -- 	}
// --
// -- }

// --05/17/95--//	-----------------------------------------------------------------------------
// --05/17/95--void determine_used_textures_level(int load_level_flag, int shareware_flag, int level_num, int *tmap_buf, int *wall_buf, byte *level_tmap_buf, int max_tmap)
// --05/17/95--{
// --05/17/95--	int	segnum, sidenum;
// --05/17/95--	int	i, j;
// --05/17/95--
// --05/17/95--	for (i=0; i<max_tmap; i++)
// --05/17/95--		tmap_buf[i] = 0;
// --05/17/95--
// --05/17/95--	if (load_level_flag) {
// --05/17/95--		if (shareware_flag)
// --05/17/95--			LoadLevelSub(Shareware_level_names[level_num]);
// --05/17/95--		else
// --05/17/95--			LoadLevelSub(Registered_level_names[level_num]);
// --05/17/95--	}
// --05/17/95--
// --05/17/95--	for (segnum=0; segnum<=gameData.segs.nLastSegment; segnum++) {
// --05/17/95--		segment	*segp = &gameData.segs.segments[segnum];
// --05/17/95--
// --05/17/95--		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
// --05/17/95--			side	*sidep = &segp->sides[sidenum];
// --05/17/95--
// --05/17/95--			if (sidep->wall_num != -1) {
// --05/17/95--				int clip_num = gameData.walls.walls[sidep->wall_num].clip_num;
// --05/17/95--				if (clip_num != -1) {
// --05/17/95--
// --05/17/95--					int num_frames = gameData.walls.anims[clip_num].nFrameCount;
// --05/17/95--
// --05/17/95--					wall_buf[clip_num] = 1;
// --05/17/95--
// --05/17/95--					for (j=0; j<num_frames; j++) {
// --05/17/95--						int	tmap_num;
// --05/17/95--
// --05/17/95--						tmap_num = gameData.walls.anims[clip_num].frames[j];
// --05/17/95--						tmap_buf[tmap_num]++;
// --05/17/95--						if (level_tmap_buf[tmap_num] == -1)
// --05/17/95--							level_tmap_buf[tmap_num] = level_num + (!shareware_flag) * NUM_SHAREWARE_LEVELS;
// --05/17/95--					}
// --05/17/95--				}
// --05/17/95--			}
// --05/17/95--
// --05/17/95--			if (sidep->tmap_num >= 0)
// --05/17/95--				if (sidep->tmap_num < max_tmap) {
// --05/17/95--					tmap_buf[sidep->tmap_num]++;
// --05/17/95--					if (level_tmap_buf[sidep->tmap_num] == -1)
// --05/17/95--						level_tmap_buf[sidep->tmap_num] = level_num + (!shareware_flag) * NUM_SHAREWARE_LEVELS;
// --05/17/95--				} else
// --05/17/95--					Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.
// --05/17/95--
// --05/17/95--			if ((sidep->tmap_num2 & 0x3fff) != 0)
// --05/17/95--				if ((sidep->tmap_num2 & 0x3fff) < max_tmap) {
// --05/17/95--					tmap_buf[sidep->tmap_num2 & 0x3fff]++;
// --05/17/95--					if (level_tmap_buf[sidep->tmap_num2 & 0x3fff] == -1)
// --05/17/95--						level_tmap_buf[sidep->tmap_num2 & 0x3fff] = level_num + (!shareware_flag) * NUM_SHAREWARE_LEVELS;
// --05/17/95--				} else
// --05/17/95--					Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.
// --05/17/95--		}
// --05/17/95--	}
// --05/17/95--}

//	Adam: Change NUM_ADAM_LEVELS to the number of levels.
#define	NUM_ADAM_LEVELS	30

//	Adam: Stick the names here.
char *Adam_level_names[NUM_ADAM_LEVELS] = {
	"D2LEVA-1.LVL",
	"D2LEVA-2.LVL",
	"D2LEVA-3.LVL",
	"D2LEVA-4.LVL",
	"D2LEVA-S.LVL",

	"D2LEVB-1.LVL",
	"D2LEVB-2.LVL",
	"D2LEVB-3.LVL",
	"D2LEVB-4.LVL",
	"D2LEVB-S.LVL",

	"D2LEVC-1.LVL",
	"D2LEVC-2.LVL",
	"D2LEVC-3.LVL",
	"D2LEVC-4.LVL",
	"D2LEVC-S.LVL",

	"D2LEVD-1.LVL",
	"D2LEVD-2.LVL",
	"D2LEVD-3.LVL",
	"D2LEVD-4.LVL",
	"D2LEVD-S.LVL",

	"D2LEVE-1.LVL",
	"D2LEVE-2.LVL",
	"D2LEVE-3.LVL",
	"D2LEVE-4.LVL",
	"D2LEVE-S.LVL",

	"D2LEVF-1.LVL",
	"D2LEVF-2.LVL",
	"D2LEVF-3.LVL",
	"D2LEVF-4.LVL",
	"D2LEVF-S.LVL",
};

typedef struct BitmapFile	{
	char			name[15];
} BitmapFile;

extern BitmapFile gameData.pig.tex.bitmapFiles[ MAX_BITMAP_FILES ];

int	Ignore_tmap_num2_error;

// ----------------------------------------------------------------------------
void determine_used_textures_level(int load_level_flag, int shareware_flag, int level_num, int *tmap_buf, int *wall_buf, byte *level_tmap_buf, int max_tmap)
{
	int	segnum, sidenum, objnum=max_tmap;
	int	i, j;

	Assert(shareware_flag != -17);

	for (i=0; i<MAX_BITMAP_FILES; i++)
		tmap_buf[i] = 0;

	if (load_level_flag) {
		LoadLevelSub(Adam_level_names[level_num]);
	}


	//	Process robots.
	for (objnum=0; objnum<=gameData.objs.nLastObject; objnum++) {
		object *objP = &gameData.objs.objects[objnum];

		if (objP->render_type == RT_POLYOBJ) {
			polymodel *po = &gameData.models.polyModels[objP->rtype.pobj_info.model_num];

			for (i=0; i<po->n_textures; i++) {

				int	tli = gameData.pig.tex.objBmIndex[gameData.pig.tex.pObjBmIndex[po->first_texture+i]].index;
				if ((tli < MAX_BITMAP_FILES) && (tli >= 0)) {
					tmap_buf[tli]++;
					if (level_tmap_buf[tli] == -1)
						level_tmap_buf[tli] = level_num;
				} else
					Int3();	//	Hmm, It seems this texture is bogus!
			}

		}
	}


	Ignore_tmap_num2_error = 0;

	//	Process walls and segment sides.
	for (segnum=0; segnum<=gameData.segs.nLastSegment; segnum++) {
		segment	*segp = &gameData.segs.segments[segnum];

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			side	*sidep = &segp->sides[sidenum];

			if (sidep->wall_num != -1) {
				int clip_num = gameData.walls.walls[sidep->wall_num].clip_num;
				if (clip_num != -1) {

					// -- int num_frames = gameData.walls.anims[clip_num].nFrameCount;

					wall_buf[clip_num] = 1;

					for (j=0; j<1; j++) {	//	Used to do through num_frames, but don't really want all the door01#3 stuff.
						int	tmap_num;

						tmap_num = gameData.pig.tex.pBmIndex[gameData.walls.anims[clip_num].frames[j]].index;
						Assert((tmap_num >= 0) && (tmap_num < MAX_BITMAP_FILES);
						tmap_buf[tmap_num]++;
						if (level_tmap_buf[tmap_num] == -1)
							level_tmap_buf[tmap_num] = level_num;
					}
				}
			} else if (segp->children[sidenum] == -1) {

				if (sidep->tmap_num >= 0)
					if (sidep->tmap_num < MAX_BITMAP_FILES) {
						Assert(gameData.pig.tex.pBmIndex[sidep->tmap_num].index < MAX_BITMAP_FILES);
						tmap_buf[gameData.pig.tex.pBmIndex[sidep->tmap_num].index]++;
						if (level_tmap_buf[gameData.pig.tex.pBmIndex[sidep->tmap_num].index] == -1)
							level_tmap_buf[gameData.pig.tex.pBmIndex[sidep->tmap_num].index] = level_num;
					} else
						Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.

				if ((sidep->tmap_num2 & 0x3fff) != 0)
					if ((sidep->tmap_num2 & 0x3fff) < MAX_BITMAP_FILES) {
						Assert(gameData.pig.tex.pBmIndex[sidep->tmap_num2 & 0x3fff].index < MAX_BITMAP_FILES);
						tmap_buf[gameData.pig.tex.pBmIndex[sidep->tmap_num2 & 0x3fff].index]++;
						if (level_tmap_buf[gameData.pig.tex.pBmIndex[sidep->tmap_num2 & 0x3fff].index] == -1)
							level_tmap_buf[gameData.pig.tex.pBmIndex[sidep->tmap_num2 & 0x3fff].index] = level_num;
					} else {
						if (!Ignore_tmap_num2_error)
							Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.
						Ignore_tmap_num2_error = 1;
						sidep->tmap_num2 = 0;
					}
			}
		}
	}
}

// ----------------------------------------------------------------------------
void merge_buffers(int *dest, int *src, int num)
{
	int	i;

	for (i=0; i<num; i++)
		if (src[i])
			dest[i] += src[i];
}

// ----------------------------------------------------------------------------
void say_used_tmaps(FILE *my_file, int *tb)
{
	int	i;
// -- mk, 08/14/95 -- 	int	count = 0;

	for (i=0; i<MAX_BITMAP_FILES; i++)
		if (tb[i]) {
			fprintf(my_file, "[%3i %8s (%4i)]\n", i, gameData.pig.tex.bitmapFiles[i].name, tb[i]);
// -- mk, 08/14/95 -- 			if (count++ >= 4) {
// -- mk, 08/14/95 -- 				fprintf(my_file, "\n");
// -- mk, 08/14/95 -- 				count = 0;
// -- mk, 08/14/95 -- 			}
		}
}

// --05/17/95--//	-----------------------------------------------------------------------------
// --05/17/95--void say_used_once_tmaps(FILE *my_file, int *tb, byte *tb_lnum)
// --05/17/95--{
// --05/17/95--	int	i;
// --05/17/95--	char	*level_name;
// --05/17/95--
// --05/17/95--	for (i=0; i<Num_tmaps; i++)
// --05/17/95--		if (tb[i] == 1) {
// --05/17/95--			int	level_num = tb_lnum[i];
// --05/17/95--			if (level_num >= NUM_SHAREWARE_LEVELS) {
// --05/17/95--				Assert((level_num - NUM_SHAREWARE_LEVELS >= 0) && (level_num - NUM_SHAREWARE_LEVELS < NUM_REGISTERED_LEVELS);
// --05/17/95--				level_name = Registered_level_names[level_num - NUM_SHAREWARE_LEVELS];
// --05/17/95--			} else {
// --05/17/95--				Assert((level_num >= 0) && (level_num < NUM_SHAREWARE_LEVELS);
// --05/17/95--				level_name = Shareware_level_names[level_num];
// --05/17/95--			}
// --05/17/95--
// --05/17/95--			fprintf(my_file, "Texture %3i %8s used only once on level %s\n", i, gameData.pig.tex.pTMapInfo[i].filename, level_name);
// --05/17/95--		}
// --05/17/95--}

// ----------------------------------------------------------------------------
void say_used_once_tmaps(FILE *my_file, int *tb, byte *tb_lnum)
{
	int	i;
	char	*level_name;

	for (i=0; i<MAX_BITMAP_FILES; i++)
		if (tb[i] == 1) {
			int	level_num = tb_lnum[i];
			level_name = Adam_level_names[level_num];

			fprintf(my_file, "Texture %3i %8s used only once on level %s\n", i, gameData.pig.tex.bitmapFiles[i].name, level_name);
		}
}

// ----------------------------------------------------------------------------
void say_unused_tmaps(FILE *my_file, int *tb)
{
	int	i;
	int	count = 0;

	for (i=0; i<MAX_BITMAP_FILES; i++)
		if (!tb[i]) {
			if (gameData.pig.tex.bitmaps[gameData.pig.tex.pBmIndex[i].index].bm_texBuf == &bogus_data)
				fprintf(my_file, "U");
			else
				fprintf(my_file, " ");

			fprintf(my_file, "[%3i %8s] ", i, gameData.pig.tex.bitmapFiles[i].name);
			if (count++ >= 4) {
				fprintf(my_file, "\n");
				count = 0;
			}
		}
}

// ----------------------------------------------------------------------------
void say_unused_walls(FILE *my_file, int *tb)
{
	int	i;

	for (i=0; i<gameData.walls.nAnims; i++)
		if (!tb[i])
			fprintf(my_file, "Wall %3i is unused.\n", i);
}

// ----------------------------------------------------------------------------
say_totals(FILE *my_file, char *level_name)
{
	int	i;		//, objnum;
	int	total_robots = 0;
	int	objects_processed = 0;

	int	used_objects[MAX_OBJECTS];

	fprintf(my_file, "\nLevel %s\n", level_name);

	for (i=0; i<MAX_OBJECTS; i++)
		used_objects[i] = 0;

	while (objects_processed < gameData.objs.nLastObject+1) {
		int	j, objtype, objid, objcount, cur_obj_val, min_obj_val, min_objnum;

		//	Find new min objnum.
		min_obj_val = 0x7fff0000;
		min_objnum = -1;

		for (j=0; j<=gameData.objs.nLastObject; j++) {
			if (!used_objects[j] && gameData.objs.objects[j].type!=OBJ_NONE) {
				cur_obj_val = gameData.objs.objects[j].type * 1000 + gameData.objs.objects[j].id;
				if (cur_obj_val < min_obj_val) {
					min_objnum = j;
					min_obj_val = cur_obj_val;
				}
			}
		}
		if ((min_objnum == -1) || (gameData.objs.objects[min_objnum].type == 255))
			break;

		objcount = 0;

		objtype = gameData.objs.objects[min_objnum].type;
		objid = gameData.objs.objects[min_objnum].id;

		for (i=0; i<=gameData.objs.nLastObject; i++) {
			if (!used_objects[i]) {

				if (((gameData.objs.objects[i].type == objtype) && (gameData.objs.objects[i].id == objid)) ||
						((gameData.objs.objects[i].type == objtype) && (objtype == OBJ_PLAYER)) ||
						((gameData.objs.objects[i].type == objtype) && (objtype == OBJ_COOP)) ||
						((gameData.objs.objects[i].type == objtype) && (objtype == OBJ_HOSTAGE))) {
					if (gameData.objs.objects[i].type == OBJ_ROBOT)
						total_robots++;
					used_objects[i] = 1;
					objcount++;
					objects_processed++;
				}
			}
		}

		if (objcount) {
			fprintf(my_file, "Object: ");
			fprintf(my_file, "%8s %8s %3i\n", object_types(min_objnum), object_ids(min_objnum), objcount);
		}
	}

	fprintf(my_file, "Total robots = %3i\n", total_robots);
}

int	First_dump_level = 0;
int	Last_dump_level = NUM_ADAM_LEVELS-1;

// ----------------------------------------------------------------------------
void say_totals_all(void)
{
	int	i;
	FILE	*my_file;

	my_file = fopen( "levels.all", "wt" );
	if (!my_file)	{
		char  ErrorMessage[200];

		sprintf( ErrorMessage, "ERROR: Unable to open levels.all\nErrno=%i", errno );
		StopTime();
		GrPaletteStepLoad (NULL);
		ExecMessageBox( NULL, 1, "Ok", ErrorMessage );
		StartTime();

		return;
	}

	for (i=First_dump_level; i<=Last_dump_level; i++) {
#if TRACE
		con_printf (CON_DEBUG, "Level %i\n", i+1);
#endif
		LoadLevelSub(Adam_level_names[i]);
		say_totals(my_file, Adam_level_names[i]);
	}

//--05/17/95--	for (i=0; i<NUM_SHAREWARE_LEVELS; i++) {
//--05/17/95--		LoadLevelSub(Shareware_level_names[i]);
//--05/17/95--		say_totals(my_file, Shareware_level_names[i]);
//--05/17/95--	}
//--05/17/95--
//--05/17/95--	for (i=0; i<NUM_REGISTERED_LEVELS; i++) {
//--05/17/95--		LoadLevelSub(Registered_level_names[i]);
//--05/17/95--		say_totals(my_file, Registered_level_names[i]);
//--05/17/95--	}

	fclose(my_file);

}

void dump_used_textures_level(FILE *my_file, int level_num)
{
	int	i;
	int	temp_tmap_buf[MAX_BITMAP_FILES];
	int	perm_tmap_buf[MAX_BITMAP_FILES];
	byte	level_tmap_buf[MAX_BITMAP_FILES];
	int	temp_wall_buf[MAX_BITMAP_FILES];
	int	perm_wall_buf[MAX_BITMAP_FILES];

	for (i=0; i<MAX_BITMAP_FILES; i++) {
		perm_tmap_buf[i] = 0;
		level_tmap_buf[i] = -1;
	}

	for (i=0; i<MAX_BITMAP_FILES; i++) {
		perm_wall_buf[i] = 0;
	}

	determine_used_textures_level(0, 1, level_num, temp_tmap_buf, temp_wall_buf, level_tmap_buf, MAX_BITMAP_FILES);
// -- 	determine_used_textures_robots(temp_tmap_buf);
	fprintf(my_file, "\nTextures used in [%s]\n", gameData.segs.szLevelFilename);
	say_used_tmaps(my_file, temp_tmap_buf);
}

FILE	*my_file;

// ----------------------------------------------------------------------------
void dump_used_textures_all(void)
{
	int	i;
	int	temp_tmap_buf[MAX_BITMAP_FILES];
	int	perm_tmap_buf[MAX_BITMAP_FILES];
	byte	level_tmap_buf[MAX_BITMAP_FILES];
	int	temp_wall_buf[MAX_BITMAP_FILES];
	int	perm_wall_buf[MAX_BITMAP_FILES];

say_totals_all();

	my_file = fopen( "textures.dmp", "wt" );
	if (!my_file)	{
		char  ErrorMessage[200];

		sprintf( ErrorMessage, "ERROR: Can't open textures.dmp\nErrno=%i", errno);
		StopTime();
		GrPaletteStepLoad (NULL);
		ExecMessageBox( NULL, 1, "Ok", ErrorMessage );
		StartTime();

		return;
	}

	for (i=0; i<MAX_BITMAP_FILES; i++) {
		perm_tmap_buf[i] = 0;
		level_tmap_buf[i] = -1;
	}

	for (i=0; i<MAX_BITMAP_FILES; i++) {
		perm_wall_buf[i] = 0;
	}

// --05/17/95--	for (i=0; i<NUM_SHAREWARE_LEVELS; i++) {
// --05/17/95--		determine_used_textures_level(1, 1, i, temp_tmap_buf, temp_wall_buf, level_tmap_buf, MAX_BITMAP_FILES);
// --05/17/95--		fprintf(my_file, "\nTextures used in [%s]\n", Shareware_level_names[i]);
// --05/17/95--		say_used_tmaps(my_file, temp_tmap_buf);
// --05/17/95--		merge_buffers(perm_tmap_buf, temp_tmap_buf, MAX_BITMAP_FILES);
// --05/17/95--		merge_buffers(perm_wall_buf, temp_wall_buf, MAX_BITMAP_FILES);
// --05/17/95--	}
// --05/17/95--
// --05/17/95--	fprintf(my_file, "\n\nUsed textures in all shareware mines:\n");
// --05/17/95--	say_used_tmaps(my_file, perm_tmap_buf);
// --05/17/95--
// --05/17/95--	fprintf(my_file, "\nUnused textures in all shareware mines:\n");
// --05/17/95--	say_unused_tmaps(my_file, perm_tmap_buf);
// --05/17/95--
// --05/17/95--	fprintf(my_file, "\nTextures used exactly once in all shareware mines:\n");
// --05/17/95--	say_used_once_tmaps(my_file, perm_tmap_buf, level_tmap_buf);
// --05/17/95--
// --05/17/95--	fprintf(my_file, "\nWall anims (eg, doors) unused in all shareware mines:\n");
// --05/17/95--	say_unused_walls(my_file, perm_wall_buf);

// --05/17/95--	for (i=0; i<NUM_REGISTERED_LEVELS; i++) {
// --05/17/95--		determine_used_textures_level(1, 0, i, temp_tmap_buf, temp_wall_buf, level_tmap_buf, MAX_BITMAP_FILES);
// --05/17/95--		fprintf(my_file, "\nTextures used in [%s]\n", Registered_level_names[i]);
// --05/17/95--		say_used_tmaps(my_file, temp_tmap_buf);
// --05/17/95--		merge_buffers(perm_tmap_buf, temp_tmap_buf, MAX_BITMAP_FILES);
// --05/17/95--	}
// --05/17/95--
// --05/17/95--	fprintf(my_file, "\n\nUsed textures in all (including registered) mines:\n");
// --05/17/95--	say_used_tmaps(my_file, perm_tmap_buf);
// --05/17/95--
// --05/17/95--	fprintf(my_file, "\nUnused textures in all (including registered) mines:\n");
// --05/17/95--	say_unused_tmaps(my_file, perm_tmap_buf);

	for (i=First_dump_level; i<=Last_dump_level; i++) {
		determine_used_textures_level(1, 0, i, temp_tmap_buf, temp_wall_buf, level_tmap_buf, MAX_BITMAP_FILES);
		fprintf(my_file, "\nTextures used in [%s]\n", Adam_level_names[i]);
		say_used_tmaps(my_file, temp_tmap_buf);
		merge_buffers(perm_tmap_buf, temp_tmap_buf, MAX_BITMAP_FILES);
	}

	fprintf(my_file, "\n\nUsed textures in all (including registered) mines:\n");
	say_used_tmaps(my_file, perm_tmap_buf);

	fprintf(my_file, "\nUnused textures in all (including registered) mines:\n");
	say_unused_tmaps(my_file, perm_tmap_buf);

	fclose(my_file);
}

#endif
