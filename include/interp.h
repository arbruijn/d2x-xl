/* $Id: interp.h,v 1.6 2003/02/13 22:07:58 btb Exp $ */
/*
 *
 * took out functions declarations from include/3d.h
 * which are implemented in 3d/interp.c
 *
 */

#ifndef _INTERP_H
#define _INTERP_H

#include "fix.h"
//#include "vecmat.h" //the vector/matrix library
#include "gr.h"
#include "cfile.h"
#include "3d.h"

//Object functions:

fix G3PolyModelSize (void *model_ptr);

//gives the interpreter an array of points to use
void G3SetModelPoints(g3s_point *pointlist);

//calls the tObject interpreter to render an tObject.  The tObject renderer
//is really a seperate pipeline. returns true if drew
bool G3DrawPolyModel (tObject *objP, void *model_ptr,grs_bitmap **model_bitmaps,vmsAngVec *animAngles,
							 fix light,fix *glowValues, tRgbColorf *obj_colors, tPOFObject *po);

int G3DrawPolyModelShadow (tObject *objP, void *modelP, vmsAngVec *pAnimAngles);

int G3FreePolyModelItems (tPOFObject *po);

//init code for bitmap models
void G3InitPolyModel(void *model_ptr);

//un-initialize, i.e., convert color entries back to RGB15
void g3_uninit_polygon_model(void *model_ptr);

//alternate interpreter for morphing tObject
bool G3DrawMorphingModel(void *model_ptr,grs_bitmap **model_bitmaps,vmsAngVec *animAngles,fix light,vmsVector *new_points);

//this remaps the 15bpp colors for the models into a new palette.  It should
//be called whenever the palette changes
void g3_remap_interp_colors(void);

G3CheckAndSwap (modelP);

void G3FreeAllPolyModelItems (void);

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
// routine to convert little to big endian in polygon model data
void swap_polygon_model_data(ubyte *data);
//routines to convert little to big endian in vectors
void VmsVectorSwap(vmsVector *v);
void VmsAngVecSwap(vmsAngVec *v);
#endif

#ifdef WORDS_NEED_ALIGNMENT
/*
 * A chunk struct (as used for alignment) contains all relevant data
 * concerning a piece of data that may need to be aligned.
 * To align it, we need to copy it to an aligned position,
 * and update all pointers  to it.
 * (Those pointers are actually offsets
 * relative to start of model_data) to it.
 */
typedef struct chunk {
	ubyte *old_base; // where the offset sets off from (relative to beginning of model_data)
	ubyte *new_base; // where the base is in the aligned structure
	short offset; // how much to add to base to get the address of the offset
	short correction; // how much the value of the offset must be shifted for alignment
} chunk;
#define MAX_CHUNKS 100 // increase if insufficent
/*
 * finds what chunks the data points to, adds them to the chunk_list, 
 * and returns the length of the current chunk
 */
int get_chunks(ubyte *data, ubyte *new_data, chunk *list, int *no);
#endif //def WORDS_NEED_ALIGNMENT

#endif //_INTERP_H
