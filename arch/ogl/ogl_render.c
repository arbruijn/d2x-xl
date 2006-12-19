/* $Id: ogl.c, v 1.14 2004/05/11 23:15:55 btb Exp $ */
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

#include "3d.h"
#include "u_mem.h"
#include "piggy.h"
#include "../../3d/globvars.h"
#include "error.h"
#include "texmap.h"
#include "palette.h"
#include "rle.h"
#include "mono.h"

#include "inferno.h"
#include "textures.h"
#include "texmerge.h"
#include "effects.h"
#include "weapon.h"
#include "powerup.h"
#include "polyobj.h"
#include "gamefont.h"
#include "byteswap.h"
#include "cameras.h"
#include "render.h"
#include "grdef.h"
#include "ogl_init.h"
#include "network.h"
#include "lightmap.h"
#include "mouse.h"
#include "gameseg.h"
#include "lighting.h"

#ifndef M_PI
#	define M_PI 3.141592653589793240
#endif

#define OGL_CLEANUP		1
#define USE_VERTNORMS	1

#if DBG_SHADOWS
int bShadowTest = 0;
#endif

extern int bZPass;
int bSingleStencil = 0;

tRgbaColorf shadowColor [2] = {{1.0f, 0.0f, 0.0f, 0.25f}, {0.0f, 0.0f, 1.0f, 0.25f}};
tRgbaColorf modelColor [2] = {{0.0f, 0.5f, 1.0f, 0.5f}, {0.0f, 1.0f, 0.5f, 0.5f}};

#define COORTRANS 0

#if defined (_WIN32) || defined (__sun__)
#	define cosf(a) cos (a)
#	define sinf(a) sin (a)
#endif

extern int gr_renderstats;
extern int gr_badtexture;

extern tRenderQuality renderQualities [];
	
GLuint bsphereh=0;
GLuint ssphereh=0;
GLuint circleh5=0;
GLuint circleh10=0;
GLuint mouseIndList = 0;
GLuint cross_lh [2]={0, 0};
GLuint primary_lh [3]={0, 0, 0};
GLuint secondary_lh [5]={0, 0, 0, 0, 0};
GLuint glInitTMU [4]= {0, 0, 0, 0};
GLuint glExitTMU = 0;

int OglBindBmTex (grsBitmap *bm, int transp);
void ogl_clean_texture_cache (void);
/*inline*/ void SetTMapColor (uvl *uvlList, int i, grsBitmap *bm, int bResetColor);

//------------------------------------------------------------------------------

void OglPalColor (ubyte *palette, int c)
{
	GLfloat	fc [4];

if (c < 0)
	glColor3f (1.0, 1.0, 1.0);
else {
	if (!palette)
		palette = gamePalette;
	if (!palette)
		palette = defaultPalette;
	c *= 3;
	fc [0] = (float) (palette [c]) / 63.0f;
	fc [1] = (float) (palette [c]) / 63.0f;
	fc [2] = (float) (palette [c]) / 63.0f;
	if (gameStates.render.grAlpha >= GR_ACTUAL_FADE_LEVELS)
		fc [3] = 1.0f;
	else {
		fc [3] = (float) gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS; //1.0f - (float) gameStates.render.grAlpha / ((float) GR_ACTUAL_FADE_LEVELS - 1.0f);
		glEnable (GL_BLEND);
		}
	glColor4fv (fc);
	}
}

//------------------------------------------------------------------------------

void OglBlendFunc (GLenum nSrcBlend, GLenum nDestBlend)
{
gameData.render.ogl.nSrcBlend = nSrcBlend;
gameData.render.ogl.nDestBlend = nDestBlend;
glBlendFunc (nSrcBlend, nDestBlend);
}

//------------------------------------------------------------------------------

void OglGrsColor (grs_color *pc)
{
	GLfloat	fc [4];

if (!pc)
	glColor3f (1.0, 1.0, 1.0);
else if (pc->rgb) {
	fc [0] = (float) (pc->color.red) / 255.0f;
	fc [1] = (float) (pc->color.green) / 255.0f;
	fc [2] = (float) (pc->color.blue) / 255.0f;
	fc [3] = (float) (pc->color.alpha) / 255.0f;
	if (fc [3] < 1.0f) {
		glEnable (GL_BLEND);
		glBlendFunc (gameData.render.ogl.nSrcBlend, gameData.render.ogl.nDestBlend);
		}
	glColor4fv (fc);
	}
else
	OglPalColor (gamePalette, pc->index);
}

//------------------------------------------------------------------------------

int r_polyc, r_tpolyc, r_bitmapc, r_ubitmapc, r_ubitbltc, r_upixelc;

#define f2glf(x) (f2fl (x))

bool G3DrawLine (g3sPoint *p0, g3sPoint *p1)
{
glDisable (GL_TEXTURE_2D);
OglGrsColor (&grdCurCanv->cv_color);
glBegin (GL_LINES);
glVertex3x (p0->p3_vec.x, p0->p3_vec.y, p0->p3_vec.z);
glVertex3x (p1->p3_vec.x, p1->p3_vec.y, p1->p3_vec.z);
if (grdCurCanv->cv_color.rgb)
	glDisable (GL_BLEND);
glEnd ();
return 1;
}

//------------------------------------------------------------------------------

void OglDrawCircle2 (int nSides, int nType, double xsc, double xo, double ysc, double yo)
{
	int i;
	double ang;

glBegin (nType);
for (i = 0; i < nSides; i++) {
	ang = 2.0 * M_PI * i / nSides;
	glVertex2d (cos (ang) * xsc + xo, sin (ang) * ysc + yo);
	}
glEnd ();
}

//------------------------------------------------------------------------------

void OglDrawCircle (int nSides, int nType)
{
	int i;
	double ang;
	glBegin (nType);
	for (i = 0; i < nSides; i++) {
		ang = 2.0 * M_PI * i / nSides;
		glVertex2d (cos (ang), sin (ang));
	}
	glEnd ();
}

//------------------------------------------------------------------------------

int CircleListInit (int nSides, int nType, int mode) 
{
	int hand=glGenLists (1);
	glNewList (hand, mode);
	/* draw a unit radius circle in xy plane centered on origin */
	OglDrawCircle (nSides, nType);
	glEndList ();
	return hand;
}

//------------------------------------------------------------------------------

void OglDrawMouseIndicator (void)
{
	double scale = (double) grdCurScreen->sc_w / (double) grdCurScreen->sc_h;

glPushMatrix ();
glTranslated (
	(double) (mouseData.x) / (double) SWIDTH, 
	1.0 - (double) (mouseData.y) / (double) SHEIGHT, 
	0);
glScaled (scale / 320.0, scale / 200.0, scale);//the positions are based upon the standard reticle at 320x200 res.
glDisable (GL_TEXTURE_2D);
#if 1
if (mouseIndList)
	glCallList (mouseIndList);
else {
	glNewList (mouseIndList, GL_COMPILE_AND_EXECUTE);
#endif
	glColor3d (1.0, 0.8, 0.0);
	glLineWidth (3);
	glEnable (GL_SMOOTH);
	OglDrawCircle2 (12, GL_LINE_LOOP, 1.5, 0, 1.5 * (double) grdCurScreen->sc_h / (double) grdCurScreen->sc_w, 0);
	glDisable (GL_SMOOTH);
	glLineWidth (1);
#if 1
	glEndList ();
   }
#endif
glPopMatrix ();
}

//------------------------------------------------------------------------------

float bright_g [4]={32.0/256, 252.0/256, 32.0/256, 1.0};
float dark_g  [4]={32.0/256, 148.0/256, 32.0/256, 1.0};
float darker_g [4]={32.0/256, 128.0/256, 32.0/256, 1.0};

void OglDrawReticle (int cross, int primary, int secondary)
{
	double scale = (double)nCanvasHeight/ (double)grdCurScreen->sc_h;

glPushMatrix ();
//	glTranslated (0.5, 0.5, 0);
glTranslated (
	(grdCurCanv->cv_bitmap.bm_props.w/2+grdCurCanv->cv_bitmap.bm_props.x)/ (double) gameStates.ogl.nLastW, 
	1.0 - (grdCurCanv->cv_bitmap.bm_props.h/ ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) ? 2 : 2)+grdCurCanv->cv_bitmap.bm_props.y)/ (double)gameStates.ogl.nLastH, 
	0);
glScaled (scale/320.0, scale/200.0, scale);//the positions are based upon the standard reticle at 320x200 res.
glDisable (GL_TEXTURE_2D);

glLineWidth (5);
glEnable (GL_SMOOTH);
if (cross_lh [cross])
	glCallList (cross_lh [cross]);
else {
	cross_lh [cross] = glGenLists (1);
	glNewList (cross_lh [cross], GL_COMPILE_AND_EXECUTE);
	glBegin (GL_LINES);
	//cross top left
	glColor3fv (darker_g);
	glVertex2d (-4.0, 4.0);
	glColor3fv (cross ? bright_g : dark_g);
	glVertex2d (-2.0, 2.0);
	//cross bottom left
	glColor3fv (dark_g);
	glVertex2d (-3.0, -2.0);
	if (cross)
		glColor3fv (bright_g);
	glVertex2d (-2.0, -1.0);
	//cross top right
	glColor3fv (darker_g);
	glVertex2d (4.0, 4.0);
	glColor3fv (cross ? bright_g : dark_g);
	glVertex2d (2.0, 2.0);
	//cross bottom right
	glColor3fv (dark_g);
	glVertex2d (3.0, -2.0);
	if (cross)
		glColor3fv (bright_g);
	glVertex2d (2.0, -1.0);
	glEnd ();
	glEndList ();
	}

//	if (nCanvasHeight>200)
//		glLineWidth (nCanvasHeight/ (double)200);
if (primary_lh [primary])
	glCallList (primary_lh [primary]);
else {
	primary_lh [primary] = glGenLists (1);
	glNewList (primary_lh [primary], GL_COMPILE_AND_EXECUTE);
	glColor3fv (dark_g);
	glBegin (GL_LINES);
	//left primary bar
	glVertex2d (-14.0, -8.0);
	glVertex2d (-8.0, -5.0);
	//right primary bar
	glVertex2d (14.0, -8.0);
	glVertex2d (8.0, -5.0);
	glEnd ();
	glColor3fv (primary ? bright_g : dark_g);
	//left upper
	OglDrawCircle2 (12, GL_POLYGON, 1.5, -7.0, 1.5, -5.0);
	//right upper
	OglDrawCircle2 (12, GL_POLYGON, 1.5, 7.0, 1.5, -5.0);
	glColor3fv ((primary == 2) ? bright_g : dark_g);
	//left lower
	OglDrawCircle2 (8, GL_POLYGON, 1.0, -14.0, 1.0, -8.0);
	//right lower
	OglDrawCircle2 (8, GL_POLYGON, 1.0, 14.0, 1.0, -8.0);
	glEndList ();
	}
//	if (nCanvasHeight>200)
//		glLineWidth (1);

if (!secondary_lh [secondary])
	glCallList (secondary_lh [secondary]);
else {
	secondary_lh [secondary] = glGenLists (1);
	glNewList (secondary_lh [secondary], GL_COMPILE_AND_EXECUTE);
	if (secondary <= 2) {
		//left secondary
		glColor3fv ((secondary == 1) ? bright_g : darker_g);
		OglDrawCircle2 (16, GL_LINE_LOOP, 2.0, -10.0, 2.0, -1.0);
		//right secondary
		glColor3fv ((secondary == 2) ? bright_g : darker_g);
		OglDrawCircle2 (16, GL_LINE_LOOP, 2.0, 10.0, 2.0, -1.0);
		}
	else {
		//bottom/middle secondary
		glColor3fv ((secondary == 4) ? bright_g : darker_g);
		OglDrawCircle2 (16, GL_LINE_LOOP, 2.0, 0.0, 2.0, -7.0);
		}
	glEndList ();
	}
glDisable (GL_SMOOTH);
glLineWidth (1);
glPopMatrix ();
}

//------------------------------------------------------------------------------

int G3DrawSphere (g3sPoint *pnt, fix rad, int bBigSphere)
{
	double r;

glDisable (GL_TEXTURE_2D);
OglGrsColor (&grdCurCanv->cv_color);
glPushMatrix ();
//	glTranslated (f2glf (0), f2glf (0), -f2glf (pnt->p3_vec.z));
glTranslatef (f2glf (pnt->p3_vec.x), f2glf (pnt->p3_vec.y), f2glf (pnt->p3_vec.z));
r = f2glf (rad);
glScaled (r, r, r);
if (bBigSphere)
	if (bsphereh)
		glCallList (bsphereh);
	else
		bsphereh=CircleListInit (20, GL_POLYGON, GL_COMPILE_AND_EXECUTE);
else
	if (ssphereh)
		glCallList (ssphereh);
	else
		ssphereh=CircleListInit (12, GL_POLYGON, GL_COMPILE_AND_EXECUTE);
glPopMatrix ();
if (grdCurCanv->cv_color.rgb)
	glDisable (GL_BLEND);
return 0;
}

//------------------------------------------------------------------------------

int GrUCircle (fix xc1, fix yc1, fix r1)
{//dunno if this really works, radar doesn't seem to.. hm..
glDisable (GL_TEXTURE_2D);
//	glPointSize (f2glf (rad);
OglGrsColor (&grdCurCanv->cv_color);
glPushMatrix ();
glTranslatef (
			(f2fl (xc1) + grdCurCanv->cv_bitmap.bm_props.x) / (float) gameStates.ogl.nLastW, 
		1.0f - (f2fl (yc1) + grdCurCanv->cv_bitmap.bm_props.y) / (float) gameStates.ogl.nLastH, 0);
glScalef (f2fl (r1), f2fl (r1), f2fl (r1));
if (r1<=i2f (5)){
	if (!circleh5) 
		circleh5=CircleListInit (5, GL_LINE_LOOP, GL_COMPILE_AND_EXECUTE);
	else 
		glCallList (circleh5);
	}
else{
	if (!circleh10) 
		circleh10=CircleListInit (10, GL_LINE_LOOP, GL_COMPILE_AND_EXECUTE);
	else 
		glCallList (circleh10);
}
glPopMatrix ();
if (grdCurCanv->cv_color.rgb)
	glDisable (GL_BLEND);
return 0;
}

//------------------------------------------------------------------------------

bool G3DrawPoly (int nv, g3sPoint **pointList)
{
	int i;

r_polyc++;
glDisable (GL_TEXTURE_2D);
OglGrsColor (&grdCurCanv->cv_color);
glBegin (GL_TRIANGLE_FAN);
for (i = 0; i < nv; i++, pointList++) {
//	glVertex3f (f2glf (pointList [c]->p3_vec.x), f2glf (pointList [c]->p3_vec.y), f2glf (pointList [c]->p3_vec.z);
	OglVertex3f (*pointList);
	}
if (grdCurCanv->cv_color.rgb || (gameStates.render.grAlpha < GR_ACTUAL_FADE_LEVELS))
	glDisable (GL_BLEND);
glEnd ();
return 0;
}

//------------------------------------------------------------------------------

bool G3DrawPolyAlpha (int nv, g3sPoint **pointList, 
							 float red, float green, float blue, float alpha)
{
	int			c;
	GLint			curFunc; 

r_polyc++;
glGetIntegerv (GL_DEPTH_FUNC, &curFunc);
if (gameStates.render.bQueryOcclusion)
	glDepthFunc (GL_LESS);
else
	glDepthFunc (GL_LEQUAL);
glEnable (GL_BLEND);
glDisable (GL_TEXTURE_2D);
if (alpha < 0)
	alpha = (float) gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS;
glColor4f (red, green, blue, alpha);
glBegin (GL_TRIANGLE_FAN);
for (c = 0; c < nv; c++)
	OglVertex3f (*pointList++);
glEnd ();
glDepthFunc (curFunc);
return 0;
}

//------------------------------------------------------------------------------

void gr_upoly_tmap (int nverts, int *vert )
{
#if TRACE	
con_printf (CON_DEBUG, "gr_upoly_tmap: unhandled\n");//should never get called
#endif
}

//------------------------------------------------------------------------------

void DrawTexPolyFlat (grsBitmap *bm, int nv, g3sPoint **vertlist)
{
#if TRACE	
con_printf (CON_DEBUG, "DrawTexPolyFlat: unhandled\n");//should never get called
#endif
}

//------------------------------------------------------------------------------

tFaceColor lightColor = {0, {1.0f, 1.0f, 1.0f}};
tFaceColor tMapColor = {0, {1.0f, 1.0f, 1.0f}};
tFaceColor vertColors [8] = {
	{0, {1.0f, 1.0f, 1.0f}}, 
	{0, {1.0f, 1.0f, 1.0f}}, 
	{0, {1.0f, 1.0f, 1.0f}}, 
	{0, {1.0f, 1.0f, 1.0f}}, 
	{0, {1.0f, 1.0f, 1.0f}}, 
	{0, {1.0f, 1.0f, 1.0f}}, 
	{0, {1.0f, 1.0f, 1.0f}}, 
	{0, {1.0f, 1.0f, 1.0f}}
	};
// cap tMapColor scales the color values in tMapColor so that none of them exceeds
// 1.0 if multiplied with any of the current face's corners' brightness values.
// To do that, first the highest corner brightness is determined.
// In the next step, color values are increased to match the max. brightness.
// Then it is determined whether any color value multiplied with the max. brightness would
// exceed 1.0. If so, all three color values are scaled so that their maximum multiplied
// with the max. brightness does not exceed 1.0.

inline void CapTMapColor (uvl *uvlList, int nv, grsBitmap *bm)
{
#if 0
	tFaceColor *color = tMapColor.index ? &tMapColor : lightColor.index ? &lightColor : NULL;

if (! (bm->bm_props.flags & BM_FLAG_NO_LIGHTING) && color) {
		double	a, m = tMapColor.color.red;
		double	h, l = 0;
		int		i;

	for (i = 0; i < nv; i++, uvlList++) {
		h = (bm->bm_props.flags & BM_FLAG_NO_LIGHTING) ? 1.0 : f2fl (uvlList->l);
		if (l < h)
			l = h;
		}

	// scale brightness with the average color to avoid darkening bright areas with the color
	a = (color->color.red + color->color.green + color->color.blue) / 3;
	if (m < color->color.green)
		m = color->color.green;
	if (m < color->color.blue)
		m = color->color.blue;
	l = l / a;
	// prevent any color component getting over 1.0
	if (l * m > 1.0)
		l = 1.0 / m;
	color->color.red *= l;
	color->color.green *= l;
	color->color.blue *= l;
	}
#endif
}

//------------------------------------------------------------------------------

static inline void ScaleColor (tFaceColor *color, float l)
{
#if 1
#	if 0
color->color.red *= l;
color->color.green *= l;
color->color.blue *= l;
#	else
	float m = color->color.red;

if (m < color->color.green)
	m = color->color.green;
if (m < color->color.blue)
	m = color->color.blue;
m = l / m;
color->color.red *= m;
color->color.green *= m;
color->color.blue *= m;
#	endif
#endif
}

//------------------------------------------------------------------------------

inline float BC (float c, float b)	//biased contrast
{
return (float) ((c < 0.5) ? pow (c, 1.0f / b) : pow (c, b));
}

void OglColor4sf (float r, float g, float b, float s)
{
if (IsMultiGame || (gameStates.ogl.nContrast == 8))
	glColor4f (r * s, g * s, b * s, gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS);
else {
	float c = (float) gameStates.ogl.nContrast - 8.0f;

	if (c > 0.0f)
		c = 1.0f / (1.0f + c * (3.0f / 8.0f));
	else
		c = 1.0f - c * (3.0f / 8.0f);
	glColor4f (BC (r, c) * s, BC (g, c) * s, BC (b, c) * s, 
				  gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS);
	}
}

//------------------------------------------------------------------------------

/*inline*/ void SetTMapColor (uvl *uvlList, int i, grsBitmap *bm, int bResetColor)
{
	float l = (bm->bm_props.flags & BM_FLAG_NO_LIGHTING) ? 1.0f : f2fl (uvlList->l);
	float s = 1.0f;

if (!gameStates.render.nRenderPass)
	return;
#if SHADOWS
if (EGI_FLAG (bShadows, 0, 0) 
	 && (gameStates.render.nShadowPass < 3)
	 && !gameOpts->render.shadows.bFast
#	if DBG_SHADOWS
	 && !bShadowTest
#	endif
	 )
	s *= gameStates.render.bHeadlightOn ? 0.4f : 0.3f;
#endif
//else
//	s = gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS;
if (gameStates.render.bHaveDynLights && gameOpts->render.bDynLighting)
	OglColor4sf (1.0f, 1.0f, 1.0f, s);
else if (tMapColor.index) {
	ScaleColor (&tMapColor, l);
	OglColor4sf (tMapColor.color.red, tMapColor.color.green, tMapColor.color.blue, s);
	tMapColor.color.red =
	tMapColor.color.green =
	tMapColor.color.blue = 1.0;
	}	
#if VERTEX_LIGHTING
else if (i >= sizeof (vertColors) / sizeof (tFaceColor))
	return;
else if (vertColors [i].index) {
		tFaceColor *pvc = vertColors + i;

//	if (pvc->index >= 0)
//		ScaleColor (pvc, l);
	OglColor4sf (pvc->color.red, pvc->color.green, pvc->color.blue, s);
	if (bResetColor)
		pvc->color.red =
		pvc->color.green =
		pvc->color.blue = 1.0;
	}	
#else
else if (lightColor.index) {
	ScaleColor (&lightColor, l);
	OglColor4sf (lightColor.color.red, lightColor.color.green, lightColor.color.blue, s);
	}
#endif	
else {
	OglColor4sf (l, l, l, s);
	}
}

//------------------------------------------------------------------------------

inline void SetTexCoord (uvl *uvlList, int orient, int multi)
{
	float u1, v1;

switch (orient) {
	case 1:
		u1 = 1.0f - f2glf (uvlList->v);
		v1 = f2glf (uvlList->u);
		break;
	case 2:
		u1 = 1.0f - f2glf (uvlList->u);
		v1 = 1.0f - f2glf (uvlList->v);
		break;
	case 3:
		u1 = f2glf (uvlList->v);
		v1 = 1.0f - f2glf (uvlList->u);
		break;
	default:
		u1 = f2glf (uvlList->u);
		v1 = f2glf (uvlList->v);
		break;
	}
#if OGL_MULTI_TEXTURING
if (multi)
	glMultiTexCoord2f (GL_TEXTURE1_ARB, u1, v1);
else
#endif
	glTexCoord2f (u1, v1);
}

//------------------------------------------------------------------------------
//glTexture MUST be bound first
void OglTexWrap (ogl_texture *tex, int state)
{
#if 0
if (tex->handle < 0)
	state = GL_CLAMP;
#endif
if ((tex->wrapstate != state) || (tex->numrend < 1)) {
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state);
	tex->wrapstate = state;
	}
}

//------------------------------------------------------------------------------

inline void InitTMU (int i)
{
	static GLuint tmuIds [] = {GL_TEXTURE0_ARB, GL_TEXTURE1_ARB, GL_TEXTURE2_ARB};

if (glIsList (glInitTMU [i]))
	glCallList (glInitTMU [i]);
else {
	glInitTMU [i] = glGenLists (1);
	if (glInitTMU [i])
		glNewList (glInitTMU [i], GL_COMPILE);
	OglActiveTexture (tmuIds [i]);
	glEnable (GL_TEXTURE_2D);
	switch (i) {
		case 0:
			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			break;
		case 1:
			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
			break;
		default:
			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}
	if (glInitTMU [i]) {
		glEndList ();
		InitTMU (i);
		}
	}
}

//------------------------------------------------------------------------------

inline void InitTMU0 (void)
{
if (glIsList (glInitTMU [0]))
	glCallList (glInitTMU [0]);
else 
	{
	glInitTMU [0] = glGenLists (1);
	if (glInitTMU [0])
		glNewList (glInitTMU [0], GL_COMPILE);
	OglActiveTexture (GL_TEXTURE0_ARB);
	glEnable (GL_TEXTURE_2D);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (glInitTMU [0]) {
		glEndList ();
		InitTMU0 ();
		}
	}
}

//------------------------------------------------------------------------------

inline void InitTMU1 (void)
{
if (glIsList (glInitTMU [1]))
	glCallList (glInitTMU [1]);
else 
	{
	glInitTMU [1] = glGenLists (1);
	if (glInitTMU [1])
		glNewList (glInitTMU [1], GL_COMPILE);
	OglActiveTexture (GL_TEXTURE1_ARB);
	glEnable (GL_TEXTURE_2D);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	if (glInitTMU [1]) {
		glEndList ();
		InitTMU1 ();
		}
	}
}

//------------------------------------------------------------------------------

inline void InitTMU2 (void)
{
if (glIsList (glInitTMU [2]))
	glCallList (glInitTMU [2]);
else 
	{
	glInitTMU [2] = glGenLists (1);
	if (glInitTMU [2])
		glNewList (glInitTMU [2], GL_COMPILE);
	OglActiveTexture (GL_TEXTURE2_ARB);
	glEnable (GL_TEXTURE_2D);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (glInitTMU [2]) {
		glEndList ();
		InitTMU2 ();
		}
	}
}

//------------------------------------------------------------------------------

inline void InitTMU3 (void)
{
if (glIsList (glInitTMU [3]))
	glCallList (glInitTMU [3]);
else 
	{
	glInitTMU [3] = glGenLists (1);
	if (glInitTMU [3])
		glNewList (glInitTMU [3], GL_COMPILE);
	OglActiveTexture (GL_TEXTURE2_ARB);
	glEnable (GL_TEXTURE_2D);
	if (glInitTMU [3]) {
		glEndList ();
		InitTMU3 ();
		}
	}
}

//------------------------------------------------------------------------------

inline void ExitTMU (void)
{
if (glIsList (glExitTMU))
	glCallList (glExitTMU);
else 
	{
	glExitTMU = glGenLists (1);
	if (glExitTMU)
		glNewList (glExitTMU, GL_COMPILE);
	OglActiveTexture (GL_TEXTURE2_ARB);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	OglActiveTexture (GL_TEXTURE1_ARB);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	OglActiveTexture (GL_TEXTURE0_ARB);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	if (glExitTMU) {
		glEndList ();
		ExitTMU ();
		}
	}
}

//------------------------------------------------------------------------------

typedef void tInitTMU (void);
typedef tInitTMU *pInitTMU;

inline int G3BindTex (grsBitmap *bmP, GLint nTexId, GLhandleARB lmProg, char *pszTexId, 
						    pInitTMU initTMU, int bShaderMerge)
{
if (bmP || (nTexId >= 0)) {
	initTMU ();
	if (nTexId >= 0)
		OGL_BINDTEX (nTexId);
	else {
		if (OglBindBmTex (bmP, 0))
			return 1;
		OglTexWrap (bmP->glTexture, GL_REPEAT);
		}
	if (bShaderMerge)
		glUniform1i (glGetUniformLocation (lmProg, pszTexId), 0);
	}
return 0;
}

//------------------------------------------------------------------------------

void G3Normal (g3sPoint **pointList, vmsVector *pvNormal)
{
vmsVector	vNormal;

#if 1
if (pvNormal) {
	if (gameStates.ogl.bUseTransform)
		glNormal3f ((GLfloat) f2fl (pvNormal->x), (GLfloat) f2fl (pvNormal->y), (GLfloat) f2fl (pvNormal->z));
		//VmVecAdd (&vNormal, pvNormal, &pointList [0]->p3_vec);
	else {
		G3RotatePoint (&vNormal, pvNormal, 0);
		glNormal3f ((GLfloat) f2fl (vNormal.x), (GLfloat) f2fl (vNormal.y), (GLfloat) f2fl (vNormal.z));
		//VmVecInc (&vNormal, &pointList [0]->p3_vec);
		}
//	glNormal3f ((GLfloat) f2fl (vNormal.x), (GLfloat) f2fl (vNormal.y), (GLfloat) f2fl (vNormal.z));
	}
else 
#endif
	{
	int	v [4];

	v [0] = pointList [0]->p3_index;
	v [1] = pointList [1]->p3_index;
	v [2] = pointList [2]->p3_index;
	if ((v [0] < 0) || (v [1] < 0) || (v [2] < 0)) {
		VmVecNormal (&vNormal, 
						 &pointList [0]->p3_vec,
						 &pointList [1]->p3_vec,
						 &pointList [2]->p3_vec);
		glNormal3f ((GLfloat) f2fl (vNormal.x), (GLfloat) f2fl (vNormal.y), (GLfloat) f2fl (vNormal.z));
		}
	else {
		int bFlip = GetVertsForNormal (v [0], v [1], v [2], 32767, v, v + 1, v + 2, v + 3);
		VmVecNormal (&vNormal, 
							gameData.segs.vertices + v [0], 
							gameData.segs.vertices + v [1], 
							gameData.segs.vertices + v [2]);
		if (bFlip)
			VmVecNegate (&vNormal);
		if (!gameStates.ogl.bUseTransform)
			G3RotatePoint (&vNormal, &vNormal, 0);
		//VmVecInc (&vNormal, &pointList [0]->p3_vec);
		glNormal3f ((GLfloat) f2fl (vNormal.x), (GLfloat) f2fl (vNormal.y), (GLfloat) f2fl (vNormal.z));
		}
	}
}

//------------------------------------------------------------------------------

void G3CalcNormal (g3sPoint **pointList, fVector *pvNormal)
{
	vmsVector	vNormal;
	int	v [4];

v [0] = pointList [0]->p3_index;
v [1] = pointList [1]->p3_index;
v [2] = pointList [2]->p3_index;
if ((v [0] < 0) || (v [1] < 0) || (v [2] < 0)) {
	VmVecNormal (&vNormal, 
					 &pointList [0]->p3_vec,
					 &pointList [1]->p3_vec,
					 &pointList [2]->p3_vec);
	}
else {
	int bFlip = GetVertsForNormal (v [0], v [1], v [2], 32767, v, v + 1, v + 2, v + 3);
	VmVecNormal (&vNormal, 
					 gameData.segs.vertices + v [0], 
					 gameData.segs.vertices + v [1], 
					 gameData.segs.vertices + v [2]);
	if (bFlip)
		VmVecNegate (&vNormal);
	}
VmsVecToFloat (pvNormal, &vNormal);
}

//------------------------------------------------------------------------------

inline fVector *G3GetNormal (g3sPoint *pPoint, fVector *pvNormal)
{
return pPoint->p3_normal.nFaces ? &pPoint->p3_normal.vNormal : pvNormal;
}

//------------------------------------------------------------------------------

fVector *G3Reflect (fVector *vReflect, fVector *vLight, fVector *vNormal)
{
//2 * n * (l dot n) - l
	float		LdotN = VmVecDotf (vLight, vNormal);

VmVecScalef (vReflect, vNormal, 2 * LdotN);
VmVecDecf (vReflect, vLight);
return vReflect;
}

//------------------------------------------------------------------------------

#define STATIC_LIGHT_TRANSFORM	0

void G3VertexColor (fVector *pvVertNorm, fVector *pVertPos, int nVertex, tFaceColor *pVertColor)
{
	fVector			lightDir, spotDir, vReflect;
	fVector			matDiffuse = {1.0f, 1.0f, 1.0f, 1.0f};
	fVector			matAmbient = {0.01f, 0.01f, 0.01f, 1.0f};
	fVector			matSpecular = {0.0f, 0.0f, 0.0f, 1.0f};
	fVector			lightColor, lightPos;
	fVector			vertNorm, vertColor = {0.0f, 0.0f, 0.0f, 1.0f}, colorSum = {0.0f, 0.0f, 0.0f, 1.0f};
#if !STATIC_LIGHT_TRANSFORM
	fVector			vertPos;
#endif
	float				s, NdotL, RdotE, spotEffect, fLightDist, fAttenuation, fMatShininess = 0.0f;
	int				i, j, nType, bMatSpecular = 0, bMatEmissive = 0, nMatLight = -1;
	int				bInRad,
						bExclusive = !gameOpts->render.shadows.bFast && (gameStates.render.nShadowPass == 3),
						bNoShadow = !gameOpts->render.shadows.bFast && (gameStates.render.nShadowPass == 4),
						bDarkness = IsMultiGame && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [IsMultiGame].bDarkness;
	tShaderLight	*psl = gameData.render.lights.dynamic.shader.lights;
	tFaceColor		*pc = NULL;

if (!gameOpts->render.shadows.bFast && (gameStates.render.nShadowPass == 3))
	s = 1.0f;// / (float) gameData.render.shadows.nLights;
else if (gameOpts->render.shadows.bFast || (gameStates.render.nShadowPass != 1))
	s = 1.0f;
else
	s = gameStates.render.bHeadlightOn ? 0.4f : 0.3f;
if (gameData.render.lights.dynamic.material.bValid) {
#if 0
	if (gameData.render.lights.dynamic.material.emissive.c.r ||
		 gameData.render.lights.dynamic.material.emissive.c.g ||
		 gameData.render.lights.dynamic.material.emissive.c.b) {
		bMatEmissive = 1;
		nMatLight = gameData.render.lights.dynamic.material.nLight;
		colorSum = gameData.render.lights.dynamic.material.emissive;
		}
#endif
	bMatSpecular = 
		gameData.render.lights.dynamic.material.specular.c.r ||
		gameData.render.lights.dynamic.material.specular.c.g ||
		gameData.render.lights.dynamic.material.specular.c.b;
	if (bMatSpecular) {
		matSpecular = gameData.render.lights.dynamic.material.specular;
		fMatShininess = (float) gameData.render.lights.dynamic.material.shininess;
		}
	}
#if 1 //cache light values per frame
if (!(bExclusive || bMatEmissive || pVertColor) && (nVertex >= 0)) {
	pc = gameData.render.color.vertices + nVertex;
	if (pc->index == gameStates.render.nFrameFlipFlop + 1) {
		OglColor4sf (pc->color.red * s, pc->color.green * s, pc->color.blue * s, 1.0);
		return;
		}
	}
#endif
if (gameStates.ogl.bUseTransform) 
	VmVecNormalizef (&vertNorm, pvVertNorm);
else {
#if !STATIC_LIGHT_TRANSFORM
	if (!gameStates.render.nState)
		VmVecNormalizef (&vertNorm, pvVertNorm);
	else 
#endif
		G3RotatePointf (&vertNorm, pvVertNorm, 0);
	}
if (!(gameStates.render.nState || pVertColor)) {
#if !STATIC_LIGHT_TRANSFORM
	VmsVecToFloat (&vertPos, gameData.segs.vertices + nVertex);
	pVertPos = &vertPos;
#endif
	SetNearestVertexLights (nVertex, 1, 0, 1);
	}
//VmVecNegatef (&vertNorm);
for (i = j = 0; i < gameData.render.lights.dynamic.shader.nLights; i++, psl++) {
	if (bExclusive) {
		if (bExclusive < 0)
			break;
		if (!psl->bExclusive)
			continue;
		bExclusive = -1;
		nType = psl->nType;
		}
	else {
		if (bNoShadow && psl->bShadow)
			continue;
		nType = psl->nType;
		if (!nType)
			continue;
		}
	if (nType == 1) {
		if (!gameStates.render.nState)
			psl->nType = 0;
		if (bDarkness)
			continue;
		//if (!(gameStates.render.nState || psl->bVariable))
		//	continue;
		}
	if (i == nMatLight)
		continue;
	if (!psl->bState)
		continue;
	lightColor = *((fVector *) &psl->color);
#if STATIC_LIGHT_TRANSFORM
	lightPos = psl->pos [1];
#else
	lightPos = psl->pos [gameStates.render.nState];
#endif
	VmVecSubf (&lightDir, &lightPos, pVertPos);
	//scaled quadratic attenuation depending on brightness
	bInRad = 0;
	if (psl->brightness < 0)
		fAttenuation = 0.01f;
	else {
		fLightDist = VmVecMagf (&lightDir) / 10.0f;
#if 1
		if (nType == 1) {
#	if 0
			if (fLightDist <= psl->rad) {
				bInRad = 1;
				fLightDist *= fLightDist;
				}
#	else
			fLightDist -= psl->rad;	//make light brighter close to light source
			if (fLightDist < 1.0f) {
				bInRad = 1;
				fLightDist = 2.0f;
				}
#	endif
			else {	//make it decay faster
				fLightDist *= fLightDist;
				fLightDist *= 2.0f;
				}
			}
		else
#endif
			fLightDist *= fLightDist;
		fAttenuation = fLightDist / psl->brightness;
		}
	VmVecNormalizef (&lightDir, &lightDir);
	NdotL = bInRad ? 1 : VmVecDotf (&vertNorm, &lightDir);
	if (psl->bSpot) {
		if (NdotL <= 0)
			continue;
		VmVecNormalizef (&spotDir, &psl->dir);
		VmVecNegatef (&lightDir);
		spotEffect = VmVecDotf (&spotDir, &lightDir);
		if (spotEffect <= psl->spotAngle)
			continue;
		spotEffect = (float) pow (spotEffect, psl->spotExponent);
		fAttenuation /= spotEffect * 10;
		VmVecScaleAddf (&vertColor, &matAmbient, &matDiffuse, NdotL);
		}
	else if (NdotL < 0) {
		NdotL = 0;
		VmVecIncf (&vertColor, &matAmbient);
		}
	else
		//vertColor = lightColor * (gl_FrontMaterial.diffuse * NdotL + matAmbient);
		VmVecScaleAddf (&vertColor, &matAmbient, &matDiffuse, NdotL);
	//if (fAttenuation > 50.0)
	//	continue;	//too far away
	VmVecMulf (&vertColor, &vertColor, (fVector *) &lightColor);
	if ((NdotL > 0.0) && bMatSpecular) {
		//spec = pow (reflect dot lightToEye, matShininess) * matSpecular * lightSpecular
		//RdotV = max (dot (reflect (-normalize (lightDir), normal), normalize (-vertPos)), 0.0);
		VmVecNegatef (&lightPos);	//create vector from light to eye
		VmVecNormalizef (&lightPos, &lightPos);
		if (!psl->bSpot)
			VmVecNegatef (&lightDir); //need direction from light to vertex now
		G3Reflect (&vReflect, &lightDir, &vertNorm);
		VmVecNormalizef (&vReflect, &vReflect);
		RdotE = VmVecDotf (&vReflect, &lightPos);
		if (RdotE < 0.0)
			RdotE = 0.0;
		//vertColor += matSpecular * lightColor * pow (RdotE, fMatShininess);
		VmVecScalef (&lightColor, &lightColor, (float) pow (RdotE, fMatShininess));
		VmVecMulf (&lightColor, &lightColor, &matSpecular);
		VmVecIncf (&vertColor, &lightColor);
		}
	VmVecScaleAddf (&colorSum, &colorSum, &vertColor, 1.0f / fAttenuation);
	j++;
	}
if (!(gameStates.render.nState && bDarkness)) {
	tRgbColorf	ambient = gameData.render.color.ambient [nVertex].color;
	colorSum.c.r += ambient.red;
	colorSum.c.g += ambient.green;
	colorSum.c.b += ambient.blue;
	}
#if 0
if (!pVertColor && gameData.render.nPaletteGamma) {
	float fBright = 1.0f + (float) gameData.render.nPaletteGamma;

	colorSum.c.r *= fBright;
	colorSum.c.g *= fBright;
	colorSum.c.b *= fBright;
	}
#endif
if (colorSum.c.r > 1.0)
	colorSum.c.r = 1.0;
if (colorSum.c.g > 1.0)
	colorSum.c.g = 1.0;
if (colorSum.c.b > 1.0)
	colorSum.c.b = 1.0;
OglColor4sf (colorSum.c.r * s, colorSum.c.g * s, colorSum.c.b * s, 1.0);
#if 1
if (!bMatEmissive && pc) {
	pc->index = gameStates.render.nFrameFlipFlop + 1;
	pc->color.red = colorSum.c.r;
	pc->color.green = colorSum.c.g;
	pc->color.blue = colorSum.c.b;
	}
if (pVertColor) {
	pVertColor->index = gameStates.render.nFrameFlipFlop + 1;
	pVertColor->color.red = colorSum.c.r;
	pVertColor->color.green = colorSum.c.g;
	pVertColor->color.blue = colorSum.c.b;
	}
#endif
} 

//------------------------------------------------------------------------------

#define	INIT_TMU(_initTMU,_bm) \
			_initTMU (); \
			if (OglBindBmTex (_bm, 0)) \
				return 1; \
			(_bm) = BmCurFrame (_bm); \
			OglTexWrap ((_bm)->glTexture, GL_REPEAT);


extern void (*tmap_drawer_ptr) (grsBitmap *bm, int nv, g3sPoint **vertlist);

static GLhandleARB	lmProg = (GLhandleARB) 0;
static GLhandleARB	tmProg = (GLhandleARB) 0;

bool G3DrawTexPolyMulti (
	int			nv, 
	g3sPoint	**pointList, 
	uvl			*uvlList, 
#if LIGHTMAPS
	uvl			*uvlLMap, 
#endif
	grsBitmap	*bmBot, 
	grsBitmap	*bmTop, 
#if LIGHTMAPS
	ogl_texture	*lightMap, 
#endif
	vmsVector	*pvNormal,
	int orient, 
	int bBlend)
{
	int			c, tmType, nFrame;
	int			bLight = 1, 
					bDynLight = gameOpts->render.bDynLighting, 
					bDrawBM = 0;
	grsBitmap	*bmP, *bmMask;
	g3sPoint		*pl, **ppl;
#if USE_VERTNORMS
	fVector		vNormal, vVertex;
#endif

if (!bmBot)
	return 1;
//if (gameStates.render.nShadowPass != 3)
	glDepthFunc (GL_LEQUAL);
if (!gameOpts->render.shadows.bFast) {
	if (gameStates.render.nShadowPass == 1)
		bLight = !bDynLight;
	else if (gameStates.render.nShadowPass == 3) {
		glEnable (GL_BLEND);
		glBlendFunc (GL_ONE, GL_ONE);
		}
	}
else {
	if (bBlend)
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	else
		glDisable (GL_BLEND);
	}
r_tpolyc++;
bmBot = BmOverride (bmBot);
if ((bmTop = BmOverride (bmTop)) && BM_FRAMES (bmTop)) {
	nFrame = (int) (BM_CURFRAME (bmTop) - BM_FRAMES (bmTop));
	bmP = bmTop;
	bmTop = BM_CURFRAME (bmTop);
	}
else
	nFrame = -1;
#if LIGHTMAPS
if (gameStates.render.color.bLightMapsOk && 
	 gameOpts->render.color.bAmbientLight && 
	 gameOpts->render.color.bUseLightMaps && 
	 lightMap && 
	 !IsMultiGame) {//lightMapping enabled
	// chose shaders depending on whether overlay bitmap present or not
	int bShaderMerge = bmTop && gameOpts->ogl.bGlTexMerge;
	if (bShaderMerge) {
		lmProg = lmShaderProgs [(bmTop->bm_props.flags & BM_FLAG_SUPER_TRANSPARENT) != 0];
		glUseProgramObject (lmProg);
		}
#if 0
	if (G3BindTex (bmBot, -1, lmProg, "btmTex", InitTMU0, bShaderMerge) ||
		 G3BindTex (bmTop, -1, lmProg, "topTex", InitTMU1, bShaderMerge) ||
		 G3BindTex (NULL, lightMap->handle, lmProg, "lMapTex", InitTMU2, bShaderMerge))
		return 1;
#else
	InitTMU0 ();	// use render pipeline 0 for bottom texture
	if (OglBindBmTex (bmBot, 0))
		return 1;
	bmBot = BmCurFrame (bmBot);
	OglTexWrap (bmBot->glTexture, GL_REPEAT);
	if (bShaderMerge)
		glUniform1i (glGetUniformLocation (lmProg, "btmTex"), 0);
	if (bmTop) { // use render pipeline 0 for overlay texture
		InitTMU1 ();
		if (OglBindBmTex (bmTop, 0))
			return 1;
		bmTop = BmCurFrame (bmTop);
		OglTexWrap (bmTop->glTexture, GL_REPEAT);
		glUniform1i (glGetUniformLocation (lmProg, "topTex"), 1);
		}
	// use render pipeline 2 for lightmap texture
	InitTMU2 ();
	OGL_BINDTEX (lightMap->handle);
	if (bShaderMerge)
		glUniform1i (glGetUniformLocation (lmProg, "lMapTex"), 2);
#endif
	glBegin (GL_TRIANGLE_FAN);
	ppl = pointList;
	//CapTMapColor (uvlList, nv, bmBot);
	for (c = 0; c < nv; c++, ppl++) {
		SetTMapColor (uvlList + c, c, bmBot, !bDrawBM);
		glMultiTexCoord2f (GL_TEXTURE0_ARB, f2glf (uvlList [c].u), f2glf (uvlList [c].v));
		if (bmTop)
			SetTexCoord (uvlList + c, orient, 1);
		glMultiTexCoord2f (GL_TEXTURE2_ARB, f2glf (uvlLMap [c].u), f2glf (uvlLMap [c].v));
		//G3SetNormal (*ppl, pvNormal);
		OglVertex3f (*ppl);
		}
	glEnd ();
#if OGL_CLEANUP
	ExitTMU ();
	if (bShaderMerge)
		glUseProgramObject (lmProg = 0);
#endif
	}
else 
#endif //LIGHTMAPS
	{//lightMapping disabled - render old way
	if (tmap_drawer_ptr == DrawTexPolyFlat) { // currently only required for cloaked poly models
		OglActiveTexture (GL_TEXTURE0_ARB);
		glDisable (GL_TEXTURE_2D);
		glColor4f (0, 0, 0, 1.0f - (gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS));
		glBegin (GL_TRIANGLE_FAN);
		for (c = 0, ppl = pointList; c < nv; c++, ppl++) {
			//G3SetNormal (*ppl, pvNormal);
			OglVertex3f (*ppl);
			}
		glEnd ();
		}
	else if (tmap_drawer_ptr == draw_tmap) {
		// this controls merging of textures containing color keyed transparency 
		// or transpareny in the bottom texture with an overlay texture present
		int	bShaderMerge,
				bTransparent = 0,
				bSuperTransp = 0;
		if (bmTop) {
			if (nFrame < 0)
            bSuperTransp = (bmTop->bm_props.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
			else
				bSuperTransp = (BM_PARENT (bmP)->bm_data.alt.bm_supertranspFrames [nFrame / 32] & (1 << (nFrame % 32))) != 0;
			//bTransparent = gameStates.render.grAlpha < (float) GR_ACTUAL_FADE_LEVELS;
			}
		// only use shaders for highres textures containing color keyed transparency
		bShaderMerge = 
			bmTop && 
			gameOpts->ogl.bGlTexMerge && 
			gameStates.render.textures.bGlsTexMergeOk && 
			bSuperTransp;
			//(bTransparent || bSuperTransp);
		// if the bottom texture contains transparency, draw the overlay texture in a separate step
		bDrawBM = bmTop && !bShaderMerge;
		if (bSuperTransp)
			r_tpolyc++;
		if (bShaderMerge || (0 && gameOpts->render.bDynLighting)) {	
			GLint loc;
			if (0 && gameOpts->render.bDynLighting) {
				glUseProgramObject (tmProg = genShaderProg);
				glUniform1f (loc = glGetUniformLocation (tmProg, "nLights"), 
								 (GLfloat) gameData.render.lights.dynamic.shader.nLights);
				glUniform1i (loc = glGetUniformLocation (tmProg, "lightTex"), 3);
				InitTMU3 ();
				glBindTexture (GL_TEXTURE_2D, gameData.render.lights.dynamic.shader.nTexHandle);
				tmType = 0;
				}
			if (bShaderMerge) {
				bmMask = BM_MASK (bmTop);
				tmType = bSuperTransp ? bmMask ? 2 : 1 : 0;
#if 0
				if (gameOpts->render.bDynLighting)
					tmType++;
				else
#endif
					glUseProgramObject (tmProg = tmShaderProgs [tmType]);
				INIT_TMU (InitTMU0, bmBot);
				glUniform1i (loc = glGetUniformLocation (tmProg, "btmTex"), 0);
				INIT_TMU (InitTMU1, bmTop);
				glUniform1i (glGetUniformLocation (tmProg, "topTex"), 1);
				if (bmMask) {
					INIT_TMU (InitTMU2, bmMask);
					glUniform1i (glGetUniformLocation (tmProg, "maskTex"), 2);
					}
#if 0 //failed attempt to properly handle overlay texture lighting
				else {
					OglActiveTexture (GL_TEXTURE2);
					glEnable (GL_TEXTURE_2D);
					glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
					glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
					glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
					glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
					glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
					glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
					}
#endif
				}
			glUniform1f (loc = glGetUniformLocation (tmProg, "grAlpha"), 
							 gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS);
			if (0 && gameOpts->render.bDynLighting) {
				glUniform1i (loc = glGetUniformLocation (tmProg, "tmTypeFS"), tmType);
				glUniform1i (loc = glGetUniformLocation (tmProg, "tmTypeVS"), tmType);
				}
			}
		else {
			InitTMU0 ();
			if (OglBindBmTex (bmBot, 0))
				return 1;
			bmBot = BmCurFrame (bmBot);
			OglTexWrap (bmBot->glTexture, GL_REPEAT);
			}
#if 0
		if (!bShaderMerge && bmTop && !bDrawBM) {
			InitTMU1 ();
			if (OglBindBmTex (bmTop, 0))
				return 1;
			bmTop = BmCurFrame (bmTop);
			OglTexWrap (bmTop->glTexture, GL_REPEAT);
			}
#endif
#if USE_VERTNORMS
		if (pvNormal)
			VmsVecToFloat (&vNormal, pvNormal);
		else
			G3CalcNormal (pointList, &vNormal);
#else
		if (gameStates.render.bHaveDynLights && gameOpts->render.bDynLighting)
			G3Normal (pointList, pvNormal);
#endif
		if (!bLight)
			glColor3i (0,0,0);
		glBegin (GL_TRIANGLE_FAN);
		for (c = 0, ppl = pointList; c < nv; c++, ppl++) {
			pl = *ppl;
			if (bLight)
				if (bDynLight)
					G3VertexColor (G3GetNormal (pl, &vNormal), VmsVecToFloat (&vVertex, &(pl->p3_vec)), pl->p3_index, NULL);
				else
					SetTMapColor (uvlList + c, c, bmBot, !bDrawBM);
			glMultiTexCoord2f (GL_TEXTURE0_ARB, f2glf (uvlList [c].u), f2glf (uvlList [c].v));
			if (bmTop && !bDrawBM)
				SetTexCoord (uvlList + c, orient, 1);
			OglVertex3f (pl);
			}
		glEnd ();
#if 0 //draw the vertex normals
		glColor3f (1.0f, 1.0f, 1.0f);
		glLineWidth (2);
		for (c = 0, ppl = pointList; c < nv; c++, ppl++) {
			g3sPoint *pl = *ppl;
			fVector	v [2], vn;
			if (!pl->p3_normal.nFaces)
				VmVecNormalizef (&vn, &vNormal);
			else
				G3RotatePointf (&vn, &pl->p3_normal.vNormal);
			VmsVecToFloat (v, &pl->p3_vec);
			VmVecScaleAddf (v + 1, v, &vn, 5);
			glDisable (GL_TEXTURE_2D);
			if (c == 1)
				glColor3f (1.0f, 0.0f, 0.0f);
			else if (c == 2)
				glColor3f (0.0f, 1.0f, 0.0f);
			else if (c == 3)
				glColor3f (0.0f, 0.0f, 1.0f);
			else
				glColor3f (1.0f, 1.0f, 1.0f);
			glBegin (GL_LINES);
			glVertex3fv ((GLfloat *) v); 
			glVertex3fv ((GLfloat *) (v + 1)); 
			glEnd ();
			}
		glLineWidth (1);
#endif
		if (bDrawBM) {
			r_tpolyc++;
			OglActiveTexture (GL_TEXTURE0_ARB);
			glEnable (GL_TEXTURE_2D);
			if (OglBindBmTex (bmTop, 0))
				return 1;
			bmTop = BmCurFrame (bmTop);
			OglTexWrap (bmTop->glTexture, GL_REPEAT);
			if (!bLight)
				glColor3i (0,0,0);
			glBegin (GL_TRIANGLE_FAN);
			for (c = 0, ppl = pointList; c < nv; c++, ppl++) {
				if (bLight)
					if (bDynLight)
						G3VertexColor (G3GetNormal (*ppl, &vNormal), VmsVecToFloat (&vVertex, &((*ppl)->p3_vec)), (*ppl)->p3_index, NULL);
					else
						SetTMapColor (uvlList + c, c, bmTop, 1);
				SetTexCoord (uvlList + c, orient, 0);
				OglVertex3f (*ppl);
				}
			glEnd ();
			glDepthFunc (GL_LESS);
#if OGL_CLEANUP
			OGL_BINDTEX (0);
			glDisable (GL_TEXTURE_2D);
#endif
			}
		else if (bShaderMerge) {
#if OGL_CLEANUP
			OglActiveTexture (GL_TEXTURE1_ARB);
			OGL_BINDTEX (0);
			glDisable (GL_TEXTURE_2D); // Disable the 2nd texture
#endif
			glUseProgramObject (tmProg = 0);
			}
		}
	else {
#if TRACE	
		con_printf (CON_DEBUG, "G3DrawTexPoly: unhandled tmap_drawer %p\n", tmap_drawer_ptr);
#endif
		}
#if OGL_CLEANUP
	OglActiveTexture (GL_TEXTURE0_ARB);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
#endif
	tMapColor.index =
	lightColor.index = 0;
	}
if (!bBlend)
	glEnable (GL_BLEND);
return 0;
}

//------------------------------------------------------------------------------

bool G3DrawBitMap (
	vmsVector	*pos, 
	fix			width, 
	fix			height, 
	grsBitmap	*bmP, 
	int			orientation, 
	float			alpha, 
	int			transp, 
	int			bDepthInfo)
{
	vmsVector	pv, v1;
	GLfloat		h, w, u, v, x, y, z;
	GLuint		depthFunc;

r_bitmapc++;
OglActiveTexture (GL_TEXTURE0_ARB);
glEnable (GL_TEXTURE_2D);
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
if (!bDepthInfo) {
	glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
	glDepthFunc (GL_ALWAYS);
	}
if (OglBindBmTex (bmP, transp)) 
	return 1;
bmP = BmOverride (bmP);
OglTexWrap (bmP->glTexture, GL_CLAMP);
glBegin (GL_QUADS);
glColor4f (1.0f, 1.0f, 1.0f, alpha);
u = bmP->glTexture->u;
v = bmP->glTexture->v;
VmVecSub (&v1, pos, &viewInfo.pos);
VmVecRotate (&pv, &v1, &viewInfo.view [0]);
x = (float) f2glf (pv.x);
y = (float) f2glf (pv.y);
z = (float) f2glf (pv.z);
w = (float) f2glf (width); //FixMul (width, viewInfo.scale.x));
h = (float) f2glf (height); //FixMul (height, viewInfo.scale.y));
glMultiTexCoord2f (GL_TEXTURE0_ARB, 0, 0);
glVertex3f (x - w, y + h, z);
glMultiTexCoord2f (GL_TEXTURE0_ARB, u, 0);
glVertex3f (x + w, y + h, z);
glMultiTexCoord2f (GL_TEXTURE0_ARB, u, v);
glVertex3f (x + w, y - h, z);
glMultiTexCoord2f (GL_TEXTURE0_ARB, 0, v);
glVertex3f (x - w, y - h, z);
glEnd ();

//These next lines are important for later leave these here - Lehm 4/26/05
//OglActiveTexture (GL_TEXTURE0_ARB);
//glBindTexture (GL_TEXTURE_2D, 0);
//glEnable (GL_DEPTH_TEST);
if (!bDepthInfo)
	glDepthFunc (depthFunc);
//glDisable (GL_TEXTURE_2D);
return 0;
} 

//------------------------------------------------------------------------------

inline void BmSetTexCoord (GLfloat u, GLfloat v, GLfloat a, int orient)
{
switch (orient) {
	case 1:
		glMultiTexCoord2d (GL_TEXTURE0_ARB, (1.0f - v) * a, u);
		break;
	case 2:
		glMultiTexCoord2d (GL_TEXTURE0_ARB, 1.0f - u, 1.0 - v);
		break;
	case 3:
		glMultiTexCoord2d (GL_TEXTURE0_ARB, v * a, (1.0f - u));
		break;
	default:
		glMultiTexCoord2d (GL_TEXTURE0_ARB, u, v);
		break;
	}
}

//------------------------------------------------------------------------------

bool OglUBitMapMC (int x, int y, int dw, int dh, grsBitmap *bmP, grs_color *c, int scale, int orient)
{
	GLint depthFunc, bBlend;
	GLfloat xo, yo, xf, yf;
	GLfloat u1, u2, v1, v2;
	GLfloat	h, a;
	GLfloat	dx, dy;

bmP = BmOverride (bmP);
if (dw < 0)
	dw = grdCurCanv->cv_bitmap.bm_props.w;
else if (dw == 0)
	dw = bmP->bm_props.w;
if (dh < 0)
	dh = grdCurCanv->cv_bitmap.bm_props.h;
else if (dh == 0)
	dh = bmP->bm_props.h;
r_ubitmapc++;
if (orient & 1) {
	int h = dw;
	dw = dh;
	dh = h;
	dx = (float) grdCurCanv->cv_bitmap.bm_props.y / (float) gameStates.ogl.nLastH;
	dy = (float) grdCurCanv->cv_bitmap.bm_props.x / (float) gameStates.ogl.nLastW;
	}
else {
	dx = (float) grdCurCanv->cv_bitmap.bm_props.x / (float) gameStates.ogl.nLastW;
	dy = (float) grdCurCanv->cv_bitmap.bm_props.y / (float) gameStates.ogl.nLastH;
	}
#if 0
a = 1.0;
h = 1.0;
orient = 0;
#else
a = (float) grdCurScreen->sc_w / (float) grdCurScreen->sc_h;
h = (float) scale / (float) F1_0;
#endif
xo = dx + x / ((float) gameStates.ogl.nLastW * h);
xf = dx + (dw + x) / ((float) gameStates.ogl.nLastW * h);
yo = 1.0f - dy - y / ((float) gameStates.ogl.nLastH * h);
yf = 1.0f - dy - (dh + y) / ((float) gameStates.ogl.nLastH * h);

OglActiveTexture (GL_TEXTURE0_ARB);
if (OglBindBmTex (bmP, 0))
	return 1;
OglTexWrap (bmP->glTexture, GL_CLAMP);

glEnable (GL_TEXTURE_2D);
glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
glDepthFunc (GL_ALWAYS);
glEnable (GL_ALPHA_TEST);
if (!(bBlend = glIsEnabled (GL_BLEND)))
	glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

if (bmP->bm_props.x == 0) {
	u1 = 0;
	if (bmP->bm_props.w == bmP->glTexture->w)
		u2 = bmP->glTexture->u;
	else
		u2 = (bmP->bm_props.w + bmP->bm_props.x) / (float) bmP->glTexture->tw;
	}
else {
	u1 = bmP->bm_props.x / (float) bmP->glTexture->tw;
	u2 = (bmP->bm_props.w + bmP->bm_props.x) / (float) bmP->glTexture->tw;
	}
if (bmP->bm_props.y == 0) {
	v1 = 0;
	if (bmP->bm_props.h == bmP->glTexture->h)
		v2 = bmP->glTexture->v;
	else
		v2 = (bmP->bm_props.h + bmP->bm_props.y) / (float) bmP->glTexture->th;
	}
else{
	v1 = bmP->bm_props.y / (float) bmP->glTexture->th;
	v2 = (bmP->bm_props.h + bmP->bm_props.y) / (float) bmP->glTexture->th;
	}

glBegin (GL_QUADS);
OglGrsColor (c);
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
glDisable (GL_ALPHA_TEST);
if (!bBlend)
	glDisable (GL_BLEND);
OglActiveTexture (GL_TEXTURE0_ARB);
OGL_BINDTEX (0);
glDisable (GL_TEXTURE_2D);
return 0;
}

//------------------------------------------------------------------------------

bool OglUBitBltI (
	int dw, int dh, int dx, int dy, 
	int sw, int sh, int sx, int sy, 
	grsBitmap *src, grsBitmap *dest, 
	int wantmip)
{
	GLdouble xo, yo, xs, ys;
	GLdouble u1, v1;//, u2, v2;
	ogl_texture tex, *texP;
	GLint curFunc; 
	int transp = (src->bm_props.flags & BM_FLAG_TGA) ? -1 : (src->bm_props.flags & BM_FLAG_TRANSPARENT) ? 2 : 0;

//	unsigned char *oldpal;
r_ubitbltc++;

u1 = v1 = 0;
dx += dest->bm_props.x;
dy += dest->bm_props.y;
xo = dx / (double) gameStates.ogl.nLastW;
xs = dw / (double) gameStates.ogl.nLastW;
yo = 1.0 - dy / (double) gameStates.ogl.nLastH;
ys = dh / (double) gameStates.ogl.nLastH;

glEnable (GL_TEXTURE_2D);
if (!(texP = src->glTexture)) {
	texP = &tex;
	OglInitTexture (texP, 0);
	texP->w = sw;
	texP->h = sh;
	texP->prio = 0.0;
	texP->wantmip = wantmip;
	texP->lw = src->bm_props.rowsize;
	OglLoadTexture (src, sx, sy, texP, transp, 0);
	}
OGL_BINDTEX (texP->handle);
OglTexWrap (texP, GL_CLAMP);
glGetIntegerv (GL_DEPTH_FUNC, &curFunc);
glDepthFunc (GL_ALWAYS); 
if (transp) {
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f (1.0, 1.0, 1.0, 1.0);
	}
else {
	glDisable (GL_BLEND);
	glColor3f (1.0, 1.0, 1.0);
	}
glBegin (GL_QUADS);
glTexCoord2d (u1, v1); 
glVertex2d (xo, yo);
glTexCoord2d (texP->u, v1); 
glVertex2d (xo + xs, yo);
glTexCoord2d (texP->u, texP->v); 
glVertex2d (xo + xs, yo-ys);
glTexCoord2d (u1, texP->v); 
glVertex2d (xo, yo - ys);
glEnd ();
if (transp)
	glDisable (GL_BLEND);
glDisable (GL_TEXTURE_2D);
glDepthFunc (curFunc);
if (!src->glTexture)
	OglFreeTexture (texP);
return 0;
}

//------------------------------------------------------------------------------

bool OglUBitBltToLinear (int w, int h, int dx, int dy, int sx, int sy, 
								 grsBitmap * src, grsBitmap * dest)
{
	unsigned char *d, *s;
	int i, j;
	int w1, h1;
	int bTGA = (dest->bm_props.flags & BM_FLAG_TGA) != 0;
//	w1=w;h1=h;
w1=grdCurScreen->sc_w;
h1=grdCurScreen->sc_h;

if (gameOpts->ogl.bReadPixels > 0) {
	glDisable (GL_TEXTURE_2D);
	glReadBuffer (GL_FRONT);
	if (bTGA)
		glReadPixels (0, 0, w1, h1, GL_RGBA, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
//			glReadPixels (sx, grdCurScreen->sc_h - (sy + h), w, h, GL_RGBA, GL_UNSIGNED_BYTE, dest->bm_texBuf);
	else {
		if (w1*h1*3>OGLTEXBUFSIZE)
			Error ("OglUBitBltToLinear: screen res larger than OGLTEXBUFSIZE\n");
		glReadPixels (0, 0, w1, h1, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
		}
//		glReadPixels (sx, grdCurScreen->sc_h- (sy+h), w, h, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
//		glReadPixels (sx, sy, w+sx, h+sy, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
	}
else {
	if (dest->bm_props.flags & BM_FLAG_TGA)
		memset (dest->bm_texBuf, 0, w1*h1*4);
	else
		memset (gameData.render.ogl.texBuf, 0, w1*h1*3);
	}
if (bTGA) {
	sx += src->bm_props.x;
	sy += src->bm_props.y;
	for (i = 0; i < h; i++) {
		d = dest->bm_texBuf + dx + (dy + i) * dest->bm_props.rowsize;
		s = gameData.render.ogl.texBuf + ((h1 - (i + sy + 1)) * w1 + sx) * 4;
		memcpy (d, s, w * 4);
		}
/*
	for (i = 0; i < h; i++)
		memcpy (dest->bm_texBuf + (h - i - 1) * dest->bm_props.rowsize, 
				  gameData.render.ogl.texBuf + ((sy + i) * h1 + sx) * 4, 
				  dest->bm_props.rowsize);
*/
	}
else {
	sx += src->bm_props.x;
	sy += src->bm_props.y;
	for (i = 0; i < h; i++) {
		d = dest->bm_texBuf + dx + (dy + i) * dest->bm_props.rowsize;
		s = gameData.render.ogl.texBuf + ((h1 - (i + sy + 1)) * w1 + sx) * 3;
		for (j = 0; j < w; j++) {
			*d++ = GrFindClosestColor (dest->bm_palette, s [0] / 4, s [1] / 4, s [2] / 4);
			s += 3;
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

bool OglUBitBltCopy (int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest)
{
#if 0 //just seems to cause a mess.
	GLdouble xo, yo;//, xs, ys;
	
	dx+=dest->bm_props.x;
	dy+=dest->bm_props.y;
	
//	xo=dx/ (double)gameStates.ogl.nLastW;
	xo=dx/ (double)grdCurScreen->sc_w;
//	yo=1.0- (dy+h)/ (double)gameStates.ogl.nLastH;
	yo=1.0- (dy+h)/ (double)grdCurScreen->sc_h;
	sx+=src->bm_props.x;
	sy+=src->bm_props.y;
	glDisable (GL_TEXTURE_2D);
	glReadBuffer (GL_FRONT);
	glRasterPos2f (xo, yo);
//	glReadPixels (0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, gameData.render.ogl.texBuf);
	glCopyPixels (sx, grdCurScreen->sc_h- (sy+h), w, h, GL_COLOR);
	glRasterPos2f (0, 0);
#endif
	return 0;
}

//------------------------------------------------------------------------------

void OglSetFOV (double fov)
{
gameStates.render.glFOV = fov;
#if 0
gameStates.render.glAspect = 90.0 / gameStates.render.glFOV;
#else
if (gameStates.ogl.bUseTransform)
	gameStates.render.glAspect = (double) grdCurScreen->sc_w / (double) grdCurScreen->sc_h;
else
	gameStates.render.glAspect = 90.0 / gameStates.render.glFOV;
#endif
gluPerspective (gameStates.render.glFOV, gameStates.render.glAspect, 0.01, 
					 (gameStates.ogl.nColorBits == 16) ? 10000.0 : 6553.5);
glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

//------------------------------------------------------------------------------

#define GL_INFINITY	0

void OglStartFrame (int bFlat, int bResetColorBuf)
{
#if SHADOWS
if (gameStates.render.nShadowPass) {
#if GL_INFINITY
	float	infProj [4][4];	//projection to infinity
#endif
	
	if (gameStates.render.nShadowPass == 1) {	//render unlit/final scene
		if (!gameStates.render.bShadowMaps) {
#if GL_INFINITY
			glMatrixMode (GL_PROJECTION);
			memset (infProj, 0, sizeof (infProj));
			infProj [1][1] = 1.0f / (float) tan (gameStates.render.glFOV);
			infProj [0][0] = infProj [1][1] / (float) gameStates.render.glAspect;
			infProj [3][2] = -0.02f;	// -2 * near
			infProj [2][2] =
			infProj [2][3] = -1.0f;
			glLoadMatrixf ((float *) infProj);
#endif
#if 0
			glMatrixMode (GL_MODELVIEW);
			glLoadIdentity ();
#endif
			glEnable (GL_DEPTH_TEST);
			glDepthFunc (GL_LESS);
			glEnable (GL_CULL_FACE);		
			glCullFace (GL_BACK);
			if (!gameOpts->render.shadows.bFast)
				glColorMask (0,0,0,0);
			}
		}
	else if (gameStates.render.nShadowPass == 2) {	//render occluders / shadow maps
		if (gameStates.render.bShadowMaps) {
			glColorMask (0,0,0,0);
			glEnable (GL_POLYGON_OFFSET_FILL);
			glPolygonOffset (1.0f, 2.0f);
			}
		else {
#	if DBG_SHADOWS
			if (bShadowTest) {
				glColorMask (1,1,1,1);
				glDepthMask (0);
				glEnable (GL_BLEND);
				glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDisable (GL_STENCIL_TEST);
				}
			else 
#	endif
				{
				glColorMask (0,0,0,0);
				glDepthMask (0);
				glEnable (GL_STENCIL_TEST);
				if (!glIsEnabled (GL_STENCIL_TEST))
					extraGameInfo [0].bShadows = 
					extraGameInfo [1].bShadows = 0;
				glClearStencil (0);
				glClear (GL_STENCIL_BUFFER_BIT);
#if 0
				if (!glActiveStencilFaceEXT)
#endif
					bSingleStencil = 1;
#	if DBG_SHADOWS
				if (bSingleStencil || bShadowTest) {
#	else
				if (bSingleStencil) {
#	endif
					glStencilMask (~0);
					glStencilFunc (GL_ALWAYS, 0, ~0);
					}
#if 0
				else {
					glEnable (GL_STENCIL_TEST_TWO_SIDE_EXT);
					glActiveStencilFaceEXT (GL_BACK);
					if (bZPass)
						glStencilOp (GL_KEEP, GL_KEEP, GL_DECR_WRAP);
					else
						glStencilOp (GL_KEEP, GL_DECR_WRAP, GL_KEEP);
					glStencilOp (GL_KEEP, GL_DECR_WRAP, GL_KEEP);
					glStencilMask (~0);
					glStencilFunc (GL_ALWAYS, 0, ~0);
					glActiveStencilFaceEXT (GL_FRONT);
					if (bZPass)
						glStencilOp (GL_KEEP, GL_KEEP, GL_INCR_WRAP);
					else
						glStencilOp (GL_KEEP, GL_INCR_WRAP, GL_KEEP);
					glStencilMask (~0);
					glStencilFunc (GL_ALWAYS, 0, ~0);
					}
#endif
#if 0
				glEnable (GL_POLYGON_OFFSET_FILL);
				glPolygonOffset (1.0f, 1.0f);
#endif
				}
			}
		}
	else if (gameStates.render.nShadowPass == 3) { //render final lit scene
		if (gameStates.render.bShadowMaps) {
			glDisable (GL_POLYGON_OFFSET_FILL);
			glDepthFunc (GL_LESS);
			}
		else {
#if 0
			glDisable (GL_POLYGON_OFFSET_FILL);
#endif
			if (gameOpts->render.shadows.bFast) {
				glStencilFunc (GL_NOTEQUAL, 0, ~0);
				glStencilOp (GL_REPLACE, GL_KEEP, GL_KEEP);		
				}
			else 
				{
				glStencilFunc (GL_EQUAL, 0, ~0);
#if 0
				glStencilOp (GL_KEEP, GL_KEEP, GL_INCR);	//problem: layered texturing fails
#else
				glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
#endif
				}
			glCullFace (GL_BACK);
			glDepthFunc (GL_LESS);
			glColorMask (1,1,1,1);
			}
		}
	else if (gameStates.render.nShadowPass == 4) {	//render unlit/final scene
		glEnable (GL_DEPTH_TEST);
		glDepthFunc (GL_LESS);
		glEnable (GL_CULL_FACE);		
		glCullFace (GL_BACK);
		}
#if GL_INFINITY
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
#endif
	}
else 
#endif //SHADOWS
	{
	r_polyc =  
	r_tpolyc = 
	r_bitmapc = 
	r_ubitmapc = 
	r_ubitbltc = 
	r_upixelc = 0;

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();//clear matrix
	OglSetFOV (gameStates.render.glFOV);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	OGL_VIEWPORT (grdCurCanv->cv_bitmap.bm_props.x, grdCurCanv->cv_bitmap.bm_props.y, 
					  nCanvasWidth, nCanvasHeight);
	if (gameStates.render.nRenderPass < 0) {
		glDepthMask (1);
		glColorMask (1,1,1,1);
		glClearColor (0,0,0,0);
		glClear (bResetColorBuf ? GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT : GL_DEPTH_BUFFER_BIT);
		}
	else if (gameStates.render.nRenderPass) {
		glDepthMask (0);
		glColorMask (1,1,1,1);
		glClearColor (0,0,0,0);
		if (bResetColorBuf)
			glClear (GL_COLOR_BUFFER_BIT);
		}
	else { //make a depth-only render pass first to decrease bandwidth waste due to overdraw
		glDepthMask (1);
		glColorMask (0,0,0,0);
		glClear (GL_DEPTH_BUFFER_BIT);
		}
	glShadeModel (GL_SMOOTH);
	if (gameStates.ogl.bEnableScissor) {
		glScissor (
			grdCurCanv->cv_bitmap.bm_props.x, 
			grdCurScreen->sc_canvas.cv_bitmap.bm_props.h - grdCurCanv->cv_bitmap.bm_props.y - nCanvasHeight, 
			nCanvasWidth, 
			nCanvasHeight);
		glEnable (GL_SCISSOR_TEST);
		}
	else
		glDisable (GL_SCISSOR_TEST);
	if (gameStates.ogl.bAntiAliasingOk && gameStates.ogl.bAntiAliasing)
		glEnable (GL_MULTISAMPLE_ARB);
	if (bFlat) {
		glDisable (GL_DEPTH_TEST);
		glDisable (GL_ALPHA_TEST);
		glDisable (GL_CULL_FACE);
		}
	else {
		glEnable (GL_CULL_FACE);		
		glFrontFace (GL_CW);	//Weird, huh? Well, D2 renders everything reverse ...
		glCullFace (GL_BACK);
		glEnable (GL_DEPTH_TEST);
		glDepthFunc (GL_LESS);
		glEnable (GL_ALPHA_TEST);
		glAlphaFunc (GL_GEQUAL, (float) 0.01);	
		}	
	if (gameStates.render.bHaveDynLights && gameOpts->render.bDynLighting)	{//for optional hardware lighting
		//GLfloat fAmbient [4] = {0.0f, 0.0f, 0.0f, 1.0f};
		//glEnable (GL_LIGHTING);
		//glLightModelfv (GL_LIGHT_MODEL_AMBIENT, fAmbient);
		//glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, 0);
		glShadeModel (GL_SMOOTH);
		//glEnable (GL_COLOR_MATERIAL);
		//glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
		}
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

//------------------------------------------------------------------------------

#ifndef NMONO
void merge_textures_stats (void);
#endif

//------------------------------------------------------------------------------

void OglEndFrame (void)
{
//	OGL_VIEWPORT (grdCurCanv->cv_bitmap.bm_props.x, grdCurCanv->cv_bitmap.bm_props.y, );
//	glViewport (0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
OGL_VIEWPORT (0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
#ifndef NMONO
//	merge_textures_stats ();
//	ogl_texture_stats ();
#endif
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();//clear matrix
glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
glMatrixMode (GL_MODELVIEW);
glLoadIdentity ();//clear matrix
glDisable (GL_SCISSOR_TEST);
glDisable (GL_ALPHA_TEST);
glDisable (GL_DEPTH_TEST);
glDisable (GL_CULL_FACE);
glDisable (GL_STENCIL_TEST);
if (gameStates.render.bHaveDynLights && gameOpts->render.bDynLighting) {
	glDisable (GL_LIGHTING);
	glDisable (GL_COLOR_MATERIAL);
	}
glDepthMask (1);
glColorMask (1,1,1,1);
if (gameStates.ogl.bAntiAliasingOk && gameStates.ogl.bAntiAliasing)
	glDisable (GL_MULTISAMPLE_ARB);
}

//------------------------------------------------------------------------------

void OglSwapBuffers (int bForce, int bClear)
{
if (!gameStates.menus.nInMenu || bForce) {
	ogl_clean_texture_cache ();
	if (gr_renderstats)
		GrPrintF (5, GAME_FONT->ft_h*13+3*13, "%i flat %i tex %i sprites %i bitmaps", r_polyc, r_tpolyc, r_bitmapc, r_ubitmapc);
	//if (gameStates.app.bGameRunning && !gameStates.menus.nInMenu)
		OglDoPalFx ();
	OglSwapBuffersInternal ();
	if (gameStates.menus.nInMenu || bClear)
		glClear (GL_COLOR_BUFFER_BIT);
	}
}

// -----------------------------------------------------------------------------------

void OglSetupTransform (void)
{
if (gameStates.ogl.bUseTransform) {
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	glLoadIdentity ();
	//glScalef (f2fl (viewInfo.scale.x), f2fl (viewInfo.scale.y), -f2fl (viewInfo.scale.z));
	glMultMatrixf (viewInfo.glViewf);
	glTranslatef (-viewInfo.glPosf [0], -viewInfo.glPosf [1], -viewInfo.glPosf [2]);
	}
}

// -----------------------------------------------------------------------------------

void OglResetTransform (void)
{
if (gameStates.ogl.bUseTransform)
	glPopMatrix ();
}

//------------------------------------------------------------------------------
