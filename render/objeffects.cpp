#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "descent.h"
#include "newdemo.h"
#include "network.h"
#include "interp.h"
#include "ogl_lib.h"
#include "rendermine.h"
#include "transprender.h"
#include "glare.h"
#include "sphere.h"
#include "marker.h"
#include "fireball.h"
#include "objsmoke.h"
#include "objrender.h"
#include "objeffects.h"
#include "hiresmodels.h"
#include "postprocessing.h"
#include "hitbox.h"

#ifndef fabsf
#	define fabsf(_f)	(float) fabs (_f)
#endif

// -----------------------------------------------------------------------------

void RenderTowedFlag (CObject *pObj)
{
if (gameStates.app.bNostalgia)
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;
if (IsTeamGame && (PLAYER (pObj->info.nId).flags & PLAYER_FLAGS_FLAG)) {

	static CFloatVector fVerts [4] = {
		CFloatVector::Create(0.0f, 2.0f / 3.0f, 0.0f, 1.0f),
		CFloatVector::Create(0.0f, 2.0f / 3.0f, -1.0f, 1.0f),
		CFloatVector::Create(0.0f, -(1.0f / 3.0f), -1.0f, 1.0f),
		CFloatVector::Create(0.0f, -(1.0f / 3.0f), 0.0f, 1.0f)
	};

	static tTexCoord2f texCoordList [2][4] = {
		{{{0.0f, -0.3f}}, {{1.0f, -0.3f}}, {{1.0f, 0.7f}}, {{0.0f, 0.7f}}},
		{{{0.0f, 0.7f}}, {{1.0f, 0.7f}}, {{1.0f, -0.3f}}, {{0.0f, -0.3f}}}
		};


		CFixVector		vPos = pObj->info.position.vPos;
		CFloatVector	vPosf, verts [4];
		tFlagData*		pf = gameData.pig.flags + !GetTeam (pObj->info.nId);
		tPathPoint*		pp = pf->path.GetPoint ();
		int32_t			i, bStencil;
		float				r;
		CBitmap*			pBm;

	if (pp) {
		bStencil = ogl.StencilOff ();
		ogl.SelectTMU (GL_TEXTURE0);
		ogl.SetTexturing (true);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		LoadTexture (pf->bmi.index, 0, 0);
		pBm = gameData.pig.tex.bitmapP + pf->pAnimInfo->frames [pf->animState.nCurFrame].index;
		pBm->SetTranspType (2);
		vPos += pObj->info.position.mOrient.m.dir.f * (-pObj->info.xSize);
		r = X2F (pObj->info.xSize);
		transformation.Begin (vPos, pp->mOrient);
		glColor3f (1.0f, 1.0f, 1.0f);
		for (i = 0; i < 4; i++) {
			vPosf.v.coord.x = 0;
			vPosf.v.coord.y = fVerts [i].v.coord.y * r;
			vPosf.v.coord.z = fVerts [i].v.coord.z * r;
			transformation.Transform (verts [i], vPosf, 0);
			}
		pBm->SetTexCoord (texCoordList [0]);
		ogl.RenderQuad (pBm, verts, 3);
		for (i = 3; i >= 0; i--) {
			vPosf.v.coord.x = 0;
			vPosf.v.coord.y = fVerts [i].v.coord.y * r;
			vPosf.v.coord.z = fVerts [i].v.coord.z * r;
			transformation.Transform (verts [3 - i], vPosf, 0);
			}
		pBm->SetTexCoord (texCoordList [1]);
		ogl.RenderQuad (pBm, verts, 3);
		transformation.End ();
		ogl.BindTexture (0);
		ogl.StencilOn (bStencil);
		}
	}
}

//------------------------------------------------------------------------------

fix flashDist = F2X (0.9f);

//create flash for CPlayerData appearance
void CObject::CreateAppearanceEffect (void)
{
	int8_t	nPlayer = (Type () == OBJ_PLAYER) ? Id () : -1;
	bool		bWarp = (nPlayer >= 0) && ((gameOpts->render.effects.bWarpAppearance == 2) || ((gameOpts->render.effects.bWarpAppearance == 1) && !gameData.multiplayer.bTeleport [nPlayer]));

if (bWarp && (nPlayer == N_LOCALPLAYER) && !AppearanceStage ()) {
	gameData.multiplayer.tAppearing [Id ()][0] = gameData.multiplayer.tAppearing [Id ()][1] = -APPEARANCE_DELAY;
	SetChaseCam (1);
	}
else {
	CFixVector vPos = info.position.vPos;
	if (this == gameData.objData.pViewer)
		vPos += info.position.mOrient.m.dir.f * FixMul (info.xSize, flashDist);
	CObject* pEffectObj = bWarp ? CreateExplBlast (true) : CreateExplosion (info.nSegment, vPos, info.xSize, ANIM_PLAYER_APPEARANCE);
	if (bWarp || pEffectObj) {
		if (pEffectObj) {
			pEffectObj->info.position.mOrient = info.position.mOrient;
			if (gameData.effects.animations [0][ANIM_PLAYER_APPEARANCE].nSound > -1)
				audio.CreateObjectSound (gameData.effects.animations [0][ANIM_PLAYER_APPEARANCE].nSound, SOUNDCLASS_PLAYER, OBJ_IDX (pEffectObj));
			}
		if (!bWarp)
			postProcessManager.Add (new CPostEffectShockwave (SDL_GetTicks (), pEffectObj ? pEffectObj->LifeLeft () : I2X (1), info.xSize, 1, OBJPOS (this)->vPos));
		else {
#if 1
			static CFloatVector color = {{{0.25f, 0.125f, 0.0f, 0.2f}}};
			lightningManager.CreateForTeleport (this, &color);
#endif
			if (pEffectObj) {
				pEffectObj->SetLife (I2X (3) / 4);
#if 1
				pEffectObj->Collapse (true);
#endif
				}
			postProcessManager.Add (new CPostEffectShockwave (SDL_GetTicks (), I2X (1), info.xSize, 1, OBJPOS (this)->vPos));
			gameData.multiplayer.tAppearing [Id ()][0] = gameData.multiplayer.tAppearing [Id ()][1] = I2X (2);
			StopPlayerMovement ();
			}
		}
	}
}

//------------------------------------------------------------------------------
//eof
