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

fix EffectFrameTime (tEffectClip *ecP)
{
#if DBG
return ecP->vClipInfo.xFrameTime;
#else
if ((ecP->changingWallTexture < 0) && (ecP->changingObjectTexture < 0))
	return ecP->vClipInfo.xFrameTime;
else {
	CBitmap	*bmP = gameData.pig.tex.bitmapP + ecP->vClipInfo.frames [0].index;
	return (fix) ((((bmP->Type () == BM_TYPE_ALT) && bmP->Frames ()) ? 
					  (ecP->vClipInfo.xFrameTime * ecP->vClipInfo.nFrameCount) / bmP->FrameCount () : 
					  ecP->vClipInfo.xFrameTime) /  gameStates.gameplay.slowmo [0].fSpeed);
	}
#endif
}

// ----------------------------------------------------------------------------

void InitSpecialEffects (void)
{
for (int bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++)
	for (int i = 0; i < gameData.eff.nEffects [gameStates.app.bD1Data]; i++)
		gameData.eff.effects [bD1][i].xTimeLeft = EffectFrameTime (gameData.eff.effects [bD1] + i);
}

// ----------------------------------------------------------------------------

void ResetPogEffects (void)
{
	int				i, bD1;
	tEffectClip		*ecP;
	tWallClip		*wcP;
	tVideoClip		*vcP;

for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++)
	for (i = gameData.eff.nEffects [bD1], ecP = gameData.eff.effects [bD1].Buffer (); i; i--, ecP++) {
		//if (ecP->flags & EF_FROMPOG)
			ecP->flags &= ~(EF_ALTFMT | EF_FROMPOG | EF_INITIALIZED);
			ecP->nCurFrame = 0;
			}
for (i = gameData.walls.nAnims [gameStates.app.bD1Data], wcP = gameData.walls.animP.Buffer (); i; i--, wcP++)
	//if (wcP->flags & WCF_FROMPOG)
		wcP->flags &= ~(WCF_ALTFMT | WCF_FROMPOG | WCF_INITIALIZED);
for (i = gameData.eff.nClips [0], vcP = gameData.eff.vClips [0].Buffer (); i; i--, vcP++)
	//if (vcP->flags & WCF_FROMPOG)
		vcP->flags &= ~(WCF_ALTFMT | WCF_FROMPOG | WCF_INITIALIZED);
}

// ----------------------------------------------------------------------------

void ResetSpecialEffects (void)
{
	tBitmapIndex	bmi;

for (int bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	tEffectClip* ecP = gameData.eff.effects [bD1].Buffer ();
	for (int i = 0; i < gameData.eff.nEffects [bD1]; i++, ecP++) {
		ecP->nSegment = -1;					//clear any active one-shots
		ecP->flags &= ~(EF_STOPPED | EF_ONE_SHOT | EF_INITIALIZED);	//restart any stopped effects
		bmi = ecP->vClipInfo.frames [ecP->nCurFrame = 0];
		//reset bitmap, which could have been changed by a nCritClip
		if (ecP->changingWallTexture != -1)
			gameData.pig.tex.bmIndex [bD1][ecP->changingWallTexture] = bmi;
		if (ecP->changingObjectTexture != -1)
			gameData.pig.tex.objBmIndex [ecP->changingObjectTexture] = bmi;
		}
	}
}

// ----------------------------------------------------------------------------

void CacheObjectEffects (void)
{
for (int bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	tEffectClip* ecP = gameData.eff.effects [bD1].Buffer ();
	for (int i = 0; i < gameData.eff.nEffects [bD1]; i++, ecP++)
		if ((ecP->changingObjectTexture != -1) && !(ecP->flags & EF_ALTFMT))
			for (int j = 0; j < ecP->vClipInfo.nFrameCount; j++)
				LoadBitmap (ecP->vClipInfo.frames [j].index, bD1);
	}	
}

// ----------------------------------------------------------------------------

#define BM_INDEX(_fP,_i,_bI,_bO)	\
			((_bI) ? (_bO) ? gameData.pig.tex.objBmIndex [((_fP) [_i])].index : \
								  gameData.pig.tex.bmIndexP [((_fP) [_i])].index : \
								  ((_fP) [_i]))

CBitmap *FindAnimBaseTex (short *frameP, int nFrames, int bIndirect, int bObject, int *piBaseFrame)
{
	CBitmap*	bmP;

for (int i = 0; i < nFrames; i++) {
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

CBitmap *SetupHiresAnim (short *frameP, int nFrames, int nBaseTex, int bIndirect, int bObject, int *pnFrames, CBitmap* bmP)
{
	CBitmap*	hbmP, * bitmapP;
	int		h, i, j, iBaseFrame, nBmFrames, nFrameStep;

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
		nDbgTexture = nDbgTexture;
#endif
	nBmFrames = bmP->FrameCount ();
	if ((bmfP = bmP->Frames ())) {
		nFrameStep = (nBmFrames > nFrames) ? nBmFrames / nFrames : 1;
		h = min (nFrames, nBmFrames);
		for (i = 0; i < h; i++) {
			j = BM_INDEX (frameP, i, bIndirect, bObject);
			hbmP = (bObject ? gameData.pig.tex.bitmaps [0] : gameData.pig.tex.bitmapP) + j;
			if (hbmP->Override () == bmP)
				hbmP->SetOverride (NULL);	//prevent the root texture from being deleted
			hbmP->Unload (j, gameStates.app.bD1Data);
			hbmP->SetOverride (bmfP);
			bmfP->SetId (j);
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
		tEffectClip*	ecP;
		tBitmapIndex	bmi;
		fix				ft;
		int				i, t, nFrames;

	for (i = 0, ecP = gameData.eff.effectP.Buffer (); i < gameData.eff.nEffects [gameStates.app.bD1Data]; i++, ecP++) {
#if DBG
		if (i == 39)
			i = i;
		for (int j = 0; j < VCLIP_MAX_FRAMES; j++)
			if ((nDbgTexture >= 0) && (ecP->vClipInfo.frames [j].index == nDbgTexture)) {
				nDbgTexture = nDbgTexture;
				break;
				}
#endif
		if ((t = ecP->changingWallTexture) == -1)
			continue;
#if DBG
		if ((nDbgTexture > 0) && (t == nDbgTexture))
			nDbgTexture = nDbgTexture;
#endif
		if (!bSetup) {
			if (ecP->flags & EF_STOPPED)
				continue;
			ecP->xTimeLeft -= (fix) (xEffectTime /  gameStates.gameplay.slowmo [0].fSpeed);
			if (ecP->xTimeLeft > 0)
				continue;
			if (!(ft = EffectFrameTime (ecP)))
				continue;
			}
		nFrames = ecP->vClipInfo.nFrameCount;
		if (ecP->flags & EF_ALTFMT) {
			if ((ecP->flags & EF_INITIALIZED) && (bmP = gameData.pig.tex.bitmapP [ecP->vClipInfo.frames [0].index].Override ())) {
				if (gameOpts->ogl.bGlTexMerge)
					nFrames = bmP->FrameCount ();
				}
			else {
				bmP = SetupHiresAnim (reinterpret_cast<short*> (ecP->vClipInfo.frames), nFrames, t, 0, 0, &nFrames);
				if (!bmP)
					ecP->flags &= ~EF_ALTFMT;
#if 0
				else if (!gameOpts->ogl.bGlTexMerge)
					ecP->flags &= ~EF_ALTFMT;
#endif
				else
					ecP->flags |= EF_INITIALIZED;
				}
			}
		if (bSetup)
			continue;
		while (ft && (ecP->xTimeLeft < 0)) {
			ecP->xTimeLeft += ft;
			if (++(ecP->nCurFrame) >= nFrames) {
				if (ecP->flags & EF_ONE_SHOT) {
#if DBG
					Assert (ecP->nSegment != -1);
					Assert ((ecP->nSide >= 0) && (ecP->nSide < 6));
					Assert (ecP->nDestBm != 0 && SEGMENTS [ecP->nSegment].m_sides [ecP->nSide].m_nOvlTex);
#endif
					SEGMENTS [ecP->nSegment].m_sides [ecP->nSide].m_nOvlTex = ecP->nDestBm;		//replace with destroyed
					ecP->flags &= ~EF_ONE_SHOT;
					ecP->nSegment = -1;		//done with this
					}
				ecP->nCurFrame = 0;
				}
			}
		if (ecP->flags & EF_CRITICAL)
			continue;
#if DBG
		if (gameOpts->ogl.bGlTexMerge && (ecP->flags & EF_ALTFMT) && bmP->FrameCount () && !bmP->Frames ())
			bmP = bmP;
#endif
		if ((ecP->nCritClip != -1) && gameData.reactor.bDestroyed) {
			int n = ecP->nCritClip;
			bmi = gameData.eff.effectP [n].vClipInfo.frames [gameData.eff.effectP [n].nCurFrame];
			gameData.pig.tex.bmIndexP [t] = bmi;
			}
		else if (gameOpts->ogl.bGlTexMerge && (ecP->flags & EF_ALTFMT)) {
			bmP->SetTranspType (-1);
			if (!bmP->Frames ())
				bmP->NeedSetup ();
			bmP->SetupTexture (1, 1);
			CBitmap* bmfP = bmP->SetCurFrame (bmP->Frames () + min (ecP->nCurFrame, bmP->FrameCount () - 1));
			bmfP->SetTranspType (-1);
			bmfP->SetupTexture (1, 1);
			}
		else {
			if ((ecP->flags & EF_ALTFMT) && (ecP->nCurFrame >= nFrames))
				ecP->nCurFrame = 0;
			bmi = ecP->vClipInfo.frames [ecP->nCurFrame];
			gameData.pig.tex.bmIndexP [t] = bmi;
			}
		}

	for (i = 0, ecP = gameData.eff.effects [0].Buffer (); i < gameData.eff.nEffects [0]; i++, ecP++) {
		if ((t = ecP->changingObjectTexture) == -1)
			continue;
		if (!bSetup) {
			if (ecP->flags & EF_STOPPED)
				continue;
			ecP->xTimeLeft -= xEffectTime;
			ft = EffectFrameTime (ecP);
			}
		nFrames = ecP->vClipInfo.nFrameCount;
		if (ecP->flags & EF_ALTFMT) {
			if (ecP->flags & EF_INITIALIZED) {
				bmP = gameData.pig.tex.bitmapP [ecP->vClipInfo.frames [0].index].Override ();
				if (gameOpts->ogl.bGlTexMerge)
					nFrames = bmP->FrameCount ();
				}
			else {
   			bmP = SetupHiresAnim (reinterpret_cast<short*> (ecP->vClipInfo.frames), nFrames, t, 0, 1, &nFrames);
	   		if (!bmP)
		   		ecP->flags &= ~EF_ALTFMT;
			   else if (!gameOpts->ogl.bGlTexMerge)
				   ecP->flags &= ~EF_ALTFMT;
   			else
	   			ecP->flags |= EF_INITIALIZED;
	   		}
			}
		if (bSetup)
			continue;
		while (ft && (ecP->xTimeLeft < 0)) {
			ecP->xTimeLeft += ft;
			ecP->nCurFrame++;
			if (ecP->nCurFrame >= nFrames) {
				if (ecP->flags & EF_ONE_SHOT) {
	#if DBG
					Assert(ecP->nSegment != -1);
					Assert((ecP->nSide >= 0) && (ecP->nSide < 6));
					Assert(ecP->nDestBm !=0 && SEGMENTS [ecP->nSegment].m_sides [ecP->nSide].m_nOvlTex);
	#endif
					SEGMENTS [ecP->nSegment].m_sides [ecP->nSide].m_nOvlTex = ecP->nDestBm;		//replace with destoyed
					ecP->flags &= ~EF_ONE_SHOT;
					ecP->nSegment = -1;		//done with this
					}
				ecP->nCurFrame = 0;
				}
			}
		if (ecP->flags & EF_CRITICAL)
			continue;
		if ((ecP->nCritClip != -1) && gameData.reactor.bDestroyed) {
			int n = ecP->nCritClip;
			bmi = gameData.eff.effects [0][n].vClipInfo.frames [gameData.eff.effects [0][n].nCurFrame];
			gameData.pig.tex.objBmIndex [t] = bmi;
			}
		else if ((ecP->flags & EF_ALTFMT) && bmP->Frames ()) {
			bmP->SetTranspType (3);
			bmP->SetupTexture (1, 1);
			if (bmP->Frames ()) {
				bmP->SetCurFrame (bmP->Frames () + ecP->nCurFrame);
				bmP->CurFrame ()->SetTranspType (3);
				bmP->CurFrame ()->SetupTexture (1, 1);
				}
			}
		else {
			bmi = ecP->vClipInfo.frames [ecP->nCurFrame];
		//*ecP->bm_ptr = &gameData.pig.tex.bitmaps [gameData.eff.effects [0][n].vClipInfo.frames [gameData.eff.effects [0][n]..nCurFrame].index];
		//if (ecP->changingObjectTexture != -1)
			gameData.pig.tex.objBmIndex [t] = bmi;
			}
		}
	xEffectTime = 0;
	}
}

// ----------------------------------------------------------------------------

void RestoreEffectBitmapIcons()
{
	int i,j;
	tEffectClip *ecP;
	tBitmapIndex	bmi;

for (i = 0, j = gameData.eff.nEffects [gameStates.app.bD1Data], ecP = gameData.eff.effectP.Buffer (); i < j; i++, ecP++)
	if (!(ecP->flags & EF_CRITICAL)) {
		bmi = ecP->vClipInfo.frames [0];
		if (ecP->changingWallTexture != -1)
			gameData.pig.tex.bmIndexP[ecP->changingWallTexture] = bmi;
		if (ecP->changingObjectTexture != -1)
			gameData.pig.tex.objBmIndex [ecP->changingObjectTexture] = bmi;
		}
for (i = 0, j = gameData.eff.nEffects [0], ecP = gameData.eff.effects [0].Buffer (); i < j; i++, ecP++)
	if (! (ecP->flags & EF_CRITICAL)) {
		bmi = ecP->vClipInfo.frames [0];
		if (ecP->changingWallTexture != -1)
			gameData.pig.tex.bmIndex [0][ecP->changingWallTexture] = bmi;
		if (ecP->changingObjectTexture != -1)
			gameData.pig.tex.objBmIndex [ecP->changingObjectTexture] = bmi;
		}
			//if (gameData.eff.effectP [i].bm_ptr != -1)
			//	*gameData.eff.effectP [i].bm_ptr = &gameData.pig.tex.bitmaps [gameData.eff.effectP [i].vClipInfo.frames [0].index];
}

// ----------------------------------------------------------------------------
//stop an effect from animating.  Show first frame.
void StopEffect(int effect_num)
{
	tEffectClip *ecP = gameData.eff.effectP + effect_num;
	//Assert(ecP->bm_ptr != -1);
ecP->flags |= EF_STOPPED;
ecP->nCurFrame = 0;
//*ecP->bm_ptr = &gameData.pig.tex.bitmaps [ecP->vClipInfo.frames [0].index];
if (ecP->changingWallTexture != -1)
	gameData.pig.tex.bmIndexP[ecP->changingWallTexture] = ecP->vClipInfo.frames [0];
if (ecP->changingObjectTexture != -1)
	gameData.pig.tex.objBmIndex [ecP->changingObjectTexture] = ecP->vClipInfo.frames [0];
}

// ----------------------------------------------------------------------------
//restart a stopped effect
void RestartEffect(int effect_num)
{
gameData.eff.effectP [effect_num].flags &= ~EF_STOPPED;

	//Assert(gameData.eff.effectP [effect_num].bm_ptr != -1);
}

// ----------------------------------------------------------------------------

/*
 * reads n tEffectClip structs from a CFile
 */

void ReadEffectClip (tEffectClip& ec, CFile& cf)
{
ReadVideoClip (ec.vClipInfo, cf);
ec.xTimeLeft = cf.ReadFix ();
ec.nCurFrame = cf.ReadInt ();
ec.changingWallTexture = cf.ReadShort ();
ec.changingObjectTexture = cf.ReadShort ();
ec.flags = cf.ReadInt ();
ec.nCritClip = cf.ReadInt ();
ec.nDestBm = cf.ReadInt ();
ec.nDestVClip = cf.ReadInt ();
ec.nDestEClip = cf.ReadInt ();
ec.xDestSize = cf.ReadFix ();
ec.nSound = cf.ReadInt ();
ec.nSegment = cf.ReadInt ();
ec.nSide = cf.ReadInt ();
}

int ReadEffectClips (CArray<tEffectClip>& ec, int n, CFile& cf)
{
	int i;

for (i = 0; i < n; i++) {
	ReadEffectClip (ec [i], cf);
	}
return i;
}

// ----------------------------------------------------------------------------
