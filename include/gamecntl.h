#ifndef __GAMECNTL_H
#define __GAMECNTL_H

void TransferEnergyToShield(fix time);
void PauseGame (void);
void ResumeGame (void);
int DoGamePause (void);
int SelectNextWindowFunction (int nWindow);
int GatherWindowFunctions (int* nWinFuncs);

#endif //__GAMECNTL_H
