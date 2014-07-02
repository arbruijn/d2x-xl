#ifndef _MODEX_H
#define _MODEX_H
extern int32_t  gr_modex_setmode(int16_t mode);
extern void gr_modex_setplane(int16_t plane);
extern void gr_modex_setstart(int16_t x, int16_t y, int32_t wait_for_retrace);
extern void gr_modex_uscanline( int16_t x1, int16_t x2, int16_t y, uint8_t color );
extern void gr_modex_line();
extern void gr_sync_display();

extern int32_t modex_line_vertincr;
extern int32_t modex_line_incr1;
extern int32_t modex_line_incr2;    
extern int32_t modex_line_x1;       
extern int32_t modex_line_y1;       
extern int32_t modex_line_x2;       
extern int32_t modex_line_y2;       
extern uint8_t modex_line_Color;
#endif
