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

#include "descent.h"
#include "error.h"
#include "key.h"
#include "text.h"
#include "strutil.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "menu.h"
#include "marker.h"
#include "vers_id.h"

CMarkerManager markerManager;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

CMarkerData::CMarkerData ()
{
Init ();
}

// ----------------------------------------------------------------------------

void CMarkerData::Init (void)
{
nHighlight = -1;
viewers [0] =
viewers [1] = -1;
fScale = 2.0f;
CLEAR (szMessage);
CLEAR (szOwner);
CLEAR (viewers);
CLEAR (szInput);
nHighlight = 0;
nIndex = 0;
nCurrent = 0;
nLast = -1;
fScale = 0;
bDefiningMsg = false;
position.Clear ();
objects.Clear ();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

inline CObject* CMarkerManager::GetObject (int32_t nPlayer, int32_t nMarker)
{
int16_t nObject = m_data.objects [((nPlayer < 0) ? N_LOCALPLAYER : nPlayer) * 2 + nMarker];
return (nObject < 0) ? NULL : OBJECT (nObject);
}

// ----------------------------------------------------------------------------

void CMarkerManager::DrawNumber (int32_t nMarker)
{
	CRenderPoint	basePoint;
	int32_t		i, j;
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

  int32_t nPoints [] = {6, 10, 8, 6, 10, 10, 4, 10, 8};

if (ogl.SizeVertexBuffer (nPoints [nMarker])) {
	if (m_data.fScale == 0.0f)
		m_data.fScale = 2.0f;
	if (nMarker == m_data.nHighlight)
		CCanvas::Current ()->SetColorRGB (255, 255, 255, 255);
	else if (!strcmp (m_data.szMessage [nMarker], "SPAWN"))
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (63, 0, 47));
	else
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (63, 15, 0));
	basePoint.TransformAndEncode (GetObject (-1, nMarker)->info.position.vPos);
	glPushMatrix ();
	glTranslatef (X2F (basePoint.ViewPos ().v.coord.x), X2F (basePoint.ViewPos ().v.coord.y), X2F (basePoint.ViewPos ().v.coord.z));
	ogl.SelectTMU (GL_TEXTURE0);
	ogl.SetTexturing (false);
	OglCanvasColor (&CCanvas::Current ()->Color ());
	px = xCoord [nMarker];
	py = yCoord [nMarker];
	for (i = nPoints [nMarker] / 2, j = 0; i; i--) {
		ogl.VertexBuffer () [j].v.coord.x = *px++ * m_data.fScale;
		ogl.VertexBuffer () [j].v.coord.y = *py++ * m_data.fScale;
		j++;
		ogl.VertexBuffer () [j].v.coord.x = *px++ * m_data.fScale;
		ogl.VertexBuffer () [j].v.coord.y = *py++ * m_data.fScale;
		j++;
		}
	ogl.FlushBuffers (GL_LINES, nPoints [nMarker], 2);
	glPopMatrix ();
	}
}

//------------------------------------------------------------------------------

#define MARKER_SPHERE_SIZE 0x58000

void CMarkerManager::Render (void)
{
	CRenderPoint spherePoint;
	int32_t		i, j, nMaxDrop, bSpawn;
	CObject*	pObj;
	uint8_t*	pColor;

	static int32_t cyc = 10, cycdir = 1;
	static uint8_t	colors [2][3] = {{20, 30, 40},{40, 50, 60}};

if (m_data.fScale == 0.0f)
	m_data.fScale = 2.0f;
nMaxDrop = MaxDrop ();
spherePoint.SetIndex (-1);
for (i = 0; i < nMaxDrop; i++)
	if ((pObj = GetObject (-1, i))) {
		bSpawn = (pObj == markerManager.SpawnObject (-1));
		pColor = &colors [bSpawn][0];
		DrawNumber (i);
		spherePoint.TransformAndEncode (pObj->info.position.vPos);
		for (j = 0; j < 3; j++, pColor++) {
			CCanvas::Current ()->SetColorRGB (PAL2RGBA (*pColor), 0, 0, 255);
			G3DrawSphere (&spherePoint, int32_t (m_data.fScale * MARKER_SPHERE_SIZE) >> j, 1);
			}
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

int32_t CMarkerManager::MoveSpawnPoint (tObjTransformation *pPos, int16_t nSegment)
{
	CObject	*pMarker = markerManager.SpawnObject (-1);

if (!pMarker)
	return 0;
pMarker->info.position = *pPos;
pMarker->RelinkToSeg (nSegment);
return 1;
}

//------------------------------------------------------------------------------

void CMarkerManager::Drop (char nPlayerMarker, int32_t bSpawn)
{
	uint8_t		nMarker = (N_LOCALPLAYER * 2) + nPlayerMarker;
	CObject	*pPlayer = OBJECT (LOCALPLAYER.nObject);

if (!(bSpawn && markerManager.MoveSpawnPoint (&pPlayer->info.position, pPlayer->info.nSegment))) {
	m_data.position [nMarker] = pPlayer->info.position.vPos;
	int16_t nObject = m_data.objects [nMarker];
	if ((nObject >= 0) && (nObject <= gameData.objData.nLastObject [0]) && (OBJECT (nObject)->info.nType == OBJ_MARKER))
		ReleaseObject (nObject);
	if (bSpawn)
		strcpy (m_data.szMessage [nMarker], "SPAWN");
	m_data.objects [nMarker] =
		DropMarkerObject (pPlayer->info.position.vPos, (int16_t) pPlayer->info.nSegment, pPlayer->info.position.mOrient, nMarker);
	if (IsMultiGame)
		MultiSendDropMarker (N_LOCALPLAYER, pPlayer->info.position.vPos, nPlayerMarker, m_data.szMessage [nMarker]);
	}
}

//------------------------------------------------------------------------------

void CMarkerManager::DropSpawnPoint (void)
{
if ((!IsMultiGame || IsCoopGame) && !(LOCALPLAYER.m_bExploded || gameStates.app.bPlayerIsDead)) {
		char nMarker = (char) markerManager.SpawnIndex (-1);

	if (nMarker < 0)
		nMarker = (m_data.nLast + 1) % MaxDrop ();
	else
		nMarker -= (N_LOCALPLAYER * 2);
	Drop (nMarker, 1);
	}
}

//------------------------------------------------------------------------------

void CMarkerManager::DropForGuidebot (CObject *pObj)
{
	uint8_t	nMarker = MAX_DROP_SINGLE + 1;

//	Find spare marker slot.  "if" code below should be an assert, but what if someone changes NUM_MARKERS or MAX_CROP_SINGLE and it never gets hit?
if (nMarker > NUM_MARKERS - 1)
	nMarker = NUM_MARKERS - 1;
sprintf (m_data.szMessage [nMarker], "RIP: %s",gameData.escortData.szName);
m_data.position [nMarker] = pObj->info.position.vPos;
if ((m_data.objects [nMarker] != -1) && (m_data.objects [nMarker] != 0))
	ReleaseObject (m_data.objects [nMarker]);
m_data.objects [nMarker] = DropMarkerObject (pObj->info.position.vPos, (int16_t) pObj->info.nSegment, pObj->info.position.mOrient, nMarker);
}

//------------------------------------------------------------------------------

void CMarkerManager::Clear (void)
{
for (int32_t i = 0; i < NUM_MARKERS; i++) {
	m_data.szMessage [i][0] = 0;
	m_data.objects [i] = -1;
	}
}

//------------------------------------------------------------------------------

int32_t CMarkerManager::Last (void)
{
	int32_t nMaxDrop, h, i;

//find free marker slot
nMaxDrop = MaxDrop ();
h = N_LOCALPLAYER * 2 + nMaxDrop;
for (i = nMaxDrop; i; i--)
	if (m_data.objects [--h] > -1)		//found free slot!
		return i - 1;
return -1;
}

//------------------------------------------------------------------------------

int32_t CMarkerManager::SpawnIndex (int32_t nPlayer)
{
	int32_t nMaxDrop, h, i;

//find free marker slot
nMaxDrop = MaxDrop ();
if (nPlayer < 0)
	nPlayer = N_LOCALPLAYER;
for (i = nPlayer * 2, h = i + nMaxDrop; i < h; i++) {
	if (m_data.objects [i] < 0)		//found free slot!
		continue;
	if (!strcmp (m_data.szMessage [i], "SPAWN"))
		return i;
	}
return -1;
}

//------------------------------------------------------------------------------

CObject* CMarkerManager::SpawnObject (int32_t nPlayer)
{
	int32_t	i = (IsCoopGame || !IsMultiGame) ? markerManager.SpawnIndex (nPlayer) : -1;

return (i < 0) ? NULL : OBJECT (m_data.objects [i]);
}

//------------------------------------------------------------------------------

int32_t CMarkerManager::IsSpawnObject (CObject *pObj)
{
if (pObj->info.nType == OBJ_MARKER) {
	int32_t nMaxDrop, h, i, nObject = pObj->Index ();

	nMaxDrop = MaxDrop ();
	for (i = N_LOCALPLAYER * 2, h = i + nMaxDrop; i < h; i++)
		if (m_data.objects [i] == nObject)		//found free slot!
			return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

void CMarkerManager::Rotate (void)
{
	int32_t	nObject; 

if ((m_data.nHighlight > -1) && ((nObject = m_data.objects [m_data.nHighlight]) != -1))
	OBJECT (nObject)->Rotate (!OBJECT (nObject)->Rotating ());
}

//------------------------------------------------------------------------------

void CMarkerManager::Delete (int32_t bForce)
{
if ((m_data.nHighlight > -1) && (m_data.objects [m_data.nHighlight] != -1)) {
	gameData.SetViewer (OBJECT (m_data.objects [m_data.nHighlight]));
	if (bForce || !InfoBox (NULL,NULL,  BG_STANDARD, 2, TXT_YES, TXT_NO, TXT_DELETE_MARKER)) {
		int32_t	h, i;
		ReleaseObject (m_data.objects [m_data.nHighlight]);
		i = markerManager.Last ();
		if (i == m_data.nHighlight) {
			m_data.objects [m_data.nHighlight] = -1;
			m_data.szMessage [m_data.nHighlight][0] = '\0';
			m_data.nHighlight = i ? 0 : -1;
			}
		else {
			h = i - m_data.nHighlight;
			memcpy (m_data.objects + m_data.nHighlight,
						m_data.objects + m_data.nHighlight + 1,
						h * sizeof (m_data.objects [0]));
			memcpy (m_data.szMessage [m_data.nHighlight],
						m_data.szMessage [m_data.nHighlight + 1],
						h * sizeof (m_data.szMessage [0]));
			m_data.objects [i] = -1;
			m_data.szMessage [i][0] = '\0';
			}
		}
	gameData.SetViewer (gameData.objData.pConsole);
	}
}

//------------------------------------------------------------------------------

void CMarkerManager::Teleport (void)
{
if (!IsMultiGame || IsCoopGame) {
#if 1 //!DBG
	if ((LOCALPLAYER.Energy () < I2X (101) / 2) || (LOCALPLAYER.Shield () < I2X (51) / 2))
		HUDMessage (0, TXT_CANNOT_TELEPORT);
	else
#endif
	if ((m_data.nHighlight > -1) && (m_data.objects [m_data.nHighlight] != -1)) {
		gameData.SetViewer (OBJECT (m_data.objects [m_data.nHighlight]));
		if (!InfoBox (NULL,NULL,  BG_STANDARD, 2, TXT_YES, TXT_NO, TXT_JUMP_TO_MARKER)) {
			CObject	*pMarker = OBJECT (m_data.objects [m_data.nHighlight]);

#if 1 //!DBG
			LOCALPLAYER.UpdateEnergy (-I2X (101) / 2);
			LOCALPLAYER.UpdateShield (-I2X (51) / 2);
#endif
			LOCALOBJECT->info.position.vPos = pMarker->info.position.vPos;
			LOCALOBJECT->RelinkToSeg (pMarker->info.nSegment);
			if (!IsMultiGame || IsCoopGame)
				gameStates.render.bDoAppearanceEffect = 1;
			}
		gameData.SetViewer (gameData.objData.pConsole);
		}
	}
}

//------------------------------------------------------------------------------

void CMarkerManager::InitInput (bool bRotate)
{
	int32_t nMaxDrop, i;

if (m_data.nLast < 0)
	m_data.nLast = MaxDrop ();
//find free marker slot
i = markerManager.Last () + 1;
nMaxDrop = MaxDrop ();
if (i == nMaxDrop) {		//no free slot
	if (IsCoopGame)
		i = (m_data.nLast + 1) % nMaxDrop;
	else if (IsMultiGame) {
		if (m_data.nLast < 0)
			m_data.nLast = MaxDrop ();
		i = !m_data.nLast;		//in multi, replace older of two
		}
	else {
		HUDInitMessage (TXT_MARKER_SLOTS);
		return;
		}
	}
//got a free slot. start inputting marker message
m_data.szInput [0] = '\0';
m_data.nIndex = 0;
m_data.bRotate = bRotate;
m_data.bDefiningMsg = true;
m_data.nCurrent = i;
}

//------------------------------------------------------------------------------

void CMarkerManager::InputMessage (int32_t key)
{
	int32_t nMarker, bSpawn;

switch (key) {
	case KEY_F8:
	case KEY_ESC:
		m_data.bDefiningMsg = 0;
		GameFlushInputs ();
		break;

	case KEY_LEFT:
	case KEY_BACKSPACE:
	case KEY_PAD4:
		if (m_data.nIndex > 0)
			m_data.nIndex--;
		m_data.szInput [m_data.nIndex] = 0;
		break;

	case KEY_ENTER:
		nMarker = (N_LOCALPLAYER * 2) + m_data.nCurrent;
		strupr (m_data.szInput);
		strcpy (m_data.szMessage [nMarker], m_data.szInput);
		if (IsMultiGame)
			strcpy (m_data.szOwner [nMarker], LOCALPLAYER.callsign);
		if ((bSpawn = !strcmp (m_data.szMessage [nMarker], "SPAWN")))
			*m_data.szMessage [nMarker] = '\0';
		Drop (m_data.nCurrent, bSpawn);
		m_data.nLast = m_data.nCurrent;
		GameFlushInputs ();
		m_data.bDefiningMsg = 0;
		break;

	default:
		if (key > 0) {
			int32_t ascii = KeyToASCII (key);
			if ((ascii < 255) && (m_data.nIndex < 38)) {
				m_data.szInput [m_data.nIndex++] = ascii;
				m_data.szInput [m_data.nIndex] = 0;
				}
			}
		break;
		}
}

//------------------------------------------------------------------------------

void CMarkerManager::SaveState (CFile& cf)
{
for (int32_t i = 0; i < NUM_MARKERS; i++)
	cf.WriteShort (m_data.objects [i]);
cf.Write (m_data.szOwner, sizeof (m_data.szOwner), 1);
cf.Write (m_data.szMessage, sizeof (m_data.szMessage), 1);
}

//------------------------------------------------------------------------------

void CMarkerManager::LoadState (CFile& cf, bool bBinary)
{
if (bBinary) {
	for (int32_t i = 0; i < NUM_MARKERS_D2; i++)
		m_data.objects [i] = int16_t (cf.ReadInt ());
	cf.Read (m_data.szOwner, sizeof (m_data.szOwner [0]) * NUM_MARKERS_D2, 1);
	cf.Read (m_data.szMessage, sizeof (m_data.szMessage [0]) * NUM_MARKERS_D2, 1);
	}
else {
	for (int32_t i = 0; i < NUM_MARKERS; i++)
		m_data.objects [i] = cf.ReadShort ();
	cf.Read (m_data.szOwner, sizeof (m_data.szOwner), 1);
	cf.Read (m_data.szMessage, sizeof (m_data.szMessage), 1);
	}
}

//------------------------------------------------------------------------------
//eof
