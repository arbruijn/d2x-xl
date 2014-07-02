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

#define WORDPTR(_p)	(reinterpret_cast<uint16_t *> (_p))
#define WORDVAL(_p)	(*WORDPTR (_p))
#define FIXPTR(_p)	((fix *) (_p))
#define VECPTR(_p)	((CFixVector *) (_p))

//------------------------------------------------------------------------------

//Object functions:

//gives the interpreter an array of points to use
void G3SetModelPoints (CRenderPoint *pointlist);

//calls the CObject interpreter to render an CObject.  The CObject renderer
//is really a seperate pipeline. returns true if drew
int32_t G3DrawPolyModel (CObject *objP, void *modelDataP, CArray<CBitmap*>& modelBitmaps, CAngleVector *animAngles, CFixVector *vOffset,
							fix light, fix *glowValues, CFloatVector *obj_colors, POF::CModel *modelP, int32_t nModel);

int32_t G3DrawPolyModelShadow (CObject *objP, void *modelDataP, CAngleVector *pAnimAngles, int32_t nModel);

int32_t G3FreePolyModelItems (POF::CModel *modelP);

//init code for bitmap models
void G3InitPolyModel (CPolyModel* modelP, int32_t nModel);

//un-initialize, i.e., convert color entries back to RGB15
void g3_uninit_polygon_model(void *model_ptr);

//alternate interpreter for morphing CObject
int32_t G3DrawMorphingModel(void *model_ptr, CArray<CBitmap*>& modelBitmaps,CAngleVector *animAngles, CFixVector *vOffset,
								 fix light, CFixVector *new_points, int32_t nModel);

//this remaps the 15bpp colors for the models into a new palette.  It should
//be called whenever the palette changes
void g3_remap_interp_colors(void);

int32_t G3CheckAndSwap (void *modelP);

void G3FreeAllPolyModelItems (void);

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
// routine to convert little to big endian in polygon model data
void swap_polygon_model_data(uint8_t *data);
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
	uint8_t *old_base; // where the offset sets off from (relative to beginning of modelData)
	uint8_t *new_base; // where the base is in the aligned structure
	int16_t offset; // how much to add to base to get the address of the offset
	int16_t correction; // how much the value of the offset must be shifted for alignment
} chunk;
#define MAX_CHUNKS 100 // increase if insufficent
/*
 * finds what chunks the data points to, adds them to the chunk_list,
 * and returns the length of the current chunk
 */
int32_t get_chunks(uint8_t *data, uint8_t *new_data, chunk *list, int32_t *no);
#endif //def WORDS_NEED_ALIGNMENT

void G3SwapPolyModelData (uint8_t *data);

int32_t G3RenderModel (CObject *objP, int16_t nModel, int16_t nSubModel, CPolyModel* pp, CArray<CBitmap*>& modelBitmaps,
						 CAngleVector *pAnimAngles, CFixVector *pOffs, fix xModelLight, fix *xGlowValues, CFloatVector *pObjColor);

void G3DynLightModel (CObject *objP, RenderModel::CModel* modelP, int16_t iVerts, int16_t nVerts, int16_t iFaceVerts, int16_t nFaceVerts);

int32_t G3ModelMinMax (int32_t nModel, tHitbox *phb);

//------------------------------------------------------------------------------

extern CRenderPoint *pointList [MAX_POINTS_PER_POLY];
extern int32_t hitboxFaceVerts [6][4];
extern CFixVector hitBoxOffsets [8];
//extern CAngleVector CAngleVector::ZERO;
//extern CFixVector vZero;
//extern CFixMatrix mIdentity;
extern int16_t nGlow;

//------------------------------------------------------------------------------

static inline void RotatePointListToVec (CFixVector *dest, CFixVector *src, int32_t n) {
	while(n--) {
		transformation.Transform(*dest, *src, 0);
		dest++; src++;
	}
}

//------------------------------------------------------------------------------

static inline int32_t G3HaveModel (int32_t nModel)
{
if (gameData.models.renderModels [0][nModel].m_bValid)
	return 1;
if (gameData.models.renderModels [1][nModel].m_bValid)
	return 2;
return 0;
}

//------------------------------------------------------------------------------

#include "byteswap.h"

static inline void ShortSwap (int16_t *s)
{
*s = SWAPSHORT (*s);
}

//------------------------------------------------------------------------------

static inline void UShortSwap (uint16_t *s)
{
*s = SWAPUSHORT (*s);
}

//------------------------------------------------------------------------------

static inline void FixSwap (fix *f)
{
*f = (fix)SWAPINT ((int32_t)*f);
}

//------------------------------------------------------------------------------

static inline void VmsVectorSwap(CFixVector& v)
{
FixSwap (FIXPTR (&v.v.coord.x));
FixSwap (FIXPTR (&v.v.coord.y));
FixSwap (FIXPTR (&v.v.coord.z));
}

//------------------------------------------------------------------------------

static inline void FixAngSwap (fixang *f)
{
*f = (fixang) SWAPSHORT ((int16_t)*f);
}

//------------------------------------------------------------------------------

static inline void VmsAngVecSwap (CAngleVector& v)
{
FixAngSwap (&v.v.coord.p);
FixAngSwap (&v.v.coord.b);
FixAngSwap (&v.v.coord.h);
}

//------------------------------------------------------------------------------

#endif //_INTERP_H

