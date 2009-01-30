#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif
//#include  "oof.h
#include "descent.h"
#include "args.h"
#include "u_mem.h"
#include "error.h"
#include "globvars.h"
#include "light.h"
#include "dynlight.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "network.h"
#include "rendermine.h"
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

pvCentroid->SetZero ();

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
*pvCentroid += vCenter / (3.0f * (fTotalArea / fArea));
// Now do the same for the rest
for (i = 2; i < nv - 1; i++) {
	fArea = CFloatVector::Perp (vNormal, pvSrc [0], pvSrc [i], pvSrc [i + 1]).Mag () / 2;
	vCenter = pvSrc [0] + pvSrc [i] + pvSrc [i + 1];
	*pvCentroid +=  vCenter / (3.0f * (fTotalArea / fArea));
	}
return fTotalArea;
}

//------------------------------------------------------------------------------
//eof

