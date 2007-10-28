#ifndef _SDLGL_H
#define _SDLGL_H

void InitGammaRamp (void);
int OglSetBrightnessInternal (void);
int OglVideoModeOK (int w, int h);
void OglInitAttributes (void);
int OglInitWindow (int w, int h, int bForce);
void OglDestroyWindow (void);
void OglDoFullScreenInternal (int bForce);
void OglSwapBuffersInternal (void);
void OglClose (void);

#endif //_SDLGL_H
