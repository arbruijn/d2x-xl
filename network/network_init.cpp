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

#ifndef _WIN32
#	include <arpa/inet.h>
#	include <netinet/in.h> /* for htons & co. */
#endif

#include "descent.h"
#include "ipx.h"
#include "error.h"
#include "network.h"
#include "network_lib.h"
#include "netmisc.h"
#include "hogfile.h"

#define SECURITY_CHECK	1

//-----------------------------------------------------------------------------

void ResetPingStats (void)
{
memset (pingStats, 0, sizeof (pingStats));
networkData.tLastPingStat = 0;
}

//------------------------------------------------------------------------------

void InitNetworkData (void)
{
memset (&networkData, 0, sizeof (networkData));
networkData.nActiveGames = 0;
networkData.nActiveGames = 0;
networkData.nLastActiveGames = 0;
networkData.nPacketsPerSec = 10;
networkData.nNamesInfoSecurity = -1;
networkData.nMaxXDataSize = NET_XDATA_SIZE;
networkData.nNetLifeKills = 0;
networkData.nNetLifeKilled = 0;
networkData.bDebug = 0;
networkData.bActive = 0;
networkData.nStatus = 0;
networkData.bGamesChanged = 0;
networkData.nSocket = 0;
networkData.bAllowSocketChanges = 1;
networkData.nSecurityFlag = NETSECURITY_OFF;
networkData.nSecurityNum = 0;
NetworkResetSyncStates ();
networkData.nJoinState = 0;					// Did WE rejoin this game?
networkData.bNewGame = 0;					// Is this the first level of a new game?
networkData.bPlayerAdded = 0;				// Is this a new CPlayerData or a returning CPlayerData?
networkData.bPacketUrgent = 0;
networkData.nGameType = 0;
networkData.nTotalMissedPackets = 0;
networkData.nTotalPacketsGot = 0;
networkData.bSyncPackInited = 1;		// Set to 1 if the networkData.syncPack is zeroed.
networkData.nSegmentCheckSum = 0;
networkData.bWantPlayersInfo = 0;
networkData.bWaitingForPlayerInfo = 0;
}

//------------------------------------------------------------------------------

void NetworkInit (void)
{
	int nPlayerSave = gameData.multiplayer.nLocalPlayer;

GameDisableCheats ();
gameStates.multi.bIWasKicked = 0;
gameStates.gameplay.bFinalBossIsDead = 0;
networkData.nNamesInfoSecurity = -1;
#ifdef NETPROFILING
OpenSendLog ();
OpenReceiveLog (); 
#endif
InitAddressFilter ();
InitPacketHandlers ();
gameData.multiplayer.maxPowerupsAllowed.Clear (0);
gameData.multiplayer.powerupsInMine.Clear (0);
networkData.nTotalMissedPackets = 0; 
networkData.nTotalPacketsGot = 0;
memset (&netGame, 0, sizeof (tNetgameInfo));
memset (&netPlayers, 0, sizeof (tAllNetPlayersInfo));
networkData.thisPlayer.nType = PID_REQUEST;
memcpy (networkData.thisPlayer.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
networkData.thisPlayer.player.versionMajor=D2X_MAJOR;
networkData.thisPlayer.player.versionMinor=D2X_MINOR | (IS_D2_OEM ? NETWORK_OEM : 0);
networkData.thisPlayer.player.rank=GetMyNetRanking ();
if (gameStates.multi.nGameType >= IPX_GAME) {
	memcpy (networkData.thisPlayer.player.network.ipx.node, IpxGetMyLocalAddress (), 6);
	if (gameStates.multi.nGameType == UDP_GAME)
		*reinterpret_cast<ushort*> (networkData.thisPlayer.player.network.ipx.node + 4) = 
			htons (*reinterpret_cast<ushort*> (networkData.thisPlayer.player.network.ipx.node + 4));
//		if (gameStates.multi.nGameType == UDP_GAME)
//			memcpy (networkData.thisPlayer.player.network.ipx.node, ipx_LocalAddress + 4, 4);
	memcpy (networkData.thisPlayer.player.network.ipx.server, IpxGetMyServerAddress (), 4);
}
networkData.thisPlayer.player.computerType = DOS;
gameData.multiplayer.nLocalPlayer = nPlayerSave;         
MultiNewGame ();
networkData.bNewGame = 1;
gameData.reactor.bDestroyed = 0;
NetworkFlush ();
netGame.nPacketsPerSec = mpParams.nPPS;
}

//------------------------------------------------------------------------------

int NetworkCreateMonitorVector (void)
{
	#define MAX_BLOWN_BITMAPS 100
	
	int      blownBitmaps [MAX_BLOWN_BITMAPS];
	int      nBlownBitmaps = 0;
	int      nMonitor = 0;
	int      vector = 0;
	CSegment *segP = SEGMENTS.Buffer ();
	CSide    *sideP;
	int      h, i, j, k;
	int      tm, ec;

for (i = 0; i < gameData.eff.nEffects [gameStates.app.bD1Data]; i++) {
	if ((h = gameData.eff.effectP [i].nDestBm) > 0) {
		for (j = 0; j < nBlownBitmaps; j++)
			if (blownBitmaps [j] == h)
				break;
		if (j == nBlownBitmaps) {
			blownBitmaps [nBlownBitmaps++] = h;
			Assert (nBlownBitmaps < MAX_BLOWN_BITMAPS);
			}
		}
	}               
	
for (i = 0; i <= gameData.segs.nLastSegment; i++, segP++) {
	for (j = 0, sideP = segP->m_sides; j < 6; j++, sideP++) {
		if ((tm = sideP->m_nOvlTex) != 0) {
			if (((ec = gameData.pig.tex.tMapInfoP [tm].nEffectClip) != -1) &&
					(gameData.eff.effectP[ec].nDestBm != -1)) {
				nMonitor++;
				Assert (nMonitor < 32);
				}
			else {
				for (k = 0; k < nBlownBitmaps; k++) {
					if ((tm) == blownBitmaps [k]) {
						vector |= (1 << nMonitor);
						nMonitor++;
						Assert (nMonitor < 32);
						break;
						}
					}
				}
			}
		}
	}
return (vector);
}

//------------------------------------------------------------------------------

void InitMonsterballSettings (tMonsterballInfo *monsterballP)
{
	tMonsterballForce *forceP = monsterballP->forces;

// primary weapons
forceP [0].nWeaponId = LASER_ID; 
forceP [0].nForce = 10;
forceP [1].nWeaponId = LASER_ID + 1; 
forceP [1].nForce = 15;
forceP [2].nWeaponId = LASER_ID + 2; 
forceP [2].nForce = 20;
forceP [3].nWeaponId = LASER_ID + 3; 
forceP [3].nForce = 25;
forceP [4].nWeaponId = SPREADFIRE_ID; 
forceP [4].nForce = 20;
forceP [5].nWeaponId = VULCAN_ID; 
forceP [5].nForce = 10;
forceP [6].nWeaponId = PLASMA_ID; 
forceP [6].nForce = 30;
forceP [7].nWeaponId = FUSION_ID; 
forceP [7].nForce = 100;
// primary "super" weapons
forceP [8].nWeaponId = SUPERLASER_ID; 
forceP [8].nForce = 50;
forceP [9].nWeaponId = SUPERLASER_ID + 1; 
forceP [9].nForce = 60;
forceP [10].nWeaponId = HELIX_ID; 
forceP [10].nForce = 40;
forceP [11].nWeaponId = GAUSS_ID; 
forceP [11].nForce = 30;
forceP [12].nWeaponId = PHOENIX_ID; 
forceP [12].nForce = 60;
forceP [13].nWeaponId = OMEGA_ID; 
forceP [13].nForce = 30;
forceP [14].nWeaponId = FLARE_ID; 
forceP [14].nForce = 5;
// missiles
forceP [15].nWeaponId = CONCUSSION_ID; 
forceP [15].nForce = 50;
forceP [16].nWeaponId = HOMINGMSL_ID; 
forceP [16].nForce = 50;
forceP [17].nWeaponId = SMARTMSL_ID; 
forceP [17].nForce = 80;
forceP [18].nWeaponId = MEGAMSL_ID; 
forceP [18].nForce = 150;
// "super" missiles
forceP [19].nWeaponId = FLASHMSL_ID; 
forceP [19].nForce = 30;
forceP [20].nWeaponId = GUIDEDMSL_ID; 
forceP [20].nForce = 40;
forceP [21].nWeaponId = MERCURYMSL_ID; 
forceP [21].nForce = 70;
forceP [22].nWeaponId = EARTHSHAKER_ID; 
forceP [22].nForce = 200;
forceP [23].nWeaponId = EARTHSHAKER_MEGA_ID; 
forceP [23].nForce = 150;
// CPlayerData ships
forceP [24].nWeaponId = 255;
forceP [24].nForce = 4;
monsterballP->nBonus = 1;
monsterballP->nSizeMod = 7;	// that is actually shield orb size * 3, because it's divided by 2, thus allowing for half sizes
}

//------------------------------------------------------------------------------

void InitEntropySettings (int i)
{
extraGameInfo [i].entropy.nEnergyFillRate = 25;
extraGameInfo [i].entropy.nShieldFillRate = 11;
extraGameInfo [i].entropy.nShieldDamageRate = 11;
extraGameInfo [i].entropy.nMaxVirusCapacity = 0; 
extraGameInfo [i].entropy.nBumpVirusCapacity = 2;
extraGameInfo [i].entropy.nBashVirusCapacity = 1; 
extraGameInfo [i].entropy.nVirusGenTime = 2; 
extraGameInfo [i].entropy.nVirusLifespan = 0; 
extraGameInfo [i].entropy.nVirusStability = 0;
extraGameInfo [i].entropy.nCaptureVirusLimit = 1; 
extraGameInfo [i].entropy.nCaptureTimeLimit = 1; 
extraGameInfo [i].entropy.bRevertRooms = 0;
extraGameInfo [i].entropy.bDoConquerWarning = 0;
extraGameInfo [i].entropy.nOverrideTextures = 2;
extraGameInfo [i].entropy.bBrightenRooms = 0;
extraGameInfo [i].entropy.bPlayerHandicap = 0;
}

//------------------------------------------------------------------------------

void InitExtraGameInfo (void)
{
	int	i;

for (i = 0; i < 2; i++) {
	extraGameInfo [i].nType = 0;
	extraGameInfo [i].bFriendlyFire = 1;
	extraGameInfo [i].bInhibitSuicide = 0;
	extraGameInfo [i].bFixedRespawns = 0;
	extraGameInfo [i].nSpawnDelay = 0;
	extraGameInfo [i].bEnhancedCTF = 0;
	extraGameInfo [i].bRadarEnabled = i ? 0 : 1;
	extraGameInfo [i].bPowerupsOnRadar = 0;
	extraGameInfo [i].nZoomMode = 0;
	extraGameInfo [i].bRobotsHitRobots = 0;
	extraGameInfo [i].bAutoDownload = 1;
	extraGameInfo [i].bAutoBalanceTeams = 0;
	extraGameInfo [i].bDualMissileLaunch = 0;
	extraGameInfo [i].bRobotsOnRadar = 0;
	extraGameInfo [i].grWallTransparency = (FADE_LEVELS * 10 + 3) / 6;
	extraGameInfo [i].nSpeedBoost = 10;
	extraGameInfo [i].bDropAllMissiles = 1;
	extraGameInfo [i].bImmortalPowerups = 0;
	extraGameInfo [i].bUseCameras = 1;
	extraGameInfo [i].nFusionRamp = 2;
	extraGameInfo [i].nOmegaRamp = 2;
	extraGameInfo [i].bWiggle = 1;
	extraGameInfo [i].bMultiBosses = 0;
	extraGameInfo [i].nBossCount = 0;
	extraGameInfo [i].bSmartWeaponSwitch = 0;
	extraGameInfo [i].bFluidPhysics = 0;
	extraGameInfo [i].nWeaponDropMode = 1;
	extraGameInfo [i].bRotateLevels = 0;
	extraGameInfo [i].bDisableReactor = 0;
	extraGameInfo [i].bMouseLook = i ? 0 : 1;
	extraGameInfo [i].nWeaponIcons = 0;
	extraGameInfo [i].bSafeUDP = 0;
	extraGameInfo [i].bFastPitch = i ? 0 : 1;
	extraGameInfo [i].bUseParticles = 1;
	extraGameInfo [i].bUseLightning = 1;
	extraGameInfo [i].bDamageExplosions = 1;
	extraGameInfo [i].bThrusterFlames = 1;
	extraGameInfo [i].bShadows = 1;
	extraGameInfo [i].bPlayerShield = 1;
	extraGameInfo [i].bTeleporterCams = 0;
	extraGameInfo [i].bDarkness = 0;
	extraGameInfo [i].bTeamDoors = 0;
	extraGameInfo [i].bEnableCheats = 0;
	extraGameInfo [i].bTargetIndicators = 0;
	extraGameInfo [i].bDamageIndicators = 0;
	extraGameInfo [i].bFriendlyIndicators = 0;
	extraGameInfo [i].bCloakedIndicators = 0;
	extraGameInfo [i].bMslLockIndicators = 0;
	extraGameInfo [i].bTagOnlyHitObjs = 0;
	extraGameInfo [i].bTowFlags = 1;
	extraGameInfo [i].bUseHitAngles = 0;
	extraGameInfo [i].bLightTrails = 0;
	extraGameInfo [i].bTracers = 0;
	extraGameInfo [i].bShockwaves = 0;
	extraGameInfo [i].bCompetition = 0;
	extraGameInfo [i].bPowerupLights = i;
	extraGameInfo [i].bBrightObjects = i;
	extraGameInfo [i].bCheckUDPPort = 1;
	extraGameInfo [i].bSmokeGrenades = 0;
	extraGameInfo [i].nMslTurnSpeed = 2;
	extraGameInfo [i].nMslStartSpeed = 0;
	extraGameInfo [i].nMaxSmokeGrenades = 2;
	extraGameInfo [i].nCoopPenalty = 0;
	extraGameInfo [i].bKillMissiles = 0;
	extraGameInfo [i].bTripleFusion = 0;
	extraGameInfo [i].bShowWeapons = 0;
	extraGameInfo [i].bEnhancedShakers = 0;
	extraGameInfo [i].nHitboxes = 0;
	extraGameInfo [i].nRadar = 0;
	extraGameInfo [i].nDrag = 10;
	extraGameInfo [i].nSpotSize = 2 - i;
	extraGameInfo [i].nSpotStrength = 2 - i;
	extraGameInfo [i].nLightRange = 0;
	extraGameInfo [i].headlight.bAvailable = 1;
	extraGameInfo [i].headlight.bBuiltIn = 0;
	extraGameInfo [i].headlight.bDrainPower = 1;
	InitEntropySettings (i);
	InitMonsterballSettings (&extraGameInfo [i].monsterball);
	}
}

//------------------------------------------------------------------------------

int InitAutoNetGame (void)
{
if (!gameData.multiplayer.autoNG.bValid)
	return 0;
if (gameData.multiplayer.autoNG.bHost) {
	hogFileManager.UseMission (gameData.multiplayer.autoNG.szFile);
	strcpy (szAutoMission, gameData.multiplayer.autoNG.szMission);
	gameStates.app.bAutoRunMission = hogFileManager.AltFiles ().bInitialized;
	strncpy (mpParams.szGameName, gameData.multiplayer.autoNG.szName, sizeof (mpParams.szGameName));
	mpParams.nLevel = gameData.multiplayer.autoNG.nLevel;
	extraGameInfo [0].bEnhancedCTF = 0;
	switch (gameData.multiplayer.autoNG.uType) {
		case 0:
			mpParams.nGameMode = gameData.multiplayer.autoNG.bTeam ? NETGAME_TEAM_ANARCHY : NETGAME_ANARCHY;
			break;
		case 1:
			mpParams.nGameMode = NETGAME_COOPERATIVE;
			break;
		case 2:
			mpParams.nGameMode = NETGAME_CAPTURE_FLAG;
			break;
		case 3:
			mpParams.nGameMode = NETGAME_CAPTURE_FLAG;
			extraGameInfo [0].bEnhancedCTF = 1;
			break;
		case 4:
			if (HoardEquipped ())
				mpParams.nGameMode = gameData.multiplayer.autoNG.bTeam ? NETGAME_TEAM_HOARD : NETGAME_HOARD;
			else
				mpParams.nGameMode = gameData.multiplayer.autoNG.bTeam ? NETGAME_TEAM_ANARCHY : NETGAME_ANARCHY;
			break;
		case 5:
			mpParams.nGameMode = NETGAME_ENTROPY;
			break;
		case 6:
			mpParams.nGameMode = NETGAME_MONSTERBALL;
			break;
			}
	mpParams.nGameType = mpParams.nGameMode;
	}
else {
	memcpy (ipx_ServerAddress + 4, gameData.multiplayer.autoNG.ipAddr, sizeof (gameData.multiplayer.autoNG.ipAddr));
	mpParams.udpPorts [1] = gameData.multiplayer.autoNG.nPort;
	}
return 1;
}

//------------------------------------------------------------------------------

void LogExtraGameInfo (void)
{
if (!gameStates.app.bHaveExtraGameInfo [1])
	PrintLog ("No extra game info data available\n");
else {
	PrintLog ("extra game info data:\n");
	PrintLog ("   bFriendlyFire: %d\n", extraGameInfo [1].bFriendlyFire);
	PrintLog ("   bInhibitSuicide: %d\n", extraGameInfo [1].bInhibitSuicide);
	PrintLog ("   bFixedRespawns: %d\n", extraGameInfo [1].bFixedRespawns);
	PrintLog ("   nSpawnDelay: %d\n", extraGameInfo [1].nSpawnDelay);
	PrintLog ("   bEnhancedCTF: %d\n", extraGameInfo [1].bEnhancedCTF);
	PrintLog ("   bRadarEnabled: %d\n", extraGameInfo [1].bRadarEnabled);
	PrintLog ("   bPowerupsOnRadar: %d\n", extraGameInfo [1].bPowerupsOnRadar);
	PrintLog ("   nZoomMode: %d\n", extraGameInfo [1].nZoomMode);
	PrintLog ("   bRobotsHitRobots: %d\n", extraGameInfo [1].bRobotsHitRobots);
	PrintLog ("   bAutoDownload: %d\n", extraGameInfo [1].bAutoDownload);
	PrintLog ("   bAutoBalanceTeams: %d\n", extraGameInfo [1].bAutoBalanceTeams);
	PrintLog ("   bDualMissileLaunch: %d\n", extraGameInfo [1].bDualMissileLaunch);
	PrintLog ("   bRobotsOnRadar: %d\n", extraGameInfo [1].bRobotsOnRadar);
	PrintLog ("   grWallTransparency: %d\n", extraGameInfo [1].grWallTransparency);
	PrintLog ("   nSpeedBoost: %d\n", extraGameInfo [1].nSpeedBoost);
	PrintLog ("   bDropAllMissiles: %d\n", extraGameInfo [1].bDropAllMissiles);
	PrintLog ("   bImmortalPowerups: %d\n", extraGameInfo [1].bImmortalPowerups);
	PrintLog ("   bUseCameras: %d\n", extraGameInfo [1].bUseCameras);
	PrintLog ("   nFusionRamp: %d\n", extraGameInfo [1].nFusionRamp);
	PrintLog ("   nOmegaRamp: %d\n", extraGameInfo [1].nOmegaRamp);
	PrintLog ("   bWiggle: %d\n", extraGameInfo [1].bWiggle);
	PrintLog ("   bMultiBosses: %d\n", extraGameInfo [1].bMultiBosses);
	PrintLog ("   nBossCount: %d\n", extraGameInfo [1].nBossCount);
	PrintLog ("   bSmartWeaponSwitch: %d\n", extraGameInfo [1].bSmartWeaponSwitch);
	PrintLog ("   bFluidPhysics: %d\n", extraGameInfo [1].bFluidPhysics);
	PrintLog ("   nWeaponDropMode: %d\n", extraGameInfo [1].nWeaponDropMode);
	PrintLog ("   bDarkness: %d\n", extraGameInfo [1].bDarkness);
	PrintLog ("   bPowerupLights: %d\n", extraGameInfo [1].bPowerupLights);
	PrintLog ("   bBrightObjects: %d\n", extraGameInfo [1].bBrightObjects);
	PrintLog ("   bTeamDoors: %d\n", extraGameInfo [1].bTeamDoors);
	PrintLog ("   bEnableCheats: %d\n", extraGameInfo [1].bEnableCheats);
	PrintLog ("   bRotateLevels: %d\n", extraGameInfo [1].bRotateLevels);
	PrintLog ("   bDisableReactor: %d\n", extraGameInfo [1].bDisableReactor);
	PrintLog ("   bMouseLook: %d\n", extraGameInfo [1].bMouseLook);
	PrintLog ("   nWeaponIcons: %d\n", extraGameInfo [1].nWeaponIcons);
	PrintLog ("   bSafeUDP: %d\n", extraGameInfo [1].bSafeUDP);
	PrintLog ("   bFastPitch: %d\n", extraGameInfo [1].bFastPitch);
	PrintLog ("   bUseParticles: %d\n", extraGameInfo [1].bUseParticles);
	PrintLog ("   bUseLightning: %d\n", extraGameInfo [1].bUseLightning);
	PrintLog ("   bDamageExplosions: %d\n", extraGameInfo [1].bDamageExplosions);
	PrintLog ("   bThrusterFlames: %d\n", extraGameInfo [1].bThrusterFlames);
	PrintLog ("   bShadows: %d\n", extraGameInfo [1].bShadows);
	PrintLog ("   bPlayerShield: %d\n", extraGameInfo [1].bPlayerShield);
	PrintLog ("   bTeleporterCams: %d\n", extraGameInfo [1].bTeleporterCams);
	PrintLog ("   bEnableCheats: %d\n", extraGameInfo [1].bEnableCheats);
	PrintLog ("   bTargetIndicators: %d\n", extraGameInfo [1].bTargetIndicators);
	PrintLog ("   bDamageIndicators: %d\n", extraGameInfo [1].bDamageIndicators);
	PrintLog ("   bFriendlyIndicators: %d\n", extraGameInfo [1].bFriendlyIndicators);
	PrintLog ("   bCloakedIndicators: %d\n", extraGameInfo [1].bCloakedIndicators);
	PrintLog ("   bMslLockIndicators: %d\n", extraGameInfo [1].bMslLockIndicators);
	PrintLog ("   bTagOnlyHitObjs: %d\n", extraGameInfo [1].bTagOnlyHitObjs);
	PrintLog ("   bTowFlags: %d\n", extraGameInfo [1].bTowFlags);
	PrintLog ("   bUseHitAngles: %d\n", extraGameInfo [1].bUseHitAngles);
	PrintLog ("   bLightTrails: %d\n", extraGameInfo [1].bLightTrails);
	PrintLog ("   bTracers: %d\n", extraGameInfo [1].bTracers);
	PrintLog ("   bShockwaves: %d\n", extraGameInfo [1].bShockwaves);
	PrintLog ("   bSmokeGrenades: %d\n", extraGameInfo [1].bSmokeGrenades);
	PrintLog ("   nMaxSmokeGrenades: %d\n", extraGameInfo [1].nMaxSmokeGrenades);
	PrintLog ("   bCompetition: %d\n", extraGameInfo [1].bCompetition);
	PrintLog ("   bFlickerLights: %d\n", extraGameInfo [1].bFlickerLights);
	PrintLog ("   bCheckUDPPort: %d\n", extraGameInfo [1].bCheckUDPPort);
	PrintLog ("   bSmokeGrenades: %d\n", extraGameInfo [1].bSmokeGrenades);
	PrintLog ("   nMslTurnSpeed: %d\n", extraGameInfo [1].nMslTurnSpeed);
	PrintLog ("   nMslStartSpeed: %d\n", extraGameInfo [1].nMslStartSpeed);
	PrintLog ("   nCoopPenalty: %d\n", (int) nCoopPenalties [(int) extraGameInfo [1].nCoopPenalty]);
	PrintLog ("   bKillMissiles: %d\n", extraGameInfo [1].bKillMissiles);
	PrintLog ("   bTripleFusion: %d\n", extraGameInfo [1].bTripleFusion);
	PrintLog ("   bShowWeapons: %d\n", extraGameInfo [1].bShowWeapons);
	PrintLog ("   bEnhancedShakers: %d\n", extraGameInfo [1].bEnhancedShakers);
	PrintLog ("   nHitboxes: %d\n", extraGameInfo [1].nHitboxes);
	PrintLog ("   nRadar: %d\n", extraGameInfo [1].nRadar);
	PrintLog ("   nDrag: %d\n", extraGameInfo [1].nDrag);
	PrintLog ("   nSpotSize: %d\n", extraGameInfo [1].nSpotSize);
	PrintLog ("   nSpotStrength: %d\n", extraGameInfo [1].nSpotStrength);
	PrintLog ("   nLightRange: %d\n", extraGameInfo [1].nLightRange);
	PrintLog ("entropy info data:\n");
	PrintLog ("   nEnergyFillRate: %d\n", extraGameInfo [1].entropy.nEnergyFillRate);
	PrintLog ("   nShieldFillRate: %d\n", extraGameInfo [1].entropy.nShieldFillRate);
	PrintLog ("   nShieldDamageRate: %d\n", extraGameInfo [1].entropy.nShieldDamageRate);
	PrintLog ("   nMaxVirusCapacity: %d\n", extraGameInfo [1].entropy.nMaxVirusCapacity); 
	PrintLog ("   nBumpVirusCapacity: %d\n", extraGameInfo [1].entropy.nBumpVirusCapacity);
	PrintLog ("   nBashVirusCapacity: %d\n", extraGameInfo [1].entropy.nBashVirusCapacity); 
	PrintLog ("   nVirusGenTime: %d\n", extraGameInfo [1].entropy.nVirusGenTime); 
	PrintLog ("   nVirusLifespan: %d\n", extraGameInfo [1].entropy.nVirusLifespan); 
	PrintLog ("   nVirusStability: %d\n", extraGameInfo [1].entropy.nVirusStability);
	PrintLog ("   nCaptureVirusLimit: %d\n", extraGameInfo [1].entropy.nCaptureVirusLimit); 
	PrintLog ("   nCaptureTimeLimit: %d\n", extraGameInfo [1].entropy.nCaptureTimeLimit); 
	PrintLog ("   bRevertRooms: %d\n", extraGameInfo [1].entropy.bRevertRooms);
	PrintLog ("   bDoConquerWarning: %d\n", extraGameInfo [1].entropy.bDoConquerWarning);
	PrintLog ("   nOverrideTextures: %d\n", extraGameInfo [1].entropy.nOverrideTextures);
	PrintLog ("   bBrightenRooms: %d\n", extraGameInfo [1].entropy.bBrightenRooms);
	PrintLog ("   bPlayerHandicap: %d\n", extraGameInfo [1].entropy.bPlayerHandicap);
	PrintLog ("   headlight.bAvailable: %d\n", extraGameInfo [1].headlight.bAvailable);
	PrintLog ("   headlight.bBuiltIn: %d\n", extraGameInfo [1].headlight.bBuiltIn);
	PrintLog ("   headlight.bDrainPower: %d\n", extraGameInfo [1].headlight.bDrainPower);
	}
}

//------------------------------------------------------------------------------

