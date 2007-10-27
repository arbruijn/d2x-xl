/* $Id: interp.c, v 1.14 2003/03/19 19:21:34 btb Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: interp.c, v 1.14 2003/03/19 19:21:34 btb Exp $";
#endif

#include <math.h>
#include <stdlib.h>
#include "error.h"

#include "inferno.h"
#include "globvars.h"
#include "interp.h"
#include "hitbox.h"
#include "gr.h"
#include "byteswap.h"
#include "u_mem.h"
#include "console.h"
#include "ogl_defs.h"
#include "network.h"
#include "render.h"
#include "gameseg.h"
#include "lighting.h"
#include "lightning.h"

extern tFaceColor tMapColor;

//------------------------------------------------------------------------------

//walks through all submodels of a polymodel and determines the coordinate extremes
int G3GetPolyModelMinMax (void *modelP, tHitbox *phb, int nSubModels)
{
	ubyte			*p = modelP;
	int			i, n, nVerts;
	vmsVector	*v, hv;
	tHitbox		hb = *phb;

G3CheckAndSwap (modelP);
for (;;)
	switch (WORDVAL (p)) {
		case OP_EOF:
			goto done;
		case OP_DEFPOINTS:
			n = WORDVAL (p + 2);
			v = VECPTR (p + 4);
			for (i = n; i; i--, v++) {
				hv = *v;
				if (hb.vMin.p.x > hv.p.x)
					hb.vMin.p.x = hv.p.x;
				else if (hb.vMax.p.x < hv.p.x)
					hb.vMax.p.x = hv.p.x;
				if (hb.vMin.p.y > hv.p.y)
					hb.vMin.p.y = hv.p.y;
				else if (hb.vMax.p.y < hv.p.y)
					hb.vMax.p.y = hv.p.y;
				if (hb.vMin.p.z > hv.p.z)
					hb.vMin.p.z = hv.p.z;
				else if (hb.vMax.p.z < hv.p.z)
					hb.vMax.p.z = hv.p.z;
				}
			p += n * sizeof (vmsVector) + 4;
			break;

		case OP_DEFP_START: 
			n = WORDVAL (p + 2);
			v = VECPTR (p + 8);
			for (i = n; i; i--, v++) {
				hv = *v;
				if (hb.vMin.p.x > hv.p.x)
					hb.vMin.p.x = hv.p.x;
				else if (hb.vMax.p.x < hv.p.x)
					hb.vMax.p.x = hv.p.x;
				if (hb.vMin.p.y > hv.p.y)
					hb.vMin.p.y = hv.p.y;
				else if (hb.vMax.p.y < hv.p.y)
					hb.vMax.p.y = hv.p.y;
				if (hb.vMin.p.z > hv.p.z)
					hb.vMin.p.z = hv.p.z;
				else if (hb.vMax.p.z < hv.p.z)
					hb.vMax.p.z = hv.p.z;
				}
			p += n * sizeof (vmsVector) + 8;
			break;

		case OP_FLATPOLY:
			nVerts = WORDVAL (p + 2);
			p += 30 + (nVerts | 1) * 2;
			break;

		case OP_TMAPPOLY:
			nVerts = WORDVAL (p + 2);
			p += 30 + (nVerts | 1) * 2 + nVerts * 12;
			break;

		case OP_SORTNORM:
			*phb = hb;
			if (G3CheckNormalFacing (VECPTR (p+16), VECPTR (p+4)) > 0) {		//facing
				nSubModels = G3GetPolyModelMinMax (p + WORDVAL (p+30), phb, nSubModels);
				nSubModels = G3GetPolyModelMinMax (p + WORDVAL (p+28), phb, nSubModels);
				}
			else {
				nSubModels = G3GetPolyModelMinMax (p + WORDVAL (p+28), phb, nSubModels);
				nSubModels = G3GetPolyModelMinMax (p + WORDVAL (p+30), phb, nSubModels);
				}
			hb = *phb;
			p += 32;
			break;


		case OP_RODBM:
			p += 36;
			break;

		case OP_SUBCALL:
#if 1
			VmVecAdd (&phb [++nSubModels].vOffset, &phb->vOffset, VECPTR (p+4));
#else
			pvOffs [nSubModels] = *VECPTR (p+4);
#endif
			nSubModels += G3GetPolyModelMinMax (p + WORDVAL (p+16), phb + nSubModels, 0);
			p += 20;
			break;


		case OP_GLOW:
			p += 4;
			break;

		default:
			Error ("invalid polygon model\n");
		}

done:

*phb = hb;
return nSubModels;
}

//------------------------------------------------------------------------------

#if 0
	static		fVector hitBoxOffsets [8] = {
		{-1.0f, +0.95f, -0.8f}, 
		{+1.0f, +0.95f, -0.8f}, 
		{+1.0f, -1.05f, -0.8f}, 
		{-1.0f, -1.05f, -0.8f}, 
		{-1.0f, +0.95f, +1.2f}, 
		{+1.0f, +0.95f, +1.2f}, 
		{+1.0f, -1.05f, +1.2f}, 
		{-1.0f, -1.05f, +1.2f}
#else
	vmsVector hitBoxOffsets [8] = {
		{{1, 0, 1}}, 
		{{0, 0, 1}}, 
		{{0, 1, 1}}, 
		{{1, 1, 1}}, 
		{{1, 0, 0}}, 
		{{0, 0, 0}}, 
		{{0, 1, 0}}, 
		{{1, 1, 0}}
#endif
		};

int hitboxFaceVerts [6][4] = {
	{0,1,2,3},
	{0,3,7,4},
	{5,6,2,1},
	{4,7,6,5},
	{4,5,1,0},
	{6,7,3,2}
	};

void ComputeHitbox (int nModel, int iSubObj)
{
	tHitbox			*phb = gameData.models.hitboxes [nModel].hitboxes + iSubObj;
	vmsVector		vMin = phb->vMin;
	vmsVector		vMax = phb->vMax;
	vmsVector		vOffset = phb->vOffset;
	vmsVector		*pv = phb->box.vertices;
	tQuad				*pf;
	int				i;

for (i = 0; i < 8; i++) {
	pv [i].p.x = (hitBoxOffsets [i].p.x ? vMin.p.x : vMax.p.x) + vOffset.p.x;
	pv [i].p.y = (hitBoxOffsets [i].p.y ? vMin.p.y : vMax.p.y) + vOffset.p.y;
	pv [i].p.z = (hitBoxOffsets [i].p.z ? vMin.p.z : vMax.p.z) + vOffset.p.z;
	}
for (i = 0, pf = phb->box.faces; i < 6; i++, pf++) {
	VmVecNormal (pf->n, pv + hitboxFaceVerts [i][0], pv + hitboxFaceVerts [i][1], pv + hitboxFaceVerts [i][2]);
	}
}

//------------------------------------------------------------------------------

#if 0

void TransformHitbox (tObject *objP, vmsVector *vPos, int iSubObj)
{
	tHitbox			*phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes + iSubObj;
	tQuad				*pf = phb->faces;
	vmsVector		rotVerts [8];
	vmsMatrix		m;
	int				i, j;

if (!vPos)
	vPos = &objP->position.vPos;
VmCopyTransposeMatrix (&m, &objP->position.mOrient);
for (i = 0; i < 8; i++) {
	VmVecRotate (rotVerts + i, phb->vertices + i, &m);
	VmVecInc (rotVerts + i, vPos);
	}
for (i = 0; i < 6; i++, pf++) {
	for (j = 0; j < 4; j++)
		pf->v [j] = rotVerts [hitboxFaceVerts [i][j]];
	VmVecRotate (pf->n + 1, pf->n, &m);
	}
}

#endif

//------------------------------------------------------------------------------

#define G3_HITBOX_TRANSFORM	0
#define HITBOX_CACHE				0

#if G3_HITBOX_TRANSFORM

void TransformHitboxes (tObject *objP, vmsVector *vPos, tBox *phb)
{
	tHitbox		*pmhb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
	tQuad			*pf;
	vmsVector	rotVerts [8];
	int			i, j, iModel, nModels;

if (extraGameInfo [IsMultiGame].nHitboxes == 1) {
	iModel =
	nModels = 0;
	}
else {
	iModel = 1;
	nModels = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].nSubModels;
	}
G3StartInstanceMatrix (vPos ? vPos : &objP->position.vPos, &objP->position.mOrient);
for (; iModel <= nModels; iModel++, phb++, pmhb++) {
	for (i = 0; i < 8; i++)
		G3TransformPoint (rotVerts + i, pmhb->box.vertices + i, 0);
	for (i = 0, pf = phb->faces; i < 6; i++, pf++) {
		for (j = 0; j < 4; j++)
			pf->v [j] = rotVerts [hitboxFaceVerts [i][j]];
		VmVecNormal (pf->n + 1, pf->v, pf->v + 1, pf->v + 2);
		}
	}
G3DoneInstance ();
}

#else //G3_HITBOX_TRANSFORM

void TransformHitboxes (tObject *objP, vmsVector *vPos, tBox *phb)
{
	tHitbox		*pmhb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
	tQuad			*pf;
	vmsVector	rotVerts [8];
	vmsMatrix	m;
	int			i, j, iModel, nModels;

if (extraGameInfo [IsMultiGame].nHitboxes == 1) {
	iModel =
	nModels = 0;
	}
else {
	iModel = 1;
	nModels = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].nSubModels;
	}
if (!vPos)
	vPos = &objP->position.vPos;
VmCopyTransposeMatrix (&m, &objP->position.mOrient);
for (phb += iModel, pmhb += iModel; iModel <= nModels; iModel++, phb++, pmhb++) {
	for (i = 0; i < 8; i++) {
		VmVecRotate (rotVerts + i, pmhb->box.vertices + i, &m);
		VmVecInc (rotVerts + i, vPos);
		}
	for (i = 0, pf = phb->faces; i < 6; i++, pf++) {
		for (j = 0; j < 4; j++)
			pf->v [j] = rotVerts [hitboxFaceVerts [i][j]];
		VmVecNormal (pf->n + 1, pf->v, pf->v + 1, pf->v + 2);
		}
	}
}

#endif //G3_HITBOX_TRANSFORM

//------------------------------------------------------------------------------
//walks through all submodels of a polymodel and determines the coordinate extremes
fix G3PolyModelSize (tPolyModel *pm, int nModel)
{
	int			nSubModels = 1;
	int			i;
	tHitbox		*phb = gameData.models.hitboxes [nModel].hitboxes;
	vmsVector	hv;
	double		dx, dy, dz;

for (i = 0; i <= MAX_SUBMODELS; i++) {
	phb [i].vMin.p.x = phb [i].vMin.p.y = phb [i].vMin.p.z = 0x7fffffff;
	phb [i].vMax.p.x = phb [i].vMax.p.y = phb [i].vMax.p.z = -0x7fffffff;
	phb [i].vOffset.p.x = phb [i].vOffset.p.y = phb [i].vOffset.p.z = 0;
	}
nSubModels = G3GetPolyModelMinMax ((void *) pm->modelData, phb + 1, 0) + 1;
for (i = 1; i <= nSubModels; i++) {
	dx = (phb [i].vMax.p.x - phb [i].vMin.p.x) / 2;
	dy = (phb [i].vMax.p.y - phb [i].vMin.p.y) / 2;
	dz = (phb [i].vMax.p.z - phb [i].vMin.p.z) / 2;
	phb [i].vSize.p.x = (fix) dx;
	phb [i].vSize.p.y = (fix) dy;
	phb [i].vSize.p.z = (fix) dz;
	VmVecAdd (&hv, &phb [i].vMin, &phb [i].vOffset);
	if (phb [0].vMin.p.x > hv.p.x)
		phb [0].vMin.p.x = hv.p.x;
	if (phb [0].vMin.p.y > hv.p.y)
		phb [0].vMin.p.y = hv.p.y;
	if (phb [0].vMin.p.z > hv.p.z)
		phb [0].vMin.p.z = hv.p.z;
	VmVecAdd (&hv, &phb [i].vMax, &phb [i].vOffset);
	if (phb [0].vMax.p.x < hv.p.x)
		phb [0].vMax.p.x = hv.p.x;
	if (phb [0].vMax.p.y < hv.p.y)
		phb [0].vMax.p.y = hv.p.y;
	if (phb [0].vMax.p.z < hv.p.z)
		phb [0].vMax.p.z = hv.p.z;
	}
dx = (phb [0].vMax.p.x - phb [0].vMin.p.x) / 2;
dy = (phb [0].vMax.p.y - phb [0].vMin.p.y) / 2;
dz = (phb [0].vMax.p.z - phb [0].vMin.p.z) / 2;
phb [0].vSize.p.x = (fix) dx;
phb [0].vSize.p.y = (fix) dy;
phb [0].vSize.p.z = (fix) dz;
gameData.models.hitboxes [nModel].nSubModels = nSubModels;
for (i = 0; i <= nSubModels; i++)
	ComputeHitbox (nModel, i);
return (fix) (sqrt (dx * dx + dy * dy + dz + dz) /** 1.33*/);
}

//------------------------------------------------------------------------------
//eof
