//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_TEXTURE_H
#define _OGL_TEXTURE_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif
#include "ogl_defs.h"
#include "fbuffer.h"
#include "pbuffer.h"
#include "cdatapool.h"

//------------------------------------------------------------------------------

#define OglActiveTexture(i,b)	if (b) glClientActiveTexture (i); else glActiveTexture (i)

#define OGL_BINDTEX(_handle)	glBindTexture (GL_TEXTURE_2D, _handle)

//------------------------------------------------------------------------------

class CBitmap;

typedef struct tTexture {
	int				index;
	GLuint	 		handle;
	GLint				internalFormat;
	GLenum			format;
	int 				w, h, tw, th, lw;
//	int 				bytesu;
//	int 				bytes;
	GLfloat			u, v;
	ubyte				bMipMaps;
	ubyte				bRenderBuffer;
#if RENDER2TEXTURE == 1
	CPBO				pbo;
#elif RENDER2TEXTURE == 2
	CFBO				fbo;
	CBitmap			*bmP;
#endif
} tTexture;

class CTexture {
	private:
#if 0
		int		m_prev, m_next;
#endif
		tTexture	m_info;
	public:
		CTexture () { Init (); }
		~CTexture () { Destroy (); }
		GLuint Create (int w, int h);
		void Init (void);
		void Setup (int w, int h, int lw, int bMask, int bMipMap, CBitmap *bmP);
		int Prepare (int w, int h, int bpp, bool bLocal, bool bCompressed = false);
#if TEXTURE_COMPRESSION
		int Load (ubyte *buffer, int nBufSize = 0, int nFormat = 0, bool bCompressed = false);
#else
		int Load (ubyte *buffer);
#endif
		void Destroy (void);
		void Unlink (void);
		static void Wrap (int state);
		inline void Bind (void) { 
			if (m_info.bRenderBuffer)
				BindRenderBuffer ();
			else
				OGL_BINDTEX (m_info.handle); 
			}
		int BindRenderBuffer (void);

		inline int Index (void) { return m_info.index; }
		inline void SetIndex (int index) { m_info.index = index; }
		inline GLint Handle (void) { return (GLint) m_info.handle; }
		inline GLenum Format (void) { return m_info.format; }
		inline GLint InternalFormat (void) { return m_info.internalFormat; }
		inline GLfloat U (void) { return m_info.u; }
		inline GLfloat V (void) { return m_info.v; }
		inline int Width (void) { return m_info.w; }
		inline int Height (void) { return m_info.h; }
		inline int TW (void) { return m_info.tw; }
		inline int TH (void) { return m_info.th; }
		inline CBitmap* Bitmap (void) { return m_info.bmP; }
		inline ubyte IsRenderBuffer (void) { return m_info.bRenderBuffer; }

		inline void SetHandle (GLuint handle) { m_info.handle = handle; }
		inline void SetFormat (GLenum format) { m_info.format = format; }
		inline void SetInternalFormat (GLint internalFormat) { m_info.internalFormat = internalFormat; }
		inline void SetBitmap (CBitmap* bmP) { m_info.bmP = bmP; }
#if RENDER2TEXTURE == 1
		inline CPBO& PBO (void) { return m_info.pbo; }
		inline void SetRenderBuffer (CPBO *pbo);
#elif RENDER2TEXTURE == 2
		inline CFBO& FBO (void) { return m_info.fbo; }
		inline void SetRenderBuffer (CFBO *fbo);
#endif
		ubyte *Copy (int dxo, int dyo, ubyte *data);
		ubyte *Convert (int dxo, int dyo,  CBitmap *bmP, int nTransp, int bSuperTransp);
#if 0
		inline int Prev (void) { return m_prev; }
		inline int Next (void) { return m_next; }
		inline void SetPrev (int prev) { m_prev = prev; }
		inline void SetNext (int next) { m_next = next; }
#endif
#if TEXTURE_COMPRESSION
		Compress ();
#endif

	private:
		void SetSize (void);
		void SetBufSize (int dbits, int bits, int w, int h);
		int Verify (void);
		int FormatSupported (void);
	};

//------------------------------------------------------------------------------

class CTexturePool : public CDataPool< CTexture > {};

class CTextureManager {
	private:
		CTexture			m_info;
		CTexturePool	m_textures;

	public:
		CTextureManager () { Init (); }
		~CTextureManager () { Destroy (); }
		void Init (void);
		void Smash (void);
		void Destroy (void);
		inline CTexture *Pop (void) { return m_textures.Pop (); }
		void Push (CTexture *texP) { 
			texP->Destroy ();
			m_textures.Push (texP->Index ()); 
			}
		CTexture *Get (CBitmap *bmP);

	private:
		CTexture* GetFirst (void) { return m_textures.GetFirst (); }
		CTexture* GetNext (void) { return m_textures.GetNext (); }
	};

extern CTextureManager textureManager;

//------------------------------------------------------------------------------

tRgbColorf *BitmapColor (CBitmap *bmP, ubyte *bufP);

//------------------------------------------------------------------------------

extern int nOglMemTarget;

//------------------------------------------------------------------------------

#endif
