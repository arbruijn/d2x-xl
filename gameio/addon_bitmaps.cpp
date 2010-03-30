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

CAddonBitmap explBlast ("explblast.tga");
CAddonBitmap corona ("corona.tga");
CAddonBitmap glare ("glare.tga");
CAddonBitmap halo ("halo.tga");
CAddonBitmap thruster ("thruster.tga");
CAddonBitmap shield ("shield.tga");
CAddonBitmap deadzone ("deadzone.tga");
CAddonBitmap damageIcon [3];
CAddonBitmap scope ("scope.tga");
CAddonBitmap sparks ("sparks.tga");
CAddonBitmap joyMouse ("joymouse.tga");

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
m_bmP = NULL;
}
//------------------------------------------------------------------------------

int CAddonBitmap::Load (char *pszName) 
{
if (m_bAvailable < 0)
	return 0;
if (pszName)
	strncpy (m_szName, pszName, sizeof (m_szName));
else
	pszName = m_szName;
if (!*pszName)
	return 0;

if (!m_bAvailable) {
	char	szFilename [FILENAME_LEN];
	CFile	cf;

	sprintf (szFilename, "%s/d2x-xl/%s", gameFolders.szTextureDir [2], pszName);
	if (!cf.Exist (szFilename, "", 0))
		sprintf (szFilename, "%s/d2x-xl/%s", gameFolders.szTextureDir [0], pszName);
	m_bmP = CreateAndReadTGA (szFilename);
	}
if (m_bmP)
	m_bAvailable = -1;
else {
	m_bAvailable = 1;
	m_bmP->SetFrameCount ();
	m_bmP->SetTranspType (-1);
	m_bmP->Bind (1);
	m_bmP->Texture ()->Wrap (GL_CLAMP);
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
damageIcon [0].Load ("aimdmg.tga");
damageIcon [1].Load ("drivedmg.tga");
damageIcon [2].Load ("gundmg.tga");
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
damageIcon [0].Unload ();
damageIcon [1].Unload ();
damageIcon [2].Unload ();
}

//------------------------------------------------------------------------------
// eof
