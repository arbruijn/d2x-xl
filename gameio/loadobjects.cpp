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
#include <string.h>

#include "descent.h"
#include "pstypes.h"
#include "strutil.h"
#include "mono.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "menu.h"
#include "error.h"
#include "dynlight.h"
#include "object.h"
#include "game.h"
#include "screens.h"
#include "wall.h"
#include "loadgeometry.h"
#include "robot.h"
#include "fireball.h"

#include "cfile.h"
#include "loadgamedata.h"
#include "menu.h"
#include "trigger.h"
#include "producers.h"
#include "reactor.h"
#include "powerup.h"
#include "weapon.h"
#include "newdemo.h"
#include "loadgame.h"
#include "automap.h"
#include "polymodel.h"
#include "text.h"
#include "gamefont.h"
#include "loadobjects.h"
#include "gamepal.h"
#include "fireweapon.h"
#include "byteswap.h"
#include "multi.h"
#include "makesig.h"
#include "segmath.h"
#include "light.h"
#include "objrender.h"
#include "createmesh.h"
#include "lightmap.h"
#include "network.h"
#include "netmisc.h"
#include "network_lib.h"

#define GAME_VERSION            32
#define GAME_COMPATIBLE_VERSION 22

//version 28->29  add delta light support
//version 27->28  controlcen id now is reactor number, not model number
//version 28->29  ??
//version 29->30  changed CTrigger structure
//version 30->31  changed CTrigger structure some more
//version 31->32  change CSegment structure, make it 512 bytes w/o editor, add SEGMENTS array.

#define MENU_CURSOR_X_MIN       MENU_X
#define MENU_CURSOR_X_MAX       MENU_X+6

tGameFileInfo	gameFileInfo;
game_top_fileinfo	gameTopFileInfo;

//  LINT: adding function prototypes
int32_t nGameSavePlayers = 0;
int32_t nSavePOFNames = 0;
char szSavePOFNames [MAX_POLYGON_MODELS][SHORT_FILENAME_LEN];

//--unused-- CBitmap * Gamesave_saved_bitmap = NULL;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CObject::Verify (void)
{
SetLife (IMMORTAL_TIME);		//all loaded CObject are immortal, for now
if (Type () == OBJ_ROBOT) {
	++gameData.objData.nInitialRobots;
	// Make sure valid id...
	if (Id () >= gameData.botData.nTypes [gameStates.app.bD1Data])
		SetId (Id () % gameData.botData.nTypes [0]);
	// Make sure model number & size are correct...
	tRobotInfo *pRobotInfo = ROBOTINFO (Id ());
	if (pRobotInfo) {
		if (info.renderType == RT_POLYOBJ) {
			rType.polyObjInfo.nModel = pRobotInfo->nModel;
			AdjustSize ();
			}
		if (info.movementType == MT_PHYSICS) {
			mType.physInfo.mass = pRobotInfo->mass;
			mType.physInfo.drag = pRobotInfo->drag;
			}
		}
	if (Id () == 65)						//special "reactor" robots
		info.movementType = MT_NONE;
	}
else if (Type () == OBJ_EFFECT) {
	gameData.objData.nEffects++;
	}
else {		//Robots taken care of above
	if ((info.renderType == RT_POLYOBJ) && (nSavePOFNames > 0)) {
		char *name = szSavePOFNames [ModelId ()];
		if (*name) {
			for (int32_t i = 0; (i < gameData.models.nPolyModels) && (i < nSavePOFNames); i++)
				if (*name && !stricmp (pofNames [i], name)) {		//found it!
					rType.polyObjInfo.nModel = i;
					break;
					}
				}
		}
	}
if (Type () == OBJ_POWERUP) {
	if (Id () >= gameData.objData.pwrUp.nTypes + POWERUP_ADDON_COUNT) {
		SetId (0);
		Assert(info.renderType != RT_POLYOBJ);
		}
	info.controlType = CT_POWERUP;
	if (Id () >= MAX_POWERUP_TYPES_D2)
		InitAddonPowerup (this);
	else {
		SetSizeFromPowerup ();
		cType.powerupInfo.xCreationTime = 0;
		if (IsMultiGame) {
			AddPowerupInMine (Id (), 1, true);
#if TRACE
			console.printf (CON_DBG, "PowerupLimiter: ID=%d\n", Id ());
			if (Id () > MAX_POWERUP_TYPES)
				console.printf (1,"POWERUP: Overwriting array bounds!\n");
#endif
			}
		}
	}
else if (Type () == OBJ_WEAPON) {
	if (Id () >= gameData.weapons.nTypes [0]) {
		SetId (0);
		Assert(info.renderType != RT_POLYOBJ);
		}
	if (Id () == SMALLMINE_ID) {		//make sure pmines have correct values
		mType.physInfo.mass = gameData.weapons.info [Id ()].mass;
		mType.physInfo.drag = gameData.weapons.info [Id ()].drag;
		mType.physInfo.flags |= PF_FREE_SPINNING;
		// Make sure model number & size are correct...	
		Assert(info.renderType == RT_POLYOBJ);
		rType.polyObjInfo.nModel = gameData.weapons.info [Id ()].nModel;
		AdjustSize ();
		}
	}
else if (Type () == OBJ_REACTOR) {
	info.renderType = RT_POLYOBJ;
	info.controlType = CT_CNTRLCEN;
	if (gameData.segData.nLevelVersion <= 1) { // descent 1 reactor
		SetId (0);                         // used to be only one kind of reactor
		rType.polyObjInfo.nModel = gameData.reactor.props [0].nModel;// descent 1 reactor
		}
	}
else if (Type () == OBJ_PLAYER) {
	if (this == gameData.objData.pConsole)	
		InitPlayerObject ();
	else
		if (info.renderType == RT_POLYOBJ)	//recover from Matt's pof file matchup bug
			rType.polyObjInfo.nModel = gameData.pig.ship.player->nModel;
	//Make sure orient matrix is orthogonal
	gameOpts->render.nMathFormat = 0;
	info.position.mOrient.CheckAndFix();
	gameOpts->render.nMathFormat = gameOpts->render.nDefMathFormat;
	SetId (nGameSavePlayers++);
	}
else if (Type () == OBJ_HOSTAGE) {
	info.renderType = RT_HOSTAGE;
	info.controlType = CT_POWERUP;
	}
VerifyPosition ();
Link ();
}

//------------------------------------------------------------------------------

void CObject::VerifyPosition (void)
{
if (Type () != OBJ_EFFECT) {
	int32_t nSegment = FindSegByPos (Position (), Segment (), 1, 0);
	if (nSegment != Segment ()) {
		if (nSegment < 0) {
			nSegment = FindClosestSeg (Position ());
			SetPos (&SEGMENT (nSegment)->Center ());
			}
		SetSegment (nSegment);
		}
	}
}

//------------------------------------------------------------------------------
//static gs_skip(int32_t len,CFile *file)
//{
//
//	cf.Seek (file,len,SEEK_CUR);
//}

// -----------------------------------------------------------------------------

static void InitGameFileInfo (void)
{
gameFileInfo.level				=	-1;
gameFileInfo.player.offset		=	-1;
gameFileInfo.player.size		=	sizeof (CPlayerData);
gameFileInfo.objects.offset	=	-1;
gameFileInfo.objects.count		=	0;
gameFileInfo.objects.size		=	sizeof (CObject);  
gameFileInfo.walls.offset		=	-1;
gameFileInfo.walls.count		=	0;
gameFileInfo.walls.size			=	sizeof (CWall);  
gameFileInfo.doors.offset		=	-1;
gameFileInfo.doors.count		=	0;
gameFileInfo.doors.size			=	sizeof (CActiveDoor);  
gameFileInfo.triggers.offset	=	-1;
gameFileInfo.triggers.count	=	0;
gameFileInfo.triggers.size		=	sizeof (CTrigger);  
gameFileInfo.control.offset	=	-1;
gameFileInfo.control.count		=	0;
gameFileInfo.control.size		=	sizeof (CTriggerTargets);
gameFileInfo.botGen.offset		=	-1;
gameFileInfo.botGen.count		=	0;
gameFileInfo.botGen.size		=	sizeof (tObjectProducerInfo);
gameFileInfo.equipGen.offset	=	-1;
gameFileInfo.equipGen.count	=	0;
gameFileInfo.equipGen.size		=	sizeof (tObjectProducerInfo);
gameFileInfo.lightDeltaIndices.offset = -1;
gameFileInfo.lightDeltaIndices.count =	0;
gameFileInfo.lightDeltaIndices.size =	sizeof (CLightDeltaIndex);
gameFileInfo.lightDeltas.offset	=	-1;
gameFileInfo.lightDeltas.count	=	0;
gameFileInfo.lightDeltas.size		=	sizeof (CLightDelta);
}

// -----------------------------------------------------------------------------

static int32_t ReadGameFileInfo (CFile& cf, int32_t nStartOffset)
{
gameTopFileInfo.fileinfo_signature = cf.ReadShort ();
gameTopFileInfo.fileinfoVersion = cf.ReadShort ();
gameTopFileInfo.fileinfo_sizeof = cf.ReadInt ();
// Check signature
if (gameTopFileInfo.fileinfo_signature != 0x6705)
	return -1;
// Check version number
if (gameTopFileInfo.fileinfoVersion < GAME_COMPATIBLE_VERSION)
	return -1;
// Now, Read in the fileinfo
if (cf.Seek (nStartOffset, SEEK_SET)) 
	Error ("Error seeking to gameFileInfo in gamesave.c");
gameFileInfo.fileinfo_signature = cf.ReadShort ();
gameFileInfo.fileinfoVersion = cf.ReadShort ();
gameFileInfo.fileinfo_sizeof = cf.ReadInt ();
cf.Read (gameFileInfo.mine_filename, sizeof (char), 15);
gameFileInfo.level = cf.ReadInt ();
gameFileInfo.player.offset = cf.ReadInt ();				// Player info
gameFileInfo.player.size = cf.ReadInt ();
gameFileInfo.objects.Read (cf);
gameFileInfo.walls.Read (cf);
gameFileInfo.doors.Read (cf);
gameFileInfo.triggers.Read (cf);
gameFileInfo.links.Read (cf);
gameFileInfo.control.Read (cf);
gameFileInfo.botGen.Read (cf);
if (gameTopFileInfo.fileinfoVersion >= 29) {
	gameFileInfo.lightDeltaIndices.Read (cf);
	gameFileInfo.lightDeltas.Read (cf);
	}
if (gameStates.app.bD2XLevel && (gameData.segData.nLevelVersion > 15))
	gameFileInfo.equipGen.Read (cf);
return 0;
}

// -----------------------------------------------------------------------------

static int32_t ReadLevelInfo (CFile& cf)
{
if (gameTopFileInfo.fileinfoVersion >= 31) { //load mine filename
	// read newline-terminated string, not sure what version this changed.
	for (int32_t i = 0; i < (int32_t) sizeof (missionManager.szCurrentLevel); i++) {
		missionManager.szCurrentLevel [i] = (char) cf.ReadByte ();
		if (missionManager.szCurrentLevel [i] == '\n')
			missionManager.szCurrentLevel [i] = '\0';
		if (missionManager.szCurrentLevel [i] == '\0')
			break;
		}
	}
else if (gameTopFileInfo.fileinfoVersion >= 14) { //load mine filename
	// read null-terminated string
	char *p = missionManager.szCurrentLevel;
	//must do read one char at a time, since no cf.GetS()
	do {
		*p = (char) cf.ReadByte ();
		} while (*p++);
}
else
	missionManager.szCurrentLevel [0] = 0;
if (gameTopFileInfo.fileinfoVersion >= 19) {	//load pof names
	nSavePOFNames = cf.ReadShort ();
	if ((nSavePOFNames != 0x614d) && (nSavePOFNames != 0x5547)) { // "Ma"de w/DMB beta/"GU"ILE
		if (nSavePOFNames >= MAX_POLYGON_MODELS)
			return -1;
		cf.Read (szSavePOFNames, nSavePOFNames, SHORT_FILENAME_LEN);
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static int32_t ReadObjectInfo (CFile& cf)
{
	int32_t	i;

if ((gameFileInfo.objects.offset > -1) && gameFileInfo.objects.count) {
	CObject	*pObj = OBJECTS.Buffer ();
	if (cf.Seek (gameFileInfo.objects.offset, SEEK_SET)) {
		Error ("Error seeking to object data\n(file damaged or invalid)");
		return -1;
		}
	OBJECTS.Clear (0, gameFileInfo.objects.count);
	gameData.objData.nObjects = 0;				//just the player
	i = 0;
	while (i < gameFileInfo.objects.count) {
#if DBG
		if (i == nDbgObj)
			BRP;
#endif
		ClaimObjectSlot (i);
		pObj->Read (cf);
#if OBJ_LIST_TYPE == 1
		pObj->ResetLinks ();
#endif
		if (!IsMultiGame && pObj->Multiplayer ())
			gameFileInfo.objects.count--;
		else {
			pObj->info.nSignature = gameData.objData.nNextSignature++;
#if DBG
			if (i == nDbgObj) {
				extern int32_t dbgObjInstances;
				dbgObjInstances++;
				}
#endif
			pObj->Verify ();
			gameData.objData.init [i] = *pObj;
			i++, pObj++;
			}
		}
	}
gameData.objData.GatherEffects ();
for (i = 0; i < LEVEL_OBJECTS - 1; i++)
	gameData.objData.dropInfo [i].nNextPowerup = i + 1;
gameData.objData.dropInfo [i].nNextPowerup = -1;
gameData.objData.nFirstDropped =
gameData.objData.nLastDropped = -1;
gameData.objData.nFreeDropped = 0;
return 0;
}

// -----------------------------------------------------------------------------

static int32_t ReadWallInfo (CFile& cf)
{
if (gameFileInfo.walls.count && (gameFileInfo.walls.offset > -1)) {
	int32_t	i;

	if (!gameData.wallData.walls.Resize (gameFileInfo.walls.count)) {
		Error ("Not enough memory for wall data\n");
		return -1;
		}
	if (cf.Seek (gameFileInfo.walls.offset, SEEK_SET)) {
		Error ("Error seeking to wall data\n(file damaged or invalid)");
		return -1;
		}
	for (i = 0; i < gameFileInfo.walls.count; i++) {
		if (gameTopFileInfo.fileinfoVersion >= 20)
			gameData.wallData.walls [i].Read (cf); // v20 walls and up.
		else if (gameTopFileInfo.fileinfoVersion >= 17) {
			tWallV19 w;

			ReadWallV19(w, cf);
			WALL (i)->nSegment	   = w.nSegment;
			WALL (i)->nSide			= w.nSide;
			WALL (i)->nLinkedWall	= w.nLinkedWall;
			WALL (i)->nType			= w.nType;
			WALL (i)->flags			= w.flags;
			WALL (i)->hps				= w.hps;
			WALL (i)->nTrigger		= w.nTrigger;
			WALL (i)->nClip			= w.nClip;
			WALL (i)->keys				= w.keys;
			WALL (i)->state			= WALL_DOOR_CLOSED;
			}
		else {
			tWallV16 w;

			ReadWallV16(w, cf);
			WALL (i)->nSegment = WALL (i)->nSide = WALL (i)->nLinkedWall = -1;
			WALL (i)->nType		= w.nType;
			WALL (i)->flags		= w.flags;
			WALL (i)->hps			= w.hps;
			WALL (i)->nTrigger	= w.nTrigger;
			WALL (i)->nClip		= w.nClip;
			WALL (i)->keys			= w.keys;
			}
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static int32_t ReadDoorInfo (CFile& cf)
{
if (gameFileInfo.doors.offset > -1) {
	int32_t	i;
	if (cf.Seek (gameFileInfo.doors.offset, SEEK_SET)) {
		Error ("Error seeking to door data\n(file damaged or invalid)");
		return -1;
		}
	for (i = 0; i < gameFileInfo.doors.count; i++) {
		if (gameTopFileInfo.fileinfoVersion >= 20)
			ReadActiveDoor (gameData.wallData.activeDoors [i], cf); // version 20 and up
		else {
			v19_door d;
			int16_t nConnSeg, nConnSide;
			CSegment* pSeg;

			ReadActiveDoorV19 (d, cf);
			gameData.wallData.activeDoors [i].nPartCount = d.nPartCount;
			for (int32_t j = 0; j < d.nPartCount; j++) {
				pSeg = SEGMENT (d.seg [j]);
				nConnSeg = pSeg->m_children [d.nSide [j]];
				nConnSide = pSeg->ConnectedSide (SEGMENT (nConnSeg));
				gameData.wallData.activeDoors [i].nFrontWall [j] = pSeg->WallNum (d.nSide [j]);
				gameData.wallData.activeDoors [i].nBackWall [j] = SEGMENT (nConnSeg)->WallNum (nConnSide);
				}
			}
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static void CheckTriggerInfo (void)
{
	int32_t		h, i, j;
	CTrigger	*pTrigger;

for (i = 0, pTrigger = GEOTRIGGERS.Buffer (); i < gameFileInfo.triggers.count; i++, pTrigger++) {
	if (pTrigger->m_nLinks < 0)
		pTrigger->m_nLinks = 0;
	else if (pTrigger->m_nLinks > MAX_TRIGGER_TARGETS)
		pTrigger->m_nLinks = MAX_TRIGGER_TARGETS;
	for (h = pTrigger->m_nLinks, j = 0; j < h; ) {
		if ((pTrigger->m_segments [j] >= 0) && 
			 (pTrigger->m_sides [j] >= 0) 
			 ? (pTrigger->m_segments [j] < gameData.segData.nSegments) && (pTrigger->m_sides [j] < 6) 
			 : (pTrigger->m_segments [j] < gameData.objData.nObjects) && (pTrigger->m_sides [j] == -1))
			j++;
		else if (--h) {
			pTrigger->m_segments [j] = pTrigger->m_segments [h];
			pTrigger->m_sides [j] = pTrigger->m_sides [h];
			}
		}
	pTrigger->m_nLinks = h;
	}
}

// -----------------------------------------------------------------------------

int32_t CmpObjTriggers (const CTrigger* pv, const CTrigger* pm)
{
return (pv->m_info.nObject < pm->m_info.nObject) ? -1 : (pv->m_info.nObject > pm->m_info.nObject) ? 1 : 
		 (pv->m_info.nType < pm->m_info.nType) ? -1 : (pv->m_info.nType > pm->m_info.nType) ? 1 : 0;
}

// -----------------------------------------------------------------------------

void BuildObjTriggerRef (void)
{
if (gameData.trigData.m_nTriggers [1]) {
	CQuickSort<CTrigger>	qs;
	qs.SortAscending (OBJTRIGGERS.Buffer (), 0, gameData.trigData.m_nTriggers [1] - 1, &CmpObjTriggers);
	CTrigger* pTrigger = OBJTRIGGERS.Buffer ();
	int32_t i;
	int16_t nObject = -1;
	for (i = 0; i < gameData.trigData.m_nTriggers [1]; i++, pTrigger++) {
		if (nObject != pTrigger->m_info.nObject) {
			if (nObject >= 0)
				gameData.trigData.objTriggerRefs [nObject].nCount = i - gameData.trigData.objTriggerRefs [nObject].nFirst;
			nObject = pTrigger->m_info.nObject;
			if (nObject >= 0)
				gameData.trigData.objTriggerRefs [nObject].nFirst = i;
			}
		}
	if (nObject >= 0)
		gameData.trigData.objTriggerRefs [nObject].nCount = i - gameData.trigData.objTriggerRefs [nObject].nFirst;
	}
}

// -----------------------------------------------------------------------------

static int32_t ReadTriggerInfo (CFile& cf)
{
	int32_t		i;
	CTrigger	*pTrigger;

gameData.trigData.m_nTriggers [1] = 0;
gameData.trigData.objTriggerRefs.Clear (0xff);
if (gameFileInfo.triggers.offset > -1) {
	if (gameFileInfo.triggers.count) {
#if TRACE
		console.printf(CON_DBG, "   loading CTrigger data ...\n");
#endif
		if (!gameData.trigData.Create (gameFileInfo.triggers.count, false)) {
			Error ("Not enough memory for trigger data");
			return -1;
			}
		if (cf.Seek (gameFileInfo.triggers.offset, SEEK_SET)) {
			Error ("Error seeking to trigger data\n(file damaged or invalid)");
			return -1;
			}
		for (i = 0, pTrigger = GEOTRIGGERS.Buffer (); i < gameFileInfo.triggers.count; i++, pTrigger++) {
			pTrigger->m_info.flagsD1 = 0;
			if (gameTopFileInfo.fileinfoVersion >= 31) 
				pTrigger->Read (cf, 0);
			else {
				tTriggerV30 trig;
				int32_t t, nType = 0, flags = 0;
				if (gameTopFileInfo.fileinfoVersion == 30)
					V30TriggerRead (trig, cf);
				else {
					tTriggerV29 trig29;
					V29TriggerRead (trig29, cf);
					pTrigger->m_info.flagsD1 = trig29.flags;
					trig.flags = 0;
					trig.nLinks	= (char) trig29.nLinks;
					trig.value = trig29.value;
					trig.time = trig29.time;
					for (t = 0; t < trig.nLinks; t++) {
						trig.segments [t] = trig29.segments [t];
						trig.sides [t] = trig29.sides [t];
						}
					}
				if (gameStates.app.bD1Mission)
					nType = TT_DESCENT1;
				else if (trig.flags & TRIGGER_UNLOCK_DOORS)
					nType = TT_UNLOCK_DOOR;
				else if (trig.flags & TRIGGER_OPEN_WALL)
					nType = TT_OPEN_WALL;
				else if (trig.flags & TRIGGER_CLOSE_WALL)
					nType = TT_CLOSE_WALL;
				else if (trig.flags & TRIGGER_ILLUSORY_WALL)
					nType = TT_ILLUSORY_WALL;
				else
					Int3();
				if (trig.flags & TRIGGER_ONE_SHOT)
					flags = TF_ONE_SHOT;

				pTrigger->m_info.nType = nType;
				pTrigger->m_info.flags = flags;
				pTrigger->m_nLinks = trig.nLinks;
				pTrigger->m_nLinks = trig.nLinks;
				pTrigger->m_info.value = trig.value;
				pTrigger->m_info.time [0] = trig.time;
				for (t = 0; t < trig.nLinks; t++) {
					pTrigger->m_segments [t] = trig.segments [t];
					pTrigger->m_sides [t] = trig.sides [t];
					}
				}
			}
		}

	if (gameTopFileInfo.fileinfoVersion >= 33) {
		gameData.trigData.m_nTriggers [1] = cf.ReadInt ();
		if (gameData.trigData.m_nTriggers [1]) {
			if (!gameData.trigData.Create (gameData.trigData.m_nTriggers [1], true)) {
				Error ("Not enough memory for object trigger data");
				return -1;
				}
			for (i = 0; i < gameData.trigData.m_nTriggers [1]; i++)
				OBJTRIGGERS [i].Read (cf, 1);
			if (gameTopFileInfo.fileinfoVersion >= 40) {
				for (i = 0; i < gameData.trigData.m_nTriggers [1]; i++)
					OBJTRIGGERS [i].m_info.nObject = cf.ReadShort ();
				}
			else {
				for (i = 0; i < gameData.trigData.m_nTriggers [1]; i++) {
					cf.Seek (2 * sizeof (int16_t), SEEK_CUR);
					OBJTRIGGERS [i].m_info.nObject = cf.ReadShort ();
					}
				if (gameTopFileInfo.fileinfoVersion < 36) 
					cf.Seek (700 * sizeof (int16_t), SEEK_CUR);
				else 
					cf.Seek (cf.ReadShort () * 2 * sizeof (int16_t), SEEK_CUR);
				}
			}

		BuildObjTriggerRef ();
		}
	else {
		OBJTRIGGERS.Clear ();
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static int32_t ReadReactorInfo (CFile& cf)
{
if (gameFileInfo.control.offset > -1) {
#if TRACE
	console.printf(CON_DBG, "   loading reactor data ...\n");
#endif
	if (cf.Seek (gameFileInfo.control.offset, SEEK_SET)) {
		Error ("Error seeking to reactor data\n(file damaged or invalid)");
		return -1;
		}
	ReadReactorTriggers (cf);
	}
return 0;
}

// -----------------------------------------------------------------------------
// Because the dumb amateurs from Parallax have been incapable of creating a consistent
// and simple robot center to segment relationship, the following function assigns
// object producers to segments in the sequence the producers are stored on disk regardless 
// of what (in Parallax levels usually inconsistent and partially wrong) segment numbers
// have been stored in them.

static int32_t AssignProducer (tObjectProducerInfo& producerInfo, int32_t nObjProducer, int32_t nFunction, int8_t bFlag)
{
#if 1

	CSegment* pSeg = SEGMENT (0);

for (int32_t i = 0, j = gameData.segData.nSegments; i < j; i++, pSeg++) {
	if ((pSeg->Function () == nFunction) && (pSeg->m_nObjProducer == nObjProducer)) {
		tObjectProducerInfo& objProducer = (nFunction == SEGMENT_FUNC_ROBOTMAKER) 
													  ? gameData.producers.robotMakers [nObjProducer] 
													  : gameData.producers.equipmentMakers [nObjProducer];
		objProducer.nSegment = i;
		objProducer.nProducer = pSeg->m_value;
		memcpy (objProducer.objFlags, producerInfo.objFlags, sizeof (objProducer.objFlags));
		tProducerInfo& producer = gameData.producers.producers [pSeg->m_value];
		producer.bFlag &= (pSeg->Function () == SEGMENT_FUNC_ROBOTMAKER) ? ~1 : (pSeg->Function () == SEGMENT_FUNC_EQUIPMAKER) ? ~2 : ~0;
		return pSeg->m_nObjProducer;
		}
	}
return -1;

#elif 0

CSegment* pSeg = SEGMENT (producerInfo.nSegment);
if (pSeg->m_function != nFunction)
	return -1;
producerInfo.nProducer = pSeg->m_nObjProducer;
tObjectProducerInfo& objProducer = (nFunction == SEGMENT_FUNC_ROBOTMAKER) 
											  ? gameData.producers.robotMakers [producerInfo.nProducer] 
											  : gameData.producers.equipmentMakers [producerInfo.nProducer];
if (objProducer.bAssigned)
	return -1;
int32_t nProducer = objProducer.nProducer;
tProducerInfo& producer = gameData.producers.producers [nProducer];
if (!(producer.bFlag & bFlag)) // this segment already has an object producer assigned
	return -1;
memcpy (objProducer.objFlags, producerInfo.objFlags, sizeof (objProducer.objFlags));
objProducer.bAssigned = true;
producer.bFlag = 0;
return pSeg->m_nObjProducer = producerInfo.nProducer;

#else

if (producerInfo.nProducer < 0) 
	return -1;
if (producerInfo.nProducer >= ((nFunction == SEGMENT_FUNC_ROBOTMAKER) ? gameData.producers.nRobotMakers : gameData.producers.nEquipmentMakers))
	return -1;

tObjectProducerInfo& objProducer = (nFunction == SEGMENT_FUNC_ROBOTMAKER) 
											  ? gameData.producers.robotMakers [producerInfo.nProducer] 
											  : gameData.producers.equipmentMakers [producerInfo.nProducer];
if (objProducer.bAssigned)
	return -1;
int32_t nProducer = objProducer.nProducer;
tProducerInfo& producer = gameData.producers.producers [nProducer];
if (producer.nSegment < 0)
	return -1;
CSegment* pSeg = SEGMENT (producer.nSegment);
if (pSeg->m_value != nProducer)
	return -1;
if (pSeg->m_function != nFunction) // this object producer has an invalid segment
	return -1;
if (!(producer.bFlag & bFlag)) // this segment already has an object producer assigned
	return -1;
memcpy (objProducer.objFlags, producerInfo.objFlags, sizeof (objProducer.objFlags));
objProducer.bAssigned = true;
producer.bFlag = 0;
return pSeg->m_nObjProducer = producerInfo.nProducer;

#endif
}

// -----------------------------------------------------------------------------

static int32_t ReadBotGenInfo (CFile& cf)
{
if (gameFileInfo.botGen.offset > -1) {
	if (cf.Seek (gameFileInfo.botGen.offset, SEEK_SET)) {
		Error ("Error seeking to robot generator data\n(file damaged or invalid)");
		return -1;
		}
	tObjectProducerInfo m;
	m.objFlags [2] = gameData.objData.nVertigoBotFlags;
	for (int32_t i = 0; i < gameFileInfo.botGen.count; ) {
		ReadObjectProducerInfo (&m, cf, gameTopFileInfo.fileinfoVersion < 27);
		int32_t h = AssignProducer (m, i, SEGMENT_FUNC_ROBOTMAKER, 1);
		if (0 <= h)
			++i;
		else {
#if DBG
			PrintLog (0, "Invalid robot generator data found\n");
#endif
			--gameData.producers.nRobotMakers;
			--gameFileInfo.botGen.count;
			}
		}
	}

return 0;
}

// -----------------------------------------------------------------------------

static int32_t ReadEquipGenInfo (CFile& cf)
{
if (gameFileInfo.equipGen.offset > -1) {
	if (cf.Seek (gameFileInfo.equipGen.offset, SEEK_SET)) {
		Error ("Error seeking to equipment generator data\n(file damaged or invalid)");
		return -1;
		}
	tObjectProducerInfo m;
	m.objFlags [2] = 0;
	for (int32_t i = 0; i < gameFileInfo.equipGen.count;) {
		ReadObjectProducerInfo (&m, cf, false);
		int32_t h = AssignProducer (m, i, SEGMENT_FUNC_EQUIPMAKER, 2);
		if (0 <= h)
			++i;
		else {
#if DBG
			PrintLog (0, "Invalid equipment generator data found\n");
#endif
			--gameData.producers.nEquipmentMakers;
			--gameFileInfo.equipGen.count;
			}
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

void CleanupProducerInfo (void)
{
CSegment* pSeg = SEGMENTS.Buffer ();
for (int32_t i = gameData.segData.nSegments; i; i--, pSeg++) {
	if (pSeg->m_function == SEGMENT_FUNC_ROBOTMAKER) {
		tProducerInfo& producer = gameData.producers.producers [pSeg->m_value];
		if (producer.bFlag) {
			producer.bFlag = 0;
			pSeg->m_function = SEGMENT_FUNC_NONE;
			}
		}
	}
}

// -----------------------------------------------------------------------------

static int32_t ReadLightDeltaIndexInfo (CFile& cf)
{
gameData.render.lights.nStatic = 0;
if ((gameFileInfo.lightDeltaIndices.offset > -1) && gameFileInfo.lightDeltaIndices.count) {
	int32_t	i;

	if (!gameData.render.lights.deltaIndices.Resize (gameFileInfo.lightDeltaIndices.count)) {
		Error ("Not enough memory for light delta index data");
		return -1;
		}
	if (cf.Seek (gameFileInfo.lightDeltaIndices.offset, SEEK_SET)) {
		Error ("Error seeking to light delta index data\n(file damaged or invalid)");
		return -1;
		}
	gameData.render.lights.nStatic = gameFileInfo.lightDeltaIndices.count;
	if (gameTopFileInfo.fileinfoVersion < 29) {
#if TRACE
		console.printf (CON_DBG, "Warning: Old mine version.  Not reading gameData.render.lights.deltaIndices info.\n");
#endif
		Int3();	//shouldn't be here!!!
		return 0;
		}
	else {
		for (i = 0; i < gameFileInfo.lightDeltaIndices.count; i++) {
			gameData.render.lights.deltaIndices [i].Read (cf);
			}
		}
	gameData.render.lights.deltaIndices.SortAscending ();
	}
return 0;
}

// -----------------------------------------------------------------------------

static int32_t ReadLightDeltaInfo (CFile& cf)
{
if ((gameFileInfo.lightDeltas.offset > -1) && gameFileInfo.lightDeltas.count) {
	int32_t	h, i;

#if TRACE
	console.printf(CON_DBG, "   loading light data ...\n");
#endif
	if (!gameData.render.lights.deltas.Resize (gameFileInfo.lightDeltas.count)) {
		Error ("Not enough memory for light delta data");
		return -1;
		}
	if (cf.Seek (gameFileInfo.lightDeltas.offset, SEEK_SET)) {
		Error ("Error seeking to light delta data\n(file damaged or invalid)");
		return -1;
		}
	for (h = i = 0; i < gameFileInfo.lightDeltas.count; i++) {
		if (gameTopFileInfo.fileinfoVersion >= 29) {
			gameData.render.lights.deltas [i].Read (cf);
			h += gameData.render.lights.deltas [i].bValid;
			}
		else {
#if TRACE
			console.printf (CON_DBG, "Warning: Old mine version.  Not reading delta light info.\n");
#endif
			}
		}
	if (h < gameFileInfo.lightDeltas.count)
		PrintLog (0, "Invalid light delta data found (%d records affected)\n", gameFileInfo.lightDeltas.count - h);
	}
return 0;
}

// -----------------------------------------------------------------------------

static int32_t ReadVariableLights (CFile& cf)
{
	int32_t	nLights = cf.ReadInt ();

if (!nLights)
	return 0;
if (!gameData.render.lights.flicker.Create (nLights))
	return -1;
for (int32_t i = 0; i < nLights; i++)
	gameData.render.lights.flicker [i].Read (cf);
return nLights;
}

// -----------------------------------------------------------------------------

static void CheckAndLinkObjects (void)
{
	int32_t		i, nObjSeg;
	CObject	*pObj = OBJECTS.Buffer ();

for (i = 0; i < gameFileInfo.objects.count; i++, pObj++) {
	pObj->info.nNextInSeg = pObj->info.nPrevInSeg = -1;
	if (pObj->Type () != OBJ_NONE) {
		nObjSeg = pObj->info.nSegment;
		if ((nObjSeg < 0) || (nObjSeg > gameData.segData.nLastSegment))	
			pObj->SetType (OBJ_NONE);
		else {
			pObj->info.nSegment = -1;	
			OBJECT (i)->LinkToSeg (nObjSeg);
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Make sure non-transparent doors are set correctly.
static void CheckAndFixDoors (void)
{
	CSegment* pSeg = SEGMENTS.Buffer ();

for (int32_t i = 0; i < gameData.segData.nSegments; i++, pSeg++) {
	CSide* pSide = pSeg->m_sides;
	for (int32_t j = 0; j < SEGMENT_SIDE_COUNT; j++, pSide++) {
		CWall* pWall = pSide->Wall ();
		if (!pWall || (pWall->nType != WALL_DOOR))
			continue;
		else if ((pWall->nClip < 0) || (pWall->nClip >= int32_t (gameData.wallData.pAnim.Length ()))) 
			pWall->nClip = -1;
		else if (gameData.wallData.pAnim [pWall->nClip].flags & WCF_TMAP1) {
			pSide->m_nBaseTex = gameData.wallData.pAnim [pWall->nClip].frames [0];
			pSide->m_nOvlTex = 0;
			}
		}
	}
}

// -----------------------------------------------------------------------------
//go through all walls, killing references to invalid triggers
static void CheckAndFixWalls (void)
{
	int32_t		i;
	int16_t		nSegment, nSide;
	CWall*	pWall;

for (i = 0; i < gameData.wallData.nWalls; i++)
	if ((WALL (i)->nTrigger >= gameData.trigData.m_nTriggers [0]) && (WALL (i)->nTrigger != NO_TRIGGER)) {
#if TRACE
		PrintLog (0, "Removing reference to invalid trigger %d from wall %d\n", WALL (i)->nTrigger, i);
#endif
		WALL (i)->nTrigger = NO_TRIGGER;	//kill CTrigger
		}

if (gameTopFileInfo.fileinfoVersion < 17) {
	CSegment* pSeg = SEGMENTS.Buffer ();
	for (nSegment = 0; nSegment <= gameData.segData.nLastSegment; nSegment++, pSeg++) {
		for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
			if ((pWall = pSeg->Wall (nSide))) {
				pWall->nSegment = nSegment;
				pWall->nSide = nSide;
				}
			}
		}
	}

// mark all triggers on "open" segment sides (i.e. which are connected to an adjacent segment) as "fly through" 
// to simplify internal trigger management
for (i = 0; i < gameData.wallData.nWalls; i++) {
	CTrigger* pTrigger = GEOTRIGGER (WALL (i)->nTrigger);
	if (pTrigger)
		pTrigger->m_info.nWall = i;
	}
}

// -----------------------------------------------------------------------------
//go through all triggers, killing unused ones
static void CheckAndFixTriggers (void)
{
	int32_t	h, i, j;
	int16_t	nSegment, nSide, nWall;

for (i = 0; i < gameData.trigData.m_nTriggers [0]; ) {
	//	Find which CWall this CTrigger is connected to.
	for (j = 0; j < gameData.wallData.nWalls; j++)
		if (WALL (j)->nTrigger == i)
			break;
		i++;
	}

for (i = 0; i < gameData.wallData.nWalls; i++)
	WALL (i)->controllingTrigger = -1;

//	MK, 10/17/95: Make walls point back at the triggers that control them.
//	Go through all triggers, stuffing controllingTrigger field in WALLS.

CTrigger* pTrigger = GEOTRIGGERS.Buffer ();
for (i = 0; i < gameData.trigData.m_nTriggers [0]; i++, pTrigger++) {
	for (h = pTrigger->m_nLinks, j = 0; j < h; ) {
		if (pTrigger->m_sides [j] >= 0) {
			nSegment = pTrigger->m_segments [j];
			if (nSegment >= gameData.segData.nSegments) {
				if (j < --h) {
					pTrigger->m_segments [j] = pTrigger->m_segments [h];
					pTrigger->m_sides [j] = pTrigger->m_sides [h];
					}
				}
			else {
				nSide = pTrigger->m_sides [j];
				nWall = SEGMENT (nSegment)->WallNum (nSide);
				//check to see that if a CTrigger requires a CWall that it has one,
				//and if it requires a botGen that it has one
				if (pTrigger->m_info.nType == TT_OBJECT_PRODUCER) {
					if ((SEGMENT (nSegment)->m_function != SEGMENT_FUNC_ROBOTMAKER) && (SEGMENT (nSegment)->m_function != SEGMENT_FUNC_EQUIPMAKER)) {
						if (j < --h) {
							pTrigger->m_segments [j] = pTrigger->m_segments [h];
							pTrigger->m_sides [j] = pTrigger->m_sides [h];
							}
						continue;
						}
					}
				else if ((pTrigger->m_info.nType != TT_LIGHT_OFF) && (pTrigger->m_info.nType != TT_LIGHT_ON)) { //light triggers don't require walls
					if (IS_WALL (nWall))
						WALL (nWall)->controllingTrigger = i;
					else {
#if 0
						if (j < --h) {
							pTrigger->m_segments [j] = pTrigger->m_segments [h];
							pTrigger->m_sides [j] = pTrigger->m_sides [h];
							}
						continue;
#endif
						}
					}
				}
			}
		j++;
		}
	pTrigger->m_nLinks = h;
	}
}

//------------------------------------------------------------------------------

void CreateGenerators (void)
{
gameData.producers.nRepairCenters = 0;
for (int32_t i = 0; i < gameData.segData.nSegments; i++) {
	SEGMENT (i)->CreateGenerator (SEGMENT (i)->m_function);
	}
}

// -----------------------------------------------------------------------------
// Load game 
// Loads all the relevant data for a level.
// If level != -1, it loads the filename with extension changed to .min
// Otherwise it loads the appropriate level mine.
// returns 0=everything ok, 1=old version, -1=error
int32_t LoadMineDataCompiled (CFile& cf, int32_t bFileInfo)
{
	int32_t 	nStartOffset;

nStartOffset = (int32_t) cf.Tell ();
InitGameFileInfo ();
if (ReadGameFileInfo (cf, nStartOffset))
	return -1;
if (ReadLevelInfo (cf))
	return -1;
gameStates.render.bD2XLights = gameStates.app.bD2XLevel && (gameTopFileInfo.fileinfoVersion >= 34);
if (bFileInfo)
	return 0;

gameData.objData.nNextSignature = 0;
gameData.render.lights.nStatic = 0;
gameData.objData.nInitialRobots = 0;
nGameSavePlayers = 0;
if (ReadObjectInfo (cf))
	return -1;
if (ReadWallInfo (cf))
	return -1;
gameData.wallData.nWalls = gameFileInfo.walls.count;
if (ReadDoorInfo (cf))
	return -1;
if (ReadTriggerInfo (cf))
	return -1;
gameData.trigData.m_nTriggers [0] = gameFileInfo.triggers.count;
if (ReadReactorInfo (cf))
	return -1;
if (ReadBotGenInfo (cf))
	return -1;
if (ReadEquipGenInfo (cf))
	return -1;
CleanupProducerInfo ();
if (ReadLightDeltaIndexInfo (cf))
	return -1;
if (ReadLightDeltaInfo (cf))
	return -1;
ClearLightSubtracted ();
ResetObjects (gameFileInfo.objects.count);
CheckAndLinkObjects ();
ClearTransientObjects (1);		//1 means clear proximity bombs
CheckAndFixDoors ();
CheckTriggerInfo ();
//gameData.wallData.nOpenDoors = gameFileInfo.doors.count;
gameData.trigData.m_nTriggers [0] = gameFileInfo.triggers.count;
gameData.wallData.nWalls = gameFileInfo.walls.count;
CheckAndFixWalls ();
CheckAndFixTriggers ();
gameData.producers.nRobotMakers = gameFileInfo.botGen.count;
FixObjectSegs ();
if ((gameTopFileInfo.fileinfoVersion < GAME_VERSION) && 
	 ((gameTopFileInfo.fileinfoVersion != 25) || (GAME_VERSION != 26)))
	return 1;		//means old version
return 0;
}

// ----------------------------------------------------------------------------

void	SetAmbientSoundFlags (void);


#define LEVEL_FILE_VERSION      8
//1 -> 2  add palette name
//2 -> 3  add control center explosion time
//3 -> 4  add reactor strength
//4 -> 5  killed hostage text stuff
//5 -> 6  added gameData.segData.secret.nReturnSegment and gameData.segData.secret.returnOrient
//6 -> 7  added flickering lights
//7 -> 8  made version 8 to be not compatible with D2 1.0 & 1.1

// ----------------------------------------------------------------------------

class CVertSegRef {
public:
	int32_t	nIndex;
	int16_t	nSegments;
};

void CreateVertexSegmentList (void)
{
#if 0
CArray<int16_t> segCount;
segCount.Create (gameData.segData.nVertices);
segCount.Clear ();

CSegment* pSeg = SEGMENTS.Buffer ();
for (int32_t i = gameData.segData.nSegments; i; i--, pSeg++) {
	for (int32_t j = 0; j < 8; j++)
		if (pSeg->m_vertices [j] != 0xFFFF)
			segCount [pSeg->m_vertices [j]]++;
	}

CArray<CVertSegRef> vertSegIndex;
vertSegIndex.Create (gameData.segData.nVertices);
vertSegIndex.Clear ();
CArray<int16_t> vertSegs;
vertSegs.Create (8 * gameData.segData.nSegments);
vertSegs.Clear ();
for (int32_t h = 0, i = 0; i < gameData.segData.nSegments; i++) {
	vertSegIndex [i].nIndex = h;
	h += segCount [i];
	}

pSeg = SEGMENTS.Buffer ();
for (int32_t i = gameData.segData.nSegments; i; i--, pSeg++) {
	for (int32_t j = 0; j < 8; j++) {
		if (pSeg->m_vertices [j] != 0xFFFF) {
			CVertSegRef* pRef = &vertSegIndex [pSeg->m_vertices [j]];
			vertSegs [pRef->nIndex + pRef->nSegments++] = i;
			}
		}
	}
#endif
}

// ----------------------------------------------------------------------------
//loads a level (.LVL) file from disk
//returns 0 if success, else error code
int32_t LoadLevelData (char * pszFilename, int32_t nLevel)
{
	CFile cf;
	char	filename [128];
	int32_t	nMineDataOffset, nGameDataOffset;
	int32_t	nError;

/*---*/PrintLog (1, "loading level data\n");
SetDataVersion (-1);
gameData.segData.bHaveSlideSegs = 0;
if (IsNetworkGame) {
	gameData.multiplayer.maxPowerupsAllowed.Clear (0);
	gameData.multiplayer.powerupsInMine.Clear (0);
	}
gameStates.render.nMeshQuality = gameOpts->render.nMeshQuality;

for (;;) {
	strcpy (filename, pszFilename);
	if (!cf.Open (filename, "", "rb", gameStates.app.bD1Mission)) {
		PrintLog (-1);
		return 1;
		}

	strcpy(gameData.segData.szLevelFilename, filename);

	cf.ReadInt ();
	gameData.segData.nLevelVersion = cf.ReadInt ();
	gameStates.app.bD2XLevel = (gameData.segData.nLevelVersion >= 10);
	#if TRACE
	console.printf (CON_DBG, "gameData.segData.nLevelVersion = %d\n", gameData.segData.nLevelVersion);
	#endif
	nMineDataOffset = cf.ReadInt ();
	nGameDataOffset = cf.ReadInt ();

	if (gameData.segData.nLevelVersion >= 8) {    //read dummy data
		cf.ReadInt ();
		cf.ReadShort ();
		cf.ReadByte ();
	}

	if (gameData.segData.nLevelVersion < 5)
		cf.ReadInt ();       //was hostagetext_offset

	if (gameData.segData.nLevelVersion > 1) {
		for (int32_t i = 0; i < (int32_t) sizeof (szCurrentLevelPalette); i++) {
			szCurrentLevelPalette [i] = (char) cf.ReadByte ();
			if (szCurrentLevelPalette [i] == '\n')
				szCurrentLevelPalette [i] = '\0';
			if (szCurrentLevelPalette [i] == '\0')
				break;
			}
		}
	if ((gameData.segData.nLevelVersion <= 1) || (szCurrentLevelPalette [0] == 0)) // descent 1 level
		strcpy (szCurrentLevelPalette, /*gameStates.app.bD1Mission ? D1_PALETTE :*/ DEFAULT_LEVEL_PALETTE); //D1_PALETTE looks off in D2X-XL

	if (gameData.segData.nLevelVersion >= 3)
		gameStates.app.nBaseCtrlCenExplTime = cf.ReadInt ();
	else
		gameStates.app.nBaseCtrlCenExplTime = DEFAULT_CONTROL_CENTER_EXPLOSION_TIME;

	if (gameData.segData.nLevelVersion >= 4)
		gameData.reactor.nStrength = cf.ReadInt ();
	else
		gameData.reactor.nStrength = -1;  //use old defaults

	if (gameData.segData.nLevelVersion >= 7) {
	#if TRACE
		console.printf (CON_DBG, "   loading dynamic lights ...\n");
	#endif
		if (0 > ReadVariableLights (cf)) {
			cf.Close ();
			PrintLog (-1);
			return 5;
			}
		}	

	if (gameData.segData.nLevelVersion < 6) {
		gameData.segData.secret.nReturnSegment = 0;
		gameData.segData.secret.returnOrient = CFixMatrix::IDENTITY;
		}
	else {
		gameData.segData.secret.nReturnSegment = cf.ReadInt ();
		for (int32_t i = 0; i < 9; i++)
			gameData.segData.secret.returnOrient.m.vec [i] = cf.ReadInt ();
		}

	//NOTE LINK TO ABOVE!!
	cf.Seek (nGameDataOffset, SEEK_SET);
	nError = LoadMineDataCompiled (cf, 1);
	if (!nError) {
		cf.Seek (nMineDataOffset, SEEK_SET);
		nError = LoadMineSegmentsCompiled (cf);
		}
	if (nError == -1) {   //error!!
		cf.Close ();
		PrintLog (-1);
		return 2;
		}
	cf.Seek (nGameDataOffset, SEEK_SET);
	gameData.objData.lists.Init ();
	nError = LoadMineDataCompiled (cf, 0);
	if (nError == -1) {   //error!!
		cf.Close ();
		PrintLog (-1);
		return 3;
		}
	cf.Close ();
	networkData.nSegmentCheckSum = CalcSegmentCheckSum ();
	CreateVertexSegmentList ();
	/*---*/PrintLog (1, "building geometry mesh\n");
	if (meshBuilder.Build (nLevel)) {
		PrintLog (-1);
		break;
		}
	PrintLog (-1);
	if (gameStates.render.nMeshQuality <= 0) {
		PrintLog (-1);
		return 6;
		}
	gameStates.render.nMeshQuality--;
	}
gameStates.render.nMeshQuality = gameOpts->render.nMeshQuality;
	/*---*/PrintLog (1, "allocating render buffers\n");
if (!gameData.render.mine.Create (1)) {
	PrintLog (-1);
	return 4;
	}
PrintLog (-1);
if (gameStates.render.CartoonStyle ())
	gameData.segData.BuildEdgeList ();
//lightManager.Setup (nLevel); 
SetAmbientSoundFlags ();
PrintLog (-1);
return 0;
}

// ----------------------------------------------------------------------------
//eof

