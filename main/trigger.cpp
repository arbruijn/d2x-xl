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
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "descent.h"
#include "cockpit.h"
#include "menu.h"
#include "error.h"
#include "segmath.h"
#include "texmap.h"
#include "newdemo.h"
#include "endlevel.h"
#include "network.h"
#include "timer.h"
#include "segment.h"
#include "input.h"
#include "text.h"
#include "light.h"
#include "textdata.h"
#include "marker.h"
#include "lightning.h"
#include "loadobjects.h"
#include "savegame.h"
#include "playerdeath.h"

#define MAX_ORIENT_STEPS	10

int oppTrigTypes  [] = {
	TT_CLOSE_DOOR,
	TT_OPEN_DOOR,
	TT_OBJECT_PRODUCER,
	TT_EXIT,
	TT_SECRET_EXIT,
	TT_ILLUSION_ON,
	TT_ILLUSION_OFF,
	TT_LOCK_DOOR,
	TT_UNLOCK_DOOR,
	TT_CLOSE_WALL,
	TT_OPEN_WALL,
	TT_ILLUSORY_WALL,
	TT_LIGHT_ON,
	TT_LIGHT_OFF,
	TT_TELEPORT,
	TT_SPEEDBOOST,
	TT_CAMERA,
	TT_SHIELD_DAMAGE,
	TT_ENERGY_DRAIN,
	TT_CHANGE_TEXTURE,
	TT_SMOKE_LIFE,
	TT_SMOKE_SPEED,
	TT_SMOKE_DENS,
	TT_SMOKE_SIZE,
	TT_SMOKE_DRIFT,
	TT_COUNTDOWN,
	TT_SPAWN_BOT,
	TT_SMOKE_BRIGHTNESS,
	TT_SET_SPAWN,
	TT_MESSAGE,
	TT_SOUND,
	TT_MASTER,
	TT_DISABLE_TRIGGER,
	TT_ENABLE_TRIGGER,
	TT_DISARM_ROBOT,
	TT_REPROGRAM_ROBOT,
	TT_SHAKE_MINE
	};

//link Links [MAX_WALL_LINKS];
//int Num_links;

void EnterSecretLevel (void);
void ExitSecretLevel (void);
int PSecretLevelDestroyed (void);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CTriggerTargets::Check (void)
{
#if DBG
	int i, j;

if (m_nLinks > MAX_TRIGGER_TARGETS) {
	m_nLinks = MAX_TRIGGER_TARGETS;
	PrintLog (0, "Invalid trigger target count (trigger #%d)\n", (CTrigger *) this - gameData.trigs.triggers.Buffer ());
	}
for (i = j = 0; i < m_nLinks; i++) {
	if (m_sides [i] > 0) {
		if ((m_segments [i] < 0) || (m_segments [i] >= gameData.segs.nSegments) || (m_sides [i] < 0) || (m_sides [i] > 5)) {
			PrintLog (0, "Invalid trigger target (trigger #%d, target %d)\n", (CTrigger *) this - gameData.trigs.triggers.Buffer (), i);
			continue;
			}
		}
#if 0 // object info not yet read
	else {
		if ((m_segments [i] < 0) || (m_segments [i] >= gameData.objs.nObjects)) {
			PrintLog (0, "Invalid trigger target (trigger #%d, target %d)\n", (CTrigger *) this - gameData.trigs.triggers.Buffer (), i);
			continue;
			}
		}
#endif
	if (j < i) {
		m_segments [j] = m_segments [i];
		m_sides [j] = m_sides [i];
		}
	j++;
	}
m_nLinks = j;
#endif
}

//------------------------------------------------------------------------------

void CTriggerTargets::Read (CFile& cf)
{
	int i;

for (i = 0; i < MAX_TRIGGER_TARGETS; i++)
	m_segments [i] = cf.ReadShort ();
for (i = 0; i < MAX_TRIGGER_TARGETS; i++)
	m_sides [i] = cf.ReadShort ();
Check ();
}

//------------------------------------------------------------------------------

void CTriggerTargets::SaveState (CFile& cf)
{
for (int i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	cf.WriteShort (m_segments [i]);
	cf.WriteShort (m_sides [i]);
	}
}

//------------------------------------------------------------------------------

void CTriggerTargets::LoadState (CFile& cf)
{
for (int i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	m_segments [i] = cf.ReadShort ();
	m_sides [i] = cf.ReadShort ();
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

inline int CTrigger::Index (void)
{
return this - TRIGGERS;
}

//------------------------------------------------------------------------------
// Executes a link, attached to a CTrigger.
// Toggles all walls linked to the switch.
// Opens doors, Blasts blast walls, turns off illusions.
void CTrigger::DoLink (void)
{
for (int i = 0; i < m_nLinks; i++)
	SEGMENTS [m_segments [i]].ToggleWall (m_sides [i]);
}

//------------------------------------------------------------------------------

void CTrigger::DoChangeTexture (void)
{
	int	baseTex = GetValue () & 0xffff,
			ovlTex = (GetValue () >> 16);

for (int i = 0; i < m_nLinks; i++)
	SEGMENTS [m_segments [i]].SetTextures (m_sides [i], baseTex, ovlTex);
audio.Update (); // react to a wall becoming passable / impassable
}

//------------------------------------------------------------------------------

inline int CTrigger::DoExecObjTrigger (short nObject, int bDamage)
{
	fix	v = GetValue ();

if (v >= I2X (1))
v = X2I (v);
v = 10 - v;
if (bDamage != ((Type () == TT_TELEPORT) || (Type () == TT_SPAWN_BOT)))
	return 0;
if (!bDamage)
	return 1;
if (v >= 10)
	return 0;
if (fix (OBJECTS [nObject].Damage () * 100) > v * 10)
	return 0;
if (!Flagged (TF_PERMANENT))
	SetValue (0);
return 1;
}

//------------------------------------------------------------------------------

void CTrigger::DoSpawnBots (CObject* objP)
{
OperateRobotMaker (objP, m_nLinks ? m_segments [0] : -1);
}

//------------------------------------------------------------------------------

void CTrigger::DoTeleportBot (CObject* objP)
{
if (m_nLinks) {
	short nSegment;
	if (m_nLinks == 1)
		nSegment = m_segments [0];
	else {
		do {
			nSegment = m_segments [RandShort () % m_nLinks];
			} while (nSegment == m_info.nTeleportDest);
		m_info.nTeleportDest = nSegment;
		}
	if (objP->info.nSegment != nSegment) {
		objP->info.nSegment = nSegment;
		objP->info.position.vPos = SEGMENTS [nSegment].Center ();
		objP->RelinkToSeg (nSegment);
		if (ROBOTINFO (objP->info.nId).bossFlag) {
			int i = gameData.bosses.Find (objP->Index ());

			if (i >= 0)
				gameData.bosses [i].Setup (objP->Index ());
			}
		}
	}
}

//------------------------------------------------------------------------------
//close a door
void CTrigger::DoCloseDoor (void)
{
for (int i = 0; i < m_nLinks; i++)
	SEGMENTS [m_segments [i]].CloseDoor (m_sides [i], true);
}

//------------------------------------------------------------------------------
//turns lighting on.  returns true if lights were actually turned on. (they
//would not be if they had previously been bShot out).
int CTrigger::DoLightOn (void)
{
	int ret = 0;
	short nSegment, nSide;

for (int i = 0; i < m_nLinks; i++) {
	nSegment = m_segments [i];
	nSide = m_sides [i];
	//check if tmap2 casts light before turning the light on.  This
	//is to keep us from turning on blown-out lights
	if (gameData.pig.tex.tMapInfoP [SEGMENTS [nSegment].m_sides [nSide].m_nOvlTex].lighting) {
		ret |= AddLight (nSegment, nSide); 		//any light sets flag
		EnableVariableLight (nSegment, nSide);
		}
	}
return ret;
}

//------------------------------------------------------------------------------
//turns lighting off.  returns true if lights were actually turned off. (they
//would not be if they had previously been bShot out).
int CTrigger::DoLightOff (void)
{
	int ret = 0;
	short nSegment, nSide;

for (int i = 0; i < m_nLinks; i++) {
	nSegment = m_segments [i];
	nSide = m_sides [i];
	//check if tmap2 casts light before turning the light off.  This
	//is to keep us from turning off blown-out lights
	if (gameData.pig.tex.tMapInfoP [SEGMENTS [nSegment].m_sides [nSide].m_nOvlTex].lighting) {
		ret |= SubtractLight (nSegment, nSide); 	//any light sets flag
		DisableVariableLight (nSegment, nSide);
		}
	}
return ret;
}

//------------------------------------------------------------------------------
// Unlocks all doors linked to the switch.
void CTrigger::DoUnlockDoors (void)
{
	CWall*	wallP;

for (int i = 0; i < m_nLinks; i++) {
	if ((wallP = SEGMENTS [m_segments [i]].Wall (m_sides [i]))) {
		wallP->flags &= ~WALL_DOOR_LOCKED;
		wallP->keys = KEY_NONE;
		}
	}
}

//------------------------------------------------------------------------------

bool CTrigger::TargetsWall (int nWall)
{
for (int i = 0; i < m_nLinks; i++)
	if ((nWall == SEGMENTS [m_segments [i]].WallNum (m_sides [i])))
		return true;
return false;
}

//------------------------------------------------------------------------------
// Return trigger number if door is controlled by a Wall switch, else return -1.
int DoorSwitch (int nWall)
{
for (int i = 0; i < gameData.trigs.m_nTriggers; i++)
	if (TRIGGERS [i].TargetsWall (nWall))
		return i;
return -1;
}

//------------------------------------------------------------------------------

void FlagTriggeredDoors (void)
{
for (int i = 0; i < gameData.walls.nWalls; i++)
	if (DoorSwitch (i) >= 0)
		WALLS [i].SetFlags (WALL_WALL_SWITCH);
}

//------------------------------------------------------------------------------
// Locks all doors linked to the switch.
void CTrigger::DoLockDoors (void)
{
	CWall*	wallP;

for (int i = 0; i < m_nLinks; i++)
	if ((wallP = SEGMENTS [m_segments [i]].Wall (m_sides [i])))
		wallP->SetFlags (WALL_DOOR_LOCKED);
}

//------------------------------------------------------------------------------
// Changes player spawns according to triggers m_segments,m_sides list

int CTrigger::DoSetSpawnPoints (void)
{
	int 		i, j;
	short		nSegment;

for (i = j = 0; i < MAX_PLAYERS; i++) {
	nSegment = m_segments [j];
	TriggerSetOrient (&gameData.multiplayer.playerInit [i].position, nSegment, m_sides [j], 1, 0);
	gameData.multiplayer.playerInit [i].nSegment = nSegment;
	gameData.multiplayer.playerInit [i].nSegType = SEGMENTS [nSegment].m_function;
	if (i == N_LOCALPLAYER)
		markerManager.MoveSpawnPoint (&gameData.multiplayer.playerInit [i].position, nSegment);
	j = (j + 1) % m_nLinks;
	}
// delete any spawn markers that have been set before passing through this trigger to
// avoid players getting stuck when respawning at that marker
if (0 <= (markerManager.SetHighlight (markerManager.SpawnIndex (-1))))
	markerManager.Delete (1);
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoMasterTrigger (short nObject, int nPlayer, bool bObjTrigger)
{
	CTrigger*	trigP;

for (int i = 0; i < m_nLinks; i++) {
#if DBG
	if ((m_segments [i] < 0) || (m_segments [i] >= gameData.segs.nSegments))
		continue;
#endif
	if ((trigP = SEGMENTS [m_segments [i]].Trigger (m_sides [i])))
		if (trigP->Operate (nObject, nPlayer, 0, bObjTrigger) < 0)
			return -1;
	}
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoEnableTrigger (void)
{
	CTrigger*	trigP;

for (int i = 0; i < m_nLinks; i++) {
	if (m_sides [i] >= 0) {
		if ((trigP = SEGMENTS [m_segments [i]].Trigger (m_sides [i]))) {
			if (trigP->Type () != TT_MASTER)
				trigP->ClearFlags (TF_DISABLED);
			else {
				if (trigP->GetValue () > 0)
					trigP->Value ()--;
				if (trigP->GetValue () <= 0)
					trigP->ClearFlags (TF_DISABLED);
				}
			if (Flagged (TF_PERMANENT | TF_DISABLED, TF_PERMANENT))
				m_info.tOperated = -1;
			}
		}
	else {
		CObject*	objP = OBJECTS + m_segments [i];
		if (!objP || (objP->info.nType != OBJ_EFFECT))
			return 0;
		if (objP->info.nId == PARTICLE_ID)
			objP->rType.particleInfo.bEnabled = 1;
		else if (objP->info.nId == LIGHTNING_ID) {
			objP->rType.lightningInfo.bEnabled = 1;
			lightningManager.Enable (objP);
			}
		else if (objP->info.nId == SOUND_ID)
			objP->rType.soundInfo.bEnabled = 1;
		else
			return 0;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoDisableTrigger (void)
{
	CTrigger*	trigP;

for (int i = 0; i < m_nLinks; i++) {
	if (m_sides [i] >= 0) {
		if ((trigP = SEGMENTS [m_segments [i]].Trigger (m_sides [i]))) {
			trigP->SetFlags (TF_DISABLED);
			if (trigP->Type () == TT_MASTER)
				trigP->Value ()++;
			}
		}
	else {
		CObject*	objP = OBJECTS + m_segments [i];
		if (!objP || (objP->info.nType != OBJ_EFFECT))
			return 0;
		if (objP->info.nId == PARTICLE_ID)
			objP->rType.particleInfo.bEnabled = 0;
		else if (objP->info.nId == LIGHTNING_ID) {
			objP->rType.lightningInfo.bEnabled = 0;
			lightningManager.Enable (objP);
			}
		else if (objP->info.nId == SOUND_ID)
			objP->rType.soundInfo.bEnabled = 0;
		else
			return 0;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoDisarmRobots (void)
{
for (int i = 0; i < m_nLinks; i++) {
	if (m_sides [i] < 0) {
		CObject*	objP = OBJECTS + m_segments [i];
		if (objP && (objP->info.nType == OBJ_ROBOT))
			objP->Disarm ();
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoReprogramRobots (void)
{
for (int i = 0; i < m_nLinks; i++) {
	if (m_sides [i] < 0) {
		CObject*	objP = OBJECTS + m_segments [i];
		if (objP && (objP->info.nType == OBJ_ROBOT))
			objP->Reprogram ();
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoShowMessage (void)
{
ShowGameMessage (gameData.messages, X2I (GetValue ()), GetTime (0));
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoShakeMine (void)
{
gameStates.gameplay.seismic.nShakeFrequency = (GetValue () < 0) ? 0 : GetValue ();
gameStates.gameplay.seismic.nShakeDuration = I2X ((GetTime (0) < 0) ? 10 : GetTime (0));
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoPlaySound (short nObject)
{
	tTextIndex	*indexP = FindTextData (&gameData.sounds, X2I (GetValue ()));

if (!indexP)
	return 0;
if (Flagged (TF_DISABLED))
	return 0;
if (GetTime (0) < 0) {
	m_info.nChannel = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xffff / 2, 1, -1, -1, -1, I2X (1), indexP->pszText);
	if (m_info.nChannel >= 0)
		SetFlags (TF_PLAYING_SOUND);
	}
else if (/*(GetTime (1) > 0) &&*/ (gameData.time.xGame - m_info.tOperated > GetTime (1))) {
	m_info.nChannel = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xffff / 2, 0, 0, GetTime (0) - 1, -1, I2X (1), indexP->pszText);
	if (m_info.nChannel >= 0) {
		SetTime (1, audio.Channel (m_info.nChannel)->Duration () * GetTime (0));
		SetFlags (TF_PLAYING_SOUND);
		}
#if DBG
	else
		indexP = FindTextData (&gameData.sounds, X2I (GetValue ()));
#endif
	}
else
	return 0;
return 1;
}

//------------------------------------------------------------------------------
// Changes walls pointed to by a CTrigger. returns true if any walls changed
int CTrigger::DoChangeWalls (void)
{
	CSegment*	segP, * connSegP;
	CWall*		wallP, * connWallP;
	int 			nNewWallType, bChanged = 0;
	short 		nSide, nConnSide;
	bool			bForceField;

for (int i = 0; i < m_nLinks; i++) {
	if ((m_segments [i] < 0) || (m_segments [i] >= gameData.segs.nSegments)) {
		PrintLog (0, "%s trigger %d has invalid target segment\n", 
					 gameData.trigs.triggers.IsElement (this) ? "Standard" : "Object", 
					 gameData.trigs.triggers.IsElement (this) ? gameData.trigs.triggers.Index (this) : gameData.trigs.objTriggers.Index (this));
		continue;
		}
	segP = SEGMENTS + m_segments [i];
	nSide = m_sides [i];

	if (segP->m_children [nSide] < 0) {
		if (gameOpts->legacy.bSwitches)
			Warning (TXT_TRIG_SINGLE, m_segments [i], nSide, Index ());
		connSegP = NULL;
		nConnSide = -1;
		}
	else {
		connSegP = SEGMENTS + segP->m_children [nSide];
		nConnSide = segP->ConnectedSide (connSegP);
		}
	switch (Type ()) {
		case TT_OPEN_WALL:
			nNewWallType = WALL_OPEN;
			break;
		case TT_CLOSE_WALL:
			nNewWallType = WALL_CLOSED;
			break;
		case TT_ILLUSORY_WALL:
			nNewWallType = WALL_ILLUSION;
			break;
		default:
			Assert (0); /* nNewWallType unset */
			return 0;
			break;
		}

	bForceField = ((gameData.pig.tex.tMapInfoP [segP->m_sides [nSide].m_nBaseTex].flags & TMI_FORCE_FIELD) != 0);
	if (!(wallP = segP->Wall (nSide))) {
#if DBG
		PrintLog (0, "WARNING: Wall trigger %d targets non-existent wall @ %d,%d\n", Index (), segP->Index (), nSide);
#endif
		continue;
		}
	connWallP = connSegP->Wall (nConnSide);
	if ((wallP->nType == nNewWallType) && (!connWallP || !bForceField || (connWallP->nType == nNewWallType)))
		continue;		//already in correct state, so skip

	bChanged = 1;
	switch (Type ()) {
		case TT_OPEN_WALL:
			if (!bForceField)
				segP->StartCloak (nSide);
			else {
				CFixVector vPos = segP->SideCenter (nSide);
				if (!Flagged (TF_SILENT))
					audio.CreateSegmentSound (SOUND_FORCEFIELD_OFF, segP->Index (), nSide, vPos, 0, I2X (1));
				wallP->nType = nNewWallType;
				audio.DestroySegmentSound (segP->Index (), nSide, SOUND_FORCEFIELD_HUM);
				if (connWallP) {
					connWallP->nType = nNewWallType;
					audio.DestroySegmentSound (SEG_IDX (connSegP), nConnSide, SOUND_FORCEFIELD_HUM);
					}
				if (SHOW_DYN_LIGHT) {
					lightManager.Toggle (segP->Index (), nSide, -1, 0);
					if (connWallP)
						lightManager.Toggle (SEG_IDX (connSegP), nConnSide, -1, 0);
					}
				}
			break;

		case TT_CLOSE_WALL:
			if (!bForceField)
				segP->StartDecloak (nSide);
			else {
				CFixVector vPos = segP->SideCenter (nSide);
				audio.CreateSegmentSound (SOUND_FORCEFIELD_HUM, segP->Index (), nSide, vPos, 1, I2X (1) / 2);
				wallP->nType = nNewWallType;
				if (connWallP)
					connWallP->nType = nNewWallType;
				if (SHOW_DYN_LIGHT) {
					lightManager.Toggle (segP->Index (), nSide, -1, 1);
					if (connWallP)
						lightManager.Toggle (SEG_IDX (connSegP), nConnSide, -1, 1);
					}
				}
			break;

		case TT_ILLUSORY_WALL:
			wallP->nType = nNewWallType;
			if (connWallP)
				connWallP->nType = nNewWallType;
			break;
		}
	KillStuckObjects (wallP - WALLS);
	if (connWallP)
		KillStuckObjects (connWallP - WALLS);
  	}
return bChanged;
}

//------------------------------------------------------------------------------

void CTrigger::PrintMessage (int nPlayer, int bShot, const char *message)
{
	static char	pluralSuffix [2][2] = {"", "s"};		//points to 's' or nothing for plural word

if (!gameStates.app.bD1Mission && ((nPlayer < 0) || (nPlayer == N_LOCALPLAYER))) {
	if (!Flagged (TF_NO_MESSAGE) && bShot)	// only display if not a silent trigger and the trigger has been shot (not flown through)
		HUDInitMessage (message, pluralSuffix [m_nLinks > 1]);
	}
}

//------------------------------------------------------------------------------

void CTrigger::DoObjectProducer (int bMessage)
{
	int i, h [3] = {0,0,0};

for (i = 0; i < m_nLinks; i++)
	h [StartObjectProducer (m_segments [i])]++;
if (bMessage) {
	if (h [1])
		HUDInitMessage (TXT_EQUIPGENS_ON, (h [1] == 1) ? "" : "s");
	if (h [2])
		HUDInitMessage (TXT_EQUIPGENS_OFF, (h [2] == 1) ? "" : "s");
	}
}

//------------------------------------------------------------------------------

void CTrigger::DoIllusionOn (void)
{
for (int i = 0; i < m_nLinks; i++)
	SEGMENTS [m_segments [i]].IllusionOn (m_sides [i]);
}

//------------------------------------------------------------------------------

void CTrigger::DoIllusionOff (void)
{
	CSegment*	segP;
	short			nSide;

for (int i = 0; i < m_nLinks; i++) {
	segP = SEGMENTS + m_segments [i];
	nSide = m_sides [i];
	segP->IllusionOff (nSide);
	if (Flagged (TF_SILENT))
		audio.CreateSegmentSound (SOUND_WALL_REMOVED, segP->Index (), nSide, segP->SideCenter (nSide), 0, I2X (1));
  	}
}

//------------------------------------------------------------------------------

void TriggerSetOrient (tObjTransformation *posP, short nSegment, short nSide, int bSetPos, int nStep)
{
	CAngleVector	an;
	CFixVector		n;

if (nStep <= 0) {
	n = *SEGMENTS [nSegment].m_sides [nSide].m_normals;
	n = -n;
	/*
	n.v.coord.y = -n.v.coord.y;
	n.v.coord.z = -n.v.coord.z;
	*/
	gameStates.gameplay.vTgtDir = n;
	if (nStep < 0)
		nStep = MAX_ORIENT_STEPS;
	}
else
	n = gameStates.gameplay.vTgtDir;
// turn the ship so that it is facing the destination nSide of the destination CSegment
// Invert the Normal as it points into the CSegment
// compute angles from the Normal
an = n.ToAnglesVec ();
// create new orientation matrix
if (!nStep)
	posP->mOrient = CFixMatrix::Create (an);
if (bSetPos)
	posP->vPos = SEGMENTS [nSegment].Center ();
// rotate the ships vel vector accordingly
//StopPlayerMovement ();
}

//------------------------------------------------------------------------------

void TriggerSetObjOrient (short nObject, short nSegment, short nSide, int bSetPos, int nStep)
{
	CAngleVector	ad, an, av;
	CFixVector		vel, n;
	CFixMatrix		rm;
	CObject*			objP = OBJECTS + nObject;

TriggerSetOrient (&objP->info.position, nSegment, nSide, bSetPos, nStep);
if (nStep <= 0) {
	n = *SEGMENTS [nSegment].m_sides [nSide].m_normals;
	/*
	n.v.coord.x = -n.v.coord.x;
	n.v.coord.y = -n.v.coord.y;
	*/
	n = -n;
	gameStates.gameplay.vTgtDir = n;
	if (nStep < 0)
		nStep = MAX_ORIENT_STEPS;
	}
else
	n = gameStates.gameplay.vTgtDir;
an = n.ToAnglesVec ();
av = objP->mType.physInfo.velocity.ToAnglesVec ();
av.v.coord.p -= an.v.coord.p;
av.v.coord.b -= an.v.coord.b;
av.v.coord.h -= an.v.coord.h;
if (nStep) {
	if (nStep > 1) {
		av.v.coord.p /= nStep;
		av.v.coord.b /= nStep;
		av.v.coord.h /= nStep;
		ad = objP->info.position.mOrient.ComputeAngles ();
		ad.v.coord.p += (an.v.coord.p - ad.v.coord.p) / nStep;
		ad.v.coord.b += (an.v.coord.b - ad.v.coord.b) / nStep;
		ad.v.coord.h += (an.v.coord.h - ad.v.coord.h) / nStep;
		objP->info.position.mOrient = CFixMatrix::Create (ad);
		}
	else
		objP->info.position.mOrient = CFixMatrix::Create (an);
	}
rm = CFixMatrix::Create (av);
vel = rm * objP->mType.physInfo.velocity;
objP->mType.physInfo.velocity = vel;
//StopPlayerMovement ();
}

//------------------------------------------------------------------------------

void TriggerSetObjPos (short nObject, short nSegment)
{
OBJECTS [nObject].RelinkToSeg (nSegment);
}

//------------------------------------------------------------------------------

void CTrigger::DoTeleport (short nObject)
{
if ((nObject >= 0) && (m_nLinks > 0)) {
		int		i;
		short		nSegment, nSide;

	StopSpeedBoost (nObject);
	i = RandShort () % m_nLinks;
	nSegment = m_segments [i];
	nSide = m_sides [i];
	// set new player direction, facing the destination nSide
	TriggerSetObjOrient (nObject, nSegment, nSide, 1, 0);
	TriggerSetObjPos (nObject, nSegment);
	gameStates.render.bDoAppearanceEffect = 1;
	MultiSendTeleport ((char) nObject, nSegment, (char) nSide);
	}
}

//------------------------------------------------------------------------------

CWall* TriggerParentWall (short nTrigger)
{
for (int i = 0; i < gameData.walls.nWalls; i++)
	if (WALLS [i].nTrigger == nTrigger)
		return WALLS + i;
return NULL;
}

//------------------------------------------------------------------------------

fix			speedBoostSpeed = 0;

void SetSpeedBoostVelocity (short nObject, fix speed,
									 short srcSegnum, short srcSidenum,
									 short destSegnum, short destSidenum,
									 CFixVector *pSrcPt, CFixVector *pDestPt,
									 int bSetOrient)
{
	CFixVector			n, h;
	CObject				*objP = OBJECTS + nObject;
	int					v;
	tSpeedBoostData	sbd = gameData.objs.speedBoost [nObject];

if (speed < 0)
	speed = speedBoostSpeed;
if ((speed <= 0) || (speed > 10))
	speed = 10;
speedBoostSpeed = speed;
v = OBJECTS [nObject].MaxSpeed () + (COMPETITION ? 100 : extraGameInfo [IsMultiGame].nSpeedBoost) * 4 * speed;
if (sbd.bBoosted) {
	if (pSrcPt && pDestPt) {
		n = *pDestPt - *pSrcPt;
		CFixVector::Normalize (n);
		}
	else if (srcSegnum >= 0) {
		sbd.vSrc = SEGMENTS [srcSegnum].SideCenter (srcSidenum);
		sbd.vDest = SEGMENTS [destSegnum].SideCenter (destSidenum);
		if (memcmp (&sbd.vSrc, &sbd.vDest, sizeof (CFixVector))) {
			n = sbd.vDest - sbd.vSrc;
			CFixVector::Normalize (n);
			}
		else {
			controls [0].verticalThrustTime =
			controls [0].forwardThrustTime =
			controls [0].sidewaysThrustTime = 0;
			memcpy (&n, SEGMENTS [destSegnum].m_sides [destSidenum].m_normals, sizeof (n));
		// turn the ship so that it is facing the destination nSide of the destination CSegment
		// Invert the Normal as it points into the CSegment
			/*
			n.v.coord.x = -n.v.coord.x;
			n.v.coord.y = -n.v.coord.y;
			*/
			n = -n;
			}
		}
	else {
		memcpy (&n, SEGMENTS [destSegnum].m_sides [destSidenum].m_normals, sizeof (n));
	// turn the ship so that it is facing the destination nSide of the destination CSegment
	// Invert the Normal as it points into the CSegment
		/*
		n.v.coord.x = -n.v.coord.x;
		n.v.coord.y = -n.v.coord.y;
		*/
		n = -n;
		}
	sbd.vVel.v.coord.x = n.v.coord.x * v;
	sbd.vVel.v.coord.y = n.v.coord.y * v;
	sbd.vVel.v.coord.z = n.v.coord.z * v;
#if 0
	d = (double) (labs (n.dir.coord.x) + labs (n.dir.coord.y) + labs (n.dir.coord.z)) / ((double) I2X (60));
	h.dir.coord.x = n.dir.coord.x ? (fix) ((double) n.dir.coord.x / d) : 0;
	h.dir.coord.y = n.dir.coord.y ? (fix) ((double) n.dir.coord.y / d) : 0;
	h.dir.coord.z = n.dir.coord.z ? (fix) ((double) n.dir.coord.z / d) : 0;
#else
#	if 1
	h.v.coord.x =
	h.v.coord.y =
	h.v.coord.z = I2X (60);
#	else
	h.dir.coord.x = (n.dir.coord.x ? n.dir.coord.x : I2X (1)) * 60;
	h.dir.coord.y = (n.dir.coord.y ? n.dir.coord.y : I2X (1)) * 60;
	h.dir.coord.z = (n.dir.coord.z ? n.dir.coord.z : I2X (1)) * 60;
#	endif
#endif
	sbd.vMinVel = sbd.vVel - h;
/*
	if (!sbd.vMinVel.v.coord.x)
		sbd.vMinVel.v.coord.x = -I2X (60);
	if (!sbd.vMinVel.v.coord.y)
		sbd.vMinVel.v.coord.y = -I2X (60);
	if (!sbd.vMinVel.v.coord.z)
		sbd.vMinVel.v.coord.z = -I2X (60);
*/
	sbd.vMaxVel = sbd.vVel + h;
/*
	if (!sbd.vMaxVel.v.coord.x)
		sbd.vMaxVel.v.coord.x = I2X (60);
	if (!sbd.vMaxVel.v.coord.y)
		sbd.vMaxVel.v.coord.y = I2X (60);
	if (!sbd.vMaxVel.v.coord.z)
		sbd.vMaxVel.v.coord.z = I2X (60);
*/
	if (sbd.vMinVel.v.coord.x > sbd.vMaxVel.v.coord.x) {
		fix h = sbd.vMinVel.v.coord.x;
		sbd.vMinVel.v.coord.x = sbd.vMaxVel.v.coord.x;
		sbd.vMaxVel.v.coord.x = h;
		}
	if (sbd.vMinVel.v.coord.y > sbd.vMaxVel.v.coord.y) {
		fix h = sbd.vMinVel.v.coord.y;
		sbd.vMinVel.v.coord.y = sbd.vMaxVel.v.coord.y;
		sbd.vMaxVel.v.coord.y = h;
		}
	if (sbd.vMinVel.v.coord.z > sbd.vMaxVel.v.coord.z) {
		fix h = sbd.vMinVel.v.coord.z;
		sbd.vMinVel.v.coord.z = sbd.vMaxVel.v.coord.z;
		sbd.vMaxVel.v.coord.z = h;
		}
	objP->mType.physInfo.velocity = sbd.vVel;
	if (bSetOrient) {
		TriggerSetObjOrient (nObject, destSegnum, destSidenum, 0, -1);
		gameStates.gameplay.nDirSteps = MAX_ORIENT_STEPS - 1;
		}
	gameData.objs.speedBoost [nObject] = sbd;
	}
else {
	objP->mType.physInfo.velocity.v.coord.x = objP->mType.physInfo.velocity.v.coord.x / v * 60;
	objP->mType.physInfo.velocity.v.coord.y = objP->mType.physInfo.velocity.v.coord.y / v * 60;
	objP->mType.physInfo.velocity.v.coord.z = objP->mType.physInfo.velocity.v.coord.z / v * 60;
	}
}

//------------------------------------------------------------------------------

void UpdatePlayerOrient (void)
{
if (gameStates.app.tick40fps.bTick && gameStates.gameplay.nDirSteps)
	TriggerSetObjOrient (LOCALPLAYER.nObject, -1, -1, 0, gameStates.gameplay.nDirSteps--);
}

//------------------------------------------------------------------------------

void CTrigger::StopSpeedBoost (short nObject)
{
gameData.objs.speedBoost [nObject].bBoosted = 0;
SetSpeedBoostVelocity ((short) nObject, -1, -1, -1, -1, -1, NULL, NULL, 0);
}

//------------------------------------------------------------------------------

void CTrigger::DoSpeedBoost (short nObject)
{
if (!(COMPETITION /*|| IsCoopGame*/) || extraGameInfo [IsMultiGame].nSpeedBoost) {
	CWall* wallP = TriggerParentWall (Index ());
	gameData.objs.speedBoost [nObject].bBoosted = (GetValue () && (m_nLinks > 0));
	SetSpeedBoostVelocity ((short) nObject, GetValue (),
								  (short) (wallP ? wallP->nSegment : -1), (short) (wallP ? wallP->nSide : -1),
								  m_segments [0], m_sides [0], NULL, NULL, Flagged (TF_SET_ORIENT) != 0);
	}
}

//------------------------------------------------------------------------------

bool CTrigger::DoExit (int nPlayer)
{
if (nPlayer != N_LOCALPLAYER)
	return false;
if (Flagged (TF_DISABLED))
	return false;
audio.StopAll ();		//kill the sounds
StopSpeedBoost (gameData.multiplayer.players [nPlayer].nObject);
missionManager.AdvanceLevel (X2I (GetValue ()));
if (gameStates.app.bD1Mission) {
	StartEndLevelSequence (0);
	return true;
	}
else if (missionManager.nCurrentLevel > 0) {
	if ((gameData.segs.nLevelVersion > 20) && (missionManager.GetLevelState (missionManager.NextLevel ()) < 0))
		return false;
	if (!Flagged (TF_PERMANENT))
		SetFlags (TF_DISABLED);
	StartEndLevelSequence (0);
	return true;
	}
else if (missionManager.nCurrentLevel < 0) {
	if ((LOCALPLAYER.Shield () < 0) || gameStates.app.bPlayerIsDead)
		return false;
	ExitSecretLevel ();
	return true;
	}
return false;
}

//------------------------------------------------------------------------------

bool CTrigger::DoSecretExit (int nPlayer)
{
if (nPlayer != N_LOCALPLAYER)
	return false;
if ((LOCALPLAYER.Shield () < 0) || gameStates.app.bPlayerIsDead)
	return false;
if (IsMultiGame) {
	HUDInitMessage (TXT_TELEPORT_MULTI);
	audio.PlaySound (SOUND_BAD_SELECTION);
	return false;
	}

bool bDisabled = PSecretLevelDestroyed () == 1;

if (gameData.demo.nState == ND_STATE_RECORDING)			// record whether we're really going to the secret level
	NDRecordSecretExitBlown (bDisabled);

if (bDisabled && (gameData.demo.nState != ND_STATE_PLAYBACK)) {
	HUDInitMessage (TXT_SECRET_DESTROYED);
	audio.PlaySound (SOUND_BAD_SELECTION);
	return false;
}

if (gameData.demo.nState == ND_STATE_RECORDING)		// stop demo recording
	gameData.demo.nState = ND_STATE_PAUSED;
audio.StopAll ();		//kill the sounds
audio.PlaySound (SOUND_SECRET_EXIT);
paletteManager.DisableEffect ();
EnterSecretLevel ();
gameData.reactor.bDestroyed = 0;
return true;
}

//------------------------------------------------------------------------------

int CTrigger::WallIsForceField (void)
{
for (int i = 0; i < m_nLinks; i++)
	if ((gameData.pig.tex.tMapInfoP [SEGMENTS [m_segments [i]].m_sides [m_sides [i]].m_nBaseTex].flags & TMI_FORCE_FIELD))
		return 1;
return 0;
}

//------------------------------------------------------------------------------

bool CTrigger::IsExit (void)
{
return (Type () == TT_EXIT) || ((Type () == TT_DESCENT1) && ((m_info.flagsD1 & TRIGGER_EXIT) != 0));
}

//------------------------------------------------------------------------------

typedef struct {
	int	nFlag;
	int	nType;
} tXlatTriggers;

tXlatTriggers xlatTriggers [] = {
	{TRIGGER_CONTROL_DOORS, TT_OPEN_DOOR},
	{TRIGGER_SHIELD_DAMAGE, TT_SHIELD_DAMAGE},
	{TRIGGER_ENERGY_DRAIN, TT_ENERGY_DRAIN},
	{TRIGGER_EXIT, TT_EXIT},
	{TRIGGER_OBJECT_PRODUCER, TT_OBJECT_PRODUCER},
	{TRIGGER_ILLUSION_OFF, TT_ILLUSION_OFF},
	{TRIGGER_ILLUSION_ON, TT_ILLUSION_ON},
	{TRIGGER_SECRET_EXIT, TT_SECRET_EXIT},
	{TRIGGER_UNLOCK_DOORS, TT_UNLOCK_DOOR},
	{TRIGGER_OPEN_WALL, TT_OPEN_WALL},
	{TRIGGER_CLOSE_WALL, TT_CLOSE_WALL},
	{TRIGGER_ILLUSORY_WALL, TT_ILLUSORY_WALL}
	};


int CTrigger::OperateD1 (short nObject, int nPlayer, int bShot)
{
	int	h = 1;
	int	nTrigger = TRIGGERS.Index (this);


for (int i = 0; i < int (sizeofa (xlatTriggers)); i++)
	if (m_info.flagsD1 & xlatTriggers [i].nFlag) {
		Type () = xlatTriggers [i].nType;
		gameData.trigs.delay [nTrigger] = -1;
		if (!Operate (nObject, nPlayer, bShot, false))
			h = 0;
		if ((xlatTriggers [i].nFlag == TRIGGER_EXIT) || (xlatTriggers [i].nFlag == TRIGGER_SECRET_EXIT))
			break;
		}
Type () = TT_DESCENT1;
if (!h)
	gameData.trigs.delay [nTrigger] = gameStates.app.nSDLTicks [0];
return h;
}

//------------------------------------------------------------------------------

#if DBG
static int nDbgTrigger = -1;
static int nDbgType = -1;
#endif

int CTrigger::Operate (short nObject, int nPlayer, int bShot, bool bObjTrigger)
{
	static int nDepth = -1;
#if DBG
if (this - gameData.trigs.triggers.Buffer () == nDbgTrigger)
	BRP;
if (bObjTrigger)
	BRP;
else
	BRP;
#endif

if (Flagged (TF_DISABLED))
	return 1;		//1 means don't send trigger hit to other players

if (nDepth > 15)
	return 1;

nDepth++;

CObject*	objP = (nObject >= 0) ? OBJECTS + nObject : NULL;
bool bIsPlayer = objP && objP->IsPlayer ();

if (bIsPlayer) {
	if (nObject != LOCALPLAYER.nObject) {
		if (IsMultiGame) {
			if (ClientOnly ()) {
				nDepth--;
				return 1;
				}
			}
		else {
			if (!objP->IsGuideBot ()) {
				nDepth--;
				return 1;
				}
			}
		}
	}
else {
	nPlayer = -1;
	if (objP &&
		 (objP->info.nType != OBJ_ROBOT) &&
		 (objP->info.nType != OBJ_DEBRIS) && // exploded robot
		 (objP->info.nType != OBJ_REACTOR) &&
		 (objP->info.nType != OBJ_HOSTAGE) &&
		 (objP->info.nType != OBJ_POWERUP))	{		
		nDepth--;
		return 1;
		}
	if (!bObjTrigger && (Type () != TT_TELEPORT) && (Type () != TT_SPEEDBOOST && (Type () != TT_MASTER))) {
		nDepth--;
		return 1;
		}
	}

int nTrigger = bObjTrigger ? OBJTRIGGERS.Index (this) : TRIGGERS.Index (this);
if (nTrigger < 0) {
	nDepth--;
	return 1;
	}

if (!nDepth && !bObjTrigger && (Type () != TT_TELEPORT) && (Type () != TT_SPEEDBOOST)) {
	int t = gameStates.app.nSDLTicks [0];
	if ((gameData.trigs.delay [nTrigger] >= 0) && (t - gameData.trigs.delay [nTrigger] < 750)) {
		nDepth--;
		return 1;
		}
	gameData.trigs.delay [nTrigger] = gameStates.app.nSDLTicks [0];
	}

if (m_info.tOperated < 0) {
	m_info.tOperated = gameData.time.xGame;
	m_info.nObject = nObject;
	m_info.nPlayer = nPlayer;
	m_info.bShot = bShot;
	if (IsDelayed ()) {
		if (GetTime (0) > 0)
			SetTime (1, GetTime (0));
		else {
			fix h = -GetTime (0) / 10;
			SetTime (1, h + h * (RandShort () % 10));
			}
		}
	}

if (Delay () > 0) {
	gameData.trigs.delay [nTrigger] = -1;
	nDepth--;
	return 1;
	}

switch (Type ()) {

	case TT_EXIT:
		if (DoExit (nPlayer)) {
			nDepth--;
			return -1;
			}
		break;

	case TT_SECRET_EXIT:
		if (DoSecretExit (nPlayer)) {
			nDepth--;
			return -1;
			}
		break;

	case TT_OPEN_DOOR:
		DoLink ();
		PrintMessage (nPlayer, bShot, "Door%s opened!");
		break;

	case TT_CLOSE_DOOR:
		DoCloseDoor ();
		PrintMessage (nPlayer, bShot, "Door%s closed!");
		break;

	case TT_UNLOCK_DOOR:
		DoUnlockDoors ();
		PrintMessage (nPlayer, bShot, "Door%s unlocked!");
		break;

	case TT_LOCK_DOOR:
		DoLockDoors ();
		PrintMessage (nPlayer, bShot, "Door%s locked!");
		break;

	case TT_OPEN_WALL:
		if (DoChangeWalls ()) {
			if (WallIsForceField ())
				PrintMessage (nPlayer, bShot, "Force field%s deactivated!");
			else
				PrintMessage (nPlayer, bShot, "Wall%s opened!");
			}
		break;

	case TT_CLOSE_WALL:
		if (DoChangeWalls ()) {
			if (WallIsForceField ())
				PrintMessage (nPlayer, bShot, "Force field%s activated!");
			else
				PrintMessage (nPlayer, bShot, "Wall%s closed!");
		}
		break;

	case TT_ILLUSORY_WALL:
		//don't know what to say, so say nothing
		DoChangeWalls ();
		PrintMessage (nPlayer, bShot, "Creating Illusion!");
		break;

	case TT_OBJECT_PRODUCER:
		if (!IsMultiGame || gameData.app.GameMode (GM_MULTI_ROBOTS))
			DoObjectProducer (nPlayer == N_LOCALPLAYER);
		break;

	case TT_ILLUSION_ON:
		DoIllusionOn ();
		PrintMessage (nPlayer, bShot, "Illusion%s on!");
		break;

	case TT_ILLUSION_OFF:
		DoIllusionOff ();
		PrintMessage (nPlayer, bShot, "Illusion%s off!");
		break;

	case TT_LIGHT_OFF:
		if (DoLightOff ())
			PrintMessage (nPlayer, bShot, "Lights off!");
		break;

	case TT_LIGHT_ON:
		if (DoLightOn ())
			PrintMessage (nPlayer, bShot, "Lights on!");
		break;

	case TT_TELEPORT:
		if (bObjTrigger) {
			DoTeleportBot (objP);
			PrintMessage (nPlayer, bShot, "Robot is fleeing!");
			}
		else {
			if (bIsPlayer) {
				if (nPlayer != N_LOCALPLAYER)
					break;
				if ((LOCALPLAYER.Shield () < 0) || gameStates.app.bPlayerIsDead)
					break;
				}
			audio.PlaySound (SOUND_SECRET_EXIT, I2X (1));
			DoTeleport (nObject);
			if (bIsPlayer)
				PrintMessage (nPlayer, bShot, "Teleport!");
			}
		break;

	case TT_SPEEDBOOST:
		if (bIsPlayer) {
			if (nPlayer != N_LOCALPLAYER)
				break;
			if ((LOCALPLAYER.Shield () < 0) || gameStates.app.bPlayerIsDead)
				break;
			}
		DoSpeedBoost (nObject);
		if (bIsPlayer)
			PrintMessage (nPlayer, bShot, "Speed Boost!");
		break;

	case TT_SHIELD_DAMAGE:
		if (!gameStates.app.bPlayerIsDead) {
			if (gameStates.app.bD1Mission)
				LOCALPLAYER.UpdateShield (-TRIGGERS [nTrigger].GetValue ());
			else
				LOCALPLAYER.UpdateShield (-fix ((float (I2X (1)) * X2F (TRIGGERS [nTrigger].GetValue ()))));
			if (LOCALPLAYER.Shield () < 0)
				StartPlayerDeathSequence (OBJECTS + gameData.multiplayer.players [N_LOCALPLAYER].nObject);
			}
		break;

	case TT_ENERGY_DRAIN:
		if (!gameStates.app.bPlayerIsDead) {
			if (gameStates.app.bD1Mission)
				LOCALPLAYER.UpdateEnergy (-TRIGGERS [nTrigger].GetValue ());
			else
				LOCALPLAYER.UpdateEnergy (-fix (LOCALPLAYER.Energy () * X2F (TRIGGERS [nTrigger].GetValue ()) / 100));
			}
		break;

	case TT_CHANGE_TEXTURE:
		DoChangeTexture ();
		PrintMessage (nPlayer, 2, "Walls have been changed!");
		break;

	case TT_SPAWN_BOT:
		DoSpawnBots (objP);
		PrintMessage (nPlayer, 1, "Robot is summoning help!");
		break;

	case TT_SET_SPAWN:
		DoSetSpawnPoints ();
		PrintMessage (nPlayer, 1, "New spawn points set!");
		break;

	case TT_SMOKE_LIFE:
	case TT_SMOKE_SPEED:
	case TT_SMOKE_DENS:
	case TT_SMOKE_SIZE:
	case TT_SMOKE_DRIFT:
		break;

	case TT_COUNTDOWN:
		InitCountdown (this, 1, -1);
		break;

	case TT_MESSAGE:
		DoShowMessage ();
		break;

	case TT_SOUND:
		if (DoPlaySound (nObject))
			m_info.tOperated = gameData.time.xGame;
		break;

	case TT_MASTER:
		if (DoMasterTrigger (nObject, nPlayer, bObjTrigger) < 0)
			return -1;
		break;

	case TT_ENABLE_TRIGGER:
		DoEnableTrigger ();
#if DBG
		PrintMessage (nPlayer, 2, "Triggers have been enabled!");
#endif
		break;

	case TT_DISABLE_TRIGGER:
		DoDisableTrigger ();
#if DBG
		PrintMessage (nPlayer, 2, "Triggers have been disabled!");
#endif
		break;

	case TT_DISARM_ROBOT:
		DoDisarmRobots ();
		break;

	case TT_REPROGRAM_ROBOT:
		DoReprogramRobots ();
		break;

	case TT_SHAKE_MINE:
		DoShakeMine ();
		break;

	case TT_DESCENT1:
		OperateD1 (nObject, nPlayer, bShot);
		break;

	default:
		Int3 ();
		break;
	}

if (Flagged (TF_ONE_SHOT))		//if this is a one-shot...
	SetFlags (TF_DISABLED);		//..then don't let it happen again
if (Flagged (TF_ALTERNATE))
	Type () = oppTrigTypes [Type ()];
#if DBG
if ((this - gameData.trigs.triggers.Buffer () == nDbgTrigger) && (nDbgType == Type ()))
	BRP;
nDbgType = Type ();
#endif
nDepth--;
return 0;
}

//------------------------------------------------------------------------------

bool CTrigger::IsDelayed (void) 
{
if (!gameStates.app.bD2XLevel)
	return 0;
if ((Type () == TT_COUNTDOWN) || (Type () == TT_MESSAGE) || (Type () == TT_SOUND))
	return 0;
if ((abs (GetTime (0)) < 100) || (abs (GetTime (0)) > 900000))
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::Delay (void) 
{ 
if (!IsDelayed ())
	return -1;
return gameData.time.xGame - m_info.tOperated < MSEC2X (GetTime (1));
}

//------------------------------------------------------------------------------

void CTrigger::Countdown (bool bObjTrigger)
{
if ((m_info.tOperated > 0) && !Delay ()) {
	Operate (m_info.nObject, m_info.nPlayer, m_info.bShot, bObjTrigger);
	if (Flagged (TF_PERMANENT))
		m_info.tOperated = -1;
	else {
		SetTime (0, 0);
		SetFlags (TF_DISABLED);
		}	
	}
}

//------------------------------------------------------------------------------

bool CTrigger::IsFlyThrough (void)
{
return !WALLS [m_info.nWall].IsSolid (true);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CTrigger::Read (CFile& cf, int bObjTrigger)
{
m_info.Clear ();
Type () = cf.ReadByte ();
if (bObjTrigger)
	SetFlags (cf.ReadUShort ());
else
	SetFlags (ushort (cf.ReadByte ()));
m_nLinks = (short) cf.ReadByte ();
cf.ReadByte ();
SetValue (cf.ReadFix ());
#if 1
if (Type () == TT_MASTER) {	//patch master trigger value (which acts as semaphore)
	if (bObjTrigger || (gameTopFileInfo.fileinfoVersion < 39))
		SetValue (0);
	else if (GetValue () > 0) {
		SetValue (X2I (GetValue ()));
		SetFlags (TF_DISABLED);
		if (Flagged (TF_AUTOPLAY))
			ClearFlags (TF_PERMANENT);
		}
	}
#endif
SetTime (0, cf.ReadFix ());
if (bObjTrigger && (Type () != TT_COUNTDOWN) && (Type () != TT_MESSAGE) && (Type () != TT_SOUND))
	SetTime (0, -1);
SetTime (1, -1);
CTriggerTargets::Read (cf);
// remove invalid trigger targets
m_info.nObject = -1;
m_info.nPlayer = -1;
m_info.nChannel = -1;
m_info.tOperated = -1;
}

//------------------------------------------------------------------------------

void CTrigger::LoadState (CFile& cf, bool bObjTrigger)
{
m_info.Clear ();
if (saveGameManager.Version () > 50) 
	m_info.nObject = cf.ReadShort ();
Type () = ubyte (cf.ReadByte ());
#if DBG
if (bObjTrigger)
	BRP;
else
	BRP;
#endif
if ((saveGameManager.Version () >= 48) || (bObjTrigger && (saveGameManager.Version () >= 41)))
	SetFlags (cf.ReadUShort ());
else
	SetFlags (ushort (cf.ReadByte ()));
m_nLinks = (short) cf.ReadByte ();
SetValue (cf.ReadFix ());
#if 1
if ((saveGameManager.Version () < 50) && (Type () == TT_MASTER))	//patch master trigger value (which acts as semaphore)
	SetValue (0);
#endif
SetTime (0, cf.ReadFix ());
SetTime (1, -1);
CTriggerTargets::LoadState (cf);
m_info.nChannel = -1;
m_info.tOperated = (saveGameManager.Version () < 44) ? -1 : cf.ReadFix ();
}

//------------------------------------------------------------------------------

void CTrigger::SaveState (CFile& cf, bool bObjTrigger)
{
cf.WriteShort (m_info.nObject);
cf.WriteByte (sbyte (Type ()));
cf.WriteShort (Flags ());
cf.WriteByte ((sbyte) m_nLinks);
cf.WriteFix (GetValue ());
cf.WriteFix (GetTime (0));
CTriggerTargets::SaveState (cf);
cf.WriteFix (m_info.tOperated);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CTrigger *FindObjTrigger (short nObject, short nType, short nTrigger)
{
if (!OBJTRIGGERS.Buffer ())
	return NULL;
if (!gameData.trigs.objTriggerRefs [nObject].nCount)
	return NULL;
if (gameData.trigs.objTriggerRefs [nObject].nFirst >= OBJTRIGGERS.Length ())
	return NULL;
if (nTrigger < 0)
	return OBJTRIGGERS + gameData.trigs.objTriggerRefs [nObject].nFirst;
if (++nTrigger - gameData.trigs.objTriggerRefs [nObject].nFirst < gameData.trigs.objTriggerRefs [nObject].nCount)
	return OBJTRIGGERS + nTrigger;
return NULL;
}

//------------------------------------------------------------------------------

void ExecObjTriggers (short nObject, int bDamage)
{
if (!OBJTRIGGERS.Buffer ())
	return;

	ubyte i = gameData.trigs.objTriggerRefs [nObject].nFirst, j = gameData.trigs.objTriggerRefs [nObject].nCount;

if (i >= OBJTRIGGERS.Length ())
	return;
for (; j; j--, i++) {
	if (OBJTRIGGERS [i].m_info.nObject < 0)
		continue;
	if (OBJTRIGGERS [i].DoExecObjTrigger (nObject, bDamage)) {
		OBJTRIGGERS [i].Operate (nObject, -1, 0, 1);
		if (IsMultiGame)
			MultiSendObjTrigger (i);
		}
	if (!bDamage)
		OBJTRIGGERS [i].m_info.nObject = -1;
	}
}

//------------------------------------------------------------------------------

void TriggersFrameProcess (void)
{
	CTrigger	*trigP;
	int		i;

trigP = TRIGGERS.Buffer ();
for (i = gameData.trigs.m_nTriggers; i > 0; i--, trigP++) {
#if DBG
	if (trigP->Index () == nDbgTrigger)
		BRP;
#endif
	if (!gameStates.app.bD2XLevel)
		continue;
	if (trigP->Flagged (TF_DISABLED))
		continue;
	if (trigP->Flagged (TF_AUTOPLAY) && (trigP->m_info.tOperated < 0) && (trigP->IsDelayed () || !trigP->Flagged (TF_PERMANENT))) {
		trigP->Operate (LOCALPLAYER.nObject, N_LOCALPLAYER, 0, false);
		if (!trigP->IsDelayed ())
			trigP->SetFlags (TF_DISABLED);
		}
	if ((trigP->Type () == TT_SOUND) && 
		 (trigP->Flagged (TF_PLAYING_SOUND)) && 
		 (trigP->GetTime (1) > 0) &&
		 (gameData.time.xGame - trigP->m_info.tOperated > trigP->GetTime (1))) {
		trigP->ClearFlags (TF_PLAYING_SOUND);
		if ((trigP->IsFlyThrough () ? !trigP->Flagged (TF_ONE_SHOT) : trigP->Flagged (TF_PERMANENT))) 
			trigP->SetFlags (TF_DISABLED); // sound has been played; make sure it doesn't get played again when laoading a save game that may subsequently be now
		}
	trigP->Countdown (false);	
	}

trigP = OBJTRIGGERS.Buffer ();
for (i = gameData.trigs.m_nObjTriggers; i > 0; i--, trigP++) {
	if (trigP->Flagged (TF_AUTOPLAY) && (trigP->m_info.nObject >= 0) && (trigP->m_info.tOperated < 0) && (!IsMultiGame || IAmGameHost ())) {
		trigP->Operate (trigP->m_info.nObject, -1, 0, true);
		trigP->SetFlags (TF_DISABLED);
		}
	trigP->Countdown (true);	
	}
}

//------------------------------------------------------------------------------

static void StartTriggeredSounds (CArray<CTrigger>& triggers)
{
	CTrigger	*trigP = triggers.Buffer ();

for (int i = triggers.Length (); i > 0; i--, trigP++)
	if ((trigP->Type () == TT_SOUND) && trigP->Flagged (TF_PLAYING_SOUND | TF_DISABLED, TF_PLAYING_SOUND) && (trigP->m_info.nChannel < 0))
		trigP->DoPlaySound (-1);
}

//------------------------------------------------------------------------------

void StartTriggeredSounds (void)
{
StartTriggeredSounds (gameData.trigs.triggers);
StartTriggeredSounds (gameData.trigs.objTriggers);
}

//------------------------------------------------------------------------------

static void StopTriggeredSounds (CArray<CTrigger>& triggers)
{
	CTrigger	*trigP = triggers.Buffer ();

for (int i = triggers.Length (); i > 0; i--, trigP++)
	trigP->m_info.nChannel = -1;
}

//------------------------------------------------------------------------------

void StopTriggeredSounds (void)
{
StopTriggeredSounds (gameData.trigs.triggers);
StopTriggeredSounds (gameData.trigs.objTriggers);
}

//------------------------------------------------------------------------------

inline int CTrigger::HasTarget (short nSegment, short nSide)
{
for (int i = 0; i < m_nLinks; i++)
	if ((m_segments [i] == nSegment) && ((nSide < 0) || (m_sides [i] == nSide)))
	return 1;
return 0;
}

//------------------------------------------------------------------------------

CWall *FindTriggerWall (short nTrigger)
{
	int	i;

for (i = 0; i < gameData.walls.nWalls; i++)
	if (WALLS [i].nTrigger == nTrigger)
		return WALLS + i;
return NULL;
}

//------------------------------------------------------------------------------

int FindTriggerSegSide (short nTrigger)
{
	CWall	*wallP = FindTriggerWall (nTrigger);

return wallP ? wallP->nSegment * 65536 + wallP->nSide : -1;
}

//------------------------------------------------------------------------------

int ObjTriggerIsValid (int nTrigger)
{
	int		j;
	//int		i;
	CObject	*objP;

FORALL_OBJS (objP, i) {
	j = objP->Index ();
	if ((nTrigger >= gameData.trigs.objTriggerRefs [j].nFirst) && 
		 (nTrigger < gameData.trigs.objTriggerRefs [j].nFirst + gameData.trigs.objTriggerRefs [j].nCount) &&
		(OBJTRIGGERS [nTrigger].m_info.nObject >= 0))
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

int FindTriggerTarget (short nSegment, short nSide, int i)
{
if (i < 0)
	i = gameData.trigs.m_nTriggers - i;

	int	j = i, nSegSide, nOvlTex, ec;

for (; i < gameData.trigs.m_nTriggers; i++) {
	nSegSide = FindTriggerSegSide (i);
	if (nSegSide == -1)
		continue;
	nOvlTex = SEGMENTS [nSegSide / 65536].m_sides [nSegSide & 0xffff].m_nOvlTex;
	if (nOvlTex <= 0)
		continue;
	ec = gameData.pig.tex.tMapInfoP [nOvlTex].nEffectClip;
	if (ec < 0) {
		if (gameData.pig.tex.tMapInfoP [nOvlTex].destroyed == -1)
			continue;
		}
	else {
		tEffectClip *ecP = gameData.effects.effectP + ec;
		if (ecP->flags & EF_ONE_SHOT)
			continue;
		if (ecP->nDestBm < 0)
			continue;
		}
	if (TRIGGERS [i].HasTarget (nSegment, nSide))
		return i + 1;
	}
for (j = i - gameData.trigs.m_nTriggers; i < gameData.trigs.m_nTriggers + gameData.trigs.m_nObjTriggers; i++, j++) {
	if (!OBJTRIGGERS [j].HasTarget (nSegment, nSide))
		continue;
	if (!ObjTriggerIsValid (j))
		continue;
	return -j - 1;
	}
return 0;
}

//------------------------------------------------------------------------------

#if 0
	static char d2TriggerMap [10] = {
		TT_OPEN_DOOR,
		TT_SHIELD_DAMAGE,
		TT_ENERGY_DRAIN,
		TT_EXIT,
		-1,
		-1,
		TT_OBJECT_PRODUCER,
		TT_ILLUSION_OFF,
		TT_ILLUSION_ON,
		TT_SECRET_EXIT
		};

	static char d2FlagMap [10] = {
		0,
		0,
		0,
		0,
		0,
		TF_ONE_SHOT,
		0,
		0,
		0,
		0
		};
#endif

/*
 * reads a tTriggerV29 structure from a CFile
 */
void V29TriggerRead (tTriggerV29& trigger, CFile& cf)
{
	int	i;

trigger.nType = cf.ReadByte ();
trigger.flags = cf.ReadShort ();
trigger.value = cf.ReadFix ();
trigger.time = cf.ReadFix ();
trigger.link_num = cf.ReadByte ();
trigger.nLinks = cf.ReadShort ();
for (i = 0; i < MAX_TRIGGER_TARGETS; i++)
	trigger.segments [i] = cf.ReadShort ();
for (i = 0; i < MAX_TRIGGER_TARGETS; i++)
	trigger.sides [i] = cf.ReadShort ();
}

//------------------------------------------------------------------------------

/*
 * reads a tTriggerV30 structure from a CFile
 */
void V30TriggerRead (tTriggerV30& trigger, CFile& cf)
{
	int i;

trigger.flags = cf.ReadShort ();
trigger.nLinks = cf.ReadByte ();
trigger.pad = cf.ReadByte ();
trigger.value = cf.ReadFix ();
trigger.time = cf.ReadFix ();
for (i = 0; i < MAX_TRIGGER_TARGETS; i++)
	trigger.segments [i] = cf.ReadShort ();
for (i = 0; i < MAX_TRIGGER_TARGETS; i++)
	trigger.sides [i] = cf.ReadShort ();
}

//	-----------------------------------------------------------------------------

int OpenExits (void)
{
	CTrigger *trigP = TRIGGERS.Buffer ();
	CWall		*wallP;
	int		nExits = 0;

for (int i = 0; i < gameData.trigs.m_nTriggers; i++, trigP++) {
	if (trigP->Type () == TT_EXIT) {
		wallP = FindTriggerWall (i);
		if (wallP) {
			SEGMENTS [wallP->nSegment].ToggleWall (wallP->nSide);
			nExits++;
			}
		}
	}
return nExits;
}

//	-----------------------------------------------------------------------------

int FindNextLevel (void)
{
missionManager.SetNextLevel (missionManager.nCurrentLevel + 1);

if (gameData.segs.nLevelVersion > 20) {
	CTrigger *trigP = TRIGGERS.Buffer ();
	int nNextLevel = 0x7FFFFFFF;
	int nLevelState = 0x7FFFFFFF;

	for (int i = 0; i < gameData.trigs.m_nTriggers; i++, trigP++) {
		if (trigP->Type () == TT_EXIT) {
			int l = (X2I (trigP->GetValue ()) > 0) ? X2I (trigP->GetValue ()) : missionManager.nCurrentLevel + 1;
			int s = missionManager.GetLevelState (l);
			if ((s >= 0) && ((s < nLevelState) || ((s == nLevelState) && (l < nNextLevel)))) {
				nLevelState = s;
				if (0 == (nNextLevel = l))
					break;
				}
			}
		}
	if (nNextLevel < 0x7FFFFFFF)
		missionManager.SetNextLevel (nNextLevel);
	}
return missionManager.NextLevel ();
}

//------------------------------------------------------------------------------
//eof
