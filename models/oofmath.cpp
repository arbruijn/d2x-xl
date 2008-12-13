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

//------------------------------------------------------------------------------

float OOF_Centroid (CFloatVector *pvCentroid, CFloatVector *pvSrc, int nv)
{
	CFloatVector	vNormal, vCenter;
	float			fArea, fTotalArea;
	int			i;

pvCentroid->x =
pvCentroid->y =
pvCentroid->z = 0.0f;

// First figure out the total area of this polygon
fTotalArea = CFloatVector::Perp (vNormal, pvSrc [0], pvSrc [1], pvSrc [2]).Mag () / 2;
for (i = 2; i < nv - 1; i++) {
	fArea = CFloatVector::Perp (vNormal, pvSrc [0], pvSrc [i], pvSrc [i + 1]).Mag () / 2;
	fTotalArea += fArea;
	}
// Now figure out how much weight each triangle represents to the overall polygon
fArea = CFloatVector::Perp (vNormal, pvSrc [0], pvSrc [1], pvSrc [2]).Mag () / 2;
// Get the center of the first polygon
vCenter = pvSrc [0] + pvSrc [1] + pvSrc [2];
pvCentroid += vCenter / (3.0f * (fTotalArea / fArea));
// Now do the same for the rest
for (i = 2; i < nv - 1; i++) {
	fArea = CFloatVector::Perp (vNormal, pvSrc [0], pvSrc [i], pvSrc [i + 1]).Mag () / 2;
	vCenter = pvSrc [0] + pvSrc [i] + pvSrc [i + 1];
	pvCentroid +=  vCenter / (3.0f * (fTotalArea / fArea));
	}
return fTotalArea;
}

//------------------------------------------------------------------------------

void OOF_MatIdentity (CFloatMatrix *pm)
{
memset (pm, 0, sizeof (*pm));
pm->r.x =
pm->u.y =
pm->f.z = 1.0f;
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

float *OOF_VecVms2Gl (float *pDest, const CFixVector& pSrc)
{
pDest [0] = (float) pSrc[X] / 65536.0f;
pDest [1] = (float) pSrc[Y] / 65536.0f;
pDest [2] = (float) pSrc[Z] / 65536.0f;
pDest [3] = 1.0f;
return pDest;
}

//------------------------------------------------------------------------------

float *OOF_MatVms2Gl (float *pDest, const CFixMatrix& pSrc)
{
OOF_GlIdent (pDest);
pDest [0] = X2F (pSrc [RVEC][X]);
pDest [4] = X2F (pSrc [RVEC][Y]);
pDest [8] = X2F (pSrc [RVEC][Z]);
pDest [1] = X2F (pSrc [UVEC][X]);
pDest [5] = X2F (pSrc [UVEC][Y]);
pDest [9] = X2F (pSrc [UVEC][Z]);
pDest [2] = X2F (pSrc [FVEC][X]);
pDest [6] = X2F (pSrc [FVEC][Y]);
pDest [10] = X2F (pSrc [FVEC][Z]);
return pDest;
}

//------------------------------------------------------------------------------

float *OOF_MatVms2Oof (CFloatMatrix *pDest, const CFixMatrix& pSrc)
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

