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

void RenderTowedFlag (CObject *objP)
{
if (gameStates.app.bNostalgia)
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;
if (IsTeamGame && (PLAYER (objP->info.nId).flags & PLAYER_FLAGS_FLAG)) {

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


		CFixVector		vPos = objP->info.position.vPos;
		CFloatVector	vPosf, verts [4];
		tFlagData		*pf = gameData.pig.flags + !GetTeam (objP->info.nId);
		tPathPoint		*pp = pf->path.GetPoint ();
		int32_t				i, bStencil;
		float				r;
		CBitmap		*bmP;

	if (pp) {
		bStencil = ogl.StencilOff ();
		ogl.SelectTMU (GL_TEXTURE0);
		ogl.SetTexturing (true);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		LoadTexture (pf->bmi.index, 0, 0);
		bmP = gameData.pig.tex.bitmapP + pf->vcP->frames [pf->vci.nCurFrame].index;
		bmP->SetTranspType (2);
		vPos += objP->info.position.mOrient.m.dir.f * (-objP->info.xSize);
		r = X2F (objP->info.xSize);
		transformation.Begin (vPos, pp->mOrient);
		glColor3f (1.0f, 1.0f, 1.0f);
		for (i = 0; i < 4; i++) {
			vPosf.v.coord.x = 0;
			vPosf.v.coord.y = fVerts [i].v.coord.y * r;
			vPosf.v.coord.z = fVerts [i].v.coord.z * r;
			transformation.Transform (verts [i], vPosf, 0);
			}
		bmP->SetTexCoord (texCoordList [0]);
		ogl.RenderQuad (bmP, verts, 3);
		for (i = 3; i >= 0; i--) {
			vPosf.v.coord.x = 0;
			vPosf.v.coord.y = fVerts [i].v.coord.y * r;
			vPosf.v.coord.z = fVerts [i].v.coord.z * r;
			transformation.Transform (verts [3 - i], vPosf, 0);
			}
		bmP->SetTexCoord (texCoordList [1]);
		ogl.RenderQuad (bmP, verts, 3);
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
	CFixVector	vPos = info.position.vPos;

if (this == gameData.objs.viewerP)
	vPos += info.position.mOrient.m.dir.f * FixMul (info.xSize, flashDist);
CObject* effectObjP = CreateExplosion (info.nSegment, vPos, info.xSize, VCLIP_PLAYER_APPEARANCE);
if (effectObjP) {
	effectObjP->info.position.mOrient = info.position.mOrient;
	if (gameData.effects.vClips [0][VCLIP_PLAYER_APPEARANCE].nSound > -1)
		audio.CreateObjectSound (gameData.effects.vClips [0][VCLIP_PLAYER_APPEARANCE].nSound, SOUNDCLASS_PLAYER, OBJ_IDX (effectObjP));
	if (Type () != OBJ_PLAYER)
		postProcessManager.Add (new CPostEffectShockwave (SDL_GetTicks (), effectObjP->LifeLeft (), info.xSize, 1, OBJPOS (this)->vPos));
	else {
		//effectObjP->SetLife (effectObjP->LifeLeft () * 2);
		postProcessManager.Add (new CPostEffectShockwave (SDL_GetTicks (), effectObjP->LifeLeft (), info.xSize * 2, 1, OBJPOS (this)->vPos));
		gameData.multiplayer.tAppearing [Id ()][0] = gameData.multiplayer.tAppearing [Id ()][1] = effectObjP->LifeLeft () * 2;
		if (this == &LOCALOBJECT) 
			SetChaseCam (1);
		}
	}
}

//------------------------------------------------------------------------------
//eof
