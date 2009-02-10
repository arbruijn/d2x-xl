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
 * Stuff for rendering the HUD
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "descent.h"
#include "error.h"
#include "rendermine.h"
#include "screens.h"
#include "cockpit.h"
#include "gamefont.h"
#include "newdemo.h"
#include "text.h"
#include "gr.h"
#include "ogl_render.h"
#include "endlevel.h"
#include "playsave.h"
#include "automap.h"
#include "gamepal.h"
#include "lightning.h"
#include "menubackground.h"

#if DBG
extern int Debug_pause;				//John's debugging pause system
#endif

#if DBG
extern int bSavingMovieFrames;
#else
#define bSavingMovieFrames 0
#endif

//------------------------------------------------------------------------------

static inline bool GuidedMissileActive (void)
{
CObject *gmObjP = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP;
return gmObjP &&
		 (gmObjP->info.nType == OBJ_WEAPON) &&
		 (gmObjP->info.nId == GUIDEDMSL_ID) &&
		 (gmObjP->info.nSignature == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature);
}

//------------------------------------------------------------------------------

//draw a crosshair for the guided missile
void DrawGuidedCrosshair (void)
{
CCanvas::Current ()->SetColorRGBi (RGB_PAL (0, 31, 0));
int w = CCanvas::Current ()->Width ()>>5;
if (w < 5)
	w = 5;
int h = I2X (w) / screen.Aspect ();
int x = CCanvas::Current ()->Width () / 2;
int y = CCanvas::Current ()->Height () / 2;
#if 1
	x = I2X (x);
	y = I2X (y);
	w = I2X (w / 2);
	h = I2X (h / 2);
	OglDrawLine (x - w, y, x + w, y);
	OglDrawLine (x, y - h, x, y + h);
#else
	OglDrawLine (I2X (x-w/2), I2X (y), I2X (x+w/2), I2X (y));
	OglDrawLine (I2X (x), I2X (y-h/2), I2X (x), I2X (y+h/2));
#endif
}

//------------------------------------------------------------------------------

#if 0
extern int gr_bitblt_dest_step_shift;
extern int gr_wait_for_retrace;
extern int gr_bitblt_double;
extern int SW_drawn [2], SW_x [2], SW_y [2], SW_w [2], SW_h [2];

//------------------------------------------------------------------------------

void ExpandRow (ubyte * dest, ubyte * src, int num_src_pixels)
{
	int i;

for (i = 0; i < num_src_pixels; i++) {
	*dest++ = *src;
	*dest++ = *src++;
	}
}

//------------------------------------------------------------------------------
// doubles the size in x or y of a bitmap in place.
void GameExpandBitmap (CBitmap * bmP, uint flags)
{
	int i;
	ubyte * dptr, * sptr;

switch (flags & 3) {
	case 2:	// expand x
		Assert (bmP->RowSize () == bmP->Width ()*2);
		dptr = &bmP->Buffer () [(bmP->Height ()-1)*bmP->RowSize ()];
		for (i=bmP->Height ()-1; i>=0; i--) {
			ExpandRow (dptr, dptr, bmP->Width ());
			dptr -= bmP->RowSize ();
			}
		bmP->SetWidth (bmP->Width () *  2);
		break;

	case 1:	// expand y
		dptr = &bmP->Buffer () [(2* (bmP->Height ()-1)+1)*bmP->RowSize ()];
		sptr = &bmP->Buffer () [(bmP->Height ()-1)*bmP->RowSize ()];
		for (i=bmP->Height ()-1; i>=0; i--) {
			memcpy (dptr, sptr, bmP->Width ());
			dptr -= bmP->RowSize ();
			memcpy (dptr, sptr, bmP->Width ());
			dptr -= bmP->RowSize ();
			sptr -= bmP->RowSize ();
			}
		bmP->SetHeight (bmP->Height () *  2);
		break;

	case 3:	// expand x & y
		Assert (bmP->RowSize () == bmP->Width ()*2);
		dptr = &bmP->Buffer () [(2* (bmP->Height ()-1)+1) * bmP->RowSize ()];
		sptr = &bmP->Buffer () [(bmP->Height ()-1) * bmP->RowSize ()];
		for (i=bmP->Height ()-1; i>=0; i--) {
			ExpandRow (dptr, sptr, bmP->Width ());
			dptr -= bmP->RowSize ();
			ExpandRow (dptr, sptr, bmP->Width ());
			dptr -= bmP->RowSize ();
			sptr -= bmP->RowSize ();
			}
		bmP->SetWidth (bmP->Width () *  2);
		bmP->SetHeight (bmP->Height () *  2);
		break;
	}
}

//------------------------------------------------------------------------------
//render a frame for the game in stereo
void RenderStereoFrame (void)
{
	int dw, dh, sw, sh;
	fix save_aspect;
	fix actual_eye_width;
	int actual_eye_offset;
	CCanvas RenderCanvas [2];
	int bNoDrawHUD = 0, bGMView = 0;
	CObject *gmObjP;

	save_aspect = screen.Aspect ();
	screen.Aspect () * = 2;	//Muck with aspect ratio

	sw = dw = gameStates.render.vr.buffers.render [0].Width ();
	sh = dh = gameStates.render.vr.buffers.render [0].Height ();

	if (gameStates.render.vr.nLowRes & 1) {
		sh /= 2;
		screen.Aspect () * = 2;  //Muck with aspect ratio
	}
	if (gameStates.render.vr.nLowRes & 2) {
		sw /= 2;
		screen.Aspect () /= 2;  //Muck with aspect ratio
	}

	GrInitSubCanvas (RenderCanvas [0, gameStates.render.vr.buffers.render, 0, 0, sw, sh);
	GrInitSubCanvas (RenderCanvas + 1, gameStates.render.vr.buffers.render + 1, 0, 0, sw, sh);

	// Draw the left eye's view
	if (gameStates.render.vr.nEyeSwitch) {
		actual_eye_width = -gameStates.render.vr.xEyeWidth;
		actual_eye_offset = -gameStates.render.vr.nEyeOffset;
	} else {
		actual_eye_width = gameStates.render.vr.xEyeWidth;
		actual_eye_offset = gameStates.render.vr.nEyeOffset;
	}

	if ((bGMView = GuidedInMainView ()))
		actual_eye_offset = 0;

	CCanvas::SetCurrent (&RenderCanvas [0]);

	if (bGMView) {
		char *msg = "Guided Missile View";
		CObject *viewerSave = gameData.objs.viewerP;
		int w, h, aw;

		gameData.objs.viewerP = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP;
		UpdateRenderedData (0, gameData.objs.viewerP, 0, 0);
		RenderFrame (0, 0);
		WakeupRenderedObjects (gameData.objs.viewerP, 0);
		gameData.objs.viewerP = viewerSave;

		fontManager.SetCurrent (GAME_FONT);    //GAME_FONT);
		fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
		fontManager.Current ()->StringSize (msg, w, h, aw);

		GrPrintF (NULL, (CCanvas::Current ()->Width ()-w)/2, 3, msg);

		glDisable (GL_DEPTH_TEST);
		DrawGuidedCrosshair ();
		glEnable (GL_DEPTH_TEST);
		HUDRenderMessageFrame ();
		bNoDrawHUD = 1;
	}
	else if (gameStates.render.bRearView)
		RenderFrame (actual_eye_width, 0);	// switch eye positions for rear view
	else
		RenderFrame (-actual_eye_width, 0);		// Left eye

	if (gameStates.render.vr.nLowRes)
		GameExpandBitmap (&RenderCanvas [0].Bitmap (), gameStates.render.vr.nLowRes);

 {	//render small window into left eye's canvas
		CCanvas *save=CCanvas::Current ();
		fix save_aspect2 = screen.Aspect ();
		screen.Aspect () = save_aspect*2;
		SW_drawn [0] = SW_drawn [1] = 0;
		ShowExtraViews ();
		CCanvas::SetCurrent (save);
		screen.Aspect () = save_aspect2;
	}

//NEWVR
	if (actual_eye_offset > 0) {
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		OglDrawFilledRect (CCanvas::Current ()->Width ()-labs (actual_eye_offset)*2, 0,
               CCanvas::Current ()->Width ()-1, CCanvas::Current ()->Height ());
	} else if (actual_eye_offset < 0) {
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		OglDrawFilledRect (0, 0, labs (actual_eye_offset)*2-1, CCanvas::Current ()->Height ());
	}

	if (gameStates.render.vr.bShowHUD && !bNoDrawHUD) {
		CCanvas tmp;
		if (actual_eye_offset < 0) {
			GrInitSubCanvas (&tmp, CCanvas::Current (), labs (actual_eye_offset*2), 0, CCanvas::Current ()->Width ()- (labs (actual_eye_offset)*2), CCanvas::Current ()->Height ());
		} else {
			GrInitSubCanvas (&tmp, CCanvas::Current (), 0, 0, CCanvas::Current ()->Width ()- (labs (actual_eye_offset)*2), CCanvas::Current ()->Height ());
		}
		CCanvas::SetCurrent (&tmp);
	}


	// Draw the right eye's view
	CCanvas::SetCurrent (&RenderCanvas [1]);

	if (gameOpts->render.cockpit.bGuidedInMainView && GuidedMissileActive ())
		RenderCanvas [0].Bitmap ().BlitClipped (0, 0);
	else {
		if (gameStates.render.bRearView)
			RenderFrame (-actual_eye_width, 0);	// switch eye positions for rear view
		else
			RenderFrame (actual_eye_width, 0);		// Right eye

		if (gameStates.render.vr.nLowRes)
			GameExpandBitmap (&RenderCanvas [1].Bitmap (), gameStates.render.vr.nLowRes);
		}


 {	//copy small window from left eye
	CCanvas temp;
	int w;
	for (w=0;w<2;w++) {
		if (SW_drawn [w]) {
			GrInitSubCanvas (&temp, &RenderCanvas [0], SW_x [w], SW_y [w], SW_w [w], SW_h [w]);
			temp.Bitmap ().BlitClipped (SW_x [w] + actual_eye_offset * 2, SW_y [w]);
			}
		}
	}

//NEWVR
	if (actual_eye_offset>0) {
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		OglDrawFilledRect (0, 0, labs (actual_eye_offset)*2-1, CCanvas::Current ()->Height ());
	} else if (actual_eye_offset < 0) {
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		OglDrawFilledRect (CCanvas::Current ()->Width ()-labs (actual_eye_offset)*2, 0,
               CCanvas::Current ()->Width ()-1, CCanvas::Current ()->Height ());
	}

//NEWVR (Add the next 2 lines)
	if (gameStates.render.vr.bShowHUD && !bNoDrawHUD) {
		CCanvas tmp;
		if (actual_eye_offset > 0) {
			GrInitSubCanvas (&tmp, CCanvas::Current (), labs (actual_eye_offset*2), 0, CCanvas::Current ()->Width ()- (labs (actual_eye_offset)*2), CCanvas::Current ()->Height ());
		} else {
			GrInitSubCanvas (&tmp, CCanvas::Current (), 0, 0, CCanvas::Current ()->Width ()- (labs (actual_eye_offset)*2), CCanvas::Current ()->Height ());
		}
		CCanvas::SetCurrent (&tmp);
	}


	// Draws white and black registration encoding lines
	// and Accounts for pixel-shift adjustment in upcoming bitblts
	if (gameStates.render.vr.bUseRegCode) {
		int width, height, quarter;

		width = RenderCanvas [0].Width ();
		height = RenderCanvas [0].Bitmap ().Height ();
		quarter = width / 4;

		// black out left-hand CSide of left page

		// draw registration code for left eye
		if (gameStates.render.vr.nEyeSwitch)
			CCanvas::SetCurrent (&RenderCanvas [1]);
		else
			CCanvas::SetCurrent (&RenderCanvas [0]);
		CCanvas::Current ()->SetColorRGB (255, 255, 255, 255);
		DrawScanLineClipped (0, quarter, height-1);
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		DrawScanLineClipped (quarter, width-1, height-1);

		if (gameStates.render.vr.nEyeSwitch)
			CCanvas::SetCurrent (&RenderCanvas [0]);
		else
			CCanvas::SetCurrent (&RenderCanvas [1]);
		CCanvas::Current ()->SetColorRGB (255, 255, 255, 255);
		DrawScanLineClipped (0, quarter*3, height-1);
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		DrawScanLineClipped (quarter*3, width-1, height-1);
   }

 		// Copy left eye, then right eye
	if (gameStates.render.vr.nScreenFlags&VRF_USE_PAGING)
		gameStates.render.vr.nCurrentPage = !gameStates.render.vr.nCurrentPage;
	else
		gameStates.render.vr.nCurrentPage = 0;
	CCanvas::SetCurrent (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);

//NEWVR

	if (gameStates.render.vr.bEyeOffsetChanged > 0) {
		gameStates.render.vr.bEyeOffsetChanged--;
		GrClearCanvas (0);
	}

	sw = dw = gameStates.render.vr.buffers.render [0].Width ();
	sh = dh = gameStates.render.vr.buffers.render [0].Height ();

	// Copy left eye, then right eye
	gr_bitblt_dest_step_shift = 1;		// Skip every other scanline.

	if (gameStates.render.vr.nRenderMode == VR_INTERLACED)  {
		if (actual_eye_offset > 0) {
			int xoff = labs (actual_eye_offset);
			BlitToBitmap (dw-xoff, dh, xoff, 0, 0, 0, &RenderCanvas [0].Bitmap (), &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ());
			BlitToBitmap (dw-xoff, dh, 0, 1, xoff, 0, &RenderCanvas [1].Bitmap (), &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ());
		} else if (actual_eye_offset < 0) {
			int xoff = labs (actual_eye_offset);
			BlitToBitmap (dw-xoff, dh, 0, 0, xoff, 0, &RenderCanvas [0].Bitmap (), &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ());
			BlitToBitmap (dw-xoff, dh, xoff, 1, 0, 0, &RenderCanvas [1].Bitmap (), &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ());
		} else {
			BlitToBitmap (dw, dh, 0, 0, 0, 0, &RenderCanvas [0].Bitmap (), &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ());
			BlitToBitmap (dw, dh, 0, 1, 0, 0, &RenderCanvas [1].Bitmap (), &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ());
		}
	} else if (gameStates.render.vr.nRenderMode == VR_AREA_DET) {
		// VFX copy
		BlitToBitmap (dw, dh, 0,  gameStates.render.vr.nCurrentPage, 0, 0, &RenderCanvas [0].Bitmap (), &gameStates.render.vr.buffers.screenPages [0].Bitmap ());
		BlitToBitmap (dw, dh, dw, gameStates.render.vr.nCurrentPage, 0, 0, &RenderCanvas [1].Bitmap (), &gameStates.render.vr.buffers.screenPages [0].Bitmap ());
	} else {
		Int3 ();		// Huh?
	}

	gr_bitblt_dest_step_shift = 0;

	//if (Game_vfxFlag)
	//	vfx_set_page (gameStates.render.vr.nCurrentPage);		// 0 or 1
	//else
		if (gameStates.render.vr.nScreenFlags&VRF_USE_PAGING) {
			gr_wait_for_retrace = 0;

//	Added by Samir from John's code
		if ((gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ().Mode () == BM_MODEX)) {
			int old_x, old_y, new_x;
			old_x = gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Width ();
			old_y = gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ().Height ();
			new_x = old_y*gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ().RowSize ();
			new_x += old_x/4;
			gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Width () = new_x;
			gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ().Height () = 0;
			gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ().Mode () = BM_SVGA;
			GrShowCanvas (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
			gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ().Mode () = BM_MODEX;
			gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Width () = old_x;
			gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].Bitmap ().Height () = old_y;
		} else {
			GrShowCanvas (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
		}
		gr_wait_for_retrace = 1;
	}
	screen.Aspect ()=save_aspect;
}
#endif

//------------------------------------------------------------------------------

int RenderMissileView (void)
{
	CObject	*objP = NULL;

if (GuidedMslView (&objP)) {
	if (gameOpts->render.cockpit.bGuidedInMainView) {
		gameStates.render.nRenderingType = 6 + (1 << 4);
		cockpit->RenderWindow (1, gameData.objs.viewerP, 0, WBUMSL, "SHIP");
		}
	else {
		gameStates.render.nRenderingType = 1+ (1 << 4);
		cockpit->RenderWindow (1, objP, 0, WBU_GUIDED, "GUIDED");
	   }
	return 1;
	}
else {
	if (objP) {		//used to be active
		if (!gameOpts->render.cockpit.bGuidedInMainView)
			cockpit->RenderWindow (1, NULL, 0, WBU_STATIC, NULL);
		gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP = NULL;
		}
	if (gameData.objs.missileViewerP && !gameStates.render.bChaseCam) {		//do missile view
		HUDMessage (0, "missile view");
		static int mslViewerSig = -1;
		if (mslViewerSig == -1)
			mslViewerSig = gameData.objs.missileViewerP->info.nSignature;
		if (gameOpts->render.cockpit.bMissileView &&
			 (gameData.objs.missileViewerP->info.nType != OBJ_NONE) &&
			 (gameData.objs.missileViewerP->info.nSignature == mslViewerSig)) {
  			gameStates.render.nRenderingType = 2 + (1 << 4);
			cockpit->RenderWindow (1, gameData.objs.missileViewerP, 0, WBUMSL, "MISSILE");
			return 1;
			}
		else {
			gameData.objs.missileViewerP = NULL;
			mslViewerSig = -1;
			gameStates.render.nRenderingType = 255;
			cockpit->RenderWindow (1, NULL, 0, WBU_STATIC, NULL);
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void RenderMonoFrame (void)
{
	CCanvas		frameWindow;
	int			bExtraInfo = 1;

gameStates.render.vr.buffers.screenPages [0].SetupPane (
	&frameWindow,
	gameStates.render.vr.buffers.subRender [0].Left (),
	gameStates.render.vr.buffers.subRender [0].Top (),
	gameStates.render.vr.buffers.subRender [0].Width (),
	gameStates.render.vr.buffers.subRender [0].Height ());
CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);
lightningManager.SetLights ();
if (gameOpts->render.cockpit.bGuidedInMainView && GuidedMissileActive ()) {
	int w, h, aw;
	const char *msg = "Guided Missile View";
	CObject *viewerSave = gameData.objs.viewerP;

   if (gameStates.render.cockpit.nType == CM_FULL_COCKPIT) {
		gameStates.render.cockpit.bBigWindowSwitch = 1;
		gameStates.render.cockpit.bRedraw = 1;
		cockpit->Activate (CM_STATUS_BAR);
		return;
		}
  	gameData.objs.viewerP = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP;
	UpdateRenderedData (0, gameData.objs.viewerP, 0, 0);
	if (cameraManager.Render ())
		CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);
	RenderFrame (0, 0);
  	WakeupRenderedObjects (gameData.objs.viewerP, 0);
	gameData.objs.viewerP = viewerSave;
	fontManager.SetCurrent (GAME_FONT);    //GAME_FONT);
	fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
	fontManager.Current ()->StringSize (msg, w, h, aw);
	GrPrintF (NULL, (CCanvas::Current ()->Width () - w) / 2, 3, msg);
	DrawGuidedCrosshair ();
	HUDRenderMessageFrame ();
	bExtraInfo = 0;
	}
else {
	if (gameStates.render.cockpit.bBigWindowSwitch) {
		gameStates.render.cockpit.bRedraw = 1;
		cockpit->Activate (CM_FULL_COCKPIT);
		gameStates.render.cockpit.bBigWindowSwitch = 0;
		return;
		}
	UpdateRenderedData (0, gameData.objs.viewerP, gameStates.render.bRearView, 0);
	if (cameraManager.Render ())
		CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);
	RenderFrame (0, 0);
	}
CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);
cockpit->Render (bExtraInfo);
console.Draw ();
OglSwapBuffers (0, 0);
if (gameStates.app.bSaveScreenshot)
	SaveScreenShot (NULL, 0);
}

//------------------------------------------------------------------------------

#define WINDOW_W_DELTA	 ((gameData.render.window.wMax / 16)&~1)	//24	//20
#define WINDOW_H_DELTA	 ((gameData.render.window.hMax / 16)&~1)	//12	//10

#define WINDOW_MIN_W		 ((gameData.render.window.wMax * 10) / 22)	//160
#define WINDOW_MIN_H		 ((gameData.render.window.hMax * 10) / 22)

void GrowWindow (void)
{
StopTime ();
if (gameStates.render.cockpit.nType == CM_FULL_COCKPIT) {
	gameData.render.window.h = gameData.render.window.hMax;
	gameData.render.window.w = gameData.render.window.wMax;
	cockpit->Toggle ();
	HUDInitMessage (TXT_COCKPIT_F3);
	StartTime (0);
	return;
	}

if (gameStates.render.cockpit.nType != CM_STATUS_BAR && (gameStates.render.vr.nScreenFlags & VRF_ALLOW_COCKPIT)) {
	StartTime (0);
	return;
	}

if (gameData.render.window.h>=gameData.render.window.hMax || gameData.render.window.w>=gameData.render.window.wMax) {
	//gameData.render.window.w = gameData.render.window.wMax;
	//gameData.render.window[HA] = gameData.render.window.hMax;
	cockpit->Activate (CM_FULL_SCREEN);
	}
else {
	//int x, y;
	gameData.render.window.w += WINDOW_W_DELTA;
	gameData.render.window.h += WINDOW_H_DELTA;
	if (gameData.render.window.h > gameData.render.window.hMax)
		gameData.render.window.h = gameData.render.window.hMax;
	if (gameData.render.window.w > gameData.render.window.wMax)
		gameData.render.window.w = gameData.render.window.wMax;
	gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w)/2;
	gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h)/2;
	GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
	}
HUDClearMessages ();	//	@mk, 11/11/94
SavePlayerProfile ();
StartTime (0);
}

//------------------------------------------------------------------------------

extern CBitmap bmBackground;

void CopyBackgroundRect (int left, int top, int right, int bot)
{
if (right < left || bot < top)
	return;

	int x, y;
	int tileLeft, tileRight, tileTop, tileBot;
	int xOffs, yOffs;
	int xDest, yDest;

tileLeft = left / bmBackground.Width ();
tileRight = right / bmBackground.Width ();
tileTop = top / bmBackground.Height ();
tileBot = bot / bmBackground.Height ();

yOffs = top % bmBackground.Height ();
yDest = top;

for (y = tileTop;y <= tileBot; y++) {
	xOffs = left % bmBackground.Width ();
	xDest = left;
	int h = min (bot - yDest + 1, bmBackground.Height () - yOffs);
	for (x = tileLeft; x <= tileRight; x++) {
		int w = min (right - xDest + 1, bmBackground.Width () - xOffs);
		bmBackground.Blit (CCanvas::Current (), xDest, yDest, w, h, xOffs, yOffs, 1);
		xOffs = 0;
		xDest += w;
		}
	yOffs = 0;
	yDest += h;
	}
}

//------------------------------------------------------------------------------
//fills int the background surrounding the 3d window
void FillBackground (void)
{
#if 1
if (gameData.render.window.x || gameData.render.window.y) {
//	OglEndFrame ();
	CCanvas::Push ();
	CCanvas::SetCurrent (CurrentGameScreen ());
	gameStates.ogl.nLastW = CCanvas::Current ()->Width ();
	gameStates.ogl.nLastH = CCanvas::Current ()->Height ();
	glDepthMask (0);
	bmBackground.Render (CCanvas::Current (), 0, 0, CCanvas::Current ()->Width (), CCanvas::Current ()->Height (), 0, 0, -bmBackground.Width (), -bmBackground.Height ());
	glDepthMask (1);
	CCanvas::Pop ();
	gameStates.ogl.nLastW = CCanvas::Current ()->Width ();
	gameStates.ogl.nLastH = CCanvas::Current ()->Height ();
//	OglStartFrame (0, 0);
	}
#else
CCanvas::SetCurrent (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
CopyBackgroundRect (0, 0, gameData.render.window.x - 1, 2 * gameData.render.window.y + gameData.render.window.h - 1);
CopyBackgroundRect (gameData.render.window.x + gameData.render.window.w, 0, 
						  CCanvas::Current ()->Width () - 1, 2 * gameData.render.window.y + gameData.render.window.h - 1);
CopyBackgroundRect (gameData.render.window.x, 0, 
						  gameData.render.window.x + gameData.render.window.w - 1, gameData.render.window.y - 1);
CopyBackgroundRect (gameData.render.window.x, gameData.render.window.y + gameData.render.window.h, 
						  gameData.render.window.x + gameData.render.window.w - 1, 2 * gameData.render.window.y + gameData.render.window.h - 1);

if (gameStates.render.vr.nScreenFlags & VRF_USE_PAGING) {
	CCanvas::SetCurrent (&gameStates.render.vr.buffers.screenPages [!gameStates.render.vr.nCurrentPage]);
	CopyBackgroundRect (0, gameData.render.window.y - gameData.render.window.y, 
							  gameData.render.window.x - 1, 2 * gameData.render.window.y + gameData.render.window.h - 1);
	CopyBackgroundRect (gameData.render.window.x + gameData.render.window.w, 0, 
							  2 * gameData.render.window.x + gameData.render.window.w - 1, 2* gameData.render.window.y + gameData.render.window.h - 1);
	CopyBackgroundRect (gameData.render.window.x, 0, 
							  gameData.render.window.x + gameData.render.window.w - 1, gameData.render.window.y - 1);
	CopyBackgroundRect (gameData.render.window.x, gameData.render.window.y + gameData.render.window.h, 
							  gameData.render.window.x + gameData.render.window.w - 1, gameData.render.window.y + gameData.render.window.h + gameData.render.window.y - 1);
	}
#endif
}

//------------------------------------------------------------------------------

void ShrinkWindow (void)
{
StopTime ();
if (gameStates.render.cockpit.nType == CM_FULL_COCKPIT && (gameStates.render.vr.nScreenFlags & VRF_ALLOW_COCKPIT)) {
	gameData.render.window.h = gameData.render.window.hMax;
	gameData.render.window.w = gameData.render.window.wMax;
	//!!ToggleCockpit ();
	gameStates.render.cockpit.nNextType = CM_FULL_COCKPIT;
	cockpit->Activate (CM_STATUS_BAR);
//		ShrinkWindow ();
//		ShrinkWindow ();
	HUDInitMessage (TXT_COCKPIT_F3);
	SavePlayerProfile ();
	StartTime (0);
	return;
	}

if (gameStates.render.cockpit.nType == CM_FULL_SCREEN && (gameStates.render.vr.nScreenFlags & VRF_ALLOW_COCKPIT)) {
	//gameData.render.window.w = gameData.render.window.wMax;
	//gameData.render.window[HA] = gameData.render.window.hMax;
	cockpit->Activate (CM_STATUS_BAR);
	SavePlayerProfile ();
	StartTime (0);
	return;
	}

if (gameStates.render.cockpit.nType != CM_STATUS_BAR && (gameStates.render.vr.nScreenFlags & VRF_ALLOW_COCKPIT)) {
	StartTime (0);
	return;
	}

#if TRACE
console.printf (CON_DBG, "Cockpit mode=%d\n", gameStates.render.cockpit.nType);
#endif
if (gameData.render.window.w > WINDOW_MIN_W) {
	//int x, y;
   gameData.render.window.w -= WINDOW_W_DELTA;
	gameData.render.window.h -= WINDOW_H_DELTA;
#if TRACE
  console.printf (CON_DBG, "NewW=%d NewH=%d VW=%d maxH=%d\n", gameData.render.window.w, gameData.render.window.h, gameData.render.window.wMax, gameData.render.window.hMax);
#endif
	if (gameData.render.window.w < WINDOW_MIN_W)
		gameData.render.window.w = WINDOW_MIN_W;
	if (gameData.render.window.h < WINDOW_MIN_H)
		gameData.render.window.h = WINDOW_MIN_H;
	gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w) / 2;
	gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h) / 2;
	GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
	HUDClearMessages ();
	SavePlayerProfile ();
	}
StartTime (0);
}

//------------------------------------------------------------------------------

void GameRenderFrame (void)
{
PROF_START
SetScreenMode (SCREEN_GAME);
cockpit->PlayHomingWarning ();
paletteManager.ClearEffect (paletteManager.Game ());
FillBackground ();
if (gameStates.render.vr.nRenderMode == VR_NONE)
	RenderMonoFrame ();
StopTime ();
paletteManager.EnableEffect ();
StartTime (0);
gameData.app.nFrameCount++;
PROF_END (ptRenderMine)
}

//------------------------------------------------------------------------------
//eof

