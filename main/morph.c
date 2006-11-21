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
 * changed the nType of it in polyobj.c, thus causing page faults.
 *
 * Revision 2.0  1995/02/27  11:27:44  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.35  1995/02/22  14:45:37  allender
 * remove anonymous unions from tObject structure
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
 * Made new multiple-tObject morph code work with the demo system
 *
 * Revision 1.27  1994/09/26  15:39:56  matt
 * Allow multiple simultaneous morphing gameData.objPs.objPects
 *
 * Revision 1.26  1994/09/11  22:44:59  mike
 * quick on vecmat function.
 *
 * Revision 1.25  1994/08/26  15:36:00  matt
 * Made eclips usable on more than one tObject at a time
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
 * Got rid of physicsObject speed field
 *
 * Revision 1.16  1994/06/08  21:16:29  matt
 * Made gameData.objPs.objPects spin while morphing
 *
 * Revision 1.15  1994/06/08  18:21:53  matt
 * Made morphing gameData.objPs.objPects light correctly
 *
 * Revision 1.14  1994/06/07  16:50:49  matt
 * Made tObject lighting work correctly; changed name of Ambient_light to
 * Dynamic_light; cleaned up polygobj tObject rendering a little.
 *
 * Revision 1.13  1994/06/01  16:33:59  yuan
 * Fixed bug.
 *
 *
 * Revision 1.12  1994/06/01  16:29:08  matt
 * If morph_frame called on tObject this isn't the morph tObject, kill it.
 *
 * Revision 1.11  1994/06/01  12:46:34  matt
 * Added needed include
 *
 * Revision 1.10  1994/05/31  22:12:41  matt
 * Set lighting for morph gameData.objPs.objPects
 * Don't let another tObject start morph while one is morphing, unless
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
//returns ptr to data for this tObject, or NULL if none
tMorphInfo *MorphFindData (tObject *objP)
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
void MorphFindModelBounds (tPolyModel *pmP, int nSubModel, vmsVector *minv, vmsVector *maxv)
{
	ushort nVerts;
	vmsVector *vp;
	ushort *data, nType;

data = (ushort *) (pmP->model_data + pmP->submodel_ptrs [nSubModel]);
nType = *data++;
Assert (nType == 7 || nType == 1);
nVerts = *data++;
if (nType==7)
	data+=2;		//skip start & pad
vp = (vmsVector *) data;
*minv = *maxv = *vp++; 
nVerts--;
while (nVerts--) {
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

int MorphInitPoints (tPolyModel *pmP, vmsVector *vBoxSize, int nSubModel, tMorphInfo *mdP)
{
	ushort		nVerts;
	vmsVector	*vp, v;
	ushort		*data, nType;
	int			i;

//printf ("initing %d ", nSubModel);
data = (ushort *) (pmP->model_data + pmP->submodel_ptrs [nSubModel]);
nType = *data++;
#ifdef _DEBUG
//Assert (nType == 7 || nType == 1);
if (nType != 7 && nType != 1)
	return 0;
#endif
nVerts = *data++;
mdP->nMorphingPoints [nSubModel] = 0;
if (nType==7) {
	i = *data++;		//get start point number
	data++;				//skip pad
	}
else
	i = 0;				//start at zero
Assert (i+nVerts < MAX_VECS);
mdP->submodelStartPoints [nSubModel] = i;
vp = (vmsVector *) data;
v = *vp;
while (nVerts--) {
	fix k, dist;

	if (vBoxSize) {
		fix t;
		k = 0x7fffffff;
		if (v.x && f2i (vBoxSize->x) < abs (v.x)/2 && (t = FixDiv (vBoxSize->x, abs (v.x))) < k) 
			k = t;
		if (v.y && f2i (vBoxSize->y) < abs (v.y)/2 && (t = FixDiv (vBoxSize->y, abs (v.y))) < k) 
			k = t;
		if (v.z && f2i (vBoxSize->z) < abs (v.z)/2 && (t = FixDiv (vBoxSize->z, abs (v.z))) < k) 
			k = t;
		if (k == 0x7fffffff) 
			k = 0;
		}
	else
		k = 0;
	VmVecCopyScale (mdP->vecs + i, vp, k);
	dist = VmVecNormalizedDirQuick (mdP->deltas + i, vp, mdP->vecs + i);
	mdP->times [i] = FixDiv (dist, gameData.render.morph.xRate);
	if (mdP->times [i] != 0)
		mdP->nMorphingPoints [nSubModel]++;
		VmVecScale (&mdP->deltas [i], gameData.render.morph.xRate);
	vp++; 
	i++;
	}
return 1;
////printf ("npoints = %d\n", nMorphingPoints [nSubModel]);
}

//-------------------------------------------------------------

int MorphUpdatePoints (tPolyModel *pmP, int nSubModel, tMorphInfo *mdP)
{
	ushort nVerts;
	vmsVector *vp;
	ushort *data, nType;
	int i;

	////printf ("updating %d ", nSubModel);
data = (ushort *) (pmP->model_data + pmP->submodel_ptrs [nSubModel]);
nType = *data++;
#ifdef _DEBUG
//Assert (nType == 7 || nType == 1);
if (nType != 7 && nType != 1)
	return 0;
#endif
nVerts = *data++;
if (nType == 7) {
	i = *data++;		//get start point number
	data++;				//skip pad
	}
else
	i = 0;				//start at zero
vp = (vmsVector *) data;
while (nVerts--) {
	if (mdP->times [i]) {		//not done yet
		if ((mdP->times [i] -= gameData.time.xFrame) <= 0) {
			 mdP->vecs [i] = *vp;
			 mdP->times [i] = 0;
			 mdP->nMorphingPoints [nSubModel]--;
			}
		else
			VmVecScaleInc (&mdP->vecs [i], &mdP->deltas [i], gameData.time.xFrame);
		}
	vp++; 
	i++;
	}
return 1;
////printf ("npoints = %d\n", nMorphingPoints [nSubModel]);
}


//-------------------------------------------------------------
//process the morphing tObject for one frame
void DoMorphFrame (tObject *objP)
{
	int			i, t;
	tPolyModel	*pmP;
	tMorphInfo	*mdP;

mdP = MorphFindData (objP);
if (mdP == NULL) {					//maybe loaded half-morphed from disk
	objP->flags |= OF_SHOULD_BE_DEAD;		//.so kill it
	return;
	}
pmP = gameData.models.polyModels + mdP->objP->rType.polyObjInfo.nModel;
G3CheckAndSwap (pmP->model_data);
for (i = 0; i < pmP->n_models; i++)
	if (mdP->submodelActive [i] == 1) {
		if (!MorphUpdatePoints (pmP, i, mdP))
			mdP->submodelActive [i] = 0;
      else if (mdP->nMorphingPoints [i] == 0) {		//maybe start submodel
			//int t;
			mdP->submodelActive [i] = 2;		//not animating, just visible
			mdP->nSubmodelsActive--;		//this one done animating
			for (t = 0; t < pmP->n_models; t++)
				if (pmP->submodel_parents [t] == i) {		//start this one
					if (mdP->submodelActive [t] = MorphInitPoints (pmP, NULL, t, mdP))
						mdP->nSubmodelsActive++;
					}
			}
		}
if (!mdP->nSubmodelsActive) {			//done morphing!
	tObject *objP = mdP->objP;
	objP->controlType = mdP->saveControlType;
	objP->movementType = mdP->saveMovementType;
	objP->renderType = RT_POLYOBJ;
	objP->mType.physInfo = mdP->savePhysInfo;
	mdP->objP = NULL;
	}
}

//-------------------------------------------------------------

vmsVector morph_rotvel = {0x4000, 0x2000, 0x1000};

void MorphInit ()
{
	int i;

for (i = 0; i < MAX_MORPH_OBJECTS; i++)
	gameData.render.morph.objects [i].objP = NULL;
}


//-------------------------------------------------------------
//make the tObject morph
void MorphStart (tObject *objP)
{
	tPolyModel *pmP;
	vmsVector pmmin, pmmax;
	vmsVector vBoxSize;
	int i;
	tMorphInfo *mdP = gameData.render.morph.objects;

for (i=0;i<MAX_MORPH_OBJECTS;i++, mdP++)
	if (mdP->objP == NULL || 
			mdP->objP->nType==OBJ_NONE  || 
			mdP->objP->nSignature!=mdP->nSignature)
		break;

if (i==MAX_MORPH_OBJECTS)		//no d_free slots
	return;

Assert (objP->renderType == RT_POLYOBJ);
mdP->objP = objP;
mdP->nSignature = objP->nSignature;
mdP->saveControlType = objP->controlType;
mdP->saveMovementType = objP->movementType;
mdP->savePhysInfo = objP->mType.physInfo;
Assert (objP->controlType == CT_AI);		//morph gameData.objs.objects are also AI gameData.objPs.objPects
objP->controlType = CT_MORPH;
objP->renderType = RT_MORPH;
objP->movementType = MT_PHYSICS;		//RT_NONE;
objP->mType.physInfo.rotVel = morph_rotvel;
pmP = gameData.models.polyModels + objP->rType.polyObjInfo.nModel;
G3CheckAndSwap (pmP->model_data);
MorphFindModelBounds (pmP, 0, &pmmin, &pmmax);
vBoxSize.x = max (-pmmin.x, pmmax.x) / 2;
vBoxSize.y = max (-pmmin.y, pmmax.y) / 2;
vBoxSize.z = max (-pmmin.z, pmmax.z) / 2;
for (i = 0; i < MAX_VECS; i++)		//clear all points
	mdP->times [i] = 0;
for (i = 1; i < MAX_SUBMODELS; i++)		//clear all parts
	mdP->submodelActive [i] = 0;
mdP->submodelActive [0] = 1;		//1 means visible & animating
mdP->nSubmodelsActive = 1;
//now, project points onto surface of box
MorphInitPoints (pmP, &vBoxSize, 0, mdP);
}

//-------------------------------------------------------------

void MorphDrawModel (tPolyModel *pmP, int nSubModel, vmsAngVec *animAngles, fix light, tMorphInfo *mdP)
{
	int i, mn;
	int facing;
	int sort_list [MAX_SUBMODELS], sort_n;
	//first, sort the submodels

sort_list [0] = nSubModel;
sort_n = 1;
for (i=0;i<pmP->n_models;i++) {
	if (mdP->submodelActive [i] && pmP->submodel_parents [i]==nSubModel) {
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
	if (mn == nSubModel) {
 		int i;
		for (i=0;i<pmP->n_textures;i++) {
			gameData.models.textureIndex [i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [pmP->first_texture+i]];
			gameData.models.textures [i] = gameData.pig.tex.bitmaps [0] + /*gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [pmP->first_texture+i]]*/gameData.models.textureIndex [i].index;
			}

#ifdef PIGGY_USE_PAGING			
		// Make sure the textures for this tObject are paged in..
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
			pmP->model_data + pmP->submodel_ptrs [nSubModel], 
			gameData.models.textures, 
			animAngles, light, 
			mdP->vecs + mdP->submodelStartPoints [nSubModel]);
		}
	else {
		vmsMatrix orient;
		VmAngles2Matrix (&orient, animAngles+mn);
		G3StartInstanceMatrix (pmP->submodel_offsets+mn, &orient);
		MorphDrawModel (pmP, mn, animAngles, light, mdP);
		G3DoneInstance ();
		}
	}
}

//-------------------------------------------------------------

void MorphDrawObject (tObject *objP)
{
//	int save_light;
	tPolyModel *pmP;
	fix light;
	tMorphInfo *mdP;

	mdP = MorphFindData (objP);
	Assert (mdP != NULL);
	Assert (objP->rType.polyObjInfo.nModel < gameData.models.nPolyModels);
	pmP=gameData.models.polyModels+objP->rType.polyObjInfo.nModel;
	light = ComputeObjectLight (objP, NULL);
	G3StartInstanceMatrix (&objP->pos, &objP->orient);
	G3SetModelPoints (gameData.models.polyModelPoints);
	MorphDrawModel (pmP, 0, objP->rType.polyObjInfo.animAngles, light, mdP);
	G3DoneInstance ();

#ifdef NEWDEMO
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMorphFrame (mdP);
#endif
}

//-------------------------------------------------------------
//eof