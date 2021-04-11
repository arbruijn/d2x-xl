#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>	// for memset ()

#include "descent.h"
#include "error.h"
#include "lightcluster.h"
#include "dynlight.h"
#include "light.h"
#include "segmath.h"

CLightClusterManager lightClusterManager;

//--------------------------------------------------------------------------

CLightClusterManager::CLightClusterManager ()
{
m_bUse = 1;
}

//--------------------------------------------------------------------------

bool CLightClusterManager::Init (void)
{
if (!m_objects.Create (LEVEL_OBJECTS, "CLightClusterManager::m_objects"))
	return false;
m_objects.Clear (0xff);
return true;
}

//--------------------------------------------------------------------------

void CLightClusterManager::Destroy (void)
{
m_objects.Destroy ();
}

//--------------------------------------------------------------------------

void CLightClusterManager::Reset (void)
{
if (!m_bUse)
	return;

	CObject*	pObj;

FORALL_LIGHT_OBJS (pObj)
	if ((pObj->info.nType == OBJ_LIGHT) && (pObj->info.nId == CLUSTER_LIGHT_ID)) {
		pObj->UpdateLife (0);
		memset (&pObj->cType.lightInfo, 0, sizeof (pObj->cType.lightInfo));
		}
}

//--------------------------------------------------------------------------

void CLightClusterManager::Set (void)
{
ENTER (1, 0);
if (!m_bUse)
	RETURN

	CObject	*pObj;
	int32_t		h, i;

FORALL_LIGHT_OBJS (pObj) {
	if ((pObj->info.nType == OBJ_LIGHT) && (pObj->info.nId == CLUSTER_LIGHT_ID)) {
		i = pObj->Index ();
#if DBG
		if (i == nDbgObj)
			BRP;
#endif
		if (!(h = pObj->cType.lightInfo.nObjects)) {
			lightManager.Delete (-1, -1, i);
			pObj->Die ();
			}
		else {
			if (h > 1) {
				pObj->info.position.vPos.v.coord.x /= h;
				pObj->info.position.vPos.v.coord.y /= h;
				pObj->info.position.vPos.v.coord.z /= h;
#if 1
				pObj->cType.lightInfo.color.Red () /= h;
				pObj->cType.lightInfo.color.Green () /= h;
				pObj->cType.lightInfo.color.Blue () /= h;
				pObj->cType.lightInfo.color.Alpha () /= h;
#endif
				}
			if (1 || (pObj->cType.lightInfo.nSegment < 0)) {
				int16_t nSegment = FindSegByPos (pObj->info.position.vPos, abs (pObj->cType.lightInfo.nSegment), 0, 0);
				pObj->cType.lightInfo.nSegment = (nSegment < 0) ? abs (pObj->cType.lightInfo.nSegment) : nSegment;
				}
			if (pObj->info.nSegment != pObj->cType.lightInfo.nSegment)
				OBJECT (i)->RelinkToSeg (pObj->cType.lightInfo.nSegment);
			lightManager.Add (NULL, &pObj->cType.lightInfo.color, pObj->cType.lightInfo.intensity, -1, -1, i, -1, NULL);
			}
		}
	}
RETURN
}

//--------------------------------------------------------------------------

int16_t CLightClusterManager::Create (CObject *pObj)
{
if (!m_bUse)
	return -1;
int16_t nObject = CreateLight (CLUSTER_LIGHT_ID, pObj->info.nSegment, OBJPOS (pObj)->vPos);
if (nObject >= 0)
	OBJECT (nObject)->SetLife (IMMORTAL_TIME);
return nObject;
}

//--------------------------------------------------------------------------

void CLightClusterManager::Delete (int16_t nObject)
{
if (!m_bUse)
	return;
m_objects [nObject].nObject = -1;
}

//--------------------------------------------------------------------------

int32_t CLightClusterManager::Add (int16_t nObject, CFloatVector *color, fix xObjIntensity)
{
ENTER (1, 0);
if (!m_bUse)
	RETVAL (0)

int16_t nLightObj = m_objects [nObject].nObject;

if (0 > nLightObj)
	RETVAL (0)
#if DBG
if (nDbgObj == nLightObj)
	BRP;
#endif
CObject *pLightObj = OBJECT (nLightObj);
if (!pLightObj || (pLightObj->info.nSignature != m_objects [nObject].nSignature)) {
	m_objects [nObject].nObject = -1;
	RETVAL (0)
	}
CObject *pObj = OBJECT (nObject);
if (pLightObj->LifeLeft () < pObj->LifeLeft ())
	pLightObj->UpdateLife (pObj->LifeLeft ());
if (!pLightObj->cType.lightInfo.nObjects++) {
	pLightObj->Position () = pObj->Position ();
	pLightObj->cType.lightInfo.nSegment = pObj->Segment ();
	}
else {
	pLightObj->Position () += pObj->Position ();
	if (pLightObj->cType.lightInfo.nSegment != pObj->Segment ())
		pLightObj->cType.lightInfo.nSegment = -pLightObj->Segment ();
	}
pLightObj->cType.lightInfo.intensity += xObjIntensity;
if (color) {
	pLightObj->cType.lightInfo.color.Red () += color->Red ();
	pLightObj->cType.lightInfo.color.Green () += color->Green ();
	pLightObj->cType.lightInfo.color.Blue () += color->Blue ();
	pLightObj->cType.lightInfo.color.Alpha () += color->Alpha ();
	}
else {
	pLightObj->cType.lightInfo.color.Red () += 1;
	pLightObj->cType.lightInfo.color.Green () += 1;
	pLightObj->cType.lightInfo.color.Blue () += 1;
	pLightObj->cType.lightInfo.color.Alpha () += 1;
	}
RETVAL (1)
}

// --------------------------------------------------------------------------------------------------------------------

void CLightClusterManager::Add (int16_t nObject, int16_t nLightObj)
{
CObject* pLight = OBJECT (nLightObj);

m_objects [nObject].nObject = pLight ? nLightObj : -1;
if (pLight) {
	m_objects [nObject].nSignature = pLight->info.nSignature;
	pLight->cType.lightInfo.nObjects++;
	}
}

// --------------------------------------------------------------------------------------------------------------------
// The purpose of this function is to combine nearby shots coming from the same robot.
// If there is already a shot listed for the object, the function checks whether the new shot is close enough.
// If the function can combine two or more shots, it will create a dedicated light object for them. This light 
// object receives the signature of the first shot assigned to it for future reference.

void CLightClusterManager::AddForAI (CObject *pObj, int16_t nObject, int16_t nShot)
{
ENTER (1, 0);
if (!m_bUse)
	RETURN

int16_t nPrevShot = pObj->Shots ().nObject;

#if DBG
if (nObject == nDbgObj)
	nObject = nDbgObj;
#endif
CObject *pPrevShot = OBJECTEX (nPrevShot, GAMEDATA_ERRLOG_BUFFER);
if (pPrevShot && (pPrevShot->info.nSignature == pObj->Shots ().nSignature)) { // the previous shot fired by that object is still alive
	CObject *pLight, *pShot = OBJECT (nShot);
	int16_t nLight = m_objects [nPrevShot].nObject; // get that shot's light object
	if (nLight < 0)
		pLight = pPrevShot;
	else {
		pLight = OBJECT (nLight);
		if (!pLight || (pLight->info.nSignature != m_objects [nPrevShot].nSignature)) {
			pLight = pPrevShot;
			nLight = -1;
			}
		}
	if (CFixVector::Dist (pShot->info.position.vPos, pLight->info.position.vPos) < I2X (15)) {
		if (nLight >= 0) {
			m_objects [nShot].nObject = nLight;
			pLight->cType.lightInfo.nObjects++;
			}
		else {
			nLight = Create (pPrevShot);
			m_objects [nShot].nObject =
			m_objects [nPrevShot].nObject = nLight;
			if (nLight >= 0) {
				pLight = OBJECT (nLight);
				m_objects [nShot].nSignature =
				m_objects [nPrevShot].nSignature = pLight->info.nSignature;
				pLight->cType.lightInfo.nObjects = 2;
				}
			}
		}
	}
RETURN
}

//--------------------------------------------------------------------------

//eof
