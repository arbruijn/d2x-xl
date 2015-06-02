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

int32_t oppTrigTypes  [] = {
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
//int32_t Num_links;

void EnterSecretLevel (void);
void ExitSecretLevel (void);
int32_t PSecretLevelDestroyed (void);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CTriggerTargets::Check (void)
{
	int32_t i, j;

if (m_nLinks > MAX_TRIGGER_TARGETS) {
	m_nLinks = MAX_TRIGGER_TARGETS;
	PrintLog (0, "Invalid trigger target count (trigger #%d)\n", (CTrigger *) this - gameData.trigData.triggers [0].Buffer ());
	}
for (i = j = 0; i < m_nLinks; i++) {
	if (m_sides [i] >= 0) {
		if ((m_segments [i] < 0) || (m_segments [i] >= gameData.segData.nSegments) || (m_sides [i] < 0) || (m_sides [i] > 5)) {
			PrintLog (0, "Invalid trigger target (trigger #%d, target %d)\n", (CTrigger *) this - gameData.trigData.triggers [0].Buffer (), i);
			continue;
			}
		}
	else {
		if ((m_segments [i] < 0) || (m_segments [i] >= gameData.objData.nObjects)) {
			PrintLog (0, "Invalid trigger target (trigger #%d, target %d)\n", (CTrigger *) this - gameData.trigData.triggers [0].Buffer (), i);
			continue;
			}
		}
	if (j < i) {
		m_segments [j] = m_segments [i];
		m_sides [j] = m_sides [i];
		}
	j++;
	}
m_nLinks = j;
}

//------------------------------------------------------------------------------

void CTriggerTargets::Read (CFile& cf)
{
	int32_t i;

for (i = 0; i < MAX_TRIGGER_TARGETS; i++)
	m_segments [i] = cf.ReadShort ();
for (i = 0; i < MAX_TRIGGER_TARGETS; i++)
	m_sides [i] = cf.ReadShort ();
Check ();
}

//------------------------------------------------------------------------------

void CTriggerTargets::SaveState (CFile& cf)
{
for (int32_t i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	cf.WriteShort (m_segments [i]);
	cf.WriteShort (m_sides [i]);
	}
}

//------------------------------------------------------------------------------

void CTriggerTargets::LoadState (CFile& cf)
{
for (int32_t i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	m_segments [i] = cf.ReadShort ();
	m_sides [i] = cf.ReadShort ();
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

inline int32_t CTrigger::Index (void)
{
return this - GEOTRIGGERS;
}

//------------------------------------------------------------------------------
// Executes a link, attached to a CTrigger.
// Toggles all walls linked to the switch.
// Opens doors, Blasts blast walls, turns off illusions.
void CTrigger::DoLink (void)
{
for (int32_t i = 0; i < m_nLinks; i++)
	SEGMENT (m_segments [i])->ToggleWall (m_sides [i]);
}

//------------------------------------------------------------------------------

void CTrigger::DoChangeTexture (void)
{
	int32_t	baseTex = GetValue () & 0xffff,
			ovlTex = (GetValue () >> 16);

for (int32_t i = 0; i < m_nLinks; i++)
	SEGMENT (m_segments [i])->SetTextures (m_sides [i], baseTex, ovlTex);
audio.Update (); // react to a wall becoming passable / impassable
}

//------------------------------------------------------------------------------

inline int32_t CTrigger::DoExecObjTrigger (int16_t nObject, int32_t bDamage)
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
if (fix (OBJECT (nObject)->Damage () * 100) > v * 10)
	return 0;
if (!Flagged (TF_PERMANENT))
	SetValue (0);
return 1;
}

//------------------------------------------------------------------------------

void CTrigger::DoSpawnBots (CObject* pObj)
{
OperateRobotMaker (pObj, m_nLinks ? m_segments [0] : -1);
}

//------------------------------------------------------------------------------

void CTrigger::DoTeleportBot (CObject* pObj)
{
if (m_nLinks) {
	int16_t nSegment;
	if (m_nLinks == 1)
		nSegment = m_segments [0];
	else {
		do {
			nSegment = m_segments [Rand (m_nLinks)];
			} while (nSegment == m_info.nTeleportDest);
		m_info.nTeleportDest = nSegment;
		}
	if (pObj->info.nSegment != nSegment) {
		pObj->info.nSegment = nSegment;
		pObj->info.position.vPos = SEGMENT (nSegment)->Center ();
		pObj->RelinkToSeg (nSegment);
		if (pObj->IsBoss ()) {
			int32_t i = gameData.bossData.Find (pObj->Index ());

			if (i >= 0)
				gameData.bossData [i].Setup (pObj->Index ());
			}
		}
	}
}

//------------------------------------------------------------------------------
//close a door
void CTrigger::DoCloseDoor (void)
{
for (int32_t i = 0; i < m_nLinks; i++)
	SEGMENT (m_segments [i])->CloseDoor (m_sides [i], true);
}

//------------------------------------------------------------------------------
//turns lighting on.  returns true if lights were actually turned on. (they
//would not be if they had previously been bShot out).
int32_t CTrigger::DoLightOn (void)
{
	int32_t ret = 0;
	int16_t nSegment, nSide;

for (int32_t i = 0; i < m_nLinks; i++) {
	nSegment = m_segments [i];
	nSide = m_sides [i];
	//check if tmap2 casts light before turning the light on.  This
	//is to keep us from turning on blown-out lights
	if (gameData.pigData.tex.pTexMapInfo [SEGMENT (nSegment)->m_sides [nSide].m_nOvlTex].lighting) {
		ret |= AddLight (nSegment, nSide); 		//any light sets flag
		EnableVariableLight (nSegment, nSide);
		}
	}
return ret;
}

//------------------------------------------------------------------------------
//turns lighting off.  returns true if lights were actually turned off. (they
//would not be if they had previously been bShot out).
int32_t CTrigger::DoLightOff (void)
{
	int32_t ret = 0;
	int16_t nSegment, nSide;

for (int32_t i = 0; i < m_nLinks; i++) {
	nSegment = m_segments [i];
	nSide = m_sides [i];
	//check if tmap2 casts light before turning the light off.  This
	//is to keep us from turning off blown-out lights
	if (gameData.pigData.tex.pTexMapInfo [SEGMENT (nSegment)->m_sides [nSide].m_nOvlTex].lighting) {
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
	CWall*	pWall;

for (int32_t i = 0; i < m_nLinks; i++) {
	if ((pWall = SEGMENT (m_segments [i])->Wall (m_sides [i]))) {
		pWall->flags &= ~WALL_DOOR_LOCKED;
		pWall->keys = KEY_NONE;
		}
	}
}

//------------------------------------------------------------------------------

bool CTrigger::TargetsWall (int32_t nWall)
{
for (int32_t i = 0; i < m_nLinks; i++)
	if ((nWall == SEGMENT (m_segments [i])->WallNum (m_sides [i])))
		return true;
return false;
}

//------------------------------------------------------------------------------
// Return trigger number if door is controlled by a Wall switch, else return -1.
int32_t DoorSwitch (int32_t nWall)
{
for (int32_t i = 0; i < gameData.trigData.m_nTriggers [0]; i++) {
	CTrigger* pTrigger = GEOTRIGGER (i);
	if (pTrigger && pTrigger->TargetsWall (nWall))
		return i;
	}
return -1;
}

//------------------------------------------------------------------------------

void FlagTriggeredDoors (void)
{
for (int32_t i = 0; i < gameData.wallData.nWalls; i++)
	if (DoorSwitch (i) >= 0)
		WALL (i)->SetFlags (WALL_WALL_SWITCH);
}

//------------------------------------------------------------------------------
// Locks all doors linked to the switch.
void CTrigger::DoLockDoors (void)
{
	CWall*	pWall;

for (int32_t i = 0; i < m_nLinks; i++)
	if ((pWall = SEGMENT (m_segments [i])->Wall (m_sides [i])))
		pWall->SetFlags (WALL_DOOR_LOCKED);
}

//------------------------------------------------------------------------------
// Changes player spawns according to triggers m_segments,m_sides list

int32_t CTrigger::DoSetSpawnPoints (void)
{
	int32_t 		i, j;
	int16_t		nSegment;

for (i = j = 0; i < MAX_PLAYERS; i++) {
	nSegment = m_segments [j];
	TriggerSetOrient (&gameData.multiplayer.playerInit [i].position, nSegment, m_sides [j], 1, 0);
	gameData.multiplayer.playerInit [i].nSegment = nSegment;
	gameData.multiplayer.playerInit [i].nSegType = SEGMENT (nSegment)->m_function;
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

int32_t CTrigger::DoMasterTrigger (int16_t nObject, int32_t nPlayer, bool bObjTrigger)
{
	CTrigger*	pTrigger;

for (int32_t i = 0; i < m_nLinks; i++) {
#if DBG
	if ((m_segments [i] < 0) || (m_segments [i] >= gameData.segData.nSegments))
		continue;
#endif
	if ((pTrigger = SEGMENT (m_segments [i])->Trigger (m_sides [i])))
		if (pTrigger->Operate (nObject, nPlayer, 0, bObjTrigger) < 0)
			return -1;
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t CTrigger::DoEnableTrigger (void)
{
	CTrigger*	pTrigger;

for (int32_t i = 0; i < m_nLinks; i++) {
	if (m_sides [i] >= 0) {
		if ((pTrigger = SEGMENT (m_segments [i])->Trigger (m_sides [i]))) {
			if (pTrigger->Type () != TT_MASTER)
				pTrigger->ClearFlags (TF_DISABLED);
			else {
				if (pTrigger->GetValue () > 0)
					pTrigger->Value ()--;
				if (pTrigger->GetValue () <= 0)
					pTrigger->ClearFlags (TF_DISABLED);
				}
			if (Flagged (TF_PERMANENT | TF_DISABLED, TF_PERMANENT))
				m_info.tOperated = -1;
			}
		}
	else {
		CObject*	pObj = OBJECT (m_segments [i]);
		if (!pObj || (pObj->info.nType != OBJ_EFFECT))
			return 0;
		if (pObj->info.nId == PARTICLE_ID)
			pObj->rType.particleInfo.bEnabled = 1;
		else if (pObj->info.nId == LIGHTNING_ID) {
			pObj->rType.lightningInfo.bEnabled = 1;
			lightningManager.Enable (pObj);
			}
		else if (pObj->info.nId == SOUND_ID)
			pObj->rType.soundInfo.bEnabled = 1;
		else
			return 0;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t CTrigger::DoDisableTrigger (void)
{
	CTrigger*	pTrigger;

for (int32_t i = 0; i < m_nLinks; i++) {
	if (m_sides [i] >= 0) {
		if ((pTrigger = SEGMENT (m_segments [i])->Trigger (m_sides [i]))) {
			pTrigger->SetFlags (TF_DISABLED);
			if (pTrigger->Type () == TT_MASTER)
				pTrigger->Value ()++;
			}
		}
	else {
		CObject*	pObj = OBJECT (m_segments [i]);
		if (!pObj || (pObj->info.nType != OBJ_EFFECT))
			return 0;
		if (pObj->info.nId == PARTICLE_ID)
			pObj->rType.particleInfo.bEnabled = 0;
		else if (pObj->info.nId == LIGHTNING_ID) {
			pObj->rType.lightningInfo.bEnabled = 0;
			lightningManager.Enable (pObj);
			}
		else if (pObj->info.nId == SOUND_ID)
			pObj->rType.soundInfo.bEnabled = 0;
		else
			return 0;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t CTrigger::DoDisarmRobots (void)
{
for (int32_t i = 0; i < m_nLinks; i++) {
	if (m_sides [i] < 0) {
		CObject*	pObj = OBJECT (m_segments [i]);
		if (pObj && (pObj->info.nType == OBJ_ROBOT))
			pObj->Disarm ();
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t CTrigger::DoReprogramRobots (void)
{
for (int32_t i = 0; i < m_nLinks; i++) {
	if (m_sides [i] < 0) {
		CObject*	pObj = OBJECT (m_segments [i]);
		if (pObj && (pObj->info.nType == OBJ_ROBOT))
			pObj->Reprogram ();
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t CTrigger::DoShowMessage (void)
{
ShowGameMessage (gameData.messages, X2I (GetValue ()), GetTime (0));
return 1;
}

//------------------------------------------------------------------------------

int32_t CTrigger::DoShakeMine (void)
{
gameStates.gameplay.seismic.nShakeFrequency = (GetValue () < 0) ? 0 : GetValue ();
gameStates.gameplay.seismic.nShakeDuration = I2X ((GetTime (0) < 0) ? 10 : GetTime (0));
return 1;
}

//------------------------------------------------------------------------------

int32_t CTrigger::DoPlaySound (int16_t nObject)
{
	tTextIndex	*pIndex = FindTextData (&gameData.soundData, X2I (GetValue ()));

if (!pIndex)
	return 0;
if (Flagged (TF_DISABLED))
	return 0;
if (GetTime (0) < 0) {
	m_info.nChannel = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xffff / 2, 1, -1, -1, -1, I2X (1), pIndex->pszText);
	if (m_info.nChannel >= 0)
		SetFlags (TF_PLAYING_SOUND);
	}
else if (/*(GetTime (1) > 0) &&*/ (gameData.timeData.xGame - m_info.tOperated > GetTime (1))) {
	m_info.nChannel = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xffff / 2, 0, 0, GetTime (0) - 1, -1, I2X (1), pIndex->pszText);
	if (m_info.nChannel >= 0) {
		SetTime (1, audio.Channel (m_info.nChannel)->Duration () * GetTime (0));
		SetFlags (TF_PLAYING_SOUND);
		}
#if DBG
	else
		pIndex = FindTextData (&gameData.soundData, X2I (GetValue ()));
#endif
	}
else
	return 0;
return 1;
}

//------------------------------------------------------------------------------
// Changes walls pointed to by a CTrigger. returns true if any walls changed
int32_t CTrigger::DoChangeWalls (void)
{
	CSegment*	pSeg, * pConnSeg;
	CWall*		pWall, * pConnWall;
	int32_t 			nNewWallType, bChanged = 0;
	int16_t 		nSide, nConnSide;
	bool			bForceField;

for (int32_t i = 0; i < m_nLinks; i++) {
	if ((m_segments [i] < 0) || (m_segments [i] >= gameData.segData.nSegments)) {
		PrintLog (0, "%s trigger %d has invalid target segment\n", 
					 gameData.trigData.triggers [0].IsElement (this) ? "Standard" : "Object", 
					 gameData.trigData.triggers [0].IsElement (this) ? gameData.trigData.triggers [0].Index (this) : gameData.trigData.triggers [1].Index (this));
		continue;
		}
	if (!(pSeg = SEGMENT (m_segments [i])))
		continue;
	nSide = m_sides [i];

	if (pSeg->m_children [nSide] < 0) {
		if (gameOpts->legacy.bSwitches)
			Warning (TXT_TRIG_SINGLE, m_segments [i], nSide, Index ());
		pConnSeg = NULL;
		nConnSide = -1;
		}
	else {
		pConnSeg = SEGMENT (pSeg->m_children [nSide]);
		nConnSide = pSeg->ConnectedSide (pConnSeg);
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

	bForceField = ((gameData.pigData.tex.pTexMapInfo [pSeg->m_sides [nSide].m_nBaseTex].flags & TMI_FORCE_FIELD) != 0);
	if (!(pWall = pSeg->Wall (nSide))) {
#if DBG
		PrintLog (0, "WARNING: Wall trigger %d targets non-existent wall @ %d,%d\n", Index (), pSeg->Index (), nSide);
#endif
		continue;
		}
	pConnWall = pConnSeg->Wall (nConnSide);
	if ((pWall->nType == nNewWallType) && (!pConnWall || !bForceField || (pConnWall->nType == nNewWallType)))
		continue;		//already in correct state, so skip

	bChanged = 1;
	switch (Type ()) {
		case TT_OPEN_WALL:
			if (!bForceField)
				pSeg->StartCloak (nSide);
			else {
				CFixVector vPos = pSeg->SideCenter (nSide);
				if (!Flagged (TF_SILENT))
					audio.CreateSegmentSound (SOUND_FORCEFIELD_OFF, pSeg->Index (), nSide, vPos, 0, I2X (1));
				pWall->nType = nNewWallType;
				audio.DestroySegmentSound (pSeg->Index (), nSide, SOUND_FORCEFIELD_HUM);
				if (pConnWall) {
					pConnWall->nType = nNewWallType;
					audio.DestroySegmentSound (SEG_IDX (pConnSeg), nConnSide, SOUND_FORCEFIELD_HUM);
					}
				if (SHOW_DYN_LIGHT) {
					lightManager.Toggle (pSeg->Index (), nSide, -1, 0);
					if (pConnWall)
						lightManager.Toggle (SEG_IDX (pConnSeg), nConnSide, -1, 0);
					}
				}
			break;

		case TT_CLOSE_WALL:
			if (!bForceField)
				pSeg->StartDecloak (nSide);
			else {
				CFixVector vPos = pSeg->SideCenter (nSide);
				audio.CreateSegmentSound (SOUND_FORCEFIELD_HUM, pSeg->Index (), nSide, vPos, 1, Max (gameOpts->sound.xCustomSoundVolume, I2X (1) / 8) / 2);
				pWall->nType = nNewWallType;
				if (pConnWall)
					pConnWall->nType = nNewWallType;
				if (SHOW_DYN_LIGHT) {
					lightManager.Toggle (pSeg->Index (), nSide, -1, 1);
					if (pConnWall)
						lightManager.Toggle (SEG_IDX (pConnSeg), nConnSide, -1, 1);
					}
				}
			break;

		case TT_ILLUSORY_WALL:
			pWall->nType = nNewWallType;
			if (pConnWall)
				pConnWall->nType = nNewWallType;
			break;
		}
	KillStuckObjects (pWall - WALLS);
	if (pConnWall)
		KillStuckObjects (pConnWall - WALLS);
  	}
return bChanged;
}

//------------------------------------------------------------------------------

void CTrigger::PrintMessage (int32_t nPlayer, int32_t bShot, const char *message)
{
	static char	pluralSuffix [2][2] = {"", "s"};		//points to 's' or nothing for plural word

if (!gameStates.app.bD1Mission && ((nPlayer < 0) || (nPlayer == N_LOCALPLAYER))) {
	if (!Flagged (TF_NO_MESSAGE) && bShot)	// only display if not a silent trigger and the trigger has been shot (not flown through)
		HUDInitMessage (message, pluralSuffix [m_nLinks > 1]);
	}
}

//------------------------------------------------------------------------------

void CTrigger::DoObjectProducer (int32_t bMessage)
{
	int32_t i, h [3] = {0,0,0};

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
for (int32_t i = 0; i < m_nLinks; i++)
	SEGMENT (m_segments [i])->IllusionOn (m_sides [i]);
}

//------------------------------------------------------------------------------

void CTrigger::DoIllusionOff (void)
{
	CSegment*	pSeg;
	int16_t			nSide;

for (int32_t i = 0; i < m_nLinks; i++) {
	pSeg = SEGMENT (m_segments [i]);
	nSide = m_sides [i];
	pSeg->IllusionOff (nSide);
	if (Flagged (TF_SILENT))
		audio.CreateSegmentSound (SOUND_WALL_REMOVED, pSeg->Index (), nSide, pSeg->SideCenter (nSide), 0, I2X (1));
  	}
}

//------------------------------------------------------------------------------

void TriggerSetOrient (tObjTransformation *pPos, int16_t nSegment, int16_t nSide, int32_t bSetPos, int32_t nStep)
{
	CAngleVector	an;
	CFixVector		n;

if (nStep <= 0) {
	n = *SEGMENT (nSegment)->m_sides [nSide].m_normals;
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
	pPos->mOrient = CFixMatrix::Create (an);
if (bSetPos)
	pPos->vPos = SEGMENT (nSegment)->Center ();
// rotate the ships vel vector accordingly
//StopPlayerMovement ();
}

//------------------------------------------------------------------------------

void TriggerSetObjOrient (int16_t nObject, int16_t nSegment, int16_t nSide, int32_t bSetPos, int32_t nStep)
{
	CAngleVector	ad, an, av;
	CFixVector		vel, n;
	CFixMatrix		rm;
	CObject*			pObj = OBJECT (nObject);

TriggerSetOrient (&pObj->info.position, nSegment, nSide, bSetPos, nStep);
if (nStep <= 0) {
	n = *SEGMENT (nSegment)->m_sides [nSide].m_normals;
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
av = pObj->mType.physInfo.velocity.ToAnglesVec ();
av.v.coord.p -= an.v.coord.p;
av.v.coord.b -= an.v.coord.b;
av.v.coord.h -= an.v.coord.h;
if (nStep) {
	if (nStep > 1) {
		av.v.coord.p /= nStep;
		av.v.coord.b /= nStep;
		av.v.coord.h /= nStep;
		ad = pObj->info.position.mOrient.ComputeAngles ();
		ad.v.coord.p += (an.v.coord.p - ad.v.coord.p) / nStep;
		ad.v.coord.b += (an.v.coord.b - ad.v.coord.b) / nStep;
		ad.v.coord.h += (an.v.coord.h - ad.v.coord.h) / nStep;
		pObj->info.position.mOrient = CFixMatrix::Create (ad);
		}
	else
		pObj->info.position.mOrient = CFixMatrix::Create (an);
	}
rm = CFixMatrix::Create (av);
vel = rm * pObj->mType.physInfo.velocity;
pObj->mType.physInfo.velocity = vel;
//StopPlayerMovement ();
}

//------------------------------------------------------------------------------

void TriggerSetObjPos (int16_t nObject, int16_t nSegment)
{
OBJECT (nObject)->RelinkToSeg (nSegment);
}

//------------------------------------------------------------------------------

void CTrigger::DoTeleport (int16_t nObject)
{
if ((nObject >= 0) && (m_nLinks > 0)) {
		int32_t		i;
		int16_t		nSegment, nSide;

	StopSpeedBoost (nObject);
	i = Rand (m_nLinks);
	nSegment = m_segments [i];
	nSide = m_sides [i];
	// set new player direction, facing the destination nSide
	TriggerSetObjOrient (nObject, nSegment, nSide, 1, 0);
	TriggerSetObjPos (nObject, nSegment);
	gameStates.render.bDoAppearanceEffect = 1; // do appearance effect, but suppress warp effect
	gameData.multiplayer.bTeleport [N_LOCALPLAYER] = 1;
	MultiSendTeleport (N_LOCALPLAYER, nSegment, (uint8_t) nSide);
	}
}

//------------------------------------------------------------------------------

CWall* TriggerParentWall (int16_t nTrigger)
{
for (int32_t i = 0; i < gameData.wallData.nWalls; i++)
	if (WALL (i)->nTrigger == nTrigger)
		return WALL (i);
return NULL;
}

//------------------------------------------------------------------------------

fix			speedBoostSpeed = 0;

void SetSpeedBoostVelocity (int16_t nObject, fix speed,
									 int16_t srcSegnum, int16_t srcSidenum,
									 int16_t destSegnum, int16_t destSidenum,
									 CFixVector *pSrcPt, CFixVector *pDestPt,
									 int32_t bSetOrient)
{
	CFixVector			n, h;
	CObject				*pObj = OBJECT (nObject);
	int32_t					v;
	tSpeedBoostData	sbd = gameData.objData.speedBoost [nObject];

if (speed < 0)
	speed = speedBoostSpeed;
if ((speed <= 0) || (speed > 10))
	speed = 10;
speedBoostSpeed = speed;
v = OBJECT (nObject)->MaxSpeedScaled () + (COMPETITION ? 100 : extraGameInfo [IsMultiGame].nSpeedBoost) * 4 * speed;
if (sbd.bBoosted) {
	if (pSrcPt && pDestPt) {
		n = *pDestPt - *pSrcPt;
		CFixVector::Normalize (n);
		}
	else if (srcSegnum >= 0) {
		sbd.vSrc = SEGMENT (srcSegnum)->SideCenter (srcSidenum);
		sbd.vDest = SEGMENT (destSegnum)->SideCenter (destSidenum);
		if (memcmp (&sbd.vSrc, &sbd.vDest, sizeof (CFixVector))) {
			n = sbd.vDest - sbd.vSrc;
			CFixVector::Normalize (n);
			}
		else {
			controls [0].verticalThrustTime =
			controls [0].forwardThrustTime =
			controls [0].sidewaysThrustTime = 0;
			memcpy (&n, SEGMENT (destSegnum)->m_sides [destSidenum].m_normals, sizeof (n));
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
		memcpy (&n, SEGMENT (destSegnum)->m_sides [destSidenum].m_normals, sizeof (n));
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
	pObj->mType.physInfo.velocity = sbd.vVel;
	if (bSetOrient) {
		TriggerSetObjOrient (nObject, destSegnum, destSidenum, 0, -1);
		gameStates.gameplay.nDirSteps = MAX_ORIENT_STEPS - 1;
		}
	gameData.objData.speedBoost [nObject] = sbd;
	}
else {
	pObj->mType.physInfo.velocity.v.coord.x = pObj->mType.physInfo.velocity.v.coord.x / v * 60;
	pObj->mType.physInfo.velocity.v.coord.y = pObj->mType.physInfo.velocity.v.coord.y / v * 60;
	pObj->mType.physInfo.velocity.v.coord.z = pObj->mType.physInfo.velocity.v.coord.z / v * 60;
	}
}

//------------------------------------------------------------------------------

void UpdatePlayerOrient (void)
{
if (gameStates.app.tick40fps.bTick && gameStates.gameplay.nDirSteps)
	TriggerSetObjOrient (LOCALPLAYER.nObject, -1, -1, 0, gameStates.gameplay.nDirSteps--);
}

//------------------------------------------------------------------------------

void CTrigger::StopSpeedBoost (int16_t nObject)
{
gameData.objData.speedBoost [nObject].bBoosted = 0;
SetSpeedBoostVelocity ((int16_t) nObject, -1, -1, -1, -1, -1, NULL, NULL, 0);
}

//------------------------------------------------------------------------------

void CTrigger::DoSpeedBoost (int16_t nObject)
{
if (!(COMPETITION /*|| IsCoopGame*/) || extraGameInfo [IsMultiGame].nSpeedBoost) {
	CWall* pWall = TriggerParentWall (Index ());
	gameData.objData.speedBoost [nObject].bBoosted = (GetValue () && (m_nLinks > 0));
	SetSpeedBoostVelocity ((int16_t) nObject, GetValue (),
								  (int16_t) (pWall ? pWall->nSegment : -1), (int16_t) (pWall ? pWall->nSide : -1),
								  m_segments [0], m_sides [0], NULL, NULL, Flagged (TF_SET_ORIENT) != 0);
	}
}

//------------------------------------------------------------------------------

bool CTrigger::DoExit (int32_t nPlayer)
{
if (nPlayer != N_LOCALPLAYER)
	return false;
if (Flagged (TF_DISABLED))
	return false;
audio.StopAll ();		//kill the sounds
StopSpeedBoost (PLAYER (nPlayer).nObject);
missionManager.AdvanceLevel (X2I (GetValue ()));
if (gameStates.app.bD1Mission) {
	StartEndLevelSequence (0);
	return true;
	}
else if (missionManager.nCurrentLevel > 0) {
	if ((gameData.segData.nLevelVersion > 20) && (missionManager.GetLevelState (missionManager.NextLevel ()) < 0))
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

bool CTrigger::DoSecretExit (int32_t nPlayer)
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

if (gameData.demoData.nState == ND_STATE_RECORDING)			// record whether we're really going to the secret level
	NDRecordSecretExitBlown (bDisabled);

if (bDisabled && (gameData.demoData.nState != ND_STATE_PLAYBACK)) {
	HUDInitMessage (TXT_SECRET_DESTROYED);
	audio.PlaySound (SOUND_BAD_SELECTION);
	return false;
}

if (gameData.demoData.nState == ND_STATE_RECORDING)		// stop demo recording
	gameData.demoData.nState = ND_STATE_PAUSED;
audio.StopAll ();		//kill the sounds
audio.PlaySound (SOUND_SECRET_EXIT);
paletteManager.DisableEffect ();
EnterSecretLevel ();
gameData.reactorData.bDestroyed = 0;
return true;
}

//------------------------------------------------------------------------------

int32_t CTrigger::WallIsForceField (void)
{
for (int32_t i = 0; i < m_nLinks; i++)
	if ((gameData.pigData.tex.pTexMapInfo [SEGMENT (m_segments [i])->m_sides [m_sides [i]].m_nBaseTex].flags & TMI_FORCE_FIELD))
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
	int32_t	nFlag;
	int32_t	nType;
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


int32_t CTrigger::OperateD1 (int16_t nObject, int32_t nPlayer, int32_t bShot)
{
	int32_t	h = 1;
	int32_t	nTrigger = GEOTRIGGERS.Index (this);


for (int32_t i = 0; i < int32_t (sizeofa (xlatTriggers)); i++)
	if (m_info.flagsD1 & xlatTriggers [i].nFlag) {
		Type () = xlatTriggers [i].nType;
		gameData.trigData.delay [nTrigger] = -1;
		if (!Operate (nObject, nPlayer, bShot, false))
			h = 0;
		if ((xlatTriggers [i].nFlag == TRIGGER_EXIT) || (xlatTriggers [i].nFlag == TRIGGER_SECRET_EXIT))
			break;
		}
Type () = TT_DESCENT1;
if (!h)
	gameData.trigData.delay [nTrigger] = gameStates.app.nSDLTicks [0];
return h;
}

//------------------------------------------------------------------------------

#if DBG
static int32_t nDbgTrigger = -1;
static int32_t nDbgType = -1;
#endif

int32_t CTrigger::Operate (int16_t nObject, int32_t nPlayer, int32_t bShot, bool bObjTrigger)
{
	static int32_t nDepth = -1;
#if DBG
if (this - gameData.trigData.triggers [0].Buffer () == nDbgTrigger)
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

CObject*	pObj = OBJECT (nObject);
bool bIsPlayer = pObj && pObj->IsPlayer ();

if (bIsPlayer) {
	if (nObject != LOCALPLAYER.nObject) {
		if (IsMultiGame) {
			if (ClientOnly ()) {
				nDepth--;
				return 1;
				}
			}
		else {
			if (!pObj->IsGuideBot ()) {
				nDepth--;
				return 1;
				}
			}
		}
	}
else {
	nPlayer = -1;
	if (pObj &&
		 (pObj->info.nType != OBJ_ROBOT) &&
		 (pObj->info.nType != OBJ_DEBRIS) && // exploded robot
		 (pObj->info.nType != OBJ_REACTOR) &&
		 (pObj->info.nType != OBJ_HOSTAGE) &&
		 (pObj->info.nType != OBJ_POWERUP))	{		
		nDepth--;
		return 1;
		}
	if (!bObjTrigger && (Type () != TT_TELEPORT) && (Type () != TT_SPEEDBOOST && (Type () != TT_MASTER))) {
		nDepth--;
		return 1;
		}
	}

int32_t nTrigger = bObjTrigger ? OBJTRIGGERS.Index (this) : GEOTRIGGERS.Index (this);
if (nTrigger < 0) {
	nDepth--;
	return 1;
	}

if (!nDepth && !bObjTrigger && (Type () != TT_TELEPORT) && (Type () != TT_SPEEDBOOST)) {
	int32_t t = gameStates.app.nSDLTicks [0];
	if ((gameData.trigData.delay [nTrigger] >= 0) && (t - gameData.trigData.delay [nTrigger] < 750)) {
		nDepth--;
		return 1;
		}
	gameData.trigData.delay [nTrigger] = gameStates.app.nSDLTicks [0];
	}

if (m_info.tOperated < 0) {
	m_info.tOperated = gameData.timeData.xGame;
	m_info.nObject = nObject;
	m_info.nPlayer = nPlayer;
	m_info.bShot = bShot;
	if (IsDelayed ()) {
		if (GetTime (0) > 0)
			SetTime (1, GetTime (0));
		else {
			fix h = -GetTime (0) / 10;
			SetTime (1, h + h * Rand (10));
			}
		}
	}

if (Delay () > 0) {
	gameData.trigData.delay [nTrigger] = -1;
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
		if (!IsMultiGame || gameData.appData.GameMode (GM_MULTI_ROBOTS))
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
			DoTeleportBot (pObj);
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
				LOCALPLAYER.UpdateShield (-GEOTRIGGER (nTrigger)->GetValue ());
			else
				LOCALPLAYER.UpdateShield (-fix ((float (I2X (1)) * X2F (GEOTRIGGER (nTrigger)->GetValue ()))));
			if (LOCALPLAYER.Shield () < 0)
				StartPlayerDeathSequence (OBJECT (LOCALPLAYER.nObject));
			}
		break;

	case TT_ENERGY_DRAIN:
		if (!gameStates.app.bPlayerIsDead) {
			if (gameStates.app.bD1Mission)
				LOCALPLAYER.UpdateEnergy (-GEOTRIGGER (nTrigger)->GetValue ());
			else
				LOCALPLAYER.UpdateEnergy (-fix (LOCALPLAYER.Energy () * X2F (GEOTRIGGER (nTrigger)->GetValue ()) / 100));
			}
		break;

	case TT_CHANGE_TEXTURE:
		DoChangeTexture ();
		PrintMessage (nPlayer, 2, "Walls have been changed!");
		break;

	case TT_SPAWN_BOT:
		DoSpawnBots (pObj);
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
		if (nPlayer == N_LOCALPLAYER)
			DoShowMessage ();
		break;

	case TT_SOUND:
		if (DoPlaySound (nObject))
			m_info.tOperated = gameData.timeData.xGame;
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
if ((this - gameData.trigData.triggers [0].Buffer () == nDbgTrigger) && (nDbgType == Type ()))
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

int32_t CTrigger::Delay (void) 
{ 
if (!IsDelayed ())
	return -1;
return gameData.timeData.xGame - m_info.tOperated < MSEC2X (GetTime (1));
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
return !WALL (m_info.nWall)->IsSolid (true);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CTrigger::Read (CFile& cf, int32_t bObjTrigger)
{
m_info.Clear ();
Type () = cf.ReadByte ();
if (bObjTrigger)
	SetFlags (cf.ReadUShort ());
else
	SetFlags (uint16_t (cf.ReadByte ()));
m_nLinks = (int16_t) cf.ReadByte ();
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
Type () = uint8_t (cf.ReadByte ());
#if DBG
if (bObjTrigger)
	BRP;
else
	BRP;
#endif
if ((saveGameManager.Version () >= 48) || (bObjTrigger && (saveGameManager.Version () >= 41)))
	SetFlags (cf.ReadUShort ());
else
	SetFlags (uint16_t (cf.ReadByte ()));
m_nLinks = (int16_t) cf.ReadByte ();
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
cf.WriteByte (int8_t (Type ()));
cf.WriteShort (Flags ());
cf.WriteByte ((int8_t) m_nLinks);
cf.WriteFix (GetValue ());
cf.WriteFix (GetTime (0));
CTriggerTargets::SaveState (cf);
cf.WriteFix (m_info.tOperated);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CTrigger *FindObjTrigger (int16_t nObject, int16_t nType, int16_t nTrigger)
{
if (!OBJTRIGGERS.Buffer ())
	return NULL;
if (!gameData.trigData.objTriggerRefs [nObject].nCount)
	return NULL;
if (gameData.trigData.objTriggerRefs [nObject].nFirst >= OBJTRIGGERS.Length ())
	return NULL;
if (nTrigger < 0)
	return OBJTRIGGER (gameData.trigData.objTriggerRefs [nObject].nFirst);
if (++nTrigger - gameData.trigData.objTriggerRefs [nObject].nFirst < gameData.trigData.objTriggerRefs [nObject].nCount)
	return OBJTRIGGER (nTrigger);
return NULL;
}

//------------------------------------------------------------------------------

void ExecObjTriggers (int16_t nObject, int32_t bDamage)
{
if (!OBJTRIGGERS.Buffer ())
	return;

	uint8_t i = gameData.trigData.objTriggerRefs [nObject].nFirst, j = gameData.trigData.objTriggerRefs [nObject].nCount;

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
	CTrigger	*pTrigger;
	int32_t	i;

pTrigger = GEOTRIGGERS.Buffer ();
for (i = gameData.trigData.m_nTriggers [0]; i > 0; i--, pTrigger++) {
#if DBG
	if (pTrigger->Index () == nDbgTrigger)
		BRP;
#endif
	if (!gameStates.app.bD2XLevel)
		continue;
	if (pTrigger->Flagged (TF_DISABLED))
		continue;
	if (pTrigger->Flagged (TF_AUTOPLAY) && (pTrigger->m_info.tOperated < 0) && (pTrigger->IsDelayed () || !pTrigger->Flagged (TF_PERMANENT))) {
		pTrigger->Operate (LOCALPLAYER.nObject, N_LOCALPLAYER, 0, false);
		if (!pTrigger->IsDelayed ())
			pTrigger->SetFlags (TF_DISABLED);
		}
	if ((pTrigger->Type () == TT_SOUND) && 
		 (pTrigger->Flagged (TF_PLAYING_SOUND)) && 
		 (pTrigger->GetTime (1) > 0) &&
		 (gameData.timeData.xGame - pTrigger->m_info.tOperated > pTrigger->GetTime (1))) {
		pTrigger->ClearFlags (TF_PLAYING_SOUND);
		if ((pTrigger->IsFlyThrough () ? !pTrigger->Flagged (TF_ONE_SHOT) : pTrigger->Flagged (TF_PERMANENT))) 
			pTrigger->SetFlags (TF_DISABLED); // sound has been played; make sure it doesn't get played again when laoading a save game that may subsequently be now
		}
	pTrigger->Countdown (false);	
	}

pTrigger = OBJTRIGGERS.Buffer ();
for (i = gameData.trigData.m_nTriggers [1]; i > 0; i--, pTrigger++) {
	if (pTrigger->Flagged (TF_AUTOPLAY) && (pTrigger->m_info.nObject >= 0) && (pTrigger->m_info.tOperated < 0) && (!IsMultiGame || IAmGameHost ())) {
		pTrigger->Operate (pTrigger->m_info.nObject, -1, 0, true);
		pTrigger->SetFlags (TF_DISABLED);
		}
	pTrigger->Countdown (true);	
	}
}

//------------------------------------------------------------------------------

static void StartTriggeredSounds (CArray<CTrigger>& triggers)
{
	CTrigger	*pTrigger = triggers.Buffer ();

for (int32_t i = triggers.Length (); i > 0; i--, pTrigger++)
	if ((pTrigger->Type () == TT_SOUND) && pTrigger->Flagged (TF_PLAYING_SOUND | TF_DISABLED, TF_PLAYING_SOUND) && (pTrigger->m_info.nChannel < 0))
		pTrigger->DoPlaySound (-1);
}

//------------------------------------------------------------------------------

void StartTriggeredSounds (void)
{
StartTriggeredSounds (gameData.trigData.triggers [0]);
StartTriggeredSounds (gameData.trigData.triggers [1]);
}

//------------------------------------------------------------------------------

static void StopTriggeredSounds (CArray<CTrigger>& triggers)
{
	CTrigger	*pTrigger = triggers.Buffer ();

for (int32_t i = triggers.Length (); i > 0; i--, pTrigger++)
	pTrigger->m_info.nChannel = -1;
}

//------------------------------------------------------------------------------

void StopTriggeredSounds (void)
{
StopTriggeredSounds (gameData.trigData.triggers [0]);
StopTriggeredSounds (gameData.trigData.triggers [1]);
}

//------------------------------------------------------------------------------

inline int32_t CTrigger::HasTarget (int16_t nSegment, int16_t nSide)
{
for (int32_t i = 0; i < m_nLinks; i++)
	if ((m_segments [i] == nSegment) && ((nSide < 0) || (m_sides [i] == nSide)))
	return 1;
return 0;
}

//------------------------------------------------------------------------------

CWall *FindTriggerWall (int16_t nTrigger)
{
	int32_t	i;

for (i = 0; i < gameData.wallData.nWalls; i++)
	if (WALL (i)->nTrigger == nTrigger)
		return WALL (i);
return NULL;
}

//------------------------------------------------------------------------------

int32_t FindTriggerSegSide (int16_t nTrigger)
{
	CWall	*pWall = FindTriggerWall (nTrigger);

return pWall ? pWall->nSegment * 65536 + pWall->nSide : -1;
}

//------------------------------------------------------------------------------

int32_t ObjTriggerIsValid (int32_t nTrigger)
{
	CObject	*pObj;
	CTrigger	*pTrigger;

FORALL_OBJS (pObj) {
	int32_t j = pObj->Index ();
	if ((nTrigger >= gameData.trigData.objTriggerRefs [j].nFirst) && 
		 (nTrigger < gameData.trigData.objTriggerRefs [j].nFirst + gameData.trigData.objTriggerRefs [j].nCount) &&
		 (pTrigger = OBJTRIGGER (nTrigger)) &&
		 (pTrigger->m_info.nObject >= 0))
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

int32_t FindTriggerTarget (int16_t nSegment, int16_t nSide, int32_t i)
{
if (i < 0)
	i = gameData.trigData.m_nTriggers [0] - i;

	int32_t	j = i, nSegSide, nOvlTex, ec;

for (; i < gameData.trigData.m_nTriggers [0]; i++) {
	nSegSide = FindTriggerSegSide (i);
	if (nSegSide == -1)
		continue;
	nOvlTex = SEGMENT (nSegSide / 65536)->m_sides [nSegSide & 0xffff].m_nOvlTex;
	if (nOvlTex <= 0)
		continue;
	ec = gameData.pigData.tex.pTexMapInfo [nOvlTex].nEffectClip;
	if (ec < 0) {
		if (gameData.pigData.tex.pTexMapInfo [nOvlTex].destroyed == -1)
			continue;
		}
	else {
		tEffectInfo *pEffectInfo = gameData.effectData.pEffect + ec;
		if (pEffectInfo->flags & EF_ONE_SHOT)
			continue;
		if (pEffectInfo->destroyed.nTexture < 0)
			continue;
		}
	if (GEOTRIGGER (i)->HasTarget (nSegment, nSide))
		return i + 1;
	}
for (j = i - gameData.trigData.m_nTriggers [0]; i < gameData.trigData.m_nTriggers [0] + gameData.trigData.m_nTriggers [1]; i++, j++) {
	CTrigger* pTrigger = OBJTRIGGER (j);
	if (!pTrigger || !pTrigger->HasTarget (nSegment, nSide))
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
	int32_t	i;

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
	int32_t i;

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

int32_t OpenExits (void)
{
	CTrigger *pTrigger = GEOTRIGGERS.Buffer ();
	CWall		*pWall;
	int32_t		nExits = 0;

for (int32_t i = 0; i < gameData.trigData.m_nTriggers [0]; i++, pTrigger++) {
	if (pTrigger->Type () == TT_EXIT) {
		pWall = FindTriggerWall (i);
		if (pWall) {
			SEGMENT (pWall->nSegment)->ToggleWall (pWall->nSide);
			nExits++;
			}
		}
	}
return nExits;
}

//	-----------------------------------------------------------------------------

int32_t FindNextLevel (void)
{
missionManager.SetNextLevel (missionManager.nCurrentLevel + 1);

if (gameData.segData.nLevelVersion > 20) {
	CTrigger *pTrigger = GEOTRIGGERS.Buffer ();
	int32_t nNextLevel = 0x7FFFFFFF;
	int32_t nLevelState = 0x7FFFFFFF;

	for (int32_t i = 0; i < gameData.trigData.m_nTriggers [0]; i++, pTrigger++) {
		if (pTrigger->Type () == TT_EXIT) {
			int32_t l = (X2I (pTrigger->GetValue ()) > 0) ? X2I (pTrigger->GetValue ()) : missionManager.nCurrentLevel + 1;
			int32_t s = missionManager.GetLevelState (l);
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
