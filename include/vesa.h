#ifndef _VESA_H
#define _VESA_H
extern int32_t  gr_vesa_setmode(int32_t mode);
extern int32_t  gr_vesa_checkmode(int32_t mode);
extern void gr_vesa_setstart(int32_t x, int32_t y );
extern void gr_vesa_setpage(int32_t page);
extern void gr_vesa_incpage();
extern int32_t  gr_vesa_setlogical(int32_t pixels_per_scanline);
extern void gr_vesa_scanline(int32_t x1, int32_t x2, int32_t y, uint8_t color );
extern void gr_vesa_bitblt( uint8_t * source_ptr, uint32_t vesa_address, int32_t height, int32_t width );
extern void gr_vesa_pixel( uint8_t color, uint32_t offset );
#endif
