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

void SetRenderView (fix xStereoSeparation, short *nStartSegP, int bOglScale)
{
	short nStartSeg;
	bool	bPlayer = gameData.objs.viewerP == gameData.objs.consoleP;

gameData.render.mine.viewerEye = gameData.objs.viewerP->info.position.vPos;
if (xStereoSeparation && bPlayer)
	gameData.render.mine.viewerEye += gameData.objs.viewerP->info.position.mOrient.RVec () * xStereoSeparation;

externalView.SetPos (NULL);
if (gameStates.render.cameras.bActive) {
	nStartSeg = gameData.objs.viewerP->info.nSegment;
	G3SetViewMatrix (gameData.render.mine.viewerEye, gameData.objs.viewerP->info.position.mOrient, gameStates.render.xZoom, bOglScale, xStereoSeparation);
	}
else {
	if (!gameStates.render.nWindow && (bPlayer))
		externalView.SetPoint (gameData.objs.viewerP);
	if ((bPlayer) && transformation.m_info.bUsePlayerHeadAngles) {
		CFixMatrix mHead = CFixMatrix::Create (transformation.m_info.playerHeadAngles);
		CFixMatrix mView = gameData.objs.viewerP->info.position.mOrient * mHead;
		G3SetViewMatrix (gameData.render.mine.viewerEye, mView, gameStates.render.xZoom, bOglScale, xStereoSeparation);
		}
	else if (gameStates.render.bRearView && (bPlayer)) {
#if 1
		CFixMatrix mView;

		mView = gameData.objs.viewerP->info.position.mOrient;
		mView.FVec ().Neg ();
		mView.RVec ().Neg ();
#else
		CFixMatrix mHead, mView;

		transformation.m_info.playerHeadAngles [PA] = 0;
		transformation.m_info.playerHeadAngles [BA] = 0x7fff;
		transformation.m_info.playerHeadAngles [HA] = 0x7fff;
		VmAngles2Matrix (&mHead, &transformation.m_info.playerHeadAngles);
		VmMatMul (&mView, &gameData.objs.viewerP->info.position.mOrient, &mHead);
#endif
		G3SetViewMatrix (gameData.render.mine.viewerEye, mView,  //gameStates.render.xZoom, bOglScale);
							  FixDiv (gameStates.render.xZoom, gameStates.zoom.nFactor), bOglScale, xStereoSeparation);
		}
	else if ((bPlayer) && (!IsMultiGame || gameStates.app.bHaveExtraGameInfo [1])) {
		gameStates.zoom.nMinFactor = I2X (gameStates.render.glAspect); 
		gameStates.zoom.nMaxFactor = gameStates.zoom.nMinFactor * 5;
		HandleZoom ();
		if ((bPlayer) &&
#if DBG
			 gameStates.render.bChaseCam) {
#else
			 gameStates.render.bChaseCam && (!IsMultiGame || IsCoopGame || (EGI_FLAG (bEnableCheats, 0, 0, 0) && !COMPETITION))) {
#endif
			externalView.GetViewPoint ();
			if (xStereoSeparation)
				gameData.render.mine.viewerEye += gameData.objs.viewerP->info.position.mOrient.RVec () * xStereoSeparation;
			G3SetViewMatrix (gameData.render.mine.viewerEye,
								  externalView.GetPos () ? externalView.GetPos ()->mOrient : gameData.objs.viewerP->info.position.mOrient,
								  gameStates.render.xZoom, bOglScale, xStereoSeparation);
			}
		else
			G3SetViewMatrix (gameData.render.mine.viewerEye, gameData.objs.viewerP->info.position.mOrient,
								  FixDiv (gameStates.render.xZoom, gameStates.zoom.nFactor), bOglScale, xStereoSeparation);
		}
	else
		G3SetViewMatrix (gameData.render.mine.viewerEye, gameData.objs.viewerP->info.position.mOrient,
							  gameStates.render.xZoom, bOglScale, xStereoSeparation);
	if (!nStartSegP)
		nStartSeg = gameStates.render.nStartSeg;
	else if (0 > (nStartSeg = FindSegByPos (gameData.render.mine.viewerEye, gameData.objs.viewerP->info.nSegment, 1, 0)))
		nStartSeg = gameData.objs.viewerP->info.nSegment;
	}
if (nStartSegP)
	*nStartSegP = nStartSeg;
}

//------------------------------------------------------------------------------

int BeginRenderMine (short nStartSeg, fix xStereoSeparation, int nWindow)
{
PROF_START
if (!nWindow)
	GetPlayerMslLock ();	// uses rendered object info from previous frame stored in windowRenderedData
if (!gameStates.render.cameras.bActive)
	windowRenderedData [nWindow].nObjects = 0;
ogl.m_states.fAlpha = FADE_LEVELS;
if (((gameStates.render.nRenderPass <= 0) && (gameStates.render.nShadowPass < 2) && (gameStates.render.nShadowBlurPass < 2)) || gameStates.render.bShadowMaps) {
	if (!automap.Display ())
		RenderStartFrame ();
	}
if ((gameStates.render.nRenderPass <= 0) && (gameStates.render.nShadowPass < 2)) {
	ogl.SetTransform (1);
	BuildRenderSegList (nStartSeg, nWindow);		//fills in gameData.render.mine.nSegRenderList & gameData.render.mine.nRenderSegs
	if ((gameStates.render.nRenderPass <= 0) && (gameStates.render.nShadowPass < 2)) {
		BuildRenderObjLists (gameData.render.mine.nRenderSegs);
		if (xStereoSeparation <= 0)	// Do for left eye or zero.
			SetDynamicLight ();
		}
	ogl.SetTransform (0);
	lightManager.Transform (0, 1);
	}
gameStates.render.bFullBright = automap.Display () && gameOpts->render.automap.bBright;
ogl.m_states.bStandardContrast = gameStates.app.bNostalgia || IsMultiGame || (ogl.m_states.nContrast == 8);
ogl.m_states.bScaleLight = EGI_FLAG (bShadows, 0, 1, 0) && (gameStates.render.nShadowPass < 3) && !FAST_SHADOWS;
gameStates.render.bUseCameras = USE_CAMERAS;
PROF_END(ptAux);
return !gameStates.render.cameras.bActive && (gameData.objs.viewerP->info.nType != OBJ_ROBOT);
}

//------------------------------------------------------------------------------

void SetupMineRenderer (void)
{
#if DBG
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

ogl.m_states.bHaveDepthBuffer =
gameData.render.nUsedFaces =
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
}

//------------------------------------------------------------------------------

void ComputeMineLighting (short nStartSeg, fix xStereoSeparation, int nWindow)
{
ogl.m_states.fLightRange = fLightRanges [IsMultiGame ? 1 : extraGameInfo [IsMultiGame].nLightRange];
if ((gameStates.render.nRenderPass <= 0) && (gameStates.render.nShadowPass < 2)) {
	gameData.render.mine.bSetAutomapVisited = BeginRenderMine (nStartSeg, xStereoSeparation, nWindow);

	if (xStereoSeparation <= 0) {
		ResetFaceList ();
		gameStates.render.nThreads = GetNumThreads ();
		lightManager.ResetSegmentLights ();
		if (gameStates.render.bPerPixelLighting || (CountRenderFaces () < 16) || (gameStates.app.nThreads < 2)
#	if !USE_OPENMP
			 || !RunRenderThreads (rtComputeFaceLight, gameStates.app.nThreads)
#	endif
			)
			{
			gameStates.render.nThreads = 1;
			if (gameStates.render.bTriangleMesh || !gameStates.render.bApplyDynLight || (gameData.render.mine.nRenderSegs < gameData.segs.nSegments))
				ComputeFaceLight (0, gameData.render.mine.nRenderSegs, 0);
			else if (gameStates.app.bEndLevelSequence < EL_OUTSIDE)
				ComputeFaceLight (0, gameData.segs.nFaces, 0);
			else
				ComputeFaceLight (0, gameData.segs.nSegments, 0);
			}
#if USE_OPENMP > 1
		else {
				int	nStart, nEnd, nMax;

			if (gameStates.render.bTriangleMesh || !gameStates.render.bApplyDynLight || (gameData.render.mine.nRenderSegs < gameData.segs.nSegments))
				nMax = gameData.render.mine.nRenderSegs;
			else if (gameStates.app.bEndLevelSequence < EL_OUTSIDE)
				nMax = gameData.segs.nFaces;
			else
				nMax = gameData.segs.nSegments;
			if (gameStates.app.nThreads & 1) {
#			pragma omp parallel
				{
#				pragma omp for private (nStart, nEnd)
					for (int i = 0; i < gameStates.app.nThreads; i++) {
						ComputeThreadRange (i, nMax, nStart, nEnd);
						ComputeFaceLight (nStart, nEnd, i);
						}
					}
				}
			else {
				int	nPivot = gameStates.app.nThreads / 2;
#				pragma omp parallel
					{
#				pragma omp for private (nStart, nEnd)
					for (int i = 0; i < gameStates.app.nThreads; i++) {
						if (i < nPivot) {
							ComputeThreadRange (i, tiRender.nMiddle, nStart, nEnd, nPivot);
							ComputeFaceLight (nStart, nEnd, i);
							}
						else {
							ComputeThreadRange (i - nPivot, nMax - tiRender.nMiddle, nStart, nEnd, nPivot);
							ComputeFaceLight (nStart + tiRender.nMiddle, nEnd + tiRender.nMiddle, i);
							}
						}
					}
				}
			}
#endif //_OPENMP
		if ((gameStates.render.bPerPixelLighting == 2) && !gameData.app.nFrameCount)
			meshBuilder.BuildVBOs ();
		gameStates.render.bHeadlights = gameOpts->ogl.bHeadlight && lightManager.Headlights ().nLights && 
												  !(gameStates.render.bFullBright || automap.Display ());
		}
	transparencyRenderer.InitBuffer (gameData.render.zMin, gameData.render.zMax);
	}
}

//------------------------------------------------------------------------------
// eof
