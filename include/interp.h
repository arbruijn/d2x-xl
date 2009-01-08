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

#define WORDVAL(_p)	(*(reinterpret_cast<short *> (_p)))
#define WORDPTR(_p)	(reinterpret_cast<short *> (_p))
#define FIXPTR(_p)	((fix *) (_p))
#define VECPTR(_p)	((CFixVector *) (_p))

//------------------------------------------------------------------------------

//Object functions:

//gives the interpreter an array of points to use
void G3SetModelPoints (g3sPoint *pointlist);

//calls the CObject interpreter to render an CObject.  The CObject renderer
//is really a seperate pipeline. returns true if drew
int G3DrawPolyModel (CObject *objP, void *modelDataP, CBitmap **modelBitmaps, CAngleVector *animAngles, CFixVector *vOffset,
							fix light, fix *glowValues, tRgbaColorf *obj_colors, POF::CModel *modelP, int nModel);

int G3DrawPolyModelShadow (CObject *objP, void *modelDataP, CAngleVector *pAnimAngles, int nModel);

int G3FreePolyModelItems (POF::CModel *modelP);

//init code for bitmap models
void G3InitPolyModel (CPolyModel* modelP, int nModel);

//un-initialize, i.e., convert color entries back to RGB15
void g3_uninit_polygon_model(void *model_ptr);

//alternate interpreter for morphing CObject
int G3DrawMorphingModel(void *model_ptr,CBitmap **model_bitmaps,CAngleVector *animAngles, CFixVector *vOffset,
								 fix light, CFixVector *new_points, int nModel);

//this remaps the 15bpp colors for the models into a new palette.  It should
//be called whenever the palette changes
void g3_remap_interp_colors(void);

int G3CheckAndSwap (void *modelP);

void G3FreeAllPolyModelItems (void);

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
// routine to convert little to big endian in polygon model data
void swap_polygon_model_data(ubyte *data);
//routines to convert little to big endian in vectors
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

int G3RenderModel (CObject *objP, short nModel, short nSubModel, CPolyModel* pp, CBitmap **modelBitmaps,
						 CAngleVector *pAnimAngles, CFixVector *pOffs, fix xModelLight, fix *xGlowValues, tRgbaColorf *pObjColor);

void G3DynLightModel (CObject *objP, RenderModel::CModel* modelP, short iVerts, short nVerts, short iFaceVerts, short nFaceVerts);

int G3ModelMinMax (int nModel, tHitbox *phb);

//------------------------------------------------------------------------------

extern g3sPoint *pointList [MAX_POINTS_PER_POLY];
extern int hitboxFaceVerts [6][4];
extern CFixVector hitBoxOffsets [8];
//extern CAngleVector CAngleVector::ZERO;
//extern CFixVector vZero;
//extern CFixMatrix mIdentity;
extern short nGlow;

//------------------------------------------------------------------------------

static inline void RotatePointListToVec (CFixVector *dest, CFixVector *src, int n) {
	while(n--) {
		transformation.Transform(*dest, *src, 0);
		dest++; src++;
	}
}

//------------------------------------------------------------------------------

static inline int G3HaveModel (int nModel)
{
if (gameData.models.renderModels [0][nModel].m_bValid)
	return 1;
if (gameData.models.renderModels [1][nModel].m_bValid)
	return 2;
return 0;
}

//------------------------------------------------------------------------------

#include "byteswap.h"

static inline void ShortSwap (short *s)
{
*s = SWAPSHORT (*s);
}

//------------------------------------------------------------------------------

static inline void FixSwap (fix *f)
{
*f = (fix)SWAPINT ((int)*f);
}

//------------------------------------------------------------------------------

static inline void VmsVectorSwap(CFixVector& v)
{
FixSwap (FIXPTR (&v[X]));
FixSwap (FIXPTR (&v[Y]));
FixSwap (FIXPTR (&v[Z]));
}

//------------------------------------------------------------------------------

static inline void FixAngSwap (fixang *f)
{
*f = (fixang) SWAPSHORT ((short)*f);
}

//------------------------------------------------------------------------------

static inline void VmsAngVecSwap (CAngleVector& v)
{
FixAngSwap (&v[PA]);
FixAngSwap (&v[BA]);
FixAngSwap (&v[HA]);
}

//------------------------------------------------------------------------------

#endif //_INTERP_H

