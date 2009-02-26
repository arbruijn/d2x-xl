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

#ifndef _RENDERTHREADS_H
#define _RENDERTHREADS_H

#include "descent.h"
#include "lightning.h"
#include "transprender.h"
#include "particles.h"

typedef struct tRenderThreadInfo {
	tRenderTask	nTask;
	int						nMiddle;
	int						nFaces;
	int						zMax [2];
	tLightning				*pl;
	int						nLightnings;
	CObject					*objP;
	RenderModel::CModel	*pm;
	tParticleEmitter		*particleEmitters [2];
	int						nCurTime [2];
	tThreadInfo				ti [2];
	} tRenderThreadInfo;

extern tRenderThreadInfo tiRender;

typedef struct tTranspRenderThreadInfo {
	tTranspItemData	itemData [2];
	tThreadInfo			ti [2];
	} tTranspRenderThreadInfo;

extern tTranspRenderThreadInfo tiTranspItems;
extern tThreadInfo tiEffects;

int RunRenderThreads (int nTask);
void StartRenderThreads (void);
void EndRenderThreads (void);
void StartTranspRenderThread (void);
void EndTranspRenderThread (void);
void ControlTranspRenderThread (void);
void WaitForRenderThreads (void);
void StartEffectsThread (void);
void EndEffectsThread (void);
void ControlEffectsThread (void);
bool WaitForEffectsThread (void);
void ControlRenderThreads (void);

//------------------------------------------------------------------------------

inline int RenderThreadsReady (void)
{
return !(gameStates.app.bMultiThreaded && (tiRender.ti [0].bExec || tiRender.ti [1].bExec));
}

//------------------------------------------------------------------------------

#endif // _RENDERTHREADS_H
