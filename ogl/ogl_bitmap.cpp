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

int G3DrawBitmap (const CFixVector&	vPos, fix width, fix height, CBitmap* bmP, tRgbaColorf* color, float alpha, int transp)
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
	if (color)
		glColor4f (color->red, color->green, color->blue, alpha);
	else
		glColor4d (1, 1, 1, (double) alpha);
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

static inline void BmSetTexCoord (GLfloat u, GLfloat v, GLfloat a, int orient)
{
if (orient == 1)
	glTexCoord2f ((1.0f - v) * a, u);
else if (orient == 2)
	glTexCoord2f (1.0f - u, 1.0f - v);
else if (orient == 3)
	glTexCoord2f (v * a, (1.0f - u));
else
	glTexCoord2f (u, v);
}

//------------------------------------------------------------------------------

int OglUBitMapMC (int x, int y, int dw, int dh, CBitmap *bmP, tCanvasColor *colorP, int scale, int orient)
{
	GLint		depthFunc, bBlend;
	GLfloat	xo, yo, xf, yf;
	GLfloat	u1, u2, v1, v2;
	GLfloat	h, a;
	GLfloat	dx, dy;
	CTexture	*texP;

bmP = bmP->Override (-1);
bmP->DelFlags (BM_FLAG_SUPER_TRANSPARENT);
if (dw < 0)
	dw = CCanvas::Current ()->Width ();
else if (dw == 0)
	dw = bmP->Width ();
if (dh < 0)
	dh = CCanvas::Current ()->Height ();
else if (dh == 0)
	dh = bmP->Height ();
if (orient & 1) {
	int h = dw;
	dw = dh;
	dh = h;
	dx = (float) CCanvas::Current ()->Top () / (float) gameStates.ogl.nLastH;
	dy = (float) CCanvas::Current ()->Left () / (float) gameStates.ogl.nLastW;
	}
else {
	dx = (float) CCanvas::Current ()->Left () / (float) gameStates.ogl.nLastW;
	dy = (float) CCanvas::Current ()->Top () / (float) gameStates.ogl.nLastH;
	}
a = (float) screen.Width () / (float) screen.Height ();
h = (float) scale / (float) I2X (1);
xo = dx + x / ((float) gameStates.ogl.nLastW * h);
xf = dx + (dw + x) / ((float) gameStates.ogl.nLastW * h);
yo = 1.0f - dy - y / ((float) gameStates.ogl.nLastH * h);
yf = 1.0f - dy - (dh + y) / ((float) gameStates.ogl.nLastH * h);

glActiveTexture (GL_TEXTURE0);
glEnable (GL_TEXTURE_2D);
if (bmP->Bind (0, 3))
	return 1;
texP = bmP->Texture ();
texP->Wrap (GL_CLAMP);

glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
glDepthFunc (GL_ALWAYS);
if (!(bBlend = glIsEnabled (GL_BLEND)))
	glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

if (bmP->Left () == 0) {
	u1 = 0;
	if (bmP->Width () == texP->Width ())
		u2 = texP->U ();
	else
		u2 = (float) bmP->Right () / (float) texP->TW ();
	}
else {
	u1 = (float) bmP->Left () / (float) texP->TW ();
	u2 = (float) bmP->Right () / (float) texP->TW ();
	}
if (bmP->Top () == 0) {
	v1 = 0;
	if (bmP->Height () == texP->Height ())
		v2 = texP->V ();
	else
		v2 = (float) bmP->Bottom () / (float) texP->TH ();
	}
else{
	v1 = bmP->Top () / (float) texP->TH ();
	v2 = (float) bmP->Bottom () / (float) texP->TH ();
	}

OglCanvasColor (colorP);
glBegin (GL_QUADS);
BmSetTexCoord (u1, v1, a, orient);
glVertex2f (xo, yo);
BmSetTexCoord (u2, v1, a, orient);
glVertex2f (xf, yo);
BmSetTexCoord (u2, v2, a, orient);
glVertex2f (xf, yf);
BmSetTexCoord (u1, v2, a, orient);
glVertex2f (xo, yf);
glEnd ();
glDepthFunc (depthFunc);
//glDisable (GL_ALPHA_TEST);
if (!bBlend)
	glDisable (GL_BLEND);
OglActiveTexture (GL_TEXTURE0, 0);
OGL_BINDTEX (0);
glDisable (GL_TEXTURE_2D);
return 0;
}

//------------------------------------------------------------------------------

void CBitmap::Render (CBitmap *dest, 
							 int xDest, int yDest, int wDest, int hDest, 
							 int xSrc, int ySrc, int wSrc, int hSrc, 
							 int bTransp, int bMipMaps, float fAlpha, tRgbaColorf* colorP)
{
	GLfloat xo, yo, xs, ys;
	GLfloat u1, v1;//, u2, v2;
	CTexture tex, *texP;
	GLint curFunc; 
	int nTransp = (Flags () & BM_FLAG_TGA) ? -1 : HasTransparency () ? 2 : 0;

//	ubyte *oldpal;
u1 = v1 = 0;
xDest += dest->Left ();
yDest += dest->Top ();
xo = xDest / (float) gameStates.ogl.nLastW;
xs = wDest / (float) gameStates.ogl.nLastW;
yo = 1.0f - yDest / (float) gameStates.ogl.nLastH;
ys = hDest / (float) gameStates.ogl.nLastH;

glActiveTexture (GL_TEXTURE0);
glEnable (GL_TEXTURE_2D);
if (!(texP = Texture ())) {
	texP = &tex;
	texP->Init ();
	texP->Setup (wSrc, hSrc, RowSize (), BPP (), 0, bMipMaps, this);
	SetTexture (texP);
	LoadTexture (xSrc, ySrc, nTransp, 0);
	}
else
	SetupTexture (0, bTransp, 1);
texP->Bind ();
texP->Wrap (GL_CLAMP);
glGetIntegerv (GL_DEPTH_FUNC, &curFunc);
glDepthFunc (GL_ALWAYS); 
if (bTransp && nTransp) {
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f (1.0, 1.0, 1.0, fAlpha);
	}
else {
	glDisable (GL_BLEND);
	glColor3f (1.0, 1.0, 1.0);
	}
glBegin (GL_QUADS);
if (colorP)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP));
glTexCoord2f (u1, v1); 
glVertex2f (xo, yo);
if (colorP)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP + 1));
glTexCoord2f (texP->U (), v1); 
glVertex2f (xo + xs, yo);
if (colorP)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP + 2));
glTexCoord2f (texP->U (), texP->V ()); 
glVertex2f (xo + xs, yo-ys);
if (colorP)
	glColor4fv (reinterpret_cast<GLfloat*> (colorP + 3));
glTexCoord2f (u1, texP->V ()); 
glVertex2f (xo, yo - ys);
glEnd ();
if (bTransp && nTransp)
	glDisable (GL_BLEND);
glDisable (GL_TEXTURE_2D);
glDepthFunc (curFunc);
if (texP == &tex)
	texP->Destroy ();
}

//------------------------------------------------------------------------------
// OglUBitBltToLinear
void CBitmap::RenderToBitmap (CBitmap * dest, int dx, int dy, int w, int h, int sx, int sy)
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
