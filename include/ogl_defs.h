//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_DEFS_H
#define _OGL_DEFS_H

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#define GL_FALLBACK	0	// fall back to glBegin/glEnd when client arrays aren't available

#define TEXTURE_COMPRESSION	0

#if DBG
#	define DBG_SHADERS			0
#	define DBG_OGL					1
#else
#	define DBG_SHADERS			0
#	define DBG_OGL					0
#endif

#define RENDER2TEXTURE			2	//0: glCopyTexSubImage, 1: pixel buffers, 2: frame buffers
#define OGL_POINT_SPRITES		1

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#endif

#ifdef __macosx__
#	include "glew.h"
#	include <OpenGL/gl.h>
#	include <OpenGL/glu.h>
#else
#	ifdef __unix__
#		include <GL/glew.h>
#		include <GL/glxew.h>
#		include <GL/glx.h>
#	else
#		include "glew.h"
#		include "wglew.h"
#	endif
#	include <GL/gl.h>
#	include <GL/glu.h>
#endif

#define DEFAULT_FOV				105.0
#define FISHEYE_FOV				135.0

#define STEREO_MIN_FOV			90
#define STEREO_MAX_FOV			160
#define STEREO_DEFAULT_FOV		135
#define STEREO_FOV_STEP			5

#define RIFT_MIN_IPD				54
#define RIFT_DEFAULT_IPD		64
#define RIFT_MAX_IPD				72

extern double glFOV, glAspect;

void OglSetFOV (double fov);

//#include "gr.h"
//#include "palette.h"
#include "pstypes.h"
#include "pbuffer.h"
#include "fbuffer.h"

//------------------------------------------------------------------------------

#ifndef GL_VERSION_20
#	define GLhandle	GLhandleARB
#endif

#define OGL_TEXENV(p,m) OGL_SETSTATE(p,m,glTexEnvi(GL_TEXTURE_ENV, p,m));
#define OGL_TEXPARAM(p,m) OGL_SETSTATE(p,m,glTexParameteri(GL_TEXTURE_2D,p,m));

//-----------------------------------------------------------------------------

typedef union tTexCoord2f {
	float a [2];
	struct {
		float	u, v;
		} v;
	} __pack__ tTexCoord2f;

typedef union tTexCoord3f {
	float a [3];
	struct {
		float	u, v, l;
		} v;
	} __pack__ tTexCoord3f;

//------------------------------------------------------------------------------

#if DBG_OGL
#define	OglDrawArrays(mode, first, count)					ogl.DrawArrays (mode, first, count)
#define	OglVertexPointer(size, type, stride, pointer)	ogl.VertexPointer (size, type, stride, pointer, const_cast<char*>(__FILE__), __LINE__)
#define	OglColorPointer(size, type, stride, pointer)		ogl.ColorPointer (size, type, stride, pointer, const_cast<char*>(__FILE__), __LINE__)
#define	OglTexCoordPointer(size, type, stride, pointer)	ogl.TexCoordPointer (size, type, stride, pointer, const_cast<char*>(__FILE__), __LINE__)
#define	OglNormalPointer(type, stride, pointer)			ogl.NormalPointer (type, stride, pointer, const_cast<char*>(__FILE__), __LINE__)
#else
#define	OglDrawArrays(mode, first, count)					glDrawArrays (mode, first, count)
#define	OglVertexPointer(size, type, stride, pointer)	glVertexPointer (size, type, stride, pointer)
#define	OglColorPointer(size, type, stride, pointer)		glColorPointer (size, type, stride, pointer)
#define	OglTexCoordPointer(size, type, stride, pointer)	glTexCoordPointer (size, type, stride, pointer)
#define	OglNormalPointer(type, stride, pointer)			glNormalPointer (type, stride, pointer)
#endif

//------------------------------------------------------------------------------

extern const char *pszOglExtensions;

//------------------------------------------------------------------------------

#endif
