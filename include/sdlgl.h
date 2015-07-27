#ifndef _SDLGL_H
#define _SDLGL_H

void InitGammaRamp (void);
int SdlGlSetBrightnessInternal (void);
int SdlGlVideoModeOK (int w, int h);
void SdlGlInitAttributes (void);
int SdlGlInitWindow (int w, int h, int bForce);
void SdlGlDestroyWindow (void);
void SdlGlDoFullScreenInternal (int bForce);
void SdlGlSwapBuffersInternal (void);
void SdlGlClose (void);

#endif //_SDLGL_H
