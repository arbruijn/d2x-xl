#ifndef _cameras_h
#define _cameras_h

#include "ogl_defs.h"
#include "ogl_texture.h"

#define MAX_CAMERAS	100

typedef struct tCamera {
	CObject			obj;
	CObject			*objP;
	short				nId;
	short				nSegment;
	short				nSide;
	CBitmap			buffer;
	ubyte				*screenBuf;
	GLuint			glTexId;
	time_t			nTimeout;
	char				nType;
	char				bValid;
	char				bVisible;
	char				bTimedOut;
	char				bHaveUVL;
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

class CCamera {
	private:
		tCamera	m_info;
	public:
		CCamera () { Init (); };
		~CCamera () { Destroy (); };
		void Init (void);
		int Render (void);
		int Ready (time_t t);
		void Reset (void);
		void Rotate (void);
		void GetUVL (CSegFace *faceP, tUVL *uvlP, tTexCoord2f *texCoordP, CFloatVector3 *vertexP);
		int Create (short nId, short srcSeg, short srcSide, short tgtSeg, short tgtSide, 
						CObject *objP, int bShadowMap, int bTeleport);
		void Destroy (void);
		int HaveBuffer (int bCheckTexture);
		int HaveTexture (void);
		inline void SetVisible (char bVisible) { m_info.bVisible = bVisible; }
		inline char GetVisible (void) { return m_info.bVisible; }
		inline char GetTeleport (void) { return m_info.bTeleport; }
		inline char Valid (void) { return m_info.bValid; }
		inline CFixMatrix& Orient (void) { return m_info.orient; }
		inline CBitmap& Texture (void) { return m_info.buffer; }
		inline tTexCoord2f* TexCoord (void) { return m_info.texCoord; }
		inline CObject* GetObject (void) { return m_info.objP; }
		inline CFBO& FrameBuffer (void) { return m_info.fbo; } 

	private:
		int CreateBuffer (void);
		int EnableBuffer (void);
		int DisableBuffer (void);
		int BindBuffer (void);
		int ReleaseBuffer (void);
		int DestroyBuffer (void);
	};

//------------------------------------------------------------------------------

class CCameraManager {
	private:
		CCamera			m_cameras [MAX_CAMERAS];
		short				m_nCameras;
		CArray<char>	m_faceCameras;
		CArray<ushort>	m_objectCameras;

	public:
		CCameraManager () { m_objectCameras = 0, m_nCameras = 0; };
		~CCameraManager ();
		int Create ();
		void Destroy ();
		int Render ();
		void Rotate (CObject *objP);
		inline CCamera* Cameras (void) { return m_cameras; }
		inline CCamera* Camera ( short i = 0 ) { return Cameras () + i; }
		CCamera* Camera (CObject *objP);
		inline int GetObjectCamera (int nObject);
		inline void SetObjectCamera (int nObject, int i);
		int GetFaceCamera (int nFace);
		inline void SetFaceCamera (int nFace, int i);
	};

extern CCameraManager cameraManager;

//------------------------------------------------------------------------------

#define USE_CAMERAS (extraGameInfo [0].bUseCameras && (!IsMultiGame || (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bUseCameras)))

//------------------------------------------------------------------------------

#endif // _cameras_h
