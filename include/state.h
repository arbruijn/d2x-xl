/* $Id: 
,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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


#ifndef _STATE_H
#define _STATE_H

#define DESC_LENGTH	20
#define NUM_SAVES		9
#define THUMBNAIL_W	100
#define THUMBNAIL_H	50
#define THUMBNAIL_LW 200
#define THUMBNAIL_LH 120


class CSaveGameHandler {
	private:
		CFile			m_cf;
		int			m_bInGame;
		int			m_bBetweenLevels;
		int			m_bSecret;
		int			m_bQuick;
		int			m_nDefaultSlot;
		int			m_nLastSlot;
		int			m_nVersion;
		int			m_nGameId;
		char			m_filename [FILENAME_LEN];
		char			m_description [DESC_LENGTH + 1];
		const char*	m_override;

	public:
		CSaveGameHandler () {};
		~CSaveGameHandler () {};
		void Init (void);
		int Save (int bBetweenLevels, int bSecret, int bQuick, const char *pszFilenameOverride);
		int Load (int bInGame, int bSecret, int bQuick, const char *pszFilenameOverride);
		int SaveState (int bSecret, char *filename = NULL, char *description = NULL);
		int LoadState (int bMulti, int bSecret, char *filename = NULL);
		int GetSaveFile (int bMulti);
		int GetLoadFile (int bMulti);
		int GetGameId (char *filename);
		inline char* Filename (void) { return m_filename; }
		inline char* Description (void) { return m_description; }

	private:
		void Backup (void);
		void AutoSave (int nSaveSlot);
		void PushSecretSave (int nSaveSlot);
		void PopSecretSave (int nSaveSlot);
		void SaveImage (void);
		void SaveGameData (void);
		void SaveSpawnPoint (int i);
		void SaveReactorTrigger (tReactorTriggers *triggerP);
		void SaveReactorState (tReactorStates *stateP);
		void SaveFuelCen (tFuelCenInfo *fuelcenP);
		void SaveMatCen (tMatCenInfo *matcenP);
		void SaveObjTriggerRef (tObjTriggerRef *refP);
		void SaveTrigger (CTrigger *triggerP);
		void SaveActiveDoor (tActiveDoor *doorP);
		void SaveCloakingWall (tCloakingWall *wallP);
		void SaveExplWall (tExplWall *wallP);
		void SaveWall (CWall *wallP);
		void SaveObject (CObject *objP);
		void SavePlayer (CPlayerData *playerP);
		void SaveNetPlayers (void);
		void SaveNetGame (void);

		int ReadBoundedInt (int nMax, int *nVal);
		void LoadMulti (char *pszOrgCallSign, int bMulti);
		int LoadMission (void);
		int SetServerPlayer (CPlayerData *restoredPlayers, int nPlayers, const char *pszServerCallSign, int *pnOtherObjNum, int *pnServerObjNum);
		void GetConnectedPlayers (CPlayerData *restoredPlayers, int nPlayers);
		void FixNetworkObjects (int nServerPlayer, int nOtherObjNum, int nServerObjNum);
		void FixObjects (void);
		void AwardReturningPlayer (CPlayerData *retPlayerP, fix xOldGameTime);;
		void LoadNetGame (void);
		void LoadNetPlayers (void);
		void LoadPlayer (CPlayerData *playerP);
		void LoadObject (CObject *objP);
		void LoadWall (CWall *wallP);
		void LoadExplWall (tExplWall *wallP);
		void LoadCloakingWall (tCloakingWall *wallP);
		void LoadActiveDoor (tActiveDoor *doorP);
		void LoadTrigger (CTrigger *triggerP);
		void LoadObjTriggerRef (tObjTriggerRef *refP);
		void LoadMatCen (tMatCenInfo *matcenP);
		void LoadFuelCen (tFuelCenInfo *fuelcenP);
		void LoadReactorTrigger (tReactorTriggers *triggerP);
		void LoadReactorState (tReactorStates *stateP);
		int LoadSpawnPoint (int i);
		int LoadUniFormat (int bMulti, fix xOldGameTime, int *nLevel);
		int LoadBinFormat (int bMulti, fix xOldGameTime, int *nLevel);

		void SaveAILocalInfo (tAILocalInfo *ailP);
		void SaveAIPointSeg (tPointSeg *psegP);
		void SaveAICloakInfo (tAICloakInfo *ciP);
		int SaveAI (void);

		int LoadAIBinFormat (void);
		void LoadAILocalInfo (tAILocalInfo *ailP);
		void LoadAIPointSeg (tPointSeg *psegP);
		void LoadAICloakInfo (tAICloakInfo *ciP);
		int LoadAIUniFormat (void);

		inline int ImageSize (void) { return (m_nVersion < 26) ? THUMBNAIL_W * THUMBNAIL_H : THUMBNAIL_LW * THUMBNAIL_LH; }
};

extern CSaveGameHandler saveGameHandler;

#endif /* _STATE_H */
