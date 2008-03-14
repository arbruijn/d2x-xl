#ifndef __lightmap_h
#define __lightmap_h

#include "ogl_defs.h"
#include "ogl_texture.h"

typedef struct tLightMap {
	vmsVector	pos;
	GLfloat		color[4];
	//float		bright;
	double		range;
	vmsVector  dir;  //currently based on face normals
	int			refside;  //(seg*6)+tSide ie which tSide the light is on
} tLightMap;

extern int				numLightMaps;
extern tOglTexture	*lightMaps;  //Level Lightmaps - currently hardset to 5400, probably need to change this to a variable number
extern tLightMap		*lightData;  //Level lights
extern GLhandleARB	lmShaderProgs [3];

void InitLightmapShaders (void);
void CreateLightMaps (void);
void DestroyLightMaps (void);
int HaveLightMaps (void);
int GetLightColor (int tMapNum, tLightMap *pTempLight);

#define	USE_LIGHTMAPS \
			(gameStates.render.color.bLightMapsOk && \
			 gameOpts->render.color.bUseLightMaps && \
			 !IsMultiGame && \
			 !gameOpts->render.bDynLighting)

#endif //__lightmap_h
