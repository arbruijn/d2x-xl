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
#	define inline
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
		_INLINE_ void Move (CFloatVector& v) { glTranslatef (-v [X], -v [Y], -v [Z]); }
		_INLINE_ void Rotate (CFloatMatrix& m) { glMultMatrixf (m.Vec ()); }

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

		_INLINE_ CFloatVector& Translate (CFloatVector& dest, const CFloatVector& src) 
		 { return dest = src - m_info.posf [0]; }

		_INLINE_ CFloatVector& Rotate (CFloatVector& dest, const CFloatVector& src, int bUnscaled = 0) 
		 { return dest = m_info.viewf [bUnscaled] * src; }

		_INLINE_ CFloatVector3& Rotate (CFloatVector3& dest, const CFloatVector3& src, int bUnscaled = 0) 
		 { return dest = m_info.viewf [bUnscaled] * src; }

		_INLINE_ CFloatVector& Transform (CFloatVector& dest, const CFloatVector& src, int bUnscaled = 0) {
			CFloatVector vTrans = src - m_info.posf [0];
			return dest = m_info.viewf [bUnscaled] * vTrans;
			}

		CFixVector RotateScaledX (CFixVector& dest, fix scale);
		CFixVector RotateScaledY (CFixVector& dest, fix scale);
		CFixVector RotateScaledZ (CFixVector& dest, fix scale);

		const CFixVector& RotateScaled (CFixVector& dest, const CFixVector& src);

		_INLINE_ ubyte Codes (CFixVector& v) {
			ubyte codes = 0;
			fix z = v [Z];
			fix x = FixMulDiv (v [X], m_info.scale [X], m_info.zoom);

			if (x > z)
				codes |= CC_OFF_RIGHT;
			if (x < -z)
				codes |= CC_OFF_LEFT;
			if (v [Y] > z)
				codes |= CC_OFF_TOP;
			if (v [Y] < -z)
				codes |= CC_OFF_BOT;
			if (z < 0)
				codes |= CC_BEHIND;
			return codes;
			}

		_INLINE_ ubyte TransformAndEncode (CFixVector& dest, const CFixVector& src) {
			Transform (dest, src, 0);
			return Codes (dest);
			}


		void ComputeAspect (void);

	};

//------------------------------------------------------------------------------

extern CTransformation	transformation;

#undef _INLINE_

#endif //_TRANSFORMATION_

