#ifndef __lightmap_h
#define __lightmap_h

#include "ogl_defs.h"
#include "carray.h"
#include "color.h"
#include "math.h"
#include "gr.h"
#include "menu.h"
#include "ogl_texture.h"

//------------------------------------------------------------------------------

#define MAX_LIGHTMAP_WIDTH	256
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
	int32_t		nIndex;  //(seg*6)+CSide ie which CSide the light is on
} tLightmapInfo;

typedef struct tLightmap {
	CFloatVector3	*pBm;
} tLightmap;

typedef struct tLightmapBuffer {
	GLuint		handle;
	CRGBColor	pBm [LIGHTMAP_BUFWIDTH][LIGHTMAP_BUFWIDTH];
} tLightmapBuffer;

typedef struct tLightmapList {
	CArray<tLightmapInfo>	info;
	CArray<tLightmapBuffer>	buffers;
	int32_t						nBuffers;
	int32_t						nLights; 
	uint16_t						nLightmaps;
} tLightmapList;

typedef CSegFace* tSegFacePtr;

//------------------------------------------------------------------------------

class CLightmapFaceData {
	public:
		int32_t					m_nType;
		int32_t					m_nColor;
		CFixVector				m_vNormal;
		CFixVector				m_vCenter;
		uint16_t					m_sideVerts [4]; 
		CVertColorData			m_vcd;
		CRGBColor				m_texColor [MAX_LIGHTMAP_WIDTH * MAX_LIGHTMAP_WIDTH];
		CFixVector				m_pixelPos [MAX_LIGHTMAP_WIDTH * MAX_LIGHTMAP_WIDTH]; 

	void Setup (CSegFace* pFace);
	};

//------------------------------------------------------------------------------

class CLightmapData : public CLightmapFaceData {
	public:
		float						nOffset [MAX_LIGHTMAP_WIDTH];
		CArray<tSegFacePtr>	faceList;
		int32_t					nBlackLightmaps;
		int32_t					nWhiteLightmaps;
		CSegFace*				pFace;
	};

//------------------------------------------------------------------------------

class CLightmapProgress {
	private:
		CMenu			*m_pProgressMenu;
		CMenuItem	*m_pTotalProgress;
		CMenuItem	*m_pLevelProgress;
		CMenuItem	*m_pLevelCount;
		CMenuItem	*m_pTime;
		int32_t		m_nLocalProgress;
		float			m_fTotal;
		int32_t		m_tStart;
		int32_t		m_nSkipped;
		bool			m_bActive;

	public:
		CLightmapProgress () 
			: m_pProgressMenu (NULL), m_pTotalProgress (NULL), m_pLevelProgress (NULL), m_pLevelCount (NULL), m_pTime (NULL), m_nLocalProgress (0), m_fTotal (0.0f), m_tStart (0), m_nSkipped (0), m_bActive (false)
			{ }

		void Setup (void);
		void Update (void);
		void Skip (int32_t i);
		inline void Reset (void) { m_bActive = false; }

		static inline const int32_t Scale (void) { return 100; }
	};

//------------------------------------------------------------------------------

class CLightmapManager {
	private:
		CLightmapData		m_data;
		tLightmapList		m_list;
		CLightmapProgress	m_progress;
		int32_t				m_bSuccess;

	public:
		CLightmapManager () { Init (); } 
		~CLightmapManager () { Destroy (); }
		void Init (void);
		int32_t Setup (int32_t nLevel);
		void Destroy (void);
		void RestoreLights (int32_t bVariable);
		int32_t Bind (int32_t nLightmap);
		int32_t BindAll (void);
		void Release (void);
		int32_t Create (int32_t nLevel);
		void Build (CSegFace* pFace, int32_t nThread);
		int32_t BuildAll (int32_t nFace);
		inline tLightmapBuffer* Buffer (uint32_t i = 0) { return &m_list.buffers [i]; }
		inline int32_t HaveLightmaps (void) { return !gameStates.app.bNostalgia && (m_list.buffers.Buffer () != NULL); }
		inline CSegFace* CurrentFace (void) { return m_data.pFace; }
		inline CLightmapProgress& Progress (void) { return m_progress; }
		inline void SetupProgress (void) { m_progress.Setup (); }
		inline void ResetProgress (void) { m_progress.Reset (); }

	private:
		int32_t Init (int32_t bVariable);
		bool FaceIsInvisible (CSegFace* pFace);
		inline void ComputePixelOffset (CFixVector& vPos, CFixVector& v1, CFixVector& v2, int32_t nOffset);
		double SideRad (int32_t nSegment, int32_t nSide);
		int32_t CountLights (int32_t bVariable);
		void Copy (CRGBColor *pTexColor, uint16_t nLightmap);
		void CreateSpecial (CRGBColor *pTexColor, uint16_t nLightmap, uint8_t nColor);
		void Realloc (int32_t nBuffers);
		int32_t Save (int32_t nLevel);
		int32_t Load (int32_t nLevel);
		void ToGrayScale (void);
		void Posterize (void);
		char* Filename (char *pszFilename, int32_t nLevel);
		void Blur (CSegFace* pFace, CLightmapFaceData& source, CLightmapFaceData& dest, int32_t direction);
		void Blur (CSegFace* pFace, CLightmapFaceData& source);

		static int32_t CompareFaces (const tSegFacePtr* pf, const tSegFacePtr* pm);
	};

extern CLightmapManager lightmapManager;

//------------------------------------------------------------------------------


#define	USE_LIGHTMAPS \
			(gameStates.render.bLightmapsOk && \
			 gameOpts->render.bUseLightmaps && \
			 !IsMultiGame && \
			 (gameOpts->render.nLightingMethod == 0))

//------------------------------------------------------------------------------

//extern CTexture	*lightmaps;
extern int32_t					lightmapWidth [5];
extern GLhandleARB		lmShaderProgs [3];

//------------------------------------------------------------------------------

int32_t SetupLightmap (CSegFace* pFace);

//------------------------------------------------------------------------------

#endif //__lightmap_h
