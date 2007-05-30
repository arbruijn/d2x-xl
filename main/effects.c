/* $Id: effects.c,v 1.5 2003/10/10 09:36:34 btb Exp $ */
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

#ifdef RCS
static char rcsid [] = "$Id: effects.c,v 1.5 2003/10/10 09:36:34 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "gr.h"
#include "inferno.h"
#include "game.h"
#include "vclip.h"
#include "effects.h"
#include "bm.h"
#include "piggy.h"
#include "mono.h"
#include "u_mem.h"
#include "textures.h"
#include "cntrlcen.h"
#include "ogl_init.h"
#include "error.h"
#include "hudmsg.h"

// ----------------------------------------------------------------------------

fix EffectFrameTime (eclip *ecP)
{
#ifdef _DEBUG
return ecP->vc.xFrameTime;
#else
if ((ecP->changingWallTexture < 0) && (ecP->changingObjectTexture < 0))
	return ecP->vc.xFrameTime;
else {
	grsBitmap	*bmP = gameData.pig.tex.pBitmaps + ecP->vc.frames [0].index;
	return (fix) ((((bmP->bmType == BM_TYPE_ALT) && BM_FRAMES (bmP)) ? 
					  (ecP->vc.xFrameTime * ecP->vc.nFrameCount) / BM_FRAMECOUNT (bmP) : 
					  ecP->vc.xFrameTime) /  gameStates.gameplay.slowmo [0].fSpeed);
	}
#endif
}

// ----------------------------------------------------------------------------

void InitSpecialEffects()
{
	int bD1, i;

for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++)
	for (i = 0;i < gameData.eff.nEffects [gameStates.app.bD1Data]; i++)
		gameData.eff.effects [bD1][i].time_left = EffectFrameTime (gameData.eff.effects [bD1] + i);
}

// ----------------------------------------------------------------------------

void ResetPogEffects (void)
{
	int				i, bD1;
	eclip				*ecP;
	tWallClip		*wcP;
	tVideoClip		*vcP;

for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++)
	for (i = gameData.eff.nEffects [bD1], ecP = gameData.eff.effects [bD1]; i; i--, ecP++) {
		//if (ecP->flags & EF_FROMPOG)
			ecP->flags &= ~(EF_ALTFMT | EF_FROMPOG | EF_INITIALIZED);
			ecP->nCurFrame = 0;
			}
for (i = gameData.walls.nAnims [gameStates.app.bD1Data], wcP = gameData.walls.pAnims; i; i--, wcP++)
	//if (wcP->flags & WCF_FROMPOG)
		wcP->flags &= ~(WCF_ALTFMT | WCF_FROMPOG | WCF_INITIALIZED);
for (i = gameData.eff.nClips [0], vcP = gameData.eff.vClips [0]; i; i--, vcP++)
	//if (vcP->flags & WCF_FROMPOG)
		vcP->flags &= ~(WCF_ALTFMT | WCF_FROMPOG | WCF_INITIALIZED);
}

// ----------------------------------------------------------------------------

void ResetSpecialEffects (void)
{
	int				i, bD1;
	eclip				*ecP;
	tBitmapIndex	bmi;

for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++)
	for (i = 0, ecP = gameData.eff.effects [bD1]; i < gameData.eff.nEffects [bD1]; i++) {
		ecP->nSegment = -1;					//clear any active one-shots
		ecP->flags &= ~(EF_STOPPED | EF_ONE_SHOT | EF_ALTFMT | EF_INITIALIZED);	//restart any stopped effects
		bmi = ecP->vc.frames [ecP->nCurFrame];
		//reset bitmap, which could have been changed by a crit_clip
		if (ecP->changingWallTexture != -1)
			gameData.pig.tex.bmIndex [bD1][ecP->changingWallTexture] = bmi;
		if (ecP->changingObjectTexture != -1)
			gameData.pig.tex.objBmIndex [ecP->changingObjectTexture] = bmi;

	}
}

// ----------------------------------------------------------------------------

#define BM_INDEX(_fP,_i,_bI,_bO)	\
			((_bI) ? (_bO) ? gameData.pig.tex.objBmIndex [((_fP) [_i])].index : \
								  gameData.pig.tex.pBmIndex [((_fP) [_i])].index : \
								  ((_fP) [_i]))

grsBitmap *FindAnimBaseTex (short *frameP, int nFrames, int bIndirect, int bObject, int *piBaseFrame)
{
	grsBitmap	*bmP;
	int			i;

for (i = 0; i < nFrames; i++) {
	if (bObject)
		bmP = gameData.pig.tex.bitmaps [0] + BM_INDEX (frameP, i, bIndirect, bObject);
	else
		bmP = gameData.pig.tex.pBitmaps + BM_INDEX (frameP, i, bIndirect, bObject);
	if ((bmP = BM_OVERRIDE (bmP)) && (bmP->bmType != BM_TYPE_FRAME)) {
		*piBaseFrame = i;
		return bmP;
		}
	}
return NULL;
}


// ----------------------------------------------------------------------------

grsBitmap *SetupHiresAnim (short *frameP, int nFrames, int nBaseTex, int bIndirect, int bObject, int *pnFrames)
{
	grsBitmap	*bmP, *hbmP, *pBitmaps;
	int			h, i, j, iBaseFrame, nBmFrames, nFrameStep;

if (!(bmP = FindAnimBaseTex (frameP, nFrames, bIndirect, bObject, &iBaseFrame)))
	return NULL;
if (gameOpts->ogl.bGlTexMerge) {
	OglLoadBmTexture (bmP, 1, 3);
	pBitmaps = bObject ? gameData.pig.tex.bitmaps [0] : gameData.pig.tex.pBitmaps;
	for (i = 0; i < nFrames; i++) {
		j = BM_INDEX (frameP, i, bIndirect, bObject);
		hbmP = pBitmaps + j;
		if (BM_OVERRIDE (hbmP) != bmP)
			PiggyFreeHiresAnimation (hbmP, gameStates.app.bD1Data && !bObject);
		BM_OVERRIDE (hbmP) = bmP;
		}
	}
else {
	grsBitmap *bmfP, *hbmP;

	OglLoadBmTexture (bmP, 1, 3);
#ifdef _DEBUG
	if (!BM_FRAMES (bmP))
		OglLoadBmTexture (bmP, 1, 3);
#endif
	nBmFrames = BM_FRAMECOUNT (bmP);
	if ((bmfP = BM_FRAMES (bmP))) {
		nFrameStep = (nBmFrames > nFrames) ? nBmFrames / nFrames : 1;
		h = min (nFrames, nBmFrames);
		for (i = 0; i < h; i++) {
			j = BM_INDEX (frameP, i, bIndirect, bObject);
			hbmP = gameData.pig.tex.pBitmaps + j;
			if (BM_OVERRIDE (hbmP) == bmP)
				BM_OVERRIDE (hbmP) = NULL;	//prevent the root texture from being deleted
			PiggyFreeBitmap (hbmP, j, gameStates.app.bD1Data);
			BM_OVERRIDE (hbmP) = bmfP;
			bmfP->bm_handle = j;
			bmfP += nFrameStep;
			}	
		}
	else {
		for (i = 0; i < nFrames; i++) {
			j = BM_INDEX (frameP, i, bIndirect, bObject);
			BM_OVERRIDE (gameData.pig.tex.pBitmaps + j) = bmP;
			}
		}
	}
return bmP;
}

// ----------------------------------------------------------------------------

void DoSpecialEffects()
{
	static fix xEffectTime = 0;

xEffectTime += gameData.time.xFrame;
//if (gameStates.app.tick40fps.bTick) 
	{
		grsBitmap		*bmP = NULL;
		eclip				*ecP;
		tBitmapIndex	bmi;
		fix				ft;
		int				i, t, nFrames;

	for (i = 0, ecP = gameData.eff.pEffects; 
		  i < gameData.eff.nEffects [gameStates.app.bD1Data]; 
		  i++, ecP++) {
		if ((t = ecP->changingWallTexture) == -1)
			continue;
		if (ecP->flags & EF_STOPPED)
			continue;
		ecP->time_left -= (fix) (xEffectTime /  gameStates.gameplay.slowmo [0].fSpeed);
		if (ecP->time_left > 0)
			continue;
		if (!(ft = EffectFrameTime (ecP)))
			continue;
		nFrames = ecP->vc.nFrameCount;
		if (ecP->flags & EF_ALTFMT) {
			if (ecP->flags & EF_INITIALIZED) {
				bmP = BM_OVERRIDE (gameData.pig.tex.pBitmaps + ecP->vc.frames [0].index);
				if (gameOpts->ogl.bGlTexMerge)
					nFrames = ((bmP->bmType != BM_TYPE_ALT) && BM_PARENT (bmP)) ? BM_FRAMECOUNT (BM_PARENT (bmP)) : BM_FRAMECOUNT (bmP);
				}
			else {
				bmP = SetupHiresAnim ((short *) ecP->vc.frames, nFrames, t, 0, 0, &nFrames);
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
		while (ft && (ecP->time_left < 0)) {
			ecP->time_left += ft;
			if (++(ecP->nCurFrame) >= nFrames) {
				if (ecP->flags & EF_ONE_SHOT) {
#ifdef _DEBUG
					Assert (ecP->nSegment != -1);
					Assert ((ecP->nSide >= 0) && (ecP->nSide < 6));
					Assert (ecP->nDestBm != 0 && gameData.segs.segments [ecP->nSegment].sides [ecP->nSide].nOvlTex);
#endif
					gameData.segs.segments [ecP->nSegment].sides [ecP->nSide].nOvlTex = ecP->nDestBm;		//replace with destroyed
					ecP->flags &= ~EF_ONE_SHOT;
					ecP->nSegment = -1;		//done with this
					}
				ecP->nCurFrame = 0;
				}
			}
		if (ecP->flags & EF_CRITICAL)
			continue;
		if ((ecP->crit_clip != -1) && gameData.reactor.bDestroyed) {
			int n = ecP->crit_clip;
			bmi = gameData.eff.pEffects [n].vc.frames [gameData.eff.pEffects [n].nCurFrame];
			gameData.pig.tex.pBmIndex [t] = bmi;
			}
		else if (gameOpts->ogl.bGlTexMerge && (ecP->flags & EF_ALTFMT) && (BM_FRAMECOUNT (bmP) > 1)) {
			OglLoadBmTexture (bmP, 1, 3);
			BM_CURFRAME (bmP) = BM_FRAMES (bmP) + min (ecP->nCurFrame, BM_FRAMECOUNT (bmP) - 1);
			OglLoadBmTexture (BM_CURFRAME (bmP), 1, 3);
			}
		else {
			if ((ecP->flags & EF_ALTFMT) && (ecP->nCurFrame >= nFrames))
				ecP->nCurFrame = 0;
			bmi = ecP->vc.frames [ecP->nCurFrame];
			gameData.pig.tex.pBmIndex [t] = bmi;
			}
		}

	for (i = 0, ecP = gameData.eff.effects [0]; i < gameData.eff.nEffects [0]; i++, ecP++) {
		if ((t = ecP->changingObjectTexture) == -1)
			continue;
		if (ecP->flags & EF_STOPPED)
			continue;
		ecP->time_left -= xEffectTime;
		ft = EffectFrameTime (ecP);
		nFrames = ecP->vc.nFrameCount;
		if (ecP->flags & EF_ALTFMT) {
			bmP = SetupHiresAnim ((short *) ecP->vc.frames, nFrames, t, 0, 1, &nFrames);
			if (!bmP)
				ecP->flags &= ~EF_ALTFMT;
			else if (!gameOpts->ogl.bGlTexMerge)
				ecP->flags &= ~EF_ALTFMT;
			else
				ecP->flags |= EF_INITIALIZED;
			}
		while (ft && (ecP->time_left < 0)) {
			ecP->time_left += ft;
			ecP->nCurFrame++;
			if (ecP->nCurFrame >= nFrames) {
				if (ecP->flags & EF_ONE_SHOT) {
	#ifdef _DEBUG
					Assert(ecP->nSegment!=-1);
					Assert((ecP->nSide >= 0) && (ecP->nSide < 6));
					Assert(ecP->nDestBm!=0 && gameData.segs.segments [ecP->nSegment].sides [ecP->nSide].nOvlTex);
	#endif
					gameData.segs.segments [ecP->nSegment].sides [ecP->nSide].nOvlTex = ecP->nDestBm;		//replace with destoyed
					ecP->flags &= ~EF_ONE_SHOT;
					ecP->nSegment = -1;		//done with this
					}
				ecP->nCurFrame = 0;
				}
			}
		if (ecP->flags & EF_CRITICAL)
			continue;
		if ((ecP->crit_clip != -1) && gameData.reactor.bDestroyed) {
			int n = ecP->crit_clip;
			bmi = gameData.eff.effects [0][n].vc.frames [gameData.eff.effects [0][n].nCurFrame];
			gameData.pig.tex.objBmIndex [t] = bmi;
			}
		else if ((ecP->flags & EF_ALTFMT) && BM_FRAMES (bmP)) {
			OglLoadBmTexture (bmP, 1, 3);
			if (BM_FRAMES (bmP)) {
				BM_CURFRAME (bmP) = BM_FRAMES (bmP) + ecP->nCurFrame;
				OglLoadBmTexture (BM_CURFRAME (bmP), 1, 3);
				}
			}
		else {
			bmi = ecP->vc.frames [ecP->nCurFrame];
		//*ecP->bm_ptr = &gameData.pig.tex.bitmaps [gameData.eff.effects [0][n].vc.frames [gameData.eff.effects [0][n]..nCurFrame].index];
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
	eclip *ecP;
	tBitmapIndex	bmi;
	
for (i=0, j=gameData.eff.nEffects [gameStates.app.bD1Data], ecP = gameData.eff.pEffects;i<j;i++, ecP++)
	if (!(ecP->flags & EF_CRITICAL))	{
		bmi = ecP->vc.frames [0];
		if (ecP->changingWallTexture != -1)
			gameData.pig.tex.pBmIndex[ecP->changingWallTexture] = bmi;
		if (ecP->changingObjectTexture != -1)
			gameData.pig.tex.objBmIndex [ecP->changingObjectTexture] = bmi;
		}
for (i = 0, j = gameData.eff.nEffects [0], ecP = gameData.eff.effects [0]; i < j; i++, ecP++)
	if (! (ecP->flags & EF_CRITICAL)) {
		bmi = ecP->vc.frames [0];
		if (ecP->changingWallTexture != -1)
			gameData.pig.tex.bmIndex [0][ecP->changingWallTexture] = bmi;
		if (ecP->changingObjectTexture != -1)
			gameData.pig.tex.objBmIndex [ecP->changingObjectTexture] = bmi;
		}
			//if (gameData.eff.pEffects [i].bm_ptr != -1)
			//	*gameData.eff.pEffects [i].bm_ptr = &gameData.pig.tex.bitmaps [gameData.eff.pEffects [i].vc.frames [0].index];
}

// ----------------------------------------------------------------------------
//stop an effect from animating.  Show first frame.
void StopEffect(int effect_num)
{
	eclip *ecP = gameData.eff.pEffects + effect_num;
	//Assert(ecP->bm_ptr != -1);
ecP->flags |= EF_STOPPED;
ecP->nCurFrame = 0;
//*ecP->bm_ptr = &gameData.pig.tex.bitmaps [ecP->vc.frames [0].index];
if (ecP->changingWallTexture != -1)
	gameData.pig.tex.pBmIndex[ecP->changingWallTexture] = ecP->vc.frames [0];
if (ecP->changingObjectTexture != -1)
	gameData.pig.tex.objBmIndex [ecP->changingObjectTexture] = ecP->vc.frames [0];
}

// ----------------------------------------------------------------------------
//restart a stopped effect
void RestartEffect(int effect_num)
{
gameData.eff.pEffects [effect_num].flags &= ~EF_STOPPED;

	//Assert(gameData.eff.pEffects [effect_num].bm_ptr != -1);
}

// ----------------------------------------------------------------------------
#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads n eclip structs from a CFILE
 */
int EClipReadN(eclip *ecP, int n, CFILE *fp)
{
	int i = n;

for (; n; n--, ecP++) {
	vclip_read_n(&ecP->vc, 1, fp);
	ecP->time_left = CFReadFix(fp);
	ecP->nCurFrame = CFReadInt(fp);
	ecP->changingWallTexture = CFReadShort(fp);
	ecP->changingObjectTexture = CFReadShort(fp);
	ecP->flags = CFReadInt(fp);
	ecP->crit_clip = CFReadInt(fp);
	ecP->nDestBm = CFReadInt(fp);
	ecP->dest_vclip = CFReadInt(fp);
	ecP->dest_eclip = CFReadInt(fp);
	ecP->dest_size = CFReadFix(fp);
	ecP->nSound = CFReadInt(fp);
	ecP->nSegment = CFReadInt(fp);
	ecP->nSide = CFReadInt(fp);
	}
	return i;
}
#endif

// ----------------------------------------------------------------------------
