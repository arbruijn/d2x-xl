/* $Id: interp.c, v 1.14 2003/03/19 19:21:34 btb Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: interp.c, v 1.14 2003/03/19 19:21:34 btb Exp $";
#endif

#include <math.h>
#include <stdlib.h>
#include "error.h"

#include "inferno.h"
#include "interp.h"
#include "shadows.h"
#include "hitbox.h"
#include "globvars.h"
#include "gr.h"
#include "byteswap.h"
#include "u_mem.h"
#include "console.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_render.h"
#include "network.h"
#include "render.h"
#include "gameseg.h"
#include "light.h"
#include "dynlight.h"
#include "lightning.h"

extern tFaceColor tMapColor;

#if DBG_SHADOWS
extern int bShadowTest;
extern int bFrontCap;
extern int bRearCap;
extern int bShadowVolume;
extern int bFrontFaces;
extern int bBackFaces;
extern int bSWCulling;
#endif
#if SHADOWS
extern int bZPass;
#endif

#define G3_DRAW_ARRAYS	1
#define G3_USE_VBOS		0

//------------------------------------------------------------------------------

//shadow clipping
//1: Compute hit point of vector from current light through each model vertex (fast)
//2: Compute hit point of vector from current light through each lit submodel vertex (slow)
//3: Compute hit point of vector from current light through each lit model face (fastest, flawed)

int bPrintLine = 0;

short nHighestTexture;
int g3d_interp_outline;

g3sPoint	*modelPointList = NULL;

#define MAX_INTERP_COLORS 100

//this is a table of mappings from RGB15 to palette colors
struct {short pal_entry, rgb15;} interpColorTable [MAX_INTERP_COLORS];

//static int bIntrinsicFacing = 0;
//static int bFlatPolys = 1;
//static int bTexPolys = 1;

vmsAngVec avZero = {0, 0, 0};
vmsVector vZero = ZERO_VECTOR;
vmsMatrix mIdentity = IDENTITY_MATRIX;

g3sPoint *pointList [MAX_POINTS_PER_POLY];

short nGlow = -1;

//------------------------------------------------------------------------------
//gives the interpreter an array of points to use
void G3SetModelPoints (g3sPoint *pointlist)
{
modelPointList = pointlist;
}

//------------------------------------------------------------------------------

#if PROFILING
time_t tTransform = 0;
#endif

void RotatePointList (g3sPoint *dest, vmsVector *src, g3sNormal *norms, int n, int o)
{
	fVector	*pfv = gameData.models.fPolyModelVerts + o;
	float		fScale;
#if PROFILING
	time_t	t = clock ();
#endif

dest += o;
if (norms)
	norms += o;
while (n--) {
	dest->p3_key = (short) o;
#if 1
	if (norms) {
		if (norms->nFaces > 1) {
			norms->vNormal.p.x /= norms->nFaces;
			norms->vNormal.p.y /= norms->nFaces;
			norms->vNormal.p.z /= norms->nFaces;
			norms->nFaces = 1;
			VmVecNormalize (&norms->vNormal, &norms->vNormal);
			}
		dest->p3_normal = *norms++;
		}
	else
#endif
		dest->p3_normal.nFaces = 0;
	fScale = (gameData.models.nScale ? f2fl (gameData.models.nScale) : 1) / 65536.0f;
	if (gameStates.ogl.bUseTransform) {
		pfv->p.x = src->p.x * fScale;
		pfv->p.y = src->p.y * fScale;
		pfv->p.z = src->p.z * fScale;
		}
	else {
		if (gameData.models.nScale) {
			vmsVector v = *src;
			VmVecScale (&v, gameData.models.nScale);
#if 1
			G3TransformPoint (&dest->p3_vec, &v, 0);
#else
			G3TransformAndEncodePoint (dest, &v);
#endif
			}
		else {
#if 1
			G3TransformPoint (&dest->p3_vec, src, 0);
#else
			G3TransformAndEncodePoint (dest, src);
#endif
			}
		pfv->p.x = (float) dest->p3_vec.p.x / 65536.0f;
		pfv->p.y = (float) dest->p3_vec.p.y / 65536.0f;
		pfv->p.z = (float) dest->p3_vec.p.z / 65536.0f;
		}
	dest->p3_index = o++;
	dest->p3_src = *src++;
	dest++;
	pfv++;
	}
#if PROFILING
tTransform += clock () - t;
#endif
}

//------------------------------------------------------------------------------

#if 1 //defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)

inline void ShortSwap (short *s)
{
*s = SWAPSHORT (*s);
}

//------------------------------------------------------------------------------

inline void FixSwap (fix *f)
{
*f = (fix)SWAPINT ((int)*f);
}

//------------------------------------------------------------------------------

inline void VmsVectorSwap (vmsVector *v)
{
FixSwap (FIXPTR (&v->p.x));
FixSwap (FIXPTR (&v->p.y));
FixSwap (FIXPTR (&v->p.z));
}

//------------------------------------------------------------------------------

inline void FixAngSwap (fixang *f)
{
*f = (fixang) SWAPSHORT ((short)*f);
}

//------------------------------------------------------------------------------

inline void VmsAngVecSwap (vmsAngVec *v)
{
FixAngSwap (&v->p);
FixAngSwap (&v->b);
FixAngSwap (&v->h);
}

//------------------------------------------------------------------------------

void G3SwapPolyModelData (ubyte *data)
{
	int i;
	short n;
	tUVL *uvl_val;
	ubyte *p = data;

for (;;) {
	ShortSwap (WORDPTR (p));
	switch (WORDVAL (p)) {
		case OP_EOF:
			return;

		case OP_DEFPOINTS:
			ShortSwap (WORDPTR (p + 2));
			n = WORDVAL (p+2);
			for (i = 0; i < n; i++)
				VmsVectorSwap (VECPTR ((p + 4) + (i * sizeof (vmsVector))));
			p += n*sizeof (vmsVector) + 4;
			break;

		case OP_DEFP_START:
			ShortSwap (WORDPTR (p + 2));
			ShortSwap (WORDPTR (p + 4));
			n = WORDVAL (p+2);
			for (i = 0; i < n; i++)
				VmsVectorSwap (VECPTR ((p + 8) + (i * sizeof (vmsVector))));
			p += n*sizeof (vmsVector) + 8;
			break;

		case OP_FLATPOLY:
			ShortSwap (WORDPTR (p+2));
			n = WORDVAL (p+2);
			VmsVectorSwap (VECPTR (p + 4));
			VmsVectorSwap (VECPTR (p + 16));
			ShortSwap (WORDPTR (p+28));
			for (i=0; i < n; i++)
				ShortSwap (WORDPTR (p + 30 + (i * 2)));
			p += 30 + ((n&~1)+1)*2;
			break;

		case OP_TMAPPOLY:
			ShortSwap (WORDPTR (p+2));
			n = WORDVAL (p+2);
			VmsVectorSwap (VECPTR (p + 4));
			VmsVectorSwap (VECPTR (p + 16));
			for (i = 0; i < n; i++) {
				uvl_val = (tUVL *) ((p+30+ ((n&~1)+1)*2) + (i * sizeof (tUVL)));
				FixSwap (&uvl_val->u);
				FixSwap (&uvl_val->v);
			}
			ShortSwap (WORDPTR (p+28));
			for (i=0;i<n;i++)
				ShortSwap (WORDPTR (p + 30 + (i * 2)));
			p += 30 + ((n&~1)+1)*2 + n*12;
			break;

		case OP_SORTNORM:
			VmsVectorSwap (VECPTR (p + 4));
			VmsVectorSwap (VECPTR (p + 16));
			ShortSwap (WORDPTR (p + 28));
			ShortSwap (WORDPTR (p + 30));
			G3SwapPolyModelData (p + WORDVAL (p+28));
			G3SwapPolyModelData (p + WORDVAL (p+30));
			p += 32;
			break;

		case OP_RODBM:
			VmsVectorSwap (VECPTR (p + 20));
			VmsVectorSwap (VECPTR (p + 4));
			ShortSwap (WORDPTR (p+2));
			FixSwap (FIXPTR (p + 16));
			FixSwap (FIXPTR (p + 32));
			p+=36;
			break;

		case OP_SUBCALL:
			ShortSwap (WORDPTR (p+2));
			VmsVectorSwap (VECPTR (p+4));
			ShortSwap (WORDPTR (p+16));
			G3SwapPolyModelData (p + WORDVAL (p+16));
			p += 20;
			break;

		case OP_GLOW:
			ShortSwap (WORDPTR (p + 2));
			p += 4;
			break;

		default:
			Error ("invalid polygon model\n"); //Int3 ();
		}
	}
}

#endif

//------------------------------------------------------------------------------

#ifdef WORDS_NEED_ALIGNMENT
void G3AddPolyModelChunk (ubyte *old_base, ubyte *new_base, int offset, 
	       chunk *chunk_list, int *no_chunks)
{
	Assert (*no_chunks + 1 < MAX_CHUNKS); //increase MAX_CHUNKS if you get this
	chunk_list [*no_chunks].old_base = old_base;
	chunk_list [*no_chunks].new_base = new_base;
	chunk_list [*no_chunks].offset = offset;
	chunk_list [*no_chunks].correction = 0;
	 (*no_chunks)++;
}

//------------------------------------------------------------------------------
/*
 * finds what chunks the data points to, adds them to the chunk_list, 
 * and returns the length of the current chunk
 */
int G3GetPolyModelChunks (ubyte *data, ubyte *new_data, chunk *list, int *no)
{
	short n;
	ubyte *p = data;

for (;;) {
	switch (INTEL_SHORT (WORDVAL (p))) {
		case OP_EOF:
			return p + 2 - data;
		case OP_DEFPOINTS:
			n = INTEL_SHORT (WORDVAL (p+2));
			p += n*sizeof (vmsVector) + 4;
			break;
		case OP_DEFP_START:
			n = INTEL_SHORT (WORDVAL (p+2));
			p += n*sizeof (vmsVector) + 8;
			break;
		case OP_FLATPOLY:
			n = INTEL_SHORT (WORDVAL (p+2));
			p += 30 + ((n&~1)+1)*2;
			break;
		case OP_TMAPPOLY:
			n = INTEL_SHORT (WORDVAL (p+2));
			p += 30 + ((n&~1)+1)*2 + n*12;
			break;
		case OP_SORTNORM:
			G3AddPolyModelChunk (p, p - data + new_data, 28, list, no);
			G3AddPolyModelChunk (p, p - data + new_data, 30, list, no);
			p += 32;
			break;
		case OP_RODBM:
			p+=36;
			break;
		case OP_SUBCALL:
			G3AddPolyModelChunk (p, p - data + new_data, 16, list, no);
			p+=20;
			break;
		case OP_GLOW:
			p += 4;
			break;
		default:
			Error ("invalid polygon model\n");
		}
	}
return p + 2 - data;
}
#endif //def WORDS_NEED_ALIGNMENT

//------------------------------------------------------------------------------

void G3PolyModelVerify (ubyte *data)
{
	short n;
	ubyte *p = data;

for (;;) {
	switch (WORDVAL (p)) {
		case OP_EOF:
			return;
		case OP_DEFPOINTS:
			n = (WORDVAL (p+2));
			p += n*sizeof (vmsVector) + 4;
			break;
		case OP_DEFP_START:
			n = (WORDVAL (p+2));
			p += n*sizeof (vmsVector) + 8;
			break;
		case OP_FLATPOLY:
			n = (WORDVAL (p+2));
			p += 30 + ((n&~1)+1)*2;
			break;
		case OP_TMAPPOLY:
			n = (WORDVAL (p+2));
			p += 30 + ((n&~1)+1)*2 + n*12;
			break;
		case OP_SORTNORM:
			G3PolyModelVerify (p + WORDVAL (p + 28));
			G3PolyModelVerify (p + WORDVAL (p + 30));
			p += 32;
			break;
		case OP_RODBM:
			p+=36;
			break;
		case OP_SUBCALL:
			G3PolyModelVerify (p + WORDVAL (p + 16));
			p+=20;
			break;
		case OP_GLOW:
			p += 4;
			break;
		default:
			Error ("invalid polygon model\n");
		}
	}
}

//------------------------------------------------------------------------------

int G3CheckAndSwap (void *modelP)
{
	short	h = WORDVAL (modelP);

if ((h >= 0) && (h <= OP_GLOW))
	return 1;
ShortSwap (&h);
if ((h < 0) || (h > OP_GLOW))
	return 0;
G3SwapPolyModelData ((ubyte *) modelP);
return 1;
}

//------------------------------------------------------------------------------

void GetThrusterPos (int nModel, vmsVector *vNormal, vmsVector *vOffset, grsBitmap *bmP, int nPoints)
{
	int					h, i, nSize;
	vmsVector			v, vForward = {{0,0,F1_0}};
	tModelThrusters	*mtP = gameData.models.thrusters + nModel;

if (mtP->nCount >= 2)
	return;
if (bmP) {
	i = (int) (bmP - gameData.pig.tex.bitmaps [0]);
	if ((i != 24) && ((i < 1741) || (i > 1745)))
		return;
	}
#if 1
if (VmVecDot (vNormal, &vForward) > -F1_0 / 3)
#else
if (vNormal->p.x || vNormal->p.y || (vNormal->p.z != -F1_0))
#endif
	return;
for (i = 1, v = pointList [0]->p3_src; i < nPoints; i++)
	VmVecInc (&v, &pointList [i]->p3_src);
v.p.x /= nPoints;
v.p.y /= nPoints;
v.p.z /= nPoints;
v.p.z -= F1_0 / 8;
if (vOffset)
	VmVecInc (&v, vOffset);
if (mtP->nCount && (v.p.x == mtP->vPos [0].p.x) && (v.p.y == mtP->vPos [0].p.y) && (v.p.z == mtP->vPos [0].p.z))
	return;
mtP->vPos [mtP->nCount] = v;
if (vOffset)
	VmVecDec (&v, vOffset);
mtP->vDir [mtP->nCount] = *vNormal;
VmVecNegate (mtP->vDir + mtP->nCount);
if (!mtP->nCount++) {
	for (i = 0, nSize = 0x7fffffff; i < nPoints; i++)
		if (nSize > (h = VmVecDist (&v, &pointList [i]->p3_src)))
			nSize = h;
	mtP->fSize = f2fl (nSize);// * 1.25f;
	}
}

//------------------------------------------------------------------------------

#define CHECK_NORMAL_FACING	0

int G3DrawPolyModel (
	tObject		*objP, 
	void			*modelP, 
	grsBitmap	**modelBitmaps, 
	vmsAngVec	*pAnimAngles, 
	vmsVector	*vOffset,
	fix			xModelLight, 
	fix			*xGlowValues, 
	tRgbaColorf	*colorP,
	tPOFObject  *po,
	int			nModel)
{
	ubyte *p = (ubyte *) modelP;
	short	nTag;
	short bGetThrusterPos = !objP || ((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_ROBOT) || ((objP->nType == OBJ_WEAPON) && gameData.objs.bIsMissile [objP->id]));
	short bLightnings = SHOW_LIGHTNINGS && gameOpts->render.lightnings.bDamage && objP && (ObjectDamage (objP) < 0.5f);

	static int nDepth = -1;

#if DBG_SHADOWS
if (bShadowTest)
	return 1;
#endif
#if CONTOUR_OUTLINE
//if (extraGameInfo [0].bShadows && (gameStates.render.nShadowPass < 3))
	return 1;
#endif
nDepth++;
G3CheckAndSwap (modelP);
if (SHOW_DYN_LIGHT && 
	!nDepth && !po && objP && ((objP->nType == OBJ_ROBOT) || (objP->nType == OBJ_PLAYER))) {
	po = gameData.models.pofData [gameStates.app.bD1Mission][0] + nModel;
	POFGatherPolyModelItems (objP, modelP, pAnimAngles, po, 0);
	}
nGlow = -1;		//glow off by default
glEnable (GL_CULL_FACE);
OglCullFace (0);
for (;;) {
	nTag = WORDVAL (p);
	if (nTag == OP_EOF)
		break;
	else if (nTag == OP_DEFPOINTS) {
		int n = WORDVAL (p+2);
		RotatePointList (modelPointList, VECPTR (p+4), po ? po->pVertNorms : NULL, n, 0);
		p += n * sizeof (vmsVector) + 4;
		break;
		}
	else if (nTag == OP_DEFP_START) {
		int n = WORDVAL (p+2);
		int s = WORDVAL (p+4);

		RotatePointList (modelPointList, VECPTR (p+8), po ? po->pVertNorms : NULL, n, s);
		p += n * sizeof (vmsVector) + 8;
		}
	else if (nTag == OP_FLATPOLY) {
		int nVerts = WORDVAL (p+2);
		Assert (nVerts < MAX_POINTS_PER_POLY);
#if CHECK_NORMAL_FACING
		if (G3CheckNormalFacing (VECPTR (p+4), VECPTR (p+16)) > 0) 
#endif
			{
			int i;
			//fix l = f2i (32 * xModelLight);
			GrSetColorRGB15bpp (WORDVAL (p+28), (ubyte) (255 * GrAlpha ()));
			GrFadeColorRGB (1.0);
			if (colorP) {
				colorP->red = (float) grdCurCanv->cvColor.color.red / 255.0f; 
				colorP->green = (float) grdCurCanv->cvColor.color.green / 255.0f;
				colorP->blue = (float) grdCurCanv->cvColor.color.blue / 255.0f;
				}
			p += 30;
			for (i = 0; i < nVerts; i++)
				pointList [i] = modelPointList + WORDPTR (p) [i];
			G3DrawPoly (nVerts, pointList);
			}
#if CHECK_NORMAL_FACING
		else
			p += 30;
#endif
		p += (nVerts | 1) * 2;
		}
	else if (nTag == OP_TMAPPOLY) {
		int nVerts = WORDVAL (p+2);
		Assert ( nVerts < MAX_POINTS_PER_POLY );
#if CHECK_NORMAL_FACING
		if (G3CheckNormalFacing (VECPTR (p+4), VECPTR (p+16)) > 0) 
#endif
			{
			tUVL *uvlList;
			int i;
			fix l;
			vmsVector *pvn = VECPTR (p+16);

			//calculate light from surface normal
			if (nGlow < 0) {			//no glow
				l = -VmVecDot (&viewInfo.view [0].fVec, VECPTR (p+16));
				l = f1_0 / 4 + (l * 3) / 4;
				l = FixMul (l, xModelLight);
				}
			else {				//yes glow
				l = xGlowValues [nGlow];
				nGlow = -1;
				}
			//now poke light into l values
			uvlList = (tUVL *) (p + 30 + (nVerts | 1) * 2);
			for (i = 0; i < nVerts; i++)
				uvlList [i].l = l;

			if (colorP) {
				unsigned char c = modelBitmaps [WORDVAL (p+28)]->bmAvgColor;
				colorP->red = CPAL2Tr (gamePalette, c);
				colorP->green = CPAL2Tg (gamePalette, c);
				colorP->blue = CPAL2Tb (gamePalette, c);
				}
			p += 30;
			for (i = 0; i < nVerts; i++)
				pointList [i] = modelPointList + WORDPTR (p) [i];
			tMapColor = gameData.objs.color;
			if (gameStates.render.bCloaked)
				G3DrawTexPolyFlat (nVerts, pointList, uvlList, NULL, modelBitmaps [WORDVAL (p-2)], NULL, NULL, VECPTR (p+16), 0, 1, -1);
			else
				G3DrawTexPolySimple (nVerts, pointList, uvlList, modelBitmaps [WORDVAL (p-2)], VECPTR (p+16), 1);
			if (!gameStates.render.bBriefing) {
				if (bLightnings)
					RenderDamageLightnings (objP, pointList, NULL, nVerts);
				if (bGetThrusterPos)
					GetThrusterPos (nModel, pvn, vOffset, modelBitmaps [WORDVAL (p-2)], nVerts);
				}
			}
#if CHECK_NORMAL_FACING
		else
			p += 30;
#endif
		p += (nVerts | 1) * 2 + nVerts * 12;
		}
	else if (nTag == OP_SORTNORM) {
#if CHECK_NORMAL_FACING
		if (G3CheckNormalFacing (VECPTR (p+16), VECPTR (p+4)) > 0) 
#endif
			{		//facing
			//draw back then front
			if (!(G3DrawPolyModel (objP, p + WORDVAL (p+30), modelBitmaps, pAnimAngles, vOffset, xModelLight, xGlowValues, colorP, po, nModel) &&
					G3DrawPolyModel (objP, p + WORDVAL (p+28), modelBitmaps, pAnimAngles, vOffset, xModelLight, xGlowValues, colorP, po, nModel)))
				return 0;
			}
#if CHECK_NORMAL_FACING
		else {			//not facing.  draw front then back
			if (!(G3DrawPolyModel (objP, p + WORDVAL (p+28), modelBitmaps, pAnimAngles, vOffset, xModelLight, xGlowValues, colorP, po, nModel) &&
					G3DrawPolyModel (objP, p + WORDVAL (p+30), modelBitmaps, pAnimAngles, vOffset, xModelLight, xGlowValues, colorP, po, nModel)))
				return 0;
			}
#endif
		p += 32;
		}
	else if (nTag == OP_RODBM) {
		g3sPoint rodBotP, rodTopP;
		G3TransformAndEncodePoint (&rodBotP, VECPTR (p+20));
		G3TransformAndEncodePoint (&rodTopP, VECPTR (p+4));
		G3DrawRodTexPoly (modelBitmaps [WORDVAL (p+2)], &rodBotP, WORDVAL (p+16), &rodTopP, WORDVAL (p+32), f1_0, NULL);
		p += 36;
		}
	else if (nTag == OP_SUBCALL) {
		vmsAngVec	*va;
		vmsVector	vo;

		va = pAnimAngles ? pAnimAngles + WORDVAL (p+2) : NULL;
		vo = *VECPTR (p+4);
		if (gameData.models.nScale)
			VmVecScale (&vo, gameData.models.nScale);
		G3StartInstanceAngles (&vo, va);
		if (vOffset)
			VmVecInc (&vo, vOffset);
		if (!G3DrawPolyModel (objP, p + WORDVAL (p+16), modelBitmaps, pAnimAngles, &vo, xModelLight, xGlowValues, colorP, po, nModel)) {
			G3DoneInstance ();
			return 0;
			}
		G3DoneInstance ();
		p += 20;
		}
	else if (nTag == OP_GLOW) {
		if (xGlowValues)
			nGlow = WORDVAL (p+2);
		p += 4;
		}
	else {
#ifdef _DEBUG
		PrintLog ("invalid polygon model\n");
#endif
		return 0;
		}
	}
nDepth--;
return 1;
}

#ifdef _DEBUG
int nestCount;
#endif

//------------------------------------------------------------------------------
//alternate interpreter for morphing tObject
int G3DrawMorphingModel (void *modelP, grsBitmap **modelBitmaps, vmsAngVec *pAnimAngles, vmsVector *vOffset,
								  fix xModelLight, vmsVector *new_points, int nModel)
{
	ubyte *p = (ubyte *) modelP;
	fix *xGlowValues = NULL;

G3CheckAndSwap (modelP);
nGlow = -1;		//glow off by default
for (;;) {
	switch (WORDVAL (p)) {
		case OP_EOF:
			return 1;

		case OP_DEFPOINTS: {
			int n = WORDVAL (p+2);
			RotatePointList (modelPointList, new_points, NULL, n, 0);
			p += n*sizeof (vmsVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int n = WORDVAL (p+2);
			int s = WORDVAL (p+4);
			RotatePointList (modelPointList, new_points, NULL, n, s);
			p += n*sizeof (vmsVector) + 8;
			break;
			}

		case OP_FLATPOLY: {
			int nVerts = WORDVAL (p+2);
			int i, nTris;
			GrSetColor (WORDVAL (p+28));
			for (i = 0; i < 2; i++)
				pointList [i] = modelPointList + WORDPTR (p+30) [i];
			for (nTris=nVerts-2;nTris;nTris--) {
				pointList [2] = modelPointList + WORDPTR (p+30) [i++];
				G3CheckAndDrawPoly (3, pointList, NULL, NULL);
				pointList [1] = pointList [2];
				}
			p += 30 + ((nVerts&~1)+1)*2;
			break;
			}

		case OP_TMAPPOLY: {
			int nVerts = WORDVAL (p+2);
			tUVL *uvlList;
			tUVL morph_uvls [3];
			int i, nTris;
			fix light;
			//calculate light from surface normal
			if (nGlow < 0) {			//no glow
				light = -VmVecDot (&viewInfo.view [0].fVec, VECPTR (p+16));
				light = f1_0/4 + (light*3)/4;
				light = FixMul (light, xModelLight);
				}
			else {				//yes glow
				light = xGlowValues [nGlow];
				nGlow = -1;
				}
			//now poke light into l values
			uvlList = (tUVL *) (p+30+ ((nVerts&~1)+1)*2);
			for (i = 0; i < 3; i++)
				morph_uvls [i].l = light;
			for (i = 0; i < 2; i++) {
				pointList [i] = modelPointList + WORDPTR (p+30) [i];
				morph_uvls [i].u = uvlList [i].u;
				morph_uvls [i].v = uvlList [i].v;
				}
			for (nTris = nVerts - 2; nTris; nTris--) {
				pointList [2] = modelPointList + WORDPTR (p+30) [i];
				morph_uvls [2].u = uvlList [i].u;
				morph_uvls [2].v = uvlList [i].v;
				i++;
				G3CheckAndDrawTMap (3, pointList, uvlList, modelBitmaps [WORDVAL (p+28)], NULL, NULL);
				pointList [1] = pointList [2];
				morph_uvls [1].u = morph_uvls [2].u;
				morph_uvls [1].v = morph_uvls [2].v;
				}
			p += 30 + ((nVerts&~1)+1)*2 + nVerts*12;
			break;
			}

		case OP_SORTNORM:
			if (G3CheckNormalFacing (VECPTR (p+16), VECPTR (p+4))) {		//facing
				//draw back then front
				G3DrawMorphingModel (p + WORDVAL (p+30), modelBitmaps, pAnimAngles, vOffset, xModelLight, new_points, nModel);
				G3DrawMorphingModel (p + WORDVAL (p+28), modelBitmaps, pAnimAngles, vOffset, xModelLight, new_points, nModel);

				}
			else {			//not facing.  draw front then back
				G3DrawMorphingModel (p + WORDVAL (p+28), modelBitmaps, pAnimAngles, vOffset, xModelLight, new_points, nModel);
				G3DrawMorphingModel (p + WORDVAL (p+30), modelBitmaps, pAnimAngles, vOffset, xModelLight, new_points, nModel);
				}
			p += 32;
			break;

		case OP_RODBM: {
			g3sPoint rodBotP, rodTopP;
			G3TransformAndEncodePoint (&rodBotP, VECPTR (p+20));
			G3TransformAndEncodePoint (&rodTopP, VECPTR (p+4));
			G3DrawRodTexPoly (modelBitmaps [WORDVAL (p+2)], &rodBotP, WORDVAL (p+16), &rodTopP, WORDVAL (p+32), f1_0, NULL);
			p += 36;
			break;
			}

		case OP_SUBCALL: {
			vmsAngVec	*va = pAnimAngles ? &pAnimAngles [WORDVAL (p+2)] : &avZero;
			vmsVector	vo = *VECPTR (p+4);
			if (gameData.models.nScale)
				VmVecScale (&vo, gameData.models.nScale);
			G3StartInstanceAngles (&vo, va);
			if (vOffset)
				VmVecInc (&vo, vOffset);
			G3DrawPolyModel (NULL, p + WORDVAL (p+16), modelBitmaps, pAnimAngles, &vo, xModelLight, xGlowValues, NULL, NULL, nModel);
			G3DoneInstance ();
			p += 20;
			break;
			}

		case OP_GLOW:
			if (xGlowValues)
				nGlow = WORDVAL (p+2);
			p += 4;
			break;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void InitSubModel (ubyte *p)
{
#ifdef _DEBUG
Assert (++nestCount < 1000);
#endif
for (;;) {
	switch (WORDVAL (p)) {
		case OP_EOF:
			return;

		case OP_DEFPOINTS: {
			int n = WORDVAL (p+2);
			p += n*sizeof (vmsVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int n = WORDVAL (p+2);
			p += n*sizeof (vmsVector) + 8;
			break;
			}

		case OP_FLATPOLY: {
			int nVerts = WORDVAL (p+2);
			Assert (nVerts > 2);		//must have 3 or more points
//				*WORDPTR (p+28) = (short)GrFindClosestColor15bpp (WORDVAL (p+28);
			p += 30 + ((nVerts&~1)+1)*2;
			break;
			}

		case OP_TMAPPOLY: {
			int nVerts = WORDVAL (p+2);
			Assert (nVerts > 2);		//must have 3 or more points
			if (WORDVAL (p+28) > nHighestTexture)
				nHighestTexture = WORDVAL (p+28);
			p += 30 + ((nVerts&~1)+1)*2 + nVerts*12;
			break;
			}

		case OP_SORTNORM:
			InitSubModel (p + WORDVAL (p+28));
			InitSubModel (p + WORDVAL (p+30));
			p += 32;
			break;

		case OP_RODBM:
			p += 36;
			break;

		case OP_SUBCALL: {
			InitSubModel (p + WORDVAL (p+16));
			p += 20;
			break;
			}

		case OP_GLOW:
			p += 4;
			break;

		default:
			Error ("invalid polygon model\n");
		}
	}
}

//------------------------------------------------------------------------------
//init code for bitmap models
void G3InitPolyModel (tPolyModel *pm, int nModel)
{
#ifdef _DEBUG
	nestCount = 0;
#endif

nHighestTexture = -1;
G3CheckAndSwap (pm->modelData);
InitSubModel ((ubyte *) pm->modelData);
G3PolyModelSize (pm, nModel);
}

//------------------------------------------------------------------------------
//eof
