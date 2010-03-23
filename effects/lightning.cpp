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

#define RENDER_LIGHTNING_OUTLINE 0

#define LIMIT_FLASH_FPS	1
#define FLASH_SLOWMO 1
#define PLASMA_WIDTH	3.0f
#define CORE_WIDTH 3.0f

#define STYLE	(((m_nStyle < 0) || (gameOpts->render.lightning.nStyle < m_nStyle)) ? \
					 gameOpts->render.lightning.nStyle : m_nStyle)

//------------------------------------------------------------------------------

CFixVector *VmRandomVector (CFixVector *vRand)
{
	CFixVector	vr;

do {
	vr [X] = (90 - d_rand () % 181) * (I2X (1) / 90);
	vr [Y] = (90 - d_rand () % 181) * (I2X (1) / 90);
	vr [Z] = (90 - d_rand () % 181) * (I2X (1) / 90);
} while (vr.IsZero ());
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
	} while (((nDot > nMaxDot) || (nDot < nMinDot)) && (++i < 10));
*vRand = vr;
return vRand;
}

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
		} while (CFixVector::Dot (vRefDir, vDir) < nMinDot);
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
if (m_bInPlane && !m_vDelta.IsZero ()) {
	vDelta [0] = m_vDelta;
	vDelta [1].SetZero ();
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
	l = m_nNodes * m_parent->m_nLength / m_parent->m_nNodes;
	m_nLength = ComputeChildEnd (&m_vPos, &m_vEnd, &m_vDir, &m_parent->m_vDir, l);// + 3 * l / (m_nNode + 1));
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
							  short nSmoothe, char bClamp, char bGlow, char bLight,
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
m_nChildren = (extraGameInfo [0].bUseLightning > 1) ? (nChildren < 0) ? nNodes / 10 : nChildren : 0;
if (vEnd) {
	m_vRefEnd = *vEnd;
	m_vEnd = *vEnd;
	}
if (vDelta)
	m_vDelta = *vDelta;
else
	m_vDelta.SetZero ();
m_bInPlane = !m_vDelta.IsZero ();
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
m_bGlow = bGlow;
m_iStep = 0;
m_color = *colorP;
m_vBase = *vPos;
m_bLight = bLight;
m_nStyle = nStyle;
}

//------------------------------------------------------------------------------

bool CLightning::Create (char nDepth, int nThread)
{
if ((m_nObject >= 0) && (0 > (m_nSegment = OBJECTS [m_nObject].info.nSegment)))
	return NULL;
if (!m_nodes.Create (m_nNodes))
	return false;
if (nDepth && (m_bGlow > 0))
	m_bGlow = 0;
if (gameOpts->render.lightning.bGlow) {
	int h = ((m_bGlow > 0) ? 2 : 1) * (m_nNodes - 1) * 4;
	if (!m_plasmaTexCoord.Create (h))
		return false;
	if (!m_plasmaVerts.Create (h))
		return false;
	}
if (!m_coreVerts.Create ((m_nNodes + 3) * 4))
	return false;
m_nodes.Clear ();
if (m_bRandom) {
	m_nTTL = 3 * m_nTTL / 4 + int (dbl_rand () * m_nTTL / 2);
	m_nLength = 3 * m_nLength / 4 + int (dbl_rand () * m_nLength / 2);
	}
if (m_nAmplitude < 0)
	m_nAmplitude = m_nLength / 6;
Setup (true);
if ((extraGameInfo [0].bUseLightning > 1) && nDepth && m_nChildren) {
	int			h, l, n, nNode, nChildNodes;
	double		nStep, j, scale;

	n = m_nChildren + 1 + (m_nChildren < 2);
	nStep = double (m_nNodes) / double (m_nChildren);
	for (h = m_nNodes - int (nStep), j = nStep / 2; j < h; j += nStep) {
		nNode = int (j) + 2 - d_rand () % 5;
		if (nNode < 1)
			nNode = int (j);
		if (!(nChildNodes = m_nNodes - nNode))
			continue;
		nChildNodes = 2 * nChildNodes / 4 + rand () % (nChildNodes / 4);
		scale = double (nChildNodes) / double (m_nNodes);
		l = int (m_nLength * scale + 0.5);
		if (!m_nodes [nNode].CreateChild (&m_vEnd, &m_vDelta, m_nLife, l, int (m_nAmplitude * scale + 0.5), m_nAngle,
													 nChildNodes, m_nChildren / 5, nDepth - 1, max (1, int (m_nSteps * scale + 0.5)), m_nSmoothe, m_bClamp, m_bGlow, m_bLight,
													 m_nStyle, &m_color, this, nNode, nThread))
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
	int i;
	for (i = abs (m_nNodes); i > 0; i--, nodeP++)
		nodeP->Destroy ();
	m_nodes.Destroy ();
	m_plasmaVerts.Destroy ();
	m_plasmaTexCoord.Destroy ();
	m_coreVerts.Destroy ();
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
		nMaxDist += (d_rand () % 4 + 1) * h / 4;
		for (i = m_nNodes - 1 - !m_bRandom, nodeP = m_nodes + 1; i > 0; i--, nodeP++)
			nodeP->m_vOffs *= FixDiv (nAmplitude, nMaxDist);
		}
	}
}

//------------------------------------------------------------------------------
// The end point of Perlin generated lightning do not necessarily lay at
// the origin. This function rotates the lightning path so that it does.

void CLightning::Rotate (int nSteps)
{
CFloatVector v0, v1, vBase;
v0.Assign (m_vEnd - m_vPos);
v1.Assign (m_nodes [m_nNodes - 1].m_vNewPos - m_vPos);
vBase.Assign (m_vPos);
float len0 = v0.Mag ();
float len1 = v1.Mag ();
len1 *= len1;
for (int i = 1; i < m_nNodes; i++) {
	m_nodes [i].Rotate (v0, len0, v1, len1, vBase, nSteps);
	}
}

//------------------------------------------------------------------------------
// Paths of lightning with style 1 isn't affected by amplitude. This function scales
// these paths with the amplitude.

void CLightning::Scale (int nSteps, int nAmplitude)
{
	fix	nOffset, nMaxOffset = 0;

for (int i = 1; i < m_nNodes; i++) {
	nOffset = m_nodes [i].Offset (m_vPos, m_vEnd);
	if (nMaxOffset < nOffset)
		nMaxOffset = nOffset;
	}

nAmplitude = 3 * nAmplitude / 4 + (rand () % (nAmplitude / 4));
CFloatVector vStart, vEnd;
vStart.Assign (m_vPos);
vEnd.Assign (m_vEnd);
float scale = X2F (nAmplitude) / X2F (nMaxOffset);

for (int i = 1; i < m_nNodes; i++)
	m_nodes [i].Scale (vStart, vEnd, scale, nSteps);

}

//------------------------------------------------------------------------------

void CLightning::CreatePath (int nDepth, int nThread)
{
	CLightningNode*	plh, * nodeP [2];
	int					h, i, j, nSteps, nStyle, nSmoothe, bClamp, nMinDist, nAmplitude, bPrevOffs [2] = {0,0};
	CFixVector			vPos [2], vBase [2], vPrevOffs [2];

	static int	nSeed [2];

vBase [0] = vPos [0] = m_vPos;
vBase [1] = vPos [1] = m_vEnd;
nStyle = STYLE;
nSteps = m_nSteps;
nSmoothe = m_nSmoothe;
bClamp = m_bClamp;
nAmplitude = m_nAmplitude;
plh = m_nodes.Buffer ();
plh->m_vNewPos = plh->m_vPos;
plh->m_vOffs.SetZero ();
if ((nDepth > 1) || m_bRandom) {
	if (nStyle == 2) {
		double h = double (m_nNodes - 1);
		perlinX [nThread].Setup (m_nNodes, 6, 1);
		perlinY [nThread].Setup (m_nNodes, 6, 1);
		for (i = 0; i < m_nNodes; i++)
			m_nodes [i].CreatePerlin (nAmplitude, double (i) / h, i, nThread);
		Rotate (nSteps);
		}
	else if (nStyle == 1) {
		nMinDist = m_nLength / (m_nNodes - 1);
		for (i = m_nNodes - 1, plh = m_nodes + 1; i > 0; i--, plh++, bPrevOffs [0] = 1)
			*vPrevOffs = plh->CreateJaggy (vPos, vPos + 1, vBase, bPrevOffs [0] ? vPrevOffs : NULL, nSteps, nAmplitude, nMinDist, i, nSmoothe, bClamp);
		Scale (nSteps, nAmplitude);
		}
	else {
		int bInPlane = m_bInPlane && (nDepth == 1); 
		if (bInPlane)
			nStyle = 0;
		nMinDist = m_nLength / (m_nNodes - 1);
		nAmplitude *= (nDepth == 1) ? 4 : 16;
		for (h = m_nNodes - 1, i = 0, plh = m_nodes + 1; i < h; i++, plh++) 
			plh->CreateErratic (vPos, vBase, nSteps, nAmplitude, 0, bInPlane, 1, i, h + 1, nSmoothe, bClamp);
		}
	}
else {
	plh = &m_nodes [m_nNodes - 1];
	plh->m_vNewPos = plh->m_vPos;
	plh->m_vOffs.SetZero ();
	if (nStyle == 2) {
		double h = double (m_nNodes - 1);
		perlinX [nThread].Setup (m_nNodes, 6, 1);
		perlinY [nThread].Setup (m_nNodes, 6, 1);
		for (i = 0, plh = m_nodes.Buffer (); i < m_nNodes; i++, plh++)
			plh->CreatePerlin (nAmplitude, double (i) / h, i, nThread);
		Rotate (nSteps);
		}
	else if (nStyle == 1) {
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
		Scale (nSteps, nAmplitude);
		}
	else {
		int bInPlane = m_bInPlane && (nDepth == 1); 
		if (bInPlane)
			nStyle = 0;
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
	}
if (nStyle < 2) {
	Smoothe ();
	ComputeOffsets ();
#if 0
	Bump ();
#endif
	}
}

//------------------------------------------------------------------------------

void CLightning::Animate (int nDepth, int nThread)
{
	CLightningNode	*nodeP;
	int				j;
	bool				bInit;

m_nTTL -= gameStates.app.tick40fps.nTime;
if (m_nNodes > 0) {
	if ((bInit = (m_nSteps < 0)))
		m_nSteps = -m_nSteps;
	if (!m_iStep) {
		CreatePath (nDepth + 1, nThread);
		m_iStep = m_nSteps;
		}
	for (j = m_nNodes - 1 - !m_bRandom, nodeP = m_nodes + 1; j > 0; j--, nodeP++)
		nodeP->Animate (bInit, m_nSegment, nDepth, nThread);
	RenderSetup (nDepth, nThread);
	(m_iStep)--;
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

int CLightning::Update (int nDepth, int nThread)
{
Animate (nDepth, nThread);
RenderSetup (nDepth, nThread);
return SetLife ();
}

//------------------------------------------------------------------------------

void CLightning::Move (int nDepth, CFixVector *vNewPos, short nSegment, bool bStretch, bool bFromEnd, int nThread)
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
		nodeP->Move (nDepth, vDelta, nSegment, nThread);
		}
	}
}

//------------------------------------------------------------------------------

inline int CLightning::MayBeVisible (int nThread)
{
if (m_nSegment >= 0)
	return SegmentMayBeVisible (m_nSegment, m_nLength / 20, 3 * m_nLength / 2, nThread);
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
// Compute billboards around each lightning segment using the normal of the plane
// spanned by the lightning segment and the vector from the camera (eye) position
// to one lightning coordinate
// vPosf: Coordinates of two subsequent lightning bolt segments

void CLightning::ComputeGlow (int nDepth, int nThread)
{

	CLightningNode*	nodeP = m_nodes.Buffer ();

if (!nodeP)
	return;

	CFloatVector*		srcP, * dstP, vEye, vn, vd, 
							vPos [2] = {CFloatVector::ZERO, CFloatVector::ZERO};
	tTexCoord2f*		texCoordP;
	int					h, i, j;
	bool					bGlow = !nDepth && (m_bGlow > 0) && gameOpts->render.lightning.bGlow;
	float					fWidth = bGlow ? PLASMA_WIDTH / 2.0f : (m_bGlow > 0) ? (PLASMA_WIDTH / 4.0f) : (m_bGlow < 0) ? (PLASMA_WIDTH / 16.0f) : (PLASMA_WIDTH / 8.0f);

if (nThread < 0)
	vEye.SetZero ();
else
	vEye.Assign (gameData.render.mine.viewerEye);
dstP = m_plasmaVerts.Buffer ();
texCoordP = m_plasmaTexCoord.Buffer ();
for (h = m_nNodes - 1, i = 0; i <= h; i++, nodeP++) {
	if (i >= h - 1)
		i = i;
	vPos [0] = vPos [1];
	vPos [1].Assign (nodeP->m_vPos);
	//if (nThread < 0)
	//	transformation.Transform (vPos [1], vPos [1]);
	if (i) {
		vn = CFloatVector::Normal (vPos [0], vPos [1], vEye);
		vn *= fWidth;
		if (i == 1) {
			vd = vPos [0] - vPos [1];
			vd *= PLASMA_WIDTH / 4.0f;
			vPos [0] += vd;
			}
		if (i == h) {
			vd = vPos [1] - vPos [0];
			vd = vd * PLASMA_WIDTH / 4.0f;
			vPos [1] += vd;
			}
		*dstP++ = vPos [0] + vn;
		*dstP++ = vPos [0] - vn;
		*dstP++ = vPos [1] - vn;
		*dstP++ = vPos [1] + vn;
		memcpy (texCoordP, plasmaTexCoord [0], 4 * sizeof (tTexCoord2f));
		texCoordP += 4;
		}
	}
memcpy (texCoordP - 4, plasmaTexCoord [2], 4 * sizeof (tTexCoord2f));
memcpy (&m_plasmaTexCoord [0], plasmaTexCoord [1], 4 * sizeof (tTexCoord2f));

dstP = m_plasmaVerts.Buffer ();
for (h = 4 * (m_nNodes - 2), i = 2, j = 4; i < h; i += 4, j += 4) {
	dstP [i+1] = dstP [j] = CFloatVector::Avg (dstP [i+1], dstP [j]);
	dstP [i] = dstP [j+1] = CFloatVector::Avg (dstP [i], dstP [j+1]);
	}

if (bGlow) {
	int h = 4 * (m_nNodes - 1);
	//for (j = 0; j < 2; j++) 
		{
		memcpy (texCoordP, texCoordP - h, h * sizeof (tTexCoord2f));
		texCoordP += h;
		srcP = dstP;
		dstP += h;
		for (i = 0; i < h; i += 2) {
			vPos [0] = CFloatVector::Avg (srcP [i], srcP [i+1]);
			vPos [1] = srcP [i] - srcP [i+1];
			vPos [1] /= 8;
			dstP [i] = vPos [0] + vPos [1];
			dstP [i+1] = vPos [0] - vPos [1];
			}
#if 0
		m_plasmaVerts [j+1][0] += (m_plasmaVerts [j+1][2] - m_plasmaVerts [j+1][0]) / 4;
		m_plasmaVerts [j+1][1] += (m_plasmaVerts [j+1][3] - m_plasmaVerts [j+1][1]) / 4;
		m_plasmaVerts [j+1][h-2] += (m_plasmaVerts [j+1][h-2] - m_plasmaVerts [j+1][h-4]) / 4;
		m_plasmaVerts [j+1][h-3] += (m_plasmaVerts [j+1][h-3] - m_plasmaVerts [j+1][h-1]) / 4;
#endif
		}
	}
}

//------------------------------------------------------------------------------

void CLightning::RenderSetup (int nDepth, int nThread)
{
if (gameOpts->render.lightning.bGlow)
	ComputeGlow (nDepth, nThread);
else
	ComputeCore ();
if (extraGameInfo [0].bUseLightning > 1)
	for (int i = 0; i < m_nNodes; i++)
		if (m_nodes [i].GetChild ())
			m_nodes [i].GetChild ()->RenderSetup (nDepth + 1, nThread);
}

//------------------------------------------------------------------------------

void CLightning::RenderGlow (tRgbaColorf *colorP, int nDepth, int nThread)
{
if (!m_plasmaVerts.Buffer ())
	return;

#if RENDER_LIGHTNING_OUTLINE
	tTexCoord2f*	texCoordP;
	CFloatVector*	vertexP;
#endif

OglTexCoordPointer (2, GL_FLOAT, 0, m_plasmaTexCoord.Buffer ());
OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), m_plasmaVerts.Buffer ());
ogl.SetBlendMode (1);
if (nDepth || (m_bGlow < 1)) {
	glColor3fv (reinterpret_cast<GLfloat*> (colorP));
	OglDrawArrays (GL_QUADS, 0, 4 * (m_nNodes - 1));
	}
else {
	int h = 4 * (m_nNodes - 1);
	glColor3f (colorP->red / 2.0f, colorP->green / 2.0f, colorP->blue / 2.0f);
	for (int i = 1; i >= 0; i--) {
		OglDrawArrays (GL_QUADS, i * h, h);
#if RENDER_LIGHTNING_OUTLINE
		if (i != 1)
			continue;
		ogl.SetTexturing (false);
		glColor3f (1,1,1);
		texCoordP = m_plasmaTexCoord.Buffer ();
		vertexP = m_plasmaVerts.Buffer ();
		for (int i = 0; i < m_nNodes - 1; i++) {
#if 1
			OglDrawArrays (GL_LINE_LOOP, i * 4, 4);
#else
			glBegin (GL_LINE_LOOP);
			for (int j = 0; j < 4; j++) {
				glTexCoord2fv (reinterpret_cast<GLfloat*> (texCoordP++));
				glVertex3fv (reinterpret_cast<GLfloat*> (vertexP++));
				}
			glEnd ();
#endif
			}
#endif
		}
	}
ogl.DisableClientStates (1, 0, 0, GL_TEXTURE0);
}

//------------------------------------------------------------------------------

void CLightning::ComputeCore (void)
{
	CFloatVector3*	vertP, * vPosf;
	int				i;

vPosf = vertP = &m_coreVerts [0];

for (i = 0; i < m_nNodes; i++, vPosf++)
	vPosf->Assign (m_nodes [i].m_vPos);

*vPosf = vertP [0] - vertP [1];
*vPosf /= 100.0f * vPosf->Mag ();
*vPosf += vertP [0];
*++vPosf = vertP [0];
i = m_nNodes - 1;
*++vPosf = vertP [i];
*++vPosf = vertP [i] - vertP [i - 1];
*vPosf /= 100.0f * vPosf->Mag ();
*vPosf += vertP [i];
}

//------------------------------------------------------------------------------

void CLightning::RenderCore (tRgbaColorf *colorP, int nDepth, int nThread)
{
if (!m_coreVerts.Buffer ())
	return;
ogl.SetBlendMode (1);
ogl.SetLineSmooth (true);
if (ogl.EnableClientStates (0, 0, 0, GL_TEXTURE0)) {
	ogl.SetTexturing (false);
	glColor4f (colorP->red / 2, colorP->green / 2, colorP->blue / 2, colorP->alpha);
	glLineWidth (GLfloat (nDepth ? CORE_WIDTH : CORE_WIDTH * 2));
	OglVertexPointer (3, GL_FLOAT, 0, m_coreVerts.Buffer ());
	OglDrawArrays (GL_LINE_STRIP, 0, m_nNodes);
	ogl.DisableClientStates (0, 0, 0, -1);
	}
#if GL_FALLBACK
else {
	ogl.SelectTMU (GL_TEXTURE0);
	ogl.SetTexturing (false);
	glBegin (GL_LINE_STRIP);
	for (i = m_nNodes, vPosf = coreBuffer [nThread]; i; i--, vPosf++)
		glVertex3fv (reinterpret_cast<GLfloat*> (vPosf));
	glEnd ();
	}
#endif
glLineWidth (GLfloat (1));
ogl.SetLineSmooth (false);
ogl.ClearError (0);
}

//------------------------------------------------------------------------------

int CLightning::SetupGlow (void)
{
if (!(/*m_bGlow &&*/ gameOpts->render.lightning.bGlow && ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0)))
	return 0;
ogl.SelectTMU (GL_TEXTURE0, true);
ogl.SetTexturing (true);
if (LoadCorona () && !bmpCorona->Bind (1)) {
	bmpCorona->Texture ()->Wrap (GL_CLAMP);
	return 1;
	}
ogl.DisableClientStates (1, 0, 0, GL_TEXTURE0);
return 0;
}

//------------------------------------------------------------------------------

#if !USE_OPENMP

static inline void WaitForRenderThread (int nThread)
{
if (gameStates.app.bMultiThreaded && (nThread > 0)) {	//thread 1 will always render after thread 0
	tiRender.ti [1].bBlock = 0;
	while (tiRender.ti [0].bBlock)
		G3_SLEEP (0);
	}
}

#endif

//------------------------------------------------------------------------------

void CLightning::Draw (int nDepth, int nThread)
{
	int				i, bGlow;
	tRgbaColorf		color;

if (!m_nodes.Buffer () || (m_nNodes <= 0) || (m_nSteps < 0))
	return;
#if 0 //!USE_OPENMP
if (gameStates.app.bMultiThreaded && (nThread > 0))
	tiRender.ti [nThread].bBlock = 1;
#endif
color = m_color;
if (m_nLife > 0) {
	if ((i = m_nLife - m_nTTL) < 250)
		color.alpha *= (float) i / 250.0f;
	else if (m_nTTL < m_nLife / 4)
		color.alpha *= (float) m_nTTL / (float) (m_nLife / 4);
	}
color.red *= (float) (0.9 + dbl_rand () / 5);
color.green *= (float) (0.9 + dbl_rand () / 5);
color.blue *= (float) (0.9 + dbl_rand () / 5);
if (!(bGlow = SetupGlow ()))
	color.alpha *= 1.5f;
if (nDepth)
	color.alpha /= 2;
#if 0 //!USE_OPENMP
WaitForRenderThread (nThread);
#endif
if (bGlow)
	RenderGlow (&color, nDepth, nThread);
else
	RenderCore (&color, nDepth, nThread);
#if 0 //!USE_OPENMP
WaitForRenderThread (nThread);
#endif
if (extraGameInfo [0].bUseLightning > 1)
		for (i = 0; i < m_nNodes; i++)
			if (m_nodes [i].GetChild ())
				m_nodes [i].GetChild ()->Draw (nDepth + 1, nThread);
ogl.ClearError (0);
}

//------------------------------------------------------------------------------

void CLightning::Render (int nDepth, int nThread)
{
if ((gameStates.render.nType != RENDER_TYPE_TRANSPARENCY) && (nThread >= 0)) {	// not in transparency renderer
	if ((m_nNodes < 0) || (m_nSteps < 0))
		return;
	if (!MayBeVisible (nThread))
		return;
#if 0
	if (!gameOpts->render.n3DGlasses) 
		RenderSetup (0, nThread);
#endif
	transparencyRenderer.AddLightning (this, nDepth);
	if (extraGameInfo [0].bUseLightning > 1)
		for (int i = 0; i < m_nNodes; i++)
			if (m_nodes [i].GetChild ())
				m_nodes [i].GetChild ()->Render (nDepth + 1, nThread);
	}
else {
	if (!nDepth)
		ogl.SetFaceCulling (false);
	if (nThread >= 0)
		ogl.SetupTransform (1);
	Draw (0, nThread);
	if (nThread >= 0)
		ogl.ResetTransform (1);
	if (!nDepth)
		ogl.SetFaceCulling (true);
	glLineWidth (1);
	ogl.SetLineSmooth (false);
	ogl.SetBlendMode (0);
	}
}

//------------------------------------------------------------------------------

int CLightning::SetLight (void)
{
	int		j, nLights = 0, nStride;
	double	h, nStep;

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
//eof
