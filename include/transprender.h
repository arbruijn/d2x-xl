#ifndef _TRANSPRENDER_H
#define _TRANSPRENDER_H

#include "particles.h"
#include "lightning.h"
#include "thrusterflames.h"

//------------------------------------------------------------------------------

#define ITEM_DEPTHBUFFER_SIZE	250000
#define ITEM_BUFFER_SIZE		75000000
#define TRANSP_DEPTH_HASH		1

typedef enum tTranspItemType {
	tiSpark,
	tiParticle,
	tiLightning,
	tiSprite,
	tiLightTrail,
	tiObject,
	tiSphere,
	tiThruster,
	tiFace,
	tiPoly
} tTranspItemType;

class CTranspItem {
	public:
		CTranspItem*		nextItemP;
		int32_t					nItem;
		int32_t					z;
		uint16_t				bTransformed :1;
		uint16_t				bValid :1;
		uint16_t				bRendered :1;

		virtual int32_t Size (void) = 0;
		virtual void Render (void) = 0;
		virtual /*inline*/ tTranspItemType Type (void) = 0;
		inline CTranspItem& operator= (CTranspItem& other) { 
			memcpy (this, &other, Size ());
			return *this;
			}
	};

class CTranspPoly : public CTranspItem {
	public:
		CSegFace*			faceP;
		tFaceTriangle*		triP;
		CBitmap*				bmP;
		CFloatVector		vertices [8];
		tTexCoord2f			texCoord [8];
		CFloatVector		color [8];
		int16_t					sideLength [8];
		int16_t					nSegment;
		int32_t					nWrap;
		int32_t					nPrimitive;
		char					nVertices;
		char					nColors;
		char					bDepthMask;
		char					bAdditive;

		virtual int32_t Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual /*inline*/ tTranspItemType Type (void) { return tiPoly; }

	private:
		void RenderFace (void);
	};

class CTranspObject : public CTranspItem {
	public:
		CObject*				objP;
		CFixVector			vScale;

	virtual int32_t Size (void) { return sizeof (*this); }
	virtual void Render (void);
	virtual /*inline*/ tTranspItemType Type (void) { return tiObject; }
	};

class CTranspSprite : public CTranspItem {
	public:
		CBitmap*				bmP;
		CFloatVector		position;
		CFloatVector		color;
		int32_t					nWidth;
		int32_t					nHeight;
		char					nFrame;
		char					bColor;
		char					bAdditive;
		char					bDepthMask;
		float					fSoftRad;

		virtual int32_t Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual /*inline*/ tTranspItemType Type (void) { return tiSprite; }
	};

class CTranspSpark : public CTranspItem {
	public:
		CFloatVector		position;
		int32_t					nSize;
		char					nFrame;
		char					nRotFrame;
		char					nOrient;
		char					nType;

		virtual int32_t Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual /*inline*/ tTranspItemType Type (void) { return tiSpark; }
	};

class	CTranspParticle : public CTranspItem {
	public:
		CParticle*			particle;
		float					fBrightness;

		virtual int32_t Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual /*inline*/ tTranspItemType Type (void) { return tiParticle; }

	private:
		void RenderBullet (CParticle *bullet);

	};

typedef enum tTranspSphereType {
	riSphereShield,
	riMonsterball
} tTranspSphereType;

class CTranspSphere : public CTranspItem {
	public:
		tTranspSphereType	nType;
		CFloatVector		color;
		CObject				*objP;
		int32_t					nSize;
		char					bAdditive;

		virtual int32_t Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual /*inline*/ tTranspItemType Type (void) { return tiSphere; }
	};

class CTranspLightning : public CTranspItem {
	public:
		CLightning			*lightning;
		int16_t					nDepth;

		virtual int32_t Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual /*inline*/ tTranspItemType Type (void) { return tiLightning; }
	};

class CTranspLightTrail : public CTranspItem {
	public:
		CBitmap*					bmP;
		CFloatVector			vertices [8];
		tTexCoord2f				texCoord [8];
		CFloatVector			color;
		char						bTrail;

		virtual int32_t Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual /*inline*/ tTranspItemType Type (void) { return tiLightTrail; }
	};

class CTranspThruster : public CTranspItem {
	public:
		CObject*					objP;
		tThrusterInfo			info;
		int32_t						nThruster;

		virtual int32_t Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual /*inline*/ tTranspItemType Type (void) { return tiThruster; }
	};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class CTranspItemBuffers {
	public:
		CArray<CTranspItem*> depthBuffer;
		CByteArray				itemHeap;
		int32_t						nHeapSize;
		CTranspItem*			freeList [tiPoly + 1];
		int32_t						nMinOffs;
		int32_t						nMaxOffs;
		int32_t						nItems [2];
		CTranspItem**			bufP;

	int32_t Create (void);
	void Destroy (void);
	void Clear (void);
	inline void Reset (void) { nItems [0] = 0; }
	void ResetFreeList (void);
	inline CTranspItem* AllocItem (int32_t nType, int32_t nSize);
	};

class CTranspRenderData {
	public:
		CTranspItemBuffers buffers [MAX_THREADS];
		int32_t					nCurType;
		int32_t					nPrevType;
		int32_t					zMin;
		int32_t					zMax;
		int32_t					nMinOffs;
		int32_t					nMaxOffs;
		double				zScale;
		int32_t					nWrap;
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
		char					bAllowAdd;
		char					bHaveDepthBuffer;
		char					bRenderGlow;
		char					bSoftBlend;
		char					bReady;
		CBitmap*				bmP [3];
		CFixVector			vViewer [2];
		CFloatVector		vViewerf [2];

	CTranspRenderData () { memset (this, 0, sizeof (*this)); }
	};

class CTranspItemData : public CTranspItem {
	public:
		int32_t				nType;
		int32_t				nSize;
		int32_t				nDepth;
		int32_t				nIndex;
		};

//------------------------------------------------------------------------------

class CTransparencyRenderer {
	private:
		CTranspRenderData m_data;

	public:
		int32_t AllocBuffers (void);
		void FreeBuffers (void);
		void ResetBuffers (void);
		void InitBuffer (int32_t zMin, int32_t zMax, int32_t nWindow);
		inline CTranspItem* AllocItem (int32_t nType, int32_t nSize, int32_t nThread);
		inline void Init (void) {
			m_data.nMinOffs = ITEM_DEPTHBUFFER_SIZE;
			m_data.nMaxOffs = 0;
			AllocBuffers ();
			}
		void Reset (void);
		int32_t ItemCount (int32_t i = 1);
		int32_t Add (CTranspItem* item, CFixVector vPos, int32_t nOffset = 0, bool bClamp = false, int32_t bTransformed = 0);
		inline int32_t AddFace (CSegFace *faceP) {
			return gameStates.render.bTriangleMesh ? AddFaceTris (faceP) : AddFaceQuads (faceP);
			}
		int32_t AddPoly (CSegFace *faceP, tFaceTriangle *triP, CBitmap *bmP,
						 CFloatVector *vertices, char nVertices, tTexCoord2f *texCoord, CFloatVector *color,
						 CFaceColor *altColor, char nColors, char bDepthMask, int32_t nPrimitive, int32_t nWrap, int32_t bAdditive,
						 int16_t nSegment);
		int32_t AddObject (CObject *objP);
		int32_t AddSprite (CBitmap *bmP, const CFixVector& position, CFloatVector *color,
							  int32_t nWidth, int32_t nHeight, char nFrame, char bAdditive, float fSoftRad);
		int32_t AddSpark (const CFixVector& position, char nType, int32_t nSize, char nFrame, char nRotFrame, char nOrient);
		int32_t AddSphere (tTranspSphereType nType, float red, float green, float blue, float alpha, CObject *objP, char bAdditive, fix nSize = 0);
		int32_t AddParticle (CParticle *particle, float fBrightness, int32_t nThread);
		int32_t AddLightning (CLightning *lightningP, int16_t nDepth);
		int32_t AddLightTrail (CBitmap *bmP, CFloatVector *vThruster, tTexCoord2f *tcThruster, CFloatVector *vFlame, tTexCoord2f *tcFlame, CFloatVector *colorP);
		int32_t AddThruster (CObject* objP, tThrusterInfo* infoP, int32_t nThruster);
		void Render (int32_t nWindow);
		void CreateRenderThreads (void);
		void DestroyRenderThreads (void);
		void Free (void);

		inline int32_t Depth (CFixVector vPos, int32_t bTransformed) {
#if TRANSP_DEPTH_HASH
			return (bTransformed > 0) ? vPos.Mag () : CFixVector::Dist (vPos, m_data.vViewer [0]);
#else
			if (bTransformed < 1)
				transformation.Transform (vPos, vPos);
			return vPos.dir.coord.z;
#endif
			}

		inline float Depth (CFloatVector vPos, int32_t bTransformed) {
#if TRANSP_DEPTH_HASH
			return (bTransformed > 0) ? vPos.Mag () : CFloatVector::Dist (vPos, m_data.vViewerf [0]);
#else
			if (bTransformed < 1)
				transformation.Transform (vPos, vPos);
			return vPos.dir.coord.z;
#endif
			}

		void FlushSparkBuffer (void);
		void FlushParticleBuffer (int32_t nType);
		void FlushBuffers (int32_t nType, CTranspItem *itemP = NULL);

		int32_t SoftBlend (int32_t nFlag);
		int32_t LoadTexture (CSegFace* faceP, CBitmap *bmP, int16_t nTexture, int32_t nFrame, int32_t bDecal, int32_t bLightmaps, int32_t nWrap);
		void ResetBitmaps (void);

		inline CTranspRenderData& Data (void) { return m_data; }

		inline char Ready (void) { return m_data.bReady; }

	private:
		inline int32_t HeapSize (void);
		inline int32_t DepthBuffer (void);
		void ResetFreeList (void);
		int32_t AddFaceTris (CSegFace *faceP);
		int32_t AddFaceQuads (CSegFace *faceP);

		int32_t NeedDepthBuffer (void);
#if 0
		void RenderFace (CTranspPoly *item);
		void RenderPoly (CTranspPoly *item);
		void RenderObject (CTranspObject *item);
		void RenderSprite (CTranspSprite *item);
		void RenderSpark (CTranspSpark *item);
		void RenderSphere (CTranspSphere *item);
		void RenderBullet (CParticle *pParticle);
		void RenderParticle (CTranspParticle *item);
		void RenderLightning (CTranspLightning *item);
		void RenderLightTrail (CTranspLightTrail *item);
		void RenderThruster (CTranspThruster *item);
#endif
		int32_t RenderItem (CTranspItem *item);
		void RenderBuffer (CTranspItemBuffers& buffer, bool bCleanup);
	};

extern CTransparencyRenderer transparencyRenderer;

//------------------------------------------------------------------------------

#endif /* _TRANSPRENDER_H */
