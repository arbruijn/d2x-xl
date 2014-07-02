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
#if defined(__unix__) || defined(__macosx__)
#	include <sys/time.h>
#endif

#include "descent.h"
#include "timer.h"
#include "automap.h"
#include "trackobject.h"
#include "newdemo.h"
#include "text.h"

// limit framerate to 30 while recording demo and to 40 when in automap and framerate display is disabled
#define MAXFPS		((gameData.demo.nState == ND_STATE_RECORDING) ? 30 : \
                   (automap.Display () && !(automap.Radar () || (gameStates.render.bShowFrameRate == 1))) ? 40 : \
						 ((gameOpts->render.stereo.nGlasses == GLASSES_SHUTTER_NVIDIA) && (gameOpts->render.nMaxFPS < 120)) ? 2 * gameOpts->render.nMaxFPS : gameOpts->render.nMaxFPS)

#define EXACT_FRAME_TIME	1

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class CGenericFrameTime {
	protected:
		time_t	m_tMinFrame;

	protected:
		virtual void Setup (void) = 0;

		virtual void GetTick (void) = 0;

		virtual time_t Elapsed (void) = 0;

	public:
		virtual void Compute (void) = 0;
	};

//------------------------------------------------------------------------------

template <class _T>
class CFrameTime : public CGenericFrameTime {
	protected:
		_T		m_tLast;
		_T		m_tick;

	protected:
		virtual void Setup (void) {
			GetTick ();
			m_tLast = m_tick;
			}

	public:
		virtual void Compute (void) {
			while (Elapsed () < m_tMinFrame)
				G3_SLEEP (0);
			m_tLast = m_tick;
			}
	};

//------------------------------------------------------------------------------

#ifdef _WIN32

class CWindowsFrameTime : public CFrameTime <LARGE_INTEGER> {
	private:
		time_t			m_ticksPerMSec;
		LARGE_INTEGER	m_ticksPerSec;
		time_t			m_tError;

	protected:
		virtual void Setup (void);
		virtual void GetTick (void);
		virtual time_t Elapsed (void);

	public:
		virtual void Compute (void);
		explicit CWindowsFrameTime() { Setup (); }
	};

//------------------------------------------------------------------------------

void CWindowsFrameTime::Setup (void)
{
QueryPerformanceFrequency (&m_ticksPerSec);
CFrameTime::Setup ();
m_tError = 0;
}

//------------------------------------------------------------------------------

void CWindowsFrameTime::GetTick (void)
{
QueryPerformanceCounter (&m_tick);
}

//------------------------------------------------------------------------------

time_t CWindowsFrameTime::Elapsed (void)
{
GetTick ();
return time_t (m_tick.QuadPart - m_tLast.QuadPart);
}

//------------------------------------------------------------------------------

void CWindowsFrameTime::Compute (void)
{
m_ticksPerMSec = time_t (m_ticksPerSec.QuadPart) / 1000;
m_tError += time_t (m_ticksPerSec.QuadPart) - m_ticksPerMSec * 1000;
time_t tSlack = m_tError / m_ticksPerMSec;
if (tSlack > 0) {
	m_ticksPerMSec += tSlack;
	m_tError -= tSlack * m_ticksPerMSec;
	}
m_tMinFrame = time_t (m_ticksPerSec.QuadPart / LONGLONG (MAXFPS));
CFrameTime<LARGE_INTEGER>::Compute ();
}

//------------------------------------------------------------------------------

#elif defined (__unix__) || defined(__macosx__)

class CUnixFrameTime : public CFrameTime <int64_t> {
	protected:
		virtual void GetTick (void);
		virtual time_t Elapsed (void);

	public:
		virtual void Setup (void);

		explicit CUnixFrameTime() { Setup (); }
	};

//------------------------------------------------------------------------------

void CUnixFrameTime::Setup (void)
{
m_tMinFrame = time_t (1000000 / MAXFPS);
CFrameTime<int64_t>::Setup ();
}

//------------------------------------------------------------------------------

void CUnixFrameTime::GetTick (void)
{
struct timeval t;
gettimeofday (&t, NULL);
m_tick = int64_t (t.tv_sec) * 1000000 + int64_t (t.tv_usec);
}

//------------------------------------------------------------------------------

time_t CUnixFrameTime::Elapsed (void)
{
GetTick ();
return time_t (m_tick - m_tLast);
}

//------------------------------------------------------------------------------

#else

class CSDLFrameTime : public CFrameTime <time_t> {
	protected:
		virtual void GetTick (void);
		virtual time_t FrameTime (void);

	public:
		virtual void Compute (void);

		explicit CSDLFrameTime() { Setup (); }
	};

//------------------------------------------------------------------------------

void CSDLFrameTime::GetTick (void)
{
m_tick = SDL_GetTicks ();
}

//------------------------------------------------------------------------------

time_t CSDLFrameTime::Elapsed (void)
{
GetTick ();
return time_t (m_tick - m_tLast);
}

//------------------------------------------------------------------------------

void CSDLFrameTime::Compute (void)
{
m_tMinFrame = 1000 / MAXFPS;
CFrameTime<time_t>::Compute ();
}

#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class CFrameTimeFactory {
	private:
		static CFrameTimeFactory* m_instance;
		static CGenericFrameTime* m_timer;

	protected:
		explicit CFrameTimeFactory() {}

	public:
		static CFrameTimeFactory* GetInstance (void) {
			if (!m_instance)
				m_instance = new CFrameTimeFactory;
			return m_instance;
			}

		static CGenericFrameTime* GetTimer (void) {
			if (!m_timer)
#ifdef _WIN32
				m_timer = new CWindowsFrameTime ();
#elif defined (__unix__) || defined(__macosx__)
				m_timer = new CUnixFrameTime ();
#else
				m_timer = new CSDLFrameTime ();
#endif
			return m_timer;
			}
	};

CFrameTimeFactory* CFrameTimeFactory::m_instance = NULL;
CGenericFrameTime* CFrameTimeFactory::m_timer = NULL;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


void StopTime (void)
{
if (pfnTIRStop)
	pfnTIRStop ();
if (++gameData.time.nPaused == 1) {
	gameData.time.xStopped = SDL_GetTicks ();
	fix xTime = TimerGetFixedSeconds ();
	gameData.time.xSlack = xTime - gameData.time.xLast;
	if (gameData.time.xSlack < 0)
		gameData.time.xLast = 0;
	}
}

//------------------------------------------------------------------------------

void StartTime (int32_t bReset)
{
if (gameData.time.nPaused <= 0)
	return;
if (bReset)
	gameData.time.nPaused = 1;
if (!--gameData.time.nPaused) {
	fix xTime = TimerGetFixedSeconds ();
	gameData.time.xLast = xTime - gameData.time.xSlack;
	gameData.physics.fLastTick += float (SDL_GetTicks () - gameData.time.xStopped);
	}
if (pfnTIRStart)
	pfnTIRStart ();
}

//------------------------------------------------------------------------------

int32_t TimeStopped (void)
{
return gameData.time.nPaused > 0;
}

//------------------------------------------------------------------------------

void ResetTime (void)
{
gameData.time.SetTime (0);
gameData.time.xLast = TimerGetFixedSeconds ();
}

//------------------------------------------------------------------------------

void CalcFrameTime (void)
{
if (gameData.app.bGamePaused) {
	gameData.time.xLast = TimerGetFixedSeconds ();
	gameData.time.SetTime (0);
	gameData.time.xRealFrame = 0;
	return;
	}

fix 	timerValue,
		xLastFrameTime = gameData.time.xFrame;

GetSlowTicks ();

#if EXACT_FRAME_TIME

	int32_t nDeltaTime;

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
#if 1
		CFrameTimeFactory::GetInstance ()->GetTimer ()->Compute ();
#elif defined(_WIN32)
		static time_t tError = 0, ticksPerMSec = 0;
		static LARGE_INTEGER tLast = {0}, ticksPerSec;

		if (!ticksPerMSec) 
			QueryPerformanceFrequency (&ticksPerSec);
		ticksPerMSec = time_t (ticksPerSec.QuadPart) / 1000;
		tError += time_t (ticksPerSec.QuadPart) - ticksPerMSec * 1000;
		time_t tSlack = tError / ticksPerMSec;
		if (tSlack > 0) {
			ticksPerMSec += tSlack;
			tError -= tSlack * ticksPerMSec;
			}
		LARGE_INTEGER tick;
		time_t tFrame, tMinFrame = time_t (ticksPerSec.QuadPart / LONGLONG (MAXFPS));
		for (;;) {
  			QueryPerformanceCounter (&tick);
			tFrame = time_t (tick.QuadPart - tLast.QuadPart);
			if (tFrame >= tMinFrame) 
				break;
			G3_SLEEP (0);
			}
		tLast = tick;
#elif defined(__unix__) || defined(__macosx__)
		static int64_t tLast = 0;

		int64_t tick, tFrame, tMinFrame = int64_t (1000000 / MAXFPS);
		for (;;) {
  			tFrame = tick - tLast;
			if (tFrame >= tMinFrame) 
				break;
			G3_SLEEP (0);
			}
		tLast = tick;
#else
		static float fSlack = 0;

		int32_t nFrameTime = gameStates.app.nSDLTicks [0] - gameData.time.tLast;
		int32_t nMinFrameTime = 1000 / MAXFPS;
		nDeltaTime = nMinFrameTime - nFrameTime;
		fSlack += 1000.0f / MAXFPS - nMinFrameTime;
		if (fSlack >= 1.0f) {
			nDeltaTime += int32_t (fSlack);
			fSlack -= int32_t (fSlack);
			}
		if (0 < nDeltaTime)
			G3_SLEEP (nDeltaTime);
		else
			nDeltaTime = 0;
#endif
		}
	}

timerValue = MSEC2X (gameStates.app.nSDLTicks [0]);

#else

fix xMinFrameTime = ((MAXFPS > 1) ? I2X (1) / MAXFPS : 1);
do {
	timerValue = TimerGetFixedSeconds ();
   gameData.time.SetTime (timerValue - gameData.time.xLast);
	if (MAXFPS < 2)
		break;
	G3_SLEEP (0);
	} while (gameData.time.xFrame < xMinFrameTime);

#endif

gameData.time.SetTime (timerValue - gameData.time.xLast);
gameData.time.xRealFrame = gameData.time.xFrame;
if (gameStates.app.cheats.bTurboMode)
	gameData.time.xFrame *= 2;
gameData.time.xLast = timerValue;

#if EXACT_FRAME_TIME
#	ifdef _WIN32
gameData.time.tLast = SDL_GetTicks ();
#else
if (nDeltaTime > 0)
	gameData.time.tLast += nDeltaTime;
#	endif
#else

gameData.time.tLast = SDL_GetTicks ();

#endif

if (gameData.time.xFrame < 0)						//if bogus frametimed:\temp\dm_test.
	gameData.time.SetTime (xLastFrameTime);	//d:\temp\dm_test.then use time from last frame
#if Arcade_mode
gameData.time.xFrame /= 2;
#endif
//	Set value to determine whether homing missile can see target.
//	The lower frametime is, the more likely that it can see its target.
#if 0
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
#endif
}

//-----------------------------------------------------------------------------

void GetSlowTicks (void)
{
gameData.time.tLast = gameStates.app.nSDLTicks [0];
gameStates.app.nSDLTicks [0] = SDL_GetTicks ();
gameStates.app.tick40fps.nTime = gameStates.app.nSDLTicks [0] - gameStates.app.tick40fps.nLastTick;
if ((gameStates.app.tick40fps.bTick = (gameStates.app.tick40fps.nTime >= 25)))
	gameStates.app.tick40fps.nLastTick = gameStates.app.nSDLTicks [0];
#if 0
gameStates.app.tick60fps.nTime = gameStates.app.nSDLTicks [0] - gameStates.app.tick60fps.nLastTick;
if ((gameStates.app.tick60fps.bTick = (gameStates.app.tick60fps.nTime >= (50 + ++gameStates.app.tick60fps.nSlack) / 3))) {
	gameStates.app.tick60fps.nLastTick = gameStates.app.nSDLTicks [0];
	gameStates.app.tick60fps.nSlack %= 3;
	}
#endif
}

//------------------------------------------------------------------------------

void GameDrawTimeLeft (void)
{
	char temp_string[30];
	fix timevar;
	int32_t i;
	static int32_t nId = 0;

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
