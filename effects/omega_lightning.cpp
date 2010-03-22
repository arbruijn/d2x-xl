#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "descent.h"
#include "gameseg.h"

#include "objsmoke.h"
#include "transprender.h"
#include "renderthreads.h"
#include "rendermine.h"
#include "error.h"
#include "light.h"
#include "dynlight.h"
#include "ogl_lib.h"
#include "automap.h"
#ifdef TACTILE
#	include "tactile.h"
#endif

COmegaLightning	omegaLightnings;

// ---------------------------------------------------------------------------------

int COmegaLightning::Find (short nObject)
{
	int	i;

for (i = 0; i < m_nHandles; i++)
	if (m_handles [i].nParentObj == nObject)
		return i;
return -1;
}

// ---------------------------------------------------------------------------------

void COmegaLightning::Delete (short nHandle)
{
if (m_nHandles) {
	lightningManager.Destroy (lightningManager.m_emitters + m_handles [nHandle].nLightning, NULL);
	if (nHandle < --m_nHandles)
		m_handles [nHandle] = m_handles [m_nHandles];
	memset (m_handles + m_nHandles, 0xff, sizeof (tOmegaLightningHandles));
	}
}

// ---------------------------------------------------------------------------------

void COmegaLightning::Destroy (short nObject)
{
	int	nHandle;

if (nObject < 0) {
	for (nHandle = m_nHandles; nHandle; )
		Delete (--nHandle);
	}
else {
	if (0 <= (nHandle = Find (nObject)))
		Delete (nHandle);
	}
}

// ---------------------------------------------------------------------------------

CFixVector *COmegaLightning::GetGunPoint (CObject* objP, CFixVector *vMuzzle)
{
	CFixVector			*vGunPoints;
	int					bSpectate;
	tObjTransformation	*posP;

if (!objP)
	return NULL;
bSpectate = SPECTATOR (objP);
posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
if ((bSpectate || (objP->info.nId != gameData.multiplayer.nLocalPlayer)) &&
	 (vGunPoints = GetGunPoints (objP, 6))) {
	TransformGunPoint (objP, vGunPoints, 6, 0, 0, vMuzzle, NULL);
	}
else {
	*vMuzzle = posP->vPos - posP->mOrient.UVec ();
	*vMuzzle += posP->mOrient.FVec () * (objP->info.xSize / 4);
	}
return vMuzzle;
}

// ---------------------------------------------------------------------------------

int COmegaLightning::Update (CObject* parentObjP, CObject* targetObjP)
{
	CFixVector					vMuzzle;
	tOmegaLightningHandles	*handleP;
	CWeaponState				*wsP;
	int							i, j, nHandle, nLightning;

if (!(SHOW_LIGHTNING && gameOpts->render.lightning.bOmega && !gameStates.render.bOmegaModded))
	return -1;
if (m_nHandles < 1)
	return 0;
if ((gameData.omega.xCharge [IsMultiGame] >= MAX_OMEGA_CHARGE) &&
	 (0 <= (nHandle = Find (LOCALPLAYER.nObject))))
	Destroy (nHandle);
short nObject = parentObjP ? OBJ_IDX (parentObjP) : -1;
if (nObject < 0) {
	i = 0;
	j = m_nHandles;
	}
else {
	i = Find (OBJ_IDX (parentObjP));
	if (i < 0)
		return 0;
	j = 1;
	m_handles [i].nTargetObj = targetObjP ? OBJ_IDX (targetObjP) : -1;
	}
for (handleP = m_handles + i; j; j--) {
	if ((nLightning = handleP->nLightning) >= 0) {
		parentObjP = OBJECTS + handleP->nParentObj;
		if (parentObjP->info.nType == OBJ_PLAYER) {
			wsP = gameData.multiplayer.weaponStates + parentObjP->info.nId;
			if ((wsP->nPrimary != OMEGA_INDEX) || !wsP->firing [0].nStart) {
				Delete (short (handleP - m_handles));
				continue;
				}
			}
		targetObjP = (handleP->nTargetObj >= 0) ? OBJECTS + handleP->nTargetObj : NULL;
		GetGunPoint (parentObjP, &vMuzzle);
		lightningManager.Move (nLightning, &vMuzzle,
									  SPECTATOR (parentObjP) ? gameStates.app.nPlayerSegment : parentObjP->info.nSegment, true, false);
		if (targetObjP)
			lightningManager.Move (nLightning, &targetObjP->info.position.vPos, targetObjP->info.nSegment, true, true);
		}
	handleP++;
	}
return 1;
}

// ---------------------------------------------------------------------------------

#define OMEGA_PLASMA 0

int COmegaLightning::Create (CFixVector *vTargetPos, CObject* parentObjP, CObject* targetObjP)
{
	tOmegaLightningHandles	*handleP;
	int							nObject;

if (!(SHOW_LIGHTNING && gameOpts->render.lightning.bOmega && !gameStates.render.bOmegaModded))
	return 0;
if ((parentObjP->info.nType == OBJ_ROBOT) && (!gameOpts->render.lightning.bRobotOmega || gameStates.app.bHaveMod))
	return 0;
nObject = OBJ_IDX (parentObjP);
if (Update (parentObjP, targetObjP)) {
	if (!(handleP = m_handles + Find (nObject)))
		return 0;
	}
else {
	static tRgbaColorf	color = {0.9f, 0.6f, 0.6f, 0.3f};
	CFixVector	vMuzzle, *vTarget;

	Destroy (nObject);
	GetGunPoint (parentObjP, &vMuzzle);
	handleP = m_handles + m_nHandles++;
	handleP->nParentObj = nObject;
	handleP->nTargetObj = targetObjP ? OBJ_IDX (targetObjP) : -1;
	vTarget = targetObjP ? &targetObjP->info.position.vPos : vTargetPos;
#if OMEGA_PLASMA
	color.alpha = gameOpts->render.lightning.bGlow ? 0.5f : 0.3f;
#endif
	handleP->nLightning =
		lightningManager.Create (10, &vMuzzle, vTarget, NULL, -nObject - 1,
										 -5000, 0, CFixVector::Dist(vMuzzle, *vTarget), I2X (3), 0, 0, 250, 0, 1, 3, 1, 1,
#if OMEGA_PLASMA
										 -((parentObjP != gameData.objs.viewerP) || gameStates.render.bFreeCam || gameStates.render.bChaseCam),
#else
										 -1,
#endif
										 1, 1, gameOpts->render.lightning.nStyle, &color);
	}
return (handleP->nLightning >= 0);
}

//------------------------------------------------------------------------------
//eof
