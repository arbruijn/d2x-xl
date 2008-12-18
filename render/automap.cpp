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

#include "inferno.h"
#include "error.h"
#include "u_mem.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "render.h"
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
#include "gauges.h"
#include "gameseg.h"
#include "gamecntl.h"
#include "input.h"
#include "slowmotion.h"
#include "marker.h"
#include "automap.h"

#define EF_USED     1   // This edge is used
#define EF_DEFINING 2   // A structure defining edge that should always draw.
#define EF_FRONTIER 4   // An edge between the known and the unknown.
#define EF_SECRET   8   // An edge that is part of a secret CWall.
#define EF_GRATE    16  // A bGrate... draw it all the time.
#define EF_NO_FADE  32  // An edge that doesn't fade with distance
#define EF_TOO_FAR  64  // An edge that is too far away

void ModexPrintF (int x,int y, char *s, CFont *font, uint color);

typedef struct tEdgeInfo {
	short verts [2];     // 4 bytes
	ubyte sides [4];     // 4 bytes
	short nSegment [4];    // 8 bytes  // This might not need to be stored... If you can access the normals of a CSide.
	ubyte flags;        // 1 bytes  // See the EF_??? defines above.
	uint color; // 4 bytes
	ubyte num_faces;    // 1 bytes  // 19 bytes...
} tEdgeInfo;

#define MAX_EDGES 65536 // Determined by loading all the levels by John & Mike, Feb 9, 1995

#define K_WALL_NORMAL_COLOR     RGBA_PAL2 (29, 29, 29)
#define K_WALL_DOOR_COLOR       RGBA_PAL2 (5, 27, 5)
#define K_WALL_DOOR_BLUE        RGBA_PAL2 (0, 0, 31)
#define K_WALL_DOOR_GOLD        RGBA_PAL2 (31, 31, 0)
#define K_WALL_DOOR_RED         RGBA_PAL2 (31, 0, 0)
#define K_WALL_REVEALED_COLOR   RGBA_PAL2 (0, 0, 25) //what you see when you have the full map powerup
#define K_HOSTAGE_COLOR         RGBA_PAL2 (0, 31, 0)
#define K_MONSTERBALL_COLOR     RGBA_PAL2 (31, 23, 0)

typedef struct amWallColors {
	uint	nNormal;
	uint	nDoor;
	uint	nDoorBlue;
	uint	nDoorGold;
	uint	nDoorRed;
	uint	nRevealed;
} amWallColors;

typedef struct amColors {
	amWallColors	walls;
	uint	nHostage;
	uint	nMonsterball;
	uint	nWhite;
	uint	nMedGreen;
	uint	nLgtBlue;
	uint	nLgtRed;
	uint	nDkGray;
} amColors;

amColors automapColors;

static char	amLevelNum [200], amLevelName [200];

void InitAutomapColors (void)
{
automapColors.walls.nNormal = K_WALL_NORMAL_COLOR;
automapColors.walls.nDoor = K_WALL_DOOR_COLOR;
automapColors.walls.nDoorBlue = K_WALL_DOOR_BLUE;
automapColors.walls.nDoorGold = K_WALL_DOOR_GOLD;
automapColors.walls.nDoorRed = K_WALL_DOOR_RED;
automapColors.walls.nRevealed = K_WALL_REVEALED_COLOR;
automapColors.nHostage = K_HOSTAGE_COLOR;
automapColors.nMonsterball = K_MONSTERBALL_COLOR;
automapColors.nDkGray = RGBA_PAL2 (20,20,20);
automapColors.nMedGreen = RGBA_PAL2 (0,31,0);
automapColors.nWhite = RGBA_PAL2 (63,63,63);
automapColors.nLgtBlue = RGBA_PAL2 (0,0,48);
automapColors.nLgtRed = RGBA_PAL2 (48,0,0);
}

// Segment visited list
// Edge list variables
static int nNumEdges=0;
static int nMaxEdges;		//set each frame
static int nHighestEdgeIndex = -1;
static tEdgeInfo Edges [MAX_EDGES];
static int DrawingListBright [MAX_EDGES];

#define EDGE_IDX(_edgeP)	((int) ((_edgeP) - Edges))

//static short DrawingListBright [MAX_EDGES];
//static short Edge_used_list [MAX_EDGES];				//which entries in edge_list have been used

// Map movement defines
#define PITCH_DEFAULT 9000
#define ZOOM_DEFAULT I2X (20*10)
#define ZOOM_MIN_VALUE I2X (20*5)
#define ZOOM_MAX_VALUE I2X (20*100)

#define SLIDE_SPEED 				 (350)
#define ZOOM_SPEED_FACTOR		500	// (1500)
#define ROT_SPEED_DIVISOR		 (115000)

//static CCanvas	automap_canvas;
static CBitmap bmAutomapBackground;

typedef struct tAutomapData {
	int			bCheat;
	int			bHires;
	fix			nViewDist;
	fix			nMaxDist;
	fix			nZoom;
	CFixVector	viewPos;
	CFixVector	viewTarget;
	CFixMatrix	viewMatrix;
} tAutomapData;

// Rendering variables
//static tAutomapData	amData = {0, 1, 0, F1_0 * 20 * 100, 0x9000, CFixVector::ZERO, CFixVector::ZERO, {CFixVector::ZERO,CFixVector::ZERO,CFixVector::ZERO}};
static tAutomapData	amData; // = {0, 1, 0, F1_0 * 20 * 100, 0x9000, CFixVector::ZERO, CFixVector::ZERO, CFixMatrix::IDENTITY};

//	Function Prototypes
void AdjustSegmentLimit (int nSegmentLimit, ushort *pVisited);
void DrawAllEdges (void);
void AutomapBuildEdgeList (void);

//------------------------------------------------------------------------------

void InitAutomapData (void)
{
amData.bCheat = 0;
amData.bHires = 1;
amData.nViewDist = 0;
amData.nMaxDist = F1_0 * 2000;
amData.nZoom = 0x9000;
amData.viewPos.SetZero ();
amData.viewTarget.SetZero ();
amData.viewMatrix = CFixMatrix::IDENTITY;
}

//------------------------------------------------------------------------------

void AutomapClearVisited ()
{
memset (gameData.render.mine.bAutomapVisited, 0, sizeof (gameData.render.mine.bAutomapVisited));
ClearMarkers ();
}

//------------------------------------------------------------------------------

#if 0
CCanvas *levelNumCanv, *levelNameCanv;
#endif

#ifndef Pi
#	define Pi 3.141592653589793240
#endif
#define f2glf(x) (X2F (x))

//------------------------------------------------------------------------------

int G3DrawSphere3D (g3sPoint *p0, int nSides, int rad)
{
	tCanvasColor	c = CCanvas::Current ()->Color ();
	g3sPoint			p = *p0;
	int				i;
	float				hx, hy, x, y, z, r;
	float				ang;

glDisable (GL_TEXTURE_2D);
OglCanvasColor (&CCanvas::Current ()->Color ());
x = f2glf (p.p3_vec[X]);
y = f2glf (p.p3_vec[Y]);
z = f2glf (p.p3_vec[Z]);
r = f2glf (rad);
glBegin (GL_POLYGON);
for (i = 0; i <= nSides; i++) {
	ang = 2.0f * (float) Pi * (i % nSides) / nSides;
	hx = x + (float) cos (ang) * r;
	hy = y + (float) sin (ang) * r;
	glVertex3f (hx, hy, z);
	}
if (c.rgb)
	glDisable (GL_BLEND);
glEnd ();
return 1;
}

//------------------------------------------------------------------------------

int G3DrawCircle3D (g3sPoint *p0, int nSides, int rad)
{
	g3sPoint		p = *p0;
	int			i, j;
	CFloatVector		v;
	float			x, y, r;
	float			ang;

glDisable (GL_TEXTURE_2D);
OglCanvasColor (&CCanvas::Current ()->Color ());
x = f2glf (p.p3_vec[X]);
y = f2glf (p.p3_vec[Y]);
v[Z] = f2glf (p.p3_vec[Z]);
r = f2glf (rad);
glBegin (GL_LINES);
for (i = 0; i <= nSides; i++)
	for (j = i; j <= i + 1; j++) {
		ang = 2.0f * (float) Pi * (j % nSides) / nSides;
		v[X] = x + (float) cos (ang) * r;
		v[Y] = y + (float) sin (ang) * r;
		glVertex3fv (reinterpret_cast<GLfloat*> (&v));
		}
if (CCanvas::Current ()->Color ().rgb)
	glDisable (GL_BLEND);
glEnd ();
return 1;
}

//------------------------------------------------------------------------------

void DrawPlayer (CObject * objP)
{
	CFixVector	vArrowPos, vHeadPos;
	g3sPoint		spherePoint, arrowPoint, headPoint;
	int			size = objP->info.xSize * (gameStates.render.automap.bRadar ? 2 : 1);
	int			bUseTransform = gameStates.ogl.bUseTransform;

//gameStates.ogl.bUseTransform = RENDERPATH;
headPoint.p3_index =
arrowPoint.p3_index =
spherePoint.p3_index = -1;
// Draw Console CPlayerData -- shaped like a ellipse with an arrow.
spherePoint.p3_vec.SetZero();
G3TransformAndEncodePoint (&spherePoint, objP->info.position.vPos);
//transformation.Rotate (&spherePoint.p3_vec, &objP->info.position.vPos, 0);
G3DrawSphere (&spherePoint, gameStates.render.automap.bRadar ? objP->info.xSize * 2 : objP->info.xSize, !gameStates.render.automap.bRadar);

if (gameStates.render.automap.bRadar && (OBJ_IDX (objP) != LOCALPLAYER.nObject))
	return;
// Draw shaft of arrow
vArrowPos = objP->info.position.vPos + objP->info.position.mOrient.FVec () * (size*3);
G3TransformAndEncodePoint(&arrowPoint, vArrowPos);
G3DrawLine (&spherePoint, &arrowPoint);

// Draw right head of arrow
vHeadPos = objP->info.position.vPos + objP->info.position.mOrient.FVec () * (size*2);
vHeadPos += objP->info.position.mOrient.RVec () * (size*1);
G3TransformAndEncodePoint(&headPoint, vHeadPos);
G3DrawLine (&arrowPoint, &headPoint);

// Draw left head of arrow
vHeadPos = objP->info.position.vPos + objP->info.position.mOrient.FVec () * (size*2);
vHeadPos += objP->info.position.mOrient.RVec () * (size* (-1));
G3TransformAndEncodePoint(&headPoint, vHeadPos);
G3DrawLine (&arrowPoint, &headPoint);

// Draw CPlayerData's up vector
vArrowPos = objP->info.position.vPos + objP->info.position.mOrient.UVec () * (size*2);
G3TransformAndEncodePoint(&arrowPoint, vArrowPos);
G3DrawLine (&spherePoint, &arrowPoint);
gameStates.ogl.bUseTransform = bUseTransform;
}

//------------------------------------------------------------------------------

#ifndef Pi
#  define  Pi    3.141592653589793240
#endif

int automap_width = 640;
int automap_height = 480;

#define RESCALE_X(x) ((x) * automap_width / 640)
#define RESCALE_Y(y) ((y) * automap_height / 480)

void DrawAutomap (void)
{
PROF_START
	int			i, color, size,
					bAutomapFrame = !gameStates.render.automap.bRadar && 
										 (gameStates.render.cockpit.nMode != CM_FULL_SCREEN) && 
										 (gameStates.render.cockpit.nMode != CM_LETTERBOX);
	CObject		*objP;
	g3sPoint		spherePoint;
	CFixMatrix	vmRadar;

gameStates.render.automap.bFull = (LOCALPLAYER.flags & (PLAYER_FLAGS_FULLMAP_CHEAT | PLAYER_FLAGS_FULLMAP)) != 0;
if (gameStates.render.automap.bRadar && gameStates.render.bTopDownRadar) {
	CFixMatrix& po = gameData.multiplayer.playerInit [gameData.multiplayer.nLocalPlayer].position.mOrient;
#if 1
	vmRadar.RVec () = po.RVec ();
	vmRadar.FVec () = po.UVec ();
	vmRadar.FVec ()[Y] = -vmRadar.FVec ()[Y];
	vmRadar.UVec () = po.FVec ();
#else
	vmRadar.rVec.p.x = po->rVec.p.x;
	vmRadar.rVec.p.y = po->rVec.p.y;
	vmRadar.rVec.p.z = po->rVec.p.z;
	vmRadar.fVec.p.x = po->uVec.p.x;
	vmRadar.fVec.p.y = -po->uVec.p.y;
	vmRadar.fVec.p.z = po->uVec.p.z;
	vmRadar.uVec.p.x = po->fVec.p.x;
	vmRadar.uVec.p.y = po->fVec.p.y;
	vmRadar.uVec.p.z = po->fVec.p.z;
#endif
	}
if (bAutomapFrame) {
	ShowFullscreenImage (&bmAutomapBackground);
	fontManager.SetCurrent (HUGE_FONT);
	fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	GrPrintF (NULL, RESCALE_X (80), RESCALE_Y (36), TXT_AUTOMAP, HUGE_FONT);
	fontManager.SetCurrent (SMALL_FONT);
	fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	GrPrintF (NULL, RESCALE_X (60), RESCALE_Y (
6), TXT_TURN_SHIP);
	GrPrintF (NULL, RESCALE_X (60), RESCALE_Y (443), TXT_SLIDE_UPDOWN);
	GrPrintF (NULL, RESCALE_X (60), RESCALE_Y (460), TXT_VIEWING_DISTANCE);
	//GrUpdate (0);
	}
G3StartFrame (gameStates.render.automap.bRadar || !gameOpts->render.automap.bTextured, 0); //!gameStates.render.automap.bRadar);
if (bAutomapFrame)
	OglViewport (RESCALE_X (27), RESCALE_Y (80), RESCALE_X (582), RESCALE_Y (334));
RenderStartFrame ();
if (gameStates.render.automap.bRadar && gameStates.render.bTopDownRadar) {
	amData.viewPos = amData.viewTarget + vmRadar.FVec () * (-amData.nViewDist);
	G3SetViewMatrix(amData.viewPos, vmRadar, amData.nZoom * 2, 1);
	}
else {
	amData.viewPos = amData.viewTarget + amData.viewMatrix.FVec () * (gameStates.render.automap.bRadar ? -amData.nViewDist : -amData.nViewDist);
	G3SetViewMatrix(amData.viewPos, amData.viewMatrix, gameStates.render.automap.bRadar ? (amData.nZoom * 3) / 2 : amData.nZoom, 1);
	}
if (!gameStates.render.automap.bRadar && gameOpts->render.automap.bTextured) {
	gameData.render.mine.viewerEye = amData.viewPos;
	RenderMine (gameData.objs.consoleP->info.nSegment, 0, 0);
	RenderEffects (0);
	}
else
	DrawAllEdges ();
	// Draw player...
color = IsTeamGame ? GetTeam (gameData.multiplayer.nLocalPlayer) : gameData.multiplayer.nLocalPlayer;	// Note link to above if!
CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (playerColors [color].r, playerColors [color].g, playerColors [color].b));

if (!gameOpts->render.automap.bTextured || gameStates.render.automap.bRadar) {
	DrawPlayer (OBJECTS + LOCALPLAYER.nObject);
	if (!gameStates.render.automap.bRadar) {
		DrawMarkers ();
		if ((gameData.marker.nHighlight > -1) && (gameData.marker.szMessage [gameData.marker.nHighlight][0] != 0)) {
			char msg [10 + MARKER_MESSAGE_LEN + 1];
			sprintf (msg, TXT_MARKER_MSG, gameData.marker.nHighlight + 1,
						gameData.marker.szMessage [(gameData.multiplayer.nLocalPlayer * 2) + gameData.marker.nHighlight]);
			CCanvas::Current ()->SetColorRGB (196, 0, 0, 255);
			ModexPrintF (5,20,msg,SMALL_FONT, automapColors.nDkGray);
			}
		}			
	// Draw CPlayerData (s)...
	if (AM_SHOW_PLAYERS) {
		for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
			if ((i != gameData.multiplayer.nLocalPlayer) && AM_SHOW_PLAYER (i)) {
				if (OBJECTS [gameData.multiplayer.players [i].nObject].info.nType == OBJ_PLAYER) {
					color = (gameData.app.nGameMode & GM_TEAM) ? GetTeam (i) : i;
					CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (playerColors [color].r, playerColors [color].g, playerColors [color].b));
					DrawPlayer (OBJECTS + gameData.multiplayer.players [i].nObject);
					}
				}
			}
		}
	objP = OBJECTS.Buffer ();
	FORALL_OBJS (objP, i) {
		size = objP->info.xSize;
		switch (objP->info.nType) {
			case OBJ_HOSTAGE:
				CCanvas::Current ()->SetColorRGBi (automapColors.nHostage);
				G3TransformAndEncodePoint(&spherePoint, objP->info.position.vPos);
				G3DrawSphere (&spherePoint,size, !gameStates.render.automap.bRadar);
				break;

			case OBJ_MONSTERBALL:
				CCanvas::Current ()->SetColorRGBi (automapColors.nMonsterball);
				G3TransformAndEncodePoint(&spherePoint, objP->info.position.vPos);
				G3DrawSphere (&spherePoint,size, !gameStates.render.automap.bRadar);
				break;

			case OBJ_ROBOT:
				if (gameData.render.mine.bAutomapVisited [objP->info.nSegment] && AM_SHOW_ROBOTS) {
					static int c = 0;
					static int t = 0;
					int h = SDL_GetTicks ();
					if (h - t > 250) {
						t = h;
						c = !c;
						}
					if (ROBOTINFO (objP->info.nId).companion)
						if (c)
							CCanvas::Current ()->SetColorRGB (0, 123, 151, 255); //gr_getcolor (47, 1, 47)); 
						else
							CCanvas::Current ()->SetColorRGB (0, 78, 112, 255); //gr_getcolor (47, 1, 47)); 
					else
						if (c)
							CCanvas::Current ()->SetColorRGB (123, 0, 135, 255); //gr_getcolor (47, 1, 47)); 
						else
							CCanvas::Current ()->SetColorRGB (78, 0, 96, 255); //gr_getcolor (47, 1, 47)); 
					G3TransformAndEncodePoint(&spherePoint, objP->info.position.vPos);
					//transformation.Begin (&objP->info.position.vPos, &objP->info.position.mOrient);
					G3DrawSphere (&spherePoint, (size * 3) / 2, !gameStates.render.automap.bRadar);
					//transformation.End ();
					}
				break;

			case OBJ_POWERUP:
				if (AM_SHOW_POWERUPS (1) && 
					(gameStates.render.bAllVisited || gameData.render.mine.bAutomapVisited [objP->info.nSegment]))	{
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
					G3TransformAndEncodePoint(&spherePoint, objP->info.position.vPos);
					G3DrawSphere (&spherePoint, size, !gameStates.render.automap.bRadar);
					}
				break;
			}
		}
	}
G3EndFrame ();
gameData.app.nFrameCount++;
if (gameStates.render.automap.bRadar) {
	gameStates.ogl.bEnableScissor = 0;
	return;
	}
if (gameStates.app.bNostalgia || gameOpts->render.cockpit.bHUD) {
	int offs = amData.bHires ? 10 : 5;

#if 1
	CFont	*curFont = CCanvas::Current ()->Font ();
	int		w, h, aw;

	fontManager.SetCurrent (SMALL_FONT);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	GrPrintF (NULL, offs, offs, amLevelNum);
	fontManager.Current ()->StringSize (amLevelName, w, h, aw);
	GrPrintF (NULL, CCanvas::Current ()->Width () - offs - w, offs, amLevelName);
	fontManager.SetCurrent (curFont);
#else
	GrBitmapM (offs, offs, &levelNumCanv->Bitmap (), 2);
	GrBitmapM (CCanvas::Current ()->Width () - offs - levelNameCanv->Width (), 
				  offs, &levelNameCanv->Bitmap (), 2);
#endif
	if (gameOpts->render.automap.bTextured)
		ShowFrameRate ();
	}
PROF_END(ptFrame)
OglSwapBuffers (0, 0);
}

#define LEAVE_TIME 0x4000

//------------------------------------------------------------------------------

//print to canvas & float height
CCanvas* PrintToCanvas (char *s, CFont *font, uint fc, uint bc, int doubleFlag)
{
	int		y;
	ubyte		*data;
	int		rs;
	CCanvas	*canvP;
	int		w,h,aw;

CCanvas::Push ();
fontManager.SetCurrent (font);					//set the font we're going to use
fontManager.Current ()->StringSize (s, w, h, aw);		//now get the string size

//canvP = GrCreateCanvas (font->width*strlen (s),font->height*2);
canvP = CCanvas::Create (w, font->Height () * 2);
canvP->SetPalette (paletteManager.Game ());
CCanvas::SetCurrent (canvP);
fontManager.SetCurrent (font);
canvP->Clear (0);						//trans color
fontManager.SetColorRGBi (fc, 1, bc, 1);
GrPrintF (NULL, 0, 0, s);
//now float it, since we're drawing to 400-line modex screen
if (doubleFlag) {
	data = canvP->Buffer ();
	rs = canvP->RowSize ();

	for (y=canvP->Height () / 2;y--;) {
		memcpy (data + rs * y * 2, data+ rs * y, canvP->Width ());
		memcpy (data + rs * (y * 2 + 1), data + rs *y, canvP->Width ());
		}
	}
CCanvas::Pop ();
return canvP;
}

//------------------------------------------------------------------------------
//print to buffer, float heights, and blit bitmap to screen
void ModexPrintF (int x,int y,char *s,CFont *font, uint color)
{
	CCanvas *canvP = PrintToCanvas (s, font, color, 0, !amData.bHires);

GrBitmapM (x, y, canvP, 2);
canvP->Destroy ();
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

void CreateNameCanv (void)
{
	char	szExplored [100];
	int	h, i;

if (gameData.missions.nCurrentLevel > 0)
	sprintf (amLevelNum, "%s %i",TXT_LEVEL, gameData.missions.nCurrentLevel);
else
	sprintf (amLevelNum, "Secret Level %i", -gameData.missions.nCurrentLevel);
if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) && 
		(gameData.missions.nCurrentLevel > 0))		//built-in mission
	sprintf (amLevelName,"%s %d: ",
				pszSystemNames [(gameData.missions.nCurrentLevel - 1) / 4], 
				((gameData.missions.nCurrentLevel - 1) % 4) + 1);
else
	strcpy (amLevelName, " ");
strcat (amLevelName, gameData.missions.szCurrentLevel);
for (h = i = 0; i < gameData.segs.nSegments; i++)
	if (gameData.render.mine.bAutomapVisited [i])
		h++;
sprintf (szExplored, " (%1.1f %s)", (float) (h * 100) / (float) gameData.segs.nSegments, TXT_PERCENT_EXPLORED);
strcat (amLevelName, szExplored);
#if 0
levelNumCanv = PrintToCanvas (amLevelNum, SMALL_FONT, automapColors.nMedGreen, 0, !amData.bHires);
levelNameCanv = PrintToCanvas (amLevelName, SMALL_FONT, automapColors.nMedGreen, 0, !amData.bHires);
#endif
}

//------------------------------------------------------------------------------

int SetSegmentDepths (int start_seg, ushort *pDepthBuf);

#if !DBG
const char	*pszMapBackgroundFilename [2] = {"\x01MAP.PCX", "\x01MAPB.PCX"};

#	define MAP_BACKGROUND_FILENAME pszMapBackgroundFilename [amData.bHires]
#else
char	*pszMapBackgroundFilename [2] = {"MAP.PCX", "MAPB.PCX"};

#	define MAP_BACKGROUND_FILENAME pszMapBackgroundFilename [amData.bHires && CFile::Exist ("mapb.pcx",gameFolders.szDataDir,0)]
#endif


int InitAutomap (int bPauseGame, fix *pxEntryTime, CAngleVector& pvTAngles)
{
		int		i, nPCXError;
		fix		t1, t2;
		CObject	*playerP;

gameStates.render.automap.bDisplay = 1;
gameStates.ogl.nContrast = 8;
InitAutomapColors ();
if (!gameStates.render.automap.bRadar)
	SlowMotionOff ();
if (gameStates.render.automap.bRadar || 
	 (IsMultiGame && 
	  (gameStates.app.nFunctionMode == FMODE_GAME) && 
	  (!gameStates.app.bEndLevelSequence)))
	bPauseGame = 0;
if (bPauseGame)
	PauseGame ();
nMaxEdges = MAX_EDGES; //min (MAX_EDGES_FROM_VERTS (gameData.segs.nVertices), MAX_EDGES);			//make maybe smaller than max
if (gameStates.render.automap.bRadar || (gameStates.video.nDisplayMode > 1)) {
	//GrSetMode (gameStates.video.nLastScreenMode);
	if (gameStates.render.automap.bRadar) {
		automap_width = CCanvas::Current ()->Width ();
		automap_height = CCanvas::Current ()->Height ();
		}
	else {
		automap_width = screen.Canvas ()->Width ();
		automap_height = screen.Canvas ()->Height ();
		}
	amData.bHires = 1;
	 }
else {
	GrSetMode (SM (320, 400));
	amData.bHires = 0;
	}
gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && amData.bHires;
if (!gameStates.render.automap.bRadar) {
	CreateNameCanv ();
	paletteManager.ResetEffect ();
	}
if (!gameStates.render.automap.bRadar) {
	bmAutomapBackground.Init ();
	nPCXError = PCXReadBitmap (MAP_BACKGROUND_FILENAME, &bmAutomapBackground, BM_LINEAR, 0);
	if (nPCXError != PCX_ERROR_NONE)
		Error ("File %s - PCX error: %s", MAP_BACKGROUND_FILENAME, pcx_errormsg (nPCXError));
	bmAutomapBackground.Remap (NULL, -1, -1);
	}
if (gameStates.render.automap.bRadar || !gameOpts->render.automap.bTextured)
	AutomapBuildEdgeList ();
if (gameStates.render.automap.bRadar)
	amData.nViewDist = ZOOM_DEFAULT;
else if (!amData.nViewDist)
	amData.nViewDist = ZOOM_DEFAULT;
playerP = OBJECTS + LOCALPLAYER.nObject;
amData.viewMatrix = playerP->info.position.mOrient;

pvTAngles[PA] = PITCH_DEFAULT;
pvTAngles[HA] = 0;
pvTAngles[BA] = 0;

amData.viewTarget = playerP->info.position.vPos;
t1 = *pxEntryTime = TimerGetFixedSeconds ();
t2 = t1;
//Fill in gameData.render.mine.bAutomapVisited from OBJECTS [LOCALPLAYER.nObject].nSegment
if (gameStates.render.automap.bRadar) {
	for (i = 0; i < gameData.segs.nSegments; i++)
		gameData.render.mine.bAutomapVisible [i] = 1;
	}
else if (gameStates.render.automap.bFull) {
	for (i = 0; i < gameData.segs.nSegments; i++)
		gameData.render.mine.bAutomapVisible [i] = 1;
	}
else
	memcpy (gameData.render.mine.bAutomapVisible, 
			  gameData.render.mine.bAutomapVisited, 
			  sizeof (gameData.render.mine.bAutomapVisited));
//gameData.render.mine.bAutomapVisited [OBJECTS [LOCALPLAYER.nObject].nSegment] = 1;
gameStates.render.automap.nSegmentLimit =
gameStates.render.automap.nMaxSegsAway = 
	SetSegmentDepths (OBJECTS [LOCALPLAYER.nObject].info.nSegment, gameData.render.mine.bAutomapVisible);
AdjustSegmentLimit (gameStates.render.automap.nSegmentLimit, gameData.render.mine.bAutomapVisible);
return bPauseGame;
}

//------------------------------------------------------------------------------

int UpdateAutomap (CAngleVector& pvTAngles)
{
	CObject		*playerP = OBJECTS + LOCALPLAYER.nObject;
	CFixMatrix	m;

if (Controls [0].firePrimaryDownCount)	{
	// Reset orientation
	amData.nViewDist = ZOOM_DEFAULT;
	pvTAngles[PA] = PITCH_DEFAULT;
	pvTAngles[HA] = 0;
	pvTAngles[BA] = 0;
	amData.viewTarget = playerP->info.position.vPos;
	}
if (Controls [0].forwardThrustTime)
	amData.viewTarget += amData.viewMatrix.FVec () * (Controls [0].forwardThrustTime * ZOOM_SPEED_FACTOR); 
pvTAngles[PA] += (fixang) FixDiv (Controls [0].pitchTime, ROT_SPEED_DIVISOR);
pvTAngles[HA] += (fixang) FixDiv (Controls [0].headingTime, ROT_SPEED_DIVISOR);
pvTAngles[BA] += (fixang) FixDiv (Controls [0].bankTime, ROT_SPEED_DIVISOR*2);

m = CFixMatrix::Create(pvTAngles);
if (Controls [0].verticalThrustTime || Controls [0].sidewaysThrustTime)	{
	// TODO MM
	amData.viewMatrix = playerP->info.position.mOrient * m;
	amData.viewTarget += amData.viewMatrix.UVec () * (Controls [0].verticalThrustTime * SLIDE_SPEED);
	amData.viewTarget += amData.viewMatrix.RVec () * (Controls [0].sidewaysThrustTime * SLIDE_SPEED);
	}
// TODO MM
amData.viewMatrix = playerP->info.position.mOrient * m;
if (amData.nViewDist < ZOOM_MIN_VALUE) 
	amData.nViewDist = ZOOM_MIN_VALUE;
if (amData.nViewDist > ZOOM_MAX_VALUE) 
	amData.nViewDist = ZOOM_MAX_VALUE;
return 1;
}

//------------------------------------------------------------------------------

static inline int ViewDistStep (void)
{
	int h = (gameStates.render.automap.nSegmentLimit + 5) / 10;

return h ? h : 1;
}

//------------------------------------------------------------------------------

int ReadAutomapControls (int nLeaveMode, int bDone, int *pbPauseGame)
{
	int	c, nMarker, nMaxDrop;

ControlsReadAll ();	
if (Controls [0].automapDownCount && !nLeaveMode)
	return 1;
while ((c = KeyInKey ())) {
	if (!gameOpts->menus.nStyle)
		MultiDoFrame();
		switch (c) {
#if DBG
		case KEY_BACKSP: Int3 (); 
			break;
#endif
		case KEY_CTRLED + KEY_P:
			if (gameOpts->menus.nStyle && !IsMultiGame) {
				if (gameData.app.bGamePaused)
					ResumeGame ();
				else
					PauseGame ();
				}
			*pbPauseGame = gameData.app.bGamePaused;
			break;

		case KEY_PRINT_SCREEN: {
			if (amData.bHires)
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
				gameData.render.mine.bAutomapVisible [i] = 1;
			AutomapBuildEdgeList ();
			gameStates.render.automap.nSegmentLimit = 
			gameStates.render.automap.nMaxSegsAway = 
				SetSegmentDepths (OBJECTS [LOCALPLAYER.nObject].info.nSegment, gameData.render.mine.bAutomapVisible);
			AdjustSegmentLimit (gameStates.render.automap.nSegmentLimit, gameData.render.mine.bAutomapVisible);
			}
			break;
#endif

		case KEY_MINUS:
		case KEY_PADMINUS:
			if (gameStates.render.automap.nSegmentLimit > 1)  {
				gameStates.render.automap.nSegmentLimit -= ViewDistStep ();
				if (!gameStates.render.automap.nSegmentLimit)
					gameStates.render.automap.nSegmentLimit = 1;
				AdjustSegmentLimit (gameStates.render.automap.nSegmentLimit, gameData.render.mine.bAutomapVisible);
				}
			break;

		case KEY_EQUAL:
		case KEY_PADPLUS:
			if (gameStates.render.automap.nSegmentLimit < gameStates.render.automap.nMaxSegsAway) {
				gameStates.render.automap.nSegmentLimit += ViewDistStep ();
				if (gameStates.render.automap.nSegmentLimit > gameStates.render.automap.nMaxSegsAway)
					gameStates.render.automap.nSegmentLimit = gameStates.render.automap.nMaxSegsAway;
				AdjustSegmentLimit (gameStates.render.automap.nSegmentLimit, gameData.render.mine.bAutomapVisible);
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

		case KEY_D+KEY_CTRLED:
			DeleteMarker (0);
			break;

		case KEY_T+KEY_CTRLED:
			TeleportToMarker ();
			break;

		case KEY_ALTED + KEY_P:
			gameStates.render.bShowProfiler = !gameStates.render.bShowProfiler;
			break;

		case KEY_ALTED + KEY_R:
			gameStates.render.bShowFrameRate = ++gameStates.render.bShowFrameRate % (6 + (gameStates.render.bPerPixelLighting == 2));
			break;

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

int AMGameFrame (int bPauseGame, int bDone)
{
	tControlInfo controlInfoSave;

if (!bPauseGame)	{
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

void DoAutomap (int nKeyCode, int bRadar)
{
	int			bDone = 0;
	CAngleVector	vTAngles;
	int			nLeaveMode = 0;
	int			bFirstTime = 1;
	fix			xEntryTime;
	int			bPauseGame = (gameOpts->menus.nStyle == 0);		// Set to 1 if everything is paused during automap...No pause during net.
	fix			t1 = 0, t2 = 0;
	int			nContrast = gameStates.ogl.nContrast;
	int			bRedrawScreen = 0;

	//static ubyte	automapPal [256*3];

gameStates.render.automap.nMaxSegsAway = 0;
gameStates.render.automap.nSegmentLimit = 1;
gameStates.render.automap.bRadar = bRadar;
bPauseGame = InitAutomap (bPauseGame, &xEntryTime, vTAngles);
bRedrawScreen = 0;
if (bRadar) {
	DrawAutomap ();
	gameStates.ogl.nContrast = nContrast;
	gameStates.render.automap.bDisplay = 0;
	return;
	}
Controls [0].automapState = 0;
GetSlowTicks ();
while (!bDone)	{
	if (!nLeaveMode && Controls [0].automapState && ((TimerGetFixedSeconds ()- xEntryTime) > LEAVE_TIME))
		nLeaveMode = 1;
	if (!Controls [0].automapState && (nLeaveMode == 1))
		bDone = 1;
	bDone = AMGameFrame (bPauseGame, bDone);
	SongsCheckRedbookRepeat ();
	bDone = ReadAutomapControls (nLeaveMode, bDone, &bPauseGame);
	UpdateAutomap(vTAngles);
	DrawAutomap ();
	if (bFirstTime) {
		bFirstTime = 0;
		paletteManager.LoadEffect ();
		}
	t2 = TimerGetFixedSeconds ();
	if (bPauseGame)
		gameData.time.xFrame = t2 - t1;
	t1 = t2;
	}
#if 0
GrFreeCanvas (levelNumCanv);  
levelNumCanv = NULL;
GrFreeCanvas (levelNameCanv);  
levelNameCanv = NULL;
#endif
GameFlushInputs ();
if (gameData.app.bGamePaused)
	ResumeGame ();
gameStates.ogl.nContrast = nContrast;
gameStates.render.automap.bDisplay = 0;
}

//------------------------------------------------------------------------------

void AdjustSegmentLimit (int nSegmentLimit, ushort *pVisited)
{
	int i,e1;
	tEdgeInfo * e;

for (i = 0; i <= nHighestEdgeIndex; i++)	{
	e = Edges + i;
	e->flags |= EF_TOO_FAR;
	for (e1 = 0; e1 < e->num_faces; e1++)	{
		if (pVisited [e->nSegment [e1]] <= nSegmentLimit)	{
			e->flags &= ~EF_TOO_FAR;
			break;
			}
		}
	}
}

//------------------------------------------------------------------------------

void DrawAllEdges (void)
{
	g3sCodes		cc;
	int			i, j, nbright = 0;
	ubyte			nfacing, nnfacing;
	tEdgeInfo	*e;
	CFixVector	*tv1;
	fix			distance;
	fix			minDistance = 0x7fffffff;
	g3sPoint		*p1, *p2;
	int			bUseTransform = gameStates.ogl.bUseTransform;

gameStates.ogl.bUseTransform = RENDERPATH;
for (i = 0; i <= nHighestEdgeIndex; i++)	{
	//e = &Edges [Edge_used_list [i]];
	e = Edges + i;
	if (!(e->flags & EF_USED)) 
		continue;
	if (e->flags & EF_TOO_FAR) 
		continue;
	if (e->flags & EF_FRONTIER) {		// A line that is between what we have seen and what we haven't
		if ((!(e->flags & EF_SECRET)) && (e->color == automapColors.walls.nNormal))
			continue;		// If a line isn't secret and is Normal color, then don't draw it
		}

	cc = RotateVertexList (2,e->verts);
	distance = gameData.segs.points [e->verts [1]].p3_vec[Z];
	if (minDistance>distance)
		minDistance = distance;
	if (!cc.ccAnd) 	{	//all off screen?
		nfacing = nnfacing = 0;
		tv1 = gameData.segs.vertices + e->verts [0];
		j = 0;
		while (j<e->num_faces && (nfacing==0 || nnfacing==0))	{
			if (!G3CheckNormalFacing (*tv1, SEGMENTS [e->nSegment [j]].m_sides [e->sides [j]].m_normals [0]))
				nfacing++;
			else
				nnfacing++;
			j++;
			}

		if (nfacing && nnfacing) {
			// a contour line
			DrawingListBright [nbright++] = EDGE_IDX (e);
			}
		else if (e->flags & (EF_DEFINING|EF_GRATE))	{
			if (nfacing == 0)	{
				if (e->flags & EF_NO_FADE)
					CCanvas::Current ()->SetColorRGBi (e->color);
				else
					CCanvas::Current ()->SetColorRGBi (RGBA_FADE (e->color, 32.0 / 8.0));
				G3DrawLine (gameData.segs.points + e->verts [0], gameData.segs.points + e->verts [1]);
				}
			else {
				DrawingListBright [nbright++] = EDGE_IDX (e);
				}
			}
		}
	}

if (minDistance < 0) 
	minDistance = 0;

// Sort the bright ones using a shell sort
{
	int t;
	int i, j, incr, v1, v2;

incr = nbright / 2;
while (incr > 0) {
	for (i = incr; i < nbright; i++) {
		j = i - incr;
		while (j>=0) {
			// compare element j and j+incr
			v1 = Edges [DrawingListBright [j]].verts [0];
			v2 = Edges [DrawingListBright [j+incr]].verts [0];

			if (gameData.segs.points [v1].p3_vec[Z] < gameData.segs.points [v2].p3_vec[Z]) {
				// If not in correct order, them swap 'em
				t = DrawingListBright [j+incr];
				DrawingListBright [j+incr] = DrawingListBright [j];
				DrawingListBright [j] = t;
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
	e = Edges + DrawingListBright [i];
	p1 = gameData.segs.points + e->verts [0];
	p2 = gameData.segs.points + e->verts [1];
	dist = p1->p3_vec[Z] - minDistance;
	// Make distance be 1.0 to 0.0, where 0.0 is 10 segments away;
	if (dist < 0) 
		dist = 0;
	if (dist >= amData.nMaxDist) 
		continue;

	if (e->flags & EF_NO_FADE)
		CCanvas::Current ()->SetColorRGBi (e->color);
	else {
		dist = F1_0 - FixDiv (dist, amData.nMaxDist);
		color = X2I (dist*31);
		CCanvas::Current ()->SetColorRGBi (RGBA_FADE (e->color, 32.0 / color));
		}
	G3DrawLine (p1, p2);
	}
gameStates.ogl.bUseTransform = bUseTransform;
}


//==================================================================
//
// All routines below here are used to build the Edge list
//
//==================================================================


//finds edge, filling in edge_ptr. if found old edge, returns index, else return -1
static int AutomapFindEdge (int v0, int v1, tEdgeInfo **edge_ptr)
{
	int vv, evv;
	int hash,oldhash;
	int ret, ev0, ev1;

vv = (v1<<16) + v0;
oldhash = hash = ((v0*5+v1) % nMaxEdges);
ret = -1;
while (ret==-1) {
	ev0 = (int) (Edges [hash].verts [0]);
	ev1 = (int) (Edges [hash].verts [1]);
	evv = (ev1<<16)+ev0;
	if (Edges [hash].num_faces == 0) ret=0;
	else if (evv == vv) ret=1;
	else {
		if (++hash==nMaxEdges) hash=0;
		if (hash==oldhash) Error ("Edge list full!");
		}
	}
*edge_ptr = &Edges [hash];
return ret ? hash : -1;
}

//------------------------------------------------------------------------------

void AddOneEdge (int va, int vb, uint color, ubyte CSide, short nSegment, 
					  int bHidden, int bGrate, int bNoFade)
{
	int found;
	tEdgeInfo *e;
	int tmp;

	if (nNumEdges >= nMaxEdges) {
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
found = AutomapFindEdge (va,vb,&e);

if (found == -1) {
	e->verts [0] = va;
	e->verts [1] = vb;
	e->color = color;
	e->num_faces = 1;
	e->flags = EF_USED | EF_DEFINING;			// Assume a Normal line
	e->sides [0] = CSide;
	e->nSegment [0] = nSegment;
	//Edge_used_list [nNumEdges] = EDGE_IDX (e);
	if ( EDGE_IDX (e) > nHighestEdgeIndex)
		nHighestEdgeIndex = EDGE_IDX (e);
	nNumEdges++;
	} 
else {
	//Assert (e->num_faces < 8);
	if ((color != automapColors.walls.nNormal) && (color != automapColors.walls.nRevealed))
		e->color = color;
	if (e->num_faces < 4) {
		e->sides [e->num_faces] = CSide;
		e->nSegment [e->num_faces] = nSegment;
		e->num_faces++;
		}
	}
if (bGrate)
	e->flags |= EF_GRATE;
if (bHidden)
	e->flags |= EF_SECRET;		// Mark this as a bHidden edge
if (bNoFade)
	e->flags |= EF_NO_FADE;
}

//------------------------------------------------------------------------------

void AddOneUnknownEdge (int va, int vb)
{
	int found;
	tEdgeInfo *e;
	int tmp;

if (va > vb) {
	tmp = va;
	va = vb;
	vb = tmp;
	}

found = AutomapFindEdge (va,vb,&e);
if (found != -1)
	e->flags |= EF_FRONTIER;		// Mark as a border edge
}

//------------------------------------------------------------------------------

void AddSegmentEdges (CSegment *segP)
{
	int		 		bIsGrate, bNoFade;
	uint				color;
	int				nWall;
	ubyte				nSide;
	short				nSegment = SEG_IDX (segP);
	int				bHidden;
	int				ttype, nTrigger;
	ushort*			contour;

for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	bHidden = 0;
	bIsGrate = 0;
	bNoFade = 0;
	color = WHITE_RGBA;
	if (segP->m_children [nSide] == -1)
		color = automapColors.walls.nNormal;
	switch (SEGMENTS [nSegment].m_nType)	{
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
		nTrigger = wallP->nTrigger;
		ttype = TRIGGERS [nTrigger].nType;
		if (ttype==TT_SECRET_EXIT)	{
	 		color = RGBA_PAL2 (29, 0, 31);
			bNoFade=1;
			goto addEdge;
			}
		switch (wallP->nType) {
			case WALL_DOOR:
				if (wallP->keys == KEY_BLUE) {
					bNoFade = 1;
					color = automapColors.walls.nDoorBlue;
					}
				else if (wallP->keys == KEY_GOLD) {
					bNoFade = 1;
					color = automapColors.walls.nDoorGold;
					}
				else if (wallP->keys == KEY_RED) {
					bNoFade = 1;
					color = automapColors.walls.nDoorRed;
					}
				else if (!(gameData.walls.animP [wallP->nClip].flags & WCF_HIDDEN)) {
					short	nConnSeg = segP->m_children [nSide];
					if (nConnSeg != -1) {
						short nConnSide = segP->ConnectedSide (&SEGMENTS [nConnSeg]);
						switch (WALLS [WallNumI (nConnSeg, nConnSide)].keys) {
							case KEY_BLUE:
								color = automapColors.walls.nDoorBlue;
								bNoFade = 1; 
								break;
							case KEY_GOLD:
								color = automapColors.walls.nDoorGold;
								bNoFade = 1; 
								break;
							case KEY_RED:
								color = automapColors.walls.nDoorRed;
								bNoFade = 1; 
								break;
							default:
								color = automapColors.walls.nDoor;
							}
						}
					}
				else {
					color = automapColors.walls.nNormal;
					bHidden = 1;
					}
				break;
			case WALL_CLOSED:
				// Make bGrates draw properly
				if (segP->IsDoorWay (nSide, NULL) & WID_RENDPAST_FLAG)
					bIsGrate = 1;
				else
					bHidden = 1;
				color = automapColors.walls.nNormal;
				break;
			case WALL_BLASTABLE:
				// Hostage doors
				color = automapColors.walls.nDoor;
				break;
			}
		}

	if (nSegment == gameData.multiplayer.playerInit [gameData.multiplayer.nLocalPlayer].nSegment)
		color = RGBA_PAL2 (31,0,31);

	if (color != WHITE_RGBA) {
		// If they have a map powerup, draw unvisited areas in dark blue.
		if ((LOCALPLAYER.flags & PLAYER_FLAGS_FULLMAP) && 
				!(gameStates.render.bAllVisited || gameData.render.mine.bAutomapVisited [nSegment]))
			color = automapColors.walls.nRevealed;

addEdge:

		contour = SEGMENTS [nSegment].Contour (nSide);
		AddOneEdge (contour [0], contour [1], color, nSide, nSegment, bHidden, 0, bNoFade);
		AddOneEdge (contour [1], contour [2], color, nSide, nSegment, bHidden, 0, bNoFade);
		AddOneEdge (contour [2], contour [3], color, nSide, nSegment, bHidden, 0, bNoFade);
		AddOneEdge (contour [3], contour [0], color, nSide, nSegment, bHidden, 0, bNoFade);

		if (bIsGrate) {
			AddOneEdge (contour [0], contour [2], color, nSide, nSegment, bHidden, 1, bNoFade);
			AddOneEdge (contour [1], contour [3], color, nSide, nSegment, bHidden, 1, bNoFade);
			}
		}
	}
}

//------------------------------------------------------------------------------
// Adds all the edges from a CSegment we haven't visited yet.

void AddUnknownSegmentEdges (CSegment* segP)
{
for (int nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	;

	// Only add edges that have no children
	if (segP->m_children [nSide] == -1) {
		ushort* contour = segP->Contour (nSide);
		AddOneUnknownEdge (contour [0], contour [1]);
		AddOneUnknownEdge (contour [1], contour [2]);
		AddOneUnknownEdge (contour [2], contour [3]);
		AddOneUnknownEdge (contour [3], contour [0]);
		}
	}
}

//------------------------------------------------------------------------------

void AutomapBuildEdgeList (void)
{
	int	h = 0, i, e1, e2, s;
	tEdgeInfo * e;

amData.bCheat = 0;
if (LOCALPLAYER.flags & PLAYER_FLAGS_FULLMAP_CHEAT)
	amData.bCheat = 1;		// Damn cheaters...

	// clear edge list
for (i=0; i < nMaxEdges; i++) {
	Edges [i].num_faces = 0;
	Edges [i].flags = 0;
	}
nNumEdges = 0;
nHighestEdgeIndex = -1;

if (amData.bCheat || (LOCALPLAYER.flags & PLAYER_FLAGS_FULLMAP))	{
	// Cheating, add all edges as visited
	for (s=0; s<=gameData.segs.nLastSegment; s++)
#ifdef EDITOR
		if (SEGMENTS [s].nSegment != -1)
#endif
			{
			AddSegmentEdges (&SEGMENTS [s]);
			}
	} 
else {
	// Not cheating, add visited edges, and then unvisited edges
	for (s=0; s<=gameData.segs.nLastSegment; s++)
#ifdef EDITOR
		if (SEGMENTS [s].nSegment != -1)
#endif
		if (gameData.render.mine.bAutomapVisited [s]) {
			h++;
			AddSegmentEdges (&SEGMENTS [s]);
			}
		for (s=0; s<=gameData.segs.nLastSegment; s++)
#ifdef EDITOR
			if (SEGMENTS [s].nSegment != -1)
#endif
			if (!gameData.render.mine.bAutomapVisited [s]) {
				AddUnknownSegmentEdges (&SEGMENTS [s]);
				}
		}
	// Find unnecessary lines (These are lines that don't have to be drawn because they have small curvature)
	for (i = 0; i <= nHighestEdgeIndex; i++)	{
		e = Edges + i;
		if (!(e->flags & EF_USED))
			continue;

		for (e1 = 0; e1 < e->num_faces; e1++) {
			for (e2 = 1; e2 < e->num_faces; e2++) {
				if ((e1 != e2) && (e->nSegment [e1] != e->nSegment [e2]))	{
					if (CFixVector::Dot (SEGMENTS [e->nSegment [e1]].m_sides [e->sides [e1]].m_normals [0], SEGMENTS [e->nSegment [e2]].m_sides [e->sides [e2]].m_normals [0]) > (F1_0- (F1_0/10)) )	{
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
