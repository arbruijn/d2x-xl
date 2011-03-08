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

fix CFixVector::Mag (void) const 
{
if (gameOpts->render.nMathFormat == 2)
	return fix (sqrt (double (v.c.x) * double (v.c.x) + double (v.c.y) * double (v.c.y) + double (v.c.z) * double (v.c.z)));
// The following code is vital for backwards compatibility: Side normals get computed after loading a level using this fixed
// math. D2X-XL temporarily switches to math format 0 to enforce that. Otherwise level checksums would differ from
// those of other Descent 2 versions, breaking multiplayer compatibility.
tQuadInt q = {0, 0};
FixMulAccum (&q, v.c.x, v.c.x);
FixMulAccum (&q, v.c.y, v.c.y);
FixMulAccum (&q, v.c.z, v.c.z);
return QuadSqrt (q.low, q.high);
}

// ------------------------------------------------------------------------

void CFixVector::Check (void)
{
fix check = labs (v.c.x) | labs(v.c.y) | labs(v.c.z);

if (check == 0)
	return;

int cnt = 0;

if (check & 0xfffc0000) {		//too big
	while (check & 0xfff00000) {
		cnt += 4;
		check >>= 4;
		}
	while (check & 0xfffc0000) {
		cnt += 2;
		check >>= 2;
		}
	v.c.x >>= cnt;
	v.c.y >>= cnt;
	v.c.z >>= cnt;
	}
else if ((check & 0xffff8000) == 0) {		//too small
	while ((check & 0xfffff000) == 0) {
		cnt += 4;
		check <<= 4;
		}
	while ((check & 0xffff8000) == 0) {
		cnt += 2;
		check <<= 2;
		}
	v.c.x >>= cnt;
	v.c.y >>= cnt;
	v.c.z >>= cnt;
	}
}

// ------------------------------------------------------------------------

CFixVector& CFixVector::Cross (CFixVector& dest, const CFixVector& v0, const CFixVector& v1)
{
#if 0
	double x = (double (v0.v.c.y) * double (v1.v.c.z) - double (v0.v.c.z) * double (v1.v.c.y)) / 65536.0;
	double y = (double (v0.v.c.z) * double (v1.v.c.x) - double (v0.v.c.x) * double (v1.v.c.z)) / 65536.0;
	double z = (double (v0.v.c.x) * double (v1.v.c.y) - double (v0.v.c.y) * double (v1.v.c.x)) / 65536.0;

if (x > double (0x7fffffff))
	x = double (0x7fffffff);
else if (x < double (-0x7fffffff))
	x = double (-0x7fffffff);
if (y > double (0x7fffffff))
	y = double (0x7fffffff);
else if (y < double (-0x7fffffff))
	y = double (-0x7fffffff);
if (z > double (0x7fffffff))
	z = double (0x7fffffff);
else if (z < double (-0x7fffffff))
	z = double (-0x7fffffff);

dest.Set (fix (x), fix (y), fix (z));
#else
QLONG x, y, z;

x = mul64 (v0.v.c.y, v1.v.c.z);
x += mul64 (-v0.v.c.z, v1.v.c.y);
y = mul64 (v0.v.c.z, v1.v.c.x);
y += mul64 (-v0.v.c.x, v1.v.c.z);
z = mul64 (v0.v.c.x, v1.v.c.y);
z += mul64 (-v0.v.c.y, v1.v.c.x);
dest.Set (fix (x / 65536), fix (y / 65536), fix (z / 65536));
#endif
return dest;
}

// ------------------------------------------------------------------------

fix FixQuadAdjust (QLONG q)
{
return fix ((q >> 32) <<16) + fix ((q & 0xffffffff) >>16);
}

const CFixVector CFixVector::Cross (const CFixVector& v0, const CFixVector& v1) 
{
#if 0
	double x = (double (v0.v.c.y) * double (v1.v.c.z) - double (v0.v.c.z) * double (v1.v.c.y)) / 65536.0;
	double y = (double (v0.v.c.z) * double (v1.v.c.x) - double (v0.v.c.x) * double (v1.v.c.z)) / 65536.0;
	double z = (double (v0.v.c.x) * double (v1.v.c.y) - double (v0.v.c.y) * double (v1.v.c.x)) / 65536.0;

if (x > double (0x7fffffff))
	x = double (0x7fffffff);
else if (x < double (-0x7fffffff))
	x = double (-0x7fffffff);
if (y > double (0x7fffffff))
	y = double (0x7fffffff);
else if (y < double (-0x7fffffff))
	y = double (-0x7fffffff);
if (z > double (0x7fffffff))
	z = double (0x7fffffff);
else if (z < double (-0x7fffffff))
	z = double (-0x7fffffff);

return Create (fix (x), fix (y), fix (z));
#else
QLONG x, y, z;

x = mul64 (v0.v.c.y, v1.v.c.z);
x += mul64 (-v0.v.c.z, v1.v.c.y);
y = mul64 (v0.v.c.z, v1.v.c.x);
y += mul64 (-v0.v.c.x, v1.v.c.z);
z = mul64 (v0.v.c.x, v1.v.c.y);
z += mul64 (-v0.v.c.y, v1.v.c.x);
return Create (FixQuadAdjust (x), FixQuadAdjust (y), FixQuadAdjust (z));
#endif
}

// ------------------------------------------------------------------------

#include <cmath>

#endif

// ------------------------------------------------------------------------

const CFloatMatrix CFloatMatrix::Create (const CFloatVector& r, const CFloatVector& u, const CFloatVector& f, const CFloatVector& w) {
	CFloatMatrix m;
	m.m.v.r = r;
	m.m.v.u = u;
	m.m.v.f = f;
	m.m.v.h = w;
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
	m.m.v.r.v.c.x = cbch + sinp * sbsh;	//m1
	m.m.v.u.v.c.z = sbsh + sinp * cbch;	//m8
	m.m.v.u.v.c.x = sinp * cbsh - sbch;	//m2
	m.m.v.r.v.c.z = sinp * sbch - cbsh;	//m7
	m.m.v.f.v.c.x = sinh * cosp;		//m3
	m.m.v.r.v.c.y = sinb * cosp;		//m4
	m.m.v.u.v.c.y = cosb * cosp;		//m5
	m.m.v.f.v.c.z = cosh * cosp;		//m9
	m.m.v.f.v.c.y = -sinp;				//m6
	return m;
}

// ------------------------------------------------------------------------

const float CFloatMatrix::Det (void) 
{
return m.v.r.v.c.x * (m.v.u.v.c.y * m.v.f.v.c.z - m.v.u.v.c.z * m.v.f.v.c.y) +
		 m.v.r.v.c.y * (m.v.u.v.c.z * m.v.f.v.c.x - m.v.u.v.c.x * m.v.f.v.c.z) +
		 m.v.r.v.c.z * (m.v.u.v.c.x * m.v.f.v.c.y - m.v.u.v.c.y * m.v.f.v.c.x);
}

// -----------------------------------------------------------------------------

const CFloatMatrix CFloatMatrix::Inverse (void) 
{
	float fDet = Det ();
	CFloatMatrix	m = *this;

m.m.v.r.v.c.x = (m.m.v.u.v.c.y * m.m.v.f.v.c.z - m.m.v.u.v.c.z * m.m.v.f.v.c.y) / fDet;
m.m.v.r.v.c.y = (m.m.v.r.v.c.z * m.m.v.f.v.c.y - m.m.v.r.v.c.y * m.m.v.f.v.c.z) / fDet;
m.m.v.r.v.c.z = (m.m.v.r.v.c.y * m.m.v.u.v.c.z - m.m.v.r.v.c.z * m.m.v.u.v.c.y) / fDet;
m.m.v.u.v.c.x = (m.m.v.u.v.c.z * m.m.v.f.v.c.x - m.m.v.u.v.c.x * m.m.v.f.v.c.z) / fDet;
m.m.v.u.v.c.y = (m.m.v.r.v.c.x * m.m.v.f.v.c.z - m.m.v.r.v.c.z * m.m.v.f.v.c.x) / fDet;
m.m.v.u.v.c.z = (m.m.v.r.v.c.z * m.m.v.u.v.c.x - m.m.v.r.v.c.x * m.m.v.u.v.c.z) / fDet;
m.m.v.f.v.c.x = (m.m.v.u.v.c.x * m.m.v.f.v.c.y - m.m.v.u.v.c.y * m.m.v.f.v.c.x) / fDet;
m.m.v.f.v.c.y = (m.m.v.r.v.c.y * m.m.v.f.v.c.x - m.m.v.r.v.c.x * m.m.v.f.v.c.y) / fDet;
m.m.v.f.v.c.z = (m.m.v.r.v.c.x * m.m.v.u.v.c.y - m.m.v.r.v.c.y * m.m.v.u.v.c.x) / fDet;
return m;
}

// -----------------------------------------------------------------------------

const CFloatMatrix CFloatMatrix::Mul (CFloatMatrix& other) 
{
CFloatVector v;
CFloatMatrix t;

v.v.c.x = m.v.r.v.c.x;
v.v.c.y = m.v.u.v.c.x;
v.v.c.z = m.v.f.v.c.x;
t.m.v.r.v.c.x = CFloatVector::Dot (v, other.m.v.r);
t.m.v.u.v.c.x = CFloatVector::Dot (v, other.m.v.u);
t.m.v.f.v.c.x = CFloatVector::Dot (v, other.m.v.f);
v.v.c.x = m.v.r.v.c.y;
v.v.c.y = m.v.u.v.c.y;
v.v.c.z = m.v.f.v.c.y;
t.m.v.r.v.c.y = CFloatVector::Dot (v, other.m.v.r);
t.m.v.u.v.c.y = CFloatVector::Dot (v, other.m.v.u);
t.m.v.f.v.c.y = CFloatVector::Dot (v, other.m.v.f);
v.v.c.x = m.v.r.v.c.z;
v.v.c.y = m.v.u.v.c.z;
v.v.c.z = m.v.f.v.c.z;
t.m.v.r.v.c.z = CFloatVector::Dot (v, other.m.v.r);
t.m.v.u.v.c.z = CFloatVector::Dot (v, other.m.v.u);
t.m.v.f.v.c.z = CFloatVector::Dot (v, other.m.v.f);
#if 0
CFloatVector::Normalize (m.m.v.r);
CFloatVector::Normalize (m.m.v.u);
CFloatVector::Normalize (m.m.v.f);
#endif
return t;
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
CFixMatrix t;

v.v.c.x = m.v.r.v.c.x;
v.v.c.y = m.v.u.v.c.x;
v.v.c.z = m.v.f.v.c.x;
t.m.v.r.v.c.x = CFixVector::Dot (v, other.m.v.r);
t.m.v.u.v.c.x = CFixVector::Dot (v, other.m.v.u);
t.m.v.f.v.c.x = CFixVector::Dot (v, other.m.v.f);
v.v.c.x = m.v.r.v.c.y;
v.v.c.y = m.v.u.v.c.y;
v.v.c.z = m.v.f.v.c.y;
t.m.v.r.v.c.y = CFixVector::Dot (v, other.m.v.r);
t.m.v.u.v.c.y = CFixVector::Dot (v, other.m.v.u);
t.m.v.f.v.c.y = CFixVector::Dot (v, other.m.v.f);
v.v.c.x = m.v.r.v.c.z;
v.v.c.y = m.v.u.v.c.z;
v.v.c.z = m.v.f.v.c.z;
t.m.v.r.v.c.z = CFixVector::Dot (v, other.m.v.r);
t.m.v.u.v.c.z = CFixVector::Dot (v, other.m.v.u);
t.m.v.f.v.c.z = CFixVector::Dot (v, other.m.v.f);
#if 0
CFixVector::Normalize (m.m.v.r);
CFixVector::Normalize (m.m.v.u);
CFixVector::Normalize (m.m.v.f);
#endif
return t;
}

// -----------------------------------------------------------------------------

const fix CFixMatrix::Det (void) 
{
fix xDet = FixMul (m.v.r.v.c.x, FixMul (m.v.u.v.c.y, m.v.f.v.c.z) - FixMul (m.v.u.v.c.z, m.v.f.v.c.y));
xDet += FixMul (m.v.r.v.c.y, FixMul (m.v.u.v.c.z, m.v.f.v.c.x) - FixMul (m.v.u.v.c.x, m.v.f.v.c.z));
xDet += FixMul (m.v.r.v.c.z, FixMul (m.v.u.v.c.x, m.v.f.v.c.y) - FixMul (m.v.u.v.c.y, m.v.f.v.c.x));
return xDet;
}

// -----------------------------------------------------------------------------

const CFixMatrix CFixMatrix::Inverse (void) 
{
	fix xDet = Det ();
	CFixMatrix i;

i.m.v.r.v.c.x = FixDiv (FixMul (m.v.u.v.c.y, m.v.f.v.c.z) - FixMul (m.v.u.v.c.z, m.v.f.v.c.y), xDet);
i.m.v.r.v.c.y = FixDiv (FixMul (m.v.r.v.c.z, m.v.f.v.c.y) - FixMul (m.v.r.v.c.y, m.v.f.v.c.z), xDet);
i.m.v.r.v.c.z = FixDiv (FixMul (m.v.r.v.c.y, m.v.u.v.c.z) - FixMul (m.v.r.v.c.z, m.v.u.v.c.y), xDet);
i.m.v.u.v.c.x = FixDiv (FixMul (m.v.u.v.c.z, m.v.f.v.c.x) - FixMul (m.v.u.v.c.x, m.v.f.v.c.z), xDet);
i.m.v.u.v.c.y = FixDiv (FixMul (m.v.r.v.c.x, m.v.f.v.c.z) - FixMul (m.v.r.v.c.z, m.v.f.v.c.x), xDet);
i.m.v.u.v.c.z = FixDiv (FixMul (m.v.r.v.c.z, m.v.u.v.c.x) - FixMul (m.v.r.v.c.x, m.v.u.v.c.z), xDet);
i.m.v.f.v.c.x = FixDiv (FixMul (m.v.u.v.c.x, m.v.f.v.c.y) - FixMul (m.v.u.v.c.y, m.v.f.v.c.x), xDet);
i.m.v.f.v.c.y = FixDiv (FixMul (m.v.r.v.c.y, m.v.f.v.c.x) - FixMul (m.v.r.v.c.x, m.v.f.v.c.y), xDet);
i.m.v.f.v.c.z = FixDiv (FixMul (m.v.r.v.c.x, m.v.u.v.c.y) - FixMul (m.v.r.v.c.y, m.v.u.v.c.x), xDet);
return i;
}

//------------------------------------------------------------------------------

CFixMatrix& CFixMatrix::Transpose (CFixMatrix& dest, CFixMatrix& src)
{
dest.m.a [0] = src.m.a [0];
dest.m.a [3] = src.m.a [1];
dest.m.a [6] = src.m.a [2];
dest.m.a [1] = src.m.a [3];
dest.m.a [4] = src.m.a [4];
dest.m.a [7] = src.m.a [5];
dest.m.a [2] = src.m.a [6];
dest.m.a [5] = src.m.a [7];
dest.m.a [8] = src.m.a [8];
return dest;
}

// -----------------------------------------------------------------------------

CFloatMatrix& CFixMatrix::Transpose (CFloatMatrix& dest, CFixMatrix& src)
{
dest.m.a [0] = X2F (src.m.a [0]);
dest.m.a [4] = X2F (src.m.a [1]);
dest.m.a [8] = X2F (src.m.a [2]);
dest.m.a [1] = X2F (src.m.a [3]);
dest.m.a [5] = X2F (src.m.a [4]);
dest.m.a [9] = X2F (src.m.a [5]);
dest.m.a [2] = X2F (src.m.a [6]);
dest.m.a [6] = X2F (src.m.a [7]);
dest.m.a [10] = X2F (src.m.a [8]);
dest.m.a [11] = dest.m.a [12] = dest.m.a [13] = dest.m.a [14] = 0;
dest.m.a [15] = 1.0f;
return dest;
}

// ----------------------------------------------------------------------------
//computes a matrix from the forward vector, a bank of
//zero is assumed.
//returns matrix.
const CFixMatrix CFixMatrix::CreateF (const CFixVector& fVec) 
{
	CFixMatrix m;

	m.m.v.f = fVec;
	CFixVector::Normalize (m.m.v.f);
	assert (m.m.v.f.Mag () != 0);

	//just forward vec
	if ((m.m.v.f.v.c.x == 0) && (m.m.v.f.v.c.z == 0)) {		//forward vec is straight up or down
		m.m.v.r.v.c.x = I2X (1);
		m.m.v.u.v.c.z = (m.m.v.f.v.c.z < 0) ? I2X (1) : -I2X (1);
		m.m.v.r.v.c.y = m.m.v.r.v.c.z = m.m.v.u.v.c.x = m.m.v.u.v.c.y = 0;
	}
	else { 		//not straight up or down
		m.m.v.r.v.c.x = m.m.v.f.v.c.z;
		m.m.v.r.v.c.y = 0;
		m.m.v.r.v.c.z = -m.m.v.f.v.c.x;
		CFixVector::Normalize (m.m.v.r);
		m.m.v.u = CFixVector::Cross (m.m.v.f, m.m.v.r);
	}
	return m;
}

//	-----------------------------------------------------------------------------
//computes a matrix from the forward and the up vector.
//returns matrix.
const CFixMatrix CFixMatrix::CreateFU (const CFixVector& fVec, const CFixVector& uVec) 
{
	CFixMatrix	m;

m.m.v.f = fVec;
CFixVector::Normalize (m.m.v.f);
assert (m.m.v.f.Mag () != 0);

m.m.v.u = uVec;
if (CFixVector::Normalize (m.m.v.u) == 0) {
	if ((m.m.v.f.v.c.x == 0) && (m.m.v.f.v.c.z == 0)) {		//forward vec is straight up or down
		m.m.v.r.v.c.x = I2X (1);
		m.m.v.u.v.c.z = (m.m.v.f.v.c.y < 0) ? I2X (1) : -I2X (1);
		m.m.v.r.v.c.y = m.m.v.r.v.c.z = m.m.v.u.v.c.x = m.m.v.u.v.c.y = 0;
		}
	else { 		//not straight up or down
		m.m.v.r.v.c.x = m.m.v.f.v.c.z;
		m.m.v.r.v.c.y = 0;
		m.m.v.r.v.c.z = -m.m.v.f.v.c.x;
		CFixVector::Normalize (m.m.v.r);
		m.m.v.u = CFixVector::Cross (m.m.v.f, m.m.v.r);
		}
	}
m.m.v.r = CFixVector::Cross (m.m.v.u, m.m.v.f);
//Normalize new perpendicular vector
if (CFixVector::Normalize (m.m.v.r) == 0) {
	if ((m.m.v.f.v.c.x == 0) && (m.m.v.f.v.c.z == 0)) {		//forward vec is straight up or down
		m.m.v.r.v.c.x = I2X (1);
		m.m.v.u.v.c.z = (m.m.v.f.v.c.y < 0) ? I2X (1) : -I2X (1);
		m.m.v.r.v.c.y = 
		m.m.v.r.v.c.z = 
		m.m.v.u.v.c.x = 
		m.m.v.u.v.c.y = 0;
		}
	else { 		//not straight up or down
		m.m.v.r.v.c.x = m.m.v.f.v.c.z;
		m.m.v.r.v.c.y = 0;
		m.m.v.r.v.c.z = -m.m.v.f.v.c.x;
		CFixVector::Normalize (m.m.v.r);
		m.m.v.u = CFixVector::Cross (m.m.v.f, m.m.v.r);
		}
	}

//	-----------------------------------------------------------------------------
//now recompute up vector, in case it wasn't entirely perpendiclar
m.m.v.u = CFixVector::Cross (m.m.v.f, m.m.v.r);
return m;
}

//	-----------------------------------------------------------------------------
//computes a matrix from the forward and the right vector.
//returns matrix.
const CFixMatrix CFixMatrix::CreateFR (const CFixVector& fVec, const CFixVector& rVec) {
	CFixMatrix m;

m.m.v.f = fVec;
CFixVector::Normalize (m.m.v.f);
assert (m.m.v.f.Mag () != 0);

//use right vec
m.m.v.r = rVec;
if (CFixVector::Normalize (m.m.v.r) == 0) {
	if ((m.m.v.f.v.c.x == 0) && (m.m.v.f.v.c.z == 0)) {		//forward vec is straight up or down
		m.m.v.r.v.c.x = I2X (1);
		m.m.v.u.v.c.z = (m.m.v.f.v.c.y < 0) ? I2X (1) : -I2X (1);
		m.m.v.r.v.c.y = 
		m.m.v.r.v.c.z = 
		m.m.v.u.v.c.x = 
		m.m.v.u.v.c.y = 0;
		}
	else { 		//not straight up or down
		m.m.v.r.v.c.x = m.m.v.f.v.c.z;
		m.m.v.r.v.c.y = 0;
		m.m.v.r.v.c.z = -m.m.v.f.v.c.x;
		CFixVector::Normalize (m.m.v.r);
		m.m.v.u = CFixVector::Cross (m.m.v.f, m.m.v.r);
		}
	}

m.m.v.u = CFixVector::Cross (m.m.v.f, m.m.v.r);
//Normalize new perpendicular vector
if (CFixVector::Normalize (m.m.v.u) == 0) {
	if ((m.m.v.f.v.c.x == 0) && (m.m.v.f.v.c.z == 0)) {		//forward vec is straight up or down
		m.m.v.r.v.c.x = I2X (1);
		m.m.v.u.v.c.z = (m.m.v.f.v.c.y < 0) ? I2X (1) : -I2X (1);
		m.m.v.r.v.c.y = m.m.v.r.v.c.z = m.m.v.u.v.c.x = m.m.v.u.v.c.y = 0;
		}
	else { 		//not straight up or down
		m.m.v.r.v.c.x = m.m.v.f.v.c.z;
		m.m.v.r.v.c.y = 0;
		m.m.v.r.v.c.z = -m.m.v.f.v.c.x;
		CFixVector::Normalize (m.m.v.r);
		m.m.v.u = CFixVector::Cross (m.m.v.f, m.m.v.r);
		}
	}
//now recompute right vector, in case it wasn't entirely perpendiclar
m.m.v.r = CFixVector::Cross (m.m.v.u, m.m.v.f);
return m;
}

//	-----------------------------------------------------------------------------
//computes a matrix from the forward and the up vector.
//returns matrix.
const CFloatMatrix CFloatMatrix::CreateFU (const CFloatVector& fVec, const CFloatVector& uVec)
{
	CFloatMatrix m;

m.m.v.f = fVec;
CFloatVector::Normalize (m.m.v.f);
assert (m.m.v.f.Mag () != 0);

m.m.v.u = uVec;
if (CFloatVector::Normalize (m.m.v.u) == 0) {
	if ((m.m.v.f.v.c.x == 0) && (m.m.v.f.v.c.z == 0)) {		//forward vec is straight up or down
		m.m.v.r.v.c.x = 1;
		m.m.v.u.v.c.z = (m.m.v.f.v.c.y < 0) ? 1.0f : -1.0f;
		m.m.v.r.v.c.y = m.m.v.r.v.c.z = m.m.v.u.v.c.x = m.m.v.u.v.c.y = 0;
		}
	else { 		//not straight up or down
		m.m.v.r.v.c.x = m.m.v.f.v.c.z;
		m.m.v.r.v.c.y = 0;
		m.m.v.r.v.c.z = -m.m.v.f.v.c.x;
		CFloatVector::Normalize (m.m.v.r);
		m.m.v.u = CFloatVector::Cross (m.m.v.f, m.m.v.r);
		}
	}
m.m.v.r = CFloatVector::Cross (m.m.v.u, m.m.v.f);
//Normalize new perpendicular vector
if (CFloatVector::Normalize (m.m.v.r) == 0) {
	if ((m.m.v.f.v.c.x == 0) && (m.m.v.f.v.c.z == 0)) {		//forward vec is straight up or down
		m.m.v.r.v.c.x = 1;
		m.m.v.u.v.c.z = (m.m.v.f.v.c.y < 0) ? 1.0f : -1.0f;
		m.m.v.r.v.c.y = m.m.v.r.v.c.z = m.m.v.u.v.c.x = m.m.v.u.v.c.y = 0;
	}
	else { 		//not straight up or down
		m.m.v.r.v.c.x = m.m.v.f.v.c.z;
		m.m.v.r.v.c.y = 0;
		m.m.v.r.v.c.z = -m.m.v.f.v.c.x;
		CFloatVector::Normalize (m.m.v.r);
		m.m.v.u = CFloatVector::Cross (m.m.v.f, m.m.v.r);
		}
	}
//now recompute up vector, in case it wasn't entirely perpendiclar
m.m.v.u = CFloatVector::Cross (m.m.v.f, m.m.v.r);
return m;
}

//	-----------------------------------------------------------------------------
//computes a matrix from the forward and the right vector.
//returns matrix.
const CFloatMatrix CFloatMatrix::CreateFR (const CFloatVector& fVec, const CFloatVector& rVec) 
{
	CFloatMatrix m;

m.m.v.f = fVec;
CFloatVector::Normalize (m.m.v.f);
assert (m.m.v.f.Mag () != 0);

//use right vec
m.m.v.r = rVec;
if (CFloatVector::Normalize (m.m.v.r) == 0) {
	if ((m.m.v.f.v.c.x == 0) && (m.m.v.f.v.c.z == 0)) {		//forward vec is straight up or down
		m.m.v.r.v.c.x = 1;
		m.m.v.u.v.c.z = (m.m.v.f.v.c.y < 0) ? 1.0f : -1.0f;
		m.m.v.r.v.c.y = m.m.v.r.v.c.z = m.m.v.u.v.c.x = m.m.v.u.v.c.y = 0;
		}
	else { 		//not straight up or down
		m.m.v.r.v.c.x = m.m.v.f.v.c.z;
		m.m.v.r.v.c.y = 0;
		m.m.v.r.v.c.z = -m.m.v.f.v.c.x;
		CFloatVector::Normalize (m.m.v.r);
		m.m.v.u = CFloatVector::Cross (m.m.v.f, m.m.v.r);
		}
	}

m.m.v.u = CFloatVector::Cross (m.m.v.f, m.m.v.r);
//Normalize new perpendicular vector
if (CFloatVector::Normalize (m.m.v.u) == 0) {
	if ((m.m.v.f.v.c.x == 0) && (m.m.v.f.v.c.z == 0)) {		//forward vec is straight up or down
		m.m.v.r.v.c.x = 1;
		m.m.v.u.v.c.z = (m.m.v.f.v.c.y < 0) ? 1.0f : -1.0f;
		m.m.v.r.v.c.y = m.m.v.r.v.c.z = m.m.v.u.v.c.x = m.m.v.u.v.c.y = 0;
		}
	else { 		//not straight up or down
		m.m.v.r.v.c.x = m.m.v.f.v.c.z;
		m.m.v.r.v.c.y = 0;
		m.m.v.r.v.c.z = -m.m.v.f.v.c.x;
		CFloatVector::Normalize (m.m.v.r);
		m.m.v.u = CFloatVector::Cross (m.m.v.f, m.m.v.r);
		}
	}
//now recompute right vector, in case it wasn't entirely perpendiclar
m.m.v.r = CFloatVector::Cross (m.m.v.u, m.m.v.f);
return m;
}

//	-----------------------------------------------------------------------------
//extract angles from a m.matrix
const CAngleVector CFixMatrix::ExtractAnglesVec (void) const 
{
	CAngleVector a;
	fix sinh, cosh, cosp;

if ((m.v.f.v.c.x == 0) && (m.v.f.v.c.z == 0))		//zero head
	a.v.c.h = 0;
else
	a.v.c.h = FixAtan2 (m.v.f.v.c.z, m.v.f.v.c.x);
FixSinCos (a.v.c.h, &sinh, &cosh);
if (abs (sinh) > abs (cosh))				//sine is larger, so use it
	cosp = FixDiv (m.v.f.v.c.x, sinh);
else											//cosine is larger, so use it
	cosp = FixDiv (m.v.f.v.c.z, cosh);
if (cosp==0 && m.v.f.v.c.y==0)
	a.v.c.p = 0;
else
	a.v.c.p = FixAtan2 (cosp, -m.v.f.v.c.y);
if (cosp == 0)	//the cosine of pitch is zero.  we're pitched straight up. say no bank
	a.v.c.b = 0;
else {
	fix sinb, cosb;

	sinb = FixDiv (m.v.r.v.c.y, cosp);
	cosb = FixDiv (m.v.u.v.c.y, cosp);
	if (sinb==0 && cosb==0)
		a.v.c.b = 0;
	else
		a.v.c.b = FixAtan2 (cosb, sinb);
	}
return a;
}

//	-----------------------------------------------------------------------------

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
const int VmPointLineIntersection (CFixVector& hitP, const CFixVector& p1, const CFixVector& p2, const CFixVector& p3, int bClampToFarthest)
{
	CFixVector	d31, d21;
	double		m, u;
	int			bClamped = 0;

d21 = p2 - p1;
m = fabs (double (d21.v.c.x) * double (d21.v.c.x) + double (d21.v.c.y) * double (d21.v.c.y) + double (d21.v.c.z) * double (d21.v.c.z));
if (!m) {
	//	if (hitP)
	hitP = p1;
	return 0;
}
d31 = p3 - p1;
u = double (d31.v.c.x) * double (d21.v.c.x) + double (d31.v.c.y) * double (d21.v.c.y) + double (d31.v.c.z) * double (d21.v.c.z);
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
	hitP.v.c.x = p1.v.c.x + (fix) (u * d21.v.c.x);
	hitP.v.c.y = p1.v.c.y + (fix) (u * d21.v.c.y);
	hitP.v.c.z = p1.v.c.z + (fix) (u * d21.v.c.z);
	}
return bClamped;
}

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
	bClamped = 1;
else if (u > 1)
	bClamped = 2;
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
m = (float) fabs (d21.SqrMag ());	// Dot (d21, d21)
if (!m) {
	hitP = p1;
	return 0;
	}
d31 = p3 - p1;
u = CFloatVector::Dot (d31, d21);
u /= m;
if (u < 0)
	bClamped = 1;
else if (u > 1)
	bClamped = 2;
else
	bClamped = 0;
// limit the intersection to [p1,p2]
if (bClamp && bClamped)
	hitP = (bClamped == 1) ? p1 : p2; //(CFloatVector::Dist (p3, p1) < CFloatVector::Dist (p3, p2)) ? p1 : p2;
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
	hitP = p1;
	return 0;
	}
d31 = p3 - p1;
u = CFloatVector3::Dot (d31, d21);
u /= m;
if (u < 0)
	bClamped = 1;
else if (u > 1)
	bClamped = 2;
else
	bClamped = 0;
// limit the intersection to [p1,p2]
if (bClamp && bClamped) {
	if (vPos)
		bClamped = (CFloatVector3::Dist (*vPos, p1) < CFloatVector3::Dist (*vPos, p2)) ? 2 : 1;
	hitP = (bClamped == 1) ? p1 : p2;
	}
else {
	hitP = p1 + u * d21;
	}
return bClamped;
}

// ------------------------------------------------------------------------

const fix VmLinePointDist (const CFixVector& a, const CFixVector& b, const CFixVector& p) 
{
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

const float VmLineLineIntersection (const CFloatVector3& v1, const CFloatVector3& v2, const CFloatVector3& v3, 
												const CFloatVector3& v4, CFloatVector3& va, CFloatVector3& vb) 
{
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

const float VmLineLineIntersection (const CFloatVector& v1, const CFloatVector& v2, const CFloatVector& v3, 
												const CFloatVector& v4, CFloatVector& va, CFloatVector& vb) 
{
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
va->x () = v1->x () + mua * v21.v.c.x;
va->y () = v1->y () + mua * v21.v.c.y;
va->z () = v1->z () + mua * v21.v.c.z;
vb->x () = v3->x () + mub * v43.v.c.x;
vb->y () = v3->y () + mub * v43.v.c.y;
vb->z () = v3->z () + mub * v43.v.c.z;
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
	CFloatVector	pa, pb;
	pa.Assign (p1 - p0);
	pb.Assign (p2 - p0);
#	if DBG
	CFloatVector px = CFloatVector::Cross (pa, pb);
	float m = px.Mag ();
	float s = m / 800.0f;
	return s;
#	else
	return CFloatVector::Cross (pa, pb).Mag () / 800.0f;
#	endif
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
	v.v.c.x = (16384 - d_rand ()) | 1;	// make sure we don't create null vector
	v.v.c.y = 16384 - d_rand ();
	v.v.c.z = 16384 - d_rand ();
	}
else if (i == 1) {
	v.v.c.y = (16384 - d_rand ()) | 1;
	v.v.c.z = 16384 - d_rand ();
	v.v.c.x = 16384 - d_rand ();	// make sure we don't create null vector
	}
else {
	v.v.c.z = (16384 - d_rand ()) | 1;
	v.v.c.x = 16384 - d_rand ();	// make sure we don't create null vector
	v.v.c.y = 16384 - d_rand ();
	}
Normalize (v);
return v;
}

//------------------------------------------------------------------------------

CFloatVector *VmsReflect (CFloatVector *vReflect, CFloatVector *vLight, CFloatVector *vNormal)
{
*vReflect = *vNormal * 2 * CFloatVector::Dot (*vLight, *vNormal) - *vLight;
return vReflect;
}

// ------------------------------------------------------------------------

float VmLineEllipsoidDistance (CFloatVector vOrigin,	// origin of ray 
										 CFloatVector vDir,		// direction of ray
										 CFloatVector vCenter,	// center of ellipsoid
										 CFloatVector vScale)	//	axes of ellipsoid
{
CFloatVector vLine = vOrigin - vCenter;

if (vDir.IsZero ())
	vDir = vCenter - vOrigin;

float a = vDir.v.c.x * vDir.v.c.x / vScale.v.c.x +
			 vDir.v.c.y * vDir.v.c.y / vScale.v.c.y +
			 vDir.v.c.z * vDir.v.c.z / vScale.v.c.z;

float b = (vLine.v.c.x * vDir.v.c.x / vScale.v.c.x +
			  vLine.v.c.y * vDir.v.c.y / vScale.v.c.y +
			  vLine.v.c.z * vDir.v.c.z / vScale.v.c.z) * 2.0f;

float c = vLine.v.c.x * vLine.v.c.x / vScale.v.c.x +
			 vLine.v.c.y * vLine.v.c.y / vScale.v.c.y +
			 vLine.v.c.z * vLine.v.c.z / vScale.v.c.z - 1.0f;

float det = b * b - 4.0f * a * c;

if (det < 0.0f)
	return 1e30f;	// no intersection

det = float (sqrt (det));
a = 1.0f / (2.0f * a);
float s1 = (-b + det) * a;
float s2 = (-b - det) * a;
return (s1 < s2) ? s1 : s2;
return true;
}

// ------------------------------------------------------------------------
// eof
