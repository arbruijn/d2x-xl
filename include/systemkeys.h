#ifndef __GAMECNTL_H
#define __GAMECNTL_H

void HandleEndlevelKey (int key);
void HandleDeathKey (int key);
void HandleDemoKey (int key);
int HandleSystemKey (int key);
void HandleVRKey (int key);
void HandleGameKey (int key);
void HandleTestKey (int key);

int SetRearView (int bOn);
int ToggleRearView (void);
void ResetRearView (void);
void CheckRearView (void);
int SetChaseCam (int bOn);
void ToggleChaseCam (void);
int SetFreeCam (int bOn);
int ToggleFreeCam (void);

#endif //__GAMECNTL_H
