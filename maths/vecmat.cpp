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

#include <cstdlib>
#include <cmath>           // for sqrt
#include <cassert>

#include "maths.h"
#include "vecmat.h"
#include "descent.h"
#include "error.h"

#define EXACT_VEC_MAG	1
#if DBG
#	define ENABLE_SSE		0
#elif defined (_MSC_VER)
#	define ENABLE_SSE		1
#else
#	define ENABLE_SSE		0
#endif

#ifndef ASM_VECMAT

// ------------------------------------------------------------------------
// static const initializations

const CFloatVector  CFloatVector::ZERO  = CFloatVector::Create (0,0,0,1);
const CFloatVector  CFloatVector::ZERO4 = CFloatVector::Create (0,0,0,0);
const CFloatVector  CFloatVector::XVEC  = CFloatVector::Create (1,0,0,1);
const CFloatVector  CFloatVector::YVEC  = CFloatVector::Create (0,1,0,1);
const CFloatVector  CFloatVector::ZVEC  = CFloatVector::Create (0,0,1,1);

const CFloatVector3 CFloatVector3::ZERO = CFloatVector3::Create (0,0,0);
const CFloatVector3 CFloatVector3::XVEC = CFloatVector3::Create (1,0,0);
const CFloatVector3 CFloatVector3::YVEC = CFloatVector3::Create (0,1,0);
const CFloatVector3 CFloatVector3::ZVEC = CFloatVector3::Create (0,0,1);

const CFixVector CFixVector::ZERO = CFixVector::Create (0,0,0);
const CFixVector CFixVector::XVEC = CFixVector::Create (I2X (1),0,0);
const CFixVector CFixVector::YVEC = CFixVector::Create (0,I2X (1),0);
const CFixVector CFixVector::ZVEC = CFixVector::Create (0,0,I2X (1));

const CAngleVector CAngleVector::ZERO = CAngleVector::Create (0,0,0);

const CFixMatrix CFixMatrix::IDENTITY = CFixMatrix::Create (CFixVector::XVEC,
																				CFixVector::YVEC,
																				CFixVector::ZVEC);

const CFloatMatrix CFloatMatrix::IDENTITY = CFloatMatrix::Create (CFloatVector::Create (1.0f, 0, 0, 0),
																						CFloatVector::Create (0, 1.0f, 0, 0),
																						CFloatVector::Create (0, 0, 1.0f, 0),
																						CFloatVector::Create (0, 0, 0, 1.0f));

// ------------------------------------------------------------------------

#include <cmath>

#endif

// ------------------------------------------------------------------------

const CFloatMatrix CFloatMatrix::Create (const CFloatVector& r, const CFloatVector& u, const CFloatVector& f, const CFloatVector& w) {
	CFloatMatrix m;
	m.m_data.mat [RVEC] = r;
	m.m_data.mat [UVEC] = u;
	m.m_data.mat [FVEC] = f;
	m.m_data.mat [HVEC] = w;
	return m;
}

// ------------------------------------------------------------------------

const CFloatMatrix CFloatMatrix::Create (float sinp, float cosp, float sinb, float cosb, float sinh, float cosh) {
	CFloatMatrix m;
	float sbsh, cbch, cbsh, sbch;

	sbsh = sinb * sinh;
	cbch = cosb * cosh;
	cbsh = cosb * sinh;
	sbch = sinb * cosh;
	m.m_data.mat [RVEC][X] = cbch + sinp * sbsh;	//m1
	m.m_data.mat [UVEC][Z] = sbsh + sinp * cbch;	//m8
	m.m_data.mat [UVEC][X] = sinp * cbsh - sbch;	//m2
	m.m_data.mat [RVEC][Z] = sinp * sbch - cbsh;	//m7
	m.m_data.mat [FVEC][X] = sinh * cosp;		//m3
	m.m_data.mat [RVEC][Y] = sinb * cosp;		//m4
	m.m_data.mat [UVEC][Y] = cosb * cosp;		//m5
	m.m_data.mat [FVEC][Z] = cosh * cosp;		//m9
	m.m_data.mat [FVEC][Y] = -sinp;				//m6
	return m;
}

// ------------------------------------------------------------------------

const float CFloatMatrix::Det (void) 
{
return m_data.mat [RVEC][X] * m_data.mat [UVEC][Y] * m_data.mat [FVEC][Z] -
		 m_data.mat [RVEC][X] * m_data.mat [UVEC][Z] * m_data.mat [FVEC][Y] -
		 m_data.mat [RVEC][Y] * m_data.mat [UVEC][X] * m_data.mat [FVEC][Z] +
		 m_data.mat [RVEC][Y] * m_data.mat [UVEC][Z] * m_data.mat [FVEC][X] +
		 m_data.mat [RVEC][Z] * m_data.mat [UVEC][X] * m_data.mat [FVEC][Y] -
		 m_data.mat [RVEC][Z] * m_data.mat [UVEC][Y] * m_data.mat [FVEC][X];
}

// -----------------------------------------------------------------------------

const CFloatMatrix CFloatMatrix::Inverse (void) 
{
	float fDet = Det ();
	CFloatMatrix	m = *this;

m.m_data.mat [RVEC][X] = (m_data.mat [UVEC][Y] * m_data.mat [FVEC][Z] - m_data.mat [UVEC][Z] * m_data.mat [FVEC][Y]) / fDet;
m.m_data.mat [RVEC][Y] = (m_data.mat [RVEC][Z] * m_data.mat [FVEC][Y] - m_data.mat [RVEC][Y] * m_data.mat [FVEC][Z]) / fDet;
m.m_data.mat [RVEC][Z] = (m_data.mat [RVEC][Y] * m_data.mat [UVEC][Z] - m_data.mat [RVEC][Z] * m_data.mat [UVEC][Y]) / fDet;
m.m_data.mat [UVEC][X] = (m_data.mat [UVEC][Z] * m_data.mat [FVEC][X] - m_data.mat [UVEC][X] * m_data.mat [FVEC][Z]) / fDet;
m.m_data.mat [UVEC][Y] = (m_data.mat [RVEC][X] * m_data.mat [FVEC][Z] - m_data.mat [RVEC][Z] * m_data.mat [FVEC][X]) / fDet;
m.m_data.mat [UVEC][Z] = (m_data.mat [RVEC][Z] * m_data.mat [UVEC][X] - m_data.mat [RVEC][X] * m_data.mat [UVEC][Z]) / fDet;
m.m_data.mat [FVEC][X] = (m_data.mat [UVEC][X] * m_data.mat [FVEC][Y] - m_data.mat [UVEC][Y] * m_data.mat [FVEC][X]) / fDet;
m.m_data.mat [FVEC][Y] = (m_data.mat [RVEC][Y] * m_data.mat [FVEC][X] - m_data.mat [RVEC][X] * m_data.mat [FVEC][Y]) / fDet;
m.m_data.mat [FVEC][Z] = (m_data.mat [RVEC][X] * m_data.mat [UVEC][Y] - m_data.mat [RVEC][Y] * m_data.mat [UVEC][X]) / fDet;
return m;
}

// -----------------------------------------------------------------------------

const CFloatMatrix CFloatMatrix::Mul (CFloatMatrix& other) 
{
CFloatVector v;
CFloatMatrix m;
v [X] = m_data.mat [RVEC][X];
v [Y] = m_data.mat [UVEC][X];
v [Z] = m_data.mat [FVEC][X];
m.m_data.mat [RVEC][X] = CFloatVector::Dot (v, other.m_data.mat [RVEC]);
m.m_data.mat [UVEC][X] = CFloatVector::Dot (v, other.m_data.mat [UVEC]);
m.m_data.mat [FVEC][X] = CFloatVector::Dot (v, other.m_data.mat [FVEC]);
v [X] = m_data.mat [RVEC][Y];
v [Y] = m_data.mat [UVEC][Y];
v [Z] = m_data.mat [FVEC][Y];
m.m_data.mat [RVEC][Y] = CFloatVector::Dot (v, other.m_data.mat [RVEC]);
m.m_data.mat [UVEC][Y] = CFloatVector::Dot (v, other.m_data.mat [UVEC]);
m.m_data.mat [FVEC][Y] = CFloatVector::Dot (v, other.m_data.mat [FVEC]);
v [X] = m_data.mat [RVEC][Z];
v [Y] = m_data.mat [UVEC][Z];
v [Z] = m_data.mat [FVEC][Z];
m.m_data.mat [RVEC][Z] = CFloatVector::Dot (v, other.m_data.mat [RVEC]);
m.m_data.mat [UVEC][Z] = CFloatVector::Dot (v, other.m_data.mat [UVEC]);
m.m_data.mat [FVEC][Z] = CFloatVector::Dot (v, other.m_data.mat [FVEC]);
CFloatVector::Normalize (m.m_data.mat [RVEC]);
CFloatVector::Normalize (m.m_data.mat [UVEC]);
CFloatVector::Normalize (m.m_data.mat [FVEC]);
return m;
}

//------------------------------------------------------------------------------

CFloatMatrix& CFloatMatrix::Transpose (CFloatMatrix& dest, CFloatMatrix& src)
{
dest [0] = src [0];
dest [4] = src [1];
dest [8] = src [2];
dest [1] = src [4];
dest [5] = src [5];
dest [9] = src [6];
dest [2] = src [8];
dest [6] = src [9];
dest [10] = src [10];
dest [11] = dest [12] = dest [13] = dest [14] = 0;
dest [15] = 1.0f;
return dest;
}

// ----------------------------------------------------------------------------

CFixMatrix CFixMatrix::Mul (const CFixMatrix& other) 
{
CFixVector v;
CFixMatrix m;

v [X] = m_data.mat [RVEC][X];
v [Y] = m_data.mat [UVEC][X];
v [Z] = m_data.mat [FVEC][X];
m.m_data.mat [RVEC][X] = CFixVector::Dot (v, other.m_data.mat [RVEC]);
m.m_data.mat [UVEC][X] = CFixVector::Dot (v, other.m_data.mat [UVEC]);
m.m_data.mat [FVEC][X] = CFixVector::Dot (v, other.m_data.mat [FVEC]);
v [X] = m_data.mat [RVEC][Y];
v [Y] = m_data.mat [UVEC][Y];
v [Z] = m_data.mat [FVEC][Y];
m.m_data.mat [RVEC][Y] = CFixVector::Dot (v, other.m_data.mat [RVEC]);
m.m_data.mat [UVEC][Y] = CFixVector::Dot (v, other.m_data.mat [UVEC]);
m.m_data.mat [FVEC][Y] = CFixVector::Dot (v, other.m_data.mat [FVEC]);
v [X] = m_data.mat [RVEC][Z];
v [Y] = m_data.mat [UVEC][Z];
v [Z] = m_data.mat [FVEC][Z];
m.m_data.mat [RVEC][Z] = CFixVector::Dot (v, other.m_data.mat [RVEC]);
m.m_data.mat [UVEC][Z] = CFixVector::Dot (v, other.m_data.mat [UVEC]);
m.m_data.mat [FVEC][Z] = CFixVector::Dot (v, other.m_data.mat [FVEC]);
CFixVector::Normalize (m.m_data.mat [RVEC]);
CFixVector::Normalize (m.m_data.mat [UVEC]);
CFixVector::Normalize (m.m_data.mat [FVEC]);
return m;
}

// -----------------------------------------------------------------------------

const fix CFixMatrix::Det (void) 
{
fix xDet = FixMul (m_data.mat [RVEC][X], FixMul (m_data.mat [UVEC][Y], m_data.mat [FVEC][Z]));
xDet -= FixMul (m_data.mat [RVEC][X], FixMul (m_data.mat [UVEC][Z], m_data.mat [FVEC][Y]));
xDet -= FixMul (m_data.mat [RVEC][Y], FixMul (m_data.mat [UVEC][X], m_data.mat [FVEC][Z]));
xDet += FixMul (m_data.mat [RVEC][Y], FixMul (m_data.mat [UVEC][Z], m_data.mat [FVEC][X]));
xDet += FixMul (m_data.mat [RVEC][Z], FixMul (m_data.mat [UVEC][X], m_data.mat [FVEC][Y]));
xDet -= FixMul (m_data.mat [RVEC][Z], FixMul (m_data.mat [UVEC][Y], m_data.mat [FVEC][X]));
return xDet;
}

// -----------------------------------------------------------------------------

const CFixMatrix CFixMatrix::Inverse (void) 
{
	fix xDet = Det ();
	CFixMatrix m;

m.m_data.mat [RVEC][X] = FixDiv (FixMul (m_data.mat [UVEC][Y], m_data.mat [FVEC][Z]) - FixMul (m_data.mat [UVEC][Z], m_data.mat [FVEC][Y]), xDet);
m.m_data.mat [RVEC][Y] = FixDiv (FixMul (m_data.mat [RVEC][Z], m_data.mat [FVEC][Y]) - FixMul (m_data.mat [RVEC][Y], m_data.mat [FVEC][Z]), xDet);
m.m_data.mat [RVEC][Z] = FixDiv (FixMul (m_data.mat [RVEC][Y], m_data.mat [UVEC][Z]) - FixMul (m_data.mat [RVEC][Z], m_data.mat [UVEC][Y]), xDet);
m.m_data.mat [UVEC][X] = FixDiv (FixMul (m_data.mat [UVEC][Z], m_data.mat [FVEC][X]) - FixMul (m_data.mat [UVEC][X], m_data.mat [FVEC][Z]), xDet);
m.m_data.mat [UVEC][Y] = FixDiv (FixMul (m_data.mat [RVEC][X], m_data.mat [FVEC][Z]) - FixMul (m_data.mat [RVEC][Z], m_data.mat [FVEC][X]), xDet);
m.m_data.mat [UVEC][Z] = FixDiv (FixMul (m_data.mat [RVEC][Z], m_data.mat [UVEC][X]) - FixMul (m_data.mat [RVEC][X], m_data.mat [UVEC][Z]), xDet);
m.m_data.mat [FVEC][X] = FixDiv (FixMul (m_data.mat [UVEC][X], m_data.mat [FVEC][Y]) - FixMul (m_data.mat [UVEC][Y], m_data.mat [FVEC][X]), xDet);
m.m_data.mat [FVEC][Y] = FixDiv (FixMul (m_data.mat [RVEC][Y], m_data.mat [FVEC][X]) - FixMul (m_data.mat [RVEC][X], m_data.mat [FVEC][Y]), xDet);
m.m_data.mat [FVEC][Z] = FixDiv (FixMul (m_data.mat [RVEC][X], m_data.mat [UVEC][Y]) - FixMul (m_data.mat [RVEC][Y], m_data.mat [UVEC][X]), xDet);
return m;
}

//------------------------------------------------------------------------------

CFixMatrix& CFixMatrix::Transpose (CFixMatrix& dest, CFixMatrix& src)
{
dest.m_data.vec [0] = src.m_data.vec [0];
dest.m_data.vec [3] = src.m_data.vec [1];
dest.m_data.vec [6] = src.m_data.vec [2];
dest.m_data.vec [1] = src.m_data.vec [3];
dest.m_data.vec [4] = src.m_data.vec [4];
dest.m_data.vec [7] = src.m_data.vec [5];
dest.m_data.vec [2] = src.m_data.vec [6];
dest.m_data.vec [5] = src.m_data.vec [7];
dest.m_data.vec [8] = src.m_data.vec [8];
return dest;
}

// -----------------------------------------------------------------------------

CFloatMatrix& CFixMatrix::Transpose (CFloatMatrix& dest, CFixMatrix& src)
{
dest [0] = X2F (src [0]);
dest [4] = X2F (src [1]);
dest [8] = X2F (src [2]);
dest [1] = X2F (src [3]);
dest [5] = X2F (src [4]);
dest [9] = X2F (src [5]);
dest [2] = X2F (src [6]);
dest [6] = X2F (src [7]);
dest [10] = X2F (src [8]);
dest [11] = dest [12] = dest [13] = dest [14] = 0;
dest [15] = 1.0f;
return dest;
}

// ----------------------------------------------------------------------------
//computes a matrix from the forward vector, a bank of
//zero is assumed.
//returns matrix.
const CFixMatrix CFixMatrix::CreateF (const CFixVector& fVec) {
	CFixMatrix m;
	CFixVector& xvec = m.m_data.mat [RVEC];
	CFixVector& yvec = m.m_data.mat [UVEC];
	CFixVector& zvec = m.m_data.mat [FVEC];

	zvec = fVec;
	CFixVector::Normalize (zvec);
	assert (zvec.Mag () != 0);

	//just forward vec
	if ((zvec [X] == 0) && (zvec [Z] == 0)) {		//forward vec is straight up or down
		m.m_data.mat [RVEC][X] = I2X (1);
		m.m_data.mat [UVEC][Z] = (zvec [Y] < 0) ? I2X (1) : -I2X (1);
		m.m_data.mat [RVEC][Y] = m.m_data.mat [RVEC][Z] = m.m_data.mat [UVEC][X] = m.m_data.mat [UVEC][Y] = 0;
	}
	else { 		//not straight up or down
		xvec [X] = zvec [Z];
		xvec [Y] = 0;
		xvec [Z] = -zvec [X];
		CFixVector::Normalize (xvec);
		yvec = CFixVector::Cross (zvec, xvec);
	}
	return m;
}


//computes a matrix from the forward and the up vector.
//returns matrix.
const CFixMatrix CFixMatrix::CreateFU (const CFixVector& fVec, const CFixVector& uVec) {
	CFixMatrix m;
	CFixVector& xvec = m.m_data.mat [RVEC];
	CFixVector& yvec = m.m_data.mat [UVEC];
	CFixVector& zvec = m.m_data.mat [FVEC];

	zvec = fVec;
	CFixVector::Normalize (zvec);
	assert (zvec.Mag () != 0);

	yvec = uVec;
	if (CFixVector::Normalize (yvec) == 0) {
		if ((zvec [X] == 0) && (zvec [Z] == 0)) {		//forward vec is straight up or down
			m.m_data.mat [RVEC][X] = I2X (1);
			m.m_data.mat [UVEC][Z] = (zvec [Y] < 0) ? I2X (1) : -I2X (1);
			m.m_data.mat [RVEC][Y] = m.m_data.mat [RVEC][Z] = m.m_data.mat [UVEC][X] = m.m_data.mat [UVEC][Y] = 0;
		}
		else { 		//not straight up or down
			xvec [X] = zvec [Z];
			xvec [Y] = 0;
			xvec [Z] = -zvec [X];
			CFixVector::Normalize (xvec);
			yvec = CFixVector::Cross (zvec, xvec);
		}
	}

	xvec = CFixVector::Cross (yvec, zvec);
	//Normalize new perpendicular vector
	if (CFixVector::Normalize (xvec) == 0) {
		if ((zvec [X] == 0) && (zvec [Z] == 0)) {		//forward vec is straight up or down
			m.m_data.mat [RVEC][X] = I2X (1);
			m.m_data.mat [UVEC][Z] = (zvec [Y] < 0) ? I2X (1) : -I2X (1);
			m.m_data.mat [RVEC][Y] = m.m_data.mat [RVEC][Z] = m.m_data.mat [UVEC][X] = m.m_data.mat [UVEC][Y] = 0;
		}
		else { 		//not straight up or down
			xvec [X] = zvec [Z];
			xvec [Y] = 0;
			xvec [Z] = -zvec [X];
			CFixVector::Normalize (xvec);
			yvec = CFixVector::Cross (zvec, xvec);
		}
	}

	//now recompute up vector, in case it wasn't entirely perpendiclar
	yvec = CFixVector::Cross (zvec, xvec);

	return m;
}


//computes a matrix from the forward and the right vector.
//returns matrix.
const CFixMatrix CFixMatrix::CreateFR (const CFixVector& fVec, const CFixVector& rVec) {
	CFixMatrix m;
	CFixVector& xvec = m.m_data.mat [RVEC];
	CFixVector& yvec = m.m_data.mat [UVEC];
	CFixVector& zvec = m.m_data.mat [FVEC];

zvec = fVec;
CFixVector::Normalize (zvec);
assert (zvec.Mag () != 0);

//use right vec
xvec = rVec;
if (CFixVector::Normalize (xvec) == 0) {
	if ((zvec [X] == 0) && (zvec [Z] == 0)) {		//forward vec is straight up or down
		m.m_data.mat [RVEC][X] = I2X (1);
		m.m_data.mat [UVEC][Z] = (zvec [Y] < 0) ? I2X (1) : -I2X (1);
		m.m_data.mat [RVEC][Y] = m.m_data.mat [RVEC][Z] = m.m_data.mat [UVEC][X] = m.m_data.mat [UVEC][Y] = 0;
		}
	else { 		//not straight up or down
		xvec [X] = zvec [Z];
		xvec [Y] = 0;
		xvec [Z] = -zvec [X];
		CFixVector::Normalize (xvec);
		yvec = CFixVector::Cross (zvec, xvec);
		}
	}

yvec = CFixVector::Cross (zvec, xvec);
//Normalize new perpendicular vector
if (CFixVector::Normalize (yvec) == 0) {
	if ((zvec [X] == 0) && (zvec [Z] == 0)) {		//forward vec is straight up or down
		m.m_data.mat [RVEC][X] = I2X (1);
		m.m_data.mat [UVEC][Z] = (zvec [Y] < 0) ? I2X (1) : -I2X (1);
		m.m_data.mat [RVEC][Y] = m.m_data.mat [RVEC][Z] = m.m_data.mat [UVEC][X] = m.m_data.mat [UVEC][Y] = 0;
		}
	else { 		//not straight up or down
		xvec [X] = zvec [Z];
		xvec [Y] = 0;
		xvec [Z] = -zvec [X];
		CFixVector::Normalize (xvec);
		yvec = CFixVector::Cross (zvec, xvec);
		}
	}
//now recompute right vector, in case it wasn't entirely perpendiclar
xvec = CFixVector::Cross (yvec, zvec);
return m;
}


//extract angles from a m_data.matrix
const CAngleVector CFixMatrix::ExtractAnglesVec (void) const 
{
	CAngleVector a;
	fix sinh, cosh, cosp;

if (m_data.mat [FVEC][X] == 0 && m_data.mat [FVEC][Z] == 0)		//zero head
	a [HA] = 0;
else
	a [HA] = FixAtan2 (m_data.mat [FVEC][Z], m_data.mat [FVEC][X]);
FixSinCos (a [HA], &sinh, &cosh);
if (abs (sinh) > abs (cosh))				//sine is larger, so use it
	cosp = FixDiv (m_data.mat [FVEC][X], sinh);
else											//cosine is larger, so use it
	cosp = FixDiv (m_data.mat [FVEC][Z], cosh);
if (cosp==0 && m_data.mat [FVEC][Y]==0)
	a [PA] = 0;
else
	a [PA] = FixAtan2 (cosp, -m_data.mat [FVEC][Y]);
if (cosp == 0)	//the cosine of pitch is zero.  we're pitched straight up. say no bank
	a [BA] = 0;
else {
	fix sinb, cosb;

	sinb = FixDiv (m_data.mat [RVEC][Y], cosp);
	cosb = FixDiv (m_data.mat [UVEC][Y], cosp);
	if (sinb==0 && cosb==0)
		a [BA] = 0;
	else
		a [BA] = FixAtan2 (cosb, sinb);
	}
return a;
}


inline int VmBehindPlane (const CFixVector& n, const CFixVector& p1, const CFixVector& p2, const CFixVector& i) {
	CFixVector	t;
#if DBG
	fix			d;
#endif

	t = p1 - p2;
#if DBG
	d = CFixVector::Dot (p1, t);
	return CFixVector::Dot (i, t) < d;
#else
	return CFixVector::Dot (i, t) - CFixVector::Dot (p1, t) < 0;
#endif
}

//	-----------------------------------------------------------------------------
// Find intersection of perpendicular on p1,p2 through p3 with p1,p2.
// If intersection is not between p1 and p2 and vPos is given, return
// further away point of p1 and p2 to vPos. Otherwise return intersection.
// returns 1 if intersection outside of p1,p2, otherwise 0.
const int VmPointLineIntersection (CFixVector& hitP, const CFixVector& p1, const CFixVector& p2, const CFixVector& p3,
									  int bClampToFarthest)
{
	CFixVector	d31, d21;
	float			m, u;
	int			bClamped = 0;

	d21 = p2 - p1;
	m = (float) fabs ((float) d21 [X] * (float) d21 [X] + (float) d21 [Y] * (float) d21 [Y] + (float) d21 [Z] * (float) d21 [Z]);
	if (!m) {
		//	if (hitP)
		hitP = p1;
		return 0;
	}
	d31 = p3 - p1;
	u = (float) d31 [X] * (float) d21 [X] + (float) d31 [Y] * (float) d21 [Y] + (float) d31 [Z] * (float) d21 [Z];
	u /= m;
	if (u < 0)
		bClamped = bClampToFarthest ? 2 : 1;
	else if (u > 1)
		bClamped = bClampToFarthest ? 1 : 2;
	else
		bClamped = 0;
	if (bClamped == 2)
		hitP = p2;
	else if (bClamped == 1)
		hitP = p1;
	else {
		hitP [X] = p1 [X] + (fix) (u * d21 [X]);
		hitP [Y] = p1 [Y] + (fix) (u * d21 [Y]);
		hitP [Z] = p1 [Z] + (fix) (u * d21 [Z]);

//		hitP = p1 + F2X (u) * d21;
	}
	return bClamped;
}

/*
int VmPointLineIntersection (CFixVector& hitP, const CFixVector& p1, const CFixVector& p2, const CFixVector& p3, const CFixVector& vPos,
									  int bClampToFarthest)
{
	CFixVector	d31, d21;
	double		m, u;
	int			bClamped = 0;

d21 = p2 - p1;
m = fabs (d21 [X] * d21 [X] + d21 [Y] * d21 [Y] + d21 [Z] * d21 [Z]);
if (!m) {
	hitP = p1;
	return 0;
}
d31 = p3 - p1;
u = d31 [X] * d21 [X] + d31 [Y] * d21 [Y] + d31 [Z] * d21 [Z];
u /= m;
if (u < 0)
	bClamped = bClampToFarthest ? 2 : 1;
else if (u > 1)
	bClamped = bClampToFarthest ? 1 : 2;
else
	bClamped = 0;
if (bClamped == 2)
	hitP = p2;
else if (bClamped == 1)
	hitP = p1;
else {
	hitP->p.x = p1->p.x + (fix) (u * d21.p.x);
	hitP->p.y = p1->p.y + (fix) (u * d21.p.y);
	hitP->p.z = p1->p.z + (fix) (u * d21.p.z);
	}
return bClamped;
}
*/
// ------------------------------------------------------------------------

// Version with vPos
const int VmPointLineIntersection (CFloatVector& hitP, const CFloatVector& p1, const CFloatVector& p2, const CFloatVector& p3, 
											  const CFloatVector& vPos, int bClamp) 
{
	CFloatVector	d31, d21;
	float		m, u;
	int		bClamped = 0;

d21 = p2 - p1;
m = (float) fabs (d21.SqrMag ());
if (!m) {
	hitP = p1;
	return 0;
	}
d31 = p3 - p1;
u = CFloatVector::Dot (d31, d21);
u /= m;
if (u < 0)
	bClamped = 2;
else if (u > 1)
	bClamped = 1;
else
	bClamped = 0;
// limit the intersection to [p1,p2]
if (bClamp && bClamped) {
	bClamped = (CFloatVector::Dist (vPos, p1) < CFloatVector::Dist (vPos, p2)) ? 2 : 1;
	hitP = (bClamped == 1) ? p1 : p2;
	}
else
	hitP = p1 + u * d21;
return bClamped;
}


// Version without vPos
const int VmPointLineIntersection (CFloatVector& hitP, const CFloatVector& p1, const CFloatVector& p2, const CFloatVector& p3, int bClamp) 
{
	CFloatVector	d31, d21;
	float		m, u;
	int		bClamped = 0;

d21 = p2 - p1;
m = (float) fabs (d21.SqrMag ());
if (!m) {
	hitP = p1;
	return 0;
	}
d31 = p3 - p1;
u = CFloatVector::Dot (d31, d21);
u /= m;
if (u < 0)
	bClamped = 2;
else if (u > 1)
	bClamped = 1;
else
	bClamped = 0;
// limit the intersection to [p1,p2]
if (bClamp && bClamped)
	hitP = (bClamped == 1) ? p1 : p2;
else
	hitP = p1 + u * d21;
return bClamped;
}

// ------------------------------------------------------------------------

const int VmPointLineIntersection (CFloatVector3& hitP, const CFloatVector3& p1, const CFloatVector3& p2, const CFloatVector3& p3, CFloatVector3 *vPos, int bClamp) 
{
	CFloatVector3	d31, d21;
	float		m, u;
	int		bClamped = 0;

	d21 = p2 - p1;
	m = (float) fabs (d21.SqrMag ());
	if (!m) {
	//	if (hitP)
		hitP = p1;
		return 0;
		}
	d31 = p3 - p1;
	u = CFloatVector3::Dot (d31, d21);
	u /= m;
	if (u < 0)
		bClamped = 2;
	else if (u > 1)
		bClamped = 1;
	else
		bClamped = 0;
	// limit the intersection to [p1,p2]
	//if (hitP) {
	if (bClamp && bClamped) {
		if (vPos)
			bClamped = (CFloatVector3::Dist (*vPos, p1) < CFloatVector3::Dist (*vPos, p2)) ? 2 : 1;
		hitP = (bClamped == 1) ? p1 : p2;
	}
	else {
/*
		hitP [X] = p1 [X] + (fix) (u * d21 [X]);
		hitP [Y] = p1 [Y] + (fix) (u * d21 [Y]);
		hitP [Z] = p1 [Z] + (fix) (u * d21 [Z]);
*/
		hitP = p1 + u * d21;
	}
	//	}
	return bClamped;
}

// ------------------------------------------------------------------------

const fix VmLinePointDist (const CFixVector& a, const CFixVector& b, const CFixVector& p) {
	CFixVector	h;

	VmPointLineIntersection (h, a, b, p, 0);
	return CFixVector::Dist (h, p);
}

// ------------------------------------------------------------------------

const float VmLinePointDist (const CFloatVector& a, const CFloatVector& b, const CFloatVector& p, int bClamp)
{
	CFloatVector	h;

	VmPointLineIntersection (h, a, b, p, bClamp);
	return CFloatVector::Dist (h, p);
}

// ------------------------------------------------------------------------

const float VmLinePointDist (const CFloatVector3& a, const CFloatVector3& b, const CFloatVector3& p, int bClamp)
{
	CFloatVector3	h;

	VmPointLineIntersection (h, a, b, p, NULL, bClamp);
	return CFloatVector3::Dist (h, p);
}

// --------------------------------------------------------------------------------------------------------------------

const float VmLineLineIntersection (const CFloatVector3& v1, const CFloatVector3& v2, const CFloatVector3& v3, const CFloatVector3& v4, CFloatVector3& va, CFloatVector3& vb) {
   CFloatVector3	v13, v43, v21;
   float		d1343, d4321, d1321, d4343, d2121;
   float		num, den, mua, mub;

v13 = v1 - v3;
v43 = v4 - v3;
if (v43.Mag () < 0.00001f) {
	va = vb = v4;
	return 0;
	}
v21 = v2 - v1;
if (v43.Mag () < 0.00001f)  {
	va = vb = v2;
	return 0;
	}
d1343 = CFloatVector3::Dot (v13, v43);
d4321 = CFloatVector3::Dot (v43, v21);
d1321 = CFloatVector3::Dot (v13, v21);
d4343 = CFloatVector3::Dot (v43, v43);
d2121 = CFloatVector3::Dot (v21, v21);
den = d2121 * d4343 - d4321 * d4321;
if (fabs (den) < 0.00001f) {
	va = CFloatVector3::Avg (v1, v2);
	vb = CFloatVector3::Avg (v3, v4);
	va = CFloatVector3::Avg (va, vb);
	vb = va;
	return 0;
	}
num = d1343 * d4321 - d1321 * d4343;
mua = num / den;
mub = (d1343 + d4321 * mua) / d4343;

va = v1 + mua * v21;
vb = v3 + mub * v43;

return CFloatVector3::Dist (va, vb);
}

// --------------------------------------------------------------------------------------------------------------------

const float VmLineLineIntersection (const CFloatVector& v1, const CFloatVector& v2, const CFloatVector& v3, const CFloatVector& v4, CFloatVector& va, CFloatVector& vb) {
   CFloatVector	v13, v43, v21;
   float		d1343, d4321, d1321, d4343, d2121;
   float		num, den, mua, mub;

v13 = v1 - v3;
v43 = v4 - v3;
if (v43.Mag () < 0.00001f) {
	va = vb = v4;
	return 0;
	}
v21 = v2 - v1;
if (v43.Mag () < 0.00001f)  {
	va = vb = v2;
	return 0;
	}
d1343 = CFloatVector::Dot (v13, v43);
d4321 = CFloatVector::Dot (v43, v21);
d1321 = CFloatVector::Dot (v13, v21);
d4343 = CFloatVector::Dot (v43, v43);
d2121 = CFloatVector::Dot (v21, v21);
den = d2121 * d4343 - d4321 * d4321;
if (fabs (den) < 0.00001f) {
	va = CFloatVector::Avg (v1, v2);
	vb = CFloatVector::Avg (v3, v4);
	va = CFloatVector::Avg (va, vb);
	vb = va;
	return 0;
	}
num = d1343 * d4321 - d1321 * d4343;
mua = num / den;
mub = (d1343 + d4321 * mua) / d4343;
/*
va->x () = v1->x () + mua * v21 [X];
va->y () = v1->y () + mua * v21 [Y];
va->z () = v1->z () + mua * v21 [Z];
vb->x () = v3->x () + mub * v43 [X];
vb->y () = v3->y () + mub * v43 [Y];
vb->z () = v3->z () + mub * v43 [Z];
*/
va = v1 + mua * v21;
vb = v3 + mub * v43;

return CFloatVector::Dist (va, vb);
}

// ------------------------------------------------------------------------
// Compute triangle size by multiplying length of perpendicular through
// longest triangle side and opposite point with length of longest triangle
// side. Divide by 2 * 20 * 20 (20 is the default side length which is
// interpreted as distance unit "1").

float TriangleSize (const CFixVector& p0, const CFixVector& p1, const CFixVector& p2) 
{
#if 1
	return X2F (CFixVector::Cross (p1 - p0, p2 - p0).Mag ()) / 800.0f;
#else
	fix			lMax, l, i = 0;

lMax = CFixVector::Dist (p0, p1);
l = CFixVector::Dist (p1, p2);
if (lMax < l) {
	lMax = l;
	i = 1;
	}
l = CFixVector::Dist (p2, p0);
if (lMax < l) {
	lMax = l;
	i = 2;
	}
if (i == 2)
	return X2F (lMax) * X2F (VmLinePointDist (p2, p0, p1)) / 800;
else if (i == 1)
	return X2F (lMax) * X2F (VmLinePointDist (p1, p2, p0)) / 800;
else
	return X2F (lMax) * X2F (VmLinePointDist (p0, p1, p2)) / 800;
#endif
}

// ------------------------------------------------------------------------

const CFixVector CFixVector::Random (void) 
{
	CFixVector v;
	int i = d_rand () % 3;

if (i == 2) {
	v [X] = (16384 - d_rand ()) | 1;	// make sure we don't create null vector
	v [Y] = 16384 - d_rand ();
	v [Z] = 16384 - d_rand ();
	}
else if (i == 1) {
	v [Y] = (16384 - d_rand ()) | 1;
	v [Z] = 16384 - d_rand ();
	v [X] = 16384 - d_rand ();	// make sure we don't create null vector
	}
else {
	v [Z] = (16384 - d_rand ()) | 1;
	v [X] = 16384 - d_rand ();	// make sure we don't create null vector
	v [Y] = 16384 - d_rand ();
	}
Normalize (v);
return v;
}

// ------------------------------------------------------------------------
// eof
