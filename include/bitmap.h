#ifndef _BITMAP_H
#define _BITMAP_H

#include "ogl_defs.h"
#include "ogl_texture.h"
#include "fbuffer.h"
#include "pbuffer.h"
#include "carray.h"

//-----------------------------------------------------------------------------

#define BM_LINEAR   0
#define BM_MODEX    1
#define BM_SVGA     2
#define BM_RGB15    3   //5 bits each r, g, b stored at 16 bits
#define BM_SVGA15   4
#define BM_OGL      5

#define BM_TYPE_STD		0
#define BM_TYPE_ALT		1
#define BM_TYPE_FRAME	2
#define BM_TYPE_MASK		4

#define BM_FLAG_TRANSPARENT         1
#define BM_FLAG_SUPER_TRANSPARENT   2
#define BM_FLAG_NO_LIGHTING         4
#define BM_FLAG_RLE                 8   // A run-length encoded bitmap.
#define BM_FLAG_PAGED_OUT           16  // This bitmap's data is paged out.
#define BM_FLAG_RLE_BIG             32  // for bitmaps that RLE to > 255 per row (i.e. cockpits)
#define BM_FLAG_SEE_THRU				64  // door or other texture containing see-through areas
#define BM_FLAG_TGA						128

#define BM_FRAMECOUNT(_bmP)	((_bmP)->m_info.frames.nCount)
#define BM_FRAMES(_bmP)			((_bmP)->m_info.frames)
#define BM_CURFRAME(_bmP)		((_bmP)->m_info.frames.currentP)
#define BM_OVERRIDE(_bmP)		((_bmP)->m_info.overrideP)
#define BM_MASK(_bmP)			((_bmP)->m_info.override.maskP)
#define BM_PARENT(_bmP)			((_bmP)->m_info.override.parentP)

#define MAX_BMP_SIZE(width, height) (4 + ((width) + 2) * (height))

//-----------------------------------------------------------------------------

typedef struct grsPoint {
	fix x, y;
} grsPoint;

//-----------------------------------------------------------------------------

class CBitmap;
class CTexture;
class CCanvas;

//-----------------------------------------------------------------------------

typedef struct tBmProps {
	short   x, y;		// Offset from parentP's origin
	short   w, h;		// width, height
	short   rowSize;	// ubyte offset to next row
	sbyte	  nMode;		// 0=Linear, 1=ModeX, 2=SVGA
	ubyte	  flags;		
} tBmProps;

class CFrameInfo {
	public:
		CBitmap*	bmP;
		CBitmap*	currentP;
		uint		nCount;
	};

//-----------------------------------------------------------------------------

class CBitmapInfo {
	public:
		char					szName [20];
		tBmProps				props;
		ubyte					flags;		
		ushort				nId;		
		tRgbColorb			avgColor;		
		ubyte					avgColorIndex;	
		ubyte					nBPP;
		ubyte					nType;
		ubyte					bWallAnim;
		ubyte					bFromPog;
		ubyte					bChild;
		ubyte					bFlat;		//no texture, just a colored area
		ubyte					nTeam;
#if TEXTURE_COMPRESSION
		ubyte					bCompressed;
		int					nFormat;
		int					nBufSize;
#endif
		int					transparentFrames [4];
		int					supertranspFrames [4];
		CBitmap*				parentP;
		CBitmap*				overrideP;
		CBitmap*				maskP;
		CFrameInfo			frames;
		CPalette*			palette;
		CTexture				texture;

	};

//-----------------------------------------------------------------------------

class CBitmap : public CArray< ubyte > {
	private:
		CBitmapInfo	m_info;

		GLfloat		x0, x1, y0, y1, u1, v1, u2, v2, aspect;
		GLint			depthFunc, bBlendState;
		int			nOrient;

	public:
		CBitmap () { Init (); };
		~CBitmap () { Destroy (); };
		static CBitmap* Create (ubyte mode, int w, int h, int bpp, const char* pszName = NULL);
		ubyte* CreateBuffer (void);
		bool Setup (ubyte mode, int w, int h, int bpp, const char* pszName, ubyte* buffer = NULL);
		void Destroy (void);
		void DestroyMask (void);
		void DestroyFrames (void);
		void DestroyBuffer (void);
		void Init (void);
		void Init (int mode, int x, int y, int w, int h, int bpp = 1, ubyte *buffer = NULL);
		bool InitChild (CBitmap *parentP, int x, int y, int w, int h);
		CBitmap* CreateChild (int x, int y, int w, int h);
		CBitmap* ReleaseTexture (CBitmap *bmP);
		void ReleaseTexture (void);

		inline CBitmap *NextFrame (void) {
			m_info.frames.currentP++;
			if (++m_info.frames.currentP >= m_info.frames.bmP + m_info.frames.nCount)
				m_info.frames.currentP = m_info.frames.bmP;
			return m_info.frames.currentP;
			}

		inline CBitmap *CurFrame (int iFrame) {
			if (m_info.nType != BM_TYPE_ALT)
				return this;
			if (iFrame < 0)
				return m_info.frames.currentP ? m_info.frames.currentP : this;
			return m_info.frames.bmP ? m_info.frames.currentP = m_info.frames.bmP + iFrame % m_info.frames.nCount : this;
			}

		inline CBitmap* HasParent (void)
		 { return (m_info.nType == BM_TYPE_STD) ? m_info.parentP :  NULL; } 
		inline CBitmap* HasOverride (void)
		 { return (m_info.nType == BM_TYPE_STD) ? m_info.overrideP :  m_info.frames.currentP; } 

		inline CBitmap *Override (int iFrame) {
			CBitmap *bmP = this;
			if (m_info.nType == BM_TYPE_STD) {
				if (!m_info.overrideP)
					return this;
				bmP = m_info.overrideP;
				}
			return bmP->CurFrame (iFrame);
			}

		void SetPalette (CPalette *palette, int transparentColor = -1, int supertranspColor = -1, int *freq = NULL);
		void Remap (CPalette *palette, int transparentColor, int superTranspColor);
		void SetTransparent (int bTransparent);
		void SetSuperTransparent (int bTransparent);
		void CheckTransparency (void);
		int HasTransparency (void);
		int AvgColor (tRgbColorf *colorP = NULL, bool bForce = true);
		inline tRgbColorb *GetAvgColor (void) { return &m_info.avgColor; }
		tRgbaColorf *GetAvgColor (tRgbaColorf *colorP);
		int AvgColorIndex (void);

		void Swap_0_255 (void);
		void RLESwap_0_255 (void);
		int RLERemap (ubyte *colorMap, int maxLen);
		int RLEExpand (ubyte *colorMap, int bSwap0255);
		int RLECompress (void);
		void ExpandTo (CBitmap *destP);
	
		inline ubyte FrameCount (void) { return ((Type () != BM_TYPE_ALT) && Parent ()) ? Parent ()->FrameCount () : m_info.frames.nCount; }
		inline CBitmap *Frames (void) { return (m_info.nType == BM_TYPE_ALT) ? m_info.frames.bmP : NULL; }
		inline CBitmap *CurFrame (void) { return m_info.frames.currentP; }
		inline CBitmap *Override (void) { return m_info.overrideP; }
		inline CBitmap *Mask (void) { return m_info.maskP; }
		inline CBitmap *Parent (void) { return m_info.parentP; }

		inline tBmProps* Props (void) { return &m_info.props; }
		inline ushort Id (void) { return m_info.nId; }
		inline void SetFrameCount (ubyte nFrameCount) { m_info.frames.nCount = nFrameCount; }
		inline void SetFrameCount (void) {  m_info.frames.nCount = m_info.props.h / m_info.props.w; }
		void SetParent (CBitmap *parentP) { m_info.parentP = parentP; }
		void SetMask (CBitmap *maskP) { m_info.maskP = maskP; }
		void SetOverride (CBitmap *override) { m_info.overrideP = override; }
		void SetCurFrame (CBitmap *frame) { m_info.frames.currentP = frame; }
		void SetCurFrame (int nFrame) { m_info.frames.currentP = m_info.frames.bmP + nFrame; }

		inline short Width (void) { return m_info.props.w; }
		inline short Height (void) { return m_info.props.h; }
		inline short Left (void) { return m_info.props.x; }
		inline short Top (void) { return m_info.props.y; }
		inline short Right (void) { return m_info.props.x + m_info.props.w; }
		inline short Bottom (void) { return m_info.props.y + m_info.props.h; }
		inline short RowSize (void) { return m_info.props.rowSize; }
		inline ubyte Flags (void) { return m_info.props.flags; }
		inline sbyte Mode (void) { return m_info.props.nMode; }
		inline ubyte BPP (void) { return m_info.nBPP; }
		inline ubyte Type (void) { return m_info.nType; }
		inline ubyte WallAnim (void) { return m_info.bWallAnim; }
		inline ubyte FromPog (void) { return m_info.bFromPog; }
		inline ubyte Flat (void) { return m_info.bFlat; }
		inline ubyte Team (void) { return m_info.nTeam; }
		inline CTexture* Texture (void) { return &m_info.texture; }
		inline int *TransparentFrames (int i = 0) { return m_info.transparentFrames + i; }
		inline int *SuperTranspFrames (int i = 0) { return m_info.supertranspFrames + i; }
		inline char* Name (void) { return m_info.szName; }
		inline int FrameSize (void) { return static_cast<int> (m_info.props.h) * static_cast<int> (m_info.props.rowSize); }
		inline void SetId (ushort nId) { m_info.nId = nId; }
		inline void SetName (const char* pszName) { if (pszName) strncpy (m_info.szName, pszName, sizeof (m_info.szName)); }
		inline void SetWidth (short w) { m_info.props.w = w; m_info.props.rowSize = w * m_info.nBPP; }
		inline void SetHeight (short h) { m_info.props.h = h; }
		inline void SetLeft (short x) { m_info.props.x = x; }
		inline void SetTop (short y) { m_info.props.y = y; }
		inline void SetRowSize (short rowSize) { m_info.props.rowSize = rowSize; }
		inline void SetFlags (ubyte flags) { m_info.props.flags = flags; }
		inline void AddFlags (ubyte flags) { m_info.props.flags |= flags; }
		inline void DelFlags (ubyte flags) { m_info.props.flags &= ~flags; }
		inline void SetMode (sbyte nMode) { m_info.props.nMode = nMode; }
		inline void SetBPP (ubyte nBPP) { m_info.nBPP = nBPP; m_info.props.rowSize = m_info.props.w * m_info.nBPP; }
		inline void SetType (ubyte nType) { m_info.nType = nType; }
		inline void SetWallAnim (ubyte bWallAnim) { m_info.bWallAnim = bWallAnim; }
		inline void SetFromPog (ubyte bFromPog) { m_info.bFromPog = bFromPog; }
		inline void SetFlat (ubyte bFlat) { m_info.bFlat = bFlat; }
		inline void SetTeam (ubyte nTeam) { m_info.nTeam = nTeam; }
		inline void SetAvgColorIndex (ubyte nIndex) { m_info.avgColorIndex = nIndex; }
		inline void SetAvgColor (tRgbColorb& color) { m_info.avgColor = color; }
		inline CPalette* Palette (void) { return m_info.palette ? m_info.palette : paletteManager.Default (); }

		CBitmap *CreateMask (void);
		int CreateMasks (void);
		int SetupFrames (int bMipMaps, int nTransp, int bLoad);
		CBitmap* SetupTexture (int bMipMaps, int nTransp, int bLoad);
		int LoadTexture (int dxo, int dyo, int nTransp, int superTransp);
#if RENDER2TEXTURE == 1
		int PrepareTexture (int bMipMap, int nTransp, int bMask, CBO *renderBuffer = NULL);
#elif RENDER2TEXTURE == 2
		int PrepareTexture (int bMipMap, int nTransp, int bMask, CFBO *renderBuffer = NULL);
#else
		int PrepareTexture (int bMipMap, int nTransp, int bMask, tPixelBuffer *renderBuffer = NULL);
#endif
		int Bind (int bMipMaps, int nTransp);
		inline bool Prepared (void) { return Texture () && (Texture ()->Handle () > 0); }

#if TEXTURE_COMPRESSION
		inline ubyte Compressed (void) { return m_info.bCompressed; }
		inline int Format (void) { return m_info.nFormat; }
		inline SetCompressed (ubyte bCompressed) { m_info.bCompressed = bCompressed; }
		inline void SetFormat (int nFormat) { m_info.nFormat = nFormat; }
#else
		inline ubyte Compressed (void) { return 0; }
		inline int Format (void) { return 0; }
#endif
#if 0
		void UnlinkTexture (void);
		void Unlink (int bAddon);
#endif
		void RenderFullScreen (void);
		int Render (CBitmap *dest, 
						int xDest, int yDest, int wDest, int hDest, 
						int xSrc, int ySrc, int wSrc, int hSrc, 
						int bTransp = 0, int bMipMaps = 0, int bSmoothe = 0, 
						float fAlpha = 1.0f, tRgbaColorf* colorP = NULL);
		inline void Render (CBitmap* dest, int bTransp = 0, int bMipMaps = 0, int bSmoothe = 0, float fAlpha = 1.0f)
		 { Render (dest, 0, 0, dest->Width (), dest->Height (), 0, 0, Width (), Height (), bTransp, bMipMaps, bSmoothe, fAlpha); }
		void RenderStretched (CBitmap* dest = NULL, int x = 0, int y = 0);
		void RenderFixed (CBitmap* dest = NULL, int x = 0, int y = 0, int w = 0, int h = 0);

		void Blit (CBitmap* dest, int dx, int dy, int w, int h, int sx, int sy, int bTransp);
		void BlitClipped (CBitmap* dest = NULL, int dx = 0, int dy = 0, int w = -1, int h = -1, int sx = 0, int sy = 0);
		void BlitClipped (int xSrc, int ySrc);
		void BlitScaled (CBitmap* destP);
		void ScreenCopy (CBitmap* dest, int dx, int dy, int w, int h, int sx, int sy);

		void OglVertices (int x, int y, int w = 0, int h = 0, int scale = I2X (1), int orient = 0, CBitmap* destP = NULL);
		void OglTexCoord (void);
		void SetTexCoord (GLfloat u, GLfloat v, int orient);
		CTexture* OglBeginRender (bool bBlend, int bMipMaps, int nTransp);
		void OglRender (tRgbaColorf* colorP, int nColors, int orient);
		void OglEndRender (void);
		int RenderScaled (int x, int y, int w = 0, int h = 0, int scale = I2X (1), int orient = 0, tCanvasColor *colorP = NULL);

		inline bool Clip (int x, int y) { return (x < 0) || (y < 0) || (x >= Width ()) || (y >= Width ()); }
		void DrawPixel (int x, int y, ubyte color);
		ubyte GetPixel (int x, int y);

		inline CBitmap& Clone (CBitmap& clone) { 
			memcpy (&clone, this, sizeof (CBitmap)); 
			return clone;
			}

		inline CBitmap& operator= (CBitmap& source) { 
			source.Clone (*this); 
			source.ShareBuffer (*this);
			return *this;
			}

		void FreeData (void);
		void FreeMask (void);
		int FreeHiresFrame (int bD1);
		int FreeHiresAnimation (int bD1);
		void Unload (int i, int bD1);

	};

inline int operator- (CBitmap* o, CArray<CBitmap>& a) { return a.Index (o); }
	
//-----------------------------------------------------------------------------

void LoadGameBackground (void);
void GrBitmapM (int x, int y, CBitmap *bmP, int bTransp);
void GrBmUBitBltM (int w, int h, int dx, int dy, int sx, int sy, CBitmap * src, CBitmap * dest, int bTransp);

//-----------------------------------------------------------------------------

#endif //_BITMAP_H

//eof
