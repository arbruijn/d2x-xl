//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _PBUFFER_H_
#define _PBUFFER_H_

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#endif

#if RENDER2TEXTURE == 1

#ifdef _WIN32
typedef	HDC			HGLDC;
#else
typedef	Display*		HGLDC;
typedef	GLXContext	HGLRC;
typedef	GLXPbuffer	HPBUFFERARB;
#endif


typedef struct tPixelBuffer {
	HPBUFFERARB	hBuf;
	HGLDC			hDC;
	HGLRC			hRC;
	GLuint		texId;
	int32_t			nWidth;
	int32_t			nHeight;
	char			bBound;
} tPixelBuffer;

#ifdef _WIN32
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
#endif

class CPBO {
	private:
		tPixelBuffer	m_info;
	public:
		CPBO () { Init (); }
		~CPBO () { Destroy (); }
		static void Setup (void);
		void Init (void);
		int32_t Create (int32_t nWidth, int32_t nHeight);
		void Destroy (void);
		int32_t Available (void);
		int32_t Enable (void);
		int32_t Disable (void);
		bool Bind (void);
		void Release (void);
		inline int32_t GetWidth (void) { return m_info.nWidth; }
		inline void SetWidth (int32_t nWidth) { m_info.nWidth = nWidth; }
		inline int32_t GetHeight (void) { return m_info.nHeight; }
		inline void SetHeight (int32_t nHeight) { m_info.nHeight = nHeight; }
		inline GLuint GetTexId (void) { return m_info.texId; }
		inline void SetTexId (int32_t nTexId) { m_info.texId = nTexId; }
		inline char Bound (void) { return m_info.bBound; }
		inline HPBUFFERARB Handle (void) { return m_info.hBuf; }
		inline GLuint TexId (void) { return m_info.texId; }
};

extern HGLDC	hGlDC;
extern HGLRC	hGlRC;
#ifndef _WIN32
extern GLXDrawable	hGlWindow;
#endif

#endif //RENDER2TEXTURE

#endif //_PBUFFER_H_
