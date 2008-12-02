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

#include <stdlib.h>

#include "inferno.h"
#include "gr.h"
#include "grdef.h"
#include "rle.h"

// John's new stuff below here....

int scale_error_term;
int scale_initial_pixelCount;
int scale_adj_up;
int scale_adjDown;
int scale_final_pixelCount;
int scale_ydelta_minus_1;
int scale_whole_step;
ubyte * scale_source_ptr;
ubyte * scale_dest_ptr;


ubyte scale_rle_data[1600];

void scale_up_bitmap(CBitmap *source_bmp, CBitmap *dest_bmp, int x0, int y0, int x1, int y1, fix u0, fix v0,  fix u1, fix v1, int orientation  );
void scale_up_bitmap_rle(CBitmap *source_bmp, CBitmap *dest_bmp, int x0, int y0, int x1, int y1, fix u0, fix v0,  fix u1, fix v1, int orientation  );
void rls_stretch_scanline_setup( int XDelta, int YDelta );
void rls_stretch_scanline(void);


void decode_row (CBitmap * bmp, int y)
{
	int i, offset=4+bmp->Height ();

	for (i=0; i<y; i++ )
		offset += bmp->TexBuf () [4+i];
	gr_rle_decode( &bmp->TexBuf () [offset], scale_rle_data );
}


void scale_up_bitmap(CBitmap *source_bmp, CBitmap *dest_bmp, int x0, int y0, int x1, int y1, fix u0, fix v0,  fix u1, fix v1, int orientation  )
{
	fix dv, v;
	int y;

	if (orientation & 1) {
		int	t;
		t = u0;	u0 = u1;	u1 = t;
	}

	if (orientation & 2) {
		int	t;
		t = v0;	v0 = v1;	v1 = t;
		if (v1 < v0)
			v0--;
	}

	v = v0;

	dv = (v1-v0) / (y1-y0);

	rls_stretch_scanline_setup( (int)(x1-x0), X2I(u1)-X2I(u0) );
	if ( scale_ydelta_minus_1 < 1 ) return;

	v = v0;

	for (y = y0; y <= y1; y++ ) {
		scale_source_ptr = &source_bmp->TexBuf () [source_bmp->RowSize () * X2I(v) + X2I(u0)];
		scale_dest_ptr = &dest_bmp->TexBuf () [dest_bmp->RowSize () * y + x0];
		rls_stretch_scanline();
		v += dv;
	}
}




void scale_up_bitmap_rle(CBitmap *source_bmp, CBitmap *dest_bmp, int x0, int y0, int x1, int y1, fix u0, fix v0,  fix u1, fix v1, int orientation  )
{
	fix dv, v;
	int y, last_row = -1;

	if (orientation & 1) {
		int	t;
		t = u0;	u0 = u1;	u1 = t;
	}

	if (orientation & 2) {
		int	t;
		t = v0;	v0 = v1;	v1 = t;
		if (v1 < v0)
			v0--;
	}

	dv = (v1-v0) / (y1-y0);

	rls_stretch_scanline_setup( (int)(x1-x0), X2I(u1)-X2I(u0) );
	if ( scale_ydelta_minus_1 < 1 ) return;

	v = v0;

	for (y=y0; y<=y1; y++ )			{
		if ( X2I(v) != last_row )	{
			last_row = X2I(v);
			decode_row( source_bmp, last_row );
		}
		scale_source_ptr = &scale_rle_data[X2I(u0)];
		scale_dest_ptr = &dest_bmp->TexBuf ()[dest_bmp->RowSize ()*y+x0];
		rls_stretch_scanline( );
		v += dv;
	}
}

void rls_stretch_scanline_setup( int XDelta, int YDelta )
{
	  scale_ydelta_minus_1 = YDelta - 1;

      /* X major line */
      /* Minimum # of pixels in a run in this line */
      scale_whole_step = XDelta / YDelta;

      /* Error term adjust each time Y steps by 1; used to tell when one
         extra pixel should be drawn as part of a run, to account for
         fractional steps along the X axis per 1-pixel steps along Y */
      scale_adj_up = (XDelta % YDelta) * 2;

      /* Error term adjust when the error term turns over, used to factor
         out the X step made at that time */
      scale_adjDown = YDelta * 2;

      /* Initial error term; reflects an initial step of 0.5 along the Y
         axis */
      scale_error_term = (XDelta % YDelta) - (YDelta * 2);

      /* The initial and last runs are partial, because Y advances only 0.5
         for these runs, rather than 1. Divide one full run, plus the
         initial pixel, between the initial and last runs */
      scale_initial_pixelCount = (scale_whole_step / 2) + 1;
      scale_final_pixelCount = scale_initial_pixelCount;

      /* If the basic run length is even and there's no fractional
         advance, we have one pixel that could go to either the initial
         or last partial run, which we'll arbitrarily allocate to the
         last run */
      if ((scale_adj_up == 0) && ((scale_whole_step & 0x01) == 0))
      {
         scale_initial_pixelCount--;
      }
     /* If there're an odd number of pixels per run, we have 1 pixel that can't
     be allocated to either the initial or last partial run, so we'll add 0.5
     to error term so this pixel will be handled by the Normal full-run loop */
      if ((scale_whole_step & 0x01) != 0)
      {
         scale_error_term += YDelta;
      }

}

void rls_stretch_scanline( )
{
	ubyte   c, *src_ptr, *dest_ptr;
	int i, j, len, ErrorTerm, initialCount, finalCount;

	// Draw the first, partial run of pixels

	src_ptr = scale_source_ptr;
	dest_ptr = scale_dest_ptr;
	ErrorTerm = scale_error_term;
	initialCount = scale_initial_pixelCount;
	finalCount = scale_final_pixelCount;

	c = *src_ptr++;
	if ( c != TRANSPARENCY_COLOR ) {
		for (i=0; i<initialCount; i++ )
			*dest_ptr++ = c;
	} else {
		dest_ptr += initialCount;
	}

	// Draw all full runs

	for (j=0; j<scale_ydelta_minus_1; j++) {
		len = scale_whole_step;     // run is at least this long

 		// Advance the error term and add an extra pixel if the error term so indicates
		if ((ErrorTerm += scale_adj_up) > 0)    {
			len++;
			ErrorTerm -= scale_adjDown;   // reset the error term
		}

		// Draw this run o' pixels
		c = *src_ptr++;
		if ( c != TRANSPARENCY_COLOR )  {
			for (i=len; i>0; i-- )
				*dest_ptr++ = c;
		} else {
			dest_ptr += len;
		}
	}

	// Draw the final run of pixels
	c = *src_ptr++;
	if ( c != TRANSPARENCY_COLOR ) {
		for (i=0; i<finalCount; i++ )
			*dest_ptr++ = c;
	} else {
		dest_ptr += finalCount;
	}
}

#if 0
void rls_stretch_scanline()
{
	ubyte   c;
	int i, j, len, ErrorTerm, x;

	// Setup initial variables
	ErrorTerm = scale_error_term;

	// Draw the first, partial run of pixels

	c = *scale_source_ptr++;
	if ( c != TRANSPARENCY_COLOR )  {
		for (i=0; i<scale_initial_pixelCount; i++ )
			*scale_dest_ptr++ = c;
	} else {
		scale_dest_ptr += scale_initial_pixelCount;
	}

	// Draw all full runs

	for (j=0; j<scale_ydelta_minus_1; j++)	{
		len = scale_whole_step;		// run is at least this long

 		// Advance the error term and add an extra pixel if the error term so indicates
		if ((ErrorTerm += scale_adj_up) > 0)	{
			len++;
			ErrorTerm -= scale_adjDown;   // reset the error term
		}

		// Draw this run o' pixels
		c = *scale_source_ptr++;
		if ( c != TRANSPARENCY_COLOR )	{
			for (i=len; i>0; i-- )
				*scale_dest_ptr++ = c;
		} else {
			scale_dest_ptr += len;
		}
	}

	// Draw the final run of pixels
	c = *scale_source_ptr++;
	if ( c != TRANSPARENCY_COLOR )	{
		for (i=0; i<scale_final_pixelCount; i++ )
			*scale_dest_ptr++ = c;
	} else {
		scale_dest_ptr += scale_final_pixelCount;
	}
}
#endif
// old stuff here...

void scale_bitmap_c(CBitmap *source_bmp, CBitmap *dest_bmp, int x0, int y0, int x1, int y1, fix u0, fix v0,  fix u1, fix v1, int orientation  )
{
	fix u, v, du, dv;
	int x, y;
	ubyte * sbits, * dbits, c;

	du = (u1-u0) / (x1-x0);
	dv = (v1-v0) / (y1-y0);

	if (orientation & 1) {
		u0 = u1;
		du = -du;
	}

	if (orientation & 2) {
		v0 = v1;
		dv = -dv;
		if (dv < 0)
			v0--;
	}

	v = v0;

	for (y=y0; y<=y1; y++ )			{
		sbits = &source_bmp->TexBuf ()[source_bmp->RowSize () * X2I(v)];
		dbits = &dest_bmp->TexBuf ()[dest_bmp->RowSize () * y + x0];
		u = u0;
		v += dv;
		for (x=x0; x<=x1; x++ )			{
			c = sbits[u >> 16];
			if (c != TRANSPARENCY_COLOR)
				*dbits = c;
			dbits++;
			u += du;
		}
	}
}

void scale_row_asm_transparent( ubyte * sbits, ubyte * dbits, int width, fix u, fix du )
{
#if 0
	int i;
	ubyte c;

	for (i=0; i<width; i++ )	{
		c = sbits[ u >> 16 ];
		if ( c!=TRANSPARENCY_COLOR)
			*dbits = c;
		dbits++;
		u += du;
	}
#endif
	int i;
	ubyte c;
	ubyte *dbits_end = &dbits[width-1];

	if ( du < F1_0 )	{
		// Scaling up.
		fix next_u;
		int next_u_int;

		next_u_int = X2I(u)+1;
		c = sbits[ next_u_int ];
		next_u = I2X(next_u_int);
		if ( c != TRANSPARENCY_COLOR ) goto NonTransparent;

Transparent:
		while (1)	{
			dbits++;
			if ( dbits > dbits_end ) return;
			u += du;
			if ( u > next_u )	{
				next_u_int = X2I(u)+1;
				c = sbits[ next_u_int ];
				next_u = I2X(next_u_int);
				if ( c != TRANSPARENCY_COLOR ) goto NonTransparent;
			}
		}
		return;

NonTransparent:
		while (1)	{
			*dbits++ = c;
			if ( dbits > dbits_end ) return;
			u += du;
			if ( u > next_u )	{
				next_u_int = X2I(u)+1;
				c = sbits[ next_u_int ];
				next_u = I2X(next_u_int);
				if ( c == TRANSPARENCY_COLOR ) goto Transparent;
			}
		}
		return;



	} else {
		for ( i=0; i<width; i++ )	{
			c = sbits[ X2I(u) ];

			if ( c != TRANSPARENCY_COLOR )
				*dbits = c;

			dbits++;
			u += du;
		}
	}
}

void scale_bitmap_c_rle(CBitmap *source_bmp, CBitmap *dest_bmp, int x0, int y0, int x1, int y1, fix u0, fix v0,  fix u1, fix v1, int orientation  )
{
	fix du, dv, v;
	int y, last_row=-1;

//	Rotation doesn't work because explosions are not square!
// -- 	if (orientation & 4) {
// -- 		int	t;
// -- 		t = u0;	u0 = v0;	v0 = t;
// -- 		t = u1;	u1 = v1;	v1 = t;
// -- 	}

	du = (u1-u0) / (x1-x0);
	dv = (v1-v0) / (y1-y0);

	if (orientation & 1) {
		u0 = u1;
		du = -du;
	}

	if (orientation & 2) {
		v0 = v1;
		dv = -dv;
		if (dv < 0)
			v0--;
	}

	v = v0;

	if (v<0) {	//was: Assert(v >= 0);
		//Int3();   //this should be checked in higher-level routine
		return;
	}

	for (y=y0; y<=y1; y++ )			{
		if ( X2I(v) != last_row )	{
			last_row = X2I(v);
			decode_row( source_bmp, last_row );
		}
		scale_row_asm_transparent (scale_rle_data, &dest_bmp->TexBuf ()[dest_bmp->RowSize ()*y+x0], x1-x0+1, u0, du );
		v += dv;
	}
}

#define FIND_SCALED_NUM(x,x0,x1,y0,y1) (FixMulDiv((x)-(x0),(y1)-(y0),(x1)-(x0))+(y0))

// Scales bitmap, bp, into vertbuf[0] to vertbuf[1]
void ScaleBitmap(CBitmap *bmP, grsPoint *vertbuf, int orientation )
{
	CBitmap * dbp = &CCanvas::Current ()->Bitmap ();
	fix x0, y0, x1, y1;
	fix u0, v0, u1, v1;
	fix clipped_x0, clipped_y0, clipped_x1, clipped_y1;
	fix clipped_u0, clipped_v0, clipped_u1, clipped_v1;
	fix xmin, xmax, ymin, ymax;
	int dx0, dy0, dx1, dy1;
	int dtemp;
	// Set initial variables....

	x0 = vertbuf[0].x; y0 = vertbuf[0].y;
	x1 = vertbuf[2].x; y1 = vertbuf[2].y;

	xmin = 0; ymin = 0;
	xmax = I2X(dbp->Width ())-F2X(.5); ymax = I2X(dbp->Height ())-F2X(.5);

	u0 = I2X(0); v0 = I2X(0);
	u1 = I2X(bmP->Width ()-1); v1 = I2X(bmP->Height ()-1);

	// Check for obviously offscreen bitmaps...
	if ( (y1<=y0) || (x1<=x0) ) return;
	if ( (x1<0 ) || (x0>=xmax) ) return;
	if ( (y1<0 ) || (y0>=ymax) ) return;

	clipped_u0 = u0; clipped_v0 = v0;
	clipped_u1 = u1; clipped_v1 = v1;

	clipped_x0 = x0; clipped_y0 = y0;
	clipped_x1 = x1; clipped_y1 = y1;

	// Clip the left, moving u0 right as necessary
	if ( x0 < xmin ) 	{
		clipped_u0 = FIND_SCALED_NUM(xmin,x0,x1,u0,u1);
		clipped_x0 = xmin;
	}

	// Clip the right, moving u1 left as necessary
	if ( x1 > xmax )	{
		clipped_u1 = FIND_SCALED_NUM(xmax,x0,x1,u0,u1);
		clipped_x1 = xmax;
	}

	// Clip the top, moving v0 down as necessary
	if ( y0 < ymin ) 	{
		clipped_v0 = FIND_SCALED_NUM(ymin,y0,y1,v0,v1);
		clipped_y0 = ymin;
	}

	// Clip the bottom, moving v1 up as necessary
	if ( y1 > ymax ) 	{
		clipped_v1 = FIND_SCALED_NUM(ymax,y0,y1,v0,v1);
		clipped_y1 = ymax;
	}

	dx0 = X2I(clipped_x0); dx1 = X2I(clipped_x1);
	dy0 = X2I(clipped_y0); dy1 = X2I(clipped_y1);

	if (dx1<=dx0) return;
	if (dy1<=dy0) return;

//	Assert( dx0>=0 );
//	Assert( dy0>=0 );
//	Assert( dx1<dbp->Width () );
//	Assert( dy1<dbp->Height () );
//	Assert( X2I(u0)<=X2I(u1) );
//	Assert( X2I(v0)<=X2I(v1) );
//	Assert( X2I(u0)>=0 );
//	Assert( X2I(v0)>=0 );
//	Assert( u1<I2X(bmP->Width ()) );
//	Assert( v1<I2X(bmP->Height ()) );

	dtemp = X2I(clipped_u1)-X2I(clipped_u0);

	if ( bmP->Flags () & BM_FLAG_RLE )	{
		if ( (dtemp < (X2I(clipped_x1)-X2I(clipped_x0))) && (dtemp>0) )
			scale_up_bitmap_rle(bmP, dbp, dx0, dy0, dx1, dy1, clipped_u0, clipped_v0, clipped_u1, clipped_v1, orientation  );
		else
			scale_bitmap_c_rle(bmP, dbp, dx0, dy0, dx1, dy1, clipped_u0, clipped_v0, clipped_u1, clipped_v1, orientation  );
	} else {
		if ( (dtemp < (X2I(clipped_x1)-X2I(clipped_x0))) && (dtemp>0) )
			scale_up_bitmap(bmP, dbp, dx0, dy0, dx1, dy1, clipped_u0, clipped_v0, clipped_u1, clipped_v1, orientation  );
		else
			scale_bitmap_c(bmP, dbp, dx0, dy0, dx1, dy1, clipped_u0, clipped_v0, clipped_u1, clipped_v1, orientation  );
	}
}

