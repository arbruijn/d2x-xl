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

fix EffectFrameTime (tEffectInfo *effectInfoP)
{
#if DBG
return effectInfoP->animationInfo.xFrameTime;
#else
if ((effectInfoP->changing.nWallTexture < 0) && (effectInfoP->changing.nObjectTexture < 0))
	return effectInfoP->animationInfo.xFrameTime;
else {
	CBitmap	*bmP = gameData.pig.tex.bitmapP + effectInfoP->animationInfo.frames [0].index;
	return (fix) ((((bmP->Type () == BM_TYPE_ALT) && bmP->Frames ()) ? 
					  (effectInfoP->animationInfo.xFrameTime * effectInfoP->animationInfo.nFrameCount) / bmP->FrameCount () : 
					  effectInfoP->animationInfo.xFrameTime) /  gameStates.gameplay.slowmo [0].fSpeed);
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
	tEffectInfo*		effectInfoP;
	tWallEffect*		wallEffectP;
	tAnimationInfo*	animInfoP;

for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++)
	for (i = gameData.effects.nEffects [bD1], effectInfoP = gameData.effects.effects [bD1].Buffer (); i; i--, effectInfoP++) {
		//if (effectInfoP->flags & EF_FROMPOG)
			effectInfoP->flags &= ~(EF_ALTFMT | EF_FROMPOG | EF_INITIALIZED);
			effectInfoP->nCurFrame = 0;
			}
for (i = gameData.walls.nAnims [gameStates.app.bD1Data], wallEffectP = gameData.walls.animP.Buffer (); i; i--, wallEffectP++)
	//if (wallEffectP->flags & WCF_FROMPOG)
		wallEffectP->flags &= ~(WCF_ALTFMT | WCF_FROMPOG | WCF_INITIALIZED);
for (i = gameData.effects.nClips [0], animInfoP = gameData.effects.animations [0].Buffer (); i; i--, animInfoP++)
	//if (animInfoP->flags & WCF_FROMPOG)
		animInfoP->flags &= ~(WCF_ALTFMT | WCF_FROMPOG | WCF_INITIALIZED);
}

// ----------------------------------------------------------------------------

void ResetSpecialEffects (void)
{
	tBitmapIndex	bmi;

for (int32_t bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	tEffectInfo* effectInfoP = gameData.effects.effects [bD1].Buffer ();
	for (int32_t i = 0; i < gameData.effects.nEffects [bD1]; i++, effectInfoP++) {
		effectInfoP->nSegment = -1;					//clear any active one-shots
		effectInfoP->flags &= ~(EF_STOPPED | EF_ONE_SHOT | EF_INITIALIZED);	//restart any stopped effects
		bmi = effectInfoP->animationInfo.frames [effectInfoP->nCurFrame = 0];
		//reset bitmap, which could have been changed by a nCriticalAnimation
		if (effectInfoP->changing.nWallTexture != -1)
			gameData.pig.tex.bmIndex [bD1][effectInfoP->changing.nWallTexture] = bmi;
		if (effectInfoP->changing.nObjectTexture != -1)
			gameData.pig.tex.objBmIndex [effectInfoP->changing.nObjectTexture] = bmi;
		}
	}
}

// ----------------------------------------------------------------------------

void CacheObjectEffects (void)
{
for (int32_t bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	tEffectInfo* effectInfoP = gameData.effects.effects [bD1].Buffer ();
	for (int32_t i = 0; i < gameData.effects.nEffects [bD1]; i++, effectInfoP++)
		if ((effectInfoP->changing.nObjectTexture != -1) && !(effectInfoP->flags & EF_ALTFMT))
			for (int32_t j = 0; j < effectInfoP->animationInfo.nFrameCount; j++)
				LoadTexture (effectInfoP->animationInfo.frames [j].index, 0, bD1);
	}	
}

// ----------------------------------------------------------------------------

#define BM_INDEX(_fP,_i,_bI,_bO)	\
			((_bI) ? (_bO) ? gameData.pig.tex.objBmIndex [((_fP) [_i])].index : \
								  gameData.pig.tex.bmIndexP [((_fP) [_i])].index : \
								  ((_fP) [_i]))

CBitmap *FindAnimBaseTex (int16_t *frameP, int32_t nFrames, int32_t bIndirect, int32_t bObject, int32_t *piBaseFrame)
{
	CBitmap*	bmP;

for (int32_t i = 0; i < nFrames; i++) {
	if (bObject)
		bmP = gameData.pig.tex.bitmaps [0] + BM_INDEX (frameP, i, bIndirect, bObject);
	else
		bmP = gameData.pig.tex.bitmapP + BM_INDEX (frameP, i, bIndirect, bObject);
	if ((bmP = bmP->Override ()) && (bmP->Type () != BM_TYPE_FRAME)) {
		*piBaseFrame = i;
		return bmP;
		}
	}
return NULL;
}


// ----------------------------------------------------------------------------

CBitmap *SetupHiresAnim (int16_t *frameP, int32_t nFrames, int32_t nBaseTex, int32_t bIndirect, int32_t bObject, int32_t *pnFrames, CBitmap* bmP)
{
	CBitmap*	hbmP, * bitmapP;
	int32_t		h, i, j, iBaseFrame, nBmFrames, nFrameStep;

if (!(bmP || (bmP = FindAnimBaseTex (frameP, nFrames, bIndirect, bObject, &iBaseFrame))))
	return NULL;
#if DBG
if (strstr (bmP->Name (), "misc068"))
	bmP = bmP;
#endif
if (bmP->FrameCount () < 2)
	return NULL;
bmP->SetTranspType (-1);
bmP->SetupTexture (1, bObject >= 0);
if (gameOpts->ogl.bGlTexMerge) {
	bitmapP = bObject ? gameData.pig.tex.bitmaps [0].Buffer () : gameData.pig.tex.bitmapP.Buffer ();
	for (i = 0; i < nFrames; i++) {
		j = BM_INDEX (frameP, i, bIndirect, bObject > 0);
		hbmP = bitmapP + j;
		if (hbmP->Override () != bmP) {
			hbmP->FreeHiresAnimation (gameStates.app.bD1Data && !bObject);
			hbmP->SetOverride (bmP);
			}
		}
	*pnFrames = bmP->FrameCount ();
	}
else {
	CBitmap *bmfP, *hbmP;

#if DBG
	if (!bmP->Frames ()) {
		bmP->SetTranspType (3);
		bmP->SetupTexture (1, 1);
		}
	if (nBaseTex == nDbgTexture)
		BRP;
#endif
	nBmFrames = bmP->FrameCount ();
	if ((bmfP = bmP->Frames ())) {
		nFrameStep = (nBmFrames > nFrames) ? nBmFrames / nFrames : 1;
		h = Min (nFrames, nBmFrames);
		for (i = 0; i < h; i++) {
			j = BM_INDEX (frameP, i, bIndirect, bObject);
			hbmP = (bObject ? gameData.pig.tex.bitmaps [0] : gameData.pig.tex.bitmapP) + j;
			if (hbmP->Override () == bmP)
				hbmP->SetOverride (NULL);	//prevent the root texture from being deleted
			hbmP->Unload (j, gameStates.app.bD1Data);
			hbmP->SetOverride (bmfP);
			bmfP->SetKey (j);
			bmfP += nFrameStep;
			}
		}
	else {
		for (i = 0; i < nFrames; i++) {
			j = BM_INDEX (frameP, i, bIndirect, bObject);
			gameData.pig.tex.bitmapP [j].SetOverride (bmP);
			}
		}
	}
return bmP;
}

// ----------------------------------------------------------------------------

void DoSpecialEffects (bool bSetup)
{
	static fix xEffectTime = 0;

xEffectTime += gameData.time.xFrame;
//if (gameStates.app.tick40fps.bTick) 
 {
		CBitmap*			bmP = NULL;
		tEffectInfo*	effectInfoP;
		tBitmapIndex	bmi;
		fix				ft = 0;
		int32_t			i, t, nFrames;

	for (i = 0, effectInfoP = gameData.effects.effectP.Buffer (); i < gameData.effects.nEffects [gameStates.app.bD1Data]; i++, effectInfoP++) {
		if ((t = effectInfoP->changing.nWallTexture) == -1)
			continue;
#if DBG
		if ((nDbgTexture > 0) && (t == nDbgTexture))
			BRP;
#endif
		if (!bSetup) {
			if (effectInfoP->flags & EF_STOPPED)
				continue;
			effectInfoP->xTimeLeft -= (fix) (xEffectTime /  gameStates.gameplay.slowmo [0].fSpeed);
			if (effectInfoP->xTimeLeft > 0)
				continue;
			if (!(ft = EffectFrameTime (effectInfoP)))
				continue;
			}
		nFrames = effectInfoP->animationInfo.nFrameCount;
		if (effectInfoP->flags & EF_ALTFMT) {
			if ((effectInfoP->flags & EF_INITIALIZED) && (bmP = gameData.pig.tex.bitmapP [effectInfoP->animationInfo.frames [0].index].Override ())) {
				if (gameOpts->ogl.bGlTexMerge)
					nFrames = bmP->FrameCount ();
				}
			else {
				bmP = SetupHiresAnim (reinterpret_cast<int16_t*> (effectInfoP->animationInfo.frames), nFrames, t, 0, 0, &nFrames);
				if (!bmP)
					effectInfoP->flags &= ~EF_ALTFMT;
				else
					effectInfoP->flags |= EF_INITIALIZED;
				}
			}
		if (bSetup)
			continue;
		while (ft && (effectInfoP->xTimeLeft < 0)) {
			effectInfoP->xTimeLeft += ft;
			if (++(effectInfoP->nCurFrame) >= nFrames) {
				if (effectInfoP->flags & EF_ONE_SHOT) {
#if DBG
					Assert (effectInfoP->nSegment != -1);
					Assert ((effectInfoP->nSide >= 0) && (effectInfoP->nSide < 6));
					Assert (effectInfoP->destroyed.nTexture != 0 && SEGMENT (effectInfoP->nSegment)->m_sides [effectInfoP->nSide].m_nOvlTex);
#endif
					SEGMENT (effectInfoP->nSegment)->m_sides [effectInfoP->nSide].m_nOvlTex = effectInfoP->destroyed.nTexture;		//replace with destroyed
					effectInfoP->flags &= ~EF_ONE_SHOT;
					effectInfoP->nSegment = -1;		//done with this
					}
				effectInfoP->nCurFrame = 0;
				}
			}
		if (effectInfoP->flags & EF_CRITICAL)
			continue;
#if DBG
		if (gameOpts->ogl.bGlTexMerge && (effectInfoP->flags & EF_ALTFMT) && bmP->FrameCount () && !bmP->Frames ())
			bmP = bmP;
#endif
		if ((effectInfoP->nCriticalAnimation != -1) && gameData.reactor.bDestroyed) {
			int32_t n = effectInfoP->nCriticalAnimation;
			bmi = gameData.effects.effectP [n].animationInfo.frames [gameData.effects.effectP [n].nCurFrame];
			gameData.pig.tex.bmIndexP [t] = bmi;
			}
		else if (gameOpts->ogl.bGlTexMerge && (effectInfoP->flags & EF_ALTFMT)) {
			bmP->SetTranspType (-1);
			if (!bmP->Frames ())
				bmP->NeedSetup ();
			bmP->SetupTexture (1, 1);
			CBitmap* bmfP = bmP->SetCurFrame (bmP->Frames () + Min (effectInfoP->nCurFrame, bmP->FrameCount () - 1));
			bmfP->SetTranspType (-1);
			bmfP->SetupTexture (1, 1);
			}
		else {
			if ((effectInfoP->flags & EF_ALTFMT) && (effectInfoP->nCurFrame >= nFrames))
				effectInfoP->nCurFrame = 0;
			bmi = effectInfoP->animationInfo.frames [effectInfoP->nCurFrame];
			gameData.pig.tex.bmIndexP [t] = bmi;
			}
		}

	for (i = 0, effectInfoP = gameData.effects.effects [0].Buffer (); i < gameData.effects.nEffects [0]; i++, effectInfoP++) {
		if ((t = effectInfoP->changing.nObjectTexture) == -1)
			continue;
		if (!bSetup) {
			if (effectInfoP->flags & EF_STOPPED)
				continue;
			effectInfoP->xTimeLeft -= xEffectTime;
			ft = EffectFrameTime (effectInfoP);
			}
		nFrames = effectInfoP->animationInfo.nFrameCount;
		if (effectInfoP->flags & EF_ALTFMT) {
#if DBG
			if (effectInfoP->animationInfo.frames [0].index == nDbgTexture)
				BRP;
#endif
			if (effectInfoP->flags & EF_INITIALIZED) {
				bmP = gameData.pig.tex.bitmapP [effectInfoP->animationInfo.frames [0].index].Override ();
				if (gameOpts->ogl.bGlTexMerge)
					nFrames = bmP->FrameCount ();
				}
			else {
   			bmP = SetupHiresAnim (reinterpret_cast<int16_t*> (effectInfoP->animationInfo.frames), nFrames, t, 0, 1, &nFrames);
	   		if (!bmP)
		   		effectInfoP->flags &= ~EF_ALTFMT;
			   else if (!gameOpts->ogl.bGlTexMerge)
				   effectInfoP->flags &= ~EF_ALTFMT;
   			else
	   			effectInfoP->flags |= EF_INITIALIZED;
	   		}
			}
		if (bSetup)
			continue;
		while (ft && (effectInfoP->xTimeLeft < 0)) {
			effectInfoP->xTimeLeft += ft;
			effectInfoP->nCurFrame++;
			if (effectInfoP->nCurFrame >= nFrames) {
				if (effectInfoP->flags & EF_ONE_SHOT) {
#if DBG
					Assert(effectInfoP->nSegment != -1);
					Assert((effectInfoP->nSide >= 0) && (effectInfoP->nSide < 6));
					Assert(effectInfoP->destroyed.nTexture !=0 && SEGMENT (effectInfoP->nSegment)->m_sides [effectInfoP->nSide].m_nOvlTex);
#endif
					SEGMENT (effectInfoP->nSegment)->m_sides [effectInfoP->nSide].m_nOvlTex = effectInfoP->destroyed.nTexture;		//replace with destoyed
					effectInfoP->flags &= ~EF_ONE_SHOT;
					effectInfoP->nSegment = -1;		//done with this
					}
				effectInfoP->nCurFrame = 0;
				}
			}
		if (effectInfoP->flags & EF_CRITICAL)
			continue;
		if ((effectInfoP->nCriticalAnimation != -1) && gameData.reactor.bDestroyed) {
			int32_t n = effectInfoP->nCriticalAnimation;
			bmi = gameData.effects.effects [0][n].animationInfo.frames [gameData.effects.effects [0][n].nCurFrame];
			gameData.pig.tex.objBmIndex [t] = bmi;
			}
		else if ((effectInfoP->flags & EF_ALTFMT) && bmP->Frames ()) {
			bmP->SetTranspType (3);
			bmP->SetupTexture (1, 1);
			if (bmP->Frames ()) {
				bmP->SetCurFrame (bmP->Frames () + effectInfoP->nCurFrame);
				bmP->CurFrame ()->SetTranspType (3);
				bmP->CurFrame ()->SetupTexture (1, 1);
				}
			}
		else {
			bmi = effectInfoP->animationInfo.frames [effectInfoP->nCurFrame];
		//*effectInfoP->bm_ptr = &gameData.pig.tex.bitmaps [gameData.effects.effects [0][n].animationInfo.frames [gameData.effects.effects [0][n]..nCurFrame].index];
		//if (effectInfoP->changing.nObjectTexture != -1)
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
	tEffectInfo *effectInfoP;
	tBitmapIndex	bmi;

for (i = 0, j = gameData.effects.nEffects [gameStates.app.bD1Data], effectInfoP = gameData.effects.effectP.Buffer (); i < j; i++, effectInfoP++)
	if (!(effectInfoP->flags & EF_CRITICAL)) {
		bmi = effectInfoP->animationInfo.frames [0];
		if (effectInfoP->changing.nWallTexture != -1)
			gameData.pig.tex.bmIndexP[effectInfoP->changing.nWallTexture] = bmi;
		if (effectInfoP->changing.nObjectTexture != -1)
			gameData.pig.tex.objBmIndex [effectInfoP->changing.nObjectTexture] = bmi;
		}
for (i = 0, j = gameData.effects.nEffects [0], effectInfoP = gameData.effects.effects [0].Buffer (); i < j; i++, effectInfoP++)
	if (! (effectInfoP->flags & EF_CRITICAL)) {
		bmi = effectInfoP->animationInfo.frames [0];
		if (effectInfoP->changing.nWallTexture != -1)
			gameData.pig.tex.bmIndex [0][effectInfoP->changing.nWallTexture] = bmi;
		if (effectInfoP->changing.nObjectTexture != -1)
			gameData.pig.tex.objBmIndex [effectInfoP->changing.nObjectTexture] = bmi;
		}
			//if (gameData.effects.effectP [i].bm_ptr != -1)
			//	*gameData.effects.effectP [i].bm_ptr = &gameData.pig.tex.bitmaps [gameData.effects.effectP [i].animationInfo.frames [0].index];
}

// ----------------------------------------------------------------------------
//stop an effect from animating.  Show first frame.
void StopEffect(int32_t effect_num)
{
	tEffectInfo *effectInfoP = gameData.effects.effectP + effect_num;
	//Assert(effectInfoP->bm_ptr != -1);
effectInfoP->flags |= EF_STOPPED;
effectInfoP->nCurFrame = 0;
//*effectInfoP->bm_ptr = &gameData.pig.tex.bitmaps [effectInfoP->animationInfo.frames [0].index];
if (effectInfoP->changing.nWallTexture != -1)
	gameData.pig.tex.bmIndexP[effectInfoP->changing.nWallTexture] = effectInfoP->animationInfo.frames [0];
if (effectInfoP->changing.nObjectTexture != -1)
	gameData.pig.tex.objBmIndex [effectInfoP->changing.nObjectTexture] = effectInfoP->animationInfo.frames [0];
}

// ----------------------------------------------------------------------------
//restart a stopped effect
void RestartEffect(int32_t effect_num)
{
gameData.effects.effectP [effect_num].flags &= ~EF_STOPPED;

	//Assert(gameData.effects.effectP [effect_num].bm_ptr != -1);
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
ec.changing.nObjectTexture = cf.ReadShort ();
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
