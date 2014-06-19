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

#include "descent.h"
#include "texmap.h"
#include "error.h"
#include "light.h"
#include "newdemo.h"
#include "interp.h"

//-------------------------------------------------------------
//returns ptr to data for this CObject, or NULL if none
tMorphInfo *MorphFindData (CObject *objP)
{
if (gameData.demo.nState == ND_STATE_PLAYBACK) {
	gameData.render.morph.objects [0].objP = objP;
	return &gameData.render.morph.objects [0];
	}

for (int i = 0; i < MAX_MORPH_OBJECTS; i++)
	if (gameData.render.morph.objects [i].objP == objP)
		return gameData.render.morph.objects + i;
return NULL;
}

//-------------------------------------------------------------
//takes pmP, fills in min& max
void MorphFindModelBounds (CPolyModel* pmP, int nSubModel, CFixVector& minv, CFixVector& maxv)
{
	ushort nVerts;
	CFixVector *vp;
	ushort *data, nType;

	data = reinterpret_cast<ushort*> (pmP->Data () + pmP->SubModels ().ptrs [nSubModel]);
	nType = *data++;
	Assert (nType == 7 || nType == 1);
	nVerts = *data++;
	if (nType==7)
		data+=2;		//skip start & pad
	vp = reinterpret_cast<CFixVector*> (data);
	minv = maxv = *vp++;
	nVerts--;
	while (nVerts--) {
		if ((*vp).v.coord.x > maxv.v.coord.x)
			maxv.v.coord.x = (*vp).v.coord.x;
		else if((*vp).v.coord.x < minv.v.coord.x)
			minv.v.coord.x = (*vp).v.coord.x;
		if ((*vp).v.coord.y > maxv.v.coord.y)
			maxv.v.coord.y = (*vp).v.coord.y;
		else if ((*vp).v.coord.y < minv.v.coord.y)
			minv.v.coord.y = (*vp).v.coord.y;
		if ((*vp).v.coord.z > maxv.v.coord.z)
			maxv.v.coord.z = (*vp).v.coord.z;
		else if ((*vp).v.coord.z < minv.v.coord.z)
			minv.v.coord.z = (*vp).v.coord.z;
		vp++;
	}
}

//-------------------------------------------------------------

int MorphInitPoints (CPolyModel* pmP, CFixVector *vBoxSize, int nSubModel, tMorphInfo *mdP)
{
	ushort		nVerts;
	CFixVector	*vp, v;
	ushort		*data, nType;
	int			i;

//printf ("initing %d ", nSubModel);
data = reinterpret_cast<ushort*> (pmP->Data () + pmP->SubModels ().ptrs [nSubModel]);
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
		if (v.v.coord.x && X2I ((*vBoxSize).v.coord.x) < abs (v.v.coord.x)/2 && (t = FixDiv ((*vBoxSize).v.coord.x, abs (v.v.coord.x))) < k)
			k = t;
		if (v.v.coord.y && X2I ((*vBoxSize).v.coord.y) < abs (v.v.coord.y)/2 && (t = FixDiv ((*vBoxSize).v.coord.y, abs (v.v.coord.y))) < k)
			k = t;
		if (v.v.coord.z && X2I ((*vBoxSize).v.coord.z) < abs (v.v.coord.z)/2 && (t = FixDiv ((*vBoxSize).v.coord.z, abs (v.v.coord.z))) < k)
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

int MorphUpdatePoints (CPolyModel* pmP, int nSubModel, tMorphInfo *mdP)
{
	ushort nVerts;
	CFixVector *vp;
	ushort *data, nType;
	int i;

	////printf ("updating %d ", nSubModel);
data = reinterpret_cast<ushort*> (pmP->Data () + pmP->SubModels ().ptrs [nSubModel]);
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
void CObject::DoMorphFrame (void)
{
	int			i, t;
	CPolyModel	*pmP;
	tMorphInfo	*mdP;

if (!(mdP = MorphFindData (this))) {	//maybe loaded half-morphed from disk
	Die ();	//so kill it
	return;
	}
pmP = gameData.models.polyModels [0] + mdP->objP->ModelId ();
G3CheckAndSwap (reinterpret_cast<void*> (pmP->Data ()));
for (i = 0; i < pmP->ModelCount (); i++)
	if (mdP->submodelActive [i] == 1) {
		if (!MorphUpdatePoints (pmP, i, mdP))
			mdP->submodelActive [i] = 0;
      else if (mdP->nMorphingPoints [i] == 0) {		//maybe start submodel
			mdP->submodelActive [i] = 2;		//not animating, just visible
			mdP->nSubmodelsActive--;		//this one done animating
			for (t = 0; t < pmP->ModelCount (); t++)
				if (pmP->SubModels ().parents [t] == i) 		//start this one
					if ((mdP->submodelActive [t] = MorphInitPoints (pmP, NULL, t, mdP)))
						mdP->nSubmodelsActive++;
			}
		}
if (!mdP->nSubmodelsActive) {			//done morphing!
	CObject* objP = mdP->objP;
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
void CObject::MorphStart (void)
{
	CPolyModel* pmP;
	CFixVector pmmin, pmmax;
	CFixVector vBoxSize;
	int i;
	tMorphInfo *mdP = &gameData.render.morph.objects [0];

for (i = 0; i < MAX_MORPH_OBJECTS; i++, mdP++)
	if (mdP->objP == NULL ||
		 mdP->objP->info.nType == OBJ_NONE  ||
		 mdP->objP->info.nSignature != mdP->nSignature)
		break;

if (i == MAX_MORPH_OBJECTS)		//no free slots
	return;

Assert (info.renderType == RT_POLYOBJ);
mdP->objP = this;
mdP->nSignature = info.nSignature;
mdP->saveControlType = info.controlType;
mdP->saveMovementType = info.movementType;
mdP->savePhysInfo = mType.physInfo;
Assert (info.controlType == CT_AI);		//morph OBJECTS are also AI gameData.objPs.objPects
info.controlType = CT_MORPH;
info.renderType = RT_MORPH;
info.movementType = MT_PHYSICS;		//RT_NONE;
mType.physInfo.rotVel = morph_rotvel;
pmP = gameData.models.polyModels [0] + ModelId ();
if (!pmP->Data ())
	return;
G3CheckAndSwap (reinterpret_cast<void*> (pmP->Data ()));
MorphFindModelBounds (pmP, 0, pmmin, pmmax);
vBoxSize.v.coord.x = Max (-pmmin.v.coord.x, pmmax.v.coord.x) / 2;
vBoxSize.v.coord.y = Max (-pmmin.v.coord.y, pmmax.v.coord.y) / 2;
vBoxSize.v.coord.z = Max (-pmmin.v.coord.z, pmmax.v.coord.z) / 2;
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

void MorphDrawModel (CPolyModel* modelP, int nSubModel, CAngleVector *animAngles, fix light, tMorphInfo *mdP, int nModel)
{
	int h, i, j, m, n;
	int sortList [2 * MAX_SUBMODELS];
	//first, sort the submodels

n = h = modelP->ModelCount ();
m = h - 1;
sortList [m] = nSubModel;
for (i = 0; i < h; i++) {
	if (mdP->submodelActive [i] && modelP->SubModels ().parents [i] == nSubModel) {
		if (!G3CheckNormalFacing (modelP->SubModels ().pnts [i], modelP->SubModels ().norms [i]))
			sortList [n++] = i;
		else
			sortList [--m] = i;
		}
	}

//now draw everything
for (i = m; i < n; i++) {
	m = sortList [i];
	if (m == nSubModel) {
		h = modelP->TextureCount ();
		for (j = 0; j < h; j++) {
			gameData.models.textureIndex [j] = gameData.pig.tex.objBmIndex [gameData.pig.tex.objBmIndexP [modelP->FirstTexture () + j]];
			gameData.models.textures [j] = gameData.pig.tex.bitmaps [0] + /*gameData.pig.tex.objBmIndex [gameData.pig.tex.objBmIndexP [modelP->FirstTexture ()+j]]*/gameData.models.textureIndex [j].index;
			}

		// Make sure the textures for this CObject are paged in..
		gameData.pig.tex.bPageFlushed = 0;
		for (j = 0; j < h; j++)
			LoadTexture (gameData.models.textureIndex [j].index, 0, 0);
		// Hmmm.. cache got flushed in the middle of paging all these in,
		// so we need to reread them all in.
		if (gameData.pig.tex.bPageFlushed) {
			gameData.pig.tex.bPageFlushed = 0;
			for (int j = 0; j < h; j++)
				LoadTexture (gameData.models.textureIndex [j].index, 0, 0);
			}
			// Make sure that they can all fit in memory.
		Assert (gameData.pig.tex.bPageFlushed == 0);

		G3DrawMorphingModel (
			modelP->Data () + modelP->SubModels ().ptrs [nSubModel],
			gameData.models.textures,
			animAngles, NULL, light,
			mdP->vecs + mdP->submodelStartPoints [nSubModel], nModel);
		}
	else {
		CFixMatrix orient;
		orient = CFixMatrix::Create (animAngles [m]);
		transformation.Begin (modelP->SubModels ().offsets [m], orient);
		MorphDrawModel (modelP, m, animAngles, light, mdP, nModel);
		transformation.End ();
		}
	}
}

//-------------------------------------------------------------

void CObject::MorphDraw (void)
{
//	int save_light;
	CPolyModel* pmP;
	fix light;
	tMorphInfo *mdP;

mdP = MorphFindData (this);
Assert (mdP != NULL);
Assert (ModelId () < gameData.models.nPolyModels);
pmP = gameData.models.polyModels [0] + ModelId ();
if (!pmP->Data ())
	return;
light = ComputeObjectLight (this, NULL);
transformation.Begin (info.position.vPos, info.position.mOrient);
G3SetModelPoints (gameData.models.polyModelPoints.Buffer ());
gameData.render.vertP = gameData.models.fPolyModelVerts.Buffer ();
MorphDrawModel (pmP, 0, rType.polyObjInfo.animAngles, light, mdP, ModelId ());
gameData.render.vertP = NULL;
transformation.End ();

if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMorphFrame (mdP);
}

//-------------------------------------------------------------
//eof
