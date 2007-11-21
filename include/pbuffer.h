//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _PBUFFER_H_
#define _PBUFFER_H_

#ifdef _MSC_VER
#	include <windows.h>
#	include <stddef.h>
#endif

#if RENDER2TEXTURE == 1

#ifdef _WIN32
typedef	HDC			HGLDC;
typedef	HANDLE		HPBUFFER;
#else
typedef	Display	*	HGLDC;
typedef	GLXContext	HGLRC;
typedef	GLXPbuffer	HPBUFFER;
#endif


typedef struct tPixelBuffer {
	HPBUFFER	hBuf;
	HGLDC		hDC;
	HGLRC		hRC;
	GLuint	texId;
	int		nWidth;
	int		nHeight;
	char		bBound;
} tPixelBuffer;

#	ifdef _WIN32
extern PFNWGLCREATEPBUFFERARBPROC				wglCreatePbufferARB;
extern PFNWGLGETPBUFFERDCARBPROC					wglGetPbufferDCARB;
extern PFNWGLRELEASEPBUFFERDCARBPROC			wglReleasePbufferDCARB;
extern PFNWGLDESTROYPBUFFERARBPROC				wglDestroyPbufferARB;
extern PFNWGLQUERYPBUFFERARBPROC					wglQueryPbufferARB;
extern PFNWGLGETPIXELFORMATATTRIBIVARBPROC	wglGetPixelFormatAttribivARB;
extern PFNWGLGETPIXELFORMATATTRIBFVARBPROC	wglGetPixelFormatAttribfvARB;
extern PFNWGLCHOOSEPIXELFORMATARBPROC			wglChoosePixelFormatARB;
extern PFNWGLBINDTEXIMAGEARBPROC					wglBindTexImageARB;
extern PFNWGLRELEASETEXIMAGEARBPROC				wglReleaseTexImageARB;
extern PFNWGLSETPBUFFERATTRIBARBPROC			wglSetPbufferAttribARB;
extern PFNWGLMAKECONTEXTCURRENTARBPROC			wglMakeContextCurrentARB;
extern PFNWGLGETCURRENTREADDCARBPROC			wglGetCurrentReadDCARB;
#	endif

void OglInitPBuffer (void);
int OglCreatePBuffer (tPixelBuffer *pb, int nWidth, int nHeight, int nDepth);
void OglDestroyPBuffer (tPixelBuffer *pb);
int OglPBufferAvail (tPixelBuffer *pb);
int OglEnablePBuffer (tPixelBuffer *pb);
int OglDisablePBuffer (tPixelBuffer *pb);

extern HGLDC	hGlDC;
extern HGLRC	hGlRC;
#	ifndef _WIN32
extern GLXDrawable	hGlWindow;
#	endif

#endif //RENDER2TEXTURE

extern int bRender2TextureOk;
extern int bUseRender2Texture;

#endif //_PBUFFER_H_
