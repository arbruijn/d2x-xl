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

void OglDrawPixel (int x, int y, CCanvasColor *colorP)
{
if (ogl.SizeVertexBuffer (1)) {
	ogl.SetTexturing (false);
	glPointSize (1.0);
	if (!colorP)
		colorP = &CCanvas::Current ()->Color ();
	OglCanvasColor (colorP);
	ogl.VertexBuffer () [0].v.coord.x = float (x /*+ CCanvas::Current ()->Left ()*/) / float (ogl.m_states.viewport [0].m_w);
	ogl.VertexBuffer () [0].v.coord.y = 1.0f - float (y /*+ CCanvas::Current ()->Top ()*/) / float (ogl.m_states.viewport [0].m_w);
	ogl.FlushBuffers (GL_POINT, 1, 2);
	}
}

//------------------------------------------------------------------------------

void OglDrawFilledRect (int left, int top, int right, int bot, CCanvasColor *colorP)
{
GLfloat x0 = float (left /*+ CCanvas::Current ()->Left ()*/) / float (ogl.m_states.viewport [0].m_w);
GLfloat x1 = float (right /*+ CCanvas::Current ()->Left ()*/) / float (ogl.m_states.viewport [0].m_w);
GLfloat y0 = 1.0f - float (top /*+ CCanvas::Current ()->Top ()*/) / float (ogl.m_states.viewport [0].m_h);
GLfloat y1 = 1.0f - float (bot /*+ CCanvas::Current ()->Top ()*/) / float (ogl.m_states.viewport [0].m_h);

ogl.SetTexturing (false);
if (!colorP)
	colorP = &CCanvas::Current ()->Color ();
OglCanvasColor (colorP);
CFloatVector vPosf;
vPosf.v.coord.x = (x0 + x1) / 2;
vPosf.v.coord.y = (y0 + y1) / 2;
ogl.RenderQuad (NULL, vPosf, (x1 - x0) / 2, (y1 - y0) / 2, 2);
//if (colorP->rgb || (gameStates.render.grAlpha < 1.0f))
//	ogl.SetBlending (false);
}

//------------------------------------------------------------------------------

void OglDrawFilledPoly (int* x, int* y, int nVerts, CCanvasColor *colorP, int nColors)
{
if (ogl.SizeBuffers (nVerts + 1)) {
	int left = 0/*CCanvas::Current ()->Left ()*/;
	int top = 0/*CCanvas::Current ()->Top ()*/;
	int j;
	bool bColor;

	ogl.SetTexturing (false);
	if (!(bColor = colorP && (nColors == nVerts)))
		OglCanvasColor (&CCanvas::Current ()->Color ());
	for (int i = 0; i <= nVerts; i++) {
		j = i % nVerts;
		if (bColor)
			OglCanvasColor (colorP + j, ogl.ColorBuffer () + i);
		ogl.VertexBuffer () [i].v.coord.x = GLfloat (x [j] + left) / GLfloat (ogl.m_states.viewport [0].m_w);
		ogl.VertexBuffer () [i].v.coord.y = 1.0f - GLfloat (y [j] + top) / GLfloat (ogl.m_states.viewport [0].m_h);
		}
	ogl.FlushBuffers (GL_POLYGON, nVerts + 1, 2);
	}
}

//------------------------------------------------------------------------------

void OglDrawLine (int left, int top, int right, int bot, CCanvasColor *colorP)
{
if (ogl.SizeVertexBuffer (2)) {
	GLfloat x0 = float (left/* + CCanvas::Current ()->Left ()*/) / float (ogl.m_states.viewport [0].m_w);
	GLfloat x1 = float (right /*+ CCanvas::Current ()->Left ()*/) / float (ogl.m_states.viewport [0].m_w);
	GLfloat y0 = 1.0f - float (top /*+ CCanvas::Current ()->Top ()*/) / float (ogl.m_states.viewport [0].m_h);
	GLfloat y1 = 1.0f - float (bot /*+ CCanvas::Current ()->Top ()*/) / float (ogl.m_states.viewport [0].m_h);
	ogl.SetTexturing (false);
	if (!colorP)
		colorP = &CCanvas::Current ()->Color ();
	OglCanvasColor (colorP);
	ogl.VertexBuffer () [0].v.coord.x = x0;
	ogl.VertexBuffer () [0].v.coord.y = y0;
	ogl.VertexBuffer () [1].v.coord.x = x1;
	ogl.VertexBuffer () [1].v.coord.y = y1;
	ogl.FlushBuffers (GL_LINES, 2, 2);
	}
}

//------------------------------------------------------------------------------

void OglDrawEmptyRect (int left, int top, int right, int bot, CCanvasColor* colorP)
{
if (ogl.SizeVertexBuffer (4)) {
	GLfloat x0 = float (left /*+ CCanvas::Current ()->Left ()*/) / float (ogl.m_states.viewport [0].m_w);
	GLfloat x1 = float (right /*+ CCanvas::Current ()->Left ()*/) / float (ogl.m_states.viewport [0].m_w);
	GLfloat y0 = 1.0f - float (top /*+ CCanvas::Current ()->Top ()*/) / float (ogl.m_states.viewport [0].m_h);
	GLfloat y1 = 1.0f - float (bot /*+ CCanvas::Current ()->Top ()*/) / float (ogl.m_states.viewport [0].m_h);
	ogl.SetTexturing (false);
	if (!colorP)
		colorP = &CCanvas::Current ()->Color ();
	OglCanvasColor (colorP);
	ogl.VertexBuffer () [0].v.coord.x = x0;
	ogl.VertexBuffer () [0].v.coord.y = y0;
	ogl.VertexBuffer () [1].v.coord.x = x1;
	ogl.VertexBuffer () [1].v.coord.y = y0;
	ogl.VertexBuffer () [2].v.coord.x = x1;
	ogl.VertexBuffer () [2].v.coord.y = y1;
	ogl.VertexBuffer () [3].v.coord.x = x0;
	ogl.VertexBuffer () [3].v.coord.y = y1;
	ogl.FlushBuffers (GL_LINE_LOOP, 4, 2);
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
	}
}

//------------------------------------------------------------------------------
