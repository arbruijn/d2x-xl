#include "soundthreads.h"

tSoundThreadInfo tiSound;

//------------------------------------------------------------------------------

int _CDECL_ SoundThread (void *pThreadId)
{
	int	nId = *((int *) pThreadId);

do {
	while (!tiSound.ti.bExec) {
		G3_SLEEP (0);
		if (tiSound.ti.bDone)
			return 0;
		}
	if (tiSound.nTask == stOpenAudio) {
		DigiInit (tiSound.fSlowDown);
		}
	else if (tiSound.nTask == stCloseAudio) {
		DigiExit ();
		}
	else if (tiSound.nTask == stReconfigureAudio) {
		DigiExit ();
		DigiInit (tiSound.fSlowDown);
		if (tiSound.fSlowDown == 1.0f) {
			gameData.songs.tPos = gameData.songs.tSlowDown - gameData.songs.tStart + 
										 2 * (SDL_GetTicks () - gameData.songs.tSlowDown) / gameOpts->gameplay.nSlowMotionSpeedup;
			}
		else {
			gameData.songs.tSlowDown = SDL_GetTicks ();
			gameData.songs.tPos = gameData.songs.tSlowDown - gameData.songs.tStart;
			}
		PlayLevelSong (gameData.missions.nCurrentLevel, 1);
		}
	tiSound.ti.bExec = 0;
	} while (!tiSound.ti.bDone);
return 0;
}

//------------------------------------------------------------------------------

void StartSoundThread (void)
{
memset (&tiSound, 0, sizeof (tiSound));
tiSound.ti.nId = 0;
tiSound.ti.pThread = SDL_CreateThread (SoundThread, &tiSound.ti.nId);
}

//------------------------------------------------------------------------------

void EndSoundThread (void)
{
tiSound.ti.bDone = 1;
G3_SLEEP (100);
}

//------------------------------------------------------------------------------

int RunSoundThread (int nTask)
{
	time_t	t1 = 0;
#ifdef _DEBUG
	static	int nLockups = 0;
#endif

#if 1
t1 = clock ();
while (tiSound.ti.bExec && (clock () - t1 < 1000))
	G3_SLEEP (0);
#endif
tiSound.nTask = nTask;
tiSound.ti.bExec = 1;
#if 0
LogErr ("running render threads (task: %d)\n", nTask);
#endif
return 1;
}

//------------------------------------------------------------------------------
//eof
