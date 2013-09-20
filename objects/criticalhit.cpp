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
#include "objeffects.h"
#include "multi.h"

#define REPAIR_DELAY	1500

//------------------------------------------------------------------------------

CFixVector CObject::RegisterHit (CFixVector vHit, short nModel)
{
if (gameStates.app.bNostalgia)
	return vHit;
if ((info.nType != OBJ_ROBOT) && (info.nType != OBJ_PLAYER) && (info.nType != OBJ_REACTOR))
	return vHit;

#if 0 //DBG
HUDMessage (0, "set hit point %d,%d,%d", vHit [0], vHit [1], vHit [2]);
#endif

CFixVector vDir;
vDir = vHit - info.position.vPos;
CFixVector::Normalize (vDir);
m_damage.tRepaired = gameStates.app.nSDLTicks [0];

if (EGI_FLAG (nDamageModel, 0, 0, 0) && (gameStates.app.nSDLTicks [0] > m_damage.tCritical)) {	// check and handle critical hits
	float fDamage = (1.0f - Damage ()) / float (sqrt (DamageRate ()));
	if ((m_damage.bCritical = RandShort () < F2X (fDamage))) {
		if (!EGI_FLAG (nHitboxes, 0, 0, extraGameInfo [0].nHitboxes))
			nModel = RandShort () > 3 * I2X (1) / 8;	// 75% chance for a torso hit with sphere based collision handling
#if DBG
		if (nModel < 2)
			HUDMessage (0, "crit. hit AIM\n");
		else if (CFixVector::Dot (info.position.mOrient.m.dir.f, vDir) < -I2X (1) / 8)
			HUDMessage (0, "crit. hit DRIVES\n");
		else
			HUDMessage (0, "crit. hit GUNS\n");
#endif
#if 1
		if (nModel < 2)
			m_damage.nHits [0]++;
		else if (CFixVector::Dot (info.position.mOrient.m.dir.f, vDir) < -I2X (1) / 8)
			m_damage.nHits [1]++;
		else
			m_damage.nHits [2]++;
#endif
		m_damage.tCritical = gameStates.app.nSDLTicks [0];
		m_damage.nCritical++;
		return vHit;
		}
	}

// avoid the shield effect lighting up to soon after a critical hit
#if 1
if (gameStates.app.nSDLTicks [0] - m_damage.tCritical < SHIELD_EFFECT_TIME / 4)
	return vHit;
#else
if ((gameStates.app.nSDLTicks [0] - m_damage.tShield < SHIELD_EFFECT_TIME * 2) &&
	 (gameStates.app.nSDLTicks [0] - m_damage.tCritical < SHIELD_EFFECT_TIME / 4))
	return vHit;
#endif

vHit = vDir * info.xSize;
m_damage.tShield = gameStates.app.nSDLTicks [0];

for (int i = 0; i < 3; i++)
#if 1
#	if 0
	if (CFixVector::Dot (m_hitInfo.dir [i], vHit) > I2X (1) - I2X (1) / 32) {
#	else
	if (CFixVector::Dist (m_hitInfo.v [i], vHit) < I2X (1) / 16) {
#	endif
		vHit = CFixVector::Avg (m_hitInfo.v [i], vHit);
		CFixVector::Normalize (vHit);
		vHit *= info.xSize;
		m_hitInfo.v [m_hitInfo.i] = vHit;
		m_hitInfo.t [i] = gameStates.app.nSDLTicks [0];
		return m_hitInfo.v [m_hitInfo.i];
		}
#endif
m_hitInfo.v [m_hitInfo.i] = vHit;
m_hitInfo.t [m_hitInfo.i] = gameStates.app.nSDLTicks [0];
m_hitInfo.i = (m_hitInfo.i + 1) % 3;
return m_hitInfo.v [m_hitInfo.i];
}

//------------------------------------------------------------------------------

bool CObject::ResetDamage (void)
{
if ((info.nType != OBJ_PLAYER) && (info.nType != OBJ_ROBOT) && (info.nType != OBJ_REACTOR))
	return false;

m_damage.bCritical = false;
m_damage.tCritical = 0;
m_damage.nCritical = 0;

	bool bReset = false;

for (int i = 0; i < 3; i++)
	if (ResetDamage (i))
		bReset = true;
return bReset;
}

//------------------------------------------------------------------------------
// Compute the absolute damage inflicted by a critical hit.
// fShieldScale is a measure of the relative shield strength. A full player ship shield has a ratio of 1.0
// It is used to scale critical hit damage with a target's durability to avoid strong targets being disabled
// by critical hits too soon. This is necessary since strong targets take a lot of hits to be taken down,
// so will receive a lot of critical hits compared to weak targets.
// Player shield are given a ratio of 2 to reduce critical hit effects on them.

float CObject::DamageRate (void)
{
if ((info.nType != OBJ_PLAYER) && (info.nType != OBJ_ROBOT) && (info.nType != OBJ_REACTOR))
	return 0.0f;
float	fShieldScale = (info.nType == OBJ_PLAYER) ? 2.0f : X2F (RobotDefaultShield (this)) / 100.0f;
if (fShieldScale < 1.0f) {
	if (fShieldScale <= 0.0f)
		return 0.0f;
	fShieldScale = 1.0f;
	}
return 1.0f - 0.25f / fShieldScale;
}

//------------------------------------------------------------------------------

bool CObject::RepairDamage (int i)
{
if (!m_damage.nHits [i])
	return false;
m_damage.nHits [i]--;
m_damage.tRepaired = gameStates.app.nSDLTicks [0];
#if DBG
static const char* szSubSystem [3] = {"AIM", "DRIVES", "GUNS"};
HUDMessage (0, "%s repaired (%d)", szSubSystem [i], m_damage.nHits [i]);
#endif
return true;
}

//------------------------------------------------------------------------------

void CObject::RepairDamage (void)
{
if ((info.nType != OBJ_PLAYER) && (info.nType != OBJ_ROBOT) && (info.nType != OBJ_REACTOR))
	return;
if (gameStates.app.nSDLTicks [0] - m_damage.tRepaired < REPAIR_DELAY)
	return;
for (int i = 0; i < 3; i++)
	RepairDamage (i);
}

//------------------------------------------------------------------------------

fix CObject::SubSystemDamage (int i)
{
	fix	nHits;

return (!gameStates.app.bNostalgia && EGI_FLAG (nDamageModel, 0, 0, 0) && (nHits = m_damage.nHits [i])) 
		 ? (fix) FRound ((I2X (1) / 2) * pow (DamageRate (), nHits)) 
		 : I2X (1) / 2;
}

//------------------------------------------------------------------------------
//eof
