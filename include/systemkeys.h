#ifndef __SYSTEMKEYS_H
#define __SYSTEMKEYS_H

void HandleEndlevelKey (int32_t key);
void HandleDeathKey (int32_t key);
void HandleDemoKey (int32_t key);
int32_t HandleSystemKey (int32_t key);
void HandleGameKey (int32_t key);
void HandleTestKey (int32_t key);

int32_t SetRearView (int32_t bOn);
int32_t ToggleRearView (void);
void ResetRearView (void);
void CheckRearView (void);
int32_t SetChaseCam (int32_t bOn);
int32_t ToggleChaseCam (void);
int32_t SetFreeCam (int32_t bOn);
int32_t ToggleFreeCam (void);
void HandleZoom (void);

#endif //__SYSTEMKEYS_H
