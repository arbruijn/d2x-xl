#ifndef _RENDERTHREADS_H
#define _RENDERTHREADS_H

#include "descent.h"
#include "lightning.h"
#include "transprender.h"
#include "particles.h"

#ifdef __macosx__  // BEGIN itaylo 06 NOV 2013
#	include "SDL/SDL_mutex.h"
#else
#	include "SDL_mutex.h"
#endif              // END itaylo 06 NOV 2013

#define UNIFY_THREADS	0

class CRenderThreadInfo {
	public:
		tRenderTask	nTask;
		int32_t						nMiddle;
		int32_t						nFaces;
		int32_t						zMin [MAX_THREADS];
		int32_t						zMax [MAX_THREADS];
		tLightning*				pLightning;
		int32_t						nLightnings;
		CObject*					pObj;
		RenderModel::CModel*	pModel;
		tParticleEmitter*		particleEmitters [MAX_THREADS];
		int32_t						nCurTime [MAX_THREADS];
		CThreadInfo				ti [MAX_THREADS];
		SDL_mutex*				semaphore;
	};

extern CRenderThreadInfo tiRender;

class CEffectThreadInfo : public CThreadInfo {
	public:
		int32_t nWindow;
};

extern CEffectThreadInfo tiEffects;

int32_t RunRenderThreads (int32_t nTask, int32_t nThreads = -1);
void CreateRenderThreads (void);
void DestroyRenderThreads (void);
bool WaitForRenderThreads (void);
void CreateEffectsThread (void);
bool StartEffectsThread (int32_t nWindow);
void DestroyEffectsThread (void);
void ControlEffectsThread (void);
bool WaitForEffectsThread (void);
void ControlRenderThreads (void);

int32_t GetNumThreads (void);

//------------------------------------------------------------------------------

inline int32_t RenderThreadsReady (void)
{
return !(gameStates.app.bMultiThreaded && (tiRender.ti [0].bExec || tiRender.ti [1].bExec));
}

//------------------------------------------------------------------------------

inline void ComputeThreadRange (int32_t nId, int32_t nMax, int32_t& nStart, int32_t& nEnd, int32_t nThreads = gameStates.app.nThreads)
{
int32_t nRange = (nMax + nThreads - 1) / nThreads;
nStart = nId * nRange;
nEnd = nStart + nRange;
if (nEnd > nMax)
	nEnd = nMax;
}

//------------------------------------------------------------------------------

#endif // _RENDERTHREADS_H
