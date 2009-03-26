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
#include "gameseg.h"
#include "gamecntl.h"
#include "input.h"
#include "slowmotion.h"
#include "marker.h"
#include "songs.h"
#include "menubackground.h"
#include "systemkeys.h"
#include "menu.h"
#include "automap.h"

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
	Error ("File %s - PCX error: %s", BackgroundName (BG_MAP), pcx_errormsg (nPCXError));
	return false;
	}
m_background.Remap (NULL, -1, -1);
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
m_data.viewPos.SetZero ();
m_data.viewTarget.SetZero ();
m_data.viewMatrix = CFixMatrix::IDENTITY;
for (int i = 0; i < 2; i++) {
	if (!m_visited [i].Buffer ())
		m_visited [i].Create (MAX_SEGMENTS_D2X);
	m_visited [i].Clear ();
	}
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
m_visited [0].Clear ();
ClearMarkers ();
}

//------------------------------------------------------------------------------

void CAutomap::DrawPlayer (CObject* objP)
{
	CFixVector	vArrowPos, vHeadPos;
	g3sPoint		spherePoint, arrowPoint, headPoint;
	int			size = objP->info.xSize * (m_bRadar ? 2 : 1);
	int			bUseTransform = gameStates.ogl.bUseTransform;

//gameStates.ogl.bUseTransform = RENDERPATH;
headPoint.p3_index =
arrowPoint.p3_index =
spherePoint.p3_index = -1;
// Draw Console CPlayerData -- shaped like a ellipse with an arrow.
spherePoint.p3_vec.SetZero ();
G3TransformAndEncodePoint (&spherePoint, objP->info.position.vPos);
//transformation.Rotate (&spherePoint.p3_vec, &objP->info.position.vPos, 0);
G3DrawSphere (&spherePoint, m_bRadar ? objP->info.xSize * 2 : objP->info.xSize, !m_bRadar);

if (m_bRadar && (objP->Index () != LOCALPLAYER.nObject))
	return;
// Draw shaft of arrow
vArrowPos = objP->info.position.vPos + objP->info.position.mOrient.FVec () * (size*3);
G3TransformAndEncodePoint (&arrowPoint, vArrowPos);
G3DrawLine (&spherePoint, &arrowPoint);

// Draw right head of arrow
vHeadPos = objP->info.position.vPos + objP->info.position.mOrient.FVec () * (size*2);
vHeadPos += objP->info.position.mOrient.RVec () * (size*1);
G3TransformAndEncodePoint (&headPoint, vHeadPos);
G3DrawLine (&arrowPoint, &headPoint);

// Draw left head of arrow
vHeadPos = objP->info.position.vPos + objP->info.position.mOrient.FVec () * (size*2);
vHeadPos += objP->info.position.mOrient.RVec () * (size* (-1));
G3TransformAndEncodePoint (&headPoint, vHeadPos);
G3DrawLine (&arrowPoint, &headPoint);

// Draw CPlayerData's up vector
vArrowPos = objP->info.position.vPos + objP->info.position.mOrient.UVec () * (size*2);
G3TransformAndEncodePoint (&arrowPoint, vArrowPos);
G3DrawLine (&spherePoint, &arrowPoint);
gameStates.ogl.bUseTransform = bUseTransform;
}

//------------------------------------------------------------------------------

void CAutomap::DrawObjects (void)
{
if (!((gameOpts->render.automap.bTextured & 2) || m_bRadar))
	return;
int color = IsTeamGame ? GetTeam (gameData.multiplayer.nLocalPlayer) : gameData.multiplayer.nLocalPlayer;	// Note link to above if!
CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (playerColors [color].red, playerColors [color].green, playerColors [color].blue));
int bTextured = (gameOpts->render.automap.bTextured & 1) && !m_bRadar;
if (bTextured) {
	glDisable (GL_CULL_FACE);
	glEnable (GL_BLEND);
	gameStates.render.grAlpha = 0.5f;
	}
glDisable (GL_TEXTURE_2D);
glLineWidth (2 * GLfloat (screen.Width ()) / 640.0f);
DrawPlayer (OBJECTS + LOCALPLAYER.nObject);
if (!m_bRadar) {
	DrawMarkers ();
	if ((gameData.marker.nHighlight > -1) && (gameData.marker.szMessage [gameData.marker.nHighlight][0] != 0)) {
		char msg [10 + MARKER_MESSAGE_LEN + 1];
		sprintf (msg, TXT_MARKER_MSG, gameData.marker.nHighlight + 1,
					gameData.marker.szMessage [(gameData.multiplayer.nLocalPlayer * 2) + gameData.marker.nHighlight]);
		CCanvas::Current ()->SetColorRGB (196, 0, 0, 255);
		fontManager.SetCurrent (SMALL_FONT);
		GrString (5, 20, msg, NULL);
		}
	}
// Draw player(s)...
if (AM_SHOW_PLAYERS) {
	for (int i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if ((i != gameData.multiplayer.nLocalPlayer) && AM_SHOW_PLAYER (i)) {
			if (OBJECTS [gameData.multiplayer.players [i].nObject].info.nType == OBJ_PLAYER) {
				color = (gameData.app.nGameMode & GM_TEAM) ? GetTeam (i) : i;
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (playerColors [color].red, playerColors [color].green, playerColors [color].blue));
				if (bTextured)
					glEnable (GL_BLEND);
				DrawPlayer (OBJECTS + gameData.multiplayer.players [i].nObject);
				}
			}
		}
	}

if (bTextured)
	glEnable (GL_BLEND);

CObject* objP = OBJECTS.Buffer ();
g3sPoint	spherePoint;

FORALL_OBJS (objP, i) {
	int size = objP->info.xSize;
	switch (objP->info.nType) {
		case OBJ_HOSTAGE:
			CCanvas::Current ()->SetColorRGBi (m_colors.nHostage);
			G3TransformAndEncodePoint (&spherePoint, objP->info.position.vPos);
			G3DrawSphere (&spherePoint,size, !m_bRadar);
			break;

		case OBJ_MONSTERBALL:
			CCanvas::Current ()->SetColorRGBi (m_colors.nMonsterball);
			G3TransformAndEncodePoint (&spherePoint, objP->info.position.vPos);
			G3DrawSphere (&spherePoint,size, !m_bRadar);
			break;

		case OBJ_ROBOT:
			if (AM_SHOW_ROBOTS && ((gameStates.render.bAllVisited && bTextured) || m_visited [0][objP->info.nSegment])) {
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
				G3TransformAndEncodePoint (&spherePoint, objP->info.position.vPos);
				//transformation.Begin (&objP->info.position.vPos, &objP->info.position.mOrient);
				G3DrawSphere (&spherePoint, bTextured ? size : (size * 3) / 2, !m_bRadar);
				//transformation.End ();
				}
			break;

		case OBJ_POWERUP:
			if (AM_SHOW_POWERUPS (1) && (gameStates.render.bAllVisited || m_visited [0][objP->info.nSegment])) {
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
						CCanvas::Current ()->SetColorRGBi (ORANGE_RGBA); //orange
						//Error ("Illegal key nType: %i", objP->info.nId);
					}
				G3TransformAndEncodePoint (&spherePoint, objP->info.position.vPos);
				G3DrawSphere (&spherePoint, size, !m_bRadar);
				}
			break;
		}
	}
gameStates.render.grAlpha = 1.0f;
glEnable (GL_CULL_FACE);
glLineWidth (1);
}

//------------------------------------------------------------------------------

void CAutomap::DrawLevelId (void)
{
if (gameStates.app.bNostalgia)
	return;

	CFont*	curFont = CCanvas::Current ()->Font ();
	int		w, h, aw, offs = m_data.bHires ? 10 : 5;
	char		szInfo [3][200];

fontManager.SetCurrent (SMALL_FONT);
fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);

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
for (int i = 0; (i < 3) && *szInfo [i]; i++) {
	fontManager.Current ()->StringSize (szInfo [i], w, h, aw);
	GrPrintF (NULL, (CCanvas::Current ()->Width () - w) / 2, CCanvas::Current ()->Height () - offs - (i + 1) * h - i * 2, szInfo [0]);
	}
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

void CAutomap::Draw (void)
{
#if 1
PROF_START
	int	bAutomapFrame = !m_bRadar &&
								 (gameStates.render.cockpit.nType != CM_FULL_SCREEN) &&
								 (gameStates.render.cockpit.nType != CM_LETTERBOX);
	CFixMatrix	mRadar;

automap.m_bFull = (LOCALPLAYER.flags & (PLAYER_FLAGS_FULLMAP_CHEAT | PLAYER_FLAGS_FULLMAP)) != 0;
if ((m_bRadar = m_bRadar) == 2) {
	CFixMatrix& po = gameData.multiplayer.playerInit [gameData.multiplayer.nLocalPlayer].position.mOrient;
#if 1
	mRadar.RVec () = po.RVec ();
	mRadar.FVec () = po.UVec ();
	mRadar.FVec () [Y] = -mRadar.FVec () [Y];
	mRadar.UVec () = po.FVec ();
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
CCanvas::Current ()->Clear (RGBA_PAL2 (0,0,0));
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

if (!gameOpts->render.automap.bTextured)
	gameOpts->render.automap.bTextured = 1;
G3StartFrame (m_bRadar || !(gameOpts->render.automap.bTextured & 1), 0); //!m_bRadar);

if (bAutomapFrame)
	OglViewport (RESCALE_X (27), RESCALE_Y (80), RESCALE_X (582), RESCALE_Y (334));
RenderStartFrame ();
if (m_bRadar == 2) {
	m_data.viewPos = m_data.viewTarget + mRadar.FVec () * (-m_data.nViewDist);
	G3SetViewMatrix (m_data.viewPos, mRadar, m_data.nZoom * 2, 1);
	}
else {
	m_data.viewPos = m_data.viewTarget + m_data.viewMatrix.FVec () * -m_data.nViewDist;
	G3SetViewMatrix (m_data.viewPos, m_data.viewMatrix, m_bRadar ? (m_data.nZoom * 3) / 2 : m_data.nZoom, 1);
	}
if (!m_bRadar && (gameOpts->render.automap.bTextured & 1)) {
	gameData.render.mine.viewerEye = m_data.viewPos;
	RenderMine (gameData.objs.consoleP->info.nSegment, 0, 0);
	RenderEffects (0);
	}
if (m_bRadar || (gameOpts->render.automap.bTextured & 2)) {
	DrawEdges ();
	DrawObjects ();
	}
G3EndFrame ();

gameData.app.nFrameCount++;
if (m_bRadar) {
	gameStates.ogl.bEnableScissor = 0;
	return;
	}
DrawLevelId ();
PROF_END(ptRenderFrame)
#endif
OglSwapBuffers (0, 0);
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

if (gameData.missions.nCurrentLevel > 0)
	sprintf (m_szLevelNum, "%s %i",TXT_LEVEL, gameData.missions.nCurrentLevel);
else
	sprintf (m_szLevelNum, "Secret Level %i", -gameData.missions.nCurrentLevel);
if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) &&
		(gameData.missions.nCurrentLevel > 0))		//built-in mission
	sprintf (m_szLevelName,"%s %d: ",
				pszSystemNames [(gameData.missions.nCurrentLevel - 1) / 4],
				((gameData.missions.nCurrentLevel - 1) % 4) + 1);
else
	strcpy (m_szLevelName, " ");
strcat (m_szLevelName, gameData.missions.szCurrentLevel);
for (h = i = 0; i < gameData.segs.nSegments; i++)
	if (m_visited [0][i])
		h++;
sprintf (szExplored, " (%1.1f %s)", (float) (h * 100) / (float) gameData.segs.nSegments, TXT_PERCENT_EXPLORED);
strcat (m_szLevelName, szExplored);
}

//------------------------------------------------------------------------------

int SetSegmentDepths (int start_seg, ushort *pDepthBuf);


int CAutomap::Setup (int bPauseGame, fix& xEntryTime)
{
	int		i;
	fix		t1, t2;
	CObject	*playerP;

if (m_bDisplay < 0) {
	m_bDisplay = 0;
	if ((m_bChaseCam = gameStates.render.bChaseCam))
		SetChaseCam (0);
	if ((m_bFreeCam = gameStates.render.bFreeCam))
		SetFreeCam (0);

	gameStates.ogl.nContrast = 8;
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
	m_data.viewMatrix = playerP->info.position.mOrient;

	m_vTAngles [PA] = PITCH_DEFAULT;
	m_vTAngles [HA] = 0;
	m_vTAngles [BA] = 0;

	m_data.viewTarget = playerP->info.position.vPos;
	t1 = xEntryTime = TimerGetFixedSeconds ();
	t2 = t1;
	}
BuildEdgeList ();
//Fill in m_visited [0] from OBJECTS [LOCALPLAYER.nObject].nSegment
if (m_bRadar) {
	for (i = 0; i < gameData.segs.nSegments; i++)
		automap.m_visible [i] = 1;
	}
else if (automap.m_bFull) {
	for (i = 0; i < gameData.segs.nSegments; i++)
		automap.m_visible [i] = 1;
	}
else
	memcpy (automap.m_visible.Buffer (), m_visited [0].Buffer (), m_visited [0].Size ());
//m_visited [0][OBJECTS [LOCALPLAYER.nObject].nSegment] = 1;
m_nSegmentLimit =
m_nMaxSegsAway =
	SetSegmentDepths (OBJECTS [LOCALPLAYER.nObject].info.nSegment, automap.m_visible.Buffer ());
AdjustSegmentLimit (m_nSegmentLimit, automap.m_visible);
m_bDisplay++;
return gameData.app.bGamePaused;
}

//------------------------------------------------------------------------------

int CAutomap::Update (void)
{
	CObject*		playerP = OBJECTS + LOCALPLAYER.nObject;
	CFixMatrix	m;

if (Controls [0].firePrimaryDownCount) {
	// Reset orientation
	m_data.nViewDist = ZOOM_DEFAULT;
	m_vTAngles [PA] = PITCH_DEFAULT;
	m_vTAngles [HA] = 0;
	m_vTAngles [BA] = 0;
	m_data.viewTarget = playerP->info.position.vPos;
	}
if (Controls [0].forwardThrustTime)
	m_data.viewTarget += m_data.viewMatrix.FVec () * (Controls [0].forwardThrustTime * ZOOM_SPEED_FACTOR);
m_vTAngles [PA] += (fixang) FixDiv (Controls [0].pitchTime, ROT_SPEED_DIVISOR);
m_vTAngles [HA] += (fixang) FixDiv (Controls [0].headingTime, ROT_SPEED_DIVISOR);
m_vTAngles [BA] += (fixang) FixDiv (Controls [0].bankTime, ROT_SPEED_DIVISOR*2);

m = CFixMatrix::Create (m_vTAngles);
if (Controls [0].verticalThrustTime || Controls [0].sidewaysThrustTime) {
	m_data.viewMatrix = playerP->info.position.mOrient * m;
	m_data.viewTarget += m_data.viewMatrix.UVec () * (Controls [0].verticalThrustTime * SLIDE_SPEED);
	m_data.viewTarget += m_data.viewMatrix.RVec () * (Controls [0].sidewaysThrustTime * SLIDE_SPEED);
	}
m_data.viewMatrix = playerP->info.position.mOrient * m;
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

ControlsReadAll ();
if (Controls [0].automapDownCount && !nLeaveMode)
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
			gameStates.app.bSaveScreenshot = 1;
			SaveScreenShot (NULL, 1);
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
			m_nMaxSegsAway =
				SetSegmentDepths (OBJECTS [LOCALPLAYER.nObject].info.nSegment, automap.m_visible.Buffer ());
			AdjustSegmentLimit (m_nSegmentLimit, automap.m_visible);
			}
			break;
#endif

		case KEY_MINUS:
		case KEY_PADMINUS:
			if (m_nSegmentLimit > 1)  {
				m_nSegmentLimit -= ViewDistStep ();
				if (!m_nSegmentLimit)
					m_nSegmentLimit = 1;
				AdjustSegmentLimit (m_nSegmentLimit, automap.m_visible);
				}
			break;

		case KEY_EQUALS:
		case KEY_PADPLUS:
			if (m_nSegmentLimit < m_nMaxSegsAway) {
				m_nSegmentLimit += ViewDistStep ();
				if (m_nSegmentLimit > m_nMaxSegsAway)
					m_nSegmentLimit = m_nMaxSegsAway;
				AdjustSegmentLimit (m_nSegmentLimit, automap.m_visible);
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
			 nMaxDrop = MaxDrop ();
			nMarker = c - KEY_1;
			if (nMarker <= nMaxDrop) {
				if (gameData.marker.objects [nMarker] != -1)
					gameData.marker.nHighlight = nMarker;
				}
		break;

		case KEY_CTRLED + KEY_D:
			DeleteMarker (0);
			break;

		case KEY_F1:
			if (gameOpts->render.automap.bTextured == 1)
				gameOpts->render.automap.bTextured = 3;
			else if (gameOpts->render.automap.bTextured == 3)
				gameOpts->render.automap.bTextured = 2;
			else
				gameOpts->render.automap.bTextured = 1;
			break;

		case KEY_F2:
			if (gameOpts->render.automap.bTextured & 1) {
				nColor = (nColor + 1) % 3;
				gameOpts->render.automap.bBright = (nColor & 1) != 0;
				gameOpts->render.automap.bGrayOut = (nColor & 2) != 0;
				}
			break;

		case KEY_F3:
			gameOpts->render.automap.bSparks =
			gameOpts->render.automap.bCoronas =
			gameOpts->render.automap.bLightnings =
			gameOpts->render.automap.bParticles =
				!(gameOpts->render.automap.bSparks ||
				  gameOpts->render.automap.bCoronas ||
				  gameOpts->render.automap.bLightnings ||
				  gameOpts->render.automap.bParticles);
			break;

		case KEY_F4:
			extraGameInfo [IsMultiGame].bPowerupsOnRadar =
			extraGameInfo [IsMultiGame].bRobotsOnRadar =
				!(extraGameInfo [IsMultiGame].bPowerupsOnRadar || extraGameInfo [IsMultiGame].bRobotsOnRadar);
			break;

		case KEY_F9:
			TeleportToMarker ();
			break;

		case KEY_ALTED + KEY_F:
			gameStates.render.bShowFrameRate = ++gameStates.render.bShowFrameRate % (6 + (gameStates.render.bPerPixelLighting == 2));
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
			gameOpts->render.automap.bLightnings = !gameOpts->render.automap.bLightnings;
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

#if DBG
		case KEY_COMMA:
			if (gameData.marker.fScale > 0.5)
				gameData.marker.fScale -= 0.5;
			break;
		case KEY_PERIOD:
			if (gameData.marker.fScale < 30.0)
				gameData.marker.fScale += 0.5;
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
	controlInfoSave = Controls [0];				// Save controls so we can zero them
	memset (&Controls, 0, sizeof (tControlInfo));	// Clear everything...
	bWiggleSave = gameData.objs.consoleP->mType.physInfo.flags & PF_WIGGLE;	// Save old wiggle
	gameData.objs.consoleP->mType.physInfo.flags &= ~PF_WIGGLE;		// Turn off wiggle
	if (MultiMenuPoll ())
		bDone = 1;
	GameLoop (0, 0);		// Do game loop with no rendering and no reading controls.
	gameData.objs.consoleP->mType.physInfo.flags |= bWiggleSave;	// Restore wiggle
	Controls [0] = controlInfoSave;
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
	int				nContrast = gameStates.ogl.nContrast;
	int				bRedrawScreen = 0;

	//static ubyte	automapPal [256*3];

m_nMaxSegsAway = 0;
m_nSegmentLimit = 1;
m_bRadar = bRadar;
bPauseGame = Setup (bPauseGame, xEntryTime);
bRedrawScreen = 0;
if (bRadar) {
	Draw ();

	gameStates.ogl.nContrast = nContrast;
	if (!--m_bDisplay) {
		if (m_bChaseCam)
			SetChaseCam (1);
		else if (m_bFreeCam)
			SetFreeCam (1);
		}
	return;
	}
Controls [0].automapState = 0;
GetSlowTicks ();
do {
	PROF_START
	if (!nLeaveMode && Controls [0].automapState && ((TimerGetFixedSeconds ()- xEntryTime) > LEAVE_TIME))
		nLeaveMode = 1;
	if (!Controls [0].automapState && (nLeaveMode == 1))
		bDone = 1;
	bDone = GameFrame (bPauseGame, bDone);
	redbook.CheckRepeat ();
	bDone = gameStates.menus.nInMenu || ReadControls (nLeaveMode, bDone, bPauseGame);
	Update ();
	Draw ();
	if (bFirstTime) {
		bFirstTime = 0;
		paletteManager.LoadEffect ();
		}
	t2 = TimerGetFixedSeconds ();
	if (bPauseGame)
		gameData.time.xFrame = t2 - t1;
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
	gameStates.ogl.nContrast = nContrast;
	}
if (!--m_bDisplay) {
	if (m_bChaseCam)
		SetChaseCam (1);
	else if (m_bFreeCam)
		SetFreeCam (1);
	}
}

//------------------------------------------------------------------------------

void CAutomap::AdjustSegmentLimit (int nSegmentLimit, CArray<ushort>& visible)
{
	int i,e1;
	tEdgeInfo * e;

for (i = 0; i <= m_nLastEdge; i++) {
	e = m_edges + i;
	e->flags |= EF_TOO_FAR;
	for (e1 = 0; e1 < e->nFaces; e1++) {
		if (visible [e->nSegment [e1]] <= nSegmentLimit) {
			e->flags &= ~EF_TOO_FAR;
			break;
			}
		}
	}
}

//------------------------------------------------------------------------------

void CAutomap::DrawEdges (void)
{
	g3sCodes		cc;
	int			i, j, nbright = 0;
	ubyte			nfacing, nnfacing;
	tEdgeInfo*	edgeP;
	CFixVector	*tv1;
	fix			distance;
	fix			minDistance = 0x7fffffff;
	g3sPoint		*p1, *p2;
	int			bUseTransform = gameStates.ogl.bUseTransform;

gameStates.ogl.bUseTransform = RENDERPATH;
glLineWidth (GLfloat (screen.Width ()) / 640.0f);
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

	cc = RotateVertexList (2, edgeP->verts);
	distance = gameData.segs.points [edgeP->verts [1]].p3_vec [Z];
	if (minDistance>distance)
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
			if (nfacing == 0) {
				if (edgeP->flags & EF_NO_FADE)
					CCanvas::Current ()->SetColorRGBi (edgeP->color);
				else
					CCanvas::Current ()->SetColorRGBi (RGBA_FADE (edgeP->color, 32.0 / 8.0));
				G3DrawLine (gameData.segs.points + edgeP->verts [0], gameData.segs.points + edgeP->verts [1]);
				}
			else {
				m_brightEdges [nbright++] = edgeP;
				}
			}
		}
	}

if (minDistance < 0)
	minDistance = 0;

// Sort the bright ones using a shell sort
{
	int i, j, incr, v1, v2;

incr = nbright / 2;
while (incr > 0) {
	for (i = incr; i < nbright; i++) {
		j = i - incr;
		while (j>=0) {
			// compare element j and j + incr
			v1 = m_brightEdges [j]->verts [0];
			v2 = m_brightEdges [j + incr]->verts [0];

			if (gameData.segs.points [v1].p3_vec [Z] < gameData.segs.points [v2].p3_vec [Z]) {
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
	int color;
	fix dist;
	edgeP = m_brightEdges [i];
	p1 = gameData.segs.points + edgeP->verts [0];
	p2 = gameData.segs.points + edgeP->verts [1];
	dist = p1->p3_vec [Z] - minDistance;
	// Make distance be 1.0 to 0.0, where 0.0 is 10 segments away;
	if (dist < 0)
		dist = 0;
	if (dist >= m_data.nMaxDist)
		continue;

	if (edgeP->flags & EF_NO_FADE)
		CCanvas::Current ()->SetColorRGBi (edgeP->color);
	else {
		dist = I2X (1) - FixDiv (dist, m_data.nMaxDist);
		color = X2I (dist*31);
		CCanvas::Current ()->SetColorRGBi (RGBA_FADE (edgeP->color, 32.0 / color));
		}
	G3DrawLine (p1, p2);
	}
glLineWidth (1);
gameStates.ogl.bUseTransform = bUseTransform;
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
	evv = (ev1<<16)+ev0;
	if (m_edges [hash].nFaces == 0) ret=0;
	else if (evv == vv)
		ret=1;
	else {
		if (++hash==MAX_EDGES)
			hash=0;
		if (hash==oldhash)
			Error ("Edge list full!");
		}
	}
edgeP = &m_edges [hash];
return ret ? hash : -1;
}

//------------------------------------------------------------------------------

void CAutomap::AddEdge (int va, int vb, uint color, ubyte nSide, short nSegment, int bHidden, int bGrate, int bNoFade)
{
	int			found;
	tEdgeInfo*	edgeP;
	int			tmp;

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

if (va > vb) {
	tmp = va;
	va = vb;
	vb = tmp;
	}
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
	short*	corners;

for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	bHidden = 0;
	bIsGrate = 0;
	bNoFade = 0;
	color = WHITE_RGBA;
	if (segP->m_children [nSide] == -1)
		color = m_colors.walls.nNormal;
	switch (SEGMENTS [nSegment].m_nType) {
		case SEGMENT_IS_FUELCEN:
			color = GOLD_RGBA;
			break;
		case SEGMENT_IS_SPEEDBOOST:
			color = ORANGE_RGBA;
			break;
		case SEGMENT_IS_BLOCKED:
			color = RGBA_PAL2 (13, 13, 13);
			break;
		case SEGMENT_IS_REPAIRCEN:
			color = RGBA_PAL2 (0, 15, 31);
			break;
		case SEGMENT_IS_CONTROLCEN:
			if (gameData.reactor.bPresent)
				color = RGBA_PAL2 (29, 0, 0);
			break;
		case SEGMENT_IS_ROBOTMAKER:
			color = RGBA_PAL2 (29, 0, 31);
			break;
		case SEGMENT_IS_SKYBOX:
			continue;
		}

	CWall* wallP = segP->Wall (nSide);
	if (wallP) {
		CTrigger* triggerP = wallP->Trigger ();
		if (triggerP && (triggerP->nType == TT_SECRET_EXIT)) {
	 		color = RGBA_PAL2 (29, 0, 31);
			bNoFade=1;
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
				if (segP->IsDoorWay (nSide, NULL) & WID_RENDPAST_FLAG)
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

	if (nSegment == gameData.multiplayer.playerInit [gameData.multiplayer.nLocalPlayer].nSegment)
		color = RGBA_PAL2 (31,0,31);

	if (color != WHITE_RGBA) {
		// If they have a map powerup, draw unvisited areas in dark blue.
		if ((LOCALPLAYER.flags & PLAYER_FLAGS_FULLMAP) &&
				!(gameStates.render.bAllVisited || automap.m_visited [0][nSegment]))
			color = m_colors.walls.nRevealed;

addEdge:

		corners = SEGMENTS [nSegment].Corners (nSide);
		AddEdge (corners [0], corners [1], color, nSide, nSegment, bHidden, 0, bNoFade);
		AddEdge (corners [1], corners [2], color, nSide, nSegment, bHidden, 0, bNoFade);
		AddEdge (corners [2], corners [3], color, nSide, nSegment, bHidden, 0, bNoFade);
		AddEdge (corners [3], corners [0], color, nSide, nSegment, bHidden, 0, bNoFade);

		if (bIsGrate) {
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
for (int nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	// Only add edges that have no children
	if (segP->m_children [nSide] == -1) {
		short* corners = segP->Corners (nSide);
		AddUnknownEdge (corners [0], corners [1]);
		AddUnknownEdge (corners [1], corners [2]);
		AddUnknownEdge (corners [2], corners [3]);
		AddUnknownEdge (corners [3], corners [0]);
		}
	}
}

//------------------------------------------------------------------------------

void CAutomap::BuildEdgeList (void)
{
	int	h = 0, i, e1, e2, s;
	tEdgeInfo * e;

m_data.bCheat = 0;
if (LOCALPLAYER.flags & PLAYER_FLAGS_FULLMAP_CHEAT)
	m_data.bCheat = 1;		// Damn cheaters...

	// clear edge list
for (i=0; i < MAX_EDGES; i++) {
	m_edges [i].nFaces = 0;
	m_edges [i].flags = 0;
	}
m_nEdges = 0;
m_nLastEdge = -1;

if (m_data.bCheat || (LOCALPLAYER.flags & PLAYER_FLAGS_FULLMAP)) {
	// Cheating, add all edges as visited
	for (s = 0; s <= gameData.segs.nLastSegment; s++)
		AddSegmentEdges (&SEGMENTS [s]);
	}
else {
	// Not cheating, add visited edges, and then unvisited edges
	for (s = 0; s <= gameData.segs.nLastSegment; s++)
		if (automap.m_visited [0][s]) {
			h++;
			AddSegmentEdges (&SEGMENTS [s]);
			}
		for (s = 0; s <= gameData.segs.nLastSegment; s++)
			if (!automap.m_visited [0][s]) {
				AddUnknownSegmentEdges (&SEGMENTS [s]);
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
