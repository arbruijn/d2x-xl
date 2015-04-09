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
void ReadObject(CObject *objP, CFile *f, int32_t version);
#if DBG
void dump_mine_info(void);
#endif

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
	++gameData.objs.nInitialRobots;
	// Make sure valid id...
	if (Id () >= gameData.bots.nTypes [gameStates.app.bD1Data])
		SetId (Id () % gameData.bots.nTypes [0]);
	// Make sure model number & size are correct...
	if (info.renderType == RT_POLYOBJ) {
		rType.polyObjInfo.nModel = ROBOTINFO (Id ())->nModel;
		AdjustSize ();
		}
	if (Id () == 65)						//special "reactor" robots
		info.movementType = MT_NONE;
	if (info.movementType == MT_PHYSICS) {
		mType.physInfo.mass = ROBOTINFO (Id ())->mass;
		mType.physInfo.drag = ROBOTINFO (Id ())->drag;
		}
	}
else if (Type () == OBJ_EFFECT) {
	gameData.objs.nEffects++;
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
	if (Id () >= gameData.objs.pwrUp.nTypes + POWERUP_ADDON_COUNT) {
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
	if (gameData.segs.nLevelVersion <= 1) { // descent 1 reactor
		SetId (0);                         // used to be only one kind of reactor
		rType.polyObjInfo.nModel = gameData.reactor.props [0].nModel;// descent 1 reactor
		}
	}
else if (Type () == OBJ_PLAYER) {
	if (this == gameData.objs.consoleP)	
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

//------------------------------------------------------------------------------

int32_t MultiPowerupIs4Pack(int32_t);
//reads one CObject of the given version from the given file
extern int32_t RemoveTriggerNum (int32_t trigger_num);

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
if (gameStates.app.bD2XLevel && (gameData.segs.nLevelVersion > 15))
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
	CObject	*objP = OBJECTS.Buffer ();
	if (cf.Seek (gameFileInfo.objects.offset, SEEK_SET)) {
		Error ("Error seeking to object data\n(file damaged or invalid)");
		return -1;
		}
	OBJECTS.Clear (0, gameFileInfo.objects.count);
	gameData.objs.nObjects = 0;				//just the player
	i = 0;
	while (i < gameFileInfo.objects.count) {
#if DBG
		if (i == nDbgObj)
			BRP;
#endif
		ClaimObjectSlot (i);
		objP->Read (cf);
#if OBJ_LIST_TYPE == 1
		objP->ResetLinks ();
#endif
		if (!IsMultiGame && objP->Multiplayer ())
			gameFileInfo.objects.count--;
		else {
			objP->info.nSignature = gameData.objs.nNextSignature++;
#if DBG
			if (i == nDbgObj) {
				extern int32_t dbgObjInstances;
				dbgObjInstances++;
				}
#endif
			objP->Verify ();
			gameData.objs.init [i] = *objP;
			i++, objP++;
			}
		}
	}
gameData.objs.GatherEffects ();
for (i = 0; i < LEVEL_OBJECTS - 1; i++)
	gameData.objs.dropInfo [i].nNextPowerup = i + 1;
gameData.objs.dropInfo [i].nNextPowerup = -1;
gameData.objs.nFirstDropped =
gameData.objs.nLastDropped = -1;
gameData.objs.nFreeDropped = 0;
return 0;
}

// -----------------------------------------------------------------------------

static int32_t ReadWallInfo (CFile& cf)
{
if (gameFileInfo.walls.count && (gameFileInfo.walls.offset > -1)) {
	int32_t	i;

	if (!gameData.walls.walls.Resize (gameFileInfo.walls.count)) {
		Error ("Not enough memory for wall data\n");
		return -1;
		}
	if (cf.Seek (gameFileInfo.walls.offset, SEEK_SET)) {
		Error ("Error seeking to wall data\n(file damaged or invalid)");
		return -1;
		}
	for (i = 0; i < gameFileInfo.walls.count; i++) {
		if (gameTopFileInfo.fileinfoVersion >= 20)
			WALLEX (i, GAMEDATA_ERRLOG_BUFFER)->Read (cf); // v20 walls and up.
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
			ReadActiveDoor (gameData.walls.activeDoors [i], cf); // version 20 and up
		else {
			v19_door d;
			int16_t nConnSeg, nConnSide;
			CSegment* segP;

			ReadActiveDoorV19 (d, cf);
			gameData.walls.activeDoors [i].nPartCount = d.nPartCount;
			for (int32_t j = 0; j < d.nPartCount; j++) {
				segP = SEGMENT (d.seg [j]);
				nConnSeg = segP->m_children [d.nSide [j]];
				nConnSide = segP->ConnectedSide (SEGMENT (nConnSeg));
				gameData.walls.activeDoors [i].nFrontWall [j] = segP->WallNum (d.nSide [j]);
				gameData.walls.activeDoors [i].nBackWall [j] = SEGMENT (nConnSeg)->WallNum (nConnSide);
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
	CTrigger	*trigP;

for (i = 0, trigP = TRIGGERS.Buffer (); i < gameFileInfo.triggers.count; i++, trigP++) {
	if (trigP->m_nLinks < 0)
		trigP->m_nLinks = 0;
	else if (trigP->m_nLinks > MAX_TRIGGER_TARGETS)
		trigP->m_nLinks = MAX_TRIGGER_TARGETS;
	for (h = trigP->m_nLinks, j = 0; j < h; ) {
		if ((trigP->m_segments [j] >= 0) && 
			 (trigP->m_sides [j] >= 0) 
			 ? (trigP->m_segments [j] < gameData.segs.nSegments) && (trigP->m_sides [j] < 6) 
			 : (trigP->m_segments [j] < gameData.objs.nObjects) && (trigP->m_sides [j] == -1))
			j++;
		else if (--h) {
			trigP->m_segments [j] = trigP->m_segments [h];
			trigP->m_sides [j] = trigP->m_sides [h];
			}
		}
	trigP->m_nLinks = h;
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
if (gameData.trigs.m_nObjTriggers) {
	CQuickSort<CTrigger>	qs;
	qs.SortAscending (OBJTRIGGERS.Buffer (), 0, gameData.trigs.m_nObjTriggers - 1, &CmpObjTriggers);
	CTrigger* trigP = OBJTRIGGERS.Buffer ();
	int32_t i;
	int16_t nObject = -1;
	for (i = 0; i < gameData.trigs.m_nObjTriggers; i++, trigP++) {
		if (nObject != trigP->m_info.nObject) {
			if (nObject >= 0)
				gameData.trigs.objTriggerRefs [nObject].nCount = i - gameData.trigs.objTriggerRefs [nObject].nFirst;
			nObject = trigP->m_info.nObject;
			if (nObject >= 0)
				gameData.trigs.objTriggerRefs [nObject].nFirst = i;
			}
		}
	if (nObject >= 0)
		gameData.trigs.objTriggerRefs [nObject].nCount = i - gameData.trigs.objTriggerRefs [nObject].nFirst;
	}
}

// -----------------------------------------------------------------------------

static int32_t ReadTriggerInfo (CFile& cf)
{
	int32_t		i;
	CTrigger	*trigP;

gameData.trigs.m_nObjTriggers = 0;
gameData.trigs.objTriggerRefs.Clear (0xff);
if (gameFileInfo.triggers.offset > -1) {
	if (gameFileInfo.triggers.count) {
#if TRACE
		console.printf(CON_DBG, "   loading CTrigger data ...\n");
#endif
		if (!gameData.trigs.Create (gameFileInfo.triggers.count, false)) {
			Error ("Not enough memory for trigger data");
			return -1;
			}
		if (cf.Seek (gameFileInfo.triggers.offset, SEEK_SET)) {
			Error ("Error seeking to trigger data\n(file damaged or invalid)");
			return -1;
			}
		for (i = 0, trigP = TRIGGERS.Buffer (); i < gameFileInfo.triggers.count; i++, trigP++) {
			trigP->m_info.flagsD1 = 0;
			if (gameTopFileInfo.fileinfoVersion >= 31) 
				trigP->Read (cf, 0);
			else {
				tTriggerV30 trig;
				int32_t t, nType = 0, flags = 0;
				if (gameTopFileInfo.fileinfoVersion == 30)
					V30TriggerRead (trig, cf);
				else {
					tTriggerV29 trig29;
					V29TriggerRead (trig29, cf);
					trigP->m_info.flagsD1 = trig29.flags;
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

				trigP->m_info.nType = nType;
				trigP->m_info.flags = flags;
				trigP->m_nLinks = trig.nLinks;
				trigP->m_nLinks = trig.nLinks;
				trigP->m_info.value = trig.value;
				trigP->m_info.time [0] = trig.time;
				for (t = 0; t < trig.nLinks; t++) {
					trigP->m_segments [t] = trig.segments [t];
					trigP->m_sides [t] = trig.sides [t];
					}
				}
			}
		}

	if (gameTopFileInfo.fileinfoVersion >= 33) {
		gameData.trigs.m_nObjTriggers = cf.ReadInt ();
		if (gameData.trigs.m_nObjTriggers) {
			if (!gameData.trigs.Create (gameData.trigs.m_nObjTriggers, true)) {
				Error ("Not enough memory for object trigger data");
				return -1;
				}
			for (i = 0; i < gameData.trigs.m_nObjTriggers; i++)
				OBJTRIGGERS [i].Read (cf, 1);
			if (gameTopFileInfo.fileinfoVersion >= 40) {
				for (i = 0; i < gameData.trigs.m_nObjTriggers; i++)
					OBJTRIGGERS [i].m_info.nObject = cf.ReadShort ();
				}
			else {
				for (i = 0; i < gameData.trigs.m_nObjTriggers; i++) {
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

	CSegment* segP = SEGMENT (0);

for (int32_t i = 0, j = gameData.segs.nSegments; i < j; i++, segP++) {
	if ((segP->Function () == nFunction) && (segP->m_nObjProducer == nObjProducer)) {
		tObjectProducerInfo& objProducer = (nFunction == SEGMENT_FUNC_ROBOTMAKER) 
													  ? gameData.producers.robotMakers [nObjProducer] 
													  : gameData.producers.equipmentMakers [nObjProducer];
		objProducer.nSegment = i;
		objProducer.nProducer = segP->m_value;
		memcpy (objProducer.objFlags, producerInfo.objFlags, sizeof (objProducer.objFlags));
		tProducerInfo& producer = gameData.producers.producers [segP->m_value];
		producer.bFlag &= (segP->Function () == SEGMENT_FUNC_ROBOTMAKER) ? ~1 : (segP->Function () == SEGMENT_FUNC_EQUIPMAKER) ? ~2 : ~0;
		return segP->m_nObjProducer;
		}
	}
return -1;

#elif 0

CSegment* segP = SEGMENT (producerInfo.nSegment);
if (segP->m_function != nFunction)
	return -1;
producerInfo.nProducer = segP->m_nObjProducer;
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
return segP->m_nObjProducer = producerInfo.nProducer;

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
CSegment* segP = SEGMENT (producer.nSegment);
if (segP->m_value != nProducer)
	return -1;
if (segP->m_function != nFunction) // this object producer has an invalid segment
	return -1;
if (!(producer.bFlag & bFlag)) // this segment already has an object producer assigned
	return -1;
memcpy (objProducer.objFlags, producerInfo.objFlags, sizeof (objProducer.objFlags));
objProducer.bAssigned = true;
producer.bFlag = 0;
return segP->m_nObjProducer = producerInfo.nProducer;

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
	m.objFlags [2] = gameData.objs.nVertigoBotFlags;
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
CSegment* segP = SEGMENTS.Buffer ();
for (int32_t i = gameData.segs.nSegments; i; i--, segP++) {
	if (segP->m_function == SEGMENT_FUNC_ROBOTMAKER) {
		tProducerInfo& producer = gameData.producers.producers [segP->m_value];
		if (producer.bFlag) {
			producer.bFlag = 0;
			segP->m_function = SEGMENT_FUNC_NONE;
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
	CObject	*objP = OBJECTS.Buffer ();

for (i = 0; i < gameFileInfo.objects.count; i++, objP++) {
	objP->info.nNextInSeg = objP->info.nPrevInSeg = -1;
	if (objP->Type () != OBJ_NONE) {
		nObjSeg = objP->info.nSegment;
		if ((nObjSeg < 0) || (nObjSeg > gameData.segs.nLastSegment))	
			objP->SetType (OBJ_NONE);
		else {
			objP->info.nSegment = -1;	
			OBJECT (i)->LinkToSeg (nObjSeg);
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Make sure non-transparent doors are set correctly.
static void CheckAndFixDoors (void)
{
	CSegment* segP = SEGMENTS.Buffer ();

for (int32_t i = 0; i < gameData.segs.nSegments; i++, segP++) {
	CSide* sideP = segP->m_sides;
	for (int32_t j = 0; j < SEGMENT_SIDE_COUNT; j++, sideP++) {
		CWall* wallP = sideP->Wall ();
		if (!wallP || (wallP->nType != WALL_DOOR))
			continue;
		else if ((wallP->nClip < 0) || (wallP->nClip >= int32_t (gameData.walls.animP.Length ()))) 
			wallP->nClip = -1;
		else if (gameData.walls.animP [wallP->nClip].flags & WCF_TMAP1) {
			sideP->m_nBaseTex = gameData.walls.animP [wallP->nClip].frames [0];
			sideP->m_nOvlTex = 0;
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
	CWall*	wallP;

for (i = 0; i < gameData.walls.nWalls; i++)
	if ((WALL (i)->nTrigger >= gameData.trigs.m_nTriggers) && (WALL (i)->nTrigger != NO_TRIGGER)) {
#if TRACE
		PrintLog (0, "Removing reference to invalid trigger %d from wall %d\n", WALL (i)->nTrigger, i);
#endif
		WALL (i)->nTrigger = NO_TRIGGER;	//kill CTrigger
		}

if (gameTopFileInfo.fileinfoVersion < 17) {
	CSegment* segP = SEGMENTS.Buffer ();
	for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++, segP++) {
		for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
			if ((wallP = segP->Wall (nSide))) {
				wallP->nSegment = nSegment;
				wallP->nSide = nSide;
				}
			}
		}
	}

// mark all triggers on "open" segment sides (i.e. which are connected to an adjacent segment) as "fly through" 
// to simplify internal trigger management
for (i = 0; i < gameData.walls.nWalls; i++) {
	CTrigger* trigP = TRIGGER (WALL (i)->nTrigger);
	if (trigP)
		trigP->m_info.nWall = i;
	}
}

// -----------------------------------------------------------------------------
//go through all triggers, killing unused ones
static void CheckAndFixTriggers (void)
{
	int32_t	h, i, j;
	int16_t	nSegment, nSide, nWall;

for (i = 0; i < gameData.trigs.m_nTriggers; ) {
	//	Find which CWall this CTrigger is connected to.
	for (j = 0; j < gameData.walls.nWalls; j++)
		if (WALL (j)->nTrigger == i)
			break;
		i++;
	}

for (i = 0; i < gameData.walls.nWalls; i++)
	WALL (i)->controllingTrigger = -1;

//	MK, 10/17/95: Make walls point back at the triggers that control them.
//	Go through all triggers, stuffing controllingTrigger field in WALLS.

CTrigger* trigP = TRIGGERS.Buffer ();
for (i = 0; i < gameData.trigs.m_nTriggers; i++, trigP++) {
	for (h = trigP->m_nLinks, j = 0; j < h; ) {
		if (trigP->m_sides [j] >= 0) {
			nSegment = trigP->m_segments [j];
			if (nSegment >= gameData.segs.nSegments) {
				if (j < --h) {
					trigP->m_segments [j] = trigP->m_segments [h];
					trigP->m_sides [j] = trigP->m_sides [h];
					}
				}
			else {
				nSide = trigP->m_sides [j];
				nWall = SEGMENT (nSegment)->WallNum (nSide);
				//check to see that if a CTrigger requires a CWall that it has one,
				//and if it requires a botGen that it has one
				if (trigP->m_info.nType == TT_OBJECT_PRODUCER) {
					if ((SEGMENT (nSegment)->m_function != SEGMENT_FUNC_ROBOTMAKER) && (SEGMENT (nSegment)->m_function != SEGMENT_FUNC_EQUIPMAKER)) {
						if (j < --h) {
							trigP->m_segments [j] = trigP->m_segments [h];
							trigP->m_sides [j] = trigP->m_sides [h];
							}
						continue;
						}
					}
				else if ((trigP->m_info.nType != TT_LIGHT_OFF) && (trigP->m_info.nType != TT_LIGHT_ON)) { //light triggers don't require walls
					if (IS_WALL (nWall))
						WALL (nWall)->controllingTrigger = i;
					else {
#if 0
						if (j < --h) {
							trigP->m_segments [j] = trigP->m_segments [h];
							trigP->m_sides [j] = trigP->m_sides [h];
							}
						continue;
#endif
						}
					}
				}
			}
		j++;
		}
	trigP->m_nLinks = h;
	}
}

//------------------------------------------------------------------------------

void CreateGenerators (void)
{
gameData.producers.nRepairCenters = 0;
for (int32_t i = 0; i < gameData.segs.nSegments; i++) {
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

gameData.objs.nNextSignature = 0;
gameData.render.lights.nStatic = 0;
gameData.objs.nInitialRobots = 0;
nGameSavePlayers = 0;
if (ReadObjectInfo (cf))
	return -1;
if (ReadWallInfo (cf))
	return -1;
gameData.walls.nWalls = gameFileInfo.walls.count;
if (ReadDoorInfo (cf))
	return -1;
if (ReadTriggerInfo (cf))
	return -1;
gameData.trigs.m_nTriggers = gameFileInfo.triggers.count;
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
//gameData.walls.nOpenDoors = gameFileInfo.doors.count;
gameData.trigs.m_nTriggers = gameFileInfo.triggers.count;
gameData.walls.nWalls = gameFileInfo.walls.count;
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

int32_t CheckSegmentConnections (void);
void	SetAmbientSoundFlags (void);


#define LEVEL_FILE_VERSION      8
//1 -> 2  add palette name
//2 -> 3  add control center explosion time
//3 -> 4  add reactor strength
//4 -> 5  killed hostage text stuff
//5 -> 6  added gameData.segs.secret.nReturnSegment and gameData.segs.secret.returnOrient
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
segCount.Create (gameData.segs.nVertices);
segCount.Clear ();

CSegment* segP = SEGMENTS.Buffer ();
for (int32_t i = gameData.segs.nSegments; i; i--, segP++) {
	for (int32_t j = 0; j < 8; j++)
		if (segP->m_vertices [j] != 0xFFFF)
			segCount [segP->m_vertices [j]]++;
	}

CArray<CVertSegRef> vertSegIndex;
vertSegIndex.Create (gameData.segs.nVertices);
vertSegIndex.Clear ();
CArray<int16_t> vertSegs;
vertSegs.Create (8 * gameData.segs.nSegments);
vertSegs.Clear ();
for (int32_t h = 0, i = 0; i < gameData.segs.nSegments; i++) {
	vertSegIndex [i].nIndex = h;
	h += segCount [i];
	}

segP = SEGMENTS.Buffer ();
for (int32_t i = gameData.segs.nSegments; i; i--, segP++) {
	for (int32_t j = 0; j < 8; j++) {
		if (segP->m_vertices [j] != 0xFFFF) {
			CVertSegRef* refP = &vertSegIndex [segP->m_vertices [j]];
			vertSegs [refP->nIndex + refP->nSegments++] = i;
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
gameData.segs.bHaveSlideSegs = 0;
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

	strcpy(gameData.segs.szLevelFilename, filename);

	cf.ReadInt ();
	gameData.segs.nLevelVersion = cf.ReadInt ();
	gameStates.app.bD2XLevel = (gameData.segs.nLevelVersion >= 10);
	#if TRACE
	console.printf (CON_DBG, "gameData.segs.nLevelVersion = %d\n", gameData.segs.nLevelVersion);
	#endif
	nMineDataOffset = cf.ReadInt ();
	nGameDataOffset = cf.ReadInt ();

	if (gameData.segs.nLevelVersion >= 8) {    //read dummy data
		cf.ReadInt ();
		cf.ReadShort ();
		cf.ReadByte ();
	}

	if (gameData.segs.nLevelVersion < 5)
		cf.ReadInt ();       //was hostagetext_offset

	if (gameData.segs.nLevelVersion > 1) {
		for (int32_t i = 0; i < (int32_t) sizeof (szCurrentLevelPalette); i++) {
			szCurrentLevelPalette [i] = (char) cf.ReadByte ();
			if (szCurrentLevelPalette [i] == '\n')
				szCurrentLevelPalette [i] = '\0';
			if (szCurrentLevelPalette [i] == '\0')
				break;
			}
		}
	if ((gameData.segs.nLevelVersion <= 1) || (szCurrentLevelPalette [0] == 0)) // descent 1 level
		strcpy (szCurrentLevelPalette, /*gameStates.app.bD1Mission ? D1_PALETTE :*/ DEFAULT_LEVEL_PALETTE); //D1_PALETTE looks off in D2X-XL

	if (gameData.segs.nLevelVersion >= 3)
		gameStates.app.nBaseCtrlCenExplTime = cf.ReadInt ();
	else
		gameStates.app.nBaseCtrlCenExplTime = DEFAULT_CONTROL_CENTER_EXPLOSION_TIME;

	if (gameData.segs.nLevelVersion >= 4)
		gameData.reactor.nStrength = cf.ReadInt ();
	else
		gameData.reactor.nStrength = -1;  //use old defaults

	if (gameData.segs.nLevelVersion >= 7) {
	#if TRACE
		console.printf (CON_DBG, "   loading dynamic lights ...\n");
	#endif
		if (0 > ReadVariableLights (cf)) {
			cf.Close ();
			PrintLog (-1);
			return 5;
			}
		}	

	if (gameData.segs.nLevelVersion < 6) {
		gameData.segs.secret.nReturnSegment = 0;
		gameData.segs.secret.returnOrient = CFixMatrix::IDENTITY;
		}
	else {
		gameData.segs.secret.nReturnSegment = cf.ReadInt ();
		for (int32_t i = 0; i < 9; i++)
			gameData.segs.secret.returnOrient.m.vec [i] = cf.ReadInt ();
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
	gameData.objs.lists.Init ();
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
//lightManager.Setup (nLevel); 
SetAmbientSoundFlags ();
PrintLog (-1);
return 0;
}

// ----------------------------------------------------------------------------
//eof

