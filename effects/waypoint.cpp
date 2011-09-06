#include "descent.h"
#include "wayPoint.h"

CWayPointManager wayPointManager;

// ---------------------------------------------------------------------------------

bool CWayPointManager::Setup (bool bAttach)
{
if (!Count ())
	return false;
if (!m_wayPoints.Create (m_nWayPoints))
	return false;
Gather ();
Renumber ();
LinkBack ();
if (bAttach)
	Attach ();
return true;
}

// ---------------------------------------------------------------------------------

void CWayPointManager::Destroy (void)
{
m_nWayPoints = 0;
m_wayPoints.Destroy ();
}

// ---------------------------------------------------------------------------------
// return the number of way point objects 

int CWayPointManager::Count (void)
{
	CObject* objP;

m_nWayPoints = 0;
FORALL_EFFECT_OBJS (objP, i) {
	if (objP->Id () == WAYPOINT_ID)
		++m_nWayPoints;
	}
return m_nWayPoints;
}

// ---------------------------------------------------------------------------------
// Find a way point object using its logical id (as assigned by the level author in DLE-XP)

CObject* CWayPointManager::Find (int nId)
{
for (int i = 0; i < m_nWayPoints; i++)
	if (m_wayPoints [i]->WayPointId () == nId)
		return m_wayPoints [i];
return NULL;
}

// ---------------------------------------------------------------------------------
// Return a pointer to the target object (end point) of a lightning effect if the effect has one

CObject* CWayPointManager::Target (CObject* objP)
{
	int nTarget = objP->rType.lightningInfo.nTarget;

if (nTarget < 0)
	return NULL;

	CObject* targetP;

FORALL_EFFECT_OBJS (targetP, i) {
	if ((targetP->Id () == LIGHTNING_ID) && (targetP->rType.lightningInfo.nId == nTarget))
		return targetP;
	}

return NULL;
}

// ---------------------------------------------------------------------------------
// Store references to all way point objects in contiguous vector

void CWayPointManager::Gather (void)
{
	CObject* objP;
	int i = 0;

FORALL_EFFECT_OBJS (objP, i) {
	if (objP->Id () == WAYPOINT_ID) 
		m_wayPoints [i++] = objP;
	}
}

// ---------------------------------------------------------------------------------
// Map logical way point ids to way point reference indices (physical ids) in way 
// point reference vector so that 0 <= physical id < way point count

void CWayPointManager::Remap (int& nId)
{
CObject* objP = Find (nId);
#if DBG
if (objP)
	nId = objP->cType.wayPointInfo.nId [0];
else
	nId = -1;
#else
nId = objP ? objP->cType.wayPointInfo.nId [0] : -1;
#endif
}

// ---------------------------------------------------------------------------------
// Map logical way point ids of all way point successors and lightning effect objects 
// to corresponding physical ids

void CWayPointManager::Renumber (void)
{
	CObject* objP;

for (int i = 0; i < m_nWayPoints; i++)
	m_wayPoints [i]->cType.wayPointInfo.nId [0] = i;

for (int i = 0; i < m_nWayPoints; i++)
	Remap (m_wayPoints [i]->NextWayPoint ());

FORALL_EFFECT_OBJS (objP, i) {
	if ((objP->Id () == LIGHTNING_ID) && (*objP->WayPoint () >= 0))
		Remap (*objP->WayPoint ());
	}
}

// ---------------------------------------------------------------------------------
// Setup the predecessor ids for all way points

void CWayPointManager::LinkBack (void)
{
for (int i = 0; i < m_nWayPoints; i++)
	if ((m_wayPoints [i]->NextWayPoint () >= 0) && (m_wayPoints [m_wayPoints [i]->NextWayPoint ()]->PrevWayPoint () < 0))
		m_wayPoints [m_wayPoints [i]->NextWayPoint ()]->PrevWayPoint () = i;
}

// ---------------------------------------------------------------------------------
// Set an object's position to it's current way point's position

void CWayPointManager::Attach (void)
{
	CObject* objP;

FORALL_EFFECT_OBJS (objP, i) {
	if ((objP->Id () == LIGHTNING_ID) && (*objP->WayPoint () >= 0))
		objP->Position () = m_wayPoints [*objP->WayPoint ()]->Position ();
	}
}

// ---------------------------------------------------------------------------------
// Return reference to an object's current way point

CObject* CWayPointManager::Current (CObject* objP)
{
return m_wayPoints [*objP->WayPoint ()];
}

// ---------------------------------------------------------------------------------
// Return reference to an object's next way point

CObject* CWayPointManager::Successor (CObject* objP)
{
return m_wayPoints [Current (objP)->cType.wayPointInfo.nSuccessor [objP->rType.lightningInfo.bDirection]];
}

// ---------------------------------------------------------------------------------
// Attach an object to its next way point and repeat until the way point speed is > 0
// or the initial way point has been reached (i.e. a circle has occurred)

bool CWayPointManager::Hop (CObject* objP)
{
	CObject* succ, * curr = Current (objP);

for (;;) {
	succ = Successor (objP);
	objP->rType.lightningInfo.nWayPoint = succ->cType.wayPointInfo.nId [0];
	objP->Position () = succ->Position ();
	if (succ->NextWayPoint () == succ->PrevWayPoint ())
		objP->rType.lightningInfo.bDirection = !objP->rType.lightningInfo.bDirection;
	if (succ == curr)
		return false; // avoid endless cycles
	if (succ->cType.wayPointInfo.nSpeed > 0)
		break;
	if (Target (objP))
		objP->StartSync ();
	}
return !objP->Synchronize ();
}

// ---------------------------------------------------------------------------------
// Use zero speed way points to synchronize linked pairs of effect objects (e.g. a 
// lightning emitter and it's lightning end point).
// Check if moving effect object has a target effect (e.g. lightning end point) and 
// has hopped to the current way point from a zero speed way point.
// If so, check if the target effect has also hopped to its current way point from a
// zero speed way point. If so, end synchronizing the movement of the two effects
// by allowing them to move again. If not, halt the effect until the other effect 
// has made such a hop, too.

bool CWayPointManager::Synchronize (CObject* objP)
{
if (!objP->Synchronize ()) 
	return false;

	CObject* targetP = Target (objP);

if (!targetP->Synchronize ()) 
	return true;

objP->StopSync ();
targetP->StopSync ();
return false;
}

// ---------------------------------------------------------------------------------
// Move an object towards its next way point. Move distance depends on the object 
// speed which is given by its current way point (i.e. the last way point it has 
// reached or passed). If a way point is reached or passed, make it or the first
// subsequent way point with a speed > 0 the current way point, and move the object
// for a distance depending on the new way point's speed and remainder of movement
// frame time.

void CWayPointManager::Move (CObject* objP)
{
if (Synchronize (objP))
	return;

	float fScale = 1.0f;

for (;;) {
	CObject* curr = Current (objP);
	int nSucc = curr->cType.wayPointInfo.nSuccessor [objP->rType.lightningInfo.bDirection];
	if (nSucc < 0)
		break;
	CObject* succ = m_wayPoints [nSucc];

		CFloatVector vMove, vLeft;

	vMove.Assign (succ->Position () - curr->Position ());
	CFloatVector::Normalize (vMove);
	float fMove = (float) curr->cType.wayPointInfo.nSpeed / 40.0f * fScale;
	vMove *= fMove;
	vLeft.Assign (succ->Position () - objP->Position ());
#if DBG
	if (CFloatVector::Dot (vLeft, vMove) < 0.0f)
		nDbgSeg = nDbgSeg;
#endif
	float fLeft = vLeft.Mag ();
	if (fLeft > fMove) {
		objP->Position () += vMove;
#if DBG
		vLeft.Assign (succ->Position () - objP->Position ());
		if (CFloatVector::Dot (vLeft, vMove) < 0.0f)
			nDbgSeg = nDbgSeg;
		else
			return;
#else
		return;
#endif
		}
	if (!Hop (objP))
		return;
	if (fLeft == fMove)
		return;
	fScale = (fMove - fLeft) / fMove;
	}
}

// ---------------------------------------------------------------------------------

void CWayPointManager::Update (void)
{
if (gameStates.app.tick40fps.bTick) {
	CObject* objP;
	int i = 0;

	FORALL_EFFECT_OBJS (objP, i) {
		if ((objP->Id () == LIGHTNING_ID) && (*objP->WayPoint () >= 0))
			Move (objP);
		}
	}
}

// ---------------------------------------------------------------------------------

