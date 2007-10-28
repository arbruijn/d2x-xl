#ifndef _OGL_SHADER_H
#define _OGL_SHADER_H

#ifdef _MSC_VER
#include <windows.h>
#include <stddef.h>
#endif

extern GLhandleARB	genShaderProg;

char *LoadShader (char* fileName);
int CreateShaderProg (GLhandleARB *progP);
int CreateShaderFunc (GLhandleARB *progP, GLhandleARB *fsP, GLhandleARB *vsP, 
							 char *fsName, char *vsName, int bFromFile);
int LinkShaderProg (GLhandleARB *progP);
void DeleteShaderProg (GLhandleARB *progP);
void InitShaders (void);
void OglInitShaders (void);

#endif
