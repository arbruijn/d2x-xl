// Windows video functions.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wtypes.h>
#include <ddraw.h>
#ifdef __GCC__
#include <Windows32/Errors.h>
#endif
#include "gr.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"
#include "vers_id.h"

#include "gamefont.h"

//added 10/05/98 by Matt Mueller - make fullscreen mode optional
#include "args.h"

//removed 07/11/99 by adb - now option
////added 02/20/99 by adb - put descent in a window, sort of. Needed for debugging
////(needs a 256 color mode to be useful)
//#ifdef _DEBUG
//#define DD_NOT_EXCL
//#endif
//end remove - adb

char *backbuffer = NULL;

int gameStates.gfx.bInstalled = 0;

// Min without sideeffects.
#ifdef _MSC_VER
#define inline __inline
#endif

#undef min
inline static int min(int x, int y) { return x < y ? x : y; }

// Windows specific
HINSTANCE hInst;
HWND g_hWnd;
LPDIRECTDRAW            lpDD;           
LPDIRECTDRAWSURFACE     lpDDSPrimary;   
LPDIRECTDRAWSURFACE     lpDDSOne;       
LPDIRECTDRAWPALETTE     lpDDPal;
PALETTEENTRY pe[256];

//added 02/20/99 by adb - put descent in a window, sort of. Needed for debugging
//(needs a 256 color mode to be useful)
//#define DD_NOT_EXCL

void GrPaletteStepClear(); // Function prototype for GrInit;


static char *DDerror(int code)
{
	static char *error;
	switch (code) {
/*                case DDERR_GENERIC:
			error = "Undefined error!";
                        break;*/
		case DDERR_EXCEPTION:
			error = "Exception encountered";
			break;
		case DDERR_INVALIDOBJECT:
			error = "Invalid tObject";
			break;
/*                case DDERR_INVALIDPARAMS:
			error = "Invalid parameters";
                        break;*/
		case DDERR_NOTFOUND:
			error = "Object not found";
			break;
		case DDERR_INVALIDRECT:
			error = "Invalid rectangle";
			break;
		case DDERR_INVALIDCAPS:
			error = "Invalid caps member";
			break;
		case DDERR_INVALIDPIXELFORMAT:
			error = "Invalid pixel format";
			break;
/*                case DDERR_OUTOFMEMORY:
			error = "Out of memory";
                        break;*/
		case DDERR_OUTOFVIDEOMEMORY:
			error = "Out of video memory";
			break;
		case DDERR_SURFACEBUSY:
			error = "Surface busy";
			break;
		case DDERR_SURFACELOST:
			error = "Surface was lost";
			break;
		case DDERR_WASSTILLDRAWING:
			error = "DirectDraw is still drawing";
			break;
		case DDERR_INVALIDSURFACETYPE:
			error = "Invalid surface nType";
			break;
		case DDERR_NOEXCLUSIVEMODE:
			error = "Not in exclusive access mode";
			break;
		case DDERR_NOPALETTEATTACHED:
			error = "No palette attached";
			break;
		case DDERR_NOPALETTEHW:
			error = "No palette hardware";
			break;
		case DDERR_NOT8BITCOLOR:
			error = "Not 8-bit color";
			break;
		case DDERR_EXCLUSIVEMODEALREADYSET:
			error = "Exclusive mode was already set";
			break;
		case DDERR_HWNDALREADYSET:
			error = "Window handle already set";
			break;
		case DDERR_HWNDSUBCLASSED:
			error = "Window handle is subclassed";
			break;
		case DDERR_NOBLTHW:
			error = "No blit hardware";
			break;
		case DDERR_IMPLICITLYCREATED:
			error = "Surface was implicitly created";
			break;
		case DDERR_INCOMPATIBLEPRIMARY:
			error = "Incompatible primary surface";
			break;
		case DDERR_NOCOOPERATIVELEVELSET:
			error = "No cooperative level set";
			break;
		case DDERR_NODIRECTDRAWHW:
			error = "No DirectDraw hardware";
			break;
		case DDERR_NOEMULATION:
			error = "No emulation available";
			break;
		case DDERR_NOFLIPHW:
			error = "No flip hardware";
			break;
		case DDERR_NOTFLIPPABLE:
			error = "Surface not flippable";
			break;
		case DDERR_PRIMARYSURFACEALREADYEXISTS:
			error = "Primary surface already exists";
			break;
		case DDERR_UNSUPPORTEDMODE:
			error = "Unsupported mode";
			break;
		case DDERR_WRONGMODE:
			error = "Surface created in different mode";
			break;
/*                case DDERR_UNSUPPORTED:
			error = "Operation not supported";
                        break;*/
		default:
                error = "unknown";
			break;
	}
	return error;
}


void GrUpdate (0)
{
  DDSURFACEDESC       ddsd;
  HRESULT             ddrval;
  int i;
  int w, h;
  char *j;
  char *k;

  ddsd.dwSize=sizeof(ddsd);
  ddrval=IDirectDrawSurface_Lock(lpDDSPrimary,NULL,&ddsd,0,NULL);
  if (ddrval!=DD_OK) {
   Assert(ddrval==DD_OK);
  }

  j=backbuffer; k=ddsd.lpSurface;
  h=grdCurScreen->sc_canvas.cv_bitmap.bm_props.h;
  w=grdCurScreen->sc_canvas.cv_bitmap.bm_props.w;
  for (i=0; i<h; i++) {
    memcpy(k, j, w);
  j+=grdCurScreen->sc_canvas.cv_bitmap.bm_props.rowsize;
  //j+=ddsd.dwWidth;
#ifdef NONAMELESSUNION
  k+=ddsd.u1.lPitch;
#else
  k+=ddsd.lPitch;
#endif
  }
  IDirectDrawSurface_Unlock(lpDDSPrimary,NULL);
/*   while (1)
 {
   ddrval=IDirectDrawSurface_Flip(lpDDSPrimary,NULL,0);
   if (ddrval == DD_OK)
   {
     //printf("Flip was successful\n");
     break;
   }
   if (ddrval == DDERR_SURFACELOST)
   {
      //printf("surface was lost\n");
     ddrval=IDirectDrawSurface_Restore(lpDDSPrimary);
     if (ddrval != DD_OK)
     {
       //printf("restore failed\n"); 
       break;
     }
   }
   if (ddrval == DDERR_WASSTILLDRAWING )
   {
     //printf("was still drawing\n");
     break;
   }
 }*/
}



int GrSetMode(u_int32_t mode)
{
        DDSURFACEDESC       ddsd;
//        DDSURFACEDESC       DDSDesc;
//        DDSCAPS             ddcaps;
        HRESULT             ddrval;
	unsigned int w,h;
	
	if (mode<=0)
		return 0;

	w=SM_W(mode);
	h=SM_H(mode);

        if(lpDDSPrimary!=NULL)
        {
            IDirectDrawSurface_Release(lpDDSPrimary);
            lpDDSPrimary=NULL;
        }

        if (backbuffer) d_free(backbuffer);


	//changed 07/11/99 by adb - nonfullscreen mode now option
	if (!FindArg("-semiwin"))
	{
        ddrval=IDirectDraw_SetDisplayMode(lpDD,w,h,8);

        if (ddrval!=DD_OK)
        {
          fprintf(stderr, "Hmmm... I had a problem changing screen modes... is %ix%ix8 supported? If so, try again... :-( %s\n",w,h, DDerror(ddrval);
          return -3; // This is **not** good...
        }
	}
	//end changes - adb
       ddsd.dwSize=sizeof(ddsd);
       ddsd.dwFlags=DDSD_CAPS /*| DDSD_BACKBUFFERCOUNT*/;
       ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE /*| DDSCAPS_FLIP | DDSCAPS_COMPLEX*/;
       //ddsd.dwBackBufferCount=1;

       ddrval=IDirectDraw_CreateSurface(lpDD,&ddsd,&lpDDSPrimary,NULL);
       if(ddrval!=DD_OK)
       {
        return -4;
       }

#if 0
       memset(&ddcaps,0,sizeof(ddcaps);
//       ddcaps.dwSize=sizeof(ddcaps);
       ddcaps.dwCaps=DDSCAPS_BACKBUFFER;

       ddrval=IDirectDrawSurface_GetAttachedSurface(lpDDSPrimary,&ddcaps,&lpDDSOne);
       Assert(ddrval==DD_OK);
       if(lpDDSOne==NULL)
       {
         return -5;
       }
#endif

       ddrval=IDirectDraw_CreatePalette(lpDD,DDPCAPS_8BIT | DDPCAPS_INITIALIZE | DDPCAPS_ALLOW256,pe,&lpDDPal,NULL);
       Assert(ddrval==DD_OK);
       if(ddrval!=DD_OK)
       {
         return FALSE;
       }

       IDirectDrawSurface_SetPalette(lpDDSPrimary,lpDDPal);
//       IDirectDrawSurface_SetPalette(lpDDSOne,lpDDPal);

       GrPaletteStepClear();

       memset( grdCurScreen, 0, sizeof(grs_screen);
       grdCurScreen->sc_mode = mode;
       grdCurScreen->sc_w = w;
       grdCurScreen->sc_h = h;
       grdCurScreen->sc_aspect = FixDiv(grdCurScreen->sc_w*3,grdCurScreen->sc_h*4);
       grdCurScreen->sc_canvas.cv_bitmap.bm_props.x = 0;
       grdCurScreen->sc_canvas.cv_bitmap.bm_props.y = 0;
       grdCurScreen->sc_canvas.cv_bitmap.bm_props.w = w;
       grdCurScreen->sc_canvas.cv_bitmap.bm_props.h = h;
		 grdCurScreen->sc_canvas.cv_bitmap.bm_bpp = 1;
       grdCurScreen->sc_canvas.cv_bitmap.bm_props.nType = BM_LINEAR;

       backbuffer = d_malloc(w*h);
       memset(backbuffer, 0, w*h);
       grdCurScreen->sc_canvas.cv_bitmap.bm_texBuf = (unsigned char *)backbuffer;

       ddsd.dwSize=sizeof(ddsd);
       ddrval=IDirectDrawSurface_Lock(lpDDSPrimary,NULL,&ddsd,0,NULL);
       if(ddrval!=DD_OK)
       {
        return -6;
       }
       
       // bm_props.rowsize is for backbuffer, so always w -- adb
       grdCurScreen->sc_canvas.cv_bitmap.bm_props.rowsize = w;
       //grdCurScreen->sc_canvas.cv_bitmap.bm_props.rowsize = (short)ddsd.lPitch;

       memset(ddsd.lpSurface,0,w*h); // Black the canvas out to stop nasty kludgy display
       IDirectDrawSurface_Unlock(lpDDSPrimary,NULL);

       GrSetCurrentCanvas(NULL);
	
	   gamefont_choose_game_font(w,h);

       //printf("Successfully completed set_mode\n");
       return 0;
}



void Win32_DoSetPalette (PALETTEENTRY *rgpe)
{
	IDirectDraw_WaitForVerticalBlank(lpDD,DDWAITVB_BLOCKBEGIN,NULL);
	IDirectDrawPalette_SetEntries(lpDDPal,0,0,256,rgpe);
}

void Win32_DoGetPalette (PALETTEENTRY *rgpe)
{
	IDirectDrawPalette_GetEntries(lpDDPal,0,0,256,rgpe);
}

//added 07/11/99 by adb for d3d
void Win32_MakePalVisible(void)
{
}
//end additions - adb

void GrClose(void);

int GrInit(int mode)
{
 int retcode;
 	// Only do this function once!
	if (gameStates.gfx.bInstalled==1)
		return -1;
	MALLOC( grdCurScreen,grs_screen,1 );
	memset( grdCurScreen, 0, sizeof(grs_screen);

	// Set the mode.
	if ((retcode=GrSetMode(mode)))
	{
		return retcode;
	}
	grdCurScreen->sc_canvas.cv_color = 0;
	grdCurScreen->sc_canvas.cv_drawmode = 0;
	grdCurScreen->sc_canvas.cv_font = NULL;
	grdCurScreen->sc_canvas.cv_font_fg_color = 0;
	grdCurScreen->sc_canvas.cv_font_bg_color = 0;
	GrSetCurrentCanvas( &grdCurScreen->sc_canvas );

	gameStates.gfx.bInstalled = 1;
	// added on 980913 by adb to add cleanup
	atexit(GrClose);
	// end changes by adb

	return 0;
}

void GrClose(void)
{
	if (gameStates.gfx.bInstalled==1)
	{
		gameStates.gfx.bInstalled = 0;
		d_free(grdCurScreen);
		d_free(backbuffer);
	}
}


