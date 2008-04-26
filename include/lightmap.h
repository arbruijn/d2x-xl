#ifndef __lightmap_h
#define __lightmap_h

#include "ogl_defs.h"
#include "ogl_texture.h"

//------------------------------------------------------------------------------

#define LIGHTMAP_WIDTH	8

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
	GLuint		handle;
	tRgbaColorf	bmP [LIGHTMAP_WIDTH][LIGHTMAP_WIDTH];
} tLightMap;

//------------------------------------------------------------------------------

void InitLightmapShaders (void);
void RestoreLights (int bVariable);
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

//------------------------------------------------------------------------------

//extern tOglTexture	*lightMaps;
extern tLightMapInfo	*lightMapInfo;
extern tLightMap		*lightMaps;
extern tLightMap		dummyLightMap;
extern GLhandleARB	lmShaderProgs [3];

//------------------------------------------------------------------------------

static inline int HaveLightMaps (void)
{
return (lightMapInfo != NULL);
}

//------------------------------------------------------------------------------

#endif //__lightmap_h
