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
	TT_ENABLE_TRIGGER
	};

//link Links [MAX_WALL_LINKS];
//int Num_links;

void EnterSecretLevel (void);
void ExitSecretLevel (void);
int PSecretLevelDestroyed (void);

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
for (int i = 0; i < m_info.nLinks; i++)
	SEGMENTS [m_info.segments [i]].ToggleWall (m_info.sides [i]);
}

//-----------------------------------------------------------------

void CTrigger::DoChangeTexture (void)
{
	int	baseTex = m_info.value & 0xffff,
			ovlTex = (m_info.value >> 16);

for (int i = 0; i < m_info.nLinks; i++)
	SEGMENTS [m_info.segments [i]].SetTextures (m_info.sides [i], baseTex, ovlTex);
}

//-----------------------------------------------------------------

inline int CTrigger::DoExecObjTrigger (short nObject, int bDamage)
{
	fix	v = 10 - m_info.value;

if (bDamage != ((m_info.nType == TT_TELEPORT) || (m_info.nType == TT_SPAWN_BOT)))
	return 0;
if (!bDamage)
	return 1;
if (v >= 10)
	return 0;
if (fix (OBJECTS [nObject].Damage () * 100) > v * 10)
	return 0;
if (!(m_info.flags & TF_PERMANENT))
	m_info.value = 0;
return 1;
}

//-----------------------------------------------------------------

void CTrigger::DoSpawnBots (CObject* objP)
{
TriggerBotGen (objP, m_info.nLinks ? m_info.segments [0] : -1);
}

//-----------------------------------------------------------------

void CTrigger::DoTeleportBot (CObject* objP)
{
if (m_info.nLinks) {
	short nSegment = m_info.segments [d_rand () % m_info.nLinks];
	if (objP->info.nSegment != nSegment) {
		objP->info.nSegment = nSegment;
		objP->info.position.vPos = SEGMENTS [nSegment].Center ();
		objP->RelinkToSeg (nSegment);
		if (ROBOTINFO (objP->info.nId).bossFlag) {
			int	i = gameData.bosses.Find (objP->Index ());

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
for (int i = 0; i < m_info.nLinks; i++)
	SEGMENTS [m_info.segments [i]].CloseDoor (m_info.sides [i]);
}

//------------------------------------------------------------------------------
//turns lighting on.  returns true if lights were actually turned on. (they
//would not be if they had previously been bShot out).
int CTrigger::DoLightOn (void)
{
	int ret = 0;
	short nSegment, nSide;

for (int i = 0; i < m_info.nLinks; i++) {
	nSegment = m_info.segments [i];
	nSide = m_info.sides [i];
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

for (int i = 0; i < m_info.nLinks; i++) {
	nSegment = m_info.segments [i];
	nSide = m_info.sides [i];
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

for (int i = 0; i < m_info.nLinks; i++) {
	if ((wallP = SEGMENTS [m_info.segments [i]].Wall (m_info.sides [i]))) {
		wallP->flags &= ~WALL_DOOR_LOCKED;
		wallP->keys = KEY_NONE;
		}
	}
}

//------------------------------------------------------------------------------

bool CTrigger::TargetsWall (int nWall)
{
for (int i = 0; i < m_info.nLinks; i++)
	if ((nWall == SEGMENTS [m_info.segments [i]].WallNum (m_info.sides [i])))
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
		WALLS [i].flags |= WALL_WALL_SWITCH;
}

//------------------------------------------------------------------------------
// Locks all doors linked to the switch.
void CTrigger::DoLockDoors (void)
{
	CWall*	wallP;

for (int i = 0; i < m_info.nLinks; i++)
	if ((wallP = SEGMENTS [m_info.segments [i]].Wall (m_info.sides [i])))
		wallP->flags |= WALL_DOOR_LOCKED;
}

//------------------------------------------------------------------------------
// Changes player spawns according to triggers m_info.segments,m_info.sides list

int CTrigger::DoSetSpawnPoints (void)
{
	int 		i, j;
	short		nSegment;

for (i = j = 0; i < MAX_PLAYERS; i++) {
	nSegment = m_info.segments [j];
	TriggerSetOrient (&gameData.multiplayer.playerInit [i].position, nSegment, m_info.sides [j], 1, 0);
	gameData.multiplayer.playerInit [i].nSegment = nSegment;
	gameData.multiplayer.playerInit [i].nSegType = SEGMENTS [nSegment].m_nType;
	if (i == gameData.multiplayer.nLocalPlayer)
		MoveSpawnMarker (&gameData.multiplayer.playerInit [i].position, nSegment);
	j = (j + 1) % m_info.nLinks;
	}
// delete any spawn markers that have been set before passing through this trigger to
// avoid players getting stuck when respawning at that marker
if (0 <= (gameData.marker.nHighlight = SpawnMarkerIndex (-1)))
	DeleteMarker (1);
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoMasterTrigger (short nObject)
{
	CTrigger*	trigP;

for (int i = 0; i < m_info.nLinks; i++)
	if ((trigP = SEGMENTS [m_info.segments [i]].Trigger (m_info.sides [i])))
		trigP->Operate (nObject, gameData.multiplayer.nLocalPlayer, 0, 0);
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoEnableTrigger (void)
{
	CTrigger*	trigP;

for (int i = 0; i < m_info.nLinks; i++)
	if ((trigP = SEGMENTS [m_info.segments [i]].Trigger (m_info.sides [i])))
		trigP->m_info.flags &= ~TF_DISABLED;
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoDisableTrigger (void)
{
	CTrigger*	trigP;

for (int i = 0; i < m_info.nLinks; i++)
	if ((trigP = SEGMENTS [m_info.segments [i]].Trigger (m_info.sides [i])))
		trigP->m_info.flags |= TF_DISABLED;
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoShowMessage (void)
{
ShowGameMessage (gameData.messages, X2I (m_info.value), m_info.time);
return 1;
}

//------------------------------------------------------------------------------

int CTrigger::DoPlaySound (short nObject)
{
	tTextIndex	*indexP = FindTextData (&gameData.sounds, X2I (m_info.value));

if (!indexP)
	return 0;
if (m_info.time < 0) {
	m_info.nChannel = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xffff / 2, -1, -1, -1, -1, I2X (1), indexP->pszText);
	m_info.flags |= TF_PLAYING_SOUND;
	}
else if (gameData.time.xGame - m_info.tOperated < m_info.time) {
	m_info.nChannel = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xffff / 2, 0, 0, m_info.time - 1, -1, I2X (1), indexP->pszText);
	m_info.flags |= TF_PLAYING_SOUND;
	}
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

for (int i = 0; i < m_info.nLinks; i++) {
	segP = SEGMENTS + m_info.segments [i];
	nSide = m_info.sides [i];

	if (segP->m_children [nSide] < 0) {
		if (gameOpts->legacy.bSwitches)
			Warning (TXT_TRIG_SINGLE, m_info.segments [i], nSide, Index ());
		connSegP = NULL;
		nConnSide = -1;
		}
	else {
		connSegP = SEGMENTS + segP->m_children [nSide];
		nConnSide = segP->ConnectedSide (connSegP);
		}
	switch (m_info.nType) {
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
		PrintLog ("WARNING: Wall trigger %d targets non-existant wall @ %d,%d\n", Index (), segP->Index (), nSide);
#endif
		continue;
		}
	connWallP = connSegP->Wall (nConnSide);
	if ((wallP->nType == nNewWallType) && (!connWallP || !bForceField || (connWallP->nType == nNewWallType)))
		continue;		//already in correct state, so skip

	bChanged = 1;
	switch (m_info.nType) {
		case TT_OPEN_WALL:
			if (!bForceField)
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
	static char	pl [2][2] = {"", "s"};		//points to 's' or nothing for plural word

if (!gameStates.app.bD1Mission && ((nPlayer < 0) || (nPlayer == gameData.multiplayer.nLocalPlayer))) {
	if (!(m_info.flags & TF_NO_MESSAGE) && bShot)	// only display if not a silent trigger and the trigger has been shot (not flown through)
		HUDInitMessage (message, pl [m_info.nLinks > 1]);
	}
}

//------------------------------------------------------------------------------

void CTrigger::DoMatCen (int bMessage)
{
	int i, h [3] = {0,0,0};

for (i = 0; i < m_info.nLinks; i++)
	h [TriggerMatCen (m_info.segments [i])]++;
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
for (int i = 0; i < m_info.nLinks; i++)
	SEGMENTS [m_info.segments [i]].IllusionOn (m_info.sides [i]);
}

//------------------------------------------------------------------------------

void CTrigger::DoIllusionOff (void)
{
	CSegment*	segP;
	short			nSide;

for (int i = 0; i < m_info.nLinks; i++) {
	segP = SEGMENTS + m_info.segments [i];
	nSide = m_info.sides [i];
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
an = n.ToAnglesVec ();
av = objP->mType.physInfo.velocity.ToAnglesVec ();
av [PA] -= an [PA];
av [BA] -= an [BA];
av [HA] -= an [HA];
if (nStep) {
	if (nStep > 1) {
		av [PA] /= nStep;
		av [BA] /= nStep;
		av [HA] /= nStep;
		ad = objP->info.position.mOrient.ExtractAnglesVec ();
		ad [PA] += (an [PA] - ad [PA]) / nStep;
		ad [BA] += (an [BA] - ad [BA]) / nStep;
		ad [HA] += (an [HA] - ad [HA]) / nStep;
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
if (m_info.nLinks > 0) {
		int		i;
		short		nSegment, nSide;

	StopSpeedBoost (nObject);
	i = d_rand () % m_info.nLinks;
	nSegment = m_info.segments [i];
	nSide = m_info.sides [i];
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
	gameData.objs.speedBoost [nObject].bBoosted = (m_info.value && (m_info.nLinks > 0));
	SetSpeedBoostVelocity ((short) nObject, m_info.value,
								  (short) (wallP ? wallP->nSegment : -1), (short) (wallP ? wallP->nSide : -1),
								  m_info.segments [0], m_info.sides [0], NULL, NULL, (m_info.flags & TF_SET_ORIENT) != 0);
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
for (int i = 0; i < m_info.nLinks; i++)
	if ((gameData.pig.tex.tMapInfoP [SEGMENTS [m_info.segments [i]].m_sides [m_info.sides [i]].m_nBaseTex].flags & TMI_FORCE_FIELD))
		return 1;
return 0;
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
	{TRIGGER_MATCEN, TT_MATCEN},
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
		m_info.nType = xlatTriggers [i].nType;
		gameData.trigs.delay [nTrigger] = -1;
		if (!Operate (nObject, nPlayer, bShot, false))
			h = 0;
		}
m_info.nType = TT_DESCENT1;
if (!h)
	gameData.trigs.delay [nTrigger] = gameStates.app.nSDLTicks;
return h;
}

//------------------------------------------------------------------------------

int CTrigger::Operate (short nObject, int nPlayer, int bShot, bool bObjTrigger)
{
if (m_info.flags & TF_DISABLED)
	return 1;		//1 means don't send trigger hit to other players

CObject*	objP = OBJECTS + nObject;
bool bIsPlayer = (objP->info.nType == OBJ_PLAYER);

if (bIsPlayer) {
	if (!IsMultiGame && (nObject != LOCALPLAYER.nObject))
		return 1;
	}
else {
	nPlayer = -1;
	if ((m_info.nType != TT_TELEPORT) && (m_info.nType != TT_SPEEDBOOST)) {
		if ((objP->info.nType != OBJ_ROBOT) &&
			 (objP->info.nType != OBJ_REACTOR) &&
			 (objP->info.nType != OBJ_HOSTAGE) &&
			 (objP->info.nType != OBJ_POWERUP))
			return 1;
		if (!bObjTrigger)
			return 1;
		}
	else
		if ((objP->info.nType != OBJ_ROBOT) &&
			 (objP->info.nType != OBJ_REACTOR) &&
			 (objP->info.nType != OBJ_HOSTAGE) &&
			 (objP->info.nType != OBJ_POWERUP))
			return 1;
		}

int nTrigger = bObjTrigger ? OBJTRIGGERS.Index (this) : TRIGGERS.Index (this);

if (!bObjTrigger && (m_info.nType != TT_TELEPORT) && (m_info.nType != TT_SPEEDBOOST)) {
	int t = gameStates.app.nSDLTicks;
	if ((gameData.trigs.delay [nTrigger] >= 0) && (t - gameData.trigs.delay [nTrigger] < 750))
		return 1;
	gameData.trigs.delay [nTrigger] = gameStates.app.nSDLTicks;
	}

if (m_info.flags & TF_ONE_SHOT)		//if this is a one-bShot...
	m_info.flags |= TF_DISABLED;		//..then don't let it happen again

if (m_info.tOperated < 0)
	m_info.tOperated = gameData.time.xGame;
if (Delay ()) {
	m_info.nObject = nObject;
	m_info.nPlayer = nPlayer;
	m_info.bShot = bShot;
	return 0;
	}

switch (m_info.nType) {

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

	case TT_MATCEN:
		if (!(gameData.app.nGameMode & GM_MULTI) || (gameData.app.nGameMode & GM_MULTI_ROBOTS))
			DoMatCen (nPlayer == gameData.multiplayer.nLocalPlayer);
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
				if (nPlayer != gameData.multiplayer.nLocalPlayer)
					break;
				if ((LOCALPLAYER.shields < 0) ||
						gameStates.app.bPlayerIsDead)
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
			if (nPlayer != gameData.multiplayer.nLocalPlayer)
				break;
			if ((LOCALPLAYER.shields < 0) ||
				 gameStates.app.bPlayerIsDead)
				break;
			}
		DoSpeedBoost (nObject);
		if (bIsPlayer)
			PrintMessage (nPlayer, bShot, "Speed Boost!");
		break;

	case TT_SHIELD_DAMAGE:
		if (gameStates.app.bD1Mission)
			LOCALPLAYER.shields -= TRIGGERS [nTrigger].m_info.value;
		else
			LOCALPLAYER.shields -= (fix) (LOCALPLAYER.shields * X2F (TRIGGERS [nTrigger].m_info.value) / 100);
		break;

	case TT_ENERGY_DRAIN:
		if (gameStates.app.bD1Mission)
			LOCALPLAYER.energy -= TRIGGERS [nTrigger].m_info.value;
		else
			LOCALPLAYER.energy -= (fix) (LOCALPLAYER.energy * X2F (TRIGGERS [nTrigger].m_info.value) / 100);
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

	case TT_DESCENT1:
		OperateD1 (nObject, nPlayer, bShot);
		break;

	default:
		Int3 ();
		break;
	}
if (m_info.flags & TF_ALTERNATE)
	m_info.nType = oppTrigTypes [m_info.nType];
return 0;
}

//------------------------------------------------------------------------------

int CTrigger::Delay (void) 
{ 
return (m_info.nType != TT_COUNTDOWN) && (m_info.nType != TT_MESSAGE) && (m_info.nType != TT_SOUND) && 
		 (m_info.time > 0) && (gameData.time.xGame - m_info.tOperated < SECS2X (m_info.time)); 
}

//------------------------------------------------------------------------------

void CTrigger::Countdown (bool bObjTrigger)
{
if ((m_info.tOperated > 0) && (m_info.time > 0) && !Delay ()) {
	Operate (m_info.nObject, m_info.nPlayer, m_info.bShot, bObjTrigger);
	if (m_info.flags & TF_PERMANENT)
		m_info.tOperated = -1;
	else
		m_info.time = 0;
	}
}

//------------------------------------------------------------------------------

void CTrigger::LoadState (CFile& cf, bool bObjTrigger)
{
m_info.nType = (ubyte) cf.ReadByte ();
if (bObjTrigger && (saveGameManager.Version () >= 41))
	m_info.flags = cf.ReadShort ();
else
	m_info.flags = short (cf.ReadByte ());
m_info.nLinks = cf.ReadByte ();
m_info.value = cf.ReadFix ();
m_info.time = cf.ReadFix ();
for (int i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	m_info.segments [i] = cf.ReadShort ();
	m_info.sides [i] = cf.ReadShort ();
	}
m_info.nChannel = -1;
m_info.tOperated = (saveGameManager.Version () < 44) ? -1 : cf.ReadFix ();
}

//------------------------------------------------------------------------------

void CTrigger::SaveState (CFile& cf, bool bObjTrigger)
{
cf.WriteByte (sbyte (m_info.nType));
if (bObjTrigger)
	cf.WriteShort (m_info.flags);
else
	cf.WriteByte (sbyte (m_info.flags));
cf.WriteByte (m_info.nLinks);
cf.WriteFix (m_info.value);
cf.WriteFix (m_info.time);
for (int i = 0; i < MAX_TRIGGER_TARGETS; i++) {
	cf.WriteShort (m_info.segments [i]);
	cf.WriteShort (m_info.sides [i]);
	}
cf.WriteFix (m_info.tOperated);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CTrigger *FindObjTrigger (short nObject, short nType, short nTrigger)
{
if (!OBJTRIGGERS.Buffer ())
	return NULL;

	short i = (nTrigger < 0) ? gameData.trigs.firstObjTrigger [nObject] : gameData.trigs.objTriggerRefs [nTrigger].next;

while (i >= 0) {
	if (gameData.trigs.objTriggerRefs [i].nObject < 0)
		break;
	if (OBJTRIGGERS [i].m_info.nType == nType)
		return OBJTRIGGERS + i;
	i = gameData.trigs.objTriggerRefs [i].next;
	}
return NULL;
}

//------------------------------------------------------------------------------

void ExecObjTriggers (short nObject, int bDamage)
{
if (!OBJTRIGGERS.Buffer ())
	return;

	short	i = gameData.trigs.firstObjTrigger [nObject], j = 0;

while ((i >= 0) && (j < 256)) {
	if (gameData.trigs.objTriggerRefs [i].nObject < 0)
		break;
	if (OBJTRIGGERS [i].DoExecObjTrigger (nObject, bDamage)) {
		OBJTRIGGERS [i].Operate (nObject, -1, 0, 1);
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
	CTrigger	*trigP;

trigP = TRIGGERS.Buffer ();
for (int i = gameData.trigs.m_nTriggers; i > 0; i--, trigP++)
	trigP->Countdown (false);	

trigP = OBJTRIGGERS.Buffer ();
for (i = gameData.trigs.m_nObjTriggers; i > 0; i--, trigP++)
	trigP->Countdown (true);	
}

//------------------------------------------------------------------------------

static void StartTriggeredSounds (CArray<CTrigger>& triggers)
{
	CTrigger	*trigP = triggers.Buffer ();

for (int i = triggers.Length (); i > 0; i--, trigP++)
	if ((trigP->m_info.nType == TT_SOUND) && (trigP->m_info.flags & TF_PLAYING_SOUND) && (trigP->m_info.nChannel < 0))
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
for (int i = 0; i < m_info.nLinks; i++)
	if ((m_info.segments [i] == nSegment) && ((nSide < 0) || (m_info.sides [i] == nSide)))
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
		tEffectClip *ecP = gameData.eff.effectP + ec;
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

m_info.nType = cf.ReadByte ();
if (bObjTrigger)
	m_info.flags = cf.ReadShort ();
else
	m_info.flags = short (cf.ReadByte ());
m_info.nLinks = cf.ReadByte ();
cf.ReadByte ();
m_info.value = cf.ReadFix ();
m_info.time = cf.ReadFix ();
for (i = 0; i < MAX_TRIGGER_TARGETS; i++)
	m_info.segments [i] = cf.ReadShort ();
for (i = 0; i < MAX_TRIGGER_TARGETS; i++)
	m_info.sides [i] = cf.ReadShort ();
m_info.nChannel = -1;
m_info.tOperated = -1;
}

//	-----------------------------------------------------------------------------

int OpenExits (void)
{
	CTrigger *trigP = TRIGGERS.Buffer ();
	CWall		*wallP;
	int		nExits = 0;

for (int i = 0; i < gameData.trigs.m_nTriggers; i++, trigP++) {
	if (trigP->m_info.nType == TT_EXIT) {
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
