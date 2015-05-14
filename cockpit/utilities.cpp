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
#include <stdlib.h>
#include <stdarg.h>

#include "descent.h"
#include "screens.h"
#include "error.h"
#include "newdemo.h"
#include "gamefont.h"
#include "text.h"
#include "network.h"
#include "network_lib.h"
#include "rendermine.h"
#include "transprender.h"
#include "gamepal.h"
#include "gr.h"
#include "ogl_lib.h"
#include "ogl_render.h"
#include "ogl_bitmap.h"
#include "ogl_hudstuff.h"
#include "playerprofile.h"
#include "hud_defs.h"
#include "cockpit.h"
#include "statusbar.h"
#include "slowmotion.h"
#include "automap.h"
#include "hudmsgs.h"
#include "hudicons.h"
#include "radar.h"
#include "visibility.h"
#include "autodl.h"
#include "key.h"
#include "addon_bitmaps.h"
#include "marker.h"

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

//------------------------------------------------------------------------------

char* CGenericCockpit::ftoa (char *pszVal, fix f)
{
	int32_t decimal, fractional;

decimal = X2I (f);
fractional = ((f & 0xffff) * 100) / 65536;
if (fractional < 0)
	fractional = -fractional;
if (fractional > 99)
	fractional = 99;
sprintf (pszVal, "%d.%02d", decimal, fractional);
return pszVal;
}

//------------------------------------------------------------------------------

char* CGenericCockpit::Convert1s (char* s)
{
char* p = s;
while ((p = strchr (p, '1')))
	*p = char (132);
return s;
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::PlayHomingWarning (void)
{
	fix	xBeepDelay;

if (gameStates.app.bEndLevelSequence || gameStates.app.bPlayerIsDead)
	return;
if (LOCALPLAYER.homingObjectDist >= 0) {
	xBeepDelay = LOCALPLAYER.homingObjectDist/128;
	if (xBeepDelay > I2X (1))
		xBeepDelay = I2X (1);
	else if (xBeepDelay < I2X (1)/8)
		xBeepDelay = I2X (1)/8;
	if (m_info.lastWarningBeepTime [0] > gameData.time.xGame)
		m_info.lastWarningBeepTime [0] = 0;
	if (gameData.time.xGame - m_info.lastWarningBeepTime [0] > xBeepDelay / 2) {
		audio.PlaySound (SOUND_HOMING_WARNING);
		m_info.lastWarningBeepTime [0] = gameData.time.xGame;
		}
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::CheckForExtraLife (int32_t nPrevScore)
{
if (LOCALPLAYER.score / EXTRA_SHIP_SCORE != nPrevScore / EXTRA_SHIP_SCORE) {
	LOCALPLAYER.lives += LOCALPLAYER.score / EXTRA_SHIP_SCORE - nPrevScore / EXTRA_SHIP_SCORE;
	PowerupBasic (20, 20, 20, 0, TXT_EXTRA_LIFE);
	int16_t nSound = gameData.objData.pwrUp.info [POW_EXTRA_LIFE].hitSound;
	if (nSound > -1)
		audio.PlaySound (nSound);
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::AddPointsToScore (int32_t points)
{
	int32_t nPrevScore;

m_info.scoreTime += I2X (1) * 2;
cockpit->AddScore (0, points);
cockpit->AddScore (1, points);
if (m_info.scoreTime > I2X (1) * 4)
	m_info.scoreTime = I2X (1) * 4;
if (!points || gameStates.app.cheats.bEnabled)
	return;
if (IsMultiGame && !IsCoopGame)
	return;
nPrevScore = LOCALPLAYER.score;
LOCALPLAYER.score += points;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordPlayerScore (points);
if (IsCoopGame)
	MultiSendScore ();
if (!IsMultiGame)
	CheckForExtraLife (nPrevScore);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::AddBonusPointsToScore (int32_t points)
{
	int32_t nPrevScore;

if (!points || gameStates.app.cheats.bEnabled)
	return;
nPrevScore = LOCALPLAYER.score;
LOCALPLAYER.score += points;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordPlayerScore (points);
if (!IsMultiGame)
	CheckForExtraLife (nPrevScore);
}

//	-----------------------------------------------------------------------------

#if DBG
extern int32_t bSavingMovieFrames;
#else
#define bSavingMovieFrames 0
#endif

//	-----------------------------------------------------------------------------

bool CGenericCockpit::ShowTextGauges (void)
{
return gameOpts->render.cockpit.bTextGauges || gameOpts->render.cockpit.nShipStateLayout || ogl.IsOculusRift ();
}

//	-----------------------------------------------------------------------------
//returns true if pViewer can see CObject

int32_t CGenericCockpit::CanSeeObject (int32_t nObject, int32_t bCheckObjs)
{
if (OBJECT (nObject))
	return 0;

	CHitQuery hitQuery ((bCheckObjs ? FQ_VISIBILITY | FQ_CHECK_OBJS | FQ_TRANSWALL : FQ_VISIBILITY | FQ_TRANSWALL),
							  &gameData.objData.pViewer->info.position.vPos,
							  &OBJECT (nObject)->info.position.vPos,
							  gameData.objData.pViewer->info.nSegment,
							  gameStates.render.cameras.bActive ? -1 : OBJ_IDX (gameData.objData.pViewer),
							  0, 0,
							  ++gameData.physics.bIgnoreObjFlag
							 );

	CHitResult hitResult;

int32_t nHitType = FindHitpoint (hitQuery, hitResult);
return bCheckObjs ? (nHitType == HIT_OBJECT) && (hitResult.nObject == nObject) : (nHitType != HIT_WALL);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DemoRecording (void)
{
if (gameData.demo.nState == ND_STATE_RECORDING) {
	if (LOCALPLAYER.homingObjectDist >= 0)
		NDRecordHomingDistance (LOCALPLAYER.homingObjectDist);

	if (m_info.nEnergy != m_history [0].energy) {
		NDRecordPlayerEnergy (m_history [0].energy, m_info.nEnergy);
		m_history [0].energy = m_info.nEnergy;
		}

	if (gameData.physics.xAfterburnerCharge != m_history [0].afterburner) {
		NDRecordPlayerAfterburner (m_history [0].afterburner, gameData.physics.xAfterburnerCharge);
		m_history [0].afterburner = gameData.physics.xAfterburnerCharge;
		}

	if (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)
		m_history [0].shield = m_info.nShield ^ 1;
	else {
		if (m_info.nShield != m_history [0].shield) {		// Draw the shield gauge
			NDRecordPlayerShield (m_history [0].shield, m_info.nShield);
			m_history [0].shield = m_info.nShield;
			}
		}
	if (LOCALPLAYER.flags != m_history [0].flags) {
		NDRecordPlayerFlags (m_history [0].flags, LOCALPLAYER.flags);
		m_history [0].flags = LOCALPLAYER.flags;
		}
	}
}

//	---------------------------------------------------------------------------------------------------------
//	Call when picked up a laser powerup.
//	If laser is active, set previous weapon [0] to -1 to force redraw.

void CGenericCockpit::UpdateLaserWeaponInfo (void)
{
#if 0
if (m_history [0].weapon [0] == 0)
	if (!(LOCALPLAYER.laserLevel > MAX_LASER_LEVEL && m_history [0].laserLevel <= MAX_LASER_LEVEL))
		m_history [0].weapon [0] = -1;
#endif
}

//------------------------------------------------------------------------------

int32_t CGenericCockpit::WidthPad (char* pszText)
{
	int32_t	w, h, aw;

fontManager.Current ()->StringSize (pszText, w, h, aw);
return ((int32_t) FRound (ScaleX (w) / fontManager.Scale ()) - w) / 2;
}

//------------------------------------------------------------------------------

int32_t CGenericCockpit::HeightPad (void)
{
return (int32_t) FRound (m_info.heightPad / fontManager.Scale ());
}

//------------------------------------------------------------------------------

int32_t CGenericCockpit::WidthPad (int32_t nValue)
{
	char szValue [20];

sprintf (szValue, "%d", nValue);
return WidthPad (szValue);
}

//	-----------------------------------------------------------------------------
