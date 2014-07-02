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
#include "ogl_lib.h"
#include "addon_bitmaps.h"

//------------------------------------------------------------------------------

CStack<CAddonBitmap*>	CAddonBitmap::m_list;

CAddonBitmap explBlast (const_cast<char*>("blast.tga"));
CAddonBitmap corona (const_cast<char*>("corona.tga"));
CAddonBitmap glare (const_cast<char*>("glare.tga"));
CAddonBitmap halo (const_cast<char*>("halo.tga"));
CAddonBitmap thruster (const_cast<char*>("thruster.tga"));
CAddonBitmap shield (const_cast<char*>("shield.tga"));
CAddonBitmap deadzone (const_cast<char*>("deadzone.tga"));
CAddonBitmap damageIcon [3];
CAddonBitmap scope (const_cast<char*>("scope.tga"));
CAddonBitmap sparks (const_cast<char*>("sparks.tga"));
CAddonBitmap joyMouse (const_cast<char*>("joymouse.tga"));

CAnimation shockwave (const_cast<char*>("shockwave1.tga"), 96);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CAddonBitmap::Register (CAddonBitmap* bmP)
{
if (!m_list.Buffer ()) {
	m_list.Create (10);
	m_list.SetGrowth (10);
	}
m_list.Push (bmP);
}

//------------------------------------------------------------------------------

void CAddonBitmap::Unregister (CAddonBitmap* bmP)
{
if (m_list.Buffer ()) {
	uint32_t i = m_list.Find (bmP);
	if (i < m_list.ToS ())
		m_list.Delete (i);
	}
}

//------------------------------------------------------------------------------

void CAddonBitmap::Prepare (void)
{
for (uint32_t i = 0; i < m_list.ToS (); i++)
	m_list [i]->Bitmap ()->SetupFrames (1, 1);
}

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

int32_t CAddonBitmap::Load (char *pszName) 
{
if (m_bAvailable < 0)
	return 0;
if (m_bAvailable > 0) {
	//ogl.SelectTMU (GL_TEXTURE0);
	//m_bmP->Bind (1);
	return 1;
	}
if (pszName)
	strncpy (m_szName, pszName, sizeof (m_szName));
else
	pszName = m_szName;
if (!*pszName)
	return 0;

if (!m_bAvailable) {
	char	szFilename [FILENAME_LEN];

	sprintf (szFilename, "%sd2x-xl/%s", gameFolders.mods.szTextures [0], pszName);
	if (!m_cf.Exist (szFilename, "", 0))
		sprintf (szFilename, "%sd2x-xl/%s", gameFolders.game.szTextures [0], pszName);
	CreateAndRead (szFilename);
	}
if (!m_bmP)
	m_bAvailable = -1;
else {
	m_bAvailable = 1;
	Register (this);
	m_bmP->SetFrameCount ();
	m_bmP->SetTranspType (-1);
	//m_bmP->Bind (1);
	m_bmP->Texture ()->Wrap (GL_CLAMP);
	}
return (m_bAvailable > 0);
}

//------------------------------------------------------------------------------

void CAddonBitmap::Unload (void)
{
if (m_bmP) {
	Unregister (this);
	delete (m_bmP);
	m_bmP = NULL;
	m_bAvailable = 0;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CAnimation::CAnimation (const char* pszName, uint32_t nFrames) 
	: m_nFrames (nFrames) 
{
if (pszName)
	strncpy (m_szName, pszName, sizeof (m_szName));
else
	*m_szName = '\0';
if (m_nFrames)
	m_frames.Create (m_nFrames);
}

//------------------------------------------------------------------------------

bool CAnimation::Load (const char* pszName) 
{
if (!m_frames.Buffer ())
	return false;

if (!pszName)
	pszName = m_szName;

	char szName [FILENAME_LEN], szFolder [FILENAME_LEN], szFile [FILENAME_LEN], szExt [FILENAME_LEN];

CFile::SplitPath (pszName, szFolder, szFile, szExt);

for (uint32_t i = 0; i < m_nFrames; i++) {
	sprintf (szName, "%s%s-%02d%s", szFolder, szFile, i + 1, szExt);
	if (!m_frames [i].Load (szName)) {
		Destroy ();
		return false;
		}
	}
return true;
}

//------------------------------------------------------------------------------

void CAnimation::Unload (void)
{
for (uint32_t i = 0; i < m_nFrames; i++)
	m_frames [i].Unload ();
}

//------------------------------------------------------------------------------

CBitmap* CAnimation::Bitmap (fix xTTL, fix xLifeLeft) 
{
if (!m_frames.Buffer ())
	return NULL;
uint32_t nFrame = (uint32_t) FRound (float (m_nFrames) * float (xTTL - xLifeLeft) / float (xTTL));
return (nFrame >= m_nFrames) ? NULL : m_frames [nFrame].Bitmap ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32_t LoadAddonBitmap (CBitmap **bmPP, const char *pszName, int32_t *bHaveP, bool bBind)
{
if (!*bHaveP) {
	char	szFilename [FILENAME_LEN];
	CFile	cf;
	CTGA	tga;

	sprintf (szFilename, "%sd2x-xl/%s", gameFolders.mods.szTextures [0], pszName);
	if (!cf.Exist (szFilename, "", 0))
		sprintf (szFilename, "%sd2x-xl/%s", gameFolders.game.szTextures [0], pszName);
	CBitmap* bmP = tga.CreateAndRead (szFilename);
	if (!bmP)
		*bHaveP = -1;
	else {
		*bHaveP = 1;
		bmP->SetFrameCount ();
		bmP->SetTranspType (-1);
		if (bBind)
			bmP->Bind (1);
		}
	*bmPP = bmP;
	}
return *bHaveP > 0;
}

//------------------------------------------------------------------------------

void LoadAddonImages (void)
{
PrintLog (1, "Loading addon images\n");
PrintLog (0, "Loading corona image\n");
corona.Load ();
PrintLog (0, "Loading glare image\n");
glare.Load ();
PrintLog (0, "Loading halo image\n");
halo.Load ();
PrintLog (0, "Loading thruster image\n");
thruster.Load ();
PrintLog (0, "Loading shield image\n");
shield.Load ();
PrintLog (0, "Loading explosion blast image\n");
explBlast.Load ();
PrintLog (0, "Loading spark image\n");
sparks.Load ();
PrintLog (0, "Loading deadzone image\n");
deadzone.Load ();
PrintLog (0, "Loading zoom image\n");
scope.Load ();
PrintLog (0, "Loading damage icons\n");
damageIcon [0].Load (const_cast<char*>("aimdmg.tga"));
damageIcon [1].Load (const_cast<char*>("drivedmg.tga"));
damageIcon [2].Load (const_cast<char*>("gundmg.tga"));
PrintLog (0, "Loading joystick emulator image\n");
joyMouse.Load ();
PrintLog (0, "Loading shockwave animation\n");
shockwave.Load ();
PrintLog (-1);
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
joyMouse.Unload ();
shockwave.Unload ();
}

//------------------------------------------------------------------------------
// eof
