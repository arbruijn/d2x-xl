#ifndef __CHEATS_H
#define __CHEATS_H

void KillAllRobots (int32_t bVerbose);
void KillEverything (int32_t bVerbose);
void KillThief (int32_t bVerbose);
#if DBG
void KillAllSnipers (int32_t bVerbose);
#endif
void KillBuddy (int32_t bVerbose);
void FinalCheats (int32_t key);
void SuperWowieCheat (int32_t bVerbose);
void WowieCheat (int32_t bVerbose);
void GasolineCheat (int32_t bVerbose);
void DoWowieCheat (int32_t bVerbose, int32_t bInitialize);
void KillAllBossRobots (int32_t bVerbose);

#if DBG
void DoCheatMenu ();
#endif

#endif //__CHEATS_H
