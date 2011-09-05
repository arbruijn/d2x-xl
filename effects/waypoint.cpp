#include "descent.h"
#include "wayPoint.h"

// ---------------------------------------------------------------------------------

bool CWayPointManager::Setup (void)
{
if (!Count ())
	return false;
if (!m_wayPoints.Create (h))
	return false;
Gather ();
Renumber ();
LinkBack ();
return true;
}

// ---------------------------------------------------------------------------------

int CWayPointManager::Count (void)
{
m_nWayPoints = 0;
FORALL_EFFECT_OBJS (objP, i) {
	if (objP->Id () == WAYPOINT_ID)
		++m_nWayPoints;
	}
}

// ---------------------------------------------------------------------------------

CObject* CWayPointManager::Find (int nId)
{
for (int i = 0; i < m_nWayPoints; i++)
	if (m_wayPoints [i].cType.wayPointInfo.nId [1] == nId)
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
	Remap (m_wayPoints [i].nSuccessor [0]);

FORALL_EFFECT_OBJS (objP, i) {
	if (objP->Id () == LIGHTNING_ID) && (objP->rType.lightningInfo.nWayPoint >= 0))
		Remap (objP->rType.lightningInfo.nWayPoint);
	}
}

// ---------------------------------------------------------------------------------

void CWayPointManager::LinkBack (void)
{
for (int i = 0; i < m_nWayPoints; i++)
	if (m_wayPoints [m_wayPoints [i].nSuccessor [0]].nSuccessor [1] < 0)
		m_wayPoints [m_wayPoints [i].nSuccessor [0]].nSuccessor [1] = i;
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
CObject* curr = Current (objP);
do {
	CObject* succ = Successor (objP);
	objP->rType.lightningInfo.nWayPoint = succ->cType.wayPointInfo.nId [0];
	objP->Position () = succ->Position ();
	if (succ->cType.wayPointInfo.nSuccessor [0] == succ->cType.wayPointInfo.nSuccessor [1])
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

do {
	CObject* wp0 = m_wayPoints [objP->rType.lightningInfo.nWayPoint];
	CObject* wp1 = m_wayPoints [wp0->cType.wayPointInfo.nSuccessor [objP->rType.lightningInfo.bDirection]];

		CFloatVector vDirf, vMovef, vLeftf;

	vDirf.Assign (wp1->Position () - wp0->Position ());
	vMovef = vDirf;
	CFloatVector::Normalize (vMove);
	float fMove = (float) wp0->cType.wayPointInfo.nSpeed / 40.0 * fScale;
	vMovef *= fMove;
	vLeftf.Assign (wp1->Position () - objP->Position ());
	float fLeft = vLeftf.Mag ();
	if (fLeft > fMove) {
		objP->Position () += vMovef;
			return;
		}
	if (!Hop (objP))
		return;
	if (fLeft == vMove)
		return;
	fScale = (fMove - fLeft) / fMove;
	}
}

// ---------------------------------------------------------------------------------

void CWayPointManager::Update (void)
{
	CObject* objP;
	int i = 0;

FORALL_EFFECT_OBJS (objP, i) {
	if (objP->Id () !LIGHTNING_ID) 
		continue;
	if (objP->rType.lightningInfo.nWayPoint < 0) 
		continue;
	Move (objP);
	}
}

// ---------------------------------------------------------------------------------

