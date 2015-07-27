#ifndef __CHEATS_H
#define __CHEATS_H

void KillAllRobots (int bVerbose);
void KillEverything (int bVerbose);
void KillThief (int bVerbose);
#if DBG
void KillAllSnipers (int bVerbose);
#endif
void KillBuddy (int bVerbose);
void FinalCheats (int key);
void SuperWowieCheat (int bVerbose);
void WowieCheat (int bVerbose);
void GasolineCheat (int bVerbose);
void DoWowieCheat (int bVerbose, int bInitialize);
void KillAllBossRobots (int bVerbose);

#if DBG
void DoCheatMenu ();
#endif

#endif //__CHEATS_H
