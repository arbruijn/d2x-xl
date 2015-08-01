#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "error.h"
#include "network.h"
#include "entropy.h"

//	------------------------------------------------------------------------------------------------------

int32_t FindProducer (int32_t nSegment)
{
for (int32_t i = 0; i < gameData.producerData.nProducers; i++)
	if (gameData.producerData.producers [i].nSegment == nSegment)
		return i;
return -1;
}

//-----------------------------------------------------------------------------

int32_t CountRooms (void)
{
	int32_t		i;
	CSegment	*pSeg = SEGMENTS.Buffer ();

memset (gameData.entropyData.nRoomOwners, 0xFF, sizeof (gameData.entropyData.nRoomOwners));
memset (gameData.entropyData.nTeamRooms, 0, sizeof (gameData.entropyData.nTeamRooms));
for (i = 0; i <= gameData.segData.nLastSegment; i++, pSeg++)
	if ((pSeg->m_owner >= 0) && (pSeg->m_group >= 0) && 
		 /* (pSeg->m_group <= N_MAX_ROOMS) &&*/ (gameData.entropyData.nRoomOwners [(int32_t) pSeg->m_group] < 0))
		gameData.entropyData.nRoomOwners [(int32_t) pSeg->m_group] = pSeg->m_owner;
for (i = 0; i < N_MAX_ROOMS; i++)
	if (gameData.entropyData.nRoomOwners [i] >= 0) {
		gameData.entropyData.nTeamRooms [(int32_t) gameData.entropyData.nRoomOwners [i]]++;
		gameData.entropyData.nTotalRooms++;
		}
return gameData.entropyData.nTotalRooms;
}

//-----------------------------------------------------------------------------

void ConquerRoom (int32_t newOwner, int32_t oldOwner, int32_t roomId)
{
	int32_t			f, h, i, j, jj, k, kk, nObject;
	CSegment*		pSeg;
	CObject*			pObj;
	tProducerInfo*	pFuel;
	int16_t			virusGens [MAX_FUEL_CENTERS];

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
gameData.entropyData.nTeamRooms [oldOwner]--;
gameData.entropyData.nTeamRooms [newOwner]++;
for (i = 0, j = jj = 0, k = kk = MAX_FUEL_CENTERS, pSeg = SEGMENTS.Buffer (); 
	  i < gameData.segData.nSegments; 
	  i++, pSeg++) {
	if ((pSeg->m_group == roomId) && (pSeg->m_owner == oldOwner)) {
		pSeg->m_owner = newOwner;
		pSeg->ChangeTexture (oldOwner);
		if (pSeg->m_function == SEGMENT_FUNC_VIRUSMAKER) {
			--k;
			if (extraGameInfo [1].entropy.bRevertRooms && (-1 < (f = FindProducer (i))) &&
				 (gameData.producerData.origProducerTypes [f] != SEGMENT_FUNC_NONE))
				virusGens [--kk] = f;
			for (nObject = pSeg->m_objects; nObject >= 0; nObject = pObj->info.nNextInSeg) {
				pObj = OBJECT (nObject);
				if ((pObj->info.nType == OBJ_POWERUP) && (pObj->info.nType == POW_ENTROPY_VIRUS))
					pObj->info.nCreator = newOwner;
				}
			}
		}
	else {
		if ((pSeg->m_owner == newOwner) && (pSeg->m_function == SEGMENT_FUNC_VIRUSMAKER)) {
			j++;
			if (extraGameInfo [1].entropy.bRevertRooms && (-1 < (f = FindProducer (i))) &&
				 (gameData.producerData.origProducerTypes [f] != SEGMENT_FUNC_NONE))
				virusGens [jj++] = f;
			}
		}
	}
// if the new owner has conquered a virus generator, turn all generators that had
// been turned into virus generators because the new owner had lost his last virus generator
// back to their original nType
if (extraGameInfo [1].entropy.bRevertRooms && (jj + (MAX_FUEL_CENTERS - kk)) && ((j > jj) || (k < kk))) {
	if (kk < MAX_FUEL_CENTERS) {
		memcpy (virusGens + jj, virusGens + kk, (MAX_FUEL_CENTERS - kk) * sizeof (int16_t));
		jj += (MAX_FUEL_CENTERS - kk);
		for (j = 0; j < jj; j++) {
			pFuel = gameData.producerData.producers + virusGens [j];
			CSegment* pFuelSeg = SEGMENT (pFuel->nSegment);
			pFuelSeg->m_function = gameData.producerData.origProducerTypes [uint32_t (pFuel - gameData.producerData.producers.Buffer ())];
			pFuelSeg->CreateProducer (pFuelSeg->m_function);
			pFuelSeg->ChangeTexture (newOwner);
			}
		}
	}

// check if the old owner's last virus center has been conquered
// if so, find a fuel or repair center owned by that and turn it into a virus generator
// preferrably convert repair centers
for (i = 0, h = -1, pSeg = SEGMENTS.Buffer (); i <= gameData.segData.nLastSegment; i++, pSeg++)
	if (pSeg->m_owner == oldOwner) 
		switch (SEGMENT (i)->m_function) {
			case SEGMENT_FUNC_VIRUSMAKER:
				return;
			case SEGMENT_FUNC_FUELCENTER:
				if (h < 0)
					h = i;
				break;
			case SEGMENT_FUNC_REPAIRCENTER:
				if ((h < 0) || (SEGMENT (h)->m_function == SEGMENT_FUNC_FUELCENTER))
					h = i;
			}
if (h < 0)
	return;
pSeg = SEGMENT (h);
i = pSeg->m_function;
pSeg->m_function = SEGMENT_FUNC_VIRUSMAKER;
pSeg->CreateRobotMaker (i);
pSeg->ChangeTexture (newOwner);
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

int32_t CSegment::ConquerCheck (void)
{
	CPlayerData	*pPlayer = gameData.multiplayer.players + N_LOCALPLAYER;
	int32_t		team = GetTeam (N_LOCALPLAYER) + 1;
	time_t	t;

gameStates.entropy.bConquering = 0;
if (!IsEntropyGame)
	return 0;
#if 0
if (gameStates.entropy.nTimeLastMoved < 0) {
	HUDMessage (0, "you are moving");
	StopConquerWarning ();
	return 0;
	}
if (pPlayer->Shield () < 0) {
	HUDMessage (0, "you are dead");
	StopConquerWarning ();
	return 0;
	}
if (pPlayer->secondaryAmmo [PROXMINE_INDEX] < extraGameInfo [1].nCaptureVirusThreshold) {
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
	 (pPlayer->Shield () < 0) || 
	 (pPlayer->secondaryAmmo [PROXMINE_INDEX] < extraGameInfo [1].entropy.nCaptureVirusThreshold) ||
	 (m_owner < 0) || 
	 (m_owner == team)) {
	StopConquerWarning ();
	return 0;
	}
#endif
t = SDL_GetTicks ();
if (!gameStates.entropy.nTimeLastMoved)
	gameStates.entropy.nTimeLastMoved = (int32_t) t;
if (t - gameStates.entropy.nTimeLastMoved < extraGameInfo [1].entropy.nCaptureTimeThreshold * 1000) {
	gameStates.entropy.bConquering = 1;
	if (m_owner > 0)
		StartConquerWarning ();
	return 0;
	}
StopConquerWarning ();
if (m_owner)
	MultiSendCaptureBonus (N_LOCALPLAYER);
pPlayer->secondaryAmmo [PROXMINE_INDEX] -= extraGameInfo [1].entropy.nCaptureVirusThreshold;
if (pPlayer->secondaryAmmo [SMARTMINE_INDEX] > extraGameInfo [1].entropy.nBashVirusCapacity)
	pPlayer->secondaryAmmo [SMARTMINE_INDEX] -= extraGameInfo [1].entropy.nBashVirusCapacity;
else
	pPlayer->secondaryAmmo [SMARTMINE_INDEX] = 0;
MultiSendConquerRoom (char (team), char (m_owner), char (m_group));
ConquerRoom (char (team), char (m_owner), char (m_group));
return 1;
}

//------------------------------------------------------------------------------
//eof
