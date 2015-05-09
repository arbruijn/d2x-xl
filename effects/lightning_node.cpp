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
#include "PerlinNoise.h"
#include "SimplexNoise.h"
#include "segmath.h"

#include "objsmoke.h"
#include "transprender.h"
#include "renderthreads.h"
#include "rendermine.h"
#include "error.h"
#include "light.h"
#include "dynlight.h"
#include "ogl_lib.h"
#include "automap.h"

#if NOISE_TYPE
CSimplexNoise noiseX [MAX_THREADS], noiseY [MAX_THREADS];
#else
CPerlinNoise noiseX [MAX_THREADS], noiseY [MAX_THREADS];
#endif

//------------------------------------------------------------------------------

bool CLightningNode::CreateChild (CFixVector *vEnd, CFixVector *vDelta,
											 int32_t nLife, int32_t nLength, int32_t nAmplitude,
											 char nAngle, int16_t nNodes, int16_t nChildren, char nDepth, int16_t nSteps,
											 int16_t nSmoothe, char bClamp, char bGlow, char bLight,
											 char nStyle, float nWidth, CFloatVector *pColor, CLightning *pParent, int16_t nNode,
											 int32_t nThread)
{
if (!(m_child = new CLightning))
	return false;
m_child->Init (&m_vPos, vEnd, vDelta, -1, nLife, 0, nLength, nAmplitude, nAngle, 0,
					nNodes, nChildren, nSteps, nSmoothe, bClamp, bGlow, bLight,
					nStyle, nWidth, pColor, pParent, nNode);
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
	if (int32_t (size_t (m_child)) == int32_t (0xffffffff))
		m_child = NULL;
	else {
		m_child->DestroyNodes ();
		delete m_child;
		m_child = NULL;
		}
	}
}

//------------------------------------------------------------------------------

void CLightningNode::Animate (bool bInit, int16_t nSegment, int32_t nDepth, int32_t nThread)
{
if (bInit)
	m_vPos = m_vNewPos;
else
	m_vPos += m_vOffs;
if (m_child) {
	m_child->Move (m_vPos, nSegment);
	m_child->Animate (nDepth + 1, nThread);
	}
}

//------------------------------------------------------------------------------

void CLightningNode::ComputeOffset (int32_t nSteps)
{
m_vOffs = m_vNewPos - m_vPos;
m_vOffs *= (I2X (1) / nSteps);
}

//------------------------------------------------------------------------------

int32_t CLightningNode::Clamp (CFixVector *vPos, CFixVector *vBase, int32_t nAmplitude)
{
	CFixVector	vRoot;
	int32_t		nDist = FindPointLineIntersection (vRoot, vBase [0], vBase [1], *vPos, 0);

if (nDist < nAmplitude)
	return nDist;
*vPos -= vRoot;	//create vector from intersection to current path point
*vPos *= FixDiv (nAmplitude, nDist);	//scale down to length nAmplitude
*vPos += vRoot;	//recalculate path point
return nAmplitude;
}

//------------------------------------------------------------------------------

void CLightningNode::Rotate (CFloatVector &v0, float len0, CFloatVector &v1, float len1, CFloatVector& vBase, int32_t nSteps)
{
	CFloatVector	vi, vj, vPos;

vPos.Assign (m_vNewPos);
vPos -= vBase;
FindPointLineIntersection (vi, CFloatVector::ZERO, v1, vPos, 0);
vj = v0 * (vi.Mag () / len1 * len0);
vi -= vPos;
vj -= vi;
m_vNewPos.Assign (vj + vBase);
m_vOffs = m_vNewPos - m_vPos;
m_vOffs *= (I2X (1) / nSteps);
}

//------------------------------------------------------------------------------

void CLightningNode::Scale (CFloatVector vStart, CFloatVector vEnd, float scale, int32_t nSteps)
{
	CFloatVector	vPos, vi;

vPos.Assign (m_vNewPos);
FindPointLineIntersection (vi, vStart, vEnd, vPos, 0);
vPos -= vi;
vPos = vi + vPos * scale;
m_vNewPos.Assign (vPos);
m_vOffs = m_vNewPos - m_vPos;
m_vOffs *= (I2X (1) / nSteps);
}

//------------------------------------------------------------------------------

int32_t CLightningNode::ComputeAttractor (CFixVector *vAttract, CFixVector *vDest, CFixVector *vPos, int32_t nMinDist, int32_t i)
{
	int32_t nDist;

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

CFixVector *CLightningNode::Create (CFixVector *vOffs, CFixVector *vAttract, int32_t nDist, int32_t nAmplitude)
{
	CFixVector	va = *vAttract;
	int32_t		i, nDot, nMinDot = I2X (1) / 45 + I2X (1) / 2 - FixDiv (I2X (1) / 2, nAmplitude);

if (nDist < I2X (1) / 16)
	return VmRandomVector (vOffs);
CFixVector::Normalize (va);
i = 0;
do {
	VmRandomVector (vOffs);
	nDot = CFixVector::Dot (va, *vOffs);
	} while ((abs (nDot) < nMinDot) && (++i < 5));
if (nDot < 0)
	vOffs->Neg ();
return vOffs;
}

//------------------------------------------------------------------------------

CFixVector *CLightningNode::Smoothe (CFixVector *vOffs, CFixVector *vPrevOffs, int32_t nDist, int32_t nSmoothe)
{
if (nSmoothe) {
		int32_t nMag = vOffs->Mag ();

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

CFixVector *CLightningNode::Attract (CFixVector *vOffs, CFixVector *vAttract, CFixVector *vPos, int32_t nDist, int32_t i, int32_t bJoinPaths)
{
	int32_t nMag = vOffs->Mag ();
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
												   int32_t nSteps, int32_t nAmplitude, int32_t nMinDist, int32_t i, int32_t nSmoothe, int32_t bClamp)
{
	CFixVector	vAttract, vOffs;
	int32_t			nDist = ComputeAttractor (&vAttract, vDest, vPos, nMinDist, i);

Create (&vOffs, &vAttract, nDist, nAmplitude);
if (vPrevOffs)
	Smoothe (&vOffs, vPrevOffs, nDist, nSmoothe);
else if (m_vOffs.v.coord.x || m_vOffs.v.coord.z || m_vOffs.v.coord.z) {
	vOffs += m_vOffs * (I2X (2));
	vOffs /= 3;
	}
if (nDist > I2X (1) / 16)
	Attract (&vOffs, &vAttract, vPos, nDist, i, 0);
if (bClamp)
	Clamp (vPos, vBase, nAmplitude);
m_vNewPos = *vPos;
m_vOffs = *vPos - m_vPos;
m_vOffs *= (I2X (1) / nSteps);
return vOffs;
}

//------------------------------------------------------------------------------

CFixVector CLightningNode::CreateErratic (CFixVector *vPos, CFixVector *vBase, int32_t nSteps, int32_t nAmplitude,
													  int32_t bInPlane, int32_t bFromEnd, int32_t bRandom, int32_t i, int32_t nNodes, int32_t nSmoothe, int32_t bClamp)
{
	int32_t	h, j, nDelta;

m_vNewPos = m_vBase;
for (j = 0; j < 2 - bInPlane; j++) {
	nDelta = nAmplitude / 2 - int32_t (RandDouble () * nAmplitude);
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

void CLightningNode::CreatePerlin (double l, double i, int32_t nThread)
{
double dx = ((l - i) * noiseX [nThread].ComputeNoise (i / l) + i * noiseX [nThread].ComputeNoise ((i - l) / l)) / l;
double dy = ((l - i) * noiseY [nThread].ComputeNoise (i / l) + i * noiseY [nThread].ComputeNoise ((i - l) / l)) / l;

static double dx0 [MAX_THREADS], dy0 [MAX_THREADS];

if (!i) {
	dx0 [nThread] = dx;
	dy0 [nThread] = dy;
	}
dx -= dx0 [nThread];
dy -= dy0 [nThread];
dx *= I2X (1);
dy *= I2X (1);
//dx *= nAmplitude;
//dy *= nAmplitude;
m_vNewPos = m_vBase + m_vDelta [0] * int32_t (dx);
m_vNewPos += m_vDelta [1] * int32_t (dy);
}

//------------------------------------------------------------------------------

void CLightningNode::Move (const CFixVector& vOffset, int16_t nSegment)
{
m_vNewPos += vOffset;
m_vBase += vOffset;
m_vPos += vOffset;
if (m_child)
	m_child->Move (m_vPos, nSegment);
}

//------------------------------------------------------------------------------

void CLightningNode::Move (const CFixVector& vOldPos, const CFixVector& vOldEnd, 
									const CFixVector& vNewPos, const CFixVector& vNewEnd, 
									float fScale, int16_t nSegment)
{
	CFixVector	vi, vj, vo;

FindPointLineIntersection (vi, vOldPos, vOldEnd, m_vPos, 0);
float fOffset = X2F (CFixVector::Dist (vi, vOldPos)) * fScale;
vo = vNewEnd - vNewPos;
fOffset /= X2F (vo.Mag ());
vo.v.coord.x = fix (vo.v.coord.x * fOffset);
vo.v.coord.y = fix (vo.v.coord.y * fOffset);
vo.v.coord.z = fix (vo.v.coord.z * fOffset);
vj = vNewPos + vo;
Move (vj - vi, nSegment);
}

//------------------------------------------------------------------------------

bool CLightningNode::SetLight (int16_t nSegment, CFloatVector *pColor)
{
if (0 > (nSegment = FindSegByPos (m_vPos, nSegment, 0, 0)))
	return false;
#if USE_OPENMP //> 1
#	pragma omp critical
#endif
	{
	lightningManager.SetSegmentLight (nSegment, &m_vPos, pColor);
	}
return true;
}

//------------------------------------------------------------------------------
//eof
