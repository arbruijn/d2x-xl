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
#include "segmath.h"
#include "light.h"
#include "dynlight.h"
#include "lightning.h"

extern CFaceColor tMapColor;

#if DBG_SHADOWS
extern int32_t bShadowTest;
extern int32_t bFrontCap;
extern int32_t bRearCap;
extern int32_t bShadowVolume;
extern int32_t bFrontFaces;
extern int32_t bBackFaces;
extern int32_t bSWCulling;
#endif
extern int32_t bZPass;

#define G3_DRAW_ARRAYS	1
#define G3_USE_VBOS		0

//------------------------------------------------------------------------------

//shadow clipping
//1: Compute hit point of vector from current light through each model vertex (fast)
//2: Compute hit point of vector from current light through each lit submodel vertex (slow)
//3: Compute hit point of vector from current light through each lit model face (fastest, flawed)

int32_t bPrintLine = 0;

int16_t nHighestTexture;
int32_t g3d_interp_outline;

CRenderPoint	*modelPointList = NULL;

#define MAX_INTERP_COLORS 100

//this is a table of mappings from RGB15 to palette colors
typedef struct {int16_t pal_entry, rgb15;} tInterpColor;

tInterpColor interpColorTable [MAX_INTERP_COLORS];

//static int32_t bIntrinsicFacing = 0;
//static int32_t bFlatPolys = 1;
//static int32_t bTexPolys = 1;

CRenderPoint *pointList [MAX_POINTS_PER_POLY];

int16_t nGlow = -1;

//------------------------------------------------------------------------------
//gives the interpreter an array of points to use
void G3SetModelPoints (CRenderPoint *pointlist)
{
modelPointList = pointlist;
}

//------------------------------------------------------------------------------

void RotatePointList (CRenderPoint *dest, CFixVector *src, CRenderNormal *norms, int32_t n, int32_t o)
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
	dest->SetKey ((int16_t) o);
#if 1
	if (norms) {
		if (norms->nFaces > 1) {
			norms->vNormal.v.coord.x /= norms->nFaces;
			norms->vNormal.v.coord.y /= norms->nFaces;
			norms->vNormal.v.coord.z /= norms->nFaces;
			norms->nFaces = 1;
			CFloatVector::Normalize (norms->vNormal);
			}
		dest->Normal () = *norms++;
		}
	else
#endif
		dest->Normal ().Reset ();
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
			transformation.Transform (dest->ViewPos (), v, 0);
#else
			dest.TransformAndEncode (dir);
#endif
			}
		else {
#if 1
			transformation.Transform (dest->ViewPos (), *src, 0);
#else
			dest.TransformAndEncode (*src);
#endif
			}
		pfv->Assign (dest->ViewPos ());
		}
	dest->SetIndex (o++);
	dest->WorldPos () = *src++;
	dest++;
	pfv++;
	}
PROF_END(ptTransform)
}

//------------------------------------------------------------------------------

void G3SwapPolyModelData (uint8_t *pData)
{
	int32_t	i;
	int16_t n;
	tUVL*	uvl_val;

for (;;) {
	UShortSwap (WORDPTR (pData));
	switch (WORDVAL (pData)) {
		case OP_EOF:
			return;

		case OP_DEFPOINTS:
			UShortSwap (WORDPTR (pData + 2));
			n = WORDVAL (pData + 2);
			for (i = 0; i < n; i++)
				VmsVectorSwap (*VECPTR ((pData + 4) + (i * sizeof (CFixVector))));
			pData += n * sizeof (CFixVector) + 4;
			break;

		case OP_DEFP_START:
			UShortSwap (WORDPTR (pData + 2));
			UShortSwap (WORDPTR (pData + 4));
			n = WORDVAL (pData + 2);
			for (i = 0; i < n; i++)
				VmsVectorSwap (*VECPTR ((pData + 8) + (i * sizeof (CFixVector))));
			pData += n * sizeof (CFixVector) + 8;
			break;

		case OP_FLATPOLY:
			UShortSwap (WORDPTR (pData + 2));
			n = WORDVAL (pData + 2);
			VmsVectorSwap (*VECPTR (pData + 4));
			VmsVectorSwap (*VECPTR (pData + 16));
			UShortSwap (WORDPTR (pData + 28));
			for (i=0; i < n; i++)
				UShortSwap (WORDPTR (pData + 30 + (i * 2)));
			pData += 30 + ((n & ~1) + 1) * 2;
			break;

		case OP_TMAPPOLY:
			UShortSwap (WORDPTR (pData + 2));
			n = WORDVAL (pData + 2);
			VmsVectorSwap (*VECPTR (pData + 4));
			VmsVectorSwap (*VECPTR (pData + 16));
			for (i = 0; i < n; i++) {
				uvl_val = reinterpret_cast<tUVL*> (((pData + 30 + (n | 1) * 2))) + i;
				FixSwap (&uvl_val->u);
				FixSwap (&uvl_val->v);
			}
			UShortSwap (WORDPTR (pData + 28));
			for (i = 0; i < n; i++)
				UShortSwap (WORDPTR (pData + 30 + (i * 2)));
			pData += 30 + (n | 1) * 2 + n * 12;
			break;

		case OP_SORTNORM:
			VmsVectorSwap (*VECPTR (pData + 4));
			VmsVectorSwap (*VECPTR (pData + 16));
			UShortSwap (WORDPTR (pData + 28));
			UShortSwap (WORDPTR (pData + 30));
			G3SwapPolyModelData (pData + WORDVAL (pData + 28));
			G3SwapPolyModelData (pData + WORDVAL (pData + 30));
			pData += 32;
			break;

		case OP_RODBM:
			VmsVectorSwap (*VECPTR (pData + 20));
			VmsVectorSwap (*VECPTR (pData + 4));
			UShortSwap (WORDPTR (pData + 2));
			FixSwap (FIXPTR (pData + 16));
			FixSwap (FIXPTR (pData + 32));
			pData += 36;
			break;

		case OP_SUBCALL:
			UShortSwap (WORDPTR (pData + 2));
			VmsVectorSwap (*VECPTR (pData + 4));
			UShortSwap (WORDPTR (pData + 16));
			G3SwapPolyModelData (pData + WORDVAL (pData + 16));
			pData += 20;
			break;

		case OP_GLOW:
			UShortSwap (WORDPTR (pData + 2));
			pData += 4;
			break;

		default:
			Error ("invalid polygon model\n"); //Int3 ();
		}
	}
}

//------------------------------------------------------------------------------

#ifdef WORDS_NEED_ALIGNMENT
void G3AddPolyModelChunk (uint8_t *old_base, uint8_t *new_base, int32_t offset, chunk *chunk_list, int32_t *no_chunks)
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

void G3PolyModelVerify (uint8_t *pData)
{
	int16_t n;

for (;;) {
	switch (WORDVAL (pData)) {
		case OP_EOF:
			return;
		case OP_DEFPOINTS:
			n = (WORDVAL (pData + 2));
			pData += n * sizeof (CFixVector) + 4;
			break;
		case OP_DEFP_START:
			n = (WORDVAL (pData + 2));
			pData += n * sizeof (CFixVector) + 8;
			break;
		case OP_FLATPOLY:
			n = (WORDVAL (pData + 2));
			pData += 30 + (n | 1) * 2;
			break;
		case OP_TMAPPOLY:
			n = (WORDVAL (pData + 2));
			pData += 30 + (n | 1) * 2 + n * 12;
			break;
		case OP_SORTNORM:
			G3PolyModelVerify (pData + WORDVAL (pData + 28));
			G3PolyModelVerify (pData + WORDVAL (pData + 30));
			pData += 32;
			break;
		case OP_RODBM:
			pData += 36;
			break;
		case OP_SUBCALL:
			G3PolyModelVerify (pData + WORDVAL (pData + 16));
			pData += 20;
			break;
		case OP_GLOW:
			pData += 4;
			break;
		default:
			Error ("invalid polygon model\n");
		}
	}
}

//------------------------------------------------------------------------------

int32_t G3CheckAndSwap (void *pData)
{
	int16_t	h = WORDVAL (pData);

if ((h >= 0) && (h <= OP_GLOW))
	return 1;
ShortSwap (&h);
if ((h < 0) || (h > OP_GLOW))
	return 0;
G3SwapPolyModelData (reinterpret_cast<uint8_t*> (pData));
return 1;
}

//------------------------------------------------------------------------------

void GetThrusterPos (int32_t nModel, CFixVector *vNormal, CFixVector *vOffset, CBitmap *pBm, int32_t nPoints)
{
	int32_t				h, i, nSize;
	CFixVector			v, vForward = CFixVector::Create(0,0,I2X (1));
	CModelThrusters	*pThruster = gameData.models.thrusters + nModel;

if (pThruster->nCount >= 2)
	return;
if (pBm) {
	if (!gameData.pig.tex.bitmaps [0].IsElement (pBm))
		return;
	i = (int32_t) (pBm - gameData.pig.tex.bitmaps [0]);
	if ((i != 24) && ((i < 1741) || (i > 1745)))
		return;
	}
#if 1
if (CFixVector::Dot (*vNormal, vForward) > -I2X (1) / 3)
#else
if (vNormal->p.x || vNormal->p.y || (vNormal->p.z != -I2X (1)))
#endif
	return;
for (i = 1, v = pointList [0]->WorldPos (); i < nPoints; i++)
	v += pointList [i]->WorldPos ();
v.v.coord.x /= nPoints;
v.v.coord.y /= nPoints;
v.v.coord.z /= nPoints;
v.v.coord.z -= I2X (1) / 8;
if (vOffset)
	v += *vOffset;
if (pThruster->nCount && (v.v.coord.x == pThruster->vPos [0].v.coord.x) && (v.v.coord.y == pThruster->vPos [0].v.coord.y) && (v.v.coord.z == pThruster->vPos [0].v.coord.z))
	return;
pThruster->vPos [pThruster->nCount] = v;
if (vOffset)
	v -= *vOffset;
pThruster->vDir [pThruster->nCount] = *vNormal;
pThruster->vDir [pThruster->nCount] = -pThruster->vDir [pThruster->nCount];
if (!pThruster->nCount++) {
	for (i = 0, nSize = 0x7fffffff; i < nPoints; i++)
		if (nSize > (h = CFixVector::Dist(v, pointList [i]->WorldPos ())))
			nSize = h;
	pThruster->fSize [i] = X2F (nSize);// * 1.25f;
	}
}

//------------------------------------------------------------------------------

#define CHECK_NORMAL_FACING	0

int32_t G3DrawPolyModel (
	CObject*				pObj,
	void*					modelDataP,
	CArray<CBitmap*>&	modelBitmaps,
	CAngleVector*		pAnimAngles,
	CFixVector*			vOffset,
	fix					xModelLight,
	fix*					xGlowValues,
	CFloatVector*		pColor,
	POF::CModel*		po,
	int32_t				nModel)
{
	uint8_t *p = reinterpret_cast<uint8_t*> (modelDataP);
	int16_t	nTag;
	int16_t bGetThrusterPos, bLightning;

	static int32_t nDepth = -1;

if (gameStates.app.bNostalgia)
	bGetThrusterPos = 
	bLightning = 0;
else {
	bGetThrusterPos = !pObj || (pObj->info.nType == OBJ_PLAYER) || (pObj->info.nType == OBJ_ROBOT) || (pObj->IsProjectile ());
	bLightning = SHOW_LIGHTNING (1) && gameOpts->render.lightning.bDamage && pObj && (pObj->Damage () < 0.5f);
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
	!nDepth && !po && pObj && ((pObj->info.nType == OBJ_ROBOT) || (pObj->info.nType == OBJ_PLAYER))) {
	po = gameData.models.pofData [gameStates.app.bD1Mission][0] + nModel;
	POFGatherPolyModelItems (pObj, modelDataP, pAnimAngles, po, 0);
	}
nGlow = -1;		//glow off by default
ogl.SetFaceCulling (true);
OglCullFace (0);
for (;;) {
	nTag = WORDVAL (p);
	if (nTag == OP_EOF)
		break;
	else if (nTag == OP_DEFPOINTS) {
		int32_t n = WORDVAL (p+2);
		RotatePointList (modelPointList, VECPTR (p+4), po ? po->m_vertNorms.Buffer () : NULL, n, 0);
		p += n * sizeof (CFixVector) + 4;
		break;
		}
	else if (nTag == OP_DEFP_START) {
		int32_t n = WORDVAL (p+2);
		int32_t s = WORDVAL (p+4);

		RotatePointList (modelPointList, VECPTR (p+8), po ? po->m_vertNorms.Buffer () : NULL, n, s);
		p += n * sizeof (CFixVector) + 8;
		}
	else if (nTag == OP_FLATPOLY) {
		int32_t nVerts = WORDVAL (p+2);
		Assert (nVerts < MAX_POINTS_PER_POLY);
#if CHECK_NORMAL_FACING
		if (G3CheckNormalFacing (*VECPTR (p+4), *VECPTR (p+16)) > 0)
#endif
		 {
			//fix l = X2I (32 * xModelLight);
			CCanvas::Current ()->SetColorRGB15bpp (WORDVAL (p+28), (uint8_t) (255 * gameStates.render.grAlpha));
			CCanvas::Current ()->FadeColorRGB (1.0);
			if (pColor) {
				pColor->Red () = (float) CCanvas::Current ()->Color ().Red () / 255.0f;
				pColor->Green () = (float) CCanvas::Current ()->Color ().Green () / 255.0f;
				pColor->Blue () = (float) CCanvas::Current ()->Color ().Blue () / 255.0f;
				}
			p += 30;
			for (int32_t i = 0; i < nVerts; i++)
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
		int32_t nVerts = WORDVAL (p+2);
		Assert ( nVerts < MAX_POINTS_PER_POLY );
#if CHECK_NORMAL_FACING
		if (G3CheckNormalFacing (*VECPTR (p+4), *VECPTR (p+16)) > 0)
#endif
		 {
			tUVL *uvlList;
			int32_t i;
			fix l;
			CFixVector *pvn = VECPTR (p+16);

			//calculate light from surface Normal
			if (nGlow < 0) {			//no glow
				l = -CFixVector::Dot (transformation.m_info.view[0].m.dir.f, *VECPTR(p+16));
				l = I2X (1) / 4 + (l * 3) / 4;
				l = FixMul (l, xModelLight);
				}
			else {				//yes glow
				l = xGlowValues [nGlow];
				nGlow = -1;
				}
			//now poke light into l values
			if (pColor)
				paletteManager.Game ()->ToRgbaf (modelBitmaps [WORDVAL (p+28)]->AvgColor (), *pColor);
			p += 30;
			uvlList = reinterpret_cast<tUVL*> (p + (nVerts | 1) * 2);
			for (i = 0; i < nVerts; i++) {
				uvlList [i].l = l;
				pointList [i] = modelPointList + WORDPTR (p) [i];
				}
			tMapColor = gameData.objData.color;
			if (gameStates.render.bCloaked)
				G3DrawTexPolyFlat (nVerts, pointList, uvlList, NULL, modelBitmaps [WORDVAL (p-2)], NULL, NULL, VECPTR (p+16), 0, 1, 0, -1);
			else
				G3DrawTexPolySimple (nVerts, pointList, uvlList, modelBitmaps [WORDVAL (p-2)], VECPTR (p+16), 1);
			if (!gameStates.render.bBriefing) {
				if (bLightning)
					lightningManager.RenderForDamage (pObj, pointList, NULL, nVerts);
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
			if (!(G3DrawPolyModel (pObj, p + WORDVAL (p+30), modelBitmaps, pAnimAngles, vOffset, xModelLight, xGlowValues, pColor, po, nModel) &&
					G3DrawPolyModel (pObj, p + WORDVAL (p+28), modelBitmaps, pAnimAngles, vOffset, xModelLight, xGlowValues, pColor, po, nModel)))
				return 0;
			}
#if CHECK_NORMAL_FACING
		else {			//not facing.  draw front then back
			if (!(G3DrawPolyModel (pObj, p + WORDVAL (p+28), modelBitmaps, pAnimAngles, vOffset, xModelLight, xGlowValues, pColor, po, nModel) &&
					G3DrawPolyModel (pObj, p + WORDVAL (p+30), modelBitmaps, pAnimAngles, vOffset, xModelLight, xGlowValues, pColor, po, nModel)))
				return 0;
			}
#endif
		p += 32;
		}
	else if (nTag == OP_RODBM) {
		CRenderPoint rodBotP, rodTopP;
		rodBotP.TransformAndEncode (*VECPTR (p+20));
		rodTopP.TransformAndEncode (*VECPTR (p+4));
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
		if (!G3DrawPolyModel (pObj, p + WORDVAL (p+16), modelBitmaps, pAnimAngles, &vo, xModelLight, xGlowValues, pColor, po, nModel)) {
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
		PrintLog (0, "invalid polygon model\n");
#endif
		return 0;
		}
	}
nDepth--;
return 1;
}

//------------------------------------------------------------------------------
//alternate interpreter for morphing CObject
int32_t G3DrawMorphingModel (void *pModel, CArray<CBitmap*>& modelBitmaps, CAngleVector *pAnimAngles, CFixVector *vOffset,
								  fix xModelLight, CFixVector *new_points, int32_t nModel)
{
	uint8_t *p = reinterpret_cast<uint8_t*> (pModel);
	fix *xGlowValues = NULL;

G3CheckAndSwap (pModel);
nGlow = -1;		//glow off by default
for (;;) {
	switch (WORDVAL (p)) {
		case OP_EOF:
			return 1;

		case OP_DEFPOINTS: {
			int32_t n = WORDVAL (p+2);
			RotatePointList (modelPointList, new_points, NULL, n, 0);
			p += n * sizeof (CFixVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int32_t n = WORDVAL (p+2);
			int32_t s = WORDVAL (p+4);
			RotatePointList (modelPointList, new_points, NULL, n, s);
			p += n * sizeof (CFixVector) + 8;
			break;
			}

		case OP_FLATPOLY: {
			int32_t nVerts = WORDVAL (p+2);
			int32_t i, nTris;
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
			int32_t nVerts = WORDVAL (p+2);
			tUVL *uvlList;
			tUVL morph_uvls [3];
			int32_t i, nTris;
			fix light;
			//calculate light from surface Normal
			if (nGlow < 0) {			//no glow
				light = -CFixVector::Dot (transformation.m_info.view [0].m.dir.f, *VECPTR (p+16));
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
			CRenderPoint rodBotP, rodTopP;
			rodBotP.TransformAndEncode (*VECPTR (p+20));
			rodTopP.TransformAndEncode (*VECPTR (p+4));
			G3DrawRodTexPoly (modelBitmaps [WORDVAL (p+2)], &rodBotP, WORDVAL (p+16), &rodTopP, WORDVAL (p+32), I2X (1), NULL);
			p += 36;
			break;
			}

		case OP_SUBCALL: {
			const CAngleVector	*va = pAnimAngles ? &pAnimAngles [WORDVAL (p+2)] : &CAngleVector::ZERO;
			CFixVector	vo = *VECPTR (p+4);
			if (!gameData.models.vScale.IsZero ())
				vo *= gameData.models.vScale;
			transformation.Begin (vo, *va);
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
