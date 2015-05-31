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
COPYRIGHT 1993-1998PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "3d.h"
#include "globvars.h"
#include "canvas.h"
#include "transprender.h"

// -----------------------------------------------------------------------------------
//checks for overflow & divides if ok, fillig in r
//returns true if div is ok, else false
#ifdef _WIN32
inline
#endif
int32_t CheckMulDiv (fix *r, fix a, fix b, fix c)
{
#ifdef _WIN32
	int64_t	q;
	if (!c)
		return 0;
	q = mul64 (a, b) / (int64_t) c;
	if ((q > 0x7fffffff) || (q < -0x7fffffff))
		return 0;
	*r = (fix) q;
	return 1;
#else
	tQuadInt q,qt;

	q.low=q.high=0;
	FixMulAccum (&q,a,b);
	qt = q;
	if (qt.high < 0)
		FixQuadNegate (&qt);
	qt.high *= 2;
	if (qt.low > 0x7fff)
		qt.high++;
	if (qt.high >= c)
		return 0;
	else {
		*r = FixDivQuadLong (q.low,q.high,c);
		return 1;
	}
#endif
}

//------------------------------------------------------------------------------

inline int32_t ScreenScale (void)
{
return (!gameStates.render.cameras.bActive || gameOpts->render.cameras.bHires) ? 1 : 2;
}

inline int32_t ScreenWidth (void)
{
return gameData.renderData.screen.Width () /*/ ScreenScale ()*/;
}

inline int32_t ScreenHeight (void)
{
return gameData.renderData.screen.Height () /*/ ScreenScale ()*/;
}

// -----------------------------------------------------------------------------------

uint8_t OglProjectPoint (CFloatVector3& p, tScreenPos& s, uint8_t flags, uint8_t codes)
{
if ((flags & PF_PROJECTED) || (codes & CC_BEHIND))
	return flags;
double x, y, z;
gluProject ((double) p.v.coord.x, (double) p.v.coord.y, (double) p.v.coord.z, 
				&transformation.m_info.oglModelview [0], &transformation.m_info.oglProjection [0], transformation.m_info.oglViewport, 
				&x, &y, &z);
#if 1
s.x = fix (x);
s.y = fix (y);
#else
s.x = fix (CCanvas::xCanvW2f - x * CCanvas::xCanvW2f / z);
s.y = fix (CCanvas::fCanvH2 - y * CCanvas::fCanvH2 / z);
#endif
#if DBG
tScreenPos h;
OglProjectPoint (p, h);
if ((h.x != s.x) || (h.y != s.y))
	s = h;
#endif
return flags | PF_PROJECTED;
}

// -----------------------------------------------------------------------------------
//projects a point

uint8_t ProjectPoint (CFloatVector3& p, tScreenPos& s, uint8_t flags, uint8_t codes)
{
if ((flags & PF_PROJECTED) || (codes & CC_BEHIND))
	return flags;
CFloatVector3 v = transformation.m_info.projection * p;
float z = fabs (p.v.coord.z);
s.x = fix (CCanvas::fCanvW2 * (1.0f + v.v.coord.x / z));
s.y = fix (CCanvas::fCanvH2 * (1.0f + v.v.coord.y / z));
return flags | PF_PROJECTED;
}

// -----------------------------------------------------------------------------------

uint8_t ProjectPoint (CFixVector& v, tScreenPos& s, uint8_t flags, uint8_t codes)
{
CFloatVector3 h;
h.Assign (v);
return ProjectPoint (h, s, flags);
}

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------

void CRenderPoint::Transform (int32_t nVertex) 
{
if (nVertex >= 0)
	m_vertex [0] = VERTICES [nVertex];
transformation.Transform (m_vertex [1], m_vertex [0]); 
}

// -----------------------------------------------------------------------------------

uint8_t CRenderPoint::Project (CTransformation& transformation, CFloatVector3& pViewos)
{
if ((m_flags & PF_PROJECTED) || (m_codes & CC_BEHIND))
	return m_flags;
CFloatVector3 v = transformation.m_info.projection * pViewos;
float z = fabs (pViewos.v.coord.z);
#if 0
m_screen.x = fix ((float) gameData.renderData.screen.Width () * 0.5f * (1.0f + v.v.coord.x / z));
m_screen.y = fix ((float) gameData.renderData.screen.Height () * 0.5f * (1.0f + v.v.coord.y / z));
#else
m_screen.x = fix (CCanvas::fCanvW2 * (1.0f + v.v.coord.x / z));
m_screen.y = fix (CCanvas::fCanvH2 * (1.0f + v.v.coord.y / z));
#endif
m_flags |= PF_PROJECTED;
return m_flags;
}

// -----------------------------------------------------------------------------------

void CRenderPoint::Project (void)
{
CFloatVector3 pViewosf;
pViewosf.Assign (ViewPos ());
m_flags = Project (transformation, pViewosf);
}

// -----------------------------------------------------------------------------------

uint8_t CRenderPoint::ProjectAndEncode (CTransformation& transformation, int32_t nVertex)
{
#if DBG
if (nVertex == nDbgVertex)
	BRP;
#endif
if (!Projected ()) {
	WorldPos () = VERTICES [nVertex];
	CFloatVector3 pViewosf;
	transformation.Transform (pViewosf, *FVERTICES [nVertex].XYZ ());
	ViewPos ().Assign (pViewosf);
	Project (transformation, pViewosf);
	Encode ();
	AddFlags (PF_PROJECTED);
	SetCodes ((pViewosf.v.coord.z < 0.0f) ? CC_BEHIND : 0);
#if TRANSP_DEPTH_HASH
	fix d = ViewPos ().Mag ();
	if (gameData.renderData.zMin > d)
		gameData.renderData.zMin = d;
	if (gameData.renderData.zMax < d)
		gameData.renderData.zMax = d;
#else
	if (gameData.renderData.zMax < point.m_vertex [1].dir.coord.z)
		gameData.renderData.zMax = point.m_vertex [1].dir.coord.z;
#endif
	}
return Codes ();
}

// -----------------------------------------------------------------------------------

uint8_t CRenderPoint::Encode (void) 
{
if (!Projected ())
	m_codes = transformation.Codes (m_vertex [1]); 
else {
	if (m_screen.x < 0)
		m_codes |= CC_OFF_LEFT;
	else if (m_screen.x > gameData.renderData.screen.Width ())
		m_codes |= CC_OFF_RIGHT;
	if (m_screen.y < 0)
		m_codes |= CC_OFF_BOT;
	else if (m_screen.y > gameData.renderData.screen.Height ())
		m_codes |= CC_OFF_TOP;
	}
return m_codes;
}

// -----------------------------------------------------------------------------------

uint8_t CRenderPoint::Add (CRenderPoint& src, CFixVector& vDelta)
{
ViewPos () = src.ViewPos () + vDelta;
m_flags = 0;		//not projected
return Encode ();
}

// -----------------------------------------------------------------------------------
//calculate the depth of a point - returns the z coord of the rotated point
fix G3CalcPointDepth (const CFixVector& v)
{
#ifdef _WIN32
	int64_t q = mul64 (v.v.coord.x - transformation.m_info.pos.v.coord.x, transformation.m_info.view [0].m.dir.f.v.coord.x);
	q += mul64 (v.v.coord.y - transformation.m_info.pos.v.coord.y, transformation.m_info.view [0].m.dir.f.v.coord.y);
	q += mul64 (v.v.coord.z - transformation.m_info.pos.v.coord.z, transformation.m_info.view [0].m.dir.f.v.coord.z);
	return (fix) (q >> 16);
#else
	tQuadInt q;

	q.low=q.high=0;
	FixMulAccum (&q, (v.v.coord.x - transformation.m_info.pos.v.coord.x),transformation.m_info.view [0].m.dir.f.v.coord.x);
	FixMulAccum (&q, (v.v.coord.y - transformation.m_info.pos.v.coord.y),transformation.m_info.view [0].m.dir.f.v.coord.y);
	FixMulAccum (&q, (v.v.coord.z - transformation.m_info.pos.v.coord.z),transformation.m_info.view [0].m.dir.f.v.coord.z);
	return FixQuadAdjust (&q);
#endif
}

// -----------------------------------------------------------------------------------
//eof

