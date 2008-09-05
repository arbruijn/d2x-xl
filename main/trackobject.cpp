#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include "inferno.h"
#include "error.h"
#include "network.h"
#include "render.h"
#include "omega.h"
#include "trackobject.h"

#define	OMEGA_MIN_TRACKABLE_DOT			 (15 * F1_0 / 16)		//	Larger values mean narrower cone.  F1_0 means damn near impossible.  0 means 180 degree field of view.
#define	OMEGA_MAX_TRACKABLE_DIST		MAX_OMEGA_DIST	//	An tObject must be at least this close to be tracked.

fix	xMinTrackableDot = MIN_TRACKABLE_DOT;

//	-----------------------------------------------------------------------------------------------------------
//	Return true if weapon *tracker is able to track tObject OBJECTS [nMslLock], else return false.
//	In order for the tObject to be trackable, it must be within a reasonable turning radius for the missile
//	and it must not be obstructed by a tWall.
int ObjectIsTrackeable (int nMslLock, tObject *tracker, fix *xDot)
{
	vmsVector	vGoal;
	tObject		*objP;

if (nMslLock == -1)
	return 0;

if (IsCoopGame)
	return 0;
objP = OBJECTS + nMslLock;
//	Don't track tPlayer if he's cloaked.
if ((nMslLock == LOCALPLAYER.nObject) && 
	 (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
	return 0;
//	Can't track AI tObject if he's cloaked.
if (objP->nType == OBJ_ROBOT) {
	if (objP->cType.aiInfo.CLOAKED)
		return 0;
	//	Your missiles don't track your escort.
	if (ROBOTINFO (objP->id).companion && 
		 (tracker->cType.laserInfo.parentType == OBJ_PLAYER))
		return 0;
	}
vGoal = objP->position.vPos - tracker->position.vPos;
vmsVector::Normalize(vGoal);
*xDot = vmsVector::Dot(vGoal, tracker->position.mOrient[FVEC]);
if ((*xDot < xMinTrackableDot) && (*xDot > 9 * F1_0 / 10)) {
	vmsVector::Normalize(vGoal);
	*xDot = vmsVector::Dot(vGoal, tracker->position.mOrient[FVEC]);
	}

if ((*xDot >= xMinTrackableDot) || 
	 (EGI_FLAG (bEnhancedShakers, 0, 0, 0) && (tracker->nType == OBJ_WEAPON) && (tracker->id == EARTHSHAKER_MEGA_ID) /*&& (*xDot >= 0)*/)) {
	//	xDot is in legal range, now see if tObject is visible
	return ObjectToObjectVisibility (tracker, objP, FQ_TRANSWALL);
	}
return 0;
}

//	--------------------------------------------------------------------------------------------

int CallFindHomingObjectComplete (tObject *tracker, vmsVector *curpos)
{
if (!IsMultiGame)
	return FindHomingObjectComplete (curpos, tracker, OBJ_ROBOT, -1);
if ((tracker->nType == OBJ_PLAYER) || (tracker->cType.laserInfo.parentType == OBJ_PLAYER)) {
	//	It's fired by a tPlayer, so if robots present, track robot, else track player.
	return IsCoopGame ? 
			 FindHomingObjectComplete (curpos, tracker, OBJ_ROBOT, -1) :
			 FindHomingObjectComplete (curpos, tracker, OBJ_PLAYER, OBJ_ROBOT);
		} 
#ifdef _DEBUG
if ((tracker->cType.laserInfo.parentType != OBJ_ROBOT) && (tracker->cType.laserInfo.parentType != OBJ_PLAYER))
	tracker = tracker;
#endif
return FindHomingObjectComplete (curpos, tracker, OBJ_PLAYER, gameStates.app.cheats.bRobotsKillRobots ? OBJ_ROBOT : -1);
}

//	--------------------------------------------------------------------------------------------
//	Find tObject to home in on.
//	Scan list of OBJECTS rendered last frame, find one that satisfies function of nearness to center and distance.
int FindHomingObject (vmsVector *vTrackerPos, tObject *trackerP)
{
	int	i, bOmega = (trackerP->nType == OBJ_WEAPON) && (trackerP->id == OMEGA_ID);
	fix	maxDot = -F1_0*2;
	int	nBestObj = -1;
	int	curMinTrackableDot;
	int	bGuidedMslView = (trackerP->nType == OBJ_WEAPON) && (trackerP == GuidedInMainView ());
	int	bSpectate = SPECTATOR (trackerP);

//	Contact Mike: This is a bad and stupid thing.  Who called this routine with an illegal laser nType??
Assert (gameStates.app.cheats.bHomingWeapons ||
		  (trackerP->nType == OBJ_PLAYER) || 
		  bGuidedMslView || 
		  WI_homingFlag (trackerP->id) || 
		  bOmega);

	//	Find an tObject to track based on game mode (eg, whether in network play) and who fired it.

if (IsMultiGame)
	return CallFindHomingObjectComplete (trackerP, vTrackerPos);

curMinTrackableDot = MIN_TRACKABLE_DOT;
if (bOmega)
	curMinTrackableDot = OMEGA_MIN_TRACKABLE_DOT;

//	Not in network mode.  If not fired by tPlayer, then track player.
if ((trackerP->nType != OBJ_PLAYER) && (trackerP->cType.laserInfo.nParentObj != LOCALPLAYER.nObject)) {
	if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
		nBestObj = OBJ_IDX (gameData.objs.console);
	} 
else {
		int	nWindow = -1;
		fix	dist, maxTrackableDist;

	//	Find the window which has the forward view.
	for (i = 0; i < MAX_RENDERED_WINDOWS; i++)
		if ((windowRenderedData [i].frame >= gameData.app.nFrameCount - 1) &&
			 ((windowRenderedData [i].viewer == gameData.objs.console) || bGuidedMslView) &&
			 !windowRenderedData [i].rearView) {
			nWindow = i;
			break;
			}

	//	Couldn't find suitable view from this frame, so do complete search.
	if (nWindow == -1)
		return CallFindHomingObjectComplete (trackerP, vTrackerPos);

	maxTrackableDist = MAX_TRACKABLE_DIST;
	if (EGI_FLAG (bEnhancedShakers, 0, 0, 0) && (trackerP->nType == OBJ_WEAPON) && (trackerP->id == EARTHSHAKER_MEGA_ID))
		maxTrackableDist *= 2;
	else if (bOmega)
		maxTrackableDist = OMEGA_MAX_TRACKABLE_DIST;

	//	Not in network mode and fired by player.
	for (i = windowRenderedData [nWindow].numObjects - 1; i >= 0; i--) {
		fix			dot; //, dist;
		vmsVector	vecToCurObj;
		int			nObject = windowRenderedData [nWindow].renderedObjects [i];
		tObject		*curObjP = OBJECTS + nObject;

		if (nObject == LOCALPLAYER.nObject)
			continue;

		//	Can't track AI tObject if he's cloaked.
		if (curObjP->nType == OBJ_ROBOT) {
			if (curObjP->cType.aiInfo.CLOAKED)
				continue;
			//	Your missiles don't track your escort.
			if (ROBOTINFO (curObjP->id).companion && 
				 ((trackerP->nType == OBJ_PLAYER) || (trackerP->cType.laserInfo.parentType == OBJ_PLAYER)))
				continue;
			}
		else if (curObjP->nType == OBJ_WEAPON) {
			if (!EGI_FLAG (bKillMissiles, 0, 1, 0) || !bOmega || !(gameData.objs.bIsMissile [curObjP->id] || WeaponIsMine (curObjP->id)))
				continue;
			}
		else if ((curObjP->nType != OBJ_PLAYER) && (curObjP->nType != OBJ_REACTOR))
			continue;
		vecToCurObj = curObjP->position.vPos - *vTrackerPos;
		dist = vmsVector::Normalize(vecToCurObj);
		if (dist < maxTrackableDist) {
			dot = vmsVector::Dot(vecToCurObj, bSpectate ? gameStates.app.playerPos.mOrient[FVEC] : trackerP->position.mOrient[FVEC]);

			//	Note: This uses the constant, not-scaled-by-frametime value, because it is only used
			//	to determine if an tObject is initially trackable.  FindHomingObject is called on subsequent
			//	frames to determine if the tObject remains trackable.
			if (dot > curMinTrackableDot) {
				if (dot > maxDot) {
					if (ObjectToObjectVisibility (trackerP, OBJECTS + nObject, FQ_TRANSWALL)) {
						maxDot = dot;
						nBestObj = nObject;
						}
					}
				} 
			else if (dot > F1_0 - (F1_0 - curMinTrackableDot) * 2) {
				vmsVector::Normalize(vecToCurObj);
				dot = vmsVector::Dot(vecToCurObj, trackerP->position.mOrient[FVEC]);
				if (dot > curMinTrackableDot) {
					if (dot > maxDot) {
						if (ObjectToObjectVisibility (trackerP, OBJECTS + nObject, FQ_TRANSWALL)) {
							maxDot = dot;
							nBestObj = nObject;
							}
						}
					}
				}
			}
		}
	}
return nBestObj;
}

//	--------------------------------------------------------------------------------------------
//	Find tObject to home in on.
//	Scan list of OBJECTS rendered last frame, find one that satisfies function of nearness to center and distance.
//	Can track two kinds of OBJECTS.  If you are only interested in one nType, set track_objType2 to NULL
//	Always track proximity bombs.  --MK, 06/14/95
//	Make homing OBJECTS not track parent's prox bombs.
int FindHomingObjectComplete (vmsVector *curpos, tObject *trackerP, int track_objType1, int track_objType2)
{
	int	nObject;
	fix	maxDot = -F1_0*2;
	int	nBestObj = -1;
	fix	maxTrackableDist;
	fix	minTrackableDot;

	//	Contact Mike: This is a bad and stupid thing.  Who called this routine with an illegal laser nType??
//Assert ((WI_homingFlag (trackerP->id)) || (trackerP->id == OMEGA_ID));

maxTrackableDist = MAX_TRACKABLE_DIST;
if (EGI_FLAG (bEnhancedShakers, 0, 0, 0) && (trackerP->nType == OBJ_WEAPON) && (trackerP->id == EARTHSHAKER_MEGA_ID)) {
	maxTrackableDist *= 2;
	minTrackableDot = -F1_0;
	}
else
#if 0//def _DEBUG
	minTrackableDot = -F1_0;
#else
	minTrackableDot = MIN_TRACKABLE_DOT;
#endif

if ((trackerP->nType == OBJ_WEAPON) && (trackerP->id == OMEGA_ID)) {
	maxTrackableDist = OMEGA_MAX_TRACKABLE_DIST;
	minTrackableDot = OMEGA_MIN_TRACKABLE_DOT;
	}

for (nObject = 0; nObject <= gameData.objs.nLastObject [0]; nObject++) {
	int			bIsProximity = 0;
	fix			dot, dist;
	vmsVector	vecToCurObj;
	tObject		*curObjP = OBJECTS + nObject;

	if ((curObjP->nType != track_objType1) && (curObjP->nType != track_objType2)) {
		if (curObjP->nType != OBJ_WEAPON) 
			continue;
		if (!WeaponIsPlayerMine (curObjP->id))
			continue;
		if (curObjP->cType.laserInfo.nParentSig == trackerP->cType.laserInfo.nParentSig)
			continue;
		bIsProximity = 1;
		}
	if (nObject == trackerP->cType.laserInfo.nParentObj) // Don't track shooter
		continue;

	//	Don't track cloaked players.
	if (curObjP->nType == OBJ_PLAYER) {
		if (gameData.multiplayer.players [curObjP->id].flags & PLAYER_FLAGS_CLOAKED)
			continue;
		// Don't track teammates in team games
		if (IsTeamGame && (OBJECTS [trackerP->cType.laserInfo.nParentObj].nType == OBJ_PLAYER) && (GetTeam (curObjP->id) == GetTeam (OBJECTS [trackerP->cType.laserInfo.nParentObj].id)))
			continue;
		}

	//	Can't track AI tObject if he's cloaked.
	if ((curObjP->nType == OBJ_ROBOT) && (curObjP->cType.aiInfo.CLOAKED))
		continue;
	//	Your missiles don't track your escort.
	if ((ROBOTINFO (curObjP->id).companion) && (trackerP->cType.laserInfo.parentType == OBJ_PLAYER))
		continue;

	vecToCurObj = curObjP->position.vPos - *curpos;
	dist = vecToCurObj.Mag();

	if (dist < maxTrackableDist) {
		vmsVector::Normalize(vecToCurObj);
		
		dot = vmsVector::Dot(vecToCurObj, trackerP->position.mOrient[FVEC]);
		if (bIsProximity)
			dot = ((dot << 3) + dot) >> 3;		//	I suspect Watcom would be too stupid to figure out the obvious...

		//	Note: This uses the constant, not-scaled-by-frametime value, because it is only used
		//	to determine if an tObject is initially trackable.  FindHomingObject is called on subsequent
		//	frames to determine if the tObject remains trackable.
		if ((dot > minTrackableDot) && (dot > maxDot) &&
			 (ObjectToObjectVisibility (trackerP, OBJECTS + nObject, FQ_TRANSWALL))) {
			maxDot = dot;
			nBestObj = nObject;
			}
		}
	}
	return nBestObj;
}

//	------------------------------------------------------------------------------------------------------------
//	See if legal to keep tracking currently tracked tObject.  If not, see if another tObject is trackable.  If not, return -1,
//	else return tObject number of tracking tObject.
//	Computes and returns a fairly precise dot product.
int TrackMslLock (int nMslLock, tObject *tracker, fix *dot)
{
	int	rVal = -2;
	int	nFrame;
	int	goalType, goal2Type = -1;

//if (!gameOpts->legacy.bHomers && gameStates.limitFPS.bHomers && !gameStates.app.tick40fps.bTick)
	//	Every 8 frames for each tObject, scan all OBJECTS.
nFrame = OBJ_IDX (tracker) ^ gameData.app.nFrameCount;
if (ObjectIsTrackeable (nMslLock, tracker, dot)) {
	if (gameOpts->legacy.bHomers) {
		if (nFrame % 8)
			return nMslLock;
		}
	else {
		if (gameStates.limitFPS.bHomers && !gameStates.app.tick40fps.bTick)
			return nMslLock;
		}
	}

if (!gameOpts->legacy.bHomers || (nFrame % 4 == 0)) {
	//	If tPlayer fired missile, then search for an tObject, if not, then give up.
	if (OBJECTS [tracker->cType.laserInfo.nParentObj].nType == OBJ_PLAYER) {
		if (nMslLock == -1) {
			if (IsMultiGame) {
				if (IsCoopGame)
					rVal = FindHomingObjectComplete (&tracker->position.vPos, tracker, OBJ_ROBOT, -1);
				else if (gameData.app.nGameMode & GM_MULTI_ROBOTS)		//	Not cooperative, if robots, track either robot or tPlayer
					rVal = FindHomingObjectComplete (&tracker->position.vPos, tracker, OBJ_PLAYER, OBJ_ROBOT);
				else		//	Not cooperative and no robots, track only a tPlayer
					rVal = FindHomingObjectComplete (&tracker->position.vPos, tracker, OBJ_PLAYER, -1);
				}
			else
				rVal = FindHomingObjectComplete (&tracker->position.vPos, tracker, OBJ_PLAYER, OBJ_ROBOT);
			} 
		else {
			goalType = OBJECTS [tracker->cType.laserInfo.nMslLock].nType;
			if ((goalType == OBJ_PLAYER) || (goalType == OBJ_ROBOT))
				rVal = FindHomingObjectComplete (&tracker->position.vPos, tracker, goalType, -1);
			else
				rVal = -1;
			}
		} 
	else {
		if (gameStates.app.cheats.bRobotsKillRobots)
			goal2Type = OBJ_ROBOT;
		if (nMslLock == -1)
			rVal = FindHomingObjectComplete (&tracker->position.vPos, tracker, OBJ_PLAYER, goal2Type);
		else {
			goalType = OBJECTS [tracker->cType.laserInfo.nMslLock].nType;
			rVal = FindHomingObjectComplete (&tracker->position.vPos, tracker, goalType, goal2Type);
			}
		}
	Assert (rVal != -2);		//	This means it never got set which is bad! Contact Mike.
	return rVal;
	}
return -1;
}

//	-----------------------------------------------------------------------------------------------------------
