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
#include "strutil.h"
#include "newdemo.h"
#include "text.h"
#include "texmerge.h"
#include "rle.h"
#include "palette.h"

//#define Sqr(x) ((x)*(x))

CPaletteManager paletteManager;

char szCurrentLevelPalette [FILENAME_LEN];

//	-----------------------------------------------------------------------------

inline int32_t Sqr (int32_t i)
{
return i * i;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CPalette::Init (int32_t nTransparentColor, int32_t nSuperTranspColor)
{
m_nComputedColors = 0;
memset (m_computedColors, 0xff, sizeof (m_computedColors));
InitTransparency (nTransparentColor, nSuperTranspColor);
}

//------------------------------------------------------------------------------

void CPalette::InitTransparency (int32_t nTransparentColor, int32_t nSuperTranspColor)
{
if (m_transparentColor < 0)
	m_transparentColor = (nTransparentColor < 0) ? TRANSPARENCY_COLOR : nTransparentColor;
if (m_superTranspColor < 0)
	m_superTranspColor = (nSuperTranspColor < 0) ? SUPER_TRANSP_COLOR : nSuperTranspColor;
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

void CPalette::ToRgbaf (uint8_t nIndex, CFloatVector& color)
{
color.Set (m_data.rgb [nIndex].Red (), m_data.rgb [nIndex].Green (), m_data.rgb [nIndex].Blue ());
color /= 64.0f;
color.Alpha () = -1;
}

//------------------------------------------------------------------------------

//	Add a computed color (by GrFindClosestColor) to list of computed colors in m_computedColors.
//	If list wasn't full already, increment gameData.renderData.m_nComputedColors.
//	If was full, replace a random one.
void CPalette::AddComputedColor (int32_t r, int32_t g, int32_t b, int32_t nIndex)
{
	int32_t	i;

if (m_nComputedColors < MAX_COMPUTED_COLORS) {
	i = m_nComputedColors;
	m_nComputedColors++;
	} 
else
	i = (RandShort () * MAX_COMPUTED_COLORS) >> 15;
m_computedColors [i].Set (r, g, b);
m_computedColors [i].nIndex = nIndex;
}

//	-----------------------------------------------------------------------------

void CPalette::InitComputedColors (void)
{
memset (m_computedColors, 0xff, sizeof (m_computedColors));
}

//	-----------------------------------------------------------------------------

static inline int32_t ColorDelta (CRGBColor& palette, int32_t r, int32_t g, int32_t b)
{
return Sqr (r - palette.Red ()) + Sqr (g - palette.Green ()) + Sqr (b - palette.Blue ());
}

//	-----------------------------------------------------------------------------

int32_t CPalette::ClosestColor (int32_t r, int32_t g, int32_t b)
{
	int32_t				i, n;
	int32_t				nBestValue, nBestIndex, value;
	CComputedColor	*pci, *pcj;

#if 0
if (!palette)
	palette = paletteManager.Default ();
if (!palette)
	return -1;
#endif
n = m_nComputedColors;
//if (!n)
//	InitComputedColors (pList);

//	If we've already computed this color, return it!
pci = m_computedColors;
for (i = 0; i < n; i++, pci++)
	if ((r == pci->Red ()) && (g == pci->Green ()) && (b == pci->Blue ())) {
		if (i <= 4)
			return pci->nIndex;
		else {
			CComputedColor	h;
			pcj = pci - 1;
			h = *pcj;
			*pcj = *pci;
			*pci = h;
			return pcj->nIndex;
			}
		}

nBestValue = 3 * Sqr (255) + 1;	// max. possible value + 1
nBestIndex = 0;
// only go to 254, 'cause we dont want to check the transparent 
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
for (int32_t i = 0; i < 3; i++)
	Swap (m_data.raw [i], m_data.raw [765 + i]);
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

int32_t CPaletteManager::FindClosestColor15bpp (int32_t rgb)
{
return m_data.game ? m_data.game->ClosestColor (((rgb >> 10) & 31) * 2, ((rgb >> 5) & 31) * 2, (rgb & 31) * 2) : 1;
}

//	-----------------------------------------------------------------------------

CPalette* CPaletteManager::Find (CPalette& palette)
{
	tPaletteList	*pList;
#if DBG
	int32_t				i;
#endif

for (pList = m_data.list; pList; pList = pList->next)
#if DBG
	{
	if ((pList->palette.TransparentColor () != palette.TransparentColor ()) || 
		 (pList->palette.SuperTranspColor () != palette.SuperTranspColor ()))
		continue;
	for (i = 0; i < PALETTE_SIZE * 3; i++)
		if (palette.Data ().raw [i] != pList->palette.Data ().raw [i])
			break;
	if (i == 768)
		return Activate (&pList->palette);
	}
#else
	if (palette == pList->palette)
		return Activate (&pList->palette);
#endif
return NULL;
}

//	-----------------------------------------------------------------------------

CPalette *CPaletteManager::Add (CPalette& palette, int32_t nTransparentColor, int32_t nSuperTranspColor)
{
palette.InitTransparency (nTransparentColor, nSuperTranspColor);
if (Find (palette))
	return m_data.current;

	tPaletteList* pList;

if (!(pList = new tPaletteList))
	return NULL;
pList->next = m_data.list;
m_data.list = pList;
pList->palette = palette;
pList->palette.Init (nTransparentColor, nSuperTranspColor);
m_data.nPalettes++;
return Activate (&pList->palette);
}

//	-----------------------------------------------------------------------------

CPalette *CPaletteManager::Add (uint8_t* buffer, int32_t nTransparentColor, int32_t nSuperTranspColor)
{
	CPalette	palette;

memcpy (palette.Raw (), buffer, PALETTE_SIZE * 3);
return Add (palette, nTransparentColor, nSuperTranspColor);
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

void CPaletteManager::StartEffect (float red, float green, float blue, bool bForce)
{
if (m_data.nSuspended)
	return;
#if 0
if (!gameStates.render.nLightingMethod) // || gameStates.menus.nInMenu || !gameStates.app.bGameRunning) 
	{
	red += float (m_data.nGamma) / 64.0f;
	green += float (m_data.nGamma) / 64.0f;
	blue += float (m_data.nGamma) / 64.0f;
	}
#endif
m_data.effect.Red () = Clamp (red, 0.0f, 1.0f);
m_data.effect.Green () = Clamp (green, 0.0f, 1.0f);
m_data.effect.Blue () = Clamp (blue, 0.0f, 1.0f);
float maxColor = Max (Max (red, green), blue);
m_data.bDoEffect = (maxColor > 0.0f); //if we arrive here, brightness needs adjustment
SetFadeDuration (m_data.bDoEffect ? /*F2X (64.0 / FADE_RATE)*/I2X (1) / 2 : 0);
}

//------------------------------------------------------------------------------

void CPaletteManager::StartEffect (int32_t red, int32_t green, int32_t blue, bool bForce)
{
StartEffect (Clamp (float (red) / 64.0f, 0.0f, 1.0f), 
	        Clamp (float (green) / 64.0f, 0.0f, 1.0f), 
	        Clamp (float (blue) / 64.0f, 0.0f, 1.0f), 
			  bForce);
}

//------------------------------------------------------------------------------

void CPaletteManager::StopEffect (void)
{
StartEffect (0, 0, 0);
}

//------------------------------------------------------------------------------

int32_t CPaletteManager::StopEffect (CPalette* palette)
{
if (m_data.nLastGamma == m_data.nGamma)
	return 1;
m_data.nLastGamma = m_data.nGamma;
StopEffect ();
m_data.current = Add (*palette);
return 1;
}

//------------------------------------------------------------------------------

int32_t CPaletteManager::DisableEffect (void)
{
#if 0 //obsolete
m_data.nSuspended++; obsolete
#endif
return 0;
}

//------------------------------------------------------------------------------

int32_t CPaletteManager::EnableEffect (bool bReset)
{
#if 0 //obsolete
if (bReset)
	m_data.nSuspended = 0;
else if (m_data.nSuspended > 0)
	m_data.nSuspended--;
#endif
return 0;
}

//------------------------------------------------------------------------------

void CPaletteManager::StartEffect (bool bForce)
{
StartEffect (m_data.effect.Red (), m_data.effect.Green (), m_data.effect.Blue (), bForce);
}

//	------------------------------------------------------------------------------------

#define MAX_PALETTE_ADD 30

void CPaletteManager::BumpEffect (int32_t red, int32_t green, int32_t blue)
{
BumpEffect (float (red) / 64.0f, float (green) / 64.0f, float (blue) / 64.0f);
}

//	------------------------------------------------------------------------------------

void CPaletteManager::BumpEffect (float red, float green, float blue)
{
	float	maxVal = 1.0f; //(paletteManager.FadeDelay () ? 60 : MAX_PALETTE_ADD) / 64.0f;
	float fFade = Clamp (FadeScale (), 0.0f, 1.0f);

red = Clamp (m_data.effect.Red () * fFade + red, 0.0f, maxVal);
green = Clamp (m_data.effect.Green () * fFade + green, 0.0f, maxVal);
blue = Clamp (m_data.effect.Blue () * fFade + blue, 0.0f, maxVal);

StartEffect (Max (red, m_data.effect.Red () * fFade), 
			  Max (green, m_data.effect.Green () * fFade), 
			  Max (blue, m_data.effect.Blue () * fFade),
			  true);
}

//------------------------------------------------------------------------------

void CPaletteManager::SaveEffect (void)
{
m_data.lastEffect = m_data.effect;
m_data.xLastDelay = m_data.xFadeDelay;
}

//------------------------------------------------------------------------------

void CPaletteManager::ResumeEffect (bool bCond)
{
if (bCond) {
	m_data.effect = m_data.lastEffect;
	StartEffect ();
	m_data.xFadeDelay = m_data.xLastDelay;
	m_data.xLastEffectTime = 0;
	}
}

//------------------------------------------------------------------------------

void CPaletteManager::SuspendEffect (bool bCond)
{
if (bCond) {
	SaveEffect ();
	ResetEffect ();
	}
//ResumeEffect ();
}

//------------------------------------------------------------------------------

void CPaletteManager::ResetEffect (void)
{
m_data.xFadeDelay = 0;
m_data.xLastEffectTime = 0;
StartEffect (0, 0, 0);
}

//	------------------------------------------------------------------------------------

void CPaletteManager::UpdateEffect (void)
{
if (m_data.bDoEffect) {
	float	fFade = FadeScale ();
	m_data.flash.Red () = m_data.effect.Red () * fFade;
	m_data.flash.Green () = m_data.effect.Green () * fFade;
	m_data.flash.Blue () = m_data.effect.Blue () * fFade;
	if (m_data.flash.IsZero ())
		StopEffect ();
	}
}

//	------------------------------------------------------------------------------------
//	Diminish palette effects towards Normal.

void CPaletteManager::FadeEffect (void)
{
	bool	bForce = false;

if (m_data.xFadeDelay) {
	//	Part of hack system to force update of palette after exiting a menu.
	if (m_data.xLastEffectTime)
		bForce = true;

	if ((m_data.xLastEffectTime + I2X (1) / 8 < gameData.timeData.xGame) || (m_data.xLastEffectTime > gameData.timeData.xGame)) {
		audio.PlaySound (SOUND_CLOAK_OFF, SOUNDCLASS_GENERIC, m_data.xFadeDelay / 4);
		m_data.xLastEffectTime = gameData.timeData.xGame;
		}

	m_data.xFadeDelay -= gameData.timeData.xFrame;
	if (m_data.xFadeDelay < 0)
		m_data.xFadeDelay = 0;

	if (bForce || (RandShort () > 4096)) {
      if ((gameData.demoData.nState == ND_STATE_RECORDING) && (m_data.effect.Red () || m_data.effect.Green () || m_data.effect.Blue ()))
	      NDRecordPaletteEffect (int16_t (m_data.effect.Red () * 64.0f), int16_t (m_data.effect.Green () * 64.0f), int16_t (m_data.effect.Blue () * 64.0f));
		}
	}
else {
	m_data.xFadeDuration [0] = Max (0, m_data.xFadeDuration [0] - gameData.timeData.xFrame);
	if ((gameData.demoData.nState == ND_STATE_RECORDING) && (m_data.effect.Red () || m_data.effect.Green () || m_data.effect.Blue ()))
		  NDRecordPaletteEffect (int16_t (m_data.effect.Red () * 64.0f), int16_t (m_data.effect.Green () * 64.0f), int16_t (m_data.effect.Blue () * 64.0f));
	}
}

//------------------------------------------------------------------------------

void CPaletteManager::SetGamma (int32_t gamma)
{
CLAMP (gamma, 0, 16);
if (m_data.nGamma != gamma) {
	m_data.nGamma = gamma;
	}
}

//------------------------------------------------------------------------------

int32_t CPaletteManager::GetGamma (void)
{
return m_data.nGamma;
}

//	-----------------------------------------------------------------------------

const char* CPaletteManager::BrightnessLevel (void) 
{
float b = Brightness ();
return (b == 1.0f) ? TXT_STANDARD : (b < 1.0f) ? TXT_LOW : TXT_HIGH;
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
	int32_t		i = 0;
	CPalette	palette;

if (pszLevel) {
	char ifile_name [FILENAME_LEN];

	CFile::ChangeFilenameExtension (ifile_name, pszLevel, ".pal");
	i = cf.Open (ifile_name, gameFolders.game.szData [0], "rb", 0);
	}
if (!i)
	i = cf.Open (pszFile, gameFolders.game.szData [0], "rb", 0);
	// the following is a hack to enable the loading of d2 levels
	// even if only the d2 mac shareware datafiles are present.
	// However, if the pig file is present but the palette file isn't,
	// the textures in the level will look wierd...
if (!i)
	i = cf.Open (DEFAULT_LEVEL_PALETTE, gameFolders.game.szData [0], "rb", 0);
if (!i) {
	Error(TXT_PAL_FILES, pszFile, DEFAULT_LEVEL_PALETTE);
	return NULL;
	}
palette.Read (cf);
cf.Read (m_data.fadeTable, sizeof (m_data.fadeTable), 1);
cf.Close ();
// This is the TRANSPARENCY COLOR
for (i = 0; i < MAX_FADE_LEVELS; i++)
	m_data.fadeTable [i * 256 + 255] = 255;
// swap colors 0 and 255 of the palette along with fade table entries
#ifdef SWAP_TRANSPARENCY_COLOR
palette.SwapTransparency ();
for (i = 0; i < MAX_FADE_LEVELS * 256; i++)
	if (m_data.fadeTable [i] == 0)
		m_data.fadeTable [i] = 255;
for (i = 0; i < MAX_FADE_LEVELS; i++)
	m_data.fadeTable [i * 256] = TRANSPARENCY_COLOR;
#endif
ResetEffect ();
return Add (palette);
}

//------------------------------------------------------------------------------
//load a palette by name. returns 1 if new palette loaded, else 0
//if nUsedForLevel is set, load pig, etc.
//if bNoScreenChange is set, the current screen does not get remapped,
//and the hardware palette does not get changed
CPalette *CPaletteManager::Load (const char *pszPaletteName, const char *pszLevelName, int32_t nUsedForLevel, int32_t bNoScreenChange, int32_t bForce)
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
	//if (!(paletteManager.FadedOut () || bNoScreenChange))
	//	ResumeEffect ();
	gameData.hudData.msgs [0].nColor = -1;
	LoadGameBackground ();
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
	
if (!cf.Open (D1_PALETTE, gameFolders.game.szData [0], "rb", 1) || (cf.Length () != 9472))
	return NULL;
palette.Read (cf);
cf.Close ();
palette.Raw () [254] = SUPER_TRANSP_COLOR;
palette.Raw () [255] = TRANSPARENCY_COLOR;
SetD1 (Add (palette));
return D1 ();
}

//	-----------------------------------------------------------------------------

const float CPaletteManager::Brightness (void) 
{ 
return 0.25f + (float) GetGamma () / 4.0f; 
}

//	-----------------------------------------------------------------------------
//eof
