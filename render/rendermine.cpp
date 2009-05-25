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
#include "error.h"
#include "gamefont.h"
#include "texmap.h"
#include "rendermine.h"
#include "fastrender.h"
#include "rendershadows.h"
#include "transprender.h"
#include "renderthreads.h"
#include "glare.h"
#include "radar.h"
#include "objrender.h"
#include "textures.h"
#include "screens.h"
#include "segpoint.h"
#include "texmerge.h"
#include "physics.h"
#include "gameseg.h"
#include "light.h"
#include "dynlight.h"
#include "headlight.h"
#include "newdemo.h"
#include "automap.h"
#include "key.h"
#include "u_mem.h"
#include "kconfig.h"
#include "mouse.h"
#include "interp.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_render.h"
#include "ogl_fastrender.h"
#include "cockpit.h"
#include "input.h"
#include "shadows.h"
#include "textdata.h"
#include "sparkeffect.h"
#include "createmesh.h"
#include "systemkeys.h"

//------------------------------------------------------------------------------

#if LIGHTMAPS
#	define LMAP_LIGHTADJUST	1
#else
#	define LMAP_LIGHTADJUST	0
#endif

#define bPreDrawSegs			0

#define OLD_SEGLIST			1
#define SORT_RENDER_SEGS	0

// ------------------------------------------------------------------------------

void StartLightingFrame (CObject *viewer);

uint	nClearWindowColor = 0;
int	nClearWindow = 0; //2	// 1 = Clear whole background tPortal, 2 = clear view portals into rest of world, 0 = no clear

#define CLEAR_WINDOW	0

void RenderSkyBox (int nWindow);

//------------------------------------------------------------------------------

extern int bLog;

CCanvas *reticleCanvas = NULL;

void _CDECL_ FreeReticleCanvas (void)
{
if (reticleCanvas) {
	PrintLog ("unloading reticle data\n");
	reticleCanvas->Destroy ();
	reticleCanvas = NULL;
	}
}

//------------------------------------------------------------------------------

#if 0

// Draw the reticle in 3D for head tracking
void Draw3DReticle (fix nEyeOffset)
{
	g3sPoint 	reticlePoints [4];
	tUVL			tUVL [4];
	g3sPoint		*pointList [4];
	int 			i;
	CFixVector	v1, v2;
	int			saved_interp_method;

//	if (!transformation.m_info.bUsePlayerHeadAngles) return;

for (i = 0; i < 4; i++) {
	reticlePoints [i].p3_index = -1;
	pointList [i] = reticlePoints + i;
	tUVL [i].l = MAX_LIGHT;
	}
tUVL [0].u =
tUVL [0].v =
tUVL [1].v =
tUVL [3].u = 0;
tUVL [1].u =
tUVL [2].u =
tUVL [2].v =
tUVL [3].v = I2X (1);

v1 = gameData.objs.viewerP->info.position.vPos + gameData.objs.viewerP->info.position.mOrient.FVec() * (I2X (4));
v1 += gameData.objs.viewerP->info.position.mOrient.RVec() * nEyeOffset;
v2 = v1 + gameData.objs.viewerP->info.position.mOrient.RVec() * (-I2X (1));
v2 += gameData.objs.viewerP->info.position.mOrient.UVec() * (I2X (1));
G3TransformAndEncodePoint(&reticlePoints [0], v2);
v2 = v1 + gameData.objs.viewerP->info.position.mOrient.RVec() * (+I2X (1));
v2 += gameData.objs.viewerP->info.position.mOrient.UVec() * (I2X (1));
G3TransformAndEncodePoint(&reticlePoints [1], v2);
v2 = v1 + gameData.objs.viewerP->info.position.mOrient.RVec() * (+I2X (1));
v2 += gameData.objs.viewerP->info.position.mOrient.UVec() * (-I2X (1));
G3TransformAndEncodePoint(&reticlePoints [2], v2);
v2 = v1 + gameData.objs.viewerP->info.position.mOrient.RVec() * (-I2X (1));
v2 += gameData.objs.viewerP->info.position.mOrient.UVec() * (-I2X (1));
G3TransformAndEncodePoint(&reticlePoints [3], v2);

if ( reticleCanvas == NULL) {
	reticleCanvas = CCanvas::Create (64, 64);
	if ( !reticleCanvas)
		Error ("Couldn't allocate reticleCanvas");
	//reticleCanvas->Bitmap ().nId = 0;
	reticleCanvas->SetFlags (BM_FLAG_TRANSPARENT);
	}

CCanvas::Push ();
CCanvas::SetCurrent (reticleCanvas);
reticleCanvas->Clear (0);		// Clear to Xparent
cockpit->DrawReticle (1);
CCanvas::Pop ();

saved_interp_method = gameStates.render.nInterpolationMethod;
gameStates.render.nInterpolationMethod	= 3;		// The best, albiet slowest.
G3DrawTexPoly (4, pointList, tUVL, reticleCanvas, NULL, 1, -1);
gameStates.render.nInterpolationMethod	= saved_interp_method;
}

#endif

//------------------------------------------------------------------------------

//cycle the flashing light for when mine destroyed
void FlashFrame (void)
{
	static fixang flash_ang = 0;

if (!(gameData.reactor.bDestroyed || gameStates.gameplay.seismic.nMagnitude)) {
	gameStates.render.nFlashScale = I2X (1);
	return;
	}
if (gameStates.app.bEndLevelSequence)
	return;
if (paletteManager.BlueEffect () > 10)		//whiting out
	return;
//	flash_ang += FixMul(FLASH_CYCLE_RATE, gameData.time.xFrame);
if (gameStates.gameplay.seismic.nMagnitude) {
	fix xAddedFlash = abs(gameStates.gameplay.seismic.nMagnitude);
	if (xAddedFlash < I2X (1))
		xAddedFlash *= 16;
	flash_ang += (fixang) FixMul (gameStates.render.nFlashRate, FixMul(gameData.time.xFrame, xAddedFlash+I2X (1)));
	FixFastSinCos (flash_ang, &gameStates.render.nFlashScale, NULL);
	gameStates.render.nFlashScale = (gameStates.render.nFlashScale + I2X (3))/4;	//	gets in range 0.5 to 1.0
	}
else {
	flash_ang += (fixang) FixMul (gameStates.render.nFlashRate, gameData.time.xFrame);
	FixFastSinCos (flash_ang, &gameStates.render.nFlashScale, NULL);
	gameStates.render.nFlashScale = (gameStates.render.nFlashScale + I2X (1))/2;
	if (gameStates.app.nDifficultyLevel == 0)
		gameStates.render.nFlashScale = (gameStates.render.nFlashScale+I2X (3))/4;
	}
}

// ----------------------------------------------------------------------------
//	Render a face.
//	It would be nice to not have to pass in nSegment and nSide, but
//	they are used for our hideously hacked in headlight system.
//	vp is a pointer to vertex ids.
//	tmap1, tmap2 are texture map ids.  tmap2 is the pasty one.

int RenderColoredSegFace (int nSegment, int nSide, int nVertices, g3sPoint **pointList)
{
	short nConnSeg = SEGMENTS [nSegment].m_children [nSide];
	int	owner = SEGMENTS [nSegment].m_owner;
	int	special = SEGMENTS [nSegment].m_nType;

if ((gameData.app.nGameMode & GM_ENTROPY) && (extraGameInfo [1].entropy.nOverrideTextures == 2) && (owner > 0)) {
	if ((nConnSeg >= 0) && (SEGMENTS [nConnSeg].m_owner == owner))
			return 0;
	if (owner == 1)
		G3DrawPolyAlpha (nVertices, pointList, segmentColors + 1, 0, nSegment);
	else
		G3DrawPolyAlpha (nVertices, pointList, segmentColors, 0, nSegment);
	return 1;
	}
if (special == SEGMENT_IS_WATER) {
	if ((nConnSeg < 0) || (SEGMENTS [nConnSeg].m_nType != SEGMENT_IS_WATER))
		G3DrawPolyAlpha (nVertices, pointList, segmentColors + 2, 0, nSegment);
	return 1;
	}
if (special == SEGMENT_IS_LAVA) {
	if ((nConnSeg < 0) || (SEGMENTS [nConnSeg].m_nType != SEGMENT_IS_LAVA))
		G3DrawPolyAlpha (nVertices, pointList, segmentColors + 3, 0, nSegment);
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

int RenderWall (tFaceProps *propsP, g3sPoint **pointList, int bIsMonitor)
{
	short c, nWallNum = SEGMENTS [propsP->segNum].WallNum (propsP->sideNum);
	static tRgbaColorf cloakColor = {0, 0, 0, -1};

if (IS_WALL (nWallNum)) {
	if (propsP->widFlags & (WID_CLOAKED_FLAG | WID_TRANSPARENT_FLAG)) {
		if (!bIsMonitor) {
			if (!RenderColoredSegFace (propsP->segNum, propsP->sideNum, propsP->nVertices, pointList)) {
				c = WALLS [nWallNum].cloakValue;
				if (propsP->widFlags & WID_CLOAKED_FLAG) {
					if (c < FADE_LEVELS) {
						gameStates.render.grAlpha = GrAlpha (ubyte (c));
						G3DrawPolyAlpha (propsP->nVertices, pointList, &cloakColor, 1, propsP->segNum);		//draw as flat poly
						}
					}
				else {
					if (!gameOpts->render.color.bWalls)
						c = 0;
					if (WALLS [nWallNum].hps)
						gameStates.render.grAlpha = float (WALLS [nWallNum].hps) / float (I2X (100));
					else if (IsMultiGame && gameStates.app.bHaveExtraGameInfo [1])
						gameStates.render.grAlpha = COMPETITION ? 2.0f / 3.0f : GrAlpha (FADE_LEVELS - extraGameInfo [1].grWallTransparency);
					else
						gameStates.render.grAlpha = GrAlpha (ubyte (FADE_LEVELS - extraGameInfo [0].grWallTransparency));
					if (gameStates.render.grAlpha < 1.0f) {
						tRgbaColorf wallColor;
						
						paletteManager.Game ()->ToRgbaf ((ubyte) c, wallColor); 
						G3DrawPolyAlpha (propsP->nVertices, pointList, &wallColor, 1, propsP->segNum);	//draw as flat poly
						}
					}
				}
			gameStates.render.grAlpha = 1.0f;
			return 1;
			}
		}
	else if (gameStates.app.bD2XLevel) {
		c = WALLS [nWallNum].cloakValue;
		if (c && (c < FADE_LEVELS))
			gameStates.render.grAlpha = GrAlpha (FADE_LEVELS - c);
		}
	else if (gameOpts->render.effects.bAutoTransparency && IsTransparentFace (propsP))
		gameStates.render.grAlpha = 0.8f;
	else
		gameStates.render.grAlpha = 1.0f;
	}
return 0;
}

//------------------------------------------------------------------------------

void RenderFace (tFaceProps *propsP)
{
	tFaceProps	props = *propsP;
	CBitmap  *bmBot = NULL;
	CBitmap  *bmTop = NULL;

	int			i, bIsMonitor, bIsTeleCam, bHaveMonitorBg, nCamNum, bCamBufAvail;
	g3sPoint		*pointList [8], **pp;
	CSegment		*segP = SEGMENTS + props.segNum;
	CSide			*sideP = segP->m_sides + props.sideNum;
	CCamera		*cameraP = NULL;

if (props.nBaseTex < 0)
	return;
if (gameStates.render.nShadowPass == 2) {
#if DBG_SHADOWS
	if (!bWallShadows)
		return;
#endif
	G3SetCullAndStencil (0, 0);
	RenderFaceShadow (propsP);
	G3SetCullAndStencil (1, 0);
	RenderFaceShadow (propsP);
	return;
	}
#if DBG //convenient place for a debug breakpoint
if (props.segNum == nDbgSeg && ((nDbgSide < 0) || (props.sideNum == nDbgSide)))
	props.segNum = props.segNum;
if (props.nBaseTex == nDbgBaseTex)
	props.segNum = props.segNum;
if (props.nOvlTex == nDbgOvlTex)
	props.segNum = props.segNum;
#	if 0
else
	return;
#	endif
#endif

gameData.render.vertexList = gameData.segs.fVertices.Buffer ();
Assert(props.nVertices <= 4);
for (i = 0, pp = pointList; i < props.nVertices; i++, pp++)
	*pp = gameData.segs.points + props.vp [i];
if (!(gameOpts->render.debug.bTextures || IsMultiGame))
	goto drawWireFrame;
#if 1
if (gameStates.render.nShadowBlurPass == 1) {
	G3DrawWhitePoly (props.nVertices, pointList);
	gameData.render.vertexList = NULL;
	return;
	}
#endif
SetVertexColors (&props);
if (gameStates.render.nType == 2) {
#if DBG //convenient place for a debug breakpoint
	if (props.segNum == nDbgSeg && ((nDbgSide < 0) || (props.sideNum == nDbgSide)))
		props.segNum = props.segNum;
#endif
	RenderColoredSegFace (props.segNum, props.sideNum, props.nVertices, pointList);
	gameData.render.vertexList = NULL;
	return;
	}
nCamNum = IsMonitorFace (props.segNum, props.sideNum, 0);
if ((bIsMonitor = gameStates.render.bUseCameras && (nCamNum >= 0))) {
	cameraP = cameraManager.Camera (nCamNum);
	cameraP->SetVisible (1);
	bIsTeleCam = cameraP->GetTeleport ();
#if RENDER2TEXTURE
	bCamBufAvail = cameraP->HaveBuffer (1) == 1;
#else
	bCamBufAvail = 0;
#endif
	bHaveMonitorBg = cameraP->Valid () && /*!cameraP->bShadowMap &&*/
						  (cameraP->HaveTexture () || bCamBufAvail) &&
						  (!bIsTeleCam || EGI_FLAG (bTeleporterCams, 0, 1, 0));
	}
else
	bIsTeleCam =
	bHaveMonitorBg =
	bCamBufAvail = 0;
if (RenderWall (&props, pointList, bIsMonitor)) {	//handle semi-transparent walls
	gameData.render.vertexList = NULL;
	return;
	}
if (props.widFlags & WID_RENDER_FLAG) {
	if (props.nBaseTex >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]) {
	sideP->m_nBaseTex = 0;
	}
if (!(bHaveMonitorBg && gameOpts->render.cameras.bFitToWall)) {
	if (gameStates.render.nType == 3) {
		bmBot = bmpCorona;
		bmTop = NULL;
		props.uvls [0].u =
		props.uvls [0].v =
		props.uvls [1].v =
		props.uvls [3].u = I2X (1) / 4;
		props.uvls [1].u =
		props.uvls [2].u =
		props.uvls [2].v =
		props.uvls [3].v = I2X (3) / 4;
		}
	else if (gameOpts->ogl.bGlTexMerge) {
		bmBot = LoadFaceBitmap (props.nBaseTex, sideP->m_nFrame);
		if (props.nOvlTex)
			bmTop = LoadFaceBitmap ((short) (props.nOvlTex), sideP->m_nFrame);
		}
	else {
		if (props.nOvlTex != 0) {
			bmBot = TexMergeGetCachedBitmap (props.nBaseTex, props.nOvlTex, props.nOvlOrient);
#if DBG
			if (!bmBot)
				bmBot = TexMergeGetCachedBitmap (props.nBaseTex, props.nOvlTex, props.nOvlOrient);
#endif
			}
		else {
			bmBot = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [props.nBaseTex].index;
			LoadBitmap (gameData.pig.tex.bmIndexP [props.nBaseTex].index, gameStates.app.bD1Mission);
			}
		}
	}

if (bHaveMonitorBg) {
	cameraP->GetUVL (NULL, props.uvls, NULL, NULL);
	if (bIsTeleCam) {
#if DBG
		bmBot = &cameraP->Texture ();
		gameStates.render.grAlpha = 1.0f;
#else
		bmTop = &cameraP->Texture ();
		gameStates.render.grAlpha = 0.7f;
#endif
		}
	else if (gameOpts->render.cameras.bFitToWall || (props.nOvlTex > 0))
		bmBot = &cameraP->Texture ();
	else
		bmTop = &cameraP->Texture ();
	}
SetFaceLight (&props);
#if DBG //convenient place for a debug breakpoint
if (props.segNum == nDbgSeg && props.sideNum == nDbgSide)
	props.segNum = props.segNum;
#endif
#if DBG
if (bmTop)
	fpDrawTexPolyMulti (
		props.nVertices, pointList, props.uvls,
#	if LIGHTMAPS
		props.uvl_lMaps,
#	endif
		bmBot, bmTop,
#	if LIGHTMAPS
		NULL, //lightmaps + props.segNum * 6 + props.sideNum,
#	endif
		&props.vNormal, props.nOvlOrient, !bIsMonitor || bIsTeleCam, props.segNum);
else
#	if LIGHTMAPS == 0
	G3DrawTexPoly (props.nVertices, pointList, props.uvls, bmBot, &props.vNormal, !bIsMonitor || bIsTeleCam, props.segNum);
#	else
	fpDrawTexPolyMulti (
		props.nVertices, pointList, props.uvls, props.uvl_lMaps, bmBot, NULL,
		NULL, //lightmaps + props.segNum * 6 + props.sideNum,
		&props.vNormal, 0, !bIsMonitor || bIsTeleCam, props.segNum);
#	endif
#else
fpDrawTexPolyMulti (
	props.nVertices, pointList, props.uvls,
#	if LIGHTMAPS
	props.uvl_lMaps,
#	endif
	bmBot, bmTop,
#	if LIGHTMAPS
	NULL, //lightmaps + props.segNum * 6 + props.sideNum,
#	endif
	&props.vNormal, props.nOvlOrient, !bIsMonitor || bIsTeleCam,
	props.segNum);
#endif
	}
gameStates.render.grAlpha = 1.0f;
gameStates.ogl.fAlpha = 1;
	// render the CSegment the CPlayerData is in with a transparent color if it is a water or lava CSegment
	//if (nSegment == OBJECTS->nSegment)
#if DBG
if (bOutLineMode)
	DrawOutline (props.nVertices, pointList);
#endif

drawWireFrame:

if (gameOpts->render.debug.bWireFrame && !IsMultiGame)
	DrawOutline (props.nVertices, pointList);
}

fix	Tulate_min_dot = (I2X (1)/4);
//--unused-- fix	Tulate_min_ratio = (I2X (2));
fix	Min_n0_n1_dot	= (I2X (15)/16);

// -----------------------------------------------------------------------------------
//	Render a side.
//	Check for Normal facing.  If so, render faces on CSide dictated by sideP->m_nType.

#undef LMAP_LIGHTADJUST
#define LMAP_LIGHTADJUST 0

void RenderSide (CSegment *segP, short nSide)
{
	CSide			*sideP = segP->m_sides + nSide;
	tFaceProps	props;

#if LIGHTMAPS
#define	LMAP_SIZE	(1.0 / 16.0)

	static tUVL	uvl_lMaps [4] = {
	 {F2X (LMAP_SIZE), F2X (LMAP_SIZE), 0}, 
	 {F2X (1.0 - LMAP_SIZE), F2X (LMAP_SIZE), 0}, 
	 {F2X (1.0 - LMAP_SIZE), F2X (1.0 - LMAP_SIZE), 0}, 
	 {F2X (LMAP_SIZE), F2X (1.0 - LMAP_SIZE), 0}
	};
#endif

props.segNum = segP->Index ();
props.sideNum = nSide;
#if DBG
if ((props.segNum == nDbgSeg) && ((nDbgSide < 0) || (props.sideNum == nDbgSide)))
	segP = segP;
#endif
props.widFlags = segP->IsDoorWay (props.sideNum, NULL);
if (!(gameOpts->render.debug.bWalls || IsMultiGame) && IS_WALL (segP->WallNum (props.sideNum)))
	return;
switch (gameStates.render.nType) {
	case -1:
		if (!(props.widFlags & WID_RENDER_FLAG) && (SEGMENTS [props.segNum].m_nType < SEGMENT_IS_WATER))		//if (WALL_IS_DOORWAY(segP, props.sideNum) == WID_NO_WALL)
			return;
		break;
	case 0:
		if (segP->m_children [props.sideNum] >= 0) //&& IS_WALL (WallNumP (segP, props.sideNum)))
			return;
		break;
	case 1:
		if (!IS_WALL (segP->WallNum (props.sideNum)))
			return;
		break;
	case 2:
		if ((SEGMENTS [props.segNum].m_nType < SEGMENT_IS_WATER) &&
			 (SEGMENTS [props.segNum].m_owner < 1))
			return;
		break;
	case 3:
		if ((IsLight (sideP->m_nBaseTex) || (sideP->m_nOvlTex && IsLight (sideP->m_nOvlTex))))
			RenderCorona (props.segNum, props.sideNum, 1, 20);
		return;
	}
#if LIGHTMAPS
if (gameStates.render.bDoLightmaps) {
		float	Xs = 8;
		float	h = 0.5f / (float) Xs;

	props.uvl_lMaps [0].u =
	props.uvl_lMaps [0].v =
	props.uvl_lMaps [1].v =
	props.uvl_lMaps [3].u = F2X (h);
	props.uvl_lMaps [1].u =
	props.uvl_lMaps [2].u =
	props.uvl_lMaps [2].v =
	props.uvl_lMaps [3].v = F2X (1-h);
	}
#endif
props.nBaseTex = sideP->m_nBaseTex;
props.nOvlTex = sideP->m_nOvlTex;
props.nOvlOrient = sideP->m_nOvlOrient;

	//	========== Mark: Here is the change...beginning here: ==========

#if LIGHTMAPS
	if (gameStates.render.bDoLightmaps) {
		memcpy (props.uvl_lMaps, uvl_lMaps, sizeof (tUVL) * 4);
#if LMAP_LIGHTADJUST
		props.uvls [0].l = props.uvls [1].l = props.uvls [2].l = props.uvls [3].l = I2X (1) / 2;
#	endif
		}
#endif

#if DBG //convenient place for a debug breakpoint
if (props.segNum == nDbgSeg && props.sideNum == nDbgSide)
	props.segNum = props.segNum;
if (props.nBaseTex == nDbgBaseTex)
	props.segNum = props.segNum;
if (props.nOvlTex == nDbgOvlTex)
	props.segNum = props.segNum;
#	if 0
else
	return;
#	endif
#endif

if (!FaceIsVisible (props.segNum, props.sideNum))
	return;
if (sideP->m_nType == SIDE_IS_QUAD) {
	props.vNormal = sideP->m_normals [0];
	props.nVertices = 4;
	memcpy (props.uvls, sideP->m_uvls, sizeof (tUVL) * 4);
	memcpy (props.vp, SEGMENTS [props.segNum].Corners (props.sideNum), 4 * sizeof (ushort));
	RenderFace (&props);
	}
else {
	// new code
	// non-planar faces are still passed as quads to the renderer as it will render triangles (GL_TRIANGLE_FAN) anyway
	// just need to make sure the vertices come in the proper order depending of the the orientation of the two non-planar triangles
	props.vNormal = sideP->m_normals [0] + sideP->m_normals [1];
	props.vNormal *= (I2X (1) / 2);
	props.nVertices = 4;
	if (sideP->m_nType == SIDE_IS_TRI_02) {
		memcpy (props.uvls, sideP->m_uvls, sizeof (tUVL) * 4);
		memcpy (props.vp, SEGMENTS [props.segNum].Corners (props.sideNum), 4 * sizeof (ushort));
		RenderFace (&props);
		}
	else if (sideP->m_nType == SIDE_IS_TRI_13) {	//just rendering the fan with vertex 1 instead of 0
		memcpy (props.uvls + 1, sideP->m_uvls, sizeof (tUVL) * 3);
		props.uvls [0] = sideP->m_uvls [3];
		memcpy (props.vp + 1, SEGMENTS [props.segNum].Corners (props.sideNum), 4 * sizeof (ushort));
		props.vp [0] = props.vp [4];
		RenderFace (&props);
		}
	else {
		Error("Illegal CSide nType in RenderSide, nType = %i, CSegment # = %i, CSide # = %i\n", sideP->m_nType, segP->Index (), props.sideNum);
		return;
		}
	}
}

// -----------------------------------------------------------------------------------

static int RenderSegmentFaces (short nSegment, int nWindow)
{
	CSegment		*segP = SEGMENTS + nSegment;
	g3sCodes 	cc;
	short			nSide;

OglSetupTransform (0);
cc = RotateVertexList (8, segP->m_verts);
gameData.render.vertP = gameData.segs.fVertices.Buffer ();
//	return;
if (cc.ccAnd /*&& !automap.m_bDisplay*/)	//all off screen and not rendering the automap
	return 0;
gameStates.render.nState = 0;
#if DBG //convenient place for a debug breakpoint
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
lightManager.SetNearestToSegment (nSegment, -1, 0, 0, 0);
for (nSide = 0; nSide < 6; nSide++) //segP->nFaces, faceP = segP->pFaces; nSide; nSide--, faceP++)
	RenderSide (segP, nSide);
OglResetTransform (0);
OGL_BINDTEX (0);
return 1;
}

//------------------------------------------------------------------------------

void DoRenderObject (int nObject, int nWindow)
{
	CObject*					objP = OBJECTS + nObject;
	int						count = 0;

if (!(IsMultiGame || gameOpts->render.debug.bObjects))
	return;
Assert(nObject < LEVEL_OBJECTS);
#if 0
if (!(nWindow || gameStates.render.cameras.bActive) && (gameStates.render.nShadowPass < 2) &&
    (gameData.render.mine.bObjectRendered [nObject] == gameStates.render.nFrameFlipFlop))	//already rendered this...
	return;
#endif
if (gameData.demo.nState == ND_STATE_PLAYBACK) {
	if ((nDemoDoingLeft == 6 || nDemoDoingRight == 6) && objP->info.nType == OBJ_PLAYER) {
  		return;
		}
	}
if ((count++ > LEVEL_OBJECTS) || (objP->info.nNextInSeg == nObject)) {
	Int3();					// infinite loop detected
	objP->info.nNextInSeg = -1;		// won't this clean things up?
	return;					// get out of this infinite loop!
	}
if (RenderObject (objP, nWindow, 0)) {
	gameData.render.mine.bObjectRendered [nObject] = gameStates.render.nFrameFlipFlop;
	if (!gameStates.render.cameras.bActive) {
		tWindowRenderedData*	wrd = windowRenderedData + nWindow;
		int nType = objP->info.nType;
		if ((nType == OBJ_ROBOT) || (nType == OBJ_PLAYER) ||
			 ((nType == OBJ_WEAPON) && (WeaponIsPlayerMine (objP->info.nId) || (gameData.objs.bIsMissile [objP->info.nId] && EGI_FLAG (bKillMissiles, 0, 0, 0))))) {
			if (wrd->nObjects >= MAX_RENDERED_OBJECTS) {
				Int3();
				wrd->nObjects /= 2;
				}
			wrd->renderedObjects [wrd->nObjects++] = nObject;
			}
		}
	}
for (int i = objP->info.nAttachedObj; i != -1; i = objP->cType.explInfo.attached.nNext) {
	objP = OBJECTS + i;
	Assert (objP->info.nType == OBJ_FIREBALL);
	Assert (objP->info.controlType == CT_EXPLOSION);
	Assert (objP->info.nFlags & OF_ATTACHED);
	RenderObject (objP, nWindow, 1);
	}
}

//------------------------------------------------------------------------------
//increment counter for checking if points bTransformed
//This must be called at the start of the frame if RotateVertexList() will be used
void RenderStartFrame (void)
{
if (!++gameStates.render.nFrameCount) {		//wrap!
	gameData.render.mine.nRotatedLast.Clear (0);		//clear all to zero
	gameStates.render.nFrameCount = 1;											//and set this frame to 1
	}
}

//------------------------------------------------------------------------------

void SetRenderView (fix nEyeOffset, short *pnStartSeg, int bOglScale)
{
	short nStartSeg;

gameData.render.mine.viewerEye = gameData.objs.viewerP->info.position.vPos;
if (nEyeOffset) {
	gameData.render.mine.viewerEye += gameData.objs.viewerP->info.position.mOrient.RVec() * nEyeOffset;
	}

externalView.SetPos (NULL);
if (gameStates.render.cameras.bActive) {
	nStartSeg = gameData.objs.viewerP->info.nSegment;
	G3SetViewMatrix (gameData.render.mine.viewerEye, gameData.objs.viewerP->info.position.mOrient, gameStates.render.xZoom, bOglScale);
	}
else {
	if (!pnStartSeg)
		nStartSeg = gameStates.render.nStartSeg;
	else {
		nStartSeg = FindSegByPos (gameData.render.mine.viewerEye, gameData.objs.viewerP->info.nSegment, 1, 0);
		if (!gameStates.render.nWindow && (gameData.objs.viewerP == gameData.objs.consoleP)) {
			externalView.SetPoint (gameData.objs.viewerP);
			if (nStartSeg == -1)
				nStartSeg = gameData.objs.viewerP->info.nSegment;
			}
		}
	if ((gameData.objs.viewerP == gameData.objs.consoleP) && transformation.m_info.bUsePlayerHeadAngles) {
		CFixMatrix mHead, mView;
		mHead = CFixMatrix::Create(transformation.m_info.playerHeadAngles);
		mView = gameData.objs.viewerP->info.position.mOrient * mHead;
		G3SetViewMatrix (gameData.render.mine.viewerEye, mView, gameStates.render.xZoom, bOglScale);
		}
	else if (gameStates.render.bRearView && (gameData.objs.viewerP == gameData.objs.consoleP)) {
#if 1
		CFixMatrix mView;

		mView = gameData.objs.viewerP->info.position.mOrient;
		mView.FVec().Neg();
		mView.RVec().Neg();
#else
		CFixMatrix mHead, mView;

		transformation.m_info.playerHeadAngles [PA] = 0;
		transformation.m_info.playerHeadAngles [BA] = 0x7fff;
		transformation.m_info.playerHeadAngles [HA] = 0x7fff;
		VmAngles2Matrix (&mHead, &transformation.m_info.playerHeadAngles);
		VmMatMul (&mView, &gameData.objs.viewerP->info.position.mOrient, &mHead);
#endif
		G3SetViewMatrix (gameData.render.mine.viewerEye, mView,  //gameStates.render.xZoom, bOglScale);
							  FixDiv (gameStates.render.xZoom, gameStates.zoom.nFactor), bOglScale);
		}
	else if ((gameData.objs.viewerP == gameData.objs.consoleP) && (!IsMultiGame || gameStates.app.bHaveExtraGameInfo [1])) {
		gameStates.zoom.nMinFactor = I2X (gameStates.render.glAspect); 
		gameStates.zoom.nMaxFactor = gameStates.zoom.nMinFactor * 5;
		HandleZoom ();
		if ((gameData.objs.viewerP == gameData.objs.consoleP) &&
#if DBG
			 gameStates.render.bChaseCam) {
#else
			 gameStates.render.bChaseCam && (!IsMultiGame || IsCoopGame || EGI_FLAG (bEnableCheats, 0, 0, 0))) {
#endif
			externalView.GetViewPoint ();
			G3SetViewMatrix (gameData.render.mine.viewerEye,
								  externalView.GetPos () ? externalView.GetPos ()->mOrient : gameData.objs.viewerP->info.position.mOrient,
								  gameStates.render.xZoom, bOglScale);
			}
		else
			G3SetViewMatrix (gameData.render.mine.viewerEye, gameData.objs.viewerP->info.position.mOrient,
								  FixDiv (gameStates.render.xZoom, gameStates.zoom.nFactor), bOglScale);
		}
	else
		G3SetViewMatrix (gameData.render.mine.viewerEye, gameData.objs.viewerP->info.position.mOrient,
							  gameStates.render.xZoom, bOglScale);
	}
if (pnStartSeg)
	*pnStartSeg = nStartSeg;
}

//------------------------------------------------------------------------------

void RenderEffects (int nWindow)
{
	int bLightnings, bParticles, bSparks;

PROF_START
#if UNIFY_THREADS
WaitForRenderThreads ();
#else
WaitForEffectsThread ();
#endif
if (automap.m_bDisplay) {
	bLightnings = gameOpts->render.automap.bLightnings;
	bParticles = gameOpts->render.automap.bParticles;
	bSparks = gameOpts->render.automap.bSparks;
	}
else {
	bSparks = (gameOptions [0].render.nQuality > 0);
	bLightnings = (!nWindow || gameOpts->render.lightning.bAuxViews) && 
					  (!gameStates.render.cameras.bActive || gameOpts->render.lightning.bMonitors);
	bParticles = (!nWindow || gameOpts->render.particles.bAuxViews) &&
					 (!gameStates.render.cameras.bActive || gameOpts->render.particles.bMonitors);
	}
if (bSparks) {
	SEM_ENTER (SEM_SPARKS)
	//PrintLog ("RenderEnergySparks\n");
	sparkManager.Render ();
	//SEM_LEAVE (SEM_SPARKS)
	}
if (bParticles) {
	SEM_ENTER (SEM_SMOKE)
	//PrintLog ("RenderSmoke\n");
	particleManager.Cleanup ();
	particleManager.Render ();
	//SEM_LEAVE (SEM_SMOKE)
	}
if (bLightnings) {
	SEM_ENTER (SEM_LIGHTNING)
	//PrintLog ("RenderLightnings\n");
	lightningManager.Render ();
	}
//PrintLog ("transparencyRenderer.Render\n");
if (bLightnings)
	SEM_LEAVE (SEM_LIGHTNING)
transparencyRenderer.Render ();
#if 1
if (bParticles)
	SEM_LEAVE (SEM_SMOKE)
if (bSparks)
	SEM_LEAVE (SEM_SPARKS)
#endif
PROF_END(ptEffects)
}

//------------------------------------------------------------------------------

extern CBitmap bmBackground;

void RenderFrame (fix nEyeOffset, int nWindow)
{
	short nStartSeg;

gameStates.render.nWindow = nWindow;
gameStates.render.nEyeOffset = nEyeOffset;
if (gameStates.app.bEndLevelSequence) {
	RenderEndLevelFrame (nEyeOffset, nWindow);
	gameData.app.nFrameCount++;
	return;
	}
#ifdef NEWDEMO
if ((gameData.demo.nState == ND_STATE_RECORDING) && (nEyeOffset >= 0)) {
   if (!gameStates.render.nRenderingType)
   	NDRecordStartFrame (gameData.app.nFrameCount, gameData.time.xFrame);
   if (gameStates.render.nRenderingType != 255)
   	NDRecordViewerObject (gameData.objs.viewerP);
	}
#endif

//PrintLog ("StartLightingFrame\n");
StartLightingFrame (gameData.objs.viewerP);		//this is for ugly light-smoothing hack
gameStates.ogl.bEnableScissor = !gameStates.render.cameras.bActive && nWindow;
if (!nWindow)
	gameData.render.dAspect = (double) CCanvas::Current ()->Width () / (double) CCanvas::Current ()->Height ();
//PrintLog ("G3StartFrame\n");
{
PROF_START
G3StartFrame (0, !(nWindow || gameStates.render.cameras.bActive));
//PrintLog ("SetRenderView\n");
SetRenderView (nEyeOffset, &nStartSeg, 1);
PROF_END(ptAux)
}
if (0 > (gameStates.render.nStartSeg = nStartSeg)) {
	G3EndFrame ();
	return;
	}
#if CLEAR_WINDOW == 1
if (!nClearWindowColor)
	nClearWindowColor = BLACK_RGBA;	//BM_XRGB(31, 15, 7);
CCanvas::Current ()->Clear (nClearWindowColor);
#endif

#if DBG
if (bShowOnlyCurSide)
	CCanvas::Current ()->Clear (nClearWindowColor);
#endif
#if SHADOWS
if (!gameStates.render.bHaveStencilBuffer)
	extraGameInfo [0].bShadows =
	extraGameInfo [1].bShadows = 0;
if (SHOW_SHADOWS &&
	 !(nWindow || gameStates.render.cameras.bActive || automap.m_bDisplay)) {
	if (!gameStates.render.bShadowMaps) {
		gameStates.render.nShadowPass = 1;
#if SOFT_SHADOWS
		if (gameOpts->render.shadows.bSoft = 1)
			gameStates.render.nShadowBlurPass = 1;
#endif
		OglStartFrame (0, 0);
#if SOFT_SHADOWS
		OglViewport (CCanvas::Current ()->props.x, CCanvas::Current ()->props.y, 128, 128);
#endif
		RenderMine (nStartSeg, nEyeOffset, nWindow);
#if 1//!DBG
		PROF_START
		RenderFastShadows (nEyeOffset, nWindow, nStartSeg);
		PROF_END(ptEffects)
#else
		if (FAST_SHADOWS)
			;//RenderFastShadows (nEyeOffset, nWindow, nStartSeg);
		else {
			PROF_START
			RenderNeatShadows (nEyeOffset, nWindow, nStartSeg);
			PROF_END(ptEffects)
			}
#endif
#if SOFT_SHADOWS
		if (gameOpts->render.shadows.bSoft) {
			PROF_START
			CreateShadowTexture ();
			PROF_END(ptEffects)
			gameStates.render.nShadowBlurPass = 2;
			gameStates.render.nShadowPass = 0;
#if 1
			OglStartFrame (0, 1);
			SetRenderView (nEyeOffset, &nStartSeg, 1);
			RenderMine (nStartSeg, nEyeOffset, nWindow);
#endif
			RenderShadowTexture ();
			}
#endif
		nWindow = 0;
		}
	}
else
#endif
	{
	if (gameStates.render.nRenderPass < 0)
		RenderMine (nStartSeg, nEyeOffset, nWindow);
	else {
		for (gameStates.render.nRenderPass = 0; gameStates.render.nRenderPass < 2; gameStates.render.nRenderPass++) {
			OglStartFrame (0, 1);
			RenderMine (nStartSeg, nEyeOffset, nWindow);
			}
		}
	}
StencilOff ();
RenderSkyBox (nWindow);
#if 1
RenderEffects (nWindow);
#endif
#if 1
if (!(nWindow || gameStates.render.cameras.bActive || gameStates.app.bEndLevelSequence || GuidedInMainView ())) {
	//PrintLog ("RenderRadar\n");
	RenderRadar ();
	}
#endif
#if 0
if (transformation.m_info.bUsePlayerHeadAngles)
	Draw3DReticle (nEyeOffset);
#endif
gameStates.render.nShadowPass = 0;
//PrintLog ("G3EndFrame\n");
G3EndFrame ();
if (!ShowGameMessage (gameData.messages, -1, -1))
	ShowGameMessage (gameData.messages + 1, -1, -1);
}

//------------------------------------------------------------------------------

static tObjRenderListItem objRenderList [MAX_OBJECTS_D2X];

void QSortObjRenderList (int left, int right)
{
	int	l = left,
			r = right,
			m = objRenderList [(l + r) / 2].xDist;

do {
	while (objRenderList [l].xDist < m)
		l++;
	while (objRenderList [r].xDist > m)
		r--;
	if (l <= r) {
		if (l < r) {
			tObjRenderListItem h = objRenderList [l];
			objRenderList [l] = objRenderList [r];
			objRenderList [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortObjRenderList (l, right);
if (left < r)
	QSortObjRenderList (left, r);
}

//------------------------------------------------------------------------------

int SortObjList (int nSegment)
{
	tObjRenderListItem*	pi;
	int						i, j;

if (nSegment < 0)
	nSegment = -nSegment - 1;
for (i = gameData.render.mine.renderObjs.ref [nSegment], j = 0; i >= 0; i = pi->nNextItem) {
	pi = gameData.render.mine.renderObjs.objs + i;
	objRenderList [j++] = *pi;
	}
#if 1
if (j > 1)
	QSortObjRenderList (0, j - 1);
#endif
return j;
}

//------------------------------------------------------------------------------

void RenderObjList (int nListPos, int nWindow)
{
PROF_START
	int i, j;
	int saveLinDepth = gameStates.render.detail.nMaxLinearDepth;

gameStates.render.detail.nMaxLinearDepth = gameStates.render.detail.nMaxLinearDepthObjects;
for (i = 0, j = SortObjList (gameData.render.mine.nSegRenderList [nListPos]); i < j; i++)
	DoRenderObject (objRenderList [i].nObject, nWindow);	// note link to above else
gameStates.render.detail.nMaxLinearDepth = saveLinDepth;
PROF_END(ptRenderObjects)
}

//------------------------------------------------------------------------------

void RenderSegment (int nListPos)
{
	int nSegment = (nListPos < 0) ? -nListPos - 1 : gameData.render.mine.nSegRenderList [nListPos];

if (nSegment < 0)
	return;
if (automap.m_bDisplay) {
	if (!(automap.m_bFull || automap.m_visited [0][nSegment]))
		return;
	if (!gameOpts->render.automap.bSkybox && (SEGMENTS [nSegment].m_nType == SEGMENT_IS_SKYBOX))
		return;
	}
else {
	if (VISITED (nSegment))
		return;
	}
#if DBG
if (nSegment == nDbgSeg)
	nSegment = nSegment;
#endif
VISIT (nSegment);
if (!RenderSegmentFaces (nSegment, gameStates.render.nWindow)) {
	gameData.render.mine.nSegRenderList [nListPos] = -gameData.render.mine.nSegRenderList [nListPos] - 1;
	gameData.render.mine.bVisible [gameData.render.mine.nSegRenderList [nListPos]] = gameData.render.mine.nVisible - 1;
	return;
	}
if ((gameStates.render.nType == 0) && !automap.m_bDisplay)
	automap.m_visited [0][nSegment] = gameData.render.mine.bSetAutomapVisited;
else if ((gameStates.render.nType == 1) && (gameData.render.mine.renderObjs.ref [gameData.render.mine.nSegRenderList [nListPos]] >= 0)) {
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	lightManager.SetNearestStatic (nSegment, 1, 1, 0);
	gameStates.render.bApplyDynLight = (gameStates.render.nLightingMethod != 0) && (gameOpts->ogl.bObjLighting || gameOpts->ogl.bLightObjects);
	RenderObjList (nListPos, gameStates.render.nWindow);
	gameStates.render.bApplyDynLight = (gameStates.render.nLightingMethod != 0);
	//gameData.render.lights.dynamic.shader.index [0][0].nActive = gameData.render.lights.dynamic.shader.iStaticLights [0];
	}
else if (gameStates.render.nType == 2)	// render objects containing transparency, like explosions
	RenderObjList (nListPos, gameStates.render.nWindow);
}

//------------------------------------------------------------------------------
//renders onto current canvas

int BeginRenderMine (short nStartSeg, fix nEyeOffset, int nWindow)
{
PROF_START
#if CLEAR_WINDOW == 2
	tPortal	*oldPortal;
#endif
#if 0//def _DEBUG
	int		i;
#endif

if (!nWindow)
	GetPlayerMslLock ();	// uses rendered object info from previous frame stored in windowRenderedData
if (!gameStates.render.cameras.bActive)
	windowRenderedData [nWindow].nObjects = 0;
#ifdef LASER_HACK
nHackLasers = 0;
#endif
//set up for rendering
gameStates.ogl.fAlpha = FADE_LEVELS;
if (((gameStates.render.nRenderPass <= 0) &&
	  (gameStates.render.nShadowPass < 2) && (gameStates.render.nShadowBlurPass < 2)) ||
	 gameStates.render.bShadowMaps) {
	if (!automap.m_bDisplay)
		RenderStartFrame ();
#if USE_SEGRADS
	TransformSideCenters ();
#endif
	if (!RENDERPATH)
		RotateSideNorms ();
	 }
if ((gameStates.render.nRenderPass <= 0) && (gameStates.render.nShadowPass < 2)) {
	gameStates.ogl.bUseTransform = RENDERPATH;
	BuildRenderSegList (nStartSeg, nWindow);		//fills in gameData.render.mine.nSegRenderList & gameData.render.mine.nRenderSegs
	if ((gameStates.render.nRenderPass <= 0) && (gameStates.render.nShadowPass < 2)) {
		BuildRenderObjLists (gameData.render.mine.nRenderSegs);
#if 1
		if (nEyeOffset <= 0)	// Do for left eye or zero.
			SetDynamicLight ();
#endif
		}
	gameStates.ogl.bUseTransform = 0;
	lightManager.Transform (0, 1);
	}

#if CLEAR_WINDOW == 2
	if (gameData.render.gameData.render.nFirstTerminalSeg < gameData.render.mine.nRenderSegs) {
		int i;

		if (nClearWindowColor == (uint) -1)
			nClearWindowColor = BLACK_RGBA;
		CCanvas::Current ()->SetColor (nClearWindowColor);
		for (i = gameData.render.gameData.render.nFirstTerminalSeg, oldPortal = renderPortals; i < gameData.render.mine.nRenderSegs; i++, oldPortal++) {
			if (gameData.render.mine.nSegRenderList [i] != -0x7fff) {
#if DBG
				if ((oldPortal->left == -1) || (oldPortal->top == -1) || (oldPortal->right == -1) || (oldPortal->bot == -1))
					Int3();
				else
#endif
					//NOTE LINK TO ABOVE!
					OglDrawFilledRect (oldPortal->left, oldPortal->top, oldPortal->right, oldPortal->bot);
				}
			}
		}
#endif //CLEAR_WINDOW

gameStates.render.bFullBright = automap.m_bDisplay && gameOpts->render.automap.bBright;
gameStates.ogl.bStandardContrast = gameStates.app.bNostalgia || IsMultiGame || (gameStates.ogl.nContrast == 8);
#if SHADOWS
gameStates.ogl.bScaleLight = EGI_FLAG (bShadows, 0, 1, 0) && (gameStates.render.nShadowPass < 3) && !FAST_SHADOWS;
#else
gameStates.ogl.bScaleLight = 0;
#endif
gameStates.render.bUseCameras = USE_CAMERAS;
PROF_END(ptAux);
return !gameStates.render.cameras.bActive && (gameData.objs.viewerP->info.nType != OBJ_ROBOT);
}

//------------------------------------------------------------------------------

void RenderSkyBoxObjects (void)
{
PROF_START
	short		i, nObject;
	short		*segNumP;

gameStates.render.nType = 1;
for (i = gameData.segs.skybox.ToS (), segNumP = gameData.segs.skybox.Buffer (); i; i--, segNumP++)
	for (nObject = SEGMENTS [*segNumP].m_objects; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg)
		DoRenderObject (nObject, gameStates.render.nWindow);
PROF_END(ptRenderObjects)
}

//------------------------------------------------------------------------------

void RenderSkyBox (int nWindow)
{
PROF_START
if (gameStates.render.bHaveSkyBox && (!automap.m_bDisplay || gameOpts->render.automap.bSkybox)) {
	glDepthMask (1);
	if (RENDERPATH)
		RenderSkyBoxFaces ();
	else {
			int	i, bFullBright = gameStates.render.bFullBright;
			short	*segP;

		gameStates.render.nType = 4;
		gameStates.render.bFullBright = 1;
		for (i = gameData.segs.skybox.ToS (), segP = gameData.segs.skybox.Buffer (); i; i--, segP++)
			RenderSegmentFaces (*segP, nWindow);
		gameStates.render.bFullBright = bFullBright;
		}
	RenderSkyBoxObjects ();
	}
PROF_END(ptRenderPass)
}

//------------------------------------------------------------------------------

void RenderMineObjects (int nType)
{
	int	nListPos, nSegLights = 0;
	short	nSegment;

#if DBG
if (!gameOpts->render.debug.bObjects)
	return;
#endif
if ((nType < 1) || (nType > 2))
	return;
gameStates.render.nState = 1;
for (nListPos = gameData.render.mine.nRenderSegs; nListPos; ) {
	nSegment = gameData.render.mine.nSegRenderList [--nListPos];
	if (nSegment < 0) {
		if (nSegment == -0x7fff)
			continue;
		nSegment = -nSegment - 1;
		}
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (0 > gameData.render.mine.renderObjs.ref [nSegment]) 
		continue;
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	if (nType == 1) {	// render opaque objects
#if DBG
		if (nSegment == nDbgSeg)
			nSegment = nSegment;
#endif
		if (gameStates.render.bUseDynLight && !gameStates.render.bQueryCoronas) {
			nSegLights = lightManager.SetNearestToSegment (nSegment, -1, 0, 1, 0);
			lightManager.SetNearestStatic (nSegment, 1, 1, 0);
			gameStates.render.bApplyDynLight = gameOpts->ogl.bLightObjects;
			}
		else
			gameStates.render.bApplyDynLight = 0;
		RenderObjList (nListPos, gameStates.render.nWindow);
		if (gameStates.render.bUseDynLight && !gameStates.render.bQueryCoronas) {
			lightManager.ResetNearestStatic (nSegment, 0);
			}
		gameStates.render.bApplyDynLight = (gameStates.render.nLightingMethod != 0);
		//lightManager.Index (0)[0].nActive = gameData.render.lights.dynamic.shader.iStaticLights [0];
		}
	else if (nType == 2)	// render objects containing transparency, like explosions
		RenderObjList (nListPos, gameStates.render.nWindow);
	}	
gameStates.render.nState = 0;
}

//------------------------------------------------------------------------------

inline int RenderSegmentList (int nType, int bFrontToBack)
{
PROF_START
gameStates.render.nType = nType;
if (!(EGI_FLAG (bShadows, 0, 1, 0) && FAST_SHADOWS && !gameOpts->render.shadows.bSoft && (gameStates.render.nShadowPass >= 2))) {
	BumpVisitedFlag ();
	if (RENDERPATH == 1)
		RenderFaceList (nType);
	else {
		int nListPos;

		if (bFrontToBack)
			for (nListPos = 0; nListPos < gameData.render.mine.nRenderSegs; )
				RenderSegment (nListPos++);
		else
			for (nListPos = gameData.render.mine.nRenderSegs; nListPos; )
				RenderSegment (--nListPos);
		}
	OglClearError (0);
	}
RenderMineObjects (nType);
lightManager.ResetAllUsed (1, 0);
if (gameStates.app.bMultiThreaded)
	lightManager.ResetAllUsed (1, 1);
OglClearError (0);
PROF_END(ptRenderPass)
return 1;
}

//------------------------------------------------------------------------------

void UpdateSlidingFaces (void)
{
	CSegFace		*faceP;
	short			h, k, nOffset;
	tTexCoord2f	*texCoordP, *ovlTexCoordP;
	tUVL			*uvlP;

for (faceP = FACES.slidingFaces; faceP; faceP = faceP->nextSlidingFace) {
#if DBG
	if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		faceP = faceP;
#endif
	texCoordP = FACES.texCoord + faceP->nIndex;
	ovlTexCoordP = FACES.ovlTexCoord + faceP->nIndex;
	uvlP = SEGMENTS [faceP->nSegment].m_sides [faceP->nSide].m_uvls;
	nOffset = faceP->nType == SIDE_IS_TRI_13;
	if (gameStates.render.bTriangleMesh) {
		static short nTriVerts [2][6] = {{0,1,2,0,2,3},{0,1,3,1,2,3}};
		for (h = 0; h < 6; h++) {
			k = nTriVerts [nOffset][h];
			texCoordP [h].v.u = X2F (uvlP [k].u);
			texCoordP [h].v.v = X2F (uvlP [k].v);
			RotateTexCoord2f (ovlTexCoordP [h], texCoordP [h], faceP->nOvlOrient);
			}
		}
	else {
		for (h = 0; h < 4; h++) {
			texCoordP [h].v.u = X2F (uvlP [(h + nOffset) % 4].u);
			texCoordP [h].v.v = X2F (uvlP [(h + nOffset) % 4].v);
			RotateTexCoord2f (ovlTexCoordP [h], texCoordP [h], faceP->nOvlOrient);
			}
		}
	}
}

//------------------------------------------------------------------------------
//renders onto current canvas

extern int bLog;

void RenderMine (short nStartSeg, fix nEyeOffset, int nWindow)
{
PROF_START
#if DBG
if (nWindow)
	nWindow = nWindow;
else
	nWindow = nWindow;
if (gameStates.app.bNostalgia) {
	gameOptions [1].render.debug.bWireFrame = 0;
	gameOptions [1].render.debug.bTextures = 1;
	gameOptions [1].render.debug.bObjects = 1;
	gameOptions [1].render.debug.bWalls = 1;
	gameOptions [1].render.debug.bDynamicLight = 1;
	}
#endif

#if 0
if (gameStates.app.bNostalgia > 1)
	gameStates.render.nLightingMethod =
	gameStates.render.bPerPixelLighting = 0;
else if (!(lightmapManager.HaveLightmaps ()))
	gameStates.render.bPerPixelLighting = 0;
else {
	if (gameStates.render.nLightingMethod == 2)
		gameStates.render.bPerPixelLighting = 2;
	else if ((gameStates.render.nLightingMethod == 1) && gameOpts->render.bUseLightmaps)
		gameStates.render.bPerPixelLighting = 1;
	else
		gameStates.render.bPerPixelLighting = 0;
	}
#endif

gameStates.ogl.bHaveDepthBuffer =
gameData.render.nTotalFaces =
gameData.render.nTotalObjects =
gameData.render.nTotalSprites =
gameData.render.nTotalLights =
gameData.render.nMaxLights =
gameData.render.nStateChanges =
gameData.render.nShaderChanges = 0;
SetFaceDrawer (-1);
gameData.render.vertColor.bNoShadow = !FAST_SHADOWS && (gameStates.render.nShadowPass == 4);
gameData.render.vertColor.bDarkness = IsMultiGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [IsMultiGame].bDarkness;
gameStates.render.bApplyDynLight =
gameStates.render.bUseDynLight = SHOW_DYN_LIGHT;
if (!EGI_FLAG (bPowerupLights, 0, 0, 0))
	gameData.render.nPowerupFilter = 0;
else if (gameStates.render.bPerPixelLighting == 2)
	gameData.render.nPowerupFilter = 1;
else
	gameData.render.nPowerupFilter = 2;
gameStates.render.bDoCameras = extraGameInfo [0].bUseCameras &&
									    (!IsMultiGame || (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bUseCameras)) &&
										 !gameStates.render.cameras.bActive;
gameStates.render.bDoLightmaps = 0;
gameStates.ogl.fLightRange = fLightRanges [IsMultiGame ? 1 : extraGameInfo [IsMultiGame].nLightRange];
PROF_END(ptAux)

if ((gameStates.render.nRenderPass <= 0) && (gameStates.render.nShadowPass < 2)) {
	gameData.render.mine.bSetAutomapVisited = BeginRenderMine (nStartSeg, nEyeOffset, nWindow);

	if (RENDERPATH) {
		lightManager.ResetSegmentLights ();
		if (!gameStates.app.bMultiThreaded || gameStates.render.bPerPixelLighting ||
			 (CountRenderFaces () < 16) || !RunRenderThreads (rtComputeFaceLight)) {
			if (gameStates.render.bTriangleMesh || !gameStates.render.bApplyDynLight || (gameData.render.mine.nRenderSegs < gameData.segs.nSegments))
				ComputeFaceLight (0, gameData.render.mine.nRenderSegs, 0);
			else if (gameStates.app.bEndLevelSequence < EL_OUTSIDE)
				ComputeFaceLight (0, gameData.segs.nFaces, 0);
			else
				ComputeFaceLight (0, gameData.segs.nSegments, 0);
			}
		PROF_START
		UpdateSlidingFaces ();
		PROF_END(ptAux);
		if ((gameStates.render.bPerPixelLighting == 2) && !gameData.app.nFrameCount)
			meshBuilder.BuildVBOs ();

		transparencyRenderer.InitBuffer (gameData.render.zMin, gameData.render.zMax);
		gameStates.render.bHeadlights = gameOpts->ogl.bHeadlight && lightManager.Headlights ().nLights && 
												  !(gameStates.render.bFullBright || automap.m_bDisplay);
		}
	}

RenderSegmentList (0, 1);	// render opaque geometry
if ((gameOpts->render.bDepthSort < 1) && !RENDERPATH)
	RenderSkyBox (nWindow);
RenderSegmentList (1, 1);		// render objects
if (!EGI_FLAG (bShadows, 0, 1, 0) || (gameStates.render.nShadowPass == 1)) {
	if (!gameData.app.nFrameCount || gameData.render.nColoredFaces) {
		glDepthFunc (GL_LEQUAL);
		RenderSegmentList (2, 1);	// render transparent geometry
		glDepthFunc (GL_LESS);
		}
#if 0
	RenderEffects (nWindow);
#endif
	if (!gameStates.app.bNostalgia &&
		 (!automap.m_bDisplay || gameOpts->render.automap.bCoronas) && gameOpts->render.coronas.bUse) {
 		glEnable (GL_TEXTURE_2D);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthFunc (GL_LEQUAL);
		if (!nWindow) {
			glDepthMask (0);
			RenderSegmentList (3, 1);
			glDepthMask (1);
			}
		glDepthFunc (GL_LESS);
		glDisable (GL_TEXTURE_2D);
		}
	glDepthFunc (GL_LESS);
	}
gameData.app.nMineRenderCount++;
PROF_END(ptRenderMine);
}

//------------------------------------------------------------------------------
// eof
