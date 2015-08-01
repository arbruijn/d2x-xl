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
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#include "descent.h"
#include "postprocessing.h"

//-----------------------------------------------------------------------------

int32_t FusionBump (void)
{
if (gameData.fusionData.xAutoFireTime) {
	if (gameData.weaponData.nPrimary != FUSION_INDEX)
		gameData.fusionData.xAutoFireTime = 0;
	else if (gameData.timeData.xGame + gameData.fusionData.xFrameTime / 2 >= gameData.fusionData.xAutoFireTime) {
		gameData.fusionData.xAutoFireTime = 0;
		gameData.laserData.nGlobalFiringCount = 1;
		}
	else {
		static time_t t0 = 0;

		time_t t = gameStates.app.nSDLTicks [0];
		if (t - t0 < 30)
			return 0;
		t0 = t;
		gameData.laserData.nGlobalFiringCount = 0;
		gameData.objData.pConsole->RandomBump (I2X (1) / 8, (gameData.FusionCharge () > I2X (2)) ? gameData.FusionCharge () * 4 : I2X (4));
		}
	}
return 1;
}

//	-----------------------------------------------------------------------------
//	Fire Laser:  Registers a laser fire, and performs special stuff for the fusion
//				    cannon.
void ChargeFusion (void)
{
if ((LOCALPLAYER.Energy () < I2X (2)) && (gameData.fusionData.xAutoFireTime == 0)) {
	gameData.laserData.nGlobalFiringCount = 0;
	}
else {
	gameData.fusionData.xFrameTime += gameData.timeData.xFrame;
	if (!gameData.FusionCharge ())
		LOCALPLAYER.UpdateEnergy (-I2X (2));
	fix h = (gameData.fusionData.xFrameTime <= LOCALPLAYER.Energy ()) ? gameData.fusionData.xFrameTime : LOCALPLAYER.Energy ();
	gameData.SetFusionCharge (gameData.FusionCharge () + h);
	LOCALPLAYER.UpdateEnergy (-h);
	if (LOCALPLAYER.Energy () > 0) 
		gameData.fusionData.xAutoFireTime = gameData.timeData.xGame + gameData.fusionData.xFrameTime / 2 + 1;
	else {
		LOCALPLAYER.SetEnergy (0);
		gameData.fusionData.xAutoFireTime = gameData.timeData.xGame - 1;	//	Fire now!
		}
	if (gameStates.limitFPS.bFusion && !gameStates.app.tick40fps.bTick)
		return;

	float fScale = float (gameData.FusionCharge () >> 11) / 64.0f;
	if (fScale > 0.0f) {
		CFloatVector* pColor = gameData.weaponData.color + FUSION_ID;

		if (gameData.FusionCharge () < I2X (2)) 
			paletteManager.BumpEffect (pColor->Red () * fScale, pColor->Green () * fScale, pColor->Blue () * fScale);
		else 
			paletteManager.BumpEffect (pColor->Blue () * fScale, pColor->Red () * fScale, pColor->Green () * fScale);
		}
	if (gameData.timeData.xGame < gameData.fusionData.xLastSoundTime)		//gametime has wrapped
		gameData.fusionData.xNextSoundTime = gameData.fusionData.xLastSoundTime = gameData.timeData.xGame;
	if (gameData.fusionData.xNextSoundTime < gameData.timeData.xGame) {
		if (gameData.FusionCharge () > I2X (2)) {
			audio.PlaySound (11);
			gameData.objData.pConsole->ApplyDamageToPlayer (gameData.objData.pConsole, RandShort () * 4);
			}
		else {
			CreateAwarenessEvent (gameData.objData.pConsole, WEAPON_ROBOT_COLLISION);
			audio.PlaySound (SOUND_FUSION_WARMUP);
			if (IsMultiGame)
				MultiSendPlaySound (SOUND_FUSION_WARMUP, I2X (1));
				}
		gameData.fusionData.xLastSoundTime = gameData.timeData.xGame;
		gameData.fusionData.xNextSoundTime = gameData.timeData.xGame + I2X (1) / 8 + RandShort () / 4;
		}
	gameData.fusionData.xFrameTime = 0;
	}
}

//-----------------------------------------------------------------------------
//eof
