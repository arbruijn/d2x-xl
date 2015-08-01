#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "descent.h"
#include "stdlib.h"
#include "texmap.h"
#include "key.h"
#include "segmath.h"
#include "textures.h"
#include "lightning.h"
#include "objsmoke.h"
#include "physics.h"
#include "slew.h"
#include "rendermine.h"
#include "fireball.h"
#include "error.h"
#include "endlevel.h"
#include "timer.h"
#include "segmath.h"
#include "collide.h"
#include "dynlight.h"
#include "interp.h"
#include "newdemo.h"
#include "cockpit.h"
#include "text.h"
#include "sphere.h"
#include "input.h"
#include "automap.h"
#include "u_mem.h"
#include "entropy.h"
#include "objrender.h"
#include "dropobject.h"
#include "marker.h"
#include "hiresmodels.h"
#include "loadgame.h"
#include "multi.h"

//------------------------------------------------------------------------------

void ConvertSmokeObject (CObject *pObj)
{
	int32_t		j;
	CTrigger*	pTrigger;

pObj->SetType (OBJ_EFFECT);
pObj->info.nId = PARTICLE_ID;
pObj->info.renderType = RT_PARTICLES;
pTrigger = FindObjTrigger (pObj->Index (), TT_SMOKE_LIFE, -1);
#if 1
j = (pTrigger && pTrigger->m_info.value) ? pTrigger->m_info.value : 5;
pObj->rType.particleInfo.nLife = (j * (j + 1)) / 2;
#else
pObj->rType.particleInfo.nLife = (pTrigger && pTrigger->m_info.value) ? pTrigger->m_info.value : 5;
#endif
pTrigger = FindObjTrigger (pObj->Index (), TT_SMOKE_BRIGHTNESS, -1);
pObj->rType.particleInfo.nBrightness = (pTrigger && pTrigger->m_info.value) ? pTrigger->m_info.value * 10 : 75;
pTrigger = FindObjTrigger (pObj->Index (), TT_SMOKE_SPEED, -1);
j = (pTrigger && pTrigger->m_info.value) ? pTrigger->m_info.value : 5;
#if 1
pObj->rType.particleInfo.nSpeed = (j * (j + 1)) / 2;
#else
pObj->rType.particleInfo.nSpeed = j;
#endif
pTrigger = FindObjTrigger (pObj->Index (), TT_SMOKE_DENS, -1);
pObj->rType.particleInfo.nParts = j * ((pTrigger && pTrigger->m_info.value) ? pTrigger->m_info.value * 50 : STATIC_SMOKE_MAX_PARTS);
pTrigger = FindObjTrigger (pObj->Index (), TT_SMOKE_DRIFT, -1);
pObj->rType.particleInfo.nDrift = (pTrigger && pTrigger->m_info.value) ? j * pTrigger->m_info.value * 50 : pObj->rType.particleInfo.nSpeed * 50;
pTrigger = FindObjTrigger (pObj->Index (), TT_SMOKE_SIZE, -1);
j = (pTrigger && pTrigger->m_info.value) ? pTrigger->m_info.value : 5;
pObj->rType.particleInfo.nSize [0] = j + 1;
pObj->rType.particleInfo.nSize [1] = (j * (j + 1)) / 2;
}

//------------------------------------------------------------------------------

void ConvertObjects (void)
{
	CObject*	pObj = OBJECTS.Buffer ();

PrintLog (0, "converting deprecated smoke objects\n");
FORALL_STATIC_OBJS (pObj)
	if (pObj->info.nType == OBJ_SMOKE)
		ConvertSmokeObject (pObj);
}

//------------------------------------------------------------------------------

void CObject::SetupSmoke (void)
{
if ((info.nType != OBJ_EFFECT) || (info.nId != PARTICLE_ID))
	return;

	tParticleInfo*	psi = &rType.particleInfo;
	int32_t			nLife, nSpeed, nParts, nSize;

info.renderType = RT_PARTICLES;
nLife = psi->nLife ? psi->nLife : 5;
#if 1
psi->nLife = (nLife * (nLife + 1)) / 2;
#else
psi->nLife = psi->value ? psi->value : 5;
#endif
psi->nBrightness = psi->nBrightness ? psi->nBrightness * 10 : 50;
if (!(nSpeed = psi->nSpeed))
	nSpeed = 5;
#if 1
psi->nSpeed = (nSpeed * (nSpeed + 1)) / 2;
#else
psi->nSpeed = i;
#endif
if (!(nParts = psi->nParts))
	nParts = 5;
psi->nDrift = psi->nDrift ? nSpeed * psi->nDrift * 75 : psi->nSpeed * 50;
nSize = psi->nSize [0] ? psi->nSize [0] : 5;
psi->nSize [0] = nSize + 1;
psi->nSize [1] = (nSize * (nSize + 1)) / 2;
psi->nParts = 90 + (nParts * psi->nLife * 3 * (1 << nSpeed)) / (11 - nParts);
if (psi->nSide > 0) {
	float faceSize = SEGMENT (info.nSegment)->FaceSize (psi->nSide - 1);
	psi->nParts = (int32_t) (psi->nParts * ((faceSize < 1) ? sqrt (faceSize) : faceSize));
	if (gameData.segData.nLevelVersion >= 18) {
		if (psi->nType == SMOKE_TYPE_SPRAY)
			psi->nParts *= 4;
		}
	else if ((gameData.segData.nLevelVersion < 18) && IsWaterTexture (SEGMENT (info.nSegment)->m_sides [psi->nSide - 1].m_nBaseTex)) {
		psi->nParts *= 4;
		//psi->nSize [1] /= 2;
		}
	}
#if 1
if (psi->nType == SMOKE_TYPE_BUBBLES) {
	psi->nParts *= 2;
	}
#endif
}

//------------------------------------------------------------------------------

void SetupEffects (void)
{
	CObject*	pObj = OBJECTS.Buffer ();

PrintLog (1, "setting up effects\n");
FORALL_EFFECT_OBJS (pObj) 
	if (pObj->info.nId == PARTICLE_ID)
		pObj->SetupSmoke ();
PrintLog (-1);
}

//------------------------------------------------------------------------------
//eof
