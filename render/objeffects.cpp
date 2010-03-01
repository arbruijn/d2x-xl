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

void RenderObjectHalo (CFixVector *vPos, fix xSize, float red, float green, float blue, float alpha, int bCorona)
{
if ((gameOpts->render.coronas.bShots && (bCorona ? LoadCorona () : LoadHalo ()))) {
	tRgbaColorf	c = {red, green, blue, alpha};
	ogl.SetDepthWrite (false);
	CBitmap* bmP = bCorona ? bmpCorona : bmpHalo;
	bmP->SetColor (&c);
	ogl.RenderSprite (bmP, *vPos, xSize, xSize, alpha * 4.0f / 3.0f, 1, 1);
	ogl.SetDepthWrite (true);
	}
}

// -----------------------------------------------------------------------------

void RenderPowerupCorona (CObject *objP, float red, float green, float blue, float alpha)
{
	int	bAdditive = gameOpts->render.coronas.bAdditiveObjs;

if ((IsEnergyPowerup (objP->info.nId) ? gameOpts->render.coronas.bPowerups : gameOpts->render.coronas.bWeapons) &&
	 (bAdditive ? LoadGlare () : LoadCorona ())) {
	static tRgbaColorf keyColors [3] = {
	 {0.2f, 0.2f, 0.9f, 0.2f},
	 {0.9f, 0.2f, 0.2f, 0.2f},
	 {0.9f, 0.8f, 0.2f, 0.2f}
		};

	tRgbaColorf color;
	fix			xSize;
	float			fScale;
	CBitmap		*bmP = bAdditive ? bmpGlare : bmpCorona;


	if ((objP->info.nId >= POW_KEY_BLUE) && (objP->info.nId <= POW_KEY_GOLD)) {
		int i = objP->info.nId - POW_KEY_BLUE;

		color = keyColors [(((i < 0) || (i > 2)) ? 3 : i)];
		xSize = I2X (12);
		}
	else {
		float b = (float) sqrt ((red * 3 + green * 5 + blue * 2) / 10);
		color.red = red / b;
		color.green = green / b;
		color.blue = blue / b;
		xSize = I2X (8);
		}
	color.alpha = alpha;
	if (bAdditive) {
		fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
		color.red *= fScale;
		color.green *= fScale;
		color.blue *= fScale;
		}
	bmP->SetColor (&color);
	ogl.RenderSprite (bmP, objP->info.position.vPos, xSize, xSize, alpha, gameOpts->render.coronas.bAdditiveObjs, 5);
	}
}

//------------------------------------------------------------------------------

void TransformHitboxf (CObject *objP, CFloatVector *vertList, int iSubObj)
{

	CFloatVector		hv;
	tHitbox*				phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes + iSubObj;
	CFixVector			vMin = phb->vMin;
	CFixVector			vMax = phb->vMax;
	int					i;

for (i = 0; i < 8; i++) {
	hv [X] = X2F (hitBoxOffsets [i][X] ? vMin [X] : vMax [X]);
	hv [Y] = X2F (hitBoxOffsets [i][Y] ? vMin [Y] : vMax [Y]);
	hv [Z] = X2F (hitBoxOffsets [i][Z] ? vMin [Z] : vMax [Z]);
	transformation.Transform (vertList [i], hv, 0);
	}
}

//------------------------------------------------------------------------------

#if RENDER_HITBOX

void RenderHitbox (CObject *objP, float red, float green, float blue, float alpha)
{
if (objP->rType.polyObjInfo.nModel < 0)
	return;

	CFloatVector	v;
	tHitbox*			pmhb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes.Buffer ();
	tCloakInfo		ci = {0, FADE_LEVELS, 0, 0, 0, 0, 0};
	int				i, j, iBox, nBoxes, bHit = 0;
	float				fFade;

if (!SHOW_OBJ_FX)
	return;
if (objP->info.nType == OBJ_PLAYER) {
	if (!EGI_FLAG (bPlayerShield, 0, 1, 0))
		return;
	if (gameData.multiplayer.players [objP->info.nId].flags & PLAYER_FLAGS_CLOAKED) {
		if (!GetCloakInfo (objP, 0, 0, &ci))
			return;
		fFade = (float) ci.nFadeValue / (float) FADE_LEVELS;
		red *= fFade;
		green *= fFade;
		blue *= fFade;
		}

	}
else if (objP->info.nType == OBJ_ROBOT) {
	if (!(gameOpts->render.effects.bEnabled && gameOpts->render.effects.bRobotShields))
		return;
	if (objP->cType.aiInfo.CLOAKED) {
		if (!GetCloakInfo (objP, 0, 0, &ci))
			return;
		fFade = (float) ci.nFadeValue / (float) FADE_LEVELS;
		red *= fFade;
		green *= fFade;
		blue *= fFade;
		}
	}
if (!EGI_FLAG (nHitboxes, 0, 0, 0)) {
	DrawShieldSphere (objP, red, green, blue, alpha, 1);
	return;
	}
else if (extraGameInfo [IsMultiGame].nHitboxes == 1) {
	iBox =
	nBoxes = 0;
	}
else {
	iBox = 1;
	nBoxes = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].nHitboxes;
	}
ogl.SetDepthMode (GL_LEQUAL);
ogl.SetBlending (true);
ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
ogl.SetTexturing (false);
ogl.SetDepthWrite (false);

tBox hb [MAX_HITBOXES + 1];
TransformHitboxes (objP, &objP->info.position.vPos, hb);

//transformation.Begin (objP->info.position.vPos, objP->info.position.mOrient);
for (; iBox <= nBoxes; iBox++) {
#if 0
	if (iBox)
		transformation.Begin (pmhb [iBox].vOffset, CAngleVector::ZERO);
	TransformHitboxf (objP, vertList, iBox);
#endif
	glColor4f (red, green, blue, alpha / 2);
	glBegin (GL_QUADS);
	for (i = 0; i < 6; i++) {
		for (j = 0; j < 4; j++) {
			v.Assign (hb [iBox].faces [i].v [j]);
			transformation.Transform (v, v, 0);
			glVertex3fv (reinterpret_cast<GLfloat*> (&v));
		//	glVertex3fv (reinterpret_cast<GLfloat*> (vertList + hitboxFaceVerts [i][j]));
			}
		}
	glEnd ();
	glLineWidth (3);
	glColor4f (red, green, blue, alpha);
	for (i = 0; i < 6; i++) {
		CFixVector c;
		c.SetZero ();
		glColor4f (red, green, blue, alpha / 2);
		glBegin (GL_LINES);
		for (j = 0; j < 4; j++) {
			c += hb [iBox].faces [i].v [j];
			v.Assign (hb [iBox].faces [i].v [j]);
			transformation.Transform (v, v, 0);
			glVertex3fv (reinterpret_cast<GLfloat*> (&v));
			//glVertex3fv (reinterpret_cast<GLfloat*> (vertList + hitboxFaceVerts [i][j]));
			}
		c /= I2X (4);
		v.Assign (c);
		glColor4f (1.0f, 0.5f, 0.0f, alpha);
		transformation.Transform (v, v, 0);
		glVertex3fv (reinterpret_cast<GLfloat*> (&v));
		v.Assign (c + hb [iBox].faces [i].n [1] * I2X (5));
		transformation.Transform (v, v, 0);
		glVertex3fv (reinterpret_cast<GLfloat*> (&v));
		glEnd ();
		}
	glLineWidth (1);
//	if (iBox)
//		transformation.End ();
	}
//transformation.End ();
float r = X2F (CFixVector::Dist (pmhb->vMin, pmhb->vMax) / 2);
#if 0 //DBG //display collision point
if (gameStates.app.nSDLTicks - gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].tHit < 500) {
	CObject	o;

	o.info.position.vPos = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].vHit;
	o.info.position.mOrient = objP->info.position.mOrient;
	o.info.xSize = I2X (2);
	objP->rType.polyObjInfo.nModel = -1;
	//SetRenderView (0, NULL);
	DrawShieldSphere (&o, 1, 0, 0, 0.33f, 1);
	}
#endif
ogl.SetDepthWrite (true);
ogl.SetDepthMode (GL_LESS);
}

#endif

// -----------------------------------------------------------------------------

void RenderPlayerShield (CObject *objP)
{
	int			bStencil, dt = 0, i = objP->info.nId, nColor = 0;
	float			alpha, scale = 1;
	tCloakInfo	ci;

	static tRgbaColorf shieldColors [3] = {{0, 0.5f, 1, 1}, {1, 0.5f, 0, 1}, {1, 0.8f, 0.6f, 1}};

if (!SHOW_OBJ_FX)
	return;
if (SHOW_SHADOWS &&
	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 1) : (gameStates.render.nShadowPass != 3)))
	return;
if (EGI_FLAG (bPlayerShield, 0, 1, 0)) {
	if (gameData.multiplayer.players [i].flags & PLAYER_FLAGS_CLOAKED) {
		if (!GetCloakInfo (objP, 0, 0, &ci))
			return;
		scale = (float) ci.nFadeValue / (float) FADE_LEVELS;
		scale *= scale;
		}
	bStencil = ogl.StencilOff ();
	gameData.render.shield.SetPulse (gameData.multiplayer.spherePulse + i);
	if (gameData.multiplayer.bWasHit [i]) {
		if (gameData.multiplayer.bWasHit [i] < 0) {
			gameData.multiplayer.bWasHit [i] = 1;
			gameData.multiplayer.nLastHitTime [i] = gameStates.app.nSDLTicks;
			SetupSpherePulse (gameData.multiplayer.spherePulse + i, 0.1f, 0.5f);
			dt = 0;
			}
		else if ((dt = gameStates.app.nSDLTicks - gameData.multiplayer.nLastHitTime [i]) >= 300) {
			gameData.multiplayer.bWasHit [i] = 0;
			SetupSpherePulse (gameData.multiplayer.spherePulse + i, 0.02f, 0.4f);
			}
		}
#if !RENDER_HITBOX
	if (gameOpts->render.effects.bOnlyShieldHits && !gameData.multiplayer.bWasHit [i])
		return;
#endif
	if (gameData.multiplayer.players [i].flags & PLAYER_FLAGS_INVULNERABLE)
		nColor = 2;
	else if (gameData.multiplayer.bWasHit [i])
		nColor = 1;
	else
		nColor = 0;
	if (gameData.multiplayer.bWasHit [i]) {
		alpha = (gameOpts->render.effects.bOnlyShieldHits ? (float) cos (sqrt ((double) dt / float (SHIELD_EFFECT_TIME)) * Pi / 2) : 1);
		scale *= alpha;
		}
	else if (gameData.multiplayer.players [i].flags & PLAYER_FLAGS_INVULNERABLE)
		alpha = 1;
	else {
		alpha = X2F (gameData.multiplayer.players [i].shields) / 100.0f;
		scale *= alpha;
		if (gameData.multiplayer.spherePulse [i].fSpeed == 0.0f)
			SetupSpherePulse (gameData.multiplayer.spherePulse + i, 0.02f, 0.5f);
		}
#if RENDER_HITBOX
	RenderHitbox (objP, shieldColors [nColor].red * scale, shieldColors [nColor].green * scale, shieldColors [nColor].blue * scale, alpha);
#else
	DrawShieldSphere (objP, shieldColors [nColor].red * scale, shieldColors [nColor].green * scale, shieldColors [nColor].blue * scale, alpha, 1);
#endif
	ogl.StencilOn (bStencil);
	}
}

// -----------------------------------------------------------------------------

void RenderRobotShield (CObject *objP)
{
	static tRgbaColorf shieldColors [3] = {{0.75f, 0, 0.75f, 1}, {0, 0.5f, 1},{1, 0.1f, 0.25f, 1}};

#if RENDER_HITBOX
RenderHitbox (objP, 0.5f, 0.0f, 0.6f, 0.4f);
#else
#endif
#if 1
	float			scale = 1;
	tCloakInfo	ci;
	fix			dt;

if (!(gameOpts->render.effects.bEnabled && gameOpts->render.effects.bRobotShields))
	return;
if ((objP->info.nType == OBJ_ROBOT) && objP->cType.aiInfo.CLOAKED) {
	if (!GetCloakInfo (objP, 0, 0, &ci))
		return;
	scale = (float) ci.nFadeValue / (float) FADE_LEVELS;
	scale *= scale;
	}
dt = gameStates.app.nSDLTicks - objP->TimeLastHit ();
if (dt < SHIELD_EFFECT_TIME) {
	scale *= gameOpts->render.effects.bOnlyShieldHits ? float (cos (sqrt (float (dt) / float (SHIELD_EFFECT_TIME)) * Pi / 2)) : 1;
	DrawShieldSphere (objP, shieldColors [2].red * scale, shieldColors [2].green * scale, shieldColors [2].blue * scale, 0.5f * scale, 1);
	}
else if (!gameOpts->render.effects.bOnlyShieldHits) {
	if ((objP->info.nType != OBJ_ROBOT) || ROBOTINFO (objP->info.nId).companion)
		DrawShieldSphere (objP, 0.0f, 0.5f * scale, 1.0f * scale, objP->Damage () / 2 * scale, 1);
	else
		DrawShieldSphere (objP, 0.75f * scale, 0.0f, 0.75f * scale, objP->Damage () / 2 * scale, 1);
	}
#endif
}

// -----------------------------------------------------------------------------

static inline tRgbColorf *ObjectFrameColor (CObject *objP, tRgbColorf *pc)
{
	static tRgbColorf	defaultColor = {0, 1.0f, 0};
	static tRgbColorf	botDefColor = {1.0f, 0, 0};
	static tRgbColorf	reactorDefColor = {0.5f, 0, 0.5f};
	static tRgbColorf	playerDefColors [] = {{0,1.0f,0},{0,0,1.0f},{1.0f,0,0}};

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

void RenderDamageIndicator (CObject *objP, tRgbColorf *pc)
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
if (EGI_FLAG (bDamageIndicators, 0, 1, 0) &&
	 (extraGameInfo [IsMultiGame].bTargetIndicators < 2)) {
	bStencil = ogl.StencilOff ();
	pc = ObjectFrameColor (objP, pc);
	PolyObjPos (objP, &vPos);
	vPosf.Assign (vPos);
	transformation.Transform (vPosf, vPosf, 0);
	r = X2F (objP->info.xSize);
	r2 = r / 10;
	r = r2 * 9;
	w = 2 * r;
	vPosf [X] -= r;
	vPosf [Y] += r;
	w *= objP->Damage ();
	fVerts [0][X] = fVerts [3][X] = vPosf [X];
	fVerts [1][X] = fVerts [2][X] = vPosf [X] + w;
	fVerts [0][Y] = fVerts [1][Y] = vPosf [Y];
	fVerts [2][Y] = fVerts [3][Y] = vPosf [Y] - r2;
	fVerts [0][Z] = fVerts [1][Z] = fVerts [2][Z] = fVerts [3][Z] = vPosf [Z];
	fVerts [0][W] = fVerts [1][W] = fVerts [2][W] = fVerts [3][W] = 1;
	glColor4f (pc->red, pc->green, pc->blue, 2.0f / 3.0f);
	ogl.SetTexturing (false);
	ogl.EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0);
	OglVertexPointer (4, GL_FLOAT, 0, fVerts);
	OglDrawArrays (GL_QUADS, 0, 4);
	w = 2 * r;
	fVerts [1][X] = fVerts [2][X] = vPosf [X] + w;
	glColor3fv (reinterpret_cast<GLfloat*> (pc));
	OglVertexPointer (4, GL_FLOAT, 0, fVerts);
	OglDrawArrays (GL_LINE_LOOP, 0, 4);
	ogl.DisableClientState (GL_VERTEX_ARRAY);
	ogl.StencilOn (bStencil);
	}
}

// -----------------------------------------------------------------------------

static tRgbaColorf	trackGoalColor [2] = {{1, 0.5f, 0, 0.8f}, {1, 0.5f, 0, 0.8f}};
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
	int					nTgtInd, bHasDmg, bVertexArrays, bMarker = (objP->info.nType == OBJ_MARKER);

if (bMarker) {
	if (objP != SpawnMarkerObject (-1))
		return;
	}
else {
	if (!EGI_FLAG (bMslLockIndicators, 0, 1, 0))
		return;
	if (!IS_TRACK_GOAL (objP))
		return;
	}
if (gameStates.app.nSDLTicks - t0 [bMarker] > tDelay [bMarker]) {
	t0 [bMarker] = gameStates.app.nSDLTicks;
	if (!nMslLockColor [bMarker] || (nMslLockColor [bMarker] == 15))
		nMslLockColorIncr [bMarker] = -nMslLockColorIncr [bMarker];
	nMslLockColor [bMarker] += nMslLockColorIncr [bMarker];
	trackGoalColor [bMarker].green = fMslLockGreen [bMarker] + (float) nMslLockColor [bMarker] / 100.0f;
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
bVertexArrays = ogl.EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0);
ogl.SelectTMU (GL_TEXTURE0);
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
	mRot.RVec ()[X] =
	mRot.UVec ()[Y] = sinCosInd [nMslLockIndPos [bMarker]].fCos;
	mRot.UVec ()[X] = sinCosInd [nMslLockIndPos [bMarker]].fSin;
	mRot.RVec ()[Y] = -mRot.UVec ()[X];
	mRot.RVec ()[Z] =
	mRot.UVec ()[Z] =
	mRot.FVec ()[X] =
	mRot.FVec ()[Y] = 0;
	mRot.FVec ()[Z] = 1;

	fVerts [0][Z] =
	fVerts [1][Z] =
	fVerts [2][Z] = 0;
	rotVerts [0][W] =
	rotVerts [1][W] =
	rotVerts [2][W] = 0;
	fVerts [0][X] = -r2;
	fVerts [1][X] = +r2;
	fVerts [2][X] = 0;
	fVerts [0][Y] =
	fVerts [1][Y] = +r;
	fVerts [2][Y] = +r - r2;
	if (bVertexArrays)
		OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), rotVerts);
	for (j = 0; j < 4; j++) {
		for (i = 0; i < 3; i++) {
			rotVerts [i] = mRot * fVerts [i];
			fVerts [i] = rotVerts [i];
			rotVerts [i] += fPos;
			}
		if (bMarker)
			glLineWidth (2);
		if (bVertexArrays)
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
			mRot.RVec ()[X] =
			mRot.UVec ()[Y] = 0;
			mRot.UVec ()[X] = 1;
			mRot.RVec ()[Y] = -1;
			}
		}
	}
else {
	fVerts [0][Z] =
	fVerts [1][Z] =
	fVerts [2][Z] = fPos [Z];
	fVerts [0][W] =
	fVerts [1][W] =
	fVerts [2][W] = 1;
	fVerts [0][X] = fPos [X] - r2;
	fVerts [1][X] = fPos [X] + r2;
	fVerts [2][X] = fPos [X];
	OglVertexPointer (4, GL_FLOAT, 0, fVerts);
	nTgtInd = extraGameInfo [IsMultiGame].bTargetIndicators;
	bHasDmg = !EGI_FLAG (bTagOnlyHitObjs, 0, 1, 0) | (objP->Damage () < 1);
	if (!nTgtInd ||
		 ((nTgtInd == 1) && (!EGI_FLAG (bDamageIndicators, 0, 1, 0) || !bHasDmg)) ||
		 ((nTgtInd == 2) && !bHasDmg)) {
		fVerts [0][Y] =
		fVerts [1][Y] = fPos [Y] + r;
		fVerts [2][Y] = fPos [Y] + r - r2;
		OglDrawArrays (GL_TRIANGLES, 0, 3);
		}
	fVerts [0][Y] =
	fVerts [1][Y] = fPos [Y] - r;
	fVerts [2][Y] = fPos [Y] - r + r2;
	OglDrawArrays (GL_TRIANGLES, 0, 3);
	fVerts [0][X] =
	fVerts [1][X] = fPos [X] + r;
	fVerts [2][X] = fPos [X] + r - r2;
	fVerts [0][Y] = fPos [Y] + r2;
	fVerts [1][Y] = fPos [Y] - r2;
	fVerts [2][Y] = fPos [Y];
	OglDrawArrays (GL_TRIANGLES, 0, 3);
	fVerts [0][X] =
	fVerts [1][X] = fPos [X] - r;
	fVerts [2][X] = fPos [X] - r + r2;
	OglDrawArrays (GL_TRIANGLES, 0, 3);
	}
ogl.DisableClientState (GL_VERTEX_ARRAY);
ogl.SetFaceCulling (true);
}

// -----------------------------------------------------------------------------

void RenderTargetIndicator (CObject *objP, tRgbColorf *pc)
{
	CFixVector		vPos;
	CFloatVector	fPos, fVerts [4];
	float				r, r2, r3;
	int				bStencil, bDrawArrays, nPlayer = (objP->info.nType == OBJ_PLAYER) ? objP->info.nId : -1;

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
	if (GetTeam (nPlayer) != GetTeam (gameData.multiplayer.nLocalPlayer)) {
		if (!(gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_FLAG))
			return;
		pc = ObjectFrameColor (NULL, NULL);
		}
	}
RenderMslLockIndicator (objP);
if (EGI_FLAG (bTagOnlyHitObjs, 0, 1, 0) && (objP->Damage () >= 1.0f))
	return;
if (EGI_FLAG (bTargetIndicators, 0, 1, 0)) {
	bStencil = ogl.StencilOff ();
	ogl.SetTexturing (false);
	pc = (EGI_FLAG (bMslLockIndicators, 0, 1, 0) && IS_TRACK_GOAL (objP) &&
			!gameOpts->render.cockpit.bRotateMslLockInd && (extraGameInfo [IsMultiGame].bTargetIndicators != 1)) ?
		  reinterpret_cast<tRgbColorf*> (&trackGoalColor [0]) : ObjectFrameColor (objP, pc);
	PolyObjPos (objP, &vPos);
	fPos.Assign (vPos);
	transformation.Transform (fPos, fPos, 0);
	r = X2F (objP->info.xSize);
	glColor3fv (reinterpret_cast<GLfloat*> (pc));
	fVerts [0][W] = fVerts [1][W] = fVerts [2][W] = fVerts [3][W] = 1;
	OglVertexPointer (4, GL_FLOAT, 0, fVerts);
	if (extraGameInfo [IsMultiGame].bTargetIndicators == 1) {	//square brackets
		r2 = r * 2 / 3;
		fVerts [0][X] = fVerts [3][X] = fPos [X] - r2;
		fVerts [1][X] = fVerts [2][X] = fPos [X] - r;
		fVerts [0][Y] = fVerts [1][Y] = fPos [Y] - r;
		fVerts [2][Y] = fVerts [3][Y] = fPos [Y] + r;
		fVerts [0][Z] =
		fVerts [1][Z] =
		fVerts [2][Z] =
		fVerts [3][Z] = fPos [Z];
		if ((bDrawArrays = ogl.EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0)))
			OglDrawArrays (GL_LINE_STRIP, 0, 4);
#if GL_FALLBACK
		else {
			glBegin (GL_LINE_STRIP);
			for (i = 0; i < 4; i++)
				glVertex3fv (reinterpret_cast<GLfloat*> (fVerts + i));
			glEnd ();
			}
#endif
		fVerts [0][X] = fVerts [3][X] = fPos [X] + r2;
		fVerts [1][X] = fVerts [2][X] = fPos [X] + r;
		if (bDrawArrays) {
			OglDrawArrays (GL_LINE_STRIP, 0, 4);
			ogl.DisableClientState (GL_VERTEX_ARRAY);
			}
#if GL_FALLBACK
		else {
			glBegin (GL_LINE_STRIP);
			for (int i = 0; i < 4; i++)
				glVertex3fv (reinterpret_cast<GLfloat*> (fVerts + i));
			glEnd ();
			}
#endif
		}
	else {	//triangle
		r2 = r / 3;
		fVerts [0][X] = fPos [X] - r2;
		fVerts [1][X] = fPos [X] + r2;
		fVerts [2][X] = fPos [X];
		fVerts [0][Y] = fVerts [1][Y] = fPos [Y] + r;
		fVerts [2][Y] = fPos [Y] + r - r2;
		fVerts [0][Z] =
		fVerts [1][Z] =
		fVerts [2][Z] = fPos [Z];
		if ((bDrawArrays = ogl.EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0)))
			OglDrawArrays (GL_LINE_LOOP, 0, 3);
#if GL_FALLBACK
		else {
			glBegin (GL_LINE_LOOP);
			glVertex3fv (reinterpret_cast<GLfloat*> (fVerts));
			glVertex3fv (reinterpret_cast<GLfloat*> (fVerts + 1));
			glVertex3fv (reinterpret_cast<GLfloat*> (fVerts + 2));
			glEnd ();
			}
#endif
		if (EGI_FLAG (bDamageIndicators, 0, 1, 0)) {
			r3 = objP->Damage ();
			if (r3 < 1.0f) {
				if (r3 < 0.0f)
					r3 = 0.0f;
				fVerts [0][X] = fPos [X] - r2 * r3;
				fVerts [1][X] = fPos [X] + r2 * r3;
				fVerts [2][X] = fPos [X];
				fVerts [0][Y] = fVerts [1][Y] = fPos [Y] + r - r2 * (1.0f - r3);
				//fVerts [2][Y] = fPos [Y] + r - r2;
				}
			}
		glColor4f (pc->red, pc->green, pc->blue, 2.0f / 3.0f);
		if (bDrawArrays) {
			OglDrawArrays (GL_TRIANGLES, 0, 3);
			ogl.DisableClientState (GL_VERTEX_ARRAY);
			}
#if GL_FALLBACK
		else {
			glBegin (GL_TRIANGLES);
			for (i = 0; i < 3; i++)
			glVertex3fv (reinterpret_cast<GLfloat*> (fVerts + i));
			glEnd ();
			}
#endif
		}
	ogl.StencilOn (bStencil);
	}
RenderDamageIndicator (objP, pc);
}

// -----------------------------------------------------------------------------

void RenderTowedFlag (CObject *objP)
{
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

if (gameStates.app.bNostalgia)
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;
if (IsTeamGame && (gameData.multiplayer.players [objP->info.nId].flags & PLAYER_FLAGS_FLAG)) {
		CFixVector		vPos = objP->info.position.vPos;
		CFloatVector	vPosf, verts [4];
		tFlagData		*pf = gameData.pig.flags + !GetTeam (objP->info.nId);
		tPathPoint		*pp = pf->path.GetPoint ();
		int				i, bStencil;
		float				r;
		CBitmap		*bmP;

	if (pp) {
		bStencil = ogl.StencilOff ();
		ogl.SelectTMU (GL_TEXTURE0);
		ogl.SetTexturing (true);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		LoadBitmap (pf->bmi.index, 0);
		bmP = gameData.pig.tex.bitmapP + pf->vcP->frames [pf->vci.nCurFrame].index;
		bmP->SetTranspType (2);
		vPos += objP->info.position.mOrient.FVec () * (-objP->info.xSize);
		r = X2F (objP->info.xSize);
		transformation.Begin (vPos, pp->mOrient);
		glColor3f (1.0f, 1.0f, 1.0f);
		for (i = 0; i < 4; i++) {
			vPosf [X] = 0;
			vPosf [Y] = fVerts [i][Y] * r;
			vPosf [Z] = fVerts [i][Z] * r;
			transformation.Transform (verts [i], vPosf, 0);
			}
		bmP->SetTexCoord (texCoordList [0]);
		ogl.RenderQuad (bmP, verts, 3);
		for (i = 3; i >= 0; i--) {
			vPosf [X] = 0;
			vPosf [Y] = fVerts [i][Y] * r;
			vPosf [Z] = fVerts [i][Z] * r;
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

// -----------------------------------------------------------------------------

#define	THRUSTER_SEGS	14	// number of rings the thruster flame is composed of
#define	RING_SEGS		16	// number of segments each ring is composed of

static CFloatVector	vFlame [THRUSTER_SEGS][RING_SEGS];
static int				bHaveFlame = 0;

static CFloatVector	vRingVerts [RING_SEGS] = {
	CFloatVector::Create (-0.5f, -0.5f, 0.0f, 1.0f),
	CFloatVector::Create (-0.6533f, -0.2706f, 0.0f, 1.0f),
	CFloatVector::Create (-0.7071f, 0.0f, 0.0f, 1.0f),
	CFloatVector::Create (-0.6533f, 0.2706f, 0.0f, 1.0f),
	CFloatVector::Create (-0.5f, 0.5f, 0.0f, 1.0f),
	CFloatVector::Create (-0.2706f, 0.6533f, 0.0f, 1.0f),
	CFloatVector::Create (0.0f, 0.7071f, 0.0f, 1.0f),
	CFloatVector::Create (0.2706f, 0.6533f, 0.0f, 1.0f),
	CFloatVector::Create (0.5f, 0.5f, 0.0f, 1.0f),
	CFloatVector::Create (0.6533f, 0.2706f, 0.0f, 1.0f),
	CFloatVector::Create (0.7071f, 0.0f, 0.0f, 1.0f),
	CFloatVector::Create (0.6533f, -0.2706f, 0.0f, 1.0f),
	CFloatVector::Create (0.5f, -0.5f, 0.0f, 1.0f),
	CFloatVector::Create (0.2706f, -0.6533f, 0.0f, 1.0f),
	CFloatVector::Create (0.0f, -0.7071f, 0.0f, 1.0f),
	CFloatVector::Create (-0.2706f, -0.6533f, 0.0f, 1.0f)
};

static int		nStripIdx [] = {0,15,1,14,2,13,3,12,4,11,5,10,6,9,7,8};

void CreateThrusterFlame (void)
{
if (!bHaveFlame) {
		CFloatVector*	pv;
		int				i, j, m, n;
		double			phi, sinPhi;
		float				z = 0,
							fScale = 2.0f / 3.0f,
							fStep [2] = {1.0f / 4.0f, 1.0f / 3.0f};

	// first part with increasing diameter
	pv = &vFlame [0][0];
	for (i = 0, phi = 0; i < 5; i++, phi += Pi / 10, z -= fStep [0]) {
		sinPhi = (1 + sin (phi) / 2) * fScale;
		for (j = 0; j < RING_SEGS; j++, pv++) {
			*pv = vRingVerts [j] * float (sinPhi);
			(*pv) [Z] = z;
			}
		}
	// second part with decreasing diameter
	m = n = THRUSTER_SEGS - i + 1;
	for (phi = Pi / 2; i < THRUSTER_SEGS; i++, phi += Pi / 12, z -= fStep [1], m--) {
		sinPhi = (1 + sin (phi) / 2) * fScale /** m / n*/;
		for (j = 0; j < RING_SEGS; j++, pv++) {
			*pv = vRingVerts [j] * float (sinPhi);
			(*pv) [Z] = z;
			}
		}
	bHaveFlame = 1;
	}
}

// -----------------------------------------------------------------------------

void CalcShipThrusterPos (CObject *objP, CFixVector *vPos)
{
	tObjTransformation	*pPos = OBJPOS (objP);

if (gameOpts->render.bHiresModels [0]) {
	vPos [0] = pPos->vPos + pPos->mOrient.FVec () * (-objP->info.xSize);
	vPos [0] += pPos->mOrient.RVec () * (-(8 * objP->info.xSize / 44));
	vPos [1] = vPos [0] + pPos->mOrient.RVec () * (8 * objP->info.xSize / 22);
	}
else {
	vPos [0] = pPos->vPos + pPos->mOrient.FVec () * (-objP->info.xSize / 10 * 9);
	if (gameStates.app.bFixModels)
		vPos [0] += pPos->mOrient.UVec () * (objP->info.xSize / 40);
	else
		vPos [0] += pPos->mOrient.UVec () * (-objP->info.xSize / 20);
	vPos [1] = vPos [0];
	vPos [0] += pPos->mOrient.RVec () * (-8 * objP->info.xSize / 49);
	vPos [1] += pPos->mOrient.RVec () * (8 * objP->info.xSize / 49);
	}
}

// -----------------------------------------------------------------------------

int CalcThrusterPos (CObject *objP, tThrusterInfo *tiP, int bAfterburnerBlob)
{
	tThrusterInfo		ti;
	CThrusterData		*pt = NULL;
	int					i, nThrusters,
							bMissile = IS_MISSILE (objP),
							bSpectate = SPECTATOR (objP);

ti = *tiP;
ti.pp = NULL;
ti.mtP = gameData.models.thrusters + objP->rType.polyObjInfo.nModel;
nThrusters = ti.mtP->nCount;
if (gameOpts->render.bHiresModels [0] && (objP->info.nType == OBJ_PLAYER) && !GetASEModel (objP->rType.polyObjInfo.nModel)) {
	if (!bSpectate) {
		pt = gameData.render.thrusters + objP->info.nId;
		ti.pp = pt->path.GetPoint ();
		}
	ti.fSize = (ti.fLength + 1) / 2;
	nThrusters = 2;
	CalcShipThrusterPos (objP, ti.vPos);
	ti.mtP = NULL;
	}
else if (bAfterburnerBlob || (bMissile && !nThrusters)) {
		tHitbox	*phb = &gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes [0];
		fix		nObjRad = (phb->vMax [Z] - phb->vMin [Z]) / 2;

	if (bAfterburnerBlob)
		nObjRad *= 2;
	if (objP->info.nId == EARTHSHAKER_ID)
		ti.fSize = 1.0f;
	else if ((objP->info.nId == MEGAMSL_ID) || (objP->info.nId == EARTHSHAKER_MEGA_ID))
		ti.fSize = 0.8f;
	else if (objP->info.nId == SMARTMSL_ID)
		ti.fSize = 0.6f;
	else
		ti.fSize = 0.5f;
	nThrusters = 1;
	if (EGI_FLAG (bThrusterFlames, 1, 1, 0) == 2)
		ti.fLength /= 2;
	if (!gameData.models.vScale.IsZero ())
		ti.vPos [0] *= gameData.models.vScale;
	*ti.vPos = objP->info.position.vPos + objP->info.position.mOrient.FVec () * (-nObjRad);
	ti.mtP = NULL;
	}
else if ((objP->info.nType == OBJ_PLAYER) ||
			((objP->info.nType == OBJ_ROBOT) && !objP->cType.aiInfo.CLOAKED) ||
			bMissile) {
	CFixMatrix	m, *viewP;
	if (!bSpectate && (objP->info.nType == OBJ_PLAYER)) {
		pt = gameData.render.thrusters + objP->info.nId;
		ti.pp = pt->path.GetPoint ();
		}
	if (!nThrusters) {
		if (objP->info.nType != OBJ_PLAYER)
			return 0;
		if (!bSpectate) {
			pt = gameData.render.thrusters + objP->info.nId;
			ti.pp = pt->path.GetPoint ();
			}
		ti.fSize = (ti.fLength + 1) / 2;
		nThrusters = 2;
		CalcShipThrusterPos (objP, ti.vPos);
		}
	else {
		tObjTransformation *posP = OBJPOS (objP);
		if (SPECTATOR (objP)) {
			viewP = &m;
			m = posP->mOrient.Transpose ();
			}
		else
			viewP = objP->View ();
		for (i = 0; i < nThrusters; i++) {
			ti.vPos [i] = *viewP * ti.mtP->vPos [i];
			if (!gameData.models.vScale.IsZero ())
				ti.vPos [i] *= gameData.models.vScale;
			ti.vPos [i] += posP->vPos;
			ti.vDir [i] = *viewP * ti.mtP->vDir [i];
			}
		ti.fSize = ti.mtP->fSize;
		if (bMissile)
			nThrusters = 1;
		}
	}
else
	return 0;
*tiP = ti;
return nThrusters;
}

// -----------------------------------------------------------------------------


void CreateLightTrail (CFixVector& vPos, CFixVector &vDir, float fSize, float fLength, CBitmap *bmP, tRgbaColorf *colorP)
{
	static tTexCoord2f	tcTrail [3] = {{{0,0}},{{1,1}},{{1,0}}};
	static tTexCoord2f	tcCorona [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
	static CFloatVector	vEye = CFloatVector::ZERO;

	CFloatVector	v, vPosf, vNormf, vTrail [3], vCorona [4], fVecf;
	float		c = 1/*0.7f + 0.03f * fPulse*/, dotTrail, dotCorona;

fVecf.Assign (vDir);
vPosf.Assign (vPos);
vTrail [2] = vPosf - fVecf * fLength;
transformation.Transform (vTrail [2], vTrail [2], 0);
transformation.Transform (vPosf, vPosf, 0);
vNormf = CFloatVector::Normal (vTrail [2], vPosf, vEye);
vTrail [0] = vPosf + vNormf * fSize;
vTrail [1] = vPosf - vNormf * fSize;
vNormf = CFloatVector::Normal (vTrail [0], vTrail [1], vTrail [2]);
vCorona [0] = vTrail [0];
vCorona [2] = vTrail [1];
vCorona [1] = vPosf + vNormf * fSize;
vCorona [3] = vPosf + vNormf * (-fSize);
CFloatVector::Normalize (vPosf);
v = vTrail [2];
CFloatVector::Normalize (v);
dotTrail = CFloatVector::Dot (vPosf, v);
v = *vCorona;
CFloatVector::Normalize (v);
dotCorona = CFloatVector::Dot (vPosf, v);
transparencyRenderer.AddLightTrail (bmP, vCorona, tcCorona, (dotTrail < dotCorona) ? vTrail : NULL, tcTrail, colorP);
}

// -----------------------------------------------------------------------------

void RenderThrusterFlames (CObject *objP)
{
	int					h, i, j, k, l, nStyle, nThrusters, bStencil, bSpectate, bTextured;
	tRgbaColorf			color [2];
	tThrusterInfo		ti;
	CFloatVector		v, verts [4];
	float					fSpeed, fPulse, fFade [4];
	CThrusterData*		pt = NULL;
	CBitmap*				bmP;
	int					bPlayer = objP->info.nType == OBJ_PLAYER;

	static time_t		tPulse = 0;
	static int			nPulse = 10;

	static tTexCoord2f	tcThruster [4] = {{{0.0f,0.0f}},{{1.0f,0.0f}},{{1.0f,1.0f}},{{0.0f,1.0f}}};

if (gameStates.app.bNostalgia)
	return;
if (!gameOpts->render.effects.bEnabled)
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;
if (!EGI_FLAG (bThrusterFlames, 1, 1, 0))
	return;
if ((objP->info.nType == OBJ_PLAYER) && (gameData.multiplayer.players [objP->info.nId].flags & PLAYER_FLAGS_CLOAKED))
	return;
fSpeed = X2F (objP->mType.physInfo.velocity.Mag ());
if (!pt || (fSpeed >= pt->fSpeed)) {
	fFade [0] = 0.95f;
	fFade [1] = 0.85f;
	fFade [2] = 0.75f;
	fFade [3] = 0.65f;
	}
else {
	fFade [0] = 0.9f;
	fFade [1] = 0.8f;
	fFade [2] = 0.7f;
	fFade [3] = 0.6f;
	}
if (pt)
	pt->fSpeed = fSpeed;
bSpectate = SPECTATOR (objP);

if (gameStates.app.nSDLTicks - tPulse > 10) {
	tPulse = gameStates.app.nSDLTicks;
	nPulse = d_rand () % 11;
	}
fPulse = float (nPulse) / 10.0f;
ti.pp = NULL;
nThrusters = CalcThrusterPos (objP, &ti, 0);
ti.fLength = fSpeed / 60.0f + 0.5f;
if (ti.fLength < ti.fSize / 2)
	ti.fLength = ti.fSize / 2;
ti.fLength += float (rand () % 100) / 1000.0f;
bStencil = ogl.StencilOff ();
bTextured = 0;
nStyle = EGI_FLAG (bThrusterFlames, 1, 1, 0);

if (!LoadThruster ()) {
	extraGameInfo [IsMultiGame].bThrusterFlames = 2;
	ogl.SetTexturing (false);
	}
else if (nStyle == 2) {
	ogl.SetTexturing (true);
	bmP = bmpThruster [1][bPlayer];
	bmP->SetTranspType (-1);
	if (bmP->Bind (1)) {
		extraGameInfo [IsMultiGame].bThrusterFlames = 2;
		ogl.SetTexturing (false);
		}
	else {
		bmP->Texture ()->Wrap (GL_CLAMP);
		bTextured = 1;
		}
	}
if (nThrusters > 1) {
	CFixVector vRot [2];
	for (i = 0; i < 2; i++)
		transformation.Rotate (vRot [i], ti.vPos [i], 0);
	if (vRot [0][Z] < vRot [1][Z]) {
		CFixVector v = ti.vPos [0];
		ti.vPos [0] = ti.vPos [1];
		ti.vPos [1] = v;
		if (objP->info.nType == OBJ_ROBOT) {
			v = ti.vDir [0];
			ti.vDir [0] = ti.vDir [1];
			ti.vDir [1] = v;
			}
		}
	}

if (nStyle == 1) {	//2D
		static tRgbaColorf	tcColor = {0.75f, 0.75f, 0.75f, 1.0f};
		static CFloatVector	vEye = CFloatVector::ZERO;

		CFloatVector	fVecf;

	if (!gameData.models.vScale.IsZero ())
		ti.fSize *= X2F (gameData.models.vScale [Z]);
	ti.fLength *= 4 * ti.fSize;
	ti.fSize *= ((objP->info.nType == OBJ_PLAYER) && HaveHiresModel (objP->rType.polyObjInfo.nModel)) ? 1.2f : 1.5f;
#if 1
	if (!ti.mtP)
		fVecf.Assign (ti.pp ? ti.pp->mOrient.FVec () : objP->info.position.mOrient.FVec ());
#endif
	for (h = 0; h < nThrusters; h++)
		CreateLightTrail (ti.vPos [h], ti.vDir [h], ti.fSize, ti.fLength, bmpThruster [0][bPlayer], &tcColor);
	}
else { //3D
	tTexCoord2f	tTexCoord2fl, tTexCoord2flStep;

	CreateThrusterFlame ();
	ogl.SetFaceCulling (false);
	ogl.SetBlendMode (GL_ONE, GL_ONE);
	ogl.SetTransform (1);
	ogl.SetDepthWrite (false);

	tTexCoord2flStep.v.u = 1.0f / RING_SEGS;
	tTexCoord2flStep.v.v = 0.9f / THRUSTER_SEGS;
	ti.fLength /= 2;

	for (h = 0; h < nThrusters; h++) {
		transformation.Begin (ti.vPos [h], (ti.pp && !bSpectate) ? ti.pp->mOrient : objP->info.position.mOrient);
		// render a cap for the thruster flame at its base
		if (bTextured && (bTextured = LoadThruster (1))) {
			CBitmap* bmP = bmpThruster [0][bPlayer];
			bmP->SetTranspType (-1);
			if (bmP->Bind (1)) {
				bTextured = 0;
				ogl.SetTexturing (false);
				}
			else {
				bmP->Texture ()->Wrap (GL_CLAMP);
				bTextured = 1;
				}
			}
		if (bTextured) {
			float z = (vFlame [0][0][Z] + vFlame [1][0][Z]) / 2.0f * ti.fLength;
			for (j = 0; j < 4; j++) {
				k = j * 4;
				verts [j] = vFlame [5][k];
				verts [j] [X] *= ti.fSize * 1.5f;
				verts [j] [Y] *= ti.fSize * 1.5f;
				verts [j] [Z] = z;
				}
			bmP->SetTexCoord (tcThruster);
			ogl.RenderQuad (bmP, verts, 3);
			bmpThruster [1][bPlayer]->Bind (1);
			}
		else {
			if (ogl.SizeVertexBuffer (RING_SEGS)) {
				for (i = 0; i < THRUSTER_SEGS - 1; i++) {
					color [0] = color [1];
					color [1].red *= 0.975f;
					color [1].green *= 0.8f;
					color [1].alpha *= fFade [i / 4];
					glColor4fv (reinterpret_cast<GLfloat*> (color));
					for (j = 0; j < RING_SEGS; j++)
						ogl.VertexBuffer () [j] = vFlame [0][nStripIdx [j]];
					ogl.FlushBuffers (GL_TRIANGLE_STRIP, RING_SEGS);
					}
				}
			}
		if (bTextured) {
			float c = 1; //0.8f + 0.02f * fPulse;
			glColor3f (c, c, c); //, 0.9f);
			}
		else {
			color [1].red = 0.5f + 0.05f * fPulse;
			color [1].green = 0.45f + 0.045f * fPulse;
			color [1].blue = 0.0f;
			color [1].alpha = 0.9f;
			}
#if 1
		if (ogl.SizeBuffers ((THRUSTER_SEGS - 1) * (RING_SEGS + 1) * 2)) {
			int nVerts = 0;
			for (i = 0; i < THRUSTER_SEGS - 1; i++) {
				if (!bTextured) {
					color [0] = color [1];
					color [1].red *= 0.975f;
					color [1].green *= 0.8f;
					color [1].alpha *= fFade [i / 4];
					}
				for (j = 0; j <= RING_SEGS; j++) {
					k = j % RING_SEGS;
					tTexCoord2fl.v.u = j * tTexCoord2flStep.v.u;
					for (l = 0; l < 2; l++) {
						v = vFlame [i + l][k];
						v [X] *= ti.fSize;
						v [Y] *= ti.fSize;
						v [Z] *= ti.fLength;
						//transformation.Transform (v, v, 0);
						if (bTextured) {
							tTexCoord2fl.v.v = 0.1f + tTexCoord2flStep.v.v * (i + l);
							ogl.TexCoordBuffer () [nVerts] = tTexCoord2fl;
							}
						else
							ogl.ColorBuffer () [nVerts] = color [l];
						ogl.VertexBuffer () [nVerts++] = v;
						}
					}
				ogl.FlushBuffers (GL_QUAD_STRIP, nVerts, bTextured, !bTextured);
				}
			}
#endif
		transformation.End ();
		}
	ogl.SetTransform (0);
	ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	ogl.SetFaceCulling (true);
	OglCullFace (0);
	ogl.SetDepthWrite (true);
	}
ogl.StencilOn (bStencil);
}

// -----------------------------------------------------------------------------

void RenderLaserCorona (CObject *objP, tRgbaColorf *colorP, float alpha, float fScale)
{
	int	bAdditive = 1; //gameOpts->render.bAdditive

if (!SHOW_OBJ_FX)
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;
if (gameOpts->render.coronas.bShots && (bAdditive ? LoadGlare () : LoadCorona ())) {
	tHitbox*			phb = &gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes [0];
	float				fLength = X2F (phb->vMax [Z] - phb->vMin [Z]) / 2;
	CBitmap*			bmP;
	tRgbaColorf		color;

	static CFloatVector	vEye = CFloatVector::ZERO;

	bmP = bAdditive ? bmpGlare : bmpCorona;
	colorP->alpha = alpha;
	if (bAdditive) {
		float fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
		color = *colorP;
		colorP = &color;
		color.red *= fScale;
		color.green *= fScale;
		color.blue *= fScale;
		}
	bmP->SetColor (colorP);
	ogl.RenderSprite (bmP, objP->info.position.vPos + objP->info.position.mOrient.FVec () * (F2X (fLength - 0.5f)), I2X (1), I2X (1), alpha, bAdditive, 1);
	}
}

// -----------------------------------------------------------------------------

static inline float WeaponBlobSize (int nId)
{
if (nId == PHOENIX_ID)
	return 2.25f;
else if (nId == PLASMA_ID)
	return 2.25f;
else if (nId == HELIX_ID)
	return 1.25f;
else if (nId == SPREADFIRE_ID)
	return 1.25f;
else if (nId == OMEGA_ID)
	return 1.5f;
else if (SMARTMSL_BLOB_ID)
	return 2.25f;
else if (ROBOT_SMARTMSL_BLOB_ID)
	return 2.25f;
else if (SMARTMINE_BLOB_ID)
	return 2.25f;
else if (ROBOT_SMARTMINE_BLOB_ID)
	return 2.25f;
else
	return 1.0f;
}

// -----------------------------------------------------------------------------

int RenderWeaponCorona (CObject *objP, tRgbaColorf *colorP, float alpha, fix xOffset,
								float fScale, int bSimple, int bViewerOffset, int bDepthSort)
{
if (!SHOW_OBJ_FX)
	return 0;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return 0;
if ((objP->info.nType == OBJ_WEAPON) && (objP->info.renderType == RT_POLYOBJ))
	RenderLaserCorona (objP, colorP, alpha, fScale);
else if (gameOpts->render.coronas.bShots && LoadCorona ()) {
	int			bStencil;
	fix			xSize;
	tRgbaColorf	color;

	static tTexCoord2f	tcCorona [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};

	CFixVector	vPos = objP->info.position.vPos;
	xSize = (fix) (WeaponBlobSize (objP->info.nId) * F2X (fScale));
	if (xOffset) {
		if (bViewerOffset) {
			CFixVector o = gameData.render.mine.viewerEye - vPos;
			CFixVector::Normalize (o);
			vPos += o * xOffset;
			}
		else
			vPos += objP->info.position.mOrient.FVec () * xOffset;
		}
	if (xSize < I2X (1))
		xSize = I2X (1);
	color.alpha = alpha;
	alpha = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
	color.red = colorP->red * alpha;
	color.green = colorP->green * alpha;
	color.blue = colorP->blue * alpha;
#if 0
	color.red *= color.red;
	color.green *= color.green;
	color.blue *= color.blue;
#endif
	if (bDepthSort && bSimple)
		return transparencyRenderer.AddSprite (bmpCorona, vPos, &color, FixMulDiv (xSize, bmpCorona->Width (), bmpCorona->Height ()), xSize, 0, 1, 3);
	bStencil = ogl.StencilOff ();
	ogl.SetDepthWrite (false);
	ogl.SetBlendMode (GL_ONE, GL_ONE);
	if (bSimple) {
		bmpCorona->SetColor (&color);
		ogl.RenderSprite (bmpCorona, vPos, FixMulDiv (xSize, bmpCorona->Width (), bmpCorona->Height ()), xSize, alpha, 1, 3);
		}
	else {
		CFloatVector	quad [4], verts [8], vCenter, vNormal, v;
		float		dot;
		int		i, j;

		ogl.SetFaceCulling (false);
		ogl.SetDepthMode (GL_LEQUAL);
		ogl.SetDepthWrite (false);
		ogl.SetTexturing (true);
		bmpCorona->SetTranspType (-1);
		if (bmpCorona->Bind (1))
			return 0;
		bmpCorona->Texture ()->Wrap (GL_CLAMP);
		transformation.Begin (vPos, objP->info.position.mOrient);
		TransformHitboxf (objP, verts, 0);
		for (i = 0; i < 6; i++) {
			vCenter.SetZero ();
			for (j = 0; j < 4; j++) {
				quad [j] = verts [hitboxFaceVerts [i][j]];
				vCenter += quad [j];
				}
			vCenter = vCenter * 0.25f;
			vNormal = CFloatVector::Normal (quad [0], quad [1], quad [2]);
			v = vCenter; 
			CFloatVector::Normalize (v);
			dot = CFloatVector::Dot (vNormal, v);
			if (dot >= 0)
				continue;
			glColor4f (colorP->red, colorP->green, colorP->blue, alpha * (float) sqrt (-dot));
			for (j = 0; j < 4; j++) {
				v = quad [j] - vCenter;
				quad [j] += v * fScale;
				}
			bmpCorona->SetTexCoord (tcCorona);
			ogl.RenderQuad (bmpCorona, quad, 3);
			}
		transformation.End ();
		ogl.SetDepthMode (GL_LESS);
		ogl.SetTexturing (false);
		ogl.SetFaceCulling (true);
		}
	ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	ogl.SetDepthWrite (true);
	ogl.StencilOn (bStencil);
	}
return 0;
}

// -----------------------------------------------------------------------------

//extern CAngleVector CAngleVector::ZERO;

void RenderShockwave (CObject *objP)
{
if (!SHOW_OBJ_FX)
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;
if ((objP->info.nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [objP->info.nId]) {
		CFixVector	vPos;
		int			bStencil;

	vPos = objP->info.position.vPos + objP->info.position.mOrient.FVec () * (objP->info.xSize / 2);
	bStencil = ogl.StencilOff ();
	if (EGI_FLAG (bShockwaves, 1, 1, 0) &&
		 (objP->mType.physInfo.velocity [X] || objP->mType.physInfo.velocity [Y] || objP->mType.physInfo.velocity [Z])) {
			CFloatVector	vPosf;
			int				h, i, j, k, n;
			float				r [4], l [4], alpha;
			tRgbaColorf		*colorP = gameData.weapons.color + objP->info.nId;

		if (ogl.SizeBuffers (2 * 3 * (RING_SEGS + 1))) {
			transformation.Begin (vPos, objP->info.position.mOrient);
			ogl.SetDepthWrite (false);
			ogl.SetTexturing (false);
			//OglCullFace (1);
			ogl.SetFaceCulling (false);
			r [3] = X2F (objP->info.xSize);
			if (r [3] >= 3.0f)
				r [3] /= 1.5f;
			else if (r [3] < 1)
				r [3] *= 2;
			else if (r [3] < 2)
				r [3] *= 1.5f;
			r [2] = r [3];
			r [1] = r [2] / 4.0f * 3.0f;
			r [0] = r [2] / 3;
			l [3] = (r [3] < 1.0f) ? 10.0f : 20.0f;
			l [2] = r [3] / 4;
			l [1] = -r [3] / 6;
			l [0] = -r [3] / 3;
			alpha = 0.15f;
			int nVerts = 0;
			tRgbaColorf	color = *colorP;
			for (h = 0; h < 3; h++) {
				for (i = 0; i <= RING_SEGS; i++) {
					j = i % RING_SEGS;
					for (k = 0; k < 2; k++) {
						n = h + k;
						color.alpha = (n == 3) ? 0.0f : alpha;
						ogl.ColorBuffer () [nVerts] = color;
						vPosf = vRingVerts [j];
						vPosf [X] *= r [n];
						vPosf [Y] *= r [n];
						vPosf [Z] = -l [n];
						transformation.Transform (ogl.VertexBuffer () [nVerts], vPosf, 0);
						nVerts++;
						}
					}
				}
			ogl.FlushBuffers (GL_QUAD_STRIP, nVerts, 0, 1);
			ogl.SetFaceCulling (true);
			nVerts = 0;
			for (h = 0; h < 3; h += 2) {
				ogl.SetCullMode (h ? GL_FRONT : GL_BACK);
				glColor4f (colorP->red, colorP->green, colorP->blue, h ? 0.1f : alpha);
				for (j = 0; j < RING_SEGS; j++) {
					vPosf = vRingVerts [nStripIdx [j]];
					vPosf [X] *= r [h];
					vPosf [Y] *= r [h];
					vPosf [Z] = -l [h];
					transformation.Transform (ogl.VertexBuffer () [nVerts], vPosf, 0);
					nVerts++;
					}
				ogl.FlushBuffers (GL_TRIANGLE_STRIP, nVerts, 0, 1);
				}
			ogl.SetDepthWrite (true);
			OglCullFace (0);
			transformation.End ();
			}
		}
	ogl.StencilOn (bStencil);
	}
}

// -----------------------------------------------------------------------------

#define TRACER_WIDTH	3

#if 0

void RenderTracers (CObject *objP)
{
if (!SHOW_OBJ_FX)
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;
if (EGI_FLAG (bTracers, 0, 1, 0) &&
	 (objP->info.nType == OBJ_WEAPON) && ((objP->info.nId == VULCAN_ID) || (objP->info.nId == GAUSS_ID))) {
		CFloatVector	vPosf [2], vDirf;
		short				i;
		int				bStencil;

	vPosf [0].Assign (objP->info.position.vPos);
	vPosf [1].Assign (objP->info.vLastPos);
	transformation.Transform (vPosf [0], vPosf [0], 0);
	transformation.Transform (vPosf [1], vPosf [1], 0);
	vDirf = vPosf [0] - vPosf [1];
	if (vDirf.IsZero ()) {
		//return;
		vPosf [1].Assign (OBJECTS [objP->cType.laserInfo.parent.nObject].info.position.vPos);
		transformation.Transform(vPosf [1], vPosf [1], 0);
		vDirf = vPosf [0] - vPosf [1];
		if(vDirf.IsZero ())
			return;
		}
	bStencil = ogl.StencilOff ();
	ogl.SetDepthWrite (false);
	glEnable (GL_LINE_STIPPLE);
	ogl.SetBlending (true);
	ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	ogl.SetLineSmooth (true);
	glLineStipple (6, 0x003F); //patterns [h]);
	vDirf *= TRACER_WIDTH / 20.0f;
	for (i = 1; i < 5; i++) {
		glLineWidth ((GLfloat) (TRACER_WIDTH * i));
		glBegin (GL_LINES);
		glColor4d (1, 1, 1, 0.5 / i);
		glVertex3fv (reinterpret_cast<GLfloat*> (vPosf + 1));
		glVertex3fv (reinterpret_cast<GLfloat*> (vPosf));
		glEnd ();
		}
	glLineWidth (1);
	glDisable (GL_LINE_STIPPLE);
	ogl.SetLineSmooth (false);
	ogl.SetDepthWrite (true);
	ogl.StencilOn (bStencil);
	}
}

#endif

// -----------------------------------------------------------------------------
// Draws a texture-mapped laser bolt
#if 0
void Laser_draw_one (int nObject, CBitmap * bmp)
{
	int t1, t2, t3;
	g3sPoint p1, p2;
	CObject *objP = OBJECTS + nObject;
	CFixVector start_pos,vEndPos;
	fix Laser_length = gameData.models.polyModels [0][objP->rType.polyObjInfo.nModel].rad * 2;
	fix Laser_width = Laser_length / 8;

	start_pos = objP->info.position.vPos;
	VmVecScaleAdd (&vEndPos,&start_pos,&objP->info.position.mOrient.FVec (),-Laser_length);

	G3TransformAndEncodePoint (&p1,&start_pos);
	G3TransformAndEncodePoint (&p2,&vEndPos);

	t1 = gameStates.render.nLighting;
	t2 = gameStates.render.nInterpolationMethod;
	t3 = gameStates.render.bTransparency;

	gameStates.render.nLighting  = 0;
	//gameStates.render.nInterpolationMethod = 3;	 //Full perspective
	gameStates.render.nInterpolationMethod = 1;	//Linear
	gameStates.render.bTransparency = 1;
#if 0
	CCanvas::Current ()->SetColor (gr_getcolor (31,15,0);
	g3_draw_line_ptrs (p1,p2);
	g3_draw_rod (p1,0x2000,p2,0x2000);
	g3_draw_rod (p1,Laser_width,p2,Laser_width);
#else
	G3DrawRodTexPoly (bmp,&p2,Laser_width,&p1,Laser_width,0,NULL);
#endif
	gameStates.render.nLighting = t1;
	gameStates.render.nInterpolationMethod = t2;
	gameStates.render.bTransparency = t3;
}
#endif

// -----------------------------------------------------------------------------

#if 0
static CFloatVector vTrailOffs [2][4] = {{{{0,0,0}},{{0,-10,-5}},{{0,-10,-50}},{{0,0,-50}}},
											 {{{0,0,0}},{{0,10,-5}},{{0,10,-50}},{{0,0,-50}}}};
#endif

void RenderLightTrail (CObject *objP)
{
	tRgbaColorf		color, *colorP;
	int				nTrailItem = -1, /*nCoronaItem = -1,*/ bGatling = 0, bAdditive = 1; //gameOpts->render.coronas.bAdditiveObjs;

if (!SHOW_OBJ_FX)
	return;
if (!gameData.objs.bIsWeapon [objP->info.nId])
	return;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
	return;

bGatling = (objP->info.nId == VULCAN_ID) || (objP->info.nId == GAUSS_ID);
if (objP->info.renderType == RT_POLYOBJ)
	colorP = gameData.weapons.color + objP->info.nId;
else {
	tRgbColorb	*pcb = VClipColor (objP);
	color.red = pcb->red / 255.0f;
	color.green = pcb->green / 255.0f;
	color.blue = pcb->blue / 255.0f;
	colorP = &color;
	}

#if 1
if (!gameData.objs.bIsSlowWeapon [objP->info.nId] && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && EGI_FLAG (bLightTrails, 0, 0, 0)) {
	if (gameOpts->render.particles.bPlasmaTrails)
		;//DoObjectSmoke (objP);
	else if (EGI_FLAG (bLightTrails, 1, 1, 0) && (objP->info.nType == OBJ_WEAPON) &&
				!gameData.objs.bIsSlowWeapon [objP->info.nId] &&
				(!objP->mType.physInfo.velocity.IsZero ()) &&
				(bAdditive ? LoadGlare () : LoadCorona ())) {
			CFloatVector	vNorm, vCenter, vOffs, vTrailVerts [8];
			float				h, l, r, dx, dy;
			CBitmap*			bmP;

			static CFloatVector vEye = CFloatVector::ZERO;

			static tRgbaColorf	trailColor = {0,0,0,0.33f};
			static tTexCoord2f	tTexCoordTrail [8] = {
				//{{0.1f,0.1f}},{{0.9f,0.1f}},{{0.9f,0.9f}},{{0.1f,0.9f}}
				{{0.0f,0.0f}},{{1.0f,0.0f}},{{1.0f,0.5f}},{{0.0f,0.5f}},
				{{0.0f,0.5f}},{{1.0f,0.5f}},{{1.0f,1.0f}},{{0.0f,1.0f}}
				};

		vCenter.Assign (objP->info.position.vPos);
		vOffs.Assign (objP->info.position.mOrient.FVec ());
		if (objP->info.renderType == RT_POLYOBJ) {
			tHitbox*	phb = &gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes [0];
			l = X2F (phb->vMax [Z] - phb->vMin [Z]);
			dx = X2F (phb->vMax [X] - phb->vMin [X]);
			dy = X2F (phb->vMax [Y] - phb->vMin [Y]);
			r = float (sqrt (dx * dx + dy * dy)) * ((objP->info.nId == FUSION_ID) ? 1.5f : 3.0f);
			vCenter += vOffs * (l / 2.0f);
			}
		else {
			r = WeaponBlobSize (objP->info.nId);
			l = r * 1.5f;
			r *= 2;
			}
		bmP = bAdditive ? bmpGlare : bmpCorona;
		memcpy (&trailColor, colorP, 3 * sizeof (float));
		if (bAdditive) {
			float fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
			trailColor.red *= fScale;
			trailColor.green *= fScale;
			trailColor.blue *= fScale;
			}
		vTrailVerts [0] = vCenter + vOffs * l;
		h = X2F (CFixVector::Dist (objP->info.position.vPos, objP->Origin ()));
		if (h > 50.0f)
			h = 50.0f;
		else if (h < 1.0f)
			h = 1.0f;
		vTrailVerts [7] = vTrailVerts [0] - vOffs * (h + l);
		transformation.Transform (vCenter, vCenter, 0);
		transformation.Transform (vTrailVerts [0], vTrailVerts [0], 0);
		transformation.Transform (vTrailVerts [7], vTrailVerts [7], 0);
		vNorm = CFloatVector::Normal (vTrailVerts [0], vTrailVerts [7], vEye);
		vNorm *= r;
		vTrailVerts [2] = 
		vTrailVerts [5] = vCenter + vNorm;
		vTrailVerts [3] = 
		vTrailVerts [4] = vCenter - vNorm;
		//vNorm /= 4;
		vTrailVerts [6] = vTrailVerts [7] + vNorm;
		vTrailVerts [7] -= vNorm;
		vNorm = CFloatVector::Normal (vTrailVerts [2], vTrailVerts [3], vEye);
		vNorm *= r;
		vTrailVerts [0] = vTrailVerts [3] - vNorm;
		vTrailVerts [1] = vTrailVerts [2] - vNorm;
#if 0 //DBG
		//trailColor.red = trailColor.green = trailColor.blue = 1.0;
		glColor3f (1,1,1);
		glLineWidth (2);
		ogl.EnableClientStates (0, 0, 0, GL_TEXTURE0);
		ogl.SetTexturing (false);
		ogl.SetBlendMode (1);
		OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), (GLfloat*) vTrailVerts);
		OglDrawArrays (GL_LINE_LOOP, 0, 4);
		OglDrawArrays (GL_LINE_LOOP, 4, 4);
		ogl.DisableClientStates (0, 0, 0, GL_TEXTURE0);
		glLineWidth (1);
#else
		transparencyRenderer.AddPoly (NULL, NULL, bmP, vTrailVerts, 8, tTexCoordTrail, &trailColor, NULL, 1, 0, GL_QUADS, GL_CLAMP, bAdditive, -1);
#endif
		}
	RenderShockwave (objP);
	}
#endif
#if 1
if ((objP->info.renderType != RT_POLYOBJ) || (objP->info.nId == FUSION_ID))
	RenderWeaponCorona (objP, colorP, 0.5f, 0, 2.0f + X2F (d_rand() % (I2X (1) / 8)), 1, 0, 1);
else
	RenderWeaponCorona (objP, colorP, 0.75f, 0, bGatling ? 1.0f : 2.0f, 0, 0, 0);
#endif
}

// -----------------------------------------------------------------------------

void DrawDebrisCorona (CObject *objP)
{
	static	tRgbaColorf	debrisGlow = {0.66f, 0, 0, 1};
	static	tRgbaColorf	markerGlow = {0, 0.66f, 0, 1};
	static	time_t t0 = 0;

if (objP->info.nType == OBJ_MARKER)
	RenderWeaponCorona (objP, &markerGlow, 0.75f, 0, 4, 1, 1, 0);
#if DBG
else if (objP->info.nType == OBJ_DEBRIS) {
#else
else if ((objP->info.nType == OBJ_DEBRIS) && gameOpts->render.nDebrisLife) {
#endif
	float	h = (float) nDebrisLife [gameOpts->render.nDebrisLife] - X2F (objP->info.xLifeLeft);
	if (h < 0)
		h = 0;
	if (h < 10) {
		h = (10 - h) / 20.0f;
		if (gameStates.app.nSDLTicks - t0 > 50) {
			t0 = gameStates.app.nSDLTicks;
			debrisGlow.red = 0.5f + X2F (d_rand () % (I2X (1) / 4));
			debrisGlow.green = X2F (d_rand () % (I2X (1) / 4));
			}
		RenderWeaponCorona (objP, &debrisGlow, h, 5 * objP->info.xSize, 1.5f, 1, 1, 0);
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
	vPos += info.position.mOrient.FVec () * FixMul (info.xSize, flashDist);
CObject* effectObjP = /*Object*/CreateExplosion (info.nSegment, vPos, info.xSize, VCLIP_PLAYER_APPEARANCE);
if (effectObjP) {
	effectObjP->info.position.mOrient = info.position.mOrient;
	if (gameData.eff.vClips [0][VCLIP_PLAYER_APPEARANCE].nSound > -1)
		audio.CreateObjectSound (gameData.eff.vClips [0][VCLIP_PLAYER_APPEARANCE].nSound, SOUNDCLASS_PLAYER, OBJ_IDX (effectObjP));
	}
}

//------------------------------------------------------------------------------
//eof
