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
# include <malloc.h>
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

int G3DrawBitmap (const CFixVector&	vPos, fix width, fix height, CBitmap* bmP, tRgbaColorf* colorP, float alpha, int transp)
{
	CFloatVector	fPos;
	GLfloat			h, w, u, v;

r_bitmapc++;
OglActiveTexture (GL_TEXTURE0, 0);
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if 1
fPos.Assign (vPos);
transformation.Transform (fPos, fPos, 0);
#else
v1 = vPos[0] - transformation.m_info.vPos;
VmVecRotate (&pv, &v1, &transformation.m_info.view [0]);
#endif
w = (GLfloat) X2F (width); //FixMul (width, transformation.m_info.scale.x));
h = (GLfloat) X2F (height); //FixMul (height, transformation.m_info.scale.y));
if (gameStates.render.nShadowBlurPass == 1) {
	glDisable (GL_TEXTURE_2D);
	glColor4d (1,1,1,1);
	glBegin (GL_QUADS);
	fPos[X] -= w;
	fPos[Y] += h;
	glVertex3fv (reinterpret_cast<GLfloat*> (&fPos));
	fPos[X] += 2 * w;
	glVertex3fv (reinterpret_cast<GLfloat*> (&fPos));
	fPos[Y] -= 2 * h;
	glVertex3fv (reinterpret_cast<GLfloat*> (&fPos));
	fPos[X] -= 2 * w;
	glVertex3fv (reinterpret_cast<GLfloat*> (&fPos));
	glEnd ();
	}
else {
	glEnable (GL_TEXTURE_2D);
	if (bmP->Bind (1, transp))
		return 1;
	bmP = bmP->Override (-1);
	bmP->Texture ()->Wrap (GL_CLAMP);
	if (colorP)
		glColor4f (colorP->red, colorP->green, colorP->blue, alpha);
	else
		glColor4d (1, 1, 1, double (alpha));
	u = bmP->Texture ()->U ();
	v = bmP->Texture ()->V ();
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	fPos [X] -= w;
	fPos [Y] += h;
	glVertex3fv (reinterpret_cast<GLfloat*> (&fPos));
	glTexCoord2f (u, 0);
	fPos[X] += 2 * w;
	glVertex3fv (reinterpret_cast<GLfloat*> (&fPos));
	glTexCoord2f (u, v);
	fPos[Y] -= 2 * h;
	glVertex3fv (reinterpret_cast<GLfloat*> (&fPos));
	glTexCoord2f (0, v);
	fPos[X] -= 2 * w;
	glVertex3fv (reinterpret_cast<GLfloat*> (&fPos));
	glEnd ();
	}
return 0;
}

//------------------------------------------------------------------------------

void CBitmap::SetTexCoord (GLfloat u, GLfloat v, int orient)
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

void CBitmap::OglVertices (int x, int y, int w, int h, int scale, int orient, CBitmap* destP)
{
if (!destP)
	destP = CCanvas::Current ();

if (!w)
	w = Width ();
else if (w < 0)
	w = destP->Width ();

if (!h)
	h = Height ();
else if (h < 0)
	h = destP->Height ();

float dx = float (destP->Left ()) / float (gameStates.ogl.nLastW);
float dy = float (destP->Top ()) / float (gameStates.ogl.nLastH);

if (orient & 1) {
	::Swap (w, h);
	::Swap (dx, dy);
	}

float fScale = X2F (scale);
m_render.aspect = float (gameStates.ogl.nLastW * fScale);
m_render.x0 = dx + float (x) / m_render.aspect;
m_render.x1 = dx + float (x + w) / m_render.aspect;
m_render.aspect = float (gameStates.ogl.nLastH * fScale);
m_render.y0 = 1.0f - dy - float (y) / m_render.aspect;
m_render.y1 = 1.0f - dy - float (y + h) / m_render.aspect;
m_render.aspect = float (screen.Width ()) / float (screen.Height ());
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

CTexture* CBitmap::OglBeginRender (bool bBlend, int bMipMaps, int nTransp)
{
OglClearError (1);
glEnable (GL_TEXTURE_2D);
glActiveTexture (GL_TEXTURE0);
if (Bind (bMipMaps, nTransp))
	return NULL;
m_info.texP->Wrap (GL_REPEAT);

m_render.bBlendState = glIsEnabled (GL_BLEND);
glGetIntegerv (GL_DEPTH_FUNC, &m_render.depthFunc);
glDepthFunc (GL_ALWAYS);
if (bBlend)
	glEnable (GL_BLEND);
else
	glDisable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
return &m_info.texture;
}

//------------------------------------------------------------------------------

void CBitmap::OglRender (tRgbaColorf* colorP, int nColors, int orient)
{
glBegin (GL_QUADS);
glColor4fv (reinterpret_cast<GLfloat*> (colorP));
SetTexCoord (m_render.u1, m_render.v1, orient);
glVertex2f (m_render.x0, m_render.y0);
if (nColors > 1)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP + 1));
SetTexCoord (m_render.u2, m_render.v1, orient);
glVertex2f (m_render.x1, m_render.y0);
if (nColors > 1)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP + 2));
SetTexCoord (m_render.u2, m_render.v2, orient);
glVertex2f (m_render.x1, m_render.y1);
if (nColors > 1)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP + 3));
SetTexCoord (m_render.u1, m_render.v2, orient);
glVertex2f (m_render.x0, m_render.y1);
glEnd ();
}

//------------------------------------------------------------------------------

void CBitmap::OglEndRender (void)
{
glDepthFunc (m_render.depthFunc);
if (m_render.bBlendState)
	glEnable (GL_BLEND);
else
	glDisable (GL_BLEND);
OglActiveTexture (GL_TEXTURE0, 0);
OGL_BINDTEX (0);
glDisable (GL_TEXTURE_2D);
}

//------------------------------------------------------------------------------

int CBitmap::RenderScaled (int x, int y, int w, int h, int scale, int orient, tCanvasColor* colorP)
{
	CBitmap*		bmoP;
	tRgbaColorf	color;

if ((bmoP = HasOverride ()))
	return bmoP->RenderScaled (x, y, w, h, scale, orient, colorP);
DelFlags (BM_FLAG_SUPER_TRANSPARENT);
if (!OglBeginRender (true, 0, 3))
	return 1; // fail
OglVertices (x, y, w, h, scale, orient);
OglTexCoord ();
color = GetCanvasColor (colorP);
OglRender (&color, 1, orient);
OglEndRender ();
return 0;
}

//------------------------------------------------------------------------------

int CBitmap::Render (CBitmap *destP,
							int xDest, int yDest, int wDest, int hDest,
							int xSrc, int ySrc, int wSrc, int hSrc,
							int bTransp, int bMipMaps, int bSmoothe,
							float fAlpha, tRgbaColorf* colorP)
{
	CBitmap*		bmoP;

if ((bmoP = HasOverride ()))
	return bmoP->Render (destP, xDest, yDest, wDest, hDest, xSrc, ySrc, wSrc, hSrc, bTransp, bMipMaps, bSmoothe, fAlpha, colorP);

	int			nTransp = (Flags () & BM_FLAG_TGA) ? -1 : HasTransparency () ? 2 : 0;
	bool			bLocal = Texture () == NULL;

//	ubyte *oldpal;
OglVertices (xDest, yDest, wDest, hDest, I2X (1), 0, destP);
tRgbaColorf color;
int nColors;
bool bBlend = bTransp && nTransp;

if (!colorP) {
	color.red = color.green = color.blue = 1.0f;
	color.alpha = bBlend ? fAlpha * gameStates.render.grAlpha : 1.0f;
	colorP = &color;
	nColors = 1;
	}
else
	nColors = 4;

if (!OglBeginRender (bBlend, bMipMaps, nTransp))
	return 1; // fail

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
void CBitmap::ScreenCopy (CBitmap * dest, int dx, int dy, int w, int h, int sx, int sy)
{
	ubyte *d, *s;
	int	i, j;
	int	bTGA = (dest->Flags () & BM_FLAG_TGA) != 0;
	int	wScreen = screen.Width ();
	int	hScreen = screen.Height ();

if (gameStates.ogl.bReadPixels > 0) {
	glDisable (GL_TEXTURE_2D);
	OglSetReadBuffer (GL_FRONT, 1);
	if (bTGA)
		glReadPixels (0, 0, wScreen, hScreen, GL_RGBA, GL_UNSIGNED_BYTE, gameData.render.ogl.buffer);
//			glReadPixels (sx, screen.Height () - (sy + h), w, h, GL_RGBA, GL_UNSIGNED_BYTE, dest->Buffer ());
	else {
		if (wScreen * hScreen * 3 > OGLTEXBUFSIZE)
			Error ("OglUBitBltToLinear: screen res larger than OGLTEXBUFSIZE\n");
		glReadPixels (0, 0, wScreen, hScreen, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.buffer);
		}
//		glReadPixels (sx, screen.Height ()- (sy+h), w, h, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.buffer);
//		glReadPixels (sx, sy, w+sx, h+sy, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.buffer);
	}
else {
	memset (gameData.render.ogl.buffer, 0, wScreen * hScreen * (3 + bTGA));
	}

ubyte* buffer = dest->Buffer ();
int rowSize = dest->RowSize ();

if (bTGA) {
	sx += Left ();
	sy += Top ();
	for (i = 0; i < h; i++) {
		d = buffer + dx + (dy + i) * rowSize;
		s = gameData.render.ogl.buffer + ((hScreen - (i + sy + 1)) * wScreen + sx) * 4;
		memcpy (d, s, w * 4);
		}
	}
else {
	sx += Left ();
	sy += Top ();
	for (i = 0; i < h; i++) {
		d = buffer + dx + (dy + i) * rowSize;
		s = gameData.render.ogl.buffer + ((hScreen - (i + sy + 1)) * wScreen + sx) * 3;
		for (j = 0; j < w; j++) {
			*d++ = dest->Palette ()->ClosestColor (s [0] / 4, s [1] / 4, s [2] / 4);
			s += 3;
			}
		}
	}
}

//------------------------------------------------------------------------------
