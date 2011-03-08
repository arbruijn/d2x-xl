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
#include <cstdlib>
#include <string.h>

//namespace VecMat {

const size_t X = 0;
const size_t Y = 1;
const size_t Z = 2;
const size_t W = 3;

const size_t R = 0;
const size_t G = 1;
const size_t B = 2;
const size_t A = 3;

const size_t PA = 0;
const size_t BA = 1;
const size_t HA = 2;

const size_t RVEC = 0;
const size_t UVEC = 1;
const size_t FVEC = 2;
const size_t HVEC = 3;

class CFixVector;
class CFloatVector;
class CFloatVector3;
class CAngleVector;

class CFixMatrix;
class CFloatMatrix;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 * \class __pack__ CFixVector
 * A 3 element fixed-point vector.
 */
class __pack__ CFixVector {
#if 1
	public:
		union {
			struct {
				fix	x, y, z;
			} c;
			fix	a [3];
		} v;
#else
	private:
		fix v [3];
#endif

	public:
		static const CFixVector ZERO;
		static const CFixVector XVEC;
		static const CFixVector YVEC;
		static const CFixVector ZVEC;

		static const CFixVector Create (fix f0, fix f1, fix f2);
		static const CFixVector Avg (const CFixVector& src0, const CFixVector& src1);
		static const CFixVector Avg (CFixVector& src0, CFixVector& src1, CFixVector& src2, CFixVector& src3);
		void Check (void);
		static const CFixVector Cross (const CFixVector& v0, const CFixVector& v1);
		static CFixVector& Cross (CFixVector& dest, const CFixVector& v0, const CFixVector& v1);
		// computes the delta angle between two vectors.
		// vectors need not be normalized. if they are, call CFixVector::DeltaAngleNorm ()
		// the forward vector (third parameter) can be NULL, in which case the absolute
		// value of the angle in returned.  Otherwise the angle around that vector is
		// returned.
		static const fixang DeltaAngle (const CFixVector& v0, const CFixVector& v1, CFixVector *fVec);		//computes the delta angle between two normalized vectors.
		static const fixang DeltaAngleNorm (const CFixVector& v0, const CFixVector& v1, CFixVector *fVec);
		static const fix Dist (const CFixVector& vec0, const CFixVector& vec1);
		static const fix Dot (const CFixVector& v0, const CFixVector& v1);
		static const fix Dot (const fix x, const fix y, const fix z, const CFixVector& v);
		static const fix Normalize (CFixVector& v);
		static const CFixVector Perp (const CFixVector& p0, const CFixVector& p1, const CFixVector& p2);
		static CFixVector& Perp (CFixVector& dest, const CFixVector& p0, const CFixVector& p1, const CFixVector& p2);
		static const CFixVector Normal (const CFixVector& p0, const CFixVector& p1, const CFixVector& p2);
		static const CFixVector Random (void);
		static const CFixVector Reflect (const CFixVector& d, const CFixVector& n);
		// return the normalized direction vector between two points
		// dest = normalized (end - start).  Returns Mag of direction vector
		// NOTE: the order of the parameters m.matches the vector subtraction
		static const fix NormalizedDir (CFixVector& dest, const CFixVector& end, const CFixVector& start);

		// access op for assignment
		fix& operator[] (size_t i);
		// read-only access op
		const fix operator[] (size_t i) const;

		bool operator== (const CFixVector& rhs) const;

		const CFixVector& Set (fix x, fix y, fix z);
		void Set (const fix *vec);

		bool IsZero (void) const;
		void SetZero (void);
		const int Sign (void) const;

		fix SqrMag (void) const;
		float Sqr (float f) const;
		fix Mag (void) const;
		CFixVector& Scale (CFixVector& scale);
		CFixVector& Neg (void);
		const CFixVector operator- (void) const;
		const bool operator== (const CFixVector& vec);
		const bool operator!= (const CFixVector& vec);
		const CFixVector& operator+= (const CFixVector& vec);
		const CFixVector& operator+= (const CFloatVector& vec);
		const CFixVector& operator-= (const CFloatVector& vec);
		const CFixVector& operator-= (const CFixVector& vec);
		const CFixVector& operator*= (const CFixVector& other);
		const CFixVector& operator*= (const fix s);
		const CFixVector& operator/= (const fix s);
		const CFixVector operator+ (const CFixVector& vec) const;
		const CFixVector operator+ (const CFloatVector& vec) const;
		const CFixVector operator- (const CFixVector& vec) const;
		const CFixVector operator- (const CFloatVector& vec) const;
		CFixVector& Assign (const CFloatVector3& other);
		CFixVector& Assign (const CFloatVector& other);
		CFixVector& Assign (const CFixVector& other);

		inline fix& X (void) { return v.c.x; }
		inline fix& Y (void) { return v.c.y; }
		inline fix& Z (void) { return v.c.z; }

		// compute intersection of a line through a point a, with the line being orthogonal relative
		// to the plane given by the Normal n and a point p lieing in the plane, and store it in i.
		const CFixVector PlaneProjection (const CFixVector& n, const CFixVector& p) const;
		//compute the distance from a point to a plane.  takes the normalized Normal
		//of the plane (ebx), a point on the plane (edi), and the point to check (esi).
		//returns distance in eax
		//distance is signed, so Negative Dist is on the back of the plane
		const fix DistToPlane (const CFixVector& n, const CFixVector& p) const;
		//extract heading and pitch from a vector, assuming bank==0
		const CAngleVector ToAnglesVecNorm (void) const;
		//extract heading and pitch from a vector, assuming bank==0
		const CAngleVector ToAnglesVec (void) const;
};

inline const fix operator* (const CFixVector& v0, const CFixVector& v1);
inline const CFixVector operator* (const CFixVector& v, const fix s);
inline const CFixVector operator* (const fix s, const CFixVector& v);
inline const CFixVector operator/ (const CFixVector& v, const fix d);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 * \class __pack__ CFloatVector
 * A 4 element floating point vector class
 */
class __pack__ CFloatVector {
#if 1
	public:
		union {
			struct {
				float x, y, z, w;
			} c;
			float a [4];
		} v;
#else
	private:
		float v [4];
#endif

	public:
		static const CFloatVector ZERO;
		static const CFloatVector ZERO4;
		static const CFloatVector XVEC;
		static const CFloatVector YVEC;
		static const CFloatVector ZVEC;

		static const CFloatVector Create (float f0, float f1, float f2, float f3 = 1.0f);

		static const CFloatVector Avg (const CFloatVector& src0, const CFloatVector& src1);
		static const CFloatVector Cross (const CFloatVector& v0, const CFloatVector& v1);
		static CFloatVector& Cross (CFloatVector& dest, const CFloatVector& v0, const CFloatVector& v1);

		static const float Dist (const CFloatVector& v0, const CFloatVector& v1);
		static const float Dot (const CFloatVector& v0, const CFloatVector& v1);
		static const float Normalize (CFloatVector& vec);
		static const CFloatVector Perp (const CFloatVector& p0, const CFloatVector& p1, const CFloatVector& p2);
		static CFloatVector& Perp (CFloatVector& dest, const CFloatVector& p0, const CFloatVector& p1, const CFloatVector& p2);
		static const CFloatVector Normal (const CFloatVector& p0, const CFloatVector& p1, const CFloatVector& p2);
		static const CFloatVector Reflect (const CFloatVector& d, const CFloatVector& n);

		inline float& X (void) { return v.c.x; }
		inline float& Y (void) { return v.c.y; }
		inline float& Z (void) { return v.c.z; }
		// access op for assignment
		float& operator[] (size_t i);
		// read-only access op
		const float operator[] (size_t i) const;

		bool IsZero (void) const;

		void SetZero (void);
		void Set (const float f0, const float f1, const float f2, const float f3=1.0f);
		void Set (const float *vec);
		const float SqrMag (void) const;
		const float Mag (void) const;
		CFloatVector& Neg (void);
		CFloatVector& Scale (CFloatVector& scale);
		CFloatVector3* XYZ (void);

		const CFloatVector operator- (void) const;
		const bool operator== (const CFloatVector& other);
		const bool operator!= (const CFloatVector& other);
		const CFloatVector& operator+= (const CFloatVector& other);
		const CFloatVector& operator+= (const CFixVector& other);
		const CFloatVector& operator-= (const CFixVector& other);
		const CFloatVector& operator-= (const CFloatVector& other);
		const CFloatVector& operator*= (const CFloatVector& other);
		const CFloatVector& operator*= (const float s);
		const CFloatVector& operator/= (const float s);
		const CFloatVector  operator+ (const CFloatVector& other) const;
		const CFloatVector  operator+ (const CFixVector& other) const;
		const CFloatVector  operator- (const CFloatVector& other) const;
		const CFloatVector  operator- (const CFixVector& other) const;
		CFloatVector& Assign (const CFloatVector3& other);
		CFloatVector& Assign (const CFloatVector& other);
		CFloatVector& Assign (const CFixVector& other);
		const float DistToPlane (const CFloatVector& n, const CFloatVector& p) const;
};

const float operator* (const CFloatVector& v0, const CFloatVector& v1);
const CFloatVector operator* (const CFloatVector& v, const float s);
const CFloatVector operator* (const float s, const CFloatVector& v);
const CFloatVector operator/ (const CFloatVector& v, const float s);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \class __pack__ CFloatVector3
 * A 3 element floating point vector class
 */
class __pack__ CFloatVector3 {
#if 1
	public:
		union {
			struct {
				float x, y, z;
			} c;
			float a [3];
		} v;
#else
	private:
		float v [3];
#endif
	public:
		static const CFloatVector3 ZERO;
		static const CFloatVector3 XVEC;
		static const CFloatVector3 YVEC;
		static const CFloatVector3 ZVEC;

		static const CFloatVector3 Create (float f0, float f1, float f2);
		static const CFloatVector3 Avg (const CFloatVector3& src0, const CFloatVector3& src1);
		static const CFloatVector3 Cross (const CFloatVector3& v0, const CFloatVector3& v1);
		static CFloatVector3& Cross (CFloatVector3& dest, const CFloatVector3& v0, const CFloatVector3& v1);

		static const float Dist (const CFloatVector3& v0, const CFloatVector3& v1);
		static const float Dot (const CFloatVector3& v0, const CFloatVector3& v1);
		static const float Normalize (CFloatVector3& vec);
		static const CFloatVector3 Perp (const CFloatVector3& p0, const CFloatVector3& p1, const CFloatVector3& p2);
		static CFloatVector3& Perp (CFloatVector3& dest, const CFloatVector3& p0, const CFloatVector3& p1, const CFloatVector3& p2);
		static const CFloatVector3 Normal (const CFloatVector3& p0, const CFloatVector3& p1, const CFloatVector3& p2);
		static const CFloatVector3 Reflect (const CFloatVector3& d, const CFloatVector3& n);

		// access op for assignment
		//float& operator[] (size_t i);
		// read-only access op
		//const float operator[] (size_t i) const;

		bool IsZero (void) const;
		void SetZero (void);
		void Set (const float f0, const float f1, const float f2);
		void Set (const float *vec);

		CFloatVector3& Neg (void);
		CFloatVector3& Scale (CFloatVector3& scale);
		const float Mag (void) const;
		const float SqrMag (void) const;

		const CFloatVector3 operator- (void) const;
		const bool operator== (const CFloatVector3& other);
		const bool operator!= (const CFloatVector3& other);
		const CFloatVector3& operator+= (const CFloatVector3& other);
		const CFloatVector3& operator-= (const CFloatVector3& other);
		const CFloatVector3& operator*= (const CFloatVector3& other);
		const CFloatVector3& operator*= (const float s);
		const CFloatVector3& operator/= (const float s);
		const CFloatVector3 operator+ (const CFloatVector3& other) const;
		const CFloatVector3 operator- (const CFloatVector3& other) const;
		CFloatVector3& Assign (const CFloatVector3& other);
		CFloatVector3& Assign (const CFloatVector& other);
		CFloatVector3& Assign (const CFixVector& other);
};

const float operator* (const CFloatVector3& v0, const CFloatVector3& v1);
const CFloatVector3 operator* (const CFloatVector3& v, float s);
const CFloatVector3 operator* (float s, const CFloatVector3& v);
const CFloatVector3 operator/ (const CFloatVector3& v, float s);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

//Angle vector.  Used to store orientations

class __pack__ CAngleVector {
#if 1
	public:
		union {
			struct {
				fixang p, b, h;
			} c;
			fixang a [3];
		} v;
#else
	private:
		 fixang a [3];
#endif
public:
	static const CAngleVector ZERO;
	static const CAngleVector Create (const fixang p, const fixang b, const fixang h) {
		CAngleVector a;
		a.v.c.p = p; a.v.c.b = b; a.v.c.h = h;
		return a;
	}
	// access op for assignment
	//fixang& operator[] (size_t i) { return a [i]; }
	// read-only access op
	//const fixang operator[] (size_t i) const { return a [i]; }

	bool IsZero (void) const { return ! (v.c.p || v.c.h || v.c.b); }
	void SetZero (void) { memset (&v, 0, sizeof (v)); }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// CFloatVector static inlines

inline const CFloatVector CFloatVector::Create (float f0, float f1, float f2, float f3) {
	CFloatVector vec;
	vec.Set (f0, f1, f2, f3);
	return vec;
}

inline const CFloatVector CFloatVector::Avg (const CFloatVector& src0, const CFloatVector& src1) {
	return Create ((src0.v.c.x + src1.v.c.x) / 2,
	               (src0.v.c.y + src1.v.c.y) / 2,
	               (src0.v.c.z + src1.v.c.z) / 2,
	               1);
}

inline CFloatVector& CFloatVector::Cross (CFloatVector& dest, const CFloatVector& v0, const CFloatVector& v1) {
	dest.Set (v0.v.c.y * v1.v.c.z - v0.v.c.z * v1.v.c.y,
	          v0.v.c.z * v1.v.c.x - v0.v.c.x * v1.v.c.z,
	          v0.v.c.x * v1.v.c.y - v0.v.c.y * v1.v.c.x);
	return dest;
}

inline const CFloatVector CFloatVector::Cross (const CFloatVector& v0, const CFloatVector& v1) {
	return Create (v0.v.c.y * v1.v.c.z - v0.v.c.z * v1.v.c.y,
	               v0.v.c.z * v1.v.c.x - v0.v.c.x * v1.v.c.z,
	               v0.v.c.x * v1.v.c.y - v0.v.c.y * v1.v.c.x);
}

inline const float CFloatVector::Dist (const CFloatVector& v0, const CFloatVector& v1) {
	return (v0-v1).Mag ();
}

inline const float CFloatVector::Dot (const CFloatVector& v0, const CFloatVector& v1) {
	return v0.v.c.x * v1.v.c.x + v0.v.c.y * v1.v.c.y + v0.v.c.z * v1.v.c.z;
}

inline const float CFloatVector::Normalize (CFloatVector& vec) {
	float m = vec.Mag ();
	if (m)
		vec /= m;
	return m;
}

inline CFloatVector& CFloatVector::Perp (CFloatVector& dest, const CFloatVector& p0, const CFloatVector& p1, const CFloatVector& p2) {
	return Cross (dest, p1 - p0, p2 - p1);
}

inline const CFloatVector CFloatVector::Perp (const CFloatVector& p0, const CFloatVector& p1, const CFloatVector& p2) {
	return Cross (p1 - p0, p2 - p1);
}

inline const CFloatVector CFloatVector::Normal (const CFloatVector& p0, const CFloatVector& p1, const CFloatVector& p2) {
	CFloatVector v = Perp (p0, p1, p2);
	Normalize (v);
	return v;
}

inline const CFloatVector CFloatVector::Reflect (const CFloatVector& d, const CFloatVector& n) {
	return -2.0f * Dot (d, n) * n + d;
}

// -----------------------------------------------------------------------------
// CFloatVector member inlines

//inline float& CFloatVector::operator[] (size_t i) { return v [i]; }

//inline const float CFloatVector::operator[] (size_t i) const { return v [i]; }

inline bool CFloatVector::IsZero (void) const { return (v.c.x == 0.0f) && (v.c.y == 0.0f) && (v.c.z == 0.0f) && (v.c.w == 0.0f); }

inline void CFloatVector::SetZero (void) { memset (&v, 0, sizeof (v)); }

inline void CFloatVector::Set (const float f0, const float f1, const float f2, const float f3) {
	v.c.x = f0; v.c.y = f1; v.c.z = f2; v.c.w = f3;
}

inline void CFloatVector::Set (const float *vec) {
	v.c.x = vec [0]; v.c.y = vec [1]; v.c.z = vec [2]; v.c.w = 1.0f;
}

inline const float CFloatVector::SqrMag (void) const {
	return v.c.x * v.c.x + v.c.y * v.c.y + v.c.z * v.c.z;
}

inline const float CFloatVector::Mag (void) const {
	return (const float) sqrt (SqrMag ());
}

inline CFloatVector& CFloatVector::Scale (CFloatVector& scale) {
	v.c.x *= scale.v.c.x, v.c.y *= scale.v.c.y, v.c.z *= scale.v.c.z;
	return *this;
	}

inline CFloatVector& CFloatVector::Neg (void) {
	v.c.x = -v.c.x, v.c.y = -v.c.y, v.c.z = -v.c.z;
	return *this;
	}

inline CFloatVector3* CFloatVector::XYZ (void) { return reinterpret_cast<CFloatVector3*> (&v.c.x); }

inline const CFloatVector CFloatVector::operator- (void) const {
	return Create (-v.c.x, -v.c.y, -v.c.z);
}

inline CFloatVector& CFloatVector::Assign (const CFloatVector3& other) {
	v.c.x = other.v.c.x, v.c.y = other.v.c.y, v.c.z = other.v.c.z;
	return *this;
}

inline CFloatVector& CFloatVector::Assign (const CFloatVector& other) {
	v.c.x = other.v.c.x, v.c.y = other.v.c.y, v.c.z = other.v.c.z;
	return *this;
}

inline CFloatVector& CFloatVector::Assign (const CFixVector& other) {
	v.c.x = X2F (other.v.c.x), v.c.y = X2F (other.v.c.y), v.c.z = X2F (other.v.c.z);
	return *this;
}

inline const bool CFloatVector::operator== (const CFloatVector& other) {
	return (v.c.x == other.v.c.x) && (v.c.y == other.v.c.y) && (v.c.z == other.v.c.z);
}

inline const bool CFloatVector::operator!= (const CFloatVector& other) {
	return (v.c.x != other.v.c.x) || (v.c.y != other.v.c.y) || (v.c.z != other.v.c.z);
}

inline const CFloatVector& CFloatVector::operator+= (const CFloatVector& other) {
	v.c.x += other.v.c.x; v.c.y += other.v.c.y; v.c.z += other.v.c.z;
	return *this;
}

inline const CFloatVector& CFloatVector::operator*= (const CFloatVector& other) {
	v.c.x *= other.v.c.x; v.c.y *= other.v.c.y; v.c.z *= other.v.c.z;
	return *this;
}

inline const CFloatVector& CFloatVector::operator+= (const CFixVector& other) {
	v.c.x += X2F (other.v.c.x); v.c.y += X2F (other.v.c.y); v.c.z += X2F (other.v.c.z);
	return *this;
}

inline const CFloatVector& CFloatVector::operator-= (const CFloatVector& other) {
	v.c.x -= other.v.c.x; v.c.y -= other.v.c.y; v.c.z -= other.v.c.z;
	return *this;
}

inline const CFloatVector& CFloatVector::operator-= (const CFixVector& other)
{
v.c.x -= X2F (other.v.c.x);
v.c.y -= X2F (other.v.c.y);
v.c.z -= X2F (other.v.c.z);
return *this;
}

inline const CFloatVector& CFloatVector::operator*= (const float s) {
	v.c.x *= s; v.c.y *= s; v.c.z *= s;
	return *this;
}

inline const CFloatVector& CFloatVector::operator/= (const float s) {
	v.c.x /= s; v.c.y /= s; v.c.z /= s;
	return *this;
}

inline const CFloatVector CFloatVector::operator+ (const CFloatVector& other) const {
	return Create (v.c.x + other.v.c.x, v.c.y + other.v.c.y, v.c.z + other.v.c.z, 1);
}

inline const CFloatVector CFloatVector::operator+ (const CFixVector& other) const {
	return Create (v.c.x + X2F (other.v.c.x), v.c.y + X2F (other.v.c.y), v.c.z + X2F (other.v.c.z), 1);
}

inline const CFloatVector CFloatVector::operator- (const CFloatVector& other) const {
	return Create (v.c.x - other.v.c.x, v.c.y - other.v.c.y, v.c.z - other.v.c.z, 1);
}

inline const CFloatVector CFloatVector::operator- (const CFixVector& other) const {
	return Create (v.c.x - X2F (other.v.c.x), v.c.y - X2F (other.v.c.y), v.c.z - X2F (other.v.c.z), 1);
}

inline const float CFloatVector::DistToPlane (const CFloatVector& n, const CFloatVector& p) const
{
CFloatVector t = *this - p;
return CFloatVector::Dot (t, n);
}

// -----------------------------------------------------------------------------
// CFloatVector-related non-member ops

inline const float operator* (const CFloatVector& v0, const CFloatVector& v1) {
	return CFloatVector::Dot (v0, v1);
}

inline const CFloatVector operator* (const CFloatVector& v, const float s) {
	return CFloatVector::Create (v.v.c.x * s, v.v.c.y * s, v.v.c.z * s, 1);
}

inline const CFloatVector operator* (const float s, const CFloatVector& v) {
	return CFloatVector::Create (v.v.c.x * s, v.v.c.y * s, v.v.c.z * s, 1);
}

inline const CFloatVector operator/ (const CFloatVector& v, const float s) {
	return CFloatVector::Create (v.v.c.x / s, v.v.c.y / s, v.v.c.z / s, 1);
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// CFloatVector3 static inlines

inline const CFloatVector3 CFloatVector3::Create (float f0, float f1, float f2) {
	CFloatVector3 vec;
	vec.Set (f0, f1, f2);
	return vec;
}

inline const CFloatVector3 CFloatVector3::Avg (const CFloatVector3& src0, const CFloatVector3& src1) {
	return Create ((src0.v.c.x + src1.v.c.x) / 2,
	               (src0.v.c.y + src1.v.c.y) / 2,
	               (src0.v.c.z + src1.v.c.z) / 2);
}

inline CFloatVector3& CFloatVector3::Cross (CFloatVector3& dest, const CFloatVector3& v0, const CFloatVector3& v1) {
	dest.Set (v0.v.c.y * v1.v.c.z - v0.v.c.z * v1.v.c.y,
	          v0.v.c.z * v1.v.c.x - v0.v.c.x * v1.v.c.z,
	          v0.v.c.x * v1.v.c.y - v0.v.c.y * v1.v.c.x);
	return dest;
}

inline const CFloatVector3 CFloatVector3::Cross (const CFloatVector3& v0, const CFloatVector3& v1) {
	return Create (v0.v.c.y * v1.v.c.z - v0.v.c.z * v1.v.c.y,
	               v0.v.c.z * v1.v.c.x - v0.v.c.x * v1.v.c.z,
	               v0.v.c.x * v1.v.c.y - v0.v.c.y * v1.v.c.x);
}

inline const float CFloatVector3::Dist (const CFloatVector3& v0, const CFloatVector3& v1) {
	return (v0-v1).Mag ();
}

inline const float CFloatVector3::Dot (const CFloatVector3& v0, const CFloatVector3& v1) {
	return v0.v.c.x * v1.v.c.x + v0.v.c.y * v1.v.c.y + v0.v.c.z * v1.v.c.z;
}

inline const float CFloatVector3::Normalize (CFloatVector3& vec) {
	float m = vec.Mag ();
	if (m)
		vec /= m;
	return m;
}

inline CFloatVector3& CFloatVector3::Perp (CFloatVector3& dest, const CFloatVector3& p0, const CFloatVector3& p1, const CFloatVector3& p2) {
	return Cross (dest, p1 - p0, p2 - p1);
}

inline const CFloatVector3 CFloatVector3::Perp (const CFloatVector3& p0, const CFloatVector3& p1, const CFloatVector3& p2) {
	return Cross (p1 - p0, p2 - p1);
}

inline const CFloatVector3 CFloatVector3::Normal (const CFloatVector3& p0, const CFloatVector3& p1, const CFloatVector3& p2) {
	CFloatVector3 v = Perp (p0, p1, p2);
	Normalize (v);
	return v;
}

inline const CFloatVector3 CFloatVector3::Reflect (const CFloatVector3& d, const CFloatVector3& n) {
	return -2.0f * Dot (d, n) * n + d;
}

// -----------------------------------------------------------------------------
// CFloatVector3 member inlines

//inline float& CFloatVector3::operator[] (size_t i) { return v [i]; }

//inline const float CFloatVector3::operator[] (size_t i) const { return v [i]; }

inline bool CFloatVector3::IsZero (void) const { return ! (v.c.x || v.c.y || v.c.z); }

inline void CFloatVector3::SetZero (void) { memset (&v, 0, sizeof (v)); }

inline void CFloatVector3::Set (const float f0, const float f1, const float f2) {
	v.c.x = f0; v.c.y = f1; v.c.z = f2;
}

inline void CFloatVector3::Set (const float *vec) {
	v.c.x = vec [0]; v.c.y = vec [1]; v.c.z = vec [2];
}

inline CFloatVector3& CFloatVector3::Scale (CFloatVector3& scale) {
	v.c.x *= scale.v.c.x, v.c.y *= scale.v.c.y, v.c.z *= scale.v.c.z;
	return *this;
	}

inline CFloatVector3& CFloatVector3::Neg (void) {
	v.c.x = -v.c.x, v.c.y = -v.c.y, v.c.z = -v.c.z;
	return *this;
	}

inline const float CFloatVector3::SqrMag (void) const {
	return v.c.x * v.c.x + v.c.y * v.c.y + v.c.z * v.c.z;
}

inline const float CFloatVector3::Mag (void) const {
	return (const float) sqrt (SqrMag ());
}

inline CFloatVector3& CFloatVector3::Assign (const CFloatVector3& other) {
	v.c.x = other.v.c.x, v.c.y = other.v.c.y, v.c.z = other.v.c.z;
	return *this;
}

inline CFloatVector3& CFloatVector3::Assign (const CFloatVector& other) {
	v.c.x = other.v.c.x, v.c.y = other.v.c.y, v.c.z = other.v.c.z;
	return *this;
}

inline CFloatVector3& CFloatVector3::Assign (const CFixVector& other) {
	v.c.x = X2F (other.v.c.x), v.c.y = X2F (other.v.c.y), v.c.z = X2F (other.v.c.z);
	return *this;
}

inline const bool CFloatVector3::operator== (const CFloatVector3& other) {
	return v.c.x == other.v.c.x && v.c.y == other.v.c.y && v.c.z == other.v.c.z;
}

inline const bool CFloatVector3::operator!= (const CFloatVector3& other) {
	return v.c.x != other.v.c.x || v.c.y != other.v.c.y || v.c.z != other.v.c.z;
}

inline const CFloatVector3 CFloatVector3::operator- (void) const {
	return Create (-v.c.x, -v.c.y, -v.c.z);
}

inline const CFloatVector3& CFloatVector3::operator+= (const CFloatVector3& other) {
	v.c.x += other.v.c.x; v.c.y += other.v.c.y; v.c.z += other.v.c.z;
	return *this;
}

inline const CFloatVector3& CFloatVector3::operator-= (const CFloatVector3& other) {
	v.c.x -= other.v.c.x; v.c.y -= other.v.c.y; v.c.z -= other.v.c.z;
	return *this;
}

inline const CFloatVector3& CFloatVector3::operator*= (const CFloatVector3& other) {
	v.c.x *= other.v.c.x; v.c.y *= other.v.c.y; v.c.z *= other.v.c.z;
	return *this;
}

inline const CFloatVector3& CFloatVector3::operator*= (const float s) {
	v.c.x *= s; v.c.y *= s; v.c.z *= s;
	return *this;
}

inline const CFloatVector3& CFloatVector3::operator/= (const float s) {
	v.c.x /= s; v.c.y /= s; v.c.z /= s;
	return *this;
}

inline const CFloatVector3 CFloatVector3::operator+ (const CFloatVector3& other) const {
	return Create (v.c.x + other.v.c.x, v.c.y + other.v.c.y, v.c.z + other.v.c.z);
}

inline const CFloatVector3 CFloatVector3::operator- (const CFloatVector3& other) const {
	return Create (v.c.x - other.v.c.x, v.c.y - other.v.c.y, v.c.z - other.v.c.z);
}


// -----------------------------------------------------------------------------
// CFloatVector3-related non-member ops

inline const float operator* (const CFloatVector3& v0, const CFloatVector3& v1) {
	return CFloatVector3::Dot (v0, v1);
}

inline const CFloatVector3 operator* (const CFloatVector3& v, float s) {
	return CFloatVector3::Create (v.v.c.x * s, v.v.c.y * s, v.v.c.z * s);
}

inline const CFloatVector3 operator* (float s, const CFloatVector3& v) {
	return CFloatVector3::Create (v.v.c.x * s, v.v.c.y * s, v.v.c.z * s);
}

inline const CFloatVector3 operator/ (const CFloatVector3& v, float s) {
	return CFloatVector3::Create (v.v.c.x / s, v.v.c.y / s, v.v.c.z / s);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// CFixVector static inlines

inline const CFixVector CFixVector::Create (fix f0, fix f1, fix f2) {
	CFixVector vec;
	vec.Set (f0, f1, f2);
	return vec;
}

inline const CFixVector CFixVector::Avg (const CFixVector& src0, const CFixVector& src1) {
	return Create ((src0.v.c.x + src1.v.c.x) / 2,
	               (src0.v.c.y + src1.v.c.y) / 2,
	               (src0.v.c.z + src1.v.c.z) / 2);
}

inline const CFixVector CFixVector::Avg (CFixVector& src0, CFixVector& src1, CFixVector& src2, CFixVector& src3) {
	return Create ((src0.v.c.x + src1.v.c.x + src2.v.c.x + src3.v.c.x) / 4,
					   (src0.v.c.y + src1.v.c.y + src2.v.c.y + src3.v.c.y) / 4,
						(src0.v.c.z + src1.v.c.z + src2.v.c.z + src3.v.c.z) / 4);

}

//computes the delta angle between two vectors.
//vectors need not be normalized. if they are, call CFixVector::DeltaAngleNorm ()
//the forward vector (third parameter) can be NULL, in which case the absolute
//value of the angle in returned.  Otherwise the angle around that vector is
//returned.
inline const fixang CFixVector::DeltaAngle (const CFixVector& v0, const CFixVector& v1, CFixVector *fVec) {
	CFixVector t0 = v0, t1 = v1;

CFixVector::Normalize (t0);
CFixVector::Normalize (t1);
return DeltaAngleNorm (t0, t1, fVec);
}

//computes the delta angle between two normalized vectors.
inline const fixang CFixVector::DeltaAngleNorm (const CFixVector& v0, const CFixVector& v1, CFixVector *fVec) {
	fixang a = FixACos (CFixVector::Dot (v0, v1));
	
if (fVec) {
	CFixVector t = CFixVector::Cross (v0, v1);
	if (CFixVector::Dot (t, *fVec) < 0)
		a = -a;
	}
return a;
}

inline const fix CFixVector::Dist (const CFixVector& vec0, const CFixVector& vec1) {
	return (vec0-vec1).Mag ();
}

inline const fix CFixVector::Dot (const CFixVector& v0, const CFixVector& v1) {
	return fix ((double (v0.v.c.x) * double (v1.v.c.x) + double (v0.v.c.y) * double (v1.v.c.y) + double (v0.v.c.z) * double (v1.v.c.z)) / 65536.0);
}

inline const fix CFixVector::Dot (const fix x, const fix y, const fix z, const CFixVector& v) {
	return fix ((double (x) * double (v.v.c.x) + double (y) * double (v.v.c.y) + double (z) * double (v.v.c.z)) / 65536.0);
}

inline const fix CFixVector::Normalize (CFixVector& v) {
fix m = v.Mag ();
if (!m)
	v.v.c.x = v.v.c.y = v.v.c.z = 0;
else {
	v.v.c.x = FixDiv (v.v.c.x, m);
	v.v.c.y = FixDiv (v.v.c.y, m);
	v.v.c.z = FixDiv (v.v.c.z, m);
	}
return m;
}

inline CFixVector& CFixVector::Perp (CFixVector& dest, const CFixVector& p0, const CFixVector& p1, const CFixVector& p2) {
	CFixVector t0 = p1 - p0, t1 = p2 - p1;
#if 0
	Normalize (t0);
	Normalize (t1);
#else
	t0.Check ();
	t1.Check ();
#endif
	return Cross (dest, t0, t1);
}

inline const CFixVector CFixVector::Perp (const CFixVector& p0, const CFixVector& p1, const CFixVector& p2) {
	CFixVector t0 = p1 - p0, t1 = p2 - p1;
#if 0
	Normalize (t0);
	Normalize (t1);
#else
	t0.Check ();
	t1.Check ();
#endif
	return Cross (t0, t1);
}

inline const CFixVector CFixVector::Normal (const CFixVector& p0, const CFixVector& p1, const CFixVector& p2) {
	CFixVector v = Perp (p0, p1, p2);
	Normalize (v);
	return v;
}

inline const CFixVector CFixVector::Reflect (const CFixVector& d, const CFixVector& n) {
	fix k = Dot (d, n) * 2;
	CFixVector r = n;
	r *= k;
	r -= d;
	r.Neg ();
	return r;
}

//return the normalized direction vector between two points
//dest = normalized (end - start).  Returns Mag of direction vector
//NOTE: the order of the parameters m.matches the vector subtraction
inline const fix CFixVector::NormalizedDir (CFixVector& dest, const CFixVector& end, const CFixVector& start) {
	dest = end - start;
	return CFixVector::Normalize (dest);
}

// -----------------------------------------------------------------------------
// CFixVector member inlines

//inline fix& CFixVector::operator[] (size_t i) { return v [i]; }

//inline const fix CFixVector::operator[] (size_t i) const { return v [i]; }

inline CFixVector& CFixVector::Assign (const CFloatVector3& other)
{
v.c.x = F2X (other.v.c.x), v.c.y = F2X (other.v.c.y), v.c.z = F2X (other.v.c.z);
return *this;
}

inline CFixVector& CFixVector::Assign (const CFloatVector& other)
{
v.c.x = F2X (other.v.c.x), v.c.y = F2X (other.v.c.y), v.c.z = F2X (other.v.c.z);
return *this;
}

inline CFixVector& CFixVector::Assign (const CFixVector& other)
{
v.c.x = other.v.c.x, v.c.y = other.v.c.y, v.c.z = other.v.c.z;
return *this;
}

inline bool CFixVector::operator== (const CFixVector& other) const
{
return (v.c.x == other.v.c.x) && (v.c.y == other.v.c.y) && (v.c.z == other.v.c.z);
}

inline const CFixVector& CFixVector::Set (fix x, fix y, fix z)
{
v.c.x = x; v.c.y = y; v.c.z = z; 
return *this;
}

inline void CFixVector::Set (const fix *vec)
{
v.c.x = vec [0]; v.c.y = vec [1]; v.c.z = vec [2];
}

inline bool CFixVector::IsZero (void) const
{
return !(v.c.x || v.c.y || v.c.z);
}

inline void CFixVector::SetZero (void)
{
memset (&v, 0, sizeof (v));
}

inline const int CFixVector::Sign (void) const
{
return (v.c.x * v.c.y * v.c.z < 0) ? -1 : 1;
}

inline fix CFixVector::SqrMag (void) const
{
return FixMul (v.c.x, v.c.x) + FixMul (v.c.y, v.c.y) + FixMul (v.c.z, v.c.z);
}

inline float CFixVector::Sqr (float f) const { return f * f; }

inline CFixVector& CFixVector::Scale (CFixVector& scale)
{
v.c.x = FixMul (v.c.x, scale.v.c.x);
v.c.y = FixMul (v.c.y, scale.v.c.y);
v.c.z = FixMul (v.c.z, scale.v.c.z);
return *this;
}

inline CFixVector& CFixVector::Neg (void)
{
v.c.x = -v.c.x, v.c.y = -v.c.y, v.c.z = -v.c.z;
return *this;
}

inline const CFixVector CFixVector::operator- (void) const
{
return Create (-v.c.x, -v.c.y, -v.c.z);
}

inline const bool CFixVector::operator== (const CFixVector& vec)
{
return (v.c.x == vec.v.c.x) && (v.c.y == vec.v.c.y) && (v.c.z == vec.v.c.z);
}

inline const bool CFixVector::operator!= (const CFixVector& vec)
{
return (v.c.x != vec.v.c.x) || (v.c.y != vec.v.c.y) || (v.c.z != vec.v.c.z);
}

inline const CFixVector& CFixVector::operator+= (const CFixVector& other)
{
v.c.x += other.v.c.x;
v.c.y += other.v.c.y;
v.c.z += other.v.c.z;
return *this;
}

inline const CFixVector& CFixVector::operator+= (const CFloatVector& other)
{
v.c.x += F2X (other.v.c.x);
v.c.y += F2X (other.v.c.y);
v.c.z += F2X (other.v.c.z);
return *this;
}

inline const CFixVector& CFixVector::operator-= (const CFixVector& other) {
	v.c.x -= other.v.c.x;
	v.c.y -= other.v.c.y;
	v.c.z -= other.v.c.z;
	return *this;
}

inline const CFixVector& CFixVector::operator-= (const CFloatVector& other)
{
v.c.x -= F2X (other.v.c.x);
v.c.y -= F2X (other.v.c.y);
v.c.z -= F2X (other.v.c.z);
return *this;
}

inline const CFixVector& CFixVector::operator*= (const fix s)
{
v.c.x = FixMul (v.c.x, s);
v.c.y = FixMul (v.c.y, s);
v.c.z = FixMul (v.c.z, s);
return *this;
}

inline const CFixVector& CFixVector::operator*= (const CFixVector& other)
{
v.c.x = FixMul (v.c.x, other.v.c.x);
v.c.y = FixMul (v.c.y, other.v.c.y);
v.c.z = FixMul (v.c.z, other.v.c.z);
return *this;
}

inline const CFixVector& CFixVector::operator/= (const fix s)
{
v.c.x = FixDiv (v.c.x, s);
v.c.y = FixDiv (v.c.y, s);
v.c.z = FixDiv (v.c.z, s);
return *this;
}

inline const CFixVector CFixVector::operator+ (const CFixVector& other) const {
	return Create (v.c.x + other.v.c.x, v.c.y + other.v.c.y, v.c.z + other.v.c.z);
}

inline const CFixVector CFixVector::operator+ (const CFloatVector& other) const
{
return Create (v.c.x + F2X (other.v.c.x), v.c.y + F2X (other.v.c.y), v.c.z + F2X (other.v.c.z));
}

inline const CFixVector CFixVector::operator- (const CFixVector& other) const
{
return Create (v.c.x - other.v.c.x, v.c.y - other.v.c.y, v.c.z - other.v.c.z);
}

inline const CFixVector CFixVector::operator- (const CFloatVector& other) const
{
return Create (v.c.x - F2X (other.v.c.x), v.c.y - F2X (other.v.c.y), v.c.z - F2X (other.v.c.z));
}


// compute intersection of a line through a point a, with the line being orthogonal relative
// to the plane given by the Normal n and a point p lieing in the plane, and store it in i.
inline const CFixVector CFixVector::PlaneProjection (const CFixVector& n, const CFixVector& p) const
{
	CFixVector i;

double l = double (-CFixVector::Dot (n, p)) / double (CFixVector::Dot (n, *this));
i.v.c.x = fix (l * double (v.c.x));
i.v.c.y = fix (l * double (v.c.y));
i.v.c.z = fix (l * double (v.c.z));
return i;
}

//compute the distance from a point to a plane.  takes the normalized Normal
//of the plane (ebx), a point on the plane (edi), and the point to check (esi).
//returns distance in eax
//distance is signed, so Negative Dist is on the back of the plane
inline const fix CFixVector::DistToPlane (const CFixVector& n, const CFixVector& p) const
{
CFixVector t = *this - p;
return CFixVector::Dot (t, n);
}

//extract heading and pitch from a vector, assuming bank==0
inline const CAngleVector CFixVector::ToAnglesVecNorm (void) const
{
	CAngleVector a;

a.v.c.b = 0;		//always zero bank
a.v.c.p = FixASin (-v.c.y);
a.v.c.h = (v.c.x || v.c.z) ? FixAtan2 (v.c.z, v.c.x) : 0;
return a;
}

//extract heading and pitch from a vector, assuming bank==0
inline const CAngleVector CFixVector::ToAnglesVec (void) const
{
	CFixVector t = *this;

CFixVector::Normalize (t);
return t.ToAnglesVecNorm ();
}


// -----------------------------------------------------------------------------
// CFixVector-related non-member ops

inline const fix operator* (const CFixVector& v0, const CFixVector& v1) {
	return CFixVector::Dot (v0, v1);
}

inline const CFixVector operator* (const CFixVector& v, const fix s) {
	return CFixVector::Create (FixMul (v.v.c.x, s), FixMul (v.v.c.y, s), FixMul (v.v.c.z, s));
}

inline const CFixVector operator* (const fix s, const CFixVector& v) {
	return CFixVector::Create (FixMul (v.v.c.x, s), FixMul (v.v.c.y, s), FixMul (v.v.c.z, s));
}

inline const CFixVector operator/ (const CFixVector& v, const fix d) {
	return CFixVector::Create (FixDiv (v.v.c.x, d), FixDiv (v.v.c.y, d), FixDiv (v.v.c.z, d));
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/**
 * \class __pack__ CFixMatrix
 *
 * A 3x3 rotation m.matrix.  Sorry about the numbering starting with one. Ordering
 * is across then down, so <m1,m2,m3> is the first row.
 */
typedef union tFixMatrixData {
	struct {
		CFixVector	r, u, f;
	} v;
	CFixVector	m [3];
	fix			a [9];
} __pack__ tFixMatrixData;


class __pack__ CFixMatrix {
	friend class CFloatMatrix;
#if 1
	public:
		tFixMatrixData	m;
#else
	private:
		tFixMatrixData	m;
#endif
	public:
		static const CFixMatrix IDENTITY;
		static const CFixMatrix Create (const CFixVector& r, const CFixVector& u, const CFixVector& f);
		static const CFixMatrix Create (fix sinp, fix cosp, fix sinb, fix cosb, fix sinh, fix cosh);
		//computes a m.matrix from a Set of three angles.  returns ptr to m.matrix
		static const CFixMatrix Create (const CAngleVector& a);
		//computes a m.matrix from a forward vector and an angle
		static const CFixMatrix Create (CFixVector *v, fixang a);
		static const CFixMatrix CreateF (const CFixVector& fVec);
		static const CFixMatrix CreateFU (const CFixVector& fVec, const CFixVector& uVec);
		static const CFixMatrix CreateFR (const CFixVector& fVec, const CFixVector& rVec);

		static CFixMatrix& Invert (CFixMatrix& m);
		static CFixMatrix& Transpose (CFixMatrix& m);
		static CFixMatrix& Transpose (CFixMatrix& dest, CFixMatrix& source);
		static CFloatMatrix& Transpose (CFloatMatrix& dest, CFixMatrix& src);

		fix& operator[] (size_t i);
		const fix operator[] (size_t i) const;

		CFixMatrix Mul (const CFixMatrix& m);
		CFixMatrix& Scale (CFixVector& scale);
		CFixVector operator* (const CFixVector& v);
		CFixMatrix operator* (const CFixMatrix& m);

		const fix Det (void);
		const CFixMatrix Inverse (void);
		const CFixMatrix Transpose (void);

		//make sure m.matrix is orthogonal
		void CheckAndFix (void);

		//extract angles from a m.matrix
		const CAngleVector ExtractAnglesVec (void) const;

		const CFixMatrix& Assign (CFixMatrix& other);
		const CFixMatrix& Assign (CFloatMatrix& other);

		inline const CFixVector Mat (size_t i) const { return m.m [i]; }
		inline const CFixVector R (void) const { return m.v.r; }
		inline const CFixVector U (void) const { return m.v.u; }
		inline const CFixVector F (void) const { return m.v.f; }

		inline CFixVector& Mat (size_t i) { return m.m [i]; }
		inline fix* Vec (void) { return m.a; }
		inline CFixVector& R (void) { return m.v.r; }
		inline CFixVector& U (void) { return m.v.u; }
		inline CFixVector& F (void) { return m.v.f; }

	private:
		static inline void Swap (fix& l, fix& r) {
			fix t = l;
			l = r;
			r = t;
			}
};



// -----------------------------------------------------------------------------
// CFixMatrix static inlines

inline const CFixMatrix CFixMatrix::Create (const CFixVector& r, const CFixVector& u, const CFixVector& f)
{
	CFixMatrix m;

m.m.v.r = r;
m.m.v.u = u;
m.m.v.f = f;
return m;
}

inline const CFixMatrix CFixMatrix::Create (fix sinp, fix cosp, fix sinb, fix cosb, fix sinh, fix cosh)
{
	CFixMatrix m;
	fix sbsh, cbch, cbsh, sbch;

sbsh = FixMul (sinb, sinh);
cbch = FixMul (cosb, cosh);
cbsh = FixMul (cosb, sinh);
sbch = FixMul (sinb, cosh);
m.m.v.r.v.c.x = cbch + FixMul (sinp, sbsh);		//m1
m.m.v.u.v.c.z = sbsh + FixMul (sinp, cbch);		//m8
m.m.v.u.v.c.x = FixMul (sinp, cbsh) - sbch;		//m2
m.m.v.r.v.c.z = FixMul (sinp, sbch) - cbsh;		//m7
m.m.v.f.v.c.x = FixMul (sinh, cosp);			//m3
m.m.v.r.v.c.y = FixMul (sinb, cosp);			//m4
m.m.v.u.v.c.y = FixMul (cosb, cosp);			//m5
m.m.v.f.v.c.z = FixMul (cosh, cosp);			//m9
m.m.v.f.v.c.y = -sinp;							//m6
return m;
}

//computes a m.matrix from a Set of three angles.  returns ptr to m.matrix
inline const CFixMatrix CFixMatrix::Create (const CAngleVector& a)
{
	fix sinp, cosp, sinb, cosb, sinh, cosh;

FixSinCos (a.v.c.p, &sinp, &cosp);
FixSinCos (a.v.c.b, &sinb, &cosb);
FixSinCos (a.v.c.h, &sinh, &cosh);
return Create (sinp, cosp, sinb, cosb, sinh, cosh);
}

//computes a m.matrix from a forward vector and an angle
inline const CFixMatrix CFixMatrix::Create (CFixVector *v, fixang a)
{
	fix sinb, cosb, sinp, cosp;

FixSinCos (a, &sinb, &cosb);
sinp = - v->v.c.x;
cosp = FixSqrt (I2X (1) - FixMul (sinp, sinp));
return Create (sinp, cosp, sinb, cosb, FixDiv (v->v.c.x, cosp), FixDiv (v->v.c.z, cosp));
}


inline CFixMatrix& CFixMatrix::Invert (CFixMatrix& m)
{
	// TODO implement?
	return m;
}

inline CFixMatrix& CFixMatrix::Transpose (CFixMatrix& m)
{
Swap (m.m.a [1], m.m.a [3]);
Swap (m.m.a [2], m.m.a [6]);
Swap (m.m.a [5], m.m.a [7]);
return m;
}

// -----------------------------------------------------------------------------
// CFixMatrix member ops

inline fix& CFixMatrix::operator[] (size_t i)
{
return m.a [i];
}

inline const fix CFixMatrix::operator[] (size_t i) const
{
return m.a [i];
}

inline CFixVector CFixMatrix::operator* (const CFixVector& v)
{
return CFixVector::Create (CFixVector::Dot (v, m.v.r),
									CFixVector::Dot (v, m.v.u),
									CFixVector::Dot (v, m.v.f));
}

inline CFixMatrix CFixMatrix::operator* (const CFixMatrix& other) { return Mul (other); }

inline CFixMatrix& CFixMatrix::Scale (CFixVector& scale)
{
m.v.r *= scale.v.c.x;
m.v.u *= scale.v.c.y;
m.v.f *= scale.v.c.z;
return *this;
};

const inline CFixMatrix CFixMatrix::Transpose (void)
{
CFixMatrix dest;
Transpose (dest, *this);
return dest;
}

//make sure this m.matrix is orthogonal
inline void CFixMatrix::CheckAndFix (void)
{
*this = CreateFU (m.v.f, m.v.u);
}


/**
 * \class __pack__ CFloatMatrix
 *
 * A 4x4 floating point transformation m.matrix
 */

typedef union tFloatMatrixData {
	struct {
		CFloatVector	r, u, f, h;
	} v;
	CFloatVector	m [4];
	float				a [16];
} __pack__ tFloatMatrixData;

class __pack__ CFloatMatrix {
	friend class CFixMatrix;

#if 1
	public:
		tFloatMatrixData	m;
#else
	private:
		tFloatMatrixData	m;
#endif
	public:
		static const CFloatMatrix IDENTITY;
		static const CFloatMatrix Create (const CFloatVector& r, const CFloatVector& u, const CFloatVector& f, const CFloatVector& w);
		static const CFloatMatrix Create (float sinp, float cosp, float sinb, float cosb, float sinh, float cosh);
		static const CFloatMatrix CreateFU (const CFloatVector& fVec, const CFloatVector& uVec);
		static const CFloatMatrix CreateFR (const CFloatVector& fVec, const CFloatVector& rVec);

		static CFloatMatrix& Invert (CFloatMatrix& m);
		static CFloatMatrix& Transpose (CFloatMatrix& m);
		static CFloatMatrix& Transpose (CFloatMatrix& dest, CFloatMatrix& source);

		float& operator[] (size_t i);

		const CFloatVector operator* (const CFloatVector& v);
		const CFloatVector3 operator* (const CFloatVector3& v);
		const CFloatMatrix operator* (CFloatMatrix& m);

		const CFloatMatrix Mul (CFloatMatrix& other);
		const CFloatMatrix& Scale (CFloatVector& scale);
		const float Det (void);
		const CFloatMatrix Inverse (void);
		const CFloatMatrix Transpose (void);

		void CheckAndFix (void);

		const CFloatMatrix& Assign (CFixMatrix& other);
		const CFloatMatrix& Assign (CFloatMatrix& other);

		static float* Transpose (float* dest, const CFloatMatrix& src);

		inline const CFloatVector Mat (size_t i) const { return m.m [i]; }
		inline const CFloatVector R (void) const { return m.v.r; }
		inline const CFloatVector U (void) const { return m.v.u; }
		inline const CFloatVector F (void) const { return m.v.f; }

		inline float* Vec (void) { return m.a; }
		inline CFloatVector& R (void) { return m.v.r; }
		inline CFloatVector& U (void) { return m.v.u; }
		inline CFloatVector& F (void) { return m.v.f; }
		inline CFloatVector& H (void) { return m.v.h; }

	private:
		static inline void Swap (float& l, float& r) {
			float t = l;
			l = r;
			r = t;
			}
};


// -----------------------------------------------------------------------------
// CFloatMatrix static inlines

inline CFloatMatrix& CFloatMatrix::Invert (CFloatMatrix& m) {
	//TODO: implement?
	return m;
}

inline CFloatMatrix& CFloatMatrix::Transpose (CFloatMatrix& m) {
	Swap (m.m.a [1], m.m.a [3]);
	Swap (m.m.a [2], m.m.a [6]);
	Swap (m.m.a [5], m.m.a [7]);
	return m;
}

// -----------------------------------------------------------------------------
// CFloatMatrix member ops

inline float& CFloatMatrix::operator[] (size_t i) {
	return m.a [i];
}

inline const CFloatVector CFloatMatrix::operator* (const CFloatVector& v)
{
return CFloatVector::Create (CFloatVector::Dot (v, m.v.r),
									  CFloatVector::Dot (v, m.v.u),
									  CFloatVector::Dot (v, m.v.f));
}

inline const CFloatVector3 CFloatMatrix::operator* (const CFloatVector3& v)
{
return CFloatVector3::Create (CFloatVector3::Dot (v, *m.v.r.XYZ ()),
										CFloatVector3::Dot (v, *m.v.u.XYZ ()),
										CFloatVector3::Dot (v, *m.v.f.XYZ ()));
}

inline const CFloatMatrix CFloatMatrix::Transpose (void)
{
CFloatMatrix dest;
Transpose (dest, *this);
return dest;
}

inline const CFloatMatrix CFloatMatrix::operator* (CFloatMatrix& other) { return Mul (other); }

inline const CFloatMatrix& CFloatMatrix::Scale (CFloatVector& scale)
{
m.v.r *= scale.v.c.x;
m.v.u *= scale.v.c.y;
m.v.f *= scale.v.c.z;
return *this;
};

// -----------------------------------------------------------------------------
// misc conversion member ops

inline const CFloatMatrix& CFloatMatrix::Assign (CFloatMatrix& other)
{
*this = other;
return *this;
}

inline const CFloatMatrix& CFloatMatrix::Assign (CFixMatrix& other)
{
*this = CFloatMatrix::IDENTITY;
m.v.r.Assign (other.m.v.r);
m.v.u.Assign (other.m.v.u);
m.v.f.Assign (other.m.v.f);
return *this;
}

inline const CFixMatrix& CFixMatrix::Assign (CFixMatrix& other)
{
*this = other;
return *this;
}

inline const CFixMatrix& CFixMatrix::Assign (CFloatMatrix& other)
{
m.v.r.Assign (other.m.v.r);
m.v.u.Assign (other.m.v.u);
m.v.f.Assign (other.m.v.f);
return *this;
}

//make sure this m.matrix is orthogonal
inline void CFloatMatrix::CheckAndFix (void) {
	*this = CreateFU (m.v.f, m.v.u);
}

//} // VecMat

// -----------------------------------------------------------------------------
// misc remaining C-style funcs

const int VmPointLineIntersection (CFixVector& hitP, const CFixVector& p1, const CFixVector& p2, const CFixVector& p3, int bClampToFarthest);
//const int VmPointLineIntersection (CFixVector& hitP, const CFixVector& p1, const CFixVector& p2, const CFixVector& p3, const CFixVector& vPos, int bClampToFarthest);
const fix VmLinePointDist (const CFixVector& a, const CFixVector& b, const CFixVector& p);
const int VmPointLineIntersection (CFloatVector& hitP, const CFloatVector& p1, const CFloatVector& p2, const CFloatVector& p3, const CFloatVector& vPos, int bClamp);
const int VmPointLineIntersection (CFloatVector& hitP, const CFloatVector& p1, const CFloatVector& p2, const CFloatVector& p3, int bClamp);
const int VmPointLineIntersection (CFloatVector3& hitP, const CFloatVector3& p1, const CFloatVector3& p2, const CFloatVector3& p3, CFloatVector3 *vPos, int bClamp);
const float VmLinePointDist (const CFloatVector& a, const CFloatVector& b, const CFloatVector& p, int bClamp);
const float VmLinePointDist (const CFloatVector3& a, const CFloatVector3& b, const CFloatVector3& p, int bClamp);
const float VmLineLineIntersection (const CFloatVector3& v1, const CFloatVector3& v2, const CFloatVector3& v3, const CFloatVector3& v4, CFloatVector3& va, CFloatVector3& vb);
const float VmLineLineIntersection (const CFloatVector& v1, const CFloatVector& v2, const CFloatVector& v3, const CFloatVector& v4, CFloatVector& va, CFloatVector& vb);

CFloatVector* VmsReflect (CFloatVector *vReflect, CFloatVector *vLight, CFloatVector *vNormal);

float TriangleSize (const CFixVector& p0, const CFixVector& p1, const CFixVector& p2);

// ------------------------------------------------------------------------

#endif //_VECMAT_H
