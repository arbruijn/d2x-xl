/*
 *
 * SDL mouse driver.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "descent.h"
#include "fix.h"
#include "timer.h"
#include "event.h"
#include "mouse.h"
#include "input.h"

#ifdef WIN32_WCE
# define LANDSCAPE
#endif

#define Z_SENSITIVITY 100

tMouseInfo mouseData;

//------------------------------------------------------------------------------

void MouseInit (void)
{
memset (&mouseData, 0, sizeof (mouseData));
}

//------------------------------------------------------------------------------

#if DBG
int32_t nDbgButton = -1;
#endif

void MouseButtonHandler (SDL_MouseButtonEvent *mbe)
{
	// to bad, SDL buttons use a different mapping as descent expects,
	// this is at least true and tested for the first three buttons 
	int32_t button_remap[11] = {
		D2_MB_LEFT,
		D2_MB_MIDDLE,
		D2_MB_RIGHT,
		D2_MB_Z_UP,
		D2_MB_Z_DOWN,
		D2_MB_PITCH_BACKWARD,
		D2_MB_PITCH_FORWARD,
		D2_MB_BANK_LEFT,
		D2_MB_BANK_RIGHT,
		D2_MB_HEAD_LEFT,
		D2_MB_HEAD_RIGHT
	};

	int32_t button = button_remap [mbe->button - 1]; // -1 since SDL seems to start counting at 1
	struct tMouseButton *mb = mouseData.buttons + button;
	fix xCurTime = TimerGetFixedSeconds ();

if (mbe->state == SDL_PRESSED) {
#if DBG
if (button == nDbgButton)
	nDbgButton = nDbgButton;
#endif
	mb->pressed = 1;
	mb->xPrevTimeWentDown = mb->xTimeWentDown;
	mb->xTimeWentDown = xCurTime;
	mb->numDowns++;

	if (button == D2_MB_Z_UP) {
		mouseData.dz += Z_SENSITIVITY;
		mouseData.z += Z_SENSITIVITY;
		mb->rotated = 1;
		}
	else if (button == D2_MB_Z_DOWN) {
		mouseData.dz -= Z_SENSITIVITY;
		mouseData.z -= Z_SENSITIVITY;
		mb->rotated = 1;
		}
	}
else {
#if DBG
if (button == nDbgButton)
	nDbgButton = nDbgButton;
#endif
	mb->pressed = 0;
	mb->xTimeHeldDown += xCurTime - mb->xTimeWentDown;
	mb->numUps++;
	mouseData.bDoubleClick = xCurTime - mb->xPrevTimeWentDown < I2X (1) / 2;
	}
}

//------------------------------------------------------------------------------

void MouseMotionHandler (SDL_MouseMotionEvent *mme)
{
#ifdef LANDSCAPE
	mouseData.dy += mme->xrel;
	mouseData.dx += mme->yrel;
	mouseData.y += mme->xrel;
	mouseData.x += mme->yrel;
#else
	if (mme->xrel || mme->yrel) {
		mouseData.dx += mme->xrel;
		mouseData.dy += mme->yrel;
		mouseData.x += mme->xrel;
		mouseData.y += mme->yrel;
		}
#endif
}

//------------------------------------------------------------------------------

void MouseFlush (void)	// clears all mouse events...
{
	int32_t i;
	fix xCurTime;

event_poll (SDL_MOUSEEVENTMASK);
xCurTime = TimerGetFixedSeconds ();
memset (&mouseData, 0, sizeof (mouseData));
for (i = 0; i < MOUSE_MAX_BUTTONS; i++)
	mouseData.buttons [i].xTimeWentDown = xCurTime;
SDL_GetMouseState (&mouseData.x, &mouseData.y); // necessary because polling only gives us the delta.
}

//========================================================================

void MouseGetPos (int32_t *x, int32_t *y)
{
#ifdef WIN32_WCE // needed here only for touchpens?
# ifdef LANDSCAPE
	SDL_GetMouseState (&mouseData.y, &mouseData.x);
# else
//	SDL_GetMouseState(&mouseData.x, &mouseData.y);
# endif
#endif
*x = mouseData.x;
*y = mouseData.y;
}

//------------------------------------------------------------------------------

void MouseGetDelta (int32_t *dx, int32_t *dy)
{
if (gameOpts->legacy.bInput)
   event_poll (SDL_MOUSEEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
*dx = mouseData.dx;
*dy = mouseData.dy;
mouseData.dx = 0;
mouseData.dy = 0;
}

//------------------------------------------------------------------------------

void MouseGetPosZ (int32_t *x, int32_t *y, int32_t *z)
{
*x = mouseData.x;
*y = mouseData.y;
*z = mouseData.z;
}

//------------------------------------------------------------------------------

void MouseGetDeltaZ (int32_t *dx, int32_t *dy, int32_t *dz)
{
*dx = mouseData.dx;
*dy = mouseData.dy;
*dz = mouseData.dz;
mouseData.dx = 0;
mouseData.dy = 0;
mouseData.dz = 0;
}

//------------------------------------------------------------------------------

int32_t MouseGetButtons (void)
{
	uint32_t flag = 1;
	int32_t status = 0;

for (int32_t i = 0; i < MOUSE_MAX_BUTTONS; i++) {
	if (mouseData.buttons [i].pressed || mouseData.buttons [i].rotated)
		status |= flag;
	flag <<= 1;
	}
return status;
}

//------------------------------------------------------------------------------

void MouseGetCybermanPos (int32_t *x, int32_t *y)
{
}

//------------------------------------------------------------------------------
// Returns how many times this button has went down since last call
int32_t MouseButtonDownCount (int32_t button)
{
	int32_t count;

count = mouseData.buttons [button].numDowns;
mouseData.buttons [button].numDowns = 0;
return count;
}

//------------------------------------------------------------------------------
// Returns how long this button has been down since last call.
fix MouseButtonDownTime (int32_t button)
{
	fix timeDown, time;

if (!mouseData.buttons [button].pressed) {
	timeDown = mouseData.buttons [button].xTimeHeldDown;
	if (!timeDown && mouseData.buttons [button].rotated)
		timeDown = (fix) (MouseButtonDownCount (button) * controls.PollTime ());
	mouseData.buttons [button].xTimeHeldDown = 0;
	}
else {
	time = TimerGetFixedSeconds();
	timeDown = time - mouseData.buttons [button].xTimeHeldDown;
	mouseData.buttons [button].xTimeHeldDown = time;
	}
return timeDown;
}

//------------------------------------------------------------------------------
// Returns 1 if this button is currently down
int32_t MouseButtonState (int32_t button)
{
if (gameOpts->legacy.bInput)
   event_poll(SDL_MOUSEEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
int32_t h = mouseData.buttons [button].rotated;
mouseData.buttons [button].rotated = 0;
return mouseData.buttons [button].pressed || h;
}

//------------------------------------------------------------------------------
