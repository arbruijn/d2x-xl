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
//	Return true if weapon *trackerP is able to track CObject OBJECTS [nTarget], else return false.
//	In order for the CObject to be trackable, it must be within a reasonable turning radius for the missile
//	and it must not be obstructed by a CWall.
int CObject::ObjectIsTrackable (int nTarget, fix& xDot)
{
if (nTarget == -1)
	return 0;
if (IsCoopGame)
	return 0;
CObject* targetP = OBJECTS + nTarget;
//	Don't track CPlayerData if he's cloaked.
if ((nTarget == LOCALPLAYER.nObject) && (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
	return 0;
//	Can't track AI CObject if he's cloaked.
if (targetP->Type () == OBJ_ROBOT) {
	if (targetP->cType.aiInfo.CLOAKED)
		return 0;
	//	Your missiles don't track your escort.
	if (ROBOTINFO (targetP->Id ()).companion && 
		 (cType.laserInfo.parent.nType == OBJ_PLAYER))
		return 0;
	}
CFixVector vTracker = SPECTATOR (trackerP) ? gameStates.app.playerPos.mOrient.m.dir.f : info.position.mOrient.m.dir.f;
CFixVector vTarget = targetP->info.position.vPos - info.position.vPos;
CFixVector::Normalize (vTarget);
xDot = CFixVector::Dot (vTarget, vTracker);
if ((xDot < xMinTrackableDot) && (xDot > I2X (9) / 10)) {
	CFixVector::Normalize (vTarget);
	xDot = CFixVector::Dot (vTarget, vTracker);
	}

if ((xDot >= xMinTrackableDot) || 
	 (EGI_FLAG (bEnhancedShakers, 0, 0, 0) && (Type () == OBJ_WEAPON) && (Id () == EARTHSHAKER_MEGA_ID) /*&& (xDot >= 0)*/)) {
	//	xDot is in legal range, now see if CObject is visible
	return ObjectToObjectVisibility (targetP, FQ_TRANSWALL);
	}
return 0;
}

//	--------------------------------------------------------------------------------------------

int CObject::SelectHomingTarget (CObject *CFixVector *vCurPos)
{
if (!IsMultiGame)
	return FindAnyHomingTarget (vCurPos, OBJ_ROBOT, -1);
if ((Type () == OBJ_PLAYER) || (cType.laserInfo.parent.nType == OBJ_PLAYER)) {
	//	It's fired by a player, so if robots present, track robot, else track player.
	return IsCoopGame 
			 ? FindAnyHomingTarget (vCurPos, OBJ_ROBOT, -1) 
			 : FindAnyHomingTarget (vCurPos, OBJ_PLAYER, OBJ_ROBOT);
		} 
CObject* parentP = Parent ();
return FindAnyHomingTarget (vCurPos, OBJ_PLAYER, (parentP && (parentP->Target ()->Type () == OBJ_ROBOT)) ? OBJ_ROBOT : -1);
}

//	--------------------------------------------------------------------------------------------

int CObject::FindTargetWindow (void)
{
for (int i = 0; i < MAX_RENDERED_WINDOWS; i++)
	if ((windowRenderedData [i].nFrame >= gameData.app.nFrameCount - 1) &&
		 ((windowRenderedData [i].viewerP == gameData.objs.consoleP) || (this == GuidedInMainView ())) &&
		 !windowRenderedData [i].bRearView) {
		return i;
		}
return -1;
}

//	--------------------------------------------------------------------------------------------

int CObject::MaxTrackableDist (int& xBestDot)
{
if (Type () == OBJ_WEAPON) {
	if (Id () == OMEGA_ID) {
		xBestDot = OMEGA_MIN_TRACKABLE_DOT;
		return OMEGA_MAX_TRACKABLE_DIST;
		}
	if (Id () != EARTHSHAKER_MEGA_ID)
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
int CObject::FindVisibleHomingTarget (CFixVector *vTrackerPos)
{
	int nBestObj = -1;

//	Find an CObject to track based on game mode (eg, whether in network play) and who fired it.
if (IsMultiGame)
	return SelectHomingTarget (vTrackerPos);

//	Not in network mode.  If not fired by player, then track player.
if ((Type () != OBJ_PLAYER) && (cType.laserInfo.parent.nObject != LOCALPLAYER.nObject) && !(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
	return OBJ_IDX (gameData.objs.consoleP);

int nWindow = FindTargetWindow (;
if (nWindow == -1)
l	return SelectHomingTarget (vTrackerPos);

fix xBestDot;
fix maxTrackableDist = MaxTrackableDist (xBestDot);
CFixVector vTracker = SPECTATOR (trackerP) ? gameStates.app.playerPos.mOrient.m.dir.f : info.position.mOrient.m.dir.f;
bool bOmega = (Type () == OBJ_WEAPON) && (Id () == OMEGA_ID);
bool bPlayer = (Type () == OBJ_PLAYER);

//	Not in multiplayer mode and fired by player.
for (int i = windowRenderedData [nWindow].nObjects - 1; i >= 0; i--) {
	short nObject = windowRenderedData [nWindow].renderedObjects [i];
	if (nObject == LOCALPLAYER.nObject)
		continue;
	CObject* targetP = OBJECTS + nObject;
	//	Can't track AI CObject if he's cloaked.
	if (targetP->Type () == OBJ_ROBOT) {
		if (targetP->cType.aiInfo.CLOAKED)
			continue;
		//	Your missiles don't track your escort.
		if (ROBOTINFO (targetP->Id ()).companion && 
			 (bPlayer || (cType.laserInfo.parent.nType == OBJ_PLAYER)))
			continue;
		}
	else if (targetP->Type () == OBJ_WEAPON) {
		if (!(bOmega && EGI_FLAG (bKillMissiles, 0, 1, 0) && bOmega && (targetP->IsMissile () || targetP->IsMine ())))
			continue;
		}
	else if ((targetP->Type () != OBJ_PLAYER) && (targetP->Type () != OBJ_REACTOR))
		continue;
	CFixVector vTarget = targetP->info.position.vPos - *vTrackerPos;
	fix dist = CFixVector::Normalize (vTarget);
	if (dist < maxTrackableDist) {
		fix dot = CFixVector::Dot (vTarget, vTracker);

		//	Note: This uses the constant, not-scaled-by-frametime value, because it is only used
		//	to determine if an CObject is initially trackable.  FindHomingTarget is called on subsequent
		//	frames to determine if the CObject remains trackable.
		if ((dot > xBestDot) && (ObjectToObjectVisibility (targetP, FQ_TRANSWALL))) {
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

int CObject::FindAnyHomingTarget (CFixVector *vCurPos, int targetType1, int targetType2, int nThread)
{
#if !NEW_TARGETTING
	int		nBestObj = -1;
#else
CStack<class CTarget>& targets = targetLists [nThread];
targets.SetGrowth (100);
targets.Reset ();
#endif

fix xBestDot;
fix maxTrackableDist = MaxTrackableDist (xBestDot);
CObject* targetP;
CFixVector vTracker = SPECTATOR (trackerP) ? gameStates.app.playerPos.mOrient.m.dir.f : info.position.mOrient.m.dir.f;
bool bOmega = (Type () == OBJ_WEAPON) && (Id () == OMEGA_ID);

FORALL_ACTOR_OBJS (targetP, nObject) {
	int			bIsProximity = 0;
	fix			dot, dist;
	CFixVector	vTarget;
	int			nType = targetP->Type ();

	if (OBJ_IDX (targetP) == cType.laserInfo.parent.nObject) // Don't track shooter
		continue;
	if (nType == OBJ_WEAPON) {
		if (targetP->cType.laserInfo.parent.nSignature == cType.laserInfo.parent.nSignature)
			continue; // target was created by tracker (e.g. a mine dropped by the tracker object)
		if (targetP->IsPlayerMine ())
			bIsProximity = 1;
		else if (!(bOmega && EGI_FLAG (bKillMissiles, 0, 1, 0) && (targetP->IsMissile () || targetP->IsMine ())))
			continue;
		}
	else if ((nType != targetType1) && (nType != targetType2) && (nType != OBJ_MONSTERBALL)) {
		continue;
		}
	
	else if (targetP->Type () == OBJ_PLAYER) {
		if (gameData.multiplayer.players [targetP->Id ()].flags & PLAYER_FLAGS_CLOAKED)
			continue; // don't track cloaked players.
		if (IsTeamGame) {
			CObject *parentObjP = OBJECTS + cType.laserInfo.parent.nObject;
			if ((parentObjP->Type () == OBJ_PLAYER) && (GetTeam (targetP->Id ()) == GetTeam (parentObjP->Id ())))
				continue; // don't track teammates in team games
			}
		}
	else if (targetP->Type () == OBJ_ROBOT) {
		if (targetP->cType.aiInfo.CLOAKED)
			continue; // don' track cloaked robots
		if (ROBOTINFO (targetP->Id ()).companion && (cType.laserInfo.parent.nType == OBJ_PLAYER))
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
	if (!ObjectToObjectVisibility (targetP, FQ_TRANSWALL))
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
	if (ObjectToObjectVisibility (targets[i].m_objP, FQ_TRANSWALL))
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
int UpdateHomingTarget (int nTarget, fix& dot, int nThread)
{
	int	rVal = -2;
	int	nFrame;
	int	goalType, goal2Type = -1;

//if (!gameOpts->legacy.bHomers && gameStates.limitFPS.bHomers && !gameStates.app.tick40fps.bTick)
	//	Every 8 frames for each CObject, scan all OBJECTS.
nFrame = OBJ_IDX (trackerP) ^ gameData.app.nFrameCount;
if (ObjectIsTrackable (nTarget, dot)) {
	if (gameOpts->legacy.bHomers) {
		if (nFrame % 8)
			return nTarget;
		}
	else {
		if (gameStates.limitFPS.bHomers && !gameStates.app.tick40fps.bTick)
			return nTarget;
		}
	}

if (!gameOpts->legacy.bHomers || (nFrame % 4 == 0)) {
	//	If CPlayerData fired missile, then search for an CObject, if not, then give up.
	if (OBJECTS [cType.laserInfo.parent.nObject].Type () == OBJ_PLAYER) {
		if (nTarget == -1) {
			if (IsMultiGame) {
				if (IsCoopGame)
					rVal = FindAnyHomingTarget (&info.position.vPos, OBJ_ROBOT, -1, nThread);
				else if (gameData.app.nGameMode & GM_MULTI_ROBOTS)		//	Not cooperative, if robots, track either robot or CPlayerData
					rVal = FindAnyHomingTarget (&info.position.vPos, OBJ_PLAYER, OBJ_ROBOT, nThread);
				else		//	Not cooperative and no robots, track only a CPlayerData
					rVal = FindAnyHomingTarget (&info.position.vPos, OBJ_PLAYER, -1, nThread);
				}
			else
				rVal = FindAnyHomingTarget (&info.position.vPos, OBJ_PLAYER, OBJ_ROBOT, nThread);
			} 
		else {
			goalType = OBJECTS [cType.laserInfo.nTarget].Type ();
			if ((goalType == OBJ_PLAYER) || (goalType == OBJ_ROBOT) || (goalType == OBJ_MONSTERBALL))
				rVal = FindAnyHomingTarget (&info.position.vPos, goalType, -1, nThread);
			else
				rVal = -1;
			}
		} 
	else {
		if (gameStates.app.cheats.bRobotsKillRobots || (Parent () && (Parent ()->Target ()->Type () == OBJ_ROBOT)))
			goal2Type = OBJ_ROBOT;
		if (nTarget == -1)
			rVal = FindAnyHomingTarget (&info.position.vPos, OBJ_PLAYER, goal2Type, nThread);
		else {
			goalType = OBJECTS [cType.laserInfo.nTarget].Type ();
			rVal = FindAnyHomingTarget (&info.position.vPos, goalType, goal2Type, nThread);
			}
		}
	Assert (rVal != -2);		//	This means it never got set which is bad! Contact Mike.
	return rVal;
	}
return -1;
}

//	-----------------------------------------------------------------------------------------------------------
