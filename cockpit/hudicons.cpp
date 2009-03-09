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
#include "error.h"
#include "u_mem.h"
#include "text.h"
#include "gamefont.h"
#include "input.h"
#include "ogl_bitmap.h"
#include "ogl_render.h"
#include "ogl_hudstuff.h"
#include "slowmotion.h"
#include "network.h"
#include "cockpit.h"
#include "headlight.h"
#include "hudicons.h"

//	-----------------------------------------------------------------------------

#define NUM_INV_ITEMS			10
#define INV_ITEM_HEADLIGHT		2
#define INV_ITEM_QUADLASERS	5
#define INV_ITEM_CLOAK			6
#define INV_ITEM_INVUL			7
#define INV_ITEM_SLOWMOTION	8
#define INV_ITEM_BULLETTIME	9

CBitmap*	bmpInventory = NULL;
CBitmap	bmInvItems [NUM_INV_ITEMS];
CBitmap	bmObjTally [2];

int bHaveInvBms = -1;
int bHaveObjTallyBms = -1;

CHUDIcons	hudIcons;

//	-----------------------------------------------------------------------------

const char *pszObjTallyIcons [] = {"rboticon.tga", "pwupicon.tga"};

int CHUDIcons::LoadTallyIcons (void)
{
	int	i;

if (bHaveObjTallyBms > -1)
	return bHaveObjTallyBms;
for (i = 0; i < 2; i++) {
	bmObjTally [i].Destroy ();
	if (!ReadTGA (pszObjTallyIcons [i], gameFolders.szDataDir, &bmObjTally [i], -1, 1.0, 0, 0)) {
		while (i) {
			i--;
			bmObjTally [i].Destroy ();
			}
		return bHaveObjTallyBms = 0;
		}
	}
return bHaveObjTallyBms = 1;
}

//	-----------------------------------------------------------------------------

void CHUDIcons::DestroyTallyIcons (void)
{
	int	i;

if (bHaveObjTallyBms > 0) {
	for (i = 0; i < 2; i++)
		bmObjTally [i].Destroy ();
	bHaveObjTallyBms = -1;
	}
}

//	-----------------------------------------------------------------------------

void CHUDIcons::DrawTally (void)
{
if (!gameOpts->render.cockpit.bObjectTally)
	return;
if (cockpit->Hide ())
	return;

	static int		objCounts [2] = {0, 0};
	static time_t	t0 = -1;
	static int		nIdTally [2] = {0, 0};
	time_t			t;

if (!IsMultiGame || IsCoopGame) {
	int	x, x0 = 0, y = 0, w, h, aw, i, bmW, bmH;
	char	szInfo [20];

	if (gameStates.render.cockpit.nType == CM_FULL_COCKPIT)
		y = 3 * nLineSpacing;
	else if (gameStates.render.cockpit.nType == CM_STATUS_BAR)
		y = 2 * nLineSpacing;
	else {//if (!cockpit->Always ()) {
		y = 2 * nLineSpacing;
		if (gameStates.render.fonts.bHires)
			y += nLineSpacing;
		}
	if (gameOpts->render.cockpit.bPlayerStats)
		y += 2 * nLineSpacing;

	x0 = CCanvas::Current ()->Width ();
	if ((extraGameInfo [0].nWeaponIcons >= 3) && (CCanvas::Current ()->Height () < 670))
		x0 -= HUD_LHX (20);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	t = gameStates.app.nSDLTicks;
	if (t - t0 > 333) {	//update 3 times per second
		t0 = t;
		for (i = 0; i < 2; i++) 
			objCounts [i] = ObjectCount (i ? OBJ_POWERUP : OBJ_ROBOT);
		}
	if (!gameOpts->render.cockpit.bTextGauges && (LoadTallyIcons () > 0)) {
		for (i = 0; i < 2; i++) {
			bmH = bmObjTally [i].Width () / 2;
			bmW = bmObjTally [i].Height () / 2;
			x = x0 - bmW - HUD_LHX (2);
			bmObjTally [i].RenderScaled (x, y, bmW, bmH, I2X (1), 0, NULL);
			sprintf (szInfo, "%d", objCounts [i]);
			fontManager.Current ()->StringSize (szInfo, w, h, aw);
			x -= w + HUD_LHY (2);
			nIdTally [i] = GrPrintF (nIdTally + i, x, y + (bmH - h) / 2, szInfo);
			y += bmH;
			}
		}
	else {
		y = 6 + 3 * nLineSpacing;
		for (i = 0; i < 2; i++) {
			sprintf (szInfo, "%s: %5d", i ? "Powerups" : "Robots", objCounts [i]);
			fontManager.Current ()->StringSize (szInfo, w, h, aw);
			nIdTally [i] = GrPrintF (nIdTally + i, CCanvas::Current ()->Width () - w - HUD_LHX (2), y, szInfo);
			y += nLineSpacing;
			}
		}
	}
}

//	-----------------------------------------------------------------------------

void CHUDIcons::ToggleWeaponIcons (void)
{
for (int i = 0; i < Controls [0].toggleIconsCount; i++)
	if (gameStates.app.bNostalgia)
		extraGameInfo [0].nWeaponIcons = 0;
	else {
		extraGameInfo [0].nWeaponIcons = (extraGameInfo [0].nWeaponIcons + 1) % 5;
		if (!gameOpts->render.weaponIcons.bEquipment && (extraGameInfo [0].nWeaponIcons == 3))
			extraGameInfo [0].nWeaponIcons = 4;
		}
}

//	-----------------------------------------------------------------------------

#define ICON_SCALE	3

void CHUDIcons::DrawWeapons (void)
{
	CBitmap*	bmP, * bmoP;
	int	nWeaponIcons = /*(gameStates.render.cockpit.nType == CM_STATUS_BAR) ? 3 :*/ extraGameInfo [0].nWeaponIcons;
	int	nIconScale = (gameOpts->render.weaponIcons.bSmall || (gameStates.render.cockpit.nType != CM_FULL_SCREEN)) ? 4 : 3;
	int	nIconPos = nWeaponIcons - 1;
	int	nHiliteColor = gameOpts->render.weaponIcons.nHiliteColor;
	int	nMaxAutoSelect;
	int	fw, fh, faw, 
			i, j, ll, n, 
			ox = 6, 
			oy = 6, 
			x, dx, y = 0, dy = 0;
	//float	fLineWidth = (CCanvas::Current ()->Width () >= 1200) ? 2.0f : 1.0f;
	float	fLineWidth = float (CCanvas::Current ()->Width ()) / 640.0f;
	ubyte	alpha = gameOpts->render.weaponIcons.alpha;
	uint	nAmmoColor;
	char	szAmmo [10];
	int	nLvlMap [2][10] = {{9, 4, 8, 3, 7, 2, 6, 1, 5, 0}, {4, 3, 2, 1, 0, 4, 3, 2, 1, 0}};

	static int	wIcon = 0, 
					hIcon = 0;
	static int	w = -1, 
					h = -1;
	static ubyte ammoType [2][10] = {{0, 1, 0, 0, 0, 0, 1, 0, 0, 0}, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
	static int bInitIcons = 1;
	static int nIdIcons [2][10] = {{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0}};

ll = LOCALPLAYER.laserLevel;
if (gameOpts->render.weaponIcons.bShowAmmo) {
	fontManager.SetCurrent (SMALL_FONT);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	}
dx = (int) (10 * xScale);
if (nWeaponIcons < 3) {
#if 0
	if (gameStates.render.cockpit.nType != CM_FULL_COCKPIT) {
#endif
		dy = (screen.Height () - CCanvas::Current ()->Height ());
		y = nIconPos ? screen.Height () - dy - oy : oy + hIcon + 12;
#if 0
		}
	else {
		y = (2 - gameStates.app.bD1Mission) * (oy + hIcon) + 12;
		nIconPos = 1;
		}
#endif
	}
if ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) && (extraGameInfo [0].nWeaponIcons == 2))
	y -= cockpit->LHX (10);
for (i = 0; i < 2; i++) {
	n = (gameStates.app.bD1Mission) ? 5 : 10;
	nMaxAutoSelect = 255;
	if (nWeaponIcons > 2) {
		int h = 0;
#if 0
		if (gameStates.render.cockpit.nType != CM_STATUS_BAR)
			h = 0;
		else 
			{
#if DBG
			h = gameStates.render.cockpit.nType + (gameStates.video.nDisplayMode ? gameData.models.nCockpits / 2 : 0);
			h = gameData.pig.tex.cockpitBmIndex [h].index;
			h = gameData.pig.tex.bitmaps [0][h].Height ();
#else
			h = gameData.pig.tex.bitmaps [0][gameData.pig.tex.cockpitBmIndex [gameStates.render.cockpit.nType + (gameStates.video.nDisplayMode ? gameData.models.nCockpits / 2 : 0)].index].Height ();
#endif
			}
#endif
		y = (CCanvas::Current ()->Height () - h - n * (hIcon + oy)) / 2 + hIcon;
		x = i ? screen.Width () - wIcon - ox : ox;
		}
	else {
		x = screen.Width () / 2;
		if (i)
			x += dx;
		else
			x -= dx + wIcon;
		}
	for (j = 0; j < n; j++) {
		int bActive, bHave, bAvailable, l, m;

		if (gameOpts->render.weaponIcons.nSort && !gameStates.app.bD1Mission) {
			l = nWeaponOrder [i][j];
			if (l == 255)
				nMaxAutoSelect = j;
			l = nWeaponOrder [i][j + (j >= nMaxAutoSelect)];
			}
		else
			l = nLvlMap [gameStates.app.bD1Mission][j];
		m = i ? secondaryWeaponToWeaponInfo [l] : primaryWeaponToWeaponInfo [l];
		if ((gameData.pig.tex.nHamFileVersion >= 3) && gameStates.video.nDisplayMode) {
			LoadBitmap (gameData.weapons.info [m].hiresPicture.index, 0);
			bmP = gameData.pig.tex.bitmaps [0] + gameData.weapons.info [m].hiresPicture.index;
			}
		else {
			bmP = gameData.pig.tex.bitmaps [0] + gameData.weapons.info [m].picture.index;
			LoadBitmap (gameData.weapons.info [m].picture.index, 0);
			}
		if ((bmoP = bmP->HasOverride ()))
			bmP = bmoP;
		Assert (bmP != NULL);
		if (w < bmP->Width ())
			w = bmP->Width ();
		if (h < bmP->Height ())
			h = bmP->Height ();
		wIcon = (int) ((w + nIconScale - 1) / nIconScale * xScale);
		hIcon = (int) ((h + nIconScale - 1) / nIconScale * yScale);
		if (bInitIcons)
			continue;
		if (i)
			bHave = (LOCALPLAYER.secondaryWeaponFlags & (1 << l));
		else if (!l) {
			bHave = (ll <= MAX_LASER_LEVEL);
			if (!bHave)
				continue;
			}
		else if (l == 5) {
			bHave = (ll > MAX_LASER_LEVEL);
			if (!bHave)
				continue;
			}
		else {
			bHave = (LOCALPLAYER.primaryWeaponFlags & (1 << l));
			if (bHave && extraGameInfo [0].bSmartWeaponSwitch && ((l == 1) || (l == 2)) &&
				 LOCALPLAYER.primaryWeaponFlags & (1 << (l + 5)))
				continue;
			}
		cockpit->BitBlt (-1, nIconScale * (x + (w - bmP->Width ()) / (2 * nIconScale)), nIconScale * (y - hIcon), false, true, I2X (nIconScale), 0, bmP);
		*szAmmo = '\0';
		nAmmoColor = GREEN_RGBA;
		if (ammoType [i][l]) {
			int nAmmo = (i ? LOCALPLAYER.secondaryAmmo [l] : LOCALPLAYER.primaryAmmo [(l == 6) ? 1 : l]);
			bAvailable = (nAmmo > 0);
			if (bHave) {
				if (bAvailable && gameOpts->render.weaponIcons.bShowAmmo) {
					if (!i && (l % 5 == 1)) {//Gauss/Vulcan
						nAmmo = X2I (nAmmo * (unsigned) VULCAN_AMMO_SCALE);
#if 0
						sprintf (szAmmo, "%d.%d", nAmmo / 1000, (nAmmo % 1000) / 100);
#else
						if (nAmmo && (nAmmo < 1000)) {
							sprintf (szAmmo, ".%d", nAmmo / 100);
							nAmmoColor = RED_RGBA;
							}
						else
							sprintf (szAmmo, "%d", nAmmo / 1000);
#endif
						}
					else
						sprintf (szAmmo, "%d", nAmmo);
					fontManager.Current ()->StringSize (szAmmo, fw, fh, faw);
					}
				}
			}
		else {
			bAvailable = (LOCALPLAYER.energy > gameData.weapons.info [l].xEnergyUsage);
			if (l == 0) {//Lasers
				sprintf (szAmmo, "%d", (ll > MAX_LASER_LEVEL) ? MAX_LASER_LEVEL + 1 : ll + 1);
				fontManager.Current ()->StringSize (szAmmo, fw, fh, faw);
				}
			else if ((l == 5) && (ll > MAX_LASER_LEVEL)) {
				sprintf (szAmmo, "%d", ll - MAX_LASER_LEVEL);
				fontManager.Current ()->StringSize (szAmmo, fw, fh, faw);
				}
			}
		if (i && !bAvailable)
			bHave = 0;
		if (bHave) {
			if (bAvailable)
				if (nHiliteColor)
					CCanvas::Current ()->SetColorRGB (0, 192, 255, ubyte (alpha * 16));
				else
					CCanvas::Current ()->SetColorRGB (255, 192, 0, ubyte (alpha * 16));
			else
				CCanvas::Current ()->SetColorRGB (128, 0, 0, ubyte (alpha * 16));
			}
		else {
			CCanvas::Current ()->SetColorRGB (64, 64, 64, (ubyte) (159 + alpha * 12));
			}
		OglDrawFilledRect (x - 1, y - hIcon - 1, x + wIcon + 2, y + 2);
		if (i) {
			if (j < 8)
				bActive = (l == gameData.weapons.nSecondary);
			else
				bActive = (j == 8) == (bLastSecondaryWasSuper [PROXMINE_INDEX] != 0);
			}
		else {
			if (l == 5)
				bActive = (bHave && (0 == gameData.weapons.nPrimary));
			else if (l)
				bActive = (l == gameData.weapons.nPrimary);
			else
				bActive = (bHave && (l == gameData.weapons.nPrimary));
			}
		if (bActive)
			if (bAvailable)
				if (nHiliteColor)
					CCanvas::Current ()->SetColorRGB (0, 192, 255, 255);
				else
					CCanvas::Current ()->SetColorRGB (255, 192, 0, 255);
			else
				CCanvas::Current ()->SetColorRGB (160, 0, 0, 255);
		else if (bHave)
			if (bAvailable)
				CCanvas::Current ()->SetColorRGB (0, 160, 0, 255);
			else
				CCanvas::Current ()->SetColorRGB (96, 0, 0, 255);
		else
			CCanvas::Current ()->SetColorRGB (64, 64, 64, 255);
		glLineWidth ((bActive && bAvailable && gameOpts->render.weaponIcons.bBoldHighlight) ? fLineWidth + 2 : fLineWidth);
		OglDrawEmptyRect (x - 1, y - hIcon - 1, x + wIcon + 2, y + 2);
//		if (bActive && bAvailable)
			if (*szAmmo) {
				fontManager.SetColorRGBi (nAmmoColor, 1, 0, 0);
				nIdIcons [i][j] = GrString (x + wIcon + 2 - fw, y - fh, szAmmo, nIdIcons [i] + j);
				fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
				}
		gameStates.render.grAlpha = 1.0f;
		if (nWeaponIcons > 2)
			y += hIcon + oy;
		else {
			if (i)
				x += wIcon + ox;
			else
				x -= wIcon + ox;
			}
		}
	}
bInitIcons = 0;
}

//	-----------------------------------------------------------------------------

int CHUDIcons::LoadInventoryIcons (void)
{
	int		h, i;
	ubyte*	buffer;

if (!((bmpInventory = PiggyLoadBitmap ("inventry.bmp")) ||
	   (bmpInventory = PiggyLoadBitmap ("inventory.bmp"))))
	return bHaveInvBms = 0;
memset (bmInvItems, 0, sizeof (bmInvItems));
h = bmpInventory->Width () * bmpInventory->Width ();
buffer = bmpInventory->Buffer ();
for (i = 0; i < NUM_INV_ITEMS; i++) {
	bmInvItems [i] = *bmpInventory;
	bmInvItems [i].SetName ("Inventory");
	bmInvItems [i].SetHeight (bmInvItems [i].Width ());
	bmInvItems [i].SetBuffer (buffer + h * i, 1, h);
	bmInvItems [i].ResetTexture ();
	bmInvItems [i].SetPalette (paletteManager.Game ());
	}
return bHaveInvBms = 1;
}

//	-----------------------------------------------------------------------------

void CHUDIcons::DestroyInventoryIcons (void)
{
if (bmpInventory) {
	delete bmpInventory;
	bmpInventory = NULL;
	bHaveInvBms = -1;
	}
}

//	-----------------------------------------------------------------------------

int CHUDIcons::EquipmentActive (int bFlag)
{
switch (bFlag) {
	case PLAYER_FLAGS_AFTERBURNER:
		return (gameData.physics.xAfterburnerCharge && Controls [0].afterburnerState);
	case PLAYER_FLAGS_CONVERTER:
		return gameStates.app.bUsingConverter;
	case PLAYER_FLAGS_HEADLIGHT:
		return HeadlightIsOn (-1) != 0;
	case PLAYER_FLAGS_FULLMAP:
		return 0;
	case PLAYER_FLAGS_AMMO_RACK:
		return 0;
	case PLAYER_FLAGS_QUAD_LASERS:
		return 0;
	case PLAYER_FLAGS_CLOAKED:
		return (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) != 0;
	case PLAYER_FLAGS_INVULNERABLE:
		return (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) != 0;
	case PLAYER_FLAGS_SLOWMOTION:
		return SlowMotionActive ();
	case PLAYER_FLAGS_BULLETTIME:
		return BulletTimeActive ();
	}
return 0;
}

//	-----------------------------------------------------------------------------

void CHUDIcons::DrawInventory (void)
{
	CBitmap*	bmP;
	char		szCount [20];
	int		nIconScale = (gameOpts->render.weaponIcons.bSmall || (gameStates.render.cockpit.nType != CM_FULL_SCREEN)) ? 3 : 2;
	int		nIconPos = extraGameInfo [0].nWeaponIcons & 1;
	int		nHiliteColor = gameOpts->render.weaponIcons.nHiliteColor;
	int		fw, fh, faw;
	int		j, n, firstItem, 
				oy = 6, 
				ox = 6, 
				x, y, dy;
	int		w = bmpInventory->Width (), 
				h = bmpInventory->Width ();
	int		wIcon = (int) ((w + nIconScale - 1) / nIconScale * xScale), 
				hIcon = (int) ((h + nIconScale - 1) / nIconScale * yScale);
	//float	fLineWidth = (CCanvas::Current ()->Width () >= 1200) ? 2.0f : 1.0f;
	float		fLineWidth = float (CCanvas::Current ()->Width ()) / 640.0f;
	ubyte		alpha = gameOpts->render.weaponIcons.alpha;

	static int nInvFlags [NUM_INV_ITEMS] = {
		PLAYER_FLAGS_AFTERBURNER, 
		PLAYER_FLAGS_CONVERTER, 
		PLAYER_FLAGS_HEADLIGHT, 
		PLAYER_FLAGS_FULLMAP, 
		PLAYER_FLAGS_AMMO_RACK, 
		PLAYER_FLAGS_QUAD_LASERS, 
		PLAYER_FLAGS_CLOAKED, 
		PLAYER_FLAGS_INVULNERABLE,
		PLAYER_FLAGS_SLOWMOTION,
		PLAYER_FLAGS_BULLETTIME
		};
	static int nEnergyType [NUM_INV_ITEMS] = {I2X (1), I2X (100), 0, I2X (1), 0, I2X (1), 0, 0, I2X (1), I2X (1)};
	static int nIdItems [NUM_INV_ITEMS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

dy = (screen.Height () - CCanvas::Current ()->Height ());
#if 1
	y = nIconPos ? screen.Height () - dy - oy : oy + hIcon + 12;
#else
if (gameStates.render.cockpit.nType != CM_STATUS_BAR) //(!cockpit->Always ())
	y = nIconPos ? screen.Height () - dy - oy : oy + hIcon + 12;
else
	y = oy + hIcon + 12;
#endif
n = (gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame)) ? NUM_INV_ITEMS : NUM_INV_ITEMS - 2;
firstItem = gameStates.app.bD1Mission ? INV_ITEM_QUADLASERS : 0;
x = (screen.Width () - (n - firstItem) * wIcon - (n - 1 - firstItem) * ox) / 2;
if ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) && (extraGameInfo [0].nWeaponIcons & 1))
	y -= cockpit->LHX (10);
for (j = firstItem; j < n; j++) {
	int bHave, bAvailable, bActive = EquipmentActive (nInvFlags [j]);
	bmP = bmInvItems + j;
	cockpit->BitBlt (-1, nIconScale * (x + (w - bmP->Width ()) / (2 * nIconScale)), nIconScale * (y - hIcon), false, true, I2X (nIconScale), 0, bmP);
	//m = 9 - j;
	*szCount = '\0';
	if (j == INV_ITEM_HEADLIGHT)
		bHave = PlayerHasHeadlight (-1);
	else if (j == INV_ITEM_INVUL) {
		if ((bHave = (LOCALPLAYER.nInvuls > 0)))
			sprintf (szCount, "%d", LOCALPLAYER.nInvuls);
		else
			bHave = LOCALPLAYER.flags & nInvFlags [j];
		}
	else if (j == INV_ITEM_CLOAK) {
		if ((bHave = (LOCALPLAYER.nCloaks > 0)))
			sprintf (szCount, "%d", LOCALPLAYER.nCloaks);
		else
			bHave = LOCALPLAYER.flags & nInvFlags [j];
		}
	else
		bHave = LOCALPLAYER.flags & nInvFlags [j];
	bAvailable = (LOCALPLAYER.energy > nEnergyType [j]);
#if 1
	if (bHave) {
		if (bAvailable)
			if (bActive)
				if (nHiliteColor)
					CCanvas::Current ()->SetColorRGB (0, 192, 255, ubyte (alpha * 16));
				else
					CCanvas::Current ()->SetColorRGB (255, 192, 0, ubyte (alpha * 16));
			else
				CCanvas::Current ()->SetColorRGB (128, 128, 0, ubyte (alpha * 16));
		else
			CCanvas::Current ()->SetColorRGB (128, 0, 0, ubyte (alpha * 16));
		}
	else {
		CCanvas::Current ()->SetColorRGB (64, 64, 64, (ubyte) (159 + alpha * 12));
		}
	OglDrawFilledRect (x - 1, y - hIcon - 1, x + wIcon + 2, y + 2);
	if (bHave)
		if (bAvailable)
			if (bActive)
				if (nHiliteColor)
					CCanvas::Current ()->SetColorRGB (0, 192, 255, 255);
				else
					CCanvas::Current ()->SetColorRGB (255, 192, 0, 255);
			else
				CCanvas::Current ()->SetColorRGB (0, 160, 0, 255);
		else
			CCanvas::Current ()->SetColorRGB (96, 0, 0, 255);
	else
		CCanvas::Current ()->SetColorRGB (64, 64, 64, 255);
	glLineWidth ((bActive && gameOpts->render.weaponIcons.bBoldHighlight) ? 3 : fLineWidth);
	OglDrawEmptyRect (x - 1, y - hIcon - 1, x + wIcon + 2, y + 2);
	if (*szCount) {
		fontManager.Current ()->StringSize (szCount, fw, fh, faw);
		fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
		nIdItems [j] = GrString (x + wIcon + 2 - fw, y - fh, szCount, nIdItems + j);
		fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
		}
#endif
	gameStates.render.grAlpha = 1.0f;
	x += wIcon + ox;
	}
}

//	-----------------------------------------------------------------------------

void CHUDIcons::Render (void)
{
if (gameStates.app.bNostalgia)
	return;
if (gameStates.app.bEndLevelSequence)
	return;
if (gameStates.render.bRearView)
	return;
if (gameData.render.window.x || gameData.render.window.y)
	return;	// render window has been shrunk
ToggleWeaponIcons ();
if (gameOpts->render.cockpit.bHUD || cockpit->ShowAlways ()) {
	nLineSpacing = cockpit->LineSpacing ();
	if (!(gameStates.render.bRearView || gameStates.render.bChaseCam || gameStates.render.bFreeCam))
		DrawTally ();
	if (!gameStates.app.bDemoData && EGI_FLAG (nWeaponIcons, 1, 1, 0)) {
		xScale = cockpit->XScale () * HUD_ASPECT;
		yScale = cockpit->YScale ();
		cockpit->SetScales (xScale, yScale);
		DrawWeapons ();
		if (gameOpts->render.weaponIcons.bEquipment) {
			if (bHaveInvBms < 0)
				LoadInventoryIcons ();
			if (bHaveInvBms > 0)
				DrawInventory ();
			}
		xScale /= HUD_ASPECT;
		cockpit->SetScales (xScale, yScale);
		glLineWidth (1);
		}
	}
}

//	-----------------------------------------------------------------------------

void CHUDIcons::Destroy (void)
{
DestroyTallyIcons ();
DestroyInventoryIcons ();
}

//	-----------------------------------------------------------------------------
