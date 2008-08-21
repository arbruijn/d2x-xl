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
#include <string>

using namespace std;

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


class vmsVector;


/**
 * \class fVector3
 * A 3 element floating point vector class
 */
class fVector3 {
	public:
		static const fVector3 ZERO;

		static const fVector3 Create(float f0, float f1, float f2);
		static const fVector3 avg(const fVector3& src0, const fVector3& src1);
		static const fVector3 cross(const fVector3& v0, const fVector3& v1);
		static const float dist(const fVector3& v0, const fVector3& v1);
		static const float dot(const fVector3& v0, const fVector3& v1);
		static const float normalize(fVector3& vec);
		static const fVector3 perp(const fVector3& p0, const fVector3& p1, const fVector3& p2);
		static const fVector3 normal(const fVector3& p0, const fVector3& p1, const fVector3& p2);
		static const fVector3 reflect(const fVector3& d, const fVector3& n);

		// access op for assignment
		float& operator[](size_t idx);
		// read-only access op
		const float operator[](size_t idx) const;

		bool isZero() const;
		void setZero();
		void set(const float f0, const float f1, const float f2);

		fVector3& neg();
		const float mag() const;
		const float sqrmag() const;

		const fVector3 operator-() const;
		const fVector3& operator+=(const fVector3& vec);
		const fVector3& operator-=(const fVector3& vec);
		const fVector3& operator*=(const float s);
		const fVector3& operator/=(const float s);
		const fVector3 operator+(const fVector3& vec) const;
		const fVector3 operator-(const fVector3& vec) const;

		const vmsVector toFix() const;

	private:
		float v[3];
};

const fVector3 operator*(const fVector3& v, float s);
const fVector3 operator*(float s, const fVector3& v);
const fVector3 operator/(const fVector3& v, float s);



// -----------------------------------------------------------------------------
// fVector3 static inlines

inline const fVector3 fVector3::Create(float f0, float f1, float f2) {
	fVector3 vec;
	vec.set(f0, f1, f2);
	return vec;
}

inline const fVector3 fVector3::avg(const fVector3& src0, const fVector3& src1) {
	return Create((src0[X] + src1[X]) / 2,
	              (src0[Y] + src1[Y]) / 2,
	              (src0[Z] + src1[Z]) / 2);
}

inline const fVector3 fVector3::cross(const fVector3& v0, const fVector3& v1) {
	return Create(v0[Y]*v1[Z] - v0[Z]*v1[Y],
	              v0[Z]*v1[X] - v0[X]*v1[Z],
	              v0[X]*v1[Y] - v0[Y]*v1[X]);
}

inline const float fVector3::dist(const fVector3& v0, const fVector3& v1) {
	return (v0-v1).mag();
}

inline const float fVector3::dot(const fVector3& v0, const fVector3& v1) {
	return v0[X]*v1[X] + v0[Y]*v1[Y] + v0[Z]*v1[Z];
}

inline const float fVector3::normalize(fVector3& vec) {
	float m = vec.mag();
	if(m)
		vec /= m;
	return m;
}

inline const fVector3 fVector3::perp(const fVector3& p0, const fVector3& p1, const fVector3& p2) {
	return cross(p1 - p0, p2 - p1);
}

inline const fVector3 fVector3::normal(const fVector3& p0, const fVector3& p1, const fVector3& p2) {
	fVector3 v = 2.0f*perp(p0, p1, p2);
	normalize(v);
	return v;
}

inline const fVector3 fVector3::reflect(const fVector3& d, const fVector3& n) {
	return -2.0f * dot(d, n) * n + d;
}


// -----------------------------------------------------------------------------
// fVector3 member inlines

inline float& fVector3::operator[](size_t idx) { return v[idx]; }

inline const float fVector3::operator[](size_t idx) const { return v[idx]; }

inline bool fVector3::isZero() const { return !(v[0] || v[1] || v[2]); }

inline void fVector3::setZero() { memset(v, 0, 3*sizeof(float)); }

inline void fVector3::set(const float f0, const float f1, const float f2) {
	v[X] = f0; v[Y] = f1; v[Z] = f2;
}

inline fVector3& fVector3::neg() { for(size_t i=0; i<3; i++) v[i] *= -1.0f; return *this; }

inline const float fVector3::sqrmag() const {
	return v[X]*v[X] + v[Y]*v[Y] + v[Z]*v[Z];
}

inline const float fVector3::mag() const {
	return sqrt(sqrmag());
}

inline const fVector3 fVector3::operator-() const {
	return Create(-v[X], -v[Y], -v[Z]);
}

inline const fVector3& fVector3::operator+=(const fVector3& vec) {
	v[0] += vec[0]; v[1] += vec[1]; v[2] += vec[2];
	return *this;
}

inline const fVector3& fVector3::operator-=(const fVector3& vec) {
	v[0] -= vec[0]; v[1] -= vec[1]; v[2] -= vec[2];
	return *this;
}

inline const fVector3& fVector3::operator*=(const float s) {
	v[0] *= s; v[1] *= s; v[2] *= s;
	return *this;
}

inline const fVector3& fVector3::operator/=(const float s) {
	v[0] /= s; v[1] /= s; v[2] /= s;
	return *this;
}

inline const fVector3 fVector3::operator+(const fVector3& vec) const {
	return Create(v[0]+vec[0], v[1]+vec[1], v[2]+vec[2]);
}

inline const fVector3 fVector3::operator-(const fVector3& vec) const {
	return Create(v[0]-vec[0], v[1]-vec[1], v[2]-vec[2]);
}


// -----------------------------------------------------------------------------
// fVector3-related non-member ops

inline const fVector3 operator*(const fVector3& v, float s) {
	return fVector3::Create(v[X]*s, v[Y]*s, v[Z]*s);
}

inline const fVector3 operator*(float s, const fVector3& v) {
	return fVector3::Create(v[X]*s, v[Y]*s, v[Z]*s);
}

inline const fVector3 operator/(const fVector3& v, float s) {
	return fVector3::Create(v[X]/s, v[Y]/s, v[Z]/s);
}



/**
 * \class fVector
 * A 4 element floating point vector class
 */
class fVector {
	public:
		static const fVector ZERO;
		static const fVector ZERO4;

		static const fVector Create(float f0, float f1, float f2, float f3=1.0f);

		static const fVector avg(const fVector& src0, const fVector& src1);
		static const fVector cross(const fVector& v0, const fVector& v1);

		static const float dist(const fVector& v0, const fVector& v1);
		static const float dot(const fVector& v0, const fVector& v1);
		static const float normalize(fVector& vec);
		static const fVector perp(const fVector& p0, const fVector& p1, const fVector& p2);
		static const fVector normal(const fVector& p0, const fVector& p1, const fVector& p2);
		static const fVector reflect(const fVector& d, const fVector& n);

		// access op for assignment
		float& operator[](size_t idx);
		// read-only access op
		const float operator[](size_t idx) const;

		bool isZero() const;

		void setZero();
		void set(const float f0, const float f1, const float f2, const float f3=1.0f);
		const float sqrmag() const;
		const float mag() const;
		fVector& neg();
		fVector3* v3();

		const fVector operator-() const;
		const fVector& operator+=(const fVector& vec);
		const fVector& operator-=(const fVector& vec);
		const fVector& operator*=(const float s);
		const fVector& operator/=(const float s);
		const fVector  operator+(const fVector& vec) const;
		const fVector  operator-(const fVector& vec) const;

		const vmsVector toFix() const;

	private:
		float v[4];
};

const fVector operator*(const fVector& v, const float s);
const fVector operator*(const float s, const fVector& v);
const fVector operator/(const fVector& v, const float s);


// -----------------------------------------------------------------------------
// fVector static inlines

inline const fVector fVector::Create(float f0, float f1, float f2, float f3) {
	fVector vec;
	vec.set(f0, f1, f2, f3);
	return vec;
}

inline const fVector fVector::avg(const fVector& src0, const fVector& src1) {
	return Create((src0[X] + src1[X]) / 2,
	              (src0[Y] + src1[Y]) / 2,
	              (src0[Z] + src1[Z]) / 2,
	              1);
}

inline const fVector fVector::cross(const fVector& v0, const fVector& v1) {
	return Create(v0[Y]*v1[Z] - v0[Z]*v1[Y],
	              v0[Z]*v1[X] - v0[X]*v1[Z],
	              v0[X]*v1[Y] - v0[Y]*v1[X]);
}

inline const float fVector::dist(const fVector& v0, const fVector& v1) {
	return (v0-v1).mag();
}

inline const float fVector::dot(const fVector& v0, const fVector& v1) {
	return v0[X]*v1[X] + v0[Y]*v1[Y] + v0[Z]*v1[Z];
}

inline const float fVector::normalize(fVector& vec) {
	float m = vec.mag();
	if(m)
		vec /= m;
	return m;
}

inline const fVector fVector::perp(const fVector& p0, const fVector& p1, const fVector& p2) {
	return cross(p1 - p0, p2 - p1);
}

inline const fVector fVector::normal(const fVector& p0, const fVector& p1, const fVector& p2) {
	fVector v = 2.0f*perp(p0, p1, p2);
	normalize(v);
	return v;
}

inline const fVector fVector::reflect(const fVector& d, const fVector& n) {
	return -2.0f * dot(d, n) * n + d;

}


// -----------------------------------------------------------------------------
// fVector member inlines

inline float& fVector::operator[](size_t idx) { return v[idx]; }

inline const float fVector::operator[](size_t idx) const { return v[idx]; }

inline bool fVector::isZero() const { return !(v[X] || v[Y] || v[Z] || v[W]); }

inline void fVector::setZero() { memset(v, 0, 4*sizeof(float)); }

inline void fVector::set(const float f0, const float f1, const float f2, const float f3) {
	v[X] = f0; v[Y] = f1; v[Z] = f2; v[W] = f3;
}

inline const float fVector::sqrmag() const {
	return v[X]*v[X] + v[Y]*v[Y] + v[Z]*v[Z];
}

inline const float fVector::mag() const {
	return sqrt(sqrmag());
}

inline fVector& fVector::neg() { for(size_t i=0; i<3; i++) v[i] *= -1.0f; return *this; }

inline fVector3* fVector::v3() { return reinterpret_cast<fVector3*>(v); }

inline const fVector fVector::operator-() const {
	return Create(-v[X], -v[Y], -v[Z]);
}

inline const fVector& fVector::operator+=(const fVector& vec) {
	v[X] += vec[X]; v[Y] += vec[Y]; v[Z] += vec[Z];
	return *this;
}

inline const fVector& fVector::operator-=(const fVector& vec) {
	v[X] -= vec[X]; v[Y] -= vec[Y]; v[Z] -= vec[Z];
	return *this;
}

inline const fVector& fVector::operator*=(const float s) {
	v[0] *= s; v[1] *= s; v[2] *= s;
	return *this;
}

inline const fVector& fVector::operator/=(const float s) {
	v[0] /= s; v[1] /= s; v[2] /= s;
	return *this;
}

inline const fVector fVector::operator+(const fVector& vec) const {
	return Create(v[0]+vec[0], v[1]+vec[1], v[2]+vec[2], 1);
}

inline const fVector fVector::operator-(const fVector& vec) const {
	return Create(v[0]-vec[0], v[1]-vec[1], v[2]-vec[2], 1);
}


// -----------------------------------------------------------------------------
// fVector-related non-member ops

inline const fVector operator*(const fVector& v, const float s) {
	return fVector::Create(v[X]*s, v[Y]*s, v[Z]*s, 1);
}

inline const fVector operator*(const float s, const fVector& v) {
	return fVector::Create(v[X]*s, v[Y]*s, v[Z]*s, 1);
}

inline const fVector operator/(const fVector& v, const float s) {
	return fVector::Create(v[X]/s, v[Y]/s, v[Z]/s, 1);
}


//Angle vector.  Used to store orientations
class __pack__ vmsAngVec {
public:
	static const vmsAngVec ZERO;
	static const vmsAngVec Create(const fixang p, const fixang b, const fixang h) {
		vmsAngVec a;
		a[PA] = p; a[BA] = b; a[HA] = h;
		return a;
	}
	// access op for assignment
	fixang& operator[](size_t idx) { return a[idx]; }
	// read-only access op
	const fixang operator[](size_t idx) const { return a[idx]; }

	bool isZero() const { return !(a[PA] || a[HA] || a[BA]); }
	void setZero() { memset(a, 0, 3*sizeof(fixang)); }

private:
    fixang a[3];
};



/**
 * \class fVector3
 * A 3 element fixed-point vector.
 */
class vmsVector {
	public:
		static const vmsVector ZERO;

		static const vmsVector Create(fix f0, fix f1, fix f2);
		static const vmsVector avg(const vmsVector& src0, const vmsVector& src1);
		static const vmsVector avg(vmsVector& src0, vmsVector& src1,
								   vmsVector& src2, vmsVector& src3);
		static const vmsVector cross(const vmsVector& v0, const vmsVector& v1);
		//computes the delta angle between two vectors.
		//vectors need not be normalized. if they are, call vmsVector::deltaAngleNorm()
		//the forward vector (third parameter) can be NULL, in which case the absolute
		//value of the angle in returned.  Otherwise the angle around that vector is
		//returned.
		static const fixang deltaAngle(const vmsVector& v0, const vmsVector& v1, vmsVector *fVec);		//computes the delta angle between two normalized vectors.
		static const fixang deltaAngleNorm(const vmsVector& v0, const vmsVector& v1, vmsVector *fVec);
		static const fix dist(const vmsVector& vec0, const vmsVector& vec1);
		static const fix ssedot(vmsVector *v0, vmsVector *v1);
		static const fix dot(const vmsVector& v0, const vmsVector& v1);
		static const fix dot(const fix x, const fix y, const fix z, const vmsVector& v);
		static const fix normalize(vmsVector& v);
		static const vmsVector perp(const vmsVector& p0, const vmsVector& p1, const vmsVector& p2);
		static const vmsVector normal(const vmsVector& p0, const vmsVector& p1, const vmsVector& p2);
		static const vmsVector Random();
		static const vmsVector reflect(const vmsVector& d, const vmsVector& n);
		//return the normalized direction vector between two points
		//dest = normalized(end - start).  Returns mag of direction vector
		//NOTE: the order of the parameters matches the vector subtraction
		static const fix normalizedDir(vmsVector& dest, const vmsVector& end, const vmsVector& start);

		// access op for assignment
		fix& operator[](size_t idx);
		// read-only access op
		const fix operator[](size_t idx) const;
//		vmsVector& operator=(vmsVector& vec) {
//			memcpy(v, &vec[X], 3*sizeof(fix));
//			return *this;
//		}

		vmsVector& set(fix x, fix y, fix z);

		bool isZero() const;
		void setZero();
		const int sign() const;

		fix sqrmag() const;
		fix mag() const;
		vmsVector& neg();
		const vmsVector operator-() const;
		const vmsVector& operator+=(const vmsVector& vec);
		const vmsVector& operator-=(const vmsVector& vec);
		const vmsVector& operator*=(const fix s);
		const vmsVector& operator/=(const fix s);
		const vmsVector operator+(const vmsVector& vec) const;
		const vmsVector operator-(const vmsVector& vec) const;

		// compute intersection of a line through a point a, with the line being orthogonal relative
		// to the plane given by the normal n and a point p lieing in the plane, and store it in i.
		const vmsVector planeProjection(const vmsVector& n, const vmsVector& p) const;
		//compute the distance from a point to a plane.  takes the normalized normal
		//of the plane (ebx), a point on the plane (edi), and the point to check (esi).
		//returns distance in eax
		//distance is signed, so negative dist is on the back of the plane
		const fix distToPlane(const vmsVector& n, const vmsVector& p) const;
		//extract heading and pitch from a vector, assuming bank==0
		const vmsAngVec toAnglesVecNorm() const;
		//extract heading and pitch from a vector, assuming bank==0
		const vmsAngVec toAnglesVec() const;
		const fVector toFloat() const;
		const fVector3 toFloat3() const;

	private:
		fix v[3];
};

inline const vmsVector operator*(const vmsVector& v, const fix s);
inline const vmsVector operator*(const fix s, const vmsVector& v);
inline const vmsVector operator/(const vmsVector& v, const fix d);


// -----------------------------------------------------------------------------
// vmsVector static inlines

inline const vmsVector vmsVector::Create(fix f0, fix f1, fix f2) {
	vmsVector vec;
	vec.set(f0, f1, f2);
	return vec;
}

inline const vmsVector vmsVector::avg(const vmsVector& src0, const vmsVector& src1) {
	return Create((src0[X] + src1[X]) / 2,
	              (src0[Y] + src1[Y]) / 2,
	              (src0[Z] + src1[Z]) / 2);
}

inline const vmsVector vmsVector::avg(vmsVector& src0, vmsVector& src1,
						   vmsVector& src2, vmsVector& src3) {
	return Create((src0[X] + src1[X] + src2[X] + src3[X]) / 4,
	 (src0[Y] + src1[Y] + src2[Y] + src3[Y]) / 4,
	 (src0[Z] + src1[Z] + src2[Z] + src3[Z]) / 4);

}

inline const vmsVector vmsVector::cross(const vmsVector& v0, const vmsVector& v1) {
	return Create((fix) (((double) v0[Y] * (double) v1[Z] - (double) v0[Z] * (double) v1[Y]) / 65536.0),
	              (fix) (((double) v0[Z] * (double) v1[X] - (double) v0[X] * (double) v1[Z]) / 65536.0),
	              (fix) (((double) v0[X] * (double) v1[Y] - (double) v0[Y] * (double) v1[X]) / 65536.0));
}

//computes the delta angle between two vectors.
//vectors need not be normalized. if they are, call vmsVector::deltaAngleNorm()
//the forward vector (third parameter) can be NULL, in which case the absolute
//value of the angle in returned.  Otherwise the angle around that vector is
//returned.
inline const fixang vmsVector::deltaAngle(const vmsVector& v0, const vmsVector& v1, vmsVector *fVec) {
	vmsVector t0=v0, t1=v1;

	vmsVector::normalize(t0);
	vmsVector::normalize(t1);
	return deltaAngleNorm(t0, t1, fVec);
}

//computes the delta angle between two normalized vectors.
inline const fixang vmsVector::deltaAngleNorm(const vmsVector& v0, const vmsVector& v1, vmsVector *fVec) {
	fixang a = FixACos(vmsVector::dot(v0, v1));
	if(fVec) {
		vmsVector t = vmsVector::cross(v0, v1);
		if (vmsVector::dot(t, *fVec) < 0)
			a = -a;
	}
	return a;
}

inline const fix vmsVector::dist(const vmsVector& vec0, const vmsVector& vec1) {
	return (vec0-vec1).mag();
}

inline const fix vmsVector::ssedot(vmsVector *v0, vmsVector *v1) {
	#if ENABLE_SSE
	if (gameStates.render.bEnableSSE) {
			fVector	v0h, v1h;

		VmVecFixToFloat (&v0h, v0);
		VmVecFixToFloat (&v1h, v1);
	#if defined (_WIN32)
		_asm {
			movups	xmm0,v0h
			movups	xmm1,v1h
			mulps		xmm0,xmm1
			movups	v0h,xmm0
			}
	#elif defined (__unix__)
		asm (
			"movups	%0,%%xmm0\n\t"
			"movups	%1,%%xmm1\n\t"
			"mulps	%%xmm1,%%xmm0\n\t"
			"movups	%%xmm0,%0\n\t"
			: "=m" (v0h)
			: "m" (v0h), "m" (v1h)
			);
	#endif
		return (fix) ((v0h[X] + v0h[Y] + v0h[Z]) * 65536);
		}
	#else
	return 0;
	#endif
}

inline const fix vmsVector::dot(const vmsVector& v0, const vmsVector& v1) {
		return (fix) (((double) v0[X] * (double) v1[X] +
							(double) v0[Y] * (double) v1[Y] +
							(double) v0[Z] * (double) v1[Z])
						  / 65536.0);
}

inline const fix vmsVector::dot(const fix x, const fix y, const fix z, const vmsVector& v) {
	return (fix) (((double) x * (double) v[X] + (double) y * (double) v[Y] + (double) z * (double) v[Z]) / 65536.0);
}

inline const fix vmsVector::normalize(vmsVector& v) {
	fix m = v.mag();
	v[X] = FixDiv(v[X], m);
	v[Y] = FixDiv(v[Y], m);
	v[Z] = FixDiv(v[Z], m);
	return m;
}

inline const vmsVector vmsVector::perp(const vmsVector& p0, const vmsVector& p1, const vmsVector& p2) {
	vmsVector t0 = p1 - p0, t1 = p2 - p1;
	vmsVector::normalize(t0);
	vmsVector::normalize(t1);
	return vmsVector::cross(t0, t1);
}

inline const vmsVector vmsVector::normal(const vmsVector& p0, const vmsVector& p1, const vmsVector& p2) {
	vmsVector v = perp(p0, p1, p2);
	normalize(v);
	return v;
}

inline const vmsVector vmsVector::Random() {
	vmsVector v;
	int i = d_rand () % 3;

	if (i == 2) {
		v[X] = (d_rand () - 16384) | 1;	// make sure we don't create null vector
		v[Y] = d_rand () - 16384;
		v[Z] = d_rand () - 16384;
		}
	else if (i == 1) {
		v[Y] = d_rand () - 16384;
		v[Z] = d_rand () - 16384;
		v[X] = (d_rand () - 16384) | 1;	// make sure we don't create null vector
		}
	else {
		v[Z] = d_rand () - 16384;
		v[X] = (d_rand () - 16384) | 1;	// make sure we don't create null vector
		v[Y] = d_rand () - 16384;
	}
	normalize(v);
	return v;
}

inline const vmsVector vmsVector::reflect(const vmsVector& d, const vmsVector& n) {
	fix k = dot(d, n) * 2;
	vmsVector r = n;
	r *= k;
	r -= d;
	r.neg();
	return r;
}

//return the normalized direction vector between two points
//dest = normalized(end - start).  Returns mag of direction vector
//NOTE: the order of the parameters matches the vector subtraction
inline const fix vmsVector::normalizedDir(vmsVector& dest, const vmsVector& end, const vmsVector& start) {
	dest = end - start;
	return vmsVector::normalize(dest);
}


// -----------------------------------------------------------------------------
// vmsVector member inlines

inline fix& vmsVector::operator[](size_t idx) { return v[idx]; }

inline const fix vmsVector::operator[](size_t idx) const { return v[idx]; }
//		vmsVector& operator=(vmsVector& vec) {
//			memcpy(v, &vec[X], 3*sizeof(fix));
//			return *this;
//		}

inline vmsVector& vmsVector::set(fix x, fix y, fix z) { v[0] = x; v[1] = y; v[2] = z; return *this; }

inline bool vmsVector::isZero() const { return !(v[X] || v[Y] || v[Z]); }

inline void vmsVector::setZero() { memset(v, 0, 3*sizeof(fix)); }

inline const int vmsVector::sign() const { return (v[X] * v[Y] * v[Z] < 0) ? -1 : 1; }

inline fix vmsVector::sqrmag() const {
	return FixMul(v[X], v[X]) + FixMul(v[Y], v[Y]) + FixMul(v[Z], v[Z]);
}

inline fix vmsVector::mag() const {
	return fl2f(sqrt(f2fl(v[X])*f2fl(v[X]) + f2fl(v[Y])*f2fl(v[Y]) + f2fl(v[Z])*f2fl(v[Z])));
}

inline vmsVector& vmsVector::neg() { for(size_t i=0; i<3; i++) v[i] *= (fix)-1; return *this; }

inline const vmsVector vmsVector::operator-() const {
	return Create(-v[X], -v[Y], -v[Z]);
}

inline const vmsVector& vmsVector::operator+=(const vmsVector& vec) {
	v[0] += vec[0]; v[1] += vec[1]; v[2] += vec[2];
	return *this;
}

inline const vmsVector& vmsVector::operator-=(const vmsVector& vec) {
	v[0] -= vec[0]; v[1] -= vec[1]; v[2] -= vec[2];
	return *this;
}

inline const vmsVector& vmsVector::operator*=(const fix s) {
	v[0] = FixMul(v[0], s); v[1] = FixMul(v[1], s); v[2] = FixMul(v[2], s);
	return *this;
}

inline const vmsVector& vmsVector::operator/=(const fix s) {
	v[0] = FixDiv(v[0], s); v[1] = FixDiv(v[1], s); v[2] = FixDiv(v[2], s);
	return *this;
}

inline const vmsVector vmsVector::operator+(const vmsVector& vec) const {
	return Create(v[0]+vec[0], v[1]+vec[1], v[2]+vec[2]);
}

inline const vmsVector vmsVector::operator-(const vmsVector& vec) const {
	return Create(v[0]-vec[0], v[1]-vec[1], v[2]-vec[2]);
}


// compute intersection of a line through a point a, with the line being orthogonal relative
// to the plane given by the normal n and a point p lieing in the plane, and store it in i.
inline const vmsVector vmsVector::planeProjection(const vmsVector& n, const vmsVector& p) const {
	vmsVector i;
	double l = (double) -vmsVector::dot(n, p) / (double) vmsVector::dot(n, *this);
	i[X] = (fix) (l * (double) v[X]);
	i[Y] = (fix) (l * (double) v[Y]);
	i[Z] = (fix) (l * (double) v[Z]);
	return i;
}

//compute the distance from a point to a plane.  takes the normalized normal
//of the plane (ebx), a point on the plane (edi), and the point to check (esi).
//returns distance in eax
//distance is signed, so negative dist is on the back of the plane
inline const fix vmsVector::distToPlane(const vmsVector& n, const vmsVector& p) const {
	vmsVector t = *this - p;
	return vmsVector::dot(t, n);
}

//extract heading and pitch from a vector, assuming bank==0
inline const vmsAngVec vmsVector::toAnglesVecNorm() const {
	vmsAngVec a;
	a[BA] = 0;		//always zero bank
	a[PA] = FixASin (-v[Y]);
	a[HA] = (v[X] || v[Z]) ? FixAtan2 (v[Z], v[X]) : 0;
	return a;
}

//extract heading and pitch from a vector, assuming bank==0
inline const vmsAngVec vmsVector::toAnglesVec() const	{
	vmsVector t = *this;

//			if (vmsVector::normalize(t))
	vmsVector::normalize(t);
	return t.toAnglesVecNorm();
}


// -----------------------------------------------------------------------------
// vmsVector-related non-member ops

inline const vmsVector operator*(const vmsVector& v, const fix s) {
	return vmsVector::Create(FixMul(v[0], s), FixMul(v[1], s), FixMul(v[2], s));
}

inline const vmsVector operator*(const fix s, const vmsVector& v) {
	return vmsVector::Create(FixMul(v[0], s), FixMul(v[1], s), FixMul(v[2], s));
}

inline const vmsVector operator/(const vmsVector& v, const fix d) {
	return vmsVector::Create(FixDiv(v[0], d), FixMul(v[1], d), FixMul(v[2], d));
}


// -----------------------------------------------------------------------------


class fMatrix;	// forward-declaration for toFloat()


/**
 * \class vmsMatrix
 *
 * A 3x3 rotation matrix.  Sorry about the numbering starting with one. Ordering
 * is across then down, so <m1,m2,m3> is the first row.
 */
class vmsMatrix {
	public:
		static const vmsMatrix IDENTITY;
		static const vmsMatrix Create(const vmsVector& r, const vmsVector& u, const vmsVector& f);
		static const vmsMatrix Create(fix sinp, fix cosp, fix sinb, fix cosb, fix sinh, fix cosh);
		//computes a matrix from a set of three angles.  returns ptr to matrix
		static const vmsMatrix Create(const vmsAngVec& a);
		//computes a matrix from a forward vector and an angle
		static const vmsMatrix Create(vmsVector *v, fixang a);
		static const vmsMatrix CreateF(const vmsVector& fVec);
		static const vmsMatrix CreateFU(const vmsVector& fVec, const vmsVector& uVec);
		static const vmsMatrix CreateFR(const vmsVector& fVec, const vmsVector& rVec);

		static vmsMatrix& invert(vmsMatrix& m);
		static vmsMatrix& transpose(vmsMatrix& m);

		// row access op for assignment
		const vmsVector& operator[](size_t idx) const;
		// read-only row access op
		vmsVector& operator[](size_t idx);

		const vmsVector operator*(const vmsVector& v) const;
		const vmsMatrix operator*(const vmsMatrix& m) const;

		const fix det() const;
		const vmsMatrix inverse() const;
		const vmsMatrix transpose() const;

		//make sure matrix is orthogonal
		void checkAndFix();

		//extract angles from a matrix
		const vmsAngVec extractAnglesVec() const;

		const fMatrix toFloat() const;

	private:
		vmsVector vec[3];
};


// -----------------------------------------------------------------------------
// vmsMatrix static inlines

inline const vmsMatrix vmsMatrix::Create(const vmsVector& r, const vmsVector& u, const vmsVector& f) {
	vmsMatrix m;
	m[RVEC] = r;
	m[UVEC] = u;
	m[FVEC] = f;
	return m;
}

inline const vmsMatrix vmsMatrix::Create(fix sinp, fix cosp, fix sinb, fix cosb, fix sinh, fix cosh) {
	vmsMatrix m;
	fix sbsh, cbch, cbsh, sbch;

	sbsh = FixMul (sinb, sinh);
	cbch = FixMul (cosb, cosh);
	cbsh = FixMul (cosb, sinh);
	sbch = FixMul (sinb, cosh);
	m[RVEC][X] = cbch + FixMul (sinp, sbsh);		//m1
	m[UVEC][Z] = sbsh + FixMul (sinp, cbch);		//m8
	m[UVEC][X] = FixMul (sinp, cbsh) - sbch;		//m2
	m[RVEC][Z] = FixMul (sinp, sbch) - cbsh;		//m7
	m[FVEC][X] = FixMul (sinh, cosp);			//m3
	m[RVEC][Y] = FixMul (sinb, cosp);			//m4
	m[UVEC][Y] = FixMul (cosb, cosp);			//m5
	m[FVEC][Z] = FixMul (cosh, cosp);			//m9
	m[FVEC][Y] = -sinp;							//m6
	return m;
}

//computes a matrix from a set of three angles.  returns ptr to matrix
inline const vmsMatrix vmsMatrix::Create(const vmsAngVec& a) {
	fix sinp, cosp, sinb, cosb, sinh, cosh;
	FixSinCos (a[PA], &sinp, &cosp);
	FixSinCos (a[BA], &sinb, &cosb);
	FixSinCos (a[HA], &sinh, &cosh);
	return Create(sinp, cosp, sinb, cosb, sinh, cosh);
}

//computes a matrix from a forward vector and an angle
inline const vmsMatrix vmsMatrix::Create(vmsVector *v, fixang a) {
	fix sinb, cosb, sinp, cosp;

	FixSinCos (a, &sinb, &cosb);
	sinp = -(*v)[Y];
	cosp = FixSqrt (f1_0 - FixMul (sinp, sinp));
	return Create(sinp, cosp, sinb, cosb, FixDiv((*v)[X], cosp), FixDiv((*v)[Z], cosp));
}


inline vmsMatrix& vmsMatrix::invert(vmsMatrix& m) {
	// TODO implement?
	return m;
}

inline vmsMatrix& vmsMatrix::transpose(vmsMatrix& m) {
	fix t;
	t = m[UVEC][X];  m[UVEC][X] = m[RVEC][Y];  m[RVEC][Y] = t;
	t = m[FVEC][X];  m[FVEC][X] = m[RVEC][Z];  m[RVEC][Z] = t;
	t = m[FVEC][Y];  m[FVEC][Y] = m[UVEC][Z];  m[UVEC][Z] = t;
	return m;
}


// -----------------------------------------------------------------------------
// vmsMatrix member ops

inline const vmsVector& vmsMatrix::operator[](size_t idx) const {
	return vec[idx];
}

inline vmsVector& vmsMatrix::operator[](size_t idx) {
	return vec[idx];
}

inline const vmsVector vmsMatrix::operator*(const vmsVector& v) const {
	return vmsVector::Create(vmsVector::dot(v, vec[RVEC]),
	                         vmsVector::dot(v, vec[UVEC]),
	                         vmsVector::dot(v, vec[FVEC]));
}

inline const vmsMatrix vmsMatrix::operator*(const vmsMatrix& m) const {
	vmsVector v;
	vmsMatrix r;
	v[X] = vec[RVEC][X];
	v[Y] = vec[UVEC][X];
	v[Z] = vec[FVEC][X];
	r[RVEC][X] = vmsVector::dot(v, m[RVEC]);
	r[UVEC][X] = vmsVector::dot(v, m[UVEC]);
	r[FVEC][X] = vmsVector::dot(v, m[FVEC]);
	v[X] = vec[RVEC][Y];
	v[Y] = vec[UVEC][Y];
	v[Z] = vec[FVEC][Y];
	r[RVEC][Y] = vmsVector::dot(v, m[RVEC]);
	r[UVEC][Y] = vmsVector::dot(v, m[UVEC]);
	r[FVEC][Y] = vmsVector::dot(v, m[FVEC]);
	v[X] = vec[RVEC][Z];
	v[Y] = vec[UVEC][Z];
	v[Z] = vec[FVEC][Z];
	r[RVEC][Z] = vmsVector::dot(v, m[RVEC]);
	r[UVEC][Z] = vmsVector::dot(v, m[UVEC]);
	r[FVEC][Z] = vmsVector::dot(v, m[FVEC]);
	return r;
}

inline const fix vmsMatrix::det() const {
	fix	xDet;
	//PrintLog ("            CalcDetValue (R: %d, %d, %d; F: %d, %d, %d; U: %d, %d, %d)\n", m->rVec[X], m->rVec[Y], m->rVec[Z], m->fVec[X], m->fVec[Y], m->fVec[Z], m->uVec[X], m->uVec[Y], m->uVec[Z]);
	//PrintLog ("               xDet = FixMul (m->rVec[X], FixMul (m->uVec[Y], m->fVec[Z]))\n");
	xDet = FixMul (vec[RVEC][X], FixMul (vec[UVEC][Y], vec[FVEC][Z]));
	//PrintLog ("               xDet -= FixMul (m->rVec[X], FixMul (m->uVec[Z], m->fVec[Y]))\n");
	xDet -= FixMul (vec[RVEC][X], FixMul (vec[UVEC][Z], vec[FVEC][Y]));
	//PrintLog ("               xDet -= FixMul (m->rVec[Y], FixMul (m->uVec[X], m->fVec[Z]))\n");
	xDet -= FixMul (vec[RVEC][Y], FixMul (vec[UVEC][X], vec[FVEC][Z]));
	//PrintLog ("               xDet += FixMul (m->rVec[Y], FixMul (m->uVec[Z], m->fVec[X]))\n");
	xDet += FixMul (vec[RVEC][Y], FixMul (vec[UVEC][Z], vec[FVEC][X]));
	//PrintLog ("               xDet += FixMul (m->rVec[Z], FixMul (m->uVec[X], m->fVec[Y]))\n");
	xDet += FixMul (vec[RVEC][Z], FixMul (vec[UVEC][X], vec[FVEC][Y]));
	//PrintLog ("               xDet -= FixMul (m->rVec[Z], FixMul (m->uVec[Y], m->fVec[X]))\n");
	xDet -= FixMul (vec[RVEC][Z], FixMul (vec[UVEC][Y], vec[FVEC][X]));
	return xDet;
	//PrintLog ("             m = %d\n", xDet);
}

inline const vmsMatrix vmsMatrix::inverse() const {
	fix	xDet = det();
	vmsMatrix m;

	m[RVEC][X] = FixDiv (FixMul (vec[UVEC][Y], vec[FVEC][Z]) - FixMul (vec[UVEC][Z], vec[FVEC][Y]), xDet);
	m[RVEC][Y] = FixDiv (FixMul (vec[RVEC][Z], vec[FVEC][Y]) - FixMul (vec[RVEC][Y], vec[FVEC][Z]), xDet);
	m[RVEC][Z] = FixDiv (FixMul (vec[RVEC][Y], vec[UVEC][Z]) - FixMul (vec[RVEC][Z], vec[UVEC][Y]), xDet);
	m[UVEC][X] = FixDiv (FixMul (vec[UVEC][Z], vec[FVEC][X]) - FixMul (vec[UVEC][X], vec[FVEC][Z]), xDet);
	m[UVEC][Y] = FixDiv (FixMul (vec[RVEC][X], vec[FVEC][Z]) - FixMul (vec[RVEC][Z], vec[FVEC][X]), xDet);
	m[UVEC][Z] = FixDiv (FixMul (vec[RVEC][Z], vec[UVEC][X]) - FixMul (vec[RVEC][X], vec[UVEC][Z]), xDet);
	m[FVEC][X] = FixDiv (FixMul (vec[UVEC][X], vec[FVEC][Y]) - FixMul (vec[UVEC][Y], vec[FVEC][X]), xDet);
	m[FVEC][Y] = FixDiv (FixMul (vec[RVEC][Y], vec[FVEC][X]) - FixMul (vec[RVEC][X], vec[FVEC][Y]), xDet);
	m[FVEC][Z] = FixDiv (FixMul (vec[RVEC][X], vec[UVEC][Y]) - FixMul (vec[RVEC][Y], vec[UVEC][X]), xDet);
	return m;
}

inline const vmsMatrix vmsMatrix::transpose() const {
	vmsMatrix dest;
	dest[RVEC][X] = vec[RVEC][X];
	dest[RVEC][Y] = vec[UVEC][X];
	dest[RVEC][Z] = vec[FVEC][X];
	dest[UVEC][X] = vec[RVEC][Y];
	dest[UVEC][Y] = vec[UVEC][Y];
	dest[UVEC][Z] = vec[FVEC][Y];
	dest[FVEC][X] = vec[RVEC][Z];
	dest[FVEC][Y] = vec[UVEC][Z];
	dest[FVEC][Z] = vec[FVEC][Z];
	return dest;
}

//make sure this matrix is orthogonal
inline void vmsMatrix::checkAndFix() {
	*this = CreateFU(vec[FVEC], vec[UVEC]);
}

//extract angles from a matrix
inline const vmsAngVec vmsMatrix::extractAnglesVec() const {
	vmsAngVec a;
	fix sinh, cosh, cosp;

	if (vec[FVEC][X]==0 && vec[FVEC][Z]==0)		//zero head
		a[HA] = 0;
	else
		a[HA] = FixAtan2(vec[FVEC][Z], vec[FVEC][X]);
	FixSinCos(a[HA], &sinh, &cosh);
	if (abs(sinh) > abs(cosh))				//sine is larger, so use it
		cosp = FixDiv(vec[FVEC][X], sinh);
	else											//cosine is larger, so use it
		cosp = FixDiv(vec[FVEC][Z], cosh);
	if (cosp==0 && vec[FVEC][Y]==0)
		a[PA] = 0;
	else
		a[PA] = FixAtan2(cosp, -vec[FVEC][Y]);
	if (cosp == 0)	//the cosine of pitch is zero.  we're pitched straight up. say no bank
		a[BA] = 0;
	else {
		fix sinb, cosb;

		sinb = FixDiv(vec[RVEC][Y], cosp);
		cosb = FixDiv(vec[UVEC][Y], cosp);
		if (sinb==0 && cosb==0)
			a[BA] = 0;
		else
			a[BA] = FixAtan2(cosb, sinb);
		}
	return a;
}



/**
 * \class fMatrix
 *
 * A 4x4 floating point transformation matrix
 */
class fMatrix {
	public:
		static const fMatrix IDENTITY;
		static const fMatrix Create(const fVector& r, const fVector& u, const fVector& f, const fVector& w);
		static const fMatrix Create(float sinp, float cosp, float sinb, float cosb, float sinh, float cosh);

		static fMatrix& invert(fMatrix& m);
		static fMatrix& transpose(fMatrix& m);

		// row access op for assignment
		const fVector& operator[](size_t idx) const;
		// read-only row access op
		fVector& operator[](size_t idx);

		const fVector operator*(const fVector& v) const;
		const fVector3 operator*(const fVector3& v);

		const float det() const;
		const fMatrix inverse() const;
		const fMatrix transpose() const;

	private:
		fVector	vec[4];
};


// -----------------------------------------------------------------------------
// fMatrix static inlines

inline const fMatrix fMatrix::Create(const fVector& r, const fVector& u, const fVector& f, const fVector& w) {
	fMatrix m;
	m[RVEC] = r;
	m[UVEC] = u;
	m[FVEC] = f;
	m[HVEC] = w;
	return m;
}

inline const fMatrix fMatrix::Create(float sinp, float cosp, float sinb, float cosb, float sinh, float cosh) {
	fMatrix m;
	float sbsh, cbch, cbsh, sbch;

	sbsh = sinb * sinh;
	cbch = cosb * cosh;
	cbsh = cosb * sinh;
	sbch = sinb * cosh;
	m[RVEC][X] = cbch + sinp * sbsh;	//m1
	m[UVEC][Z] = sbsh + sinp * cbch;	//m8
	m[UVEC][X] = sinp * cbsh - sbch;	//m2
	m[RVEC][Z] = sinp * sbch - cbsh;	//m7
	m[FVEC][X] = sinh * cosp;		//m3
	m[RVEC][Y] = sinb * cosp;		//m4
	m[UVEC][Y] = cosb * cosp;		//m5
	m[FVEC][Z] = cosh * cosp;		//m9
	m[FVEC][Y] = -sinp;				//m6
	return m;
}

inline fMatrix& fMatrix::invert(fMatrix& m) {
	//TODO: implement?
	return m;
}

inline fMatrix& fMatrix::transpose(fMatrix& m) {
	float t;
	t = m[UVEC][X];  m[UVEC][X] = m[RVEC][Y];  m[RVEC][Y] = t;
	t = m[FVEC][X];  m[FVEC][X] = m[RVEC][Z];  m[RVEC][Z] = t;
	t = m[FVEC][Y];  m[FVEC][Y] = m[UVEC][Z];  m[UVEC][Z] = t;
	return m;
}


// -----------------------------------------------------------------------------
// fMatrix member ops

inline const fVector& fMatrix::operator[](size_t idx) const {
	return vec[idx];
}

inline fVector& fMatrix::operator[](size_t idx) {
	return vec[idx];
}


inline const fVector fMatrix::operator*(const fVector& v) const {
	return fVector::Create(fVector::dot(v, vec[RVEC]),
			fVector::dot(v, vec[UVEC]),
			fVector::dot(v, vec[FVEC]));
}

inline const fVector3 fMatrix::operator*(const fVector3& v) {
	return fVector3::Create(fVector3::dot(v, *vec[RVEC].v3()),
			fVector3::dot(v, *vec[UVEC].v3()),
			fVector3::dot(v, *vec[FVEC].v3()));
}

inline const float fMatrix::det() const {
	return vec[RVEC][X] * vec[UVEC][Y] * vec[FVEC][Z] -
	vec[RVEC][X] * vec[UVEC][Z] * vec[FVEC][Y] -
	vec[RVEC][Y] * vec[UVEC][X] * vec[FVEC][Z] +
	vec[RVEC][Y] * vec[UVEC][Z] * vec[FVEC][X] +
	vec[RVEC][Z] * vec[UVEC][X] * vec[FVEC][Y] -
	vec[RVEC][Z] * vec[UVEC][Y] * vec[FVEC][X];
}

inline const fMatrix fMatrix::inverse() const {
	float fDet = det();
	fMatrix	m = *this;

	m[RVEC][X] = (vec[UVEC][Y] * vec[FVEC][Z] - vec[UVEC][Z] * vec[FVEC][Y]) / fDet;
	m[RVEC][Y] = (vec[RVEC][Z] * vec[FVEC][Y] - vec[RVEC][Y] * vec[FVEC][Z]) / fDet;
	m[RVEC][Z] = (vec[RVEC][Y] * vec[UVEC][Z] - vec[RVEC][Z] * vec[UVEC][Y]) / fDet;
	m[UVEC][X] = (vec[UVEC][Z] * vec[FVEC][X] - vec[UVEC][X] * vec[FVEC][Z]) / fDet;
	m[UVEC][Y] = (vec[RVEC][X] * vec[FVEC][Z] - vec[RVEC][Z] * vec[FVEC][X]) / fDet;
	m[UVEC][Z] = (vec[RVEC][Z] * vec[UVEC][X] - vec[RVEC][X] * vec[UVEC][Z]) / fDet;
	m[FVEC][X] = (vec[UVEC][X] * vec[FVEC][Y] - vec[UVEC][Y] * vec[FVEC][X]) / fDet;
	m[FVEC][Y] = (vec[RVEC][Y] * vec[FVEC][X] - vec[RVEC][X] * vec[FVEC][Y]) / fDet;
	m[FVEC][Z] = (vec[RVEC][X] * vec[UVEC][Y] - vec[RVEC][Y] * vec[UVEC][X]) / fDet;
	return m;
}

inline const fMatrix fMatrix::transpose() const {
	fMatrix dest;
	dest[RVEC][X] = vec[RVEC][X];
	dest[RVEC][Y] = vec[UVEC][X];
	dest[RVEC][Z] = vec[FVEC][X];
	dest[UVEC][X] = vec[RVEC][Y];
	dest[UVEC][Y] = vec[UVEC][Y];
	dest[UVEC][Z] = vec[FVEC][Y];
	dest[FVEC][X] = vec[RVEC][Z];
	dest[FVEC][Y] = vec[UVEC][Z];
	dest[FVEC][Z] = vec[FVEC][Z];
	return dest;
}


// -----------------------------------------------------------------------------
// misc conversion member ops

inline const fVector vmsVector::toFloat() const {
	fVector d;
	d[X] = f2fl(v[X]); d[Y] = f2fl(v[Y]); d[Z] = f2fl(v[Z]); d[W] = 1; return d;
}

inline const fVector3 vmsVector::toFloat3() const {
	fVector3 d;
	d[X] = f2fl(v[X]); d[Y] = f2fl(v[Y]); d[Z] = f2fl(v[Z]); return d;
}

inline const vmsVector fVector::toFix() const {
	vmsVector d;
	d[X] = fl2f(v[X]); d[Y] = fl2f(v[Y]); d[Z] = fl2f(v[Z]); return d;
}

inline const vmsVector fVector3::toFix() const {
	vmsVector d;
	d[X] = fl2f(v[X]); d[Y] = fl2f(v[Y]); d[Z] = fl2f(v[Z]); return d;
}

inline const fMatrix vmsMatrix::toFloat() const {
	fMatrix m;
	m[RVEC] = vec[RVEC].toFloat();
	m[UVEC] = vec[UVEC].toFloat();
	m[FVEC] = vec[FVEC].toFloat();
	m[HVEC] = fVector::ZERO;
	return m;
}


// -----------------------------------------------------------------------------
// misc remaining C-style funcs

const int VmPointLineIntersection(vmsVector& hitP, const vmsVector& p1, const vmsVector& p2, const vmsVector& p3, int bClampToFarthest);
//const int VmPointLineIntersection(vmsVector& hitP, const vmsVector& p1, const vmsVector& p2, const vmsVector& p3, const vmsVector& vPos, int bClampToFarthest);
const fix VmLinePointDist(const vmsVector& a, const vmsVector& b, const vmsVector& p);
const int VmPointLineIntersection(fVector& hitP, const fVector& p1, const fVector& p2, const fVector& p3, const fVector& vPos, int bClamp);
const int VmPointLineIntersection(fVector& hitP, const fVector& p1, const fVector& p2, const fVector& p3, int bClamp);
const int VmPointLineIntersection(fVector3& hitP, const fVector3& p1, const fVector3& p2, const fVector3& p3, fVector3 *vPos, int bClamp);
const float VmLinePointDist(const fVector& a, const fVector& b, const fVector& p, int bClamp);
const float VmLinePointDist(const fVector3& a, const fVector3& b, const fVector3& p, int bClamp);
const float VmLineLineIntersection(const fVector3& v1, const fVector3& v2, const fVector3& v3, const fVector3& v4, fVector3& va, fVector3& vb);
const float VmLineLineIntersection(const fVector& v1, const fVector& v2, const fVector& v3, const fVector& v4, fVector& va, fVector& vb);

float TriangleSize (const vmsVector& p0, const vmsVector& p1, const vmsVector& p2);

// ------------------------------------------------------------------------

#endif //_VECMAT_H
