#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include "descent.h"
#include "error.h"
#include "input.h"
#include "text.h"
#include "songs.h"
#include "slowmotion.h"
#include "soundthreads.h"

static int nSlowMotionChannel = -1;

//	-----------------------------------------------------------------------------------------------------------

void SetSlowMotionState (int i)
{
if (gameStates.gameplay.slowmo [i].nState) {
	gameStates.gameplay.slowmo [i].nState = -gameStates.gameplay.slowmo [i].nState;
	if (nSlowMotionChannel >= 0) {
		audio.StopSound (nSlowMotionChannel);
		nSlowMotionChannel = -1;
		}
	}
else if (gameStates.gameplay.slowmo [i].fSpeed > 1) {
	gameStates.gameplay.slowmo [i].nState = -1;
	}
else {
	gameStates.gameplay.slowmo [i].nState = 1;
	}
gameStates.gameplay.slowmo [i].tUpdate = gameStates.app.nSDLTicks [0];
}

//	-----------------------------------------------------------------------------------------------------------

void SlowMotionMessage (void)
{
if (gameStates.gameplay.slowmo [0].nState > 0) {
	if (gameOpts->sound.bUseSDLMixer)
		nSlowMotionChannel = audio.PlayWAV ("slowdown.wav");
	HUDInitMessage (TXT_SLOWING_DOWN);
	}
else if ((gameStates.gameplay.slowmo [0].nState < 0) ||
			((gameStates.gameplay.slowmo [0].nState == 0) && (gameStates.gameplay.slowmo [0].fSpeed == 1.0f)) || 
			(gameStates.gameplay.slowmo [1].nState < 0) || 
			((gameStates.gameplay.slowmo [1].nState == 0) && (gameStates.gameplay.slowmo [1].fSpeed == 1.0f))) {
	if (gameOpts->sound.bUseSDLMixer)
		nSlowMotionChannel = audio.PlayWAV ("speedup.wav");
	HUDInitMessage (TXT_SPEEDING_UP);
	}
else {
	if (gameOpts->sound.bUseSDLMixer)
		nSlowMotionChannel = audio.PlayWAV ("slowdown.wav");
	HUDInitMessage (TXT_SLOWING_DOWN);
	}
}

//	-----------------------------------------------------------------------------------------------------------

void InitBulletTime (int nState)
{
gameStates.gameplay.slowmo [1].nState = nState;
SetSlowMotionState (1);
}

//	-----------------------------------------------------------------------------------------------------------

void InitSlowMotion (int nState)
{
gameStates.gameplay.slowmo [0].nState = nState;
SetSlowMotionState (0);
}

//	-----------------------------------------------------------------------------------------------------------

int SlowMotionActive (void)
{
return gameStates.gameplay.slowmo [0].bActive =
		 (gameStates.gameplay.slowmo [0].nState > 0) || (gameStates.gameplay.slowmo [0].fSpeed > 1.0f);
}

//	-----------------------------------------------------------------------------------------------------------

int BulletTimeActive (void)
{
return gameStates.gameplay.slowmo [1].bActive =
		 SlowMotionActive () && 
		 ((gameStates.gameplay.slowmo [1].nState < 0) || (gameStates.gameplay.slowmo [1].fSpeed == 1));
}

//	-----------------------------------------------------------------------------------------------------------

void SlowMotionOff (void)
{
if (SlowMotionActive () && (gameStates.gameplay.slowmo [0].nState != -1)) {
	InitSlowMotion (1);
	if (!BulletTimeActive ())
		InitBulletTime (1);
	SlowMotionMessage ();
	}
}

//	-----------------------------------------------------------------------------------------------------------

void BulletTimeOn (void)
{
if (!BulletTimeActive ())
	InitBulletTime (1);
if (!SlowMotionActive ())
	InitSlowMotion (-1);
SlowMotionMessage ();
}

//	-----------------------------------------------------------------------------------------------------------

int ToggleSlowMotion (void)
{
if (gameData.reactor.bDestroyed)
	return 0;

	int	bSlowMotionOk = gameStates.app.cheats.bSpeed || ((LOCALPLAYER.Energy () > I2X (10)) && (LOCALPLAYER.flags & PLAYER_FLAGS_SLOWMOTION));
	int	bBulletTimeOk = bSlowMotionOk && (gameStates.app.cheats.bSpeed || (LOCALPLAYER.flags & (PLAYER_FLAGS_SLOWMOTION | PLAYER_FLAGS_BULLETTIME)));
	int	bSlowMotion = bSlowMotionOk && (controls [0].slowMotionCount > 0);
	int	bBulletTime = bBulletTimeOk && (controls [0].bulletTimeCount > 0);

controls [0].bulletTimeCount =
controls [0].slowMotionCount = 0;
#if 1//!DBG
if (SlowMotionActive ()) {
	if (!gameStates.app.cheats.bSpeed)
#if 0
		LOCALPLAYER.UpdateEnergy (-gameData.time.xFrame * (1 + BulletTimeActive ()));
#else
		LOCALPLAYER.UpdateEnergy (-((4 + gameStates.app.nDifficultyLevel) * gameData.time.xFrame * (1 + BulletTimeActive ())) / 6);
#endif
	if (!bSlowMotionOk) {
		if (gameStates.gameplay.slowmo [0].nState != -1) {
			InitSlowMotion (1);
			if (!BulletTimeActive () && (gameStates.gameplay.slowmo [1].nState != -1))
				InitBulletTime (1);
			SlowMotionMessage ();
			}
		return 0;
		}
	if (!bBulletTimeOk) {
		if (gameStates.gameplay.slowmo [1].nState != -1)
			InitBulletTime (-1);
		bBulletTime = 0;
		}
	}
#endif
if (!(bSlowMotion || bBulletTime))
	return 0;
if (bBulletTime) {	//toggle bullet time and slow motion
	if (SlowMotionActive ()) {
		if (BulletTimeActive ())
			InitSlowMotion (1);
		InitBulletTime (1);
		}
	else {
		InitSlowMotion (-1);
		if (!BulletTimeActive ())
			InitBulletTime (1);
		}
	}
else {
	if (SlowMotionActive ()) {
		if (BulletTimeActive ())
			InitBulletTime (-1);
		else {
			InitSlowMotion (1);
			if (!BulletTimeActive ())
				InitBulletTime (1);
			}
		}
	else {
		InitSlowMotion (-1);
		if (BulletTimeActive ())
			InitBulletTime (-1);
		}
	}
return 1;
}

//	-----------------------------------------------------------------------------------------------------------

void SpeedupSound (void)
{
if (!gameOpts->gameplay.nSlowMotionSpeedup)
	gameOpts->gameplay.nSlowMotionSpeedup = 6;
if (HaveSoundThread ()) {
	tiSound.fSlowDown = 1.0f;
	StartSoundThread (stReconfigureAudio);
	}
else {
	audio.Shutdown ();
	audio.Setup (1);
	time_t tSlowDown = songManager.SlowDown ();
	if (tSlowDown)
		songManager.SetPos (tSlowDown - songManager.Start () + 2 * (SDL_GetTicks () - tSlowDown) / gameOpts->gameplay.nSlowMotionSpeedup);
	else
		songManager.SetPos (SDL_GetTicks () - songManager.Start ());
	songManager.SetSlowDown (0);
	songManager.PlayLevelSong (missionManager.nCurrentLevel, 1);
	}
}

//	-----------------------------------------------------------------------------------------------------------

void SlowdownSound (void)
{
if (HaveSoundThread ()) {
	tiSound.fSlowDown = float (gameOpts->gameplay.nSlowMotionSpeedup) / 2;
	StartSoundThread (stReconfigureAudio);
	}
else {
	audio.Shutdown ();
	audio.Setup (float (gameOpts->gameplay.nSlowMotionSpeedup) / 2);
	songManager.SetSlowDown (SDL_GetTicks ());
#if DBG
	songManager.SetPos (0);
#else
	songManager.SetPos (songManager.SlowDown () - songManager.Start ());
#endif
	songManager.PlayLevelSong (missionManager.nCurrentLevel, 1);
	}
}

//	-----------------------------------------------------------------------------------------------------------

#define SLOWDOWN_SECS	2
#define SLOWDOWN_FPS		40

void DoSlowMotionFrame (void)
{
	int	i, bMsg;
	float	f, h;

if (gameStates.app.bNostalgia || IsMultiGame)
	return;
bMsg = ToggleSlowMotion ();
f = float (gameOpts->gameplay.nSlowMotionSpeedup) / 2;
h = (f - 1) / (SLOWDOWN_SECS * SLOWDOWN_FPS);
for (i = 0; i < 2; i++) {
	if (gameStates.gameplay.slowmo [i].nState && (gameStates.app.nSDLTicks [0] - gameStates.gameplay.slowmo [i].tUpdate > 25)) {
		gameStates.gameplay.slowmo [i].fSpeed += gameStates.gameplay.slowmo [i].nState * h;
		if (gameStates.gameplay.slowmo [i].fSpeed >= f) {
			gameStates.gameplay.slowmo [i].fSpeed = f;
			gameStates.gameplay.slowmo [i].nState = 0;
			if (!i) 
				SlowdownSound ();
			}
		else if (gameStates.gameplay.slowmo [i].fSpeed <= 1) {
			gameStates.gameplay.slowmo [i].fSpeed = 1;
			gameStates.gameplay.slowmo [i].nState = 0;
			if (!i)
				SpeedupSound ();
			}
		gameStates.gameplay.slowmo [i].tUpdate = gameStates.app.nSDLTicks [0];
		}
	}
if (bMsg)
	SlowMotionMessage ();
#if 0//DBG
HUDMessage (0, "%1.2f %1.2f %d", 
				gameStates.gameplay.slowmo [0].fSpeed, gameStates.gameplay.slowmo [1].fSpeed,
				gameStates.gameplay.slowmo [1].bActive);
#endif
}

//------------------------------------------------------------------------------
//eof
