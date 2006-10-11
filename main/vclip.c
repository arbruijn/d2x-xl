/* $Id: vclip.c,v 1.5 2003/10/10 09:36:35 btb Exp $ */
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
 * Routines for vclips.
 *
 * Old Log:
 * Revision 1.2  1995/09/14  14:14:31  allender
 * return void in DrawVClipObject
 *
 * Revision 1.1  1995/05/16  15:32:00  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:32:41  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.8  1994/09/25  23:40:52  matt
 * Changed the object load & save code to read/write the structure fields one
 * at a time (rather than the whole structure at once).  This mean that the
 * object structure can be changed without breaking the load/save functions.
 * As a result of this change, the local_object data can be and has been
 * incorporated into the object array.  Also, timeToLive is now a property
 * of all gameData.objs.objects, and the object structure has been otherwise cleaned up.
 *
 * Revision 1.7  1994/09/25  15:45:26  matt
 * Added OBJ_LIGHT, a type of object that casts light
 * Added generalized lifeleft, and moved it to local_object
 *
 * Revision 1.6  1994/09/09  20:05:57  mike
 * Add vclips for weapons.
 *
 * Revision 1.5  1994/06/14  21:14:35  matt
 * Made rod gameData.objs.objects draw lighted or not depending on a parameter, so the
 * materialization effect no longer darkens.
 *
 * Revision 1.4  1994/06/08  18:16:24  john
 * Bunch of new stuff that basically takes constants out of the code
 * and puts them into bitmaps.tbl.
 *
 * Revision 1.3  1994/06/03  10:47:17  matt
 * Made vclips (used by explosions) which can be either rods or blobs, as
 * specified in BITMAPS.TBL.  (This is for the materialization center effect).
 *
 * Revision 1.2  1994/05/11  09:25:25  john
 * Abandoned new vclip system for now because each wallclip, vclip,
 * etc, is different and it would be a huge pain to change all of them.
 *
 * Revision 1.1  1994/05/10  15:21:12  john
 * Initial revision
 *
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: vclip.c,v 1.5 2003/10/10 09:36:35 btb Exp $";
#endif

#include <stdlib.h>

#include "error.h"

#include "inferno.h"
#include "vclip.h"
#include "weapon.h"
#include "laser.h"
#include "hudmsg.h"

//----------------- Variables for video clips -------------------

inline int CurFrame (int nClip, fix timeToLive)
{
vclip	*pvc = gameData.eff.vClips [0] + nClip;
int nFrames = pvc->nFrameCount;
//	iFrame = (nFrames - f2i (FixDiv ((nFrames - 1) * timeToLive, pvc->xTotalTime))) - 1;
int iFrame;
if (timeToLive > pvc->xTotalTime)
	timeToLive = timeToLive % pvc->xTotalTime;
iFrame = (pvc->xTotalTime - timeToLive) / pvc->xFrameTime;
#if 0//def _DEBUG
HUDMessage (0, "%d/%d %d/%d", iFrame, nFrames, timeToLive, pvc->xFrameTime);
#endif
return (iFrame < nFrames) ? iFrame : nFrames - 1;
}

//----------------- Variables for video clips -------------------
//draw an object which renders as a vclip

#define	FIREBALL_ALPHA		0.7
#define	THRUSTER_ALPHA		(1.0 / 3.0)
#define	WEAPON_ALPHA		1.0

void DrawVClipObject(object *objP,fix timeToLive, int lighted, int vclip_num, tRgbColorf *color)
{
	double	ta = 0, alpha = 0;
	vclip		*pvc = gameData.eff.vClips [0] + vclip_num;
	int		nFrames = pvc->nFrameCount;
	int		iFrame = CurFrame (vclip_num, timeToLive);
	int		bThruster = (objP->render_type == RT_THRUSTER) && (objP->mtype.phys_info.flags & PF_WIGGLE);

if (objP->type == OBJ_FIREBALL) {
	if (bThruster) {
		alpha = THRUSTER_ALPHA;
		//if (objP->mtype.phys_info.flags & PF_WIGGLE)	//player ship
			iFrame = nFrames - iFrame - 1;	//render the other way round
		}
	else
		alpha = FIREBALL_ALPHA;
	ta = (double) iFrame / (double) nFrames * alpha;
	alpha = (ta >= 0) ? alpha - ta : alpha + ta;
	}
else if (objP->type == OBJ_WEAPON)
	alpha = WEAPON_ALPHA;
#if 1
if (bThruster)
	glDepthMask (0);
#endif
if (pvc->flags & VF_ROD)
	DrawObjectRodTexPoly (objP, pvc->frames [iFrame], lighted);
else {
	Assert(lighted==0);		//blob cannot now be lighted
	DrawObjectBlob (objP, pvc->frames [0], pvc->frames [iFrame], iFrame, color, (float) alpha);
	}
#if 1
if (bThruster)
	glDepthMask (1);
#endif
}

//------------------------------------------------------------------------------

void DrawWeaponVClip(object *objP)
{
	int	vclip_num;
	fix	modtime,play_time;

	Assert(objP->type == OBJ_WEAPON);
	vclip_num = gameData.weapons.info[objP->id].weapon_vclip;
	modtime = objP->lifeleft;
	play_time = gameData.eff.pVClips[vclip_num].xTotalTime;
	//	Special values for modtime were causing enormous slowdown for omega blobs.
	if (modtime == IMMORTAL_TIME)
		modtime = play_time;
	//	Should cause Omega blobs (which live for one frame) to not always be the same.
	if (modtime == ONE_FRAME_TIME)
		modtime = d_rand();
	if (objP->id == PROXIMITY_ID) {		//make prox bombs spin out of sync
		int objnum = OBJ_IDX (objP);
		modtime += (modtime * (objnum&7)) / 16;	//add variance to spin rate
		while (modtime > play_time)
			modtime -= play_time;
		if ((objnum&1) ^ ((objnum>>1)&1))			//make some spin other way
			modtime = play_time - modtime;
	}
	else {
		while (modtime > play_time)
			modtime -= play_time;
	}

	DrawVClipObject(objP, modtime, 0, vclip_num, gameData.weapons.color + objP->id);
}

//------------------------------------------------------------------------------

#ifndef FAST_FILE_IO
/*
 * reads n vclip structs from a CFILE
 */
int vclip_read_n(vclip *vc, int n, CFILE *fp)
{
	int i, j;

	for (i = 0; i < n; i++) {
		vc[i].xTotalTime = CFReadFix(fp);
		vc[i].nFrameCount = CFReadInt(fp);
		vc[i].xFrameTime = CFReadFix(fp);
		vc[i].flags = CFReadInt(fp);
		vc[i].sound_num = CFReadShort(fp);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			vc[i].frames[j].index = CFReadShort(fp);
		vc[i].light_value = CFReadFix(fp);
	}
	return i;
}
#endif

//------------------------------------------------------------------------------
