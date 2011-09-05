#include "descent.h"
#include "wayPoint.h"

CWayPointManager wayPointManager;

// ---------------------------------------------------------------------------------

bool CWayPointManager::Setup (void)
{
if (!Count ())
	return false;
if (!m_wayPoints.Create (m_nWayPoints))
	return false;
Gather ();
Renumber ();
LinkBack ();
return true;
}

// ---------------------------------------------------------------------------------

void CWayPointManager::Destroy (void)
{
m_nWayPoints = 0;
m_wayPoints.Destroy ();
}

// ---------------------------------------------------------------------------------

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

CObject* CWayPointManager::Find (int nId)
{
for (int i = 0; i < m_nWayPoints; i++)
	if (m_wayPoints [i]->WayPointId () == nId)
		return m_wayPoints [i];
return NULL;
}

// ---------------------------------------------------------------------------------

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

void CWayPointManager::Remap (int& nId)
{
CObject* objP = Find (nId);
nId = objP ? objP->cType.wayPointInfo.nId [0] : -1;
}

// ---------------------------------------------------------------------------------

void CWayPointManager::Renumber (void)
{
	CObject* objP;

for (int i = 0; i < m_nWayPoints; i++)
	Remap (m_wayPoints [i]->NextWayPoint ());

FORALL_EFFECT_OBJS (objP, i) {
	if ((objP->Id () == LIGHTNING_ID) && (objP->WayPoint () >= 0))
		Remap (*objP->WayPoint ());
	}
}

// ---------------------------------------------------------------------------------

void CWayPointManager::LinkBack (void)
{
for (int i = 0; i < m_nWayPoints; i++)
	if (m_wayPoints [m_wayPoints [i]->NextWayPoint ()]->PrevWayPoint () < 0)
		m_wayPoints [m_wayPoints [i]->NextWayPoint ()]->PrevWayPoint () = i;
}

// ---------------------------------------------------------------------------------

CObject* CWayPointManager::Current (CObject* objP)
{
return m_wayPoints [objP->rType.lightningInfo.nWayPoint];
}

// ---------------------------------------------------------------------------------

CObject* CWayPointManager::Successor (CObject* objP)
{
return m_wayPoints [Current (objP)->cType.wayPointInfo.nSuccessor [objP->rType.lightningInfo.bDirection]];
}

// ---------------------------------------------------------------------------------

bool CWayPointManager::Hop (CObject* objP)
{
CObject* succ, * curr = Current (objP);
do {
	succ = Successor (objP);
	objP->rType.lightningInfo.nWayPoint = succ->cType.wayPointInfo.nId [0];
	objP->Position () = succ->Position ();
	if (succ->NextWayPoint () == succ->PrevWayPoint ())
		objP->rType.lightningInfo.bDirection = !objP->rType.lightningInfo.bDirection;
	if (succ == curr)
		return false; // avoid endless cycles
	} while (succ->cType.wayPointInfo.nSpeed <= 0);
return true;
}

// ---------------------------------------------------------------------------------

void CWayPointManager::Move (CObject* objP)
{
	float fScale = 1.0f;

for (;;) {
	CObject* wp0 = m_wayPoints [objP->rType.lightningInfo.nWayPoint];
	CObject* wp1 = m_wayPoints [wp0->cType.wayPointInfo.nSuccessor [objP->rType.lightningInfo.bDirection]];

		CFloatVector vDir, vMove, vLeft;

	vDir.Assign (wp1->Position () - wp0->Position ());
	vMove = vDir;
	CFloatVector::Normalize (vMove);
	float fMove = (float) wp0->cType.wayPointInfo.nSpeed / 40.0f * fScale;
	vMove *= fMove;
	vLeft.Assign (wp1->Position () - objP->Position ());
	float fLeft = vLeft.Mag ();
	if (fLeft > fMove) {
		objP->Position () += vMove;
			return;
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
		if ((objP->Id () == LIGHTNING_ID) && (objP->WayPoint () >= 0))
			Move (objP);
		}
	}
}

// ---------------------------------------------------------------------------------

