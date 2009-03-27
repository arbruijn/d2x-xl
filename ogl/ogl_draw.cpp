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
#include "grdef.h"
#include "light.h"
#include "lightmap.h"
#include "texmerge.h"
#include "transprender.h"

//------------------------------------------------------------------------------

extern int r_upixelc;

void OglDrawPixel (int x, int y, tCanvasColor *colorP)
{
glDisable (GL_TEXTURE_2D);
glPointSize (1.0);
glBegin (GL_POINTS);
if (!colorP)
	colorP = &COLOR;
OglCanvasColor (colorP);
glVertex2f (float (x + CCanvas::Current ()->Left ()) / float (gameStates.ogl.nLastW),
				1.0f - float (y + CCanvas::Current ()->Top ()) / float (gameStates.ogl.nLastW));
//if (colorP->rgb)
//	glDisable (GL_BLEND);
glEnd ();
}

//------------------------------------------------------------------------------

void OglDrawFilledRect (int left, int top, int right, int bot, tCanvasColor *colorP)
{
GLfloat x0 = float (left + CCanvas::Current ()->Left ()) / float (gameStates.ogl.nLastW);
GLfloat x1 = float (right + CCanvas::Current ()->Left ()) / float (gameStates.ogl.nLastW);
GLfloat y0 = 1.0f - float (top + CCanvas::Current ()->Top ()) / float (gameStates.ogl.nLastH);
GLfloat y1 = 1.0f - float (bot + CCanvas::Current ()->Top ()) / float (gameStates.ogl.nLastH);

glDisable (GL_TEXTURE_2D);
if (!colorP)
	colorP = &COLOR;
OglCanvasColor (colorP);
glBegin (GL_QUADS);
glVertex2f (x0, y0);
glVertex2f (x0, y1);
glVertex2f (x1, y1);
glVertex2f (x1, y0);
glEnd ();
//if (colorP->rgb || (gameStates.render.grAlpha < 1.0f))
//	glDisable (GL_BLEND);
}

//------------------------------------------------------------------------------

void OglDrawFilledPoly (int* x, int* y, int nVerts, tCanvasColor *colorP, int nColors)
{
	int left = CCanvas::Current ()->Left ();
	int top = CCanvas::Current ()->Top ();
	int j;

glDisable (GL_TEXTURE_2D);
if (!colorP) {
	colorP = &COLOR;
	nColors = 1;
	}
glBegin (GL_POLYGON);
for (int i = 0; i <= nVerts; i++) {
	j = i % nVerts;
	if (j < nColors)
		OglCanvasColor (colorP + j);
	glVertex2f (GLfloat (x [j] + left) / GLfloat (gameStates.ogl.nLastW),
					1.0f - GLfloat (y [j] + top) / GLfloat (gameStates.ogl.nLastH));
	}
glEnd ();
//if (colorP->rgb || (gameStates.render.grAlpha < 1.0f))
//	glDisable (GL_BLEND);
}

//------------------------------------------------------------------------------

void OglDrawLine (int left, int top, int right, int bot, tCanvasColor *colorP)
{
GLfloat x0 = float (left + CCanvas::Current ()->Left ()) / float (gameStates.ogl.nLastW);
GLfloat x1 = float (right + CCanvas::Current ()->Left ()) / float (gameStates.ogl.nLastW);
GLfloat y0 = 1.0f - float (top + CCanvas::Current ()->Top ()) / float (gameStates.ogl.nLastH);
GLfloat y1 = 1.0f - float (bot + CCanvas::Current ()->Top ()) / float (gameStates.ogl.nLastH);
glDisable (GL_TEXTURE_2D);
if (!colorP)
	colorP = &COLOR;
OglCanvasColor (colorP);
glBegin (GL_LINES);
glVertex2f (x0, y0);
glVertex2f (x1, y1);
//if (colorP->rgb)
//	glDisable (GL_BLEND);
glEnd();
}

//------------------------------------------------------------------------------

void OglDrawEmptyRect (int left, int top, int right, int bot, tCanvasColor* colorP)
{
GLfloat x0 = float (left + CCanvas::Current ()->Left ()) / float (gameStates.ogl.nLastW);
GLfloat x1 = float (right + CCanvas::Current ()->Left ()) / float (gameStates.ogl.nLastW);
GLfloat y0 = 1.0f - float (top + CCanvas::Current ()->Top ()) / float (gameStates.ogl.nLastH);
GLfloat y1 = 1.0f - float (bot + CCanvas::Current ()->Top ()) / float (gameStates.ogl.nLastH);
glDisable (GL_TEXTURE_2D);
if (!colorP)
	colorP = &COLOR;
OglCanvasColor (colorP);
glBegin (GL_LINE_LOOP);
glVertex2f (x0, y0);
glVertex2f (x1, y0);
glVertex2f (x1, y1);
glVertex2f (x0, y1);
glEnd ();
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//if (colorP->rgb)
//	glDisable (GL_BLEND);
}

//------------------------------------------------------------------------------
