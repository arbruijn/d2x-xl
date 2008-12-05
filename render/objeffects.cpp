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

#include "inferno.h"
#include "newdemo.h"
#include "network.h"
#include "interp.h"
#include "ogl_lib.h"
#include "render.h"
#include "transprender.h"
#include "glare.h"
#include "sphere.h"
#include "marker.h"
#include "fireball.h"
#include "objsmoke.h"
#include "objrender.h"
#include "objeffects.h"
#include "hiresmodels.h"

#ifndef fabsf
#	define fabsf(_f)	(float) fabs (_f)
#endif

#define IS_TRACK_GOAL(_objP)	(((_objP) == gameData.objs.trackGoals [0]) || ((_objP) == gameData.objs.trackGoals [1]))

// -----------------------------------------------------------------------------

void RenderObjectHalo (vmsVector *vPos, fix xSize, float red, float green, float blue, float alpha, int bCorona)
{
if ((gameOpts->render.coronas.bShots && (bCorona ? LoadCorona () : LoadHalo ()))) {
	tRgbaColorf	c = {red, green, blue, alpha};
	glDepthMask (0);
	G3DrawSprite(*vPos, xSize, xSize, bCorona ? bmpCorona : bmpHalo, &c, alpha * 4.0f / 3.0f, 1, 1);
	glDepthMask (1);
	}
}

// -----------------------------------------------------------------------------

void RenderPowerupCorona (tObject *objP, float red, float green, float blue, float alpha)
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
	int			bDepthSort;
	CBitmap		*bmP = bAdditive ? bmpGlare : bmpCorona;


	if ((objP->info.nId >= POW_KEY_BLUE) && (objP->info.nId <= POW_KEY_GOLD)) {
		int i = objP->info.nId - POW_KEY_BLUE;

		color = keyColors [(((i < 0) || (i > 2)) ? 3 : i)];
		xSize = 12 * F1_0;
		}
	else {
		float b = (float) sqrt ((red * 3 + green * 5 + blue * 2) / 10);
		color.red = red / b;
		color.green = green / b;
		color.blue = blue / b;
		xSize = 8 * F1_0;
		}
	color.alpha = alpha;
	if (bAdditive) {
		fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
		color.red *= fScale;
		color.green *= fScale;
		color.blue *= fScale;
		}
	bDepthSort = gameOpts->render.bDepthSort;
	gameOpts->render.bDepthSort = -1;
	G3DrawSprite (objP->info.position.vPos, xSize, xSize, bmP, &color, alpha, gameOpts->render.coronas.bAdditiveObjs, 5);
	gameOpts->render.bDepthSort = bDepthSort;
	}
}

//------------------------------------------------------------------------------

void TransformHitboxf (tObject *objP, fVector *vertList, int iSubObj)
{

	fVector		hv;
	tHitbox		*phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes + iSubObj;
	vmsVector	vMin = phb->vMin;
	vmsVector	vMax = phb->vMax;
	int			i;

for (i = 0; i < 8; i++) {
	hv [X] = X2F (hitBoxOffsets [i][X] ? vMin [X] : vMax [X]);
	hv [Y] = X2F (hitBoxOffsets [i][Y] ? vMin [Y] : vMax [Y]);
	hv [Z] = X2F (hitBoxOffsets [i][Z] ? vMin [Z] : vMax [Z]);
	G3TransformPoint (vertList [i], hv, 0);
	}
}

//------------------------------------------------------------------------------

#if RENDER_HITBOX

void RenderHitbox (tObject *objP, float red, float green, float blue, float alpha)
{
	fVector		vertList [8], v;
	tHitbox		*pmhb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
	tCloakInfo	ci = {0, FADE_LEVELS, 0, 0, 0, 0, 0};
	int			i, j, iBox, nBoxes, bHit = 0;
	float			fFade;

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
	if (!gameOpts->render.effects.bRobotShields)
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
	DrawShieldSphere (objP, red, green, blue, alpha);
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
glDepthFunc (GL_LEQUAL);
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDisable (GL_TEXTURE_2D);
glDepthMask (0);
glColor4f (red, green, blue, alpha / 2);
G3StartInstanceMatrix(objP->info.position.vPos, objP->info.position.mOrient);
for (; iBox <= nBoxes; iBox++) {
	if (iBox)
		G3StartInstanceAngles(pmhb [iBox].vOffset, &vmsAngVec::ZERO);
	TransformHitboxf (objP, vertList, iBox);
	glBegin (GL_QUADS);
	for (i = 0; i < 6; i++) {
		for (j = 0; j < 4; j++)
			glVertex3fv (reinterpret_cast<GLfloat*> (vertList + hitboxFaceVerts [i][j]));
		}
	glEnd ();
	glLineWidth (2);
	for (i = 0; i < 6; i++) {
		glBegin (GL_LINES);
		v.SetZero();
		for (j = 0; j < 4; j++) {
			glVertex3fv (reinterpret_cast<GLfloat*> (vertList + hitboxFaceVerts [i][j]));
			v += vertList [hitboxFaceVerts [i][j]];
			}
		glEnd ();
		}
	glLineWidth (1);
	if (iBox)
		G3DoneInstance ();
	}
G3DoneInstance ();
float r = X2F (VmVecDist (&pmhb->vMin, &pmhb->vMax) / 2);
#if 0//def _DEBUG	//display collision point
if (gameStates.app.nSDLTicks - gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].tHit < 500) {
	tObject	o;

	o.position.vPos = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].vHit;
	o.position.mOrient = objP->info.position.mOrient;
	o.size = F1_0 * 2;
	//SetRenderView (0, NULL);
	DrawShieldSphere (&o, 1, 0, 0, 0.33f);
	}
#endif
glDepthMask (1);
glDepthFunc (GL_LESS);
}

#endif

// -----------------------------------------------------------------------------

void RenderPlayerShield (tObject *objP)
{
	int			bStencil, dt = 0, i = objP->info.nId, nColor = 0;
	float			alpha, scale = 1;
	tCloakInfo	ci;

	static tRgbaColorf shieldColors [3] = {{0, 0.5f, 1, 1}, {1, 0.5f, 0, 1}, {1, 0.8f, 0.6f, 1}};

if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS &&
	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 1) : (gameStates.render.nShadowPass != 3)))
	return;
#endif
if (EGI_FLAG (bPlayerShield, 0, 1, 0)) {
	if (gameData.multiplayer.players [i].flags & PLAYER_FLAGS_CLOAKED) {
		if (!GetCloakInfo (objP, 0, 0, &ci))
			return;
		scale = (float) ci.nFadeValue / (float) FADE_LEVELS;
		scale *= scale;
		}
	bStencil = StencilOff ();
	UseSpherePulse (&gameData.render.shield, gameData.multiplayer.spherePulse + i);
	if (gameData.multiplayer.bWasHit [i]) {
		if (gameData.multiplayer.bWasHit [i] < 0) {
			gameData.multiplayer.bWasHit [i] = 1;
			gameData.multiplayer.nLastHitTime [i] = gameStates.app.nSDLTicks;
			SetSpherePulse (gameData.multiplayer.spherePulse + i, 0.1f, 0.5f);
			dt = 0;
			}
		else if ((dt = gameStates.app.nSDLTicks - gameData.multiplayer.nLastHitTime [i]) >= 300) {
			gameData.multiplayer.bWasHit [i] = 0;
			SetSpherePulse (gameData.multiplayer.spherePulse + i, 0.02f, 0.4f);
			}
		}
	if (gameOpts->render.effects.bOnlyShieldHits && !gameData.multiplayer.bWasHit [i])
		return;
	if (gameData.multiplayer.players [i].flags & PLAYER_FLAGS_INVULNERABLE)
		nColor = 2;
	else if (gameData.multiplayer.bWasHit [i])
		nColor = 1;
	else
		nColor = 0;
	if (gameData.multiplayer.bWasHit [i]) {
		alpha = (gameOpts->render.effects.bOnlyShieldHits ? (float) cos (sqrt ((double) dt / 300.0) * Pi / 2) : 1);
		scale *= alpha;
		}
	else if (gameData.multiplayer.players [i].flags & PLAYER_FLAGS_INVULNERABLE)
		alpha = 1;
	else {
		alpha = X2F (gameData.multiplayer.players [i].shields) / 100.0f;
		scale *= alpha;
		if (gameData.multiplayer.spherePulse [i].fSpeed == 0.0f)
			SetSpherePulse (gameData.multiplayer.spherePulse + i, 0.02f, 0.5f);
		}
#if RENDER_HITBOX
	RenderHitbox (objP, shieldColors [nColor].red * scale, shieldColors [nColor].green * scale, shieldColors [nColor].blue * scale, alpha);
#else
	DrawShieldSphere (objP, shieldColors [nColor].red * scale, shieldColors [nColor].green * scale, shieldColors [nColor].blue * scale, alpha);
#endif
	StencilOn (bStencil);
	}
}

// -----------------------------------------------------------------------------

void RenderRobotShield (tObject *objP)
{
	static tRgbaColorf shieldColors [3] = {{0.75f, 0, 0.75f, 1}, {0, 0.5f, 1},{1, 0.5f, 0, 1}};

#if RENDER_HITBOX
RenderHitbox (objP, 0.5f, 0.0f, 0.6f, 0.4f);
#else
	float			scale = 1;
	tCloakInfo	ci;
	fix			dt;

if (!gameOpts->render.effects.bRobotShields)
	return;
if ((objP->info.nType == OBJ_ROBOT) && objP->cType.aiInfo.CLOAKED) {
	if (!GetCloakInfo (objP, 0, 0, &ci))
		return;
	scale = (float) ci.nFadeValue / (float) FADE_LEVELS;
	scale *= scale;
	}
dt = gameStates.app.nSDLTicks - objP->xTimeLastHit;
if (dt < 300) {
	scale *= gameOpts->render.effects.bOnlyShieldHits ? (float) cos (sqrt ((double) dt / 300.0) * Pi / 2) : 1;
	DrawShieldSphere (objP, shieldColors [2].red * scale, shieldColors [2].green * scale, shieldColors [2].blue * scale, 0.5f * scale);
	}
else if (!gameOpts->render.effects.bOnlyShieldHits) {
	if ((objP->info.nType != OBJ_ROBOT) || ROBOTINFO (objP->info.nId).companion)
		DrawShieldSphere (objP, 0.0f, 0.5f * scale, 1.0f * scale, ObjectDamage (objP) / 2 * scale);
	else
		DrawShieldSphere (objP, 0.75f * scale, 0.0f, 0.75f * scale, ObjectDamage (objP) / 2 * scale);
	}
#endif
}

// -----------------------------------------------------------------------------

static inline tRgbColorf *ObjectFrameColor (tObject *objP, tRgbColorf *pc)
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

void RenderDamageIndicator (tObject *objP, tRgbColorf *pc)
{
	vmsVector	vPos;
	fVector		fPos, fVerts [4];
	float			r, r2, w;
	int			i, bStencil, bDrawArrays;

if (!SHOW_OBJ_FX)
	return;
if ((gameData.demo.nState == ND_STATE_PLAYBACK) && gameOpts->demo.bOldFormat)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (EGI_FLAG (bDamageIndicators, 0, 1, 0) &&
	 (extraGameInfo [IsMultiGame].bTargetIndicators < 2)) {
	bStencil = StencilOff ();
	pc = ObjectFrameColor (objP, pc);
	PolyObjPos (objP, &vPos);
	fPos = vPos.ToFloat();
	G3TransformPoint (fPos, fPos, 0);
	r = X2F (objP->info.xSize);
	r2 = r / 10;
	r = r2 * 9;
	w = 2 * r;
	fPos [X] -= r;
	fPos [Y] += r;
	w *= ObjectDamage (objP);
	fVerts [0][X] = fVerts [3][X] = fPos [X];
	fVerts [1][X] = fVerts [2][X] = fPos [X] + w;
	fVerts [0][Y] = fVerts [1][Y] = fPos [Y];
	fVerts [2][Y] = fVerts [3][Y] = fPos [Y] - r2;
	fVerts [0][Z] = fVerts [1][Z] = fVerts [2][Z] = fVerts [3][Z] = fPos [Z];
	fVerts [0][W] = fVerts [1][W] = fVerts [2][W] = fVerts [3][W] = 1;
	glColor4f (pc->red, pc->green, pc->blue, 2.0f / 3.0f);
	glDisable (GL_TEXTURE_2D);
#if 1
	if ((bDrawArrays = G3EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0))) {
		glVertexPointer (4, GL_FLOAT, 0, fVerts);
		glDrawArrays (GL_QUADS, 0, 4);
		}
	else {
		glBegin (GL_QUADS);
		for (i = 0; i < 4; i++)
			glVertex3fv (reinterpret_cast<GLfloat*> (fVerts + i));
		glEnd ();
		}
#else
	bDrawArrays = 0;
	glBegin (GL_QUADS);
	glVertex3f (fPos [X], fPos [Y], fPos [Z]);
	glVertex3f (fPos [X] + w, fPos [Y], fPos [Z]);
	glVertex3f (fPos [X] + w, fPos [Y] - r2, fPos [Z]);
	glVertex3f (fPos [X], fPos [Y] - r2, fPos [Z]);
	glEnd ();
#endif
	w = 2 * r;
	fVerts [1][X] = fVerts [2][X] = fPos [X] + w;
	glColor3fv (reinterpret_cast<GLfloat*> (pc));
	if (bDrawArrays) {
		glVertexPointer (4, GL_FLOAT, 0, fVerts);
		glDrawArrays (GL_LINE_LOOP, 0, 4);
		glDisableClientState (GL_VERTEX_ARRAY);
		}
	else {
		glBegin (GL_LINE_LOOP);
		for (i = 0; i < 4; i++)
			glVertex3fv (reinterpret_cast<GLfloat*> (fVerts + i));
		glEnd ();
		}
	StencilOn (bStencil);
	}
}

// -----------------------------------------------------------------------------

static tRgbaColorf	trackGoalColor [2] = {{1, 0.5f, 0, 0.8f}, {1, 0.5f, 0, 0.8f}};
static int				nMslLockColor [2] = {0, 0};
static int				nMslLockColorIncr [2] = {-1, -1};
static float			fMslLockGreen [2] = {0.65f, 0.0f};

void RenderMslLockIndicator (tObject *objP)
{
	#define INDICATOR_POSITIONS	60

	static tSinCosf	sinCosInd [INDICATOR_POSITIONS];
	static int			bInitSinCos = 1;
	static int			nMslLockIndPos [2] = {0, 0};
	static int			t0 [2] = {0, 0}, tDelay [2] = {25, 40};

	vmsVector			vPos;
	fVector				fPos, fVerts [3];
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
fPos = vPos.ToFloat();
G3TransformPoint (fPos, fPos, 0);
r = X2F (objP->info.xSize);
if (bMarker)
	r = 17 * r / 12;
r2 = r / 4;

glDisable (GL_CULL_FACE);
G3DisableClientStates (1, 1, 1, GL_TEXTURE0);
bVertexArrays = G3EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0);
glActiveTexture (GL_TEXTURE0);
glDisable (GL_TEXTURE_2D);
glColor4fv (reinterpret_cast<GLfloat*> (trackGoalColor + bMarker));
if (bMarker || gameOpts->render.cockpit.bRotateMslLockInd) {
	fVector	rotVerts [3];
	fMatrix	mRot;
	int		h, i, j;

	if (bInitSinCos) {
		OglComputeSinCos (sizeofa (sinCosInd), sinCosInd);
		bInitSinCos = 0;
		}
	mRot [RVEC][X] =
	mRot [UVEC][Y] = sinCosInd [nMslLockIndPos [bMarker]].fCos;
	mRot [UVEC][X] = sinCosInd [nMslLockIndPos [bMarker]].fSin;
	mRot [RVEC][Y] = -mRot [UVEC][X];
	mRot [RVEC][Z] =
	mRot [UVEC][Z] =
	mRot [FVEC][X] =
	mRot [FVEC][Y] = 0;
	mRot [FVEC][Z] = 1;

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
		glVertexPointer (3, GL_FLOAT, sizeof (fVector), rotVerts);
	for (j = 0; j < 4; j++) {
		for (i = 0; i < 3; i++) {
			rotVerts [i] = mRot * fVerts [i];
			fVerts [i] = rotVerts [i];
			rotVerts [i] += fPos;
			}
		if (bMarker)
			glLineWidth (2);
		if (bVertexArrays)
			glDrawArrays (bMarker ? GL_LINE_LOOP : GL_TRIANGLES, 0, 3);
		else {
			glBegin (bMarker ? GL_LINE_LOOP : GL_TRIANGLES);
			for (h = 0; h < 3; h++)
				glVertex3fv (reinterpret_cast<GLfloat*> (rotVerts + h));
			glEnd ();
			}
		if (bMarker)
			glLineWidth (1);
		if (!j) {	//now rotate by 90 degrees
			mRot [RVEC][X] =
			mRot [UVEC][Y] = 0;
			mRot [UVEC][X] = 1;
			mRot [RVEC][Y] = -1;
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
	glVertexPointer (4, GL_FLOAT, 0, fVerts);
	nTgtInd = extraGameInfo [IsMultiGame].bTargetIndicators;
	bHasDmg = !EGI_FLAG (bTagOnlyHitObjs, 0, 1, 0) | (ObjectDamage (objP) < 1);
	if (!nTgtInd ||
		 ((nTgtInd == 1) && (!EGI_FLAG (bDamageIndicators, 0, 1, 0) || !bHasDmg)) ||
		 ((nTgtInd == 2) && !bHasDmg)) {
		fVerts [0][Y] =
		fVerts [1][Y] = fPos [Y] + r;
		fVerts [2][Y] = fPos [Y] + r - r2;
		glDrawArrays (GL_TRIANGLES, 0, 3);
		}
	fVerts [0][Y] =
	fVerts [1][Y] = fPos [Y] - r;
	fVerts [2][Y] = fPos [Y] - r + r2;
	glDrawArrays (GL_TRIANGLES, 0, 3);
	fVerts [0][X] =
	fVerts [1][X] = fPos [X] + r;
	fVerts [2][X] = fPos [X] + r - r2;
	fVerts [0][Y] = fPos [Y] + r2;
	fVerts [1][Y] = fPos [Y] - r2;
	fVerts [2][Y] = fPos [Y];
	glDrawArrays (GL_TRIANGLES, 0, 3);
	fVerts [0][X] =
	fVerts [1][X] = fPos [X] - r;
	fVerts [2][X] = fPos [X] - r + r2;
	glDrawArrays (GL_TRIANGLES, 0, 3);
	}
glDisableClientState (GL_VERTEX_ARRAY);
glEnable (GL_CULL_FACE);
}

// -----------------------------------------------------------------------------

void RenderTargetIndicator (tObject *objP, tRgbColorf *pc)
{
	vmsVector	vPos;
	fVector		fPos, fVerts [4];
	float			r, r2, r3;
	int			i, bStencil, bDrawArrays, nPlayer = (objP->info.nType == OBJ_PLAYER) ? objP->info.nId : -1;

if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
#if 0
if (!CanSeeObject (OBJ_IDX (objP), 1))
	return;
#endif
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
if (EGI_FLAG (bTagOnlyHitObjs, 0, 1, 0) && (ObjectDamage (objP) >= 1.0f))
	return;
if (EGI_FLAG (bTargetIndicators, 0, 1, 0)) {
	bStencil = StencilOff ();
	glDisable (GL_TEXTURE_2D);
	pc = (EGI_FLAG (bMslLockIndicators, 0, 1, 0) && IS_TRACK_GOAL (objP) &&
			!gameOpts->render.cockpit.bRotateMslLockInd && (extraGameInfo [IsMultiGame].bTargetIndicators != 1)) ?
		  reinterpret_cast<tRgbColorf*> (&trackGoalColor [0]) : ObjectFrameColor (objP, pc);
	PolyObjPos (objP, &vPos);
	fPos = vPos.ToFloat();
	G3TransformPoint (fPos, fPos, 0);
	r = X2F (objP->info.xSize);
	glColor3fv (reinterpret_cast<GLfloat*> (pc));
	fVerts [0][W] = fVerts [1][W] = fVerts [2][W] = fVerts [3][W] = 1;
	glVertexPointer (4, GL_FLOAT, 0, fVerts);
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
		if ((bDrawArrays = G3EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0)))
			glDrawArrays (GL_LINE_STRIP, 0, 4);
		else {
			glBegin (GL_LINE_STRIP);
			for (i = 0; i < 4; i++)
				glVertex3fv (reinterpret_cast<GLfloat*> (fVerts + i));
			glEnd ();
			}
		fVerts [0][X] = fVerts [3][X] = fPos [X] + r2;
		fVerts [1][X] = fVerts [2][X] = fPos [X] + r;
		if (bDrawArrays) {
			glDrawArrays (GL_LINE_STRIP, 0, 4);
			glDisableClientState (GL_VERTEX_ARRAY);
			}
		else {
			glBegin (GL_LINE_STRIP);
			for (i = 0; i < 4; i++)
				glVertex3fv (reinterpret_cast<GLfloat*> (fVerts + i));
			glEnd ();
			}
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
		if ((bDrawArrays = G3EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0)))
			glDrawArrays (GL_LINE_LOOP, 0, 3);
		else {
			glBegin (GL_LINE_LOOP);
			glVertex3fv (reinterpret_cast<GLfloat*> (fVerts));
			glVertex3fv (reinterpret_cast<GLfloat*> (fVerts + 1));
			glVertex3fv (reinterpret_cast<GLfloat*> (fVerts + 2));
			glEnd ();
			}
		if (EGI_FLAG (bDamageIndicators, 0, 1, 0)) {
			r3 = ObjectDamage (objP);
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
			glDrawArrays (GL_TRIANGLES, 0, 3);
			glDisableClientState (GL_VERTEX_ARRAY);
			}
		else {
			glBegin (GL_TRIANGLES);
			for (i = 0; i < 3; i++)
			glVertex3fv (reinterpret_cast<GLfloat*> (fVerts + i));
			glEnd ();
			}
		}
	StencilOn (bStencil);
	}
RenderDamageIndicator (objP, pc);
}

// -----------------------------------------------------------------------------

void RenderTowedFlag (tObject *objP)
{
	static fVector fVerts [4] = {
		fVector::Create(0.0f, 2.0f / 3.0f, 0.0f, 1.0f),
		fVector::Create(0.0f, 2.0f / 3.0f, -1.0f, 1.0f),
		fVector::Create(0.0f, -(1.0f / 3.0f), -1.0f, 1.0f),
		fVector::Create(0.0f, -(1.0f / 3.0f), 0.0f, 1.0f)
	};

	static tTexCoord2f texCoordList [4] = {{{0.0f, -0.3f}}, {{1.0f, -0.3f}}, {{1.0f, 0.7f}}, {{0.0f, 0.7f}}};

if (gameStates.app.bNostalgia)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (IsTeamGame && (gameData.multiplayer.players [objP->info.nId].flags & PLAYER_FLAGS_FLAG)) {
		vmsVector		vPos = objP->info.position.vPos;
		fVector			vPosf;
		tFlagData		*pf = gameData.pig.flags + !GetTeam (objP->info.nId);
		tPathPoint		*pp = GetPathPoint (&pf->path);
		int				i, bStencil;
		float				r;
		CBitmap		*bmP;

	if (pp) {
		bStencil = StencilOff ();
		OglActiveTexture (GL_TEXTURE0, 0);
		glEnable (GL_TEXTURE_2D);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		PIGGY_PAGE_IN (pf->bmi.index, 0);
		bmP = gameData.pig.tex.pBitmaps + pf->vcP->frames [pf->vci.nCurFrame].index;
		if (bmP->Bind (1, 2))
			return;
		bmP = bmP->CurFrame (-1);
		bmP->Texture ()->Wrap (GL_REPEAT);
		vPos += objP->info.position.mOrient [FVEC] * (-objP->info.xSize);
		r = X2F (objP->info.xSize);
		G3StartInstanceMatrix (vPos, pp->mOrient);
		glBegin (GL_QUADS);
		glColor3f (1.0f, 1.0f, 1.0f);
		for (i = 0; i < 4; i++) {
			vPosf [X] = 0;
			vPosf [Y] = fVerts [i][Y] * r;
			vPosf [Z] = fVerts [i][Z] * r;
			G3TransformPoint (vPosf, vPosf, 0);
			glTexCoord2fv (reinterpret_cast<GLfloat*> (texCoordList + i));
			glVertex3fv (reinterpret_cast<GLfloat*> (&vPosf));
			}
		for (i = 3; i >= 0; i--) {
			vPosf [X] = 0;
			vPosf [Y] = fVerts [i][Y] * r;
			vPosf [Z] = fVerts [i][Z] * r;
			G3TransformPoint (vPosf, vPosf, 0);
			glTexCoord2fv (reinterpret_cast<GLfloat*> (texCoordList + i));
			glVertex3fv (reinterpret_cast<GLfloat*> (&vPosf));
			}
		glEnd ();
		G3DoneInstance ();
		OGL_BINDTEX (0);
		StencilOn (bStencil);
		}
	}
}

// -----------------------------------------------------------------------------

#define	RING_SIZE		16
#define	THRUSTER_SEGS	14

static fVector	vFlame [THRUSTER_SEGS][RING_SIZE];
static int			bHaveFlame = 0;

static fVector	vRing [RING_SIZE] = {
	fVector::Create(-0.5f, -0.5f, 0.0f, 1.0f),
	fVector::Create(-0.6533f, -0.2706f, 0.0f, 1.0f),
	fVector::Create(-0.7071f, 0.0f, 0.0f, 1.0f),
	fVector::Create(-0.6533f, 0.2706f, 0.0f, 1.0f),
	fVector::Create(-0.5f, 0.5f, 0.0f, 1.0f),
	fVector::Create(-0.2706f, 0.6533f, 0.0f, 1.0f),
	fVector::Create(0.0f, 0.7071f, 0.0f, 1.0f),
	fVector::Create(0.2706f, 0.6533f, 0.0f, 1.0f),
	fVector::Create(0.5f, 0.5f, 0.0f, 1.0f),
	fVector::Create(0.6533f, 0.2706f, 0.0f, 1.0f),
	fVector::Create(0.7071f, 0.0f, 0.0f, 1.0f),
	fVector::Create(0.6533f, -0.2706f, 0.0f, 1.0f),
	fVector::Create(0.5f, -0.5f, 0.0f, 1.0f),
	fVector::Create(0.2706f, -0.6533f, 0.0f, 1.0f),
	fVector::Create(0.0f, -0.7071f, 0.0f, 1.0f),
	fVector::Create(-0.2706f, -0.6533f, 0.0f, 1.0f)
};

static int		nStripIdx [] = {0,15,1,14,2,13,3,12,4,11,5,10,6,9,7,8};

void CreateThrusterFlame (void)
{
if (!bHaveFlame) {
		fVector		*pv;
		int			i, j, m, n;
		double		phi, sinPhi;
		float			z = 0,
						fScale = 2.0f / 3.0f,
						fStep [2] = {1.0f / 4.0f, 1.0f / 3.0f};

	pv = &vFlame [0][0];
	for (i = 0, phi = 0; i < 5; i++, phi += Pi / 8, z -= fStep [0]) {
		sinPhi = (1 + sin (phi) / 2) * fScale;
		for (j = 0; j < RING_SIZE; j++, pv++) {
/*
			pv->x() = vRing [j][X] * (float) sinPhi;
			pv->y() = vRing [j][Y] * (float) sinPhi;
*/
			*pv = vRing [j] * (float)sinPhi;
			(*pv) [Z] = z;
			}
		}
	m = n = THRUSTER_SEGS - i + 1;
	for (phi = Pi / 2; i < THRUSTER_SEGS; i++, phi += Pi / 8, z -= fStep [1], m--) {
		sinPhi = (1 + sin (phi) / 2) * fScale * m / n;
		for (j = 0; j < RING_SIZE; j++, pv++) {
/*
			pv->x() = vRing [j][X] * (float) sinPhi;
			pv->y() = vRing [j][Y] * (float) sinPhi;
*/
			*pv = vRing [j] * (float)sinPhi;
			(*pv) [Z] = z;
			}
		}
	bHaveFlame = 1;
	}
}

// -----------------------------------------------------------------------------

void CalcShipThrusterPos (tObject *objP, vmsVector *vPos)
{
	tTransformation	*pPos = OBJPOS (objP);

if (gameOpts->render.bHiresModels) {
	vPos [0] = pPos->vPos + pPos->mOrient [FVEC] * (-objP->info.xSize);
	vPos [0] += pPos->mOrient [RVEC] * (-(8 * objP->info.xSize / 44));
	vPos [1] = vPos [0] + pPos->mOrient [RVEC] * (8 * objP->info.xSize / 22);
	}
else {
	vPos [0] = pPos->vPos + pPos->mOrient [FVEC] * (-objP->info.xSize / 10 * 9);
	if (gameStates.app.bFixModels)
		vPos [0] += pPos->mOrient [UVEC] * (objP->info.xSize / 40);
	else
		vPos [0] += pPos->mOrient [UVEC] * (-objP->info.xSize / 20);
	vPos [1] = vPos [0];
	vPos [0] += pPos->mOrient [RVEC] * (-8 * objP->info.xSize / 49);
	vPos [1] += pPos->mOrient [RVEC] * (8 * objP->info.xSize / 49);
	}
}

// -----------------------------------------------------------------------------

int CalcThrusterPos (tObject *objP, tThrusterInfo *tiP, int bAfterburnerBlob)
{
	tThrusterInfo		ti;
	tThrusterData		*pt = NULL;
	int					i, nThrusters,
							bMissile = IS_MISSILE (objP),
							bSpectate = SPECTATOR (objP);

ti = *tiP;
ti.pp = NULL;
ti.mtP = gameData.models.thrusters + objP->rType.polyObjInfo.nModel;
nThrusters = ti.mtP->nCount;
if (gameOpts->render.bHiresModels && (objP->info.nType == OBJ_PLAYER) && !ASEModel (objP->rType.polyObjInfo.nModel)) {
	if (!bSpectate) {
		pt = gameData.render.thrusters + objP->info.nId;
		ti.pp = GetPathPoint (&pt->path);
		}
	ti.fSize = (ti.fLength + 1) / 2;
	nThrusters = 2;
	CalcShipThrusterPos (objP, ti.vPos);
	ti.mtP = NULL;
	}
else if (bAfterburnerBlob || (bMissile && !nThrusters)) {
		tHitbox	*phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
		fix		nObjRad = RENDERPATH ? (phb->vMax [Z] - phb->vMin [Z]) / 2 : 2 * (phb->vMax [Z] - phb->vMin [Z]) / 3;

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
	*ti.vPos = objP->info.position.vPos + objP->info.position.mOrient [FVEC] * (-nObjRad);
	ti.mtP = NULL;
	}
else if ((objP->info.nType == OBJ_PLAYER) ||
			((objP->info.nType == OBJ_ROBOT) && !objP->cType.aiInfo.CLOAKED) ||
			bMissile) {
	vmsMatrix	m, *viewP;
	if (!bSpectate && (objP->info.nType == OBJ_PLAYER)) {
		pt = gameData.render.thrusters + objP->info.nId;
		ti.pp = GetPathPoint (&pt->path);
		}
	if (!nThrusters) {
		if (objP->info.nType != OBJ_PLAYER)
			return 0;
		if (!bSpectate) {
			pt = gameData.render.thrusters + objP->info.nId;
			ti.pp = GetPathPoint (&pt->path);
			}
		ti.fSize = (ti.fLength + 1) / 2;
		nThrusters = 2;
		CalcShipThrusterPos (objP, ti.vPos);
		}
	else {
		tTransformation *posP = OBJPOS (objP);
		if (SPECTATOR (objP)) {
			viewP = &m;
			m = posP->mOrient.Transpose();
		}
		else
			viewP = ObjectView (objP);
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

void CreateLightTrail (vmsVector& vPos, vmsVector &vDir, float fSize, float fLength, CBitmap *bmP, tRgbaColorf *colorP)
{
	static tTexCoord2f	tcCorona [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
	static tTexCoord2f	tcTrail [3] = {{{0,0}},{{1,1}},{{1,0}}};
	static fVector	vEye = fVector::ZERO;

	fVector	v, vPosf, vDirf, vNormf, vTrail [3], vCorona [4], fVecf;
	float		c = 1/*0.7f + 0.03f * fPulse*/, dotTrail, dotCorona;
	int		i;

fVecf = vDir.ToFloat();
vPosf = vPos.ToFloat();
vTrail [2] = vPosf - fVecf * fLength;
G3TransformPoint (vTrail [2], vTrail [2], 0);
G3TransformPoint (vPosf, vPosf, 0);
vNormf = fVector::Normal (vTrail [2], vPosf, vEye);
vTrail [0] = vPosf + vNormf * fSize;
vTrail [1] = vPosf - vNormf * fSize;
vNormf = fVector::Normal (vTrail [0], vTrail [1], vTrail [2]);
vCorona [0] = vTrail [0];
vCorona [2] = vTrail [1];
vCorona [1] = vPosf + vNormf * fSize;
vCorona [3] = vPosf + vNormf * (-fSize);
fVector::Normalize (vPosf);
v = vTrail [2]; 
fVector::Normalize (v);
dotTrail = fVector::Dot (vPosf, v);
v = *vCorona; 
fVector::Normalize (v);
dotCorona = fVector::Dot (vPosf, v);
if (gameOpts->render.bDepthSort > 0)
	TIAddLightTrail (bmP, vCorona, tcCorona, (dotTrail < dotCorona) ? vTrail : NULL, tcTrail, colorP);
else {
	glDisable (GL_CULL_FACE);
	glColor3f (c, c, c);
	if (dotTrail < dotCorona) {
		glBegin (GL_TRIANGLES);
		for (i = 0; i < 3; i++) {
			glTexCoord2fv (reinterpret_cast<GLfloat*> (tcTrail + i));
			glVertex3fv (reinterpret_cast<GLfloat*> (vTrail + i));
			}
		glEnd ();
		}
	glBegin (GL_QUADS);
	for (i = 0; i < 4; i++) {
		glTexCoord2fv (reinterpret_cast<GLfloat*> (tcCorona + i));
		glVertex3fv (reinterpret_cast<GLfloat*> (vCorona + i));
		}
	glEnd ();
	glEnable (GL_CULL_FACE);
	}
}

// -----------------------------------------------------------------------------

void RenderThrusterFlames (tObject *objP)
{
	int					h, i, j, k, l, nStyle, nThrusters, bStencil, bSpectate, bTextured;
	tRgbaColorf			c [2];
	tThrusterInfo		ti;
	fVector				v;
	float					fSpeed, fPulse, fFade [4];
	tThrusterData		*pt = NULL;

	static time_t		tPulse = 0;
	static int			nPulse = 10;

if (gameStates.app.bNostalgia)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
#if 1//ndef _DEBUG
if (!EGI_FLAG (bThrusterFlames, 1, 1, 0))
	return;
#endif
if ((objP->info.nType == OBJ_PLAYER) && (gameData.multiplayer.players [objP->info.nId].flags & PLAYER_FLAGS_CLOAKED))
	return;
fSpeed = X2F (objP->mType.physInfo.velocity.Mag());
ti.fLength = fSpeed / 60.0f + 0.5f + (float) (rand () % 100) / 1000.0f;
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
fPulse = (float) nPulse / 10.0f;
ti.pp = NULL;
nThrusters = CalcThrusterPos (objP, &ti, 0);
bStencil = StencilOff ();
//glDepthMask (0);
bTextured = 0;
nStyle = EGI_FLAG (bThrusterFlames, 1, 1, 0) == 2;
if (!LoadThruster ()) {
	extraGameInfo [IsMultiGame].bThrusterFlames = 2;
	glDisable (GL_TEXTURE_2D);
	}
else if (gameOpts->render.bDepthSort <= 0) {
	glEnable (GL_TEXTURE_2D);
	if (bmpThruster [nStyle]->Bind (1, -1)) {
		extraGameInfo [IsMultiGame].bThrusterFlames = 2;
		glDisable (GL_TEXTURE_2D);
		}
	else {
		bmpThruster [nStyle]->Texture ()->Wrap (GL_CLAMP);
		bTextured = 1;
		}
	}
if (nThrusters > 1) {
	vmsVector vRot [2];
	for (i = 0; i < 2; i++)
		G3RotatePoint (vRot [i], ti.vPos [i], 0);
	if (vRot [0][Z] < vRot [1][Z]) {
		vmsVector v = ti.vPos [0];
		ti.vPos [0] = ti.vPos [1];
		ti.vPos [1] = v;
		if (objP->info.nType == OBJ_ROBOT) {
			v = ti.vDir [0];
			ti.vDir [0] = ti.vDir [1];
			ti.vDir [1] = v;
			}
		}
	}
glEnable (GL_BLEND);
if (EGI_FLAG (bThrusterFlames, 1, 1, 0) == 1) {
		static tTexCoord2f	tcThruster [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
		static tTexCoord2f	tcFlame [3] = {{{0,0}},{{1,1}},{{1,0}}};
		static tRgbaColorf	tcColor = {0.75f, 0.75f, 0.75f, 1.0f};
		static fVector	vEye = fVector::ZERO;

		fVector	/*vPosf, vNormf, vFlame [3], vThruster [4],*/ fVecf;
		float		c = 1/*0.7f + 0.03f * fPulse, dotFlame, dotThruster*/;

	if (!gameData.models.vScale.IsZero ())
		ti.fSize *= X2F (gameData.models.vScale [Z]);
	ti.fLength *= 4 * ti.fSize;
	ti.fSize *= ((objP->info.nType == OBJ_PLAYER) && HaveHiresModel (objP->rType.polyObjInfo.nModel)) ? 1.2f : 1.5f;
#if 1
	if (!ti.mtP)
		fVecf = ti.pp ? ti.pp->mOrient [FVEC].ToFloat() : objP->info.position.mOrient [FVEC].ToFloat();
#endif
	for (h = 0; h < nThrusters; h++)
		CreateLightTrail (ti.vPos [h], ti.vDir [h], ti.fSize, ti.fLength, bmpThruster [nStyle], &tcColor);
	}
else {
	tTexCoord3f	tTexCoord2fl, tTexCoord2flStep;

	CreateThrusterFlame ();
	glLineWidth (3);
	OglCullFace (1);
	tTexCoord2flStep.v.u = 1.0f / RING_SIZE;
	tTexCoord2flStep.v.v = 0.5f / THRUSTER_SEGS;
	for (h = 0; h < nThrusters; h++) {
		if (bTextured) {
			float c = 1; //0.8f + 0.02f * fPulse;
			glColor3f (c, c, c); //, 0.9f);
			}
		else
			{
			c [1].red = 0.5f + 0.05f * fPulse;
			c [1].green = 0.45f + 0.045f * fPulse;
			c [1].blue = 0.0f;
			c [1].alpha = 0.9f;
			}
		G3StartInstanceMatrix (ti.vPos [h], (ti.pp && !bSpectate) ? ti.pp->mOrient : objP->info.position.mOrient);
		for (i = 0; i < THRUSTER_SEGS - 1; i++) {
#if 1
			if (!bTextured) {
				c [0] = c [1];
				c [1].red *= 0.975f;
				c [1].green *= 0.8f;
				c [1].alpha *= fFade [i / 4];
				}
			glBegin (GL_QUAD_STRIP);
			for (j = 0; j < RING_SIZE + 1; j++) {
				k = j % RING_SIZE;
				tTexCoord2fl.v.u = j * tTexCoord2flStep.v.u;
				for (l = 0; l < 2; l++) {
					v = vFlame [i + l][k];
					v [X] *= ti.fSize;
					v [Y] *= ti.fSize;
					v [Z] *= ti.fLength;
					G3TransformPoint(v, v, 0);
					if (bTextured) {
						tTexCoord2fl.v.v = 0.25f + tTexCoord2flStep.v.v * (i + l);
						glTexCoord2fv (reinterpret_cast<GLfloat*> (&tTexCoord2fl));
						}
					else
						glColor4fv (reinterpret_cast<GLfloat*> (c + l)); // (c [l].red, c [l].green, c [l].blue, c [l].alpha);
					glVertex3fv (reinterpret_cast<GLfloat*> (&v));
					}
				}
			glEnd ();
#else
			glBegin (GL_LINE_LOOP);
			glColor4f (c [1].red, c [1].green, c [1].blue, c [1].alpha);
			for (j = 0; j < RING_SIZE; j++) {
				G3TransformPoint (&v, vFlame [i] + j);
				glVertex3fv (reinterpret_cast<GLfloat*> (&v));
				}
			glEnd ();
#endif
			}
		glBegin (GL_TRIANGLE_STRIP);
		for (j = 0; j < RING_SIZE; j++) {
			G3TransformPoint(v, vFlame [0][nStripIdx [j]], 0);
			glVertex3fv (reinterpret_cast<GLfloat*> (&v));
			}
		glEnd ();
		G3DoneInstance ();
		}
	glLineWidth (1);
	OglCullFace (0);
	}
glDepthMask (1);
StencilOn (bStencil);
}

// -----------------------------------------------------------------------------

void RenderLaserCorona (tObject *objP, tRgbaColorf *colorP, float alpha, float fScale)
{
	int	bAdditive = 1; //gameOpts->render.bAdditive
if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (gameOpts->render.coronas.bShots && (bAdditive ? LoadGlare () : LoadCorona ())) {
	int			bStencil, bDrawArrays, i;
	float			a1, a2;
	fVector		vCorona [4], vh [5], vPos, vNorm, vDir;
	tHitbox		*phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
	float			fLength = X2F (phb->vMax [Z] - phb->vMin [Z]) / 2;
	float			dx = X2F (phb->vMax [X] - phb->vMin [X]);
	float			dy = X2F (phb->vMax [Y] - phb->vMin [Y]);
	float			fRad = (float) (sqrt (dx * dx + dy * dy) / 2);
	CBitmap	*bmP;
	tRgbaColorf	color;

	static fVector	vEye = fVector::ZERO;
	static tTexCoord2f	tcCorona [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};

	bmP = bAdditive ? bmpGlare : bmpCorona;
	colorP->alpha = alpha;
	vDir = objP->info.position.mOrient [FVEC].ToFloat();
	vPos = objP->info.position.vPos.ToFloat();
	vCorona [0] = vPos + vDir * (fScale * fLength);
	vh [4] = vCorona [0];
	vCorona [3] = vPos + vDir * (-fScale * fLength);
	G3TransformPoint (vPos, vPos, 0);
	G3TransformPoint (vCorona [0], vCorona [0], 0);
	G3TransformPoint (vCorona [3], vCorona [3], 0);
	vNorm = fVector::Normal(vPos, vCorona [0], vEye);
	fScale *= fRad;
	vCorona [0] += vNorm * fScale;
	vCorona [1] = vCorona [0] + vNorm * (-2 * fScale);
	vCorona [3] += vNorm * fScale;
	vCorona [2] = vCorona [3] + vNorm * (-2 * fScale);
	vNorm = fVector::Normal(vCorona [0], vCorona [1], vCorona [2]);
	vh [0] = vCorona [0] + vCorona [1] * 0.5f;
	vh [2] = vCorona [3] + vCorona [2] * 0.5f;
	vh [1] = vPos + vNorm * fScale;
	vh [3] = vPos + vNorm * (-fScale);
	for (i = 0; i < 4; i++)
		fVector::Normalize(vh [i]);
	a1 = (float) fabs (fVector::Dot(vh [2], vh [0]));
	a2 = (float) fabs (fVector::Dot(vh [3], vh [1]));
#if 0
	HUDMessage (0, "%1.2f %1.2f", a1, a2);
	glLineWidth (2);
	glColor4d (1,1,1,1);
	glDisable (GL_TEXTURE_2D);
	glBegin (GL_LINES);
	for (i = 1; i < 3; i++)
		glVertex3fv (reinterpret_cast<GLfloat*> (vh + i));
	glEnd ();
	glLineWidth (1);
	glColor4fv (reinterpret_cast<GLfloat*> (colorP));
#endif
	if (bAdditive) {
		float fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
		color = *colorP;
		colorP = &color;
		color.red *= fScale;
		color.green *= fScale;
		color.blue *= fScale;
		}
	if (a2 < a1) {
		fix xSize = F2X (fScale);
		G3DrawSprite (objP->info.position.vPos, xSize, xSize, bmP, colorP, alpha, bAdditive, 1);
		}
	else {
		bStencil = StencilOff ();
		glDepthMask (0);
		glEnable (GL_TEXTURE_2D);
		glEnable (GL_BLEND);
		glColor4fv (reinterpret_cast<GLfloat*> (colorP));
		if (bmP->Bind (1, -1))
			return;
		bmP->Texture ()->Wrap (GL_CLAMP);
		if (bAdditive)
			glBlendFunc (GL_ONE, GL_ONE);
		if ((bDrawArrays = G3EnableClientStates (1, 0, 0, GL_TEXTURE0))) {
			glTexCoordPointer (2, GL_FLOAT, 0, tcCorona);
			glVertexPointer (3, GL_FLOAT, sizeof (fVector), vCorona);
			glDrawArrays (GL_QUADS, 0, 4);
			G3DisableClientStates (1, 0, 0, -1);
			}
		else {
			glBegin (GL_QUADS);
			for (i = 0; i < 4; i++) {
				glTexCoord2fv (reinterpret_cast<GLfloat*> (tcCorona + i));
				glVertex3fv (reinterpret_cast<GLfloat*> (vCorona + i));
				}
			glEnd ();
			}
		if (bAdditive)
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if 0
		glLineWidth (2);
		glColor4d (1,1,1,1);
		glDisable (GL_TEXTURE_2D);
		glBegin (GL_LINE_LOOP);
		for (i = 0; i < 4; i++)
			glVertex3fv (reinterpret_cast<GLfloat*> (vCorona + i));
		glEnd ();
		glLineWidth (1);
#endif
		glDepthMask (1);
		StencilOn (bStencil);
		}
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

int RenderWeaponCorona (tObject *objP, tRgbaColorf *colorP, float alpha, fix xOffset,
								float fScale, int bSimple, int bViewerOffset, int bDepthSort)
{
if (!SHOW_OBJ_FX)
	return 0;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return 0;
#endif
if ((objP->info.nType == OBJ_WEAPON) && (objP->info.renderType == RT_POLYOBJ))
	RenderLaserCorona (objP, colorP, alpha, fScale);
else if (gameOpts->render.coronas.bShots && LoadCorona ()) {
	int			bStencil;
	fix			xSize;
	tRgbaColorf	color;

	static tTexCoord2f	tcCorona [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};

	vmsVector	vPos = objP->info.position.vPos;
	xSize = (fix) (WeaponBlobSize (objP->info.nId) * fScale * F1_0);
	bDepthSort = bDepthSort && bSimple && (gameOpts->render.bDepthSort > 0);
	if (xOffset) {
		if (bViewerOffset) {
			vmsVector o = gameData.render.mine.viewerEye - vPos;
			vmsVector::Normalize(o);
			vPos += o * xOffset;
			}
		else
			vPos += objP->info.position.mOrient [FVEC] * xOffset;
		}
	if (xSize < F1_0)
		xSize = F1_0;
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
	if (bDepthSort)
		return TIAddSprite (bmpCorona, vPos, &color, FixMulDiv (xSize, bmpCorona->Width (), bmpCorona->Height ()), xSize, 0, 1, 3);
	bStencil = StencilOff ();
	glDepthMask (0);
	glBlendFunc (GL_ONE, GL_ONE);
	if (bSimple) {
		G3DrawSprite (vPos, FixMulDiv (xSize, bmpCorona->Width (), bmpCorona->Height ()), xSize, bmpCorona, &color, alpha, 1, 3);
		}
	else {
		fVector	quad [4], verts [8], vCenter, vNormal, v;
		float		dot;
		int		i, j;

		glDisable (GL_CULL_FACE);
		glDepthFunc (GL_LEQUAL);
		glDepthMask (0);
		glEnable (GL_TEXTURE_2D);
		if (bmpCorona->Bind (1, -1))
			return 0;
		bmpCorona->Texture ()->Wrap (GL_CLAMP);
		G3StartInstanceMatrix (vPos, objP->info.position.mOrient);
		TransformHitboxf (objP, verts, 0);
		for (i = 0; i < 6; i++) {
			vCenter.SetZero();
			for (j = 0; j < 4; j++) {
				quad [j] = verts [hitboxFaceVerts [i][j]];
				vCenter += quad [j];
				}
			vCenter = vCenter * 0.25f;
			vNormal = fVector::Normal (quad [0], quad [1], quad [2]);
			v = vCenter; fVector::Normalize (v);
			dot = fVector::Dot (vNormal, v);
			if (dot >= 0)
				continue;
			glColor4f (colorP->red, colorP->green, colorP->blue, alpha * (float) sqrt (-dot));
			glBegin (GL_QUADS);
			for (j = 0; j < 4; j++) {
				v = quad [j] - vCenter;
				quad [j] += v * fScale;
 				glTexCoord2fv (reinterpret_cast<GLfloat*> (tcCorona + j));
				glVertex3fv (reinterpret_cast<GLfloat*> (quad + j));
				}
			glEnd ();
			}
		G3DoneInstance ();
		glDepthFunc (GL_LESS);
		glDisable (GL_TEXTURE_2D);
		glEnable (GL_CULL_FACE);
		}
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (1);
	StencilOn (bStencil);
	}
return 0;
}

// -----------------------------------------------------------------------------

//extern vmsAngVec vmsAngVec::ZERO;

void RenderShockwave (tObject *objP)
{
if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if ((objP->info.nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [objP->info.nId]) {
		vmsVector	vPos;
		int			bStencil;

	vPos = objP->info.position.vPos + objP->info.position.mOrient [FVEC] * (objP->info.xSize / 2);
	bStencil = StencilOff ();
	if (EGI_FLAG (bShockwaves, 1, 1, 0) &&
		 (objP->mType.physInfo.velocity [X] || objP->mType.physInfo.velocity [Y] || objP->mType.physInfo.velocity [Z])) {
			fVector			vPosf;
			int				h, i, j, k, n;
			float				r [4], l [4], alpha;
			tRgbaColorf		*pc = gameData.weapons.color + objP->info.nId;

		G3StartInstanceMatrix (vPos, objP->info.position.mOrient);
		glDepthMask (0);
		glDisable (GL_TEXTURE_2D);
		//OglCullFace (1);
		glDisable (GL_CULL_FACE);
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
		for (h = 0; h < 3; h++) {
			glBegin (GL_QUAD_STRIP);
			for (i = 0; i < RING_SIZE + 1; i++) {
				j = i % RING_SIZE;
				for (k = 0; k < 2; k++) {
					n = h + k;
					glColor4f (pc->red, pc->green, pc->blue, (n == 3) ? 0.0f : alpha);
					vPosf = vRing [j];
					vPosf [X] *= r [n];
					vPosf [Y] *= r [n];
					vPosf [Z] = -l [n];
					G3TransformPoint(vPosf, vPosf, 0);
					glVertex3fv (reinterpret_cast<GLfloat*> (&vPosf));
					}
				}
			glEnd ();
			}
		glEnable (GL_CULL_FACE);
		for (h = 0; h < 3; h += 2) {
			glCullFace (h ? GL_FRONT : GL_BACK);
			glColor4f (pc->red, pc->green, pc->blue, h ? 0.1f : alpha);
			glBegin (GL_TRIANGLE_STRIP);
			for (j = 0; j < RING_SIZE; j++) {
				vPosf = vRing [nStripIdx [j]];
				vPosf [X] *= r [h];
				vPosf [Y] *= r [h];
				vPosf [Z] = -l [h];
				G3TransformPoint(vPosf, vPosf, 0);
				glVertex3fv (reinterpret_cast<GLfloat*> (&vPosf));
				}
			glEnd ();
			}
		glDepthMask (1);
		OglCullFace (0);
		G3DoneInstance ();
		}
	StencilOn (bStencil);
	}
}

// -----------------------------------------------------------------------------

#define TRACER_WIDTH	3

void RenderTracers (tObject *objP)
{
if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (EGI_FLAG (bTracers, 0, 1, 0) &&
	 (objP->info.nType == OBJ_WEAPON) && ((objP->info.nId == VULCAN_ID) || (objP->info.nId == GAUSS_ID)
	 /*&& !OBJECTS [objP->cType.laserInfo.parent.nObject].nTracers*/)) {
#if 0
	objP->rType.polyObjInfo.nModel = gameData.weapons.info [SUPERLASER_ID + 1].nModel;
	objP->info.xSize = FixDiv (gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad,
								gameData.weapons.info [objP->info.nId].po_len_to_width_ratio) / 4;
	gameData.models.vScale.Set (F1_0 / 4, F1_0 / 4, F1_0 / 4);
	DrawPolygonObject (objP, 0);
	gameData.models.vScale.SetZero ();
#else
		fVector			vPosf [2], vDirf;
		short				i;
		int				bStencil;
//		static short	patterns [] = {0x0603, 0x0203, 0x0103, 0x0202};

	vPosf [0] = objP->info.position.vPos.ToFloat();
	vPosf [1] = objP->info.vLastPos.ToFloat();
	G3TransformPoint (vPosf [0], vPosf [0], 0);
	G3TransformPoint (vPosf [1], vPosf [1], 0);
	vDirf = vPosf [0] - vPosf [1];
	if (vDirf.IsZero()) {
		//return;
		vPosf [1] = OBJECTS [objP->cType.laserInfo.parent.nObject].info.position.vPos.ToFloat();
		G3TransformPoint(vPosf [1], vPosf [1], 0);
		vDirf = vPosf [0] - vPosf [1];
		if(vDirf.IsZero())
			return;
		}
	bStencil = StencilOff ();
	glDepthMask (0);
	glEnable (GL_LINE_STIPPLE);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_LINE_SMOOTH);
	glLineStipple (6, 0x003F); //patterns [h]);
	vDirf *= TRACER_WIDTH / 20.0f;
/*
	vDirf [Y] *= TRACER_WIDTH / 20.0f;
	vDirf [Z] *= TRACER_WIDTH / 20.0f;
*/
	for (i = 1; i < 5; i++) {
		glLineWidth ((GLfloat) (TRACER_WIDTH * i));
		glBegin (GL_LINES);
		glColor4d (1, 1, 1, 0.5 / i);
		glVertex3fv (reinterpret_cast<GLfloat*> (vPosf + 1));
		glVertex3fv (reinterpret_cast<GLfloat*> (vPosf));
#if 0
		VmVecDec (vPosf, &vDirf);
		VmVecDec (vPosf + 1, &vDirf);
#endif
		glEnd ();
		}
	glLineWidth (1);
	glDisable (GL_LINE_STIPPLE);
	glDisable (GL_LINE_SMOOTH);
	glDepthMask (1);
	StencilOn (bStencil);
#endif
	}
}

// -----------------------------------------------------------------------------
// Draws a texture-mapped laser bolt
#if 0
void Laser_draw_one (int nObject, CBitmap * bmp)
{
	int t1, t2, t3;
	g3sPoint p1, p2;
	tObject *objP = OBJECTS + nObject;
	vmsVector start_pos,vEndPos;
	fix Laser_length = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad * 2;
	fix Laser_width = Laser_length / 8;

	start_pos = objP->info.position.vPos;
	VmVecScaleAdd (&vEndPos,&start_pos,&objP->info.position.mOrient [FVEC],-Laser_length);

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
static fVector vTrailOffs [2][4] = {{{{0,0,0}},{{0,-10,-5}},{{0,-10,-50}},{{0,0,-50}}},
												{{{0,0,0}},{{0,10,-5}},{{0,10,-50}},{{0,0,-50}}}};
#endif

void RenderLightTrail (tObject *objP)
{
	tRgbaColorf		color, *colorP;
	int				nTrailItem = -1, nCoronaItem = -1, bGatling = 0, bAdditive = 1; //gameOpts->render.coronas.bAdditiveObjs;

if (!SHOW_OBJ_FX)
	return;
if (!gameData.objs.bIsWeapon [objP->info.nId])
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
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

if (!gameData.objs.bIsSlowWeapon [objP->info.nId] && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && EGI_FLAG (bLightTrails, 0, 0, 0)) {
	if (gameOpts->render.particles.bPlasmaTrails)
		;//DoObjectSmoke (objP);
	else if (EGI_FLAG (bLightTrails, 1, 1, 0) && (objP->info.nType == OBJ_WEAPON) &&
				!gameData.objs.bIsSlowWeapon [objP->info.nId] &&
				(objP->mType.physInfo.velocity [X] || objP->mType.physInfo.velocity [Y] || objP->mType.physInfo.velocity [Z]) &&
				(bAdditive ? LoadGlare () : LoadCorona ())) {
			fVector			vNormf, vOffsf, vTrailVerts [4];
			int				i, bStencil, bDrawArrays, bDepthSort = (gameOpts->render.bDepthSort > 0);
			float				l, r, dx, dy;
			CBitmap		*bmP;

			static fVector vEye = fVector::ZERO;

			static tRgbaColorf	trailColor = {0,0,0,0.33f};
			static tTexCoord2f	tTexCoordTrail [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};

		if (objP->info.renderType == RT_POLYOBJ) {
			tHitbox	*phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
			l = X2F (phb->vMax [Z] - phb->vMin [Z]);
			dx = X2F (phb->vMax [X] - phb->vMin [X]);
			dy = X2F (phb->vMax [Y] - phb->vMin [Y]);
			r = (float) (sqrt (dx * dx + dy * dy) / sqrt (2.0f));
			if (objP->info.nId == FUSION_ID) {
				l *= 1.5f;
				r /= 1.5f;
				}
			}
		else {
			r = WeaponBlobSize (objP->info.nId) / 1.5f;
			l = 4 * r;
			}
		bmP = bAdditive ? bmpGlare : bmpCorona;
		memcpy (&trailColor, colorP, 3 * sizeof (float));
		if (bAdditive) {
			float fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
			trailColor.red *= fScale;
			trailColor.green *= fScale;
			trailColor.blue *= fScale;
			}
		vOffsf = objP->info.position.mOrient [FVEC].ToFloat();
		vTrailVerts [0] = objP->info.position.vPos.ToFloat();
		vTrailVerts [0] += vOffsf * l;
		vTrailVerts [2] = vTrailVerts [0] - vOffsf * 100;
		G3TransformPoint (vTrailVerts [0], vTrailVerts [0], 0);
		G3TransformPoint (vTrailVerts [2], vTrailVerts [2], 0);
		vOffsf = vTrailVerts [2] - vTrailVerts [0];
		vOffsf = vOffsf * (r * 0.04f);
		vNormf = fVector::Normal (vTrailVerts [0], vTrailVerts [2], vEye);
		vNormf = vNormf * (r * 4);
		vTrailVerts [1] = vTrailVerts [0] + vNormf;
		vTrailVerts [1] += vOffsf;
		vTrailVerts [3] = vTrailVerts [0] - vNormf;
		vTrailVerts [3] += vOffsf;
		if (bDepthSort) {
			nTrailItem = TIAddPoly (NULL, NULL, bmP, vTrailVerts, 4, tTexCoordTrail, &trailColor, NULL, 1, 0, GL_QUADS, GL_CLAMP, bAdditive, -1);
			}
		else {
			glEnable (GL_BLEND);
			if (bAdditive)
				glBlendFunc (GL_ONE, GL_ONE);
			else
				glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4fv (reinterpret_cast<GLfloat*> (&trailColor));
			bDrawArrays = G3EnableClientStates (1, 0, 0, GL_TEXTURE0);
			bStencil = StencilOff ();
			glDisable (GL_CULL_FACE);
			glDepthMask (0);
			glEnable (GL_TEXTURE_2D);
			if (bmP->Bind (1, -1))
				return;
			bmP->Texture ()->Wrap (GL_CLAMP);
			if (bDrawArrays) {
				glTexCoordPointer (2, GL_FLOAT, 0, tTexCoordTrail);
				glVertexPointer (3, GL_FLOAT, sizeof (fVector), vTrailVerts);
				glDrawArrays (GL_QUADS, 0, 4);
				G3DisableClientStates (1, 0, 0, -1);
				}
			else {
				glBegin (GL_QUADS);
				for (i = 0; i < 4; i++) {
					glTexCoord3fv (reinterpret_cast<GLfloat*> (tTexCoordTrail + i));
					glVertex3fv (reinterpret_cast<GLfloat*> (vTrailVerts + i));
					}
				glEnd ();
			if (bAdditive)
				glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if 0 // render outline
				glDisable (GL_TEXTURE_2D);
				glColor3d (1, 0, 0);
				glBegin (GL_LINE_LOOP);
				for (i = 0; i < 4; i++)
					glVertex3fv (reinterpret_cast<GLfloat*> (vTrailVerts + i));
				glEnd ();
#endif
				}
			StencilOn (bStencil);
			glEnable (GL_CULL_FACE);
			glDepthMask (1);
			}
		}
	RenderShockwave (objP);
	}
if ((objP->info.renderType != RT_POLYOBJ) || (objP->info.nId == FUSION_ID))
	RenderWeaponCorona (objP, colorP, 0.5f, 0, 2.0f + X2F (d_rand() % (F1_0 / 8)), 1, 0, 1);
else
	RenderWeaponCorona (objP, colorP, 0.75f, 0, bGatling ? 1.0f : 2.0f, 0, 0, 0);
}

// -----------------------------------------------------------------------------

void DrawDebrisCorona (tObject *objP)
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
			debrisGlow.red = 0.5f + X2F (d_rand () % (F1_0 / 4));
			debrisGlow.green = X2F (d_rand () % (F1_0 / 4));
			}
		RenderWeaponCorona (objP, &debrisGlow, h, 5 * objP->info.xSize, 1.5f, 1, 1, 0);
		}
	}
}

//------------------------------------------------------------------------------

fix flashDist=F2X (.9);

//create flash for tPlayer appearance
void CreatePlayerAppearanceEffect (tObject *playerObjP)
{
	vmsVector	pos;
	tObject		*effectObjP;

if (playerObjP == gameData.objs.viewerP)
	pos = playerObjP->info.position.vPos + playerObjP->info.position.mOrient [FVEC] * FixMul(playerObjP->info.xSize,flashDist);
else
	pos = playerObjP->info.position.vPos;
effectObjP = ObjectCreateExplosion (playerObjP->info.nSegment, &pos, playerObjP->info.xSize, VCLIP_PLAYER_APPEARANCE);
if (effectObjP) {
	effectObjP->info.position.mOrient = playerObjP->info.position.mOrient;
	if (gameData.eff.vClips [0][VCLIP_PLAYER_APPEARANCE].nSound > -1)
		DigiLinkSoundToObject (gameData.eff.vClips [0][VCLIP_PLAYER_APPEARANCE].nSound, OBJ_IDX (effectObjP), 0, F1_0, SOUNDCLASS_PLAYER);
	}
}

//------------------------------------------------------------------------------
//eof
