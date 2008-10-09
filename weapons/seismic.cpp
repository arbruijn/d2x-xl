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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inferno.h"
#include "error.h"
#include "text.h"
#include "network.h"

//	-----------------------------------------------------------------------------

#define	ESHAKER_SHAKE_TIME		(F1_0*2)
#define	MAX_ESHAKER_DETONATES	4

fix	eshakerDetonateTimes [MAX_ESHAKER_DETONATES];

//	Call this to initialize for a new level.
//	Sets all super mega missile detonation times to 0 which means there aren't any.
void InitShakerDetonates (void)
{
memset (eshakerDetonateTimes, 0, sizeof (eshakerDetonateTimes));
}

//	-----------------------------------------------------------------------------

//	If a smega missile been detonated, rock the mine!
//	This should be called every frame.
//	Maybe this should affect all robots, being called when they get their physics done.
void RockTheMineFrame (void)
{
	int	i;

for (i = 0; i < MAX_ESHAKER_DETONATES; i++) {
	if (eshakerDetonateTimes [i] != 0) {
		fix	deltaTime = gameData.time.xGame - eshakerDetonateTimes [i];
		if (!gameStates.gameplay.seismic.bSound) {
			DigiPlaySampleLooping ((short) gameStates.gameplay.seismic.nSound, F1_0, -1, -1);
			gameStates.gameplay.seismic.bSound = 1;
			gameStates.gameplay.seismic.nNextSoundTime = gameData.time.xGame + d_rand()/2;
			}
		if (deltaTime < ESHAKER_SHAKE_TIME) {
			//	Control center destroyed, rock the tPlayer's ship.
			int	fc, rx, rz;
			fix	h;
			// -- fc = abs(deltaTime - ESHAKER_SHAKE_TIME/2);
			//	Changed 10/23/95 to make decreasing for super mega missile.
			fc = (ESHAKER_SHAKE_TIME - deltaTime) / (ESHAKER_SHAKE_TIME / 16);
			if (fc > 16)
				fc = 16;
			else if (fc == 0)
				fc = 1;
			gameStates.gameplay.seismic.nVolume += fc;
			h = 3 * F1_0 / 16 + (F1_0 * (16 - fc)) / 32;
			rx = FixMul(d_rand() - 16384, h);
			rz = FixMul(d_rand() - 16384, h);
			gameData.objs.console->mType.physInfo.rotVel[X] += rx;
			gameData.objs.console->mType.physInfo.rotVel[Z] += rz;
			//	Shake the buddy!
			if (gameData.escort.nObjNum != -1) {
				OBJECTS[gameData.escort.nObjNum].mType.physInfo.rotVel[X] += rx*4;
				OBJECTS[gameData.escort.nObjNum].mType.physInfo.rotVel[Z] += rz*4;
				}
			//	Shake a guided missile!
			gameStates.gameplay.seismic.nMagnitude += rx;
			} 
		else
			eshakerDetonateTimes [i] = 0;
		}
	}
//	Hook in the rumble sound effect here.
}

//	-----------------------------------------------------------------------------

#define	SEISMIC_DISTURBANCE_DURATION	(F1_0*5)

int SeismicLevel (void)
{
return gameStates.gameplay.seismic.nLevel;
}

//	-----------------------------------------------------------------------------

void InitSeismicDisturbances (void)
{
gameStates.gameplay.seismic.nStartTime = 0;
gameStates.gameplay.seismic.nEndTime = 0;
}

//	-----------------------------------------------------------------------------
//	Return true if time to start a seismic disturbance.
int StartSeismicDisturbance (void)
{
	int	rval;

if (gameStates.gameplay.seismic.nShakeDuration < 1)
	return 0;
rval =  (2 * FixMul(d_rand(), gameStates.gameplay.seismic.nShakeFrequency)) < gameData.time.xFrame;
if (rval) {
	gameStates.gameplay.seismic.nStartTime = gameData.time.xGame;
	gameStates.gameplay.seismic.nEndTime = gameData.time.xGame + gameStates.gameplay.seismic.nShakeDuration;
	if (!gameStates.gameplay.seismic.bSound) {
		DigiPlaySampleLooping((short) gameStates.gameplay.seismic.nSound, F1_0, -1, -1);
		gameStates.gameplay.seismic.bSound = 1;
		gameStates.gameplay.seismic.nNextSoundTime = gameData.time.xGame + d_rand()/2;
		}
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendSeismic (gameStates.gameplay.seismic.nStartTime,gameStates.gameplay.seismic.nEndTime);
	}
return rval;
}

//	-----------------------------------------------------------------------------

void SeismicDisturbanceFrame (void)
{
if (gameStates.gameplay.seismic.nShakeFrequency) {
	if (((gameStates.gameplay.seismic.nStartTime < gameData.time.xGame) && 
		  (gameStates.gameplay.seismic.nEndTime > gameData.time.xGame)) || StartSeismicDisturbance()) {
		fix	deltaTime = gameData.time.xGame - gameStates.gameplay.seismic.nStartTime;
		int	fc, rx, rz;
		fix	h;

		fc = abs(deltaTime - (gameStates.gameplay.seismic.nEndTime - gameStates.gameplay.seismic.nStartTime) / 2);
		fc /= F1_0 / 16;
		if (fc > 16)
			fc = 16;
		else if (fc == 0)
			fc = 1;
		gameStates.gameplay.seismic.nVolume += fc;
		h = 3 * F1_0 / 16 + (F1_0 * (16 - fc)) / 32;
		rx = FixMul(d_rand() - 16384, h);
		rz = FixMul(d_rand() - 16384, h);
		gameData.objs.console->mType.physInfo.rotVel[X] += rx;
		gameData.objs.console->mType.physInfo.rotVel[Z] += rz;
		//	Shake the buddy!
		if (gameData.escort.nObjNum != -1) {
			OBJECTS[gameData.escort.nObjNum].mType.physInfo.rotVel[X] += rx*4;
			OBJECTS[gameData.escort.nObjNum].mType.physInfo.rotVel[Z] += rz*4;
			}
		//	Shake a guided missile!
		gameStates.gameplay.seismic.nMagnitude += rx;
		}
	}
}


//	-----------------------------------------------------------------------------
//	Call this when a smega detonates to start the process of rocking the mine.
void ShakerRockStuff (void)
{
#if !DBG
	int	i;

for (i = 0; i < MAX_ESHAKER_DETONATES; i++)
	if (eshakerDetonateTimes [i] + ESHAKER_SHAKE_TIME < gameData.time.xGame)
		eshakerDetonateTimes [i] = 0;
for (i = 0; i < MAX_ESHAKER_DETONATES; i++)
	if (eshakerDetonateTimes [i] == 0) {
		eshakerDetonateTimes [i] = gameData.time.xGame;
		break;
		}
#endif
}

//	---------------------------------------------------------------------------------------
//	Do seismic disturbance stuff including the looping sounds with changing volume.

void DoSeismicStuff (void)
{
	int		stv_save;

if (gameStates.limitFPS.bSeismic && !gameStates.app.tick40fps.bTick)
	return;
stv_save = gameStates.gameplay.seismic.nVolume;
gameStates.gameplay.seismic.nMagnitude = 0;
gameStates.gameplay.seismic.nVolume = 0;
RockTheMineFrame();
SeismicDisturbanceFrame();
if (stv_save != 0) {
	if (gameStates.gameplay.seismic.nVolume == 0) {
		DigiStopLoopingSound();
		gameStates.gameplay.seismic.bSound = 0;
		}

	if ((gameData.time.xGame > gameStates.gameplay.seismic.nNextSoundTime) && gameStates.gameplay.seismic.nVolume) {
		int volume = gameStates.gameplay.seismic.nVolume * 2048;
		if (volume > F1_0)
			volume = F1_0;
		DigiChangeLoopingVolume (volume);
		gameStates.gameplay.seismic.nNextSoundTime = gameData.time.xGame + d_rand () / 4 + 8192;
		}
	}
}

//	-----------------------------------------------------------------------------
//eof
