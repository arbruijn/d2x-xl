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

#include "inferno.h"
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
	glTexCoord2f ((1.0f - v) * aspect, u);
else if (orient == 2)
	glTexCoord2f (1.0f - u, 1.0f - v);
else if (orient == 3)
	glTexCoord2f (v * aspect, 1.0f - u);
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
aspect = float (gameStates.ogl.nLastW * fScale);
x0 = dx + float (x) / aspect;
x1 = dx + float (x + w) / aspect;
aspect = float (gameStates.ogl.nLastH * fScale);
y0 = 1.0f - dy - float (y) / aspect;
y1 = 1.0f - dy - float (y + h) / aspect;
aspect = float (screen.Width ()) / float (screen.Height ());
}

//------------------------------------------------------------------------------

void CBitmap::OglTexCoord (void)
{
float h = float (m_info.texture->TW ());
u1 = float (Left ()) / h;
u2 = float (Right ()) / h;
h = float (m_info.texture->TH ());
v1 = float (Top ()) / h;
v2 = float (Bottom ()) / h;
}

//------------------------------------------------------------------------------

CTexture* CBitmap::OglBeginRender (bool bBlend, int bMipMaps, int nTransp)
{
glActiveTexture (GL_TEXTURE0);
glEnable (GL_TEXTURE_2D);
if (Bind (bMipMaps, nTransp))
	return NULL;
m_info.texture->Wrap (GL_CLAMP);

bBlendState = glIsEnabled (GL_BLEND);
glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
glDepthFunc (GL_ALWAYS);
if (bBlend)
	glEnable (GL_BLEND);
else
	glDisable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
return m_info.texture;
}

//------------------------------------------------------------------------------

void CBitmap::OglRender (tRgbaColorf* colorP, int nColors, int orient)
{
glBegin (GL_QUADS);
glColor4fv (reinterpret_cast<GLfloat*> (colorP));
SetTexCoord (u1, v1, orient);
glVertex2f (x0, y0);
if (nColors > 1)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP + 1));
SetTexCoord (u2, v1, orient);
glVertex2f (x1, y0);
if (nColors > 1)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP + 2));
SetTexCoord (u2, v2, orient);
glVertex2f (x1, y1);
if (nColors > 1)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP + 3));
SetTexCoord (u1, v2, orient);
glVertex2f (x0, y1);
glEnd ();
}

//------------------------------------------------------------------------------

void CBitmap::OglEndRender (void)
{
glDepthFunc (depthFunc);
if (bBlendState)
	glEnable (GL_BLEND);
else
	glDisable (GL_BLEND);
OglActiveTexture (GL_TEXTURE0, 0);
OGL_BINDTEX (0);
glDisable (GL_TEXTURE_2D);
}

//------------------------------------------------------------------------------

int CBitmap::OglUBitMapMC (int x, int y, int w, int h, int scale, int orient, tCanvasColor* colorP)
{
	CBitmap*		bmoP;
	tRgbaColorf	color;

if (bmoP = HasOverride ())
	return bmoP->OglUBitMapMC (x, y, w, h, scale, orient, colorP);
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

if (bmoP = HasOverride ())
	return bmoP->Render (destP, xDest, yDest, wDest, hDest, xSrc, ySrc, wSrc, hSrc, bTransp, bMipMaps, bSmoothe, fAlpha, colorP);

	int			nTransp = (Flags () & BM_FLAG_TGA) ? -1 : HasTransparency () ? 2 : 0;
	bool			bLocal = Texture () == NULL;

//	ubyte *oldpal;
OglVertices (xDest, yDest, wDest, hDest, I2X (1), 0, destP);
#if 0
if (!(texP = Texture ())) {
	texP = &tex;
	texP->Init ();
	texP->Setup (Width (), Height (), RowSize (), BPP (), 0, bMipMaps, bSmoothe, this);
	SetTexture (texP);
	LoadTexture (xSrc, ySrc, nTransp, 0);
	}
else
	SetupTexture (0, bTransp, 1);
#endif
tRgbaColorf color;
int nColors;
bool bBlend = bTransp && nTransp;

if (!colorP) {
	color.red = color.green = color.blue = 1.0f;
	color.alpha = bBlend ? fAlpha : 1.0f;
	colorP = &color;
	nColors = 1;
	}
else
	nColors = 4;

if (!OglBeginRender (bBlend, bMipMaps, nTransp))
	return 1; // fail

u1 = v1 = 0;
u2 = float (wSrc) / float (Width ());
if (u2 < 1.0f)
	u2 *= m_info.texture->U ();
else
	u2 = m_info.texture->U ();
v2 = float (hSrc) / float (Height ());
if (v2 < 1.0f)
	v2 *= m_info.texture->V ();
else
	v2 = m_info.texture->V ();

#if 1
OglRender (colorP, nColors, 0);
#else
glBegin (GL_QUADS);
if (colorP)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP));
glTexCoord2f (u1, v1); 
glVertex2f (x0, y0);
if (colorP)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP + 1));
glTexCoord2f (u2, v1); 
glVertex2f (x1, y0);
if (colorP)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP + 2));
glTexCoord2f (u2, v2); 
glVertex2f (x1, y1);
if (colorP)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP + 3));
glTexCoord2f (u1, v2); 
glVertex2f (x0, y1);
glEnd ();
#endif
OglEndRender ();

if (bLocal) {
	m_info.texture->Release ();
	SetTexture (NULL);
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
