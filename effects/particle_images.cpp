
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
#include "gameseg.h"
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

//------------------------------------------------------------------------------

tParticleImageInfo particleImageInfo [4][PARTICLE_TYPES] = {
	{{NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0},
	 {NULL, "", 1, 0, 0, 0, 0, 0}},
	{{NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "bubble.tga", 4, 0, 0, 1, 0, 0},
	 {NULL, "smokingfire.tga", 2, 0, 0, 1, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 0, 0, 0},
	 {NULL, "bullcase.tga", 1, 0, 0, 1, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0}},
	{{NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "bubble.tga", 4, 0, 0, 1, 0, 0},
	 {NULL, "smokingfire.tga", 2, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 0, 0, 0},
	 {NULL, "bullcase.tga", 1, 0, 0, 1, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0}},
	{{NULL, "simplesmoke.tga", 1, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 1, 0, 0},
	 {NULL, "bubble.tga", 4, 0, 0, 1, 0, 0},
	 {NULL, "smokingfire.tga", 2, 0, 0, 0, 0, 0},
	 {NULL, "smoke.tga", 8, 0, 0, 0, 0, 0},
	 {NULL, "bullcase.tga", 1, 0, 0, 1, 0, 0},
	 {NULL, "corona.tga", 1, 0, 0, 0, 0, 0}}
	};

CParticleImageManager particleImageManager;

//	-----------------------------------------------------------------------------

int CParticleImageManager::GetType (int nType)
{
return (nType == GATLING_PARTICLES) ? LIGHT_PARTICLES : nType;
}

//	-----------------------------------------------------------------------------

void CParticleImageManager::Animate (int nType)
{
	tParticleImageInfo& pii = ParticleImageInfo (nType);

if (pii.bAnimate && (pii.nFrames > 1)) {
	static time_t to [PARTICLE_TYPES] = {150, 150, 150, 50, 150, 150, 150};
	static time_t t0 [PARTICLE_TYPES] = {0, 0, 0, 0, 0, 0, 0};

	if (gameStates.app.nSDLTicks - t0 [nType] >= to [nType]) {
		CBitmap*	bmP = ParticleImageInfo (GetType (nType)).bmP;
		if (!bmP->Frames ())
			return;
		bmP->SetCurFrame (pii.iFrame);
		t0 [nType] = gameStates.app.nSDLTicks;
		pii.iFrame = ++pii.iFrame % pii.nFrames;
		}
	}
}

//	-----------------------------------------------------------------------------

void CParticleImageManager::AdjustBrightness (CBitmap *bmP)
{
	CBitmap*	bmfP;
	int		i, j = bmP->FrameCount ();
	float*	fFrameBright, fAvgBright = 0, fMaxBright = 0;

if (j < 2)
	return;
if (!(fFrameBright = new float [j]))
	return;
for (i = 0, bmfP = bmP->Frames (); i < j; i++, bmfP++) {
	fAvgBright += (fFrameBright [i] = (float) TGABrightness (bmfP));
	if (fMaxBright < fFrameBright [i])
		fMaxBright = fFrameBright [i];
	}
fAvgBright /= j;
for (i = 0, bmfP = bmP->Frames (); i < j; i++, bmfP++) {
	TGAChangeBrightness (bmfP, 0, 1, 2 * (int) (255 * fFrameBright [i] * (fAvgBright - fFrameBright [i])), 0);
	}
delete[] fFrameBright;
}

//	-----------------------------------------------------------------------------

int CParticleImageManager::Load (int nType)
{
nType = particleImageManager.GetType (nType);

	tParticleImageInfo&	pii = ParticleImageInfo (nType);

if (pii.bHave)
	return 1;
if (!LoadAddonBitmap (&pii.bmP, pii.szName, &pii.bHave))
	return 0;
#if MAKE_SMOKE_IMAGE
{
	tTgaHeader h;

TGAInterpolate (bmP, 2);
if (TGAMakeSquare (bmP)) {
	memset (&h, 0, sizeof (h));
	SaveTGA (ParticleImageInfo (nType).szName, gameFolders.szDataDir, &h, bmP);
	}
}
#endif
pii.bmP->SetFrameCount ();
pii.bmP->SetupTexture (0, 1);
pii.xBorder = 
pii.yBorder = 0;
if (nType <= SMOKE_PARTICLES)
	;//pii.nFrames = 8; 
else if (nType == BUBBLE_PARTICLES)
	;//pii.nFrames = 4;
else if (nType == WATERFALL_PARTICLES)
	;//pii.nFrames = 8;
else if (nType == FIRE_PARTICLES) {
	;//pii.nFrames = 4; 
	pii.xBorder = 1.0f / float (pii.bmP->Width ());
	pii.yBorder = 1.0f / float (pii.bmP->Height ());
	}
else {
	pii.nFrames = pii.bmP->FrameCount ();
	pii.bAnimate = pii.nFrames > 1;
	}
return 1;
}

//	-----------------------------------------------------------------------------

int CParticleImageManager::LoadAll (void)
{
for (int i = 0; i < PARTICLE_TYPES; i++) {
	if (!Load (i))
		return 0;
	Animate (i);
	}
return 1;
}

//	-----------------------------------------------------------------------------

void CParticleImageManager::FreeAll (void)
{
	int	i, j;
	tParticleImageInfo* piiP = particleImageInfo [0];

for (i = 0; i < 2; i++)
	for (j = 0; j < PARTICLE_TYPES; j++, piiP++)
		if (piiP->bmP) {
			delete piiP->bmP;
			piiP->bmP = NULL;
			piiP->bHave = 0;
			}
}

//------------------------------------------------------------------------------
//eof
