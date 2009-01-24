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

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "inferno.h"
#include "stdlib.h"
#include "texmap.h"
#include "key.h"
#include "gameseg.h"
#include "textures.h"
#include "lightning.h"
#include "objsmoke.h"
#include "physics.h"
#include "slew.h"
#include "render.h"
#include "fireball.h"
#include "error.h"
#include "endlevel.h"
#include "timer.h"
#include "gameseg.h"
#include "collide.h"
#include "dynlight.h"
#include "interp.h"
#include "newdemo.h"
#include "gauges.h"
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
#ifdef TACTILE
#	include "tactile.h"
#endif

//------------------------------------------------------------------------------

void ConvertSmokeObject (CObject *objP)
{
	int			j;
	CTrigger		*trigP;

objP->SetType (OBJ_EFFECT);
objP->info.nId = SMOKE_ID;
objP->info.renderType = RT_SMOKE;
trigP = FindObjTrigger (objP->Index (), TT_SMOKE_LIFE, -1);
#if 1
j = (trigP && trigP->value) ? trigP->value : 5;
objP->rType.particleInfo.nLife = (j * (j + 1)) / 2;
#else
objP->rType.particleInfo.nLife = (trigP && trigP->value) ? trigP->value : 5;
#endif
trigP = FindObjTrigger (objP->Index (), TT_SMOKE_BRIGHTNESS, -1);
objP->rType.particleInfo.nBrightness = (trigP && trigP->value) ? trigP->value * 10 : 75;
trigP = FindObjTrigger (objP->Index (), TT_SMOKE_SPEED, -1);
j = (trigP && trigP->value) ? trigP->value : 5;
#if 1
objP->rType.particleInfo.nSpeed = (j * (j + 1)) / 2;
#else
objP->rType.particleInfo.nSpeed = j;
#endif
trigP = FindObjTrigger (objP->Index (), TT_SMOKE_DENS, -1);
objP->rType.particleInfo.nParts = j * ((trigP && trigP->value) ? trigP->value * 50 : STATIC_SMOKE_MAX_PARTS);
trigP = FindObjTrigger (objP->Index (), TT_SMOKE_DRIFT, -1);
objP->rType.particleInfo.nDrift = (trigP && trigP->value) ? j * trigP->value * 50 : objP->rType.particleInfo.nSpeed * 50;
trigP = FindObjTrigger (objP->Index (), TT_SMOKE_SIZE, -1);
j = (trigP && trigP->value) ? trigP->value : 5;
objP->rType.particleInfo.nSize [0] = j + 1;
objP->rType.particleInfo.nSize [1] = (j * (j + 1)) / 2;
}

//------------------------------------------------------------------------------

void ConvertObjects (void)
{
	CObject	*objP;
	//int		i;

PrintLog ("   converting deprecated smoke objects\n");
FORALL_STATIC_OBJS (objP, i)
	if (objP->info.nType == OBJ_SMOKE)
		ConvertSmokeObject (objP);
}

//------------------------------------------------------------------------------

void CObject::SetupSmoke (void)
{
if ((info.nType != OBJ_EFFECT) || (info.nId != SMOKE_ID))
	return;

	tParticleInfo*	psi = &rType.particleInfo;
	int				nLife, nSpeed, nParts, nSize;

info.renderType = RT_SMOKE;
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
	float faceSize = FaceSize (info.nSegment, psi->nSide - 1);
	psi->nParts = (int) (psi->nParts * ((faceSize < 1) ? sqrt (faceSize) : faceSize));
	if (gameData.segs.nLevelVersion >= 18) {
		if (psi->nType == SMOKE_TYPE_SPRAY)
			psi->nParts *= 4;
		}
	else if ((gameData.segs.nLevelVersion < 18) && IsWaterTexture (SEGMENTS [info.nSegment].m_sides [psi->nSide - 1].m_nBaseTex)) {
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
	CObject	*objP;
	//int		i;

PrintLog ("   setting up effects\n");
FORALL_EFFECT_OBJS (objP, i) 
	if (objP->info.nId == SMOKE_ID)
		objP->SetupSmoke ();
}

//------------------------------------------------------------------------------
//eof
