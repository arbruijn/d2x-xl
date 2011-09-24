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
#include "hitbox.h"

#ifndef fabsf
#	define fabsf(_f)	(float) fabs (_f)
#endif

#define IS_TRACK_GOAL(_objP)	(((_objP) == gameData.objs.trackGoals [0]) || ((_objP) == gameData.objs.trackGoals [1]))

// -----------------------------------------------------------------------------

static inline CFloatVector3 *ObjectFrameColor (CObject *objP, CFloatVector3 *pc)
{
	static CFloatVector3	defaultColor = {0, 1.0f, 0};
	static CFloatVector3	botDefColor = {1.0f, 0, 0};
	static CFloatVector3	reactorDefColor = {0.5f, 0, 0.5f};
	static CFloatVector3	playerDefColors [] = {{0,1.0f,0},{0,0,1.0f},{1.0f,0,0}};

if (pc)
	return pc;
if (objP) {
	if (objP->info.nType == OBJ_REACTOR)
		return &reactorDefColor;
	else if (objP->info.nType == OBJ_ROBOT) {
		if (!ROBOTINFO (objP->info.nId).companion)
			return &botDefColor;
		}
	else if (objP->info.nType == OBJ_PLAYER) {
		if (IsTeamGame)
			return playerDefColors + GetTeam (objP->info.nId) + 1;
		return playerDefColors;
		}
	}
return &defaultColor;
}

// -----------------------------------------------------------------------------


static float NDC (float d)
{
#define _A (ZNEAR + ZFAR)
#define _B (ZNEAR - ZFAR)
#define _C (2.0f * ZNEAR * ZFAR)
//#define NDC(z) (2.0f * z - 1.0f)
//#define D(z) (NDC (z) * B)
//#define ZEYE(z) (C / (A + D (z)))
//d = C / (A + D(z))
//C / d = A + D(z)
//C / d - A = D(z)
//C / d - A = NDC(z) * B
//(C / d - A) / B = NDC(z)
//(C / d - A) / B = 2.0 * z - 1.0
//(C / d - A) / B + 1.0 = 2.0 * z
//z = ((C / d - A) / B + 1.0) * 0.5f
return ((_C / d - _A) / _B + 1.0f) * 0.5f;
}

// -----------------------------------------------------------------------------
// Scale the distance between target and viewer with the view range and stretch
// the scale to increase the alpha scaling effect.

static float AlphaScale (CObject* objP)
{
float scale = X2F (CFixVector::Dist (objP->Position (), gameData.objs.viewerP->Position ()));
scale = /*ZFAR **/ 1.0f - sqrt (scale / ZFAR);
//scale = NDC (scale);
//scale /= ZFAR;
return scale * scale;
}

// -----------------------------------------------------------------------------

void RenderDamageIndicator (CObject *objP, CFloatVector3 *pc)
{
	CFixVector		vPos;
	CFloatVector	vPosf, fVerts [4];
	float				r, r2, w;
	int				bStencil;

if (!SHOW_OBJ_FX)
	return;
if ((gameData.demo.nState == ND_STATE_PLAYBACK) && gameOpts->demo.bOldFormat)
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;
if (EGI_FLAG (bDamageIndicators, 0, 1, 0) && (extraGameInfo [IsMultiGame].bTargetIndicators < 2)) {
	bStencil = ogl.StencilOff ();
	pc = ObjectFrameColor (objP, pc);
	PolyObjPos (objP, &vPos);
	vPosf.Assign (vPos);
	transformation.Transform (vPosf, vPosf, 0);
	r = X2F (objP->info.xSize);
	r2 = r / 10;
	r = r2 * 9;
	w = 2 * r;
	vPosf.v.coord.x -= r;
	vPosf.v.coord.y += r;
	w *= objP->Damage ();
	fVerts [0].v.coord.x = fVerts [3].v.coord.x = vPosf.v.coord.x;
	fVerts [1].v.coord.x = fVerts [2].v.coord.x = vPosf.v.coord.x + w;
	fVerts [0].v.coord.y = fVerts [1].v.coord.y = vPosf.v.coord.y;
	fVerts [2].v.coord.y = fVerts [3].v.coord.y = vPosf.v.coord.y - r2;
	fVerts [0].v.coord.z = fVerts [1].v.coord.z = fVerts [2].v.coord.z = fVerts [3].v.coord.z = vPosf.v.coord.z;
	fVerts [0].v.coord.w = fVerts [1].v.coord.w = fVerts [2].v.coord.w = fVerts [3].v.coord.w = 1;
	float alphaScale = AlphaScale (objP);
	alphaScale *= alphaScale;
	glColor4f (pc->Red (), pc->Green (), pc->Blue (), alphaScale * 2.0f / 3.0f);
	ogl.SetTexturing (false);
	ogl.SetDepthMode (GL_ALWAYS);
	ogl.EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0);
	OglVertexPointer (4, GL_FLOAT, 0, fVerts);
	OglDrawArrays (GL_QUADS, 0, 4);
	w = 2 * r;
	fVerts [1].v.coord.x = fVerts [2].v.coord.x = vPosf.v.coord.x + w;
	//glColor3fv (reinterpret_cast<GLfloat*> (pc));
	glColor4f (pc->Red (), pc->Green (), pc->Blue (), alphaScale);
	OglVertexPointer (4, GL_FLOAT, 0, fVerts);
	OglDrawArrays (GL_LINE_LOOP, 0, 4);
	ogl.DisableClientState (GL_VERTEX_ARRAY);
	ogl.SetDepthMode (GL_LEQUAL);
	ogl.StencilOn (bStencil);
	}
}

// -----------------------------------------------------------------------------

static CFloatVector	trackGoalColor [2] = {{1, 0.5f, 0, 0.8f}, {1, 0.5f, 0, 0.8f}};
static int				nMslLockColor [2] = {0, 0};
static int				nMslLockColorIncr [2] = {-1, -1};
static float			fMslLockGreen [2] = {0.65f, 0.0f};

void RenderMslLockIndicator (CObject *objP)
{
	#define INDICATOR_POSITIONS	60

	static tSinCosf	sinCosInd [INDICATOR_POSITIONS];
	static int			bInitSinCos = 1;
	static int			nMslLockIndPos [2] = {0, 0};
	static int			t0 [2] = {0, 0}, tDelay [2] = {25, 40};

	CFixVector			vPos;
	CFloatVector		fPos, fVerts [3];
	float					r, r2;
	int					nTgtInd, bHasDmg, bMarker = (objP->info.nType == OBJ_MARKER);

if (bMarker) {
	if (objP != markerManager.SpawnObject (-1))
		return;
	}
else {
	if (!EGI_FLAG (bMslLockIndicators, 0, 1, 0))
		return;
	if (!IS_TRACK_GOAL (objP))
		return;
	}
if (gameStates.app.nSDLTicks [0] - t0 [bMarker] > tDelay [bMarker]) {
	t0 [bMarker] = gameStates.app.nSDLTicks [0];
	if (!nMslLockColor [bMarker] || (nMslLockColor [bMarker] == 15))
		nMslLockColorIncr [bMarker] = -nMslLockColorIncr [bMarker];
	nMslLockColor [bMarker] += nMslLockColorIncr [bMarker];
	trackGoalColor [bMarker].Green () = fMslLockGreen [bMarker] + (float) nMslLockColor [bMarker] / 100.0f;
	trackGoalColor [bMarker].Alpha () = AlphaScale (objP);
	nMslLockIndPos [bMarker] = (nMslLockIndPos [bMarker] + 1) % INDICATOR_POSITIONS;
	}
PolyObjPos (objP, &vPos);
fPos.Assign (vPos);
transformation.Transform (fPos, fPos, 0);
r = X2F (objP->info.xSize);
if (bMarker || (objP->info.nType == OBJ_MONSTERBALL))
	r = 17 * r / 12;
r2 = r / 4;

ogl.SetFaceCulling (false);
ogl.DisableClientStates (1, 1, 1, GL_TEXTURE0);
ogl.EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0);
ogl.SelectTMU (GL_TEXTURE0);
ogl.SetDepthMode (GL_ALWAYS);
ogl.SetTexturing (false);
glColor4fv (reinterpret_cast<GLfloat*> (trackGoalColor + bMarker));
if (bMarker || gameOpts->render.cockpit.bRotateMslLockInd) {
	CFloatVector	rotVerts [3];
	CFloatMatrix	mRot;
	int				i, j;

	if (bInitSinCos) {
		ComputeSinCosTable (sizeofa (sinCosInd), sinCosInd);
		bInitSinCos = 0;
		}
	mRot.m.dir.r.v.coord.x =
	mRot.m.dir.u.v.coord.y = sinCosInd [nMslLockIndPos [bMarker]].fCos;
	mRot.m.dir.u.v.coord.x = sinCosInd [nMslLockIndPos [bMarker]].fSin;
	mRot.m.dir.r.v.coord.y = -mRot.m.dir.u.v.coord.x;
	mRot.m.dir.r.v.coord.z =
	mRot.m.dir.u.v.coord.z =
	mRot.m.dir.f.v.coord.x =
	mRot.m.dir.f.v.coord.y = 0;
	mRot.m.dir.f.v.coord.z = 1;

	fVerts [0].v.coord.z =
	fVerts [1].v.coord.z =
	fVerts [2].v.coord.z = 0;
	rotVerts [0].v.coord.w =
	rotVerts [1].v.coord.w =
	rotVerts [2].v.coord.w = 0;
	fVerts [0].v.coord.x = -r2;
	fVerts [1].v.coord.x = +r2;
	fVerts [2].v.coord.x = 0;
	fVerts [0].v.coord.y =
	fVerts [1].v.coord.y = +r;
	fVerts [2].v.coord.y = +r - r2;
	OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), rotVerts);
	for (j = 0; j < 4; j++) {
		for (i = 0; i < 3; i++) {
			rotVerts [i] = mRot * fVerts [i];
			fVerts [i] = rotVerts [i];
			rotVerts [i] += fPos;
			}
		if (bMarker)
			glLineWidth (2);
		OglDrawArrays (bMarker ? GL_LINE_LOOP : GL_TRIANGLES, 0, 3);
#if GL_FALLBACK
		else {
			glBegin (bMarker ? GL_LINE_LOOP : GL_TRIANGLES);
			for (int h = 0; h < 3; h++)
				glVertex3fv (reinterpret_cast<GLfloat*> (rotVerts + h));
			glEnd ();
			}
#endif
		if (bMarker)
			glLineWidth (1);
		if (!j) {	//now rotate by 90 degrees
			mRot.m.dir.r.v.coord.x =
			mRot.m.dir.u.v.coord.y = 0;
			mRot.m.dir.u.v.coord.x = 1;
			mRot.m.dir.r.v.coord.y = -1;
			}
		}
	}
else {
	fVerts [0].v.coord.z =
	fVerts [1].v.coord.z =
	fVerts [2].v.coord.z = fPos.v.coord.z;
	fVerts [0].v.coord.w =
	fVerts [1].v.coord.w =
	fVerts [2].v.coord.w = 1;
	fVerts [0].v.coord.x = fPos.v.coord.x - r2;
	fVerts [1].v.coord.x = fPos.v.coord.x + r2;
	fVerts [2].v.coord.x = fPos.v.coord.x;
	OglVertexPointer (4, GL_FLOAT, 0, fVerts);
	nTgtInd = extraGameInfo [IsMultiGame].bTargetIndicators;
	bHasDmg = !EGI_FLAG (bTagOnlyHitObjs, 0, 1, 0) | (objP->Damage () < 1);
	if (!nTgtInd ||
		 ((nTgtInd == 1) && (!EGI_FLAG (bDamageIndicators, 0, 1, 0) || !bHasDmg)) ||
		 ((nTgtInd == 2) && !bHasDmg)) {
		fVerts [0].v.coord.y =
		fVerts [1].v.coord.y = fPos.v.coord.y + r;
		fVerts [2].v.coord.y = fPos.v.coord.y + r - r2;
		OglDrawArrays (GL_TRIANGLES, 0, 3);
		}
	fVerts [0].v.coord.y =
	fVerts [1].v.coord.y = fPos.v.coord.y - r;
	fVerts [2].v.coord.y = fPos.v.coord.y - r + r2;
	OglDrawArrays (GL_TRIANGLES, 0, 3);
	fVerts [0].v.coord.x =
	fVerts [1].v.coord.x = fPos.v.coord.x + r;
	fVerts [2].v.coord.x = fPos.v.coord.x + r - r2;
	fVerts [0].v.coord.y = fPos.v.coord.y + r2;
	fVerts [1].v.coord.y = fPos.v.coord.y - r2;
	fVerts [2].v.coord.y = fPos.v.coord.y;
	OglDrawArrays (GL_TRIANGLES, 0, 3);
	fVerts [0].v.coord.x =
	fVerts [1].v.coord.x = fPos.v.coord.x - r;
	fVerts [2].v.coord.x = fPos.v.coord.x - r + r2;
	OglDrawArrays (GL_TRIANGLES, 0, 3);
	}
ogl.DisableClientState (GL_VERTEX_ARRAY);
ogl.SetDepthMode (GL_LEQUAL);
ogl.SetFaceCulling (true);
}

// -----------------------------------------------------------------------------

void RenderTargetIndicator (CObject *objP, CFloatVector3 *pc)
{
	CFixVector		vPos;
	CFloatVector	fPos, fVerts [4];
	float				r, r2, r3;
	int				bStencil, nPlayer = (objP->info.nType == OBJ_PLAYER) ? objP->info.nId : -1;

if (!SHOW_OBJ_FX)
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;
if (!EGI_FLAG (bCloakedIndicators, 0, 1, 0)) {
	if (nPlayer >= 0) {
		if ((gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_CLOAKED) && !GetCloakInfo (objP, 0, 0, NULL))
			return;
		}
	else if (objP->info.nType == OBJ_ROBOT) {
		if (objP->cType.aiInfo.CLOAKED && !GetCloakInfo (objP, 0, 0, NULL))
			return;
		}
	}
if (IsTeamGame && EGI_FLAG (bFriendlyIndicators, 0, 1, 0)) {
	if (GetTeam (nPlayer) != GetTeam (N_LOCALPLAYER)) {
		if (!(gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_FLAG))
			return;
		pc = ObjectFrameColor (NULL, NULL);
		}
	}

ogl.SetBlendMode (OGL_BLEND_ALPHA);
RenderMslLockIndicator (objP);

if (EGI_FLAG (bTagOnlyHitObjs, 0, 1, 0) && (objP->Damage () >= 1.0f))
	return;

if (EGI_FLAG (bTargetIndicators, 0, 1, 0)) {
	bStencil = ogl.StencilOff ();
	ogl.SetDepthMode (GL_ALWAYS);
	ogl.SetTexturing (false);
	pc = (EGI_FLAG (bMslLockIndicators, 0, 1, 0) && IS_TRACK_GOAL (objP) &&
			!gameOpts->render.cockpit.bRotateMslLockInd && (extraGameInfo [IsMultiGame].bTargetIndicators != 1)) ?
		  reinterpret_cast<CFloatVector3*> (&trackGoalColor [0]) : ObjectFrameColor (objP, pc);
	PolyObjPos (objP, &vPos);
	fPos.Assign (vPos);
	transformation.Transform (fPos, fPos, 0);
	r = X2F (objP->info.xSize);
	float alphaScale = AlphaScale (objP);
	alphaScale *= alphaScale;
	glColor4f (pc->Red (), pc->Green (), pc->Blue (), alphaScale);
	fVerts [0].v.coord.w = fVerts [1].v.coord.w = fVerts [2].v.coord.w = fVerts [3].v.coord.w = 1;
	OglVertexPointer (4, GL_FLOAT, 0, fVerts);
	if (extraGameInfo [IsMultiGame].bTargetIndicators == 1) {	//square brackets
		r2 = r * 2 / 3;
		fVerts [0].v.coord.x = fVerts [3].v.coord.x = fPos.v.coord.x - r2;
		fVerts [1].v.coord.x = fVerts [2].v.coord.x = fPos.v.coord.x - r;
		fVerts [0].v.coord.y = fVerts [1].v.coord.y = fPos.v.coord.y - r;
		fVerts [2].v.coord.y = fVerts [3].v.coord.y = fPos.v.coord.y + r;
		fVerts [0].v.coord.z =
		fVerts [1].v.coord.z =
		fVerts [2].v.coord.z =
		fVerts [3].v.coord.z = fPos.v.coord.z;
		ogl.EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0);
		OglDrawArrays (GL_LINE_STRIP, 0, 4);
		fVerts [0].v.coord.x = fVerts [3].v.coord.x = fPos.v.coord.x + r2;
		fVerts [1].v.coord.x = fVerts [2].v.coord.x = fPos.v.coord.x + r;
		OglDrawArrays (GL_LINE_STRIP, 0, 4);
		ogl.DisableClientState (GL_VERTEX_ARRAY);
		}
	else {	//triangle
		r2 = r / 3;
		fVerts [0].v.coord.x = fPos.v.coord.x - r2;
		fVerts [1].v.coord.x = fPos.v.coord.x + r2;
		fVerts [2].v.coord.x = fPos.v.coord.x;
		fVerts [0].v.coord.y = fVerts [1].v.coord.y = fPos.v.coord.y + r;
		fVerts [2].v.coord.y = fPos.v.coord.y + r - r2;
		fVerts [0].v.coord.z =
		fVerts [1].v.coord.z =
		fVerts [2].v.coord.z = fPos.v.coord.z;
		ogl.EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0);
		OglDrawArrays (GL_LINE_LOOP, 0, 3);
		if (EGI_FLAG (bDamageIndicators, 0, 1, 0)) {
			r3 = objP->Damage ();
			if (r3 < 1.0f) {
				if (r3 < 0.0f)
					r3 = 0.0f;
				fVerts [0].v.coord.x = fPos.v.coord.x - r2 * r3;
				fVerts [1].v.coord.x = fPos.v.coord.x + r2 * r3;
				fVerts [2].v.coord.x = fPos.v.coord.x;
				fVerts [0].v.coord.y = fVerts [1].v.coord.y = fPos.v.coord.y + r - r2 * (1.0f - r3);
				//fVerts [2].v.c.y = fPos.v.c.y + r - r2;
				}
			}
		glColor4f (pc->Red (), pc->Green (), pc->Blue (), alphaScale * 2.0f / 3.0f);
		OglDrawArrays (GL_TRIANGLES, 0, 3);
		ogl.DisableClientState (GL_VERTEX_ARRAY);
		}
	ogl.SetDepthMode (GL_LEQUAL);
	ogl.StencilOn (bStencil);
	}
RenderDamageIndicator (objP, pc);
}

//------------------------------------------------------------------------------
//eof
