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
 * Multiplayer tObject capability.
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
 * moved tSegment validation functions from editor to main.
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
#include "loadgame.h"
#include "polyobj.h"
#include "gamesave.h"


#ifdef EDITOR

extern ubyte bogus_data[1024*1024];
extern grsBitmap bogus_bitmap;

// ----------------------------------------------------------------------------
char	*objectTypes(int nObject)
{
	int	nType = gameData.objs.objects[nObject].nType;

	Assert((nType >= 0) && (nType < MAX_OBJECT_TYPES);
	return	&szObjectTypeNames[nType];
}

// ----------------------------------------------------------------------------
char	*object_ids(int nObject)
{
	int	nType = gameData.objs.objects[nObject].nType;
	int	id = gameData.objs.objects[nObject].id;

	switch (nType) {
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
	con_printf (CONDBG, "%s", message);
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
		if (gameData.trigs.triggers[i].nType == TT_EXIT) {
			int	count2;

			fprintf(my_file, "Trigger %2i, is an exit tTrigger with %i links.\n", i, gameData.trigs.triggers[i].nLinks);
			count++;
			if (gameData.trigs.triggers[i].nLinks != 0)
				err_printf(my_file, "Error: Exit triggers must have 0 links, this one has %i links.\n", gameData.trigs.triggers[i].nLinks);

			//	Find tWall pointing to this tTrigger.
			count2 = 0;
			for (j=0; j<gameData.walls.nWalls; j++)
				if (gameData.walls.walls[j].nTrigger == i) {
					count2++;
					fprintf(my_file, "Exit tTrigger %i is in tSegment %i, on tSide %i, bound to tWall %i\n", i, gameData.walls.walls[j].nSegment, gameData.walls.walls[j].nSide, j);
				}
			if (count2 == 0)
				err_printf(my_file, "Error: Trigger %i is not bound to any tWall.\n", i);
			else if (count2 > 1)
				err_printf(my_file, "Error: Trigger %i is bound to %i walls.\n", i, count2);

		}

	if (count == 0)
		err_printf(my_file, "Error: No exit tTrigger in this mine.\n");
	else if (count != 1)
		err_printf(my_file, "Error: More than one exit tTrigger in this mine.\n");
	else
		fprintf(my_file, "\n");

	//	---------- Find exit doors ----------
	count = 0;
	for (i=0; i<=gameData.segs.nLastSegment; i++)
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (gameData.segs.segments[i].children[j] == -2) {
				fprintf(my_file, "Segment %3i, tSide %i is an exit door.\n", i, j);
				count++;
			}

	if (count == 0)
		err_printf(my_file, "Error: No external tWall in this mine.\n");
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
	int	redCount, blueCount, goldCount;
	int	redCount2, blueCount2, goldCount2;
	int	blue_segnum=-1, blue_sidenum=-1, red_segnum=-1, red_sidenum=-1, gold_segnum=-1, gold_sidenum=-1;
	int	connect_side;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "Key stuff:\n");

	redCount = 0;
	blueCount = 0;
	goldCount = 0;

	for (i=0; i<gameData.walls.nWalls; i++) {
		if (gameData.walls.walls[i].keys & KEY_BLUE) {
			fprintf(my_file, "Wall %i (seg=%i, tSide=%i) is keyed to the blue key.\n", i, gameData.walls.walls[i].nSegment, gameData.walls.walls[i].nSide);
			if (blue_segnum == -1) {
				blue_segnum = gameData.walls.walls[i].nSegment;
				blue_sidenum = gameData.walls.walls[i].nSide;
				blueCount++;
			} else {
				connect_side = FindConnectedSide(&gameData.segs.segments[gameData.walls.walls[i].nSegment], &gameData.segs.segments[blue_segnum]);
				if (connect_side != blue_sidenum) {
					warning_printf(my_file, "Warning: This blue door at seg %i, is different than the one at seg %i, tSide %i\n", gameData.walls.walls[i].nSegment, blue_segnum, blue_sidenum);
					blueCount++;
				}
			}
		}
		if (gameData.walls.walls[i].keys & KEY_RED) {
			fprintf(my_file, "Wall %i (seg=%i, tSide=%i) is keyed to the red key.\n", i, gameData.walls.walls[i].nSegment, gameData.walls.walls[i].nSide);
			if (red_segnum == -1) {
				red_segnum = gameData.walls.walls[i].nSegment;
				red_sidenum = gameData.walls.walls[i].nSide;
				redCount++;
			} else {
				connect_side = FindConnectedSide(&gameData.segs.segments[gameData.walls.walls[i].nSegment], &gameData.segs.segments[red_segnum]);
				if (connect_side != red_sidenum) {
					warning_printf(my_file, "Warning: This red door at seg %i, is different than the one at seg %i, tSide %i\n", gameData.walls.walls[i].nSegment, red_segnum, red_sidenum);
					redCount++;
				}
			}
		}
		if (gameData.walls.walls[i].keys & KEY_GOLD) {
			fprintf(my_file, "Wall %i (seg=%i, tSide=%i) is keyed to the gold key.\n", i, gameData.walls.walls[i].nSegment, gameData.walls.walls[i].nSide);
			if (gold_segnum == -1) {
				gold_segnum = gameData.walls.walls[i].nSegment;
				gold_sidenum = gameData.walls.walls[i].nSide;
				goldCount++;
			} else {
				connect_side = FindConnectedSide(&gameData.segs.segments[gameData.walls.walls[i].nSegment], &gameData.segs.segments[gold_segnum]);
				if (connect_side != gold_sidenum) {
					warning_printf(my_file, "Warning: This gold door at seg %i, is different than the one at seg %i, tSide %i\n", gameData.walls.walls[i].nSegment, gold_segnum, gold_sidenum);
					goldCount++;
				}
			}
		}
	}

	if (blueCount > 1)
		warning_printf(my_file, "Warning: %i doors are keyed to the blue key.\n", blueCount);

	if (redCount > 1)
		warning_printf(my_file, "Warning: %i doors are keyed to the red key.\n", redCount);

	if (goldCount > 1)
		warning_printf(my_file, "Warning: %i doors are keyed to the gold key.\n", goldCount);

	redCount2 = 0;
	blueCount2 = 0;
	goldCount2 = 0;

	for (i=0; i<=gameData.objs.nLastObject; i++) {
		if (gameData.objs.objects[i].nType == OBJ_POWERUP)
			if (gameData.objs.objects[i].id == POW_KEY_BLUE) {
				fprintf(my_file, "The BLUE key is tObject %i in tSegment %i\n", i, gameData.objs.objects[i].nSegment);
				blueCount2++;
			}
		if (gameData.objs.objects[i].nType == OBJ_POWERUP)
			if (gameData.objs.objects[i].id == POW_KEY_RED) {
				fprintf(my_file, "The RED key is tObject %i in tSegment %i\n", i, gameData.objs.objects[i].nSegment);
				redCount2++;
			}
		if (gameData.objs.objects[i].nType == OBJ_POWERUP)
			if (gameData.objs.objects[i].id == POW_KEY_GOLD) {
				fprintf(my_file, "The GOLD key is tObject %i in tSegment %i\n", i, gameData.objs.objects[i].nSegment);
				goldCount2++;
			}

		if (gameData.objs.objects[i].containsCount) {
			if (gameData.objs.objects[i].containsType == OBJ_POWERUP) {
				switch (gameData.objs.objects[i].containsId) {
					case POW_KEY_BLUE:
						fprintf(my_file, "The BLUE key is contained in tObject %i (a %s %s) in tSegment %i\n", i, szObjectTypeNames[gameData.objs.objects[i].nType], gameData.bots.names[gameData.objs.objects[i].id], gameData.objs.objects[i].nSegment);
						blueCount2 += gameData.objs.objects[i].containsCount;
						break;
					case POW_KEY_GOLD:
						fprintf(my_file, "The GOLD key is contained in tObject %i (a %s %s) in tSegment %i\n", i, szObjectTypeNames[gameData.objs.objects[i].nType], gameData.bots.names[gameData.objs.objects[i].id], gameData.objs.objects[i].nSegment);
						goldCount2 += gameData.objs.objects[i].containsCount;
						break;
					case POW_KEY_RED:
						fprintf(my_file, "The RED key is contained in tObject %i (a %s %s) in tSegment %i\n", i, szObjectTypeNames[gameData.objs.objects[i].nType], gameData.bots.names[gameData.objs.objects[i].id], gameData.objs.objects[i].nSegment);
						redCount2 += gameData.objs.objects[i].containsCount;
						break;
					default:
						break;
				}
			}
		}
	}

	if (blueCount)
		if (blueCount2 == 0)
			err_printf(my_file, "Error: There is a door keyed to the blue key, but no blue key!\n");

	if (redCount)
		if (redCount2 == 0)
			err_printf(my_file, "Error: There is a door keyed to the red key, but no red key!\n");

	if (goldCount)
		if (goldCount2 == 0)
			err_printf(my_file, "Error: There is a door keyed to the gold key, but no gold key!\n");

	if (blueCount2 > 1)
		err_printf(my_file, "Error: There are %i blue keys!\n", blueCount2);

	if (redCount2 > 1)
		err_printf(my_file, "Error: There are %i red keys!\n", redCount2);

	if (goldCount2 > 1)
		err_printf(my_file, "Error: There are %i gold keys!\n", goldCount2);
}

// ----------------------------------------------------------------------------
void write_control_center_text(FILE *my_file)
{
	int	i, count, nObject, count2;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "Control Center stuff:\n");

	count = 0;
	for (i=0; i<=gameData.segs.nLastSegment; i++)
		if (gameData.segs.segment2s[i].special == SEGMENT_IS_CONTROLCEN) {
			count++;
			fprintf(my_file, "Segment %3i is a control center.\n", i);
			nObject = gameData.segs.segments[i].objects;
			count2 = 0;
			while (nObject != -1) {
				if (gameData.objs.objects[nObject].nType == OBJ_CNTRLCEN)
					count2++;
				nObject = gameData.objs.objects[nObject].next;
			}
			if (count2 == 0)
				fprintf(my_file, "No control center tObject in control center tSegment.\n");
			else if (count2 != 1)
				fprintf(my_file, "%i control center gameData.objs.objects in control center tSegment.\n", count2);
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
		fprintf(my_file, "Fuelcenter %i: Type=%i (%s), tSegment = %3i\n", i, gameData.matCens.fuelCenters[i].Type, Special_names[gameData.matCens.fuelCenters[i].Type], gameData.matCens.fuelCenters[i].nSegment);
		if (gameData.segs.segment2s[gameData.matCens.fuelCenters[i].nSegment].special != gameData.matCens.fuelCenters[i].Type)
			err_printf(my_file, "Error: Conflicting data: Segment %i has special nType %i (%s), expected to be %i\n", gameData.matCens.fuelCenters[i].nSegment, gameData.segs.segment2s[gameData.matCens.fuelCenters[i].nSegment].special, Special_names[gameData.segs.segment2s[gameData.matCens.fuelCenters[i].nSegment].special], gameData.matCens.fuelCenters[i].Type);
	}
}

// ----------------------------------------------------------------------------
void write_segment_text(FILE *my_file)
{
	int	i, nObject;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "Segment stuff:\n");

	for (i=0; i<=gameData.segs.nLastSegment; i++) {

		fprintf(my_file, "Segment %4i: ", i);
		if (gameData.segs.segment2s[i].special != 0)
			fprintf(my_file, "special = %3i (%s), value = %3i ", gameData.segs.segment2s[i].special, Special_names[gameData.segs.segment2s[i].special], gameData.segs.segment2s[i].value);

		if (gameData.segs.segment2s[i].nMatCen != -1)
			fprintf(my_file, "matcen = %3i, ", gameData.segs.segment2s[i].nMatCen);
	
		fprintf(my_file, "\n");
	}

	for (i=0; i<=gameData.segs.nLastSegment; i++) {
		int	depth;

		nObject = gameData.segs.segments[i].objects;
		fprintf(my_file, "Segment %4i: ", i);
		depth=0;
		if (nObject != -1) {
			fprintf(my_file, "gameData.objs.objects: ");
			while (nObject != -1) {
				fprintf(my_file, "[%8s %8s %3i] ", objectTypes(nObject), object_ids(nObject), nObject);
				nObject = gameData.objs.objects[nObject].next;
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
// which is not true.  The setting of nSegment is bogus.
void write_matcen_text(FILE *my_file)
{
	int	i, j, k;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "Materialization centers:\n");
	for (i=0; i<gameData.matCens.nBotCenters; i++) {
		int	triggerCount=0, nSegment, nFuelCen;

		fprintf(my_file, "tFuelCenInfo[%02i].Segment = %04i  ", i, gameData.matCens.fuelCenters[i].nSegment);
		fprintf(my_file, "Segment2[%04i].nMatCen = %02i  ", gameData.matCens.fuelCenters[i].nSegment, gameData.segs.segment2s[gameData.matCens.fuelCenters[i].nSegment].nMatCen);

		nFuelCen = gameData.matCens.botGens[i].nFuelCen;
		if (gameData.matCens.fuelCenters[nFuelCen].Type != SEGMENT_IS_ROBOTMAKER)
			err_printf(my_file, "Error: Matcen %i corresponds to gameData.matCens.fuelCenters %i, which has nType %i (%s).\n", i, nFuelCen, gameData.matCens.fuelCenters[nFuelCen].Type, Special_names[gameData.matCens.fuelCenters[nFuelCen].Type]);

		nSegment = gameData.matCens.fuelCenters[nFuelCen].nSegment;

		//	Find tTrigger for this materialization center.
		for (j=0; j<gameData.trigs.nTriggers; j++) {
			if (gameData.trigs.triggers[j].nType == TT_MATCEN) {
				for (k=0; k<gameData.trigs.triggers[j].nLinks; k++)
					if (gameData.trigs.triggers[j].seg[k] == nSegment) {
						fprintf(my_file, "Trigger = %2i  ", j );
						triggerCount++;
					}
			}
		}
		fprintf(my_file, "\n");

		if (triggerCount == 0)
			err_printf(my_file, "Error: Matcen %i in tSegment %i has no tTrigger!\n", i, nSegment);

	}
}

// ----------------------------------------------------------------------------
void write_wall_text(FILE *my_file)
{
	int	i, j;
	byte	wallFlags[MAX_WALLS];
	tSegment *segp;
	tSide *sideP;
	tWall	*wallP;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "gameData.walls.walls:\n");
	for (i=0, wallP = gameData.walls.walls; i<gameData.walls.nWalls; i++, wallP++) {
		int	nSegment, nSide;

		fprintf(my_file, 
			"Wall %03i: seg=%3i, tSide=%2i, nLinkedWall=%3i, nType=%s, flags=%4x, hps=%3i, tTrigger=%2i, nClip=%2i, keys=%2i, state=%i\n", i,
			wallP->nSegment, 
			wallP->nSide, 
			wallP->nLinkedWall, 
			pszWallNames[wallP->nType], 
			wallP->flags, wallP->hps >> 16, 
			wallP->nTrigger, 
			wallP->nClip, 
			wallP->keys, 
			wallP->state);

		if (wallP->nTrigger >= gameData.trigs.nTriggers)
			fprintf(my_file, "Wall %03d points to invalid tTrigger %d\n",i,wallP->nTrigger);

		nSegment = wallP->nSegment;
		nSide = wallP->nSide;

		if (WallNumI (nSegment, nSide) != i)
			err_printf(my_file, "Error: Wall %i points at tSegment %i, tSide %i, but that tSegment doesn't point back (it's nWall = %i)\n", 
							i, nSegment, nSide, WallNumI (nSegment, nSide));
	}

	for (i=0; i<MAX_WALLS; i++)
		wallFlags[i] = 0;

	for (i=0, segp = gameData.segs.segments; i<=gameData.segs.nLastSegment; i++, segp++) {
		for (j=0, sideP = segp->sides; j<MAX_SIDES_PER_SEGMENT; j++, sideP++) {
			short nWall = WallNumP (sideP);
			if (IS_WALL (nWall))
				if (wallFlags[nWall])
					err_printf(my_file, "Error: Wall %i appears in two or more segments, including tSegment %i, tSide %i.\n", sideP->nWall, i, j);
				else
					wallFlags[nWall] = 1;
		}
	}

}

//typedef struct tTrigger {
//	byte		nType;
//	short		flags;
//	fix		value;
//	fix		time;
//	byte		link_num;
//	short 	nLinks;
//	short 	seg[MAX_WALLS_PER_LINK];
//	short		tSide[MAX_WALLS_PER_LINK];
//	} tTrigger;

// ----------------------------------------------------------------------------
void write_player_text(FILE *my_file)
{
	int	i, num_players=0;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "gameData.multiplayer.players:\n");
	for (i=0; i<=gameData.objs.nLastObject; i++) {
		if (gameData.objs.objects[i].nType == OBJ_PLAYER) {
			num_players++;
			fprintf(my_file, "Player %2i is tObject #%3i in tSegment #%3i.\n", gameData.objs.objects[i].id, i, gameData.objs.objects[i].nSegment);
		}
	}

	if (num_players != MAX_PLAYERS)
		err_printf(my_file, "Error: %i tPlayer gameData.objs.objects.  %i are required.\n", num_players, MAX_PLAYERS);
	if (num_players > MAX_MULTI_PLAYERS)
		err_printf(my_file, "Error: %i tPlayer gameData.objs.objects.  %i are required.\n", num_players, MAX_PLAYERS);
}

// ----------------------------------------------------------------------------
void write_trigger_text(FILE *my_file)
{
	int	i, j, w;
	tWall	*wallP;

	fprintf(my_file, "-----------------------------------------------------------------------------\n");
	fprintf(my_file, "gameData.trigs.triggers:\n");
	for (i=0; i<gameData.trigs.nTriggers; i++) {
		fprintf(my_file, "Trigger %03i: nType=%02x flags=%04x, value=%08x, time=%8x, nLinks=%i ", i,
			gameData.trigs.triggers[i].nType, gameData.trigs.triggers[i].flags, gameData.trigs.triggers[i].value, gameData.trigs.triggers[i].time, gameData.trigs.triggers[i].nLinks);

		for (j=0; j<gameData.trigs.triggers[i].nLinks; j++)
			fprintf(my_file, "[%03i:%i] ", gameData.trigs.triggers[i].seg[j], gameData.trigs.triggers[i].nSide[j]);

		//	Find which tWall this tTrigger is connected to.
		for (w=gameData.walls.nWalls, wallP = gameData.walls.walls; w; w--, wallP++)
			if (wallP->nTrigger == i)
				break;

		if (w == gameData.walls.nWalls)
			err_printf(my_file, "\nError: Trigger %i is not connected to any tWall, so it can never be triggered.\n", i);
		else
			fprintf(my_file, "Attached to seg:tSide = %i:%i, tWall %i\n", 
				wallP->nSegment, wallP->nSide, WallNumI (wallP->nSegment, wallP->nSide));

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

	dump_used_texturesLevel(my_file, 0);
	sayTotals(my_file, gameData.segs.szLevelFilename);

	fprintf(my_file, "\nNumber of segments:   %4i\n", gameData.segs.nLastSegment+1);
	fprintf(my_file, "Number of gameData.objs.objects:    %4i\n", gameData.objs.nLastObject+1);
	fprintf(my_file, "Number of walls:      %4i\n", gameData.walls.nWalls);
	fprintf(my_file, "Number of open doors: %4i\n", gameData.walls.nOpenDoors);
	fprintf(my_file, "Number of triggers:   %4i\n", gameData.trigs.nTriggers);
	fprintf(my_file, "Number of matcens:    %4i\n", gameData.matCens.nBotCenters);
	fprintf(my_file, "\n");

	write_segment_text(my_file);

	write_fuelcen_text(my_file);

	write_matcen_text(my_file);

	write_player_text(my_file);

	write_wall_text(my_file);

	write_trigger_text(my_file);

	write_exit_text(my_file);

	//	---------- Find control center tSegment ----------
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

//	Adam: Change NUM_ADAM_LEVELS to the number of levels.
#define	NUM_ADAM_LEVELS	30

//	Adam: Stick the names here.
char *AdamLevel_names[NUM_ADAM_LEVELS] = {
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

typedef struct tBitmapFile	{
	char			name[15];
} tBitmapFile;

extern tBitmapFile gameData.pig.tex.bitmapFiles[ MAX_BITMAP_FILES ];

int	Ignore_tmap_num2_error;

// ----------------------------------------------------------------------------
void determine_used_texturesLevel(int loadLevelFlag, int sharewareFlag, int level_num, int *tmap_buf, int *wall_buf, byte *level_tmap_buf, int max_tmap)
{
	int	nSegment, nSide, nObject=max_tmap;
	int	i, j;

	Assert(sharewareFlag != -17);

	for (i=0; i<MAX_BITMAP_FILES; i++)
		tmap_buf[i] = 0;

	if (loadLevelFlag) {
		LoadLevelSub(AdamLevel_names[level_num]);
	}


	//	Process robots.
	for (nObject=0; nObject<=gameData.objs.nLastObject; nObject++) {
		tObject *objP = &gameData.objs.objects[nObject];

		if (objP->renderType == RT_POLYOBJ) {
			tPolyModel *po = &gameData.models.polyModels[objP->rType.polyObjInfo.nModel];

			for (i=0; i<po->nTextures; i++) {

				int	tli = gameData.pig.tex.objBmIndex[gameData.pig.tex.pObjBmIndex[po->nFirstTexture+i]].index;
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

	//	Process walls and tSegment sides.
	for (nSegment=0; nSegment<=gameData.segs.nLastSegment; nSegment++) {
		tSegment	*segp = &gameData.segs.segments[nSegment];

		for (nSide=0; nSide<MAX_SIDES_PER_SEGMENT; nSide++) {
			tSide	*sideP = &segp->sides[nSide];

			if (sideP->nWall != -1) {
				int nClip = gameData.walls.walls[sideP->nWall].nClip;
				if (nClip != -1) {

					// -- int nFrameCount = gameData.walls.anims[nClip].nFrameCount;

					wall_buf[nClip] = 1;

					for (j=0; j<1; j++) {	//	Used to do through nFrameCount, but don't really want all the door01#3 stuff.
						int	nBaseTex;

						nBaseTex = gameData.pig.tex.pBmIndex[gameData.walls.anims[nClip].frames[j]].index;
						Assert((nBaseTex >= 0) && (nBaseTex < MAX_BITMAP_FILES);
						tmap_buf[nBaseTex]++;
						if (level_tmap_buf[nBaseTex] == -1)
							level_tmap_buf[nBaseTex] = level_num;
					}
				}
			} else if (segp->children[nSide] == -1) {

				if (sideP->nBaseTex >= 0)
					if (sideP->nBaseTex < MAX_BITMAP_FILES) {
						Assert(gameData.pig.tex.pBmIndex[sideP->nBaseTex].index < MAX_BITMAP_FILES);
						tmap_buf[gameData.pig.tex.pBmIndex[sideP->nBaseTex].index]++;
						if (level_tmap_buf[gameData.pig.tex.pBmIndex[sideP->nBaseTex].index] == -1)
							level_tmap_buf[gameData.pig.tex.pBmIndex[sideP->nBaseTex].index] = level_num;
					} else
						Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.

				if ((sideP->nOvlTex) != 0)
					if ((sideP->nOvlTex) < MAX_BITMAP_FILES) {
						Assert(gameData.pig.tex.pBmIndex[sideP->nOvlTex].index < MAX_BITMAP_FILES);
						tmap_buf[gameData.pig.tex.pBmIndex[sideP->nOvlTex].index]++;
						if (level_tmap_buf[gameData.pig.tex.pBmIndex[sideP->nOvlTex].index] == -1)
							level_tmap_buf[gameData.pig.tex.pBmIndex[sideP->nOvlTex].index] = level_num;
					} else {
						if (!Ignore_tmap_num2_error)
							Int3();	//	Error, bogus texture map.  Should not be greater than max_tmap.
						Ignore_tmap_num2_error = 1;
						sideP->nOvlTex = 0;
						sideP->nOvlOrient = 0;
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
// --05/17/95--				level_name = RegisteredLevel_names[level_num - NUM_SHAREWARE_LEVELS];
// --05/17/95--			} else {
// --05/17/95--				Assert((level_num >= 0) && (level_num < NUM_SHAREWARE_LEVELS);
// --05/17/95--				level_name = SharewareLevel_names[level_num];
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
			level_name = AdamLevel_names[level_num];

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
			if (gameData.pig.tex.bitmaps[gameData.pig.tex.pBmIndex[i].index].bmTexBuf == &bogus_data)
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
sayTotals(FILE *my_file, char *level_name)
{
	int	i;		//, nObject;
	int	totalRobots = 0;
	int	objects_processed = 0;

	int	usedObjects[MAX_OBJECTS];

	fprintf(my_file, "\nLevel %s\n", level_name);

	for (i=0; i<MAX_OBJECTS; i++)
		usedObjects[i] = 0;

	while (objects_processed < gameData.objs.nLastObject+1) {
		int	j, objtype, objid, objcount, cur_obj_val, min_obj_val, min_objnum;

		//	Find new min nObject.
		min_obj_val = 0x7fff0000;
		min_objnum = -1;

		for (j=0; j<=gameData.objs.nLastObject; j++) {
			if (!usedObjects[j] && gameData.objs.objects[j].nType!=OBJ_NONE) {
				cur_obj_val = gameData.objs.objects[j].nType * 1000 + gameData.objs.objects[j].id;
				if (cur_obj_val < min_obj_val) {
					min_objnum = j;
					min_obj_val = cur_obj_val;
				}
			}
		}
		if ((min_objnum == -1) || (gameData.objs.objects[min_objnum].nType == 255))
			break;

		objcount = 0;

		objtype = gameData.objs.objects[min_objnum].nType;
		objid = gameData.objs.objects[min_objnum].id;

		for (i=0; i<=gameData.objs.nLastObject; i++) {
			if (!usedObjects[i]) {

				if (((gameData.objs.objects[i].nType == objtype) && (gameData.objs.objects[i].id == objid)) ||
						((gameData.objs.objects[i].nType == objtype) && (objtype == OBJ_PLAYER)) ||
						((gameData.objs.objects[i].nType == objtype) && (objtype == OBJ_COOP)) ||
						((gameData.objs.objects[i].nType == objtype) && (objtype == OBJ_HOSTAGE))) {
					if (gameData.objs.objects[i].nType == OBJ_ROBOT)
						totalRobots++;
					usedObjects[i] = 1;
					objcount++;
					objects_processed++;
				}
			}
		}

		if (objcount) {
			fprintf(my_file, "Object: ");
			fprintf(my_file, "%8s %8s %3i\n", objectTypes(min_objnum), object_ids(min_objnum), objcount);
		}
	}

	fprintf(my_file, "Total robots = %3i\n", totalRobots);
}

int	First_dumpLevel = 0;
int	Last_dumpLevel = NUM_ADAM_LEVELS-1;

// ----------------------------------------------------------------------------
void sayTotals_all(void)
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

	for (i=First_dumpLevel; i<=Last_dumpLevel; i++) {
#if TRACE
		con_printf (CONDBG, "Level %i\n", i+1);
#endif
		LoadLevelSub(AdamLevel_names[i]);
		sayTotals(my_file, AdamLevel_names[i]);
	}

//--05/17/95--	for (i=0; i<NUM_SHAREWARE_LEVELS; i++) {
//--05/17/95--		LoadLevelSub(SharewareLevel_names[i]);
//--05/17/95--		sayTotals(my_file, SharewareLevel_names[i]);
//--05/17/95--	}
//--05/17/95--
//--05/17/95--	for (i=0; i<NUM_REGISTERED_LEVELS; i++) {
//--05/17/95--		LoadLevelSub(RegisteredLevel_names[i]);
//--05/17/95--		sayTotals(my_file, RegisteredLevel_names[i]);
//--05/17/95--	}

	fclose(my_file);

}

void dump_used_texturesLevel(FILE *my_file, int level_num)
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

	determine_used_texturesLevel(0, 1, level_num, temp_tmap_buf, temp_wall_buf, level_tmap_buf, MAX_BITMAP_FILES);
// -- 	determine_used_texturesRobots(temp_tmap_buf);
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

sayTotals_all();

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
// --05/17/95--		determine_used_texturesLevel(1, 1, i, temp_tmap_buf, temp_wall_buf, level_tmap_buf, MAX_BITMAP_FILES);
// --05/17/95--		fprintf(my_file, "\nTextures used in [%s]\n", SharewareLevel_names[i]);
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
// --05/17/95--		determine_used_texturesLevel(1, 0, i, temp_tmap_buf, temp_wall_buf, level_tmap_buf, MAX_BITMAP_FILES);
// --05/17/95--		fprintf(my_file, "\nTextures used in [%s]\n", RegisteredLevel_names[i]);
// --05/17/95--		say_used_tmaps(my_file, temp_tmap_buf);
// --05/17/95--		merge_buffers(perm_tmap_buf, temp_tmap_buf, MAX_BITMAP_FILES);
// --05/17/95--	}
// --05/17/95--
// --05/17/95--	fprintf(my_file, "\n\nUsed textures in all (including registered) mines:\n");
// --05/17/95--	say_used_tmaps(my_file, perm_tmap_buf);
// --05/17/95--
// --05/17/95--	fprintf(my_file, "\nUnused textures in all (including registered) mines:\n");
// --05/17/95--	say_unused_tmaps(my_file, perm_tmap_buf);

	for (i=First_dumpLevel; i<=Last_dumpLevel; i++) {
		determine_used_texturesLevel(1, 0, i, temp_tmap_buf, temp_wall_buf, level_tmap_buf, MAX_BITMAP_FILES);
		fprintf(my_file, "\nTextures used in [%s]\n", AdamLevel_names[i]);
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
