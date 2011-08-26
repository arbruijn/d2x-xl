#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "error.h"
#include "network.h"
#include "entropy.h"

//	------------------------------------------------------------------------------------------------------

int FindFuelCen (int nSegment)
{
for (int i = 0; i < gameData.matCens.nFuelCenters; i++)
	if (gameData.matCens.fuelCenters [i].nSegment == nSegment)
		return i;
return -1;
}

//-----------------------------------------------------------------------------

int CountRooms (void)
{
	int		i;
	CSegment	*segP = SEGMENTS.Buffer ();

memset (gameData.entropy.nRoomOwners, 0xFF, sizeof (gameData.entropy.nRoomOwners));
memset (gameData.entropy.nTeamRooms, 0, sizeof (gameData.entropy.nTeamRooms));
for (i = 0; i <= gameData.segs.nLastSegment; i++, segP++)
	if ((segP->m_owner >= 0) && (segP->m_group >= 0) && 
		 /* (segP->m_group <= N_MAX_ROOMS) &&*/ (gameData.entropy.nRoomOwners [(int) segP->m_group] < 0))
		gameData.entropy.nRoomOwners [(int) segP->m_group] = segP->m_owner;
for (i = 0; i < N_MAX_ROOMS; i++)
	if (gameData.entropy.nRoomOwners [i] >= 0) {
		gameData.entropy.nTeamRooms [(int) gameData.entropy.nRoomOwners [i]]++;
		gameData.entropy.nTotalRooms++;
		}
return gameData.entropy.nTotalRooms;
}

//-----------------------------------------------------------------------------

void ConquerRoom (int newOwner, int oldOwner, int roomId)
{
	int				f, h, i, j, jj, k, kk, nObject;
	CSegment*		segP;
	CObject*			objP;
	tFuelCenInfo*	fuelP;
	short				virusGens [MAX_FUEL_CENTERS];

// this loop with
// a) convert all segments with group 'roomId' to newOwner 'newOwner'
// b) count all virus centers newOwner 'newOwner' was owning already, 
//    j holding the total number of virus centers, jj holding the number of 
//    virus centers converted from fuel/repair centers
// c) count all virus centers newOwner 'newOwner' has just conquered, 
//    k holding the total number of conquered virus centers, kk holding the number of 
//    conquered virus centers converted from fuel/repair centers
// So if j > jj or k < kk, all virus centers placed in virusGens can be converted
// back to their original type
gameData.entropy.nTeamRooms [oldOwner]--;
gameData.entropy.nTeamRooms [newOwner]++;
for (i = 0, j = jj = 0, k = kk = MAX_FUEL_CENTERS, segP = SEGMENTS.Buffer (); 
	  i <= gameData.segs.nSegments; 
	  i++, segP++) {
	if ((segP->m_group == roomId) && (segP->m_owner == oldOwner)) {
		segP->m_owner = newOwner;
		segP->ChangeTexture (oldOwner);
		if (segP->m_function == SEGMENT_FUNC_VIRUSMAKER) {
			--k;
			if (extraGameInfo [1].entropy.bRevertRooms && (-1 < (f = FindFuelCen (i))) &&
				 (gameData.matCens.origStationTypes [f] != SEGMENT_FUNC_NONE))
				virusGens [--kk] = f;
			for (nObject = segP->m_objects; nObject >= 0; nObject = objP->info.nNextInSeg) {
				objP = OBJECTS + nObject;
				if ((objP->info.nType == OBJ_POWERUP) && (objP->info.nType == POW_ENTROPY_VIRUS))
					objP->info.nCreator = newOwner;
				}
			}
		}
	else {
		if ((segP->m_owner == newOwner) && (segP->m_function == SEGMENT_FUNC_VIRUSMAKER)) {
			j++;
			if (extraGameInfo [1].entropy.bRevertRooms && (-1 < (f = FindFuelCen (i))) &&
				 (gameData.matCens.origStationTypes [f] != SEGMENT_FUNC_NONE))
				virusGens [jj++] = f;
			}
		}
	}
// if the new owner has conquered a virus generator, turn all generators that had
// been turned into virus generators because the new owner had lost his last virus generator
// back to their original nType
if (extraGameInfo [1].entropy.bRevertRooms && (jj + (MAX_FUEL_CENTERS - kk)) && ((j > jj) || (k < kk))) {
	if (kk < MAX_FUEL_CENTERS) {
		memcpy (virusGens + jj, virusGens + kk, (MAX_FUEL_CENTERS - kk) * sizeof (short));
		jj += (MAX_FUEL_CENTERS - kk);
		for (j = 0; j < jj; j++) {
			fuelP = gameData.matCens.fuelCenters + virusGens [j];
			h = fuelP->nSegment;
			SEGMENTS [h].m_function = gameData.matCens.origStationTypes [uint (fuelP - gameData.matCens.fuelCenters.Buffer ())];
			SEGMENTS [h].CreateFuelCen (SEGMENTS [h].m_function);
			SEGMENTS [h].ChangeTexture (newOwner);
			}
		}
	}

// check if the old owner's last virus center has been conquered
// if so, find a fuel or repair center owned by that and turn it into a virus generator
// preferrably convert repair centers
for (i = 0, h = -1, segP = SEGMENTS.Buffer (); i <= gameData.segs.nLastSegment; i++, segP++)
	if (segP->m_owner == oldOwner) 
		switch (SEGMENTS [i].m_function) {
			case SEGMENT_FUNC_VIRUSMAKER:
				return;
			case SEGMENT_FUNC_FUELCEN:
				if (h < 0)
					h = i;
				break;
			case SEGMENT_FUNC_REPAIRCEN:
				if ((h < 0) || (SEGMENTS [h].m_function == SEGMENT_FUNC_FUELCEN))
					h = i;
			}
if (h < 0)
	return;
i = SEGMENTS [h].m_function;
SEGMENTS [h].m_function = SEGMENT_FUNC_VIRUSMAKER;
SEGMENTS [h].CreateBotGen (i);
SEGMENTS [h].ChangeTexture (newOwner);
}

//	------------------------------------------------------------------------------------------------------

void StartConquerWarning (void)
{
if (extraGameInfo [1].entropy.bDoCaptureWarning) {
	gameStates.sound.nConquerWarningSoundChannel = audio.PlaySound (SOUND_CONTROL_CENTER_WARNING_SIREN, SOUNDCLASS_GENERIC, I2X (3));
	MultiSendConquerWarning ();
	gameStates.entropy.bConquerWarning = 1;
	}
}

//	------------------------------------------------------------------------------------------------------

void StopConquerWarning (void)
{
if (gameStates.entropy.bConquerWarning) {
	if (gameStates.sound.nConquerWarningSoundChannel >= 0)
		audio.StopSound (gameStates.sound.nConquerWarningSoundChannel);
	MultiSendStopConquerWarning ();
	gameStates.entropy.bConquerWarning = 0;
	}
}

//	------------------------------------------------------------------------------------------------------

int CSegment::ConquerCheck (void)
{
	CPlayerData	*playerP = gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer;
	int		team = GetTeam (gameData.multiplayer.nLocalPlayer) + 1;
	time_t	t;

gameStates.entropy.bConquering = 0;
if (!(gameData.app.nGameMode & GM_ENTROPY))
	return 0;
#if 0
if (gameStates.entropy.nTimeLastMoved < 0) {
	HUDMessage (0, "you are moving");
	StopConquerWarning ();
	return 0;
	}
if (playerP->Shield () < 0) {
	HUDMessage (0, "you are dead");
	StopConquerWarning ();
	return 0;
	}
if (playerP->secondaryAmmo [PROXMINE_INDEX] < extraGameInfo [1].nCaptureVirusThreshold) {
	HUDMessage (0, "too few viruses");
	StopConquerWarning ();
	return 0;
	}
if (m_owner < 0) {
	HUDMessage (0, "neutral room");
	StopConquerWarning ();
	return 0;
	}
if (m_owner == team) {
	HUDMessage (0, "own room");
	StopConquerWarning ();
	return 0;
	}
#else
if ((gameStates.entropy.nTimeLastMoved < 0) || 
	 (playerP->Shield () < 0) || 
	 (playerP->secondaryAmmo [PROXMINE_INDEX] < extraGameInfo [1].entropy.nCaptureVirusThreshold) ||
	 (m_owner < 0) || 
	 (m_owner == team)) {
	StopConquerWarning ();
	return 0;
	}
#endif
t = SDL_GetTicks ();
if (!gameStates.entropy.nTimeLastMoved)
	gameStates.entropy.nTimeLastMoved = (int) t;
if (t - gameStates.entropy.nTimeLastMoved < extraGameInfo [1].entropy.nCaptureTimeLimit * 1000) {
	gameStates.entropy.bConquering = 1;
	if (m_owner > 0)
		StartConquerWarning ();
	return 0;
	}
StopConquerWarning ();
if (m_owner)
	MultiSendCaptureBonus ((char) gameData.multiplayer.nLocalPlayer);
playerP->secondaryAmmo [PROXMINE_INDEX] -= extraGameInfo [1].entropy.nCaptureVirusThreshold;
if (playerP->secondaryAmmo [SMARTMINE_INDEX] > extraGameInfo [1].entropy.nBashVirusCapacity)
	playerP->secondaryAmmo [SMARTMINE_INDEX] -= extraGameInfo [1].entropy.nBashVirusCapacity;
else
	playerP->secondaryAmmo [SMARTMINE_INDEX] = 0;
MultiSendConquerRoom (char (team), char (m_owner), char (m_group));
ConquerRoom (char (team), char (m_owner), char (m_group));
return 1;
}

//------------------------------------------------------------------------------
//eof
