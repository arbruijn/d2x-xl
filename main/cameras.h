#ifndef _cameras_h
#define _cameras_h

#include "ogl_init.h"

#define MAX_CAMERAS	100

typedef struct tCamera {
	tObject		obj;
	tObject		*objP;
	short			segNum;
	short			sideNum;
	grsBitmap	texBuf;
	char			*screenBuf;
	GLuint		glTexId;
	time_t		nTimeout;
	char			nType;
	char			bValid;
	char			bVisible;
	char			bTimedOut;
	char			bHaveUVL;
	char			bShadowMap;
	char			bTeleport;
	int			nWaitFrames;
	tUVL			uvlList [8];
#if RENDER2TEXTURE == 1
	ogl_pbuffer	pb;
	ogl_texture	glTex;
#elif RENDER2TEXTURE == 2
	ogl_fbuffer	fb;
	ogl_texture	glTex;
#endif
	vmsMatrix	orient;
	fixang		curAngle;
	fixang		curDelta;
	time_t		t0;
} tCamera;

int CreateCameras (void);
void DestroyCameras (void);
int RenderCameras (void);
int RenderCamera (tCamera *pc);
void GetCameraUVL (tCamera *pc, tUVL *uvlCopy);
#if RENDER2TEXTURE
int OglCamBufAvail (tCamera *pc, int bCheckTexture);
#endif
int CreateCamera (tCamera *pc, short srcSeg, short srcSide, short tgtSeg, short tgtSide, 
						tObject *objP, int bShadowMap, int bTeleport);
void DestroyCamera (tCamera *pc);

#endif // _cameras_h
