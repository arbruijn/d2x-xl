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
#include "light.h"
#include "lightning.h"

extern tFaceColor tMapColor;

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
				if (hb.vMin[X] > hv[X])
					hb.vMin[X] = hv[X];
				else if (hb.vMax[X] < hv[X])
					hb.vMax[X] = hv[X];
				if (hb.vMin[Y] > hv[Y])
					hb.vMin[Y] = hv[Y];
				else if (hb.vMax[Y] < hv[Y])
					hb.vMax[Y] = hv[Y];
				if (hb.vMin[Z] > hv[Z])
					hb.vMin[Z] = hv[Z];
				else if (hb.vMax[Z] < hv[Z])
					hb.vMax[Z] = hv[Z];
				}
			p += n * sizeof (CFixVector) + 4;
			break;

		case OP_DEFP_START:
			n = WORDVAL (p + 2);
			v = VECPTR (p + 8);
			for (i = n; i; i--, v++) {
				hv = *v;
				if (hb.vMin[X] > hv[X])
					hb.vMin[X] = hv[X];
				else if (hb.vMax[X] < hv[X])
					hb.vMax[X] = hv[X];
				if (hb.vMin[Y] > hv[Y])
					hb.vMin[Y] = hv[Y];
				else if (hb.vMax[Y] < hv[Y])
					hb.vMax[Y] = hv[Y];
				if (hb.vMin[Z] > hv[Z])
					hb.vMin[Z] = hv[Z];
				else if (hb.vMax[Z] < hv[Z])
					hb.vMax[Z] = hv[Z];
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

#if 0
	static		CFloatVector hitBoxOffsets [8] = {
	 {-1.0f, +0.95f, -0.8f},
	 {+1.0f, +0.95f, -0.8f},
	 {+1.0f, -1.05f, -0.8f},
	 {-1.0f, -1.05f, -0.8f},
	 {-1.0f, +0.95f, +1.2f},
	 {+1.0f, +0.95f, +1.2f},
	 {+1.0f, -1.05f, +1.2f},
	 {-1.0f, -1.05f, +1.2f}
#else
	CFixVector hitBoxOffsets [8] = {
		CFixVector::Create(1, 0, 1),
		CFixVector::Create(0, 0, 1),
		CFixVector::Create(0, 1, 1),
		CFixVector::Create(1, 1, 1),
		CFixVector::Create(1, 0, 0),
		CFixVector::Create(0, 0, 0),
		CFixVector::Create(0, 1, 0),
		CFixVector::Create(1, 1, 0)
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
	pv [i][X] = (hitBoxOffsets [i][X] ? vMin[X] : vMax[X]) + vOffset[X];
	pv [i][Y] = (hitBoxOffsets [i][Y] ? vMin[Y] : vMax[Y]) + vOffset[Y];
	pv [i][Z] = (hitBoxOffsets [i][Z] ? vMin[Z] : vMax[Z]) + vOffset[Z];
	}
for (i = 0, pf = phb->box.faces; i < 6; i++, pf++) {
	*pf->n = CFixVector::Normal(pv[hitboxFaceVerts [i][0]], pv[hitboxFaceVerts [i][1]], pv[hitboxFaceVerts [i][2]]);
	}
}

//------------------------------------------------------------------------------

#if 0

void TransformHitbox (CObject *objP, CFixVector *vPos, int iSubObj)
{
	tHitbox			*phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes + iSubObj;
	tQuad				*pf = phb->faces;
	CFixVector		rotVerts [8];
	CFixMatrix		*viewP = ObjectView (objP);
	int				i, j;

if (!vPos)
	vPos = &objP->info.position.vPos;
for (i = 0; i < 8; i++) {
	VmVecRotate (rotVerts + i, phb->vertices + i, viewP);
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

void TransformHitboxes (CObject *objP, CFixVector *vPos, tBox *phb)
{
	tHitbox		*pmhb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
	tQuad			*pf;
	CFixVector	rotVerts [8];
	int			i, j, iModel, nModels;

if (extraGameInfo [IsMultiGame].nHitboxes == 1) {
	iModel =
	nModels = 0;
	}
else {
	iModel = 1;
	nModels = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].nSubModels;
	}
transformation.Begin (vPos ? vPos : &objP->info.position.vPos, &objP->info.position.mOrient);
for (; iModel <= nModels; iModel++, phb++, pmhb++) {
	for (i = 0; i < 8; i++)
		transformation.Transform (rotVerts + i, pmhb->box.vertices + i, 0);
	for (i = 0, pf = phb->faces; i < 6; i++, pf++) {
		for (j = 0; j < 4; j++)
			pf->v [j] = rotVerts [hitboxFaceVerts [i][j]];
		VmVecNormal (pf->n + 1, pf->v, pf->v + 1, pf->v + 2);
		}
	}
transformation.End ();
}

#else //G3_HITBOX_TRANSFORM

void TransformHitboxes (CObject *objP, CFixVector *vPos, tBox *phb)
{
	tHitbox*		pmhb = &gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes [0];
	tQuad*		pf;
	CFixVector	rotVerts [8];
	CFixMatrix*	viewP = ObjectView (objP);
	int			i, j, iBox, nBoxes;

if (extraGameInfo [IsMultiGame].nHitboxes == 1) {
	iBox =
	nBoxes = 0;
	}
else {
	iBox = 1;
	nBoxes = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].nHitboxes;
	}
if (!vPos)
	vPos = &objP->info.position.vPos;
for (phb += iBox, pmhb += iBox; iBox <= nBoxes; iBox++, phb++, pmhb++) {
	for (i = 0; i < 8; i++) {
		rotVerts[i] = *viewP * pmhb->box.vertices[i];
		rotVerts[i] += *vPos;
		}
	for (i = 0, pf = phb->faces; i < 6; i++, pf++) {
		for (j = 0; j < 4; j++)
			pf->v [j] = rotVerts [hitboxFaceVerts [i][j]];
		pf->n[1] = CFixVector::Normal(pf->v[0], pf->v[1], pf->v[2]);
		}
	}
}

#endif //G3_HITBOX_TRANSFORM

//------------------------------------------------------------------------------
//eof
