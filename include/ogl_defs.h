//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_DEFS_H
#define _OGL_DEFS_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#ifdef __macosx__
#	include <OpenGL/gl.h>
#	include <OpenGL/glu.h>
#	include <OpenGL/glext.h>
#else
#	include <GL/gl.h>
#	include <GL/glu.h>
#	ifdef _WIN32
#		include <glext.h>
#		include <wglext.h>
#	else
#		include <GL/glext.h>
#		include <GL/glx.h>
#		include <GL/glxext.h>
#	endif
#endif

//kludge: since multi texture support has been rewritten
#undef GL_ARB_multitexture
#undef GL_SGIS_multitexture

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

#define LIGHTMAPS					1

#define TEXTURE_COMPRESSION	0

#ifdef _DEBUG
#	define DBG_SHADERS			0
#else
#	define DBG_SHADERS			0
#endif

#define RENDER2TEXTURE			2	//0: glCopyTexSubImage, 1: pixel buffers, 2: frame buffers
#define OGL_POINT_SPRITES		1

#define DEFAULT_FOV				90.0
#define FISHEYE_FOV				135.0

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

//------------------------------------------------------------------------------

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

// multi texturing -------------------------------------------------------------

#	if OGL_MULTI_TEXTURING
#	define glActiveTexture				pglActiveTextureARB
#	define glClientActiveTexture		pglClientActiveTextureARB

extern PFNGLACTIVETEXTUREARBPROC			pglActiveTextureARB;
extern PFNGLMULTITEXCOORD2FARBPROC		pglMultiTexCoord2fARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC pglClientActiveTextureARB;

extern PFNGLACTIVETEXTUREARBPROC			glActiveTexture;
#		ifdef _WIN32
extern PFNGLMULTITEXCOORD2DARBPROC		glMultiTexCoord2d;
extern PFNGLMULTITEXCOORD2FARBPROC		glMultiTexCoord2f;
extern PFNGLMULTITEXCOORD2FVARBPROC		glMultiTexCoord2fv;
#		endif
extern PFNGLCLIENTACTIVETEXTUREARBPROC	glClientActiveTexture;
#	endif

// occlusion queries -----------------------------------------------------------

#	if OGL_QUERY
#		define glGenQueries						pglGenQueriesARB
#		define glDeleteQueries					pglDeleteQueriesARB
#		define glIsQuery							pglIsQueryARB
#		define glBeginQuery						pglBeginQueryARB
#		define glEndQuery							pglEndQueryARB
#		define glGetQueryiv						pglGetQueryivARB
#		define glGetQueryObjectiv				pglGetQueryObjectivARB
#		define glGetQueryObjectuiv				pglGetQueryObjectuivARB

extern PFNGLGENQUERIESARBPROC        	glGenQueries;
extern PFNGLDELETEQUERIESARBPROC     	glDeleteQueries;
extern PFNGLISQUERYARBPROC           	glIsQuery;
extern PFNGLBEGINQUERYARBPROC        	glBeginQuery;
extern PFNGLENDQUERYARBPROC          	glEndQuery;
extern PFNGLGETQUERYIVARBPROC        	glGetQueryiv;
extern PFNGLGETQUERYOBJECTIVARBPROC  	glGetQueryObjectiv;
extern PFNGLGETQUERYOBJECTUIVARBPROC 	glGetQueryObjectuiv;
#	endif

// point sprites --------------------------------------------------------------

#	if OGL_POINT_SPRITES
#		ifdef _WIN32
extern PFNGLPOINTPARAMETERFVARBPROC		glPointParameterfvARB;
extern PFNGLPOINTPARAMETERFARBPROC		glPointParameterfARB;
#		endif
#	endif

// vertex buffer objects -------------------------------------------------------

extern PFNGLGENBUFFERSPROC					glGenBuffers;
extern PFNGLBINDBUFFERPROC					glBindBuffer;
extern PFNGLBUFFERDATAPROC					glBufferData;
extern PFNGLMAPBUFFERPROC					glMapBuffer;
extern PFNGLUNMAPBUFFERPROC				glUnmapBuffer;
extern PFNGLDELETEBUFFERSPROC				glDeleteBuffers;
#	ifdef _WIN32
extern PFNGLDRAWRANGEELEMENTSPROC		glDrawRangeElements;
#	endif

// stencil operations ----------------------------------------------------------

#	ifdef _WIN32
extern PFNGLACTIVESTENCILFACEEXTPROC	glActiveStencilFaceEXT;
#	endif

//------------------------------------------------------------------------------

#	define	glCreateShaderObject			pglCreateShaderObjectARB
#	define	glShaderSource					pglShaderSourceARB
#	define	glCompileShader				pglCompileShaderARB
#	define	glCreateProgramObject		pglCreateProgramObjectARB
#	define	glAttachObject					pglAttachObjectARB
#	define	glLinkProgram					pglLinkProgramARB
#	define	glUseProgramObject			pglUseProgramObjectARB
#	define	glDeleteObject					pglDeleteObjectARB
#	define	glGetObjectParameteriv		pglGetObjectParameterivARB
#	define	glGetInfoLog					pglGetInfoLogARB
#	define	glGetUniformLocation			pglGetUniformLocationARB
#	define	glUniform4f						pglUniform4fARB
#	define	glUniform3f						pglUniform3fARB
#	define	glUniform1f						pglUniform1fARB
#	define	glUniform1i						pglUniform1iARB
#	define	glUniform4fv					pglUniform4fvARB
#	define	glUniform3fv					pglUniform3fvARB
#	define	glUniform2fv					pglUniform2fvARB
#	define	glUniform1fv					pglUniform1fvARB
#	define	glUniform1i						pglUniform1iARB

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
extern PFNGLUNIFORM2FVARBPROC					glUniform2fv;
extern PFNGLUNIFORM1FVARBPROC					glUniform1fv;

//------------------------------------------------------------------------------

#	ifndef _WIN32
#	define	wglGetProcAddress	SDL_GL_GetProcAddress
#	endif

#else //GL_VERSION_20

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

#endif //GL_VERSION_20

#define OGL_TEXENV(p,m) OGL_SETSTATE(p,m,glTexEnvi(GL_TEXTURE_ENV, p,m));
#define OGL_TEXPARAM(p,m) OGL_SETSTATE(p,m,glTexParameteri(GL_TEXTURE_2D,p,m));

void OglInitExtensions (void);
void OglViewport (int x, int y, int w, int h);

//------------------------------------------------------------------------------

extern char *pszOglExtensions;

//------------------------------------------------------------------------------

#endif
