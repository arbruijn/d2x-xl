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

#include <math.h>
#include <stdlib.h>
#include "error.h"

#include "descent.h"
#include "globvars.h"
#include "interp.h"
#include "hitbox.h"
#include "gr.h"
#include "byteswap.h"
#include "u_mem.h"
#include "console.h"
#include "ogl_defs.h"
#include "network.h"
#include "rendermine.h"
#include "segmath.h"
#include "light.h"
#include "lightning.h"

extern CFaceColor tMapColor;

//------------------------------------------------------------------------------

//walks through all submodels of a polymodel and determines the coordinate extremes
int GetPolyModelMinMax (void *modelP, tHitbox *phb, int nSubModels)
{
	ubyte			*p = reinterpret_cast<ubyte*> (modelP);
	int			i, n, nVerts;
	CFixVector	*v, hv;
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
				if (hb.vMin.v.coord.x > hv.v.coord.x)
					hb.vMin.v.coord.x = hv.v.coord.x;
				else if (hb.vMax.v.coord.x < hv.v.coord.x)
					hb.vMax.v.coord.x = hv.v.coord.x;
				if (hb.vMin.v.coord.y > hv.v.coord.y)
					hb.vMin.v.coord.y = hv.v.coord.y;
				else if (hb.vMax.v.coord.y < hv.v.coord.y)
					hb.vMax.v.coord.y = hv.v.coord.y;
				if (hb.vMin.v.coord.z > hv.v.coord.z)
					hb.vMin.v.coord.z = hv.v.coord.z;
				else if (hb.vMax.v.coord.z < hv.v.coord.z)
					hb.vMax.v.coord.z = hv.v.coord.z;
				}
			p += n * sizeof (CFixVector) + 4;
			break;

		case OP_DEFP_START:
			n = WORDVAL (p + 2);
			v = VECPTR (p + 8);
			for (i = n; i; i--, v++) {
				hv = *v;
				if (hb.vMin.v.coord.x > hv.v.coord.x)
					hb.vMin.v.coord.x = hv.v.coord.x;
				else if (hb.vMax.v.coord.x < hv.v.coord.x)
					hb.vMax.v.coord.x = hv.v.coord.x;
				if (hb.vMin.v.coord.y > hv.v.coord.y)
					hb.vMin.v.coord.y = hv.v.coord.y;
				else if (hb.vMax.v.coord.y < hv.v.coord.y)
					hb.vMax.v.coord.y = hv.v.coord.y;
				if (hb.vMin.v.coord.z > hv.v.coord.z)
					hb.vMin.v.coord.z = hv.v.coord.z;
				else if (hb.vMax.v.coord.z < hv.v.coord.z)
					hb.vMax.v.coord.z = hv.v.coord.z;
				}
			p += n * sizeof (CFixVector) + 8;
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
			if (G3CheckNormalFacing (*VECPTR (p+16), *VECPTR (p+4))) {		//facing
				nSubModels = GetPolyModelMinMax (p + WORDVAL (p+30), phb, nSubModels);
				nSubModels = GetPolyModelMinMax (p + WORDVAL (p+28), phb, nSubModels);
				}
			else {
				nSubModels = GetPolyModelMinMax (p + WORDVAL (p+28), phb, nSubModels);
				nSubModels = GetPolyModelMinMax (p + WORDVAL (p+30), phb, nSubModels);
				}
			hb = *phb;
			p += 32;
			break;


		case OP_RODBM:
			p += 36;
			break;

		case OP_SUBCALL:
#if 1
			phb [++nSubModels].vOffset = phb->vOffset + *VECPTR (p+4);
#else
			pvOffs [nSubModels] = *VECPTR (p+4);
#endif
			nSubModels += GetPolyModelMinMax (p + WORDVAL (p+16), phb + nSubModels, 0);
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

CFixVector hitBoxOffsets [8] = {
	CFixVector::Create(1, 0, 1),
	CFixVector::Create(0, 0, 1),
	CFixVector::Create(0, 1, 1),
	CFixVector::Create(1, 1, 1),
	CFixVector::Create(1, 0, 0),
	CFixVector::Create(0, 0, 0),
	CFixVector::Create(0, 1, 0),
	CFixVector::Create(1, 1, 0)
	};

int hitboxFaceVerts [6][4] = {
 {0,1,2,3},
 {0,3,7,4},
 {5,6,2,1},
 {4,7,6,5},
 {4,5,1,0},
 {6,7,3,2}
	};

void ComputeHitbox (int nModel, int iHitbox)
{
	tHitbox			*phb = gameData.models.hitboxes [nModel].hitboxes + iHitbox;
	CFixVector		vMin = phb->vMin;
	CFixVector		vMax = phb->vMax;
	CFixVector		vOffset = phb->vOffset;
	CFixVector		*pv = phb->box.vertices;
	tQuad				*pf;
	int				i;

for (i = 0; i < 8; i++) {
	pv [i].v.coord.x = (hitBoxOffsets [i].v.coord.x ? vMin.v.coord.x : vMax.v.coord.x) + vOffset.v.coord.x;
	pv [i].v.coord.y = (hitBoxOffsets [i].v.coord.y ? vMin.v.coord.y : vMax.v.coord.y) + vOffset.v.coord.y;
	pv [i].v.coord.z = (hitBoxOffsets [i].v.coord.z ? vMin.v.coord.z : vMax.v.coord.z) + vOffset.v.coord.z;
	}
for (i = 0, pf = phb->box.faces; i < 6; i++, pf++) {
	*pf->n = CFixVector::Normal (pv [hitboxFaceVerts [i][0]], pv [hitboxFaceVerts [i][1]], pv [hitboxFaceVerts [i][2]]);
	}
}

//------------------------------------------------------------------------------
// This function rotates and positions an object's hitboxes relative to the 
// object's orientation and position in world space. It must not transform the
// hitbox coordinates to view space since that's not necessary to test collisions
// of different objects, and since collision tests with geometry are done using
// the geometry's world space coordinates.

#define G3_HITBOX_TRANSFORM	0
#define HITBOX_CACHE				0

#if G3_HITBOX_TRANSFORM

void TransformHitboxes (CObject *objP, CFixVector *vPos, tBox *phb)
{
	tHitbox		*pmhb = gameData.models.hitboxes [objP->ModelId ()].hitboxes;
	tQuad			*pf;
	CFixVector	rotVerts [8];
	int			i, j, iModel, nModels;

if (CollisionModel () == 1) {
	iModel =
	nModels = 0;
	}
else {
	iModel = 1;
	nModels = gameData.models.hitboxes [objP->ModelId ()].nSubModels;
	}
transformation.Begin (vPos ? vPos : &objP->info.position.vPos, &objP->info.position.mOrient);
for (; iModel <= nModels; iModel++, phb++, pmhb++) {
	for (i = 0; i < 8; i++)
		transformation.Transform (rotVerts + i, pmhb->box.vertices + i, 0);
	for (i = 0, pf = phb->faces; i < SEGMENT_SIDE_COUNT; i++, pf++) {
		for (j = 0; j < 4; j++)
			pf->dir [j] = rotVerts [hitboxFaceVerts [i][j]];
		VmVecNormal (pf->n + 1, pf->dir, pf->dir + 1, pf->dir + 2);
		}
	}
transformation.End ();
}

#else //G3_HITBOX_TRANSFORM

tHitbox* TransformHitboxes (CObject *objP, CFixVector *vPos)
{
	int			nId = objP->ModelId ();
	tHitbox*		hb = &gameData.models.hitboxes [nId].hitboxes [0];

#if !DBG
if (gameData.models.hitboxes [nId].nFrame == gameData.objs.nFrameCount)
	return hb;
gameData.models.hitboxes [nId].nFrame = gameData.objs.nFrameCount;
#endif
	tQuad*		pf;
	CFixVector	rotVerts [8];
	CFixMatrix*	viewP = objP->View (0);
	int			i, j, iBox, nBoxes;

if (CollisionModel () == 1) {
	iBox = 0;
	nBoxes = 1;
	}
else {
	iBox = 1;
	nBoxes = gameData.models.hitboxes [objP->ModelId ()].nHitboxes;
	}
if (!vPos)
	vPos = &objP->info.position.vPos;
for (; iBox <= nBoxes; iBox++) {
	for (i = 0; i < 8; i++) {
		rotVerts [i] = *viewP * hb [iBox].box.vertices [i];
		rotVerts [i] += *vPos;
		}
	for (i = 0, pf = hb [iBox].box.faces; i < 6; i++, pf++) {
		for (j = 0; j < 4; j++)
			pf->v [j] = rotVerts [hitboxFaceVerts [i][j]];
		pf->n [1] = CFixVector::Normal (pf->v [0], pf->v [1], pf->v [2]);
		}
	}
return hb;
}

#endif //G3_HITBOX_TRANSFORM

//------------------------------------------------------------------------------
//eof
