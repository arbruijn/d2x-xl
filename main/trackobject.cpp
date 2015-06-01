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

#define NEW_TARGETTING 1

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

class CTarget {
	public:
		fix		m_xDot;
		CObject*	m_pObj;

		CTarget (fix xDot = 0, CObject* pObj = NULL) : m_xDot(xDot), m_pObj(pObj) {}

		inline bool operator< (CTarget& other) { return m_xDot < other.m_xDot; }
		inline bool operator> (CTarget& other) { return m_xDot > other.m_xDot; }
		inline bool operator<= (CTarget& other) { return m_xDot <= other.m_xDot; }
		inline bool operator>= (CTarget& other) { return m_xDot >= other.m_xDot; }
	};

static CStack<class CTarget>	targetLists [MAX_THREADS + 1];

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

class CHomingTargetData {
	public:
		CObject		*m_pTracker;
		CFixVector	m_vTrackerPos;
		CFixVector	m_vTrackerViewDir;
		fix			m_xMaxDist;
		fix			m_xBestDot;
		int32_t		m_nBestObj;
		CStack<class CTarget>& m_targets;

		CHomingTargetData (CObject* pTracker, CFixVector& vTrackerPos, CStack<class CTarget>& targets) 
			: m_pTracker (pTracker), m_vTrackerPos (vTrackerPos), m_nBestObj (-1), m_targets (targets)
			{ 
			m_vTrackerViewDir = SPECTATOR (m_pTracker) ? gameStates.app.playerPos.mOrient.m.dir.f : m_pTracker->info.position.mOrient.m.dir.f;
			m_xMaxDist = pTracker->MaxTrackableDist (m_xBestDot); 
#if NEW_TARGETTING
			m_targets.SetGrowth (100);
			m_targets.Reset ();
#endif
			}

		void Add (CObject* pTarget, float dotScale = 1.0f);

		int32_t Target (int32_t nThread);
	};

//	-----------------------------------------------------------------------------

void CHomingTargetData::Add (CObject* pTarget, float dotScale)
{
ENTER (0, 0);
CFixVector vTarget = OBJPOS (pTarget)->vPos - m_vTrackerPos;
fix dist = CFixVector::Normalize (vTarget);
if (dist >= m_xMaxDist)
	LEAVE;
fix dot = fix (CFixVector::Dot (vTarget, m_vTrackerViewDir) * dotScale);
if (dot < m_xBestDot)
	LEAVE;
#if NEW_TARGETTING
#	if 1
m_targets.Push (CTarget (dot, pTarget));
#	else
m_targets.Push (CTarget (fix (dist * (1.0f - X2F (dot) / 2.0f)), pTarget));
#	endif
#else
//	Note: This uses the constant, not-scaled-by-frametime value, because it is only used
//	to determine if an CObject is initially trackable.  FindHomingTarget is called on subsequent
//	frames to determine if the CObject remains trackable.
if (ObjectToObjectVisibility (m_pTracker, pTarget, FQ_TRANSWALL, 1.0f, nThread)) {
	m_xBestDot = dot;
	m_nBestObj = pTarget->Index ();
	}
#endif
LEAVE
}

//	-----------------------------------------------------------------------------

int32_t CHomingTargetData::Target (int32_t nThread)
{
ENTER (0, nThread);
#if NEW_TARGETTING
if (m_targets.ToS () < 1)
	RETURN (-1)
if (m_targets.ToS () > 1)
	m_targets.SortDescending ();

for (uint32_t i = 0; i < m_targets.ToS (); i++) {
	if (ObjectToObjectVisibility (m_pTracker, m_targets [i].m_pObj, FQ_TRANSWALL, -1.0f, nThread))
		RETURN (m_targets[i].m_pObj->Index ())
	}
RETURN (-1)
#else
RETURN (m_nBestObj)
#endif
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	Return true if weapon *pTracker is able to track CObject OBJECT (nTarget), else return false.
//	In order for the CObject to be trackable, it must be within a reasonable turning radius for the missile
//	and it must not be obstructed by a CWall.
int32_t CObject::ObjectIsTrackable (int32_t nTarget, fix& xDot, int32_t nThread)
{
ENTER (0, nThread);
if (IsCoopGame)
	RETURN (0)
CObject* pTarget = OBJECT (nTarget);
if (!pTarget)
	RETURN (0)
//	Don't track player if he's cloaked.
if ((nTarget == LOCALPLAYER.nObject) && (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
	RETURN (0)
//	Can't track AI CObject if he's cloaked.
if (pTarget->Type () == OBJ_ROBOT) {
	if (pTarget->cType.aiInfo.CLOAKED)
		RETURN (0)
	//	Your missiles don't track your escort.
	if (pTarget->IsGuideBot () && (cType.laserInfo.parent.nType == OBJ_PLAYER))
		RETURN (0)
	}
CFixVector vTracker = OBJPOS (this)->mOrient.m.dir.f;
CFixVector vTarget = pTarget->Position () - Position ();
CFixVector::Normalize (vTarget);
xDot = CFixVector::Dot (vTarget, vTracker);
if ((xDot < gameData.weaponData.xMinTrackableDot) && (xDot > I2X (9) / 10)) {
	CFixVector::Normalize (vTarget);
	xDot = CFixVector::Dot (vTarget, vTracker);
	}

if ((EGI_FLAG (bEnhancedShakers, 0, 0, 0) && (Type () == OBJ_WEAPON) && (Id () == EARTHSHAKER_MEGA_ID) /*&& (xDot >= 0)*/)) {
	//	xDot is in legal range, now see if CObject is visible
	RETURN (ObjectToObjectVisibility (this, pTarget, FQ_TRANSWALL, -1.0f, nThread))
	}
RETURN (0)
}

//	-----------------------------------------------------------------------------

int32_t CObject::SelectHomingTarget (CFixVector& vTrackerPos, int32_t nThread)
{
ENTER (0, nThread);
if (!IsMultiGame)
	RETURN (FindAnyHomingTarget (vTrackerPos, OBJ_ROBOT, -1, nThread))
if ((Type () == OBJ_PLAYER) || (cType.laserInfo.parent.nType == OBJ_PLAYER)) {
	//	It's fired by a player, so if robots present, track robot, else track player.
	RETURN (IsCoopGame 
			  ? FindAnyHomingTarget (vTrackerPos, OBJ_ROBOT, -1, nThread) 
			  : FindAnyHomingTarget (vTrackerPos, OBJ_PLAYER, OBJ_ROBOT, nThread))
		} 
CObject* pParent = Parent ();
RETURN (FindAnyHomingTarget (vTrackerPos, OBJ_PLAYER, (pParent && (pParent->Target ()->Type () == OBJ_ROBOT)) ? OBJ_ROBOT : -1, nThread))
}

//	-----------------------------------------------------------------------------

int32_t CObject::FindTargetWindow (void)
{
ENTER (0, 0);
for (int32_t i = 0; i < MAX_RENDERED_WINDOWS; i++)
	if ((windowRenderedData [i].nFrame >= gameData.appData.nFrameCount - 1) &&
		 ((windowRenderedData [i].pViewer == gameData.objData.pConsole) || (this == GuidedInMainView ())) &&
		 !windowRenderedData [i].bRearView) {
		RETURN (i)
		}
RETURN (-1)
}

//	-----------------------------------------------------------------------------

int32_t CObject::MaxTrackableDist (int32_t& xBestDot)
{
ENTER (0, 0);
if (Type () == OBJ_WEAPON) {
	if (Id () == OMEGA_ID) {
		xBestDot = OMEGA_MIN_TRACKABLE_DOT;
		RETURN (OMEGA_MAX_TRACKABLE_DIST)
		}
	if ((Id () == EARTHSHAKER_MEGA_ID) && EGI_FLAG (bEnhancedShakers, 0, 0, 0)) {
		xBestDot = I2X (1) / 4;
		RETURN (MAX_TRACKABLE_DIST * 2)
		}
	}
xBestDot = MIN_TRACKABLE_DOT;
RETURN (MAX_TRACKABLE_DIST)
}

//	-----------------------------------------------------------------------------
//	Find CObject to home in on.
//	Scan list of OBJECTS rendered last frame, find one that satisfies function of nearness to center and distance.
int32_t CObject::FindVisibleHomingTarget (CFixVector& vTrackerPos, int32_t nThread)
{
ENTER (0, nThread);
//	Find an CObject to track based on game mode (eg, whether in network play) and who fired it.
if (IsMultiGame)
	RETURN (SelectHomingTarget (vTrackerPos, nThread))

//	Not in network mode.  If not fired by player, then track player.
if ((Type () != OBJ_PLAYER) && (cType.laserInfo.parent.nObject != LOCALPLAYER.nObject) && !(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
	RETURN (OBJ_IDX (gameData.objData.pConsole))

	int32_t nWindow = FindTargetWindow ();

if (nWindow == -1)
	RETURN (SelectHomingTarget (vTrackerPos, nThread))

	bool bOmega = (Type () == OBJ_WEAPON) && (Id () == OMEGA_ID);
	bool bPlayer = (Type () == OBJ_PLAYER);
	CHomingTargetData targetData (this, vTrackerPos, targetLists [0]);

//	Not in multiplayer mode and fired by player.
for (int32_t i = windowRenderedData [nWindow].nObjects - 1; i >= 0; i--) {
	int16_t nObject = windowRenderedData [nWindow].renderedObjects [i];
	if (nObject == LOCALPLAYER.nObject)
		continue;
	CObject* pTarget = OBJECT (nObject);
	if (!pTarget)
		continue;
	//	Can't track AI CObject if he's cloaked.
	int32_t nType = pTarget->Type ();
	if (nType == OBJ_ROBOT) {
		if (pTarget->cType.aiInfo.CLOAKED)
			continue;
		//	Your missiles don't track your escort.
		if (pTarget->IsGuideBot () && (bPlayer || (cType.laserInfo.parent.nType == OBJ_PLAYER)))
			continue;
		}
	else if (nType == OBJ_WEAPON) {
		if (!(bOmega && EGI_FLAG (bKillMissiles, 0, 1, 0) && bOmega && (pTarget->IsMissile () || pTarget->IsMine ())))
			continue;
		}
	else if ((nType != OBJ_PLAYER) && (nType != OBJ_REACTOR))
		continue;

	targetData.Add (pTarget);
	}
RETURN (targetData.Target (nThread))
}

//	-----------------------------------------------------------------------------
//	Find CObject to home in on.
//	Scan list of OBJECTS rendered last frame, find one that satisfies function of nearness to center and distance.
//	Can track two kinds of OBJECTS.  If you are only interested in one nType, set targetType2 to NULL
//	Always track proximity bombs.  --MK, 06/14/95
//	Make homing OBJECTS not track parent's prox bombs.

int32_t CObject::FindAnyHomingTarget (CFixVector& vTrackerPos, int32_t targetType1, int32_t targetType2, int32_t nThread)
{
ENTER (0, nThread);
	bool bOmega = (Type () == OBJ_WEAPON) && (Id () == OMEGA_ID);
	CObject* pTarget;
	CHomingTargetData targetData (this, vTrackerPos, targetLists [nThread]);

FORALL_ACTOR_OBJS (pTarget) {
	if (OBJ_IDX (pTarget) == cType.laserInfo.parent.nObject) // Don't track shooter
		continue;

	float dotScale = 1.0f;
	int32_t nType = pTarget->Type ();
	if (nType == OBJ_WEAPON) {
		if (pTarget->cType.laserInfo.parent.nSignature == cType.laserInfo.parent.nSignature)
			continue; // target was created by tracker (e.g. a mine dropped by the tracker object)
		if (pTarget->IsPlayerMine ())
			dotScale = 9.0f / 8.0f;
		else if (!(bOmega && EGI_FLAG (bKillMissiles, 0, 1, 0) && (pTarget->IsMissile () || pTarget->IsMine ())))
			continue;
		}
	else if ((nType != targetType1) && (nType != targetType2) && (nType != OBJ_MONSTERBALL)) {
		continue;
		}
	else if (pTarget->Type () == OBJ_PLAYER) {
		if (PLAYER (pTarget->Id ()).flags & PLAYER_FLAGS_CLOAKED)
			continue; // don't track cloaked players.
		if (IsTeamGame) {
			CObject *pParentObj = OBJECT (cType.laserInfo.parent.nObject);
			if (pParentObj && (pParentObj->Type () == OBJ_PLAYER) && (GetTeam (pTarget->Id ()) == GetTeam (pParentObj->Id ())))
				continue; // don't track teammates in team games
			}
		}
	else if (pTarget->Type () == OBJ_ROBOT) {
		if (pTarget->cType.aiInfo.CLOAKED)
			continue; // don' track cloaked robots
		if (pTarget->IsGuideBot () && (cType.laserInfo.parent.nType == OBJ_PLAYER))
			continue;	//	player missiles don't track the guidebot.
		}

	targetData.Add (pTarget, dotScale);
	}

RETURN (targetData.Target (nThread))
}

//	---------------------------------------------------------------------------------------------
//	See if legal to keep tracking currently tracked CObject.  If not, see if another CObject is trackable.  If not, return -1,
//	else return CObject number of tracking CObject.
//	Computes and returns a fairly precise dot product.
// This function is only called every 25 ms max. (-> updating at 40 fps or less) 

int32_t CObject::UpdateHomingTarget (int32_t nTarget, fix& dot, int32_t nThread)
{
ENTER (0, nThread);
	int32_t	rVal = -2;
	int32_t	nFrame;
	int32_t	targetType1, targetType2 = -1;

//if (!gameOpts->legacy.bHomers && gameStates.limitFPS.bHomers && !gameStates.app.tick40fps.bTick)
	//	Every 8 frames for each CObject, scan all OBJECTS.
nFrame = OBJ_IDX (this) ^ gameData.appData.nFrameCount;
if (ObjectIsTrackable (nTarget, dot, nThread) && (!gameOpts->legacy.bHomers || (nFrame % 8)))
	RETURN (nTarget)

if (!gameOpts->legacy.bHomers || (nFrame % 4 == 0)) {
	//	If player fired missile, then search for an CObject, if not, then give up.
	CObject *pTarget, *pParent = OBJECT (cType.laserInfo.parent.nObject);
	if (pParent && pParent->IsPlayer ()) {
		if (nTarget == -1) {
			if (IsMultiGame) {
				if (IsCoopGame)
					rVal = FindAnyHomingTarget (Position (), OBJ_ROBOT, -1, nThread);
				else if (gameData.appData.GameMode (GM_MULTI_ROBOTS))		//	Not cooperative, if robots, track either robot or player
					rVal = FindAnyHomingTarget (Position (), OBJ_PLAYER, OBJ_ROBOT, nThread);
				else		//	Not cooperative and no robots, track only a player
					rVal = FindAnyHomingTarget (Position (), OBJ_PLAYER, -1, nThread);
				}
			else
				rVal = FindAnyHomingTarget (Position (), OBJ_PLAYER, OBJ_ROBOT, nThread);
			} 
		else if ((pTarget = OBJECT (cType.laserInfo.nHomingTarget))) {
			targetType1 = pTarget->Type ();
			if ((targetType1 == OBJ_PLAYER) || (targetType1 == OBJ_ROBOT) || (targetType1 == OBJ_MONSTERBALL))
				rVal = FindAnyHomingTarget (Position (), targetType1, -1, nThread);
			else
				rVal = -1;
			}
		} 
	else {
		if (AttacksRobots () || (Parent () && (Parent ()->Target ()->Type () == OBJ_ROBOT)))
			targetType2 = OBJ_ROBOT;
		if (nTarget == -1)
			rVal = FindAnyHomingTarget (Position (), OBJ_PLAYER, targetType2, nThread);
		else if ((pTarget = OBJECT (cType.laserInfo.nHomingTarget))) {
			rVal = FindAnyHomingTarget (Position (), pTarget->Type (), targetType2, nThread);
			}
		}
	Assert (rVal != -2);		//	This means it never got set which is bad! Contact Mike.
	RETURN (rVal)
	}
RETURN (-1)
}

//	--------------------------------------------------------------------------------------------
