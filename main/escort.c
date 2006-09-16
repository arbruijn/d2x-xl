/* $Id: escort.c,v 1.7 2003/10/10 09:36:35 btb Exp $ */
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
 * Escort robot behavior.
 *
 * Old Log:
 * Revision 1.1  1995/05/06  23:32:19  mike
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>		// for printf()
#include <stdlib.h>		// for rand() and qsort()
#include <string.h>		// for memset()

#include "inferno.h"
#include "mono.h"
#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "3d.h"
#include "palette.h"

#include "object.h"
#include "error.h"
#include "ai.h"
#include "robot.h"
#include "fvi.h"
#include "physics.h"
#include "wall.h"
#include "player.h"
#include "fireball.h"
#include "game.h"
#include "powerup.h"
#include "cntrlcen.h"
#include "gauges.h"
#include "key.h"
#include "fuelcen.h"
#include "sounds.h"
#include "screens.h"
#include "text.h"
#include "gamefont.h"
#include "newmenu.h"
#include "playsave.h"
#include "gameseq.h"
#include "automap.h"
#include "laser.h"
#include "pa_enabl.h"
#include "escort.h"
#include "ogl_init.h"
#include "network.h"
#include "gameseg.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

void SayEscortGoal(int goal_num);
void ShowEscortMenu(char *msg);

int nEscortGoalText[MAX_ESCORT_GOALS] = {
	702,
	703,
	704,
	705,
	706,
	707,
	708,
	709,
	710,
	711,
	712,
	713,
	714,
	715,
	716,
	717,
	718,
	719,
	720,
	721,
	722,
	723,
	724
// -- too much work -- 	"KAMIKAZE  "
};

//	-------------------------------------------------------------------------------------------------

void InitBuddyForLevel(void)
{
	int	i;

	gameData.escort.bMayTalk = 0;
	gameData.escort.nObjNum = -1;
	gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escort.nSpecialGoal = -1;
	gameData.escort.nGoalIndex = -1;
	gameData.escort.bMsgsSuppressed = 0;

	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (gameData.bots.pInfo[gameData.objs.objects[i].id].companion)
			break;
	if (i <= gameData.objs.nLastObject)
		gameData.escort.nObjNum = i;

	gameData.escort.xSorryTime = -F1_0;

	gameData.escort.bSearchingMarker = -1;
	gameData.escort.nLastKey = -1;
}

//	-----------------------------------------------------------------------------
//	See if segment from curseg through sidenum is reachable.
//	Return true if it is reachable, else return false.
int SegmentIsReachable (int curseg, short sidenum)
{
return AIDoorIsOpenable (NULL, gameData.segs.segments + curseg, sidenum);
}


//	-----------------------------------------------------------------------------
//	Create a breadth-first list of segments reachable from current segment.
//	max_segs is maximum number of segments to search.  Use MAX_SEGMENTS to search all.
//	On exit, *length <= max_segs.
//	Input:
//		start_seg
//	Output:
//		bfs_list:	array of shorts, each reachable segment.  Includes start segment.
//		length:		number of elements in bfs_list
void CreateBfsList(int start_seg, short bfs_list[], int *length, int max_segs)
{
	int		head, tail;
	sbyte		bVisited[MAX_SEGMENTS];

#if 1
	memset (bVisited, 0, MAX_SEGMENTS * sizeof (sbyte));
#else
	for (i=0; i<MAX_SEGMENTS; i++)
		bVisited[i] = 0;
#endif
	head = 0;
	tail = 0;

	bfs_list[head++] = start_seg;
	bVisited[start_seg] = 1;

while ((head != tail) && (head < max_segs)) {
	int		i;
	short		curseg;
	segment	*cursegp;

	curseg = bfs_list[tail++];
	cursegp = gameData.segs.segments + curseg;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		int	connected_seg = cursegp->children[i];
		if (!IS_CHILD (connected_seg))
			continue;
		if (bVisited[connected_seg])
			continue;
		if (!SegmentIsReachable(curseg, (short) i))
			continue;
		bfs_list[head++] = connected_seg;
		if (head >= max_segs)
			break;
		bVisited[connected_seg] = 1;
		Assert(head < MAX_SEGMENTS);
		}
	}
*length = head;
}

//	-----------------------------------------------------------------------------
//	Return true if ok for buddy to talk, else return false.
//	Buddy is allowed to talk if the segment he is in does not contain a blastable wall that has not been blasted
//	AND he has never yet, since being initialized for level, been allowed to talk.
int BuddyMayTalk(void)
{
	int		i;
	segment	*segp;

	if (gameData.objs.objects[gameData.escort.nObjNum].type != OBJ_ROBOT) {
		gameData.escort.bMayTalk = 0;
		return 0;
	}

	if (gameData.escort.bMayTalk)
		return 1;

	if ((gameData.objs.objects[gameData.escort.nObjNum].type == OBJ_ROBOT) && 
		(gameData.escort.nObjNum <= gameData.objs.nLastObject) && 
		!gameData.bots.pInfo[gameData.objs.objects[gameData.escort.nObjNum].id].companion) {
		for (i=0; i<=gameData.objs.nLastObject; i++)
			if (gameData.bots.pInfo[gameData.objs.objects[i].id].companion)
				break;
		if (i > gameData.objs.nLastObject)
			return 0;
		else
			gameData.escort.nObjNum = i;
	}

	segp = gameData.segs.segments + gameData.objs.objects[gameData.escort.nObjNum].segnum;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		short	wall_num = WallNumP (segp, (short) i);

		if (IS_WALL (wall_num)) {
			if ((gameData.walls.walls[wall_num].type == WALL_BLASTABLE) && 
				!(gameData.walls.walls[wall_num].flags & WALL_BLASTED))
				return 0;
		}

		//	Check one level deeper.
		if (IS_CHILD(segp->children[i])) {
			int		j;
			segment	*csegp = &gameData.segs.segments[segp->children[i]];

			for (j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
				short	wall2 = WallNumP (csegp, (short) j);

				if (IS_WALL (wall2)) {
					if ((gameData.walls.walls[wall2].type == WALL_BLASTABLE) && !(gameData.walls.walls[wall2].flags & WALL_BLASTED))
						return 0;
				}
			}
		}
	}

	gameData.escort.bMayTalk = 1;
	return 1;
}

//	--------------------------------------------------------------------------------------------
void DetectEscortGoalAccomplished(int index)
{
	int		i,j;
	int		bDetected = 0;
	object	*objP;

	if (!gameData.escort.bMayTalk)
		return;

	//	If goal is to go away, how can it be achieved?
	if (gameData.escort.nSpecialGoal == ESCORT_GOAL_SCRAM)
		return;

//	See if goal found was a key.  Need to handle default goals differently.
//	Note, no buddy_met_goal sound when blow up reactor or exit.  Not great, but ok
//	since for reactor, noisy, for exit, buddy is disappearing.
if ((gameData.escort.nSpecialGoal == -1) && (gameData.escort.nGoalIndex == index)) {
	bDetected = 1;
	goto dega_ok;
}

if ((gameData.escort.nGoalIndex <= ESCORT_GOAL_RED_KEY) && (index >= 0)) {
	objP = gameData.objs.objects + index;
	if (objP->type == OBJ_POWERUP)  {
		ubyte id = objP->id;
		if (id == POW_KEY_BLUE) {
			if (gameData.escort.nGoalIndex == ESCORT_GOAL_BLUE_KEY) {
				bDetected = 1;
				goto dega_ok;
				}
			}
		else if (id == POW_KEY_GOLD) {
			if (gameData.escort.nGoalIndex == ESCORT_GOAL_GOLD_KEY) {
				bDetected = 1;
				goto dega_ok;
				}
			}
		else if (id == POW_KEY_RED) {
			if (gameData.escort.nGoalIndex == ESCORT_GOAL_RED_KEY) {
				bDetected = 1;
				goto dega_ok;
				}
			}
		}
	}
	if (gameData.escort.nSpecialGoal != -1){
		if (gameData.escort.nSpecialGoal == ESCORT_GOAL_ENERGYCEN) {
			if (index == -4)
				bDetected = 1;
			else {
				for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
					if (gameData.segs.segments[index].children[i] == gameData.escort.nGoalIndex) {
						bDetected = 1;
						goto dega_ok;
						}
					else {
						for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
							if (gameData.segs.segments[i].children[j] == gameData.escort.nGoalIndex) {
								bDetected = 1;
								goto dega_ok;
							}
					}
			}
		} else if ((gameData.objs.objects[index].type == OBJ_POWERUP) && 
					  (gameData.escort.nSpecialGoal == ESCORT_GOAL_POWERUP))
			bDetected = 1;	//	Any type of powerup picked up will do.
		else if ((gameData.objs.objects[index].type == gameData.objs.objects[gameData.escort.nGoalIndex].type) && 
					(gameData.objs.objects[index].id == gameData.objs.objects[gameData.escort.nGoalIndex].id)) {
			//	Note: This will help a little bit in making the buddy believe a goal is satisfied.  Won't work for a general goal like "find any powerup"
			// because of the insistence of both type and id matching.
			bDetected = 1;
		}
	}

dega_ok: ;
	if (bDetected && BuddyMayTalk()) {
		DigiPlaySampleOnce(SOUND_BUDDY_MET_GOAL, F1_0);
		gameData.escort.nGoalIndex = -1;
		gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
		gameData.escort.nSpecialGoal = -1;
		gameData.escort.bSearchingMarker = -1;
	}
}

//	-------------------------------------------------------------------------------------------------

void ChangeGuidebotName()
{
	newmenu_item m;
	char text[GUIDEBOT_NAME_LEN+1]="";
	int item;

	strcpy(text,gameData.escort.szName);

	memset (&m, 0, sizeof (m));
	m.type=NM_TYPE_INPUT; 
	m.text_len = GUIDEBOT_NAME_LEN; 
	m.text = text;
	item = ExecMenu( NULL, "Enter Guide-bot name:", 1, &m, NULL, NULL );

	if (item != -1) {
		strcpy(gameData.escort.szName,text);
		strcpy(gameData.escort.szRealName,text);
		WritePlayerFile();
	}
}

//	-----------------------------------------------------------------------------
void _CDECL_ BuddyMessage(char * format, ... )
{
	if (gameData.escort.bMsgsSuppressed)
		return;

	if (gameData.app.nGameMode & GM_MULTI)
		return;

if ((gameData.escort.xLastMsgTime + F1_0 < gameData.app.xGameTime) || 
	 (gameData.escort.xLastMsgTime > gameData.app.xGameTime)) {
	if (BuddyMayTalk()) {
		char	gb_str[16], new_format[128];
		va_list	args;
		int t;

		va_start(args, format );
		vsprintf(new_format, format, args);
		va_end(args);

		gb_str[0] = 1;
		gb_str[1] = GrFindClosestColor (gamePalette, 28, 28, 0);
		strcpy(&gb_str[2], gameData.escort.szName);
		t = (int) strlen(gb_str);
		gb_str[t] = ':';
		gb_str[t+1] = 1;
		gb_str[t+2] = GrFindClosestColor (gamePalette, 0, 31, 0);
		gb_str[t+3] = 0;

		HUDInitMessage("%s %s", gb_str, new_format);
		gameData.escort.xLastMsgTime = gameData.app.xGameTime;
		}
	}
}

//	-----------------------------------------------------------------------------
//	Return true if marker #id has been placed.
int MarkerExistsInMine(int id)
{
	int	i;

	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (gameData.objs.objects[i].type == OBJ_MARKER)
			if (gameData.objs.objects[i].id == id)
				return 1;

	return 0;
}

//	-----------------------------------------------------------------------------
void EscortSetSpecialGoal(int special_key)
{
	int marker_key;

	gameData.escort.bMsgsSuppressed = 0;

	if (!gameData.escort.bMayTalk) {
		BuddyMayTalk();
		if (!gameData.escort.bMayTalk) {
			int	i;

			for (i=0; i<=gameData.objs.nLastObject; i++)
				if ((gameData.objs.objects[i].type == OBJ_ROBOT) && gameData.bots.pInfo[gameData.objs.objects[i].id].companion) {
					HUDInitMessage(TXT_GB_RELEASE, gameData.escort.szName);
					break;
				}
			if (i == gameData.objs.nLastObject+1)
				HUDInitMessage(TXT_GB_NONE);

			return;
		}
	}

	special_key = special_key & (~KEY_SHIFTED);

	marker_key = special_key;
	

	if (gameData.escort.nLastKey == special_key)
	{
		if ((gameData.escort.bSearchingMarker == -1) && (special_key != KEY_0)) {
			if (MarkerExistsInMine(marker_key - KEY_1))
				gameData.escort.bSearchingMarker = marker_key - KEY_1;
			else {
				gameData.escort.xLastMsgTime = 0;	//	Force this message to get through.
				BuddyMessage("Marker %i not placed.", marker_key - KEY_1 + 1);
				gameData.escort.bSearchingMarker = -1;
			}
		} else {
			gameData.escort.bSearchingMarker = -1;
		}
	}

	gameData.escort.nLastKey = special_key;

	if (special_key == KEY_0)
		gameData.escort.bSearchingMarker = -1;
		
	if ( gameData.escort.bSearchingMarker != -1 )
		gameData.escort.nSpecialGoal = ESCORT_GOAL_MARKER1 + marker_key - KEY_1;
	else {
		switch (special_key) {
			case KEY_1:	
				gameData.escort.nSpecialGoal = ESCORT_GOAL_ENERGY;			
				break;
			case KEY_2:	
				gameData.escort.nSpecialGoal = ESCORT_GOAL_ENERGYCEN;		
				break;
			case KEY_3:	
				gameData.escort.nSpecialGoal = ESCORT_GOAL_SHIELD;			
				break;
			case KEY_4:	
				gameData.escort.nSpecialGoal = ESCORT_GOAL_POWERUP;		
				break;
			case KEY_5:	
				gameData.escort.nSpecialGoal = ESCORT_GOAL_ROBOT;			
				break;
			case KEY_6:	
				gameData.escort.nSpecialGoal = ESCORT_GOAL_HOSTAGE;		
				break;
			case KEY_7:	
				gameData.escort.nSpecialGoal = ESCORT_GOAL_SCRAM;			
				break;
			case KEY_8:	
				gameData.escort.nSpecialGoal = ESCORT_GOAL_PLAYER_SPEW;	
				break;
			case KEY_9:	
				gameData.escort.nSpecialGoal = ESCORT_GOAL_EXIT;			
				break;
			case KEY_0:	
				gameData.escort.nSpecialGoal = -1;								
				break;
			default:
				Int3();		//	Oops, called with illegal key value.
		}
	}

	gameData.escort.xLastMsgTime = gameData.app.xGameTime - 2*F1_0;	//	Allow next message to come through.

	SayEscortGoal(gameData.escort.nSpecialGoal);
	// -- gameData.escort.nGoalObject = EscortSetGoalObject();

	gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
}

//	-----------------------------------------------------------------------------
//	Return id of boss.
int GetBossId(void)
{
	int	i;
#if 1
	int	objnum;
	
	for (i = 0; i < extraGameInfo [0].nBossCount; i++) {
		if (0 <= (objnum = gameData.boss.objList [i]))
			return gameData.objs.objects [objnum].id;
		}
#else
	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (gameData.objs.objects[i].type == OBJ_ROBOT)
			if (gameData.bots.pInfo[gameData.objs.objects[i].id].boss_flag)
				return gameData.objs.objects[i].id;
#endif
	return -1;
}

//	-----------------------------------------------------------------------------
//	Return object index if object of objtype, objid exists in mine, else return -1
//	"special" is used to find gameData.objs.objects spewed by player which is hacked into flags field of powerup.
int ExistsInMine2(int segnum, int objtype, int objid, int special)
{
	if (gameData.segs.segments[segnum].objects != -1) {
		int		objnum = gameData.segs.segments[segnum].objects;

		while (objnum != -1) {
			object	*curObjP = &gameData.objs.objects[objnum];

			if (special == ESCORT_GOAL_PLAYER_SPEW) {
				if (curObjP->flags & OF_PLAYER_DROPPED)
					return objnum;
			}

			if (curObjP->type == objtype) {
				//	Don't find escort robots if looking for robot!
				if ((curObjP->type == OBJ_ROBOT) && (gameData.bots.pInfo[curObjP->id].companion))
					;
				else if (objid == -1) {
					if ((objtype == OBJ_POWERUP) && 
						 (curObjP->id != POW_KEY_BLUE) && 
						 (curObjP->id != POW_KEY_GOLD) && 
						 (curObjP->id != POW_KEY_RED))
						return objnum;
					else
						return objnum;
				} else if (curObjP->id == objid)
					return objnum;
			}

			if (objtype == OBJ_POWERUP)
				if (curObjP->contains_count)
					if (curObjP->contains_type == OBJ_POWERUP)
						if (curObjP->contains_id == objid)
							return objnum;

			objnum = curObjP->next;
		}
	}
return -1;
}

//	-----------------------------------------------------------------------------
//	Return nearest object of interest.
//	If special == ESCORT_GOAL_PLAYER_SPEW, then looking for any object spewed by player.
//	-1 means object does not exist in mine.
//	-2 means object does exist in mine, but buddy-bot can't reach it (eg, behind triggered wall)
int ExistsInMine(int start_seg, int objtype, int objid, int special)
{
	int	segindex, segnum;
	short	bfs_list[MAX_SEGMENTS];
	int	length;

	CreateBfsList(start_seg, bfs_list, &length, MAX_SEGMENTS);

	if (objtype == FUELCEN_CHECK) {
		for (segindex = 0; segindex < length; segindex++) {
			segnum = bfs_list [segindex];
			if (gameData.segs.segment2s[segnum].special == SEGMENT_IS_FUELCEN)
				return segnum;
			}
		} 
	else {
		for (segindex = 0; segindex < length; segindex++) {
			int	objnum;

			segnum = bfs_list [segindex];
			objnum = ExistsInMine2(segnum, objtype, objid, special);
			if (objnum != -1)
				return objnum;
		}
	}

	//	Couldn't find what we're looking for by looking at connectivity.
	//	See if it's in the mine.  It could be hidden behind a trigger or switch
	//	which the buddybot doesn't understand.
	if (objtype == FUELCEN_CHECK) {
		for (segnum=0; segnum<=gameData.segs.nLastSegment; segnum++)
			if (gameData.segs.segment2s[segnum].special == SEGMENT_IS_FUELCEN)
				return -2;
	} else {
		for (segnum=0; segnum<=gameData.segs.nLastSegment; segnum++) {
			int	objnum;

			objnum = ExistsInMine2(segnum, objtype, objid, special);
			if (objnum != -1)
				return -2;
		}
	}

	return -1;
}

//	-----------------------------------------------------------------------------
//	Return true if it happened, else return false.
int FindExitSegment(void)
{
	int	i,j;

	//	---------- Find exit doors ----------
	for (i=0; i<=gameData.segs.nLastSegment; i++)
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (gameData.segs.segments[i].children[j] == -2) {
				return i;
			}

	return -1;
}

#define	BUDDY_MARKER_TEXT_LEN	25

//	-----------------------------------------------------------------------------
void SayEscortGoal(int goal_num)
{
	if (gameStates.app.bPlayerIsDead)
		return;

	switch (goal_num) {
		case ESCORT_GOAL_BLUE_KEY:		
			BuddyMessage(TXT_FIND_BLUEKEY);			
			break;
		case ESCORT_GOAL_GOLD_KEY:		
			BuddyMessage(TXT_FIND_GOLDKEY);		
			break;
		case ESCORT_GOAL_RED_KEY:		
			BuddyMessage(TXT_FIND_REDKEY);			
			break;
		case ESCORT_GOAL_CONTROLCEN:	
			BuddyMessage(TXT_FIND_REACTOR);			
			break;
		case ESCORT_GOAL_EXIT:			
			BuddyMessage(TXT_FIND_EXIT);				
			break;
		case ESCORT_GOAL_ENERGY:		
			BuddyMessage(TXT_FIND_ENERGY);				
			break;
		case ESCORT_GOAL_ENERGYCEN:	
			BuddyMessage(TXT_FIND_ECENTER);	
			break;
		case ESCORT_GOAL_SHIELD:		
			BuddyMessage(TXT_FIND_SHIELD);			
			break;
		case ESCORT_GOAL_POWERUP:		
			BuddyMessage(TXT_FIND_POWERUP);			
			break;
		case ESCORT_GOAL_ROBOT:			
			BuddyMessage(TXT_FIND_ROBOT);			
			break;
		case ESCORT_GOAL_HOSTAGE:		
			BuddyMessage(TXT_FIND_HOSTAGE);			
			break;
		case ESCORT_GOAL_SCRAM:			
			BuddyMessage(TXT_STAYING_AWAY);			
			break;
		case ESCORT_GOAL_BOSS:			
			BuddyMessage(TXT_FIND_BOSS);		
			break;
		case ESCORT_GOAL_PLAYER_SPEW:	
			BuddyMessage(TXT_FIND_YOURSTUFF);	
			break;
		case ESCORT_GOAL_MARKER1:
		case ESCORT_GOAL_MARKER2:
		case ESCORT_GOAL_MARKER3:
		case ESCORT_GOAL_MARKER4:
		case ESCORT_GOAL_MARKER5:
		case ESCORT_GOAL_MARKER6:
		case ESCORT_GOAL_MARKER7:
		case ESCORT_GOAL_MARKER8:
		case ESCORT_GOAL_MARKER9: { 
				char marker_text[BUDDY_MARKER_TEXT_LEN];
				
			strncpy(marker_text, gameData.marker.szMessage[goal_num-ESCORT_GOAL_MARKER1], BUDDY_MARKER_TEXT_LEN-1);
			marker_text[BUDDY_MARKER_TEXT_LEN-1] = 0;
			BuddyMessage(TXT_FIND_MARKER, goal_num-ESCORT_GOAL_MARKER1+1, marker_text);
			break;
			}
	}
}

//	-----------------------------------------------------------------------------
void EscortCreatePathToGoal(object *objP)
{
	short			goal_seg = -1;
	short			objnum = OBJ_IDX (objP);
	ai_static	*aip = &objP->ctype.ai_info;
	ai_local		*ailp = &gameData.ai.localInfo[objnum];

if (gameData.escort.nSpecialGoal != -1)
	gameData.escort.nGoalObject = gameData.escort.nSpecialGoal;
gameData.escort.nKillObject = -1;
if (gameData.escort.bSearchingMarker != -1) {
	gameData.escort.nGoalIndex = ExistsInMine(objP->segnum, OBJ_MARKER, gameData.escort.nGoalObject-ESCORT_GOAL_MARKER1, -1);
	if (gameData.escort.nGoalIndex > -1)
		goal_seg = gameData.objs.objects[gameData.escort.nGoalIndex].segnum;
	} 
else {
	switch (gameData.escort.nGoalObject) {
		case ESCORT_GOAL_BLUE_KEY:
			gameData.escort.nGoalIndex = ExistsInMine(objP->segnum, OBJ_POWERUP, POW_KEY_BLUE, -1);
			if (gameData.escort.nGoalIndex > -1) 
				goal_seg = gameData.objs.objects[gameData.escort.nGoalIndex].segnum;
			break;
		case ESCORT_GOAL_GOLD_KEY:
			gameData.escort.nGoalIndex = ExistsInMine(objP->segnum, OBJ_POWERUP, POW_KEY_GOLD, -1);
			if (gameData.escort.nGoalIndex > -1) 
				goal_seg = gameData.objs.objects[gameData.escort.nGoalIndex].segnum;
			break;
		case ESCORT_GOAL_RED_KEY:
			gameData.escort.nGoalIndex = ExistsInMine(objP->segnum, OBJ_POWERUP, POW_KEY_RED, -1);
			if (gameData.escort.nGoalIndex > -1) 
				goal_seg = gameData.objs.objects[gameData.escort.nGoalIndex].segnum;
			break;
		case ESCORT_GOAL_CONTROLCEN:
			gameData.escort.nGoalIndex = ExistsInMine(objP->segnum, OBJ_CNTRLCEN, -1, -1);
			if (gameData.escort.nGoalIndex > -1) 
				goal_seg = gameData.objs.objects[gameData.escort.nGoalIndex].segnum;
			break;
		case ESCORT_GOAL_EXIT:
		case ESCORT_GOAL_EXIT2:
			goal_seg = FindExitSegment();
			gameData.escort.nGoalIndex = goal_seg;
			break;
		case ESCORT_GOAL_ENERGY:
			gameData.escort.nGoalIndex = ExistsInMine(objP->segnum, OBJ_POWERUP, POW_ENERGY, -1);
			if (gameData.escort.nGoalIndex > -1) 
				goal_seg = gameData.objs.objects[gameData.escort.nGoalIndex].segnum;
			break;
		case ESCORT_GOAL_ENERGYCEN:
			goal_seg = ExistsInMine(objP->segnum, FUELCEN_CHECK, -1, -1);
			gameData.escort.nGoalIndex = goal_seg;
			break;
		case ESCORT_GOAL_SHIELD:
			gameData.escort.nGoalIndex = ExistsInMine(objP->segnum, OBJ_POWERUP, POW_SHIELD_BOOST, -1);
			if (gameData.escort.nGoalIndex > -1) 
				goal_seg = gameData.objs.objects[gameData.escort.nGoalIndex].segnum;
			break;
		case ESCORT_GOAL_POWERUP:
			gameData.escort.nGoalIndex = ExistsInMine(objP->segnum, OBJ_POWERUP, -1, -1);
			if (gameData.escort.nGoalIndex > -1) 
				goal_seg = gameData.objs.objects[gameData.escort.nGoalIndex].segnum;
			break;
		case ESCORT_GOAL_ROBOT:
			gameData.escort.nGoalIndex = ExistsInMine(objP->segnum, OBJ_ROBOT, -1, -1);
			if (gameData.escort.nGoalIndex > -1) 
				goal_seg = gameData.objs.objects[gameData.escort.nGoalIndex].segnum;
			break;
		case ESCORT_GOAL_HOSTAGE:
			gameData.escort.nGoalIndex = ExistsInMine(objP->segnum, OBJ_HOSTAGE, -1, -1);
			if (gameData.escort.nGoalIndex > -1) 
				goal_seg = gameData.objs.objects[gameData.escort.nGoalIndex].segnum;
			break;
		case ESCORT_GOAL_PLAYER_SPEW:
			gameData.escort.nGoalIndex = ExistsInMine(objP->segnum, -1, -1, ESCORT_GOAL_PLAYER_SPEW);
			if (gameData.escort.nGoalIndex > -1) 
				goal_seg = gameData.objs.objects[gameData.escort.nGoalIndex].segnum;
			break;
		case ESCORT_GOAL_SCRAM:
			goal_seg = -3;		//	Kinda a hack.
			gameData.escort.nGoalIndex = goal_seg;
			break;
		case ESCORT_GOAL_BOSS: {
			int	boss_id;

			boss_id = GetBossId();
			Assert(boss_id != -1);
			gameData.escort.nGoalIndex = ExistsInMine(objP->segnum, OBJ_ROBOT, boss_id, -1);
			if (gameData.escort.nGoalIndex > -1) 
				goal_seg = gameData.objs.objects[gameData.escort.nGoalIndex].segnum;
			break;
		}
		default:
			Int3();	//	Oops, Illegal value in gameData.escort.nGoalObject.
			goal_seg = 0;
			break;
		}
	}
if ((gameData.escort.nGoalIndex < 0) && (gameData.escort.nGoalIndex != -3)) {	//	I apologize for this statement -- MK, 09/22/95
	if (gameData.escort.nGoalIndex == -1) {
		gameData.escort.xLastMsgTime = 0;	//	Force this message to get through.
		BuddyMessage(TXT_NOT_IN_MINE, GT (nEscortGoalText[gameData.escort.nGoalObject-1]));
		gameData.escort.bSearchingMarker = -1;
		}
	else if (gameData.escort.nGoalIndex == -2) {
		gameData.escort.xLastMsgTime = 0;	//	Force this message to get through.
		BuddyMessage(TXT_CANT_REACH, GT (nEscortGoalText[gameData.escort.nGoalObject-1]));
		gameData.escort.bSearchingMarker = -1;
		}
	else
		Int3();

	gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escort.nSpecialGoal = -1;
	}
else {
	if (goal_seg == -3) {
		create_n_segment_path(objP, 16 + d_rand() * 16, -1);
		aip->path_length = polish_path(objP, &gameData.ai.pointSegs[aip->hide_index], aip->path_length);
		}
	else {
		CreatePathToSegment(objP, goal_seg, gameData.escort.nMaxLength, 1);	//	MK!: Last parm (safety_flag) used to be 1!!
		if (aip->path_length > 3)
			aip->path_length = polish_path(objP, &gameData.ai.pointSegs[aip->hide_index], aip->path_length);
		if ((aip->path_length > 0) && (gameData.ai.pointSegs[aip->hide_index + aip->path_length - 1].segnum != goal_seg)) {
			fix	dist_to_player;
			gameData.escort.xLastMsgTime = 0;	//	Force this message to get through.
			BuddyMessage(TXT_CANT_REACH, GT (nEscortGoalText[gameData.escort.nGoalObject-1]));
			gameData.escort.bSearchingMarker = -1;
			gameData.escort.nGoalObject = ESCORT_GOAL_SCRAM;
			dist_to_player = FindConnectedDistance(&objP->pos, objP->segnum, &gameData.ai.vBelievedPlayerPos, gameData.ai.nBelievedPlayerSeg, 100, WID_FLY_FLAG);
			if (dist_to_player > MIN_ESCORT_DISTANCE)
				create_path_to_player(objP, gameData.escort.nMaxLength, 1);	//	MK!: Last parm used to be 1!
			else {
				create_n_segment_path(objP, 8 + d_rand() * 8, -1);
				aip->path_length = polish_path(objP, gameData.ai.pointSegs + aip->hide_index, aip->path_length);
				}
			}
		}
	ailp->mode = AIM_GOTO_OBJECT;
	SayEscortGoal(gameData.escort.nGoalObject);
	}
}

//	-----------------------------------------------------------------------------
//	Escort robot chooses goal object based on player's keys, location.
//	Returns goal object.
int EscortSetGoalObject(void)
{
	if (gameData.escort.nSpecialGoal != -1)
		return ESCORT_GOAL_UNSPECIFIED;
	else if (!(gameData.objs.console->flags & PLAYER_FLAGS_BLUE_KEY) && 
				 (ExistsInMine(gameData.objs.console->segnum, OBJ_POWERUP, POW_KEY_BLUE, -1) != -1))
		return ESCORT_GOAL_BLUE_KEY;
	else if (!(gameData.objs.console->flags & PLAYER_FLAGS_GOLD_KEY) && 
				 (ExistsInMine(gameData.objs.console->segnum, OBJ_POWERUP, POW_KEY_GOLD, -1) != -1))
		return ESCORT_GOAL_GOLD_KEY;
	else if (!(gameData.objs.console->flags & PLAYER_FLAGS_RED_KEY) && 
				 (ExistsInMine(gameData.objs.console->segnum, OBJ_POWERUP, POW_KEY_RED, -1) != -1))
		return ESCORT_GOAL_RED_KEY;
	else if (gameData.reactor.bDestroyed == 0) {
		int	i;
		
		for (i = 0; i < extraGameInfo [0].nBossCount; i++)
			if ((gameData.boss.objList [i] >= 0) && gameData.boss.nTeleportSegs [i])
				return ESCORT_GOAL_BOSS;
			return ESCORT_GOAL_CONTROLCEN;
		}
	else
		return ESCORT_GOAL_EXIT;
	
}

#define	MAX_ESCORT_TIME_AWAY		(F1_0*4)

fix	Buddy_last_seen_player = 0, Buddy_last_player_path_created;

//	-----------------------------------------------------------------------------
int TimeToVisitPlayer(object *objP, ai_local *ailp, ai_static *aip)
{
	//	Note: This one has highest priority because, even if already going towards player,
	//	might be necessary to create a new path, as player can move.
	if (gameData.app.xGameTime - Buddy_last_seen_player > MAX_ESCORT_TIME_AWAY)
		if (gameData.app.xGameTime - Buddy_last_player_path_created > F1_0)
			return 1;

	if (ailp->mode == AIM_GOTO_PLAYER)
		return 0;

	if (objP->segnum == gameData.objs.console->segnum)
		return 0;

	if (aip->cur_path_index < aip->path_length/2)
		return 0;
	
	return 1;
}

fix Last_come_back_message_time = 0;

fix Buddy_last_missile_time;

//	-----------------------------------------------------------------------------
void BashBuddyWeaponInfo(int weapon_objnum)
{
	object	*objP = &gameData.objs.objects[weapon_objnum];

	objP->ctype.laser_info.parent_num = OBJ_IDX (gameData.objs.console);
	objP->ctype.laser_info.parent_type = OBJ_PLAYER;
	objP->ctype.laser_info.parent_signature = gameData.objs.console->signature;
}

//	-----------------------------------------------------------------------------
int MaybeBuddyFireMega(short objnum)
{
	object	*objP = gameData.objs.objects + objnum;
	object	*buddyObjP = gameData.objs.objects + gameData.escort.nObjNum;
	fix		dist, dot;
	vms_vector	vec_to_robot;
	int		weapon_objnum;

	VmVecSub(&vec_to_robot, &buddyObjP->pos, &objP->pos);
	dist = VmVecNormalizeQuick(&vec_to_robot);

	if (dist > F1_0*100)
		return 0;

	dot = VmVecDot(&vec_to_robot, &buddyObjP->orient.fvec);

	if (dot < F1_0/2)
		return 0;

	if (!ObjectToObjectVisibility(buddyObjP, objP, FQ_TRANSWALL))
		return 0;

	if (gameData.weapons.info[MEGA_ID].render_type == 0) {
#if TRACE
		con_printf(CON_VERBOSE, "Buddy can't fire mega (shareware)\n");
#endif
		BuddyMessage(TXT_BUDDY_CLICK);
		return 0;
	}
#if TRACE
	con_printf (CON_DEBUG, "Buddy firing mega in frame %i\n", gameData.app.nFrameCount);
#endif
	BuddyMessage(TXT_BUDDY_GAHOOGA);

	weapon_objnum = CreateNewLaserEasy( &buddyObjP->orient.fvec, &buddyObjP->pos, objnum, MEGA_ID, 1);

	if (weapon_objnum != -1)
		BashBuddyWeaponInfo(weapon_objnum);

	return 1;
}

//-----------------------------------------------------------------------------
int MaybeBuddyFireSmart(short objnum)
{
	object	*objP = &gameData.objs.objects[objnum];
	object	*buddyObjP = &gameData.objs.objects[gameData.escort.nObjNum];
	fix		dist;
	short		weapon_objnum;

	dist = VmVecDistQuick(&buddyObjP->pos, &objP->pos);

	if (dist > F1_0*80)
		return 0;

	if (!ObjectToObjectVisibility(buddyObjP, objP, FQ_TRANSWALL))
		return 0;
#if TRACE
	con_printf (CON_DEBUG, "Buddy firing smart missile in frame %i\n", gameData.app.nFrameCount);
#endif
	BuddyMessage(TXT_BUDDY_WHAMMO);

	weapon_objnum = CreateNewLaserEasy( &buddyObjP->orient.fvec, &buddyObjP->pos, objnum, SMART_ID, 1);

	if (weapon_objnum != -1)
		BashBuddyWeaponInfo(weapon_objnum);

	return 1;
}

//	-----------------------------------------------------------------------------
void DoBuddyDudeStuff(void)
{
	short	i;

	if (!BuddyMayTalk())
		return;

	if (Buddy_last_missile_time > gameData.app.xGameTime)
		Buddy_last_missile_time = 0;

	if (Buddy_last_missile_time + F1_0*2 < gameData.app.xGameTime) {
		//	See if a robot potentially in view cone
		for (i=0; i<=gameData.objs.nLastObject; i++)
			if ((gameData.objs.objects[i].type == OBJ_ROBOT) && !gameData.bots.pInfo[gameData.objs.objects[i].id].companion)
				if (MaybeBuddyFireMega(i)) {
					Buddy_last_missile_time = gameData.app.xGameTime;
					return;
				}

		//	See if a robot near enough that buddy should fire smart missile
		for (i=0; i<=gameData.objs.nLastObject; i++)
			if ((gameData.objs.objects[i].type == OBJ_ROBOT) && !gameData.bots.pInfo[gameData.objs.objects[i].id].companion)
				if (MaybeBuddyFireSmart(i)) {
					Buddy_last_missile_time = gameData.app.xGameTime;
					return;
				}

	}
}

//	-----------------------------------------------------------------------------
//	Called every frame (or something).
void DoEscortFrame(object *objP, fix dist_to_player, int player_visibility)
{
	int			objnum = OBJ_IDX (objP);
	ai_static	*aip = &objP->ctype.ai_info;
	ai_local		*ailp = &gameData.ai.localInfo[objnum];

	gameData.escort.nObjNum = OBJ_IDX (objP);

	if (player_visibility) {
		Buddy_last_seen_player = gameData.app.xGameTime;
		if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON)	//	DAMN! MK, stupid bug, fixed 12/08/95, changed PLAYER_FLAGS_HEADLIGHT to PLAYER_FLAGS_HEADLIGHT_ON
			if (f2i(gameData.multi.players[gameData.multi.nLocalPlayer].energy) < 40)
				if ((f2i(gameData.multi.players[gameData.multi.nLocalPlayer].energy)/2) & 2)
					if (!gameStates.app.bPlayerIsDead)
						BuddyMessage(TXT_HEADLIGHT_WARN);

	}

	if (gameStates.app.cheats.bMadBuddy)
		DoBuddyDudeStuff();

	if (gameData.escort.xSorryTime + F1_0 > gameData.app.xGameTime) {
		gameData.escort.xLastMsgTime = 0;	//	Force this message to get through.
		if (gameData.escort.xSorryTime < gameData.app.xGameTime + F1_0*2)
			BuddyMessage(TXT_BUDDY_SORRY);
		gameData.escort.xSorryTime = -F1_0*2;
	}

	//	If buddy not allowed to talk, then he is locked in his room.  Make him mostly do nothing unless you're nearby.
	if (!gameData.escort.bMayTalk)
		if (dist_to_player > F1_0*100)
			aip->SKIP_AI_COUNT = (F1_0/4)/gameData.app.xFrameTime;

	//	AIM_WANDER has been co-opted for buddy behavior (didn't want to modify aistruct.h)
	//	It means the object has been told to get lost and has come to the end of its path.
	//	If the player is now visible, then create a path.
	if (ailp->mode == AIM_WANDER)
		if (player_visibility) {
			create_n_segment_path(objP, 16 + d_rand() * 16, -1);
			aip->path_length = polish_path(objP, &gameData.ai.pointSegs[aip->hide_index], aip->path_length);
		}

	if (gameData.escort.nSpecialGoal == ESCORT_GOAL_SCRAM) {
		if (player_visibility)
			if (gameData.escort.xLastPathCreated + F1_0*3 < gameData.app.xGameTime) {
#if TRACE
				con_printf (CON_DEBUG, "Frame %i: Buddy creating new scram path.\n", gameData.app.nFrameCount);
#endif
				create_n_segment_path(objP, 10 + d_rand() * 16, gameData.objs.console->segnum);
				gameData.escort.xLastPathCreated = gameData.app.xGameTime;
			}

		// -- Int3();
		return;
	}

	//	Force checking for new goal every 5 seconds, and create new path, if necessary.
	if (((gameData.escort.nSpecialGoal != ESCORT_GOAL_SCRAM) && ((gameData.escort.xLastPathCreated + F1_0*5) < gameData.app.xGameTime)) ||
		((gameData.escort.nSpecialGoal == ESCORT_GOAL_SCRAM) && ((gameData.escort.xLastPathCreated + F1_0*15) < gameData.app.xGameTime))) {
		gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
		gameData.escort.xLastPathCreated = gameData.app.xGameTime;
	}

	if ((gameData.escort.nSpecialGoal != ESCORT_GOAL_SCRAM) && TimeToVisitPlayer(objP, ailp, aip)) {
		int	max_len;

		Buddy_last_player_path_created = gameData.app.xGameTime;
		ailp->mode = AIM_GOTO_PLAYER;
		if (!player_visibility) {
			if ((Last_come_back_message_time + F1_0 < gameData.app.xGameTime) || (Last_come_back_message_time > gameData.app.xGameTime)) {
				BuddyMessage(TXT_COMING_BACK);
				Last_come_back_message_time = gameData.app.xGameTime;
			}
		}
		//	No point in Buddy creating very long path if he's not allowed to talk.  Really kills framerate.
		max_len = gameData.escort.nMaxLength;
		if (!gameData.escort.bMayTalk)
			max_len = 3;
		create_path_to_player(objP, max_len, 1);	//	MK!: Last parm used to be 1!
		aip->path_length = polish_path(objP, &gameData.ai.pointSegs[aip->hide_index], aip->path_length);
		ailp->mode = AIM_GOTO_PLAYER;
	}	else if (gameData.app.xGameTime - Buddy_last_seen_player > MAX_ESCORT_TIME_AWAY) {
		//	This is to prevent buddy from looking for a goal, which he will do because we only allow path creation once/second.
		return;
	} else if ((ailp->mode == AIM_GOTO_PLAYER) && (dist_to_player < MIN_ESCORT_DISTANCE)) {
		gameData.escort.nGoalObject = EscortSetGoalObject();
		ailp->mode = AIM_GOTO_OBJECT;		//	May look stupid to be before path creation, but AIDoorIsOpenable uses mode to determine what doors can be got through
		EscortCreatePathToGoal(objP);
		aip->path_length = polish_path(objP, gameData.ai.pointSegs + aip->hide_index, aip->path_length);
		if (aip->path_length < 3) {
			create_n_segment_path (objP, 5, gameData.ai.nBelievedPlayerSeg);
		}
		ailp->mode = AIM_GOTO_OBJECT;
	} else if (gameData.escort.nGoalObject == ESCORT_GOAL_UNSPECIFIED) {
		if ((ailp->mode != AIM_GOTO_PLAYER) || (dist_to_player < MIN_ESCORT_DISTANCE)) {
			gameData.escort.nGoalObject = EscortSetGoalObject();
			ailp->mode = AIM_GOTO_OBJECT;		//	May look stupid to be before path creation, but AIDoorIsOpenable uses mode to determine what doors can be got through
			EscortCreatePathToGoal(objP);
			aip->path_length = polish_path(objP, &gameData.ai.pointSegs[aip->hide_index], aip->path_length);
			if (aip->path_length < 3) {
				create_n_segment_path(objP, 5, gameData.ai.nBelievedPlayerSeg);
			}
			ailp->mode = AIM_GOTO_OBJECT;
		}
	} else
		;

}

//	-------------------------------------------------------------------------------------------------

void InvalidateEscortGoal(void)
{
	gameData.escort.nGoalObject = -1;
}

// --------------------------------------------------------------------------------------------------------------

int ShowEscortHelp (char *goal_str, char *tstr)
{

	int				nItems;
	newmenu_item	m [12];
	char szGoal		[40], szMsgs [40];

	static char *szEscortHelp [12] = {
		"0.  Next Goal: %s",
		"1.  Find Energy Powerup",
		"2.  Find Energy Center",
		"3.  Find Shield Powerup",
		"4.  Find Any Powerup",
		"5.  Find a Robot",
		"6.  Find a Hostage",
		"7.  Stay Away From Me",
		"8.  Find My Powerups",
		"9.  Find the exit",
		"",
		"T.  %s Messages"
		};

sprintf (szGoal, TXT_GOAL_NEXT, goal_str);
sprintf (szMsgs, TXT_GOAL_MESSAGES, tstr);
memset (m, 0, sizeof (m));
for (nItems = 1; nItems < 10; nItems++) {
	m [nItems].text = GT (343 + nItems);
	m [nItems].type = *m [nItems].text ? NM_TYPE_MENU : NM_TYPE_TEXT;
	m [nItems].key = -1;
	}
m [0].text = szGoal;
m [10].text = szMsgs;
m [10].key = KEY_T;
return ExecMenutiny2 (NULL, "Guide-Bot Commands", 11, m, NULL);
}

// --------------------------------------------------------------------------------------------------------------

void DoEscortMenu(void)
{
	int	i;
	int	paused;
	int	next_goal;
	char	goal_str[32], tstr[32];

	if (gameData.app.nGameMode & GM_MULTI) {
		HUDInitMessage(TXT_GB_MULTIPLAYER);
		return;
	}

	for (i=0; i<=gameData.objs.nLastObject; i++) {
		if (gameData.objs.objects[i].type == OBJ_ROBOT)
			if (gameData.bots.pInfo[gameData.objs.objects[i].id].companion)
				break;
	}

	if (i > gameData.objs.nLastObject) {

#if 1//def _DEBUG - always allow buddy bot creation
		//	If no buddy bot, create one!
		HUDInitMessage (TXT_GB_CREATE);
		CreateBuddyBot();
#else
		HUDInitMessage(TXT_GB_NONE);
		return;
#endif
	}

	BuddyMayTalk();	//	Needed here or we might not know buddy can talk when he can.

	if (!gameData.escort.bMayTalk) {
		HUDInitMessage(TXT_GB_RELEASE, gameData.escort.szName);
		return;
	}

	DigiPauseDigiSounds();
	if (!gameOpts->menus.nStyle)
		StopTime();

	PaletteSave();
	apply_modified_palette();
	ResetPaletteAdd();

	GameFlushInputs();

	paused = 1;

//	SetScreenMode( SCREEN_MENU );
	SetPopupScreenMode();
	GrPaletteStepLoad( NULL );

	//	This prevents the buddy from coming back if you've told him to scram.
	//	If we don't set next_goal, we get garbage there.
	if (gameData.escort.nSpecialGoal == ESCORT_GOAL_SCRAM) {
		gameData.escort.nSpecialGoal = -1;	//	Else setting next goal might fail.
		next_goal = EscortSetGoalObject();
		gameData.escort.nSpecialGoal = ESCORT_GOAL_SCRAM;
	} else {
		gameData.escort.nSpecialGoal = -1;	//	Else setting next goal might fail.
		next_goal = EscortSetGoalObject();
	}

	switch (next_goal) {
	#ifndef NDEBUG
		case ESCORT_GOAL_UNSPECIFIED:
			Int3();
			sprintf(goal_str, "ERROR");
			break;
	#endif
			
		case ESCORT_GOAL_BLUE_KEY:
			sprintf(goal_str, TXT_GB_BLUEKEY);
			break;
		case ESCORT_GOAL_GOLD_KEY:
			sprintf(goal_str, TXT_GB_YELLOWKEY);
			break;
		case ESCORT_GOAL_RED_KEY:
			sprintf(goal_str, TXT_GB_REDKEY);
			break;
		case ESCORT_GOAL_CONTROLCEN:
			sprintf(goal_str, TXT_GB_REACTOR);
			break;
		case ESCORT_GOAL_BOSS:
			sprintf(goal_str, TXT_GB_BOSS);
			break;
		case ESCORT_GOAL_EXIT:
			sprintf(goal_str, TXT_GB_EXIT);
			break;
		case ESCORT_GOAL_MARKER1:
		case ESCORT_GOAL_MARKER2:
		case ESCORT_GOAL_MARKER3:
		case ESCORT_GOAL_MARKER4:
		case ESCORT_GOAL_MARKER5:
		case ESCORT_GOAL_MARKER6:
		case ESCORT_GOAL_MARKER7:
		case ESCORT_GOAL_MARKER8:
		case ESCORT_GOAL_MARKER9:
			sprintf(goal_str, TXT_GB_MARKER, next_goal-ESCORT_GOAL_MARKER1+1);
			break;

	}
			
	if (!gameData.escort.bMsgsSuppressed)
		sprintf(tstr, TXT_GB_SUPPRESS);
	else
		sprintf(tstr, TXT_GB_ENABLE);

	i = ShowEscortHelp (goal_str, tstr);
	if (i < 11) {
		gameData.escort.bSearchingMarker = -1;
		gameData.escort.nLastKey = -1;
		EscortSetSpecialGoal (i ? KEY_1 + i - 1 : KEY_0);
		gameData.escort.nLastKey = -1;
		paused = 0;
		}
	else if (i == 11) {
		BuddyMessage (gameData.escort.bMsgsSuppressed ? TXT_GB_MSGS_ON : TXT_GB_MSGS_OFF);
		gameData.escort.bMsgsSuppressed = !gameData.escort.bMsgsSuppressed;
		paused = 0;
		}
	GameFlushInputs();
	PaletteRestore();
	if (!gameOpts->menus.nStyle)
		StartTime();
	DigiResumeDigiSounds();
}

//	-------------------------------------------------------------------------------
//	Show the Buddy menu!
#if 0 //obsolete in d2x-xl
void ShowEscortMenu(char *msg)
{	
	int	w,h,aw;
	int	x,y;
	bkg	bg;

	memset (&bg, 0, sizeof (bg));
	bg.bIgnoreBg = 1;

	WINDOS(
		DDGrSetCurrentCanvas(&dd_VR_screen_pages[0]),
		GrSetCurrentCanvas(&VR_screen_pages[0])
	);

	GrSetCurFont( GAME_FONT );
	GrGetStringSize(msg,&w,&h,&aw);
	x = (grdCurScreen->sc_w-w)/2;
	y = (grdCurScreen->sc_h-h)/4;
	GrSetFontColorRGBi (RGBA (0, PAL2RGBA (28), 0, 255), 1, 0, 0);
   PA_DFX (pa_set_frontbuffer_current());
	PA_DFX (NMDrawBackground(x-15,y-15,x+w+15-1,y+h+15-1));
   PA_DFX (pa_set_backbuffer_current());
   NMDrawBackground(NULL,x-15,y-15,x+w+15-1,y+h+15-1);
WIN(DDGRLOCK(dd_grd_curcanv));
	PA_DFX (pa_set_frontbuffer_current());
  	PA_DFX (GrUString( x, y, msg ));
	PA_DFX (pa_set_backbuffer_current());
  	GrUString( x, y, msg );
WIN(DDGRUNLOCK(dd_grd_curcanv));
	ResetCockpit();
}
#endif
