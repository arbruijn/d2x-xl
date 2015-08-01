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
#define EXACT_FRAME_TIME	1

//------------------------------------------------------------------------------

int32_t MaxFPS (void)
{
if (gameData.demoData.nState == ND_STATE_RECORDING) 
	return 30;
if (automap.Active () && !(automap.Radar () || (gameStates.render.bShowFrameRate == 1)))
	return 40;
if ((gameOpts->render.stereo.nGlasses == GLASSES_SHUTTER_NVIDIA) && (gameOpts->render.nMaxFPS < 120)) 
	return 2 * gameOpts->render.nMaxFPS;
return gameOpts->render.nMaxFPS;
}

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
		CGenericFrameTime () : m_tMinFrame (0) {}
		virtual ~CGenericFrameTime () {}
		virtual void Compute (int32_t fps = 0) = 0;
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
		CFrameTime () {}
		virtual ~CFrameTime () {}
		virtual void Compute (int32_t fps = 0) {
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
		virtual void Compute (int32_t fps = 0);
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

void CWindowsFrameTime::Compute (int32_t fps)
{
m_ticksPerMSec = time_t (m_ticksPerSec.QuadPart) / 1000;
m_tError += time_t (m_ticksPerSec.QuadPart) - m_ticksPerMSec * 1000;
time_t tSlack = m_tError / m_ticksPerMSec;
if (tSlack > 0) {
	m_ticksPerMSec += tSlack;
	m_tError -= tSlack * m_ticksPerMSec;
	}
m_tMinFrame = fps ? time_t (m_ticksPerSec.QuadPart / LONGLONG (fps)) : 0;
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
		virtual void Compute (int32_t fps = 0);

		explicit CUnixFrameTime() { Setup (); }
	};

//------------------------------------------------------------------------------

void CUnixFrameTime::Setup (void)
{
CFrameTime<int64_t>::Setup ();
}

//------------------------------------------------------------------------------

void CUnixFrameTime::Compute (int32_t fps)
{
m_tMinFrame = fps ? time_t (1000000 / fps) : 0;
CFrameTime<int64_t>::Compute ();
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
		virtual void Compute (int32_t fps = 0);

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

void CSDLFrameTime::Compute (int32_t fps = 0)
{
m_tMinFrame = fps ? 1000 / fps : 0;
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
if (++gameData.timeData.nPaused == 1) {
	gameData.timeData.xStopped = SDL_GetTicks ();
	fix xTime = TimerGetFixedSeconds ();
	gameData.timeData.xSlack = xTime - gameData.timeData.xLast;
	if (gameData.timeData.xSlack < 0)
		gameData.timeData.xLast = 0;
	}
}

//------------------------------------------------------------------------------

void StartTime (int32_t bReset)
{
if (gameData.timeData.nPaused <= 0)
	return;
if (bReset)
	gameData.timeData.nPaused = 1;
if (!--gameData.timeData.nPaused) {
	fix xTime = TimerGetFixedSeconds ();
	gameData.timeData.xLast = xTime - gameData.timeData.xSlack;
	gameData.physicsData.fLastTick += float (SDL_GetTicks () - gameData.timeData.xStopped);
	}
if (pfnTIRStart)
	pfnTIRStart ();
}

//------------------------------------------------------------------------------

int32_t TimeStopped (void)
{
return gameData.timeData.nPaused > 0;
}

//------------------------------------------------------------------------------

void ResetTime (void)
{
gameData.timeData.SetTime (0);
gameData.timeData.xLast = TimerGetFixedSeconds ();
}

//------------------------------------------------------------------------------

void CalcFrameTime (int32_t fps)
{
if (gameData.appData.bGamePaused) {
	gameData.timeData.xLast = TimerGetFixedSeconds ();
	gameData.timeData.SetTime (0);
	gameData.timeData.xRealFrame = 0;
	return;
	}

fix 	timerValue,
		xLastFrameTime = gameData.timeData.xFrame;

GetSlowTicks ();

if (fps <= 0)
	fps = MaxFPS ();

#if EXACT_FRAME_TIME

	int32_t nDeltaTime;

if (fps <= 1) 
	nDeltaTime = 0;
else {
#ifdef RELEASE
	if (!gameOpts->app.bExpertMode && (gameOpts->render.nMaxFPS > 1))
		gameOpts->render.nMaxFPS = MAX_FRAMERATE;
#endif
	if (!gameData.timeData.tLast)
		nDeltaTime = 0;
	else 
		CFrameTimeFactory::GetInstance ()->GetTimer ()->Compute (fps);
	}

timerValue = MSEC2X (gameStates.app.nSDLTicks [0]);

#else

fix xMinFrameTime = ((fps > 1) ? I2X (1) / fps : 1);
do {
	timerValue = TimerGetFixedSeconds ();
   gameData.timeData.SetTime (timerValue - gameData.timeData.xLast);
	if (fps < 2)
		break;
	G3_SLEEP (0);
	} while (gameData.timeData.xFrame < xMinFrameTime);

#endif

gameData.timeData.SetTime (timerValue - gameData.timeData.xLast);
gameData.timeData.xRealFrame = gameData.timeData.xFrame;
if (gameStates.app.cheats.bTurboMode)
	gameData.timeData.xFrame *= 2;
gameData.timeData.xLast = timerValue;

#if EXACT_FRAME_TIME
#	ifdef _WIN32
gameData.timeData.tLast = SDL_GetTicks ();
#else
if (nDeltaTime > 0)
	gameData.timeData.tLast += nDeltaTime;
#	endif
#else

gameData.timeData.tLast = SDL_GetTicks ();

#endif

if (gameData.timeData.xFrame < 0)						//if bogus frametimed:\temp\dm_test.
	gameData.timeData.SetTime (xLastFrameTime);	//d:\temp\dm_test.then use time from last frame
#if Arcade_mode
gameData.timeData.xFrame /= 2;
#endif
//	Set value to determine whether homing missile can see target.
//	The lower frametime is, the more likely that it can see its target.
#if 0
if (gameStates.limitFPS.bHomers)
	gameData.weaponData.xMinTrackableDot = MIN_TRACKABLE_DOT;
else if (gameData.timeData.xFrame <= I2X (1)/64)
	gameData.weaponData.xMinTrackableDot = MIN_TRACKABLE_DOT;	// -- 3* (I2X (1) - MIN_TRACKABLE_DOT)/4 + MIN_TRACKABLE_DOT;
else if (gameData.timeData.xFrame < I2X (1)/32)
	gameData.weaponData.xMinTrackableDot = MIN_TRACKABLE_DOT + I2X (1)/64 - 2*gameData.timeData.xFrame;	// -- FixMul (I2X (1) - MIN_TRACKABLE_DOT, I2X (1)-4*gameData.timeData.xFrame) + MIN_TRACKABLE_DOT;
else if (gameData.timeData.xFrame < I2X (1)/4)
	gameData.weaponData.xMinTrackableDot = MIN_TRACKABLE_DOT + I2X (1)/64 - I2X (1)/16 - gameData.timeData.xFrame;	// -- FixMul (I2X (1) - MIN_TRACKABLE_DOT, I2X (1)-4*gameData.timeData.xFrame) + MIN_TRACKABLE_DOT;
else
	gameData.weaponData.xMinTrackableDot = MIN_TRACKABLE_DOT + I2X (1)/64 - I2X (1)/8;
#endif
}

//-----------------------------------------------------------------------------

void GetSlowTicks (void)
{
gameData.timeData.tLast = gameStates.app.nSDLTicks [0];
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
timevar = I2X (netGameInfo.GetPlayTimeAllowed () * 5 * 60);
i = X2I (timevar - gameStates.app.xThisLevelTime) + 1;
sprintf (temp_string, TXT_TIME_LEFT, i);
if (i >= 0)
	nId = GrString (0, 32, temp_string, &nId);
}

//-----------------------------------------------------------------------------
//eof
