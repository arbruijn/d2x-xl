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
if (!m_objects.Create (LEVEL_OBJECTS))
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

	CObject*	objP;

FORALL_LIGHT_OBJS (objP)
	if ((objP->info.nType == OBJ_LIGHT) && (objP->info.nId == CLUSTER_LIGHT_ID)) {
		objP->UpdateLife (0);
		memset (&objP->cType.lightInfo, 0, sizeof (objP->cType.lightInfo));
		}
}

//--------------------------------------------------------------------------

void CLightClusterManager::Set (void)
{
if (!m_bUse)
	return;

	CObject	*objP;
	int32_t		h, i;

FORALL_LIGHT_OBJS (objP) {
	if ((objP->info.nType == OBJ_LIGHT) && (objP->info.nId == CLUSTER_LIGHT_ID)) {
		i = objP->Index ();
#if DBG
		if (i == nDbgObj)
			BRP;
#endif
		if (!(h = objP->cType.lightInfo.nObjects)) {
			lightManager.Delete (-1, -1, i);
			objP->Die ();
			}
		else {
			if (h > 1) {
				objP->info.position.vPos.v.coord.x /= h;
				objP->info.position.vPos.v.coord.y /= h;
				objP->info.position.vPos.v.coord.z /= h;
#if 1
				objP->cType.lightInfo.color.Red () /= h;
				objP->cType.lightInfo.color.Green () /= h;
				objP->cType.lightInfo.color.Blue () /= h;
				objP->cType.lightInfo.color.Alpha () /= h;
#endif
				}
			if (1 || (objP->cType.lightInfo.nSegment < 0)) {
				int16_t nSegment = FindSegByPos (objP->info.position.vPos, abs (objP->cType.lightInfo.nSegment), 0, 0);
				objP->cType.lightInfo.nSegment = (nSegment < 0) ? abs (objP->cType.lightInfo.nSegment) : nSegment;
				}
			if (objP->info.nSegment != objP->cType.lightInfo.nSegment)
				OBJECTS [i].RelinkToSeg (objP->cType.lightInfo.nSegment);
			lightManager.Add (NULL, &objP->cType.lightInfo.color, objP->cType.lightInfo.intensity, -1, -1, i, -1, NULL);
			}
		}
	}
}

//--------------------------------------------------------------------------

int16_t CLightClusterManager::Create (CObject *objP)
{
if (!m_bUse)
	return -1;
int16_t nObject = CreateLight (CLUSTER_LIGHT_ID, objP->info.nSegment, OBJPOS (objP)->vPos);
if (nObject >= 0)
	OBJECTS [nObject].SetLife (IMMORTAL_TIME);
return nObject;
}

//--------------------------------------------------------------------------

int32_t CLightClusterManager::Add (int16_t nObject, CFloatVector *color, fix xObjIntensity)
{
if (!m_bUse)
	return 0;

int16_t nLightObj = m_objects [nObject].nObject;

if (0 > nLightObj)
	return 0;
#if DBG
if (nDbgObj == nLightObj)
	BRP;
#endif
CObject *lightObjP = OBJECTS + nLightObj;
if (lightObjP->info.nSignature != m_objects [nObject].nSignature) {
	m_objects [nObject].nObject = -1;
	return 0;
	}
CObject *objP = OBJECTS + nObject;
if (lightObjP->LifeLeft () < objP->LifeLeft ())
	lightObjP->UpdateLife (objP->LifeLeft ());
if (!lightObjP->cType.lightInfo.nObjects++) {
	lightObjP->Position () = objP->Position ();
	lightObjP->cType.lightInfo.nSegment = objP->Segment ();
	}
else {
	lightObjP->Position () += objP->Position ();
	if (lightObjP->cType.lightInfo.nSegment != objP->Segment ())
		lightObjP->cType.lightInfo.nSegment = -lightObjP->Segment ();
	}
lightObjP->cType.lightInfo.intensity += xObjIntensity;
if (color) {
	lightObjP->cType.lightInfo.color.Red () += color->Red ();
	lightObjP->cType.lightInfo.color.Green () += color->Green ();
	lightObjP->cType.lightInfo.color.Blue () += color->Blue ();
	lightObjP->cType.lightInfo.color.Alpha () += color->Alpha ();
	}
else {
	lightObjP->cType.lightInfo.color.Red () += 1;
	lightObjP->cType.lightInfo.color.Green () += 1;
	lightObjP->cType.lightInfo.color.Blue () += 1;
	lightObjP->cType.lightInfo.color.Alpha () += 1;
	}
return 1;
}

// --------------------------------------------------------------------------------------------------------------------

void CLightClusterManager::Add (int16_t nObject, int16_t nLightObj)
{
m_objects [nObject].nObject = nLightObj;
if (nLightObj >= 0) {
	m_objects [nObject].nSignature = OBJECTS [nLightObj].info.nSignature;
	OBJECTS [nLightObj].cType.lightInfo.nObjects++;
	}
}

// --------------------------------------------------------------------------------------------------------------------
// The purpose of this function is to combine nearby shots coming from the same robot.
// If there is already a shot listed for the object, the function checks whether the new shot is close enough.
// If the function can combine two or more shots, it will create a dedicated light object for them. This light 
// object receives the signature of the first shot assigned to it for future reference.

void CLightClusterManager::AddForAI (CObject *objP, int16_t nObject, int16_t nShot)
{
if (!m_bUse)
	return;

int16_t nPrevShot = objP->Shots ().nObject;

#if DBG
if (nObject == nDbgObj)
	nObject = nDbgObj;
#endif
if (nPrevShot >= 0) {
	CObject *prevShotP = OBJECTS + nPrevShot;
	if (prevShotP->info.nSignature == objP->Shots ().nSignature) {
		CObject *lightP, *shotP = OBJECTS + nShot;
		int16_t nLight = m_objects [nPrevShot].nObject;
		if (nLight < 0)
			lightP = prevShotP;
		else {
			lightP = OBJECTS + nLight;
			if (lightP->info.nSignature != m_objects [nPrevShot].nSignature) {
				lightP = prevShotP;
				nLight = -1;
				}
			}
		if (CFixVector::Dist (shotP->info.position.vPos, lightP->info.position.vPos) < I2X (15)) {
			if (nLight >= 0) {
				m_objects [nShot].nObject = nLight;
				lightP->cType.lightInfo.nObjects++;
				}
			else {
				nLight = Create (prevShotP);
				m_objects [nShot].nObject =
				m_objects [nPrevShot].nObject = nLight;
				if (nLight >= 0) {
					lightP = OBJECTS + nLight;
					m_objects [nShot].nSignature =
					m_objects [nPrevShot].nSignature = lightP->info.nSignature;
					lightP->cType.lightInfo.nObjects = 2;
					}
				}
			}
		}
	}
}

//--------------------------------------------------------------------------

//eof
