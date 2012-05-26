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
void ReadObject(CObject *objP, CFile *f, int version);
#if DBG
void dump_mine_info(void);
#endif

int nGameSavePlayers = 0;
int nSavePOFNames = 0;
char szSavePOFNames [MAX_POLYGON_MODELS][SHORT_FILENAME_LEN];

//--unused-- CBitmap * Gamesave_saved_bitmap = NULL;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void VerifyObject (CObject * objP)
{
objP->SetLife (IMMORTAL_TIME);		//all loaded CObject are immortal, for now
if (objP->info.nType == OBJ_ROBOT) {
	++gameData.objs.nInitialRobots;
	// Make sure valid id...
	if (objP->info.nId >= gameData.bots.nTypes [gameStates.app.bD1Data])
		objP->info.nId %= gameData.bots.nTypes [0];
	// Make sure model number & size are correct...
	if (objP->info.renderType == RT_POLYOBJ) {
		Assert(ROBOTINFO (objP->info.nId).nModel != -1);
			//if you fail this assert, it means that a robot in this level
			//hasn't been loaded, possibly because he's marked as
			//non-shareware.  To see what robot number, print objP->info.nId.
		Assert(ROBOTINFO (objP->info.nId).always_0xabcd == 0xabcd);
			//if you fail this assert, it means that the robot_ai for
			//a robot in this level hasn't been loaded, possibly because
			//it's marked as non-shareware.  To see what robot number,
			//print objP->info.nId.
		objP->rType.polyObjInfo.nModel = ROBOTINFO (objP->info.nId).nModel;
		objP->SetSizeFromModel ();
		}
	if (objP->info.nId == 65)						//special "reactor" robots
		objP->info.movementType = MT_NONE;
	if (objP->info.movementType == MT_PHYSICS) {
		objP->mType.physInfo.mass = ROBOTINFO (objP->info.nId).mass;
		objP->mType.physInfo.drag = ROBOTINFO (objP->info.nId).drag;
		}
	}
else if (objP->info.nType == OBJ_EFFECT) {
	gameData.objs.nEffects++;
	}
else {		//Robots taken care of above
	if ((objP->info.renderType == RT_POLYOBJ) && (nSavePOFNames > 0)) {
		char *name = szSavePOFNames [objP->ModelId ()];
		if (*name) {
			for (int i = 0; (i < gameData.models.nPolyModels) && (i < nSavePOFNames); i++)
				if (*name && !stricmp (pofNames [i], name)) {		//found it!
					objP->rType.polyObjInfo.nModel = i;
					break;
					}
				}
		}
	}
if (objP->info.nType == OBJ_POWERUP) {
	if (objP->info.nId >= gameData.objs.pwrUp.nTypes + POWERUP_ADDON_COUNT) {
		objP->info.nId = 0;
		Assert(objP->info.renderType != RT_POLYOBJ);
		}
	objP->info.controlType = CT_POWERUP;
	if (objP->info.nId >= MAX_POWERUP_TYPES_D2)
		InitAddonPowerup (objP);
	else {
		objP->SetSizeFromPowerup ();
		objP->cType.powerupInfo.xCreationTime = 0;
		if (IsMultiGame) {
			AddPowerupInMine (objP->info.nId, true);
#if TRACE
			console.printf (CON_DBG, "PowerupLimiter: ID=%d\n", objP->info.nId);
			if (objP->info.nId > MAX_POWERUP_TYPES)
				console.printf (1,"POWERUP: Overwriting array bounds!\n");
#endif
			}
		}
	}
else if (objP->info.nType == OBJ_WEAPON) {
	if (objP->info.nId >= gameData.weapons.nTypes [0]) {
		objP->info.nId = 0;
		Assert(objP->info.renderType != RT_POLYOBJ);
		}
	if (objP->info.nId == SMALLMINE_ID) {		//make sure pmines have correct values
		objP->mType.physInfo.mass = gameData.weapons.info [objP->info.nId].mass;
		objP->mType.physInfo.drag = gameData.weapons.info [objP->info.nId].drag;
		objP->mType.physInfo.flags |= PF_FREE_SPINNING;
		// Make sure model number & size are correct...	
		Assert(objP->info.renderType == RT_POLYOBJ);
		objP->rType.polyObjInfo.nModel = gameData.weapons.info [objP->info.nId].nModel;
		objP->SetSizeFromModel ();
		}
	}
else if (objP->info.nType == OBJ_REACTOR) {
	objP->info.renderType = RT_POLYOBJ;
	objP->info.controlType = CT_CNTRLCEN;
	if (gameData.segs.nLevelVersion <= 1) { // descent 1 reactor
		objP->info.nId = 0;                         // used to be only one kind of reactor
		objP->rType.polyObjInfo.nModel = gameData.reactor.props [0].nModel;// descent 1 reactor
		}
	}
else if (objP->info.nType == OBJ_PLAYER) {
	if (objP == gameData.objs.consoleP)	
		InitPlayerObject ();
	else
		if (objP->info.renderType == RT_POLYOBJ)	//recover from Matt's pof file matchup bug
			objP->rType.polyObjInfo.nModel = gameData.pig.ship.player->nModel;
	//Make sure orient matrix is orthogonal
	gameOpts->render.nMathFormat = 0;
	objP->info.position.mOrient.CheckAndFix();
	gameOpts->render.nMathFormat = gameOpts->render.nDefMathFormat;
	objP->info.nId = nGameSavePlayers++;
	}
else if (objP->info.nType == OBJ_HOSTAGE) {
	objP->info.renderType = RT_HOSTAGE;
	objP->info.controlType = CT_POWERUP;
	}
objP->Link ();
}

//------------------------------------------------------------------------------
//static gs_skip(int len,CFile *file)
//{
//
//	cf.Seek (file,len,SEEK_CUR);
//}

//------------------------------------------------------------------------------

int MultiPowerupIs4Pack(int);
//reads one CObject of the given version from the given file
extern int RemoveTriggerNum (int trigger_num);

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

static int ReadGameFileInfo (CFile& cf, int nStartOffset)
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

static int ReadLevelInfo (CFile& cf)
{
if (gameTopFileInfo.fileinfoVersion >= 31) { //load mine filename
	// read newline-terminated string, not sure what version this changed.
	for (int i = 0; i < (int) sizeof (missionManager.szCurrentLevel); i++) {
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

static int ReadObjectInfo (CFile& cf)
{
	int	i;

if ((gameFileInfo.objects.offset > -1) && gameFileInfo.objects.count) {
	CObject	*objP = OBJECTS.Buffer ();
	if (cf.Seek (gameFileInfo.objects.offset, SEEK_SET)) {
		Error ("Error seeking to object data\n(file damaged or invalid)");
		return -1;
		}
	OBJECTS.Clear (0, gameFileInfo.objects.count);
	i = 0;
	while (i < gameFileInfo.objects.count) {
		objP->Read (cf);
		if (!IsMultiGame && objP->Multiplayer ())
			gameFileInfo.objects.count--;
		else {
			objP->info.nSignature = gameData.objs.nNextSignature++;
#if DBG
			if (i == nDbgObj) {
				extern int dbgObjInstances;
				dbgObjInstances++;
				}
#endif
			VerifyObject (objP);
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

static int ReadWallInfo (CFile& cf)
{
if (gameFileInfo.walls.count && (gameFileInfo.walls.offset > -1)) {
	int	i;

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
			WALLS [i].Read (cf); // v20 walls and up.
		else if (gameTopFileInfo.fileinfoVersion >= 17) {
			tWallV19 w;

			ReadWallV19(w, cf);
			WALLS [i].nSegment	   = w.nSegment;
			WALLS [i].nSide			= w.nSide;
			WALLS [i].nLinkedWall	= w.nLinkedWall;
			WALLS [i].nType			= w.nType;
			WALLS [i].flags			= w.flags;
			WALLS [i].hps				= w.hps;
			WALLS [i].nTrigger		= w.nTrigger;
			WALLS [i].nClip			= w.nClip;
			WALLS [i].keys				= w.keys;
			WALLS [i].state			= WALL_DOOR_CLOSED;
			}
		else {
			tWallV16 w;

			ReadWallV16(w, cf);
			WALLS [i].nSegment = WALLS [i].nSide = WALLS [i].nLinkedWall = -1;
			WALLS [i].nType		= w.nType;
			WALLS [i].flags		= w.flags;
			WALLS [i].hps			= w.hps;
			WALLS [i].nTrigger	= w.nTrigger;
			WALLS [i].nClip		= w.nClip;
			WALLS [i].keys			= w.keys;
			}
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static int ReadDoorInfo (CFile& cf)
{
if (gameFileInfo.doors.offset > -1) {
	int	i;
	if (cf.Seek (gameFileInfo.doors.offset, SEEK_SET)) {
		Error ("Error seeking to door data\n(file damaged or invalid)");
		return -1;
		}
	for (i = 0; i < gameFileInfo.doors.count; i++) {
		if (gameTopFileInfo.fileinfoVersion >= 20)
			ReadActiveDoor (gameData.walls.activeDoors [i], cf); // version 20 and up
		else {
			v19_door d;
			short nConnSeg, nConnSide;
			CSegment* segP;

			ReadActiveDoorV19 (d, cf);
			gameData.walls.activeDoors [i].nPartCount = d.nPartCount;
			for (int j = 0; j < d.nPartCount; j++) {
				segP = SEGMENTS + d.seg [j];
				nConnSeg = segP->m_children [d.nSide [j]];
				nConnSide = segP->ConnectedSide (SEGMENTS + nConnSeg);
				gameData.walls.activeDoors [i].nFrontWall [j] = segP->WallNum (d.nSide [j]);
				gameData.walls.activeDoors [i].nBackWall [j] = SEGMENTS [nConnSeg].WallNum (nConnSide);
				}
			}
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static void CheckTriggerInfo (void)
{
	int		h, i, j;
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

int CmpObjTriggers (const CTrigger* pv, const CTrigger* pm)
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
	int i;
	short nObject = -1;
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

static int ReadTriggerInfo (CFile& cf)
{
	int		i;
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
				int t, nType = 0, flags = 0;
				if (gameTopFileInfo.fileinfoVersion == 30)
					V30TriggerRead (trig, cf);
				else {
					tTriggerV29 trig29;
					V29TriggerRead (trig29, cf);
					trigP->m_info.flagsD1 = trig.flags = trig29.flags;
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
					cf.Seek (2 * sizeof (short), SEEK_CUR);
					OBJTRIGGERS [i].m_info.nObject = cf.ReadShort ();
					}
				if (gameTopFileInfo.fileinfoVersion < 36) 
					cf.Seek (700 * sizeof (short), SEEK_CUR);
				else 
					cf.Seek (cf.ReadShort () * 2 * sizeof (short), SEEK_CUR);
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

static int ReadReactorInfo (CFile& cf)
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

static int AssignProducer (tObjectProducerInfo& producerInfo, int nObjProducer, int nFunction, sbyte bFlag)
{
#if 1

	CSegment* segP = &SEGMENTS [0];

for (int i = 0, j = gameData.segs.nSegments; i < j; i++, segP++) {
	if ((segP->Function () == nFunction) && segP->m_nObjProducer == nObjProducer)) {
		tObjectProducerInfo& objProducer = (nFunction == SEGMENT_FUNC_ROBOTMAKER) 
													  ? gameData.producers.robotMakers [nObjProducer] 
													  : gameData.producers.equipmentMakers [nObjProducer];
		objProducer.nSegment = i;
		objProducer.nProducer = segP->m_value;
		memcpy (objProducer.objFlags, producerInfo.objFlags, sizeof (objProducer.objFlags));
		return segP->m_nObjProducer = producerInfo.nProducer;
		}
	}
return -1;

#elif 0

CSegment* segP = &SEGMENTS [producerInfo.nSegment];
if (segP->m_function != nFunction)
	return -1;
producerInfo.nProducer = segP->m_nObjProducer;
tObjectProducerInfo& objProducer = (nFunction == SEGMENT_FUNC_ROBOTMAKER) 
											  ? gameData.producers.robotMakers [producerInfo.nProducer] 
											  : gameData.producers.equipmentMakers [producerInfo.nProducer];
if (objProducer.bAssigned)
	return -1;
int nProducer = objProducer.nProducer;
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
int nProducer = objProducer.nProducer;
tProducerInfo& producer = gameData.producers.producers [nProducer];
if (producer.nSegment < 0)
	return -1;
CSegment* segP = &SEGMENTS [producer.nSegment];
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

static int ReadBotGenInfo (CFile& cf)
{
if (gameFileInfo.botGen.offset > -1) {
	if (cf.Seek (gameFileInfo.botGen.offset, SEEK_SET)) {
		Error ("Error seeking to robot generator data\n(file damaged or invalid)");
		return -1;
		}
	tObjectProducerInfo m;
	m.objFlags [2] = gameData.objs.nVertigoBotFlags;
	for (int h, i = 0; i < gameFileInfo.botGen.count; ) {
		ReadObjectProducerInfo (&m, cf, gameTopFileInfo.fileinfoVersion < 27);
		int h = AssignProducer (m, i, SEGMENT_FUNC_ROBOTMAKER, 1);
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

static int ReadEquipGenInfo (CFile& cf)
{
if (gameFileInfo.equipGen.offset > -1) {
	if (cf.Seek (gameFileInfo.equipGen.offset, SEEK_SET)) {
		Error ("Error seeking to equipment generator data\n(file damaged or invalid)");
		return -1;
		}
	tObjectProducerInfo m;
	m.objFlags [2] = 0;
	for (int h, i = 0; i < gameFileInfo.equipGen.count;) {
		ReadObjectProducerInfo (&m, cf, false);
		int h = AssignProducer (m, i, SEGMENT_FUNC_EQUIPMAKER, 2);
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
for (int i = gameData.segs.nSegments; i; i--, segP++) {
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

static int ReadLightDeltaIndexInfo (CFile& cf)
{
gameData.render.lights.nStatic = 0;
if ((gameFileInfo.lightDeltaIndices.offset > -1) && gameFileInfo.lightDeltaIndices.count) {
	int	i;

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

static int ReadLightDeltaInfo (CFile& cf)
{
if ((gameFileInfo.lightDeltas.offset > -1) && gameFileInfo.lightDeltas.count) {
	int	h, i;

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

static int ReadVariableLights (CFile& cf)
{
	int	nLights = cf.ReadInt ();

if (!nLights)
	return 0;
if (!gameData.render.lights.flicker.Create (nLights))
	return -1;
for (int i = 0; i < nLights; i++)
	gameData.render.lights.flicker [i].Read (cf);
return nLights;
}

// -----------------------------------------------------------------------------

static void CheckAndLinkObjects (void)
{
	int		i, nObjSeg;
	CObject	*objP = OBJECTS.Buffer ();

for (i = 0; i < gameFileInfo.objects.count; i++, objP++) {
	objP->info.nNextInSeg = objP->info.nPrevInSeg = -1;
	if (objP->info.nType != OBJ_NONE) {
		nObjSeg = objP->info.nSegment;
		if ((nObjSeg < 0) || (nObjSeg > gameData.segs.nLastSegment))	
			objP->info.nType = OBJ_NONE;
		else {
			objP->info.nSegment = -1;	
			OBJECTS [i].LinkToSeg (nObjSeg);
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Make sure non-transparent doors are set correctly.
static void CheckAndFixDoors (void)
{
	CSegment* segP = SEGMENTS.Buffer ();

for (int i = 0; i < gameData.segs.nSegments; i++, segP++) {
	CSide* sideP = segP->m_sides;
	for (int j = 0; j < SEGMENT_SIDE_COUNT; j++, sideP++) {
		CWall* wallP = sideP->Wall ();
		if (!wallP || (wallP->nType != WALL_DOOR))
			continue;
		else if ((wallP->nClip < 0) || (wallP->nClip >= int (gameData.walls.animP.Length ()))) 
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
	int		i;
	short		nSegment, nSide;
	CWall*	wallP;

for (i = 0; i < gameData.walls.nWalls; i++)
	if (WALLS [i].nTrigger >= gameData.trigs.m_nTriggers) {
#if TRACE
		console.printf (CON_DBG,"Removing reference to invalid CTrigger %d from CWall %d\n",WALLS [i].nTrigger,i);
#endif
		WALLS [i].nTrigger = NO_TRIGGER;	//kill CTrigger
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
}

// -----------------------------------------------------------------------------
//go through all triggers, killing unused ones
static void CheckAndFixTriggers (void)
{
	int	h, i, j;
	short	nSegment, nSide, nWall;

for (i = 0; i < gameData.trigs.m_nTriggers; ) {
	//	Find which CWall this CTrigger is connected to.
	for (j = 0; j < gameData.walls.nWalls; j++)
		if (WALLS [j].nTrigger == i)
			break;
		i++;
	}

for (i = 0; i < gameData.walls.nWalls; i++)
	WALLS [i].controllingTrigger = -1;

//	MK, 10/17/95: Make walls point back at the triggers that control them.
//	Go through all triggers, stuffing controllingTrigger field in WALLS.

CTrigger* trigP = TRIGGERS.Buffer ();
for (i = 0; i < gameData.trigs.m_nTriggers; i++, trigP++) {
	for (h = trigP->m_nLinks, j = 0; j < h; ) {
		nSegment = trigP->m_segments [j];
		if (nSegment >= gameData.segs.nSegments) {
			if (j < --h) {
				trigP->m_segments [j] = trigP->m_segments [h];
				trigP->m_sides [j] = trigP->m_sides [h];
				}
			}
		else {
			nSide = trigP->m_sides [j];
			nWall = SEGMENTS [nSegment].WallNum (nSide);
			//check to see that if a CTrigger requires a CWall that it has one,
			//and if it requires a botGen that it has one
			if (trigP->m_info.nType == TT_OBJECT_PRODUCER) {
				if ((SEGMENTS [nSegment].m_function != SEGMENT_FUNC_ROBOTMAKER) && (SEGMENTS [nSegment].m_function != SEGMENT_FUNC_EQUIPMAKER)) {
					if (j < --h) {
						trigP->m_segments [j] = trigP->m_segments [h];
						trigP->m_sides [j] = trigP->m_sides [h];
						}
					continue;
					}
				}
			else if ((trigP->m_info.nType != TT_LIGHT_OFF) && (trigP->m_info.nType != TT_LIGHT_ON)) { //light triggers don't require walls
				if (IS_WALL (nWall))
					WALLS [nWall].controllingTrigger = i;
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
			j++;
			}
		}
	trigP->m_nLinks = h;
	}
}

//------------------------------------------------------------------------------

void CreateGenerators (void)
{
gameData.producers.nRepairCenters = 0;
for (int i = 0; i < gameData.segs.nSegments; i++) {
	SEGMENTS [i].CreateGenerator (SEGMENTS [i].m_function);
	}
}

// -----------------------------------------------------------------------------
// Load game 
// Loads all the relevant data for a level.
// If level != -1, it loads the filename with extension changed to .min
// Otherwise it loads the appropriate level mine.
// returns 0=everything ok, 1=old version, -1=error
int LoadMineDataCompiled (CFile& cf, int bFileInfo)
{
	int 	nStartOffset;

nStartOffset = (int) cf.Tell ();
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
if (ReadDoorInfo (cf))
	return -1;
if (ReadTriggerInfo (cf))
	return -1;
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

int CheckSegmentConnections (void);
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
	int	nIndex;
	short	nSegments;
};

void CreateVertexSegmentList (void)
{
#if 0
CArray<short> segCount;
segCount.Create (gameData.segs.nVertices);
segCount.Clear ();

CSegment* segP = SEGMENTS.Buffer ();
for (int i = gameData.segs.nSegments; i; i--, segP++) {
	for (int j = 0; j < 8; j++)
		if (segP->m_vertices [j] != 0xFFFF)
			segCount [segP->m_vertices [j]]++;
	}

CArray<CVertSegRef> vertSegIndex;
vertSegIndex.Create (gameData.segs.nVertices);
vertSegIndex.Clear ();
CArray<short> vertSegs;
vertSegs.Create (8 * gameData.segs.nSegments);
vertSegs.Clear ();
for (int h = 0, i = 0; i < gameData.segs.nSegments; i++) {
	vertSegIndex [i].nIndex = h;
	h += segCount [i];
	}

segP = SEGMENTS.Buffer ();
for (int i = gameData.segs.nSegments; i; i--, segP++) {
	for (int j = 0; j < 8; j++) {
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
int LoadLevelData (char * pszFilename, int nLevel)
{
	CFile cf;
	char	filename [128];
	int	nMineDataOffset, nGameDataOffset;
	int	nError;

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
		for (int i = 0; i < (int) sizeof (szCurrentLevelPalette); i++) {
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
		for (int i = 0; i < 9; i++)
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

