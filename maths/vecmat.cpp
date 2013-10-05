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
	return fix (sqrt (double (v.coord.x) * double (v.coord.x) + double (v.coord.y) * double (v.coord.y) + double (v.coord.z) * double (v.coord.z)));
// The following code is vital for backwards compatibility: Side normals get computed after loading a level using this fixed
// math. D2X-XL temporarily switches to math format 0 to enforce that. Otherwise level checksums would differ from
// those of other Descent 2 versions, breaking multiplayer compatibility.
tQuadInt q = {0, 0};
FixMulAccum (&q, v.coord.x, v.coord.x);
FixMulAccum (&q, v.coord.y, v.coord.y);
FixMulAccum (&q, v.coord.z, v.coord.z);
return QuadSqrt (q.low, q.high);
}

// ------------------------------------------------------------------------

void CFixVector::Check (void)
{
fix check = labs (v.coord.x) | labs(v.coord.y) | labs(v.coord.z);

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
	v.coord.x >>= cnt;
	v.coord.y >>= cnt;
	v.coord.z >>= cnt;
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
	v.coord.x >>= cnt;
	v.coord.y >>= cnt;
	v.coord.z >>= cnt;
	}
}

// ------------------------------------------------------------------------

CFixVector& CFixVector::Cross (CFixVector& dest, const CFixVector& v0, const CFixVector& v1)
{
#if 0
	double x = (double (v0.dir.coord.y) * double (v1.dir.coord.z) - double (v0.dir.coord.z) * double (v1.dir.coord.y)) / 65536.0;
	double y = (double (v0.dir.coord.z) * double (v1.dir.coord.x) - double (v0.dir.coord.x) * double (v1.dir.coord.z)) / 65536.0;
	double z = (double (v0.dir.coord.x) * double (v1.dir.coord.y) - double (v0.dir.coord.y) * double (v1.dir.coord.x)) / 65536.0;

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

x = mul64 (v0.v.coord.y, v1.v.coord.z);
x += mul64 (-v0.v.coord.z, v1.v.coord.y);
y = mul64 (v0.v.coord.z, v1.v.coord.x);
y += mul64 (-v0.v.coord.x, v1.v.coord.z);
z = mul64 (v0.v.coord.x, v1.v.coord.y);
z += mul64 (-v0.v.coord.y, v1.v.coord.x);
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
	double x = (double (v0.dir.coord.y) * double (v1.dir.coord.z) - double (v0.dir.coord.z) * double (v1.dir.coord.y)) / 65536.0;
	double y = (double (v0.dir.coord.z) * double (v1.dir.coord.x) - double (v0.dir.coord.x) * double (v1.dir.coord.z)) / 65536.0;
	double z = (double (v0.dir.coord.x) * double (v1.dir.coord.y) - double (v0.dir.coord.y) * double (v1.dir.coord.x)) / 65536.0;

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

x = mul64 (v0.v.coord.y, v1.v.coord.z);
x += mul64 (-v0.v.coord.z, v1.v.coord.y);
y = mul64 (v0.v.coord.z, v1.v.coord.x);
y += mul64 (-v0.v.coord.x, v1.v.coord.z);
z = mul64 (v0.v.coord.x, v1.v.coord.y);
z += mul64 (-v0.v.coord.y, v1.v.coord.x);
return Create (FixQuadAdjust (x), FixQuadAdjust (y), FixQuadAdjust (z));
#endif
}

// ------------------------------------------------------------------------

#include <cmath>

#endif

// ------------------------------------------------------------------------

const CFloatMatrix CFloatMatrix::Create (const CFloatVector& r, const CFloatVector& u, const CFloatVector& f, const CFloatVector& w) {
	CFloatMatrix m;
	m.m.dir.r = r;
	m.m.dir.u = u;
	m.m.dir.f = f;
	m.m.dir.h = w;
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
	m.m.dir.r.v.coord.x = cbch + sinp * sbsh;	//m1
	m.m.dir.u.v.coord.z = sbsh + sinp * cbch;	//m8
	m.m.dir.u.v.coord.x = sinp * cbsh - sbch;	//m2
	m.m.dir.r.v.coord.z = sinp * sbch - cbsh;	//m7
	m.m.dir.f.v.coord.x = sinh * cosp;		//m3
	m.m.dir.r.v.coord.y = sinb * cosp;		//m4
	m.m.dir.u.v.coord.y = cosb * cosp;		//m5
	m.m.dir.f.v.coord.z = cosh * cosp;		//m9
	m.m.dir.f.v.coord.y = -sinp;				//m6
	return m;
}

// ------------------------------------------------------------------------

const float CFloatMatrix::Det (void) 
{
return m.dir.r.v.coord.x * (m.dir.u.v.coord.y * m.dir.f.v.coord.z - m.dir.u.v.coord.z * m.dir.f.v.coord.y) +
		 m.dir.r.v.coord.y * (m.dir.u.v.coord.z * m.dir.f.v.coord.x - m.dir.u.v.coord.x * m.dir.f.v.coord.z) +
		 m.dir.r.v.coord.z * (m.dir.u.v.coord.x * m.dir.f.v.coord.y - m.dir.u.v.coord.y * m.dir.f.v.coord.x);
}

// -----------------------------------------------------------------------------

const CFloatMatrix CFloatMatrix::Inverse (void) 
{
	float fDet = Det ();
	CFloatMatrix	m = *this;

m.m.dir.r.v.coord.x = (m.m.dir.u.v.coord.y * m.m.dir.f.v.coord.z - m.m.dir.u.v.coord.z * m.m.dir.f.v.coord.y) / fDet;
m.m.dir.r.v.coord.y = (m.m.dir.r.v.coord.z * m.m.dir.f.v.coord.y - m.m.dir.r.v.coord.y * m.m.dir.f.v.coord.z) / fDet;
m.m.dir.r.v.coord.z = (m.m.dir.r.v.coord.y * m.m.dir.u.v.coord.z - m.m.dir.r.v.coord.z * m.m.dir.u.v.coord.y) / fDet;
m.m.dir.u.v.coord.x = (m.m.dir.u.v.coord.z * m.m.dir.f.v.coord.x - m.m.dir.u.v.coord.x * m.m.dir.f.v.coord.z) / fDet;
m.m.dir.u.v.coord.y = (m.m.dir.r.v.coord.x * m.m.dir.f.v.coord.z - m.m.dir.r.v.coord.z * m.m.dir.f.v.coord.x) / fDet;
m.m.dir.u.v.coord.z = (m.m.dir.r.v.coord.z * m.m.dir.u.v.coord.x - m.m.dir.r.v.coord.x * m.m.dir.u.v.coord.z) / fDet;
m.m.dir.f.v.coord.x = (m.m.dir.u.v.coord.x * m.m.dir.f.v.coord.y - m.m.dir.u.v.coord.y * m.m.dir.f.v.coord.x) / fDet;
m.m.dir.f.v.coord.y = (m.m.dir.r.v.coord.y * m.m.dir.f.v.coord.x - m.m.dir.r.v.coord.x * m.m.dir.f.v.coord.y) / fDet;
m.m.dir.f.v.coord.z = (m.m.dir.r.v.coord.x * m.m.dir.u.v.coord.y - m.m.dir.r.v.coord.y * m.m.dir.u.v.coord.x) / fDet;
return m;
}

// -----------------------------------------------------------------------------

const CFloatMatrix CFloatMatrix::Mul (CFloatMatrix& other) 
{
CFloatVector v;
CFloatMatrix t;

v.v.coord.x = m.dir.r.v.coord.x;
v.v.coord.y = m.dir.u.v.coord.x;
v.v.coord.z = m.dir.f.v.coord.x;
t.m.dir.r.v.coord.x = CFloatVector::Dot (v, other.m.dir.r);
t.m.dir.u.v.coord.x = CFloatVector::Dot (v, other.m.dir.u);
t.m.dir.f.v.coord.x = CFloatVector::Dot (v, other.m.dir.f);
v.v.coord.x = m.dir.r.v.coord.y;
v.v.coord.y = m.dir.u.v.coord.y;
v.v.coord.z = m.dir.f.v.coord.y;
t.m.dir.r.v.coord.y = CFloatVector::Dot (v, other.m.dir.r);
t.m.dir.u.v.coord.y = CFloatVector::Dot (v, other.m.dir.u);
t.m.dir.f.v.coord.y = CFloatVector::Dot (v, other.m.dir.f);
v.v.coord.x = m.dir.r.v.coord.z;
v.v.coord.y = m.dir.u.v.coord.z;
v.v.coord.z = m.dir.f.v.coord.z;
t.m.dir.r.v.coord.z = CFloatVector::Dot (v, other.m.dir.r);
t.m.dir.u.v.coord.z = CFloatVector::Dot (v, other.m.dir.u);
t.m.dir.f.v.coord.z = CFloatVector::Dot (v, other.m.dir.f);
#if 0
CFloatVector::Normalize (m.m.dir.r);
CFloatVector::Normalize (m.m.dir.u);
CFloatVector::Normalize (m.m.dir.f);
#endif
return t;
}

//------------------------------------------------------------------------------

CFloatMatrix& CFloatMatrix::Transpose (CFloatMatrix& dest, CFloatMatrix& src)
{
dest.m.vec [0] = src.m.vec [0];
dest.m.vec [4] = src.m.vec [1];
dest.m.vec [8] = src.m.vec [2];
dest.m.vec [1] = src.m.vec [4];
dest.m.vec [5] = src.m.vec [5];
dest.m.vec [9] = src.m.vec [6];
dest.m.vec [2] = src.m.vec [8];
dest.m.vec [6] = src.m.vec [9];
dest.m.vec [10] = src.m.vec [10];
dest.m.vec [11] = dest.m.vec [12] = dest.m.vec [13] = dest.m.vec [14] = 0;
dest.m.vec [15] = 1.0f;
return dest;
}

// ----------------------------------------------------------------------------

CFixMatrix CFixMatrix::Mul (const CFixMatrix& other) 
{
CFixVector v;
CFixMatrix t;

v.v.coord.x = m.dir.r.v.coord.x;
v.v.coord.y = m.dir.u.v.coord.x;
v.v.coord.z = m.dir.f.v.coord.x;
t.m.dir.r.v.coord.x = CFixVector::Dot (v, other.m.dir.r);
t.m.dir.u.v.coord.x = CFixVector::Dot (v, other.m.dir.u);
t.m.dir.f.v.coord.x = CFixVector::Dot (v, other.m.dir.f);
v.v.coord.x = m.dir.r.v.coord.y;
v.v.coord.y = m.dir.u.v.coord.y;
v.v.coord.z = m.dir.f.v.coord.y;
t.m.dir.r.v.coord.y = CFixVector::Dot (v, other.m.dir.r);
t.m.dir.u.v.coord.y = CFixVector::Dot (v, other.m.dir.u);
t.m.dir.f.v.coord.y = CFixVector::Dot (v, other.m.dir.f);
v.v.coord.x = m.dir.r.v.coord.z;
v.v.coord.y = m.dir.u.v.coord.z;
v.v.coord.z = m.dir.f.v.coord.z;
t.m.dir.r.v.coord.z = CFixVector::Dot (v, other.m.dir.r);
t.m.dir.u.v.coord.z = CFixVector::Dot (v, other.m.dir.u);
t.m.dir.f.v.coord.z = CFixVector::Dot (v, other.m.dir.f);
#if 0
CFixVector::Normalize (m.m.dir.r);
CFixVector::Normalize (m.m.dir.u);
CFixVector::Normalize (m.m.dir.f);
#endif
return t;
}

// -----------------------------------------------------------------------------

void CFloatMatrix::Flip (void)
{
Swap (m.vec [1], m.vec [4]);
Swap (m.vec [2], m.vec [8]);
Swap (m.vec [3], m.vec [12]);
Swap (m.vec [7], m.vec [13]);
Swap (m.vec [11], m.vec [14]);
}

// -----------------------------------------------------------------------------

const fix CFixMatrix::Det (void) 
{
fix xDet = FixMul (m.dir.r.v.coord.x, FixMul (m.dir.u.v.coord.y, m.dir.f.v.coord.z) - FixMul (m.dir.u.v.coord.z, m.dir.f.v.coord.y));
xDet += FixMul (m.dir.r.v.coord.y, FixMul (m.dir.u.v.coord.z, m.dir.f.v.coord.x) - FixMul (m.dir.u.v.coord.x, m.dir.f.v.coord.z));
xDet += FixMul (m.dir.r.v.coord.z, FixMul (m.dir.u.v.coord.x, m.dir.f.v.coord.y) - FixMul (m.dir.u.v.coord.y, m.dir.f.v.coord.x));
return xDet;
}

// -----------------------------------------------------------------------------

const CFixMatrix CFixMatrix::Inverse (void) 
{
	fix xDet = Det ();
	CFixMatrix i;

i.m.dir.r.v.coord.x = FixDiv (FixMul (m.dir.u.v.coord.y, m.dir.f.v.coord.z) - FixMul (m.dir.u.v.coord.z, m.dir.f.v.coord.y), xDet);
i.m.dir.r.v.coord.y = FixDiv (FixMul (m.dir.r.v.coord.z, m.dir.f.v.coord.y) - FixMul (m.dir.r.v.coord.y, m.dir.f.v.coord.z), xDet);
i.m.dir.r.v.coord.z = FixDiv (FixMul (m.dir.r.v.coord.y, m.dir.u.v.coord.z) - FixMul (m.dir.r.v.coord.z, m.dir.u.v.coord.y), xDet);
i.m.dir.u.v.coord.x = FixDiv (FixMul (m.dir.u.v.coord.z, m.dir.f.v.coord.x) - FixMul (m.dir.u.v.coord.x, m.dir.f.v.coord.z), xDet);
i.m.dir.u.v.coord.y = FixDiv (FixMul (m.dir.r.v.coord.x, m.dir.f.v.coord.z) - FixMul (m.dir.r.v.coord.z, m.dir.f.v.coord.x), xDet);
i.m.dir.u.v.coord.z = FixDiv (FixMul (m.dir.r.v.coord.z, m.dir.u.v.coord.x) - FixMul (m.dir.r.v.coord.x, m.dir.u.v.coord.z), xDet);
i.m.dir.f.v.coord.x = FixDiv (FixMul (m.dir.u.v.coord.x, m.dir.f.v.coord.y) - FixMul (m.dir.u.v.coord.y, m.dir.f.v.coord.x), xDet);
i.m.dir.f.v.coord.y = FixDiv (FixMul (m.dir.r.v.coord.y, m.dir.f.v.coord.x) - FixMul (m.dir.r.v.coord.x, m.dir.f.v.coord.y), xDet);
i.m.dir.f.v.coord.z = FixDiv (FixMul (m.dir.r.v.coord.x, m.dir.u.v.coord.y) - FixMul (m.dir.r.v.coord.y, m.dir.u.v.coord.x), xDet);
return i;
}

//------------------------------------------------------------------------------

CFixMatrix& CFixMatrix::Transpose (CFixMatrix& dest, CFixMatrix& src)
{
dest.m.vec [0] = src.m.vec [0];
dest.m.vec [3] = src.m.vec [1];
dest.m.vec [6] = src.m.vec [2];
dest.m.vec [1] = src.m.vec [3];
dest.m.vec [4] = src.m.vec [4];
dest.m.vec [7] = src.m.vec [5];
dest.m.vec [2] = src.m.vec [6];
dest.m.vec [5] = src.m.vec [7];
dest.m.vec [8] = src.m.vec [8];
return dest;
}

// -----------------------------------------------------------------------------

CFloatMatrix& CFixMatrix::Transpose (CFloatMatrix& dest, CFixMatrix& src)
{
dest.m.vec [0] = X2F (src.m.vec [0]);
dest.m.vec [4] = X2F (src.m.vec [1]);
dest.m.vec [8] = X2F (src.m.vec [2]);
dest.m.vec [1] = X2F (src.m.vec [3]);
dest.m.vec [5] = X2F (src.m.vec [4]);
dest.m.vec [9] = X2F (src.m.vec [5]);
dest.m.vec [2] = X2F (src.m.vec [6]);
dest.m.vec [6] = X2F (src.m.vec [7]);
dest.m.vec [10] = X2F (src.m.vec [8]);
dest.m.vec [11] = dest.m.vec [12] = dest.m.vec [13] = dest.m.vec [14] = 0;
dest.m.vec [15] = 1.0f;
return dest;
}

// ----------------------------------------------------------------------------
//computes a matrix from the forward vector, a bank of
//zero is assumed.
//returns matrix.
const CFixMatrix CFixMatrix::CreateF (const CFixVector& fVec) 
{
	CFixMatrix m;

	m.m.dir.f = fVec;
	CFixVector::Normalize (m.m.dir.f);
	assert (m.m.dir.f.Mag () != 0);

	//just forward vec
	if ((m.m.dir.f.v.coord.x == 0) && (m.m.dir.f.v.coord.z == 0)) {		//forward vec is straight up or down
		m.m.dir.r.v.coord.x = I2X (1);
		m.m.dir.u.v.coord.z = (m.m.dir.f.v.coord.z < 0) ? I2X (1) : -I2X (1);
		m.m.dir.r.v.coord.y = m.m.dir.r.v.coord.z = m.m.dir.u.v.coord.x = m.m.dir.u.v.coord.y = 0;
	}
	else { 		//not straight up or down
		m.m.dir.r.v.coord.x = m.m.dir.f.v.coord.z;
		m.m.dir.r.v.coord.y = 0;
		m.m.dir.r.v.coord.z = -m.m.dir.f.v.coord.x;
		CFixVector::Normalize (m.m.dir.r);
		m.m.dir.u = CFixVector::Cross (m.m.dir.f, m.m.dir.r);
	}
	return m;
}

//	-----------------------------------------------------------------------------
//computes a matrix from the forward and the up vector.
//returns matrix.
const CFixMatrix CFixMatrix::CreateFU (const CFixVector& fVec, const CFixVector& uVec) 
{
	CFixMatrix	m;

m.m.dir.f = fVec;
CFixVector::Normalize (m.m.dir.f);
assert (m.m.dir.f.Mag () != 0);

m.m.dir.u = uVec;
if (CFixVector::Normalize (m.m.dir.u) == 0) {
	if ((m.m.dir.f.v.coord.x == 0) && (m.m.dir.f.v.coord.z == 0)) {		//forward vec is straight up or down
		m.m.dir.r.v.coord.x = I2X (1);
		m.m.dir.u.v.coord.z = (m.m.dir.f.v.coord.y < 0) ? I2X (1) : -I2X (1);
		m.m.dir.r.v.coord.y = m.m.dir.r.v.coord.z = m.m.dir.u.v.coord.x = m.m.dir.u.v.coord.y = 0;
		}
	else { 		//not straight up or down
		m.m.dir.r.v.coord.x = m.m.dir.f.v.coord.z;
		m.m.dir.r.v.coord.y = 0;
		m.m.dir.r.v.coord.z = -m.m.dir.f.v.coord.x;
		CFixVector::Normalize (m.m.dir.r);
		m.m.dir.u = CFixVector::Cross (m.m.dir.f, m.m.dir.r);
		}
	}
m.m.dir.r = CFixVector::Cross (m.m.dir.u, m.m.dir.f);
//Normalize new perpendicular vector
if (CFixVector::Normalize (m.m.dir.r) == 0) {
	if ((m.m.dir.f.v.coord.x == 0) && (m.m.dir.f.v.coord.z == 0)) {		//forward vec is straight up or down
		m.m.dir.r.v.coord.x = I2X (1);
		m.m.dir.u.v.coord.z = (m.m.dir.f.v.coord.y < 0) ? I2X (1) : -I2X (1);
		m.m.dir.r.v.coord.y = 
		m.m.dir.r.v.coord.z = 
		m.m.dir.u.v.coord.x = 
		m.m.dir.u.v.coord.y = 0;
		}
	else { 		//not straight up or down
		m.m.dir.r.v.coord.x = m.m.dir.f.v.coord.z;
		m.m.dir.r.v.coord.y = 0;
		m.m.dir.r.v.coord.z = -m.m.dir.f.v.coord.x;
		CFixVector::Normalize (m.m.dir.r);
		m.m.dir.u = CFixVector::Cross (m.m.dir.f, m.m.dir.r);
		}
	}

//	-----------------------------------------------------------------------------
//now recompute up vector, in case it wasn't entirely perpendiclar
m.m.dir.u = CFixVector::Cross (m.m.dir.f, m.m.dir.r);
return m;
}

//	-----------------------------------------------------------------------------
//computes a matrix from the forward and the right vector.
//returns matrix.
const CFixMatrix CFixMatrix::CreateFR (const CFixVector& fVec, const CFixVector& rVec) {
	CFixMatrix m;

m.m.dir.f = fVec;
CFixVector::Normalize (m.m.dir.f);
assert (m.m.dir.f.Mag () != 0);

//use right vec
m.m.dir.r = rVec;
if (CFixVector::Normalize (m.m.dir.r) == 0) {
	if ((m.m.dir.f.v.coord.x == 0) && (m.m.dir.f.v.coord.z == 0)) {		//forward vec is straight up or down
		m.m.dir.r.v.coord.x = I2X (1);
		m.m.dir.u.v.coord.z = (m.m.dir.f.v.coord.y < 0) ? I2X (1) : -I2X (1);
		m.m.dir.r.v.coord.y = 
		m.m.dir.r.v.coord.z = 
		m.m.dir.u.v.coord.x = 
		m.m.dir.u.v.coord.y = 0;
		}
	else { 		//not straight up or down
		m.m.dir.r.v.coord.x = m.m.dir.f.v.coord.z;
		m.m.dir.r.v.coord.y = 0;
		m.m.dir.r.v.coord.z = -m.m.dir.f.v.coord.x;
		CFixVector::Normalize (m.m.dir.r);
		m.m.dir.u = CFixVector::Cross (m.m.dir.f, m.m.dir.r);
		}
	}

m.m.dir.u = CFixVector::Cross (m.m.dir.f, m.m.dir.r);
//Normalize new perpendicular vector
if (CFixVector::Normalize (m.m.dir.u) == 0) {
	if ((m.m.dir.f.v.coord.x == 0) && (m.m.dir.f.v.coord.z == 0)) {		//forward vec is straight up or down
		m.m.dir.r.v.coord.x = I2X (1);
		m.m.dir.u.v.coord.z = (m.m.dir.f.v.coord.y < 0) ? I2X (1) : -I2X (1);
		m.m.dir.r.v.coord.y = m.m.dir.r.v.coord.z = m.m.dir.u.v.coord.x = m.m.dir.u.v.coord.y = 0;
		}
	else { 		//not straight up or down
		m.m.dir.r.v.coord.x = m.m.dir.f.v.coord.z;
		m.m.dir.r.v.coord.y = 0;
		m.m.dir.r.v.coord.z = -m.m.dir.f.v.coord.x;
		CFixVector::Normalize (m.m.dir.r);
		m.m.dir.u = CFixVector::Cross (m.m.dir.f, m.m.dir.r);
		}
	}
//now recompute right vector, in case it wasn't entirely perpendiclar
m.m.dir.r = CFixVector::Cross (m.m.dir.u, m.m.dir.f);
return m;
}

//	-----------------------------------------------------------------------------
//computes a matrix from the forward and the up vector.
//returns matrix.
const CFloatMatrix CFloatMatrix::CreateFU (const CFloatVector& fVec, const CFloatVector& uVec)
{
	CFloatMatrix m;

m.m.dir.f = fVec;
CFloatVector::Normalize (m.m.dir.f);
assert (m.m.dir.f.Mag () != 0);

m.m.dir.u = uVec;
if (CFloatVector::Normalize (m.m.dir.u) == 0) {
	if ((m.m.dir.f.v.coord.x == 0) && (m.m.dir.f.v.coord.z == 0)) {		//forward vec is straight up or down
		m.m.dir.r.v.coord.x = 1;
		m.m.dir.u.v.coord.z = (m.m.dir.f.v.coord.y < 0) ? 1.0f : -1.0f;
		m.m.dir.r.v.coord.y = m.m.dir.r.v.coord.z = m.m.dir.u.v.coord.x = m.m.dir.u.v.coord.y = 0;
		}
	else { 		//not straight up or down
		m.m.dir.r.v.coord.x = m.m.dir.f.v.coord.z;
		m.m.dir.r.v.coord.y = 0;
		m.m.dir.r.v.coord.z = -m.m.dir.f.v.coord.x;
		CFloatVector::Normalize (m.m.dir.r);
		m.m.dir.u = CFloatVector::Cross (m.m.dir.f, m.m.dir.r);
		}
	}
m.m.dir.r = CFloatVector::Cross (m.m.dir.u, m.m.dir.f);
//Normalize new perpendicular vector
if (CFloatVector::Normalize (m.m.dir.r) == 0) {
	if ((m.m.dir.f.v.coord.x == 0) && (m.m.dir.f.v.coord.z == 0)) {		//forward vec is straight up or down
		m.m.dir.r.v.coord.x = 1;
		m.m.dir.u.v.coord.z = (m.m.dir.f.v.coord.y < 0) ? 1.0f : -1.0f;
		m.m.dir.r.v.coord.y = m.m.dir.r.v.coord.z = m.m.dir.u.v.coord.x = m.m.dir.u.v.coord.y = 0;
	}
	else { 		//not straight up or down
		m.m.dir.r.v.coord.x = m.m.dir.f.v.coord.z;
		m.m.dir.r.v.coord.y = 0;
		m.m.dir.r.v.coord.z = -m.m.dir.f.v.coord.x;
		CFloatVector::Normalize (m.m.dir.r);
		m.m.dir.u = CFloatVector::Cross (m.m.dir.f, m.m.dir.r);
		}
	}
//now recompute up vector, in case it wasn't entirely perpendiclar
m.m.dir.u = CFloatVector::Cross (m.m.dir.f, m.m.dir.r);
return m;
}

//	-----------------------------------------------------------------------------
//computes a matrix from the forward and the right vector.
//returns matrix.
const CFloatMatrix CFloatMatrix::CreateFR (const CFloatVector& fVec, const CFloatVector& rVec) 
{
	CFloatMatrix m;

m.m.dir.f = fVec;
CFloatVector::Normalize (m.m.dir.f);
assert (m.m.dir.f.Mag () != 0);

//use right vec
m.m.dir.r = rVec;
if (CFloatVector::Normalize (m.m.dir.r) == 0) {
	if ((m.m.dir.f.v.coord.x == 0) && (m.m.dir.f.v.coord.z == 0)) {		//forward vec is straight up or down
		m.m.dir.r.v.coord.x = 1;
		m.m.dir.u.v.coord.z = (m.m.dir.f.v.coord.y < 0) ? 1.0f : -1.0f;
		m.m.dir.r.v.coord.y = m.m.dir.r.v.coord.z = m.m.dir.u.v.coord.x = m.m.dir.u.v.coord.y = 0;
		}
	else { 		//not straight up or down
		m.m.dir.r.v.coord.x = m.m.dir.f.v.coord.z;
		m.m.dir.r.v.coord.y = 0;
		m.m.dir.r.v.coord.z = -m.m.dir.f.v.coord.x;
		CFloatVector::Normalize (m.m.dir.r);
		m.m.dir.u = CFloatVector::Cross (m.m.dir.f, m.m.dir.r);
		}
	}

m.m.dir.u = CFloatVector::Cross (m.m.dir.f, m.m.dir.r);
//Normalize new perpendicular vector
if (CFloatVector::Normalize (m.m.dir.u) == 0) {
	if ((m.m.dir.f.v.coord.x == 0) && (m.m.dir.f.v.coord.z == 0)) {		//forward vec is straight up or down
		m.m.dir.r.v.coord.x = 1;
		m.m.dir.u.v.coord.z = (m.m.dir.f.v.coord.y < 0) ? 1.0f : -1.0f;
		m.m.dir.r.v.coord.y = m.m.dir.r.v.coord.z = m.m.dir.u.v.coord.x = m.m.dir.u.v.coord.y = 0;
		}
	else { 		//not straight up or down
		m.m.dir.r.v.coord.x = m.m.dir.f.v.coord.z;
		m.m.dir.r.v.coord.y = 0;
		m.m.dir.r.v.coord.z = -m.m.dir.f.v.coord.x;
		CFloatVector::Normalize (m.m.dir.r);
		m.m.dir.u = CFloatVector::Cross (m.m.dir.f, m.m.dir.r);
		}
	}
//now recompute right vector, in case it wasn't entirely perpendiclar
m.m.dir.r = CFloatVector::Cross (m.m.dir.u, m.m.dir.f);
return m;
}

//	-----------------------------------------------------------------------------
//extract angles from a m.matrix
const CFloatVector CFloatMatrix::ComputeAngles (void) const 
{
	CFloatVector	a;

a.v.coord.z = ((m.dir.f.v.coord.x == 0.0f) && (m.dir.f.v.coord.z == 0.0f)) ? 0.0f : a.v.coord.z = atan2 (m.dir.f.v.coord.z, m.dir.f.v.coord.x);
float sinh = sin (a.v.coord.z);
float cosh = cos (a.v.coord.z);
float cosp = (fabs (sinh) > fabs (cosh))	? m.dir.f.v.coord.x / sinh : m.dir.f.v.coord.z / cosh;
a.v.coord.x = ((cosp == 0.0f) && (m.dir.f.v.coord.y == 0.0f)) ? 0.0f : atan2 (cosp, -m.dir.f.v.coord.y);
if (cosp == 0)	//the cosine of pitch is zero.  we're pitched straight up. say no bank
	a.v.coord.y = 0.0f;
else {
	float sinb = m.dir.r.v.coord.y / cosp;
	float cosb = m.dir.u.v.coord.y / cosp;
	a.v.coord.y = ((sinb == 0.0f) && (cosb == 0.0f)) ? 0.0f : a.v.coord.y = atan2 (cosb, sinb);
	}
return a;
}

//	-----------------------------------------------------------------------------
//extract angles from a m.matrix
const CAngleVector CFixMatrix::ComputeAngles (void) const 
{
	CAngleVector	a;
	fix				sinh, cosh, cosp;

a.v.coord.h = (m.dir.f.v.coord.x | m.dir.f.v.coord.z) ? FixAtan2 (m.dir.f.v.coord.z, m.dir.f.v.coord.x) : 0;
FixSinCos (a.v.coord.h, &sinh, &cosh);
cosp = (abs (sinh) > abs (cosh)) ? FixDiv (m.dir.f.v.coord.x, sinh) : FixDiv (m.dir.f.v.coord.z, cosh);
a.v.coord.p = (cosp | m.dir.f.v.coord.y) ? FixAtan2 (cosp, -m.dir.f.v.coord.y) : 0;
if (cosp == 0)	//the cosine of pitch is zero.  we're pitched straight up. say no bank
	a.v.coord.b = 0;
else {
	fix sinb = FixDiv (m.dir.r.v.coord.y, cosp);
	fix cosb = FixDiv (m.dir.u.v.coord.y, cosp);
	a.v.coord.b = (sinb | cosb) ? FixAtan2 (cosb, sinb) : 0;
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
const int FindPointLineIntersection (CFixVector& hitP, const CFixVector& p1, const CFixVector& p2, const CFixVector& p3, int bClamp)
{
CFixVector d21 = p2 - p1;
double m = fabs (double (d21.v.coord.x) * double (d21.v.coord.x) + double (d21.v.coord.y) * double (d21.v.coord.y) + double (d21.v.coord.z) * double (d21.v.coord.z));
if (m == 0.0) {
	hitP = p1;
	return 0;
	}
CFixVector d31 = p3 - p1;
double u = double (d31.v.coord.x) * double (d21.v.coord.x) + double (d31.v.coord.y) * double (d21.v.coord.y) + double (d31.v.coord.z) * double (d21.v.coord.z);
u /= m;

int bClamped = 0;
if (u < 0.0)
	bClamped = bClamp ? 2 : 1;
else if (u > 1.0)
	bClamped = bClamp ? 1 : 2;
if ((bClamp >= 0) && bClamped) {
	if (bClamped == 2)
		hitP = p2;
	else if (bClamped == 1)
		hitP = p1;
	}
else {
	hitP.v.coord.x = p1.v.coord.x + (fix) (u * d21.v.coord.x);
	hitP.v.coord.y = p1.v.coord.y + (fix) (u * d21.v.coord.y);
	hitP.v.coord.z = p1.v.coord.z + (fix) (u * d21.v.coord.z);
	}
return bClamped;
}

// ------------------------------------------------------------------------

// Version with vPos
const int FindPointLineIntersection (CFloatVector& hitP, const CFloatVector& p1, const CFloatVector& p2, const CFloatVector& p3, 
											    const CFloatVector& vPos, int bClamp) 
{
	CFloatVector	d31, d21;
	float		m, u;
	int		bClamped = 0;

d21 = p2 - p1;
m = (float) fabs (d21.SqrMag ());
if (m != 0.0f) {
	hitP = p1;
	return 0;
	}
d31 = p3 - p1;
u = CFloatVector::Dot (d31, d21);
u /= m;
if (u < 0.0f)
	bClamped = 1;
else if (u > 1.0f)
	bClamped = 2;
else
	bClamped = 0;
// limit the intersection to [p1,p2]
if (bClamp && bClamped) {
	bClamped = (CFloatVector::Dist (vPos, p1) < CFloatVector::Dist (vPos, p2)) ? 2 : 1;
	hitP = (bClamped == 1) ? p1 : p2;
	}
else
	hitP = p1 + d21 * u;
return bClamped;
}


// Version without vPos
const int FindPointLineIntersection (CFloatVector& hitP, const CFloatVector& p1, const CFloatVector& p2, const CFloatVector& p3, int bClamp) 
{
	CFloatVector	d31, d21;
	float		m, u;
	int		bClamped = 0;

d21 = p2 - p1;
m = (float) fabs (d21.SqrMag ());	// Dot (d21, d21)
if (m == 0.0f) {
	hitP = p1;
	return 0;
	}
d31 = p3 - p1;
u = CFloatVector::Dot (d31, d21);
u /= m;
if (u < 0.0f)
	bClamped = 1;
else if (u > 1.0f)
	bClamped = 2;
else
	bClamped = 0;
// limit the intersection to [p1,p2]
if (bClamp && bClamped)
	hitP = (bClamped == 1) ? p1 : p2; //(CFloatVector::Dist (p3, p1) < CFloatVector::Dist (p3, p2)) ? p1 : p2;
else
	hitP = p1 + d21 * u;
return bClamped;
}

// ------------------------------------------------------------------------

const int FindPointLineIntersection (CFloatVector3& hitP, const CFloatVector3& p1, const CFloatVector3& p2, const CFloatVector3& p3, CFloatVector3 *vPos, int bClamp) 
{
	CFloatVector3	d31, d21;
	float		m, u;
	int		bClamped = 0;

d21 = p2 - p1;
m = (float) fabs (d21.SqrMag ());
if (m == 0.0f) {
	hitP = p1;
	return 0;
	}
d31 = p3 - p1;
u = CFloatVector3::Dot (d31, d21);
u /= m;
if (u < 0.0f)
	bClamped = 1;
else if (u > 1.0f)
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
	hitP = p1 + d21 * u;
	}
return bClamped;
}

// ------------------------------------------------------------------------

const fix VmLinePointDist (const CFixVector& a, const CFixVector& b, const CFixVector& p) 
{
	CFixVector	h;

FindPointLineIntersection (h, a, b, p, 0);
return CFixVector::Dist (h, p);
}

// ------------------------------------------------------------------------

const float VmLinePointDist (const CFloatVector& a, const CFloatVector& b, const CFloatVector& p, int bClamp)
{
	CFloatVector	h;

FindPointLineIntersection (h, a, b, p, bClamp);
return CFloatVector::Dist (h, p);
}

// ------------------------------------------------------------------------

const float VmLinePointDist (const CFloatVector3& a, const CFloatVector3& b, const CFloatVector3& p, int bClamp)
{
	CFloatVector3	h;

FindPointLineIntersection (h, a, b, p, NULL, bClamp);
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

va = v1 + v21 * mua;
vb = v3 + v43 * mub;

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
va->x () = v1->x () + mua * v21.v.coord.x;
va->y () = v1->y () + mua * v21.v.coord.y;
va->z () = v1->z () + mua * v21.v.coord.z;
vb->x () = v3->x () + mub * v43.v.coord.x;
vb->y () = v3->y () + mub * v43.v.coord.y;
vb->z () = v3->z () + mub * v43.v.coord.z;
*/
va = v1 + v21 * mua;
vb = v3 + v43 * mub;

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
	int i = RandShort () % 3;

if (i == 2) {
	v.v.coord.x = (SRandShort ()) | 1;	// make sure we don't create null vector
	v.v.coord.y = SRandShort ();
	v.v.coord.z = SRandShort ();
	}
else if (i == 1) {
	v.v.coord.y = (SRandShort ()) | 1;
	v.v.coord.z = SRandShort ();
	v.v.coord.x = SRandShort ();	// make sure we don't create null vector
	}
else {
	v.v.coord.z = (SRandShort ()) | 1;
	v.v.coord.x = SRandShort ();	// make sure we don't create null vector
	v.v.coord.y = SRandShort ();
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

float a = vDir.v.coord.x * vDir.v.coord.x / vScale.v.coord.x +
			 vDir.v.coord.y * vDir.v.coord.y / vScale.v.coord.y +
			 vDir.v.coord.z * vDir.v.coord.z / vScale.v.coord.z;

float b = (vLine.v.coord.x * vDir.v.coord.x / vScale.v.coord.x +
			  vLine.v.coord.y * vDir.v.coord.y / vScale.v.coord.y +
			  vLine.v.coord.z * vDir.v.coord.z / vScale.v.coord.z) * 2.0f;

float c = vLine.v.coord.x * vLine.v.coord.x / vScale.v.coord.x +
			 vLine.v.coord.y * vLine.v.coord.y / vScale.v.coord.y +
			 vLine.v.coord.z * vLine.v.coord.z / vScale.v.coord.z - 1.0f;

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
