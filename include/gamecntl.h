#ifndef __GAMECNTL_H
#define __GAMECNTL_H

void TransferEnergyToShield(fix time);
void PauseGame (void);
void ResumeGame (void);
int32_t DoGamePause (void);
int32_t SelectNextWindowFunction (int32_t nWindow);

#endif //__GAMECNTL_H
