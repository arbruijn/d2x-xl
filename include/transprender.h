#ifndef _TRANSPRENDER_H
#define _TRANSPRENDER_H

#include "particles.h"
#include "lightning.h"

//------------------------------------------------------------------------------

#define ITEM_DEPTHBUFFER_SIZE	100000
#define ITEM_BUFFER_SIZE		100000
#define TRANSP_DEPTH_HASH		1

typedef enum tTranspItemType {
	tiSprite,
	tiSpark,
	tiSphere,
	tiParticle,
	tiLightning,
	tiThruster,
	tiObject,
	tiFace,
	tiPoly
} tTranspItemType;

typedef struct tTranspPoly {
	CSegFace*			faceP;
	tFaceTriangle*		triP;
	CBitmap*				bmP;
	CFloatVector		vertices [8];
	tTexCoord2f			texCoord [8];
	tRgbaColorf			color [8];
	short					sideLength [8];
	short					nSegment;
	int					nWrap;
	int					nPrimitive;
	char					nVertices;
	char					nColors;
	char					bDepthMask;
	char					bAdditive;
} tTranspPoly;

typedef struct tTranspObject {
	CObject				*objP;
	CFixVector			vScale;
} tTranspObject;

typedef struct tTranspSprite {
	CBitmap				*bmP;
	CFloatVector		position;
	tRgbaColorf			color;
	int					nWidth;
	int					nHeight;
	char					nFrame;
	char					bColor;
	char					bAdditive;
	char					bDepthMask;
	float					fSoftRad;
} tTranspSprite;

typedef struct tTranspSpark {
	CFloatVector				position;
	int					nSize;
	char					nFrame;
	char					nType;
} tTranspSpark;

typedef struct tTranspParticle {
	CParticle			*particle;
	float					fBrightness;
} tTranspParticle;

typedef enum tTranspSphereType {
	riSphereShield,
	riMonsterball
} tTranspSphereType;

typedef struct tTranspSphere {
	tTranspSphereType	nType;
	tRgbaColorf			color;
	CObject				*objP;
	int					nSize;
	char					bAdditive;
} tTranspSphere;

typedef struct tTranspLightning {
	CLightning			*lightning;
	short					nDepth;
} tTranspLightning;

typedef struct tTranspLightTrail {
	CBitmap					*bmP;
	CFloatVector			vertices [7];
	tTexCoord2f				texCoord [7];
	tRgbaColorf				color;
	char						bTrail;
} tTranspLightTrail;

typedef struct tTranspItem {
	struct tTranspItem	*pNextItem;
	struct tTranspItem	*parentP;
	tTranspItemType		nType;
	int						nItem;
	int						z;
	int						bTransformed;
	bool						bValid;
	bool						bRendered;
	union {
		tTranspPoly					poly;
		tTranspObject				object;
		tTranspSprite				sprite;
		tTranspSpark				spark;
		tTranspParticle			particle;
		tTranspSphere				sphere;
		tTranspLightning			lightning;
		tTranspLightTrail			thruster;
	} item;
} tTranspItem;

typedef struct tTranspItemList {
	tTranspItem*	head;
	tTranspItem*	tail;
} tTranspItemList;

typedef struct tTranspItemBuffer {
	CArray<tTranspItemList> depthBuffer;
	CArray<tTranspItem> itemLists;
	int					nMinOffs;
	int					nMaxOffs;
	int					nItems [2];
	int					nFreeItems;
	tTranspItemList	freeList;
	int					nCurType;
	int					nPrevType;
	int					zMin;
	int					zMax;
	double				zScale;
	int					nWrap;
	char					nFrame;
	char					bClientState;
	char					bTextured;
	char					bClientColor;
	char					bClientTexCoord;
	char					bDisplay;
	char					bHaveParticles;
	char					bLightmaps;
	char					bUseLightmaps;
	char					bDecal;
	char					bSplitPolys;
	CBitmap*				bmP [3];
	CFixVector			vViewer [2];
	CFloatVector		vViewerf [2];
} tTranspItemBuffer;

typedef struct tTranspItemData {
	tTranspItem		item;
	int				nType;
	int				nSize;
	int				nDepth;
	int				nIndex;
	} tTranspItemData;

//------------------------------------------------------------------------------

class CTransparencyRenderer {
	private:
		tTranspItemBuffer	m_data;

	public:
		int AllocBuffers (void);
		void FreeBuffers (void);
		void ResetBuffers (void);
		void InitBuffer (int zMin, int zMax);
		inline void Init (void) {
			m_data.nMinOffs = ITEM_DEPTHBUFFER_SIZE;
			m_data.nMaxOffs = 0;
			}
		inline void Reset (void) { m_data.nItems [0] = 0; }
		inline int ItemCount (int i = 1) { return m_data.nItems [i]; }
		int Add (tTranspItemType nType, void *itemData, int itemSize, CFixVector vPos, int nOffset = 0, bool bClamp = false, int bTransformed = 0);
		inline int AddFace (CSegFace *faceP) {
			return gameStates.render.bTriangleMesh ? AddFaceTris (faceP) : AddFaceQuads (faceP);
			}
		int AddPoly (CSegFace *faceP, tFaceTriangle *triP, CBitmap *bmP,
							CFloatVector *vertices, char nVertices, tTexCoord2f *texCoord, tRgbaColorf *color,
							tFaceColor *altColor, char nColors, char bDepthMask, int nPrimitive, int nWrap, int bAdditive,
							short nSegment);
		int AddObject (CObject *objP);
		int AddSprite (CBitmap *bmP, const CFixVector& position, tRgbaColorf *color,
							  int nWidth, int nHeight, char nFrame, char bAdditive, float fSoftRad);
		int AddSpark (const CFixVector& position, char nType, int nSize, char nFrame);
		int AddSphere (tTranspSphereType nType, float red, float green, float blue, float alpha, CObject *objP, char bAdditive, fix nSize = 0);
		int AddParticle (CParticle *particle, float fBrightness, int nThread);
		int AddLightning (CLightning *lightningP, short nDepth);
		int AddLightTrail (CBitmap *bmP, CFloatVector *vThruster, tTexCoord2f *tcThruster, CFloatVector *vFlame, tTexCoord2f *tcFlame, tRgbaColorf *colorP);
		void Render (int nWindow);
		void StartRenderThreads (void);
		void EndRenderThreads (void);
		void Free (void);
		tTranspItem *SetParent (int nChild, int nParent);

		inline int Depth (CFixVector vPos, int bTransformed) {
#if TRANSP_DEPTH_HASH
			return (bTransformed > 0) ? vPos.Mag () : CFixVector::Dist (vPos, m_data.vViewer [0]);
#else
			if (bTransformed < 1)
				transformation.Transform (vPos, vPos);
			return vPos [Z];
#endif
			}

		inline float Depth (CFloatVector vPos, int bTransformed) {
#if TRANSP_DEPTH_HASH
			return (bTransformed > 0) ? vPos.Mag () : CFloatVector::Dist (vPos, m_data.vViewerf [0]);
#else
			if (bTransformed < 1)
				transformation.Transform (vPos, vPos);
			return vPos [Z];
#endif
			}

	private:
		void ResetFreeList (void);
		int AddFaceTris (CSegFace *faceP);
		int AddFaceQuads (CSegFace *faceP);
		void ResetBitmaps (void);
		int LoadImage (CBitmap *bmP, int nFrame, int bDecal, int bLightmaps, int nWrap);

		void FlushSparkBuffer (void);
		void FlushParticleBuffer (int nType);
		void FlushBuffers (int nType);
		void RenderFace (tTranspPoly *item);
		void RenderPoly (tTranspPoly *item);
		void RenderObject (tTranspObject *item);
		void RenderSprite (tTranspSprite *item);
		void RenderSpark (tTranspSpark *item);
		void RenderSphere (tTranspSphere *item);
		void RenderBullet (CParticle *pParticle);
		void RenderParticle (tTranspParticle *item);
		void RenderLightning (tTranspLightning *item);
		void RenderLightTrail (tTranspLightTrail *item);
		int RenderItem (struct tTranspItem *pl);
	};

extern CTransparencyRenderer transparencyRenderer;

//------------------------------------------------------------------------------

#endif /* _TRANSPRENDER_H */
