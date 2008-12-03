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
#include "grdef.h"
#include "light.h"
#include "lightmap.h"
#include "texmerge.h"
#include "transprender.h"

//------------------------------------------------------------------------------

extern int r_upixelc;

void OglUPixelC (int x, int y, tCanvasColor *colorP)
{
r_upixelc++;
glDisable (GL_TEXTURE_2D);
glPointSize (1.0);
glBegin (GL_POINTS);
OglCanvasColor (colorP);
glVertex2d ((x + CCanvas::Current ()->.Left ()) / (double) gameStates.ogl.nLastW,
				1.0 - (y + CCanvas::Current ()->.Top ()) / (double) gameStates.ogl.nLastW);
if (colorP->rgb)
	glDisable (GL_BLEND);
glEnd();
}

//------------------------------------------------------------------------------

void OglURect(int left, int top, int right, int bot)
{
	GLfloat		xo, yo, xf, yf;

xo = ((float) left + CCanvas::Current ()->.Left ()) / (float) gameStates.ogl.nLastW;
xf = (float) (right + CCanvas::Current ()->.Left ()) / (float) gameStates.ogl.nLastW;
yo = 1.0f - (float) (top + CCanvas::Current ()->.Top ()) / (float) gameStates.ogl.nLastH;
yf = 1.0f - (float) (bot + CCanvas::Current ()->.Top ()) / (float) gameStates.ogl.nLastH;

glDisable (GL_TEXTURE_2D);
OglCanvasColor (&COLOR);
glBegin (GL_QUADS);
glVertex2f (xo,yo);
glVertex2f (xo,yf);
glVertex2f (xf,yf);
glVertex2f (xf,yo);
glEnd ();
if (COLOR.rgb || (gameStates.render.grAlpha < FADE_LEVELS))
	glDisable (GL_BLEND);
}

//------------------------------------------------------------------------------

void OglULineC (int left,int top,int right,int bot, tCanvasColor *c)
{
	GLfloat xo, yo, xf, yf;

xo = (left + CCanvas::Current ()->.Left ()) / (float) gameStates.ogl.nLastW;
xf = (right + CCanvas::Current ()->.Left ()) / (float) gameStates.ogl.nLastW;
yo = 1.0f - (top + CCanvas::Current ()->.Top ()) / (float) gameStates.ogl.nLastH;
yf = 1.0f - (bot + CCanvas::Current ()->.Top ()) / (float) gameStates.ogl.nLastH;
glDisable (GL_TEXTURE_2D);
OglCanvasColor (c);
glBegin (GL_LINES);
glVertex2f (xo,yo);
glVertex2f (xf,yf);
if (c->rgb)
	glDisable (GL_BLEND);
glEnd();
}

//------------------------------------------------------------------------------

void OglUPolyC (int left, int top, int right, int bot, tCanvasColor *c)
{
	GLfloat xo, yo, xf, yf;

xo = (left + CCanvas::Current ()->.Left ()) / (float) gameStates.ogl.nLastW;
xf = (right + CCanvas::Current ()->.Left ()) / (float) gameStates.ogl.nLastW;
yo = 1.0f - (top + CCanvas::Current ()->.Top ()) / (float) gameStates.ogl.nLastH;
yf = 1.0f - (bot + CCanvas::Current ()->.Top ()) / (float) gameStates.ogl.nLastH;
glDisable (GL_TEXTURE_2D);
OglCanvasColor (c);
glBegin (GL_LINE_LOOP);
glVertex2f (xo, yo);
glVertex2f (xf, yo);
glVertex2f (xf, yf);
glVertex2f (xo, yf);
glEnd();
if (c->rgb)
	glDisable (GL_BLEND);
}

//------------------------------------------------------------------------------
