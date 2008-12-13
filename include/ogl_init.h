//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_INIT_H_
#define _OGL_INIT_H_

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#ifdef _WIN32
#	define OGL_MULTI_TEXTURING	1
#	define VERTEX_LIGHTING		1
#	define OGL_QUERY				1
#elif defined(__unix__)
#	define OGL_MULTI_TEXTURING	1
#	define VERTEX_LIGHTING		1
#	define OGL_QUERY				1
#elif defined(__macosx__)
#	define OGL_MULTI_TEXTURING	1
#	define VERTEX_LIGHTING		1
#	define OGL_QUERY				1
#endif

#define LIGHTMAPS 1

#define TEXTURE_COMPRESSION	0

#if DBG
#	define DBG_SHADERS	0
#else
#	define DBG_SHADERS	0
#endif

#define RENDER2TEXTURE			2	//0: glCopyTexSubImage, 1: pixel buffers, 2: frame buffers
#define OGL_POINT_SPRITES		1

#define DEFAULT_FOV	90.0
#define FISHEYE_FOV	135.0

extern double glFOV, glAspect;

void OglSetFOV (double fov);

#ifndef _WIN32
#	define GL_GLEXT_PROTOTYPES
#endif

#ifdef OGL_RUNTIME_LOAD
#	include "loadgl.h"
int OglInitLoadLibrary (void);
#else
#	ifdef __macosx__
#		include <OpenGL/gl.h>
#		include <OpenGL/glu.h>
#		include <OpenGL/glext.h>
#	else
#		include <GL/gl.h>
#		include <GL/glu.h>
#		ifdef _WIN32
#			include <glext.h>
#			include <wglext.h>
#		else
#			include <GL/glext.h>
#			include <GL/glx.h>
#			include <GL/glxext.h>
#		endif
#	endif
//kludge: since multi texture support has been rewritten
#	undef GL_ARB_multitexture
#	undef GL_SGIS_multitexture
#endif

#ifndef GL_VERSION_1_1
#	ifdef GL_EXT_texture
#		define GL_INTENSITY4 GL_INTENSITY4_EXT
#		define GL_INTENSITY8 GL_INTENSITY8_EXT
#	endif
#endif


#include "gr.h"
#include "palette.h"
#include "pstypes.h"
#include "pbuffer.h"
#include "fbuffer.h"

/* I assume this ought to be >= MAX_BITMAP_FILES in piggy.h? */
#define OGL_TEXTURE_LIST_SIZE 5000

typedef struct tFaceColor {
	tRgbaColorf	color;
	char			index;
} tFaceColor;

typedef struct tFaceColord {
	tRgbColord	color;
	char			index;
} tFaceColord;

extern CTexture ogl_texture_list[OGL_TEXTURE_LIST_SIZE];

extern int nOglMemTarget;
CTexture* OglGetFreeTexture(void);
void OglInitTexture(CTexture* t, int bMask);
void OglInitTextureListInternal(void);
void OglSmashTextureListInternal(void);
void OglVivifyTextureListInternal(void);

extern int ogl_fullscreen;
void OglDoFullScreenInternal(int);

int OglSetBrightnessInternal (void);
void InitGammaRamp (void);

int OglBindBmTex (CBitmap *bmP, int bMipMaps, int nTransp);

extern int ogl_brightness_ok;
extern int ogl_brightness_r, ogl_brightness_g, ogl_brightness_b;

typedef struct tRenderQuality {
	int	texMinFilter;
	int	texMagFilter;
	int	bNeedMipmap;
	int	bAntiAliasing;
} tRenderQuality;

extern tRenderQuality renderQualities [];

extern int bOcclusionQuery;

#ifdef _WIN32
#	ifdef GL_VERSION_20
#		undef GL_VERSION_20
#	endif
#elif defined (__linux__)
#	ifdef GL_VERSION_20
#		undef GL_VERSION_20
#	endif
#else
#	ifndef GL_VERSION_20
#		define GL_VERSION_20
#	endif
#endif

#ifndef GL_VERSION_20

#define GLhandle								GLhandleARB

#if OGL_MULTI_TEXTURING
#define glActiveTexture						pglActiveTextureARB
#define glClientActiveTexture				pglClientActiveTextureARB

extern PFNGLACTIVETEXTUREARBPROC			pglActiveTextureARB;
extern PFNGLMULTITEXCOORD2FARBPROC		pglMultiTexCoord2fARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC pglClientActiveTextureARB;

#endif

#if OGL_QUERY
#define glGenQueries							pglGenQueriesARB
#define glDeleteQueries						pglDeleteQueriesARB
#define glIsQuery								pglIsQueryARB
#define glBeginQuery							pglBeginQueryARB
#define glEndQuery							pglEndQueryARB
#define glGetQueryiv							pglGetQueryivARB
#define glGetQueryObjectiv					pglGetQueryObjectivARB
#define glGetQueryObjectuiv				pglGetQueryObjectuivARB

extern PFNGLGENQUERIESARBPROC        	glGenQueries;
extern PFNGLDELETEQUERIESARBPROC     	glDeleteQueries;
extern PFNGLISQUERYARBPROC           	glIsQuery;
extern PFNGLBEGINQUERYARBPROC        	glBeginQuery;
extern PFNGLENDQUERYARBPROC          	glEndQuery;
extern PFNGLGETQUERYIVARBPROC        	glGetQueryiv;
extern PFNGLGETQUERYOBJECTIVARBPROC  	glGetQueryObjectiv;
extern PFNGLGETQUERYOBJECTUIVARBPROC 	glGetQueryObjectuiv;
#endif

#if OGL_POINT_SPRITES
#	ifdef _WIN32
extern PFNGLPOINTPARAMETERFVARBPROC		glPointParameterfvARB;
extern PFNGLPOINTPARAMETERFARBPROC		glPointParameterfARB;
#	endif
#endif

#ifdef _WIN32
extern PFNGLGENBUFFERSPROC					glGenBuffers;
extern PFNGLBINDBUFFERPROC					glBindBuffer;
extern PFNGLBUFFERDATAPROC					glBufferData;
extern PFNGLMAPBUFFERPROC					glMapBuffer;
extern PFNGLUNMAPBUFFERPROC				glUnmapBuffer;
extern PFNGLDELETEBUFFERSPROC				glDeleteBuffers;
extern PFNGLDRAWRANGEELEMENTSPROC		glDrawRangeElements;
#endif

#	ifdef _WIN32
extern PFNGLACTIVESTENCILFACEEXTPROC	glActiveStencilFaceEXT;
#	endif

#	ifndef _WIN32
#	define	wglGetProcAddress	SDL_GL_GetProcAddress
#	endif

#endif

#ifndef GL_VERSION_20
#		define	glCreateShaderObject			pglCreateShaderObjectARB
#		define	glShaderSource					pglShaderSourceARB
#		define	glCompileShader				pglCompileShaderARB
#		define	glCreateProgramObject		pglCreateProgramObjectARB
#		define	glAttachObject					pglAttachObjectARB
#		define	glLinkProgram					pglLinkProgramARB
#		define	glUseProgramObject			pglUseProgramObjectARB
#		define	glDeleteObject					pglDeleteObjectARB
#		define	glGetObjectParameteriv		pglGetObjectParameterivARB
#		define	glGetInfoLog					pglGetInfoLogARB
#		define	glGetUniformLocation			pglGetUniformLocationARB
#		define	glUniform4f						pglUniform4fARB
#		define	glUniform3f						pglUniform3fARB
#		define	glUniform1f						pglUniform1fARB
#		define	glUniform1i						pglUniform1iARB
#		define	glUniform4fv					pglUniform4fvARB
#		define	glUniform3fv					pglUniform3fvARB
#		define	glUniform1fv					pglUniform1fvARB
#		define	glUniform1i						pglUniform1iARB

extern PFNGLCREATESHADEROBJECTARBPROC		glCreateShaderObject;
extern PFNGLSHADERSOURCEARBPROC				glShaderSource;
extern PFNGLCOMPILESHADERARBPROC				glCompileShader;
extern PFNGLCREATEPROGRAMOBJECTARBPROC		glCreateProgramObject;
extern PFNGLATTACHOBJECTARBPROC				glAttachObject;
extern PFNGLLINKPROGRAMARBPROC				glLinkProgram;
extern PFNGLUSEPROGRAMOBJECTARBPROC			glUseProgramObject;
extern PFNGLDELETEOBJECTARBPROC				glDeleteObject;
extern PFNGLGETOBJECTPARAMETERIVARBPROC	glGetObjectParameteriv;
extern PFNGLGETINFOLOGARBPROC					glGetInfoLog;
extern PFNGLGETUNIFORMLOCATIONARBPROC		glGetUniformLocation;
extern PFNGLUNIFORM4FARBPROC					glUniform4f;
extern PFNGLUNIFORM3FARBPROC					glUniform3f;
extern PFNGLUNIFORM1FARBPROC					glUniform1f;
extern PFNGLUNIFORM1IARBPROC					glUniform1i;
extern PFNGLUNIFORM4FVARBPROC					glUniform4fv;
extern PFNGLUNIFORM3FVARBPROC					glUniform3fv;
extern PFNGLUNIFORM1FVARBPROC					glUniform1fv;
#else
#  ifdef __macosx__
#    define glCreateShaderObject   glCreateShaderObjectARB
#    define glShaderSource         glShaderSourceARB
#    define glCompileShader        glCompileShaderARB
#    define glCreateProgramObject  glCreateProgramObjectARB
#    define glAttachObject         glAttachObjectARB
#    define glLinkProgram          glLinkProgramARB
#    define glUseProgramObject     glUseProgramObjectARB
#    define glDeleteObject         glDeleteObjectARB
#    define glGetObjectParameteriv glGetObjectParameterivARB
#    define glGetInfoLog           glGetInfoLogARB
#    define glGetUniformLocation   glGetUniformLocationARB
#    define glUniform4f            glUniform4fARB
#    define glUniform3f            glUniform3fARB
#    define glUniform1f            glUniform1fARB
#    define glUniform1i            glUniform1iARB
#    define glUniform4fv           glUniform4fvARB
#    define glUniform3fv           glUniform3fvARB
#    define glUniform1fv           glUniform1fvARB
#  endif
#endif

//extern int GL_texclamp_enabled;
//#define OGL_ENABLE2(a,f) {if (a ## _enabled!=1) {f;a ## _enabled=1;}}
//#define OGL_DISABLE2(a,f) {if (a ## _enabled!=0) {f;a ## _enabled=0;}}

//#define OGL_ENABLE(a) OGL_ENABLE2(a,glEnable(a))
//#define OGL_DISABLE(a) OGL_DISABLE2(a,glDisable(a))
//#define OGL_ENABLE(a) OGL_ENABLE2(GL_ ## a,glEnable(GL_ ## a))
//#define OGL_DISABLE(a) OGL_DISABLE2(GL_ ## a,glDisable(GL_ ## a))

//#define OGL_TEXCLAMP() OGL_ENABLE2(GL_texclamp,glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);)
//#define OGL_TEXREPEAT() OGL_DISABLE2(GL_texclamp,glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);)

//#define OGL_SETSTATE(a,s,f) {if (a ## State!=s) {f;a ## State=s;}}

#define OGL_TEXENV(p,m) OGL_SETSTATE(p,m,glTexEnvi(GL_TEXTURE_ENV, p,m));
#define OGL_TEXPARAM(p,m) OGL_SETSTATE(p,m,glTexParameteri(GL_TEXTURE_2D,p,m));

#define	OGL_VIEWPORT(_x,_y,_w,_h) \
			{if (((int) (_x) != gameStates.ogl.nLastX) || ((int) (_y) != gameStates.ogl.nLastY) || \
				  ((int) (_w) != gameStates.ogl.nLastW) || ((int) (_h) != gameStates.ogl.nLastH)) \
				{glViewport ((GLint) (_x), (GLint) (grdCurScreen->scCanvas.cvBitmap.props.h - (_y) - (_h)), (GLsizei) (_w), (GLsizei) (_h));\
				gameStates.ogl.nLastX = (_x); \
				gameStates.ogl.nLastY = (_y); \
				gameStates.ogl.nLastW = (_w); \
				gameStates.ogl.nLastH = (_h);}}

//platform specific funcs
//MSVC seems to have problems with inline funcs not being found during linking
#ifndef _WIN32
inline
#endif
void OglSwapBuffersInternal (void);
int OglVideoModeOK (int x, int y); // check if mode is valid
int OglInitWindow (int x, int y, int bForce);//create a window/switch modes/etc
void OglDestroyWindow (void);//destroy window/etc
void OglInitAttributes (void);//one time initialization
void OglClose (void);//one time shutdown

//generic funcs
//#define OGLTEXBUFSIZE (1024*1024*4)

//void ogl_filltexbuf(ubyte *data,GLubyte *texp,int width,int height,int twidth,int theight);
void ogl_filltexbuf(ubyte *data,GLubyte *texp,int truewidth,int width,int height,int dxo,int dyo,int twidth,int theight,int nType,int nTransp,int bSuperTransp);
#if RENDER2TEXTURE == 1
int OglLoadBmTextureM (CBitmap *bm, int bMipMap, int nTransp, int bMask, tPixelBuffer *pb);
#elif RENDER2TEXTURE == 2
int OglLoadBmTextureM (CBitmap *bm, int bMipMap, int nTransp, int bMask, tFrameBuffer *pb);
#else
int OglLoadBmTextureM (CBitmap *bm, int bMipMap, int nTransp, int bMask, void *pb);
#endif
int OglLoadBmTexture (CBitmap *bm, int bMipMap, int nTransp, int bLoad);
//void ogl_loadtexture(ubyte * data, int width, int height,int dxo,int dyo, int *texid,double *u,double *v,char bMipMap,double prio);
int OglLoadTexture (CBitmap *bmP, int dxo,int dyo, CTexture *tex, int nTransp, int bSuperTransp);
void OglFreeTexture (CTexture *glTexture);
void OglFreeBmTexture (CBitmap *bm);
int OglSetupBmFrames (CBitmap *bmP, int bDoMipMap, int nTransp, int bLoad);
void OglDoPalFx (void);
void OglStartFrame (int bFlat, int bResetColorBuf);
void OglEndFrame (void);
void OglSwapBuffers (int bForce, int bClear);
void OglSetScreenMode (void);
int OglCacheLevelTextures (void);

void OglURect(int left,int top,int right,int bot);
int OglUBitMapMC (int x, int y, int dw, int dh, CBitmap *bm, tCanvasColor *c, int scale, int orient);
int OglUBitBltI (int dw,int dh,int dx,int dy, int sw, int sh, int sx, int sy, CBitmap * src, CBitmap * dest, int bMipMaps, int bTransp);
int OglUBitBltToLinear (int w,int h,int dx,int dy, int sx, int sy, CBitmap * src, CBitmap * dest);
int OglUBitBltCopy (int w,int h,int dx,int dy, int sx, int sy, CBitmap * src, CBitmap * dest);
void OglUPixelC (int x, int y, tCanvasColor *c);
void OglULineC (int left,int top,int right,int bot, tCanvasColor *c);
void OglUPolyC (int left, int top, int right, int bot, tCanvasColor *c);
void OglTexWrap (CTexture *tex, int state);
void RebuildRenderContext (int bGame);

tRgbColorf *BitmapColor (CBitmap *bmP, ubyte *bufP);

extern GLenum curDrawBuffer;

#include "3d.h"
#include "gr.h"

int G3DrawWhitePoly (int nv, g3sPoint **pointList);
int G3DrawPolyAlpha (int nv, g3sPoint **pointlist, tRgbaColorf *color, char bDepthMask);
int G3DrawFace (tFace *faceP, CBitmap *bmBot, CBitmap *bmTop, int bBlend, int bDrawArrays, int bTextured, int bDepthOnly);
void G3FlushFaceBuffer (int bForce);

int G3DrawTexPolyMulti (
	int			nVerts, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	tBitmapInfo	*bmBot, 
	tBitmapInfo	*bmTop, 
	tLightmap	*lightmap, 
	CFixVector	*pvNormal,
	int			orient, 
	int			bBlend);

int G3DrawTexPolyLightmap (
	int			nVerts, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	tBitmapInfo	*bmBot, 
	tBitmapInfo	*bmTop, 
	tLightmap	*lightmap, 
	CFixVector	*pvNormal,
	int			orient, 
	int			bBlend);

int G3DrawTexPolyFlat (
	int			nVerts, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tUVL			*uvlLMap, 
	tBitmapInfo	*bmBot, 
	tBitmapInfo	*bmTop, 
	tLightmap	*lightmap, 
	CFixVector	*pvNormal,
	int			orient, 
	int			bBlend);

void OglDrawReticle (int cross,int primary,int secondary);
void OglCachePolyModelTextures (int nModel);

//whee
//#define PAL2Tr(c) ((gr_palette[c*3]+gameData.render.bPaletteGamma)/63.0)
//#define PAL2Tg(c) ((gr_palette[c*3+1]+gameData.render.bPaletteGamma)/63.0)
//#define PAL2Tb(c) ((gr_palette[c*3+2]+gameData.render.bPaletteGamma)/63.0)
//#define PAL2Tr(c) ((gr_palette[c*3])/63.0)
//#define PAL2Tg(c) ((gr_palette[c*3+1])/63.0)
//#define PAL2Tb(c) ((gr_palette[c*3+2])/63.0)
#if 1
#define CPAL2Tr(_p,_c)	((float) ((_p)[(_c)*3])/63.0f)
#define CPAL2Tg(_p,_c)	((float) ((_p)[(_c)*3+1])/63.0f)
#define CPAL2Tb(_p,_c)	((float) ((_p)[(_c)*3+2])/63.0f)
#define PAL2Tr(_p,_c)	((float) ((_p)[(_c)*3])/63.0f)
#define PAL2Tg(_p,_c)	((float) ((_p)[(_c)*3+1])/63.0f)
#define PAL2Tb(_p,_c)	((float) ((_p)[(_c)*3+2])/63.0f)
#endif
#define CC2T(c) ((float) c / 255.0f)
//inline GLfloat PAL2Tr(int c);
//inline GLfloat PAL2Tg(int c);
//inline GLfloat PAL2Tb(int c);

GLuint EmptyTexture (int Xsize, int Ysize);
char *LoadShader (char* fileName);
int CreateShaderProg (GLhandleARB *progP);
int CreateShaderFunc (GLhandleARB *progP, GLhandleARB *fsP, GLhandleARB *vsP, 
							 char *fsName, char *vsName, int bFromFile);
int LinkShaderProg (GLhandleARB *progP);
void DeleteShaderProg (GLhandleARB *progP);
void InitShaders ();
void SetRenderQuality ();
void DrawTexPolyFlat (CBitmap *bm,int nv,g3sPoint **vertlist);
void OglSetupTransform (int bForce);
void OglResetTransform (int bForce);
void OglPalColor (ubyte *palette, int c);
void OglCanvasColor (tCanvasColor *pc);
void OglBlendFunc (GLenum nSrcBlend, GLenum nDestBlend);
int G3EnableClientState (GLuint nState, int nTMU);
int G3EnableClientStates (int bTexCoord, int bColor, int bNormals, int nTMU);
void G3DisableClientStates (int bTexCoord, int bColor, int bNormals, int nTMU);
int OglRenderArrays (CBitmap *bmP, int nFrame, CFloatVector *vertexP, int nVertices, tTexCoord3f *texCoordP, 
							tRgbaColorf *colorP, int nColors, int nPrimitive, int nWrap);

//------------------------------------------------------------------------------

#if OGL_MULTI_TEXTURING
#	ifndef GL_VERSION_20
extern PFNGLACTIVETEXTUREARBPROC			glActiveTexture;
#		ifdef _WIN32
extern PFNGLMULTITEXCOORD2DARBPROC		glMultiTexCoord2d;
extern PFNGLMULTITEXCOORD2FARBPROC		glMultiTexCoord2f;
extern PFNGLMULTITEXCOORD2FVARBPROC		glMultiTexCoord2fv;
#		endif
extern PFNGLCLIENTACTIVETEXTUREARBPROC	glClientActiveTexture;
#	endif
#endif

//------------------------------------------------------------------------------

static inline int OglUBitBlt (int w,int h,int dx,int dy, int sx, int sy, CBitmap *src, CBitmap *dest, int bTransp)
{
return OglUBitBltI (w, h, dx, dy, w, h, sx, sy, src, dest, 0, bTransp);
}

static inline int OglUBitMapM (int x, int y,CBitmap *bm)
{
return OglUBitMapMC (x, y, 0, 0, bm, NULL, F1_0, 0);
}

typedef struct tSinCosf {
	double	dSin, dCos;
} tSinCosf;

void OglComputeSinCos (int nSides, tSinCosf *sinCosP);
void OglColor4sf (float r, float g, float b, float s);
void G3VertexColor (CFloatVector *pvVertNorm, CFloatVector *pVertPos, int nVertex, tFaceColor *pVertColor, 
						  tFaceColor *pBaseColor, float fScale, int bSetColor, int nThread);
void OglDrawEllipse (int nSides, int nType, double xsc, double xo, double ysc, double yo, tSinCosf *sinCosP);
void OglDrawCircle (int nSides, int nType);

void OglEnableLighting (int bSpecular);
void OglDisableLighting (void);

#define BINDTEX_OPT 0

#if BINDTEX_OPT

extern GLint	boundHandles [8];
extern int		nTMU;

#define OglActiveTexture(_i)	{nTMU = (_i) - GL_TEXTURE0_ARB; glActiveTexture (_i);}

extern int nTexBinds, nTexBindCalls;

static inline void OGL_BINDTEX (GLint handle) 
{
nTexBindCalls++;
if (nTMU < 0) {
	glBindTexture (GL_TEXTURE_2D, handle);
	nTexBinds++;
	}
else if (!handle || (boundHandles [nTMU] != handle)) {
	boundHandles [nTMU] = handle; 
	glBindTexture (GL_TEXTURE_2D, handle);
	nTexBinds++;
	nTMU = -1;
	}
}

#else

#define OglActiveTexture(i,b)	if (b) glClientActiveTexture (i); else glActiveTexture (i)

#define OGL_BINDTEX(_handle)	glBindTexture (GL_TEXTURE_2D, _handle)

#endif

extern GLhandleARB	genShaderProg;

typedef	int tTexPolyMultiDrawer (int, g3sPoint **, tUVL *, tUVL *, CBitmap *, CBitmap *, CTexture *, CFixVector *, int, int);
extern tTexPolyMultiDrawer	*fpDrawTexPolyMulti;

int G3DrawTexPolySimple (
	int			nVerts, 
	g3sPoint		**pointList, 
	tUVL			*uvlList, 
	tBitmapInfo	*bmBot, 
	CFixVector	*pvNormal,
	int			bBlend);

//------------------------------------------------------------------------------

static inline int G3DrawTexPoly (int nVerts, g3sPoint **pointList, tUVL *uvlList,
											CBitmap *bmP, CFixVector *pvNormal, int bBlend)
{
return fpDrawTexPolyMulti (nVerts, pointList, uvlList, NULL, bmP, NULL, NULL, pvNormal, 0, bBlend);
}

//------------------------------------------------------------------------------

#endif
