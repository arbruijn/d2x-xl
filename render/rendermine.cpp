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
#include "cockpit.h"
#include "radar.h"
#include "objrender.h"
#include "textures.h"
#include "screens.h"
#include "segpoint.h"
#include "texmerge.h"
#include "physics.h"
#include "segmath.h"
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

#if USE_OPENMP
#	define PERSISTENT_THREADS 1
#else
#	define PERSISTENT_THREADS 0
#endif

// ------------------------------------------------------------------------------

#define CLEAR_WINDOW	0

int	nClearWindow = 0; //2	// 1 = Clear whole background tPortal, 2 = clear view portals into rest of world, 0 = no clear

void RenderSkyBox (int nWindow);

//------------------------------------------------------------------------------

extern int bLog;

CCanvas *reticleCanvas = NULL;

void _CDECL_ FreeReticleCanvas (void)
{
if (reticleCanvas) {
	PrintLog (0, "unloading reticle data\n");
	reticleCanvas->Destroy ();
	reticleCanvas = NULL;
	}
}

//------------------------------------------------------------------------------

//cycle the flashing light for when mine destroyed
void FlashFrame (void)
{
	static fixang flashAngle = 0;

if (automap.Display ())
	return;
if (!(gameData.reactor.bDestroyed || gameStates.gameplay.seismic.nMagnitude)) {
	gameStates.render.nFlashScale = 0;
	return;
	}
if (gameStates.app.bEndLevelSequence)
	return;
if (paletteManager.BlueEffect () > 10)		//whiting out
	return;
//	flashAngle += FixMul(FLASH_CYCLE_RATE, gameData.time.xFrame);
if (gameStates.gameplay.seismic.nMagnitude) {
	fix xAddedFlash = abs(gameStates.gameplay.seismic.nMagnitude);
	if (xAddedFlash < I2X (1))
		xAddedFlash *= 16;
	flashAngle += (fixang) FixMul (gameStates.render.nFlashRate, FixMul(gameData.time.xFrame, xAddedFlash+I2X (1)));
	FixFastSinCos (flashAngle, &gameStates.render.nFlashScale, NULL);
	gameStates.render.nFlashScale = (gameStates.render.nFlashScale + I2X (3)) / 4;	//	gets in range 0.5 to 1.0
	}
else {
	flashAngle += (fixang) FixMul (gameStates.render.nFlashRate, gameData.time.xFrame);
	FixFastSinCos (flashAngle, &gameStates.render.nFlashScale, NULL);
	gameStates.render.nFlashScale = (gameStates.render.nFlashScale + I2X (1)) / 2;
	if (gameStates.app.nDifficultyLevel == 0)
		gameStates.render.nFlashScale = (gameStates.render.nFlashScale + I2X (3)) / 4;
	}
}

//------------------------------------------------------------------------------

void DoRenderObject (int nObject, int nWindow)
{
#if DBG
if (!(IsMultiGame || gameOpts->render.debug.bObjects))
	return;
#endif
#if 0 //DBG
if (gameData.render.mine.bObjectRendered [nObject] == gameStates.render.nFrameCount) 
	return;
#endif

CObject*	objP = OBJECTS + nObject;

#if DBG
if (nWindow && (objP->info.nType == OBJ_WEAPON))
	nWindow = nWindow;
#endif

if (gameData.demo.nState == ND_STATE_PLAYBACK) {
	if ((nDemoDoingLeft == 6 || nDemoDoingRight == 6) && objP->info.nType == OBJ_PLAYER) {
  		return;
		}
	}

if (RenderObject (objP, nWindow, 0)) {
#if 0 //DBG
	gameData.render.mine.bObjectRendered [nObject] = gameStates.render.nFrameCount;
#endif
	if (!gameStates.render.cameras.bActive) {
		tWindowRenderedData*	wrd = windowRenderedData + nWindow;
		int nType = objP->info.nType;
		if ((nType == OBJ_ROBOT) || (nType == OBJ_PLAYER) ||
			 ((nType == OBJ_WEAPON) && (objP->IsPlayerMine () || (objP->IsMissile () && EGI_FLAG (bKillMissiles, 0, 0, 0))))) {
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
for (i = gameData.render.mine.objRenderList.ref [nSegment], j = 0; i >= 0; i = pi->nNextItem) {
	pi = gameData.render.mine.objRenderList.objs + i;
	objRenderList [j++] = *pi;
#if DBG
	if (OBJECTS [pi->nObject].info.nSegment != nSegment)
		nDbgSeg = nDbgSeg;
#endif
	}
#if 1
if (j > 1)
	QSortObjRenderList (0, j - 1);
#endif
return j;
}

//------------------------------------------------------------------------------

static int nDbgListPos = -1;

void RenderObjList (int nListPos, int nWindow)
{
PROF_START
	int i, j;
	int saveLinDepth = gameStates.render.detail.nMaxLinearDepth;

nDbgListPos = nListPos;
gameStates.render.detail.nMaxLinearDepth = gameStates.render.detail.nMaxLinearDepthObjects;
for (i = 0, j = SortObjList (gameData.render.mine.objRenderSegList [nListPos]); i < j; i++)
	DoRenderObject (objRenderList [i].nObject, nWindow);	// note link to above else
gameStates.render.detail.nMaxLinearDepth = saveLinDepth;
PROF_END(ptRenderObjects)
}

//------------------------------------------------------------------------------

void RenderSkyBoxObjects (void)
{
PROF_START
	short		i, nObject;
	short		*segNumP;

gameStates.render.nType = RENDER_TYPE_OBJECTS;
gameStates.render.nState = 1;
for (i = gameData.segs.skybox.ToS (), segNumP = gameData.segs.skybox.Buffer (); i; i--, segNumP++)
	for (nObject = SEGMENTS [*segNumP].m_objects; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg)
		DoRenderObject (nObject, gameStates.render.nWindow [0]);
PROF_END(ptRenderObjects)
}

//------------------------------------------------------------------------------

void RenderSkyBox (int nWindow)
{
PROF_START
if (gameStates.render.bHaveSkyBox && (!automap.Display () || gameOpts->render.automap.bSkybox)) {
	ogl.SetDepthWrite (true);
	RenderSkyBoxFaces ();
	}
PROF_END(ptRenderPass)
}

//------------------------------------------------------------------------------

void RenderObjectsST (void)
{
	short nSegment;

for (int i = 0; i < gameData.render.mine.nObjRenderSegs; i++) {
	nSegment = gameData.render.mine.objRenderSegList [i];
#if DBG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	if (gameStates.render.bApplyDynLight) {
		lightManager.SetNearestToSegment (nSegment, -1, 0, 1, 0);
		//lightManager.ResetNearestStatic (nSegment, 0);
		lightManager.SetNearestStatic (nSegment, 1, 0);
		}
	RenderObjList (i, gameStates.render.nWindow [0]);
	if (gameStates.render.bApplyDynLight)
		lightManager.ResetNearestStatic (nSegment, 0);
	}	
}

//------------------------------------------------------------------------------
// The following pragmas keep the Visual C++ 9 optimizer from trying to inline
// the following thread function.

#ifdef _MSC_VER
#	pragma optimize("ga", off)
#	pragma auto_inline(off)
#endif

static sbyte bSemaphore [MAX_THREADS] = {0,0,0,0,0,0,0,0};
static SDL_mutex* threadLock = NULL;
static int nThreads = 0;

//------------------------------------------------------------------------------
// LightObjectsThread computes the segment lighting for the object render process.
// It will set its semaphore to 1 after lighting for a segment has been computed to
// tell the render process that this segment's objects can be rendered, and waits
// for the render process to reset its semaphore before proceeding with the next 
// segment.
#if DBG
static int nDbgThread = -1;
#endif

int _CDECL_ LightObjectsThread (void* nThreadP)
{
#if 1
	int	nThread = *((int*) nThreadP);
#endif
	short nSegment;

#if PERSISTENT_THREADS
for (;;) {
	while (bSemaphore [nThread] < 0)
		G3_SLEEP (1);
#endif
	for (int i = nThread; i < gameData.render.mine.nObjRenderSegs; i += gameStates.app.nThreads - 1) {
		nSegment = gameData.render.mine.objRenderSegList [i];
		if (gameStates.render.bApplyDynLight) {
			lightManager.SetNearestToSegment (nSegment, -1, 0, 1, nThread);
			lightManager.SetNearestStatic (nSegment, 1, nThread);
			}
		bSemaphore [nThread] = 1;
		while (bSemaphore [nThread])
			G3_SLEEP (0);
		if (gameStates.render.bApplyDynLight)
			lightManager.ResetNearestStatic (nSegment, nThread);
		}	
	SDL_mutexP (threadLock);
	nThreads--;
	SDL_mutexV (threadLock);
#if PERSISTENT_THREADS
	bSemaphore [nThread] = -1;
	}
#endif
return 1;
}

//------------------------------------------------------------------------------
// RenderObjectsMT is the object rendering function. It must reside in the main
// process because only the main process has a valid OpenGL context. It communicates
// with the object lighting threads via a semaphore. If a lighting threads semaphore
// is set, that thread has finished lighting calculations for its current segment
// and all objects in that segment can be rendered. The lighting thread waits for 
// semaphore to be reset by the render process after the objects have been rendered.
// Since the object render process needs to run full throttle, only gameStates.app.nThreads - 1
// lighting threads are executed.
// Note: SDL's semaphore/mutex handling doesn't really cut it here (and is too slow).

void RenderObjectsMT (void)
{
	int	i, nListPos [MAX_THREADS];
	
for (i = 0; i < MAX_THREADS; i++)
 nListPos [i] = i;

i = 0;
while (nThreads > 0) {
	if (bSemaphore [i] > 0) {
		lightManager.SetThreadId (i);
		RenderObjList (nListPos [i], gameStates.render.nWindow [0]);
		lightManager.SetThreadId (-1);
		nListPos [i] += (gameStates.app.nThreads - 1);
		bSemaphore [i] = 0;
		}
	i = (i + 1) % (gameStates.app.nThreads - 1);
	}
}

#ifdef _MSC_VER
#	pragma optimize("ga", on)
#	pragma auto_inline(on)
#endif

//------------------------------------------------------------------------------

#if DBG

static inline int VerifyObjectRenderSegment (short nSegment)
{
for (int i = 0; i < gameData.render.mine.nObjRenderSegs; i++)
	if (gameData.render.mine.objRenderSegList [i] == nSegment)
		return -1;
return nSegment;
}

#else

#	define VerifyObjectRenderSegment(_nSegment) (_nSegment)


#endif

//------------------------------------------------------------------------------

static inline int ObjectRenderSegment (int i)
{
if (i >= gameData.render.mine.visibility [0].nSegments)
	return -1;
short nSegment = gameData.render.mine.visibility [0].segments [i];
if (nSegment < 0) {
	if (nSegment == -0x7fff)
		return -1;
	nSegment = -nSegment - 1;
	}
if (0 > gameData.render.mine.objRenderList.ref [nSegment])
	return -1;
if (!automap.Display ())
	return VerifyObjectRenderSegment (nSegment);
if (extraGameInfo [IsMultiGame].bPowerupsOnRadar && extraGameInfo [IsMultiGame].bRobotsOnRadar)
	return VerifyObjectRenderSegment (nSegment);

tObjRenderListItem* pi;

for (i = gameData.render.mine.objRenderList.ref [nSegment]; i >= 0; i = pi->nNextItem) {
	pi = gameData.render.mine.objRenderList.objs + i;
	int nType = OBJECTS [pi->nObject].info.nType;
	if (nType == OBJ_POWERUP) {
		if (extraGameInfo [IsMultiGame].bPowerupsOnRadar)
			return VerifyObjectRenderSegment (nSegment);
		}
	else if (nType == OBJ_ROBOT) {
		if (extraGameInfo [IsMultiGame].bRobotsOnRadar)
			return VerifyObjectRenderSegment (nSegment);
		}
	else
		return VerifyObjectRenderSegment (nSegment);
	}

return -1;
}

//------------------------------------------------------------------------------

void RenderMineObjects (int nType)
{
#if DBG
if (!gameOpts->render.debug.bObjects)
	return;
#endif
if (nType != RENDER_TYPE_GEOMETRY)
	return;
if (automap.Display () && !(gameOpts->render.automap.bTextured & 1))
	return;

gameStates.render.nType = RENDER_TYPE_OBJECTS;
gameStates.render.nState = 1;
gameStates.render.bApplyDynLight = gameStates.render.bUseDynLight && gameOpts->ogl.bLightObjects && !gameStates.render.bFullBright;

	int	i;
	short nSegment;

gameData.render.mine.nObjRenderSegs = 0;
for (i = 0; i < gameData.render.mine.visibility [0].nSegments; i++)
	if (0 <= (nSegment = ObjectRenderSegment (i)))
		gameData.render.mine.objRenderSegList [gameData.render.mine.nObjRenderSegs++] = nSegment;

#if 0

RenderObjectsST ();

#else


if (!gameStates.app.bMultiThreaded || (gameStates.render.nShadowPass == 2) || (gameStates.app.nThreads < 3) || (gameData.render.mine.nObjRenderSegs < 2 * (gameStates.app.nThreads - 1)))
	RenderObjectsST ();
else {
#if USE_OPENMP // > 1
	if (!threadLock)
		threadLock = SDL_CreateMutex ();
	nThreads = gameStates.app.nThreads - 1;
	memset (bSemaphore, 0, sizeof (bSemaphore));
	int nThreadIds [MAX_THREADS];
#if PERSISTENT_THREADS
	static bool bInit = true;
	static SDL_Thread* threads [MAX_THREADS];
	if (bInit) {
		bInit = false;
		memset (threads, 0, sizeof (threads));
		}
#else
	SDL_Thread* threads [MAX_THREADS];
#endif
	for (i = 0; i < MAX_THREADS; i++) 
		nThreadIds [i] = i;
	for (i = 0; i < nThreads; i++) 
#if PERSISTENT_THREADS
		if (!threads [i])
#endif
		threads [i] = SDL_CreateThread (LightObjectsThread, nThreadIds + i);
	RenderObjectsMT ();
#if !PERSISTENT_THREADS
	for (i = 0; i < gameStates.app.nThreads; i++) 
		SDL_WaitThread (threads [i], NULL);
#endif
#elif !USE_OPENMP
	if (!threadLock)
		threadLock = SDL_CreateMutex ();
	nThreads = gameStates.app.nThreads - 1;
	memset (bSemaphore, 0, sizeof (bSemaphore));
	RunRenderThreads (-int (rtPolyModel) - 1, nThreads);
	RenderObjectsMT ();
#else
RenderObjectsST ();
#endif
	}

#endif

gameStates.render.bApplyDynLight = (gameStates.render.nLightingMethod != 0);
gameStates.render.nState = 0;
}

//------------------------------------------------------------------------------

int RenderSegmentList (int nType)
{
PROF_START

gameStates.render.nType = nType;
#if MAX_SHADOWMAPS
#	if MAX_SHADOWMAPS > 0
if (gameStates.render.nShadowMap == 0) 
#endif
	{
#else
if (!(EGI_FLAG (bShadows, 0, 1, 0) && FAST_SHADOWS && !gameOpts->render.shadows.bSoft && (gameStates.render.nShadowPass >= 2))) {
#endif
	gameData.render.mine.visibility [0].BumpVisitedFlag ();
	RenderFaceList (nType);
	ogl.ClearError (0);
	}
#if MAX_SHADOWMAPS >= 0
RenderMineObjects (nType);
#endif
for (int i = 0; i < gameStates.app.nThreads; i++)
	lightManager.ResetAllUsed (1, i);
ogl.ClearError (0);
PROF_END(ptRenderPass)
return 1;
}

//------------------------------------------------------------------------------

void RenderEffects (int nWindow)
{
PROF_START

if (!ogl.StereoDevice () || (ogl.StereoSeparation () < 0) || nWindow || gameStates.render.cameras.bActive) {
	int bLightning, bParticles, bSparks;

	if (gameStates.app.nThreads > 1) {
		while (!transparencyRenderer.Ready ())
			G3_SLEEP (0);
		}
	if (automap.Display ()) {
		bLightning = gameOpts->render.automap.bLightning;
		bParticles = gameOpts->render.automap.bParticles;
		bSparks = gameOpts->render.automap.bSparks;
		}
	else {
		bSparks = (gameOptions [0].render.nQuality > 0);
		bLightning = (!nWindow || gameOpts->render.lightning.bAuxViews) && 
						  (!gameStates.render.cameras.bActive || gameOpts->render.lightning.bMonitors);
		bParticles = (!nWindow || gameOpts->render.particles.bAuxViews) &&
						 (!gameStates.render.cameras.bActive || gameOpts->render.particles.bMonitors);
		}

	if (bSparks) 
		sparkManager.Render ();
	if (bParticles) {
		//particleManager.Cleanup ();
		particleManager.Render ();
		}
	if (bLightning) 
		lightningManager.Render ();
	}
transparencyRenderer.Render (nWindow);

PROF_END(ptEffects)
}

//------------------------------------------------------------------------------

int bHave3DCockpit = -1;

void RenderCockpitModel (void)
{
	static int bCockpit = 1;
	static float yOffset = 5.0f;

if (bCockpit && bHave3DCockpit && (gameStates.render.cockpit.nType == CM_FULL_COCKPIT)) {
	int bFullBright = gameStates.render.bFullBright;
	gameStates.render.bFullBright = 1;
	ogl.SetTransform (1);
	CFixVector vOffset = /*OBJECTS [0].Orientation ().m.dir.f * F2X (xOffset) +*/ OBJECTS [0].Orientation ().m.dir.u * F2X (yOffset);
	OBJECTS [0].Position () -= vOffset;
	ogl.SetViewport (0, 0, screen.Width (), screen.Height ());
	gameData.models.vScale.Set (F2X (float (screen.Width ()) / float (screen.Height ()) * 0.75f), I2X (1), I2X (1));
	bHave3DCockpit = (G3RenderModel (&OBJECTS [0], COCKPIT_MODEL, -1, NULL, gameData.models.textures, NULL, NULL, 0, NULL, NULL) > 0);
	gameData.models.vScale.Set (I2X (1), I2X (1), I2X (1));
	CCanvas::Current ()->SetViewport ();
	OBJECTS [0].Position () += vOffset;
	ogl.SetTransform (0);
	gameStates.render.bFullBright = bFullBright;
	}
}

//------------------------------------------------------------------------------

void RenderSegmentOutline (void)
{
ogl.SetCullMode ((gameStates.render.bRearView < 0) ? GL_BACK : GL_FRONT);
glPolygonMode (GL_BACK, GL_LINE);       // Draw Backfacing Polygons As Wireframes
glLineWidth (4.0);         // Set The Line Width
ogl.SetDepthMode (GL_LEQUAL);            // Change The Depth Mode
glColor3f (0, 0, 0);   
RenderSegmentList (RENDER_TYPE_OUTLINE);
ogl.SetCullMode ((gameStates.render.bRearView < 0) ? GL_FRONT : GL_BACK);
glPolygonMode (GL_BACK, GL_FILL);       // Draw Backfacing Polygons As Wireframes
}

//------------------------------------------------------------------------------
//renders onto current canvas

extern int bLog;

void RenderMine (short nStartSeg, fix xStereoSeparation, int nWindow)
{
PROF_START
SetupMineRenderer (nWindow);
PROF_END(ptAux)
ComputeMineLighting (nStartSeg, xStereoSeparation, nWindow);
#if 0
++gameStates.render.bFullBright;
RenderSegmentList (RENDER_TYPE_ZCULL);	// render depth only
--gameStates.render.bFullBright;
#endif
#if 0 //DBG
RenderCockpitModel ();
#endif
#if 1
RenderSkyBoxObjects ();
RenderSegmentList (RENDER_TYPE_GEOMETRY);
//RenderSegmentOutline ();
if (!(EGI_FLAG (bShadows, 0, 1, 0) && (gameStates.render.nShadowMap > 0))) {
	if (!gameStates.app.bNostalgia &&
		 (!automap.Display () || gameOpts->render.automap.bCoronas) && gameOpts->render.effects.bEnabled && gameOpts->render.coronas.bUse) 
		if (!nWindow && SetupCoronas ())
			RenderSegmentList (RENDER_TYPE_CORONAS);
	}
#endif
gameData.app.nMineRenderCount++;
PROF_END(ptRenderMine);
}

//------------------------------------------------------------------------------
// eof
