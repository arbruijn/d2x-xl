//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_INIT_H_
#define _OGL_INIT_H_

#ifdef _MSC_VER
#include <windows.h>
#include <stddef.h>
#endif

#ifdef _WIN32
#	define OGL_MULTI_TEXTURING	1
#	define VERTEX_LIGHTING		1
#	define OGL_QUERY				0
#elif defined(__unix__)
#	define OGL_MULTI_TEXTURING	1
#	define VERTEX_LIGHTING		1
#	define OGL_QUERY				0
#elif defined(__macosx__)
#	define OGL_MULTI_TEXTURING	1
#	define VERTEX_LIGHTING		1
#	define OGL_QUERY				0
#endif

#ifdef _WIN32
#	define LIGHTMAPS 1
#else
#	define LIGHTMAPS 1
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

typedef struct _ogl_texture {
	GLint	 		handle;
	GLint			internalformat;
	GLenum		format;
	int 			w,h,tw,th,lw;
	int 			bytesu;
	int 			bytes;
	GLfloat		u,v;
	GLfloat 		prio;
	int 			wrapstate;
	fix 			lastrend;
	unsigned		long numrend;
	char 			wantmip;
#if RENDER2TEXTURE == 1
	ogl_pbuffer	pbuffer;
#elif RENDER2TEXTURE == 2
	ogl_fbuffer	fbuffer;
#endif
} ogl_texture;

typedef struct tFaceColor {
	char			index;
	tRgbColorf	color;
} tFaceColor;

typedef struct tFaceColord {
	char			index;
	tRgbColord	color;
} tFaceColord;

extern ogl_texture ogl_texture_list[OGL_TEXTURE_LIST_SIZE];

extern int ogl_mem_target;
ogl_texture* OglGetFreeTexture(void);
void OglInitTexture(ogl_texture* t, int bMask);
void OglInitTextureListInternal(void);
void OglSmashTextureListInternal(void);
void ogl_vivify_texture_list_internal(void);

extern int ogl_fullscreen;
void OglDoFullScreenInternal(int);

int OglSetBrightnessInternal (void);
void InitGammaRamp (void);

int OglBindBmTex (grs_bitmap *bm, int nTransp);

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

#	ifndef _WIN32
#	define	wglGetProcAddress	SDL_GL_GetProcAddress
#	endif

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

//#define OGL_SETSTATE(a,s,f) {if (a ## _state!=s) {f;a ## _state=s;}}

#define OGL_TEXENV(p,m) OGL_SETSTATE(p,m,glTexEnvi(GL_TEXTURE_ENV, p,m));
#define OGL_TEXPARAM(p,m) OGL_SETSTATE(p,m,glTexParameteri(GL_TEXTURE_2D,p,m));

#define	OGL_VIEWPORT(_x,_y,_w,_h) \
			{if (((int) (_x) != gameStates.ogl.nLastX) || ((int) (_y) != gameStates.ogl.nLastY) || \
				  ((int) (_w) != gameStates.ogl.nLastW) || ((int) (_h) != gameStates.ogl.nLastH)) \
				{glViewport ((GLint) (_x), (GLint) (grdCurScreen->sc_canvas.cv_bitmap.bm_props.h - (_y) - (_h)), (GLsizei) (_w), (GLsizei) (_h));\
				gameStates.ogl.nLastX = (_x); \
				gameStates.ogl.nLastY = (_y); \
				gameStates.ogl.nLastW = (_w); \
				gameStates.ogl.nLastH = (_h);}}

//platform specific funcs
//MSVC seems to have problems with inline funcs not being found during linking
#ifndef _MSC_VER
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

//void ogl_filltexbuf(unsigned char *data,GLubyte *texp,int width,int height,int twidth,int theight);
void ogl_filltexbuf(unsigned char *data,GLubyte *texp,int truewidth,int width,int height,int dxo,int dyo,int twidth,int theight,int type,int nTransp,int bSuperTransp);
#if RENDER2TEXTURE == 1
int OglLoadBmTextureM (grs_bitmap *bm, int bMipMap, int nTransp, int bMask, ogl_pbuffer *pb);
#elif RENDER2TEXTURE == 2
int OglLoadBmTextureM (grs_bitmap *bm, int bMipMap, int nTransp, int bMask, ogl_fbuffer *pb);
#else
int OglLoadBmTextureM (grs_bitmap *bm, int bMipMap, int nTransp, int bMask, void *pb);
#endif
int OglLoadBmTexture (grs_bitmap *bm, int bMipMap, int nTransp);
//void ogl_loadtexture(unsigned char * data, int width, int height,int dxo,int dyo, int *texid,double *u,double *v,char bMipMap,double prio);
int OglLoadTexture (grs_bitmap *bmP, int dxo,int dyo, ogl_texture *tex, int nTransp, int bSuperTransp);
void OglFreeTexture (ogl_texture *glTexture);
void OglFreeBmTexture (grs_bitmap *bm);
void OglDoPalFx (void);
void OglStartFrame (int bFlat, int bResetColorBuf);
void OglEndFrame (void);
void OglSwapBuffers (int bForce, int bClear);
void OglSetScreenMode (void);
int OglCacheLevelTextures (void);

void OglURect(int left,int top,int right,int bot);
bool OglUBitMapMC (int x, int y, int dw, int dh, grs_bitmap *bm, grs_color *c, int scale, int orient);
bool OglUBitBltI (int dw,int dh,int dx,int dy, int sw, int sh, int sx, int sy, grs_bitmap * src, grs_bitmap * dest, int wantmip);
bool OglUBitBltToLinear (int w,int h,int dx,int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
bool OglUBitBltCopy (int w,int h,int dx,int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
void OglUPixelC (int x, int y, grs_color *c);
void OglULineC (int left,int top,int right,int bot, grs_color *c);
void OglTexWrap (ogl_texture *tex, int state);
void RebuildGfxFx (int bGame, int bCameras);

tRgbColorf *BitmapColor (grs_bitmap *bmP, ubyte *bufP);

extern ubyte *defaultPalette;
extern ubyte *fadePalette;
extern ubyte *gamePalette;
extern ubyte *starsPalette;
extern GLenum curDrawBuffer;

#include "3d.h"

bool G3DrawPolyAlpha (int nv, g3s_point **pointlist, float red, float green, float blue, float alpha);

bool G3DrawTexPolyMulti (
	int nv,
	g3s_point **pointlist,
	uvl *uvl_list,
#if LIGHTMAPS
	uvl *uvl_lMap,
#endif
	grs_bitmap *bmbot,
	grs_bitmap *bm,
#if LIGHTMAPS
	ogl_texture *lightMap,
#endif
	vms_vector	*pvNormal,
	int orient,
	int bBlend);
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
void DrawTexPolyFlat (grs_bitmap *bm,int nv,g3s_point **vertlist);
void OglSetupTransform ();
void OglResetTransform ();
void OglPalColor (ubyte *palette, int c);
void OglGrsColor (grs_color *pc);

#if OGL_MULTI_TEXTURING
#	ifndef GL_VERSION_20
extern PFNGLACTIVETEXTUREARBPROC			glActiveTexture;
#		ifdef _WIN32
extern PFNGLMULTITEXCOORD2DARBPROC		glMultiTexCoord2d;
extern PFNGLMULTITEXCOORD2FARBPROC		glMultiTexCoord2f;
#		endif
extern PFNGLCLIENTACTIVETEXTUREARBPROC	glClientActiveTexture;
#	endif
#endif

//------------------------------------------------------------------------------

static inline int OglUBitBlt (int w,int h,int dx,int dy, int sx, int sy, grs_bitmap *src, grs_bitmap *dest)
{
return OglUBitBltI (w,h,dx,dy,w,h,sx,sy,src,dest, 0);
}

static inline int OglUBitMapM (int x, int y,grs_bitmap *bm)
{
return OglUBitMapMC (x, y, 0, 0, bm, NULL, F1_0, 0);
}

static inline int G3DrawTexPoly (int nv, g3s_point **pointlist, uvl *uvl_list,
											grs_bitmap *bm, vms_vector *pvNormal, int bBlend)
{
#if LIGHTMAPS
return G3DrawTexPolyMulti (nv, pointlist, uvl_list, NULL, bm, NULL, NULL, pvNormal, 0, bBlend);
#else
return G3DrawTexPolyMulti (nv, pointlist, uvl_list, bm, NULL, pvNormal, 0, bBlend);
#endif
}
void OglColor4sf (float r, float g, float b, float s);
void G3VertexColor (fVector3 *pvVertNorm, fVector3 *pVertPos, int nVertex, tFaceColor *pVertColor);

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

#define OglActiveTexture(i)	glActiveTexture (i)

#define OGL_BINDTEX(_handle)	glBindTexture (GL_TEXTURE_2D, _handle)

#endif

extern GLhandleARB	genShaderProg;

//------------------------------------------------------------------------------

#endif
