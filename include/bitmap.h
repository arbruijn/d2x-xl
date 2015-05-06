#ifndef _BITMAP_H
#define _BITMAP_H

#include "carray.h"
#include "color.h"
#include "palette.h"
#include "rectangle.h"

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
#define BM_FLAG_OPAQUE					256

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

#include "ogl_defs.h"
#include "ogl_texture.h"

//-----------------------------------------------------------------------------

typedef struct tBmProps {
	int16_t   x, y;		// Offset from parentP's origin
	int16_t   w, h;		// width, height
	int16_t   rowSize;	// uint8_t offset to next row
	uint16_t  flags;
	int8_t	  nMode;		// 0=Linear, 1=ModeX, 2=SVGA
} tBmProps;

class CBitmapFrameInfo {
	public:
		CBitmap*		bmP;
		CBitmap*		currentP;
		int32_t		nCurrent;
		uint32_t		nCount;
	};

//-----------------------------------------------------------------------------

#if TEXTURE_COMPRESSION

class CBitmapCompressionData {
	public:
		CArray< uint8_t >	buffer;
		bool					bCompressed;
		int32_t					nWidth;
		int32_t					nHeight;
		int32_t					nFormat;
		int32_t					nBufSize;
	};

#endif

class CBitmapInfo {
	public:
		char					szName [FILENAME_LEN];
		tBmProps				props;
		uint8_t				flags;
		uint16_t				nId;
		CRGBColor			avgColor;
		uint8_t				avgColorIndex;
		uint8_t				nBPP;
		uint8_t				nType;
		uint8_t				bWallAnim;
		uint8_t				bFromPog;
		uint8_t				bChild;
		uint8_t				bFlat;		//no texture, just a colored area
		uint8_t				bStatic;		//must remain in RAM
		uint8_t				bSetup;
		char					nMasks;
		uint8_t				nTeam;
		uint8_t				nBlendMode;	//0: alpha, 1: additive
		uint8_t				bCartoonize;
#if TEXTURE_COMPRESSION
		CBitmapCompressionData	compressed;
#endif
		int32_t				nTranspType;
		int32_t				transparentFrames [4];
		int32_t				supertranspFrames [4];
		CBitmap*				parentP;
		CBitmap*				overrideP;
		CBitmap*				maskP;
		CBitmapFrameInfo	frames;
		CPalette*			palette;
		CTexture				texture;
		CTexture*			texP;
		tTexCoord2f*		texCoordP;
		CFloatVector*		colorP;
		int32_t				nColors;
	};

class CBitmapRenderData {
	public:
		GLfloat		x0, x1, y0, y1, u1, v1, u2, v2, aspect;
		GLint			depthFunc, bBlendState;
		int32_t			nOrient;

	CBitmapRenderData() { memset (this, 0, sizeof (*this)); }
	};

//-----------------------------------------------------------------------------

class CBitmap : public CArray< uint8_t > {
	public:
		CBitmapInfo			m_info;
		CBitmapRenderData	m_render;

	public:
		CBitmap ();
		~CBitmap ();
		static CBitmap* Create (uint8_t mode, int32_t w, int32_t h, int32_t bpp, const char* pszName = NULL);
		uint8_t* CreateBuffer (void);
		bool Setup (uint8_t mode, int32_t w, int32_t h, int32_t bpp, const char* pszName, uint8_t* buffer = NULL);
		void Destroy (void);
		void DestroyMask (void);
		void DestroyFrames (void);
		void DestroyBuffer (void);
		void Reset (void);
		void Init (void);
		void Init (int32_t mode, int32_t x, int32_t y, int32_t w, int32_t h, int32_t bpp = 1, uint8_t *buffer = NULL, bool bReset = false);
		bool InitChild (CBitmap *parentP, int32_t x, int32_t y, int32_t w, int32_t h);
		CBitmap* CreateChild (int32_t x, int32_t y, int32_t w, int32_t h);
		CBitmap* ReleaseTexture (CBitmap *bmP);
		void ReleaseTexture (void);

		inline CBitmap *NextFrame (void) {
			if (++m_info.frames.nCurrent >= int32_t (m_info.frames.nCount))
				m_info.frames.nCurrent = 0;
			m_info.frames.currentP = m_info.frames.bmP + m_info.frames.nCurrent;
			return m_info.frames.currentP;
			}

		inline CBitmap *CurFrame (int32_t iFrame) {
			if (m_info.nType != BM_TYPE_ALT)
				return this;
			if (iFrame < 0)
				return m_info.frames.currentP ? m_info.frames.currentP : this;
			m_info.frames.nCurrent = m_info.frames.nCount ? int32_t (iFrame % m_info.frames.nCount) : 0;
			return m_info.frames.bmP ? m_info.frames.currentP = m_info.frames.bmP + m_info.frames.nCurrent : this;
			}

		inline CBitmap* HasParent (void)
		 { return (m_info.nType == BM_TYPE_STD) ? m_info.parentP :  NULL; }
		inline CBitmap* HasOverride (void)
		 { return (m_info.nType == BM_TYPE_STD) ? m_info.overrideP : m_info.frames.currentP; }

		inline CBitmap *Override (int32_t iFrame) {
			CBitmap *bmP = this;
			if (m_info.nType == BM_TYPE_STD) {
				if (!m_info.overrideP)
					return this;
				bmP = m_info.overrideP;
				}
			return bmP->CurFrame (iFrame);
			}

		inline void SetColor (CFloatVector* colorP = NULL, int32_t nColors = 1) { m_info.colorP = colorP, m_info.nColors = nColors; }
		inline CFloatVector* GetColor (int32_t* nColors = NULL) { 
			if (nColors)
				*nColors = m_info.nColors; 
			return m_info.colorP; 
			}
		inline void SetTexCoord (tTexCoord2f* texCoordP = NULL) { m_info.texCoordP = texCoordP; }
		inline tTexCoord2f* GetTexCoord (void) { return m_info.texCoordP; }
		void SetPalette (CPalette *palette, int32_t transparentColor = -1, int32_t supertranspColor = -1, uint8_t* bufP = NULL, int32_t bufLen = 0);
		void SetTransparent (int32_t bTransparent);
		void SetSuperTransparent (int32_t bTransparent);
		void CheckTransparency (void);
		int32_t HasTransparency (void);
		int32_t AvgColor (CFloatVector3 *colorP = NULL, bool bForce = true);
		inline CRGBColor *GetAvgColor (void) { return &m_info.avgColor; }
		CFloatVector *GetAvgColor (CFloatVector *colorP);
		int32_t AvgColorIndex (void);

		void SwapTransparencyColor (void);
		void RLESwapTransparencyColor (void);
		int32_t RLERemap (uint8_t *colorMap, int32_t maxLen);
		int32_t RLEExpand (uint8_t *colorMap, int32_t bSwapTranspColor);
		int32_t RLECompress (void);
		void ExpandTo (CBitmap *destP);

		inline uint8_t FrameCount (void) { return ((m_info.nType != BM_TYPE_ALT) && Parent ()) ? m_info.parentP->FrameCount () : m_info.frames.nCount; }
		inline uint8_t FrameIndex (void) { return m_info.frames.nCurrent; }
		inline CBitmap *Frames (void) { return (m_info.nType == BM_TYPE_ALT) ? m_info.frames.bmP : NULL; }

		inline CBitmap *CurFrame (void) { return m_info.frames.currentP; }
		inline CBitmap *Override (void) { return m_info.overrideP; }
		inline CBitmap *Mask (void) { return m_info.maskP; }
		inline CBitmap *Parent (void) { return m_info.parentP; }

		inline tBmProps* Props (void) { return &m_info.props; }
		inline uint16_t Id (void) { return m_info.nId; }
		inline void SetRenderStyle (uint8_t bCartoonize) { m_info.bCartoonize = bCartoonize; }
		inline void SetFrameCount (uint8_t nFrameCount) { m_info.frames.nCount = nFrameCount; }
		inline void SetFrameCount (void) {  m_info.frames.nCount = m_info.props.h / m_info.props.w; }
		void SetParent (CBitmap *parentP) { m_info.parentP = parentP; }
		void SetMask (CBitmap *maskP) { m_info.maskP = maskP; }
		CBitmap* SetOverride (CBitmap *overrideP);
		CBitmap* SetCurFrame (CBitmap *frameP) {
			m_info.frames.nCurrent = int32_t ((m_info.frames.currentP = frameP) - m_info.frames.bmP);
			return m_info.frames.currentP;
			}
		CBitmap* SetCurFrame (int32_t nFrame) {
			if ((m_info.nType != BM_TYPE_ALT) || !m_info.frames.bmP)
				return this;
			m_info.frames.currentP = m_info.frames.bmP + (m_info.frames.nCurrent = nFrame);
			return m_info.frames.currentP;
			}

		inline int16_t Width (void) { return m_info.props.w; }
		inline int16_t Height (void) { return m_info.props.h; }
		inline int16_t Left (void) { return m_info.props.x; }
		inline int16_t Top (void) { return m_info.props.y; }
		inline int16_t Right (void) { return Left () + Width (); }
		inline int16_t Bottom (void) { return Top () + Height (); }

		inline int16_t RowSize (void) { return m_info.props.rowSize; }
		inline uint16_t Flags (void) { return m_info.props.flags; }
		inline int8_t Mode (void) { return m_info.props.nMode; }
		inline uint8_t BPP (void) { return m_info.nBPP; }
		inline uint8_t Type (void) { return m_info.nType; }
		inline uint8_t WallAnim (void) { return m_info.bWallAnim; }
		inline uint8_t FromPog (void) { return m_info.bFromPog; }
		inline uint8_t Flat (void) { return m_info.bFlat; }
		inline uint8_t Static (void) { return m_info.bStatic; }
		inline uint8_t Team (void) { return m_info.nTeam; }
		inline uint8_t BlendMode (void) { return m_info.nBlendMode; }
		inline CTexture* Texture (void) { return m_info.texP; }
		inline int32_t *TransparentFrames (int32_t i = 0) { return m_info.transparentFrames + i; }
		inline int32_t *SuperTranspFrames (int32_t i = 0) { return m_info.supertranspFrames + i; }
		inline char* Name (void) { return m_info.szName; }
		inline int32_t FrameSize (void) { return static_cast<int32_t> (m_info.props.h) * static_cast<int32_t> (m_info.props.rowSize); }
		inline void SetKey (uint16_t nId) { m_info.nId = nId; }
		void SetName (const char* pszName);
		inline void SetWidth (int16_t w) { m_info.props.w = w; m_info.props.rowSize = w * m_info.nBPP; }
		inline void SetHeight (int16_t h) { m_info.props.h = h; }
		inline void SetLeft (int16_t x) { m_info.props.x = x; }
		inline void SetTop (int16_t y) { m_info.props.y = y; }
		inline void SetRowSize (int16_t rowSize) { m_info.props.rowSize = rowSize; }
		inline void SetFlags (uint16_t flags) { m_info.props.flags = flags; }
		inline void AddFlags (uint16_t flags) { m_info.props.flags |= flags; }
		inline void DelFlags (uint16_t flags) { m_info.props.flags &= ~flags; }
		inline void SetMode (int8_t nMode) { m_info.props.nMode = nMode; }
		inline void SetBPP (uint8_t nBPP) { m_info.nBPP = nBPP; m_info.props.rowSize = m_info.props.w * m_info.nBPP; }
		inline void SetType (uint8_t nType) { m_info.nType = nType; }
		inline void SetStatic (uint8_t bStatic) { m_info.bStatic = bStatic; }
		inline void SetWallAnim (uint8_t bWallAnim) { m_info.bWallAnim = bWallAnim; }
		inline void SetFromPog (uint8_t bFromPog) { m_info.bFromPog = bFromPog; }
		inline void SetFlat (uint8_t bFlat) { m_info.bFlat = bFlat; }
		inline void SetTeam (uint8_t nTeam) { m_info.nTeam = nTeam; }
		inline void SetBlendMode (uint8_t nBlendMode) { m_info.nBlendMode = nBlendMode; }
		inline void SetAvgColorIndex (uint8_t nIndex) { m_info.avgColorIndex = nIndex; }
		inline void SetAvgColor (CRGBColor& color) { m_info.avgColor = color; }
		inline void SetTranspType (int32_t nTranspType) { m_info.nTranspType = ((m_info.nBPP > 1) ? -1 : nTranspType); }
		inline void SetupTexture (void) {
			m_info.texP = &m_info.texture;
			m_info.texture.SetBitmap (this);
			}
		inline void SetTexture (CTexture *texP) { m_info.texP = texP; }
		inline void ResetTexture (void) { m_info.texP = &m_info.texture; }
		void NeedSetup (void);
		inline CPalette* Palette (void) { return m_info.palette ? m_info.palette : paletteManager.Default (); }

		inline void GetExtent (int32_t& x, int32_t& y, int32_t& w, int32_t& h) {
			x = Left ();
			y = Top ();
			w = Width ();
			h = Height ();
			}

		CBitmap *CreateMask (void);
		int32_t CreateMasks (void);
		int32_t CreateFrames (int32_t bMipMaps, int32_t bLoad);
		bool SetupFrames (int32_t bMipMaps, int32_t bLoad);
		bool SetupTexture (int32_t bMipMaps, int32_t bLoad);
		int32_t LoadTexture (int32_t dxo, int32_t dyo, int32_t superTransp);
#if RENDER2TEXTURE == 1
		int32_t PrepareTexture (int32_t bMipMap, int32_t bMask, CBO *renderBuffer = NULL);
#elif RENDER2TEXTURE == 2
		int32_t PrepareTexture (int32_t bMipMap, int32_t bMask, CFBO *renderBuffer = NULL);
#else
		int32_t PrepareTexture (int32_t bMipMap, int32_t bMask, tPixelBuffer *renderBuffer = NULL);
#endif
		int32_t Bind (int32_t bMipMaps);
		inline bool IsBound (void) { return m_info.texP && m_info.texP->IsBound (); }
		inline bool Prepared (void) { return m_info.texP && m_info.texP->Handle (); }

#if TEXTURE_COMPRESSION
		inline CArray<uint8_t>& CompressedBuffer (void) { return m_info.compressed.buffer; }
		inline uint8_t Compressed (void) { return m_info.compressed.bCompressed; }
		inline int32_t Format (void) { return m_info.compressed.nFormat; }
		inline void SetCompressed (bool bCompressed) { m_info.compressed.bCompressed = bCompressed; }
		inline void SetFormat (int32_t nFormat) { m_info.compressed.nFormat = nFormat; }
		inline uint32_t CompressedSize (void) { return m_info.compressed.buffer.Size (); }
		int32_t SaveS3TC (const char *pszFolder, const char *pszFilename);
		int32_t ReadS3TC (const char *pszFolder, const char *pszFilename);
#else
		inline uint8_t Compressed (void) { return 0; }
		inline int32_t Format (void) { return 0; }
#endif
#if 0
		void UnlinkTexture (void);
		void Unlink (int32_t bAddon);
#endif
		void RenderFullScreen (void);
		int32_t Render (CRectangle* dest,
						int32_t xDest, int32_t yDest, int32_t wDest, int32_t hDest,
						int32_t xSrc, int32_t ySrc, int32_t wSrc, int32_t hSrc,
						int32_t bTransp = 0, int32_t bMipMaps = 0, int32_t bSmoothe = 0,
						float fAlpha = 1.0f, CFloatVector* colorP = NULL);
		inline void Render (CRectangle* destP, int32_t bTransp = 0, int32_t bMipMaps = 0, int32_t bSmoothe = 0, float fAlpha = 1.0f)
			{ Render (destP, 0, 0, destP->Width (), destP->Height (), 0, 0, Width (), Height (), bTransp, bMipMaps, bSmoothe, fAlpha); }
		void RenderStretched (CRectangle* dest = NULL, int32_t x = 0, int32_t y = 0);
		void RenderFixed (CRectangle* dest = NULL, int32_t x = 0, int32_t y = 0, int32_t w = 0, int32_t h = 0);

		void Blit (CBitmap* dest, int32_t dx, int32_t dy, int32_t w, int32_t h, int32_t sx, int32_t sy, int32_t bTransp);
		void BlitClipped (CBitmap* dest = NULL, int32_t dx = 0, int32_t dy = 0, int32_t w = -1, int32_t h = -1, int32_t sx = 0, int32_t sy = 0);
		void BlitClipped (int32_t xSrc, int32_t ySrc);
		void BlitScaled (CBitmap* destP);
		void ScreenCopy (CBitmap* dest, int32_t dx, int32_t dy, int32_t w, int32_t h, int32_t sx, int32_t sy);

		void OglVertices (int32_t x, int32_t y, int32_t w = 0, int32_t h = 0, int32_t scale = I2X (1), int32_t orient = 0, CRectangle* destP = NULL);
		void OglTexCoord (void);
		void SetTexCoord (GLfloat u, GLfloat v, int32_t orient);
		void SetTexCoord (GLfloat u, GLfloat v, int32_t orient, tTexCoord2f& texCoord);
		CTexture* OglBeginRender (bool bBlend, int32_t bMipMaps, int32_t nTranspType);
		void OglRender (CFloatVector* colorP, int32_t nColors, int32_t orient);
		void OglEndRender (void);
		int32_t RenderScaled (int32_t x, int32_t y, int32_t w = 0, int32_t h = 0, int32_t scale = I2X (1), int32_t orient = 0, CCanvasColor *colorP = NULL, int32_t bSmoothe = 1);

		inline bool Clip (int32_t x, int32_t y) { return (x < 0) || (y < 0) || (x >= Width ()) || (y >= Width ()); }
		void DrawPixel (int32_t x, int32_t y, uint8_t color);
		uint8_t GetPixel (int32_t x, int32_t y);

		inline CBitmap& Clone (CBitmap& clone) {
			memcpy (&clone, this, sizeof (CBitmap));
			clone.m_info.texP = &clone.m_info.texture;
			clone.Texture ()->SetBitmap (&clone);
			return clone;
			}

		inline CBitmap& Copy (CBitmap& source) {
			memcpy (this, &source, sizeof (CBitmap));
			source.ShareBuffer (*this);
			return *this;
			}

		void FreeData (void);
		void FreeMask (void);
		int32_t FreeHiresFrame (int32_t bD1);
		int32_t FreeHiresAnimation (int32_t bD1);
		void Unload (int32_t i, int32_t bD1);

		operator const CRectangle() {
			CRectangle rc (Left (), Top (), Width (), Height ()); 
			return rc;
			}
	};

inline int32_t operator- (CBitmap* o, CArray<CBitmap>& a) { return a.Index (o); }

//-----------------------------------------------------------------------------

CFloatVector3 *BitmapColor (CBitmap *bmP, uint8_t *bufP);
void LoadGameBackground (void);
void GrBitmapM (int32_t x, int32_t y, CBitmap *bmP, int32_t bTransp);
void GrBmUBitBltM (int32_t w, int32_t h, int32_t dx, int32_t dy, int32_t sx, int32_t sy, CBitmap * src, CBitmap * dest, int32_t bTransp);

//-----------------------------------------------------------------------------

//#error ++++++++++++++++++++++ BITMAP_H +++++++++++++++++++++++++

#endif //_BITMAP_H

//eof
