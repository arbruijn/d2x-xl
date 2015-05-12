#ifndef _cameras_h
#define _cameras_h

#include "ogl_defs.h"
#include "ogl_texture.h"
#include "dynlight.h"

#if DBG
#	define MAX_SHADOWMAPS	0
#else
#	define MAX_SHADOWMAPS	0 //4
#endif

#define MAX_CAMERAS	100

typedef struct tCamera {
	CObject			obj;
	CObject			*pObj;
	int16_t				nId;
	int16_t				nSegment;
	int16_t				nSide;
	uint8_t				*screenBuf;
	GLuint			glTexId;
	time_t			nTimeout;
	char				nType;
	char				bValid;
	char				bVisible;
	char				bTimedOut;
	char				bAligned;
	char				bShadowMap;
	char				bTeleport;
	char				bMirror;
	int32_t				nWaitFrames;
	tUVL				uvlList [4];
	tTexCoord2f		texCoord [6];
#if RENDER2TEXTURE == 1
	tPixelBuffer	pb;
	CTexture		glTex;
#elif RENDER2TEXTURE == 2
	CFBO				fbo;
	CTexture			glTex;
#endif
	CFixMatrix		orient;
	fixang			curAngle;
	fixang			curDelta;
	time_t			t0;
} tCamera;

class CCamera : public CCanvas {
	private:
		tCamera	m_data;
	public:
		CCamera () { Init (); };
		~CCamera () { Destroy (); };
		void Init (void);
		int32_t Render (void);
		int32_t Ready (time_t t);
		void Reset (void);
		void Rotate (void);
		void Align (CSegFace *pFace, tUVL *pUVL, tTexCoord2f *pTexCoord, CFloatVector3 *pVertex);
		int32_t Create (int16_t nId, int16_t srcSeg, int16_t srcSide, int16_t tgtSeg, int16_t tgtSide, 
						CObject *pObj, int32_t bShadowMap, int32_t bTeleport);
		void Setup (int32_t nId, int16_t srcSeg, int16_t srcSide, int16_t tgtSeg, int16_t tgtSide, CObject *pObj, int32_t bTeleport);
		void Destroy (void);
		int32_t HaveBuffer (int32_t bCheckTexture);
		int32_t HaveTexture (void);
		inline void ReAlign (void) { m_data.bAligned = 0; }
		inline void SetVisible (char bVisible) { m_data.bVisible = bVisible; }
		inline char GetVisible (void) { return m_data.bVisible; }
		inline char GetTeleport (void) { return m_data.bTeleport; }
		inline char Valid (void) { return m_data.bValid; }
		inline CFixMatrix& Orient (void) { return m_data.orient; }
		//inline CBitmap& Buffer (void) { return *this; }
		inline tTexCoord2f* TexCoord (void) { return m_data.texCoord; }
		inline CObject* GetObject (void) { return m_data.pObj; }
		inline CFBO& FrameBuffer (void) { return m_data.fbo; } 
		inline GLuint DrawBuffer (void) { return m_data.bShadowMap ? FrameBuffer ().DepthBuffer () : FrameBuffer ().ColorBuffer (); }
		int32_t EnableBuffer (void);
		int32_t DisableBuffer (bool bPrepare = true);
		inline int16_t Id (void) { return m_data.nId; }
		inline char Type (void) { return m_data.nType; }

	private:
		int32_t CreateBuffer (void);
		int32_t BindBuffer (void);
		int32_t ReleaseBuffer (void);
		int32_t DestroyBuffer (void);
	};

//------------------------------------------------------------------------------

typedef struct tShadowMapInfo {
	public:
		int32_t			nCamera;
		CDynLight*	pLight;
} tShadowMapInfo;

class CCameraManager {
	private:
		CStack<CCamera>		m_cameras;
		CCamera*					m_current;
		CArray<char>			m_faceCameras;
		CArray<uint16_t>			m_objectCameras;
#if MAX_SHADOWMAPS < 0
		CStaticArray<tShadowMapInfo, -MAX_SHADOWMAPS> m_shadowMaps;
#elif MAX_SHADOWMAPS > 0
		CStaticArray<tShadowMapInfo, MAX_SHADOWMAPS> m_shadowMaps;
#endif

	public:
		int32_t				m_fboType;

	public:
		CCameraManager () : m_current (NULL), m_objectCameras (0), m_fboType (1) {}
		~CCameraManager ();
		int32_t Create ();
		void Destroy ();
		int32_t Render ();
		void Rotate (CObject *pObj);
		inline uint32_t Count (void) { return m_cameras.Buffer () ? m_cameras.ToS () : 0; }
		inline CCamera* Cameras (void) { return m_cameras.Buffer (); }
		//inline CCamera* Camera (int16_t i = 0) { return Cameras () + i; }
		inline int32_t CurrentIndex (void) { return int32_t (m_current - m_cameras.Buffer ()); }
		inline CCamera* Current (void) { return m_current; }
		inline void SetCurrent (CCamera* pCamera) { m_current = pCamera; }
		CCamera* Camera (CObject *pObj);
		inline int32_t GetObjectCamera (int32_t nObject);
		inline void SetObjectCamera (int32_t nObject, int32_t i);
		int32_t GetFaceCamera (int32_t nFace);
		void ReAlign (void);
		inline void SetFaceCamera (int32_t nFace, int32_t i);
		inline int32_t Index (CCamera* pCamera) { return m_cameras.Buffer () ? int32_t (pCamera - m_cameras.Buffer ()) : -1; }
		inline CCamera* Add (void) { return ((m_cameras.Buffer () || Create ()) && m_cameras.Grow ()) ? m_cameras.Top () : NULL; }
		inline CCamera* operator[] (uint32_t i) { return (i < m_cameras.ToS ()) ? &m_cameras [i] : NULL; }

#if MAX_SHADOWMAPS
		inline CCamera* ShadowMap (int32_t i) { return (m_shadowMaps [i].nCamera < 0) ? NULL : (*this) [i]; }
		inline CDynLight* ShadowLightSource (int32_t i) { return (m_shadowMaps [i].nCamera < 0) ? NULL : m_shadowMaps [i].pLight; }
		inline CCamera* AddShadowMap (int32_t i, CDynLight* pLight) { 
			CCamera* pCamera = Add ();
			if (!pCamera)
				return NULL;
			m_shadowMaps [i].nCamera = Index (pCamera);
			m_shadowMaps [i].BRP;
			return pCamera;
			}
		inline void DestroyShadowMap (int32_t i) {
			CCamera* pCamera = ShadowMap (i);
			if (pCamera) {
				pCamera->Destroy ();
				m_shadowMaps [i].nCamera = -1;
				m_shadowMaps [i].pLight = NULL;
				}
			}
#endif
};

extern CCameraManager cameraManager;

//------------------------------------------------------------------------------

#define USE_CAMERAS (extraGameInfo [0].bUseCameras && (!IsMultiGame || (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bUseCameras)))

//------------------------------------------------------------------------------

#endif // _cameras_h
