/* $Id: render.c, v 1.18 2003/10/10 09:36:35 btb Exp $ */
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

#include "inferno.h"
#include "render.h"
#include "fastrender.h"
#include "renderthreads.h"

tRenderThreadInfo tiRender;

//------------------------------------------------------------------------------

int RunRenderThreads (int nTask)
{
	time_t	t1 = 0, t2 = 0;

if (!gameStates.app.bMultiThreaded)
	return 0;
#if 0
while (tiRender.ti [0].bExec || tiRender.ti [1].bExec)
	G3_SLEEP (0);	//already running, so wait
#endif
tiRender.nTask = nTask;
tiRender.ti [0].bExec =
tiRender.ti [1].bExec = 1;
t1 = clock ();
while (tiRender.ti [0].bExec || tiRender.ti [1].bExec && (clock () - t1 < 1000)) {
	G3_SLEEP (0);
	if (tiRender.ti [0].bExec != tiRender.ti [1].bExec) {
		if (!t2)
			t2 = clock ();
		else if (clock () - t2 > 10)	//slower thread must not take more than 10 ms longer than faster one
			tiRender.ti [0].bExec =
			tiRender.ti [1].bExec = 0;
		}
	}	
return 1;
}

//------------------------------------------------------------------------------

int _CDECL_ RenderThread (void *pThreadId)
{
	int	nId = *((int *) pThreadId);

do {
	while (!tiRender.ti [nId].bExec)
		G3_SLEEP (0);
	if (tiRender.nTask == rtSortSegZRef) {
		if (nId)
			QSortSegZRef (gameData.render.mine.nRenderSegs / 2, gameData.render.mine.nRenderSegs - 1);
		else
			QSortSegZRef (0, gameData.render.mine.nRenderSegs / 2 - 1);
		}
	else if (tiRender.nTask == rtInitSegZRef) {
		if (nId)
			InitSegZRef (gameData.render.mine.nRenderSegs / 2, gameData.render.mine.nRenderSegs, nId);
		else
			InitSegZRef (0, gameData.render.mine.nRenderSegs / 2, nId);
		}
	if (tiRender.nTask == rtSortFaces) {
		if (nId)
			QSortFaces (tiRender.nFaces / 2, tiRender.nFaces - 1);
		else
			QSortFaces (0, tiRender.nFaces / 2 - 1);
		}
	else if (tiRender.nTask == rtComputeFaceLight) {
		if (nId)
			ComputeFaceLight (tiRender.nMiddle, gameData.render.mine.nRenderSegs, nId);
		else
			ComputeFaceLight (0, tiRender.nMiddle, nId);
		}
	else if (tiRender.nTask == rtAnimateLightnings) {
		if (nId)
			AnimateLightning (tiRender.pl, tiRender.nLightnings / 2, tiRender.nLightnings, 0);
		else
			AnimateLightning (tiRender.pl, 0, tiRender.nLightnings / 2, 0);
		}
	else if (tiRender.nTask == rtRenderLightnings) {
		if (nId)
			RenderLightningsBuffered (tiRender.pl, tiRender.nLightnings / 2, tiRender.nLightnings, 0, nId);
		else
			RenderLightningsBuffered (tiRender.pl, 0, tiRender.nLightnings / 2, 0, nId);
		}
	tiRender.ti [nId].bExec = 0;
	} while (!tiRender.ti [nId].bDone);
return 0;
}

//------------------------------------------------------------------------------

void StartRenderThreads (void)
{
	int	i;

for (i = 0; i < 2; i++) {
	tiRender.ti [i].bDone = 0;
	tiRender.ti [i].bBlock = 0;
	tiRender.ti [i].bExec = 0;
	tiRender.ti [i].nId = i;
	tiRender.ti [i].pThread = SDL_CreateThread (RenderThread, &tiRender.ti [i].nId);
	}
}

//------------------------------------------------------------------------------

void EndRenderThreads (void)
{
	int	i;

for (i = 0; i < 2; i++)
	tiRender.ti [i].bDone = 1;
G3_SLEEP (1);
#if 1
SDL_KillThread (tiRender.ti [0].pThread);
SDL_KillThread (tiRender.ti [1].pThread);
#else
SDL_WaitThread (tiRender.ti [0].pThread, NULL);
SDL_WaitThread (tiRender.ti [1].pThread, NULL);
#endif
}

//------------------------------------------------------------------------------
// eof