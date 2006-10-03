/* $Id: bitblt.c,v 1.13 2003/12/08 21:21:16 btb Exp $ */
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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Routines for bitblt's.
 *
 * Old Log:
 * Revision 1.29  1995/03/14  12:14:28  john
 * Added code to double horz/vert bitblts.
 *
 * Revision 1.28  1995/03/13  09:01:48  john
 * Fixed bug with VFX1 screen not tall enough.
 *
 * Revision 1.27  1995/03/01  15:38:10  john
 * Better ModeX support.
 *
 * Revision 1.26  1994/12/15  12:19:00  john
 * Added GrBmBitBlt (clipped!) function.
 *
 * Revision 1.25  1994/12/09  18:58:42  matt
 * Took out include of 3d.h
 *
 * Revision 1.24  1994/11/28  17:08:32  john
 * Took out some unused functions in linear.asm, moved
 * gr_linear_movsd from linear.asm to bitblt.c, made sure that
 * the code in ibiblt.c sets the direction flags before rep movsing.
 *
 * Revision 1.22  1994/11/23  16:04:00  john
 * Fixed generic rle'ing to use new bit method.
 *
 * Revision 1.21  1994/11/18  22:51:03  john
 * Changed a bunch of shorts to ints in calls.
 *
 * Revision 1.20  1994/11/10  15:59:48  john
 * Fixed bugs with canvas's being created with bogus bm_props.flags.
 *
 * Revision 1.19  1994/11/09  21:03:35  john
 * Added RLE for svga gr_ubitmap.
 *
 * Revision 1.18  1994/11/09  17:41:29  john
 * Made a slow version of rle bitblt to svga, modex.
 *
 * Revision 1.17  1994/11/09  16:35:15  john
 * First version with working RLE bitmaps.
 *
 * Revision 1.16  1994/11/04  10:06:58  john
 * Added fade table for fading fonts. Made font that partially clips
 * not print a warning message.
 *
 * Revision 1.15  1994/09/22  16:08:38  john
 * Fixed some palette stuff.
 *
 * Revision 1.14  1994/09/19  11:44:27  john
 * Changed call to allocate selector to the dpmi module.
 *
 * Revision 1.13  1994/08/08  13:03:00  john
 * Fixed bug in GrBitmap in modex
 *
 * Revision 1.12  1994/07/13  19:47:23  john
 * Fixed bug with modex bitblt to page 2 not working.
 *
 * Revision 1.11  1994/05/31  11:10:52  john
 * *** empty log message ***
 *
 * Revision 1.10  1994/03/18  15:24:34  matt
 * Removed interlace stuff
 *
 * Revision 1.9  1994/02/18  15:32:20  john
 * *** empty log message ***
 *
 * Revision 1.8  1994/02/01  13:22:54  john
 * *** empty log message ***
 *
 * Revision 1.7  1994/01/13  08:28:25  mike
 * Modify rect copy to copy alternate scanlines when in interlaced mode.
 *
 * Revision 1.6  1993/12/28  12:09:46  john
 * added lbitblt.asm
 *
 * Revision 1.5  1993/10/26  13:18:09  john
 * *** empty log message ***
 *
 * Revision 1.4  1993/10/15  16:23:30  john
 * y
 *
 * Revision 1.3  1993/09/13  17:52:58  john
 * Fixed bug in BitBlt linear to SVGA
 *
 * Revision 1.2  1993/09/08  14:47:00  john
 * Made bitmap00 add rowsize instead of bitmap width.
 * Other routines might have this problem too.
 *
 * Revision 1.1  1993/09/08  11:43:01  john
 * Initial revision
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "pa_enabl.h"                   //$$POLY_ACC
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "rle.h"
#include "mono.h"
#include "byteswap.h"       // because of rle code that has short for row offsets
#include "inferno.h"
#include "bitmap.h"
#include "ogl_init.h"
#include "error.h"

#ifdef OGL
#include "ogl_init.h"
#endif

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

int gr_bitblt_dest_step_shift = 0;
int gr_bitblt_double = 0;
ubyte *grBitBltFadeTable=NULL;

extern void gr_vesa_bitmap(grs_bitmap * source, grs_bitmap * dest, int x, int y);

void gr_linear_movsd(ubyte * source, ubyte * dest, unsigned int nbytes);
// This code aligns edi so that the destination is aligned to a dword boundry before rep movsd

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux gr_linear_movsd parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx] = \
" cld "					\
" mov		ebx, ecx	"	\
" mov		eax, edi"	\
" and		eax, 011b"	\
" jz		d_aligned"	\
" mov		ecx, 4"		\
" sub		ecx, eax"	\
" sub		ebx, ecx"	\
" rep		movsb"		\
" d_aligned: "			\
" mov		ecx, ebx"	\
" shr		ecx, 2"		\
" rep 	movsd"		\
" mov		ecx, ebx"	\
" and 	ecx, 11b"	\
" rep 	movsb";

#elif !defined(NO_ASM) && defined(__GNUC__)

inline void gr_linear_movsd(ubyte *src, ubyte *dest, unsigned int num_pixels) {
	int dummy[3];
 __asm__ __volatile__ (
" cld;"
" movl      %%ecx, %%ebx;"
" movl      %%edi, %%eax;"
" andl      $3, %%eax;"
" jz        0f;"
" movl      $4, %%ecx;"
" subl      %%eax,%%ecx;"
" subl      %%ecx,%%ebx;"
" rep;      movsb;"
"0: ;"
" movl      %%ebx, %%ecx;"
" shrl      $2, %%ecx;"
" rep;      movsl;"
" movl      %%ebx, %%ecx;"
" andl      $3, %%ecx;"
" rep;      movsb"
 : "=S" (dummy[0]), "=D" (dummy[1]), "=c" (dummy[2])
 : "0" (src), "1" (dest), "2" (num_pixels)
 :	"%eax", "%ebx");
}

#elif !defined(NO_ASM) && defined(_MSC_VER)

__inline void gr_linear_movsd(ubyte *src, ubyte *dest, unsigned int num_pixels)
{
 __asm {
   mov esi, [src]
   mov edi, [dest]
   mov ecx, [num_pixels]
   cld
   mov ebx, ecx
   mov eax, edi
   and eax, 011b
   jz d_aligned
   mov ecx, 4
   sub ecx, eax
   sub ebx, ecx
   rep movsb
d_aligned:
   mov ecx, ebx
   shr ecx, 2
   rep movsd
   mov ecx, ebx
   and ecx, 11b
   rep movsb
 }
}

#else // NO_ASM or unknown compiler

//------------------------------------------------------------------------------

#define THRESHOLD   8

#ifdef RELEASE
#define test_byteblit   0
#else
ubyte test_byteblit = 0;
#endif

void gr_linear_movsd(ubyte * src, ubyte * dest, unsigned int num_pixels)
{
	unsigned int i;
	uint n, r;
	double *d, *s;
	ubyte *d1, *s1;

// check to see if we are starting on an even byte boundry
// if not, move appropriate number of bytes to even
// 8 byte boundry

	if ((num_pixels < THRESHOLD) || (((size_t)src & 0x7) != ((size_t)dest & 0x7)) || test_byteblit) {
		for (i = 0; i < num_pixels; i++)
			*dest++ = *src++;
		return;
	}

	i = 0;
	if ((r = (uint) ((size_t)src & 0x7))) {
		for (i = 0; i < 8 - r; i++)
			*dest++ = *src++;
	}
	num_pixels -= i;

	n = num_pixels / 8;
	r = num_pixels % 8;
	s = (double *)src;
	d = (double *)dest;
	for (i = 0; i < n; i++)
		*d++ = *s++;
	s1 = (ubyte *)s;
	d1 = (ubyte *)d;
	for (i = 0; i < r; i++)
		*d1++ = *s1++;
}

#endif	//#ifdef NO_ASM

//------------------------------------------------------------------------------

static void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, unsigned int num_pixels);

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux gr_linear_rep_movsdm parm [esi] [edi] [ecx] modify exact [ecx esi edi eax] = \
"nextpixel:"                \
    "mov    al,[esi]"       \
    "inc    esi"            \
    "cmp    al, " TRANSPARENCY_COLOR_STR   \
    "je skip_it"            \
    "mov    [edi], al"      \
"skip_it:"                  \
    "inc    edi"            \
    "dec    ecx"            \
    "jne    nextpixel";

#elif !defined(NO_ASM) && defined(__GNUC__)

static inline void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, unsigned int num_pixels) {
	int dummy[3];
 __asm__ __volatile__ (
"0: ;"
" movb  (%%esi), %%al;"
" incl  %%esi;"
" cmpb  $" TRANSPARENCY_COLOR_STR ", %%al;"
" je    1f;"
" movb  %%al,(%%edi);"
"1: ;"
" incl  %%edi;"
" decl  %%ecx;"
" jne   0b"
 : "=S" (dummy[0]), "=D" (dummy[1]), "=c" (dummy[2])
 : "0" (src), "1" (dest), "2" (num_pixels)
 : "%eax");
}

#elif !defined(NO_ASM) && defined(_MSC_VER)

__inline void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, unsigned int num_pixels)
{
 __asm {
  nextpixel:
  mov esi, [src]
  mov edi, [dest]
  mov ecx, [num_pixels]
  mov al,  [esi]
  inc esi
  cmp al,  TRANSPARENCY_COLOR
  je skip_it
  mov [edi], al
  skip_it:
  inc edi
  dec ecx
  jne nextpixel
 }
}

#else

static void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, unsigned int num_pixels)
{
	unsigned int i;
	for (i=0; i<num_pixels; i++) {
		if (*src != TRANSPARENCY_COLOR)
			*dest = *src;
		dest++;
		src++;
	}
}

#endif

//------------------------------------------------------------------------------

static void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, unsigned int num_pixels, 
													ubyte fade_value, ubyte *srcPalette, ubyte *destPalette);

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux gr_linear_rep_movsdm_faded parm [esi] [edi] [ecx] [ebx] modify exact [ecx esi edi eax ebx] = \
"  xor eax, eax"                    \
"  mov ah, bl"                      \
"nextpixel:"                        \
    "mov    al,[esi]"               \
    "inc    esi"                    \
    "cmp    al, " TRANSPARENCY_COLOR_STR     \
    "je skip_it"                    \
    "mov  al, grFadeTable[eax]"   \
    "mov    [edi], al"              \
"skip_it:"                          \
    "inc    edi"                    \
    "dec    ecx"                    \
    "jne    nextpixel";

#elif !defined(NO_ASM) && defined(__GNUC__)

/* #pragma aux gr_linear_rep_movsdm_faded parm [esi] [edi] [ecx] [ebx] modify exact [ecx esi edi eax ebx] */
static inline void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, unsigned int num_pixels, ubyte fade_value) {
	int dummy[4];
 __asm__ __volatile__ (
"  xorl   %%eax, %%eax;"
"  movb   %%bl, %%ah;"
"0:;"
"  movb   (%%esi), %%al;"
"  incl   %%esi;"
"  cmpb   $" TRANSPARENCY_COLOR_STR ", %%al;"
"  je 1f;"
#ifdef __ELF__
"  movb   grFadeTable(%%eax), %%al;"
#else
"  movb   _gr_fade_table(%%eax), %%al;"
#endif
"  movb   %%al, (%%edi);"
"1:"
"  incl   %%edi;"
"  decl   %%ecx;"
"  jne    0b"
 : "=S" (dummy[0]), "=D" (dummy[1]), "=c" (dummy[2]), "=b" (dummy[3])
 : "0" (src), "1" (dest), "2" (num_pixels), "3" (fade_value)
 : "%eax");
}

#elif !defined(NO_ASM) && defined(_MSC_VER)

__inline void gr_linear_rep_movsdm_faded(void * src, void * dest, unsigned int num_pixels, ubyte fade_value)
{
 __asm {
  mov esi, [src]
  mov edi, [dest]
  mov ecx, [num_pixels]
  movzx ebx, byte ptr [fade_value]
  xor eax, eax
  mov ah, bl
  nextpixel:
  mov al, [esi]
  inc esi
  cmp al, TRANSPARENCY_COLOR
  je skip_it
  mov al, grFadeTable[eax]
  mov [edi], al
  skip_it:
  inc edi
  dec ecx
  jne nextpixel
 }
}

#else

static void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, unsigned int num_pixels, 
													ubyte fade_value, ubyte *srcPalette, ubyte *destPalette)
{
	int	i;
	short c;
	ubyte *fade_base;
	float fade = (float) fade_value / 31.0f;

	if (!destPalette)
		destPalette = srcPalette;
	fade_base = grFadeTable + (fade_value * 256);
	for (i=num_pixels; i != 0; i--) {
		c= (short) *src;
		if ((ubyte) c != (ubyte) TRANSPARENCY_COLOR) {
#if 1
			c *= 3;
			c = GrFindClosestColor (destPalette, 
											(ubyte) (srcPalette [c] * fade + 0.5), 
											(ubyte) (srcPalette [c + 1] * fade + 0.5), 
											(ubyte) (srcPalette [c + 2] * fade + 0.5));
			*dest = (ubyte) c;
#else
				*dest = fade_base [c];
#endif
			}
		dest++;
		src++;
	}
}

#endif

//------------------------------------------------------------------------------

void gr_linear_rep_movsd_2x(ubyte *src, ubyte *dest, unsigned int num_dest_pixels);

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux gr_linear_rep_movsd_2x parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx] = \
    "shr    ecx, 1"             \
    "jnc    nextpixel"          \
    "mov    al, [esi]"          \
    "mov    [edi], al"          \
    "inc    esi"                \
    "inc    edi"                \
    "cmp    ecx, 0"             \
    "je done"                   \
"nextpixel:"                    \
    "mov    al,[esi]"           \
    "mov    ah, al"             \
    "mov    [edi], ax"          \
    "inc    esi"                \
    "inc    edi"                \
    "inc    edi"                \
    "dec    ecx"                \
    "jne    nextpixel"          \
"done:"

#elif !defined(NO_ASM) && defined (__GNUC__)

inline void gr_linear_rep_movsd_2x(ubyte *src, ubyte *dest, unsigned int num_dest_pixels)
{
/* #pragma aux gr_linear_rep_movsd_2x parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx] */
	int dummy[3];
 __asm__ __volatile__ (
    "shrl   $1, %%ecx;"
    "jnc    0f;"
    "movb   (%%esi), %%al;"
    "movb   %%al, (%%edi);"
    "incl   %%esi;"
    "incl   %%edi;"
    "cmpl   $0, %%ecx;"
    "je 1f;"
"0: ;"
    "movb   (%%esi), %%al;"
    "movb   %%al, %%ah;"
    "movw   %%ax, (%%edi);"
    "incl   %%esi;"
    "incl   %%edi;"
    "incl   %%edi;"
    "decl   %%ecx;"
    "jne    0b;"
"1:"
 : "=S" (dummy[0]), "=D" (dummy[1]), "=c" (dummy[2])
 : "0" (src), "1" (dest), "2" (num_dest_pixels)
 :      "%eax");
}

#elif !defined(NO_ASM) && defined(_MSC_VER)

__inline void gr_linear_rep_movsd_2x(ubyte *src, ubyte *dest, unsigned int num_dest_pixels)
{
 __asm {
  mov esi, [src]
  mov edi, [dest]
  mov ecx, [num_dest_pixels]
  shr ecx, 1
  jnc nextpixel
  mov al, [esi]
  mov [edi], al
  inc esi
  inc edi
  cmp ecx, 0
  je done
nextpixel:
  mov al, [esi]
  mov ah, al
  mov [edi], ax
  inc esi
  inc edi
  inc edi
  dec ecx
  jne nextpixel
done:
 }
}

#else

void gr_linear_rep_movsd_2x(ubyte *src, ubyte *dest, unsigned int num_pixels)
{
	double  *d = (double *)dest;
	uint    *s = (uint *)src;
	uint    doubletemp[2];
	uint    temp, work;
	unsigned int     i;

	if (num_pixels & 0x3) {
		// not a multiple of 4?  do single pixel at a time
		for (i=0; i<num_pixels; i++) {
			*dest++ = *src;
			*dest++ = *src++;
		}
		return;
	}

	for (i = 0; i < num_pixels / 4; i++) {
		temp = work = *s++;

		temp = ((temp >> 8) & 0x00FFFF00) | (temp & 0xFF0000FF); // 0xABCDEFGH -> 0xABABCDEF
		temp = ((temp >> 8) & 0x000000FF) | (temp & 0xFFFFFF00); // 0xABABCDEF -> 0xABABCDCD
		doubletemp[0] = temp;

		work = ((work << 8) & 0x00FFFF00) | (work & 0xFF0000FF); // 0xABCDEFGH -> 0xABEFGHGH
		work = ((work << 8) & 0xFF000000) | (work & 0x00FFFFFF); // 0xABEFGHGH -> 0xEFEFGHGH
		doubletemp[1] = work;

		*d = *((double *) &(doubletemp[0]));
		d++;
	}
}

#endif

//------------------------------------------------------------------------------

#ifdef __MSDOS__

static void modex_copy_column(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize);

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux modex_copy_column parm [esi] [edi] [ecx] [ebx] [edx] modify exact [ecx esi edi] = \
"nextpixel:"            \
    "mov    al,[esi]"   \
    "add    esi, ebx"   \
    "mov    [edi], al"  \
    "add    edi, edx"   \
    "dec    ecx"        \
    "jne    nextpixel"

#elif !defined(NO_ASM) && defined(__GNUC__)

static inline void modex_copy_column(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize) {
/*#pragma aux modex_copy_column parm [esi] [edi] [ecx] [ebx] [edx] modify exact [ecx esi edi] = */
 __asm__ __volatile__ (
"0: ;"
    "movb   (%%esi), %%al;"
    "addl   %%ebx, %%esi;"
    "movb   %%al, (%%edi);"
    "addl   %%edx, %%edi;"
    "decl   %%ecx;"
    "jne    0b"
 : : "S" (src), "D" (dest), "c" (num_pixels), "b" (src_rowsize), "d" (dest_rowsize)
 : "%eax", "%ecx", "%esi", "%edi");
}

#else

static void modex_copy_column(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize)
{
	src = src;
	dest = dest;
	num_pixels = num_pixels;
	src_rowsize = src_rowsize;
	dest_rowsize = dest_rowsize;
	Int3();
}

#endif

//------------------------------------------------------------------------------

static void modex_copy_column_m(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize);

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux modex_copy_column_m parm [esi] [edi] [ecx] [ebx] [edx] modify exact [ecx esi edi] = \
"nextpixel:"            \
    "mov    al,[esi]"   \
    "add    esi, ebx"   \
    "cmp    al, " TRANSPARENCY_COLOR_STR   \
    "je skip_itx"       \
    "mov    [edi], al"  \
"skip_itx:"             \
    "add    edi, edx"   \
    "dec    ecx"        \
    "jne    nextpixel"

#elif !defined(NO_ASM) && defined(__GNUC__)

static inline void modex_copy_column_m(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize) {
/* #pragma aux modex_copy_column_m parm [esi] [edi] [ecx] [ebx] [edx] modify exact [ecx esi edi] = */
 int dummy[3];
 __asm__ __volatile__ (
"0: ;"
    "movb    (%%esi), %%al;"
    "addl    %%ebx, %%esi;"
    "cmpb    $" TRANSPARENCY_COLOR_STR ", %%al;"
    "je 1f;"
    "movb   %%al, (%%edi);"
"1: ;"
    "addl   %%edx, %%edi;"
    "decl   %%ecx;"
    "jne    0b"
 : "=c" (dummy[0]), "=S" (dummy[1]), "=D" (dummy[2])
 : "1" (src), "2" (dest), "0" (num_pixels), "b" (src_rowsize), "d" (dest_rowsize)
 :      "%eax");
}

#else

static void modex_copy_column_m(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize)
{
	src = src;
	dest = dest;
	num_pixels = num_pixels;
	src_rowsize = src_rowsize;
	dest_rowsize = dest_rowsize;
	Int3();
}

#endif

#endif /* __MSDOS__ */

//------------------------------------------------------------------------------

void gr_ubitmap00(int x, int y, grs_bitmap *bmP)
{
	register int y1;
	int dest_rowsize;

	unsigned char * dest;
	unsigned char * src;

	dest_rowsize=grdCurCanv->cv_bitmap.bm_props.rowsize << gr_bitblt_dest_step_shift;
	dest = &(grdCurCanv->cv_bitmap.bm_texBuf[ dest_rowsize*y+x ]);

	src = bmP->bm_texBuf;

	for (y1=0; y1 < bmP->bm_props.h; y1++)    {
		if (gr_bitblt_double)
			gr_linear_rep_movsd_2x(src, dest, bmP->bm_props.w);
		else
			gr_linear_movsd(src, dest, bmP->bm_props.w);
		src += bmP->bm_props.rowsize;
		dest+= (int)(dest_rowsize);
	}
}

//------------------------------------------------------------------------------

void gr_ubitmap00m(int x, int y, grs_bitmap *bmP)
{
	register int y1;
	int dest_rowsize;

	unsigned char * dest;
	unsigned char * src;

	dest_rowsize=grdCurCanv->cv_bitmap.bm_props.rowsize << gr_bitblt_dest_step_shift;
	dest = &(grdCurCanv->cv_bitmap.bm_texBuf[ dest_rowsize*y+x ]);

	src = bmP->bm_texBuf;

	if (grBitBltFadeTable==NULL) {
		for (y1=0; y1 < bmP->bm_props.h; y1++)    {
			gr_linear_rep_movsdm(src, dest, bmP->bm_props.w);
			src += bmP->bm_props.rowsize;
			dest+= (int)(dest_rowsize);
		}
	} else {
		for (y1=0; y1 < bmP->bm_props.h; y1++)    {
			gr_linear_rep_movsdm_faded(src, dest, bmP->bm_props.w, grBitBltFadeTable[y1+y], 
												bmP->bm_palette, grdCurCanv->cv_bitmap.bm_palette);
			src += bmP->bm_props.rowsize;
			dest+= (int)(dest_rowsize);
		}
	}
}

//------------------------------------------------------------------------------

#if 0
"       jmp     aligned4        "   \
"       mov eax, edi            "   \
"       and eax, 11b            "   \
"       jz      aligned4        "   \
"       mov ebx, 4              "   \
"       sub ebx, eax            "   \
"       sub ecx, ebx            "   \
"alignstart:                    "   \
"       mov al, [esi]           "   \
"       add esi, 4              "   \
"       mov [edi], al           "   \
"       inc edi                 "   \
"       dec ebx                 "   \
"       jne alignstart          "   \
"aligned4:                      "
#endif

#ifdef __MSDOS__

static void modex_copy_scanline(ubyte * src, ubyte * dest, int npixels);

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux modex_copy_scanline parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx edx] = \
"       mov ebx, ecx            "   \
"       and ebx, 11b            "   \
"       shr ecx, 2              "   \
"       cmp ecx, 0              "   \
"       je      no2group        "   \
"next4pixels:                   "   \
"       mov al, [esi+8]         "   \
"       mov ah, [esi+12]        "   \
"       shl eax, 16             "   \
"       mov al, [esi]           "   \
"       mov ah, [esi+4]         "   \
"       mov [edi], eax          "   \
"       add esi, 16             "   \
"       add edi, 4              "   \
"       dec ecx                 "   \
"       jne next4pixels         "   \
"no2group:                      "   \
"       cmp ebx, 0              "   \
"       je      done2           "   \
"finishend:                     "   \
"       mov al, [esi]           "   \
"       add esi, 4              "   \
"       mov [edi], al           "   \
"       inc edi                 "   \
"       dec ebx                 "   \
"       jne finishend           "   \
"done2:                         ";

#elif !defined (NO_ASM) && defined(__GNUC__)

static inline void modex_copy_scanline(ubyte * src, ubyte * dest, int npixels) {
/* #pragma aux modex_copy_scanline parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx edx] */
int dummy[3];
 __asm__ __volatile__ (
"       movl %%ecx, %%ebx;"
"       andl $3, %%ebx;"
"       shrl $2, %%ecx;"
"       cmpl $0, %%ecx;"
"       je   1f;"
"0: ;"
"       movb 8(%%esi), %%al;"
"       movb 12(%%esi), %%ah;"
"       shll $16, %%eax;"
"       movb (%%esi), %%al;"
"       movb 4(%%esi), %%ah;"
"       movl %%eax, (%%edi);"
"       addl $16, %%esi;"
"       addl $4, %%edi;"
"       decl %%ecx;"
"       jne 0b;"
"1: ;"
"       cmpl $0, %%ebx;"
"       je      3f;"
"2: ;"
"       movb (%%esi), %%al;"
"       addl $4, %%esi;"
"       movb %%al, (%%edi);"
"       incl %%edi;"
"       decl %%ebx;"
"       jne 2b;"
"3:"
 : "=c" (dummy[0]), "=S" (dummy[1]), "=D" (dummy[2])
 : "1" (src), "2" (dest), "0" (npixels)
 :      "%eax", "%ebx", "%edx");
}

#else

static void modex_copy_scanline(ubyte * src, ubyte * dest, int npixels)
{
	src = src;
	dest = dest;
	npixels = npixels;
	Int3();
}

#endif

//------------------------------------------------------------------------------

static void modex_copy_scanline_2x(ubyte * src, ubyte * dest, int npixels);

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux modex_copy_scanline_2x parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx edx] = \
"       mov ebx, ecx            "   \
"       and ebx, 11b            "   \
"       shr ecx, 2              "   \
"       cmp ecx, 0              "   \
"       je      no2group        "   \
"next4pixels:                   "   \
"       mov al, [esi+4]         "   \
"       mov ah, [esi+6]         "   \
"       shl eax, 16             "   \
"       mov al, [esi]           "   \
"       mov ah, [esi+2]         "   \
"       mov [edi], eax          "   \
"       add esi, 8              "   \
"       add edi, 4              "   \
"       dec ecx                 "   \
"       jne next4pixels         "   \
"no2group:                      "   \
"       cmp ebx, 0              "   \
"       je      done2           "   \
"finishend:                     "   \
"       mov al, [esi]           "   \
"       add esi, 2              "   \
"       mov [edi], al           "   \
"       inc edi                 "   \
"       dec ebx                 "   \
"       jne finishend           "   \
"done2:                         ";

#elif !defined(NO_ASM) && defined(__GNUC__)

static inline void modex_copy_scanline_2x(ubyte * src, ubyte * dest, int npixels) {
/* #pragma aux modex_copy_scanline_2x parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx edx] = */
int dummy[3];
 __asm__ __volatile__ (
"       movl %%ecx, %%ebx;"
"       andl $3, %%ebx;"
"       shrl $2, %%ecx;"
"       cmpl $0, %%ecx;"
"       je 1f;"
"0: ;"
"       movb 4(%%esi), %%al;"
"       movb 6(%%esi), %%ah;"
"       shll $16, %%eax;"
"       movb (%%esi), %%al;"
"       movb 2(%%esi), %%ah;"
"       movl %%eax, (%%edi);"
"       addl $8, %%esi;"
"       addl $4, %%edi;"
"       decl %%ecx;"
"       jne 0b;"
"1: ;"
"       cmp $0, %%ebx;"
"       je  3f;"
"2:"
"       movb (%%esi),%%al;"
"       addl $2, %%esi;"
"       movb %%al, (%%edi);"
"       incl %%edi;"
"       decl %%ebx;"
"       jne 2b;"
"3:"
 : "=c" (dummy[0]), "=S" (dummy[1]), "=D" (dummy[2])
 : "1" (src), "2" (dest), "0" (npixels)
 :      "%eax", "%ebx", "%edx");
}

#else

static void modex_copy_scanline_2x(ubyte * src, ubyte * dest, int npixels)
{
	src = src;
	dest = dest;
	npixels = npixels;
	Int3();
}

#endif

//------------------------------------------------------------------------------
// From Linear to ModeX
void gr_bm_ubitblt01(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	ubyte * dbits;
	ubyte * sbits;
	int sstep,dstep;
	int y,plane;
	int w1;

	if (w < 4) return;

	sstep = src->bm_props.rowsize;
	dstep = dest->bm_props.rowsize << gr_bitblt_dest_step_shift;

	if (!gr_bitblt_double) {
		for (plane=0; plane<4; plane++) {
			gr_modex_setplane((plane+dx)&3);
			sbits = src->bm_texBuf + (src->bm_props.rowsize * sy) + sx + plane;
			dbits = &gr_video_memory[(dest->bm_props.rowsize * dy) + ((plane+dx)/4) ];
			w1 = w >> 2;
			if ((w&3) > plane) w1++;
			for (y=dy; y < dy+h; y++) {
				modex_copy_scanline(sbits, dbits, w1);
				dbits += dstep;
				sbits += sstep;
			}
		}
	} else {
		for (plane=0; plane<4; plane++) {
			gr_modex_setplane((plane+dx)&3);
			sbits = src->bm_texBuf + (src->bm_props.rowsize * sy) + sx + plane/2;
			dbits = &gr_video_memory[(dest->bm_props.rowsize * dy) + ((plane+dx)/4) ];
			w1 = w >> 2;
			if ((w&3) > plane) w1++;
			for (y=dy; y < dy+h; y++) {
				modex_copy_scanline_2x(sbits, dbits, w1);
				dbits += dstep;
				sbits += sstep;
			}
		}
	}
}

//------------------------------------------------------------------------------
// From Linear to ModeX masked
void gr_bm_ubitblt01m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	//ubyte * dbits1;
	//ubyte * sbits1;

	ubyte * dbits;
	ubyte * sbits;

	int x;
	//int y;

	sbits =   src->bm_texBuf  + (src->bm_props.rowsize * sy) + sx;
	dbits =   &gr_video_memory[(dest->bm_props.rowsize * dy) + dx/4];

	for (x=dx; x < dx+w; x++) {
		gr_modex_setplane(x&3);

		//sbits1 = sbits;
		//dbits1 = dbits;
		//for (y=0; y < h; y++)    {
		//	*dbits1 = *sbits1;
		//	sbits1 += src_bm_rowsize;
		//	dbits1 += dest_bm_rowsize;
		//}
		modex_copy_column_m(sbits, dbits, h, src->bm_props.rowsize, dest->bm_props.rowsize << gr_bitblt_dest_step_shift);

		sbits++;
		if ((x&3)==3)
			dbits++;
	}
}

#endif /* __MSDOS__ */

//------------------------------------------------------------------------------

void gr_ubitmap012(int x, int y, grs_bitmap *bm)
{
	register int x1, y1;
	unsigned char * src;

	src = bm->bm_texBuf;

	for (y1=y; y1 < (y+bm->bm_props.h); y1++)    {
		for (x1=x; x1 < (x+bm->bm_props.w); x1++)    {
			GrSetColor(*src++);
			gr_upixel(x1, y1);
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmap012m(int x, int y, grs_bitmap *bm)
{
	register int x1, y1;
	unsigned char * src;

	src = bm->bm_texBuf;

	for (y1=y; y1 < (y+bm->bm_props.h); y1++) {
		for (x1=x; x1 < (x+bm->bm_props.w); x1++) {
			if (*src != TRANSPARENCY_COLOR) {
				GrSetColor(*src);
				gr_upixel(x1, y1);
			}
			src++;
		}
	}
}

//------------------------------------------------------------------------------

#if defined(POLY_ACC)
void gr_ubitmap05(int x, int y, grs_bitmap *bm)
{
	register int x1, y1;
	unsigned char *src;
	short *dst;
	int mod;

	pa_flush();
	src = bm->bm_texBuf;
	dst = (short *)(DATA + y * ROWSIZE + x * PA_BPP);
	mod = ROWSIZE / 2 - bm->bm_props.w;

	for (y1=y; y1 < (y+bm->bm_props.h); y1++)    {
		for (x1=x; x1 < (x+bm->bm_props.w); x1++)    {
			*dst++ = pa_clut[*src++];
		}
		dst += mod;
	}
}

//------------------------------------------------------------------------------

void gr_ubitmap05m(int x, int y, grs_bitmap *bm)
{
	register int x1, y1;
	unsigned char *src;
	short *dst;
	int mod;

	pa_flush();
	src = bm->bm_texBuf;
	dst = (short *)(DATA + y * ROWSIZE + x * PA_BPP);
	mod = ROWSIZE / 2 - bm->bm_props.w;

	for (y1=y; y1 < (y+bm->bm_props.h); y1++)    {
		for (x1=x; x1 < (x+bm->bm_props.w); x1++)    {
			if (*src != TRANSPARENCY_COLOR)   {
				*dst = pa_clut[*src];
			}
			src++;
			++dst;
		}
		dst += mod;
	}
}

//------------------------------------------------------------------------------

void gr_bm_ubitblt05_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned short * dbits;
	unsigned char * sbits, scanline[1600];
	int i, data_offset, j, nextrow;

	pa_flush();
	nextrow=dest->bm_props.rowsize/PA_BPP;

	data_offset = 1;
	if (src->bm_props.flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_texBuf[4 + (src->bm_props.h*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->bm_texBuf[4+(i*data_offset)]);

	dbits = (unsigned short *)(dest->bm_texBuf + (dest->bm_props.rowsize * dy) + dx*PA_BPP);

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++)    {
		gr_rle_expand_scanline(scanline, sbits, sx, sx+w-1);
		for(j = 0; j != w; ++j)
			dbits[j] = pa_clut[scanline[j]];
		if (src->bm_props.flags & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_texBuf[4+((i+sy)*data_offset)]));
		else
			sbits += (int)(src->bm_texBuf[4+i+sy]);
		dbits += nextrow;
	}
}

//------------------------------------------------------------------------------

void gr_bm_ubitblt05m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned short * dbits;
	unsigned char * sbits, scanline[1600];
	int i, data_offset, j, nextrow;

	pa_flush();
	nextrow=dest->bm_props.rowsize/PA_BPP;
	data_offset = 1;
	if (src->bm_props.flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_texBuf[4 + (src->bm_props.h*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->bm_texBuf[4+(i*data_offset)]);

	dbits = (unsigned short *)(dest->bm_texBuf + (dest->bm_props.rowsize * dy) + dx*PA_BPP);

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++)    {
		gr_rle_expand_scanline(scanline, sbits, sx, sx+w-1);
		for(j = 0; j != w; ++j)
			if(scanline[j] != TRANSPARENCY_COLOR)
				dbits[j] = pa_clut[scanline[j]];
		if (src->bm_props.flags & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_texBuf[4+((i+sy)*data_offset)]));
		else
			sbits += (int)(src->bm_texBuf[4+i+sy]);
		dbits += nextrow;
	}
}
#endif

//------------------------------------------------------------------------------

void gr_ubitmapGENERIC(int x, int y, grs_bitmap * bm)
{
	register int x1, y1;

	for (y1=0; y1 < bm->bm_props.h; y1++)    {
		for (x1=0; x1 < bm->bm_props.w; x1++)    {
			GrSetColor(gr_gpixel(bm,x1,y1));
			gr_upixel(x+x1, y+y1);
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmapGENERICm(int x, int y, grs_bitmap * bm)
{
	register int x1, y1;
	ubyte c;

	for (y1=0; y1 < bm->bm_props.h; y1++) {
		for (x1=0; x1 < bm->bm_props.w; x1++) {
			c = gr_gpixel(bm,x1,y1);
			if (c != TRANSPARENCY_COLOR) {
				GrSetColor(c);
				gr_upixel(x+x1, y+y1);
			}
		}
	}
}

//------------------------------------------------------------------------------

#ifdef __MSDOS__
// From linear to SVGA
void gr_bm_ubitblt02(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * sbits;

	unsigned int offset, EndingOffset, VideoLocation;

	int sbpr, dbpr, y1, page, BytesToMove;

	sbpr = src->bm_props.rowsize;

	dbpr = dest->bm_props.rowsize << gr_bitblt_dest_step_shift;

	VideoLocation = (unsigned int)dest->bm_texBuf + (dest->bm_props.rowsize * dy) + dx;

	sbits = src->bm_texBuf + (sbpr*sy) + sx;

	for (y1=0; y1 < h; y1++)    {

		page    = VideoLocation >> 16;
		offset  = VideoLocation & 0xFFFF;

		gr_vesa_setpage(page);

		EndingOffset = offset+w-1;

		if (EndingOffset <= 0xFFFF)
		{
			if (gr_bitblt_double)
				gr_linear_rep_movsd_2x((void *)sbits, (void *)(offset+0xA0000), w);
			else
				gr_linear_movsd((void *)sbits, (void *)(offset+0xA0000), w);

			VideoLocation += dbpr;
			sbits += sbpr;
		}
		else
		{
			BytesToMove = 0xFFFF-offset+1;

			if (gr_bitblt_double)
				gr_linear_rep_movsd_2x((void *)sbits, (void *)(offset+0xA0000), BytesToMove);
			else
				gr_linear_movsd((void *)sbits, (void *)(offset+0xA0000), BytesToMove);

			page++;
			gr_vesa_setpage(page);

			if (gr_bitblt_double)
				gr_linear_rep_movsd_2x((void *)(sbits+BytesToMove/2), (void *)0xA0000, EndingOffset - 0xFFFF);
			else
				gr_linear_movsd((void *)(sbits+BytesToMove), (void *)0xA0000, EndingOffset - 0xFFFF);

			VideoLocation += dbpr;
			sbits += sbpr;
		}
	}
}
#endif

//------------------------------------------------------------------------------

#ifdef __MSDOS__

void gr_bm_ubitblt02m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * sbits;

	unsigned int offset, EndingOffset, VideoLocation;

	int sbpr, dbpr, y1, page, BytesToMove;

	sbpr = src->bm_props.rowsize;

	dbpr = dest->bm_props.rowsize << gr_bitblt_dest_step_shift;

	VideoLocation = (unsigned int)dest->bm_texBuf + (dest->bm_props.rowsize * dy) + dx;

	sbits = src->bm_texBuf + (sbpr*sy) + sx;

	for (y1=0; y1 < h; y1++)    {

		page    = VideoLocation >> 16;
		offset  = VideoLocation & 0xFFFF;

		gr_vesa_setpage(page);

		EndingOffset = offset+w-1;

		if (EndingOffset <= 0xFFFF)
		{
			gr_linear_rep_movsdm((void *)sbits, (void *)(offset+0xA0000), w);

			VideoLocation += dbpr;
			sbits += sbpr;
		}
		else
		{
			BytesToMove = 0xFFFF-offset+1;

			gr_linear_rep_movsdm((void *)sbits, (void *)(offset+0xA0000), BytesToMove);

			page++;
			gr_vesa_setpage(page);

			gr_linear_rep_movsdm((void *)(sbits+BytesToMove), (void *)0xA0000, EndingOffset - 0xFFFF);

			VideoLocation += dbpr;
			sbits += sbpr;
		}
	}
}

//------------------------------------------------------------------------------
// From SVGA to linear
void gr_bm_ubitblt20(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;

	unsigned int offset, offset1, offset2;

	int sbpr, dbpr, y1, page;

	dbpr = dest->bm_props.rowsize;

	sbpr = src->bm_props.rowsize;

	for (y1=0; y1 < h; y1++)    {

		offset2 =   (unsigned int)src->bm_texBuf  + (sbpr * (y1+sy)) + sx;
		dbits   =   dest->bm_texBuf + (dbpr * (y1+dy)) + dx;

		page = offset2 >> 16;
		offset = offset2 & 0xFFFF;
		offset1 = offset+w-1;
		gr_vesa_setpage(page);

		if (offset1 > 0xFFFF)  {
			// Overlaps two pages
			while(offset <= 0xFFFF)
				*dbits++ = gr_video_memory[offset++];
			offset1 -= (0xFFFF+1);
			offset = 0;
			page++;
			gr_vesa_setpage(page);
		}
		while(offset <= offset1)
			*dbits++ = gr_video_memory[offset++];

	}
}

#endif

//------------------------------------------------------------------------------
//@extern int Interlacing_on;

// From Linear to Linear
void gr_bm_ubitblt00(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	//int src_bm_rowsize_2, dest_bm_rowsize_2;
	int dstep;

	int i;

	sbits =   src->bm_texBuf  + (src->bm_props.rowsize * sy) + sx;
	dbits =   dest->bm_texBuf + (dest->bm_props.rowsize * dy) + dx;

	dstep = dest->bm_props.rowsize << gr_bitblt_dest_step_shift;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++)    {
		if (gr_bitblt_double)
			gr_linear_rep_movsd_2x(sbits, dbits, w);
		else
			gr_linear_movsd(sbits, dbits, w);
		sbits += src->bm_props.rowsize;
		dbits += dstep;
	}
}

//------------------------------------------------------------------------------
// From Linear to Linear Masked
void gr_bm_ubitblt00m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	//int src_bm_rowsize_2, dest_bm_rowsize_2;

	int i;

	sbits =   src->bm_texBuf  + (src->bm_props.rowsize * sy) + sx;
	dbits =   dest->bm_texBuf + (dest->bm_props.rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.

	if (grBitBltFadeTable==NULL) {
		for (i=0; i < h; i++)    {
			gr_linear_rep_movsdm(sbits, dbits, w);
			sbits += src->bm_props.rowsize;
			dbits += dest->bm_props.rowsize;
		}
	} else {
		for (i=0; i < h; i++)    {
			gr_linear_rep_movsdm_faded(sbits, dbits, w, grBitBltFadeTable[dy+i], src->bm_palette, dest->bm_palette);
			sbits += src->bm_props.rowsize;
			dbits += dest->bm_props.rowsize;
		}
	}
}


//------------------------------------------------------------------------------

extern void gr_lbitblt(grs_bitmap * source, grs_bitmap * dest, int height, int width);

// Clipped bitmap ...

//------------------------------------------------------------------------------

void GrBitmap(int x, int y, grs_bitmap *bm)
{
	grs_bitmap * const scr = &grdCurCanv->cv_bitmap;
	int dx1=x, dx2=x+bm->bm_props.w-1;
	int dy1=y, dy2=y+bm->bm_props.h-1;
	int sx=0, sy=0;

	if ((dx1 >= scr->bm_props.w) || (dx2 < 0)) 
		return;
	if ((dy1 >= scr->bm_props.h) || (dy2 < 0)) 
		return;
	if (dx1 < 0) { 
		sx = -dx1; 
		dx1 = 0; 
		}
	if (dy1 < 0) { 
		sy = -dy1; 
		dy1 = 0; 
		}
	if (dx2 >= scr->bm_props.w)
		dx2 = scr->bm_props.w-1;
	if (dy2 >= scr->bm_props.h)
		dy2 = scr->bm_props.h-1;

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)

	GrBmUBitBlt(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grdCurCanv->cv_bitmap);

}

//-NOT-used // From linear to SVGA
//-NOT-used void gr_bm_ubitblt02_2x(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
//-NOT-used {
//-NOT-used 	unsigned char * sbits;
//-NOT-used
//-NOT-used 	unsigned int offset, EndingOffset, VideoLocation;
//-NOT-used
//-NOT-used 	int sbpr, dbpr, y1, page, BytesToMove;
//-NOT-used
//-NOT-used 	sbpr = src->bm_props.rowsize;
//-NOT-used
//-NOT-used 	dbpr = dest->bm_props.rowsize << gr_bitblt_dest_step_shift;
//-NOT-used
//-NOT-used 	VideoLocation = (unsigned int)dest->bm_texBuf + (dest->bm_props.rowsize * dy) + dx;
//-NOT-used
//-NOT-used 	sbits = src->bm_texBuf + (sbpr*sy) + sx;
//-NOT-used
//-NOT-used 	for (y1=0; y1 < h; y1++)    {
//-NOT-used
//-NOT-used 		page    = VideoLocation >> 16;
//-NOT-used 		offset  = VideoLocation & 0xFFFF;
//-NOT-used
//-NOT-used 		gr_vesa_setpage(page);
//-NOT-used
//-NOT-used 		EndingOffset = offset+w-1;
//-NOT-used
//-NOT-used 		if (EndingOffset <= 0xFFFF)
//-NOT-used 		{
//-NOT-used 			gr_linear_rep_movsd_2x((void *)sbits, (void *)(offset+0xA0000), w);
//-NOT-used
//-NOT-used 			VideoLocation += dbpr;
//-NOT-used 			sbits += sbpr;
//-NOT-used 		}
//-NOT-used 		else
//-NOT-used 		{
//-NOT-used 			BytesToMove = 0xFFFF-offset+1;
//-NOT-used
//-NOT-used 			gr_linear_rep_movsd_2x((void *)sbits, (void *)(offset+0xA0000), BytesToMove);
//-NOT-used
//-NOT-used 			page++;
//-NOT-used 			gr_vesa_setpage(page);
//-NOT-used
//-NOT-used 			gr_linear_rep_movsd_2x((void *)(sbits+BytesToMove/2), (void *)0xA0000, EndingOffset - 0xFFFF);
//-NOT-used
//-NOT-used 			VideoLocation += dbpr;
//-NOT-used 			sbits += sbpr;
//-NOT-used 		}
//-NOT-used
//-NOT-used
//-NOT-used 	}
//-NOT-used }


//-NOT-used // From Linear to Linear
//-NOT-used void gr_bm_ubitblt00_2x(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
//-NOT-used {
//-NOT-used 	unsigned char * dbits;
//-NOT-used 	unsigned char * sbits;
//-NOT-used 	//int src_bm_rowsize_2, dest_bm_rowsize_2;
//-NOT-used
//-NOT-used 	int i;
//-NOT-used
//-NOT-used 	sbits =   src->bm_texBuf  + (src->bm_props.rowsize * sy) + sx;
//-NOT-used 	dbits =   dest->bm_texBuf + (dest->bm_props.rowsize * dy) + dx;
//-NOT-used
//-NOT-used 	// No interlacing, copy the whole buffer.
//-NOT-used 	for (i=0; i < h; i++)    {
//-NOT-used 		gr_linear_rep_movsd_2x(sbits, dbits, w);
//-NOT-used
//-NOT-used 		sbits += src->bm_props.rowsize;
//-NOT-used 		dbits += dest->bm_props.rowsize << gr_bitblt_dest_step_shift;
//-NOT-used 	}
//-NOT-used }

//------------------------------------------------------------------------------

void gr_bm_ubitblt00_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	int i, data_offset;

	data_offset = 1;
	if (src->bm_props.flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_texBuf[4 + (src->bm_props.h*data_offset)];

	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->bm_texBuf[4+(i*data_offset)]));

	dbits = dest->bm_texBuf + (dest->bm_props.rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++)    {
		gr_rle_expand_scanline(dbits, sbits, sx, sx+w-1);
		if (src->bm_props.flags & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_texBuf[4+((i+sy)*data_offset)])));
		else
			sbits += (int)(src->bm_texBuf[4+i+sy]);
		dbits += dest->bm_props.rowsize << gr_bitblt_dest_step_shift;
	}
}

//------------------------------------------------------------------------------

void gr_bm_ubitblt00m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	int i, data_offset;

	data_offset = 1;
	if (src->bm_props.flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_texBuf[4 + (src->bm_props.h*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->bm_texBuf[4+(i*data_offset)]));

	dbits = dest->bm_texBuf + (dest->bm_props.rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++)    {
		gr_rle_expand_scanline_masked(dbits, sbits, sx, sx+w-1);
		if (src->bm_props.flags & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_texBuf[4+((i+sy)*data_offset)])));
		else
			sbits += (int)(src->bm_texBuf[4+i+sy]);
		dbits += dest->bm_props.rowsize << gr_bitblt_dest_step_shift;
	}
}

//------------------------------------------------------------------------------
// in rle.c

extern void gr_rle_expand_scanline_generic(grs_bitmap * dest, int dx, int dy, ubyte *src, int x1, int x2 );
extern void gr_rle_expand_scanline_generic_masked(grs_bitmap * dest, int dx, int dy, ubyte *src, int x1, int x2 );
extern void gr_rle_expand_scanline_svga_masked(grs_bitmap * dest, int dx, int dy, ubyte *src, int x1, int x2 );

void gr_bm_ubitblt0x_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	int i, data_offset;
	register int y1;
	unsigned char * sbits;

	//con_printf(0, "SVGA RLE!\n");

	data_offset = 1;
	if (src->bm_props.flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_texBuf[4 + (src->bm_props.h*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->bm_texBuf[4+(i*data_offset)]));

	for (y1=0; y1 < h; y1++)    {
		gr_rle_expand_scanline_generic(dest, dx, dy+y1,  sbits, sx, sx+w-1 );
		if (src->bm_props.flags & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_texBuf[4+((y1+sy)*data_offset)])));
		else
			sbits += (int)src->bm_texBuf[4+y1+sy];
	}

}

//------------------------------------------------------------------------------

void gr_bm_ubitblt0xm_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	int i, data_offset;
	register int y1;
	unsigned char * sbits;

	//con_printf(0, "SVGA RLE!\n");

	data_offset = 1;
	if (src->bm_props.flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_texBuf[4 + (src->bm_props.h*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->bm_texBuf[4+(i*data_offset)]));

	for (y1=0; y1 < h; y1++)    {
		gr_rle_expand_scanline_generic_masked(dest, dx, dy+y1,  sbits, sx, sx+w-1 );
		if (src->bm_props.flags & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_texBuf[4+((y1+sy)*data_offset)])));
		else
			sbits += (int)src->bm_texBuf[4+y1+sy];
	}

}

//------------------------------------------------------------------------------

#ifdef __MSDOS__
void gr_bm_ubitblt02m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	int i, data_offset;
	register int y1;
	unsigned char * sbits;

	//con_printf(0, "SVGA RLE!\n");

	data_offset = 1;
	if (src->bm_props.flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_texBuf[4 + (src->bm_props.h*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->bm_texBuf[4+(i*data_offset)]);

	for (y1=0; y1 < h; y1++)    {
		gr_rle_expand_scanline_svga_masked(dest, dx, dy+y1,  sbits, sx, sx+w-1 );
		if (src->bm_props.flags & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_texBuf[4+((y1+sy)*data_offset)]));
		else
			sbits += (int)src->bm_texBuf[4+y1+sy];
	}
}
#endif

void GrBmUBitBlt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	register int x1, y1;

if ((src->bm_props.type == BM_LINEAR) && (dest->bm_props.type == BM_LINEAR)) {
	if (src->bm_props.flags & BM_FLAG_RLE)
		gr_bm_ubitblt00_rle(w, h, dx, dy, sx, sy, src, dest);
	else
		gr_bm_ubitblt00(w, h, dx, dy, sx, sy, src, dest);
	return;
	}

#ifdef OGL
if ((src->bm_props.type == BM_LINEAR) && (dest->bm_props.type == BM_OGL)) {
	OglUBitBlt(w, h, dx, dy, sx, sy, src, dest);
	return;
	}
if ((src->bm_props.type == BM_OGL) && (dest->bm_props.type == BM_LINEAR)) {
	OglUBitBltToLinear(w, h, dx, dy, sx, sy, src, dest);
	return;
	}
if ((src->bm_props.type == BM_OGL) && (dest->bm_props.type == BM_OGL)) {
	OglUBitBltCopy(w, h, dx, dy, sx, sy, src, dest);
	return;
	}
#endif

#ifdef D1XD3D
if ((src->bm_props.type == BM_LINEAR) && (dest->bm_props.type == BM_DIRECTX)) {
	Assert ((int)dest->bm_texBuf == BM_D3D_RENDER || (int)dest->bm_texBuf == BM_D3D_DISPLAY);
	Win32_BlitLinearToDirectX_bm (src, sx, sy, w, h, dx, dy, 0);
	return;
	}
if ((src->bm_props.type == BM_DIRECTX) && (dest->bm_props.type == BM_LINEAR))
	return;
if ((src->bm_props.type == BM_DIRECTX) && (dest->bm_props.type == BM_DIRECTX))
	return;
#endif
if ((src->bm_props.flags & BM_FLAG_RLE) && (src->bm_props.type == BM_LINEAR)) {
	gr_bm_ubitblt0x_rle(w, h, dx, dy, sx, sy, src, dest);
	return;
	}
#ifdef __MSDOS__
if ((src->bm_props.type == BM_LINEAR) && (dest->bm_props.type == BM_SVGA)) {
	gr_bm_ubitblt02(w, h, dx, dy, sx, sy, src, dest);
	return;
	}
if ((src->bm_props.type == BM_SVGA) && (dest->bm_props.type == BM_LINEAR)) {
	gr_bm_ubitblt20(w, h, dx, dy, sx, sy, src, dest);
	return;
	}
if ((src->bm_props.type == BM_LINEAR) && (dest->bm_props.type == BM_MODEX)) {
	gr_bm_ubitblt01(w, h, dx+XOFFSET, dy+YOFFSET, sx, sy, src, dest);
	return;
	}
#endif
#if defined(POLY_ACC)
if ((src->bm_props.type == BM_LINEAR) && (dest->bm_props.type == BM_LINEAR15)) {
	ubyte *s = src->bm_texBuf + sy * src->bm_props.rowsize + sx;
	ushort *t = (ushort *)(dest->bm_texBuf + dy * dest->bm_props.rowsize + dx * PA_BPP);
	int x;
	pa_flush();
	for(;h--;) {
		for(x = 0; x < w; x++)
			t[x] = pa_clut[s[x]];
		s += src->bm_props.rowsize;
		t += dest->bm_props.rowsize / PA_BPP;
		}
	return;
	}
if ((src->bm_props.type == BM_LINEAR15) && (dest->bm_props.type == BM_LINEAR15)) {
	pa_blit(dest, dx, dy, src, sx, sy, w, h);
	return;
	}
#endif
for (y1=0; y1 < h; y1++)  
	for (x1=0; x1 < w; x1++)  
		gr_bm_pixel(dest, dx+x1, dy+y1, gr_gpixel(src,sx+x1,sy+y1));
}

//------------------------------------------------------------------------------

void GrBmBitBlt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	int	dx1 = dx, 
			dx2 = dx + dest->bm_props.w - 1;
	int	dy1 = dy, 
			dy2 = dy + dest->bm_props.h - 1;
	int	sx1 = sx, 
			sx2 = sx + src->bm_props.w - 1;
	int	sy1 = sy, 
			sy2 = sy + src->bm_props.h - 1;

if ((dx1 >= dest->bm_props.w) || (dx2 < 0)) 
	return;
if ((dy1 >= dest->bm_props.h) || (dy2 < 0)) 
	return;
if (dx1 < 0) { 
	sx1 += -dx1; 
	dx1 = 0; 
	}
if (dy1 < 0) { 
	sy1 += -dy1; 
	dy1 = 0; 
	}
if (dx2 >= dest->bm_props.w) { 
	dx2 = dest->bm_props.w-1; 
	}
if (dy2 >= dest->bm_props.h) { 
	dy2 = dest->bm_props.h-1; 
	}

if ((sx1 >= src->bm_props.w) || (sx2 < 0)) 
	return;
if ((sy1 >= src->bm_props.h) || (sy2 < 0)) 
	return;
if (sx1 < 0) { 
	dx1 += -sx1; sx1 = 0; 
	}
if (sy1 < 0) { 
	dy1 += -sy1; sy1 = 0; 
	}
if (sx2 >= src->bm_props.w) { 
	sx2 = src->bm_props.w-1; 
	}
if (sy2 >= src->bm_props.h) { 
	sy2 = src->bm_props.h-1; 
	}

// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
if (dx2-dx1+1 < w)
	w = dx2-dx1+1;
if (dy2-dy1+1 < h)
	h = dy2-dy1+1;
if (sx2-sx1+1 < w)
	w = sx2-sx1+1;
if (sy2-sy1+1 < h)
	h = sy2-sy1+1;

GrBmUBitBlt(w,h, dx1, dy1, sx1, sy1, src, dest);
}

//------------------------------------------------------------------------------

void gr_ubitmap(int x, int y, grs_bitmap *bmP)
{
	int source, dest;

	source = bmP->bm_props.type;
	dest = TYPE;

	if (source==BM_LINEAR) {
		switch(dest)
		{
		case BM_LINEAR:
			if (bmP->bm_props.flags & BM_FLAG_RLE)
				gr_bm_ubitblt00_rle(bmP->bm_props.w, bmP->bm_props.h, x, y, 0, 0, bmP, &grdCurCanv->cv_bitmap);
			else
				gr_ubitmap00(x, y, bmP);
			return;
#ifdef OGL
		case BM_OGL:
			OglUBitMapM(x,y,bmP);
			return;
#endif
#ifdef D1XD3D
		case BM_DIRECTX:
			Assert ((int)grdCurCanv->cv_bitmap.bm_texBuf == BM_D3D_RENDER || (int)grdCurCanv->cv_bitmap.bm_texBuf == BM_D3D_DISPLAY);
			Win32_BlitLinearToDirectX_bm(bmP, 0, 0, bmP->bm_props.w, bmP->bm_props.h, x, y, 0);
			return;
#endif
#ifdef __MSDOS__
		case BM_SVGA:
			if (bmP->bm_props.flags & BM_FLAG_RLE)
				gr_bm_ubitblt0x_rle(bmP->bm_props.w, bmP->bm_props.h, x, y, 0, 0, bmP, &grdCurCanv->cv_bitmap);
			else
				gr_vesa_bitmap(bmP, &grdCurCanv->cv_bitmap, x, y);
			return;
		case BM_MODEX:
			gr_bm_ubitblt01(bmP->bm_props.w, bmP->bm_props.h, x+XOFFSET, y+YOFFSET, 0, 0, bmP, &grdCurCanv->cv_bitmap);
			return;
#endif
#if defined(POLY_ACC)
		case BM_LINEAR15:
			if (bmP->bm_props.flags & BM_FLAG_RLE)
				gr_bm_ubitblt05_rle(bmP->bm_props.w, bmP->bm_props.h, x, y, 0, 0, bmP, &grdCurCanv->cv_bitmap);
			else
				gr_ubitmap05(x, y, bmP);
			return;

#endif
		default:
			gr_ubitmap012(x, y, bmP);
			return;
		}
	} else  {
		gr_ubitmapGENERIC(x, y, bmP);
	}
}

//------------------------------------------------------------------------------

void GrUBitmapM(int x, int y, grs_bitmap *bmP)
{
	int source, dest;

	source = bmP->bm_props.type;
	dest = TYPE;

	Assert(x+bmP->bm_props.w <= grdCurCanv->cv_w);
	Assert(y+bmP->bm_props.h <= grdCurCanv->cv_h);

#ifdef _3DFX
	_3dfx_Blit(x, y, bmP);
	if (_3dfx_skip_ddraw)
		return;
#endif

	if (source==BM_LINEAR) {
		switch(dest)
		{
		case BM_LINEAR:
			if (bmP->bm_props.flags & BM_FLAG_RLE)
				gr_bm_ubitblt00m_rle(bmP->bm_props.w, bmP->bm_props.h, x, y, 0, 0, bmP, &grdCurCanv->cv_bitmap);
			else
				gr_ubitmap00m(x, y, bmP);
			return;
#ifdef OGL
		case BM_OGL:
			OglUBitMapM(x,y,bmP);
			return;
#endif
#ifdef D1XD3D
		case BM_DIRECTX:
			if (bmP->bm_props.w < 35 && bmP->bm_props.h < 35) {
				// ugly hack needed for reticle
				if (bmP->bm_props.flags & BM_FLAG_RLE)
					gr_bm_ubitblt0x_rle(bmP->bm_props.w, bmP->bm_props.h, x, y, 0, 0, bmP, &grdCurCanv->cv_bitmap, 1);
				else
					gr_ubitmapGENERICm(x, y, bmP);
				return;
			}
			Assert ((int)grdCurCanv->cv_bitmap.bm_texBuf == BM_D3D_RENDER || (int)grdCurCanv->cv_bitmap.bm_texBuf == BM_D3D_DISPLAY);
			Win32_BlitLinearToDirectX_bm(bmP, 0, 0, bmP->bm_props.w, bmP->bm_props.h, x, y, 1);
			return;
#endif
#ifdef __MSDOS__
		case BM_SVGA:
			if (bmP->bm_props.flags & BM_FLAG_RLE)
				gr_bm_ubitblt02m_rle(bmP->bm_props.w, bmP->bm_props.h, x, y, 0, 0, bmP, &grdCurCanv->cv_bitmap);
			//gr_bm_ubitblt0xm_rle(bmP->bm_props.w, bmP->bm_props.h, x, y, 0, 0, bmP, &grdCurCanv->cv_bitmap);
			else
				gr_bm_ubitblt02m(bmP->bm_props.w, bmP->bm_props.h, x, y, 0, 0, bmP, &grdCurCanv->cv_bitmap);
			//gr_ubitmapGENERICm(x, y, bmP);
			return;
		case BM_MODEX:
			gr_bm_ubitblt01m(bmP->bm_props.w, bmP->bm_props.h, x+XOFFSET, y+YOFFSET, 0, 0, bmP, &grdCurCanv->cv_bitmap);
			return;
#endif
#if defined(POLY_ACC)
		case BM_LINEAR15:
			if (bmP->bm_props.flags & BM_FLAG_RLE)
				gr_bm_ubitblt05m_rle(bmP->bm_props.w, bmP->bm_props.h, x, y, 0, 0, bmP, &grdCurCanv->cv_bitmap);
			else
				gr_ubitmap05m(x, y, bmP);
			return;
#endif

		default:
			gr_ubitmap012m(x, y, bmP);
			return;
		}
	} else {
		gr_ubitmapGENERICm(x, y, bmP);
	}
}

//------------------------------------------------------------------------------

void GrBitmapM(int x, int y, grs_bitmap *bmP)
{
	int dx1=x, dx2=x+bmP->bm_props.w-1;
	int dy1=y, dy2=y+bmP->bm_props.h-1;
	int sx=0, sy=0;

	if ((dx1 >= grdCurCanv->cv_bitmap.bm_props.w) || (dx2 < 0)) 
		return;
	if ((dy1 >= grdCurCanv->cv_bitmap.bm_props.h) || (dy2 < 0)) 
		return;
	if (dx1 < 0) { 
		sx = -dx1; 
		dx1 = 0; 
		}
	if (dy1 < 0) { 
		sy = -dy1; 
		dy1 = 0; 
		}
	if (dx2 >= grdCurCanv->cv_bitmap.bm_props.w)
		dx2 = grdCurCanv->cv_bitmap.bm_props.w-1;
	if (dy2 >= grdCurCanv->cv_bitmap.bm_props.h)
		dy2 = grdCurCanv->cv_bitmap.bm_props.h-1; 
	// Draw bitmap bmP[x,y] into (dx1,dy1)-(dx2,dy2)

	if ((bmP->bm_props.type == BM_LINEAR) && (grdCurCanv->cv_bitmap.bm_props.type == BM_LINEAR)) {
		if (bmP->bm_props.flags & BM_FLAG_RLE)
			gr_bm_ubitblt00m_rle(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bmP, &grdCurCanv->cv_bitmap);
		else
			gr_bm_ubitblt00m(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bmP, &grdCurCanv->cv_bitmap);
		return;
	}
#ifdef __MSDOS__
	else if ((bmP->bm_props.type == BM_LINEAR) && (grdCurCanv->cv_bitmap.bm_props.type == BM_SVGA))
	{
		gr_bm_ubitblt02m(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bmP, &grdCurCanv->cv_bitmap);
		return;
	}
#endif

	GrBmUBitBltM(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bmP, &grdCurCanv->cv_bitmap);

}

//------------------------------------------------------------------------------

void GrBmUBitBltM(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	register int x1, y1;
	ubyte c;

#ifdef OGL
	if ((src->bm_props.type == BM_LINEAR) && (dest->bm_props.type == BM_OGL))
		OglUBitBlt(w, h, dx, dy, sx, sy, src, dest);
	else if ((src->bm_props.type == BM_OGL) && (dest->bm_props.type == BM_LINEAR))
		OglUBitBltToLinear(w, h, dx, dy, sx, sy, src, dest);
	else if ((src->bm_props.type == BM_OGL) && (dest->bm_props.type == BM_OGL))
		OglUBitBltCopy(w, h, dx, dy, sx, sy, src, dest);
	else
#endif
#ifdef D1XD3D
	if ((src->bm_props.type == BM_LINEAR) && (dest->bm_props.type == BM_DIRECTX))
	{
		Assert ((int)dest->bm_texBuf == BM_D3D_RENDER || (int)dest->bm_texBuf == BM_D3D_DISPLAY);
		Win32_BlitLinearToDirectX_bm (src, sx, sy, w, h, dx, dy, 1);
		return;
	}
	if ((src->bm_props.type == BM_DIRECTX) && (dest->bm_props.type == BM_DIRECTX))
	{
		Assert ((int)src->bm_texBuf == BM_D3D_RENDER || (int)src->bm_texBuf == BM_D3D_DISPLAY);
		//Win32_BlitDirectXToDirectX (w, h, dx, dy, sx, sy, src->bm_texBuf, dest->bm_texBuf, 0);
		return;
	}
#endif
#if defined(POLY_ACC)
	if(src->bm_props.type == BM_LINEAR && dest->bm_props.type == BM_LINEAR15)
	{
		ubyte *s;
		ushort *d;
		ushort u;
		int smod, dmod;

		pa_flush();
		s = (ubyte *)(src->bm_texBuf + src->bm_props.rowsize * sy + sx);
		smod = src->bm_props.rowsize - w;
		d = (ushort *)(dest->bm_texBuf + dest->bm_props.rowsize * dy + dx * PA_BPP);
		dmod = dest->bm_props.rowsize / PA_BPP - w;
		for (; h--;) {
			for (x1=w; x1--;) {
				if ((u = *s) != TRANSPARENCY_COLOR)
					*d = pa_clut[u];
				++s;
				++d;
			}
			s += smod;
			d += dmod;
		}
	}

	if(src->bm_props.type == BM_LINEAR15)
	{
		Assert(src->bm_props.type == dest->bm_props.type);         // I don't support 15 to 8 yet.
		pa_blit_transparent(dest, dx, dy, src, sx, sy, w, h);
		return;
	}
#endif

	for (y1=0; y1 < h; y1++) {
		for (x1=0; x1 < w; x1++) {
			if ((c=gr_gpixel(src,sx+x1,sy+y1))!=TRANSPARENCY_COLOR)
				gr_bm_pixel(dest, dx+x1, dy+y1,c);
		}
	}
}

//------------------------------------------------------------------------------
// rescalling bitmaps, 10/14/99 Jan Bobrowski jb@wizard.ae.krakow.pl

inline void scale_line(sbyte *in, sbyte *out, int ilen, int olen)
{
	int a = olen/ilen, b = olen%ilen;
	int c = 0, i;
	sbyte *end = out + olen;
	while(out<end) {
		i = a;
		c += b;
		if(c >= ilen) {
			c -= ilen;
			goto inside;
		}
		while(--i>=0) {
		inside:
			*out++ = *in;
		}
		in++;
	}
}

//------------------------------------------------------------------------------

void GrBitmapScaleTo(grs_bitmap *src, grs_bitmap *dst)
{
	sbyte *s = (sbyte *) (src->bm_texBuf);
	sbyte *d = (sbyte *) (dst->bm_texBuf);
	int h = src->bm_props.h;
	int a = dst->bm_props.h/h, b = dst->bm_props.h%h;
	int c = 0, i, y;

	for(y=0; y<h; y++) {
		i = a;
		c += b;
		if(c >= h) {
			c -= h;
			goto inside;
		}
		while(--i>=0) {
		inside:
			scale_line(s, d, src->bm_props.w, dst->bm_props.w);
			d += dst->bm_props.rowsize;
		}
		s += src->bm_props.rowsize;
	}
}

//------------------------------------------------------------------------------

void show_fullscr(grs_bitmap *src)
{
	grs_bitmap * const dest = &grdCurCanv->cv_bitmap;

#ifdef OGL
if(src->bm_props.type == BM_LINEAR && dest->bm_props.type == BM_OGL) {
	if (!gameStates.render.bBlendBackground)
		glDisable(GL_BLEND);
	OglUBitBltI (dest->bm_props.w, dest->bm_props.h, 0, 0, src->bm_props.w, src->bm_props.h, 0, 0, src, dest, 0);//use opengl to scale, faster and saves ram. -MPM
	if (!gameStates.render.bBlendBackground)
		glEnable(GL_BLEND);
	return;
	}
#endif
if(dest->bm_props.type != BM_LINEAR) {
	grs_bitmap *tmp = GrCreateBitmap (dest->bm_props.w, dest->bm_props.h, 0);
	GrBitmapScaleTo(src, tmp);
	GrBitmap(0, 0, tmp);
	GrFreeBitmap(tmp);
	return;
	}
GrBitmapScaleTo (src, dest);
}

//------------------------------------------------------------------------------

