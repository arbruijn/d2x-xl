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

#define NEW_FVI_STUFF 1

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "u_mem.h"
#include "error.h"
#include "rle.h"
#include "segmath.h"
#include "interp.h"
#include "hitbox.h"
#include "network.h"
#include "renderlib.h"
#include "visibility.h"

//	-----------------------------------------------------------------------------

int32_t PixelTranspType (int16_t nTexture, int16_t nOrient, int16_t nFrame, fix u, fix v)
{
	CBitmap *pBm;
	int32_t bmx, bmy, w, h, offs;
	uint8_t	c;
#if 0
	tBitmapIndex *pBmIndex;

pBmIndex = gameData.pig.tex.pBmIndex + (nTexture);
LoadTexture (*pBmIndex, 0, gameStates.app.bD1Data);
pBm = BmOverride (gameData.pig.tex.pBitmap + pBmIndex->index);
#else
pBm = LoadFaceBitmap (nTexture, nFrame);
if (!pBm->Buffer ())
	return 0;
#endif
if (pBm->Flags () & BM_FLAG_RLE)
	pBm = rle_expand_texture (pBm);
w = pBm->Width ();
h = ((pBm->Type () == BM_TYPE_ALT) && pBm->Frames ()) ? w : pBm->Height ();
if (nOrient == 0) {
	bmx = ((uint32_t) X2I (u * w)) % w;
	bmy = ((uint32_t) X2I (v * h)) % h;
	}
else if (nOrient == 1) {
	bmx = ((uint32_t) X2I ((I2X (1) - v) * w)) % w;
	bmy = ((uint32_t) X2I (u * h)) % h;
	}
else if (nOrient == 2) {
	bmx = ((uint32_t) X2I ((I2X (1) - u) * w)) % w;
	bmy = ((uint32_t) X2I ((I2X (1) - v) * h)) % h;
	}
else {
	bmx = ((uint32_t) X2I (v * w)) % w;
	bmy = ((uint32_t) X2I ((I2X (1) - u) * h)) % h;
	}
offs = bmy * w + bmx;
if (pBm->Flags () & BM_FLAG_TGA) {
	uint8_t *p;

	if (pBm->BPP () == 3)	//no alpha -> no transparency
		return 0;
	p = pBm->Buffer () + offs * pBm->BPP ();
	// check super transparency color
#if 1
	if ((p [0] == 120) && (p [1] == 88) && (p [2] == 128))
#else
	if ((gameOpts->ogl.bGlTexMerge) ?
	    (p [3] == 1) : ((p [0] == 120) && (p [1] == 88) && (p [2] == 128)))
#endif
		return -1;
	// check alpha
	if (!p [3])
		return 1;
	}
else {
	c = pBm->Buffer () [offs];
	if (c == SUPER_TRANSP_COLOR)
		return -1;
	if (c == TRANSPARENCY_COLOR)
		return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------
//check if a particular pRef on a CWall is a transparent pixel
//returns 1 if can pass though the CWall, else 0
int32_t CSide::CheckForTranspPixel (CFixVector& intersection, int16_t iFace)
{
	fix	u, v;
	int32_t	nTranspType;

HitPointUV (&u, &v, NULL, intersection, iFace);	//	Don't compute light value.
if (m_nOvlTex) {
	nTranspType = PixelTranspType (m_nOvlTex, m_nOvlOrient, m_nFrame, u, v);
	if (nTranspType < 0)
		return 1;
	if (!nTranspType)
		return 0;
	}
nTranspType = PixelTranspType (m_nBaseTex, 0, m_nFrame, u, v) != 0;
return nTranspType;
}

//------------------------------------------------------------------------------

bool CHitQuery::InFoV (CObject *pObj)
{
CFixVector v = *p1 - *p0;
CFixVector::Normalize (v);
fix dot = CFixVector::Dot (v, OBJPOS (pObj)->mOrient.m.dir.f);
return dot >= pObj->FoV ();
}

//------------------------------------------------------------------------------

int32_t CanSeePoint (CObject *pObj, CFixVector *pvSource, CFixVector *pvDest, int16_t nSegment, fix xRad, int32_t nThread)
{
	CHitQuery	hitQuery (FQ_TRANSWALL | FQ_VISIBILITY, pvSource, pvDest, -1, pObj ? pObj->Index () : -1, 1, xRad);
	CHitResult	hitResult;

if (!hitQuery.InFoV (pObj))
	return;

hitQuery.nSegment = FindSegByPos (OBJPOS (pObj)->vPos, pObj->info.nSegment, 1, 0);
int32_t nHitType = FindHitpoint (hitQuery, hitResult, 0, nThread);
return nHitType != HIT_WALL;
}

//	-----------------------------------------------------------------------------
//returns true if pViewer can see CObject

int32_t CanSeeObject (int32_t nObject, int32_t bCheckObjs, int32_t nThread)
{
	CHitQuery	hitQuery (bCheckObjs ? FQ_CHECK_OBJS | FQ_TRANSWALL | FQ_VISIBILITY : FQ_TRANSWALL | FQ_VISIBILITY,
								 &gameData.objData.pViewer->info.position.vPos,
								 &OBJECT (nObject)->info.position.vPos,
								 gameData.objData.pViewer->info.nSegment,
								 gameStates.render.cameras.bActive ? -1 : OBJ_IDX (gameData.objData.pViewer),
								 0, 0,
								 ++gameData.physics.bIgnoreObjFlag
								);
	CHitResult		hitResult;

int32_t nHitType = FindHitpoint (hitQuery, hitResult, 0, nThread);
return bCheckObjs ? (nHitType == HIT_OBJECT) && (hitResult.nObject == nObject) : (nHitType != HIT_WALL);
}

//	-----------------------------------------------------------------------------------------------------------
//	Determine if two OBJECTS are on a line of sight.  If so, return true, else return false.
//	Calls fvi.
int32_t ObjectToObjectVisibility (CObject *pViewer, CObject *pTarget, int32_t transType, int32_t nThread)
{
	CHitQuery	hitQuery;
	CHitResult	hitResult;
	int32_t		fate, nTries = 0, bSpectate = SPECTATOR (pViewer);

do {
	hitQuery.flags = transType | FQ_CHECK_OBJS | FQ_VISIBILITY;
	hitQuery.p0 = &OBJPOS (pViewer)->vPos;
	hitQuery.p1 = &OBJPOS (pTarget)->vPos;
	if (!hitQuery.InFoV (pViewer))
		return false;

	hitQuery.radP0 =
	hitQuery.radP1 = 0x10;
	hitQuery.nObject = OBJ_IDX (pViewer);
	if (nTries++) {
		hitQuery.nSegment	= bSpectate 
								  ? FindSegByPos (gameStates.app.playerPos.vPos, gameStates.app.nPlayerSegment, 1, 0) 
								  : FindSegByPos (pViewer->info.position.vPos, pViewer->info.nSegment, 1, 0);
		if (hitQuery.nSegment < 0) {
			fate = HIT_BAD_P0;
			return false;
			}
		}
	else
		hitQuery.nSegment	= bSpectate ? gameStates.app.nPlayerSegment : pViewer->info.nSegment;
	fate = gameData.segData.SegVis (hitQuery.nSegment, pTarget->Segment ()) ? FindHitpoint (hitQuery, hitResult, 0, nThread) : HIT_WALL;
	}
while ((fate == HIT_BAD_P0) && (nTries < 2));
return (fate == HIT_NONE) || (fate == HIT_BAD_P0) || ((fate == HIT_OBJECT) && (hitResult.nObject == pTarget->Index ()));
}

//	-----------------------------------------------------------------------------

int32_t TargetInLineOfFire (void)
{
#if 0 //DBG
return 0;
#else
	int32_t		nType;
	CHitResult	hitResult;
	CObject*		pObj;

	//see if we can see this CPlayerData

CFixVector vEndPos = gameData.objData.pViewer->info.position.vPos + gameData.objData.pViewer->info.position.mOrient.m.dir.f * I2X (2000);
CHitQuery hitQuery (FQ_CHECK_OBJS | FQ_VISIBLE_OBJS | FQ_IGNORE_POWERUPS | FQ_TRANSWALL | FQ_VISIBILITY,
						  &gameData.objData.pViewer->info.position.vPos,
						  &vEndPos,
						  gameData.objData.pViewer->info.nSegment,
						  OBJ_IDX (gameData.objData.pViewer),
						  0, 0,
						  ++gameData.physics.bIgnoreObjFlag
						  );

int32_t nHitType = FindHitpoint (hitQuery, hitResult);
if (nHitType != HIT_OBJECT)
	return 0;
pObj = OBJECT (hitResult.nObject);
nType = pObj->Type ();
if (nType == OBJ_ROBOT) 
	return int32_t (!pObj->IsGuideBot ());
if (nType == OBJ_REACTOR)
	return 1;
if (nType != OBJ_PLAYER)
	return 0;
if (IsCoopGame)
	return 0;
if (!IsTeamGame)
	return 1;
return GetTeam (gameData.objData.pConsole->info.nId) != GetTeam (pObj->info.nId);
#endif
}

//	-----------------------------------------------------------------------------
//eof
