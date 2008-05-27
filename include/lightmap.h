#ifndef __lightmap_h
#define __lightmap_h

#include "ogl_defs.h"
#include "ogl_texture.h"

//------------------------------------------------------------------------------

#define MAX_LIGHTMAP_WIDTH	128
#define LIGHTMAP_WIDTH		lightmapWidth [gameOpts->render.nLightmapQuality]
#define LIGHTMAP_BUFWIDTH	512
#define LIGHTMAP_ROWSIZE	(LIGHTMAP_BUFWIDTH / LIGHTMAP_WIDTH)
#define LIGHTMAP_BUFSIZE	(LIGHTMAP_ROWSIZE * LIGHTMAP_ROWSIZE)

//------------------------------------------------------------------------------

typedef struct tLightmapInfo {
	vmsVector	vPos;
	vmsVector	vDir;  //currently based on face normals
	GLfloat		color [3];
	//float		bright;
	double		range;
	int			nIndex;  //(seg*6)+tSide ie which tSide the light is on
} tLightmapInfo;

typedef struct tLightmap {
	tRgbColorf	*bmP;
} tLightmap;

typedef struct tLightmapBuffer {
	GLuint		handle;
	tRgbColorb	bmP [LIGHTMAP_BUFWIDTH][LIGHTMAP_BUFWIDTH];
} tLightmapBuffer;

typedef struct tLightmapData {
	tLightmapInfo		*info;
	tLightmapBuffer	*buffers;
	int					nBuffers;
	int					nLights; 
	ushort				nLightmaps;
} tLightmapData;

//------------------------------------------------------------------------------

void InitLightmapShaders (void);
void RestoreLights (int bVariable);
void CreateLightmaps (int nLevel);
void DestroyLightmaps (void);
int OglCreateLightmap (int nLightmap);
int OglCreateLightmaps (void);
void OglDestroyLightmaps (void);

#define	USE_LIGHTMAPS \
			(gameStates.render.color.bLightmapsOk && \
			 gameOpts->render.color.bUseLightmaps && \
			 !IsMultiGame && \
			 (gameOpts->render.nLightingMethod == 0))

//------------------------------------------------------------------------------

//extern tOglTexture	*lightmaps;
extern tLightmapData		lightmapData;
extern int					lightmapWidth [5];
extern GLhandleARB		lmShaderProgs [3];

//------------------------------------------------------------------------------

static inline int HaveLightmaps (void)
{
return (lightmapData.info != NULL);
}

//------------------------------------------------------------------------------

#endif //__lightmap_h
