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
	tiSprite,
	tiSpark,
	tiSphere,
	tiParticle,
	tiLightning,
	tiLightTrail,
	tiObject,
	tiThruster,
	tiFace,
	tiPoly
} tTranspItemType;

class CTranspItem {
	public:
		CTranspItem*		nextItemP;
		int					nItem;
		int					z;
		ushort				bTransformed :1;
		ushort				bValid :1;
		ushort				bRendered :1;

		virtual int Size (void) = 0;
		virtual void Render (void) = 0;
		virtual inline tTranspItemType Type (void) = 0;
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
		short					sideLength [8];
		short					nSegment;
		int					nWrap;
		int					nPrimitive;
		char					nVertices;
		char					nColors;
		char					bDepthMask;
		char					bAdditive;

		virtual int Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual inline tTranspItemType Type (void) { return tiPoly; }

	private:
		void RenderFace (void);
	};

class CTranspObject : public CTranspItem {
	public:
		CObject*				objP;
		CFixVector			vScale;

	virtual int Size (void) { return sizeof (*this); }
	virtual void Render (void);
	virtual inline tTranspItemType Type (void) { return tiObject; }
	};

class CTranspSprite : public CTranspItem {
	public:
		CBitmap*				bmP;
		CFloatVector		position;
		CFloatVector		color;
		int					nWidth;
		int					nHeight;
		char					nFrame;
		char					bColor;
		char					bAdditive;
		char					bDepthMask;
		float					fSoftRad;

		virtual int Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual inline tTranspItemType Type (void) { return tiSprite; }
	};

class CTranspSpark : public CTranspItem {
	public:
		CFloatVector		position;
		int					nSize;
		char					nFrame;
		char					nRotFrame;
		char					nOrient;
		char					nType;

		virtual int Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual inline tTranspItemType Type (void) { return tiSpark; }
	};

class	CTranspParticle : public CTranspItem {
	public:
		CParticle*			particle;
		float					fBrightness;

		virtual int Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual inline tTranspItemType Type (void) { return tiParticle; }

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
		int					nSize;
		char					bAdditive;

		virtual int Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual inline tTranspItemType Type (void) { return tiSphere; }
	};

class CTranspLightning : public CTranspItem {
	public:
		CLightning			*lightning;
		short					nDepth;

		virtual int Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual inline tTranspItemType Type (void) { return tiLightning; }
	};

class CTranspLightTrail : public CTranspItem {
	public:
		CBitmap*					bmP;
		CFloatVector			vertices [8];
		tTexCoord2f				texCoord [8];
		CFloatVector			color;
		char						bTrail;

		virtual int Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual inline tTranspItemType Type (void) { return tiLightTrail; }
	};

class CTranspThruster : public CTranspItem {
	public:
		CObject*					objP;
		tThrusterInfo			info;
		int						nThruster;

		virtual int Size (void) { return sizeof (*this); }
		virtual void Render (void);
		virtual inline tTranspItemType Type (void) { return tiThruster; }
	};

typedef struct tTranspItemBuffer {
	CArray<CTranspItem*> depthBuffer;
	CByteArray			itemHeap;
	int					nHeapSize;
	CTranspItem*		freeList [tiPoly + 1];
	int					nMinOffs;
	int					nMaxOffs;
	int					nItems [2];
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
	char					bAllowAdd;
	char					bHaveDepthBuffer;
	char					bRenderGlow;
	char					bSoftBlend;
	CBitmap*				bmP [3];
	CFixVector			vViewer [2];
	CFloatVector		vViewerf [2];
} tTranspItemBuffer;

class CTranspItemData : public CTranspItem {
	public:
		int				nType;
		int				nSize;
		int				nDepth;
		int				nIndex;
		};

//------------------------------------------------------------------------------

class CTransparencyRenderer {
	private:
		tTranspItemBuffer	m_data;

	public:
		int AllocBuffers (void);
		void FreeBuffers (void);
		void ResetBuffers (void);
		void InitBuffer (int zMin, int zMax, int nWindow);
		inline CTranspItem* AllocItem (int nType, int nSize);
		inline void Init (void) {
			m_data.nMinOffs = ITEM_DEPTHBUFFER_SIZE;
			m_data.nMaxOffs = 0;
			}
		inline void Reset (void) { m_data.nItems [0] = 0; }
		inline int ItemCount (int i = 1) { return m_data.nItems [i]; }
		int Add (CTranspItem* item, CFixVector vPos, int nOffset = 0, bool bClamp = false, int bTransformed = 0);
		inline int AddFace (CSegFace *faceP) {
			return gameStates.render.bTriangleMesh ? AddFaceTris (faceP) : AddFaceQuads (faceP);
			}
		int AddPoly (CSegFace *faceP, tFaceTriangle *triP, CBitmap *bmP,
							CFloatVector *vertices, char nVertices, tTexCoord2f *texCoord, CFloatVector *color,
							CFaceColor *altColor, char nColors, char bDepthMask, int nPrimitive, int nWrap, int bAdditive,
							short nSegment);
		int AddObject (CObject *objP);
		int AddSprite (CBitmap *bmP, const CFixVector& position, CFloatVector *color,
							  int nWidth, int nHeight, char nFrame, char bAdditive, float fSoftRad);
		int AddSpark (const CFixVector& position, char nType, int nSize, char nFrame, char nRotFrame, char nOrient);
		int AddSphere (tTranspSphereType nType, float red, float green, float blue, float alpha, CObject *objP, char bAdditive, fix nSize = 0);
		int AddParticle (CParticle *particle, float fBrightness, int nThread);
		int AddLightning (CLightning *lightningP, short nDepth);
		int AddLightTrail (CBitmap *bmP, CFloatVector *vThruster, tTexCoord2f *tcThruster, CFloatVector *vFlame, tTexCoord2f *tcFlame, CFloatVector *colorP);
		int AddThruster (CObject* objP, tThrusterInfo* infoP, int nThruster);
		void Render (int nWindow);
		void StartRenderThreads (void);
		void EndRenderThreads (void);
		void Free (void);

		inline int Depth (CFixVector vPos, int bTransformed) {
#if TRANSP_DEPTH_HASH
			return (bTransformed > 0) ? vPos.Mag () : CFixVector::Dist (vPos, m_data.vViewer [0]);
#else
			if (bTransformed < 1)
				transformation.Transform (vPos, vPos);
			return vPos.dir.coord.z;
#endif
			}

		inline float Depth (CFloatVector vPos, int bTransformed) {
#if TRANSP_DEPTH_HASH
			return (bTransformed > 0) ? vPos.Mag () : CFloatVector::Dist (vPos, m_data.vViewerf [0]);
#else
			if (bTransformed < 1)
				transformation.Transform (vPos, vPos);
			return vPos.dir.coord.z;
#endif
			}

		void FlushSparkBuffer (void);
		void FlushParticleBuffer (int nType);
		void FlushBuffers (int nType, CTranspItem *itemP = NULL);

		int SoftBlend (int nFlag);
		int LoadTexture (CBitmap *bmP, int nFrame, int bDecal, int bLightmaps, int nWrap);
		void ResetBitmaps (void);

		inline tTranspItemBuffer& Data (void) { return m_data; }

	private:
		void ResetFreeList (void);
		int AddFaceTris (CSegFace *faceP);
		int AddFaceQuads (CSegFace *faceP);

		int NeedDepthBuffer (void);
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
		int RenderItem (CTranspItem *item);
	};

extern CTransparencyRenderer transparencyRenderer;

//------------------------------------------------------------------------------

#endif /* _TRANSPRENDER_H */
