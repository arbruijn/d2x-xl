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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "descent.h"
#include "screens.h"
#include "error.h"
#include "newdemo.h"
#include "gamefont.h"
#include "text.h"
#include "network.h"
#include "network_lib.h"
#include "rendermine.h"
#include "transprender.h"
#include "gamepal.h"
#include "gr.h"
#include "ogl_lib.h"
#include "ogl_render.h"
#include "ogl_bitmap.h"
#include "ogl_hudstuff.h"
#include "playerprofile.h"
#include "hud_defs.h"
#include "cockpit.h"
#include "statusbar.h"
#include "slowmotion.h"
#include "automap.h"
#include "hudmsgs.h"
#include "hudicons.h"
#include "radar.h"
#include "visibility.h"
#include "autodl.h"
#include "key.h"
#include "addon_bitmaps.h"
#include "marker.h"

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

bool GuidedMissileActive (void)
{
CObject *gmObjP = gameData.objs.guidedMissile [N_LOCALPLAYER].objP;
return gmObjP &&
		 (gmObjP->info.nType == OBJ_WEAPON) &&
		 (gmObjP->info.nId == GUIDEDMSL_ID) &&
		 (gmObjP->info.nSignature == gameData.objs.guidedMissile [N_LOCALPLAYER].nSignature);
}

//	-----------------------------------------------------------------------------

inline float RadToDeg (float r)
{
return r * float (180.0 / PI);
}

//------------------------------------------------------------------------------

//draw a crosshair for the guided missile
void DrawGuidedCrosshairs (fix xStereoSeparation)
{
if (transformation.HaveHeadAngles ())
	return;

int nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
CCanvas::Current ()->SetColorRGBi (RGB_PAL (0, 31, 0));
int w = CCanvas::Current ()->Width () >> 5;
if (w < 5)
	w = 5;
int h = I2X (w) / gameData.render.screen.Aspect ();
int x = CCanvas::Current ()->Width () / 2;
if (xStereoSeparation) {
	ogl.ColorMask (1,1,1,1,1);
	x -= int (float (x / 32) * X2F (xStereoSeparation));
	}
int y = CCanvas::Current ()->Height () / 2;
glLineWidth (float (gameData.render.frame.Width ()) / 640.0f);
x = gameData.X (x);
#if 1
	w /= 2 << ogl.IsOculusRift ();
	h /= 2 << ogl.IsOculusRift ();
	OglDrawLine (x - w, y, x + w, y);
	OglDrawLine (x, y - h, x, y + h);
#else
	OglDrawLine (I2X (x-w/2), I2X (y), I2X (x+w/2), I2X (y));
	OglDrawLine (I2X (x), I2X (y-h/2), I2X (x), I2X (y+h/2));
#endif
glLineWidth (1.0f);
if (xStereoSeparation) {
	ogl.ColorMask (1,1,1,1,0);
	}
gameData.SetStereoOffsetType (nOffsetSave);
}

//------------------------------------------------------------------------------

void DrawScope (void)
{
if (scope.Load ()) {
	float sh = float (gameData.render.screen.Height ());
	float ch = float (CCanvas::Current ()->Height ());
	float w = 0.25f * float (CCanvas::Current ()->Width ()) / ch;
	float y = 1.0f - float (CCanvas::Current ()->Top ()) / sh;
	float h = ch / sh;

	ogl.SetTexturing (true);
	ogl.SetBlending (true);
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
	ogl.SetDepthTest (false);
	if (scope.Bind (1))
		return;
	scope.Texture ()->Wrap (GL_REPEAT);
	glColor3f (1.0f, 1.0f, 1.0f);
	glBegin (GL_QUADS);
	glTexCoord2f (0.5f - w, 0.25f);
	glVertex2f (0, y);
	glTexCoord2f (0.5f + w, 0.25f);
	glVertex2f (1, y);
	glTexCoord2f (0.5f + w, 0.75f);
	glVertex2f (1, y - h);
	glTexCoord2f (0.5f - w, 0.75f);
	glVertex2f (0, y - h);
	glEnd ();
	ogl.BindTexture (0);
	ogl.SetDepthTest (true);
	ogl.SetTexturing (false);
	glPopMatrix ();
	}
}

//------------------------------------------------------------------------------

//draw a crosshair for the zoom
void DrawZoomCrosshairs (void)
{
if (transformation.HaveHeadAngles ())
	return;

int nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
DrawScope ();

	static tSinCosf sinCos [124];
	static int bInitSinCos = 1;

if (bInitSinCos) {
	ComputeSinCosTable (sizeofa (sinCos), sinCos);
	bInitSinCos = 0;
	}

	int bHaveTarget = TargetInLineOfFire ();
	int sh = gameData.render.screen.Height ();
	int ch = CCanvas::Current ()->Height ();
	int cw = CCanvas::Current ()->Width ();
	int h = ch >> 2;
	int w = X2I (h * gameData.render.screen.Aspect ());

if (ogl.IsSideBySideDevice ()) {
	w /= 3;
	h /= 3;
	}

	int x = gameData.X (cw / 2);
	int y = ch / 2;
	int left = x - w, right = x + w, top = y - h, bottom = y + h;
	float xStep = float (2 * w + 1) / 12.0f;
	//float yStep = float (2 * h + 1) / 12.0f;
	float xScale = float (w + (w >> 1)) / float (cw);
	float yScale = float (h + (h >> 1)) / float (sh);
	float	x1, y1;
	int	i;

w >>= 4;
h >>= 4;

glLineWidth (float (cw) / 640.0f);

if (bHaveTarget)
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (39, 0, 0, 128));
else
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 39, 0, 128));
for (i = 0, x1 = float (left); i < 11; i++) {
	x1 += xStep;
	if (i != 5)
		OglDrawLine ((int) FRound (x1), y - h, (int) FRound (x1), y + h);
	}

for (i = 0, y1 = float (top); i < 11; i++) {
	y1 += xStep;
	if (i != 5)
		OglDrawLine (x - w, (int) FRound (y1), x + w, (int) FRound (y1));
	}

if (bHaveTarget)
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (63, 0, 0, 160));
else
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 63, 0, 160));

glLineWidth (float (floor (2 * float (cw) / 640.0f)));

glPushMatrix ();
ogl.SetLineSmooth (true);
glTranslatef (0.5f - X2F (ogl.StereoSeparation ()), 1.0f - float (CCanvas::Current ()->Top () + y) / float (gameData.render.screen.Height ()), 0.0f);
glScalef (xScale, yScale, 1.0f);
#if 0
float fh = 2.0f * float (h) / float (sh);
float fw = 2.0f * float (w) / float (cw);
#endif
glBegin (GL_LINES);
glVertex2f (0.0f, -1.0f);
glVertex2f (0.0f, -1.125f);
glVertex2f (sinCos [15].fCos, sinCos [15].fSin);
glVertex2f (sinCos [15].fCos * 1.0625f, sinCos [15].fSin * 1.0625f);
glVertex2f (0.0f, 1.0f);
glVertex2f (0.0f, 1.125f);
glVertex2f (sinCos [46].fCos, sinCos [46].fSin);
glVertex2f (sinCos [46].fCos * 1.0625f, sinCos [46].fSin * 1.0625f);
glVertex2f (-1.125f, 0.0f);
glVertex2f (-1.0f, 0.0f);
glVertex2f (sinCos [77].fCos, sinCos [77].fSin);
glVertex2f (sinCos [77].fCos * 1.0625f, sinCos [77].fSin * 1.0625f);
glVertex2f (1.125f, 0.0f);
glVertex2f (1.0f, 0.0f);
glVertex2f (sinCos [108].fCos, sinCos [108].fSin);
glVertex2f (sinCos [108].fCos * 1.0625f, sinCos [108].fSin * 1.0625f);
glEnd ();
glPopMatrix ();

w += w >> 1;
h += h >> 1;
w <<= 1;
h <<= 1;

glLineWidth (float (cw) / 640.0f);

OglDrawLine (left, y, right, y);
OglDrawLine (x, top, x, bottom);
OglDrawLine (left, y - h, left, y + h);
OglDrawLine (right, y - h, right, y + h);
OglDrawLine (x - w, top, x + w, top);
OglDrawLine (x - w, bottom, x + w, bottom);
glLineWidth (1.0f);

w >>= 1;
h >>= 1;
w -= w >> 1;
h -= h >> 1;
glLineWidth (float (floor (2 * float (cw) / 640.0f)));
#if 1
//float xScale = float (w << 5) / float (cw);
//float yScale = float (h << 5) / float (gameData.render.screen.Height ());
OglDrawEllipse (sizeofa (sinCos), GL_LINE_LOOP, xScale, 0.5f - X2F (ogl.StereoSeparation ()), yScale, 
					 1.0f - float (CCanvas::Current ()->Top () + y) / float (gameData.render.screen.Height ()), sinCos);
#else
glPushMatrix ();
ogl.SetLineSmooth (true);
if (bHaveTarget)
	glColor4f (1.0f, 0.0f, 0.0f, 0.25f);
else
	glColor4f (0.0f, 1.0f, 0.0f, 0.25f);
glTranslatef (0.5f, 0.5f, 0.5f);
glScalef (float (w << 5) / float (cw), float (h << 5) / float (ch), 0.1f);
OglDrawEllipse (sizeofa (sinCos), GL_LINE_LOOP, 1.0f, 0, 1.0f, 0, sinCos);
ogl.SetLineSmooth (false);
glPopMatrix ();
#endif

char	szZoom [20];
int	r, aw;
if (extraGameInfo [IsMultiGame].nZoomMode == 2)
	r = int (100.0f * gameStates.zoom.nFactor / float (gameStates.zoom.nMinFactor));
else {
	float s = float (pow (float (gameStates.zoom.nMaxFactor) / float (gameStates.zoom.nMinFactor), 0.25f));
	fix f = gameStates.zoom.nMinFactor;
	for (r = 1; f < fix (gameStates.zoom.nFactor); r++)
		f = (fix) FRound (float (f) * s);
	r *= 100;
	}
sprintf (szZoom, "X %d.%02d", r / 100, r % 100);
fontManager.Current ()->StringSize (szZoom, w, h, aw);
if (bHaveTarget)
	fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
else
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
GrPrintF (NULL, (cw - w) / 2, bottom + h, szZoom);
gameData.SetStereoOffsetType (nOffsetSave);
}

//	-----------------------------------------------------------------------------

CRGBColor playerColors [] = {
 {15, 15, 23},
 {27, 0, 0},
 {0, 23, 0},
 {30, 11, 31},
 {31, 16, 0},
 {24, 17, 6},
 {14, 21, 12},
 {29, 29, 0}};

typedef struct {
	int x, y;
} xy;

//offsets for reticle parts: high-big  high-sml  low-big  low-sml
xy crossOffsets [4] = 	{{-8, -5},  {-4, -2},  {-4, -2}, {-2, -1}};
xy primaryOffsets [4] =  {{-30, 14}, {-16, 6},  {-15, 6}, {-8, 2}};
xy secondaryOffsets [4] = {{-24, 2},  {-12, 0}, {-12, 1}, {-6, -2}};

//draw the reticle
void CGenericCockpit::DrawReticle (int bForceBig, fix xStereoSeparation)
{
if (cockpit->Hide ())
	return;
if (!gameOpts->render.cockpit.bReticle
	 || !gameStates.app.bGameRunning 
	 || gameStates.menus.nInMenu 
	 || gameStates.render.bRearView 
	 || gameStates.app.bPlayerIsDead 
	 || gameStates.render.bChaseCam 
	 || (gameStates.render.bFreeCam > 0)
	 || automap.Display ())
	return;

	int		x, y;
	int		bLaserReady, bMissileReady, bLaserAmmo, bMissileAmmo;
	int		nBmReticle, nCrossBm, nPrimaryBm, nSecondaryBm;
	int		bHiresReticle, bSmallReticle, ofs;

if (((gameOpts->render.cockpit.bGuidedInMainView && GuidedMissileActive ()) ||
	 ((gameData.demo.nState == ND_STATE_PLAYBACK) && gameData.demo.bFlyingGuided))) {
	DrawGuidedCrosshairs (m_info.xStereoSeparation);
	return;
	}

if (gameStates.zoom.nFactor > float (gameStates.zoom.nMinFactor)) {
	DrawZoomCrosshairs ();
	return;
	}

gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
m_info.xStereoSeparation = xStereoSeparation;
x = CCanvas::Current ()->Width () / 2;
y = CCanvas::Current ()->Height () / 2;
bLaserReady = AllowedToFireGun ();
bMissileReady = AllowedToFireMissile (-1, 1);
bLaserAmmo = PlayerHasWeapon (gameData.weapons.nPrimary, 0, -1, 1);
bMissileAmmo = PlayerHasWeapon (gameData.weapons.nSecondary, 1, -1, 1);
nPrimaryBm = bLaserReady && (bLaserAmmo == HAS_ALL);
nSecondaryBm = bMissileReady && (bMissileAmmo == HAS_ALL);
if (nPrimaryBm && (gameData.weapons.nPrimary == LASER_INDEX) && (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS))
	nPrimaryBm++;

if (secondaryWeaponToGunNum [gameData.weapons.nSecondary] == 7)
	nSecondaryBm += 3;		//now value is 0, 1 or 3, 4
else if (nSecondaryBm && !(gameData.laser.nMissileGun & 1))
		nSecondaryBm++;

nCrossBm = ((nPrimaryBm > 0) || (nSecondaryBm > 0));

Assert (nPrimaryBm <= 2);
Assert (nSecondaryBm <= 4);
Assert (nCrossBm <= 1);
#if DBG
if (gameStates.render.bChaseCam)
#else
if (gameStates.render.bChaseCam && (!IsMultiGame || (EGI_FLAG (bEnableCheats, 0, 0, 0) && !COMPETITION)))
#endif
	return;
m_info.xScale *= float (HUD_ASPECT);
if (ogl.IsOculusRift ()) {
	m_info.xScale *= 0.5f;
	m_info.yScale *= 0.5f;
	}
bHiresReticle = 1; //(gameStates.render.fonts.bHires != 0) && !gameStates.app.bDemoData;
bSmallReticle = !bForceBig && (CCanvas::Current ()->Width () * 3 <= gameData.render.frame.Width () * 2);
ofs = (bHiresReticle ? 0 : 2) + bSmallReticle;
nBmReticle = ((!IsMultiGame || IsCoopGame) && TargetInLineOfFire ()) 
				 ? BM_ADDON_RETICLE_RED 
				 : BM_ADDON_RETICLE_GREEN;
ogl.SetBlendMode (OGL_BLEND_ALPHA);
glColor3i (1,1,1);
int nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);

if (ogl.IsOculusRift ()) {
	float fov = float (gameStates.render.glFOV * X2D (transformation.m_info.zoom));
	float yStep = float (CCanvas::Current ()->Height ()) / fov * 4;
	float xStep = yStep * float (CCanvas::Current ()->AspectRatio ());
	//if ((fabs (X2F (transformation.m_info.playerHeadAngles.v.coord.h)) > 0.1f) || 
	//	 (fabs (X2F (transformation.m_info.playerHeadAngles.v.coord.p)) > 0.1f)) 
		{
		float fade = 1.0f - 2.0f * X2F (max (abs (transformation.m_info.playerHeadAngles.v.coord.h), 
														 abs (transformation.m_info.playerHeadAngles.v.coord.p)));
		fade *= fade;
		if (gameOpts->input.oculusRift.nDeadzone) { // display a reference image of the reticle
			gameStates.render.grAlpha = pow (fade, 4);
			BitBlt ((bSmallReticle ? SML_RETICLE_CROSS : RETICLE_CROSS) + nCrossBm,
					  x + ScaleX (crossOffsets [ofs].x - 1), (y + ScaleY (crossOffsets [ofs].y - 1)), false, true,
					  I2X (1), 0, NULL, BM_ADDON (nBmReticle + nCrossBm));
			BitBlt ((bSmallReticle ? SML_RETICLE_PRIMARY : RETICLE_PRIMARY) + nPrimaryBm,
					  x + ScaleX (primaryOffsets [ofs].x - 1), (y + ScaleY (primaryOffsets [ofs].y - 1)), false, true,
					  I2X (1), 0, NULL, BM_ADDON (nBmReticle + 2 + nPrimaryBm));
			BitBlt ((bSmallReticle ? SML_RETICLE_SECONDARY : RETICLE_SECONDARY) + nSecondaryBm,
					  x + ScaleX (secondaryOffsets [ofs].x - 1), (y + ScaleY (secondaryOffsets [ofs].y - 1)), false, true,
					  I2X (1), 0, NULL, BM_ADDON (nBmReticle + 5 + nSecondaryBm));
			gameStates.render.grAlpha = 1.0f - gameStates.render.grAlpha;
			}
		}
	x -= int (ceil (xStep * RadToDeg (X2F (transformation.m_info.playerHeadAngles.v.coord.h))));
	y -= int (ceil (yStep * RadToDeg (X2F (transformation.m_info.playerHeadAngles.v.coord.p))));
	}

BitBlt ((bSmallReticle ? SML_RETICLE_CROSS : RETICLE_CROSS) + nCrossBm,
		  x + ScaleX (crossOffsets [ofs].x - 1), (y + ScaleY (crossOffsets [ofs].y - 1)), false, true,
		  I2X (1), 0, NULL, BM_ADDON (nBmReticle + nCrossBm));
BitBlt ((bSmallReticle ? SML_RETICLE_PRIMARY : RETICLE_PRIMARY) + nPrimaryBm,
		  x + ScaleX (primaryOffsets [ofs].x - 1), (y + ScaleY (primaryOffsets [ofs].y - 1)), false, true,
		  I2X (1), 0, NULL, BM_ADDON (nBmReticle + 2 + nPrimaryBm));
BitBlt ((bSmallReticle ? SML_RETICLE_SECONDARY : RETICLE_SECONDARY) + nSecondaryBm,
		  x + ScaleX (secondaryOffsets [ofs].x - 1), (y + ScaleY (secondaryOffsets [ofs].y - 1)), false, true,
		  I2X (1), 0, NULL, BM_ADDON (nBmReticle + 5 + nSecondaryBm));
gameData.SetStereoOffsetType (nOffsetSave);

if (!gameStates.app.bNostalgia && gameOpts->input.mouse.bJoystick && gameOpts->render.cockpit.bMouseIndicator)
	OglDrawMouseIndicator ();
m_info.xScale /= float (HUD_ASPECT);
if (ogl.IsOculusRift ()) {
	m_info.xScale *= 2.0f;
	m_info.yScale *= 2.0f;
	}
gameStates.render.grAlpha = 1.0f;
#if 0
if (m_info.xStereoSeparation) {
	ogl.ColorMask (1,1,1,1,0);
	}
#endif
}

//	-----------------------------------------------------------------------------
