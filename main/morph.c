/* $Id: morph.c, v 1.3 2003/01/02 23:31:50 btb Exp $ */
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
 * Morphing code
 *
 * Old Log:
 * Revision 1.5  1995/08/23  21:36:10  allender
 * mcc compiler warnings fixed
 *
 * Revision 1.4  1995/08/12  11:34:19  allender
 * removed #ifdef NEWDEMO -- always in
 *
 * Revision 1.3  1995/07/28  15:39:51  allender
 * removed FixDiv thing
 *
 * Revision 1.2  1995/07/28  15:21:23  allender
 * inverse magnitude fixup thing
 *
 * Revision 1.1  1995/05/16  15:28:05  allender
 * Initial revision
 *
 * Revision 2.1  1995/02/27  18:26:33  john
 * Fixed bug that was caused by externing gameData.models.polyModels, and I had
 * changed the type of it in polyobj.c, thus causing page faults.
 *
 * Revision 2.0  1995/02/27  11:27:44  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.35  1995/02/22  14:45:37  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.34  1995/01/14  19:16:52  john
 * First version of new bitmap paging code.
 *
 * Revision 1.33  1995/01/03  20:38:36  john
 * Externed MAX_MORPH_OBJECTS
 *
 * Revision 1.32  1994/11/17  15:34:04  matt
 * Attempt #4 to fix morph bug
 *
 * Revision 1.31  1994/11/15  10:57:14  matt
 * Tried again to fix morph
 *
 * Revision 1.30  1994/11/14  14:06:45  matt
 * Fixed stupid bug
 *
 * Revision 1.29  1994/11/14  11:55:13  matt
 * Added divide overflow check
 *
 * Revision 1.28  1994/09/26  17:28:14  matt
 * Made new multiple-object morph code work with the demo system
 *
 * Revision 1.27  1994/09/26  15:39:56  matt
 * Allow multiple simultaneous morphing gameData.objPs.objPects
 *
 * Revision 1.26  1994/09/11  22:44:59  mike
 * quick on vecmat function.
 *
 * Revision 1.25  1994/08/26  15:36:00  matt
 * Made eclips usable on more than one object at a time
 *
 * Revision 1.24  1994/07/25  00:02:46  matt
 * Various changes to accomodate new 3d, which no longer takes point numbers
 * as parms, and now only takes pointers to points.
 *
 * Revision 1.23  1994/07/12  12:39:58  matt
 * Revamped physics system
 *
 * Revision 1.22  1994/06/28  11:54:51  john
 * Made newdemo system record/play directly to/from disk, so
 * we don't need the 4 MB buffer anymore.
 *
 * Revision 1.21  1994/06/27  15:53:01  john
 * #define'd out the newdemo stuff
 *
 *
 * Revision 1.20  1994/06/16  14:30:19  matt
 * Moved morph record data call to reder routine
 *
 * Revision 1.19  1994/06/16  13:57:23  matt
 * Added support for morphing gameData.objPs.objPects in demos
 *
 * Revision 1.18  1994/06/16  12:24:23  matt
 * Made robot lighting not mess with gameStates.render.nLighting so robots now night
 * according to this variable.
 *
 * Revision 1.17  1994/06/14  16:55:01  matt
 * Got rid of physics_object speed field
 *
 * Revision 1.16  1994/06/08  21:16:29  matt
 * Made gameData.objPs.objPects spin while morphing
 *
 * Revision 1.15  1994/06/08  18:21:53  matt
 * Made morphing gameData.objPs.objPects light correctly
 *
 * Revision 1.14  1994/06/07  16:50:49  matt
 * Made object lighting work correctly; changed name of Ambient_light to
 * Dynamic_light; cleaned up polygobj object rendering a little.
 *
 * Revision 1.13  1994/06/01  16:33:59  yuan
 * Fixed bug.
 *
 *
 * Revision 1.12  1994/06/01  16:29:08  matt
 * If morph_frame called on object this isn't the morph object, kill it.
 *
 * Revision 1.11  1994/06/01  12:46:34  matt
 * Added needed include
 *
 * Revision 1.10  1994/05/31  22:12:41  matt
 * Set lighting for morph gameData.objPs.objPects
 * Don't let another object start morph while one is morphing, unless
 * that one dies.
 *
 * Revision 1.9  1994/05/31  18:49:53  john
 * Took out debugging //printf's that Matt left in.
 *
 * Revision 1.8  1994/05/30  22:50:22  matt
 * Added morph effect for robots
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: morph.c, v 1.3 2003/01/02 23:31:50 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "texmap.h"
#include "error.h"

#include "inferno.h"
#include "morph.h"
#include "polyobj.h"
#include "game.h"
#include "lighting.h"
#include "newdemo.h"
#include "piggy.h"

#include "mono.h"
#include "bm.h"
#include "interp.h"

//-------------------------------------------------------------
//returns ptr to data for this object, or NULL if none
morph_data *MorphFindData (object *objP)
{
	int i;

#ifdef NEWDEMO
if (gameData.demo.nState == ND_STATE_PLAYBACK) {
	gameData.render.morph.objects [0].objP = objP;
	return &gameData.render.morph.objects [0];
}
#endif
for (i = 0; i < MAX_MORPH_OBJECTS; i++)
	if (gameData.render.morph.objects [i].objP == objP)
		return gameData.render.morph.objects + i;
return NULL;
}

//-------------------------------------------------------------
//takes pmP, fills in min & max
void MorphFindModelBounds (polymodel *pmP, int submodel_num, vms_vector *minv, vms_vector *maxv)
{
	ushort nverts;
	vms_vector *vp;
	ushort *data, type;

data = (ushort *) (pmP->model_data + pmP->submodel_ptrs [submodel_num]);
type = *data++;
Assert (type == 7 || type == 1);
nverts = *data++;
if (type==7)
	data+=2;		//skip start & pad
vp = (vms_vector *) data;
*minv = *maxv = *vp++; 
nverts--;
while (nverts--) {
	if (vp->x > maxv->x) 
		maxv->x = vp->x;
	else if (vp->x < minv->x) 
		minv->x = vp->x;
	if (vp->y > maxv->y) 
		maxv->y = vp->y;
	else if (vp->y < minv->y) 
		minv->y = vp->y;
	if (vp->z > maxv->z) 
		maxv->z = vp->z;
	else if (vp->z < minv->z) 
		minv->z = vp->z;
	vp++;
	}
}

//-------------------------------------------------------------

void MorphInitPoints (polymodel *pmP, vms_vector *box_size, int submodel_num, morph_data *mdP)
{
	ushort nverts;
	vms_vector *vp;
	ushort *data, type;
	int i;

//printf ("initing %d ", submodel_num);
data = (ushort *) &pmP->model_data [pmP->submodel_ptrs [submodel_num]];
type = *data++;
Assert (type == 7 || type == 1);
nverts = *data++;
mdP->n_morphing_points [submodel_num] = 0;
if (type==7) {
	i = *data++;		//get start point number
	data++;				//skip pad
	}
else
	i = 0;				//start at zero
Assert (i+nverts < MAX_VECS);
mdP->submodel_startpoints [submodel_num] = i;
vp = (vms_vector *) data;
while (nverts--) {
	fix k, dist;

	if (box_size) {
		fix t;
		k = 0x7fffffff;
		if (vp->x && f2i (box_size->x)<abs (vp->x)/2 && (t = FixDiv (box_size->x, abs (vp->x))) < k) k=t;
		if (vp->y && f2i (box_size->y)<abs (vp->y)/2 && (t = FixDiv (box_size->y, abs (vp->y))) < k) k=t;
		if (vp->z && f2i (box_size->z)<abs (vp->z)/2 && (t = FixDiv (box_size->z, abs (vp->z))) < k) k=t;
		if (k==0x7fffffff) k=0;
		}
	else
		k=0;
	VmVecCopyScale (&mdP->morph_vecs [i], vp, k);
	dist = VmVecNormalizedDirQuick (&mdP->morph_deltas [i], vp, &mdP->morph_vecs [i]);
	mdP->morph_times [i] = FixDiv (dist, gameData.render.morph.xRate);
	if (mdP->morph_times [i] != 0)
		mdP->n_morphing_points [submodel_num]++;
		VmVecScale (&mdP->morph_deltas [i], gameData.render.morph.xRate);
	vp++; 
	i++;
	}
////printf ("npoints = %d\n", n_morphing_points [submodel_num]);
}

//-------------------------------------------------------------

void MorphUpdatePoints (polymodel *pmP, int submodel_num, morph_data *mdP)
{
	ushort nverts;
	vms_vector *vp;
	ushort *data, type;
	int i;

	////printf ("updating %d ", submodel_num);
data = (ushort *) (pmP->model_data + pmP->submodel_ptrs [submodel_num]);
type = *data++;
Assert (type == 7 || type == 1);
nverts = *data++;
if (type == 7) {
	i = *data++;		//get start point number
	data++;				//skip pad
	}
else
	i = 0;				//start at zero
vp = (vms_vector *) data;
while (nverts--) {
	if (mdP->morph_times [i]) {		//not done yet
		if ((mdP->morph_times [i] -= gameData.app.xFrameTime) <= 0) {
			 mdP->morph_vecs [i] = *vp;
			 mdP->morph_times [i] = 0;
			 mdP->n_morphing_points [submodel_num]--;
			}
		else
			VmVecScaleInc (&mdP->morph_vecs [i], &mdP->morph_deltas [i], gameData.app.xFrameTime);
		}
	vp++; 
	i++;
	}
////printf ("npoints = %d\n", n_morphing_points [submodel_num]);
}


//-------------------------------------------------------------
//process the morphing object for one frame
void DoMorphFrame (object *objP)
{
	int			i;
	polymodel	*pmP;
	morph_data	*mdP;

mdP = MorphFindData (objP);
if (mdP == NULL) {					//maybe loaded half-morphed from disk
	objP->flags |= OF_SHOULD_BE_DEAD;		//.so kill it
	return;
	}
pmP = gameData.models.polyModels + mdP->objP->rtype.pobj_info.model_num;
G3CheckAndSwap (pmP->model_data);
for (i = 0; i < pmP->n_models; i++)
	if (mdP->submodel_active [i] == 1) {
		MorphUpdatePoints (pmP, i, mdP);
		if (mdP->n_morphing_points [i] == 0) {		//maybe start submodel
			int t;
			mdP->submodel_active [i] = 2;		//not animating, just visible
			mdP->n_submodels_active--;		//this one done animating
			for (t=0;t<pmP->n_models;t++)
				if (pmP->submodel_parents [t] == i) {		//start this one
					MorphInitPoints (pmP, NULL, t, mdP);
					mdP->n_submodels_active++;
					mdP->submodel_active [t] = 1;
					}
			}
		}
if (!mdP->n_submodels_active) {			//done morphing!
	object *objP = mdP->objP;
	objP->control_type = mdP->morph_save_control_type;
	objP->movement_type = mdP->morph_save_movement_type;
	objP->render_type = RT_POLYOBJ;
	objP->mtype.phys_info = mdP->morph_save_phys_info;
	mdP->objP = NULL;
	}
}

//-------------------------------------------------------------

vms_vector morph_rotvel = {0x4000, 0x2000, 0x1000};

void MorphInit ()
{
	int i;

for (i=0;i<MAX_MORPH_OBJECTS;i++)
	gameData.render.morph.objects [i].objP = NULL;
}


//-------------------------------------------------------------
//make the object morph
void MorphStart (object *objP)
{
	polymodel *pmP;
	vms_vector pmmin, pmmax;
	vms_vector box_size;
	int i;
	morph_data *mdP = gameData.render.morph.objects;

for (i=0;i<MAX_MORPH_OBJECTS;i++, mdP++)
	if (mdP->objP == NULL || 
			mdP->objP->type==OBJ_NONE  || 
			mdP->objP->signature!=mdP->Morph_sig)
		break;

if (i==MAX_MORPH_OBJECTS)		//no d_free slots
	return;

Assert (objP->render_type == RT_POLYOBJ);
mdP->objP = objP;
mdP->Morph_sig = objP->signature;
mdP->morph_save_control_type = objP->control_type;
mdP->morph_save_movement_type = objP->movement_type;
mdP->morph_save_phys_info = objP->mtype.phys_info;
Assert (objP->control_type == CT_AI);		//morph gameData.objs.objects are also AI gameData.objPs.objPects
objP->control_type = CT_MORPH;
objP->render_type = RT_MORPH;
objP->movement_type = MT_PHYSICS;		//RT_NONE;
objP->mtype.phys_info.rotvel = morph_rotvel;
pmP = gameData.models.polyModels + objP->rtype.pobj_info.model_num;
G3CheckAndSwap (pmP->model_data);
MorphFindModelBounds (pmP, 0, &pmmin, &pmmax);
box_size.x = max (-pmmin.x, pmmax.x) / 2;
box_size.y = max (-pmmin.y, pmmax.y) / 2;
box_size.z = max (-pmmin.z, pmmax.z) / 2;
for (i=0;i<MAX_VECS;i++)		//clear all points
	mdP->morph_times [i] = 0;
for (i=1;i<MAX_SUBMODELS;i++)		//clear all parts
	mdP->submodel_active [i] = 0;
mdP->submodel_active [0] = 1;		//1 means visible & animating
mdP->n_submodels_active = 1;
//now, project points onto surface of box
MorphInitPoints (pmP, &box_size, 0, mdP);
}

//-------------------------------------------------------------

void MorphDrawModel (polymodel *pmP, int submodel_num, vms_angvec *anim_angles, fix light, morph_data *mdP)
{
	int i, mn;
	int facing;
	int sort_list [MAX_SUBMODELS], sort_n;
	//first, sort the submodels

sort_list [0] = submodel_num;
sort_n = 1;
for (i=0;i<pmP->n_models;i++) {
	if (mdP->submodel_active [i] && pmP->submodel_parents [i]==submodel_num) {
		facing = G3CheckNormalFacing (pmP->submodel_pnts+i, pmP->submodel_norms+i);
		if (!facing)
			sort_list [sort_n++] = i;
		else {		//put at start
			int t;
			for (t=sort_n;t>0;t--)
				sort_list [t] = sort_list [t-1];
			sort_list [0] = i;
			sort_n++;
			}
		}
	}

	//now draw everything
for (i=0;i<sort_n;i++) {
	mn = sort_list [i];
	if (mn == submodel_num) {
 		int i;
		for (i=0;i<pmP->n_textures;i++) {
			gameData.models.textureIndex [i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [pmP->first_texture+i]];
			gameData.models.textures [i] = gameData.pig.tex.bitmaps [0] + /*gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [pmP->first_texture+i]]*/gameData.models.textureIndex [i].index;
			}

#ifdef PIGGY_USE_PAGING			
		// Make sure the textures for this object are paged in..
		gameData.pig.tex.bPageFlushed = 0;
		for (i = 0; i < pmP->n_textures;i++)	
			PIGGY_PAGE_IN (gameData.models.textureIndex [i], 0);
		// Hmmm.. cache got flushed in the middle of paging all these in, 
		// so we need to reread them all in.
		if (gameData.pig.tex.bPageFlushed)	{
			gameData.pig.tex.bPageFlushed = 0;
			for (i=0;i<pmP->n_textures;i++)	
				PIGGY_PAGE_IN (gameData.models.textureIndex [i], 0);
			}
			// Make sure that they can all fit in memory.
		Assert ( gameData.pig.tex.bPageFlushed == 0);
#endif
		G3DrawMorphingModel (
			pmP->model_data + pmP->submodel_ptrs [submodel_num], 
			gameData.models.textures, 
			anim_angles, light, 
			mdP->morph_vecs + mdP->submodel_startpoints [submodel_num]);
		}
	else {
		vms_matrix orient;
		VmAngles2Matrix (&orient, anim_angles+mn);
		G3StartInstanceMatrix (pmP->submodel_offsets+mn, &orient);
		MorphDrawModel (pmP, mn, anim_angles, light, mdP);
		G3DoneInstance ();
		}
	}
}

//-------------------------------------------------------------

void MorphDrawObject (object *objP)
{
//	int save_light;
	polymodel *pmP;
	fix light;
	morph_data *mdP;

	mdP = MorphFindData (objP);
	Assert (mdP != NULL);
	Assert (objP->rtype.pobj_info.model_num < gameData.models.nPolyModels);
	pmP=gameData.models.polyModels+objP->rtype.pobj_info.model_num;
	light = ComputeObjectLight (objP, NULL);
	G3StartInstanceMatrix (&objP->pos, &objP->orient);
	G3SetModelPoints (gameData.models.polyModelPoints);
	MorphDrawModel (pmP, 0, objP->rtype.pobj_info.anim_angles, light, mdP);
	G3DoneInstance ();

#ifdef NEWDEMO
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMorphFrame (mdP);
#endif
}

//-------------------------------------------------------------
//eof