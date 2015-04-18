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
#include "segmath.h"

#include "objsmoke.h"
#include "transprender.h"
#include "renderthreads.h"
#include "rendermine.h"
#include "error.h"
#include "light.h"
#include "dynlight.h"
#include "ogl_lib.h"
#include "automap.h"

COmegaLightning	omegaLightning;

// ---------------------------------------------------------------------------------

int32_t COmegaLightning::Find (int16_t nObject)
{
	int32_t	i;

for (i = 0; i < m_nHandles; i++)
	if (m_handles [i].nParentObj == nObject)
		return i;
return -1;
}

// ---------------------------------------------------------------------------------

void COmegaLightning::Delete (int16_t nHandle)
{
if (m_nHandles > 0) {
	for (int32_t i = 0; i < 2; i++) {
		if (m_handles [nHandle].nLightning [i] >= 0)
			lightningManager.Destroy (lightningManager.m_emitters + m_handles [nHandle].nLightning [i], NULL);
#if DBG
		else
			m_handles [nHandle].nLightning [i] = -1;
#endif
		}
	if (nHandle < --m_nHandles)
		m_handles [nHandle] = m_handles [m_nHandles];
	memset (m_handles + m_nHandles, 0xff, sizeof (tOmegaLightningHandles));
	}
}

// ---------------------------------------------------------------------------------

void COmegaLightning::Destroy (int16_t nObject)
{
	int32_t	nHandle;

if (nObject < 0) {
	for (nHandle = m_nHandles; nHandle > 0; )
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
	int32_t					bSpectate;
	tObjTransformation	*posP;

if (!objP)
	return NULL;
bSpectate = SPECTATOR (objP);
posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
if ((bSpectate || (objP->info.nId != N_LOCALPLAYER)) &&
	 (vGunPoints = GetGunPoints (objP, 6))) {
	TransformGunPoint (objP, vGunPoints, 6, 0, 0, vMuzzle, NULL);
	}
else {
	*vMuzzle = posP->vPos - posP->mOrient.m.dir.u;
	*vMuzzle += posP->mOrient.m.dir.f * (objP->info.xSize / 4);
	}
return vMuzzle;
}

// ---------------------------------------------------------------------------------

int32_t COmegaLightning::Update (CObject* parentObjP, CObject* targetObjP, CFixVector* vTargetPos)
{
	CFixVector					vMuzzle;
	tOmegaLightningHandles	*handleP;
	CWeaponState*				wsP;
	int32_t						h, i, nHandle, nLightning;
	int16_t						nSegment;

if (!(SHOW_LIGHTNING (1) && gameOpts->render.lightning.bOmega && !gameStates.render.bOmegaModded))
	return -1;
if (m_nHandles < 1)
	return 0;
if ((gameData.omega.xCharge [IsMultiGame] >= MAX_OMEGA_CHARGE) && (0 <= (nHandle = Find (LOCALPLAYER.nObject))))
	Destroy (nHandle);
int16_t nObject = parentObjP ? OBJ_IDX (parentObjP) : -1;
if (nObject < 0) {
	i = 0;
	h = m_nHandles;
	}
else {
	i = Find (OBJ_IDX (parentObjP));
	if (i < 0)
		return 0;
	h = 1;
	m_handles [i].nTargetObj = targetObjP ? OBJ_IDX (targetObjP) : -1;
	}

for (handleP = m_handles + i; h; h--) {
	for (int32_t j = 0; j < 2; j++) {
		if ((nLightning = handleP->nLightning [j]) >= 0) {
			parentObjP = OBJECT (handleP->nParentObj);
			if (parentObjP->info.nType == OBJ_PLAYER) {
				wsP = gameData.multiplayer.weaponStates + parentObjP->info.nId;
				if ((wsP->nPrimary != OMEGA_INDEX) || !wsP->firing [0].nStart) {
					Delete (int16_t (handleP - m_handles));
					continue;
					}
				}
			targetObjP = (handleP->nTargetObj >= 0) ? OBJECT (handleP->nTargetObj) : NULL;
			GetGunPoint (parentObjP, &vMuzzle);
			nSegment = SPECTATOR (parentObjP) ? gameStates.app.nPlayerSegment : parentObjP->info.nSegment;
			lightningManager.Move (nLightning, vMuzzle, nSegment);
			if (targetObjP)
				lightningManager.Move (nLightning, vMuzzle, targetObjP->info.position.vPos, nSegment);
			else if (vTargetPos)
				lightningManager.Move (nLightning, vMuzzle, *vTargetPos, nSegment);
			}
		}
	handleP++;
	}
return 1;
}

// ---------------------------------------------------------------------------------

#define OMEGA_PLASMA 0
#if 0
#	define OMEGA_BOLTS 1
#	define OMEGA_NODES 150
#	define OMEGA_FRAMES 30
#	define OMEGA_LIFE -50000
#else
#	define OMEGA_BOLTS 10
#	define OMEGA_NODES 150
#	define OMEGA_FRAMES 3
#	define OMEGA_LIFE -5000
#endif

static tLightningInfo omegaLightningInfo [2] = {
	{
	OMEGA_LIFE, // nLife
	0, // nDelay
	0, // nLength
	-4, // nAmplitude
	0, // nOffset
	-1, // nWayPoint
	OMEGA_BOLTS, // nBolts
	-1, // nId
	-1, // nTarget
	OMEGA_NODES, // nNodes
	0, // nChildren
	OMEGA_FRAMES, // nFrames
	3, // nWidth
	0, // nAngle
	-1, // nStyle
	1, // nSmoothe
	1, // bClamp
	-1, // bGlow
	1, // bSound
	0, // bRandom
	0, // bInPlane
	1, // bEnabled
	0, // bDirection
	{(uint8_t) (255 * 0.9f), (uint8_t) (255 * 0.6f), (uint8_t) (255 * 0.6f), (uint8_t) (255 * 0.3f)} // color;
	},
	{
	OMEGA_LIFE, // nLife
	0, // nDelay
	0, // nLength
	4, // nAmplitude
	0, // nOffset
	-1, // nWayPoint
	OMEGA_BOLTS, // nBolts
	-1, // nId
	-1, // nTarget
	OMEGA_NODES, // nNodes
	0, // nChildren
	OMEGA_FRAMES, // nFrames
	9, // nWidth
	0, // nAngle
	-1, // nStyle
	1, // nSmoothe
	1, // bClamp
	-1, // bGlow
	1, // bSound
	0, // bRandom
	0, // bInPlane
	1, // bEnabled
	0, // bDirection
	{(uint8_t) (255 * 0.9f), (uint8_t) (255 * 0.6f), (uint8_t) (255 * 0.6f), (uint8_t) (255 * 0.3f)} // color;
	}
};

int32_t COmegaLightning::Create (CFixVector *vTargetPos, CObject* parentObjP, CObject* targetObjP)
{
	tOmegaLightningHandles*	handleP;
	int32_t							nObject;

if (!(SHOW_LIGHTNING (1) && gameOpts->render.lightning.bOmega && !gameStates.render.bOmegaModded))
	return 0;
if ((parentObjP->info.nType == OBJ_ROBOT) && (!gameOpts->render.lightning.bRobotOmega || gameStates.app.bHaveMod))
	return 0;
nObject = OBJ_IDX (parentObjP);
if (Update (parentObjP, targetObjP, vTargetPos)) {
	if (!(handleP = m_handles + Find (nObject)))
		return 0;
	}
else {
#if OMEGA_PLASMA
	static CFloatVector	color = {{{0.9f, 0.6f, 0.6f, 0.3f}}};
#endif
	CFixVector	vMuzzle, *vTarget;

	Destroy (nObject);
	GetGunPoint (parentObjP, &vMuzzle);
	handleP = m_handles + m_nHandles;
	handleP->nParentObj = nObject;
	handleP->nTargetObj = targetObjP ? OBJ_IDX (targetObjP) : -1;
	vTarget = targetObjP ? &targetObjP->info.position.vPos : vTargetPos;
#if OMEGA_PLASMA
	color.Alpha () = gameOpts->render.lightning.bGlow ? 0.5f : 0.3f;
#endif
	handleP->nLightning [0] = lightningManager.Create (omegaLightningInfo [0], &vMuzzle, vTarget, NULL, nObject);
	if (handleP->nLightning [0] < 0)
		handleP->nLightning [1] = -1;
	else {
		m_nHandles++;
		handleP->nLightning [1] = (gameOpts->render.nQuality < 3) ? -1 : lightningManager.Create (omegaLightningInfo [1], &vMuzzle, vTarget, NULL, nObject);
		}
	}
return (handleP->nLightning [0] >= 0);
}

//------------------------------------------------------------------------------
//eof
