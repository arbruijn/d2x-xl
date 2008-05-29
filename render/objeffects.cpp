/* $Id: tObject.c, v 1.9 2003/10/04 03:14:47 btb Exp $ */
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
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "render.h"
#include "renderlib.h"
#include "transprender.h"
#include "glare.h"
#include "sphere.h"
#include "flightpath.h"
#include "marker.h"
#include "fireball.h"
#include "objsmoke.h"
#include "objrender.h"
#include "objeffects.h"
#include "hiresmodels.h"

#define fabsf(_f)	(float) fabs (_f)

#define IS_TRACK_GOAL(_objP)	(((_objP) == gameData.objs.trackGoals [0]) || ((_objP) == gameData.objs.trackGoals [1]))

// -----------------------------------------------------------------------------

void RenderObjectHalo (vmsVector *vPos, fix xSize, float red, float green, float blue, float alpha, int bCorona)
{
if ((gameOpts->render.coronas.bShots && (bCorona ? LoadCorona () : LoadHalo ()))) {
	tRgbaColorf	c = {red, green, blue, alpha};
	glDepthMask (0);
	G3DrawSprite (vPos, xSize, xSize, bCorona ? bmpCorona : bmpHalo, &c, alpha * 4.0f / 3.0f, 1);
	glDepthMask (1);
	}
}

// -----------------------------------------------------------------------------

void RenderPowerupCorona (tObject *objP, float red, float green, float blue, float alpha)
{
	int	bAdditive = gameOpts->render.coronas.bAdditiveObjs;

if ((IsEnergyPowerup (objP->id) ? gameOpts->render.coronas.bPowerups : gameOpts->render.coronas.bWeapons) &&
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
	grsBitmap	*bmP = bAdditive ? bmpGlare : bmpCorona;


	if ((objP->id >= POW_KEY_BLUE) && (objP->id <= POW_KEY_GOLD)) {
		int i = objP->id - POW_KEY_BLUE;

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
	glDepthMask (0);
	G3DrawSprite (&objP->position.vPos, xSize, xSize, bmP, &color, alpha, gameOpts->render.coronas.bAdditiveObjs);
	glDepthMask (1);
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
	hv.p.x = f2fl (hitBoxOffsets [i].p.x ? vMin.p.x : vMax.p.x);
	hv.p.y = f2fl (hitBoxOffsets [i].p.y ? vMin.p.y : vMax.p.y);
	hv.p.z = f2fl (hitBoxOffsets [i].p.z ? vMin.p.z : vMax.p.z);
	G3TransformPoint (vertList + i, &hv, 0);
	}
}

//------------------------------------------------------------------------------

#if RENDER_HITBOX

void RenderHitbox (tObject *objP, float red, float green, float blue, float alpha)
{
	fVector		vertList [8], v;
	tHitbox		*pmhb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
	tCloakInfo	ci = {0, GR_ACTUAL_FADE_LEVELS, 0, 0, 0, 0, 0};
	int			i, j, iBox, nBoxes, bHit = 0;
	float			fFade;

if (!SHOW_OBJ_FX)
	return;
if (objP->nType == OBJ_PLAYER) {
	if (!EGI_FLAG (bPlayerShield, 0, 1, 0))
		return;
	if (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_CLOAKED) {
		if (!GetCloakInfo (objP, 0, 0, &ci))
			return;
		fFade = (float) ci.nFadeValue / (float) GR_ACTUAL_FADE_LEVELS;
		red *= fFade;
		green *= fFade;
		blue *= fFade;
		}

	}
else if (objP->nType == OBJ_ROBOT) {
	if (!gameOpts->render.effects.bRobotShields)
		return;
	if (objP->cType.aiInfo.CLOAKED) {
		if (!GetCloakInfo (objP, 0, 0, &ci))
			return;
		fFade = (float) ci.nFadeValue / (float) GR_ACTUAL_FADE_LEVELS;
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
G3StartInstanceMatrix (&objP->position.vPos, &objP->position.mOrient);
for (; iBox <= nBoxes; iBox++) {
	if (iBox)
		G3StartInstanceAngles (&pmhb [iBox].vOffset, &avZero);
	TransformHitboxf (objP, vertList, iBox);
	glBegin (GL_QUADS);
	for (i = 0; i < 6; i++) {
		for (j = 0; j < 4; j++)
			glVertex3fv ((GLfloat *) (vertList + hitboxFaceVerts [i][j]));
		}
	glEnd ();
	glLineWidth (2);
	for (i = 0; i < 6; i++) {
		glBegin (GL_LINES);
		v.p.x = v.p.y = v.p.z = 0;
		for (j = 0; j < 4; j++) {
			glVertex3fv ((GLfloat *) (vertList + hitboxFaceVerts [i][j]));
			VmVecInc (&v, vertList + hitboxFaceVerts [i][j]);
			}
		glEnd ();
		}
	glLineWidth (1);
	if (iBox)
		G3DoneInstance ();
	}
G3DoneInstance ();
#if 0//def _DEBUG	//display collision point
if (gameStates.app.nSDLTicks - gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].tHit < 500) {
	tObject	o;

	o.position.vPos = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].vHit;
	o.position.mOrient = objP->position.mOrient;
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
	int			bStencil, dt = 0, i = objP->id, nColor = 0;
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
		scale = (float) ci.nFadeValue / (float) GR_ACTUAL_FADE_LEVELS;
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
		alpha = f2fl (gameData.multiplayer.players [i].shields) / 100.0f;
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

	tCloakInfo	ci;
	float			scale = 1;
	fix			dt;

#if RENDER_HITBOX
RenderHitbox (objP, 0.5f, 0.0f, 0.6f, 0.4f);
#else
if (!gameOpts->render.effects.bRobotShields)
	return;
if ((objP->nType == OBJ_ROBOT) && objP->cType.aiInfo.CLOAKED) {
	if (!GetCloakInfo (objP, 0, 0, &ci))
		return;
	scale = (float) ci.nFadeValue / (float) GR_ACTUAL_FADE_LEVELS;
	scale *= scale;
	}
dt = gameStates.app.nSDLTicks - gameData.objs.xTimeLastHit [OBJ_IDX (objP)];
if (dt < 300) {
	scale *= gameOpts->render.effects.bOnlyShieldHits ? (float) cos (sqrt ((double) dt / 300.0) * Pi / 2) : 1;
	DrawShieldSphere (objP, shieldColors [2].red * scale, shieldColors [2].green * scale, shieldColors [2].blue * scale, 0.5f * scale);
	}
else if (!gameOpts->render.effects.bOnlyShieldHits) {
	if ((objP->nType != OBJ_ROBOT) || ROBOTINFO (objP->id).companion)
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
	if (objP->nType == OBJ_REACTOR)
		return &reactorDefColor;
	else if (objP->nType == OBJ_ROBOT) {
		if (!ROBOTINFO (objP->id).companion)
			return &botDefColor;
		}
	else if (objP->nType == OBJ_PLAYER) {
		if (IsTeamGame)
			return playerDefColors + GetTeam (objP->id) + 1;
		return playerDefColors;
		}
	}
return &defaultColor;
}

// -----------------------------------------------------------------------------

void RenderDamageIndicator (tObject *objP, tRgbColorf *pc)
{
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
	VmVecFixToFloat (&fPos, &objP->position.vPos);
	G3TransformPoint (&fPos, &fPos, 0);
	r = f2fl (objP->size);
	r2 = r / 10;
	r = r2 * 9;
	w = 2 * r;
	fPos.p.x -= r;
	fPos.p.y += r;
	w *= ObjectDamage (objP);
	fVerts [0].p.x = fVerts [3].p.x = fPos.p.x;
	fVerts [1].p.x = fVerts [2].p.x = fPos.p.x + w;
	fVerts [0].p.y = fVerts [1].p.y = fPos.p.y;
	fVerts [2].p.y = fVerts [3].p.y = fPos.p.y - r2;
	fVerts [0].p.z = fVerts [1].p.z = fVerts [2].p.z = fVerts [3].p.z = fPos.p.z;
	fVerts [0].p.w = fVerts [1].p.w = fVerts [2].p.w = fVerts [3].p.w = 1;
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
			glVertex3fv ((GLfloat *) (fVerts + i));
		glEnd ();
		}
#else
	bDrawArrays = 0;
	glBegin (GL_QUADS);
	glVertex3f (fPos.p.x, fPos.p.y, fPos.p.z);
	glVertex3f (fPos.p.x + w, fPos.p.y, fPos.p.z);
	glVertex3f (fPos.p.x + w, fPos.p.y - r2, fPos.p.z);
	glVertex3f (fPos.p.x, fPos.p.y - r2, fPos.p.z);
	glEnd ();
#endif
	w = 2 * r;
	fVerts [1].p.x = fVerts [2].p.x = fPos.p.x + w;
	glColor3fv ((GLfloat *) pc);
	if (bDrawArrays) {
		glVertexPointer (4, GL_FLOAT, 0, fVerts);
		glDrawArrays (GL_LINE_LOOP, 0, 4);
		glDisableClientState (GL_VERTEX_ARRAY);
		}
	else {
		glBegin (GL_LINE_LOOP);
		for (i = 0; i < 4; i++)
			glVertex3fv ((GLfloat *) (fVerts + i));
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

	fVector				fPos, fVerts [3];
	float					r, r2;
	int					nTgtInd, bHasDmg, bVertexArrays, bMarker = (objP->nType == OBJ_MARKER);

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
VmVecFixToFloat (&fPos, &objP->position.vPos);
G3TransformPoint (&fPos, &fPos, 0);
r = f2fl (objP->size);
if (bMarker)
	r = 17 * r / 12;
r2 = r / 4;

glDisable (GL_CULL_FACE);
G3DisableClientStates (1, 1, 1, GL_TEXTURE0);
bVertexArrays = G3EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0);
glActiveTexture (GL_TEXTURE0);
glDisable (GL_TEXTURE_2D);
glColor4fv ((GLfloat *) (trackGoalColor + bMarker));
if (bMarker || gameOpts->render.cockpit.bRotateMslLockInd) {
	fVector	rotVerts [3];
	fMatrix	mRot;
	int		h, i, j;

	if (bInitSinCos) {
		OglComputeSinCos (sizeofa (sinCosInd), sinCosInd);
		bInitSinCos = 0;
		}
	mRot.rVec.p.x =
	mRot.uVec.p.y = sinCosInd [nMslLockIndPos [bMarker]].fCos;
	mRot.uVec.p.x = sinCosInd [nMslLockIndPos [bMarker]].fSin;
	mRot.rVec.p.y = -mRot.uVec.p.x;
	mRot.rVec.p.z =
	mRot.uVec.p.z =
	mRot.fVec.p.x = 
	mRot.fVec.p.y = 0;
	mRot.fVec.p.z = 1;

	fVerts [0].p.z =
	fVerts [1].p.z =
	fVerts [2].p.z = 0;
	rotVerts [0].p.w = 
	rotVerts [1].p.w = 
	rotVerts [2].p.w = 0;
	fVerts [0].p.x = -r2;
	fVerts [1].p.x = +r2;
	fVerts [2].p.x = 0;
	fVerts [0].p.y = 
	fVerts [1].p.y = +r;
	fVerts [2].p.y = +r - r2;
	if (bVertexArrays)
		glVertexPointer (3, GL_FLOAT, sizeof (fVector), rotVerts);
	for (j = 0; j < 4; j++) {
		for (i = 0; i < 3; i++) {
			VmVecRotate (rotVerts + i, fVerts + i, &mRot);
			fVerts [i] = rotVerts [i];
			VmVecInc (rotVerts + i, &fPos);
			}
		if (bMarker)
			glLineWidth (2);
		if (bVertexArrays)
			glDrawArrays (bMarker ? GL_LINE_LOOP : GL_TRIANGLES, 0, 3);
		else {
			glBegin (bMarker ? GL_LINE_LOOP : GL_TRIANGLES);
			for (h = 0; h < 3; h++)
				glVertex3fv ((GLfloat *) (rotVerts + h));
			glEnd ();
			}
		if (bMarker)
			glLineWidth (1);
		if (!j) {	//now rotate by 90 degrees
			mRot.rVec.p.x =
			mRot.uVec.p.y = 0;
			mRot.uVec.p.x = 1;
			mRot.rVec.p.y = -1;
			}
		}
	}
else {
	fVerts [0].p.z =
	fVerts [1].p.z =
	fVerts [2].p.z = fPos.p.z;
	fVerts [0].p.w = 
	fVerts [1].p.w = 
	fVerts [2].p.w = 1;
	fVerts [0].p.x = fPos.p.x - r2;
	fVerts [1].p.x = fPos.p.x + r2;
	fVerts [2].p.x = fPos.p.x;
	glVertexPointer (4, GL_FLOAT, 0, fVerts);
	nTgtInd = extraGameInfo [IsMultiGame].bTargetIndicators;
	bHasDmg = !EGI_FLAG (bTagOnlyHitObjs, 0, 1, 0) | (ObjectDamage (objP) < 1);
	if (!nTgtInd ||
		 ((nTgtInd == 1) && (!EGI_FLAG (bDamageIndicators, 0, 1, 0) || !bHasDmg)) ||
		 ((nTgtInd == 2) && !bHasDmg)) {
		fVerts [0].p.y = 
		fVerts [1].p.y = fPos.p.y + r;
		fVerts [2].p.y = fPos.p.y + r - r2;
		glDrawArrays (GL_TRIANGLES, 0, 3);
		}
	fVerts [0].p.y = 
	fVerts [1].p.y = fPos.p.y - r;
	fVerts [2].p.y = fPos.p.y - r + r2;
	glDrawArrays (GL_TRIANGLES, 0, 3);
	fVerts [0].p.x = 
	fVerts [1].p.x = fPos.p.x + r;
	fVerts [2].p.x = fPos.p.x + r - r2;
	fVerts [0].p.y = fPos.p.y + r2;
	fVerts [1].p.y = fPos.p.y - r2;
	fVerts [2].p.y = fPos.p.y;
	glDrawArrays (GL_TRIANGLES, 0, 3);
	fVerts [0].p.x = 
	fVerts [1].p.x = fPos.p.x - r;
	fVerts [2].p.x = fPos.p.x - r + r2;
	glDrawArrays (GL_TRIANGLES, 0, 3);
	}
glDisableClientState (GL_VERTEX_ARRAY);
glEnable (GL_CULL_FACE);
}

// -----------------------------------------------------------------------------

void RenderTargetIndicator (tObject *objP, tRgbColorf *pc)
{
	fVector		fPos, fVerts [4];
	float			r, r2, r3;
	int			i, bStencil, bDrawArrays, nPlayer = (objP->nType == OBJ_PLAYER) ? objP->id : -1;

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
	else if (objP->nType == OBJ_ROBOT) {
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
		  (tRgbColorf *) &trackGoalColor [0] : ObjectFrameColor (objP, pc);
	VmVecFixToFloat (&fPos, &objP->position.vPos);
	G3TransformPoint (&fPos, &fPos, 0);
	r = f2fl (objP->size);
	glColor3fv ((GLfloat *) pc);
	fVerts [0].p.w = fVerts [1].p.w = fVerts [2].p.w = fVerts [3].p.w = 1;
	glVertexPointer (4, GL_FLOAT, 0, fVerts);
	if (extraGameInfo [IsMultiGame].bTargetIndicators == 1) {	//square brackets
		r2 = r * 2 / 3;
		fVerts [0].p.x = fVerts [3].p.x = fPos.p.x - r2;
		fVerts [1].p.x = fVerts [2].p.x = fPos.p.x - r;
		fVerts [0].p.y = fVerts [1].p.y = fPos.p.y - r;
		fVerts [2].p.y = fVerts [3].p.y = fPos.p.y + r;
		fVerts [0].p.z =
		fVerts [1].p.z =
		fVerts [2].p.z =
		fVerts [3].p.z = fPos.p.z;
		if ((bDrawArrays = G3EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0)))
			glDrawArrays (GL_LINE_STRIP, 0, 4);
		else {
			glBegin (GL_LINE_STRIP);
			for (i = 0; i < 4; i++)
				glVertex3fv ((GLfloat *) (fVerts + i));
			glEnd ();
			}
		fVerts [0].p.x = fVerts [3].p.x = fPos.p.x + r2;
		fVerts [1].p.x = fVerts [2].p.x = fPos.p.x + r;
		if (bDrawArrays) {
			glDrawArrays (GL_LINE_STRIP, 0, 4);
			glDisableClientState (GL_VERTEX_ARRAY);
			}
		else {
			glBegin (GL_LINE_STRIP);
			for (i = 0; i < 4; i++)
				glVertex3fv ((GLfloat *) (fVerts + i));
			glEnd ();
			}
		}
	else {	//triangle
		r2 = r / 3;
		fVerts [0].p.x = fPos.p.x - r2;
		fVerts [1].p.x = fPos.p.x + r2;
		fVerts [2].p.x = fPos.p.x;
		fVerts [0].p.y = fVerts [1].p.y = fPos.p.y + r;
		fVerts [2].p.y = fPos.p.y + r - r2;
		fVerts [0].p.z =
		fVerts [1].p.z =
		fVerts [2].p.z = fPos.p.z;
		if ((bDrawArrays = G3EnableClientState (GL_VERTEX_ARRAY, GL_TEXTURE0)))
			glDrawArrays (GL_LINE_LOOP, 0, 3);
		else {
			glBegin (GL_LINE_LOOP);
			glVertex3fv ((GLfloat *) fVerts);
			glVertex3fv ((GLfloat *) (fVerts + 1));
			glVertex3fv ((GLfloat *) (fVerts + 2));
			glEnd ();
			}
		if (EGI_FLAG (bDamageIndicators, 0, 1, 0)) {
			r3 = ObjectDamage (objP);
			if (r3 < 1.0f) {
				if (r3 < 0.0f)
					r3 = 0.0f;
				fVerts [0].p.x = fPos.p.x - r2 * r3;
				fVerts [1].p.x = fPos.p.x + r2 * r3;
				fVerts [2].p.x = fPos.p.x;
				fVerts [0].p.y = fVerts [1].p.y = fPos.p.y + r - r2 * (1.0f - r3);
				//fVerts [2].p.y = fPos.p.y + r - r2;
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
			glVertex3fv ((GLfloat *) (fVerts + i));
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
		{{0.0f, 2.0f / 3.0f, 0.0f, 1.0f}},
		{{0.0f, 2.0f / 3.0f, -1.0f, 1.0f}},
		{{0.0f, -(1.0f / 3.0f), -1.0f, 1.0f}},
		{{0.0f, -(1.0f / 3.0f), 0.0f, 1.0f}}
	};

	static tTexCoord2f texCoordList [4] = {{{0.0f, -0.3f}}, {{1.0f, -0.3f}}, {{1.0f, 0.7f}}, {{0.0f, 0.7f}}};

if (gameStates.app.bNostalgia)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (IsTeamGame && (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_FLAG)) {
		vmsVector		vPos = objP->position.vPos;
		fVector			vPosf;
		tFlagData		*pf = gameData.pig.flags + !GetTeam (objP->id);
		tPathPoint		*pp = GetPathPoint (&pf->path);
		int				i, bStencil;
		float				r;
		grsBitmap		*bmP;

	if (pp) {
		bStencil = StencilOff ();
		OglActiveTexture (GL_TEXTURE0, 0);
		glEnable (GL_TEXTURE_2D);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		PIGGY_PAGE_IN (pf->bmi.index, 0);
		bmP = gameData.pig.tex.pBitmaps + pf->vcP->frames [pf->vci.nCurFrame].index;
		if (OglBindBmTex (bmP, 1, 2))
			return;
		bmP = BmCurFrame (bmP, -1);
		OglTexWrap (bmP->glTexture, GL_REPEAT);
		VmVecScaleInc (&vPos, &objP->position.mOrient.fVec, -objP->size);
		r = f2fl (objP->size);
		G3StartInstanceMatrix (&vPos, &pp->mOrient);
		glBegin (GL_QUADS);
		glColor3f (1.0f, 1.0f, 1.0f);
		for (i = 0; i < 4; i++) {
			vPosf.p.x = 0;
			vPosf.p.y = fVerts [i].p.y * r;
			vPosf.p.z = fVerts [i].p.z * r;
			G3TransformPoint (&vPosf, &vPosf, 0);
			glTexCoord2fv ((GLfloat *) (texCoordList + i));
			glVertex3fv ((GLfloat *) &vPosf);
			}
		for (i = 3; i >= 0; i--) {
			vPosf.p.x = 0;
			vPosf.p.y = fVerts [i].p.y * r;
			vPosf.p.z = fVerts [i].p.z * r;
			G3TransformPoint (&vPosf, &vPosf, 0);
			glTexCoord2fv ((GLfloat *) (texCoordList + i));
			glVertex3fv ((GLfloat *) &vPosf);
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
	{{-0.5f, -0.5f, 0.0f, 1.0f}},
	{{-0.6533f, -0.2706f, 0.0f, 1.0f}},
	{{-0.7071f, 0.0f, 0.0f, 1.0f}},
	{{-0.6533f, 0.2706f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f, 1.0f}},
	{{-0.2706f, 0.6533f, 0.0f, 1.0f}},
	{{0.0f, 0.7071f, 0.0f, 1.0f}},
	{{0.2706f, 0.6533f, 0.0f, 1.0f}},
	{{0.5f, 0.5f, 0.0f, 1.0f}},
	{{0.6533f, 0.2706f, 0.0f, 1.0f}},
	{{0.7071f, 0.0f, 0.0f, 1.0f}},
	{{0.6533f, -0.2706f, 0.0f, 1.0f}},
	{{0.5f, -0.5f, 0.0f, 1.0f}},
	{{0.2706f, -0.6533f, 0.0f, 1.0f}},
	{{0.0f, -0.7071f, 0.0f, 1.0f}},
	{{-0.2706f, -0.6533f, 0.0f, 1.0f}}
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
			pv->p.x = vRing [j].p.x * (float) sinPhi;
			pv->p.y = vRing [j].p.y * (float) sinPhi;
			pv->p.z = z;
			}
		}
	m = n = THRUSTER_SEGS - i + 1;
	for (phi = Pi / 2; i < THRUSTER_SEGS; i++, phi += Pi / 8, z -= fStep [1], m--) {
		sinPhi = (1 + sin (phi) / 2) * fScale * m / n;
		for (j = 0; j < RING_SIZE; j++, pv++) {
			pv->p.x = vRing [j].p.x * (float) sinPhi;
			pv->p.y = vRing [j].p.y * (float) sinPhi;
			pv->p.z = z;
			}
		}
	bHaveFlame = 1;
	}
}

// -----------------------------------------------------------------------------

void CalcShipThrusterPos (tObject *objP, vmsVector *vPos)
{
	tPosition	*pPos = OBJPOS (objP);

if (gameOpts->render.bHiresModels) {
	VmVecScaleAdd (vPos, &pPos->vPos, &pPos->mOrient.fVec, -objP->size);
	VmVecScaleInc (vPos, &pPos->mOrient.rVec, -(8 * objP->size / 44));
	VmVecScaleAdd (vPos + 1, vPos, &pPos->mOrient.rVec, 8 * objP->size / 22);
	}
else {
	VmVecScaleAdd (vPos, &pPos->vPos, &pPos->mOrient.fVec, -objP->size / 10 * 9);
	if (gameStates.app.bFixModels)
		VmVecScaleInc (vPos, &pPos->mOrient.uVec, objP->size / 40);
	else
		VmVecScaleInc (vPos, &pPos->mOrient.uVec, -objP->size / 20);
	vPos [1] = vPos [0];
	VmVecScaleInc (vPos, &pPos->mOrient.rVec, -8 * objP->size / 49);
	VmVecScaleInc (vPos + 1, &pPos->mOrient.rVec, 8 * objP->size / 49);
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
if (gameOpts->render.bHiresModels && (objP->nType == OBJ_PLAYER) && !ASEModel (objP->rType.polyObjInfo.nModel)) {
	if (!bSpectate) {
		pt = gameData.render.thrusters + objP->id;
		ti.pp = GetPathPoint (&pt->path);
		}
	ti.fSize = (ti.fLength + 1) / 2;
	nThrusters = 2;
	CalcShipThrusterPos (objP, ti.vPos);
	ti.mtP = NULL;
	}
else if (bAfterburnerBlob || (bMissile && !nThrusters)) {
		tHitbox	*phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
		fix		nObjRad = gameOpts->render.nPath ? (phb->vMax.p.z - phb->vMin.p.z) / 2 : 2 * (phb->vMax.p.z - phb->vMin.p.z) / 3;

	if (bAfterburnerBlob)
		nObjRad *= 2;
	if (objP->id == EARTHSHAKER_ID)
		ti.fSize = 1.0f;
	else if ((objP->id == MEGAMSL_ID) || (objP->id == EARTHSHAKER_MEGA_ID))
		ti.fSize = 0.8f;
	else if (objP->id == SMARTMSL_ID)
		ti.fSize = 0.6f;
	else
		ti.fSize = 0.5f;
	nThrusters = 1;
	if (EGI_FLAG (bThrusterFlames, 1, 1, 0) == 2)
		ti.fLength /= 2;
	if (gameData.models.nScale)
		VmVecScale (ti.vPos, gameData.models.nScale);
	VmVecScaleAdd (ti.vPos, &objP->position.vPos, &objP->position.mOrient.fVec, -nObjRad);
	ti.mtP = NULL;
	}
else if ((objP->nType == OBJ_PLAYER) || 
			((objP->nType == OBJ_ROBOT) && !objP->cType.aiInfo.CLOAKED) || 
			bMissile) {
	vmsMatrix	m, *viewP;
	if (!bSpectate && (objP->nType == OBJ_PLAYER)) {
		pt = gameData.render.thrusters + objP->id;
		ti.pp = GetPathPoint (&pt->path);
		}
	if (!nThrusters) {
		if (objP->nType != OBJ_PLAYER)
			return 0;
		if (!bSpectate) {
			pt = gameData.render.thrusters + objP->id;
			ti.pp = GetPathPoint (&pt->path);
			}
		ti.fSize = (ti.fLength + 1) / 2;
		nThrusters = 2;
		CalcShipThrusterPos (objP, ti.vPos);
		}
	else {
		tPosition *posP = OBJPOS (objP);
		if (SPECTATOR (objP))
			VmCopyTransposeMatrix (viewP = &m, &posP->mOrient);
		else
			viewP = ObjectView (objP);
		for (i = 0; i < nThrusters; i++) {
			VmVecRotate (ti.vPos + i, ti.mtP->vPos + i, viewP);
			if (gameData.models.nScale)
				VmVecScale (ti.vPos + i, gameData.models.nScale);
			VmVecInc (ti.vPos + i, &posP->vPos);
			VmVecRotate (ti.vDir + i, ti.mtP->vDir + i, viewP);
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
if ((objP->nType == OBJ_PLAYER) && (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_CLOAKED))
	return;
fSpeed = f2fl (VmVecMag (&objP->mType.physInfo.velocity));
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
	if (OglBindBmTex (bmpThruster [nStyle], 1, -1)) {
		extraGameInfo [IsMultiGame].bThrusterFlames = 2;
		glDisable (GL_TEXTURE_2D);
		}
	else {
		OglTexWrap (bmpThruster [nStyle]->glTexture, GL_CLAMP);
		bTextured = 1;
		}
	}
if (nThrusters > 1) {
	vmsVector vRot [2];
	for (i = 0; i < 2; i++)
		G3RotatePoint (vRot + i, ti.vPos + i, 0);
	if (vRot [0].p.z < vRot [1].p.z) {
		vmsVector v = ti.vPos [0];
		ti.vPos [0] = ti.vPos [1];
		ti.vPos [1] = v;
		if (objP->nType == OBJ_ROBOT) {
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
		static fVector	vEye = {{0, 0, 0}};

		fVector	vPosf, vNormf, vFlame [3], vThruster [4], fVecf;
		float		c = 1/*0.7f + 0.03f * fPulse*/, dotFlame, dotThruster;

	if (gameData.models.nScale)
		ti.fSize *= f2fl (gameData.models.nScale);
	ti.fLength *= 4 * ti.fSize;
	ti.fSize *= ((objP->nType == OBJ_PLAYER) && HaveHiresModel (objP->rType.polyObjInfo.nModel)) ? 1.2f : 1.5f;
#if 1
	if (!ti.mtP)
		VmVecFixToFloat (&fVecf, ti.pp ? &ti.pp->mOrient.fVec : &objP->position.mOrient.fVec);
#endif
	for (h = 0; h < nThrusters; h++) {
		if (ti.mtP)
			VmVecFixToFloat (&fVecf, ti.vDir + h);
		VmVecFixToFloat (&vPosf, ti.vPos + h);
		VmVecScaleAdd (vFlame + 2, &vPosf, &fVecf, -ti.fLength);
		G3TransformPoint (vFlame + 2, vFlame + 2, 0);
		G3TransformPoint (&vPosf, &vPosf, 0);
		VmVecNormal (&vNormf, vFlame + 2, &vPosf, &vEye);
		VmVecScaleAdd (vFlame, &vPosf, &vNormf, ti.fSize);
		VmVecScaleAdd (vFlame + 1, &vPosf, &vNormf, -ti.fSize);
		VmVecNormal (&vNormf, vFlame, vFlame + 1, vFlame + 2);
		vThruster [0] = vFlame [0];
		vThruster [2] = vFlame [1];
		VmVecScaleAdd (vThruster + 1, &vPosf, &vNormf, ti.fSize);
		VmVecScaleAdd (vThruster + 3, &vPosf, &vNormf, -ti.fSize);
		VmVecNormalize (&vPosf, &vPosf);
		VmVecNormalize (&v, vFlame + 2);
		dotFlame = VmVecDot (&vPosf, &v);
		VmVecNormalize (&v, vThruster);
		dotThruster = VmVecDot (&vPosf, &v);
		if (gameOpts->render.bDepthSort > 0)
			RIAddThruster (bmpThruster [nStyle], vThruster, tcThruster, (dotFlame < dotThruster) ? vFlame : NULL, tcFlame);
		else {
			glDisable (GL_CULL_FACE);
			glColor3f (c, c, c);
			if (dotFlame < dotThruster) {
				glBegin (GL_TRIANGLES);
				for (i = 0; i < 3; i++) {
					glTexCoord2fv ((GLfloat *) (tcFlame + i));
					glVertex3fv ((GLfloat *) (vFlame + i));
					}
				glEnd ();
				}
			glBegin (GL_QUADS);
			for (i = 0; i < 4; i++) {
				glTexCoord2fv ((GLfloat *) (tcThruster + i));
				glVertex3fv ((GLfloat *) (vThruster + i));
				}
			glEnd ();
			glEnable (GL_CULL_FACE);
			}
		}
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
		G3StartInstanceMatrix (ti.vPos + h, (ti.pp && !bSpectate) ? &ti.pp->mOrient : &objP->position.mOrient);
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
					v.p.x *= ti.fSize;
					v.p.y *= ti.fSize;
					v.p.z *= ti.fLength;
					G3TransformPoint (&v, &v, 0);
					if (bTextured) {
						tTexCoord2fl.v.v = 0.25f + tTexCoord2flStep.v.v * (i + l);
						glTexCoord2fv ((GLfloat *) &tTexCoord2fl);
						}
					else
						glColor4fv ((GLfloat *) (c + l)); // (c [l].red, c [l].green, c [l].blue, c [l].alpha);
					glVertex3fv ((GLfloat *) &v);
					}
				}
			glEnd ();
#else
			glBegin (GL_LINE_LOOP);
			glColor4f (c [1].red, c [1].green, c [1].blue, c [1].alpha);
			for (j = 0; j < RING_SIZE; j++) {
				G3TransformPoint (&v, vFlame [i] + j);
				glVertex3fv ((GLfloat *) &v);
				}
			glEnd ();
#endif
			}
		glBegin (GL_TRIANGLE_STRIP);
		for (j = 0; j < RING_SIZE; j++) {
			G3TransformPoint (&v, vFlame [0] + nStripIdx [j], 0);
			glVertex3fv ((GLfloat *) &v);
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

typedef struct tVertRef {
	float	dot;
	int	i;
} tVertRef;

typedef struct fEdge {
	int	v0, v1;
	int	f0, f1;
	int	bContour;
	int	nPred, nSucc;	//previous and next connected contour edge
} fEdge;

// -----------------------------------------------------------------------------

#define EXPAND_CORONA	2

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
	float			fLength = f2fl (phb->vMax.p.z - phb->vMin.p.z) / 2;
	float			dx = f2fl (phb->vMax.p.x - phb->vMin.p.x);
	float			dy = f2fl (phb->vMax.p.y - phb->vMin.p.y);
	float			fRad = (float) (sqrt (dx * dx + dy * dy) / 2);
	grsBitmap	*bmP;
	tRgbaColorf	color;

	static fVector	vEye = {{0, 0, 0}};
	static tTexCoord2f	tcCorona [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};

	bmP = bAdditive ? bmpGlare : bmpCorona;
	colorP->alpha = alpha;
	VmVecFixToFloat (&vDir, &objP->position.mOrient.fVec);
	VmVecFixToFloat (&vPos, &objP->position.vPos);
	VmVecScaleAdd (vCorona, &vPos, &vDir, fScale * fLength);
	vh [4] = vCorona [0];
	VmVecScaleAdd (vCorona + 3, &vPos, &vDir, -fScale * fLength);
	G3TransformPoint (&vPos, &vPos, 0);
	G3TransformPoint (vCorona, vCorona, 0);
	G3TransformPoint (vCorona + 3, vCorona + 3, 0);
	VmVecNormal (&vNorm, &vPos, vCorona, &vEye);
	fScale *= fRad;
	VmVecScaleInc (vCorona, &vNorm, fScale);
	VmVecScaleAdd (vCorona + 1, vCorona, &vNorm, -2 * fScale);
	VmVecScaleInc (vCorona + 3, &vNorm, fScale);
	VmVecScaleAdd (vCorona + 2, vCorona + 3, &vNorm, -2 * fScale);
	VmVecNormal (&vNorm, vCorona, vCorona + 1, vCorona + 2);
	VmVecScaleAdd (vh, vCorona, vCorona + 1, 0.5f);
	VmVecScaleAdd (vh + 2, vCorona + 3, vCorona + 2, 0.5f);
	VmVecScaleAdd (vh + 1, &vPos, &vNorm, fScale);
	VmVecScaleAdd (vh + 3, &vPos, &vNorm, -fScale);
	for (i = 0; i < 4; i++)
		VmVecNormalize (vh + i, vh + i);
	a1 = (float) fabs (VmVecDot (vh + 2, vh));
	a2 = (float) fabs (VmVecDot (vh + 3, vh + 1));
#if 0
	HUDMessage (0, "%1.2f %1.2f", a1, a2);
	glLineWidth (2);
	glColor4d (1,1,1,1);
	glDisable (GL_TEXTURE_2D);
	glBegin (GL_LINES);
	for (i = 1; i < 3; i++)
		glVertex3fv ((GLfloat *) (vh + i));
	glEnd ();
	glLineWidth (1);
	glColor4fv ((GLfloat *) colorP);
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
		fix xSize = fl2f (fScale);
		G3DrawSprite (&objP->position.vPos, xSize, xSize, bmP, colorP, alpha, bAdditive);
		}
	else {
		bStencil = StencilOff ();
		glDepthMask (0);
		glEnable (GL_TEXTURE_2D);
		glEnable (GL_BLEND);
		glColor4fv ((GLfloat *) colorP);
		if (OglBindBmTex (bmP, 1, -1)) 
			return;
		OglTexWrap (bmP->glTexture, GL_CLAMP);
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
				glTexCoord2fv ((GLfloat *) (tcCorona + i));
				glVertex3fv ((GLfloat *) (vCorona + i));
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
			glVertex3fv ((GLfloat *) (vCorona + i));
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
else
	return 1.0f;
}

// -----------------------------------------------------------------------------

void RenderWeaponCorona (tObject *objP, tRgbaColorf *colorP, float alpha, fix xOffset, 
								 float fScale, int bSimple, int bViewerOffset, int bDepthSort)
{
if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if ((objP->nType == OBJ_WEAPON) && (objP->renderType == RT_POLYOBJ))
	RenderLaserCorona (objP, colorP, alpha, fScale);
else if (gameOpts->render.coronas.bShots && LoadCorona ()) {
	int			bStencil;
	fix			xSize;
	tRgbaColorf	color;

	static tTexCoord2f	tcCorona [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};

	vmsVector	vPos = objP->position.vPos;
	xSize = (fix) (WeaponBlobSize (objP->id) * fScale * F1_0);
	bDepthSort = bDepthSort && bSimple && (gameOpts->render.bDepthSort > 0);
	if (xOffset) {
		if (bViewerOffset) {
			vmsVector o;
			VmVecNormalize (VmVecSub (&o, &gameData.render.mine.viewerEye /*&gameData.objs.console->position.vPos*/, &vPos));
			VmVecScaleInc (&vPos, &o, xOffset);
			}
		else
			VmVecScaleInc (&vPos, &objP->position.mOrient.fVec, xOffset);
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
	if (bDepthSort) {
		RIAddSprite (bmpCorona, &vPos, &color, FixMulDiv (xSize, bmpCorona->bmProps.w, bmpCorona->bmProps.h), xSize, 0, 1);
		return;
		}
	bStencil = StencilOff ();
	glDepthMask (0);
	glBlendFunc (GL_ONE, GL_ONE);
	if (bSimple) {
		G3DrawSprite (&vPos, FixMulDiv (xSize, bmpCorona->bmProps.w, bmpCorona->bmProps.h), xSize, bmpCorona, &color, alpha, 1);
		}
	else {
		fVector	quad [4], verts [8], vCenter, vNormal, v;
		float		dot;
		int		i, j;

		glDisable (GL_CULL_FACE);
		glDepthFunc (GL_LEQUAL);
		glDepthMask (0);
		glEnable (GL_TEXTURE_2D);
		if (OglBindBmTex (bmpCorona, 1, -1)) 
			return;
		OglTexWrap (bmpCorona->glTexture, GL_CLAMP);
		G3StartInstanceMatrix (&vPos, &objP->position.mOrient);
		TransformHitboxf (objP, verts, 0);
		for (i = 0; i < 6; i++) {
			vCenter.p.x = vCenter.p.y = vCenter.p.z = 0;
			for (j = 0; j < 4; j++) {
				quad [j] = verts [hitboxFaceVerts [i][j]];
				VmVecInc (&vCenter, quad + j);
				}
			VmVecScale (&vCenter, &vCenter, 0.25f);
			VmVecNormal (&vNormal, quad, quad + 1, quad + 2);
			VmVecNormalize (&v, &vCenter);
			dot = VmVecDot (&vNormal, &v);
			if (dot >= 0)
				continue;
			glColor4f (colorP->red, colorP->green, colorP->blue, alpha * (float) sqrt (-dot));
			glBegin (GL_QUADS);
			for (j = 0; j < 4; j++) {
				VmVecSub (&v, quad + j, &vCenter);
				VmVecScaleInc (quad + j, &v, fScale);
 				glTexCoord2fv ((GLfloat *) (tcCorona + j));
				glVertex3fv ((GLfloat *) (quad + j));
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
}

// -----------------------------------------------------------------------------

extern vmsAngVec avZero;

void RenderShockwave (tObject *objP)
{
if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if ((objP->nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [objP->id]) {
		vmsVector	vPos;
		int			bStencil;

	VmVecScaleAdd (&vPos, &objP->position.vPos, &objP->position.mOrient.fVec, objP->size / 2);
	bStencil = StencilOff ();
	if (EGI_FLAG (bShockwaves, 1, 1, 0) && 
		 (objP->mType.physInfo.velocity.p.x || objP->mType.physInfo.velocity.p.y || objP->mType.physInfo.velocity.p.z)) {
			fVector			vPosf;
			int				h, i, j, k, n;
			float				r [4], l [4], alpha;
			tRgbaColorf		*pc = gameData.weapons.color + objP->id;

		G3StartInstanceMatrix (&vPos, &objP->position.mOrient);
		glDepthMask (0);
		glDisable (GL_TEXTURE_2D);
		//OglCullFace (1);
		glDisable (GL_CULL_FACE);	
		r [3] = f2fl (objP->size);
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
					vPosf.p.x *= r [n];
					vPosf.p.y *= r [n];
					vPosf.p.z = -l [n];
					G3TransformPoint (&vPosf, &vPosf, 0);
					glVertex3fv ((GLfloat *) &vPosf);
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
				vPosf.p.x *= r [h];
				vPosf.p.y *= r [h];
				vPosf.p.z = -l [h];
				G3TransformPoint (&vPosf, &vPosf, 0);
				glVertex3fv ((GLfloat *) &vPosf);
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
	 (objP->nType == OBJ_WEAPON) && ((objP->id == VULCAN_ID) || (objP->id == GAUSS_ID)
	 /*&& !gameData.objs.nTracers [objP->cType.laserInfo.nParentObj]*/)) {
#if 0
	objP->rType.polyObjInfo.nModel = gameData.weapons.info [SUPERLASER_ID + 1].nModel;
	objP->size = FixDiv (gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad, 
								gameData.weapons.info [objP->id].po_len_to_width_ratio) / 4;
	gameData.models.nScale = F1_0 / 4;
	DrawPolygonObject (objP, 0);
	gameData.models.nScale = 0;
#else
		fVector			vPosf [2], vDirf;
		short				i;
		int				bStencil;
//		static short	patterns [] = {0x0603, 0x0203, 0x0103, 0x0202};

	VmVecFixToFloat (vPosf, &objP->position.vPos);
	VmVecFixToFloat (vPosf + 1, &objP->vLastPos);
	G3TransformPoint (vPosf, vPosf, 0);
	G3TransformPoint (vPosf + 1, vPosf + 1, 0);
	VmVecSub (&vDirf, vPosf, vPosf + 1);
	if (!(vDirf.p.x || vDirf.p.y || vDirf.p.z)) {
		//return;
		VmVecFixToFloat (vPosf + 1, &OBJECTS [objP->cType.laserInfo.nParentObj].position.vPos);
		G3TransformPoint (vPosf + 1, vPosf + 1, 0);
		VmVecSub (&vDirf, vPosf, vPosf + 1);
		if (!(vDirf.p.x || vDirf.p.y || vDirf.p.z))
			return;
		}
	bStencil = StencilOff ();
	glDepthMask (0);
	glEnable (GL_LINE_STIPPLE);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_LINE_SMOOTH);
	glLineStipple (6, 0x003F); //patterns [h]);
	vDirf.p.x *= TRACER_WIDTH / 20.0f;
	vDirf.p.y *= TRACER_WIDTH / 20.0f;
	vDirf.p.z *= TRACER_WIDTH / 20.0f;
	for (i = 1; i < 5; i++) {
		glLineWidth ((GLfloat) (TRACER_WIDTH * i));
		glBegin (GL_LINES);
		glColor4d (1, 1, 1, 0.5 / i);
		glVertex3fv ((GLfloat *) (vPosf + 1));
		glVertex3fv ((GLfloat *) vPosf);
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
void Laser_draw_one (int nObject, grsBitmap * bmp)
{
	int t1, t2, t3;
	g3sPoint p1, p2;
	tObject *objP = OBJECTS + nObject;
	vmsVector start_pos,vEndPos;
	fix Laser_length = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad * 2;
	fix Laser_width = Laser_length / 8;

	start_pos = objP->position.vPos;
	VmVecScaleAdd (&vEndPos,&start_pos,&objP->position.mOrient.fVec,-Laser_length);

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
	GrSetColor (gr_getcolor (31,15,0);
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
	int				bAdditive = 1; //gameOpts->render.coronas.bAdditiveObjs;

if (!SHOW_OBJ_FX)
	return;
if (!gameData.objs.bIsWeapon [objP->id])
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (objP->renderType == RT_POLYOBJ)
	colorP = gameData.weapons.color + objP->id;
else {
	tRgbColorb	*pcb = VClipColor (objP);
	color.red = pcb->red / 255.0f;
	color.green = pcb->green / 255.0f;
	color.blue = pcb->blue / 255.0f;
	colorP = &color;
	}

if (!gameData.objs.bIsSlowWeapon [objP->id] && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && EGI_FLAG (bLightTrails, 0, 0, 0)) {
	if (gameOpts->render.smoke.bPlasmaTrails)
		DoObjectSmoke (objP);
	else if (EGI_FLAG (bLightTrails, 1, 1, 0) && (objP->nType == OBJ_WEAPON) && 
				!gameData.objs.bIsSlowWeapon [objP->id] &&
				(objP->mType.physInfo.velocity.p.x || objP->mType.physInfo.velocity.p.y || objP->mType.physInfo.velocity.p.z) &&
				(bAdditive ? LoadGlare () : LoadCorona ())) {
			fVector			vNormf, vOffsf, vTrailVerts [4];
			int				i, bStencil, bDrawArrays, bDepthSort = (gameOpts->render.bDepthSort > 0);
			float				l, r, dx, dy;
			grsBitmap		*bmP;

			static fVector vEye = {{0, 0, 0}};

			static tRgbaColorf	trailColor = {0,0,0,0.33f};
			static tTexCoord2f	tTexCoordTrail [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};
		
		if (objP->renderType == RT_POLYOBJ) {
			tHitbox	*phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
			l = f2fl (phb->vMax.p.z - phb->vMin.p.z);
			dx = f2fl (phb->vMax.p.x - phb->vMin.p.x);
			dy = f2fl (phb->vMax.p.y - phb->vMin.p.y);
			r = (float) (sqrt (dx * dx + dy * dy) / sqrt (2.0f));
			if (objP->id == FUSION_ID) {
				l *= 1.5f;
				r /= 1.5f;
				}
			}
		else {
			r = WeaponBlobSize (objP->id) / 1.5f;
			l = 4 * r;
			}

		VmVecFixToFloat (&vOffsf, &objP->position.mOrient.fVec);
		VmVecFixToFloat (vTrailVerts, &objP->position.vPos);
		VmVecScaleInc (vTrailVerts, &vOffsf, l);// * -0.75f);
		VmVecScaleAdd (vTrailVerts + 2, vTrailVerts, &vOffsf, -100);
		G3TransformPoint (vTrailVerts, vTrailVerts, 0);
		G3TransformPoint (vTrailVerts + 2, vTrailVerts + 2, 0);
		VmVecSub (&vOffsf, vTrailVerts + 2, vTrailVerts);
		VmVecScale (&vOffsf, &vOffsf, r * 0.04f);
		VmVecNormal (&vNormf, vTrailVerts, vTrailVerts + 2, &vEye);
		VmVecScale (&vNormf, &vNormf, r * 4);
		VmVecAdd (vTrailVerts + 1, vTrailVerts, &vNormf);
		VmVecInc (vTrailVerts + 1, &vOffsf);
		VmVecSub (vTrailVerts + 3, vTrailVerts, &vNormf);
		VmVecInc (vTrailVerts + 3, &vOffsf);
		bmP = bAdditive ? bmpGlare : bmpCorona;
		memcpy (&trailColor, colorP, 3 * sizeof (float));
		if (bAdditive) {
			float fScale = coronaIntensities [gameOpts->render.coronas.nObjIntensity] / 2;
			trailColor.red *= fScale;
			trailColor.green *= fScale;
			trailColor.blue *= fScale;
			}
		if (bDepthSort) {
			RIAddPoly (NULL, NULL, bmP, vTrailVerts, 4, tTexCoordTrail, &trailColor, NULL, 1, 0, GL_QUADS, GL_CLAMP, bAdditive, -1);
			}
		else {
			glEnable (GL_BLEND);
			if (bAdditive)
				glBlendFunc (GL_ONE, GL_ONE);
			else
				glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4fv ((GLfloat *) &trailColor);
			bDrawArrays = G3EnableClientStates (1, 0, 0, GL_TEXTURE0);
			bStencil = StencilOff ();
			glDisable (GL_CULL_FACE);	
			glDepthMask (0);
			glEnable (GL_TEXTURE_2D);
			if (OglBindBmTex (bmP, 1, -1)) 
				return;
			OglTexWrap (bmP->glTexture, GL_CLAMP);
			if (bDrawArrays) {
				glTexCoordPointer (2, GL_FLOAT, 0, tTexCoordTrail);
				glVertexPointer (3, GL_FLOAT, sizeof (fVector), vTrailVerts);
				glDrawArrays (GL_QUADS, 0, 4);
				G3DisableClientStates (1, 0, 0, -1);
				}
			else {
				glBegin (GL_QUADS);
				for (i = 0; i < 4; i++) {
					glTexCoord3fv ((GLfloat *) (tTexCoordTrail + i));
					glVertex3fv ((GLfloat *) (vTrailVerts + i));
					}
				glEnd ();
			if (bAdditive)
				glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if 0 // render outline
				glDisable (GL_TEXTURE_2D);
				glColor3d (1, 0, 0);
				glBegin (GL_LINE_LOOP);
				for (i = 0; i < 4; i++)
					glVertex3fv ((GLfloat *) (vTrailVerts + i));
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
if ((objP->renderType != RT_POLYOBJ) || (objP->id == FUSION_ID))
	RenderWeaponCorona (objP, colorP, 0.5f, 0, 2, 1, 0, 1);
else
	RenderWeaponCorona (objP, colorP, 0.75f, 0, 2, 0, 0, 0);
}

// -----------------------------------------------------------------------------

void DrawDebrisCorona (tObject *objP)
{
	static	tRgbaColorf	debrisGlow = {0.66f, 0, 0, 1};
	static	tRgbaColorf	markerGlow = {0, 0.66f, 0, 1};
	static	time_t t0 = 0;

if (objP->nType == OBJ_MARKER)
	RenderWeaponCorona (objP, &markerGlow, 0.75f, 0, 4, 1, 1, 0);
#ifdef _DEBUG
else if (objP->nType == OBJ_DEBRIS) {
#else
else if ((objP->nType == OBJ_DEBRIS) && gameOpts->render.nDebrisLife) {
#endif
	float	h = (float) nDebrisLife [gameOpts->render.nDebrisLife] - f2fl (objP->lifeleft);
	if (h < 0)
		h = 0;
	if (h < 10) {
		h = (10 - h) / 20.0f;
		if (gameStates.app.nSDLTicks - t0 > 50) {
			t0 = gameStates.app.nSDLTicks;
			debrisGlow.red = 0.5f + f2fl (d_rand () % (F1_0 / 4));
			debrisGlow.green = f2fl (d_rand () % (F1_0 / 4));
			}
		RenderWeaponCorona (objP, &debrisGlow, h, 5 * objP->size / 2, 1.5f, 1, 1, 0);
		}
	}
}

//------------------------------------------------------------------------------

fix flashDist=fl2f (.9);

//create flash for tPlayer appearance
void CreatePlayerAppearanceEffect (tObject *playerObjP)
{
	vmsVector	pos;
	tObject		*effectObjP;

if (playerObjP == gameData.objs.viewer)
	VmVecScaleAdd (&pos, &playerObjP->position.vPos, &playerObjP->position.mOrient.fVec, FixMul (playerObjP->size,flashDist));
else
	pos = playerObjP->position.vPos;
effectObjP = ObjectCreateExplosion (playerObjP->nSegment, &pos, playerObjP->size, VCLIP_PLAYER_APPEARANCE);
if (effectObjP) {
	effectObjP->position.mOrient = playerObjP->position.mOrient;
	if (gameData.eff.vClips [0][VCLIP_PLAYER_APPEARANCE].nSound > -1)
		DigiLinkSoundToObject (gameData.eff.vClips [0][VCLIP_PLAYER_APPEARANCE].nSound, OBJ_IDX (effectObjP), 0, F1_0, SOUNDCLASS_PLAYER);
	}
}

//------------------------------------------------------------------------------
//eof
