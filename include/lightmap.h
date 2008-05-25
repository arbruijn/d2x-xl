#ifndef __lightmap_h
#define __lightmap_h

#include "ogl_defs.h"
#include "ogl_texture.h"

//------------------------------------------------------------------------------

#define MAX_LIGHTMAP_WIDTH	128
#define LIGHTMAP_WIDTH		lightMapWidth [gameOpts->render.nLightmapQuality]
#define LIGHTMAP_BUFWIDTH	512
#define LIGHTMAP_ROWSIZE	(LIGHTMAP_BUFWIDTH / LIGHTMAP_WIDTH)
#define LIGHTMAP_BUFSIZE	(LIGHTMAP_ROWSIZE * LIGHTMAP_ROWSIZE)

//------------------------------------------------------------------------------

typedef struct tLightMapInfo {
	vmsVector	vPos;
	vmsVector	vDir;  //currently based on face normals
	GLfloat		color [3];
	//float		bright;
	double		range;
	int			nIndex;  //(seg*6)+tSide ie which tSide the light is on
} tLightMapInfo;

typedef struct tLightMap {
	tRgbColorf	*bmP;
} tLightMap;

typedef struct tLightMapBuffer {
	GLuint		handle;
	tRgbColorb	bmP [LIGHTMAP_BUFWIDTH][LIGHTMAP_BUFWIDTH];
} tLightMapBuffer;

typedef struct tLightMapData {
	tLightMapInfo		*info;
	tLightMapBuffer	*buffers;
	int					nBuffers;
	int					nLights; 
	ushort				nLightmaps;
} tLightMapData;

//------------------------------------------------------------------------------

void InitLightmapShaders (void);
void RestoreLights (int bVariable);
void CreateLightMaps (int nLevel);
void DestroyLightMaps (void);
int OglCreateLightMap (int nLightMap);
int OglCreateLightMaps (void);
void OglDestroyLightMaps (void);

#define	USE_LIGHTMAPS \
			(gameStates.render.color.bLightMapsOk && \
			 gameOpts->render.color.bUseLightMaps && \
			 !IsMultiGame && \
			 (gameOpts->render.nLightingMethod == 0))

//------------------------------------------------------------------------------

//extern tOglTexture	*lightMaps;
extern tLightMapData		lightMapData;
extern int					lightMapWidth [5];
extern GLhandleARB		lmShaderProgs [3];

//------------------------------------------------------------------------------

static inline int HaveLightMaps (void)
{
return (lightMapData.info != NULL);
}

//------------------------------------------------------------------------------

#endif //__lightmap_h
