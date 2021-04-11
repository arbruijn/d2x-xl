//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_TEXTURE_H
#define _OGL_TEXTURE_H

#ifdef _WIN32
#	pragma pack(push)
#	pragma pack(8)
#	include <windows.h>
#	pragma pack(pop)
#	include <stddef.h>
#endif
#include "ogl_defs.h"
#include "fbuffer.h"
#include "pbuffer.h"
#include "cdatapool.h"

//------------------------------------------------------------------------------

class CBitmap;
class CTextureManager;

typedef struct tTexture {
	int32_t				index;
	GLuint	 		handle;
	GLint				internalFormat;
	GLenum			format;
	int32_t 				w, h, tw, th, lw;
//	int32_t 				bytesu;
//	int32_t 				bytes;
	GLfloat			u, v;
	GLclampf			prio;
	uint8_t				bMipMaps;
	char				bSmoothe;
	uint8_t				bRenderBuffer;
	CBitmap*			pBm;
#if RENDER2TEXTURE == 1
	CPBO				pbo;
#elif RENDER2TEXTURE == 2
	CFBO				fbo;
#endif
} tTexture;

class CTexture {
	private:
#if 1
		uint32_t			m_nRegistered;
#endif
		tTexture	m_info;

	public:
		CTexture () : m_nRegistered (0) { Init (); }
		~CTexture () { Destroy (); }
		GLuint Create (int32_t w, int32_t h);
		void Init (void);
		void Setup (int32_t w, int32_t h, int32_t lw, int32_t bpp  = 0, int32_t bMask = 0, int32_t bMipMap = 0, int32_t bSmoothe = 0, CBitmap *pBm = NULL);
		int32_t Prepare (bool bCompressed = false);
#if TEXTURE_COMPRESSION
		int32_t Load (uint8_t *buffer, int32_t nBufSize = 0, int32_t nFormat = 0, bool bCompressed = false);
#else
		int32_t Load (uint8_t *buffer);
#endif
		void Destroy (void);
		bool Register (uint32_t i = 0);
		void Release (void);
		static void Wrap (int32_t state);
		void Bind (void);
		int32_t BindRenderBuffer (void);
		bool IsBound (void);

		inline int32_t Index (void) { return m_info.index; }
		inline void SetIndex (int32_t index) { m_info.index = index; }
		inline GLint Handle (void) { return GLint (m_info.handle); }
		inline GLenum Format (void) { return m_info.format; }
		inline GLint InternalFormat (void) { return m_info.internalFormat; }
		inline GLfloat U (void) { return m_info.u; }
		inline GLfloat V (void) { return m_info.v; }
		inline int32_t Width (void) { return m_info.w; }
		inline int32_t Height (void) { return m_info.h; }
		inline int32_t TW (void) { return m_info.tw; }
		inline int32_t TH (void) { return m_info.th; }
		inline uint32_t Registered (void) { return m_nRegistered; }
		inline uint8_t IsRenderBuffer (void) { return m_info.bRenderBuffer; }
		inline CBitmap* Bitmap (void) { return m_info.pBm; }
		inline void SetBitmap (CBitmap* pBm) { m_info.pBm = pBm; }

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
		uint8_t *Copy (int32_t dxo, int32_t dyo, uint8_t *data);
		uint8_t *Convert (int32_t dxo, int32_t dyo,  CBitmap *pBm, int32_t nTransp, int32_t bSuperTransp, int32_t& bpp);
#if TEXTURE_COMPRESSION
		int32_t Compress ();
#endif
		int32_t Verify (void);
		bool Check (void);

	private:
		void SetSize (void);
		void SetBufSize (int32_t dbits, int32_t bits, int32_t w, int32_t h);
		int32_t FormatSupported (void);
	};

//------------------------------------------------------------------------------

class CTexturePool : public CDataPool< CTexture > {};
class CTextureList : public CStack< CTexture* > {};

class CTextureManager {
	private:
		CTexture			m_info;
		CTextureList	m_textures;
		int32_t				m_nTextures;

	public:
		CTextureManager () { Init (); }
		~CTextureManager () { Destroy (); }
		void Init (void);
		void Smash (void);
		void Destroy (void);
		uint32_t Find (CTexture* pTexture);
		uint32_t Register (CTexture* pTexture);
		bool Release (CTexture* pTexture);
		inline CTextureList Textures (void) { return m_textures; }
		bool Check (void);
		uint32_t Check (CTexture* pTexture);
	};

extern CTextureManager textureManager;

//------------------------------------------------------------------------------

int32_t Pow2ize (int32_t v, int32_t max = 4096);

extern int32_t nOglMemTarget;

//------------------------------------------------------------------------------

#endif
