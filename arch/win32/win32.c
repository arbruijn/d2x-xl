#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "gr.h"
#include "grdef.h"
#include "u_mem.h"
#include "timer.h"
#include "error.h"

#include "gr.h"
#include "grdef.h"
#include "palette.h"
#include "rle.h"
#include "d3dhelp.h"
#include "game.h"
#include "gauges.h"
#include "args.h"

#include "gamefont.h"

#ifdef _DEBUG
#ifdef _WIN32
#include <crtdbg.h>
#endif
#endif

void gr_linear_rep_movsdm(ubyte *src, ubyte *dest, int num_pixels);
void InitMain();
void show_dd_error(HRESULT hr, char *loc);
void GetDDErrorString(HRESULT hr, char *buf, int size);

void BlitToPrimary(HDC hSrcDC);
void BlitToPrimaryRect(HDC hSrcDC, int x, int y, int w, int h, 
	unsigned char *dst);
extern HWND g_hWnd;

void key_init(void);
void MouseInit(void);


unsigned char *createdib(void);

static unsigned char *backbuffer, *screenbuffer;
static int screensize;
static HBITMAP screen_bitmap;

void arch_init_start()
{
	#ifdef _DEBUG
	#ifdef _WIN32
	if (FindArg("-memdbg"))
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | 
			/* _CRTDBG_CHECK_CRT_DF | */
			_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#endif
	#endif

	InitMain ();
}

void arch_init()
{
	//SetPriorityClass (GetCurrentProcess(),HIGH_PRIORITY_CLASS);

	timer_init ();
	key_init();
	MouseInit();

	//printf("arch_init successfully completed\n");
}




int gameStates.gfx.bInstalled = 0;

int GrVideoModeOK(int mode)
{
	if (mode == SM_320x200C)
		return 0;

	return 11;
}

int GrSetMode(int mode)
{
	unsigned int w,h,t,r;

	if (mode == SM_ORIGINAL)
		return 0;

	switch (mode)
	{
	case SM(320,200):
		w = 320; r = 320; h = 200; t=BM_LINEAR;//BM_DIRECTX;;
		break;
	default:
		return 1;
	}

	GrPaletteStepClear();

	memset( grdCurScreen, 0, sizeof(grsScreen);
	grdCurScreen->scMode = mode;
	grdCurScreen->scWidth = w;
	grdCurScreen->scHeight = h;
	grdCurScreen->scAspect = FixDiv(grdCurScreen->scWidth*3,grdCurScreen->scHeight*4);
	GrInitCanvas(&grdCurScreen->scCanvas, (unsigned char *)BM_D3D_DISPLAY, t, w, h);
	GrSetCurrentCanvas(NULL);


	if (!(backbuffer = createdib()))
		return 1;

	grdCurScreen->scCanvas.cvBitmap.bmTexBuf = backbuffer;
	grdCurScreen->scCanvas.cvBitmap.bmProps.nType = BM_LINEAR;
	grdCurScreen->scCanvas.cvBitmap.bmProps.x = 0;
	grdCurScreen->scCanvas.cvBitmap.bmProps.y = 0;
	grdCurScreen->scCanvas.cvBitmap.bmProps.w = w;
	grdCurScreen->scCanvas.cvBitmap.bmProps.h = h;
	grdCurScreen->scCanvas.cvBitmap.bmBPP = 1;
	grdCurScreen->scCanvas.cvBitmap.bmProps.rowSize = w;

	gamefont_choose_game_font(w,h);

	return 0;
}

int GrInit(int mode)
{
	//int org_gamma;
	int retcode;
	//HRESULT hr;

	// Only do this function once!
	if (gameStates.gfx.bInstalled==1)
		return -1;

	MALLOC( grdCurScreen,grsScreen,1 );
	memset( grdCurScreen, 0, sizeof(grsScreen);

	// Set the mode.
	if ((retcode=GrSetMode(mode)))
	{
		return retcode;
	}

	// Set all the screen, canvas, and bitmap variables that
	// aren't set by the GrSetMode call:
	grdCurScreen->scCanvas.cvColor = 0;
	grdCurScreen->scCanvas.cvDrawMode = 0;
	grdCurScreen->scCanvas.cvFont = NULL;
	grdCurScreen->scCanvas.cvFontFgColor = 0;
	grdCurScreen->scCanvas.cvFontBgColor = 0;
	GrSetCurrentCanvas( &grdCurScreen->scCanvas );

	// Set flags indicating that this is installed.
	gameStates.gfx.bInstalled = 1;

	return 0;
}

void gr_upixel( int x, int y )
{
	gr_bm_upixel(&grdCurCanv->cvBitmap, x, y, (unsigned char)COLOR);
#if 0
	grsBitmap * bm = &grdCurCanv->cvBitmap;
	Win32_Rect (
		//x + bm->bmProps.x, y + bm->bmProps.y,
		//x + bm->bmProps.x, y + bm->bmProps.y,
		x, y, x, y,
		bm->bmTexBuf, COLOR);
#endif
}

void gr_bm_upixel( grsBitmap * bm, int x, int y, unsigned char color )
{
	switch (bm->bmProps.nType)
	{
	case BM_LINEAR:
		bm->bmTexBuf[ bm->bmProps.rowSize*y+x ] = color;
		break;

	case BM_DIRECTX:
		{
			unsigned char *p = gr_current_pal + color * 3;
		Win32_Rect (
			x, y, x, y,
			//x + bm->bmProps.x, y + bm->bmProps.y,
			//x + bm->bmProps.x, y + bm->bmProps.y,
				(int)bm->bmTexBuf, color);
		}
		break;

	default:
		Assert (FALSE);
		break;
	}
}

RGBQUAD w32lastrgb[256];

void GrUpdate (0)
{
	HDC hdc;
	unsigned char *p;
	int i;

	p = gr_current_pal;
	for (i = 0; i < 256; i++) {
		w32lastrgb[i].rgbRed = *p++ * 4;
		w32lastrgb[i].rgbGreen = *p++ * 4;
		w32lastrgb[i].rgbBlue = *p++ * 4;
	}
	hdc = CreateCompatibleDC(NULL);
	SelectObject(hdc, screen_bitmap);
	SetDIBColorTable(hdc, 0, 256, w32lastrgb);

	BlitToPrimary(hdc);
	DeleteDC(hdc);
}

void show_dd_error(HRESULT hr, char *loc)
{
	char buf[512], len;

	strcpy(buf, loc);
	len = (int) strlen(buf);
	#ifndef _DEBUG
	sprintf(buf + len, "%x", hr);
	#else
	GetDDErrorString(hr, buf + len, sizeof(buf) - len);
	#endif
	Error(buf);
}

unsigned char *createdib(void)
{
	BITMAPINFO *bmHdr;
	HDC hdc;
	int i;
	unsigned char *p;
	unsigned char *buffer;

	if (!(bmHdr = D2_ALLOC(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD))))
		return NULL;

	memset(bmHdr, 0, sizeof(*bmHdr);
	bmHdr->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmHdr->bmiHeader.biWidth = grdCurScreen->scCanvas.cvBitmap.bmProps.w;
	bmHdr->bmiHeader.biHeight = -grdCurScreen->scCanvas.cvBitmap.bmProps.h;
	bmHdr->bmiHeader.biPlanes = 1;
	bmHdr->bmiHeader.biBitCount = 8;
	bmHdr->bmiHeader.biCompression = BI_RGB;

	p = gr_current_pal;
	for (i = 0; i < 256; i++) {
		#if 0
		((short *)bmHdr->bmiColors)[i] = i;
		#else
		bmHdr->bmiColors[i].rgbRed = (*p++) << 2;
		bmHdr->bmiColors[i].rgbGreen = (*p++) << 2;
		bmHdr->bmiColors[i].rgbBlue = (*p++) << 2;
		bmHdr->bmiColors[i].rgbReserved = 0;
		#endif
	}
	hdc = CreateCompatibleDC(NULL);
	if (!(screen_bitmap = CreateDIBSection(hdc, bmHdr, DIB_RGB_COLORS,
		&buffer, NULL, 0))) {
			int err = GetLastError();
			char buf[256];
			sprintf(buf, "CreateDISection():%d", err);
			MessageBox(g_hWnd, buf, NULL, MB_OK);
	}
	DeleteDC(hdc);
	D2_FREE(bmHdr);
	return buffer;
}

void Win32_BlitLinearToDirectX_bm(grsBitmap *bm, int sx, int sy, 
	int w, int h, int dx, int dy, int masked) {
	HDC hdc;
	unsigned char *src, *dest, *rle_w;
	int i;

	dest = backbuffer + dy * 320 + dx;

	if (bm->bmProps.flags & BM_FLAG_RLE) {
		src = bm->bmTexBuf + 4 + bm->bmProps.h;
		rle_w = bm->bmTexBuf + 4;
		while (sy--)
			src += (int)*rle_w++;
		if (masked) {
			for (i = h; i--; ) {
				gr_rle_expand_scanline_masked(dest, src, sx, sx + w - 1);
				src += (int)*rle_w++;
				dest += 320;
			}
		} else {
			for (i = h; i--; ) {
				gr_rle_expand_scanline(dest, src, sx, sx + w - 1);
				src += (int)*rle_w++;
				dest += 320;
			}
		}
	} else {
		src = bm->bmTexBuf + sy * bm->bmProps.rowSize + sx;
		if (masked) {
			for (i = h; i--; ) {
				gr_linear_rep_movsdm(src, dest, w);
				src += bm->bmProps.rowSize;
				dest += 320;
			}
		} else {
			for (i = h; i--; ) {
				gr_linear_movsd(src, dest, w);
				src += bm->bmProps.rowSize;
				dest += 320;
			}
		}
	}
	hdc = CreateCompatibleDC(NULL);
	SelectObject(hdc, screen_bitmap);
	SetDIBColorTable(hdc, 0, 256, w32lastrgb);
	BlitToPrimaryRect(hdc, dx, dy, w, h, grdCurCanv->cvBitmap.bmTexBuf);
	DeleteDC(hdc);
}

void Win32_MakePalVisible(void) {
	GrUpdate (0);
}

void Win32_InvalidatePages(void) {
	ResetCockpit();
	InitGauges();
}
