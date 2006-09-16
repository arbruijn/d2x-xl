#ifndef _cameras_h
#define _cameras_h

#ifdef OGL
#	include "ogl_init.h"
#endif

#define MAX_CAMERAS	100

typedef struct tCamera {
	object		obj;
	object		*objP;
	short			segNum;
	short			sideNum;
	grs_bitmap	texBuf;
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
	uvl			uvlList [8];
#if RENDER2TEXTURE == 1
	ogl_pbuffer	pb;
	ogl_texture	glTex;
#elif RENDER2TEXTURE == 2
	ogl_fbuffer	fb;
	ogl_texture	glTex;
#endif
	vms_matrix	orient;
	fixang		curAngle;
	fixang		curDelta;
	time_t		t0;
} tCamera;

extern tCamera cameras [MAX_CAMERAS];
extern char nSideCameras [MAX_SEGMENTS][6];
extern char bRenderCameras;
extern int bFullScreenCameras;
extern int nCameraFPS;

int CreateCameras (void);
void DestroyCameras (void);
int RenderCameras (void);
int RenderCamera (tCamera *pc);
void GetCameraUVL (tCamera *pc, uvl *uvlCopy);
#if RENDER2TEXTURE
int OglCamBufAvail (tCamera *pc, int bCheckTexture);
#endif
int CreateCamera (tCamera *pc, short srcSeg, short srcSide, short tgtSeg, short tgtSide, 
						object *objP, int bShadowMap, int bTeleport);
void DestroyCamera (tCamera *pc);

#endif // _cameras_h
