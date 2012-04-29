/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "ogl_color.h"
#include "rendermine.h"
#include "transprender.h"
#include "key.h"
#include "screens.h"
#include "mouse.h"
#include "timer.h"
#include "segpoint.h"
#include "joy.h"
#include "iff.h"
#include "pcx.h"
#include "gamefont.h"
#include "kconfig.h"
#include "endlevel.h"
#include "text.h"
#include "cockpit.h"
#include "segmath.h"
#include "gamecntl.h"
#include "input.h"
#include "slowmotion.h"
#include "marker.h"
#include "songs.h"
#include "menubackground.h"
#include "systemkeys.h"
#include "menu.h"
#include "renderframe.h"
#include "automap.h"
#include "ogl_shader.h"
#include "fastrender.h"

#ifndef Pi
#	define Pi 3.141592653589793240
#endif

//------------------------------------------------------------------------------

#define RESCALE_X(x) ((x) * m_nWidth / 640)
#define RESCALE_Y(y) ((y) * m_nHeight / 480)

#define EF_USED     1   // This edge is used
#define EF_DEFINING 2   // A structure defining edge that should always draw.
#define EF_FRONTIER 4   // An edge between the known and the unknown.
#define EF_SECRET   8   // An edge that is part of a secret CWall.
#define EF_GRATE    16  // A bGrate... draw it all the time.
#define EF_NO_FADE  32  // An edge that doesn't fade with distance
#define EF_TOO_FAR  64  // An edge that is too far away

#define K_WALL_NORMAL_COLOR     RGBA_PAL2 (29, 29, 29)
#define K_WALL_DOOR_COLOR       RGBA_PAL2 (5, 27, 5)
#define K_WALL_DOOR_BLUE        RGBA_PAL2 (0, 0, 31)
#define K_WALL_DOOR_GOLD        RGBA_PAL2 (31, 31, 0)
#define K_WALL_DOOR_RED         RGBA_PAL2 (31, 0, 0)
#define K_WALL_REVEALED_COLOR   RGBA_PAL2 (0, 0, 25) //what you see when you have the full map powerup
#define K_HOSTAGE_COLOR         RGBA_PAL2 (0, 31, 0)
#define K_MONSTERBALL_COLOR     RGBA_PAL2 (31, 23, 0)

// Map movement defines
#define PITCH_DEFAULT			9000
#define ZOOM_DEFAULT				I2X (20*10)
#define ZOOM_MIN_VALUE			I2X (20*5)
#define ZOOM_MAX_VALUE			I2X (20*100)

#define SLIDE_SPEED 				(350)
#define ZOOM_SPEED_FACTOR		500	// (1500)
#define ROT_SPEED_DIVISOR		(115000)

#define LEAVE_TIME				0x4000

#define EDGE_IDX(_edgeP)		((int) ((_edgeP) - m_edges.Buffer ()))

CAutomap automap;

//------------------------------------------------------------------------------

void CAutomap::InitColors (void)
{
m_colors.walls.nNormal = K_WALL_NORMAL_COLOR;
m_colors.walls.nDoor = K_WALL_DOOR_COLOR;
m_colors.walls.nDoorBlue = K_WALL_DOOR_BLUE;
m_colors.walls.nDoorGold = K_WALL_DOOR_GOLD;
m_colors.walls.nDoorRed = K_WALL_DOOR_RED;
m_colors.walls.nRevealed = K_WALL_REVEALED_COLOR;
m_colors.nHostage = K_HOSTAGE_COLOR;
m_colors.nMonsterball = K_MONSTERBALL_COLOR;
m_colors.nDkGray = RGBA_PAL2 (20,20,20);
m_colors.nMedGreen = RGBA_PAL2 (0,31,0);
m_colors.nWhite = RGBA_PAL2 (63,63,63);
m_colors.nLgtBlue = RGBA_PAL2 (0,0,48);
m_colors.nLgtRed = RGBA_PAL2 (48,0,0);
}

//------------------------------------------------------------------------------

bool CAutomap::InitBackground (void)
{
//m_background.Init ();
if (m_background.Buffer ())
	return true;

int nPCXError = PCXReadBitmap (BackgroundName (BG_MAP), &m_background, BM_LINEAR, 0);
if (nPCXError != PCX_ERROR_NONE) {
	Error ("File %s - PCX error: %s", BackgroundName (BG_MAP), PcxErrorMsg (nPCXError));
	return false;
	}
m_background.SetPalette (NULL, -1, -1);
return (m_background.Buffer () != NULL);
}

//------------------------------------------------------------------------------

void CAutomap::Init (void)
{
m_nWidth = 640;
m_nHeight = 480;
m_bFull = false;
m_bDisplay = 0;
m_data.bCheat = 0;
m_data.bHires = 1;
m_data.nViewDist = 0;
m_data.nMaxDist = I2X (2000);
m_data.nZoom = 0x9000;
m_data.viewer.vPos.SetZero ();
m_data.viewTarget.SetZero ();
m_data.viewer.mOrient = CFixMatrix::IDENTITY;
if (!m_visited.Buffer ())
	m_visited.Create (MAX_SEGMENTS_D2X);
m_visited.Clear ();
if (!m_visible.Buffer ())
	m_visible.Create (MAX_SEGMENTS_D2X);
m_visible.Clear ();
m_nEdges = 0;
m_nMaxEdges = MAX_EDGES;
m_nLastEdge = -1;
m_edges.Create (MAX_EDGES);
m_brightEdges.Create (MAX_EDGES);
InitColors ();
}

//------------------------------------------------------------------------------

void CAutomap::ClearVisited ()
{
m_visited.Clear ();
markerManager.Clear ();
}

//------------------------------------------------------------------------------

void CAutomap::DrawPlayer (CObject* objP)
{
	CFixVector	vArrowPos, vHeadPos;
	CRenderPoint		spherePoint, arrowPoint, headPoint;
	int			size = objP->info.xSize * (m_bRadar ? 2 : 1);
//	int			bUseTransform = ogl.m_states.bUseTransform;

spherePoint.SetIndex (-1);
// Draw Console CPlayerData -- shaped like a ellipse with an arrow.
//spherePoint.m_vertex [1].SetZero ();
spherePoint.TransformAndEncode (objP->info.position.vPos);
//transformation.Rotate (&spherePoint.m_vertex [1], &objP->info.position.vPos, 0);
G3DrawSphere (&spherePoint, m_bRadar ? objP->info.xSize * 2 : objP->info.xSize, !m_bRadar);

if (m_bRadar && (objP->Index () != LOCALPLAYER.nObject))
	return;
if (ogl.SizeVertexBuffer (3)) {
	headPoint.SetIndex (-1);
	arrowPoint.SetIndex (-1);
	ogl.VertexBuffer () [1].Assign (spherePoint.ViewPos ());
	// Draw CPlayerData's up vector
	vArrowPos = objP->info.position.vPos + objP->info.position.mOrient.m.dir.u * (size*2);
	arrowPoint.TransformAndEncode (vArrowPos);
	ogl.VertexBuffer () [0].Assign (arrowPoint.ViewPos ());
	// Draw shaft of arrow
	vArrowPos = objP->info.position.vPos + objP->info.position.mOrient.m.dir.f * (size * 3);
	arrowPoint.TransformAndEncode (vArrowPos);
	ogl.VertexBuffer () [2].Assign (arrowPoint.ViewPos ());
	ogl.FlushBuffers (GL_LINE_STRIP, 3);
	ogl.VertexBuffer () [1].Assign (arrowPoint.ViewPos ());
	// Draw right head of arrow
	vHeadPos = objP->info.position.vPos + objP->info.position.mOrient.m.dir.f * (size*2);
	vHeadPos += objP->info.position.mOrient.m.dir.r * (size*1);
	headPoint.TransformAndEncode (vHeadPos);
	ogl.VertexBuffer () [0].Assign (headPoint.ViewPos ());
	// Draw left head of arrow
	vHeadPos = objP->info.position.vPos + objP->info.position.mOrient.m.dir.f * (size*2);
	vHeadPos += objP->info.position.mOrient.m.dir.r * (size* (-1));
	headPoint.TransformAndEncode (vHeadPos);
	ogl.VertexBuffer () [2].Assign (headPoint.ViewPos ());
	ogl.FlushBuffers (GL_LINE_STRIP, 3);
	}
}

//------------------------------------------------------------------------------

void CAutomap::DrawObjects (void)
{
if (!((gameOpts->render.automap.bTextured & 2) || m_bRadar))
	return;
int color = IsTeamGame ? GetTeam (N_LOCALPLAYER) : N_LOCALPLAYER % MAX_PLAYER_COLORS;	// Note link to above if!
CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (playerColors [color].r, playerColors [color].g, playerColors [color].b));
int bTextured = (gameOpts->render.automap.bTextured & 1) && !m_bRadar;
ogl.SetFaceCulling (false);
ogl.SetBlending (true);
gameStates.render.grAlpha = gameStates.app.bNostalgia ? 1.0f : bTextured ? 0.5f : 0.9f;
ogl.SetTexturing (false);
glLineWidth (2 * GLfloat (screen.Width ()) / 640.0f);
DrawPlayer (OBJECTS + LOCALPLAYER.nObject);
if (!m_bRadar) {
	markerManager.Render ();
	if ((markerManager.Highlight () > -1) && (markerManager.Message () [0] != 0)) {
		char msg [10 + MARKER_MESSAGE_LEN + 1];
		sprintf (msg, TXT_MARKER_MSG, markerManager.Highlight () + 1,
					markerManager.Message (N_LOCALPLAYER * 2 + markerManager.Highlight ()));
		CCanvas::Current ()->SetColorRGB (196, 0, 0, 255);
		fontManager.SetCurrent (SMALL_FONT);
		GrString (5, 20, msg, NULL);
		}
	}
// Draw player(s)...
if (AM_SHOW_PLAYERS) {
	for (int i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if ((i != N_LOCALPLAYER) && AM_SHOW_PLAYER (i)) {
			if (OBJECTS [gameData.multiplayer.players [i].nObject].info.nType == OBJ_PLAYER) {
				color = IsTeamGame ? GetTeam (i) : i;
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (playerColors [color].r, playerColors [color].g, playerColors [color].b));
				if (bTextured)
					ogl.SetBlending (true);
				DrawPlayer (OBJECTS + gameData.multiplayer.players [i].nObject);
				}
			}
		}
	}

if (bTextured)
	ogl.SetBlending (true);

CObject* objP = OBJECTS.Buffer ();
CRenderPoint	spherePoint;

FORALL_OBJS (objP, i) {
	int size = objP->info.xSize;
	switch (objP->info.nType) {
		case OBJ_HOSTAGE:
			CCanvas::Current ()->SetColorRGBi (m_colors.nHostage);
			spherePoint.TransformAndEncode (objP->info.position.vPos);
			G3DrawSphere (&spherePoint, size, !m_bRadar);
			break;

		case OBJ_MONSTERBALL:
			CCanvas::Current ()->SetColorRGBi (m_colors.nMonsterball);
			spherePoint.TransformAndEncode (objP->info.position.vPos);
			G3DrawSphere (&spherePoint,size, !m_bRadar);
			break;

		case OBJ_ROBOT:
			if (AM_SHOW_ROBOTS && ((gameStates.render.bAllVisited && bTextured) || m_visited [objP->info.nSegment])) {
				static int t = 0;
				static int d = 1;
				int h = SDL_GetTicks ();
				if (h - t > 333) {
					t = h;
					d = -d;
					}
				float fScale = float (h - t) / 333.0f;
				if (ROBOTINFO (objP->info.nId).companion) {
					if (d < 0)
						CCanvas::Current ()->SetColorRGB (0, 123 - int ((123 - 78) * fScale + 0.5f), 151 - int ((151 - 112) * fScale + 0.5f), 255);
					else
						CCanvas::Current ()->SetColorRGB (0, 78 + int ((123 - 78) * fScale + 0.5f), 122 + int ((151 - 112) * fScale + 0.5f), 255);
					}
				else {
					if (d < 0)
						CCanvas::Current ()->SetColorRGB (123 - int ((123 - 78) * fScale + 0.5f), 0, 135 - int ((135 - 96) * fScale + 0.5f), 255);
					else
						CCanvas::Current ()->SetColorRGB (78 + int ((123 - 78) * fScale + 0.5f), 0, 96 + int ((135 - 96) * fScale + 0.5f), 255);
					}
				spherePoint.TransformAndEncode (objP->info.position.vPos);
				//transformation.Begin (&objP->info.position.vPos, &objP->info.position.mOrient);
				G3DrawSphere (&spherePoint, bTextured ? size : (size * 3) / 2, !m_bRadar);
				//transformation.End ();
				}
			break;

		case OBJ_POWERUP:
#if DBG
			if (objP->info.nSegment == nDbgSeg)
				nDbgSeg = nDbgSeg;
#endif
			switch (objP->info.nId) {
				case POW_KEY_RED:
					CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (63, 5, 5));
					size *= 4;
					break;
				case POW_KEY_BLUE:
					CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (5, 5, 63));
					size *= 4;
					break;
				case POW_KEY_GOLD:
					CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (63, 63, 10));
					size *= 4;
					break;
				default:
					if (!(AM_SHOW_POWERUPS (1) && (gameStates.render.bAllVisited || m_visited [objP->info.nSegment])))
						continue;
					CCanvas::Current ()->SetColorRGBi (ORANGE_RGBA); //orange
					break;
				}
#if DBG
			if (objP->info.nSegment == nDbgSeg)
				nDbgSeg = nDbgSeg;
#endif
			spherePoint.TransformAndEncode (objP->info.position.vPos);
			G3DrawSphere (&spherePoint, size, !m_bRadar);
			break;
		}
	}
gameStates.render.grAlpha = 1.0f;
ogl.SetFaceCulling (true);
glLineWidth (1);
}

//------------------------------------------------------------------------------

void CAutomap::DrawLevelId (void)
{
if (gameStates.app.bNostalgia)
	return;
if (gameStates.app.bSaveScreenShot && cockpit->Hide ())
	return;

	CFont*	curFont = CCanvas::Current ()->Font ();
	int		w, h, aw, offs = m_data.bHires ? 10 : 5;
	char		szInfo [3][200];

#if 0
if (gameOpts->app.bExpertMode) {
	if (CCanvas::Current ()->Width () >= 1024) {
		*szInfo [2] = '\0';
		strcpy (szInfo [1], "ALT+A: Sparks   ALT+B: Bright   ALT+C: Coronas   ALT+G: Grayout ALT+L: Lightnings");
		strcpy (szInfo [0], "ALT+P: Powerups   ALT+R: Robots   ALT+S: Smoke   ALT+T: Textures/Wireframe   CTRL+T: Teleport");
		}
	else {
		strcpy (szInfo [2], "ALT+A: Sparks   ALT+B: Bright   ALT+C: Coronas   ALT+G: Grayout");
		strcpy (szInfo [1], "ALT+L: Lightnings   ALT+P: Powerups   ALT+R: Robots");
		strcpy (szInfo [0], "ALT+S: Smoke   ALT+T: Textures/wireframe   CTRL+T: Teleport");
		}
	}
else
#endif
	{
	if (CCanvas::Current ()->Width () >= 1024) {
		*szInfo [2] =
		*szInfo [1] = '\0';
		strcpy (szInfo [0], "F1: Textures/Wireframe   F2: Color   F3: Effects   F4: Objects   F9: Teleport");
		}
	else {
		*szInfo [2] = '\0';
		strcpy (szInfo [1], "F1: Textures/Wireframe   F2: Color");
		strcpy (szInfo [0], "F3: Effects   F4: Objects   F9: Teleport");
		}
	}
fontManager.SetCurrent (SMALL_FONT);
if ((gameStates.render.cockpit.nType == CM_FULL_SCREEN) || (gameStates.render.cockpit.nType == CM_LETTERBOX)) {
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	}
else {
	fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	offs /= 2;
	}
for (int i = 0; (i < 3) && *szInfo [i]; i++) {
	fontManager.Current ()->StringSize (szInfo [i], w, h, aw);
	GrPrintF (NULL, (CCanvas::Current ()->Width () - w) / 2, CCanvas::Current ()->Height () - offs - (i + 1) * h - i * 2, szInfo [i]);
	}
fontManager.SetCurrent (SMALL_FONT);
fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
offs = m_data.bHires ? 10 : 5;
if (gameOpts->render.cockpit.bHUD) {
	GrPrintF (NULL, offs, offs, m_szLevelNum);
	fontManager.Current ()->StringSize (m_szLevelName, w, h, aw);
	GrPrintF (NULL, CCanvas::Current ()->Width () - offs - w, offs, m_szLevelName);
	fontManager.SetCurrent (curFont);
	if (gameOpts->render.automap.bTextured & 1)
		cockpit->DrawFrameRate ();
	}
}

//------------------------------------------------------------------------------

void CAutomap::Render (fix xStereoSeparation)
{
#if 1
PROF_START
	int	bAutomapFrame = !m_bRadar &&
								 (gameStates.render.cockpit.nType != CM_FULL_SCREEN) &&
								 (gameStates.render.cockpit.nType != CM_LETTERBOX);
	CFixMatrix	mRadar;

automap.m_bFull = (LOCALPLAYER.flags & (PLAYER_FLAGS_FULLMAP_CHEAT | PLAYER_FLAGS_FULLMAP)) != 0;
if ((m_bRadar = m_bRadar) == 2) {
	CFixMatrix& po = gameData.multiplayer.playerInit [N_LOCALPLAYER].position.mOrient;
#if 1
	mRadar.m.dir.r = po.m.dir.r;
	mRadar.m.dir.f = po.m.dir.u;
	mRadar.m.dir.f.v.coord.y = -mRadar.m.dir.f.v.coord.y;
	mRadar.m.dir.u = po.m.dir.f;
#else
	mRadar.rVec.p.x = po->rVec.p.x;
	mRadar.rVec.p.y = po->rVec.p.y;
	mRadar.rVec.p.z = po->rVec.p.z;
	mRadar.fVec.p.x = po->uVec.p.x;
	mRadar.fVec.p.y = -po->uVec.p.y;
	mRadar.fVec.p.z = po->uVec.p.z;
	mRadar.uVec.p.x = po->fVec.p.x;
	mRadar.uVec.p.y = po->fVec.p.y;
	mRadar.uVec.p.z = po->fVec.p.z;
#endif
	}
if (m_bRadar)
	CCanvas::Current ()->Clear (RGBA_PAL2 (0,0,0));
#if 0
if (bAutomapFrame) {
	if (InitBackground ())
		m_background.RenderFullScreen ();
	fontManager.SetCurrent (HUGE_FONT);
	fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	GrPrintF (NULL, RESCALE_X (80), RESCALE_Y (36), TXT_AUTOMAP, HUGE_FONT);
	fontManager.SetCurrent (SMALL_FONT);
	fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	GrPrintF (NULL, RESCALE_X (60), RESCALE_Y (426), TXT_TURN_SHIP);
	GrPrintF (NULL, RESCALE_X (60), RESCALE_Y (443), TXT_SLIDE_UPDOWN);
	GrPrintF (NULL, RESCALE_X (60), RESCALE_Y (460), TXT_VIEWING_DISTANCE);
	//GrUpdate (0);
	}
#endif
if (!gameOpts->render.automap.bTextured || gameStates.app.bNostalgia)
	gameOpts->render.automap.bTextured = 1;
G3StartFrame (transformation, m_bRadar /*|| !(gameOpts->render.automap.bTextured & 1)*/, !m_bRadar, xStereoSeparation);
ogl.ResetClientStates ();
shaderManager.Deploy (-1);

if (bAutomapFrame)
	ogl.Viewport (RESCALE_X (27), RESCALE_Y (80), RESCALE_X (582), RESCALE_Y (334));
RenderStartFrame ();
if (m_bRadar == 2) {
	m_data.viewer.vPos = m_data.viewTarget + mRadar.m.dir.f * (-m_data.nViewDist);
	SetupTransformation (transformation, m_data.viewer.vPos, mRadar, m_data.nZoom * 2, 1);
	}
else {
	m_data.viewer.vPos = m_data.viewTarget + m_data.viewer.mOrient.m.dir.f * -m_data.nViewDist;
	if (!m_bRadar && xStereoSeparation) {
		//glClear (GL_COLOR_BUFFER_BIT);
		m_data.viewer.vPos += m_data.viewer.mOrient.m.dir.r * xStereoSeparation;
		}
	SetupTransformation (transformation, m_data.viewer.vPos, m_data.viewer.mOrient, m_bRadar ? (m_data.nZoom * 3) / 2 : m_data.nZoom, 1);
	}
UpdateSlidingFaces ();
if (!m_bRadar && (gameStates.app.bNostalgia < 2) && (gameOpts->render.automap.bTextured & 1)) {
	gameData.render.mine.viewer = m_data.viewer;
	RenderMine (gameData.objs.consoleP->info.nSegment, 0, 0);
	RenderEffects (0);
	}
if (m_bRadar || (gameOpts->render.automap.bTextured & 2)) {
	DrawEdges ();
	DrawObjects ();
	}
G3EndFrame (transformation, 0);

if (m_bRadar) {
	ogl.m_states.bEnableScissor = 0;
	return;
	}
gameData.app.nFrameCount++;
if (bAutomapFrame) {
	if (InitBackground ()) {
		m_background.SetTranspType (2);
		m_background.AddFlags (BM_FLAG_TRANSPARENT);
		m_background.RenderFullScreen ();
		}
	fontManager.SetCurrent (HUGE_FONT);
	fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	GrPrintF (NULL, RESCALE_X (80), RESCALE_Y (36), TXT_AUTOMAP, HUGE_FONT);
	fontManager.SetCurrent (SMALL_FONT);
	fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	GrPrintF (NULL, RESCALE_X (60), RESCALE_Y (426), TXT_TURN_SHIP);
	GrPrintF (NULL, RESCALE_X (60), RESCALE_Y (443), TXT_SLIDE_UPDOWN);
	GrPrintF (NULL, RESCALE_X (60), RESCALE_Y (460), TXT_VIEWING_DISTANCE);
	//GrUpdate (0);
	}
DrawLevelId ();
PROF_END(ptRenderFrame)
#endif
FlushFrame (xStereoSeparation);
if (gameStates.app.bSaveScreenShot)
	SaveScreenShot (NULL, 1);
}

//------------------------------------------------------------------------------
//name for each group.  maybe move somewhere else
const char *pszSystemNames [] = {
	"Zeta Aquilae",
	"Quartzon System",
	"Brimspark System",
	"Limefrost Spiral",
	"Baloris Prime",
	"Omega System"};

void CAutomap::CreateNameCanvas (void)
{
	char	szExplored [100];
	int	h, i;

if (missionManager.nCurrentLevel > 0)
	sprintf (m_szLevelNum, "%s %i",TXT_LEVEL, missionManager.nCurrentLevel);
else
	sprintf (m_szLevelNum, "Secret Level %i", -missionManager.nCurrentLevel);
if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) && (missionManager.nCurrentLevel > 0))		//built-in mission
	sprintf (m_szLevelName,"%s %d: ",
				pszSystemNames [(missionManager.nCurrentLevel - 1) / 4],
				((missionManager.nCurrentLevel - 1) % 4) + 1);
else
	strcpy (m_szLevelName, " ");
strcat (m_szLevelName, missionManager.szCurrentLevel);
for (h = i = 0; i < gameData.segs.nSegments; i++)
	if (m_visited [i])
		h++;
sprintf (szExplored, " (%1.1f %s)", (float) (h * 100) / (float) gameData.segs.nSegments, TXT_PERCENT_EXPLORED);
strcat (m_szLevelName, szExplored);
}

//	-----------------------------------------------------------------------------
//	Set the segment depth of all segments from nStartSeg in *segbuf.
//	Returns maximum nDepth value.
int CAutomap::SetSegmentDepths (int nStartSeg, ushort *depthBufP)
{
	ubyte		bVisited [MAX_SEGMENTS_D2X];
	short		queue [MAX_SEGMENTS_D2X];
	int		head = 0;
	int		tail = 0;
	int		nDepth = 1;
	int		nSegment, nSide, nChild;
	ushort	nParentDepth = 0;
	short*	childP;

	head = 0;
	tail = 0;

if ((nStartSeg < 0) || (nStartSeg >= gameData.segs.nSegments))
	return 1;
if (depthBufP [nStartSeg] == 0)
	return 1;
queue [tail++] = nStartSeg;
memset (bVisited, 0, sizeof (*bVisited) * gameData.segs.nSegments);
bVisited [nStartSeg] = 1;
depthBufP [nStartSeg] = nDepth++;
if (nDepth == 0)
	nDepth = 0x7fff;
while (head < tail) {
	nSegment = queue [head++];
#if DBG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	nParentDepth = depthBufP [nSegment];
	childP = SEGMENTS [nSegment].m_children;
	for (nSide = SEGMENT_SIDE_COUNT; nSide; nSide--, childP++) {
		if (0 > (nChild = *childP))
			continue;
#if DBG
		if (nChild >= gameData.segs.nSegments) {
			Error ("Invalid segment in SetSegmentDepths()\nsegment=%d, side=%d, child=%d",
					 nSegment, nSide, nChild);
			return 1;
			}
#endif
#if DBG
		if (nChild == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		if (!depthBufP [nChild])
			continue;
		if (bVisited [nChild])
			continue;
		bVisited [nChild] = 1;
		depthBufP [nChild] = nParentDepth + 1;
		queue [tail++] = nChild;
		}
	}
return (nParentDepth + 1) * gameStates.render.bViewDist;
}

//------------------------------------------------------------------------------

int CAutomap::Setup (int bPauseGame, fix& xEntryTime)
{
	int		i;
	fix		t1, t2;
	CObject	*playerP;

if (m_bDisplay < 0) {
	m_bDisplay = 0;
	if ((m_bChaseCam = gameStates.render.bChaseCam))
		SetChaseCam (0);
	if ((m_bFreeCam = (gameStates.render.bFreeCam > 0)))
		SetFreeCam (0);

	ogl.m_states.nContrast = 8;
	InitColors ();
	if (!m_bRadar)
		SlowMotionOff ();
	if (m_bRadar ||
		 (IsMultiGame &&
		  (gameStates.app.nFunctionMode == FMODE_GAME) &&
		  (!gameStates.app.bEndLevelSequence)))
		bPauseGame = 0;
	if (bPauseGame)
		PauseGame ();
	if (m_bRadar || (gameStates.video.nDisplayMode > 1)) {
		//GrSetMode (gameStates.video.nLastScreenMode);
		if (m_bRadar) {
			m_nWidth = CCanvas::Current ()->Width ();
			m_nHeight = CCanvas::Current ()->Height ();
			}
		else {
			m_nWidth = screen.Canvas ()->Width ();
			m_nHeight = screen.Canvas ()->Height ();
			}
		m_data.bHires = 1;
		 }
	else {
		GrSetMode (SM (320, 400));
		m_data.bHires = 0;
		}
	gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && m_data.bHires;
	if (!m_bRadar) {
		CreateNameCanvas ();
		paletteManager.ResetEffect ();
		}
	if (m_bRadar)
		m_data.nViewDist = ZOOM_DEFAULT;
	else if (!m_data.nViewDist)
		m_data.nViewDist = ZOOM_DEFAULT;
	playerP = OBJECTS + LOCALPLAYER.nObject;
	m_data.viewer.mOrient = playerP->info.position.mOrient;

	m_vTAngles.v.coord.p = PITCH_DEFAULT;
	m_vTAngles.v.coord.h = 0;
	m_vTAngles.v.coord.b = 0;

	m_data.viewTarget = playerP->info.position.vPos;
	t1 = xEntryTime = TimerGetFixedSeconds ();
	t2 = t1;
	}
BuildEdgeList ();

if (m_bRadar) {
	for (i = 0; i < gameData.segs.nSegments; i++)
		m_visible [i] = 1;
	}
else if (m_bFull) {
	for (i = 0; i < gameData.segs.nSegments; i++)
		m_visible [i] = 1;
	}
else
	memcpy (m_visible.Buffer (), m_visited.Buffer (), m_visited.Size ());
//m_visited [OBJECTS [LOCALPLAYER.nObject].nSegment] = 1;
m_nSegmentLimit =
m_nMaxSegsAway = SetSegmentDepths (OBJECTS [LOCALPLAYER.nObject].info.nSegment, m_visible.Buffer ());
AdjustSegmentLimit ();
m_bDisplay++;
return gameData.app.bGamePaused;
}

//------------------------------------------------------------------------------

int CAutomap::Update (void)
{
	CObject*		playerP = OBJECTS + LOCALPLAYER.nObject;
	CFixMatrix	m;

if (controls [0].firePrimaryDownCount) {
	// Reset orientation
	m_data.nViewDist = ZOOM_DEFAULT;
	m_vTAngles.v.coord.p = PITCH_DEFAULT;
	m_vTAngles.v.coord.h = 0;
	m_vTAngles.v.coord.b = 0;
	m_data.viewTarget = playerP->info.position.vPos;
	}
if (controls [0].forwardThrustTime)
	m_data.viewTarget += m_data.viewer.mOrient.m.dir.f * (controls [0].forwardThrustTime * ZOOM_SPEED_FACTOR);
m_vTAngles.v.coord.p += (fixang) FixDiv (controls [0].pitchTime, ROT_SPEED_DIVISOR);
m_vTAngles.v.coord.h += (fixang) FixDiv (controls [0].headingTime, ROT_SPEED_DIVISOR);
m_vTAngles.v.coord.b += (fixang) FixDiv (controls [0].bankTime, ROT_SPEED_DIVISOR*2);

m = CFixMatrix::Create (m_vTAngles);
if (controls [0].verticalThrustTime || controls [0].sidewaysThrustTime) {
	m_data.viewer.mOrient = playerP->info.position.mOrient * m;
	m_data.viewTarget += m_data.viewer.mOrient.m.dir.u * (controls [0].verticalThrustTime * SLIDE_SPEED);
	m_data.viewTarget += m_data.viewer.mOrient.m.dir.r * (controls [0].sidewaysThrustTime * SLIDE_SPEED);
	}
m_data.viewer.mOrient = playerP->info.position.mOrient * m;
if (m_data.nViewDist < ZOOM_MIN_VALUE)
	m_data.nViewDist = ZOOM_MIN_VALUE;
if (m_data.nViewDist > ZOOM_MAX_VALUE)
	m_data.nViewDist = ZOOM_MAX_VALUE;
return 1;
}

//------------------------------------------------------------------------------

static inline int ViewDistStep (void)
{
	int h = (automap.SegmentLimit () + 5) / 10;

return h ? h : 1;
}

//------------------------------------------------------------------------------

int CAutomap::ReadControls (int nLeaveMode, int bDone, int& bPauseGame)
{
	int	c, nMarker, nMaxDrop, nColor = gameOpts->render.automap.bBright | (gameOpts->render.automap.bGrayOut << 1);

controls.Read ();
if (controls [0].automapDownCount && !nLeaveMode)
	return 1;
while ((c = KeyInKey ())) {
	if (!gameOpts->menus.nStyle)
		MultiDoFrame();
		switch (c) {
#if DBG
		case KEY_BACKSPACE: Int3 ();
			break;
#endif
		case KEY_PAUSE:
			if (gameOpts->menus.nStyle && !IsMultiGame) {
				if (gameData.app.bGamePaused)
					ResumeGame ();
				else
					PauseGame ();
				}
			bPauseGame = gameData.app.bGamePaused;
			break;

		case KEY_PRINT_SCREEN: {
			if (m_data.bHires)
				CCanvas::SetCurrent (NULL);
			gameStates.app.bSaveScreenShot = 1;
			break;
			}

		case KEY_ESC:
			if (!nLeaveMode)
				bDone = 1;
			break;

#if DBG
		case KEYDBGGED+KEY_F: {
			int i;
			for (i = 0; i <= gameData.segs.nLastSegment; i++)
				automap.m_visible [i] = 1;
			BuildEdgeList ();
			m_nSegmentLimit =
			m_nMaxSegsAway = SetSegmentDepths (OBJECTS [LOCALPLAYER.nObject].info.nSegment, automap.m_visible.Buffer ());
			AdjustSegmentLimit ();
			}
			break;
#endif

		case KEY_MINUS:
		case KEY_PADMINUS:
			if (m_nSegmentLimit > 1)  {
				m_nSegmentLimit -= ViewDistStep ();
				if (!m_nSegmentLimit)
					m_nSegmentLimit = 1;
				AdjustSegmentLimit ();
				}
			break;

		case KEY_EQUALS:
		case KEY_PADPLUS:
			if (m_nSegmentLimit < m_nMaxSegsAway) {
				m_nSegmentLimit += ViewDistStep ();
				if (m_nSegmentLimit > m_nMaxSegsAway)
					m_nSegmentLimit = m_nMaxSegsAway;
				AdjustSegmentLimit ();
				}
			break;

		case KEY_1:
		case KEY_2:
		case KEY_3:
		case KEY_4:
		case KEY_5:
		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
		case KEY_0:
			 nMaxDrop = markerManager.MaxDrop ();
			nMarker = c - KEY_1;
			if (nMarker <= nMaxDrop) {
				if (markerManager.Objects (nMarker) != -1)
					markerManager.SetHighlight (nMarker);
				}
		break;

		case KEY_CTRLED + KEY_D:
			markerManager.Delete (0);
			break;

		case KEY_CTRLED + KEY_R:
			if (!gameStates.app.bNostalgia)
				markerManager.Rotate ();
			break;

		case KEY_F1:
			if (!gameStates.app.bNostalgia) {
				if (gameOpts->render.automap.bTextured == 1)
					gameOpts->render.automap.bTextured = 3;
				else if (gameOpts->render.automap.bTextured == 3)
					gameOpts->render.automap.bTextured = 2;
				else
					gameOpts->render.automap.bTextured = 1;
				}
			break;

		case KEY_F2:
			if (!gameStates.app.bNostalgia) {
				if (gameOpts->render.automap.bTextured & 1) {
					nColor = (nColor + 1) % 3;
					gameOpts->render.automap.bBright = (nColor & 1) != 0;
					gameOpts->render.automap.bGrayOut = (nColor & 2) != 0;
					}
				}
			break;

		case KEY_F3:
			if (!gameStates.app.bNostalgia) {
				gameOpts->render.automap.bCoronas =
				gameOpts->render.automap.bLightning =
				gameOpts->render.automap.bParticles =
					!(gameOpts->render.automap.bCoronas ||
					  gameOpts->render.automap.bLightning ||
					  gameOpts->render.automap.bParticles);
				gameOpts->render.automap.bSparks =
					gameOpts->render.automap.bCoronas &&
					gameOpts->render.automap.bLightning &&
					gameOpts->render.automap.bParticles &&
					(gameOptions [0].render.nQuality > 0);
				}
			break;

		case KEY_F4:
			if (!gameStates.app.bNostalgia) {
				extraGameInfo [IsMultiGame].bPowerupsOnRadar =
				extraGameInfo [IsMultiGame].bRobotsOnRadar =
					!(extraGameInfo [IsMultiGame].bPowerupsOnRadar || extraGameInfo [IsMultiGame].bRobotsOnRadar);
				}
			break;

		case KEY_F9:
			if (!gameStates.app.bNostalgia && missionConfig.m_bTeleport)
				markerManager.Teleport ();
			break;

		case KEY_ALTED + KEY_F:
			gameStates.render.bShowFrameRate = (gameStates.render.bShowFrameRate + 1) % (6 + (gameStates.render.bPerPixelLighting == 2));
			break;

#if PROFILING
		case KEY_ALTED + KEY_Z:
			gameStates.render.bShowProfiler = !gameStates.render.bShowProfiler;
			break;
#endif

#if 0
		case KEY_ALTED + KEY_A:
			gameOpts->render.automap.bSparks = !gameOpts->render.automap.bSparks;
			break;

		case KEY_ALTED + KEY_B:
			if (gameOpts->render.automap.bTextured & 1)
				gameOpts->render.automap.bBright = !gameOpts->render.automap.bBright;
			break;

		case KEY_ALTED + KEY_C:
			gameOpts->render.automap.bCoronas = !gameOpts->render.automap.bCoronas;
			break;

		case KEY_ALTED + KEY_G:
			gameOpts->render.automap.bGrayOut = !gameOpts->render.automap.bGrayOut;
			break;

		case KEY_ALTED + KEY_L:
			gameOpts->render.automap.bLightning = !gameOpts->render.automap.bLightning;
			break;

		case KEY_ALTED + KEY_P:
			extraGameInfo [IsMultiGame].bPowerupsOnRadar = !extraGameInfo [IsMultiGame].bPowerupsOnRadar;
			break;

		case KEY_ALTED + KEY_R:
			extraGameInfo [IsMultiGame].bRobotsOnRadar = !extraGameInfo [IsMultiGame].bRobotsOnRadar;
			break;

		case KEY_ALTED + KEY_S:
			gameOpts->render.automap.bParticles = !gameOpts->render.automap.bParticles;
			break;
#endif

		case KEY_ALTED+KEY_ENTER:
		case KEY_ALTED+KEY_PADENTER:
			GrToggleFullScreenGame ();
			break;
		}
	}
return bDone;
}

//------------------------------------------------------------------------------

int CAutomap::GameFrame (int bPauseGame, int bDone)
{
	tControlInfo controlInfoSave;

if (!bPauseGame) {
	ushort bWiggleSave;
	controlInfoSave = controls [0];				// Save controls so we can zero them
	controls.Reset ();
	bWiggleSave = gameData.objs.consoleP->mType.physInfo.flags & PF_WIGGLE;	// Save old wiggle
	gameData.objs.consoleP->mType.physInfo.flags &= ~PF_WIGGLE;		// Turn off wiggle
	if (MultiMenuPoll ())
		bDone = 1;
	::GameFrame (0, -1);		// Do game loop with no rendering and no reading controls.
	gameData.objs.consoleP->mType.physInfo.flags |= bWiggleSave;	// Restore wiggle
	controls [0] = controlInfoSave;
	}
return bDone;
}

//------------------------------------------------------------------------------

void CAutomap::DoFrame (int nKeyCode, int bRadar)
{
	int				bDone = 0;
	int				nLeaveMode = 0;
	int				bFirstTime = 1;
	fix				xEntryTime;
	int				bPauseGame = (gameOpts->menus.nStyle == 0);		// Set to 1 if everything is paused during automap...No pause during net.
	fix				t1 = 0, t2 = 0;
	int				nContrast = ogl.m_states.nContrast;
	int				bRedrawScreen = 0;

	//static ubyte	automapPal [256*3];

m_nMaxSegsAway = 0;
m_nSegmentLimit = 1;
m_bRadar = bRadar;
bPauseGame = Setup (bPauseGame, xEntryTime);
bRedrawScreen = 0;
if (bRadar) {
	int bRenderToTexture = ogl.m_features.bRenderToTexture.Apply ();
	ogl.m_features.bRenderToTexture = 0;
	Render ();
	ogl.m_features.bRenderToTexture = bRenderToTexture;

	ogl.m_states.nContrast = nContrast;
	if (!--m_bDisplay) {
		if (m_bChaseCam)
			SetChaseCam (1);
		else if (m_bFreeCam)
			SetFreeCam (1);
		}
	return;
	}
controls [0].automapState = 0;
GetSlowTicks ();
do {
	PROF_START
	if (!nLeaveMode && controls [0].automapState && ((TimerGetFixedSeconds ()- xEntryTime) > LEAVE_TIME))
		nLeaveMode = 1;
	if (!controls [0].automapState && (nLeaveMode == 1))
		bDone = 1;
	bDone = GameFrame (bPauseGame, bDone);
	redbook.CheckRepeat ();
	bDone = gameStates.menus.nInMenu || ReadControls (nLeaveMode, bDone, bPauseGame);
	Update ();
	if (!gameOpts->render.stereo.nGlasses)
		Render ();
	else {
		Render (-gameOpts->render.stereo.xSeparation);
		Render (gameOpts->render.stereo.xSeparation);
		}
	if (bFirstTime) {
		bFirstTime = 0;
		//paletteManager.ResumeEffect ();
		}
	t2 = TimerGetFixedSeconds ();
	if (bPauseGame)
		gameData.time.SetTime (t2 - t1);
	t1 = t2;
	PROF_END(ptFrame)
	} while (!bDone);
#if 0
GrFreeCanvas (levelNumCanv);
levelNumCanv = NULL;
GrFreeCanvas (levelNameCanv);
levelNameCanv = NULL;
#endif
if (!gameStates.menus.nInMenu) {
	GameFlushInputs ();
	if (gameData.app.bGamePaused)
		ResumeGame ();
	ogl.m_states.nContrast = nContrast;
	}
if (!--m_bDisplay) {
	if (m_bChaseCam)
		SetChaseCam (1);
	else if (m_bFreeCam)
		SetFreeCam (1);
	}
}

//------------------------------------------------------------------------------

void CAutomap::AdjustSegmentLimit (void)
{
	int i,e1;
	tEdgeInfo * e;

for (i = 0; i <= m_nLastEdge; i++) {
	e = m_edges + i;
	e->flags |= EF_TOO_FAR;
	for (e1 = 0; e1 < e->nFaces; e1++) {
		if (m_visible [e->nSegment [e1]] <= m_nSegmentLimit) {
			e->flags &= ~EF_TOO_FAR;
			break;
			}
		}
	}
}

//------------------------------------------------------------------------------

void CAutomap::SetEdgeColor (int nColor, int bFade, float fScale)
{
if ((bFade != m_bFade) || (nColor != m_nColor) || (fScale != m_fScale)) {
	m_bFade = bFade;
	m_nColor = nColor;
	m_fScale = fScale;
	if (bFade)
		CCanvas::Current ()->SetColorRGBi (nColor);
	else
		CCanvas::Current ()->SetColorRGBi (RGBA_FADE (nColor, 32.0f / fScale));
	if (m_bDrawBuffers) {
		CCanvasColor canvColor = CCanvas::Current ()->Color ();
		m_color.Set (float (canvColor.Red ()), float (canvColor.Green ()), float (canvColor.Blue ()), float (canvColor.Alpha ()));
		m_color /= 255.0f;
		}
	}
}

//------------------------------------------------------------------------------

void CAutomap::DrawLine (short v0, short v1)
{
if ((v0 < 0) || (v1 < 0))
	return;
if (!m_bDrawBuffers)
	G3DrawLine (RENDERPOINTS + v0, RENDERPOINTS + v1);
else {
	if (m_nVerts > 998) {
		ogl.FlushBuffers (GL_LINES, m_nVerts, 3, 0, 1);
		m_nVerts = 0;
		}
	ogl.VertexBuffer () [m_nVerts].Assign (RENDERPOINTS [v0].ViewPos ());
	ogl.ColorBuffer () [m_nVerts] = m_color;
	m_nVerts++;
	ogl.VertexBuffer () [m_nVerts].Assign (RENDERPOINTS [v1].ViewPos ());
	ogl.ColorBuffer () [m_nVerts] = m_color;
	m_nVerts++;
	}
}

//------------------------------------------------------------------------------

void CAutomap::DrawEdges (void)
{
	tRenderCodes	cc;
	int				i, j, nbright = 0;
	ubyte				nfacing, nnfacing;
	tEdgeInfo*		edgeP;
	CFixVector		*tv1;
	fix				distance;
	fix				minDistance = 0x7fffffff;
	CRenderPoint	*p1, *p2;
	int				bUseTransform = ogl.UseTransform ();
	
m_bDrawBuffers = ogl.SizeBuffers (1000);
ogl.SetTransform (1);
glLineWidth (GLfloat (screen.Width ()) / 640.0f);
ogl.SetDepthTest (false);
ogl.SetDepthWrite (false);
ogl.SetTexturing (false);

m_bFade = m_nColor = -1;
m_fScale = 1e10f;
m_nVerts = 0;

for (i = 0; i <= m_nLastEdge; i++) {
	//edgeP = &m_edges [Edge_used_list [i]];
	edgeP = m_edges + i;
	if (!(edgeP->flags & EF_USED))
		continue;
	if (edgeP->flags & EF_TOO_FAR)
		continue;
	if (edgeP->flags & EF_FRONTIER) {		// A line that is between what we have seen and what we haven't
		if ((!(edgeP->flags & EF_SECRET)) && (edgeP->color == m_colors.walls.nNormal))
			continue;		// If a line isn't secret and is Normal color, then don't draw it
		}

	cc = TransformVertexList (2, edgeP->verts);
	distance = RENDERPOINTS [edgeP->verts [1]].ViewPos ().v.coord.z;
	if (minDistance > distance)
		minDistance = distance;
	if (!cc.ccAnd)  {	//all off screen?
		nfacing = nnfacing = 0;
		tv1 = gameData.segs.vertices + edgeP->verts [0];
		j = 0;
		while ((j < edgeP->nFaces) && !(nfacing && nnfacing)) {
			if (!G3CheckNormalFacing (*tv1, SEGMENTS [edgeP->nSegment [j]].m_sides [edgeP->sides [j]].m_normals [0]))
				nfacing++;
			else
				nnfacing++;
			j++;
			}

		if (nfacing && nnfacing) {
			// a corners line
			m_brightEdges [nbright++] = edgeP;
			}
		else if (edgeP->flags & (EF_DEFINING|EF_GRATE)) {
			if (nfacing) 
				m_brightEdges [nbright++] = edgeP;
			else {
				SetEdgeColor (int (edgeP->color), (edgeP->flags & EF_NO_FADE) != 0, 8.0f);
				DrawLine (edgeP->verts [0], edgeP->verts [1]);
				}
			}
		}
	}

if (m_bDrawBuffers && m_nVerts) {
	ogl.FlushBuffers (GL_LINES, m_nVerts, 3, 0, 1);
	m_nVerts = 0;
	}
if (minDistance < 0)
	minDistance = 0;

m_bFade = m_nColor = -1;
m_fScale = 1e10f;
// Sort the bright ones using a shell sort
{
	int i, j, incr, v1, v2;

incr = nbright / 2;
while (incr > 0) {
	for (i = incr; i < nbright; i++) {
		j = i - incr;
		while (j >= 0) {
			// compare element j and j + incr
			v1 = m_brightEdges [j]->verts [0];
			v2 = m_brightEdges [j + incr]->verts [0];

			if (RENDERPOINTS [v1].ViewPos ().v.coord.z < RENDERPOINTS [v2].ViewPos ().v.coord.z) {
				// If not in correct order, them swap 'em
				Swap (m_brightEdges [j + incr], m_brightEdges [j]);
				j -= incr;
				}
			else
				break;
			}
		}
	incr /= 2;
	}
}

// Draw the bright ones
for (i = 0; i < nbright; i++) {
	edgeP = m_brightEdges [i];
	p1 = RENDERPOINTS + edgeP->verts [0];
	p2 = RENDERPOINTS + edgeP->verts [1];
	fix xDist = p1->ViewPos ().v.coord.z - minDistance;
	// Make distance be 1.0 to 0.0, where 0.0 is 10 segments away;
	if (xDist < 0)
		xDist = 0;
	else if (xDist >= m_data.nMaxDist)
		continue;
	SetEdgeColor (int (edgeP->color), (edgeP->flags & EF_NO_FADE) != 0, X2F (I2X (1) - FixDiv (xDist, m_data.nMaxDist)) * 31);
	DrawLine (edgeP->verts [0], edgeP->verts [1]);
	}
if (m_bDrawBuffers && m_nVerts) {
	ogl.FlushBuffers (GL_LINES, m_nVerts, 3, 0, 1);
	m_nVerts = 0;
	}

ogl.SetDepthTest (true);
ogl.SetDepthWrite (true);
glLineWidth (1);
ogl.SetTransform (bUseTransform);
}


//==================================================================
//
// All routines below here are used to build the Edge list
//
//==================================================================


//finds edge, filling in edge_ptr. if found old edge, returns index, else return -1
int CAutomap::FindEdge (int v0, int v1, tEdgeInfo*& edgeP)
{
	int vv, evv;
	int hash,oldhash;
	int ret, ev0, ev1;

vv = (v1<<16) + v0;
oldhash = hash = ((v0*5+v1) % MAX_EDGES);
ret = -1;
while (ret == -1) {
	ev0 = (int) (m_edges [hash].verts [0]);
	ev1 = (int) (m_edges [hash].verts [1]);
	evv = (ev1 << 16) + ev0;
	if (m_edges [hash].nFaces == 0) 
		ret = 0;
	else if (evv == vv)
		ret = 1;
	else {
		if (++hash == MAX_EDGES)
			hash = 0;
		if (hash == oldhash)
			Error ("Edge list full!");
		}
	}
edgeP = &m_edges [hash];
return ret ? hash : -1;
}

//------------------------------------------------------------------------------

void CAutomap::AddEdge (int va, int vb, uint color, ubyte nSide, short nSegment, int bHidden, int bGrate, int bNoFade)
{
if (va == vb)
	return;
if ((va < 0) || (vb < 0))
	return;
if ((va == 0xFFFF) || (vb == 0xFFFF))
	return;

	int			found;
	tEdgeInfo*	edgeP;

	if (m_nEdges >= MAX_EDGES) {
		// GET JOHN!(And tell him that his
		// MAX_EDGES_FROM_VERTS formula is hosed.)
		// If he's not around, save the mine,
		// and send him  mail so he can look
		// at the mine later. Don't modify it.
		// This is important if this happens.
		Int3 ();		// LOOK ABOVE!!!!!!
		return;
	}

if (va > vb)
	Swap (va, vb);

#if DBG
if ((va == 1) && (vb == 3))
	va = va;
#endif

found = FindEdge (va, vb, edgeP);

if (found == -1) {
	edgeP->verts [0] = va;
	edgeP->verts [1] = vb;
	edgeP->color = color;
	edgeP->nFaces = 1;
	edgeP->flags = EF_USED | EF_DEFINING;			// Assume a Normal line
	edgeP->sides [0] = nSide;
	edgeP->nSegment [0] = nSegment;
	//Edge_used_list [m_nEdges] = EDGE_IDX (edgeP);
	if (EDGE_IDX (edgeP) > m_nLastEdge)
		m_nLastEdge = EDGE_IDX (edgeP);
	m_nEdges++;
	}
else {
	//Assert (edgeP->nFaces < 8);
	if ((color != m_colors.walls.nNormal) && (color != m_colors.walls.nRevealed))
		edgeP->color = color;
	if (edgeP->nFaces < 4) {
		edgeP->sides [edgeP->nFaces] = nSide;
		edgeP->nSegment [edgeP->nFaces] = nSegment;
		edgeP->nFaces++;
		}
	}
if (bGrate)
	edgeP->flags |= EF_GRATE;
if (bHidden)
	edgeP->flags |= EF_SECRET;		// Mark this as a bHidden edge
if (bNoFade)
	edgeP->flags |= EF_NO_FADE;
}

//------------------------------------------------------------------------------

void CAutomap::AddUnknownEdge (int va, int vb)
{
if (va == vb)
	return;

	tEdgeInfo *edgeP;

if (va > vb)
	Swap (va, vb);
if (FindEdge (va, vb, edgeP) != -1)
	edgeP->flags |= EF_FRONTIER;		// Mark as a border edge
}

//------------------------------------------------------------------------------

void CAutomap::AddSegmentEdges (CSegment *segP)
{
	int		bIsGrate, bNoFade;
	uint		color;
	ubyte		nSide;
	short		nSegment = segP->Index ();
	int		bHidden;
	ushort*	corners;
	CSide*	sideP = segP->Side (0);

for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++, sideP++) {
	if (sideP->m_nShape > SIDE_SHAPE_TRIANGLE)
		continue;
	bHidden = 0;
	bIsGrate = 0;
	bNoFade = 0;
	color = WHITE_RGBA;
	if (segP->m_children [nSide] == -1)
		color = m_colors.walls.nNormal;
	switch (segP->m_function) {
		case SEGMENT_FUNC_FUELCEN:
			color = GOLD_RGBA;
			break;
		case SEGMENT_FUNC_SPEEDBOOST:
			color = ORANGE_RGBA;
			break;
		case SEGMENT_FUNC_REPAIRCEN:
			color = RGBA_PAL2 (0, 15, 31);
			break;
		case SEGMENT_FUNC_CONTROLCEN:
			if (gameData.reactor.bPresent)
				color = RGBA_PAL2 (29, 0, 0);
			break;
		case SEGMENT_FUNC_ROBOTMAKER:
			color = RGBA_PAL2 (29, 0, 31);
			break;
		case SEGMENT_FUNC_SKYBOX:
			continue;
		case SEGMENT_FUNC_NONE:
			if (segP->HasBlockedProp ())
				color = RGBA_PAL2 (13, 13, 13);
			break;
		}

	CWall* wallP = segP->Wall (nSide);
	if (wallP) {
		CTrigger* triggerP = wallP->Trigger ();
		if (triggerP && (triggerP->m_info.nType == TT_SECRET_EXIT)) {
	 		color = RGBA_PAL2 (29, 0, 31);
			bNoFade = 1;
			goto addEdge;
			}
		switch (wallP->nType) {
			case WALL_DOOR:
				if (wallP->keys == KEY_BLUE) {
					bNoFade = 1;
					color = m_colors.walls.nDoorBlue;
					}
				else if (wallP->keys == KEY_GOLD) {
					bNoFade = 1;
					color = m_colors.walls.nDoorGold;
					}
				else if (wallP->keys == KEY_RED) {
					bNoFade = 1;
					color = m_colors.walls.nDoorRed;
					}
				else if (!(gameData.walls.animP [wallP->nClip].flags & WCF_HIDDEN)) {
					short	nConnSeg = segP->m_children [nSide];
					if (nConnSeg != -1) {
						short nConnSide = segP->ConnectedSide (SEGMENTS + nConnSeg);
						CWall* connWallP = SEGMENTS [nConnSeg].Wall (nConnSide);
						if (connWallP) {
							switch (connWallP->keys) {
								case KEY_BLUE:
									color = m_colors.walls.nDoorBlue;
									bNoFade = 1;
									break;
								case KEY_GOLD:
									color = m_colors.walls.nDoorGold;
									bNoFade = 1;
									break;
								case KEY_RED:
									color = m_colors.walls.nDoorRed;
									bNoFade = 1;
									break;
								default:
									color = m_colors.walls.nDoor;
								}
							}
						}
					}
				else {
					color = m_colors.walls.nNormal;
					bHidden = 1;
					}
				break;
			case WALL_CLOSED:
				// Make bGrates draw properly
				if (segP->IsDoorWay (nSide, NULL) & WID_TRANSPARENT_FLAG)
					bIsGrate = 1;
				else
					bHidden = 1;
				color = m_colors.walls.nNormal;
				break;
			case WALL_BLASTABLE:
				// Hostage doors
				color = m_colors.walls.nDoor;
				break;
			}
		}

	if (nSegment == gameData.multiplayer.playerInit [N_LOCALPLAYER].nSegment)
		color = RGBA_PAL2 (31,0,31);

	if (color != WHITE_RGBA) {
		// If they have a map powerup, draw unvisited areas in dark b.
		if ((LOCALPLAYER.flags & PLAYER_FLAGS_FULLMAP) &&
				!(gameStates.render.bAllVisited || m_visited [nSegment]))
			color = m_colors.walls.nRevealed;

addEdge:

		corners = SEGMENTS [nSegment].Corners (nSide);
		int nCorners = sideP->CornerCount ();
		for (int i = 0; i < nCorners; i++)
			AddEdge (corners [i % nCorners], corners [(i + 1) % nCorners], color, nSide, nSegment, bHidden, 0, bNoFade);
		if (bIsGrate && (nCorners == 4)) {
			AddEdge (corners [0], corners [2], color, nSide, nSegment, bHidden, 1, bNoFade);
			AddEdge (corners [1], corners [3], color, nSide, nSegment, bHidden, 1, bNoFade);
			}
		}
	}
}

//------------------------------------------------------------------------------
// Adds all the edges from a CSegment we haven't visited yet.

void CAutomap::AddUnknownSegmentEdges (CSegment* segP)
{
for (int nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
	// Only add edges that have no children
	if (segP->Side (nSide)->m_nShape > SIDE_SHAPE_TRIANGLE)
		continue;
	if (segP->m_children [nSide] == -1) {
		ushort* vertices = segP->m_vertices;
		int nVertices = segP->m_nVertices;
		for (int i = 0; i <= nVertices; i++)
			AddUnknownEdge (vertices [i], vertices [(i + 1) % nVertices]);
		}
	}
}

//------------------------------------------------------------------------------

void CAutomap::BuildEdgeList (void)
{
	int	i, e1, e2, nSegment;
	tEdgeInfo * e;

m_data.bCheat = 0;
if (LOCALPLAYER.flags & PLAYER_FLAGS_FULLMAP_CHEAT)
	m_data.bCheat = 1;		// Damn cheaters...

	// clear edge list
for (i = 0; i < MAX_EDGES; i++) {
	m_edges [i].nFaces = 0;
	m_edges [i].flags = 0;
	}
m_nEdges = 0;
m_nLastEdge = -1;

if (m_data.bCheat || (LOCALPLAYER.flags & PLAYER_FLAGS_FULLMAP)) {
	// Cheating, add all edges as visited
	for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++)
		AddSegmentEdges (&SEGMENTS [nSegment]);
	}
else {
	// Not cheating, add visited edges, and then unvisited edges
	for (nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++) {
#if DBG
		if (nSegment == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		if (m_visited [nSegment]) 
			AddSegmentEdges (&SEGMENTS [nSegment]);
		}
	for (nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++) {
#if DBG
		if (nSegment == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		if (!m_visited [nSegment]) 
			AddUnknownSegmentEdges (&SEGMENTS [nSegment]);
		}
	}	

// Find unnecessary lines (These are lines that don't have to be drawn because they have small curvature)
for (i = 0; i <= m_nLastEdge; i++) {
	e = m_edges + i;
	if (!(e->flags & EF_USED))
		continue;

	for (e1 = 0; e1 < e->nFaces; e1++) {
		for (e2 = 1; e2 < e->nFaces; e2++) {
			if ((e1 != e2) && (e->nSegment [e1] != e->nSegment [e2])) {
				if (CFixVector::Dot (SEGMENTS [e->nSegment [e1]].m_sides [e->sides [e1]].m_normals [0],
											SEGMENTS [e->nSegment [e2]].m_sides [e->sides [e2]].m_normals [0]) > (I2X (1)- (I2X (1)/10))) {
					e->flags &= (~EF_DEFINING);
					break;
					}
				}
			}
		if (!(e->flags & EF_DEFINING))
			break;
		}
	}
}

//------------------------------------------------------------------------------
//eof
