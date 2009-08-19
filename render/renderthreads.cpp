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

static inline int ThreadsActive (int nThreads)
{
	int	nActive = 0;

for (int i = 0; i < nThreads; i++)
	if (tiRender.ti [i].bExec)
		nActive++;
return nActive;
}

//------------------------------------------------------------------------------

int RunRenderThreads (int nTask, int nThreads)
{
#if DBG
	time_t	t0 = 0, t2 = 0;
	int		nActive;
	static	int nLockups = 0;
#endif
	int		i;

if (!gameStates.app.bMultiThreaded)
	return 0;
#if 0
if ((nTask < rtTaskCount) && !gameData.app.bUseMultiThreading [nTask])
	return 0;
#endif
tiRender.nTask = (tRenderTask) nTask;
for (i = 0; i < nThreads; i++)
	tiRender.ti [i].bExec = 1;
#if DBG
t0 = clock ();
while ((nActive = ThreadsActive (nThreads)) && (clock () - t0 < 1000)) {
	G3_SLEEP (0);
	if (nActive < nThreads) {
		if (!t2)
			t2 = clock ();
		else if (clock () - t2 > 33) {	//slower threads must not take more than 33 ms over the fastest one
#if 0//!DBG
			t2 = clock ();
#else
			PrintLog ("threads locked up (task: %d)\n", nTask);
			for (i = 0; i < nThreads; i++)
				tiRender.ti [i].bExec = 0;
			if (++nLockups > 100)
				gameStates.app.bMultiThreaded = 0;
#endif
			}
		}
	}
#else
while (ThreadsActive (nThreads))
	G3_SLEEP (0);
#endif
return 1;
}

//------------------------------------------------------------------------------

static inline void ComputeThreadRange (int nId, int nMax, int& nStart, int& nEnd)
{
int nRange = (nMax + gameStates.app.nThreads - 1) / gameStates.app.nThreads;
nStart = nId * nRange;
nEnd = nStart + nRange;
if (nEnd > nMax)
	nEnd = nMax;
}

//------------------------------------------------------------------------------

int _CDECL_ RenderThread (void *pThreadId)
{
	int		nId = *reinterpret_cast<int*> (pThreadId);
	int		nRange, nStart, nEnd;
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
		ComputeThreadRange (nId, gameData.render.mine.nRenderSegs, nStart, nEnd);
		QSortSegZRef (nStart, nEnd);
		}
	else if (tiRender.nTask == rtInitSegZRef) {
		ComputeThreadRange (nId, gameData.render.mine.nRenderSegs, nStart, nEnd);
		InitSegZRef (nStart, nEnd, nId);
		}
	else if (tiRender.nTask == rtStaticVertLight) {
		ComputeThreadRange (nId, gameData.segs.nVertices, nStart, nEnd);
		lightManager.GatherStaticVertexLights (nStart, nEnd, nId);
		}
	else if (tiRender.nTask == rtComputeFaceLight) {
		if (gameStates.render.bTriangleMesh || !gameStates.render.bApplyDynLight || (gameData.render.mine.nRenderSegs < gameData.segs.nSegments)) {
			// special handling: 
			// tiMiddle is the index at which an equal number of visible faces is both at indices below and above it
			// use it to balance thread load
			int nThreads2 = gameStates.app.nThreads / 2;
			if (nId < nThreads2) {
				nRange = (tiRender.nMiddle + nThreads2 - 1) / nThreads2;
				nStart = nId * nRange;
				nEnd = nStart + nRange;
				if (nEnd > tiRender.nMiddle)
					nEnd = tiRender.nMiddle;
				}
			else {
				nRange = (gameData.render.mine.nRenderSegs - tiRender.nMiddle + nThreads2 - 1) / nThreads2;
				nStart = (nId - nThreads2) * nRange;
				nEnd = nStart + nRange;
				if (nEnd > gameData.render.mine.nRenderSegs)
					nEnd = gameData.render.mine.nRenderSegs;
				}
			ComputeFaceLight (nStart, nEnd, nId);
			}
		else if (gameStates.app.bEndLevelSequence < EL_OUTSIDE) 
			ComputeThreadRange (nId, gameData.segs.nFaces, nStart, nEnd);
		else 
			ComputeThreadRange (nId, gameData.segs.nSegments, nStart, nEnd);
		ComputeFaceLight (nStart, nEnd, nId);
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
if (gameStates.app.bMultiThreaded == 2)
	StartEffectsThread ();
}

//------------------------------------------------------------------------------

bool WaitForEffectsThread (void)
{
if ((gameStates.app.bMultiThreaded == 2) && tiEffects.pThread) {
	while (tiEffects.bExec)
		G3_SLEEP (0);
	return true;
	}
return false;
}

//------------------------------------------------------------------------------

int OMP_GetNumThreads (void)
{
#ifdef OPENMP
	int nThreads;

#pragma omp parallel
	{
	nThreads = omp_get_num_threads ();
	if (nThreads > MAX_THREADS)
		omp_set_num_threads (MAX_THREADS);

	}
return nThreads;
#else
return 2;
#endif
}

//------------------------------------------------------------------------------
// eof
