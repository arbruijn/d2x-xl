#ifndef __lightmap_h
#define __lightmap_h

#include "ogl_defs.h"
#include "ogl_texture.h"

#define LIGHTMAP_WIDTH	8

typedef struct tLightMap {
	GLuint		handle;
	tRgbaColorf	bmP [LIGHTMAP_WIDTH][LIGHTMAP_WIDTH];
} tLightMap;

//extern tOglTexture	*lightMaps;
extern tLightMap		*lightMaps;
extern GLhandleARB	lmShaderProgs [3];

void InitLightmapShaders (void);
void CreateLightMaps (void);
void DestroyLightMaps (void);
int OglCreateLightMaps (void);
void OglDestroyLightMaps (void);
int HaveLightMaps (void);
double GetLightColor (int tMapNum, GLfloat *colorP);

#define	USE_LIGHTMAPS \
			(gameStates.render.color.bLightMapsOk && \
			 gameOpts->render.color.bUseLightMaps && \
			 !IsMultiGame && \
			 !gameOpts->render.bDynLighting)

#endif //__lightmap_h
