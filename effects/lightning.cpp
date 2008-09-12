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

#define RENDER_TARGET_LIGHTNING 0
#define RENDER_LIGHTNING_PLASMA 1
#define RENDER_LIGHTING_SEGMENTS 0
#define RENDER_LIGHTNING_OUTLINE 0
#define RENDER_LIGHTINGS_BUFFERED 1
#define UPDATE_LIGHTNINGS 1

#define STYLE(_pl)	((((_pl)->nStyle < 0) || (gameOpts->render.lightnings.nStyle < (_pl)->nStyle)) ? \
							gameOpts->render.lightnings.nStyle : \
							(_pl)->nStyle)

void CreateLightningPath (tLightning *pl, int bSeed, int nDepth);
int UpdateLightning (tLightning *pl, int nLightnings, int nDepth);

//------------------------------------------------------------------------------

#ifdef _DEBUG

void TRAP (tLightningNode *pln)
{
if (pln->pChild == (tLightning *) (size_t) 0xfeeefeee)
	pln = pln;
}

void CHECK (tLightning *pl, int i)
{
	tLightningNode *pln;
	int j;

for (; i > 0; i--, pl++)
	if (pl->nNodes > 0)
		for (j = pl->nNodes, pln = pl->pNodes; j > 0; j--, pln++)
			TRAP (pln);
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

//	-----------------------------------------------------------------------------

void InitLightnings (void)
{
	int i, j;

for (i = 0, j = 1; j < MAX_LIGHTNINGS; i++, j++)
	gameData.lightnings.buffer [i].nNext = j;
gameData.lightnings.buffer [i].nNext = -1;
gameData.lightnings.iFree = 0;
gameData.lightnings.iUsed = -1;
gameData.lightnings.bDestroy = 0;
memset (gameData.lightnings.objects, 0xff, MAX_OBJECTS * sizeof (*gameData.lightnings.objects));
}

//------------------------------------------------------------------------------

vmsVector *VmRandomVector (vmsVector *vRand)
{
	vmsVector	vr;

do {
	vr[X] = F1_0 / 4 - d_rand ();
	vr[Y] = F1_0 / 4 - d_rand ();
	vr[Z] = F1_0 / 4 - d_rand ();
} while (!(vr[X] && vr[Y] && vr[Z]));
vmsVector::Normalize(vr);
*vRand = vr;
return vRand;
}

//------------------------------------------------------------------------------

#define SIGN(_i)	(((_i) < 0) ? -1 : 1)

#define VECSIGN(_v)	(SIGN ((_v)[X]) * SIGN ((_v)[Y]) * SIGN ((_v)[Z]))

//------------------------------------------------------------------------------

vmsVector *DirectedRandomVector (vmsVector *vRand, vmsVector *vDir, int nMinDot, int nMaxDot)
{
	vmsVector	vr, vd = *vDir, vSign;
	int			nDot, nSign, i = 0;

vmsVector::Normalize(vd);
vSign[X] = vd[X] ? vd[X] / abs(vd[X]) : 0;
vSign[Y] = vd[Y] ? vd[Y] / abs(vd[Y]) : 0;
vSign[Z] = vd[Z] ? vd[Z] / abs(vd[Z]) : 0;
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

int ComputeChildEnd (vmsVector *vPos, vmsVector *vEnd, vmsVector *vDir, vmsVector *vParentDir, int nLength)
{
nLength = 3 * nLength / 4 + (int) (dbl_rand () * nLength / 4);
DirectedRandomVector (vDir, vParentDir, 3 * F1_0 / 4, 9 * F1_0 / 10);
*vEnd = *vPos + *vDir * nLength;
return nLength;
}

//------------------------------------------------------------------------------

void SetupLightning (tLightning *pl, int bInit)
{
	tLightning		*pp;
	tLightningNode	*pln;
	vmsVector		vPos, vDir, vRefDir, vDelta [2], v;
	int				i = 0, l;

pl->vPos = pl->vBase;
if (pl->bRandom) {
	if (!pl->nAngle)
		VmRandomVector (&vDir);
	else {
		int nMinDot = F1_0 - pl->nAngle * F1_0 / 90;
		vRefDir = pl->vRefEnd - pl->vPos;
		vmsVector::Normalize(vRefDir);
		do {
			VmRandomVector (&vDir);
		}
		while (vmsVector::Dot(vRefDir, vDir) < nMinDot);
	}
	pl->vEnd = pl->vPos + vDir * pl->nLength;
}
else {
	vDir = pl->vEnd - pl->vPos;
	vmsVector::Normalize(vDir);
	}
pl->vDir = vDir;
if (pl->nOffset) {
	i = pl->nOffset / 2 + (int) (dbl_rand () * pl->nOffset / 2);
	pl->vPos += vDir * i;
	pl->vEnd += vDir * i;
	}
vPos = pl->vPos;
if (pl->bInPlane) {
	vDelta[0] = pl->vDelta;
	vDelta[i].SetZero();
	}
else {
	do {
		VmRandomVector(&vDelta[0]);
	}
	while (abs (vmsVector::Dot(vDir, vDelta[0])) > 9 * F1_0 / 10);
	vDelta[1] = vmsVector::Normal(vPos, pl->vEnd, *vDelta);
	v = vPos + vDelta[1];
	vDelta[0] = vmsVector::Normal(vPos, pl->vEnd, v);
	}
vDir *= FixDiv(pl->nLength, (pl->nNodes - 1) * F1_0);
pl->nNodes = abs (pl->nNodes);
pl->iStep = 0;
if ((pp = pl->pParent)) {
	i = pp->nChildren + 1;
	l = pp->nLength / i;
	pl->nLength = ComputeChildEnd (&pl->vPos, &pl->vEnd, &pl->vDir, &pp->vDir, l + 3 * l / (pl->nNode + 1));
	vDir = pl->vDir * (pl->nLength / (pl->nNodes - 1));
	}
for (i = pl->nNodes, pln = pl->pNodes; i > 0; i--, pln++) {
	pln->vBase =
	pln->vPos = vPos;
	pln->vOffs.SetZero();
	memcpy (pln->vDelta, vDelta, sizeof (vDelta));
	vPos += vDir;
	if (bInit)
		pln->pChild = NULL;
	else if (pln->pChild)
		SetupLightning (pln->pChild, 0);
	}
pl->nSteps = -abs (pl->nSteps);
//(pln - 1)->vPos = pl->vEnd;
}

//------------------------------------------------------------------------------

tLightning *AllocLightning (int nLightnings, vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
									 short nObject, int nLife, int nDelay, int nLength, int nAmplitude,
									 char nAngle, int nOffset, short nNodes, short nChildren, char nDepth, short nSteps,
									 short nSmoothe, char bClamp, char bPlasma, char bLight,
									 char nStyle, tRgbaColorf *colorP, tLightning *pParent, short nNode)
{
	tLightning		*pfRoot, *pl;
	tLightningNode	*pln;
	int				h, i, bRandom = (vEnd == NULL) || (nAngle > 0);

if (!(nLife && nLength && (nNodes > 4)))
	return NULL;
#if 0
if (!bRandom)
	nLightnings = 1;
#endif
if (!(pfRoot = (tLightning *) D2_ALLOC (nLightnings * sizeof (tLightning))))
	return NULL;
for (i = nLightnings, pl = pfRoot; i > 0; i--, pl++) {
	if (!(pl->pNodes = (tLightningNode *) D2_ALLOC (nNodes * sizeof (tLightningNode)))) {
		while (++i < nLightnings)
			D2_FREE (pl->pNodes);
		D2_FREE (pfRoot);
		return NULL;
		}
	pl->nIndex = gameData.lightnings.iFree;
	if (nObject < 0) {
		pl->nObject = -1;
		pl->nSegment = -nObject - 1;
		}
	else {
		pl->nObject = nObject;
		if (0 > (pl->nSegment = OBJECTS [nObject].nSegment))
			return NULL;
		}
	pl->pParent = pParent;
	pl->nNode = nNode;
	pl->nNodes = nNodes;
	pl->nChildren = gameOpts->render.lightnings.nQuality ? (nChildren < 0) ? nNodes / 10 : nChildren : 0;
	pl->nLife = nLife;
	h = abs (nLife);
	pl->nTTL = (bRandom) ? 3 * h / 4 + (int) (dbl_rand () * h / 2) : h;
	pl->nDelay = abs (nDelay) * 10;
	pl->nLength = (bRandom) ? 3 * nLength / 4 + (int) (dbl_rand () * nLength / 2) : nLength;
	pl->nAmplitude = (nAmplitude < 0) ? nLength / 6 : nAmplitude;
	pl->nAngle = vEnd ? nAngle : 0;
	pl->nOffset = nOffset;
	pl->nSteps = -nSteps;
	pl->nSmoothe = nSmoothe;
	pl->bClamp = bClamp;
	pl->bPlasma = bPlasma;
	pl->iStep = 0;
	pl->color = *colorP;
	pl->vBase = *vPos;
	pl->bLight = bLight;
	pl->nStyle = nStyle;
	pl->nNext = -1;
	if (vEnd)
		pl->vRefEnd = *vEnd;
	if (!(pl->bRandom = bRandom))
		pl->vEnd = *vEnd;
	if ((pl->bInPlane = (vDelta != NULL)))
		pl->vDelta = *vDelta;
	SetupLightning (pl, 1);
	if (gameOpts->render.lightnings.nQuality && nDepth && nChildren) {
		tLightningNode *plh;
		vmsVector		vEnd;
		int				l, n, nNode;
		double			nStep, j;

		n = nChildren + 1 + (nChildren < 2);
		nStep = (double) pl->nNodes / (double) nChildren;
		l = nLength / n;
		plh = pl->pNodes;
#if 0	//children have completely Random nodes
		for (h = pl->nNodes - (int) nStep, nNode = (int) (nStep / 2 + 0.5); (nNode < h) && nChildren; nNode++) {
			if (d_rand () % (int) nStep)
				continue;
			nChildren--;
			pln = plh + nNode;
#else //children are about nStep nodes away from each other
		for (h = pl->nNodes - (int) nStep, j = nStep / 2; j < h; j += nStep) {
			nNode = (int) j + 2 - d_rand () % 5;
			if (nNode < 1)
				nNode = (int) j;
			pln = plh + nNode;
#endif
			pln->pChild = AllocLightning (1, &pln->vPos, &vEnd, vDelta, -1, pl->nLife, 0, l, nAmplitude / n * 2, nAngle, 0,
													2 * pl->nNodes / n, nChildren / 5, nDepth - 1, nSteps / 2, nSmoothe, bClamp, bPlasma, bLight,
													nStyle, colorP, pl, nNode);
			}
		}
	}
return pfRoot;
}

//------------------------------------------------------------------------------

void CreateLightningSound (tLightningBundle *plb, int bSound)
{
if ((plb->bSound = bSound)) {
	DigiSetObjectSound (plb->nObject, -1, AddonSoundName (SND_ADDON_LIGHTNING));
	if (plb->bForcefield) {
		if (0 <= (plb->nSound = DigiGetSoundByName ("ff_amb_1")))
			DigiSetObjectSound (plb->nObject, plb->nSound, NULL);
		}
	}
else
	plb->nSound = -1;
}

//------------------------------------------------------------------------------

int CreateLightning (int nLightnings, vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
							short nObject, int nLife, int nDelay, int nLength, int nAmplitude,
							char nAngle, int nOffset, short nNodes, short nChildren, char nDepth, short nSteps,
							short nSmoothe, char bClamp, char bPlasma, char bSound, char bLight,
							char nStyle, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNINGS && colorP) {
		tLightningBundle	*plb;
		tLightning			*pl;
		int					n;

	if (!nLightnings)
		return -1;
	if (gameData.lightnings.iFree < 0)
		return -1;
	SEM_ENTER (SEM_LIGHTNINGS)
	srand (gameStates.app.nSDLTicks);
	if (!(pl = AllocLightning (nLightnings, vPos, vEnd, vDelta, nObject, nLife, nDelay, nLength, nAmplitude,
										nAngle, nOffset, nNodes, nChildren, nDepth, nSteps, nSmoothe, bClamp, bPlasma, bLight,
										nStyle, colorP, NULL, -1))) {
		SEM_LEAVE (SEM_LIGHTNINGS)
		return -1;
		}
	n = gameData.lightnings.iFree;
	plb = gameData.lightnings.buffer + n;
	gameData.lightnings.iFree = plb->nNext;
	plb->nNext = gameData.lightnings.iUsed;
	gameData.lightnings.iUsed = n;
	plb->pl = pl;
	plb->nLightnings = nLightnings;
	plb->nObject = nObject;
	plb->bForcefield = !nDelay && (vEnd || (nAngle <= 0));
	CreateLightningSound (plb, bSound);
	plb->tUpdate = -1;
	plb->nKey [0] =
	plb->nKey [1] = 0;
	plb->bDestroy = 0;
	CHECK (pl, nLightnings);
	SEM_LEAVE (SEM_LIGHTNINGS)
	}
return gameData.lightnings.iUsed;
}

//------------------------------------------------------------------------------

int IsUsedLightning (int iLightning)
{
	int	i;

if (iLightning < 0)
	return 0;
for (i = gameData.lightnings.iUsed; i >= 0; i = gameData.lightnings.buffer [i].nNext)
	if (iLightning == i)
		return 1;
return 0;
}

//------------------------------------------------------------------------------

tLightningBundle *PrevLightning (int iLightning)
{
	int	i, j;

if (iLightning < 0)
	return NULL;
for (i = gameData.lightnings.iUsed; i >= 0; i = j)
	if ((j = gameData.lightnings.buffer [i].nNext) == iLightning)
		return gameData.lightnings.buffer + i;
return NULL;
}

//------------------------------------------------------------------------------

void DestroyLightningNodes (tLightning *pl)
{
if (pl && pl->pNodes) {
		tLightningNode	*pln;
		int				i;

	for (i = abs (pl->nNodes), pln = pl->pNodes; i > 0; i--, pln++) {
		if (pln->pChild) {
			if ((int) (size_t) pln->pChild == (int) 0xffffffff)
				pln->pChild = NULL;
			else {
				DestroyLightningNodes (pln->pChild);
				D2_FREE (pln->pChild);
				}
			}
		}
	D2_FREE (pl->pNodes);
	pl->nNodes = 0;
	}
}

//------------------------------------------------------------------------------

void DestroyLightningSound (tLightningBundle *plb)
{
if ((plb->bSound > 0) & (plb->nObject >= 0))
	DigiKillSoundLinkedToObject (plb->nObject);
}

//------------------------------------------------------------------------------

void DestroyLightnings (int iLightning, tLightning *pl, int bDestroy)
{
	tLightningBundle	*plh, *plb = NULL;
	int					i;

if (pl)
	i = 1;
else {
	if (!IsUsedLightning (iLightning))
		return;
	plb = gameData.lightnings.buffer + iLightning;
	if (!bDestroy) {
		plb->bDestroy = 1;
		return;
		}
	pl = plb->pl;
	i = plb->nNext;
	if (gameData.lightnings.iUsed == iLightning)
		gameData.lightnings.iUsed = i;
	plb->nNext = gameData.lightnings.iFree;
	if ((plh = PrevLightning (iLightning)))
		plh->nNext = i;
	gameData.lightnings.iFree = iLightning;
	if ((plb->nObject >= 0) && (gameData.lightnings.objects [plb->nObject] == iLightning))
		gameData.lightnings.objects [plb->nObject] = -1;
	DestroyLightningSound (plb);
	i = plb->nLightnings;
	}
for (; i > 0; i--, pl++)
	DestroyLightningNodes (pl);
if (plb) {
	plb->nLightnings = 0;
	D2_FREE (plb->pl);
	}
}

//------------------------------------------------------------------------------

int DestroyAllLightnings (int bForce)
{
	int	i, j;

if (!bForce && (gameData.lightnings.bDestroy >= 0))
	gameData.lightnings.bDestroy = 1;
else {
	uint bSem = gameData.app.semaphores [SEM_LIGHTNINGS];
	if (!bSem)
		SEM_ENTER (SEM_LIGHTNINGS)
	for (i = gameData.lightnings.iUsed; i >= 0; i = j) {
		j = gameData.lightnings.buffer [i].nNext;
		DestroyLightnings (i, NULL, 1);
		}
	ResetLightningLights (1);
	InitLightnings ();
	if (!bSem)
		SEM_LEAVE (SEM_LIGHTNINGS)
	}
return 1;
}

//------------------------------------------------------------------------------

int ClampLightningPathPoint (vmsVector *vPos, vmsVector *vBase, int nAmplitude)
{
	vmsVector	vRoot;
	int			nDist = VmPointLineIntersection(vRoot, vBase[0], vBase[1], *vPos, 0);

if (nDist < nAmplitude)
	return nDist;
*vPos -= vRoot;	//create vector from intersection to current path point
*vPos *= FixDiv(nAmplitude, nDist);	//scale down to length nAmplitude
*vPos += vRoot;	//recalculate path point
return nAmplitude;
}

//------------------------------------------------------------------------------

int ComputeAttractor (vmsVector *vAttract, vmsVector *vDest, vmsVector *vPos, int nMinDist, int i)
{
	int nDist;

*vAttract = *vDest - *vPos;
nDist = vAttract->Mag() / i;
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

vmsVector *CreateLightningPathPoint (vmsVector *vOffs, vmsVector *vAttract, int nDist)
{
	vmsVector	va = *vAttract;
	int			nDot, i = 0;

if (nDist < F1_0 / 16)
	return VmRandomVector (vOffs);
vmsVector::Normalize(va);
if (!(va[X] && va[Y] && va[Z]))
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
	vOffs->Neg();
return vOffs;
}

//------------------------------------------------------------------------------

vmsVector *SmootheLightningOffset (vmsVector *vOffs, vmsVector *vPrevOffs, int nDist, int nSmoothe)
{
if (nSmoothe) {
		int nMag = vOffs->Mag();

	if (nSmoothe > 0)
		*vOffs *= FixDiv(nSmoothe * nDist, nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
	else
		*vOffs *= FixDiv(nDist, nSmoothe * nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
	nMag = vPrevOffs->Mag();
	*vOffs += *vPrevOffs;
	nMag = vOffs->Mag();
	*vOffs *= FixDiv(nDist, nMag);
	nMag = vOffs->Mag();
	}
return vOffs;
}

//------------------------------------------------------------------------------

vmsVector *AttractLightningPathPoint (vmsVector *vOffs, vmsVector *vAttract, vmsVector *vPos, int nDist, int i, int bJoinPaths)
{
	int nMag = vOffs->Mag();
// attract offset vector by scaling it with distance from attracting node
*vOffs *= FixDiv(i * nDist / 2, nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
*vOffs += *vAttract;	//add offset and attractor vectors (attractor is the bigger the closer)
nMag = vOffs->Mag();
*vOffs *= FixDiv(bJoinPaths ? nDist / 2 : nDist, nMag);	//rescale to desired path length
*vPos += *vOffs;
return vPos;
}

//------------------------------------------------------------------------------

vmsVector ComputeJaggyNode (tLightningNode *pln, vmsVector *vPos, vmsVector *vDest, vmsVector *vBase, vmsVector *vPrevOffs,
										  int nSteps, int nAmplitude, int nMinDist, int i, int nSmoothe, int bClamp)
{
	vmsVector	vAttract, vOffs;
	int			nDist = ComputeAttractor (&vAttract, vDest, vPos, nMinDist, i);

TRAP (pln);
CreateLightningPathPoint (&vOffs, &vAttract, nDist);
if (vPrevOffs)
	SmootheLightningOffset (&vOffs, vPrevOffs, nDist, nSmoothe);
else if (pln->vOffs[X] || pln->vOffs[Z] || pln->vOffs[Z]) {
	vOffs += pln->vOffs * (2 * F1_0);
	vOffs /= 3;
	}
if (nDist > F1_0 / 16)
	AttractLightningPathPoint (&vOffs, &vAttract, vPos, nDist, i, 0);
if (bClamp)
	ClampLightningPathPoint (vPos, vBase, nAmplitude);
pln->vNewPos = *vPos;
pln->vOffs = pln->vNewPos - pln->vPos;
pln->vOffs *= (F1_0 / nSteps);
return vOffs;
}

//------------------------------------------------------------------------------

vmsVector ComputeErraticNode (tLightningNode *pln, vmsVector *vPos, vmsVector *vBase, int nSteps, int nAmplitude,
										int bInPlane, int bFromEnd, int bRandom, int i, int nNodes, int nSmoothe, int bClamp)
{
	int	h, j, nDelta;

TRAP (pln);
pln->vNewPos = pln->vBase;
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
				nDelta += (h + 1) * (pln + h)->nDelta [j];
		}
	else {
		for (h = 1; h <= 2; h++)
			if (i - h > 0)
				nDelta += (h + 1) * (pln - h)->nDelta [j];
		}
	nDelta /= 6;
	pln->nDelta [j] = nDelta;
	pln->vNewPos += pln->vDelta[j] * nDelta;
	}
pln->vOffs = pln->vNewPos - pln->vPos;
pln->vOffs *= (F1_0 / nSteps);
if (bClamp)
	ClampLightningPathPoint (vPos, vBase, nAmplitude);
return pln->vOffs;
}

//------------------------------------------------------------------------------

vmsVector ComputePerlinNode (tLightningNode *pln, int nSteps, int nAmplitude, int *nSeed, double phi, double i)
{
double dx = PerlinNoise1D (i, 0.25, 6, nSeed [0]);
double dy = PerlinNoise1D (i, 0.25, 6, nSeed [1]);
TRAP (pln);
phi = sin (phi * Pi);
phi = sqrt (phi);
dx *= nAmplitude * phi;
dy *= nAmplitude * phi;
pln->vNewPos = pln->vBase + pln->vDelta[0] * ((int)dx);
pln->vNewPos += pln->vDelta[1] * ((int) dy);
pln->vOffs = pln->vNewPos - pln->vPos;
pln->vOffs *= (F1_0 / nSteps);
return pln->vOffs;
}

//------------------------------------------------------------------------------

void SmootheLightningPath (tLightning *pl)
{
	tLightningNode	*plh, *pfi, *pfj;
	int			i, j;

for (i = pl->nNodes - 1, j = 0, pfi = pl->pNodes, plh = NULL; j < i; j++) {
	pfj = plh;
	plh = pfi++;
	if (j) {
		plh->vNewPos[X] = pfj->vNewPos[X] / 4 + plh->vNewPos[X] / 2 + pfi->vNewPos[X] / 4;
		plh->vNewPos[Y] = pfj->vNewPos[Y] / 4 + plh->vNewPos[Y] / 2 + pfi->vNewPos[Y] / 4;
		plh->vNewPos[Z] = pfj->vNewPos[Z] / 4 + plh->vNewPos[Z] / 2 + pfi->vNewPos[Z] / 4;
		}
	}
}

//------------------------------------------------------------------------------

void ComputeNodeOffsets (tLightning *pl)
{
	tLightningNode	*pln;
	int			i, nSteps = pl->nSteps;

if (pl->nNodes > 0) {
	for (i = pl->nNodes - 1 - !pl->bRandom, pln = pl->pNodes + 1; i > 0; i--, pln++) {
		pln->vOffs = pln->vNewPos - pln->vPos;
		pln->vOffs *= (F1_0 / nSteps);
		}
	}
}

//------------------------------------------------------------------------------
// Make sure max. amplitude is reached every once in a while

void BumpLightningPath (tLightning *pl)
{
	tLightningNode	*pln;
	int			h, i, nSteps, nDist, nAmplitude, nMaxDist = 0;
	vmsVector	vBase [2];

nSteps = pl->nSteps;
nAmplitude = pl->nAmplitude;
vBase [0] = pl->vPos;
vBase [1] = pl->pNodes [pl->nNodes - 1].vPos;
for (i = pl->nNodes - 1 - !pl->bRandom, pln = pl->pNodes + 1; i > 0; i--, pln++) {
	nDist = vmsVector::Dist(pln->vNewPos, pln->vPos);
	if (nMaxDist < nDist) {
		nMaxDist = nDist;
		h = i;
		}
	}
if ((h = nAmplitude - nMaxDist)) {
	if (pl->nNodes > 0) {
		nMaxDist += (d_rand () % 5) * h / 4;
		for (i = pl->nNodes - 1 - !pl->bRandom, pln = pl->pNodes + 1; i > 0; i--, pln++)
			pln->vOffs *= FixDiv(nAmplitude, nMaxDist);
		}
	}
}

//------------------------------------------------------------------------------

void CreateLightningPath (tLightning *pl, int bSeed, int nDepth)
{
	tLightningNode	*plh, *pln [2];
	int			h, i, j, nSteps, nStyle, nSmoothe, bClamp, bInPlane, nMinDist, nAmplitude, bPrevOffs [2] = {0,0};
	vmsVector	vPos [2], vBase [2], vPrevOffs [2];
	double		phi;

	static int	nSeed [2];

vBase [0] = vPos [0] = pl->vPos;
vBase [1] = vPos [1] = pl->vEnd;
if (bSeed) {
	nSeed [0] = d_rand ();
	nSeed [1] = d_rand ();
	nSeed [0] *= d_rand ();
	nSeed [1] *= d_rand ();
	}
#ifdef _DEBUG
else
	bSeed = 0;
#endif
nStyle = STYLE (pl);
nSteps = pl->nSteps;
nSmoothe = pl->nSmoothe;
bClamp = pl->bClamp;
bInPlane = pl->bInPlane && ((nDepth == 1) || (nStyle == 2));
nAmplitude = pl->nAmplitude;
plh = pl->pNodes;
plh->vNewPos = plh->vPos;
plh->vOffs.SetZero();
if ((nDepth > 1) || pl->bRandom) {
	if (nStyle == 2) {
		if (nDepth > 1)
			nAmplitude *= 4;
		for (h = pl->nNodes, i = 0, plh = pl->pNodes; i < h; i++, plh++) {
			phi = bClamp ?(double) i / (double) (h - 1) : 1;
			ComputePerlinNode (plh, nSteps, nAmplitude, nSeed, 2 * phi / 3, phi * 7.5);
			TRAP (plh);
			}
		}
	else {
		if (pl->bInPlane)
			nStyle = 0;
		nMinDist = pl->nLength / (pl->nNodes - 1);
		if (!nStyle) {
			nAmplitude *= (nDepth == 1) ? 4 : 16;
			for (h = pl->nNodes - 1, i = 0, plh = pl->pNodes + 1; i < h; i++, plh++, bPrevOffs [0] = 1) {
				ComputeErraticNode (plh, vPos, vBase, nSteps, nAmplitude, 0, bInPlane, 1, i, h + 1, nSmoothe, bClamp);
				TRAP (plh);
				}
			}
		else {
			for (i = pl->nNodes - 1, plh = pl->pNodes + 1; i > 0; i--, plh++, bPrevOffs [0] = 1) {
				*vPrevOffs = ComputeJaggyNode (plh, vPos, vPos + 1, vBase, bPrevOffs [0] ? vPrevOffs : NULL, nSteps, nAmplitude, nMinDist, i, nSmoothe, bClamp);
				TRAP (plh);
				}
			}
		}
	}
else {
	plh = pl->pNodes + pl->nNodes - 1;
	plh->vNewPos = plh->vPos;
	plh->vOffs.SetZero();
	if (nStyle == 2) {
		nAmplitude = 5 * nAmplitude / 3;
		for (h = pl->nNodes, i = 0, plh = pl->pNodes; i < h; i++, plh++) {
			phi = bClamp ? (double) i / (double) (h - 1) : 1;
			ComputePerlinNode (plh, nSteps, nAmplitude, nSeed, phi, phi * 10);
			}
		}
	else {
		if (pl->bInPlane)
			nStyle = 0;
		if (!nStyle) {
			nAmplitude *= 4;
			for (h = pl->nNodes - 1, i = j = 0, pln [0] = pl->pNodes + 1, pln [1] = pl->pNodes + h - 1; i < h; i++, j = !j) {
				plh = pln [j];
				ComputeErraticNode (plh, vPos + j, vBase, nSteps, nAmplitude, bInPlane, j, 0, i, h, nSmoothe, bClamp);
				if (pln [1] <= pln [0])
					break;
				TRAP (pln [0]);
				TRAP (pln [1]);
				if (j)
					pln [1]--;
				else
					pln [0]++;
				}
			}
		else {
			for (i = pl->nNodes - 1, j = 0, pln [0] = pl->pNodes + 1, pln [1] = pl->pNodes + i - 1; i > 0; i--, j = !j) {
				plh = pln [j];
				vPrevOffs [j] = ComputeJaggyNode (plh, vPos + j, vPos + !j, vBase, bPrevOffs [j] ? vPrevOffs + j : NULL, nSteps, nAmplitude, 0, i, nSmoothe, bClamp);
				bPrevOffs [j] = 1;
				if (pln [1] <= pln [0])
					break;
				TRAP (pln [0]);
				TRAP (pln [1]);
				if (j)
					pln [1]--;
				else
					pln [0]++;
				}
			}
		}
	}
if (nStyle < 2) {
	SmootheLightningPath (pl);
	ComputeNodeOffsets (pl);
	BumpLightningPath (pl);
	}
}

//------------------------------------------------------------------------------

void AnimateLightning (tLightning *pl, int nStart, int nLightnings, int nDepth)
{
	tLightningNode	*pln;
	int				i, j, bSeed, bInit;

for (i = nStart, pl += nStart; i < nLightnings; i++, pl++) {
#if UPDATE_LIGHTNINGS
	pl->nTTL -= gameStates.app.tick40fps.nTime;
#endif
	if (pl->nNodes > 0) {
		if ((bInit = (pl->nSteps < 0)))
			pl->nSteps = -pl->nSteps;
		if (!pl->iStep) {
			bSeed = 1;
			CreateLightningPath (pl, bSeed, nDepth + 1);
			CHECK (pl, 1);
			pl->iStep = pl->nSteps;
			}
		for (j = pl->nNodes - 1 - !pl->bRandom, pln = pl->pNodes + 1; j > 0; j--, pln++) {
			TRAP (pln);
			if (bInit)
				pln->vPos = pln->vNewPos;
#if UPDATE_LIGHTNINGS
			else
				pln->vPos += pln->vOffs;
#endif
			if (pln->pChild) {
				MoveLightnings (1, pln->pChild, &pln->vPos, pl->nSegment, 0, 0);
				AnimateLightning (pln->pChild, 0, 1, nDepth + 1);
				}
			}
		CHECK (pl, 1);
#if UPDATE_LIGHTNINGS
		(pl->iStep)--;
#endif
		}
	}
}

//------------------------------------------------------------------------------

int SetLightningLife (tLightning *pl, int nLightnings)
{
	tLightning	*plRoot = pl;
	int			h, i;

for (i = 0; i < nLightnings; i++, pl++) {
	if (pl->nTTL <= 0) {
		if (pl->nLife < 0) {
			if (0 > (pl->nNodes = -pl->nNodes)) {
				h = pl->nDelay / 2;
				pl->nTTL = h + (int) (dbl_rand () * h);
				}
			else {
				if (pl->bRandom) {
					h = -pl->nLife;
					pl->nTTL = 3 * h / 4 + (int) (dbl_rand () * h / 2);
					SetupLightning (pl, 0);
					CHECK (pl, 1);
					}
				else {
					pl->nTTL = -pl->nLife;
					pl->nNodes = abs (pl->nNodes);
					SetupLightning (pl, 0);
					CHECK (pl, 1);
					}
				}
			}
		else {
			DestroyLightningNodes (pl);
			CHECK (pl, 1);
			if (!--nLightnings)
				return 0;
			if (i < nLightnings) {
				*pl = plRoot [nLightnings];
				CHECK (pl, 1);
				pl--;
				i--;
				}
			}
		}
	}
return nLightnings;
}

//------------------------------------------------------------------------------

int UpdateLightning (tLightning *pl, int nLightnings, int nDepth)
{
if (!(pl && nLightnings))
	return 0;
CHECK (pl, nLightnings);
AnimateLightning (pl, 0, nLightnings, nDepth);
CHECK (pl, nLightnings);
return SetLightningLife (pl, nLightnings);
}

//------------------------------------------------------------------------------

void UpdateLightningSound (tLightningBundle *plb)
{
	tLightning	*pl;
	int			i;

if (!plb->bSound)
	return;
for (i = plb->nLightnings, pl = plb->pl; i > 0; i--, pl++)
	if (pl->nNodes > 0) {
		if (plb->bSound < 0)
			CreateLightningSound (plb, 1);
		return;
		}
if (plb->bSound < 0)
	return;
DestroyLightningSound (plb);
plb->bSound = -1;
}

//------------------------------------------------------------------------------

void MoveObjectLightnings (tObject *objP)
{
MoveLightnings (gameData.lightnings.objects [OBJ_IDX (objP)], NULL, &OBJPOS (objP)->vPos, objP->nSegment, 0, 0);
}

//------------------------------------------------------------------------------

tRgbaColorf *LightningColor (tObject *objP)
{
if (objP->nType == OBJ_ROBOT) {
	if (ROBOTINFO (objP->id).energyDrain) {
		static tRgbaColorf color = {1.0f, 0.8f, 0.3f, 0.2f};
		return &color;
		}
	}
else if ((objP->nType == OBJ_PLAYER) && gameOpts->render.lightnings.bPlayers) {
	int s = gameData.segs.segment2s [objP->nSegment].special;
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

#define LIMIT_FLASH_FPS	1
#define FLASH_SLOWMO 1

void UpdateLightnings (void)
{
if (SHOW_LIGHTNINGS) {

		tLightningBundle	*plb;
		tObject				*objP;
		ubyte					h;
		int					i, n;

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
			DestroyObjectLightnings (objP);
			}
		}
	for (i = gameData.lightnings.iUsed; i >= 0; i = n) {
		plb = gameData.lightnings.buffer + i;
		n = plb->nNext;
		if (gameStates.app.nSDLTicks - plb->tUpdate < 25)
			continue;
		plb->tUpdate = gameStates.app.nSDLTicks;
		if (plb->bDestroy)
			DestroyLightnings (i, NULL, 1);
		else if (!(plb->nKey [0] || plb->nKey [1])) {
			if (!(plb->nLightnings = UpdateLightning (plb->pl, plb->nLightnings, 0)))
				DestroyLightnings (i, NULL, 1);
			else if (plb->nObject >= 0) {
				UpdateLightningSound (plb);
				MoveObjectLightnings (OBJECTS + plb->nObject);
				}
			}
		}

	SEM_LEAVE (SEM_LIGHTNINGS)

	for (i = 0, objP = OBJECTS; i < gameData.objs.nLastObject [0]; i++, objP++) {
		h = gameData.objs.bWantEffect [i];
		if (h & EXPL_LIGHTNINGS) {
			if ((objP->nType == OBJ_ROBOT) || (objP->nType == OBJ_REACTOR))
				CreateBlowupLightnings (objP);
			else if (objP->nType != 255)
				PrintLog ("invalid effect requested\n");
			}
		else if (h & MISSILE_LIGHTNINGS) {
			if ((objP->nType == OBJ_WEAPON) || gameData.objs.bIsMissile [objP->id])
				CreateMissileLightnings (objP);
			else if (objP->nType != 255)
				PrintLog ("invalid effect requested\n");
			}
		else if (h & ROBOT_LIGHTNINGS) {
			if (objP->nType == OBJ_ROBOT)
				CreateRobotLightnings (objP, LightningColor (objP));
			else if (objP->nType != 255)
				PrintLog ("invalid effect requested\n");
			}
		else if (h & PLAYER_LIGHTNINGS) {
			if (objP->nType == OBJ_PLAYER)
				CreatePlayerLightnings (objP, LightningColor (objP));
			else if (objP->nType != 255)
				PrintLog ("invalid effect requested\n");
			}
		gameData.objs.bWantEffect [i] &= ~(PLAYER_LIGHTNINGS | ROBOT_LIGHTNINGS | MISSILE_LIGHTNINGS | EXPL_LIGHTNINGS);
		}
	}
}

//------------------------------------------------------------------------------

void MoveLightnings (int i, tLightning *pl, vmsVector *vNewPos, short nSegment, int bStretch, int bFromEnd)
{
if (nSegment < 0)
	return;
if (SHOW_LIGHTNINGS) {

		tLightningNode	*pln;
		vmsVector		vDelta, vOffs;
		int				h, j, nLightnings;
		double			fStep;

	if (pl)
		i = 1;
	else if (IsUsedLightning (i)) {
		tLightningBundle	*plb = gameData.lightnings.buffer + i;
		pl = plb->pl;
		i = plb->nLightnings;
		}
	else
		return;
	nLightnings = i;
	CHECK (pl, nLightnings);
	for (; i > 0; i--, pl++) {
		pl->nSegment = nSegment;
		if (bFromEnd)
			vDelta = *vNewPos - pl->vEnd;
		else
			vDelta = *vNewPos - pl->vPos;
		if (vDelta[X] || vDelta[Y] || vDelta[Z]) {
			if (bStretch) {
				vOffs = vDelta;
				if (bFromEnd)
					pl->vEnd += vDelta;
				else
					pl->vPos += vDelta;
				}
			else if (bFromEnd) {
				pl->vPos += vDelta;
				pl->vEnd = *vNewPos;
				}
			else {
				pl->vEnd += vDelta;
				pl->vPos = *vNewPos;
				}
			if (bStretch) {
				pl->vDir = pl->vEnd - pl->vPos;
				pl->nLength = pl->vDir.Mag();
				}
			if (0 < (h = pl->nNodes)) {
				for (j = h, pln = pl->pNodes; j > 0; j--, pln++) {
					TRAP (pln);
					if (bStretch) {
						vDelta = vOffs;
						if (bFromEnd)
							fStep = (double) (h - j + 1) / (double) h;
						else
							fStep = (double) j / (double) h;
						vDelta[X] = (int) (vOffs[X] * fStep + 0.5);
						vDelta[Y] = (int) (vOffs[Y] * fStep + 0.5);
						vDelta[Z] = (int) (vOffs[Z] * fStep + 0.5);
						}
					pln->vNewPos += vDelta;
					pln->vBase += vDelta;
					pln->vPos += vDelta;
					if (pln->pChild)
						MoveLightnings (-1, pln->pChild, &pln->vPos, nSegment, 0, bFromEnd);
					}
				}
			}
		}
	CHECK (pl - nLightnings, nLightnings);
	}
}

//------------------------------------------------------------------------------
#if 0
void MoveObjectLightnings (tObject *objP)
{
SEM_ENTER (SEM_LIGHTNINGS)
MoveObjectLightningsInternal (objP);
SEM_LEAVE (SEM_LIGHTNINGS)
}
#endif
//------------------------------------------------------------------------------

void DestroyObjectLightnings (tObject *objP)
{
	int i = OBJ_IDX (objP);

if (gameData.lightnings.objects [i] >= 0) {
	DestroyLightnings (gameData.lightnings.objects [i], NULL, 0);
	gameData.lightnings.objects [i] = -1;
	}
}

//------------------------------------------------------------------------------

void DestroyAllObjectLightnings (int nType, int nId)
{
	tObject	*objP;
	int		i;

for (i = 0, objP = OBJECTS; i <= gameData.objs.nLastObject [0]; i++, objP++)
	if ((objP->nType == nType) && ((nId < 0) || (objP->id == nId)))
#if 1
		RequestEffects (objP, DESTROY_LIGHTNINGS);
#else
		DestroyObjectLightnings (objP);
#endif
}

//------------------------------------------------------------------------------

void DestroyPlayerLightnings (void)
{
DestroyAllObjectLightnings (OBJ_PLAYER, -1);
}

//------------------------------------------------------------------------------

void DestroyRobotLightnings (void)
{
DestroyAllObjectLightnings (OBJ_ROBOT, -1);
}

//------------------------------------------------------------------------------

void DestroyStaticLightnings (void)
{
DestroyAllObjectLightnings (OBJ_EFFECT, LIGHTNING_ID);
}

//------------------------------------------------------------------------------

#define LIGHTNING_VERT_ARRAYS 1

static tTexCoord2f plasmaTexCoord [3][4] = {
	{{{0,0.45f}},{{1,0.45f}},{{1,0.55f}},{{0,0.55f}}},
	{{{0,0.15f}},{{1,0.15f}},{{1,0.5f}},{{0,0.5f}}},
	{{{0,0.5f}},{{1,0.5f}},{{1,0.85f}},{{0,0.85f}}}
	};

void RenderLightningSegment (fVector *vLine, fVector *vPlasma, tRgbaColorf *colorP, int bPlasma, int bStart, int bEnd, short nDepth)
{
	fVector		vDelta;
	int			i, j, h = bStart + 2 * bEnd, bDrawArrays;
	tRgbaColorf	color = *colorP;

if (!bPlasma)
	color.alpha *= 1.5f;
if (nDepth)
	color.alpha /= 2;
if (bPlasma) {
#if RENDER_LIGHTNING_OUTLINE //render lightning segment outline
	glDisable (GL_TEXTURE_2D);
	glDisable (GL_BLEND);
	if (bStart)
		glColor3f (1,0,0);
	else
		glColor3f (0,0,1);
	glLineWidth (2);
	glBegin (GL_LINE_LOOP);
	for (i = 0; i < 4; i++) {
		if (i < 2)
			glColor3d (1,1,1);
		else if (i < 3)
			glColor3d (1,0.8,0);
		else
			glColor3d (1,0.5,0);
		glVertex3fv ((GLfloat *) (vPlasma + i));
		}
	glEnd ();
	glLineWidth (1);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_BLEND);
#endif
	bDrawArrays = G3EnableClientStates (1, 0, 0, GL_TEXTURE0);
	glEnable (GL_TEXTURE_2D);
	if (LoadCorona () && !OglBindBmTex (bmpCorona, 1, -1)) {
		OglTexWrap (bmpCorona->glTexture, GL_CLAMP);
		for (i = 0; i < 2; i++) {
			if (!i) {
				//OglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glColor4f (color.red / 2, color.green / 2, color.blue / 2, color.alpha);
				}
			else {
				//OglBlendFunc (GL_SRC_ALPHA, GL_ONE);
				glColor4f (1, 1, 1, color.alpha / 2);
				}
			if (bDrawArrays) {
				glTexCoordPointer (2, GL_FLOAT, sizeof (tTexCoord3f), plasmaTexCoord + h);
				glVertexPointer (3, GL_FLOAT, sizeof (fVector), vPlasma);
				glDrawArrays (GL_TRIANGLE_FAN, 0, 4);
				}
			else {
				glBegin (GL_QUADS);
				for (j = 0; j < 4; j++) {
					glTexCoord2fv ((GLfloat *) (plasmaTexCoord [h] + j));
					glVertex3fv ((GLfloat *) (vPlasma + j));
					}
				glEnd ();
				}
			if (!i) {	//resize plasma quad for inner, white plasma path
				vDelta = vPlasma[0] - vPlasma[1];
				vDelta = vDelta * 0.25f;
				vPlasma[0] -= vDelta;
				vPlasma[1] += vDelta;
				vDelta = vPlasma[2] - vPlasma[3];
				vDelta = vDelta * 0.25f;
				vPlasma[2] -= vDelta;
				vPlasma[3] += vDelta;
				}
			}
		}
	if (bDrawArrays)
		G3DisableClientStates (1, 0, 0, -1);
	}
glBlendFunc (GL_SRC_ALPHA, GL_ONE);
glColor4fv ((GLfloat *) &color);
glLineWidth ((GLfloat) (nDepth ? 2 : 4));
glDisable (GL_TEXTURE_2D);
glEnable (GL_SMOOTH);
glBegin (GL_LINES);
glVertex3fv ((GLfloat *) vLine);
glVertex3fv ((GLfloat *) (vLine + 1));
glEnd ();
glLineWidth (1);
glDisable (GL_SMOOTH);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//------------------------------------------------------------------------------

#define MAX_LIGHTNING_SEGMENTS	10000

typedef struct tPlasmaBuffer {
	tTexCoord2f	texCoord [4 * MAX_LIGHTNING_SEGMENTS];
	fVector		vertices [4 * MAX_LIGHTNING_SEGMENTS];
} tPlasmaBuffer;

static tPlasmaBuffer plasmaBuffers [2][2];
static fVector3 coreBuffer [2][MAX_LIGHTNING_SEGMENTS];

//------------------------------------------------------------------------------

void ComputePlasmaSegment (fVector *vPosf, int bScale, short nSegment, char bStart, char bEnd, int nDepth, int nThread)
{
	fVector			*vPlasma = plasmaBuffers [nThread][bScale].vertices + 4 * nSegment;
	fVector			vn [2], vd;

	static fVector vEye = fVector::ZERO;
	static fVector vNormal [3] = {fVector::ZERO,fVector::ZERO,fVector::ZERO};

memcpy (vNormal, vNormal + 1, 2 * sizeof (fVector));
if (bStart) {
	vNormal[1] = fVector::Normal(vPosf[0], vPosf[1], vEye);
	vn [0] = vNormal [1];
	}
else {
	vn[0] = vNormal[0] + vNormal[1];
	vn[0] = vn[0] * 0.5f;
}

if (bEnd) {
	vn [1] = vNormal [1];
	vd = vPosf[1] - vPosf[0];
	vd = vd * (bScale ? 0.25f : 0.5f);
	vPosf[1] += vd;
	}
else {
	vNormal[2] = fVector::Normal(vPosf[1], vPosf[2], vEye);
	if (fVector::Dot(vNormal[1], vNormal[2]) < 0)
		vNormal[2].Neg();
	vn[1] = vNormal[1] + vNormal[2];
	vn[1] = vn[1] * 0.5f;
	}
if (!(nDepth || bScale)) {
	vn[0] = vn[0] * 2;
	vn[1] = vn[1] * 2;
	}
if (!bScale && nDepth) {
	vn[0] = vn[0] * 0.5f;
	vn[1] = vn[1] * 0.5f;
	}
if (bStart) {
	vPlasma[0] = vPosf[0] + vn[0];
	vPlasma[1] = vPosf[0] - vn[0];
	vd = vPosf[0] - vPosf[1];
	fVector::Normalize(vd);
	if (bScale)
		vd = vd * 0.5f;
	vPlasma[0] += vd;
	vPlasma[1] += vd;
	}
else {
	vPlasma [0] = vPlasma [-1];
	vPlasma [1] = vPlasma [-2];
	}
vPlasma[3] = vPosf[1] + vn[1];
vPlasma[2] = vPosf[1] - vn[1];
memcpy (plasmaBuffers [nThread][bScale].texCoord + 4 * nSegment, plasmaTexCoord [bStart + 2 * bEnd], 4 * sizeof (tTexCoord2f));
}

//------------------------------------------------------------------------------

void ComputePlasmaBuffer (tLightning *pl, int nDepth, int nThread)
{
	tLightningNode	*pln;
	fVector			vPosf [3] = {fVector::ZERO,fVector::ZERO,fVector::ZERO};
	int				bScale, i, j;

for (bScale = 0; bScale < 2; bScale++) {
	pln = pl->pNodes;
	vPosf[2] = (pln++)->vPos.ToFloat();
	if (!gameStates.ogl.bUseTransform)
		G3TransformPoint(vPosf[2], vPosf[2], 0);
	for (i = pl->nNodes - 2, j = 0; j <= i; j++) {
		TRAP (pln);
		memcpy (vPosf, vPosf + 1, 2 * sizeof (fVector));
		vPosf[2] = (++pln)->vPos.ToFloat();
		if (!gameStates.ogl.bUseTransform)
			G3TransformPoint (vPosf[2], vPosf[2], 0);
		TRAP (pln);
		ComputePlasmaSegment (vPosf, bScale, j, j == 1, j == i, nDepth, nThread);
		TRAP (pln);
		}
	}
}

//------------------------------------------------------------------------------

void RenderPlasmaBuffer (tLightning *pl, tRgbaColorf *colorP, int nThread)
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
	glDrawArrays (GL_QUADS, 0, 4 * (pl->nNodes - 1));
#if RENDER_LIGHTNING_OUTLINE
	glDisable (GL_TEXTURE_2D);
	glColor3f (1,1,1);
	texCoordP = plasmaBuffers [nThread][bScale].texCoord;
	vertexP = plasmaBuffers [nThread][bScale].vertices;
	for (i = pl->nNodes - 1; i; i--) {
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

void RenderLightningCore (tLightning *pl, tRgbaColorf *colorP, int nDepth, int nThread)
{
	tLightningNode	*pln;
	fVector3			*vPosf = coreBuffer [nThread];
	int				i;

glBlendFunc (GL_ONE, GL_ONE);
glDisable (GL_TEXTURE_2D);
glColor4f (colorP->red / 4, colorP->green / 4, colorP->blue / 4, colorP->alpha);
glLineWidth ((GLfloat) (nDepth ? 2 : 4));
glDisable (GL_SMOOTH);
for (i = pl->nNodes, pln = pl->pNodes; i > 0; i--, pln++, vPosf++) {
	TRAP (pln);
	// Check ToFloat
	*vPosf = pln->vPos.ToFloat3();
	}
if (!gameStates.ogl.bUseTransform)
	OglSetupTransform (1);
if (G3EnableClientStates (0, 0, 0, GL_TEXTURE0)) {
	glVertexPointer (3, GL_FLOAT, 0, coreBuffer [nThread]);
	glDrawArrays (GL_LINE_STRIP, 0, pl->nNodes);
	G3DisableClientStates (0, 0, 0, GL_TEXTURE0);
	}
else {
	glBegin (GL_LINE_STRIP);
	for (i = 0; i < pl->nNodes; i++)
		glVertex3fv ((GLfloat *) (coreBuffer [nThread] + i));
	glEnd ();
	}
if (!gameStates.ogl.bUseTransform)
	OglResetTransform (1);
glLineWidth (1);
glDisable (GL_SMOOTH);

#if defined (_DEBUG) && RENDER_TARGET_LIGHTNING
glColor3f (1,1,1);
glLineWidth (1);
glBegin (GL_LINE_STRIP);
for (i = pl->nNodes, pln = pl->pNodes; i > 0; i--, pln++) {
	TRAP (pln);
	VmVecFixToFloat (vPosf, &pln->vNewPos);
	G3TransformPoint (vPosf, vPosf, 0);
	glVertex3fv ((GLfloat *) vPosf);
	}
glEnd ();
#endif
}

//------------------------------------------------------------------------------

int SetupLightningPlasma (tLightning *pl)
{
if (!(gameOpts->render.lightnings.bPlasma && pl->bPlasma && G3EnableClientStates (1, 0, 0, GL_TEXTURE0)))
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

void RenderLightningsBuffered (tLightning *plRoot, int nStart, int nLightnings, int nDepth, int nThread)
{
	tLightning		*pl = plRoot + nStart;
	tLightningNode	*pln;
	int				h, i;
	int				bPlasma;
	tRgbaColorf		color;

bPlasma = SetupLightningPlasma (pl);
for (h = nLightnings - nStart; h > 0; h--, pl++) {
	if ((pl->nNodes < 0) || (pl->nSteps < 0))
		continue;
	if (gameStates.app.bMultiThreaded)
		tiRender.ti [nThread].bBlock = 1;
	color = pl->color;
	if (pl->nLife > 0) {
		if ((i = pl->nLife - pl->nTTL) < 250)
			color.alpha *= (float) i / 250.0f;
		else if (pl->nTTL < pl->nLife / 3)
			color.alpha *= (float) pl->nTTL / (float) (pl->nLife / 3);
		}
	color.red *= (float) (0.9 + dbl_rand () / 5);
	color.green *= (float) (0.9 + dbl_rand () / 5);
	color.blue *= (float) (0.9 + dbl_rand () / 5);
	if (!bPlasma)
		color.alpha *= 1.5f;
	if (nDepth)
		color.alpha /= 2;
	if (gameStates.app.bMultiThreaded && nThread) {	//thread 1 will always render after thread 0
		tiRender.ti [1].bBlock = 0;
		while (tiRender.ti [0].bBlock)
			G3_SLEEP (0);
		}
	if (bPlasma) {
		ComputePlasmaBuffer (pl, nDepth, nThread);
		RenderPlasmaBuffer (pl, &color, nThread);
		}
	RenderLightningCore (pl, &color, nDepth, nThread);
	if (gameStates.app.bMultiThreaded && !nThread) { //thread 0 will wait for thread 1 to complete its rendering
		tiRender.ti [0].bBlock = 0;
		while (tiRender.ti [1].bBlock)
			G3_SLEEP (0);
		}
	}
if (gameOpts->render.lightnings.nQuality)
	for (pl = plRoot + nStart, h = nLightnings -= nStart; h > 0; h--, pl++)
		if (0 < (i = pl->nNodes))
			for (pln = pl->pNodes; i > 0; i--, pln++)
				if (pln->pChild)
					RenderLightningsBuffered (pln->pChild, 0, 1, nDepth + 1, nThread);
}

//------------------------------------------------------------------------------

void RenderLightningPlasma (fVector *vPosf, tRgbaColorf *color, int bScale, int bDrawArrays,
									 char bStart, char bEnd, char bPlasma, short nDepth, int bDepthSort)
{
	static fVector	vEye = fVector::ZERO;
	static fVector	vPlasma [6] = {fVector::ZERO,fVector::ZERO,fVector::ZERO,
	                               fVector::ZERO,fVector::ZERO,fVector::ZERO};
	static fVector vNormal [3] = {fVector::ZERO,fVector::ZERO,fVector::ZERO};

	fVector	vn [2], vd;
	int		i, j = bStart + 2 * bEnd;

memcpy (vNormal, vNormal + 1, 2 * sizeof (fVector));
if (bStart) {
	vNormal[1] = fVector::Normal(vPosf[0], vPosf[1], vEye);
	vn [0] = vNormal [1];
	}
else {
	vn[0] = vNormal[0] + vNormal[1];
	vn[0] = vn[0] * 0.5f;
}

if (bEnd)
	vn [1] = vNormal [1];
else {
	vNormal[2] = fVector::Normal(vPosf[1], vPosf[2], vEye);
	vn[1] = vNormal[1] + vNormal[2];
	vn[1] = vn[1] * 0.5f;
	}
if (!(nDepth || bScale)) {
	vn[0] = vn[0] * 2;
	vn[1] = vn[1] * 2;
	}
if (!bScale && nDepth) {
	vn[0] = vn[0] * 0.5f;
	vn[1] = vn[1] * 0.5f;
	}
if (bStart) {
	vPlasma[0] = vPosf[0] + vn[0];
	vPlasma[1] = vPosf[0] - vn[0];
	vd = vPosf[0] - vPosf[1];
	fVector::Normalize(vd);
	if (bScale)
		vd = vd * 0.5f;
	vPlasma[0] += vd;
	vPlasma[1] += vd;
	}
else {
	vPlasma [0] = vPlasma [3];
	vPlasma [1] = vPlasma [2];
	}
vPlasma[3] = vPosf[1] + vn[1];
vPlasma[2] = vPosf[1] - vn[1];
if (bEnd) {
	vd = vPosf[1] - vPosf[0];
	fVector::Normalize(vd);
	if (bScale)
		vd = vd * 0.5f;
	vPlasma[2] += vd;
	vPlasma[3] += vd;
	}
if (bDepthSort) {
	RIAddLightningSegment (vPosf, vPlasma, color, bPlasma, bStart, bEnd, nDepth);
	}
else {
#if 1
	if (bDrawArrays) {
		glTexCoordPointer (2, GL_FLOAT, 0, plasmaTexCoord + j);
		glVertexPointer (3, GL_FLOAT, sizeof (fVector), vPlasma);
		glDrawArrays (GL_TRIANGLE_FAN, 0, 4);
		}
	else {
#if 0
		float		fDot;
		VmVecNormal (vn, vPlasma, vPlasma + 1, vPlasma + 2);
		VmVecNormal (vn + 1, vPlasma, vPlasma + 2, vPlasma + 3);
		fDot = fVector::Dot (vn, vn + 1);
		if (fDot >= 0) {
			glBegin (GL_TRIANGLES);
			glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j]));
			glVertex3fv ((GLfloat *) (vPlasma));
			glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j] + 1));
			glVertex3fv ((GLfloat *) (vPlasma + 1));
			glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j] + 2));
			glVertex3fv ((GLfloat *) (vPlasma + 2));
			glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j]));
			glVertex3fv ((GLfloat *) (vPlasma));
			glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j] + 2));
			glVertex3fv ((GLfloat *) (vPlasma + 2));
			glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j] + 3));
			glVertex3fv ((GLfloat *) (vPlasma + 3));
			glEnd ();
			}
		else if (fDot < 0) {
			glBegin (GL_TRIANGLES);
			glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j]));
			glVertex3fv ((GLfloat *) (vPlasma));
			glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j] + 1));
			glVertex3fv ((GLfloat *) (vPlasma + 1));
			glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j] + 3));
			glVertex3fv ((GLfloat *) (vPlasma + 3));
			glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j]));
			glVertex3fv ((GLfloat *) (vPlasma));
			glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j] + 2));
			glVertex3fv ((GLfloat *) (vPlasma + 2));
			glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j] + 3));
			glVertex3fv ((GLfloat *) (vPlasma + 3));
			glEnd ();
			}
		else
#endif
			{
			glBegin (GL_QUADS);
			for (i = 0; i < 4; i++) {
				glTexCoord2fv ((GLfloat *) (plasmaTexCoord [j] + i));
				glVertex3fv ((GLfloat *) (vPlasma + i));
				}
			}
		glEnd ();
		}
#endif
#if RENDER_LIGHTNING_OUTLINE //render lightning segment outline
	glDisable (GL_TEXTURE_2D);
	glDisable (GL_BLEND);
	glLineWidth (1);
	if (bStart)
		glColor3f (1,0,0);
	else
		glColor3f (0,0,1);
	glBegin (GL_LINE_LOOP);
	for (i = 0; i < 4; i++) {
		glVertex3fv ((GLfloat *) (vPlasma + i));
		}
	glEnd ();
	glColor4fv ((GLfloat *) color);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_BLEND);
#endif
	}
}

//------------------------------------------------------------------------------

inline int LightningMayBeVisible (tLightning *pl)
{
if (pl->nSegment >= 0)
	return SegmentMayBeVisible (pl->nSegment, pl->nLength / 20, 3 * pl->nLength / 2);
if (pl->nObject >= 0)
	return (gameData.render.mine.bObjectRendered [pl->nObject] == gameStates.render.nFrameFlipFlop);
return 1;
}

//------------------------------------------------------------------------------

void RenderLightning (tLightning *pl, int nLightnings, short nDepth, int bDepthSort)
{
	tLightningNode	*pln;
	int			i;
#if !RENDER_LIGHTINGS_BUFFERED
	int			h, j;
#endif
	int			bPlasma = gameOpts->render.lightnings.bPlasma && pl->bPlasma;
	tRgbaColorf	color;
#if RENDER_LIGHTING_SEGMENTS
	fVector		vPosf [3] = {{{0,0,0}},{{0,0,0}}};
	tObject		*objP = NULL;
#endif

if (!pl && LightningMayBeVisible (pl))
	return;
if (bDepthSort > 0) {
	bPlasma = gameOpts->render.lightnings.bPlasma && pl->bPlasma;
	color = pl->color;
	if (pl->nLife > 0) {
		if ((i = pl->nLife - pl->nTTL) < 250)
			color.alpha *= (float) i / 250.0f;
		else if (pl->nTTL < pl->nLife / 3)
			color.alpha *= (float) pl->nTTL / (float) (pl->nLife / 3);
		}
	color.red *= (float) (0.9 + dbl_rand () / 5);
	color.green *= (float) (0.9 + dbl_rand () / 5);
	color.blue *= (float) (0.9 + dbl_rand () / 5);
	for (; nLightnings; nLightnings--, pl++) {
		if ((pl->nNodes < 0) || (pl->nSteps < 0))
			continue;
#if RENDER_LIGHTING_SEGMENTS
		for (i = pl->nNodes - 1, j = 0, pln = pl->pNodes; j <= i; j++) {
			if (j < i)
				memcpy (vPosf, vPosf + 1, 2 * sizeof (fVector));
			if (!j) {
				VmVecFixToFloat (vPosf + 1, &(pln++)->vPos);
				G3TransformPoint (vPosf + 1, vPosf + 1, 0);
				}
			if (j < i) {
				VmVecFixToFloat (vPosf + 2, &(++pln)->vPos);
				G3TransformPoint (vPosf + 2, vPosf + 2, 0);
				}
			if (j)
				RenderLightningPlasma (vPosf, &color, 0, 0, j == 1, j == i, 1, nDepth, 1);
			}
#else
		RIAddLightnings (pl, 1, nDepth);
#endif
		if (gameOpts->render.lightnings.nQuality)
			for (i = pl->nNodes, pln = pl->pNodes; i > 0; i--, pln++)
				if (pln->pChild)
					RenderLightning (pln->pChild, 1, nDepth + 1, 1);
		}
	}
else {
	if (!nDepth) {
		glEnable (GL_BLEND);
		if ((bDepthSort < 1) || (gameOpts->render.bDepthSort < 1)) {
			glDepthMask (0);
			glDisable (GL_CULL_FACE);
			}
		}
#if RENDER_LIGHTINGS_BUFFERED
#	if 0 // no speed gain here
	if (gameStates.app.bMultiThreaded && (nLightnings > 1)) {
		tiRender.pl = pl;
		tiRender.ti [0].bBlock =
		tiRender.ti [1].bBlock = 0;
		tiRender.nLightnings = nLightnings;
		RunRenderThreads (rtRenderLightnings);
		}
	else
#	endif
		RenderLightningsBuffered (pl, 0, nLightnings, 0, 0);
#else
	for (; nLightnings; nLightnings--, pl++) {
		if ((pl->nNodes < 0) || (pl->nSteps < 0))
			continue;
		color = pl->color;
		if (pl->nLife > 0) {
			if ((i = pl->nLife - pl->nTTL) < 250)
				color.alpha *= (float) i / 250.0f;
			else if (pl->nTTL < pl->nLife / 3)
				color.alpha *= (float) pl->nTTL / (float) (pl->nLife / 3);
			}
		color.red *= (float) (0.9 + dbl_rand () / 5);
		color.green *= (float) (0.9 + dbl_rand () / 5);
		color.blue *= (float) (0.9 + dbl_rand () / 5);
		if (!bPlasma)
			color.alpha *= 1.5f;
		if (nDepth)
			color.alpha /= 2;
#if RENDER_LIGHTNING_PLASMA
		if (bPlasma) {
			for (h = 0; h < 2; h++) {
				glBlendFunc (GL_ONE, GL_ONE);
				if (h)
					glColor4f (0.05f, 0.05f, 0.05f, color.alpha / 2);
				else
					glColor4f (color.red / 20, color.green / 20, color.blue / 20, color.alpha);
				for (i = pl->nNodes - 1, j = 0, pln = pl->pNodes; j <= i; j++) {
					if (j < i)
						memcpy (vPosf, vPosf + 1, 2 * sizeof (fVector));
					if (!j) {
						VmVecFixToFloat (vPosf + 1, &(pln++)->vPos);
						if (!gameStates.ogl.bUseTransform)
							G3TransformPoint (vPosf + 1, vPosf + 1, 0);
						}
					if (j < i) {
						VmVecFixToFloat (vPosf + 2, &(++pln)->vPos);
						if (!gameStates.ogl.bUseTransform)
							G3TransformPoint (vPosf + 2, vPosf + 2, 0);
						}
					if (j)
						RenderLightningPlasma (vPosf, &color, h != 0, bPlasma, j == 1, j == i, 1, nDepth, 0);
					}
				}
			if (bPlasma)
				G3DisableClientStates (1, 0, 0, -1);
			}
#endif
#if 1
#	if 0
		if (nDepth)
			color.alpha /= 2;
#	endif
		glBlendFunc (GL_SRC_ALPHA, GL_ONE);
		glColor4fv ((GLfloat *) &color);
		glLineWidth ((GLfloat) (nDepth ? 2 : 4));
		glDisable (GL_TEXTURE_2D);
		glEnable (GL_SMOOTH);
		glBegin (GL_LINE_STRIP);
		for (i = pl->nNodes, pln = pl->pNodes; i > 0; i--, pln++) {
			VmVecFixToFloat (vPosf, &pln->vPos);
			if (!gameStates.ogl.bUseTransform)
				G3TransformPoint (vPosf, vPosf, 0);
			glVertex3fv ((GLfloat *) vPosf);
			}
		glEnd ();
#	if 0
		if (nDepth)
			color.alpha *= 2;
#	endif
#endif
#if defined (_DEBUG) && RENDER_TARGET_LIGHTNING
		glColor3f (1,0,0,1);
		glLineWidth (1);
		glBegin (GL_LINE_STRIP);
		for (i = pl->nNodes, pln = pl->pNodes; i > 0; i--, pln++) {
			VmVecFixToFloat (vPosf, &pln->vNewPos);
			G3TransformPoint (vPosf, vPosf, 0);
			glVertex3fv ((GLfloat *) vPosf);
			}
		glEnd ();
#endif
		if (gameOpts->render.lightnings.nQuality)
			for (i = pl->nNodes, pln = pl->pNodes; i > 0; i--, pln++)
				if (pln->pChild)
					RenderLightning (pln->pChild, 1, nDepth + 1, bDepthSort);
		}
#endif
	if (!nDepth) {
		if ((bDepthSort < 1) || (gameOpts->render.bDepthSort < 1)) {
			glEnable (GL_CULL_FACE);
			glDepthMask (1);
			}
		}
	glLineWidth (1);
	glDisable (GL_SMOOTH);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

//------------------------------------------------------------------------------

void RenderLightnings (void)
{
if (SHOW_LIGHTNINGS) {
		tLightningBundle	*plb;
		int					i, n, bStencil = StencilOff ();

	for (i = gameData.lightnings.iUsed; i >= 0; i = n) {
		plb = gameData.lightnings.buffer + i;
		n = plb->nNext;
		if (!(plb->nKey [0] | plb->nKey [1]))
			RenderLightning (plb->pl, plb->nLightnings, 0, gameOpts->render.bDepthSort > 0);
		}
	StencilOn (bStencil);
	}
}

//------------------------------------------------------------------------------

vmsVector *FindLightningTargetPos (tObject *emitterP, short nTarget)
{
	int				i;
	tObject			*objP;

if (!nTarget)
	return 0;
for (i = 0, objP = OBJECTS; i <= gameData.objs.nLastObject [0]; i++, objP++)
	if ((objP != emitterP) && (objP->nType == OBJ_EFFECT) && (objP->id == LIGHTNING_ID) && (objP->rType.lightningInfo.nId == nTarget))
		return &objP->position.vPos;
return NULL;
}

//------------------------------------------------------------------------------

void StaticLightningFrame (void)
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
for (i = 0, objP = OBJECTS; i <= gameData.objs.nLastObject [0]; i++, objP++) {
	if ((objP->nType != OBJ_EFFECT) || (objP->id != LIGHTNING_ID))
		continue;
	if (gameData.lightnings.objects [i] >= 0)
		continue;
	pli = &objP->rType.lightningInfo;
	if (pli->nLightnings <= 0)
		continue;
	if (pli->bRandom && !pli->nAngle)
		vEnd = NULL;
	else if ((vEnd = FindLightningTargetPos (objP, pli->nTarget)))
		pli->nLength = vmsVector::Dist(objP->position.vPos, *vEnd) / F1_0;
	else {
		v = objP->position.vPos + objP->position.mOrient[FVEC] * F1_0 * pli->nLength;
		vEnd = &v;
		}
	color.red = (float) pli->color.red / 255.0f;
	color.green = (float) pli->color.green / 255.0f;
	color.blue = (float) pli->color.blue / 255.0f;
	color.alpha = (float) pli->color.alpha / 255.0f;
	vDelta = pli->bInPlane ? &objP->position.mOrient[RVEC] : NULL;
	h = CreateLightning (pli->nLightnings, &objP->position.vPos, vEnd, vDelta, i, -abs (pli->nLife), pli->nDelay, pli->nLength * F1_0,
							   pli->nAmplitude * F1_0, pli->nAngle, pli->nOffset * F1_0, pli->nNodes, pli->nChildren, pli->nChildren > 0, pli->nSteps,
							   pli->nSmoothe, pli->bClamp, pli->bPlasma, pli->bSound, 1, pli->nStyle, &color);
	if (h >= 0)
		gameData.lightnings.objects [i] = h;
	}
}

//------------------------------------------------------------------------------

void DoLightningFrame (void)
{
if (gameData.lightnings.bDestroy) {
	gameData.lightnings.bDestroy = -1;
	DestroyAllLightnings (0);
	}
else {
	UpdateLightnings ();
	UpdateOmegaLightnings (NULL, NULL);
	StaticLightningFrame ();
	}
}

//------------------------------------------------------------------------------

void SetLightningSegLight (short nSegment, vmsVector *vPosP, tRgbaColorf *colorP)
{
if ((nSegment < 0) || (nSegment >= gameData.segs.nSegments))
	return;
else {
		tLightningLight	*pll = gameData.lightnings.lights + nSegment;

#ifdef _DEBUG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	if (pll->nFrame != gameData.app.nFrameCount) {
		memset (pll, 0, sizeof (*pll));
		pll->nFrame = gameData.app.nFrameCount;
		pll->nSegment = nSegment;
		pll->nNext = gameData.lightnings.nFirstLight;
		gameData.lightnings.nFirstLight = nSegment;
		}
	pll->nLights++;
	pll->vPos += *vPosP;
	pll->color.red += colorP->red;
	pll->color.green += colorP->green;
	pll->color.blue += colorP->blue;
	pll->color.alpha += colorP->alpha;
	}
}

//------------------------------------------------------------------------------

int SetLightningLight (tLightning *pl, int i)
{
	tLightningNode	*pln;
	vmsVector		*vPosP;
	short				nSegment;
	int				j, nLights, nStride;
	double			h, nStep;

//SetLightningSegLight (pl->nSegment, &pl->vPos, &pl->color);
if (!pl->bLight)
	return 0;
for (nLights = 1; i > 0; i--, pl++) {
	if (0 < (j = pl->nNodes)) {
		if (!(nStride = (int) ((double) pl->nLength / (F1_0 * 20) + 0.5)))
			nStride = 1;
		if (!(nStep = (double) (j - 1) / (double) nStride))
			nStep = (double) ((int) (j + 0.5));
		nSegment = pl->nSegment;
#ifdef _DEBUG
		if (nSegment == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		pln = pl->pNodes;
		for (h = nStep / 2; h < j; h += nStep) {
			TRAP (pln);
			vPosP = &pln [(int) h].vPos;
			if (0 > (nSegment = FindSegByPos (*vPosP, nSegment, 0, 0)))
				break;
			SetLightningSegLight (nSegment, vPosP, &pl->color);
			nLights++;
			}
		}
	}
return nLights;
}

//------------------------------------------------------------------------------

void ResetLightningLights (int bForce)
{
if (SHOW_LIGHTNINGS || bForce) {
		tLightningLight	*pll;
		int					i;

	for (i = gameData.lightnings.nFirstLight; i >= 0; ) {
		if ((i < 0) || (i >= MAX_SEGMENTS))
			continue;
		pll = gameData.lightnings.lights + i;
		i = pll->nNext;
		pll->nLights = 0;
		pll->nNext = -1;
		pll->vPos[X] =
		pll->vPos[Y] =
		pll->vPos[Z] = 0;
		pll->color.red =
		pll->color.green =
		pll->color.blue = 0;
		pll->nBrightness = 0;
		if (pll->nDynLight >= 0) {
			pll->nDynLight = -1;
			}
		}
	gameData.lightnings.nFirstLight = -1;
	DeleteLightningLights ();
	}
}

//------------------------------------------------------------------------------

void SetLightningLights (void)
{
ResetLightningLights (0);
if (SHOW_LIGHTNINGS) {
		tLightningBundle	*plb;
		tLightningLight	*pll = NULL;
		int					i, n, nLights = 0, bDynLighting = gameOpts->render.nLightingMethod;

	gameData.lightnings.nFirstLight = -1;
	for (i = gameData.lightnings.iUsed; i >= 0; i = n) {
		plb = gameData.lightnings.buffer + i;
		n = plb->nNext;
		nLights += SetLightningLight (plb->pl, plb->nLightnings);
		}
	if (nLights) {
		for (i = gameData.lightnings.nFirstLight; i >= 0; i = pll->nNext) {
			if ((i < 0) || (i >= MAX_SEGMENTS))
				continue;
			pll = gameData.lightnings.lights + i;
#ifdef _DEBUG
			if (pll->nSegment == nDbgSeg)
				nDbgSeg = nDbgSeg;
#endif
			n = pll->nLights;
			pll->vPos[X] /= n;
			pll->vPos[Y] /= n;
			pll->vPos[Z] /= n;
			pll->color.red /= n;
			pll->color.green /= n;
			pll->color.blue /= n;

			if (gameStates.render.bPerPixelLighting == 2)
				pll->nBrightness = F2X (sqrt ((pll->color.red * 3 + pll->color.green * 5 + pll->color.blue * 2) * pll->color.alpha));
			else
				pll->nBrightness = F2X ((pll->color.red * 3 + pll->color.green * 5 + pll->color.blue * 2) * pll->color.alpha);
			if (bDynLighting)
				pll->nDynLight = AddDynLight (NULL, &pll->color, pll->nBrightness, pll->nSegment, -1, -1, -1, &pll->vPos);
			}
		}
	}
}

//------------------------------------------------------------------------------

void CreateExplosionLightnings (tObject *objP, tRgbaColorf *colorP, int nRods, int nRad, int nTTL)
{
if (SHOW_LIGHTNINGS && gameOpts->render.lightnings.bExplosions) {
	//gameData.lightnings.objects [OBJ_IDX (objP)] =
		CreateLightning (
			nRods, &objP->position.vPos, NULL, NULL, OBJ_IDX (objP), nTTL, 0,
			nRad, F1_0 * 4, 0, 2 * F1_0, 50, 5, 1, 3, 1, 1, 0, 0, 1, -1, colorP);
	}
}

//------------------------------------------------------------------------------

void CreateShakerLightnings (tObject *objP)
{
static tRgbaColorf color = {0.1f, 0.1f, 0.8f, 0.2f};

CreateExplosionLightnings (objP, &color, 30, 20 * F1_0, 750);
}

//------------------------------------------------------------------------------

void CreateShakerMegaLightnings (tObject *objP)
{
static tRgbaColorf color = {0.1f, 0.1f, 0.6f, 0.2f};

CreateExplosionLightnings (objP, &color, 20, 15 * F1_0, 750);
}

//------------------------------------------------------------------------------

void CreateMegaLightnings (tObject *objP)
{
static tRgbaColorf color = {0.8f, 0.1f, 0.1f, 0.2f};

CreateExplosionLightnings (objP, &color, 30, 15 * F1_0, 750);
}

//------------------------------------------------------------------------------

int CreateMissileLightnings (tObject *objP)
{
if (gameData.objs.bIsMissile [objP->id]) {
	if ((objP->id == EARTHSHAKER_ID) || (objP->id == EARTHSHAKER_ID))
		CreateShakerLightnings (objP);
	else if ((objP->id == EARTHSHAKER_MEGA_ID) || (objP->id == ROBOT_SHAKER_MEGA_ID))
		CreateShakerMegaLightnings (objP);
	else if ((objP->id == MEGAMSL_ID) || (objP->id == ROBOT_MEGAMSL_ID))
		CreateMegaLightnings (objP);
	else
		return 0;
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

void CreateBlowupLightnings (tObject *objP)
{
static tRgbaColorf color = {0.1f, 0.1f, 0.8f, 0.2f};

int h = X2I (objP->size) * 2;

CreateExplosionLightnings (objP, &color, h + rand () % h, h * (F1_0 + F1_0 / 2), 500);
}

//------------------------------------------------------------------------------

void CreateRobotLightnings (tObject *objP, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNINGS && gameOpts->render.lightnings.bRobots && OBJECT_EXISTS (objP)) {
		int h, i = OBJ_IDX (objP);

	if (0 <= gameData.lightnings.objects [i])
		MoveObjectLightnings (objP);
	else {
		h = CreateLightning (2 * objP->size / F1_0, &objP->position.vPos, NULL, NULL, OBJ_IDX (objP), -1000, 100,
									objP->size, objP->size / 8, 0, 0, 25, 3, 1, 3, 1, 1, 0, 0, 1, 0, colorP);
		if (h >= 0)
			gameData.lightnings.objects [i] = h;
		}
	}
}

//------------------------------------------------------------------------------

void CreatePlayerLightnings (tObject *objP, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNINGS && gameOpts->render.lightnings.bPlayers && OBJECT_EXISTS (objP)) {
	int h, i = OBJ_IDX (objP);

	if (0 <= gameData.lightnings.objects [i])
		MoveObjectLightnings (objP);
	else {
		h = CreateLightning (4 * objP->size / F1_0, &objP->position.vPos, NULL, NULL, OBJ_IDX (objP), -5000, 1000,
									4 * objP->size, objP->size, 0, 2 * objP->size, 50, 5, 1, 5, 1, 1, 0, 1, 1, 1, colorP);
		if (h >= 0)
			gameData.lightnings.objects [i] = h;
		}
	}
}

//------------------------------------------------------------------------------

void CreateDamageLightnings (tObject *objP, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNINGS && gameOpts->render.lightnings.bDamage && OBJECT_EXISTS (objP)) {
		int h, n, i = OBJ_IDX (objP);
		tLightningBundle	*plb;

	n = X2IR (RobotDefaultShields (objP));
	h = X2IR (objP->shields) * 100 / n;
	if ((h < 0) || (h >= 50))
		return;
	n = (5 - h / 10) * 2;
	if (0 <= (h = gameData.lightnings.objects [i])) {
		plb = gameData.lightnings.buffer + h;
		if (plb->nLightnings == n) {
			MoveObjectLightnings (objP);
			return;
			}
		DestroyLightnings (h, NULL, 0);
		}
	h = CreateLightning (n, &objP->position.vPos, NULL, NULL, OBJ_IDX (objP), -1000, 4000,
								objP->size, objP->size / 8, 0, 0, 20, 0, 1, 10, 1, 1, 0, 0, 0, -1, colorP);
	if (h >= 0)
		gameData.lightnings.objects [i] = h;
	}
}

//------------------------------------------------------------------------------

int FindDamageLightning (short nObject, int *pKey)
{
		tLightningBundle	*plb;
		int					i;

for (i = gameData.lightnings.iUsed; i >= 0; i = plb->nNext) {
	plb = gameData.lightnings.buffer + i;
	if ((plb->nObject == nObject) && (plb->nKey [0] == pKey [0]) && (plb->nKey [1] == pKey [1]))
		return i;
	}
return -1;
}

//------------------------------------------------------------------------------

typedef union tPolyKey {
	int	i [2];
	short	s [4];
} tPolyKey;

void RenderDamageLightnings (tObject *objP, g3sPoint **pointList, tG3ModelVertex *pVerts, int nVertices)
{
	tLightningBundle	*plb;
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
if ((objP->nType != OBJ_ROBOT) && (objP->nType != OBJ_PLAYER))
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
		vDelta = vmsVector::Normal(vNorm, vPos, vEnd);
		h = vmsVector::Dist(vPos, vEnd);
		}
	else {
		memcpy (&vPosf, &pVerts->vertex, sizeof (fVector3));
		memcpy (&vEndf, &pVerts [1 + d_rand () % (nVertices - 1)].vertex, sizeof (fVector3));
		memcpy (&v, &pVerts [1].vertex, sizeof (fVector3));
		vNormf = fVector::Normal(vPosf, v, vEndf);
		vPosf += vNormf * (1.0f / 64.0f);
		vEndf += vNormf * (1.0f / 64.0f);
		vDeltaf = fVector::Normal(vNormf, vPosf, vEndf);
		h = F2X (fVector::Dist(vPosf, vEndf));
		vPos[X] = F2X (vPosf[X]);
		vPos[Y] = F2X (vPosf[Y]);
		vPos[Z] = F2X (vPosf[Z]);
		vEnd[X] = F2X (vEndf[X]);
		vEnd[Y] = F2X (vEndf[Y]);
		vEnd[Z] = F2X (vEndf[Z]);
		}
	i = CreateLightning (1, &vPos, &vEnd, NULL /*&vDelta*/, nObject, 1000 + d_rand () % 2000, 0,
								h, h / 4 + d_rand () % 2, 0, 0, 20, 2, 1, 5, 0, 1, 0, 0, 0, 1, &color);
	bUpdate = 1;
	}
if (i >= 0) {
	plb = gameData.lightnings.buffer + i;
	glDisable (GL_CULL_FACE);
	if (bUpdate) {
		plb->nKey [0] = key.i [0];
		plb->nKey [1] = key.i [1];
		}
	if ((plb->nLightnings = UpdateLightning (plb->pl, plb->nLightnings, 0)))
		RenderLightning (plb->pl, plb->nLightnings, 0, -1);
	else
		DestroyLightnings (i, NULL, 1);
	}
}

//------------------------------------------------------------------------------
//eof
