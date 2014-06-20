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
#include "multi.h"

//------------------------------------------------------------------------------

void CBossInfo::InitGateInterval (void)
{
m_nGateInterval = I2X (4) - gameStates.app.nDifficultyLevel * I2X (2) / 3;
}

#define	QUEUE_SIZE	256

// --------------------------------------------------------------------------------------------------------------------
//	Create list of segments boss is allowed to teleport to at segListP.
//	Set *segCountP.
//	Boss is allowed to teleport to segments he fits in (calls ObjectIntersectsWall) and
//	he can reach from his initial position (calls PathLength).
//	If bSizeCheck is set, then only add CSegment if boss can fit in it, else any segment is legal.
//	bOneWallHack added by MK, 10/13/95: A mega-hack! Set to !0 to ignore the
bool CBossInfo::SetupSegments (CShortArray& segments, int bSizeCheck, int bOneWallHack)
{
	CSegment		*segP;
	CObject		*bossObjP = OBJECTS + m_nObject;
	CFixVector	vBossHomePos;
	int			nMaxSegments, nSegments = 0;
	int			nBossHomeSeg;
	int			head, tail, w, childSeg;
	int			nGroup, nSide;
	CIntArray	queue;
	fix			xBossSizeSave;

	static short bossSegs [MAX_BOSS_TELEPORT_SEGS];

//	See if there is a boss.  If not, quick out.
xBossSizeSave = bossObjP->info.xSize;
// -- Causes problems!!	-- bossObjP->SetSize (FixMul (I2X (3) / 4, bossObjP->info.xSize));
nBossHomeSeg = bossObjP->info.nSegment;
vBossHomePos = bossObjP->info.position.vPos;
nGroup = SEGMENTS [nBossHomeSeg].m_group;
if (!queue.Create (gameData.segs.nSegments))
	return false;
head = 1;
tail = 0;
queue [0] = nBossHomeSeg;
bossSegs [nSegments++] = nBossHomeSeg;
nMaxSegments = Min (MAX_BOSS_TELEPORT_SEGS, LEVEL_SEGMENTS);
gameData.render.mine.visibility [0].BumpVisitedFlag ();

while (tail != head) {
	segP = SEGMENTS + queue [tail++];
	for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
		childSeg = segP->m_children [nSide];
		if (!bOneWallHack) {
			w = segP->IsPassable (nSide, NULL);
			if (!(w & WID_PASSABLE_FLAG))
				continue;
			}
		//	If we get here and w == WID_SOLID_WALL, then we want to process through this CWall, else not.
		if (!IS_CHILD (childSeg))
			continue;
		if (bOneWallHack)
			bOneWallHack--;
		if (gameData.render.mine.Visited (childSeg))
			continue;
		if (nGroup != SEGMENTS [childSeg].m_group)
			continue;
		gameData.render.mine.Visit (childSeg);
		if (bSizeCheck && !BossFitsInSeg (bossObjP, childSeg))
			continue;
		queue [head++] = childSeg;
		if (nSegments >= nMaxSegments - 1)
			goto done;
#if DBG
		if (bSizeCheck && (childSeg == nDbgSeg))
			BRP;
#endif
		bossSegs [nSegments++] = childSeg;
		}
	}

done:

segments.Destroy ();
if (!(nSegments && segments.Create (nSegments)))
	return false;
memcpy (segments.Buffer (), bossSegs, nSegments * sizeof (segments [0]));
bossObjP->SetSize (xBossSizeSave);
bossObjP->info.position.vPos = vBossHomePos;
OBJECTS [m_nObject].RelinkToSeg (nBossHomeSeg);
return true;
}

// -----------------------------------------------------------------------------

void CBossInfo::Init (void)
{
m_nObject = -1;
m_nCloakStartTime = 0;
m_nCloakEndTime = 0;
m_nLastTeleportTime = 0;
m_nTeleportInterval = I2X (8);
m_nCloakInterval = I2X (10);
m_nCloakDuration = BOSS_CLOAK_DURATION;
m_nLastGateTime = 0;
m_nGateInterval = I2X (6);
m_bDyingSoundPlaying = 0;
m_nDying = 0;
}

// -----------------------------------------------------------------------------

void CBossInfo::Destroy (void)
{
m_gateSegs.Destroy ();
m_teleportSegs.Destroy ();
Init ();
}

// -----------------------------------------------------------------------------

bool CBossInfo::Setup (short nObject)
{
if (nObject >= 0)
	m_nObject = nObject;
else if (m_nObject < 0)
	return false;
if (!(SetupSegments (m_gateSegs, 0, 0) && SetupSegments (m_teleportSegs, 1, 0))) {
	Destroy ();
	return false;
	}
m_nGateSegs = m_gateSegs.Length ();
m_nTeleportSegs = m_teleportSegs.Length ();
if (gameStates.app.bD1Mission)
	m_nGateInterval = I2X (5) - I2X (gameStates.app.nDifficultyLevel) / 2;
else
	m_nGateInterval = I2X (4) - gameStates.app.nDifficultyLevel * I2X (2) / 3;
if (missionManager.nCurrentLevel == missionManager.nLastLevel) {
	m_nTeleportInterval = I2X (10);
	m_nCloakInterval = I2X (15);					//	Time between cloaks
	}
else {
	m_nTeleportInterval = I2X (7);
	m_nCloakInterval = I2X (10);					//	Time between cloaks
	}
return true;
}

// -----------------------------------------------------------------------------

void CBossInfo::SaveState (CFile& cf)
{
cf.WriteShort (m_nObject);
cf.WriteFix (m_nCloakStartTime);
cf.WriteFix (m_nCloakEndTime );
cf.WriteFix (m_nLastTeleportTime );
cf.WriteFix (m_nTeleportInterval);
cf.WriteFix (m_nCloakInterval);
cf.WriteFix (m_nCloakDuration);
cf.WriteFix (m_nLastGateTime);
cf.WriteFix (m_nGateInterval);
cf.WriteFix (m_nDyingStartTime);
cf.WriteInt (m_nDying);
cf.WriteInt (m_bDyingSoundPlaying);
cf.WriteFix (m_nHitTime);
}

// -----------------------------------------------------------------------------

void CBossInfo::SaveSizeStates (CFile& cf)
{
cf.WriteShort (m_teleportSegs.Length ());
cf.WriteShort (m_gateSegs.Length ());
}

// -----------------------------------------------------------------------------

void CBossInfo::SaveBufferState (CFile& cf, CShortArray& buffer)
{
	int	i, j;

if (buffer.Buffer () && (j = buffer.Length ()))
	for (i = 0; i < j; i++)
		cf.WriteShort (buffer [i]);
}

// -----------------------------------------------------------------------------

void CBossInfo::SaveBufferStates (CFile& cf)
{
SaveBufferState (cf, m_gateSegs);
SaveBufferState (cf, m_teleportSegs);
}

// -----------------------------------------------------------------------------

void CBossInfo::LoadBinState (CFile& cf)
{
cf.Read (&m_nCloakStartTime, sizeof (fix), 1);
cf.Read (&m_nCloakEndTime , sizeof (fix), 1);
cf.Read (&m_nLastTeleportTime , sizeof (fix), 1);
cf.Read (&m_nTeleportInterval, sizeof (fix), 1);
cf.Read (&m_nCloakInterval, sizeof (fix), 1);
cf.Read (&m_nCloakDuration, sizeof (fix), 1);
cf.Read (&m_nLastGateTime, sizeof (fix), 1);
cf.Read (&m_nGateInterval, sizeof (fix), 1);
cf.Read (&m_nDyingStartTime, sizeof (fix), 1);
int h;
cf.Read (&h, sizeof (int), 1);
m_nDying = short (h);
if ((m_nDying < 0) || (m_nDying >= gameData.objs.nObjects) || !ROBOTINFO (OBJECTS [m_nDying].info.nId).bossFlag)
	m_nDying = 0;
else if (m_nDying && (m_nDyingStartTime > gameData.time.xGame))
	m_nDyingStartTime = gameData.time.xGame;
cf.Read (&h, sizeof (int), 1);
m_bDyingSoundPlaying = sbyte (h);
cf.Read (&m_nHitTime, sizeof (fix), 1);
}

// -----------------------------------------------------------------------------

void CBossInfo::LoadState (CFile& cf, int nVersion)
{
if (nVersion > 31)
	m_nObject = cf.ReadShort ();
m_nCloakStartTime = cf.ReadFix ();
m_nCloakEndTime = cf.ReadFix ();
m_nLastTeleportTime = cf.ReadFix ();
m_nTeleportInterval = cf.ReadFix ();
m_nCloakInterval = cf.ReadFix ();
m_nCloakDuration = cf.ReadFix ();
m_nLastGateTime = cf.ReadFix ();
m_nGateInterval = cf.ReadFix ();
m_nDyingStartTime = cf.ReadFix ();
m_nDying = cf.ReadInt ();
if (m_nDying && (m_nDyingStartTime > gameData.time.xGame))
	m_nDyingStartTime = gameData.time.xGame;
m_bDyingSoundPlaying = cf.ReadInt ();
m_nHitTime = cf.ReadFix ();
}

// -----------------------------------------------------------------------------

void CBossInfo::LoadSizeStates (CFile& cf)
{
m_nTeleportSegs = cf.ReadShort ();
m_nGateSegs = cf.ReadShort ();
}

// -----------------------------------------------------------------------------

int CBossInfo::LoadBufferState (CFile& cf, CShortArray& buffer, int nBufSize)
{
if (!nBufSize || (uint (nBufSize) > uint (LEVEL_SEGMENTS)))
	return 1;
if (!buffer.Create (nBufSize))
	return 0;
for (int i = 0; i < nBufSize; i++)
	buffer [i] = cf.ReadShort ();
return 1;
}

// -----------------------------------------------------------------------------

int CBossInfo::LoadBufferStates (CFile& cf)
{
return LoadBufferState (cf, m_gateSegs, m_nGateSegs) && LoadBufferState (cf, m_teleportSegs, m_nTeleportSegs);
}

// -----------------------------------------------------------------------------

void CBossInfo::ResetCloakTime (void) 
{ 
m_nCloakStartTime = m_nCloakEndTime = gameData.time.xGame; 
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

CBossData::CBossData ()
{
}

// ----------------------------------------------------------------------------

bool CBossData::Create (uint nBosses)
{
if (!m_info.Create (nBosses ? nBosses : 10))
	return false;
m_info.SetGrowth (10);
m_info.Clear ();
return true;
}

// ----------------------------------------------------------------------------

void CBossData::Destroy (void)
{
m_info.Destroy ();
}

short CBossData::Find (short nBossObj)
{
for (short nBoss = 0; nBoss < short (m_info.ToS ()); nBoss++)
	if (m_info [nBoss].m_nObject == nBossObj)
		return nBoss;
return -1;
}

// -----------------------------------------------------------------------------

void CBossData::Setup (int nObject)
{
for (uint i = 0; i < m_info.ToS (); i++)
	m_info [i].Setup (nObject);
}

// -----------------------------------------------------------------------------

int CBossData::Add (short nObject)
{
	short	nBoss = Find (nObject);

if (nBoss >= 0)
	return nBoss;
if (!(m_info.Buffer () || Create ()))
	return -1;
if (!m_info.Grow ())
	return -1;
m_info.Top ()->Init ();
m_info.Top ()->Setup (nObject);
if (ROBOTINFO (OBJECTS [nObject].info.nId).bEndsLevel)
	extraGameInfo [0].nBossCount [0]++;
return nBoss;
}

// -----------------------------------------------------------------------------

void CBossData::Remove (short nBoss)
{
if ((nBoss < 0) || (nBoss >= short (m_info.ToS ())))
	return;
	
short nObject = m_info [nBoss].m_nObject;

if (m_info.Delete (nBoss)) {
	if (ROBOTINFO (OBJECTS [nObject].info.nId).bEndsLevel)
		extraGameInfo [0].nBossCount [0]--;
	memset (&m_info [m_info.ToS ()], 0, sizeof (CBossInfo));
	}
}

//------------------------------------------------------------------------------

void CBossData::InitGateIntervals (void)
{
for (uint i = 0; i < m_info.ToS (); i++)
	m_info [i].InitGateInterval ();
}

// -----------------------------------------------------------------------------

void CBossData::ResetHitTimes (void)
{
for (uint i = 0; i < m_info.ToS (); i++)
	m_info [i].ResetHitTime ();
}

// -----------------------------------------------------------------------------

void CBossData::ResetCloakTimes (void)
{
for (uint i = 0; i < m_info.ToS (); i++)
	gameData.bosses [i].ResetCloakTime ();
}

// -----------------------------------------------------------------------------

void CBossData::SaveStates (CFile& cf) {
	for (uint i = 0; i < m_info.ToS (); i++)
		m_info [i].SaveState (cf);
	}

// -----------------------------------------------------------------------------

void CBossData::SaveSizeStates (CFile& cf) 
{
for (uint i = 0; i < m_info.ToS (); i++)
	m_info [i].SaveSizeStates (cf);
}

// -----------------------------------------------------------------------------

void CBossData::SaveBufferStates (CFile& cf) 
{
for (uint i = 0; i < m_info.ToS (); i++)
	m_info [i].SaveBufferStates (cf);
}

// -----------------------------------------------------------------------------

void CBossData::LoadBinStates (CFile& cf) 
{
for (uint i = 0; i < m_info.ToS (); i++)
	m_info [i].LoadBinState (cf);
}

// -----------------------------------------------------------------------------

void CBossData::LoadStates (CFile& cf, int nVersion) 
{
for (uint i = 0; i < m_info.ToS (); i++)
	m_info [i].LoadState (cf, nVersion);
}

// -----------------------------------------------------------------------------

void CBossData::LoadSizeStates (CFile& cf) 
{
for (uint i = 0; i < m_info.ToS (); i++)
	m_info [i].LoadSizeStates (cf);
}

// -----------------------------------------------------------------------------

int CBossData::LoadBufferStates (CFile& cf) 
{
for (uint i = 0; i < m_info.ToS (); i++)
	if (!m_info [i].LoadBufferStates (cf))
		return 0;
return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//eof
