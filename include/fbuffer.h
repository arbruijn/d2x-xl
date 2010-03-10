//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _FBUFFER_H_
#define _FBUFFER_H_

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#endif

#if RENDER2TEXTURE == 2

#define MAX_COLOR_BUFFERS 16

typedef struct tFrameBuffer {
	GLuint	hFBO;
	GLuint	hColorBuffer [MAX_COLOR_BUFFERS];
	GLuint	bufferIds [MAX_COLOR_BUFFERS];
	GLuint	hDepthBuffer;
	GLuint	hStencilBuffer;
	int		nColorBuffers [2];
	int		nType;
	int		nWidth;
	int		nHeight;
	int		bActive;
	GLenum	nStatus;
} tFrameBuffer;

#ifdef _WIN32
extern PFNGLDRAWBUFFERSPROC glDrawBuffers;
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

class CFBO {
	private:
		tFrameBuffer	m_info;
	public:
		CFBO () { Init (); }
		~CFBO () { Destroy (); }
		static void Setup (void);
		void Init (void);
		int Create (int nWidth, int nHeight, int nType, int nColorBuffers = 1);
		void Destroy (void);
		int Available (void);
		int Enable (void);
		int Disable (void);
		inline int GetType (void) { return m_info.nType; }
		inline void SetType (int nType) { m_info.nType = nType; }
		inline int GetWidth (void) { return m_info.nWidth; }
		inline void SetWidth (int nWidth) { m_info.nWidth = nWidth; }
		inline int GetHeight (void) { return m_info.nHeight; }
		inline void SetHeight (int nHeight) { m_info.nHeight = nHeight; }
		inline GLenum GetStatus (void) { return m_info.nStatus; }
		inline void SetStatus (GLenum nStatus) { m_info.nStatus = nStatus; }
		inline int Active (void) { return m_info.bActive; }
		inline GLuint* BufferIds (void) { return m_info.bufferIds; }
		inline GLuint BufferCount (void) { return m_info.nColorBuffers [0]; }
		inline int UseBuffers (int nBuffers = 0) { 
			return m_info.nColorBuffers [0] = (nBuffers && (nBuffers <= m_info.nColorBuffers [1])) ? nBuffers : m_info.nColorBuffers [1]; 
			}
		int IsBound (void);
		GLuint Handle (void) { return m_info.hFBO; }
		GLuint& ColorBuffer (int i = 0) { return m_info.hColorBuffer [(i < m_info.nColorBuffers [1]) ? i : m_info.nColorBuffers [1] - 1]; }
		GLuint& DepthBuffer (void) { return m_info.hDepthBuffer; }
		GLuint& StencilBuffer (void) { return m_info.hStencilBuffer; }
};

#endif //RENDER2TEXTURE

#endif //_FBUFFER_H_
