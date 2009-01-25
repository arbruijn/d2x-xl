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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "descent.h"
#include "pstypes.h"
#include "strutil.h"
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "cfile.h"
#include "error.h"
#include "mono.h"
#include "fix.h"
#include "newdemo.h"
#include "text.h"
#include "texmerge.h"
#include "rle.h"
#include "bitmap.h"
#include "palette.h"

//#define Sqr(x) ((x)*(x))

CPaletteManager paletteManager;

char szCurrentLevelPalette [SHORT_FILENAME_LEN];

//	-----------------------------------------------------------------------------

inline int Sqr (int i)
{
return i * i;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CPalette::Init (void)
{
m_nComputedColors = 0;
memset (m_computedColors, 0xff, sizeof (m_computedColors));
}

//------------------------------------------------------------------------------

bool CPalette::Read (CFile& cf)
{
return cf.File () && (cf.Read (&m_data, sizeof (m_data), 1) == 1);
}

//------------------------------------------------------------------------------

bool CPalette::Write (CFile& cf)
{
return cf.File () && (cf.Write (&m_data, sizeof (m_data), 1) == 1);
}

//	-----------------------------------------------------------------------------

void CPalette::ToRgbaf (ubyte nIndex, tRgbaColorf& color)
{
color.red = float (m_data.rgb [nIndex].red) / 63.0f;
color.green = float (m_data.rgb [nIndex].green) / 63.0f;
color.blue = float (m_data.rgb [nIndex].blue) / 63.0f;
color.alpha = -1;
}

//------------------------------------------------------------------------------

//	Add a computed color (by GrFindClosestColor) to list of computed colors in m_computedColors.
//	If list wasn't full already, increment gameData.render.m_nComputedColors.
//	If was full, replace a random one.
void CPalette::AddComputedColor (int r, int g, int b, int nIndex)
{
	int	i;

if (m_nComputedColors < MAX_COMPUTED_COLORS) {
	i = m_nComputedColors;
	m_nComputedColors++;
	} 
else
	i = (d_rand() * MAX_COMPUTED_COLORS) >> 15;
m_computedColors [i].color.red = r;
m_computedColors [i].color.green = g;
m_computedColors [i].color.blue = b;
m_computedColors [i].nIndex = nIndex;
}

//	-----------------------------------------------------------------------------

void CPalette::InitComputedColors (void)
{
memset (m_computedColors, 0xff, sizeof (m_computedColors));
}

//	-----------------------------------------------------------------------------

static inline int ColorDelta (tRgbColorb& palette, int r, int g, int b)
{
return Sqr (r - palette.red) + Sqr (g - palette.green) + Sqr (b - palette.blue);
}

//	-----------------------------------------------------------------------------

int CPalette::ClosestColor (int r, int g, int b)
{
	int				i, n;
	int				nBestValue, nBestIndex, value;
	tComputedColor	*pci, *pcj;

#if 0
if (!palette)
	palette = paletteManager.Default ();
if (!palette)
	return -1;
#endif
n = m_nComputedColors;
//if (!n)
//	InitComputedColors (plP);

//	If we've already computed this color, return it!
pci = m_computedColors;
for (i = 0; i < n; i++, pci++)
	if ((r == pci->color.red) && (g == pci->color.green) && (b == pci->color.blue)) {
		if (i <= 4)
			return pci->nIndex;
		else {
			tComputedColor	h;
			pcj = pci - 1;
			h = *pcj;
			*pcj = *pci;
			*pci = h;
			return pcj->nIndex;
			}
		}

nBestValue = 3 * Sqr (255) + 1;	// max. possible value + 1
nBestIndex = 0;
// only go to 254, 'cause we dont want to check the transparent color.
for (i = 0; i < 255; i++) {
	if (!(value = ColorDelta (m_data.rgb [i], r, g, b))) {
		AddComputedColor (r, g, b, i);
		return i;
		}
	if (value < nBestValue) {
		nBestValue = value;
		nBestIndex = i;
		}
	}
AddComputedColor (r, g, b, nBestIndex);
return nBestIndex;
}

//	-----------------------------------------------------------------------------

void CPalette::SwapTransparency (void)
{
for (int i = 0; i < 3; i++) {
	ubyte h = m_data.raw [i];
	m_data.raw [i] = m_data.raw [765 + i];
	m_data.raw [765 + i] = h;
	}
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

int CPaletteManager::FindClosestColor15bpp (int rgb)
{
return m_data.game ? m_data.game->ClosestColor (((rgb >> 10) & 31) * 2, ((rgb >> 5) & 31) * 2, (rgb & 31) * 2) : 1;
}

//	-----------------------------------------------------------------------------

CPalette* CPaletteManager::Find (CPalette& palette)
{
	tPaletteList	*plP;
#if DBG
	int				i;
#endif

for (plP = m_data.list; plP; plP = plP->next)
#if DBG
 {
	for (i = 0; i < PALETTE_SIZE * 3; i++)
		if (palette.Data ().raw [i] != plP->palette.Data ().raw [i])
			break;
	if (i == 768)
		return Activate (&plP->palette);
	}
#else
	if (palette == plP->palette)
		return Activate (&plP->palette);
#endif
return NULL;
}

//	-----------------------------------------------------------------------------

CPalette *CPaletteManager::Add (CPalette& palette)
{
	tPaletteList	*plP;

if (Find (palette))
	return m_data.current;
if (!(plP = new tPaletteList))
	return NULL;
plP->next = m_data.list;
m_data.list = plP;
plP->palette = palette;
plP->palette.Init ();
m_data.nPalettes++;
return Activate (&plP->palette);
}

//	-----------------------------------------------------------------------------

CPalette *CPaletteManager::Add (ubyte* buffer)
{
	CPalette	palette;

memcpy (palette.Raw (), buffer, PALETTE_SIZE * 3);
return Add (palette);
}

//	-----------------------------------------------------------------------------

void CPaletteManager::Destroy (void)
{
	tPaletteList	*pi, *pj;

for (pi = m_data.list; pi; pi = pj) {
	pj = pi->next;
	delete pi;
	}
m_data.list = NULL;
m_data.nPalettes = 0;
}

//------------------------------------------------------------------------------

void CPaletteManager::SetEffect (bool bForce)
{
if (gameStates.render.vr.bUseRegCode)
	;//GrPaletteStepUpVR (r, g, b, VR_WHITE_INDEX, VR_BLACK_INDEX);
else
	SetEffect (m_data.effect.red, m_data.effect.green, m_data.effect.blue, bForce);
}

//	------------------------------------------------------------------------------------

#define MAX_PALETTE_ADD 30

void CPaletteManager::BumpEffect (int red, int green, int blue)
{
BumpEffect (float (red) / 64.0f, float (green) / 64.0f, float (blue) / 64.0f);
}

//	------------------------------------------------------------------------------------

void CPaletteManager::BumpEffect (float red, float green, float blue)
{
	float	maxVal = (paletteManager.EffectDuration () ? 60 : MAX_PALETTE_ADD) / 64.0f;

CLAMP (red, -maxVal, maxVal);
CLAMP (green, -maxVal, maxVal);
CLAMP (blue, -maxVal, maxVal);
m_data.effect.red = red;
m_data.effect.green = green;
m_data.effect.blue = blue;
}

//------------------------------------------------------------------------------

void CPaletteManager::SaveEffect (void)
{
m_data.lastEffect = m_data.effect;
}

//------------------------------------------------------------------------------

void CPaletteManager::LoadEffect (CPalette *palette)
{
SetEffect (m_data.lastEffect.red, m_data.lastEffect.green, m_data.lastEffect.blue);
//	Forces flash effect to fixup palette next frame.
if (palette)
	Activate (palette);
m_data.xLastEffectTime = 0;
}

//------------------------------------------------------------------------------

void CPaletteManager::SaveEffectAndReset (void)
{
SaveEffect ();
ResetEffect ();
LoadEffect ();
}

//------------------------------------------------------------------------------

void CPaletteManager::ResetEffect (void)
{
m_data.effect.red =
m_data.effect.green =
m_data.effect.blue= 0;
m_data.xEffectDuration = 0;
m_data.xLastEffectTime = 0;
SetEffect (0, 0, 0);
}

//	------------------------------------------------------------------------------------

static inline float UpdateEffect (float nColor, float nChange)
{
if (nColor > 0) {
	nColor -= nChange;
	if (nColor < 0)
		nColor = 0;
	}
else if (nColor < 0) {
	nColor += nChange;
	if (nColor > 0)
		nColor = 0;
	}
return nColor;
}

//	------------------------------------------------------------------------------------
//	Diminish palette effects towards Normal.

void CPaletteManager::FadeEffect (void)
{
	float	nDecAmount = 0;
	bool	bForce = false;

	//	Diminish at FADE_RATE units/second.
	//	For frame rates > FADE_RATE Hz, use randomness to achieve this.
if (gameData.time.xFrame < I2X (1) / FADE_RATE) {
	if (d_rand () < gameData.time.xFrame * FADE_RATE / 2)	
		nDecAmount = 1;
	}
else {
	if (!(nDecAmount = X2F (gameData.time.xFrame * FADE_RATE)))		// one second = FADE_RATE counts
		nDecAmount = 1;						// make sure we decrement by something
	}
nDecAmount /= 64.0f;

if (m_data.xEffectDuration) {
	//	Part of hack system to force update of palette after exiting a menu.
	if (m_data.xLastEffectTime)
		bForce = true;

	if ((m_data.xLastEffectTime + I2X (1)/8 < gameData.time.xGame) || (m_data.xLastEffectTime > gameData.time.xGame)) {
		audio.PlaySound (SOUND_CLOAK_OFF, SOUNDCLASS_GENERIC, m_data.xEffectDuration / 4);
		m_data.xLastEffectTime = gameData.time.xGame;
		}

	m_data.xEffectDuration -= gameData.time.xFrame;
	if (m_data.xEffectDuration < 0)
		m_data.xEffectDuration = 0;

	if (bForce || (d_rand () > 4096)) {
      if ((gameData.demo.nState == ND_STATE_RECORDING) && (m_data.effect.red || m_data.effect.green || m_data.effect.blue))
	      NDRecordPaletteEffect (short (m_data.effect.red * 64), short (m_data.effect.green * 64), short (m_data.effect.blue * 64));
		paletteManager.SetEffect ();
		return;
		}
	}

m_data.effect.red = UpdateEffect (m_data.effect.red, nDecAmount);
m_data.effect.green = UpdateEffect (m_data.effect.green, nDecAmount);
m_data.effect.blue = UpdateEffect (m_data.effect.blue, nDecAmount);

if ((gameData.demo.nState == ND_STATE_RECORDING) && (m_data.effect.red || m_data.effect.green || m_data.effect.blue))
     NDRecordPaletteEffect (short (m_data.effect.red * 64), short (m_data.effect.green * 64), short (m_data.effect.blue * 64));
SetEffect (bForce);
}

//------------------------------------------------------------------------------

void CPaletteManager::SetGamma (int gamma)
{
CLAMP (gamma, 0, 16);
if (m_data.nGamma != gamma) {
	m_data.nGamma = gamma;
#if 0
	if (!paletteManager.EffectDisabled ())
		paletteManager.LoadEffect ();
#endif
	}
}

//------------------------------------------------------------------------------

int CPaletteManager::GetGamma (void)
{
return m_data.nGamma;
}

//	-----------------------------------------------------------------------------

void CPaletteManager::Init (void)
{
memset (&m_data, 0, sizeof (m_data));
SetGamma (-1);
m_save.Create (10);
}

//------------------------------------------------------------------------------

CPalette *CPaletteManager::Load (const char *pszFile, const char *pszLevel)
{
	CFile		cf;
	int		i = 0, fsize;
	CPalette	palette;
#ifdef SWAP_0_255
	ubyte		c;
#endif

if (pszLevel) {
	char ifile_name [FILENAME_LEN];

	CFile::ChangeFilenameExtension (ifile_name, pszLevel, ".pal");
	i = cf.Open (ifile_name, gameFolders.szDataDir, "rb", 0);
	}
if (!i)
	i = cf.Open (pszFile, gameFolders.szDataDir, "rb", 0);
	// the following is a hack to enable the loading of d2 levels
	// even if only the d2 mac shareware datafiles are present.
	// However, if the pig file is present but the palette file isn't,
	// the textures in the level will look wierd...
if (!i)
	i = cf.Open (DEFAULT_LEVEL_PALETTE, gameFolders.szDataDir, "rb", 0);
if (!i) {
	Error(TXT_PAL_FILES, pszFile, DEFAULT_LEVEL_PALETTE);
	return NULL;
	}
fsize	= cf.Length ();
Assert (fsize == 9472);
palette.Read (cf);
cf.Read (m_data.fadeTable, sizeof (m_data.fadeTable), 1);
cf.Close ();
// This is the TRANSPARENCY COLOR
for (i = 0; i < MAX_FADE_LEVELS; i++)
	m_data.fadeTable [i * 256 + 255] = 255;
// swap colors 0 and 255 of the palette along with fade table entries
#ifdef SWAP_0_255
palette.SwapTransparency ();
for (i = 0; i < MAX_FADE_LEVELS * 256; i++)
	if (m_fadeTable [i] == 0)
		m_fadeTable [i] = 255;
for (i = 0; i < MAX_FADE_LEVELS; i++)
	m_fadeTable [i * 256] = TRANSPARENCY_COLOR;
#endif
ClearEffect (&palette);
return Add (palette);
}

//------------------------------------------------------------------------------
//load a palette by name. returns 1 if new palette loaded, else 0
//if nUsedForLevel is set, load pig, etc.
//if bNoScreenChange is set, the current screen does not get remapped,
//and the hardware palette does not get changed
CPalette *CPaletteManager::Load (const char *pszPaletteName, const char *pszLevelName, int nUsedForLevel, int bNoScreenChange, int bForce)
{
	char		szPigName [FILENAME_LEN];
	CPalette	*palette = NULL;

	//special hack to tell that palette system about a pig that's been loaded elsewhere
if (nUsedForLevel == -2) {
	paletteManager.SetLastPig (pszPaletteName);
	return m_data.last;
	}
if (!pszPaletteName)
	pszPaletteName = paletteManager.LastPig ();
if (!*pszPaletteName)
	pszPaletteName = "groupa.256";
if (nUsedForLevel && stricmp (paletteManager.LastPig (), pszPaletteName) != 0) {
	if (gameStates.app.bD1Mission)
		strcpy (szPigName, "groupa.pig");
	else {
		_splitpath (const_cast<char*> (pszPaletteName), NULL, NULL, szPigName, NULL);
		strcat (szPigName, ".pig");
		PiggyInitPigFile (szPigName);
		}
	}
if (bForce || pszLevelName || stricmp (paletteManager.LastLoaded (), pszPaletteName)) {
	strncpy (paletteManager.LastLoaded (), pszPaletteName, sizeof (paletteManager.LastLoaded ()));
	palette = paletteManager.Load (pszPaletteName, pszLevelName);
	if (!paletteManager.FadedOut () && !bNoScreenChange)
		LoadEffect ();
	gameData.hud.msgs [0].nColor = -1;
	LoadBackgroundBitmap ();
	}
if (nUsedForLevel && stricmp (paletteManager.LastPig (), pszPaletteName) != 0) {
	strncpy (paletteManager.LastPig (), pszPaletteName, sizeof (paletteManager.LastPig ()));
	TexMergeFlush ();
	RLECacheFlush ();
	}
return palette;
}

//------------------------------------------------------------------------------
/* calculate table to translate d1 bitmaps to current palette,
 * return -1 on error
 */
CPalette* CPaletteManager::LoadD1 (void)
{
	CPalette	palette;
	CFile 	cf;
	
if (!cf.Open (D1_PALETTE, gameFolders.szDataDir, "rb", 1) || (cf.Length () != 9472))
	return NULL;
palette.Read (cf);
cf.Close ();
palette.Raw () [254] = SUPER_TRANSP_COLOR;
palette.Raw () [255] = TRANSPARENCY_COLOR;
SetD1 (Add (palette));
return D1 ();
}

//	-----------------------------------------------------------------------------
//eof
