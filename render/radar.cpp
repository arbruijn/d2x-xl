#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "rendermine.h"
#include "cockpit.h"
#include "automap.h"
#include "ogl_render.h"

// -----------------------------------------------------------------------------------

int radarRanges [] = {100, 150, 200};

#define RADAR_RANGE	radarRanges [gameOpts->render.automap.nRange]
#define RADAR_SLICES	40
#define BLIP_SLICES	40

static CAngleVector	aRadar = CAngleVector::Create(I2X (1) / 4, 0, 0);
static CFixMatrix		mRadar;
static float			yOffs = 18.0f;
static float			yRadar, fRadius = 10.0f;
static float			fLineWidth = 1.0f;

void RenderRadarBlip (CObject *objP, float r, float g, float b, float a)
{
	CFixVector	n, v [2];
	fix			m;
	float			h, s;

	static tSinCosf sinCosRadar [RADAR_SLICES];
	static tSinCosf sinCosBlip [BLIP_SLICES];
	static int bInitSinCos = 1;

if (bInitSinCos) {
	ComputeSinCosTable (sizeofa (sinCosRadar), sinCosRadar);
	ComputeSinCosTable (sizeofa (sinCosBlip), sinCosBlip);
	bInitSinCos = 0;
	}
n = objP->info.position.vPos;
transformation.Transform (n, n, 0);
if ((m = n.Mag ()) > I2X (RADAR_RANGE))
	return;
if (m) {
	//HUDMessage (0, "%1.2f", X2F (m));
	v [0][X] = FixDiv (n [X], m) * 15; // /= RADAR_RANGE;
	v [0][Y] = FixDiv (n [Y], m) * 20; // /= RADAR_RANGE;
	v [0][Z] = n[X] / RADAR_RANGE;
	//VmVecNormalize (&n);
	}
else {
	glPushMatrix ();
	glColor4f (r, g, b, a);
	glTranslatef (0, yRadar, 50);
#if 0
	glColor4f (r, g, b, a / 2);
 	OglDrawEllipse (RADAR_SLICES, GL_POLYGON, 10, 0, 7.5f, 0, sinCosRadar);
#endif
	glColor4f (r, g, b, a);
 	OglDrawEllipse (RADAR_SLICES, GL_POLYGON, fRadius, 0, fRadius / 3.0f, 0, sinCosRadar);
	glColor4f (0.5f, 0.5f, 0.5f, 0.8f);
	glLineWidth (fLineWidth);
 	OglDrawEllipse (RADAR_SLICES, GL_LINE_LOOP, fRadius, 0, fRadius / 3.0f, 0, sinCosRadar);
	glColor4f (0.6f, 0.6f, 0.6f, 0.7f);
	glLineWidth (1.5f * fLineWidth);
 	OglDrawEllipse (RADAR_SLICES, GL_LINE_LOOP, 2 * fRadius / 3.0f, 0, 2 * fRadius / 9.0f, 0, sinCosRadar);
	glColor4f (0.7f, 0.7f, 0.7f, 0.6f);
	glLineWidth (2 * fLineWidth);
 	OglDrawEllipse (RADAR_SLICES, GL_LINE_LOOP, fRadius / 3.0f, 0, fRadius / 9.0f, 0, sinCosRadar);
	glLineWidth (fLineWidth);
	if (ogl.SizeVertexBuffer (6)) {
		float x = fRadius * 0.707f + 0.333f;
		float y = fRadius / 3.0f;
		ogl.VertexBuffer () [0][X] = 0;
		ogl.VertexBuffer () [0][Y] = y + 1;
		ogl.VertexBuffer () [1][X] = 0;
		ogl.VertexBuffer () [1][Y] = -y - 1;
		y -= 0.333f;
		ogl.VertexBuffer () [2][X] = -x;
		ogl.VertexBuffer () [2][Y] = -y;
		ogl.VertexBuffer () [3][X] = x;
		ogl.VertexBuffer () [3][Y] = y;
		ogl.VertexBuffer () [4][X] = -x;
		ogl.VertexBuffer () [4][Y] = y;
		ogl.VertexBuffer () [5][X] = x;
		ogl.VertexBuffer () [5][Y] = -y;
		ogl.FlushBuffers (GL_LINES, 6, 2);
		}
	//ogl.SetLineSmooth (false);
	glLineWidth (2);
	glPopMatrix ();
	return;
	}
v [0] *= FixDiv (1, 3);
h = X2F (n [Z]) / RADAR_RANGE;
glPushMatrix ();
glTranslatef (0, yRadar + h * fRadius / 3.0f, 50);
glPushMatrix ();
s = 1.0f - (float) fabs (X2F (m) / RADAR_RANGE);
h = 3 * s;
a += a * h;
glColor4f (r + r * h, g + g * h, b + b * h, (float) sqrt (a));
glTranslatef (X2F (v [0][X]), X2F (v [0][Y]), X2F (v [0][Z]));
OglDrawEllipse (BLIP_SLICES, GL_POLYGON, 0.33f + 0.33f * s, 0, 0.33f + 0.33f * s, 0, sinCosBlip);
glPopMatrix ();
#if 1
v [1] = v [0];
v [1][Y] = 0;
if (ogl.SizeVertexBuffer (2)) {
	ogl.VertexBuffer () [0].Assign (v [0]);
	ogl.VertexBuffer () [1].Assign (v [1]);
	ogl.FlushBuffers (GL_LINES, 2);
	}
#endif
glPopMatrix ();
}

// -----------------------------------------------------------------------------------

static tRgbColorf shipColors [8];
static tRgbColorf guidebotColor = {0, 0.75f / 4, 0.25f};
static tRgbColorf robotColor = {0.75f / 4, 0, 0.25f};
static tRgbColorf powerupColor = {0.25f, 0.5f / 4, 0};
static tRgbColorf radarColor [2] = {{1, 1, 1}, {0, 0, 0}};
static int bHaveShipColors = 0;

void InitShipColors (void)
{
if (!bHaveShipColors) {
	int	i;

	for (i = 0; i < 8; i++) {
		shipColors [i].red = 2 * playerColors [i].red / 255.0f;
		shipColors [i].green = 2 * playerColors [i].green / 255.0f;
		shipColors [i].blue = 2 * playerColors [i].blue / 255.0f;
		}
	bHaveShipColors = 1;
	}
}

// -----------------------------------------------------------------------------------

void RenderRadar (void)
{
	int			i, bStencil;
	CObject		*objP;
	tRgbColorf	*pc;

if (gameStates.app.bNostalgia)
	return;
if (cockpit->Hide ())
	return;
if (automap.Display ())
	return;
if (gameStates.zoom.nFactor > gameStates.zoom.nMinFactor)
	return;
if (!(i = EGI_FLAG (nRadar, 0, 1, 0)))
	return;
bStencil = ogl.StencilOff ();
InitShipColors ();
yRadar = ((i == 1) || (gameStates.render.cockpit.nType == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nType == CM_STATUS_BAR)) ? yOffs : -yOffs;
fRadius = 5.0f / transformation.m_info.scalef [X];
fLineWidth = float (CCanvas::Current ()->Width ()) / 640.0f;
mRadar = CFixMatrix::Create (aRadar);
ogl.SelectTMU (GL_TEXTURE3);
ogl.SetTexturing (false);
ogl.SelectTMU (GL_TEXTURE2);
ogl.SetTexturing (false);
ogl.SelectTMU (GL_TEXTURE1);
ogl.SetTexturing (false);
ogl.SelectTMU (GL_TEXTURE0);
ogl.SetTexturing (false);
ogl.SetFaceCulling (false);
ogl.SetDepthTest (false);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
ogl.SetLineSmooth (true);
glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
glLineWidth (fLineWidth);
pc = radarColor + gameOpts->render.automap.nColor;
RenderRadarBlip (gameData.objs.consoleP, pc->red, pc->green, pc->blue, 2.0f / 3.0f); //0.5, 0.75, 0.5, 2.0f / 3.0f);
glLineWidth (3);
FORALL_OBJS (objP, i) {
	if ((objP->info.nType == OBJ_PLAYER) && (objP != gameData.objs.consoleP)) {
		if (AM_SHOW_PLAYERS && AM_SHOW_PLAYER (objP->info.nId)) {
			pc = shipColors + (IsTeamGame ? GetTeam (objP->info.nId) : objP->info.nId);
			RenderRadarBlip (objP, pc->red, pc->green, pc->blue, 0.9f / 4);
			}
		}
	else if (objP->info.nType == OBJ_ROBOT) {
		if (AM_SHOW_ROBOTS) {
			if (ROBOTINFO (objP->info.nId).companion)
				RenderRadarBlip (objP, guidebotColor.red, guidebotColor.green, guidebotColor.blue, 0.9f / 4);
			else
				RenderRadarBlip (objP, robotColor.red, robotColor.green, robotColor.blue, 0.9f / 4);
			}
		}
	else if (objP->info.nType == OBJ_POWERUP) {
		if (AM_SHOW_POWERUPS (2))
			RenderRadarBlip (objP, powerupColor.red, powerupColor.green, powerupColor.blue, 0.9f / 4);
		}
	}
ogl.SetLineSmooth (false);
glLineWidth (1);
ogl.SetDepthTest (true);
ogl.SetFaceCulling (true);
ogl.StencilOn (bStencil);
}

//------------------------------------------------------------------------------
// eof
