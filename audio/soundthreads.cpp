#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "pstypes.h"
#include "maths.h"
#include "songs.h"
#include "soundthreads.h"

tSoundThreadInfo tiSound;

//------------------------------------------------------------------------------

int _CDECL_ SoundThread (void *pThreadId)
{
do {
	while (!tiSound.ti.bExec) {
		G3_SLEEP (1);
		if (tiSound.ti.bDone)
			return 0;
		}
	if (tiSound.nTask == stOpenAudio) {
		audio.Setup (tiSound.fSlowDown);
		}
	else if (tiSound.nTask == stCloseAudio) {
		audio.Shutdown ();
		}
	else if (tiSound.nTask == stReconfigureAudio) {
		FreeAddonSounds ();
		audio.Shutdown ();
		audio.Setup (tiSound.fSlowDown);
		if (tiSound.fSlowDown == 1.0f) {
			songManager.SetPos (songManager.SlowDown () - songManager.Start () + 
									  2 * (SDL_GetTicks () - songManager.SlowDown ()) / gameOpts->gameplay.nSlowMotionSpeedup);
			songManager.SetSlowDown (0);
			}
		else {
			songManager.SetSlowDown (SDL_GetTicks ());
			songManager.SetPos (songManager.SlowDown () - songManager.Start ());
			}
		songManager.PlayLevelSong (gameData.missions.nCurrentLevel, 1, false);
		}
	tiSound.ti.bExec = 0;
	} while (!tiSound.ti.bDone);
return 0;
}

//------------------------------------------------------------------------------

void WaitForSoundThread (void)
{
time_t t1 = SDL_GetTicks ();
while (tiSound.ti.pThread && tiSound.ti.bExec /*&& (SDL_GetTicks () - t1 < 1000)*/)
	G3_SLEEP (1);
}

//------------------------------------------------------------------------------

void StartSoundThread (void)
{
if (gameData.app.bUseMultiThreading [rtSound] && !tiSound.ti.pThread) {
	memset (&tiSound, 0, sizeof (tiSound));
	tiSound.ti.nId = 0;
	tiSound.ti.pThread = SDL_CreateThread (SoundThread, &tiSound.ti.nId);
	}
}

//------------------------------------------------------------------------------

void EndSoundThread (void)
{
if (tiSound.ti.pThread) {
	WaitForSoundThread ();
	tiSound.ti.bDone = 1;
	G3_SLEEP (100);
	//SDL_KillThread (tiSound.ti.pThread);
	tiSound.ti.pThread = NULL;
	}	
}

//------------------------------------------------------------------------------

int RunSoundThread (tSoundTask nTask)
{
if (tiSound.ti.pThread && gameData.app.bUseMultiThreading [rtSound]) {
	WaitForSoundThread ();
	tiSound.nTask = nTask;
	tiSound.ti.bExec = 1;
#if 0
	PrintLog ("running render threads (task: %d)\n", nTask);
#endif
	}
return 1;
}

//------------------------------------------------------------------------------
//eof
