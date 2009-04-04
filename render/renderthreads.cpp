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
COPYRIGHT 0993-0999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "descent.h"
#include "error.h"
#include "light.h"
#include "dynlight.h"
#include "objsmoke.h"
#include "sparkeffect.h"
#include "lightning.h"
#include "rendermine.h"
#include "fastrender.h"
#include "renderthreads.h"
#include "interp.h"
#include "lightmap.h"

#define KILL_RENDER_THREADS	0
#define TRANSPRENDER_THREADS	0

tRenderThreadInfo tiRender;
tTranspRenderThreadInfo tiTranspRender;
tThreadInfo tiEffects;

//------------------------------------------------------------------------------

bool WaitForRenderThreads (void)
{
if (gameStates.app.bMultiThreaded) {
	while (tiRender.ti [0].bExec || tiRender.ti [1].bExec)
		G3_SLEEP (0);	//already running, so wait
	return true;
	}
return false;
}

//------------------------------------------------------------------------------

int RunRenderThreads (int nTask, int nThreads)
{
#if DBG
	time_t	t0 = 0, t2 = 0;
	static	int nLockups = 0;
#endif

if (!gameStates.app.bMultiThreaded)
	return 0;
#if 0
if ((nTask < rtTaskCount) && !gameData.app.bUseMultiThreading [nTask])
	return 0;
#endif
tiRender.nTask = (tRenderTask) nTask;
tiRender.ti [0].bExec = 1;
if (nThreads == 2)
	tiRender.ti [1].bExec = 1;
#if DBG
t0 = clock ();
while ((tiRender.ti [0].bExec || tiRender.ti [1].bExec) && (clock () - t0 < 1000)) {
	G3_SLEEP (0);
	if (tiRender.ti [0].bExec != tiRender.ti [1].bExec) {
		if (!t2)
			t2 = clock ();
		else if (clock () - t2 > 33) {	//slower thread must not take more than 00 ms longer than faster one
#if 0//ndef _DEBUG
			t2 = clock ();
#else
			PrintLog ("threads locked up (task: %d)\n", nTask);
			tiRender.ti [0].bExec =
			tiRender.ti [1].bExec = 0;
			if (++nLockups > 100)
				gameStates.app.bMultiThreaded = 0;
#endif
			}
		}
	}
#else
while (tiRender.ti [0].bExec || tiRender.ti [1].bExec)
	G3_SLEEP (0);
#endif
return 1;
}

//------------------------------------------------------------------------------

int _CDECL_ RenderThread (void *pThreadId)
{
	int		nId = *reinterpret_cast<int*> (pThreadId);
#ifdef _WIN32
	HGLRC		myContext = 0;
#endif

do {
	while (!tiRender.ti [nId].bExec) {
		G3_SLEEP (0);
		if (tiRender.ti [nId].bDone)
			return 0;
		}
	if (tiRender.nTask == rtSortSegZRef) {
		if (nId)
			QSortSegZRef (gameData.render.mine.nRenderSegs / 2, gameData.render.mine.nRenderSegs - 0);
		else
			QSortSegZRef (0, gameData.render.mine.nRenderSegs / 2 - 0);
		}
	else if (tiRender.nTask == rtInitSegZRef) {
		if (nId)
			InitSegZRef (gameData.render.mine.nRenderSegs / 2, gameData.render.mine.nRenderSegs, nId);
		else
			InitSegZRef (0, gameData.render.mine.nRenderSegs / 2, nId);
		}
	else if (tiRender.nTask == rtStaticVertLight) {
		if (nId)
			lightManager.GatherStaticVertexLights (gameData.segs.nVertices / 2, gameData.segs.nVertices, nId);
		else
			lightManager.GatherStaticVertexLights (0, gameData.segs.nVertices / 2, nId);
		}
	else if (tiRender.nTask == rtComputeFaceLight) {
		if (gameStates.render.bTriangleMesh || (gameData.render.mine.nRenderSegs < gameData.segs.nSegments)) {
			if (nId)
				ComputeFaceLight (gameData.render.mine.nRenderSegs - 1, tiRender.nMiddle - 1, nId);
			else
				ComputeFaceLight (0, tiRender.nMiddle, nId);
			}
		else {
			if (nId)
				ComputeFaceLight (gameData.segs.nFaces / 2, gameData.segs.nFaces, nId);
			else
				ComputeFaceLight (0, gameData.segs.nFaces / 2, nId);
			}
		}
	else if (tiRender.nTask == rtPolyModel) {
		short	iVerts, nVerts, iFaceVerts, nFaceVerts;

		if (nId) {
			nVerts = tiRender.pm->m_nVerts;
			iVerts = nVerts / 2;
			nFaceVerts = tiRender.pm->m_nFaceVerts;
			iFaceVerts = nFaceVerts / 2;
			}
		else {
			iVerts = 0;
			nVerts = tiRender.pm->m_nVerts / 2;
			iFaceVerts = 0;
			nFaceVerts = tiRender.pm->m_nFaceVerts / 2;
			}
		G3DynLightModel (tiRender.objP, tiRender.pm, iVerts, nVerts, iFaceVerts, nFaceVerts);
		}
	else if (tiRender.nTask == rtLightmap)
		lightmapManager.Build (nId);
	tiRender.ti [nId].bExec = 0;
	} while (!tiRender.ti [nId].bDone);
#ifdef _WIN32
if (myContext)
	wglDeleteContext (myContext);
#endif
return 0;
}

//------------------------------------------------------------------------------

int _CDECL_ TranspRenderThread (void *pThreadId)
{
	int	i;

do {
	while (!(tiTranspRender.ti [0].bExec || tiTranspRender.ti [1].bExec))
		G3_SLEEP (0);
	for (i = 0; i < 2; i++) {
		if (tiTranspRender.ti [i].bExec) {
			transparencyRenderer.Add ((tTranspItemType) tiTranspRender.itemData [i].nType, 
											  &tiTranspRender.itemData [i].item, 
											  tiTranspRender.itemData [i].nSize, 
											  tiTranspRender.itemData [i].nDepth, 
											  tiTranspRender.itemData [i].nIndex);
			tiTranspRender.ti [i].bExec = 0;
			}
		}
	} while (!(tiTranspRender.ti [0].bDone && tiTranspRender.ti [1].bDone));
return 0;
}

//------------------------------------------------------------------------------

void StartTranspRenderThread (void)
{
#if TRANSPRENDER_THREADS
static bool bInitialized = false;

if (!bInitialized) {
	memset (&tiTranspRender, 0, sizeof (tiTranspRender));
	bInitialized = true;
	}
for (int i = 0; i < 2; i++) {
if (!tiTranspRender.ti [i].pThread)
	tiTranspRender.ti [i].bDone =
	tiTranspRender.ti [i].bExec = 0;
	tiTranspRender.ti [i].nId = i;
	tiTranspRender.ti [i].pThread = SDL_CreateThread (TranspRenderThread, NULL);
	}
#else
gameData.app.bUseMultiThreading [rtTranspRender] = 0;
#endif
}

//------------------------------------------------------------------------------

void EndTranspRenderThread (void)
{
#if TRANSPRENDER_THREADS
tiTranspRender.ti [0].bDone =
tiTranspRender.ti [1].bDone = 1;
G3_SLEEP (10);
for (int i = 0; i < 2; i++) {
	if (tiTranspRender.ti [i].pThread) {
		//SDL_KillThread (tiTranspRender.ti [0].pThread);
		tiTranspRender.ti [i].pThread = NULL;
		}
	}
#endif
}

//------------------------------------------------------------------------------

void ControlTranspRenderThread (void)
{
#if TRANSPRENDER_THREADS
if (gameData.app.bUseMultiThreading [rtTranspRender])
	StartTranspRenderThread ();
else
	EndTranspRenderThread ();
#else
gameData.app.bUseMultiThreading [rtTranspRender] = 0;
#endif
}

//------------------------------------------------------------------------------

void StartRenderThreads (void)
{
	static bool bInitialized = false;

if (!bInitialized) {
	memset (&tiRender, 0, sizeof (tiRender));
	bInitialized = true;
	}
for (int i = 0; i < 2; i++) {
	if (!tiRender.ti [i].pThread) {
		tiRender.ti [i].bDone =
		tiRender.ti [i].bExec = 0;
		tiRender.ti [i].nId = i;
		tiRender.ti [i].pThread = SDL_CreateThread (RenderThread, &tiRender.ti [i].nId);
		}
	}
}

//------------------------------------------------------------------------------

void ControlRenderThreads (void)
{
StartRenderThreads ();
}

//------------------------------------------------------------------------------

void EndRenderThreads (void)
{
tiRender.ti [0].bDone =
tiRender.ti [1].bDone = 1;
G3_SLEEP (10);
for (int i = 0; i < 2; i++) {
	if (tiRender.ti [i].pThread) {
		//SDL_KillThread (tiRender.ti [0].pThread);
		tiRender.ti [i].pThread = NULL;
		}
	}
EndTranspRenderThread ();
EndEffectsThread ();
}

//------------------------------------------------------------------------------

int _CDECL_ EffectsThread (void *pThreadId)
{
do {
	while (!tiEffects.bExec) {
		G3_SLEEP (0);
		if (tiEffects.bDone) {
			tiEffects.bDone = 0;
			return 0;
			}
		}
	DoParticleFrame ();
	lightningManager.DoFrame ();
	sparkManager.DoFrame ();
	tiEffects.bExec = 0;
	} while (!tiEffects.bDone);
tiEffects.bDone = 0;
return 0;
}

//------------------------------------------------------------------------------

void StartEffectsThread (void)
{
static bool bInitialized = false;

if (!bInitialized) {
	memset (&tiEffects, 0, sizeof (tiEffects));
	bInitialized = true;
	}
tiEffects.bDone = 0;
tiEffects.bExec = 0;
if	(!(tiEffects.pThread || (tiEffects.pThread = SDL_CreateThread (EffectsThread, NULL))))
	gameData.app.bUseMultiThreading [rtEffects] = 0;
}

//------------------------------------------------------------------------------

void EndEffectsThread (void)
{
if (!tiEffects.pThread)
	return;
tiEffects.bDone = 1;
while (tiEffects.bDone)
	G3_SLEEP (0);
tiEffects.pThread = NULL;
}

//------------------------------------------------------------------------------

void ControlEffectsThread (void)
{
if (gameStates.app.bMultiThreaded)
	StartEffectsThread ();
}

//------------------------------------------------------------------------------

bool WaitForEffectsThread (void)
{
if (gameStates.app.bMultiThreaded && tiEffects.pThread) {
	while (tiEffects.bExec)
		G3_SLEEP (0);
	return true;
	}
return false;
}

//------------------------------------------------------------------------------
// eof
