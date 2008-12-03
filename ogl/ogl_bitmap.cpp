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
	const vmsVector&	vPos, 
	fix					width, 
	fix					height, 
	CBitmap				*bmP, 
	tRgbaColorf			*color,
	float					alpha, 
	int					transp)
{
	fVector		fPos;
	GLfloat		h, w, u, v;

r_bitmapc++;
OglActiveTexture (GL_TEXTURE0, 0);
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if 1
fPos = vPos.ToFloat();
G3TransformPoint (fPos, fPos, 0);
#else
v1 = vPos[0] - viewInfo.vPos;
VmVecRotate (&pv, &v1, &viewInfo.view [0]);
#endif
w = (GLfloat) X2F (width); //FixMul (width, viewInfo.scale.x));
h = (GLfloat) X2F (height); //FixMul (height, viewInfo.scale.y));
if (gameStates.render.nShadowBlurPass == 1) {
	glDisable (GL_TEXTURE_2D);
	glColor4d (1,1,1,1);
	glBegin (GL_QUADS);
	fPos[X] -= w;
	fPos[Y] += h;
	glVertex3fv ((GLfloat *) &fPos);
	fPos[X] += 2 * w;
	glVertex3fv ((GLfloat *) &fPos);
	fPos[Y] -= 2 * h;
	glVertex3fv ((GLfloat *) &fPos);
	fPos[X] -= 2 * w;
	glVertex3fv ((GLfloat *) &fPos);
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
	glVertex3fv ((GLfloat *) &fPos);
	glTexCoord2f (u, 0);
	fPos[X] += 2 * w;
	glVertex3fv ((GLfloat *) &fPos);
	glTexCoord2f (u, v);
	fPos[Y] -= 2 * h;
	glVertex3fv ((GLfloat *) &fPos);
	glTexCoord2f (0, v);
	fPos[X] -= 2 * w;
	glVertex3fv ((GLfloat *) &fPos);
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

int OglUBitMapMC (int x, int y, int dw, int dh, CBitmap *bmP, tCanvasColor *c, int scale, int orient)
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
	dh = CCanvas::Current ()->Bitmap ().Height ();
else if (dh == 0)
	dh = bmP->Height ();
if (orient & 1) {
	int h = dw;
	dw = dh;
	dh = h;
	dx = (float) CCanvas::Current ()->.Top () / (float) gameStates.ogl.nLastH;
	dy = (float) CCanvas::Current ()->.Left () / (float) gameStates.ogl.nLastW;
	}
else {
	dx = (float) CCanvas::Current ()->.Left () / (float) gameStates.ogl.nLastW;
	dy = (float) CCanvas::Current ()->.Top () / (float) gameStates.ogl.nLastH;
	}
a = (float) screen.Width () / (float) screen.Height ();
h = (float) scale / (float) F1_0;
xo = dx + x / ((float) gameStates.ogl.nLastW * h);
xf = dx + (dw + x) / ((float) gameStates.ogl.nLastW * h);
yo = 1.0f - dy - y / ((float) gameStates.ogl.nLastH * h);
yf = 1.0f - dy - (dh + y) / ((float) gameStates.ogl.nLastH * h);

OglActiveTexture (GL_TEXTURE0, 0);
if (bmP->Bind (0, 3))
	return 1;
texP = bmP->Texture ();
texP->Wrap (GL_CLAMP);

glEnable (GL_TEXTURE_2D);
glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
glDepthFunc (GL_ALWAYS);
if (!(bBlend = glIsEnabled (GL_BLEND)))
	glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

if (bmP->Left () == 0) {
	u1 = 0;
	if (bmP->Width () == texP->w)
		u2 = texP->u;
	else
		u2 = (float) bmP->Right () / (float) texP->TW ();
	}
else {
	u1 = (float) bmP->Left () / (float) texP->TW ();
	u2 = (float) bmP->Right () / (float) texP->TW ();
	}
if (bmP->Top () == 0) {
	v1 = 0;
	if (bmP->Height () == texP->h)
		v2 = texP->v;
	else
		v2 = (float) bmP->Bottom () / (float) texP->TH ();
	}
else{
	v1 = bmP->Top () / (float) texP->TH ();
	v2 = (float) bmP->Bottom () / (float) texP->TH ();
	}

OglCanvasColor (c);
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
	CBitmap *src, CBitmap *dest, 
	int bMipMaps, int bTransp, float fAlpha)
{
	GLfloat xo, yo, xs, ys;
	GLfloat u1, v1;//, u2, v2;
	CTexture tex, *texP;
	GLint curFunc; 
	int nTransp = (src->Flags () & BM_FLAG_TGA) ? -1 : src->HasTransparency () ? 2 : 0;

//	unsigned char *oldpal;
r_ubitbltc++;

u1 = v1 = 0;
dx += dest->Left ();
dy += dest->Top ();
xo = dx / (float) gameStates.ogl.nLastW;
xs = dw / (float) gameStates.ogl.nLastW;
yo = 1.0f - dy / (float) gameStates.ogl.nLastH;
ys = dh / (float) gameStates.ogl.nLastH;

glActiveTexture (GL_TEXTURE0);
glEnable (GL_TEXTURE_2D);
if (!(texP = src->Texture ())) {
	texP = &tex;
	OglInitTexture (texP, 0, NULL);
	texP->w = sw;
	texP->h = sh;
	texP->prio = 0.0;
	texP->bMipMaps = bMipMaps;
	texP->lw = src->RowSize ();
	OglLoadTexture (src, sx, sy, texP, nTransp, 0);
	}
else
	src->SetupTexture (0, bTransp, 1);
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
if (!src->Texture())
	OglFreeTexture (texP);
return 0;
}

//------------------------------------------------------------------------------

int OglUBitBltToLinear (int w, int h, int dx, int dy, int sx, int sy, 
								 CBitmap * src, CBitmap * dest)
{
	unsigned char *d, *s;
	int i, j;
	int w1, h1;
	int bTGA = (dest->Flags () & BM_FLAG_TGA) != 0;
//	w1=w;h1=h;
w1=screen.Width ();
h1=screen.Height ();

if (gameStates.ogl.bReadPixels > 0) {
	glDisable (GL_TEXTURE_2D);
	OglSetReadBuffer (GL_FRONT, 1);
	if (bTGA)
		glReadPixels (0, 0, w1, h1, GL_RGBA, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
//			glReadPixels (sx, screen.Height () - (sy + h), w, h, GL_RGBA, GL_UNSIGNED_BYTE, dest->TexBuf ());
	else {
		if (w1*h1*3>OGLTEXBUFSIZE)
			Error ("OglUBitBltToLinear: screen res larger than OGLTEXBUFSIZE\n");
		glReadPixels (0, 0, w1, h1, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
		}
//		glReadPixels (sx, screen.Height ()- (sy+h), w, h, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
//		glReadPixels (sx, sy, w+sx, h+sy, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
	}
else {
	if (dest->Flags () & BM_FLAG_TGA)
		memset (dest->TexBuf (), 0, w1*h1*4);
	else
		memset (gameData.render.ogl.texBuf, 0, w1*h1*3);
	}
if (bTGA) {
	sx += src->Left ();
	sy += src->Top ();
	for (i = 0; i < h; i++) {
		d = dest->TexBuf () + dx + (dy + i) * dest->RowSize ();
		s = gameData.render.ogl.texBuf + ((h1 - (i + sy + 1)) * w1 + sx) * 4;
		memcpy (d, s, w * 4);
		}
/*
	for (i = 0; i < h; i++)
		memcpy (dest->TexBuf () + (h - i - 1) * dest->RowSize (), 
				  gameData.render.ogl.texBuf + ((sy + i) * h1 + sx) * 4, 
				  dest->RowSize ());
*/
	}
else {
	sx += src->Left ();
	sy += src->Top ();
	for (i = 0; i < h; i++) {
		d = dest->TexBuf () + dx + (dy + i) * dest->RowSize ();
		s = gameData.render.ogl.texBuf + ((h1 - (i + sy + 1)) * w1 + sx) * 3;
		for (j = 0; j < w; j++) {
			*d++ = dest->Palette ()->ClosestColor (s [0] / 4, s [1] / 4, s [2] / 4);
			s += 3;
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int OglUBitBltCopy (int w, int h, int dx, int dy, int sx, int sy, CBitmap * src, CBitmap * dest)
{
#if 0 //just seems to cause a mess.
	GLdouble xo, yo;//, xs, ys;

	dx+=dest->Left ();
	dy+=dest->Top ();

//	xo=dx/ (double)gameStates.ogl.nLastW;
	xo=dx/ (double)screen.Width ();
//	yo=1.0- (dy+h)/ (double)gameStates.ogl.nLastH;
	yo=1.0- (dy+h)/ (double)screen.Height ();
	sx+=src->Left ();
	sy+=src->Top ();
	glDisable (GL_TEXTURE_2D);
	OglSetReadBuffer (GL_FRONT, 1);
	glRasterPos2f (xo, yo);
//	glReadPixels (0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
	glCopyPixels (sx, screen.Height ()- (sy+h), w, h, GL_COLOR);
	glRasterPos2f (0, 0);
#endif
	return 0;
}

//------------------------------------------------------------------------------
