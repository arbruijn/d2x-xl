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

#include <stdio.h>		// for printf ()
#include <stdlib.h>		// for rand () and qsort ()
#include <string.h>		// for memset ()

#include "descent.h"
#include "error.h"
#include "key.h"
#include "screens.h"
#include "text.h"
#include "menu.h"
#include "escort.h"
#include "playerprofile.h"
#include "network.h"
#include "findpath.h"
#include "segmath.h"
#include "headlight.h"
#include "visibility.h"
#include "marker.h"

void SayEscortGoal (int32_t nGoal);
void ShowEscortMenu (char *msg);

int32_t nEscortGoalText [MAX_ESCORT_GOALS] = {
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

void InitBuddyForLevel (void)
{
	CObject*	objP;

gameData.escort.bMayTalk = 0;
gameData.escort.nObjNum = -1;
gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
gameData.escort.nSpecialGoal = -1;
gameData.escort.nGoalIndex = -1;
gameData.escort.bMsgsSuppressed = 0;
FORALL_ROBOT_OBJS (objP)
	if (IS_GUIDEBOT (objP))
		break;
if (IS_OBJECT (objP, objP->Index ()))
	gameData.escort.nObjNum = objP->Index ();
gameData.escort.xSorryTime = -I2X (1);
gameData.escort.bSearchingMarker = -1;
gameData.escort.nLastKey = -1;
}

//	-----------------------------------------------------------------------------
//	See if CSegment from curseg through nSide is reachable.
//	Return true if it is reachable, else return false.
int32_t SegmentIsReachable (int32_t curseg, int16_t nSide)
{
return AIDoorIsOpenable (NULL, SEGMENTS + curseg, nSide);
}


//	-----------------------------------------------------------------------------
//	Create a breadth-first list of segments reachable from current CSegment.
//	nMaxSegs is maximum number of segments to search.  Use LEVEL_SEGMENTS to search all.
//	On exit, *length <= nMaxSegs.
//	Input:
//		nStartSeg
//	Output:
//		bfsList:	array of shorts, each reachable CSegment.  Includes start CSegment.
//		length:		number of elements in bfsList
void CreateBfsList (int32_t nStartSeg, int16_t segList [], int32_t *length, int32_t nMaxSegs)
{
	int32_t		head = 0, tail = 0;
	int8_t		bVisited [MAX_SEGMENTS_D2X];

memset (bVisited, 0, LEVEL_SEGMENTS * sizeof (int8_t));
segList [head++] = nStartSeg;
bVisited [nStartSeg] = 1;
if (nMaxSegs > LEVEL_SEGMENTS)
	nMaxSegs = LEVEL_SEGMENTS;

while ((head != tail) && (head < nMaxSegs)) {
	int16_t nSegment = segList [tail++];
	CSegment	*segP = SEGMENTS + nSegment;

	for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
		int32_t nConnSeg = segP->m_children [i];
		if (!IS_CHILD (nConnSeg))
			continue;
		if (bVisited [nConnSeg])
			continue;
		if (!SegmentIsReachable (nSegment, (int16_t) i))
			continue;
		segList [head++] = nConnSeg;
		if (head >= nMaxSegs)
			break;
		bVisited [nConnSeg] = 1;
		//Assert (head < LEVEL_SEGMENTS);
		}
	}
*length = head;
}

//	-----------------------------------------------------------------------------
//	Return true if ok for buddy to talk, else return false.
//	Buddy is allowed to talk if the CSegment he is in does not contain a blastable CWall that has not been blasted
//	AND he has never yet, since being initialized for level, been allowed to talk.
int32_t BuddyMayTalk (void)
{
	int32_t		i;
	CSegment	*segP;
	CObject	*objP;

if ((gameData.escort.nObjNum < 0) || (OBJECTS [gameData.escort.nObjNum].info.nType != OBJ_ROBOT)) {
	gameData.escort.bMayTalk = 0;
	return 0;
	}
if (gameData.escort.bMayTalk)
	return 1;
if ((OBJECTS [gameData.escort.nObjNum].info.nType == OBJ_ROBOT) &&
	 (gameData.escort.nObjNum <= gameData.objs.nLastObject [0]) &&
	!ROBOTINFO (OBJECTS [gameData.escort.nObjNum].info.nId).companion) {
	FORALL_ROBOT_OBJS (objP)
		if (IS_GUIDEBOT (objP))
			break;
	if (!IS_OBJECT (objP, objP->Index ()))
		return 0;
	gameData.escort.nObjNum = objP->Index ();
	}
segP = SEGMENTS + OBJECTS [gameData.escort.nObjNum].info.nSegment;
for (i = 0; i < SEGMENT_SIDE_COUNT; i++) {
	CWall* wallP = segP->Wall (i);
	if (wallP && (wallP->nType == WALL_BLASTABLE) && !(wallP->flags & WALL_BLASTED))
		return 0;
	//	Check one level deeper.
	if (IS_CHILD (segP->m_children [i])) {
		int32_t		j;
		CSegment	*connSegP = SEGMENTS + segP->m_children [i];

		for (j = 0; j < SEGMENT_SIDE_COUNT; j++) {
			CWall* otherWallP = connSegP->Wall (j);
			if (otherWallP && (otherWallP->nType == WALL_BLASTABLE) && !(otherWallP->flags & WALL_BLASTED))
				return 0;
			}
		}
	}
gameData.escort.bMayTalk = 1;
return 1;
}

//	--------------------------------------------------------------------------------------------

void DetectEscortGoalAccomplished (int32_t index)
{
	int32_t		i, j;
	int32_t		bDetected = 0;
	CObject	*objP;
	int16_t*	childI, * childJ;

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

if ((gameData.escort.nGoalIndex <= ESCORT_GOAL_RED_KEY) && (index >= 0) && (index <= gameData.objs.nLastObject [0])) {
	objP = OBJECTS + index;
	if (objP->info.nType == OBJ_POWERUP)  {
		uint8_t id = objP->info.nId;
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
if (gameData.escort.nSpecialGoal != -1) {
	if (gameData.escort.nSpecialGoal == ESCORT_GOAL_ENERGYCEN) {
		if (index == -4)
			bDetected = 1;
		else if ((index >= 0) && (index <= gameData.segs.nLastSegment) &&
					(gameData.escort.nGoalIndex >= 0) && (gameData.escort.nGoalIndex <= gameData.segs.nLastSegment)) {
			childI = SEGMENTS [index].m_children;
			for (i = SEGMENT_SIDE_COUNT; i; i--, childI++) {
				if (*childI == gameData.escort.nGoalIndex) {
					bDetected = 1;
					goto dega_ok;
					}
				else {
					childJ = SEGMENTS [*childI].m_children;
					for (j = SEGMENT_SIDE_COUNT; j; j--, childJ++) {
						if (*childJ == gameData.escort.nGoalIndex) {
							bDetected = 1;
							goto dega_ok;
							}
						}
					}
				}
			}
		}
	else if ((index >= 0) && (index <= gameData.objs.nLastObject [0])) {
		objP = OBJECTS + index;
		if ((objP->info.nType == OBJ_POWERUP) && (gameData.escort.nSpecialGoal == ESCORT_GOAL_POWERUP))
			bDetected = 1;	//	Any nType of powerup picked up will do.
		else if ((gameData.escort.nGoalIndex >= 0) && (gameData.escort.nGoalIndex <= gameData.objs.nLastObject [0]) &&
					(objP->info.nType == OBJECTS [gameData.escort.nGoalIndex].info.nType) &&
					(objP->info.nId == OBJECTS [gameData.escort.nGoalIndex].info.nId)) {
		//	Note: This will help a little bit in making the buddy believe a goal is satisfied.  Won't work for a general goal like "find any powerup"
		// because of the insistence of both nType and id matching.
			bDetected = 1;
			}
		}
	}

dega_ok: ;
if (bDetected) {
	if (BuddyMayTalk ())
		audio.PlaySound (SOUND_BUDDY_MET_GOAL);
	gameData.escort.nGoalIndex = -1;
	gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escort.nSpecialGoal = -1;
	gameData.escort.bSearchingMarker = -1;
	}
}

//	-------------------------------------------------------------------------------------------------

void ChangeGuidebotName (void)
{
	CMenu	m;
	char	text [GUIDEBOT_NAME_LEN+1] = "";
	int32_t	item;

strcpy (text,gameData.escort.szName);
m.AddInput ("guidebot name", text, GUIDEBOT_NAME_LEN);
item = m.Menu (NULL, "Enter Guide-bot name:");
if (item != -1) {
	strcpy (gameData.escort.szName, text);
	strcpy (gameData.escort.szRealName, text);
	SavePlayerProfile ();
	}
}

//	-----------------------------------------------------------------------------

void BuddyOuchMessage (fix damage)
{
	char	szOuch [6 * 4 + 2];

int32_t count = X2I (damage / 8);
if (count > 4)
	count = 4;
else if (count <= 0)
	count = 1;
szOuch [0] = 0;
for (int32_t i = 0; i < count; i++) {
	strcat (szOuch, TXT_BUDDY_OUCH);
	strcat (szOuch, " ");
	}
BuddyMessage (szOuch);
}

//	-----------------------------------------------------------------------------

void _CDECL_ BuddyMessage (const char * format, ... )
{
if (gameData.escort.bMsgsSuppressed)
	return;
if (IsMultiGame)
	return;
if ((gameData.escort.xLastMsgTime + I2X (1) < gameData.time.xGame) ||
	 (gameData.escort.xLastMsgTime > gameData.time.xGame)) {
	if (BuddyMayTalk ()) {
		char		szMsg [200];
		va_list	args;
		int32_t		l = (int32_t) strlen (gameData.escort.szName);

		va_start (args, format);
		vsprintf (szMsg + l + 10, format, args);
		va_end (args);

		szMsg [0] = (char) 1;
		szMsg [1] = (char) (127 + 128);
		szMsg [2] = (char) (127 + 128);
		szMsg [3] = (char) (0 + 128);
		memcpy (szMsg + 4, gameData.escort.szName, l);
		szMsg [l+4] = ':';
		szMsg [l+5] = ' ';
		szMsg [l+6] = (char) 1;
		szMsg [l+7] = (char) (0 + 128);
		szMsg [l+8] = (char) (127 + 128);
		szMsg [l+9] = (char) (0 + 128);

		HUDInitMessage (szMsg);
		gameData.escort.xLastMsgTime = gameData.time.xGame;
		}
	}
}

//	-----------------------------------------------------------------------------
//	Return true if marker #id has been placed.
int32_t MarkerExistsInMine (int32_t id)
{
	CObject*	objP;

FORALL_OBJS (objP)
	if ((objP->info.nType == OBJ_MARKER) && (objP->info.nId == id))
		return 1;
return 0;
}

//	-----------------------------------------------------------------------------

void EscortSetSpecialGoal (int32_t specialKey)
{
	int32_t markerKey;

gameData.escort.bMsgsSuppressed = 0;
if (!gameData.escort.bMayTalk) {
	BuddyMayTalk ();
	if (!gameData.escort.bMayTalk) {
		CObject*	objP;

		FORALL_ROBOT_OBJS (objP)
			if (IS_GUIDEBOT (objP)) {
				HUDInitMessage (TXT_GB_RELEASE, gameData.escort.szName);
				return;
				}
		HUDInitMessage (TXT_GB_NONE);
		return;
		}
	}

specialKey = specialKey & (~KEY_SHIFTED);
markerKey = specialKey;

if (gameData.escort.nLastKey == specialKey) {
	if ((gameData.escort.bSearchingMarker == -1) && (specialKey != KEY_0)) {
		if (MarkerExistsInMine (markerKey - KEY_1))
			gameData.escort.bSearchingMarker = markerKey - KEY_1;
		else {
			gameData.escort.xLastMsgTime = 0;	//	Force this message to get through.
			BuddyMessage ("Marker %i not placed.", markerKey - KEY_1 + 1);
			gameData.escort.bSearchingMarker = -1;
			}
		} 
	else {
		gameData.escort.bSearchingMarker = -1;
		}
	}
gameData.escort.nLastKey = specialKey;
if (specialKey == KEY_0)
	gameData.escort.bSearchingMarker = -1;
	if (gameData.escort.bSearchingMarker != -1)
		gameData.escort.nSpecialGoal = ESCORT_GOAL_MARKER1 + markerKey - KEY_1;
else {
	switch (specialKey) {
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
			Int3 ();		//	Oops, called with illegal key value.
		}
	}
gameData.escort.xLastMsgTime = gameData.time.xGame - I2X (2);	//	Allow next message to come through.
SayEscortGoal (gameData.escort.nSpecialGoal);
gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
}

//	-----------------------------------------------------------------------------
//	Return id of boss.
int32_t GetBossId (void)
{
	int32_t	h, i;
	int32_t	nObject;

	for (h = gameData.bosses.ToS (), i = 0; i < h; i++) {
		if (0 <= (nObject = gameData.bosses [i].m_nObject))
			return OBJECTS [nObject].info.nId;
		}
	return -1;
}

//	-----------------------------------------------------------------------------
//	Return bject index if CObject of objType, objId exists in mine, else return -1
//	"special" is used to find OBJECTS spewed by player which is hacked into flags field of powerup.
int32_t ExistsInMine2 (int32_t nSegment, int32_t objType, int32_t objId, int32_t special)
{
if ((objType == OBJ_POWERUP) && (gameStates.app.bGameSuspended & SUSP_POWERUPS))
	return -1;


	int32_t nObject = SEGMENTS [nSegment].m_objects;
	
if (nObject != -1) {
	while (nObject != -1) {
		CObject	*curObjP = OBJECTS + nObject;

		if ((special == ESCORT_GOAL_PLAYER_SPEW) && (curObjP->info.nFlags & OF_PLAYER_DROPPED))
			return nObject;
		if (curObjP->info.nType == objType) {
			//	Don't find escort robots if looking for robot!
			if (IS_GUIDEBOT (curObjP))
				;
			else if (objId == -1)
				return nObject;
			else if (curObjP->info.nId == objId)
				return nObject;
			}
		if (objType == OBJ_POWERUP) {
			if (curObjP->info.contains.nCount && (curObjP->info.contains.nType == OBJ_POWERUP) && (curObjP->info.contains.nId == objId))
				return nObject;
			}
		nObject = curObjP->info.nNextInSeg;
		}
	}
return -1;
}

//	-----------------------------------------------------------------------------
//	Return nearest object of interest.
//	If special == ESCORT_GOAL_PLAYER_SPEW, then looking for any CObject spewed by player.
//	-1 means CObject does not exist in mine.
//	-2 means CObject does exist in mine, but buddy-bot can't reach it (eg, behind triggered CWall)
int32_t ExistsInMine (int32_t nStartSeg, int32_t objType, int32_t objId, int32_t special)
{
	int32_t	nSegIdx, nSegment;
	int16_t	bfsList [MAX_SEGMENTS_D2X];
	int32_t	length;
	int32_t	nObject;

CreateBfsList (nStartSeg, bfsList, &length, LEVEL_SEGMENTS);
if (objType == PRODUCER_CHECK) {
	for (nSegIdx = 0; nSegIdx < length; nSegIdx++) {
		nSegment = bfsList [nSegIdx];
		if (SEGMENTS [nSegment].m_function == SEGMENT_FUNC_FUELCENTER)
			return nSegment;
		}
	}
else {
	for (nSegIdx = 0; nSegIdx < length; nSegIdx++) {
		nSegment = bfsList [nSegIdx];
		nObject = ExistsInMine2 (nSegment, objType, objId, special);
		if (nObject != -1)
			return nObject;
		}
	}
//	Couldn't find what we're looking for by looking at connectivity.
//	See if it's in the mine.  It could be hidden behind a trigger or switch
//	which the buddybot doesn't understand.
if (objType == PRODUCER_CHECK) {
	for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++)
		if (SEGMENTS [nSegment].m_function == SEGMENT_FUNC_FUELCENTER)
			return -2;
	}
else {
	for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++) {
		nObject = ExistsInMine2 (nSegment, objType, objId, special);
		if (nObject != -1)
			return -2;
		}
	}
return -1;
}

//	-----------------------------------------------------------------------------
//	Return true if it happened, else return false.
int32_t FindExitSegment (void)
{
	CSegment* segP = SEGMENTS.Buffer ();

for (int32_t i = 0; i <= gameData.segs.nSegments; i++, segP++)
	for (int32_t j = 0; j < SEGMENT_SIDE_COUNT; j++)
		if (segP->m_children [j] == -2) {
			return i;
		}
return -1;
}

#define	BUDDY_MARKER_TEXT_LEN	25

//	-----------------------------------------------------------------------------

void SayEscortGoal (int32_t nGoal)
{
if (gameStates.app.bPlayerIsDead)
	return;
switch (nGoal) {
	case ESCORT_GOAL_BLUE_KEY:
		BuddyMessage (TXT_FIND_BLUEKEY);
		break;
	case ESCORT_GOAL_GOLD_KEY:
		BuddyMessage (TXT_FIND_GOLDKEY);
		break;
	case ESCORT_GOAL_RED_KEY:
		BuddyMessage (TXT_FIND_REDKEY);
		break;
	case ESCORT_GOAL_CONTROLCEN:
		BuddyMessage (TXT_FIND_REACTOR);
		break;
	case ESCORT_GOAL_EXIT:
		BuddyMessage (TXT_FIND_EXIT);
		break;
	case ESCORT_GOAL_ENERGY:
		BuddyMessage (TXT_FIND_ENERGY);
		break;
	case ESCORT_GOAL_ENERGYCEN:
		BuddyMessage (TXT_FIND_ECENTER);
		break;
	case ESCORT_GOAL_SHIELD:
		BuddyMessage (TXT_FIND_SHIELD);
		break;
	case ESCORT_GOAL_POWERUP:
		BuddyMessage (TXT_FIND_POWERUP);
		break;
	case ESCORT_GOAL_ROBOT:
		BuddyMessage (TXT_FIND_ROBOT);
		break;
	case ESCORT_GOAL_HOSTAGE:
		BuddyMessage (TXT_FIND_HOSTAGE);
		break;
	case ESCORT_GOAL_SCRAM:
		BuddyMessage (TXT_STAYING_AWAY);
		break;
	case ESCORT_GOAL_BOSS:
		BuddyMessage (TXT_FIND_BOSS);
		break;
	case ESCORT_GOAL_PLAYER_SPEW:
		BuddyMessage (TXT_FIND_YOURSTUFF);
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
			char szMarkerMsg [BUDDY_MARKER_TEXT_LEN];

		strncpy (szMarkerMsg, markerManager.Message (nGoal - ESCORT_GOAL_MARKER1), BUDDY_MARKER_TEXT_LEN - 1);
		szMarkerMsg [BUDDY_MARKER_TEXT_LEN - 1] = 0;
		BuddyMessage (TXT_FIND_MARKER, nGoal - ESCORT_GOAL_MARKER1 + 1, szMarkerMsg);
		break;
		}
	}
}

//	-----------------------------------------------------------------------------

#if 0
typedef struct tEscortGoal {
	int16_t	nType, nId, special;
} tEscortGoal;

static tEscortGoal escortGoals [MAX_ESCORT_GOALS] = {
	{0, 0, -1},
	{OBJ_POWERUP, POW_KEY_BLUE, -1},
	{OBJ_POWERUP, POW_KEY_GOD, -1},
	{OBJ_POWERUP, POW_KEY_RED, -1},
	{OBJ_REACTOR, -1, -1},
	{-1, -1, -1},
	{OBJ_POWERUP, POW_ENERGY, -1},
	{-1, -1, -1},
	{PRODUCER_CHECK, -1, -1},
	{OBJ_POWERUP, POW_SHIELD_BOOST, -1},
	{OBJ_POWERUP, -1, -1},
	{OBJ_ROBOT, -1, -1},
	{OBJ_HOSTAGE, -1, -1},
	{-1, -1, ESCORT_GOAL_PLAYER_SPEW},
	{OBJ_POWERUP, POW_, -1},
	{OBJ_POWERUP, POW_, -1},
	{OBJ_POWERUP, POW_, -1},
	{OBJ_POWERUP, POW_, -1},
	{OBJ_POWERUP, POW_, -1},
	{OBJ_POWERUP, POW_, -1},
	{OBJ_POWERUP, POW_, -1},
	{OBJ_POWERUP, POW_, -1},
	{OBJ_POWERUP, POW_, -1},
#endif

void EscortCreatePathToGoal (CObject *objP)
{
	int16_t				nGoalSeg = -1;
	int16_t				nObject = objP->Index ();
	tAIStaticInfo*	aip = &objP->cType.aiInfo;
	tAILocalInfo*	ailp = gameData.ai.localInfo + nObject;

Assert (nObject >= 0);
if (gameData.escort.nSpecialGoal != -1)
	gameData.escort.nGoalObject = gameData.escort.nSpecialGoal;
gameData.escort.nKillObject = -1;
if (gameData.escort.bSearchingMarker != -1) {
	gameData.escort.nGoalIndex = ExistsInMine (objP->info.nSegment, OBJ_MARKER, gameData.escort.nGoalObject - ESCORT_GOAL_MARKER1, -1);
	if (gameData.escort.nGoalIndex > -1)
		nGoalSeg = OBJECTS [gameData.escort.nGoalIndex].info.nSegment;
	}
else {
	switch (gameData.escort.nGoalObject) {
		case ESCORT_GOAL_BLUE_KEY:
			gameData.escort.nGoalIndex = ExistsInMine (objP->info.nSegment, OBJ_POWERUP, POW_KEY_BLUE, -1);
			if (gameData.escort.nGoalIndex > -1)
				nGoalSeg = OBJECTS [gameData.escort.nGoalIndex].info.nSegment;
			break;
		case ESCORT_GOAL_GOLD_KEY:
			gameData.escort.nGoalIndex = ExistsInMine (objP->info.nSegment, OBJ_POWERUP, POW_KEY_GOLD, -1);
			if (gameData.escort.nGoalIndex > -1)
				nGoalSeg = OBJECTS [gameData.escort.nGoalIndex].info.nSegment;
			break;
		case ESCORT_GOAL_RED_KEY:
			gameData.escort.nGoalIndex = ExistsInMine (objP->info.nSegment, OBJ_POWERUP, POW_KEY_RED, -1);
			if (gameData.escort.nGoalIndex > -1)
				nGoalSeg = OBJECTS [gameData.escort.nGoalIndex].info.nSegment;
			break;
		case ESCORT_GOAL_CONTROLCEN:
			gameData.escort.nGoalIndex = ExistsInMine (objP->info.nSegment, OBJ_REACTOR, -1, -1);
			if (gameData.escort.nGoalIndex > -1)
				nGoalSeg = OBJECTS [gameData.escort.nGoalIndex].info.nSegment;
			break;
		case ESCORT_GOAL_EXIT:
		case ESCORT_GOAL_EXIT2:
			nGoalSeg = FindExitSegment ();
			gameData.escort.nGoalIndex = nGoalSeg;
			break;
		case ESCORT_GOAL_ENERGY:
			gameData.escort.nGoalIndex = ExistsInMine (objP->info.nSegment, OBJ_POWERUP, POW_ENERGY, -1);
			if (gameData.escort.nGoalIndex > -1)
				nGoalSeg = OBJECTS [gameData.escort.nGoalIndex].info.nSegment;
			break;
		case ESCORT_GOAL_ENERGYCEN:
			nGoalSeg = ExistsInMine (objP->info.nSegment, PRODUCER_CHECK, -1, -1);
			gameData.escort.nGoalIndex = nGoalSeg;
			break;
		case ESCORT_GOAL_SHIELD:
			gameData.escort.nGoalIndex = ExistsInMine (objP->info.nSegment, OBJ_POWERUP, POW_SHIELD_BOOST, -1);
			if (gameData.escort.nGoalIndex > -1)
				nGoalSeg = OBJECTS [gameData.escort.nGoalIndex].info.nSegment;
			break;
		case ESCORT_GOAL_POWERUP:
			gameData.escort.nGoalIndex = ExistsInMine (objP->info.nSegment, OBJ_POWERUP, -1, -1);
			if (gameData.escort.nGoalIndex > -1)
				nGoalSeg = OBJECTS [gameData.escort.nGoalIndex].info.nSegment;
			break;
		case ESCORT_GOAL_ROBOT:
			gameData.escort.nGoalIndex = ExistsInMine (objP->info.nSegment, OBJ_ROBOT, -1, -1);
			if (gameData.escort.nGoalIndex > -1)
				nGoalSeg = OBJECTS [gameData.escort.nGoalIndex].info.nSegment;
			break;
		case ESCORT_GOAL_HOSTAGE:
			gameData.escort.nGoalIndex = ExistsInMine (objP->info.nSegment, OBJ_HOSTAGE, -1, -1);
			if (gameData.escort.nGoalIndex > -1)
				nGoalSeg = OBJECTS [gameData.escort.nGoalIndex].info.nSegment;
			break;
		case ESCORT_GOAL_PLAYER_SPEW:
			gameData.escort.nGoalIndex = ExistsInMine (objP->info.nSegment, -1, -1, ESCORT_GOAL_PLAYER_SPEW);
			if (gameData.escort.nGoalIndex > -1)
				nGoalSeg = OBJECTS [gameData.escort.nGoalIndex].info.nSegment;
			break;
		case ESCORT_GOAL_SCRAM:
			nGoalSeg = -3;		//	Kinda a hack.
			gameData.escort.nGoalIndex = nGoalSeg;
			break;
		case ESCORT_GOAL_BOSS: {
			int32_t	boss_id;

			boss_id = GetBossId ();
			Assert (boss_id != -1);
			gameData.escort.nGoalIndex = ExistsInMine (objP->info.nSegment, OBJ_ROBOT, boss_id, -1);
			if (gameData.escort.nGoalIndex > -1)
				nGoalSeg = OBJECTS [gameData.escort.nGoalIndex].info.nSegment;
			break;
		}
		default:
			Int3 ();	//	Oops, Illegal value in gameData.escort.nGoalObject.
			nGoalSeg = 0;
			break;
		}
	}
if ((gameData.escort.nGoalIndex < 0) && (gameData.escort.nGoalIndex != -3)) {	//	I apologize for this statement -- MK, 09/22/95
	if (gameData.escort.nGoalIndex == -1) {
		gameData.escort.xLastMsgTime = 0;	//	Force this message to get through.
		BuddyMessage (TXT_NOT_IN_MINE, GT (nEscortGoalText [gameData.escort.nGoalObject-1]));
		gameData.escort.bSearchingMarker = -1;
		}
	else if (gameData.escort.nGoalIndex == -2) {
		gameData.escort.xLastMsgTime = 0;	//	Force this message to get through.
		BuddyMessage (TXT_CANT_REACH, GT (nEscortGoalText [gameData.escort.nGoalObject-1]));
		gameData.escort.bSearchingMarker = -1;
		}
	else
		Int3 ();

	gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escort.nSpecialGoal = -1;
	}
else {
	if (nGoalSeg == -3) {
		CreateNSegmentPath (objP, 16 + Rand (16), -1);
		aip->nPathLength = SmoothPath (objP, gameData.ai.routeSegs + aip->nHideIndex, aip->nPathLength);
		}
	else {
		CreatePathToSegment (objP, nGoalSeg, gameData.escort.nMaxLength, 1);	//	MK!: Last parm (safetyFlag) used to be 1!!
		if (aip->nPathLength > 3)
			aip->nPathLength = SmoothPath (objP, gameData.ai.routeSegs + aip->nHideIndex, aip->nPathLength);
		if ((aip->nPathLength > 0) && (gameData.ai.routeSegs [aip->nHideIndex + aip->nPathLength - 1].nSegment != nGoalSeg)) {
			fix	xDistToPlayer;
			gameData.escort.xLastMsgTime = 0;	//	Force this message to get through.
			BuddyMessage (TXT_CANT_REACH, GT (nEscortGoalText [gameData.escort.nGoalObject-1]));
			gameData.escort.bSearchingMarker = -1;
			gameData.escort.nGoalObject = ESCORT_GOAL_SCRAM;
			xDistToPlayer = simpleRouter [0].PathLength (objP->info.position.vPos, objP->info.nSegment, 
																		gameData.ai.target.vBelievedPos, gameData.ai.target.nBelievedSeg, 
																		100, WID_PASSABLE_FLAG, 1);
			if (xDistToPlayer > MIN_ESCORT_DISTANCE)
				CreatePathToTarget (objP, gameData.escort.nMaxLength, 1);	//	MK!: Last parm used to be 1!
			else {
				CreateNSegmentPath (objP, 8 + RandShort () * 8, -1);
				aip->nPathLength = SmoothPath (objP, gameData.ai.routeSegs + aip->nHideIndex, aip->nPathLength);
				}
			}
		}
	ailp->mode = AIM_GOTO_OBJECT;
	SayEscortGoal (gameData.escort.nGoalObject);
	}
}

//	-----------------------------------------------------------------------------
//	Escort robot chooses goal CObject based on player's keys, location.
//	Returns goal CObject.
int32_t EscortSetGoalObject (void)
{
if (gameData.escort.nSpecialGoal != -1)
	return ESCORT_GOAL_UNSPECIFIED;
else if (!(gameData.objs.consoleP->info.nFlags & PLAYER_FLAGS_BLUE_KEY) &&
			 (ExistsInMine (gameData.objs.consoleP->info.nSegment, OBJ_POWERUP, POW_KEY_BLUE, -1) != -1))
	return ESCORT_GOAL_BLUE_KEY;
else if (!(gameData.objs.consoleP->info.nFlags & PLAYER_FLAGS_GOLD_KEY) &&
			 (ExistsInMine (gameData.objs.consoleP->info.nSegment, OBJ_POWERUP, POW_KEY_GOLD, -1) != -1))
	return ESCORT_GOAL_GOLD_KEY;
else if (!(gameData.objs.consoleP->info.nFlags & PLAYER_FLAGS_RED_KEY) &&
			 (ExistsInMine (gameData.objs.consoleP->info.nSegment, OBJ_POWERUP, POW_KEY_RED, -1) != -1))
	return ESCORT_GOAL_RED_KEY;
else if (!gameData.reactor.bDestroyed) {
	for (int32_t i = 0; i < int32_t (gameData.bosses.ToS ()); i++)
		if ((gameData.bosses [i].m_nObject >= 0) && gameData.bosses [i].m_nTeleportSegs)
			return ESCORT_GOAL_BOSS;
		return ESCORT_GOAL_CONTROLCEN;
	}
else
	return ESCORT_GOAL_EXIT;
}

#define	MAX_ESCORT_TIME_AWAY		 (I2X (4))

fix	xBuddyLastSeenPlayer = 0, Buddy_last_player_path_created;

//	-----------------------------------------------------------------------------

int32_t TimeToVisitPlayer (CObject *objP, tAILocalInfo *ailp, tAIStaticInfo *aip)
{
	//	Note: This one has highest priority because, even if already going towards player,
	//	might be necessary to create a new path, as player can move.
if (gameData.time.xGame - xBuddyLastSeenPlayer > MAX_ESCORT_TIME_AWAY)
	if (gameData.time.xGame - Buddy_last_player_path_created > I2X (1))
		return 1;
if (ailp->mode == AIM_GOTO_PLAYER)
	return 0;
if (objP->info.nSegment == gameData.objs.consoleP->info.nSegment)
	return 0;
if (aip->nCurPathIndex < aip->nPathLength/2)
	return 0;
return 1;
}

fix Last_come_back_messageTime = 0;

fix Buddy_last_missileTime;

//	-----------------------------------------------------------------------------

void BashBuddyWeaponInfo (int32_t nWeaponObj)
{
	CObject	*objP = OBJECTS + nWeaponObj;

objP->cType.laserInfo.parent.nObject = OBJ_IDX (gameData.objs.consoleP);
objP->cType.laserInfo.parent.nType = OBJ_PLAYER;
objP->cType.laserInfo.parent.nSignature = gameData.objs.consoleP->info.nSignature;
}

//	-----------------------------------------------------------------------------

int32_t MaybeBuddyFireMega (int16_t nObject)
{
	CObject		*objP = OBJECTS + nObject;
	CObject		*buddyObjP = OBJECTS + gameData.escort.nObjNum;
	fix			dist, dot;
	CFixVector	vVecToRobot;
	int32_t			nWeaponObj;

vVecToRobot = buddyObjP->info.position.vPos - objP->info.position.vPos;
dist = CFixVector::Normalize (vVecToRobot);
if (dist > I2X (100))
	return 0;
dot = CFixVector::Dot (vVecToRobot, buddyObjP->info.position.mOrient.m.dir.f);
if (dot < I2X (1)/2)
	return 0;
if (!ObjectToObjectVisibility (buddyObjP, objP, FQ_TRANSWALL))
	return 0;
if (gameData.weapons.info [MEGAMSL_ID].renderType == 0) {
#if TRACE
	console.printf (CON_VERBOSE, "Buddy can't fire mega (shareware)\n");
#endif
	BuddyMessage (TXT_BUDDY_CLICK);
	return 0;
	}
#if TRACE
console.printf (CON_DBG, "Buddy firing mega in frame %i\n", gameData.app.nFrameCount);
#endif
BuddyMessage (TXT_BUDDY_GAHOOGA);
nWeaponObj = CreateNewWeaponSimple (&buddyObjP->info.position.mOrient.m.dir.f, &buddyObjP->info.position.vPos, nObject, MEGAMSL_ID, 1);
if (nWeaponObj != -1)
	BashBuddyWeaponInfo (nWeaponObj);
return 1;
}

//-----------------------------------------------------------------------------

int32_t MaybeBuddyFireSmart (int16_t nObject)
{
	CObject*	objP = &OBJECTS [nObject];
	CObject*	buddyObjP = &OBJECTS [gameData.escort.nObjNum];
	fix		dist;
	int16_t	nWeaponObj;

dist = CFixVector::Dist(buddyObjP->info.position.vPos, objP->info.position.vPos);
if (dist > I2X (80))
	return 0;
if (!ObjectToObjectVisibility (buddyObjP, objP, FQ_TRANSWALL))
	return 0;
#if TRACE
console.printf (CON_DBG, "Buddy firing smart missile in frame %i\n", gameData.app.nFrameCount);
#endif
BuddyMessage (TXT_BUDDY_WHAMMO);
nWeaponObj = CreateNewWeaponSimple (&buddyObjP->info.position.mOrient.m.dir.f, &buddyObjP->info.position.vPos, nObject, SMARTMSL_ID, 1);
if (nWeaponObj != -1)
	BashBuddyWeaponInfo (nWeaponObj);
return 1;
}

//	-----------------------------------------------------------------------------

void DoBuddyDudeStuff (void)
{
	CObject*	objP;

if (!BuddyMayTalk ())
	return;

if (Buddy_last_missileTime > gameData.time.xGame)
	Buddy_last_missileTime = 0;

if (Buddy_last_missileTime + I2X (2) < gameData.time.xGame) {
	//	See if a robot potentially in view cone
	FORALL_ROBOT_OBJS (objP)
		if (!ROBOTINFO (objP->info.nId).companion)
			if (MaybeBuddyFireMega (objP->Index ())) {
				Buddy_last_missileTime = gameData.time.xGame;
				return;
			}
	//	See if a robot near enough that buddy should fire smart missile
	FORALL_ROBOT_OBJS (objP)
		if (!ROBOTINFO (objP->info.nId).companion)
			if (MaybeBuddyFireSmart (objP->Index ())) {
				Buddy_last_missileTime = gameData.time.xGame;
				return;
			}
	}
}

//	-----------------------------------------------------------------------------
//	Called every frame (or something).
void DoEscortFrame (CObject *objP, fix xDistToPlayer, int32_t nPlayerVisibility)
{
	int32_t				nObject = objP->Index ();
	tAIStaticInfo*	aip = &objP->cType.aiInfo;
	tAILocalInfo*	ailp = gameData.ai.localInfo + nObject;

Assert (nObject >= 0);
gameData.escort.nObjNum = nObject;
if (nPlayerVisibility) {
	xBuddyLastSeenPlayer = gameData.time.xGame;
	if ((PlayerHasHeadlight (-1) && EGI_FLAG (headlight.bDrainPower, 0, 0, 1))	&&
		 (X2I (LOCALPLAYER.Energy ()) < 40) && ((X2I (LOCALPLAYER.Energy ()) / 2) & 2) && (!gameStates.app.bPlayerIsDead))
		BuddyMessage (TXT_HEADLIGHT_WARN);
	}
if (gameStates.app.cheats.bMadBuddy)
	DoBuddyDudeStuff ();
if (gameData.escort.xSorryTime + I2X (1) > gameData.time.xGame) {
	gameData.escort.xLastMsgTime = 0;	//	Force this message to get through.
	if (gameData.escort.xSorryTime < gameData.time.xGame + I2X (2))
		BuddyMessage (TXT_BUDDY_SORRY);
	gameData.escort.xSorryTime = -I2X (2);
	}
//	If buddy not allowed to talk, then he is locked in his room.  Make him mostly do nothing unless you're nearby.
if (!gameData.escort.bMayTalk)
	if (xDistToPlayer > I2X (100))
		aip->SKIP_AI_COUNT = (int8_t) ((I2X (1) / 4) / (gameData.time.xFrame ? gameData.time.xFrame : 1));
//	AIM_WANDER has been co-opted for buddy behavior (didn't want to modify aistruct.h)
//	It means the CObject has been told to get lost and has come to the end of its path.
//	If the player is now visible, then create a path.
if (ailp->mode == AIM_WANDER)
	if (nPlayerVisibility) {
		CreateNSegmentPath (objP, 16 + RandShort () * 16, -1);
		aip->nPathLength = SmoothPath (objP, gameData.ai.routeSegs + aip->nHideIndex, aip->nPathLength);
		}
if (gameData.escort.nSpecialGoal == ESCORT_GOAL_SCRAM) {
	if (nPlayerVisibility)
		if (gameData.escort.xLastPathCreated + I2X (3) < gameData.time.xGame) {
#if TRACE
			console.printf (CON_DBG, "Frame %i: Buddy creating new scram path.\n", gameData.app.nFrameCount);
#endif
			CreateNSegmentPath (objP, 10 + RandShort () * 16, gameData.objs.consoleP->info.nSegment);
			gameData.escort.xLastPathCreated = gameData.time.xGame;
			}
	// -- Int3 ();
	return;
	}
//	Force checking for new goal every 5 seconds, and create new path, if necessary.
if (((gameData.escort.nSpecialGoal != ESCORT_GOAL_SCRAM) && ((gameData.escort.xLastPathCreated + I2X (5)) < gameData.time.xGame)) ||
	 ((gameData.escort.nSpecialGoal == ESCORT_GOAL_SCRAM) && ((gameData.escort.xLastPathCreated + I2X (15)) < gameData.time.xGame))) {
	gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escort.xLastPathCreated = gameData.time.xGame;
	}
if ((gameData.escort.nSpecialGoal != ESCORT_GOAL_SCRAM) && TimeToVisitPlayer (objP, ailp, aip)) {
	int32_t	nMaxLen;

	Buddy_last_player_path_created = gameData.time.xGame;
	ailp->mode = AIM_GOTO_PLAYER;
	if (!nPlayerVisibility) {
		if ((Last_come_back_messageTime + I2X (1) < gameData.time.xGame) || (Last_come_back_messageTime > gameData.time.xGame)) {
			BuddyMessage (TXT_COMING_BACK);
			Last_come_back_messageTime = gameData.time.xGame;
			}
		}
	//	No point in Buddy creating very long path if he's not allowed to talk.  Really kills framerate.
	nMaxLen = gameData.escort.nMaxLength;
	if (!gameData.escort.bMayTalk)
		nMaxLen = 3;
	CreatePathToTarget (objP, nMaxLen, 1);	//	MK!: Last parm used to be 1!
	if (aip->nPathLength > 0) {
		aip->nPathLength = SmoothPath (objP, gameData.ai.routeSegs + aip->nHideIndex, aip->nPathLength);
		ailp->mode = AIM_GOTO_PLAYER;
		}
	}
else if (gameData.time.xGame - xBuddyLastSeenPlayer > MAX_ESCORT_TIME_AWAY) {
	//	This is to prevent buddy from looking for a goal, which he will do because we only allow path creation once/second.
	return;
	}
else if ((ailp->mode == AIM_GOTO_PLAYER) && (xDistToPlayer < MIN_ESCORT_DISTANCE)) {
	gameData.escort.nGoalObject = EscortSetGoalObject ();
	ailp->mode = AIM_GOTO_OBJECT;		//	May look stupid to be before path creation, but AIDoorIsOpenable uses mode to determine what doors can be got through
	EscortCreatePathToGoal (objP);
	if (aip->nPathLength > 0) {
		aip->nPathLength = SmoothPath (objP, gameData.ai.routeSegs + aip->nHideIndex, aip->nPathLength);
		if (aip->nPathLength < 3)
			CreateNSegmentPath (objP, 5, gameData.ai.target.nBelievedSeg);
		ailp->mode = AIM_GOTO_OBJECT;
		}
	}
else if (gameData.escort.nGoalObject == ESCORT_GOAL_UNSPECIFIED) {
	if ((ailp->mode != AIM_GOTO_PLAYER) || (xDistToPlayer < MIN_ESCORT_DISTANCE)) {
		gameData.escort.nGoalObject = EscortSetGoalObject ();
		ailp->mode = AIM_GOTO_OBJECT;		//	May look stupid to be before path creation, but AIDoorIsOpenable uses mode to determine what doors can be got through
		EscortCreatePathToGoal (objP);
		if (aip->nPathLength > 0) {
			aip->nPathLength = SmoothPath (objP, gameData.ai.routeSegs + aip->nHideIndex, aip->nPathLength);
			if (aip->nPathLength < 3)
				CreateNSegmentPath (objP, 5, gameData.ai.target.nBelievedSeg);
			ailp->mode = AIM_GOTO_OBJECT;
			}
		}
	}
}

//	-------------------------------------------------------------------------------------------------

void InvalidateEscortGoal (void)
{
	gameData.escort.nGoalObject = -1;
}

// --------------------------------------------------------------------------------------------------------------

int32_t ShowEscortHelp (char *pszGoal, char *tstr)
{

	int32_t	nItems;
	CMenu	m (12);
	char	szGoal	[40], szMsgs [40];
#if 0
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
#endif

sprintf (szGoal, TXT_GOAL_NEXT, pszGoal);
sprintf (szMsgs, TXT_GOAL_MESSAGES, tstr);
m.AddText ("", szGoal);
for (nItems = 1; nItems < 10; nItems++) {
	m.AddText ("", const_cast<char*> (GT (343 + nItems)), -1);
	if (*m.Top ()->m_text)
		m.Top ()->m_nType = NM_TYPE_MENU;
	}
m.AddText ("", szMsgs, KEY_T);
return m.TinyMenu (NULL, "Guide-Bot Commands");
}

// --------------------------------------------------------------------------------------------------------------

void DoEscortMenu (void)
{
if (IsMultiGame) {
	HUDInitMessage (TXT_GB_MULTIPLAYER);
	return;
	}

if (gameStates.app.bD1Mission)
	return;

	int32_t		i;
	int32_t		next_goal;
	char		szGoal [32], tstr [32];
	CObject	*objP;

FORALL_ROBOT_OBJS (objP) {
	if (ROBOTINFO (objP->info.nId).companion)
		break;
	}

if (!IS_OBJECT (objP, objP->Index ())) {
#if 1//DBG - always allow buddy bot creation
		//	If no buddy bot, create one!
		HUDInitMessage (TXT_GB_CREATE);
		CreateBuddyBot ();
#else
		HUDInitMessage (TXT_GB_NONE);
		return;
#endif
	}

BuddyMayTalk ();	//	Needed here or we might not know buddy can talk when he can.

if (!gameData.escort.bMayTalk) {
	HUDInitMessage (TXT_GB_RELEASE, gameData.escort.szName);
	return;
	}

audio.PauseSounds ();
if (!gameOpts->menus.nStyle)
	StopTime ();

//paletteManager.SuspendEffect ();
GameFlushInputs ();
SetPopupScreenMode ();
//paletteManager.ResumeEffect ();

//	This prevents the buddy from coming back if you've told him to scram.
//	If we don't set next_goal, we get garbage there.
if (gameData.escort.nSpecialGoal == ESCORT_GOAL_SCRAM) {
	gameData.escort.nSpecialGoal = -1;	//	Else setting next goal might fail.
	next_goal = EscortSetGoalObject ();
	gameData.escort.nSpecialGoal = ESCORT_GOAL_SCRAM;
	}
else {
	gameData.escort.nSpecialGoal = -1;	//	Else setting next goal might fail.
	next_goal = EscortSetGoalObject ();
	}

switch (next_goal) {
#if DBG
	case ESCORT_GOAL_UNSPECIFIED:
		Int3 ();
		sprintf (szGoal, "ERROR");
		break;
#endif

	case ESCORT_GOAL_BLUE_KEY:
		sprintf (szGoal, TXT_GB_BLUEKEY);
		break;
	case ESCORT_GOAL_GOLD_KEY:
		sprintf (szGoal, TXT_GB_YELLOWKEY);
		break;
	case ESCORT_GOAL_RED_KEY:
		sprintf (szGoal, TXT_GB_REDKEY);
		break;
	case ESCORT_GOAL_CONTROLCEN:
		sprintf (szGoal, TXT_GB_REACTOR);
		break;
	case ESCORT_GOAL_BOSS:
		sprintf (szGoal, TXT_GB_BOSS);
		break;
	case ESCORT_GOAL_EXIT:
		sprintf (szGoal, TXT_GB_EXIT);
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
		sprintf (szGoal, TXT_GB_MARKER, next_goal-ESCORT_GOAL_MARKER1+1);
		break;
	}

if (!gameData.escort.bMsgsSuppressed)
	sprintf (tstr, TXT_GB_SUPPRESS);
else
	sprintf (tstr, TXT_GB_ENABLE);

i = ShowEscortHelp (szGoal, tstr);
if (i < 11) {
	gameData.escort.bSearchingMarker = -1;
	gameData.escort.nLastKey = -1;
	EscortSetSpecialGoal (i ? KEY_1 + i - 1 : KEY_0);
	gameData.escort.nLastKey = -1;
	}
else if (i == 11) {
	BuddyMessage (gameData.escort.bMsgsSuppressed ? TXT_GB_MSGS_ON : TXT_GB_MSGS_OFF);
	gameData.escort.bMsgsSuppressed = !gameData.escort.bMsgsSuppressed;
	}
GameFlushInputs ();
//paletteManager.ResumeEffect ();
if (!gameOpts->menus.nStyle)
	StartTime (0);
audio.ResumeSounds ();
}

// --------------------------------------------------------------------------------------------------------------------
//	Create a Buddy bot.
//	This automatically happens when you bring up the Buddy menu in a debug version.
//	It is available as a cheat in a non-debug (release) version.
void CreateBuddyBot (void)
{
	uint8_t	buddy_id;
	CFixVector	vObjPos;

for (buddy_id = 0; buddy_id < gameData.bots.nTypes [0]; buddy_id++)
	if (gameData.bots.info [0][buddy_id].companion)
		break;

	if (buddy_id == gameData.bots.nTypes [0]) {
#if TRACE
		console.printf (CON_DBG, "Can't create Buddy.  No 'companion' bot found in gameData.bots.infoP!\n");
#endif
		return;
	}
	vObjPos = SEGMENTS [OBJSEG (gameData.objs.consoleP)].Center ();
	CreateMorphRobot (SEGMENTS + OBJSEG (gameData.objs.consoleP), &vObjPos, buddy_id);
}

//	-------------------------------------------------------------------------------
