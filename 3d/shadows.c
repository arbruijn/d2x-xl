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
#include "interp.h"
#include "shadows.h"
#include "globvars.h"
#include "gr.h"
#include "byteswap.h"
#include "u_mem.h"
#include "console.h"
#include "ogl_init.h"
#include "network.h"
#include "render.h"
#include "gameseg.h"
#include "lighting.h"
#include "lightning.h"

extern tFaceColor tMapColor;
extern int bPrintLine;

//------------------------------------------------------------------------------

//shadow clipping
//1: Compute hit point of vector from current light through each model vertex (fast)
//2: Compute hit point of vector from current light through each lit submodel vertex (slow)
//3: Compute hit point of vector from current light through each lit model face (fastest, flawed)

#define SHADOW_TEST				0
#define NORM_INF					1

float fInfinity [4] = {100.0f, 100.0f, 200.0f, 400.0f};	//5, 10, 20 standard cubes
float fInf;

extern tRgbaColorf shadowColor [2], modelColor [2];

static tOOF_vector	vLightPosf, 
							//vViewerPos, 
							vCenter;
static vmsVector vLightPos;

//#define N_OPCODES (sizeof (opcode_table) / sizeof (*opcode_table))

#define MAX_POINTS_PER_POLY 25

short nHighestTexture;
int g3d_interp_outline;

#define MAX_INTERP_COLORS 100

//this is a table of mappings from RGB15 to palette colors
struct {short pal_entry, rgb15;} interpColorTable [MAX_INTERP_COLORS];

#if DBG_SHADOWS
extern int bShadowTest;
extern int bFrontCap;
extern int bRearCap;
extern int bShadowVolume;
extern int bFrontFaces;
extern int bBackFaces;
extern int bSWCulling;
#endif
#if SHADOWS
extern int bZPass;
#endif
static int bTriangularize = 0;
//static int bIntrinsicFacing = 0;
static int bFlatPolys = 1;
static int bTexPolys = 1;

g3sPoint *pointList [MAX_POINTS_PER_POLY];

//------------------------------------------------------------------------------

inline void RotatePointListToVec (vmsVector *dest, vmsVector *src, int n)
{
while (n--)
	G3TransformPoint (dest++, src++, 0);
}

//------------------------------------------------------------------------------

inline int G3CheckPointFacing (tOOF_vector *pv, tOOF_vector *pNorm, tOOF_vector *pDir)
{
	tOOF_vector	h;

return OOF_VecMul (OOF_VecSub (&h, pDir, pv), pNorm) > 0;
}

//------------------------------------------------------------------------------

inline int G3CheckLightFacing (tOOF_vector *pv, tOOF_vector *pNorm)
{
return G3CheckPointFacing (pv, pNorm, &vLightPosf);
}

//------------------------------------------------------------------------------

inline int G3CheckViewerFacing (tOOF_vector *pv, tOOF_vector *pNorm)
{
return OOF_VecMul (pv, pNorm) >= 0;
}

//------------------------------------------------------------------------------

inline tOOF_vector *G3RotateFaceNormal (tPOF_face *pf)
{
return (tOOF_vector *) OOF_VecVms2Oof (&pf->vNormf, G3RotatePoint (&pf->vRotNorm, &pf->vNorm, 0));
}
					
//------------------------------------------------------------------------------

inline tOOF_vector *G3TransformFaceCenter (tPOF_face *pf)
{
	vmsVector	v;

return (tOOF_vector *) OOF_VecVms2Oof (&pf->vCenterf, G3TransformPoint (&v, &pf->vCenter, 0));
}
					
//------------------------------------------------------------------------------

inline int G3FaceIsLit (tPOFObject *po, tPOF_face *pf)
{
return pf->bFacingLight = G3CheckLightFacing (&pf->vCenterf, &pf->vNormf);
}

//------------------------------------------------------------------------------

inline int G3FaceIsFront (tPOFObject *po, tPOF_face *pf)
{
return pf->bFrontFace = G3CheckViewerFacing (&pf->vCenterf, &pf->vNormf);
}

//------------------------------------------------------------------------------

int G3GetFaceWinding (tOOF_vector *v0, tOOF_vector *v1, tOOF_vector *v2)
{
return ((v1->x - v0->x) * (v2->y - v1->y) < 0) ? GL_CW : GL_CCW;
}

//------------------------------------------------------------------------------

bool POFCountPolyModelItems (void *modelP, short *pnSubObjs, short *pnVerts, short *pnFaces, short *pnFaceVerts, short *pnAdjFaces)
{
	ubyte *p = modelP;

G3CheckAndSwap (modelP);
for (;;)
	switch (WORDVAL (p)) {
		case OP_EOF:
			return 1;

		case OP_DEFPOINTS: {
			int n = WORDVAL (p+2);
			(*pnVerts) += n;
			p += n * sizeof (vmsVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int n = WORDVAL (p+2);
			int s = (int) WORDVAL (p+4);
			p += n * sizeof (vmsVector) + 8;
			(*pnVerts) += n;
			break;
			}

		case OP_FLATPOLY: {
			int nVerts = WORDVAL (p+2);
			*pnAdjFaces += nVerts;
			(*pnFaces)++;
			(*pnFaceVerts) += nVerts;
			p += 30 + ((nVerts & ~ 1) + 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int nVerts = WORDVAL (p + 2);
			*pnAdjFaces += nVerts;
			(*pnFaces) += nVerts - 2;
			(*pnFaceVerts) += (nVerts - 2) * 3;
			p += 30 + ((nVerts&~1)+1)*2 + nVerts*12;
			break;
			}

		case OP_SORTNORM:
			POFCountPolyModelItems (p + WORDVAL (p+28), pnSubObjs, pnVerts, pnFaces, pnFaceVerts, pnAdjFaces);
			POFCountPolyModelItems (p + WORDVAL (p+30), pnSubObjs, pnVerts, pnFaces, pnFaceVerts, pnAdjFaces);
			p += 32;
			break;


		case OP_RODBM: {
			p+=36;
			break;
			}

		case OP_SUBCALL: {
			(*pnSubObjs)++;
			POFCountPolyModelItems (p + WORDVAL (p+16), pnSubObjs, pnVerts, pnFaces, pnFaceVerts, pnAdjFaces);
			p += 20;
			break;
			}

		case OP_GLOW:
			p += 4;
			break;

		default:
			Error ("invalid polygon model\n");
	}
return 1;
}

//------------------------------------------------------------------------------

short G3FindPolyModelVert (vmsVector *pVerts, vmsVector *pv, int nVerts)
{
	int	i;

for (i = 0; i < nVerts; i++, pVerts++)
	if ((pVerts->p.x == pv->p.x) && (pVerts->p.y == pv->p.y) && (pVerts->p.z == pv->p.z))
		return i;
return nVerts;
}

//------------------------------------------------------------------------------

void G3CalcFaceNormal (tPOFObject *po, tPOF_face *pf)
{
if (bTriangularize) {
	vmsVector	*pv = po->pvVerts;
	short			*pfv = pf->pVerts;

	VmVecNormal (&pf->vNorm, pv + pfv [0], pv + pfv [1], pv + pfv [2]);
	}
else
	OOF_VecVms2Oof (&pf->vNormf, &pf->vNorm);
}

//------------------------------------------------------------------------------

tOOF_vector *G3CalcFaceCenterf (tPOFObject *po, tPOF_face *pf)
{
	tOOF_vector	*pv = po->pvVertsf;
	short			*pfv = pf->pVerts;
	int			i;
	static tOOF_vector	c;

c.x = c.y = c.z = 0.0f;
for (i = pf->nVerts; i; i--, pfv++)
	OOF_VecInc (&c, pv + *pfv);
OOF_VecScale (&c, 1.0f / pf->nVerts);
return &c;
}

//------------------------------------------------------------------------------

vmsVector *G3CalcFaceCenter (tPOFObject *po, tPOF_face *pf)
{
	tOOF_vector	cf;
	static vmsVector c;

cf = *G3CalcFaceCenterf (po, pf);
c.p.x = (fix) (cf.x * F1_0);
c.p.y = (fix) (cf.y * F1_0);
c.p.z = (fix) (cf.z * F1_0);
return &c;
}

//------------------------------------------------------------------------------

inline void G3AddPolyModelTri (tPOFObject *po, tPOF_face *pf, short v0, short v1, short v2)
{
	short	*pfv = po->pFaceVerts + po->iFaceVert;

pf->pVerts = pfv;
pf->nVerts = 3;
pfv [0] = v0;
pfv [1] = v1;
pfv [2] = v2;
G3CalcFaceNormal (po, pf);
}

//------------------------------------------------------------------------------

#if 0

short G3FindPolyModelFace (tPOFObject *po, tPOF_face *pf)
{
	int			h, i, j, k, l;
	tPOF_face	*pfh;

for (h = pf->nVerts, i = po->iFace, pfh = po->faces.pFaces; i; i--, pfh++)
	if (pfh->nVerts == h) {
		for (k = h, j = 0; j < h; j++)
			for (l = 0; l < h; l++)
				if (pf->pVerts [j] == pfh->pVerts [l]) {
					k--;
					break;
					}
		if (!k)
			return i;
		}
return -1;
}

#else
//------------------------------------------------------------------------------

int G3FindPolyModelFace (tPOFObject *po, short *p, int nVerts)
{
	tPOF_face	*pf;
	int			h, i, j, k;
	short			*pfv;

for (i = po->iFace, pf = po->faces.pFaces; i; i--, pf++) {
	if (pf->nVerts != nVerts)
		continue;
	pfv = pf->pVerts;
	for (j = 0, h = *p; j < nVerts; j++) {
		if (h == pfv [j]) {
			h = j;
			for (k = 0; k < nVerts; k++, j = (j + 1) % nVerts) {
				if (p [k] != pfv [j]) {
					j = h;
					for (k = 0; k < nVerts; k++) {
						if (p [k] != pfv [j])
							break;
						if (!j)
							j = nVerts;
						j--;
						}
					break;
					}
				}
			if (k == nVerts)
				return 1;
			break;
			}
		}
	}
return 0;
}
#endif
//------------------------------------------------------------------------------

tPOF_face *POFAddPolyModelFace (tPOFObject *po, tPOFSubObject *pso, tPOF_face *pf, 
										  vmsVector *pn, ubyte *p, int bShadowData)
{
	short			nVerts = WORDVAL (p+2);
	vmsVector	*pv = po->pvVerts, v;
	short			*pfv;
	short			i, v0;

//if (G3FindPolyModelFace (po, WORDPTR (p+30), nVerts))
//	return pf;
if (bShadowData) {
	pf->vNorm = *pn;
	if (bTriangularize) {
		pfv = WORDPTR (p+30);
		v0 = *pfv;
		if (nVerts == 3) {
			G3AddPolyModelTri (po, pf, pfv [0], pfv [1], pfv [2]);
			po->iFaceVert += 3;
			pf->bGlow = (nGlow >= 0);
			pf++;
			po->iFace++;
			pso->faces.nFaces++;
			}
		else if (nVerts == 4) {
			VmVecSub (&v, pv + pfv [3], pv + pfv [1]);
			if (VmVecDot (pn, &v) < 0) {
				G3AddPolyModelTri (po, pf, pfv [0], pfv [1], pfv [3]);
				po->iFaceVert += 3;
				pf->bGlow = (nGlow >= 0);
				pf++;
				po->iFace++;
				pso->faces.nFaces++;
				G3AddPolyModelTri (po, pf, pfv [1], pfv [2], pfv [3]);
				po->iFaceVert += 3;
				pf->bGlow = (nGlow >= 0);
				pf++;
				po->iFace++;
				pso->faces.nFaces++;
				}
			else {
				G3AddPolyModelTri (po, pf, pfv [0], pfv [1], pfv [2]);
				po->iFaceVert += 3;
				pf->bGlow = (nGlow >= 0);
				pf++;
				po->iFace++;
				pso->faces.nFaces++;
				G3AddPolyModelTri (po, pf, pfv [0], pfv [2], pfv [3]);
				po->iFaceVert += 3;
				pf->bGlow = (nGlow >= 0);
				pf++;
				po->iFace++;
				pso->faces.nFaces++;
				}
			}
		else {
			for (i = 1; i < nVerts - 1; i++) {
				G3AddPolyModelTri (po, pf, v0, pfv [i], pfv [i + 1]);
				po->iFaceVert += 3;
				pf->bGlow = (nGlow >= 0);
				pf++;
				po->iFace++;
				pso->faces.nFaces++;
				}
			}
		}
	else { //if (nVerts < 5) {
		pfv = pf->pVerts = po->pFaceVerts + po->iFaceVert;
		pf->nVerts = nVerts;
		memcpy (pfv, WORDPTR (p+30), nVerts * sizeof (short));
		pf->bGlow = (nGlow >= 0);
		G3CalcFaceNormal (po, pf);
#if 0
		if (memcmp (&pf->vCenter, po->pvVerts + *pv, sizeof (vmsVector))) //make sure we have a vertex from the face
			pf->vCenter = po->pvVerts [*pv];
		VmVecNormal (&n, po->pvVerts + pv [0], po->pvVerts + pv [1], po->pvVerts + pv [2]); //check the precomputed normal
		if (memcmp (&pf->vNorm, &n, sizeof (vmsVector)))
			pf->vNorm = n;
#endif
#if 0
		if (G3FindPolyModelFace (po, pf) < 0) //check for duplicate faces
#endif
			{
			po->iFaceVert += nVerts;
			pf++;
			po->iFace++;
			pso->faces.nFaces++;
			}
		}
#if 0
	else {
		short h = (nVerts + 1) / 2;
		pfv = pf->pVerts = po->pFaceVerts + po->iFaceVert;
		pf->nVerts = h;
		memcpy (pfv, WORDPTR (p+30), h * sizeof (short));
		pf->bGlow = (nGlow >= 0);
		G3CalcFaceNormal (po, pf);
		po->iFaceVert += h;
		pf->bTest = 1;
		pf++;
		po->iFace++;
		pso->faces.nFaces++;
		pfv = pf->pVerts = po->pFaceVerts + po->iFaceVert;
		pf->nVerts = h;
		memcpy (pfv, WORDPTR (p + 30) + nVerts / 2, h * sizeof (short));
		pf->bGlow = (nGlow >= 0);
		G3CalcFaceNormal (po, pf);
		pf->bTest = 1;
		po->iFaceVert += h;
		pf++;
		po->iFace++;
		pso->faces.nFaces++;
		}
#endif
	}
else {
	fVector		nf;
	g3sNormal	*pvn;

	pfv = WORDPTR (p+30);
	VmsVecToFloat (&nf, pn);
	for (i = 0; i < nVerts; i++) {
		pvn = po->pVertNorms + pfv [i];
		VmVecIncf (&pvn->vNormal, &nf);
		pvn->nFaces++;
		}
	}
return pf;
}

//------------------------------------------------------------------------------

void G3RotatePolyModelNormals (tPOFObject *po, tPOFSubObject *pso)
{
	tPOF_face	*pf;
	int			i;

for (i = pso->faces.nFaces, pf = pso->faces.pFaces; i; i--, pf++) {
	G3RotateFaceNormal (pf);
	G3TransformFaceCenter (pf);
	}
}

//------------------------------------------------------------------------------

tOOF_vector *G3PolyModelVerts2Float (tPOFObject *po)
{
	int			i;
	vmsVector	*pv;
	tOOF_vector	*pvf;

for (i = po->nVerts, pv = po->pvVerts, pvf = po->pvVertsf; i; i--, pv++, pvf++) {
	pvf->x = f2fl (pv->p.x);
	pvf->y = f2fl (pv->p.y);
	pvf->z = f2fl (pv->p.z);
	}
return po->pvVertsf;
}

//------------------------------------------------------------------------------

#define MAXGAP	0.01f;

int G3FindPolyModelEdge (tPOFSubObject *pso, tPOF_face *pf0, int v0, int v1)
{
	int			h, i, j, n;
	tPOF_face	*pf1;
	short			*pfv;

for (i = pso->faces.nFaces, pf1 = pso->faces.pFaces; i; i--, pf1++) {
	if (pf1 != pf0) {
		for (j = 0, n = pf1->nVerts, pfv = pf1->pVerts; j < n; j++) {
			h = (j + 1) % n;
			if (((pfv [j] == v0) && (pfv [h] == v1)) || ((pfv [j] == v1) && (pfv [h] == v0)))
				return (int) (pf1 - pso->faces.pFaces);
			}
		}
	}
return -1;
}

//------------------------------------------------------------------------------

int G3GetAdjFaces (tPOFObject *po)
{
	short				h, i, j, n;
	tPOFSubObject	*pso = po->subObjs.pSubObjs;
	tPOF_face		*pf;
	short				*pfv;

po->nAdjFaces = 0;
G3PolyModelVerts2Float (po);
for (i = po->subObjs.nSubObjs; i; i--, pso++) {
	pso->pAdjFaces = po->pAdjFaces + po->nAdjFaces;
	for (j = pso->faces.nFaces, pf = pso->faces.pFaces; j; j--, pf++) {
		pf->nAdjFaces = po->nAdjFaces;
		for (h = 0, n = pf->nVerts, pfv = pf->pVerts; h < n; h++)
			po->pAdjFaces [po->nAdjFaces++] = G3FindPolyModelEdge (pso, pf, pfv [h], pfv [(h + 1) % n]);
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int G3CalcModelFacing (tPOFObject *po, tPOFSubObject *pso)
{
	short				i;
	tPOF_face		*pf;

G3RotatePolyModelNormals (po, pso);
for (i = pso->faces.nFaces, pf = pso->faces.pFaces; i; i--, pf++) {
	G3FaceIsLit (po, pf);
#if 0
	G3FaceIsFront (po, pf);
#endif
	}
return pso->faces.nFaces;
}

//------------------------------------------------------------------------------

int G3GetLitFaces (tPOFObject *po, tPOFSubObject *pso)
{
	short				i;
	tPOF_face		*pf;

G3CalcModelFacing (po, pso);
pso->litFaces.nFaces = 0;
pso->litFaces.pFaces = po->litFaces.pFaces + po->litFaces.nFaces;
for (i = pso->faces.nFaces, pf = pso->faces.pFaces; i; i--, pf++) {
	if (pf->bFacingLight) {
		po->litFaces.pFaces [po->litFaces.nFaces++] = pf;
		pso->litFaces.nFaces++;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void G3GetPolyModelCenters (tPOFObject *po)
{
	short				i, j;
	tPOFSubObject	*pso = po->subObjs.pSubObjs;
	tPOF_face		*pf;

for (i = po->subObjs.nSubObjs; i; i--, pso++)
	for (j = pso->faces.nFaces, pf = pso->faces.pFaces; j; j--, pf++)
		pf->vCenter = *G3CalcFaceCenter (po, pf);
}

//------------------------------------------------------------------------------

bool POFGetPolyModelItems (void *modelP, vmsAngVec *pAnimAngles, tPOFObject *po, 
								  int bInitModel, int bShadowData, int nThis, int nParent)
{
	ubyte				*p = modelP;
	tPOFSubObject	*pso = po->subObjs.pSubObjs + nThis;
	tPOF_face		*pf = po->faces.pFaces + po->iFace;
	int				nChild;

G3CheckAndSwap (modelP);
nGlow = -1;
if (bInitModel && !pso->faces.pFaces) {
	pso->faces.pFaces = pf;
	pso->nParent = nParent;
	}
for (;;)
	switch (WORDVAL (p)) {
		case OP_EOF:
			return 1;
		case OP_DEFPOINTS: {
			int n = WORDVAL (p+2);
			if (bInitModel)
				memcpy (po->pvVerts, VECPTR (p+4), n * sizeof (vmsVector));
			else
				RotatePointListToVec (po->pvVerts, VECPTR (p+4), n);
			//po->nVerts += n;
			p += n * sizeof (vmsVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int n = WORDVAL (p+2);
			int s = WORDVAL (p+4);
			if (bInitModel)
				memcpy (po->pvVerts + s, VECPTR (p+8), n * sizeof (vmsVector));
			else
				RotatePointListToVec (po->pvVerts + s, VECPTR (p+8), n);
			p += n * sizeof (vmsVector) + 8;
			break;
			}

		case OP_FLATPOLY: {
			int nVerts = WORDVAL (p+2);
			if (bInitModel && bFlatPolys) {
				pf = POFAddPolyModelFace (po, pso, pf, VECPTR (p+16), p, bShadowData);
				}
			p += 30 + (nVerts | 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int nVerts = WORDVAL (p + 2);
			if (bInitModel && bTexPolys) {
				pf = POFAddPolyModelFace (po, pso, pf, VECPTR (p+16), p, bShadowData);
				}
			p += 30 + (nVerts | 1) * 2 + nVerts * 12;
			break;
			}

		case OP_SORTNORM:
			POFGetPolyModelItems (p + WORDVAL (p+28), pAnimAngles, po, bInitModel, bShadowData, nThis, nParent);
			if (bInitModel)
				pf = po->faces.pFaces + po->iFace;
			POFGetPolyModelItems (p + WORDVAL (p+30), pAnimAngles, po, bInitModel, bShadowData, nThis, nParent);
			if (bInitModel)
				pf = po->faces.pFaces + po->iFace;
			p += 32;
			break;


		case OP_RODBM:
			p+=36;
			break;

		case OP_SUBCALL: {
			vmsAngVec *a;

			if (pAnimAngles)
				a = &pAnimAngles [WORDVAL (p+2)];
			else
				a = &avZero;
			nChild = ++po->iSubObj;
			po->subObjs.pSubObjs [nChild].vPos = *VECPTR (p+4);
			po->subObjs.pSubObjs [nChild].vAngles = *a;
			G3StartInstanceAngles (VECPTR (p+4), a);
			POFGetPolyModelItems (p + WORDVAL (p+16), pAnimAngles, po, bInitModel, bShadowData, nChild, nThis);
			G3DoneInstance ();
			if (bInitModel)
				pf = po->faces.pFaces + po->iFace;
			p += 20;
			break;
			}

		case OP_GLOW:
			nGlow = WORDVAL (p+2);
			p += 4;
			break;

		default:
			Error ("invalid polygon model\n");
	}
return 1;
}

//------------------------------------------------------------------------------

#define pof_free(_p)	{if (_p) D2_FREE (_p);}

int POFFreePolyModelItems (tPOFObject *po)
{
pof_free (po->subObjs.pSubObjs);
pof_free (po->pvVerts);
pof_free (po->pvVertsf);
pof_free (po->pfClipDist);
pof_free (po->pVertFlags);
pof_free (po->pVertNorms);
pof_free (po->faces.pFaces);
pof_free (po->litFaces.pFaces);
pof_free (po->pAdjFaces);
pof_free (po->pFaceVerts);
memset (po, 0, sizeof (po));
return 0;
}

//------------------------------------------------------------------------------

int POFAllocPolyModelItems (void *modelP, tPOFObject *po, int bShadowData)
{
	short	nFaceVerts = 0;
	int	h;

//memset (po, 0, sizeof (po));
po->subObjs.nSubObjs = 1;
POFCountPolyModelItems (modelP, &po->subObjs.nSubObjs, &po->nVerts, &po->faces.nFaces, &nFaceVerts, &po->nAdjFaces);
h = po->subObjs.nSubObjs * sizeof (tPOFSubObject);
if (!(po->subObjs.pSubObjs = (tPOFSubObject *) D2_ALLOC (h)))
	return POFFreePolyModelItems (po);
memset (po->subObjs.pSubObjs, 0, h);
h = po->nVerts;
if (!(po->pvVerts = (vmsVector *) D2_ALLOC (h * sizeof (vmsVector))))
	return POFFreePolyModelItems (po);
if (!(po->pfClipDist = (float *) D2_ALLOC (h * sizeof (float))))
	gameOpts->render.shadows.nClip = 1;
if (!(po->pVertFlags = (ubyte *) D2_ALLOC (h * sizeof (ubyte))))
	gameOpts->render.shadows.nClip = 1;
if (bShadowData) {
	if (!(po->faces.pFaces = (tPOF_face *) D2_ALLOC (po->faces.nFaces * sizeof (tPOF_face))))
		return POFFreePolyModelItems (po);
	if (!(po->litFaces.pFaces = (tPOF_face **) D2_ALLOC (po->faces.nFaces * sizeof (tPOF_face *))))
		return POFFreePolyModelItems (po);
	if (!(po->pAdjFaces = (short *) D2_ALLOC (po->nAdjFaces * sizeof (short))))
		return POFFreePolyModelItems (po);
	if (!(po->pvVertsf = (tOOF_vector *) D2_ALLOC (po->nVerts * sizeof (tOOF_vector))))
		return POFFreePolyModelItems (po);
	if (!(po->pFaceVerts = (short *) D2_ALLOC (nFaceVerts * sizeof (short))))
		return POFFreePolyModelItems (po);
	}
if (!(po->pVertNorms = (g3sNormal *) D2_ALLOC (h * sizeof (g3sNormal))))
	return POFFreePolyModelItems (po);
memset (po->pVertNorms, 0, h * sizeof (g3sNormal));
return po->nState = 1;
}

//------------------------------------------------------------------------------

void G3SetCullAndStencil (int bCullFront, int bZPass)
{
glEnable (GL_CULL_FACE);
if (bCullFront) {
	glCullFace (GL_FRONT);
	if (bZPass)
		glStencilOp (GL_KEEP, GL_KEEP, GL_INCR_WRAP);
	else
		glStencilOp (GL_KEEP, GL_INCR_WRAP, GL_KEEP);
	}
else {
	glCullFace (GL_BACK);
	if (bZPass)
		glStencilOp (GL_KEEP, GL_KEEP, GL_DECR_WRAP);
	else
		glStencilOp (GL_KEEP, GL_DECR_WRAP, GL_KEEP);
	}
}

//------------------------------------------------------------------------------

void G3RenderShadowVolumeFace (tOOF_vector *pv)
{
	tOOF_vector	v [4];

memcpy (v, pv, 2 * sizeof (tOOF_vector));
OOF_VecSub (v+3, v, &gameData.render.shadows.vLightPos);
OOF_VecSub (v+2, v+1, &gameData.render.shadows.vLightPos);
#if NORM_INF
OOF_VecScale (v+3, fInf / OOF_VecMag (v+3));
OOF_VecScale (v+2, fInf / OOF_VecMag (v+2));
#else
OOF_VecScale (v+3, fInf);
OOF_VecScale (v+2, fInf);
#endif
OOF_VecInc (v+2, v+1);
OOF_VecInc (v+3, v);
glVertexPointer (3, GL_FLOAT, 0, v);
#if DBG_SHADOWS
if (bShadowTest)
	glDrawArrays (GL_LINES, 0, 4);
else
#endif
glDrawArrays (GL_QUADS, 0, 4);
}

//------------------------------------------------------------------------------

void G3RenderFarShadowCapFace (tOOF_vector *pv, int nVerts)
{
	tOOF_vector	v0, v1;

#if DBG_SHADOWS
if (bShadowTest == 1)
	glColor4fv ((GLfloat *) shadowColor);
else if (bShadowTest > 1)
	glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
#	if 0
if (bShadowTest)
	glBegin (GL_LINE_LOOP);
else
#	endif
#endif
glBegin (GL_TRIANGLE_FAN);
for (pv += nVerts; nVerts; nVerts--) {
	v0 = *(--pv);
	OOF_VecSub (&v1, &v0, &vLightPosf);
#if NORM_INF
	OOF_VecScale (&v1, fInf / OOF_VecMag (&v1));
#else
	OOF_VecScale (&v1, fInf);
#endif
	OOF_VecInc (&v0, &v1);
	glVertex3fv ((GLfloat *) &v0);
	}	
glEnd ();
}

//------------------------------------------------------------------------------

int G3RenderSubModelShadowVolume (tPOFObject *po, tPOFSubObject *pso, int bCullFront)
{
	tOOF_vector	*pvf, v [4];
	tPOF_face	*pf, **ppf;
	short			*pfv, *paf;
	short			h, i, j, n;
	float			fClipDist;
	int			nClip;

#if DBG_SHADOWS
if (!bShadowVolume)
	return 1;
if (bCullFront && !bBackFaces)
	return 1;
if (!bCullFront && !bFrontFaces)
	return 1;
if (bShadowTest == 1)
	glColor4fv ((GLfloat *) shadowColor);
else if (bShadowTest > 1)
	glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
#endif
G3SetCullAndStencil (bCullFront, bZPass);
pvf = po->pvVertsf;
#if DBG_SHADOWS
if (bShadowTest < 2)
#endif
glEnableClientState (GL_VERTEX_ARRAY);
#if DBG_SHADOWS
else if (bShadowTest == 2)
	glLineWidth (3);
else {
	glLineWidth (3);
	glBegin (GL_LINES);
	}
#endif
nClip = gameOpts->render.shadows.nClip ? po->pfClipDist ? gameOpts->render.shadows.nClip : 1 : 0;
fClipDist = (nClip >= 2) ? pso->fClipDist : fInf;
for (i = pso->litFaces.nFaces, ppf = pso->litFaces.pFaces; i; i--, ppf++) {
	pf = *ppf;
	paf = po->pAdjFaces + pf->nAdjFaces;
	for (j = 0, n = pf->nVerts, pfv = pf->pVerts; j < n; j++) {
		h = *paf++;
		if ((h < 0) || !pso->faces.pFaces [h].bFacingLight) {
			v [1] = pvf [pfv [j]];
			v [0] = pvf [pfv [(j + 1) % n]];
#if DBG_SHADOWS
			if (bShadowTest < 3) {
				glColor4fv ((GLfloat *) (shadowColor + bCullFront));
#endif
				OOF_VecSub (v+3, v, &vLightPosf);
				OOF_VecSub (v+2, v+1, &vLightPosf);
#if NORM_INF
				OOF_VecScale (v+3, fClipDist / OOF_VecMag (v+3));
				OOF_VecScale (v+2, fClipDist / OOF_VecMag (v+2));
#else
				OOF_VecScale (v+3, fClipDist);
				OOF_VecScale (v+2, fClipDist);
#endif
				OOF_VecInc (v+2, v+1);
				OOF_VecInc (v+3, v);
#if 1//def RELEASE
				glVertexPointer (3, GL_FLOAT, 0, v);
				glDrawArrays (GL_QUADS, 0, 4);
#else
				glVertex3fv ((GLfloat *) v);
				glVertex3fv ((GLfloat *) (v+1));
				glVertex3fv ((GLfloat *) (v+2));
				glVertex3fv ((GLfloat *) (v+3));
#endif
#if DBG_SHADOWS
				}
			else {
				glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
				glVertex3fv ((GLfloat *) v);
				glVertex3fv ((GLfloat *) (v + 1));
				}
#endif
			}
		}
	}
#if DBG_SHADOWS
glLineWidth (1);
if (bShadowTest != 2)
#endif
glDisableClientState (GL_VERTEX_ARRAY);
return 1;
}

//	-----------------------------------------------------------------------------

int LineHitsFace (vmsVector *pHit, vmsVector *p0, vmsVector *p1, short nSegment, short nSide)
{
	short			i, nFaces, nVerts;
	tSegment		*segP = gameData.segs.segments + nSegment;

nFaces = GetNumFaces (segP->sides + nSide);
nVerts = 5 - nFaces;
for (i = 0; i < nFaces; i++)
	if (CheckLineToSegFace (pHit, p0, p1, nSegment, nSide, i, nVerts, 0))
		return nSide;
return -1;
}

//	-----------------------------------------------------------------------------

float NearestShadowedWallDist (short nObject, short nSegment, vmsVector *vPos, float fScale)
{
	static float fClip [4] = {1.1f, 1.5f, 2.0f, 3.0f};

#if 1
	vmsVector	vHit, v, vh;
	tSegment		*segP;
	int			nSide, nHitSide, nParent, nChild, nWID, bHit = 0;
	float			fDist;
#if USE_SEGRADS
	fix			xDist;
#endif
	static		unsigned int nVisited = 0;
	static		unsigned int bVisited [MAX_SEGMENTS_D2X];

if (0 > (nSegment = FindSegByPoint (vPos, nSegment, 1)))
	return G3_INFINITY;
VmVecSub (&v, vPos, &vLightPos);
VmVecNormalize (&v);
VmVecScale (&v, (fix) F1_0 * (fix) G3_INFINITY);
if (!nVisited++)
	memset (bVisited, 0, gameData.segs.nSegments * sizeof (unsigned int));
#ifdef _DEBUG
if (bPrintLine) {	
	fVector	vf;
	glLineWidth (3);
	if (!bShadowTest) {
		glColorMask (1,1,1,1);
		glDisable (GL_STENCIL_TEST);
		}
	glColor4d (1,0.8,0,1);
	glBegin (GL_LINES);
	VmsVecToFloat (&vf, vPos);
	glVertex3fv ((GLfloat*) &vf);
	VmsVecToFloat (&vf, &v);
	glVertex3fv ((GLfloat*) &vf);
	glEnd ();
	if (!bShadowTest) {
		glColorMask (0,0,0,0);
		glEnable (GL_STENCIL_TEST);
		}
	}
#endif
nParent = 0x7fffffff;
vHit = *vPos;
for (;;) {
	segP = gameData.segs.segments + nSegment;
	bVisited [nSegment] = nVisited;
	nHitSide = -1;
#if USE_SEGRADS
	for (nSide = 0; nSide < 6; nSide++) {
		nChild = segP->children [nSide];
		if ((nChild < 0) || (bVisited [nChild] == nVisited))
			continue;
		xDist = VmLinePointDist (vPos, &v, gameData.segs.segCenters [1] + nChild);
		if (xDist <= gameData.segs.segRads [0][nChild]) {
			nHitSide = LineHitsFace (&vHit, vPos, &v, nSegment, nSide);
			break;
			}
		} 
	if (nHitSide < 0) 
#endif
		{
		for (nSide = 0; nSide < 6; nSide++) {
			nChild = segP->children [nSide];
			if ((nChild >= 0) && (bVisited [nChild] == nVisited))
				continue;
			if (0 <= LineHitsFace (&vHit, vPos, &v, nSegment, nSide)) {
				VmVecSub (&vh, &vHit, vPos);
				if (VmVecDot (&vh, &v) > 0) {
					nHitSide = nSide;
					break;
					}
				}
			}
		}
	if (nHitSide < 0)
		break;
	fDist = f2fl (VmVecDist (vPos, &vHit));
	if (nChild < 0) {
		bHit = 1;
		break;
		}
	nWID = WALL_IS_DOORWAY (segP, nHitSide, gameData.objs.objects + nObject);
	if (!(nWID & WID_FLY_FLAG) &&
		 (((nWID & (WID_RENDER_FLAG | WID_RENDPAST_FLAG)) != (WID_RENDER_FLAG | WID_RENDPAST_FLAG)))) {
		bHit = 1;
		break;
		}
	if (fDist >= G3_INFINITY)
		break;
	nParent = nSegment;
	nSegment = nChild;
	}
if (bHit)
	return fDist * (/*fScale ? fScale :*/ fClip [gameOpts->render.shadows.nReach]);
return G3_INFINITY;

#else //slower method

	tVFIQuery	fq;
	tFVIData		fi;
	vmsVector	v;

if (!gameOpts->render.shadows.nClip)
	return G3_INFINITY;
if (0 > (nSegment = FindSegByPoint (vPos, nSegment, 1)))
	return G3_INFINITY;
fq.p0				  = vPos;
VmVecSub (&v, fq.p0, &vLightPos);
VmVecNormalize (&v);
VmVecScale (&v, (fix) F1_0 * (fix) G3_INFINITY);
fq.startSeg		  = nSegment;
fq.p1				  = &v;
fq.rad			  = 0;
fq.thisObjNum	  = nObject;
fq.ignoreObjList = NULL;
fq.flags			  = FQ_TRANSWALL;
if (FindVectorIntersection (&fq, &fi) != HIT_WALL)
	return G3_INFINITY;
return //fScale ? f2fl (VmVecDist (fq.p0, &fi.hit.vPoint)) * fScale : 
		 f2fl (VmVecDist (fq.p0, &fi.hit.vPoint)) * fClip [gameOpts->render.shadows.nReach];
#endif
}

//------------------------------------------------------------------------------

inline float G3FaceClipDist (tObject *objP, tPOF_face *pf)
{
	vmsVector	vCenter;

vCenter.p.x = (fix) (pf->vCenterf.x * 65536.0f);
vCenter.p.y = (fix) (pf->vCenterf.y * 65536.0f);
vCenter.p.z = (fix) (pf->vCenterf.z * 65536.0f);
return NearestShadowedWallDist (OBJ_IDX (objP), objP->nSegment, &vCenter, 0.0f);
}

//------------------------------------------------------------------------------
// use face centers to determine clipping distance

float G3ClipDistByFaceCenters (tObject *objP, tPOFObject *po, tPOFSubObject *pso, int i, int incr)
{
	tPOF_face	*pf, **ppf;
	short			h;
	float			fClipDist, fMaxDist = 0;

for (h = pso->litFaces.nFaces, ppf = pso->litFaces.pFaces + i; i < h; i += incr, ppf += incr) {
	pf = *ppf;
	fClipDist = G3FaceClipDist (objP, pf);
#ifdef _DEBUG
	if (fClipDist == G3_INFINITY)
		fClipDist = G3FaceClipDist (objP, pf);
#endif
	if (fMaxDist < fClipDist)
		fMaxDist = fClipDist;
	}
return fMaxDist;
}

//------------------------------------------------------------------------------

float G3ClipDistByFaceVerts (tObject *objP, tPOFObject *po, tPOFSubObject *pso, 
									  float fMaxDist, int i, int incr)
{
	tOOF_vector	*pv;
	vmsVector	v;
	float			*pfc;
	tPOF_face	*pf, **ppf;
	short			*pfv, h, j, m, n;
	short			nObject = OBJ_IDX (objP);
	short			nPointSeg, nSegment = objP->nSegment;
	float			fClipDist;

pv = po->pvVertsf;
pfc = po->pfClipDist;
for (m = pso->litFaces.nFaces, ppf = pso->litFaces.pFaces + i; i < m; i += incr, ppf += incr) {
	pf = *ppf;
	for (j = 0, n = pf->nVerts, pfv = pf->pVerts; j < n; j++, pfv++) {
		h = *pfv;
		if (!(fClipDist = pfc [h])) {
			v.p.x = (fix) (pv [h].x * 65536.0f);
			v.p.y = (fix) (pv [h].y * 65536.0f);
			v.p.z = (fix) (pv [h].z * 65536.0f);
			nPointSeg = FindSegByPoint (&v, nSegment, 1);
			if (nPointSeg < 0)
				continue;
			pfc [h] = fClipDist = NearestShadowedWallDist (nObject, nPointSeg, &v, 3.0f);
#ifdef _DEBUG
			if (fClipDist == G3_INFINITY)
				fClipDist = NearestShadowedWallDist (nObject, nPointSeg, &v, 3.0f);
#endif
			}
		if (fMaxDist < fClipDist)
			fMaxDist = fClipDist;
		}
	}
return fMaxDist;
}

//------------------------------------------------------------------------------

float G3ClipDistByLitVerts (tObject *objP, tPOFObject *po, float fMaxDist, int i, int incr)
{
	tOOF_vector	*pv;
	vmsVector	v;
	float			*pfc;
	short			j;
	ubyte			*pvf;
	short			nObject = OBJ_IDX (objP);
	short			nPointSeg, nSegment = objP->nSegment;
	float			fClipDist;
	ubyte			nVertFlag = po->nVertFlag;

pv = po->pvVertsf;
pfc = po->pfClipDist;
pvf = po->pVertFlags + i;
for (j = po->nVerts; i < j; i += incr, pvf += incr) {
	if (*pvf != nVertFlag)
		continue;
	if (!(fClipDist = pfc [i])) {
		v.p.x = (fix) (pv [i].x * 65536.0f);
		v.p.y = (fix) (pv [i].y * 65536.0f);
		v.p.z = (fix) (pv [i].z * 65536.0f);
		nPointSeg = FindSegByPoint (&v, nSegment, 1);
		if (nPointSeg < 0)
			continue;
		pfc [i] = fClipDist = NearestShadowedWallDist (nObject, nPointSeg, &v, 3.0f);
#ifdef _DEBUG
		if (fClipDist == G3_INFINITY)
			fClipDist = NearestShadowedWallDist (nObject, nPointSeg, &v, 3.0f);
#endif
		}
	if (fMaxDist < fClipDist)
		fMaxDist = fClipDist;
	}
return fMaxDist;
}

//------------------------------------------------------------------------------

#if MULTI_THREADED_SHADOWS

int _CDECL_ ClipDistThread (void *pThreadId)
{
	int		nId = *((int *) pThreadId);

while (!gameStates.app.bExit) {
	while (!gameData.threads.clipDist.info [nId].bExec)
		Sleep (0);
	gameData.threads.clipDist.data.fClipDist [nId] = 
		G3ClipDistByFaceCenters (gameData.threads.clipDist.data.objP, 
										 gameData.threads.clipDist.data.po, 
										 gameData.threads.clipDist.data.pso, nId, 2);
	if (gameOpts->render.shadows.nClip == 3)
		gameData.threads.clipDist.data.fClipDist [nId] = 
			G3ClipDistByLitVerts (gameData.threads.clipDist.data.objP, 
										 gameData.threads.clipDist.data.po, 
										 gameData.threads.clipDist.data.fClipDist [nId],
										 nId, 2);
	gameData.threads.clipDist.info [nId].bExec = 0;
	}
return 0;
}

#endif

//------------------------------------------------------------------------------

void G3GetLitVertices (tPOFObject *po, tPOFSubObject *pso)
{
	tPOF_face	*pf, **ppf;
	short			*pfv, i, j;
	ubyte			*pvf;
	ubyte			nVertFlag = po->nVertFlag++;

if (!nVertFlag)
	memset (po->pVertFlags, 0, po->nVerts * sizeof (ubyte));
nVertFlag++;
pvf = po->pVertFlags;
for (i = pso->litFaces.nFaces, ppf = pso->litFaces.pFaces; i; i--, ppf++) {
	pf = *ppf;
	for (j = pf->nVerts, pfv = pf->pVerts; j; j--, pfv++)
		pvf [*pfv] = nVertFlag;
	}
}

//------------------------------------------------------------------------------
// use face centers and vertices to determine clipping distance

float G3SubModelClipDist (tObject *objP, tPOFObject *po, tPOFSubObject *pso)
{
	float	fMaxDist = 0;

if (gameOpts->render.shadows.nClip < 2)
	return fInf;
#if 0
if (!pso->bCalcClipDist)
	return pso->fClipDist;	//only recompute every 2nd frame
pso->bCalcClipDist = 0;
#endif
#if MULTI_THREADED_SHADOWS
	if (gameStates.app.bMultiThreaded) {
	gameData.threads.clipDist.data.objP = objP;
	gameData.threads.clipDist.data.po = po;
	gameData.threads.clipDist.data.pso = pso;
	G3GetLitVertices (po, pso);
	SDL_SemPost (gameData.threads.clipDist.info [0].exec);
	SDL_SemPost (gameData.threads.clipDist.info [1].exec);
	SDL_SemWait (gameData.threads.clipDist.info [0].done);
	SDL_SemWait (gameData.threads.clipDist.info [1].done);
	fMaxDist = (gameData.threads.clipDist.data.fClipDist [0] > gameData.threads.clipDist.data.fClipDist [1]) ?
					gameData.threads.clipDist.data.fClipDist [0] : gameData.threads.clipDist.data.fClipDist [1];
	}
else 
#endif
	{
	fMaxDist = G3ClipDistByFaceCenters (objP, po, pso, 0, 1);
	if (gameOpts->render.shadows.nClip == 3)
		fMaxDist = G3ClipDistByFaceVerts (objP, po, pso, fMaxDist, 0, 1);
	}
return pso->fClipDist = (fMaxDist ? fMaxDist : (fInf < G3_INFINITY) ? fInf : G3_INFINITY);
}

//------------------------------------------------------------------------------

int G3RenderSubModelShadowCaps (tObject *objP, tPOFObject *po, tPOFSubObject *pso, int bCullFront)
{
	tOOF_vector	*pvf, v0, v1;
	tPOF_face	*pf, **ppf;
	short			*pfv, i, j;
	float			fClipDist;
	int			nClip;

#if DBG_SHADOWS
if (bShadowTest) {
	glColor4fv ((GLfloat *) (modelColor + bCullFront));
	glDepthFunc (GL_LEQUAL);
	}
#endif
G3SetCullAndStencil (bCullFront, bZPass);
pvf = po->pvVertsf;
#if DBG_SHADOWS
if (bRearCap)
#endif
nClip = gameOpts->render.shadows.nClip ? po->pfClipDist ? gameOpts->render.shadows.nClip : 1 : 0;
fClipDist = (nClip >= 2) ? pso->fClipDist : fInf;
for (i = pso->litFaces.nFaces, ppf = pso->litFaces.pFaces; i; i--, ppf++) {
	pf = *ppf;
#ifdef _DEBUG
	if (pf->bFacingLight && (bShadowTest > 3)) {
		glColor4f (0.20f, 0.8f, 1.0f, 1.0f);
		v1 = v0 = pf->vCenterf;
		glBegin (GL_LINES);
		glVertex3fv ((GLfloat *) &v0);
		OOF_VecInc (&v0, &pf->vNormf);
		glVertex3fv ((GLfloat *) &v0);
		glEnd ();
		glColor4d (0,0,1,1);
		glBegin (GL_LINES);
		glVertex3fv ((GLfloat *) &v1);
		glVertex3fv ((GLfloat *) &vLightPosf);
		glEnd ();
		glColor4fv ((GLfloat *) (modelColor + bCullFront));
		}
	if (bShadowTest && (bShadowTest != 2)) {
		glLineWidth (1);
		glBegin (GL_LINE_LOOP);
		}
	else
#endif
		glBegin (GL_TRIANGLE_FAN);
	for (j = pf->nVerts, pfv = pf->pVerts + j; j; j--) {
		v0 = pvf [*--pfv];
#if 1
		OOF_VecSub (&v1, &v0, &vLightPosf);
#if DBG_SHADOWS
		if (bShadowTest < 4) 
#endif
			{
#if NORM_INF
#	if DBG_SHADOWS
			if (bShadowTest == 2)
				OOF_VecScale (&v1, 5.0f / OOF_VecMag (&v1));
			else
#	endif
				OOF_VecScale (&v1, fClipDist / OOF_VecMag (&v1));
#else
			OOF_VecScale (&v1, fClipDist);
#endif
			OOF_VecInc (&v0, &v1);
#endif
			}
		glVertex3fv ((GLfloat *) &v0);
		}	
	glEnd ();
	}
#if DBG_SHADOWS
if (bFrontCap)
#endif
for (i = pso->litFaces.nFaces, ppf = pso->litFaces.pFaces; i; i--, ppf++) {
	pf = *ppf;
	if (!pf->bFacingLight)
		continue;
#if DBG_SHADOWS
	if (pf->bFacingLight && (bShadowTest > 3)) {
		glColor4f (1.0f, 0.8f, 0.2f, 1.0f);
		v1 = v0 = pf->vCenterf;
		glBegin (GL_LINES);
		glVertex3fv ((GLfloat *) &v0);
		OOF_VecInc (&v0, &pf->vNormf);
		glVertex3fv ((GLfloat *) &v0);
		glEnd ();
		glColor4d (1,0,0,1);
		glBegin (GL_LINES);
		glVertex3fv ((GLfloat *) &v1);
		glVertex3fv ((GLfloat *) &vLightPosf);
		glEnd ();
		glColor4fv ((GLfloat *) (modelColor + bCullFront));
		}
	if (bShadowTest && (bShadowTest != 2)) {
		glLineWidth (1);
		glBegin (GL_LINE_LOOP);
		}
	else
#endif
		glBegin (GL_TRIANGLE_FAN);
	for (j = pf->nVerts, pfv = pf->pVerts; j; j--) {
		v0 = pvf [*pfv++];
		glVertex3fv ((GLfloat *) &v0);
		}
	glEnd ();
	}
return 1;
}

//------------------------------------------------------------------------------

int G3DrawSubModelShadow (tObject *objP, tPOFObject *po, tPOFSubObject *pso)
{
	int			h = 1, i;

if (pso->nParent >= 0)
	G3StartInstanceAngles (&pso->vPos, &pso->vAngles);
h = (int) (pso - po->subObjs.pSubObjs);
for (i = 0; i < po->subObjs.nSubObjs; i++)
	if (po->subObjs.pSubObjs [i].nParent == h)
		G3DrawSubModelShadow (objP, po, po->subObjs.pSubObjs + i);
#ifdef _DEBUG
#	if 0
if (pso - po->subObjs.pSubObjs == 8)
#	endif
#endif
{
G3GetLitFaces (po, pso);
if ((pso->nRenderFlipFlop = !pso->nRenderFlipFlop))
	pso->fClipDist = G3SubModelClipDist (objP, po, pso);
h = G3RenderSubModelShadowCaps (objP, po, pso, 0) &&
	 G3RenderSubModelShadowCaps (objP, po, pso, 1) &&
	 G3RenderSubModelShadowVolume (po, pso, 0) &&
	 G3RenderSubModelShadowVolume (po, pso, 1);
;
}
if (pso->nParent >= 0)
	G3DoneInstance ();
return h;
}

//------------------------------------------------------------------------------

int POFGatherPolyModelItems (tObject *objP, void *modelP, vmsAngVec *pAnimAngles, tPOFObject *po, int bShadowData)
{
	int			j;
	vmsVector	*pv;
	tOOF_vector	*pvf;

if (!(po->nState || POFAllocPolyModelItems (modelP, po, bShadowData)))
	return 0;
if (po->nState == 1) {
	G3StartInstanceMatrix (&objP->position.vPos, &objP->position.mOrient);
	POFGetPolyModelItems (modelP, pAnimAngles, po, 1, bShadowData, 0, -1);
	if (bShadowData) {
		vCenter.x = vCenter.y = vCenter.z = 0.0f;
		for (j = po->nVerts, pv = po->pvVerts, pvf = po->pvVertsf; j; j--, pv++, pvf++) {
			vCenter.x += f2fl (pv->p.x);
			vCenter.y += f2fl (pv->p.y);
			vCenter.z += f2fl (pv->p.z);
			}
		OOF_VecScale (&vCenter, 1.0f / (float) po->nVerts);
		po->vCenter.p.x = fl2f (vCenter.x);
		po->vCenter.p.y = fl2f (vCenter.y);
		po->vCenter.p.z = fl2f (vCenter.z);

		G3GetAdjFaces (po);
		G3GetPolyModelCenters (po);
		}
	po->nState = 2;
	G3DoneInstance ();
	}
if (bShadowData) {
	po->iSubObj = 0;
	G3StartInstanceMatrix (&objP->position.vPos, &objP->position.mOrient);
	POFGetPolyModelItems (modelP, pAnimAngles, po, 0, 1, 0, -1);
	G3DoneInstance ();
	}
return 1;
}

//	-----------------------------------------------------------------------------

int G3DrawPolyModelShadow (tObject *objP, void *modelP, vmsAngVec *pAnimAngles, int nModel)
{
#if SHADOWS
	vmsVector		v, vLightDir;
	short				*pnl;
	int				h, i, j;
	tPOFObject		*po = gameData.models.pofData [gameStates.app.bD1Mission][1] + nModel;

Assert (objP->id < MAX_ROBOT_TYPES);
if (!gameStates.render.bShadowMaps) {
	if (!POFGatherPolyModelItems (objP, modelP, pAnimAngles, po, 1))
		return 0;
	}
OglActiveTexture (GL_TEXTURE0, 0);
glEnable (GL_TEXTURE_2D);
pnl = gameData.render.lights.dynamic.nNearestSegLights + objP->nSegment * MAX_NEAREST_LIGHTS;
gameData.render.shadows.nLight = 0;
if (FAST_SHADOWS) {
	for (i = 0; (gameData.render.shadows.nLight < gameOpts->render.shadows.nLights) && (*pnl >= 0); i++, pnl++) {
		gameData.render.shadows.pLight = gameData.render.lights.dynamic.shader.lights + *pnl;
		if (!gameData.render.shadows.pLight->bState)
			continue;
		if (!CanSeePoint (objP, &objP->position.vPos, &gameData.render.shadows.pLight->vPos, objP->nSegment))
			continue;
		VmVecSub (&vLightDir, &objP->position.vPos, &gameData.render.shadows.pLight->vPos);
		VmVecNormalize (&vLightDir);
		if (gameData.render.shadows.nLight) {
			for (j = 0; j < gameData.render.shadows.nLight; j++)
				if (abs (VmVecDot (&vLightDir, gameData.render.shadows.vLightDir + j)) > 2 * F1_0 / 3) // 60 deg
					break;
			if (j < gameData.render.shadows.nLight)
				continue;
			}
		gameData.render.shadows.vLightDir [gameData.render.shadows.nLight++] = vLightDir;
		if (gameStates.render.bShadowMaps)
			RenderShadowMap (gameData.render.lights.dynamic.lights + (gameData.render.shadows.pLight - gameData.render.lights.dynamic.shader.lights));
		else {
			gameStates.render.bRendering = 1;
			G3TransformPoint (&vLightPos, &gameData.render.shadows.pLight->vPos, 0);
			OOF_VecVms2Oof (&vLightPosf, &vLightPos);
			if (gameOpts->render.shadows.nClip) {
				// get a default clipping distance using the model position as fall back
				G3TransformPoint (&v, &objP->position.vPos, 0);
				fInf = NearestShadowedWallDist (OBJ_IDX (objP), objP->nSegment, &v, 0);
				}
			else
				fInf = G3_INFINITY;
			G3PolyModelVerts2Float (po);
			G3StartInstanceMatrix (&objP->position.vPos, &objP->position.mOrient);
			po->litFaces.nFaces = 0;
			if (gameOpts->render.shadows.nClip >= 2)
				memset (po->pfClipDist, 0, po->nVerts * sizeof (float));
			G3DrawSubModelShadow (objP, po, po->subObjs.pSubObjs);
			G3DoneInstance ();
			gameStates.render.bRendering = 0;
			}
		}
	}
else {
	h = OBJ_IDX (objP);
	j = (int) (gameData.render.shadows.pLight - gameData.render.lights.dynamic.shader.lights);
	pnl = gameData.render.shadows.objLights + h * MAX_SHADOW_LIGHTS;
	for (i = 0; i < gameOpts->render.shadows.nLights; i++, pnl++) {
		if (*pnl < 0)
			break;
		if (*pnl == j) {
			vLightPosf = gameData.render.shadows.vLightPos;
			G3PolyModelVerts2Float (po);
			G3StartInstanceMatrix (&objP->position.vPos, &objP->position.mOrient);
			po->litFaces.nFaces = 0;
			G3DrawSubModelShadow (objP, po, po->subObjs.pSubObjs);
			G3DoneInstance ();
			}
		}
	}
glDisable (GL_TEXTURE_2D);
#endif
return 1;
}

//------------------------------------------------------------------------------

void POFFreeAllPolyModelItems (void)
{
	int	h, i, j;

for (h = 0; h < 2; h++)
	for (i = 0; i < 2; i++)
		for (j = 0; j < MAX_POLYGON_MODELS; j++)
			POFFreePolyModelItems (gameData.models.pofData [h][i] + j);
memset (gameData.models.pofData, 0, sizeof (gameData.models.pofData));
}

//------------------------------------------------------------------------------
//eof
