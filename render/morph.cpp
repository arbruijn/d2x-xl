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
#include <string.h>

#include "inferno.h"
#include "texmap.h"
#include "error.h"
#include "light.h"
#include "newdemo.h"
#include "interp.h"

//-------------------------------------------------------------
//returns ptr to data for this CObject, or NULL if none
tMorphInfo *MorphFindData (CObject *objP)
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
//takes pmP, fills in min& max
void MorphFindModelBounds (tPolyModel *pmP, int nSubModel, CFixVector& minv, CFixVector& maxv)
{
	ushort nVerts;
	CFixVector *vp;
	ushort *data, nType;

	data = reinterpret_cast<ushort*> (pmP->modelData + pmP->subModels.ptrs [nSubModel]);
	nType = *data++;
	Assert (nType == 7 || nType == 1);
	nVerts = *data++;
	if (nType==7)
		data+=2;		//skip start & pad
	vp = reinterpret_cast<CFixVector*> (data);
	minv = maxv = *vp++;
	nVerts--;
	while (nVerts--) {
		if ((*vp)[X] > maxv[X])
			maxv[X] = (*vp)[X];
		else if((*vp)[X] < minv[X])
			minv[X] = (*vp)[X];
		if ((*vp)[Y] > maxv[Y])
			maxv[Y] = (*vp)[Y];
		else if ((*vp)[Y] < minv[Y])
			minv[Y] = (*vp)[Y];
		if ((*vp)[Z] > maxv[Z])
			maxv[Z] = (*vp)[Z];
		else if ((*vp)[Z] < minv[Z])
			minv[Z] = (*vp)[Z];
		vp++;
	}
}

//-------------------------------------------------------------

int MorphInitPoints (tPolyModel *pmP, CFixVector *vBoxSize, int nSubModel, tMorphInfo *mdP)
{
	ushort		nVerts;
	CFixVector	*vp, v;
	ushort		*data, nType;
	int			i;

//printf ("initing %d ", nSubModel);
data = reinterpret_cast<ushort*> (pmP->modelData + pmP->subModels.ptrs [nSubModel]);
nType = *data++;
#if DBG
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
vp = reinterpret_cast<CFixVector*> (data);
v = *vp;
while (nVerts--) {
	fix k, dist;

	if (vBoxSize) {
		fix t;
		k = 0x7fffffff;
		if (v[X] && X2I ((*vBoxSize)[X]) < abs (v[X])/2 && (t = FixDiv ((*vBoxSize)[X], abs (v[X]))) < k)
			k = t;
		if (v[Y] && X2I ((*vBoxSize)[Y]) < abs (v[Y])/2 && (t = FixDiv ((*vBoxSize)[Y], abs (v[Y]))) < k)
			k = t;
		if (v[Z] && X2I ((*vBoxSize)[Z]) < abs (v[Z])/2 && (t = FixDiv ((*vBoxSize)[Z], abs (v[Z]))) < k)
			k = t;
		if (k == 0x7fffffff)
			k = 0;
		}
	else
		k = 0;
	mdP->vecs[i] = *vp * k;
	dist = CFixVector::NormalizedDir(mdP->deltas[i], *vp, mdP->vecs[i]);
	mdP->times [i] = FixDiv (dist, gameData.render.morph.xRate);
	if (mdP->times [i] != 0)
		mdP->nMorphingPoints [nSubModel]++;
		mdP->deltas [i] *= gameData.render.morph.xRate;
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
	CFixVector *vp;
	ushort *data, nType;
	int i;

	////printf ("updating %d ", nSubModel);
data = reinterpret_cast<ushort*> (pmP->modelData + pmP->subModels.ptrs [nSubModel]);
nType = *data++;
#if DBG
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
vp = reinterpret_cast<CFixVector*> (data);
while (nVerts--) {
	if (mdP->times [i]) {		//not done yet
		if ((mdP->times [i] -= gameData.time.xFrame) <= 0) {
			 mdP->vecs [i] = *vp;
			 mdP->times [i] = 0;
			 mdP->nMorphingPoints [nSubModel]--;
			}
		else
			mdP->vecs[i] += mdP->deltas[i] * gameData.time.xFrame;
		}
	vp++;
	i++;
	}
return 1;
////printf ("npoints = %d\n", nMorphingPoints [nSubModel]);
}


//-------------------------------------------------------------
//process the morphing CObject for one frame
void DoMorphFrame (CObject *objP)
{
	int			i, t;
	tPolyModel	*pmP;
	tMorphInfo	*mdP;

if (!(mdP = MorphFindData (objP))) {	//maybe loaded half-morphed from disk
	KillObject (objP);	//so kill it
	return;
	}
pmP = gameData.models.polyModels + mdP->objP->rType.polyObjInfo.nModel;
G3CheckAndSwap (reinterpret_cast<void*> (pmP->modelData.Buffer ()));
for (i = 0; i < pmP->nModels; i++)
	if (mdP->submodelActive [i] == 1) {
		if (!MorphUpdatePoints (pmP, i, mdP))
			mdP->submodelActive [i] = 0;
      else if (mdP->nMorphingPoints [i] == 0) {		//maybe start submodel
			mdP->submodelActive [i] = 2;		//not animating, just visible
			mdP->nSubmodelsActive--;		//this one done animating
			for (t = 0; t < pmP->nModels; t++)
				if (pmP->subModels.parents [t] == i) 		//start this one
					if ((mdP->submodelActive [t] = MorphInitPoints (pmP, NULL, t, mdP)))
						mdP->nSubmodelsActive++;
			}
		}
if (!mdP->nSubmodelsActive) {			//done morphing!
	CObject *objP = mdP->objP;
	objP->info.controlType = mdP->saveControlType;
	objP->info.movementType = mdP->saveMovementType;
	objP->info.renderType = RT_POLYOBJ;
	objP->mType.physInfo = mdP->savePhysInfo;
	mdP->objP = NULL;
	}
}

//-------------------------------------------------------------

CFixVector morph_rotvel = CFixVector::Create(0x4000, 0x2000, 0x1000);

void MorphInit ()
{
	int i;

for (i = 0; i < MAX_MORPH_OBJECTS; i++)
	gameData.render.morph.objects [i].objP = NULL;
}


//-------------------------------------------------------------
//make the CObject morph
void MorphStart (CObject *objP)
{
	tPolyModel *pmP;
	CFixVector pmmin, pmmax;
	CFixVector vBoxSize;
	int i;
	tMorphInfo *mdP = gameData.render.morph.objects;

for (i=0;i<MAX_MORPH_OBJECTS;i++, mdP++)
	if (mdP->objP == NULL ||
			mdP->objP->info.nType==OBJ_NONE  ||
			mdP->objP->info.nSignature!=mdP->nSignature)
		break;

if (i==MAX_MORPH_OBJECTS)		//no free slots
	return;

Assert (objP->info.renderType == RT_POLYOBJ);
mdP->objP = objP;
mdP->nSignature = objP->info.nSignature;
mdP->saveControlType = objP->info.controlType;
mdP->saveMovementType = objP->info.movementType;
mdP->savePhysInfo = objP->mType.physInfo;
Assert (objP->info.controlType == CT_AI);		//morph OBJECTS are also AI gameData.objPs.objPects
objP->info.controlType = CT_MORPH;
objP->info.renderType = RT_MORPH;
objP->info.movementType = MT_PHYSICS;		//RT_NONE;
objP->mType.physInfo.rotVel = morph_rotvel;
pmP = gameData.models.polyModels + objP->rType.polyObjInfo.nModel;
G3CheckAndSwap (reinterpret_cast<void*> (pmP->modelData.Buffer ()));
MorphFindModelBounds (pmP, 0, pmmin, pmmax);
vBoxSize[X] = max (-pmmin[X], pmmax[X]) / 2;
vBoxSize[Y] = max (-pmmin[Y], pmmax[Y]) / 2;
vBoxSize[Z] = max (-pmmin[Z], pmmax[Z]) / 2;
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

void MorphDrawModel (tPolyModel *pmP, int nSubModel, CAngleVector *animAngles, fix light, tMorphInfo *mdP, int nModel)
{
	int i, mn;
	int facing;
	int sort_list [MAX_SUBMODELS], sort_n;
	//first, sort the submodels

sort_list [0] = nSubModel;
sort_n = 1;
for (i = 0; i < pmP->nModels; i++) {
	if (mdP->submodelActive [i] && pmP->subModels.parents [i]==nSubModel) {
		facing = G3CheckNormalFacing(pmP->subModels.pnts[i], pmP->subModels.norms[i]);
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
for (i = 0; i < sort_n; i++) {
	mn = sort_list [i];
	if (mn == nSubModel) {
 		int i;
		for (i=0;i<pmP->nTextures;i++) {
			gameData.models.textureIndex [i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.objBmIndexP [pmP->nFirstTexture+i]];
			gameData.models.textures [i] = gameData.pig.tex.bitmaps [0] + /*gameData.pig.tex.objBmIndex [gameData.pig.tex.objBmIndexP [pmP->nFirstTexture+i]]*/gameData.models.textureIndex [i].index;
			}

#ifdef PIGGY_USE_PAGING
		// Make sure the textures for this CObject are paged in..
		gameData.pig.tex.bPageFlushed = 0;
		for (i = 0; i < pmP->nTextures;i++)
			PIGGY_PAGE_IN (gameData.models.textureIndex [i].index, 0);
		// Hmmm.. cache got flushed in the middle of paging all these in,
		// so we need to reread them all in.
		if (gameData.pig.tex.bPageFlushed)	{
			gameData.pig.tex.bPageFlushed = 0;
			for (i=0;i<pmP->nTextures;i++)
				PIGGY_PAGE_IN (gameData.models.textureIndex [i].index, 0);
			}
			// Make sure that they can all fit in memory.
		Assert ( gameData.pig.tex.bPageFlushed == 0);
#endif
		G3DrawMorphingModel (
			pmP->modelData + pmP->subModels.ptrs [nSubModel],
			gameData.models.textures,
			animAngles, NULL, light,
			mdP->vecs + mdP->submodelStartPoints [nSubModel], nModel);
		}
	else {
		CFixMatrix orient;
		orient = CFixMatrix::Create(animAngles[mn]);
		transformation.Begin(pmP->subModels.offsets[mn], orient);
		MorphDrawModel (pmP, mn, animAngles, light, mdP, nModel);
		transformation.End ();
		}
	}
}

//-------------------------------------------------------------

void MorphDrawObject (CObject *objP)
{
//	int save_light;
	tPolyModel *pmP;
	fix light;
	tMorphInfo *mdP;

mdP = MorphFindData (objP);
Assert (mdP != NULL);
Assert (objP->rType.polyObjInfo.nModel < gameData.models.nPolyModels);
pmP = gameData.models.polyModels+objP->rType.polyObjInfo.nModel;
light = ComputeObjectLight (objP, NULL);
transformation.Begin(objP->info.position.vPos, objP->info.position.mOrient);
G3SetModelPoints (gameData.models.polyModelPoints);
gameData.render.vertP = gameData.models.fPolyModelVerts;
MorphDrawModel (pmP, 0, objP->rType.polyObjInfo.animAngles, light, mdP, objP->rType.polyObjInfo.nModel);
gameData.render.vertP = NULL;
transformation.End ();

#ifdef NEWDEMO
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMorphFrame (mdP);
#endif
}

//-------------------------------------------------------------
//eof
