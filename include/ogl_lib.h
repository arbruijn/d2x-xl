//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_LIB_H
#define _OGL_LIB_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "descent.h"
#include "vecmat.h"
#include "sdlgl.h"
#include "canvas.h"

//------------------------------------------------------------------------------

extern GLuint hBigSphere;
extern GLuint hSmallSphere;
extern GLuint circleh5;
extern GLuint circleh10;
extern GLuint mouseIndList;
extern GLuint cross_lh [2];
extern GLuint primary_lh [3];
extern GLuint secondary_lh [5];
extern GLuint g3InitTMU [4][2];
extern GLuint g3ExitTMU [2];

extern int32_t r_polyc, r_tpolyc, r_bitmapc, r_ubitmapc, r_ubitbltc, r_upixelc, r_tvertexc, r_texcount;
extern int32_t gr_renderstats, gr_badtexture;

extern int32_t nScreenDists [10];

//------------------------------------------------------------------------------

typedef struct tRenderQuality {
	int32_t	texMinFilter;
	int32_t	texMagFilter;
	int32_t	bNeedMipmap;
	int32_t	bAntiAliasing;
} tRenderQuality;

extern tRenderQuality renderQualities [];

typedef struct tSinCosf {
	float	fSin, fCos;
} tSinCosf;

//------------------------------------------------------------------------------

void OglDeleteLists (GLuint *lp, int32_t n);
void ComputeSinCosTable (int32_t nSides, tSinCosf *pSinCos);
int32_t CircleListInit (int32_t nSides, int32_t nType, int32_t mode);
void G3Normal (CRenderPoint** pointList, CFixVector* pvNormal);
void G3CalcNormal (CRenderPoint **pointList, CFloatVector *pvNormal);

//------------------------------------------------------------------------------

#define OGL_BLEND_ALPHA					0
#define OGL_BLEND_ADD					1
#define OGL_BLEND_ADD_WEAK				2
#define OGL_BLEND_REPLACE				3
#define OGL_BLEND_MULTIPLY				4
#define OGL_BLEND_ALPHA_CONTROLLED	5

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#define OGLTEXBUFSIZE (4096 * 4096 * 4)

typedef float vec2 [2];
typedef float vec3 [3];
typedef float vec4 [4];

typedef union tWindowScale {
	struct {
		float x, y;
		} dim;
	vec2 vec;
} tWindowScale;

#if DBG_OGL

typedef struct tClientBuffer {
	const GLvoid*	buffer;
	const char*		pszFile;
	int32_t			nLine;
} tClientBuffer;

#endif

class COglData {
	public:
		GLubyte			buffer [2][OGLTEXBUFSIZE];
		uint8_t			outlineFilter [OGLTEXBUFSIZE / 4];
		CPalette*		palette;
		float				zNear;
		float				zFar;
		CFloatVector3	depthScale;
		tWindowScale	windowScale;
		CStaticArray<CFBO, 9> drawBuffers;
		CFBO*				pDrawBuffer;
		int32_t			nPerPixelLights [9];
		float				lightRads [8];
		CFloatVector	lightPos [8];
		int32_t			bLightmaps;
		int32_t			nHeadlights;
		fix				xStereoSeparation;
		GLuint			nTexture [4];
		GLenum			nSrcBlendMode;
		GLenum			nDestBlendMode;
		GLenum			nDepthMode;
		GLenum			nCullMode;
		GLfloat			lineWidthRange [2];
		GLuint			nDepthFunc;
		bool				bUseTextures [4];
		bool				bAlphaTest;
		bool				bScissorTest;
		bool				bDepthTest;
		bool				bDepthWrite;
		bool				bUseBlending;
		bool				bStencilTest;
		bool				bCullFaces;
		bool				bLineSmooth;
		bool				bLighting;
		bool				bPolyOffsetFill;
		int32_t			nTMU [2];	//active driver and client TMUs
		int32_t			clientStates [4][6];	// client states for the first 4 TMUs
		int32_t			bClientTexCoord;
		int32_t			bClientColor;
#if DBG_OGL
		tClientBuffer	clientBuffers [4][6];
#endif

	public:
		COglData () { Initialize (); }
		void Initialize (void);
		inline CFBO* GetDrawBuffer (int32_t nBuffer) { return &drawBuffers [nBuffer]; }
		inline CFBO* GetGlowBuffer (int32_t nBuffer) { return &drawBuffers [nBuffer + 2]; }
		inline CFBO* GetBlurBuffer (int32_t nBuffer) { return &drawBuffers [nBuffer + 3]; }
		inline CFBO* GetDepthBuffer (int32_t nBuffer) { return &drawBuffers [nBuffer + 5]; }
};

//-----------------------------------------------------------------------------

class COglFeature {
	private:
		int32_t	m_bAvailable;
		int32_t	m_bApply;

	public:
		inline int32_t Available (const int32_t i) { return (m_bAvailable = i); }
		inline int32_t Available (void) { return m_bAvailable; }
		inline int32_t Apply (void) { return m_bAvailable && m_bApply; }
		inline const COglFeature& operator = (const int32_t i) { 
			m_bApply = i; 
			return *this;
			}
		operator int32_t() { return (m_bAvailable > 0) * m_bApply; }
		inline bool operator == (const int32_t i) { return ((m_bAvailable > 0) * m_bApply) == i; }
		inline bool operator != (const int32_t i) { return ((m_bAvailable > 0) * m_bApply) != i; }
		inline bool operator > (const int32_t i) { return ((m_bAvailable > 0) * m_bApply) > i; }
		inline bool operator < (const int32_t i) { return ((m_bAvailable > 0) * m_bApply) < i; }
		//operator bool() { return bool (Available () && Apply ()); }
		COglFeature () : m_bAvailable (1), m_bApply (0) {}
	};


class COglFeatures {
	public:
		COglFeature	bAntiAliasing;
		COglFeature bDepthBlending;
		COglFeature	bGlowRendering;
		COglFeature	bMultipleRenderTargets;
		COglFeature	bMultiTexturing;
		COglFeature	bOcclusionQuery;
		COglFeature	bPerPixelLighting;
		COglFeature	bRenderToTexture;
		COglFeature	bSeparateStencilOps;
		COglFeature	bShaders;
		COglFeature	bStencilBuffer;
		COglFeature	bStereoBuffers;
		COglFeature	bQuadBuffers;
		COglFeature	bTextureArrays;
		COglFeature	bTextureCompression;
		COglFeature	bVertexBufferObjects;
	};

class COglStates {
	public:
		int32_t	bInitialized;
		int32_t	bRebuilding;
		int32_t	bFullScreen;
		int32_t	bLastFullScreen;
		int32_t	bUseTransform;
		int32_t	nTransformCalls;
		int32_t	bBrightness;
		int32_t	bLowMemory;
		int32_t	nColorBits;
		int32_t	nPreloadTextures;
		uint8_t	nTransparencyLimit;
		GLint	nDepthBits;
		GLint	nStencilBits;
		GLint	nStereo;
		int32_t	bEnableTexture2D;
		int32_t	bEnableTexClamp;
		int32_t	bEnableScissor;
		int32_t	bNeedMipMaps;
		int32_t	bFSAA;
		int32_t	texMinFilter;
		int32_t	texMagFilter;
		int32_t	nTexMagFilterState;
		int32_t	nTexMinFilterState;
		int32_t	nTexEnvModeState;
		int32_t	nContrast;
		CViewport viewport [2];
		int32_t	nCurWidth;
		int32_t	nCurHeight;
		int32_t	nLights;
		int32_t	iLight;
		int32_t	nFirstLight;
		int32_t	bCurFullScreen;
		int32_t	bpp;
		int32_t	bScaleLight;
		int32_t	bDynObjLight;
		int32_t	bVertexLighting;
		int32_t	bHeadlight;
		int32_t	bStandardContrast;
		int32_t	nRGBAFormat;
		int32_t	nRGBFormat;
		int32_t	bIntensity4;
		int32_t	bLuminance4Alpha4;
		int32_t	bGlowRendering;
		int32_t	bHaveDepthBuffer [2];
		int32_t	bHaveColorBuffer;
		int32_t	nDrawBuffer;
		int32_t	nStencil;
		int32_t	nCamera;
	#ifdef GL_ARB_multitexture
		int32_t	bArbMultiTexture;
	#endif
	#ifdef GL_SGIS_multitexture
		int32_t	bSgisMultiTexture;
	#endif
		float	fAlpha;
		float	fLightRange;
		int32_t	bDepthBuffer [2];
		int32_t	bColorBuffer;
		GLuint hDepthBuffer [2];
		GLuint hColorBuffer;

	public:
		COglStates () { Initialize (); }
		void Initialize (void);
};


class COglBuffers {
	public:
		CArray<CFloatVector>		vertices;
		CArray<tTexCoord2f>		texCoord [2];
		CArray<CFloatVector>		color;
		CUShortArray				indices;
		int32_t						m_nVertices;

	public:
		COglBuffers () { 
			m_nVertices = 0; 
			SizeBuffers (1000);
			}
		bool SizeVertices (int32_t nVerts);
		bool SizeColor (int32_t nVerts);
		bool SizeTexCoord (int32_t nVerts);
		bool SizeIndex (int32_t nVerts);
		bool SizeBuffers (int32_t nVerts);
		void Flush (GLenum nPrimitive, int32_t nVerts, int32_t nDimension = 3, int32_t bTextured = 0, int32_t bColored = 0);

	};

class COGL {
	public:
		COglData			m_data;
		COglStates		m_states;
		COglBuffers		m_buffers;
		COglFeatures	m_features;

	public:
		COGL () { Initialize (); }
		void Initialize (void);
		inline void ResetStates (void) { m_data.Initialize (); }

		void SetupExtensions (void);
		void SetupMultiTexturing (void);
		void SetupShaders (void);
		void SetupOcclusionQuery (void);
		void SetupPointSprites (void);
		void SetupTextureCompression (void);
		void SetupStencilOps (void);
		void SetupRefreshSync (void);
		void SetupAntiAliasing (void);
		void SetupMRT (void);
		void SetupVBOs (void);
		void SetupTextureArrays (void);

		void InitState (void);
		void InitShaders (void);

		void SetupProjection (CTransformation& transformation);
		void SetupFrustum (fix xStereoSeparation);
		void SelectTMU (int32_t nTMU, bool bClient = true);
		int32_t EnableClientState (GLuint nState, int32_t nTMU = -1);
		int32_t DisableClientState (GLuint nState, int32_t nTMU = -1);
		int32_t EnableClientStates (int32_t bTexCoord, int32_t bColor, int32_t bNormals, int32_t nTMU = -1);
		void DisableClientStates (int32_t bTexCoord, int32_t bColor, int32_t bNormals, int32_t nTMU = -1);
		void ResetClientStates (int32_t nFirst = 0);
		void StartFrame (int32_t bFlat, int32_t bResetColorBuf, fix xStereoSeparation);
		void EndFrame (int32_t nWindow);
		void EnableLighting (int32_t bSpecular);
		void DisableLighting (void);
		void SetRenderQuality (int32_t nQuality = -1);
		CViewport& Viewport (void) { return m_states.viewport [0]; }
		void SetViewport (int32_t x, int32_t y, int32_t w, int32_t h);
		void GetViewport (vec4& viewport);
		void SaveViewport (void);
		void RestoreViewport (void);
		void SwapBuffers (int32_t bForce, int32_t bClear);
		void Update (int32_t bClear);
		void SetupTransform (int32_t bForce);
		void ResetTransform (int32_t bForce);
		void SetScreenMode (void);
		void GetVerInfo (void);
		GLuint CreateDepthTexture (int32_t nTMU, int32_t bFBO, int32_t nType = 0, int32_t nWidth = -1, int32_t hHeight = -1);
		void DestroyDepthTexture (int32_t bFBO);
		GLuint CopyDepthTexture (int32_t nId, int32_t nTMU = GL_TEXTURE1);
		GLuint CreateColorTexture (int32_t nTMU, int32_t bFBO);
		void DestroyColorTexture (void);
		GLuint CopyColorTexture (void);
#if 0
		GLuint CreateStencilTexture (int32_t nTMU, int32_t bFBO);
#endif
		void CreateDrawBuffer (int32_t nType = 1);
		void DestroyDrawBuffer (void);
		void DestroyDrawBuffers (void);
		void SetDrawBuffer (int32_t nBuffer, int32_t bFBO);
		void SetReadBuffer (int32_t nBuffer, int32_t bFBO);
		int32_t DrawBufferWidth (void);
		int32_t DrawBufferHeight (void);
		void MergeAnaglyphBuffers (void);
		void FlushOculusRiftBuffers (void);
		void FlushSideBySideBuffers (void);
		void FlushNVidiaStereoBuffers (void);
		void FlushAnaglyphBuffers (void);
		void FlushMonoFrameBuffer (void);
		void FlushEffectsSideBySide (void);
#if MAX_SHADOWMAPS
		void FlushShadowMaps (int32_t nEffects);
#endif
		void FlushDrawBuffer (bool bAdditive = false);
		void ChooseDrawBuffer (void);

		int32_t BindBuffers (CFloatVector *pVertex, int32_t nVertices, int32_t nDimensions,
							  tTexCoord2f *pTexCoord, 
							  CFloatVector *pColor, int32_t nColors,
							  CBitmap *pBm, 							  
							  int32_t nTMU = -1);
		void ReleaseBuffers (void);
		int32_t BindBitmap (CBitmap* pBm, int32_t nFrame, int32_t nWrap, int32_t bTextured);
		int32_t RenderArrays (int32_t nPrimitive, 
								CFloatVector *pVertex, int32_t nVertices, int32_t nDimensions = 3,
								tTexCoord2f *pTexCoord = NULL, 
								CFloatVector *pColor = NULL, int32_t nColors = 1, 
								CBitmap *pBm = NULL, int32_t nFrame = 0, int32_t nWrap = GL_REPEAT);
		int32_t RenderQuad (CBitmap* pBm, CFloatVector* pVertex, int32_t nDimensions = 3, tTexCoord2f* pTexCoord = NULL, CFloatVector* pColor = NULL, int32_t nColors = 1, int32_t nWrap = GL_CLAMP);
		int32_t RenderQuad (CBitmap* pBm, CFloatVector& vPosf, float width, float height, int32_t nDimensions = 3, int32_t nWrap = GL_CLAMP);
		int32_t RenderBitmap (CBitmap* pBm, const CFixVector& vPos, fix xWidth, fix xHeight, CFloatVector* pColor, float alpha, int32_t bAdditive);
		int32_t RenderSprite (CBitmap* pBm, const CFixVector& vPos, fix xWidth, fix xHeight, float alpha, int32_t bAdditive, float fSoftRad);
		void RenderScreenQuad (GLuint nTexture = 0);

		inline int32_t FullScreen (void) { return m_states.bFullScreen; }

		inline void SetFullScreen (int32_t bFullScreen) {
#if !DBG
			if (bFullScreen || !IsSideBySideDevice ()) {
#endif
				m_states.bFullScreen = bFullScreen;
				if (m_states.bInitialized)
					SdlGlDoFullScreenInternal (0);
#if !DBG
				}
#endif
			}

		int32_t ToggleFullScreen (void) {
			SetFullScreen (!m_states.bFullScreen);
			return m_states.bFullScreen;
			}


		inline bool Enable (bool& bCurrent, bool bNew, GLenum nFunc) {
#if DBG_OGL < 2
			if (bCurrent != bNew) 
#endif
				{
				if ((bCurrent = bNew))
					glEnable (nFunc);
				else
					glDisable (nFunc);
#if DBG_OGL
				bCurrent = glIsEnabled (nFunc) != 0;
#endif
				}
			return bCurrent;
			}

		inline COglBuffers& Buffers (void) { return m_buffers; }
		inline CArray<CFloatVector>& VertexBuffer (void) { return Buffers ().vertices; }
		inline CArray<tTexCoord2f>& TexCoordBuffer (int32_t i = 0) { return Buffers ().texCoord [i]; }
		inline CArray<CFloatVector>& ColorBuffer (void) { return Buffers ().color; }
		inline bool SizeBuffers (int32_t nVerts) { return m_buffers.SizeBuffers (nVerts); }
		inline bool SizeVertexBuffer (int32_t nVerts) { return m_buffers.SizeVertices (nVerts); }
		inline void FlushBuffers (GLenum nPrimitive, int32_t nVerts, int32_t nDimensions = 3, int32_t bTextured = 0, int32_t bColored = 0) {
			m_buffers.Flush (nPrimitive, nVerts, nDimensions, bTextured, bColored); 
			}

		inline bool SetFaceCulling (bool bCullFaces) { return Enable (m_data.bCullFaces, bCullFaces, GL_CULL_FACE); }
		inline bool GetFaceCulling (void) { return m_data.bCullFaces; }

		inline bool SetDepthTest (bool bDepthTest) { return Enable (m_data.bDepthTest, bDepthTest, GL_DEPTH_TEST); }
		inline bool GetDepthTest (void) { return m_data.bDepthTest; }

		inline bool SetLighting (bool bLighting) { return Enable (m_data.bLighting, bLighting, GL_LIGHTING); }
		inline bool GetLighting (void) { return m_data.bLighting; }

		inline bool SetPolyOffsetFill (bool bPolyOffsetFill) { return Enable (m_data.bPolyOffsetFill, bPolyOffsetFill, GL_POLYGON_OFFSET_FILL); }
		inline bool GetPolyOffsetFill (void) { return m_data.bPolyOffsetFill; }

		inline bool SetAlphaTest (bool bAlphaTest) { return Enable (m_data.bAlphaTest, bAlphaTest, GL_ALPHA_TEST); }
		inline bool GetAlphaTest (void) { return m_data.bAlphaTest; }

		inline bool SetScissorTest (bool bScissorTest) { return Enable (m_data.bScissorTest, bScissorTest, GL_SCISSOR_TEST); }
		inline bool GetScissorTest (void) { return m_data.bScissorTest; }

		inline bool SetStencilTest (bool bStencilTest) { return Enable (m_data.bStencilTest, bStencilTest, GL_STENCIL_TEST); }
		inline bool getStencilTest (void) { return m_data.bStencilTest; }

		inline bool SetBlending (bool bUseBlending) { return Enable (m_data.bUseBlending, bUseBlending, GL_BLEND); }
		inline bool GetBlendUsage (void) { return m_data.bUseBlending; }
		
		inline bool SetTexturing (bool bUseTextures) { 
#if 0
			if (!bUseTextures)
				m_data.nTexture [m_data.nTMU [0]] = 0;
#endif
			return Enable (m_data.bUseTextures [m_data.nTMU [0]], bUseTextures, GL_TEXTURE_2D); 
			}
		inline bool GetTextureUsage (void) { return m_data.bUseTextures [m_data.nTMU [0]]; }
		
		inline bool SetLineSmooth (bool bLineSmooth) { 
			if ((bLineSmooth = Enable (m_data.bLineSmooth, bLineSmooth, GL_LINE_SMOOTH)))
				glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
			return bLineSmooth;
			}
		inline bool GetLineSmoothe (void) { return m_data.bLineSmooth; }
		
		inline bool SetBlendMode (GLenum nSrcBlendMode, GLenum nDestBlendMode) {
			SetBlending (true);
#if DBG_OGL < 2
			if ((m_data.nSrcBlendMode == nSrcBlendMode) && (m_data.nDestBlendMode == nDestBlendMode))
				return true;
#endif
			glBlendFunc (m_data.nSrcBlendMode = nSrcBlendMode, m_data.nDestBlendMode = nDestBlendMode);
#if DBG_OGL < 2
			return true;
#else
			glGetIntegerv (GL_BLEND_SRC, (GLint*) &m_data.nSrcBlendMode);
			glGetIntegerv (GL_BLEND_DST, (GLint*) &m_data.nDestBlendMode);
			return (m_data.nSrcBlendMode == nSrcBlendMode) && (m_data.nDestBlendMode == nDestBlendMode);
#endif
			}

		inline void GetBlendMode (GLenum& nSrcBlendMode, GLenum& nDestBlendMode) {
			nSrcBlendMode = m_data.nSrcBlendMode;
			nDestBlendMode = m_data.nDestBlendMode;
			}

		inline void SetBlendMode (int32_t nBlendMode) {
			switch (nBlendMode) {
				case OGL_BLEND_REPLACE:
					SetBlendMode (GL_ONE, GL_ZERO); // replace 
					break;
				case OGL_BLEND_ADD:
					SetBlendMode (GL_ONE, GL_ONE); // additive
					break;
				case OGL_BLEND_ADD_WEAK:
					SetBlendMode (GL_ONE, GL_ONE_MINUS_SRC_COLOR); // weak additive
					break;
				case OGL_BLEND_MULTIPLY: //-1
					SetBlendMode (GL_DST_COLOR, GL_ZERO); // multiplicative
					break;
				case OGL_BLEND_ALPHA_CONTROLLED:
					SetBlendMode (GL_ONE, GL_SRC_ALPHA);
					break;
				case OGL_BLEND_ALPHA:
				default:
					SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // alpha
					break;
				}
			}


		inline GLenum SetDepthMode (GLenum nDepthMode) {
#if DBG_OGL < 2
			if (m_data.nDepthMode != nDepthMode)
#endif
				glDepthFunc (m_data.nDepthMode = nDepthMode);
			return m_data.nDepthMode;
			}

		inline GLenum GetDepthMode (void) { return m_data.nDepthMode; }

		inline void SetDepthWrite (bool bDepthWrite) { 
#if DBG_OGL < 2
			if (m_data.bDepthWrite != bDepthWrite)
#endif
				glDepthMask (GLboolean (m_data.bDepthWrite = bDepthWrite));
			}

		inline bool GetDepthWrite (void) { return m_data.bDepthWrite; }

		inline GLenum SetCullMode (GLenum nCullMode) {
#if DBG_OGL < 2
			if (m_data.nCullMode != nCullMode)
#endif
				glCullFace (m_data.nCullMode = nCullMode);
			return m_data.nCullMode;
			}

		int32_t SelectDrawBuffer (int32_t nBuffer, int32_t nColorBuffers = 0);

		int32_t SelectGlowBuffer (void);

		int32_t SelectBlurBuffer (int32_t nBuffer);

		inline CFBO* DrawBuffer (int32_t nBuffer = -1) { return (nBuffer < 0) ? m_data.pDrawBuffer : m_data.GetDrawBuffer (nBuffer); }

		inline CFBO* GlowBuffer () { return m_data.GetDrawBuffer (2); }

		CFBO* BlurBuffer (int32_t nBuffer);

		inline CFBO* DepthBuffer (int32_t nBuffer = -1) { return (nBuffer < 0) ? m_data.pDrawBuffer : m_data.GetDepthBuffer (nBuffer + 5); }

		void RebuildContext (int32_t bGame);
		void DrawArrays (GLenum mode, GLint first, GLsizei count);
		void ColorMask (GLboolean bRed, GLboolean bGreen, GLboolean bBlue, GLboolean bAlpha, GLboolean bEyeOffset = GL_TRUE);
		int32_t StereoDevice (void);

		inline int32_t IsAnaglyphDevice (int32_t nDevice = 0x7fffffff) { 
			if (nDevice == 0x7fffffff)
				nDevice = StereoDevice ();
			return nDevice > 0; 
			}

		inline int32_t IsOculusRift (int32_t nDevice = 0x7fffffff) {
			if (nDevice == 0x7fffffff)
				nDevice = StereoDevice ();
			return nDevice == -GLASSES_OCULUS_RIFT;
			}

		inline int32_t IsSideBySideDevice (int32_t nDevice = 0x7fffffff) {
			if (nDevice == 0x7fffffff)
				nDevice = StereoDevice ();
			return (nDevice <= -DEVICE_STEREO_SIDEBYSIDE) && (nDevice > -DEVICE_STEREO_DOUBLE_BUFFER);
			}

		inline void BindTexture (GLuint handle) { 
#if 0 //DBG_OGL < 2
			if (m_data.nTexture [m_data.nTMU [0]] != handle)
#endif
				glBindTexture (GL_TEXTURE_2D, m_data.nTexture [m_data.nTMU [0]] = handle); 
			}

		inline GLuint BoundTexture (void) { return m_data.nTexture [m_data.nTMU [0]]; }

		inline void ReleaseTexture (GLuint handle) { 
			for (int32_t i = 0; i < 4; i++)
				if (m_data.nTexture [i] == handle) {
					glBindTexture (GL_TEXTURE_2D, m_data.nTexture [i] = 0); 
					break;
					}
			}

		inline int32_t IsBound (GLuint handle) { 
			for (int32_t i = 0; i < 4; i++)
				if (m_data.nTexture [i] == handle)
					return i;
			return -1;
			}

#if DBG_OGL
		void VertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int32_t nLine);
		void ColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int32_t nLine);
		void TexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int32_t nLine);
		void NormalPointer (GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int32_t nLine);

		void GenTextures (GLsizei n, GLuint *hTextures);
		void DeleteTextures (GLsizei n, GLuint *hTextures);
	
#else
		inline void VertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int32_t nLine)
			{ glVertexPointer (size, type, stride, pointer); }
		inline void ColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int32_t nLine)
			{ glColorPointer (size, type, stride, pointer); }
		inline void TexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int32_t nLine)
			{ glTexCoordPointer (size, type, stride, pointer); }
		inline void NormalPointer (GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int32_t nLine)
			{ glNormalPointer (type, stride, pointer); }

		inline void GenTextures (GLsizei n, GLuint *hTextures) { glGenTextures (n, hTextures); }
		inline void DeleteTextures (GLsizei n, GLuint *hTextures) { glDeleteTextures (n, hTextures); }
#endif

		GLenum ClearError (int32_t bTrapError);

		inline int32_t SetTransform (int32_t bUseTransform) { return m_states.bUseTransform = bUseTransform; }
		inline int32_t UseTransform (void) { return m_states.bUseTransform; }
		inline void SetStereoSeparation (fix xStereoSeparation) { m_data.xStereoSeparation = xStereoSeparation; }
		inline fix StereoSeparation (void) { return m_data.xStereoSeparation; }

		inline int32_t HaveDrawBuffer (void) {
			return m_features.bRenderToTexture.Available () && m_data.pDrawBuffer->Handle () && m_data.pDrawBuffer->Active ();
			}

		int32_t StencilOff (void);
		void StencilOn (int32_t bStencil);
		
		double ZScreen (void);
		
		void InitEnhanced3DShader (void);
		void DeleteEnhanced3DShader (void);
		void InitShadowMapShader (void);
		void DeleteShadowMapShader (void);

#if DBG_OGL
	private:
		int32_t m_bLocked;

	public:
		inline int32_t Lock (void) { return m_bLocked ? 0 : ++m_bLocked; }
		inline int32_t Unlock (void) { return m_bLocked ? --m_bLocked : 0; }
		inline bool Locked (void) { return m_bLocked != 0; }
#endif
};

extern COGL ogl;

//------------------------------------------------------------------------------

static inline void OglCullFace (int32_t bFront)
{
#if 0
if (gameStates.render.bRearView)
	ogl.SetCullMode (bFront ? GL_BACK : GL_FRONT);
else
#endif
	ogl.SetCullMode (bFront ? GL_FRONT : GL_BACK);
}

//------------------------------------------------------------------------------

#define ZNEAR		1.0f
#define ZFAR		ogl.m_data.zFar
#define ZRANGE		(ZFAR - ZNEAR)

//------------------------------------------------------------------------------

#endif
