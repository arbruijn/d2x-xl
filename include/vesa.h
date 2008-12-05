#ifndef _VESA_H
#define _VESA_H
extern int  gr_vesa_setmode(int mode);
extern int  gr_vesa_checkmode(int mode);
extern void gr_vesa_setstart(int x, int y );
extern void gr_vesa_setpage(int page);
extern void gr_vesa_incpage();
extern int  gr_vesa_setlogical(int pixels_per_scanline);
extern void gr_vesa_scanline(int x1, int x2, int y, ubyte color );
extern void gr_vesa_bitblt( ubyte * source_ptr, uint vesa_address, int height, int width );
extern void gr_vesa_pixel( ubyte color, uint offset );
#endif
