#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif
//#include  "oof.h
#include "inferno.h"
#include "args.h"
#include "u_mem.h"
#include "error.h"
#include "globvars.h"
#include "light.h"
#include "dynlight.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "network.h"
#include "render.h"
#include "strutil.h"

//------------------------------------------------------------------------------

inline float fsqr (float x)
{
return x * x;
}

float OOF_VecMag (tOOF_vector *pv)
{
return (float) sqrt (fsqr (pv->x) + fsqr (pv->y) + fsqr (pv->z));
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_VecScale (tOOF_vector *pv, float fScale)
{
pv->x *= fScale;
pv->y *= fScale;
pv->z *= fScale;
return pv;
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_VecSub (tOOF_vector *pvDest, tOOF_vector *pvMin, tOOF_vector *pvSub)
{
pvDest->x = pvMin->x - pvSub->x;
pvDest->y = pvMin->y - pvSub->y;
pvDest->z = pvMin->z - pvSub->z;
return pvDest;
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_VecAdd (tOOF_vector *pvDest, tOOF_vector *pvSrc, tOOF_vector *pvAdd)
{
pvDest->x = pvSrc->x + pvAdd->x;
pvDest->y = pvSrc->y + pvAdd->y;
pvDest->z = pvSrc->z + pvAdd->z;
return pvDest;
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_VecNormalize (tOOF_vector *pv)
{
return OOF_VecScale (pv, 1.0f / OOF_VecMag (pv));
}

//------------------------------------------------------------------------------

float OOF_VecMul (tOOF_vector *pvSrc, tOOF_vector *pvMul)
{
return pvSrc->x * pvMul->x + pvSrc->y * pvMul->y + pvSrc->z * pvMul->z;
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_VecInc (tOOF_vector *pvDest, tOOF_vector *pvSrc)
{
pvDest->x += pvSrc->x;
pvDest->y += pvSrc->y;
pvDest->z += pvSrc->z;
return pvDest;
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_VecDec (tOOF_vector *pvDest, tOOF_vector *pvSrc)
{
pvDest->x -= pvSrc->x;
pvDest->y -= pvSrc->y;
pvDest->z -= pvSrc->z;
return pvDest;
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_CrossProd (tOOF_vector *pvDest, tOOF_vector *pu, tOOF_vector *pv)
{
pvDest->x = (pu->y * pv->z) - (pu->z * pv->y);
pvDest->y = (pu->z * pv->x) - (pu->x * pv->z);
pvDest->z = (pu->x * pv->y) - (pu->y * pv->x);
return pvDest;
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_VecPerp (tOOF_vector *pvPerp, tOOF_vector *pv0, tOOF_vector *pv1, tOOF_vector *pv2)
{
	tOOF_vector	u, v;

return OOF_CrossProd (pvPerp, OOF_VecSub (&u, pv1, pv0), OOF_VecSub (&v, pv2, pv1));
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_VecRot (tOOF_vector *pDest, tOOF_vector *pSrc, tOOF_matrix *pRot)
{
pDest->x = OOF_VecDot (pSrc, &pRot->r);
pDest->y = OOF_VecDot (pSrc, &pRot->u);
pDest->z = OOF_VecDot (pSrc, &pRot->f);
return pDest;
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_Normalize (tOOF_vector *pv)
{
float fMag = OOF_VecMag (pv);
if (fMag > 0)
	OOF_VecScale (pv, 1.0f / fMag);
else {
	pv->x = 1.0f;
	pv->y =
	pv->z = 0.0f;
	}
return pv;
}

//------------------------------------------------------------------------------

tOOF_vector *OOF_VecNormal (tOOF_vector *pvNormal, tOOF_vector *pv0, tOOF_vector *pv1, tOOF_vector *pv2)
{
return OOF_Normalize (OOF_VecPerp (pvNormal, pv0, pv1, pv2));
}

//------------------------------------------------------------------------------

float OOF_Centroid (tOOF_vector *pvCentroid, tOOF_vector *pvSrc, int nv)
{
	tOOF_vector	vNormal, vCenter;
	float			fArea, fTotalArea;
	int			i;

pvCentroid->x =
pvCentroid->y =
pvCentroid->z = 0.0f;

// First figure out the total area of this polygon
fTotalArea = OOF_VecMag (OOF_VecPerp (&vNormal, pvSrc, pvSrc + 1, pvSrc + 2)) / 2;
for (i = 2; i < nv - 1; i++) {
	fArea = OOF_VecMag (OOF_VecPerp (&vNormal, pvSrc, pvSrc + i, pvSrc + i + 1)) / 2;
	fTotalArea += fArea;
	}
// Now figure out how much weight each triangle represents to the overall polygon
fArea = OOF_VecMag (OOF_VecPerp (&vNormal, pvSrc, pvSrc + 1, pvSrc + 2)) / 2;
// Get the center of the first polygon
vCenter = *pvSrc;
OOF_VecInc (&vCenter, pvSrc + 1);
OOF_VecInc (&vCenter, pvSrc + 2);
OOF_VecInc (pvCentroid, OOF_VecScale (&vCenter, 1.0f / (3.0f * (fTotalArea / fArea))));
// Now do the same for the rest
for (i = 2; i < nv - 1; i++) {
	fArea = OOF_VecMag (OOF_VecPerp (&vNormal, pvSrc, pvSrc + i, pvSrc + i + 1)) / 2;
	vCenter = *pvSrc;
	OOF_VecInc (&vCenter, pvSrc + i);
	OOF_VecInc (&vCenter, pvSrc + 1);
	OOF_VecInc (pvCentroid, OOF_VecScale (&vCenter, 1.0f / (3.0f * (fTotalArea / fArea))));
	}
return fTotalArea;
}

//------------------------------------------------------------------------------

void OOF_MatIdentity (tOOF_matrix *pm)
{
memset (pm, 0, sizeof (*pm));
pm->r.x =
pm->u.y =
pm->f.z = 1.0f;
}

//------------------------------------------------------------------------------

void OOF_MatMul (tOOF_matrix *pd, tOOF_matrix *ps0, tOOF_matrix *ps1)
{
pd->r.x = OOF_VecDot3 (ps0->r.x, ps0->u.x, ps0->f.x, &ps1->r);
pd->u.x = OOF_VecDot3 (ps0->r.x, ps0->u.x, ps0->f.x, &ps1->u);
pd->f.x = OOF_VecDot3 (ps0->r.x, ps0->u.x, ps0->f.x, &ps1->f);
pd->r.y = OOF_VecDot3 (ps0->r.y, ps0->u.y, ps0->f.y, &ps1->r);
pd->u.y = OOF_VecDot3 (ps0->r.y, ps0->u.y, ps0->f.y, &ps1->u);
pd->f.y = OOF_VecDot3 (ps0->r.y, ps0->u.y, ps0->f.y, &ps1->f);
pd->r.z = OOF_VecDot3 (ps0->r.z, ps0->u.z, ps0->f.z, &ps1->r);
pd->u.z = OOF_VecDot3 (ps0->r.z, ps0->u.z, ps0->f.z, &ps1->u);
pd->f.z = OOF_VecDot3 (ps0->r.z, ps0->u.z, ps0->f.z, &ps1->f);
}

//------------------------------------------------------------------------------

float *OOF_GlIdent (float *pm)
{
memset (pm, 0, sizeof (glMatrixf));
pm [0] = 1.0f;
pm [5] = 1.0f;
pm [10] = 1.0f;
pm [15] = 1.0f;
return pm;
}

//------------------------------------------------------------------------------

float *OOF_VecVms2Gl (float *pDest, const vmsVector& pSrc)
{
pDest [0] = (float) pSrc[X] / 65536.0f;
pDest [1] = (float) pSrc[Y] / 65536.0f;
pDest [2] = (float) pSrc[Z] / 65536.0f;
pDest [3] = 1.0f;
return pDest;
}

//------------------------------------------------------------------------------

float *OOF_MatVms2Gl (float *pDest, const vmsMatrix& pSrc)
{
OOF_GlIdent (pDest);
pDest [0] = X2F (pSrc[RVEC][X]);
pDest [4] = X2F (pSrc[RVEC][Y]);
pDest [8] = X2F (pSrc[RVEC][Z]);
pDest [1] = X2F (pSrc[UVEC][X]);
pDest [5] = X2F (pSrc[UVEC][Y]);
pDest [9] = X2F (pSrc[UVEC][Z]);
pDest [2] = X2F (pSrc[FVEC][X]);
pDest [6] = X2F (pSrc[FVEC][Y]);
pDest [10] = X2F (pSrc[FVEC][Z]);
return pDest;
}

//------------------------------------------------------------------------------

float *OOF_VecVms2Oof (tOOF_vector *pDest, const vmsVector& pSrc)
{
pDest->x = (float) pSrc[X] / 65536.0f;
pDest->y = (float) pSrc[Y] / 65536.0f;
pDest->z = (float) pSrc[Z] / 65536.0f;
return reinterpret_cast<float*> (pDest);
}

//------------------------------------------------------------------------------

float *OOF_MatVms2Oof (tOOF_matrix *pDest, const vmsMatrix& pSrc)
{
OOF_VecVms2Oof (&pDest->f, pSrc[FVEC]);
OOF_VecVms2Oof (&pDest->u, pSrc[UVEC]);
OOF_VecVms2Oof (&pDest->r, pSrc[RVEC]);
return reinterpret_cast<float*> (pDest);
}

//------------------------------------------------------------------------------

float *OOF_GlTranspose (float *pDest, float *pSrc)
{
	glMatrixf	m;
	int			x, y;

if (!pDest)
	pDest = m;
for (y = 0; y < 4; y++)
	for (x = 0; x < 4; x++)
		pDest [x * 4 + y] = pSrc [y * 4 + x];
if (pDest == m) {
	pDest = pSrc;
	memcpy (pDest, &m, sizeof (glMatrixf));
	}
return pDest;
}

//------------------------------------------------------------------------------
//eof

