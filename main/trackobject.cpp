#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include "descent.h"
#include "error.h"
#include "network.h"
#include "rendermine.h"
#include "omega.h"
#include "visibility.h"
#include "trackobject.h"

#define	OMEGA_MIN_TRACKABLE_DOT			 (I2X (15) / 16)		//	Larger values mean narrower cone.  I2X (1) means damn near impossible.  0 means 180 degree field of view.
#define	OMEGA_MAX_TRACKABLE_DIST		MAX_OMEGA_DIST	//	An CObject must be at least this close to be tracked.

fix	xMinTrackableDot = MIN_TRACKABLE_DOT;

#define NEW_TARGETTING 1

#if NEW_TARGETTING

class CTarget {
	public:
		fix		m_xDot;
		CObject*	m_objP;

		CTarget (fix xDot = 0, CObject* objP = NULL) : m_xDot(xDot), m_objP(objP) {}

		inline bool operator< (CTarget& other) { return m_xDot < other.m_xDot; }
		inline bool operator> (CTarget& other) { return m_xDot > other.m_xDot; }
		inline bool operator<= (CTarget& other) { return m_xDot <= other.m_xDot; }
		inline bool operator>= (CTarget& other) { return m_xDot >= other.m_xDot; }
	};

static CStack<class CTarget>	targetLists [MAX_THREADS];

#endif

//	-----------------------------------------------------------------------------------------------------------
//	Return true if weapon *trackerP is able to track CObject OBJECTS [nHomingTarget], else return false.
//	In order for the CObject to be trackable, it must be within a reasonable turning radius for the missile
//	and it must not be obstructed by a CWall.
int ObjectIsTrackeable (int nHomingTarget, CObject *trackerP, fix *xDot)
{
	CFixVector	vGoal;
	CObject		*objP;

if (nHomingTarget == -1)
	return 0;
if (IsCoopGame)
	return 0;
objP = OBJECTS + nHomingTarget;
//	Don't track CPlayerData if he's cloaked.
if ((nHomingTarget == LOCALPLAYER.nObject) && (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
	return 0;
//	Can't track AI CObject if he's cloaked.
if (objP->info.nType == OBJ_ROBOT) {
	if (objP->cType.aiInfo.CLOAKED)
		return 0;
	//	Your missiles don't track your escort.
	if (ROBOTINFO (objP->info.nId).companion && 
		 (trackerP->cType.laserInfo.parent.nType == OBJ_PLAYER))
		return 0;
	}
vGoal = objP->info.position.vPos - trackerP->info.position.vPos;
CFixVector::Normalize (vGoal);
*xDot = CFixVector::Dot (vGoal, trackerP->info.position.mOrient.m.dir.f);
if ((*xDot < xMinTrackableDot) && (*xDot > I2X (9) / 10)) {
	CFixVector::Normalize (vGoal);
	*xDot = CFixVector::Dot (vGoal, trackerP->info.position.mOrient.m.dir.f);
	}

if ((*xDot >= xMinTrackableDot) || 
	 (EGI_FLAG (bEnhancedShakers, 0, 0, 0) && (trackerP->info.nType == OBJ_WEAPON) && (trackerP->info.nId == EARTHSHAKER_MEGA_ID) /*&& (*xDot >= 0)*/)) {
	//	xDot is in legal range, now see if CObject is visible
	return ObjectToObjectVisibility (trackerP, objP, FQ_TRANSWALL);
	}
return 0;
}

//	--------------------------------------------------------------------------------------------

int CallUpdateHomingTarget (CObject *trackerP, CFixVector *vCurPos)
{
if (!IsMultiGame)
	return UpdateHomingTarget (vCurPos, trackerP, OBJ_ROBOT, -1);
if ((trackerP->info.nType == OBJ_PLAYER) || (trackerP->cType.laserInfo.parent.nType == OBJ_PLAYER)) {
	//	It's fired by a player, so if robots present, track robot, else track player.
	return IsCoopGame 
			 ? UpdateHomingTarget (vCurPos, trackerP, OBJ_ROBOT, -1) 
			 : UpdateHomingTarget (vCurPos, trackerP, OBJ_PLAYER, OBJ_ROBOT);
		} 
CObject* parentP = trackerP->Parent ();
return UpdateHomingTarget (vCurPos, trackerP, OBJ_PLAYER, (parentP && (parentP->Target ()->Type () == OBJ_ROBOT)) ? OBJ_ROBOT : -1);
}

//	--------------------------------------------------------------------------------------------

static inline int FindTargetWindow (CObject* trackerP)
{
for (int i = 0; i < MAX_RENDERED_WINDOWS; i++)
	if ((windowRenderedData [i].nFrame >= gameData.app.nFrameCount - 1) &&
		 ((windowRenderedData [i].viewerP == gameData.objs.consoleP) || (trackerP == GuidedInMainView ())) &&
		 !windowRenderedData [i].bRearView) {
		return i;
		}
return -1;
}

//	--------------------------------------------------------------------------------------------

static inline int MaxTrackableDist (CObject* trackerP, int& xBestDot)
{
if (trackerP->Type () == OBJ_WEAPON) {
	if (trackerP->Id () == OMEGA_ID) {
		xBestDot = OMEGA_MIN_TRACKABLE_DOT;
		return OMEGA_MAX_TRACKABLE_DIST;
		}
	if (trackerP->Id () != EARTHSHAKER_MEGA_ID)
		return MAX_TRACKABLE_DIST;
	if (EGI_FLAG (bEnhancedShakers, 0, 0, 0))
		return MAX_TRACKABLE_DIST * 2;
	}
xBestDot = MIN_TRACKABLE_DOT;
return MAX_TRACKABLE_DIST;
}

//	--------------------------------------------------------------------------------------------
//	Find CObject to home in on.
//	Scan list of OBJECTS rendered last frame, find one that satisfies function of nearness to center and distance.
int FindHomingTarget (CFixVector *vTrackerPos, CObject *trackerP)
{
	int nBestObj = -1;

//	Find an CObject to track based on game mode (eg, whether in network play) and who fired it.
if (IsMultiGame)
	return CallUpdateHomingTarget (trackerP, vTrackerPos);

//	Not in network mode.  If not fired by player, then track player.
if ((trackerP->info.nType != OBJ_PLAYER) && (trackerP->cType.laserInfo.parent.nObject != LOCALPLAYER.nObject) && !(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
	return OBJ_IDX (gameData.objs.consoleP);

int nWindow = FindTargetWindow (trackerP);
if (nWindow == -1)
	return CallUpdateHomingTarget (trackerP, vTrackerPos);

fix xBestDot;
fix maxTrackableDist = MaxTrackableDist (trackerP, xBestDot);
CFixVector vTracker = SPECTATOR (trackerP) ? gameStates.app.playerPos.mOrient.m.dir.f : trackerP->info.position.mOrient.m.dir.f;
bool bOmega = (trackerP->Type () == OBJ_WEAPON) && (trackerP->Id () == OMEGA_ID);

//	Not in multiplayer mode and fired by player.
for (int i = windowRenderedData [nWindow].nObjects - 1; i >= 0; i--) {
	short nObject = windowRenderedData [nWindow].renderedObjects [i];
	if (nObject == LOCALPLAYER.nObject)
		continue;
	CObject* targetP = OBJECTS + nObject;
	//	Can't track AI CObject if he's cloaked.
	if (targetP->info.nType == OBJ_ROBOT) {
		if (targetP->cType.aiInfo.CLOAKED)
			continue;
		//	Your missiles don't track your escort.
		if (ROBOTINFO (targetP->info.nId).companion && 
			 ((trackerP->info.nType == OBJ_PLAYER) || (trackerP->cType.laserInfo.parent.nType == OBJ_PLAYER)))
			continue;
		}
	else if (targetP->info.nType == OBJ_WEAPON) {
		if (!(bOmega && EGI_FLAG (bKillMissiles, 0, 1, 0) && bOmega && (targetP->IsMissile () || targetP->IsMine ())))
			continue;
		}
	else if ((targetP->info.nType != OBJ_PLAYER) && (targetP->info.nType != OBJ_REACTOR))
		continue;
	CFixVector vTarget = targetP->info.position.vPos - *vTrackerPos;
	fix dist = CFixVector::Normalize (vTarget);
	if (dist < maxTrackableDist) {
		fix dot = CFixVector::Dot (vTarget, vTracker);

		//	Note: This uses the constant, not-scaled-by-frametime value, because it is only used
		//	to determine if an CObject is initially trackable.  FindHomingTarget is called on subsequent
		//	frames to determine if the CObject remains trackable.
		if ((dot > xBestDot) && (ObjectToObjectVisibility (trackerP, targetP, FQ_TRANSWALL))) {
			xBestDot = dot;
			nBestObj = nObject;
			} 
		}
	}
return nBestObj;
}

//	--------------------------------------------------------------------------------------------
//	Find CObject to home in on.
//	Scan list of OBJECTS rendered last frame, find one that satisfies function of nearness to center and distance.
//	Can track two kinds of OBJECTS.  If you are only interested in one nType, set targetType2 to NULL
//	Always track proximity bombs.  --MK, 06/14/95
//	Make homing OBJECTS not track parent's prox bombs.

int UpdateHomingTarget (CFixVector *vCurPos, CObject *trackerP, int targetType1, int targetType2, int nThread)
{
#if !NEW_TARGETTING
	int		nBestObj = -1;
#else
CStack<class CTarget>& targets = targetLists [nThread];
targets.SetGrowth (100);
targets.Reset ();
#endif

fix xBestDot;
fix maxTrackableDist = MaxTrackableDist (trackerP, xBestDot);
CObject* targetP;
CFixVector vTracker = SPECTATOR (trackerP) ? gameStates.app.playerPos.mOrient.m.dir.f : trackerP->info.position.mOrient.m.dir.f;
bool bOmega = (trackerP->Type () == OBJ_WEAPON) && (trackerP->Id () == OMEGA_ID);

FORALL_ACTOR_OBJS (targetP, nObject) {
	int			bIsProximity = 0;
	fix			dot, dist;
	CFixVector	vTarget;
	int			nType = targetP->info.nType;

	if (OBJ_IDX (targetP) == trackerP->cType.laserInfo.parent.nObject) // Don't track shooter
		continue;
	if (nType == OBJ_WEAPON) {
		if (targetP->cType.laserInfo.parent.nSignature == trackerP->cType.laserInfo.parent.nSignature)
			continue; // target was created by tracker (e.g. a mine dropped by the tracker object)
		if (targetP->IsPlayerMine ())
			bIsProximity = 1;
		else if (!(bOmega && EGI_FLAG (bKillMissiles, 0, 1, 0) && (targetP->IsMissile () || targetP->IsMine ())))
			continue;
		}
	else if ((nType != targetType1) && (nType != targetType2) && (nType != OBJ_MONSTERBALL)) {
		continue;
		}
	
	else if (targetP->info.nType == OBJ_PLAYER) {
		if (gameData.multiplayer.players [targetP->info.nId].flags & PLAYER_FLAGS_CLOAKED)
			continue; // don't track cloaked players.
		if (IsTeamGame) {
			CObject *parentObjP = OBJECTS + trackerP->cType.laserInfo.parent.nObject;
			if ((parentObjP->info.nType == OBJ_PLAYER) && (GetTeam (targetP->info.nId) == GetTeam (parentObjP->info.nId)))
				continue; // don't track teammates in team games
			}
		}
	else if (targetP->info.nType == OBJ_ROBOT) {
		if (targetP->cType.aiInfo.CLOAKED)
			continue; // don' track cloaked robots
		if (ROBOTINFO (targetP->info.nId).companion && (trackerP->cType.laserInfo.parent.nType == OBJ_PLAYER))
			continue;	//	player missiles don't track the guidebot.
		}

	vTarget = targetP->info.position.vPos - *vCurPos;
	dist = CFixVector::Normalize (vTarget);
	if (dist >= maxTrackableDist)
		continue;
	dot = CFixVector::Dot (vTarget, vTracker);
	if (bIsProximity)
		dot += dot / 8;		//	I suspect Watcom would be too stupid to figure out the obvious...
	if (dot < xBestDot)
		continue;
#if NEW_TARGETTING
#	if 1
	targets.Push (CTarget (dot, targetP));
#	else
	targets.Push (CTarget (fix (dist * (1.0f - X2F (dot) / 2.0f)), targetP));
#	endif
#else
	//	Note: This uses the constant, not-scaled-by-frametime value, because it is only used
	//	to determine if an CObject is initially trackable.  FindHomingTarget is called on subsequent
	//	frames to determine if the CObject remains trackable.
	if (!ObjectToObjectVisibility (trackerP, targetP, FQ_TRANSWALL))
		continue;

	xBestDot = dot;
	nBestObj = OBJ_IDX (targetP);
#endif
	}

#if NEW_TARGETTING
if (targets.ToS () < 1)
	return -1;
if (targets.ToS () > 1)
	targets.SortDescending ();

for (uint i = 0; i < targets.ToS (); i++) {
	if (ObjectToObjectVisibility (trackerP, targets[i].m_objP, FQ_TRANSWALL))
		return targets[i].m_objP->Index ();
	}
return -1;
#else
return nBestObj;
#endif
}

//	------------------------------------------------------------------------------------------------------------
//	See if legal to keep tracking currently tracked CObject.  If not, see if another CObject is trackable.  If not, return -1,
//	else return CObject number of tracking CObject.
//	Computes and returns a fairly precise dot product.
int TrackHomingTarget (int nHomingTarget, CObject *trackerP, fix *dot, int nThread)
{
	int	rVal = -2;
	int	nFrame;
	int	goalType, goal2Type = -1;

//if (!gameOpts->legacy.bHomers && gameStates.limitFPS.bHomers && !gameStates.app.tick40fps.bTick)
	//	Every 8 frames for each CObject, scan all OBJECTS.
nFrame = OBJ_IDX (trackerP) ^ gameData.app.nFrameCount;
if (ObjectIsTrackeable (nHomingTarget, trackerP, dot)) {
	if (gameOpts->legacy.bHomers) {
		if (nFrame % 8)
			return nHomingTarget;
		}
	else {
		if (gameStates.limitFPS.bHomers && !gameStates.app.tick40fps.bTick)
			return nHomingTarget;
		}
	}

if (!gameOpts->legacy.bHomers || (nFrame % 4 == 0)) {
	//	If CPlayerData fired missile, then search for an CObject, if not, then give up.
	if (OBJECTS [trackerP->cType.laserInfo.parent.nObject].info.nType == OBJ_PLAYER) {
		if (nHomingTarget == -1) {
			if (IsMultiGame) {
				if (IsCoopGame)
					rVal = UpdateHomingTarget (&trackerP->info.position.vPos, trackerP, OBJ_ROBOT, -1, nThread);
				else if (gameData.app.nGameMode & GM_MULTI_ROBOTS)		//	Not cooperative, if robots, track either robot or CPlayerData
					rVal = UpdateHomingTarget (&trackerP->info.position.vPos, trackerP, OBJ_PLAYER, OBJ_ROBOT, nThread);
				else		//	Not cooperative and no robots, track only a CPlayerData
					rVal = UpdateHomingTarget (&trackerP->info.position.vPos, trackerP, OBJ_PLAYER, -1, nThread);
				}
			else
				rVal = UpdateHomingTarget (&trackerP->info.position.vPos, trackerP, OBJ_PLAYER, OBJ_ROBOT, nThread);
			} 
		else {
			goalType = OBJECTS [trackerP->cType.laserInfo.nHomingTarget].info.nType;
			if ((goalType == OBJ_PLAYER) || (goalType == OBJ_ROBOT) || (goalType == OBJ_MONSTERBALL))
				rVal = UpdateHomingTarget (&trackerP->info.position.vPos, trackerP, goalType, -1, nThread);
			else
				rVal = -1;
			}
		} 
	else {
		if (gameStates.app.cheats.bRobotsKillRobots || (trackerP->Parent () && (trackerP->Parent ()->Target ()->Type () == OBJ_ROBOT)))
			goal2Type = OBJ_ROBOT;
		if (nHomingTarget == -1)
			rVal = UpdateHomingTarget (&trackerP->info.position.vPos, trackerP, OBJ_PLAYER, goal2Type, nThread);
		else {
			goalType = OBJECTS [trackerP->cType.laserInfo.nHomingTarget].info.nType;
			rVal = UpdateHomingTarget (&trackerP->info.position.vPos, trackerP, goalType, goal2Type, nThread);
			}
		}
	Assert (rVal != -2);		//	This means it never got set which is bad! Contact Mike.
	return rVal;
	}
return -1;
}

//	-----------------------------------------------------------------------------------------------------------
