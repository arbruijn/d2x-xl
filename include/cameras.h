#ifndef _cameras_h
#define _cameras_h

#include "ogl_defs.h"
#include "ogl_texture.h"

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
	char			bMirror;
	int			nWaitFrames;
	tUVL			uvlList [4];
	tTexCoord2f	texCoord [6];
#if RENDER2TEXTURE == 1
	tPixelBuffer	pb;
	tOglTexture	glTex;
#elif RENDER2TEXTURE == 2
	tFrameBuffer	fb;
	tOglTexture	glTex;
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
void GetCameraUVL (tCamera *pc, grsFace *faceP, tUVL *uvlP, tTexCoord2f *texCoordP, fVector3 *vertexP);
#if RENDER2TEXTURE
int OglCamBufAvail (tCamera *pc, int bCheckTexture);
#endif
int CreateCamera (tCamera *pc, short srcSeg, short srcSide, short tgtSeg, short tgtSide, 
						tObject *objP, int bShadowMap, int bTeleport);
void DestroyCamera (tCamera *pc);

#define USE_CAMERAS (extraGameInfo [0].bUseCameras && (!IsMultiGame || (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bUseCameras)))

#endif // _cameras_h
