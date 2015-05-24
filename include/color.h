#ifndef _COLOR_H
#define _COLOR_H

//-----------------------------------------------------------------------------

class CRGBColor {
	public:
		uint8_t	r, g, b;

	inline uint8_t& Red (void) { return r; }
	inline uint8_t& Green (void) { return g; }
	inline uint8_t& Blue (void) { return b; }

	inline void Set (uint8_t red, uint8_t green, uint8_t blue) {
		r = red, g = green, b = blue;
		}

	inline void ToGrayScale (int32_t bWeighted = 0) {
		if (bWeighted)
			r = g = b = (uint8_t) FRound (((float) r + (float) g + (float) b) / 3.0f);
		else
			r = g = b = (uint8_t) FRound ((float) r * 0.30f + (float) g * 0.584f + (float) b * 0.116f);
		}

	inline uint8_t Posterize (int32_t nColor, int32_t nSteps) {
		return Max (0, ((nColor + nSteps / 2) / nSteps) * nSteps - nSteps);
		}

	inline void Posterize (int32_t nSteps = 15) {
		r = Posterize (r, nSteps);
		g = Posterize (g, nSteps);
		b = Posterize (b, nSteps);
		}

	inline void Saturate (int32_t nMethod = 1) {
		uint8_t m = Max (r, Max (g, b));
		if (nMethod) {
			float s = 255.0f / float (m);
			r = uint8_t (float (r) * s);
			g = uint8_t (float (g) * s);
			b = uint8_t (float (b) * s);
			}
		else {
			if ((m = 255 - m)) {
				r += m;
				g += m;
				b += m;
				}
			}
		}

	inline void Assign (CRGBColor& other) { r = other.r, g = other.g, b = other.b;	}
	};

//-----------------------------------------------------------------------------

class CRGBAColor {
	public:
		uint8_t	r, g, b, a;

	inline void Set (uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) {
		r = red, g = green, b = blue, a = alpha;
		}

	inline uint8_t& Red (void) { return r; }
	inline uint8_t& Green (void) { return g; }
	inline uint8_t& Blue (void) { return b; }
	inline uint8_t& Alpha (void) { return a; }

	inline void ToGrayScale (int32_t bWeighted = 0) {
		if (bWeighted)
			r = g = b = (uint8_t) FRound (((float) r + (float) g + (float) b) / 3.0f);
		else
			r = g = b = (uint8_t) FRound ((float) r * 0.30f + (float) g * 0.584f + (float) b * 0.116f);
		}

	inline uint8_t Posterize (int32_t nColor, int32_t nSteps) {
		return Max (0, ((nColor + nSteps / 2) / nSteps) * nSteps - nSteps);
		}

	inline void Posterize (int32_t nSteps = 15) {
		r = Posterize (r, nSteps);
		g = Posterize (g, nSteps);
		b = Posterize (b, nSteps);
		}

	inline void Saturate (int32_t nMethod = 1) {
		uint8_t m = Max (r, Max (g, b));
		if (nMethod) {
			float s = 255.0f / float (m);
			r = uint8_t (float (r) * s);
			g = uint8_t (float (g) * s);
			b = uint8_t (float (b) * s);
			}
		else {
			if ((m = 255 - m)) {
				r += m;
				g += m;
				b += m;
				}
			}
		}

	inline void Assign (CRGBAColor& other) { r = other.r, g = other.g, b = other.b, a = other.a;	}
	};

//-----------------------------------------------------------------------------

class CFaceColor : public CFloatVector {
	public:
		char	index;

	explicit CFaceColor () : index (1) { Set (1.0f, 1.0f, 1.0f, 1.0f); }
	};

class CCanvasColor : public CRGBAColor {
	public:
		int16_t				index;       // current color
		uint8_t				rgb;
	};

template <uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha>
class CStaticFaceColor : public CFaceColor {
	public:
		explicit CStaticFaceColor () { 
			Set (float (red) / 255.0f, float (green) / 255.0f, float (blue) / 255.0f, float (alpha) / 255.0f); 
			index = 1;
			}
	};

template <uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha>
class CStaticCanvasColor : public CCanvasColor {
	public:
		explicit CStaticCanvasColor () { 
			Set (red, green, blue, alpha); 
			index = -1;
			rgb = 1;
			}
	};

//-----------------------------------------------------------------------------

#if 1

#	define DEFAULT_TRANSPARENCY_COLOR  255 // palette entry of transparency color -- 255 on the PC

#else

#ifdef MACDATA
#	define SWAP_TRANSPARENCY_COLOR       // swap black and transparency color codes in palettes
#	define DEFAULT_TRANSPARENCY_COLOR  0 // palette entry of transparency color -- 255 on the PC
#else
//#undef  SWAP_TRANSPARENCY_COLOR         // no swapping for PC people
#	define DEFAULT_TRANSPARENCY_COLOR  255 // palette entry of transparency color -- 255 on the PC
#endif /* MACDATA */

#endif

#define TRANSPARENCY_COLOR			gameData.render.transpColor // palette entry of transparency color -- 255 on the PC
#define SUPER_TRANSP_COLOR			254   // palette entry of super transparency color

#define RGBA(_r,_g,_b,_a)			((uint32_t (_r) << 24) | (uint32_t (_g) << 16) | (uint32_t (_b) << 8) | (uint32_t (_a)))
#define RGBA_RED(_i)					((uint32_t (_i) >> 24) & 0xff)
#define RGBA_GREEN(_i)				((uint32_t (_i) >> 16) & 0xff)
#define RGBA_BLUE(_i)				((uint32_t (_i) >> 8) & 0xff)
#define RGBA_ALPHA(_i)				((uint32_t (_i)) & 0xff)
#define PAL2RGBA(_c)					(((_c) >= 63) ? 255 : uint8_t ((uint32_t (_c) * 255) / 63))
#define RGBA_PAL(_r,_g,_b,_a)		RGBA (PAL2RGBA (_r), PAL2RGBA (_g), PAL2RGBA (_b), _a)
#define RGB_PAL(_r,_g,_b)			RGBA_PAL (_r, _g, _b, 255)
#define RGBA_PALX(_r,_g,_b,_x)	RGB_PAL ((_r) * (_x), (_g) * (_x), (_b) * (_x))
#define RGBA_PAL3(_r,_g,_b)		RGBA_PALX (_r, _g, _b, 3)
#define RGBA_PAL2(_r,_g,_b)		RGBA_PALX (_r, _g, _b, 2)
#define RGBA_FADE(_c,_f)			RGBA (RGBA_RED (_c) / (_f), RGBA_GREEN (_c) / (_f), RGBA_BLUE (_c) / (_f), RGBA_ALPHA (_c))

#define WHITE_RGBA					RGBA (255,255,255,255)
#define GRAY_RGBA						RGBA (128,128,128,255)
#define DKGRAY_RGBA					RGBA (80,80,80,255)
#define BLACK_RGBA					RGBA (0,0,0,255)
#define RED_RGBA						RGBA (255,0,0,255)
#define MEDRED_RGBA					RGBA (128,0,0,255)
#define DKRED_RGBA					RGBA (80,0,0,255)
#define GREEN_RGBA					RGBA (0,255,0,255)
#define MEDGREEN_RGBA				RGBA (0,128,0,255)
#define DKGREEN_RGBA					RGBA (0,80,0,255)
#define BLUE_RGBA						RGBA (0,0,255,255)
#define MEDBLUE_RGBA					RGBA (0,0,128,255)
#define DKBLUE_RGBA					RGBA (0,0,80,255)
#define ORANGE_RGBA					RGBA (255,128,0,255)
#define GOLD_RGBA						RGBA (255,224,0,255)
#define CYAN_RGBA						RGBA (0,255,255,255)

#define D2BLUE_RGBA					RGB_PAL (35,35,55)

//-----------------------------------------------------------------------------

static inline void CountColors (uint8_t *data, int32_t i, int32_t *freq)
{
if (data)
	for (; i; i--)
		freq [*data++]++;
}

//-----------------------------------------------------------------------------

#endif //_COLOR_H
