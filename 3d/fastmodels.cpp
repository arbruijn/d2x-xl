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
#include "weapon.h"

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

//------------------------------------------------------------------------------

void G3DynLightModel (CObject *objP, RenderModel::CModel *pm, short iVerts, short nVerts, short iFaceVerts, short nFaceVerts)
{
	CFloatVector				vPos, vVertex;
	CFloatVector3*				pv, * pn;
	RenderModel::CVertex*	pmv;
	tFaceColor*					pc;
	float							fAlpha = GrAlpha ();
	int							h, i, 
									bEmissive = (objP->info.nType == OBJ_WEAPON) && 
													gameData.objs.bIsWeapon [objP->info.nId] && 
													!gameData.objs.bIsMissile [objP->info.nId];

if (!gameStates.render.bBrightObject) {
	vPos.Assign (objP->info.position.vPos);
	for (i = iVerts, pv = pm->m_verts + iVerts, pn = pm->m_vertNorms + iVerts, pc = pm->m_color + iVerts;
		  i < nVerts;
		  i++, pv++, pn++, pc++) {
		pc->index = 0;
		vVertex = vPos + *reinterpret_cast<CFloatVector*> (pv);
		G3VertexColor (reinterpret_cast<CFloatVector3*> (pn), vVertex.V3(), i, pc, NULL, 1, 0, 0);
		}
	}
for (i = iFaceVerts, h = iFaceVerts, pmv = pm->m_faceVerts + iFaceVerts; i < nFaceVerts; i++, h++, pmv++) {
	if (gameStates.render.bBrightObject || bEmissive) {
		pm->m_vbColor [h] = pmv->m_baseColor;
		pm->m_vbColor [h].alpha = fAlpha;
		}
	else if (pmv->m_bTextured)
		pm->m_vbColor [h] = pm->m_color [pmv->m_nIndex].color;
	else {
		pc = pm->m_color + pmv->m_nIndex;
		pm->m_vbColor [h].red = pmv->m_baseColor.red * pc->color.red;
		pm->m_vbColor [h].green = pmv->m_baseColor.green * pc->color.green;
		pm->m_vbColor [h].blue = pmv->m_baseColor.blue * pc->color.blue;
		pm->m_vbColor [h].alpha = pmv->m_baseColor.alpha;
		}
	}
}

//------------------------------------------------------------------------------

void G3LightModel (CObject *objP, int nModel, fix xModelLight, fix *xGlowValues, int bHires)
{
	RenderModel::CModel*		pm = gameData.models.renderModels [bHires] + nModel;
	RenderModel::CVertex*	pmv;
	RenderModel::CFace*		pmf;
	tRgbaColorf					baseColor, *colorP;
	float							fLight, fAlpha = (float) gameStates.render.grAlpha / (float) FADE_LEVELS;
	int							h, i, j, l;
	int							bEmissive = (objP->info.nType == OBJ_MARKER) ||
													((objP->info.nType == OBJ_WEAPON) && 
													 gameData.objs.bIsWeapon [objP->info.nId] && 
													 !gameData.objs.bIsMissile [objP->info.nId]);

#if DBG
if (objP->Index () == nDbgObj)
	objP = objP;
#endif
#if 0
if (xModelLight > I2X (1))
	xModelLight = I2X (1);
#endif
if (SHOW_DYN_LIGHT && (gameOpts->ogl.bObjLighting ||
    (gameOpts->ogl.bLightObjects && (gameOpts->ogl.bLightPowerups || (objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT))))) {
	tiRender.objP = objP;
	tiRender.pm = pm;
	if (!RunRenderThreads (rtPolyModel))
		G3DynLightModel (objP, pm, 0, pm->m_nVerts, 0, pm->m_nFaceVerts);
	}
else {
	if (gameData.objs.color.index)
		baseColor = gameData.objs.color.color;
	else
		baseColor.red = baseColor.green = baseColor.blue = 1;
	baseColor.alpha = fAlpha;
	for (i = pm->m_nFaces, pmf = pm->m_faces.Buffer (); i; i--, pmf++) {
		if (pmf->m_nBitmap >= 0)
			colorP = &baseColor;
		else
			colorP = NULL;
		if (pmf->m_bGlow)
			l = xGlowValues [nGlow];
		else if (bEmissive)
			l = I2X (1);
		else {
			l = -CFixVector::Dot (transformation.m_info.view [0].FVec (), pmf->m_vNormal);
			l = 3 * I2X (1) / 4 + l / 4;
			l = FixMul (l, xModelLight);
			}
		fLight = X2F (l);
		for (j = pmf->m_nVerts, h = pmf->m_nIndex, pmv = pm->m_faceVerts + pmf->m_nIndex; j; j--, h++, pmv++) {
#if G3_DRAW_ARRAYS
			if (colorP) {
				pm->m_vbColor [h].red = colorP->red * fLight;
				pm->m_vbColor [h].green = colorP->green * fLight;
				pm->m_vbColor [h].blue = colorP->blue * fLight;
				pm->m_vbColor [h].alpha = fAlpha;
				}
			else {
				pm->m_vbColor [h].red = pmv->m_baseColor.red * fLight;
				pm->m_vbColor [h].green = pmv->m_baseColor.green * fLight;
				pm->m_vbColor [h].blue = pmv->m_baseColor.blue * fLight;
				pm->m_vbColor [h].alpha = fAlpha;
				}
#else
			if (colorP) {
				pmv->m_renderColor.red = colorP->red * fLight;
				pmv->m_renderColor.green = colorP->green * fLight;
				pmv->m_renderColor.blue = colorP->blue * fLight;
				pmv->m_renderColor.alpha = fAlpha;
				}
			else {
				pmv->m_renderColor.red = pmv->m_baseColor.red * fLight;
				pmv->m_renderColor.green = pmv->m_baseColor.green * fLight;
				pmv->m_renderColor.blue = pmv->m_baseColor.blue * fLight;
				pmv->m_renderColor.alpha = fAlpha;
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
	RenderModel::CModel			*pm = gameData.models.renderModels [bHires] + nModel;
	CFloatVector			fScale;
	int				i;
	CFloatVector3			*pv;
	RenderModel::CVertex	*pmv;

if (gameData.models.vScale.IsZero ())
	fScale.Create (1,1,1);
else
	fScale.Assign (gameData.models.vScale);
if (pm->m_fScale == fScale)
	return;
fScale /= pm->m_fScale;
for (i = pm->m_nVerts, pv = pm->m_verts; i; i--, pv++) {
	pv->p.x *= fScale;
	pv->p.y *= fScale;
	pv->p.z *= fScale;
	}
for (i = pm->m_nFaceVerts, pmv = pm->m_faceVerts; i; i--, pmv++)
	pmv->m_vertex = pm->m_verts [pmv->m_nIndex];
pm->m_fScale *= fScale;
}

#endif

//------------------------------------------------------------------------------

void G3GetThrusterPos (CObject *objP, short nModel, RenderModel::CFace *pmf, CFixVector *vOffsetP,
							  CFixVector *vNormal, int nRad, int bHires)
{
	RenderModel::CModel				*pm = gameData.models.renderModels [bHires] + nModel;
	RenderModel::CVertex		*pmv = NULL;
	CFloatVector3				v = CFloatVector3::ZERO, vn, vo, vForward = CFloatVector3::Create(0,0,1);
	tModelThrusters	*mtP = gameData.models.thrusters + nModel;
	int					i, j = 0;
	float					h, nSize;

if (!objP)
	return;
if (!pm->m_bRendered || !gameData.models.vScale.IsZero ())
	mtP->nCount = 0;
else if (mtP->nCount >= (((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT)) ? 2 : 1))
	return;
vn.Assign (pmf ? pmf->m_vNormal : *vNormal);
if (CFloatVector3::Dot (vn, vForward) > -1.0f / 3.0f)
	return;
if (pmf) {
	for (i = 0, j = pmf->m_nVerts, pmv = pm->m_faceVerts + pmf->m_nIndex; i < j; i++)
		v += pmv [i].m_vertex;
	v[X] /= j;
	v[Y] /= j;
	v[Z] /= j;
	}
else
	v.SetZero ();
v[Z] -= 1.0f / 16.0f;
#if 0
transformation.Transform (&v, &v, 0);
#else
#	if 1
if (vOffsetP) {
	vo.Assign (*vOffsetP);
	v += vo;
	}
#	endif
#endif
if (mtP->nCount && (v[X] == mtP->vPos [0][X]) && (v[Y] == mtP->vPos [0][Y]) && (v[Z] == mtP->vPos [0][Z]))
	return;
mtP->vPos [mtP->nCount][X] = F2X (v[X]);
mtP->vPos [mtP->nCount][Y] = F2X (v[Y]);
mtP->vPos [mtP->nCount][Z] = F2X (v[Z]);
if (vOffsetP)
	v -= vo;
mtP->vDir [mtP->nCount] = *vNormal;
mtP->vDir [mtP->nCount] = -mtP->vDir [mtP->nCount];
if (!mtP->nCount) {
	if (!pmf)
		mtP->fSize = X2F (nRad);
	else {
		for (i = 0, nSize = 1000000000; i < j; i++)
			if (nSize > (h = CFloatVector3::Dist (v, pmv [i].m_vertex)))
				nSize = h;
		mtP->fSize = nSize;// * 1.25f;
		}
	}
mtP->nCount++;
}

//------------------------------------------------------------------------------

static int bCenterGuns [] = {0, 1, 1, 0, 0, 0, 1, 1, 0, 1};

int G3FilterSubModel (CObject *objP, RenderModel::CSubModel *psm, int nGunId, int nBombId, int nMissileId, int nMissiles)
{
	int nId = objP->info.nId;

if (!psm->m_bRender)
	return 0;
if (psm->m_nGunPoint >= 0)
	return 1;
if (psm->m_bBullets)
	return 1;
if (psm->m_bWeapon) {
	CPlayerData	*playerP = gameData.multiplayer.players + nId;
	int		bLasers = (nGunId == LASER_INDEX) || (nGunId == SUPER_LASER_INDEX);
	int		bSuperLasers = playerP->laserLevel > MAX_LASER_LEVEL;
	int		bQuadLasers = gameData.multiplayer.weaponStates [gameData.multiplayer.nLocalPlayer].bQuadLasers;
	int		bCenter = bCenterGuns [nGunId];
	int		nWingtip = gameOpts->render.ship.nWingtip;

	if (nWingtip == 0)
		nWingtip = bLasers && bSuperLasers && bQuadLasers;
	else if (nWingtip == 1)
		nWingtip = !bLasers || bSuperLasers;

	if (EGI_FLAG (bShowWeapons, 0, 1, 0)) {
		if (psm->m_nGun == nGunId + 1) {
			if (psm->m_nGun == FUSION_INDEX + 1) {
				if ((psm->m_nWeaponPos == 3) && !gameData.multiplayer.weaponStates [gameData.multiplayer.nLocalPlayer].bTripleFusion)
					return 1;
				}
			else if (bLasers) {
				if ((psm->m_nWeaponPos > 2) && !bQuadLasers && (nWingtip != bSuperLasers))
					return 1;
				}
			}
		else if (psm->m_nGun == LASER_INDEX + 1) {
			if (nWingtip)
				return 1;
			return !bCenter && (psm->m_nWeaponPos < 3);
			}
		else if (psm->m_nGun == SUPER_LASER_INDEX + 1) {
			if (nWingtip != 1)
				return 1;
			return !bCenter && (psm->m_nWeaponPos < 3);
			}
		else if (!psm->m_nGun) {
			if (bLasers && bQuadLasers)
				return 1;
			if (psm->m_nType != gameOpts->render.ship.nWingtip)
				return 1;
			return 0;
			}
		else if (psm->m_nBomb == nBombId)
			return (nId == gameData.multiplayer.nLocalPlayer) && !AllowedToFireMissile (nId, 0);
		else if (psm->m_nMissile == nMissileId) {
			if (psm->m_nWeaponPos > nMissiles)
				return 1;
			else {
				static int nMslPos [] = {-1, 1, 0, 3, 2};
				int nLaunchPos = gameData.multiplayer.weaponStates [nId].nMslLaunchPos;
				return (nId == gameData.multiplayer.nLocalPlayer) && !AllowedToFireMissile (nId, 0) &&
						 (nLaunchPos == (nMslPos [(int) psm->m_nWeaponPos]));
				}
			}
		else
			return 1;
		}
	else {
		if (psm->m_nGun == 1)
			return 0;
		if ((psm->m_nGun < 0) && (psm->m_nMissile == 1))
			return 0;
		return 1;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

static inline int ObjectHasThruster (CObject *objP)
{
return (objP->info.nType == OBJ_PLAYER) ||
		 (objP->info.nType == OBJ_ROBOT) ||
		 ((objP->info.nType == OBJ_WEAPON) && (gameData.objs.bIsMissile [objP->info.nId]));
}

//------------------------------------------------------------------------------

int G3AnimateSubModel (CObject *objP, RenderModel::CSubModel *psm, short nModel)
{
	tFiringData	*fP;
	float			nTimeout, y;
	int			nDelay;

if (!psm->m_nFrames)
	return 0;
fP = gameData.multiplayer.weaponStates [objP->info.nId].firing;
nTimeout = 25 * gameStates.gameplay.slowmo [0].fSpeed;
if (gameData.weapons.nPrimary == VULCAN_INDEX)
	nTimeout /= 2;
if (fP->nStop > 0) {
	nDelay = gameStates.app.nSDLTicks - fP->nStop;
	if (nDelay > GATLING_DELAY)
		return 0;
	nTimeout += (float) nDelay / 10;
	}
else if (fP->nStart > 0) {
	nDelay = GATLING_DELAY - fP->nDuration;
	if (nDelay > 0)
		nTimeout += (float) nDelay / 10;
	}
else
	return 0;
if (gameStates.app.nSDLTicks - psm->m_tFrame > nTimeout) {
	psm->m_tFrame = gameStates.app.nSDLTicks;
	psm->m_iFrame = ++psm->m_iFrame % psm->m_nFrames;
	}
glPushMatrix ();
y = X2F (psm->m_vCenter[Y]);
glTranslatef (0, y, 0);
glRotatef (360 * (float) psm->m_iFrame / (float) psm->m_nFrames, 0, 0, 1);
glTranslatef (0, -y, 0);
return 1;
}

//------------------------------------------------------------------------------

void G3DrawSubModel (CObject *objP, short nModel, short nSubModel, short nExclusive, CBitmap **modelBitmaps,
						   CAngleVector *pAnimAngles, CFixVector *vOffsetP, int bHires, int bUseVBO, int nPass, int bTransparency,
							int nGunId, int nBombId, int nMissileId, int nMissiles)
{
	RenderModel::CModel			*pm = gameData.models.renderModels [bHires] + nModel;
	RenderModel::CSubModel		*psm = pm->m_subModels + nSubModel;
	RenderModel::CFace	*pmf;
	CBitmap		*bmP = NULL;
	CAngleVector		va = pAnimAngles ? pAnimAngles [psm->m_nAngles] : CAngleVector::ZERO;
	CFixVector		vo;
	int				h, i, j, bTransparent, bAnimate, bTextured = !(gameStates.render.bCloaked /*|| nPass*/),
						bGetThruster = !nPass && ObjectHasThruster (objP);
	short				nId, nFaceVerts, nVerts, nIndex, nBitmap = -1, nTeamColor;

if (objP->info.nType == OBJ_PLAYER)
	nTeamColor = IsMultiGame ? (IsTeamGame ? GetTeam (objP->info.nId) : objP->info.nId) + 1 : gameOpts->render.ship.nColor;
else
	nTeamColor = 0;
#if 1
if (psm->m_bThruster) {
	if (!nPass) {
		vo = psm->m_vOffset + psm->m_vCenter;
		G3GetThrusterPos (objP, nModel, NULL, &vo, &psm->m_faces->m_vNormal, psm->m_nRad, bHires);
	}
	return;
	}
if (G3FilterSubModel (objP, psm, nGunId, nBombId, nMissileId, nMissiles))
	return;
#endif
vo = psm->m_vOffset;
if (!gameData.models.vScale.IsZero ())
	vo *= gameData.models.vScale;
#if 1
if (vOffsetP && (nExclusive < 0)) {
	transformation.Begin (vo, va);
	vo += *vOffsetP;
	}
#endif
bAnimate = G3AnimateSubModel (objP, psm, nModel);
#if G3_DRAW_SUBMODELS
// render any dependent submodels
for (i = 0, j = pm->m_nSubModels, psm = pm->m_subModels.Buffer (); i < j; i++, psm++)
	if (psm->m_nParent == nSubModel)
		G3DrawSubModel (objP, nModel, i, nExclusive, modelBitmaps, pAnimAngles, &vo, bHires,
							 bUseVBO, nPass, bTransparency, nGunId, nBombId, nMissileId, nMissiles);
#endif
// render the faces
if ((nExclusive < 0) || (nSubModel == nExclusive)) {
#if 0
	if (vOffsetP && (nSubModel == nExclusive))
		transformation.Begin (vOffsetP, NULL);
#endif
	glDisable (GL_TEXTURE_2D);
	if (gameStates.render.bCloaked)
		glColor4f (0, 0, 0, GrAlpha ());
	for (psm = pm->m_subModels + nSubModel, i = psm->m_nFaces, pmf = psm->m_faces; i; ) {
		if (bTextured && (nBitmap != pmf->m_nBitmap)) {
			if (0 > (nBitmap = pmf->m_nBitmap))
				glDisable (GL_TEXTURE_2D);
			else {
				if (!bHires)
					bmP = modelBitmaps [nBitmap];
				else {
					bmP = pm->m_textures + nBitmap;
					if (nTeamColor && bmP->Team () && (0 <= (h = pm->m_teamTextures [nTeamColor % MAX_PLAYERS]))) {
						nBitmap = h;
						bmP = pm->m_textures + nBitmap;
						}
					}
				glActiveTexture (GL_TEXTURE0);
				glClientActiveTexture (GL_TEXTURE0);
				glEnable (GL_TEXTURE_2D);
				bmP = bmP->Override (-1);
				if (bmP->Frames ())
					bmP = bmP->CurFrame ();
				if (bmP->Bind (1, 3))
					continue;
				bmP->Texture ()->Wrap (GL_REPEAT);
				}
			}
		nIndex = pmf->m_nIndex;
		if (bHires) {
			bTransparent = bmP && ((bmP->Flags () & BM_FLAG_TRANSPARENT) != 0);
			if (bTransparent != bTransparency) {
				if (bTransparent)
					pm->m_bHasTransparency = 1;
				pmf++;
				i--;
				continue;
				}
			}
		if ((nFaceVerts = pmf->m_nVerts) > 4) {
			if (bGetThruster && pmf->m_bThruster)
				G3GetThrusterPos (objP, nModel, pmf, &vo, &pmf->m_vNormal, 0, bHires);
			nVerts = nFaceVerts;
			pmf++;
			i--;
			}
		else {
			nId = pmf->m_nId;
			nVerts = 0;
			do {
				if (bGetThruster && pmf->m_bThruster)
					G3GetThrusterPos (objP, nModel, pmf, &vo, &pmf->m_vNormal, 0, bHires);
				nVerts += nFaceVerts;
				pmf++;
				i--;
				} while (i && (pmf->m_nId == nId));
			}
#ifdef _WIN32
		if (glDrawRangeElements)
#endif
			if (bUseVBO)
				glDrawRangeElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
											0, nVerts - 1, nVerts, GL_UNSIGNED_SHORT,
											G3_BUFFER_OFFSET (nIndex * sizeof (short)));
			else
				glDrawRangeElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
											nIndex, nIndex + nVerts - 1, nVerts, GL_UNSIGNED_SHORT,
											pm->m_index [0] + nIndex);
#ifdef _WIN32
		else
			if (bUseVBO)
				glDrawElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
									 nVerts, GL_UNSIGNED_SHORT, G3_BUFFER_OFFSET (nIndex * sizeof (short)));
			else
				glDrawElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
									 nVerts, GL_UNSIGNED_SHORT, pm->m_index + nIndex);
#endif
		}
	}
if (bAnimate)
	glPopMatrix ();
#if 1
if ((nExclusive < 0) /*|| (nSubModel == nExclusive)*/)
	transformation.End ();
#endif
}

//------------------------------------------------------------------------------

void G3DrawModel (CObject *objP, short nModel, short nSubModel, CBitmap **modelBitmaps,
						CAngleVector *pAnimAngles, CFixVector *vOffsetP, int bHires, int bUseVBO, int bTransparency,
						int nGunId, int nBombId, int nMissileId, int nMissiles)
{
	RenderModel::CModel*	pm;
	CDynLightIndex*		sliP = &lightManager.Index (0)[0];
	CActiveDynLight*		activeLightsP = lightManager.Active (0) + sliP->nFirst;
	CDynLight*				prl;
	int						nPass, iLight, nLights, nLightRange;
	int						bBright = objP && (objP->info.nType == OBJ_MARKER);
	int						bEmissive = objP && (objP->info.nType == OBJ_WEAPON) && 
												gameData.objs.bIsWeapon [objP->info.nId] && !gameData.objs.bIsMissile [objP->info.nId];
	int						bLighting = SHOW_DYN_LIGHT && gameOpts->ogl.bObjLighting && 
												!(gameStates.render.bQueryCoronas || gameStates.render.bCloaked || bEmissive || bBright);
	GLenum					hLight;
	float						fBrightness, fLightScale = gameData.models.nLightScale ? X2F (gameData.models.nLightScale) : 1.0f;
	CFloatVector			color;
	tObjTransformation*	posP = OBJPOS (objP);

OglSetupTransform (1);
if (bLighting) {
	nLights = sliP->nActive;
	if (nLights > gameStates.render.nMaxLightsPerObject)
		nLights = gameStates.render.nMaxLightsPerObject;
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

if (sliP->nLast < 0)
	nLightRange = 0;
else
	nLightRange = sliP->nLast - sliP->nFirst + 1;
for (nPass = 0; ((nLightRange > 0) && (nLights > 0)) || !nPass; nPass++) {
	if (bLighting) {
		if (nPass) {
			glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
			glDepthMask (0);
			}
		for (iLight = 0; (nLightRange > 0) && (iLight < 8) && nLights; activeLightsP++, nLightRange--) {
#if DBG
			if (activeLightsP - lightManager.Active (0) >= MAX_SHADER_LIGHTS)
				break;
			if (activeLightsP < lightManager.Active (0))
				break;
#endif
			if (nLights < 0) {
				tFaceColor *psc = lightManager.AvgSgmColor (objP->info.nSegment, NULL);
				CFloatVector3 vPos;
				hLight = GL_LIGHT0 + iLight++;
				glEnable (hLight);
				vPos.Assign (objP->info.position.vPos);
				glLightfv (hLight, GL_POSITION, reinterpret_cast<GLfloat*> (&vPos));
				glLightfv (hLight, GL_DIFFUSE, reinterpret_cast<GLfloat*> (&psc->color));
				glLightfv (hLight, GL_SPECULAR, reinterpret_cast<GLfloat*> (&psc->color));
				glLightf (hLight, GL_CONSTANT_ATTENUATION, 0.1f);
				glLightf (hLight, GL_LINEAR_ATTENUATION, 0.1f);
				glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.01f);
				nLights = 0;
				}
			else if ((prl = lightManager.GetActive (activeLightsP, 0))) {
				hLight = GL_LIGHT0 + iLight++;
				glEnable (hLight);
	//			sprintf (szLightSources + strlen (szLightSources), "%d ", (prl->nObject >= 0) ? -prl->nObject : prl->nSegment);
				fBrightness = prl->info.fBrightness * fLightScale;
				color = *(reinterpret_cast<CFloatVector*> (&prl->info.color));
				color[R] *= fLightScale;
				color[G] *= fLightScale;
				color[B] *= fLightScale;
				glLightfv (hLight, GL_POSITION, reinterpret_cast<GLfloat*> (prl->render.vPosf));
				glLightfv (hLight, GL_DIFFUSE, reinterpret_cast<GLfloat*> (&color));
				glLightfv (hLight, GL_SPECULAR, reinterpret_cast<GLfloat*> (&color));
				if (prl->info.bSpot) {
#if 0
					prl = prl;
#else
					glLighti (hLight, GL_SPOT_EXPONENT, 12);
					glLighti (hLight, GL_SPOT_CUTOFF, 25);
					glLightfv (hLight, GL_SPOT_DIRECTION, reinterpret_cast<GLfloat*> (&prl->info.vDirf));
#endif
					glLightf (hLight, GL_CONSTANT_ATTENUATION, 0.1f / fBrightness);
					glLightf (hLight, GL_LINEAR_ATTENUATION, 0.01f / fBrightness);
					glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.001f / fBrightness);
					}
				else {
					glLightf (hLight, GL_CONSTANT_ATTENUATION, 0.1f / fBrightness);
					glLightf (hLight, GL_LINEAR_ATTENUATION, 0.1f / fBrightness);
					glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.01f / fBrightness);
					}
				nLights--;
				}
			}
		for (; iLight < 8; iLight++)
			glDisable (GL_LIGHT0 + iLight);
		}
	//if (nSubModel < 0)
		transformation.Begin(posP->vPos, posP->mOrient);
	pm = gameData.models.renderModels [bHires] + nModel;
	if (bHires) {
		for (int i = 0; i < pm->m_nSubModels; i++)
			if (pm->m_subModels [i].m_nParent == -1)
				G3DrawSubModel (objP, nModel, i, nSubModel, modelBitmaps, pAnimAngles, (nSubModel < 0) ? &pm->m_subModels [0].m_vOffset : vOffsetP,
									 bHires, bUseVBO, nPass, bTransparency, nGunId, nBombId, nMissileId, nMissiles);
		}
	else
		G3DrawSubModel (objP, nModel, 0, nSubModel, modelBitmaps, pAnimAngles, (nSubModel < 0) ? &pm->m_subModels [0].m_vOffset : vOffsetP,
							 bHires, bUseVBO, nPass, bTransparency, nGunId, nBombId, nMissileId, nMissiles);
	//if (nSubModel < 0)
		transformation.End ();
	if (!bLighting)
		break;
	}
#if DBG
if (!nLightRange && nLights)
	nLights = 0;
#endif
if (bLighting) {
	OglDisableLighting ();
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (1);
	}
OglResetTransform (1);
//HUDMessage (0, "%s", szLightSources);
}

//------------------------------------------------------------------------------

void G3RenderDamageLightnings (CObject *objP, short nModel, short nSubModel,
										 CAngleVector *pAnimAngles, CFixVector *vOffsetP, int bHires)
{
	RenderModel::CModel			*pm;
	RenderModel::CSubModel		*psm;
	RenderModel::CFace	*pmf;
	const CAngleVector	*va;
	CFixVector		vo;
	int				i, j;

pm = gameData.models.renderModels [bHires] + nModel;
if (pm->m_bValid < 1) {
	if (!bHires)
		return;
	pm = gameData.models.renderModels [0] + nModel;
	if (pm->m_bValid < 1)
		return;
	}
psm = pm->m_subModels + nSubModel;
va = pAnimAngles ? pAnimAngles + psm->m_nAngles : &CAngleVector::ZERO;
if (!(SHOW_LIGHTNINGS && gameOpts->render.lightnings.bDamage))
	return;
if (!objP || (ObjectDamage (objP) > 0.5f))
	return;
// set the translation
vo = psm->m_vOffset;
if (!gameData.models.vScale.IsZero ())
	vo *= gameData.models.vScale;
if (vOffsetP) {
	transformation.Begin(vo, *va);
	vo += *vOffsetP;
	}
// render any dependent submodels
for (i = 0, j = pm->m_nSubModels, psm = pm->m_subModels.Buffer (); i < j; i++, psm++)
	if (psm->m_nParent == nSubModel)
		G3RenderDamageLightnings (objP, nModel, i, pAnimAngles, &vo, bHires);
// render the lightnings
for (psm = pm->m_subModels + nSubModel, i = psm->m_nFaces, pmf = psm->m_faces; i; i--, pmf++)
	lightningManager.RenderForDamage (objP, NULL, pm->m_faceVerts + pmf->m_nIndex, pmf->m_nVerts);
if (vOffsetP)
	transformation.End ();
}

//------------------------------------------------------------------------------

int G3RenderModel (CObject *objP, short nModel, short nSubModel, CPolyModel* pp, CBitmap **modelBitmaps,
						 CAngleVector *pAnimAngles, CFixVector *vOffsetP, fix xModelLight, fix *xGlowValues, tRgbaColorf *pObjColor)
{
	RenderModel::CModel	*pm = gameData.models.renderModels [1] + nModel;
	int		i, bHires = 1, bUseVBO = gameStates.ogl.bHaveVBOs && ((gameStates.render.bPerPixelLighting == 2) || gameOpts->ogl.bObjLighting),
				nGunId, nBombId, nMissileId, nMissiles;

if (!objP)
	return 0;
#if 1//def _DEBUG
if (nModel == nDbgModel)
	nDbgModel = nModel;
#endif
if (gameStates.render.bQueryCoronas &&
	 (((objP->info.nType == OBJ_WEAPON) && (objP->info.nId < MAX_WEAPONS) &&
	  gameData.objs.bIsWeapon [objP->info.nId] && !gameData.objs.bIsMissile [objP->info.nId]) || gameStates.render.bCloaked))
	return 1;
#if G3_FAST_MODELS
if (!RENDERPATH)
#endif
 {
	gameData.models.renderModels [0][nModel].m_bValid =
	gameData.models.renderModels [1][nModel].m_bValid = -1;
	return 0;
	}
if (pm->m_bValid < 1) {
	if (pm->m_bValid) {
		i = 0;
		bHires = 0;
		}
	else {
		if (IsDefaultModel (nModel)) {
			if (bUseVBO && pm->m_bValid && !(pm->m_vboDataHandle && pm->m_vboIndexHandle))
				pm->m_bValid = 0;
			i = G3BuildModel (objP, nModel, pp, modelBitmaps, pObjColor, 1);
			if (i < 0)	//successfully built new model
				return gameStates.render.bBuildModels;
			}
		else
			i = 0;
		pm->m_bValid = -1;
		}
	pm = gameData.models.renderModels [0] + nModel;
	if (pm->m_bValid < 0)
		return 0;
	if (bUseVBO && pm->m_bValid && !(pm->m_vboDataHandle && pm->m_vboIndexHandle))
		pm->m_bValid = 0;
	if (!(i || pm->m_bValid)) {
		i = G3BuildModel (objP, nModel, pp, modelBitmaps, pObjColor, 0);
		if (i <= 0) {
			if (!i)
				pm->m_bValid = -1;
			return gameStates.render.bBuildModels;
			}
		}
	}
PROF_START
if (!(gameStates.render.bCloaked ?
	   G3EnableClientStates (0, 0, 0, GL_TEXTURE0) :
		G3EnableClientStates (1, 1, gameOpts->ogl.bObjLighting, GL_TEXTURE0)))
	return 0;
if (bUseVBO) {
	int i;
	glBindBufferARB (GL_ARRAY_BUFFER_ARB, pm->m_vboDataHandle);
	if ((i = glGetError ())) {
		glBindBufferARB (GL_ARRAY_BUFFER_ARB, pm->m_vboDataHandle);
		if ((i = glGetError ()))
			return 0;
		}
	}
else {
	pm->m_vbVerts = reinterpret_cast<CFloatVector3*> (pm->m_vertBuf [0].Buffer ());
	pm->m_vbNormals = pm->m_vbVerts + pm->m_nFaceVerts;
	pm->m_vbColor = reinterpret_cast<tRgbaColorf*> (pm->m_vbNormals + pm->m_nFaceVerts);
	pm->m_vbTexCoord = reinterpret_cast<tTexCoord2f*> (pm->m_vbColor + pm->m_nFaceVerts);
	}
#if G3_SW_SCALING
G3ScaleModel (nModel);
#else
#	if 0
if (bHires)
	gameData.models.vScale.SetZero ();
#	endif
#endif
if (!(gameOpts->ogl.bObjLighting || gameStates.render.bQueryCoronas || gameStates.render.bCloaked))
	G3LightModel (objP, nModel, xModelLight, xGlowValues, bHires);
if (bUseVBO) {
	if (!gameStates.render.bCloaked) {
		glNormalPointer (GL_FLOAT, 0, G3_BUFFER_OFFSET (pm->m_nFaceVerts * sizeof (CFloatVector3)));
		glColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (pm->m_nFaceVerts * 2 * sizeof (CFloatVector3)));
		glTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (pm->m_nFaceVerts * ((2 * sizeof (CFloatVector3) + sizeof (tRgbaColorf)))));
		}
	glVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (0));
	if (pm->m_vboIndexHandle)
		glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, pm->m_vboIndexHandle);
	}
else
 {
	if (!gameStates.render.bCloaked) {
		glTexCoordPointer (2, GL_FLOAT, 0, pm->m_vbTexCoord.Buffer ());
		if (gameOpts->ogl.bObjLighting)
			glNormalPointer (GL_FLOAT, 0, pm->m_vbNormals.Buffer ());
		glColorPointer (4, GL_FLOAT, 0, pm->m_vbColor.Buffer ());
		}
	glVertexPointer (3, GL_FLOAT, 0, pm->m_vbVerts.Buffer ());
	}
nGunId = EquippedPlayerGun (objP);
nBombId = EquippedPlayerBomb (objP);
nMissileId = EquippedPlayerMissile (objP, &nMissiles);
if (!bHires && (objP->info.nType == OBJ_POWERUP)) {
	if ((objP->info.nId == POW_SMARTMINE) || (objP->info.nId == POW_PROXMINE))
		gameData.models.vScale.Set (I2X (2), I2X (2), I2X (2));
	else
		gameData.models.vScale.Set (I2X (3) / 2, I2X (3) / 2, I2X (3) / 2);
	}
OglSetLibFlags (1);
G3DrawModel (objP, nModel, nSubModel, modelBitmaps, pAnimAngles, vOffsetP, bHires, bUseVBO, 0,
				 nGunId, nBombId, nMissileId, nMissiles);
if ((objP->info.nType != OBJ_DEBRIS) && bHires && pm->m_bHasTransparency)
	G3DrawModel (objP, nModel, nSubModel, modelBitmaps, pAnimAngles, vOffsetP, bHires, bUseVBO, 1,
					 nGunId, nBombId, nMissileId, nMissiles);
glDisable (GL_TEXTURE_2D);
glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
if (gameStates.render.bCloaked)
	G3DisableClientStates (0, 0, 0, -1);
else
	G3DisableClientStates (1, 1, gameOpts->ogl.bObjLighting, -1);
if (objP && ((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT) || (objP->info.nType == OBJ_REACTOR))) {
	transformation.Begin (objP->info.position.vPos, objP->info.position.mOrient);
	G3RenderDamageLightnings (objP, nModel, 0, pAnimAngles, NULL, bHires);
	transformation.End ();
	}
pm->m_bRendered = 1;
OglClearError (0);
PROF_END(ptRenderObjectsFast)
return 1;
}

//------------------------------------------------------------------------------
//eof
