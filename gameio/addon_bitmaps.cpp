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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "error.h"
#include "addon_bitmaps.h"

//------------------------------------------------------------------------------

CAddonBitmap explBlast("explblast.tga");
CAddonBitmap corona("corona.tga");
CAddonBitmap glare("glare.tga");
CAddonBitmap halo("halo.tga");
CAddonBitmap thruster("thruster.tga");
CAddonBitmap shield("shield.tga");
CAddonBitmap deadzone("deadzone.tga");
CAddonBitmap damageIcons [3];
CAddonBitmap scope("scope.tga");
CAddonBitmap sparks("sparks.tga");
CAddonBitmap joyMouse("joymouse.tga");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CAddonBitmap::CAddonBitmap (char *pszName) 
{
if (pszName)
	strncpy (m_szName, pszName, sizeof (m_szName));
else
	m_szName [0] = '\0';
m_bAvailable = 0;
m_bmp = NULL;
}
//------------------------------------------------------------------------------

int CAddonBitmap::Load (char *pszName) 
{
if (m_bAvailable)
	return (m_bAvailable > 0);
if (pszName)
	strncpy (m_szName, pszName, sizeof (m_szName));
else
	pszName = m_szName;
if (!pszName)
	return 0;

char	szFilename [FILENAME_LEN];
CFile	cf;

sprintf (szFilename, "%s/d2x-xl/%s", gameFolders.szTextureDir [2], pszName);
if (!cf.Exist (szFilename, "", 0))
	sprintf (szFilename, "%s/d2x-xl/%s", gameFolders.szTextureDir [0], pszName);
m_bmP = CreateAndReadTGA (szFilename);
if (m_bmP)
	m_bAvailable = -1;
else {
	m_bAvailable = 1;
	m_bmP->SetFrameCount ();
	m_bmP->SetTranspType (-1);
	m_bmP->Bind (1);
	}
return (m_bAvailable > 0);
}

//------------------------------------------------------------------------------

void CAddonBitmap::Unload (void)
{
if (m_bmP) {
	delete (m_bmP);
	m_bmP = NULL;
	m_bAvailable = 0;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int LoadAddonBitmap (CBitmap **bmPP, const char *pszName, int *bHaveP)
{
if (!*bHaveP) {
	char	szFilename [FILENAME_LEN];
	CFile	cf;

	sprintf (szFilename, "%s/d2x-xl/%s", gameFolders.szTextureDir [2], pszName);
	if (!cf.Exist (szFilename, "", 0))
		sprintf (szFilename, "%s/d2x-xl/%s", gameFolders.szTextureDir [0], pszName);
	CBitmap *bmP = CreateAndReadTGA (szFilename);
	if (!bmP)
		*bHaveP = -1;
	else {
		*bHaveP = 1;
		bmP->SetFrameCount ();
		bmP->SetTranspType (-1);
		bmP->Bind (1);
		}
	*bmPP = bmP;
	}
return *bHaveP > 0;
}

//------------------------------------------------------------------------------

CBitmap *bmpScope = NULL;
int bHaveScope = 0;

int LoadScope (void)
{
return LoadAddonBitmap (&bmpScope, "scope.tga", &bHaveScope);
}

//------------------------------------------------------------------------------

void FreeScope (void)
{
if (bmpScope) {
	delete bmpScope;
	bHaveScope = 0;
	}
}

//------------------------------------------------------------------------------

CBitmap *bmpDamageIcon [3] = {NULL, NULL, NULL};
int bHaveDamageIcon [3] = {0,0,0};

const char* szDamageIcon [3] = {"aimdmg.tga", "drivedmg.tga", "gundmg.tga"};

//------------------------------------------------------------------------------

int LoadDamageIcon (int i)
{

i %= sizeofa (bmpDamageIcon);
return LoadAddonBitmap (bmpDamageIcon + i, szDamageIcon [i], bHaveDamageIcon + i);
}

//------------------------------------------------------------------------------

void FreeDamageIcon (int i)
{
if (bmpDamageIcon [i]) {
	delete bmpDamageIcon [i];
	bHaveDamageIcon [i] = 0;
	}
}

//------------------------------------------------------------------------------

void FreeDamageIcons (void)
{
for (int i = 0; i < int (sizeofa (bmpDamageIcon)); i++)
	FreeDamageIcon (i);
}

//------------------------------------------------------------------------------

void LoadDamageIcons (void)
{
for (int i = 0; i < int (sizeofa (bmpDamageIcon)); i++)
	LoadDamageIcon (i);
}

//------------------------------------------------------------------------------

CBitmap *bmpDeadzone = NULL;
int bHaveDeadzone = 0;

int LoadDeadzone (void)
{
return LoadAddonBitmap (&bmpDeadzone, "deadzone.tga", &bHaveDeadzone);
}

//------------------------------------------------------------------------------

void FreeDeadzone (void)
{
if (bmpDeadzone) {
	delete bmpDeadzone;
	bHaveDeadzone = 0;
	}
}

//------------------------------------------------------------------------------

CBitmap *bmpJoyMouse = NULL;
int bHaveJoyMouse = 0;

int LoadJoyMouse (void)
{
return LoadAddonBitmap (&bmpJoyMouse, "joymouse.tga", &bHaveJoyMouse);
}

//------------------------------------------------------------------------------

void FreeJoyMouse (void)
{
if (bmpJoyMouse) {
	delete bmpJoyMouse;
	bHaveJoyMouse = 0;
	}
}

//------------------------------------------------------------------------------

CBitmap *bmpExplBlast = NULL;
int bHaveExplBlast = 0;

int LoadExplBlast (void)
{
return LoadAddonBitmap (&bmpExplBlast, "blast.tga", &bHaveExplBlast);
}

//------------------------------------------------------------------------------

void FreeExplBlast (void)
{
if (bmpExplBlast) {
	delete bmpExplBlast;
	bHaveExplBlast = 0;
	}
}

//------------------------------------------------------------------------------

CBitmap *bmpSparks = NULL;
int bHaveSparks = 0;

int LoadSparks (void)
{
return LoadAddonBitmap (&bmpSparks, "sparks.tga", &bHaveSparks);
}

//------------------------------------------------------------------------------

void FreeSparks (void)
{
if (bmpSparks) {
	delete bmpSparks;
	bmpSparks = NULL;
	bHaveSparks = 0;
	}
}

//------------------------------------------------------------------------------

CBitmap *bmpCorona = NULL;
int bHaveCorona = 0;

int LoadCorona (void)
{
return LoadAddonBitmap (&bmpCorona, "corona.tga", &bHaveCorona);
}

//------------------------------------------------------------------------------

void FreeCorona (void)
{
if (bmpCorona) {
	delete bmpCorona;
	bmpCorona = NULL;
	bHaveCorona = 0;
	}
}

//------------------------------------------------------------------------------

CBitmap *bmpGlare = NULL;
int bHaveGlare = 0;

int LoadGlare (void)
{
return LoadAddonBitmap (&bmpGlare, "glare.tga", &bHaveGlare);
}

//------------------------------------------------------------------------------

void FreeGlare (void)
{
if (bmpGlare) {
	delete bmpGlare;
	bmpGlare = NULL;
	bHaveGlare = 0;
	}
}

//------------------------------------------------------------------------------

CBitmap *bmpHalo = NULL;
int bHaveHalo = 0;

int LoadHalo (void)
{
return LoadAddonBitmap (&bmpHalo, "halo.tga", &bHaveHalo);
}

//------------------------------------------------------------------------------

void FreeHalo (void)
{
if (bmpHalo) {
	delete bmpHalo;
	bmpHalo = NULL;
	bHaveHalo = 0;
	}
}

//------------------------------------------------------------------------------

CBitmap* bmpThruster = NULL;
int bHaveThruster = 0;

int LoadThruster (int nStyle)
{
	static const char* szThruster = {"thruster.tga"};

return LoadAddonBitmap (&bmpThruster, szThruster, &bHaveThruster);
}

//------------------------------------------------------------------------------

void FreeThruster (void)
{
if (bmpThruster) {
	delete bmpThruster;
	bmpThruster = NULL;
	bHaveThruster = 0;
	}
}

//------------------------------------------------------------------------------

CBitmap *bmpShield = NULL;
int bHaveShield = 0;

int LoadShield (void)
{
return LoadAddonBitmap (&bmpShield, "shield.tga", &bHaveShield);
}

//------------------------------------------------------------------------------

void FreeShield (void)
{
if (bmpShield) {
	delete bmpShield;
	bmpShield = NULL;
	bHaveShield = 0;
	}
}

//------------------------------------------------------------------------------

void LoadAddonImages (void)
{
PrintLog ("Loading addon images\n");
PrintLog ("   Loading corona image\n");
corona.Load ();
PrintLog ("   Loading glare image\n");
glare.Load ();
PrintLog ("   Loading halo image\n");
halo.Load ();
PrintLog ("   Loading thruster image\n");
thruster.Load ();
PrintLog ("   Loading shield image\n");
shield.Load ();
PrintLog ("   Loading explosion blast image\n");
explBlast.Load ();
PrintLog ("   Loading spark image\n");
sparks.Load ();
PrintLog ("   Loading deadzone image\n");
deadzone.Load ();
PrintLog ("   Loading zoom image\n");
scope.Load ();
PrintLog ("   Loading damage icons\n");
damageIcons [0].Load ("aimdmg.tga");
damageIcons [1].Load ("drivedmg.tga");
damageIcons [2].Load ("gundmg.tga");
PrintLog ("   Loading joystick emulator image\n");
joyMouse.Load ();
}

//------------------------------------------------------------------------------

void UnloadAddonImages (void)
{
corona.Unload ();
glare.Unload ();
halo.Unload ();
thruster.Unload ();
shield.Unload ();
explBlast.Unload ();
sparks.Unload ();
deadzone.Unload ();
scope.Unload ();
damageIcons [0].Unload ();
damageIcons [1].Unload ();
damageIcons [2].Unload ();
}

//------------------------------------------------------------------------------
// eof
