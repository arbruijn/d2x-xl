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

CRenderThreadInfo tiRender;
CEffectThreadInfo tiEffects;

int32_t _CDECL_ LightObjectsThread (void* nThreadP);

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

static inline int32_t ThreadsActive (int32_t nThreads)
{
	int32_t	nActive = 0;

for (int32_t i = 0; i < nThreads; i++)
	if (tiRender.ti [i].bExec)
		nActive++;
return nActive;
}

//------------------------------------------------------------------------------

int32_t RunRenderThreads (int32_t nTask, int32_t nThreads)
{
#if USE_OPENMP

return 0;

#else

if (!gameStates.app.bMultiThreaded)
	return 0;

#	if DBG
	time_t	t0 = 0, t2 = 0;
	static	int32_t nLockups = 0;
#	endif
	int32_t		i, bWait = 1;

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
int32_t nActive;
t0 = clock ();
while ((nActive = ThreadsActive (nThreads)) && (clock () - t0 < 1000)) {
	G3_SLEEP (0);
	if (nActive < nThreads) {
		if (!t2)
			t2 = clock ();
		else if (clock () - t2 > 33) {	//slower threads must not take more than 33 ms over the fastest one
			PrintLog (0, "thread locked up (task: %d)\n", nTask);
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

int32_t _CDECL_ RenderThread (void *pThreadId)
{
	int32_t		nId = *reinterpret_cast<int32_t*> (pThreadId);
	int32_t		nStart, nEnd;
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
		ComputeThreadRange (nId, gameData.renderData.mine.visibility [0].nSegments, nStart, nEnd);
		gameData.renderData.mine.visibility [nId].QSortZRef (nStart, nEnd);
		}
	else if (tiRender.nTask == rtInitSegZRef) {
		ComputeThreadRange (nId, gameData.renderData.mine.visibility [0].nSegments, nStart, nEnd);
		gameData.renderData.mine.visibility [nId].InitZRef (nStart, nEnd, nId);
		}
	else if (tiRender.nTask == rtStaticVertLight) {
		ComputeThreadRange (nId, gameData.segData.nVertices, nStart, nEnd);
		lightManager.GatherStaticVertexLights (nStart, nEnd, nId);
		}
	else if (tiRender.nTask == rtComputeFaceLight) {
		if (gameStates.render.bTriangleMesh || !gameStates.render.bApplyDynLight || (gameData.renderData.mine.visibility [0].nSegments < gameData.segData.nSegments)) {
			// special handling: 
			// tiMiddle is the index at which an equal number of visible faces is both at indices below and above it
			// use it to balance thread load
			if (gameStates.app.nThreads & 1) {
				ComputeThreadRange (nId, gameData.renderData.mine.visibility [0].nSegments, nStart, nEnd);
				ComputeFaceLight (nStart, nEnd, nId);
				}
			else {
				int32_t nPivot = gameStates.app.nThreads / 2;
				if (nId < nPivot) {
					ComputeThreadRange (nId, tiRender.nMiddle, nStart, nEnd, nPivot);
					ComputeFaceLight (nStart, nEnd, nId);
					}
				else {
					ComputeThreadRange (nId - nPivot, gameData.renderData.mine.visibility [0].nSegments - tiRender.nMiddle, nStart, nEnd, nPivot);
					ComputeFaceLight (nStart + tiRender.nMiddle, nEnd + tiRender.nMiddle, nId);
					}
				}
			}
		else {
			if (gameStates.app.bEndLevelSequence < EL_OUTSIDE) 
				ComputeThreadRange (nId, FACES.nFaces, nStart, nEnd);
			else 
				ComputeThreadRange (nId, gameData.segData.nSegments, nStart, nEnd);
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

void CreateRenderThreads (void)
{
if (!gameStates.app.bMultiThreaded)
	return;

	static bool bInitialized = false;

if (!bInitialized) {
	memset (&tiRender, 0, sizeof (tiRender));
	bInitialized = true;
	}
#if !USE_OPENMP
for (int32_t i = 0; i < gameStates.app.nThreads; i++) {
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
CreateRenderThreads ();
#endif
}

//------------------------------------------------------------------------------

void DestroyRenderThreads (void)
{
if (!gameStates.app.bMultiThreaded)
	return;
#if !USE_OPENMP
for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
	tiRender.ti [i].bDone = 1;
G3_SLEEP (10);
for (int32_t i = 0; i < gameStates.app.nThreads; i++) {
	if (tiRender.ti [i].pThread) {
		//SDL_KillThread (tiRender.ti [0].pThread);
		tiRender.ti [i].pThread = NULL;
		}
	}
SDL_DestroyMutex (tiRender.semaphore);
tiRender.semaphore = NULL;
#endif
DestroyEffectsThread ();
}

//------------------------------------------------------------------------------

int32_t _CDECL_ EffectsThread (void *pThreadId)
{
do {
	while (!tiEffects.bExec) {
		G3_SLEEP (0);
		if (tiEffects.bDone) {
			tiEffects.bDone = 0;
			return 0;
			}
		}
	UpdateEffects ();
	RenderEffects (tiEffects.nWindow);
	tiEffects.bExec = 0;
	} while (!tiEffects.bDone);
tiEffects.bDone = 0;
return 0;
}

//------------------------------------------------------------------------------

void CreateEffectsThread (void)
{
#if 0
if (gameStates.app.nThreads > 1) {
	static bool bInitialized = false;

	if (!bInitialized) {
		memset (&tiEffects, 0, sizeof (tiEffects));
		bInitialized = true;
		}
	#if 1 //!USE_OPENMP
	tiEffects.bDone = 0;
	tiEffects.bExec = 0;
	if	(!(tiEffects.pThread || (tiEffects.pThread = SDL_CreateThread (EffectsThread, NULL))))
		gameData.appData.bUseMultiThreading [rtEffects] = 0;
	#endif
	}
#endif
}

//------------------------------------------------------------------------------

void DestroyEffectsThread (void)
{
#if 0
if (!tiEffects.pThread)
	return;
tiEffects.bDone = 1;
while (tiEffects.bDone)
	G3_SLEEP (0);
tiEffects.pThread = NULL;
#endif
}

//------------------------------------------------------------------------------

bool StartEffectsThread (int32_t nWindow)
{
#if 1 //!USE_OPENMP
if (!WaitForEffectsThread ())
	return false;
tiEffects.nWindow = nWindow;
tiEffects.bExec = 1;
return true;
#endif
}

//------------------------------------------------------------------------------
// waits for effects thread to finish current processing and return true if effects thread exists, otherwise false

bool WaitForEffectsThread (void)
{
#if 1 //!USE_OPENMP
if ((gameStates.app.nThreads < 2) || !tiEffects.pThread) 
	return false;
while (tiEffects.bExec)
	G3_SLEEP (0);
return true;
#endif
}

//------------------------------------------------------------------------------
// Retrieve the number of actual hardware CPU cores, because Intel's hyper threading 
// actually slows multiple threads processing massive amounts of data down!

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#	include <intrin.h>
#else
#	include <cpuid.h>
#endif

typedef union {
	int v [4];
	struct {
		unsigned int eax, ebx, ecx, edx;
	} regs;
} cpu_t;

//------------------------------------------------------------------------------

inline void CPUID (cpu_t& cpu, int infoType)
{
#ifdef _WIN32
__cpuid (cpu.v, infoType);
#else
__get_cpuid (infoType, &cpu.regs.eax, &cpu.regs.ebx, &cpu.regs.ecx, &cpu.regs.edx);
#endif
}

//------------------------------------------------------------------------------

uint32_t GetCPUCores (void)
{
	cpu_t	cpu;

CPUID (cpu, 0);

//int nIds = cpu.v [0];

union {
	char	s [16];
	int	v [4];
} vendor;

vendor.v [0] = cpu.v [1];
vendor.v [1] = cpu.v [3];
vendor.v [2] = cpu.v [2];
vendor.v [3] = 0;

CPUID (cpu, 1);

unsigned int nFeatures = cpu.v [3];
unsigned int nLogical = (cpu.v [1] >> 16) & 0xFF;
unsigned int nCores = nLogical;

if (!strcmp (vendor.s, "GenuineIntel")) {
	CPUID (cpu, 4);
	nCores = ((cpu.v [0] >> 26) & 0x3F) + 1;
	}
else if (!strcmp (vendor.s, "AuthenticAMD")) {
	CPUID (cpu, 0x80000008);
	nCores = (cpu.v [2] & 0xff) + 1;
	}

int bHyperThreads = (nFeatures & (1 << 28)) && (nCores < nLogical);

PrintLog (0, "\nGetCPUCores: CPU Id = '%s'. Found %d physical and %d logical CPUs. Hyper threading = %s.\n\n",
			 vendor.s, nCores, nLogical, bHyperThreads ? "on" : "off");

return nCores / (bHyperThreads + 1);
}

//------------------------------------------------------------------------------

int32_t GetNumThreads (void)
{
if (!gameStates.app.bMultiThreaded)
	return gameStates.app.nThreads = 1;

#if USE_OPENMP && defined(_OPENMP)

int32_t nCores = GetCPUCores ();
int32_t nThreads = omp_get_num_threads ();
if (nThreads < 2)
#pragma omp parallel 
	{
	nThreads = omp_get_max_threads ();
	}

if (nCores > nThreads)
	nCores = nThreads;
else if (nThreads > nCores)
	nThreads = nCores;

if (gameStates.app.nThreads > MAX_THREADS)
	gameStates.app.nThreads = MAX_THREADS;
if (nThreads < gameStates.app.nThreads)
	gameStates.app.nThreads = nThreads;
omp_set_num_threads (gameStates.app.nThreads);

#endif

return gameStates.app.nThreads;
}

//------------------------------------------------------------------------------
// eof
