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
ENTER (1, 0);
	CObject*	pObj;

gameData.escortData.bMayTalk = 0;
gameData.escortData.nObjNum = -1;
gameData.escortData.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
gameData.escortData.nSpecialGoal = -1;
gameData.escortData.nGoalIndex = -1;
gameData.escortData.bMsgsSuppressed = 0;
FORALL_ROBOT_OBJS (pObj)
	if (pObj->IsGuideBot ())
		break;
if (IS_OBJECT (pObj, pObj->Index ()))
	gameData.escortData.nObjNum = pObj->Index ();
gameData.escortData.xSorryTime = -I2X (1);
gameData.escortData.bSearchingMarker = -1;
gameData.escortData.nLastKey = -1;
RETURN
}

//	-----------------------------------------------------------------------------
//	See if CSegment from curseg through nSide is reachable.
//	Return true if it is reachable, else return false.
int32_t SegmentIsReachable (int32_t curseg, int16_t nSide)
{
ENTER (1, 0);
RETVAL (AIDoorIsOpenable (NULL, SEGMENT (curseg), nSide))
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
ENTER (1, 0);
	int32_t		head = 0, tail = 0;
	int8_t		bVisited [MAX_SEGMENTS_D2X];

memset (bVisited, 0, LEVEL_SEGMENTS * sizeof (int8_t));
segList [head++] = nStartSeg;
bVisited [nStartSeg] = 1;
if (nMaxSegs > LEVEL_SEGMENTS)
	nMaxSegs = LEVEL_SEGMENTS;

while ((head != tail) && (head < nMaxSegs)) {
	int16_t nSegment = segList [tail++];
	CSegment	*pSeg = SEGMENT (nSegment);

	for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
		int32_t nConnSeg = pSeg->m_children [i];
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
RETURN
}

//	-----------------------------------------------------------------------------
//	Return true if ok for buddy to talk, else return false.
//	Buddy is allowed to talk if the CSegment he is in does not contain a blastable CWall that has not been blasted
//	AND he has never yet, since being initialized for level, been allowed to talk.
int32_t BuddyMayTalk (void)
{
ENTER (1, 0);
CObject* pObj = OBJECT (gameData.escortData.nObjNum);

if ((gameData.escortData.nObjNum < 0) || (pObj && !pObj->IsRobot ())) {
	gameData.escortData.bMayTalk = 0;
	RETVAL (0)
	}

if (gameData.escortData.bMayTalk)
	RETVAL (1)

if (pObj && !pObj->IsGuideBot ()) {
	FORALL_ROBOT_OBJS (pObj)
		if (pObj->IsGuideBot ())
			break;
	if (!IS_OBJECT (pObj, pObj->Index ()))
		RETVAL (0)
	gameData.escortData.nObjNum = pObj->Index ();
	}

CSegment *pSeg = SEGMENT (OBJECT (gameData.escortData.nObjNum)->info.nSegment);

for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
	CWall* pWall = pSeg->Wall (i);
	if (pWall && (pWall->nType == WALL_BLASTABLE) && !(pWall->flags & WALL_BLASTED))
		RETVAL (0)
	//	Check one level deeper.
	if (IS_CHILD (pSeg->m_children [i])) {
		int32_t		j;
		CSegment	*pConnSeg = SEGMENT (pSeg->m_children [i]);

		for (j = 0; j < SEGMENT_SIDE_COUNT; j++) {
			CWall* pOtherWall = pConnSeg->Wall (j);
			if (pOtherWall && (pOtherWall->nType == WALL_BLASTABLE) && !(pOtherWall->flags & WALL_BLASTED))
				RETVAL (0)
			}
		}
	}
gameData.escortData.bMayTalk = 1;
RETVAL (1)
}

//	--------------------------------------------------------------------------------------------

void DetectEscortGoalAccomplished (int32_t index)
{
ENTER (1, 0);
	int32_t	i, j;
	int32_t	bDetected = 0;
	CObject	*pObj;
	int16_t*	childI, * childJ;

//	If goal is to go away, how can it be achieved?
if (gameData.escortData.nSpecialGoal == ESCORT_GOAL_SCRAM)
	RETURN

//	See if goal found was a key.  Need to handle default goals differently.
//	Note, no buddy_met_goal sound when blow up reactor or exit.  Not great, but ok
//	since for reactor, noisy, for exit, buddy is disappearing.
if ((gameData.escortData.nSpecialGoal == -1) && (gameData.escortData.nGoalIndex == index)) {
	bDetected = 1;
	goto dega_ok;
	}

if ((gameData.escortData.nGoalIndex <= ESCORT_GOAL_RED_KEY) && (index >= 0) && (index <= gameData.objData.nLastObject [0])) {
	pObj = OBJECT (index);
	if (pObj->info.nType == OBJ_POWERUP)  {
		uint8_t id = pObj->info.nId;
		if (id == POW_KEY_BLUE) {
			if (gameData.escortData.nGoalIndex == ESCORT_GOAL_BLUE_KEY) {
				bDetected = 1;
				goto dega_ok;
				}
			}
		else if (id == POW_KEY_GOLD) {
			if (gameData.escortData.nGoalIndex == ESCORT_GOAL_GOLD_KEY) {
				bDetected = 1;
				goto dega_ok;
				}
			}
		else if (id == POW_KEY_RED) {
			if (gameData.escortData.nGoalIndex == ESCORT_GOAL_RED_KEY) {
				bDetected = 1;
				goto dega_ok;
				}
			}
		}
	}
if (gameData.escortData.nSpecialGoal != -1) {
	if (gameData.escortData.nSpecialGoal == ESCORT_GOAL_ENERGYCEN) {
		if (index == -4)
			bDetected = 1;
		else if ((index >= 0) && (index <= gameData.segData.nLastSegment) &&
					(gameData.escortData.nGoalIndex >= 0) && (gameData.escortData.nGoalIndex <= gameData.segData.nLastSegment)) {
			childI = SEGMENT (index)->m_children;
			for (i = SEGMENT_SIDE_COUNT; i; i--, childI++) {
				if (*childI == gameData.escortData.nGoalIndex) {
					bDetected = 1;
					goto dega_ok;
					}
				else {
					childJ = SEGMENT (*childI)->m_children;
					for (j = SEGMENT_SIDE_COUNT; j; j--, childJ++) {
						if (*childJ == gameData.escortData.nGoalIndex) {
							bDetected = 1;
							goto dega_ok;
							}
						}
					}
				}
			}
		}
	else if ((index >= 0) && (index <= gameData.objData.nLastObject [0])) {
		pObj = OBJECT (index);
		if ((pObj->info.nType == OBJ_POWERUP) && (gameData.escortData.nSpecialGoal == ESCORT_GOAL_POWERUP))
			bDetected = 1;	//	Any nType of powerup picked up will do.
		else if ((gameData.escortData.nGoalIndex >= 0) && (gameData.escortData.nGoalIndex <= gameData.objData.nLastObject [0]) &&
					(pObj->info.nType == OBJECT (gameData.escortData.nGoalIndex)->info.nType) &&
					(pObj->info.nId == OBJECT (gameData.escortData.nGoalIndex)->info.nId)) {
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
	gameData.escortData.nGoalIndex = -1;
	gameData.escortData.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escortData.nSpecialGoal = -1;
	gameData.escortData.bSearchingMarker = -1;
	}
RETURN
}

//	-------------------------------------------------------------------------------------------------

void ChangeGuidebotName (void)
{
	CMenu	m;
	char	text [GUIDEBOT_NAME_LEN+1] = "";
	int32_t	item;

strcpy (text,gameData.escortData.szName);
m.AddInput ("guidebot name", text, GUIDEBOT_NAME_LEN);
item = m.Menu (NULL, "Enter Guide-bot name:");
if (item != -1) {
	strcpy (gameData.escortData.szName, text);
	strcpy (gameData.escortData.szRealName, text);
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
ENTER (1, 0);
if (gameData.escortData.bMsgsSuppressed)
	RETURN
if (IsMultiGame)
	RETURN
if ((gameData.escortData.xLastMsgTime + I2X (1) < gameData.timeData.xGame) ||
	 (gameData.escortData.xLastMsgTime > gameData.timeData.xGame)) {
	if (BuddyMayTalk ()) {
		char		szMsg [200];
		va_list	args;
		int32_t		l = (int32_t) strlen (gameData.escortData.szName);

		va_start (args, format);
		vsprintf (szMsg + l + 10, format, args);
		va_end (args);

		szMsg [0] = (char) 1;
		szMsg [1] = (char) (127 + 128);
		szMsg [2] = (char) (127 + 128);
		szMsg [3] = (char) (0 + 128);
		memcpy (szMsg + 4, gameData.escortData.szName, l);
		szMsg [l+4] = ':';
		szMsg [l+5] = ' ';
		szMsg [l+6] = (char) 1;
		szMsg [l+7] = (char) (0 + 128);
		szMsg [l+8] = (char) (127 + 128);
		szMsg [l+9] = (char) (0 + 128);

		HUDInitMessage (szMsg);
		gameData.escortData.xLastMsgTime = gameData.timeData.xGame;
		}
	}
RETURN
}

//	-----------------------------------------------------------------------------
//	Return true if marker #id has been placed.
int32_t MarkerExistsInMine (int32_t id)
{
	CObject*	pObj;

FORALL_OBJS (pObj)
	if ((pObj->info.nType == OBJ_MARKER) && (pObj->info.nId == id))
		return 1;
return 0;
}

//	-----------------------------------------------------------------------------

void EscortSetSpecialGoal (int32_t specialKey)
{
ENTER (1, 0);
	int32_t markerKey;

gameData.escortData.bMsgsSuppressed = 0;
if (!gameData.escortData.bMayTalk) {
	BuddyMayTalk ();
	if (!gameData.escortData.bMayTalk) {
		CObject*	pObj;

		FORALL_ROBOT_OBJS (pObj)
			if (pObj->IsGuideBot ()) {
				HUDInitMessage (TXT_GB_RELEASE, gameData.escortData.szName);
				RETURN
				}
		HUDInitMessage (TXT_GB_NONE);
		RETURN
		}
	}

specialKey = specialKey & (~KEY_SHIFTED);
markerKey = specialKey;

if (gameData.escortData.nLastKey == specialKey) {
	if ((gameData.escortData.bSearchingMarker == -1) && (specialKey != KEY_0)) {
		if (MarkerExistsInMine (markerKey - KEY_1))
			gameData.escortData.bSearchingMarker = markerKey - KEY_1;
		else {
			gameData.escortData.xLastMsgTime = 0;	//	Force this message to get through.
			BuddyMessage ("Marker %i not placed.", markerKey - KEY_1 + 1);
			gameData.escortData.bSearchingMarker = -1;
			}
		} 
	else {
		gameData.escortData.bSearchingMarker = -1;
		}
	}
gameData.escortData.nLastKey = specialKey;
if (specialKey == KEY_0)
	gameData.escortData.bSearchingMarker = -1;
	if (gameData.escortData.bSearchingMarker != -1)
		gameData.escortData.nSpecialGoal = ESCORT_GOAL_MARKER1 + markerKey - KEY_1;
else {
	switch (specialKey) {
		case KEY_1:
			gameData.escortData.nSpecialGoal = ESCORT_GOAL_ENERGY;
			break;
		case KEY_2:
			gameData.escortData.nSpecialGoal = ESCORT_GOAL_ENERGYCEN;
			break;
		case KEY_3:
			gameData.escortData.nSpecialGoal = ESCORT_GOAL_SHIELD;
			break;
		case KEY_4:
			gameData.escortData.nSpecialGoal = ESCORT_GOAL_POWERUP;
			break;
		case KEY_5:
			gameData.escortData.nSpecialGoal = ESCORT_GOAL_ROBOT;
			break;
		case KEY_6:
			gameData.escortData.nSpecialGoal = ESCORT_GOAL_HOSTAGE;
			break;
		case KEY_7:
			gameData.escortData.nSpecialGoal = ESCORT_GOAL_SCRAM;
			break;
		case KEY_8:
			gameData.escortData.nSpecialGoal = ESCORT_GOAL_PLAYER_SPEW;
			break;
		case KEY_9:
			gameData.escortData.nSpecialGoal = ESCORT_GOAL_EXIT;
			break;
		case KEY_0:
			gameData.escortData.nSpecialGoal = -1;
			break;
		default:
			Int3 ();		//	Oops, called with illegal key value.
		}
	}
gameData.escortData.xLastMsgTime = gameData.timeData.xGame - I2X (2);	//	Allow next message to come through.
SayEscortGoal (gameData.escortData.nSpecialGoal);
gameData.escortData.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
RETURN
}

//	-----------------------------------------------------------------------------
//	Return id of boss.
int32_t GetBossId (void)
{
ENTER (1, 0);
	int32_t	h, i;

for (h = gameData.bossData.ToS (), i = 0; i < h; i++) {
	int32_t nObject = gameData.bossData [i].m_nObject;
	CObject *pObj = OBJECT (nObject);
	if (pObj)
		RETVAL (pObj->info.nId)
	}
RETVAL (-1)
}

//	-----------------------------------------------------------------------------
//	Return bject index if CObject of objType, objId exists in mine, else return -1
//	"special" is used to find OBJECTS spewed by player which is hacked into flags field of powerup.
int32_t ExistsInMine2 (int32_t nSegment, int32_t objType, int32_t objId, int32_t special)
{
ENTER (1, 0);
if ((objType == OBJ_POWERUP) && (gameStates.app.bGameSuspended & SUSP_POWERUPS))
	RETVAL (-1)


	int32_t nObject = SEGMENT (nSegment)->m_objects;
	
if (nObject != -1) {
	while (nObject != -1) {
		CObject	*pCurObj = OBJECT (nObject);

		if ((special == ESCORT_GOAL_PLAYER_SPEW) && (pCurObj->info.nFlags & OF_PLAYER_DROPPED))
			RETVAL (nObject)
		if (pCurObj->info.nType == objType) {
			//	Don't find escort robots if looking for robot!
			if (pCurObj->IsGuideBot ())
				;
			else if (objId == -1)
				RETVAL (nObject)
			else if (pCurObj->info.nId == objId)
				RETVAL (nObject)
			}
		if (objType == OBJ_POWERUP) {
			if (pCurObj->info.contains.nCount && (pCurObj->info.contains.nType == OBJ_POWERUP) && (pCurObj->info.contains.nId == objId))
				RETVAL (nObject)
			}
		nObject = pCurObj->info.nNextInSeg;
		}
	}
RETVAL (-1)
}

//	-----------------------------------------------------------------------------
//	Return nearest object of interest.
//	If special == ESCORT_GOAL_PLAYER_SPEW, then looking for any CObject spewed by player.
//	-1 means CObject does not exist in mine.
//	-2 means CObject does exist in mine, but buddy-bot can't reach it (eg, behind triggered CWall)
int32_t ExistsInMine (int32_t nStartSeg, int32_t objType, int32_t objId, int32_t special)
{
ENTER (1, 0);
	int32_t	nSegIdx, nSegment;
	int16_t	bfsList [MAX_SEGMENTS_D2X];
	int32_t	length;
	int32_t	nObject;

CreateBfsList (nStartSeg, bfsList, &length, LEVEL_SEGMENTS);
if (objType == PRODUCER_CHECK) {
	for (nSegIdx = 0; nSegIdx < length; nSegIdx++) {
		nSegment = bfsList [nSegIdx];
		if (SEGMENT (nSegment)->m_function == SEGMENT_FUNC_FUELCENTER)
			RETVAL (nSegment)
		}
	}
else {
	for (nSegIdx = 0; nSegIdx < length; nSegIdx++) {
		nSegment = bfsList [nSegIdx];
		nObject = ExistsInMine2 (nSegment, objType, objId, special);
		if (nObject != -1)
			RETVAL (nObject)
		}
	}
//	Couldn't find what we're looking for by looking at connectivity.
//	See if it's in the mine.  It could be hidden behind a trigger or switch
//	which the buddybot doesn't understand.
if (objType == PRODUCER_CHECK) {
	for (nSegment = 0; nSegment <= gameData.segData.nLastSegment; nSegment++)
		if (SEGMENT (nSegment)->m_function == SEGMENT_FUNC_FUELCENTER)
			RETVAL (-2)
	}
else {
	for (nSegment = 0; nSegment <= gameData.segData.nLastSegment; nSegment++) {
		nObject = ExistsInMine2 (nSegment, objType, objId, special);
		if (nObject != -1)
			RETVAL (-2)
		}
	}
RETVAL (-1)
}

//	-----------------------------------------------------------------------------
//	Return true if it happened, else return false.
int32_t FindExitSegment (void)
{
ENTER (1, 0);
	CSegment* pSeg = SEGMENTS.Buffer ();

for (int32_t i = 0; i <= gameData.segData.nSegments; i++, pSeg++)
	for (int32_t j = 0; j < SEGMENT_SIDE_COUNT; j++)
		if (pSeg->m_children [j] == -2) {
			RETVAL (i)
		}
RETVAL (-1)
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

void EscortCreatePathToGoal (CObject *pObj)
{
ENTER (1, 0);
	int16_t			nGoalSeg = -1;
	int16_t			nObject = pObj->Index ();
	tAIStaticInfo*	aip = &pObj->cType.aiInfo;
	tAILocalInfo*	ailp = gameData.aiData.localInfo + nObject;

Assert (nObject >= 0);
if (gameData.escortData.nSpecialGoal != -1)
	gameData.escortData.nGoalObject = gameData.escortData.nSpecialGoal;
gameData.escortData.nKillObject = -1;
if (gameData.escortData.bSearchingMarker != -1) {
	gameData.escortData.nGoalIndex = ExistsInMine (pObj->info.nSegment, OBJ_MARKER, gameData.escortData.nGoalObject - ESCORT_GOAL_MARKER1, -1);
	if (gameData.escortData.nGoalIndex > -1)
		nGoalSeg = OBJECT (gameData.escortData.nGoalIndex)->info.nSegment;
	}
else {
	switch (gameData.escortData.nGoalObject) {
		case ESCORT_GOAL_BLUE_KEY:
			gameData.escortData.nGoalIndex = ExistsInMine (pObj->info.nSegment, OBJ_POWERUP, POW_KEY_BLUE, -1);
			if (gameData.escortData.nGoalIndex > -1)
				nGoalSeg = OBJECT (gameData.escortData.nGoalIndex)->info.nSegment;
			break;
		case ESCORT_GOAL_GOLD_KEY:
			gameData.escortData.nGoalIndex = ExistsInMine (pObj->info.nSegment, OBJ_POWERUP, POW_KEY_GOLD, -1);
			if (gameData.escortData.nGoalIndex > -1)
				nGoalSeg =OBJECT (gameData.escortData.nGoalIndex)->info.nSegment;
			break;
		case ESCORT_GOAL_RED_KEY:
			gameData.escortData.nGoalIndex = ExistsInMine (pObj->info.nSegment, OBJ_POWERUP, POW_KEY_RED, -1);
			if (gameData.escortData.nGoalIndex > -1)
				nGoalSeg =OBJECT (gameData.escortData.nGoalIndex)->info.nSegment;
			break;
		case ESCORT_GOAL_CONTROLCEN:
			gameData.escortData.nGoalIndex = ExistsInMine (pObj->info.nSegment, OBJ_REACTOR, -1, -1);
			if (gameData.escortData.nGoalIndex > -1)
				nGoalSeg =OBJECT (gameData.escortData.nGoalIndex)->info.nSegment;
			break;
		case ESCORT_GOAL_EXIT:
		case ESCORT_GOAL_EXIT2:
			nGoalSeg = FindExitSegment ();
			gameData.escortData.nGoalIndex = nGoalSeg;
			break;
		case ESCORT_GOAL_ENERGY:
			gameData.escortData.nGoalIndex = ExistsInMine (pObj->info.nSegment, OBJ_POWERUP, POW_ENERGY, -1);
			if (gameData.escortData.nGoalIndex > -1)
				nGoalSeg =OBJECT (gameData.escortData.nGoalIndex)->info.nSegment;
			break;
		case ESCORT_GOAL_ENERGYCEN:
			nGoalSeg = ExistsInMine (pObj->info.nSegment, PRODUCER_CHECK, -1, -1);
			gameData.escortData.nGoalIndex = nGoalSeg;
			break;
		case ESCORT_GOAL_SHIELD:
			gameData.escortData.nGoalIndex = ExistsInMine (pObj->info.nSegment, OBJ_POWERUP, POW_SHIELD_BOOST, -1);
			if (gameData.escortData.nGoalIndex > -1)
				nGoalSeg =OBJECT (gameData.escortData.nGoalIndex)->info.nSegment;
			break;
		case ESCORT_GOAL_POWERUP:
			gameData.escortData.nGoalIndex = ExistsInMine (pObj->info.nSegment, OBJ_POWERUP, -1, -1);
			if (gameData.escortData.nGoalIndex > -1)
				nGoalSeg =OBJECT (gameData.escortData.nGoalIndex)->info.nSegment;
			break;
		case ESCORT_GOAL_ROBOT:
			gameData.escortData.nGoalIndex = ExistsInMine (pObj->info.nSegment, OBJ_ROBOT, -1, -1);
			if (gameData.escortData.nGoalIndex > -1)
				nGoalSeg =OBJECT (gameData.escortData.nGoalIndex)->info.nSegment;
			break;
		case ESCORT_GOAL_HOSTAGE:
			gameData.escortData.nGoalIndex = ExistsInMine (pObj->info.nSegment, OBJ_HOSTAGE, -1, -1);
			if (gameData.escortData.nGoalIndex > -1)
				nGoalSeg =OBJECT (gameData.escortData.nGoalIndex)->info.nSegment;
			break;
		case ESCORT_GOAL_PLAYER_SPEW:
			gameData.escortData.nGoalIndex = ExistsInMine (pObj->info.nSegment, -1, -1, ESCORT_GOAL_PLAYER_SPEW);
			if (gameData.escortData.nGoalIndex > -1)
				nGoalSeg =OBJECT (gameData.escortData.nGoalIndex)->info.nSegment;
			break;
		case ESCORT_GOAL_SCRAM:
			nGoalSeg = -3;		//	Kinda a hack.
			gameData.escortData.nGoalIndex = nGoalSeg;
			break;
		case ESCORT_GOAL_BOSS: {
			int32_t	boss_id;

			boss_id = GetBossId ();
			Assert (boss_id != -1);
			gameData.escortData.nGoalIndex = ExistsInMine (pObj->info.nSegment, OBJ_ROBOT, boss_id, -1);
			if (gameData.escortData.nGoalIndex > -1)
				nGoalSeg =OBJECT (gameData.escortData.nGoalIndex)->info.nSegment;
			break;
		}
		default:
			Int3 ();	//	Oops, Illegal value in gameData.escortData.nGoalObject.
			nGoalSeg = 0;
			break;
		}
	}
if ((gameData.escortData.nGoalIndex < 0) && (gameData.escortData.nGoalIndex != -3)) {	//	I apologize for this statement -- MK, 09/22/95
	if (gameData.escortData.nGoalIndex == -1) {
		gameData.escortData.xLastMsgTime = 0;	//	Force this message to get through.
		BuddyMessage (TXT_NOT_IN_MINE, GT (nEscortGoalText [gameData.escortData.nGoalObject-1]));
		gameData.escortData.bSearchingMarker = -1;
		}
	else if (gameData.escortData.nGoalIndex == -2) {
		gameData.escortData.xLastMsgTime = 0;	//	Force this message to get through.
		BuddyMessage (TXT_CANT_REACH, GT (nEscortGoalText [gameData.escortData.nGoalObject-1]));
		gameData.escortData.bSearchingMarker = -1;
		}
	else
		Int3 ();

	gameData.escortData.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escortData.nSpecialGoal = -1;
	}
else {
	if (nGoalSeg == -3) {
		CreateNSegmentPath (pObj, 16 + Rand (16), -1);
		aip->nPathLength = SmoothPath (pObj, gameData.aiData.routeSegs + aip->nHideIndex, aip->nPathLength);
		}
	else {
		CreatePathToSegment (pObj, nGoalSeg, gameData.escortData.nMaxLength, 1);	//	MK!: Last parm (safetyFlag) used to be 1!!
		if (aip->nPathLength > 3)
			aip->nPathLength = SmoothPath (pObj, gameData.aiData.routeSegs + aip->nHideIndex, aip->nPathLength);
		if ((aip->nPathLength > 0) && (gameData.aiData.routeSegs [aip->nHideIndex + aip->nPathLength - 1].nSegment != nGoalSeg)) {
			fix	xDistToPlayer;
			gameData.escortData.xLastMsgTime = 0;	//	Force this message to get through.
			BuddyMessage (TXT_CANT_REACH, GT (nEscortGoalText [gameData.escortData.nGoalObject-1]));
			gameData.escortData.bSearchingMarker = -1;
			gameData.escortData.nGoalObject = ESCORT_GOAL_SCRAM;
			xDistToPlayer = simpleRouter [0].PathLength (pObj->info.position.vPos, pObj->info.nSegment, 
																		gameData.aiData.target.vBelievedPos, gameData.aiData.target.nBelievedSeg, 
																		100, WID_PASSABLE_FLAG, 1);
			if (xDistToPlayer > MIN_ESCORT_DISTANCE)
				CreatePathToTarget (pObj, gameData.escortData.nMaxLength, 1);	//	MK!: Last parm used to be 1!
			else {
				CreateNSegmentPath (pObj, 8 + RandShort () * 8, -1);
				aip->nPathLength = SmoothPath (pObj, gameData.aiData.routeSegs + aip->nHideIndex, aip->nPathLength);
				}
			}
		}
	ailp->mode = AIM_GOTO_OBJECT;
	SayEscortGoal (gameData.escortData.nGoalObject);
	}
RETURN
}

//	-----------------------------------------------------------------------------
//	Escort robot chooses goal CObject based on player's keys, location.
//	Returns goal CObject.
int32_t EscortSetGoalObject (void)
{
if (gameData.escortData.nSpecialGoal != -1)
	return ESCORT_GOAL_UNSPECIFIED;
else if (!(gameData.objData.pConsole->info.nFlags & PLAYER_FLAGS_BLUE_KEY) &&
			 (ExistsInMine (gameData.objData.pConsole->info.nSegment, OBJ_POWERUP, POW_KEY_BLUE, -1) != -1))
	return ESCORT_GOAL_BLUE_KEY;
else if (!(gameData.objData.pConsole->info.nFlags & PLAYER_FLAGS_GOLD_KEY) &&
			 (ExistsInMine (gameData.objData.pConsole->info.nSegment, OBJ_POWERUP, POW_KEY_GOLD, -1) != -1))
	return ESCORT_GOAL_GOLD_KEY;
else if (!(gameData.objData.pConsole->info.nFlags & PLAYER_FLAGS_RED_KEY) &&
			 (ExistsInMine (gameData.objData.pConsole->info.nSegment, OBJ_POWERUP, POW_KEY_RED, -1) != -1))
	return ESCORT_GOAL_RED_KEY;
else if (!gameData.reactorData.bDestroyed) {
	for (int32_t i = 0; i < int32_t (gameData.bossData.ToS ()); i++)
		if ((gameData.bossData [i].m_nObject >= 0) && gameData.bossData [i].m_nTeleportSegs)
			return ESCORT_GOAL_BOSS;
		return ESCORT_GOAL_CONTROLCEN;
	}
else
	return ESCORT_GOAL_EXIT;
}

#define	MAX_ESCORT_TIME_AWAY		 (I2X (4))

fix	xBuddyLastSeenPlayer = 0, Buddy_last_player_path_created;

//	-----------------------------------------------------------------------------

int32_t TimeToVisitPlayer (CObject *pObj, tAILocalInfo *ailp, tAIStaticInfo *aip)
{
ENTER (1, 0);
	//	Note: This one has highest priority because, even if already going towards player,
	//	might be necessary to create a new path, as player can move.
if (gameData.timeData.xGame - xBuddyLastSeenPlayer > MAX_ESCORT_TIME_AWAY)
	if (gameData.timeData.xGame - Buddy_last_player_path_created > I2X (1))
		RETVAL (1)
if (ailp->mode == AIM_GOTO_PLAYER)
	RETVAL (0)
if (pObj->info.nSegment == gameData.objData.pConsole->info.nSegment)
	RETVAL (0)
if (aip->nCurPathIndex < aip->nPathLength/2)
	RETVAL (0)
RETVAL (1)
}

fix Last_come_back_messageTime = 0;

fix buddyLastMissileTime;

//	-----------------------------------------------------------------------------

void BashBuddyWeaponInfo (int32_t nWeaponObj)
{
	CObject	*pObj = OBJECT (nWeaponObj);

if (pObj) {
	pObj->cType.laserInfo.parent.nObject = OBJ_IDX (gameData.objData.pConsole);
	pObj->cType.laserInfo.parent.nType = OBJ_PLAYER;
	pObj->cType.laserInfo.parent.nSignature = gameData.objData.pConsole->info.nSignature;
	}
}

//	-----------------------------------------------------------------------------

int32_t MaybeBuddyFireMega (int16_t nObject)
{
ENTER (1, 0);
	CObject		*pObj = OBJECT (nObject);
	CObject		*pBuddyObj = OBJECT (gameData.escortData.nObjNum);
	fix			dist;
	CFixVector	vVecToRobot;
	int32_t		nWeaponObj;

vVecToRobot = pBuddyObj->info.position.vPos - pObj->info.position.vPos;
dist = CFixVector::Normalize (vVecToRobot);
if (dist > I2X (400))
	RETVAL (0)
if (!ObjectToObjectVisibility (pBuddyObj, pObj, FQ_TRANSWALL, dist < I2X(40) ? 0.95f : 0.99f))
	RETVAL (0)
if (gameData.weaponData.info [0][MEGAMSL_ID].renderType == 0) {
#if TRACE
	console.printf (CON_VERBOSE, "Buddy can't fire mega (shareware)\n");
#endif
	BuddyMessage (TXT_BUDDY_CLICK);
	RETVAL (0)
	}
#if TRACE
console.printf (CON_DBG, "Buddy firing mega in frame %i\n", gameData.appData.nFrameCount);
#endif
BuddyMessage (TXT_BUDDY_GAHOOGA);
CFixVector pos = pBuddyObj->info.position.vPos;
pos += pBuddyObj->info.position.mOrient.m.dir.f * I2X(5);
nWeaponObj = CreateNewWeaponSimple (&pBuddyObj->info.position.mOrient.m.dir.f, &pos, nObject, CONCUSSION_ID, 1);
if (nWeaponObj != -1)
	BashBuddyWeaponInfo (nWeaponObj);
RETVAL (1)
}

//-----------------------------------------------------------------------------

int32_t MaybeBuddyFireSmart (int16_t nObject)
{
ENTER (1, 0);
	CObject*	pObj = OBJECT (nObject);
	CObject*	pBuddyObj = OBJECT (gameData.escortData.nObjNum);
	fix		dist;
	int16_t	nWeaponObj;

dist = CFixVector::Dist (pBuddyObj->info.position.vPos, pObj->info.position.vPos);
if (dist > I2X (200) || dist < I2X(20))
	RETVAL (0)
if (!ObjectToObjectVisibility (pBuddyObj, pObj, FQ_TRANSWALL, 0.8f))
	RETVAL (0)
#if TRACE
console.printf (CON_DBG, "Buddy firing smart missile in frame %i\n", gameData.appData.nFrameCount);
#endif
BuddyMessage (TXT_BUDDY_WHAMMO);
nWeaponObj = CreateNewWeaponSimple (&pBuddyObj->info.position.mOrient.m.dir.f, &pBuddyObj->info.position.vPos, nObject, HOMINGMSL_ID, 1);
if (nWeaponObj != -1)
	BashBuddyWeaponInfo (nWeaponObj);
RETVAL (1)
}

//	-----------------------------------------------------------------------------

void DoBuddyDudeStuff (void)
{
ENTER (1, 0);
	CObject*	pObj;

if (!BuddyMayTalk ())
	RETURN

if (buddyLastMissileTime > gameData.timeData.xGame)
	buddyLastMissileTime = 0;

if (buddyLastMissileTime + (I2X (1) / 2) < gameData.timeData.xGame) {
	if (gameData.reactorData.states [0].nObject != -1 &&
		MaybeBuddyFireMega (gameData.reactorData.states [0].nObject)) {
		buddyLastMissileTime = gameData.timeData.xGame;
		RETURN
		}
	//	See if a robot potentially in view cone
	FORALL_ROBOT_OBJS (pObj)
		if (!pObj->IsGuideBot ())
			if (MaybeBuddyFireMega (pObj->Index ())) {
				buddyLastMissileTime = gameData.timeData.xGame;
				RETURN
			}
	//	See if a robot near enough that buddy should fire smart missile
	FORALL_ROBOT_OBJS (pObj)
		if (!pObj->IsGuideBot ())
			if (MaybeBuddyFireSmart (pObj->Index ())) {
				buddyLastMissileTime = gameData.timeData.xGame;
				RETURN
			}
	}
RETURN
}

//	-----------------------------------------------------------------------------
//	Called every frame (or something).
void DoEscortFrame (CObject *pObj, fix xDistToPlayer, int32_t nPlayerVisibility)
{
ENTER (1, 0);
	int32_t			nObject = pObj->Index ();

if (nObject < 0)
	RETURN

	tAIStaticInfo*	aip = &pObj->cType.aiInfo;
	tAILocalInfo*	ailp = gameData.aiData.localInfo + nObject;

nPlayerVisibility = 1;
gameData.escortData.nObjNum = nObject;
if (nPlayerVisibility) {
	xBuddyLastSeenPlayer = gameData.timeData.xGame;
	if ((PlayerHasHeadlight (-1) && EGI_FLAG (headlight.bDrainPower, 0, 0, 1))	&&
		 (X2I (LOCALPLAYER.Energy ()) < 40) && ((X2I (LOCALPLAYER.Energy ()) / 2) & 2) && (!gameStates.app.bPlayerIsDead))
		BuddyMessage (TXT_HEADLIGHT_WARN);
	}
if (gameStates.app.cheats.bMadBuddy)
	DoBuddyDudeStuff ();
if (gameData.escortData.xSorryTime + I2X (1) > gameData.timeData.xGame) {
	gameData.escortData.xLastMsgTime = 0;	//	Force this message to get through.
	if (gameData.escortData.xSorryTime < gameData.timeData.xGame + I2X (2))
		BuddyMessage (TXT_BUDDY_SORRY);
	gameData.escortData.xSorryTime = -I2X (2);
	}
//	If buddy not allowed to talk, then he is locked in his room.  Make him mostly do nothing unless you're nearby.
if (!gameData.escortData.bMayTalk)
	if (xDistToPlayer > I2X (100))
		aip->SKIP_AI_COUNT = (int8_t) ((I2X (1) / 4) / (gameData.timeData.xFrame ? gameData.timeData.xFrame : 1));
//	AIM_WANDER has been co-opted for buddy behavior (didn't want to modify aistruct.h)
//	It means the CObject has been told to get lost and has come to the end of its path.
//	If the player is now visible, then create a path.
if (ailp->mode == AIM_WANDER)
	if (nPlayerVisibility) {
		CreateNSegmentPath (pObj, 16 + RandShort () * 16, -1);
		aip->nPathLength = SmoothPath (pObj, gameData.aiData.routeSegs + aip->nHideIndex, aip->nPathLength);
		}
if (gameData.escortData.nSpecialGoal == ESCORT_GOAL_SCRAM) {
	if (nPlayerVisibility)
		if (gameData.escortData.xLastPathCreated + I2X (3) < gameData.timeData.xGame) {
#if TRACE
			console.printf (CON_DBG, "Frame %i: Buddy creating new scram path.\n", gameData.appData.nFrameCount);
#endif
			CreateNSegmentPath (pObj, 10 + RandShort () * 16, gameData.objData.pConsole->info.nSegment);
			gameData.escortData.xLastPathCreated = gameData.timeData.xGame;
			}
	// -- Int3 ();
	RETURN
	}
//	Force checking for new goal every 5 seconds, and create new path, if necessary.
if (ailp->mode == AIM_IDLING &&
	(((gameData.escortData.nSpecialGoal != ESCORT_GOAL_SCRAM) && ((gameData.escortData.xLastPathCreated + I2X (5)) < gameData.timeData.xGame)) ||
	 ((gameData.escortData.nSpecialGoal == ESCORT_GOAL_SCRAM) && ((gameData.escortData.xLastPathCreated + I2X (15)) < gameData.timeData.xGame)))) {
	gameData.escortData.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	gameData.escortData.xLastPathCreated = gameData.timeData.xGame;
	}
if (0 && (gameData.escortData.nSpecialGoal != ESCORT_GOAL_SCRAM) && TimeToVisitPlayer (pObj, ailp, aip)) {
	int32_t	nMaxLen;

	Buddy_last_player_path_created = gameData.timeData.xGame;
	ailp->mode = AIM_GOTO_PLAYER;
	if (!nPlayerVisibility) {
		if ((Last_come_back_messageTime + I2X (1) < gameData.timeData.xGame) || (Last_come_back_messageTime > gameData.timeData.xGame)) {
			BuddyMessage (TXT_COMING_BACK);
			Last_come_back_messageTime = gameData.timeData.xGame;
			}
		}
	//	No point in Buddy creating very long path if he's not allowed to talk.  Really kills framerate.
	nMaxLen = gameData.escortData.nMaxLength;
	if (!gameData.escortData.bMayTalk)
		nMaxLen = 3;
	CreatePathToTarget (pObj, nMaxLen, 1);	//	MK!: Last parm used to be 1!
	if (aip->nPathLength > 0) {
		aip->nPathLength = SmoothPath (pObj, gameData.aiData.routeSegs + aip->nHideIndex, aip->nPathLength);
		ailp->mode = AIM_GOTO_PLAYER;
		}
	}
else if (gameData.timeData.xGame - xBuddyLastSeenPlayer > MAX_ESCORT_TIME_AWAY) {
	//	This is to prevent buddy from looking for a goal, which he will do because we only allow path creation once/second.
	RETURN
	}
else if ((ailp->mode == AIM_GOTO_PLAYER) && (xDistToPlayer < MIN_ESCORT_DISTANCE)) {
	gameData.escortData.nGoalObject = EscortSetGoalObject ();
	ailp->mode = AIM_GOTO_OBJECT;		//	May look stupid to be before path creation, but AIDoorIsOpenable uses mode to determine what doors can be got through
	EscortCreatePathToGoal (pObj);
	if (aip->nPathLength > 0) {
		aip->nPathLength = SmoothPath (pObj, gameData.aiData.routeSegs + aip->nHideIndex, aip->nPathLength);
		if (aip->nPathLength < 3)
			CreateNSegmentPath (pObj, 5, gameData.aiData.target.nBelievedSeg);
		ailp->mode = AIM_GOTO_OBJECT;
		}
	}
else if (gameData.escortData.nGoalObject == ESCORT_GOAL_UNSPECIFIED) {
	if ((ailp->mode != AIM_GOTO_PLAYER) || (xDistToPlayer < MIN_ESCORT_DISTANCE)) {
		gameData.escortData.nGoalObject = EscortSetGoalObject ();
		ailp->mode = AIM_GOTO_OBJECT;		//	May look stupid to be before path creation, but AIDoorIsOpenable uses mode to determine what doors can be got through
		EscortCreatePathToGoal (pObj);
		if (gameData.escortData.nGoalObject == ESCORT_GOAL_CONTROLCEN && aip->nPathLength)
			aip->nPathLength--;
		if (aip->nPathLength > 0) {
			aip->nPathLength = SmoothPath (pObj, gameData.aiData.routeSegs + aip->nHideIndex, aip->nPathLength);
			//if (aip->nPathLength < 3)
			//	CreateNSegmentPath (pObj, 5, gameData.aiData.target.nBelievedSeg);
			ailp->mode = AIM_GOTO_OBJECT;
			}
		else
			ailp->mode = AIM_IDLING;
		}
	}
else if (ailp->mode == AIM_IDLING && gameData.escortData.nGoalObject != ESCORT_GOAL_EXIT && 
	gameData.escortData.nGoalIndex >= 0) {
	CFixVector vNormToGoal = OBJECT(gameData.escortData.nGoalIndex)->Position () - pObj-> Position ();
	CFixVector::Normalize (vNormToGoal);
	AITurnTowardsVector (&vNormToGoal, pObj, ROBOTINFO (pObj)->turnTime [gameStates.app.nDifficultyLevel]);
	}
else if (ailp->mode == AIM_IDLING) {
	gameData.escortData.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
	}
RETURN
}

//	-------------------------------------------------------------------------------------------------

void InvalidateEscortGoal (void)
{
	gameData.escortData.nGoalObject = -1;
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
	CObject	*pObj;

FORALL_ROBOT_OBJS (pObj) {
	if (pObj->IsGuideBot ())
		break;
	}

if (!IS_OBJECT (pObj, pObj->Index ())) {
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

if (!gameData.escortData.bMayTalk) {
	HUDInitMessage (TXT_GB_RELEASE, gameData.escortData.szName);
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
if (gameData.escortData.nSpecialGoal == ESCORT_GOAL_SCRAM) {
	gameData.escortData.nSpecialGoal = -1;	//	Else setting next goal might fail.
	next_goal = EscortSetGoalObject ();
	gameData.escortData.nSpecialGoal = ESCORT_GOAL_SCRAM;
	}
else {
	gameData.escortData.nSpecialGoal = -1;	//	Else setting next goal might fail.
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

if (!gameData.escortData.bMsgsSuppressed)
	sprintf (tstr, TXT_GB_SUPPRESS);
else
	sprintf (tstr, TXT_GB_ENABLE);

i = ShowEscortHelp (szGoal, tstr);
if (i < 11) {
	gameData.escortData.bSearchingMarker = -1;
	gameData.escortData.nLastKey = -1;
	EscortSetSpecialGoal (i ? KEY_1 + i - 1 : KEY_0);
	gameData.escortData.nLastKey = -1;
	}
else if (i == 11) {
	BuddyMessage (gameData.escortData.bMsgsSuppressed ? TXT_GB_MSGS_ON : TXT_GB_MSGS_OFF);
	gameData.escortData.bMsgsSuppressed = !gameData.escortData.bMsgsSuppressed;
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
	uint8_t		nBuddyId;
	CFixVector	vObjPos;

for (nBuddyId = 0; nBuddyId < gameData.botData.nTypes [0]; nBuddyId++)
	if (gameData.botData.info [0][nBuddyId].companion)
		break;
if (nBuddyId == gameData.botData.nTypes [0]) {
#if TRACE
		console.printf (CON_DBG, "Can't create Buddy.  No 'companion' bot found in gameData.botData.pInfo!\n");
#endif
	return;
	}
vObjPos = SEGMENT (OBJSEG (gameData.objData.pConsole))->Center ();
CreateMorphRobot (SEGMENT (OBJSEG (gameData.objData.pConsole)), &vObjPos, nBuddyId);
}

//	-------------------------------------------------------------------------------
