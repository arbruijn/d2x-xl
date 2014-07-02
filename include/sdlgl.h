#ifndef _SDLGL_H
#define _SDLGL_H

void InitGammaRamp (void);
int32_t SdlGlSetBrightnessInternal (void);
int32_t SdlGlVideoModeOK (int32_t w, int32_t h);
void SdlGlInitAttributes (void);
int32_t SdlGlInitWindow (int32_t w, int32_t h, int32_t bForce);
void SdlGlDestroyWindow (void);
void SdlGlDoFullScreenInternal (int32_t bForce);
void SdlGlSwapBuffersInternal (void);
void SdlGlClose (void);

#endif //_SDLGL_H
