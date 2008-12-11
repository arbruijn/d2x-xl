#ifndef __lightmap_h
#define __lightmap_h

#include "ogl_defs.h"
#include "carray.h"
#include "color.h"
#include "math.h"
#include "gr.h"
#include "newmenu.h"
#include "ogl_texture.h"

//------------------------------------------------------------------------------

#define MAX_LIGHTMAP_WIDTH	128
#define LIGHTMAP_WIDTH		lightmapWidth [gameOpts->render.nLightmapQuality]
#define LIGHTMAP_BUFWIDTH	512
#define LIGHTMAP_ROWSIZE	(LIGHTMAP_BUFWIDTH / LIGHTMAP_WIDTH)
#define LIGHTMAP_BUFSIZE	(LIGHTMAP_ROWSIZE * LIGHTMAP_ROWSIZE)

//------------------------------------------------------------------------------

typedef struct tLightmapInfo {
	CFixVector	vPos;
	CFixVector	vDir;  //currently based on face normals
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

typedef struct tLightmapList {
	CArray<tLightmapInfo>	info;
	CArray<tLightmapBuffer>	buffers;
	int							nBuffers;
	int							nLights; 
	ushort						nLightmaps;
} tLightmapList;

typedef struct tLightmapData {
	int					nType;
	int					nColor;
	CFixVector			vNormal;
	ushort				sideVerts [4]; 
	CVertColorData		vcd;
	tRgbColorb			texColor [MAX_LIGHTMAP_WIDTH * MAX_LIGHTMAP_WIDTH];
	CFixVector			pixelPos [MAX_LIGHTMAP_WIDTH * MAX_LIGHTMAP_WIDTH]; 
	double				fOffset [MAX_LIGHTMAP_WIDTH];
	tFace				*faceP;
	} tLightmapData;

class CLightmapManager {
	private:
		tLightmapData	m_data;
		tLightmapList	m_list;

	public:
		CLightmapManager () { Init (); } 
		~CLightmapManager () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void RestoreLights (int bVariable);
		int Bind (int nLightmap);
		int BindAll (void);
		void Release (void);
		int Create (int nLevel);
		void Build (int nThread);
		void BuildAll (int nFace, int nThread);
		int _CDECL_ Thread (void *pThreadId);
		inline tLightmapBuffer* Buffer (uint i = 0) { return &m_list.buffers [i]; }
		inline int HaveLightmaps (void) { return !gameStates.app.bNostalgia && (m_list.buffers.Buffer () != NULL); }

	private:
		int Init (int bVariable);
		inline void ComputePixelPos (CFixVector *vPos, CFixVector v1, CFixVector v2, double fOffset);
		double SideRad (int nSegment, int nSide);
		int CountLights (int bVariable);
		void Copy (tRgbColorb *texColorP, ushort nLightmap);
		void CreateSpecial (tRgbColorb *texColorP, ushort nLightmap, ubyte nColor);
		void Realloc (int nBuffers);
		int Save (int nLevel);
		int Load (int nLevel);
		char* Filename (char *pszFilename, int nLevel);

	};

extern CLightmapManager lightmapManager;

//------------------------------------------------------------------------------


#define	USE_LIGHTMAPS \
			(gameStates.render.color.bLightmapsOk && \
			 gameOpts->render.color.bUseLightmaps && \
			 !IsMultiGame && \
			 (gameOpts->render.nLightingMethod == 0))

//------------------------------------------------------------------------------

//extern CTexture	*lightmaps;
extern tLightmapData		lightmapData;
extern int					lightmapWidth [5];
extern GLhandleARB		lmShaderProgs [3];

//------------------------------------------------------------------------------

#endif //__lightmap_h
