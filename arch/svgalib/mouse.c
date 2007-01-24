/*
 * $Source: /cvs/cvsroot/d2x/arch/svgalib/mouse.c,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001/10/24 09:25:05 $
 *
 * SVGALib mouse support
 *
 * $Log: mouse.c,v $
 * Revision 1.1  2001/10/24 09:25:05  bradleyb
 * Moved input stuff to arch subdirs, as in d1x.
 *
 * Revision 1.3  2001/01/29 14:03:57  bradleyb
 * Fixed build, minor fixes
 *
 * Revision 1.2  2001/01/28 16:09:39  bradleyb
 * Removed duplicate files, and unified input headers.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include <vga.h>
#include <vgamouse.h>
#include "fix.h"
#include "timer.h"
#include "event.h"
#include "mouse.h"

#define MOUSE_MAX_BUTTONS       8

ubyte installed = 0;

struct mousebutton {
 ubyte pressed;
 fix time_wentDown;
 fix time_heldDown;
 uint numDowns;
 uint num_ups;
};

static struct mouseinfo {
 struct mousebutton buttons[MOUSE_MAX_BUTTONS];
//added on 10/17/98 by Hans de Goede for mouse functionality
 int min_x, min_y;
 int max_x, max_y;
 int delta_x, delta_y;
 int x,y;
//end this section addition - Hans
} Mouse;

void mouse_handler (int vga_button, int dx, int dy, int dz, int drx, int dry, int drz)
{
 int i;
 int button;
 int state;

 /* map bit 0,1,2 (R,M,L) to 1,2,0 */
 button = ((vga_button & 1) << 1) | ((vga_button & 2) << 1) | 
   ((vga_button & 4) >> 2);

 for (i = 0; i < 8; i++)
 { 
  state = button & (1 << i);
  if (!Mouse.buttons[i].pressed && state)
    {
      Mouse.buttons[i].time_wentDown = TimerGetFixedSeconds();
      Mouse.buttons[i].numDowns++;
    }
  else if (Mouse.buttons[i].pressed && !state)
    {
      Mouse.buttons[i].num_ups++;
    }
  
  Mouse.buttons[i].pressed = state;
 }

 Mouse.delta_x += dx;
 Mouse.delta_y += dy;
 Mouse.x += dx;
 Mouse.y += dy;
 if      (Mouse.x > Mouse.max_x) Mouse.x = Mouse.max_x;
 else if (Mouse.x < Mouse.min_x) Mouse.x = Mouse.min_x;
 if      (Mouse.y > Mouse.max_y) Mouse.y = Mouse.max_y;
 else if (Mouse.y < Mouse.min_y) Mouse.y = Mouse.min_y;
}

void d_mouse_close(void)
{
 if (installed)
	vga_setmousesupport(0);
 installed = 0;
}

int
d_mouse_init(int enable_cyberman)
{
 memset(&Mouse,0,sizeof(Mouse);
 vga_setmousesupport(1);
 if (!installed)
	atexit(d_mouse_close);
 installed = 1;
 return 0; /* should return # of buttons... dunno how w/svgalib */
}

void
mouse_set_limits( int x1, int y1, int x2, int y2 )
{
 event_poll();
 Mouse.min_x = x1;
 Mouse.min_y = y1;
 Mouse.max_x = x2;
 Mouse.max_y = y2;
}

void mouse_flush()	// clears all mice events...
{
 int i;
 fix currentTime;

 event_poll();

 currentTime = TimerGetFixedSeconds();
 for (i=0; i<MOUSE_MAX_BUTTONS; i++)
 {
   Mouse.buttons[i].pressed=0;
   Mouse.buttons[i].time_wentDown=currentTime;
   Mouse.buttons[i].time_heldDown=0;
   Mouse.buttons[i].num_ups=0;
   Mouse.buttons[i].numDowns=0;
 }
//added on 10/17/98 by Hans de Goede for mouse functionality 
 Mouse.delta_x = 0;
 Mouse.delta_y = 0;
 Mouse.x = 0;
 Mouse.y = 0;
//end this section addition - Hans
}

//========================================================================
void mouse_get_pos( int *x, int *y)
{
  event_poll();
  *x = Mouse.x;
  *y = Mouse.y;
}

void MouseGetDelta( int *dx, int *dy )
{
 event_poll();
 *dx = Mouse.delta_x;
 *dy = Mouse.delta_y;
 Mouse.delta_x = 0;
 Mouse.delta_y = 0;
}

int MouseGetButtons()
{
	int i;
        uint flag=1;
	int status = 0;

        event_poll();
	
	for (i=0; i<MOUSE_MAX_BUTTONS; i++ )
        {
		if (Mouse.buttons[i].pressed)
			status |= flag;
		flag <<= 1;
	}
	return status;
}

void mouse_set_pos( int x, int y)
{
 event_poll();
 Mouse.x=x;
 Mouse.y=y;
 if      (Mouse.x > Mouse.max_x) Mouse.x = Mouse.max_x;
 else if (Mouse.x < Mouse.min_x) Mouse.x = Mouse.min_x;
 if      (Mouse.y > Mouse.max_y) Mouse.y = Mouse.max_y;
 else if (Mouse.y < Mouse.min_y) Mouse.y = Mouse.min_y;
//end this section change - Hans
}

void mouse_get_cyberman_pos( int *x, int *y )
{
 // Shrug...
 event_poll();
}

// Returns how long this button has been down since last call.
fix MouseButtonDownTime(int button)
{
 fix timeDown, time;

 event_poll();

 if (!Mouse.buttons[button].pressed) {
   timeDown = Mouse.buttons[button].time_heldDown;
   Mouse.buttons[button].time_heldDown = 0;
 } else {
   time = TimerGetFixedSeconds();
   timeDown = time - Mouse.buttons[button].time_heldDown;
   Mouse.buttons[button].time_heldDown = time;
 }
 return timeDown;
}

// Returns how many times this button has went down since last call
int MouseButtonDownCount(int button)
{
  int count;

  event_poll();

  count = Mouse.buttons[button].numDowns;
  Mouse.buttons[button].numDowns = 0;

  return count;
}

// Returns 1 if this button is currently down
int MouseButtonState(int button)
{
  event_poll();
  return Mouse.buttons[button].pressed;
}
