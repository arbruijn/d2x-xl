/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _PALETTE_H
#define _PALETTE_H

#define DEFAULT_LEVEL_PALETTE "groupa.256" //don't confuse with DEFAULT_PALETTE
#define D1_PALETTE "palette.256"

#include "cstack.h"
#include "color.h"
//#include "cfile.h"

#define PALETTE_SIZE				256
#define MAX_COMPUTED_COLORS	64
#define MAX_FADE_LEVELS			34
#define FADE_LEVELS				31
#define FADE_RATE					16		//	gots to be a power of 2, else change the code in DiminishPaletteTowardsNormal

typedef struct tBGR {
	uint8_t	b,g,r;
} __pack__ tBGR;

typedef struct tRGB {
	uint8_t	r,g,b;
} __pack__ tRGB;

typedef union tPalette {
	uint8_t			raw [PALETTE_SIZE * 3];
	CRGBColor	rgb [PALETTE_SIZE];
} __pack__ tPalette;

class CComputedColor : public CRGBColor {
	public:
		uint8_t			nIndex;
	};

class CPalette {
	private:
		tPalette			m_data;
		CComputedColor	m_computedColors [MAX_COMPUTED_COLORS];
		int16_t				m_nComputedColors;
		int32_t				m_transparentColor;
		int32_t				m_superTranspColor;

	public:
		CPalette () : m_transparentColor (-1), m_superTranspColor (-1) {}
		~CPalette () {}
		void Init (int32_t nTransparentColor = -1, int32_t nSuperTranspColor = -1);
		void InitTransparency (int32_t nTransparentColor = -1, int32_t nSuperTranspColor = -1);
		void ClearStep ();
		bool Read (CFile& cf);
		bool Write (CFile& cf);
		int32_t ClosestColor (int32_t r, int32_t g, int32_t b);
		inline int32_t ClosestColor (CRGBColor* colorP)
		 { return ClosestColor ((int32_t) colorP->r, (int32_t) colorP->g, (int32_t) colorP->b); }
		inline int32_t ClosestColor (CFloatVector3* colorP)
		 { return ClosestColor ((int32_t) (colorP->Red () * 63.0f), (int32_t) (colorP->Green () * 63.0f), (int32_t) (colorP->Blue () * 63.0f)); }
		void SwapTransparency (void);
		void AddComputedColor (int32_t r, int32_t g, int32_t b, int32_t nIndex);
		void InitComputedColors (void);
		void ToRgbaf (uint8_t nColor, CFloatVector& color);

		inline tPalette& Data (void) { return m_data; }
		inline CRGBColor *Color (void) { return m_data.rgb; }
		inline uint8_t *Raw (void) { return m_data.raw; }
		inline void Skip (CFile& cf) { cf.Seek (sizeof (m_data), SEEK_CUR); }
		inline size_t Size (void) { return sizeof (m_data); }
		inline void SetBlack (uint8_t r, uint8_t g, uint8_t b) 
		 { m_data.rgb [0].r = r, m_data.rgb [0].g = g, m_data.rgb [0].b = b; }
		inline void SetTransparency (uint8_t r, uint8_t g, uint8_t b) 
		 { m_data.rgb [PALETTE_SIZE - 1].r = r, m_data.rgb [PALETTE_SIZE - 1].g = g, m_data.rgb [PALETTE_SIZE - 1].b = b; }
		inline CPalette& operator= (CPalette& source) { 
			Init ();
			memcpy (&Data (), &source.Data (), sizeof (tPalette));
			m_transparentColor = source.TransparentColor ();
			m_superTranspColor = source.SuperTranspColor ();
			return *this;
			}
		inline bool operator== (CPalette& source) { 
			return (TransparentColor () == source.TransparentColor ()) && 
					 (SuperTranspColor () == source.SuperTranspColor ()) &&
					 (memcmp (&Data (), &source.Data (), sizeof (tPalette)) == 0); 
			}
		inline CRGBColor& operator[] (const int32_t i) { return m_data.rgb [i]; }
		inline int32_t TransparentColor (void) { return m_transparentColor; }
		inline int32_t SuperTranspColor (void) { return m_superTranspColor; }
	};

//------------------------------------------------------------------------------

typedef struct tPaletteList {
	struct tPaletteList	*next;
	CPalette					palette;
} tPaletteList;

//------------------------------------------------------------------------------

class CPaletteData {
	public:
		CPalette*			deflt;
		CPalette*			fade;
		CPalette*			game;
		CPalette*			last;
		CPalette*			D1;
		CPalette*			texture;
		CPalette*			current;
		CPalette*			prev;
		tPaletteList*		list;
		char					szLastLoaded [FILENAME_LEN];
		char					szLastPig [FILENAME_LEN];
		int32_t					nPalettes;
		int32_t					nGamma;
		int32_t					nLastGamma;
		fix					xFadeDelay;
		fix					xFadeDuration [2];
		fix					xLastDelay;
		fix					xLastEffectTime;

		uint8_t					fadeTable [PALETTE_SIZE * MAX_FADE_LEVELS];
		CFloatVector3		flash;
		CFloatVector3		effect;
		CFloatVector3		lastEffect;
		bool					bDoEffect;
		int32_t					nSuspended;
	};


class CPalette;

class CPaletteManager {
	private:
		CPaletteData	m_data;

		CStack< CPalette* > m_save;

	public:
		CPaletteManager () { Init (); };
		~CPaletteManager () { Destroy (); }
		void Init (void);
		void Destroy (void); 
		CPalette* Find (CPalette& palette);
		CPalette* Add (CPalette& palette, int32_t nTransparentColor = -1, int32_t nSuperTranspColor = -1);
		CPalette* Add (uint8_t* buffer, int32_t nTransparentColor = -1, int32_t nSuperTranspColor = -1);
		CPalette* Load (const char *pszPaletteName, const char *pszLevelName, int32_t nUsedForLevel, int32_t bNoScreenChange, int32_t bForce);
		CPalette* Load (const char* filename, const char* levelname);
		CPalette* LoadD1 (void);
		int32_t FindClosestColor15bpp (int32_t rgb);
		void SetGamma (int32_t gamma);
		int32_t GetGamma (void);
		const float Brightness (void);
		const char* BrightnessLevel (void);
		void Flash (void);

		void ClearStep (void);
		void StepUp (int32_t r, int32_t g, int32_t b);
		void StepUp (void);
		void RenderEffect (void);
		void FadeEffect (void);
		void ResetEffect (void);
		void SaveEffect (void);
		void SuspendEffect (bool bCond = true);
		void ResumeEffect (bool bCond = true);
		void SetEffect (int32_t r, int32_t g, int32_t b, bool bForce = false);
		void SetEffect (float r, float g, float b, bool bForce = false);
		void BumpEffect (int32_t r, int32_t g, int32_t b);
		void BumpEffect (float r, float g, float b);
		void UpdateEffect (void);
		inline void SetRedEffect (int32_t color) { m_data.effect.Red () = float (color) / 64.0f; }
		inline void SetGreenEffect (int32_t color) { m_data.effect.Green () = float (color) / 64.0f; }
		inline void SetBlueEffect (int32_t color) { m_data.effect.Blue () = float (color) / 64.0f; }
		void SetEffect (bool bForce = false);
		void ClearEffect (void);
		int32_t ClearEffect (CPalette* palette);
		int32_t EnableEffect (bool bReset = false);
		int32_t DisableEffect (void);
		bool EffectEnabled (void) { return m_data.nSuspended <= 0; }
		bool EffectDisabled (void) { return m_data.nSuspended > 0; }
		inline bool FadedOut (void) { return m_data.nSuspended <= 0; }
		void SetPrev (CPalette* palette) { m_data.prev = palette; }

		 void Push (void) { 
			m_save.Push (m_data.current); 
			}
		 void Pop (void) { 
			m_data.current = m_save.Pop (); 
			}

		inline CPalette* Activate (CPalette* palette) {
			if (palette && (m_data.current != palette)) {
				m_data.current = palette;
				m_data.current->Init ();
				}
			return m_data.current;
			}

		inline int8_t RedEffect (bool bFade = false) { return int8_t (m_data.effect.Red () * 64.0f * (bFade ? FadeScale () : 1.0f)); }
		inline int8_t GreenEffect (bool bFade = false) { return int8_t (m_data.effect.Green () * 64.0f * (bFade ? FadeScale () : 1.0f)); }
		inline int8_t BlueEffect (bool bFade = false) { return int8_t (m_data.effect.Blue () * 64.0f * (bFade ? FadeScale () : 1.0f)); }
		inline void SetRedEffect (int8_t color) {  m_data.effect.Red () = float (color) / 64.0f; }
		inline void SetGreenEffect (int8_t color) {  m_data.effect.Green () = float (color) / 64.0f; }
		inline void SetBlueEffect (int8_t color) {  m_data.effect.Blue () = float (color) / 64.0f; }
		inline void SetFadeDelay (fix xDelay) { m_data.xFadeDelay = xDelay; }
		inline void SetFadeDuration (fix xDuration) { m_data.xFadeDuration [0] = m_data.xFadeDuration [1] = xDuration; }
		inline void SetLastEffectTime (fix xTime) { m_data.xLastEffectTime = xTime; }
		inline fix FadeDelay (void) { return m_data.xFadeDelay; }
		inline fix LastEffectTime (void) { return m_data.xLastEffectTime; }

		inline void SetDefault (CPalette* palette) { m_data.deflt = palette; }
		inline void SetCurrent (CPalette* palette) { m_data.current = palette; }
		inline void SetGame (CPalette* palette) { m_data.game = palette; }
		inline void SetFade (CPalette* palette) { m_data.fade = palette; }
		inline void SetTexture (CPalette* palette) { m_data.texture = palette; }
		inline void SetD1 (CPalette* palette) { m_data.D1 = palette; }

		inline CPalette* Default (void) { return m_data.deflt; }
		inline CPalette* Current (void) { return m_data.current ? m_data.current : m_data.deflt; }
		inline CPalette* Game (void) { return m_data.game ? m_data.game : Current (); }
		inline CPalette* Texture (void) { return m_data.texture ? m_data.texture : Current (); };
		inline CPalette* D1 (void) { return m_data.D1 ? m_data.D1 : Current (); }
		inline CPalette* Fade (void) { return m_data.fade; }
		inline uint8_t* FadeTable (void) { return m_data.fadeTable; }
		inline bool DoEffect (void) { return m_data.bDoEffect; }

		inline char* LastLoaded (void) { return m_data.szLastLoaded; }
		inline char* LastPig (void) { return m_data.szLastPig; }
		inline void SetLastLoaded (const char *name) { strncpy (m_data.szLastLoaded, name, sizeof (m_data.szLastLoaded)); }
		inline void SetLastPig (const char *name) { strncpy (m_data.szLastPig, name, sizeof (m_data.szLastPig)); }

		inline int32_t ClosestColor (int32_t r, int32_t g, int32_t b)
		 { return m_data.current ? m_data.current->ClosestColor (r, g, b) : 0; }

		inline float FadeScale (void) { return m_data.xFadeDuration [1] ? (float) m_data.xFadeDuration [0] / (float) m_data.xFadeDuration [1] : 0.0f; }

	};

extern CPaletteManager paletteManager;
extern char szCurrentLevelPalette [FILENAME_LEN];

#endif //_PALETTE_H
