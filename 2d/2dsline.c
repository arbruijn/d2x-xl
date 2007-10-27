/* $Id: 2dsline.c,v 1.10 2003/04/29 08:05:41 btb Exp $ */
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>

#include "u_mem.h"

#include "gr.h"
#include "grdef.h"
#include "error.h"

#include "inferno.h"
#include "ogl_defs.h"

#if !defined(NO_ASM) && defined(__WATCOMC__)

void gr_linear_darken( ubyte * dest, int darkeningLevel, int count, ubyte * fade_table );
#pragma aux gr_linear_darken parm [edi] [eax] [ecx] [edx] modify exact [eax ebx ecx edx edi] = \
"               xor ebx, ebx                "   \
"               mov bh, al                  "   \
"gld_loop:      mov bl, [edi]               "   \
"               mov al, [ebx+edx]           "   \
"               mov [edi], al               "   \
"               inc edi                     "   \
"               dec ecx                     "   \
"               jnz gld_loop                "

#elif !defined(NO_ASM) && defined(__GNUC__)

static inline void gr_linear_darken( ubyte * dest, int darkeningLevel, int count, ubyte * fade_table ) {
   int dummy[4];
   __asm__ __volatile__ (
"               xorl %%ebx, %%ebx;"
"               movb %%al, %%bh;"
"0:             movb (%%edi), %%bl;"
"               movb (%%ebx, %%edx), %%al;"
"               movb %%al, (%%edi);"
"               incl %%edi;"
"               decl %%ecx;"
"               jnz 0b"
   : "=D" (dummy[0]), "=a" (dummy[1]), "=c" (dummy[2]), "=d" (dummy[3])
   : "0" (dest), "1" (darkeningLevel), "2" (count), "3" (fade_table)
   : "%ebx");
}

#elif !defined(NO_ASM) && defined(_MSC_VER)

__inline void gr_linear_darken( ubyte * dest, int darkeningLevel, int count, ubyte * fade_table )
{
	__asm {
    mov edi,[dest]
    mov eax,[darkeningLevel]
    mov ecx,[count]
    mov edx,[fade_table]
    xor ebx, ebx
    mov bh, al
gld_loop:
    mov bl,[edi]
    mov al,[ebx+edx]
    mov [edi],al
    inc edi
    dec ecx
    jnz gld_loop
	}
}

#else // Unknown compiler, or no assembler. So we use C.

void gr_linear_darken(ubyte * dest, int darkeningLevel, int count, ubyte * fade_table) {
	register int i;

	for (i=0;i<count;i++)
	{
		*dest = fade_table[(*dest)+(darkeningLevel*256)];
		dest++;
	}
}

#endif

#ifdef NO_ASM // No Assembler. So we use C.
#if 0
void gr_linear_stosd( ubyte * dest, ubyte color, unsigned short count )
{
	int i, x;

	if (count > 3) {
		while ((int)(dest) & 0x3) { *dest++ = color; count--; };
		if (count >= 4) {
			x = (color << 24) | (color << 16) | (color << 8) | color;
			while (count > 4) { *(int *)dest = x; dest += 4; count -= 4; };
		}
		while (count > 0) { *dest++ = color; count--; };
	} else {
		for (i=0; i<count; i++ )
			*dest++ = color;
	}
}
#else
void gr_linear_stosd( ubyte * dest, grsColor *color, unsigned int nbytes) 
{
memset(dest, color->index, nbytes);
}
#endif
#endif

void gr_uscanline( int x1, int x2, int y )
{
	if (gameStates.render.grAlpha >= GR_ACTUAL_FADE_LEVELS ) {
		switch(TYPE)
		{
		case BM_LINEAR:
		case BM_OGL:
			OglULineC(x1, y, x2, y, &COLOR);
			break;
		}
	} else {
		switch(TYPE)
		{
		case BM_LINEAR:
		case BM_OGL:
			gr_linear_darken( DATA + ROWSIZE*y + x1, (int) gameStates.render.grAlpha, x2-x1+1, grFadeTable);
			break;
		}
	}
}

void GrScanLine( int x1, int x2, int y )
{
	if ((y<0)||(y>MAXY)) return;

	if (x2 < x1 ) x2 ^= x1 ^= x2;

	if (x1 > MAXX) return;
	if (x2 < MINX) return;

	if (x1 < MINX) x1 = MINX;
	if (x2 > MAXX) x2 = MAXX;

	if (gameStates.render.grAlpha >= GR_ACTUAL_FADE_LEVELS ) {
		switch(TYPE)
		{
		case BM_LINEAR:
		case BM_OGL:
			gr_linear_stosd( DATA + ROWSIZE*y + x1, &COLOR, x2-x1+1);
			break;
		}
	} else {
		switch(TYPE)
		{
		case BM_LINEAR:
		case BM_OGL:
			gr_linear_darken( DATA + ROWSIZE*y + x1, (int) gameStates.render.grAlpha, x2-x1+1, grFadeTable);
			break;
		}
	}
}
