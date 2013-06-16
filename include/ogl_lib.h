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

extern int r_polyc, r_tpolyc, r_bitmapc, r_ubitmapc, r_ubitbltc, r_upixelc, r_tvertexc, r_texcount;
extern int gr_renderstats, gr_badtexture;

extern int nScreenDists [10];

//------------------------------------------------------------------------------

typedef struct tRenderQuality {
	int	texMinFilter;
	int	texMagFilter;
	int	bNeedMipmap;
	int	bAntiAliasing;
} tRenderQuality;

extern tRenderQuality renderQualities [];

typedef struct tSinCosf {
	float	fSin, fCos;
} tSinCosf;

//------------------------------------------------------------------------------

void OglDeleteLists (GLuint *lp, int n);
void ComputeSinCosTable (int nSides, tSinCosf *sinCosP);
int CircleListInit (int nSides, int nType, int mode);
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
	int				nLine;
} tClientBuffer;

#endif

class COglData {
	public:
		GLubyte			buffer [OGLTEXBUFSIZE];
		CPalette*		palette;
		float				zNear;
		float				zFar;
		CFloatVector3	depthScale;
		tWindowScale	windowScale;
		CStaticArray<CFBO, 9> drawBuffers;
		CFBO*				drawBufferP;
		int				nPerPixelLights [9];
		float				lightRads [8];
		CFloatVector	lightPos [8];
		int				bLightmaps;
		int				nHeadlights;
		fix				xStereoSeparation;
		GLuint			nTexture [4];
		GLenum			nSrcBlendMode;
		GLenum			nDestBlendMode;
		GLenum			nDepthMode;
		GLenum			nCullMode;
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
		int				nTMU [2];	//active driver and client TMUs
		int				clientStates [4][6];	// client states for the first 4 TMUs
		int				bClientTexCoord;
		int				bClientColor;
#if DBG_OGL
		tClientBuffer	clientBuffers [4][6];
#endif

	public:
		COglData () { Initialize (); }
		void Initialize (void);
		inline CFBO* GetDrawBuffer (int nBuffer) { return &drawBuffers [nBuffer]; }
		inline CFBO* GetGlowBuffer (int nBuffer) { return &drawBuffers [nBuffer + 2]; }
		inline CFBO* GetBlurBuffer (int nBuffer) { return &drawBuffers [nBuffer + 3]; }
		inline CFBO* GetDepthBuffer (int nBuffer) { return &drawBuffers [nBuffer + 5]; }
};

class CViewport {
	public:
		int m_x, m_y, m_w, m_h, m_t;

		CViewport (int x = 0, int y = 0, int w = 0, int h = 0, int t = 0) : m_x (x), m_y (y), m_w (w), m_h (h), m_t (t) {}

		void Apply (int t = -1) { 
			if (t >= 0)
				m_t = t;
			glViewport ((GLint) m_x, (GLint) (m_t - m_y - m_h), (GLsizei) m_w, (GLsizei) m_h); 
			}

		inline const CViewport& operator= (CViewport const other) {
			m_x = other.m_x;
			m_y = other.m_y;
			m_w = other.m_w;
			m_h = other.m_h;
			m_t = other.m_t;
			return *this;
			}

		inline const bool operator!= (CViewport const other) {
			return (m_x != other.m_x) || (m_y != other.m_y) || (m_w != other.m_w) || (m_h != other.m_h) || (m_t != other.m_t);
			}

		inline int Left (void) { return m_x; }
		inline int Top (void) { return m_y; }
		inline int Width (void) { return m_w - m_x; }
		inline int Height (void) { return m_h - m_y; }
	};


class COglFeature {
	private:
		int	m_bAvailable;
		int	m_bApply;

	public:
		inline int Available (const int i) { return (m_bAvailable = i); }
		inline int Available (void) { return m_bAvailable; }
		inline int Apply (void) { return m_bAvailable && m_bApply; }
		inline const COglFeature& operator = (const int i) { 
			m_bApply = i; 
			return *this;
			}
		operator int() { return (m_bAvailable > 0) * m_bApply; }
		inline bool operator == (const int i) { return ((m_bAvailable > 0) * m_bApply) == i; }
		inline bool operator != (const int i) { return ((m_bAvailable > 0) * m_bApply) != i; }
		inline bool operator > (const int i) { return ((m_bAvailable > 0) * m_bApply) > i; }
		inline bool operator < (const int i) { return ((m_bAvailable > 0) * m_bApply) < i; }
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
		int	bInitialized;
		int	bRebuilding;
		int	bFullScreen;
		int	bLastFullScreen;
		int	bUseTransform;
		int	nTransformCalls;
		int	bBrightness;
		int	bLowMemory;
		int	nColorBits;
		int	nPreloadTextures;
		ubyte	nTransparencyLimit;
		GLint	nDepthBits;
		GLint	nStencilBits;
		GLint	nStereo;
		int	bEnableTexture2D;
		int	bEnableTexClamp;
		int	bEnableScissor;
		int	bNeedMipMaps;
		int	bFSAA;
		int	texMinFilter;
		int	texMagFilter;
		int	nTexMagFilterState;
		int	nTexMinFilterState;
		int	nTexEnvModeState;
		int	nContrast;
		CViewport viewport [2];
		int	nCurWidth;
		int	nCurHeight;
		int	nLights;
		int	iLight;
		int	nFirstLight;
		int	bCurFullScreen;
		int	bpp;
		int	bScaleLight;
		int	bDynObjLight;
		int	bVertexLighting;
		int	bHeadlight;
		int	bStandardContrast;
		int	nRGBAFormat;
		int	nRGBFormat;
		int	bIntensity4;
		int	bLuminance4Alpha4;
		int	bGlowRendering;
		int	bHaveDepthBuffer [2];
		int	bHaveColorBuffer;
		int	nDrawBuffer;
		int	nStencil;
		int	nCamera;
	#ifdef GL_ARB_multitexture
		int	bArbMultiTexture;
	#endif
	#ifdef GL_SGIS_multitexture
		int	bSgisMultiTexture;
	#endif
		float	fAlpha;
		float	fLightRange;
		int	bDepthBuffer [2];
		int	bColorBuffer;
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
		int							m_nVertices;

	public:
		COglBuffers () { 
			m_nVertices = 0; 
			SizeBuffers (1000);
			}
		bool SizeVertices (int nVerts);
		bool SizeColor (int nVerts);
		bool SizeTexCoord (int nVerts);
		bool SizeIndex (int nVerts);
		bool SizeBuffers (int nVerts);
		void Flush (GLenum nPrimitive, int nVerts, int nDimension = 3, int bTextured = 0, int bColored = 0);

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
		void SetupFrustum (void);
		void SelectTMU (int nTMU, bool bClient = true);
		int EnableClientState (GLuint nState, int nTMU = -1);
		int DisableClientState (GLuint nState, int nTMU = -1);
		int EnableClientStates (int bTexCoord, int bColor, int bNormals, int nTMU = -1);
		void DisableClientStates (int bTexCoord, int bColor, int bNormals, int nTMU = -1);
		void ResetClientStates (int nFirst = 0);
		void StartFrame (int bFlat, int bResetColorBuf, fix xStereoSeparation);
		void EndFrame (int nWindow);
		void EnableLighting (int bSpecular);
		void DisableLighting (void);
		void SetRenderQuality (int nQuality = -1);
		CViewport& Viewport (void) { return m_states.viewport [0]; }
		void SetViewport (int x, int y, int w, int h);
		void GetViewport (vec4& viewport);
		void SaveViewport (void);
		void RestoreViewport (void);
		void SwapBuffers (int bForce, int bClear);
		void SetupTransform (int bForce);
		void ResetTransform (int bForce);
		void SetScreenMode (void);
		void GetVerInfo (void);
		GLuint CreateDepthTexture (int nTMU, int bFBO, int nType = 0, int nWidth = -1, int hHeight = -1);
		void DestroyDepthTexture (int bFBO);
		GLuint CopyDepthTexture (int nId, int nTMU = GL_TEXTURE1);
		GLuint CreateColorTexture (int nTMU, int bFBO);
		void DestroyColorTexture (void);
		GLuint CopyColorTexture (void);
#if 0
		GLuint CreateStencilTexture (int nTMU, int bFBO);
#endif
		void CreateDrawBuffer (int nType = 1);
		void DestroyDrawBuffer (void);
		void DestroyDrawBuffers (void);
		void SetDrawBuffer (int nBuffer, int bFBO);
		void SetReadBuffer (int nBuffer, int bFBO);
		void FlushStereoBuffers (int nEffects);
		void FlushEffects (int nEffects);
#if MAX_SHADOWMAPS
		void FlushShadowMaps (int nEffects);
#endif
		void FlushDrawBuffer (bool bAdditive = false);
		void ChooseDrawBuffer (void);

		int BindBuffers (CFloatVector *vertexP, int nVertices, int nDimensions,
							  tTexCoord2f *texCoordP, 
							  CFloatVector *colorP, int nColors,
							  CBitmap *bmP, 							  
							  int nTMU = -1);
		void ReleaseBuffers (void);
		int BindBitmap (CBitmap* bmP, int nFrame, int nWrap, int bTextured);
		int RenderArrays (int nPrimitive, 
								CFloatVector *vertexP, int nVertices, int nDimensions = 3,
								tTexCoord2f *texCoordP = NULL, 
								CFloatVector *colorP = NULL, int nColors = 1, 
								CBitmap *bmP = NULL, int nFrame = 0, int nWrap = GL_REPEAT);
		int RenderQuad (CBitmap* bmP, CFloatVector* vertexP, int nDimensions = 3, tTexCoord2f* texCoordP = NULL, CFloatVector* colorP = NULL, int nColors = 1, int nWrap = GL_CLAMP);
		int RenderQuad (CBitmap* bmP, CFloatVector& vPosf, float width, float height, int nDimensions = 3, int nWrap = GL_CLAMP);
		int RenderBitmap (CBitmap* bmP, const CFixVector& vPos, fix xWidth, fix xHeight, CFloatVector* colorP, float alpha, int bAdditive);
		int RenderSprite (CBitmap* bmP, const CFixVector& vPos, fix xWidth, fix xHeight, float alpha, int bAdditive, float fSoftRad);
		void RenderScreenQuad (GLuint nTexture = 0);

		inline int FullScreen (void) { return m_states.bFullScreen; }

		inline void SetFullScreen (int bFullScreen) {
			m_states.bFullScreen = bFullScreen;
			if (m_states.bInitialized)
				SdlGlDoFullScreenInternal (0);
			}

		int ToggleFullScreen (void) {
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
		inline CArray<tTexCoord2f>& TexCoordBuffer (int i = 0) { return Buffers ().texCoord [i]; }
		inline CArray<CFloatVector>& ColorBuffer (void) { return Buffers ().color; }
		inline bool SizeBuffers (int nVerts) { return m_buffers.SizeBuffers (nVerts); }
		inline bool SizeVertexBuffer (int nVerts) { return m_buffers.SizeVertices (nVerts); }
		inline void FlushBuffers (GLenum nPrimitive, int nVerts, int nDimensions = 3, int bTextured = 0, int bColored = 0) {
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
		
		inline bool SetLineSmooth (bool bLineSmooth) { return Enable (m_data.bLineSmooth, bLineSmooth, GL_LINE_SMOOTH); }
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

		inline void SetBlendMode (int nBlendMode) {
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

		int SelectDrawBuffer (int nBuffer, int nColorBuffers = 0);

		int SelectGlowBuffer (void);

		int SelectBlurBuffer (int nBuffer);

		inline CFBO* DrawBuffer (int nBuffer = -1) { return (nBuffer < 0) ? m_data.drawBufferP : m_data.GetDrawBuffer (nBuffer); }

		inline CFBO* GlowBuffer () { return m_data.GetDrawBuffer (2); }

		inline CFBO* BlurBuffer (int nBuffer) { 
			return m_data.GetDrawBuffer ((nBuffer >= 0) ? nBuffer + 3 : int (m_data.xStereoSeparation > 0)); 
			}

		inline CFBO* DepthBuffer (int nBuffer = -1) { return (nBuffer < 0) ? m_data.drawBufferP : m_data.GetDepthBuffer (nBuffer + 5); }

		void RebuildContext (int bGame);
		void DrawArrays (GLenum mode, GLint first, GLsizei count);
		void ColorMask (GLboolean bRed, GLboolean bGreen, GLboolean bBlue, GLboolean bAlpha, GLboolean bEyeOffset = GL_TRUE);
		inline int StereoDevice (int bForce = 0) { 
			return !m_features.bShaders
					 ? 0
					 : !(bForce || gameOpts->render.stereo.bEnhance)
						 ? 0
						 : (gameOpts->render.stereo.nGlasses == GLASSES_AMBER_BLUE) 
							 ? 1 
							 : (gameOpts->render.stereo.nGlasses == GLASSES_RED_CYAN) 
								? 2 
								 : (gameOpts->render.stereo.nGlasses == GLASSES_GREEN_MAGENTA) 
									? 3 
									 : (gameOpts->render.stereo.nGlasses == GLASSES_OCULUS_RIFT)
										? -2
										 : ((gameOpts->render.stereo.nGlasses == GLASSES_SHUTTER) && m_states.nStereo)
											? -1
											: 0; 
			}

		inline void BindTexture (GLuint handle) { 
#if 0 //DBG_OGL < 2
			if (m_data.nTexture [m_data.nTMU [0]] != handle)
#endif
				glBindTexture (GL_TEXTURE_2D, m_data.nTexture [m_data.nTMU [0]] = handle); 
			}

		inline GLuint BoundTexture (void) { return m_data.nTexture [m_data.nTMU [0]]; }

		inline void ReleaseTexture (GLuint handle) { 
			for (int i = 0; i < 4; i++)
				if (m_data.nTexture [i] == handle) {
					glBindTexture (GL_TEXTURE_2D, m_data.nTexture [i] = 0); 
					break;
					}
			}

		inline int IsBound (GLuint handle) { 
			for (int i = 0; i < 4; i++)
				if (m_data.nTexture [i] == handle)
					return i;
			return -1;
			}

#if DBG_OGL
		void VertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int nLine);
		void ColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int nLine);
		void TexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int nLine);
		void NormalPointer (GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int nLine);

		void GenTextures (GLsizei n, GLuint *hTextures);
		void DeleteTextures (GLsizei n, GLuint *hTextures);
	
#else
		inline void VertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int nLine)
			{ glVertexPointer (size, type, stride, pointer); }
		inline void ColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int nLine)
			{ glColorPointer (size, type, stride, pointer); }
		inline void TexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int nLine)
			{ glTexCoordPointer (size, type, stride, pointer); }
		inline void NormalPointer (GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int nLine)
			{ glNormalPointer (type, stride, pointer); }

		inline void GenTextures (GLsizei n, GLuint *hTextures) { glGenTextures (n, hTextures); }
		inline void DeleteTextures (GLsizei n, GLuint *hTextures) { glDeleteTextures (n, hTextures); }
#endif

		inline GLenum ClearError (int bTrapError) {
			GLenum nError = glGetError ();
#if DBG_OGL
			if (nError) {
				const char* pszError = reinterpret_cast<const char*> (gluErrorString (nError));
				PrintLog (0, "%s\n", pszError);
				if (bTrapError)
					nError = nError;
				}
#endif
			return nError;
			}


		inline int SetTransform (int bUseTransform) { return m_states.bUseTransform = bUseTransform; }
		inline int UseTransform (void) { return m_states.bUseTransform; }
		inline void SetStereoSeparation (fix xStereoSeparation) { m_data.xStereoSeparation = xStereoSeparation; }
		inline fix StereoSeparation (void) { return m_data.xStereoSeparation; }

		inline int HaveDrawBuffer (void) {
			return m_features.bRenderToTexture.Available () && m_data.drawBufferP->Handle () && m_data.drawBufferP->Active ();
			}

		int StencilOff (void);
		void StencilOn (int bStencil);
		
		double ZScreen (void);
		
		void InitEnhanced3DShader (void);
		void DeleteEnhanced3DShader (void);
		void InitShadowMapShader (void);
		void DeleteShadowMapShader (void);

#if DBG_OGL
	private:
		int m_bLocked;

	public:
		inline int Lock (void) { return m_bLocked ? 0 : ++m_bLocked; }
		inline int Unlock (void) { return m_bLocked ? --m_bLocked : 0; }
		inline bool Locked (void) { return m_bLocked != 0; }
#endif
};

extern COGL ogl;

//------------------------------------------------------------------------------

static inline void OglCullFace (int bFront)
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
