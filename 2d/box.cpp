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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "ogl_defs.h"
#include "gr.h"

//------------------------------------------------------------------------------ 

void SWDrawEmptyRect (int left, int top, int right, int bot)
{
	int i, d, c = COLOR.index;
	ubyte* ptr1 = DATA + ROWSIZE *top+left;
	ubyte* ptr2 = ptr1;

d = right - left;
for (i = top; i <= bot; i++) {
	ptr2 [0] = c;
	ptr2 [d] = c;
	ptr2 += ROWSIZE;
	}

ptr2 = ptr1;
d = (bot - top) * ROWSIZE;
for (i = 1; i < (right-left); i++) {
	ptr2 [i + 0] = c;
	ptr2 [i + d] = c;
	}
}

//------------------------------------------------------------------------------ 

void DrawEmptyRect (int left, int top, int right, int bot)
{
if (MODE == BM_LINEAR)
	SWDrawEmptyRect (left, top, right, bot);
else
	OglDrawEmptyRect (left, top, right, bot);
}

//------------------------------------------------------------------------------ 
// eof
