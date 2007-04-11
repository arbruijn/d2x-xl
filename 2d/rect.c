/* $Id: rect.c,v 1.4 2002/10/10 18:55:32 btb Exp $ */
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
/*
 *
 * Graphical routines for drawing rectangles.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "u_mem.h"

#include "gr.h"
#include "grdef.h"
#include "ogl_init.h"

void GrURect(int left,int top,int right,int bot)
{
	int i;

	if (TYPE == BM_OGL) {
		OglURect(left,top,right,bot);
		return;
	}
	for ( i=top; i<=bot; i++ )
		gr_uscanline( left, right, i );
}

void GrRect(int left,int top,int right,int bot)
{
	int i;

	if (TYPE == BM_OGL) {
		OglURect(left,top,right,bot);
		return;
	}
	for ( i=top; i<=bot; i++ )
		GrScanLine( left, right, i );
}
