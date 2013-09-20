#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "descent.h"
#include "stdlib.h"
#include "texmap.h"
#include "key.h"
#include "segmath.h"
#include "textures.h"
#include "lightning.h"
#include "objsmoke.h"
#include "physics.h"
#include "slew.h"
#include "rendermine.h"
#include "fireball.h"
#include "error.h"
#include "endlevel.h"
#include "timer.h"
#include "segmath.h"
#include "collide.h"
#include "dynlight.h"
#include "interp.h"
#include "newdemo.h"
#include "cockpit.h"
#include "text.h"
#include "sphere.h"
#include "input.h"
#include "automap.h"
#include "u_mem.h"
#include "entropy.h"
#include "objrender.h"
#include "dropobject.h"
#include "marker.h"
#include "hiresmodels.h"
#include "loadgame.h"
#include "loadobjects.h"
#include "multi.h"
#include "savegame.h"
#include "strutil.h"

//------------------------------------------------------------------------------

void CObject::Read (CFile& cf)
{
#if DBG
if (OBJ_IDX (this) == nDbgObj)
	nDbgObj = nDbgObj;
#endif
info.nType = cf.ReadByte ();
#if DBG
if (info.nType == nDbgObjType)
	nDbgObjType = nDbgObjType;
#endif
info.nId = cf.ReadByte ();
#if DBG
if ((info.nType == OBJ_EFFECT) && (info.nId == WAYPOINT_ID))
	info.nId = WAYPOINT_ID;
#endif
info.controlType = cf.ReadByte ();
info.movementType = cf.ReadByte ();
info.renderType = cf.ReadByte ();
info.nFlags = cf.ReadByte ();
if (gameTopFileInfo.fileinfoVersion > 37)
	m_bMultiplayer = cf.ReadByte () != 0;
else
	m_bMultiplayer = false;
info.nSegment = cf.ReadShort ();
#if DBG
if (info.nSegment == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
info.nAttachedObj = -1;
cf.ReadVector (info.position.vPos);
cf.ReadMatrix (info.position.mOrient);
SetSize (cf.ReadFix ());
SetShield (cf.ReadFix ());
cf.ReadVector (info.vLastPos);
info.contains.nType = cf.ReadByte ();
info.contains.nId = cf.ReadByte ();
info.contains.nCount = cf.ReadByte ();
switch (info.movementType) {
	case MT_PHYSICS:
		cf.ReadVector (mType.physInfo.velocity);
		cf.ReadVector (mType.physInfo.thrust);
		mType.physInfo.mass = cf.ReadFix ();
		mType.physInfo.drag = cf.ReadFix ();
		mType.physInfo.brakes = cf.ReadFix ();
		cf.ReadVector (mType.physInfo.rotVel);
		cf.ReadVector (mType.physInfo.rotThrust);
		mType.physInfo.turnRoll	= cf.ReadFixAng ();
		mType.physInfo.flags	= cf.ReadShort ();
		break;

	case MT_SPINNING:
		cf.ReadVector (mType.spinRate);
		break;

	case MT_NONE:
		break;

	default:
		Int3();
	}

int i;

switch (info.controlType) {
	case CT_AI: 
		cType.aiInfo.behavior = cf.ReadByte ();
		for (i = 0; i < MAX_AI_FLAGS; i++)
			cType.aiInfo.flags [i] = cf.ReadByte ();
		cType.aiInfo.nHideSegment = cf.ReadShort ();
		cType.aiInfo.nHideIndex = cf.ReadShort ();
		cType.aiInfo.nPathLength = cf.ReadShort ();
		cType.aiInfo.nCurPathIndex = (char) cf.ReadShort ();
		if (gameTopFileInfo.fileinfoVersion <= 25) {
			cf.ReadShort ();	//				cType.aiInfo.follow_path_start_seg	= 
			cf.ReadShort ();	//				cType.aiInfo.follow_path_end_seg		= 
			}
		break;

	case CT_EXPLOSION:
		cType.explInfo.nSpawnTime = cf.ReadFix ();
		cType.explInfo.nDeleteTime	= cf.ReadFix ();
		cType.explInfo.nDeleteObj = cf.ReadShort ();
		cType.explInfo.attached.nNext = cType.explInfo.attached.nPrev = cType.explInfo.attached.nParent = -1;
		break;

	case CT_WEAPON: //do I really need to read these?  Are they even saved to disk?
		cType.laserInfo.parent.nType = cf.ReadShort ();
		cType.laserInfo.parent.nObject = cf.ReadShort ();
		cType.laserInfo.parent.nSignature = cf.ReadInt ();
		break;

	case CT_LIGHT:
		cType.lightInfo.intensity = cf.ReadFix ();
		break;

	case CT_POWERUP:
		if (gameTopFileInfo.fileinfoVersion >= 25)
			cType.powerupInfo.nCount = cf.ReadInt ();
		else
			cType.powerupInfo.nCount = 1;
		if (info.nId == POW_VULCAN)
			cType.powerupInfo.nCount = VULCAN_WEAPON_AMMO_AMOUNT;
		else if (info.nId == POW_GAUSS)
			cType.powerupInfo.nCount = VULCAN_WEAPON_AMMO_AMOUNT;
		else if (info.nId == POW_OMEGA)
			cType.powerupInfo.nCount = MAX_OMEGA_CHARGE;
		break;

	case CT_NONE:
	case CT_FLYING:
	case CT_DEBRIS:
		break;

	case CT_SLEW:		//the player is generally saved as slew
		break;

	case CT_CNTRLCEN:
		break;

	case CT_WAYPOINT:
		cType.wayPointInfo.nId [0] = -1;
		cType.wayPointInfo.nId [1] = cf.ReadInt ();
		cType.wayPointInfo.nSuccessor [0] = cf.ReadInt ();
		cType.wayPointInfo.nSuccessor [1] = -1;
		cType.wayPointInfo.nSpeed = cf.ReadInt ();
		break;

	case CT_MORPH:
	case CT_FLYTHROUGH:
	case CT_REPAIRCEN:
		default:
		Int3();
	}

switch (info.renderType) {
	case RT_NONE:
		break;

	case RT_MORPH:
	case RT_POLYOBJ: {
		rType.polyObjInfo.nModel = cf.ReadInt ();
		for (int i = 0; i <MAX_SUBMODELS; i++)
			cf.ReadAngVec(rType.polyObjInfo.animAngles [i]);
		rType.polyObjInfo.nSubObjFlags = cf.ReadInt ();
		int tmo = cf.ReadInt ();
		rType.polyObjInfo.nTexOverride = tmo;
		rType.polyObjInfo.nAltTextures = 0;
		break;
		}

	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
		rType.vClipInfo.nClipIndex	= cf.ReadInt ();
		rType.vClipInfo.xFrameTime	= cf.ReadFix ();
		rType.vClipInfo.nCurFrame	= cf.ReadByte ();
		if ((rType.vClipInfo.nClipIndex < 0) || (rType.vClipInfo.nClipIndex >= MAX_VCLIPS)) {
			rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [info.nId].nClipIndex;
			rType.vClipInfo.xFrameTime = gameData.effects.vClipP [rType.vClipInfo.nClipIndex].xFrameTime;
			rType.vClipInfo.nCurFrame = 0;
			}
		break;

	case RT_THRUSTER:
	case RT_LASER:
		break;

	case RT_SMOKE:
		rType.particleInfo.nLife = cf.ReadInt ();
		rType.particleInfo.nSize [0] = cf.ReadInt ();
		rType.particleInfo.nParts = cf.ReadInt ();
		rType.particleInfo.nSpeed = cf.ReadInt ();
		rType.particleInfo.nDrift = cf.ReadInt ();
		rType.particleInfo.nBrightness = cf.ReadInt ();
		rType.particleInfo.color.Red () = cf.ReadByte ();
		rType.particleInfo.color.Green () = cf.ReadByte ();
		rType.particleInfo.color.Blue () = cf.ReadByte ();
		rType.particleInfo.color.Alpha () = cf.ReadByte ();
		rType.particleInfo.nSide = cf.ReadByte ();
		if (gameData.segs.nLevelVersion < 18)
			rType.particleInfo.nType = 0;
		else
			rType.particleInfo.nType = cf.ReadByte ();
		if (gameData.segs.nLevelVersion < 19)
			rType.particleInfo.bEnabled = 1;
		else
			rType.particleInfo.bEnabled = cf.ReadByte ();
		break;

	case RT_LIGHTNING:
		rType.lightningInfo.nLife = cf.ReadInt ();
		rType.lightningInfo.nDelay = cf.ReadInt ();
		rType.lightningInfo.nLength = cf.ReadInt ();
		rType.lightningInfo.nAmplitude = cf.ReadInt ();
		rType.lightningInfo.nOffset = cf.ReadInt ();
		rType.lightningInfo.nWayPoint = (gameData.segs.nLevelVersion <= 22) ? -1 : cf.ReadInt ();
		rType.lightningInfo.nBolts = cf.ReadShort ();
		rType.lightningInfo.nId = cf.ReadShort ();
		rType.lightningInfo.nTarget = cf.ReadShort ();
		rType.lightningInfo.nNodes = cf.ReadShort ();
		rType.lightningInfo.nChildren = cf.ReadShort ();
		rType.lightningInfo.nFrames = cf.ReadShort ();
		rType.lightningInfo.nWidth = (gameData.segs.nLevelVersion <= 21) ? 3 : cf.ReadByte ();
		rType.lightningInfo.nAngle = cf.ReadByte ();
		rType.lightningInfo.nStyle = cf.ReadByte ();
		rType.lightningInfo.nSmoothe = cf.ReadByte ();
		rType.lightningInfo.bClamp = cf.ReadByte ();
		rType.lightningInfo.bGlow = cf.ReadByte ();
		rType.lightningInfo.bSound = cf.ReadByte ();
		rType.lightningInfo.bRandom = cf.ReadByte ();
		rType.lightningInfo.bInPlane = cf.ReadByte ();
		rType.lightningInfo.bDirection = 0;
		rType.lightningInfo.color.Red () = cf.ReadByte ();
		rType.lightningInfo.color.Green () = cf.ReadByte ();
		rType.lightningInfo.color.Blue () = cf.ReadByte ();
		rType.lightningInfo.color.Alpha () = cf.ReadByte ();
		rType.lightningInfo.bEnabled = (gameData.segs.nLevelVersion < 19) ? 1 : cf.ReadByte ();
		break;

	case RT_SOUND:
		cf.Read (rType.soundInfo.szFilename, 1, sizeof (rType.soundInfo.szFilename));
		rType.soundInfo.szFilename [sizeof (rType.soundInfo.szFilename) - 1] = '\0';
		strlwr (rType.soundInfo.szFilename);
		rType.soundInfo.nVolume = (int) FRound (float (cf.ReadInt ()) * float (I2X (1)) / 10.0f);
		if (gameData.segs.nLevelVersion < 19)
			rType.soundInfo.bEnabled = 1;
		else
			rType.soundInfo.bEnabled = cf.ReadByte ();
		break;

	default:
		Int3();
	}
ResetDamage ();
SetTarget (NULL);
m_nAttackRobots = -1;
}

//------------------------------------------------------------------------------

void CObject::LoadState (CFile& cf)
{
info.nSignature = cf.ReadInt ();      
info.nType = (ubyte) cf.ReadByte (); 
info.nId = (ubyte) cf.ReadByte ();
info.nNextInSeg = cf.ReadShort ();
info.nPrevInSeg = cf.ReadShort ();
info.controlType = (ubyte) cf.ReadByte ();
info.movementType = (ubyte) cf.ReadByte ();
info.renderType = (ubyte) cf.ReadByte ();
info.nFlags = (ubyte) cf.ReadByte ();
info.nSegment = cf.ReadShort ();
info.nAttachedObj = cf.ReadShort ();
cf.ReadVector (info.position.vPos);     
cf.ReadMatrix (info.position.mOrient);  
SetSize (cf.ReadFix ()); 
SetShield (cf.ReadFix ());
cf.ReadVector (info.vLastPos);  
info.contains.nType = cf.ReadByte (); 
info.contains.nId = cf.ReadByte ();   
info.contains.nCount = cf.ReadByte ();
info.nCreator = cf.ReadByte ();
SetLife (cf.ReadFix ());
if (saveGameManager.Version () > 56)
	m_nAttackRobots = cf.ReadInt ();
else
	SetAttackMode (ROBOT_IS_HOSTILE);

if (info.nType == OBJ_PLAYER)
	SetLife (IMMORTAL_TIME);

if (info.movementType == MT_PHYSICS) {
	cf.ReadVector (mType.physInfo.velocity);   
	cf.ReadVector (mType.physInfo.thrust);     
	mType.physInfo.mass = cf.ReadFix ();       
	mType.physInfo.drag = cf.ReadFix ();       
	mType.physInfo.brakes = cf.ReadFix ();     
	cf.ReadVector (mType.physInfo.rotVel);     
	cf.ReadVector (mType.physInfo.rotThrust);  
	mType.physInfo.turnRoll = cf.ReadFixAng ();   
	mType.physInfo.flags = (ushort) cf.ReadShort ();      
	}
else if (info.movementType == MT_SPINNING) {
	cf.ReadVector (mType.spinRate);  
	}

switch (info.controlType) {
	case CT_WEAPON:
		cType.laserInfo.parent.nType = cf.ReadShort ();
		cType.laserInfo.parent.nObject = cf.ReadShort ();
		cType.laserInfo.parent.nSignature = cf.ReadInt ();
		cType.laserInfo.xCreationTime = cf.ReadFix ();
		cType.laserInfo.nLastHitObj = cf.ReadShort ();
		if (cType.laserInfo.nLastHitObj < 0)
			cType.laserInfo.nLastHitObj = 0;
		else {
			gameData.objs.nHitObjects [Index () * MAX_HIT_OBJECTS] = cType.laserInfo.nLastHitObj;
			cType.laserInfo.nLastHitObj = 1;
			}
		cType.laserInfo.nHomingTarget = cf.ReadShort ();
		cType.laserInfo.xScale = cf.ReadFix ();
		break;

	case CT_EXPLOSION:
		cType.explInfo.nSpawnTime = cf.ReadFix ();
		cType.explInfo.nDeleteTime = cf.ReadFix ();
		cType.explInfo.nDeleteObj = cf.ReadShort ();
		cType.explInfo.attached.nParent = cf.ReadShort ();
		cType.explInfo.attached.nPrev = cf.ReadShort ();
		cType.explInfo.attached.nNext = cf.ReadShort ();
		break;

	case CT_AI:
		cType.aiInfo.behavior = (ubyte) cf.ReadByte ();
		cf.Read (cType.aiInfo.flags, 1, MAX_AI_FLAGS);
		cType.aiInfo.nHideSegment = cf.ReadShort ();
		cType.aiInfo.nHideIndex = cf.ReadShort ();
		cType.aiInfo.nPathLength = cf.ReadShort ();
		cType.aiInfo.nCurPathIndex = cf.ReadByte ();
		cType.aiInfo.bDyingSoundPlaying = cf.ReadByte ();
		cType.aiInfo.nDangerLaser = cf.ReadShort ();
		cType.aiInfo.nDangerLaserSig = cf.ReadInt ();
		cType.aiInfo.xDyingStartTime = cf.ReadFix ();
		break;

	case CT_LIGHT:
		cType.lightInfo.intensity = cf.ReadFix ();
		break;

	case CT_POWERUP:
		cType.powerupInfo.nCount = cf.ReadInt ();
		cType.powerupInfo.xCreationTime = cf.ReadFix ();
		cType.powerupInfo.nFlags = cf.ReadInt ();
		break;
	}
switch (info.renderType) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		rType.polyObjInfo.nModel = cf.ReadInt ();
		for (int i = 0; i < MAX_SUBMODELS; i++)
			cf.ReadAngVec (rType.polyObjInfo.animAngles [i]);
		rType.polyObjInfo.nSubObjFlags = cf.ReadInt ();
		rType.polyObjInfo.nTexOverride = cf.ReadInt ();
		rType.polyObjInfo.nAltTextures = cf.ReadInt ();
		break;
		}
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
	case RT_THRUSTER:
		rType.vClipInfo.nClipIndex = cf.ReadInt ();
		rType.vClipInfo.xFrameTime = cf.ReadFix ();
		rType.vClipInfo.nCurFrame = cf.ReadByte ();
		break;

	case RT_LASER:
		break;

	case RT_SMOKE:
		rType.particleInfo.bEnabled = (saveGameManager.Version () < 49) ? 1 : cf.ReadByte ();
		break;

	case RT_LIGHTNING:
		rType.lightningInfo.nWayPoint = (saveGameManager.Version () < 56) ? -1 : cf.ReadInt ();
		rType.lightningInfo.bDirection = (saveGameManager.Version () < 56) ? 0 : cf.ReadByte ();
		rType.lightningInfo.bEnabled = (saveGameManager.Version () < 49) ? 1 : cf.ReadByte ();
		break;

	case RT_SOUND:
		rType.soundInfo.bEnabled = (saveGameManager.Version () < 48) ? 1 : cf.ReadByte ();
		break;
	}
if (saveGameManager.Version () < 45)
	ResetDamage ();
else {
	m_damage.nHits [0] = cf.ReadFix ();
	m_damage.nHits [1] = cf.ReadFix ();
	m_damage.nHits [2] = cf.ReadFix ();
	if (saveGameManager.Version () < 46)
		m_damage.tRepaired = gameStates.app.nSDLTicks [0];
	else
		m_damage.tRepaired = gameStates.app.nSDLTicks [0] - cf.ReadFix ();
	}
}

//------------------------------------------------------------------------------

void CObject::SaveState (CFile& cf)
{
cf.WriteInt (info.nSignature);      
cf.WriteByte ((sbyte) info.nType); 
cf.WriteByte ((sbyte) info.nId);
cf.WriteShort (info.nNextInSeg);
cf.WriteShort (info.nPrevInSeg);
cf.WriteByte ((sbyte) info.controlType);
cf.WriteByte ((sbyte) info.movementType);
cf.WriteByte ((sbyte) info.renderType);
cf.WriteByte ((sbyte) info.nFlags);
cf.WriteShort (info.nSegment);
cf.WriteShort (info.nAttachedObj);
cf.WriteVector (OBJPOS (this)->vPos);     
cf.WriteMatrix (OBJPOS (this)->mOrient);  
cf.WriteFix (info.xSize); 
cf.WriteFix (info.xShield);
cf.WriteVector (info.vLastPos);  
cf.WriteByte (info.contains.nType); 
cf.WriteByte (info.contains.nId);   
cf.WriteByte (info.contains.nCount);
cf.WriteByte (info.nCreator);
cf.WriteFix (info.xLifeLeft);   
cf.WriteInt (m_nAttackRobots);

if (info.movementType == MT_PHYSICS) {
	cf.WriteVector (mType.physInfo.velocity);   
	cf.WriteVector (mType.physInfo.thrust);     
	cf.WriteFix (mType.physInfo.mass);       
	cf.WriteFix (mType.physInfo.drag);       
	cf.WriteFix (mType.physInfo.brakes);     
	cf.WriteVector (mType.physInfo.rotVel);     
	cf.WriteVector (mType.physInfo.rotThrust);  
	cf.WriteFixAng (mType.physInfo.turnRoll);   
	cf.WriteShort ((short) mType.physInfo.flags);      
	}
else if (info.movementType == MT_SPINNING) {
	cf.WriteVector(mType.spinRate);  
	}
switch (info.controlType) {
	case CT_WEAPON:
		cf.WriteShort (cType.laserInfo.parent.nType);
		cf.WriteShort (cType.laserInfo.parent.nObject);
		cf.WriteInt (cType.laserInfo.parent.nSignature);
		cf.WriteFix (cType.laserInfo.xCreationTime);
		if (cType.laserInfo.nLastHitObj)
			cf.WriteShort (gameData.objs.nHitObjects [Index () * MAX_HIT_OBJECTS + cType.laserInfo.nLastHitObj - 1]);
		else
			cf.WriteShort (-1);
		cf.WriteShort (cType.laserInfo.nHomingTarget);
		cf.WriteFix (cType.laserInfo.xScale);
		break;

	case CT_EXPLOSION:
		cf.WriteFix (cType.explInfo.nSpawnTime);
		cf.WriteFix (cType.explInfo.nDeleteTime);
		cf.WriteShort (cType.explInfo.nDeleteObj);
		cf.WriteShort (cType.explInfo.attached.nParent);
		cf.WriteShort (cType.explInfo.attached.nPrev);
		cf.WriteShort (cType.explInfo.attached.nNext);
		break;

	case CT_AI:
		cf.WriteByte ((sbyte) cType.aiInfo.behavior);
		cf.Write (cType.aiInfo.flags, 1, MAX_AI_FLAGS);
		cf.WriteShort (cType.aiInfo.nHideSegment);
		cf.WriteShort (cType.aiInfo.nHideIndex);
		cf.WriteShort (cType.aiInfo.nPathLength);
		cf.WriteByte (cType.aiInfo.nCurPathIndex);
		cf.WriteByte (cType.aiInfo.bDyingSoundPlaying);
		cf.WriteShort (cType.aiInfo.nDangerLaser);
		cf.WriteInt (cType.aiInfo.nDangerLaserSig);
		cf.WriteFix (cType.aiInfo.xDyingStartTime);
		break;

	case CT_LIGHT:
		cf.WriteFix (cType.lightInfo.intensity);
		break;

	case CT_POWERUP:
		cf.WriteInt (cType.powerupInfo.nCount);
		cf.WriteFix (cType.powerupInfo.xCreationTime);
		cf.WriteInt (cType.powerupInfo.nFlags);
		break;
	}
switch (info.renderType) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
		cf.WriteInt (rType.polyObjInfo.nModel);
		for (i = 0; i < MAX_SUBMODELS; i++)
			cf.WriteAngVec (rType.polyObjInfo.animAngles [i]);
		cf.WriteInt (rType.polyObjInfo.nSubObjFlags);
		cf.WriteInt (rType.polyObjInfo.nTexOverride);
		cf.WriteInt (rType.polyObjInfo.nAltTextures);
		break;
		}
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
	case RT_THRUSTER:
		cf.WriteInt (rType.vClipInfo.nClipIndex);
		cf.WriteFix (rType.vClipInfo.xFrameTime);
		cf.WriteByte (rType.vClipInfo.nCurFrame);
		break;

	case RT_LASER:
		break;

	case RT_SMOKE:
		cf.WriteByte (rType.particleInfo.bEnabled);
		break;

	case RT_LIGHTNING:
		cf.WriteInt (rType.lightningInfo.nWayPoint);
		cf.WriteByte (rType.lightningInfo.bDirection);
		cf.WriteByte (rType.lightningInfo.bEnabled);
		break;

	case RT_SOUND:
		cf.WriteByte (rType.soundInfo.bEnabled);
		break;
	}
cf.WriteFix (m_damage.nHits [0]);
cf.WriteFix (m_damage.nHits [1]);
cf.WriteFix (m_damage.nHits [2]);
cf.WriteFix (gameStates.app.nSDLTicks [0] - m_damage.tRepaired);
}

//------------------------------------------------------------------------------
//eof
