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
tThreadInfo tiEffects;

int _CDECL_ LightObjectsThread (void* nThreadP);

//------------------------------------------------------------------------------

bool WaitForRenderThreads (void)
{
#if !USE_OPENMP
if (gameStates.app.bMultiThreaded) {
	while (tiRender.ti [0].bExec || tiRender.ti [1].bExec)
		G3_SLEEP (0);	//already running, so wait
	return true;
	}
#endif
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
#if USE_OPENMP

return 0;

#else

if (!gameStates.app.bMultiThreaded)
	return 0;

#	if DBG
	time_t	t0 = 0, t2 = 0;
	static	int nLockups = 0;
#	endif
	int		i, bWait = 1;

if (nTask < 0) {
	nTask = -nTask - 1;
	bWait = 0;
	}
tiRender.nTask = (tRenderTask) nTask;
if (nThreads < 0)
	nThreads = gameStates.app.nThreads;
for (i = 0; i < nThreads; i++)
	tiRender.ti [i].bExec = 1;
if (!bWait)
	return 1;
#if 0 //DBG
int nActive;
t0 = clock ();
while ((nActive = ThreadsActive (nThreads)) && (clock () - t0 < 1000)) {
	G3_SLEEP (0);
	if (nActive < nThreads) {
		if (!t2)
			t2 = clock ();
		else if (clock () - t2 > 33) {	//slower threads must not take more than 33 ms over the fastest one
			PrintLog ("threads locked up (task: %d)\n", nTask);
			for (i = 0; i < nThreads; i++)
				tiRender.ti [i].bExec = 0;
			if (++nLockups > 100) {
				gameStates.app.bMultiThreaded = 0;
				gameStates.app.nThreads = 1;
				}
			}
		}
	}
#	else
while (ThreadsActive (nThreads))
	G3_SLEEP (0);
#	endif
return 1;

#endif
}

//------------------------------------------------------------------------------

int _CDECL_ RenderThread (void *pThreadId)
{
	int		nId = *reinterpret_cast<int*> (pThreadId);
	int		nStart, nEnd;
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
		ComputeThreadRange (nId, gameData.render.mine.nRenderSegs [0], nStart, nEnd);
		QSortSegZRef (nStart, nEnd);
		}
	else if (tiRender.nTask == rtInitSegZRef) {
		ComputeThreadRange (nId, gameData.render.mine.nRenderSegs [0], nStart, nEnd);
		InitSegZRef (nStart, nEnd, nId);
		}
	else if (tiRender.nTask == rtStaticVertLight) {
		ComputeThreadRange (nId, gameData.segs.nVertices, nStart, nEnd);
		lightManager.GatherStaticVertexLights (nStart, nEnd, nId);
		}
	else if (tiRender.nTask == rtComputeFaceLight) {
		if (gameStates.render.bTriangleMesh || !gameStates.render.bApplyDynLight || (gameData.render.mine.nRenderSegs [0] < gameData.segs.nSegments)) {
			// special handling: 
			// tiMiddle is the index at which an equal number of visible faces is both at indices below and above it
			// use it to balance thread load
			if (gameStates.app.nThreads & 1) {
				ComputeThreadRange (nId, gameData.render.mine.nRenderSegs [0], nStart, nEnd);
				ComputeFaceLight (nStart, nEnd, nId);
				}
			else {
				int nPivot = gameStates.app.nThreads / 2;
				if (nId < nPivot) {
					ComputeThreadRange (nId, tiRender.nMiddle, nStart, nEnd, nPivot);
					ComputeFaceLight (nStart, nEnd, nId);
					}
				else {
					ComputeThreadRange (nId - nPivot, gameData.render.mine.nRenderSegs [0] - tiRender.nMiddle, nStart, nEnd, nPivot);
					ComputeFaceLight (nStart + tiRender.nMiddle, nEnd + tiRender.nMiddle, nId);
					}
				}
			}
		else {
			if (gameStates.app.bEndLevelSequence < EL_OUTSIDE) 
				ComputeThreadRange (nId, gameData.segs.nFaces, nStart, nEnd);
			else 
				ComputeThreadRange (nId, gameData.segs.nSegments, nStart, nEnd);
			ComputeFaceLight (nStart, nEnd, nId);
			}
		}
	else if (tiRender.nTask == rtPolyModel) {
		LightObjectsThread (&nId);
		}
	else if (tiRender.nTask == rtLightmap)
		lightmapManager.Build (lightmapManager.CurrentFace (), nId);
	else if (tiRender.nTask == rtParticles) 
		particleManager.SetupParticles (nId);
	tiRender.ti [nId].bExec = 0;
	} while (!tiRender.ti [nId].bDone);
#ifdef _WIN32
if (myContext)
	wglDeleteContext (myContext);
#endif
return 0;
}

//------------------------------------------------------------------------------

void StartRenderThreads (void)
{
if (!gameStates.app.bMultiThreaded)
	return;

	static bool bInitialized = false;

if (!bInitialized) {
	memset (&tiRender, 0, sizeof (tiRender));
	bInitialized = true;
	}
#if !USE_OPENMP
for (int i = 0; i < gameStates.app.nThreads; i++) {
	if (!tiRender.ti [i].pThread) {
		tiRender.ti [i].bDone =
		tiRender.ti [i].bExec = 0;
		tiRender.ti [i].nId = i;
		tiRender.ti [i].pThread = SDL_CreateThread (RenderThread, &tiRender.ti [i].nId);
		}
	tiRender.semaphore = SDL_CreateMutex ();
	}
#endif
}

//------------------------------------------------------------------------------

void ControlRenderThreads (void)
{
#if !USE_OPENMP
StartRenderThreads ();
#endif
}

//------------------------------------------------------------------------------

void EndRenderThreads (void)
{
if (!gameStates.app.bMultiThreaded)
	return;
#if !USE_OPENMP
for (int i = 0; i < gameStates.app.nThreads; i++) 
	tiRender.ti [i].bDone = 1;
G3_SLEEP (10);
for (int i = 0; i < gameStates.app.nThreads; i++) {
	if (tiRender.ti [i].pThread) {
		//SDL_KillThread (tiRender.ti [0].pThread);
		tiRender.ti [i].pThread = NULL;
		}
	}
SDL_DestroyMutex (tiRender.semaphore);
tiRender.semaphore = NULL;
#endif
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
#if 0 //!USE_OPENMP
tiEffects.bDone = 0;
tiEffects.bExec = 0;
if	(!(tiEffects.pThread || (tiEffects.pThread = SDL_CreateThread (EffectsThread, NULL))))
	gameData.app.bUseMultiThreading [rtEffects] = 0;
#endif
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
#if !USE_OPENMP
if (gameStates.app.bMultiThreaded > 1)
	StartEffectsThread ();
#endif
}

//------------------------------------------------------------------------------

bool WaitForEffectsThread (void)
{
#if !USE_OPENMP
if ((gameStates.app.bMultiThreaded > 1) && tiEffects.pThread) {
	while (tiEffects.bExec)
		G3_SLEEP (0);
	return true;
	}
#endif
return false;
}

//------------------------------------------------------------------------------

int GetNumThreads (void)
{
if (!gameStates.app.bMultiThreaded)
	return gameStates.app.nThreads = 1;
#if USE_OPENMP
int nThreads = omp_get_num_threads ();
if (nThreads < 2)
#pragma omp parallel 
	{
	nThreads = omp_get_max_threads ();
	}

if (gameStates.app.nThreads > MAX_THREADS)
	gameStates.app.nThreads = MAX_THREADS;
if (nThreads < gameStates.app.nThreads)
	gameStates.app.nThreads = nThreads;
else if (nThreads > gameStates.app.nThreads)
#	if 0 //DBG
	omp_set_num_threads (gameStates.app.nThreads = 2);
#	else
	omp_set_num_threads (gameStates.app.nThreads);
#	endif
#endif
return gameStates.app.nThreads;
}

//------------------------------------------------------------------------------
// eof
