/* $Id: tObject.c, v 1.9 2003/10/04 03:14:47 btb Exp $ */
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

#include "inferno.h"
#include "perlin.h"
#include "game.h"
#include "gr.h"
#include "stdlib.h"
#include "bm.h"
//#include "error.h"
#include "mono.h"
#include "3d.h"
#include "segment.h"
#include "texmap.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"
#include "textures.h"

#include "object.h"
#include "objsmoke.h"
#include "objrender.h"
#include "lightning.h"
#include "render.h"
#include "error.h"
#include "pa_enabl.h"
#include "timer.h"

#include "sounds.h"
#include "collide.h"

#include "lighting.h"
#include "interp.h"
#include "player.h"
#include "weapon.h"
#include "network.h"
#include "newmenu.h"
#include "multi.h"
#include "menu.h"
#include "args.h"
#include "text.h"
#include "vecmat.h"
#include "particles.h"
#include "hudmsg.h"
#include "oof.h"
#include "sphere.h"
#include "globvars.h"
#ifdef TACTILE
#include "tactile.h"
#endif
#include "ogl_init.h"
#include "input.h"
#include "automap.h"
#include "u_mem.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#ifdef _3DFX
#include "3dfx_des.h"
#endif

#define RENDER_TARGET_LIGHTNING 0
#define RENDER_LIGHTNING_CORONA 1

void CreateLightningPath (tLightning *pl, int bSeed);

//------------------------------------------------------------------------------

inline double f_rand (void)
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
do {
	vRand->p.x = F1_0 / 4 - d_rand ();
	vRand->p.y = F1_0 / 4 - d_rand ();
	vRand->p.z = F1_0 / 4 - d_rand ();
	} while (!(vRand->p.x || vRand->p.y || vRand->p.z));
return vRand;
}

//------------------------------------------------------------------------------

int ComputeChildEnd (vmsVector *vPos, vmsVector *vEnd, vmsVector *vDir, vmsVector *vParentDir, int nLength)
{
	int	nDot;
	int	i = 0;

//VmVecNormalize (vDir);
nLength = 3 * nLength / 4 + (int) (f_rand () * nLength / 4);
do {
	VmRandomVector (vDir);
	VmVecNormalize (vDir);
	nDot = VmVecDot (vDir, vParentDir);
	} while ((nDot > 9 * F1_0 / 10) || (nDot < 3 * F1_0 / 4));
VmVecScaleAdd (vEnd, vPos, vDir, nLength);
return nLength;
}

//------------------------------------------------------------------------------

void SetupLightning (tLightning *pl, int bInit)
{
	tLightning		*pp;
	tLightningNode	*pln;
	vmsVector		vPos, vDir, vDelta [2], v;
	int				i;

if (pl->bRandom) {
	vDir.p.x = F1_0 / 4 - d_rand ();
	vDir.p.y = F1_0 / 4 - d_rand ();
	vDir.p.z = F1_0 / 4 - d_rand ();
	VmVecNormalize (&vDir);
	VmVecScaleAdd (&pl->vEnd, &pl->vPos, &vDir, pl->nLength);
	}
else {
	VmVecSub (&vDir, &pl->vEnd, &pl->vPos);
	VmVecNormalize (&vDir);
	}
pl->vDir = vDir;
if (pl->nOffset) {
	i = pl->nOffset / 2 + (int) f_rand () * pl->nOffset / 2;
	VmVecScaleInc (&pl->vPos, &vDir, i);
	VmVecScaleInc (&pl->vEnd, &vDir, i);
	}
vPos = pl->vPos;
do {
	VmRandomVector (vDelta);
	} while (abs (VmVecDot (&vDir, vDelta)) > 9 * F1_0 / 10);
VmVecNormal (vDelta + 1, &vPos, &pl->vEnd, vDelta);
VmVecAdd (&v, &vPos, vDelta + 1);
VmVecNormal (vDelta, &vPos, &pl->vEnd, &v);
VmVecScaleFrac (&vDir, pl->nLength, pl->nNodeC * F1_0);
pl->nNodeC = abs (pl->nNodeC);
pl->iStep = 0;
#if 0
pl->nSteps = -abs (pl->nSteps);
#endif
if (pp = pl->pParent) {
	pl->nLength = ComputeChildEnd (&pl->vPos, &pl->vEnd, &pl->vDir, &pp->vDir, pp->nLength / pp->nChildC * 3 / 2);
	VmVecCopyScale (&vDir, &pl->vDir, pl->nLength / (pl->nNodeC - 1));
	}
for (i = pl->nNodeC, pln = pl->pNodes; i; i--, pln++) {
	pln->vBase =
	pln->vPos = vPos;	
	VmVecZero (&pln->vOffs);
	memcpy (pln->vDelta, vDelta, sizeof (vDelta));
	VmVecInc (&vPos, &vDir);
	if (bInit)
		pln->pChild = NULL;
	else if (pln->pChild)
		SetupLightning (pln->pChild, 0);
	}
}

//------------------------------------------------------------------------------

tLightning *AllocLightning (int nLightnings, vmsVector *vPos, vmsVector *vEnd, short nObject, 
									 int nLife, int nDelay, int nLength, int nAmplitude, int nOffset,
									 short nNodeC, short nChildC, short nDepth, short nSteps, 
									 short nSmoothe, char bClamp, tRgbaColorf *colorP, 
									 tLightning *pParent, short nNode)
{
	tLightning		*pfRoot, *pl;
	tLightningNode	*pln;
	int				h, i, bRandom = (vEnd == NULL);

if (!(nLife && nLength && (nNodeC > 4)))
	return NULL;
#if 0
if (!bRandom)
	nLightnings = 1;
#endif
if (!(pfRoot = (tLightning *) D2_ALLOC (nLightnings * sizeof (tLightning))))
	return NULL;
for (i = nLightnings, pl = pfRoot; i; i--, pl++) {
	if (!(pl->pNodes = (tLightningNode *) D2_ALLOC (nNodeC * sizeof (tLightningNode)))) {
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
		pl->nSegment = OBJECTS [nObject].nSegment;
		}
	pl->pParent = pParent;
	pl->nNode = nNode;
	pl->nNodeC = nNodeC;
	pl->nChildC = nChildC;
	pl->nDepth = nDepth;
	pl->nLife = nLife;
	h = abs (nLife);
	pl->nTTL = (bRandom) ? 3 * h / 4 + (int) (f_rand () * h / 2) : h;
	pl->nDelay = abs (nDelay);
	pl->nLength = (bRandom) ? 3 * nLength / 4 + (int) (f_rand () * nLength / 2) : nLength;
	pl->nAmplitude = nAmplitude;
	pl->nOffset = nOffset;
	pl->nSteps = nSteps;
	pl->nSmoothe = nSmoothe;
	pl->bClamp = bClamp;
	pl->iStep = 0;
	pl->color = *colorP;
	pl->vPos = *vPos;
	if (!(pl->bRandom = bRandom))
		pl->vEnd = *vEnd;
	SetupLightning (pl, 1);
	if (nDepth) {
		tLightningNode *plh;
		vmsVector		vEnd;
		int				l, nNode;
		double			nStep, j;

		nStep = (double) (pl->nNodeC - pl->nNodeC / nChildC) / (double) nChildC;
		l = nLength / nChildC;
		plh = pl->pNodes;
		for (h = pl->nNodeC, j = nStep / 2; j < h; j += nStep) {
			nNode = (int) j + 1 - rand () % 3;
			pln = plh + nNode;
			pln->pChild = AllocLightning (1, &pln->vPos, &vEnd, -1, pl->nLife, 0, l, nAmplitude / nChildC * 2, 0,
													pl->nNodeC / nChildC, nChildC / 5, nDepth - 1, nSteps / 2, nSmoothe, bClamp, colorP, pl, nNode);
			}
		}
	}
return pfRoot;
}

//------------------------------------------------------------------------------

int CreateLightning (int nLightnings, vmsVector *vPos, vmsVector *vEnd, short nObject, 
							int nLife, int nDelay, int nLength, int nAmplitude, int nOffset,
							short nNodeC, short nChildC, short nDepth, short nSteps, 
							short nSmoothe, char bClamp, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNINGS) {
		tLightningBundle	*plb;
		tLightning			*pl;
		int					n;

	if (!nLightnings)
		return -1;
	if (gameData.lightnings.iFree < 0)
		return -1;
	srand (gameStates.app.nSDLTicks);
	if (!(pl = AllocLightning (nLightnings, vPos, vEnd, nObject, nLife, nDelay, nLength, nAmplitude, 
										nOffset, nNodeC, nChildC, nDepth, nSteps, nSmoothe, bClamp, colorP, NULL, -1)))
		return -1;
	n = gameData.lightnings.iFree;
	plb = gameData.lightnings.buffer + n;
	gameData.lightnings.iFree = plb->nNext;
	plb->nNext = gameData.lightnings.iUsed;
	gameData.lightnings.iUsed = n;
	plb->pl = pl;
	plb->nLightnings = nLightnings;
	plb->bDestroy = 0;
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

	for (i = abs (pl->nNodeC), pln = pl->pNodes; i; i--, pln++)
		DestroyLightningNodes (pln->pChild);
	D2_FREE (pl->pNodes);
	pl->nNodeC = 0;
	}
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
	gameData.lightnings.objects [iLightning] = -1;
	i = plb->nLightnings;
	}
for (; i; i--, pl++)
	DestroyLightningNodes (pl);
if (plb) {
	plb->nLightnings = 0;
	D2_FREE (plb->pl);
	}
}

//------------------------------------------------------------------------------

int DestroyAllLightnings (void)
{
	int	i, j;

if (gameData.lightnings.bDestroy >= 0)
	gameData.lightnings.bDestroy = 1;
else {
	for (i = gameData.lightnings.iUsed; i >= 0; i = j) {
		j = gameData.lightnings.buffer [i].nNext;
		DestroyLightnings (-i - 1, NULL, 1);
		}
	InitLightnings ();
	}
return 1;
}

//------------------------------------------------------------------------------

int ComputeAttractor (vmsVector *vAttract, vmsVector *vDest, vmsVector *vPos, int nMinDist, int i)
{
	int nDist;

VmVecSub (vAttract, vDest, vPos);
nDist = VmVecMag (vAttract) / i;
if (!nMinDist)
	VmVecScale (vAttract, F1_0 / i * 2);	// scale attractor with inverse of remaining distance
else {
	if (nDist < nMinDist)
		nDist = nMinDist;
	VmVecScale (vAttract, F1_0 / i / 2);	// scale attractor with inverse of remaining distance
	}
return nDist;
}

//------------------------------------------------------------------------------

vmsVector *CreateLightningPathPoint (vmsVector *vOffs, vmsVector *vAttract, int nDist)
{
	int nDot;

if (nDist < F1_0 / 16)
	return VmRandomVector (vOffs);
do {
	VmRandomVector (vOffs);
	nDot = VmVecDot (vAttract, vOffs);
	} while (!nDot);
if (nDot < 0)
	VmVecNegate (vOffs);
return vOffs;
}

//------------------------------------------------------------------------------

vmsVector *SmootheLightningOffset (vmsVector *vOffs, vmsVector *vPrevOffs, int nDist, int nSmoothe)
{
if (nSmoothe) {
		int nMag = VmVecMag (vOffs);

	if (nSmoothe > 0)
		VmVecScaleFrac (vOffs, nSmoothe * nDist, nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
	else 
		VmVecScaleFrac (vOffs, nDist, nSmoothe * nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
	nMag = VmVecMag (vPrevOffs);
	VmVecInc (vOffs, vPrevOffs);
	nMag = VmVecMag (vOffs);
	VmVecScaleFrac (vOffs, nDist, nMag);
	nMag = VmVecMag (vOffs);
	}
return vOffs;
}

//------------------------------------------------------------------------------

vmsVector *AttractLightningPathPoint (vmsVector *vOffs, vmsVector *vAttract, vmsVector *vPos, int nDist, int i, int bJoinPaths)
{
	int nMag = VmVecMag (vOffs);
// attract offset vector by scaling it with distance from attracting node
VmVecScaleFrac (vOffs, i * nDist / 2, nMag);	//scale offset vector with distance to attractor (the closer, the smaller)
VmVecInc (vOffs, vAttract);	//add offset and attractor vectors (attractor is the bigger the closer)
nMag = VmVecMag (vOffs);
VmVecScaleFrac (vOffs, bJoinPaths ? nDist / 2 : nDist, nMag);	//rescale to desired path length
VmVecInc (vPos, vOffs);
return vPos;
}

//------------------------------------------------------------------------------

int ClampLightningPathPoint (vmsVector *vPos, vmsVector *vBase, int nAmplitude)
{
	vmsVector	vRoot;
	int			nDist = VmPointLineIntersection (&vRoot, vBase, vBase + 1, vPos, NULL, 0);

if (nDist < nAmplitude)
	return nDist;
VmVecDec (vPos, &vRoot);	//create vector from intersection to current path point
VmVecScaleFrac (vPos, nAmplitude, nDist);	//scale down to length nAmplitude
VmVecInc (vPos, &vRoot);	//recalculate path point
return nAmplitude;
}

//------------------------------------------------------------------------------

vmsVector ComputeLightningNode (tLightningNode *pln, vmsVector *vPos, vmsVector *vDest, vmsVector *vBase, vmsVector *vPrevOffs,
									 int nSteps, int nAmplitude, int nMinDist, int i, int nSmoothe, int bClamp)
{
	vmsVector	vAttract, vOffs;
	int			nDist = ComputeAttractor (&vAttract, vDest, vPos, nMinDist, i);

CreateLightningPathPoint (&vOffs, &vAttract, nDist);
if (vPrevOffs)
	SmootheLightningOffset (&vOffs, vPrevOffs, nDist, nSmoothe);
else if (pln->vOffs.p.x || pln->vOffs.p.z || pln->vOffs.p.z) {
	VmVecScaleInc (&vOffs, &pln->vOffs, 2 * F1_0);
	VmVecScale (&vOffs, F1_0 / 3);
	}
if (nDist > F1_0 / 16)
	AttractLightningPathPoint (&vOffs, &vAttract, vPos, nDist, i, 0);
if (bClamp)
	ClampLightningPathPoint (vPos, vBase, nAmplitude);
pln->vNewPos = *vPos;
return vOffs;
}

//------------------------------------------------------------------------------

vmsVector ComputePerlinNode (tLightningNode *pln, int nSteps, int nAmplitude, int *nSeed, double phi, double i)
{
double dx = PerlinNoise1D (i, 0.25, 6, nSeed [0]);
double dy = PerlinNoise1D (i, 0.25, 6, nSeed [1]);
phi = sin (phi * Pi);
phi = sqrt (phi);
dx *= nAmplitude * phi;
dy *= nAmplitude * phi;
VmVecScaleAdd (&pln->vNewPos, &pln->vBase, pln->vDelta, (int) dx);
VmVecScaleInc (&pln->vNewPos, pln->vDelta + 1, (int) dy);
VmVecSub (&pln->vOffs, &pln->vNewPos, &pln->vPos);
VmVecScale (&pln->vOffs, F1_0 / nSteps);
return pln->vOffs;
}

//------------------------------------------------------------------------------

void SmootheLightningPath (tLightning *pl)
{
	tLightningNode	*pfh, *pfi, *pfj;
	int			i, j;

for (i = pl->nNodeC - 1, j = 0, pfi = pl->pNodes, pfh = NULL; j < i; j++) {
	pfj = pfh;
	pfh = pfi++;
	if (j) {
		pfh->vNewPos.p.x = pfj->vNewPos.p.x / 4 + pfh->vNewPos.p.x / 2 + pfi->vNewPos.p.x / 4;
		pfh->vNewPos.p.y = pfj->vNewPos.p.y / 4 + pfh->vNewPos.p.y / 2 + pfi->vNewPos.p.y / 4;
		pfh->vNewPos.p.z = pfj->vNewPos.p.z / 4 + pfh->vNewPos.p.z / 2 + pfi->vNewPos.p.z / 4;
		}
	}
}

//------------------------------------------------------------------------------

void ComputeLightningNodeOffsets (tLightning *pl)
{
	tLightningNode	*pln;
	int			i, nSteps = pl->nSteps;

for (i = pl->nNodeC - 1 - !pl->bRandom, pln = pl->pNodes + 1; i; i--, pln++) {
	VmVecSub (&pln->vOffs, &pln->vNewPos, &pln->vPos);
	VmVecScale (&pln->vOffs, F1_0 / nSteps);
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
vBase [1] = pl->pNodes [pl->nNodeC - 1].vPos;
for (i = pl->nNodeC - 1 - !pl->bRandom, pln = pl->pNodes + 1; i; i--, pln++) {
	nDist = VmVecDist (&pln->vNewPos, &pln->vPos);
	if (nMaxDist < nDist) {
		nMaxDist = nDist;
		h = i;
		}
	}
if ((nMaxDist < nAmplitude) /*&& (nMaxDist > 3 * nAmplitude / 4)*/ && (rand () < RAND_MAX / 4))
	for (i = pl->nNodeC - 1 - !pl->bRandom, pln = pl->pNodes + 1; i; i--, pln++) {
		VmVecScaleFrac (&pln->vOffs, nAmplitude, nMaxDist);
		}
}

//------------------------------------------------------------------------------

void CreateLightningPath (tLightning *pl, int bSeed)
{
	tLightningNode	*pfh, *pln [2];
	int			h, i, j, nSteps, nSmoothe, bClamp, nMinDist, nAmplitude, bPrevOffs [2] = {0,0};
	vmsVector	vPos [2], vBase [2], vPrevOffs [2];
	double		phi;

	static int	nSeed [2];

vBase [0] = vPos [0] = pl->vPos;
vBase [1] = vPos [1] = pl->vEnd;
if (bSeed) {
	nSeed [0] = rand ();
	nSeed [1] = rand ();
	nSeed [0] *= rand ();
	nSeed [1] *= rand ();
	}
#ifdef _DEBUG
else
	bSeed = 0;
#endif
nSteps = pl->nSteps;
nSmoothe = pl->nSmoothe;
bClamp = pl->bClamp;
nAmplitude = pl->nAmplitude;

pfh = pl->pNodes;
pfh->vNewPos = pfh->vPos;
VmVecZero (&pfh->vOffs);
if (pl->bRandom) {
	if (gameOpts->render.lightnings.nQuality) {
		nAmplitude *= 4;
		for (h = pl->nNodeC, i = 0, pfh = pl->pNodes; i < h; i++, pfh++) {
			phi = (double) i / (double) (h - 1);
			ComputePerlinNode (pfh, nSteps, nAmplitude, nSeed, phi / 2, phi * 5);
			}
		}
	else {
		nMinDist = pl->nLength / (pl->nNodeC - 1);
		for (i = pl->nNodeC - 1, pfh = pl->pNodes + 1; i; i--, pfh++, bPrevOffs [0] = 1) 
			*vPrevOffs = ComputeLightningNode (pfh, vPos, vPos + 1, vBase, bPrevOffs [0] ? vPrevOffs : NULL, nSteps, nAmplitude, nMinDist, i, nSmoothe, bClamp);
		}
	}
else {
	pfh = pl->pNodes + pl->nNodeC - 1;
	pfh->vNewPos = pfh->vPos;
	VmVecZero (&pfh->vOffs);
	nAmplitude *= 4;
	if (gameOpts->render.lightnings.nQuality) {
		for (h = pl->nNodeC, i = 0, pfh = pl->pNodes; i < h; i++, pfh++) {
			phi = (double) i / (double) (h - 1);
			ComputePerlinNode (pfh, nSteps, nAmplitude, nSeed, phi, phi * 10);
			}
		}
	else {
		for (i = pl->nNodeC - 1, j = 0, pln [0] = pl->pNodes + 1, pln [1] = pl->pNodes + i - 1; i; i--, j = !j) {
			pfh = pln [j];
			vPrevOffs [j] = ComputeLightningNode (pfh, vPos + j, vPos + !j, vBase, bPrevOffs [j] ? vPrevOffs + j : NULL, nSteps, nAmplitude, 0, i, nSmoothe, bClamp);
			bPrevOffs [j] = 1;
			if (pln [1] <= pln [0])
				break;
			if (j)
				pln [1]--;
			else
				pln [0]++;
			}
		}
	}
if (!gameOpts->render.lightnings.nQuality) {
	SmootheLightningPath (pl);
	ComputeLightningNodeOffsets (pl);
	BumpLightningPath (pl);
	}
}

//------------------------------------------------------------------------------

int UpdateLightning (tLightning *pl, int nLightnings)
{
	tLightningNode	*pln;
	int				h, i, j, bSeed, bInit;
	tLightning		*pfRoot = pl;

	static int nDepth = -1;

if (!pl)
	return 0;
#if 0
if (!gameStates.app.tick40fps.bTick)
	return -1;
#endif
nDepth++;
for (i = 0; i < nLightnings; i++, pl++) {
	pl->nTTL -= gameStates.app.tick40fps.nTime;
	if (pl->nNodeC > 0) {
		if (bInit = (pl->nSteps < 0))
			pl->nSteps = -pl->nSteps;
		if (!pl->iStep) {
#if 1
			bSeed = 1;
			do {
				LogErr ("CreateLightningPath ...\n");
				CreateLightningPath (pl, bSeed);
				LogErr ("done\n");
#if 1
				break;
#else
				if (!bSeed)
					break;
				for (j = pl->nNodeC - 1 - !pl->bRandom, pln = pl->pNodes + 1; j; j--, pln++)
					if (VmVecDot (&pln [0].vOffs, &pln [1].vOffs) < 65500)
						break;
				bSeed = !bSeed;
#endif
				} while (!j);
#else
			for (j = pl->nNodeC - 1 - !pl->bRandom, pln = pl->pNodes + 1; j; j--, pln++)
				VmVecNegate (&pln->vOffs);
#endif
			pl->iStep = pl->nSteps;
			}
		for (j = pl->nNodeC - 1 - !pl->bRandom, pln = pl->pNodes + 1; j; j--, pln++) {
			if (bInit)
				pln->vPos = pln->vNewPos;
			else
				VmVecInc (&pln->vPos, &pln->vOffs);
			if (pln->pChild) {
				MoveLightnings (1, pln->pChild, &pln->vPos, pl->nSegment, 0, 0);
				UpdateLightning (pln->pChild, 1);
				}
			}
		(pl->iStep)--;
		}
#if 1
	if (pl->nTTL <= 0) {
		if (pl->nLife < 0) {
			if (pl->bRandom) {
				if (0 > (pl->nNodeC = -pl->nNodeC)) {
					h = pl->nDelay / 2;
					pl->nTTL = h + (int) (f_rand () * h);
					}
				else {
					h = -pl->nLife;
					pl->nTTL = 3 * h / 4 + (int) (f_rand () * h / 2);
					SetupLightning (pl, 0);
					}
				}
			else {
				pl->nTTL = -pl->nLife;
				pl->nNodeC = abs (pl->nNodeC);
				SetupLightning (pl, 0);
				}
			}
		else {
			DestroyLightningNodes (pl);
			if (!--nLightnings)
				return 0;
			if (i < nLightnings) {
				*pl = pl [nLightnings - i - 1];
				pl--;
				i--;
				}
			}
		}
#endif
	}
nDepth--;
return nLightnings;
}

//------------------------------------------------------------------------------

#define LIMIT_FLASH_FPS	1
#define FLASH_SLOWMO 1

void UpdateLightnings (void)
{
if (EGI_FLAG (bUseLightnings, 0, 0, 1)) {
		tLightningBundle	*plb;
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
	for (i = gameData.lightnings.iUsed; i >= 0; i = n) {
		plb = gameData.lightnings.buffer + i;
		n = plb->nNext;
		if (plb->bDestroy || !(plb->nLightnings = UpdateLightning (plb->pl, plb->nLightnings)))
			DestroyLightnings (i, NULL, 1);
		}
	}
}

//------------------------------------------------------------------------------

void MoveLightnings (int i, tLightning *pl, vmsVector *vNewPos, short nSegment, int bStretch, int bFromEnd)
{
if (EGI_FLAG (bUseLightnings, 0, 0, 1)) {
		tLightningNode	*pln;
		vmsVector		vDelta, vOffs;
		int				h, j, nStep;

	if (pl) 
		i = 1;
	else if (IsUsedLightning (i)) {
		tLightningBundle	*plb = gameData.lightnings.buffer + i;
		pl = plb->pl;
		i = plb->nLightnings;
		}
	else
		return;
	for (; i; i--, pl++) {
		pl->nSegment = nSegment;
		if (bFromEnd)
			VmVecSub (&vDelta, vNewPos, &pl->vEnd);
		else
			VmVecSub (&vDelta, vNewPos, &pl->vPos);
		if (vDelta.p.x || vDelta.p.y || vDelta.p.z) {
			if (bStretch)
				vOffs = vDelta;
			else if (bFromEnd) {
				VmVecInc (&pl->vPos, &vDelta);
				pl->vEnd = *vNewPos;
				}
			else {
				VmVecInc (&pl->vEnd, &vDelta);
				pl->vPos = *vNewPos;
				}
			VmVecSub (&pl->vDir, &pl->vEnd, &pl->vPos);
			pl->nLength = VmVecMag (&pl->vDir);
			if (0 < (h = pl->nNodeC)) {
				nStep = F1_0 / h;
				for (j = h, pln = pl->pNodes; j; j--, pln++) {
					if (bStretch) {
						vDelta = vOffs;
						if (bFromEnd)
							VmVecCopyScale (&vDelta, &vOffs, (h - j + 1) * nStep);
						else
							VmVecCopyScale (&vDelta, &vOffs, j * nStep);
						}
					else
						VmVecInc (&pln->vNewPos, &vDelta);
					VmVecInc (&pln->vBase, &vDelta);
					VmVecInc (&pln->vPos, &vDelta);
					MoveLightnings (-1, pln->pChild, &pln->vPos, nSegment, bStretch, bFromEnd);
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void MoveObjectLightnings (tObject *objP)
{
MoveLightnings (gameData.lightnings.objects [OBJ_IDX (objP)], NULL, &objP->position.vPos, objP->nSegment, 0, 0);
}

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

#define LIGHTNING_VERT_ARRAYS 1

void RenderLightningCorona (fVector *vPosf, int nScale, int bDrawArrays, int bStart, int bEnd)
{
	static tUVLf	uvlCorona [3][4] = {
		{{{0,0.45f,1}},{{1,0.45f,1}},{{1,0.55f,1}},{{0,0.55f,1}}},
		{{{0,0.2f,1}},{{1,0.2f,1}},{{1,0.5f,1}},{{0,0.5f,1}}},
		{{{0,0.5f,1}},{{1,0.5f,1}},{{1,0.8f,1}},{{0,0.8f,1}}}
		};

	static fVector	vEye = {{0,0,0}};

	static fVector	vCorona [6] = {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}};

	fVector	vn, vd;
	int		i, j = bStart + 2 * bEnd;

VmVecNormalf (&vn, vPosf, vPosf + 1, &vEye);
if (!nScale)
	VmVecScalef (&vn, &vn, 0.5f);
if (bStart) {
	VmVecAddf (vCorona, vPosf, &vn);
	VmVecSubf (vCorona + 1, vPosf, &vn);
	VmVecSubf (&vd, vPosf, vPosf + 1);
	VmVecNormalizef (&vd, &vd);
	if (!nScale)
		VmVecScalef (&vd, &vd, 0.5f);
	VmVecIncf (vCorona, &vd);
	VmVecIncf (vCorona + 1, &vd);
	}
else {
	vCorona [0] = vCorona [3];
	vCorona [1] = vCorona [2];
	}
VmVecAddf (vCorona + 3, vPosf + 1, &vn);
VmVecSubf (vCorona + 2, vPosf + 1, &vn);
if (bEnd) {
	VmVecSubf (&vd, vPosf + 1, vPosf);
	VmVecNormalizef (&vd, &vd);
	if (nScale)
		VmVecScalef (&vd, &vd, 0.5f);
	VmVecIncf (vCorona + 2, &vd);
	VmVecIncf (vCorona + 3, &vd);
	}
if (bDrawArrays) {
	glVertexPointer (3, GL_FLOAT, sizeof (fVector), vCorona);
	glTexCoordPointer (2, GL_FLOAT, sizeof (tUVLf), uvlCorona + j);
	glDrawArrays (GL_QUADS, 0, 4);
	}
else {
#if 1
	glBegin (GL_QUADS);
	for (i = 0; i < 4; i++) {
		glTexCoord2fv ((GLfloat *) (uvlCorona [j] + i));
		glVertex3fv ((GLfloat *) (vCorona + i));
		}
	glEnd ();
#endif
#if 0
	glDisable (GL_TEXTURE_2D);
	glDisable (GL_BLEND);
	if (bStart)
		glColor3f (1,0,0);
	else
		glColor3f (0,0,1);
	glBegin (GL_LINE_LOOP);
	for (i = 0; i < 4; i++) {
		glTexCoord2fv ((GLfloat *) (uvlCorona + i));
		glVertex3fv ((GLfloat *) (vCorona + i));
		}
	glEnd ();
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_BLEND);
#endif
	}
}

//------------------------------------------------------------------------------

void RenderLightning (tLightning *pl, int nLightnings, short nDepth, int bDepthSort)
{
	tLightningNode	*pln;
	fVector		vPosf [2] = {{{0,0,0}},{{0,0,0}}};
	int			i;
#if RENDER_LIGHTNING_CORONA
	int			h, j, bDrawArrays;
#endif
	tRgbaColorf	color;
	float			f;

if (!pl)
	return;
if (bDepthSort) {
	for (; nLightnings; nLightnings--, pl++) {
		if (pl->nNodeC < 0)
			continue;
		RIAddLightnings (pl, 1, nDepth);
		for (i = pl->nNodeC, pln = pl->pNodes; i; i--, pln++)
			RenderLightning (pln->pChild, 1, nDepth + 1, 1);
		}
	}
else {
	glEnable (GL_BLEND);
	OglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (gameOpts->render.bDepthSort < 1) {
		glDepthMask (0);
		glDisable (GL_CULL_FACE);
		}
	for (; nLightnings; nLightnings--, pl++) {
		if (pl->nNodeC < 0)
			continue;
		color = pl->color;
		if (pl->nLife > 0) {
			if ((i = pl->nLife - pl->nTTL) < 250)
				color.alpha *= (float) i / 250.0f;
			else if (pl->nTTL < pl->nLife / 3)
				color.alpha *= (float) pl->nTTL / (float) (pl->nLife / 3);
			}
		color.red *= (float) (0.9 + f_rand () / 5);
		color.green *= (float) (0.9 + f_rand () / 5);
		color.blue *= (float) (0.9 + f_rand () / 5);
		if (nDepth)
			color.alpha /= 2;
		f = (color.red * 3 + color.green * 5 + color.blue * 2) / 10;
		f = (float) sqrt (f);
#if RENDER_LIGHTNING_CORONA
		if (gameOpts->render.lightnings.bCoronas) {
			bDrawArrays = 0; //OglEnableClientStates (1, 0);
			glEnable (GL_TEXTURE_2D);
			if (LoadCorona () && !OglBindBmTex (bmpCorona, 1, -1)) {
				OglTexWrap (bmpCorona->glTexture, GL_CLAMP);
				for (h = 0; h < 2; h++) {
					if (h) {
						glColor4f (color.red / 2, color.green / 2, color.blue / 2, color.alpha);
						}
					else {
						glColor4f (1, 1, 1, color.alpha / 2);
						}
					for (i = pl->nNodeC - 1, j = 0, pln = pl->pNodes; j <= i; j++, pln++) {
						vPosf [0] = vPosf [1];
						VmsVecToFloat (vPosf + 1, &pln->vPos);
						G3TransformPointf (vPosf + 1, vPosf + 1, 0);
						if (j)
							RenderLightningCorona (vPosf, h, bDrawArrays, j == 1, j == i);
						}
					}
				}
			if (bDrawArrays)
				OglDisableClientStates (1, 0);
			glDisable (GL_TEXTURE_2D);
			}
#endif
#if 1
		if (nDepth)
			color.alpha /= 2;
		OglBlendFunc (GL_SRC_ALPHA, GL_ONE);
		glColor4fv ((GLfloat *) &color);
		glLineWidth ((GLfloat) (nDepth ? 2 : 3));
		glEnable (GL_SMOOTH);
		glBegin (GL_LINE_STRIP);
		for (i = pl->nNodeC, pln = pl->pNodes; i; i--, pln++) {
			VmsVecToFloat (vPosf, &pln->vPos);
			G3TransformPointf (vPosf, vPosf, 0);
			glVertex3fv ((GLfloat *) vPosf);
			}
		glEnd ();
#endif
#if defined (_DEBUG) && RENDER_TARGET_LIGHTNING
		glColor3f (1,0,0,1);
		glLineWidth (1);
		glBegin (GL_LINE_STRIP);
		for (i = pl->nNodeC, pln = pl->pNodes; i; i--, pln++) {
			VmsVecToFloat (vPosf, &pln->vNewPos);
			G3TransformPointf (vPosf, vPosf, 0);
			glVertex3fv ((GLfloat *) vPosf);
			}
		glEnd ();
#endif
		for (i = pl->nNodeC, pln = pl->pNodes; i; i--, pln++)
			RenderLightning (pln->pChild, 1, nDepth + 1, 0);
		}
	if (gameOpts->render.bDepthSort < 1) {
		glEnable (GL_CULL_FACE);
		glDepthMask (1);
		}
	glLineWidth (1);
	glDisable (GL_SMOOTH);
	OglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

//------------------------------------------------------------------------------

void RenderLightnings (void)
{
if (SHOW_LIGHTNINGS) {
		tLightningBundle	*plb;
		int			i, n;

	if (EGI_FLAG (bShadows, 0, 1, 0)) 
		glDisable (GL_STENCIL_TEST);
	for (i = gameData.lightnings.iUsed; i >= 0; i = n) {
		plb = gameData.lightnings.buffer + i;
		n = plb->nNext;
		RenderLightning (plb->pl, plb->nLightnings, 0, gameOpts->render.bDepthSort > 0);
		}
	if (EGI_FLAG (bShadows, 0, 1, 0)) 
		glEnable (GL_STENCIL_TEST);
	}
}

//------------------------------------------------------------------------------

void DoLightningFrame (void)
{
if (gameData.lightnings.bDestroy) {
	gameData.lightnings.bDestroy = -1;
	DestroyAllLightnings ();
	}
else {
	UpdateLightnings ();
	UpdateOmegaLightnings (NULL, NULL);
	}
}

//------------------------------------------------------------------------------

void SetLightningSegLight (short nSegment, vmsVector *vPos, tRgbaColorf *colorP)
{
	tLightningLight	*pll = gameData.lightnings.lights + nSegment;

if (pll->nFrameFlipFlop != gameStates.render.nFrameFlipFlop) {
	memset (pll, 0, sizeof (*pll));
	pll->nFrameFlipFlop = gameStates.render.nFrameFlipFlop;
	pll->nNext = gameData.lightnings.nFirstLight;
	gameData.lightnings.nFirstLight = nSegment;
	}
pll->nLights++;
VmVecInc (&pll->vPos, vPos);
pll->color.red += colorP->red;
pll->color.green += colorP->green;
pll->color.blue += colorP->blue;
pll->color.alpha += colorP->alpha;
}

//------------------------------------------------------------------------------

int SetLightningLight (tLightning *pl, int i)
{
	tLightningNode	*pln;
	vmsVector		*vPos;
	short				nSegment;
	int				j, nLights;
	double			h, nStep;

for (nLights = 0; i; i--, pl++) {
	if (0 < (j = pl->nNodeC)) {
		if (!(nStep = (double) (j - 1) / ((double) pl->nLength / F1_0)))
			nStep = j - 1;
		nSegment = pl->nSegment;
		pln = pl->pNodes;
		SetLightningSegLight (nSegment, &pln->vPos, &pl->color);
		nLights++;
		for (h = 0; h < j; h += nStep) {
			vPos = &pln [(int) h].vPos;
			if (0 > (nSegment = FindSegByPoint (vPos, nSegment, 0)))
				break;
			SetLightningSegLight (nSegment, vPos, &pl->color);
			nLights++;
			}
		}
	}
return nLights;
}

//------------------------------------------------------------------------------

void ResetLightningLights (void)
{
if (EGI_FLAG (bUseLightnings, 0, 0, 1)) {
		tLightningLight	*pll;
		int					i, nLights = 0;

	for (i = gameData.lightnings.nFirstLight; i >= 0; ) {
		pll = gameData.lightnings.lights + i;
		i = pll->nNext;
		pll->nNext = -1;
		pll->color.red =
		pll->color.green =
		pll->color.blue = 0;
		pll->nBrightness = 0;
		}
	gameData.lightnings.nFirstLight = -1;
	}
}

//------------------------------------------------------------------------------

void SetLightningLights (void)
{
if (EGI_FLAG (bUseLightnings, 0, 0, 1)) {
		tLightningBundle	*plb;
		tLightningLight	*pll;
		int					i, n, nLights = 0;

	gameData.lightnings.nFirstLight = -1;
	for (i = gameData.lightnings.iUsed; i >= 0; i = n) {
		plb = gameData.lightnings.buffer + i;
		n = plb->nNext;
		nLights += SetLightningLight (plb->pl, plb->nLightnings);
		}
	if (nLights) {
		for (i = gameData.lightnings.nFirstLight; i >= 0; i = pll->nNext) {
			pll = gameData.lightnings.lights + i;
			n = pll->nLights;
			VmVecScale (&pll->vPos, F1_0 / n);
			pll->color.red /= n;
			pll->color.green /= n;
			pll->color.blue /= n;
			pll->nBrightness = fl2f (sqrt ((pll->color.red * 3 + pll->color.green * 5 + pll->color.blue * 2) * pll->color.alpha));
			}
		}
	}
}

//------------------------------------------------------------------------------

void CreateMissileLightnings (tObject *objP, tRgbaColorf *colorP, int nRods, int nRad)
{
if (EGI_FLAG (bUseLightnings, 0, 0, 1)) {
	//gameData.lightnings.objects [OBJ_IDX (objP)] = 
		CreateLightning (
			nRods, &objP->position.vPos, NULL, OBJ_IDX (objP), 750, 0, nRad, F1_0 * 4, 2 * F1_0, 50, 5, 1, 3, 1, 1, colorP);
	}
}

//------------------------------------------------------------------------------

void CreateShakerLightnings (tObject *objP)
{
tRgbaColorf color = {0.1f, 0.1f, 0.8f, 0.3f};

CreateMissileLightnings (objP, &color, 20, 20 * F1_0);
}

//------------------------------------------------------------------------------

void CreateShakerMegaLightnings (tObject *objP)
{
tRgbaColorf color = {0.1f, 0.1f, 0.8f, 0.3f};

CreateMissileLightnings (objP, &color, 15, 15 * F1_0);
}

//------------------------------------------------------------------------------

void CreateMegaLightnings (tObject *objP)
{
tRgbaColorf color = {0.8f, 0.1f, 0.1f, 0.3f};

CreateMissileLightnings (objP, &color, 20, 15 * F1_0);
}

//------------------------------------------------------------------------------

void CreateRobotLightnings (tObject *objP, tRgbaColorf *colorP)
{
if (EGI_FLAG (bUseLightnings, 0, 0, 1)) {
		int h, n, i = OBJ_IDX (objP);
		tLightningBundle	*plb;

	n = f2ir (RobotDefaultShields (objP));
	h = f2ir (objP->shields) * 100 / n;
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
	gameData.lightnings.objects [i] = CreateLightning (
		n, &objP->position.vPos, NULL, OBJ_IDX (objP), -1000, 4000, objP->size, objP->size / 8, 0, 20, 2, 1, 4, 1, 1, colorP);
	}
}

//------------------------------------------------------------------------------
//eof
