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
#include "addon_bitmaps.h"

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

void G3DynLightModel (CObject *pObj, RenderModel::CModel *pModel, uint16_t iVerts, uint16_t nVerts, uint16_t iFaceVerts, uint16_t nFaceVerts)
{
	CFloatVector				vPos, vVertex;
	CFloatVector3*				pv, * pn;
	RenderModel::CVertex*	pmv;
	CFaceColor*					pColor;
	float							fAlpha = gameStates.render.grAlpha;
	int32_t						h, i, bEmissive = pObj->IsProjectile ();

if (!gameStates.render.bBrightObject) {
	vPos.Assign (pObj->info.position.vPos);
	for (i = iVerts, pv = pModel->m_vertices + iVerts, pn = pModel->m_vertNorms + iVerts, pColor = pModel->m_color + iVerts;
		  i < nVerts;
		  i++, pv++, pn++, pColor++) {
		pColor->index = 0;
		vVertex = vPos + *reinterpret_cast<CFloatVector*> (pv);
		GetVertexColor (-1, -1, i, reinterpret_cast<CFloatVector3*> (pn), vVertex.XYZ (), pColor, NULL, 1, 0, 0);
		}
	}
for (i = iFaceVerts, h = iFaceVerts, pmv = pModel->m_faceVerts + iFaceVerts; i < nFaceVerts; i++, h++, pmv++) {
	if (gameStates.render.bBrightObject || bEmissive) {
		pModel->m_vbColor [h] = pmv->m_baseColor;
		pModel->m_vbColor [h].Alpha () = fAlpha;
		}
	else if (pmv->m_bTextured)
		pModel->m_vbColor [h].Assign (pModel->m_color [pmv->m_nIndex]);
	else {
		pColor = pModel->m_color + pmv->m_nIndex;
		pModel->m_vbColor [h].Red () = pmv->m_baseColor.Red () * pColor->Red ();
		pModel->m_vbColor [h].Green () = pmv->m_baseColor.Green () * pColor->Green ();
		pModel->m_vbColor [h].Blue () = pmv->m_baseColor.Blue () * pColor->Blue ();
		pModel->m_vbColor [h].Alpha () = pmv->m_baseColor.Alpha ();
		}
	}
}

//------------------------------------------------------------------------------

void G3LightModel (CObject *pObj, int32_t nModel, fix xModelLight, fix *xGlowValues, int32_t bHires)
{
	RenderModel::CModel*		pModel = gameData.modelData.renderModels [bHires] + nModel;
	RenderModel::CVertex*	pmv;
	RenderModel::CFace*		pmf;
	CFloatVector				baseColor, *pColor;
	float							fLight, fAlpha = gameStates.render.grAlpha;
	int32_t						h, i, j, l;
	int32_t						bEmissive = (pObj->info.nType == OBJ_MARKER) || (pObj->IsProjectile ());

#if DBG
if (pObj->Index () == nDbgObj)
	BRP;
#endif
#if 0
if (xModelLight > I2X (1))
	xModelLight = I2X (1);
#endif
if (SHOW_DYN_LIGHT && (gameOpts->ogl.bObjLighting ||
    (gameOpts->ogl.bLightObjects && (gameOpts->ogl.bLightPowerups || (pObj->info.nType == OBJ_PLAYER) || (pObj->info.nType == OBJ_ROBOT))))) {
	tiRender.pObj = pObj;
	tiRender.pModel = pModel;
	if (!RunRenderThreads (rtPolyModel))
		G3DynLightModel (pObj, pModel, 0, pModel->m_nVerts, 0, pModel->m_nFaceVerts);
	}
else {
	if (gameData.objData.color.index)
		baseColor.Assign (gameData.objData.color);
	else
		baseColor.Set (1.0f, 1.0f, 1.0f);
	baseColor.Alpha () = fAlpha;
	for (i = pModel->m_nFaces, pmf = pModel->m_faces.Buffer (); i; i--, pmf++) {
		if (pmf->m_nBitmap >= 0)
			pColor = &baseColor;
		else
			pColor = NULL;
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
		for (j = pmf->m_nVerts, h = pmf->m_nIndex, pmv = pModel->m_faceVerts + pmf->m_nIndex; j; j--, h++, pmv++) {
#if G3_DRAW_ARRAYS
			if (pColor) {
				pModel->m_vbColor [h].Red () = pColor->Red () * fLight;
				pModel->m_vbColor [h].Green () = pColor->Green () * fLight;
				pModel->m_vbColor [h].Blue () = pColor->Blue () * fLight;
				pModel->m_vbColor [h].Alpha () = fAlpha;
				}
			else {
				pModel->m_vbColor [h].Red () = pmv->m_baseColor.Red () * fLight;
				pModel->m_vbColor [h].Green () = pmv->m_baseColor.Green () * fLight;
				pModel->m_vbColor [h].Blue () = pmv->m_baseColor.Blue () * fLight;
				pModel->m_vbColor [h].Alpha () = fAlpha;
				}
#else
			if (pColor) {
				pmv->m_renderColor.Red () = pColor->Red () * fLight;
				pmv->m_renderColor.Green () = pColor->Green () * fLight;
				pmv->m_renderColor.Blue () = pColor->Blue () * fLight;
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
	RenderModel::CModel	*pModel = gameData.modelData.renderModels [bHires] + nModel;
	CFloatVector			fScale;
	int32_t					i;
	CFloatVector3			*pv;
	RenderModel::CVertex	*pmv;

if (gameData.modelData.vScale.IsZero ())
	fScale.Create (1,1,1);
else
	fScale.Assign (gameData.modelData.vScale);
if (pModel->m_fScale == fScale)
	return;
fScale /= pModel->m_fScale;
for (i = pModel->m_nVerts, pv = pModel->m_vertices; i; i--, pv++) {
	pv->p.x *= fScale;
	pv->p.y *= fScale;
	pv->p.z *= fScale;
	}
for (i = pModel->m_nFaceVerts, pmv = pModel->m_faceVerts; i; i--, pmv++)
	pmv->m_vertex = pModel->m_vertices [pmv->m_nIndex];
pModel->m_fScale *= fScale;
}

#endif

//------------------------------------------------------------------------------

void G3GetThrusterPos (CObject *pObj, int16_t nModel, RenderModel::CFace *pmf, CFixVector *pvOffset,
							  CFixVector *vNormal, int32_t nRad, int32_t bHires, uint8_t nType = 255)
{
ENTER (0, 0);
	RenderModel::CModel		*pModel = gameData.modelData.renderModels [bHires] + nModel;
	RenderModel::CVertex		*pModelVertex = NULL;
	CFloatVector3				v = CFloatVector3::ZERO, vn, vo, vForward = CFloatVector3::Create(0,0,1);
	CModelThrusters*			pThruster = gameData.modelData.thrusters + nModel;
	int32_t						i, j = 0, nCount = -pThruster->nCount;
	float							h, nSize;

if (!pObj)
	RETURN
if (nCount < 0) {
	if (pModel->m_bRendered && gameData.modelData.vScale.IsZero ())
		RETURN
	nCount = 0;
	}	
else if (nCount >= MAX_THRUSTERS)
	RETURN
vn.Assign (pmf ? pmf->m_vNormal : *vNormal);
if (!IsPlayerShip (nModel) && (CFloatVector3::Dot (vn, vForward) > -1.0f / 3.0f))
	RETURN
if (pmf) {
	for (i = 0, j = pmf->m_nVerts, pModelVertex = pModel->m_faceVerts + pmf->m_nIndex; i < j; i++)
		v += pModelVertex [i].m_vertex;
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
if (pvOffset) {
	vo.Assign (*pvOffset);
	v += vo;
	}
#endif
#endif
if (nCount && (v.v.coord.x == pThruster->vPos [0].v.coord.x) && (v.v.coord.y == pThruster->vPos [0].v.coord.y) && (v.v.coord.z == pThruster->vPos [0].v.coord.z))
	RETURN
pThruster->vPos [nCount].Assign (v);
if (pvOffset)
	v -= vo;
pThruster->vDir [nCount] = *vNormal;
pThruster->vDir [nCount].Neg ();
pThruster->nType [nCount] = nType;
//if (!nCount) 
	{
	if (!pmf)
		pThruster->fSize [nCount] = X2F (nRad);
	else {
		for (i = 0, nSize = 1000000000; i < j; i++)
			if (nSize > (h = CFloatVector3::Dist (v, pModelVertex [i].m_vertex)))
				nSize = h;
		pThruster->fSize [nCount] = nSize;// * 1.25f;
		}
	}
pThruster->nCount = -(++nCount);
RETURN
}

//------------------------------------------------------------------------------

static int32_t bCenterGuns [] = {0, 1, 1, 0, 0, 0, 1, 1, 0, 1};

int32_t G3FilterSubModel (CObject *pObj, RenderModel::CSubModel *pSubModel, int32_t nGunId, int32_t nBombId, int32_t nMissileId, int32_t nMissiles)
{
ENTER (0, 0);
	int32_t nId = pObj->info.nId;

if (!pSubModel->m_bRender)
	RETVAL (0)
#if 1
if (pSubModel->m_bFlare)
	RETVAL (1)
#endif
if (pSubModel->m_nGunPoint >= 0)
	RETVAL (1)
if (pSubModel->m_bBullets)
	RETVAL (1)
#if 1
if (pSubModel->m_bThruster && ((pSubModel->m_bThruster & (REAR_THRUSTER | FRONTAL_THRUSTER)) != (REAR_THRUSTER | FRONTAL_THRUSTER)))
	RETVAL (1)
#endif
if (pSubModel->m_bHeadlight)
	RETVAL (!HeadlightIsOn (nId))
if (pSubModel->m_bBombMount)
	RETVAL (nBombId == 0)
if (pSubModel->m_bWeapon) {
	CPlayerData	*pPlayer = gameData.multiplayer.players + nId;
	int32_t		bLasers = (nGunId == LASER_INDEX) || (nGunId == SUPER_LASER_INDEX);
	int32_t		bSuperLasers = pPlayer->HasSuperLaser ();
	int32_t		bQuadLasers = gameData.multiplayer.weaponStates [N_LOCALPLAYER].bQuadLasers;
	int32_t		bCenterGun = bCenterGuns [nGunId];
	int32_t		nWingtip = bQuadLasers ? bSuperLasers : 2; //gameOpts->render.ship.nWingtip;

	gameOpts->render.ship.nWingtip = nWingtip;
	if (nWingtip == 0)
		nWingtip = bLasers && bSuperLasers && bQuadLasers;
	else if (nWingtip == 1)
		nWingtip = !bLasers || bSuperLasers;
	
	if (EGI_FLAG (bShowWeapons, 0, 1, 0)) {
		if (pSubModel->m_nGun == nGunId + 1) {
			if (pSubModel->m_nGun == FUSION_INDEX + 1) {
				if ((pSubModel->m_nWeaponPos == 3) && !gameData.multiplayer.weaponStates [N_LOCALPLAYER].bTripleFusion)
					RETVAL (1)
				}
			else if (bLasers) {
				if ((pSubModel->m_nWeaponPos > 2) && !bQuadLasers && (nWingtip != bSuperLasers))
					RETVAL (1)
				}
			}
		else if (pSubModel->m_nGun == LASER_INDEX + 1) {
			if (nWingtip)
				RETVAL (1)
			RETVAL (!bCenterGun && (pSubModel->m_nWeaponPos < 3))
			}
		else if (pSubModel->m_nGun == SUPER_LASER_INDEX + 1) {
			if (nWingtip != 1)
				RETVAL (1)
			RETVAL (!bCenterGun && (pSubModel->m_nWeaponPos < 3))
			}
		else if (!pSubModel->m_nGun) {
			if (bLasers && bQuadLasers)
				RETVAL (1)
			if (pSubModel->m_nType != gameOpts->render.ship.nWingtip)
				RETVAL (1)
			RETVAL (0)
			}
		else if (pSubModel->m_nBomb == nBombId)
			RETVAL ((nId == N_LOCALPLAYER) && !AllowedToFireMissile (nId, 0))
		else if (pSubModel->m_nMissile == nMissileId) {
			if (pSubModel->m_nWeaponPos > nMissiles)
				RETVAL (1)
			else {
				static int32_t nMslPos [] = {-1, 1, 0, 3, 2};
				int32_t nLaunchPos = gameData.multiplayer.weaponStates [nId].nMslLaunchPos;
				RETVAL ((nId == N_LOCALPLAYER) && !AllowedToFireMissile (nId, 0) && (nLaunchPos == (nMslPos [(int32_t) pSubModel->m_nWeaponPos])))
				}
			}
		else
			RETVAL (1)
		}
	else {
		if (pSubModel->m_nGun == 1)
			RETVAL (0)
		if ((pSubModel->m_nGun < 0) && (pSubModel->m_nMissile == 1))
			RETVAL (0)
		RETVAL (1)
		}
	}
RETVAL (0)
}

//------------------------------------------------------------------------------

static inline int32_t ObjectHasThruster (CObject *pObj)
{
return (pObj->info.nType == OBJ_PLAYER) || (pObj->info.nType == OBJ_ROBOT) || pObj->IsMissile ();
}

//------------------------------------------------------------------------------

int32_t G3AnimateSubModel (CObject *pObj, RenderModel::CSubModel *pSubModel, int16_t nModel)
{
ENTER (0, 0);
	tFiringData*	pFiringData;
	float				nTimeout, y;
	int32_t			nDelay;

if (!pSubModel->m_nFrames)
	RETVAL (0)
if ((pSubModel->m_bThruster & (REAR_THRUSTER | FRONTAL_THRUSTER)) == (REAR_THRUSTER | FRONTAL_THRUSTER)) {
	nTimeout = gameStates.gameplay.slowmo [0].fSpeed;
	glPushMatrix ();
	CFloatVector vCenter;
	vCenter.Assign (pSubModel->m_vCenter);
	glTranslatef (vCenter .v.coord.x, vCenter .v.coord.y, vCenter .v.coord.z);
	glRotatef (-360.0f * 5.0f * float (pSubModel->m_iFrame) / float (pSubModel->m_nFrames), 0, 0, 1);
	glTranslatef (-vCenter .v.coord.x, -vCenter .v.coord.y, -vCenter .v.coord.z);
	if (gameStates.app.nSDLTicks [0] - pSubModel->m_tFrame > nTimeout) {
		pSubModel->m_tFrame = gameStates.app.nSDLTicks [0];
		pSubModel->m_iFrame = (pSubModel->m_iFrame + 1) % pSubModel->m_nFrames;
		}
	}
else {
	nTimeout = 25 * gameStates.gameplay.slowmo [0].fSpeed;
	pFiringData = gameData.multiplayer.weaponStates [pObj->info.nId].firing;
	if (gameData.weaponData.nPrimary == VULCAN_INDEX)
		nTimeout /= 2;
	if (pFiringData->nStop > 0) {
		nDelay = gameStates.app.nSDLTicks [0] - pFiringData->nStop;
		if (nDelay > GATLING_DELAY)
			RETVAL (0)
		nTimeout += (float) nDelay / 10;
		}
	else if (pFiringData->nStart > 0) {
		nDelay = GATLING_DELAY - pFiringData->nDuration;
		if (nDelay > 0)
			nTimeout += (float) nDelay / 10;
		}
	else
		RETVAL (0)
	glPushMatrix ();
	y = X2F (pSubModel->m_vCenter .v.coord.y);
	glTranslatef (0, y, 0);
	glRotatef (360 * float (pSubModel->m_iFrame) / float (pSubModel->m_nFrames), 0, 0, 1);
	glTranslatef (0, -y, 0);
	if (gameStates.app.nSDLTicks [0] - pSubModel->m_tFrame > nTimeout) {
		pSubModel->m_tFrame = gameStates.app.nSDLTicks [0];
		pSubModel->m_iFrame = (pSubModel->m_iFrame + 1) % pSubModel->m_nFrames;
		}
	}
RETVAL (1)
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

void G3TransformSubModel (CObject *pObj, int16_t nModel, int16_t nSubModel, CAngleVector *animAnglesP, CFixVector *pvOffset, 
								  int32_t bHires, int32_t nGunId, int32_t nBombId, int32_t nMissileId, int32_t nMissiles, int32_t bEdges)
{
ENTER (0, 0);
	RenderModel::CModel*		pModel = gameData.modelData.renderModels [bHires] + nModel;
	RenderModel::CSubModel*	pSubModel = pModel->m_subModels + nSubModel;
	CAngleVector				va = animAnglesP ? animAnglesP [pSubModel->m_nAngles] : CAngleVector::ZERO;
	CFixVector					vo;
	int32_t						i, j;

if (pSubModel->m_bThruster)
	RETURN
if (G3FilterSubModel (pObj, pSubModel, nGunId, nBombId, nMissileId, nMissiles))
	RETURN
vo = pSubModel->m_vOffset;
if (!gameData.modelData.vScale.IsZero ())
	vo *= gameData.modelData.vScale;
#if 1
if (pvOffset) {
	transformation.Begin (vo, va, __FILE__, __LINE__);
	vo += *pvOffset;
	}
#endif
if (bEdges) {
	RenderModel::CModelEdge *pEdge = pSubModel->m_edges.Buffer ();
	for (i = pSubModel->m_nEdges; i; i--, pEdge++)
		pEdge->Transform ();
	}
else {
	RenderModel::CFace *pFace = pSubModel->m_faces;
	for (i = 0; i < pSubModel->m_nFaces; i++, pFace++) {
		transformation.Transform (pFace->m_vNormalf [1], pFace->m_vNormalf [0]);
		transformation.Transform (pFace->m_vCenterf [1], pFace->m_vCenterf [0]);
		}
	for (i = 0, j = pSubModel->m_nVertexIndex [1]; i < pSubModel->m_nVertices; i++, j++) {
		uint16_t h = pModel->m_vertexOwner [j].m_nVertex;
		transformation.Transform (pModel->m_vertices [h], pModel->m_vertices [h]);
		}
	}
for (i = 0, j = pModel->m_nSubModels, pSubModel = pModel->m_subModels.Buffer (); i < j; i++, pSubModel++) {
	if (pSubModel->m_nParent == nSubModel)
		G3TransformSubModel (pObj, nModel, i, animAnglesP, &vo, bHires, nGunId, nBombId, nMissileId, nMissiles, bEdges);
	}
transformation.End (__FILE__, __LINE__);
RETURN
}

//------------------------------------------------------------------------------

void G3DrawSubModel (CObject *pObj, int16_t nModel, int16_t nSubModel, int16_t nExclusive, CArray<CBitmap*>& modelBitmaps,
						   CAngleVector *animAnglesP, CFixVector *pvOffset, int32_t bHires, int32_t bUseVBO, int32_t nPass, int32_t bTranspFilter,
							int32_t nGunId, int32_t nBombId, int32_t nMissileId, int32_t nMissiles, int32_t bEdges, int32_t bBlur)
{
ENTER (0, 0);
	RenderModel::CModel		* pModel = gameData.modelData.renderModels [bHires] + nModel;
	RenderModel::CSubModel	* pSubModel = pModel->m_subModels + nSubModel;
	CBitmap*						pBm = NULL;
	CAngleVector				va = animAnglesP ? animAnglesP [pSubModel->m_nAngles] : CAngleVector::ZERO;
	CFixVector					vo;
	int32_t						h, i, j, bTranspState, bRestoreMatrix = 0, bTextured = gameOpts->render.debug.bTextures /*&& !gameStates.render.bCloaked*/&& !(gameOpts->render.debug.bWireFrame & 2),
									bGetThruster = !(nPass || bTranspFilter) && ObjectHasThruster (pObj);
	int16_t						nId, nBitmap = -1, nTeamColor;
	uint16_t						nFaceVerts, nVerts, nIndex;

#if DBG
if (nModel == nDbgModel)
	nDbgModel = nModel;
if (pObj) {
	if (ObjIdx (pObj) == nDbgObj)
		BRP;
	if (pObj->info.nSegment == nDbgSeg)
		BRP;
	if (pObj->info.nType == OBJ_DEBRIS)
		BRP;
	}
#endif

if (pObj->info.nType == OBJ_PLAYER)
	nTeamColor = IsMultiGame ? IsTeamGame ? GetTeam (pObj->info.nId) + 1 : PlayerColor (pObj->Index ()) : gameOpts->render.ship.nColor;
else
	nTeamColor = 0;

if (pSubModel->m_bThruster) {
	if (!nPass) {
		vo = pSubModel->m_vOffset + pSubModel->m_vCenter;
		G3GetThrusterPos (pObj, nModel, NULL, &vo, &pSubModel->m_faces->m_vNormal, pSubModel->m_nRad, bHires, pSubModel->m_bThruster);
		}
	if (!pSubModel->m_nFrames)
		RETURN
	}
if (G3FilterSubModel (pObj, pSubModel, nGunId, nBombId, nMissileId, nMissiles))
	RETURN

vo = pSubModel->m_vOffset;
if (!gameData.modelData.vScale.IsZero ())
	vo *= gameData.modelData.vScale;

if (pvOffset && (nExclusive < 0)) {
	transformation.Begin (vo, va, __FILE__, __LINE__);
	vo += *pvOffset;
	}

bRestoreMatrix = G3AnimateSubModel (pObj, pSubModel, nModel);
// render the faces
#if 0
pSubModel = pModel->m_subModels + nSubModel;
if (pSubModel->m_bBillboard)
#endif
if ((nExclusive < 0) || (nSubModel == nExclusive)) {
	pSubModel = pModel->m_subModels + nSubModel;
	if (pSubModel->m_bBillboard) {
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
	if (bEdges) {
		if (!pSubModel->m_bThruster) {
			CFloatVector vViewer;
			vViewer.Assign (transformation.m_info.viewer);
			float fSize = X2F (pObj->Size ());
			float d = Max (0.0f, X2F (CFixVector::Dist (pObj->Position (), transformation.m_info.viewer)) - fSize);
			d /= sqrt (X2F (pObj->Size ()));
			RenderModel::CModelEdge* pEdge = pSubModel->m_edges.Buffer ();
			int32_t nScale = 0xFFFF, nEdgeFilter = pObj->IsWeapon () ? 0 : bHires || (fSize > 9.0f); //0 : 2; //bHires;
			gameData.segData.edgeVertexData [0].Reset ();
			gameData.segData.edgeVertexData [1].Reset ();
			for (i = pSubModel->m_nEdges; i; i--, pEdge++) {
				int32_t s = pEdge->Prepare (vViewer, nEdgeFilter, d);
				if ((s >= 0) && (nScale > s))
					nScale = s;
				}
			if (nScale < 0xFFFF)
				RenderMeshOutline (nScale/*, Max (1.0f, X2F (pObj->Size () / 4.5f))*/);
			}	
		}
	else {
	if (gameOpts->render.debug.bWireFrame & 2)
		glColor3f (1.0f, 0.5f, 0.0f);

		RenderModel::CFace* pFace = pSubModel->m_faces;
		for (i = pSubModel->m_nFaces; i; ) {
			if (bTextured && (nBitmap != (gameStates.render.bCloaked ? 0 : pFace->m_nBitmap))) {
				nBitmap = gameStates.render.bCloaked ? 0 : pFace->m_nBitmap;
				if (nBitmap < 0)
					ogl.SetTexturing (false);
				else {
					if (gameStates.render.bCloaked) {
						pBm = shield [0].Bitmap ();
						float c = 1.0f - gameStates.render.grAlpha * gameStates.render.grAlpha; //bBlur ? 1.0f - gameStates.render.grAlpha * gameStates.render.grAlpha : pBm ? 1.0f - gameStates.render.grAlpha : 0.0f;
						//ogl.SetBlendMode (bBlur ? OGL_BLEND_REPLACE : OGL_BLEND_MULTIPLY);
						glColor4f (c, c, c, gameStates.render.grAlpha);
						}
					else if (!bHires)
						pBm = modelBitmaps [nBitmap];
					else {
						pBm = pModel->m_textures + nBitmap;
#if 0
						if (!nPass)
							ogl.SetBlendMode (pSubModel->m_bFlare);
#endif
						if (nTeamColor && pBm->Team () && (0 <= (h = pModel->m_teamTextures [nTeamColor % MAX_PLAYER_COLORS]))) {
							nBitmap = h;
							pBm = pModel->m_textures + nBitmap;
							}
						}
					if (!pBm)
						ogl.SetTexturing (false);
					else {
#if 0
						pBm->AvgColor ();
						CRGBColor *pColor = pBm->GetAvgColor ();
						if (!gameStates.render.bCloaked)
							glColor4f ((float) pColor->Red () / 255.0f, (float) pColor->Green () / 255.0f, (float) pColor->Blue () / 255.0f, gameStates.render.grAlpha);
						ogl.SetTexturing (false);
						ogl.DisableClientState (GL_COLOR_ARRAY);
#else
						ogl.SelectTMU (GL_TEXTURE0, true);
						ogl.SetTexturing (true);
						pBm = pBm->Override (-1);
						if (pBm->Frames ())
							pBm = pBm->CurFrame ();
						if (pBm->Bind (1))
							continue;
						pBm->Texture ()->Wrap (GL_REPEAT);
						glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#endif
						}
					}
				}
			nIndex = pFace->m_nIndex;
			if (bHires) {
				if (!(bTranspState = (pModel->m_bHasTransparency >> 2)))
					bTranspState = !pBm ? 0 : gameStates.render.bCloaked ? 0 : pSubModel->m_bThruster ? 0 : (pSubModel->m_bGlow || pSubModel->m_bFlare) ? 2 : ((pBm->Flags () & BM_FLAG_TRANSPARENT) != 0) ? 1 : 0;
				if (bTranspState != bTranspFilter) {
					if (bTranspState)
						pModel->m_bHasTransparency |= bTranspState;
					pFace++;
					i--;
					continue;
					}
				}
			if ((nFaceVerts = pFace->m_nVerts) > 4) {
				if (bGetThruster && pFace->m_bThruster)
					G3GetThrusterPos (pObj, nModel, pFace, &vo, &pFace->m_vNormal, 0, bHires);
				nVerts = nFaceVerts;
				pFace++;
				i--;
				}
			else {
				nId = pFace->m_nId;
				nVerts = 0;
				do {
					if (bGetThruster && pFace->m_bThruster)
						G3GetThrusterPos (pObj, nModel, pFace, &vo, &pFace->m_vNormal, 0, bHires);
					nVerts += nFaceVerts;
					pFace++;
					i--;
					} while (i && (pFace->m_nId == nId));
				}

#ifdef _WIN32
			if (glDrawRangeElements)
#endif
#if 0 //DBG
				if (bUseVBO)
#	if 1
					glDrawRangeElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
												0, pModel->m_nFaceVerts - 1, nVerts, GL_UNSIGNED_SHORT,
												G3_BUFFER_OFFSET (nIndex * sizeof (int16_t)));
#	else
					glDrawElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
										 nVerts, GL_UNSIGNED_SHORT,
										 G3_BUFFER_OFFSET (nIndex * sizeof (int16_t)));
#	endif
				else
					glDrawRangeElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
												nIndex, nIndex + nVerts - 1, nVerts, GL_UNSIGNED_SHORT,
												pModel->m_index [0] + nIndex);
#else // DBG
				if (bUseVBO)
					glDrawRangeElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
												0, pModel->m_nFaceVerts - 1, nVerts, GL_UNSIGNED_SHORT,
												G3_BUFFER_OFFSET (nIndex * sizeof (int16_t)));
				else
					glDrawRangeElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
												nIndex, nIndex + nVerts - 1, nVerts, GL_UNSIGNED_SHORT,
												pModel->m_index [0] + nIndex);
#endif // DBG
#ifdef _WIN32
			else
				if (bUseVBO)
					glDrawElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
										 nVerts, GL_UNSIGNED_SHORT, G3_BUFFER_OFFSET (nIndex * sizeof (int16_t)));
				else
					glDrawElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN,
										 nVerts, GL_UNSIGNED_SHORT, pModel->m_index + nIndex);
#endif //_WIN32
			}
		}
	}

#if G3_DRAW_SUBMODELS
// render any dependent submodels
if ((nExclusive < 0) || (nSubModel != nExclusive)) {
	for (i = 0, j = pModel->m_nSubModels, pSubModel = pModel->m_subModels.Buffer (); i < j; i++, pSubModel++)
		if (pSubModel->m_nParent == nSubModel)
			G3DrawSubModel (pObj, nModel, i, nExclusive, modelBitmaps, animAnglesP, &vo, bHires,bUseVBO, nPass, bTranspFilter, nGunId, nBombId, nMissileId, nMissiles, bEdges, bBlur);
	}
if (bRestoreMatrix)
	glPopMatrix ();
#endif
#if 1
if (pvOffset && (nExclusive < 0))
	transformation.End (__FILE__, __LINE__);
#endif
RETURN
}

//------------------------------------------------------------------------------

void G3DrawModel (CObject *pObj, int16_t nModel, int16_t nSubModel, CArray<CBitmap*>& modelBitmaps,
						CAngleVector *animAnglesP, CFixVector *pvOffset, int32_t bHires, int32_t bUseVBO, int32_t bTranspFilter,
						int32_t nGunId, int32_t nBombId, int32_t nMissileId, int32_t nMissiles, int32_t bEdges, int32_t bBlur)
{
ENTER (0, 0);
	RenderModel::CModel	* pModel;
	CDynLight				* pLight;
	int32_t					nPass, iLight, nLights, nLightRange;
	int32_t					bBright = bEdges || gameStates.render.bFullBright || (pObj && (pObj->info.nType == OBJ_MARKER));
	int32_t					bEmissive = !bEdges && pObj && (pObj->IsProjectile ());
	int32_t					bLighting = !bEdges && SHOW_DYN_LIGHT && gameOpts->ogl.bObjLighting && (bTranspFilter < 2) && !(gameStates.render.bCloaked || bEmissive || bBright) && !(gameOpts->render.debug.bWireFrame & 2);
	GLenum					hLight;
	float						fBrightness, fLightScale = gameData.modelData.nLightScale ? X2F (gameData.modelData.nLightScale) : 1.0f;
	CFloatVector			color;
	CDynLightIndex			* pLightIndex = bLighting ? &lightManager.Index (0,0) : NULL;
	CActiveDynLight		* pActiveLights = pLightIndex ? lightManager.Active (0) + pLightIndex->nFirst : NULL;
	tObjTransformation	* pPos = OBJPOS (pObj);

#if POLYGONAL_OUTLINE
if (bEdges && bPolygonalOutline) // only required if not transforming model outlines via OpenGL when rendering
	ogl.SetTransform (0);
else
#endif
	ogl.SetupTransform (1);
if (bLighting) {
	nLights = pLightIndex->nActive;
	if (nLights > gameStates.render.nMaxLightsPerObject)
		nLights = gameStates.render.nMaxLightsPerObject;
	ogl.EnableLighting (0);
	}
else {
	nLights = 1;
	}	
ogl.SetBlending (true);
ogl.SetDepthWrite (false);
if (bEmissive || (bTranspFilter == 2))
	ogl.SetBlendMode (OGL_BLEND_ADD);
else if (gameStates.render.bCloaked) {
	ogl.SetBlendMode (bBlur ? OGL_BLEND_REPLACE : OGL_BLEND_MULTIPLY);
	ogl.SetDepthWrite (true);
	}
else if (bTranspFilter)
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
else {
	ogl.SetBlendMode (OGL_BLEND_REPLACE);
	ogl.SetDepthWrite (true);
	}

if (!bLighting || (pLightIndex->nLast < 0))
	nLightRange = 0;
else
	nLightRange = pLightIndex->nLast - pLightIndex->nFirst + 1;

for (nPass = 0; ((nLightRange > 0) && (nLights > 0)) || !nPass; nPass++) {
	if (bLighting) {
		if (nPass) {
			ogl.SetBlendMode (OGL_BLEND_ADD_WEAK);
			ogl.SetDepthWrite (false);
			}
		for (iLight = 0; (nLightRange > 0) && (iLight < 8) && nLights; pActiveLights++, nLightRange--) {
#if DBG
			if (pActiveLights - lightManager.Active (0) >= MAX_OGL_LIGHTS)
				break;
			if (pActiveLights < lightManager.Active (0))
				break;
#endif
			if (nLights < 0) {
				CFaceColor* pSegColor = lightManager.AvgSgmColor (pObj->info.nSegment, NULL, 0);
				CFloatVector3 vPos;
				hLight = GL_LIGHT0 + iLight++;
				glEnable (hLight);
				vPos.Assign (pObj->info.position.vPos);
				glLightfv (hLight, GL_POSITION, reinterpret_cast<GLfloat*> (&vPos));
				glLightfv (hLight, GL_DIFFUSE, reinterpret_cast<GLfloat*> (&pSegColor->v.color));
				glLightfv (hLight, GL_SPECULAR, reinterpret_cast<GLfloat*> (&pSegColor->v.color));
				glLightf (hLight, GL_CONSTANT_ATTENUATION, 0.1f);
				glLightf (hLight, GL_LINEAR_ATTENUATION, 0.1f);
				glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.01f);
				nLights = 0;
				}
			else if ((pLight = lightManager.GetActive (pActiveLights, 0))) {
#if DBG
				if ((nDbgSeg >= 0) && (pLight->info.nObject >= 0) && (OBJECT (pLight->info.nObject)->info.nSegment == nDbgSeg))
					BRP;
#endif
				hLight = GL_LIGHT0 + iLight++;
				glEnable (hLight);
	//			sprintf (szLightSources + strlen (szLightSources), "%d ", (pLight->nObject >= 0) ? -pLight->nObject : pLight->nSegment);
				fBrightness = pLight->info.fBrightness * fLightScale;
				memcpy (&color.v.color, &pLight->info.color, sizeof (color.v.color));
				color.Red () *= fLightScale;
				color.Green () *= fLightScale;
				color.Blue () *= fLightScale;
				glLightfv (hLight, GL_POSITION, reinterpret_cast<GLfloat*> (pLight->render.vPosf));
				glLightfv (hLight, GL_DIFFUSE, reinterpret_cast<GLfloat*> (&color));
				glLightfv (hLight, GL_SPECULAR, reinterpret_cast<GLfloat*> (&color));
				if (pLight->info.bSpot) {
					glLighti (hLight, GL_SPOT_EXPONENT, 12);
					glLighti (hLight, GL_SPOT_CUTOFF, 25);
					glLightfv (hLight, GL_SPOT_DIRECTION, reinterpret_cast<GLfloat*> (&pLight->info.vDirf));
					glLightf (hLight, GL_CONSTANT_ATTENUATION, 0.0f); //0.1f / fBrightness);
					glLightf (hLight, GL_LINEAR_ATTENUATION, 0.05f / fBrightness);
					glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.005f / fBrightness);
					}
				else {
					glLightf (hLight, GL_CONSTANT_ATTENUATION, 0.0f); //0.1f / fBrightness);
					if (X2F (CFixVector::Dist (pObj->info.position.vPos, pLight->info.vPos)) <= pLight->info.fRad) {
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

	transformation.Begin (pPos->vPos, pPos->mOrient, __FILE__, __LINE__);

	pModel = gameData.modelData.renderModels [bHires] + nModel;
	if (bEdges) {
		if (bHires) {
		for (int32_t i = 0; i < pModel->m_nSubModels; i++)
				if (pModel->m_subModels [i].m_nParent == -1) 
					G3TransformSubModel (pObj, nModel, i, animAnglesP, (nSubModel < 0) ? &pModel->m_subModels [0].m_vOffset : pvOffset,
												bHires, nGunId, nBombId, nMissileId, nMissiles, bEdges);
			}
		else {
			G3TransformSubModel (pObj, nModel, 0, animAnglesP, (nSubModel < 0) ? &pModel->m_subModels [0].m_vOffset : pvOffset,
										bHires, nGunId, nBombId, nMissileId, nMissiles, bEdges);
			}
		}

	if (bHires) {
		for (int32_t i = 0; i < pModel->m_nSubModels; i++)
			if (pModel->m_subModels [i].m_nParent == -1) {
				G3DrawSubModel (pObj, nModel, i, nSubModel, modelBitmaps, animAnglesP, (nSubModel < 0) ? &pModel->m_subModels [0].m_vOffset : pvOffset,
									 bHires, bUseVBO, nPass, bTranspFilter, nGunId, nBombId, nMissileId, nMissiles, bEdges, bBlur);
				}
		}
	else {
		G3DrawSubModel (pObj, nModel, 0, nSubModel, modelBitmaps, animAnglesP, (nSubModel < 0) ? &pModel->m_subModels [0].m_vOffset : pvOffset,
							 bHires, bUseVBO, nPass, bTranspFilter, nGunId, nBombId, nMissileId, nMissiles, bEdges, bBlur);
		}
	transformation.End (__FILE__, __LINE__);
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
RETURN
}

//------------------------------------------------------------------------------

void G3RenderDamageLightning (CObject *pObj, int16_t nModel, int16_t nSubModel, CAngleVector *animAnglesP, CFixVector *pvOffset, int32_t bHires)
{
ENTER (0, 0);
if (!(SHOW_LIGHTNING (1) && gameOpts->render.lightning.bDamage))
	RETURN

	RenderModel::CModel*		pModel;
	RenderModel::CSubModel*	pSubModel;
	RenderModel::CFace*		pmf;
	const CAngleVector*		va;
	CFixVector					vo;
	int32_t						i, j;

pModel = gameData.modelData.renderModels [bHires] + nModel;
if (pModel->m_bValid < 1) {
	if (!bHires)
		RETURN
	pModel = gameData.modelData.renderModels [0] + nModel;
	if (pModel->m_bValid < 1)
		RETURN
	}
pSubModel = pModel->m_subModels + nSubModel;
va = animAnglesP ? animAnglesP + pSubModel->m_nAngles : &CAngleVector::ZERO;
if (!pObj || (pObj->Damage () > 0.5f))
	RETURN
// set the translation
vo = pSubModel->m_vOffset;
if (!gameData.modelData.vScale.IsZero ())
	vo *= gameData.modelData.vScale;
if (pvOffset) {
	transformation.Begin (vo, *va, __FILE__, __LINE__);
	vo += *pvOffset;
	}
// render any dependent submodels
for (i = 0, j = pModel->m_nSubModels, pSubModel = pModel->m_subModels.Buffer (); i < j; i++, pSubModel++)
	if (pSubModel->m_nParent == nSubModel)
		G3RenderDamageLightning (pObj, nModel, i, animAnglesP, &vo, bHires);
// render the lightnings
for (pSubModel = pModel->m_subModels + nSubModel, i = pSubModel->m_nFaces, pmf = pSubModel->m_faces; i; i--, pmf++)
	lightningManager.RenderForDamage (pObj, NULL, pModel->m_faceVerts + pmf->m_nIndex, pmf->m_nVerts);
if (pvOffset)
	transformation.End (__FILE__, __LINE__);
RETURN
}

//------------------------------------------------------------------------------

int32_t G3RenderModel (CObject *pObj, int16_t nModel, int16_t nSubModel, CPolyModel* pp, CArray<CBitmap*>& modelBitmaps,
							  CAngleVector *animAnglesP, CFixVector *pvOffset, fix xModelLight, fix *xGlowValues, CFloatVector *pObjColor)
{
ENTER (0, 0);
if (pObj && (pObj->info.nType == OBJ_PLAYER) && (nModel > 0) && (nModel != COCKPIT_MODEL))
	nModel += gameData.multiplayer.weaponStates [pObj->info.nId].nShip;

	int32_t	i = 0, 
			bHires = (nModel > 0), 
			bUseVBO = ogl.m_features.bVertexBufferObjects && ((gameStates.render.bPerPixelLighting == 2) || gameOpts->ogl.bObjLighting),
			bEmissive = (pObj != NULL) && pObj->IsProjectile (),
			bBlur = 0,
			nGunId, nBombId, nMissileId, nMissiles;

nModel = abs (nModel);
RenderModel::CModel*	pModel = gameData.modelData.renderModels [bHires] + nModel;

if (!pObj)
	RETVAL (0)
#if DBG
if (nModel == nDbgModel)
	nDbgModel = nModel;
if (pObj) {
	if (ObjIdx (pObj) == nDbgObj)
		BRP;
	if (pObj->info.nSegment == nDbgSeg)
		BRP;
	if (pObj->info.nType == OBJ_DEBRIS)
		BRP;
	}
#endif

if (bHires) {
	if (pModel->m_bValid < 1) {
		if (pModel->m_bValid)
			bHires = 0;
		else {
			if (IsDefaultModel (nModel)) {
				if (bUseVBO && pModel->m_bValid && !(pModel->m_vboDataHandle && pModel->m_vboIndexHandle))
					pModel->m_bValid = 0;
				i = G3BuildModel (pObj, nModel, pp, modelBitmaps, pObjColor, 1);
				if (i < 0)	//successfully built new model
					RETVAL (gameStates.render.bBuildModels)
				}
			else
				i = 0;
			pModel->m_bValid = -1;
			bHires = 0;
			}
		}
	}

if (!bHires) {
	pModel = gameData.modelData.renderModels [0] + nModel;
	if (pModel->m_bValid < 0)
		RETVAL (0)
	if (bUseVBO && pModel->m_bValid && !(pModel->m_vboDataHandle && pModel->m_vboIndexHandle))
		pModel->m_bValid = 0;
	if (!pModel->m_bValid) {
		i = G3BuildModel (pObj, nModel, pp, modelBitmaps, pObjColor, 0);
		if (i <= 0) {
			if (!i)
				pModel->m_bValid = -1;
			RETVAL (gameStates.render.bBuildModels)
			}
		}
	}

pModel->m_bHasTransparency |= (bEmissive << 2);

int32_t bRenderTransparency = bHires && !gameStates.render.bCloaked && (gameStates.render.nType == RENDER_TYPE_TRANSPARENCY) && pModel->m_bHasTransparency;

if (!bRenderTransparency && gameStates.render.bCloaked) {
	shield [0].Animate (10);
	CFixVector vPos;
	PolyObjPos (pObj, &vPos);
#if 1
	glowRenderer.End ();
	ogl.ResetClientStates (0);
	bBlur = (gameOpts->render.nImageQuality >= 4) && glowRenderer.Begin (BLUR_OUTLINE, 5, false, 1.0f);
	if (bBlur && (glowRenderer.SetViewport (BLUR_OUTLINE, vPos, 2 * X2F (pObj->info.xSize) < 1))) {
		glowRenderer.Done (BLUR_OUTLINE);
		bBlur = false;
		}	
#endif
	}
else
	ogl.ResetClientStates (1);

//#pragma omp critical (fastModelRender)
{
PROF_START
if (gameOpts->render.debug.bWireFrame & 2)
	ogl.EnableClientStates (0, 0, 0, GL_TEXTURE0);
else if (gameStates.render.bCloaked)
	ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
else
	ogl.EnableClientStates (1, 1, gameOpts->ogl.bObjLighting, GL_TEXTURE0);
if (bUseVBO)
	glBindBufferARB (GL_ARRAY_BUFFER_ARB, pModel->m_vboDataHandle);
else {
	pModel->m_vbVerts.SetBuffer (reinterpret_cast<CFloatVector3*> (pModel->m_vertBuf [0].Buffer ()), 1, pModel->m_nFaceVerts);
	pModel->m_vbNormals.SetBuffer (pModel->m_vbVerts.Buffer () + pModel->m_nFaceVerts, 1, pModel->m_nFaceVerts);
	pModel->m_vbColor.SetBuffer (reinterpret_cast<CFloatVector*> (pModel->m_vbNormals.Buffer () + pModel->m_nFaceVerts), 1, pModel->m_nFaceVerts);
	pModel->m_vbTexCoord.SetBuffer (reinterpret_cast<tTexCoord2f*> (pModel->m_vbColor.Buffer () + pModel->m_nFaceVerts), 1, pModel->m_nFaceVerts);
	}
#if G3_SW_SCALING
G3ScaleModel (nModel);
#else
#	if 0
if (bHires)
	gameData.modelData.vScale.SetZero ();
#	endif
#endif
if (!(gameOpts->ogl.bObjLighting || gameStates.render.bCloaked))
	G3LightModel (pObj, nModel, xModelLight, xGlowValues, bHires);
if (bUseVBO) {
	if (!(gameOpts->render.debug.bWireFrame & 2)) {
		if (!gameStates.render.bCloaked) {
			OglNormalPointer (GL_FLOAT, 0, G3_BUFFER_OFFSET (pModel->m_nFaceVerts * sizeof (CFloatVector3)));
			OglColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (pModel->m_nFaceVerts * 2 * sizeof (CFloatVector3)));
			}
		OglTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (pModel->m_nFaceVerts * ((2 * sizeof (CFloatVector3) + sizeof (CFloatVector)))));
		}
	OglVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (0));
	if (pModel->m_vboIndexHandle)
		glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, pModel->m_vboIndexHandle);
	}
else {
	if (!gameStates.render.bCloaked) {
		OglTexCoordPointer (2, GL_FLOAT, 0, pModel->m_vbTexCoord.Buffer ());
		if (gameOpts->ogl.bObjLighting)
			OglNormalPointer (GL_FLOAT, 0, pModel->m_vbNormals.Buffer ());
		OglColorPointer (4, GL_FLOAT, 0, pModel->m_vbColor.Buffer ());
		}
	OglVertexPointer (3, GL_FLOAT, 0, pModel->m_vbVerts.Buffer ());
	}
nGunId = EquippedPlayerGun (pObj);
nBombId = EquippedPlayerBomb (pObj);
nMissileId = EquippedPlayerMissile (pObj, &nMissiles);
if (!bHires && (pObj->info.nType == OBJ_POWERUP)) {
	if ((pObj->info.nId == POW_SMARTMINE) || (pObj->info.nId == POW_PROXMINE))
		gameData.modelData.vScale.Set (I2X (2), I2X (2), I2X (2));
	else
		gameData.modelData.vScale.Set (I2X (3) / 2, I2X (3) / 2, I2X (3) / 2);
	}

#if DBG
if (gameOpts->render.debug.bWireFrame & 2) {
	glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth (2.0f);
	}
#endif

if (bRenderTransparency) {
	if (pObj->info.nType != OBJ_DEBRIS) {
		if (pModel->m_bHasTransparency & 5)
			G3DrawModel (pObj, nModel, nSubModel, modelBitmaps, animAnglesP, pvOffset, bHires, bUseVBO, 1, nGunId, nBombId, nMissileId, nMissiles, 0, 0);
		if (pModel->m_bHasTransparency & 10) {
			CFixVector vPos;
			PolyObjPos (pObj, &vPos);
			glowRenderer.Begin (GLOW_HEADLIGHT, 2, true, 0.666f);
			if (glowRenderer.SetViewport (GLOW_HEADLIGHT, vPos, 2 * X2F (pObj->info.xSize)) < 1) 
				glowRenderer.Done (GLOW_HEADLIGHT);
			else {
				ogl.SetFaceCulling (false);
				G3DrawModel (pObj, nModel, nSubModel, modelBitmaps, animAnglesP, pvOffset, bHires, bUseVBO, 2, nGunId, nBombId, nMissileId, nMissiles, 0, 0);
				ogl.SetFaceCulling (true);
				glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
				glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				glowRenderer.End ();
				}
			}
		}
	}
else {
	G3DrawModel (pObj, nModel, nSubModel, modelBitmaps, animAnglesP, pvOffset, bHires, bUseVBO, 0, nGunId, nBombId, nMissileId, nMissiles, 0, bBlur);
	pModel->m_bRendered = 1;
	if (gameData.modelData.thrusters [nModel].nCount < 0)
		gameData.modelData.thrusters [nModel].nCount = -gameData.modelData.thrusters [nModel].nCount;
	if ((pObj->info.nType != OBJ_DEBRIS) && bHires && pModel->m_bHasTransparency)
		transparencyRenderer.AddObject (pObj);
	}

glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
ogl.SetTexturing (false);
if (gameStates.render.bCloaked)
	ogl.DisableClientStates (1, 0, 0, -1);
else
	ogl.DisableClientStates (1, 1, gameOpts->ogl.bObjLighting, -1);

if (bBlur)
	glowRenderer.End (/*gameStates.render.grAlpha*/);

if (gameStates.render.CartoonStyle () && !gameStates.render.bCloaked && !bRenderTransparency && (!pObj->IsWeapon () || pObj->IsMissile ()))
	G3DrawModel (pObj, nModel, nSubModel, modelBitmaps, animAnglesP, pvOffset, bHires, bUseVBO, 0, nGunId, nBombId, nMissileId, nMissiles, 1, 0);

#if 1 //!DBG
if (gameStates.render.nType != RENDER_TYPE_TRANSPARENCY) {
	if (pObj && ((pObj->info.nType == OBJ_PLAYER) || (pObj->info.nType == OBJ_ROBOT) || (pObj->info.nType == OBJ_REACTOR))) {
		RenderModel::CModel*	pModel = gameData.modelData.renderModels [bHires] + nModel;

		if (pModel->m_bValid < 1) {
			if (!bHires)
				pModel = NULL;
			else {
				pModel = gameData.modelData.renderModels [0] + nModel;
				if (pModel->m_bValid < 1)
					pModel = NULL;
				}
			}
		if (pModel) {
			transformation.Begin (pObj->info.position.vPos, pObj->info.position.mOrient, __FILE__, __LINE__);
			RenderModel::CSubModel*	pSubModel = pModel->m_subModels.Buffer ();
			for (int32_t i = 0, j = pModel->m_nSubModels; i < j; i++, pSubModel++)
				if ((pSubModel->m_nParent == -1) && !G3FilterSubModel (pObj, pSubModel, nGunId, nBombId, nMissileId, nMissiles))
					G3RenderDamageLightning (pObj, nModel, i, animAnglesP, NULL, bHires);
			//	G3RenderDamageLightning (pObj, nModel, 0, animAnglesP, NULL, bHires);
			transformation.End (__FILE__, __LINE__);
#if 0 // lightning is just pushed to the transparency renderer here and will be actually rendered in a subsequent render pass
			glowRenderer.End ();
#endif
			}
		}
	}
#endif
#if DBG
if (gameOpts->render.debug.bWireFrame & 2) {
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glLineWidth (1.0f);
	}
#endif
ogl.ClearError (0);
PROF_END(ptRenderObjectMeshes)
}
RETVAL (1)
}

//------------------------------------------------------------------------------
//eof
