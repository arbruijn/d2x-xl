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

CSoundThreadInfo tiSound;

//------------------------------------------------------------------------------

int32_t _CDECL_ SoundThread (void *pThreadId)
{
do {
	while (!tiSound.bExec) {
		G3_SLEEP (1);
		if (tiSound.bDone) {
			tiSound.bDone = 0;
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
	tiSound.bExec = 0;
	} while (!tiSound.bDone);
tiSound.bDone = 0;
return 0;
}

//------------------------------------------------------------------------------

void WaitForSoundThread (time_t nTimeout)
{
time_t t0 = (nTimeout < 0) ? 0 : (time_t) SDL_GetTicks ();
while (tiSound.pThread && tiSound.bExec && ((nTimeout < 0) || ((time_t) SDL_GetTicks () - t0 < nTimeout)))
	G3_SLEEP (1);
}

//------------------------------------------------------------------------------

bool HaveSoundThread (void)
{
return tiSound.pThread != NULL;
}

//------------------------------------------------------------------------------

void CreateSoundThread (void)
{
if (!tiSound.pThread) {
	memset (&tiSound, 0, sizeof (tiSound));
	tiSound.nId = 0;
	tiSound.pThread = SDL_CreateThread (SoundThread, &tiSound.nId);
	}
}

//------------------------------------------------------------------------------

void DestroySoundThread (void)
{
if (tiSound.pThread) {
	WaitForSoundThread (1000);
	tiSound.bDone = 1;
	CTimeout to (1000);
	do {
		G3_SLEEP (1);
		} while (tiSound.bDone && !to.Expired ());
	if (tiSound.bDone)
		SDL_KillThread (tiSound.pThread);
	tiSound.pThread = NULL;
	}	
}

//------------------------------------------------------------------------------

int32_t StartSoundThread (tSoundTask nTask)
{
#if 1
if (tiSound.pThread) {
#else
if (tiSound.pThread && gameData.appData.bUseMultiThreading [rtSound]) {
#endif
	WaitForSoundThread ();
	tiSound.nTask = nTask;
	tiSound.bExec = 1;
#if 0
	PrintLog (0, "running render threads (task: %d)\n", nTask);
#endif
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------
//eof
