#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <math.h>
#include <stdlib.h>
#include "error.h"

#include "descent.h"
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
#include "rendermine.h"
#include "segmath.h"
#include "light.h"
#include "dynlight.h"
#include "lightning.h"
#include "../effects/glow.h"
#include "renderthreads.h"
#include "hiresmodels.h"
#include "buildmodel.h"
#include "weapon.h"
#include "headlight.h"

extern CFaceColor tMapColor;

#if DBG_SHADOWS
extern int32_t bShadowTest;
extern int32_t bFrontCap;
extern int32_t bRearCap;
extern int32_t bShadowVolume;
extern int32_t bFrontFaces;
extern int32_t bBackFaces;
extern int32_t bSWCulling;
#endif
extern int32_t bZPass;

#define G3_DRAW_ARRAYS				1
#define G3_DRAW_SUBMODELS			1
#define G3_FAST_MODELS				1
#define G3_DRAW_RANGE_ELEMENTS	1
#define G3_SW_SCALING				0
#define G3_HW_LIGHTING				1
#define G3_USE_VBOS					1 //G3_HW_LIGHTING
#define G3_ALLOW_TRANSPARENCY		1

#if POLYGONAL_OUTLINE
extern bool bPolygonalOutline;
#endif

//------------------------------------------------------------------------------

void G3DynLightModel (CObject *objP, RenderModel::CModel *modelP, uint16_t iVerts, uint16_t nVerts, uint16_t iFaceVerts, uint16_t nFaceVerts)
{
	CFloatVector				vPos, vVertex;
	CFloatVector3*				pv, * pn;
	RenderModel::CVertex*	pmv;
	CFaceColor*					colorP;
	float							fAlpha = gameStates.render.grAlpha;
	int32_t						h, i, bEmissive = objP->IsProjectile ();

if (!gameStates.render.bBrightObject) {
	vPos.Assign (objP->info.position.vPos);
	for (i = iVerts, pv = modelP->m_vertices + iVerts, pn = modelP->m_vertNorms + iVerts, colorP = modelP->m_color + iVerts;
		  i < nVerts;
		  i++, pv++, pn++, colorP++) {
		colorP->index = 0;
		vVertex = vPos + *reinterpret_cast<CFloatVector*> (pv);
		GetVertexColor (-1, -1, i, reinterpret_cast<CFloatVector3*> (pn), vVertex.XYZ (), colorP, NULL, 1, 0, 0);
		}
	}
for (i = iFaceVerts, h = iFaceVerts, pmv = modelP->m_faceVerts + iFaceVerts; i < nFaceVerts; i++, h++, pmv++) {
	if (gameStates.render.bBrightObject || bEmissive) {
		modelP->m_vbColor [h] = pmv->m_baseColor;
		modelP->m_vbColor [h].Alpha () = fAlpha;
		}
	else if (pmv->m_bTextured)
		modelP->m_vbColor [h].Assign (modelP->m_color [pmv->m_nIndex]);
	else {
		colorP = modelP->m_color + pmv->m_nIndex;
		modelP->m_vbColor [h].Red () = pmv->m_baseColor.Red () * colorP->Red ();
		modelP->m_vbColor [h].Green () = pmv->m_baseColor.Green () * colorP->Green ();
		modelP->m_vbColor [h].Blue () = pmv->m_baseColor.Blue () * colorP->Blue ();
		modelP->m_vbColor [h].Alpha () = pmv->m_baseColor.Alpha ();
		}
	}
}

//------------------------------------------------------------------------------

void G3LightModel (CObject *objP, int32_t nModel, fix xModelLight, fix *xGlowValues, int32_t bHires)
{
	RenderModel::CModel*		modelP = gameData.models.renderModels [bHires] + nModel;
	RenderModel::CVertex*	pmv;
	RenderModel::CFace*		pmf;
	CFloatVector				baseColor, *colorP;
	float							fLight, fAlpha = gameStates.render.grAlpha;
	int32_t						h, i, j, l;
	int32_t						bEmissive = (objP->info.nType == OBJ_MARKER) || (objP->IsProjectile ());

#if DBG
if (objP->Index () == nDbgObj)
	BRP;
#endif
#if 0
if (xModelLight > I2X (1))
	xModelLight = I2X (1);
#endif
if (SHOW_DYN_LIGHT && (gameOpts->ogl.bObjLighting ||
    (gameOpts->ogl.bLightObjects && (gameOpts->ogl.bLightPowerups || (objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT))))) {
	tiRender.objP = objP;
	tiRender.modelP = modelP;
	if (!RunRenderThreads (rtPolyModel))
		G3DynLightModel (objP, modelP, 0, modelP->m_nVerts, 0, modelP->m_nFaceVerts);
	}
else {
	if (gameData.objData.color.index)
		baseColor.Assign (gameData.objData.color);
	else
		baseColor.Set (1.0f, 1.0f, 1.0f);
	baseColor.Alpha () = fAlpha;
	for (i = modelP->m_nFaces, pmf = modelP->m_faces.Buffer (); i; i--, pmf++) {
		if (pmf->m_nBitmap >= 0)
			colorP = &baseColor;
		else
			colorP = NULL;
		if (pmf->m_bGlow)
			l = xGlowValues [nGlow];
		else if (bEmissive)
			l = I2X (1);
		else {
			l = -CFixVector::Dot (transformation.m_info.view [0].m.dir.f, pmf->m_vNormal);
			l = 3 * I2X (1) / 4 + l / 4;
			l = FixMul (l, xModelLight);
			}
		fLight = X2F (l);
		for (j = pmf->m_nVerts, h = pmf->m_nIndex, pmv = modelP->m_faceVerts + pmf->m_nIndex; j; j--, h++, pmv++) {
#if G3_DRAW_ARRAYS
			if (colorP) {
				modelP->m_vbColor [h].Red () = colorP->Red () * fLight;
				modelP->m_vbColor [h].Green () = colorP->Green () * fLight;
				modelP->m_vbColor [h].Blue () = colorP->Blue () * fLight;
				modelP->m_vbColor [h].Alpha () = fAlpha;
				}
			else {
				modelP->m_vbColor [h].Red () = pmv->m_baseColor.Red () * fLight;
				modelP->m_vbColor [h].Green () = pmv->m_baseColor.Green () * fLight;
				modelP->m_vbColor [h].Blue () = pmv->m_baseColor.Blue () * fLight;
				modelP->m_vbColor [h].Alpha () = fAlpha;
				}
#else
			if (colorP) {
				pmv->m_renderColor.Red () = colorP->Red () * fLight;
				pmv->m_renderColor.Green () = colorP->Green () * fLight;
				pmv->m_renderColor.Blue () = colorP->Blue () * fLight;
				pmv->m_renderColor.Alpha () = fAlpha;
				}
			else {
				pmv->m_renderColor.Red () = pmv->m_baseColor.Red () * fLight;
				pmv->m_renderColor.Green () = pmv->m_baseColor.Green () * fLight;
				pmv->m_renderColor.Blue () = pmv->m_baseColor.Blue () * fLight;
				pmv->m_renderColor.Alpha () = fAlpha;
				}
#endif
			}
		}
	}
}

//------------------------------------------------------------------------------

#if G3_SW_SCALING

void G3ScaleModel (int32_t nModel, int32_t bHires)
{
	RenderModel::CModel	*modelP = gameData.models.renderModels [bHires] + nModel;
	CFloatVector			fScale;
	int32_t					i;
	CFloatVector3			*pv;
	RenderModel::CVertex	*pmv;

if (gameData.models.vScale.IsZero ())
	fScale.Create (1,1,1);
else
	fScale.Assign (gameData.models.vScale);
if (modelP->m_fScale == fScale)
	return;
fScale /= modelP->m_fScale;
for (i = modelP->m_nVerts, pv = modelP->m_vertices; i; i--, pv++) {
	pv->p.x *= fScale;
	pv->p.y *= fScale;
	pv->p.z *= fScale;
	}
for (i = modelP->m_nFaceVerts, pmv = modelP->m_faceVerts; i; i--, pmv++)
	pmv->m_vertex = modelP->m_vertices [pmv->m_nIndex];
modelP->m_fScale *= fScale;
}

#endif

//------------------------------------------------------------------------------

void G3GetThrusterPos (CObject *objP, int16_t nModel, RenderModel::CFace *pmf, CFixVector *vOffsetP,
							  CFixVector *vNormal, int32_t nRad, int32_t bHires, uint8_t nType = 255)
{
	RenderModel::CModel*		modelP = gameData.models.renderModels [bHires] + nModel;
	RenderModel::CVertex*	pmv = NULL;
	CFloatVector3				v = CFloatVector3::ZERO, vn, vo, vForward = CFloatVector3::Create(0,0,1);
	CModelThrusters*			mtP = gameData.models.thrusters + nModel;
	int32_t						i, j = 0, nCount = -mtP->nCount;
	float							h, nSize;

if (!objP)
	return;
if (nCount < 0) {
	if (modelP->m_bRendered && gameData.models.vScale.IsZero ())
		return;
	nCount = 0;
	}	
else if (nCount >= MAX_THRUSTERS)
	return;
vn.Assign (pmf ? pmf->m_vNormal : *vNormal);
if (!IsPlayerShip (nModel) && (CFloatVector3::Dot (vn, vForward) > -1.0f / 3.0f))
	return;
if (pmf) {
	for (i = 0, j = pmf->m_nVerts, pmv = modelP->m_faceVerts + pmf->m_nIndex; i < j; i++)
		v += pmv [i].m_vertex;
	v.v.coord.x /= j;
	v.v.coord.y /= j;
	v.v.coord.z /= j;
	}
else
	v.SetZero ();
v.v.coord.z -= 1.0f / 16.0f;
#if 0
transformation.Transform (&dir, &dir, 0);
#else
#if 1
if (vOffsetP) {
	vo.Assign (*vOffsetP);
	v += vo;
	}
#endif
#endif
if (nCount && (v.v.coord.x == mtP->vPos [0].v.coord.x) && (v.v.coord.y == mtP->vPos [0].v.coord.y) && (v.v.coord.z == mtP->vPos [0].v.coord.z))
	return;
mtP->vPos [nCount].Assign (v);
if (vOffsetP)
	v -= vo;
mtP->vDir [nCount] = *vNormal;
mtP->vDir [nCount].Neg ();
mtP->nType [nCount] = nType;
//if (!nCount) 
	{
	if (!pmf)
		mtP->fSize [nCount] = X2F (nRad);
	else {
		for (i = 0, nSize = 1000000000; i < j; i++)
			if (nSize > (h = CFloatVector3::Dist (v, pmv [i].m_vertex)))
				nSize = h;
		mtP->fSize [nCount] = nSize;// * 1.25f;
		}
	}
mtP->nCount = -(++nCount);
}

//------------------------------------------------------------------------------

static int32_t bCenterGuns [] = {0, 1, 1, 0, 0, 0, 1, 1, 0, 1};

int32_t G3FilterSubModel (CObject *objP, RenderModel::CSubModel *subModelP, int32_t nGunId, int32_t nBombId, int32_t nMissileId, int32_t nMissiles)
{
	int32_t nId = objP->info.nId;

if (!subModelP->m_bRender)
	return 0;
#if 1
if (subModelP->m_bFlare)
	return 1;
#endif
if (subModelP->m_nGunPoint >= 0)
	return 1;
if (subModelP->m_bBullets)
	return 1;
#if 1
if (subModelP->m_bThruster && ((subModelP->m_bThruster & (REAR_THRUSTER | FRONTAL_THRUSTER)) != (REAR_THRUSTER | FRONTAL_THRUSTER)))
	return 1;
#endif
if (subModelP->m_bHeadlight)
	return !HeadlightIsOn (nId);
if (subModelP->m_bBombMount)
	return (nBombId == 0);
if (subModelP->m_bWeapon) {
	CPlayerData	*playerP = gameData.multiplayer.players + nId;
	int32_t		bLasers = (nGunId == LASER_INDEX) || (nGunId == SUPER_LASER_INDEX);
	int32_t		bSuperLasers = playerP->HasSuperLaser ();
	int32_t		bQuadLasers = gameData.multiplayer.weaponStates [N_LOCALPLAYER].bQuadLasers;
	int32_t		bCenterGun = bCenterGuns [nGunId];
	int32_t		nWingtip = bQuadLasers ? bSuperLasers : 2; //gameOpts->render.ship.nWingtip;

	gameOpts->render.ship.nWingtip = nWingtip;
	if (nWingtip == 0)
		nWingtip = bLasers && bSuperLasers && bQuadLasers;
	else if (nWingtip == 1)
		nWingtip = !bLasers || bSuperLasers;
	
	if (EGI_FLAG (bShowWeapons, 0, 1, 0)) {
		if (subModelP->m_nGun == nGunId + 1) {
			if (subModelP->m_nGun == FUSION_INDEX + 1) {
				if ((subModelP->m_nWeaponPos == 3) && !gameData.multiplayer.weaponStates [N_LOCALPLAYER].bTripleFusion)
					return 1;
				}
			else if (bLasers) {
				if ((subModelP->m_nWeaponPos > 2) && !bQuadLasers && (nWingtip != bSuperLasers))
					return 1;
				}
			}
		else if (subModelP->m_nGun == LASER_INDEX + 1) {
			if (nWingtip)
				return 1;
			return !bCenterGun && (subModelP->m_nWeaponPos < 3);
			}
		else if (subModelP->m_nGun == SUPER_LASER_INDEX + 1) {
			if (nWingtip != 1)
				return 1;
			return !bCenterGun && (subModelP->m_nWeaponPos < 3);
			}
		else if (!subModelP->m_nGun) {
			if (bLasers && bQuadLasers)
				return 1;
			if (subModelP->m_nType != gameOpts->render.ship.nWingtip)
				return 1;
			return 0;
			}
		else if (subModelP->m_nBomb == nBombId)
			return (nId == N_LOCALPLAYER) && !AllowedToFireMissile (nId, 0);
		else if (subModelP->m_nMissile == nMissileId) {
			if (subModelP->m_nWeaponPos > nMissiles)
				return 1;
			else {
				static int32_t nMslPos [] = {-1, 1, 0, 3, 2};
				int32_t nLaunchPos = gameData.multiplayer.weaponStates [nId].nMslLaunchPos;
				return (nId == N_LOCALPLAYER) && !AllowedToFireMissile (nId, 0) &&
						 (nLaunchPos == (nMslPos [(int32_t) subModelP->m_nWeaponPos]));
				}
			}
		else
			return 1;
		}
	else {
		if (subModelP->m_nGun == 1)
			return 0;
		if ((subModelP->m_nGun < 0) && (subModelP->m_nMissile == 1))
			return 0;
		return 1;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

static inline int32_t ObjectHasThruster (CObject *objP)
{
return (objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT) || objP->IsMissile ();
}

//------------------------------------------------------------------------------

int32_t G3AnimateSubModel (CObject *objP, RenderModel::CSubModel *subModelP, int16_t nModel)
{
	tFiringData*	fP;
	float				nTimeout, y;
	int32_t			nDelay;

if (!subModelP->m_nFrames)
	return 0;
if ((subModelP->m_bThruster & (REAR_THRUSTER | FRONTAL_THRUSTER)) == (REAR_THRUSTER | FRONTAL_THRUSTER)) {
	nTimeout = gameStates.gameplay.slowmo [0].fSpeed;
	glPushMatrix ();
	CFloatVector vCenter;
	vCenter.Assign (subModelP->m_vCenter);
	glTranslatef (vCenter .v.coord.x, vCenter .v.coord.y, vCenter .v.coord.z);
	glRotatef (-360.0f * 5.0f * float (subModelP->m_iFrame) / float (subModelP->m_nFrames), 0, 0, 1);
	glTranslatef (-vCenter .v.coord.x, -vCenter .v.coord.y, -vCenter .v.coord.z);
	if (gameStates.app.nSDLTicks [0] - subModelP->m_tFrame > nTimeout) {
		subModelP->m_tFrame = gameStates.app.nSDLTicks [0];
		subModelP->m_iFrame = (subModelP->m_iFrame + 1) % subModelP->m_nFrames;
		}
	}
else {
	nTimeout = 25 * gameStates.gameplay.slowmo [0].fSpeed;
	fP = gameData.multiplayer.weaponStates [objP->info.nId].firing;
	if (gameData.weapons.nPrimary == VULCAN_INDEX)
		nTimeout /= 2;
	if (fP->nStop > 0) {
		nDelay = gameStates.app.nSDLTicks [0] - fP->nStop;
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
	glPushMatrix ();
	y = X2F (subModelP->m_vCenter .v.coord.y);
	glTranslatef (0, y, 0);
	glRotatef (360 * float (subModelP->m_iFrame) / float (subModelP->m_nFrames), 0, 0, 1);
	glTranslatef (0, -y, 0);
	if (gameStates.app.nSDLTicks [0] - subModelP->m_tFrame > nTimeout) {
		subModelP->m_tFrame = gameStates.app.nSDLTicks [0];
		subModelP->m_iFrame = (subModelP->m_iFrame + 1) % subModelP->m_nFrames;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

static inline int32_t PlayerColor (int32_t nObject)
{
	int32_t	nColor;

for (nColor = 0; nColor < N_PLAYERS; nColor++)
	if (PLAYER (nColor).nObject == nObject)
		return (nColor % MAX_PLAYER_COLORS) + 1;
return 1;
}

//------------------------------------------------------------------------------

void G3TransformSubModel (CObject *objP, int16_t nModel, int16_t nSubModel, CAngleVector *animAnglesP, CFixVector *vOffsetP, 
								  int32_t bHires, int32_t nGunId, int32_t nBombId, int32_t nMissileId, int32_t nMissiles, int32_t bEdges)
{
	RenderModel::CModel*		modelP = gameData.models.renderModels [bHires] + nModel;
	RenderModel::CSubModel*	subModelP = modelP->m_subModels + nSubModel;
	CAngleVector				va = animAnglesP ? animAnglesP [subModelP->m_nAngles] : CAngleVector::ZERO;
	CFixVector					vo;
	int32_t						i, j;

if (subModelP->m_bThruster)
	return;
if (G3FilterSubModel (objP, subModelP, nGunId, nBombId, nMissileId, nMissiles))
	return;
vo = subModelP->m_vOffset;
if (!gameData.models.vScale.IsZero ())
	vo *= gameData.models.vScale;
#if 1
if (vOffsetP) {
	transformation.Begin (vo, va);
	vo += *vOffsetP;
	}
#endif
if (bEdges) {
	RenderModel::CModelEdge *edgeP = subModelP->m_edges.Buffer ();
	for (i = subModelP->m_nEdges; i; i--, edgeP++)
		edgeP->Transform ();
	}
else {
	RenderModel::CFace *faceP = subModelP->m_faces;
	for (i = 0; i < subModelP->m_nFaces; i++, faceP++) {
		transformation.Transform (faceP->m_vNormalf [1], faceP->m_vNormalf [0]);
		transformation.Transform (faceP->m_vCenterf [1], faceP->m_vCenterf [0]);
		}
	for (i = 0, j = subModelP->m_nVertexIndex [1]; i < subModelP->m_nVertices; i++, j++) {
		uint16_t h = modelP->m_vertexOwner [j].m_nVertex;
		transformation.Transform (modelP->m_vertices [h], modelP->m_vertices [h]);
		}
	}
for (i = 0, j = modelP->m_nSubModels, subModelP = modelP->m_subModels.Buffer (); i < j; i++, subModelP++) {
	if (subModelP->m_nParent == nSubModel)
		G3TransformSubModel (objP, nModel, i, animAnglesP, &vo, bHires, nGunId, nBombId, nMissileId, nMissiles, bEdges);
	}
transformation.End ();
}

//------------------------------------------------------------------------------

void G3DrawSubModel (CObject *objP, int16_t nModel, int16_t nSubModel, int16_t nExclusive, CArray<CBitmap*>& modelBitmaps,
						   CAngleVector *animAnglesP, CFixVector *vOffsetP, int32_t bHires, int32_t bUseVBO, int32_t nPass, int32_t bTranspFilter,
							int32_t nGunId, int32_t nBombId, int32_t nMissileId, int32_t nMissiles, int32_t bEdges)
{
	RenderModel::CModel*		modelP = gameData.models.renderModels [bHires] + nModel;
	RenderModel::CSubModel*	subModelP = modelP->m_subModels + nSubModel;
	CBitmap*						bmP = NULL;
	CAngleVector				va = animAnglesP ? animAnglesP [subModelP->m_nAngles] : CAngleVector::ZERO;
	CFixVector					vo;
	int32_t						h, i, j, bTranspState, bRestoreMatrix, bTextured = gameOpts->render.debug.bTextures && !(gameStates.render.bCloaked /*|| nPass*/),
									bGetThruster = !(nPass || bTranspFilter) && ObjectHasThruster (objP);
	int16_t						nId, nBitmap = -1, nTeamColor;
	uint16_t						nFaceVerts, nVerts, nIndex;

#if DBG
if (nModel == nDbgModel)
	nDbgModel = nModel;
if (objP) {
	if (ObjIdx (objP) == nDbgObj)
		BRP;
	if (objP->info.nSegment == nDbgSeg)
		BRP;
	if (objP->info.nType == OBJ_DEBRIS)
		BRP;
	}
#endif

if (objP->info.nType == OBJ_PLAYER)
	nTeamColor = IsMultiGame ? IsTeamGame ? GetTeam (objP->info.nId) + 1 : PlayerColor (objP->Index ()) : gameOpts->render.ship.nColor;
else
	nTeamColor = 0;
#if 1
if (subModelP->m_bThruster) {
	if (!nPass) {
		vo = subModelP->m_vOffset + subModelP->m_vCenter;
		G3GetThrusterPos (objP, nModel, NULL, &vo, &subModelP->m_faces->m_vNormal, subModelP->m_nRad, bHires, subModelP->m_bThruster);
		}
	if (!subModelP->m_nFrames)
		return;
	}
if (G3FilterSubModel (objP, subModelP, nGunId, nBombId, nMissileId, nMissiles))
	return;
#endif
vo = subModelP->m_vOffset;
if (!gameData.models.vScale.IsZero ())
	vo *= gameData.models.vScale;
#if 1
if (vOffsetP && (nExclusive < 0)) {
	transformation.Begin (vo, va);
	vo += *vOffsetP;
	}
#endif
bRestoreMatrix = G3AnimateSubModel (objP, subModelP, nModel);
// render the faces
#if G3_DRAW_SUBMODELS
// render any dependent submodels
if ((nExclusive < 0) || (nSubModel != nExclusive)) {
	for (i = 0, j = modelP->m_nSubModels, subModelP = modelP->m_subModels.Buffer (); i < j; i++, subModelP++)
		if (subModelP->m_nParent == nSubModel)
			G3DrawSubModel (objP, nModel, i, nExclusive, modelBitmaps, animAnglesP, &vo, bHires,bUseVBO, nPass, bTranspFilter, nGunId, nBombId, nMissileId, nMissiles, bEdges);
	}
#endif
#if 0
subModelP = modelP->m_subModels + nSubModel;
if (subModelP->m_bBillboard)
#endif
if ((nExclusive < 0) || (nSubModel == nExclusive)) {
	subModelP = modelP->m_subModels + nSubModel;
	if (subModelP->m_bBillboard) {
		if (!bRestoreMatrix) {
			bRestoreMatrix = 1;
			glPushMatrix ();
			}
		float modelView [16];
		glGetFloatv (GL_MODELVIEW_MATRIX, modelView);
		// undo all rotations
		// beware all scaling is lost as well 
#if 1
		for (int32_t i = 0; i < 3; i++) 
			 for (int32_t j = 0; j < 3; j++)
				modelView [i * 4 + j] = (i == j) ? 1.0f : 0.0f;
#endif
		glLoadMatrixf (modelView);
		}

	ogl.SetTexturing (false);
	if (gameStates.render.bCloaked)
		glColor4f (0, 0, 0, gameStates.render.grAlpha);
	if (bEdges) {
		if (!subModelP->m_bThruster) {
			RenderModel::CModelEdge* edgeP = subModelP->m_edges.Buffer ();
			int32_t nVertices [2] = { 0, gameData.segData.edgeVertices.Length () };
			int32_t nEdgeFilter = objP->IsWeapon () ? 0 : bHires; //0 : 2; //bHires;
			for (i = subModelP->m_nEdges; i; i--, edgeP++) 
				edgeP->Prepare (CFloatVector::ZERO, nVertices, nEdgeFilter);
			RenderOutline (nVertices);
			}	
		}
	else {
		RenderModel::CFace* faceP = subModelP->m_faces;
		for (i = subModelP->m_nFaces; i; ) {
			if (bTextured && (nBitmap != faceP->m_nBitmap)) {
				if (0 > (nBitmap = faceP->m_nBitmap))
					ogl.SetTexturing (false);
				else {
					if (!bHires)
						bmP = modelBitmaps [nBitmap];
					else {
						bmP = modelP->m_textures + nBitmap;
#if 0
						if (!nPass)
							ogl.SetBlendMode (subModelP->m_bFlare);
#endif
						if (nTeamColor && bmP->Team () && (0 <= (h = modelP->m_teamTextures [nTeamColor % MAX_PLAYER_COLORS]))) {
							nBitmap = h;
							bmP = modelP->m_textures + nBitmap;
							}
						}
#if 0
					bmP->AvgColor ();
					CRGBColor *colorP = bmP->GetAvgColor ();
					if (!gameStates.render.bCloaked)
						glColor4f ((float) colorP->Red () / 255.0f, (float) colorP->Green () / 255.0f, (float) colorP->Blue () / 255.0f, gameStates.render.grAlpha);
					ogl.SetTexturing (false);
					ogl.DisableClientState (GL_COLOR_ARRAY);
#else
					ogl.SelectTMU (GL_TEXTURE0, true);
					ogl.SetTexturing (true);
					bmP = bmP->Override (-1);
					if (bmP->Frames ())
						bmP = bmP->CurFrame ();
					if (bmP->Bind (1))
						continue;
					bmP->Texture ()->Wrap (GL_REPEAT);
#endif
					}
				}
			nIndex = faceP->m_nIndex;
			if (bHires) {
				if (!(bTranspState = (modelP->m_bHasTransparency >> 2)))
					bTranspState = !bmP ? 0 : subModelP->m_bThruster ? 0 : (subModelP->m_bGlow || subModelP->m_bFlare) ? 2 : ((bmP->Flags () & BM_FLAG_TRANSPARENT) != 0) ? 1 : 0;
				if (bTranspState != bTranspFilter) {
					if (bTranspState)
						modelP->m_bHasTransparency |= bTranspState;
					faceP++;
					i--;
					continue;
					}
				}
			if ((nFaceVerts = faceP->m_nVerts) > 4) {
				if (bGetThruster && faceP->m_bThruster)
					G3GetThrusterPos (objP, nModel, faceP, &vo, &faceP->m_vNormal, 0, bHires);
				nVerts = nFaceVerts;
				faceP++;
				i--;
				}
			else {
				nId = faceP->m_nId;
				nVerts = 0;
				do {
					if (bGetThruster && faceP->m_bThruster)
						G3GetThrusterPos (objP, nModel, faceP, &vo, &faceP->m_vNormal, 0, bHires);
					nVerts += nFaceVerts;
					faceP++;
					i--;
					} while (i && (faceP->m_nId == nId));
				}
			//if ((gameStates.render.nType == RENDER_TYPE_TRANSPARENCY) && subModelP->m_bGlow)
			//	glowRenderer.Begin (GLOW_HEADLIGHT, 2, false, 1.0f);
#ifdef _WIN32
			if (glDrawRangeElements)
#endif
#if 0 //DBG
				if (bUseVBO)
#	if 1
					glDrawRangeElements (gameOpts->render.debug.bWireFrame ? GL_LINE_LOOP : (nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
												0, modelP->m_nFaceVerts - 1, nVerts, GL_UNSIGNED_SHORT,
												G3_BUFFER_OFFSET (nIndex * sizeof (int16_t)));
#	else
					glDrawElements (gameOpts->render.debug.bWireFrame ? GL_LINE_LOOP : (nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
										 nVerts, GL_UNSIGNED_SHORT,
										 G3_BUFFER_OFFSET (nIndex * sizeof (int16_t)));
#	endif
				else
					glDrawRangeElements (gameOpts->render.debug.bWireFrame ? GL_LINE_LOOP : (nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
												nIndex, nIndex + nVerts - 1, nVerts, GL_UNSIGNED_SHORT,
												modelP->m_index [0] + nIndex);
#else // DBG
				if (bUseVBO)
					glDrawRangeElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
												0, modelP->m_nFaceVerts - 1, nVerts, GL_UNSIGNED_SHORT,
												G3_BUFFER_OFFSET (nIndex * sizeof (int16_t)));
				else
					glDrawRangeElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
												nIndex, nIndex + nVerts - 1, nVerts, GL_UNSIGNED_SHORT,
												modelP->m_index [0] + nIndex);
#endif // DBG
#ifdef _WIN32
			else
				if (bUseVBO)
					glDrawElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
										 nVerts, GL_UNSIGNED_SHORT, G3_BUFFER_OFFSET (nIndex * sizeof (int16_t)));
				else
					glDrawElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
										 nVerts, GL_UNSIGNED_SHORT, modelP->m_index + nIndex);
#endif //_WIN32
			//if ((gameStates.render.nType == RENDER_TYPE_TRANSPARENCY) && subModelP->m_bGlow)
			//	glowRenderer.End ();
			}
		}
	}
if (bRestoreMatrix)
	glPopMatrix ();
#if 1
if (vOffsetP && (nExclusive < 0))
	transformation.End ();
#endif
}

//------------------------------------------------------------------------------

void G3DrawModel (CObject *objP, int16_t nModel, int16_t nSubModel, CArray<CBitmap*>& modelBitmaps,
						CAngleVector *animAnglesP, CFixVector *vOffsetP, int32_t bHires, int32_t bUseVBO, int32_t bTranspFilter,
						int32_t nGunId, int32_t nBombId, int32_t nMissileId, int32_t nMissiles, int32_t bEdges)
{
	RenderModel::CModel*	modelP;
	CDynLight*				lightP;
	int32_t					nPass, iLight, nLights, nLightRange;
	int32_t					bBright = bEdges || gameStates.render.bFullBright || (objP && (objP->info.nType == OBJ_MARKER));
	int32_t					bEmissive = !bEdges && objP && (objP->IsProjectile ());
	int32_t					bLighting = !bEdges && SHOW_DYN_LIGHT && gameOpts->ogl.bObjLighting && (bTranspFilter < 2) && !(gameStates.render.bCloaked || bEmissive || bBright);
	GLenum					hLight;
	float						fBrightness, fLightScale = gameData.models.nLightScale ? X2F (gameData.models.nLightScale) : 1.0f;
	CFloatVector			color;
	CDynLightIndex*		sliP = bLighting ? &lightManager.Index (0,0) : NULL;
	CActiveDynLight*		activeLightsP = sliP ? lightManager.Active (0) + sliP->nFirst : NULL;
	tObjTransformation*	posP = OBJPOS (objP);

#if POLYGONAL_OUTLINE
if (bEdges && bPolygonalOutline) // only required if not transforming model outlines via OpenGL when rendering
	ogl.SetTransform (0);
else
#endif
	ogl.SetupTransform (1);
if (bLighting) {
	nLights = sliP->nActive;
	if (nLights > gameStates.render.nMaxLightsPerObject)
		nLights = gameStates.render.nMaxLightsPerObject;
	ogl.EnableLighting (0);
	}
else
	nLights = 1;
ogl.SetBlending (true);
ogl.SetDepthWrite (false);
if (bEmissive || (bTranspFilter == 2))
	ogl.SetBlendMode (OGL_BLEND_ADD);
else if (gameStates.render.bCloaked)
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
else if (bTranspFilter)
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
else {
	ogl.SetBlendMode (OGL_BLEND_REPLACE);
	ogl.SetDepthWrite (true);
	}

if (!bLighting || (sliP->nLast < 0))
	nLightRange = 0;
else
	nLightRange = sliP->nLast - sliP->nFirst + 1;

for (nPass = 0; ((nLightRange > 0) && (nLights > 0)) || !nPass; nPass++) {
	if (bLighting) {
		if (nPass) {
			ogl.SetBlendMode (OGL_BLEND_ADD_WEAK);
			ogl.SetDepthWrite (false);
			}
		for (iLight = 0; (nLightRange > 0) && (iLight < 8) && nLights; activeLightsP++, nLightRange--) {
#if DBG
			if (activeLightsP - lightManager.Active (0) >= MAX_OGL_LIGHTS)
				break;
			if (activeLightsP < lightManager.Active (0))
				break;
#endif
			if (nLights < 0) {
				CFaceColor* segColorP = lightManager.AvgSgmColor (objP->info.nSegment, NULL, 0);
				CFloatVector3 vPos;
				hLight = GL_LIGHT0 + iLight++;
				glEnable (hLight);
				vPos.Assign (objP->info.position.vPos);
				glLightfv (hLight, GL_POSITION, reinterpret_cast<GLfloat*> (&vPos));
				glLightfv (hLight, GL_DIFFUSE, reinterpret_cast<GLfloat*> (&segColorP->v.color));
				glLightfv (hLight, GL_SPECULAR, reinterpret_cast<GLfloat*> (&segColorP->v.color));
				glLightf (hLight, GL_CONSTANT_ATTENUATION, 0.1f);
				glLightf (hLight, GL_LINEAR_ATTENUATION, 0.1f);
				glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.01f);
				nLights = 0;
				}
			else if ((lightP = lightManager.GetActive (activeLightsP, 0))) {
#if DBG
				if ((nDbgSeg >= 0) && (lightP->info.nObject >= 0) && (OBJECT (lightP->info.nObject)->info.nSegment == nDbgSeg))
					BRP;
#endif
				hLight = GL_LIGHT0 + iLight++;
				glEnable (hLight);
	//			sprintf (szLightSources + strlen (szLightSources), "%d ", (lightP->nObject >= 0) ? -lightP->nObject : lightP->nSegment);
				fBrightness = lightP->info.fBrightness * fLightScale;
				memcpy (&color.v.color, &lightP->info.color, sizeof (color.v.color));
				color.Red () *= fLightScale;
				color.Green () *= fLightScale;
				color.Blue () *= fLightScale;
				glLightfv (hLight, GL_POSITION, reinterpret_cast<GLfloat*> (lightP->render.vPosf));
				glLightfv (hLight, GL_DIFFUSE, reinterpret_cast<GLfloat*> (&color));
				glLightfv (hLight, GL_SPECULAR, reinterpret_cast<GLfloat*> (&color));
				if (lightP->info.bSpot) {
					glLighti (hLight, GL_SPOT_EXPONENT, 12);
					glLighti (hLight, GL_SPOT_CUTOFF, 25);
					glLightfv (hLight, GL_SPOT_DIRECTION, reinterpret_cast<GLfloat*> (&lightP->info.vDirf));
					glLightf (hLight, GL_CONSTANT_ATTENUATION, 0.0f); //0.1f / fBrightness);
					glLightf (hLight, GL_LINEAR_ATTENUATION, 0.05f / fBrightness);
					glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.005f / fBrightness);
					}
				else {
					glLightf (hLight, GL_CONSTANT_ATTENUATION, 0.0f); //0.1f / fBrightness);
					if (X2F (CFixVector::Dist (objP->info.position.vPos, lightP->info.vPos)) <= lightP->info.fRad) {
						glLightf (hLight, GL_LINEAR_ATTENUATION, 0.01f / fBrightness);
						glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.001f / fBrightness);
						}
					else {
						glLightf (hLight, GL_LINEAR_ATTENUATION, 0.05f / fBrightness);
						glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.005f / fBrightness);
						}
					}
				nLights--;
				}
			}
		for (; iLight < 8; iLight++)
			glDisable (GL_LIGHT0 + iLight);
		}

	transformation.Begin (posP->vPos, posP->mOrient);

	modelP = gameData.models.renderModels [bHires] + nModel;
	if (bEdges) {
		if (bHires) {
		for (int32_t i = 0; i < modelP->m_nSubModels; i++)
				if (modelP->m_subModels [i].m_nParent == -1) 
					G3TransformSubModel (objP, nModel, i, animAnglesP, (nSubModel < 0) ? &modelP->m_subModels [0].m_vOffset : vOffsetP,
												bHires, nGunId, nBombId, nMissileId, nMissiles, bEdges);
			}
		else {
			G3TransformSubModel (objP, nModel, 0, animAnglesP, (nSubModel < 0) ? &modelP->m_subModels [0].m_vOffset : vOffsetP,
										bHires, nGunId, nBombId, nMissileId, nMissiles, bEdges);
			}
		}

	if (bHires) {
		for (int32_t i = 0; i < modelP->m_nSubModels; i++)
			if (modelP->m_subModels [i].m_nParent == -1) {
				G3DrawSubModel (objP, nModel, i, nSubModel, modelBitmaps, animAnglesP, (nSubModel < 0) ? &modelP->m_subModels [0].m_vOffset : vOffsetP,
									 bHires, bUseVBO, nPass, bTranspFilter, nGunId, nBombId, nMissileId, nMissiles, bEdges);
				}
		}
	else {
		G3DrawSubModel (objP, nModel, 0, nSubModel, modelBitmaps, animAnglesP, (nSubModel < 0) ? &modelP->m_subModels [0].m_vOffset : vOffsetP,
							 bHires, bUseVBO, nPass, bTranspFilter, nGunId, nBombId, nMissileId, nMissiles, bEdges);
		}
	transformation.End ();
	if (!bLighting)
		break;
	}
#if DBG
if (!nLightRange && nLights)
	nLights = 0;
#endif
if (bLighting) {
	ogl.DisableLighting ();
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
	ogl.SetDepthWrite (true);
	}
#if POLYGONAL_OUTLINE
if (!bEdges || !bPolygonalOutline)
#endif
	ogl.ResetTransform (1);
//HUDMessage (0, "%s", szLightSources);
}

//------------------------------------------------------------------------------

void G3RenderDamageLightning (CObject *objP, int16_t nModel, int16_t nSubModel, CAngleVector *animAnglesP, CFixVector *vOffsetP, int32_t bHires)
{
if (!(SHOW_LIGHTNING (1) && gameOpts->render.lightning.bDamage))
	return;

	RenderModel::CModel*		modelP;
	RenderModel::CSubModel*	subModelP;
	RenderModel::CFace*		pmf;
	const CAngleVector*		va;
	CFixVector					vo;
	int32_t						i, j;

modelP = gameData.models.renderModels [bHires] + nModel;
if (modelP->m_bValid < 1) {
	if (!bHires)
		return;
	modelP = gameData.models.renderModels [0] + nModel;
	if (modelP->m_bValid < 1)
		return;
	}
subModelP = modelP->m_subModels + nSubModel;
va = animAnglesP ? animAnglesP + subModelP->m_nAngles : &CAngleVector::ZERO;
if (!objP || (objP->Damage () > 0.5f))
	return;
// set the translation
vo = subModelP->m_vOffset;
if (!gameData.models.vScale.IsZero ())
	vo *= gameData.models.vScale;
if (vOffsetP) {
	transformation.Begin (vo, *va);
	vo += *vOffsetP;
	}
// render any dependent submodels
for (i = 0, j = modelP->m_nSubModels, subModelP = modelP->m_subModels.Buffer (); i < j; i++, subModelP++)
	if (subModelP->m_nParent == nSubModel)
		G3RenderDamageLightning (objP, nModel, i, animAnglesP, &vo, bHires);
// render the lightnings
for (subModelP = modelP->m_subModels + nSubModel, i = subModelP->m_nFaces, pmf = subModelP->m_faces; i; i--, pmf++)
	lightningManager.RenderForDamage (objP, NULL, modelP->m_faceVerts + pmf->m_nIndex, pmf->m_nVerts);
if (vOffsetP)
	transformation.End ();
}

//------------------------------------------------------------------------------

int32_t G3RenderModel (CObject *objP, int16_t nModel, int16_t nSubModel, CPolyModel* pp, CArray<CBitmap*>& modelBitmaps,
							  CAngleVector *animAnglesP, CFixVector *vOffsetP, fix xModelLight, fix *xGlowValues, CFloatVector *pObjColor)
{
if (objP && (objP->info.nType == OBJ_PLAYER) && (nModel > 0) && (nModel != COCKPIT_MODEL))
	nModel += gameData.multiplayer.weaponStates [objP->info.nId].nShip;

	int32_t	i = 0, 
			bHires = (nModel > 0), 
			bUseVBO = ogl.m_features.bVertexBufferObjects && ((gameStates.render.bPerPixelLighting == 2) || gameOpts->ogl.bObjLighting),
			bEmissive = (objP != NULL) && objP->IsProjectile (),
			nGunId, nBombId, nMissileId, nMissiles;

nModel = abs (nModel);
RenderModel::CModel*	modelP = gameData.models.renderModels [bHires] + nModel;

if (!objP)
	return 0;
#if DBG
if (nModel == nDbgModel)
	nDbgModel = nModel;
if (objP) {
	if (ObjIdx (objP) == nDbgObj)
		BRP;
	if (objP->info.nSegment == nDbgSeg)
		BRP;
	if (objP->info.nType == OBJ_DEBRIS)
		BRP;
	}
#endif

if (bHires) {
	if (modelP->m_bValid < 1) {
		if (modelP->m_bValid)
			bHires = 0;
		else {
			if (IsDefaultModel (nModel)) {
				if (bUseVBO && modelP->m_bValid && !(modelP->m_vboDataHandle && modelP->m_vboIndexHandle))
					modelP->m_bValid = 0;
				i = G3BuildModel (objP, nModel, pp, modelBitmaps, pObjColor, 1);
				if (i < 0)	//successfully built new model
					return gameStates.render.bBuildModels;
				}
			else
				i = 0;
			modelP->m_bValid = -1;
			bHires = 0;
			}
		}
	}

if (!bHires) {
	modelP = gameData.models.renderModels [0] + nModel;
	if (modelP->m_bValid < 0)
		return 0;
	if (bUseVBO && modelP->m_bValid && !(modelP->m_vboDataHandle && modelP->m_vboIndexHandle))
		modelP->m_bValid = 0;
	if (!modelP->m_bValid) {
		i = G3BuildModel (objP, nModel, pp, modelBitmaps, pObjColor, 0);
		if (i <= 0) {
			if (!i)
				modelP->m_bValid = -1;
			return gameStates.render.bBuildModels;
			}
		}
	}

modelP->m_bHasTransparency |= (bEmissive << 2);

//#pragma omp critical (fastModelRender)
{
PROF_START
if (gameStates.render.bCloaked)
	 ogl.EnableClientStates (0, 0, 0, GL_TEXTURE0);
else
	ogl.EnableClientStates (1, 1, gameOpts->ogl.bObjLighting, GL_TEXTURE0);
if (bUseVBO)
	glBindBufferARB (GL_ARRAY_BUFFER_ARB, modelP->m_vboDataHandle);
else {
	modelP->m_vbVerts.SetBuffer (reinterpret_cast<CFloatVector3*> (modelP->m_vertBuf [0].Buffer ()), 1, modelP->m_nFaceVerts);
	modelP->m_vbNormals.SetBuffer (modelP->m_vbVerts.Buffer () + modelP->m_nFaceVerts, 1, modelP->m_nFaceVerts);
	modelP->m_vbColor.SetBuffer (reinterpret_cast<CFloatVector*> (modelP->m_vbNormals.Buffer () + modelP->m_nFaceVerts), 1, modelP->m_nFaceVerts);
	modelP->m_vbTexCoord.SetBuffer (reinterpret_cast<tTexCoord2f*> (modelP->m_vbColor.Buffer () + modelP->m_nFaceVerts), 1, modelP->m_nFaceVerts);
	}
#if G3_SW_SCALING
G3ScaleModel (nModel);
#else
#	if 0
if (bHires)
	gameData.models.vScale.SetZero ();
#	endif
#endif
if (!(gameOpts->ogl.bObjLighting || gameStates.render.bCloaked))
	G3LightModel (objP, nModel, xModelLight, xGlowValues, bHires);
if (bUseVBO) {
	if (!gameStates.render.bCloaked) {
		OglNormalPointer (GL_FLOAT, 0, G3_BUFFER_OFFSET (modelP->m_nFaceVerts * sizeof (CFloatVector3)));
		OglColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (modelP->m_nFaceVerts * 2 * sizeof (CFloatVector3)));
		OglTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (modelP->m_nFaceVerts * ((2 * sizeof (CFloatVector3) + sizeof (CFloatVector)))));
		}
	OglVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (0));
	if (modelP->m_vboIndexHandle)
		glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, modelP->m_vboIndexHandle);
	}
else {
	if (!gameStates.render.bCloaked) {
		OglTexCoordPointer (2, GL_FLOAT, 0, modelP->m_vbTexCoord.Buffer ());
		if (gameOpts->ogl.bObjLighting)
			OglNormalPointer (GL_FLOAT, 0, modelP->m_vbNormals.Buffer ());
		OglColorPointer (4, GL_FLOAT, 0, modelP->m_vbColor.Buffer ());
		}
	OglVertexPointer (3, GL_FLOAT, 0, modelP->m_vbVerts.Buffer ());
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

int32_t bRenderTransparency = bHires && !gameStates.render.bCloaked && (gameStates.render.nType == RENDER_TYPE_TRANSPARENCY) && modelP->m_bHasTransparency;

if (bRenderTransparency) {
	if (objP->info.nType != OBJ_DEBRIS) {
		if (modelP->m_bHasTransparency & 5)
			G3DrawModel (objP, nModel, nSubModel, modelBitmaps, animAnglesP, vOffsetP, bHires, bUseVBO, 1, nGunId, nBombId, nMissileId, nMissiles, 0);
		if (modelP->m_bHasTransparency & 10) {
			CFixVector vPos;
			PolyObjPos (objP, &vPos);
			glowRenderer.Begin (GLOW_HEADLIGHT, 2, true, 0.666f);
			if (!glowRenderer.SetViewport (GLOW_HEADLIGHT, vPos, 2 * X2F (objP->info.xSize))) 
				glowRenderer.Done (GLOW_HEADLIGHT);
			else {
				ogl.SetFaceCulling (false);
				G3DrawModel (objP, nModel, nSubModel, modelBitmaps, animAnglesP, vOffsetP, bHires, bUseVBO, 2, nGunId, nBombId, nMissileId, nMissiles, 0);
				ogl.SetFaceCulling (true);
				glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
				glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				glowRenderer.End ();
				}
			}
		}
	}
else {
	G3DrawModel (objP, nModel, nSubModel, modelBitmaps, animAnglesP, vOffsetP, bHires, bUseVBO, 0, nGunId, nBombId, nMissileId, nMissiles, 0);
	modelP->m_bRendered = 1;
	if (gameData.models.thrusters [nModel].nCount < 0)
		gameData.models.thrusters [nModel].nCount = -gameData.models.thrusters [nModel].nCount;
	if ((objP->info.nType != OBJ_DEBRIS) && bHires && modelP->m_bHasTransparency)
		transparencyRenderer.AddObject (objP);
	}

glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
ogl.SetTexturing (false);
if (gameStates.render.bCloaked)
	ogl.DisableClientStates (0, 0, 0, -1);
else
	ogl.DisableClientStates (1, 1, gameOpts->ogl.bObjLighting, -1);
#if DBG
if (gameOpts->render.debug.bWireFrame)
	glLineWidth (3.0f);
#endif

if (gameStates.render.bCartoonize && !bRenderTransparency && (!objP->IsWeapon () || objP->IsMissile ()))
	G3DrawModel (objP, nModel, nSubModel, modelBitmaps, animAnglesP, vOffsetP, bHires, bUseVBO, 0, nGunId, nBombId, nMissileId, nMissiles, 1);

#if 1 //!DBG
if (gameStates.render.nType != RENDER_TYPE_TRANSPARENCY) {
	if (objP && ((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT) || (objP->info.nType == OBJ_REACTOR))) {
		RenderModel::CModel*	modelP = gameData.models.renderModels [bHires] + nModel;

		if (modelP->m_bValid < 1) {
			if (!bHires)
				modelP = NULL;
			else {
				modelP = gameData.models.renderModels [0] + nModel;
				if (modelP->m_bValid < 1)
					modelP = NULL;
				}
			}
		if (modelP) {
			transformation.Begin (objP->info.position.vPos, objP->info.position.mOrient);
			RenderModel::CSubModel*	subModelP = modelP->m_subModels.Buffer ();
			for (int32_t i = 0, j = modelP->m_nSubModels; i < j; i++, subModelP++)
				if ((subModelP->m_nParent == -1) && !G3FilterSubModel (objP, subModelP, nGunId, nBombId, nMissileId, nMissiles))
					G3RenderDamageLightning (objP, nModel, i, animAnglesP, NULL, bHires);
			//	G3RenderDamageLightning (objP, nModel, 0, animAnglesP, NULL, bHires);
			transformation.End ();
			glowRenderer.End ();
			}
		}
	}
#endif
#if DBG
if (gameOpts->render.debug.bWireFrame)
	glLineWidth (1.0f);
#endif
ogl.ClearError (0);
PROF_END(ptRenderObjectMeshes)
}
return 1;
}

//------------------------------------------------------------------------------
//eof
