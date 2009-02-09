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
 * Internal definitions for graphics lib.
 *
 */

extern void gr_set_buffer(int w, int h, int r, int (*buffer_func)());

extern void gr_pal_setblock( int start, int n, ubyte * palette );
extern void gr_pal_getblock( int start, int n, ubyte * palette );
extern void gr_pal_setone( int index, ubyte red, ubyte green, ubyte blue );

void gr_linear_movsb( ubyte * source, ubyte * dest, int nbytes);
void gr_linear_movsw( ubyte * source, ubyte * dest, int nbytes);
void gr_linear_stosd( ubyte * dest, tCanvasColor *color, uint nbytes);

extern uint gr_var_color;
extern uint gr_var_bwidth;
extern ubyte * gr_var_bitmap;

void gr_linear_movsd( ubyte * source, ubyte * dest, uint nbytes);
void gr_linear_rep_movsd_2x(ubyte *source, ubyte *dest, uint nbytes);

void gr_linear_line( int x0, int y0, int x1, int y1);

extern uint Table8to32[256];

#define MINX    0
#define MINY    0
#define MAXX    (CCanvas::Current ()->Width ()-1)
#define MAXY    (CCanvas::Current ()->Height ()-1)
#define MODE    CCanvas::Current ()->Mode ()
#define XOFFSET CCanvas::Current ()->Left ()
#define YOFFSET CCanvas::Current ()->Top ()
#define ROWSIZE CCanvas::Current ()->RowSize ()
#define DATA    CCanvas::Current ()->Buffer ()
#define COLOR   CCanvas::Current ()->Color ()

void order( int *x1, int *x2 );
