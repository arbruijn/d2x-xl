/* $Id: newdemo.c, v 1.15 2003/11/26 12:26:31 btb Exp $ */
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h> // for memset
#ifndef _WIN32_WCE
#include <errno.h>
#endif
#include <ctype.h>      /* for isdigit */
#include <limits.h>
#if defined (__unix__) || defined (__macosx__)
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "u_mem.h"
#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "stdlib.h"
#include "bm.h"
//#include "error.h"
#include "mono.h"
#include "3d.h"
#include "segment.h"
#include "texmap.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"

#include "object.h"
#include "objrender.h"
#include "physics.h"
#include "slew.h"
#include "render.h"
#include "wall.h"
#include "vclip.h"
#include "polyobj.h"
#include "fireball.h"
#include "laser.h"
#include "error.h"
#include "ai.h"
#include "hostage.h"
#include "morph.h"

#include "powerup.h"
#include "fuelcen.h"

#include "sounds.h"
#include "collide.h"

#include "light.h"
#include "newdemo.h"
#include "loadgame.h"
#include "gamesave.h"
#include "gamemine.h"
#include "switch.h"
#include "gauges.h"
#include "player.h"
#include "vecmat.h"
#include "newmenu.h"
#include "args.h"
#include "palette.h"
#include "multi.h"
#include "network.h"
#include "text.h"
#include "cntrlcen.h"
#include "aistruct.h"
#include "mission.h"
#include "piggy.h"
#include "controls.h"
#include "d_io.h"
#include "timer.h"
#include "objsmoke.h"
#include "menu.h"

#include "findfile.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

extern void SetFunctionMode (int);

void DoJasonInterpolate (fix xRecordedTime);

//#include "nocfile.h"

//Does demo start automatically?

static sbyte	bNDBadRead;
static int		bRevertFormat = -1;

#define	CATCH_BAD_READ				if (bNDBadRead) {bDone = -1; break;}

#define ND_EVENT_EOF                0   // EOF
#define ND_EVENT_START_DEMO         1   // Followed by 16 character, NULL terminated filename of .SAV file to use
#define ND_EVENT_START_FRAME        2   // Followed by integer frame number, then a fix gameData.time.xFrame
#define ND_EVENT_VIEWER_OBJECT      3   // Followed by an tObject structure
#define ND_EVENT_RENDER_OBJECT      4   // Followed by an tObject structure
#define ND_EVENT_SOUND              5   // Followed by int soundum
#define ND_EVENT_SOUND_ONCE         6   // Followed by int soundum
#define ND_EVENT_SOUND_3D           7   // Followed by int soundum, int angle, int volume
#define ND_EVENT_WALL_HIT_PROCESS   8   // Followed by int nSegment, int nSide, fix damage
#define ND_EVENT_TRIGGER            9   // Followed by int nSegment, int nSide, int nObject
#define ND_EVENT_HOSTAGE_RESCUED    10  // Followed by int hostageType
#define ND_EVENT_SOUND_3D_ONCE      11  // Followed by int soundum, int angle, int volume
#define ND_EVENT_MORPH_FRAME        12  // Followed by ? data
#define ND_EVENT_WALL_TOGGLE        13  // Followed by int seg, int nSide
#define ND_EVENT_HUD_MESSAGE        14  // Followed by char size, char * string (+null)
#define ND_EVENT_CONTROL_CENTER_DESTROYED 15 // Just a simple flag
#define ND_EVENT_PALETTE_EFFECT     16  // Followed by short r, g, b
#define ND_EVENT_PLAYER_ENERGY      17  // followed by byte energy
#define ND_EVENT_PLAYER_SHIELD      18  // followed by byte shields
#define ND_EVENT_PLAYER_FLAGS       19  // followed by tPlayer flags
#define ND_EVENT_PLAYER_WEAPON      20  // followed by weapon nType and weapon number
#define ND_EVENT_EFFECT_BLOWUP      21  // followed by tSegment, nSide, and pnt
#define ND_EVENT_HOMING_DISTANCE    22  // followed by homing distance
#define ND_EVENT_LETTERBOX          23  // letterbox mode for death seq.
#define ND_EVENT_RESTORE_COCKPIT    24  // restore cockpit after death
#define ND_EVENT_REARVIEW           25  // going to rear view mode
#define ND_EVENT_WALL_SET_TMAP_NUM1 26  // Wall changed
#define ND_EVENT_WALL_SET_TMAP_NUM2 27  // Wall changed
#define ND_EVENT_NEW_LEVEL          28  // followed by level number
#define ND_EVENT_MULTI_CLOAK        29  // followed by tPlayer num
#define ND_EVENT_MULTI_DECLOAK      30  // followed by tPlayer num
#define ND_EVENT_RESTORE_REARVIEW   31  // restore cockpit after rearview mode
#define ND_EVENT_MULTI_DEATH        32  // with tPlayer number
#define ND_EVENT_MULTI_KILL         33  // with tPlayer number
#define ND_EVENT_MULTI_CONNECT      34  // with tPlayer number
#define ND_EVENT_MULTI_RECONNECT    35  // with tPlayer number
#define ND_EVENT_MULTI_DISCONNECT   36  // with tPlayer number
#define ND_EVENT_MULTI_SCORE        37  // playernum / score
#define ND_EVENT_PLAYER_SCORE       38  // followed by score
#define ND_EVENT_PRIMARY_AMMO       39  // with old/new ammo count
#define ND_EVENT_SECONDARY_AMMO     40  // with old/new ammo count
#define ND_EVENT_DOOR_OPENING       41  // with tSegment/nSide
#define ND_EVENT_LASER_LEVEL        42  // no data
#define ND_EVENT_PLAYER_AFTERBURNER 43  // followed by byte old ab, current ab
#define ND_EVENT_CLOAKING_WALL      44  // info changing while tWall cloaking
#define ND_EVENT_CHANGE_COCKPIT     45  // change the cockpit
#define ND_EVENT_START_GUIDED       46  // switch to guided view
#define ND_EVENT_END_GUIDED         47  // stop guided view/return to ship
#define ND_EVENT_SECRET_THINGY      48  // 0/1 = secret exit functional/non-functional
#define ND_EVENT_LINK_SOUND_TO_OBJ  49  // record DigiLinkSoundToObject3
#define ND_EVENT_KILL_SOUND_TO_OBJ  50  // record DigiKillSoundLinkedToObject


#define NORMAL_PLAYBACK         0
#define SKIP_PLAYBACK           1
#define INTERPOLATE_PLAYBACK    2
#define INTERPOL_FACTOR         (F1_0 + (F1_0/5))

#define DEMO_VERSION_D2X        17      // last D1 version was 13
#define DEMO_GAME_TYPE          3       // 1 was shareware, 2 registered

#define DEMO_FILENAME           "tmpdemo.dem"

#define DEMO_MAX_LEVELS         29

CFILE ndInFile = {NULL, 0, 0, 0};
CFILE ndOutFile = {NULL, 0, 0, 0};

//	-----------------------------------------------------------------------------

int NDErrorMsg (char *pszMsg1, char *pszMsg2, char *pszMsg3)
{
	tMenuItem	m [3];
	int			opt = 0;

memset (m, 0, sizeof (m));
ADD_TEXT (opt, pszMsg1, 0);
opt++;
if (pszMsg2 && *pszMsg2) {
	ADD_TEXT (opt, pszMsg2, 0);
	opt++;
	}
if (pszMsg3 && *pszMsg3) {
	ADD_TEXT (opt, pszMsg3, 0);
	opt++;
	}
ExecMenu (NULL, NULL, opt, m, NULL, NULL);
return 1;
}

//	-----------------------------------------------------------------------------

void InitDemoData (void)
{
gameData.demo.bAuto = 0;
gameData.demo.nState = 0;
gameData.demo.nVcrState = 0;
gameData.demo.nStartFrame = -1;
gameData.demo.bInterpolate = 0; // 1
gameData.demo.bWarningGiven = 0;
gameData.demo.bCtrlcenDestroyed = 0;
gameData.demo.nFrameBytesWritten = 0;
gameData.demo.bFirstTimePlayback = 1;
gameData.demo.xJasonPlaybackTotal = 0;
}

//	-----------------------------------------------------------------------------

int NDGetPercentDone () 
{
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return (CFTell (&ndInFile) * 100) / gameData.demo.nSize;
if (gameData.demo.nState == ND_STATE_RECORDING)
	return CFTell (&ndOutFile);
return 0;
}

//	-----------------------------------------------------------------------------

#define VEL_PRECISION 12

void my_extract_shortpos (tObject *objP, tShortPos *spp)
{
	int nSegment;
	sbyte *sp;
	vmsVector	*pv;

sp = spp->bytemat;
objP->position.mOrient.rVec.p.x = *sp++ << MATRIX_PRECISION;
objP->position.mOrient.uVec.p.x = *sp++ << MATRIX_PRECISION;
objP->position.mOrient.fVec.p.x = *sp++ << MATRIX_PRECISION;
objP->position.mOrient.rVec.p.y = *sp++ << MATRIX_PRECISION;
objP->position.mOrient.uVec.p.y = *sp++ << MATRIX_PRECISION;
objP->position.mOrient.fVec.p.y = *sp++ << MATRIX_PRECISION;
objP->position.mOrient.rVec.p.z = *sp++ << MATRIX_PRECISION;
objP->position.mOrient.uVec.p.z = *sp++ << MATRIX_PRECISION;
objP->position.mOrient.fVec.p.z = *sp++ << MATRIX_PRECISION;
nSegment = spp->nSegment;
objP->nSegment = nSegment;
pv = gameData.segs.vertices + gameData.segs.segments [nSegment].verts [0];
objP->position.vPos.p.x = (spp->xo << RELPOS_PRECISION) + pv->p.x;
objP->position.vPos.p.y = (spp->yo << RELPOS_PRECISION) + pv->p.y;
objP->position.vPos.p.z = (spp->zo << RELPOS_PRECISION) + pv->p.z;
objP->mType.physInfo.velocity.p.x = (spp->velx << VEL_PRECISION);
objP->mType.physInfo.velocity.p.y = (spp->vely << VEL_PRECISION);
objP->mType.physInfo.velocity.p.z = (spp->velz << VEL_PRECISION);
}

//	-----------------------------------------------------------------------------

int NDFindObject (int nSignature)
{
	int i;
	tObject * objP = gameData.objs.objects;

for (i = 0; i <= gameData.objs.nLastObject; i++, objP++) {
if ((objP->nType != OBJ_NONE) && (objP->nSignature == nSignature))
	return i;
	}
return -1;
}

//	-----------------------------------------------------------------------------

#ifdef _DEBUG

void CHK (void)
{
Assert (&ndOutFile.file != NULL);
if (gameData.demo.nWritten >= 750)
	gameData.demo.nWritten = gameData.demo.nWritten;
}

#else

#define	CHK()

#endif

//	-----------------------------------------------------------------------------

int NDWrite (void *buffer, int elsize, int nelem)
{
	int nWritten, nTotalSize = elsize * nelem;

gameData.demo.nFrameBytesWritten += nTotalSize;
gameData.demo.nWritten += nTotalSize;
Assert (&ndOutFile.file != NULL);
CHK();
nWritten = CFWrite (buffer, elsize, nelem, &ndOutFile);
if ((bRevertFormat < 1) && (gameData.demo.nWritten > gameData.demo.nSize) && !gameData.demo.bNoSpace)
	gameData.demo.bNoSpace = 1;
if ((nWritten == nelem) && !gameData.demo.bNoSpace)
	return nWritten;
gameData.demo.bNoSpace = 2;
NDStopRecording ();
return -1;
}

/*
 *  The next bunch of files taken from Matt's gamesave.c.  We have to modify
 *  these since the demo must save more information about gameData.objs.objects that
 *  just a gamesave
*/

//	-----------------------------------------------------------------------------

static inline void NDWriteByte (sbyte b)
{
gameData.demo.nFrameBytesWritten += sizeof (b);
gameData.demo.nWritten += sizeof (b);
CHK();
CFWriteByte (b, &ndOutFile);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteShort (short s)
{
gameData.demo.nFrameBytesWritten += sizeof (s);
gameData.demo.nWritten += sizeof (s);
CHK();
CFWriteShort (s, &ndOutFile);
}

//	-----------------------------------------------------------------------------

static void NDWriteInt (int i)
{
gameData.demo.nFrameBytesWritten += sizeof (i);
gameData.demo.nWritten += sizeof (i);
CHK();
CFWriteInt (i, &ndOutFile);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteString (char *str)
{
	sbyte l = (int) strlen (str) + 1;

NDWriteByte (l);
NDWrite (str, (int) l, 1);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteFix (fix f)
{
gameData.demo.nFrameBytesWritten += sizeof (f);
gameData.demo.nWritten += sizeof (f);
CHK();
CFWriteFix (f, &ndOutFile);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteFixAng (fixang f)
{
gameData.demo.nFrameBytesWritten += sizeof (f);
gameData.demo.nWritten += sizeof (f);
CFWriteFixAng (f, &ndOutFile);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteVector (vmsVector *v)
{
gameData.demo.nFrameBytesWritten += sizeof (*v);
gameData.demo.nWritten += sizeof (*v);
CHK();
CFWriteVector (v, &ndOutFile);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteAngVec (vmsAngVec *v)
{
gameData.demo.nFrameBytesWritten += sizeof (*v);
gameData.demo.nWritten += sizeof (*v);
CHK();
CFWriteAngVec (v, &ndOutFile);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteMatrix (vmsMatrix *m)
{
gameData.demo.nFrameBytesWritten += sizeof (*m);
gameData.demo.nWritten += sizeof (*m);
CHK();
CFWriteMatrix (m, &ndOutFile);
}

//	-----------------------------------------------------------------------------

void NDWritePosition (tObject *objP)
{
	ubyte			renderType = objP->renderType;
	tShortPos	sp;
	int			bOldFormat = gameStates.app.bNostalgia || gameOpts->demo.bOldFormat || (bRevertFormat > 0);

if (bOldFormat)
	CreateShortPos (&sp, objP, 0);
if ((renderType == RT_POLYOBJ) || (renderType == RT_HOSTAGE) || (renderType == RT_MORPH) || 
	 (objP->nType == OBJ_CAMERA) ||
	 (!bOldFormat && (renderType == RT_POWERUP))) {
	if (bOldFormat)
		NDWrite (sp.bytemat, sizeof (sp.bytemat [0]), sizeof (sp.bytemat) / sizeof (sp.bytemat [0]));
	else
		NDWriteMatrix (&objP->position.mOrient);
	}
if (bOldFormat) {
	NDWriteShort (sp.xo);
	NDWriteShort (sp.yo);
	NDWriteShort (sp.zo);
	NDWriteShort (sp.nSegment);
	NDWriteShort (sp.velx);
	NDWriteShort (sp.vely);
	NDWriteShort (sp.velz);
	}
else {
	NDWriteVector (&objP->position.vPos);
	NDWriteShort (objP->nSegment);
	NDWriteVector (&objP->mType.physInfo.velocity);
	}
}

//	-----------------------------------------------------------------------------

int NDRead (void *buffer, int elsize, int nelem)
{
int nRead = (int) CFRead (buffer, elsize, nelem, &ndInFile);
if (CFError (&ndInFile) || CFEoF (&ndInFile))
	bNDBadRead = -1;
else if (bRevertFormat > 0)
	NDWrite (buffer, elsize, nelem);
return nRead;
}

//	-----------------------------------------------------------------------------

static inline ubyte NDReadByte (void)
{
if (bRevertFormat > 0) {
	ubyte	h = CFReadByte (&ndInFile);
	NDWriteByte (h);
	return h;
	}
return CFReadByte (&ndInFile);
}

//	-----------------------------------------------------------------------------

static inline short NDReadShort (void)
{
if (bRevertFormat > 0) {
	short	h = CFReadShort (&ndInFile);
	NDWriteShort (h);
	return h;
	}
return CFReadShort (&ndInFile);
}

//	-----------------------------------------------------------------------------

static inline int NDReadInt ()
{
if (bRevertFormat > 0) {
	int h = CFReadInt (&ndInFile);
	NDWriteInt (h);
	return h;
	}
return CFReadInt (&ndInFile);
}

//	-----------------------------------------------------------------------------

static inline char *NDReadString (char *str)
{
	sbyte len;

len = NDReadByte ();
NDRead (str, len, 1);
return str;
}

//	-----------------------------------------------------------------------------

static inline fix NDReadFix (void)
{
if (bRevertFormat > 0) {
	fix h = CFReadFix (&ndInFile);
	NDWriteFix (h);
	return h;
	}
return CFReadFix (&ndInFile);
}

//	-----------------------------------------------------------------------------

static inline fixang NDReadFixAng (void)
{
if (bRevertFormat > 0) {
	fixang h = CFReadFixAng (&ndInFile);
	NDWriteFixAng (h);
	return h;
	}
return CFReadFixAng (&ndInFile);
}

//	-----------------------------------------------------------------------------

static inline void NDReadVector (vmsVector *v)
{
CFReadVector (v, &ndInFile);
if (bRevertFormat > 0)
	NDWriteVector (v);
}

//	-----------------------------------------------------------------------------

static inline void NDReadAngVec (vmsAngVec *v)
{
CFReadAngVec (v, &ndInFile);
if (bRevertFormat > 0)
	NDWriteAngVec (v);
}

//	-----------------------------------------------------------------------------

static inline void NDReadMatrix (vmsMatrix *m)
{
CFReadMatrix (m, &ndInFile);
if (bRevertFormat > 0)
	NDWriteMatrix (m);
}

//	-----------------------------------------------------------------------------

static void NDReadPosition (tObject *objP, int bSkip)
{
	tShortPos sp;
	ubyte renderType;

if (bRevertFormat > 0)
	bRevertFormat = 0;	//temporarily suppress writing back data
renderType = objP->renderType;
if ((renderType == RT_POLYOBJ) || (renderType == RT_HOSTAGE) || (renderType == RT_MORPH) || 
	 (objP->nType == OBJ_CAMERA) ||
	 ((renderType == RT_POWERUP) && (gameData.demo.nVersion > DEMO_VERSION + 1))) {
	if (gameData.demo.bUseShortPos)
		NDRead (sp.bytemat, sizeof (sp.bytemat [0]), sizeof (sp.bytemat) / sizeof (sp.bytemat [0]));
	else
		CFReadMatrix (&objP->position.mOrient, &ndInFile);
	}
if (gameData.demo.bUseShortPos) {
	sp.xo = NDReadShort ();
	sp.yo = NDReadShort ();
	sp.zo = NDReadShort ();
	sp.nSegment = NDReadShort ();
	sp.velx = NDReadShort ();
	sp.vely = NDReadShort ();
	sp.velz = NDReadShort ();
	my_extract_shortpos (objP, &sp);
	}
else {
	NDReadVector (&objP->position.vPos);
	objP->nSegment = NDReadShort ();
	NDReadVector (&objP->mType.physInfo.velocity);
	}
if ((objP->id == VCLIP_MORPHING_ROBOT) && 
	 (renderType == RT_FIREBALL) && 
	 (objP->controlType == CT_EXPLOSION))
	ExtractOrientFromSegment (&objP->position.mOrient, gameData.segs.segments + objP->nSegment);
if (!(bRevertFormat || bSkip)) {
	bRevertFormat = 1;
	NDWritePosition (objP);
	}
}

//	-----------------------------------------------------------------------------

tObject *prevObjP = NULL;      //ptr to last tObject read in

void NDReadObject (tObject *objP)
{
	int	bSkip = 0;

memset (objP, 0, sizeof (tObject));
/*
 * Do render nType first, since with renderType == RT_NONE, we
 * blow by all other tObject information
 */
if (bRevertFormat > 0)
	bRevertFormat = 0;
objP->renderType = NDReadByte ();
if (!bRevertFormat) {
	if (objP->renderType <= RT_WEAPON_VCLIP) {
		bRevertFormat = gameOpts->demo.bRevertFormat;
		NDWriteByte ((sbyte) objP->renderType);
		}
	else {
		bSkip = 1;
		CFSeek (&ndOutFile, -1, SEEK_CUR);
		gameData.demo.nFrameBytesWritten--;
		gameData.demo.nWritten--;
		}
	}
objP->nType = NDReadByte ();
if ((objP->renderType == RT_NONE) && (objP->nType != OBJ_CAMERA)) {
	if (!bRevertFormat)
		bRevertFormat = gameOpts->demo.bRevertFormat;
	return;
	}
objP->id = NDReadByte ();
if (gameData.demo.nVersion > DEMO_VERSION + 1) {
	if (bRevertFormat > 0)
		bRevertFormat = 0;
	objP->shields = NDReadFix ();
	if (!(bRevertFormat || bSkip))
		bRevertFormat = gameOpts->demo.bRevertFormat;
	}
objP->flags = NDReadByte ();
objP->nSignature = NDReadShort ();
NDReadPosition (objP, bSkip);
if ((objP->nType == OBJ_ROBOT) && (objP->id == SPECIAL_REACTOR_ROBOT))
	Int3 ();
objP->attachedObj = -1;
switch (objP->nType) {
	case OBJ_HOSTAGE:
		objP->controlType = CT_POWERUP;
		objP->movementType = MT_NONE;
		objP->size = HOSTAGE_SIZE;
		break;

	case OBJ_ROBOT:
		objP->controlType = CT_AI;
		// (MarkA and MikeK said we should not do the crazy last secret stuff with multiple reactors...
		// This necessary code is our vindication. --MK, 2/15/96)
		if (objP->id != SPECIAL_REACTOR_ROBOT)
			objP->movementType = MT_PHYSICS;
		else
			objP->movementType = MT_NONE;
		objP->size = gameData.models.polyModels [ROBOTINFO (objP->id).nModel].rad;
		objP->rType.polyObjInfo.nModel = ROBOTINFO (objP->id).nModel;
		objP->rType.polyObjInfo.nSubObjFlags = 0;
		objP->cType.aiInfo.CLOAKED = (ROBOTINFO (objP->id).cloakType?1:0);
		break;

	case OBJ_POWERUP:
		objP->controlType = CT_POWERUP;
		objP->movementType = NDReadByte ();        // might have physics movement
		objP->size = gameData.objs.pwrUp.info [objP->id].size;
		break;

	case OBJ_PLAYER:
		objP->controlType = CT_NONE;
		objP->movementType = MT_PHYSICS;
		objP->size = gameData.models.polyModels [gameData.pig.ship.player->nModel].rad;
		objP->rType.polyObjInfo.nModel = gameData.pig.ship.player->nModel;
		objP->rType.polyObjInfo.nSubObjFlags = 0;
		break;

	case OBJ_CLUTTER:
		objP->controlType = CT_NONE;
		objP->movementType = MT_NONE;
		objP->size = gameData.models.polyModels [objP->id].rad;
		objP->rType.polyObjInfo.nModel = objP->id;
		objP->rType.polyObjInfo.nSubObjFlags = 0;
		break;

	default:
		objP->controlType = NDReadByte ();
		objP->movementType = NDReadByte ();
		objP->size = NDReadFix ();
		break;
	}

NDReadVector (&objP->vLastPos);
if ((objP->nType == OBJ_WEAPON) && (objP->renderType == RT_WEAPON_VCLIP))
	objP->lifeleft = NDReadFix ();
else {
	ubyte b;

	b = NDReadByte ();
	objP->lifeleft = (fix) b;
	// MWA old way -- won't work with big endian machines       NDReadByte ((sbyte *) (ubyte *)&(objP->lifeleft);
	objP->lifeleft = (fix) ((int) objP->lifeleft << 12);
	}
if (objP->nType == OBJ_ROBOT) {
	if (ROBOTINFO (objP->id).bossFlag) {
		sbyte cloaked;
		cloaked = NDReadByte ();
		objP->cType.aiInfo.CLOAKED = cloaked;
		}
	}

switch (objP->movementType) {
	case MT_PHYSICS:
		NDReadVector (&objP->mType.physInfo.velocity);
		NDReadVector (&objP->mType.physInfo.thrust);
		break;

	case MT_SPINNING:
		NDReadVector (&objP->mType.spinRate);
		break;

	case MT_NONE:
		break;

	default:
		Int3 ();
	}

switch (objP->controlType) {
	case CT_EXPLOSION:
		objP->cType.explInfo.nSpawnTime = NDReadFix ();
		objP->cType.explInfo.nDeleteTime = NDReadFix ();
		objP->cType.explInfo.nDeleteObj = NDReadShort ();
		objP->cType.explInfo.nNextAttach = 
		objP->cType.explInfo.nPrevAttach = 
		objP->cType.explInfo.nAttachParent = -1;
		if (objP->flags & OF_ATTACHED) {     //attach to previous tObject
			Assert (prevObjP!=NULL);
			if (prevObjP->controlType == CT_EXPLOSION) {
				if ((prevObjP->flags & OF_ATTACHED) && (prevObjP->cType.explInfo.nAttachParent != -1))
					AttachObject (gameData.objs.objects + prevObjP->cType.explInfo.nAttachParent, objP);
				else
					objP->flags &= ~OF_ATTACHED;
				}
			else
				AttachObject (prevObjP, objP);
			}
		break;

	case CT_LIGHT:
		objP->cType.lightInfo.intensity = NDReadFix ();
		break;

	case CT_AI:
	case CT_WEAPON:
	case CT_NONE:
	case CT_FLYING:
	case CT_DEBRIS:
	case CT_POWERUP:
	case CT_SLEW:
	case CT_CNTRLCEN:
	case CT_REMOTE:
	case CT_MORPH:
		break;

	case CT_FLYTHROUGH:
	case CT_REPAIRCEN:
	default:
		Int3 ();
	}

switch (objP->renderType) {
	case RT_NONE:
		break;

	case RT_MORPH:
	case RT_POLYOBJ: {
		int i, tmo;
		if ((objP->nType != OBJ_ROBOT) && (objP->nType != OBJ_PLAYER) && (objP->nType != OBJ_CLUTTER)) {
			objP->rType.polyObjInfo.nModel = NDReadInt ();
			objP->rType.polyObjInfo.nSubObjFlags = NDReadInt ();
			}
		if ((objP->nType != OBJ_PLAYER) && (objP->nType != OBJ_DEBRIS))
		for (i = 0; i < gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nModels; i++)
			NDReadAngVec (objP->rType.polyObjInfo.animAngles + i);
		tmo = NDReadInt ();
#ifndef EDITOR
		objP->rType.polyObjInfo.nTexOverride = tmo;
#else
		if (tmo==-1)
			objP->rType.polyObjInfo.nTexOverride = -1;
		else {
			int xlated_tmo = tmap_xlate_table [tmo];
			if (xlated_tmo < 0) {
				Int3 ();
				xlated_tmo = 0;
				}
			objP->rType.polyObjInfo.nTexOverride = xlated_tmo;
			}
#endif
		break;
		}

	case RT_POWERUP:
	case RT_FIREBALL:
	case RT_HOSTAGE:
	case RT_WEAPON_VCLIP:
	case RT_THRUSTER:
		objP->rType.vClipInfo.nClipIndex = NDReadInt ();
		objP->rType.vClipInfo.xFrameTime = NDReadFix ();
		objP->rType.vClipInfo.nCurFrame = NDReadByte ();
		break;

	case RT_LASER:
		break;

	default:
		Int3 ();
	}
prevObjP = objP;
if (!bRevertFormat)
	bRevertFormat = gameOpts->demo.bRevertFormat;
}

//------------------------------------------------------------------------------
//process this powerup for this frame
void NDSetPowerupClip (tObject *objP)
{
//if (gameStates.app.tick40fps.bTick) 
	tVClipInfo	*vciP = &objP->rType.vClipInfo;
	tVideoClip	*vcP = gameData.eff.vClips [0] + vciP->nClipIndex;

vciP->nCurFrame = ((gameData.time.xGame - gameData.demo.xStartTime) / vcP->xFrameTime) % vcP->nFrameCount;
}

//	-----------------------------------------------------------------------------

void NDWriteObject (tObject *objP)
{
	int		life;
	tObject	o = *objP;

if ((o.renderType > RT_WEAPON_VCLIP) && ((gameStates.app.bNostalgia || gameOpts->demo.bOldFormat)))
	return;
if ((o.nType == OBJ_ROBOT) && (o.id == SPECIAL_REACTOR_ROBOT))
	Int3 ();
// Do renderType first so on read, we can make determination of
// what else to read in
if ((o.nType == OBJ_POWERUP) && (o.renderType == RT_POLYOBJ)) {
	ConvertWeaponToPowerup (&o);
	NDSetPowerupClip (&o);
	}
NDWriteByte (o.renderType);
NDWriteByte (o.nType);
if ((o.renderType == RT_NONE) && (o.nType != OBJ_CAMERA))
	return;
NDWriteByte (o.id);
if (!(gameStates.app.bNostalgia || gameOpts->demo.bOldFormat))
	NDWriteFix (o.shields);
NDWriteByte (o.flags);
NDWriteShort ((short) o.nSignature);
NDWritePosition (&o);
if ((o.nType != OBJ_HOSTAGE) && (o.nType != OBJ_ROBOT) && (o.nType != OBJ_PLAYER) && (o.nType != OBJ_POWERUP) && (o.nType != OBJ_CLUTTER)) {
	NDWriteByte (o.controlType);
	NDWriteByte (o.movementType);
	NDWriteFix (o.size);
	}
else if (o.nType == OBJ_POWERUP)
	NDWriteByte (o.movementType);
NDWriteVector (&o.vLastPos);
if ((o.nType == OBJ_WEAPON) && (o.renderType == RT_WEAPON_VCLIP))
	NDWriteFix (o.lifeleft);
else {
	life = ((int) o.lifeleft) >> 12;
	if (life > 255)
		life = 255;
	NDWriteByte ((ubyte)life);
	}
if (o.nType == OBJ_ROBOT) {
	if (ROBOTINFO (o.id).bossFlag) {
		int i = FindBoss (OBJ_IDX (objP));
		if ((i >= 0) &&
			 (gameData.time.xGame > gameData.boss [i].nCloakStartTime) && 
			 (gameData.time.xGame < gameData.boss [i].nCloakEndTime))
			NDWriteByte (1);
		else
			NDWriteByte (0);
		}
	}
switch (o.movementType) {
	case MT_PHYSICS:
		NDWriteVector (&o.mType.physInfo.velocity);
		NDWriteVector (&o.mType.physInfo.thrust);
		break;

	case MT_SPINNING:
		NDWriteVector (&o.mType.spinRate);
		break;

	case MT_NONE:
		break;

	default:
		Int3 ();
	}

switch (o.controlType) {
	case CT_AI:
		break;

	case CT_EXPLOSION:
		NDWriteFix (o.cType.explInfo.nSpawnTime);
		NDWriteFix (o.cType.explInfo.nDeleteTime);
		NDWriteShort (o.cType.explInfo.nDeleteObj);
		break;

	case CT_WEAPON:
		break;

	case CT_LIGHT:
		NDWriteFix (o.cType.lightInfo.intensity);
		break;

	case CT_NONE:
	case CT_FLYING:
	case CT_DEBRIS:
	case CT_POWERUP:
	case CT_SLEW:       //the tPlayer is generally saved as slew
	case CT_CNTRLCEN:
	case CT_REMOTE:
	case CT_MORPH:
		break;

	case CT_REPAIRCEN:
	case CT_FLYTHROUGH:
	default:
		Int3 ();
	}

switch (o.renderType) {
	case RT_NONE:
		break;

	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
		if ((o.nType != OBJ_ROBOT) && (o.nType != OBJ_PLAYER) && (o.nType != OBJ_CLUTTER)) {
			NDWriteInt (o.rType.polyObjInfo.nModel);
			NDWriteInt (o.rType.polyObjInfo.nSubObjFlags);
			}
		if ((o.nType != OBJ_PLAYER) && (o.nType != OBJ_DEBRIS))
#if 0
			for (i=0;i<MAX_SUBMODELS;i++)
				NDWriteAngVec (&o.polyObjInfo.animAngles [i]);
#endif
		for (i = 0; i < gameData.models.polyModels [o.rType.polyObjInfo.nModel].nModels; i++)
			NDWriteAngVec (&o.rType.polyObjInfo.animAngles [i]);
		NDWriteInt (o.rType.polyObjInfo.nTexOverride);
		break;
		}

	case RT_POWERUP:
	case RT_FIREBALL:
	case RT_HOSTAGE:
	case RT_WEAPON_VCLIP:
	case RT_THRUSTER:
		NDWriteInt (o.rType.vClipInfo.nClipIndex);
		NDWriteFix (o.rType.vClipInfo.xFrameTime);
		NDWriteByte (o.rType.vClipInfo.nCurFrame);
		break;

	case RT_LASER:
		break;

	default:
		Int3 ();
	}
}

//	-----------------------------------------------------------------------------

int	bJustStartedRecording = 0, 
		bJustStartedPlayback = 0;

void NDRecordStartDemo ()
{
	int i;

StopTime ();
NDWriteByte (ND_EVENT_START_DEMO);
NDWriteByte ((gameStates.app.bNostalgia || gameOpts->demo.bOldFormat) ? DEMO_VERSION : DEMO_VERSION_D2X);
NDWriteByte (DEMO_GAME_TYPE);
NDWriteFix (gameData.time.xGame);
if (gameData.demo.nGameMode & GM_MULTI)
	NDWriteInt (gameData.app.nGameMode | (gameData.multiplayer.nLocalPlayer << 16));
else
	// NOTE LINK TO ABOVE!!!
	NDWriteInt (gameData.app.nGameMode);
if (IsTeamGame) {
	NDWriteByte (netGame.teamVector);
	NDWriteString (netGame.team_name [0]);
	NDWriteString (netGame.team_name [1]);
	}
if (gameData.demo.nGameMode & GM_MULTI) {
	NDWriteByte ((sbyte)gameData.multiplayer.nPlayers);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		NDWriteString (gameData.multiplayer.players [i].callsign);
		NDWriteByte (gameData.multiplayer.players [i].connected);
		if (gameData.app.nGameMode & GM_MULTI_COOP)
			NDWriteInt (gameData.multiplayer.players [i].score);
		else {
			NDWriteShort ((short)gameData.multiplayer.players [i].netKilledTotal);
			NDWriteShort ((short)gameData.multiplayer.players [i].netKillsTotal);
			}
		}
	} 
else
	// NOTE LINK TO ABOVE!!!
	NDWriteInt (LOCALPLAYER.score);
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	NDWriteShort ((short)LOCALPLAYER.primaryAmmo [i]);
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	NDWriteShort ((short)LOCALPLAYER.secondaryAmmo [i]);
NDWriteByte ((sbyte)LOCALPLAYER.laserLevel);
//  Support for missions added here
NDWriteString (gameStates.app.szCurrentMissionFile);
NDWriteByte ((sbyte) (f2ir (LOCALPLAYER.energy)));
NDWriteByte ((sbyte) (f2ir (LOCALPLAYER.shields)));
NDWriteInt (LOCALPLAYER.flags);        // be sure players flags are set
NDWriteByte ((sbyte)gameData.weapons.nPrimary);
NDWriteByte ((sbyte)gameData.weapons.nSecondary);
gameData.demo.nStartFrame = gameData.app.nFrameCount;
bJustStartedRecording = 1;
NDSetNewLevel (gameData.missions.nCurrentLevel);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordStartFrame (int nFrameNumber, fix xFrameTime)
{
if (gameData.demo.bNoSpace) {
	NDStopPlayback ();
	return;
	}
StopTime ();
memset (gameData.demo.bWasRecorded, 0, sizeof (*gameData.demo.bWasRecorded) * MAX_OBJECTS);
memset (gameData.demo.bViewWasRecorded, 0, sizeof (*gameData.demo.bViewWasRecorded) * MAX_OBJECTS);
memset (gameData.demo.bRenderingWasRecorded, 0, sizeof (gameData.demo.bRenderingWasRecorded));
nFrameNumber -= gameData.demo.nStartFrame;
Assert (nFrameNumber >= 0);
NDWriteByte (ND_EVENT_START_FRAME);
NDWriteShort ((short) (gameData.demo.nFrameBytesWritten - 1));        // from previous frame
gameData.demo.nFrameBytesWritten = 3;
NDWriteInt (nFrameNumber);
NDWriteInt (xFrameTime);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordRenderObject (tObject * objP)
{
if (gameData.demo.bViewWasRecorded [OBJ_IDX (objP)])
	return;
//if (obj==&gameData.objs.objects [LOCALPLAYER.nObject] && !gameStates.app.bPlayerIsDead)
//	return;
StopTime ();
NDWriteByte (ND_EVENT_RENDER_OBJECT);
NDWriteObject (objP);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordViewerObject (tObject * objP)
{
	int	i = OBJ_IDX (objP);
	int	h = gameData.demo.bViewWasRecorded [i];
if (h && (h - 1 == gameStates.render.nRenderingType))
	return;
//if (gameData.demo.bWasRecorded [OBJ_IDX (objP)])
//	return;
if (gameData.demo.bRenderingWasRecorded [gameStates.render.nRenderingType])
	return;
gameData.demo.bViewWasRecorded [i]=gameStates.render.nRenderingType+1;
gameData.demo.bRenderingWasRecorded [gameStates.render.nRenderingType]=1;
StopTime ();
NDWriteByte (ND_EVENT_VIEWER_OBJECT);
NDWriteByte (gameStates.render.nRenderingType);
NDWriteObject (objP);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordSound (int soundno)
{
StopTime ();
NDWriteByte (ND_EVENT_SOUND);
NDWriteInt (soundno);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordCockpitChange (int mode)
{
StopTime ();
NDWriteByte (ND_EVENT_CHANGE_COCKPIT);
NDWriteInt (mode);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordSound3D (int soundno, int angle, int volume)
{
StopTime ();
NDWriteByte (ND_EVENT_SOUND_3D);
NDWriteInt (soundno);
NDWriteInt (angle);
NDWriteInt (volume);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordSound3DOnce (int soundno, int angle, int volume)
{
StopTime ();
NDWriteByte (ND_EVENT_SOUND_3D_ONCE);
NDWriteInt (soundno);
NDWriteInt (angle);
NDWriteInt (volume);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordLinkSoundToObject3 (int soundno, short nObject, fix maxVolume, fix  maxDistance, int loop_start, int loop_end)
{
StopTime ();
NDWriteByte (ND_EVENT_LINK_SOUND_TO_OBJ);
NDWriteInt (soundno);
NDWriteInt (gameData.objs.objects [nObject].nSignature);
NDWriteInt (maxVolume);
NDWriteInt (maxDistance);
NDWriteInt (loop_start);
NDWriteInt (loop_end);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordKillSoundLinkedToObject (int nObject)
{
StopTime ();
NDWriteByte (ND_EVENT_KILL_SOUND_TO_OBJ);
NDWriteInt (gameData.objs.objects [nObject].nSignature);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordWallHitProcess (int nSegment, int nSide, int damage, int playernum)
{
StopTime ();
//nSegment = nSegment;
//nSide = nSide;
//damage = damage;
//playernum = playernum;
NDWriteByte (ND_EVENT_WALL_HIT_PROCESS);
NDWriteInt (nSegment);
NDWriteInt (nSide);
NDWriteInt (damage);
NDWriteInt (playernum);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordGuidedStart ()
{
NDWriteByte (ND_EVENT_START_GUIDED);
}

//	-----------------------------------------------------------------------------

void NDRecordGuidedEnd ()
{
NDWriteByte (ND_EVENT_END_GUIDED);
}

//	-----------------------------------------------------------------------------

void NDRecordSecretExitBlown (int truth)
{
StopTime ();
NDWriteByte (ND_EVENT_SECRET_THINGY);
NDWriteInt (truth);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordTrigger (int nSegment, int nSide, int nObject, int shot)
{
StopTime ();
NDWriteByte (ND_EVENT_TRIGGER);
NDWriteInt (nSegment);
NDWriteInt (nSide);
NDWriteInt (nObject);
NDWriteInt (shot);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordHostageRescued (int hostage_number) 
{
StopTime ();
NDWriteByte (ND_EVENT_HOSTAGE_RESCUED);
NDWriteInt (hostage_number);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMorphFrame (tMorphInfo *md)
{
StopTime ();
NDWriteByte (ND_EVENT_MORPH_FRAME);
NDWriteObject (md->objP);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordWallToggle (int nSegment, int nSide)
{
StopTime ();
NDWriteByte (ND_EVENT_WALL_TOGGLE);
NDWriteInt (nSegment);
NDWriteInt (nSide);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordControlCenterDestroyed ()
{
StopTime ();
NDWriteByte (ND_EVENT_CONTROL_CENTER_DESTROYED);
NDWriteInt (gameData.reactor.countdown.nSecsLeft);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordHUDMessage (char * message)
{
StopTime ();
NDWriteByte (ND_EVENT_HUD_MESSAGE);
NDWriteString (message);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPaletteEffect (short r, short g, short b)
{
StopTime ();
NDWriteByte (ND_EVENT_PALETTE_EFFECT);
NDWriteShort (r);
NDWriteShort (g);
NDWriteShort (b);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerEnergy (int old_energy, int energy)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_ENERGY);
NDWriteByte ((sbyte) old_energy);
NDWriteByte ((sbyte) energy);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerAfterburner (fix old_afterburner, fix afterburner)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_AFTERBURNER);
NDWriteByte ((sbyte) (old_afterburner>>9));
NDWriteByte ((sbyte) (afterburner>>9));
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerShields (int old_shield, int shield)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_SHIELD);
NDWriteByte ((sbyte)old_shield);
NDWriteByte ((sbyte)shield);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerFlags (uint oflags, uint flags)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_FLAGS);
NDWriteInt (( (short)oflags << 16) | (short)flags);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerWeapon (int nWeaponType, int weapon_num)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_WEAPON);
NDWriteByte ((sbyte)nWeaponType);
NDWriteByte ((sbyte)weapon_num);
if (nWeaponType)
	NDWriteByte ((sbyte)gameData.weapons.nSecondary);
else
	NDWriteByte ((sbyte)gameData.weapons.nPrimary);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordEffectBlowup (short tSegment, int nSide, vmsVector *pnt)
{
StopTime ();
NDWriteByte (ND_EVENT_EFFECT_BLOWUP);
NDWriteShort (tSegment);
NDWriteByte ((sbyte)nSide);
NDWriteVector (pnt);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordHomingDistance (fix distance)
{
StopTime ();
NDWriteByte (ND_EVENT_HOMING_DISTANCE);
NDWriteShort ((short) (distance>>16));
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordLetterbox (void)
{
StopTime ();
NDWriteByte (ND_EVENT_LETTERBOX);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordRearView (void)
{
StopTime ();
NDWriteByte (ND_EVENT_REARVIEW);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordRestoreCockpit (void)
{
StopTime ();
NDWriteByte (ND_EVENT_RESTORE_COCKPIT);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordRestoreRearView (void)
{
StopTime ();
NDWriteByte (ND_EVENT_RESTORE_REARVIEW);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordWallSetTMapNum1 (short nSegment, ubyte nSide, short nConnSeg, ubyte nConnSide, short tmap)
{
StopTime ();
NDWriteByte (ND_EVENT_WALL_SET_TMAP_NUM1);
NDWriteShort (nSegment);
NDWriteByte (nSide);
NDWriteShort (nConnSeg);
NDWriteByte (nConnSide);
NDWriteShort (tmap);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordWallSetTMapNum2 (short nSegment, ubyte nSide, short nConnSeg, ubyte nConnSide, short tmap)
{
StopTime ();
NDWriteByte (ND_EVENT_WALL_SET_TMAP_NUM2);
NDWriteShort (nSegment);
NDWriteByte (nSide);
NDWriteShort (nConnSeg);
NDWriteByte (nConnSide);
NDWriteShort (tmap);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiCloak (int nPlayer)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_CLOAK);
NDWriteByte ((sbyte)nPlayer);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiDeCloak (int nPlayer)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_DECLOAK);
NDWriteByte ((sbyte)nPlayer);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiDeath (int nPlayer)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_DEATH);
NDWriteByte ((sbyte)nPlayer);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiKill (int nPlayer, sbyte kill)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_KILL);
NDWriteByte ((sbyte)nPlayer);
NDWriteByte (kill);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiConnect (int nPlayer, int nNewPlayer, char *pszNewCallsign)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_CONNECT);
NDWriteByte ((sbyte)nPlayer);
NDWriteByte ((sbyte)nNewPlayer);
if (!nNewPlayer) {
	NDWriteString (gameData.multiplayer.players [nPlayer].callsign);
	NDWriteInt (gameData.multiplayer.players [nPlayer].netKilledTotal);
	NDWriteInt (gameData.multiplayer.players [nPlayer].netKillsTotal);
	}
NDWriteString (pszNewCallsign);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiReconnect (int nPlayer)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_RECONNECT);
NDWriteByte ((sbyte)nPlayer);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiDisconnect (int nPlayer)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_DISCONNECT);
NDWriteByte ((sbyte)nPlayer);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerScore (int score)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_SCORE);
NDWriteInt (score);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordMultiScore (int nPlayer, int score)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_SCORE);
NDWriteByte ((sbyte)nPlayer);
NDWriteInt (score - gameData.multiplayer.players [nPlayer].score);      // called before score is changed!!!!
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordPrimaryAmmo (int nOldAmmo, int nNewAmmo)
{
StopTime ();
NDWriteByte (ND_EVENT_PRIMARY_AMMO);
if (nOldAmmo < 0)
	NDWriteShort ((short)nNewAmmo);
else
	NDWriteShort ((short)nOldAmmo);
NDWriteShort ((short)nNewAmmo);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordSecondaryAmmo (int nOldAmmo, int nNewAmmo)
{
StopTime ();
NDWriteByte (ND_EVENT_SECONDARY_AMMO);
if (nOldAmmo < 0)
	NDWriteShort ((short)nNewAmmo);
else
	NDWriteShort ((short)nOldAmmo);
NDWriteShort ((short)nNewAmmo);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordDoorOpening (int nSegment, int nSide)
{
StopTime ();
NDWriteByte (ND_EVENT_DOOR_OPENING);
NDWriteShort ((short)nSegment);
NDWriteByte ((sbyte)nSide);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordLaserLevel (sbyte oldLevel, sbyte newLevel)
{
StopTime ();
NDWriteByte (ND_EVENT_LASER_LEVEL);
NDWriteByte (oldLevel);
NDWriteByte (newLevel);
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDRecordCloakingWall (int nFrontWall, int nBackWall, ubyte nType, ubyte state, fix cloakValue, fix l0, fix l1, fix l2, fix l3)
{
Assert (nFrontWall <= 255 && nBackWall <= 255);
StopTime ();
NDWriteByte (ND_EVENT_CLOAKING_WALL);
NDWriteByte ((sbyte) nFrontWall);
NDWriteByte ((sbyte) nBackWall);
NDWriteByte ((sbyte) nType);
NDWriteByte ((sbyte) state);
NDWriteByte ((sbyte) cloakValue);
NDWriteShort ((short) (l0>>8));
NDWriteShort ((short) (l1>>8));
NDWriteShort ((short) (l2>>8));
NDWriteShort ((short) (l3>>8));
StartTime ();
}

//	-----------------------------------------------------------------------------

void NDSetNewLevel (int level_num)
{
	int i;
	int nSide;
	tSegment *segP;

StopTime ();
NDWriteByte (ND_EVENT_NEW_LEVEL);
NDWriteByte ((sbyte)level_num);
NDWriteByte ((sbyte)gameData.missions.nCurrentLevel);

if (bJustStartedRecording==1) {
	NDWriteInt (gameData.walls.nWalls);
	for (i = 0; i < gameData.walls.nWalls; i++) {
		NDWriteByte (gameData.walls.walls [i].nType);
		NDWriteByte (gameData.walls.walls [i].flags);
		NDWriteByte (gameData.walls.walls [i].state);
		segP = &gameData.segs.segments [gameData.walls.walls [i].nSegment];
		nSide = gameData.walls.walls [i].nSide;
		NDWriteShort (segP->sides [nSide].nBaseTex);
		NDWriteShort (segP->sides [nSide].nOvlTex | (segP->sides [nSide].nOvlOrient << 14));
		bJustStartedRecording=0;
		}
	}
StartTime ();
}

//	-----------------------------------------------------------------------------

int NDReadDemoStart (int bRandom)
{
	sbyte	i, gameType, laserLevel;
	char	c, energy, shield;
	int	nVersionFilter;
	char	szMsg [128], szCurrentMission [FILENAME_LEN];

i = CFTell (&ndInFile);
c = NDReadByte ();
if ((c != ND_EVENT_START_DEMO) || bNDBadRead) {
	sprintf (szMsg, "%s %s", TXT_CANT_PLAYBACK, TXT_DEMO_CORRUPT);
	return NDErrorMsg (szMsg, NULL, NULL);
	}
if (bRevertFormat > 0)
	bRevertFormat = 0;
gameData.demo.nVersion = NDReadByte ();
if (gameOpts->demo.bRevertFormat && !bRevertFormat) {
	if (gameData.demo.nVersion == DEMO_VERSION)
		bRevertFormat = -1;
	else {
		NDWriteByte (DEMO_VERSION);
		bRevertFormat = 1;
		}
	}
gameType = NDReadByte ();
if (gameType < DEMO_GAME_TYPE) {
	sprintf (szMsg, "%s %s", TXT_CANT_PLAYBACK, TXT_RECORDED);
	return NDErrorMsg (szMsg, "    In Descent: First Strike", NULL);
	}
if (gameType != DEMO_GAME_TYPE) {
	sprintf (szMsg, "%s %s", TXT_CANT_PLAYBACK, TXT_RECORDED);
	return NDErrorMsg (szMsg, "   In Unknown Descent version", NULL);
	}
if (gameData.demo.nVersion < DEMO_VERSION) {
	if (bRandom)
		return 1;
	sprintf (szMsg, "%s %s", TXT_CANT_PLAYBACK, TXT_DEMO_OLD);
	return NDErrorMsg (szMsg, NULL, NULL);
	}
gameData.demo.bUseShortPos = (gameData.demo.nVersion == DEMO_VERSION);
gameData.time.xGame = NDReadFix ();
for (i = 0; i < MAX_BOSS_COUNT; i++)
	gameData.boss [i].nCloakStartTime =
	gameData.boss [i].nCloakEndTime = gameData.time.xGame;
gameData.demo.xJasonPlaybackTotal = 0;
gameData.demo.nGameMode = NDReadInt ();
ChangePlayerNumTo ((gameData.demo.nGameMode >> 16) & 0x7);
if (gameData.demo.nGameMode & GM_TEAM) {
	netGame.teamVector = NDReadByte ();
	NDReadString (netGame.team_name [0]);
	NDReadString (netGame.team_name [1]);
	}
if (gameData.demo.nGameMode & GM_MULTI) {
	MultiNewGame ();
	c = NDReadByte ();
	gameData.multiplayer.nPlayers = (int)c;
	// changed this to above two lines -- breaks on the mac because of
	// endian issues
	//		NDReadByte ((sbyte *)&gameData.multiplayer.nPlayers);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		gameData.multiplayer.players [i].cloakTime = 0;
		gameData.multiplayer.players [i].invulnerableTime = 0;
		NDReadString (gameData.multiplayer.players [i].callsign);
		gameData.multiplayer.players [i].connected = (sbyte) NDReadByte ();
		if (IsCoopGame)
			gameData.multiplayer.players [i].score = NDReadInt ();
		else {
			gameData.multiplayer.players [i].netKilledTotal = NDReadShort ();
			gameData.multiplayer.players [i].netKillsTotal = NDReadShort ();
			}
		}
	gameData.app.nGameMode = gameData.demo.nGameMode;
	MultiSortKillList ();
	gameData.app.nGameMode = GM_NORMAL;
	}
else
	LOCALPLAYER.score = NDReadInt ();      // Note link to above if!
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	LOCALPLAYER.primaryAmmo [i] = NDReadShort ();
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		LOCALPLAYER.secondaryAmmo [i] = NDReadShort ();
laserLevel = NDReadByte ();
if (laserLevel != LOCALPLAYER.laserLevel) {
	LOCALPLAYER.laserLevel = laserLevel;
	UpdateLaserWeaponInfo ();
	}
// Support for missions
i = CFTell (&ndInFile);
NDReadString (szCurrentMission);
nVersionFilter = gameOpts->app.nVersionFilter;
gameOpts->app.nVersionFilter = 3;	//make sure mission will be loaded
i = LoadMissionByName (szCurrentMission, -1);
gameOpts->app.nVersionFilter = nVersionFilter;
if (!i) {
	if (bRandom)
		return 1;
	sprintf (szMsg, TXT_NOMISSION4DEMO, szCurrentMission);
	return NDErrorMsg (szMsg, NULL, NULL);
	}
gameData.demo.xRecordedTotal = 0;
gameData.demo.xPlaybackTotal = 0;
energy = NDReadByte ();
shield = NDReadByte ();
LOCALPLAYER.flags = NDReadInt ();
if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
	LOCALPLAYER.cloakTime = gameData.time.xGame - (CLOAK_TIME_MAX / 2);
	gameData.demo.bPlayersCloaked |= (1 << gameData.multiplayer.nLocalPlayer);
	}
if (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)
	LOCALPLAYER.invulnerableTime = gameData.time.xGame - (INVULNERABLE_TIME_MAX / 2);
gameData.weapons.nPrimary = NDReadByte ();
gameData.weapons.nSecondary = NDReadByte ();
// Next bit of code to fix problem that I introduced between 1.0 and 1.1
// check the next byte -- it _will_ be a load_newLevel event.  If it is
// not, then we must shift all bytes up by one.
LOCALPLAYER.energy = i2f (energy);
LOCALPLAYER.shields = i2f (shield);
bJustStartedPlayback=1;
return 0;
}

//	-----------------------------------------------------------------------------

void NDPopCtrlCenTriggers ()
{
	short		anim_num, n, i;
	short		side, nConnSide;
	tSegment *segP, *connSegP;

for (i = 0; i < gameData.reactor.triggers.nLinks; i++) {
	segP = gameData.segs.segments + gameData.reactor.triggers.nSegment [i];
	side = gameData.reactor.triggers.nSide [i];
	connSegP = gameData.segs.segments + segP->children [side];
	nConnSide = FindConnectedSide (segP, connSegP);
	anim_num = gameData.walls.walls [WallNumP (segP, side)].nClip;
	n = gameData.walls.pAnims [anim_num].nFrameCount;
	if (gameData.walls.pAnims [anim_num].flags & WCF_TMAP1)
		segP->sides [side].nBaseTex = 
		connSegP->sides [nConnSide].nBaseTex = gameData.walls.pAnims [anim_num].frames [n-1];
	else
		segP->sides [side].nOvlTex = 
		connSegP->sides [nConnSide].nOvlTex = gameData.walls.pAnims [anim_num].frames [n-1];
	}
}

//	-----------------------------------------------------------------------------

int NDUpdateSmoke (void)
{
if (!EGI_FLAG (bUseSmoke, 0, 1, 0))
	return 0;
else {
		int		i, nObject;
		tSmoke	*pSmoke = gameData.smoke.buffer;

	for (i = gameData.smoke.iUsed; i >= 0; i = pSmoke->nNext) {
		pSmoke = gameData.smoke.buffer + i;
		nObject = NDFindObject (pSmoke->nSignature);
		if (nObject < 0) {
			gameData.smoke.objects [pSmoke->nObject] = -1;
			SetSmokeLife (i, 0);
			}
		else {
			gameData.smoke.objects [nObject] = i;
			pSmoke->nObject = nObject;
			}
		}
	return 1;
	}
}

//	-----------------------------------------------------------------------------

#define N_PLAYER_SHIP_TEXTURES 6

void NDRenderExtras (ubyte, tObject *); extern void MultiApplyGoalTextures ();

int NDReadFrameInfo ()
{
	int bDone, nSegment, nTexture, nSide, nObject, soundno, angle, volume, i, shot;
	tObject *objP;
	ubyte c, WhichWindow;
	static sbyte saved_letter_cockpit;
	static sbyte saved_rearview_cockpit;
	tObject extraobj;
	static char LastReadValue=101;
	tSegment *segP;

bDone = 0;
if (gameData.demo.nVcrState != ND_STATE_PAUSED)
	for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++)
		gameData.segs.segments [nSegment].objects = -1;
ResetObjects (1);
LOCALPLAYER.homingObjectDist = -F1_0;
prevObjP = NULL;
while (!bDone) {
	c = NDReadByte ();
	CATCH_BAD_READ
	switch (c) {
		case ND_EVENT_START_FRAME: {        // Followed by an integer frame number, then a fix gameData.time.xFrame
			short nPrevFrameLength;

			bDone = 1;
			//LogErr ("%4d %4d %d\n", gameData.demo.nFrameCount, gameData.demo.nFrameBytesWritten - 1, CFTell (&ndOutFile);
			if (bRevertFormat > 0)
				bRevertFormat = 0;
			nPrevFrameLength = NDReadShort ();
			if (!bRevertFormat) {
				NDWriteShort (gameData.demo.nFrameBytesWritten - 1);
				bRevertFormat = 1;
				}
			if (bRevertFormat > 0)
				gameData.demo.nFrameBytesWritten = 3;
			gameData.demo.nFrameCount = NDReadInt ();
			gameData.demo.xRecordedTime = NDReadInt ();
			if (gameData.demo.nVcrState == ND_STATE_PLAYBACK)
				gameData.demo.xRecordedTotal += gameData.demo.xRecordedTime;
			gameData.demo.nFrameCount--;
			CATCH_BAD_READ
			break;
			}

		case ND_EVENT_VIEWER_OBJECT:        // Followed by an tObject structure
			WhichWindow = NDReadByte ();
			if (WhichWindow&15) {
				NDReadObject (&extraobj);
				if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
					CATCH_BAD_READ
					NDRenderExtras (WhichWindow, &extraobj);
					}
				}
			else {
				//gameData.objs.viewer=&gameData.objs.objects [0];
				NDReadObject (gameData.objs.viewer);
				if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
					CATCH_BAD_READ
					nSegment = gameData.objs.viewer->nSegment;
					gameData.objs.viewer->next = 
					gameData.objs.viewer->prev = 
					gameData.objs.viewer->nSegment = -1;

					// HACK HACK HACK -- since we have multiple level recording, it can be the case
					// HACK HACK HACK -- that when rewinding the demo, the viewer is in a tSegment
					// HACK HACK HACK -- that is greater than the highest index of segments.  Bash
					// HACK HACK HACK -- the viewer to tSegment 0 for bogus view.

					if (nSegment > gameData.segs.nLastSegment)
						nSegment = 0;
					LinkObject (OBJ_IDX (gameData.objs.viewer), nSegment);
					}
				}
			break;

		case ND_EVENT_RENDER_OBJECT:       // Followed by an tObject structure
			nObject = AllocObject ();
			if (nObject == -1)
				break;
			objP = gameData.objs.objects + nObject;
			NDReadObject (objP);
			CATCH_BAD_READ
			if (objP->controlType == CT_POWERUP)
				DoPowerupFrame (objP);
			if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
				nSegment = objP->nSegment;
				objP->next = objP->prev = objP->nSegment = -1;
				// HACK HACK HACK -- don't render gameData.objs.objects is segments greater than gameData.segs.nLastSegment
				// HACK HACK HACK -- (see above)
				if (nSegment > gameData.segs.nLastSegment)
					break;
				LinkObject (OBJ_IDX (objP), nSegment);
				if ((objP->nType == OBJ_PLAYER) && (gameData.demo.nGameMode & GM_MULTI)) {
					int tPlayer = (gameData.demo.nGameMode & GM_TEAM) ? GetTeam (objP->id) : objP->id;
					if (tPlayer == 0)
						break;
					tPlayer--;
					for (i = 0; i < N_PLAYER_SHIP_TEXTURES; i++)
						MultiPlayerTextures [tPlayer] [i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nFirstTexture+i]];
					MultiPlayerTextures [tPlayer] [4] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [gameData.pig.tex.nFirstMultiBitmap+ (tPlayer)*2]];
					MultiPlayerTextures [tPlayer] [5] = gameData.pig.tex.objBmIndex [gameData.pig.tex.pObjBmIndex [gameData.pig.tex.nFirstMultiBitmap+ (tPlayer)*2+1]];
					objP->rType.polyObjInfo.nAltTextures = tPlayer+1;
					}
				}
			break;

		case ND_EVENT_SOUND:
			soundno = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState == ND_STATE_PLAYBACK)
				DigiPlaySample ((short) soundno, F1_0);
			break;

			//--unused		case ND_EVENT_SOUND_ONCE:
			//--unused			NDReadInt (&soundno);
			//--unused			if (bNDBadRead) { bDone = -1; break; }
			//--unused			if (gameData.demo.nVcrState == ND_STATE_PLAYBACK)
			//--unused				DigiPlaySampleOnce (soundno, F1_0);
			//--unused			break;

		case ND_EVENT_SOUND_3D:
			soundno = NDReadInt ();
			angle = NDReadInt ();
			volume = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState == ND_STATE_PLAYBACK)
				DigiPlaySample3D ((short) soundno, angle, volume, 0, NULL, NULL);
			break;

		case ND_EVENT_SOUND_3D_ONCE:
			soundno = NDReadInt ();
			angle = NDReadInt ();
			volume = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState == ND_STATE_PLAYBACK)
				DigiPlaySample3D ((short) soundno, angle, volume, 1, NULL, NULL);
			break;

		case ND_EVENT_LINK_SOUND_TO_OBJ: {
				int soundno, nObject, maxVolume, maxDistance, loop_start, loop_end;
				int nSignature;

			soundno = NDReadInt ();
			nSignature = NDReadInt ();
			maxVolume = NDReadInt ();
			maxDistance = NDReadInt ();
			loop_start = NDReadInt ();
			loop_end = NDReadInt ();
			nObject = NDFindObject (nSignature);
			if (nObject > -1)   //  @mk, 2/22/96, John told me to.
				DigiLinkSoundToObject3 ((short) soundno, (short) nObject, 1, maxVolume, maxDistance, loop_start, loop_end, NULL, 0);
			}
			break;

		case ND_EVENT_KILL_SOUND_TO_OBJ: {
				int nObject, nSignature;

			nSignature = NDReadInt ();
			nObject = NDFindObject (nSignature);
			if (nObject > -1)   //  @mk, 2/22/96, John told me to.
				DigiKillSoundLinkedToObject (nObject);
			}
			break;

		case ND_EVENT_WALL_HIT_PROCESS: {
				int tPlayer, nSegment;
				fix damage;

			nSegment = NDReadInt ();
			nSide = NDReadInt ();
			damage = NDReadFix ();
			tPlayer = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState != ND_STATE_PAUSED)
				WallHitProcess (&gameData.segs.segments [nSegment], (short) nSide, damage, tPlayer, &(gameData.objs.objects [0]));
			break;
		}

		case ND_EVENT_TRIGGER:
			nSegment = NDReadInt ();
			nSide = NDReadInt ();
			nObject = NDReadInt ();
			shot = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
				if (gameData.trigs.triggers [gameData.walls.walls [WallNumI ((short) nSegment, (short) nSide)].nTrigger].nType == TT_SECRET_EXIT) {
					int truth;

					c = NDReadByte ();
					Assert (c == ND_EVENT_SECRET_THINGY);
					truth = NDReadInt ();
					if (!truth)
						CheckTrigger (gameData.segs.segments + nSegment, (short) nSide, (short) nObject, shot);
					} 
				else
					CheckTrigger (gameData.segs.segments + nSegment, (short) nSide, (short) nObject, shot);
				}
			break;

		case ND_EVENT_HOSTAGE_RESCUED: {
				int hostage_number;

			hostage_number = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState != ND_STATE_PAUSED)
				hostage_rescue (hostage_number);
			}
			break;

		case ND_EVENT_MORPH_FRAME: {
			nObject = AllocObject ();
			if (nObject==-1)
				break;
			objP = gameData.objs.objects + nObject;
			NDReadObject (objP);
			objP->renderType = RT_POLYOBJ;
			if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
				CATCH_BAD_READ
				if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
					nSegment = objP->nSegment;
					objP->next = objP->prev = objP->nSegment = -1;
					LinkObject (OBJ_IDX (objP), nSegment);
					}
				}
			}
			break;

		case ND_EVENT_WALL_TOGGLE:
			nSegment = NDReadInt ();
			nSide = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState != ND_STATE_PAUSED)
				WallToggle (gameData.segs.segments + nSegment, (short) nSide);
			break;

		case ND_EVENT_CONTROL_CENTER_DESTROYED:
			gameData.reactor.countdown.nSecsLeft = NDReadInt ();
			gameData.reactor.bDestroyed = 1;
			CATCH_BAD_READ
			if (!gameData.demo.bCtrlcenDestroyed) {
				NDPopCtrlCenTriggers ();
				gameData.demo.bCtrlcenDestroyed = 1;
				//DoReactorDestroyedStuff (NULL);
				}
			break;

		case ND_EVENT_HUD_MESSAGE: {
			char hud_msg [60];

			NDReadString (hud_msg);
			CATCH_BAD_READ
			HUDInitMessage (hud_msg);
			}
			break;

		case ND_EVENT_START_GUIDED:
			gameData.demo.bFlyingGuided=1;
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.demo.bFlyingGuided=0;
			break;

		case ND_EVENT_END_GUIDED:
			gameData.demo.bFlyingGuided=0;
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.demo.bFlyingGuided=1;
			break;

		case ND_EVENT_PALETTE_EFFECT: {
			short r, g, b;

			r = NDReadShort ();
			g = NDReadShort ();
			b = NDReadShort ();
			CATCH_BAD_READ
			PALETTE_FLASH_SET (r, g, b);
			}
			break;

		case ND_EVENT_PLAYER_ENERGY: {
			ubyte energy;
			ubyte old_energy;

			old_energy = NDReadByte ();
			energy = NDReadByte ();
			CATCH_BAD_READ
			if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
				 (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				LOCALPLAYER.energy = i2f (energy);
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_energy != 255)
					LOCALPLAYER.energy = i2f (old_energy);
				}
			}
			break;

		case ND_EVENT_PLAYER_AFTERBURNER: {
			ubyte afterburner;
			ubyte old_afterburner;

			old_afterburner = NDReadByte ();
			afterburner = NDReadByte ();
			CATCH_BAD_READ
			if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
				 (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.physics.xAfterburnerCharge = afterburner<<9;
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_afterburner != 255)
					gameData.physics.xAfterburnerCharge = old_afterburner<<9;
				}
			break;
		}

		case ND_EVENT_PLAYER_SHIELD: {
			ubyte shield;
			ubyte old_shield;

			old_shield = NDReadByte ();
			shield = NDReadByte ();
			CATCH_BAD_READ
			if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
				 (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				LOCALPLAYER.shields = i2f (shield);
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_shield != 255)
					LOCALPLAYER.shields = i2f (old_shield);
				}
			}
			break;

		case ND_EVENT_PLAYER_FLAGS: {
			uint oflags;

			LOCALPLAYER.flags = NDReadInt ();
			CATCH_BAD_READ
			oflags = LOCALPLAYER.flags >> 16;
			LOCALPLAYER.flags &= 0xffff;

			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 ((gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD) && (oflags != 0xffff))) {
				if (!(oflags & PLAYER_FLAGS_CLOAKED) && (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)) {
					LOCALPLAYER.cloakTime = 0;
					gameData.demo.bPlayersCloaked &= ~ (1 << gameData.multiplayer.nLocalPlayer);
					}
				if ((oflags & PLAYER_FLAGS_CLOAKED) && !(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)) {
					LOCALPLAYER.cloakTime = gameData.time.xGame - (CLOAK_TIME_MAX / 2);
					gameData.demo.bPlayersCloaked |= (1 << gameData.multiplayer.nLocalPlayer);
					}
				if (!(oflags & PLAYER_FLAGS_INVULNERABLE) && (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE))
					LOCALPLAYER.invulnerableTime = 0;
				if ((oflags & PLAYER_FLAGS_INVULNERABLE) && !(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE))
					LOCALPLAYER.invulnerableTime = gameData.time.xGame - (INVULNERABLE_TIME_MAX / 2);
				LOCALPLAYER.flags = oflags;
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				if (!(oflags & PLAYER_FLAGS_CLOAKED) && (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)) {
					LOCALPLAYER.cloakTime = gameData.time.xGame - (CLOAK_TIME_MAX / 2);
					gameData.demo.bPlayersCloaked |= (1 << gameData.multiplayer.nLocalPlayer);
					}
				if ((oflags & PLAYER_FLAGS_CLOAKED) && !(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)) {
					LOCALPLAYER.cloakTime = 0;
					gameData.demo.bPlayersCloaked &= ~ (1 << gameData.multiplayer.nLocalPlayer);
					}
				if (!(oflags & PLAYER_FLAGS_INVULNERABLE) && (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE))
					LOCALPLAYER.invulnerableTime = gameData.time.xGame - (INVULNERABLE_TIME_MAX / 2);
				if ((oflags & PLAYER_FLAGS_INVULNERABLE) && !(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE))
					LOCALPLAYER.invulnerableTime = 0;
				}
			UpdateLaserWeaponInfo ();     // in case of quad laser change
			}
			break;

		case ND_EVENT_PLAYER_WEAPON: {
			sbyte nWeaponType, weapon_num;
			sbyte old_weapon;

			nWeaponType = NDReadByte ();
			weapon_num = NDReadByte ();
			old_weapon = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
				 (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				if (nWeaponType == 0)
					gameData.weapons.nPrimary = (int)weapon_num;
				else
					gameData.weapons.nSecondary = (int)weapon_num;
				}
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				if (nWeaponType == 0)
					gameData.weapons.nPrimary = (int)old_weapon;
				else
					gameData.weapons.nSecondary = (int)old_weapon;
				}
			}
			break;

		case ND_EVENT_EFFECT_BLOWUP: {
			short nSegment;
			sbyte nSide;
			vmsVector pnt;
			tObject dummy;

			//create a dummy tObject which will be the weapon that hits
			//the monitor. the blowup code wants to know who the parent of the
			//laser is, so create a laser whose parent is the tPlayer
			dummy.cType.laserInfo.parentType = OBJ_PLAYER;
			nSegment = NDReadShort ();
			nSide = NDReadByte ();
			NDReadVector (&pnt);
			if (gameData.demo.nVcrState != ND_STATE_PAUSED)
				CheckEffectBlowup (gameData.segs.segments + nSegment, nSide, &pnt, &dummy, 0);
			}
			break;

		case ND_EVENT_HOMING_DISTANCE: {
			short distance;

			distance = NDReadShort ();
			LOCALPLAYER.homingObjectDist = 
				i2f ((int) distance << 16);
			}
			break;

		case ND_EVENT_LETTERBOX:
			if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
				 (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				saved_letter_cockpit = gameStates.render.cockpit.nMode;
				SelectCockpit (CM_LETTERBOX);
				}
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				SelectCockpit (saved_letter_cockpit);
			break;

		case ND_EVENT_CHANGE_COCKPIT: {
				int dummy;

			dummy = NDReadInt ();
			SelectCockpit (dummy);
			}
			break;

		case ND_EVENT_REARVIEW:
			if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
				 (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				saved_rearview_cockpit = gameStates.render.cockpit.nMode;
				if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT)
					SelectCockpit (CM_REAR_VIEW);
				gameStates.render.bRearView=1;
				}
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				if (saved_rearview_cockpit == CM_REAR_VIEW)     // hack to be sure we get a good cockpit on restore
					saved_rearview_cockpit = CM_FULL_COCKPIT;
				SelectCockpit (saved_rearview_cockpit);
				gameStates.render.bRearView = 0;
				}
			break;

		case ND_EVENT_RESTORE_COCKPIT:
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				saved_letter_cockpit = gameStates.render.cockpit.nMode;
				SelectCockpit (CM_LETTERBOX);
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				SelectCockpit (saved_letter_cockpit);
			break;


		case ND_EVENT_RESTORE_REARVIEW:
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				saved_rearview_cockpit = gameStates.render.cockpit.nMode;
				if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT)
					SelectCockpit (CM_REAR_VIEW);
				gameStates.render.bRearView=1;
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				if (saved_rearview_cockpit == CM_REAR_VIEW)     // hack to be sure we get a good cockpit on restore
					saved_rearview_cockpit = CM_FULL_COCKPIT;
				SelectCockpit (saved_rearview_cockpit);
				gameStates.render.bRearView=0;
				}
			break;


		case ND_EVENT_WALL_SET_TMAP_NUM1: {
			short segP, nConnSeg, tmap;
			ubyte nSide, nConnSide;

			segP = NDReadShort ();
			nSide = NDReadByte ();
			nConnSeg = NDReadShort ();
			nConnSide = NDReadByte ();
			tmap = NDReadShort ();
			if ((gameData.demo.nVcrState != ND_STATE_PAUSED) && 
				 (gameData.demo.nVcrState != ND_STATE_REWINDING) &&
				 (gameData.demo.nVcrState != ND_STATE_ONEFRAMEBACKWARD))
				gameData.segs.segments [segP].sides [nSide].nBaseTex = 
					gameData.segs.segments [nConnSeg].sides [nConnSide].nBaseTex = tmap;
			}
			break;

		case ND_EVENT_WALL_SET_TMAP_NUM2: {
			short segP, nConnSeg, tmap;
			ubyte nSide, nConnSide;

			segP = NDReadShort ();
			nSide = NDReadByte ();
			nConnSeg = NDReadShort ();
			nConnSide = NDReadByte ();
			tmap = NDReadShort ();
			if ((gameData.demo.nVcrState != ND_STATE_PAUSED) &&
				 (gameData.demo.nVcrState != ND_STATE_REWINDING) &&
				 (gameData.demo.nVcrState != ND_STATE_ONEFRAMEBACKWARD)) {
				Assert (tmap!=0 && gameData.segs.segments [segP].sides [nSide].nOvlTex!=0);
				gameData.segs.segments [segP].sides [nSide].nOvlTex = 
				gameData.segs.segments [nConnSeg].sides [nConnSide].nOvlTex = tmap & 0x3fff;
				gameData.segs.segments [segP].sides [nSide].nOvlOrient = 
				gameData.segs.segments [nConnSeg].sides [nConnSide].nOvlOrient = (tmap >> 14) & 3;
				}
			}
			break;

		case ND_EVENT_MULTI_CLOAK: {
			sbyte nPlayer = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				gameData.multiplayer.players [nPlayer].flags &= ~PLAYER_FLAGS_CLOAKED;
				gameData.multiplayer.players [nPlayer].cloakTime = 0;
				gameData.demo.bPlayersCloaked &= ~(1 << nPlayer);
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				gameData.multiplayer.players [nPlayer].flags |= PLAYER_FLAGS_CLOAKED;
				gameData.multiplayer.players [nPlayer].cloakTime = gameData.time.xGame  - (CLOAK_TIME_MAX / 2);
				gameData.demo.bPlayersCloaked |= (1 << nPlayer);
				}
			}
			break;

		case ND_EVENT_MULTI_DECLOAK: {
			sbyte nPlayer = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				gameData.multiplayer.players [nPlayer].flags |= PLAYER_FLAGS_CLOAKED;
				gameData.multiplayer.players [nPlayer].cloakTime = gameData.time.xGame  - (CLOAK_TIME_MAX / 2);
				gameData.demo.bPlayersCloaked |= (1 << nPlayer);
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				gameData.multiplayer.players [nPlayer].flags &= ~PLAYER_FLAGS_CLOAKED;
				gameData.multiplayer.players [nPlayer].cloakTime = 0;
				gameData.demo.bPlayersCloaked &= ~(1 << nPlayer);
				}
			}
			break;

		case ND_EVENT_MULTI_DEATH: {
			sbyte nPlayer = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.multiplayer.players [nPlayer].netKilledTotal--;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multiplayer.players [nPlayer].netKilledTotal++;
			}
			break;

		case ND_EVENT_MULTI_KILL: {
			sbyte nPlayer = NDReadByte ();
			sbyte kill = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				gameData.multiplayer.players [nPlayer].netKillsTotal -= kill;
				if (gameData.demo.nGameMode & GM_TEAM)
					gameData.multigame.kills.nTeam [GetTeam (nPlayer)] -= kill;
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				gameData.multiplayer.players [nPlayer].netKillsTotal += kill;
				if (gameData.demo.nGameMode & GM_TEAM)
					gameData.multigame.kills.nTeam [GetTeam (nPlayer)] += kill;
				}
			gameData.app.nGameMode = gameData.demo.nGameMode;
			MultiSortKillList ();
			gameData.app.nGameMode = GM_NORMAL;
			}
			break;

		case ND_EVENT_MULTI_CONNECT: {
			sbyte nPlayer, nNewPlayer;
			int killedTotal = 0, killsTotal = 0;
			char pszNewCallsign [CALLSIGN_LEN+1], old_callsign [CALLSIGN_LEN+1];

			nPlayer = NDReadByte ();
			nNewPlayer = NDReadByte ();
			if (!nNewPlayer) {
				NDReadString (old_callsign);
				killedTotal = NDReadInt ();
				killsTotal = NDReadInt ();
				}
			NDReadString (pszNewCallsign);
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				gameData.multiplayer.players [nPlayer].connected = 0;
				if (!nNewPlayer) {
					memcpy (gameData.multiplayer.players [nPlayer].callsign, old_callsign, CALLSIGN_LEN+1);
					gameData.multiplayer.players [nPlayer].netKilledTotal = killedTotal;
					gameData.multiplayer.players [nPlayer].netKillsTotal = killsTotal;
					}
				else
					gameData.multiplayer.nPlayers--;
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				gameData.multiplayer.players [nPlayer].connected = 1;
				gameData.multiplayer.players [nPlayer].netKillsTotal = 0;
				gameData.multiplayer.players [nPlayer].netKilledTotal = 0;
				memcpy (gameData.multiplayer.players [nPlayer].callsign, pszNewCallsign, CALLSIGN_LEN+1);
				if (nNewPlayer)
					gameData.multiplayer.nPlayers++;
				}
			}
			break;

		case ND_EVENT_MULTI_RECONNECT: {
			sbyte nPlayer = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.multiplayer.players [nPlayer].connected = 0;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multiplayer.players [nPlayer].connected = 1;
			}
			break;

		case ND_EVENT_MULTI_DISCONNECT: {
			sbyte nPlayer = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.multiplayer.players [nPlayer].connected = 1;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multiplayer.players [nPlayer].connected = 0;
			}
			break;

		case ND_EVENT_MULTI_SCORE: {
			sbyte nPlayer = NDReadByte ();
			int score = NDReadInt ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				gameData.multiplayer.players [nPlayer].score -= score;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				gameData.multiplayer.players [nPlayer].score += score;
			gameData.app.nGameMode = gameData.demo.nGameMode;
			MultiSortKillList ();
			gameData.app.nGameMode = GM_NORMAL;
			}
			break;

		case ND_EVENT_PLAYER_SCORE: {
			int score = NDReadInt ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				LOCALPLAYER.score -= score;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				LOCALPLAYER.score += score;
			}
			break;

		case ND_EVENT_PRIMARY_AMMO: {
			short nOldAmmo = NDReadShort ();
			short nNewAmmo = NDReadShort ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				LOCALPLAYER.primaryAmmo [gameData.weapons.nPrimary] = nOldAmmo;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				LOCALPLAYER.primaryAmmo [gameData.weapons.nPrimary] = nNewAmmo;
			}
			break;

		case ND_EVENT_SECONDARY_AMMO: {
			short nOldAmmo = NDReadShort ();
			short nNewAmmo = NDReadShort ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary] = nOldAmmo;
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD))
				LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary] = nNewAmmo;
			}
			break;

		case ND_EVENT_DOOR_OPENING: {
			short nSegment = NDReadShort ();
			ubyte nSide = NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				int anim_num;
				int nConnSide;
				tSegment *segP, *oppSegP;

				segP = gameData.segs.segments + nSegment;
				oppSegP = gameData.segs.segments + segP->children [nSide];
				nConnSide = FindConnectedSide (segP, oppSegP);
				anim_num = gameData.walls.walls [WallNumP (segP, nSide)].nClip;
				if (gameData.walls.pAnims [anim_num].flags & WCF_TMAP1)
					segP->sides [nSide].nBaseTex = oppSegP->sides [nConnSide].nBaseTex =
						gameData.walls.pAnims [anim_num].frames [0];
				else
					segP->sides [nSide].nOvlTex = 
					oppSegP->sides [nConnSide].nOvlTex = gameData.walls.pAnims [anim_num].frames [0];
				}
			else
				WallOpenDoor (gameData.segs.segments + nSegment, nSide);
			}
			break;

		case ND_EVENT_LASER_LEVEL: {
			sbyte oldLevel, newLevel;

			oldLevel = (sbyte) NDReadByte ();
			newLevel = (sbyte) NDReadByte ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				LOCALPLAYER.laserLevel = oldLevel;
				UpdateLaserWeaponInfo ();
				}
			else if ((gameData.demo.nVcrState == ND_STATE_PLAYBACK) || 
						(gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD)) {
				LOCALPLAYER.laserLevel = newLevel;
				UpdateLaserWeaponInfo ();
				}
			}
			break;

		case ND_EVENT_CLOAKING_WALL: {
			ubyte nBackWall, nFrontWall, nType, state, cloakValue;
			short l0, l1, l2, l3;
			tSegment *segP;
			int nSide;

			nFrontWall = NDReadByte ();
			nBackWall = NDReadByte ();
			nType = NDReadByte ();
			state = NDReadByte ();
			cloakValue = NDReadByte ();
			l0 = NDReadShort ();
			l1 = NDReadShort ();
			l2 = NDReadShort ();
			l3 = NDReadShort ();
			gameData.walls.walls [nFrontWall].nType = nType;
			gameData.walls.walls [nFrontWall].state = state;
			gameData.walls.walls [nFrontWall].cloakValue = cloakValue;
			segP = gameData.segs.segments + gameData.walls.walls [nFrontWall].nSegment;
			nSide = gameData.walls.walls [nFrontWall].nSide;
			segP->sides [nSide].uvls [0].l = ((int) l0) << 8;
			segP->sides [nSide].uvls [1].l = ((int) l1) << 8;
			segP->sides [nSide].uvls [2].l = ((int) l2) << 8;
			segP->sides [nSide].uvls [3].l = ((int) l3) << 8;
			gameData.walls.walls [nBackWall].nType = nType;
			gameData.walls.walls [nBackWall].state = state;
			gameData.walls.walls [nBackWall].cloakValue = cloakValue;
			segP = &gameData.segs.segments [gameData.walls.walls [nBackWall].nSegment];
			nSide = gameData.walls.walls [nBackWall].nSide;
			segP->sides [nSide].uvls [0].l = ((int) l0) << 8;
			segP->sides [nSide].uvls [1].l = ((int) l1) << 8;
			segP->sides [nSide].uvls [2].l = ((int) l2) << 8;
			segP->sides [nSide].uvls [3].l = ((int) l3) << 8;
			}
			break;

		case ND_EVENT_NEW_LEVEL: {
			sbyte newLevel, oldLevel, loadedLevel;

			newLevel = NDReadByte ();
			oldLevel = NDReadByte ();
			if (gameData.demo.nVcrState == ND_STATE_PAUSED)
				break;
			StopTime ();
			if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
				 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD))
				loadedLevel = oldLevel;
			else {
				loadedLevel = newLevel;
				for (i = 0; i < MAX_PLAYERS; i++) {
					gameData.multiplayer.players [i].cloakTime = 0;
					gameData.multiplayer.players [i].flags &= ~PLAYER_FLAGS_CLOAKED;
					}
				}
			if ((loadedLevel < gameData.missions.nLastSecretLevel) || 
				 (loadedLevel > gameData.missions.nLastLevel)) {
				NDErrorMsg (TXT_CANT_PLAYBACK, TXT_LEVEL_CANT_LOAD, TXT_DEMO_OLD_CORRUPT);
				return -1;
				}
			LoadLevel ((int) loadedLevel, 1, 0);
			gameData.demo.bCtrlcenDestroyed = 0;
			if (bJustStartedPlayback) {
				gameData.walls.nWalls = NDReadInt ();
				for (i = 0; i < gameData.walls.nWalls; i++) {   // restore the walls
					gameData.walls.walls [i].nType = NDReadByte ();
					gameData.walls.walls [i].flags = NDReadByte ();
					gameData.walls.walls [i].state = NDReadByte ();
					segP = gameData.segs.segments + gameData.walls.walls [i].nSegment;
					nSide = gameData.walls.walls [i].nSide;
					segP->sides [nSide].nBaseTex = NDReadShort ();
					nTexture = NDReadShort ();
					segP->sides [nSide].nOvlTex = nTexture & 0x3fff;
					segP->sides [nSide].nOvlOrient = (nTexture >> 14) & 3;
					}
				if (gameData.demo.nGameMode & GM_CAPTURE)
					MultiApplyGoalTextures ();
				bJustStartedPlayback = 0;
				}
			ResetPaletteAdd ();                // get palette back to normal
			StartTime ();
			}
			break;

		case ND_EVENT_EOF:
			bDone = -1;
			CFSeek (&ndInFile, -1, SEEK_CUR);        // get back to the EOF marker
			gameData.demo.bEof = 1;
			gameData.demo.nFrameCount++;
			break;

		default:
			bDone = -1;
			CFSeek (&ndInFile, -1, SEEK_CUR);        // get back to the EOF marker
			gameData.demo.bEof = 1;
			gameData.demo.nFrameCount++;
			break;
		}
	}
LastReadValue = c;
if (bNDBadRead)
	NDErrorMsg (TXT_DEMO_ERR_READING, TXT_DEMO_OLD_CORRUPT, NULL);
else
	NDUpdateSmoke ();
return bDone;
}

//	-----------------------------------------------------------------------------

void NDGotoBeginning ()
{
CFSeek (&ndInFile, 0, SEEK_SET);
gameData.demo.nVcrState = ND_STATE_PLAYBACK;
if (NDReadDemoStart (0))
	NDStopPlayback ();
if (NDReadFrameInfo () == -1)
	NDStopPlayback ();
if (NDReadFrameInfo () == -1)
	NDStopPlayback ();
gameData.demo.nVcrState = ND_STATE_PAUSED;
gameData.demo.bEof = 0;
}

//	-----------------------------------------------------------------------------

void NDGotoEnd ()
{
	short nFrameLength, byteCount, bshort;
	sbyte level, bbyte, laserLevel;
	ubyte energy, shield, c;
	int i, loc, bint;

CFSeek (&ndInFile, -2, SEEK_END);
level = NDReadByte ();
if ((level < gameData.missions.nLastSecretLevel) || (level > gameData.missions.nLastLevel)) {
	NDErrorMsg (TXT_CANT_PLAYBACK, TXT_LEVEL_CANT_LOAD, TXT_DEMO_OLD_CORRUPT);
	NDStopPlayback ();
	return;
	}
if (level != gameData.missions.nCurrentLevel)
	LoadLevel (level, 1, 0);
CFSeek (&ndInFile, -4, SEEK_END);
byteCount = NDReadShort ();
CFSeek (&ndInFile, -2 - byteCount, SEEK_CUR);

nFrameLength = NDReadShort ();
loc = CFTell (&ndInFile);
if (gameData.demo.nGameMode & GM_MULTI)
	gameData.demo.bPlayersCloaked = NDReadByte ();
else
	bbyte = NDReadByte ();
bbyte = NDReadByte ();
bshort = NDReadShort ();
bint = NDReadInt ();
energy = NDReadByte ();
shield = NDReadByte ();
LOCALPLAYER.energy = i2f (energy);
LOCALPLAYER.shields = i2f (shield);
LOCALPLAYER.flags = NDReadInt ();
if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
	LOCALPLAYER.cloakTime = gameData.time.xGame - (CLOAK_TIME_MAX / 2);
	gameData.demo.bPlayersCloaked |= (1 << gameData.multiplayer.nLocalPlayer);
	}
if (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)
	LOCALPLAYER.invulnerableTime = gameData.time.xGame - (INVULNERABLE_TIME_MAX / 2);
gameData.weapons.nPrimary = NDReadByte ();
gameData.weapons.nSecondary = NDReadByte ();
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	LOCALPLAYER.primaryAmmo [i] = NDReadShort ();
for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
	LOCALPLAYER.secondaryAmmo [i] = NDReadShort ();
laserLevel = NDReadByte ();
if (laserLevel != LOCALPLAYER.laserLevel) {
	LOCALPLAYER.laserLevel = laserLevel;
	UpdateLaserWeaponInfo ();
	}
if (gameData.demo.nGameMode & GM_MULTI) {
	c = NDReadByte ();
	gameData.multiplayer.nPlayers = (int)c;
	// see newdemo_read_start_demo for explanation of
	// why this is commented out
	//		NDReadByte ((sbyte *)&gameData.multiplayer.nPlayers);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		NDReadString (gameData.multiplayer.players [i].callsign);
		gameData.multiplayer.players [i].connected = NDReadByte ();
		if (IsCoopGame)
			gameData.multiplayer.players [i].score = NDReadInt ();
		else {
			gameData.multiplayer.players [i].netKilledTotal = NDReadShort ();
			gameData.multiplayer.players [i].netKillsTotal = NDReadShort ();
			}
		}
	}
else
	LOCALPLAYER.score = NDReadInt ();
CFSeek (&ndInFile, loc - nFrameLength, SEEK_SET);
gameData.demo.nFrameCount = NDReadInt ();            // get the frame count
gameData.demo.nFrameCount--;
CFSeek (&ndInFile, 4, SEEK_CUR);
gameData.demo.nVcrState = ND_STATE_PLAYBACK;
NDReadFrameInfo ();           // then the frame information
gameData.demo.nVcrState = ND_STATE_PAUSED;
return;
}

//	-----------------------------------------------------------------------------

inline void NDBackOneFrame (void)
{
	short nPrevFrameLength;

CFSeek (&ndInFile, -10, SEEK_CUR);
nPrevFrameLength = NDReadShort ();
CFSeek (&ndInFile, 8 - nPrevFrameLength, SEEK_CUR);
}

//	-----------------------------------------------------------------------------

void NDBackFrames (int nFrames)
{
	int	i;

for (i = nFrames; i; i--) {
	NDBackOneFrame ();
	if (!gameData.demo.bEof && (NDReadFrameInfo () == -1)) {
		NDStopPlayback ();
		return;
		}
	if (gameData.demo.bEof)
		gameData.demo.bEof = 0;
	NDBackOneFrame ();
	}
}

//	-----------------------------------------------------------------------------

/*
 *  routine to interpolate the viewer position.  the current position is
 *  stored in the gameData.objs.viewer tObject.  Save this position, and read the next
 *  frame to get all gameData.objs.objects read in.  Calculate the delta playback and
 *  the delta recording frame times between the two frames, then intepolate
 *  the viewers position accordingly.  gameData.demo.xRecordedTime is the time that it
 *  took the recording to render the frame that we are currently looking
 *  at.
*/

void NDInterpolateFrame (fix d_play, fix d_recorded)
{
	int			nCurObjs;
	fix			factor;
	vmsVector  fvec1, fvec2, rvec1, rvec2;
	fix         mag1;
	fix			delta_x, delta_y, delta_z;
	ubyte			renderType;
	tObject		*curObjP, *objP, *i, *j;

	static tObject curObjs [MAX_OBJECTS_D2X];

factor = FixDiv (d_play, d_recorded);
if (factor > F1_0)
	factor = F1_0;
nCurObjs = gameData.objs.nLastObject;
#if 1
memcpy (curObjs, gameData.objs.objects, sizeof (tObject) * (nCurObjs + 1));
#else
for (i = 0; i <= nCurObjs; i++)
	memcpy (&(curObjs [i]), &(gameData.objs.objects [i]), sizeof (tObject));
#endif
gameData.demo.nVcrState = ND_STATE_PAUSED;
if (NDReadFrameInfo () == -1) {
	NDStopPlayback ();
	return;
	}
for (i = curObjs + nCurObjs, curObjP = curObjs; curObjP < i; curObjP++) {
	for (j = gameData.objs.objects + gameData.objs.nLastObject, objP = gameData.objs.objects; objP < j; objP++) {
		if (curObjP->nSignature == objP->nSignature) {
			renderType = curObjP->renderType;
			//fix delta_p, delta_h, delta_b;
			//vmsAngVec cur_angles, dest_angles;
			//  Extract the angles from the tObject orientation matrix.
			//  Some of this code taken from AITurnTowardsVector
			//  Don't do the interpolation on certain render types which don't use an orientation matrix
			if ((renderType != RT_LASER) &&
				 (renderType != RT_FIREBALL) && 
					(renderType != RT_THRUSTER) && 
					(renderType != RT_POWERUP)) {

				fvec1 = curObjP->position.mOrient.fVec;
				VmVecScale (&fvec1, F1_0-factor);
				fvec2 = objP->position.mOrient.fVec;
				VmVecScale (&fvec2, factor);
				VmVecInc (&fvec1, &fvec2);
				mag1 = VmVecNormalizeQuick (&fvec1);
				if (mag1 > F1_0/256) {
					rvec1 = curObjP->position.mOrient.rVec;
					VmVecScale (&rvec1, F1_0-factor);
					rvec2 = objP->position.mOrient.rVec;
					VmVecScale (&rvec2, factor);
					VmVecInc (&rvec1, &rvec2);
					VmVecNormalizeQuick (&rvec1); // Note: Doesn't matter if this is null, if null, VmVector2Matrix will just use fvec1
					VmVector2Matrix (&curObjP->position.mOrient, &fvec1, NULL, &rvec1);
					}
				}
			// Interpolate the tObject position.  This is just straight linear
			// interpolation.
			delta_x = objP->position.vPos.p.x - curObjP->position.vPos.p.x;
			delta_y = objP->position.vPos.p.y - curObjP->position.vPos.p.y;
			delta_z = objP->position.vPos.p.z - curObjP->position.vPos.p.z;
			delta_x = FixMul (delta_x, factor);
			delta_y = FixMul (delta_y, factor);
			delta_z = FixMul (delta_z, factor);
			curObjP->position.vPos.p.x += delta_x;
			curObjP->position.vPos.p.y += delta_y;
			curObjP->position.vPos.p.z += delta_z;
				// -- old fashioned way --// stuff the new angles back into the tObject structure
				// -- old fashioned way --				VmAngles2Matrix (&(curObjs [i].position.mOrient), &cur_angles);
			}
		}
	}

// get back to original position in the demo file.  Reread the current
// frame information again to reset all of the tObject stuff not covered
// with gameData.objs.nLastObject and the tObject array (previously rendered
// gameData.objs.objects, etc....)
NDBackFrames (1);
NDBackFrames (1);
if (NDReadFrameInfo () == -1)
	NDStopPlayback ();
gameData.demo.nVcrState = ND_STATE_PLAYBACK;
#if 1
memcpy (gameData.objs.objects, curObjs, sizeof (tObject) * (nCurObjs + 1));
#else
for (i = 0; i <= nCurObjs; i++)
	memcpy (&(gameData.objs.objects [i]), &(curObjs [i]), sizeof (tObject));
#endif
gameData.objs.nLastObject = nCurObjs;
}

//	-----------------------------------------------------------------------------

void NDPlayBackOneFrame ()
{
	int nFramesBack, i, level;
	static fix base_interpolTime = 0;
	static fix d_recorded = 0;

for (i = 0; i < MAX_PLAYERS; i++)
	if (gameData.demo.bPlayersCloaked &(1 << i))
		gameData.multiplayer.players [i].cloakTime = gameData.time.xGame - (CLOAK_TIME_MAX / 2);
if (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)
	LOCALPLAYER.invulnerableTime = gameData.time.xGame - (INVULNERABLE_TIME_MAX / 2);
if (gameData.demo.nVcrState == ND_STATE_PAUSED)       // render a frame or not
	return;
if (gameData.demo.nVcrState == ND_STATE_PLAYBACK)
	DoJasonInterpolate (gameData.demo.xRecordedTime);
gameData.reactor.bDestroyed = 0;
gameData.reactor.countdown.nSecsLeft = -1;
PALETTE_FLASH_SET (0, 0, 0);       //clear flash
if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
	 (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
	level = gameData.missions.nCurrentLevel;
	if (gameData.demo.nFrameCount == 0)
		return;
	else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) && (gameData.demo.nFrameCount < 10)) {
		NDGotoBeginning ();
		return;
		}
	nFramesBack = (gameData.demo.nVcrState == ND_STATE_REWINDING) ? 10 : 1;
	if (gameData.demo.bEof) {
		CFSeek (&ndInFile, 11, SEEK_CUR);
		}
	NDBackFrames (nFramesBack);
	if (level != gameData.missions.nCurrentLevel)
		NDPopCtrlCenTriggers ();
	if (gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD) {
		if (level != gameData.missions.nCurrentLevel)
			NDBackFrames (1);
		gameData.demo.nVcrState = ND_STATE_PAUSED;
		}
	}
else if (gameData.demo.nVcrState == ND_STATE_FASTFORWARD) {
	if (!gameData.demo.bEof) {
		for (i = 0; i < 10; i++) {
			if (NDReadFrameInfo () == -1) {
				if (gameData.demo.bEof)
					gameData.demo.nVcrState = ND_STATE_PAUSED;
				else
					NDStopPlayback ();
				break;
				}
			}
		}
	else
		gameData.demo.nVcrState = ND_STATE_PAUSED;
	}
else if (gameData.demo.nVcrState == ND_STATE_ONEFRAMEFORWARD) {
	if (!gameData.demo.bEof) {
		level = gameData.missions.nCurrentLevel;
		if (NDReadFrameInfo () == -1) {
			if (!gameData.demo.bEof)
				NDStopPlayback ();
			}
		if (level != gameData.missions.nCurrentLevel) {
			if (NDReadFrameInfo () == -1) {
				if (!gameData.demo.bEof)
					NDStopPlayback ();
				}
			}
		gameData.demo.nVcrState = ND_STATE_PAUSED;
		} 
	else
		gameData.demo.nVcrState = ND_STATE_PAUSED;
	}
else {
	//  First, uptate the total playback time to date.  Then we check to see
	//  if we need to change the playback style to interpolate frames or
	//  skip frames based on where the playback time is relative to the
	//  recorded time.
	if (gameData.demo.nFrameCount <= 0)
		gameData.demo.xPlaybackTotal = gameData.demo.xRecordedTotal;      // baseline total playback time
	else
		gameData.demo.xPlaybackTotal += gameData.time.xFrame;
	if ((gameData.demo.nPlaybackStyle == NORMAL_PLAYBACK) && (gameData.demo.nFrameCount > 10))
		if ((gameData.demo.xPlaybackTotal * INTERPOL_FACTOR) < gameData.demo.xRecordedTotal) {
			gameData.demo.nPlaybackStyle = INTERPOLATE_PLAYBACK;
			gameData.demo.xPlaybackTotal = gameData.demo.xRecordedTotal + gameData.time.xFrame;  // baseline playback time
			base_interpolTime = gameData.demo.xRecordedTotal;
			d_recorded = gameData.demo.xRecordedTime;                      // baseline delta recorded
			}
	if ((gameData.demo.nPlaybackStyle == NORMAL_PLAYBACK) && (gameData.demo.nFrameCount > 10))
		if (gameData.demo.xPlaybackTotal > gameData.demo.xRecordedTotal)
			gameData.demo.nPlaybackStyle = SKIP_PLAYBACK;
	if ((gameData.demo.nPlaybackStyle == INTERPOLATE_PLAYBACK) && gameData.demo.bInterpolate) {
		fix d_play = 0;

		if (gameData.demo.xRecordedTotal - gameData.demo.xPlaybackTotal < gameData.time.xFrame) {
			d_recorded = gameData.demo.xRecordedTotal - gameData.demo.xPlaybackTotal;
			while (gameData.demo.xRecordedTotal - gameData.demo.xPlaybackTotal < gameData.time.xFrame) {
				tObject *curObjs, *objP;
				int i, j, nObjects, nLevel, nSig;

				nObjects = gameData.objs.nLastObject;
				curObjs = (tObject *) D2_ALLOC (sizeof (tObject) * (nObjects + 1));
				if (!
					curObjs) {
					Warning (TXT_INTERPOLATE_BOTS, sizeof (tObject) * nObjects);
					break;
					}
				for (i = 0; i <= nObjects; i++)
					memcpy (curObjs, gameData.objs.objects, (nObjects + 1) * sizeof (tObject));
				nLevel = gameData.missions.nCurrentLevel;
				if (NDReadFrameInfo () == -1) {
					D2_FREE (curObjs);
					NDStopPlayback ();
					return;
					}
				if (nLevel != gameData.missions.nCurrentLevel) {
					D2_FREE (curObjs);
					if (NDReadFrameInfo () == -1)
						NDStopPlayback ();
					break;
					}
				//  for each new tObject in the frame just read in, determine if there is
				//  a corresponding tObject that we have been interpolating.  If so, then
				//  copy that interpolated tObject to the new gameData.objs.objects array so that the
				//  interpolated position and orientation can be preserved.
				for (i = 0; i <= nObjects; i++) {
					nSig = curObjs [i].nSignature;
					objP = gameData.objs.objects;
					for (j = 0; j <= gameData.objs.nLastObject; j++, objP++) {
						if (nSig == objP->nSignature) {
							objP->position.mOrient = curObjs [i].position.mOrient;
							objP->position.vPos = curObjs [i].position.vPos;
							break;
							}
						}
					}
				D2_FREE (curObjs);
				d_recorded += gameData.demo.xRecordedTime;
				base_interpolTime = gameData.demo.xPlaybackTotal - gameData.time.xFrame;
				}
			}
		d_play = gameData.demo.xPlaybackTotal - base_interpolTime;
		NDInterpolateFrame (d_play, d_recorded);
		return;
		}
	else {
		if (NDReadFrameInfo () == -1) {
			NDStopPlayback ();
			return;
			}
		if (gameData.demo.nPlaybackStyle == SKIP_PLAYBACK) {
			while (gameData.demo.xPlaybackTotal > gameData.demo.xRecordedTotal) {
				if (NDReadFrameInfo () == -1) {
					NDStopPlayback ();
					return;
					}
				}
			}
		}
	}
}

//	-----------------------------------------------------------------------------

void NDStartRecording ()
{
gameData.demo.nSize = GetDiskFree ();
gameData.demo.nSize -= 100000;
if ((gameData.demo.nSize+100000) <  2000000000) {
	if (( (int) (gameData.demo.nSize)) < 500000) {
		ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_DEMO_NO_SPACE);
		return;
		}
	}
InitDemoData ();
gameData.demo.nWritten = 0;
gameData.demo.bNoSpace = 0;
gameData.demo.xStartTime = gameData.time.xGame;
gameData.demo.nState = ND_STATE_RECORDING;
CFOpen (&ndOutFile, DEMO_FILENAME, gameFolders.szDemoDir, "wb", 0);
#ifndef _WIN32_WCE
if (!ndOutFile.file && (errno == ENOENT)) {   //dir doesn't exist?
#else
if (&ndOutFile.file) {                      //dir doesn't exist and no errno on mac!
#endif
	CFMkDir (gameFolders.szDemoDir); //try making directory
	CFOpen (&ndOutFile, DEMO_FILENAME, gameFolders.szDemoDir, "wb", 0);
	}
if (!ndOutFile.file) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, "Cannot open demo temp file");
	gameData.demo.nState = ND_STATE_NORMAL;
	}
else
	NDRecordStartDemo ();
}

//	-----------------------------------------------------------------------------

void NDFinishRecording (void)
{
	ubyte cloaked = 0;
	unsigned short byteCount = 0;
	int l;

NDWriteByte (ND_EVENT_EOF);
NDWriteShort ((short) (gameData.demo.nFrameBytesWritten - 1));
if (gameData.demo.nGameMode & GM_MULTI) {
	for (l = 0; l < gameData.multiplayer.nPlayers; l++) {
		if (gameData.multiplayer.players [l].flags & PLAYER_FLAGS_CLOAKED)
			cloaked |= (1 << l);
		}
	NDWriteByte (cloaked);
	NDWriteByte (ND_EVENT_EOF);
	}
else {
	NDWriteShort (ND_EVENT_EOF);
	}
NDWriteShort (ND_EVENT_EOF);
NDWriteInt (ND_EVENT_EOF);
byteCount += 10;       // from gameData.demo.nFrameBytesWritten
NDWriteByte ((sbyte) (f2ir (LOCALPLAYER.energy)));
NDWriteByte ((sbyte) (f2ir (LOCALPLAYER.shields)));
NDWriteInt (LOCALPLAYER.flags);        // be sure players flags are set
NDWriteByte ((sbyte)gameData.weapons.nPrimary);
NDWriteByte ((sbyte)gameData.weapons.nSecondary);
byteCount += 8;
for (l = 0; l < MAX_PRIMARY_WEAPONS; l++)
	NDWriteShort ((short)LOCALPLAYER.primaryAmmo [l]);
for (l = 0; l < MAX_SECONDARY_WEAPONS; l++)
	NDWriteShort ((short)LOCALPLAYER.secondaryAmmo [l]);
byteCount += (sizeof (short) * (MAX_PRIMARY_WEAPONS + MAX_SECONDARY_WEAPONS));
NDWriteByte (LOCALPLAYER.laserLevel);
byteCount++;
if (gameData.demo.nGameMode & GM_MULTI) {
	NDWriteByte ((sbyte)gameData.multiplayer.nPlayers);
	byteCount++;
	for (l = 0; l < gameData.multiplayer.nPlayers; l++) {
		NDWriteString (gameData.multiplayer.players [l].callsign);
		byteCount += ((int) strlen (gameData.multiplayer.players [l].callsign) + 2);
		NDWriteByte ((sbyte) gameData.multiplayer.players [l].connected);
		if (gameData.app.nGameMode & GM_MULTI_COOP) {
			NDWriteInt (gameData.multiplayer.players [l].score);
			byteCount += 5;
			}
		else {
			NDWriteShort ((short)gameData.multiplayer.players [l].netKilledTotal);
			NDWriteShort ((short)gameData.multiplayer.players [l].netKillsTotal);
			byteCount += 5;
			}
		}
	} 
else {
	NDWriteInt (LOCALPLAYER.score);
	byteCount += 4;
	}
NDWriteShort (byteCount);
NDWriteByte ((sbyte) gameData.missions.nCurrentLevel);
NDWriteByte (ND_EVENT_EOF);
CFClose (&ndOutFile);
}

//	-----------------------------------------------------------------------------

char demoname_allowed_chars [] = "azAZ09__--";
void NDStopRecording ()
{
	tMenuItem m [6];
	int exit = 0;
	static char filename [15] = "", *s;
	static ubyte tmpcnt = 0;
	char fullname [15+FILENAME_LEN] = "";

NDFinishRecording ();
gameData.demo.nState = ND_STATE_NORMAL;
GrPaletteStepLoad (NULL);
if (filename [0] != '\0') {
	int num, i = (int) strlen (filename) - 1;
	char newfile [15];

	while (isdigit (filename [i])) {
		i--;
		if (i == -1)
			break;
		}
	i++;
	num = atoi (&(filename [i]));
	num++;
	filename [i] = '\0';
	sprintf (newfile, "%s%d", filename, num);
	strncpy (filename, newfile, 8);
	filename [8] = '\0';
	}

try_again:
	;

nmAllowedChars = demoname_allowed_chars;
memset (m, 0, sizeof (m));
if (!gameData.demo.bNoSpace) {
	m [0].nType = NM_TYPE_INPUT; 
	m [0].text_len = 8; 
	m [0].text = filename;
	exit = ExecMenu (NULL, TXT_SAVE_DEMO_AS, 1, &(m [0]), NULL, NULL);
	}
else if (gameData.demo.bNoSpace == 1) {
	m [0].nType = NM_TYPE_TEXT; 
	m [0].text = TXT_DEMO_SAVE_BAD;
	m [1].nType = NM_TYPE_INPUT;
	m [1].text_len = 8; 
	m [1].text = filename;
	exit = ExecMenu (NULL, NULL, 2, m, NULL, NULL);
	} 
else if (gameData.demo.bNoSpace == 2) {
	m [0].nType = NM_TYPE_TEXT; 
	m [0].text = TXT_DEMO_SAVE_NOSPACE;
	m [1].nType = NM_TYPE_INPUT;
	m [1].text_len = 8; 
	m [1].text = filename;
	exit = ExecMenu (NULL, NULL, 2, m, NULL, NULL);
	}
nmAllowedChars = NULL;
if (exit == -2) {                   // got bumped out from network menu
	char save_file [7+FILENAME_LEN];

	if (filename [0] != '\0') {
		strcpy (save_file, filename);
		strcat (save_file, ".dem");
	} else
		sprintf (save_file, "tmp%d.dem", tmpcnt++);
	CFDelete (save_file, gameFolders.szDemoDir);
	CFRename (DEMO_FILENAME, save_file, gameFolders.szDemoDir);
	return;
	}
if (exit == -1) {               // pressed ESC
	CFDelete (DEMO_FILENAME, gameFolders.szDemoDir);      // might as well remove the file
	return;                     // return without doing anything
	}
if (filename [0]==0) //null string
	goto try_again;
//check to make sure name is ok
for (s=filename;*s;s++)
	if (!isalnum (*s) && *s!='_') {
		ExecMessageBox (NULL, NULL, 1, TXT_CONTINUE, TXT_DEMO_USE_LETTERS);
		goto try_again;
		}
if (gameData.demo.bNoSpace)
	strcpy (fullname, m [1].text);
else
	strcpy (fullname, m [0].text);
strcat (fullname, ".dem");
CFDelete (fullname, gameFolders.szDemoDir);
CFRename (DEMO_FILENAME, fullname, gameFolders.szDemoDir);
}

//	-----------------------------------------------------------------------------
//returns the number of demo files on the disk
int NDCountDemos ()
{
	FFS	ffs;
	int 	nFiles=0;
	char	searchName [FILENAME_LEN];

sprintf (searchName, "%s%s*.dem", gameFolders.szDemoDir, *gameFolders.szDemoDir ? "/" : "");
if (!FFF (searchName, &ffs, 0)) {
	do {
		nFiles++;
		} while (!FFN (&ffs, 0));
	FFC (&ffs);
	}
if (gameFolders.bAltHogDirInited) {
	sprintf (searchName, "%s/%s%s*.dem", gameFolders.szAltHogDir, gameFolders.szDemoDir, gameFolders.szDemoDir ? "/" : "");
	if (!FFF (searchName, &ffs, 0)) {
		do {
			nFiles++;
			} while (!FFN (&ffs, 0));
		FFC (&ffs);
		}
	}
return nFiles;
}

//	-----------------------------------------------------------------------------

void NDStartPlayback (char * filename)
{
	FFS ffs;
	int bRandom = 0;
	char filename2 [PATH_MAX+FILENAME_LEN], searchName [FILENAME_LEN];

ChangePlayerNumTo (0);
*filename2 = '\0';
InitDemoData ();
gameData.demo.bFirstTimePlayback = 1;
gameData.demo.xJasonPlaybackTotal = 0;
if (!filename) {
	// Randomly pick a filename
	int nFiles = 0, nRandFiles;
	bRandom = 1;
	nFiles = NDCountDemos ();
	if (nFiles == 0) {
		return;     // No files found!
		}
	nRandFiles = d_rand () % nFiles;
	nFiles = 0;
	sprintf (searchName, "%s%s*.dem", gameFolders.szDemoDir, *gameFolders.szDemoDir ? "/" : "");
	if (!FFF (searchName, &ffs, 0)) {
		do {
			if (nFiles == nRandFiles) {
				filename = (char *)&ffs.name;
				break;
				}
			nFiles++;
			} while (!FFN (&ffs, 0));
		FFC (&ffs);
		}
	if (!filename && gameFolders.bAltHogDirInited) {
		sprintf (searchName, "%s/%s%s*.dem", gameFolders.szAltHogDir, gameFolders.szDemoDir, gameFolders.szDemoDir ? "/" : "");
		if (!FFF (searchName, &ffs, 0)) {
			do {
				if (nFiles==nRandFiles) {
					filename = (char *)&ffs.name;
					break;
					}
				nFiles++;
				} while (!FFN (&ffs, 0));
			FFC (&ffs);
			}
		}
		if (!filename) 
			return;
	}
if (!filename)
	return;
strcpy (filename2, filename);
bRevertFormat = gameOpts->demo.bRevertFormat ? 1 : -1;
if (!CFOpen (&ndInFile, filename2, gameFolders.szDemoDir, "rb", 0)) {
#if TRACE			
	con_printf (CONDBG, "Error reading '%s'\n", filename);
#endif
	return;
	}
if (bRevertFormat > 0) {
	strcat (filename2, ".v15");
	if (!CFOpen (&ndOutFile, filename2, gameFolders.szDemoDir, "wb", 0))
		bRevertFormat = -1;
	}
else
	ndOutFile.file = NULL;
bNDBadRead = 0;
ChangePlayerNumTo (0);                 // force playernum to 0
strncpy (gameData.demo.callSignSave, LOCALPLAYER.callsign, CALLSIGN_LEN);
gameData.objs.viewer = gameData.objs.console = gameData.objs.objects;   // play properly as if console tPlayer
if (NDReadDemoStart (bRandom)) {
	CFClose (&ndInFile);
	CFClose (&ndOutFile);
	return;
	}
if (gameOpts->demo.bRevertFormat && ndOutFile.file && (bRevertFormat < 0)) {
	CFClose (&ndOutFile);
	CFDelete (filename2, gameFolders.szDemoDir);
	}
gameData.app.nGameMode = GM_NORMAL;
gameData.demo.nState = ND_STATE_PLAYBACK;
gameData.demo.nVcrState = ND_STATE_PLAYBACK;
gameData.demo.nOldCockpit = gameStates.render.cockpit.nMode;
gameData.demo.nSize = CFLength (&ndInFile, 0);
bNDBadRead = 0;
gameData.demo.bEof = 0;
gameData.demo.nFrameCount = 0;
gameData.demo.bPlayersCloaked = 0;
gameData.demo.nPlaybackStyle = NORMAL_PLAYBACK;
SetFunctionMode (FMODE_GAME);
gameStates.render.cockpit.n3DView [0] = CV_NONE;       //turn off 3d views on cockpit
gameStates.render.cockpit.n3DView [1] = CV_NONE;       //turn off 3d views on cockpit
SDL_ShowCursor (0);
NDPlayBackOneFrame ();       // this one loads new level
NDPlayBackOneFrame ();       // get all of the gameData.objs.objects to renderb game
}

//	-----------------------------------------------------------------------------

void NDStopPlayback ()
{
if (bRevertFormat > 0) {
	int h = CFLength (&ndInFile, 0) - CFTell (&ndInFile);
	char *p = (char *) D2_ALLOC (h);
	if (p) {
		bRevertFormat = 0;
		NDRead (p, h, 1);
		//LogErr ("%4d %4d %d\n", gameData.demo.nFrameCount, gameData.demo.nFrameBytesWritten - 1, CFTell (&ndOutFile);
		NDWriteShort ((short) (gameData.demo.nFrameBytesWritten - 1));
		NDWrite (p + 3, h - 3, 1);
		D2_FREE (p);
		}
	CFClose (&ndOutFile);
	bRevertFormat = -1;
	}
CFClose (&ndInFile);
gameData.demo.nState = ND_STATE_NORMAL;
ChangePlayerNumTo (0);             //this is reality
strncpy (LOCALPLAYER.callsign, gameData.demo.callSignSave, CALLSIGN_LEN);
gameStates.render.cockpit.nMode = gameData.demo.nOldCockpit;
gameData.app.nGameMode = GM_GAME_OVER;
SetFunctionMode (FMODE_MENU);
SDL_ShowCursor (1);
longjmp (gameExitPoint, 0);               // Exit game loop
}

//	-----------------------------------------------------------------------------

#ifdef _DEBUG

#define BUF_SIZE 16384

void NewDemoStripFrames (char *outname, int bytes_to_strip)
{
	CFILE	ndOutFile;
	char	*buf;
	int	nTotalSize, bytes_done, read_elems, bytes_back;
	int	trailer_start, loc1, loc2, stop_loc, bytes_to_read;
	short	nPrevFrameLength;

bytes_done = 0;
nTotalSize = CFLength (&ndInFile, 0);
if (!CFOpen (&ndOutFile, outname, "", "wb", 0)) {
	NDErrorMsg ("Can't open output file", NULL, NULL);
	NDStopPlayback ();
	return;
	}
if (!(buf = D2_ALLOC (BUF_SIZE))) {
	NDErrorMsg ("Mot enough memory for output buffer", NULL, NULL);
	CFClose (&ndOutFile);
	NDStopPlayback ();
	return;
	}
NDGotoEnd ();
trailer_start = CFTell (&ndInFile);
CFSeek (&ndInFile, 11, SEEK_CUR);
bytes_back = 0;
while (bytes_back < bytes_to_strip) {
	loc1 = CFTell (&ndInFile);
	//CFSeek (&ndInFile, -10, SEEK_CUR);
	//NDReadShort (&nPrevFrameLength);
	//CFSeek (&ndInFile, 8 - nPrevFrameLength, SEEK_CUR);
	NDBackFrames (1);
	loc2 = CFTell (&ndInFile);
	bytes_back += (loc1 - loc2);
	}
CFSeek (&ndInFile, -10, SEEK_CUR);
nPrevFrameLength = NDReadShort ();
CFSeek (&ndInFile, -3, SEEK_CUR);
stop_loc = CFTell (&ndInFile);
CFSeek (&ndInFile, 0, SEEK_SET);
while (stop_loc > 0) {
	if (stop_loc < BUF_SIZE)
		bytes_to_read = stop_loc;
	else
		bytes_to_read = BUF_SIZE;
	read_elems = (int) CFRead (buf, 1, bytes_to_read, &ndInFile);
	CFWrite (buf, 1, read_elems, &ndOutFile);
	stop_loc -= read_elems;
	}
stop_loc = CFTell (&ndOutFile);
CFSeek (&ndInFile, trailer_start, SEEK_SET);
while ((read_elems = (int) CFRead (buf, 1, BUF_SIZE, &ndInFile)))
	CFWrite (buf, 1, read_elems, &ndOutFile);
CFSeek (&ndOutFile, stop_loc, SEEK_SET);
CFSeek (&ndOutFile, 1, SEEK_CUR);
CFWrite (&nPrevFrameLength, 2, 1, &ndOutFile);
CFClose (&ndOutFile);
NDStopPlayback ();
DestroyAllSmoke ();
}

#endif

//	-----------------------------------------------------------------------------

tObject demoRightExtra, demoLeftExtra;
ubyte nDemoDoRight=0, nDemoDoLeft=0;

void NDRenderExtras (ubyte which, tObject *objP)
{
	ubyte w=which>>4;
	ubyte nType=which&15;

if (which==255) {
	Int3 (); // how'd we get here?
	DoCockpitWindowView (w, NULL, 0, WBU_WEAPON, NULL);
	return;
	}
if (w) {
	memcpy (&demoRightExtra, objP, sizeof (tObject));  
	nDemoDoRight=nType;
	}
else {
	memcpy (&demoLeftExtra, objP, sizeof (tObject)); 
	nDemoDoLeft=nType;
	}
}

//	-----------------------------------------------------------------------------

void DoJasonInterpolate (fix xRecordedTime)
{
	fix xDelay;

gameData.demo.xJasonPlaybackTotal += gameData.time.xFrame;
if (!gameData.demo.bFirstTimePlayback) {
	// get the difference between the recorded time and the playback time
	xDelay = (xRecordedTime - gameData.time.xFrame);
	if (xDelay >= f0_0) {
		StopTime ();
		timer_delay (xDelay);
		StartTime ();
		}
	else {
		while (gameData.demo.xJasonPlaybackTotal > gameData.demo.xRecordedTotal)
			if (NDReadFrameInfo () == -1) {
				NDStopPlayback ();
				return;
				}
		//xDelay = gameData.demo.xRecordedTotal - gameData.demo.xJasonPlaybackTotal;
		//if (xDelay > f0_0)
		//	timer_delay (xDelay);
		}
	}
gameData.demo.bFirstTimePlayback=0;
}

//	-----------------------------------------------------------------------------
//eof
