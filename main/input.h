#ifndef __INPUT_H
#define __INPUT_H

extern control_info Controls;

int ControlsReadJoystick (int *joy_axis);
void ControlsReadFCS (int raw_axis);
int ControlsReadAll (void);
void FlushInput (void);
void ResetCruise (void);
char GetKeyValue (char key);
void SetMaxPitch (int nMinTurnRate);
void SetControlType (void);

#endif //__INPUT_H