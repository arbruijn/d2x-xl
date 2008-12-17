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

int gr_disk(fix xc1,fix yc1,fix r1)
{
	int p,x, y, xc, yc, r;

	r = X2I(r1);
	xc = X2I(xc1);
	yc = X2I(yc1);
	p=3-(r*2);
	x=0;
	y=r;

	// Big clip
	if ( (xc+r) < 0 ) return 1;
	if ( (xc-r) > CCanvas::Current ()->Width () ) return 1;
	if ( (yc+r) < 0 ) return 1;
	if ( (yc-r) > CCanvas::Current ()->Height () ) return 1;

	while(x<y)
	{
		// Draw the first octant
		GrScanLine( xc-y, xc+y, yc-x );
		GrScanLine( xc-y, xc+y, yc+x );

		if (p<0)
			p=p+(x<<2)+6;
		else {
			// Draw the second octant
			GrScanLine( xc-x, xc+x, yc-y );
			GrScanLine( xc-x, xc+x, yc+y );
			p=p+((x-y)<<2)+10;
			y--;
		}
		x++;
	}
	if(x==y) {
		GrScanLine( xc-x, xc+x, yc-y );
		GrScanLine( xc-x, xc+x, yc+y );
	}
	return 0;
}

int gr_udisk(fix xc1,fix yc1,fix r1)
{
	int p,x, y, xc, yc, r;

	r = X2I(r1);
	xc = X2I(xc1);
	yc = X2I(yc1);
	p=3-(r*2);
	x=0;
	y=r;

	while(x<y)
	{
		// Draw the first octant
		gr_uscanline( xc-y, xc+y, yc-x );
		gr_uscanline( xc-y, xc+y, yc+x );

		if (p<0)
			p=p+(x<<2)+6;
		else {
			// Draw the second octant
			gr_uscanline( xc-x, xc+x, yc-y );
			gr_uscanline( xc-x, xc+x, yc+y );
			p=p+((x-y)<<2)+10;
			y--;
		}
		x++;
	}
	if(x==y) {
		gr_uscanline( xc-x, xc+x, yc-y );
		gr_uscanline( xc-x, xc+x, yc+y );
	}
	return 0;
}

