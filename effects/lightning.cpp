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

#define RENDER_LIGHTNING_PLASMA 1
#define RENDER_LIGHTNING_OUTLINE 0
#define RENDER_LIGHTINGS_BUFFERED 1
#define UPDATE_LIGHTNINGS 1
#define USE_NEW 1

#define LIMIT_FLASH_FPS	1
#define FLASH_SLOWMO 1

#define STYLE	(((m_nStyle < 0) || (gameOpts->render.lightning.nStyle < m_nStyle)) ? \
					 gameOpts->render.lightning.nStyle : m_nStyle)

CLightningManager lightningManager;
COmegaLightnings	omegaLightnings;

//------------------------------------------------------------------------------

#define MAX_LIGHTNING_SEGMENTS	10000

typedef struct tPlasmaBuffer {
	tTexCoord2f	texCoord [4 * MAX_LIGHTNING_SEGMENTS];
	CFloatVector		vertices [4 * MAX_LIGHTNING_SEGMENTS];
} tPlasmaBuffer;

static tPlasmaBuffer plasmaBuffers [2][2];
static CFloatVector3 coreBuffer [2][MAX_LIGHTNING_SEGMENTS];

//------------------------------------------------------------------------------

#if DBG

void TRAP (CLightningNode *nodeP)
{
if (nodeP->m_child == reinterpret_cast<CLightning*> ((size_t) 0xfeeefeee))
	int i = 0;
}

void CHECK (CLightning *lightningP, int i)
{
	CLightningNode *nodeP;
	int j;

for (; i > 0; i--, lightningP++)
	if (lightningP->m_nNodes > 0)
		for (j = lightningP->m_nNodes, nodeP = lightningP->m_nodes.Buffer (); j > 0; j--, nodeP++)
			TRAP (nodeP);
}


#else
#	define TRAP(_pln)
#	define CHECK(_pl,_i)
#endif

//------------------------------------------------------------------------------

inline double dbl_rand (void)
{
return double (rand ()) / double (RAND_MAX);
}

//------------------------------------------------------------------------------

CFixVector *VmRandomVector (CFixVector *vRand)
{
	CFixVector	vr;

do {
	vr [X] = I2X (1) / 4 - d_rand ();
	vr [Y] = I2X (1) / 4 - d_rand ();
	vr [Z] = I2X (1) / 4 - d_rand ();
} while (!(vr [X] && vr [Y] && vr [Z]));
CFixVector::Normalize (vr);
*vRand = vr;
return vRand;
}

//------------------------------------------------------------------------------

#define SIGN(_i)	(((_i) < 0) ? -1 : 1)

#define VECSIGN(_v)	(SIGN ((_v) [X]) * SIGN ((_v) [Y]) * SIGN ((_v) [Z]))

//------------------------------------------------------------------------------

CFixVector *DirectedRandomVector (CFixVector *vRand, CFixVector *vDir, int nMinDot, int nMaxDot)
{
	CFixVector	vr, vd = *vDir, vSign;
	int			nDot, nSign, i = 0;

CFixVector::Normalize (vd);
vSign [X] = vd [X] ? vd [X] / abs(vd [X]) : 0;
vSign [Y] = vd [Y] ? vd [Y] / abs(vd [Y]) : 0;
vSign [Z] = vd [Z] ? vd [Z] / abs(vd [Z]) : 0;
nSign = VECSIGN (vd);
do {
	VmRandomVector (&vr);
	nDot = CFixVector::Dot (vr, vd);
	if (++i == 100)
		i = 0;
	} while ((nDot > nMaxDot) || (nDot < nMinDot));
*vRand = vr;
return vRand;
}

//------------------------------------------------------------------------------

bool CLightningNode::CreateChild (CFixVector *vEnd, CFixVector *vDelta,
											 int nLife, int nLength, int nAmplitude,
											 char nAngle, short nNodes, short nChildren, char nDepth, short nSteps,
											 short nSmoothe, char bClamp, char bPlasma, char bLight,
											 char nStyle, tRgbaColorf *colorP, CLightning *parentP, short nNode)
{
if (!(m_child = new CLightning))
	return false;
m_child->Init (&m_vPos, vEnd, vDelta, -1, nLife, 0, nLength, nAmplitude, nAngle, 0,
					nNodes, nChildren, nSteps, nSmoothe, bClamp, bPlasma, bLight,
					nStyle, colorP, parentP, nNode);
return m_child->Create (nDepth - 1);
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
*vPos *= FixDiv(nAmplitude, nDist);	//scale down to length nAmplitude
*vPos += vRoot;	//recalculate path point
return nAmplitude;
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

CFixVector *CLightningNode::Create (CFixVector *vOffs, CFixVector *vAttract, int nDist)
{
	CFixVector	va = *vAttract;
	int			nDot, i = 0;

if (nDist < I2X (1) / 16)
	return VmRandomVector (vOffs);
CFixVector::Normalize (va);
if (!(va [X] && va [Y] && va [Z]))
	i = 0;
do {
	VmRandomVector (vOffs);
	nDot = CFixVector::Dot (va, *vOffs);
	if (++i > 100)
		i = 0;
	} while (abs (nDot) < I2X (1) / 32);
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
*vOffs *= FixDiv(i * nDist / 2, nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
*vOffs += *vAttract;	//add offset and attractor vectors (attractor is the bigger the closer)
nMag = vOffs->Mag ();
*vOffs *= FixDiv(bJoinPaths ? nDist / 2 : nDist, nMag);	//rescale to desired path length
*vPos += *vOffs;
return vPos;
}

//------------------------------------------------------------------------------

CFixVector CLightningNode::CreateJaggy (CFixVector *vPos, CFixVector *vDest, CFixVector *vBase, CFixVector *vPrevOffs,
												   int nSteps, int nAmplitude, int nMinDist, int i, int nSmoothe, int bClamp)
{
	CFixVector	vAttract, vOffs;
	int			nDist = ComputeAttractor (&vAttract, vDest, vPos, nMinDist, i);

Create (&vOffs, &vAttract, nDist);
if (vPrevOffs)
	Smoothe (&vOffs, vPrevOffs, nDist, nSmoothe);
else if (m_vOffs [X] || m_vOffs [Z] || m_vOffs [Z]) {
	vOffs += m_vOffs * (I2X (2));
	vOffs /= 3;
	}
if (nDist > I2X (1) / 16)
	Attract (&vOffs, &vAttract, vPos, nDist, i, 0);
if (bClamp)
	Clamp (vPos, vBase, nAmplitude);
m_vNewPos = *vPos;
m_vOffs = m_vNewPos - m_vPos;
m_vOffs *= (I2X (1) / nSteps);
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

CFixVector CLightningNode::CreatePerlin (int nSteps, int nAmplitude, int *nSeed, double phi, double i)
{
double dx = PerlinNoise1D (i, 0.25, 6, nSeed [0]);
double dy = PerlinNoise1D (i, 0.25, 6, nSeed [1]);
phi = sin (phi * Pi);
phi = sqrt (phi);
dx *= nAmplitude * phi;
dy *= nAmplitude * phi;
m_vNewPos = m_vBase + m_vDelta [0] * int (dx);
m_vNewPos += m_vDelta [1] * int (dy);
m_vOffs = m_vNewPos - m_vPos;
m_vOffs *= (I2X (1) / nSteps);
return m_vOffs;
}

//------------------------------------------------------------------------------

void CLightningNode::Move (CFixVector& vDelta, short nSegment)
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

int CLightning::ComputeChildEnd (CFixVector *vPos, CFixVector *vEnd, CFixVector *vDir, CFixVector *vParentDir, int nLength)
{
nLength = 3 * nLength / 4 + int (dbl_rand () * nLength / 4);
DirectedRandomVector (vDir, vParentDir, I2X (3) / 4, I2X (9) / 10);
*vEnd = *vPos + *vDir * nLength;
return nLength;
}

//------------------------------------------------------------------------------

void CLightning::Setup (bool bInit)
{
	CFixVector		vPos, vDir, vRefDir, vDelta [2], v;
	int				i = 0, l;

m_vPos = m_vBase;
if (m_bRandom) {
	if (!m_nAngle)
		VmRandomVector (&vDir);
	else {
		int nMinDot = I2X (1) - I2X (m_nAngle) / 90;
		vRefDir = m_vRefEnd - m_vPos;
		CFixVector::Normalize (vRefDir);
		do {
			VmRandomVector (&vDir);
		}
		while (CFixVector::Dot (vRefDir, vDir) < nMinDot);
	}
	m_vEnd = m_vPos + vDir * m_nLength;
}
else {
	vDir = m_vEnd - m_vPos;
	CFixVector::Normalize (vDir);
	}
m_vDir = vDir;
if (m_nOffset) {
	i = m_nOffset / 2 + int (dbl_rand () * m_nOffset / 2);
	m_vPos += vDir * i;
	m_vEnd += vDir * i;
	}
vPos = m_vPos;
if (m_bInPlane) {
	vDelta [0] = m_vDelta;
	vDelta [i].SetZero ();
	}
else {
	do {
		VmRandomVector(&vDelta [0]);
	} while (abs (CFixVector::Dot (vDir, vDelta [0])) > I2X (9) / 10);
	vDelta [1] = CFixVector::Normal (vPos, m_vEnd, *vDelta);
	v = vPos + vDelta [1];
	vDelta [0] = CFixVector::Normal (vPos, m_vEnd, v);
	}
vDir *= FixDiv (m_nLength, I2X (m_nNodes - 1));
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

void CLightning::Init (CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
							  short nObject, int nLife, int nDelay, int nLength, int nAmplitude,
							  char nAngle, int nOffset, short nNodes, short nChildren, short nSteps,
							  short nSmoothe, char bClamp, char bPlasma, char bLight,
							  char nStyle, tRgbaColorf *colorP, CLightning *parentP, short nNode)
{
	int	bRandom = (vEnd == NULL) || (nAngle > 0);

memset (this, 0, sizeof (*this));
if (nObject < 0) {
	m_nObject = -1;
	m_nSegment = -nObject - 1;
	}
else {
	m_nObject = nObject;
	}
m_parent = parentP;
m_nNode = nNode;
m_nNodes = nNodes;
m_nChildren = gameOpts->render.lightning.nQuality ? (nChildren < 0) ? nNodes / 10 : nChildren : 0;
if (vEnd) {
	m_vRefEnd = *vEnd;
	m_vEnd = *vEnd;
	}
if ((m_bInPlane = (vDelta != NULL)))
	m_vDelta = *vDelta;
m_bRandom = bRandom;
m_nLife = nLife;
m_nTTL = abs (nLife);
m_nLength = nLength;
m_nDelay = abs (nDelay) * 10;
m_nAmplitude = nAmplitude;
m_nAngle = nAngle;
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
}

//------------------------------------------------------------------------------

bool CLightning::Create (char nDepth)
{
if ((m_nObject >= 0) && (0 > (m_nSegment = OBJECTS [m_nObject].info.nSegment)))
	return NULL;
if (!m_nodes.Create (m_nNodes)) 
	return false;
m_nodes.Clear ();
if (m_bRandom) {
	m_nTTL = 3 * m_nTTL / 4 + int (dbl_rand () * m_nTTL / 2);
	m_nLength = 3 * m_nLength / 4 + int (dbl_rand () * m_nLength / 2);
	}
if (m_nAmplitude < 0)
	m_nAmplitude = m_nLength / 6;
Setup (true);
if (gameOpts->render.lightning.nQuality && nDepth && m_nChildren) {
	int			h, l, n, nNode;
	double		nStep, j;

	n = m_nChildren + 1 + (m_nChildren < 2);
	nStep = double (m_nNodes) / double (m_nChildren);
	l = m_nLength / n;
	for (h = m_nNodes - int (nStep), j = nStep / 2; j < h; j += nStep) {
		nNode = int (j) + 2 - d_rand () % 5;
		if (nNode < 1)
			nNode = int (j);
		if (!m_nodes [nNode].CreateChild (&m_vEnd, &m_vDelta, m_nLife, l, m_nAmplitude / n * 2, m_nAngle,
													 2 * m_nNodes / n, m_nChildren / 5, nDepth - 1, m_nSteps / 2, m_nSmoothe, m_bClamp, m_bPlasma, m_bLight,
													 m_nStyle, &m_color, this, nNode))
			return false;
		}
	}
return true;
}

//------------------------------------------------------------------------------

void CLightning::DestroyNodes (void)
{
	CLightningNode	*nodeP = m_nodes.Buffer ();

if (nodeP) {
	for (int i = abs (m_nNodes); i > 0; i--, nodeP++)
		nodeP->Destroy ();
	m_nodes.Destroy ();
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

for (i = m_nNodes - 1, j = 0, pfi = m_nodes.Buffer (), plh = NULL; j < i; j++) {
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
	CFixVector	vBase [2];

nSteps = m_nSteps;
nAmplitude = m_nAmplitude;
vBase [0] = m_vPos;
vBase [1] = m_nodes [m_nNodes - 1].m_vPos;
for (i = m_nNodes - 1 - !m_bRandom, nodeP = m_nodes + 1; i > 0; i--, nodeP++) {
	nDist = CFixVector::Dist (nodeP->m_vNewPos, nodeP->m_vPos);
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
	CFixVector	vPos [2], vBase [2], vPrevOffs [2];
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
plh = m_nodes.Buffer ();
plh->m_vNewPos = plh->m_vPos;
plh->m_vOffs.SetZero ();
if ((nDepth > 1) || m_bRandom) {
	if (nStyle == 2) {
		if (nDepth > 1)
			nAmplitude *= 4;
		for (i = 0; i < m_nNodes; i++) {
			phi = bClamp ? double (i) / double (m_nNodes - 1) : 1;
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
	plh = &m_nodes [m_nNodes - 1];
	plh->m_vNewPos = plh->m_vPos;
	plh->m_vOffs.SetZero ();
	if (nStyle == 2) {
		nAmplitude = 5 * nAmplitude / 3;
		for (h = m_nNodes, i = 0, plh = m_nodes.Buffer (); i < h; i++, plh++) {
			phi = bClamp ? double (i) / double (h - 1) : 1;
			plh->CreatePerlin (nSteps, nAmplitude, nSeed, phi, phi * 10);
			}
		}
	else {
		if (m_bInPlane)
			nStyle = 0;
		if (!nStyle) {
			nAmplitude *= 4;
			for (h = m_nNodes - 1, i = j = 0, nodeP [0] = m_nodes + 1, nodeP [1] = &m_nodes [h - 1]; i < h; i++, j = !j) {
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
			for (i = m_nNodes - 1, j = 0, nodeP [0] = m_nodes + 1, nodeP [1] = &m_nodes [i - 1]; i > 0; i--, j = !j) {
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
			m_nTTL = h + int (dbl_rand () * h);
			}
		else {
			if (m_bRandom) {
				h = -m_nLife;
				m_nTTL = 3 * h / 4 + int (dbl_rand () * h / 2);
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

void CLightning::Move (CFixVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd)
{
if (nSegment < 0)
	return;
if (!m_nodes.Buffer ())
	return;

	CLightningNode	*nodeP;
	CFixVector		vDelta, vOffs;
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
	for (j = h, nodeP = m_nodes.Buffer (); j > 0; j--, nodeP++) {
		if (bStretch) {
			vDelta = vOffs;
			if (bFromEnd)
				fStep = double (h - j + 1) / double (h);
			else
				fStep = double (j) / double (h);
			vDelta [X] = int (vOffs [X] * fStep + 0.5);
			vDelta [Y] = int (vOffs [Y] * fStep + 0.5);
			vDelta [Z] = int (vOffs [Z] * fStep + 0.5);
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

void CLightning::ComputePlasmaSegment (CFloatVector *vPosf, int bScale, short nSegment, char bStart, char bEnd, int nDepth, int nThread)
{
	CFloatVector			*vPlasma = plasmaBuffers [nThread][bScale].vertices + 4 * nSegment;
	CFloatVector			vn [2], vd;

	static CFloatVector vEye = CFloatVector::ZERO;
	static CFloatVector vNormal [3] = {CFloatVector::ZERO,CFloatVector::ZERO,CFloatVector::ZERO};

memcpy (vNormal, vNormal + 1, 2 * sizeof (CFloatVector));
if (bStart) {
	vNormal [1] = CFloatVector::Normal (vPosf [0], vPosf [1], vEye);
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
	vNormal [2] = CFloatVector::Normal (vPosf [1], vPosf [2], vEye);
	if (CFloatVector::Dot (vNormal [1], vNormal [2]) < 0)
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
	CFloatVector::Normalize (vd);
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
	CFloatVector			vPosf [3] = {CFloatVector::ZERO,CFloatVector::ZERO,CFloatVector::ZERO};
	int				bScale, i, j;

for (bScale = 0; bScale < 2; bScale++) {
	nodeP = m_nodes.Buffer ();
	vPosf [2].Assign (nodeP->m_vPos);
	nodeP++;
	if (!ogl.m_states.bUseTransform)
		transformation.Transform (vPosf [2], vPosf [2], 0);
	for (i = m_nNodes - 2, j = 0, nodeP = m_nodes.Buffer (); j <= i; j++) {
		memcpy (vPosf, vPosf + 1, 2 * sizeof (CFloatVector));
		nodeP++;
		vPosf [2].Assign (nodeP->m_vPos);
		if (!ogl.m_states.bUseTransform)
			transformation.Transform (vPosf [2], vPosf [2], 0);
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
	CFloatVector			*vertexP;
	int				i, j;
#endif

if (!ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0))
	return;
glBlendFunc (GL_ONE, GL_ONE);
for (bScale = 0; bScale < 2; bScale++) {
	if (bScale)
		glColor4f (0.1f, 0.1f, 0.1f, colorP->alpha / 2);
	else
		glColor4f (colorP->red / 4, colorP->green / 4, colorP->blue / 4, colorP->alpha);
	glTexCoordPointer (2, GL_FLOAT, 0, plasmaBuffers [nThread][bScale].texCoord);
	glVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), plasmaBuffers [nThread][bScale].vertices);
	glDrawArrays (GL_QUADS, 0, 4 * (m_nNodes - 1));
#if RENDER_LIGHTNING_OUTLINE
	glDisable (GL_TEXTURE_2D);
	glColor3f (1,1,1);
	texCoordP = plasmaBuffers [nThread][bScale].texCoord;
	vertexP = plasmaBuffers [nThread][bScale].vertices;
	for (i = nNodes - 1; i; i--) {
		glBegin (GL_LINE_LOOP);
		for (j = 0; j < 4; j++) {
			glTexCoord2fv (reinterpret_cast<GLfloat*> (texCoordP++));
			glVertex3fv (reinterpret_cast<GLfloat*> (vertexP++));
			}
		glEnd ();
		}
#endif
	}
ogl.DisableClientStates (1, 0, 0, GL_TEXTURE0);
}

//------------------------------------------------------------------------------

void CLightning::RenderCore (tRgbaColorf *colorP, int nDepth, int nThread)
{
	CFloatVector3*	vPosf = coreBuffer [nThread];
	int				i;

glBlendFunc (GL_ONE, GL_ONE);
glColor4f (colorP->red / 4, colorP->green / 4, colorP->blue / 4, colorP->alpha);
glLineWidth ((GLfloat) (nDepth ? 2 : 4));
glEnable (GL_LINE_SMOOTH);
for (i = 0; i < m_nNodes; i++)
	vPosf [i].Assign (m_nodes [i].m_vPos);
if (!ogl.m_states.bUseTransform)
	OglSetupTransform (1);
#if 1
if (ogl.EnableClientStates (0, 0, 0, GL_TEXTURE0)) {
	glDisable (GL_TEXTURE_2D);
	glVertexPointer (3, GL_FLOAT, 0, coreBuffer [nThread]);
	glDrawArrays (GL_LINE_STRIP, 0, m_nNodes);
	ogl.DisableClientStates (0, 0, 0, -1);
	}
else {
	glActiveTexture (GL_TEXTURE0);
	glDisable (GL_TEXTURE_2D);
	glBegin (GL_LINE_STRIP);
	for (i = m_nNodes, vPosf = coreBuffer [nThread]; i; i--, vPosf++)
		glVertex3fv (reinterpret_cast<GLfloat*> (vPosf));
	glEnd ();
	}
#endif
if (!ogl.m_states.bUseTransform)
	OglResetTransform (1);
glLineWidth ((GLfloat) 1);
glDisable (GL_LINE_SMOOTH);
OglClearError (0);
}

//------------------------------------------------------------------------------

int CLightning::SetupPlasma (void)
{
if (!(gameOpts->render.lightning.bPlasma && m_bPlasma && ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0)))
	return 0;
glActiveTexture (GL_TEXTURE0);
glClientActiveTexture (GL_TEXTURE0);
glEnable (GL_TEXTURE_2D);
if (LoadCorona () && !bmpCorona->Bind (1)) {
	bmpCorona->Texture ()->Wrap (GL_CLAMP);
	return 1;
	}
ogl.DisableClientStates (1, 0, 0, GL_TEXTURE0);
return 0;
}

//------------------------------------------------------------------------------

void CLightning::RenderBuffered (int nDepth, int nThread)
{
	int				i, bPlasma;
	tRgbaColorf		color;

if (!m_nodes.Buffer () || (m_nNodes <= 0) || (m_nSteps < 0))
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
if (gameOpts->render.lightning.nQuality)
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
	transparencyRenderer.AddLightning (this, nDepth);
	if (gameOpts->render.lightning.nQuality)
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
if (!m_nodes.Buffer ())
	return 0;
if (0 < (j = m_nNodes)) {
	if (!(nStride = int ((double (m_nLength) / I2X (20) + 0.5))))
		nStride = 1;
	if (!(nStep = double (j - 1) / double (nStride)))
		nStep = double (int (j + 0.5));
#if DBG
	if (m_nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	for (h = nStep / 2; h < j; h += nStep) {
		if (!m_nodes [int (h)].SetLight (m_nSegment, &m_color))
			break;
		nLights++;
		}
	}
return nLights;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CLightningSystem::Init (int nId)
{
m_bValid = 0;
m_nId = nId;
}

//------------------------------------------------------------------------------

bool CLightningSystem::Create (int nLightnings, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
										 short nObject, int nLife, int nDelay, int nLength, int nAmplitude,
										 char nAngle, int nOffset, short nNodes, short nChildren, char nDepth, short nSteps,
										 short nSmoothe, char bClamp, char bPlasma, char bSound, char bLight,
										 char nStyle, tRgbaColorf *colorP)
{
m_nObject = nObject;
if (!(nLife && nLength && (nNodes > 4)))
	return false;
m_nLightnings = nLightnings;
if (nObject >= 0)
	m_nSegment [0] =
	m_nSegment [1] = -1;
else {
	m_nSegment [0] = FindSegByPos (*vPos, -1, 1, 0);
	m_nSegment [1] = FindSegByPos (*vEnd, -1, 1, 0);
	}
m_bForcefield = !nDelay && (vEnd || (nAngle <= 0));
if (!m_lightnings.Create (nLightnings))
	return false;
m_lightnings.Clear ();
CLightning l;
l.Init (vPos, vEnd, vDelta, nObject, nLife, nDelay, nLength, nAmplitude, 
		  nAngle, nOffset, nNodes, nChildren, nSteps,
		  nSmoothe, bClamp, bPlasma, bLight, nStyle, colorP, false, -1);
for (int i = 0; i < nLightnings; i++) {
	m_lightnings [i] = l;
	if (!m_lightnings [i].Create (0))
		return false;
	}
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
m_bValid = 
m_bDestroy = 0;
DestroySound ();
#if 0
if (m_lightnings.Buffer ()) {
	for (int i = 0; i < m_nLightnings; i++)
		m_lightnings [i].Destroy ();
	m_lightnings.Destroy ();
	m_nLightnings = 0;
	}
#else
m_lightnings.Destroy ();	//class and d-tors will handle everything gracefully
m_nLightnings = 0;
#endif
if ((m_nObject >= 0) && (lightningManager.GetObjectSystem (m_nObject) == m_nId))
	lightningManager.SetObjectSystem (m_nObject, -1);
m_nObject = -1;
}

//------------------------------------------------------------------------------

void CLightningSystem::CreateSound (int bSound)
{
if ((m_bSound = bSound)) {
	audio.CreateObjectSound (-1, SOUNDCLASS_GENERIC, m_nObject, 1, I2X (1), I2X (256), -1, -1, AddonSoundName (SND_ADDON_LIGHTNING));
	if (m_bForcefield) {
		if (0 <= (m_nSound = audio.GetSoundByName ("ff_amb_1")))
			audio.CreateObjectSound (m_nSound, SOUNDCLASS_GENERIC, m_nObject, 1);
		}
	}
else
	m_nSound = -1;
}

//------------------------------------------------------------------------------

void CLightningSystem::DestroySound (void)
{
if ((m_bSound > 0) & (m_nObject >= 0))
	audio.DestroyObjectSound (m_nObject);
}

//------------------------------------------------------------------------------

void CLightningSystem::Animate (int nStart, int nLightnings)
{
if (!m_bValid)
	return;

	CLightning *lightningP = m_lightnings + nStart;

if (nLightnings < 0)
	nLightnings = m_nLightnings;
for (; nLightnings; nLightnings--, lightningP++)
	lightningP->Animate (0);
}

//------------------------------------------------------------------------------

int CLightningSystem::SetLife (void)
{
if (!m_bValid)
	return 0;

	CLightning	*lightningP = m_lightnings.Buffer ();
	int			i;

for (i = 0; i < m_nLightnings; ) {
	if (!lightningP->SetLife ()) {
		lightningP->DestroyNodes ();
		if (!--m_nLightnings)
			return 0;
		if (i < m_nLightnings) {
			*lightningP = m_lightnings [m_nLightnings];
			memset (m_lightnings + m_nLightnings, 0, sizeof (m_lightnings [m_nLightnings]));
			continue;
			}
		}
	i++, lightningP++;
	}
return m_nLightnings;
}

//------------------------------------------------------------------------------

int CLightningSystem::Update (void)
{
if (m_bDestroy) {
	Destroy ();
	return -1;
	}

if (!m_bValid)
	return 0;

if (gameStates.app.nSDLTicks - m_tUpdate >= 25) {
	if (!(m_nKey [0] || m_nKey [1])) {
		m_tUpdate = gameStates.app.nSDLTicks;
		Animate (0, m_nLightnings);
		if (!(m_nLightnings = SetLife ()))
			lightningManager.Destroy (this, NULL);
		else if (m_nObject >= 0) {
			UpdateSound ();
			MoveForObject ();
			}
		}
	}
return m_nLightnings;
}

//------------------------------------------------------------------------------

void CLightningSystem::Mute (void)
{
m_bSound = -1;
}

//------------------------------------------------------------------------------

void CLightningSystem::UpdateSound (void)
{
if (!m_bValid)
	return;
if (!m_bSound)
	return;

	CLightning	*lightningP;
	int			i;

for (i = m_nLightnings, lightningP = m_lightnings.Buffer (); i > 0; i--, lightningP++)
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

void CLightningSystem::Move (CFixVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd)
{
if (!m_bValid)
	return;
if (nSegment < 0)
	return;
if (!m_lightnings.Buffer ())
	return;
if (SHOW_LIGHTNING) {
	for (int i = 0; i < m_nLightnings; i++)
		m_lightnings [i].Move (vNewPos, nSegment, bStretch, bFromEnd);
	}
}

//------------------------------------------------------------------------------

void CLightningSystem::MoveForObject (void)
{
if (!m_bValid)
	return;

	CObject* objP = OBJECTS + m_nObject;

Move (&OBJPOS (objP)->vPos, objP->info.nSegment, 0, 0);
}

//------------------------------------------------------------------------------

void CLightningSystem::Render (int nStart, int nLightnings, int bDepthSort, int nThread)
{
if (!m_bValid)
	return;

if (automap.m_bDisplay && !(gameStates.render.bAllVisited || automap.m_bFull)) {
	if (m_nObject >= 0) {
		if (!automap.m_visited [0][OBJECTS [m_nObject].Segment ()])
			return;
		}
	else if (!automap.m_bFull) {
		if (((m_nSegment [0] >= 0) && !automap.m_visible [m_nSegment [0]]) &&
			 ((m_nSegment [1] >= 0) && !automap.m_visible [m_nSegment [1]]))
			return;
		}
	}

CLightning *lightningP = m_lightnings + nStart;

if (nLightnings < 0)
	nLightnings = m_nLightnings;
for (; nLightnings; nLightnings--, lightningP++)
	lightningP->Render (0, bDepthSort, nThread);
}

//------------------------------------------------------------------------------

int CLightningSystem::SetLight (void)
{
if (!m_bValid)
	return 0;

	int nLights = 0;

if (m_lightnings.Buffer ())
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
m_nFirstLight = -1;
}

//------------------------------------------------------------------------------

CLightningManager::~CLightningManager () 
{ 
Shutdown (true);
}

//------------------------------------------------------------------------------

void CLightningManager::Init (void)
{
if (!(m_objects.Buffer () || m_objects.Create (LEVEL_OBJECTS))) {
	Shutdown (1);
	extraGameInfo [0].bUseLightning = 0;
	return;
	}
m_objects.Clear (0xff);
if (!(m_lights.Buffer () || m_lights.Create (2 * LEVEL_SEGMENTS))) {
	Shutdown (1);
	extraGameInfo [0].bUseLightning = 0;
	return;
	}
if (!m_systems.Create (MAX_LIGHTNING_SYSTEMS)) {
	Shutdown (1);
	extraGameInfo [0].bUseLightning = 0;
	return;
	}	
int i = 0;
int nCurrent = m_systems.FreeList ();
for (CLightningSystem* systemP = m_systems.GetFirst (nCurrent); systemP; systemP = m_systems.GetNext (nCurrent))
	systemP->Init (i++);
m_nFirstLight = -1;
m_bDestroy = 0;
}

//------------------------------------------------------------------------------

int CLightningManager::Create (int nLightnings, CFixVector *vPos, CFixVector *vEnd, CFixVector *vDelta,
										 short nObject, int nLife, int nDelay, int nLength, int nAmplitude,
										 char nAngle, int nOffset, short nNodes, short nChildren, char nDepth, short nSteps,
										 short nSmoothe, char bClamp, char bPlasma, char bSound, char bLight,
										 char nStyle, tRgbaColorf *colorP)
{
if (!(SHOW_LIGHTNING && colorP))
	return -1;
if (!nLightnings)
	return -1;
SEM_ENTER (SEM_LIGHTNING)
CLightningSystem* systemP = m_systems.Pop ();
if (!systemP)
	return -1;
if (!(systemP->Create (nLightnings, vPos, vEnd, vDelta, nObject, nLife, nDelay, nLength, nAmplitude,
							  nAngle, nOffset, nNodes, nChildren, nDepth, nSteps, nSmoothe, bClamp, bPlasma, bSound, bLight,
							  nStyle, colorP))) {
	m_systems.Push (systemP->Id ());
	systemP->Destroy ();
	SEM_LEAVE (SEM_LIGHTNING)
	return -1;
	}
systemP->m_bValid = 1;
SEM_LEAVE (SEM_LIGHTNING)
return systemP->Id ();
}

//------------------------------------------------------------------------------

void CLightningManager::Destroy (CLightningSystem* systemP, CLightning *lightningP)
{
if (lightningP)
	lightningP->Destroy ();
else
	systemP->m_bDestroy = 1;
}

//------------------------------------------------------------------------------

void CLightningManager::Cleanup (void)
{
SEM_ENTER (SEM_LIGHTNING)
int nCurrent = -1;
for (CLightningSystem* systemP = m_systems.GetFirst (nCurrent), * nextP = NULL; systemP; systemP = nextP) {
	nextP = m_systems.GetNext (nCurrent);
	if (systemP->m_bDestroy) {
		systemP->Destroy ();
		m_systems.Push (systemP->Id ());
		}
	}
SEM_LEAVE (SEM_LIGHTNING)
}

//------------------------------------------------------------------------------

int CLightningManager::Shutdown (bool bForce)
{
if (!bForce && (m_bDestroy >= 0))
	m_bDestroy = 1;
else {
	uint bSem = gameData.app.semaphores [SEM_LIGHTNING];
	if (!bSem)
		SEM_ENTER (SEM_LIGHTNING)
	int nCurrent = -1;
	for (CLightningSystem* systemP = m_systems.GetFirst (nCurrent), * nextP = NULL; systemP; systemP = nextP) {
		nextP = m_systems.GetNext (nCurrent);
		Destroy (systemP, NULL);
		m_systems.Push (systemP->Id ());
		}
	ResetLights (1);
	m_systems.Destroy ();
	m_objects.Destroy ();
	m_lights.Destroy ();
	if (!bSem)
		SEM_LEAVE (SEM_LIGHTNING)
	}
return 1;
}

//------------------------------------------------------------------------------

void CLightningManager::Move (int i, CFixVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd)
{
if (nSegment < 0)
	return;
if (SHOW_LIGHTNING)
	m_systems [i].Move (vNewPos, nSegment, bStretch, bFromEnd);
}

//------------------------------------------------------------------------------

void CLightningManager::MoveForObject (CObject* objP)
{
	int i = objP->Index ();

if (m_objects [i] >= 0)
	Move (m_objects [i], &OBJPOS (objP)->vPos, OBJSEG (objP), 0, 0);
}

//------------------------------------------------------------------------------

tRgbaColorf *CLightningManager::LightningColor (CObject* objP)
{
if (objP->info.nType == OBJ_ROBOT) {
	if (ROBOTINFO (objP->info.nId).energyDrain) {
		static tRgbaColorf color = {1.0f, 0.8f, 0.3f, 0.2f};
		return &color;
		}
	}
else if ((objP->info.nType == OBJ_PLAYER) && gameOpts->render.lightning.bPlayers) {
	if (gameData.fusion.xCharge > I2X (2)) {
		static tRgbaColorf color = {0.666f, 0.0f, 0.75f, 0.2f};
		return &color;
		}
	int s = SEGMENTS [objP->info.nSegment].m_nType;
	if (s == SEGMENT_IS_FUELCEN) {
		static tRgbaColorf color = {1.0f, 0.8f, 0.3f, 0.2f};
		return &color;
		}
	if (s == SEGMENT_IS_REPAIRCEN) {
		static tRgbaColorf color = {0.3f, 0.5f, 0.1f, 0.2f};
		return &color;
		}
	}
return NULL;
}

//------------------------------------------------------------------------------

void CLightningManager::Update (void)
{
if (SHOW_LIGHTNING) {

		CObject	*objP;
		ubyte		h;
		int		i;

#if LIMIT_LIGHTNING_FPS
#	if LIGHTNING_SLOWMO
		static int	t0 = 0;
		int t = gameStates.app.nSDLTicks - t0;

	if (t / gameStates.gameplay.slowmo [0].fSpeed < 25)
		return;
	t0 = gameStates.app.nSDLTicks + 25 - int (gameStates.gameplay.slowmo [0].fSpeed * 25);
#	else
	if (!gameStates.app.tick40fps.bTick)
		return 0;
#	endif
#endif
	for (i = 0, objP = OBJECTS.Buffer (); i < gameData.objs.nLastObject [1]; i++, objP++) {
		if (gameData.objs.bWantEffect [i] & DESTROY_LIGHTNINGS) {
			gameData.objs.bWantEffect [i] &= ~DESTROY_LIGHTNINGS;
			DestroyForObject (objP);
			}
		}
	int nCurrent = -1;
	for (CLightningSystem* systemP = m_systems.GetFirst (nCurrent), * nextP = NULL; systemP; systemP = nextP) {
		nextP = m_systems.GetNext (nCurrent);
		if (0 > systemP->Update ())
			Destroy (systemP, NULL);
		}

	FORALL_OBJS (objP, i) {
		i = objP->Index ();
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
		else if (h & MOVE_LIGHTNINGS) {
			if ((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT))
				MoveForObject (objP);
			}
		gameData.objs.bWantEffect [i] &= ~(PLAYER_LIGHTNINGS | ROBOT_LIGHTNINGS | MISSILE_LIGHTNINGS | EXPL_LIGHTNINGS | MOVE_LIGHTNINGS);
		}
	}
}

//------------------------------------------------------------------------------
#if 0
void MoveForObject (CObject* objP)
{
SEM_ENTER (SEM_LIGHTNING)
MoveForObjectInternal (objP);
SEM_LEAVE (SEM_LIGHTNING)
}
#endif
//------------------------------------------------------------------------------

void CLightningManager::DestroyForObject (CObject* objP)
{
	int i = objP->Index ();

if (m_objects [i] >= 0) {
	Destroy (m_systems + m_objects [i], NULL);
	m_objects [i] = -1;
	}
}

//------------------------------------------------------------------------------

void CLightningManager::DestroyForAllObjects (int nType, int nId)
{
	CObject	*objP;
	//int		i;

FORALL_OBJS (objP, i) {
	if ((objP->info.nType == nType) && ((nId < 0) || (objP->info.nId == nId)))
#if 1
		objP->RequestEffects (DESTROY_LIGHTNINGS);
#else
		DestroyObjectLightnings (objP);
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
if (SHOW_LIGHTNING) {
		int bStencil = StencilOff ();

	int nCurrent = -1;
	for (CLightningSystem* systemP = m_systems.GetFirst (nCurrent); systemP; systemP = m_systems.GetNext (nCurrent))
		if (!(systemP->m_nKey [0] | systemP->m_nKey [1]))
			systemP->Render (0, systemP->m_nLightnings, gameOpts->render.bDepthSort > 0, 0);
	StencilOn (bStencil);
	}
}

//------------------------------------------------------------------------------

void CLightningManager::Mute (void)
{
if (SHOW_LIGHTNING) {
	int nCurrent = -1;
	for (CLightningSystem* systemP = m_systems.GetFirst (nCurrent); systemP; systemP = m_systems.GetNext (nCurrent))
		systemP->Mute ();
	}
}

//------------------------------------------------------------------------------

CFixVector *CLightningManager::FindTargetPos (CObject* emitterP, short nTarget)
{
	//int		i;
	CObject*	objP;

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
	CObject			*objP;
	CFixVector		*vEnd, *vDelta, v;
	tLightningInfo	*pli;
	tRgbaColorf		color;

if (!SHOW_LIGHTNING)
	return;
if (!gameOpts->render.lightning.bStatic)
	return;
FORALL_EFFECT_OBJS (objP, i) {
	if (objP->info.nId != LIGHTNING_ID)
		continue;
	i = objP->Index ();
	if (m_objects [i] >= 0)
		continue;
	pli = &objP->rType.lightningInfo;
	if (pli->nLightnings <= 0)
		continue;
	if (pli->bRandom && !pli->nAngle)
		vEnd = NULL;
	else if ((vEnd = FindTargetPos (objP, pli->nTarget)))
		pli->nLength = CFixVector::Dist (objP->info.position.vPos, *vEnd) / I2X (1);
	else {
		v = objP->info.position.vPos + objP->info.position.mOrient.FVec () * I2X (pli->nLength);
		vEnd = &v;
		}
	color.red = (float) pli->color.red / 255.0f;
	color.green = (float) pli->color.green / 255.0f;
	color.blue = (float) pli->color.blue / 255.0f;
	color.alpha = (float) pli->color.alpha / 255.0f;
	vDelta = pli->bInPlane ? &objP->info.position.mOrient.RVec () : NULL;
	h = Create (pli->nLightnings, &objP->info.position.vPos, vEnd, vDelta, i, -abs (pli->nLife), pli->nDelay, I2X (pli->nLength),
				   I2X (pli->nAmplitude), pli->nAngle, I2X (pli->nOffset), pli->nNodes, pli->nChildren, pli->nChildren > 0, pli->nSteps,
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
	Shutdown (0);
	}
else {
	Update ();
	omegaLightnings.Update (NULL, NULL);
	StaticFrame ();
	Cleanup ();
	}
}

//------------------------------------------------------------------------------

void CLightningManager::SetSegmentLight (short nSegment, CFixVector *vPosP, tRgbaColorf *colorP)
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
if ((SHOW_LIGHTNING || bForce) && m_lights.Buffer ()) {
		tLightningLight	*llP;
		int					i;

	for (i = m_nFirstLight; i >= 0; ) {
		if ((i < 0) || (i >= LEVEL_SEGMENTS))
			break;
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
	lightManager.DeleteLightnings ();
	}
}

//------------------------------------------------------------------------------

void CLightningManager::SetLights (void)
{
	int nLights = 0;

ResetLights (0);
if (SHOW_LIGHTNING) {
		tLightningLight	*llP = NULL;
		int					i, n, bDynLighting = gameStates.render.nLightingMethod;

	m_nFirstLight = -1;
	int nCurrent = -1;
	for (CLightningSystem* systemP = m_systems.GetFirst (nCurrent); systemP; systemP = m_systems.GetNext (nCurrent))
		nLights += systemP->SetLight ();
	if (!nLights)
		return;
	nLights = 0;
	for (i = m_nFirstLight; i >= 0; i = llP->nNext) {
		if ((i < 0) || (i >= LEVEL_SEGMENTS))
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
			llP->nDynLight = lightManager.Add (NULL, &llP->color, llP->nBrightness, llP->nSegment, -1, -1, -1, &llP->vPos);
			nLights++;
			}
		}
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForExplosion (CObject* objP, tRgbaColorf *colorP, int nRods, int nRad, int nTTL)
{
if (SHOW_LIGHTNING && gameOpts->render.lightning.bExplosions) {
	//m_objects [objP->Index ()] =
		Create (
			nRods, &objP->info.position.vPos, NULL, NULL, objP->Index (), nTTL, 0,
			nRad, I2X (4), 0, I2X (2), 50, 5, 1, 3, 1, 1, 0, 0, 1, -1, colorP);
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForShaker (CObject* objP)
{
static tRgbaColorf color = {0.1f, 0.1f, 0.8f, 0.2f};

CreateForExplosion (objP, &color, 30, I2X (20), 750);
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForShakerMega (CObject* objP)
{
static tRgbaColorf color = {0.1f, 0.1f, 0.6f, 0.2f};

CreateForExplosion (objP, &color, 20, I2X (15), 750);
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForMega (CObject* objP)
{
static tRgbaColorf color = {0.8f, 0.1f, 0.1f, 0.2f};

CreateForExplosion (objP, &color, 30, I2X (15), 750);
}

//------------------------------------------------------------------------------

int CLightningManager::CreateForMissile (CObject* objP)
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

void CLightningManager::CreateForBlowup (CObject* objP)
{
static tRgbaColorf color = {0.1f, 0.1f, 0.8f, 0.2f};

int h = X2I (objP->info.xSize) * 2;

CreateForExplosion (objP, &color, h + rand () % h, h * (I2X (1) + I2X (1) / 2), 500);
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForRobot (CObject* objP, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNING && gameOpts->render.lightning.bRobots && OBJECT_EXISTS (objP)) {
		int h, i = objP->Index ();

	if (0 <= m_objects [i])
		MoveForObject (objP);
	else {
		h = Create (2 * objP->info.xSize / I2X (1), &objP->info.position.vPos, NULL, NULL, objP->Index (), -1000, 100,
						objP->info.xSize, objP->info.xSize / 8, 0, 0, 25, 3, 1, 3, 1, 1, 0, 0, 1, 0, colorP);
		if (h >= 0)
			m_objects [i] = h;
		}
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForPlayer (CObject* objP, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNING && gameOpts->render.lightning.bPlayers && OBJECT_EXISTS (objP)) {
	int h, i = objP->Index ();

	if (0 <= m_objects [i])
		MoveForObject (objP);
	else {
		h = Create (4 * objP->info.xSize / I2X (1), &OBJPOS (objP)->vPos, NULL, NULL, objP->Index (), -5000, 1000,
						4 * objP->info.xSize, objP->info.xSize, 0, 2 * objP->info.xSize, 50, 5, 1, 5, 1, 1, 0, 1, 1, 1, colorP);
		if (h >= 0)
			m_objects [i] = h;
		}
	}
}

//------------------------------------------------------------------------------

void CLightningManager::CreateForDamage (CObject* objP, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNING && gameOpts->render.lightning.bDamage && OBJECT_EXISTS (objP)) {
		int h, n, i = objP->Index ();

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
		Destroy (m_systems + h, NULL);
		}
	h = Create (n, &objP->info.position.vPos, NULL, NULL, objP->Index (), -1000, 4000,
					objP->info.xSize, objP->info.xSize / 8, 0, 0, 20, 0, 1, 10, 1, 1, 0, 0, 0, -1, colorP);
	if (h >= 0)
		m_objects [i] = h;
	}
}

//------------------------------------------------------------------------------

int CLightningManager::FindDamageLightning (short nObject, int *pKey)
{
int nCurrent = -1;
for (CLightningSystem* systemP = m_systems.GetFirst (nCurrent); systemP; systemP = m_systems.GetNext (nCurrent))
	if ((systemP->m_nObject == nObject) && (systemP->m_nKey [0] == pKey [0]) && (systemP->m_nKey [1] == pKey [1]))
		return systemP->Id ();
return -1;
}

//------------------------------------------------------------------------------

typedef union tPolyKey {
	int	i [2];
	short	s [4];
} tPolyKey;

void CLightningManager::RenderForDamage (CObject* objP, g3sPoint **pointList, RenderModel::CVertex *vertP, int nVertices)
{
	CLightningSystem*	systemP;
	CFloatVector		v, vPosf, vEndf, vNormf, vDeltaf;
	CFixVector			vPos, vEnd, vNorm, vDelta;
	int					h, i, j, bUpdate = 0;
	short					nObject;
	tPolyKey				key;

	static short	nLastObject = -1;
	static float	fDamage;
	static int		nFrameFlipFlop = -1;

	static tRgbaColorf color = {0.2f, 0.2f, 1.0f, 1.0f};

if (!(SHOW_LIGHTNING && gameOpts->render.lightning.bDamage))
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
		key.s [i] = vertP [i].m_nIndex;
	for (; i < 4; i++)
		key.s [i] = 0;
	}
i = FindDamageLightning (nObject = objP->Index (), key.i);
if (i < 0) {
	if ((nLastObject != nObject) || (nFrameFlipFlop != gameStates.render.nFrameFlipFlop)) {
		nLastObject = nObject;
		nFrameFlipFlop = gameStates.render.nFrameFlipFlop;
		fDamage = (0.5f - objP->Damage ()) / 250.0f;
		}
#if 1
	if (dbl_rand () > fDamage)
		return;
#endif
	if (pointList) {
		vPos = pointList [0]->p3_src;
		vEnd = pointList [1 + d_rand () % (nVertices - 1)]->p3_vec;
		vNorm = CFixVector::Normal (vPos, pointList [1]->p3_vec, vEnd);
		vPos += vNorm * (I2X (1) / 64);
		vEnd += vNorm * (I2X (1) / 64);
		vDelta = CFixVector::Normal (vNorm, vPos, vEnd);
		h = CFixVector::Dist (vPos, vEnd);
		}
	else {
		memcpy (&vPosf, &vertP->m_vertex, sizeof (CFloatVector3));
		memcpy (&vEndf, &vertP [1 + d_rand () % (nVertices - 1)].m_vertex, sizeof (CFloatVector3));
		memcpy (&v, &vertP [1].m_vertex, sizeof (CFloatVector3));
		vNormf = CFloatVector::Normal (vPosf, v, vEndf);
		vPosf += vNormf * (1.0f / 64.0f);
		vEndf += vNormf * (1.0f / 64.0f);
		vDeltaf = CFloatVector::Normal (vNormf, vPosf, vEndf);
		h = F2X (CFloatVector::Dist (vPosf, vEndf));
		vPos.Assign (vPosf);
		vEnd.Assign (vEndf);
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
		Destroy (m_systems + i, NULL);
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
	lightningManager.Destroy (lightningManager.m_systems + m_handles [nHandle].nLightning, NULL);
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

CFixVector *COmegaLightnings::GetGunPoint (CObject* objP, CFixVector *vMuzzle)
{
	CFixVector			*vGunPoints;
	int					bSpectate;
	tObjTransformation	*posP;

if (!objP)
	return NULL;
bSpectate = SPECTATOR (objP);
posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
if ((bSpectate || (objP->info.nId != gameData.multiplayer.nLocalPlayer)) &&
	 (vGunPoints = GetGunPoints (objP, 6))) {
	TransformGunPoint (objP, vGunPoints, 6, 0, 0, vMuzzle, NULL);
	}
else {
	*vMuzzle = posP->vPos - posP->mOrient.UVec ();
	*vMuzzle += posP->mOrient.FVec () * (objP->info.xSize / 3);
	}
return vMuzzle;
}

// ---------------------------------------------------------------------------------

int COmegaLightnings::Update (CObject* parentObjP, CObject* targetObjP)
{
	CFixVector					vMuzzle;
	tOmegaLightningHandles	*handleP;
	CWeaponState				*wsP;
	int							i, j, nHandle, nLightning;

if (!(SHOW_LIGHTNING && gameOpts->render.lightning.bOmega && !gameStates.render.bOmegaModded))
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

int COmegaLightnings::Create (CFixVector *vTargetPos, CObject* parentObjP, CObject* targetObjP)
{
	tOmegaLightningHandles	*handleP;
	int							nObject;

if (!(SHOW_LIGHTNING && gameOpts->render.lightning.bOmega && !gameStates.render.bOmegaModded))
	return 0;
if ((parentObjP->info.nType == OBJ_ROBOT) && (!gameOpts->render.lightning.bRobotOmega || gameStates.app.bHaveMod))
	return 0;
nObject = OBJ_IDX (parentObjP);
if (Update (parentObjP, targetObjP)) {
	if (!(handleP = m_handles + Find (nObject)))
		return 0;
	}
else {
	static tRgbaColorf	color = {0.9f, 0.6f, 0.6f, 0.3f};
	CFixVector	vMuzzle, *vTarget;

	Destroy (nObject);
	GetGunPoint (parentObjP, &vMuzzle);
	handleP = m_handles + m_nHandles++;
	handleP->nParentObj = nObject;
	handleP->nTargetObj = targetObjP ? OBJ_IDX (targetObjP) : -1;
	vTarget = targetObjP ? &targetObjP->info.position.vPos : vTargetPos;
#if OMEGA_PLASMA
	color.alpha = gameOpts->render.lightning.bPlasma ? 0.5f : 0.3f;
#endif
	handleP->nLightning = 
		lightningManager.Create (10, &vMuzzle, vTarget, NULL, -nObject - 1,
										 -5000, 0, CFixVector::Dist(vMuzzle, *vTarget), I2X (3), 0, 0, 100, 10, 1, 3, 1, 1,
#if OMEGA_PLASMA
										 (parentObjP != gameData.objs.viewerP) || gameStates.render.bFreeCam || gameStates.render.bChaseCam,
#else
										 0,
#endif
										 1, 1, -1, &color);
	}
return (handleP->nLightning >= 0);
}

//------------------------------------------------------------------------------
//eof
