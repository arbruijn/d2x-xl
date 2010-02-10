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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#include "descent.h"
#include "timer.h"
#include "automap.h"
#include "trackobject.h"
#include "newdemo.h"
#include "text.h"

// limit framerate to 30 while recording demo and to 40 when in automap and framerate display is disabled
#define MAXFPS		((gameData.demo.nState == ND_STATE_RECORDING) ? 30 : \
                   (automap.Display () && !(automap.Radar () || (gameStates.render.bShowFrameRate == 1))) ? 40 : \
						 (gameOpts->render.n3DGlasses == 5) ? 2 * gameOpts->render.nMaxFPS : gameOpts->render.nMaxFPS)

//------------------------------------------------------------------------------

void StopTime (void)
{
if (pfnTIRStop)
	pfnTIRStop ();
if (++gameData.time.nPaused == 1) {
	fix xTime = TimerGetFixedSeconds ();
	gameData.time.xSlack = xTime - gameData.time.xLast;
	if (gameData.time.xSlack < 0) {
#if defined (TIMER_TEST) && defined (_DEBUG)
		Int3 ();		//get Matt!!!!
#endif
		gameData.time.xLast = 0;
		}
#if defined (TIMER_TEST) && defined (_DEBUG)
	gameData.time.xStopped = xTime;
	#endif
	}
#if defined (TIMER_TEST) && defined (_DEBUG)
gameData.time.xStops++;
#endif
}

//------------------------------------------------------------------------------

void StartTime (int bReset)
{
if (gameData.time.nPaused <= 0)
	return;
if (bReset)
	gameData.time.nPaused = 1;
if (!--gameData.time.nPaused) {
	fix xTime = TimerGetFixedSeconds ();
#if defined (TIMER_TEST) && defined (_DEBUG)
	if (gameData.time.xLast < 0)
		Int3 ();		//get Matt!!!!
#endif
	gameData.time.xLast = xTime - gameData.time.xSlack;
#if defined (TIMER_TEST) && defined (_DEBUG)
	gameData.time.xStarted = time;
#endif
	}
#if defined (TIMER_TEST) && defined (_DEBUG)
gameData.time.xStarts++;
#endif
if (pfnTIRStart)
	pfnTIRStart ();
}

//------------------------------------------------------------------------------

int TimeStopped (void)
{
return gameData.time.nPaused > 0;
}

//------------------------------------------------------------------------------

void ResetTime (void)
{
gameData.time.xFrame = 0;
gameData.time.xLast = TimerGetFixedSeconds ();
}

//------------------------------------------------------------------------------

#define EXACT_FRAME_TIME	1

void CalcFrameTime (void)
{
if (gameData.app.bGamePaused) {
	gameData.time.xLast = TimerGetFixedSeconds ();
	gameData.time.xFrame = 0;
	gameData.time.xRealFrame = 0;
	return;
	}

fix 	timerValue,
		xLastFrameTime = gameData.time.xFrame;
GetSlowTicks ();

#if EXACT_FRAME_TIME

	static float fSlack = 0;

	int	nFrameTime, nMinFrameTime, nDeltaTime;

if (MAXFPS <= 1) 
	nDeltaTime = 0;
else {
#ifdef RELEASE
	if (!gameOpts->app.bExpertMode && (gameOpts->render.nMaxFPS > 1))
		gameOpts->render.nMaxFPS = MAX_FRAMERATE;
#endif
	if (!gameData.time.tLast)
		nDeltaTime = 0;
	else {
		nFrameTime = gameStates.app.nSDLTicks - gameData.time.tLast;
		nMinFrameTime = 1000 / MAXFPS;
		nDeltaTime = nMinFrameTime - nFrameTime;
		fSlack += 1000.0f / MAXFPS - nMinFrameTime;
		if (fSlack >= 1) {
			nDeltaTime += int (fSlack);
			fSlack -= int (fSlack);
			}
		if (0 < nDeltaTime)
			G3_SLEEP (nDeltaTime);
		}
	}
timerValue = MSEC2X (gameStates.app.nSDLTicks);

#else

fix xMinFrameTime = ((MAXFPS > 1) ? I2X (1) / MAXFPS : 1);
do {
	timerValue = TimerGetFixedSeconds ();
   gameData.time.xFrame = timerValue - gameData.time.xLast;
	if (MAXFPS < 2)
		break;
	G3_SLEEP (1);
	} while (gameData.time.xFrame < xMinFrameTime);

#endif

gameData.time.xFrame = timerValue - gameData.time.xLast;
gameData.time.xRealFrame = gameData.time.xFrame;
if (gameStates.app.cheats.bTurboMode)
	gameData.time.xFrame *= 2;
gameData.time.xLast = timerValue;

#if EXACT_FRAME_TIME

gameData.time.tLast = gameStates.app.nSDLTicks;
if (nDeltaTime > 0)
	gameData.time.tLast += nDeltaTime;

#else

gameData.time.tLast = SDL_GetTicks ();

#endif

if (gameData.time.xFrame < 0)						//if bogus frametimed:\temp\dm_test.
	gameData.time.xFrame = xLastFrameTime;		//d:\temp\dm_test.then use time from last frame
#if Arcade_mode
gameData.time.xFrame /= 2;
#endif
#if defined (TIMER_TEST) && defined (_DEBUG)
gameData.time.xStops = gameData.time.xStarts = 0;
#endif
//	Set value to determine whether homing missile can see target.
//	The lower frametime is, the more likely that it can see its target.
if (gameStates.limitFPS.bHomers)
	xMinTrackableDot = MIN_TRACKABLE_DOT;
else if (gameData.time.xFrame <= I2X (1)/64)
	xMinTrackableDot = MIN_TRACKABLE_DOT;	// -- 3* (I2X (1) - MIN_TRACKABLE_DOT)/4 + MIN_TRACKABLE_DOT;
else if (gameData.time.xFrame < I2X (1)/32)
	xMinTrackableDot = MIN_TRACKABLE_DOT + I2X (1)/64 - 2*gameData.time.xFrame;	// -- FixMul (I2X (1) - MIN_TRACKABLE_DOT, I2X (1)-4*gameData.time.xFrame) + MIN_TRACKABLE_DOT;
else if (gameData.time.xFrame < I2X (1)/4)
	xMinTrackableDot = MIN_TRACKABLE_DOT + I2X (1)/64 - I2X (1)/16 - gameData.time.xFrame;	// -- FixMul (I2X (1) - MIN_TRACKABLE_DOT, I2X (1)-4*gameData.time.xFrame) + MIN_TRACKABLE_DOT;
else
	xMinTrackableDot = MIN_TRACKABLE_DOT + I2X (1)/64 - I2X (1)/8;
}

//-----------------------------------------------------------------------------

void GetSlowTicks (void)
{
gameStates.app.nSDLTicks = SDL_GetTicks ();
gameStates.app.tick40fps.nTime = gameStates.app.nSDLTicks - gameStates.app.tick40fps.nLastTick;
if ((gameStates.app.tick40fps.bTick = (gameStates.app.tick40fps.nTime >= 25)))
	gameStates.app.tick40fps.nLastTick = gameStates.app.nSDLTicks;
gameStates.app.tick60fps.nTime = gameStates.app.nSDLTicks - gameStates.app.tick60fps.nLastTick;
if ((gameStates.app.tick60fps.bTick = (gameStates.app.tick60fps.nTime >= (50 + ++gameStates.app.tick60fps.nSlack) / 3))) {
	gameStates.app.tick60fps.nLastTick = gameStates.app.nSDLTicks;
	gameStates.app.tick60fps.nSlack %= 3;
	}
}

//------------------------------------------------------------------------------

void GameDrawTimeLeft (void)
{
	char temp_string[30];
	fix timevar;
	int i;
	static int nId = 0;

fontManager.SetCurrent (GAME_FONT);    //GAME_FONT
fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
timevar = I2X (netGame.GetPlayTimeAllowed () * 5 * 60);
i = X2I (timevar - gameStates.app.xThisLevelTime) + 1;
sprintf (temp_string, TXT_TIME_LEFT, i);
if (i >= 0)
	nId = GrString (0, 32, temp_string, &nId);
}

//-----------------------------------------------------------------------------
//eof
