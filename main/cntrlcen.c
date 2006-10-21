/* $Id: cntrlcen.c,v 1.14 2003/11/26 12:26:29 btb Exp $ */
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

/*
 *
 * Code for the control center
 *
 * Old Log:
 * Revision 1.2  1995/10/17  13:12:13  allender
 * added param to ai call
 *
 * Revision 1.1  1995/05/16  15:23:27  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/21  14:40:25  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.0  1995/02/27  11:31:25  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.22  1995/02/11  01:56:14  mike
 * robots don't fire cheat.
 *
 * Revision 1.21  1995/02/05  13:39:39  mike
 * fix stupid bug in control center firing timing.
 *
 * Revision 1.20  1995/02/03  17:41:21  mike
 * fix control cen next fire time in multiplayer.
 *
 * Revision 1.19  1995/01/29  13:46:41  mike
 * adapt to new CreateSmallFireballOnObject prototype.
 *
 * Revision 1.18  1995/01/18  16:12:13  mike
 * Make control center aware of a cloaked playerr when he fires.
 *
 * Revision 1.17  1995/01/12  12:53:44  rob
 * Trying to fix a bug with having cntrlcen in robotarchy games.
 *
 * Revision 1.16  1994/12/11  12:37:22  mike
 * make control center smarter about firing at cloaked player, don't fire through self, though
 * it still looks that way due to prioritization problems.
 *
 * Revision 1.15  1994/12/01  11:34:33  mike
 * fix control center shield strength in multiplayer team games.
 *
 * Revision 1.14  1994/11/30  15:44:29  mike
 * make cntrlcen harder at higher levels.
 *
 * Revision 1.13  1994/11/29  22:26:23  yuan
 * Fixed boss bug.
 *
 * Revision 1.12  1994/11/27  23:12:31  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.11  1994/11/23  17:29:38  mike
 * deal with peculiarities going between net and regular game on boss level.
 *
 * Revision 1.10  1994/11/18  18:27:15  rob
 * Fixed some bugs with the last version.
 *
 * Revision 1.9  1994/11/18  17:13:59  mike
 * special case handling for level 8.
 *
 * Revision 1.8  1994/11/15  12:45:28  mike
 * don't let cntrlcen know where a cloaked player is.
 *
 * Revision 1.7  1994/11/08  12:18:37  mike
 * small explosions on control center.
 *
 * Revision 1.6  1994/11/02  17:59:18  rob
 * Changed control centers so they can find people in network games.
 * Side effect of this is that control centers can find cloaked players.
 * (see in-code comments for explanation).
 * Also added network hooks so control center shots 'sync up'.
 *
 * Revision 1.5  1994/10/22  14:13:21  mike
 * Make control center stop firing shortly after player dies.
 * Fix bug: If play from editor and die, tries to initialize non-control center tObject.
 *
 * Revision 1.4  1994/10/20  15:17:30  mike
 * Hack for control center inside boss robot.
 *
 * Revision 1.3  1994/10/20  09:47:46  mike
 * lots stuff.
 *
 * Revision 1.2  1994/10/17  21:35:09  matt
 * Added support for new Control Center/Main Reactor
 *
 * Revision 1.1  1994/10/17  20:24:01  matt
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: cntrlcen.c,v 1.14 2003/11/26 12:26:29 btb Exp $";
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "pstypes.h"
#include "error.h"
#include "mono.h"

#include "inferno.h"
#include "cntrlcen.h"
#include "game.h"
#include "laser.h"
#include "gameseq.h"
#include "ai.h"
#ifdef NETWORK
#include "multi.h"
#endif
#include "wall.h"
#include "object.h"
#include "robot.h"
#include "vclip.h"
#include "fireball.h"
#include "endlevel.h"
#include "network.h"

//@@vmsVector controlcen_gun_points[MAX_CONTROLCEN_GUNS];
//@@vmsVector controlcen_gun_dirs[MAX_CONTROLCEN_GUNS];

int	N_controlcen_guns;

vmsVector	Gun_pos[MAX_CONTROLCEN_GUNS], Gun_dir[MAX_CONTROLCEN_GUNS];

void DoCountdownFrame ();

//	-----------------------------------------------------------------------------
//return the position & orientation of a gun on the control center tObject
void CalcReactorGunPoint (vmsVector *gun_point,vmsVector *gun_dir,tObject *objP,int gun_num)
{
	reactor *reactor;
	vmsMatrix m;

	Assert (objP->nType == OBJ_CNTRLCEN);
	Assert (objP->renderType==RT_POLYOBJ);

	reactor = &gameData.reactor.reactors[objP->id];

	Assert (gun_num < reactor->nGuns);

	//instance gun position & orientation

	VmCopyTransposeMatrix (&m,&objP->orient);

	VmVecRotate (gun_point,&reactor->gun_points[gun_num],&m);
	VmVecInc (gun_point,&objP->pos);
	VmVecRotate (gun_dir,&reactor->gun_dirs[gun_num],&m);
}

//	-----------------------------------------------------------------------------
//	Look at control center guns, find best one to fire at *objP.
//	Return best gun number (one whose direction dotted with vector to player is largest).
//	If best gun has negative dot, return -1, meaning no gun is good.
int CalcBestReactorGun (int num_guns, vmsVector *gun_pos, vmsVector *gun_dir, vmsVector *objpos)
{
	int	i;
	fix	best_dot;
	int	best_gun;

	best_dot = -F1_0*2;
	best_gun = -1;

	for (i=0; i<num_guns; i++) {
		fix			dot;
		vmsVector	gun_vec;

		VmVecSub (&gun_vec, objpos, &gun_pos[i]);
		VmVecNormalizeQuick (&gun_vec);
		dot = VmVecDot (&gun_dir[i], &gun_vec);

		if (dot > best_dot) {
			best_dot = dot;
			best_gun = i;
		}
	}

	Assert (best_gun != -1);		// Contact Mike.  This is impossible.  Or maybe you're getting an unnormalized vector somewhere.

	if (best_dot < 0)
		return -1;
	else
		return best_gun;

}

//how long to blow up on insane
int nAlanPavlishReactorTimes [2][NDL] = {{90,60,45,35,30},{50,45,40,35,30}};

//	-----------------------------------------------------------------------------
//	Called every frame.  If control center been destroyed, then actually do something.
void DoReactorDeadFrame (void)
{
if ((gameData.reactor.nDeadObj != -1) && 
	 (gameData.objs.objects [gameData.reactor.nDeadObj].nType == OBJ_CNTRLCEN) &&
	 (gameData.reactor.countdown.nSecsLeft > 0))
	if (d_rand () < gameData.time.xFrame * 4)
		CreateSmallFireballOnObject (&gameData.objs.objects[gameData.reactor.nDeadObj], F1_0, 1);
if (gameData.reactor.bDestroyed && !gameStates.app.bEndLevelSequence)
	DoCountdownFrame ();
}

//	-----------------------------------------------------------------------------

#define COUNTDOWN_VOICE_TIME fl2f (12.75)

void DoCountdownFrame ()
{
	fix	oldTime;
	int	fc, h, div_scale;
	static fix cdtFrameTime;

if (!gameData.reactor.bDestroyed) {
	cdtFrameTime = 0;
	return;
	}
cdtFrameTime += gameData.time.xRealFrame;
if (gameStates.limitFPS.bCountDown && !gameStates.app.b40fpsTick)
	return;
if (!IS_D2_OEM && !IS_MAC_SHARE && !IS_SHAREWARE) {  // get countdown in OEM and SHAREWARE only
	// On last level, we don't want a countdown.
	if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) && 
		 (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel)) {
		if (!(gameData.app.nGameMode & GM_MULTI))
			return;
		if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
			return;
		}
	}

//	Control center destroyed, rock the player's ship.
fc = gameData.reactor.countdown.nSecsLeft;
if (fc > 16)
	fc = 16;
//	At Trainee, decrease rocking of ship by 4x.
div_scale = 1;
if (gameStates.app.nDifficultyLevel == 0)
	div_scale = 4;
h = 3 * F1_0 / 16 + (F1_0 * (16 - fc)) / 32;
gameData.objs.console->mType.physInfo.rotVel.x += (FixMul (d_rand () - 16384, h)) / div_scale;
gameData.objs.console->mType.physInfo.rotVel.z += (FixMul (d_rand () - 16384, h)) / div_scale;
//	Hook in the rumble sound effect here.
oldTime = gameData.reactor.countdown.nTimer;
if (!TimeStopped ())
	gameData.reactor.countdown.nTimer -= cdtFrameTime;
cdtFrameTime = 0;
gameData.reactor.countdown.nSecsLeft = f2i (gameData.reactor.countdown.nTimer + F1_0 * 7 / 8);
if ((oldTime > COUNTDOWN_VOICE_TIME) && (gameData.reactor.countdown.nTimer <= COUNTDOWN_VOICE_TIME))	{
	DigiPlaySample (SOUND_COUNTDOWN_13_SECS, F3_0);
	}
if (f2i (oldTime + F1_0*7/8) != gameData.reactor.countdown.nSecsLeft) {
	if ((gameData.reactor.countdown.nSecsLeft >= 0) && (gameData.reactor.countdown.nSecsLeft < 10))
		DigiPlaySample ((short) (SOUND_COUNTDOWN_0_SECS + gameData.reactor.countdown.nSecsLeft), F3_0);
	if (gameData.reactor.countdown.nSecsLeft == gameData.reactor.countdown.nTotalTime - 1)
		DigiPlaySample (SOUND_COUNTDOWN_29_SECS, F3_0);
	}						
if (gameData.reactor.countdown.nTimer > 0) {
	fix size,old_size;
	size = (i2f (gameData.reactor.countdown.nTotalTime) - gameData.reactor.countdown.nTimer) / fl2f (0.65);
	old_size = (i2f (gameData.reactor.countdown.nTotalTime) - oldTime) / fl2f (0.65);
	if (size != old_size && (gameData.reactor.countdown.nSecsLeft < (gameData.reactor.countdown.nTotalTime-5)))	// Every 2 seconds!
		DigiPlaySample (SOUND_CONTROL_CENTER_WARNING_SIREN, F3_0);
	}
else {
	int flashValue = f2i (-gameData.reactor.countdown.nTimer * (64 / 4));	// 4 seconds to total whiteness
	if (oldTime > 0)
		DigiPlaySample (SOUND_MINE_BLEW_UP, F1_0);
	PALETTE_FLASH_SET (flashValue,flashValue,flashValue);
	if (gameStates.ogl.palAdd.blue > 64) {
		WINDOS (
			DDGrSetCurrentCanvas (NULL),
			GrSetCurrentCanvas (NULL)
			);
		WINDOS (
			dd_gr_clear_canvas (RGBA_PAL2 (31,31,31)),
			GrClearCanvas (RGBA_PAL2 (31,31,31))
			);														//make screen all white to match palette effect
		ResetCockpit ();								//force cockpit redraw next time
		ResetPaletteAdd ();							//restore palette for death message
		//controlcen->MaxCapacity = gameData.matCens.xFuelMaxAmount;
		//gauge_message ("Control Center Reset");
		DoPlayerDead ();		//kill_player ();
		}																				
	}
}

//	-----------------------------------------------------------------------------
//	Called when control center gets destroyed.
//	This code is common to whether control center is implicitly imbedded in a boss,
//	or is an tObject of its own.
//	if objP == NULL that means the boss was the control center and don't set gameData.reactor.nDeadObj
void DoReactorDestroyedStuff (tObject *objP)
{
	int	i;

if ((gameData.app.nGameMode & GM_MULTI_ROBOTS) && gameData.reactor.bDestroyed)
   return; // Don't allow resetting if control center and boss on same level
// Must toggle walls whether it is a boss or control center.
for (i = 0; i < gameData.reactor.triggers.nLinks; i++)
	WallToggle (gameData.segs.segments + gameData.reactor.triggers.nSegment [i], gameData.reactor.triggers.nSide [i]);
// And start the countdown stuff.
if (!(gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses && extraGameInfo [0].nBossCount))
	gameData.reactor.bDestroyed = 1;
//	If a secret level, delete secret.sgc to indicate that we can't return to our secret level.
if (gameData.missions.nCurrentLevel < 0)
	CFDelete ("secret.sgc", gameFolders.szSaveDir);
if (gameStates.app.nBaseCtrlCenExplTime != DEFAULT_CONTROL_CENTER_EXPLOSION_TIME)
	gameData.reactor.countdown.nTotalTime = gameStates.app.nBaseCtrlCenExplTime + gameStates.app.nBaseCtrlCenExplTime * (NDL-gameStates.app.nDifficultyLevel-1)/2;
else
	gameData.reactor.countdown.nTotalTime = nAlanPavlishReactorTimes [gameStates.app.bD1Mission][gameStates.app.nDifficultyLevel];
gameData.reactor.countdown.nTimer = i2f (gameData.reactor.countdown.nTotalTime);
if (!(gameData.reactor.bPresent && objP)) {
	//Assert (objP == NULL);
	return;
	}
gameData.reactor.nDeadObj = OBJ_IDX (objP);
}

int	LastTime_cc_vis_check = 0;

//	-----------------------------------------------------------------------------
//do whatever this thing does in a frame
void DoReactorFrame (tObject *objP)
{
	int	nBestGun;

	//	If a boss level, then gameData.reactor.bPresent will be 0.
if (!gameData.reactor.bPresent)
	return;

#ifndef NDEBUG
if (!gameStates.app.cheats.bRobotsFiring || (gameStates.app.bGameSuspended & SUSP_ROBOTS))
	return;
#else
if (!gameStates.app.cheats.bRobotsFiring)
	return;
#endif

if (!(gameData.reactor.bHit || gameData.reactor.bSeenPlayer)) {
	if (!(gameData.app.nFrameCount % 8)) {		//	Do every so often...
		vmsVector	vec_to_player;
		fix			dist_to_player;
		int			i;
		tSegment		*segp = &gameData.segs.segments[objP->nSegment];

		// This is a hack.  Since the control center is not processed by
		// ai_do_frame, it doesn't know to deal with cloaked dudes.  It
		// seems to work in single-player mode because it is actually using
		// the value of Believed_player_position that was set by the last
		// person to go through ai_do_frame.  But since a no-robots game
		// never goes through ai_do_frame, I'm making it so the control
		// center can spot cloaked dudes.

		if (gameData.app.nGameMode & GM_MULTI)
			gameData.ai.vBelievedPlayerPos = gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject].pos;

		//	Hack for special control centers which are isolated and not reachable because the
		//	real control center is inside the boss.
		for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
			if (IS_CHILD (segp->children[i]))
				break;
		if (i == MAX_SIDES_PER_SEGMENT)
			return;

		VmVecSub (&vec_to_player, &gameData.objs.console->pos, &objP->pos);
		dist_to_player = VmVecNormalizeQuick (&vec_to_player);
		if (dist_to_player < F1_0*200) {
			gameData.reactor.bSeenPlayer = ObjectCanSeePlayer (objP, &objP->pos, 0, &vec_to_player);
			gameData.reactor.nNextFireTime = 0;
			}
		}			
		return;
	}

//	Periodically, make the reactor fall asleep if player not visible.
if (gameData.reactor.bHit || gameData.reactor.bSeenPlayer) {
	if ((LastTime_cc_vis_check + F1_0*5 < gameData.time.xGame) || (LastTime_cc_vis_check > gameData.time.xGame)) {
		vmsVector	vec_to_player;
		fix			dist_to_player;

		VmVecSub (&vec_to_player, &gameData.objs.console->pos, &objP->pos);
		dist_to_player = VmVecNormalizeQuick (&vec_to_player);
		LastTime_cc_vis_check = gameData.time.xGame;
		if (dist_to_player < F1_0*120) {
			gameData.reactor.bSeenPlayer = ObjectCanSeePlayer (objP, &objP->pos, 0, &vec_to_player);
			if (!gameData.reactor.bSeenPlayer)
				gameData.reactor.bHit = 0;
			}
		}
	}

if ((gameData.reactor.nNextFireTime < 0) && !(gameStates.app.bPlayerIsDead && (gameData.time.xGame > gameStates.app.nPlayerTimeOfDeath+F1_0*2))) {
	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED)
		nBestGun = CalcBestReactorGun (N_controlcen_guns, Gun_pos, Gun_dir, &gameData.ai.vBelievedPlayerPos);
	else
		nBestGun = CalcBestReactorGun (N_controlcen_guns, Gun_pos, Gun_dir, &gameData.objs.console->pos);

	if (nBestGun != -1) {
		int			rand_prob, count;
		vmsVector	vec_to_goal;
		fix			dist_to_player;
		fix			delta_fireTime;

		if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
			VmVecSub (&vec_to_goal, &gameData.ai.vBelievedPlayerPos, &Gun_pos[nBestGun]);
			dist_to_player = VmVecNormalizeQuick (&vec_to_goal);
			} 
		else {
			VmVecSub (&vec_to_goal, &gameData.objs.console->pos, &Gun_pos[nBestGun]);
			dist_to_player = VmVecNormalizeQuick (&vec_to_goal);
			}
		if (dist_to_player > F1_0*300) {
			gameData.reactor.bHit = 0;
			gameData.reactor.bSeenPlayer = 0;
			return;
			}
#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendCtrlcenFire (&vec_to_goal, nBestGun, OBJ_IDX (objP));	
#endif
		CreateNewLaserEasy (&vec_to_goal, &Gun_pos[nBestGun], OBJ_IDX (objP), CONTROLCEN_WEAPON_NUM, 1);
		//	some of time, based on level, fire another thing, not directly at player, so it might hit him if he's constantly moving.
		rand_prob = F1_0/ (abs (gameData.missions.nCurrentLevel)/4+2);
		count = 0;
		while ((d_rand () > rand_prob) && (count < 4)) {
			vmsVector	randvec;

			MakeRandomVector (&randvec);
			VmVecScaleInc (&vec_to_goal, &randvec, F1_0/6);
			VmVecNormalizeQuick (&vec_to_goal);
			#ifdef NETWORK
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendCtrlcenFire (&vec_to_goal, nBestGun, OBJ_IDX (objP));
			#endif
			CreateNewLaserEasy (&vec_to_goal, &Gun_pos[nBestGun], OBJ_IDX (objP), CONTROLCEN_WEAPON_NUM, 0);
			count++;
			}

		delta_fireTime = (NDL - gameStates.app.nDifficultyLevel) * F1_0/4;
		if (gameStates.app.nDifficultyLevel == 0)
			delta_fireTime += F1_0/2;
		if (gameData.app.nGameMode & GM_MULTI) // slow down rate of fire in multi player
			delta_fireTime *= 2;
		gameData.reactor.nNextFireTime = delta_fireTime;
		}
	} 
else
	gameData.reactor.nNextFireTime -= gameData.time.xFrame;
}

//	-----------------------------------------------------------------------------
//	This must be called at the start of each level.
//	If this level contains a boss and mode != multiplayer, don't do control center stuff.  (Ghost out control center tObject.)
//	If this level contains a boss and mode == multiplayer, do control center stuff.
void InitReactorForLevel (void)
{
	int		i;
	tObject	*objP;
	short		cntrlcen_objnum=-1, boss_objnum=-1;

gameStates.gameplay.bMultiBosses = gameStates.app.bD2XLevel && EGI_FLAG (bMultiBosses, 0, 0);
extraGameInfo [0].nBossCount = 0;
for (i=0, objP = gameData.objs.objects; i<=gameData.objs.nLastObject; i++, objP++) {
	if (objP->nType == OBJ_CNTRLCEN) {
		if (cntrlcen_objnum != -1) {
#if TRACE
			con_printf (1, "Warning: Two or more control centers including %i and %i\n", i, cntrlcen_objnum);
#endif
			}				
		else {
			cntrlcen_objnum = i;
			extraGameInfo [0].nBossCount++;
			}
		}

	if ((objP->nType == OBJ_ROBOT) && (gameData.bots.pInfo[objP->id].bossFlag)) {
		extraGameInfo [0].nBossCount++;
		if (boss_objnum != -1) {
#if TRACE
			con_printf (1, "Warning: Two or more bosses including %i and %i\n", i, boss_objnum);
#endif
			}				
		else
			boss_objnum = i;
		}
	}

#ifndef NDEBUG
if (cntrlcen_objnum == -1) {
#if TRACE
	con_printf (1, "Warning: No control center.\n");
#endif
	return;
	}
#endif

if (EGI_FLAG (bDisableReactor, 0, 0) ||
	 ((boss_objnum != -1) && (cntrlcen_objnum != -1) && 
	  !(gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses))) {
	BashToShield (cntrlcen_objnum, "reactor");
	gameData.reactor.bPresent = 0;
	gameData.reactor.bDisabled = 1;
	cntrlcen_objnum = -1;
	extraGameInfo [0].nBossCount--;
	}
else
	gameData.reactor.bDisabled = 0;
if (cntrlcen_objnum != -1) {
//	Compute all gun positions.
	objP = gameData.objs.objects + cntrlcen_objnum;
	N_controlcen_guns = gameData.reactor.reactors [objP->id].nGuns;
	for (i=0; i<N_controlcen_guns; i++)
		CalcReactorGunPoint (Gun_pos+  i, Gun_dir + i, objP, i);
	gameData.reactor.bPresent = 1;

	if (gameData.reactor.nStrength == -1) {		//use old defaults
//	Boost control center strength at higher levels.
		if (gameData.missions.nCurrentLevel >= 0)
			objP->shields = F1_0*200 + (F1_0*200/4) * gameData.missions.nCurrentLevel;
		else
			objP->shields = F1_0*200 - gameData.missions.nCurrentLevel*F1_0*150;
		}
	else {
		objP->shields = i2f (gameData.reactor.nStrength);
	}
}
//	Say the control center has not yet been hit.
gameData.reactor.bHit = 0;
gameData.reactor.bSeenPlayer = 0;
gameData.reactor.nNextFireTime = 0;
gameData.reactor.nDeadObj = -1;
}

//------------------------------------------------------------------------------

void SpecialReactorStuff (void)
{
#if TRACE
con_printf (CON_DEBUG, "Mucking with reactor countdown time.\n");
#endif
if (gameData.reactor.bDestroyed) {
	gameData.reactor.countdown.nTimer += i2f (gameStates.app.nBaseCtrlCenExplTime + (NDL-1-gameStates.app.nDifficultyLevel)*gameStates.app.nBaseCtrlCenExplTime/ (NDL-1));
	gameData.reactor.countdown.nTotalTime = f2i (gameData.reactor.countdown.nTimer)+2;	//	Will prevent "Self destruct sequence activated" message from replaying.
	}
}

#ifndef FAST_FILE_IO
//------------------------------------------------------------------------------
/*
 * reads n reactor structs from a CFILE
 */
extern int ReactorReadN (reactor *r, int n, CFILE *fp)
{
	int i, j;

for (i = 0; i < n; i++) {
	r[i].nModel = CFReadInt (fp);
	r[i].nGuns = CFReadInt (fp);
	for (j = 0; j < MAX_CONTROLCEN_GUNS; j++)
		CFReadVector (r[i].gun_points + j, fp);
	for (j = 0; j < MAX_CONTROLCEN_GUNS; j++)
		CFReadVector (r[i].gun_dirs + j, fp);
	}
return i;
}

//------------------------------------------------------------------------------
/*
 * reads a tReactorTriggers structure from a CFILE
 */
extern int ControlCenterTriggersReadN (tReactorTriggers *cct, int n, CFILE *fp)
{
	int i, j;

for (i = 0; i < n; i++) {
	cct->nLinks = CFReadShort (fp);
	for (j = 0; j < MAX_CONTROLCEN_LINKS; j++)
		cct->nSegment [j] = CFReadShort (fp);
	for (j = 0; j < MAX_CONTROLCEN_LINKS; j++)
		cct->nSide [j] = CFReadShort (fp);
	}
return i;
}
#endif

//------------------------------------------------------------------------------
