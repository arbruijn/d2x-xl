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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "descent.h"
#include "u_mem.h"
#include "textures.h"
#include "error.h"

// ----------------------------------------------------------------------------

fix EffectFrameTime (tEffectInfo *pEffectInfo)
{
#if DBG
return pEffectInfo->animationInfo.xFrameTime;
#else
if ((pEffectInfo->changing.nWallTexture < 0) && (pEffectInfo->changing.nObjectTexture < 0))
	return pEffectInfo->animationInfo.xFrameTime;
else {
	CBitmap	*pBm = gameData.pig.tex.pBitmap + pEffectInfo->animationInfo.frames [0].index;
	return (fix) ((((pBm->Type () == BM_TYPE_ALT) && pBm->Frames ()) ? 
					  (pEffectInfo->animationInfo.xFrameTime * pEffectInfo->animationInfo.nFrameCount) / pBm->FrameCount () : 
					  pEffectInfo->animationInfo.xFrameTime) /  gameStates.gameplay.slowmo [0].fSpeed);
	}
#endif
}

// ----------------------------------------------------------------------------

void InitSpecialEffects (void)
{
for (int32_t bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++)
	for (int32_t i = 0; i < gameData.effects.nEffects [gameStates.app.bD1Data]; i++)
		gameData.effects.effects [bD1][i].xTimeLeft = EffectFrameTime (gameData.effects.effects [bD1] + i);
}

// ----------------------------------------------------------------------------

void ResetPogEffects (void)
{
	int32_t				i, bD1;
	tEffectInfo*		pEffectInfo;
	tWallEffect*		pWallEffect;
	tAnimationInfo*	pAnimInfo;

for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++)
	for (i = gameData.effects.nEffects [bD1], pEffectInfo = gameData.effects.effects [bD1].Buffer (); i; i--, pEffectInfo++) {
		//if (pEffectInfo->flags & EF_FROMPOG)
			pEffectInfo->flags &= ~(EF_ALTFMT | EF_FROMPOG | EF_INITIALIZED);
			pEffectInfo->nCurFrame = 0;
			}
for (i = gameData.wallData.nAnims [gameStates.app.bD1Data], pWallEffect = gameData.wallData.pAnim.Buffer (); i; i--, pWallEffect++)
	//if (pWallEffect->flags & WCF_FROMPOG)
		pWallEffect->flags &= ~(WCF_ALTFMT | WCF_FROMPOG | WCF_INITIALIZED);
for (i = gameData.effects.nClips [0], pAnimInfo = gameData.effects.animations [0].Buffer (); i; i--, pAnimInfo++)
	//if (pAnimInfo->flags & WCF_FROMPOG)
		pAnimInfo->flags &= ~(WCF_ALTFMT | WCF_FROMPOG | WCF_INITIALIZED);
}

// ----------------------------------------------------------------------------

void ResetSpecialEffects (void)
{
	tBitmapIndex	bmi;

for (int32_t bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	tEffectInfo* pEffectInfo = gameData.effects.effects [bD1].Buffer ();
	for (int32_t i = 0; i < gameData.effects.nEffects [bD1]; i++, pEffectInfo++) {
		pEffectInfo->nSegment = -1;					//clear any active one-shots
		pEffectInfo->flags &= ~(EF_STOPPED | EF_ONE_SHOT | EF_INITIALIZED);	//restart any stopped effects
		bmi = pEffectInfo->animationInfo.frames [pEffectInfo->nCurFrame = 0];
		//reset bitmap, which could have been changed by a nCriticalAnimation
		if (pEffectInfo->changing.nWallTexture != -1)
			gameData.pig.tex.bmIndex [bD1][pEffectInfo->changing.nWallTexture] = bmi;
		if (pEffectInfo->changing.nObjectTexture != -1)
			gameData.pig.tex.objBmIndex [pEffectInfo->changing.nObjectTexture] = bmi;
		}
	}
}

// ----------------------------------------------------------------------------

void CacheObjectEffects (void)
{
for (int32_t bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	tEffectInfo* pEffectInfo = gameData.effects.effects [bD1].Buffer ();
	for (int32_t i = 0; i < gameData.effects.nEffects [bD1]; i++, pEffectInfo++)
		if ((pEffectInfo->changing.nObjectTexture != -1) && !(pEffectInfo->flags & EF_ALTFMT))
			for (int32_t j = 0; j < pEffectInfo->animationInfo.nFrameCount; j++)
				LoadTexture (pEffectInfo->animationInfo.frames [j].index, 0, bD1);
	}	
}

// ----------------------------------------------------------------------------

#define BM_INDEX(_fP,_i,_bI,_bO)	\
			((_bI) ? (_bO) ? gameData.pig.tex.objBmIndex [((_fP) [_i])].index : \
								  gameData.pig.tex.pBmIndex [((_fP) [_i])].index : \
								  ((_fP) [_i]))

CBitmap *FindAnimBaseTex (int16_t *pFrame, int32_t nFrames, int32_t bIndirect, int32_t bObject, int32_t *piBaseFrame)
{
	CBitmap*	pBm;

for (int32_t i = 0; i < nFrames; i++) {
	if (bObject)
		pBm = gameData.pig.tex.bitmaps [0] + BM_INDEX (pFrame, i, bIndirect, bObject);
	else
		pBm = gameData.pig.tex.pBitmap + BM_INDEX (pFrame, i, bIndirect, bObject);
	if ((pBm = pBm->Override ()) && (pBm->Type () != BM_TYPE_FRAME)) {
		*piBaseFrame = i;
		return pBm;
		}
	}
return NULL;
}


// ----------------------------------------------------------------------------

CBitmap *SetupHiresAnim (int16_t *pFrame, int32_t nFrames, int32_t nBaseTex, int32_t bIndirect, int32_t bObject, int32_t *pnFrames, CBitmap* pBm)
{
	CBitmap*	pBmh, * pBitmap;
	int32_t	h, i, j, iBaseFrame, nBmFrames, nFrameStep;

if (!(pBm || (pBm = FindAnimBaseTex (pFrame, nFrames, bIndirect, bObject, &iBaseFrame))))
	return NULL;
#if DBG
if (strstr (pBm->Name (), "misc068"))
	pBm = pBm;
#endif
if (pBm->FrameCount () < 2)
	return NULL;
pBm->SetTranspType (-1);
pBm->SetupTexture (1, bObject >= 0);
if (gameOpts->ogl.bGlTexMerge) {
	pBitmap = bObject ? gameData.pig.tex.bitmaps [0].Buffer () : gameData.pig.tex.pBitmap.Buffer ();
	for (i = 0; i < nFrames; i++) {
		j = BM_INDEX (pFrame, i, bIndirect, bObject > 0);
		pBmh = pBitmap + j;
		if (pBmh->Override () != pBm) {
			pBmh->FreeHiresAnimation (gameStates.app.bD1Data && !bObject);
			pBmh->SetOverride (pBm);
			}
		}
	*pnFrames = pBm->FrameCount ();
	}
else {
	CBitmap *pBmf, *pBmh;

#if DBG
	if (!pBm->Frames ()) {
		pBm->SetTranspType (3);
		pBm->SetupTexture (1, 1);
		}
	if (nBaseTex == nDbgTexture)
		BRP;
#endif
	nBmFrames = pBm->FrameCount ();
	if ((pBmf = pBm->Frames ())) {
		nFrameStep = (nBmFrames > nFrames) ? nBmFrames / nFrames : 1;
		h = Min (nFrames, nBmFrames);
		for (i = 0; i < h; i++) {
			j = BM_INDEX (pFrame, i, bIndirect, bObject);
			pBmh = (bObject ? gameData.pig.tex.bitmaps [0] : gameData.pig.tex.pBitmap) + j;
			if (pBmh->Override () == pBm)
				pBmh->SetOverride (NULL);	//prevent the root texture from being deleted
			pBmh->Unload (j, gameStates.app.bD1Data);
			pBmh->SetOverride (pBmf);
			pBmf->SetKey (j);
			pBmf += nFrameStep;
			}
		}
	else {
		for (i = 0; i < nFrames; i++) {
			j = BM_INDEX (pFrame, i, bIndirect, bObject);
			gameData.pig.tex.pBitmap [j].SetOverride (pBm);
			}
		}
	}
return pBm;
}

// ----------------------------------------------------------------------------

void DoSpecialEffects (bool bSetup)
{
	static fix xEffectTime = 0;

xEffectTime += gameData.time.xFrame;
//if (gameStates.app.tick40fps.bTick) 
 {
		CBitmap*			pBm = NULL;
		tEffectInfo*	pEffectInfo;
		tBitmapIndex	bmi;
		fix				ft = 0;
		int32_t			i, t, nFrames;

	for (i = 0, pEffectInfo = gameData.effects.pEffect.Buffer (); i < gameData.effects.nEffects [gameStates.app.bD1Data]; i++, pEffectInfo++) {
		if ((t = pEffectInfo->changing.nWallTexture) == -1)
			continue;
#if DBG
		if ((nDbgTexture > 0) && (t == nDbgTexture))
			BRP;
#endif
		if (!bSetup) {
			if (pEffectInfo->flags & EF_STOPPED)
				continue;
			pEffectInfo->xTimeLeft -= (fix) (xEffectTime /  gameStates.gameplay.slowmo [0].fSpeed);
			if (pEffectInfo->xTimeLeft > 0)
				continue;
			if (!(ft = EffectFrameTime (pEffectInfo)))
				continue;
			}
		nFrames = pEffectInfo->animationInfo.nFrameCount;
		if (pEffectInfo->flags & EF_ALTFMT) {
			if ((pEffectInfo->flags & EF_INITIALIZED) && (pBm = gameData.pig.tex.pBitmap [pEffectInfo->animationInfo.frames [0].index].Override ())) {
				if (gameOpts->ogl.bGlTexMerge)
					nFrames = pBm->FrameCount ();
				}
			else {
				pBm = SetupHiresAnim (reinterpret_cast<int16_t*> (pEffectInfo->animationInfo.frames), nFrames, t, 0, 0, &nFrames);
				if (!pBm)
					pEffectInfo->flags &= ~EF_ALTFMT;
				else
					pEffectInfo->flags |= EF_INITIALIZED;
				}
			}
		if (bSetup)
			continue;
		while (ft && (pEffectInfo->xTimeLeft < 0)) {
			pEffectInfo->xTimeLeft += ft;
			if (++(pEffectInfo->nCurFrame) >= nFrames) {
				if (pEffectInfo->flags & EF_ONE_SHOT) {
#if DBG
					Assert (pEffectInfo->nSegment != -1);
					Assert ((pEffectInfo->nSide >= 0) && (pEffectInfo->nSide < 6));
					Assert (pEffectInfo->destroyed.nTexture != 0 && SEGMENT (pEffectInfo->nSegment)->m_sides [pEffectInfo->nSide].m_nOvlTex);
#endif
					SEGMENT (pEffectInfo->nSegment)->m_sides [pEffectInfo->nSide].m_nOvlTex = pEffectInfo->destroyed.nTexture;		//replace with destroyed
					pEffectInfo->flags &= ~EF_ONE_SHOT;
					pEffectInfo->nSegment = -1;		//done with this
					}
				pEffectInfo->nCurFrame = 0;
				}
			}
		if (pEffectInfo->flags & EF_CRITICAL)
			continue;
#if DBG
		if (gameOpts->ogl.bGlTexMerge && (pEffectInfo->flags & EF_ALTFMT) && pBm->FrameCount () && !pBm->Frames ())
			pBm = pBm;
#endif
		if ((pEffectInfo->nCriticalAnimation != -1) && gameData.reactor.bDestroyed) {
			int32_t n = pEffectInfo->nCriticalAnimation;
			bmi = gameData.effects.pEffect [n].animationInfo.frames [gameData.effects.pEffect [n].nCurFrame];
			gameData.pig.tex.pBmIndex [t] = bmi;
			}
		else if (gameOpts->ogl.bGlTexMerge && (pEffectInfo->flags & EF_ALTFMT)) {
			pBm->SetTranspType (-1);
			if (!pBm->Frames ())
				pBm->NeedSetup ();
			pBm->SetupTexture (1, 1);
			CBitmap* pBmf = pBm->SetCurFrame (pBm->Frames () + Min (pEffectInfo->nCurFrame, pBm->FrameCount () - 1));
			pBmf->SetTranspType (-1);
			pBmf->SetupTexture (1, 1);
			}
		else {
			if ((pEffectInfo->flags & EF_ALTFMT) && (pEffectInfo->nCurFrame >= nFrames))
				pEffectInfo->nCurFrame = 0;
			bmi = pEffectInfo->animationInfo.frames [pEffectInfo->nCurFrame];
			gameData.pig.tex.pBmIndex [t] = bmi;
			}
		}

	for (i = 0, pEffectInfo = gameData.effects.effects [0].Buffer (); i < gameData.effects.nEffects [0]; i++, pEffectInfo++) {
		if ((t = pEffectInfo->changing.nObjectTexture) == -1)
			continue;
		if (!bSetup) {
			if (pEffectInfo->flags & EF_STOPPED)
				continue;
			pEffectInfo->xTimeLeft -= xEffectTime;
			ft = EffectFrameTime (pEffectInfo);
			}
		nFrames = pEffectInfo->animationInfo.nFrameCount;
		if (pEffectInfo->flags & EF_ALTFMT) {
#if DBG
			if (pEffectInfo->animationInfo.frames [0].index == nDbgTexture)
				BRP;
#endif
			if (pEffectInfo->flags & EF_INITIALIZED) {
				pBm = gameData.pig.tex.pBitmap [pEffectInfo->animationInfo.frames [0].index].Override ();
				if (gameOpts->ogl.bGlTexMerge)
					nFrames = pBm->FrameCount ();
				}
			else {
   			pBm = SetupHiresAnim (reinterpret_cast<int16_t*> (pEffectInfo->animationInfo.frames), nFrames, t, 0, 1, &nFrames);
	   		if (!pBm)
		   		pEffectInfo->flags &= ~EF_ALTFMT;
			   else if (!gameOpts->ogl.bGlTexMerge)
				   pEffectInfo->flags &= ~EF_ALTFMT;
   			else
	   			pEffectInfo->flags |= EF_INITIALIZED;
	   		}
			}
		if (bSetup)
			continue;
		while (ft && (pEffectInfo->xTimeLeft < 0)) {
			pEffectInfo->xTimeLeft += ft;
			pEffectInfo->nCurFrame++;
			if (pEffectInfo->nCurFrame >= nFrames) {
				if (pEffectInfo->flags & EF_ONE_SHOT) {
#if DBG
					Assert(pEffectInfo->nSegment != -1);
					Assert((pEffectInfo->nSide >= 0) && (pEffectInfo->nSide < 6));
					Assert(pEffectInfo->destroyed.nTexture !=0 && SEGMENT (pEffectInfo->nSegment)->m_sides [pEffectInfo->nSide].m_nOvlTex);
#endif
					SEGMENT (pEffectInfo->nSegment)->m_sides [pEffectInfo->nSide].m_nOvlTex = pEffectInfo->destroyed.nTexture;		//replace with destoyed
					pEffectInfo->flags &= ~EF_ONE_SHOT;
					pEffectInfo->nSegment = -1;		//done with this
					}
				pEffectInfo->nCurFrame = 0;
				}
			}
		if (pEffectInfo->flags & EF_CRITICAL)
			continue;
		if ((pEffectInfo->nCriticalAnimation != -1) && gameData.reactor.bDestroyed) {
			int32_t n = pEffectInfo->nCriticalAnimation;
			bmi = gameData.effects.effects [0][n].animationInfo.frames [gameData.effects.effects [0][n].nCurFrame];
			gameData.pig.tex.objBmIndex [t] = bmi;
			}
		else if ((pEffectInfo->flags & EF_ALTFMT) && pBm->Frames ()) {
			pBm->SetTranspType (3);
			pBm->SetupTexture (1, 1);
			if (pBm->Frames ()) {
				pBm->SetCurFrame (pBm->Frames () + pEffectInfo->nCurFrame);
				pBm->CurFrame ()->SetTranspType (3);
				pBm->CurFrame ()->SetupTexture (1, 1);
				}
			}
		else {
			bmi = pEffectInfo->animationInfo.frames [pEffectInfo->nCurFrame];
		//*pEffectInfo->bm_ptr = &gameData.pig.tex.bitmaps [gameData.effects.effects [0][n].animationInfo.frames [gameData.effects.effects [0][n]..nCurFrame].index];
		//if (pEffectInfo->changing.nObjectTexture != -1)
			gameData.pig.tex.objBmIndex [t] = bmi;
			}
		}
	xEffectTime = 0;
	}
}

// ----------------------------------------------------------------------------

void RestoreEffectBitmapIcons()
{
	int32_t i,j;
	tEffectInfo *pEffectInfo;
	tBitmapIndex	bmi;

for (i = 0, j = gameData.effects.nEffects [gameStates.app.bD1Data], pEffectInfo = gameData.effects.pEffect.Buffer (); i < j; i++, pEffectInfo++)
	if (!(pEffectInfo->flags & EF_CRITICAL)) {
		bmi = pEffectInfo->animationInfo.frames [0];
		if (pEffectInfo->changing.nWallTexture != -1)
			gameData.pig.tex.pBmIndex[pEffectInfo->changing.nWallTexture] = bmi;
		if (pEffectInfo->changing.nObjectTexture != -1)
			gameData.pig.tex.objBmIndex [pEffectInfo->changing.nObjectTexture] = bmi;
		}
for (i = 0, j = gameData.effects.nEffects [0], pEffectInfo = gameData.effects.effects [0].Buffer (); i < j; i++, pEffectInfo++)
	if (! (pEffectInfo->flags & EF_CRITICAL)) {
		bmi = pEffectInfo->animationInfo.frames [0];
		if (pEffectInfo->changing.nWallTexture != -1)
			gameData.pig.tex.bmIndex [0][pEffectInfo->changing.nWallTexture] = bmi;
		if (pEffectInfo->changing.nObjectTexture != -1)
			gameData.pig.tex.objBmIndex [pEffectInfo->changing.nObjectTexture] = bmi;
		}
			//if (gameData.effects.pEffect [i].bm_ptr != -1)
			//	*gameData.effects.pEffect [i].bm_ptr = &gameData.pig.tex.bitmaps [gameData.effects.pEffect [i].animationInfo.frames [0].index];
}

// ----------------------------------------------------------------------------
//stop an effect from animating.  Show first frame.
void StopEffect(int32_t effect_num)
{
	tEffectInfo *pEffectInfo = gameData.effects.pEffect + effect_num;
	//Assert(pEffectInfo->bm_ptr != -1);
pEffectInfo->flags |= EF_STOPPED;
pEffectInfo->nCurFrame = 0;
//*pEffectInfo->bm_ptr = &gameData.pig.tex.bitmaps [pEffectInfo->animationInfo.frames [0].index];
if (pEffectInfo->changing.nWallTexture != -1)
	gameData.pig.tex.pBmIndex[pEffectInfo->changing.nWallTexture] = pEffectInfo->animationInfo.frames [0];
if (pEffectInfo->changing.nObjectTexture != -1)
	gameData.pig.tex.objBmIndex [pEffectInfo->changing.nObjectTexture] = pEffectInfo->animationInfo.frames [0];
}

// ----------------------------------------------------------------------------
//restart a stopped effect
void RestartEffect(int32_t effect_num)
{
gameData.effects.pEffect [effect_num].flags &= ~EF_STOPPED;

	//Assert(gameData.effects.pEffect [effect_num].bm_ptr != -1);
}

// ----------------------------------------------------------------------------

/*
 * reads n tEffectInfo structs from a CFile
 */

void ReadEffectClip (tEffectInfo& ec, CFile& cf)
{
ReadVideoClip (ec.animationInfo, cf);
ec.xTimeLeft = cf.ReadFix ();
ec.nCurFrame = cf.ReadInt ();
ec.changing.nWallTexture = cf.ReadShort ();
#if DBG
if (ec.changing.nWallTexture < 0)
	BRP;
if (ec.changing.nWallTexture == nDbgTexture)
	BRP;
#endif
ec.changing.nObjectTexture = cf.ReadShort ();
#if DBG
if ((nDbgTexture >= 0) && (nDbgTexture == ec.changing.nObjectTexture))
	BRP;
#endif
ec.flags = cf.ReadInt ();
ec.nCriticalAnimation = cf.ReadInt ();
ec.destroyed.nTexture = cf.ReadInt ();
ec.destroyed.nAnimation = cf.ReadInt ();
ec.destroyed.nEffect = cf.ReadInt ();
ec.destroyed.xSize = cf.ReadFix ();
ec.nSound = cf.ReadInt ();
ec.nSegment = cf.ReadInt ();
ec.nSide = cf.ReadInt ();
}

int32_t ReadEffectInfo (CArray<tEffectInfo>& ec, int32_t n, CFile& cf)
{
	int32_t i;

for (i = 0; i < n; i++) {
	ReadEffectClip (ec [i], cf);
	}
return i;
}

// ----------------------------------------------------------------------------
