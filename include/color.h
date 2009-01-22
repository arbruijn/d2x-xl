#ifndef _COLOR_H
#define _COLOR_H

//-----------------------------------------------------------------------------

typedef struct tRgbColord {
	double red;
	double green;
	double blue;
} tRgbColord;

typedef struct tRgbaColord {
	double red;
	double green;
	double blue;
	double alpha;
} tRgbaColord;

typedef struct tRgbColorf {
	float red;
	float green;
	float blue;
} tRgbColorf;

typedef struct tRgbaColorf {
	float red;
	float green;
	float blue;
	float	alpha;
} tRgbaColorf;

typedef struct tRgbaColorb {
	ubyte	red, green, blue, alpha;
} tRgbaColorb;

typedef struct tRgbColorb {
	ubyte	red, green, blue;
} tRgbColorb;

typedef struct tRgbColors {
	sbyte red, green, blue;
} tRgbColors;

typedef struct tFaceColor {
	tRgbaColorf	color;
	char			index;
} tFaceColor;

typedef struct tCanvasColor {
	short       index;       // current color
	ubyte			rgb;
	tRgbaColorb	color;
} tCanvasColor;

//-----------------------------------------------------------------------------

#ifdef MACDATA
#	define SWAP_0_255              // swap black and white
#	define DEFAULT_TRANSPARENCY_COLOR  0 // palette entry of transparency color -- 255 on the PC
#	define TRANSPARENCY_COLOR_STR  "0"
#else
/* #undef  SWAP_0_255 */        // no swapping for PC people
#	define DEFAULT_TRANSPARENCY_COLOR  255 // palette entry of transparency color -- 255 on the PC
#	define TRANSPARENCY_COLOR_STR  "255"
#endif /* MACDATA */

#define TRANSPARENCY_COLOR  gameData.render.transpColor // palette entry of transparency color -- 255 on the PC
#define SUPER_TRANSP_COLOR  254   // palette entry of super transparency color

#define RGBA(_r,_g,_b,_a)			(((uint) (_r) << 24) | ((uint) (_g) << 16) | ((uint) (_b) << 8) | ((uint) (_a)))
#define RGBA_RED(_i)					(((uint) (_i) >> 24) & 0xff)
#define RGBA_GREEN(_i)				(((uint) (_i) >> 16) & 0xff)
#define RGBA_BLUE(_i)				(((uint) (_i) >> 8) & 0xff)
#define RGBA_ALPHA(_i)				(((uint) (_i)) & 0xff)
#define PAL2RGBA(_c)					((ubyte) (((uint) (_c) * 255) / 63))
#define RGBA_PAL(_r,_g,_b)			RGBA (PAL2RGBA (_r), PAL2RGBA (_g), PAL2RGBA (_b), 255)
#define RGBA_PALX(_r,_g,_b,_x)	RGBA_PAL ((_r) * (_x), (_g) * (_x), (_b) * (_x))
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

#define D2BLUE_RGBA			RGBA_PAL (35,35,55)

//-----------------------------------------------------------------------------

static inline void CountColors (ubyte *data, int i, int *freq)
{
for (; i; i--)
	freq [*data++]++;
}

//-----------------------------------------------------------------------------

#endif //_COLOR_H
