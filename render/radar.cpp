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

int				CRadar::radarRanges [5] = {0, 100, 150, 200, 500};
float				CRadar::radarSizes [3] = {3.0f, 4.0f, 5.0f};
float				CRadar::sizeOffsets [2][3] = {{-1.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 1.0f}};

CAngleVector	CRadar::aRadar = CAngleVector::Create(I2X (1) / 4, 0, 0);
float				CRadar::yOffs [2][CM_LETTERBOX + 1] = {{17.0f, -20.5f, 18.0f, -20.5f, -19.0f}, {17.0f, 20.5f, 18.0f, 20.5f, 19.0f}};

tSinCosf			CRadar::sinCosRadar [RADAR_SLICES];
tSinCosf			CRadar::sinCosBlip [BLIP_SLICES];
int				CRadar::bInitSinCos = 1;

tRgbColorf		CRadar::shipColors [8];
tRgbColorf		CRadar::guidebotColor = {0, 0.75f / 4, 0.25f};
tRgbColorf		CRadar::robotColor = {0.75f / 4, 0, 0.25f};
tRgbColorf		CRadar::powerupColor = {0.25f, 0.5f / 4, 0};
tRgbColorf		CRadar::radarColor [2] = {{1, 1, 1}, {0, 0, 0}};
int				CRadar::bHaveShipColors = 0;

#define RADAR_RANGE	radarRanges [gameOpts->render.cockpit.nRadarRange]

// -----------------------------------------------------------------------------------

void CRadar::RenderDevice (bool bBackground)
{
if (bInitSinCos) {
	ComputeSinCosTable (sizeofa (sinCosRadar), sinCosRadar);
	ComputeSinCosTable (sizeofa (sinCosBlip), sinCosBlip);
	bInitSinCos = 0;
	}

	CFixMatrix mOrient;
	CFloatVector vf, vu, vr;
	int i;

vf.Assign (gameData.objs.viewerP->Orientation ().m.dir.f);
vf *= m_offset.v.coord.z;
vu.Assign (gameData.objs.viewerP->Orientation ().m.dir.u);
vu *= m_offset.v.coord.y;
vr.Assign (gameData.objs.viewerP->Orientation ().m.dir.r);
vr *= m_offset.v.coord.x;
m_vCenter.Assign (vf + vu + vr);
m_vCenter += gameData.objs.viewerP->Position ();

int nSides = 64;
if (ogl.SizeVertexBuffer (nSides)) {
	CFloatVector* pv = &ogl.VertexBuffer () [0];

	for (i = 0; i < nSides; i++, pv++) {
		double ang = 2.0 * Pi * i / nSides;
		pv->v.coord.x = float (cos (ang));
		pv->v.coord.y = float (sin (ang));
		pv->v.coord.z = 0.0f;
		}

	ogl.SetTransform (1);

	if (bBackground) {
		glPushMatrix ();
		mOrient = gameData.objs.viewerP->Orientation ();
		transformation.Begin (m_vCenter, mOrient);
		glScalef (m_radius * 1.2f, m_radius * 1.2f, m_radius * 1.2f);
		glColor4f (0.0f, 0.0f, 0.0f, 0.5f);
		ogl.FlushBuffers (GL_POLYGON, nSides, 3);
		transformation.End ();
		glPopMatrix ();
		}
	else {
		glPushMatrix ();
		mOrient = CFixMatrix::IDENTITY;
		transformation.Begin (m_vCenter, mOrient);
		glScalef (m_radius, m_radius, m_radius);

		glColor4f (0.0f, 0.5f, 0.0f, 0.5f);
		glLineWidth (m_lineWidth);

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
		transformation.Begin (m_vCenter, mOrient);

		//glColor4f (r, g, b, a);
		glColor4f (0.0f, 0.5f, 0.0f, 0.25f);
		glScalef (m_radius, m_radius, m_radius);
		ogl.FlushBuffers (GL_POLYGON, nSides, 3);

		glColor4f (0.0f, 0.6f, 0.0f, 0.5f);
		glLineWidth (1.5f * m_lineWidth);
		ogl.FlushBuffers (GL_LINE_LOOP, nSides, 3);

		glColor4f (0.0f, 0.8f, 0.0f, 0.5f);
		//glLineWidth (1.5f * m_lineWidth);
		glScalef (0.6666667f, 0.6666667f, 0.6666667f);
		ogl.FlushBuffers (GL_LINE_LOOP, nSides, 3);

		glColor4f (0.0f, 1.0f, 0.0f, 0.5f);
		//glLineWidth (2.0f * m_lineWidth);
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
		transformation.Begin (m_vCenter, mOrient);
		glScalef (m_radius, m_radius, m_radius);

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
		glLineWidth (m_lineWidth);
		ogl.FlushBuffers (GL_LINES, 12, 3);
		transformation.End ();
		glPopMatrix ();

		ogl.SetTransform (0);
		}
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
transformation.Transform (n, n, 0);
if ((n.v.coord.y < gameData.objs.viewerP->Position ().v.coord.y) != bAbove)
	return;
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
	//glTranslatef (0.0f, 0.0f, m_offset.v.coord.z);
	//glScalef (m_radius, m_radius, m_radius);
	}
v [0] *= FixDiv (1, 3);
h = X2F (n.v.coord.z) / RADAR_RANGE;
glPushMatrix ();
glTranslatef (m_offset.v.coord.x, m_offset.v.coord.y + h * m_radius / 3.0f, m_offset.v.coord.z);
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
extraGameInfo [0].nRadar = (gameOpts->render.cockpit.nRadarRange > 0);
if (!EGI_FLAG (nRadar, 0, 1, 0))
	return;
if (gameStates.zoom.nFactor > gameStates.zoom.nMinFactor)
	return;

int bStencil = ogl.StencilOff ();
InitShipColors ();

m_offset.v.coord.x = 0.0f;
m_offset.v.coord.y = /*ogl.m_states.bRender2TextureOk ? 0.0f :*/ yOffs [gameOpts->render.cockpit.nRadarPos][gameStates.render.cockpit.nType];
m_offset.v.coord.y += sizeOffsets [gameOpts->render.cockpit.nRadarPos][gameOpts->render.cockpit.nRadarSize];
m_offset.v.coord.z = /*ogl.m_states.bRender2TextureOk ? 10.0f :*/ 50.0f;
m_radius = radarSizes [gameOpts->render.cockpit.nRadarSize] / transformation.m_info.scalef.v.coord.x;
m_lineWidth = float (CCanvas::Current ()->Width ()) / 640.0f;

ogl.ResetClientStates (1);
ogl.SetTexturing (false);
ogl.SetFaceCulling (false);
ogl.SetBlending (1);
ogl.SetBlendMode (0);
ogl.SetLineSmooth (true);
glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
glLineWidth (m_lineWidth);
#if 0
if (ogl.m_states.bRender2TextureOk) {
	//static tTexCoord2f texCoord [4] = {{{0.3f, 0.3f}}, {{0.3f, 0.7f}}, {{0.7f, 0.7f}}, {{0.7f, 0.3f}}};
	static float texCoord [4][2] = {{0,0},{0,1},{1,1},{1,0}};
	static float verts [4][2] = {{0.2f, 1.0f}, {0.2f, 0.8f}, {0.4f, 0.8f}, {0.4f, 1.0f}};

	ogl.SelectGlowBuffer ();
	glClearColor (0,0,0,0);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ogl.SetDepthTest (false);
	RenderObjects (1);
	RenderDevice ();
	RenderObjects (0);
	ogl.SetDepthTest (true);

	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	glLoadIdentity ();//clear matrix
	glMatrixMode (GL_PROJECTION);
	glPushMatrix ();
	glLoadIdentity ();//clear matrix
	glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
	ogl.Viewport (0, 0, screen.Width (), screen.Height ());

	ogl.ChooseDrawBuffer ();
#if 0
	ogl.RenderScreenQuad (ogl.DrawBuffer (2)->ColorBuffer ());
#else
	ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
	ogl.BindTexture (ogl.DrawBuffer (2)->ColorBuffer ());
	OglTexCoordPointer (2, GL_FLOAT, 0, texCoord);
	OglVertexPointer (2, GL_FLOAT, 0, verts);

	ogl.SetDepthMode (GL_ALWAYS);
	glColor3f (1,1,1);
	OglDrawArrays (GL_QUADS, 0, 4);
	ogl.SetDepthMode (GL_LEQUAL);

	glMatrixMode (GL_PROJECTION);
	glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix ();

#endif
	}
else 
#endif
	{
	ogl.SetDepthTest (false);
	RenderDevice (true);
	RenderObjects (1);
	RenderDevice (false);
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
