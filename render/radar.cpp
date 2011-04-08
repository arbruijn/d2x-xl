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
#include "radar.h"

// -----------------------------------------------------------------------------------

CRadar radar;

// -----------------------------------------------------------------------------------

int				CRadar::radarRanges [4] = {100, 150, 200, 500};

CAngleVector	CRadar::aRadar = CAngleVector::Create(I2X (1) / 4, 0, 0);
CFixMatrix		CRadar::mRadar;
float				CRadar::yOffs [2][CM_LETTERBOX + 1] = {{17.5f, -20.5f, 18.0f, -20.5f, -19.0f}, {17.5f, 20.5f, 18.0f, 20.5f, 19.0f}};
float				CRadar::yRadar;
float				CRadar::fRadius = 8.0f;
float				CRadar::fLineWidth = 1.0f;

tSinCosf			CRadar::sinCosRadar [RADAR_SLICES];
tSinCosf			CRadar::sinCosBlip [BLIP_SLICES];
int				CRadar::bInitSinCos = 1;

tRgbColorf		CRadar::shipColors [8];
tRgbColorf		CRadar::guidebotColor = {0, 0.75f / 4, 0.25f};
tRgbColorf		CRadar::robotColor = {0.75f / 4, 0, 0.25f};
tRgbColorf		CRadar::powerupColor = {0.25f, 0.5f / 4, 0};
tRgbColorf		CRadar::radarColor [2] = {{1, 1, 1}, {0, 0, 0}};
int				CRadar::bHaveShipColors = 0;


// -----------------------------------------------------------------------------------

void CRadar::RenderDevice (void)
{
if (bInitSinCos) {
	ComputeSinCosTable (sizeofa (sinCosRadar), sinCosRadar);
	ComputeSinCosTable (sizeofa (sinCosBlip), sinCosBlip);
	bInitSinCos = 0;
	}

	CFixMatrix mOrient;
	CFloatVector vf, vu;
	int i;

vf.Assign (gameData.objs.viewerP->Orientation ().m.dir.f);
vf *= 50.0f;
vu.Assign (gameData.objs.viewerP->Orientation ().m.dir.u);
vu *= yRadar;
CFixVector vPos;
vPos.Assign (vf + vu);
vPos += gameData.objs.viewerP->Position ();

int nSides = 64;
if (ogl.SizeVertexBuffer (nSides)) {
	glPushMatrix ();
	ogl.SetTransform (1);

	mOrient = CFixMatrix::IDENTITY;
	transformation.Begin (vPos, mOrient);
	glScalef (fRadius, fRadius, fRadius);
	//glColor4f (0.4f, 0.4f, 0.4f, 0.5f);
	glColor4f (0.0f, 0.5f, 0.0f, 0.5f);
	glLineWidth (fLineWidth);

	CFloatVector* pv = &ogl.VertexBuffer () [0];
	for (i = 0; i < nSides; i++, pv++) {
		double ang = 2.0 * Pi * i / nSides;
		pv->v.coord.x = float (cos (ang));
		pv->v.coord.y = float (sin (ang));
		pv->v.coord.z = 0.0f;
		}
	ogl.FlushBuffers (GL_LINES, nSides, 3);

	pv = &ogl.VertexBuffer () [0];
	for (i = 0; i < nSides; i++, pv++) {
		pv->v.coord.z = pv->v.coord.x; // cos
		pv->v.coord.x = 0.0f;
		}
	ogl.FlushBuffers (GL_LINES, nSides, 3);

	pv = &ogl.VertexBuffer () [0];
	for (i = 0; i < nSides; i++, pv++) {
		pv->v.coord.x = pv->v.coord.y; // sin
		pv->v.coord.y = 0.0f;
		}
	ogl.FlushBuffers (GL_LINES, nSides, 3);

	transformation.End ();
	glPopMatrix ();

	glPushMatrix ();
	mOrient = gameData.objs.viewerP->Orientation ();
	transformation.Begin (vPos, mOrient);

	//glColor4f (r, g, b, a);
	glColor4f (0.0f, 0.5f, 0.0f, 0.25f);
	glScalef (fRadius, fRadius, fRadius);
	ogl.FlushBuffers (GL_POLYGON, nSides, 3);

	glColor4f (0.0f, 0.6f, 0.0f, 0.5f);
	glLineWidth (1.5f * fLineWidth);
	ogl.FlushBuffers (GL_LINE_LOOP, nSides, 3);

	glColor4f (0.0f, 0.8f, 0.0f, 0.5f);
	//glLineWidth (1.5f * fLineWidth);
	glScalef (0.6666667f, 0.6666667f, 0.6666667f);
	ogl.FlushBuffers (GL_LINE_LOOP, nSides, 3);

	glColor4f (0.0f, 1.0f, 0.0f, 0.5f);
	//glLineWidth (2.0f * fLineWidth);
	glScalef (0.5f, 0.5f, 0.5f);
	ogl.FlushBuffers (GL_LINE_LOOP, nSides, 3);

#if 0
	CFloatVector	vAxis [8], vOffset;

	pv = &ogl.VertexBuffer () [0];

	vAxis [0] = pv [nSides / 8];
	vAxis [1] = pv [3 * nSides / 8];
	vAxis [2] = pv [5 * nSides / 8];
	vAxis [3] = pv [7 * nSides / 8];

	vOffset = (vAxis [2] - vAxis [0]) * 0.1625f;
	pv [0] = vAxis [0];
	pv [1] = vAxis [0] + vOffset;
	pv [2] = vAxis [2];
	pv [3] = vAxis [2] - vOffset;
	vOffset = (vAxis [3] - vAxis [1]) * 0.1625f;
	pv [4] = vAxis [1];
	pv [5] = vAxis [1] + vOffset;
	pv [6] = vAxis [3];
	pv [7] = vAxis [3] - vOffset;

	glScalef (3.0f, 3.0f, 3.0f);
	glColor4f (0.0f, 0.6f, 0.0f, 0.5f);
	ogl.FlushBuffers (GL_LINES, 8, 3);
#endif
	transformation.End ();
	glPopMatrix ();

	mOrient = CFixMatrix::IDENTITY;
	transformation.Begin (vPos, mOrient);
	glScalef (fRadius, fRadius, fRadius);

	pv = &ogl.VertexBuffer () [0];
	pv [0].Set (-1.0f, 0.0f, 0.0f);
	pv [1].Set (-0.5f, 0.0f, 0.0f);
	pv [2].Set (1.0f, 0.0f, 0.0f);
	pv [3].Set (0.5f, 0.0f, 0.0f);
	pv [4].Set (0.0f, -1.0f, 0.0f);
	pv [5].Set (0.0f, -0.5f, 0.0f);
	pv [6].Set (0.0f, 1.0f, 0.0f);
	pv [7].Set (0.0f, 0.5f, 0.0f);
	pv [8].Set (0.0f, 0.0f, -1.0f);
	pv [9].Set (0.0f, 0.0f, -0.5f);
	pv [10].Set (0.0f, 0.0f, 1.0f);
	pv [11].Set (0.0f, 0.0f, 0.5f);

	glColor4f (0.0f, 0.333f, 0.0f, 0.5f);
	glLineWidth (fLineWidth);
	ogl.FlushBuffers (GL_LINES, 12, 3);
	transformation.End ();
	glPopMatrix ();

	ogl.SetTransform (0);
	}

glLineWidth (2);
glPopMatrix ();
return;
}

// -----------------------------------------------------------------------------------

void CRadar::RenderBlip (CObject *objP, float r, float g, float b, float a, int bAbove)
{
	CFixVector	n, v [2];
	fix			m;
	float			h, s;

n = objP->info.position.vPos;
if ((bAbove >= 0) && ((n.v.coord.y < gameData.objs.viewerP->Position ().v.coord.y) != bAbove))
	return;
transformation.Transform (n, n, 0);
if ((m = n.Mag ()) > I2X (RADAR_RANGE))
	return;
if (m) {
	//HUDMessage (0, "%1.2f", X2F (m));
	v [0].v.coord.x = FixDiv (n.v.coord.x, m) * 15; // /= RADAR_RANGE;
	v [0].v.coord.y = FixDiv (n.v.coord.y, m) * 20; // /= RADAR_RANGE;
	v [0].v.coord.z = n.v.coord.x / RADAR_RANGE;
	//VmVecNormalize (&n);
	}
else {
	//glTranslatef (0.0f, 0.0f, 50.0f);
	//glScalef (fRadius, fRadius, fRadius);
	}
v [0] *= FixDiv (1, 3);
h = X2F (n.v.coord.z) / RADAR_RANGE;
glPushMatrix ();
glTranslatef (0, yRadar + h * fRadius / 3.0f, 50);
glPushMatrix ();
s = 1.0f - (float) fabs (X2F (m) / RADAR_RANGE);
h = 3 * s;
a += a * h;
glColor4f (r + r * h, g + g * h, b + b * h, (float) sqrt (a));
glTranslatef (X2F (v [0].v.coord.x), X2F (v [0].v.coord.y), X2F (v [0].v.coord.z));
OglDrawEllipse (BLIP_SLICES, GL_POLYGON, 0.33f + 0.33f * s, 0, 0.33f + 0.33f * s, 0, sinCosBlip);
glPopMatrix ();
#if 1
v [1] = v [0];
v [1].v.coord.y = 0;
if (ogl.SizeVertexBuffer (2)) {
	ogl.VertexBuffer () [0].Assign (v [0]);
	ogl.VertexBuffer () [1].Assign (v [1]);
	ogl.FlushBuffers (GL_LINES, 2);
	}
#endif
glPopMatrix ();
}

// -----------------------------------------------------------------------------------

void CRadar::RenderObjects (int bAbove)
{
	CObject*		objP;
	tRgbColorf*	pc = radarColor + gameOpts->render.automap.nColor;

glLineWidth (3);
FORALL_OBJS (objP, i) {
	if ((objP->info.nType == OBJ_PLAYER) && (objP != gameData.objs.consoleP)) {
		if (AM_SHOW_PLAYERS && AM_SHOW_PLAYER (objP->info.nId)) {
			pc = shipColors + (IsTeamGame ? GetTeam (objP->info.nId) : objP->info.nId);
			RenderBlip (objP, pc->red, pc->green, pc->blue, 0.9f / 4, bAbove);
			}
		}
	else if (objP->info.nType == OBJ_ROBOT) {
		if (AM_SHOW_ROBOTS) {
			if (ROBOTINFO (objP->info.nId).companion)
				RenderBlip (objP, guidebotColor.red, guidebotColor.green, guidebotColor.blue, 0.9f / 4, bAbove);
			else
				RenderBlip (objP, robotColor.red, robotColor.green, robotColor.blue, 0.9f / 4, bAbove);
			}
		}
	else if (objP->info.nType == OBJ_POWERUP) {
		if (AM_SHOW_POWERUPS (2))
			RenderBlip (objP, powerupColor.red, powerupColor.green, powerupColor.blue, 0.9f / 4, bAbove);
		}
	}
}

// -----------------------------------------------------------------------------------

void CRadar::InitShipColors (void)
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

void CRadar::Render (void)
{
if (gameStates.app.bNostalgia)
	return;
if (cockpit->Hide ())
	return;
if (automap.Display ())
	return;
if (gameStates.zoom.nFactor > gameStates.zoom.nMinFactor)
	return;

int bRadar = EGI_FLAG (nRadar, 0, 1, 0);
if (!bRadar)
	return;
int bStencil = ogl.StencilOff ();
InitShipColors ();
yRadar = yOffs [bRadar - 1][gameStates.render.cockpit.nType];
fRadius = 4.0f / transformation.m_info.scalef.v.coord.x;
fLineWidth = float (CCanvas::Current ()->Width ()) / 640.0f;
mRadar = CFixMatrix::Create (aRadar);
ogl.ResetClientStates (1);
ogl.SetTexturing (false);
ogl.SetFaceCulling (false);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
ogl.SetLineSmooth (true);
glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
glLineWidth (fLineWidth);
if (ogl.HaveDrawBuffer ()) {
	ogl.SelectGlowBuffer ();
	RenderDevice ();
	RenderObjects (-1);
	ogl.RenderScreenQuad (ogl.DrawBuffer (2)->ColorBuffer ());
	ogl.ChooseDrawBuffer ();
	}
else {
	ogl.SetDepthTest (false);
	RenderObjects (1);
	RenderDevice ();
	RenderObjects (0);
	ogl.SetDepthTest (true);
	}
ogl.SetLineSmooth (false);
glLineWidth (1);
ogl.SetFaceCulling (true);
ogl.StencilOn (bStencil);
}

//------------------------------------------------------------------------------
// eof
