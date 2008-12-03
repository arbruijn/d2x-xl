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

#define VESA 0
#define NUM_TMAPS 16

#define HEADLIGHT_LIGHTING 0

#define WIREFRAME 0
#define PERSPECTIVE 1

#include "inferno.h"
#include "pstypes.h"
#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "3d.h"
#include "error.h"

#include "texmap.h"
#include "texmapl.h"
#include "rle.h"
#include "scanline.h"

#ifdef EDITOR
#define EDITOR_TMAP 1       //if in, include extra stuff
#endif

#define F15_5 (F1_0*15 + F0_5)

// Temporary texture map, interface from Matt's 3d system to Mike's texture mapper.
g3ds_tmap Tmap1;

CBitmap Texmap_ptrs[NUM_TMAPS];
CBitmap Texmap4_ptrs[NUM_TMAPS];

int	nCurrentSegDepth;		// HACK INTERFACE: how far away the current tSegment (& thus texture) is

int nWindowClipLeft, nWindowClipTop, nWindowClipRight, nWindowClipBot;

// These variables are the interface to assembler.  They get set for each texture map, which is a real waste of time.
//	They should be set only when they change, which is generally when the window bounds change.  And, even still, it's
//	a pretty bad interface.
int	bytes_per_row=-1;
unsigned char *write_buffer;
int  	window_left;
int	window_right;
int	window_top;
int	window_bottom;
int  	window_width;
int  	window_height;

#define MAX_Y_POINTERS  1024

int	y_pointers[MAX_Y_POINTERS];

fix fix_recip[FIX_RECIP_TABLE_SIZE];

static int	bLightingEnabled;
int	bFixRecipTableComputed=0;

fix fx_l, fx_u, fx_v, fx_z, fx_du_dx, fx_dv_dx, fx_dz_dx, fx_dl_dx;
int fx_xleft, fx_xright, fx_y;
unsigned char * pixptr;
int per2Flag = 0;

ubyte * tmap_flat_cthru_table;
ubyte tmap_flat_color;
ubyte tmap_flat_shadeValue;



// -------------------------------------------------------------------------------------
void init_fix_recip_table(void)
{
	int	i;

	fix_recip[0] = F1_0;

	for (i=1; i<FIX_RECIP_TABLE_SIZE; i++)
		fix_recip[i] = F1_0/i;

	bFixRecipTableComputed = 1;
}

// -------------------------------------------------------------------------------------
//	Initialize interface variables to assembler.
//	These things used to be constants.  This routine is now (10/6/93) getting called for
//	every texture map.  It should get called whenever the window changes, or, preferably,
//	not at all.  I'm pretty sure these variables are only being used for range checking.
void init_interface_vars_to_assembler(void)
{
	CBitmap	*bmP;

	bmP = &CCanvas::Current ()->Bitmap ();

	Assert(bmP!=NULL);
	Assert(bmP->Buffer ()!=NULL);
	Assert(bmP->Height () <= MAX_Y_POINTERS);

	//	If bytes_per_row has changed, create new table of pointers.
	if (bytes_per_row != (int) bmP->RowSize ()) {
		int	y_val, i;

		bytes_per_row = (int) bmP->RowSize ();

		y_val = 0;
		for (i=0; i<MAX_Y_POINTERS; i++) {
			y_pointers[i] = y_val;
			y_val += bytes_per_row;
		}
	}

        write_buffer = (unsigned char *) bmP->Buffer ();

	window_left = 0;
	window_right = (int) bmP->Width ()-1;
	window_top = 0;
	window_bottom = (int) bmP->Height ()-1;

	nWindowClipLeft = window_left;
	nWindowClipRight = window_right;
	nWindowClipTop = window_top;
	nWindowClipBot = window_bottom;

	window_width = bmP->Width ();
	window_height = bmP->Height ();

	if (!bFixRecipTableComputed)
		init_fix_recip_table();
}

// -------------------------------------------------------------------------------------
//                             VARIABLES
extern g3ds_tmap Tmap1;

// -------------------------------------------------------------------------------------
//	Returns number preceding val modulo modulus.
//	prevmod(3,4) = 2
//	prevmod(0,4) = 3
int prevmod(int val,int modulus)
{
	if (val > 0)
		return val-1;
	else
		return modulus-1;
//	return (val + modulus - 1) % modulus;
}


//	Returns number succeeding val modulo modulus.
//	succmod(3,4) = 0
//	succmod(0,4) = 1
int succmod(int val,int modulus)
{
	if (val < modulus-1)
		return val+1;
	else
		return 0;

//	return (val + 1) % modulus;
}

// -------------------------------------------------------------------------------------
//	Select topmost vertex (minimum y coordinate) and bottommost (maximum y coordinate) in
//	texture map.  If either is part of a horizontal edge, then select leftmost vertex for
//	top, rightmost vertex for bottom.
//	Important: Vertex is selected with integer precision.  So, if there are vertices at
//	(0.0,0.7) and (0.5,0.3), the first vertex is selected, because they y coordinates are
//	considered the same, so the smaller x is favored.
//	Parameters:
//		nv		number of vertices
//		v3d	pointer to 3d vertices containing u,v,x2d,y2d coordinates
//	Results in:
//		*min_y_ind
//		*max_y_ind
// -------------------------------------------------------------------------------------
void compute_y_bounds(g3ds_tmap *t, int *vlt, int *vlb, int *vrt, int *vrb,int *bottom_y_ind)
{
	int	i;
	int	min_y,max_y;
	int	min_y_ind;
	int	original_vrt;
	fix	min_x;

	// Scan all vertices, set min_y_ind to vertex with smallest y coordinate.
	min_y = X2I(t->verts[0].y2d);
	max_y = min_y;
	min_y_ind = 0;
	min_x = X2I(t->verts[0].x2d);
	*bottom_y_ind = 0;

	for (i=1; i<t->nv; i++) {
		if (X2I(t->verts[i].y2d) < min_y) {
			min_y = X2I(t->verts[i].y2d);
			min_y_ind = i;
			min_x = X2I(t->verts[i].x2d);
		} else if (X2I(t->verts[i].y2d) == min_y) {
			if (X2I(t->verts[i].x2d) < min_x) {
				min_y_ind = i;
				min_x = X2I(t->verts[i].x2d);
			}
		}
		if (X2I(t->verts[i].y2d) > max_y) {
			max_y = X2I(t->verts[i].y2d);
			*bottom_y_ind = i;
		}
	}

//--removed mk, 11/27/94--	//	Check for a non-upright-hourglass polygon and fix, if necessary, by bashing a y coordinate.
//--removed mk, 11/27/94--	//	min_y_ind = index of minimum y coordinate, *bottom_y_ind = index of maximum y coordinate
//--removed mk, 11/27/94--{
//--removed mk, 11/27/94--	int	max_temp, min_temp;
//--removed mk, 11/27/94--
//--removed mk, 11/27/94--	max_temp = *bottom_y_ind;
//--removed mk, 11/27/94--	if (*bottom_y_ind < min_y_ind)
//--removed mk, 11/27/94--		max_temp += t->nv;
//--removed mk, 11/27/94--
//--removed mk, 11/27/94--	for (i=min_y_ind; i<max_temp; i++) {
//--removed mk, 11/27/94--		if (X2I(t->verts[i%t->nv].y2d) > X2I(t->verts[(i+1)%t->nv].y2d)) {
//--removed mk, 11/27/94--			Int3();
//--removed mk, 11/27/94--			t->verts[(i+1)%t->nv].y2d = t->verts[i%t->nv].y2d;
//--removed mk, 11/27/94--		}
//--removed mk, 11/27/94--	}
//--removed mk, 11/27/94--
//--removed mk, 11/27/94--	min_temp = min_y_ind;
//--removed mk, 11/27/94--	if (min_y_ind < *bottom_y_ind)
//--removed mk, 11/27/94--		min_temp += t->nv;
//--removed mk, 11/27/94--
//--removed mk, 11/27/94--	for (i=*bottom_y_ind; i<min_temp; i++) {
//--removed mk, 11/27/94--		if (X2I(t->verts[i%t->nv].y2d) < X2I(t->verts[(i+1)%t->nv].y2d)) {
//--removed mk, 11/27/94--			Int3();
//--removed mk, 11/27/94--			t->verts[(i+1)%t->nv].y2d = t->verts[i%t->nv].y2d;
//--removed mk, 11/27/94--		}
//--removed mk, 11/27/94--	}
//--removed mk, 11/27/94--}

	// Set "vertex left top", etc. based on vertex with topmost y coordinate
	*vlt = min_y_ind;
	*vrt = *vlt;
	*vlb = prevmod(*vlt,t->nv);
	*vrb = succmod(*vrt,t->nv);

	// If right edge is horizontal, then advance along polygon bound until it no longer is or until all
	// vertices have been examined.
	// (Left edge cannot be horizontal, because *vlt is set to leftmost point with highest y coordinate.)

	original_vrt = *vrt;

	while (X2I(t->verts[*vrt].y2d) == X2I(t->verts[*vrb].y2d)) {
		if (succmod(*vrt,t->nv) == original_vrt) {
			break;
		}
		*vrt = succmod(*vrt,t->nv);
		*vrb = succmod(*vrt,t->nv);
	}
}

// -------------------------------------------------------------------------------------
//	Returns dx/dy given two vertices.
//	If dy == 0, returns 0.0
// -------------------------------------------------------------------------------------
//--fix compute_dx_dy_lin(g3ds_tmap *t, int top_vertex,int bottom_vertex)
//--{
//--	int	dy;
//--
//--	// compute delta x with respect to y for any edge
//--	dy = X2I(t->verts[bottom_vertex].y2d - t->verts[top_vertex].y2d) + 1;
//--	if (dy)
//--		return (t->verts[bottom_vertex].x2d - t->verts[top_vertex].x2d) / dy;
//--	else
//--		return 0;
//--
//--}

fix compute_du_dy_lin(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return FixMul(t->verts[bottom_vertex].u - t->verts[top_vertex].u, recip_dy);
}


fix compute_dv_dy_lin(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return FixMul(t->verts[bottom_vertex].v - t->verts[top_vertex].v, recip_dy);
}

fix compute_dl_dy_lin(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return FixMul(t->verts[bottom_vertex].l - t->verts[top_vertex].l, recip_dy);

}

fix compute_dx_dy(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return FixMul(t->verts[bottom_vertex].x2d - t->verts[top_vertex].x2d, recip_dy);
}

fix compute_du_dy(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return FixMul(FixMul(t->verts[bottom_vertex].u,t->verts[bottom_vertex].z) - FixMul(t->verts[top_vertex].u,t->verts[top_vertex].z), recip_dy);
}


fix compute_dv_dy(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return FixMul(FixMul(t->verts[bottom_vertex].v,t->verts[bottom_vertex].z) - FixMul(t->verts[top_vertex].v,t->verts[top_vertex].z), recip_dy);

}

fix compute_dz_dy(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return FixMul(t->verts[bottom_vertex].z - t->verts[top_vertex].z, recip_dy);

}
int Skip_shortFlag=0;

// -------------------------------------------------------------------------------------
//	Texture map current scanline in perspective.
// -------------------------------------------------------------------------------------
void ntmap_scanline_lighted(CBitmap *srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix zleft, fix zright, fix lleft, fix lright)
{
	fix	dx,recip_dx;

	fx_xright = X2I(xright);
	//edited 06/27/99 Matt Mueller - moved these tests up from within the switch so as not to do a bunch of needless calculations when we are just gonna return anyway.  Slight fps boost?
	if (fx_xright < nWindowClipLeft)
		return;
	fx_xleft = X2I(xleft);
	if (fx_xleft > nWindowClipRight)
		return;
	//end edit -MM

	dx = fx_xright - fx_xleft;
	if ((dx < 0) || (xright < 0) || (xleft > xright))		// the (xleft > xright) term is not redundant with (dx < 0) because dx is computed using integers
		return;

	// setup to call assembler scanline renderer
	if (dx < FIX_RECIP_TABLE_SIZE)
		recip_dx = fix_recip[dx];
	else
		recip_dx = F1_0/dx;

	fx_u = uleft;
	fx_v = vleft;
	fx_z = zleft;

	fx_du_dx = FixMul(uright - uleft,recip_dx);
	fx_dv_dx = FixMul(vright - vleft,recip_dx);
	fx_dz_dx = FixMul(zright - zleft,recip_dx);
	fx_y = y;
	pixptr = srcb->Buffer ();

	switch (bLightingEnabled) {
		case 0:
			//added 05/17/99 Matt Mueller - prevent writing before the buffer
            if ((fx_y == 0) && (fx_xleft < 0))
				fx_xleft = 0;
			//end addition -MM
			if (fx_xright > nWindowClipRight)
				fx_xright = nWindowClipRight;
		
			cur_tmap_scanline_per();
			break;
		case 1: {
			fix	mul_thing;

			if (lleft < 0) lleft = 0;
			if (lright < 0) lright = 0;
			if (lleft > (NUM_LIGHTING_LEVELS*F1_0-F1_0/2)) lleft = (NUM_LIGHTING_LEVELS*F1_0-F1_0/2);
			if (lright > (NUM_LIGHTING_LEVELS*F1_0-F1_0/2)) lright = (NUM_LIGHTING_LEVELS*F1_0-F1_0/2);

			fx_l = lleft;
			fx_dl_dx = FixMul(lright - lleft,recip_dx);

			//	This is a pretty ugly hack to prevent lighting overflows.
			mul_thing = dx * fx_dl_dx;
			if (lleft + mul_thing < 0)
				fx_dl_dx += 12;
			else if (lleft + mul_thing > (NUM_LIGHTING_LEVELS*F1_0-F1_0/2))
				fx_dl_dx -= 12;

			//added 05/17/99 Matt Mueller - prevent writing before the buffer
            if ((fx_y == 0) && (fx_xleft < 0))
				fx_xleft = 0;
			//end addition -MM
			if (fx_xright > nWindowClipRight)
				fx_xright = nWindowClipRight;

			cur_tmap_scanline_per();
			break;
		}
		case 2:
#ifdef EDITOR_TMAP
			fx_xright = X2I(xright);
			fx_xleft = X2I(xleft);

			tmap_flat_color = 1;
			cur_tmap_scanline_flat();
#else
			Int3();	//	Illegal, called an editor only routine!
#endif
			break;
	}

}

int Do_vertical_scan=0;

int	Break_on_flat=0;

// -------------------------------------------------------------------------------------
//	Render a texture map with lighting using perspective interpolation in inner and outer loops.
// -------------------------------------------------------------------------------------
void ntexture_map_lighted(CBitmap *srcb, g3ds_tmap *t)
{
	int	vlt,vrt,vlb,vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy,boty,y, dy;
	fix	dx_dy_left,dx_dy_right;
	fix	du_dy_left,du_dy_right;
	fix	dv_dy_left,dv_dy_right;
	fix	dz_dy_left,dz_dy_right;
	fix	dl_dy_left,dl_dy_right;
	fix	recip_dyl, recip_dyr;
	int	max_y_vertex;
	fix	xleft,xright,uleft,vleft,uright,vright,zleft,zright,lleft,lright;
	int	next_break_left, next_break_right;

     	g3ds_vertex *v3d;

        //remove stupid warnings in compile
        dl_dy_left = F1_0;
        dl_dy_right = F1_0;
        lleft = F1_0;
        lright = F1_0;

	v3d = t->verts;

	// Determine top and bottom y coords.
	compute_y_bounds(t,&vlt,&vlb,&vrt,&vrb,&max_y_vertex);

	// Set top and bottom (of entire texture map) y coordinates.
	topy = X2I(v3d[vlt].y2d);
	boty = X2I(v3d[max_y_vertex].y2d);
	if (topy > nWindowClipBot)
		return;
	if (boty > nWindowClipBot)
		boty = nWindowClipBot;

	// Set amount to change x coordinate for each advance to next scanline.
	dy = X2I(t->verts[vlb].y2d) - X2I(t->verts[vlt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyl = fix_recip[dy];
	else
		recip_dyl = F1_0/dy;

	dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dyl);
	du_dy_left = compute_du_dy(t,vlt,vlb, recip_dyl);
	dv_dy_left = compute_dv_dy(t,vlt,vlb, recip_dyl);
	dz_dy_left = compute_dz_dy(t,vlt,vlb, recip_dyl);

	dy = X2I(t->verts[vrb].y2d) - X2I(t->verts[vrt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyr = fix_recip[dy];
	else
		recip_dyr = F1_0/dy;

	du_dy_right = compute_du_dy(t,vrt,vrb, recip_dyr);
	dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dyr);
	dv_dy_right = compute_dv_dy(t,vrt,vrb, recip_dyr);
	dz_dy_right = compute_dz_dy(t,vrt,vrb, recip_dyr);

	if (bLightingEnabled) {
		dl_dy_left = compute_dl_dy_lin(t,vlt,vlb, recip_dyl);
		dl_dy_right = compute_dl_dy_lin(t,vrt,vrb, recip_dyr);

		lleft = v3d[vlt].l;
		lright = v3d[vrt].l;
	}

 	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	zleft = v3d[vlt].z;
	zright = v3d[vrt].z;

	uleft = FixMul(v3d[vlt].u,zleft);
	uright = FixMul(v3d[vrt].u,zright);
	vleft = FixMul(v3d[vlt].v,zleft);
	vright = FixMul(v3d[vrt].v,zright);

	// scan all rows in texture map from top through first break.
	next_break_left = X2I(v3d[vlb].y2d);
	next_break_right = X2I(v3d[vrb].y2d);

	for (y = topy; y < boty; y++) {

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == next_break_left) {
			fix	recip_dy;

			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == X2I(v3d[vlb].y2d)) {
				vlt = vlb;
				vlb = prevmod(vlb,t->nv);
			}
			next_break_left = X2I(v3d[vlb].y2d);

			dy = X2I(t->verts[vlb].y2d) - X2I(t->verts[vlt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0/dy;

			dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dy);

			xleft = v3d[vlt].x2d;
			zleft = v3d[vlt].z;
			uleft = FixMul(v3d[vlt].u,zleft);
			vleft = FixMul(v3d[vlt].v,zleft);
			lleft = v3d[vlt].l;

			du_dy_left = compute_du_dy(t,vlt,vlb, recip_dy);
			dv_dy_left = compute_dv_dy(t,vlt,vlb, recip_dy);
			dz_dy_left = compute_dz_dy(t,vlt,vlb, recip_dy);

			if (bLightingEnabled) {
				dl_dy_left = compute_dl_dy_lin(t,vlt,vlb, recip_dy);
				lleft = v3d[vlt].l;
			}
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == next_break_right) {
			fix	recip_dy;

			while (y == X2I(v3d[vrb].y2d)) {
				vrt = vrb;
				vrb = succmod(vrb,t->nv);
			}

			next_break_right = X2I(v3d[vrb].y2d);

			dy = X2I(t->verts[vrb].y2d) - X2I(t->verts[vrt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0/dy;

			dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dy);

			xright = v3d[vrt].x2d;
			zright = v3d[vrt].z;
			uright = FixMul(v3d[vrt].u,zright);
			vright = FixMul(v3d[vrt].v,zright);

			du_dy_right = compute_du_dy(t,vrt,vrb, recip_dy);
			dv_dy_right = compute_dv_dy(t,vrt,vrb, recip_dy);
			dz_dy_right = compute_dz_dy(t,vrt,vrb, recip_dy);

			if (bLightingEnabled) {
				dl_dy_right = compute_dl_dy_lin(t,vrt,vrb, recip_dy);
				lright = v3d[vrt].l;
			}
		}

		if (bLightingEnabled) {
			if (y >= nWindowClipTop)
				ntmap_scanline_lighted(srcb,y,xleft,xright,uleft,uright,vleft,vright,zleft,zright,lleft,lright);
			lleft += dl_dy_left;
			lright += dl_dy_right;
		} else
			if (y >= nWindowClipTop)
				ntmap_scanline_lighted(srcb,y,xleft,xright,uleft,uright,vleft,vright,zleft,zright,lleft,lright);

		uleft += du_dy_left;
		vleft += dv_dy_left;

		uright += du_dy_right;
		vright += dv_dy_right;

		xleft += dx_dy_left;
		xright += dx_dy_right;

		zleft += dz_dy_left;
		zright += dz_dy_right;

	}

	// We can get lleft or lright out of bounds here because we compute dl_dy using fixed point values,
	//	but we plot an integer number of scanlines, therefore doing an integer number of additions of the delta.

	ntmap_scanline_lighted(srcb,y,xleft,xright,uleft,uright,vleft,vright,zleft,zright,lleft,lright);
}


// -------------------------------------------------------------------------------------
//	Texture map current scanline using linear interpolation.
// -------------------------------------------------------------------------------------
void ntmap_scanline_lighted_linear(CBitmap *srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix lleft, fix lright)
{
	fix	u,v,l;
	fix	dx,recip_dx;

	fix	du_dx,dv_dx,dl_dx;

	u = uleft;
	v = vleft;
	l = lleft;

	dx = X2I(xright) - X2I(xleft);
	if ((dx < 0) || (xright < 0) || (xleft > xright))		// the (xleft > xright) term is not redundant with (dx < 0) because dx is computed using integers
		return;

		// setup to call assembler scanline renderer
		if (dx < FIX_RECIP_TABLE_SIZE)
			recip_dx = fix_recip[dx];
		else
			recip_dx = F1_0/dx;

		du_dx = FixMul(uright - uleft,recip_dx);
		dv_dx = FixMul(vright - vleft,recip_dx);

		fx_u = uleft;
		fx_v = vleft;
		fx_du_dx = du_dx;
		fx_dv_dx = dv_dx;
		fx_y = y;
		fx_xright = X2I(xright);
		fx_xleft = X2I(xleft);
		pixptr = srcb->Buffer ();

		switch (bLightingEnabled) {
			case 0:
				//added 07/11/99 adb - prevent writing before the buffer
				if (fx_xleft < 0)
					fx_xleft = 0;
				//end addition -adb
			
				cur_tmap_scanline_lin_nolight();
				break;
			case 1:
				if (lleft < F1_0/2)
					lleft = F1_0/2;
				if (lright < F1_0/2)
					lright = F1_0/2;

				if (lleft > MAX_LIGHTING_VALUE*NUM_LIGHTING_LEVELS)
					lleft = MAX_LIGHTING_VALUE*NUM_LIGHTING_LEVELS;
				if (lright > MAX_LIGHTING_VALUE*NUM_LIGHTING_LEVELS)
					lright = MAX_LIGHTING_VALUE*NUM_LIGHTING_LEVELS;

				//added 07/11/99 adb - prevent writing before the buffer
				if (fx_xleft < 0)
					fx_xleft = 0;
				//end addition -adb

{
			fix mul_thing;

			fx_l = lleft;
			fx_dl_dx = FixMul(lright - lleft,recip_dx);

			//	This is a pretty ugly hack to prevent lighting overflows.
			mul_thing = dx * fx_dl_dx;
			if (lleft + mul_thing < 0)
				fx_dl_dx += 12;
			else if (lleft + mul_thing > (NUM_LIGHTING_LEVELS*F1_0-F1_0/2))
				fx_dl_dx -= 12;
}

				fx_l = lleft;
				dl_dx = FixMul(lright - lleft,recip_dx);
				fx_dl_dx = dl_dx;
				cur_tmap_scanline_lin();
				break;
			case 2:
#ifdef EDITOR_TMAP
				fx_xright = X2I(xright);
				fx_xleft = X2I(xleft);
				tmap_flat_color = 1;
				cur_tmap_scanline_flat();
#else
				Int3();	//	Illegal, called an editor only routine!
#endif
				break;
		}
}

// -------------------------------------------------------------------------------------
//	Render a texture map with lighting using perspective interpolation in inner and outer loops.
// -------------------------------------------------------------------------------------
void ntexture_map_lighted_linear(CBitmap *srcb, g3ds_tmap *t)
{
	int	vlt,vrt,vlb,vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy,boty,y, dy;
	fix	dx_dy_left,dx_dy_right;
	fix	du_dy_left,du_dy_right;
	fix	dv_dy_left,dv_dy_right;
	fix	dl_dy_left,dl_dy_right;
	int	max_y_vertex;
	fix	xleft,xright,uleft,vleft,uright,vright,lleft,lright;
	int	next_break_left, next_break_right;
	fix	recip_dyl, recip_dyr;

	g3ds_vertex *v3d;

        //remove stupid warnings in compile
        dl_dy_left = F1_0;
        dl_dy_right = F1_0;
        lleft = F1_0;
        lright = F1_0;

	v3d = t->verts;

	// Determine top and bottom y coords.
	compute_y_bounds(t,&vlt,&vlb,&vrt,&vrb,&max_y_vertex);

	// Set top and bottom (of entire texture map) y coordinates.
	topy = X2I(v3d[vlt].y2d);
	boty = X2I(v3d[max_y_vertex].y2d);

	if (topy > nWindowClipBot)
		return;
	if (boty > nWindowClipBot)
		boty = nWindowClipBot;

	dy = X2I(t->verts[vlb].y2d) - X2I(t->verts[vlt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyl = fix_recip[dy];
	else
		recip_dyl = F1_0/dy;

	dy = X2I(t->verts[vrb].y2d) - X2I(t->verts[vrt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyr = fix_recip[dy];
	else
		recip_dyr = F1_0/dy;

	// Set amount to change x coordinate for each advance to next scanline.
	dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dyl);
	dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dyr);

	du_dy_left = compute_du_dy_lin(t,vlt,vlb, recip_dyl);
	du_dy_right = compute_du_dy_lin(t,vrt,vrb, recip_dyr);

	dv_dy_left = compute_dv_dy_lin(t,vlt,vlb, recip_dyl);
	dv_dy_right = compute_dv_dy_lin(t,vrt,vrb, recip_dyr);

	if (bLightingEnabled) {
		dl_dy_left = compute_dl_dy_lin(t,vlt,vlb, recip_dyl);
		dl_dy_right = compute_dl_dy_lin(t,vrt,vrb, recip_dyr);

		lleft = v3d[vlt].l;
		lright = v3d[vrt].l;
	}

 	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	uleft = v3d[vlt].u;
	uright = v3d[vrt].u;
	vleft = v3d[vlt].v;
	vright = v3d[vrt].v;

	// scan all rows in texture map from top through first break.
	next_break_left = X2I(v3d[vlb].y2d);
	next_break_right = X2I(v3d[vrb].y2d);

	for (y = topy; y < boty; y++) {

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == next_break_left) {
			fix	recip_dy;

			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == X2I(v3d[vlb].y2d)) {
				vlt = vlb;
				vlb = prevmod(vlb,t->nv);
			}
			next_break_left = X2I(v3d[vlb].y2d);

			dy = X2I(t->verts[vlb].y2d) - X2I(t->verts[vlt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0/dy;

			dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dy);

			xleft = v3d[vlt].x2d;
			uleft = v3d[vlt].u;
			vleft = v3d[vlt].v;
			lleft = v3d[vlt].l;

			du_dy_left = compute_du_dy_lin(t,vlt,vlb, recip_dy);
			dv_dy_left = compute_dv_dy_lin(t,vlt,vlb, recip_dy);

			if (bLightingEnabled) {
				dl_dy_left = compute_dl_dy_lin(t,vlt,vlb, recip_dy);
				lleft = v3d[vlt].l;
			}
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == next_break_right) {
			fix	recip_dy;

			while (y == X2I(v3d[vrb].y2d)) {
				vrt = vrb;
				vrb = succmod(vrb,t->nv);
			}

			dy = X2I(t->verts[vrb].y2d) - X2I(t->verts[vrt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0/dy;

			next_break_right = X2I(v3d[vrb].y2d);
			dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dy);

			xright = v3d[vrt].x2d;
			uright = v3d[vrt].u;
			vright = v3d[vrt].v;

			du_dy_right = compute_du_dy_lin(t,vrt,vrb, recip_dy);
			dv_dy_right = compute_dv_dy_lin(t,vrt,vrb, recip_dy);

			if (bLightingEnabled) {
				dl_dy_right = compute_dl_dy_lin(t,vrt,vrb, recip_dy);
				lright = v3d[vrt].l;
			}
		}

		if (bLightingEnabled) {
			ntmap_scanline_lighted_linear(srcb,y,xleft,xright,uleft,uright,vleft,vright,lleft,lright);
			lleft += dl_dy_left;
			lright += dl_dy_right;
		} else
			ntmap_scanline_lighted_linear(srcb,y,xleft,xright,uleft,uright,vleft,vright,lleft,lright);

		uleft += du_dy_left;
		vleft += dv_dy_left;

		uright += du_dy_right;
		vright += dv_dy_right;

		xleft += dx_dy_left;
		xright += dx_dy_right;

	}

	// We can get lleft or lright out of bounds here because we compute dl_dy using fixed point values,
	//	but we plot an integer number of scanlines, therefore doing an integer number of additions of the delta.

	ntmap_scanline_lighted_linear(srcb,y,xleft,xright,uleft,uright,vleft,vright,lleft,lright);
}

// fix	DivNum = F1_0*12;

extern void DrawTexPolyFlat(CBitmap *bmP,int nverts,g3sPoint **vertbuf);

// -------------------------------------------------------------------------------------
// Interface from Matt's data structures to Mike's texture mapper.
// -------------------------------------------------------------------------------------
void draw_tmap(CBitmap *bmP,int nverts,g3sPoint **vertbuf)
{
	int	i;

	//	These variables are used in system which renders texture maps which lie on one scanline as a line.
	// fix	div_numerator;
	int	lighting_on_save = gameStates.render.nLighting;

	Assert(nverts <= MAX_TMAP_VERTS);


#ifdef USE_MULT_CODE
	if ( !divide_table_filled ) fill_divide_table();
#endif

	// -- now called from G3StartFrame -- init_interface_vars_to_assembler();

	//	If no transparency and seg depth is large, render as flat shaded.
	if ((nCurrentSegDepth > gameStates.render.detail.nMaxLinearDepth) && ((bmP->Flags () & 3) == 0)) {
		DrawTexPolyFlat(bmP, nverts, vertbuf);
		return;
	}

	if ( bmP->Flags () & BM_FLAG_RLE )
		bmP = rle_expand_texture( bmP );		// Expand if rle'd

	gameStates.render.bTransparency = bmP->Flags () & BM_FLAG_TRANSPARENT;
	if (bmP->Flags () & BM_FLAG_NO_LIGHTING)
		gameStates.render.nLighting = 0;


	// Setup texture map in Tmap1
	Tmap1.nv = nverts;						// Initialize number of vertices

// 	div_numerator = DivNum;	//f1_0*3;

	for (i=0; i<nverts; i++) {
		g3ds_vertex	*tvp = &Tmap1.verts[i];
		g3sPoint	*vp = vertbuf[i];

		tvp->x2d = I2X (vp->p3_screen.x);
		tvp->y2d = I2X (vp->p3_screen.y);

		//	Check for overflow on FixDiv.  Will overflow on vp->z <= something small.  Allow only as low as 256.
		if (vp->p3_vec[Z] < 256) {
			vp->p3_vec[Z] = 256;
			// Int3();		// we would overflow if we divided!
		}

		tvp->z = FixDiv(F1_0*12, vp->p3_vec[Z]);
		tvp->u = vp->p3_uvl.u << 6; //* bmP->Width ();
		tvp->v = vp->p3_uvl.v << 6; //* bmP->Height ();

		Assert(gameStates.render.nLighting < 3);

		if (gameStates.render.nLighting)
			tvp->l = vp->p3_uvl.l * NUM_LIGHTING_LEVELS;
	}


	bLightingEnabled = gameStates.render.nLighting;

	// Now, call my texture mapper.
	if (gameStates.render.nLighting) {
		switch (gameStates.render.nInterpolationMethod) {	// 0 = choose, 1 = linear, 2 = /8 perspective, 3 = full perspective
			case 0:								// choose best interpolation
				per2Flag = 1;
				if (nCurrentSegDepth > gameStates.render.detail.nMaxPerspectiveDepth)
					ntexture_map_lighted_linear(bmP, &Tmap1);
				else
					ntexture_map_lighted(bmP, &Tmap1);
				break;
			case 1:								// linear interpolation
				per2Flag = 1;
				ntexture_map_lighted_linear(bmP, &Tmap1);
				break;
			case 2:								// perspective every 8th pixel interpolation
				per2Flag = 1;
				ntexture_map_lighted(bmP, &Tmap1);
				break;
			case 3:								// perspective every pixel interpolation
				per2Flag = 0;					// this hack means do divide every pixel
				ntexture_map_lighted(bmP, &Tmap1);
				break;
			default:
				Assert(0);				// Illegal value for gameStates.render.nInterpolationMethod, must be 0,1,2,3
		}
	} else {
		switch (gameStates.render.nInterpolationMethod) {	// 0 = choose, 1 = linear, 2 = /8 perspective, 3 = full perspective
			case 0:								// choose best interpolation
				per2Flag = 1;
				if (nCurrentSegDepth > gameStates.render.detail.nMaxPerspectiveDepth)
					ntexture_map_lighted_linear(bmP, &Tmap1);
				else
					ntexture_map_lighted(bmP, &Tmap1);
				break;
			case 1:								// linear interpolation
				per2Flag = 1;
				ntexture_map_lighted_linear(bmP, &Tmap1);
				break;
			case 2:								// perspective every 8th pixel interpolation
				per2Flag = 1;
				ntexture_map_lighted(bmP, &Tmap1);
				break;
			case 3:								// perspective every pixel interpolation
				per2Flag = 0;					// this hack means do divide every pixel
				ntexture_map_lighted(bmP, &Tmap1);
				break;
			default:
				Assert(0);				// Illegal value for gameStates.render.nInterpolationMethod, must be 0,1,2,3
		}
	}

	gameStates.render.nLighting = lighting_on_save;

}
