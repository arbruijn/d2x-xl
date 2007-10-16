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

#define OP_EOF          0   //eof
#define OP_DEFPOINTS    1   //defpoints
#define OP_FLATPOLY     2   //flat-shaded polygon
#define OP_TMAPPOLY     3   //texture-mapped polygon
#define OP_SORTNORM     4   //sort by normal
#define OP_RODBM        5   //rod bitmap
#define OP_SUBCALL      6   //call a subobject
#define OP_DEFP_START   7   //defpoints with start
#define OP_GLOW         8   //glow value for next poly

#define MAX_POINTS_PER_POLY 25

#define WORDVAL(_p)	(*((short *) (_p)))
#define WORDPTR(_p)	((short *) (_p))
#define FIXPTR(_p)	((fix *) (_p))
#define VECPTR(_p)	((vmsVector *) (_p))

//------------------------------------------------------------------------------

//Object functions:

fix G3PolyModelSize (tPolyModel *pm, int nModel);

//gives the interpreter an array of points to use
void G3SetModelPoints(g3sPoint *pointlist);

//calls the tObject interpreter to render an tObject.  The tObject renderer
//is really a seperate pipeline. returns true if drew
bool G3DrawPolyModel (tObject *objP, void *modelP, grsBitmap **modelBitmaps, vmsAngVec *animAngles, vmsVector *vOffset,
							 fix light, fix *glowValues, tRgbaColorf *obj_colors, tPOFObject *po, int nModel);

int G3DrawPolyModelShadow (tObject *objP, void *modelP, vmsAngVec *pAnimAngles, int nModel);

int G3FreePolyModelItems (tPOFObject *po);

//init code for bitmap models
void G3InitPolyModel(tPolyModel *pm, int nModel);

//un-initialize, i.e., convert color entries back to RGB15
void g3_uninit_polygon_model(void *model_ptr);

//alternate interpreter for morphing tObject
bool G3DrawMorphingModel(void *model_ptr,grsBitmap **model_bitmaps,vmsAngVec *animAngles, vmsVector *vOffset, 
								 fix light, vmsVector *new_points, int nModel);

//this remaps the 15bpp colors for the models into a new palette.  It should
//be called whenever the palette changes
void g3_remap_interp_colors(void);

int G3CheckAndSwap (void *modelP);

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
 * relative to start of modelData) to it.
 */
typedef struct chunk {
	ubyte *old_base; // where the offset sets off from (relative to beginning of modelData)
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

void G3SwapPolyModelData (ubyte *data);

int G3RenderModel (tObject *objP, int nModel, tPolyModel *pp, grsBitmap **modelBitmaps, vmsAngVec *pAnimAngles, 
						 fix xModelLight, fix *xGlowValues, tRgbaColorf *pObjColor);

void G3StartModelLightThreads (void);
void G3EndModelLightThreads (void);

//------------------------------------------------------------------------------

extern g3sPoint *pointList [MAX_POINTS_PER_POLY];
extern int hitboxFaceVerts [6][4];
extern vmsVector hitBoxOffsets [8];
extern vmsAngVec avZero;
extern vmsVector vZero;
extern vmsMatrix mIdentity;
extern short nGlow;

//------------------------------------------------------------------------------

static inline void RotatePointListToVec (vmsVector *dest, vmsVector *src, int n)
{
while (n--)
	G3TransformPoint (dest++, src++, 0);
}

//------------------------------------------------------------------------------

#endif //_INTERP_H

