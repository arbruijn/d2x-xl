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

class CBitmap;
class CTextureManager;

typedef struct tTexture {
	int				index;
	GLuint	 		handle;
	GLint				internalFormat;
	GLenum			format;
	int 				w, h, tw, th, lw;
//	int 				bytesu;
//	int 				bytes;
	GLfloat			u, v;
	GLclampf			prio;
	ubyte				bMipMaps;
	char				bSmoothe;
	ubyte				bRenderBuffer;
	CBitmap*			bmP;
#if RENDER2TEXTURE == 1
	CPBO				pbo;
#elif RENDER2TEXTURE == 2
	CFBO				fbo;
#endif
} tTexture;

class CTexture {
	private:
#if 1
		uint			m_nRegistered;
#endif
		tTexture	m_info;

	public:
		CTexture () : m_nRegistered (0) { Init (); }
		~CTexture () { Destroy (); }
		GLuint Create (int w, int h);
		void Init (void);
		void Setup (int w, int h, int lw, int bpp  = 0, int bMask = 0, int bMipMap = 0, int bSmoothe = 0, CBitmap *bmP = NULL);
		int Prepare (bool bCompressed = false);
#if TEXTURE_COMPRESSION
		int Load (ubyte *buffer, int nBufSize = 0, int nFormat = 0, bool bCompressed = false);
#else
		int Load (ubyte *buffer);
#endif
		void Destroy (void);
		bool Register (uint i = 0);
		void Release (void);
		static void Wrap (int state);
		void Bind (void);
		int BindRenderBuffer (void);
		bool IsBound (void);

		inline int Index (void) { return m_info.index; }
		inline void SetIndex (int index) { m_info.index = index; }
		inline GLint Handle (void) { return GLint (m_info.handle); }
		inline GLenum Format (void) { return m_info.format; }
		inline GLint InternalFormat (void) { return m_info.internalFormat; }
		inline GLfloat U (void) { return m_info.u; }
		inline GLfloat V (void) { return m_info.v; }
		inline int Width (void) { return m_info.w; }
		inline int Height (void) { return m_info.h; }
		inline int TW (void) { return m_info.tw; }
		inline int TH (void) { return m_info.th; }
		inline uint Registered (void) { return m_nRegistered; }
		inline ubyte IsRenderBuffer (void) { return m_info.bRenderBuffer; }
		inline CBitmap* Bitmap (void) { return m_info.bmP; }
		inline void SetBitmap (CBitmap* bmP) { m_info.bmP = bmP; }

		inline void SetHandle (GLuint handle) { m_info.handle = handle; }
		inline void SetFormat (GLenum format) { m_info.format = format; }
		inline void SetInternalFormat (GLint internalFormat) { m_info.internalFormat = internalFormat; }
#if RENDER2TEXTURE == 1
		inline CPBO& PBO (void) { return m_info.pbo; }
		inline void SetRenderBuffer (CPBO *pbo);
#elif RENDER2TEXTURE == 2
		inline CFBO& FBO (void) { return m_info.fbo; }
		inline void SetRenderBuffer (CFBO *fbo);
#endif
		ubyte *Copy (int dxo, int dyo, ubyte *data);
		ubyte *Convert (int dxo, int dyo,  CBitmap *bmP, int nTransp, int bSuperTransp);
#if TEXTURE_COMPRESSION
		int Compress ();
#endif
		int Verify (void);
		bool Check (void);

	private:
		void SetSize (void);
		void SetBufSize (int dbits, int bits, int w, int h);
		int FormatSupported (void);
	};

//------------------------------------------------------------------------------

class CTexturePool : public CDataPool< CTexture > {};
class CTextureList : public CStack< CTexture* > {};

class CTextureManager {
	private:
		CTexture			m_info;
		CTextureList	m_textures;
		int				m_nTextures;

	public:
		CTextureManager () { Init (); }
		~CTextureManager () { Destroy (); }
		void Init (void);
		void Smash (void);
		void Destroy (void);
		uint Find (CTexture* texP);
		uint Register (CTexture* texP);
		bool Release (CTexture* texP);
		inline CTextureList Textures (void) { return m_textures; }
		bool Check (void);
		uint Check (CTexture* texP);
	};

extern CTextureManager textureManager;

//------------------------------------------------------------------------------

int Pow2ize (int v, int max = 4096);

extern int nOglMemTarget;

//------------------------------------------------------------------------------

#endif
