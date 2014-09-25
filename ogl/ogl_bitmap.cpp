/*
 *
 * Graphics support functions for OpenGL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#	include <io.h>
#endif
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef __macosx__
# include <stdlib.h>
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "descent.h"
#include "error.h"
#include "maths.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texture.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "ogl_render.h"
#include "ogl_tmu.h"
#include "light.h"
#include "lightmap.h"
#include "texmerge.h"
#include "transprender.h"


//------------------------------------------------------------------------------

void CBitmap::SetTexCoord (GLfloat u, GLfloat v, int32_t orient)
{
if (orient == 1)
	glTexCoord2f ((1.0f - v) * m_render.aspect, u);
else if (orient == 2)
	glTexCoord2f (1.0f - u, 1.0f - v);
else if (orient == 3)
	glTexCoord2f (v * m_render.aspect, 1.0f - u);
else
	glTexCoord2f (u, v);
}

//------------------------------------------------------------------------------

void CBitmap::SetTexCoord (GLfloat u, GLfloat v, int32_t orient, tTexCoord2f& texCoord)
{
if (orient == 1)
	texCoord.v.u = (1.0f - v) * m_render.aspect, texCoord.v.v = u;
else if (orient == 2)
	texCoord.v.u = 1.0f - u, texCoord.v.v = 1.0f - v;
else if (orient == 3)
	texCoord.v.u = v * m_render.aspect, texCoord.v.v = 1.0f - u;
else
	texCoord.v.u = u, texCoord.v.v = v;
}

//------------------------------------------------------------------------------

void CBitmap::OglVertices (int32_t x, int32_t y, int32_t w, int32_t h, int32_t scale, int32_t orient, CRectangle* destP)
{
if (!w)
	w = Width ();
else if (w < 0)
	w = destP ? destP->Width () : CCanvas::Current ()->Width ();

if (!h)
	h = Height ();
else if (h < 0)
	h = destP ? destP->Height () : CCanvas::Current ()->Height ();

float dx = destP ? float (destP->Left ()) / float (ogl.m_states.viewport [0].m_w) : 0.0f;
float dy = destP ? float (destP->Top ()) / float (ogl.m_states.viewport [0].m_h) : 0.0f;

if (orient & 1) {
	::Swap (w, h);
	::Swap (dx, dy);
	}

float fScale = X2F (scale);
float a = float (ogl.m_states.viewport [0].m_w) * fScale;
m_render.x0 = dx + float (x) / a;
m_render.x1 = dx + float (x + w) / a;
a = float (ogl.m_states.viewport [0].m_h) * fScale;
m_render.aspect = float (ogl.m_states.viewport [0].m_h) * fScale;
m_render.y0 = 1.0f - dy - float (y) / a;
m_render.y1 = 1.0f - dy - float (y + h) / a;
}

//------------------------------------------------------------------------------

void CBitmap::OglTexCoord (void)
{
float h = float (m_info.texP->TW ());
m_render.u1 = float (Left ()) / h;
m_render.u2 = float (Right ()) / h;
h = float (m_info.texP->TH ());
m_render.v1 = float (Top ()) / h;
m_render.v2 = float (Bottom ()) / h;
}

//------------------------------------------------------------------------------

CTexture* CBitmap::OglBeginRender (bool bBlend, int32_t bMipMaps, int32_t nTransp)
{
m_render.bBlendState = ogl.GetBlendUsage ();
m_render.depthFunc = ogl.GetDepthMode ();

ogl.SelectTMU (GL_TEXTURE0);
ogl.SetTexturing (true);
if (Bind (bMipMaps))
	return NULL;
m_info.texP->Wrap (GL_REPEAT);
ogl.SetDepthMode (GL_ALWAYS);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
return &m_info.texture;
}

//------------------------------------------------------------------------------

void CBitmap::OglRender (CFloatVector* colorP, int32_t nColors, int32_t orient)
{
	float				verts [4][2] = {{m_render.x0, m_render.y0}, {m_render.x1, m_render.y0}, {m_render.x1, m_render.y1}, {m_render.x0, m_render.y1}};
	tTexCoord2f		texCoord [4];

SetTexCoord (m_render.u1, m_render.v1, orient, texCoord [0]);
SetTexCoord (m_render.u2, m_render.v1, orient, texCoord [1]);
SetTexCoord (m_render.u2, m_render.v2, orient, texCoord [2]);
SetTexCoord (m_render.u1, m_render.v2, orient, texCoord [3]);
ogl.EnableClientStates (1, (nColors == 4), 0, GL_TEXTURE0);
glTexCoordPointer (2, GL_FLOAT, sizeof (tTexCoord2f), texCoord);
if (nColors == 4)
	OglColorPointer (4, GL_FLOAT, sizeof (CFloatVector), colorP);
else
	glColor4fv (reinterpret_cast<GLfloat*> (colorP));
OglVertexPointer (2, GL_FLOAT, 2 * sizeof (float), verts);
OglDrawArrays (GL_QUADS, 0, 4);
ogl.DisableClientStates (1, (nColors == 4), 0, GL_TEXTURE0);
}

//------------------------------------------------------------------------------

void CBitmap::OglEndRender (void)
{
ogl.SetDepthMode (m_render.depthFunc);
ogl.SetBlending (m_render.bBlendState != 0);
ogl.SelectTMU (GL_TEXTURE0);
ogl.BindTexture (0);
ogl.SetTexturing (false);
}

//------------------------------------------------------------------------------

int32_t CBitmap::RenderScaled (int32_t x, int32_t y, int32_t w, int32_t h, int32_t scale, int32_t orient, CCanvasColor* colorP, int32_t bSmoothe)
{
	CBitmap*			bmoP;
	CFloatVector	color;

if ((bmoP = HasOverride ()))
	return bmoP->RenderScaled (x, y, w, h, scale, orient, colorP, bSmoothe);
DelFlags (BM_FLAG_SUPER_TRANSPARENT);
if (!OglBeginRender (true, bSmoothe ? 0 : -1, 3))
	return 1; // fail
OglVertices (x, y, w, h, scale, orient);
OglTexCoord ();
color = GetCanvasColor (colorP);
OglRender (&color, 1, orient);
OglEndRender ();
return 0;
}

//------------------------------------------------------------------------------

int32_t CBitmap::Render (CRectangle* destP,
								 int32_t xDest, int32_t yDest, int32_t wDest, int32_t hDest,
								 int32_t xSrc, int32_t ySrc, int32_t wSrc, int32_t hSrc,
								 int32_t bTransp, int32_t bMipMaps, int32_t bSmoothe,
								 float fAlpha, CFloatVector* colorP)
{
	CBitmap*		bmoP;

if ((bmoP = HasOverride ()))
	return bmoP->Render (destP, xDest, yDest, wDest, hDest, xSrc, ySrc, wSrc, hSrc, bTransp, bMipMaps, bSmoothe, fAlpha, colorP);

	int32_t		nTransp = (Flags () & BM_FLAG_TGA) ? -1 : HasTransparency () ? 2 : 0;
	bool			bLocal = Texture () == NULL;

//	uint8_t *oldpal;
CFloatVector color;
int32_t nColors;
bool bBlend = bTransp && nTransp;

if (!OglBeginRender (bBlend, bMipMaps, nTransp))
	return 1; // fail

OglVertices (xDest, yDest, wDest, hDest, I2X (1), 0, destP);

if (!colorP) {
	color.Red () = color.Green () = color.Blue () = 1.0f;
	color.Alpha () = bBlend ? fAlpha * gameStates.render.grAlpha : 1.0f;
	colorP = &color;
	nColors = 1;
	}
else
	nColors = 4;

m_render.u1 = m_render.v1 = 0;
if (wSrc < 0)
	m_render.u2 = float (destP->Width ()) / float (-wSrc);
else {
	m_render.u2 = float (wSrc) / float (Width ());
	if (m_render.u2 < 1.0f)
		m_render.u2 *= m_info.texP->U ();
	else
		m_render.u2 = m_info.texP->U ();
	}
if (hSrc < 0)
	m_render.v2 = float (destP->Height ()) / float (-hSrc);
else {
	m_render.v2 = float (hSrc) / float (Height ());
	if (m_render.v2 < 1.0f)
		m_render.v2 *= m_info.texP->V ();
	else
		m_render.v2 = m_info.texP->V ();
	}

OglRender (colorP, nColors, 0);
OglEndRender ();

if (bLocal) {
	m_info.texP->Destroy ();
	}
return 0;
}

//------------------------------------------------------------------------------
// OglUBitBltToLinear
void CBitmap::ScreenCopy (CBitmap * dest, int32_t dx, int32_t dy, int32_t w, int32_t h, int32_t sx, int32_t sy)
{
	uint8_t *d, *s;
	int32_t	i, j;
	int32_t	bTGA = (dest->Flags () & BM_FLAG_TGA) != 0;
	int32_t	wScreen = gameData.render.screen.Width ();
	int32_t	hScreen = gameData.render.screen.Height ();

ogl.SetTexturing (false);
ogl.SetReadBuffer (GL_FRONT, 1);
if (bTGA) {
	ogl.SetReadBuffer (GL_FRONT, 0);
	glReadPixels (0, 0, wScreen, hScreen, GL_RGBA, GL_UNSIGNED_BYTE, ogl.m_data.buffer);
	}
else {
	if (wScreen * hScreen * 3 > OGLTEXBUFSIZE)
		Error ("OglUBitBltToLinear: screen res larger than OGLTEXBUFSIZE\n");
	ogl.SetReadBuffer (GL_FRONT, 0);
	glReadPixels (0, 0, wScreen, hScreen, GL_RGB, GL_UNSIGNED_BYTE, ogl.m_data.buffer);
	}

uint8_t* buffer = dest->Buffer ();
int32_t rowSize = dest->RowSize ();

if (bTGA) {
	sx += Left ();
	sy += Top ();
	for (i = 0; i < h; i++) {
		d = buffer + dx + (dy + i) * rowSize;
		s = ogl.m_data.buffer + ((hScreen - (i + sy + 1)) * wScreen + sx) * 4;
		memcpy (d, s, w * 4);
		}
	}
else {
	sx += Left ();
	sy += Top ();
	for (i = 0; i < h; i++) {
		d = buffer + dx + (dy + i) * rowSize;
		s = ogl.m_data.buffer + ((hScreen - (i + sy + 1)) * wScreen + sx) * 3;
		for (j = 0; j < w; j++) {
			*d++ = dest->Palette ()->ClosestColor (s [0] / 4, s [1] / 4, s [2] / 4);
			s += 3;
			}
		}
	}
}

//------------------------------------------------------------------------------
