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
#include "transprender.h"
#include "renderthreads.h"
#include "lightning.h"
#include "render.h"
#include "error.h"
#include "pa_enabl.h"
#include "timer.h"

#include "sounds.h"
#include "collide.h"

#include "light.h"
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
#include "ogl_defs.h"
#include "ogl_lib.h"
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
if ( pln->pChild == (tLightning *) (size_t) 0xfeeefeee)
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
	vmsVector	vr;

do {
	vr.p.x = F1_0 / 4 - d_rand ();
	vr.p.y = F1_0 / 4 - d_rand ();
	vr.p.z = F1_0 / 4 - d_rand ();
} while (!(vr.p.x && vr.p.y && vr.p.z));
VmVecNormalize (&vr);
*vRand = vr;
return vRand;
}

//------------------------------------------------------------------------------

#define SIGN(_i)	(((_i) < 0) ? -1 : 1)

#define VECSIGN(_v)	(SIGN ((_v).p.x) * SIGN ((_v).p.y) * SIGN ((_v).p.z))

//------------------------------------------------------------------------------

vmsVector *DirectedRandomVector (vmsVector *vRand, vmsVector *vDir, int nMinDot, int nMaxDot)
{
	vmsVector	vr, vd, vSign;
	int			nDot, nSign, i = 0;

VmVecCopyNormalize (&vd, vDir);
vSign.p.x = vd.p.x ? vd.p.x / abs (vd.p.x) : 0;
vSign.p.y = vd.p.y ? vd.p.y / abs (vd.p.y) : 0;
vSign.p.z = vd.p.z ? vd.p.z / abs (vd.p.z) : 0;
nSign = VECSIGN (vd);
do {
	VmRandomVector (&vr);
	nDot = VmVecDot (&vr, &vd);
	if (++i == 100)
		i = 0;
	} while ((nDot > nMaxDot) || (nDot < nMinDot));
*vRand = vr;
return vRand;
}

//------------------------------------------------------------------------------

int ComputeChildEnd (vmsVector *vPos, vmsVector *vEnd, vmsVector *vDir, vmsVector *vParentDir, int nLength)
{
nLength = 3 * nLength / 4 + (int) (f_rand () * nLength / 4);
DirectedRandomVector (vDir, vParentDir, 3 * F1_0 / 4, 9 * F1_0 / 10);
VmVecScaleAdd (vEnd, vPos, vDir, nLength);
return nLength;
}

//------------------------------------------------------------------------------

void SetupLightning (tLightning *pl, int bInit)
{
	tLightning		*pp;
	tLightningNode	*pln;
	vmsVector		vPos, vDir, vRefDir, vDelta [2], v;
	int				i, l;

pl->vPos = pl->vBase;
if (pl->bRandom) {
	if (!pl->nAngle)
		VmRandomVector (&vDir);
	else {
		int nMinDot = F1_0 - pl->nAngle * F1_0 / 90;
		VmVecSub (&vRefDir, &pl->vRefEnd, &pl->vPos);
		VmVecNormalize (&vRefDir);
		do {
			VmRandomVector (&vDir);
			} while (VmVecDot (&vRefDir, &vDir) < nMinDot);
		}
	VmVecScaleAdd (&pl->vEnd, &pl->vPos, &vDir, pl->nLength);
	}
else {
	VmVecSub (&vDir, &pl->vEnd, &pl->vPos);
	VmVecNormalize (&vDir);
	}
pl->vDir = vDir;
if (pl->nOffset) {
	i = pl->nOffset / 2 + (int) (f_rand () * pl->nOffset / 2);
	VmVecScaleInc (&pl->vPos, &vDir, i);
	VmVecScaleInc (&pl->vEnd, &vDir, i);
	}
vPos = pl->vPos;
if (pl->bInPlane) {
	vDelta [0] = pl->vDelta;
	VmVecZero (vDelta + 1);
	}
else {
	do {
		VmRandomVector (vDelta);
		} while (abs (VmVecDot (&vDir, vDelta)) > 9 * F1_0 / 10);
	VmVecNormal (vDelta + 1, &vPos, &pl->vEnd, vDelta);
	VmVecAdd (&v, &vPos, vDelta + 1);
	VmVecNormal (vDelta, &vPos, &pl->vEnd, &v);
	}
VmVecScaleFrac (&vDir, pl->nLength, (pl->nNodes - 1) * F1_0);
pl->nNodes = abs (pl->nNodes);
pl->iStep = 0;
if (pp = pl->pParent) {
	i = pp->nChildren + 1;
	l = pp->nLength / i;
	pl->nLength = ComputeChildEnd (&pl->vPos, &pl->vEnd, &pl->vDir, &pp->vDir, l + 3 * l / (pl->nNode + 1)); 
	VmVecCopyScale (&vDir, &pl->vDir, pl->nLength / (pl->nNodes - 1));
	}
for (i = pl->nNodes, pln = pl->pNodes; i > 0; i--, pln++) {
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
pl->nSteps = -abs (pl->nSteps);
//(pln - 1)->vPos = pl->vEnd;
}

//------------------------------------------------------------------------------

tLightning *AllocLightning (int nLightnings, vmsVector *vPos, vmsVector *vEnd, vmsVector *vDelta,
									 short nObject, int nLife, int nDelay, int nLength, int nAmplitude, 
									 char nAngle, int nOffset, short nNodes, short nChildren, char nDepth, short nSteps, 
									 short nSmoothe, char bClamp, char bPlasma, char nStyle, tRgbaColorf *colorP, 
									 tLightning *pParent, short nNode)
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
		pl->nSegment = OBJECTS [nObject].nSegment;
		}
	pl->pParent = pParent;
	pl->nNode = nNode;
	pl->nNodes = nNodes;
	pl->nChildren = gameOpts->render.lightnings.nQuality ? (nChildren < 0) ? nNodes / 10 : nChildren : 0;
	pl->nLife = nLife;
	h = abs (nLife);
	pl->nTTL = (bRandom) ? 3 * h / 4 + (int) (f_rand () * h / 2) : h;
	pl->nDelay = abs (nDelay) * 10;
	pl->nLength = (bRandom) ? 3 * nLength / 4 + (int) (f_rand () * nLength / 2) : nLength;
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
	pl->nStyle = nStyle;
	pl->nNext = -1;
	if (vEnd)
		pl->vRefEnd = *vEnd;
	if (!(pl->bRandom = bRandom))
		pl->vEnd = *vEnd;
	if (pl->bInPlane = (vDelta != NULL))
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
#if 0	//children have completely random nodes
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
													2 * pl->nNodes / n, nChildren / 5, nDepth - 1, nSteps / 2, nSmoothe, bClamp, bPlasma, nStyle, colorP, pl, nNode);
			}
		}
	}
return pfRoot;
}

//------------------------------------------------------------------------------

void CreateLightningSound (tLightningBundle *plb, int bSound)
{
if (plb->bSound = bSound) {
	DigiSetObjectSound (plb->nObject, -1, "lightng.wav");
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
							short nSmoothe, char bClamp, char bPlasma, char bSound, char nStyle, tRgbaColorf *colorP)
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
	if (!(pl = AllocLightning (nLightnings, vPos, vEnd, vDelta, nObject, nLife, nDelay, nLength, nAmplitude, 
										nAngle, nOffset, nNodes, nChildren, nDepth, nSteps, nSmoothe, bClamp, bPlasma, nStyle, colorP, NULL, -1)))
		return -1;
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
			if ((int) (size_t) pln->pChild == 0xffffffff)
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
	for (i = gameData.lightnings.iUsed; i >= 0; i = j) {
		j = gameData.lightnings.buffer [i].nNext;
		DestroyLightnings (i, NULL, 1);
		}
	ResetLightningLights (1);
	InitLightnings ();
	}
return 1;
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
	vmsVector	va;
	int			nDot, i = 0;

if (nDist < F1_0 / 16)
	return VmRandomVector (vOffs);
VmVecCopyNormalize (&va, vAttract);
if (!(va.p.x && va.p.y && va.p.z))
	i = 0;
do {
	VmRandomVector (vOffs);
	nDot = VmVecDot (&va, vOffs);
	if (++i > 100)
		i = 0;
	} while (abs (nDot) < F1_0 / 32);
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

vmsVector ComputeJaggyNode (tLightningNode *pln, vmsVector *vPos, vmsVector *vDest, vmsVector *vBase, vmsVector *vPrevOffs,
										  int nSteps, int nAmplitude, int nMinDist, int i, int nSmoothe, int bClamp)
{
	vmsVector	vAttract, vOffs;
	int			nDist = ComputeAttractor (&vAttract, vDest, vPos, nMinDist, i);

TRAP (pln);
CreateLightningPathPoint (&vOffs, &vAttract, nDist);
if (vPrevOffs)
	SmootheLightningOffset (&vOffs, vPrevOffs, nDist, nSmoothe);
else if (pln->vOffs.p.x || pln->vOffs.p.z || pln->vOffs.p.z) {
	VmVecScaleInc (&vOffs, &pln->vOffs, 2 * F1_0);
	VmVecScaleFrac (&vOffs, 1, 3);
	}
if (nDist > F1_0 / 16)
	AttractLightningPathPoint (&vOffs, &vAttract, vPos, nDist, i, 0);
if (bClamp)
	ClampLightningPathPoint (vPos, vBase, nAmplitude);
pln->vNewPos = *vPos;
VmVecSub (&pln->vOffs, &pln->vNewPos, &pln->vPos);
VmVecScale (&pln->vOffs, F1_0 / nSteps);
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
	nDelta = nAmplitude / 2 - (int) (f_rand () * nAmplitude);
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
	VmVecScaleInc (&pln->vNewPos, pln->vDelta + j, nDelta);
	}
VmVecSub (&pln->vOffs, &pln->vNewPos, &pln->vPos);
VmVecScale (&pln->vOffs, F1_0 / nSteps);
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
VmVecScaleAdd (&pln->vNewPos, &pln->vBase, pln->vDelta, (int) dx);
VmVecScaleInc (&pln->vNewPos, pln->vDelta + 1, (int) dy);
VmVecSub (&pln->vOffs, &pln->vNewPos, &pln->vPos);
VmVecScale (&pln->vOffs, F1_0 / nSteps);
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
		plh->vNewPos.p.x = pfj->vNewPos.p.x / 4 + plh->vNewPos.p.x / 2 + pfi->vNewPos.p.x / 4;
		plh->vNewPos.p.y = pfj->vNewPos.p.y / 4 + plh->vNewPos.p.y / 2 + pfi->vNewPos.p.y / 4;
		plh->vNewPos.p.z = pfj->vNewPos.p.z / 4 + plh->vNewPos.p.z / 2 + pfi->vNewPos.p.z / 4;
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
		VmVecSub (&pln->vOffs, &pln->vNewPos, &pln->vPos);
		VmVecScale (&pln->vOffs, F1_0 / nSteps);
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
	nDist = VmVecDist (&pln->vNewPos, &pln->vPos);
	if (nMaxDist < nDist) {
		nMaxDist = nDist;
		h = i;
		}
	}
if (h = nAmplitude - nMaxDist) {
	if (pl->nNodes > 0) {
		nMaxDist += (d_rand () % 5) * h / 4;
		for (i = pl->nNodes - 1 - !pl->bRandom, pln = pl->pNodes + 1; i > 0; i--, pln++)
			VmVecScaleFrac (&pln->vOffs, nAmplitude, nMaxDist);
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
VmVecZero (&plh->vOffs);
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
	VmVecZero (&plh->vOffs);
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
		if (bInit = (pl->nSteps < 0))
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
				VmVecInc (&pln->vPos, &pln->vOffs);
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
				pl->nTTL = h + (int) (f_rand () * h);
				}
			else {
				if (pl->bRandom) {
					h = -pl->nLife;
					pl->nTTL = 3 * h / 4 + (int) (f_rand () * h / 2);
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
if (gameStates.app.bMultiThreaded && (nLightnings > 1)) {
	tiRender.pl = pl;
	tiRender.nLightnings = nLightnings;
	RunRenderThreads (rtAnimateLightnings);
	}
else
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

#define LIMIT_FLASH_FPS	1
#define FLASH_SLOWMO 1

void UpdateLightnings (void)
{
if (SHOW_LIGHTNINGS) {
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
		if (gameStates.app.nSDLTicks - plb->tUpdate < 25)
			continue;
		plb->tUpdate = gameStates.app.nSDLTicks; 
		if (plb->bDestroy)
			DestroyLightnings (i, NULL, 1);
		else {
			if (!(plb->nLightnings = UpdateLightning (plb->pl, plb->nLightnings, 0)))
				DestroyLightnings (i, NULL, 1);
			else if (!(plb->nKey [0] || plb->nKey [1]) && (plb->nObject >= 0)) {
				UpdateLightningSound (plb);
				MoveObjectLightnings (OBJECTS + plb->nObject);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void MoveLightnings (int i, tLightning *pl, vmsVector *vNewPos, short nSegment, int bStretch, int bFromEnd)
{
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
			VmVecSub (&vDelta, vNewPos, &pl->vEnd);
		else
			VmVecSub (&vDelta, vNewPos, &pl->vPos);
		if (vDelta.p.x || vDelta.p.y || vDelta.p.z) {
			if (bStretch) {
				vOffs = vDelta;
				if (bFromEnd)
					VmVecInc (&pl->vEnd, &vDelta);
				else
					VmVecInc (&pl->vPos, &vDelta);
				}
			else if (bFromEnd) {
				VmVecInc (&pl->vPos, &vDelta);
				pl->vEnd = *vNewPos;
				}
			else {
				VmVecInc (&pl->vEnd, &vDelta);
				pl->vPos = *vNewPos;
				}
			if (bStretch) {
				VmVecSub (&pl->vDir, &pl->vEnd, &pl->vPos);
				pl->nLength = VmVecMag (&pl->vDir);
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
						vDelta.p.x = (int) (vOffs.p.x * fStep + 0.5);
						vDelta.p.y = (int) (vOffs.p.y * fStep + 0.5);
						vDelta.p.z = (int) (vOffs.p.z * fStep + 0.5);
						}
					VmVecInc (&pln->vNewPos, &vDelta);
					VmVecInc (&pln->vBase, &vDelta);
					VmVecInc (&pln->vPos, &vDelta);
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

void MoveObjectLightnings (tObject *objP)
{
#if 0
if (!SPECTATOR (objP) &&
	 ((objP->vLastPos.p.x != objP->position.vPos.p.x) ||
	  (objP->vLastPos.p.y != objP->position.vPos.p.y) ||
	 (objP->vLastPos.p.z != objP->position.vPos.p.z)))
#endif
	MoveLightnings (gameData.lightnings.objects [OBJ_IDX (objP)], NULL, &OBJPOS (objP)->vPos, objP->nSegment, 0, 0);
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

void DestroyAllObjectLightnings (int nType, int nId)
{
	tObject	*objP;
	int		i;

for (i = 0, objP = OBJECTS; i <= gameData.objs.nLastObject; i++, objP++)
	if ((objP->nType == nType) && ((nId < 0) || (objP->id == nId)))
		DestroyObjectLightnings (objP);
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
				VmVecScalef (&vDelta, VmVecSubf (&vDelta, vPlasma, vPlasma + 1), 0.25f);
				VmVecDecf (vPlasma, &vDelta);
				VmVecIncf (vPlasma + 1, &vDelta);
				VmVecScalef (&vDelta, VmVecSubf (&vDelta, vPlasma + 2, vPlasma + 3), 0.25f);
				VmVecDecf (vPlasma + 2, &vDelta);
				VmVecIncf (vPlasma + 3, &vDelta);
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

	static fVector	vEye = {{0,0,0}};
	static fVector vNormal [3] = {{{0,0,0}},{{0,0,0}},{{0,0,0}}};

memcpy (vNormal, vNormal + 1, 2 * sizeof (fVector));
if (bStart) {
	VmVecNormalf (vNormal + 1, vPosf, vPosf + 1, &vEye);
	vn [0] = vNormal [1];
	}
else
	VmVecScalef (vn, VmVecAddf (vn, vNormal, vNormal + 1), 0.5f);
if (bEnd) {
	vn [1] = vNormal [1];
	VmVecSubf (&vd, vPosf + 1, vPosf);
	VmVecScalef (&vd, &vd, bScale ? 0.25f : 0.5f);
	VmVecIncf (vPosf + 1, &vd);
	}
else {
	VmVecNormalf (vNormal + 2, vPosf + 1, vPosf + 2, &vEye);
	if (VmVecDotf (vNormal + 1, vNormal + 2) < 0)
		VmVecNegatef (vNormal + 2);
	VmVecScalef (vn + 1, VmVecAddf (vn + 1, vNormal + 1, vNormal + 2), 0.5f);
	}
if (!(nDepth || bScale)) {
	VmVecScalef (vn, vn, 2);
	VmVecScalef (vn + 1, vn + 1, 2);
	}
if (!bScale && nDepth) {
	VmVecScalef (vn, vn, 0.5f);
	VmVecScalef (vn + 1, vn + 1, 0.5f);
	}
if (bStart) {
	VmVecAddf (vPlasma, vPosf, vn);
	VmVecSubf (vPlasma + 1, vPosf, vn);
	VmVecSubf (&vd, vPosf, vPosf + 1);
	VmVecNormalizef (&vd, &vd);
	if (bScale)
		VmVecScalef (&vd, &vd, 0.5f);
	VmVecIncf (vPlasma, &vd);
	VmVecIncf (vPlasma + 1, &vd);
	}
else {
	vPlasma [0] = vPlasma [-1];
	vPlasma [1] = vPlasma [-2];
	}
VmVecAddf (vPlasma + 3, vPosf + 1, vn + 1);
VmVecSubf (vPlasma + 2, vPosf + 1, vn + 1);
memcpy (plasmaBuffers [nThread][bScale].texCoord + 4 * nSegment, plasmaTexCoord [bStart + 2 * bEnd], 4 * sizeof (tTexCoord2f));
}

//------------------------------------------------------------------------------

void ComputePlasmaBuffer (tLightning *pl, int nDepth, int nThread)
{
	tLightningNode	*pln;
	fVector			vPosf [3] = {{{0,0,0}},{{0,0,0}},{{0,0,0}}};
	int				bScale, i, j;

for (bScale = 0; bScale < 2; bScale++) {
	pln = pl->pNodes;
	VmsVecToFloat (vPosf + 2, &(pln++)->vPos);
	if (!gameStates.ogl.bUseTransform)
		G3TransformPointf (vPosf + 2, vPosf + 2, 0);
	for (i = pl->nNodes - 2, j = 0; j <= i; j++) {
		TRAP (pln);
		memcpy (vPosf, vPosf + 1, 2 * sizeof (fVector));
		VmsVecToFloat (vPosf + 2, &(++pln)->vPos);
		if (!gameStates.ogl.bUseTransform)
			G3TransformPointf (vPosf + 2, vPosf + 2, 0);
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
	VmsVecToFloat ((fVector *) vPosf, &pln->vPos);
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
	VmsVecToFloat (vPosf, &pln->vNewPos);
	G3TransformPointf (vPosf, vPosf, 0);
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
	color.red *= (float) (0.9 + f_rand () / 5);
	color.green *= (float) (0.9 + f_rand () / 5);
	color.blue *= (float) (0.9 + f_rand () / 5);
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
	static fVector	vEye = {{0,0,0}};
	static fVector	vPlasma [6] = {{{0,0,0}},{{0,0,0}},{{0,0,0}},{{0,0,0}},{{0,0,0}},{{0,0,0}}};
	static fVector vNormal [3] = {{{0,0,0}},{{0,0,0}},{{0,0,0}}};

	fVector	vn [2], vd;
	int		i, j = bStart + 2 * bEnd;

memcpy (vNormal, vNormal + 1, 2 * sizeof (fVector));
if (bStart) {
	VmVecNormalf (vNormal + 1, vPosf, vPosf + 1, &vEye);
	vn [0] = vNormal [1];
	}
else
	VmVecScalef (vn, VmVecAddf (vn, vNormal, vNormal + 1), 0.5f);
if (bEnd)
	vn [1] = vNormal [1];
else {
	VmVecNormalf (vNormal + 2, vPosf + 1, vPosf + 2, &vEye);
	VmVecScalef (vn + 1, VmVecAddf (vn + 1, vNormal + 1, vNormal + 2), 0.5f);
	}
if (!(nDepth || bScale)) {
	VmVecScalef (vn, vn, 2);
	VmVecScalef (vn + 1, vn + 1, 2);
	}
if (!bScale && nDepth) {
	VmVecScalef (vn, vn, 0.5f);
	VmVecScalef (vn + 1, vn + 1, 0.5f);
	}
if (bStart) {
	VmVecAddf (vPlasma, vPosf, vn);
	VmVecSubf (vPlasma + 1, vPosf, vn);
	VmVecSubf (&vd, vPosf, vPosf + 1);
	VmVecNormalizef (&vd, &vd);
	if (bScale)
		VmVecScalef (&vd, &vd, 0.5f);
	VmVecIncf (vPlasma, &vd);
	VmVecIncf (vPlasma + 1, &vd);
	}
else {
	vPlasma [0] = vPlasma [3];
	vPlasma [1] = vPlasma [2];
	}
VmVecAddf (vPlasma + 3, vPosf + 1, vn + 1);
VmVecSubf (vPlasma + 2, vPosf + 1, vn + 1);
if (bEnd) {
	VmVecSubf (&vd, vPosf + 1, vPosf);
	VmVecNormalizef (&vd, &vd);
	if (bScale)
		VmVecScalef (&vd, &vd, 0.5f);
	VmVecIncf (vPlasma + 2, &vd);
	VmVecIncf (vPlasma + 3, &vd);
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
		VmVecNormalf (vn, vPlasma, vPlasma + 1, vPlasma + 2);
		VmVecNormalf (vn + 1, vPlasma, vPlasma + 2, vPlasma + 3);
		fDot = VmVecDotf (vn, vn + 1);
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
return (pl->nSegment < 0) || SegmentMayBeVisible (pl->nSegment, pl->nLength / 20, 3 * pl->nLength / 2);
}

//------------------------------------------------------------------------------

void RenderLightning (tLightning *pl, int nLightnings, short nDepth, int bDepthSort)
{
	tLightningNode	*pln;
	fVector		vPosf [3] = {{{0,0,0}},{{0,0,0}}};
	int			i;
#if !RENDER_LIGHTINGS_BUFFERED
	int			h, j;
#endif
	int			bPlasma =  gameOpts->render.lightnings.bPlasma && pl->bPlasma;
	tRgbaColorf	color;
	tObject		*objP = NULL;

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
	color.red *= (float) (0.9 + f_rand () / 5);
	color.green *= (float) (0.9 + f_rand () / 5);
	color.blue *= (float) (0.9 + f_rand () / 5);
	for (; nLightnings; nLightnings--, pl++) {
		if ((pl->nNodes < 0) || (pl->nSteps < 0))
			continue;
#if RENDER_LIGHTING_SEGMENTS
		for (i = pl->nNodes - 1, j = 0, pln = pl->pNodes; j <= i; j++) {
			if (j < i)
				memcpy (vPosf, vPosf + 1, 2 * sizeof (fVector));
			if (!j) {
				VmsVecToFloat (vPosf + 1, &(pln++)->vPos);
				G3TransformPointf (vPosf + 1, vPosf + 1, 0);
				}
			if (j < i) {
				VmsVecToFloat (vPosf + 2, &(++pln)->vPos);
				G3TransformPointf (vPosf + 2, vPosf + 2, 0);
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
		color.red *= (float) (0.9 + f_rand () / 5);
		color.green *= (float) (0.9 + f_rand () / 5);
		color.blue *= (float) (0.9 + f_rand () / 5);
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
						VmsVecToFloat (vPosf + 1, &(pln++)->vPos);
						if (!gameStates.ogl.bUseTransform)
							G3TransformPointf (vPosf + 1, vPosf + 1, 0);
						}
					if (j < i) {
						VmsVecToFloat (vPosf + 2, &(++pln)->vPos);
						if (!gameStates.ogl.bUseTransform)
							G3TransformPointf (vPosf + 2, vPosf + 2, 0);
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
			VmsVecToFloat (vPosf, &pln->vPos);
			if (!gameStates.ogl.bUseTransform)
				G3TransformPointf (vPosf, vPosf, 0);
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
			VmsVecToFloat (vPosf, &pln->vNewPos);
			G3TransformPointf (vPosf, vPosf, 0);
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
		int			i, n, bStencil = StencilOff ();

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
for (i = 0, objP = gameData.objs.objects; i <= gameData.objs.nLastObject; i++, objP++)
	if ((objP != emitterP) && (objP->nType == OBJ_EFFECT) && (objP->id == LIGHTNING_ID) && (objP->rType.lightningInfo.nId == nTarget))
		return &objP->position.vPos;
return NULL;
}

//------------------------------------------------------------------------------

void StaticLightningFrame (void)
{
	int				i;
	tObject			*objP;
	vmsVector		*vEnd, *vDelta, v;
	tLightningInfo	*pli;
	tRgbaColorf		color;

if (!SHOW_LIGHTNINGS)
	return;
if (!gameOpts->render.lightnings.bStatic)
	return;
for (i = 0, objP = gameData.objs.objects; i <= gameData.objs.nLastObject; i++, objP++) {
	if ((objP->nType != OBJ_EFFECT) || (objP->id != LIGHTNING_ID))
		continue;
	if (gameData.lightnings.objects [i] >= 0)
		continue;
	pli = &objP->rType.lightningInfo;
	if (pli->nLightnings <= 0)
		continue;
	if (pli->bRandom && !pli->nAngle)
		vEnd = NULL;
	else if (vEnd = FindLightningTargetPos (objP, pli->nTarget))
		pli->nLength = VmVecDist (&objP->position.vPos, vEnd) / F1_0;
	else {
		VmVecScaleAdd (&v, &objP->position.vPos, &objP->position.mOrient.fVec, F1_0 * pli->nLength);
		vEnd = &v;
		}
	color.red = (float) pli->color.red / 255.0f;
	color.green = (float) pli->color.green / 255.0f;
	color.blue = (float) pli->color.blue / 255.0f;
	color.alpha = (float) pli->color.alpha / 255.0f;
	vDelta = pli->bInPlane ? &objP->position.mOrient.rVec : NULL;
	gameData.lightnings.objects [i] =
		CreateLightning (pli->nLightnings, &objP->position.vPos, vEnd, vDelta, i, -abs (pli->nLife), pli->nDelay, pli->nLength * F1_0,
							  pli->nAmplitude * F1_0, pli->nAngle, pli->nOffset * F1_0, pli->nNodes, pli->nChildren, pli->nChildren > 0, pli->nSteps,
							  pli->nSmoothe, pli->bClamp, pli->bPlasma, pli->bSound, pli->nStyle, &color);
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

void SetLightningSegLight (short nSegment, vmsVector *vPos, tRgbaColorf *colorP)
{
	tLightningLight	*pll = gameData.lightnings.lights + nSegment;

if (pll->nFrameFlipFlop != gameStates.render.nFrameFlipFlop + 1) {
	memset (pll, 0, sizeof (*pll));
	pll->nFrameFlipFlop = gameStates.render.nFrameFlipFlop + 1;
	pll->nNext = gameData.lightnings.nFirstLight;
	gameData.lightnings.nFirstLight = nSegment;
	}
pll->nLights++;
VmVecInc (&pll->vPos, vPos);
pll->color.red += colorP->red;
pll->color.green += colorP->green;
pll->color.blue += colorP->blue;
pll->color.alpha += colorP->alpha;
pll->nSegment = nSegment;
}

//------------------------------------------------------------------------------

int SetLightningLight (tLightning *pl, int i)
{
	tLightningNode	*pln;
	vmsVector		*vPos;
	short				nSegment;
	int				j, nLights;
	double			h, nStep;

SetLightningSegLight (pl->nSegment, &pl->vPos, &pl->color);
for (nLights = 1; i > 0; i--, pl++) {
	if (0 < (j = pl->nNodes)) {
		if (!(nStep = (double) (j - 1) / (double) ((int) ((double) pl->nLength / F1_0 / 20 + 0.5))))
			nStep = (double) ((int) (j + 0.5));
		nSegment = pl->nSegment;
		pln = pl->pNodes;
		for (h = nStep; h < j; h += nStep) {
			TRAP (pln);
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

void ResetLightningLights (int bForce)
{
if (SHOW_LIGHTNINGS || bForce) {
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
		tLightningLight	*pll;
		int					i, n, nLights = 0, bDynLighting = gameOpts->render.bDynLighting;

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
			pll->nBrightness = fl2f ((pll->color.red * 3 + pll->color.green * 5 + pll->color.blue * 2) * pll->color.alpha);
			if (bDynLighting)
				pll->nDynLight = AddDynLight (&pll->color, pll->nBrightness, pll->nSegment, -1, -1, &pll->vPos);
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
			nRad, F1_0 * 4, 0, 2 * F1_0, 50, 5, 1, 3, 1, 1, 0, 0, -1, colorP);
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

void CreateBlowupLightnings (tObject *objP)
{
static tRgbaColorf color = {0.1f, 0.1f, 0.8f, 0.2f};

int h = f2i (objP->size) * 2;

CreateExplosionLightnings (objP, &color, h + rand () % h, h * (F1_0 + F1_0 / 2), 500);
}

//------------------------------------------------------------------------------

void CreateRobotLightnings (tObject *objP, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNINGS && gameOpts->render.lightnings.bRobots && OBJECT_EXISTS (objP)) {
		int i = OBJ_IDX (objP);

	if (0 <= gameData.lightnings.objects [i]) 
		MoveObjectLightnings (objP);
	else
		gameData.lightnings.objects [i] = CreateLightning (
			2 * objP->size / F1_0, &objP->position.vPos, NULL, NULL, OBJ_IDX (objP), -1000, 100, 
			objP->size, objP->size / 8, 0, 0, 25, 3, 1, 3, 1, 1, 0, 0, 0, colorP);
	}
}

//------------------------------------------------------------------------------

void CreatePlayerLightnings (tObject *objP, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNINGS && gameOpts->render.lightnings.bPlayers && OBJECT_EXISTS (objP)) {
		int i = OBJ_IDX (objP);

	if (0 <= gameData.lightnings.objects [i]) 
		MoveObjectLightnings (objP);
	else
		gameData.lightnings.objects [i] = CreateLightning (
			4 * objP->size / F1_0, &objP->position.vPos, NULL, NULL, OBJ_IDX (objP), -5000, 1000, 
			4 * objP->size, objP->size, 0, 2 * objP->size, 50, 5, 1, 5, 1, 1, 0, 1, 1, colorP);
	}
}

//------------------------------------------------------------------------------

void CreateDamageLightnings (tObject *objP, tRgbaColorf *colorP)
{
if (SHOW_LIGHTNINGS && gameOpts->render.lightnings.bDamage && OBJECT_EXISTS (objP)) {
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
		n, &objP->position.vPos, NULL, NULL, OBJ_IDX (objP), -1000, 4000, 
		objP->size, objP->size / 8, 0, 0, 20, 0, 1, 10, 1, 1, 0, 0, -1, colorP);
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
	if (f_rand () > fDamage)
		return;
#endif
	if (pointList) {
		vPos = pointList [0]->p3_src;
		vEnd = pointList [1 + d_rand () % (nVertices - 1)]->p3_vec;
		VmVecNormal (&vNorm, &vPos, &pointList [1]->p3_vec, &vEnd);
		VmVecScaleInc (&vPos, &vNorm, F1_0 / 64);
		VmVecScaleInc (&vEnd, &vNorm, F1_0 / 64);
		VmVecNormal (&vDelta, &vNorm, &vPos, &vEnd);
		h = VmVecDist (&vPos, &vEnd);
		}
	else {
		memcpy (&vPosf, &pVerts->vertex, sizeof (fVector3));
		memcpy (&vEndf, &pVerts [1 + d_rand () % (nVertices - 1)].vertex, sizeof (fVector3));
		memcpy (&v, &pVerts [1].vertex, sizeof (fVector3));
		VmVecNormalf (&vNormf, &vPosf, &v, &vEndf);
		VmVecScaleIncf3 (&vPosf, &vNormf, 1.0f / 64.0f);
		VmVecScaleIncf3 (&vEndf, &vNormf, 1.0f / 64.0f);
		VmVecNormalf (&vDeltaf, &vNormf, &vPosf, &vEndf);
		h = fl2f (VmVecDistf (&vPosf, &vEndf));
		vPos.p.x = fl2f (vPosf.p.x);
		vPos.p.y = fl2f (vPosf.p.y);
		vPos.p.z = fl2f (vPosf.p.z);
		vEnd.p.x = fl2f (vEndf.p.x);
		vEnd.p.y = fl2f (vEndf.p.y);
		vEnd.p.z = fl2f (vEndf.p.z);
		}
	i = CreateLightning (1, &vPos, &vEnd, NULL /*&vDelta*/, nObject, 1000 + d_rand () % 2000, 0, 
								h, h / 4 + d_rand () % 2, 0, 0, 20, 2, 1, 5, 0, 1, 0, 0, 1, &color);
	bUpdate = 1;
	}
if (i >= 0) {
	plb = gameData.lightnings.buffer + i;
	glDisable (GL_CULL_FACE);
	if (bUpdate) {
		plb->nKey [0] = key.i [0];
		plb->nKey [1] = key.i [1];
		}
	plb->nLightnings = UpdateLightning (plb->pl, plb->nLightnings, 0);
	RenderLightning (plb->pl, plb->nLightnings, 0, -1);
	}
}

//------------------------------------------------------------------------------
//eof
