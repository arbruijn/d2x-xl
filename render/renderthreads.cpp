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

#include "inferno.h"
#include "error.h"
#include "light.h"
#include "dynlight.h"
#include "objsmoke.h"
#include "sparkeffect.h"
#include "lightning.h"
#include "render.h"
#include "fastrender.h"
#include "renderthreads.h"
#include "interp.h"
#include "lightmap.h"

#define KILL_RENDER_THREADS 0

tRenderThreadInfo tiRender;
tRenderItemThreadInfo tiRenderItems;
tThreadInfo tiEffects;

//------------------------------------------------------------------------------

void WaitForRenderThreads (void)
{
if (gameStates.app.bMultiThreaded)
	while (tiRender.ti [0].bExec || tiRender.ti [1].bExec)
		G3_SLEEP (0);	//already running, so wait
}

//------------------------------------------------------------------------------

int RunRenderThreads (int nTask)
{
#if DBG
	time_t	t0 = 0, t2 = 0;
	static	int nLockups = 0;
#endif

if (!gameStates.app.bMultiThreaded)
	return 0;
if (!gameData.app.bUseMultiThreading [nTask])
	return 0;
tiRender.nTask = (tRenderTask) nTask;
tiRender.ti [0].bExec =
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
	int		nId = *((int *) pThreadId);
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
			ComputeStaticVertexLights (gameData.segs.nVertices / 2, gameData.segs.nVertices, nId);
		else
			ComputeStaticVertexLights (0, gameData.segs.nVertices / 2, nId);
		}
	else if (tiRender.nTask == rtComputeFaceLight) {
		if (gameData.render.mine.nRenderSegs < 0) {
			if (nId)
				ComputeFaceLight (gameData.segs.nFaces / 2, gameData.segs.nFaces, nId);
			else
				ComputeFaceLight (0, gameData.segs.nFaces / 2, nId);
			}
		else {
			if (nId)
				ComputeFaceLight (gameData.render.mine.nRenderSegs - 1, tiRender.nMiddle - 1, nId);
			else
				ComputeFaceLight (0, tiRender.nMiddle, nId);
			}
		}
	else if (tiRender.nTask == rtPolyModel) {
		short	iVerts, nVerts, iFaceVerts, nFaceVerts;

		if (nId) {
			nVerts = tiRender.pm->nVerts;
			iVerts = nVerts / 2;
			nFaceVerts = tiRender.pm->nFaceVerts;
			iFaceVerts = nFaceVerts / 2;
			}
		else {
			iVerts = 0;
			nVerts = tiRender.pm->nVerts / 2;
			iFaceVerts = 0;
			nFaceVerts = tiRender.pm->nFaceVerts / 2;
			}
		G3DynLightModel (tiRender.objP, tiRender.pm, iVerts, nVerts, iFaceVerts, nFaceVerts);
		}
	else if (tiRender.nTask == rtPolyModel)
		ComputeOneLightmap (nId);
	tiRender.ti [nId].bExec = 0;
	} while (!tiRender.ti [nId].bDone);
#ifdef _WIN32
if (myContext)
	wglDeleteContext (myContext);
#endif
return 0;
}

//------------------------------------------------------------------------------

int _CDECL_ RenderItemThread (void *pThreadId)
{
#if 1
	int	i;

do {
	while (!(tiRenderItems.ti [0].bExec || tiRenderItems.ti [1].bExec))
		G3_SLEEP (0);
	for (i = 0; i < 2; i++) {
		if (tiRenderItems.ti [i].bExec) {
			AddRenderItem ((tRenderItemType) tiRenderItems.itemData [i].nType, 
								&tiRenderItems.itemData [i].item, 
								tiRenderItems.itemData [i].nSize, 
								tiRenderItems.itemData [i].nDepth, 
								tiRenderItems.itemData [i].nIndex);
			tiRenderItems.ti [i].bExec = 0;
			}
		}
	} while (!(tiRenderItems.ti [0].bDone && tiRenderItems.ti [1].bDone));
return 0;
#endif
}

//------------------------------------------------------------------------------

void StartRenderItemThread (void)
{
#if 1
memset (&tiRenderItems, 0, sizeof (tiRenderItems));
tiRenderItems.ti [0].pThread = SDL_CreateThread (RenderItemThread, NULL);
#endif
}

//------------------------------------------------------------------------------

void EndRenderItemThread (void)
{
if (!tiRenderItems.ti [0].pThread)
	return;
tiRenderItems.ti [0].bDone = 0;
G3_SLEEP (10);
//SDL_KillThread (tiRenderItems.ti [0].pThread);
tiRenderItems.ti [0].pThread = NULL;
}

//------------------------------------------------------------------------------

void StartRenderThreads (void)
{
	int	i;

gameData.app.bUseMultiThreading [rtLightmap] = 1;
memset (&tiRender, 0, sizeof (tiRender));
for (i = 0; i < 2; i++) {
	tiRender.ti [i].nId = i;
	tiRender.ti [i].pThread = SDL_CreateThread (RenderThread, &tiRender.ti [i].nId);
	}
StartRenderItemThread ();
StartEffectsThread ();
}

//------------------------------------------------------------------------------

void EndRenderThreads (void)
{
	int	i;

for (i = 0; i < 2; i++)
	tiRender.ti [i].bDone = 0;
G3_SLEEP (10);
for (i = 0; i < 2; i++) {
	if (tiRender.ti [i].pThread) {
		//SDL_KillThread (tiRender.ti [0].pThread);
		tiRender.ti [i].pThread = NULL;
		}
	}
EndRenderItemThread ();
EndEffectsThread ();
}

//------------------------------------------------------------------------------

int _CDECL_ EffectsThread (void *pThreadId)
{
do {
	while (!tiEffects.bExec)
		G3_SLEEP (0);
	DoSmokeFrame ();
	DoEnergySparkFrame ();
	DoLightningFrame ();
	tiEffects.bExec = 0;
	} while (!tiEffects.bDone);
return 0;
}

//------------------------------------------------------------------------------

void StartEffectsThread (void)
{
memset (&tiEffects, 0, sizeof (tiEffects));
if (!(tiEffects.pThread = SDL_CreateThread (EffectsThread, NULL)))
	gameData.app.bUseMultiThreading [rtEffects] = 0;
}

//------------------------------------------------------------------------------

void EndEffectsThread (void)
{
if (!tiEffects.pThread)
	return;
tiEffects.bDone = 0;
G3_SLEEP (100);
//SDL_KillThread (tiEffects.pThread);
tiEffects.pThread = NULL;
}

//------------------------------------------------------------------------------
// eof
