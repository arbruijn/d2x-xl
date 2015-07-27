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

inline int Sqr (int i)
{
return i * i;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CPalette::Init (int nTransparentColor, int nSuperTranspColor)
{
m_nComputedColors = 0;
memset (m_computedColors, 0xff, sizeof (m_computedColors));
InitTransparency (nTransparentColor, nSuperTranspColor);
}

//------------------------------------------------------------------------------

void CPalette::InitTransparency (int nTransparentColor, int nSuperTranspColor)
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

void CPalette::ToRgbaf (ubyte nIndex, CFloatVector& color)
{
color.Set (m_data.rgb [nIndex].Red (), m_data.rgb [nIndex].Green (), m_data.rgb [nIndex].Blue ());
color /= 64.0f;
color.Alpha () = -1;
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

static inline int ColorDelta (CRGBColor& palette, int r, int g, int b)
{
return Sqr (r - palette.Red ()) + Sqr (g - palette.Green ()) + Sqr (b - palette.Blue ());
}

//	-----------------------------------------------------------------------------

int CPalette::ClosestColor (int r, int g, int b)
{
	int				i, n;
	int				nBestValue, nBestIndex, value;
	CComputedColor	*pci, *pcj;

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
	if ((plP->palette.TransparentColor () != palette.TransparentColor ()) || 
		 (plP->palette.SuperTranspColor () != palette.SuperTranspColor ()))
		continue;
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

CPalette *CPaletteManager::Add (CPalette& palette, int nTransparentColor, int nSuperTranspColor)
{
palette.InitTransparency (nTransparentColor, nSuperTranspColor);
if (Find (palette))
	return m_data.current;

	tPaletteList* plP;

if (!(plP = new tPaletteList))
	return NULL;
plP->next = m_data.list;
m_data.list = plP;
plP->palette = palette;
plP->palette.Init (nTransparentColor, nSuperTranspColor);
m_data.nPalettes++;
return Activate (&plP->palette);
}

//	-----------------------------------------------------------------------------

CPalette *CPaletteManager::Add (ubyte* buffer, int nTransparentColor, int nSuperTranspColor)
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

void CPaletteManager::SetEffect (bool bForce)
{
if (gameStates.render.vr.bUseRegCode)
	;//GrPaletteStepUpVR (r, g, b, VR_WHITE_INDEX, VR_BLACK_INDEX);
else
	SetEffect (m_data.effect.Red (), m_data.effect.Green (), m_data.effect.Blue (), bForce);
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
	float	maxVal = (paletteManager.FlashDuration () ? 60 : MAX_PALETTE_ADD) / 64.0f;

m_data.effect.Red () += red;
m_data.effect.Green () += green;
m_data.effect.Blue () += blue;
CLAMP (m_data.effect.Red (), -maxVal, maxVal);
CLAMP (m_data.effect.Green (), -maxVal, maxVal);
CLAMP (m_data.effect.Blue (), -maxVal, maxVal);
}

//------------------------------------------------------------------------------

void CPaletteManager::SaveEffect (void)
{
m_data.lastEffect = m_data.effect;
m_data.xLastDuration = m_data.xFlashDuration;
}

//------------------------------------------------------------------------------

void CPaletteManager::ResumeEffect (bool bCond)
{
if (bCond) {
	SetEffect (m_data.lastEffect.Red (), m_data.lastEffect.Green (), m_data.lastEffect.Blue ());
	m_data.xFlashDuration = m_data.xLastDuration;
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
m_data.xFlashDuration = 0;
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
	float	nDelta = 0;
	bool	bForce = false;

	//	Diminish at FADE_RATE units/second.
	//	For frame rates > FADE_RATE Hz, use randomness to achieve this.
if (gameData.time.xFrame < I2X (1) / FADE_RATE) {
	if (RandShort () < gameData.time.xFrame * FADE_RATE / 2)	
		nDelta = 1;
	}
else {
	if (!(nDelta = X2F (gameData.time.xFrame * FADE_RATE)))		// one second = FADE_RATE counts
		nDelta = 1;						// make sure we decrement by something
	}
nDelta /= 64.0f;

if (m_data.xFlashDuration) {
	//	Part of hack system to force update of palette after exiting a menu.
	if (m_data.xLastEffectTime)
		bForce = true;

	if ((m_data.xLastEffectTime + I2X (1)/8 < gameData.time.xGame) || (m_data.xLastEffectTime > gameData.time.xGame)) {
		audio.PlaySound (SOUND_CLOAK_OFF, SOUNDCLASS_GENERIC, m_data.xFlashDuration / 4);
		m_data.xLastEffectTime = gameData.time.xGame;
		}

	m_data.xFlashDuration -= gameData.time.xFrame;
	if (m_data.xFlashDuration < 0)
		m_data.xFlashDuration = 0;

	if (bForce || (RandShort () > 4096)) {
      if ((gameData.demo.nState == ND_STATE_RECORDING) && (m_data.effect.Red () || m_data.effect.Green () || m_data.effect.Blue ()))
	      NDRecordPaletteEffect (short (m_data.effect.Red () * 64), short (m_data.effect.Green () * 64), short (m_data.effect.Blue () * 64));
		paletteManager.SetEffect ();
		return;
		}
	}

m_data.effect.Red () = UpdateEffect (m_data.effect.Red (), nDelta);
m_data.effect.Green () = UpdateEffect (m_data.effect.Green (), nDelta);
m_data.effect.Blue () = UpdateEffect (m_data.effect.Blue (), nDelta);

if ((gameData.demo.nState == ND_STATE_RECORDING) && (m_data.effect.Red () || m_data.effect.Green () || m_data.effect.Blue ()))
     NDRecordPaletteEffect (short (m_data.effect.Red () * 64), short (m_data.effect.Green () * 64), short (m_data.effect.Blue () * 64));
SetEffect (bForce);
}

//------------------------------------------------------------------------------

void CPaletteManager::SetGamma (int gamma)
{
CLAMP (gamma, 0, 16);
if (m_data.nGamma != gamma) {
	m_data.nGamma = gamma;
	}
}

//------------------------------------------------------------------------------

int CPaletteManager::GetGamma (void)
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
	int		i = 0;
	CPalette	palette;
#ifdef SWAP_0_255
	ubyte		coord;
#endif

if (pszLevel) {
	char ifile_name [FILENAME_LEN];

	CFile::ChangeFilenameExtension (ifile_name, pszLevel, ".pal");
	i = cf.Open (ifile_name, gameFolders.szDataDir [0], "rb", 0);
	}
if (!i)
	i = cf.Open (pszFile, gameFolders.szDataDir [0], "rb", 0);
	// the following is a hack to enable the loading of d2 levels
	// even if only the d2 mac shareware datafiles are present.
	// However, if the pig file is present but the palette file isn't,
	// the textures in the level will look wierd...
if (!i)
	i = cf.Open (DEFAULT_LEVEL_PALETTE, gameFolders.szDataDir [0], "rb", 0);
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
#ifdef SWAP_0_255
palette.SwapTransparency ();
for (i = 0; i < MAX_FADE_LEVELS * 256; i++)
	if (m_fadeTable [i] == 0)
		m_fadeTable [i] = 255;
for (i = 0; i < MAX_FADE_LEVELS; i++)
	m_fadeTable [i * 256] = TRANSPARENCY_COLOR;
#endif
ResetEffect ();
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
	//if (!(paletteManager.FadedOut () || bNoScreenChange))
	//	ResumeEffect ();
	gameData.hud.msgs [0].nColor = -1;
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
	
if (!cf.Open (D1_PALETTE, gameFolders.szDataDir [0], "rb", 1) || (cf.Length () != 9472))
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
