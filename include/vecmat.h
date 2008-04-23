/* $ Id: $ */
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

#ifndef _VECMAT_H
#define _VECMAT_H

#include "maths.h"

#define QUICK_VEC_MATH 0

//#define INLINE 1              //are some of these functions inline?

typedef union fVector3 {
	float v [3];
	struct {float x, y, z;} p;
	struct {float r, g, b;} c;
} fVector3;

typedef union fVector {
	float v [4];
	struct {float x, y, z, w;} p;
	struct {float r, g, b, a;} c;
	fVector3 v3;
} fVector;

//The basic fixed-point vector.  Access elements by name or position
typedef union vmsVector {
	fix v [3];
	struct {fix x, y, z;} p;
  } __pack__ vmsVector;


//Short vector, used for pre-rotation points. 
//Access elements by name or position
typedef struct vms_svec {
    short sv_x, sv_y, sv_z;
  } __pack__ vms_svec;


//Angle vector.  Used to store orientations
typedef struct vmsAngVec {
    fixang p, b, h;
  } __pack__ vmsAngVec;


//A 3x3 rotation matrix.  Sorry about the numbering starting with one.
//Ordering is across then down, so <m1,m2,m3> is the first row
typedef struct vmsMatrix {
    vmsVector rVec, uVec, fVec;
  } __pack__ vmsMatrix;

typedef struct fMatrix {
	fVector		rVec, uVec, fVec, wVec;
} fMatrix;

//Macros/functions to fill in fields of structures

//macro to check if vector is zero
#define IS_VEC_NULL(v) (v->p.x == 0 && v->p.y == 0 && v->p.z == 0)

//macro to set a vector to zero.  we could do this with an in-line assembly 
//macro, but it's probably better to let the compiler optimize it.
//Note: NO RETURN VALUE
#define VmVecZero(v) (v)->p.x = (v)->p.y = (v)->p.z = 0

//macro set set a matrix to the identity. Note: NO RETURN VALUE

// DPH (18/9/98): Begin mod to fix linefeed problem under linux. Uses an
// inline function instead of a multi-line macro to fix CR/LF problems.

static inline void VmSetIdentity(vmsMatrix *m)
{
m->rVec.p.x = m->uVec.p.y = m->fVec.p.z = f1_0;
m->rVec.p.y = m->rVec.p.z = m->uVec.p.x = m->uVec.p.z = m->fVec.p.x = m->fVec.p.y = 0;
}

// DPH (19/8/98): End changes.

vmsVector *VmVecMake (vmsVector *v, fix x, fix y, fix z);

vmsAngVec *VmAngVecMake (vmsAngVec * v, fixang p, fixang b, fixang h);

//Global constants

extern vmsVector vmdZeroVector;
extern vmsMatrix vmdIdentityMatrix;

//returns magnitude of a vector
fix VmVecMag (vmsVector *v);
float VmVecMag (fVector *v);
float VmVecMag (fVector3 *v);

//Here's a handy constant

#define ZERO_VECTOR {{0,0,0}}
#define IDENTITY_MATRIX {{{f1_0,0,0}}, {{0,f1_0,0}}, {{0,0,f1_0}}}

//negate a vector
static inline vmsVector *VmVecNegate (vmsVector *v)
	{v->p.x = -v->p.x; v->p.y = -v->p.y; v->p.z = -v->p.z; return v;}

static inline fVector *VmVecNegate (fVector *v)
	{v->p.x = -v->p.x; v->p.y = -v->p.y; v->p.z = -v->p.z; return v;}

static inline fVector3 *VmVecNegate (fVector3 *v)
	{v->p.x = -v->p.x; v->p.y = -v->p.y; v->p.z = -v->p.z; return v;}

//Functions in library

#ifndef INLINE

#define INLINE_VEC_ADD 1

#if INLINE_VEC_ADD

static inline vmsVector *VmVecAdd (vmsVector *d, vmsVector *s0, vmsVector *s1)
	{d->p.x = s0->p.x + s1->p.x; d->p.y = s0->p.y + s1->p.y; d->p.z = s0->p.z + s1->p.z; return d;}

static inline vmsVector *VmVecSub (vmsVector *d, vmsVector *s0, vmsVector *s1)
	{d->p.x = s0->p.x - s1->p.x; d->p.y = s0->p.y - s1->p.y; d->p.z = s0->p.z - s1->p.z; return d;}

static inline fVector *VmVecAdd (fVector *d, fVector *s0, fVector *s1)
	{d->p.x = s0->p.x + s1->p.x; d->p.y = s0->p.y + s1->p.y; d->p.z = s0->p.z + s1->p.z; return d;}

static inline fVector3 *VmVecAdd (fVector3 *d, fVector3 *s0, fVector3 *s1)
	{d->p.x = s0->p.x + s1->p.x; d->p.y = s0->p.y + s1->p.y; d->p.z = s0->p.z + s1->p.z; return d;}

static inline fVector *VmVecSub (fVector *d, fVector *s0, fVector *s1)
	{d->p.x = s0->p.x - s1->p.x; d->p.y = s0->p.y - s1->p.y; d->p.z = s0->p.z - s1->p.z; return d;}

static inline fVector3 *VmVecSub (fVector3 *d, fVector3 *s0, fVector3 *s1)
	{d->p.x = s0->p.x - s1->p.x; d->p.y = s0->p.y - s1->p.y; d->p.z = s0->p.z - s1->p.z; return d;}

static inline vmsVector *VmVecDec (vmsVector *d, vmsVector *s)
	{d->p.x -= s->p.x; d->p.y -= s->p.y; d->p.z -= s->p.z; return d;}

static inline vmsVector *VmVecInc (vmsVector *d, vmsVector *s)
	{d->p.x += s->p.x; d->p.y += s->p.y; d->p.z += s->p.z; return d;}

static inline fVector *VmVecInc (fVector *d, fVector *s)
	{d->p.x += s->p.x; d->p.y += s->p.y; d->p.z += s->p.z; return d;}

static inline fVector3 *VmVecInc (fVector3 *d, fVector3 *s)
	{d->p.x += s->p.x; d->p.y += s->p.y; d->p.z += s->p.z; return d;}

static inline fVector *VmVecDec (fVector *d, fVector *s)
	{d->p.x -= s->p.x; d->p.y -= s->p.y; d->p.z -= s->p.z; return d;}

static inline fVector3 *VmVecDec (fVector3 *d, fVector3 *s)
	{d->p.x -= s->p.x; d->p.y -= s->p.y; d->p.z -= s->p.z; return d;}

static inline fVector *VmVecFixToFloat (fVector *d, vmsVector *s)
	{d->p.x = f2fl (s->p.x); d->p.y = f2fl (s->p.y); d->p.z = f2fl (s->p.z); d->p.w = 1; return d;}

static inline fVector3 *VmVecFixToFloat (fVector3 *d, vmsVector *s)
	{d->p.x = f2fl (s->p.x); d->p.y = f2fl (s->p.y); d->p.z = f2fl (s->p.z); return d;}

static inline vmsVector *VmVecFloatToFix (vmsVector *d, fVector *s)
	{d->p.x = fl2f (s->p.x); d->p.y = fl2f (s->p.y); d->p.z = fl2f (s->p.z); return d;}

static inline vmsVector *VmVecFloatToFix (vmsVector *d, fVector3 *s)
	{d->p.x = fl2f (s->p.x); d->p.y = fl2f (s->p.y); d->p.z = fl2f (s->p.z); return d;}

static inline fix VmVecDist (vmsVector *v0, vmsVector *v1) {vmsVector d; VmVecSub (&d, v0, v1); return VmVecMag (&d); }

fMatrix *VmsMatToFloat (fMatrix *dest, vmsMatrix *src);

#else
//adds two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use VmVecInc() if so
vmsVector *VmVecAdd (vmsVector *dest, vmsVector *src0, vmsVector *src1);

//subs two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use VmVecDec() if so
vmsVector *VmVecSub (vmsVector *dest, vmsVector *src0, vmsVector *src1);

//adds one vector to another. returns ptr to dest
//dest can equal source
vmsVector *VmVecInc (vmsVector *dest, vmsVector *src);

//subs one vector from another, returns ptr to dest
//dest can equal source
vmsVector *VmVecDec (vmsVector *dest, vmsVector *src);

#endif

#else

#define	VmVecAdd(dest,src0,src1) \
			(dest)->p.x = (src0)->p.x + (src1)->p.x, (dest)->p.y = (src0)->p.y + (src1)->p.y, (dest)->p.z = (src0)->p.z + (src1)->p.z;

#define	VmVecSub(dest,src0,src1) \
			(dest)->p.x = (src0)->p.x - (src1)->p.x, (dest)->p.y = (src0)->p.y - (src1)->p.y, (dest)->p.z = (src0)->p.z - (src1)->p.z;


#define	VmVecInc(dest,src)	\
			(dest)->p.x += (src)->p.x, (dest)->p.y += (src)->p.y, (dest)->p.z += (src)->p.z;


#define	VmVecDec(dest,src) \
			(dest)->p.x -= (src)->p.x, (dest)->p.y -= (src)->p.y, (dest)->p.z -= (src)->p.z;

#endif	/* 
 */

fVector *VmVecMul (fVector *dest, fVector *src0, fVector *src1);

//averages two vectors. returns ptr to dest
//dest can equal either source
vmsVector *VmVecAvg (vmsVector *dest, vmsVector *src0, vmsVector *src1);

fVector *VmVecAvg (fVector *dest, fVector *src0, fVector *src1);
fVector3 *VmVecAvg (fVector3 *dest, fVector3 *src0, fVector3 *src1);

//averages four vectors. returns ptr to dest
//dest can equal any source
vmsVector *VmVecAvg4 (vmsVector *dest, vmsVector *src0, vmsVector *src1, vmsVector *src2, vmsVector *src3);

//scales a vector in place.  returns ptr to vector
vmsVector *VmVecScale (vmsVector *dest, fix s);
fVector *VmVecScale (fVector *dest, fVector *src, float scale);
fVector3 *VmVecScale (fVector3 *dest, fVector3 *src, float scale);

//scales and copies a vector.  returns ptr to dest
vmsVector *VmVecCopyScale (vmsVector *dest, vmsVector *src, fix s);

//scales a vector, adds it to another, and stores in a 3rd vector
//dest = src1 + k * src2
vmsVector *VmVecScaleAdd (vmsVector *dest, vmsVector *src1, vmsVector *src2, fix k);
fVector *VmVecScaleAdd (fVector *dest, fVector *src1, fVector *src2, float scale);

//scales a vector and adds it to another
//dest += k * src
vmsVector *VmVecScaleInc (vmsVector *dest, vmsVector *src, fix k);
fVector *VmVecScaleInc (fVector *dest, fVector *src, float scale);
fVector3 *VmVecScaleInc (fVector3 *dest, fVector3 *src, float scale);

//scales a vector in place, taking n/d for scale.  returns ptr to vector
//dest *= n/d
vmsVector *VmVecScaleFrac (vmsVector *dest, fix n, fix d);

//computes the distance between two points. (does sub and mag)
fix VmVecDist (vmsVector *v0, vmsVector *v1);
float VmVecDist (fVector *v0, fVector *v1);
float VmVecDist (fVector3 *v0, fVector3 *v1);

//computes an approximation of the magnitude of the vector
//uses dist = largest + next_largest*3/8 + smallest*3/16
#if QUICK_VEC_MATH
fix FixVecMagQuick (fix a, fix b, fix c);
#define VmVecMagQuick(_v) FixVecMagQuick((_v)->x, (_v)->y, (_v)->z)
#else
# define VmVecMagQuick(_v) VmVecMag(_v)
#endif

//computes an approximation of the distance between two points.
//uses dist = largest + next_largest*3/8 + smallest*3/16
#if QUICK_VEC_MATH
#	define	VmVecDistQuick(_v0, _v1) \
			FixVecMagQuick ((_v0)->x - (_v1)->x, (_v0)->y - (_v1)->y, (_v0)->z - (_v1)->z)
#else
#	define	VmVecDistQuick(_v0, _v1)	VmVecDist (_v0, _v1)
#endif


//normalize a vector. returns mag of source vec
fix VmVecNormalize (vmsVector *dest, vmsVector *src = NULL);
float VmVecNormalize (fVector *dest, fVector *src = NULL);
float VmVecNormalize (fVector3 *dest, fVector3 *src = NULL);

////returns dot product of two vectors
fix VmVecDot (vmsVector *v0, vmsVector *v1);
float VmVecDot (fVector *v0, fVector *v1);
float VmVecDot (fVector3 *v0, fVector3 *v1);

//computes cross product of two vectors. returns ptr to dest
//dest CANNOT equal either source
vmsVector *VmVecCrossProd (vmsVector *dest, vmsVector *src0, vmsVector *src1);

//computes surface normal from three points. result is normalized
//returns ptr to dest
//dest CANNOT equal either source
vmsVector *VmVecNormalChecked (vmsVector *dest, vmsVector *p0, vmsVector *p1, vmsVector *p2);

//computes non-normalized surface normal from three points. 
//returns ptr to dest
//dest CANNOT equal either source
vmsVector *VmVecPerp (vmsVector *dest, vmsVector *p0, vmsVector *p1, vmsVector *p2);
vmsVector *VmVecPerpChecked (vmsVector *dest, vmsVector *p0, vmsVector *p1, vmsVector *p2);

//computes the delta angle between two vectors. 
//vectors need not be normalized. if they are, call VmVecDeltaAngNorm()
//the forward vector (third parameter) can be NULL, in which case the absolute
//value of the angle in returned.  Otherwise the angle around that vector is
//returned.
fixang VmVecDeltaAng (vmsVector *v0, vmsVector *v1, vmsVector *fVec);
float VmVecDeltaAng (fVector *v0, fVector *v1, fVector *fVec);

//computes the delta angle between two normalized vectors. 
fixang VmVecDeltaAngNorm (vmsVector *v0, vmsVector *v1, vmsVector *fVec);
float VmVecDeltaAngNorm (fVector *v0, fVector *v1, fVector *fVec);

//computes a matrix from a set of three angles.  returns ptr to matrix
vmsMatrix *VmAngles2Matrix (vmsMatrix * m, vmsAngVec * a);


//computes a matrix from a forward vector and an angle
vmsMatrix *VmVecAng2Matrix (vmsMatrix * m, vmsVector *v, fixang a);


//computes a matrix from one or more vectors. The forward vector is required,
//with the other two being optional.  If both up & right vectors are passed,
//the up vector is used.  If only the forward vector is passed, a bank of
//zero is assumed
//returns ptr to matrix
vmsMatrix *VmVector2Matrix (vmsMatrix * m, vmsVector *fVec, vmsVector *uVec, vmsVector *rVec);


//this version of vector_2_matrix requires that the vectors be more-or-less
//normalized and close to perpendicular
vmsMatrix *VmVector2MatrixNorm (vmsMatrix * m, vmsVector *fVec, vmsVector *uVec, vmsVector *rVec);


//rotates a vector through a matrix. returns ptr to dest vector
//dest CANNOT equal either source
vmsVector *VmVecRotate (vmsVector *dest, vmsVector *src, vmsMatrix * m);
fVector *VmVecRotate (fVector *dest, fVector *src, fMatrix *m);
fVector3 *VmVecRotate (fVector3 *dest, fVector3 *src, fMatrix *m);

//transpose a matrix in place. returns ptr to matrix
vmsMatrix *VmTransposeMatrix (vmsMatrix *m);
fMatrix *VmTransposeMatrix (fMatrix *m);

#define VmTranspose(m) VmTransposeMatrix(m)

//copy and transpose a matrix. returns ptr to matrix
//dest CANNOT equal source. use VmTransposeMatrix() if this is the case
vmsMatrix *VmCopyTransposeMatrix (vmsMatrix * dest, vmsMatrix * src);
fMatrix *VmCopyTransposeMatrix (fMatrix *dest, fMatrix *src);

#define VmCopyTranspose(dest,src) VmCopyTransposeMatrix((dest),(src))

//mulitply 2 matrices, fill in dest.  returns ptr to dest
//dest CANNOT equal either source
vmsMatrix *VmMatMul (vmsMatrix * dest, vmsMatrix * src0, vmsMatrix * src1);

//extract angles from a matrix 
vmsAngVec *VmExtractAnglesMatrix (vmsAngVec * a, vmsMatrix * m);

//extract heading and pitch from a vector, assuming bank==0
vmsAngVec *VmExtractAnglesVector (vmsAngVec * a, vmsVector *v);

vmsVector *MakeRandomVector (vmsVector *v);

//compute the distance from a point to a plane.  takes the normalized normal
//of the plane (ebx), a point on the plane (edi), and the point to check (esi).
//returns distance in eax
//distance is signed, so negative dist is on the back of the plane
fix VmDistToPlane (vmsVector *checkp, vmsVector *norm, vmsVector *planep);

vmsVector *VmPlaneProjection (vmsVector *i, vmsVector *n, vmsVector *p, vmsVector *a);

int VmTriangleHitTest (vmsVector *n, vmsVector *p1, vmsVector *p2, vmsVector *p3, vmsVector *i);

int VmTriangleHitTestQuick (vmsVector *n, vmsVector *p1, vmsVector *p2, vmsVector *p3, vmsVector *a);

vmsMatrix *VmSinCos2Matrix (vmsMatrix *m, fix sinp, fix cosp, fix sinb, fix cosb, fix sinh, fix cosh);
int VmPointLineIntersection (vmsVector *hitP, vmsVector *p1, vmsVector *p2, vmsVector *p3, vmsVector *vPos, int bClampToFarthest);
fix VmLinePointDist (vmsVector *a, vmsVector *b, vmsVector *p);
int VmPointLineIntersection (fVector *hitP, fVector *p1, fVector *p2, fVector *p3, fVector *vPos, int bClamp);
int VmPointLineIntersection (fVector3 *hitP, fVector3 *p1, fVector3 *p2, fVector3 *p3, fVector3 *vPos, int bClamp);
float VmLinePointDist (fVector *a, fVector *b, fVector *p, int bClamp);
float VmLinePointDist (fVector3 *a, fVector3 *b, fVector3 *p, int bClamp);
float VmLineLineIntersection (fVector3 *v1, fVector3 *v2, fVector3 *v3, fVector3 *v4, fVector3 *va, fVector3 *vb);
float VmLineLineIntersection (fVector *v1, fVector *v2, fVector *v3, fVector *v4, fVector *va, fVector *vb);
vmsVector *VmVecReflect (vmsVector *vReflect, vmsVector *vDir, vmsVector *vNormal);
fVector *VmVecReflect (fVector *vReflect, fVector *vDir, fVector *vNormal);
fVector3 *VmVecReflect (fVector3 *vReflect, fVector3 *vDir, fVector3 *vNormal);

fVector *VmVecCrossProd (fVector *dest, fVector *src0, fVector *src1);
fVector *VmVecPerp (fVector *dest, fVector *p0, fVector *p1, fVector *p2);
fVector *VmVecNormal (fVector *dest, fVector *p0, fVector *p1, fVector *p2);
fVector3 *VmVecNormal (fVector3 *dest, fVector3 *p0, fVector3 *p1, fVector3 *p2);
fMatrix *VmSinCos2Matrix (fMatrix *m, float sinp, float cosp, float sinb, float cosb, float sinh, float cosh);
fMatrix *VmSinCos2Matrixd (fMatrix *m, double sinp, double cosp, double sinb, double cosb, double sinh, double cosh);

fix VmMatrixDetValue (vmsMatrix *det);
float VmMatrixDetValue (fMatrix *det);
vmsMatrix *VmInvertMatrix (vmsMatrix *pDest, vmsMatrix *pSrc);
fMatrix *VmInvertMatrix (fMatrix *pDest, fMatrix *pSrc);

//fills in fields of an angle vector
#define VmAngVecMake(v,_p,_b,_h) (((v)->p=(_p), (v)->b=(_b), (v)->h=(_h)), (v))

// ------------------------------------------------------------------------
//return the normalized direction vector between two points
//dest = normalized(end - start).  Returns mag of direction vector
//NOTE: the order of the parameters matches the vector subtraction
static inline fix VmVecNormalizedDir (vmsVector *dest, vmsVector *end, vmsVector *start)
{
return VmVecNormalize (VmVecSub (dest, end, start));
}

// ------------------------------------------------------------------------
//computes surface normal from three points. result is normalized
//returns ptr to dest
//dest CANNOT equal either source
static inline vmsVector *VmVecNormal (vmsVector *dest, vmsVector *p0, vmsVector *p1, vmsVector *p2)
{
VmVecNormalize (VmVecPerp (dest, p0, p1, p2));
return dest;
}

// ------------------------------------------------------------------------

#endif //_VECMAT_H
