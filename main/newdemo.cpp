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
#if defined (__unix__) || defined (__macosx__)
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "inferno.h"
#include "u_mem.h"
#include "stdlib.h"
#include "texmap.h"
#include "key.h"
#include "gameseg.h"
#include "objrender.h"
#include "physics.h"
#include "slew.h"
#include "render.h"
#include "error.h"
#include "hostage.h"
#include "collide.h"
#include "light.h"
#include "newdemo.h"
#include "gauges.h"
#include "text.h"
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
#define ND_EVENT_VIEWER_OBJECT      3   // Followed by an CObject structure
#define ND_EVENT_RENDER_OBJECT      4   // Followed by an CObject structure
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
#define ND_EVENT_PLAYER_FLAGS       19  // followed by CPlayerData flags
#define ND_EVENT_PLAYER_WEAPON      20  // followed by weapon nType and weapon number
#define ND_EVENT_EFFECT_BLOWUP      21  // followed by CSegment, nSide, and pnt
#define ND_EVENT_HOMING_DISTANCE    22  // followed by homing distance
#define ND_EVENT_LETTERBOX          23  // letterbox mode for death seq.
#define ND_EVENT_RESTORE_COCKPIT    24  // restore cockpit after death
#define ND_EVENT_REARVIEW           25  // going to rear view mode
#define ND_EVENT_WALL_SET_TMAP_NUM1 26  // Wall changed
#define ND_EVENT_WALL_SET_TMAP_NUM2 27  // Wall changed
#define ND_EVENT_NEW_LEVEL          28  // followed by level number
#define ND_EVENT_MULTI_CLOAK        29  // followed by CPlayerData num
#define ND_EVENT_MULTI_DECLOAK      30  // followed by CPlayerData num
#define ND_EVENT_RESTORE_REARVIEW   31  // restore cockpit after rearview mode
#define ND_EVENT_MULTI_DEATH        32  // with CPlayerData number
#define ND_EVENT_MULTI_KILL         33  // with CPlayerData number
#define ND_EVENT_MULTI_CONNECT      34  // with CPlayerData number
#define ND_EVENT_MULTI_RECONNECT    35  // with CPlayerData number
#define ND_EVENT_MULTI_DISCONNECT   36  // with CPlayerData number
#define ND_EVENT_MULTI_SCORE        37  // playernum / score
#define ND_EVENT_PLAYER_SCORE       38  // followed by score
#define ND_EVENT_PRIMARY_AMMO       39  // with old/new ammo count
#define ND_EVENT_SECONDARY_AMMO     40  // with old/new ammo count
#define ND_EVENT_DOOR_OPENING       41  // with CSegment/nSide
#define ND_EVENT_LASER_LEVEL        42
  // no data
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

CFile ndInFile;
CFile ndOutFile;

//	-----------------------------------------------------------------------------

int NDErrorMsg (const char *pszMsg1, const char *pszMsg2, const char *pszMsg3)
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
	return (ndInFile.Tell () * 100) / gameData.demo.nSize;
if (gameData.demo.nState == ND_STATE_RECORDING)
	return ndOutFile.Tell ();
return 0;
}

//	-----------------------------------------------------------------------------

#define VEL_PRECISION 12

void my_extract_shortpos (CObject *objP, tShortPos *spp)
{
	int nSegment;
	sbyte *sp;

sp = spp->orient;
objP->info.position.mOrient [RVEC][X] = *sp++ << MATRIX_PRECISION;
objP->info.position.mOrient [UVEC][X] = *sp++ << MATRIX_PRECISION;
objP->info.position.mOrient [FVEC][X] = *sp++ << MATRIX_PRECISION;
objP->info.position.mOrient [RVEC][Y] = *sp++ << MATRIX_PRECISION;
objP->info.position.mOrient [UVEC][Y] = *sp++ << MATRIX_PRECISION;
objP->info.position.mOrient [FVEC][Y] = *sp++ << MATRIX_PRECISION;
objP->info.position.mOrient [RVEC][Z] = *sp++ << MATRIX_PRECISION;
objP->info.position.mOrient [UVEC][Z] = *sp++ << MATRIX_PRECISION;
objP->info.position.mOrient [FVEC][Z] = *sp++ << MATRIX_PRECISION;
nSegment = spp->nSegment;
objP->info.nSegment = nSegment;
const CFixVector& v = gameData.segs.vertices [gameData.segs.segments [nSegment].verts [0]];
objP->info.position.vPos [X] = (spp->pos [X] << RELPOS_PRECISION) + v [X];
objP->info.position.vPos [Y] = (spp->pos [Y] << RELPOS_PRECISION) + v [Y];
objP->info.position.vPos [Z] = (spp->pos [Z] << RELPOS_PRECISION) + v [Z];
objP->mType.physInfo.velocity [X] = (spp->vel [X] << VEL_PRECISION);
objP->mType.physInfo.velocity [Y] = (spp->vel [Y] << VEL_PRECISION);
objP->mType.physInfo.velocity [Z] = (spp->vel [Z] << VEL_PRECISION);
}

//	-----------------------------------------------------------------------------

int NDFindObject (int nSignature)
{
	int 		i;
	CObject 	*objP = OBJECTS.Buffer ();

FORALL_OBJSi (objP, i)
	if ((objP->info.nType != OBJ_NONE) && (objP->info.nSignature == nSignature))
		return OBJ_IDX (objP);
return -1;
}

//	-----------------------------------------------------------------------------

#if DBG

void CHK (void)
{
Assert (&ndOutFile.File () != NULL);
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
Assert (&ndOutFile.File () != NULL);
CHK();
nWritten = ndOutFile.Write (buffer, elsize, nelem);
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
 *  these since the demo must save more information about OBJECTS that
 *  just a gamesave
*/

//	-----------------------------------------------------------------------------

static inline void NDWriteByte (sbyte b)
{
gameData.demo.nFrameBytesWritten += sizeof (b);
gameData.demo.nWritten += sizeof (b);
CHK();
ndOutFile.WriteByte (b);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteShort (short s)
{
gameData.demo.nFrameBytesWritten += sizeof (s);
gameData.demo.nWritten += sizeof (s);
CHK();
ndOutFile.WriteShort (s);
}

//	-----------------------------------------------------------------------------

static void NDWriteInt (int i)
{
gameData.demo.nFrameBytesWritten += sizeof (i);
gameData.demo.nWritten += sizeof (i);
CHK();
ndOutFile.WriteInt (i);
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
ndOutFile.WriteFix (f);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteFixAng (fixang f)
{
gameData.demo.nFrameBytesWritten += sizeof (f);
gameData.demo.nWritten += sizeof (f);
ndOutFile.WriteFixAng (f);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteVector(const CFixVector& v)
{
gameData.demo.nFrameBytesWritten += sizeof (v);
gameData.demo.nWritten += sizeof (v);
CHK();
ndOutFile.WriteVector (v);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteAngVec (const vmsAngVec& v)
{
gameData.demo.nFrameBytesWritten += sizeof (v);
gameData.demo.nWritten += sizeof (v);
CHK();
ndOutFile.WriteAngVec (v);
}

//	-----------------------------------------------------------------------------

static inline void NDWriteMatrix (const vmsMatrix& m)
{
gameData.demo.nFrameBytesWritten += sizeof (m);
gameData.demo.nWritten += sizeof (m);
CHK();
ndOutFile.WriteMatrix(m);
}

//	-----------------------------------------------------------------------------

void NDWritePosition (CObject *objP)
{
	ubyte			renderType = objP->info.renderType;
	tShortPos	sp;
	int			bOldFormat = gameStates.app.bNostalgia || gameOpts->demo.bOldFormat || (bRevertFormat > 0);

if (bOldFormat)
	CreateShortPos (&sp, objP, 0);
if ((renderType == RT_POLYOBJ) || (renderType == RT_HOSTAGE) || (renderType == RT_MORPH) || 
	 (objP->info.nType == OBJ_CAMERA) ||
	 (!bOldFormat && (renderType == RT_POWERUP))) {
	if (bOldFormat)
		NDWrite (sp.orient, sizeof (sp.orient [0]), sizeof (sp.orient) / sizeof (sp.orient [0]));
	else
		NDWriteMatrix(objP->info.position.mOrient);
	}
if (bOldFormat) {
	NDWriteShort (sp.pos [X]);
	NDWriteShort (sp.pos [Y]);
	NDWriteShort (sp.pos [Z]);
	NDWriteShort (sp.nSegment);
	NDWriteShort (sp.vel [X]);
	NDWriteShort (sp.vel [Y]);
	NDWriteShort (sp.vel [Z]);
	}
else {
	NDWriteVector(objP->info.position.vPos);
	NDWriteShort (objP->info.nSegment);
	NDWriteVector(objP->mType.physInfo.velocity);
	}
}

//	-----------------------------------------------------------------------------

int NDRead (void *buffer, int elsize, int nelem)
{
int nRead = (int) ndInFile.Read (buffer, elsize, nelem);
if (ndInFile.Error () || ndInFile.EoF ())
	bNDBadRead = -1;
else if (bRevertFormat > 0)
	NDWrite (buffer, elsize, nelem);
return nRead;
}

//	-----------------------------------------------------------------------------

static inline ubyte NDReadByte (void)
{
if (bRevertFormat > 0) {
	ubyte	h = ndInFile.ReadByte ();
	NDWriteByte (h);
	return h;
	}
return ndInFile.ReadByte ();
}

//	-----------------------------------------------------------------------------

static inline short NDReadShort (void)
{
if (bRevertFormat > 0) {
	short	h = ndInFile.ReadShort ();
	NDWriteShort (h);
	return h;
	}
return ndInFile.ReadShort ();
}

//	-----------------------------------------------------------------------------

static inline int NDReadInt ()
{
if (bRevertFormat > 0) {
	int h = ndInFile.ReadInt ();
	NDWriteInt (h);
	return h;
	}
return ndInFile.ReadInt ();
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
	fix h = ndInFile.ReadFix ();
	NDWriteFix (h);
	return h;
	}
return ndInFile.ReadFix ();
}

//	-----------------------------------------------------------------------------

static inline fixang NDReadFixAng (void)
{
if (bRevertFormat > 0) {
	fixang h = ndInFile.ReadFixAng ();
	NDWriteFixAng (h);
	return h;
	}
return ndInFile.ReadFixAng ();
}

//	-----------------------------------------------------------------------------

static inline void NDReadVector(CFixVector& v)
{
ndInFile.ReadVector(v);
if (bRevertFormat > 0)
	NDWriteVector(v);
}

//	-----------------------------------------------------------------------------

static inline void NDReadAngVec (vmsAngVec& v)
{
ndInFile.ReadAngVec (v);
if (bRevertFormat > 0)
	NDWriteAngVec (v);
}

//	-----------------------------------------------------------------------------

static inline void NDReadMatrix(vmsMatrix& m)
{
ndInFile.ReadMatrix(m);
if (bRevertFormat > 0)
	NDWriteMatrix (m);
}

//	-----------------------------------------------------------------------------

static void NDReadPosition (CObject *objP, int bSkip)
{
	tShortPos sp;
	ubyte renderType;

if (bRevertFormat > 0)
	bRevertFormat = 0;	//temporarily suppress writing back data
renderType = objP->info.renderType;
if ((renderType == RT_POLYOBJ) || (renderType == RT_HOSTAGE) || (renderType == RT_MORPH) || 
	 (objP->info.nType == OBJ_CAMERA) ||
	 ((renderType == RT_POWERUP) && (gameData.demo.nVersion > DEMO_VERSION + 1))) {
	if (gameData.demo.bUseShortPos)
		NDRead (sp.orient, sizeof (sp.orient [0]), sizeof (sp.orient) / sizeof (sp.orient [0]));
	else
		ndInFile.ReadMatrix(objP->info.position.mOrient);
	}
if (gameData.demo.bUseShortPos) {
	sp.pos [X] = NDReadShort ();
	sp.pos [Y] = NDReadShort ();
	sp.pos [Z] = NDReadShort ();
	sp.nSegment = NDReadShort ();
	sp.vel [X] = NDReadShort ();
	sp.vel [Y] = NDReadShort ();
	sp.vel [Z] = NDReadShort ();
	my_extract_shortpos (objP, &sp);
	}
else {
	NDReadVector(objP->info.position.vPos);
	objP->info.nSegment = NDReadShort ();
	NDReadVector(objP->mType.physInfo.velocity);
	}
if ((objP->info.nId == VCLIP_MORPHING_ROBOT) && 
	 (renderType == RT_FIREBALL) && 
	 (objP->info.controlType == CT_EXPLOSION))
	ExtractOrientFromSegment (&objP->info.position.mOrient, gameData.segs.segments + objP->info.nSegment);
if (!(bRevertFormat || bSkip)) {
	bRevertFormat = 1;
	NDWritePosition (objP);
	}
}

//	-----------------------------------------------------------------------------

CObject *prevObjP = NULL;      //ptr to last CObject read in

void NDReadObject (CObject *objP)
{
	int		bSkip = 0;

memset (objP, 0, sizeof (CObject));
/*
 * Do render nType first, since with renderType == RT_NONE, we
 * blow by all other CObject information
 */
if (bRevertFormat > 0)
	bRevertFormat = 0;
objP->info.renderType = NDReadByte ();
if (!bRevertFormat) {
	if (objP->info.renderType <= RT_WEAPON_VCLIP) {
		bRevertFormat = gameOpts->demo.bRevertFormat;
		NDWriteByte ((sbyte) objP->info.renderType);
		}
	else {
		bSkip = 1;
		ndOutFile.Seek (-1, SEEK_CUR);
		gameData.demo.nFrameBytesWritten--;
		gameData.demo.nWritten--;
		}
	}
objP->info.nType = NDReadByte ();
if ((objP->info.renderType == RT_NONE) && (objP->info.nType != OBJ_CAMERA)) {
	if (!bRevertFormat)
		bRevertFormat = gameOpts->demo.bRevertFormat;
	return;
	}
objP->info.nId = NDReadByte ();
if (gameData.demo.nVersion > DEMO_VERSION + 1) {
	if (bRevertFormat > 0)
		bRevertFormat = 0;
	objP->info.xShields = NDReadFix ();
	if (!(bRevertFormat || bSkip))
		bRevertFormat = gameOpts->demo.bRevertFormat;
	}
objP->info.nFlags = NDReadByte ();
objP->info.nSignature = NDReadShort ();
NDReadPosition (objP, bSkip);
#if DBG
if ((objP->info.nType == OBJ_ROBOT) && (objP->info.nId == SPECIAL_REACTOR_ROBOT))
	Int3 ();
#endif
objP->info.nAttachedObj = -1;
switch (objP->info.nType) {
	case OBJ_HOSTAGE:
		objP->info.controlType = CT_POWERUP;
		objP->info.movementType = MT_NONE;
		objP->info.xSize = HOSTAGE_SIZE;
		break;

	case OBJ_ROBOT:
		objP->info.controlType = CT_AI;
		// (MarkA and MikeK said we should not do the crazy last secret stuff with multiple reactors...
		// This necessary code is our vindication. --MK, 2/15/96)
		if (objP->info.nId != SPECIAL_REACTOR_ROBOT)
			objP->info.movementType = MT_PHYSICS;
		else
			objP->info.movementType = MT_NONE;
		objP->info.xSize = gameData.models.polyModels [ROBOTINFO (objP->info.nId).nModel].rad;
		objP->rType.polyObjInfo.nModel = ROBOTINFO (objP->info.nId).nModel;
		objP->rType.polyObjInfo.nSubObjFlags = 0;
		objP->cType.aiInfo.CLOAKED = (ROBOTINFO (objP->info.nId).cloakType?1:0);
		break;

	case OBJ_POWERUP:
		objP->info.controlType = CT_POWERUP;
		objP->info.movementType = NDReadByte ();        // might have physics movement
		objP->info.xSize = gameData.objs.pwrUp.info [objP->info.nId].size;
		break;

	case OBJ_PLAYER:
		objP->info.controlType = CT_NONE;
		objP->info.movementType = MT_PHYSICS;
		objP->info.xSize = gameData.models.polyModels [gameData.pig.ship.player->nModel].rad;
		objP->rType.polyObjInfo.nModel = gameData.pig.ship.player->nModel;
		objP->rType.polyObjInfo.nSubObjFlags = 0;
		break;

	case OBJ_CLUTTER:
		objP->info.controlType = CT_NONE;
		objP->info.movementType = MT_NONE;
		objP->info.xSize = gameData.models.polyModels [objP->info.nId].rad;
		objP->rType.polyObjInfo.nModel = objP->info.nId;
		objP->rType.polyObjInfo.nSubObjFlags = 0;
		break;

	default:
		objP->info.controlType = NDReadByte ();
		objP->info.movementType = NDReadByte ();
		objP->info.xSize = NDReadFix ();
		break;
	}

NDReadVector(objP->info.vLastPos);
if ((objP->info.nType == OBJ_WEAPON) && (objP->info.renderType == RT_WEAPON_VCLIP))
	objP->info.xLifeLeft = NDReadFix ();
else {
	ubyte b = NDReadByte ();
	objP->info.xLifeLeft = (fix) b;
	// MWA old way -- won't work with big endian machines       NDReadByte (reinterpret_cast<sbyte*> (reinterpret_cast<ubyte*> (&(objP->info.xLifeLeft);
	objP->info.xLifeLeft = (fix) ((int) objP->info.xLifeLeft << 12);
	}
if (objP->info.nType == OBJ_ROBOT) {
	if (ROBOTINFO (objP->info.nId).bossFlag) {
		sbyte cloaked = NDReadByte ();
		objP->cType.aiInfo.CLOAKED = cloaked;
		}
	}

switch (objP->info.movementType) {
	case MT_PHYSICS:
		NDReadVector(objP->mType.physInfo.velocity);
		NDReadVector(objP->mType.physInfo.thrust);
		break;

	case MT_SPINNING:
		NDReadVector(objP->mType.spinRate);
		break;

	case MT_NONE:
		break;

	default:
		Int3 ();
	}

switch (objP->info.controlType) {
	case CT_EXPLOSION:
		objP->cType.explInfo.nSpawnTime = NDReadFix ();
		objP->cType.explInfo.nDeleteTime = NDReadFix ();
		objP->cType.explInfo.nDeleteObj = NDReadShort ();
		objP->cType.explInfo.attached.nNext = 
		objP->cType.explInfo.attached.nPrev = 
		objP->cType.explInfo.attached.nParent = -1;
		if (objP->info.nFlags & OF_ATTACHED) {     //attach to previous CObject
			Assert (prevObjP != NULL);
			if (prevObjP->info.controlType == CT_EXPLOSION) {
				if ((prevObjP->info.nFlags & OF_ATTACHED) && (prevObjP->cType.explInfo.attached.nParent != -1))
					AttachObject (OBJECTS + prevObjP->cType.explInfo.attached.nParent, objP);
				else
					objP->info.nFlags &= ~OF_ATTACHED;
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

switch (objP->info.renderType) {
	case RT_NONE:
		break;

	case RT_MORPH:
	case RT_POLYOBJ: {
		int i, tmo;
		if ((objP->info.nType != OBJ_ROBOT) && (objP->info.nType != OBJ_PLAYER) && (objP->info.nType != OBJ_CLUTTER)) {
			objP->rType.polyObjInfo.nModel = NDReadInt ();
			objP->rType.polyObjInfo.nSubObjFlags = NDReadInt ();
			}
		if ((objP->info.nType != OBJ_PLAYER) && (objP->info.nType != OBJ_DEBRIS))
		for (i = 0; i < gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nModels; i++)
			NDReadAngVec (objP->rType.polyObjInfo.animAngles[i]);
		tmo = NDReadInt ();
#ifndef EDITOR
		objP->rType.polyObjInfo.nTexOverride = tmo;
#else
		if (tmo == -1)
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
void NDSetPowerupClip (CObject *objP)
{
//if (gameStates.app.tick40fps.bTick) 
	tVClipInfo	*vciP = &objP->rType.vClipInfo;
	tVideoClip	*vcP = gameData.eff.vClips [0] + vciP->nClipIndex;

vciP->nCurFrame = vcP->xFrameTime ? ((gameData.time.xGame - gameData.demo.xStartTime) / vcP->xFrameTime) % vcP->nFrameCount : 0;
}

//	-----------------------------------------------------------------------------

void NDWriteObject (CObject *objP)
{
	int		life;
	CObject	o = *objP;

if ((o.info.renderType > RT_WEAPON_VCLIP) && ((gameStates.app.bNostalgia || gameOpts->demo.bOldFormat)))
	return;
#if DBG
if ((o.info.nType == OBJ_ROBOT) && (o.info.nId == SPECIAL_REACTOR_ROBOT))
	Int3 ();
#endif
if (o.cType.aiInfo.behavior == AIB_STATIC)
	o.info.movementType = MT_PHYSICS;
// Do renderType first so on read, we can make determination of
// what else to read in
if ((o.info.nType == OBJ_POWERUP) && (o.info.renderType == RT_POLYOBJ)) {
	ConvertWeaponToPowerup (&o);
	NDSetPowerupClip (&o);
	}
NDWriteByte (o.info.renderType);
NDWriteByte (o.info.nType);
if ((o.info.renderType == RT_NONE) && (o.info.nType != OBJ_CAMERA))
	return;
NDWriteByte (o.info.nId);
if (!(gameStates.app.bNostalgia || gameOpts->demo.bOldFormat))
	NDWriteFix (o.info.xShields);
NDWriteByte (o.info.nFlags);
NDWriteShort ((short) o.info.nSignature);
NDWritePosition (&o);
if ((o.info.nType != OBJ_HOSTAGE) && (o.info.nType != OBJ_ROBOT) && (o.info.nType != OBJ_PLAYER) && 
	 (o.info.nType != OBJ_POWERUP) && (o.info.nType != OBJ_CLUTTER)) {
	NDWriteByte (o.info.controlType);
	NDWriteByte (o.info.movementType);
	NDWriteFix (o.info.xSize);
	}
else if (o.info.nType == OBJ_POWERUP)
	NDWriteByte (o.info.movementType);
NDWriteVector(o.info.vLastPos);
if ((o.info.nType == OBJ_WEAPON) && (o.info.renderType == RT_WEAPON_VCLIP))
	NDWriteFix (o.info.xLifeLeft);
else {
	life = ((int) o.info.xLifeLeft) >> 12;
	if (life > 255)
		life = 255;
	NDWriteByte ((ubyte)life);
	}
if (o.info.nType == OBJ_ROBOT) {
	if (ROBOTINFO (o.info.nId).bossFlag) {
		int i = FindBoss (OBJ_IDX (objP));
		if ((i >= 0) &&
			 (gameData.time.xGame > gameData.boss [i].nCloakStartTime) && 
			 (gameData.time.xGame < gameData.boss [i].nCloakEndTime))
			NDWriteByte (1);
		else
			NDWriteByte (0);
		}
	}
switch (o.info.movementType) {
	case MT_PHYSICS:
		NDWriteVector(o.mType.physInfo.velocity);
		NDWriteVector(o.mType.physInfo.thrust);
		break;

	case MT_SPINNING:
		NDWriteVector(o.mType.spinRate);
		break;

	case MT_NONE:
		break;

	default:
		Int3 ();
	}

switch (o.info.controlType) {
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
	case CT_SLEW:       //the CPlayerData is generally saved as slew
	case CT_CNTRLCEN:
	case CT_REMOTE:
	case CT_MORPH:
		break;

	case CT_REPAIRCEN:
	case CT_FLYTHROUGH:
	default:
		Int3 ();
	}

switch (o.info.renderType) {
	case RT_NONE:
		break;

	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
		if ((o.info.nType != OBJ_ROBOT) && (o.info.nType != OBJ_PLAYER) && (o.info.nType != OBJ_CLUTTER)) {
			NDWriteInt (o.rType.polyObjInfo.nModel);
			NDWriteInt (o.rType.polyObjInfo.nSubObjFlags);
			}
		if ((o.info.nType != OBJ_PLAYER) && (o.info.nType != OBJ_DEBRIS))
#if 0
			for (i = 0; i < MAX_SUBMODELS; i++)
				NDWriteAngVec (o.polyObjInfo.animAngles + i);
#endif
		for (i = 0; i < gameData.models.polyModels [o.rType.polyObjInfo.nModel].nModels; i++)
			NDWriteAngVec (o.rType.polyObjInfo.animAngles[i]);
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

void NDRecordStartDemo (void)
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
	NDWriteString (netGame.szTeamName [0]);
	NDWriteString (netGame.szTeamName [1]);
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
NDWriteByte ((sbyte) (X2IR (LOCALPLAYER.energy)));
NDWriteByte ((sbyte) (X2IR (LOCALPLAYER.shields)));
NDWriteInt (LOCALPLAYER.flags);        // be sure players flags are set
NDWriteByte ((sbyte)gameData.weapons.nPrimary);
NDWriteByte ((sbyte)gameData.weapons.nSecondary);
gameData.demo.nStartFrame = gameData.app.nFrameCount;
bJustStartedRecording = 1;
NDSetNewLevel (gameData.missions.nCurrentLevel);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordStartFrame (int nFrameNumber, fix xFrameTime)
{
if (gameData.demo.bNoSpace) {
	NDStopPlayback ();
	return;
	}
StopTime ();
gameData.demo.bWasRecorded.Clear ();
gameData.demo.bViewWasRecorded.Clear ();
memset (gameData.demo.bRenderingWasRecorded, 0, sizeof (gameData.demo.bRenderingWasRecorded));
nFrameNumber -= gameData.demo.nStartFrame;
Assert (nFrameNumber >= 0);
NDWriteByte (ND_EVENT_START_FRAME);
NDWriteShort ((short) (gameData.demo.nFrameBytesWritten - 1));        // from previous frame
gameData.demo.nFrameBytesWritten = 3;
NDWriteInt (nFrameNumber);
NDWriteInt (xFrameTime);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordRenderObject (CObject * objP)
{
if (gameData.demo.bViewWasRecorded [OBJ_IDX (objP)])
	return;
//if (obj==&OBJECTS [LOCALPLAYER.nObject] && !gameStates.app.bPlayerIsDead)
//	return;
StopTime ();
NDWriteByte (ND_EVENT_RENDER_OBJECT);
NDWriteObject (objP);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordViewerObject (CObject * objP)
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
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordSound (int soundno)
{
StopTime ();
NDWriteByte (ND_EVENT_SOUND);
NDWriteInt (soundno);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordCockpitChange (int mode)
{
StopTime ();
NDWriteByte (ND_EVENT_CHANGE_COCKPIT);
NDWriteInt (mode);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordSound3D (int soundno, int angle, int volume)
{
StopTime ();
NDWriteByte (ND_EVENT_SOUND_3D);
NDWriteInt (soundno);
NDWriteInt (angle);
NDWriteInt (volume);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordSound3DOnce (int soundno, int angle, int volume)
{
StopTime ();
NDWriteByte (ND_EVENT_SOUND_3D_ONCE);
NDWriteInt (soundno);
NDWriteInt (angle);
NDWriteInt (volume);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordLinkSoundToObject3 (int soundno, short nObject, fix maxVolume, fix  maxDistance, int loop_start, int loop_end)
{
StopTime ();
NDWriteByte (ND_EVENT_LINK_SOUND_TO_OBJ);
NDWriteInt (soundno);
NDWriteInt (OBJECTS [nObject].info.nSignature);
NDWriteInt (maxVolume);
NDWriteInt (maxDistance);
NDWriteInt (loop_start);
NDWriteInt (loop_end);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordKillSoundLinkedToObject (int nObject)
{
StopTime ();
NDWriteByte (ND_EVENT_KILL_SOUND_TO_OBJ);
NDWriteInt (OBJECTS [nObject].info.nSignature);
StartTime (0);
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
StartTime (0);
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
StartTime (0);
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
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordHostageRescued (int hostage_number) 
{
StopTime ();
NDWriteByte (ND_EVENT_HOSTAGE_RESCUED);
NDWriteInt (hostage_number);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordMorphFrame (tMorphInfo *md)
{
StopTime ();
NDWriteByte (ND_EVENT_MORPH_FRAME);
NDWriteObject (md->objP);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordWallToggle (int nSegment, int nSide)
{
StopTime ();
NDWriteByte (ND_EVENT_WALL_TOGGLE);
NDWriteInt (nSegment);
NDWriteInt (nSide);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordControlCenterDestroyed ()
{
StopTime ();
NDWriteByte (ND_EVENT_CONTROL_CENTER_DESTROYED);
NDWriteInt (gameData.reactor.countdown.nSecsLeft);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordHUDMessage (char * message)
{
StopTime ();
NDWriteByte (ND_EVENT_HUD_MESSAGE);
NDWriteString (message);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordPaletteEffect (short r, short g, short b)
{
StopTime ();
NDWriteByte (ND_EVENT_PALETTE_EFFECT);
NDWriteShort (r);
NDWriteShort (g);
NDWriteShort (b);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerEnergy (int old_energy, int energy)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_ENERGY);
NDWriteByte ((sbyte) old_energy);
NDWriteByte ((sbyte) energy);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerAfterburner (fix old_afterburner, fix afterburner)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_AFTERBURNER);
NDWriteByte ((sbyte) (old_afterburner>>9));
NDWriteByte ((sbyte) (afterburner>>9));
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerShields (int old_shield, int shield)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_SHIELD);
NDWriteByte ((sbyte)old_shield);
NDWriteByte ((sbyte)shield);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerFlags (uint oflags, uint flags)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_FLAGS);
NDWriteInt (( (short)oflags << 16) | (short)flags);
StartTime (0);
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
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordEffectBlowup (short CSegment, int nSide, CFixVector *pnt)
{
StopTime ();
NDWriteByte (ND_EVENT_EFFECT_BLOWUP);
NDWriteShort (CSegment);
NDWriteByte ((sbyte)nSide);
NDWriteVector(*pnt);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordHomingDistance (fix distance)
{
StopTime ();
NDWriteByte (ND_EVENT_HOMING_DISTANCE);
NDWriteShort ((short) (distance>>16));
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordLetterbox (void)
{
StopTime ();
NDWriteByte (ND_EVENT_LETTERBOX);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordRearView (void)
{
StopTime ();
NDWriteByte (ND_EVENT_REARVIEW);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordRestoreCockpit (void)
{
StopTime ();
NDWriteByte (ND_EVENT_RESTORE_COCKPIT);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordRestoreRearView (void)
{
StopTime ();
NDWriteByte (ND_EVENT_RESTORE_REARVIEW);
StartTime (0);
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
StartTime (0);
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
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordMultiCloak (int nPlayer)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_CLOAK);
NDWriteByte ((sbyte)nPlayer);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordMultiDeCloak (int nPlayer)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_DECLOAK);
NDWriteByte ((sbyte)nPlayer);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordMultiDeath (int nPlayer)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_DEATH);
NDWriteByte ((sbyte)nPlayer);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordMultiKill (int nPlayer, sbyte kill)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_KILL);
NDWriteByte ((sbyte)nPlayer);
NDWriteByte (kill);
StartTime (0);
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
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordMultiReconnect (int nPlayer)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_RECONNECT);
NDWriteByte ((sbyte)nPlayer);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordMultiDisconnect (int nPlayer)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_DISCONNECT);
NDWriteByte ((sbyte)nPlayer);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordPlayerScore (int score)
{
StopTime ();
NDWriteByte (ND_EVENT_PLAYER_SCORE);
NDWriteInt (score);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordMultiScore (int nPlayer, int score)
{
StopTime ();
NDWriteByte (ND_EVENT_MULTI_SCORE);
NDWriteByte ((sbyte)nPlayer);
NDWriteInt (score - gameData.multiplayer.players [nPlayer].score);      // called before score is changed!!!!
StartTime (0);
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
StartTime (0);
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
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordDoorOpening (int nSegment, int nSide)
{
StopTime ();
NDWriteByte (ND_EVENT_DOOR_OPENING);
NDWriteShort ((short)nSegment);
NDWriteByte ((sbyte)nSide);
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDRecordLaserLevel (sbyte oldLevel, sbyte newLevel)
{
StopTime ();
NDWriteByte (ND_EVENT_LASER_LEVEL);
NDWriteByte (oldLevel);
NDWriteByte (newLevel);
StartTime (0);
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
StartTime (0);
}

//	-----------------------------------------------------------------------------

void NDSetNewLevel (int level_num)
{
	int i;
	int nSide;
	CSegment *segP;

StopTime ();
NDWriteByte (ND_EVENT_NEW_LEVEL);
NDWriteByte ((sbyte)level_num);
NDWriteByte ((sbyte)gameData.missions.nCurrentLevel);

if (bJustStartedRecording == 1) {
	NDWriteInt (gameData.walls.nWalls);
	for (i = 0; i < gameData.walls.nWalls; i++) {
		NDWriteByte (gameData.walls.walls [i].nType);
		NDWriteByte (gameData.walls.walls [i].flags);
		NDWriteByte (gameData.walls.walls [i].state);
		segP = &gameData.segs.segments [gameData.walls.walls [i].nSegment];
		nSide = gameData.walls.walls [i].nSide;
		NDWriteShort (segP->sides [nSide].nBaseTex);
		NDWriteShort (segP->sides [nSide].nOvlTex | (segP->sides [nSide].nOvlOrient << 14));
		bJustStartedRecording = 0;
		}
	}
StartTime (0);
}

//	-----------------------------------------------------------------------------

int NDReadDemoStart (int bRandom)
{
	sbyte	i, gameType, laserLevel;
	char	c, energy, shield;
	int	nVersionFilter;
	char	szMsg [128], szCurrentMission [FILENAME_LEN];

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
	NDReadString (netGame.szTeamName [0]);
	NDReadString (netGame.szTeamName [1]);
	}
if (gameData.demo.nGameMode & GM_MULTI) {
	MultiNewGame ();
	c = NDReadByte ();
	gameData.multiplayer.nPlayers = (int)c;
	// changed this to above two lines -- breaks on the mac because of
	// endian issues
	//		NDReadByte (reinterpret_cast<sbyte*> (&gameData.multiplayer.nPlayers);
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
LOCALPLAYER.energy = I2X (energy);
LOCALPLAYER.shields = I2X (shield);
bJustStartedPlayback=1;
return 0;
}

//	-----------------------------------------------------------------------------

void NDPopCtrlCenTriggers ()
{
	short		anim_num, n, i;
	short		side, nConnSide;
	CSegment *segP, *connSegP;

for (i = 0; i < gameData.reactor.triggers.nLinks; i++) {
	segP = gameData.segs.segments + gameData.reactor.triggers.nSegment [i];
	side = gameData.reactor.triggers.nSide [i];
	connSegP = gameData.segs.segments + segP->children [side];
	nConnSide = FindConnectedSide (segP, connSegP);
	anim_num = gameData.walls.walls [WallNumP (segP, side)].nClip;
	n = gameData.walls.animP [anim_num].nFrameCount;
	if (gameData.walls.animP [anim_num].flags & WCF_TMAP1)
		segP->sides [side].nBaseTex = 
		connSegP->sides [nConnSide].nBaseTex = gameData.walls.animP [anim_num].frames [n-1];
	else
		segP->sides [side].nOvlTex = 
		connSegP->sides [nConnSide].nOvlTex = gameData.walls.animP [anim_num].frames [n-1];
	}
}

//	-----------------------------------------------------------------------------

int NDUpdateSmoke (void)
{
if (!EGI_FLAG (bUseParticles, 0, 1, 0))
	return 0;
else {
		int					i, nObject;
		CParticleSystem	*systemP;

	for (i = particleManager.GetUsed (); i >= 0; i = systemP->GetNext ()) {
		systemP = &particleManager.GetSystem (i);
		nObject = NDFindObject (systemP->m_nSignature);
		if (nObject < 0) {
			particleManager.SetObjectSystem (systemP->m_nObject, -1);
			systemP->SetLife (0);
			}
		else {
			particleManager.SetObjectSystem (nObject, i);
			systemP->m_nObject = nObject;
			}
		}
	return 1;
	}
}

//	-----------------------------------------------------------------------------

void NDRenderExtras (ubyte, CObject *); extern void MultiApplyGoalTextures ();

int NDReadFrameInfo ()
{
	int bDone, nSegment, nTexture, nSide, nObject, soundno, angle, volume, i, shot;
	CObject *objP;
	ubyte nTag, nPrevTag, WhichWindow;
	static sbyte saved_letter_cockpit;
	static sbyte saved_rearview_cockpit;
	CObject extraobj;
	static char LastReadValue=101;
	CSegment *segP;

bDone = 0;
nTag = 255;
if (gameData.demo.nVcrState != ND_STATE_PAUSED)
	ResetSegObjLists ();
ResetObjects (1);
LOCALPLAYER.homingObjectDist = -F1_0;
prevObjP = NULL;
while (!bDone) {
	nPrevTag = nTag;
	nTag = NDReadByte ();
	CATCH_BAD_READ
	switch (nTag) {
		case ND_EVENT_START_FRAME: {        // Followed by an integer frame number, then a fix gameData.time.xFrame
			short nPrevFrameLength;

			bDone = 1;
			//PrintLog ("%4d %4d %d\n", gameData.demo.nFrameCount, gameData.demo.nFrameBytesWritten - 1, CFTell (&ndOutFile);
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

		case ND_EVENT_VIEWER_OBJECT:        // Followed by an CObject structure
			WhichWindow = NDReadByte ();
			if (WhichWindow&15) {
				NDReadObject (&extraobj);
				if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
					CATCH_BAD_READ
					NDRenderExtras (WhichWindow, &extraobj);
					}
				}
			else {
				gameData.objs.viewerP = OBJECTS.Buffer ();
				NDReadObject (gameData.objs.viewerP);
				if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
					CATCH_BAD_READ
					nSegment = gameData.objs.viewerP->info.nSegment;
					gameData.objs.viewerP->info.nNextInSeg = 
					gameData.objs.viewerP->info.nPrevInSeg = 
					gameData.objs.viewerP->info.nSegment = -1;

					// HACK HACK HACK -- since we have multiple level recording, it can be the case
					// HACK HACK HACK -- that when rewinding the demo, the viewer is in a CSegment
					// HACK HACK HACK -- that is greater than the highest index of segments.  Bash
					// HACK HACK HACK -- the viewer to CSegment 0 for bogus view.

					if (nSegment > gameData.segs.nLastSegment)
						nSegment = 0;
					gameData.objs.viewerP->LinkToSeg (nSegment);
					}
				}
			break;

		case ND_EVENT_RENDER_OBJECT:       // Followed by an CObject structure
			nObject = AllocObject ();
			if (nObject == -1)
				break;
			objP = OBJECTS + nObject;
			NDReadObject (objP);
			CATCH_BAD_READ
			if (objP->info.controlType == CT_POWERUP)
				DoPowerupFrame (objP);
			if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
				nSegment = objP->info.nSegment;
				objP->info.nNextInSeg = objP->info.nPrevInSeg = objP->info.nSegment = -1;
				// HACK HACK HACK -- don't render OBJECTS is segments greater than gameData.segs.nLastSegment
				// HACK HACK HACK -- (see above)
				if (nSegment > gameData.segs.nLastSegment)
					break;
				objP->LinkToSeg (nSegment);
				if ((objP->info.nType == OBJ_PLAYER) && IsMultiGame) {
					int CPlayerData = IsTeamGame ? GetTeam (objP->info.nId) : objP->info.nId;
					if (CPlayerData == 0)
						break;
					CPlayerData--;
					for (i = 0; i < N_PLAYER_SHIP_TEXTURES; i++)
						mpTextureIndex [CPlayerData] [i] = gameData.pig.tex.objBmIndex [gameData.pig.tex.objBmIndexP [gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nFirstTexture+i]];
					mpTextureIndex [CPlayerData] [4] = gameData.pig.tex.objBmIndex [gameData.pig.tex.objBmIndexP [gameData.pig.tex.nFirstMultiBitmap+ (CPlayerData)*2]];
					mpTextureIndex [CPlayerData] [5] = gameData.pig.tex.objBmIndex [gameData.pig.tex.objBmIndexP [gameData.pig.tex.nFirstMultiBitmap+ (CPlayerData)*2+1]];
					objP->rType.polyObjInfo.nAltTextures = CPlayerData+1;
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
				DigiLinkSoundToObject3 ((short) soundno, (short) nObject, 1, maxVolume, maxDistance, loop_start, loop_end, NULL, 0, ObjectSoundClass (OBJECTS + nObject));
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
				int CPlayerData, nSegment;
				fix damage;

			nSegment = NDReadInt ();
			nSide = NDReadInt ();
			damage = NDReadFix ();
			CPlayerData = NDReadInt ();
			CATCH_BAD_READ
			if (gameData.demo.nVcrState != ND_STATE_PAUSED)
				WallHitProcess (&gameData.segs.segments [nSegment], (short) nSide, damage, CPlayerData, &(OBJECTS [0]));
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

					nTag = NDReadByte ();
					Assert (nTag == ND_EVENT_SECRET_THINGY);
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
				RescueHostage (hostage_number);
			}
			break;

		case ND_EVENT_MORPH_FRAME: {
			nObject = AllocObject ();
			if (nObject==-1)
				break;
			objP = OBJECTS + nObject;
			NDReadObject (objP);
			objP->info.renderType = RT_POLYOBJ;
			if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
				CATCH_BAD_READ
				if (gameData.demo.nVcrState != ND_STATE_PAUSED) {
					nSegment = objP->info.nSegment;
					objP->info.nNextInSeg = objP->info.nPrevInSeg = objP->info.nSegment = -1;
					objP->LinkToSeg (nSegment);
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
			paletteManager.SetEffect (r, g, b);
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
				LOCALPLAYER.energy = I2X (energy);
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_energy != 255)
					LOCALPLAYER.energy = I2X (old_energy);
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
				LOCALPLAYER.shields = I2X (shield);
			else if ((gameData.demo.nVcrState == ND_STATE_REWINDING) || 
						(gameData.demo.nVcrState == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_shield != 255)
					LOCALPLAYER.shields = I2X (old_shield);
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
			CFixVector pnt;
			CObject dummy;

			//create a dummy CObject which will be the weapon that hits
			//the monitor. the blowup code wants to know who the parent of the
			//laser is, so create a laser whose parent is the CPlayerData
			dummy.cType.laserInfo.parent.nType = OBJ_PLAYER;
			nSegment = NDReadShort ();
			nSide = NDReadByte ();
			NDReadVector(pnt);
			if (gameData.demo.nVcrState != ND_STATE_PAUSED)
				CheckEffectBlowup (gameData.segs.segments + nSegment, nSide, &pnt, &dummy, 0);
			}
			break;

		case ND_EVENT_HOMING_DISTANCE: {
			short distance;

			distance = NDReadShort ();
			LOCALPLAYER.homingObjectDist = 
				I2X ((int) distance << 16);
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
				CSegment *segP, *oppSegP;

				segP = gameData.segs.segments + nSegment;
				oppSegP = gameData.segs.segments + segP->children [nSide];
				nConnSide = FindConnectedSide (segP, oppSegP);
				anim_num = gameData.walls.walls [WallNumP (segP, nSide)].nClip;
				if (gameData.walls.animP [anim_num].flags & WCF_TMAP1)
					segP->sides [nSide].nBaseTex = oppSegP->sides [nConnSide].nBaseTex =
						gameData.walls.animP [anim_num].frames [0];
				else
					segP->sides [nSide].nOvlTex = 
					oppSegP->sides [nConnSide].nOvlTex = gameData.walls.animP [anim_num].frames [0];
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
			CSegment *segP;
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
			paletteManager.ResetEffect ();                // get palette back to Normal
			StartTime (0);
			}
			break;

		case ND_EVENT_EOF:
			bDone = -1;
			ndInFile.Seek (-1, SEEK_CUR);        // get back to the EOF marker
			gameData.demo.bEof = 1;
			gameData.demo.nFrameCount++;
			break;

		default:
			bDone = -1;
			ndInFile.Seek (-1, SEEK_CUR);        // get back to the EOF marker
			gameData.demo.bEof = 1;
			gameData.demo.nFrameCount++;
			break;
		}
	}
LastReadValue = nTag;
if (bNDBadRead)
	NDErrorMsg (TXT_DEMO_ERR_READING, TXT_DEMO_OLD_CORRUPT, NULL);
else
	NDUpdateSmoke ();
return bDone;
}

//	-----------------------------------------------------------------------------

void NDGotoBeginning ()
{
ndInFile.Seek (0, SEEK_SET);
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

ndInFile.Seek (-2, SEEK_END);
level = NDReadByte ();
if ((level < gameData.missions.nLastSecretLevel) || (level > gameData.missions.nLastLevel)) {
	NDErrorMsg (TXT_CANT_PLAYBACK, TXT_LEVEL_CANT_LOAD, TXT_DEMO_OLD_CORRUPT);
	NDStopPlayback ();
	return;
	}
if (level != gameData.missions.nCurrentLevel)
	LoadLevel (level, 1, 0);
ndInFile.Seek (-4, SEEK_END);
byteCount = NDReadShort ();
ndInFile.Seek (-2 - byteCount, SEEK_CUR);

nFrameLength = NDReadShort ();
loc = ndInFile.Tell ();
if (gameData.demo.nGameMode & GM_MULTI)
	gameData.demo.bPlayersCloaked = NDReadByte ();
else
	bbyte = NDReadByte ();
bbyte = NDReadByte ();
bshort = NDReadShort ();
bint = NDReadInt ();
energy = NDReadByte ();
shield = NDReadByte ();
LOCALPLAYER.energy = I2X (energy);
LOCALPLAYER.shields = I2X (shield);
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
	//		NDReadByte (reinterpret_cast<sbyte*> (&gameData.multiplayer.nPlayers);
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
ndInFile.Seek (loc - nFrameLength, SEEK_SET);
gameData.demo.nFrameCount = NDReadInt ();            // get the frame count
gameData.demo.nFrameCount--;
ndInFile.Seek (4, SEEK_CUR);
gameData.demo.nVcrState = ND_STATE_PLAYBACK;
NDReadFrameInfo ();           // then the frame information
gameData.demo.nVcrState = ND_STATE_PAUSED;
return;
}

//	-----------------------------------------------------------------------------

inline void NDBackOneFrame (void)
{
	short nPrevFrameLength;

ndInFile.Seek (-10, SEEK_CUR);
nPrevFrameLength = NDReadShort ();
ndInFile.Seek (8 - nPrevFrameLength, SEEK_CUR);
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
 *  stored in the gameData.objs.viewerP CObject.  Save this position, and read the next
 *  frame to get all OBJECTS read in.  Calculate the delta playback and
 *  the delta recording frame times between the two frames, then intepolate
 *  the viewers position accordingly.  gameData.demo.xRecordedTime is the time that it
 *  took the recording to render the frame that we are currently looking
 *  at.
*/

void NDInterpolateFrame (fix d_play, fix d_recorded)
{
	int			nCurObjs;
	fix			factor;
	CFixVector  fvec1, fvec2, rvec1, rvec2;
	fix         mag1;
	fix			delta_x, delta_y, delta_z;
	ubyte			renderType;
	CObject		*curObjP, *objP, *i, *j;

	static CObject curObjs [MAX_OBJECTS_D2X];

factor = FixDiv (d_play, d_recorded);
if (factor > F1_0)
	factor = F1_0;
nCurObjs = gameData.objs.nLastObject [0];
#if 1
memcpy (curObjs, OBJECTS.Buffer (), OBJECTS.Size ());
#else
for (i = 0; i <= nCurObjs; i++)
	memcpy (&(curObjs [i]), &(OBJECTS [i]), sizeof (CObject));
#endif
gameData.demo.nVcrState = ND_STATE_PAUSED;
if (NDReadFrameInfo () == -1) {
	NDStopPlayback ();
	return;
	}
for (i = curObjs + nCurObjs, curObjP = curObjs; curObjP < i; curObjP++) {
	j = OBJECTS + gameData.objs.nLastObject [0];
	int h;
	FORALL_OBJSi (objP, h) {
		if (curObjP->info.nSignature == objP->info.nSignature) {
			renderType = curObjP->info.renderType;
			//fix delta_p, delta_h, delta_b;
			//vmsAngVec cur_angles, dest_angles;
			//  Extract the angles from the CObject orientation matrix.
			//  Some of this code taken from AITurnTowardsVector
			//  Don't do the interpolation on certain render types which don't use an orientation matrix
			if ((renderType != RT_LASER) &&
				 (renderType != RT_FIREBALL) && 
					(renderType != RT_THRUSTER) && 
					(renderType != RT_POWERUP)) {

				fvec1 = curObjP->info.position.mOrient[FVEC];
				fvec1 *= (F1_0-factor);
				fvec2 = objP->info.position.mOrient[FVEC];
				fvec2 *= factor;
				fvec1 += fvec2;
				mag1 = CFixVector::Normalize(fvec1);
				if (mag1 > F1_0/256) {
					rvec1 = curObjP->info.position.mOrient[RVEC];
					rvec1 *= (F1_0-factor);
					rvec2 = objP->info.position.mOrient[RVEC];
					rvec2 *= factor;
					rvec1 += rvec2;
					CFixVector::Normalize(rvec1); // Note: Doesn't matter if this is null, if null, VmVector2Matrix will just use fvec1
					curObjP->info.position.mOrient = vmsMatrix::CreateFR(fvec1, rvec1);
					//curObjP->info.position.mOrient = vmsMatrix::CreateFR(fvec1, NULL, &rvec1);
					}
				}
			// Interpolate the CObject position.  This is just straight linear
			// interpolation.
			delta_x = objP->info.position.vPos[X] - curObjP->info.position.vPos[X];
			delta_y = objP->info.position.vPos[Y] - curObjP->info.position.vPos[Y];
			delta_z = objP->info.position.vPos[Z] - curObjP->info.position.vPos[Z];
			delta_x = FixMul (delta_x, factor);
			delta_y = FixMul (delta_y, factor);
			delta_z = FixMul (delta_z, factor);
			curObjP->info.position.vPos[X] += delta_x;
			curObjP->info.position.vPos[Y] += delta_y;
			curObjP->info.position.vPos[Z] += delta_z;
				// -- old fashioned way --// stuff the new angles back into the CObject structure
				// -- old fashioned way --				VmAngles2Matrix (&(curObjs [i].info.position.mOrient), &cur_angles);
			}
		}
	}

// get back to original position in the demo file.  Reread the current
// frame information again to reset all of the CObject stuff not covered
// with gameData.objs.nLastObject [0] and the CObject array (previously rendered
// OBJECTS, etc....)
NDBackFrames (1);
NDBackFrames (1);
if (NDReadFrameInfo () == -1)
	NDStopPlayback ();
gameData.demo.nVcrState = ND_STATE_PLAYBACK;
OBJECTS = curObjs;
gameData.objs.nLastObject [0] = nCurObjs;
}

//	-----------------------------------------------------------------------------

void NDPlayBackOneFrame (void)
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
paletteManager.SetEffect (0, 0, 0);       //clear flash
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
		ndInFile.Seek (11, SEEK_CUR);
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
				CObject *curObjs, *objP;
				int i, j, nObjects, nLevel, nSig;

				nObjects = gameData.objs.nLastObject [0];
				if (!(curObjs = new CObject [nObjects + 1])) {
					Warning (TXT_INTERPOLATE_BOTS, sizeof (CObject) * nObjects);
					break;
					}
				memcpy (curObjs, OBJECTS.Buffer (), (nObjects + 1) * sizeof (CObject));
				nLevel = gameData.missions.nCurrentLevel;
				if (NDReadFrameInfo () == -1) {
					delete[] curObjs;
					NDStopPlayback ();
					return;
					}
				if (nLevel != gameData.missions.nCurrentLevel) {
					delete[] curObjs;
					if (NDReadFrameInfo () == -1)
						NDStopPlayback ();
					break;
					}
				//  for each new CObject in the frame just read in, determine if there is
				//  a corresponding CObject that we have been interpolating.  If so, then
				//  copy that interpolated CObject to the new OBJECTS array so that the
				//  interpolated position and orientation can be preserved.
				for (i = 0; i <= nObjects; i++) {
					nSig = curObjs [i].info.nSignature;
					objP;
					FORALL_OBJSi (objP, j) {
						if (nSig == objP->info.nSignature) {
							objP->info.position.mOrient = curObjs [i].info.position.mOrient;
							objP->info.position.vPos = curObjs [i].info.position.vPos;
							break;
							}
						}
					}
				delete[] curObjs;
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

void NDStartRecording (void)
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
ndOutFile.Open (DEMO_FILENAME, gameFolders.szDemoDir, "wb", 0);
#ifndef _WIN32_WCE
if (!ndOutFile.File () && (errno == ENOENT)) {   //dir doesn't exist?
#else
if (&ndOutFile.File ()) {                      //dir doesn't exist and no errno on mac!
#endif
	CFile::MkDir (gameFolders.szDemoDir); //try making directory
	ndOutFile.Open (DEMO_FILENAME, gameFolders.szDemoDir, "wb", 0);
	}
if (!ndOutFile.File ()) {
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
	ushort byteCount = 0;
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
NDWriteByte ((sbyte) (X2IR (LOCALPLAYER.energy)));
NDWriteByte ((sbyte) (X2IR (LOCALPLAYER.shields)));
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
ndOutFile.Close ();
}

//	-----------------------------------------------------------------------------

char demoname_allowed_chars [] = "azAZ09__--";
void NDStopRecording (void)
{
	tMenuItem m [6];
	int exit = 0;
	static char filename [15] = "", *s;
	static ubyte tmpcnt = 0;
	char fullname [15+FILENAME_LEN] = "";

NDFinishRecording ();
gameData.demo.nState = ND_STATE_NORMAL;
paletteManager.LoadEffect  ();
if (filename [0] != '\0') {
	int num, i = (int) strlen (filename) - 1;
	char newfile [15];

	while (::isdigit (filename [i])) {
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
	m [0].text = const_cast<char*> (TXT_DEMO_SAVE_BAD);
	m [1].nType = NM_TYPE_INPUT;
	m [1].text_len = 8; 
	m [1].text = filename;
	exit = ExecMenu (NULL, NULL, 2, m, NULL, NULL);
	} 
else if (gameData.demo.bNoSpace == 2) {
	m [0].nType = NM_TYPE_TEXT; 
	m [0].text = const_cast<char*> (TXT_DEMO_SAVE_NOSPACE);
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
	CFile::Delete (save_file, gameFolders.szDemoDir);
	CFile::Rename (DEMO_FILENAME, save_file, gameFolders.szDemoDir);
	return;
	}
if (exit == -1) {               // pressed ESC
	CFile::Delete (DEMO_FILENAME, gameFolders.szDemoDir);      // might as well remove the file
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
CFile::Delete (fullname, gameFolders.szDemoDir);
CFile::Rename (DEMO_FILENAME, fullname, gameFolders.szDemoDir);
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
				filename = reinterpret_cast<char*> (&ffs.name);
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
					filename = reinterpret_cast<char*> (&ffs.name);
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
if (!ndInFile.Open (filename2, gameFolders.szDemoDir, "rb", 0)) {
#if TRACE			
	con_printf (CONDBG, "Error reading '%s'\n", filename);
#endif
	return;
	}
if (bRevertFormat > 0) {
	strcat (filename2, ".v15");
	if (!ndOutFile.Open (filename2, gameFolders.szDemoDir, "wb", 0))
		bRevertFormat = -1;
	}
else
	ndOutFile.File () = NULL;
bNDBadRead = 0;
ChangePlayerNumTo (0);                 // force playernum to 0
strncpy (gameData.demo.callSignSave, LOCALPLAYER.callsign, CALLSIGN_LEN);
gameData.objs.viewerP = gameData.objs.consoleP = OBJECTS.Buffer ();   // play properly as if console CPlayerData
if (NDReadDemoStart (bRandom)) {
	ndInFile.Close ();
	ndOutFile.Close ();
	return;
	}
if (gameOpts->demo.bRevertFormat && ndOutFile.File () && (bRevertFormat < 0)) {
	ndOutFile.Close ();
	CFile::Delete (filename2, gameFolders.szDemoDir);
	}
gameData.app.nGameMode = GM_NORMAL;
gameData.demo.nState = ND_STATE_PLAYBACK;
gameData.demo.nVcrState = ND_STATE_PLAYBACK;
gameData.demo.nOldCockpit = gameStates.render.cockpit.nMode;
gameData.demo.nSize = ndInFile.Length ();
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
ResetTime ();
NDPlayBackOneFrame ();       // get all of the OBJECTS to renderb game
}

//	-----------------------------------------------------------------------------

void NDStopPlayback ()
{
if (bRevertFormat > 0) {
	int h = ndInFile.Length () - ndInFile.Tell ();
	char *p = new char [h];
	if (p) {
		bRevertFormat = 0;
		NDRead (p, h, 1);
		//PrintLog ("%4d %4d %d\n", gameData.demo.nFrameCount, gameData.demo.nFrameBytesWritten - 1, CFTell (&ndOutFile);
		NDWriteShort ((short) (gameData.demo.nFrameBytesWritten - 1));
		NDWrite (p + 3, h - 3, 1);
		D2_FREE (p);
		}
	ndOutFile.Close ();
	bRevertFormat = -1;
	}
ndInFile.Close ();
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

#if DBG

#define BUF_SIZE 16384

void NDStripFrames (char *outname, int bytes_to_strip)
{
	CFile	ndOutFile;
	char	*buf;
	int	nTotalSize, bytes_done, read_elems, bytes_back;
	int	trailer_start, loc1, loc2, stop_loc, bytes_to_read;
	short	nPrevFrameLength;

bytes_done = 0;
nTotalSize = ndInFile.Length ();
if (!ndOutFile.Open (outname, "", "wb", 0)) {
	NDErrorMsg ("Can't open output file", NULL, NULL);
	NDStopPlayback ();
	return;
	}
if (!(buf = new char [BUF_SIZE])) {
	NDErrorMsg ("Mot enough memory for output buffer", NULL, NULL);
	ndOutFile.Close ();
	NDStopPlayback ();
	return;
	}
NDGotoEnd ();
trailer_start = ndInFile.Tell ();
ndInFile.Seek (11, SEEK_CUR);
bytes_back = 0;
while (bytes_back < bytes_to_strip) {
	loc1 = ndInFile.Tell ();
	//ndInFile.Seek (-10, SEEK_CUR);
	//NDReadShort (&nPrevFrameLength);
	//ndInFile.Seek (8 - nPrevFrameLength, SEEK_CUR);
	NDBackFrames (1);
	loc2 = ndInFile.Tell ();
	bytes_back += (loc1 - loc2);
	}
ndInFile.Seek (-10, SEEK_CUR);
nPrevFrameLength = NDReadShort ();
ndInFile.Seek (-3, SEEK_CUR);
stop_loc = ndInFile.Tell ();
ndInFile.Seek (0, SEEK_SET);
while (stop_loc > 0) {
	if (stop_loc < BUF_SIZE)
		bytes_to_read = stop_loc;
	else
		bytes_to_read = BUF_SIZE;
	read_elems = (int) ndInFile.Read (buf, 1, bytes_to_read);
	ndOutFile.Write (buf, 1, read_elems);
	stop_loc -= read_elems;
	}
stop_loc = ndOutFile.Tell ();
ndInFile.Seek (trailer_start, SEEK_SET);
while ((read_elems = (int) ndInFile.Read (buf, 1, BUF_SIZE)))
	ndOutFile.Write (buf, 1, read_elems);
ndOutFile.Seek (stop_loc, SEEK_SET);
ndOutFile.Seek (1, SEEK_CUR);
ndOutFile.Write (&nPrevFrameLength, 2, 1);
ndOutFile.Close ();
NDStopPlayback ();
particleManager.Shutdown ();
}

#endif

//	-----------------------------------------------------------------------------

CObject demoRightExtra, demoLeftExtra;
ubyte nDemoDoRight=0, nDemoDoLeft=0;

void NDRenderExtras (ubyte which, CObject *objP)
{
	ubyte w=which>>4;
	ubyte nType=which&15;

if (which==255) {
	Int3 (); // how'd we get here?
	DoCockpitWindowView (w, NULL, 0, WBU_WEAPON, NULL);
	return;
	}
if (w) {
	memcpy (&demoRightExtra, objP, sizeof (CObject));  
	nDemoDoRight=nType;
	}
else {
	memcpy (&demoLeftExtra, objP, sizeof (CObject)); 
	nDemoDoLeft = nType;
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
		TimerDelay (xDelay);
		StartTime (0);
		}
	else {
		while (gameData.demo.xJasonPlaybackTotal > gameData.demo.xRecordedTotal)
			if (NDReadFrameInfo () == -1) {
				NDStopPlayback ();
				return;
				}
		//xDelay = gameData.demo.xRecordedTotal - gameData.demo.xJasonPlaybackTotal;
		//if (xDelay > f0_0)
		//	TimerDelay (xDelay);
		}
	}
gameData.demo.bFirstTimePlayback = 0;
}

//	-----------------------------------------------------------------------------
//eof
