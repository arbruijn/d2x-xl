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
#	include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef __unix__
#	include <strings.h>
#else
#	include <string.h>
#endif

#include "inferno.h"
#include "error.h"
#include "key.h"
#include "text.h"
#include "strutil.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "newmenu.h"
#include "marker.h"

// -------------------------------------------------------------

static inline CObject *MarkerObj (int nPlayer, int nMarker)
{
short nObject = gameData.marker.objects [((nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer) * 2 + nMarker];
return (nObject < 0) ? NULL : OBJECTS + nObject;
}

// -------------------------------------------------------------

void DrawMarkerNumber (int nMarker)
{
	g3sPoint	basePoint;
	int		i;
	float		*px, *py;

	static float xCoord [9][10] =
		{{-0.25f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
       {-1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f},
       {-1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f},
       {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
       {-1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f},
       {-1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f},
       {-1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
       {-1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f},
       {-1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f}
      };

	static float yCoord [9][10] =
		{{0.75f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
       {1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, -1.0f, -1.0f},
       {1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
       {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
       {1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, -1.0f, -1.0f},
       {1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f},
       {1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
       {1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f},
       {1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f}
      };

  int nPoints [] = {6, 10, 8, 6, 10, 10, 4, 10, 8};

if (gameData.marker.fScale == 0.0f)
	gameData.marker.fScale = 2.0f;
if (nMarker == gameData.marker.nHighlight)
	CCanvas::Current ()->SetColorRGB (255, 255, 255, 255);
else if (!strcmp (gameData.marker.szMessage [nMarker], "SPAWN"))
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (63, 0, 47));
else
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (63, 15, 0));
G3TransformAndEncodePoint (&basePoint, MarkerObj (-1, nMarker)->info.position.vPos);
glPushMatrix ();
glTranslatef (X2F (basePoint.p3_vec[X]), X2F (basePoint.p3_vec[Y]), X2F (basePoint.p3_vec[Z]));
glDisable (GL_TEXTURE_2D);
OglCanvasColor (&CCanvas::Current ()->Color ());
glBegin (GL_LINES);
px = xCoord [nMarker];
py = yCoord [nMarker];
for (i = nPoints [nMarker] / 2; i; i--) {
	glVertex2f (*px++ * gameData.marker.fScale, *py++ * gameData.marker.fScale);
	glVertex2f (*px++ * gameData.marker.fScale, *py++ * gameData.marker.fScale);
	}
glEnd ();
glPopMatrix ();
}

//------------------------------------------------------------------------------

#define MARKER_SPHERE_SIZE 0x58000

void DrawMarkers (void)
{
	g3sPoint spherePoint;
	int		i, j, nMaxDrop, bSpawn;
	CObject	*objP;

	static int cyc = 10, cycdir = 1;
	static ubyte	colors [2][3] = {{20, 30, 40},{40, 50, 60}};

if (gameData.marker.fScale == 0.0f)
	gameData.marker.fScale = 2.0f;
nMaxDrop = MaxDrop ();
spherePoint.p3_index = -1;
for (i = 0; i < nMaxDrop; i++)
	if ((objP = MarkerObj (-1, i))) {
		bSpawn = (objP == SpawnMarkerObject (-1));
		G3TransformAndEncodePoint(&spherePoint, objP->info.position.vPos);
		for (j = 0; j < 3; j++) {
			CCanvas::Current ()->SetColorRGB (PAL2RGBA (colors [bSpawn][j]), 0, 0, 255);
			G3DrawSphere (&spherePoint, (int) (gameData.marker.fScale * MARKER_SPHERE_SIZE) >> j, 1);
			}
		DrawMarkerNumber (i);
		}
if (cycdir) {
	cyc += 2;
	if (cyc > 43) {
		cyc = 43;
		cycdir = 0;
		}
	}
else {
	cyc -= 2;
	if (cyc < 10) {
		cyc = 10;
		cycdir = 1;
		}
	}
}

//------------------------------------------------------------------------------

int MoveSpawnMarker (tObjTransformation *posP, short nSegment)
{
	CObject	*markerP;

if (!(markerP = SpawnMarkerObject (-1)))
	return 0;
markerP->info.position = *posP;
markerP->RelinkToSeg (nSegment);
return 1;
}

//------------------------------------------------------------------------------

void DropMarker (char nPlayerMarker, int bSpawn)
{
	ubyte		nMarker = (gameData.multiplayer.nLocalPlayer * 2) + nPlayerMarker;
	CObject	*playerP = OBJECTS + LOCALPLAYER.nObject;

if (!(bSpawn && MoveSpawnMarker (&playerP->info.position, playerP->info.nSegment))) {
	gameData.marker.point [nMarker] = playerP->info.position.vPos;
	short nObject = gameData.marker.objects [nMarker];
	if ((nObject >= 0) && (nObject <= gameData.objs.nLastObject [0]) && (OBJECTS [nObject].info.nType == OBJ_MARKER))
		ReleaseObject (nObject);
	if (bSpawn)
		strcpy (gameData.marker.szMessage [nMarker], "SPAWN");
	gameData.marker.objects [nMarker] =
		DropMarkerObject (&playerP->info.position.vPos, (short) playerP->info.nSegment, &playerP->info.position.mOrient, nMarker);
	if (IsMultiGame)
		MultiSendDropMarker (gameData.multiplayer.nLocalPlayer, playerP->info.position.vPos, nPlayerMarker, gameData.marker.szMessage [nMarker]);
	}
}

//------------------------------------------------------------------------------

void DropSpawnMarker (void)
{
if ((!IsMultiGame || IsCoopGame) && !(gameStates.app.bPlayerExploded || gameStates.app.bPlayerIsDead)) {
		char nMarker = (char) SpawnMarkerIndex (-1);

	if (nMarker < 0)
		nMarker = (gameData.marker.nLast + 1) % MaxDrop ();
	else
		nMarker -= (gameData.multiplayer.nLocalPlayer * 2);
	DropMarker (nMarker, 1);
	}
}

//------------------------------------------------------------------------------

void DropBuddyMarker (CObject *objP)
{
	ubyte	nMarker;

	//	Find spare marker slot.  "if" code below should be an assert, but what if someone changes NUM_MARKERS or MAX_CROP_SINGLE and it never gets hit?
nMarker = MAX_DROP_SINGLE + 1;
if (nMarker > NUM_MARKERS - 1)
	nMarker = NUM_MARKERS - 1;
sprintf (gameData.marker.szMessage [nMarker], "RIP: %s",gameData.escort.szName);
gameData.marker.point [nMarker] = objP->info.position.vPos;
if (gameData.marker.objects [nMarker] != -1 && gameData.marker.objects [nMarker] !=0)
	ReleaseObject (gameData.marker.objects [nMarker]);
gameData.marker.objects [nMarker] = DropMarkerObject (&objP->info.position.vPos, (short) objP->info.nSegment, &objP->info.position.mOrient, nMarker);
}

//------------------------------------------------------------------------------

void ClearMarkers (void)
{
int i;

for (i = 0; i < NUM_MARKERS; i++) {
	gameData.marker.szMessage [i][0] = 0;
	gameData.marker.objects [i] = -1;
	}
}

//------------------------------------------------------------------------------

int LastMarker (void)
{
	int nMaxDrop, h, i;

//find free marker slot
nMaxDrop = MaxDrop ();
h = gameData.multiplayer.nLocalPlayer * 2 + nMaxDrop;
for (i = nMaxDrop; i; i--)
	if (gameData.marker.objects [--h] > -1)		//found free slot!
		return i - 1;
return -1;
}

//------------------------------------------------------------------------------

int SpawnMarkerIndex (int nPlayer)
{
	int nMaxDrop, h, i;

//find free marker slot
nMaxDrop = MaxDrop ();
if (nPlayer < 0)
	nPlayer = gameData.multiplayer.nLocalPlayer;
for (i = nPlayer * 2, h = i + nMaxDrop; i < h; i++) {
	if (gameData.marker.objects [i] < 0)		//found free slot!
		continue;
	if (!strcmp (gameData.marker.szMessage [i], "SPAWN"))
		return i;
	}
return -1;
}

//------------------------------------------------------------------------------

CObject *SpawnMarkerObject (int nPlayer)
{
	int	i = (IsCoopGame || !IsMultiGame) ? SpawnMarkerIndex (nPlayer) : -1;

return (i < 0) ? NULL : OBJECTS + gameData.marker.objects [i];
}

//------------------------------------------------------------------------------

int IsSpawnMarkerObject (CObject *objP)
{
if (objP->info.nType == OBJ_MARKER) {
	int nMaxDrop, h, i, nObject = objP->Index ();

	nMaxDrop = MaxDrop ();
	for (i = gameData.multiplayer.nLocalPlayer * 2, h = i + nMaxDrop; i < h; i++)
		if (gameData.marker.objects [i] == nObject)		//found free slot!
			return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

void DeleteMarker (int bForce)
{
if ((gameData.marker.nHighlight > -1) && (gameData.marker.objects [gameData.marker.nHighlight] != -1)) {
	gameData.objs.viewerP = OBJECTS + gameData.marker.objects [gameData.marker.nHighlight];
	if (bForce || !ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_DELETE_MARKER)) {
		int	h, i;
		ReleaseObject (gameData.marker.objects [gameData.marker.nHighlight]);
		i = LastMarker ();
		if (i == gameData.marker.nHighlight) {
			gameData.marker.objects [gameData.marker.nHighlight] = -1;
			gameData.marker.szMessage [gameData.marker.nHighlight][0] = '\0';
			gameData.marker.nHighlight = i ? 0 : -1;
			}
		else {
			h = i - gameData.marker.nHighlight;
			memcpy (gameData.marker.objects + gameData.marker.nHighlight,
						gameData.marker.objects + gameData.marker.nHighlight + 1,
						h * sizeof (gameData.marker.objects [0]));
			memcpy (gameData.marker.szMessage [gameData.marker.nHighlight],
						gameData.marker.szMessage [gameData.marker.nHighlight + 1],
						h * sizeof (gameData.marker.szMessage [0]));
			gameData.marker.objects [i] = -1;
			gameData.marker.szMessage [i][0] = '\0';
			}
		}
	gameData.objs.viewerP = gameData.objs.consoleP;
	}
}

//------------------------------------------------------------------------------

void TeleportToMarker (void)
{
if (!IsMultiGame || IsCoopGame) {
#if !DBG
	if (LOCALPLAYER.energy < F1_0 * 25)
		HUDMessage (0, TXT_CANNOT_TELEPORT);
	else
#endif
	if ((gameData.marker.nHighlight > -1) && (gameData.marker.objects [gameData.marker.nHighlight] != -1)) {
		gameData.objs.viewerP = OBJECTS + gameData.marker.objects [gameData.marker.nHighlight];
		if (!ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_JUMP_TO_MARKER)) {
			CObject	*markerP = OBJECTS + gameData.marker.objects [gameData.marker.nHighlight];

#if !DBG
			LOCALPLAYER.energy -= F1_0 * 25;
#endif
			OBJECTS [LOCALPLAYER.nObject].info.position.vPos = markerP->info.position.vPos;
			OBJECTS [LOCALPLAYER.nObject].RelinkToSeg (markerP->info.nSegment);
			gameStates.render.bDoAppearanceEffect = 1;
			}
		gameData.objs.viewerP = gameData.objs.consoleP;
		}
	}
}

//------------------------------------------------------------------------------

void InitMarkerInput (void)
{
	int nMaxDrop, i;

if (gameData.marker.nLast < 0)
	gameData.marker.nLast = MaxDrop ();
//find free marker slot
i = LastMarker () + 1;
nMaxDrop = MaxDrop ();
if (i == nMaxDrop) {		//no free slot
	if (IsCoopGame)
		i = (gameData.marker.nLast + 1) % nMaxDrop;
	else if (IsMultiGame) {
		if (gameData.marker.nLast < 0)
			gameData.marker.nLast = MaxDrop ();
		i = !gameData.marker.nLast;		//in multi, replace older of two
		}
	else {
		HUDInitMessage (TXT_MARKER_SLOTS);
		return;
		}
	}
//got a free slot. start inputting marker message
gameData.marker.szInput [0] = '\0';
gameData.marker.nIndex = 0;
gameData.marker.nDefiningMsg = 1;
gameData.marker.nCurrent = i;
}

//------------------------------------------------------------------------------

void MarkerInputMessage (int key)
{
	int nMarker, bSpawn;

switch (key) {
	case KEY_F8:
	case KEY_ESC:
		gameData.marker.nDefiningMsg = 0;
		GameFlushInputs ();
		break;

	case KEY_LEFT:
	case KEY_BACKSP:
	case KEY_PAD4:
		if (gameData.marker.nIndex > 0)
			gameData.marker.nIndex--;
		gameData.marker.szInput [gameData.marker.nIndex] = 0;
		break;

	case KEY_ENTER:
		nMarker = (gameData.multiplayer.nLocalPlayer * 2)+gameData.marker.nCurrent;
		strupr (gameData.marker.szInput);
		strcpy (gameData.marker.szMessage [nMarker], gameData.marker.szInput);
		if (IsMultiGame)
			strcpy (gameData.marker.nOwner [nMarker], LOCALPLAYER.callsign);
		if ((bSpawn = !strcmp (gameData.marker.szMessage [nMarker], "SPAWN")))
			*gameData.marker.szMessage [nMarker] = '\0';
		DropMarker (gameData.marker.nCurrent, bSpawn);
		gameData.marker.nLast = gameData.marker.nCurrent;
		GameFlushInputs ();
		gameData.marker.nDefiningMsg = 0;
		break;

	default:
		if (key > 0) {
			int ascii = KeyToASCII (key);
			if ((ascii < 255) && (gameData.marker.nIndex < 38)) {
				gameData.marker.szInput [gameData.marker.nIndex++] = ascii;
				gameData.marker.szInput [gameData.marker.nIndex] = 0;
				}
			}
		break;
		}
}

//------------------------------------------------------------------------------
//eof
