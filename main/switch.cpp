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

#include "inferno.h"
#include "gauges.h"
#include "menu.h"
#include "error.h"
#include "gameseg.h"
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
#include "state.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#define MAX_ORIENT_STEPS	10

int oppTrigTypes  [] = {
	TT_CLOSE_DOOR,
	TT_OPEN_DOOR,
	TT_MATCEN,
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
	TT_CHANGE_TEXTURE
	};

//link Links [MAX_WALL_LINKS];
//int Num_links;

void EnterSecretLevel (void);
void ExitSecretLevel (void);
int PSecretLevelDestroyed (void);

#ifdef EDITOR
fix triggerTimeCount=I2X (1);

//-----------------------------------------------------------------
// Initializes all the switches.
void TriggerInit ()
{
	int i;

	gameData.trigs.nTriggers = 0;

for (i = 0; i < MAX_TRIGGERS; i++) {
	TRIGGERS [i].nType = 0;
	TRIGGERS [i].flags = 0;
	TRIGGERS [i].nLinks = 0;
	TRIGGERS [i].value = 0;
	TRIGGERS [i].time = -1;
	}
memset (gameData.trigs.delay, -1, sizeof (gameData.trigs.delay));
}
#endif

//-----------------------------------------------------------------

inline int CTrigger::Index (void)
{
return this - TRIGGERS;
}

//-----------------------------------------------------------------
// Executes a link, attached to a CTrigger.
// Toggles all walls linked to the switch.
// Opens doors, Blasts blast walls, turns off illusions.
void CTrigger::DoLink (void)
{
for (int i = 0; i < nLinks; i++)
	SEGMENTS [segments [i]].ToggleWall (sides [i]);
}

//-----------------------------------------------------------------

void CTrigger::DoChangeTexture (void)
{
	int	baseTex = value & 0xffff,
			ovlTex = (value >> 16);

for (int i = 0; i < nLinks; i++)
	SEGMENTS [segments [i]].SetTextures (sides [i], baseTex, ovlTex);
}

//-----------------------------------------------------------------

inline int CTrigger::DoExecObjTrigger (short nObject, int bDamage)
{
	fix	v = 10 - value;

if (bDamage != ((nType == TT_TELEPORT) || (nType == TT_SPAWN_BOT)))
	return 0;
if (!bDamage)
	return 1;
if (v >= 10)
	return 0;
if ((fix) (ObjectDamage (OBJECTS + nObject) * 100) > v * 10)
	return 0;
if (!(flags & TF_PERMANENT))
	value = 0;
return 1;
}

//-----------------------------------------------------------------

void CTrigger::DoSpawnBots (CObject* objP)
{
TriggerBotGen (objP, nLinks ? segments [0] : -1);
}

//-----------------------------------------------------------------

void CTrigger::DoTeleportBot (CObject* objP)
{
if (nLinks) {
	short nSegment = segments [d_rand () % nLinks];
	if (objP->info.nSegment != nSegment) {
		objP->info.nSegment = nSegment;
		objP->info.position.vPos = SEGMENTS [nSegment].Center ();
		objP->RelinkToSeg (nSegment);
		if (ROBOTINFO (objP->info.nId).bossFlag) {
			int	i = FindBoss (objP->Index ());

			if (i >= 0)
				InitBossData (i, objP->Index ());
			}
		}
	}
}

//------------------------------------------------------------------------------
//close a door
void CTrigger::DoCloseDoor (void)
{
for (int i = 0; i < nLinks; i++)
	SEGMENTS [segments [i]].CloseDoor (sides [i]);
}

//------------------------------------------------------------------------------
//turns lighting on.  returns true if lights were actually turned on. (they
//would not be if they had previously been shot out).
int CTrigger::DoLightOn (void)
{
	int ret = 0;
	short nSegment, nSide;

for (int i = 0; i < nLinks; i++) {
	nSegment = segments [i];
	nSide = sides [i];
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
//would not be if they had previously been shot out).
int CTrigger::DoLightOff (void)
{
	int ret = 0;
	short nSegment, nSide;

for (int i = 0; i < nLinks; i++) {
	nSegment = segments [i];
	nSide = sides [i];
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

for (int i = 0; i < nLinks; i++) {
	if ((wallP = SEGMENTS [segments [i]].Wall (sides [i]))) {
		wallP->flags &= ~WALL_DOOR_LOCKED;
		wallP->keys = KEY_NONE;
		}
	}	
}

//------------------------------------------------------------------------------

bool CTrigger::TargetsWall (int nWall)
{
for (int i = 0; i < nLinks; i++) 
	if ((nWall == SEGMENTS [segments [i]].WallNum (sides [i]))) 
		return true;
return false;
}

//------------------------------------------------------------------------------
// Return trigger number if door is controlled by a Wall switch, else return -1.
int DoorSwitch (int nWall)
{
for (int i = 0; i < gameData.trigs.nTriggers; i++)
	if (TRIGGERS [i].TargetsWall (nWall))
		return i;
return -1;
}

//------------------------------------------------------------------------------

void FlagTriggeredDoors (void)
{
for (int i = 0; i < gameData.walls.nWalls; i++)
	if (DoorSwitch (i) >= 0)
		WALLS [i].flags |= WALL_WALL_SWITCH;
}

//------------------------------------------------------------------------------
// Locks all doors linked to the switch.
void CTrigger::DoLockDoors (void)
{
	CWall*	wallP;

for (int i = 0; i < nLinks; i++) 
	if ((wallP = SEGMENTS [segments [i]].Wall (sides [i])))
		wallP->flags |= WALL_DOOR_LOCKED;
}

//------------------------------------------------------------------------------
// Changes player spawns according to triggers segments,sides list

int CTrigger::DoSetSpawnPoints (void)
{
	int 		i, j;
	short		nSegment;

for (i = j = 0; i < MAX_PLAYERS; i++) {
	nSegment = segments [j];
	TriggerSetOrient (&gameData.multiplayer.playerInit [i].position, nSegment, sides [j], 1, 0);
	gameData.multiplayer.playerInit [i].nSegment = nSegment;
	gameData.multiplayer.playerInit [i].nSegType = SEGMENTS [nSegment].m_nType;
	if (i == gameData.multiplayer.nLocalPlayer)
		MoveSpawnMarker (&gameData.multiplayer.playerInit [i].position, nSegment);
	j = (j + 1) % nLinks;
	}
// delete any spawn markers that have been set before passing through this trigger to 
// avoid players getting stuck when respawning at that marker
if (0 <= (gameData.marker.nHighlight = SpawnMarkerIndex (-1)))
	DeleteMarker (1);
return 1;
}

//------------------------------------------------------------------------------
// Changes player spawns according to triggers segments,sides list

int CTrigger::DoMasterTrigger (short nObject)
{
	CTrigger*	trigP;

for (int i = 0; i < nLinks; i++)
	if ((trigP = SEGMENTS [segments [i]].Trigger (sides [i])))
		trigP->Operate (nObject, gameData.multiplayer.nLocalPlayer, 0, 0);
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoShowMessage (void)
{
ShowGameMessage (gameData.messages, X2I (value), time);
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoPlaySound (short nObject)
{
	tTextIndex	*indexP = FindTextData (&gameData.sounds, X2I (value));

if (!indexP)
	return 0;
if (time < 0)
	audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xffff / 2, -1, -1, -1, -1, I2X (1), indexP->pszText);
else
	audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xffff / 2, 0, 0, time - 1, -1, I2X (1), indexP->pszText);
return 1;
}

//------------------------------------------------------------------------------
// Changes walls pointed to by a CTrigger. returns true if any walls changed
int CTrigger::DoChangeWalls (void)
{
	int 			bChanged = 0;
	short 		nSide, nConnSide;
	int 			nNewWallType;
	CSegment*	segP, * connSegP;
	CWall*		wallP, * connWallP;

for (int i = 0; i < nLinks; i++) {
	segP = SEGMENTS + segments [i];
	nSide = sides [i];

	if (segP->m_children [nSide] < 0) {
		if (gameOpts->legacy.bSwitches)
			Warning (TXT_TRIG_SINGLE, segments [i], nSide, Index ());
		connSegP = NULL;
		nConnSide = -1;
		}
	else {
		connSegP = SEGMENTS + segP->m_children [nSide];
		nConnSide = segP->ConnectedSide (connSegP);
		}
	switch (nType) {
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

	if (!(wallP = segP->Wall (nSide))) {
#if DBG
		PrintLog ("WARNING: Wall trigger %d targets non-existant wall @ %d,%d\n", Index (), segP->Index (), nSide);
#endif
		continue;
		}

	connWallP = connSegP->Wall (nConnSide);
	if ((wallP->nType == nNewWallType) && (!connWallP || (connWallP->nType == nNewWallType)))
		continue;		//already in correct state, so skip

	bChanged = 1;
	switch (nType) {
		case TT_OPEN_WALL:
			if (!(gameData.pig.tex.tMapInfoP [segP->m_sides [nSide].m_nBaseTex].flags & TMI_FORCE_FIELD)) 
				segP->StartCloak (nSide);
			else {
				CFixVector vPos = segP->SideCenter (nSide);
				audio.CreateSegmentSound (SOUND_FORCEFIELD_OFF, segP->Index (), nSide, vPos, 0, I2X (1));
				wallP->nType = nNewWallType;
				audio.DestroySegmentSound (segP->Index (), nSide, SOUND_FORCEFIELD_HUM);
				if (connWallP) {
					connWallP->nType = nNewWallType;
					audio.DestroySegmentSound (SEG_IDX (connSegP), nConnSide, SOUND_FORCEFIELD_HUM);
					}
				}
			break;

		case TT_CLOSE_WALL:
			if (!(gameData.pig.tex.tMapInfoP [segP->m_sides [nSide].m_nBaseTex].flags & TMI_FORCE_FIELD)) 
				segP->StartDecloak (nSide);
			else {
				CFixVector vPos = segP->SideCenter (nSide);
				audio.CreateSegmentSound (SOUND_FORCEFIELD_HUM, segP->Index (), nSide, vPos, 1, I2X (1)/2);
				wallP->nType = nNewWallType;
				if (connWallP)
					connWallP->nType = nNewWallType;
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

void CTrigger::PrintMessage (int nPlayer, int shot, const char *message)
{
	static char	pl [2][2] = {"", "s"};		//points to 's' or nothing for plural word

if ((nPlayer < 0) || (nPlayer == gameData.multiplayer.nLocalPlayer)) {
	if (!(flags & TF_NO_MESSAGE) && shot)
		HUDInitMessage (message, pl [nLinks > 1]);
	}
}

//------------------------------------------------------------------------------

void CTrigger::DoMatCen (int bMessage)
{
	int i, h [3] = {0,0,0};

for (i = 0; i < nLinks; i++)
	h [TriggerMatCen (segments [i])]++;
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
for (int i = 0; i < nLinks; i++)
	SEGMENTS [segments [i]].IllusionOn (sides [i]);
}

//------------------------------------------------------------------------------

void CTrigger::DoIllusionOff (void)
{
	CSegment*	segP;
	short			nSide;

for (int i = 0; i < nLinks; i++) {
	segP = SEGMENTS + segments [i];
	nSide = sides [i];
	segP->IllusionOff (nSide);
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
	n [Y] = -n [Y];
	n [Z] = -n [Z];
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
an = n.ToAnglesVec();
// create new orientation matrix
if (!nStep)
	posP->mOrient = CFixMatrix::Create(an);
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
	n [X] = -n [X];
	n [Y] = -n [Y];
	*/
	n = -n;
	gameStates.gameplay.vTgtDir = n;
	if (nStep < 0)
		nStep = MAX_ORIENT_STEPS;
	}
else
	n = gameStates.gameplay.vTgtDir;
an = n.ToAnglesVec();
av = objP->mType.physInfo.velocity.ToAnglesVec();
av [PA] -= an [PA];
av [BA] -= an [BA];
av [HA] -= an [HA];
if (nStep) {
	if (nStep > 1) {
		av [PA] /= nStep;
		av [BA] /= nStep;
		av [HA] /= nStep;
		ad = objP->info.position.mOrient.ExtractAnglesVec();
		ad [PA] += (an [PA] - ad [PA]) / nStep;
		ad [BA] += (an [BA] - ad [BA]) / nStep;
		ad [HA] += (an [HA] - ad [HA]) / nStep;
		objP->info.position.mOrient = CFixMatrix::Create(ad);
		}
	else
		objP->info.position.mOrient = CFixMatrix::Create(an);
	}
rm = CFixMatrix::Create(av);
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
if (nLinks > 0) {
		int		i;
		short		nSegment, nSide;

	StopSpeedBoost (nObject);
	i = d_rand () % nLinks;
	nSegment = segments [i];
	nSide = sides [i];
	// set new CPlayerData direction, facing the destination nSide
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
v = 60 + (COMPETITION ? 100 : extraGameInfo [IsMultiGame].nSpeedBoost) * 4 * speed;
if (sbd.bBoosted) {
	if (pSrcPt && pDestPt) {
		n = *pDestPt - *pSrcPt;
		CFixVector::Normalize(n);
		}
	else if (srcSegnum >= 0) {
		sbd.vSrc = SEGMENTS [srcSegnum].SideCenter (srcSidenum);
		sbd.vDest = SEGMENTS [destSegnum].SideCenter (destSidenum);
		if (memcmp (&sbd.vSrc, &sbd.vDest, sizeof (CFixVector))) {
			n = sbd.vDest - sbd.vSrc;
			CFixVector::Normalize(n);
			}
		else {
			Controls [0].verticalThrustTime =
			Controls [0].forwardThrustTime =
			Controls [0].sidewaysThrustTime = 0;
			memcpy (&n, SEGMENTS [destSegnum].m_sides [destSidenum].m_normals, sizeof (n));
		// turn the ship so that it is facing the destination nSide of the destination CSegment
		// Invert the Normal as it points into the CSegment
			/*
			n [X] = -n [X];
			n [Y] = -n [Y];
			*/
			n = -n;
			}
		}
	else {
		memcpy (&n, SEGMENTS [destSegnum].m_sides [destSidenum].m_normals, sizeof (n));
	// turn the ship so that it is facing the destination nSide of the destination CSegment
	// Invert the Normal as it points into the CSegment
		/*
		n [X] = -n [X];
		n [Y] = -n [Y];
		*/
		n = -n;
		}
	sbd.vVel [X] = n [X] * v;
	sbd.vVel [Y] = n [Y] * v;
	sbd.vVel [Z] = n [Z] * v;
#if 0
	d = (double) (labs (n [X]) + labs (n [Y]) + labs (n [Z])) / ((double) I2X (60));
	h [X] = n [X] ? (fix) ((double) n [X] / d) : 0;
	h [Y] = n [Y] ? (fix) ((double) n [Y] / d) : 0;
	h [Z] = n [Z] ? (fix) ((double) n [Z] / d) : 0;
#else
#	if 1
	h [X] =
	h [Y] =
	h [Z] = I2X (60);
#	else
	h [X] = (n [X] ? n [X] : I2X (1)) * 60;
	h [Y] = (n [Y] ? n [Y] : I2X (1)) * 60;
	h [Z] = (n [Z] ? n [Z] : I2X (1)) * 60;
#	endif
#endif
	sbd.vMinVel = sbd.vVel - h;
/*
	if (!sbd.vMinVel [X])
		sbd.vMinVel [X] = -I2X (60);
	if (!sbd.vMinVel [Y])
		sbd.vMinVel [Y] = -I2X (60);
	if (!sbd.vMinVel [Z])
		sbd.vMinVel [Z] = -I2X (60);
*/
	sbd.vMaxVel = sbd.vVel + h;
/*
	if (!sbd.vMaxVel [X])
		sbd.vMaxVel [X] = I2X (60);
	if (!sbd.vMaxVel [Y])
		sbd.vMaxVel [Y] = I2X (60);
	if (!sbd.vMaxVel [Z])
		sbd.vMaxVel [Z] = I2X (60);
*/
	if (sbd.vMinVel [X] > sbd.vMaxVel [X]) {
		fix h = sbd.vMinVel [X];
		sbd.vMinVel [X] = sbd.vMaxVel [X];
		sbd.vMaxVel [X] = h;
		}
	if (sbd.vMinVel [Y] > sbd.vMaxVel [Y]) {
		fix h = sbd.vMinVel [Y];
		sbd.vMinVel [Y] = sbd.vMaxVel [Y];
		sbd.vMaxVel [Y] = h;
		}
	if (sbd.vMinVel [Z] > sbd.vMaxVel [Z]) {
		fix h = sbd.vMinVel [Z];
		sbd.vMinVel [Z] = sbd.vMaxVel [Z];
		sbd.vMaxVel [Z] = h;
		}
	objP->mType.physInfo.velocity = sbd.vVel;
	if (bSetOrient) {
		TriggerSetObjOrient (nObject, destSegnum, destSidenum, 0, -1);
		gameStates.gameplay.nDirSteps = MAX_ORIENT_STEPS - 1;
		}
	gameData.objs.speedBoost [nObject] = sbd;
	}
else {
	objP->mType.physInfo.velocity [X] = objP->mType.physInfo.velocity [X] / v * 60;
	objP->mType.physInfo.velocity [Y] = objP->mType.physInfo.velocity [Y] / v * 60;
	objP->mType.physInfo.velocity [Z] = objP->mType.physInfo.velocity [Z] / v * 60;
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
if (!(COMPETITION || IsCoopGame) || extraGameInfo [IsMultiGame].nSpeedBoost) {
	CWall* wallP = TriggerParentWall (Index ());
	gameData.objs.speedBoost [nObject].bBoosted = (value && (nLinks > 0));
	SetSpeedBoostVelocity ((short) nObject, value, 
								  (short) (wallP ? wallP->nSegment : -1), (short) (wallP ? wallP->nSide : -1),
								  segments [0], sides [0], NULL, NULL, (flags & TF_SET_ORIENT) != 0);
	}
}

//------------------------------------------------------------------------------

bool CTrigger::DoExit (int nPlayer)
{
if (nPlayer != gameData.multiplayer.nLocalPlayer)
	return false;
audio.StopAll ();		//kill the sounds
StopSpeedBoost (gameData.multiplayer.players [nPlayer].nObject);
if ((gameData.missions.nCurrentLevel > 0) || gameStates.app.bD1Mission) {
	StartEndLevelSequence (0);
	return true;
	} 
else if (gameData.missions.nCurrentLevel < 0) {
	if ((LOCALPLAYER.shields < 0) || gameStates.app.bPlayerIsDead)
		return false;
	ExitSecretLevel ();
	return true;
	}
return false;
}

//------------------------------------------------------------------------------

bool CTrigger::DoSecretExit (int nPlayer)
{
if (nPlayer != gameData.multiplayer.nLocalPlayer)
	return false;
if ((LOCALPLAYER.shields < 0) || gameStates.app.bPlayerIsDead)
	return false;
if (gameData.app.nGameMode & GM_MULTI) {
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
for (int i = 0; i < nLinks; i++)
	if ((gameData.pig.tex.tMapInfoP [SEGMENTS [segments [i]].m_sides [sides [i]].m_nBaseTex].flags & TMI_FORCE_FIELD))
		return 1;
return 0;
}

//------------------------------------------------------------------------------

int CTrigger::Operate (short nObject, int nPlayer, int shot, bool bObjTrigger)
{
if (flags & TF_DISABLED)
	return 1;		//1 means don't send trigger hit to other players

CObject*	objP = OBJECTS + nObject;
bool bIsPlayer = (objP->info.nType == OBJ_PLAYER);

if (bIsPlayer) {
	if (!IsMultiGame && (nObject != LOCALPLAYER.nObject))
		return 1;
	}
else {
	nPlayer = -1;
	if ((nType != TT_TELEPORT) && (nType != TT_SPEEDBOOST)) {
		if ((objP->info.nType != OBJ_ROBOT) && (objP->info.nType != OBJ_REACTOR))
			return 1;
		if (!bObjTrigger)
			return 1;
		}
	else
		if ((objP->info.nType != OBJ_ROBOT) && (objP->info.nType != OBJ_REACTOR))
			return 1;
		}

int nTrigger = Index ();

if (!bObjTrigger && (nType != TT_TELEPORT) && (nType != TT_SPEEDBOOST)) {
	int t = gameStates.app.nSDLTicks;
	if ((gameData.trigs.delay [nTrigger] >= 0) && (t - gameData.trigs.delay [nTrigger] < 750))
		return 1;
	gameData.trigs.delay [nTrigger] = t;
	}

if (flags & TF_ONE_SHOT)		//if this is a one-shot...
	flags |= TF_DISABLED;		//..then don't let it happen again

switch (nType) {

	case TT_EXIT:
		if (DoExit (nPlayer))
			return 1;
		break;

	case TT_SECRET_EXIT:
		if (DoSecretExit (nPlayer))
			return 1;
		break;

	case TT_OPEN_DOOR:
		DoLink ();
		PrintMessage (nPlayer, shot, "Door%s opened!");
		break;

	case TT_CLOSE_DOOR:
		DoCloseDoor ();
		PrintMessage (nPlayer, shot, "Door%s closed!");
		break;

	case TT_UNLOCK_DOOR:
		DoUnlockDoors ();
		PrintMessage (nPlayer, shot, "Door%s unlocked!");
		break;

	case TT_LOCK_DOOR:
		DoLockDoors ();
		PrintMessage (nPlayer, shot, "Door%s locked!");
		break;

	case TT_OPEN_WALL:
		if (DoChangeWalls ()) {
			if (WallIsForceField ())
				PrintMessage (nPlayer, shot, "Force field%s deactivated!");
			else
				PrintMessage (nPlayer, shot, "Wall%s opened!");
			}
		break;

	case TT_CLOSE_WALL:
		if (DoChangeWalls ()) {
			if (WallIsForceField ())
				PrintMessage (nPlayer, shot, "Force field%s activated!");
			else
				PrintMessage (nPlayer, shot, "Wall%s closed!");
		}
		break;

	case TT_ILLUSORY_WALL:
		//don't know what to say, so say nothing
		DoChangeWalls ();
		PrintMessage (nPlayer, shot, "Creating Illusion!");
		break;

	case TT_MATCEN:
		if (!(gameData.app.nGameMode & GM_MULTI) || (gameData.app.nGameMode & GM_MULTI_ROBOTS))
			DoMatCen (nPlayer == gameData.multiplayer.nLocalPlayer);
		break;

	case TT_ILLUSION_ON:
		DoIllusionOn ();
		PrintMessage (nPlayer, shot, "Illusion%s on!");
		break;

	case TT_ILLUSION_OFF:
		DoIllusionOff ();
		PrintMessage (nPlayer, shot, "Illusion%s off!");
		break;

	case TT_LIGHT_OFF:
		if (DoLightOff ())
			PrintMessage (nPlayer, shot, "Lights off!");
		break;

	case TT_LIGHT_ON:
		if (DoLightOn ())
			PrintMessage (nPlayer, shot, "Lights on!");
		break;

	case TT_TELEPORT:
		if (bObjTrigger) {
			DoTeleportBot (objP);
			PrintMessage (nPlayer, shot, "Robot is fleeing!");
			}
		else {
			if (bIsPlayer) {
				if (nPlayer != gameData.multiplayer.nLocalPlayer)
					break;
				if ((LOCALPLAYER.shields < 0) || 
						gameStates.app.bPlayerIsDead)
					break;
				}
			audio.PlaySound (SOUND_SECRET_EXIT, I2X (1));
			DoTeleport (nObject);
			if (bIsPlayer)
				PrintMessage (nPlayer, shot, "Teleport!");
			}
		break;

	case TT_SPEEDBOOST:
		if (bIsPlayer) {
			if (nPlayer != gameData.multiplayer.nLocalPlayer)
				break;
			if ((LOCALPLAYER.shields < 0) || 
				 gameStates.app.bPlayerIsDead)
				break;
			}
		DoSpeedBoost (nObject);
		if (bIsPlayer)
			PrintMessage (nPlayer, shot, "Speed Boost!");
		break;

	case TT_SHIELD_DAMAGE:
		if (gameStates.app.bD1Mission)
			LOCALPLAYER.shields += TRIGGERS [nTrigger].value;
		else
			LOCALPLAYER.shields += (fix) (LOCALPLAYER.shields * X2F (TRIGGERS [nTrigger].value) / 100);
		break;

	case TT_ENERGY_DRAIN:
		if (gameStates.app.bD1Mission)
			LOCALPLAYER.energy += TRIGGERS [nTrigger].value;
		else
			LOCALPLAYER.energy += (fix) (LOCALPLAYER.energy * X2F (TRIGGERS [nTrigger].value) / 100);
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
		DoPlaySound (nObject);
		break;

	case TT_MASTER:
		DoMasterTrigger (nObject);

	default:
		Int3 ();
		break;
	}
if (flags & TF_ALTERNATE)
	nType = oppTrigTypes [nType];
return 0;
}

//------------------------------------------------------------------------------

void CTrigger::LoadState (CFile& cf, bool bObjTrigger)
{
nType = (ubyte) cf.ReadByte (); 
if (bObjTrigger && (saveGameHandler.Version () >= 41))
	flags = cf.ReadShort (); 
else
	flags = short (cf.ReadByte ()); 
nLinks = cf.ReadByte ();
value = cf.ReadFix ();
time = cf.ReadFix ();
for (int i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	segments [i] = cf.ReadShort ();
	sides [i] = cf.ReadShort ();
	}
}

//------------------------------------------------------------------------------

void CTrigger::SaveState (CFile& cf, bool bObjTrigger)
{
cf.WriteByte (sbyte (nType)); 
if (bObjTrigger)
	cf.WriteShort (flags); 
else
	cf.WriteByte (sbyte (flags)); 
cf.WriteByte (nLinks);
cf.WriteFix (value);
cf.WriteFix (time);
for (int i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	cf.WriteShort (segments [i]);
	cf.WriteShort (sides [i]);
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CTrigger *FindObjTrigger (short nObject, short nType, short nTrigger)
{
	short i = (nTrigger < 0) ? gameData.trigs.firstObjTrigger [nObject] : gameData.trigs.objTriggerRefs [nTrigger].next;

while (i >= 0) {
	if (gameData.trigs.objTriggerRefs [i].nObject < 0)
		break;
	if (OBJTRIGGERS [i].nType == nType)
		return OBJTRIGGERS + i;
	i = gameData.trigs.objTriggerRefs [i].next;
	}
return NULL;
}

//------------------------------------------------------------------------------

void ExecObjTriggers (short nObject, int bDamage)
{
	short		i = gameData.trigs.firstObjTrigger [nObject], j = 0;

while ((i >= 0) && (j < 256)) {
	if (gameData.trigs.objTriggerRefs [i].nObject < 0)
		break;
	if (OBJTRIGGERS [i].DoExecObjTrigger (nObject, bDamage)) {
		OBJTRIGGERS [i].Operate (nObject, -1, 1, 1);
		if (IsMultiGame)
			MultiSendObjTrigger (i);
		}
	if (!bDamage)
		gameData.trigs.objTriggerRefs [i].nObject = -1;
	i = gameData.trigs.objTriggerRefs [i].next;
	j++;
	}
}

//------------------------------------------------------------------------------

void TriggersFrameProcess (void)
{
	int		i;
	CTrigger	*trigP = TRIGGERS.Buffer ();

for (i = gameData.trigs.nTriggers; i > 0; i--, trigP++)
	if ((trigP->nType != TT_COUNTDOWN) && (trigP->nType != TT_MESSAGE) && (trigP->nType != TT_SOUND) && (trigP->time >= 0))
		trigP->time -= gameData.time.xFrame;
}

//------------------------------------------------------------------------------

inline int CTrigger::HasTarget (short nSegment, short nSide)
{
for (int i = 0; i < nLinks; i++)
	if ((segments [i] == nSegment) && ((nSide < 0) || (sides [i] == nSide)))
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
	int		h, j;
	//int		i;
	CObject	*objP;

FORALL_OBJS (objP, i) {
	j = gameData.trigs.firstObjTrigger [objP->Index ()];
	if (j < 0)
		continue;
	if (gameData.trigs.objTriggerRefs [j].nObject < 0)
		continue;
	h = 0;
	while ((j >= 0) && (h < 256)) {
		if (j == nTrigger)
			return 1;
		j = gameData.trigs.objTriggerRefs [j].next;
		h++;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int FindTriggerTarget (short nSegment, short nSide)
{
	int	i, nSegSide, nOvlTex, ec;

for (i = 0; i < gameData.trigs.nTriggers; i++) {
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
		tEffectClip *ecP = gameData.eff.effectP + ec;
		if (ecP->flags & EF_ONE_SHOT)
			continue;
		if (ecP->nDestBm < 0)
			continue;
		}
	if (TRIGGERS [i].HasTarget (nSegment, nSide))
		return i + 1;
	}
for (i = 0; i < gameData.trigs.nObjTriggers; i++) {
	if (!OBJTRIGGERS [i].HasTarget (nSegment, nSide))
		continue;
	if (!ObjTriggerIsValid (i))
		continue;
	return -i - 1;
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
		TT_MATCEN,
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

//------------------------------------------------------------------------------

/*
 * reads a CTrigger structure from a CFile
 */
void CTrigger::Read (CFile& cf, int bObjTrigger)
{
	int i;

nType = cf.ReadByte ();
if (bObjTrigger)
	flags = cf.ReadShort ();
else
	flags = short (cf.ReadByte ());
nLinks = cf.ReadByte ();
cf.ReadByte ();
value = cf.ReadFix ();
time = cf.ReadFix ();
for (i = 0; i < MAX_TRIGGER_TARGETS; i++)
	segments [i] = cf.ReadShort ();
for (i = 0; i < MAX_TRIGGER_TARGETS; i++)
	sides [i] = cf.ReadShort ();
}

//	-----------------------------------------------------------------------------

int OpenExits (void)
{
	CTrigger *trigP = TRIGGERS.Buffer ();
	CWall		*wallP;
	int		nExits = 0;

for (int i = 0; i < gameData.trigs.nTriggers; i++, trigP++) {
	if (trigP->nType == TT_EXIT) {
		wallP = FindTriggerWall (i);
		if (wallP) {
			SEGMENTS [wallP->nSegment].ToggleWall (wallP->nSide);
			nExits++;
			}
		}
	}
return nExits;
}

//------------------------------------------------------------------------------
//eof
