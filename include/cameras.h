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
	CObject			*objP;
	short				nId;
	short				nSegment;
	short				nSide;
	ubyte				*screenBuf;
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
	int				nWaitFrames;
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
		int Render (void);
		int Ready (time_t t);
		void Reset (void);
		void Rotate (void);
		void Align (CSegFace *faceP, tUVL *uvlP, tTexCoord2f *texCoordP, CFloatVector3 *vertexP);
		int Create (short nId, short srcSeg, short srcSide, short tgtSeg, short tgtSide, 
						CObject *objP, int bShadowMap, int bTeleport);
		void Setup (int nId, short srcSeg, short srcSide, short tgtSeg, short tgtSide, CObject *objP, int bTeleport);
		void Destroy (void);
		int HaveBuffer (int bCheckTexture);
		int HaveTexture (void);
		inline void ReAlign (void) { m_data.bAligned = 0; }
		inline void SetVisible (char bVisible) { m_data.bVisible = bVisible; }
		inline char GetVisible (void) { return m_data.bVisible; }
		inline char GetTeleport (void) { return m_data.bTeleport; }
		inline char Valid (void) { return m_data.bValid; }
		inline CFixMatrix& Orient (void) { return m_data.orient; }
		//inline CBitmap& Buffer (void) { return *this; }
		inline tTexCoord2f* TexCoord (void) { return m_data.texCoord; }
		inline CObject* GetObject (void) { return m_data.objP; }
		inline CFBO& FrameBuffer (void) { return m_data.fbo; } 
		inline GLuint DrawBuffer (void) { return m_data.bShadowMap ? FrameBuffer ().DepthBuffer () : FrameBuffer ().ColorBuffer (); }
		int EnableBuffer (void);
		int DisableBuffer (bool bPrepare = true);
		inline short Id (void) { return m_data.nId; }
		inline char Type (void) { return m_data.nType; }

	private:
		int CreateBuffer (void);
		int BindBuffer (void);
		int ReleaseBuffer (void);
		int DestroyBuffer (void);
	};

//------------------------------------------------------------------------------

typedef struct tShadowMapInfo {
	public:
		int			nCamera;
		CDynLight*	lightP;
} tShadowMapInfo;

class CCameraManager {
	private:
		CStack<CCamera>		m_cameras;
		CCamera*					m_current;
		CArray<char>			m_faceCameras;
		CArray<ushort>			m_objectCameras;
#if MAX_SHADOWMAPS < 0
		CStaticArray<tShadowMapInfo, -MAX_SHADOWMAPS> m_shadowMaps;
#elif MAX_SHADOWMAPS > 0
		CStaticArray<tShadowMapInfo, MAX_SHADOWMAPS> m_shadowMaps;
#endif

	public:
		int				m_fboType;

	public:
		CCameraManager () : m_current (NULL), m_objectCameras (0), m_fboType (1) {}
		~CCameraManager ();
		int Create ();
		void Destroy ();
		int Render ();
		void Rotate (CObject *objP);
		inline uint Count (void) { return m_cameras.Buffer () ? m_cameras.ToS () : 0; }
		inline CCamera* Cameras (void) { return m_cameras.Buffer (); }
		//inline CCamera* Camera (short i = 0) { return Cameras () + i; }
		inline int Current (void) { return int (m_current - m_cameras.Buffer ()); }
		inline void SetCurrent (CCamera* cameraP) { m_current = cameraP; }
		CCamera* Camera (CObject *objP);
		inline int GetObjectCamera (int nObject);
		inline void SetObjectCamera (int nObject, int i);
		int GetFaceCamera (int nFace);
		void ReAlign (void);
		inline void SetFaceCamera (int nFace, int i);
		inline int Index (CCamera* cameraP) { return m_cameras.Buffer () ? int (cameraP - m_cameras.Buffer ()) : -1; }
		inline CCamera* Add (void) { return ((m_cameras.Buffer () || Create ()) && m_cameras.Grow ()) ? m_cameras.Top () : NULL; }
		inline CCamera* operator[] (uint i) { return (i < m_cameras.ToS ()) ? &m_cameras [i] : NULL; }

#if MAX_SHADOWMAPS
		inline CCamera* ShadowMap (int i) { return (m_shadowMaps [i].nCamera < 0) ? NULL : (*this) [i]; }
		inline CDynLight* ShadowLightSource (int i) { return (m_shadowMaps [i].nCamera < 0) ? NULL : m_shadowMaps [i].lightP; }
		inline CCamera* AddShadowMap (int i, CDynLight* lightP) { 
			CCamera* cameraP = Add ();
			if (!cameraP)
				return NULL;
			m_shadowMaps [i].nCamera = Index (cameraP);
			m_shadowMaps [i].lightP = lightP;
			return cameraP;
			}
		inline void DestroyShadowMap (int i) {
			CCamera* cameraP = ShadowMap (i);
			if (cameraP) {
				cameraP->Destroy ();
				m_shadowMaps [i].nCamera = -1;
				m_shadowMaps [i].lightP = NULL;
				}
			}
#endif
};

extern CCameraManager cameraManager;

//------------------------------------------------------------------------------

#define USE_CAMERAS (extraGameInfo [0].bUseCameras && (!IsMultiGame || (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bUseCameras)))

//------------------------------------------------------------------------------

#endif // _cameras_h
