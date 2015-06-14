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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "u_mem.h"
#include "error.h"
#include "segmath.h"
#include "network.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "ogl_color.h"
#include "ogl_render.h"
#include "ogl_hudstuff.h"
#include "renderlib.h"
#include "renderthreads.h"
#include "cameras.h"
#include "menubackground.h"
#include "input.h"
#include "mouse.h"
#include "newdemo.h"
#include "cockpit.h"
#include "timer.h"

//------------------------------------------------------------------------------

#define SW_CULLING 1

//------------------------------------------------------------------------------

int32_t	bOutLineMode = 0,
		bShowOnlyCurSide = 0;

//------------------------------------------------------------------------------

int32_t FaceIsVisible (int16_t nSegment, int16_t nSide)
{
#if SW_CULLING
CSegment *pSeg = SEGMENT (nSegment);
CSide *pSide = pSeg->m_sides + nSide;
CFixVector v;
if (pSide->m_nType == SIDE_IS_QUAD) {
	v = gameData.renderData.mine.viewer.vPos - pSeg->SideCenter (nSide); //gameData.segData.vertices + pSeg->m_vertices [sideVertIndex [nSide][0]]);
	return CFixVector::Dot (pSide->m_normals [0], v) >= 0;
	}
v = gameData.renderData.mine.viewer.vPos - VERTICES [pSide->m_vertices [(pSide->m_nType == SIDE_IS_TRI_13) ? 3 : 0]];
return (CFixVector::Dot (pSide->m_normals [0], v) >= 0) || (CFixVector::Dot (pSide->m_normals [1], v) >= 0);
#else
return 1;
#endif
}

//------------------------------------------------------------------------------

void RotateTexCoord2f (tTexCoord2f& dest, tTexCoord2f& src, uint8_t nOrient)
{
if (nOrient == 1) {
	dest.v.u = 1.0f - src.v.v;
	dest.v.v = src.v.u;
	}
else if (nOrient == 2) {
	dest.v.u = 1.0f - src.v.u;
	dest.v.v = 1.0f - src.v.v;
	}
else if (nOrient == 3) {
	dest.v.u = src.v.v;
	dest.v.v = 1.0f - src.v.u;
	}
else {
	dest.v.u = src.v.u;
	dest.v.v = src.v.v;
	}
}

//------------------------------------------------------------------------------

int32_t ToggleOutlineMode (void)
{
return bOutLineMode = !bOutLineMode;
}

//------------------------------------------------------------------------------

int32_t ToggleShowOnlyCurSide (void)
{
return bShowOnlyCurSide = !bShowOnlyCurSide;
}

//------------------------------------------------------------------------------

void DrawOutline (int32_t nVertices, CRenderPoint **pointList)
{
	GLint				depthFunc;
	CRenderPoint	center, normal;
	CFixVector		n;

#if 1 //!DBG
if (gameStates.render.bQueryOcclusion) {
	CFloatVector outlineColor = {{{1, 1, 0, -1}}};
	G3DrawPolyAlpha (nVertices, pointList, &outlineColor, 1, -1);
	return;
	}
#endif

glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
ogl.SetDepthMode (GL_ALWAYS);
CCanvas::Current ()->SetColorRGB (255, 255, 255, 255);
center.ViewPos ().SetZero ();
for (int32_t i = 0; i < nVertices; i++) {
	G3DrawLine (pointList [i], pointList [(i + 1) % nVertices]);
	center.ViewPos () += pointList [i]->ViewPos ();
	n.Assign (*pointList [i]->GetNormal ());
	transformation.Rotate (n, n, 0);
	normal.ViewPos () = pointList [i]->ViewPos () + (n * (I2X (10)));
	G3DrawLine (pointList [i], &normal);
	}
#if 0
VmVecNormal (&Normal.m_vertex [1],
				 &pointList [0]->m_vertex [1],
				 &pointList [1]->m_vertex [1],
				 &pointList [2]->m_vertex [1]);
VmVecInc (&Normal.m_vertex [1], &center.m_vertex [1]);
VmVecScale (&Normal.m_vertex [1], I2X (10));
G3DrawLine (&center, &Normal);
#endif
ogl.SetDepthMode (depthFunc);
}

// ----------------------------------------------------------------------------

char IsColoredSeg (int16_t nSegment)
{
if (nSegment < 0)
	return 0;
//if (!gameStates.render.nLightingMethod)
//	return 0;
CSegment* pSeg = SEGMENT (nSegment);
if (IsEntropyGame && (extraGameInfo [1].entropy.nOverrideTextures == 2) && (pSeg->m_owner > 0))
	return (pSeg->m_owner == 1) ? 2 : 1;
if (!missionConfig.m_bColoredSegments)
	return 0;
int32_t nFogType = pSeg->FogType ();
if (nFogType)
	return 2 + nFogType;
#if 0
if (pSeg->m_function == SEGMENT_FUNC_TEAM_BLUE) 
	return 1;
if (pSeg->m_function == SEGMENT_FUNC_TEAM_RED)
	return 2;
#endif
return 0;
}

// ----------------------------------------------------------------------------

char IsColoredSegFace (int16_t nSegment, int16_t nSide)
{
#if 0 //!DBG
if (!gameStates.render.nLightingMethod)
	return 0;
#endif
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
#endif
	CSegment*	pSeg = SEGMENT (nSegment);
	CSegment*	pConnSeg = SEGMENT (pSeg->m_children [nSide]);

if (IsEntropyGame && (extraGameInfo [1].entropy.nOverrideTextures == 2) && (pSeg->m_owner > 0)) {
	if (!pConnSeg || (pConnSeg->m_owner != pSeg->m_owner))
		return (pSeg->m_owner == 1) ? 2 : 1;
	}

if (!missionConfig.m_bColoredSegments)
	return 0;

int32_t nFogType = pSeg->FogType ();
if (!pConnSeg) {
	return nFogType ? 2 + nFogType : 0;
	}
int32_t nOtherFogType = pConnSeg->FogType ();
if (nFogType != nOtherFogType)
	return (nFogType ? nFogType : nOtherFogType) + 2;
#if 1
return 0;
#else
if (pSeg->m_function == pConnSeg->m_function)
	return 0;
return (pSeg->m_function == SEGMENT_FUNC_TEAM_BLUE) || 
		 (pSeg->m_function == SEGMENT_FUNC_TEAM_RED) || 
		 (pConnSeg->m_function == SEGMENT_FUNC_TEAM_BLUE) || 
		 (pConnSeg->m_function == SEGMENT_FUNC_TEAM_RED);
#endif
}

// ----------------------------------------------------------------------------

CFloatVector segmentColors [6] = {
#if 0 //DBG
	 {{{1, 1, 1, 0}}},
	 {{{1, 1, 1, 0}}},
	 {{{1, 1, 1, 0}}},
	 {{{1, 1, 1, 0}}},
	 {{{1, 1, 1, 0}}},
	 {{{1, 1, 1, 0}}}
#else
	 {{{0.8f, 0.6f, 0, 0.333f}}},
	 {{{0.0, 0.6f, 0.8f, 0.333f}}},
	 {{{0.2f, 0.4f, 0.6f, 0.333f}}},
	 {{{1.0f, 0.7f, 0.4f, 0.333f}}},
	 {{{0.7f, 0.7f, 0.7f, 0.0f}}},
	 {{{0.7f, 0.7f, 0.7f, 0.0f}}}
#endif
	 };

CFloatVector *ColoredSegmentColor (int32_t nSegment, int32_t nSide, char nColor)
{
//if (!gameStates.render.nLightingMethod)
//	return NULL;

	CSegment*	pSeg = SEGMENT (nSegment);
	CSegment*	pConnSeg = SEGMENT (pSeg->m_children [nSide]);

#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
#endif

#if 0
if (nColor > 0)
	nColor--;
else 
#endif
	{
	if (IsEntropyGame && (extraGameInfo [1].entropy.nOverrideTextures == 2) && (pSeg->m_owner > 0)) {
		if (pConnSeg && (pConnSeg->m_owner == pSeg->m_owner))
			return NULL;
		nColor = (pSeg->m_owner == 1);
		}

	char nFogType = pSeg->FogType ();
	char nOtherFogType = pConnSeg ? pConnSeg->FogType () : 0;
	if (nFogType != nOtherFogType) {
#if 1 // no colored segment faces of colored segments if volumetric fog is enabled
		if ((ogl.m_features.bDepthBlending >= 0) && gameOpts->render.effects.bEnabled && gameOpts->render.effects.bFog &&  (gameOptions [0].render.nQuality > 1))
			return NULL;
#endif
		if (!missionConfig.m_bColoredSegments)
			return NULL;
		nColor = (nFogType ? nFogType : nOtherFogType) + 1;
		}
	if (pConnSeg && (pSeg->m_function != pConnSeg->m_function)) {
		if ((nFogType != nOtherFogType) &&
			 (pSeg->m_function != SEGMENT_FUNC_TEAM_BLUE) &&
			 (pSeg->m_function != SEGMENT_FUNC_TEAM_RED) &&
			 (pConnSeg->m_function != SEGMENT_FUNC_TEAM_BLUE) &&
			 (pConnSeg->m_function != SEGMENT_FUNC_TEAM_RED))
			return NULL;
		if (IS_WALL (pSeg->WallNum (nSide)))
			return NULL;
		}
	}
return segmentColors + nColor;
}

//------------------------------------------------------------------------------
// If any color component > 1, scale all components down so that the greatest == 1.

static inline void ScaleColor (CFaceColor *pColor, float l)
{
	float m = pColor->Red ();

if (m < pColor->Green ())
	m = pColor->Green ();
if (m < pColor->Blue ())
	m = pColor->Blue ();
if (m > l)
	*pColor *= l / m;
}

//------------------------------------------------------------------------------

void AlphaBlend (CFloatVector& dest, CFloatVector& src, float fAlpha)
{
float da = 1.0f - fAlpha;
dest.v.color.r = dest.v.color.r * da + src.v.color.r * fAlpha;
dest.v.color.g = dest.v.color.g * da + src.v.color.g * fAlpha;
dest.v.color.b = dest.v.color.b * da + src.v.color.b * fAlpha;
//dest.v.color.a += other.v.color.a;
//if (dest.v.color.a > 1.0f)
//	dest.v.color.a = 1.0f;
}

//------------------------------------------------------------------------------

int32_t SetVertexColor (int32_t nVertex, CFaceColor *pColor, int32_t bBlend)
{
#if DBG
if (nVertex == nDbgVertex)
	BRP;
#endif
if (gameStates.render.bAmbientColor) { 
	if (bBlend == 1)
		*pColor *= gameData.renderData.color.ambient [nVertex];
	else if (bBlend == 2) {
		CFaceColor* pVertexColor = &gameData.renderData.color.ambient [nVertex];
		float a = pColor->v.color.a, da = 1.0f - a;
		pColor->v.color.r = pColor->v.color.r * a + pVertexColor->v.color.r * da;
		pColor->v.color.g = pColor->v.color.g * a + pVertexColor->v.color.g * da;
		pColor->v.color.b = pColor->v.color.b * a + pVertexColor->v.color.b * da;
		}
	else
		*pColor += gameData.renderData.color.ambient [nVertex];
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t SetVertexColors (tFaceProps *pProps)
{
if (SHOW_DYN_LIGHT) {
	// set material properties specific for certain textures here
	lightManager.SetMaterial (pProps->segNum, pProps->sideNum, -1);
	return 0;
	}
memset (vertColors, 0, sizeof (vertColors));
if (gameStates.render.bAmbientColor) {
	int32_t i, j = pProps->nVertices;
	for (i = 0; i < j; i++)
		SetVertexColor (pProps->vp [i], vertColors + i);
	}
else
	memset (vertColors, 0, sizeof (vertColors));
return 1;
}

//------------------------------------------------------------------------------

fix SetVertexLight (int32_t nSegment, int32_t nSide, int32_t nVertex, CFaceColor *pColor, fix light)
{
	fix				dynLight;
	float				fl, dl, hl;

//the tUVL struct has static light already in it
//scale static light for destruction effect
if (EGI_FLAG (bDarkness, 0, 0, 0))
	light = 0;
else {
#if LMAP_LIGHTADJUST
	if (USE_LIGHTMAPS) {
		else {
			light = I2X (1) / 2 + gameData.renderData.lights.segDeltas [nSegment * 6 + nSide];
			if (light < 0)
				light = 0;
			}
		}
#endif
	if (gameData.reactorData.bDestroyed || gameStates.gameplay.seismic.nMagnitude)	//make lights flash
		light = FixMul (gameStates.render.nFlashScale, light);
	}
//add in dynamic light (from explosions, etc.)
dynLight = gameData.renderData.lights.dynamicLight [nVertex];
fl = X2F (light);
dl = X2F (dynLight);
light += dynLight;
#if DBG
if (nVertex == nDbgVertex)
	BRP;
#endif
if (gameStates.app.bHaveExtraGameInfo [IsMultiGame]) {
	if (gameData.renderData.lights.bGotDynColor [nVertex]) {
#if DBG
		if (nVertex == nDbgVertex)
			BRP;
#endif
		CFloatVector dynColor;
		dynColor.Assign (gameData.renderData.lights.dynamicColor [nVertex]);
		if (gameOpts->render.color.bMix) {
			if (gameOpts->render.color.nLevel) {
				if (gameStates.render.bAmbientColor) {
					if ((fl != 0) && gameData.renderData.color.vertBright [nVertex]) {
						hl = fl / gameData.renderData.color.vertBright [nVertex];
						*pColor *= hl;
						*pColor += dynColor * dl;
						ScaleColor (pColor, fl + dl);
						}
					else {
						pColor->Assign (dynColor);
						*pColor *= dl;
						ScaleColor (pColor, dl);
						}
					}
				else {
					pColor->Set (fl, fl, fl);
					*pColor += dynColor * dl;
					ScaleColor (pColor, fl + dl);
					}
				}
			else {
				pColor->Red () =
				pColor->Green () =
				pColor->Blue () = fl + dl;
				}
			if (gameOpts->render.color.bCap) {
				if (pColor->Red () > 1.0)
					pColor->Red () = 1.0;
				if (pColor->Green () > 1.0)
					pColor->Green () = 1.0;
				if (pColor->Blue () > 1.0)
					pColor->Blue () = 1.0;
				}
			}
		else {
			float dl = X2F (light);
			dl = (float) pow (dl, 1.0f / 3.0f);
			pColor->Assign (dynColor);
			*pColor *= dl;
			}
		}
	else {
		ScaleColor (pColor, fl + dl);
		}
	}
else {
	ScaleColor (pColor, fl + dl);
	}
*pColor *= gameData.renderData.fBrightness;
light = fix (light * gameData.renderData.fBrightness);
//saturate at max value
if (light > MAX_LIGHT)
	light = MAX_LIGHT;
return light;
}

//------------------------------------------------------------------------------

int32_t SetFaceLight (tFaceProps *pProps)
{
if (SHOW_DYN_LIGHT)
	return 0;
for (int32_t i = 0; i < pProps->nVertices; i++) {
	pProps->uvls [i].l = SetVertexLight (pProps->segNum, pProps->sideNum, pProps->vp [i], vertColors + i, pProps->uvls [i].l);
	vertColors [i].index = -1;
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t IsTransparentTexture (int16_t nTexture)
{
return !gameStates.app.bD1Mission &&
		 ((nTexture == 378) ||
		  (nTexture == 353) ||
		  (nTexture == 420) ||
		  (nTexture == 432) ||
		  ((nTexture >= 399) && (nTexture <= 409)));
}

//------------------------------------------------------------------------------

float WallAlpha (int16_t nSegment, int16_t nSide, int16_t nWall, uint8_t widFlags, int32_t bIsMonitor, uint8_t bAdditive,
					  CFloatVector *pColor, int32_t& nColor, uint8_t& bTextured, uint8_t& bCloaked, uint8_t& bTransparent)
{
	static CFloatVector cloakColor = {{{0.0f, 0.0f, 0.0f, 0}}};

	CWall	*pWall;
	float fAlpha, fMaxColor;
	int16_t	c;

if (!IS_WALL (nWall))
	return 1;
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
#endif
if (!(pWall = WALL (nWall)))
	return 1;
if (SHOW_DYN_LIGHT) {
	bTransparent = (pWall->state == WALL_DOOR_CLOAKING) || (pWall->state == WALL_DOOR_DECLOAKING);
	bCloaked = !bTransparent && ((widFlags & WID_CLOAKED_FLAG) != 0);
	}
else {
	bTransparent = 0;
	bCloaked = (pWall->state == WALL_DOOR_CLOAKING) || (pWall->state == WALL_DOOR_DECLOAKING) || ((widFlags & WID_CLOAKED_FLAG) != 0);
	}
if (bCloaked || bTransparent || (widFlags & WID_TRANSPCOLOR_FLAG)) {
	if (bIsMonitor)
		return 1;
	c = pWall->cloakValue;
	if (bCloaked || bTransparent) {
		*pColor = cloakColor;
		nColor = 1;
		bTextured = !bCloaked;
		fAlpha = (c >= FADE_LEVELS) ? 0.0f : 1.0f - float (c) / float (FADE_LEVELS);
		if (bTransparent)
			pColor->Red () =
			pColor->Green () =
			pColor->Blue () = fAlpha;
		return fAlpha;
		}

	if (!gameOpts->render.color.bWalls)
		c = 0;
	if (WALL (nWall)->hps)
		fAlpha = (float) fabs ((1.0f - (float) WALL (nWall)->hps / ((float) I2X (100))));
	else if (IsMultiGame && gameStates.app.bHaveExtraGameInfo [1])
		fAlpha = COMPETITION ? 0.5f : (float) (FADE_LEVELS - extraGameInfo [1].grWallTransparency) / (float) FADE_LEVELS;
	else
		fAlpha = 1.0f - extraGameInfo [0].grWallTransparency / (float) FADE_LEVELS;
	if (fAlpha < 1.0f) {
		//fAlpha = (float) sqrt (fAlpha);
		paletteManager.Game ()->ToRgbaf ((uint8_t) c, *pColor);
		if (bAdditive) {
			pColor->Red () /= fAlpha;
			pColor->Green () /= fAlpha;
			pColor->Blue () /= fAlpha;
			}
		fMaxColor = pColor->Max ();
		if (fMaxColor > 1.0f) {
			pColor->Red () /= fMaxColor;
			pColor->Green () /= fMaxColor;
			pColor->Blue () /= fMaxColor;
			}
		bTextured = 0;
		nColor = 1;
		}
	return pColor->Alpha () = fAlpha;
	}
if (gameStates.app.bD2XLevel) {
	c = pWall->cloakValue;
	return pColor->Alpha () = (c && (c < FADE_LEVELS)) ? (float) (FADE_LEVELS - c) / (float) FADE_LEVELS : 1.0f;
	}
if (gameOpts->render.effects.bAutoTransparency && IsTransparentTexture (SEGMENT (nSegment)->m_sides [nSide].m_nBaseTex))
	return pColor->Alpha () = 0.8f;
return pColor->Alpha () = 1.0f;
}

//------------------------------------------------------------------------------

int32_t IsMonitorFace (int16_t nSegment, int16_t nSide, int32_t bForce)
{
return (bForce || gameStates.render.bDoCameras) ? cameraManager.GetFaceCamera (nSegment * 6 + nSide) : -1;
}

//------------------------------------------------------------------------------

int32_t SetupMonitorFace (int16_t nSegment, int16_t nSide, int16_t nCamera, CSegFace *pFace)
{
	CCamera		*pCamera = cameraManager [nCamera];

if (!pCamera) {
	pFace->m_info.nCamera = -1;
	return 0;
	}

	int32_t			bHaveMonitorBg, bIsTeleCam = pCamera->GetTeleport ();
#if !DBG
	int32_t			i;
#endif
#if RENDER2TEXTURE
	int32_t			bCamBufAvail = pCamera->HaveBuffer (1) == 1;
#else
	int32_t			bCamBufAvail = 0;
#endif

if (!gameStates.render.bDoCameras)
	return 0;
bHaveMonitorBg = pCamera->Valid () && /*!pCamera->bShadowMap &&*/
					  (pCamera->Texture () || bCamBufAvail) &&
					  (!bIsTeleCam || EGI_FLAG (bTeleporterCams, 0, 1, 0));
if (bHaveMonitorBg) {
	pCamera->Align (pFace, NULL, FACES.texCoord + pFace->m_info.nIndex, FACES.vertices + pFace->m_info.nIndex);
	if (bIsTeleCam) {
#if DBG
		pFace->bmBot = pCamera;
		gameStates.render.grAlpha = 1.0f;
#else
		pFace->bmTop = pCamera;
		for (i = 0; i < 4; i++)
			gameData.renderData.color.vertices [pFace->m_info.index [i]].Alpha () = 0.7f;
#endif
		}
	else if (/*gameOpts->render.cameras.bFitToWall ||*/ (pFace->m_info.nOvlTex == 0) || !pFace->bmBot)
		pFace->bmBot = pCamera;
	else
		pFace->bmTop = pCamera;
	pFace->pTexCoord = pCamera->TexCoord ();
	}
pFace->m_info.bTeleport = bIsTeleCam;
pCamera->SetVisible (1);
return bHaveMonitorBg || gameOpts->render.cameras.bFitToWall;
}

//------------------------------------------------------------------------------

void AdjustVertexColor (CBitmap *pBm, CFaceColor *pColor, fix xLight)
{
	float l = (pBm && (pBm->Flags () & BM_FLAG_NO_LIGHTING)) ? 1.0f : X2F (xLight);
	float s = 1.0f;

if (ogl.m_states.bScaleLight)
	s *= gameStates.render.bHeadlightOn ? 0.4f : 0.3f;
if (!pColor->index || !gameStates.render.bAmbientColor || (gameStates.app.bEndLevelSequence >= EL_OUTSIDE)) {
	pColor->Red () =
	pColor->Green () =
	pColor->Blue () = l * s;
	}
else if (s != 1.0f)
	*pColor *= s;
pColor->Alpha () = 1.0f;
}

// -----------------------------------------------------------------------------------
//Given a list of point numbers, rotate any that haven't been bRotated this frame
//cc.ccAnd and cc.ccOr will contain the position/orientation of the face that is determined
//by the vertices passed relative to the viewer

static inline CRenderPoint* TransformVertex (int32_t i)
{
CRenderPoint& p = RENDERPOINTS [i];
p.SetFlags (0);
p.TransformAndEncode (gameData.segData.vertices [i]);
if (!ogl.m_states.bUseTransform) 
	gameData.segData.fVertices [i].Assign (p.ViewPos ());
p.SetIndex (i);
return &p;
}

// -----------------------------------------------------------------------------------
//Given a list of point numbers, rotate any that haven't been bRotated this frame
//cc.ccAnd and cc.ccOr will contain the position/orientation of the face that is determined
//by the vertices passed relative to the viewer

tRenderCodes TransformVertexList (int32_t nVertices, uint16_t* pVertexIndex)
{
	tRenderCodes cc = {0, 0xff};

for (int32_t i = 0; i < nVertices; i++) {
	CRenderPoint* p = TransformVertex (pVertexIndex [i]);
	uint8_t c = p->Codes ();
	cc.ccAnd &= c;
	cc.ccOr |= c;
	}
return cc;
}

//------------------------------------------------------------------------------

void RotateSideNorms (void)
{
	int32_t			i, j;
	CSide			*pSide;

for (i = 0; i < gameData.segData.nSegments; i++)
	for (j = 6, pSide = SEGMENT (i)->m_sides; j; j--, pSide++) {
		transformation.Rotate (pSide->m_rotNorms [0], pSide->m_normals [0], 0);
		transformation.Rotate (pSide->m_rotNorms [1], pSide->m_normals [1], 0);
		}
}

//------------------------------------------------------------------------------

#if USE_SEGRADS

void TransformSideCenters (void)
{
	int32_t	i;

for (i = 0; i < gameData.segData.nSegments; i++)
	transformation.Transform (gameData.segData.segCenters [1] + i, gameData.segData.segCenters [0] + i, 0);
}

#endif

//------------------------------------------------------------------------------

uint8_t CVisibilityData::BumpVisitedFlag (void)
{
#if USE_OPENMP // > 1
#	pragma omp critical (CVisibilityDataBumpVisitedFlag)
#endif
	{
	if (!++nVisited) {
		bVisited.Clear (0);
		nVisited = 1;
		}
	}
return nVisited;
}

//------------------------------------------------------------------------------

uint8_t CVisibilityData::BumpProcessedFlag (void)
{
#if USE_OPENMP // > 1
#	pragma omp critical (CVisibilityDataBumpProcessedFlag)
#endif
	{
	if (!++nProcessed) {
		bProcessed.Clear (0);
		nProcessed = 1;
		}
	}
return nProcessed;
}

//------------------------------------------------------------------------------

uint8_t CVisibilityData::BumpVisibleFlag (void)
{
#if USE_OPENMP // > 1
#	pragma omp critical (CVisibilityDataBumpVisibleFlag)
#endif
	{
	if (!++nVisible) {
		bVisible.Clear (0);
		nVisible = 1;
		}
	}
return nVisible;
}

// ----------------------------------------------------------------------------

int32_t CVisibilityData::SegmentMayBeVisible (int16_t nStartSeg, int32_t nRadius, int32_t nMaxDist)
{
if (gameData.renderData.mine.Visible (nStartSeg))
	return 1;

	uint8_t*		visitedP = bVisited.Buffer ();
	int16_t*		pSegList = segments.Buffer ();

pSegList [0] = nStartSeg;
bProcessed [nStartSeg] = BumpProcessedFlag ();
bVisited [nStartSeg] = BumpVisitedFlag ();
if (nMaxDist < 0)
	nMaxDist = nRadius * I2X (20);
for (int32_t i = 0, j = 1; nRadius; nRadius--) {
	for (int32_t h = i, i = j; h < i; h++) {
		int32_t nSegment = pSegList [h];
		if ((bVisible [nSegment] == nVisible) &&
			 (!nMaxDist || (CFixVector::Dist (SEGMENT (nStartSeg)->Center (), SEGMENT (nSegment)->Center ()) <= nMaxDist)))
			return 1;
		CSegment* pSeg = SEGMENT (nSegment);
		for (int32_t nChild = 0; nChild < SEGMENT_SIDE_COUNT; nChild++) {
			int32_t nChildSeg = pSeg->m_children [nChild];
			if ((nChildSeg >= 0) && (visitedP [nChildSeg] != nVisited) && (pSeg->IsPassable (nChild, NULL) & WID_TRANSPARENT_FLAG)) {
				pSegList [j++] = nChildSeg;
				visitedP [nChildSeg] = nVisited;
				}
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int32_t SegmentMayBeVisible (int16_t nStartSeg, int32_t nRadius, int32_t nMaxDist, int32_t nThread)
{
return gameData.renderData.mine.visibility [nThread + 2].SegmentMayBeVisible (nStartSeg, nRadius, nMaxDist);
}

//------------------------------------------------------------------------------

int32_t SetRearView (int32_t bOn)
{
if (gameStates.render.bRearView == bOn)
	return 0;

if ((gameStates.render.bRearView = bOn)) {
	SetFreeCam (0);
	SetChaseCam (0);
	if (gameStates.render.cockpit.nType == CM_FULL_COCKPIT) {
		CGenericCockpit::Save ();
		cockpit->Activate (CM_REAR_VIEW);
		}
	if (gameData.demoData.nState == ND_STATE_RECORDING)
		NDRecordRearView ();

	}
else {
	if (gameStates.render.cockpit.nType == CM_REAR_VIEW)
		CGenericCockpit::Restore ();
	if (gameData.demoData.nState == ND_STATE_RECORDING)
		NDRecordRestoreRearView ();
	}
return 1;
}

//------------------------------------------------------------------------------

void CheckRearView (void)
{
#if DBG
if (controls [0].rearViewDownCount) {		//key/button has gone down
	controls [0].rearViewDownCount = 0;
	ToggleRearView ();
	}
#else
	#define LEAVE_TIME 0x1000		//how long until we decide key is down	 (Used to be 0x4000)

	static int32_t nLeaveMode;
	static fix entryTime;

if (controls [0].rearViewDownCount) {		//key/button has gone down
	controls [0].rearViewDownCount = 0;
	if (ToggleRearView () && gameStates.render.bRearView) {
		nLeaveMode = 0;		//means wait for another key
		entryTime = TimerGetFixedSeconds ();
		}
	}
else if (controls [0].rearViewDownState) {
	if (!nLeaveMode && (TimerGetFixedSeconds () - entryTime) > LEAVE_TIME)
		nLeaveMode = 1;
	}
else if (nLeaveMode)
	SetRearView (0);
#endif
}

//------------------------------------------------------------------------------

int32_t ToggleRearView (void)
{
return SetRearView (!gameStates.render.bRearView);
}

//------------------------------------------------------------------------------

void ResetRearView (void)
{
if (gameStates.render.bRearView) {
	if (gameData.demoData.nState == ND_STATE_RECORDING)
		NDRecordRestoreRearView ();
	}
gameStates.render.bRearView = 0;
if ((gameStates.render.cockpit.nType < 0) || (gameStates.render.cockpit.nType > 4) || (gameStates.render.cockpit.nType == CM_REAR_VIEW)) {
	if (!CGenericCockpit::Restore ())
		cockpit->Activate (CM_FULL_COCKPIT);
	}
}

//------------------------------------------------------------------------------

int32_t SetChaseCam (int32_t bOn)
{
if (gameStates.render.bChaseCam == bOn)
	return 0;
if ((gameStates.render.bChaseCam = bOn)) {
	SetFreeCam (0);
	SetRearView (0);
	CGenericCockpit::Save ();
	if (gameStates.render.cockpit.nType < CM_FULL_SCREEN)
		cockpit->Activate (CM_FULL_SCREEN);
	else
		gameStates.zoom.nFactor = float (gameStates.zoom.nMinFactor);
	}
else
	CGenericCockpit::Restore ();
if (LOCALPLAYER.ObservedPlayer () == N_LOCALPLAYER)
	FLIGHTPATH.Reset (-1, -1);
return 1;
}

//------------------------------------------------------------------------------

int32_t ToggleChaseCam (void)
{
#if !DBG
if (IsMultiGame && !IsCoopGame && (!EGI_FLAG (bEnableCheats, 0, 0, 0) || COMPETITION)) {
	HUDMessage (0, "Chase camera is not available");
	return 0;
	}
#endif
return 
SetChaseCam ((OBSERVING) ? 1 : !gameStates.render.bChaseCam);
}

//------------------------------------------------------------------------------

int32_t SetFreeCam (int32_t bOn)
{
if (gameStates.render.bFreeCam < 0)
	return 0;
if (OBSERVING)
	return 0;
if (gameStates.render.bFreeCam == bOn)
	return 0;
if ((gameStates.render.bFreeCam = bOn)) {
	SetChaseCam (0);
	SetRearView (0);
	gameStates.app.playerPos = gameData.objData.pViewer->info.position;
	gameStates.app.nPlayerSegment = gameData.objData.pViewer->info.nSegment;
	CGenericCockpit::Save ();
	if (gameStates.render.cockpit.nType < CM_FULL_SCREEN)
		cockpit->Activate (CM_FULL_SCREEN);
	else
		gameStates.zoom.nFactor = float (gameStates.zoom.nMinFactor);
	}
else {
	gameData.objData.pViewer->info.position = gameStates.app.playerPos;
	gameData.objData.pViewer->RelinkToSeg (gameStates.app.nPlayerSegment);
	CGenericCockpit::Restore ();
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t ToggleFreeCam (void)
{
#if !DBG
if (IsMultiGame && !IsCoopGame && (!EGI_FLAG (bEnableCheats, 0, 0, 0) || COMPETITION)) {
	HUDMessage (0, "Free camera is not available");
	return 0;
	}
#endif
return SetFreeCam (!gameStates.render.bFreeCam);
}

//------------------------------------------------------------------------------

void ToggleRadar (void)
{
gameOpts->render.cockpit.nRadarRange = (gameOpts->render.cockpit.nRadarRange + 1) % 5;
}

//------------------------------------------------------------------------------

extern kcItem kcMouse [];

inline int32_t ZoomKeyPressed (void)
{
#if 1
	int32_t	v;

return gameStates.input.keys.pressed [kcKeyboard [52].value] || gameStates.input.keys.pressed [kcKeyboard [53].value] ||
		 (((v = kcMouse [30].value) < 255) && MouseButtonState (v));
#else
return (controls [0].zoomDownCount > 0);
#endif
}

//------------------------------------------------------------------------------

void HandleZoom (void)
{
if (extraGameInfo [IsMultiGame].nZoomMode == 0)
	return;

	bool bAllow = (gameData.weaponData.nPrimary == VULCAN_INDEX) || (gameData.weaponData.nPrimary == GAUSS_INDEX);

if (extraGameInfo [IsMultiGame].nZoomMode == 1) {
	if (!gameStates.zoom.nState) {
		if (!bAllow || ZoomKeyPressed ()) {
			if (!bAllow) {
				if (gameStates.zoom.nFactor <= gameStates.zoom.nMinFactor)
					return;
				gameStates.zoom.nDestFactor = gameStates.zoom.nMinFactor;
				}
			else if (gameStates.zoom.nFactor >= gameStates.zoom.nMaxFactor)
				gameStates.zoom.nDestFactor = gameStates.zoom.nMinFactor;
			else
				gameStates.zoom.nDestFactor = (fix) FRound (gameStates.zoom.nFactor * pow (float (gameStates.zoom.nMaxFactor) / float (gameStates.zoom.nMinFactor), 0.25f));
			gameStates.zoom.nStep = float (gameStates.zoom.nDestFactor - gameStates.zoom.nFactor) / 6.25f;
			gameStates.zoom.nTime = gameStates.app.nSDLTicks [0] - 40;
			gameStates.zoom.nState = (gameStates.zoom.nStep > 0) ? 1 : -1;
			if (audio.ChannelIsPlaying (gameStates.zoom.nChannel))
				audio.StopSound (gameStates.zoom.nChannel);
			gameStates.zoom.nChannel = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xFFFF / 2, 0, 0, 0, -1, I2X (1), AddonSoundName (SND_ADDON_ZOOM1));
			}
		}
	}
else if (extraGameInfo [IsMultiGame].nZoomMode == 2) {
	if (bAllow && ZoomKeyPressed ()) {
		if ((gameStates.zoom.nState <= 0) && (gameStates.zoom.nFactor < gameStates.zoom.nMaxFactor)) {
			if (audio.ChannelIsPlaying (gameStates.zoom.nChannel))
				audio.StopSound (gameStates.zoom.nChannel);
			gameStates.zoom.nChannel = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xFFFF / 2, 0, 0, 0, -1, I2X (1), AddonSoundName (SND_ADDON_ZOOM2));
			gameStates.zoom.nDestFactor = gameStates.zoom.nMaxFactor;
			gameStates.zoom.nStep = float (gameStates.zoom.nDestFactor - gameStates.zoom.nFactor) / 25.0f;
			gameStates.zoom.nTime = gameStates.app.nSDLTicks [0] - 40;
			gameStates.zoom.nState = 1;
			}
		}
	else if ((gameStates.zoom.nState >= 0) && (gameStates.zoom.nFactor > gameStates.zoom.nMinFactor)) {
		if (audio.ChannelIsPlaying (gameStates.zoom.nChannel))
			audio.StopSound (gameStates.zoom.nChannel);
		gameStates.zoom.nChannel = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xFFFF / 2, 0, 0, 0, -1, I2X (1), AddonSoundName (SND_ADDON_ZOOM2));
		gameStates.zoom.nDestFactor = gameStates.zoom.nMinFactor;
		gameStates.zoom.nStep = float (gameStates.zoom.nDestFactor - gameStates.zoom.nFactor) / 25.0f;
		gameStates.zoom.nTime = gameStates.app.nSDLTicks [0] - 40;
		gameStates.zoom.nState = -1;
		}
	}
if (!gameStates.zoom.nState)
	gameStates.zoom.nChannel = -1;
else if (gameStates.app.nSDLTicks [0] - gameStates.zoom.nTime >= 40) {
	gameStates.zoom.nTime += 40;
	gameStates.zoom.nFactor += gameStates.zoom.nStep;
	if (((gameStates.zoom.nState > 0) && (gameStates.zoom.nFactor > gameStates.zoom.nDestFactor)) || 
		 ((gameStates.zoom.nState < 0) && (gameStates.zoom.nFactor < gameStates.zoom.nDestFactor))) {
		gameStates.zoom.nFactor = float (gameStates.zoom.nDestFactor);
		gameStates.zoom.nState = 0;
		gameStates.zoom.nChannel = -1;
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CFrameController::CFrameController () : m_nFrames (0), m_iFrame (0), m_nEye (0), m_nOffsetSave (9)
{
}

//------------------------------------------------------------------------------

void CFrameController::Begin (void)
{
m_nFrames = ogl.IsSideBySideDevice () ? 2 : 1;
m_iFrame = 0;
m_nEye = -1;
}

//------------------------------------------------------------------------------

bool CFrameController::Continue (void)
{
if (m_iFrame >= m_nFrames)
	return false;
Setup ();
return true;
}

//------------------------------------------------------------------------------

void CFrameController::End (void)
{
++m_iFrame;
m_nEye += 2;
gameData.SetStereoOffsetType (m_nOffsetSave);
}

//------------------------------------------------------------------------------

void CFrameController::Setup (void)
{
m_nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
gameData.SetStereoSeparation (m_nEye);
SetupCanvasses ();
ogl.ChooseDrawBuffer ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// eof
