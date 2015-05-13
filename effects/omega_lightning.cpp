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

CFixVector *COmegaLightning::GetGunPoint (CObject* pObj, CFixVector *vMuzzle)
{
	CFixVector			*vGunPoints;
	int32_t					bSpectate;
	tObjTransformation	*pPos;

if (!pObj)
	return NULL;
bSpectate = SPECTATOR (pObj);
pPos = bSpectate ? &gameStates.app.playerPos : &pObj->info.position;
if ((bSpectate || (pObj->info.nId != N_LOCALPLAYER)) &&
	 (vGunPoints = GetGunPoints (pObj, 6))) {
	TransformGunPoint (pObj, vGunPoints, 6, 0, 0, vMuzzle, NULL);
	}
else {
	*vMuzzle = pPos->vPos - pPos->mOrient.m.dir.u;
	*vMuzzle += pPos->mOrient.m.dir.f * (pObj->info.xSize / 4);
	}
return vMuzzle;
}

// ---------------------------------------------------------------------------------

int32_t COmegaLightning::Update (CObject* pParentObj, CObject* pTargetObj, CFixVector* vTargetPos)
{
	CFixVector					vMuzzle;
	tOmegaLightningHandles	*pHandle;
	CWeaponState*				pWeaponStates;
	int32_t						h, i, nHandle, nLightning;
	int16_t						nSegment;

if (!(SHOW_LIGHTNING (1) && gameOpts->render.lightning.bOmega && !gameStates.render.bOmegaModded))
	return -1;
if (m_nHandles < 1)
	return 0;
if ((gameData.omega.xCharge [IsMultiGame] >= MAX_OMEGA_CHARGE) && (0 <= (nHandle = Find (LOCALPLAYER.nObject))))
	Destroy (nHandle);
int16_t nObject = pParentObj ? OBJ_IDX (pParentObj) : -1;
if (nObject < 0) {
	i = 0;
	h = m_nHandles;
	}
else {
	i = Find (OBJ_IDX (pParentObj));
	if (i < 0)
		return 0;
	h = 1;
	m_handles [i].nTargetObj = pTargetObj ? OBJ_IDX (pTargetObj) : -1;
	}

for (pHandle = m_handles + i; h; h--) {
	for (int32_t j = 0; j < 2; j++) {
		if ((nLightning = pHandle->nLightning [j]) >= 0) {
			pParentObj = OBJECT (pHandle->nParentObj);
			if (pParentObj->info.nType == OBJ_PLAYER) {
				pWeaponStates = gameData.multiplayer.weaponStates + pParentObj->info.nId;
				if ((pWeaponStates->nPrimary != OMEGA_INDEX) || !pWeaponStates->firing [0].nStart) {
					Delete (int16_t (pHandle - m_handles));
					continue;
					}
				}
			pTargetObj = (pHandle->nTargetObj >= 0) ? OBJECT (pHandle->nTargetObj) : NULL;
			GetGunPoint (pParentObj, &vMuzzle);
			nSegment = SPECTATOR (pParentObj) ? gameStates.app.nPlayerSegment : pParentObj->info.nSegment;
			lightningManager.Move (nLightning, vMuzzle, nSegment);
			if (pTargetObj)
				lightningManager.Move (nLightning, vMuzzle, pTargetObj->info.position.vPos, nSegment);
			else if (vTargetPos)
				lightningManager.Move (nLightning, vMuzzle, *vTargetPos, nSegment);
			}
		}
	pHandle++;
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
	1, // bBlur
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
	1, // bBlur
	1, // bSound
	0, // bRandom
	0, // bInPlane
	1, // bEnabled
	0, // bDirection
	{(uint8_t) (255 * 0.9f), (uint8_t) (255 * 0.6f), (uint8_t) (255 * 0.6f), (uint8_t) (255 * 0.3f)} // color;
	}
};

int32_t COmegaLightning::Create (CFixVector *vTargetPos, CObject* pParentObj, CObject* pTargetObj)
{
	tOmegaLightningHandles*	pHandle;
	int32_t							nObject;

if (!(SHOW_LIGHTNING (1) && gameOpts->render.lightning.bOmega && !gameStates.render.bOmegaModded))
	return 0;
if ((pParentObj->info.nType == OBJ_ROBOT) && (!gameOpts->render.lightning.bRobotOmega || gameStates.app.bHaveMod))
	return 0;
nObject = OBJ_IDX (pParentObj);
if (Update (pParentObj, pTargetObj, vTargetPos)) {
	if (!(pHandle = m_handles + Find (nObject)))
		return 0;
	}
else {
#if OMEGA_PLASMA
	static CFloatVector	color = {{{0.9f, 0.6f, 0.6f, 0.3f}}};
#endif
	CFixVector	vMuzzle, *vTarget;

	Destroy (nObject);
	GetGunPoint (pParentObj, &vMuzzle);
	pHandle = m_handles + m_nHandles;
	pHandle->nParentObj = nObject;
	pHandle->nTargetObj = pTargetObj ? OBJ_IDX (pTargetObj) : -1;
	vTarget = pTargetObj ? &pTargetObj->info.position.vPos : vTargetPos;
#if OMEGA_PLASMA
	color.Alpha () = gameOpts->render.lightning.bGlow ? 0.5f : 0.3f;
#endif
	pHandle->nLightning [0] = lightningManager.Create (omegaLightningInfo [0], &vMuzzle, vTarget, NULL, nObject);
	if (pHandle->nLightning [0] < 0)
		pHandle->nLightning [1] = -1;
	else {
		m_nHandles++;
		pHandle->nLightning [1] = (gameOpts->render.nQuality < 3) ? -1 : lightningManager.Create (omegaLightningInfo [1], &vMuzzle, vTarget, NULL, nObject);
		}
	}
return (pHandle->nLightning [0] >= 0);
}

//------------------------------------------------------------------------------
//eof
