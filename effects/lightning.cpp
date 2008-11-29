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

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "inferno.h"
#include "perlin.h"
#include "gameseg.h"

#include "objsmoke.h"
#include "transprender.h"
#include "renderthreads.h"
#include "render.h"
#include "error.h"
#include "light.h"
#include "dynlight.h"
#include "ogl_lib.h"
#include "u_mem.h"
#ifdef TACTILE
#	include "tactile.h"
#endif

#ifdef EDITOR
#include "editor/editor.h"
#endif

#ifdef _3DFX
#include "3dfx_des.h"
#endif

#define RENDER_LIGHTNING_PLASMA 1
#define RENDER_LIGHTNING_OUTLINE 0
#define RENDER_LIGHTINGS_BUFFERED 1
#define UPDATE_LIGHTNINGS 1
#define USE_NEW 0

#define LIMIT_FLASH_FPS	1
#define FLASH_SLOWMO 1

#define STYLE	(((m_nStyle < 0) || (gameOpts->render.lightnings.nStyle < m_nStyle)) ? \
					 gameOpts->render.lightnings.nStyle : m_nStyle)

CLightningManager lightningManager;
COmegaLightnings	omegaLightnings;

//------------------------------------------------------------------------------

#define MAX_LIGHTNING_SEGMENTS	10000

typedef struct tPlasmaBuffer {
	tTexCoord2f	texCoord [4 * MAX_LIGHTNING_SEGMENTS];
	fVector		vertices [4 * MAX_LIGHTNING_SEGMENTS];
} tPlasmaBuffer;

static tPlasmaBuffer plasmaBuffers [2][2];
static fVector3 coreBuffer [2][MAX_LIGHTNING_SEGMENTS];

//------------------------------------------------------------------------------

#if DBG

void TRAP (CLightningNode *nodeP)
{
if (nodeP->m_child == (CLightning *) (size_t) 0xfeeefeee)
	nodeP = nodeP;
}

void CHECK (CLightning *lightningP, int i)
{
	CLightningNode *nodeP;
	int j;

for (; i > 0; i--, lightningP++)
	if (lightningP->m_nNodes > 0)
		for (j = lightningP->m_nNodes, nodeP = lightningP->m_nodes; j > 0; j--, nodeP++)
			TRAP (nodeP);
}


#else
#	define TRAP(_pln)
#	define CHECK(_pl,_i)
#endif

//------------------------------------------------------------------------------

inline double dbl_rand (void)
{
return (double) rand () / (double) RAND_MAX;
}

//------------------------------------------------------------------------------

vmsVector *VmRandomVector (vmsVector *vRand)
{
	vmsVector	vr;

do {
	vr [X] = F1_0 / 4 - d_rand ();
	vr [Y] = F1_0 / 4 - d_rand ();
	vr [Z] = F1_0 / 4 - d_rand ();
} while (!(vr [X] && vr [Y] && vr [Z]));
vmsVector::Normalize(vr);
*vRand = vr;
return vRand;
}

//------------------------------------------------------------------------------

#define SIGN(_i)	(((_i) < 0) ? -1 : 1)

#define VECSIGN(_v)	(SIGN ((_v) [X]) * SIGN ((_v) [Y]) * SIGN ((_v) [Z]))

//------------------------------------------------------------------------------

vmsVector *DirectedRandomVector (vmsVector *vRand, vmsVector *vDir, int nMinDot, int nMaxDot)
{
	vmsVector	vr, vd = *vDir, vSign;
	int			nDot, nSign, i = 0;

vmsVector::Normalize(vd);
vSign [X] = vd [X] ? vd [X] / abs(vd [X]) : 0;
vSign [Y] = vd [Y] ? vd [Y] / abs(vd [Y]) : 0;
vSign [Z] = vd [Z] ? vd [Z] / abs(vd [Z]) : 0;
nSign = VECSIGN (vd);
do {
	VmRandomVector (&vr);
	nDot = vmsVector::Dot(vr, vd);
	if (++i == 100)
		i = 0;
	} while ((nDot > nMaxDot) || (nDot < nMinDot));
*vRand = vr;
return vRand;
}

//------------------------------------------------------------------------------

bool CLightningNode::CreateChild (vmsVector *vEnd, vmsVector *vDelta,
											 int nLife, int nLength, int nAmplitude,
											 char nAngle, short nNodes, short nChildren, char nDepth, short nSteps,
											 short nSmoothe, char bClamp, char bPlasma, char bLight,
											 char nStyle, tRgbaColorf *colorP, CLightning *parentP, short nNode)
{
#if USE_NEW
if (!(m_child = new CLightning))
#else
if (!(m_child = (CLightning *) D2_ALLOC (sizeof (CLightning))))
#endif
	return false;
return m_child->Create (&m_vPos, vEnd, vDelta, -1, nLife, 0, nLength, nAmplitude, nAngle, 0,
								nNodes, nChildren, nDepth - 1, nSteps, nSmoothe, bClamp, bPlasma, bLight,
								nStyle, colorP, parentP, nNode);
}

//------------------------------------------------------------------------------

void CLightningNode::Setup (bool bInit, vmsVector *vPos, vmsVector *vDelta)
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
	if ((int) (size_t) m_child == (int) 0xffffffff)
		m_child = NULL;
	else {
		m_child->DestroyNodes ();
#if USE_NEW
		delete m_child;
		m_child = NULL;
#else
		D2_FREE (m_child);
#endif
		}
	}
}

//------------------------------------------------------------------------------

void CLightningNode::Animate (bool bInit, short nSegment, int nDepth)
{
if (bInit)
	m_vPos = m_vNewPos;
#if UPDATE_LIGHTNINGS
else
	m_vPos += m_vOffs;
#endif
if (m_child) {
	m_child->Move (&m_vPos, nSegment, 0, 0);
	m_child->Animate (nDepth + 1);
	}
}

//------------------------------------------------------------------------------

void CLightningNode::ComputeOffset (int nSteps)
{
m_vOffs = m_vNewPos - m_vPos;
m_vOffs *= (F1_0 / nSteps);
}

//------------------------------------------------------------------------------

int CLightningNode::Clamp (vmsVector *vPos, vmsVector *vBase, int nAmplitude)
{
	vmsVector	vRoot;
	int			nDist = VmPointLineIntersection (vRoot, vBase [0], vBase [1], *vPos, 0);

if (nDist < nAmplitude)
	return nDist;
*vPos -= vRoot;	//create vector from intersection to current path point
*vPos *= FixDiv(nAmplitude, nDist);	//scale down to length nAmplitude
*vPos += vRoot;	//recalculate path point
return nAmplitude;
}

//------------------------------------------------------------------------------

int CLightningNode::ComputeAttractor (vmsVector *vAttract, vmsVector *vDest, vmsVector *vPos, int nMinDist, int i)
{
	int nDist;

*vAttract = *vDest - *vPos;
nDist = vAttract->Mag () / i;
if (!nMinDist)
	*vAttract *= (F1_0 / i * 2);	// scale attractor with inverse of remaining distance
else {
	if (nDist < nMinDist)
		nDist = nMinDist;
	*vAttract *= (F1_0 / i / 2);	// scale attractor with inverse of remaining distance
	}
return nDist;
}

//------------------------------------------------------------------------------

vmsVector *CLightningNode::Create (vmsVector *vOffs, vmsVector *vAttract, int nDist)
{
	vmsVector	va = *vAttract;
	int			nDot, i = 0;

if (nDist < F1_0 / 16)
	return VmRandomVector (vOffs);
vmsVector::Normalize(va);
if (!(va [X] && va [Y] && va [Z]))
	i = 0;
do {
	VmRandomVector (vOffs);
	// TODO: Use new vector dot prod method
	// right now it hangs/crashes
	nDot = vmsVector::Dot(va, *vOffs);
	if (++i > 100)
		i = 0;
	} while (abs (nDot) < F1_0 / 32);
if (nDot < 0)
	vOffs->Neg ();
return vOffs;
}

//------------------------------------------------------------------------------

vmsVector *CLightningNode::Smoothe (vmsVector *vOffs, vmsVector *vPrevOffs, int nDist, int nSmoothe)
{
if (nSmoothe) {
		int nMag = vOffs->Mag ();

	if (nSmoothe > 0)
		*vOffs *= FixDiv (nSmoothe * nDist, nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
	else
		*vOffs *= FixDiv (nDist, nSmoothe * nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
	nMag = vPrevOffs->Mag  ();
	*vOffs += *vPrevOffs;
	nMag = vOffs->Mag  ();
	*vOffs *= FixDiv(nDist, nMag);
	nMag = vOffs->Mag  ();
	}
return vOffs;
}

//------------------------------------------------------------------------------

vmsVector *CLightningNode::Attract (vmsVector *vOffs, vmsVector *vAttract, vmsVector *vPos, int nDist, int i, int bJoinPaths)
{
	int nMag = vOffs->Mag ();
// attract offset vector by scaling it with distance from attracting node
*vOffs *= FixDiv(i * nDist / 2, nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
*vOffs += *vAttract;	//add offset and attractor vectors (attractor is the bigger the closer)
nMag = vOffs->Mag ();
*vOffs *= FixDiv(bJoinPaths ? nDist / 2 : nDist, nMag);	//rescale to desired path length
*vPos += *vOffs;
return vPos;
}

//------------------------------------------------------------------------------

vmsVector CLightningNode::CreateJaggy (vmsVector *vPos, vmsVector *vDest, vmsVector *vBase, vmsVector *vPrevOffs,
												   int nSteps, int nAmplitude, int nMinDist, int i, int nSmoothe, int bClamp)
{
	vmsVector	vAttract, vOffs;
	int			nDist = ComputeAttractor (&vAttract, vDest, vPos, nMinDist, i);

Create (&vOffs, &vAttract, nDist);
if (vPrevOffs)
	Smoothe (&vOffs, vPrevOffs, nDist, nSmoothe);
else if (m_vOffs [X] || m_vOffs [Z] || m_vOffs [Z]) {
	vOffs += m_vOffs * (2 * F1_0);
	vOffs /= 3;
	}
if (nDist > F1_0 / 16)
	Attract (&vOffs, &vAttract, vPos, nDist, i, 0);
if (bClamp)
	Clamp (vPos, vBase, nAmplitude);
m_vNewPos = *vPos;
m_vOffs = m_vNewPos - m_vPos;
m_vOffs *= (F1_0 / nSteps);
return vOffs;
}

//------------------------------------------------------------------------------

vmsVector CLightningNode::CreateErratic (vmsVector *vPos, vmsVector *vBase, int nSteps, int nAmplitude,
													  int bInPlane, int bFromEnd, int bRandom, int i, int nNodes, int nSmoothe, int bClamp)
{
	int	h, j, nDelta;

m_vNewPos = m_vBase;
for (j = 0; j < 2 - bInPlane; j++) {
	nDelta = nAmplitude / 2 - (int) (dbl_rand () * nAmplitude);
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
m_vOffs *= (F1_0 / nSteps);
if (bClamp)
	Clamp (vPos, vBase, nAmplitude);
return m_vOffs;
}

//------------------------------------------------------------------------------

vmsVector CLightningNode::CreatePerlin (int nSteps, int nAmplitude, int *nSeed, double phi, double i)
{
double dx = PerlinNoise1D (i, 0.25, 6, nSeed [0]);
double dy = PerlinNoise1D (i, 0.25, 6, nSeed [1]);
phi = sin (phi * Pi);
phi = sqrt (phi);
dx *= nAmplitude * phi;
dy *= nAmplitude * phi;
m_vNewPos = m_vBase + m_vDelta [0] * ((int)dx);
m_vNewPos += m_vDelta [1] * ((int) dy);
m_vOffs = m_vNewPos - m_vPos;
m_vOffs *= (F1_0 / nSteps);
return m_vOffs;
}

//------------------------------------------------------------------------------

void CLightningNode::Move (vmsVector& vDelta, short nSegment)
{
m_vNewPos += vDelta;
m_vBase += vDelta;
m_vPos += vDelta;
if (m_child)
	m_child->Move (&m_vPos, nSegment, 0, false);
}

//------------------------------------------------------------------------------

bool CLightningNode::SetLight (short nSegment, tRgbaColorf *colorP)
{
if (0 > (nSegment = FindSegByPos (m_vPos, nSegment, 0, 0)))
	return false;
lightningManager.SetSegmentLight (nSegment, &m_vPos, colorP);
return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int CLightning::ComputeChildEnd (vmsVector *vPos, vmsVector *vEnd, vmsVector *vDir, vmsVector *vParentDir, int nLength)
{
nLength = 3 * nLength / 4 + (int) (dbl_rand () * nLength / 4);
DirectedRandomVector (vDir, vParentDir, 3 * F1_0 / 4, 9 * F1_0 / 10);
*vEnd = *vPos + *vDir * nLength;
return nLength;
}

//------------------------------------------------------------------------------

void CLightning::Setup (bool bInit)
{
	vmsVector		vPos, vDir, vRefDir, vDelta [2], v;
	int				i = 0, l;

m_vPos = m_vBase;
if (m_bRandom) {
	if (!m_nAngle)
		VmRandomVector (&vDir);
	else {
		int nMinDot = F1_0 - m_nAngle * F1_0 / 90;
		vRefDir = m_vRefEnd - m_vPos;
		vmsVector::Normalize(vRefDir);
		do {
			VmRandomVector (&vDir);
		}
		while (vmsVector::Dot(vRefDir, vDir) < nMinDot);
	}
	m_vEnd = m_vPos + vDir * m_nLength;
}
else {
	vDir = m_vEnd - m_vPos;
	vmsVector::Normalize(vDir);
	}
m_vDir = vDir;
if (m_nOffset) {
	i = m_nOffset / 2 + (int) (dbl_rand () * m_nOffset / 2);
	m_vPos += vDir * i;
	m_vEnd += vDir * i;
	}
vPos = m_vPos;
if (m_bInPlane) {
	vDelta [0] = m_vDelta;
	vDelta [i].SetZero();
	}
else {
	do {
		VmRandomVector(&vDelta [0]);
	} while (abs (vmsVector::Dot (vDir, vDelta [0])) > 9 * F1_0 / 10);
	vDelta [1] = vmsVector::Normal (vPos, m_vEnd, *vDelta);
	v = vPos + vDelta [1];
	vDelta [0] = vmsVector::Normal (vPos, m_vEnd, v);
	}
vDir *= FixDiv (m_nLength, (m_nNodes - 1) * F1_0);
m_nNodes = abs (m_nNodes);
m_iStep = 0;
if (m_parent) {
	i = m_parent->m_nChildren + 1;
	l = m_parent->m_nLength / i;
	m_nLength = ComputeChildEnd (&m_vPos, &m_vEnd, &m_vDir, &m_parent->m_vDir, l + 3 * l / (m_nNode + 1));
	vDir = m_vDir * (m_nLength / (m_nNodes - 1));
	}
for (i = 0; i < m_nNodes; i++) {
	m_nodes [i].Setup (bInit, &vPos, vDelta);
	vPos += vDir;
	}
m_nSteps = -abs (m_nSteps);
}

//------------------------------------------------------------------------------

bool CLightning::Create (vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
								 short nObject, int nLife, int nDelay, int nLength, int nAmplitude,
								 char nAngle, int nOffset, short nNodes, short nChildren, char nDepth, short nSteps,
								 short nSmoothe, char bClamp, char bPlasma, char bLight,
								 char nStyle, tRgbaColorf *colorP, CLightning *parentP, short nNode)
{
	int	h, bRandom = (vEnd == NULL) || (nAngle > 0);

#if USE_NEW
if (!(m_nodes = new CLightningNode [nNodes])) 
#else
if (!(m_nodes = (CLightningNode *) D2_ALLOC (nNodes * sizeof (CLightningNode)))) 
#endif
	return false;
if (nObject < 0) {
	m_nObject = -1;
	m_nSegment = -nObject - 1;
	}
else {
	m_nObject = nObject;
	if (0 > (m_nSegment = OBJECTS [nObject].info.nSegment))
		return NULL;
	}
m_parent = parentP;
m_nNode = nNode;
m_nNodes = nNodes;
m_nChildren = gameOpts->render.lightnings.nQuality ? (nChildren < 0) ? nNodes / 10 : nChildren : 0;
m_nLife = nLife;
h = abs (nLife);
m_nTTL = (bRandom) ? 3 * h / 4 + (int) (dbl_rand () * h / 2) : h;
m_nDelay = abs (nDelay) * 10;
m_nLength = (bRandom) ? 3 * nLength / 4 + (int) (dbl_rand () * nLength / 2) : nLength;
m_nAmplitude = (nAmplitude < 0) ? nLength / 6 : nAmplitude;
m_nAngle = vEnd ? nAngle : 0;
m_nOffset = nOffset;
m_nSteps = -nSteps;
m_nSmoothe = nSmoothe;
m_bClamp = bClamp;
m_bPlasma = bPlasma;
m_iStep = 0;
m_color = *colorP;
m_vBase = *vPos;
m_bLight = bLight;
m_nStyle = nStyle;
m_nNext = -1;
if (vEnd)
	m_vRefEnd = *vEnd;
if (!(m_bRandom = bRandom))
	m_vEnd = *vEnd;
if ((m_bInPlane = (vDelta != NULL)))
	m_vDelta = *vDelta;
Setup (1);
if (gameOpts->render.lightnings.nQuality && nDepth && nChildren) {
	vmsVector	vEnd;
	int			l, n, nNode;
	double		nStep, j;

	n = nChildren + 1 + (nChildren < 2);
	nStep = (double) m_nNodes / (double) nChildren;
	l = nLength / n;
	for (h = m_nNodes - (int) nStep, j = nStep / 2; j < h; j += nStep) {
		nNode = (int) j + 2 - d_rand () % 5;
		if (nNode < 1)
			nNode = (int) j;
		if (!m_nodes [nNode].CreateChild (&vEnd, vDelta, m_nLife, l, nAmplitude / n * 2, nAngle,
													 2 * m_nNodes / n, nChildren / 5, nDepth - 1, nSteps / 2, nSmoothe, bClamp, bPlasma, bLight,
													 nStyle, colorP, this, nNode))
			return false;
		}
	}
return true;
}

//------------------------------------------------------------------------------

void CLightning::DestroyNodes (void)
{
if (m_nodes) {
		CLightningNode	*nodeP;
		int				i;

	for (i = abs (m_nNodes), nodeP = m_nodes; i > 0; i--, nodeP++)
		nodeP->Destroy ();
#if USE_NEW
	delete[] m_nodes;
	 m_nodes = NULL;
#else
	D2_FREE (m_nodes);
#endif
	m_nNodes = 0;
	}
}

//------------------------------------------------------------------------------

void CLightning::Destroy (void)
{
DestroyNodes ();
}

//------------------------------------------------------------------------------

void CLightning::Smoothe (void)
{
	CLightningNode	*plh, *pfi, *pfj;
	int			i, j;

for (i = m_nNodes - 1, j = 0, pfi = m_nodes, plh = NULL; j < i; j++) {
	pfj = plh;
	plh = pfi++;
	if (j) {
		plh->m_vNewPos [X] = pfj->m_vNewPos [X] / 4 + plh->m_vNewPos [X] / 2 + pfi->m_vNewPos [X] / 4;
		plh->m_vNewPos [Y] = pfj->m_vNewPos [Y] / 4 + plh->m_vNewPos [Y] / 2 + pfi->m_vNewPos [Y] / 4;
		plh->m_vNewPos [Z] = pfj->m_vNewPos [Z] / 4 + plh->m_vNewPos [Z] / 2 + pfi->m_vNewPos [Z] / 4;
		}
	}
}

//------------------------------------------------------------------------------

void CLightning::ComputeOffsets (void)
{
if (m_nNodes > 0)
	for (int i = 1, j = m_nNodes - 1 - !m_bRandom; i < j; i++)
		m_nodes [i].ComputeOffset (m_nSteps);
}

//------------------------------------------------------------------------------
// Make sure max. amplitude is reached every once in a while

void CLightning::Bump (void)
{
	CLightningNode	*nodeP;
	int			h, i, nSteps, nDist, nAmplitude, nMaxDist = 0;
	vmsVector	vBase [2];

nSteps = m_nSteps;
nAmplitude = m_nAmplitude;
vBase [0] = m_vPos;
vBase [1] = m_nodes [m_nNodes - 1].m_vPos;
for (i = m_nNodes - 1 - !m_bRandom, nodeP = m_nodes + 1; i > 0; i--, nodeP++) {
	nDist = vmsVector::Dist (nodeP->m_vNewPos, nodeP->m_vPos);
	if (nMaxDist < nDist) {
		nMaxDist = nDist;
		h = i;
		}
	}
if ((h = nAmplitude - nMaxDist)) {
	if (m_nNodes > 0) {
		nMaxDist += (d_rand () % 5) * h / 4;
		for (i = m_nNodes - 1 - !m_bRandom, nodeP = m_nodes + 1; i > 0; i--, nodeP++)
			nodeP->m_vOffs *= FixDiv (nAmplitude, nMaxDist);
		}
	}
}

//------------------------------------------------------------------------------

void CLightning::CreatePath (int bSeed, int nDepth)
{
	CLightningNode	*plh, *nodeP [2];
	int			h, i, j, nSteps, nStyle, nSmoothe, bClamp, bInPlane, nMinDist, nAmplitude, bPrevOffs [2] = {0,0};
	vmsVector	vPos [2], vBase [2], vPrevOffs [2];
	double		phi;

	static int	nSeed [2];

vBase [0] = vPos [0] = m_vPos;
vBase [1] = vPos [1] = m_vEnd;
if (bSeed) {
	nSeed [0] = d_rand ();
	nSeed [1] = d_rand ();
	nSeed [0] *= d_rand ();
	nSeed [1] *= d_rand ();
	}
#if DBG
else
	bSeed = 0;
#endif
nStyle = STYLE;
nSteps = m_nSteps;
nSmoothe = m_nSmoothe;
bClamp = m_bClamp;
bInPlane = m_bInPlane && ((nDepth == 1) || (nStyle == 2));
nAmplitude = m_nAmplitude;
plh = m_nodes;
plh->m_vNewPos = plh->m_vPos;
plh->m_vOffs.SetZero();
if ((nDepth > 1) || m_bRandom) {
	if (nStyle == 2) {
		if (nDepth > 1)
			nAmplitude *= 4;
		for (i = 0; i < m_nNodes; i++) {
			phi = bClamp ? (double) i / (double) (m_nNodes - 1) : 1;
			m_nodes [i].CreatePerlin (nSteps, nAmplitude, nSeed, 2 * phi / 3, phi * 7.5);
			}
		}
	else {
		if (m_bInPlane)
			nStyle = 0;
		nMinDist = m_nLength / (m_nNodes - 1);
		if (!nStyle) {
			nAmplitude *= (nDepth == 1) ? 4 : 16;
			for (h = m_nNodes - 1, i = 0, plh = m_nodes + 1; i < h; i++, plh++) {
				plh->CreateErratic (vPos, vBase, nSteps, nAmplitude, 0, bInPlane, 1, i, h + 1, nSmoothe, bClamp);
				}
			}
		else {
			for (i = m_nNodes - 1, plh = m_nodes + 1; i > 0; i--, plh++, bPrevOffs [0] = 1) {
				*vPrevOffs = plh->CreateJaggy (vPos, vPos + 1, vBase, bPrevOffs [0] ? vPrevOffs : NULL, nSteps, nAmplitude, nMinDist, i, nSmoothe, bClamp);
				TRAP (plh);
				}
			}
		}
	}
else {
	plh = m_nodes + m_nNodes - 1;
	plh->m_vNewPos = plh->m_vPos;
	plh->m_vOffs.SetZero();
	if (nStyle == 2) {
		nAmplitude = 5 * nAmplitude / 3;
		for (h = m_nNodes, i = 0, plh = m_nodes; i < h; i++, plh++) {
			phi = bClamp ? (double) i / (double) (h - 1) : 1;
			plh->CreatePerlin (nSteps, nAmplitude, nSeed, phi, phi * 10);
			}
		}
	else {
		if (m_bInPlane)
			nStyle = 0;
		if (!nStyle) {
			nAmplitude *= 4;
			for (h = m_nNodes - 1, i = j = 0, nodeP [0] = m_nodes + 1, nodeP [1] = m_nodes + h - 1; i < h; i++, j = !j) {
				plh = nodeP [j];
				plh->CreateErratic (vPos + j, vBase, nSteps, nAmplitude, bInPlane, j, 0, i, h, nSmoothe, bClamp);
				if (nodeP [1] <= nodeP [0])
					break;
				if (j)
					nodeP [1]--;
				else
					nodeP [0]++;
				}
			}
		else {
			for (i = m_nNodes - 1, j = 0, nodeP [0] = m_nodes + 1, nodeP [1] = m_nodes + i - 1; i > 0; i--, j = !j) {
				plh = nodeP [j];
				vPrevOffs [j] = plh->CreateJaggy (vPos + j, vPos + !j, vBase, bPrevOffs [j] ? vPrevOffs + j : NULL, nSteps, nAmplitude, 0, i, nSmoothe, bClamp);
				bPrevOffs [j] = 1;
				if (nodeP [1] <= nodeP [0])
					break;
				if (j)
					nodeP [1]--;
				else
					nodeP [0]++;
				}
			}
		}
	}
if (nStyle < 2) {
	Smoothe ();
	ComputeOffsets ();
	Bump ();
	}
}

//------------------------------------------------------------------------------

void CLightning::Animate (int nDepth)
{
	CLightningNode	*nodeP;
	int				j, bSeed;
	bool				bInit;

#if UPDATE_LIGHTNINGS
m_nTTL -= gameStates.app.tick40fps.nTime;
#endif
if (m_nNodes > 0) {
	if ((bInit = (m_nSteps < 0)))
		m_nSteps = -m_nSteps;
	if (!m_iStep) {
		bSeed = 1;
		CreatePath (bSeed, nDepth + 1);
		m_iStep = m_nSteps;
		}
	for (j = m_nNodes - 1 - !m_bRandom, nodeP = m_nodes + 1; j > 0; j--, nodeP++)
		nodeP->Animate (bInit, m_nSegment, nDepth);
#if UPDATE_LIGHTNINGS
	(m_iStep)--;
#endif
	}
}

//------------------------------------------------------------------------------

int CLightning::SetLife (void)
{
	int	h;

if (m_nTTL <= 0) {
	if (m_nLife < 0) {
		if ((m_nDelay > 1) && (0 > (m_nNodes = -m_nNodes))) {
			h = m_nDelay / 2;
			m_nTTL = h + (int) (dbl_rand () * h);
			}
		else {
			if (m_bRandom) {
				h = -m_nLife;
				m_nTTL = 3 * h / 4 + (int) (dbl_rand () * h / 2);
				Setup (0);
				}
			else {
				m_nTTL = -m_nLife;
				m_nNodes = abs (m_nNodes);
				Setup (0);
				}
			}
		}
	else {
		DestroyNodes ();
		return 0;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int CLightning::Update (int nDepth)
{
Animate (nDepth);
return SetLife ();
}

//------------------------------------------------------------------------------

void CLightning::Move (vmsVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd)
{
if (nSegment < 0)
	return;

	CLightningNode	*nodeP;
	vmsVector		vDelta, vOffs;
	int				h, j;
	double			fStep;

m_nSegment = nSegment;
if (bFromEnd)
	vDelta = *vNewPos - m_vEnd;
else
	vDelta = *vNewPos - m_vPos;
if (vDelta.IsZero ())
	return;
if (bStretch) {
	vOffs = vDelta;
	if (bFromEnd)
		m_vEnd += vDelta;
	else
		m_vPos += vDelta;
	}
else if (bFromEnd) {
	m_vPos += vDelta;
	m_vEnd = *vNewPos;
	}
else {
	m_vEnd += vDelta;
	m_vPos = *vNewPos;
	}
if (bStretch) {
	m_vDir = m_vEnd - m_vPos;
	m_nLength = m_vDir.Mag();
	}
if (0 < (h = m_nNodes)) {
	for (j = h, nodeP = m_nodes; j > 0; j--, nodeP++) {
		if (bStretch) {
			vDelta = vOffs;
			if (bFromEnd)
				fStep = (double) (h - j + 1) / (double) h;
			else
				fStep = (double) j / (double) h;
			vDelta [X] = (int) (vOffs [X] * fStep + 0.5);
			vDelta [Y] = (int) (vOffs [Y] * fStep + 0.5);
			vDelta [Z] = (int) (vOffs [Z] * fStep + 0.5);
			}
		nodeP->Move (vDelta, nSegment);
		}
	}
}

//------------------------------------------------------------------------------

inline int CLightning::MayBeVisible (void)
{
if (m_nSegment >= 0)
	return SegmentMayBeVisible (m_nSegment, m_nLength / 20, 3 * m_nLength / 2);
if (m_nObject >= 0)
	return (gameData.render.mine.bObjectRendered [m_nObject] == gameStates.render.nFrameFlipFlop);
return 1;
}

//------------------------------------------------------------------------------

#define LIGHTNING_VERT_ARRAYS 1

static tTexCoord2f plasmaTexCoord [3][4] = {
	{{{0,0.45f}},{{1,0.45f}},{{1,0.55f}},{{0,0.55f}}},
	{{{0,0.15f}},{{1,0.15f}},{{1,0.5f}},{{0,0.5f}}},
	{{{0,0.5f}},{{1,0.5f}},{{1,0.85f}},{{0,0.85f}}}
	};

//------------------------------------------------------------------------------

void CLightning::ComputePlasmaSegment (fVector *vPosf, int bScale, short nSegment, char bStart, char bEnd, int nDepth, int nThread)
{
	fVector			*vPlasma = plasmaBuffers [nThread][bScale].vertices + 4 * nSegment;
	fVector			vn [2], vd;

	static fVector vEye = fVector::ZERO;
	static fVector vNormal [3] = {fVector::ZERO,fVector::ZERO,fVector::ZERO};

memcpy (vNormal, vNormal + 1, 2 * sizeof (fVector));
if (bStart) {
	vNormal [1] = fVector::Normal(vPosf [0], vPosf [1], vEye);
	vn [0] = vNormal [1];
	}
else {
	vn [0] = vNormal [0] + vNormal [1];
	vn [0] = vn [0] * 0.5f;
}

if (bEnd) {
	vn [1] = vNormal [1];
	vd = vPosf [1] - vPosf [0];
	vd = vd * (bScale ? 0.25f : 0.5f);
	vPosf [1] += vd;
	}
else {
	vNormal [2] = fVector::Normal (vPosf [1], vPosf [2], vEye);
	if (fVector::Dot (vNormal [1], vNormal [2]) < 0)
		vNormal [2].Neg ();
	vn [1] = vNormal [1] + vNormal [2];
	vn [1] = vn [1] * 0.5f;
	}
if (!(nDepth || bScale)) {
	vn [0] = vn [0] * 2;
	vn [1] = vn [1] * 2;
	}
if (!bScale && nDepth) {
	vn [0] = vn [0] * 0.5f;
	vn [1] = vn [1] * 0.5f;
	}
if (bStart) {
	vPlasma [0] = vPosf [0] + vn [0];
	vPlasma [1] = vPosf [0] - vn [0];
	vd = vPosf [0] - vPosf [1];
	fVector::Normalize(vd);
	if (bScale)
		vd = vd * 0.5f;
	vPlasma [0] += vd;
	vPlasma [1] += vd;
	}
else {
	vPlasma [0] = vPlasma [-1];
	vPlasma [1] = vPlasma [-2];
	}
vPlasma [3] = vPosf [1] + vn [1];
vPlasma [2] = vPosf [1] - vn [1];
memcpy (plasmaBuffers [nThread][bScale].texCoord + 4 * nSegment, 
		  plasmaTexCoord [bStart + 2 * bEnd], 4 * sizeof (tTexCoord2f));
}

//------------------------------------------------------------------------------

void CLightning::ComputePlasma (int nDepth, int nThread)
{
	CLightningNode	*nodeP;
	fVector			vPosf [3] = {fVector::ZERO,fVector::ZERO,fVector::ZERO};
	int				bScale, i, j;

for (bScale = 0; bScale < 2; bScale++) {
	nodeP = m_nodes;
	vPosf [2] = (nodeP++)->m_vPos.ToFloat();
	if (!gameStates.ogl.bUseTransform)
		G3TransformPoint (vPosf [2], vPosf [2], 0);
	for (i = m_nNodes - 2, j = 0, nodeP = m_nodes; j <= i; j++) {
		memcpy (vPosf, vPosf + 1, 2 * sizeof (fVector));
		vPosf [2] = (++nodeP)->m_vPos.ToFloat ();
		if (!gameStates.ogl.bUseTransform)
			G3TransformPoint (vPosf [2], vPosf [2], 0);
		ComputePlasmaSegment (vPosf, bScale, j, j == 1, j == i, nDepth, nThread);
		}
	}
}

//------------------------------------------------------------------------------

void CLightning::RenderPlasma (tRgbaColorf *colorP, int nThread)
{
	int				bScale;
#if RENDER_LIGHTNING_OUTLINE
	tTexCoord2f		*texCoordP;
	fVector			*vertexP;
	int				i, j;
#endif

if (!G3EnableClientStates (1, 0, 0, GL_TEXTURE0))
	return;
glBlendFunc (GL_ONE, GL_ONE);
for (bScale = 0; bScale < 2; bScale++) {
	if (bScale)
		glColor4f (0.1f, 0.1f, 0.1f, colorP->alpha / 2);
	else
		glColor4f (colorP->red / 4, colorP->green / 4, colorP->blue / 4, colorP->alpha);
	glTexCoordPointer (2, GL_FLOAT, 0, plasmaBuffers [nThread][bScale].texCoord);
	glVertexPointer (3, GL_FLOAT, sizeof (fVector), plasmaBuffers [nThread][bScale].vertices);
	glDrawArrays (GL_QUADS, 0, 4 * (m_nNodes - 1));
#if RENDER_LIGHTNING_OUTLINE
	glDisable (GL_TEXTURE_2D);
	glColor3f (1,1,1);
	texCoordP = plasmaBuffers [nThread][bScale].texCoord;
	vertexP = plasmaBuffers [nThread][bScale].vertices;
	for (i = nNodes - 1; i; i--) {
		glBegin (GL_LINE_LOOP);
		for (j = 0; j < 4; j++) {
			glTexCoord2fv ((GLfloat *) texCoordP++);
			glVertex3fv ((GLfloat *) vertexP++);
			}
		glEnd ();
		}
#endif
	}
G3DisableClientStates (1, 0, 0, GL_TEXTURE0);
}

//------------------------------------------------------------------------------

void CLightning::RenderCore (tRgbaColorf *colorP, int nDepth, int nThread)
{
	fVector3		*vPosf = coreBuffer [nThread];
	int			i;

glBlendFunc (GL_ONE, GL_ONE);
glColor4f (colorP->red / 4, colorP->green / 4, colorP->blue / 4, colorP->alpha);
glLineWidth ((GLfloat) (nDepth ? 2 : 4));
glEnable (GL_LINE_SMOOTH);
for (i = 0; i < m_nNodes; i++)
	vPosf [i] = m_nodes [i].m_vPos.ToFloat3 ();
if (!gameStates.ogl.bUseTransform)
	OglSetupTransform (1);
#if 1
if (G3EnableClientStates (0, 0, 0, GL_TEXTURE0)) {
	glDisable (GL_TEXTURE_2D);
	glVertexPointer (3, GL_FLOAT, 0, coreBuffer [nThread]);
	glDrawArrays (GL_LINE_STRIP, 0, m_nNodes);
	G3DisableClientStates (0, 0, 0, -1);
	}
else {
	glActiveTexture (GL_TEXTURE0);
	glDisable (GL_TEXTURE_2D);
	glBegin (GL_LINE_STRIP);
	for (i = m_nNodes, vPosf = coreBuffer [nThread]; i; i--, vPosf++)
		glVertex3fv ((GLfloat *) vPosf);
	glEnd ();
	}
#endif
if (!gameStates.ogl.bUseTransform)
	OglResetTransform (1);
glLineWidth ((GLfloat) 1);
glDisable (GL_LINE_SMOOTH);
OglClearError (0);
}

//------------------------------------------------------------------------------

int CLightning::SetupPlasma (void)
{
if (!(gameOpts->render.lightnings.bPlasma && m_bPlasma && G3EnableClientStates (1, 0, 0, GL_TEXTURE0)))
	return 0;
glActiveTexture (GL_TEXTURE0);
glClientActiveTexture (GL_TEXTURE0);
glEnable (GL_TEXTURE_2D);
if (LoadCorona () && !OglBindBmTex (bmpCorona, 1, -1)) {
	OglTexWrap (bmpCorona->glTexture, GL_CLAMP);
	return 1;
	}
G3DisableClientStates (1, 0, 0, GL_TEXTURE0);
return 0;
}

//------------------------------------------------------------------------------

void CLightning::RenderBuffered (int nDepth, int nThread)
{
	int				i, bPlasma;
	tRgbaColorf		color;

if ((m_nNodes <= 0) || (m_nSteps < 0))
	return;
if (gameStates.app.bMultiThreaded)
	tiRender.ti [nThread].bBlock = 1;
color = m_color;
if (m_nLife > 0) {
	if ((i = m_nLife - m_nTTL) < 250)
		color.alpha *= (float) i / 250.0f;
	else if (m_nTTL < m_nLife / 3)
		color.alpha *= (float) m_nTTL / (float) (m_nLife / 3);
	}
color.red *= (float) (0.9 + dbl_rand () / 5);
color.green *= (float) (0.9 + dbl_rand () / 5);
color.blue *= (float) (0.9 + dbl_rand () / 5);
if (!(bPlasma = SetupPlasma ()))
	color.alpha *= 1.5f;
if (nDepth)
	color.alpha /= 2;
if (gameStates.app.bMultiThreaded && nThread) {	//thread 1 will always render after thread 0
	tiRender.ti [1].bBlock = 0;
	while (tiRender.ti [0].bBlock)
		G3_SLEEP (0);
	}
if (bPlasma) {
	ComputePlasma (nDepth, nThread);
	RenderPlasma (&color, nThread);
	}
RenderCore (&color, nDepth, nThread);
if (gameStates.app.bMultiThreaded && !nThread) { //thread 0 will wait for thread 1 to complete its rendering
	tiRender.ti [0].bBlock = 0;
	while (tiRender.ti [1].bBlock)
		G3_SLEEP (0);
	}
if (gameOpts->render.lightnings.nQuality)
		for (i = 0; i < m_nNodes; i++)
			if (m_nodes [i].GetChild ())
				m_nodes [i].GetChild ()->RenderBuffered (nDepth + 1, nThread);
OglClearError (0);
}

//------------------------------------------------------------------------------

void CLightning::Render (int nDepth, int bDepthSort, int nThread)
{
	int				i;

if (bDepthSort > 0) {
	if ((m_nNodes < 0) || (m_nSteps < 0))
		return;
	if (!MayBeVisible ())
		return;
	TIAddLightning (this, nDepth);
	if (gameOpts->render.lightnings.nQuality)
		for (i = 0; i < m_nNodes; i++)
			if (m_nodes [i].GetChild ())
				m_nodes [i].GetChild ()->Render (nDepth + 1, 1, nThread);
	}
else {
	if (!nDepth) 
		glDisable (GL_CULL_FACE);
	RenderBuffered (0, nThread);
	if (!nDepth) 
		glEnable (GL_CULL_FACE);
	glLineWidth (1);
	glDisable (GL_LINE_SMOOTH);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

//------------------------------------------------------------------------------

int CLightning::SetLight (void)
{
	int				j, nLights = 0, nStride;
	double			h, nStep;

if (!m_bLight)
	return 0;
if (0 < (j = m_nNodes)) {
	if (!(nStride = (int) ((double) m_nLength / (F1_0 * 20) + 0.5)))
		nStride = 1;
	if (!(nStep = (double) (j - 1) / (double) nStride))
		nStep = (double) ((int) (j + 0.5));
#if DBG
	if (m_nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	for (h = nStep / 2; h < j; h += nStep) {
		if (!m_nodes [(int) h].SetLight (m_nSegment, &m_color))
			break;
		nLights++;
		}
	}
return nLights;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool CLightningSystem::Create (int nId, int nLightnings, vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
										 short nObject, int nLife, int nDelay, int nLength, int nAmplitude,
										 char nAngle, int nOffset, short nNodes, short nChildren, char nDepth, short nSteps,
										 short nSmoothe, char bClamp, char bPlasma, char bSound, char bLight,
										 char nStyle, tRgbaColorf *colorP)
{
m_nId = nId;
m_nObject = nObject;
if (!(nLife && nLength && (nNodes > 4)))
	return false;
m_nLightnings = nLightnings;
m_bForcefield = !nDelay && (vEnd || (nAngle <= 0));
#if USE_NEW
if (!(m_lightnings = new CLightning [nLightnings]))
#else
if (!(m_lightnings = (CLightning *) D2_ALLOC (nLightnings * sizeof (CLightning))))
#endif
	return false;
for (int i = 0; i < nLightnings; i++)
	if (!m_lightnings [i].Create (vPos, vEnd, vDelta, nObject, nLife, nDelay, nLength, nAmplitude, 
											nAngle, nOffset, nNodes, nChildren, 0, nSteps,
											nSmoothe, bClamp, bPlasma, bLight, nStyle, colorP, false, -1))
	return false;
CreateSound (bSound);
m_nKey [0] =
m_nKey [1] = 0;
m_bDestroy = 0;
m_tUpdate = -1;
return true;
}

//------------------------------------------------------------------------------

void CLightningSystem::Destroy (void)
{
DestroySound ();
if (m_lightnings) {
	for (int i = 0; i < m_nLightnings; i++)
		m_lightnings [i].Destroy ();
#if USE_NEW
	delete[] m_lightnings;
	 m_lightnings = NULL;
#else
	D2_FREE (m_lightnings);
#endif
	m_nLightnings = 0;
	}
if ((m_nObject >= 0) && (lightningManager.GetObjectSystem (m_nObject) == m_nId)) {
	lightningManager.SetObjectSystem (m_nObject, -1);
	m_nObject = -1;
	}
}

//------------------------------------------------------------------------------

void CLightningSystem::CreateSound (int bSound)
{
if ((m_bSound = bSound)) {
	DigiSetObjectSound (m_nObject, -1, AddonSoundName (SND_ADDON_LIGHTNING));
	if (m_bForcefield) {
		if (0 <= (m_nSound = DigiGetSoundByName ("ff_amb_1")))
			DigiSetObjectSound (m_nObject, m_nSound, NULL);
		}
	}
else
	m_nSound = -1;
}

//------------------------------------------------------------------------------

void CLightningSystem::DestroySound (void)
{
if ((m_bSound > 0) & (m_nObject >= 0))
	DigiKillSoundLinkedToObject (m_nObject);
}

//------------------------------------------------------------------------------

void CLightningSystem::Animate (int nStart, int nLightnings)
{
	CLightning *lightningP = m_lightnings + nStart;

if (nLightnings < 0)
	nLightnings = m_nLightnings;
for (; nLightnings; nLightnings--, lightningP++)
	lightningP->Animate (0);
}

//------------------------------------------------------------------------------

int CLightningSystem::SetLife (void)
{
	CLightning	*lightningP = m_lightnings;
	int			i;

for (i = 0; i < m_nLightnings; ) {
	if (!lightningP->SetLife ()) {
		lightningP->DestroyNodes ();
		if (!--m_nLightnings)
			return 0;
		if (i < m_nLightnings) {
			*lightningP = m_lightnings [m_nLightnings];
			continue;
			}
		}
	i++, lightningP++;
	}
return m_nLightnings;
}

//------------------------------------------------------------------------------

int CLightningSystem::Update (bool bDamage)
{
if (m_bDestroy) {
	Destroy ();
	return -1;
	}
if (gameStates.app.nSDLTicks - m_tUpdate >= 25) {
	if (bDamage || !(m_nKey [0] || m_nKey [1])) {
		m_tUpdate = gameStates.app.nSDLTicks;
		Animate (0, m_nLightnings);
		if (!(m_nLightnings = SetLife ()))
			lightningManager.Destroy (m_nId, NULL, true);
		else if (m_nObject >= 0) {
			UpdateSound ();
			MoveForObject ();
			}
		}
	}
return m_nLightnings;
}

//------------------------------------------------------------------------------

void CLightningSystem::UpdateSound (void)
{
	CLightning	*lightningP;
	int			i;

if (!m_bSound)
	return;
for (i = m_nLightnings, lightningP = m_lightnings; i > 0; i--, lightningP++)
	if (lightningP->m_nNodes > 0) {
		if (m_bSound < 0)
			CreateSound (1);
		return;
		}
if (m_bSound < 0)
	return;
DestroySound ();
m_bSound = -1;
}

//------------------------------------------------------------------------------

void CLightningSystem::Move (vmsVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd)
{
if (nSegment < 0)
	return;
if (SHOW_LIGHTNINGS) {
	for (int i = 0; i < m_nLightnings; i++)
		m_lightnings [i].Move (vNewPos, nSegment, bStretch, bFromEnd);
	}
}

//------------------------------------------------------------------------------

void CLightningSystem::MoveForObject (void)
{
	tObject *objP = OBJECTS + m_nObject;

Move (&OBJPOS (objP)->vPos, objP->info.nSegment, 0, 0);
}

//------------------------------------------------------------------------------

void CLightningSystem::Render (int nStart, int nLightnings, int bDepthSort, int nThread)
{
	CLightning *lightningP = m_lightnings + nStart;

if (nLightnings < 0)
	nLightnings = m_nLightnings;
for (; nLightnings; nLightnings--, lightningP++)
	lightningP->Render (0, bDepthSort, nThread);
}

//------------------------------------------------------------------------------

int CLightningSystem::SetLight (void)
{
	int nLights = 0;

if (m_lightnings)
	for (int i = 0; i < m_nLightnings; i++)
		nLights += m_lightnings [i].SetLight ();
return nLights;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CLightningManager::CLightningManager () 
{
m_objects = NULL;
m_lights = NULL; 
}

//------------------------------------------------------------------------------

CLightningManager::~CLightningManager () 
{ 
DestroyAll (true);
#if USE_NEW
delete[] m_objects;
m_objects = NULL;
delete[] m_lights;
m_lights = NULL;
#else
D2_FREE (m_objects);
D2_FREE (m_lights); 
#endif
}

//------------------------------------------------------------------------------

void CLightningManager::Init (void)
{
	int i, j;

if (!m_objects)
#if USE_NEW
	m_objects = new short [MAX_OBJECTS];
#else
	GETMEM (short, m_objects, MAX_OBJECTS, (char) 0xff);
#endif
if (!m_lights)
#if USE_NEW
	m_lights = new tLightningLight [2*MAX_SEGMENTS];
#else
	GETMEM (tLightningLight, m_lights, MAX_SEGMENTS, (char) 0xff);
#endif
for (i = 0, j = 1; j < MAX_LIGHTNINGS; i++, j++)
	m_systems [i].SetNext (j);
m_systems [i].SetNext (-1);
m_nFree = 0;
m_nUsed = -1;
m_bDestroy = 0;
memset (m_objects, 0xff, MAX_OBJECTS * sizeof (*m_objects));
}

//------------------------------------------------------------------------------

int CLightningManager::Create (int nLightnings, vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
										 short nObject, int nLife, int nDelay, int nLength, int nAmplitude,
										 char nAngle, int nOffset, short nNodes, short nChildren, char nDepth, short nSteps,
										 short nSmoothe, char bClamp, char bPlasma, char bSound, char bLight,
										 char nStyle, tRgbaColorf *colorP)
{
if (!(SHOW_LIGHTNINGS && colorP))
	return -1;
if (!nLightnings)
	return -1;
if (m_nFree < 0)
	return -1;
SEM_ENTER (SEM_LIGHTNINGS)
srand (gameStates.app.nSDLTicks);
int i = m_nFree;
CLightningSystem* systemP = m_systems + i;
if (!(systemP->Create (i, nLightnings, vPos, vEnd, vDelta, nObject, nLife, nDelay, nLength, nAmplitude,
							  nAngle, nOffset, nNodes, nChildren, nDepth, nSteps, nSmoothe, bClamp, bPlasma, bSound, bLight,
							  nStyle, colorP))) {
	systemP->Destroy ();
	SEM_LEAVE (SEM_LIGHTNINGS)
	return -1;
	}
m_nFree = systemP->GetNext ();
systemP->SetNext (m_nUsed);
m_nUsed = i;
SEM_LEAVE (SEM_LIGHTNINGS)
return m_nUsed;
}

//------------------------------------------------------------------------------

int CLightningManager::IsUsed (int iSystem)
{
	int	i, j;

if (iSystem < 0)
	return -1;
for (i = m_nUsed, j = -1; i >= 0; j = i, i = m_systems [i].GetNext ())
	if (iSystem == i)
		return j + 1;
return -1;
}

//------------------------------------------------------------------------------

CLightningSystem* CLightningManager::PrevSystem (int iSystem)
{
	int	i, j;

if (iSystem < 0)
	return NULL;
for (i = m_nUsed; i >= 0; i = j)
	if ((j = m_systems [i].GetNext ()) == iSystem)
		return m_systems + i;
return NULL;
}

//------------------------------------------------------------------------------

void CLightningManager::Destroy (int iSystem, CLightning *lightningP, bool bDestroy)
{
	CLightningSystem	*systemP = NULL;
	int					i, j;

if (lightningP)
	lightningP->Destroy ();
else {
	if (0 > (j = IsUsed (iSystem)))
		return;
	systemP = m_systems + iSystem;
	if (!bDestroy) {
		systemP->m_bDestroy = 1;
		return;
		}
	i = systemP->GetNext ();
	if (m_nUsed == iSystem)
		m_nUsed = i;
	systemP->SetNext (m_nFree);
	if (j > 0)
		m_systems [j - 1].SetNext (i);
	m_nFree = iSystem;
	systemP->Destroy ();
	}
}

//------------------------------------------------------------------------------

int CLightningManager::DestroyAll (bool bForce)
{
	int	i, j;

if (!bForce && (m_bDestroy >= 0))
	m_bDestroy = 1;
else {
	uint bSem = gameData.app.semaphores [SEM_LIGHTNINGS];
	if (!bSem)
		SEM_ENTER (SEM_LIGHTNINGS)
	for (i = m_nUsed; i >= 0; i = j) {
		j = m_systems [i].GetNext ();
		Destroy (i, NULL, true);
		}
	ResetLights (1);
	Init ();
	if (!bSem)
		SEM_LEAVE (SEM_LIGHTNINGS)
	}
return 1;
}

//------------------------------------------------------------------------------

void CLightningManager::Move (int i, vmsVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd)
{
if (nSegment < 0)
	return;
if (SHOW_LIGHTNINGS && (IsUsed (i) >= 0))
	m_systems [i].Move (vNewPos, nSegment, bStretch, bFromEnd);
}

//------------------------------------------------------------------------------

void CLightningManager::MoveForObject (tObject *objP)
{
Move (m_objects [OBJ_IDX (objP)], &OBJPOS (objP)->vPos, objP->info.nSegment, 0, 0);
}

//------------------------------------------------------------------------------

tRgbaColorf *CLightningManager::LightningColor (tObject *objP)
{
if (objP->info.nType == OBJ_ROBOT) {
	if (ROBOTINFO (objP->info.nId).energyDrain) {
		static tRgbaColorf color = {1.0f, 0.8f, 0.3f, 0.2f};
		return &color;
		}
	}
else if ((objP->info.nType == OBJ_PLAYER) && gameOpts->render.lightnings.bPlayers) {
	int s = SEGMENT2S [objP->info.nSegment].special;
	if (s == SEGMENT_IS_FUELCEN) {
		static tRgbaColorf color = {1.0f, 0.8f, 0.3f, 0.2f};
		return &color;
		}
	else if (s == SEGMENT_IS_REPAIRCEN) {
		static tRgbaColorf color = {0.3f, 0.5f, 0.1f, 0.2f};
		return &color;
		}
	}
return NULL;
}

//------------------------------------------------------------------------------

void CLightningManager::Update (void)
{
if (SHOW_LIGHTNINGS) {

		tObject	*objP;
		ubyte		h;
		int		i, n;

#if LIMIT_LIGHTNING_FPS
#	if LIGHTNING_SLOWMO
		static int	t0 = 0;
		int t = gameStates.app.nSDLTicks - t0;

	if (t / gameStates.gameplay.slowmo [0].fSpeed < 25)
		return;
	t0 = gameStates.app.nSDLTicks + 25 - (int) (gameStates.gameplay.slowmo [0].fSpeed * 25);
#	else
	if (!gameStates.app.tick40fps.bTick)
		return 0;
#	endif
#endif
	SEM_ENTER (SEM_LIGHTNINGS)
	for (i = 0, objP = OBJECTS; i < gameData.objs.nLastObject [1]; i++, objP++) {
		if (gameData.objs.bWantEffect [i] & DESTROY_LIGHTNINGS) {
			gameData.objs.bWantEffect [i] &= ~DESTROY_LIGHTNINGS;
			DestroyForObject (objP);
			}
		}
	for (i = m_nUsed; i >= 0; i = n) {
		n = m_systems [i].GetNext ();
		if (0 > m_systems [i].Update ())
			Destroy (i, NULL, true);
		}

	SEM_LEAVE (SEM_LIGHTNINGS)

	FORALL_OBJS (objP, i) {
		i = OBJ_IDX (objP);
		h = gameData.objs.bWantEffect [i];
		if (h & EXPL_LIGHTNINGS) {
			if ((objP->info.nType == OBJ_ROBOT) || (objP->info.nType == OBJ_REACTOR))
				CreateForBlowup (objP);
			else if (objP->info.nType != 255)
				PrintLog ("invalid effect requested\n");
			}
		else if (h & MISSILE_LIGHTNINGS) {
			if ((objP->info.nType == OBJ_WEAPON) || gameData.objs.bIsMissile [objP->info.nId])
				CreateForMissile (objP);
			else if (objP->info.nType != 255)
				PrintLog ("invalid effect requested\n");
			}
		else if (h & ROBOT_LIGHTNINGS) {
			if (objP->info.nType == OBJ_ROBOT)
				CreateForRobot (objP, LightningColor (objP));
			else if (objP->info.nType != 255)
				PrintLog ("invalid effect requested\n");
			}
		else if (h & PLAYER_LIGHTNINGS) {
			if (objP->info.nType == OBJ_PLAYER)
				CreateForPlayer (objP, LightningColor (objP));
			else if (objP->info.nType != 255)
				PrintLog ("invalid effect requested\n");
			}
		gameData.objs.bWantEffect [i] &= ~(PLAYER_LIGHTNINGS | ROBOT_LIGHTNINGS | MISSILE_LIGHTNINGS | EXPL_LIGHTNINGS);
		}
	}
}

//------------------------------------------------------------------------------
#if 0
void MoveForObject (tObject *objP)
{
SEM_ENTER (SEM_LIGHTNINGS)
MoveForObjectInternal (objP);
SEM_LEAVE (SEM_LIGHTNINGS)
}
#endif
//------------------------------------------------------------------------------

void CLightningManager::DestroyForObject (tObject *objP)
{
	int i = OBJ_IDX (objP);

if (m_objects [i] >= 0) {
	Destroy (m_objects [i], NULL, 0);
	m_objects [i] = -1;
	}
}

//------------------------------------------------------------------------------

void CLightningManager::DestroyForAllObjects (int nType, int nId)
{
	tObject	*objP;
	int		i;

FORALL_OBJS (objP, i) {
	if ((objP->info.nType == nType) && ((nId < 0) || (objP->info.nId == nId)))
#if 1
		RequestEffects (objP, DESTROY_LIGHTNINGS);
#else
		DestroyObjecCLightnings (objP);
#endif
	}
}

//------------------------------------------------------------------------------

void CLightningManager::DestroyForPlayers (void)
{
DestroyForAllObjects (OBJ_PLAYER, -1);
}

//------------------------------------------------------------------------------

void CLightningManager::DestroyForRobots (void)
{
DestroyForAllObjects (OBJ_ROBOT, -1);
}

//------------------------------------------------------------------------------

void CLightningManager::DestroyStatic (void)
{
DestroyForAllObjects (OBJ_EFFECT, LIGHTNING_ID);
}

//------------------------------------------------------------------------------

void CLightningManager::Render (void)
{
if (SHOW_LIGHTNINGS) {
		CLightningSystem	*systemP;
		int					i, n, bStencil = StencilOff ();

	for (i = m_nUsed; i >= 0; i = n) {
		systemP = m_systems + i;
		n = systemP->GetNext ();
		if (!(systemP->m_nKey [0] | systemP->m_nKey [1]))
			systemP->Render (0, systemP->m_nLightnings, gameOpts->render.bDepthSort > 0, 0);
		}
	StencilOn (bStencil);
	}
}

//------------------------------------------------------------------------------

vmsVector *CLightningManager::FindTargetPos (tObject *emitterP, short nTarget)
{
	int				i;
	tObject			*objP;

if (!nTarget)
	return 0;
FORALL_EFFECT_OBJS (objP, i) {
	if ((objP != emitterP) && (objP->info.nId == LIGHTNING_ID) && (objP->rType.lightningInfo.nId == nTarget))
		return &objP->info.position.vPos;
	}
return NULL;
}

//------------------------------------------------------------------------------

void CLightningManager::StaticFrame (void)
{
	int				h, i;
	tObject			*objP;
	vmsVector		*vEnd, *vDelta, v;
	tLightningInfo	*pli;
	tRgbaColorf		color;

if (!SHOW_LIGHTNINGS)
	return;
if (!gameOpts->render.lightnings.bStatic)
	return;
FORALL_EFFECT_OBJS (objP, i) {
	if (objP->info.nId != LIGHTNING_ID)
		continue;
	i = OBJ_IDX (objP);
	if (m_objects [i] >= 0)
		continue;
	pli = &objP->rType.lightningInfo;
	if (pli->nLightnings <= 0)
		continue;
	if (pli->bRandom && !pli->nAngle)
		vEnd = NULL;
	else if ((vEnd = FindTargetPos (objP, pli->nTarget)))
		pli->nLength = vmsVector::Dist (objP->info.position.vPos, *vEnd) / F1_0;
	else {
		v = objP->info.position.vPos + objP->info.position.mOrient [FVEC] * F1_0 * pli->nLength;
		vEnd = &v;
		}
	color.red = (float) pli->color.red / 255.0f;
	color.green = (float) pli->color.green / 255.0f;
	color.blue = (float) pli->color.blue / 255.0f;
	color.alpha = (float) pli->color.alpha / 255.0f;
	vDelta = pli->bInPlane ? &objP->info.position.mOrient [RVEC] : NULL;
	h = Create (pli->nLightnings, &objP->info.position.vPos, vEnd, vDelta, i, -abs (pli->nLife), pli->nDelay, pli->nLength * F1_0,
				   pli->nAmplitude * F1_0, pli->nAngle, pli->nOffset * F1_0, pli->nNodes, pli->nChildren, pli->nChildren > 0, pli->nSteps,
				   pli->nSmoothe, pli->bClamp, pli->bPlasma, pli->bSound, 1, pli->nStyle, &color);
	if (h >= 0)
		m_objects [i] = h;
	}
}

//------------------------------------------------------------------------------

void CLightningManager::DoFrame (void)
{
if (m_bDestroy) {
	m_bDestroy = -1;
	DestroyAll (0);
	}
else {
	Update ();
	omegaLightnings.Update (NULL, NULL);
	StaticFrame ();
	}
}

//------------------------------------------------------------------------------

void CLightningManager::SetSegmentLight (short nSegment, vmsVector *vPosP, tRgbaColorf *colorP)
{
if ((nSegment < 0) || (nSegment >= gameData.segs.nSegments))
	return;
else {
		tLightningLight	*llP = m_lights + nSegment;

#if DBG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	if (llP->nFrame != gameData.app.nFrameCount) {
		memset (llP, 0, sizeof (*llP));
		llP->nFrame = gameData.app.nFrameCount;
		llP->nSegment = nSegment;
		llP->nNext = m_nFirstLight;
		m_nFirstLight = nSegment;
		}
	llP->nLights++;
	llP->vPos += *vPosP;
	llP->color.red += colorP->red;
	llP->color.green += colorP->green;
	llP->color.blue += colorP->blue;
	llP->color.alpha += colorP->alpha;
	}
}

//------------------------------------------------------------------------------

void CLightningManager::ResetLights (int bForce)
{
if (SHOW_LIGHTNINGS || bForce) {
		tLightningLight	*llP;
		int					i;

	for (i = m_nFirstLight; i >= 0; ) {
		if ((i < 0) || (i >= MAX_SEGMENTS))
			continue;
		llP = m_lights + i;
		i = llP->nNext;
		llP->nLights = 0;
		llP->nNext = -1;
		llP->vPos.SetZero ();
		llP->color.red =
		llP->color.green =
		llP->color.blue = 0;
		llP->nBrightness = 0;
		if (llP->nDynLight >= 0) {
			llP->nDynLight = -1;
			}
		}
	m_nFirstLight = -1;
	DeleteLightningLights ();
	}
}

//------------------------------------------------------------------------------

void CLightningManager::SetLights (void)
{
	int nLights = 0;

ResetLights (0);
if (SHOW_LIGHTNINGS) {
		tLightningLight	*llP = NULL;
		int					i, n, bDynLighting = gameOpts->render.nLightingMethod;

	m_nFirstLight = -1;
	for (i = m_nUsed; i >= 0; i = m_systems [i].GetNext ())
		nLights += m_systems [i].SetLight ();
	if (!nLights)
		return;
	nLights = 0;
	for (i = m_nFirstLight; i >= 0; i = llP->nNext) {
		if ((i < 0) || (i >= MAX_SEGMENTS))
			continue;
		llP = m_lights + i;
#if DBG
		if (llP->nSegment == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		n = llP->nLights;
		llP->vPos [X] /= n;
		llP->vPos [Y] /= n;
		llP->vPos [Z] /= n;
		llP->color.red /= n;
		llP->color.green /= n;
		llP->color.blue /= n;

		if (gameStates.render.bPerPixelLighting == 2)
			llP->nBrightness = F2X (sqrt ((llP->color.red * 3 + llP->color.green * 5 + llP->color.blue * 2) * llP->color.alpha));
		else
			llP->nBrightness = F2X ((llP->color.red * 3 + llP->color.green * 5 + llP->color.blue * 2) * llP->color.alpha);
		if (bDynLighting) {
			llP->nDynLight = AddDynLight (NULL, &llP->color, llP->nBrightness, llP->nSegment, -1, -1, -1, &llP->vPos);
			nLights++;
			}
		}
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForExplosion (tObject *objP, tRgbaColorf *colorP, int nRods, int nRad, int nTTL)
{
if (SHOW_LIGHTNINGS && gameOpts->render.lightnings.bExplosions) {
	//m_objects [OBJ_IDX (objP)] =
		Create (
			nRods, &objP->info.position.vPos, NULL, NULL, OBJ_IDX (objP), nTTL, 0,
			nRad, F1_0 * 4, 0, 2 * F1_0, 50, 5, 1, 3, 1, 1, 0, 0, 1, -1, colorP);
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForShaker (tObject *objP)
{
static tRgbaColorf color = {0.1f, 0.1f, 0.8f, 0.2f};

CreateForExplosion (objP, &color, 30, 20 * F1_0, 750);
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForShakerMega (tObject *objP)
{
static tRgbaColorf color = {0.1f, 0.1f, 0.6f, 0.2f};

CreateForExplosion (objP, &color, 20, 15 * F1_0, 750);
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForMega (tObject *objP)
{
static tRgbaColorf color = {0.8f, 0.1f, 0.1f, 0.2f};

CreateForExplosion (objP, &color, 30, 15 * F1_0, 750);
}

//------------------------------------------------------------------------------

int CLightningManager::CreateForMissile (tObject *objP)
{
if (gameData.objs.bIsMissile [objP->info.nId]) {
	if ((objP->info.nId == EARTHSHAKER_ID) || (objP->info.nId == EARTHSHAKER_ID))
		CreateForShaker (objP);
	else if ((objP->info.nId == EARTHSHAKER_MEGA_ID) || (objP->info.nId == ROBOT_SHAKER_MEGA_ID))
		CreateForShakerMega (objP);
	else if ((objP->info.nId == MEGAMSL_ID) || (objP->info.nId == ROBOT_MEGAMSL_ID))
		CreateForMega (objP);
	else
		return 0;
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForBlowup (tObject *objP)
{
static tRgbaColorf color = {0.1f, 0.1f, 0.8f, 0.2f};

int h = X2I (objP->info.xSize) * 2;

CreateForExplosion (objP, &color, h + rand () % h, h * (F1_0 + F1_0 / 2), 500);
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForRobot (tObject *objP, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNINGS && gameOpts->render.lightnings.bRobots && OBJECT_EXISTS (objP)) {
		int h, i = OBJ_IDX (objP);

	if (0 <= m_objects [i])
		MoveForObject (objP);
	else {
		h = Create (2 * objP->info.xSize / F1_0, &objP->info.position.vPos, NULL, NULL, OBJ_IDX (objP), -1000, 100,
						objP->info.xSize, objP->info.xSize / 8, 0, 0, 25, 3, 1, 3, 1, 1, 0, 0, 1, 0, colorP);
		if (h >= 0)
			m_objects [i] = h;
		}
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForPlayer (tObject *objP, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNINGS && gameOpts->render.lightnings.bPlayers && OBJECT_EXISTS (objP)) {
	int h, i = OBJ_IDX (objP);

	if (0 <= m_objects [i])
		MoveForObject (objP);
	else {
		h = Create (4 * objP->info.xSize / F1_0, &objP->info.position.vPos, NULL, NULL, OBJ_IDX (objP), -5000, 1000,
						4 * objP->info.xSize, objP->info.xSize, 0, 2 * objP->info.xSize, 50, 5, 1, 5, 1, 1, 0, 1, 1, 1, colorP);
		if (h >= 0)
			m_objects [i] = h;
		}
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForDamage (tObject *objP, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNINGS && gameOpts->render.lightnings.bDamage && OBJECT_EXISTS (objP)) {
		int h, n, i = OBJ_IDX (objP);

	n = X2IR (RobotDefaultShields (objP));
	h = X2IR (objP->info.xShields) * 100 / n;
	if ((h < 0) || (h >= 50))
		return;
	n = (5 - h / 10) * 2;
	if (0 <= (h = m_objects [i])) {
		if (m_systems [h].m_nLightnings == n) {
			MoveForObject (objP);
			return;
			}
		Destroy (h, NULL, 0);
		}
	h = Create (n, &objP->info.position.vPos, NULL, NULL, OBJ_IDX (objP), -1000, 4000,
					objP->info.xSize, objP->info.xSize / 8, 0, 0, 20, 0, 1, 10, 1, 1, 0, 0, 0, -1, colorP);
	if (h >= 0)
		m_objects [i] = h;
	}
}

//------------------------------------------------------------------------------

int CLightningManager::FindDamageLightning (short nObject, int *pKey)
{
		CLightningSystem	*systemP;
		int					i;

for (i = m_nUsed; i >= 0; i = systemP->m_nNext) {
	systemP = m_systems + i;
	if ((systemP->m_nObject == nObject) && (systemP->m_nKey [0] == pKey [0]) && (systemP->m_nKey [1] == pKey [1]))
		return i;
	}
return -1;
}

//------------------------------------------------------------------------------

typedef union tPolyKey {
	int	i [2];
	short	s [4];
} tPolyKey;

void CLightningManager::RenderForDamage (tObject *objP, g3sPoint **pointList, tG3ModelVertex *pVerts, int nVertices)
{
	CLightningSystem	*systemP;
	fVector				v, vPosf, vEndf, vNormf, vDeltaf;
	vmsVector			vPos, vEnd, vNorm, vDelta;
	int					h, i, j, bUpdate = 0;
	short					nObject;
	tPolyKey				key;

	static short	nLastObject = -1;
	static float	fDamage;
	static int		nFrameFlipFlop = -1;

	static tRgbaColorf color = {0.2f, 0.2f, 1.0f, 1.0f};

if (!(SHOW_LIGHTNINGS && gameOpts->render.lightnings.bDamage))
	return;
if ((objP->info.nType != OBJ_ROBOT) && (objP->info.nType != OBJ_PLAYER))
	return;
if (nVertices < 3)
	return;
j = (nVertices > 4) ? 4 : nVertices;
h = (nVertices + 1) / 2;
if (pointList) {
	for (i = 0; i < j; i++)
		key.s [i] = pointList [i]->p3_key;
	for (; i < 4; i++)
		key.s [i] = 0;
	}
else {
	for (i = 0; i < j; i++)
		key.s [i] = pVerts [i].nIndex;
	for (; i < 4; i++)
		key.s [i] = 0;
	}
i = FindDamageLightning (nObject = OBJ_IDX (objP), key.i);
if (i < 0) {
	if ((nLastObject != nObject) || (nFrameFlipFlop != gameStates.render.nFrameFlipFlop)) {
		nLastObject = nObject;
		nFrameFlipFlop = gameStates.render.nFrameFlipFlop;
		fDamage = (0.5f - ObjectDamage (objP)) / 250.0f;
		}
#if 1
	if (dbl_rand () > fDamage)
		return;
#endif
	if (pointList) {
		vPos = pointList [0]->p3_src;
		vEnd = pointList [1 + d_rand () % (nVertices - 1)]->p3_vec;
		vNorm = vmsVector::Normal(vPos, pointList [1]->p3_vec, vEnd);
		vPos += vNorm * (F1_0 / 64);
		vEnd += vNorm * (F1_0 / 64);
		vDelta = vmsVector::Normal (vNorm, vPos, vEnd);
		h = vmsVector::Dist (vPos, vEnd);
		}
	else {
		memcpy (&vPosf, &pVerts->vertex, sizeof (fVector3));
		memcpy (&vEndf, &pVerts [1 + d_rand () % (nVertices - 1)].vertex, sizeof (fVector3));
		memcpy (&v, &pVerts [1].vertex, sizeof (fVector3));
		vNormf = fVector::Normal (vPosf, v, vEndf);
		vPosf += vNormf * (1.0f / 64.0f);
		vEndf += vNormf * (1.0f / 64.0f);
		vDeltaf = fVector::Normal (vNormf, vPosf, vEndf);
		h = F2X (fVector::Dist (vPosf, vEndf));
		vPos = vPosf.ToFix ();
		vEnd = vEndf.ToFix ();
		}
	i = Create (1, &vPos, &vEnd, NULL /*&vDelta*/, nObject, 1000 + d_rand () % 2000, 0,
					h, h / 4 + d_rand () % 2, 0, 0, 20, 2, 1, 5, 0, 1, 0, 0, 0, 1, &color);
	bUpdate = 1;
	}
if (i >= 0) {
	systemP = m_systems + i;
	glDisable (GL_CULL_FACE);
	if (bUpdate) {
		systemP->m_nKey [0] = key.i [0];
		systemP->m_nKey [1] = key.i [1];
		}
	if (systemP->Lightnings () && (systemP->m_nLightnings = systemP->Lightnings ()->Update (0)))
		systemP->Render (0, -1, 0, -1);
	else
		Destroy (i, NULL, 1);
	}
}

// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------

int COmegaLightnings::Find (short nObject)
{
	int	i;

for (i = 0; i < m_nHandles; i++)
	if (m_handles [i].nParentObj == nObject)
		return i;
return -1;
}

// ---------------------------------------------------------------------------------

void COmegaLightnings::Delete (short nHandle)
{
if (m_nHandles) {
	lightningManager.Destroy (m_handles [nHandle].nLightning, NULL, 0);
	if (nHandle < --m_nHandles)
		m_handles [nHandle] = m_handles [m_nHandles];
	memset (m_handles + m_nHandles, 0xff, sizeof (tOmegaLightningHandles));
	}
}

// ---------------------------------------------------------------------------------

void COmegaLightnings::Destroy (short nObject)
{
	int	nHandle;

if (nObject < 0) {
	for (nHandle = m_nHandles; nHandle; )
		Delete (--nHandle);
	}
else {
	if (0 <= (nHandle = Find (nObject)))
		Delete (nHandle);
	}
}

// ---------------------------------------------------------------------------------

vmsVector *COmegaLightnings::GetGunPoint (tObject *objP, vmsVector *vMuzzle)
{
	vmsVector			*vGunPoints;
	int					bSpectate;
	tTransformation	*posP;

if (!objP)
	return NULL;
bSpectate = SPECTATOR (objP);
posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
if ((bSpectate || (objP->info.nId != gameData.multiplayer.nLocalPlayer)) &&
	 (vGunPoints = GetGunPoints (objP, 6))) {
	TransformGunPoint (objP, vGunPoints, 6, 0, 0, vMuzzle, NULL);
	}
else {
	*vMuzzle = posP->vPos - posP->mOrient [UVEC];
	*vMuzzle += posP->mOrient [FVEC] * (objP->info.xSize / 3);
	}
return vMuzzle;
}

// ---------------------------------------------------------------------------------

int COmegaLightnings::Update (tObject *parentObjP, tObject *targetObjP)
{
	vmsVector					vMuzzle;
	tOmegaLightningHandles	*handleP;
	tWeaponState				*wsP;
	int							i, j, nHandle, nLightning;

if (!(SHOW_LIGHTNINGS && gameOpts->render.lightnings.bOmega))
	return -1;
if (m_nHandles < 1)
	return 0;
if ((gameData.omega.xCharge [IsMultiGame] >= MAX_OMEGA_CHARGE) &&
	 (0 <= (nHandle = Find (LOCALPLAYER.nObject))))
	Destroy (nHandle);
short nObject = parentObjP ? OBJ_IDX (parentObjP) : -1;
if (nObject < 0) {
	i = 0;
	j = m_nHandles;
	}
else {
	i = Find (OBJ_IDX (parentObjP));
	if (i < 0)
		return 0;
	j = 1;
	m_handles [i].nTargetObj = targetObjP ? OBJ_IDX (targetObjP) : -1;
	}
for (handleP = m_handles + i; j; j--) {
	if ((nLightning = handleP->nLightning) >= 0) {
		parentObjP = OBJECTS + handleP->nParentObj;
		if (parentObjP->info.nType == OBJ_PLAYER) {
			wsP = gameData.multiplayer.weaponStates + parentObjP->info.nId;
			if ((wsP->nPrimary != OMEGA_INDEX) || !wsP->firing [0].nStart) {
				Delete (handleP - m_handles);
				continue;
				}
			}
		targetObjP = (handleP->nTargetObj >= 0) ? OBJECTS + handleP->nTargetObj : NULL;
		GetGunPoint (parentObjP, &vMuzzle);
		lightningManager.Move (nLightning, &vMuzzle,
									  SPECTATOR (parentObjP) ? gameStates.app.nPlayerSegment : parentObjP->info.nSegment, true, false);
		if (targetObjP)
			lightningManager.Move (nLightning, &targetObjP->info.position.vPos, targetObjP->info.nSegment, true, true);
		}
	handleP++;
	}
return 1;
}

// ---------------------------------------------------------------------------------

#define OMEGA_PLASMA 0

int COmegaLightnings::Create (vmsVector *vTargetPos, tObject *parentObjP, tObject *targetObjP)
{
	tOmegaLightningHandles	*handleP;
	int							nObject;

if (!(SHOW_LIGHTNINGS && gameOpts->render.lightnings.bOmega))
	return 0;
if ((parentObjP->info.nType == OBJ_ROBOT) && !gameOpts->render.lightnings.bRobotOmega)
	return 0;
nObject = OBJ_IDX (parentObjP);
if (Update (parentObjP, targetObjP)) {
	if (!(handleP = m_handles + Find (nObject)))
		return 0;
	}
else {
	static tRgbaColorf	color = {0.9f, 0.6f, 0.6f, 0.3f};
	vmsVector	vMuzzle, *vTarget;

	Destroy (nObject);
	GetGunPoint (parentObjP, &vMuzzle);
	handleP = m_handles + m_nHandles++;
	handleP->nParentObj = nObject;
	handleP->nTargetObj = targetObjP ? OBJ_IDX (targetObjP) : -1;
	vTarget = targetObjP ? &targetObjP->info.position.vPos : vTargetPos;
#if OMEGA_PLASMA
	color.alpha = gameOpts->render.lightnings.bPlasma ? 0.5f : 0.3f;
#endif
	handleP->nLightning = 
		lightningManager.Create (10, &vMuzzle, vTarget, NULL, -OBJSEG (parentObjP) - 1,
										 -5000, 0, vmsVector::Dist(vMuzzle, *vTarget), F1_0 * 3, 0, 0, 100, 10, 1, 3, 1, 1,
#if OMEGA_PLASMA
										 (parentObjP != gameData.objs.viewerP) || gameStates.app.bFreeCam || gameStates.render.bExternalView,
#else
										 0,
#endif
										 1, 1, -1, &color);
	}
return (handleP->nLightning >= 0);
}

//------------------------------------------------------------------------------
//eof
