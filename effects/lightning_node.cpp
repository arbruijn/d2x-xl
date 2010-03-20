#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "descent.h"
#include "perlin.h"
#include "gameseg.h"

#include "objsmoke.h"
#include "transprender.h"
#include "renderthreads.h"
#include "rendermine.h"
#include "error.h"
#include "light.h"
#include "dynlight.h"
#include "ogl_lib.h"
#include "automap.h"
#ifdef TACTILE
#	include "tactile.h"
#endif

//------------------------------------------------------------------------------

bool CLightningNode::CreateChild (CFixVector *vEnd, CFixVector *vDelta,
											 int nLife, int nLength, int nAmplitude,
											 char nAngle, short nNodes, short nChildren, char nDepth, short nSteps,
											 short nSmoothe, char bClamp, char bPlasma, char bLight,
											 char nStyle, tRgbaColorf *colorP, CLightning *parentP, short nNode,
											 int nThread)
{
if (!(m_child = new CLightning))
	return false;
m_child->Init (&m_vPos, vEnd, vDelta, -1, nLife, 0, nLength, nAmplitude, nAngle, 0,
					nNodes, nChildren, nSteps, nSmoothe, bClamp, bPlasma, bLight,
					nStyle, colorP, parentP, nNode);
return m_child->Create (nDepth, nThread);
}

//------------------------------------------------------------------------------

void CLightningNode::Setup (bool bInit, CFixVector *vPos, CFixVector *vDelta)
{
m_vBase =
m_vPos = *vPos;
m_vOffs.SetZero ();
memcpy (m_vDelta, vDelta, sizeof (m_vDelta));
if (bInit)
	m_child = NULL;
else if (m_child)
	m_child->Setup (false);
}

//------------------------------------------------------------------------------

void CLightningNode::Destroy (void)
{
if (m_child) {
	if (int (size_t (m_child)) == int (0xffffffff))
		m_child = NULL;
	else {
		m_child->DestroyNodes ();
		delete m_child;
		m_child = NULL;
		}
	}
}

//------------------------------------------------------------------------------

void CLightningNode::Animate (bool bInit, short nSegment, int nDepth, int nThread)
{
if (bInit)
	m_vPos = m_vNewPos;
else
	m_vPos += m_vOffs;
if (m_child) {
	m_child->Move (nDepth + 1, &m_vPos, nSegment, 0, 0, nThread);
	m_child->Animate (nDepth + 1, nThread);
	}
}

//------------------------------------------------------------------------------

void CLightningNode::ComputeOffset (int nSteps)
{
m_vOffs = m_vNewPos - m_vPos;
m_vOffs *= (I2X (1) / nSteps);
}

//------------------------------------------------------------------------------

int CLightningNode::Clamp (CFixVector *vPos, CFixVector *vBase, int nAmplitude)
{
	CFixVector	vRoot;
	int			nDist = VmPointLineIntersection (vRoot, vBase [0], vBase [1], *vPos, 0);

if (nDist < nAmplitude)
	return nDist;
*vPos -= vRoot;	//create vector from intersection to current path point
*vPos *= FixDiv (nAmplitude, nDist);	//scale down to length nAmplitude
*vPos += vRoot;	//recalculate path point
return nAmplitude;
}

//------------------------------------------------------------------------------

void CLightningNode::Clamp (CFloatVector &v0, CFloatVector &v1, CFloatVector& vBase, float nBaseLen)
{
	CFloatVector	vi, vj, vPos;
	float				di, dj;

vPos.Assign (m_vNewPos);
vPos -= vBase;
VmPointLineIntersection (vi, CFloatVector::ZERO, v1, vPos, 0);
di = vi.Mag ();
vj = v0 * (di / nBaseLen);
vi -= m_vNewPos;
di = vi.Mag ();
vj -= m_vNewPos;
dj = vj.Mag ();
vj *= di / dj;
m_vNewPos [X] += F2X (vj [X]);
m_vNewPos [Y] += F2X (vj [Y]);
m_vNewPos [Z] += F2X (vj [Z]);
}

//------------------------------------------------------------------------------

int CLightningNode::ComputeAttractor (CFixVector *vAttract, CFixVector *vDest, CFixVector *vPos, int nMinDist, int i)
{
	int nDist;

*vAttract = *vDest - *vPos;
nDist = vAttract->Mag () / i;
if (!nMinDist)
	*vAttract *= (I2X (1) / i * 2);	// scale attractor with inverse of remaining distance
else {
	if (nDist < nMinDist)
		nDist = nMinDist;
	*vAttract *= (I2X (1) / i / 2);	// scale attractor with inverse of remaining distance
	}
return nDist;
}

//------------------------------------------------------------------------------

CFixVector *CLightningNode::Create (CFixVector *vOffs, CFixVector *vAttract, int nDist, int nAmplitude)
{
	CFixVector	va = *vAttract;
	int			nDot, nMinDot = I2X (1) / 45 + I2X (1) / 2 - FixDiv (I2X (1) / 2, nAmplitude);

if (nDist < I2X (1) / 16)
	return VmRandomVector (vOffs);
CFixVector::Normalize (va);
do {
	VmRandomVector (vOffs);
	nDot = CFixVector::Dot (va, *vOffs);
	} while (abs (nDot) < nMinDot);
if (nDot < 0)
	vOffs->Neg ();
return vOffs;
}

//------------------------------------------------------------------------------

CFixVector *CLightningNode::Smoothe (CFixVector *vOffs, CFixVector *vPrevOffs, int nDist, int nSmoothe)
{
if (nSmoothe) {
		int nMag = vOffs->Mag ();

	if (nSmoothe > 0)
		*vOffs *= FixDiv (nSmoothe * nDist, nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
	else
		*vOffs *= FixDiv (nDist, nSmoothe * nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
	nMag = vPrevOffs->Mag ();
	*vOffs += *vPrevOffs;
	nMag = vOffs->Mag ();
	*vOffs *= FixDiv(nDist, nMag);
	nMag = vOffs->Mag ();
	}
return vOffs;
}

//------------------------------------------------------------------------------

CFixVector *CLightningNode::Attract (CFixVector *vOffs, CFixVector *vAttract, CFixVector *vPos, int nDist, int i, int bJoinPaths)
{
	int nMag = vOffs->Mag ();
// attract offset vector by scaling it with distance from attracting node
*vOffs *= FixDiv (i * nDist / 2, nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
*vOffs += *vAttract;	//add offset and attractor vectors (attractor is the bigger the closer)
nMag = vOffs->Mag ();
*vOffs *= FixDiv (bJoinPaths ? nDist / 2 : nDist, nMag);	//rescale to desired path length
*vPos += *vOffs;
return vPos;
}

//------------------------------------------------------------------------------

CFixVector CLightningNode::CreateJaggy (CFixVector *vPos, CFixVector *vDest, CFixVector *vBase, CFixVector *vPrevOffs,
												   int nSteps, int nAmplitude, int nMinDist, int i, int nSmoothe, int bClamp)
{
	CFixVector	vAttract, vOffs;
	int			nDist = ComputeAttractor (&vAttract, vDest, vPos, nMinDist, i);

Create (&vOffs, &vAttract, nDist, nAmplitude);
if (!vPrevOffs)
	vPrevOffs = &m_vOffs;
if (!vPrevOffs->IsZero ())
	vOffs = CFixVector::Avg (vOffs, *vPrevOffs);
vOffs *= nDist;
vOffs += vAttract;
CFixVector::Normalize (vOffs);
if (bClamp)
	Clamp (vPos, vBase, nAmplitude);
m_vNewPos = *vPos + vOffs * nDist;
m_vOffs = m_vNewPos - m_vPos;
//m_vOffs *= (I2X (1) / nSteps);
return vOffs;
}

//------------------------------------------------------------------------------

CFixVector CLightningNode::CreateErratic (CFixVector *vPos, CFixVector *vBase, int nSteps, int nAmplitude,
													  int bInPlane, int bFromEnd, int bRandom, int i, int nNodes, int nSmoothe, int bClamp)
{
	int	h, j, nDelta;

m_vNewPos = m_vBase;
for (j = 0; j < 2 - bInPlane; j++) {
	nDelta = nAmplitude / 2 - int (dbl_rand () * nAmplitude);
	if (!bRandom) {
		i -= bFromEnd;
		nDelta *= 3;
#if 0
		nDelta /= (nNodes - i);
		nDelta *= 4;
#endif
		}
	if (bFromEnd) {
		for (h = 1; h <= 2; h++)
			if (i - h > 0)
				nDelta += (h + 1) * m_nDelta [j];
		}
	else {
		for (h = 1; h <= 2; h++)
			if (i - h > 0)
				nDelta += (h + 1) * m_nDelta [j];
		}
	nDelta /= 6;
	m_nDelta [j] = nDelta;
	m_vNewPos += m_vDelta [j] * nDelta;
	}
m_vOffs = m_vNewPos - m_vPos;
m_vOffs *= (I2X (1) / nSteps);
if (bClamp)
	Clamp (vPos, vBase, nAmplitude);
return m_vOffs;
}

//------------------------------------------------------------------------------
// phi is used to modulate the perlin path via a sine function to make it begin
// at the intended start and end points and then have increasing amplitude as 
// moving out from them.

CFixVector CLightningNode::CreatePerlin (int nSteps, int nAmplitude, double phi, int i)
{
	double h = double (i) * 0.03125;

double dx = perlinX.PerlinNoise1D (h, 0.6, 6);
double dy = perlinY.PerlinNoise1D (h, 0.6, 6);
#if 1
static double dx0, dy0;
if (!i) {
	dx0 = dx;
	dy0 = dy;
	}
dx -= dx0;
dy -= dy0;
dx *= nAmplitude;
dy *= nAmplitude;
#else
phi = pow (sin (phi * Pi), 0.3333333) * nAmplitude;
dx *= phi;
dy *= phi;
#endif
m_vNewPos = m_vBase + m_vDelta [0] * int (dx);
m_vNewPos += m_vDelta [1] * int (dy);
m_vOffs = m_vNewPos - m_vPos;
m_vOffs *= (I2X (1) / nSteps);
return m_vOffs;
}

//------------------------------------------------------------------------------

void CLightningNode::Move (int nDepth, CFixVector& vDelta, short nSegment, int nThread)
{
m_vNewPos += vDelta;
m_vBase += vDelta;
m_vPos += vDelta;
if (m_child)
	m_child->Move (nDepth + 1, &m_vPos, nSegment, 0, false, nThread);
}

//------------------------------------------------------------------------------

bool CLightningNode::SetLight (short nSegment, tRgbaColorf *colorP)
{
if (0 > (nSegment = FindSegByPos (m_vPos, nSegment, 0, 0)))
	return false;
#if USE_OPENMP > 1
#	pragma omp critical
#endif
	{
	lightningManager.SetSegmentLight (nSegment, &m_vPos, colorP);
	}
return true;
}

//------------------------------------------------------------------------------
//eof
