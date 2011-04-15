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

#ifndef _TRANSFORMATION_H
#define _TRANSFORMATION_H

#include "fix.h"
#include "vecmat.h" 

#if DBG
#	define _INLINE_
#else
#	define _INLINE_	inline
#endif
//clipping codes flags

#define CC_OFF_LEFT     1
#define CC_OFF_RIGHT    2
#define CC_OFF_BOT      4
#define CC_OFF_TOP      8
#define CC_BEHIND       0x80

typedef struct tTransformation {
		CFixVector		pos;
		CAngleVector	playerHeadAngles;
		int				bUsePlayerHeadAngles;
		CFixMatrix		view [2];
		CFixVector		scale;
		CFloatVector	scalef;
		CFixVector		aspect;		//scaling for window aspect
		CFloatVector	posf [2];
		CFloatMatrix	viewf [3];
		CFloatMatrix	projection;
		fix				zoom;
		float				zoomf;
} tTransformation;

class CTransformation {
	public:
		tTransformation			m_info;

	private:
		CStack<tTransformation>	m_save;

	public:
		CTransformation () { Init (); }
		void Init (void);
		bool Push (void);
		bool Pop (void);
		void Begin (const CFixVector& vPos, CFixMatrix& mOrient);
		_INLINE_ void Begin (const CFixVector& pos, const CAngleVector& angles) {
			CFixMatrix m = CFixMatrix::Create (angles);
			Begin (pos, m); 
			}
		_INLINE_ void Begin (const CFixVector& pos) {
			CFixMatrix m = CFixMatrix::IDENTITY;
			Begin (pos, m); 
			}
		_INLINE_ void End (void) { Pop (); }
		_INLINE_ void Move (CFloatVector& v) { glTranslatef (-v.v.coord.x, -v.v.coord.y, -v.v.coord.z); }
		_INLINE_ void Rotate (CFloatMatrix& m) { glMultMatrixf (m.m.vec); }

		_INLINE_ void Move (const CFixVector& v) {
			CFloatVector vf;
			vf.Assign (v);
			Move (vf);
			}
		_INLINE_ void Rotate (CFixMatrix& m) {
			CFloatMatrix mf;
			mf.Assign (m);
			Rotate (mf);
			}

		_INLINE_ CFixVector& Translate (CFixVector& dest, const CFixVector& src) 
		 { return dest = src - m_info.pos; }

		_INLINE_ CFixVector& Rotate (CFixVector& dest, const CFixVector& src, int bUnscaled = 0) 
		 { return dest = m_info.view [bUnscaled] * src; }

		_INLINE_ CFixVector& Transform (CFixVector& dest, const CFixVector& src, int bUnscaled = 0) {
			CFixVector vTrans = src - m_info.pos;
			return dest = m_info.view [bUnscaled] * vTrans;
			}

		_INLINE_ CFloatVector& Rotate (CFloatVector& dest, const CFloatVector& src, int bUnscaled = 0) 
		 { return dest = m_info.viewf [bUnscaled] * src; }

		_INLINE_ CFloatVector& Transform (CFloatVector& dest, const CFloatVector& src, int bUnscaled = 0) {
			CFloatVector vTrans = src - m_info.posf [0];
			return dest = m_info.viewf [bUnscaled] * vTrans;
			}

		_INLINE_ CFloatVector3& Translate (CFloatVector3& dest, const CFloatVector3& src) 
		 { return dest = src - *m_info.posf [0].XYZ (); }

		_INLINE_ CFloatVector3& Rotate (CFloatVector3& dest, const CFloatVector3& src, int bUnscaled = 0) { 
			CFloatVector vTemp;
			vTemp.Assign (src);
			dest.Assign (m_info.viewf [bUnscaled] * vTemp);
			return dest;			
			}

		_INLINE_ CFloatVector3& Transform (CFloatVector3& dest, const CFloatVector3& src, int bUnscaled = 0) {
			CFloatVector v;
			v.Assign (src - *m_info.posf [0].XYZ ());
			dest.Assign (m_info.viewf [bUnscaled] * v);
			return dest;
			}

		CFixVector RotateScaledX (CFixVector& dest, fix scale);
		CFixVector RotateScaledY (CFixVector& dest, fix scale);
		CFixVector RotateScaledZ (CFixVector& dest, fix scale);

		const CFixVector& RotateScaled (CFixVector& dest, const CFixVector& src);

		_INLINE_ ubyte Codes (CFixVector& v) {
			ubyte codes = 0;
			fix z = v.v.coord.z;
			fix x = FixMulDiv (v.v.coord.x, m_info.scale.v.coord.x, m_info.zoom);

			if (x > z)
				codes |= CC_OFF_RIGHT;
			if (x < -z)
				codes |= CC_OFF_LEFT;
			if (v.v.coord.y > z)
				codes |= CC_OFF_TOP;
			if (v.v.coord.y < -z)
				codes |= CC_OFF_BOT;
			if (z < 0)
				codes |= CC_BEHIND;
			return codes;
			}

		_INLINE_ ubyte TransformAndEncode (CFixVector& dest, const CFixVector& src) {
			Transform (dest, src, 0);
			return Codes (dest);
			}

		inline CFloatMatrix& Projection (void) { return m_info.projection; }

		void ComputeAspect (void);

		void SetupProjection (void);

	};

//------------------------------------------------------------------------------

extern CTransformation	transformation;

#undef _INLINE_

#endif //_TRANSFORMATION_

