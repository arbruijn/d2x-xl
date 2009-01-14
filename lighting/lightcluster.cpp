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

#include <math.h>
#include <stdio.h>
#include <string.h>	// for memset ()

#include "inferno.h"
#include "error.h"
#include "lightcluster.h"
#include "dynlight.h"
#include "light.h"
#include "gameseg.h"

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

	CObject	*objP;

FORALL_LIGHT_OBJS (objP, i)
	if ((objP->info.nType == OBJ_LIGHT) && (objP->info.nId == CLUSTER_LIGHT_ID)) {
		objP->info.xLifeLeft = 0;
		memset (&objP->cType.lightInfo, 0, sizeof (objP->cType.lightInfo));
		}
}

//--------------------------------------------------------------------------

void CLightClusterManager::Set (void)
{
if (!m_bUse)
	return;

	CObject	*objP;
	int		h, i;

FORALL_LIGHT_OBJS (objP, i) {
	if ((objP->info.nType == OBJ_LIGHT) && (objP->info.nId == CLUSTER_LIGHT_ID)) {
		i = objP->Index ();
		if (!(h = objP->cType.lightInfo.nObjects)) {
			lightManager.Delete (-1, -1, i);
			objP->Die ();
			}
		else {
			if (h > 1) {
				objP->info.position.vPos [X] /= h;
				objP->info.position.vPos [Y] /= h;
				objP->info.position.vPos [Z] /= h;
#if 1
				objP->cType.lightInfo.color.red /= h;
				objP->cType.lightInfo.color.green /= h;
				objP->cType.lightInfo.color.blue /= h;
				objP->cType.lightInfo.color.alpha /= h;
#endif
				}
			if (1 || (objP->cType.lightInfo.nSegment < 0)) {
				short nSegment = FindSegByPos (objP->info.position.vPos, abs (objP->cType.lightInfo.nSegment), 0, 0);
				objP->cType.lightInfo.nSegment = (nSegment < 0) ? abs (objP->cType.lightInfo.nSegment) : nSegment;
				}
			if (objP->info.nSegment != objP->cType.lightInfo.nSegment)
				OBJECTS [i].RelinkToSeg (objP->cType.lightInfo.nSegment);
			AddDynLight (NULL, &objP->cType.lightInfo.color, objP->cType.lightInfo.intensity, -1, -1, i, -1, NULL);
			}
		}
	}
}

//--------------------------------------------------------------------------

int CLightClusterManager::Add (short nObject, tRgbaColorf *color, fix xObjIntensity)
{
if (!m_bUse)
	return 0;

short nLightObj = m_objects [nObject].nObject;

if (0 > nLightObj)
	return 0;
#if DBG
if (nDbgObj == nLightObj)
	nDbgObj = nDbgObj;
#endif
CObject *lightObjP = OBJECTS + nLightObj;
if (lightObjP->info.nSignature != m_objects [nObject].nSignature) {
	m_objects [nObject].nObject = -1;
	return 0;
	}
CObject *objP = OBJECTS + nObject;
if (lightObjP->info.xLifeLeft < objP->info.xLifeLeft)
	lightObjP->info.xLifeLeft = objP->info.xLifeLeft;
if (!lightObjP->cType.lightInfo.nObjects++) {
	lightObjP->info.position.vPos = objP->info.position.vPos;
	lightObjP->cType.lightInfo.nSegment = objP->info.nSegment;
	}
else {
	lightObjP->info.position.vPos += objP->info.position.vPos;
	if (lightObjP->cType.lightInfo.nSegment != objP->info.nSegment)
		lightObjP->cType.lightInfo.nSegment = -lightObjP->info.nSegment;
	}
lightObjP->cType.lightInfo.intensity += xObjIntensity;
if (color) {
	lightObjP->cType.lightInfo.color.red += color->red;
	lightObjP->cType.lightInfo.color.green += color->green;
	lightObjP->cType.lightInfo.color.blue += color->blue;
	lightObjP->cType.lightInfo.color.alpha += color->alpha;
	}
else {
	lightObjP->cType.lightInfo.color.red += 1;
	lightObjP->cType.lightInfo.color.green += 1;
	lightObjP->cType.lightInfo.color.blue += 1;
	lightObjP->cType.lightInfo.color.alpha += 1;
	}
return 1;
}

// --------------------------------------------------------------------------------------------------------------------

void CLightClusterManager::AddForAI (CObject *objP, short nObject, short nShot)
{
if (!m_bUse)
	return;

short nPrevShot = objP->Shots ().nObject;

#if DBG
if (nObject == nDbgObj)
	nObject = nDbgObj;
#endif
if (nPrevShot >= 0) {
	CObject *prevShotP = OBJECTS + nPrevShot;
	if (prevShotP->info.nSignature == objP->Shots ().nSignature) {
		CObject *lightP, *shotP = OBJECTS + nShot;
		short nLight = gameData.objs.lightObjs [nPrevShot].nObject;
		if (nLight < 0)
			lightP = prevShotP;
		else {
			lightP = OBJECTS + nLight;
			if (lightP->info.nSignature != gameData.objs.lightObjs [nPrevShot].nSignature) {
				lightP = prevShotP;
				nLight = -1;
				}
			}
		if (CFixVector::Dist (shotP->info.position.vPos, lightP->info.position.vPos) < I2X (10)) {
			if (nLight >= 0) {
				gameData.objs.lightObjs [nShot].nObject = nLight;
				lightP->cType.lightInfo.nObjects++;
				}
			else {
				nLight = Create (prevShotP);
				gameData.objs.lightObjs [nShot].nObject =
				gameData.objs.lightObjs [nPrevShot].nObject = nLight;
				if (nLight >= 0) {
					lightP = OBJECTS + nLight;
					gameData.objs.lightObjs [nShot].nSignature =
					gameData.objs.lightObjs [nPrevShot].nSignature = lightP->info.nSignature;
					lightP->cType.lightInfo.nObjects = 2;
					}
				}
			}
		}
	}
}

//--------------------------------------------------------------------------

//eof
