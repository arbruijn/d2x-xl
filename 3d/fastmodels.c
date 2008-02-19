/* $Id: interp.c, v 1.14 2003/03/19 19:21:34 btb Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: interp.c, v 1.14 2003/03/19 19:21:34 btb Exp $";
#endif

#include <math.h>
#include <stdlib.h>
#include "error.h"

#include "inferno.h"
#include "interp.h"
#include "shadows.h"
#include "hitbox.h"
#include "globvars.h"
#include "gr.h"
#include "byteswap.h"
#include "u_mem.h"
#include "console.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "network.h"
#include "render.h"
#include "gameseg.h"
#include "light.h"
#include "dynlight.h"
#include "lightning.h"
#include "renderthreads.h"
#include "hiresmodels.h"
#include "buildmodel.h"

extern tFaceColor tMapColor;

#if DBG_SHADOWS
extern int bShadowTest;
extern int bFrontCap;
extern int bRearCap;
extern int bShadowVolume;
extern int bFrontFaces;
extern int bBackFaces;
extern int bSWCulling;
#endif
#if SHADOWS
extern int bZPass;
#endif

#define G3_DRAW_ARRAYS				1
#define G3_DRAW_SUBMODELS			1
#define G3_FAST_MODELS				1
#define G3_DRAW_RANGE_ELEMENTS	1
#define G3_SW_SCALING				0
#define G3_HW_LIGHTING				1
#define G3_USE_VBOS					1 //G3_HW_LIGHTING
#define G3_ALLOW_TRANSPARENCY		1

#define G3_BUFFER_OFFSET(_i)	(GLvoid *) ((char *) NULL + (_i))

//------------------------------------------------------------------------------

void G3DynLightModel (tObject *objP, tG3Model *pm, short iVerts, short nVerts, short iFaceVerts, short nFaceVerts)
{
	fVector			vPos, vVertex;
	fVector3			*pv, *pn;
	tG3ModelVertex	*pmv;
	tFaceColor		*pc;
	float				fAlpha = GrAlpha ();
	int				h, i, bEmissive = (objP->nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [objP->id] && !gameData.objs.bIsMissile [objP->id];

if (!gameStates.render.bBrightObject) {
	VmVecFixToFloat (&vPos, &objP->position.vPos);
	for (i = iVerts, pv = pm->pVerts + iVerts, pn = pm->pVertNorms + iVerts, pc = pm->pColor + iVerts; 
		  i < nVerts; 
		  i++, pv++, pn++, pc++) {
		pc->index = 0;
		VmVecAddf (&vVertex, &vPos, (fVector *) pv);
		G3VertexColor ((fVector *) pn, &vVertex, i, pc, NULL, 1, 0, 0);
		}
	}
for (i = iFaceVerts, h = iFaceVerts, pmv = pm->pFaceVerts + iFaceVerts; i < nFaceVerts; i++, h++, pmv++) {
	if (gameStates.render.bBrightObject || bEmissive) {
		pm->pVBColor [h] = pmv->baseColor;
		pm->pVBColor [h].alpha = fAlpha;
		}
	else if (pmv->bTextured)
		pm->pVBColor [h] = pm->pColor [pmv->nIndex].color;
	else {
		pc = pm->pColor + pmv->nIndex;
		pm->pVBColor [h].red = pmv->baseColor.red * pc->color.red;
		pm->pVBColor [h].green = pmv->baseColor.green * pc->color.green;
		pm->pVBColor [h].blue = pmv->baseColor.blue * pc->color.blue;
		pm->pVBColor [h].alpha = pmv->baseColor.alpha;
		}
	}
}

//------------------------------------------------------------------------------

void G3LightModel (tObject *objP, int nModel, fix xModelLight, fix *xGlowValues, int bHires)
{
	tG3Model			*pm = gameData.models.g3Models [bHires] + nModel;
	tG3ModelVertex	*pmv;
	tG3ModelFace	*pmf;
	tRgbaColorf		baseColor, *colorP;
	float				fLight, fAlpha = (float) gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS; 
	int				h, i, j, l;
	int				bEmissive = (objP->nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [objP->id] && !gameData.objs.bIsMissile [objP->id];

#ifdef _DEBUG
if (OBJ_IDX (objP) == nDbgObj)
	objP = objP;
#endif
#if 0
if (xModelLight > F1_0)
	xModelLight = F1_0;
#endif
if (SHOW_DYN_LIGHT && (gameOpts->ogl.bObjLighting || 
    (gameOpts->ogl.bLightObjects && (gameOpts->ogl.bLightPowerups || (objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_ROBOT))))) {
	tiRender.objP = objP;
	tiRender.pm = pm;
	if (!RunRenderThreads (rtPolyModel))
		G3DynLightModel (objP, pm, 0, pm->nVerts, 0, pm->nFaceVerts);
	}
else {
	if (gameData.objs.color.index)
		baseColor = gameData.objs.color.color;
	else
		baseColor.red = baseColor.green = baseColor.blue = 1;
	baseColor.alpha = fAlpha;
	for (i = pm->nFaces, pmf = pm->pFaces; i; i--, pmf++) {
		if (pmf->nBitmap >= 0)
			colorP = &baseColor;
		else
			colorP = NULL;
		if (pmf->bGlow) 
			l = xGlowValues [nGlow];
		else if (bEmissive)
			l = F1_0;
		else {
			l = -VmVecDot (&viewInfo.view [0].fVec, &pmf->vNormal);
			l = 3 * f1_0 / 4 + l / 4;
			l = FixMul (l, xModelLight);
			}
		fLight = f2fl (l);
		for (j = pmf->nVerts, h = pmf->nIndex, pmv = pm->pFaceVerts + pmf->nIndex; j; j--, h++, pmv++) {
#if G3_DRAW_ARRAYS
			if (colorP) {
				pm->pVBColor [h].red = colorP->red * fLight;
				pm->pVBColor [h].green = colorP->green * fLight;
				pm->pVBColor [h].blue = colorP->blue * fLight;
				pm->pVBColor [h].alpha = fAlpha;
				}
			else {
				pm->pVBColor [h].red = pmv->baseColor.red * fLight;
				pm->pVBColor [h].green = pmv->baseColor.green * fLight;
				pm->pVBColor [h].blue = pmv->baseColor.blue * fLight;
				pm->pVBColor [h].alpha = fAlpha;
				}
#else
			if (colorP) {
				pmv->renderColor.red = colorP->red * fLight;
				pmv->renderColor.green = colorP->green * fLight;
				pmv->renderColor.blue = colorP->blue * fLight;
				pmv->renderColor.alpha = fAlpha;
				}
			else {
				pmv->renderColor.red = pmv->baseColor.red * fLight;
				pmv->renderColor.green = pmv->baseColor.green * fLight;
				pmv->renderColor.blue = pmv->baseColor.blue * fLight;
				pmv->renderColor.alpha = fAlpha;
				}
#endif
			}
		}
	}
}

//------------------------------------------------------------------------------

#if G3_SW_SCALING

void G3ScaleModel (int nModel, int bHires)
{
	tG3Model			*pm = gameData.models.g3Models [bHires] + nModel;
	float				fScale = gameData.models.nScale ? f2fl (gameData.models.nScale) : 1;
	int				i;
	fVector3			*pv;
	tG3ModelVertex	*pmv;

if (pm->fScale == fScale)
	return;
fScale /= pm->fScale;
for (i = pm->nVerts, pv = pm->pVerts; i; i--, pv++) {
	pv->p.x *= fScale;
	pv->p.y *= fScale;
	pv->p.z *= fScale;
	}
for (i = pm->nFaceVerts, pmv = pm->pFaceVerts; i; i--, pmv++)
	pmv->vertex = pm->pVerts [pmv->nIndex];
pm->fScale *= fScale;
}

#endif

//------------------------------------------------------------------------------

void G3GetThrusterPos (tObject *objP, short nModel, tG3ModelFace *pmf, vmsVector *vOffset, 
							  vmsVector *vNormal, int nRad, int bHires)
{
	tG3Model				*pm = gameData.models.g3Models [bHires] + nModel;
	tG3ModelVertex		*pmv = NULL;
	fVector				v = {{0,0,0}}, vn, vo, vForward = {{0,0,1}};
	tModelThrusters	*mtP = gameData.models.thrusters + nModel;
	int					i, j;
	float					h, nSize;

if (!pm->bRendered)
	mtP->nCount = 0;
#if 0//def _DEBUG
mtP->nCount = 0;
#endif
if (!objP || (mtP->nCount >= 2))
	return;
VmVecFixToFloat (&vn, pmf ? &pmf->vNormal : vNormal);
if (VmVecDotf (&vn, &vForward) > -1.0f / 3.0f)
	return;
if (pmf) {
	for (i = 0, j = pmf->nVerts, pmv = pm->pFaceVerts + pmf->nIndex; i < j; i++)
		VmVecIncf (&v, (fVector *) &pmv [i].vertex);
	v.p.x /= j;
	v.p.y /= j;
	v.p.z /= j;
	}
else
	v.p.x = v.p.y = v.p.z = 0;
v.p.z -= 1.0f / 16.0f;
#if 0
G3TransformPoint (&v, &v, 0);
#else
#	if 1
if (vOffset) {
	VmVecFixToFloat (&vo, vOffset);
	VmVecIncf (&v, &vo);
	}
#	endif
#endif
if (mtP->nCount && (v.p.x == mtP->vPos [0].p.x) && (v.p.y == mtP->vPos [0].p.y) && (v.p.z == mtP->vPos [0].p.z))
	return;
mtP->vPos [mtP->nCount].p.x = fl2f (v.p.x);
mtP->vPos [mtP->nCount].p.y = fl2f (v.p.y);
mtP->vPos [mtP->nCount].p.z = fl2f (v.p.z);
if (vOffset)
	VmVecDecf (&v, &vo);
mtP->vDir [mtP->nCount] = *vNormal;
VmVecNegate (mtP->vDir + mtP->nCount);
if (!mtP->nCount) {
	if (!pmf)
		mtP->fSize = f2fl (nRad);
	else {
		for (i = 0, nSize = 1000000000; i < j; i++)
			if (nSize > (h = VmVecDistf (&v, (fVector *) &pmv [i].vertex)))
				nSize = h;
		mtP->fSize = nSize;// * 1.25f;
		}
	}
mtP->nCount++;
}

//------------------------------------------------------------------------------

int G3FilterSubModel (tObject *objP, tG3SubModel *psm, int nGunId, int nBombId, int nMissileId, int nMissiles)
{
if (!psm->bRender)
	return 0;
if (psm->nGunPoint >= 0)
	return 1;
if (psm->bWeapon) {
	tPlayer	*playerP = gameData.multiplayer.players + objP->id;
	int		bLasers = (nGunId == LASER_INDEX) || (nGunId == SUPER_LASER_INDEX);
	int		bSuperLasers = playerP->laserLevel > MAX_LASER_LEVEL;
	int		bQuadLasers = (playerP->flags & PLAYER_FLAGS_QUAD_LASERS) != 0;
	int		bCenter = (nGunId == VULCAN_INDEX) || (nGunId == GAUSS_INDEX) || (nGunId == OMEGA_INDEX);

	if (EGI_FLAG (bShowWeapons, 0, 1, 0)) {
		if (psm->nGun == 127)
			return 1;
		if (psm->nGun == nGunId + 1) {
			if (psm->nGun == FUSION_INDEX + 1) {
				if ((psm->nWeaponPos == 3) && !gameData.multiplayer.weaponStates [gameData.multiplayer.nLocalPlayer].bTripleFusion)
					return 1;
				}
			else if (bLasers) {
				if ((psm->nWeaponPos > 2) && !bQuadLasers)
					return 1;
				}
			}
		else if (psm->nGun == LASER_INDEX + 1) {
			if (gameOpts->render.ship.nWingtip)
				return 1;
			if (bLasers || bSuperLasers || bQuadLasers)
				return 1;
			return !bCenter && (psm->nWeaponPos < 3);
			}
		else if (psm->nGun == SUPER_LASER_INDEX + 1) {
			if (gameOpts->render.ship.nWingtip)
				return 1;
			if (bLasers || !bSuperLasers)
				return 1;
			return !bCenter && (psm->nWeaponPos < 3);
			}
		else if (!psm->nGun) {
			if (bLasers && bQuadLasers)
				return 1;
			if (psm->nType != gameOpts->render.ship.nWingtip)
				return 1;
			return 0;
			}
		else if (psm->nBomb == nBombId)
			return 0;
		else if ((psm->nMissile == nMissileId) && (psm->nWeaponPos <= nMissiles))
			return 0;
		else
			return 1;
		}
	else {
		if (psm->nGun == 127)
			return 0;
		if ((psm->nGun < 0) && (psm->nMissile == 1))
			return 0;
		return 1;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void G3DrawSubModel (tObject *objP, short nModel, short nSubModel, short nExclusive, grsBitmap **modelBitmaps, 
						   vmsAngVec *pAnimAngles, vmsVector *vOffset, int bHires, int bUseVBO, int nPass, int bTransparency,
							int nGunId, int nBombId, int nMissileId, int nMissiles)
{
	tG3Model			*pm = gameData.models.g3Models [bHires] + nModel;
	tG3SubModel		*psm = pm->pSubModels + nSubModel;
	tG3ModelFace	*pmf;
	grsBitmap		*bmP = NULL;
	vmsAngVec		va = pAnimAngles ? pAnimAngles [psm->nAngles] : avZero;
	vmsVector		vo;
	int				h, i, j, bTransparent, bAnimate, bTextured = !(gameStates.render.bCloaked /*|| nPass*/);
	short				nId, nFaceVerts, nVerts, nIndex, nBitmap = -1, nTeamColor;

if ((objP->nType == OBJ_PLAYER) && IsMultiGame)
	nTeamColor = (IsTeamGame ? GetTeam (objP->id) : objP->id) + 1;
else
	nTeamColor = 2;
#if 1
if (psm->bThruster) {
	if (!nPass)
		G3GetThrusterPos (objP, nModel, NULL, VmVecAdd (&vo, &psm->vOffset, &psm->vCenter), 
								&psm->pFaces->vNormal, psm->nRad, bHires);
	return;
	}
if (G3FilterSubModel (objP, psm, nGunId, nBombId, nMissileId, nMissiles))
	return;
#endif
vo = psm->vOffset;
if (gameData.models.nScale)
	VmVecScale (&vo, gameData.models.nScale);
if (vOffset && (nExclusive < 0)) {
	G3StartInstanceAngles (&vo, &va);
	VmVecInc (&vo, vOffset);
	}
if ((bAnimate = psm->nFrames && gameData.multiplayer.weaponStates [objP->id].bFiring [0])) {
	if (gameStates.app.nSDLTicks - psm->tFrame > 40 * gameStates.gameplay.slowmo [0].fSpeed) {
		psm->tFrame = gameStates.app.nSDLTicks;
		psm->iFrame = ++psm->iFrame % psm->nFrames;
		}
#if 1
	glPushMatrix ();
	glTranslatef (0, f2fl (psm->vCenter.p.y), 0);
	glRotatef (360 * (float) psm->iFrame / (float) psm->nFrames, 0, 0, 1);
	glTranslatef (0, -f2fl (psm->vCenter.p.y), 0);
#else
	va.b = (psm->iFrame * F1_0 / psm->nFrames) % F1_0;
	vo = psm->vCenter;
	VmVecNegate (&vo);
	G3StartInstanceAngles (&vo, &va);
#endif
	}
#if G3_DRAW_SUBMODELS
// render any dependent submodels
for (i = 0, j = pm->nSubModels, psm = pm->pSubModels; i < j; i++, psm++)
	if (psm->nParent == nSubModel)
		G3DrawSubModel (objP, nModel, i, nExclusive, modelBitmaps, pAnimAngles, &vo, bHires, 
							 bUseVBO, nPass, bTransparency, nGunId, nBombId, nMissileId, nMissiles);
#endif
// render the faces
if ((nExclusive < 0) || (nSubModel == nExclusive)) {
	if (vOffset && (nSubModel == nExclusive))
		G3StartInstanceMatrix (vOffset, NULL);
	glDisable (GL_TEXTURE_2D);
	if (gameStates.render.bCloaked)
		glColor4f (0, 0, 0, GrAlpha ());
	for (psm = pm->pSubModels + nSubModel, i = psm->nFaces, pmf = psm->pFaces; i; ) {
		if (bTextured && (nBitmap != pmf->nBitmap)) {
			if (0 > (nBitmap = pmf->nBitmap)) 
				glDisable (GL_TEXTURE_2D);
			else {
				if (!bHires)
					bmP = modelBitmaps [nBitmap];
				else {
					bmP = pm->pTextures + nBitmap;
					if (nTeamColor && bmP->bmTeam && (0 <= (h = pm->teamTextures [nTeamColor - 1]))) {
						nBitmap = h;
						bmP = pm->pTextures + nBitmap;
						}
					}
				glActiveTexture (GL_TEXTURE0);
				glClientActiveTexture (GL_TEXTURE0);
				glEnable (GL_TEXTURE_2D);
				bmP = BmOverride (bmP, -1);
				if (BM_FRAMES (bmP))
					bmP = BM_CURFRAME (bmP);
				if (OglBindBmTex (bmP, 1, 3))
					continue;
				OglTexWrap (bmP->glTexture, GL_REPEAT);
				}
			}
		nIndex = pmf->nIndex;
#if G3_DRAW_ARRAYS
#	if G3_ALLOW_TRANSPARENCY
		if (bHires) {
			bTransparent = bmP && ((bmP->bmProps.flags & BM_FLAG_TRANSPARENT) != 0);
			if (bTransparent != bTransparency) {
				if (bTransparent)
					pm->bHasTransparency = 1;
				pmf++;
				i--;
				continue;
				}
			}
#	endif
#if G3_DRAW_ARRAYS
		if ((nFaceVerts = pmf->nVerts) > 4) {
#else
		if ((nFaceVerts = pmf->nVerts) > 0) {
#endif
			if (!nPass && pmf->bThruster)
				G3GetThrusterPos (objP, nModel, pmf, &vo, &pmf->vNormal, 0, bHires);
			nVerts = nFaceVerts;
			pmf++;
			i--;
			}
		else { 
			nId = pmf->nId;
			nVerts = 0;
			do {
				if (!nPass && pmf->bThruster)
					G3GetThrusterPos (objP, nModel, pmf, &vo, &pmf->vNormal, 0, bHires);
				nVerts += nFaceVerts;
				pmf++;
				i--;
				} while (i && (pmf->nId == nId));
			}
#else
		nFaceVerts = pmf->nVerts;
		if (pmf->bThruster)
			G3GetThrusterPos (objP, nModel, pmf, vOffset, &pmf->vNormal, 0, bHires);
		nVerts = nFaceVerts;
		pmf++;
		i--;
#endif
#ifdef _DEBUG
		if (nFaceVerts == 8)
			nFaceVerts = nFaceVerts;
		if (nIndex + nVerts > pm->nFaceVerts)
			break;
#endif
#if G3_DRAW_ARRAYS
#	if G3_DRAW_RANGE_ELEMENTS
		if (glDrawRangeElements)
			if (bUseVBO)
				glDrawRangeElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN, 
											0, nVerts - 1, nVerts, GL_UNSIGNED_SHORT, 
											G3_BUFFER_OFFSET (nIndex * sizeof (short)));
			else
				glDrawRangeElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN, 
											nIndex, nIndex + nVerts - 1, nVerts, GL_UNSIGNED_SHORT, 
											pm->pIndex [0] + nIndex);

		else
			if (bUseVBO)
				glDrawElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN, 
									nVerts, GL_UNSIGNED_SHORT, G3_BUFFER_OFFSET (nIndex * sizeof (short)));
			else
				glDrawElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN, 
									nVerts, GL_UNSIGNED_SHORT, pm->pIndex + nIndex);
#	else
		glDrawArrays ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN, nIndex, nVerts);
#	endif
#else
		{
		tG3ModelVertex	*pmv = pm->pFaceVerts + nIndex;
		glBegin ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN);
		for (j = nVerts; j; j--, pmv++) {
			if (!gameStates.render.bCloaked) {
				glTexCoord2fv ((GLfloat *) &pmv->texCoord);
				glColor4fv ((GLfloat *) (pm->pVBColor + pmv->nIndex));
				}
			if (gameOpts->ogl.bObjLighting)
				glNormal3fv ((GLfloat *) &pmv->normal);
			glVertex3fv ((GLfloat *) &pmv->vertex);
			}
		glEnd ();
		pmv -= nVerts;
#	if 0
		glDisable (GL_TEXTURE_2D);
		glBegin (GL_LINE_LOOP);
		for (j = nVerts; j; j--, pmv++) {
			if (!gameStates.render.bCloaked) {
				glTexCoord2fv ((GLfloat *) &pmv->texCoord);
				glColor4fv ((GLfloat *) (pm->pVBColor + pmv->nIndex));
				}
			if (gameOpts->ogl.bObjLighting)
				glNormal3fv ((GLfloat *) &pmv->normal);
			glVertex3fv ((GLfloat *) &pmv->vertex);
			}
		glEnd ();
		if (bmP)
			glEnable (GL_TEXTURE_2D);
#	endif
		}
#endif
		}
	}
if (bAnimate)
	glPopMatrix ();
if ((nExclusive < 0) || (nSubModel == nExclusive))
	G3DoneInstance ();
}

//------------------------------------------------------------------------------

void G3DrawModel (tObject *objP, short nModel, short nSubModel, grsBitmap **modelBitmaps, 
						vmsAngVec *pAnimAngles, vmsVector *vOffset, int bHires, int bUseVBO, int bTransparency,
						int nGunId, int nBombId, int nMissileId, int nMissiles)
{
	tG3Model			*pm;
	tShaderLight	*psl;
	int				nPass, iLightSource = 0, iLight, nLights;
	int				bEmissive = objP && (objP->nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [objP->id] && !gameData.objs.bIsMissile [objP->id];
	int				bLighting = SHOW_DYN_LIGHT && gameOpts->ogl.bObjLighting && !(gameStates.render.bQueryCoronas || gameStates.render.bCloaked || bEmissive);
	GLenum			hLight;
	tPosition		*posP = OBJPOS (objP);

if (bLighting) {
	nLights = gameData.render.lights.dynamic.shader.nActiveLights [0];
	OglEnableLighting (0); 
	}
else
	nLights = 1;
glEnable (GL_BLEND);
if (bEmissive)
	glBlendFunc (GL_ONE, GL_ONE);
else if (gameStates.render.bCloaked)
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
else if (bTransparency) {
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (0);
	}
else
	glBlendFunc (GL_ONE, GL_ZERO);
for (nPass = 0; nLights || !nPass; nPass++) {
	if (bLighting) {
		if (nPass) {
			glBlendFunc (GL_ONE, GL_ONE);
			glDepthMask (0);
			}
		OglSetupTransform (1);
		for (iLight = 0; (iLight < 8) && nLights; iLight++, nLights--, iLightSource++) { 
			psl = gameData.render.lights.dynamic.shader.activeLights [0][iLightSource];
			hLight = GL_LIGHT0 + iLight;
			glEnable (hLight);
//			sprintf (szLightSources + strlen (szLightSources), "%d ", (psl->nObject >= 0) ? -psl->nObject : psl->nSegment);
			glLightfv (hLight, GL_POSITION, (GLfloat *) (psl->pos));
			glLightfv (hLight, GL_DIFFUSE, (GLfloat *) &(psl->color));
			glLightfv (hLight, GL_SPECULAR, (GLfloat *) &(psl->color));
			glLightf (hLight, GL_CONSTANT_ATTENUATION, 0.1f / psl->brightness);
			glLightf (hLight, GL_LINEAR_ATTENUATION, 0.1f / psl->brightness);
			glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.01f / psl->brightness);
			if (psl->bSpot) {
#if 0
				psl = psl;
#else
				glLighti (hLight, GL_SPOT_EXPONENT, 12);
				glLighti (hLight, GL_SPOT_CUTOFF, 25);
				glLightfv (hLight, GL_SPOT_DIRECTION, (GLfloat *) &psl->dir);
#endif
				}
			}
		OglResetTransform (1);
		for (; iLight < 8; iLight++)
			glDisable (GL_LIGHT0 + iLight);
		}
	if (nSubModel < 0)
		G3StartInstanceMatrix (&posP->vPos, &posP->mOrient);
	pm = gameData.models.g3Models [bHires] + nModel;
	if (bHires) {
		int i;
		for (i = 0; i < pm->nSubModels; i++)
			if (pm->pSubModels [i].nParent == -1) 
				G3DrawSubModel (objP, nModel, i, nSubModel, modelBitmaps, pAnimAngles, (nSubModel < 0) ? &pm->pSubModels->vOffset : vOffset, 
									 bHires, bUseVBO, nPass, bTransparency, nGunId, nBombId, nMissileId, nMissiles);
		}
	else
		G3DrawSubModel (objP, nModel, 0, nSubModel, modelBitmaps, pAnimAngles, (nSubModel < 0) ? &pm->pSubModels->vOffset : vOffset,
							 bHires, bUseVBO, nPass, bTransparency, nGunId, nBombId, nMissileId, nMissiles);
	if (nSubModel < 0)
		G3DoneInstance ();
	if (!bLighting)
		break;
	}
if (bLighting) {
	OglDisableLighting ();
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (1);
	}
//HUDMessage (0, "%s", szLightSources);
}

//------------------------------------------------------------------------------

void G3RenderDamageLightnings (tObject *objP, short nModel, short nSubModel, 
										 vmsAngVec *pAnimAngles, vmsVector *vOffset, int bHires)
{
	tG3Model			*pm;
	tG3SubModel		*psm;
	tG3ModelFace	*pmf;
	vmsAngVec		*va;
	vmsVector		vo;
	int				i, j;

pm = gameData.models.g3Models [bHires] + nModel;
if (pm->bValid < 1) {
	if (!bHires)
		return;
	pm = gameData.models.g3Models [0] + nModel;
	if (pm->bValid < 1)
		return;
	}
psm = pm->pSubModels + nSubModel;
va = pAnimAngles ? pAnimAngles + psm->nAngles : &avZero;
if (!(SHOW_LIGHTNINGS && gameOpts->render.lightnings.bDamage))
	return;
if (!objP || (ObjectDamage (objP) > 0.5f))
	return;
// set the translation
vo = psm->vOffset;
if (gameData.models.nScale)
	VmVecScale (&vo, gameData.models.nScale);
if (vOffset) {
	G3StartInstanceAngles (&vo, va);
	VmVecInc (&vo, vOffset);
	}
// render any dependent submodels
for (i = 0, j = pm->nSubModels, psm = pm->pSubModels; i < j; i++, psm++)
	if (psm->nParent == nSubModel)
		G3RenderDamageLightnings (objP, nModel, i, pAnimAngles, &vo, bHires);
// render the lightnings
for (psm = pm->pSubModels + nSubModel, i = psm->nFaces, pmf = psm->pFaces; i; i--, pmf++) 
	RenderDamageLightnings (objP, NULL, pm->pFaceVerts + pmf->nIndex, pmf->nVerts);
if (vOffset)
	G3DoneInstance ();
}

//------------------------------------------------------------------------------

int G3RenderModel (tObject *objP, short nModel, short nSubModel, tPolyModel *pp, grsBitmap **modelBitmaps, 
						 vmsAngVec *pAnimAngles, vmsVector *vOffset, fix xModelLight, fix *xGlowValues, tRgbaColorf *pObjColor)
{
	tG3Model	*pm = gameData.models.g3Models [1] + nModel;
	int		i, bHires = 1, bUseVBO = gameStates.ogl.bHaveVBOs && gameOpts->ogl.bObjLighting;
	int		nGunId, nBombId, nMissileId, nMissiles;

if (!objP)
	return 0;
if (gameStates.render.bQueryCoronas && gameStates.render.bCloaked)
	return 0;
#if G3_FAST_MODELS
if (!gameOpts->render.nPath)
#endif
	{
	gameData.models.g3Models [0][nModel].bValid =
	gameData.models.g3Models [1][nModel].bValid = -1;
	return 0;
	}
if (pm->bValid < 1) {
	if (pm->bValid) {
		i = 0;
		bHires = 0;
		}
	else {
		i = G3BuildModel (objP, nModel, pp, modelBitmaps, pObjColor, 1);
		if (i < 0)	//successfully built new model
			return gameStates.render.bBuildModels;
		pm->bValid = -1;
		}
	pm = gameData.models.g3Models [0] + nModel;
	if (pm->bValid < 0)
		return 0;
	if (!(i || pm->bValid)) {
		i = G3BuildModel (objP, nModel, pp, modelBitmaps, pObjColor, 0);
		if (i <= 0) {
			if (!i)
				pm->bValid = -1;
			return gameStates.render.bBuildModels;
			}
		}
	}
if (!(gameStates.render.bCloaked ? 
	   G3EnableClientStates (0, 0, 0, GL_TEXTURE0) : 
		G3EnableClientStates (1, 1, gameOpts->ogl.bObjLighting, GL_TEXTURE0)))
	return 0;
if (bUseVBO) {
	int i;
	glBindBuffer (GL_ARRAY_BUFFER_ARB, pm->vboDataHandle);
	if ((i = glGetError ())) {
		glBindBuffer (GL_ARRAY_BUFFER_ARB, pm->vboDataHandle);
		if ((i = glGetError ()))
			return 0;
		}
	}
else {
	pm->pVBVerts = (fVector3 *) pm->pVertBuf [0];
	pm->pVBNormals = pm->pVBVerts + pm->nFaceVerts;
	pm->pVBColor = (tRgbaColorf *) (pm->pVBNormals + pm->nFaceVerts);
	pm->pVBTexCoord = (tTexCoord2f *) (pm->pVBColor + pm->nFaceVerts);
	}
#if G3_SW_SCALING
G3ScaleModel (nModel);
#else
#	if 0
if (bHires)
	gameData.models.nScale = 0;
#	endif
#endif
if (!(gameOpts->ogl.bObjLighting || gameStates.render.bQueryCoronas || gameStates.render.bCloaked))
	G3LightModel (objP, nModel, xModelLight, xGlowValues, bHires);
if (bUseVBO) {
	if (!gameStates.render.bCloaked) {
		glNormalPointer (GL_FLOAT, 0, G3_BUFFER_OFFSET (pm->nFaceVerts * sizeof (fVector3)));
		glColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (pm->nFaceVerts * 2 * sizeof (fVector3)));
		glTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (pm->nFaceVerts * ((2 * sizeof (fVector3) + sizeof (tRgbaColorf)))));
		}
	glVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (0));
	if (pm->vboIndexHandle)
		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER_ARB, pm->vboIndexHandle);
	}
else 
	{
	if (!gameStates.render.bCloaked) {
		glTexCoordPointer (2, GL_FLOAT, 0, pm->pVBTexCoord);
		if (gameOpts->ogl.bObjLighting)
			glNormalPointer (GL_FLOAT, 0, pm->pVBNormals);
		glColorPointer (4, GL_FLOAT, 0, pm->pVBColor);
		}
	glVertexPointer (3, GL_FLOAT, 0, pm->pVBVerts);
	}
nGunId = EquippedPlayerGun (objP);
nBombId = EquippedPlayerBomb (objP);
nMissileId = EquippedPlayerMissile (objP, &nMissiles);
G3DrawModel (objP, nModel, nSubModel, modelBitmaps, pAnimAngles, vOffset, bHires, bUseVBO, 0,
				 nGunId, nBombId, nMissileId, nMissiles);
if ((objP->nType != OBJ_DEBRIS) && bHires && pm->bHasTransparency)
	G3DrawModel (objP, nModel, nSubModel, modelBitmaps, pAnimAngles, vOffset, bHires, bUseVBO, 1,
					 nGunId, nBombId, nMissileId, nMissiles);
glDisable (GL_TEXTURE_2D);
glBindBuffer (GL_ARRAY_BUFFER_ARB, 0);
glBindBuffer (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
if (gameStates.render.bCloaked)
	G3DisableClientStates (0, 0, 0, -1);
else
	G3DisableClientStates (1, 1, gameOpts->ogl.bObjLighting, -1);
if (objP && ((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_ROBOT) || (objP->nType == OBJ_REACTOR))) {
	G3StartInstanceMatrix (&objP->position.vPos, &objP->position.mOrient);
	G3RenderDamageLightnings (objP, nModel, 0, pAnimAngles, NULL, bHires);
	G3DoneInstance ();
	}
pm->bRendered = 1;
return 1;
}

//------------------------------------------------------------------------------
//eof
