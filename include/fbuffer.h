#ifndef __FBO_H
#define __FBO_H

#ifdef _WIN32
#	pragma pack(push)
#	pragma pack(8)
#	include <windows.h>
#	pragma pack(pop)
#	include <stddef.h>
#endif
#include "ogl_defs.h"

#if RENDER2TEXTURE == 2

#define MAX_COLOR_BUFFERS 16

typedef struct tFrameBuffer {
	GLuint	hFBO;
	GLuint	hColorBuffer [MAX_COLOR_BUFFERS];
	GLuint	bufferIds [MAX_COLOR_BUFFERS];
	GLuint	hDepthBuffer;
	GLuint	hStencilBuffer;
	int32_t	nColorBuffers;
	int32_t	nFirstBuffer;
	int32_t	nBufferCount;
	int32_t	nBuffer;
	int32_t	nType;
	int32_t	nWidth;
	int32_t	nHeight;
	int32_t	bActive;
	GLenum	nStatus;
} tFrameBuffer;

class CFBO {
	private:
		tFrameBuffer	m_info;

	public:
		CFBO () { Init (); }
		~CFBO () { Destroy (); }
		static void Setup (void);
		void Init (void);
		int32_t Create (int32_t nWidth, int32_t nHeight, int32_t nType, int32_t nColorBuffers = 1);
		void Destroy (void);
		int32_t Available (void);
		int32_t Enable (int32_t nColorBuffers = 0);
		int32_t Disable (void);
		inline int32_t GetType (void) { return m_info.nType; }
		inline void SetType (int32_t nType) { m_info.nType = nType; }
		inline int32_t GetWidth (void) { return m_info.nWidth; }
		inline void SetWidth (int32_t nWidth) { m_info.nWidth = nWidth; }
		inline int32_t GetHeight (void) { return m_info.nHeight; }
		inline void SetHeight (int32_t nHeight) { m_info.nHeight = nHeight; }
		inline GLenum GetStatus (void) { return m_info.nStatus; }
		inline void SetStatus (GLenum nStatus) { m_info.nStatus = nStatus; }
		inline int32_t Active (void) { return m_info.bActive; }
		inline GLuint* BufferIds (void) { return m_info.bufferIds + m_info.nFirstBuffer; }
		inline GLuint BufferCount (void) { return m_info.nBufferCount; }
		inline bool Resize (int32_t nWidth, int32_t nHeight) { return (nWidth != GetWidth ()) || (nHeight != GetHeight ()); }

		inline void SelectColorBuffers (int32_t nBuffer = 0) { 
			if ((m_info.nBufferCount == 1) || (nBuffer >= m_info.nBufferCount)) 
				glDrawBuffer (m_info.bufferIds [nBuffer = 0]);
			else if (nBuffer < 0)
				glDrawBuffers (m_info.nBufferCount, m_info.bufferIds); 
			else
				glDrawBuffer (m_info.bufferIds [nBuffer]);
			m_info.nBuffer = nBuffer;
			}

		int32_t IsBound (void);
		GLuint Handle (void) { return m_info.hFBO; }
		GLuint& ColorBuffer (int32_t i = 0) { return m_info.hColorBuffer [(i < m_info.nColorBuffers) ? i : m_info.nColorBuffers - 1]; }
		GLuint& DepthBuffer (void) { return m_info.hDepthBuffer; }
		GLuint& StencilBuffer (void) { return m_info.hStencilBuffer; }

	private:
		int32_t CreateColorBuffers (int32_t nBuffers);
		int32_t CreateDepthBuffer (void);
		void AttachBuffers (void);

};

#endif //RENDER2TEXTURE

#endif //__FBO_H
