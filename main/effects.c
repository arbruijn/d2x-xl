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

/*
 *
 * Special effects, such as rotating fans, electrical walls, and
 * other cool animations.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:24:25  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:32:49  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.21  1995/02/13  20:35:06  john
 * Lintized
 *
 * Revision 1.20  1994/12/10  16:44:50  matt
 * Added debugging code to track down door that turns into rock
 *
 * Revision 1.19  1994/12/06  16:27:14  matt
 * Fixed horrible bug that was referencing tSegment -1
 *
 * Revision 1.18  1994/12/02  23:20:51  matt
 * Reset bitmaps possibly changed by crit clips
 *
 * Revision 1.17  1994/11/14  14:00:19  matt
 * Fixed stupid bug
 *
 * Revision 1.16  1994/11/14  12:42:43  matt
 * Allow holes in effects list
 *
 * Revision 1.15  1994/11/08  21:11:52  matt
 * Added functions to stop & start effects
 *
 * Revision 1.14  1994/10/04  18:59:08  matt
 * Exploding eclips now play eclip while exploding, then switch to static bm
 *
 * Revision 1.13  1994/10/04  15:17:42  matt
 * Took out references to unused constant
 *
 * Revision 1.12  1994/09/29  11:00:01  matt
 * Made eclips (wall animations) not frame-rate dependent (for now)
 *
 * Revision 1.11  1994/09/25  00:40:24  matt
 * Added the ability to make eclips (monitors, fans) which can be blown up
 *
 * Revision 1.10  1994/08/14  23:15:14  matt
 * Added animating bitmap hostages, and cleaned up vclips a bit
 *
 * Revision 1.9  1994/08/05  15:56:04  matt
 * Cleaned up effects system, and added alternate effects for after mine
 * destruction.
 *
 * Revision 1.8  1994/08/01  23:17:21  matt
 * Add support for animating textures on robots
 *
 * Revision 1.7  1994/05/23  15:10:46  yuan
 * Make Eclips read directly...
 * No more need for $EFFECTS list.
 *
 * Revision 1.6  1994/04/06  14:42:44  yuan
 * Adding new powerups.
 *
 * Revision 1.5  1994/03/15  16:31:54  yuan
 * Cleaned up bm-loading code.
 * (And structures)
 *
 * Revision 1.4  1994/03/04  17:09:09  yuan
 * New door stuff.
 *
 * Revision 1.3  1994/01/11  11:18:50  yuan
 * Fixed .nCurFrame
 *
 * Revision 1.2  1994/01/11  10:38:55  yuan
 * Special effects new implementation
 *
 * Revision 1.1  1994/01/10  09:45:29  yuan
 * Initial revision
 *
 *
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
#if 0//def _DEBUG
return ecP->vc.xFrameTime;
#else
if ((ecP->changing_wall_texture < 0) && (ecP->changingObject_texture < 0))
	return ecP->vc.xFrameTime;
else {
	grsBitmap	*bmP = gameData.pig.tex.pBitmaps + ecP->vc.frames [0].index;
	return ((bmP->bmType == BM_TYPE_ALT) && BM_FRAMES (bmP)) ? 
			 (ecP->vc.xFrameTime * ecP->vc.nFrameCount) / BM_FRAMECOUNT (bmP) : 
			 ecP->vc.xFrameTime;
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
	int		i, bD1;
	eclip		*ecP;
	tWallClip		*wcP;
	tVideoClip		*vcP;

for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++)
	for (i = gameData.eff.nEffects [bD1], ecP = gameData.eff.effects [bD1]; i; i--, ecP++)
		//if (ecP->flags & EF_FROMPOG)
			ecP->flags &= ~(EF_ALTFMT | EF_FROMPOG | EF_INITIALIZED);
for (i = gameData.walls.nAnims [gameStates.app.bD1Data], wcP = gameData.walls.pAnims; i; i--, wcP++)
	//if (wcP->flags & WCF_FROMPOG)
		wcP->flags &= ~(WCF_ALTFMT | WCF_FROMPOG | WCF_INITIALIZED);
for (i = gameData.eff.nClips [0], vcP = gameData.eff.vClips [0]; i; i--, vcP++)
	//if (vcP->flags & WCF_FROMPOG)
		vcP->flags &= ~(WCF_ALTFMT | WCF_FROMPOG | WCF_INITIALIZED);
}

// ----------------------------------------------------------------------------

void ResetSpecialEffects()
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
		if (ecP->changing_wall_texture != -1)
			gameData.pig.tex.bmIndex [bD1][ecP->changing_wall_texture] = bmi;
		if (ecP->changingObject_texture != -1)
			gameData.pig.tex.objBmIndex [ecP->changingObject_texture] = bmi;

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
	int			i, j, iBaseFrame, nBmFrames, nFrameStep;

if (!(bmP = FindAnimBaseTex (frameP, nFrames, bIndirect, bObject, &iBaseFrame)))
	return NULL;
if (gameOpts->ogl.bGlTexMerge) {
	OglLoadBmTexture (bmP, 1, 0);
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

	OglLoadBmTexture (bmP, 1, 0);
#ifdef _DEBUG
	if (!BM_FRAMES (bmP))
		OglLoadBmTexture (bmP, 1, 0);
#endif
	nBmFrames = BM_FRAMECOUNT (bmP) - 1;
	if (!nBmFrames)
		return 0;
	if (bmfP = BM_FRAMES (bmP)) {
		nFrameStep = (nBmFrames > nFrames) ? nBmFrames / nFrames : 1;
		for (i = 0; i < nFrames; i++) {
			j = BM_INDEX (frameP, i, bIndirect, bObject);
			bmfP->bm_handle = j;
			hbmP = gameData.pig.tex.pBitmaps + j;
			if (BM_OVERRIDE (hbmP) == bmP)
				BM_OVERRIDE (hbmP) = NULL;	//prevent the root texture from being deleted
			PiggyFreeBitmap (hbmP, j, gameStates.app.bD1Data);
			BM_OVERRIDE (hbmP) = bmfP;
			bmfP += nFrameStep;
			if (bmfP > BM_FRAMES (bmP) + nBmFrames)
				bmfP = BM_FRAMES (bmP) + nBmFrames;
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
//if (gameStates.app.b40fpsTick) 
	{
		grsBitmap		*bmP;
		eclip				*ecP;
		tBitmapIndex	bmi;
		fix				ft;
		int				i, t, nFrames;

	for (i = 0, ecP = gameData.eff.pEffects; 
		  i < gameData.eff.nEffects [gameStates.app.bD1Data]; 
		  i++, ecP++) {
		if ((t = ecP->changing_wall_texture) == -1)
			continue;
		if (ecP->flags & EF_STOPPED)
			continue;
		ecP->time_left -= xEffectTime;
		if (ecP->time_left > 0)
			continue;
		if (!(ft = EffectFrameTime (ecP)))
			continue;
		nFrames = ecP->vc.nFrameCount;
		if (ecP->flags & EF_ALTFMT) {
			if (ecP->flags & EF_INITIALIZED) {
				bmP = BM_OVERRIDE (gameData.pig.tex.pBitmaps + ecP->vc.frames [0].index);
				nFrames = ((bmP->bmType != BM_TYPE_ALT) && BM_PARENT (bmP)) ? BM_FRAMECOUNT (BM_PARENT (bmP)) : BM_FRAMECOUNT (bmP);
				}
			else {
				bmP = SetupHiresAnim ((short *) ecP->vc.frames, nFrames, t, 0, 0, &nFrames);
				if (!bmP)
					ecP->flags &= ~EF_ALTFMT;
				else if (!gameOpts->ogl.bGlTexMerge)
					ecP->flags &= ~EF_ALTFMT;
				else
					ecP->flags |= EF_INITIALIZED;
				}
			}
		while (ft && (ecP->time_left < 0)) {
			ecP->time_left += ft;
			ecP->nCurFrame++;
			if (ecP->nCurFrame >= nFrames) {
				if (ecP->flags & EF_ONE_SHOT) {
#ifdef _DEBUG
					Assert (ecP->nSegment!=-1);
					Assert ((ecP->nSide >= 0) && (ecP->nSide < 6));
					Assert (ecP->dest_bm_num!=0 && gameData.segs.segments [ecP->nSegment].sides [ecP->nSide].nOvlTex);
#endif
					gameData.segs.segments [ecP->nSegment].sides [ecP->nSide].nOvlTex = ecP->dest_bm_num;		//replace with destroyed
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
		else if ((ecP->flags & EF_ALTFMT) && (BM_FRAMECOUNT (bmP) > 1)) {
			OglLoadBmTexture (bmP, 1, 0);
			BM_CURFRAME (bmP) = BM_FRAMES (bmP) + ecP->nCurFrame;
			OglLoadBmTexture (BM_CURFRAME (bmP), 1, 0);
			}
		else {
			bmi = ecP->vc.frames [ecP->nCurFrame];
			gameData.pig.tex.pBmIndex [t] = bmi;
			}
		}

	for (i = 0, ecP = gameData.eff.effects [0]; i < gameData.eff.nEffects [0]; i++, ecP++) {
		if ((t = ecP->changingObject_texture) == -1)
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
					Assert(ecP->dest_bm_num!=0 && gameData.segs.segments [ecP->nSegment].sides [ecP->nSide].nOvlTex);
	#endif
					gameData.segs.segments [ecP->nSegment].sides [ecP->nSide].nOvlTex = ecP->dest_bm_num;		//replace with destoyed
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
			OglLoadBmTexture (bmP, 1, 0);
			if (BM_FRAMES (bmP)) {
				BM_CURFRAME (bmP) = BM_FRAMES (bmP) + ecP->nCurFrame;
				OglLoadBmTexture (BM_CURFRAME (bmP), 1, 0);
				}
			}
		else {
			bmi = ecP->vc.frames [ecP->nCurFrame];
		//*ecP->bm_ptr = &gameData.pig.tex.bitmaps [gameData.eff.effects [0][n].vc.frames [gameData.eff.effects [0][n]..nCurFrame].index];
		//if (ecP->changingObject_texture != -1)
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
		if (ecP->changing_wall_texture != -1)
			gameData.pig.tex.pBmIndex[ecP->changing_wall_texture] = bmi;
		if (ecP->changingObject_texture != -1)
			gameData.pig.tex.objBmIndex [ecP->changingObject_texture] = bmi;
		}
for (i = 0, j = gameData.eff.nEffects [0], ecP = gameData.eff.effects [0]; i < j; i++, ecP++)
	if (! (ecP->flags & EF_CRITICAL)) {
		bmi = ecP->vc.frames [0];
		if (ecP->changing_wall_texture != -1)
			gameData.pig.tex.bmIndex [0][ecP->changing_wall_texture] = bmi;
		if (ecP->changingObject_texture != -1)
			gameData.pig.tex.objBmIndex [ecP->changingObject_texture] = bmi;
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
if (ecP->changing_wall_texture != -1)
	gameData.pig.tex.pBmIndex[ecP->changing_wall_texture] = ecP->vc.frames [0];
if (ecP->changingObject_texture != -1)
	gameData.pig.tex.objBmIndex [ecP->changingObject_texture] = ecP->vc.frames [0];
}

// ----------------------------------------------------------------------------
//restart a stopped effect
void RestartEffect(int effect_num)
{
gameData.eff.pEffects [effect_num].flags &= ~EF_STOPPED;

	//Assert(gameData.eff.pEffects [effect_num].bm_ptr != -1);
}

// ----------------------------------------------------------------------------
#ifndef FAST_FILE_IO
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
	ecP->changing_wall_texture = CFReadShort(fp);
	ecP->changingObject_texture = CFReadShort(fp);
	ecP->flags = CFReadInt(fp);
	ecP->crit_clip = CFReadInt(fp);
	ecP->dest_bm_num = CFReadInt(fp);
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
