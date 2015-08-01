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

extern void gr_set_buffer(int32_t w, int32_t h, int32_t r, int32_t (*buffer_func)());

extern void gr_pal_setblock( int32_t start, int32_t n, uint8_t * palette );
extern void gr_pal_getblock( int32_t start, int32_t n, uint8_t * palette );
extern void gr_pal_setone( int32_t index, uint8_t red, uint8_t green, uint8_t blue );

void gr_linear_movsb( uint8_t * source, uint8_t * dest, int32_t nbytes);
void gr_linear_movsw( uint8_t * source, uint8_t * dest, int32_t nbytes);
void gr_linear_stosd( uint8_t * dest, CCanvasColor *color, uint32_t nbytes);

extern uint32_t gr_var_color;
extern uint32_t gr_var_bwidth;
extern uint8_t * gr_var_bitmap;

void gr_linear_movsd( uint8_t * source, uint8_t * dest, uint32_t nbytes);
void gr_linear_rep_movsd_2x(uint8_t *source, uint8_t *dest, uint32_t nbytes);

void gr_linear_line( int32_t x0, int32_t y0, int32_t x1, int32_t y1);

extern uint32_t Table8to32[256];

#if 0
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
#endif

void order( int32_t *x1, int32_t *x2 );
