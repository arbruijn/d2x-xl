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
#include "grdef.h"
#include "light.h"
#include "lightmap.h"
#include "texmerge.h"
#include "transprender.h"

//------------------------------------------------------------------------------

extern int r_upixelc;

void OglDrawPixel (int x, int y, tCanvasColor *colorP)
{
ogl.SetTextureUsage (false);
glPointSize (1.0);
glBegin (GL_POINTS);
if (!colorP)
	colorP = &COLOR;
OglCanvasColor (colorP);
glVertex2f (float (x + CCanvas::Current ()->Left ()) / float (ogl.m_states.nLastW),
				1.0f - float (y + CCanvas::Current ()->Top ()) / float (ogl.m_states.nLastW));
//if (colorP->rgb)
//	ogl.SetBlending (false);
glEnd ();
}

//------------------------------------------------------------------------------

void OglDrawFilledRect (int left, int top, int right, int bot, tCanvasColor *colorP)
{
GLfloat x0 = float (left + CCanvas::Current ()->Left ()) / float (ogl.m_states.nLastW);
GLfloat x1 = float (right + CCanvas::Current ()->Left ()) / float (ogl.m_states.nLastW);
GLfloat y0 = 1.0f - float (top + CCanvas::Current ()->Top ()) / float (ogl.m_states.nLastH);
GLfloat y1 = 1.0f - float (bot + CCanvas::Current ()->Top ()) / float (ogl.m_states.nLastH);

ogl.SetTextureUsage (false);
if (!colorP)
	colorP = &COLOR;
OglCanvasColor (colorP);
CFloatVector vPosf;
vPosf [X] = (x0 + x1) / 2;
vPosf [Y] = (y0 + y1) / 2;
ogl.RenderQuad (NULL, vPosf, vPosf [X], vPosf [Y], 2);
//if (colorP->rgb || (gameStates.render.grAlpha < 1.0f))
//	ogl.SetBlending (false);
}

//------------------------------------------------------------------------------

void OglDrawFilledPoly (int* x, int* y, int nVerts, tCanvasColor *colorP, int nColors)
{
if (ogl.SizeBuffers (nVerts + 1)) {
	int left = CCanvas::Current ()->Left ();
	int top = CCanvas::Current ()->Top ();
	int j;
	bool bColor;

	ogl.SetTextureUsage (false);
	if (!(bColor = colorP && (nColors == nVerts)))
		OglCanvasColor (&COLOR);
	for (int i = 0; i <= nVerts; i++) {
		j = i % nVerts;
		if (bColor)
			OglCanvasColor (colorP + j, ogl.ColorBuffer () + i);
		ogl.VertexBuffer () [i][X] = GLfloat (x [j] + left) / GLfloat (ogl.m_states.nLastW);
		ogl.VertexBuffer () [i][Y] = 1.0f - GLfloat (y [j] + top) / GLfloat (ogl.m_states.nLastH);
		}
	ogl.FlushBuffers (GL_POLYGON, nVerts + 1);
	}
}

//------------------------------------------------------------------------------

void OglDrawLine (int left, int top, int right, int bot, tCanvasColor *colorP)
{
if (ogl.SizeVertexBuffer (2)) {
	GLfloat x0 = float (left + CCanvas::Current ()->Left ()) / float (ogl.m_states.nLastW);
	GLfloat x1 = float (right + CCanvas::Current ()->Left ()) / float (ogl.m_states.nLastW);
	GLfloat y0 = 1.0f - float (top + CCanvas::Current ()->Top ()) / float (ogl.m_states.nLastH);
	GLfloat y1 = 1.0f - float (bot + CCanvas::Current ()->Top ()) / float (ogl.m_states.nLastH);
	ogl.SetTextureUsage (false);
	if (!colorP)
		colorP = &COLOR;
	OglCanvasColor (colorP);
	ogl.VertexBuffer () [0][X] = x0;
	ogl.VertexBuffer () [0][Y] = y0;
	ogl.VertexBuffer () [1][X] = x1;
	ogl.VertexBuffer () [1][Y] = y1;
	ogl.FlushBuffers (GL_LINES, 2, 2);
	}
}

//------------------------------------------------------------------------------

void OglDrawEmptyRect (int left, int top, int right, int bot, tCanvasColor* colorP)
{
if (ogl.SizeVertexBuffer (4)) {
	GLfloat x0 = float (left + CCanvas::Current ()->Left ()) / float (ogl.m_states.nLastW);
	GLfloat x1 = float (right + CCanvas::Current ()->Left ()) / float (ogl.m_states.nLastW);
	GLfloat y0 = 1.0f - float (top + CCanvas::Current ()->Top ()) / float (ogl.m_states.nLastH);
	GLfloat y1 = 1.0f - float (bot + CCanvas::Current ()->Top ()) / float (ogl.m_states.nLastH);
	ogl.SetTextureUsage (false);
	if (!colorP)
		colorP = &COLOR;
	OglCanvasColor (colorP);
	ogl.VertexBuffer () [0][X] = x0;
	ogl.VertexBuffer () [0][Y] = y0;
	ogl.VertexBuffer () [1][X] = x1;
	ogl.VertexBuffer () [1][Y] = y0;
	ogl.VertexBuffer () [2][X] = x1;
	ogl.VertexBuffer () [2][Y] = y1;
	ogl.VertexBuffer () [3][X] = x0;
	ogl.VertexBuffer () [3][Y] = y1;
	ogl.FlushBuffers (GL_LINE_LOOP, 4, 2);
	ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

//------------------------------------------------------------------------------
