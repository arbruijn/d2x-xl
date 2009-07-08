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

#include <math.h>
#include <stdlib.h>
#include "error.h"

#include "descent.h"
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
#include "rendermine.h"
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
typedef struct {short pal_entry, rgb15;} tInterpColor;

tInterpColor interpColorTable [MAX_INTERP_COLORS];

//static int bIntrinsicFacing = 0;
//static int bFlatPolys = 1;
//static int bTexPolys = 1;

g3sPoint *pointList [MAX_POINTS_PER_POLY];

short nGlow = -1;

//------------------------------------------------------------------------------
//gives the interpreter an array of points to use
void G3SetModelPoints (g3sPoint *pointlist)
{
modelPointList = pointlist;
}

//------------------------------------------------------------------------------

void RotatePointList (g3sPoint *dest, CFixVector *src, g3sNormal *norms, int n, int o)
{
PROF_START
	CFloatVector	*pfv = gameData.models.fPolyModelVerts + o;
	CFloatVector	fScale;
	bool				bScale;

if ((bScale = !gameData.models.vScale.IsZero ()))
	fScale.Assign (gameData.models.vScale);

dest += o;
if (norms)
	norms += o;
while (n--) {
	dest->p3_key = (short) o;
#if 1
	if (norms) {
		if (norms->nFaces > 1) {
			norms->vNormal[X] /= norms->nFaces;
			norms->vNormal[Y] /= norms->nFaces;
			norms->vNormal[Z] /= norms->nFaces;
			norms->nFaces = 1;
			CFloatVector::Normalize (norms->vNormal);
			}
		dest->p3_normal = *norms++;
		}
	else
#endif
		dest->p3_normal.nFaces = 0;
	if (ogl.m_states.bUseTransform) {
		pfv->Assign (*src);
		if (bScale)
			pfv->Scale (fScale);
		}
	else {
		if (!gameData.models.vScale.IsZero ()) {
			CFixVector v = *src;
			v *= gameData.models.vScale;
#if 1
			transformation.Transform (dest->p3_vec, v, 0);
#else
			G3TransformAndEncodePoint (dest, &v);
#endif
			}
		else {
#if 1
			transformation.Transform (dest->p3_vec, *src, 0);
#else
			G3TransformAndEncodePoint (dest, src);
#endif
			}
		pfv->Assign (dest->p3_vec);
		}
	dest->p3_index = o++;
	dest->p3_src = *src++;
	dest++;
	pfv++;
	}
PROF_END(ptTransform)
}

//------------------------------------------------------------------------------

void G3SwapPolyModelData (ubyte *dataP)
{
	int	i;
	short n;
	tUVL*	uvl_val;

for (;;) {
	ShortSwap (WORDPTR (dataP));
	switch (WORDVAL (dataP)) {
		case OP_EOF:
			return;

		case OP_DEFPOINTS:
			ShortSwap (WORDPTR (dataP + 2));
			n = WORDVAL (dataP + 2);
			for (i = 0; i < n; i++)
				VmsVectorSwap (*VECPTR ((dataP + 4) + (i * sizeof (CFixVector))));
			dataP += n * sizeof (CFixVector) + 4;
			break;

		case OP_DEFP_START:
			ShortSwap (WORDPTR (dataP + 2));
			ShortSwap (WORDPTR (dataP + 4));
			n = WORDVAL (dataP + 2);
			for (i = 0; i < n; i++)
				VmsVectorSwap (*VECPTR ((dataP + 8) + (i * sizeof (CFixVector))));
			dataP += n * sizeof (CFixVector) + 8;
			break;

		case OP_FLATPOLY:
			ShortSwap (WORDPTR (dataP + 2));
			n = WORDVAL (dataP + 2);
			VmsVectorSwap (*VECPTR (dataP + 4));
			VmsVectorSwap (*VECPTR (dataP + 16));
			ShortSwap (WORDPTR (dataP + 28));
			for (i=0; i < n; i++)
				ShortSwap (WORDPTR (dataP + 30 + (i * 2)));
			dataP += 30 + ((n & ~1) + 1) * 2;
			break;

		case OP_TMAPPOLY:
			ShortSwap (WORDPTR (dataP + 2));
			n = WORDVAL (dataP + 2);
			VmsVectorSwap (*VECPTR (dataP + 4));
			VmsVectorSwap (*VECPTR (dataP + 16));
			for (i = 0; i < n; i++) {
				uvl_val = reinterpret_cast<tUVL*> (((dataP + 30 + ((n & ~1) + 1) * 2))) + i;
				FixSwap (&uvl_val->u);
				FixSwap (&uvl_val->v);
			}
			ShortSwap (WORDPTR (dataP + 28));
			for (i=0;i<n;i++)
				ShortSwap (WORDPTR (dataP + 30 + (i * 2)));
			dataP += 30 + ((n & ~1) + 1) * 2 + n * 12;
			break;

		case OP_SORTNORM:
			VmsVectorSwap (*VECPTR (dataP + 4));
			VmsVectorSwap (*VECPTR (dataP + 16));
			ShortSwap (WORDPTR (dataP + 28));
			ShortSwap (WORDPTR (dataP + 30));
			G3SwapPolyModelData (dataP + WORDVAL (dataP + 28));
			G3SwapPolyModelData (dataP + WORDVAL (dataP + 30));
			dataP += 32;
			break;

		case OP_RODBM:
			VmsVectorSwap (*VECPTR (dataP + 20));
			VmsVectorSwap (*VECPTR (dataP + 4));
			ShortSwap (WORDPTR (dataP + 2));
			FixSwap (FIXPTR (dataP + 16));
			FixSwap (FIXPTR (dataP + 32));
			dataP += 36;
			break;

		case OP_SUBCALL:
			ShortSwap (WORDPTR (dataP + 2));
			VmsVectorSwap (*VECPTR (dataP + 4));
			ShortSwap (WORDPTR (dataP + 16));
			G3SwapPolyModelData (dataP + WORDVAL (dataP + 16));
			dataP += 20;
			break;

		case OP_GLOW:
			ShortSwap (WORDPTR (dataP + 2));
			dataP += 4;
			break;

		default:
			Error ("invalid polygon model\n"); //Int3 ();
		}
	}
}

//------------------------------------------------------------------------------

#ifdef WORDS_NEED_ALIGNMENT
void G3AddPolyModelChunk (ubyte *old_base, ubyte *new_base, int offset, chunk *chunk_list, int *no_chunks)
{
Assert (*no_chunks + 1 < MAX_CHUNKS); //increase MAX_CHUNKS if you get this
chunk_list [*no_chunks].old_base = old_base;
chunk_list [*no_chunks].new_base = new_base;
chunk_list [*no_chunks].offset = offset;
chunk_list [*no_chunks].correction = 0;
(*no_chunks)++;
}
#endif

//------------------------------------------------------------------------------

void G3PolyModelVerify (ubyte *dataP)
{
	short n;

for (;;) {
	switch (WORDVAL (dataP)) {
		case OP_EOF:
			return;
		case OP_DEFPOINTS:
			n = (WORDVAL (dataP + 2));
			dataP += n * sizeof (CFixVector) + 4;
			break;
		case OP_DEFP_START:
			n = (WORDVAL (dataP + 2));
			dataP += n * sizeof (CFixVector) + 8;
			break;
		case OP_FLATPOLY:
			n = (WORDVAL (dataP + 2));
			dataP += 30 + ((n & ~1) + 1) * 2;
			break;
		case OP_TMAPPOLY:
			n = (WORDVAL (dataP + 2));
			dataP += 30 + ((n & ~1) + 1) * 2 + n * 12;
			break;
		case OP_SORTNORM:
			G3PolyModelVerify (dataP + WORDVAL (dataP + 28));
			G3PolyModelVerify (dataP + WORDVAL (dataP + 30));
			dataP += 32;
			break;
		case OP_RODBM:
			dataP += 36;
			break;
		case OP_SUBCALL:
			G3PolyModelVerify (dataP + WORDVAL (dataP + 16));
			dataP += 20;
			break;
		case OP_GLOW:
			dataP += 4;
			break;
		default:
			Error ("invalid polygon model\n");
		}
	}
}

//------------------------------------------------------------------------------

int G3CheckAndSwap (void *dataP)
{
	short	h = WORDVAL (dataP);

if ((h >= 0) && (h <= OP_GLOW))
	return 1;
ShortSwap (&h);
if ((h < 0) || (h > OP_GLOW))
	return 0;
G3SwapPolyModelData (reinterpret_cast<ubyte*> (dataP));
return 1;
}

//------------------------------------------------------------------------------

void GetThrusterPos (int nModel, CFixVector *vNormal, CFixVector *vOffset, CBitmap *bmP, int nPoints)
{
	int					h, i, nSize;
	CFixVector			v, vForward = CFixVector::Create(0,0,I2X (1));
	CModelThrusters	*mtP = gameData.models.thrusters + nModel;

if (mtP->nCount >= 2)
	return;
if (bmP) {
	if (!gameData.pig.tex.bitmaps [0].IsElement (bmP))
		return;
	i = (int) (bmP - gameData.pig.tex.bitmaps [0]);
	if ((i != 24) && ((i < 1741) || (i > 1745)))
		return;
	}
#if 1
if (CFixVector::Dot (*vNormal, vForward) > -I2X (1) / 3)
#else
if (vNormal->p.x || vNormal->p.y || (vNormal->p.z != -I2X (1)))
#endif
	return;
for (i = 1, v = pointList [0]->p3_src; i < nPoints; i++)
	v += pointList [i]->p3_src;
v[X] /= nPoints;
v[Y] /= nPoints;
v[Z] /= nPoints;
v[Z] -= I2X (1) / 8;
if (vOffset)
	v += *vOffset;
if (mtP->nCount && (v[X] == mtP->vPos [0][X]) && (v[Y] == mtP->vPos [0][Y]) && (v[Z] == mtP->vPos [0][Z]))
	return;
mtP->vPos [mtP->nCount] = v;
if (vOffset)
	v -= *vOffset;
mtP->vDir [mtP->nCount] = *vNormal;
mtP->vDir [mtP->nCount] = -mtP->vDir [mtP->nCount];
if (!mtP->nCount++) {
	for (i = 0, nSize = 0x7fffffff; i < nPoints; i++)
		if (nSize > (h = CFixVector::Dist(v, pointList [i]->p3_src)))
			nSize = h;
	mtP->fSize = X2F (nSize);// * 1.25f;
	}
}

//------------------------------------------------------------------------------

#define CHECK_NORMAL_FACING	0

int G3DrawPolyModel (
	CObject*				objP,
	void*					modelDataP,
	CArray<CBitmap*>&	modelBitmaps,
	CAngleVector*		pAnimAngles,
	CFixVector*			vOffset,
	fix					xModelLight,
	fix*					xGlowValues,
	tRgbaColorf*		colorP,
	POF::CModel*		po,
	int					nModel)
{
	ubyte *p = reinterpret_cast<ubyte*> (modelDataP);
	short	nTag;
	short bGetThrusterPos, bLightnings;

	static int nDepth = -1;

if (gameStates.app.bNostalgia)
	bGetThrusterPos = 
	bLightnings = 0;
else {
	bGetThrusterPos = !objP || ((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT) || 
						   ((objP->info.nType == OBJ_WEAPON) && gameData.objs.bIsMissile [objP->info.nId]));
	bLightnings = SHOW_LIGHTNING && gameOpts->render.lightning.bDamage && objP && (objP->Damage () < 0.5f);
	}
#if DBG_SHADOWS
if (bShadowTest)
	return 1;
#endif
#if CONTOUR_OUTLINE
//if (extraGameInfo [0].bShadows && (gameStates.render.nShadowPass < 3))
	return 1;
#endif
nDepth++;
G3CheckAndSwap (modelDataP);
if (SHOW_DYN_LIGHT &&
	!nDepth && !po && objP && ((objP->info.nType == OBJ_ROBOT) || (objP->info.nType == OBJ_PLAYER))) {
	po = gameData.models.pofData [gameStates.app.bD1Mission][0] + nModel;
	POFGatherPolyModelItems (objP, modelDataP, pAnimAngles, po, 0);
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
		RotatePointList (modelPointList, VECPTR (p+4), po ? po->m_vertNorms.Buffer () : NULL, n, 0);
		p += n * sizeof (CFixVector) + 4;
		break;
		}
	else if (nTag == OP_DEFP_START) {
		int n = WORDVAL (p+2);
		int s = WORDVAL (p+4);

		RotatePointList (modelPointList, VECPTR (p+8), po ? po->m_vertNorms.Buffer () : NULL, n, s);
		p += n * sizeof (CFixVector) + 8;
		}
	else if (nTag == OP_FLATPOLY) {
		int nVerts = WORDVAL (p+2);
		Assert (nVerts < MAX_POINTS_PER_POLY);
#if CHECK_NORMAL_FACING
		if (G3CheckNormalFacing (*VECPTR (p+4), *VECPTR (p+16)) > 0)
#endif
		 {
			int i;
			//fix l = X2I (32 * xModelLight);
			CCanvas::Current ()->SetColorRGB15bpp (WORDVAL (p+28), (ubyte) (255 * gameStates.render.grAlpha));
			CCanvas::Current ()->FadeColorRGB (1.0);
			if (colorP) {
				colorP->red = (float) CCanvas::Current ()->Color ().color.red / 255.0f;
				colorP->green = (float) CCanvas::Current ()->Color ().color.green / 255.0f;
				colorP->blue = (float) CCanvas::Current ()->Color ().color.blue / 255.0f;
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
		if (G3CheckNormalFacing (*VECPTR (p+4), *VECPTR (p+16)) > 0)
#endif
		 {
			tUVL *uvlList;
			int i;
			fix l;
			CFixVector *pvn = VECPTR (p+16);

			//calculate light from surface Normal
			if (nGlow < 0) {			//no glow
				l = -CFixVector::Dot (transformation.m_info.view[0].FVec (), *VECPTR(p+16));
				l = I2X (1) / 4 + (l * 3) / 4;
				l = FixMul (l, xModelLight);
				}
			else {				//yes glow
				l = xGlowValues [nGlow];
				nGlow = -1;
				}
			//now poke light into l values
			uvlList = reinterpret_cast<tUVL*> (p + 30 + (nVerts | 1) * 2);
			for (i = 0; i < nVerts; i++)
				uvlList [i].l = l;

			if (colorP)
				paletteManager.Game ()->ToRgbaf (modelBitmaps [WORDVAL (p+28)]->AvgColor (), *colorP);
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
					lightningManager.RenderForDamage (objP, pointList, NULL, nVerts);
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
		if (G3CheckNormalFacing (*VECPTR (p+16), *VECPTR (p+4)) > 0)
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
		G3TransformAndEncodePoint (&rodBotP, *VECPTR (p+20));
		G3TransformAndEncodePoint (&rodTopP, *VECPTR (p+4));
		G3DrawRodTexPoly (modelBitmaps [WORDVAL (p+2)], &rodBotP, WORDVAL (p+16), &rodTopP, WORDVAL (p+32), I2X (1), NULL);
		p += 36;
		}
	else if (nTag == OP_SUBCALL) {
		CAngleVector	va;
		CFixVector	vo;

		va = pAnimAngles ? pAnimAngles [WORDVAL (p+2)] : CAngleVector::ZERO;
		vo = *VECPTR (p+4);
		if (!gameData.models.vScale.IsZero ())
			vo *= gameData.models.vScale;
		transformation.Begin (vo, va);
		if (vOffset)
			vo += *vOffset;
		if (!G3DrawPolyModel (objP, p + WORDVAL (p+16), modelBitmaps, pAnimAngles, &vo, xModelLight, xGlowValues, colorP, po, nModel)) {
			transformation.End ();
			return 0;
			}
		transformation.End ();
		p += 20;
		}
	else if (nTag == OP_GLOW) {
		if (xGlowValues)
			nGlow = WORDVAL (p+2);
		p += 4;
		}
	else {
#if DBG
		PrintLog ("invalid polygon model\n");
#endif
		return 0;
		}
	}
nDepth--;
return 1;
}

//------------------------------------------------------------------------------
//alternate interpreter for morphing CObject
int G3DrawMorphingModel (void *modelP, CArray<CBitmap*>& modelBitmaps, CAngleVector *pAnimAngles, CFixVector *vOffset,
								  fix xModelLight, CFixVector *new_points, int nModel)
{
	ubyte *p = reinterpret_cast<ubyte*> (modelP);
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
			p += n * sizeof (CFixVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int n = WORDVAL (p+2);
			int s = WORDVAL (p+4);
			RotatePointList (modelPointList, new_points, NULL, n, s);
			p += n * sizeof (CFixVector) + 8;
			break;
			}

		case OP_FLATPOLY: {
			int nVerts = WORDVAL (p+2);
			int i, nTris;
			CCanvas::Current ()->SetColor (WORDVAL (p+28));
			for (i = 0; i < 2; i++)
				pointList [i] = modelPointList + WORDPTR (p+30) [i];
			for (nTris = nVerts - 2; nTris; nTris--) {
				pointList [2] = modelPointList + WORDPTR (p+30) [i++];
				G3CheckAndDrawPoly (3, pointList, NULL, NULL);
				pointList [1] = pointList [2];
				}
			p += 30 + ((nVerts & ~1) + 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int nVerts = WORDVAL (p+2);
			tUVL *uvlList;
			tUVL morph_uvls [3];
			int i, nTris;
			fix light;
			//calculate light from surface Normal
			if (nGlow < 0) {			//no glow
				light = -CFixVector::Dot (transformation.m_info.view [0].FVec (), *VECPTR (p+16));
				light = I2X (1)/4 + (light*3)/4;
				light = FixMul (light, xModelLight);
				}
			else {				//yes glow
				light = xGlowValues [nGlow];
				nGlow = -1;
				}
			//now poke light into l values
			uvlList = reinterpret_cast<tUVL*> (p+30+ ((nVerts & ~1) + 1) * 2);
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
			p += 30 + ((nVerts & ~1) + 1) * 2 + nVerts*12;
			break;
			}

		case OP_SORTNORM:
			if (G3CheckNormalFacing (*VECPTR (p+16), *VECPTR (p+4))) {		//facing
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
			G3TransformAndEncodePoint (&rodBotP, *VECPTR (p+20));
			G3TransformAndEncodePoint (&rodTopP, *VECPTR (p+4));
			G3DrawRodTexPoly (modelBitmaps [WORDVAL (p+2)], &rodBotP, WORDVAL (p+16), &rodTopP, WORDVAL (p+32), I2X (1), NULL);
			p += 36;
			break;
			}

		case OP_SUBCALL: {
			const CAngleVector	*va = pAnimAngles ? &pAnimAngles [WORDVAL (p+2)] : &CAngleVector::ZERO;
			CFixVector	vo = *VECPTR (p+4);
			if (!gameData.models.vScale.IsZero ())
				vo *= gameData.models.vScale;
			transformation.Begin(vo, *va);
			if (vOffset)
				vo += *vOffset;
			G3DrawPolyModel (NULL, p + WORDVAL (p+16), modelBitmaps, pAnimAngles, &vo, xModelLight, xGlowValues, NULL, NULL, nModel);
			transformation.End ();
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
//eof
