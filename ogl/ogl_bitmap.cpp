/* $Id: ogl.c, v 1.14 204/05/11 23:15:55 btb Exp $ */
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

int G3DrawBitmap (
	vmsVector	*vPos, 
	fix			width, 
	fix			height, 
	grsBitmap	*bmP, 
	tRgbaColorf	*color,
	float			alpha, 
	int			transp)
{
	fVector		fPos;
	GLfloat		h, w, u, v;

r_bitmapc++;
OglActiveTexture (GL_TEXTURE0, 0);
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if 1
VmVecFixToFloat (&fPos, vPos);
G3TransformPointf (&fPos, &fPos, 0);
#else
VmVecSub (&v1, vPos, &viewInfo.vPos);
VmVecRotate (&pv, &v1, &viewInfo.view [0]);
#endif
w = (GLfloat) f2fl (width); //FixMul (width, viewInfo.scale.x));
h = (GLfloat) f2fl (height); //FixMul (height, viewInfo.scale.y));
if (gameStates.render.nShadowBlurPass == 1) {
	glDisable (GL_TEXTURE_2D);
	glColor4d (1,1,1,1);
	glBegin (GL_QUADS);
	fPos.p.x -= w;
	fPos.p.y += h;
	glVertex3fv ((GLfloat *) &fPos);
	fPos.p.x += 2 * w;
	glVertex3fv ((GLfloat *) &fPos);
	fPos.p.y -= 2 * h;
	glVertex3fv ((GLfloat *) &fPos);
	fPos.p.x -= 2 * w;
	glVertex3fv ((GLfloat *) &fPos);
	glEnd ();
	}
else {
	glEnable (GL_TEXTURE_2D);
	if (OglBindBmTex (bmP, 1, transp)) 
		return 1;
	bmP = BmOverride (bmP, -1);
	OglTexWrap (bmP->glTexture, GL_CLAMP);
	if (color)
		glColor4f (color->red, color->green, color->blue, alpha);
	else
		glColor4d (1, 1, 1, (double) alpha);
	u = bmP->glTexture->u;
	v = bmP->glTexture->v;
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	fPos.p.x -= w;
	fPos.p.y += h;
	glVertex3fv ((GLfloat *) &fPos);
	glTexCoord2f (u, 0);
	fPos.p.x += 2 * w;
	glVertex3fv ((GLfloat *) &fPos);
	glTexCoord2f (u, v);
	fPos.p.y -= 2 * h;
	glVertex3fv ((GLfloat *) &fPos);
	glTexCoord2f (0, v);
	fPos.p.x -= 2 * w;
	glVertex3fv ((GLfloat *) &fPos);
	glEnd ();
	}
return 0;
} 

//------------------------------------------------------------------------------

inline void BmSetTexCoord (GLfloat u, GLfloat v, GLfloat a, int orient)
{
switch (orient) {
	case 1:
		glMultiTexCoord2f (GL_TEXTURE0, (1.0f - v) * a, u);
		break;
	case 2:
		glMultiTexCoord2f (GL_TEXTURE0, 1.0f - u, 1.0f - v);
		break;
	case 3:
		glMultiTexCoord2f (GL_TEXTURE0, v * a, (1.0f - u));
		break;
	default:
		glMultiTexCoord2f (GL_TEXTURE0, u, v);
		break;
	}
}

//------------------------------------------------------------------------------

int OglUBitMapMC (int x, int y, int dw, int dh, grsBitmap *bmP, grsColor *c, int scale, int orient)
{
	GLint depthFunc, bBlend;
	GLfloat xo, yo, xf, yf;
	GLfloat u1, u2, v1, v2;
	GLfloat	h, a;
	GLfloat	dx, dy;

bmP = BmOverride (bmP, -1);
bmP->bmProps.flags &= ~BM_FLAG_SUPER_TRANSPARENT;
if (dw < 0)
	dw = grdCurCanv->cvBitmap.bmProps.w;
else if (dw == 0)
	dw = bmP->bmProps.w;
if (dh < 0)
	dh = grdCurCanv->cvBitmap.bmProps.h;
else if (dh == 0)
	dh = bmP->bmProps.h;
r_ubitmapc++;
if (orient & 1) {
	int h = dw;
	dw = dh;
	dh = h;
	dx = (float) grdCurCanv->cvBitmap.bmProps.y / (float) gameStates.ogl.nLastH;
	dy = (float) grdCurCanv->cvBitmap.bmProps.x / (float) gameStates.ogl.nLastW;
	}
else {
	dx = (float) grdCurCanv->cvBitmap.bmProps.x / (float) gameStates.ogl.nLastW;
	dy = (float) grdCurCanv->cvBitmap.bmProps.y / (float) gameStates.ogl.nLastH;
	}
#if 0
a = 1.0;
h = 1.0;
orient = 0;
#else
a = (float) grdCurScreen->scWidth / (float) grdCurScreen->scHeight;
h = (float) scale / (float) F1_0;
#endif
xo = dx + x / ((float) gameStates.ogl.nLastW * h);
xf = dx + (dw + x) / ((float) gameStates.ogl.nLastW * h);
yo = 1.0f - dy - y / ((float) gameStates.ogl.nLastH * h);
yf = 1.0f - dy - (dh + y) / ((float) gameStates.ogl.nLastH * h);

OglActiveTexture (GL_TEXTURE0, 0);
if (OglBindBmTex (bmP, 0, 3))
	return 1;
OglTexWrap (bmP->glTexture, GL_CLAMP);

glEnable (GL_TEXTURE_2D);
glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
glDepthFunc (GL_ALWAYS);
//glEnable (GL_ALPHA_TEST);
if (!(bBlend = glIsEnabled (GL_BLEND)))
	glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

if (bmP->bmProps.x == 0) {
	u1 = 0;
	if (bmP->bmProps.w == bmP->glTexture->w)
		u2 = bmP->glTexture->u;
	else
		u2 = (bmP->bmProps.w + bmP->bmProps.x) / (float) bmP->glTexture->tw;
	}
else {
	u1 = bmP->bmProps.x / (float) bmP->glTexture->tw;
	u2 = (bmP->bmProps.w + bmP->bmProps.x) / (float) bmP->glTexture->tw;
	}
if (bmP->bmProps.y == 0) {
	v1 = 0;
	if (bmP->bmProps.h == bmP->glTexture->h)
		v2 = bmP->glTexture->v;
	else
		v2 = (bmP->bmProps.h + bmP->bmProps.y) / (float) bmP->glTexture->th;
	}
else{
	v1 = bmP->bmProps.y / (float) bmP->glTexture->th;
	v2 = (bmP->bmProps.h + bmP->bmProps.y) / (float) bmP->glTexture->th;
	}

OglGrsColor (c);
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

int OglUBitBltI (
	int dw, int dh, int dx, int dy, 
	int sw, int sh, int sx, int sy, 
	grsBitmap *src, grsBitmap *dest, 
	int bMipMaps, int bTransp, float fAlpha)
{
	GLfloat xo, yo, xs, ys;
	GLfloat u1, v1;//, u2, v2;
	tOglTexture tex, *texP;
	GLint curFunc; 
	int nTransp = (src->bmProps.flags & BM_FLAG_TGA) ? -1 : GrBitmapHasTransparency (src) ? 2 : 0;

//	unsigned char *oldpal;
r_ubitbltc++;

u1 = v1 = 0;
dx += dest->bmProps.x;
dy += dest->bmProps.y;
xo = dx / (float) gameStates.ogl.nLastW;
xs = dw / (float) gameStates.ogl.nLastW;
yo = 1.0f - dy / (float) gameStates.ogl.nLastH;
ys = dh / (float) gameStates.ogl.nLastH;

glActiveTexture (GL_TEXTURE0);
glEnable (GL_TEXTURE_2D);
if (!(texP = src->glTexture)) {
	texP = &tex;
	OglInitTexture (texP, 0);
	texP->w = sw;
	texP->h = sh;
	texP->prio = 0.0;
	texP->bMipMaps = bMipMaps;
	texP->lw = src->bmProps.rowSize;
	OglLoadTexture (src, sx, sy, texP, nTransp, 0);
	}
else
	OglLoadBmTexture (src, 0, bTransp, 1);
OGL_BINDTEX (texP->handle);
OglTexWrap (texP, GL_CLAMP);
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
glTexCoord2f (u1, v1); 
glVertex2f (xo, yo);
glTexCoord2f (texP->u, v1); 
glVertex2f (xo + xs, yo);
glTexCoord2f (texP->u, texP->v); 
glVertex2f (xo + xs, yo-ys);
glTexCoord2f (u1, texP->v); 
glVertex2f (xo, yo - ys);
glEnd ();
if (bTransp && nTransp)
	glDisable (GL_BLEND);
glDisable (GL_TEXTURE_2D);
glDepthFunc (curFunc);
if (!src->glTexture)
	OglFreeTexture (texP);
return 0;
}

//------------------------------------------------------------------------------

int OglUBitBltToLinear (int w, int h, int dx, int dy, int sx, int sy, 
								 grsBitmap * src, grsBitmap * dest)
{
	unsigned char *d, *s;
	int i, j;
	int w1, h1;
	int bTGA = (dest->bmProps.flags & BM_FLAG_TGA) != 0;
//	w1=w;h1=h;
w1=grdCurScreen->scWidth;
h1=grdCurScreen->scHeight;

if (gameStates.ogl.bReadPixels > 0) {
	glDisable (GL_TEXTURE_2D);
	OglReadBuffer (GL_FRONT, 1);
	if (bTGA)
		glReadPixels (0, 0, w1, h1, GL_RGBA, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
//			glReadPixels (sx, grdCurScreen->scHeight - (sy + h), w, h, GL_RGBA, GL_UNSIGNED_BYTE, dest->bmTexBuf);
	else {
		if (w1*h1*3>OGLTEXBUFSIZE)
			Error ("OglUBitBltToLinear: screen res larger than OGLTEXBUFSIZE\n");
		glReadPixels (0, 0, w1, h1, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
		}
//		glReadPixels (sx, grdCurScreen->scHeight- (sy+h), w, h, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
//		glReadPixels (sx, sy, w+sx, h+sy, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
	}
else {
	if (dest->bmProps.flags & BM_FLAG_TGA)
		memset (dest->bmTexBuf, 0, w1*h1*4);
	else
		memset (gameData.render.ogl.texBuf, 0, w1*h1*3);
	}
if (bTGA) {
	sx += src->bmProps.x;
	sy += src->bmProps.y;
	for (i = 0; i < h; i++) {
		d = dest->bmTexBuf + dx + (dy + i) * dest->bmProps.rowSize;
		s = gameData.render.ogl.texBuf + ((h1 - (i + sy + 1)) * w1 + sx) * 4;
		memcpy (d, s, w * 4);
		}
/*
	for (i = 0; i < h; i++)
		memcpy (dest->bmTexBuf + (h - i - 1) * dest->bmProps.rowSize, 
				  gameData.render.ogl.texBuf + ((sy + i) * h1 + sx) * 4, 
				  dest->bmProps.rowSize);
*/
	}
else {
	sx += src->bmProps.x;
	sy += src->bmProps.y;
	for (i = 0; i < h; i++) {
		d = dest->bmTexBuf + dx + (dy + i) * dest->bmProps.rowSize;
		s = gameData.render.ogl.texBuf + ((h1 - (i + sy + 1)) * w1 + sx) * 3;
		for (j = 0; j < w; j++) {
			*d++ = GrFindClosestColor (dest->bmPalette, s [0] / 4, s [1] / 4, s [2] / 4);
			s += 3;
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int OglUBitBltCopy (int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest)
{
#if 0 //just seems to cause a mess.
	GLdouble xo, yo;//, xs, ys;

	dx+=dest->bmProps.x;
	dy+=dest->bmProps.y;

//	xo=dx/ (double)gameStates.ogl.nLastW;
	xo=dx/ (double)grdCurScreen->scWidth;
//	yo=1.0- (dy+h)/ (double)gameStates.ogl.nLastH;
	yo=1.0- (dy+h)/ (double)grdCurScreen->scHeight;
	sx+=src->bmProps.x;
	sy+=src->bmProps.y;
	glDisable (GL_TEXTURE_2D);
	OglReadBuffer (GL_FRONT, 1);
	glRasterPos2f (xo, yo);
//	glReadPixels (0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
	glCopyPixels (sx, grdCurScreen->scHeight- (sy+h), w, h, GL_COLOR);
	glRasterPos2f (0, 0);
#endif
	return 0;
}

//------------------------------------------------------------------------------
