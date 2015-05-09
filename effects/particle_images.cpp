
//particle.h
//simple particle system handler

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "pstypes.h"
#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "vecmat.h"
#include "hudmsgs.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "ogl_fastrender.h"
#include "piggy.h"
#include "globvars.h"
#include "segmath.h"
#include "network.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "renderlib.h"
#include "rendermine.h"
#include "transprender.h"
#include "objsmoke.h"
#include "glare.h"
#include "particles.h"
#include "renderthreads.h"
#include "automap.h"
#include "addon_bitmaps.h"

//------------------------------------------------------------------------------

#if 1

tParticleImageInfo particleImageInfo [MAX_PARTICLE_QUALITY + 1][PARTICLE_TYPES] = {
	{{NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0}},

	{{NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "bubble.tga", 4, 0, 0, 1, 0, 0},
	 {NULL, "rain.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smokingfire.tga", 2, 0, 0, 1, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 0, 0, 0},
	 {NULL, "bullcase.tga", 1, 0, 0, 1, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "sparks.tga", 8, 0, 0, 0, 0, 0}},

	{{NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "bubble.tga", 4, 0, 0, 1, 0, 0},
	 {NULL, "rain.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smokingfire.tga", 2, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 0, 0, 0},
	 {NULL, "bullcase.tga", 1, 0, 0, 1, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "sparks.tga", 8, 0, 0, 0, 0, 0}},

	{{NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "bubble.tga", 4, 0, 0, 1, 0, 0},
	 {NULL, "rain.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smokingfire.tga", 2, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 0, 0, 0},
	 {NULL, "bullcase.tga", 1, 0, 0, 1, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "sparks.tga", 8, 0, 0, 0, 0, 0}},
	};

#else

tParticleImageInfo particleImageInfo [MAX_PARTICLE_QUALITY + 1][PARTICLE_TYPES] = {
	{{NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0}},

	{{NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "bubble.tga", 4, 0, 0, 1, 0, 0},
	 {NULL, "rain.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smokingfire.tga", 2, 0, 0, 1, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 0, 0, 0},
	 {NULL, "bullcase.tga", 1, 0, 0, 1, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0}},

	{{NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "bubble.tga", 4, 0, 0, 1, 0, 0},
	 {NULL, "rain.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smokingfire.tga", 2, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 0, 0, 0},
	 {NULL, "bullcase.tga", 1, 0, 0, 1, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0}},

	{{NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "bubble.tga", 4, 0, 0, 1, 0, 0},
	 {NULL, "rain.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smokingfire.tga", 2, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 0, 0, 0},
	 {NULL, "bullcase.tga", 1, 0, 0, 1, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0}},

	{{NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "bubble.tga", 4, 0, 0, 1, 0, 0},
	 {NULL, "rain.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smokingfire.tga", 2, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 0, 0, 0},
	 {NULL, "bullcase.tga", 1, 0, 0, 1, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0}}
	};

#endif

CParticleImageManager particleImageManager;

//	-----------------------------------------------------------------------------

int32_t CParticleImageManager::GetType (int32_t nType)
{
return nType;
}

//	-----------------------------------------------------------------------------

void CParticleImageManager::Animate (int32_t nType)
{
	tParticleImageInfo& pii = ParticleImageInfo (nType);

if (pii.bAnimate && (pii.nFrames > 1)) {
	static time_t to [PARTICLE_TYPES] = {150, 150, 150, 50, 150, 150, 150, 150, 150, 150};
	static time_t t0 [PARTICLE_TYPES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	if (gameStates.app.nSDLTicks [0] - t0 [nType] >= to [nType]) {
		CBitmap*	pBm = ParticleImageInfo (GetType (nType)).pBm;
		if (!(pBm && pBm->Frames ()))
			return;
		pBm->SetCurFrame (pii.iFrame);
		t0 [nType] = gameStates.app.nSDLTicks [0];
		pii.iFrame = (pii.iFrame + 1) % pii.nFrames;
		}
	}
}

//	-----------------------------------------------------------------------------

void CParticleImageManager::AdjustBrightness (CBitmap *pBm)
{
	CBitmap*	pBmf;
	int32_t		i, j = pBm->FrameCount ();
	float*	fFrameBright, fAvgBright = 0, fMaxBright = 0;

if (j < 2)
	return;
if (!(fFrameBright = new float [j]))
	return;
for (i = 0, pBmf = pBm->Frames (); i < j; i++, pBmf++) {
	CTGA tga (pBmf);
	fAvgBright += (fFrameBright [i] = (float) tga.Brightness ());
	if (fMaxBright < fFrameBright [i])
		fMaxBright = fFrameBright [i];
	}
fAvgBright /= j;
for (i = 0, pBmf = pBm->Frames (); i < j; i++, pBmf++) {
	CTGA tga (pBmf);
	tga.ChangeBrightness (0, 1, 2 * (int32_t) (255 * fFrameBright [i] * (fAvgBright - fFrameBright [i])), 0);
	}
delete[] fFrameBright;
}

//	-----------------------------------------------------------------------------

	static inline bool Bind (int32_t nType)
	{
	if (!ogl.m_features.bTextureArrays.Available ())
		return true;
	return (nType != SPARK_PARTICLES) && (nType != SMOKE_PARTICLES) && (nType != BUBBLE_PARTICLES);
	}


int32_t CParticleImageManager::Load (int32_t nType, int32_t bForce)
{
nType = particleImageManager.GetType (nType);

	tParticleImageInfo&	pii = ParticleImageInfo (nType);

if (pii.bHave && !bForce)
	return pii.bHave > 0;
pii.bHave = 0;
if (!LoadAddonBitmap (&pii.pBm, pii.szName, &pii.bHave, Bind (nType)))
	return 0;

#if 0
if (strstr (pii.szName, "smoke")) {
	CTGA tga (pii.pBm);
	tga.PreMultiplyAlpha (0.1f);
	tga.ConvertToRGB ();
	}
#endif

#if MAKE_SMOKE_IMAGE
{
	tTGAHeader h;

TGAInterpolate (pBm, 2);
if (TGAMakeSquare (pBm)) {
	memset (&h, 0, sizeof (h));
	SaveTGA (ParticleImageInfo (nType).szName, gameFolders.game.szData [0], &h, pBm);
	}
}
#endif
pii.pBm->SetFrameCount ();
pii.pBm->SetupTexture (0, 1);
pii.xBorder = 
pii.yBorder = 0;
if (nType <= SMOKE_PARTICLES)
	;//pii.nFrames = 8; 
else if (nType == BUBBLE_PARTICLES)
	;//pii.nFrames = 4;
else if (nType == RAIN_PARTICLES)
	;//pii.nFrames = 4;
else if (nType == SNOW_PARTICLES)
	;//pii.nFrames = 4;
else if (nType == WATERFALL_PARTICLES)
	;//pii.nFrames = 8;
else if (nType == FIRE_PARTICLES) {
	;//pii.nFrames = 4; 
	pii.xBorder = 1.0f / float (pii.pBm->Width ());
	pii.yBorder = 1.0f / float (pii.pBm->Height ());
	}
else {
	pii.nFrames = pii.pBm->FrameCount ();
	pii.bAnimate = pii.nFrames > 1;
	}
return 1;
}

//	-----------------------------------------------------------------------------

int32_t CParticleImageManager::LoadAll (void)
{
if (!gameOpts->render.particles.nQuality)
	return 0;
for (int32_t i = 0; i < PARTICLE_TYPES; i++) {
	if (!Load (i))
		return 0;
	Animate (i);
	}
SetupMultipleTextures (ParticleImageInfo (SPARK_PARTICLES).pBm, ParticleImageInfo (SMOKE_PARTICLES).pBm, ParticleImageInfo (BUBBLE_PARTICLES).pBm);
return 1;
}

//	-----------------------------------------------------------------------------

void CParticleImageManager::FreeAll (void)
{
if (m_textureArray) {
	ogl.DeleteTextures (1, &m_textureArray);
	m_textureArray = 0;
	}

	int32_t	i, j;
	tParticleImageInfo* pInfo = particleImageInfo [0];

for (i = 0; i < MAX_PARTICLE_QUALITY + 1; i++)
	for (j = 0; j < PARTICLE_TYPES; j++, pInfo++)
		if (pInfo->pBm) {
			delete pInfo->pBm;
			pInfo->pBm = NULL;
			pInfo->bHave = 0;
			}
}

//-------------------------------------------------------------------------

bool CParticleImageManager::SetupMultipleTextures (CBitmap* bmP1, CBitmap* bmP2, CBitmap* bmP3)
{
if (!bmP1)
	return false;

int32_t nWidth = bmP1->Width ();
int32_t nHeight = bmP1->Height ();

if ((bmP2->Width () != nWidth) || (bmP2->Height () != nHeight) || 
	 (bmP3->Width () != nWidth) || (bmP3->Height () != nHeight))
	return false;

if (!ogl.m_features.bTextureArrays.Available ())
	return false;
if (m_textureArray)
	return true;

ogl.GenTextures (1, &m_textureArray);
if (!m_textureArray)
	return false;

static GLfloat borderColor [4] = {0.0, 0.0, 0.0, 0.0};

glBindTexture (GL_TEXTURE_2D_ARRAY_EXT, m_textureArray);
glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
glTexParameterf (GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameterf (GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameterf (GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
glTexParameterf (GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
glTexParameterfv (GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_BORDER_COLOR, borderColor);
glTexImage3D (GL_TEXTURE_2D_ARRAY_EXT, 0, GL_RGBA, nWidth, nHeight, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
glTexSubImage3D (GL_TEXTURE_2D_ARRAY_EXT, 0, 0, 0, 0, nWidth, nHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE, bmP1->Buffer ());
glTexSubImage3D (GL_TEXTURE_2D_ARRAY_EXT, 0, 0, 0, 1, nWidth, nHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE, bmP2->Buffer ());
glTexSubImage3D (GL_TEXTURE_2D_ARRAY_EXT, 0, 0, 0, 2, nWidth, nHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE, bmP3->Buffer ());
return true;
}
	
//-------------------------------------------------------------------------

bool CParticleImageManager::LoadMultipleTextures (int32_t nTMU)
{
if (!ogl.m_features.bTextureArrays.Available ())
	return false;
if (!m_textureArray)
	return false;
ogl.SelectTMU (nTMU);
ogl.SetTexturing (true);
ogl.BindTexture (m_textureArray);
return true;
}

//------------------------------------------------------------------------------
//eof
