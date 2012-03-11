//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _FBUFFER_H_
#define _FBUFFER_H_

#ifdef _WIN32
#	include <windows.h>
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
	int		nColorBuffers;
	int		nFirstBuffer;
	int		nBufferCount;
	int		nBuffer;
	int		nType;
	int		nWidth;
	int		nHeight;
	int		bActive;
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
		int Create (int nWidth, int nHeight, int nType, int nColorBuffers = 1);
		void Destroy (void);
		int Available (void);
		int Enable (int nColorBuffers = 0);
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
		inline GLuint* BufferIds (void) { return m_info.bufferIds + m_info.nFirstBuffer; }
		inline GLuint BufferCount (void) { return m_info.nBufferCount; }

		inline int UseBuffers (int nFirst = 0, int nLast = 0) { 
			if (nFirst > m_info.nColorBuffers - 1)
				nFirst = m_info.nColorBuffers - 1;
			if (nLast < nFirst)
				nLast = nFirst;
			else if (nLast > m_info.nColorBuffers - 1)
				nLast = m_info.nColorBuffers - 1;
			m_info.nBufferCount = nLast - nFirst + 1;
			return nFirst;
			}

		inline void SelectColorBuffers (int nBuffer = 0) { 
			if ((m_info.nBufferCount == 1) || (nBuffer >= m_info.nBufferCount)) 
				glDrawBuffer (m_info.bufferIds [nBuffer = 0]);
			else if (nBuffer < 0)
				glDrawBuffers (m_info.nBufferCount, m_info.bufferIds); 
			else
				glDrawBuffer (m_info.bufferIds [nBuffer]);
			m_info.nBuffer = nBuffer;
			}

		void FlipBuffers (int i, int j) {
			if ((i != j) && (i < m_info.nBufferCount) && (j < m_info.nBufferCount)) {
				GLuint h = m_info.bufferIds [i];
				m_info.bufferIds [i] = m_info.bufferIds [j];
				m_info.bufferIds [j] = h;
				}
			}

		int IsBound (void);
		GLuint Handle (void) { return m_info.hFBO; }
		GLuint& ColorBuffer (int i = 0) { return m_info.hColorBuffer [(i < m_info.nColorBuffers) ? i : m_info.nColorBuffers - 1]; }
		GLuint& DepthBuffer (void) { return m_info.hDepthBuffer; }
		GLuint& StencilBuffer (void) { return m_info.hStencilBuffer; }

	private:
		int CreateColorBuffers (int nBuffers);
		int CreateDepthBuffer (void);
		void AttachBuffers (void);

};

#endif //RENDER2TEXTURE

#endif //_FBUFFER_H_
