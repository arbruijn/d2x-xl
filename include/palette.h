/* $Id: palette.h,v 1.4 2004/05/12 22:06:02 btb Exp $ */
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

typedef struct tBGR {
	ubyte	b,g,r;
} tBGR;

typedef struct tRGB {
	ubyte	r,g,b;
} tRGB;

typedef ubyte	tPalette [256*3];

extern void GrSetPaletteGamma ( int gamma);
extern int GrGetPaletteGamma ();
extern void GrPaletteStepClear ();
extern int GrPaletteFadeOut (ubyte *pal, int nsteps, int allow_keys);
extern int GrPaletteFadeIn (ubyte *pal,int nsteps, int allow_keys);
extern int GrPaletteStepLoad ( ubyte * pal);
extern void gr_make_cthru_table (ubyte * table, ubyte r, ubyte g, ubyte b);
extern void gr_make_blend_table (ubyte *blend_table, ubyte r, ubyte g, ubyte b);
extern int GrFindClosestColorCurrent ( int r, int g, int b);
extern void GrPaletteRead (ubyte * palette);
extern void init_computed_colors (void);

void FullPaletteSave (void);
void PaletteRestore (void);
void GamePaletteStepUp (int r, int g, int b);

ubyte *FindPalette (ubyte *palP);
ubyte *AddPalette (ubyte *palP);
void FreePalettes (void);

#ifdef D1XD3D
typedef ubyte PALETTE [256 * 3];
extern void DoSetPalette (PALETTE *pPal);
#endif

extern ubyte *defaultPalette;
extern ubyte *fadePalette;
extern ubyte *gamePalette;
extern ubyte *starsPalette;

#endif //_PALETTE_H
