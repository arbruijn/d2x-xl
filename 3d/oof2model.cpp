#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <math.h>
#include <stdlib.h>
#include "error.h"

#include "descent.h"
#include "interp.h"
#include "hitbox.h"
#include "gr.h"
#include "hiresmodels.h"
#include "buildmodel.h"

using namespace RenderModel;

//------------------------------------------------------------------------------

void CModel::CountOOFModelItems (OOF::CModel *po)
{
	OOF::CSubModel*	pso;
	OOF::CFace*			pf;
	int32_t					i, j;

i = po->m_nSubModels;
m_nSubModels = i;
m_nFaces = 0;
m_nVerts = 0;
m_nFaceVerts = 0;
for (pso = po->m_subModels.Buffer (); i; i--, pso++) {
	j = pso->m_faces.m_nFaces;
	m_nFaces += j;
	m_nVerts += pso->m_nVerts;
	for (pf = pso->m_faces.m_list.Buffer (); j; j--, pf++)
		m_nFaceVerts += pf->m_nVerts;
	}
}

//------------------------------------------------------------------------------

void CModel::GetOOFModelItems (int32_t nModel, OOF::CModel *po, float fScale)
{
	OOF::CSubModel*	pso;
	OOF::CFace*			pof;
	OOF::CFaceVert*	pfv;
	CSubModel*			psm;
	CFloatVector3*		pvn = m_vertNorms.Buffer (), vNormal;
	CVertex*				pmv = m_faceVerts.Buffer ();
	CFace*				pmf = m_faces.Buffer ();
	int32_t				h, i, j, n, nIndex = 0;

for (i = po->m_nSubModels, pso = po->m_subModels.Buffer (), psm = m_subModels.Buffer (); i; i--, pso++, psm++) {
	psm->m_nParent = pso->m_nParent;
	if (psm->m_nParent < 0)
		m_iSubModel = (int16_t) (psm - m_subModels);
	psm->m_vOffset.v.coord.x = F2X (pso->m_vOffset.v.coord.x * fScale);
	psm->m_vOffset.v.coord.y = F2X (pso->m_vOffset.v.coord.y * fScale);
	psm->m_vOffset.v.coord.z = F2X (pso->m_vOffset.v.coord.z * fScale);
	psm->m_nAngles = 0;
	psm->m_nBomb = -1;
	psm->m_nMissile = -1;
	psm->m_nGun = -1;
	psm->m_nGunPoint = -1;
	psm->m_bBullets = 0;
	psm->m_bThruster = 0;
	psm->m_bGlow = 0;
	psm->m_bRender = 1;
	j = pso->m_faces.m_nFaces;
	psm->m_nVertexIndex [0] = nIndex;
	psm->m_nVertices = pso->m_nVerts;
	psm->m_nFaces = j;
	psm->m_faces = pmf;
	psm->InitMinMax ();
	for (pof = pso->m_faces.m_list.Buffer (); j; j--, pof++, pmf++) {
		pmf->m_nIndex = nIndex;
		pmf->m_bThruster = 0;
		pmf->m_bGlow = 0;
		n = pof->m_nVerts;
		pmf->m_nVerts = n;
		if (pof->m_bTextured)
			pmf->m_nBitmap = pof->m_texProps.nTexId;
		else
			pmf->m_nBitmap = -1;
		pfv = pof->m_vertices;
		h = pfv->m_nIndex;
		if (nModel > 200) {
			vNormal = CFloatVector3::Normal (*pso->m_vertices [pfv [0].m_nIndex].XYZ (), 
														*pso->m_vertices [pfv [1].m_nIndex].XYZ (), 
														*pso->m_vertices [pfv [2].m_nIndex].XYZ ());
			}
		else
			vNormal.Assign (pof->m_vNormal);
		for (; n; n--, pfv++, pmv++, pvn++) {
			h = pfv->m_nIndex;
			pmv->m_nIndex = h;
#if DBG
			if (h >= int32_t (m_vertices.Length ()))
				continue;
#endif
			pmv->m_texCoord.v.u = pfv->m_fu;
			pmv->m_texCoord.v.v = pfv->m_fv;
			pmv->m_normal = vNormal;
			m_vertices [h].Assign (pso->m_vertices [h]);
			m_vertices [h] *= fScale;
			m_vertexOwner [h].m_nOwner = (uint16_t) m_iSubModel;
			m_vertexOwner [h].m_nVertex = (uint16_t) h;
			pmv->m_vertex = m_vertices [h];
			psm->SetMinMax (&pmv->m_vertex);
			*pvn = vNormal;
			if ((pmv->m_bTextured = pof->m_bTextured))
				pmv->m_baseColor.Red () =
				pmv->m_baseColor.Green () =
				pmv->m_baseColor.Blue () = 1.0f;
			else {
				pmv->m_baseColor.Red () = (float) pof->m_texProps.color.Red () / 255.0f;
				pmv->m_baseColor.Green () = (float) pof->m_texProps.color.Green () / 255.0f;
				pmv->m_baseColor.Blue () = (float) pof->m_texProps.color.Blue () / 255.0f;
				}
			pmv->m_baseColor.Alpha () = 1.0f;
			nIndex++;
			}
		}
	}
}

//------------------------------------------------------------------------------

int32_t CModel::BuildFromOOF (CObject *pObj, int32_t nModel)
{
	OOF::CModel*	po = gameData.models.modelToOOF [1][nModel];
	CBitmap*			pBm;
	int32_t				i;

if (!po) {
	po = gameData.models.modelToOOF [0][nModel];
	if (!po)
		return 0;
	}
#if DBG
HUDMessage (0, "optimizing model");
#endif
if (gameStates.app.nLogLevel > 1)
	PrintLog (1, "optimizing OOF model %d\n", nModel);
CountOOFModelItems (po);
if (!Create ()) {
	if (gameStates.app.nLogLevel > 1)
		PrintLog (-1);
	return 0;
	}
GetOOFModelItems (nModel, po, /*((nModel == 108) || (nModel == 110)) ? 0.805f :*/ 1.0f);
m_nModel = nModel;
m_textures = po->m_textures.m_bitmaps;
m_nTextures = po->m_textures.m_nBitmaps;
for (i = 0, pBm = m_textures.Buffer (); i < m_nTextures; i++, pBm++) {
	pBm->Texture ()->SetBitmap (pBm);
	sprintf (m_textures [i].Name (), "OOF model %d texture %d", nModel, i);
	po->m_textures.m_bitmaps [i].ShareBuffer (*pBm);
	}
memset (m_teamTextures, 0xFF, sizeof (m_teamTextures));
m_nType = -1;
gameData.models.polyModels [0][nModel].SetRad (Size (pObj, 1), 1);
Setup (1, 1);
#if 1
SetGunPoints (pObj, 0);
#endif
if (gameStates.app.nLogLevel > 1)
	PrintLog (-1);
 return -1;
}

//------------------------------------------------------------------------------
//eof
