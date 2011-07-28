#ifndef _RENDERTHREADS_H
#define _RENDERTHREADS_H

#include "descent.h"
#include "lightning.h"
#include "transprender.h"
#include "particles.h"

#include "SDL_mutex.h"

#define UNIFY_THREADS	0

typedef struct tRenderThreadInfo {
	tRenderTask	nTask;
	int						nMiddle;
	int						nFaces;
	int						zMin [MAX_THREADS];
	int						zMax [MAX_THREADS];
	tLightning*				lightningP;
	int						nLightnings;
	CObject*					objP;
	RenderModel::CModel*	modelP;
	tParticleEmitter*		particleEmitters [MAX_THREADS];
	int						nCurTime [MAX_THREADS];
	tThreadInfo				ti [MAX_THREADS];
	SDL_mutex*				semaphore;
	} tRenderThreadInfo;

extern tRenderThreadInfo tiRender;

extern tThreadInfo tiEffects;

int RunRenderThreads (int nTask, int nThreads = -1);
void StartRenderThreads (void);
void EndRenderThreads (void);
bool WaitForRenderThreads (void);
void StartEffectsThread (void);
void EndEffectsThread (void);
void ControlEffectsThread (void);
bool WaitForEffectsThread (void);
void ControlRenderThreads (void);

int GetNumThreads (void);

//------------------------------------------------------------------------------

inline int RenderThreadsReady (void)
{
return !(gameStates.app.bMultiThreaded && (tiRender.ti [0].bExec || tiRender.ti [1].bExec));
}

//------------------------------------------------------------------------------

inline void ComputeThreadRange (int nId, int nMax, int& nStart, int& nEnd, int nThreads = gameStates.app.nThreads)
{
int nRange = (nMax + nThreads - 1) / nThreads;
nStart = nId * nRange;
nEnd = nStart + nRange;
if (nEnd > nMax)
	nEnd = nMax;
}

//------------------------------------------------------------------------------

#endif // _RENDERTHREADS_H
