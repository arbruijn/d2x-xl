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

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "inferno.h"
#include "stdlib.h"
#include "texmap.h"
#include "key.h"
#include "gameseg.h"
#include "textures.h"
#include "lightning.h"
#include "objsmoke.h"
#include "physics.h"
#include "slew.h"
#include "render.h"
#include "fireball.h"
#include "error.h"
#include "endlevel.h"
#include "timer.h"
#include "gameseg.h"
#include "collide.h"
#include "dynlight.h"
#include "interp.h"
#include "newdemo.h"
#include "gauges.h"
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
#include "multi.h"
#ifdef TACTILE
#	include "tactile.h"
#endif
#ifdef EDITOR
#	include "editor/editor.h"
#endif

#ifdef _3DFX
#include "3dfx_des.h"
#endif

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
info.xSize = cf.ReadFix (); 
info.xShields = cf.ReadFix ();
cf.ReadVector (info.vLastPos);  
info.contains.nType = cf.ReadByte (); 
info.contains.nId = cf.ReadByte ();   
info.contains.nCount = cf.ReadByte ();
info.nCreator = cf.ReadByte ();
info.xLifeLeft = cf.ReadFix ();   
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
		int i;
		rType.polyObjInfo.nModel = cf.ReadInt ();
		for (i = 0; i < MAX_SUBMODELS; i++)
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
cf.WriteFix (info.xShields);
cf.WriteVector (info.vLastPos);  
cf.WriteByte (info.contains.nType); 
cf.WriteByte (info.contains.nId);   
cf.WriteByte (info.contains.nCount);
cf.WriteByte (info.nCreator);
cf.WriteFix (info.xLifeLeft);   
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
	}
}

//------------------------------------------------------------------------------
//eof
