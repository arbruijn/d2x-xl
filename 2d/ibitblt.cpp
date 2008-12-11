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

#include "inferno.h"
#include "pa_enabl.h"
#include "pstypes.h"
#include "gr.h"
#include "ibitblt.h"
#include "error.h"
#include "u_mem.h"
#include "grdef.h"

#define FIND_START      1
#define FIND_STOP       2

#define MAX_WIDTH       1600
#define MAX_SCANLINES   1200
#define MAX_HOLES       5

static short start_points[MAX_SCANLINES][MAX_HOLES];
static short hole_length[MAX_SCANLINES][MAX_HOLES];
static double *scanline = NULL;

//	-----------------------------------------------------------------------------

void gr_ibitblt(CBitmap *src_bmp, CBitmap *dest_bmp, ubyte pixel_double)
{
	int x, y, sw, sh, srowSize, drowSize, dstart, sy, dy;
	ubyte *src, *dest;
	short *current_hole, *current_hole_length;

// variable setup

	sw = src_bmp->Width ();
	sh = src_bmp->Height ();
	srowSize = src_bmp->RowSize ();
	drowSize = dest_bmp->RowSize ();
	src = src_bmp->Buffer ();
	dest = dest_bmp->Buffer ();

	sy = 0;
	while (start_points[sy][0] == -1) {
		sy++;
		dest += drowSize;
	}

 	if (pixel_double) {
		ubyte *scan = reinterpret_cast<ubyte*> (scanline);    // set up for byte processing of scanline

		dy = sy;
		for (y = sy; y < sy + sh; y++) {
			gr_linear_rep_movsd_2x(src, scan, sw); // was: gr_linear_movsd_double(src, scan, sw*2);
			current_hole = start_points[dy];
			current_hole_length = hole_length[dy];
			for (x = 0; x < MAX_HOLES; x++) {
				if (*current_hole == -1)
					break;
				dstart = *current_hole;
				gr_linear_movsd(&(scan[dstart]), &(dest[dstart]), *current_hole_length);
				current_hole++;
				current_hole_length++;
			}
			dy++;
			dest += drowSize;
			current_hole = start_points[dy];
			current_hole_length = hole_length[dy];
			for (x = 0;x < MAX_HOLES; x++) {
				if (*current_hole == -1)
					break;
				dstart = *current_hole;
				gr_linear_movsd(&(scan[dstart]), &(dest[dstart]), *current_hole_length);
				current_hole++;
				current_hole_length++;
			}
			dy++;
			dest += drowSize;
			src += srowSize;
		}
	} else {
		Assert(sw <= MAX_WIDTH);
		Assert(sh <= MAX_SCANLINES);
		for (y = sy; y < sy + sh; y++) {
			for (x = 0; x < MAX_HOLES; x++) {
				if (start_points[y][x] == -1)
					break;
				dstart = start_points[y][x];
				gr_linear_movsd(&(src[dstart]), &(dest[dstart]), hole_length[y][x]);
			}
			dest += drowSize;
			src += srowSize;
		}
	}
}

//	-----------------------------------------------------------------------------

void gr_ibitblt_create_mask(CBitmap *mask_bmp, int sx, int sy, int sw, int sh, int srowSize)
{
	int x, y;
	ubyte mode;
	int count = 0;

	Assert( (!(mask_bmp->Flags () & BM_FLAG_RLE)) );

	for (y = 0; y < MAX_SCANLINES; y++) {
		for (x = 0; x < MAX_HOLES; x++) {
			start_points[y][x] = -1;
			hole_length[y][x] = -1;
		}
	}

	for (y = sy; y < sy+sh; y++) {
		count = 0;
		mode = FIND_START;
		for (x = sx; x < sx + sw; x++) {
			if ((mode == FIND_START) && (mask_bmp->Buffer ()[mask_bmp->RowSize ()*y+x] == TRANSPARENCY_COLOR)) {
				start_points[y][count] = x;
				mode = FIND_STOP;
			} else if ((mode == FIND_STOP) && (mask_bmp->Buffer ()[mask_bmp->RowSize ()*y+x] != TRANSPARENCY_COLOR)) {
				hole_length[y][count] = x - start_points[y][count];
				count++;
				mode = FIND_START;
			}
		}
		if (mode == FIND_STOP) {
			hole_length[y][count] = x - start_points[y][count];
			count++;
		}
		Assert(count <= MAX_HOLES);
	}
}

//	-----------------------------------------------------------------------------

void gr_ibitblt_find_hole_size(CBitmap *mask_bmp, int *minx, int *miny, int *maxx, int *maxy)
{
	ubyte c;
	int x, y, count = 0;

	Assert( (!(mask_bmp->Flags () &BM_FLAG_RLE)) );
	Assert( mask_bmp->Flags () &BM_FLAG_TRANSPARENT );

	*minx = mask_bmp->Width () - 1;
	*maxx = 0;
	*miny = mask_bmp->Height () - 1;
	*maxy = 0;

	if (scanline == NULL)
		scanline = new double [MAX_WIDTH / sizeof(double)];

	for (y = 0; y < mask_bmp->Height (); y++) {
		for (x = 0; x < mask_bmp->Width (); x++) {
			c = (*mask_bmp) [mask_bmp->RowSize ()*y+x];
			if (c == TRANSPARENCY_COLOR) { // don't look for transparancy color here.
				count++;
				if (x < *minx) *minx = x;
				if (y < *miny) *miny = y;
				if (x > *maxx) *maxx = x;
				if (y > *maxy) *maxy = y;
			}
		}
	}
	Assert (count);
}

//	-----------------------------------------------------------------------------
//eof
