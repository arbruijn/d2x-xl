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
#include "timeout.h"
#include "config.h"

tSoundThreadInfo tiSound;

//------------------------------------------------------------------------------

int _CDECL_ SoundThread (void *pThreadId)
{
do {
	while (!tiSound.ti.bExec) {
		G3_SLEEP (1);
		if (tiSound.ti.bDone) {
			tiSound.ti.bDone = 0;
			return 0;
			}
		}
	if (tiSound.nTask == stOpenAudio) {
		audio.Setup (tiSound.fSlowDown);
		audio.SetFxVolume ((gameConfig.nAudioVolume [1] * 32768) / 8, 1);
		audio.SetVolumes ((gameConfig.nAudioVolume [0] * 32768) / 8, (gameConfig.nMidiVolume * 128) / 8);
		}
	else if (tiSound.nTask == stCloseAudio) {
		audio.Shutdown ();
		}
	else if (tiSound.nTask == stReconfigureAudio) {
		FreeAddonSounds ();
		audio.Shutdown ();
		audio.Setup (tiSound.fSlowDown);
		LoadAddonSounds ();
		if (tiSound.fSlowDown == 1.0f) {
			songManager.SetPos (songManager.SlowDown () - songManager.Start () + 
									  2 * (SDL_GetTicks () - songManager.SlowDown ()) / gameOpts->gameplay.nSlowMotionSpeedup);
			songManager.SetSlowDown (0);
			}
		else {
			songManager.SetSlowDown (SDL_GetTicks ());
			songManager.SetPos (songManager.SlowDown () - songManager.Start ());
			}
		songManager.PlayLevelSong (missionManager.nCurrentLevel, 1, false);
		}
	tiSound.ti.bExec = 0;
	} while (!tiSound.ti.bDone);
tiSound.ti.bDone = 0;
return 0;
}

//------------------------------------------------------------------------------

void WaitForSoundThread (time_t nTimeout)
{
time_t t0 = (nTimeout < 0) ? 0 : (time_t) SDL_GetTicks ();
while (tiSound.ti.pThread && tiSound.ti.bExec && ((nTimeout < 0) || ((time_t) SDL_GetTicks () - t0 < nTimeout)))
	G3_SLEEP (1);
}

//------------------------------------------------------------------------------

bool HaveSoundThread (void)
{
return tiSound.ti.pThread != NULL;
}

//------------------------------------------------------------------------------

void StartSoundThread (void)
{
if (!tiSound.ti.pThread) {
	memset (&tiSound, 0, sizeof (tiSound));
	tiSound.ti.nId = 0;
	tiSound.ti.pThread = SDL_CreateThread (SoundThread, &tiSound.ti.nId);
	}
}

//------------------------------------------------------------------------------

void EndSoundThread (void)
{
if (tiSound.ti.pThread) {
	WaitForSoundThread (1000);
	tiSound.ti.bDone = 1;
	CTimeout to (1000);
	do {
		G3_SLEEP (1);
		} while (tiSound.ti.bDone && !to.Expired ());
	if (tiSound.ti.bDone)
		SDL_KillThread (tiSound.ti.pThread);
	tiSound.ti.pThread = NULL;
	}	
}

//------------------------------------------------------------------------------

void ControlSoundThread (void)
{
#if 1
StartSoundThread ();
#else
if (gameStates.app.bMultiThreaded) {
	if (gameData.app.bUseMultiThreading [rtSound])
		StartSoundThread ();
	else
		EndSoundThread ();
	}
#endif
}

//------------------------------------------------------------------------------

int RunSoundThread (tSoundTask nTask)
{
#if 1
if (tiSound.ti.pThread) {
#else
if (tiSound.ti.pThread && gameData.app.bUseMultiThreading [rtSound]) {
#endif
	WaitForSoundThread ();
	tiSound.nTask = nTask;
	tiSound.ti.bExec = 1;
#if 0
	PrintLog (1, "running render threads (task: %d)\n", nTask);
#endif
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------
//eof
