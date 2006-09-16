//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _FBUFFER_H_
#define _FBUFFER_H_

#ifdef _MSC_VER
#	include <windows.h>
#	include <stddef.h>
#endif

#if RENDER2TEXTURE == 2

typedef struct _ogl_fbuffer {
	GLuint	hBuf;
	GLuint	hDepthRb;
	GLuint	texId;
	int		nWidth;
	int		nHeight;
	GLenum	nStatus;
} ogl_fbuffer;

#ifdef _WIN32
extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
extern PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT;
extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;
extern PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT;
extern PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT;
#endif

void OglInitFBuffer (void);
int OglCreateFBuffer (ogl_fbuffer *pb, int nWidth, int nHeight, int nDepth);
void OglDestroyFBuffer (ogl_fbuffer *pb);
int OglFBufferAvail (ogl_fbuffer *pb);
int OglEnableFBuffer (ogl_fbuffer *pb);
int OglDisableFBuffer (ogl_fbuffer *pb);

#endif //RENDER2TEXTURE

extern int bRender2TextureOk;
extern int bUseRender2Texture;

#endif //_FBUFFER_H_
