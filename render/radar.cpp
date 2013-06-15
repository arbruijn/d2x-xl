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

#if 1
static CFloatVector lineColors [3] = {
	{{{0.5f, 0.0f, 1.0f, 0.5f}}},
	{{{1.0f, 0.5f, 0.0f, 0.5f}}},
	{{{0.0f, 0.5f, 1.0f, 0.5f}}}
	};

static CFloatVector planeColors [3] = {
	{{{0.0f, 0.5f, 0.0f, 0.25f}}},
	{{{0.6f, 0.6f, 0.0f, 0.25f}}},
	{{{0.0f, 0.4f, 0.6f, 0.25f}}}
	};

static CFloatVector radarColors [3][3] = {
	{
	{{{0.0f, 0.6f, 0.0f, 0.5f}}},
	{{{0.0f, 0.8f, 0.0f, 0.5f}}},
	{{{0.0f, 1.0f, 0.0f, 0.5f}}}
	},
	{
	{{{0.4f, 0.4f, 0.0f, 0.5f}}},
	{{{0.6f, 0.6f, 0.0f, 0.5f}}},
	{{{0.8f, 0.8f, 0.0f, 0.5f}}}
	},
	{
	{{{0.0f, 0.2f, 0.4f, 0.5f}}},
	{{{0.0f, 0.4f, 0.6f, 0.5f}}},
	{{{0.0f, 0.6f, 0.8f, 0.5f}}}
	},
};
#endif
CRadar radar;

// -----------------------------------------------------------------------------------

int				CRadar::radarRanges [5] = {0, 100, 250, 500, 2000};
float				CRadar::radarSizes [3] = {2.0f, 3.0f, 4.0f};
float				CRadar::sizeOffsets [2][3] = {{4.0f, 2.0f, 0.0f}, {4.0f, 2.0f, 0.0f}};

CAngleVector	CRadar::aRadar = CAngleVector::Create(I2X (1) / 4, 0, 0);
float				CRadar::yOffs [2][CM_LETTERBOX + 1] = {{17.0f, -20.5f, 18.0f, -20.5f, -19.0f}, {16.0f, 19.5f, 17.0f, 19.5f, 18.0f}};

tSinCosf			CRadar::sinCosRadar [RADAR_SLICES];
tSinCosf			CRadar::sinCosBlip [BLIP_SLICES];

CFloatVector3		CRadar::shipColors [8];
CFloatVector3		CRadar::guidebotColor = {{{0, 0.75f / 4, 0.25f}}};
CFloatVector3		CRadar::robotColor = {{{0.75f / 4, 0, 0.25f}}};
CFloatVector3		CRadar::powerupColor = {{{0.25f, 0.5f / 4, 0}}};
CFloatVector3		CRadar::radarColor [2] = {{{{1, 1, 1}}}, {{{0, 0, 0}}}};

#define RADAR_RANGE	radarRanges [gameOpts->render.cockpit.nRadarRange]

// -----------------------------------------------------------------------------------

CRadar::CRadar () 
	: m_lineWidth (1.0f) 
{
ComputeSinCosTable (sizeofa (sinCosRadar), sinCosRadar);
ComputeSinCosTable (sizeofa (sinCosBlip), sinCosBlip);

int i;

for (i = 0; i < 8; i++) {
	shipColors [i].Set (playerColors [i].r, playerColors [i].g, playerColors [i].b);
	shipColors [i] /= (255.0f * 0.5f);
	}

for (i = 0; i < RADAR_SLICES; i++) {
	double a = 2.0 * Pi * float (i) / float (RADAR_SLICES);
	m_vertices [i].Set (float (cos (a)), float (sin (a)), 0.0f);
	}

}

// -----------------------------------------------------------------------------------

void CRadar::ComputeCenter (void)
{
	CFloatVector vf, vu, vr;

vf.Assign (gameData.objs.viewerP->Orientation ().m.dir.f);
vf *= m_offset.v.coord.z;
vu.Assign (gameData.objs.viewerP->Orientation ().m.dir.u);
vu *= m_offset.v.coord.y;
vr.Assign (gameData.objs.viewerP->Orientation ().m.dir.r);
vr *= m_offset.v.coord.x;
m_vCenter.Assign (vf + vu + vr);
m_vCenter += gameData.objs.viewerP->Position ();
m_vCenterf.Assign (m_vCenter);
}

// -----------------------------------------------------------------------------------

void CRadar::RenderSetup (void)
{
#if 1
memcpy (ogl.VertexBuffer ().Buffer (), m_vertices, sizeof (m_vertices));
#else
CFloatVector* pv = &ogl.VertexBuffer () [0];
for (int i = 0; i < RADAR_SLICES; i++, pv++) {
	double ang = 2.0 * Pi * i / RADAR_SLICES;
	pv->v.coord.x = float (cos (ang));
	pv->v.coord.y = float (sin (ang));
	pv->v.coord.z = 0.0f;
	}
#endif
}

// -----------------------------------------------------------------------------------

void CRadar::RenderBackground (void)
{
// glPushMatrix ();
RenderSetup ();
ogl.SetTransform (1);
CFixMatrix mOrient = gameData.objs.viewerP->Orientation ();
transformation.Begin (m_vCenter, mOrient);
glScalef (m_radius * 1.2f, m_radius * 1.2f, m_radius * 1.2f);
glColor4f (0.0f, 0.0f, 0.0f, 0.5f);
ogl.FlushBuffers (GL_POLYGON, RADAR_SLICES, 3);
#if 0
glColor4f (0.125f, 0.125f, 0.125f, 0.5f);
ogl.FlushBuffers (GL_LINE_LOOP, RADAR_SLICES, 3);
#endif
transformation.End ();
ogl.SetTransform (0);
// glPopMatrix ();
}

// -----------------------------------------------------------------------------------

void CRadar::RenderDevice (void)
{
	CFloatVector* pv;
	CFixMatrix mOrient;
	int i;

RenderSetup ();
ogl.SetTransform (1);
ogl.SetTexturing (false);

// render the three dashed, moving rings
// glPushMatrix ();
mOrient = CFixMatrix::IDENTITY;
transformation.Begin (m_vCenter, mOrient);
glScalef (m_radius, m_radius, m_radius);
glLineWidth (m_lineWidth);

if (gameOpts->render.cockpit.nRadarStyle)
	glColor4fv ((GLfloat*) &lineColors [0]);
else
	glColor4fv ((GLfloat*) &radarColors [gameOpts->render.cockpit.nRadarColor]);
ogl.FlushBuffers (GL_LINES, RADAR_SLICES, 3);

pv = &ogl.VertexBuffer () [0];
for (i = 0; i < RADAR_SLICES; i++, pv++) {
	pv->v.coord.z = pv->v.coord.x; // cos
	pv->v.coord.x = 0.0f;
	}
if (gameOpts->render.cockpit.nRadarStyle)
	glColor4fv ((GLfloat*) &lineColors [1]);
ogl.FlushBuffers (GL_LINES, RADAR_SLICES, 3);

pv = &ogl.VertexBuffer () [0];
for (i = 0; i < RADAR_SLICES; i++, pv++) {
	pv->v.coord.x = pv->v.coord.y; // sin
	pv->v.coord.y = 0.0f;
	}
if (gameOpts->render.cockpit.nRadarStyle)
	glColor4fv ((GLfloat*) &lineColors [2]);
ogl.FlushBuffers (GL_LINES, RADAR_SLICES, 3);

transformation.End ();
// glPopMatrix ();

// render the radar plane
// glPushMatrix ();
mOrient = gameData.objs.viewerP->Orientation ();
transformation.Begin (m_vCenter, mOrient);

// render the transparent green dish
glColor4fv ((GLfloat*) &planeColors [gameOpts->render.cockpit.nRadarColor]);
glScalef (m_radius, m_radius, m_radius);
ogl.FlushBuffers (GL_POLYGON, RADAR_SLICES, 3);

// render the green rings (dark to bright from outside to inside)
glColor4fv ((GLfloat*) &radarColors [gameOpts->render.cockpit.nRadarColor][0]);
glLineWidth (1.5f * m_lineWidth);
ogl.FlushBuffers (GL_LINE_LOOP, RADAR_SLICES, 3);

glColor4fv ((GLfloat*) &radarColors [gameOpts->render.cockpit.nRadarColor][1]);
glScalef (0.6666667f, 0.6666667f, 0.6666667f);
ogl.FlushBuffers (GL_LINE_LOOP, RADAR_SLICES, 3);

glColor4fv ((GLfloat*) &radarColors [gameOpts->render.cockpit.nRadarColor][2]);
glScalef (0.5f, 0.5f, 0.5f);
ogl.FlushBuffers (GL_LINE_LOOP, RADAR_SLICES, 3);

#if 0
CFloatVector	vAxis [8], vOffset;

// render 4 short lines pointing from the outer ring of the radar dish to its center
// disabled as it makes the radar look too cluttered
pv = &ogl.VertexBuffer () [0];

vAxis [0] = pv [RADAR_SLICES / 8];
vAxis [1] = pv [3 * RADAR_SLICES / 8];
vAxis [2] = pv [5 * RADAR_SLICES / 8];
vAxis [3] = pv [7 * RADAR_SLICES / 8];

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
// glPopMatrix ();

// glPushMatrix ();
mOrient = CFixMatrix::IDENTITY;
transformation.Begin (m_vCenter, mOrient);
glScalef (m_radius, m_radius, m_radius);

// render the 6 lines pointing inwards from where the dashed rings touch
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
// glPopMatrix ();

ogl.SetTransform (0);
glLineWidth (2);
}

// -----------------------------------------------------------------------------------

void CRadar::RenderBlip (CObject *objP, float r, float g, float b, float a, int bAbove)
{
	CFloatVector	v [2];
	float				m, h, s;

v [0].Assign (objP->info.position.vPos);
transformation.Transform (v [0], v [0], 0);
if ((v [0].v.coord.y < X2F (gameData.objs.viewerP->Position ().v.coord.y)) != bAbove)
	return;
if ((m = v [0].Mag ()) > RADAR_RANGE)
	return;
v [0] *= m_radius / float (RADAR_RANGE);
v [0].v.coord.z = v [0].v.coord.z;
v [0] += m_offset;
v [1].v.coord.x = v [0].v.coord.x;
v [1].v.coord.y = m_offset.v.coord.y;
v [1].v.coord.z = v [0].v.coord.z;
#if 0 // increase distance from radar plane
CFloatVector hv = v [0] - v [1];
hv *= 0.5f;
v [0] += hv;
#endif
s = 1.0f - fabs (m) / RADAR_RANGE;
h = 3 * s;
a += a * h;
glColor4f (r + r * h, g + g * h, b + b * h, (float) sqrt (a));
h = radarSizes [gameOpts->render.cockpit.nRadarSize] * 0.0875f;
h += h * s;
glTranslatef (v [0].v.coord.x, v [0].v.coord.y, v [0].v.coord.z);
OglDrawEllipse (BLIP_SLICES, GL_POLYGON, h, 0, h, 0, sinCosBlip);
glTranslatef (-v [0].v.coord.x, -v [0].v.coord.y, -v [0].v.coord.z);

ogl.VertexBuffer () [0] = v [0];
ogl.VertexBuffer () [1] = v [1];
ogl.FlushBuffers (GL_LINES, 2);
}

// -----------------------------------------------------------------------------------

void CRadar::RenderObjects (int bAbove)
{
	CObject*			objP;
	CFloatVector3*	colorP = radarColor + gameOpts->render.automap.nColor;

// glPushMatrix ();
glLineWidth (GLfloat (2 + gameOpts->render.cockpit.nRadarSize));
FORALL_OBJS (objP, i) {
	if ((objP->info.nType == OBJ_PLAYER) && (objP != gameData.objs.consoleP)) {
		if (AM_SHOW_PLAYERS && AM_SHOW_PLAYER (objP->info.nId)) {
			colorP = shipColors + (IsTeamGame ? GetTeam (objP->info.nId) : objP->info.nId);
			RenderBlip (objP, colorP->Red (), colorP->Green (), colorP->Blue (), 0.9f / 4, bAbove);
			}
		}
	else if (objP->info.nType == OBJ_ROBOT) {
		if (AM_SHOW_ROBOTS) {
			if (ROBOTINFO (objP->info.nId).companion)
				RenderBlip (objP, guidebotColor.Red (), guidebotColor.Green (), guidebotColor.Blue (), 0.9f * 0.25f, bAbove);
			else
				RenderBlip (objP, robotColor.Red (), robotColor.Green (), robotColor.Blue (), 0.9f * 0.25f, bAbove);
			}
		}
	else if (objP->info.nType == OBJ_POWERUP) {
		if (AM_SHOW_POWERUPS (2))
			RenderBlip (objP, powerupColor.Red (), powerupColor.Green (), powerupColor.Blue (), 0.9f * 0.25f, bAbove);
		}
	}
//transformation.End ();
//ogl.SetTransform (0);
// glPopMatrix ();
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
if (gameStates.render.bChaseCam)
	return;
if (gameStates.app.bPlayerIsDead)
	return;
extraGameInfo [0].nRadar = (gameOpts->render.cockpit.nRadarRange > 0);
if (!EGI_FLAG (nRadar, 0, 1, 0))
	return;
if (gameStates.zoom.nFactor > gameStates.zoom.nMinFactor)
	return;
if (!ogl.SizeVertexBuffer (RADAR_SLICES))
	return;

int bStencil = ogl.StencilOff ();

m_offset.v.coord.x = 0.0f;
m_offset.v.coord.y = yOffs [gameOpts->render.cockpit.nRadarPos][gameStates.render.cockpit.nType];
m_offset.v.coord.y += sizeOffsets [gameOpts->render.cockpit.nRadarPos][gameOpts->render.cockpit.nRadarSize] * (m_offset.v.coord.y / fabs (m_offset.v.coord.y));
if (EGI_FLAG (nWeaponIcons, 1, 1, 0) && (gameOpts->render.cockpit.bHUD || cockpit->ShowAlways ())) {
	if ((extraGameInfo [0].nWeaponIcons < 3) || ((extraGameInfo [0].nWeaponIcons & 1) == !gameOpts->render.cockpit.nRadarPos))
		m_offset.v.coord.y += (m_offset.v.coord.y > 0) ? -3.75f : 3.25f;
	}
m_offset.v.coord.z = 50.0f;
m_radius = radarSizes [gameOpts->render.cockpit.nRadarSize] / transformation.m_info.scalef.v.coord.x;
m_lineWidth = float (CCanvas::Current ()->Width ()) / 640.0f * radarSizes [gameOpts->render.cockpit.nRadarSize] / radarSizes [sizeofa (radarSizes) - 1];

ogl.ResetClientStates (0);
ogl.EnableClientStates (0, 0, 0, GL_TEXTURE0);
ogl.SetTexturing (false);
ogl.SetFaceCulling (false);
ogl.SetBlending (1);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
ogl.SetLineSmooth (true);
glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
glLineWidth (m_lineWidth);
#if 0
// render the radar to a texture and render that texture to the HUD (unfinished)
if (ogl.m_features.bRenderToTexture.Available ()) {
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
	// glPushMatrix ();
	glLoadIdentity ();//clear matrix
	glMatrixMode (GL_PROJECTION);
	// glPushMatrix ();
	glLoadIdentity ();//clear matrix
	glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
	ogl.SetViewport (0, 0, screen.Width (), screen.Height ());

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
	// glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);
	// glPopMatrix ();

#endif
	}
else 
#endif
	{
	ogl.SetDepthTest (false);
	ComputeCenter ();
	RenderBackground ();
	RenderObjects (!gameOpts->render.cockpit.nRadarPos);
	RenderDevice ();
	RenderObjects (gameOpts->render.cockpit.nRadarPos);
	ogl.SetDepthTest (true);
	}
ogl.SetLineSmooth (false);
glLineWidth (1);
ogl.SetFaceCulling (true);
ogl.StencilOn (bStencil);
}

//------------------------------------------------------------------------------
// eof
