/* $Id: gamesave.c,v 1.21 2003/06/16 07:15:59 btb Exp $ */
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

#include "inferno.h"
#include "pstypes.h"
#include "strutil.h"
#include "mono.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "newmenu.h"
#ifdef EDITOR
#	include "editor/editor.h"
#endif
#include "error.h"
#include "dynlight.h"
#include "object.h"
#include "game.h"
#include "screens.h"
#include "wall.h"
#include "gamemine.h"
#include "robot.h"
#include "fireball.h"

#include "cfile.h"
#include "bm.h"
#include "menu.h"
#include "switch.h"
#include "fuelcen.h"
#include "reactor.h"
#include "powerup.h"
#include "weapon.h"
#include "newdemo.h"
#include "loadgame.h"
#include "automap.h"
#include "polyobj.h"
#include "text.h"
#include "gamefont.h"
#include "gamesave.h"
#include "gamepal.h"
#include "laser.h"
#include "byteswap.h"
#include "multi.h"
#include "makesig.h"
#include "gameseg.h"
#include "light.h"
#include "objrender.h"
#include "createmesh.h"
#include "lightmap.h"

#define GAME_VERSION            32
#define GAME_COMPATIBLE_VERSION 22

//version 28->29  add delta light support
//version 27->28  controlcen id now is reactor number, not model number
//version 28->29  ??
//version 29->30  changed tTrigger structure
//version 30->31  changed tTrigger structure some more
//version 31->32  change tSegment structure, make it 512 bytes w/o editor, add gameData.segs.segment2s array.

#define MENU_CURSOR_X_MIN       MENU_X
#define MENU_CURSOR_X_MAX       MENU_X+6

tGameFileInfo	gameFileInfo;
game_top_fileinfo	gameTopFileInfo;

//  LINT: adding function prototypes
void ReadObject(tObject *objP, CFILE *f, int version);
#ifdef EDITOR
void writeObject(tObject *objP, FILE *f);
void DoLoadSaveLevels(int save);
#endif
#if DBG
void dump_mine_info(void);
#endif

#ifdef EDITOR
extern char mine_filename [];
extern int save_mine_data_compiled(FILE * SaveFile);
//--unused-- #else
//--unused-- char mine_filename [128];
#endif

int nGameSavePlayers = 0;
int nGameSaveOrgRobots = 0;
int nSavePOFNames = 0;
char szSavePOFNames [MAX_POLYGON_MODELS][SHORT_FILENAME_LEN];

//--unused-- grsBitmap * Gamesave_saved_bitmap = NULL;

//------------------------------------------------------------------------------
#ifdef EDITOR
// Return true if this level has a name of the form "level??"
// Note that a pathspec can appear at the beginning of the filename.
int IsRealLevel(char *filename)
{
	int len = (int) strlen(filename);

if (len < 6)
	return 0;
return !strnicmp(&filename [len-11], "level", 5);
}
#endif

//------------------------------------------------------------------------------

void VerifyObject (tObject * objP)
{
objP->info.xLifeLeft = IMMORTAL_TIME;		//all loaded tObject are immortal, for now
if (objP->info.nType == OBJ_ROBOT) {
	nGameSaveOrgRobots++;
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
		objP->info.xSize = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad;
		}
	if (objP->info.nId == 65)						//special "reactor" robots
		objP->info.movementType = MT_NONE;
	if (objP->info.movementType == MT_PHYSICS) {
		objP->mType.physInfo.mass = ROBOTINFO (objP->info.nId).mass;
		objP->mType.physInfo.drag = ROBOTINFO (objP->info.nId).drag;
		}
	}
else {		//Robots taken care of above
	if (objP->info.renderType == RT_POLYOBJ) {
		char *name = szSavePOFNames [objP->rType.polyObjInfo.nModel];
		for (int i = 0; i < gameData.models.nPolyModels; i++)
			if (!stricmp (Pof_names [i], name)) {		//found it!
				objP->rType.polyObjInfo.nModel = i;
				break;
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
		objP->info.xSize = gameData.objs.pwrUp.info [objP->info.nId].size;
		objP->cType.powerupInfo.xCreationTime = 0;
		if (gameData.app.nGameMode & GM_NETWORK) {
		if (MultiPowerupIs4Pack (objP->info.nId)) {
				gameData.multiplayer.powerupsInMine [objP->info.nId-1] += 4;
	 			gameData.multiplayer.maxPowerupsAllowed [objP->info.nId-1] += 4;
				}
			gameData.multiplayer.powerupsInMine [objP->info.nId]++;
			gameData.multiplayer.maxPowerupsAllowed [objP->info.nId]++;
#if TRACE
			con_printf (CONDBG, "PowerupLimiter: ID=%d\n", objP->info.nId);
			if (objP->info.nId > MAX_POWERUP_TYPES)
				con_printf (1,"POWERUP: Overwriting array bounds!\n");
#endif
			}
		}
	}
if (objP->info.nType == OBJ_WEAPON)	{
	if (objP->info.nId >= gameData.weapons.nTypes [0])	{
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
		objP->info.xSize = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad;
		}
	}
if (objP->info.nType == OBJ_REACTOR) {
	objP->info.renderType = RT_POLYOBJ;
	objP->info.controlType = CT_CNTRLCEN;
	if (gameData.segs.nLevelVersion <= 1) { // descent 1 reactor
		objP->info.nId = 0;                         // used to be only one kind of reactor
		objP->rType.polyObjInfo.nModel = gameData.reactor.props [0].nModel;// descent 1 reactor
		}
#ifdef EDITOR
	{
	int i;
	// Check, and set, strength of reactor
	for (i = 0; i < gameData.objs.types.count; i++)
		if ((gameData.objs.types.nType [i] == OL_CONTROL_CENTER) && 
			 (gameData.objs.types.nType.nId [i] == objP->info.nId)) {
			objP->info.xShields = gameData.objs.types.nType.nStrength [i];
			break;	
			}
		Assert(i < gameData.objs.types.count);		//make sure we found it
		}
#endif
	}
if (objP->info.nType == OBJ_PLAYER) {
	if (objP == gameData.objs.consoleP)	
		InitPlayerObject();
	else
		if (objP->info.renderType == RT_POLYOBJ)	//recover from Matt's pof file matchup bug
			objP->rType.polyObjInfo.nModel = gameData.pig.ship.player->nModel;
	//Make sure orient matrix is orthogonal
	gameOpts->render.nMathFormat = 0;
	objP->info.position.mOrient.CheckAndFix();
	gameOpts->render.nMathFormat = gameOpts->render.nDefMathFormat;
	objP->info.nId = nGameSavePlayers++;
	}
if (objP->info.nType == OBJ_HOSTAGE) {
	objP->info.renderType = RT_HOSTAGE;
	objP->info.controlType = CT_POWERUP;
	}
LinkObject (objP);
}

//------------------------------------------------------------------------------
//static gs_skip(int len,CFILE *file)
//{
//
//	CFSeek (file,len,SEEK_CUR);
//}

#ifdef EDITOR
static void gs_write_int(int i,FILE *file)
{
	if (fwrite(&i, sizeof(i), 1, file) != 1)
		Error("Error reading int in gamesave.c");

}

static void gs_write_fix(fix f,FILE *file)
{
	if (fwrite(&f, sizeof(f), 1, file) != 1)
		Error("Error reading fix in gamesave.c");

}

static void gs_write_short(short s,FILE *file)
{
	if (fwrite(&s, sizeof(s), 1, file) != 1)
		Error("Error reading short in gamesave.c");

}

static void gs_write_fixang(fixang f,FILE *file)
{
	if (fwrite(&f, sizeof(f), 1, file) != 1)
		Error("Error reading fixang in gamesave.c");

}

static void gs_write_byte(byte b,FILE *file)
{
	if (fwrite(&b, sizeof(b), 1, file) != 1)
		Error("Error reading byte in gamesave.c");

}

static void gr_write_vector(vmsVector *v,FILE *file)
{
	gs_write_fix(v->x,file);
	gs_write_fix(v->y,file);
	gs_write_fix(v->z,file);
}

static void gs_write_matrix(vmsMatrix *m,FILE *file)
{
	gr_write_vector(&m->rVec,file);
	gr_write_vector(&m->uVec,file);
	gr_write_vector(&m->fVec,file);
}

static void gs_write_angvec(vmsAngVec *v,FILE *file)
{
	gs_write_fixang(v->p,file);
	gs_write_fixang(v->b,file);
	gs_write_fixang(v->h,file);
}

#endif

//------------------------------------------------------------------------------

extern int MultiPowerupIs4Pack(int);
//reads one tObject of the given version from the given file
void ReadObject (tObject *objP, CFILE *cfP, int version)
{
	int	i;

objP->info.nType = CFReadByte (cfP);
objP->info.nId = CFReadByte (cfP);
objP->info.controlType = CFReadByte (cfP);
objP->info.movementType = CFReadByte (cfP);
objP->info.renderType = CFReadByte (cfP);
objP->info.nFlags = CFReadByte (cfP);
objP->info.nSegment = CFReadShort (cfP);
objP->info.nAttachedObj = -1;
CFReadVector(objP->info.position.vPos, cfP);
CFReadMatrix(objP->info.position.mOrient, cfP);
objP->info.xSize = CFReadFix (cfP);
objP->info.xShields = CFReadFix (cfP);
CFReadVector (objP->info.vLastPos, cfP);
objP->info.contains.nType = CFReadByte (cfP);
objP->info.contains.nId = CFReadByte (cfP);
objP->info.contains.nCount = CFReadByte (cfP);
switch (objP->info.movementType) {
	case MT_PHYSICS:
		CFReadVector (objP->mType.physInfo.velocity, cfP);
		CFReadVector (objP->mType.physInfo.thrust, cfP);
		objP->mType.physInfo.mass = CFReadFix (cfP);
		objP->mType.physInfo.drag = CFReadFix (cfP);
		objP->mType.physInfo.brakes = CFReadFix (cfP);
		CFReadVector (objP->mType.physInfo.rotVel, cfP);
		CFReadVector (objP->mType.physInfo.rotThrust, cfP);
		objP->mType.physInfo.turnRoll	= CFReadFixAng (cfP);
		objP->mType.physInfo.flags	= CFReadShort (cfP);
		break;

	case MT_SPINNING:
		CFReadVector (objP->mType.spinRate, cfP);
		break;

	case MT_NONE:
		break;

	default:
		Int3();
	}

switch (objP->info.controlType) {
	case CT_AI: 
		objP->cType.aiInfo.behavior = CFReadByte (cfP);
		for (i=0;i<MAX_AI_FLAGS;i++)
			objP->cType.aiInfo.flags [i] = CFReadByte (cfP);
		objP->cType.aiInfo.nHideSegment = CFReadShort (cfP);
		objP->cType.aiInfo.nHideIndex = CFReadShort (cfP);
		objP->cType.aiInfo.nPathLength = CFReadShort (cfP);
		objP->cType.aiInfo.nCurPathIndex = (char) CFReadShort (cfP);
		if (version <= 25) {
			CFReadShort (cfP);	//				objP->cType.aiInfo.follow_path_start_seg	= 
			CFReadShort (cfP);	//				objP->cType.aiInfo.follow_path_end_seg		= 
			}
		break;

	case CT_EXPLOSION:
		objP->cType.explInfo.nSpawnTime = CFReadFix (cfP);
		objP->cType.explInfo.nDeleteTime	= CFReadFix (cfP);
		objP->cType.explInfo.nDeleteObj = CFReadShort (cfP);
		objP->cType.explInfo.attached.nNext = objP->cType.explInfo.attached.nPrev = objP->cType.explInfo.attached.nParent = -1;
		break;

	case CT_WEAPON: //do I really need to read these?  Are they even saved to disk?
		objP->cType.laserInfo.parent.nType = CFReadShort (cfP);
		objP->cType.laserInfo.parent.nObject = CFReadShort (cfP);
		objP->cType.laserInfo.parent.nSignature = CFReadInt (cfP);
		break;

	case CT_LIGHT:
		objP->cType.lightInfo.intensity = CFReadFix (cfP);
		break;

	case CT_POWERUP:
		if (version >= 25)
			objP->cType.powerupInfo.nCount = CFReadInt (cfP);
		else
			objP->cType.powerupInfo.nCount = 1;
		if (objP->info.nId == POW_VULCAN)
			objP->cType.powerupInfo.nCount = VULCAN_WEAPON_AMMO_AMOUNT;
		else if (objP->info.nId == POW_GAUSS)
			objP->cType.powerupInfo.nCount = VULCAN_WEAPON_AMMO_AMOUNT;
		else if (objP->info.nId == POW_OMEGA)
			objP->cType.powerupInfo.nCount = MAX_OMEGA_CHARGE;
		break;

	case CT_NONE:
	case CT_FLYING:
	case CT_DEBRIS:
		break;

	case CT_SLEW:		//the tPlayer is generally saved as slew
		break;

	case CT_CNTRLCEN:
		break;

	case CT_MORPH:
	case CT_FLYTHROUGH:
	case CT_REPAIRCEN:
		default:
		Int3();
	}

switch (objP->info.renderType) {
	case RT_NONE:
		break;

	case RT_MORPH:
	case RT_POLYOBJ: {
		int i,tmo;
		objP->rType.polyObjInfo.nModel = CFReadInt (cfP);
		for (i=0;i<MAX_SUBMODELS;i++)
			CFReadAngVec(objP->rType.polyObjInfo.animAngles [i], cfP);
		objP->rType.polyObjInfo.nSubObjFlags = CFReadInt (cfP);
		tmo = CFReadInt (cfP);
#ifndef EDITOR
		objP->rType.polyObjInfo.nTexOverride = tmo;
#else
		if (tmo==-1)
			objP->rType.polyObjInfo.nTexOverride = -1;
		else {
			int xlated_tmo = tmap_xlate_table [tmo];
			if (xlated_tmo < 0)	{
#if TRACE
				con_printf (CONDBG, "Couldn't find texture for demo tObject, nModel = %d\n", objP->rType.polyObjInfo.nModel);
#endif
				Int3();
				xlated_tmo = 0;
				}
			objP->rType.polyObjInfo.nTexOverride	= xlated_tmo;
			}
#endif
		objP->rType.polyObjInfo.nAltTextures = 0;
		break;
		}

	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
		objP->rType.vClipInfo.nClipIndex	= CFReadInt (cfP);
		objP->rType.vClipInfo.xFrameTime	= CFReadFix (cfP);
		objP->rType.vClipInfo.nCurFrame	= CFReadByte (cfP);
		break;

	case RT_THRUSTER:
	case RT_LASER:
		break;

	case RT_SMOKE:
		objP->rType.smokeInfo.nLife = CFReadInt (cfP);
		objP->rType.smokeInfo.nSize [0] = CFReadInt (cfP);
		objP->rType.smokeInfo.nParts = CFReadInt (cfP);
		objP->rType.smokeInfo.nSpeed = CFReadInt (cfP);
		objP->rType.smokeInfo.nDrift = CFReadInt (cfP);
		objP->rType.smokeInfo.nBrightness = CFReadInt (cfP);
		objP->rType.smokeInfo.color.red = CFReadByte (cfP);
		objP->rType.smokeInfo.color.green = CFReadByte (cfP);
		objP->rType.smokeInfo.color.blue = CFReadByte (cfP);
		objP->rType.smokeInfo.color.alpha = CFReadByte (cfP);
		objP->rType.smokeInfo.nSide = CFReadByte (cfP);
		break;

	case RT_LIGHTNING:
		objP->rType.lightningInfo.nLife = CFReadInt (cfP);
		objP->rType.lightningInfo.nDelay = CFReadInt (cfP);
		objP->rType.lightningInfo.nLength = CFReadInt (cfP);
		objP->rType.lightningInfo.nAmplitude = CFReadInt (cfP);
		objP->rType.lightningInfo.nOffset = CFReadInt (cfP);
		objP->rType.lightningInfo.nLightnings = CFReadShort (cfP);
		objP->rType.lightningInfo.nId = CFReadShort (cfP);
		objP->rType.lightningInfo.nTarget = CFReadShort (cfP);
		objP->rType.lightningInfo.nNodes = CFReadShort (cfP);
		objP->rType.lightningInfo.nChildren = CFReadShort (cfP);
		objP->rType.lightningInfo.nSteps = CFReadShort (cfP);
		objP->rType.lightningInfo.nAngle = CFReadByte (cfP);
		objP->rType.lightningInfo.nStyle = CFReadByte (cfP);
		objP->rType.lightningInfo.nSmoothe = CFReadByte (cfP);
		objP->rType.lightningInfo.bClamp = CFReadByte (cfP);
		objP->rType.lightningInfo.bPlasma = CFReadByte (cfP);
		objP->rType.lightningInfo.bSound = CFReadByte (cfP);
		objP->rType.lightningInfo.bRandom = CFReadByte (cfP);
		objP->rType.lightningInfo.bInPlane = CFReadByte (cfP);
		objP->rType.lightningInfo.color.red = CFReadByte (cfP);
		objP->rType.lightningInfo.color.green = CFReadByte (cfP);
		objP->rType.lightningInfo.color.blue = CFReadByte (cfP);
		objP->rType.lightningInfo.color.alpha = CFReadByte (cfP);
		break;

	default:
		Int3();
	}
}

//------------------------------------------------------------------------------
#ifdef EDITOR

//writes one tObject to the given file
void writeObject(tObject *objP,FILE *f)
{
	gs_write_byte(objP->info.nType,f);
	gs_write_byte(objP->info.nId,f);

	gs_write_byte(objP->info.controlType,f);
	gs_write_byte(objP->info.movementType,f);
	gs_write_byte(objP->info.renderType,f);
	gs_write_byte(objP->info.nFlags,f);

	gs_write_short(objP->info.nSegment,f);

	gr_write_vector(&objP->info.position.vPos,f);
	gs_write_matrix(&objP->info.position.mOrient,f);

	gs_write_fix(objP->info.xSize,f);
	gs_write_fix(objP->info.xShields,f);

	gr_write_vector(&objP->info.vLastPos,f);

	gs_write_byte(objP->info.contains.nType,f);
	gs_write_byte(objP->info.contains.nId,f);
	gs_write_byte(objP->info.contains.count,f);

	switch (objP->info.movementType) {

		case MT_PHYSICS:

	 		gr_write_vector(&objP->mType.physInfo.velocity,f);
			gr_write_vector(&objP->mType.physInfo.thrust,f);

			gs_write_fix(objP->mType.physInfo.mass,f);
			gs_write_fix(objP->mType.physInfo.drag,f);
			gs_write_fix(objP->mType.physInfo.brakes,f);

			gr_write_vector(&objP->mType.physInfo.rotVel,f);
			gr_write_vector(&objP->mType.physInfo.rotThrust,f);

			gs_write_fixang(objP->mType.physInfo.turnRoll,f);
			gs_write_short(objP->mType.physInfo.flags,f);

			break;

		case MT_SPINNING:

			gr_write_vector(&objP->mType.spinRate,f);
			break;

		case MT_NONE:
			break;

		default:
			Int3();
	}

	switch (objP->info.controlType) {

		case CT_AI: {
			int i;

			gs_write_byte(objP->cType.aiInfo.behavior,f);

			for (i=0;i<MAX_AI_FLAGS;i++)
				gs_write_byte(objP->cType.aiInfo.flags [i],f);

			gs_write_short(objP->cType.aiInfo.nHideSegment,f);
			gs_write_short(objP->cType.aiInfo.nHideIndex,f);
			gs_write_short(objP->cType.aiInfo.nPathLength,f);
			gs_write_short(objP->cType.aiInfo.nCurPathIndex,f);

			// -- unused! mk, 08/13/95 -- gs_write_short(objP->cType.aiInfo.follow_path_start_seg,f);
			// -- unused! mk, 08/13/95 -- gs_write_short(objP->cType.aiInfo.follow_path_end_seg,f);

			break;
		}

		case CT_EXPLOSION:

			gs_write_fix(objP->cType.explInfo.nSpawnTime,f);
			gs_write_fix(objP->cType.explInfo.nDeleteTime,f);
			gs_write_short(objP->cType.explInfo.nDeleteObj,f);

			break;

		case CT_WEAPON:

			//do I really need to write these OBJECTS?

			gs_write_short(objP->cType.laserInfo.parent.nType,f);
			gs_write_short(objP->cType.laserInfo.parent.nObject,f);
			gs_write_int(objP->cType.laserInfo.parent.nSignature,f);

			break;

		case CT_LIGHT:

			gs_write_fix(objP->cType.lightInfo.intensity,f);
			break;

		case CT_POWERUP:

			gs_write_int(objP->cType.powerupInfo.count,f);
			break;

		case CT_NONE:
		case CT_FLYING:
		case CT_DEBRIS:
			break;

		case CT_SLEW:		//the tPlayer is generally saved as slew
			break;

		case CT_CNTRLCEN:
			break;			//control center tObject.

		case CT_MORPH:
		case CT_REPAIRCEN:
		case CT_FLYTHROUGH:
		default:
			Int3();

	}

	switch (objP->info.renderType) {

		case RT_NONE:
			break;

		case RT_MORPH:
		case RT_POLYOBJ: {
			int i;

			gs_write_int(objP->rType.polyObjInfo.nModel,f);

			for (i=0;i<MAX_SUBMODELS;i++)
				gs_write_angvec(&objP->rType.polyObjInfo.animAngles [i],f);

			gs_write_int(objP->rType.polyObjInfo.nSubObjFlags,f);

			gs_write_int(objP->rType.polyObjInfo.nTexOverride,f);

			break;
		}

		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:

			gs_write_int(objP->rType.vClipInfo.nClipIndex,f);
			gs_write_fix(objP->rType.vClipInfo.xFrameTime,f);
			gs_write_byte(objP->rType.vClipInfo.nCurFrame,f);

			break;

		case RT_THRUSTER:
		case RT_LASER:
			break;

		default:
			Int3();

	}

}
#endif

extern int RemoveTriggerNum (int trigger_num);

// -----------------------------------------------------------------------------

#define	DLIndex(_i)	(gameData.render.lights.deltaIndices + (_i))

void SortDLIndexD2X (int left, int right)
{
	int	l = left,
			r = right,
			m = (left + right) / 2;
	short	mSeg = DLIndex (m)->d2x.nSegment, 
			mSide = DLIndex (m)->d2x.nSide;
	tLightDeltaIndex	*pl, *pr;

do {
	pl = DLIndex (l);
	while ((pl->d2x.nSegment < mSeg) || ((pl->d2x.nSegment == mSeg) && (pl->d2x.nSide < mSide))) {
		pl++;
		l++;
		}
	pr = DLIndex (r);
	while ((pr->d2x.nSegment > mSeg) || ((pr->d2x.nSegment == mSeg) && (pr->d2x.nSide > mSide))) {
		pr--;
		r--;
		}
	if (l <= r) {
		if (l < r) {
			tLightDeltaIndex	h = *pl;
			*pl = *pr;
			*pr = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (right > l)
   SortDLIndexD2X (l, right);
if (r > left)
   SortDLIndexD2X (left, r);
}

// -----------------------------------------------------------------------------

void SortDLIndexD2 (int left, int right)
{
	int	l = left,
			r = right,
			m = (left + right) / 2;
	short	mSeg = DLIndex (m)->d2.nSegment, 
			mSide = DLIndex (m)->d2.nSide;
	tLightDeltaIndex	*pl, *pr;

do {
	pl = DLIndex (l);
	while ((pl->d2.nSegment < mSeg) || ((pl->d2.nSegment == mSeg) && (pl->d2.nSide < mSide))) {
		pl++;
		l++;
		}
	pr = DLIndex (r);
	while ((pr->d2.nSegment > mSeg) || ((pr->d2.nSegment == mSeg) && (pr->d2.nSide > mSide))) {
		pr--;
		r--;
		}
	if (l <= r) {
		if (l < r) {
			tLightDeltaIndex	h = *pl;
			*pl = *pr;
			*pr = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (right > l)
   SortDLIndexD2 (l, right);
if (r > left)
   SortDLIndexD2 (left, r);
}

// -----------------------------------------------------------------------------

void SortDLIndex (void)
{
if (gameStates.render.bD2XLights)
	SortDLIndexD2X (0, gameFileInfo.lightDeltaIndices.count - 1);
else
	SortDLIndexD2 (0, gameFileInfo.lightDeltaIndices.count - 1);
}

// -----------------------------------------------------------------------------

static void InitGameFileInfo (void)
{
gameFileInfo.level				=	-1;
gameFileInfo.player.offset		=	-1;
gameFileInfo.player.size		=	sizeof(tPlayer);
gameFileInfo.objects.offset	=	-1;
gameFileInfo.objects.count		=	0;
gameFileInfo.objects.size		=	sizeof(tObject);  
gameFileInfo.walls.offset		=	-1;
gameFileInfo.walls.count		=	0;
gameFileInfo.walls.size			=	sizeof(tWall);  
gameFileInfo.doors.offset		=	-1;
gameFileInfo.doors.count		=	0;
gameFileInfo.doors.size			=	sizeof(tActiveDoor);  
gameFileInfo.triggers.offset	=	-1;
gameFileInfo.triggers.count	=	0;
gameFileInfo.triggers.size		=	sizeof(tTrigger);  
gameFileInfo.control.offset	=	-1;
gameFileInfo.control.count		=	0;
gameFileInfo.control.size		=	sizeof(tReactorTriggers);
gameFileInfo.botGen.offset		=	-1;
gameFileInfo.botGen.count		=	0;
gameFileInfo.botGen.size		=	sizeof(tMatCenInfo);
gameFileInfo.equipGen.offset	=	-1;
gameFileInfo.equipGen.count	=	0;
gameFileInfo.equipGen.size		=	sizeof(tMatCenInfo);
gameFileInfo.lightDeltaIndices.offset = -1;
gameFileInfo.lightDeltaIndices.count =	0;
gameFileInfo.lightDeltaIndices.size =	sizeof(tLightDeltaIndex);
gameFileInfo.lightDeltas.offset	=	-1;
gameFileInfo.lightDeltas.count	=	0;
gameFileInfo.lightDeltas.size		=	sizeof(tLightDelta);
}

// -----------------------------------------------------------------------------

static int ReadGameFileInfo (CFILE *cfP, int nStartOffset)
{
gameTopFileInfo.fileinfo_signature = CFReadShort (cfP);
gameTopFileInfo.fileinfoVersion = CFReadShort (cfP);
gameTopFileInfo.fileinfo_sizeof = CFReadInt (cfP);
// Check signature
if (gameTopFileInfo.fileinfo_signature != 0x6705)
	return -1;
// Check version number
if (gameTopFileInfo.fileinfoVersion < GAME_COMPATIBLE_VERSION)
	return -1;
// Now, Read in the fileinfo
if (CFSeek (cfP, nStartOffset, SEEK_SET)) 
	Error ("Error seeking to gameFileInfo in gamesave.c");
gameFileInfo.fileinfo_signature = CFReadShort (cfP);
gameFileInfo.fileinfoVersion = CFReadShort (cfP);
gameFileInfo.fileinfo_sizeof = CFReadInt (cfP);
CFRead (gameFileInfo.mine_filename, sizeof (char), 15, cfP);
gameFileInfo.level = CFReadInt (cfP);
gameFileInfo.player.offset = CFReadInt (cfP);				// Player info
gameFileInfo.player.size = CFReadInt (cfP);
gameFileInfo.objects.offset = CFReadInt (cfP);				// Object info
gameFileInfo.objects.count = CFReadInt (cfP);    
gameFileInfo.objects.size = CFReadInt (cfP);  
gameFileInfo.walls.offset = CFReadInt (cfP);
gameFileInfo.walls.count = CFReadInt (cfP);
gameFileInfo.walls.size = CFReadInt (cfP);
gameFileInfo.doors.offset = CFReadInt (cfP);
gameFileInfo.doors.count = CFReadInt (cfP);
gameFileInfo.doors.size = CFReadInt (cfP);
gameFileInfo.triggers.offset = CFReadInt (cfP);
gameFileInfo.triggers.count = CFReadInt (cfP);
gameFileInfo.triggers.size = CFReadInt (cfP);
gameFileInfo.links.offset = CFReadInt (cfP);
gameFileInfo.links.count = CFReadInt (cfP);
gameFileInfo.links.size = CFReadInt (cfP);
gameFileInfo.control.offset = CFReadInt (cfP);
gameFileInfo.control.count = CFReadInt (cfP);
gameFileInfo.control.size = CFReadInt (cfP);
gameFileInfo.botGen.offset = CFReadInt (cfP);
gameFileInfo.botGen.count = CFReadInt (cfP);
gameFileInfo.botGen.size = CFReadInt (cfP);
if (gameTopFileInfo.fileinfoVersion >= 29) {
	gameFileInfo.lightDeltaIndices.offset = CFReadInt (cfP);
	gameFileInfo.lightDeltaIndices.count = CFReadInt (cfP);
	gameFileInfo.lightDeltaIndices.size = CFReadInt (cfP);

	gameFileInfo.lightDeltas.offset = CFReadInt (cfP);
	gameFileInfo.lightDeltas.count = CFReadInt (cfP);
	gameFileInfo.lightDeltas.size = CFReadInt (cfP);
	}
if (gameData.segs.nLevelVersion >= 17) {
	gameFileInfo.equipGen.offset = CFReadInt (cfP);
	gameFileInfo.equipGen.count = CFReadInt (cfP);
	gameFileInfo.equipGen.size = CFReadInt (cfP);
	}
return 0;
}

// -----------------------------------------------------------------------------

static int ReadLevelInfo (CFILE *cfP)
{
if (gameTopFileInfo.fileinfoVersion >= 31) { //load mine filename
	// read newline-terminated string, not sure what version this changed.
	CFGetS (gameData.missions.szCurrentLevel, sizeof (gameData.missions.szCurrentLevel), cfP);

	if (gameData.missions.szCurrentLevel [strlen (gameData.missions.szCurrentLevel) - 1] == '\n')
		gameData.missions.szCurrentLevel [strlen (gameData.missions.szCurrentLevel) - 1] = 0;
}
else if (gameTopFileInfo.fileinfoVersion >= 14) { //load mine filename
	// read null-terminated string
	char *p = gameData.missions.szCurrentLevel;
	//must do read one char at a time, since no CFGetS()
	do {
		*p = CFGetC (cfP);
		} while (*p++);
}
else
	gameData.missions.szCurrentLevel [0] = 0;
if (gameTopFileInfo.fileinfoVersion >= 19) {	//load pof names
	nSavePOFNames = CFReadShort (cfP);
	if ((nSavePOFNames != 0x614d) && (nSavePOFNames != 0x5547)) { // "Ma"de w/DMB beta/"GU"ILE
		if (nSavePOFNames >= MAX_POLYGON_MODELS)
			return -1;
		CFRead (szSavePOFNames, nSavePOFNames, SHORT_FILENAME_LEN, cfP);
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static int ReadObjectInfo (CFILE *cfP)
{
	int	i;

if (gameFileInfo.objects.offset > -1) {
	tObject	*objP = OBJECTS;
	if (CFSeek (cfP, gameFileInfo.objects.offset, SEEK_SET)) {
		Error ("Error seeking to object data\n(file damaged or invalid)");
		return -1;
		}
	memset (OBJECTS, 0, gameFileInfo.objects.count * sizeof (tObject));
	for (i = 0; i < gameFileInfo.objects.count; i++, objP++) {
		ReadObject (objP, cfP, gameTopFileInfo.fileinfoVersion);
		objP->info.nSignature = gameData.objs.nNextSignature++;
		VerifyObject (objP);
		gameData.objs.init [i] = *objP;
		}
	}
for (i = 0; i < MAX_OBJECTS - 1; i++)
	gameData.objs.dropInfo [i].nNextPowerup = i + 1;
gameData.objs.dropInfo [i].nNextPowerup = -1;
gameData.objs.nFirstDropped =
gameData.objs.nLastDropped = -1;
gameData.objs.nFreeDropped = 0;
return 0;
}

// -----------------------------------------------------------------------------

static int ReadWallInfo (CFILE *cfP)
{
if (gameFileInfo.walls.offset > -1) {
	int	i;
	if (CFSeek (cfP, gameFileInfo.walls.offset, SEEK_SET)) {
		Error ("Error seeking to wall data\n(file damaged or invalid)");
		return -1;
		}
	for (i = 0; i <gameFileInfo.walls.count; i++) {
		if (gameTopFileInfo.fileinfoVersion >= 20)
			ReadWall(&gameData.walls.walls [i], cfP); // v20 walls and up.
		else if (gameTopFileInfo.fileinfoVersion >= 17) {
			tWallV19 w;

			ReadWallV19(&w, cfP);
			gameData.walls.walls [i].nSegment	   = w.nSegment;
			gameData.walls.walls [i].nSide			= w.nSide;
			gameData.walls.walls [i].nLinkedWall	= w.nLinkedWall;
			gameData.walls.walls [i].nType			= w.nType;
			gameData.walls.walls [i].flags			= w.flags;
			gameData.walls.walls [i].hps				= w.hps;
			gameData.walls.walls [i].nTrigger		= w.nTrigger;
			gameData.walls.walls [i].nClip			= w.nClip;
			gameData.walls.walls [i].keys				= w.keys;
			gameData.walls.walls [i].state			= WALL_DOOR_CLOSED;
			}
		else {
			tWallV16 w;

			ReadWallV16(&w, cfP);
			gameData.walls.walls [i].nSegment = gameData.walls.walls [i].nSide = gameData.walls.walls [i].nLinkedWall = -1;
			gameData.walls.walls [i].nType		= w.nType;
			gameData.walls.walls [i].flags		= w.flags;
			gameData.walls.walls [i].hps			= w.hps;
			gameData.walls.walls [i].nTrigger	= w.nTrigger;
			gameData.walls.walls [i].nClip		= w.nClip;
			gameData.walls.walls [i].keys			= w.keys;
			}
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static int ReadDoorInfo (CFILE *cfP)
{
if (gameFileInfo.doors.offset > -1) {
	int	i;
	if (CFSeek (cfP, gameFileInfo.doors.offset, SEEK_SET))	{
		Error ("Error seeking to door data\n(file damaged or invalid)");
		return -1;
		}
	for (i = 0; i < gameFileInfo.doors.count; i++) {
		if (gameTopFileInfo.fileinfoVersion >= 20)
			ReadActiveDoor (&gameData.walls.activeDoors [i], cfP); // version 20 and up
		else {
			v19_door d;
			int p;
			short nConnSeg, nConnSide;

			ReadActiveDoorV19(&d, cfP);
			gameData.walls.activeDoors [i].nPartCount = d.nPartCount;
			for (p = 0; p < d.nPartCount; p++) {
				nConnSeg = gameData.segs.segments [d.seg [p]].children [d.nSide [p]];
				nConnSide = FindConnectedSide(gameData.segs.segments + d.seg [p], gameData.segs.segments + nConnSeg);
				gameData.walls.activeDoors [i].nFrontWall [p] = WallNumI (d.seg [p], d.nSide [p]);
				gameData.walls.activeDoors [i].nBackWall [p] = WallNumI (nConnSeg, nConnSide);
				}
			}
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static int ReadTriggerInfo (CFILE *cfP)
{
	int		h, i, j;
	tTrigger	*trigP;

if (gameFileInfo.triggers.offset > -1) {
#if TRACE
	con_printf(CONDBG, "   loading tTrigger data ...\n");
#endif
	if (CFSeek (cfP, gameFileInfo.triggers.offset, SEEK_SET)) {
		Error ("Error seeking to trigger data\n(file damaged or invalid)");
		return -1;
		}
	for (i = 0, trigP = gameData.trigs.triggers; i < gameFileInfo.triggers.count; i++, trigP++) {
		if (gameTopFileInfo.fileinfoVersion >= 31) 
			TriggerRead (trigP, cfP, 0);
		else {
			tTriggerV30 trig;
			int t, nType = 0, flags = 0;
			if (gameTopFileInfo.fileinfoVersion == 30)
				V30TriggerRead (&trig, cfP);
			else {
				tTriggerV29 trig29;
				V29TriggerRead (&trig29, cfP);
				trig.flags = trig29.flags;
				trig.nLinks	= (char) trig29.nLinks;
				trig.value = trig29.value;
				trig.time = trig29.time;
				for (t = 0; t < trig.nLinks; t++) {
					trig.nSegment [t] = trig29.nSegment [t];
					trig.nSide [t] = trig29.nSide [t];
					}
				}
			//Assert(trig.flags & TRIGGER_ON);
			trig.flags &= ~TRIGGER_ON;
			if (trig.flags & TRIGGER_CONTROL_DOORS)
				nType = TT_OPEN_DOOR;
			else if (trig.flags & TRIGGER_SHIELD_DAMAGE)
				nType = TT_SHIELD_DAMAGE;
			else if (trig.flags & TRIGGER_ENERGY_DRAIN)
				nType = TT_ENERGY_DRAIN;
			else if (trig.flags & TRIGGER_EXIT)
				nType = TT_EXIT;
			else if (trig.flags & TRIGGER_MATCEN)
				nType = TT_MATCEN;
			else if (trig.flags & TRIGGER_ILLUSION_OFF)
				nType = TT_ILLUSION_OFF;
			else if (trig.flags & TRIGGER_SECRET_EXIT)
				nType = TT_SECRET_EXIT;
			else if (trig.flags & TRIGGER_ILLUSION_ON)
				nType = TT_ILLUSION_ON;
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

			trigP->nType = nType;
			trigP->flags = flags;
			trigP->nLinks = trig.nLinks;
			trigP->nLinks = trig.nLinks;
			trigP->value = trig.value;
			trigP->time = trig.time;
			for (t = 0; t < trig.nLinks; t++) {
				trigP->nSegment [t] = trig.nSegment [t];
				trigP->nSide [t] = trig.nSide [t];
				}
			}
		if (trigP->nLinks < 0)
			trigP->nLinks = 0;
		else if (trigP->nLinks > MAX_TRIGGER_TARGETS)
			trigP->nLinks = MAX_TRIGGER_TARGETS;
		for (h = trigP->nLinks, j = 0; j < h; ) {
			if ((trigP->nSegment [j] >= 0) && (trigP->nSegment [j] < gameData.segs.nSegments) &&
				 (trigP->nSide [j] >= 0) && (trigP->nSide [j] < 6))
				j++;
			else if (--h) {
				trigP->nSegment [j] = trigP->nSegment [h];
				trigP->nSide [j] = trigP->nSide [h];
				}
			}
		trigP->nLinks = h;
		}
	if (gameTopFileInfo.fileinfoVersion >= 33) {
		gameData.trigs.nObjTriggers = CFReadInt (cfP);
		if (gameData.trigs.nObjTriggers) {
			for (i = 0; i < gameData.trigs.nObjTriggers; i++)
				TriggerRead (gameData.trigs.objTriggers + i, cfP, 1);
			for (i = 0; i < gameData.trigs.nObjTriggers; i++) {
				gameData.trigs.objTriggerRefs [i].prev = CFReadShort (cfP);
				gameData.trigs.objTriggerRefs [i].next = CFReadShort (cfP);
				gameData.trigs.objTriggerRefs [i].nObject = CFReadShort (cfP);
				}
			}
		if (gameTopFileInfo.fileinfoVersion < 36) {
			for (i = 0; i < 700; i++)
				gameData.trigs.firstObjTrigger [i] = CFReadShort (cfP);
			}
		else {
			memset (gameData.trigs.firstObjTrigger, 0xff, sizeof (gameData.trigs.firstObjTrigger));
			for (i = CFReadShort (cfP); i; i--) {
				j = CFReadShort (cfP);
				gameData.trigs.firstObjTrigger [j] = CFReadShort (cfP);
				}
			}
		}
	else {
		gameData.trigs.nObjTriggers = 0;
		memset (gameData.trigs.objTriggers, 0, sizeof (tTrigger) * MAX_OBJ_TRIGGERS);
		memset (gameData.trigs.objTriggerRefs, 0xff, sizeof (tObjTriggerRef) * MAX_OBJ_TRIGGERS);
		memset (gameData.trigs.firstObjTrigger, 0xff, sizeof (gameData.trigs.firstObjTrigger));
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static int ReadReactorInfo (CFILE *cfP)
{
if (gameFileInfo.control.offset > -1) {
#if TRACE
	con_printf(CONDBG, "   loading reactor data ...\n");
#endif
	if (CFSeek (cfP, gameFileInfo.control.offset, SEEK_SET)) {
		Error ("Error seeking to reactor data\n(file damaged or invalid)");
		return -1;
		}
	ControlCenterTriggersReadN (&gameData.reactor.triggers, gameFileInfo.control.count, cfP);
	}
return 0;
}

// -----------------------------------------------------------------------------

static int ReadBotGenInfo (CFILE *cfP)
{
if (gameFileInfo.botGen.offset > -1) {
	int	i, j;

	if (CFSeek (cfP, gameFileInfo.botGen.offset, SEEK_SET)) {
		Error ("Error seeking to robot generator data\n(file damaged or invalid)");
		return -1;
		}
	for (i = 0; i < gameFileInfo.botGen.count; i++) {
		if (gameTopFileInfo.fileinfoVersion < 27) {
			old_tMatCenInfo m;

			OldMatCenInfoRead (&m, cfP);

			gameData.matCens.botGens [i].objFlags [0] = m.objFlags;
			gameData.matCens.botGens [i].objFlags [1] = 0;
			gameData.matCens.botGens [i].xHitPoints = m.xHitPoints;
			gameData.matCens.botGens [i].xInterval = m.xInterval;
			gameData.matCens.botGens [i].nSegment = m.nSegment;
			gameData.matCens.botGens [i].nFuelCen = m.nFuelCen;
		}
		else
			MatCenInfoRead (gameData.matCens.botGens + i, cfP);

		//	Set links in gameData.matCens.botGens to gameData.matCens.fuelCenters array
		for (j = 0; j <= gameData.segs.nLastSegment; j++)
			if ((gameData.segs.segment2s [j].special == SEGMENT_IS_ROBOTMAKER) &&
					(gameData.segs.segment2s [j].nMatCen == i)) {
				gameData.matCens.botGens [i].nFuelCen = gameData.segs.segment2s [j].value;
				break;
				}
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static int ReadEquipGenInfo (CFILE *cfP)
{
if (gameFileInfo.equipGen.offset > -1) {
	int	i, j;

	if (CFSeek (cfP, gameFileInfo.equipGen.offset, SEEK_SET)) {
		Error ("Error seeking to equipment generator data\n(file damaged or invalid)");
		return -1;
		}
	for (i = 0; i < gameFileInfo.equipGen.count; i++) {
		MatCenInfoRead (gameData.matCens.equipGens + i, cfP);
		//	Set links in gameData.matCens.botGens to gameData.matCens.fuelCenters array
		for (j = 0; j <= gameData.segs.nLastSegment; j++)
			if ((gameData.segs.segment2s [j].special == SEGMENT_IS_EQUIPMAKER) &&
					(gameData.segs.segment2s [j].nMatCen == i))
				gameData.matCens.equipGens [i].nFuelCen = gameData.segs.segment2s [j].value;
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static int ReadlightDeltaIndexInfo (CFILE *cfP)
{
if (gameFileInfo.lightDeltaIndices.offset > -1) {
	int	i;

	if (CFSeek (cfP, gameFileInfo.lightDeltaIndices.offset, SEEK_SET)) {
		Error ("Error seeking to light delta index data\n(file damaged or invalid)");
		return -1;
		}
	gameData.render.lights.nStatic = gameFileInfo.lightDeltaIndices.count;
	if (gameTopFileInfo.fileinfoVersion < 29) {
#if TRACE
		con_printf (CONDBG, "Warning: Old mine version.  Not reading gameData.render.lights.deltaIndices info.\n");
#endif
		Int3();	//shouldn't be here!!!
		return 0;
		}
	else {
		for (i = 0; i < gameFileInfo.lightDeltaIndices.count; i++) {
			//PrintLog ("reading DL index %d\n", i);
			ReadlightDeltaIndex (gameData.render.lights.deltaIndices + i, cfP);
			}
		}
	}
SortDLIndex ();
return 0;
}

// -----------------------------------------------------------------------------

static int ReadlightDeltaInfo (CFILE *cfP)
{
if (gameFileInfo.lightDeltas.offset > -1) {
	int	i;

#if TRACE
	con_printf(CONDBG, "   loading light data ...\n");
#endif
	if (CFSeek (cfP, gameFileInfo.lightDeltas.offset, SEEK_SET)) {
		Error ("Error seeking to light delta data\n(file damaged or invalid)");
		return -1;
		}
	for (i = 0; i < gameFileInfo.lightDeltas.count; i++) {
		if (gameTopFileInfo.fileinfoVersion >= 29) 
			ReadlightDelta (gameData.render.lights.deltas + i, cfP);
		else {
#if TRACE
			con_printf (CONDBG, "Warning: Old mine version.  Not reading delta light info.\n");
#endif
			}
		}
	}
return 0;
}

// -----------------------------------------------------------------------------

static void CheckAndLinkObjects (void)
{
	int		i, nObjSeg;
	tObject	*objP = OBJECTS;

for (i = 0; i < gameFileInfo.objects.count; i++, objP++) {
	objP->info.nNextInSeg = OBJECTS [i].info.nPrevInSeg = -1;
	if (objP->info.nType != OBJ_NONE) {
		nObjSeg = objP->info.nSegment;
		if ((nObjSeg < 0) || (nObjSeg > gameData.segs.nLastSegment))	
			objP->info.nType = OBJ_NONE;
		else {
			objP->info.nSegment = -1;	
			LinkObjToSeg (i, nObjSeg);
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Make sure non-transparent doors are set correctly.
static void CheckAndFixDoors (void)
{
	int	i, j;
	tSide	*sideP;

for (i = 0; i < gameData.segs.nSegments; i++) {
	sideP = gameData.segs.segments [i].sides;
	for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++, sideP++) {
		short nWall = WallNumS (sideP);
		tWall  *w;
		if (!IS_WALL (nWall))
			continue;
		w = gameData.walls.walls + nWall;
		if (w->nClip == -1)
			continue;
		if (gameData.walls.pAnims [w->nClip].flags & WCF_TMAP1) {
			sideP->nBaseTex = gameData.walls.pAnims [w->nClip].frames [0];
			sideP->nOvlTex = 0;
			}
		}
	}
}

// -----------------------------------------------------------------------------
//go through all walls, killing references to invalid triggers
static void CheckAndFixWalls (void)
{
	int	i;
	short	nSegment, nSide, nWall;

for (i = 0; i < gameData.walls.nWalls; i++)
	if (gameData.walls.walls [i].nTrigger >= gameData.trigs.nTriggers) {
#if TRACE
		con_printf (CONDBG,"Removing reference to invalid tTrigger %d from tWall %d\n",gameData.walls.walls [i].nTrigger,i);
#endif
		gameData.walls.walls [i].nTrigger = NO_TRIGGER;	//kill tTrigger
		}
if (gameTopFileInfo.fileinfoVersion < 17) {
	for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++)
		for (nSide = 0; nSide < 6; nSide++)
			if (IS_WALL (nWall = WallNumI (nSegment, nSide))) {
				gameData.walls.walls [nWall].nSegment = nSegment;
				gameData.walls.walls [nWall].nSide = nSide;
				}
	}

#if DBG
for (nSide = 0; nSide < 6; nSide++) {
	nWall = WallNumI (gameData.segs.nLastSegment, nSide);
	if (IS_WALL (nWall) &&
			((gameData.walls.walls [nWall].nSegment != gameData.segs.nLastSegment) || 
			(gameData.walls.walls [nWall].nSide != nSide))) {
			Int3();	//	Error.  Bogus walls in this segment.
		}
	}
#endif
}

// -----------------------------------------------------------------------------
//go through all triggers, killing unused ones
static void CheckAndFixTriggers (void)
{
	int	i, j;
	short	nSegment, nSide, nWall;

for (i = 0; i < gameData.trigs.nTriggers; ) {
	//	Find which tWall this tTrigger is connected to.
	for (j = 0; j < gameData.walls.nWalls; j++)
		if (gameData.walls.walls [j].nTrigger == i)
			break;
#ifdef EDITOR
	if (j == gameData.walls.nWalls) {
#if TRACE
		con_printf (CONDBG,"Removing unreferenced tTrigger %d\n",i);
#endif
		RemoveTriggerNum (i);
		}
	else
#endif
		i++;
	}

for (i = 0; i < gameData.walls.nWalls; i++)
	gameData.walls.walls [i].controllingTrigger = -1;

//	MK, 10/17/95: Make walls point back at the triggers that control them.
//	Go through all triggers, stuffing controllingTrigger field in gameData.walls.walls.

for (i = 0; i < gameData.trigs.nTriggers; i++) {
	for (j = 0; j < gameData.trigs.triggers [i].nLinks; j++) {
		nSegment = gameData.trigs.triggers [i].nSegment [j];
		nSide = gameData.trigs.triggers [i].nSide [j];
		nWall = WallNumI (nSegment, nSide);
		//check to see that if a tTrigger requires a tWall that it has one,
		//and if it requires a botGen that it has one
		if (gameData.trigs.triggers [i].nType == TT_MATCEN) {
			if (gameData.segs.segment2s [nSegment].special != SEGMENT_IS_ROBOTMAKER)
				continue;		//botGen tTrigger doesn'i point to botGen
			}
		else if ((gameData.trigs.triggers [i].nType != TT_LIGHT_OFF) && 
					(gameData.trigs.triggers [i].nType != TT_LIGHT_ON)) { //light triggers don't require walls
			if (IS_WALL (nWall))
				gameData.walls.walls [nWall].controllingTrigger = i;
			else {
				Int3();	//	This is illegal.  This ttrigger requires a tWall
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Load game 
// Loads all the relevant data for a level.
// If level != -1, it loads the filename with extension changed to .min
// Otherwise it loads the appropriate level mine.
// returns 0=everything ok, 1=old version, -1=error
int LoadMineDataCompiled (CFILE *cfP, int bFileInfo)
{
	int 	nStartOffset;

nStartOffset = CFTell (cfP);
InitGameFileInfo ();
if (ReadGameFileInfo (cfP, nStartOffset))
	return -1;
if (ReadLevelInfo (cfP))
	return -1;
gameStates.render.bD2XLights = gameStates.app.bD2XLevel && (gameTopFileInfo.fileinfoVersion >= 34);
if (bFileInfo)
	return 0;

gameData.objs.nNextSignature = 0;
gameData.render.lights.nStatic = 0;
nGameSaveOrgRobots = 0;
nGameSavePlayers = 0;
if (ReadObjectInfo (cfP))
	return -1;
if (ReadWallInfo (cfP))
	return -1;
if (ReadDoorInfo (cfP))
	return -1;
if (ReadTriggerInfo (cfP))
	return -1;
if (ReadReactorInfo (cfP))
	return -1;
if (ReadBotGenInfo (cfP))
	return -1;
if (ReadEquipGenInfo (cfP))
	return -1;
if (ReadlightDeltaIndexInfo (cfP))
	return -1;
if (ReadlightDeltaInfo (cfP))
	return -1;
ClearLightSubtracted ();
ResetObjects (gameFileInfo.objects.count);
CheckAndLinkObjects ();
ClearTransientObjects (1);		//1 means clear proximity bombs
CheckAndFixDoors ();
gameData.walls.nWalls = gameFileInfo.walls.count;
ResetWalls ();
gameData.walls.nOpenDoors = gameFileInfo.doors.count;
gameData.trigs.nTriggers = gameFileInfo.triggers.count;
CheckAndFixWalls ();
CheckAndFixTriggers ();
gameData.matCens.nBotCenters = gameFileInfo.botGen.count;
FixObjectSegs ();
#if DBG
dump_mine_info ();
#endif
if ((gameTopFileInfo.fileinfoVersion < GAME_VERSION) && 
	 ((gameTopFileInfo.fileinfoVersion != 25) || (GAME_VERSION != 26)))
	return 1;		//means old version
return 0;
}

// ----------------------------------------------------------------------------

int CheckSegmentConnections(void);

extern void	SetAmbientSoundFlags(void);


#define LEVEL_FILE_VERSION      8
//1 -> 2  add palette name
//2 -> 3  add control center explosion time
//3 -> 4  add reactor strength
//4 -> 5  killed hostage text stuff
//5 -> 6  added gameData.segs.secret.nReturnSegment and gameData.segs.secret.returnOrient
//6 -> 7  added flickering lights
//7 -> 8  made version 8 to be not compatible with D2 1.0 & 1.1

#if DBG
char *Level_being_loaded=NULL;
#endif

int no_oldLevel_file_error=0;

// ----------------------------------------------------------------------------
//loads a level (.LVL) file from disk
//returns 0 if success, else error code
int LoadLevelSub (char * pszFilename, int nLevel)
{
#ifdef EDITOR
	int bUseCompiledLevel = 1;
#endif
	CFILE cf;
	char filename [128];
	int sig, nMineDataOffset, nGameDataOffset;
	int nError;
	//int i;

SetDataVersion (-1);
gameData.segs.bHaveSlideSegs = 0;
if (gameData.app.nGameMode & GM_NETWORK) {
	memset (gameData.multiplayer.maxPowerupsAllowed, 0, sizeof (gameData.multiplayer.maxPowerupsAllowed));
	memset (gameData.multiplayer.powerupsInMine, 0, sizeof (gameData.multiplayer.powerupsInMine));
	}
#if DBG
Level_being_loaded = pszFilename;
#endif

gameStates.render.nMeshQuality = gameOpts->render.nMeshQuality;

reloadLevel:

strcpy (filename, pszFilename);
if (!CFOpen (&cf, filename, "", "rb", gameStates.app.bD1Mission))
	return 1;

strcpy(gameData.segs.szLevelFilename, filename);

//	#ifdef NEWDEMO
//	if (gameData.demo.nState == ND_STATE_RECORDING)
//		NDRecordStartDemo();
//	#endif

sig = CFReadInt (&cf);
gameData.segs.nLevelVersion = CFReadInt (&cf);
gameStates.app.bD2XLevel = (gameData.segs.nLevelVersion >= 10);
#if TRACE
con_printf (CONDBG, "gameData.segs.nLevelVersion = %d\n", gameData.segs.nLevelVersion);
#endif
nMineDataOffset = CFReadInt (&cf);
nGameDataOffset = CFReadInt (&cf);

Assert(sig == MAKE_SIG('P','L','V','L'));
if (gameData.segs.nLevelVersion >= 8) {    //read dummy data
	CFReadInt (&cf);
	CFReadShort (&cf);
	CFReadByte(&cf);
}

if (gameData.segs.nLevelVersion < 5)
	CFReadInt (&cf);       //was hostagetext_offset

if (gameData.segs.nLevelVersion > 1) {
	CFGetS (szCurrentLevelPalette, sizeof (szCurrentLevelPalette), &cf);
	if (szCurrentLevelPalette [strlen(szCurrentLevelPalette) - 1] == '\n')
		szCurrentLevelPalette [strlen(szCurrentLevelPalette) - 1] = 0;
}
if ((gameData.segs.nLevelVersion <= 1) || (szCurrentLevelPalette [0] == 0)) // descent 1 level
	strcpy (szCurrentLevelPalette, DEFAULT_LEVEL_PALETTE); //D1_PALETTE

if (gameData.segs.nLevelVersion >= 3)
	gameStates.app.nBaseCtrlCenExplTime = CFReadInt (&cf);
else
	gameStates.app.nBaseCtrlCenExplTime = DEFAULT_CONTROL_CENTER_EXPLOSION_TIME;

if (gameData.segs.nLevelVersion >= 4)
	gameData.reactor.nStrength = CFReadInt (&cf);
else
	gameData.reactor.nStrength = -1;  //use old defaults

if (gameData.segs.nLevelVersion >= 7) {
	int i;

#if TRACE
con_printf (CONDBG, "   loading dynamic lights ...\n");
#endif
gameData.render.lights.flicker.nLights = CFReadInt (&cf);
Assert ((gameData.render.lights.flicker.nLights >= 0) && (gameData.render.lights.flicker.nLights < MAX_FLICKERING_LIGHTS));
for (i = 0; i < gameData.render.lights.flicker.nLights; i++)
	ReadVariableLight (&gameData.render.lights.flicker.lights [i], &cf);
}
else
	gameData.render.lights.flicker.nLights = 0;

if (gameData.segs.nLevelVersion < 6) {
	gameData.segs.secret.nReturnSegment = 0;
	gameData.segs.secret.returnOrient[RVEC][Y] =
	gameData.segs.secret.returnOrient[RVEC][Z] = 
	gameData.segs.secret.returnOrient[FVEC][X] =
	gameData.segs.secret.returnOrient[FVEC][Z] =
	gameData.segs.secret.returnOrient[UVEC][X] =
	gameData.segs.secret.returnOrient[UVEC][Y] = 0;
	gameData.segs.secret.returnOrient[RVEC][X] =
	gameData.segs.secret.returnOrient[FVEC][Y] =
	gameData.segs.secret.returnOrient[UVEC][Z] = F1_0;
	}
else {
	gameData.segs.secret.nReturnSegment = CFReadInt (&cf);
	gameData.segs.secret.returnOrient[RVEC][X] = CFReadInt (&cf);
	gameData.segs.secret.returnOrient[RVEC][Y] = CFReadInt (&cf);
	gameData.segs.secret.returnOrient[RVEC][Z] = CFReadInt (&cf);
	gameData.segs.secret.returnOrient[FVEC][X] = CFReadInt (&cf);
	gameData.segs.secret.returnOrient[FVEC][Y] = CFReadInt (&cf);
	gameData.segs.secret.returnOrient[FVEC][Z] = CFReadInt (&cf);
	gameData.segs.secret.returnOrient[UVEC][X] = CFReadInt (&cf);
	gameData.segs.secret.returnOrient[UVEC][Y] = CFReadInt (&cf);
	gameData.segs.secret.returnOrient[UVEC][Z] = CFReadInt (&cf);
	}

//NOTE LINK TO ABOVE!!
CFSeek (&cf, nGameDataOffset, SEEK_SET);
nError = LoadMineDataCompiled (&cf, 1);
CFSeek (&cf, nMineDataOffset, SEEK_SET);
nError = LoadMineSegmentsCompiled (&cf);
if (nError == -1) {   //error!!
	CFClose(&cf);
	return 2;
	}
CFSeek (&cf, nGameDataOffset, SEEK_SET);
nError = LoadMineDataCompiled (&cf, 0);
if (nError == -1) {   //error!!
	CFClose(&cf);
	return 3;
	}
CFClose(&cf);

if (!meshBuilder.Build (nLevel))
	goto reloadLevel;

if (!gameStates.app.bNostalgia) {
#if !SHADOWS
	if (SHOW_DYN_LIGHT || !gameStates.app.bD2XLevel)
#endif
		{
		AddDynGeometryLights ();
		ComputeNearestLights (nLevel);
		if (gameStates.render.bPerPixelLighting) {
			CreateLightmaps (nLevel);
			if (HaveLightmaps ())
				meshBuilder.RebuildLightmapTexCoord ();	//rebuild to create proper lightmap texture coordinates
			else
				gameOpts->render.bUseLightmaps = 0;
			}
		}
	}

SetAmbientSoundFlags ();
#ifdef EDITOR
write_game_text_file(filename);
if (Errors_in_mine) {
	if (IsRealLevel(filename)) {
		char  ErrorMessage [200];

		sprintf(ErrorMessage, TXT_MINE_ERRORS, Errors_in_mine, Level_being_loaded);
		StopTime();
		GrPaletteStepLoad (NULL);
		ExecMessageBox(NULL, 1, TXT_CONTINUE, ErrorMessage);
		StartTime();
	} else {
#if TRACE
		con_printf (1, TXT_MINE_ERRORS, Errors_in_mine, Level_being_loaded);
#endif
	}
}
#endif

#ifdef EDITOR
//If an old version, ask the use if he wants to save as new version
if (!no_oldLevel_file_error && (gameStates.app.nFunctionMode == FMODE_EDITOR) && 
    (((LEVEL_FILE_VERSION > 3) && gameData.segs.nLevelVersion < LEVEL_FILE_VERSION) || nError == 1 || nError == 1)) {
	char  ErrorMessage [200];

	sprintf(ErrorMessage,
				"You just loaded a old version\n"
				"level.  Would you like to save\n"
				"it as a current version level?");

	StopTime();
	GrPaletteStepLoad (NULL);
	if (ExecMessageBox(NULL, 2, "Don't Save", "Save", ErrorMessage)==1)
		SaveLevel(filename);
	StartTime();
}
#endif

#ifdef EDITOR
if (gameStates.app.nFunctionMode == FMODE_EDITOR)
	editor_status("Loaded NEW mine %s, \"%s\"",filename,gameData.missions.szCurrentLevel);
#endif

#ifdef EDITOR
if (CheckSegmentConnections())
	ExecMessageBox("ERROR", 1, "Ok", 
			"Connectivity errors detected in\n"
			"mine.  See monochrome screen for\n"
			"details, and contact Matt or Mike.");
#endif

return 0;
}

#ifdef EDITOR
void GetLevelName()
{
//NO_UI!!!	UI_WINDOW 				*NameWindow = NULL;
//NO_UI!!!	UI_GADGET_INPUTBOX	*NameText;
//NO_UI!!!	UI_GADGET_BUTTON 		*QuitButton;
//NO_UI!!!
//NO_UI!!!	// Open a window with a quit button
//NO_UI!!!	NameWindow = ui_open_window(20, 20, 300, 110, WIN_DIALOG);
//NO_UI!!!	QuitButton = ui_add_gadget_button(NameWindow, 150-24, 60, 48, 40, "Done", NULL);
//NO_UI!!!
//NO_UI!!!	ui_wprintf_at(NameWindow, 10, 12,"Please enter a name for this mine:");
//NO_UI!!!	NameText = ui_add_gadget_inputbox(NameWindow, 10, 30, LEVEL_NAME_LEN, LEVEL_NAME_LEN, gameData.missions.szCurrentLevel);
//NO_UI!!!
//NO_UI!!!	NameWindow->keyboard_focus_gadget = (UI_GADGET *)NameText;
//NO_UI!!!	QuitButton->hotkey = KEY_ENTER;
//NO_UI!!!
//NO_UI!!!	ui_gadget_calc_keys(NameWindow);
//NO_UI!!!
//NO_UI!!!	while (!QuitButton->pressed && last_keypress!=KEY_ENTER) {
//NO_UI!!!		ui_mega_process();
//NO_UI!!!		ui_window_do_gadgets(NameWindow);
//NO_UI!!!	}
//NO_UI!!!
//NO_UI!!!	strcpy(gameData.missions.szCurrentLevel, NameText->text);
//NO_UI!!!
//NO_UI!!!	if (NameWindow!=NULL)	{
//NO_UI!!!		ui_close_window(NameWindow);
//NO_UI!!!		NameWindow = NULL;
//NO_UI!!!	}
//NO_UI!!!

	tMenuItem m [2];

	memset (m, 0, sizeof (m));
	m [0].nType = NM_TYPE_TEXT; 
	m [0].text = "Please enter a name for this mine:";
	m [1].nType = NM_TYPE_INPUT; 
	m [1].text = gameData.missions.szCurrentLevel; 
	m [1].text_len = LEVEL_NAME_LEN;

	ExecMenu(NULL, "Enter mine name", 2, m, NULL);

}
#endif


#ifdef EDITOR

int	Errors_in_mine;

// -----------------------------------------------------------------------------

int CountDeltaLightRecords(void)
{
	int	i;
	int	total = 0;

	for (i=0; i<gameData.render.lights.nStatic; i++) {
		total += gameData.render.lights.deltaIndices [i].count;
	}

	return total;

}

// -----------------------------------------------------------------------------
// Save game
int SaveGameData(FILE * SaveFile)
{
	int  player.offset, tObject.offset, walls.offset, doors.offset, triggers.offset, control.offset, botGen.offset; //, links.offset;
	int	gameData.render.lights.deltaIndices.offset, deltaLight.offset;
	int start_offset,end_offset;

	start_offset = ftell(SaveFile);

	//===================== SAVE FILE INFO ========================

	gameFileInfo.fileinfo_signature =	0x6705;
	gameFileInfo.fileinfoVersion	=	GAME_VERSION;
	gameFileInfo.level					=  gameData.missions.nCurrentLevel;
	gameFileInfo.fileinfo_sizeof		=	sizeof(gameFileInfo);
	gameFileInfo.player.offset		=	-1;
	gameFileInfo.player.size		=	sizeof(tPlayer);
	gameFileInfo.objects.offset		=	-1;
	gameFileInfo.objects.count		=	gameData.objs.nLastObject [0]+1;
	gameFileInfo.objects.size		=	sizeof(tObject);
	gameFileInfo.walls.offset			=	-1;
	gameFileInfo.walls.count		=	gameData.walls.nWalls;
	gameFileInfo.walls.size			=	sizeof(tWall);
	gameFileInfo.doors.offset			=	-1;
	gameFileInfo.doors.count		=	gameData.walls.nOpenDoors;
	gameFileInfo.doors.size			=	sizeof(tActiveDoor);
	gameFileInfo.triggers.offset		=	-1;
	gameFileInfo.triggers.count	=	gameData.trigs.nTriggers;
	gameFileInfo.triggers.size		=	sizeof(tTrigger);
	gameFileInfo.control.offset		=	-1;
	gameFileInfo.control.count		=  1;
	gameFileInfo.control.size		=  sizeof(tReactorTriggers);
 	gameFileInfo.botGen.offset		=	-1;
	gameFileInfo.botGen.count		=	gameData.matCens.nBotCenters;
	gameFileInfo.botGen.size		=	sizeof(tMatCenInfo);

 	gameFileInfo.lightDeltaIndices.offset		=	-1;
	gameFileInfo.lightDeltaIndices.count		=	gameData.render.lights.nStatic;
	gameFileInfo.lightDeltaIndices.size		=	sizeof(tLightDeltaIndex);

 	gameFileInfo.lightDeltas.offset		=	-1;
	gameFileInfo.lightDeltas.count	=	CountDeltaLightRecords();
	gameFileInfo.lightDeltas.size		=	sizeof(tLightDelta);

	// Write the fileinfo
	fwrite(&gameFileInfo, sizeof(gameFileInfo), 1, SaveFile);

	// Write the mine name
	fprintf(SaveFile,"%s\n",gameData.missions.szCurrentLevel);

	fwrite(&gameData.models.nPolyModels,2,1,SaveFile);
	fwrite(Pof_names,gameData.models.nPolyModels,sizeof(*Pof_names),SaveFile);

	//==================== SAVE PLAYER INFO ===========================

	player.offset = ftell(SaveFile);
	fwrite(&LOCALPLAYER, sizeof(tPlayer), 1, SaveFile);

	//==================== SAVE OBJECT INFO ===========================

	tObject.offset = ftell(SaveFile);
	//fwrite(&OBJECTS, sizeof(tObject), gameFileInfo.objects.count, SaveFile);
	{
		int i;
		for (i=0;i<gameFileInfo.objects.count;i++)
			writeObject(&OBJECTS [i],SaveFile);
	}

	//==================== SAVE WALL INFO =============================

	walls.offset = ftell(SaveFile);
	fwrite(gameData.walls.walls, sizeof(tWall), gameFileInfo.walls.count, SaveFile);

	//==================== SAVE DOOR INFO =============================

	doors.offset = ftell(SaveFile);
	fwrite(gameData.walls.activeDoors, sizeof(tActiveDoor), gameFileInfo.doors.count, SaveFile);

	//==================== SAVE TRIGGER INFO =============================

	triggers.offset = ftell(SaveFile);
	fwrite(gameData.trigs.triggers, sizeof(tTrigger), gameFileInfo.triggers.count, SaveFile);

	//================ SAVE CONTROL CENTER TRIGGER INFO ===============

	control.offset = ftell(SaveFile);
	fwrite(&gameData.reactor.triggers, sizeof(tReactorTriggers), 1, SaveFile);


	//================ SAVE MATERIALIZATION CENTER TRIGGER INFO ===============

	botGen.offset = ftell(SaveFile);
	fwrite(gameData.matCens.botGens, sizeof(tMatCenInfo), gameFileInfo.botGen.count, SaveFile);

	//================ SAVE DELTA LIGHT INFO ===============
	gameData.render.lights.deltaIndices.offset = ftell(SaveFile);
	fwrite(gameData.render.lights.deltaIndices, sizeof(tLightDeltaIndex), gameFileInfo.lightDeltaIndices.count, SaveFile);

	deltaLight.offset = ftell(SaveFile);
	fwrite(gameData.render.lights.deltas, sizeof(tLightDelta), gameFileInfo.lightDeltas.count, SaveFile);

	//============= REWRITE FILE INFO, TO SAVE OFFSETS ===============

	// Update the offset fields
	gameFileInfo.player.offset		=	player.offset;
	gameFileInfo.objects.offset		=	tObject.offset;
	gameFileInfo.walls.offset			=	walls.offset;
	gameFileInfo.doors.offset			=	doors.offset;
	gameFileInfo.triggers.offset		=	triggers.offset;
	gameFileInfo.control.offset		=	control.offset;
	gameFileInfo.botGen.offset		=	botGen.offset;
	gameFileInfo.lightDeltaIndices.offset	=	gameData.render.lights.deltaIndices.offset;
	gameFileInfo.lightDeltas.offset	=	deltaLight.offset;


	end_offset = ftell(SaveFile);

	// Write the fileinfo
	fseek( SaveFile, start_offset, SEEK_SET);  // Move to TOF
	fwrite(&gameFileInfo, sizeof(gameFileInfo), 1, SaveFile);

	// Go back to end of data
	fseek(SaveFile,end_offset, SEEK_SET);

	return 0;
}

int save_mine_data(FILE * SaveFile);

// -----------------------------------------------------------------------------
// Save game
int saveLevel_sub(char * filename, int compiledVersion)
{
	FILE * SaveFile;
	char temp_filename [128];
	int sig = MAKE_SIG('P','L','V','L'),version=LEVEL_FILE_VERSION;
	int nMineDataOffset=0,nGameDataOffset=0;

	if (!compiledVersion)	{
		write_game_text_file(filename);

		if (Errors_in_mine) {
			if (IsRealLevel(filename)) {
				char  ErrorMessage [200];

				sprintf(ErrorMessage, TXT_MINE_ERRORS2, Errors_in_mine);
				StopTime();
				GrPaletteStepLoad (NULL);
	 
				if (ExecMessageBox(NULL, 2, TXT_CANCEL_SAVE, TXT_DO_SAVE, ErrorMessage)!=1)	{
					StartTime();
					return 1;
				}
				StartTime();
			}
		}
		ChangeFilenameExtension(temp_filename,filename,".LVL");
	}
	else
	{
		// macs are using the regular hog/rl2 files for shareware
	}

	SaveFile = fopen(temp_filename, "wb");
	if (!SaveFile)
	{
		char ErrorMessage [256];

		char fname [20];
		_splitpath(temp_filename, NULL, NULL, fname, NULL);

		sprintf(ErrorMessage, \
			"ERROR: Cannot write to '%s'.\nYou probably need to check out a locked\nversion of the file. You should save\nthis under a different filename, and then\ncheck out a locked copy by typing\n\'co -l %s.lvl'\nat the DOS prompt.\n" 
			, temp_filename, fname);
		StopTime();
		GrPaletteStepLoad (NULL);
		ExecMessageBox(NULL, 1, "Ok", ErrorMessage);
		StartTime();
		return 1;
	}

	if (gameData.missions.szCurrentLevel [0] == 0)
		strcpy(gameData.missions.szCurrentLevel,"Untitled");

	ClearTransientObjects(1);		//1 means clear proximity bombs

	compressObjects();		//after this, gameData.objs.nLastObject [0] == num OBJECTS

	//make sure tPlayer is in a tSegment
	if (!UpdateObjectSeg(OBJECTS + gameData.multiplayer.players [0].nObject)) {
		if (gameData.objs.consoleP->info.nSegment > gameData.segs.nLastSegment)
			gameData.objs.consoleP->info.nSegment = 0;
		COMPUTE_SEGMENT_CENTER (&gameData.objs.consoleP->info.position.vPos, gameData.segs.segments + gameData.objs.consoleP->info.nSegment);
	}
	FixObjectSegs();

	//Write the header

	gs_write_int(sig,SaveFile);
	gs_write_int(version,SaveFile);

	//save placeholders
	gs_write_int(nMineDataOffset,SaveFile);
	gs_write_int(nGameDataOffset,SaveFile);

	//Now write the damn data

	//write the version 8 data (to make file unreadable by 1.0 & 1.1)
	gs_write_int(gameData.time.xGame,SaveFile);
	gs_write_short(gameData.app.nFrameCount,SaveFile);
	gs_write_byte(gameData.time.xFrame,SaveFile);

	// Write the palette file name
	fprintf(SaveFile,"%s\n",szCurrentLevelPalette);

	gs_write_int(gameStates.app.nBaseCtrlCenExplTime,SaveFile);
	gs_write_int(gameData.reactor.nStrength,SaveFile);

	gs_write_int(gameData.render.lights.flicker.nLights,SaveFile);
	fwrite(gameData.render.lights.flicker.lights,sizeof(*gameData.render.lights.flicker.lights),gameData.render.lights.flicker.nLights,SaveFile);

	gs_write_int(gameData.segs.secret.nReturnSegment, SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient[RVEC][X], SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient[RVEC][Y], SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient[RVEC][Z], SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient[FVEC][X], SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient[FVEC][Y], SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient[FVEC][Z], SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient[UVEC][X], SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient[UVEC][Y], SaveFile);
	gs_write_int(gameData.segs.secret.returnOrient[UVEC][Z], SaveFile);

	nMineDataOffset = ftell(SaveFile);
	if (!compiledVersion)
		save_mine_data(SaveFile);
	else
		save_mine_data_compiled(SaveFile);
	nGameDataOffset = ftell(SaveFile);
	SaveGameData(SaveFile);

	fseek(SaveFile,sizeof(sig)+sizeof(version), SEEK_SET);
	gs_write_int(nMineDataOffset,SaveFile);
	gs_write_int(nGameDataOffset,SaveFile);

	//==================== CLOSE THE FILE =============================
	fclose(SaveFile);

	if (!compiledVersion)	{
		if (gameStates.app.nFunctionMode == FMODE_EDITOR)
			editor_status("Saved mine %s, \"%s\"",filename,gameData.missions.szCurrentLevel);
	}

	return 0;

}

// -----------------------------------------------------------------------------

#if 0 //dunno - 3rd party stuff?
extern void compress_uv_coordinates_all(void);
#endif

int SaveLevel(char * filename)
{
	int r1;

	// Save Normal version...
	r1 = saveLevel_sub(filename, 0);

	// Save compiled version...
	saveLevel_sub(filename, 1);

	return r1;
}

#endif	//EDITOR

#if DBG
void dump_mine_info(void)
{
	int	nSegment, nSide;
	fix	min_u, max_u, min_v, max_v, min_l, max_l, max_sl;

	min_u = F1_0*1000;
	min_v = min_u;
	min_l = min_u;

	max_u = -min_u;
	max_v = max_u;
	max_l = max_u;

	max_sl = 0;

	for (nSegment=0; nSegment<=gameData.segs.nLastSegment; nSegment++) {
		for (nSide=0; nSide<MAX_SIDES_PER_SEGMENT; nSide++) {
			int	vertnum;
			tSide	*sideP = &gameData.segs.segments [nSegment].sides [nSide];

			if (gameData.segs.segment2s [nSegment].xAvgSegLight > max_sl)
				max_sl = gameData.segs.segment2s [nSegment].xAvgSegLight;

			for (vertnum=0; vertnum<4; vertnum++) {
				if (sideP->uvls [vertnum].u < min_u)
					min_u = sideP->uvls [vertnum].u;
				else if (sideP->uvls [vertnum].u > max_u)
					max_u = sideP->uvls [vertnum].u;

				if (sideP->uvls [vertnum].v < min_v)
					min_v = sideP->uvls [vertnum].v;
				else if (sideP->uvls [vertnum].v > max_v)
					max_v = sideP->uvls [vertnum].v;

				if (sideP->uvls [vertnum].l < min_l)
					min_l = sideP->uvls [vertnum].l;
				else if (sideP->uvls [vertnum].l > max_l)
					max_l = sideP->uvls [vertnum].l;
			}

		}
	}
}

#endif

#ifdef EDITOR

// -----------------------------------------------------------------------------
//read in every level in mission and save out compiled version 
void save_all_compiledLevels(void)
{
DoLoadSaveLevels(1);
}

// -----------------------------------------------------------------------------
//read in every level in mission
void LoadAllLevels(void)
{
DoLoadSaveLevels(0);
}

// -----------------------------------------------------------------------------

void DoLoadSaveLevels(int save)
{
	int level_num;

	if (! SafetyCheck())
		return;

	no_oldLevel_file_error=1;

	for (level_num=1;level_num<=gameData.missions.nLastLevel;level_num++) {
		LoadLevelSub(gameData.missions.szLevelNames [level_num-1]);
		LoadPalette(szCurrentLevelPalette,1,1,0);		//don't change screen
		if (save)
			saveLevel_sub(gameData.missions.szLevelNames [level_num-1],1);
	}

	for (level_num = -1; level_num >= gameData.missions.nLastSecretLevel; level_num--) {
		LoadLevelSub(gameData.missions.szSecretLevelNames [-level_num-1]);
		LoadPalette(szCurrentLevelPalette,1,1,0);		//don't change screen
		if (save)
			saveLevel_sub (gameData.missions.szSecretLevelNames [-level_num-1],1);
	}

	no_oldLevel_file_error=0;

}

#endif
