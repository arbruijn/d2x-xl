//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _FBUFFER_H_
#define _FBUFFER_H_

#ifdef _MSC_VER
#	include <windows.h>
#	include <stddef.h>
#endif

#if RENDER2TEXTURE == 2

typedef struct tFrameBuffer {
	GLuint	hFBO;
	GLuint	hDepthBuffer;
	GLuint	hRenderBuffer;
	int		nWidth;
	int		nHeight;
	GLenum	nStatus;
} tFrameBuffer;

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
int OglCreateFBuffer (tFrameBuffer *pb, int nWidth, int nHeight, int nType);
void OglDestroyFBuffer (tFrameBuffer *pb);
int OglFBufferAvail (tFrameBuffer *pb);
int OglEnableFBuffer (tFrameBuffer *pb);
int OglDisableFBuffer (tFrameBuffer *pb);

#endif //RENDER2TEXTURE

extern int bRender2TextureOk;
extern int bUseRender2Texture;

#endif //_FBUFFER_H_
