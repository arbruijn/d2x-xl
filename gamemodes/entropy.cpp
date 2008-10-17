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

#include "inferno.h"
#include "error.h"
#include "network.h"
#include "entropy.h"

//	------------------------------------------------------------------------------------------------------

int FindFuelCen (int nSegment)
{
	int	i;

for (i = 0; i < gameData.matCens.nFuelCenters; i++)
	if (gameData.matCens.fuelCenters [i].nSegment == nSegment)
		return i;
return -1;
}

//-----------------------------------------------------------------------------

int CountRooms (void)
{
	int		i;
	xsegment	*segP = gameData.segs.xSegments;

memset (gameData.entropy.nRoomOwners, 0xFF, sizeof (gameData.entropy.nRoomOwners));
memset (gameData.entropy.nTeamRooms, 0, sizeof (gameData.entropy.nTeamRooms));
for (i = 0; i <= gameData.segs.nLastSegment; i++, segP++)
	if ((segP->owner >= 0) && (segP->group >= 0) && 
		 /* (segP->group <= N_MAX_ROOMS) &&*/ (gameData.entropy.nRoomOwners [(int) segP->group] < 0))
		gameData.entropy.nRoomOwners [(int) segP->group] = segP->owner;
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
	int			f, h, i, j, jj, k, kk, nObject;
	tSegment		*segP;
	xsegment		*xsegP;
	tObject		*objP;
	tFuelCenInfo	*fuelP;
	short			virusGens [MAX_FUEL_CENTERS];

// this loop with
// a) convert all segments with group 'roomId' to newOwner 'newOwner'
// b) count all virus centers newOwner 'newOwner' was owning already, 
//    j holding the total number of virus centers, jj holding the number of 
//    virus centers converted from fuel/repair centers
// c) count all virus centers newOwner 'newOwner' has just conquered, 
//    k holding the total number of conquered virus centers, kk holding the number of 
//    conquered virus centers converted from fuel/repair centers
// So if j > jj or k < kk, all virus centers placed in virusGens can be converted
// back to their original nType
gameData.entropy.nTeamRooms [oldOwner]--;
gameData.entropy.nTeamRooms [newOwner]++;
for (i = 0, j = jj = 0, k = kk = MAX_FUEL_CENTERS, segP = gameData.segs.segments, xsegP = gameData.segs.xSegments; 
	  i <= gameData.segs.nLastSegment; 
	  i++, segP++, xsegP++) {
	if ((xsegP->group == roomId) && (xsegP->owner == oldOwner)) {
		xsegP->owner = newOwner;
		ChangeSegmentTexture (i, oldOwner);
		if (gameData.segs.segment2s [i].special == SEGMENT_IS_ROBOTMAKER) {
			--k;
			if (extraGameInfo [1].entropy.bRevertRooms && (-1 < (f = FindFuelCen (i))) &&
				 (gameData.matCens.origStationTypes [f] != SEGMENT_IS_NOTHING))
				virusGens [--kk] = f;
			for (nObject = gameData.segs.objects [SEG_IDX (segP)]; nObject >= 0; nObject = objP->info.nNextInSeg) {
				objP = OBJECTS + nObject;
				if ((objP->info.nType == OBJ_POWERUP) && (objP->info.nType == POW_ENTROPY_VIRUS))
					objP->info.nCreator = newOwner;
				}
			}
		}
	else {
		if ((xsegP->owner == newOwner) && (gameData.segs.segment2s [i].special == SEGMENT_IS_ROBOTMAKER)) {
			j++;
			if (extraGameInfo [1].entropy.bRevertRooms && (-1 < (f = FindFuelCen (i))) &&
				 (gameData.matCens.origStationTypes [f] != SEGMENT_IS_NOTHING))
				virusGens [jj++] = f;
			}
		}
	}
// if the new owner has conquered a virus generator, turn all generators that had
// been turned into virus generators because the new owner had lost his last virus generators
// back to their original nType
if (extraGameInfo [1].entropy.bRevertRooms && (jj + (MAX_FUEL_CENTERS - kk)) && ((j > jj) || (k < kk))) {
	if (kk < MAX_FUEL_CENTERS) {
		memcpy (virusGens + jj, virusGens + kk, (MAX_FUEL_CENTERS - kk) * sizeof (short));
		jj += (MAX_FUEL_CENTERS - kk);
		for (j = 0; j < jj; j++) {
			fuelP = gameData.matCens.fuelCenters + virusGens [j];
			h = fuelP->nSegment;
			gameData.segs.segment2s [h].special = gameData.matCens.origStationTypes [fuelP - gameData.matCens.fuelCenters];
			FuelCenCreate (gameData.segs.segments + h, gameData.segs.segment2s [h].special);
			ChangeSegmentTexture (h, newOwner);
			}
		}
	}

// check if the other newOwner's last virus center has been conquered
// if so, find a fuel or repair center owned by that and turn it into a virus generator
// preferrably convert repair centers
for (i = 0, h = -1, xsegP = gameData.segs.xSegments; i <= gameData.segs.nLastSegment; i++, xsegP++)
	if (xsegP->owner == oldOwner) 
		switch (gameData.segs.segment2s [i].special) {
			case SEGMENT_IS_ROBOTMAKER:
				return;
			case SEGMENT_IS_FUELCEN:
				if (h < 0)
					h = i;
				break;
			case SEGMENT_IS_REPAIRCEN:
				if ((h < 0) || (gameData.segs.segment2s [h].special == SEGMENT_IS_FUELCEN))
					h = i;
			}
if (h < 0)
	return;
i = gameData.segs.segment2s [h].special;
gameData.segs.segment2s [h].special = SEGMENT_IS_ROBOTMAKER;
BotGenCreate (gameData.segs.segments + h, i);
ChangeSegmentTexture (h, newOwner);
}

//	------------------------------------------------------------------------------------------------------

void StartConquerWarning (void)
{
if (extraGameInfo [1].entropy.bDoConquerWarning) {
	gameStates.sound.nConquerWarningSoundChannel = DigiPlaySample (SOUND_CONTROL_CENTER_WARNING_SIREN, F3_0);
	MultiSendConquerWarning ();
	gameStates.entropy.bConquerWarning = 1;
	}
}

//	------------------------------------------------------------------------------------------------------

void StopConquerWarning (void)
{
if (gameStates.entropy.bConquerWarning) {
	if (gameStates.sound.nConquerWarningSoundChannel >= 0)
		DigiStopSound (gameStates.sound.nConquerWarningSoundChannel);
	MultiSendStopConquerWarning ();
	gameStates.entropy.bConquerWarning = 0;
	}
}

//	------------------------------------------------------------------------------------------------------

int CheckConquerRoom (xsegment *segP)
{
	tPlayer	*playerP = gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer;
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
if (playerP->shields < 0) {
	HUDMessage (0, "you are dead");
	StopConquerWarning ();
	return 0;
	}
if (playerP->secondaryAmmo [PROXMINE_INDEX] < extraGameInfo [1].nCaptureVirusLimit) {
	HUDMessage (0, "too few viruses");
	StopConquerWarning ();
	return 0;
	}
if (segP->owner < 0) {
	HUDMessage (0, "neutral room");
	StopConquerWarning ();
	return 0;
	}
if (segP->owner == team) {
	HUDMessage (0, "own room");
	StopConquerWarning ();
	return 0;
	}
#else
if ((gameStates.entropy.nTimeLastMoved < 0) || 
	 (playerP->shields < 0) || 
	 (playerP->secondaryAmmo [PROXMINE_INDEX] < extraGameInfo [1].entropy.nCaptureVirusLimit) ||
	 (segP->owner < 0) || 
	 (segP->owner == team)) {
	StopConquerWarning ();
	return 0;
	}
#endif
t = SDL_GetTicks ();
if (!gameStates.entropy.nTimeLastMoved)
	gameStates.entropy.nTimeLastMoved = (int) t;
if (t - gameStates.entropy.nTimeLastMoved < extraGameInfo [1].entropy.nCaptureTimeLimit * 1000) {
	gameStates.entropy.bConquering = 1;
	if (segP->owner > 0)
		StartConquerWarning ();
	return 0;
	}
StopConquerWarning ();
if (segP->owner)
	MultiSendCaptureBonus ((char) gameData.multiplayer.nLocalPlayer);
playerP->secondaryAmmo [PROXMINE_INDEX] -= extraGameInfo [1].entropy.nCaptureVirusLimit;
if (playerP->secondaryAmmo [SMARTMINE_INDEX] > extraGameInfo [1].entropy.nBashVirusCapacity)
	playerP->secondaryAmmo [SMARTMINE_INDEX] -= extraGameInfo [1].entropy.nBashVirusCapacity;
else
	playerP->secondaryAmmo [SMARTMINE_INDEX] = 0;
MultiSendConquerRoom ((char) team, (char) segP->owner, (char) segP->group);
ConquerRoom ((char) team, (char) segP->owner, (char) segP->group);
return 1;
}

//------------------------------------------------------------------------------
//eof
