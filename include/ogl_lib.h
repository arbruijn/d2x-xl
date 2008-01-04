//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_LIB_H
#define _OGL_LIB_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "inferno.h"
#include "vecmat.h"

//------------------------------------------------------------------------------

extern GLuint hBigSphere;
extern GLuint hSmallSphere;
extern GLuint circleh5;
extern GLuint circleh10;
extern GLuint mouseIndList;
extern GLuint cross_lh [2];
extern GLuint primary_lh [3];
extern GLuint secondary_lh [5];
extern GLuint g3InitTMU [4][2];
extern GLuint g3ExitTMU [2];

extern int r_polyc, r_tpolyc, r_bitmapc, r_ubitmapc, r_ubitbltc, r_upixelc, r_tvertexc, r_texcount;
extern int gr_renderstats, gr_badtexture;

//------------------------------------------------------------------------------

typedef struct tRenderQuality {
	int	texMinFilter;
	int	texMagFilter;
	int	bNeedMipmap;
	int	bAntiAliasing;
} tRenderQuality;

extern tRenderQuality renderQualities [];

typedef struct tSinCosf {
	float	fSin, fCos;
} tSinCosf;

//------------------------------------------------------------------------------

void SetRenderQuality (void);
void OglDeleteLists (GLuint *lp, int n);
void OglComputeSinCos (int nSides, tSinCosf *sinCosP);
int CircleListInit (int nSides, int nType, int mode); 
void G3Normal (g3sPoint **pointList, vmsVector *pvNormal);
void G3CalcNormal (g3sPoint **pointList, fVector *pvNormal);
static inline fVector *G3GetNormal (g3sPoint *pPoint, fVector *pvNormal);
fVector *G3Reflect (fVector *vReflect, fVector *vLight, fVector *vNormal);
int G3EnableClientState (GLuint nState, int nTMU);
int G3DisableClientState (GLuint nState, int nTMU);
void G3DisableClientStates (int bTexCoord, int bColor, int bNormals, int nTMU);
int G3EnableClientStates (int bTexCoord, int bColor, int bNormals, int nTMU);
void OglSetFOV (double fov);
void OglStartFrame (int bFlat, int bResetColorBuf);
void OglEndFrame (void);
void OglEnableLighting (int bSpecular);
void OglDisableLighting (void);
void OglSwapBuffers (int bForce, int bClear);
void OglSetupTransform (int bForce);
void OglResetTransform (int bForce);
void OglBlendFunc (GLenum nSrcBlend, GLenum nDestBlend);
void RebuildRenderContext (int bGame);
void OglSetScreenMode (void);
void OglGetVerInfo (void);
GLuint OglCreateDepthTexture (int nTMU, int bFBO);
GLuint OglCreateStencilTexture (int nTMU, int bFBO);
void OglCreateDrawBuffer (void);
void OglDestroyDrawBuffer (void);
void OglDrawBuffer (int nBuffer, int bFBO);
void OglReadBuffer (int nBuffer, int bFBO);
void OglFlushDrawBuffer (void);

//------------------------------------------------------------------------------

static inline int OglHaveDrawBuffer (void)
{
return gameStates.ogl.bRender2TextureOk && gameData.render.ogl.drawBuffer.hFBO && gameStates.ogl.bDrawBufferActive;
}

//------------------------------------------------------------------------------

static inline fVector *G3GetNormal (g3sPoint *pPoint, fVector *pvNormal)
{
return pPoint->p3_normal.nFaces ? &pPoint->p3_normal.vNormal : pvNormal;
}

//------------------------------------------------------------------------------

#ifdef _DEBUG
void OglGenTextures (GLsizei n, GLuint *hTextures);
void OglDeleteTextures (GLsizei n, GLuint *hTextures);
#else
#	define OglGenTextures glGenTextures
#	define OglDeleteTextures glDeleteTextures
#endif

//------------------------------------------------------------------------------

#endif
