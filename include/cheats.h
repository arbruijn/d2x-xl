#ifndef __CHEATS_H
#define __CHEATS_H

void KillAllRobots (int bVerbose);
void KillEverything (int bVerbose);
void KillThief (int bVerbose);
#ifdef _DEBUG
void KillAllSnipers (int bVerbose);
#endif
void KillBuddy (int bVerbose);
void FinalCheats (int key);
void SuperWowieCheat (int bVerbose);
void WowieCheat (int bVerbose);
void GasolineCheat (int bVerbose);
void DoWowieCheat (int bVerbose, int bInitialize);

#ifdef _DEBUG
void DoCheatMenu ();
#endif

#endif //__CHEATS_H
