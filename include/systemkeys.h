#ifndef __SYSTEMKEYS_H
#define __SYSTEMKEYS_H

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
int ToggleChaseCam (void);
int SetFreeCam (int bOn);
int ToggleFreeCam (void);

#endif //__SYSTEMKEYS_H
