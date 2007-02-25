#ifndef __CHEATS_H
#define __CHEATS_H

void KillAllRobots (int bVerbose);
void KillEverything (int bVerbose);
void KillThief (int bVerbose);
#ifndef RELEASE
void KillAllSnipers (int bVerbose);
#endif
void KillBuddy (int bVerbose);
void FinalCheats (int key);
SuperWowieCheat (int bVerbose);

#ifndef RELEASE
void DoCheatMenu ();
#endif

#endif //__CHEATS_H
