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

//------------------------------------------------------------------------------

#define SW_CULLING 1

//------------------------------------------------------------------------------

int	bOutLineMode = 0,
		bShowOnlyCurSide = 0;

//------------------------------------------------------------------------------

int FaceIsVisible (short nSegment, short nSide)
{
#if SW_CULLING
CSegment *segP = SEGMENTS + nSegment;
CSide *sideP = segP->m_sides + nSide;
CFixVector v;
if (sideP->m_nType == SIDE_IS_QUAD) {
	v = gameData.render.mine.viewer.vPos - segP->SideCenter (nSide); //gameData.segs.vertices + segP->m_vertices [sideVertIndex [nSide][0]]);
	return CFixVector::Dot (sideP->m_normals [0], v) >= 0;
	}
v = gameData.render.mine.viewer.vPos - VERTICES [sideP->m_vertices [(sideP->m_nType == SIDE_IS_TRI_02) ? 0 : 3]];
return (CFixVector::Dot (sideP->m_normals [0], v) >= 0) || (CFixVector::Dot (sideP->m_normals [1], v) >= 0);
#else
return 1;
#endif
}

//------------------------------------------------------------------------------

void RotateTexCoord2f (tTexCoord2f& dest, tTexCoord2f& src, ubyte nOrient)
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

int ToggleOutlineMode (void)
{
return bOutLineMode = !bOutLineMode;
}

//------------------------------------------------------------------------------

int ToggleShowOnlyCurSide (void)
{
return bShowOnlyCurSide = !bShowOnlyCurSide;
}

//------------------------------------------------------------------------------

void DrawOutline (int nVertices, CRenderPoint **pointList)
{
	GLint				depthFunc;
	CRenderPoint	center, normal;
	CFixVector		n;

#if 1 //!DBG
if (gameStates.render.bQueryOcclusion) {
	CFloatVector outlineColor = {1, 1, 0, -1};
	G3DrawPolyAlpha (nVertices, pointList, &outlineColor, 1, -1);
	return;
	}
#endif

glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
ogl.SetDepthMode (GL_ALWAYS);
CCanvas::Current ()->SetColorRGB (255, 255, 255, 255);
center.ViewPos ().SetZero ();
for (int i = 0; i < nVertices; i++) {
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

char IsColoredSeg (short nSegment)
{
if (nSegment < 0)
	return 0;
//if (!gameStates.render.nLightingMethod)
//	return 0;
CSegment* segP = SEGMENTS + nSegment;
if (IsEntropyGame && (extraGameInfo [1].entropy.nOverrideTextures == 2) && (segP->m_owner > 0))
	return (segP->m_owner == 1) ? 2 : 1;
if (segP->HasWaterProp ())
	return 3;
if (segP->HasLavaProp ())
	return 4;
#if 0
if (segP->m_function == SEGMENT_FUNC_TEAM_BLUE) 
	return 1;
if (segP->m_function == SEGMENT_FUNC_TEAM_RED)
	return 2;
#endif
return 0;
}

// ----------------------------------------------------------------------------

char IsColoredSegFace (short nSegment, short nSide)
{
#if 0 //!DBG
if (!gameStates.render.nLightingMethod)
	return 0;
#endif

	CSegment*	segP = SEGMENTS + nSegment;
	CSegment*	connSegP = (segP->m_children [nSide] < 0) ? NULL : SEGMENTS + segP->m_children [nSide];

if (IsEntropyGame && (extraGameInfo [1].entropy.nOverrideTextures == 2) && (segP->m_owner > 0)) {
	if (!connSegP || (connSegP->m_owner != segP->m_owner))
		return (segP->m_owner == 1) ? 2 : 1;
	}

if (!connSegP) {
	if (segP->HasWaterProp ())
		return 3;
	if (segP->HasLavaProp ())
		return 4;
	return 0;
	}
if (segP->HasWaterProp () != connSegP->HasWaterProp ())
	return 3;
if (segP->HasLavaProp () != connSegP->HasLavaProp ())
	return 4;
#if 1
return 0;
#else
if (segP->m_function == connSegP->m_function)
	return 0;
return (segP->m_function == SEGMENT_FUNC_TEAM_BLUE) || 
		 (segP->m_function == SEGMENT_FUNC_TEAM_RED) || 
		 (connSegP->m_function == SEGMENT_FUNC_TEAM_BLUE) || 
		 (connSegP->m_function == SEGMENT_FUNC_TEAM_RED);
#endif
}

// ----------------------------------------------------------------------------

CFloatVector segmentColors [4] = {
	 {0.5f, 0, 0, 0.333f},
	 {0, 0, 0.5f, 0.333f},
	 {0, 1.0f / 16.0f, 0.5f, 0.333f},
	 {0.5f, 0, 0, 0.333f}};

CFloatVector *ColoredSegmentColor (int nSegment, int nSide, char nColor)
{
//if (!gameStates.render.nLightingMethod)
//	return NULL;

	CSegment*	segP = SEGMENTS + nSegment;
	CSegment*	connSegP = (segP->m_children [nSide] < 0) ? NULL : SEGMENTS + segP->m_children [nSide];

#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

if (nColor > 0)
	nColor--;
else {
	if (IsEntropyGame && (extraGameInfo [1].entropy.nOverrideTextures == 2) && (segP->m_owner > 0)) {
		if (connSegP && (connSegP->m_owner == segP->m_owner))
			return NULL;
		nColor = (segP->m_owner == 1);
		}
	if (segP->HasWaterProp ())
		nColor = 2;
	else if (segP->HasLavaProp ())
		nColor = 3;
	else
		return NULL;
	if (connSegP >= 0) {
		if (segP->HasWaterProp () == connSegP->HasWaterProp ())
			return NULL;
		if (segP->HasLavaProp () == connSegP->HasLavaProp ())
			return NULL;
#if 1
		if (segP->m_function == connSegP->m_function)
			return NULL;
		if ((segP->m_function != SEGMENT_FUNC_TEAM_BLUE) &&
			 (segP->m_function != SEGMENT_FUNC_TEAM_RED) &&
			 (connSegP->m_function != SEGMENT_FUNC_TEAM_BLUE) &&
			 (connSegP->m_function != SEGMENT_FUNC_TEAM_RED))
			return NULL;
		if (IS_WALL (segP->WallNum (nSide)))
#endif
			return NULL;
		}
	}
return segmentColors + nColor;
}

//------------------------------------------------------------------------------
// If any color component > 1, scale all components down so that the greatest == 1.

static inline void ScaleColor (CFaceColor *colorP, float l)
{
	float m = colorP->Red ();

if (m < colorP->Green ())
	m = colorP->Green ();
if (m < colorP->Blue ())
	m = colorP->Blue ();
if (m > l)
	*colorP *= l / m;
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

int SetVertexColor (int nVertex, CFaceColor *colorP, int bBlend)
{
#if DBG
if (nVertex == nDbgVertex)
	nVertex = nVertex;
#endif
if (gameStates.render.bAmbientColor) { 
	if (bBlend == 1)
		*colorP *= gameData.render.color.ambient [nVertex];
	else if (bBlend == 2) {
		CFaceColor* vertColorP = &gameData.render.color.ambient [nVertex];
		float a = colorP->v.color.a, da = 1.0f - a;
		colorP->v.color.r = colorP->v.color.r * a + vertColorP->v.color.r * da;
		colorP->v.color.g = colorP->v.color.g * a + vertColorP->v.color.g * da;
		colorP->v.color.b = colorP->v.color.b * a + vertColorP->v.color.b * da;
		}
	else
		*colorP += gameData.render.color.ambient [nVertex];
	}
return 1;
}

//------------------------------------------------------------------------------

int SetVertexColors (tFaceProps *propsP)
{
if (SHOW_DYN_LIGHT) {
	// set material properties specific for certain textures here
	lightManager.SetMaterial (propsP->segNum, propsP->sideNum, -1);
	return 0;
	}
memset (vertColors, 0, sizeof (vertColors));
if (gameStates.render.bAmbientColor) {
	int i, j = propsP->nVertices;
	for (i = 0; i < j; i++)
		SetVertexColor (propsP->vp [i], vertColors + i);
	}
else
	memset (vertColors, 0, sizeof (vertColors));
return 1;
}

//------------------------------------------------------------------------------

fix SetVertexLight (int nSegment, int nSide, int nVertex, CFaceColor *colorP, fix light)
{
	CFloatVector	dynColor;
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
			light = I2X (1) / 2 + gameData.render.lights.segDeltas [nSegment * 6 + nSide];
			if (light < 0)
				light = 0;
			}
		}
#endif
	if (gameData.reactor.bDestroyed || gameStates.gameplay.seismic.nMagnitude)	//make lights flash
		light = FixMul (gameStates.render.nFlashScale, light);
	}
//add in dynamic light (from explosions, etc.)
dynLight = gameData.render.lights.dynamicLight [nVertex];
fl = X2F (light);
dl = X2F (dynLight);
light += dynLight;
#if DBG
if (nVertex == nDbgVertex)
	nVertex = nVertex;
#endif
if (gameStates.app.bHaveExtraGameInfo [IsMultiGame]) {
	if (gameData.render.lights.bGotDynColor [nVertex]) {
		dynColor.Assign (gameData.render.lights.dynamicColor [nVertex]);
		if (gameOpts->render.color.bMix) {
			if (gameOpts->render.color.nLevel) {
				if (gameStates.render.bAmbientColor) {
					if ((fl != 0) && gameData.render.color.vertBright [nVertex]) {
						hl = fl / gameData.render.color.vertBright [nVertex];
						*colorP *= hl;
						*colorP += dynColor * dl;
						ScaleColor (colorP, fl + dl);
						}
					else {
						colorP->Assign (dynColor);
						*colorP *= dl;
						ScaleColor (colorP, dl);
						}
					}
				else {
					colorP->Set (fl, fl, fl);
					*colorP += dynColor * dl;
					ScaleColor (colorP, fl + dl);
					}
				}
			else {
				colorP->Red () =
				colorP->Green () =
				colorP->Blue () = fl + dl;
				}
			if (gameOpts->render.color.bCap) {
				if (colorP->Red () > 1.0)
					colorP->Red () = 1.0;
				if (colorP->Green () > 1.0)
					colorP->Green () = 1.0;
				if (colorP->Blue () > 1.0)
					colorP->Blue () = 1.0;
				}
			}
		else {
			float dl = X2F (light);
			dl = (float) pow (dl, 1.0f / 3.0f);
			colorP->Assign (dynColor);
			*colorP *= dl;
			}
		}
	else {
		ScaleColor (colorP, fl + dl);
		}
	}
else {
	ScaleColor (colorP, fl + dl);
	}
*colorP *= gameData.render.fBrightness;
light = fix (light * gameData.render.fBrightness);
//saturate at max value
if (light > MAX_LIGHT)
	light = MAX_LIGHT;
return light;
}

//------------------------------------------------------------------------------

int SetFaceLight (tFaceProps *propsP)
{
	int	i;

if (SHOW_DYN_LIGHT)
	return 0;
for (i = 0; i < propsP->nVertices; i++) {
	propsP->uvls [i].l = SetVertexLight (propsP->segNum, propsP->sideNum, propsP->vp [i], vertColors + i, propsP->uvls [i].l);
	vertColors [i].index = -1;
	}
return 1;
}

//------------------------------------------------------------------------------

int IsTransparentTexture (short nTexture)
{
return !gameStates.app.bD1Mission &&
		 ((nTexture == 378) ||
		  (nTexture == 353) ||
		  (nTexture == 420) ||
		  (nTexture == 432) ||
		  ((nTexture >= 399) && (nTexture <= 409)));
}

//------------------------------------------------------------------------------

float WallAlpha (short nSegment, short nSide, short nWall, ubyte widFlags, int bIsMonitor, ubyte bAdditive,
					  CFloatVector *colorP, int& nColor, ubyte& bTextured, ubyte& bCloaked, ubyte& bTransparent)
{
	static CFloatVector cloakColor = {0.0f, 0.0f, 0.0f, 0};

	CWall	*wallP;
	float fAlpha, fMaxColor;
	short	c;

if (!IS_WALL (nWall))
	return 1;
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if (!(wallP = WALLS + nWall))
	return 1;
if (SHOW_DYN_LIGHT) {
	bTransparent = (wallP->state == WALL_DOOR_CLOAKING) || (wallP->state == WALL_DOOR_DECLOAKING);
	bCloaked = !bTransparent && ((widFlags & WID_CLOAKED_FLAG) != 0);
	}
else {
	bTransparent = 0;
	bCloaked = (wallP->state == WALL_DOOR_CLOAKING) || (wallP->state == WALL_DOOR_DECLOAKING) || ((widFlags & WID_CLOAKED_FLAG) != 0);
	}
if (bCloaked || bTransparent || (widFlags & WID_TRANSPCOLOR_FLAG)) {
	if (bIsMonitor)
		return 1;
	c = wallP->cloakValue;
	if (bCloaked || bTransparent) {
		*colorP = cloakColor;
		nColor = 1;
		bTextured = !bCloaked;
		fAlpha = (c >= FADE_LEVELS) ? 0.0f : 1.0f - float (c) / float (FADE_LEVELS);
		if (bTransparent)
			colorP->Red () =
			colorP->Green () =
			colorP->Blue () = fAlpha;
		return fAlpha;
		}

	if (!gameOpts->render.color.bWalls)
		c = 0;
	if (WALLS [nWall].hps)
		fAlpha = (float) fabs ((1.0f - (float) WALLS [nWall].hps / ((float) I2X (100))));
	else if (IsMultiGame && gameStates.app.bHaveExtraGameInfo [1])
		fAlpha = COMPETITION ? 0.5f : (float) (FADE_LEVELS - extraGameInfo [1].grWallTransparency) / (float) FADE_LEVELS;
	else
		fAlpha = 1.0f - extraGameInfo [0].grWallTransparency / (float) FADE_LEVELS;
	if (fAlpha < 1.0f) {
		//fAlpha = (float) sqrt (fAlpha);
		paletteManager.Game ()->ToRgbaf ((ubyte) c, *colorP);
		if (bAdditive) {
			colorP->Red () /= fAlpha;
			colorP->Green () /= fAlpha;
			colorP->Blue () /= fAlpha;
			}
		fMaxColor = colorP->Max ();
		if (fMaxColor > 1.0f) {
			colorP->Red () /= fMaxColor;
			colorP->Green () /= fMaxColor;
			colorP->Blue () /= fMaxColor;
			}
		bTextured = 0;
		nColor = 1;
		}
	return colorP->Alpha () = fAlpha;
	}
if (gameStates.app.bD2XLevel) {
	c = wallP->cloakValue;
	return colorP->Alpha () = (c && (c < FADE_LEVELS)) ? (float) (FADE_LEVELS - c) / (float) FADE_LEVELS : 1.0f;
	}
if (gameOpts->render.effects.bAutoTransparency && IsTransparentTexture (SEGMENTS [nSegment].m_sides [nSide].m_nBaseTex))
	return colorP->Alpha () = 0.8f;
return colorP->Alpha () = 1.0f;
}

//------------------------------------------------------------------------------

int IsMonitorFace (short nSegment, short nSide, int bForce)
{
return (bForce || gameStates.render.bDoCameras) ? cameraManager.GetFaceCamera (nSegment * 6 + nSide) : -1;
}

//------------------------------------------------------------------------------

int SetupMonitorFace (short nSegment, short nSide, short nCamera, CSegFace *faceP)
{
	CCamera		*cameraP = cameraManager [nCamera];

if (!cameraP) {
	faceP->m_info.nCamera = -1;
	return 0;
	}

	int			bHaveMonitorBg, bIsTeleCam = cameraP->GetTeleport ();
#if !DBG
	int			i;
#endif
#if RENDER2TEXTURE
	int			bCamBufAvail = cameraP->HaveBuffer (1) == 1;
#else
	int			bCamBufAvail = 0;
#endif

if (!gameStates.render.bDoCameras)
	return 0;
bHaveMonitorBg = cameraP->Valid () && /*!cameraP->bShadowMap &&*/
					  (cameraP->Texture () || bCamBufAvail) &&
					  (!bIsTeleCam || EGI_FLAG (bTeleporterCams, 0, 1, 0));
if (bHaveMonitorBg) {
	cameraP->Align (faceP, NULL, FACES.texCoord + faceP->m_info.nIndex, FACES.vertices + faceP->m_info.nIndex);
	if (bIsTeleCam) {
#if DBG
		faceP->bmBot = cameraP;
		gameStates.render.grAlpha = 1.0f;
#else
		faceP->bmTop = cameraP;
		for (i = 0; i < 4; i++)
			gameData.render.color.vertices [faceP->m_info.index [i]].Alpha () = 0.7f;
#endif
		}
	else if (/*gameOpts->render.cameras.bFitToWall ||*/ (faceP->m_info.nOvlTex == 0) || !faceP->bmBot)
		faceP->bmBot = cameraP;
	else
		faceP->bmTop = cameraP;
	faceP->texCoordP = cameraP->TexCoord ();
	}
faceP->m_info.bTeleport = bIsTeleCam;
cameraP->SetVisible (1);
return bHaveMonitorBg || gameOpts->render.cameras.bFitToWall;
}

//------------------------------------------------------------------------------

void AdjustVertexColor (CBitmap *bmP, CFaceColor *colorP, fix xLight)
{
	float l = (bmP && (bmP->Flags () & BM_FLAG_NO_LIGHTING)) ? 1.0f : X2F (xLight);
	float s = 1.0f;

if (ogl.m_states.bScaleLight)
	s *= gameStates.render.bHeadlightOn ? 0.4f : 0.3f;
if (!colorP->index || !gameStates.render.bAmbientColor || (gameStates.app.bEndLevelSequence >= EL_OUTSIDE)) {
	colorP->Red () =
	colorP->Green () =
	colorP->Blue () = l * s;
	}
else if (s != 1.0f)
	*colorP *= s;
colorP->Alpha () = 1.0f;
}

// -----------------------------------------------------------------------------------
//Given a list of point numbers, rotate any that haven't been bRotated this frame
//cc.ccAnd and cc.ccOr will contain the position/orientation of the face that is determined
//by the vertices passed relative to the viewer

CRenderPoint *RotateVertex (int i)
{
CRenderPoint& p = gameData.segs.points [i];
#if !DBG
if (gameData.render.mine.nRotatedLast [i] != gameStates.render.nFrameCount) 
#endif
	{
#if DBG
	if (i == nDbgVertex)
		nDbgVertex = nDbgVertex;
#endif
	p.TransformAndEncode (gameData.segs.vertices [i]);
#if TRANSP_DEPTH_HASH
	fix d = p.ViewPos ().Mag ();
	if (gameData.render.zMin > d)
		gameData.render.zMin = d;
	if (gameData.render.zMax < d)
		gameData.render.zMax = d;
#else
	if (gameData.render.zMax < p->m_vertex [1].dir.coord.z)
		gameData.render.zMax = p->m_vertex [1].dir.coord.z;
#endif
	if (!ogl.m_states.bUseTransform) {
		gameData.segs.fVertices [i].Assign (p.ViewPos ());
		}
	p.SetIndex (i);
	gameData.render.mine.nRotatedLast [i] = gameStates.render.nFrameCount;
	}
return &p;
}

// -----------------------------------------------------------------------------------
//Given a list of point numbers, rotate any that haven't been bRotated this frame
//cc.ccAnd and cc.ccOr will contain the position/orientation of the face that is determined
//by the vertices passed relative to the viewer

tRenderCodes RotateVertexList (int nVertices, ushort* vertexIndexP)
{
	tRenderCodes cc = {0, 0xff};

for (int i = 0; i < nVertices; i++) {
	CRenderPoint* p = RotateVertex (vertexIndexP [i]);
	ubyte c = p->Codes ();
	cc.ccAnd &= c;
	cc.ccOr |= c;
	}
return cc;
}

// -----------------------------------------------------------------------------------
//Given a lit of point numbers, project any that haven't been projected
void ProjectVertexList (int nVertices, ushort *vertexIndexP)
{
for (int i = 0; i < nVertices; i++) {
	int j = vertexIndexP [i];
	if (!gameData.segs.points [j].Projected ())
		gameData.segs.points [j].Project ();
	}
}

//------------------------------------------------------------------------------

ubyte ProjectRenderPoint (int nVertex)
{
#if DBG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
CRenderPoint& point = gameData.segs.points [nVertex];
#if 0 //DBG
point.m_flags = 0;
#else
if (!point.Projected ()) 
#endif
	{
#if 0
	point.Transform (point.m_vertex [1], point.m_vertex [0] = gameData.segs.vertices [nVertex]);
	G3ProjectPoint (&point);
	point.m_codes = (point.m_vertex [1].v.coord.z < 0) ? CC_BEHIND : 0;
#else
	CFloatVector3 v;
	point.AddFlags (PF_PROJECTED);
#if 0
	transformation.Transform (point.m_vertex [1], point.m_vertex [0] = gameData.segs.vertices [nVertex]);
	v.Assign (point.m_vertex [1]);
#else
	point.WorldPos () = gameData.segs.vertices [nVertex];
	transformation.Transform (v, *((CFloatVector3*) &gameData.segs.fVertices [nVertex]));
	point.ViewPos ().Assign (v);
#endif
	point.SetCodes ((v.v.coord.z < 0.0f) ? CC_BEHIND : 0);
	ProjectPoint (v, point.Screen ());
#endif
	point.Encode ();
#if TRANSP_DEPTH_HASH
	fix d = point.ViewPos ().Mag ();
	if (gameData.render.zMin > d)
		gameData.render.zMin = d;
	if (gameData.render.zMax < d)
		gameData.render.zMax = d;
#else
	if (gameData.render.zMax < point.m_vertex [1].dir.coord.z)
		gameData.render.zMax = point.m_vertex [1].dir.coord.z;
#endif
	}
return point.Codes ();
}

//------------------------------------------------------------------------------

void RotateSideNorms (void)
{
	int			i, j;
	CSide			*sideP;

for (i = 0; i < gameData.segs.nSegments; i++)
	for (j = 6, sideP = SEGMENTS [i].m_sides; j; j--, sideP++) {
		transformation.Rotate (sideP->m_rotNorms [0], sideP->m_normals [0], 0);
		transformation.Rotate (sideP->m_rotNorms [1], sideP->m_normals [1], 0);
		}
}

//------------------------------------------------------------------------------

#if USE_SEGRADS

void TransformSideCenters (void)
{
	int	i;

for (i = 0; i < gameData.segs.nSegments; i++)
	transformation.Transform (gameData.segs.segCenters [1] + i, gameData.segs.segCenters [0] + i, 0);
}

#endif

//------------------------------------------------------------------------------

ubyte CVisibilityData::BumpVisitedFlag (void)
{
#if USE_OPENMP // > 1
#	pragma omp critical
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

ubyte CVisibilityData::BumpProcessedFlag (void)
{
#if USE_OPENMP // > 1
#	pragma omp critical
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

ubyte CVisibilityData::BumpVisibleFlag (void)
{
#if USE_OPENMP // > 1
#	pragma omp critical
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

int CVisibilityData::SegmentMayBeVisible (short nStartSeg, short nRadius, int nMaxDist)
{
if (gameData.render.mine.bVisible [nStartSeg] == gameData.render.mine.nVisible)
	return 1;

	ubyte*		visitedP = bVisited.Buffer ();
	short*		segListP = segments.Buffer ();

segListP [0] = nStartSeg;
bProcessed [nStartSeg] = BumpProcessedFlag ();
bVisited [nStartSeg] = BumpVisitedFlag ();
if (nMaxDist < 0)
	nMaxDist = nRadius * I2X (20);
for (int i = 0, j = 1; nRadius; nRadius--) {
	for (int h = i, i = j; h < i; h++) {
		int nSegment = segListP [h];
		if ((bVisible [nSegment] == nVisible) &&
			 (!nMaxDist || (CFixVector::Dist (SEGMENTS [nStartSeg].Center (), SEGMENTS [nSegment].Center ()) <= nMaxDist)))
			return 1;
		CSegment* segP = SEGMENTS + nSegment;
		for (int nChild = 0; nChild < 6; nChild++) {
			int nChildSeg = segP->m_children [nChild];
			if ((nChildSeg >= 0) && (visitedP [nChildSeg] != nVisited) && (segP->IsDoorWay (nChild, NULL) & WID_TRANSPARENT_FLAG)) {
				segListP [j++] = nChildSeg;
				visitedP [nChildSeg] = nVisited;
				}
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int SegmentMayBeVisible (short nStartSeg, short nRadius, int nMaxDist, int nThread = 0)
{
return gameData.render.mine.visibility [nThread].SegmentMayBeVisible (nStartSeg, nRadius, nMaxDist);
}

//------------------------------------------------------------------------------
// eof
