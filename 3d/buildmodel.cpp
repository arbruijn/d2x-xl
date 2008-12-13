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

using namespace RenderModel;

//------------------------------------------------------------------------------

void CModel::Init (void)
{
memset (m_teamTextures, 0, sizeof (m_teamTextures));
memset (m_nGunSubModels, 0, sizeof (m_nGunSubModels));
m_fScale = 0;
m_nType = 0; //-1: custom mode, 0: default model, 1: alternative model, 2: hires model
m_nFaces = 0;
m_iFace = 0;
m_nVerts = 0;
m_nFaceVerts = 0;
m_iFaceVert = 0;
m_nSubModels = 0;
m_nTextures = 0;
m_iSubModel = 0;
m_bHasTransparency = 0;
m_bValid = 0;
m_bRendered = 0;
m_bBullets = 0;
m_vBullets.SetZero ();
m_vboDataHandle;
m_vboIndexHandle;
}

//------------------------------------------------------------------------------

bool CModel::Create (void)
{
m_verts.Create (m_nVerts);
m_color.Clear (0);
m_color.Create m_nVerts);
m_color.Clear (0xff);
if (gameStates.ogl.bHaveVBOs) {
	int i;
	glGenBuffersARB (1, &m_vboDataHandle);
	if ((i = glGetError ())) {
		glGenBuffersARB (1, &m_vboDataHandle);
		if ((i = glGetError ())) {
#	if DBG
			HUDMessage (0, "glGenBuffersARB failed (%d)", i);
#	endif
			gameStates.ogl.bHaveVBOs = 0;
			return G3FreeModelItems (pm);
			}
		}
	glBindBufferARB (GL_ARRAY_BUFFER_ARB, m_vboDataHandle);
	if ((i = glGetError ())) {
#	if DBG
		HUDMessage (0, "glBindBufferARB failed (%d)", i);
#	endif
		gameStates.ogl.bHaveVBOs = 0;
		return G3FreeModelItems (pm);
		}
	glBufferDataARB (GL_ARRAY_BUFFER, m_nFaceVerts * sizeof (CRenderRenderVertex), NULL, GL_STATIC_DRAW_ARB);
	m_vertBuf [1].SetBuffer (reinterpret_cast<CRenderRenderVertex*> (glMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB)));
	m_vboIndexHandle = 0;
	glGenBuffersARB (1, &m_vboIndexHandle);
	if (m_vboIndexHandle) {
		glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, m_vboIndexHandle);
		glBufferDataARB (GL_ELEMENT_ARRAY_BUFFER_ARB, m_nFaceVerts * sizeof (short), NULL, GL_STATIC_DRAW_ARB);
		m_index [1].SetBuffer (reinterpret_cast<short*> (glMapBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB)));
		}
	}
m_vertBuf [0].Create (m_nFaceVerts);
m_vertBuf [0].Clear (0);
m_faceVerts.Create (m_nFaceVerts);
m_faceVerts.Clear (0);
m_vertNorms.Create (m_nFaceVerts);
m_vertNorms.Clear (0);
m_subModels.Create (m_nSubModels);
m_subModels.Clear (0);
m_faces.Create (m_nFaces);
m_faces.Clear (0);
m_index [0].Create (m_nFaceVerts);
m_index [0].Clear (0);
m_sortedVerts.Create (m_nFaceVerts);
m_sortedVerts [0].Clear (0);
return true;
}

//------------------------------------------------------------------------------

void CModel::Destroy (void)
{
pm->faces.Destroy ();
pm->subModels.Destroy ();
if (gameStates.ogl.bHaveVBOs && pm->vboDataHandle)
	glDeleteBuffersARB (1, &pm->vboDataHandle);
pm->vertBuf [0].Destroy ();
pm->vertBuf [1].SetBuffer (0);	//avoid trying to delete memory allocated by the graphics driver
pm->faceVerts.Destroy ();
pm->color.Destroy ();
pm->vertNorms.Destroy ();
pm->verts.Destroy ();
pm->sortedVerts.Destroy ();
if (gameStates.ogl.bHaveVBOs && pm->vboIndexHandle)
	glDeleteBuffersARB (1, &pm->vboIndexHandle);
pm->index [0]);
Init ();
}

//------------------------------------------------------------------------------

int G3AllocModel (CRenderModel *pm)
{
return 1;
}

//------------------------------------------------------------------------------

int G3FreeModelItems (CRenderModel *pm)
{
memset (pm, 0, sizeof (*pm));
return 0;
}

//------------------------------------------------------------------------------

void G3FreeAllPolyModelItems (void)
{
	int	i, j;

PrintLog ("unloading polygon model data\n");
for (j = 0; j < 2; j++)
	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		G3FreeModelItems (gameData.models.g3Models [j] + i);
POFFreeAllPolyModelItems ();
}

//------------------------------------------------------------------------------

void CSubModel::G3InitMinMax (void)
{
m_vMin = CFixVector3::Create ((float) 1e30, (float) 1e30, (float) 1e30);
m_vMax = CFixVector3::Create ((float) -1e30, (float) -1e30, (float) -1e30);
}

//------------------------------------------------------------------------------

void CSubModel::SetMinMax (CFixVector3 *vertexP)
{
	CFixVector3	v = *vertexP;

#if 0
v [X] += X2F (psm->vOffset [X]);
v [Y] += X2F (psm->vOffset [Y]);
v [Z] += X2F (psm->vOffset [Z]);
#endif
if (m_vMin [X] > v [X])
	m_vMin [X] = v [X];
if (m_vMin [Y] > v [Y])
	m_vMin [Y] = v [Y];
if (m_vMin [Z] > v [Z])
	m_vMin [Z] = v [Z];
if (m_vMax [X] < v [X])
	m_vMax [X] = v [X];
if (m_vMax [Y] < v [Y])
	m_vMax [Y] = v [Y];
if (m_vMax [Z] < v [Z])
	m_vMax [Z] = v [Z];
}

//------------------------------------------------------------------------------

inline const bool CFace::operator< (CFace& other)
{
if (m_textureP && (m_nBitmap >= 0) && (other.m_nBitmap >= 0)) {
	if (m_textureP->BPP () < textureP->BPP ())
		return true;
	if (m_textureP->BPP () > m_textureP->BPP ())
		return false;
	}
if (m_nBitmap < other.m_nBitmap)
	return true;
if (m_nBitmap > other.m_nBitmap)
	return false;
if (m_nVerts < other.m_nVerts)
	return true;
if (m_nVerts > other.m_nVerts)
	return false;
return false;
}

//------------------------------------------------------------------------------

inline const bool CFace::operator> (CFace& other)
{
if (m_textureP && (m_nBitmap >= 0) && (other.m_nBitmap >= 0)) {
	if (m_textureP->BPP () > textureP->BPP ())
		return true;
	if (m_textureP->BPP () < m_textureP->BPP ())
		return false;
	}
if (m_nBitmap > other.m_nBitmap)
	return true;
if (m_nBitmap < other.m_nBitmap)
	return false;
if (m_nVerts > other.m_nVerts)
	return true;
if (m_nVerts < other.m_nVerts)
	return false;
return false;
}

//------------------------------------------------------------------------------

void CSubModel::SortFaces (int left, int right)
{
	int					l = left,
							r = right;
	CRenderModelFace	m = m_faces [(l + r) / 2];

do {
	while (m_faces [l] < m)
		l++;
	while (m_faces [r] > m)
		r--;
	if (l <= r) {
		if (l < r) {
			CFace h = m_faces [l];
			m_faces [l] = m_faces [r];
			m_faces [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	SortFaces (l, right);
if (left < r)
	SortFaces (left, r);
}

//------------------------------------------------------------------------------

void G3SortFaceVerts (CRenderModel *pm, CRenderSubModel *psm, RenderModel::CVertex *psv)
{
	CFace		*pmf;
	CVertex	*pmv = pm->faceVerts.Buffer ();
	int		i, j, nIndex = psm->nIndex;

psv += nIndex;
for (i = psm->nFaces, pmf = psm->faces; i; i--, pmf++, psv += j) {
	j = pmf->nVerts;
	if (nIndex + j > pm->nFaceVerts)
		break;
	memcpy (psv, pmv + pmf->nIndex, j * sizeof (RenderModel::CVertex));
	pmf->nIndex = nIndex;
	nIndex += j;
	}
}

//------------------------------------------------------------------------------

void CModel::Setup (int bHires, int bSort)
{
	CSubModel	*psm;
	CFace	*pfi, *pfj;
	CVertex		*pmv, *sortedVerts;
	CFixVector3	*pv, *pn;
	tTexCoord2f	*pt;
	tRgbaColorf	*pc;
	CBitmap		*textureP = bHires ? m_textures.Buffer () : NULL;
	int			i, j;
	short			nId;

m_fScale = 1;
sortedVerts = m_sortedVerts.Buffer ();
for (i = 0, j = m_nFaceVerts; i < j; i++)
	m_index [0][i] = i;
//sort each submodel's faces
for (i = m_nSubModels, psm = m_subModels.Buffer (); i; i--, psm++) {
	if (bSort) {
		G3SortFaces (psm, 0, psm->nFaces - 1, textureP);
		G3SortFaceVerts (pm, psm, sortedVerts);
		}
	for (nId = 0, j = psm->nFaces - 1, pfi = psm->faces; j; j--) {
		pfi->nId = nId;
		pfj = pfi++;
		if (G3CmpFaces (pfi, pfj, textureP))
			nId++;
#if G3_ALLOW_TRANSPARENCY
		if (textureP && (textureP [pfi->nBitmap].props.flags & BM_FLAG_TRANSPARENT))
			m_bHasTransparency = 1;
#endif
		}
	pfi->nId = nId;
	}
m_vbVerts = reinterpret_cast<CFixVector3*> (m_vertBuf [0].Buffer ());
m_vbNormals = m_vbVerts + m_nFaceVerts;
m_vbColor = reinterpret_cast<tRgbaColorf*> (m_vbNormals + m_nFaceVerts);
m_vbTexCoord = reinterpret_cast<tTexCoord2f*> (m_vbColor + m_nFaceVerts);
pv = m_vbVerts.Buffer ();
pn = m_vbNormals.Buffer ();
pt = m_vbTexCoord.Buffer ();
pc = m_vbColor.Buffer ();
pmv = bSort ? sortedVerts : m_faceVerts.Buffer ();
for (i = 0, j = m_nFaceVerts; i < j; i++, pmv++) {
	pv [i] = pmv->vertex;
	pn [i] = pmv->normal;
	pc [i] = pmv->baseColor;
	pt [i] = pmv->texCoord;
	}
if (m_vertBuf [1].Buffer ())
	m_vertBuf [1] = m_vertBuf [0];
if (m_index [1].Buffer ())
	m_index [1] = m_index [0];
if (bSort)
	m_faceVerts = sortedVerts;
else
	memcpy (sortedVerts, m_faceVerts.Buffer (), m_nFaceVerts * sizeof (RenderModel::CVertex));
m_bValid = 1;
if (gameStates.ogl.bHaveVBOs) {
	glUnmapBufferARB (GL_ARRAY_BUFFER_ARB);
	glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
	glUnmapBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB);
	glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}
m_sortedVerts.Destroy ();
}

//------------------------------------------------------------------------------

int G3ShiftModel (CObject *objP, int nModel, int bHires, CFixVector3 *vOffsetfP)
{
#if 1
return 0;
#else
	CRenderModel			*pm = gameData.models.g3Models [bHires] + nModel;
	CRenderSubModel		*psm;
	int				i;
	CFixVector3			vOffsetf;
	RenderModel::CVertex	*pmv;

if (objP->info.nType == OBJ_PLAYER) {
	if (IsMultiGame && !IsCoopGame)
		return 0;
	}
else if ((objP->info.nType != OBJ_ROBOT) && (objP->info.nType != OBJ_HOSTAGE) && (objP->info.nType != OBJ_POWERUP))
	return 0;
if (vOffsetfP)
	vOffsetf = *vOffsetfP;
else
	VmVecFixToFloat (&vOffsetf, gameData.models.offsets + nModel);
if (!(vOffsetf [X] || vOffsetf [Y] || vOffsetf [Z]))
	return 0;
if (vOffsetfP) {
	for (i = pm->nFaceVerts, pmv = pm->faceVerts; i; i--, pmv++) {
		pmv->vertex [X] += vOffsetf [X];
		pmv->vertex [Y] += vOffsetf [Y];
		pmv->vertex [Z] += vOffsetf [Z];
		}
	}
else {
	for (i = pm->nSubModels, psm = pm->subModels; i; i--, psm++) {
		psm->vMin [X] += vOffsetf [X];
		psm->vMin [Y] += vOffsetf [Y];
		psm->vMin [Z] += vOffsetf [Z];
		psm->vMax [X] += vOffsetf [X];
		psm->vMax [Y] += vOffsetf [Y];
		psm->vMax [Z] += vOffsetf [Z];
		}
	}
return 1;
#endif
}

//------------------------------------------------------------------------------

void G3SubModelSize (CObject* objP, int nModel, int nSubModel, CFixVector* vOffset, int bHires)
{
	CRenderModel		*pm = gameData.models.g3Models [bHires] + nModel;
	CRenderSubModel	*psm = pm->subModels + nSubModel;
	tHitbox				*phb = (psm->nHitbox < 0) ? NULL : gameData.models.hitboxes [nModel].hitboxes + psm->nHitbox;
	CFixVector			vMin, vMax, vOffs;
	int					i, j;

if (vOffset)
	vOffs = *vOffset + psm->vOffset;	//compute absolute offset (i.e. including offsets of all parent submodels)
else
	vOffs = psm->vOffset;
if (phb)
	phb->vOffset = vOffs;
vMin [X] = F2X (psm->vMin [X]);
vMin [Y] = F2X (psm->vMin [Y]);
vMin [Z] = F2X (psm->vMin [Z]);
vMax [X] = F2X (psm->vMax [X]);
vMax [Y] = F2X (psm->vMax [Y]);
vMax [Z] = F2X (psm->vMax [Z]);
psm->vCenter = CFixVector::Avg (vMin, vMax);
if (psm->bBullets) {
	pm->bBullets = 1;
	pm->vBullets = psm->vCenter;
	}
psm->nRad = CFixVector::Dist (vMin, vMax) / 2;
for (i = 0, j = pm->nSubModels, psm = pm->subModels.Buffer (); i < j; i++, psm++)
	if (psm->nParent == nSubModel)
		G3SubModelSize (objP, nModel, i, &vOffs, bHires);
}

//------------------------------------------------------------------------------

static inline float G3CmpVerts (CFixVector3 *pv, CFixVector3 *pm)
{
	float h;

if ((h = ((*pv) [X] - (*pm) [X])))
	return h;
if ((h = ((*pv) [Y] - (*pm) [Y])))
	return h;
return (*pv) [Z] - (*pm) [Z];
}

//------------------------------------------------------------------------------

static void G3SortModelVerts (CFixVector3* vertices, short left, short right)
{
	short			l = left,
					r = right;
	CFixVector3	m = vertices [(l + r) / 2];

do {
	while (G3CmpVerts (vertices + l, &m) < 0)
		l++;
	while (G3CmpVerts (vertices + r, &m) > 0)
		r--;
	if (l <= r) {
		if (l < r) {
			CFixVector3 h = vertices [l];
			vertices [l] = vertices [r];
			vertices [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	G3SortModelVerts (vertices, l, right);
if (left < r)
	G3SortModelVerts (vertices, left, r);
}

//------------------------------------------------------------------------------

short G3FilterModelVerts (CFixVector3 *vertices, short nVertices)
{
	CFixVector3	*pi, *pj;

for (pi = vertices, pj = vertices + 1, --nVertices; nVertices; nVertices--, pj++)
	if (G3CmpVerts (pi, pj))
		*++pi = *pj;
return (short) (pi - vertices) + 1;
}

//------------------------------------------------------------------------------

fix G3ModelRad (CObject *objP, int nModel, int bHires)
{
	CRenderModel			*pm = gameData.models.g3Models [bHires] + nModel;
	CRenderSubModel		*psm;
	CRenderModelFace	*pmf;
	RenderModel::CVertex	*pmv;
	CFixVector3			*vertices, vCenter, vOffset, v, vMin, vMax;
	float				fRad = 0, r;
	short				h, i, j, k;

#if DBG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
tModelSphere *sP = gameData.models.spheres + nModel;
if (pm->nType >= 0) {
	if ((pm->nSubModels == sP->nSubModels) && (pm->nFaces == sP->nFaces) && (pm->nFaceVerts == sP->nFaceVerts)) {
		gameData.models.offsets [nModel] = sP->vOffsets [pm->nType];
		return sP->xRads [pm->nType];
		}
	}
//first get the biggest distance between any two model vertices
if ((vertices = new CFixVector3 [pm->nFaceVerts])) {
		CFixVector3	*pv, *pvi, *pvj;

	for (i = 0, h = pm->nSubModels, psm = pm->subModels.Buffer (), pv = vertices; i < h; i++, psm++) {
		if (psm->nHitbox > 0) {
			vOffset = gameData.models.hitboxes [nModel].hitboxes [psm->nHitbox].vOffset.ToFloat3();
			for (j = psm->nFaces, pmf = psm->faces; j; j--, pmf++) {
				for (k = pmf->nVerts, pmv = pm->faceVerts + pmf->nIndex; k; k--, pmv++, pv++)
					*pv = pmv->vertex + vOffset;
				}
			}
		}
	h = (short) (pv - vertices) - 1;
	G3SortModelVerts (vertices, 0, h);
	h = G3FilterModelVerts (vertices, h);
	for (i = 0, pvi = vertices; i < h - 1; i++, pvi++)
		for (j = i + 1, pvj = vertices + j; j < h; j++, pvj++)
			if (fRad < (r = CFixVector3::Dist(*pvi, *pvj))) {
				fRad = r;
				vMin = *pvi;
				vMax = *pvj;
				}
	fRad /= 2;
	// then move the tentatively computed model center around so that all vertices are enclosed in the sphere
	// around the center with the radius computed above
	vCenter = gameData.models.offsets [nModel].ToFloat3();
	for (i = h, pv = vertices; i; i--, pv++) {
		v = *pv - vCenter;
		r = v.Mag();
		if (fRad < r)
			vCenter += v * ((r - fRad) / r);
		}

	for (i = h, pv = vertices; i; i--, pv++)
		if (fRad < (r = CFixVector3::Dist(*pv, vCenter)))
			fRad = r;

	delete [] vertices;

	gameData.models.offsets [nModel] = vCenter.ToFix();
	if (pm->nType >= 0) {
		sP->nSubModels = pm->nSubModels;
		sP->nFaces = pm->nFaces;
		sP->nFaceVerts = pm->nFaceVerts;
		sP->vOffsets [pm->nType] = gameData.models.offsets [nModel];
		sP->xRads [pm->nType] = F2X (fRad);
		}
	}
else {
	// then move the tentatively computed model center around so that all vertices are enclosed in the sphere
	// around the center with the radius computed above
	vCenter = gameData.models.offsets [nModel].ToFloat3();
	for (i = 0, h = pm->nSubModels, psm = pm->subModels.Buffer (); i < h; i++, psm++) {
		if (psm->nHitbox > 0) {
			vOffset = gameData.models.hitboxes [nModel].hitboxes [psm->nHitbox].vOffset.ToFloat3();
			for (j = psm->nFaces, pmf = psm->faces; j; j--, pmf++) {
				for (k = pmf->nVerts, pmv = pm->faceVerts + pmf->nIndex; k; k--, pmv++) {
					v = pmv->vertex + vOffset;
					if (fRad < (r = CFixVector3::Dist(v, vCenter)))
						fRad = r;
					}
				}
			}
		}
	gameData.models.offsets [nModel] = vCenter.ToFix();
	}
return F2X (fRad);
}

//------------------------------------------------------------------------------

fix G3ModelSize (CObject *objP, CRenderModel *pm, int nModel, int bHires)
{
	int			nSubModels = pm->nSubModels;
	CRenderSubModel	*psm;
	int			i, j;
	tHitbox		*phb = gameData.models.hitboxes [nModel].hitboxes;
	CFixVector	hv;
	CFixVector3		vOffset;
	double		dx, dy, dz, r;

#if DBG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
psm = pm->subModels.Buffer ();
vOffset = psm->vMin;

j = 1;
if (nSubModels == 1) {
	psm->nHitbox = 1;
	gameData.models.hitboxes [nModel].nHitboxes = 1;
	}
else {
	for (i = nSubModels; i; i--, psm++)
		psm->nHitbox = (psm->bGlow || psm->bThruster || psm->bWeapon || (psm->nGunPoint >= 0)) ? -1 : j++;
	gameData.models.hitboxes [nModel].nHitboxes = j - 1;
	}
do {
	// initialize
	for (i = 0; i <= MAX_HITBOXES; i++) {
		phb [i].vMin [X] = phb [i].vMin [Y] = phb [i].vMin [Z] = 0x7fffffff;
		phb [i].vMax [X] = phb [i].vMax [Y] = phb [i].vMax [Z] = -0x7fffffff;
		phb [i].vOffset [X] = phb [i].vOffset [Y] = phb [i].vOffset [Z] = 0;
		}
	// walk through all submodels, getting their sizes
	if (bHires) {
		for (i = 0; i < pm->nSubModels; i++)
			if (pm->subModels [i].nParent == -1)
				G3SubModelSize (objP, nModel, i, NULL, 1);
		}
	else
		G3SubModelSize (objP, nModel, 0, NULL, 0);
	// determine min and max size
	for (i = 0, psm = pm->subModels.Buffer (); i < nSubModels; i++, psm++) {
		if (0 < (j = psm->nHitbox)) {
			phb [j].vMin [X] = F2X (psm->vMin [X]);
			phb [j].vMin [Y] = F2X (psm->vMin [Y]);
			phb [j].vMin [Z] = F2X (psm->vMin [Z]);
			phb [j].vMax [X] = F2X (psm->vMax [X]);
			phb [j].vMax [Y] = F2X (psm->vMax [Y]);
			phb [j].vMax [Z] = F2X (psm->vMax [Z]);
			dx = (phb [j].vMax [X] - phb [j].vMin [X]) / 2;
			dy = (phb [j].vMax [Y] - phb [j].vMin [Y]) / 2;
			dz = (phb [j].vMax [Z] - phb [j].vMin [Z]) / 2;
			r = sqrt (dx * dx + dy * dy + dz * dz) / 2;
			phb [j].vSize [X] = (fix) dx;
			phb [j].vSize [Y] = (fix) dy;
			phb [j].vSize [Z] = (fix) dz;
			hv = phb [j].vMin + phb [j].vOffset;
			if (phb [0].vMin [X] > hv [X])
				phb [0].vMin [X] = hv [X];
			if (phb [0].vMin [Y] > hv [Y])
				phb [0].vMin [Y] = hv [Y];
			if (phb [0].vMin [Z] > hv [Z])
				phb [0].vMin [Z] = hv [Z];
			hv = phb [j].vMax + phb [j].vOffset;
			if (phb [0].vMax [X] < hv [X])
				phb [0].vMax [X] = hv [X];
			if (phb [0].vMax [Y] < hv [Y])
				phb [0].vMax [Y] = hv [Y];
			if (phb [0].vMax [Z] < hv [Z])
				phb [0].vMax [Z] = hv [Z];
			}
		}
	if (IsMultiGame)
		gameData.models.offsets [nModel][X] =
		gameData.models.offsets [nModel][Y] =
		gameData.models.offsets [nModel][Z] = 0;
	else {
		gameData.models.offsets [nModel][X] = (phb [0].vMin [X] + phb [0].vMax [X]) / -2;
		gameData.models.offsets [nModel][Y] = (phb [0].vMin [Y] + phb [0].vMax [Y]) / -2;
		gameData.models.offsets [nModel][Z] = (phb [0].vMin [Z] + phb [0].vMax [Z]) / -2;
		}
	} while (G3ShiftModel (objP, nModel, bHires, NULL));

psm = pm->subModels.Buffer ();
vOffset = psm->vMin - vOffset;
gameData.models.offsets [nModel] = vOffset.ToFix();
#if DBG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
#if 1
//VmVecInc (&psm [0].vOffset, gameData.models.offsets + nModel);
#else
G3ShiftModel (objP, nModel, bHires, &vOffset);
#endif
dx = (phb [0].vMax [X] - phb [0].vMin [X]);
dy = (phb [0].vMax [Y] - phb [0].vMin [Y]);
dz = (phb [0].vMax [Z] - phb [0].vMin [Z]);
phb [0].vSize [X] = (fix) dx;
phb [0].vSize [Y] = (fix) dy;
phb [0].vSize [Z] = (fix) dz;
gameData.models.offsets [nModel][X] = (phb [0].vMax [X] + phb [0].vMin [X]) / 2;
gameData.models.offsets [nModel][Y] = (phb [0].vMax [Y] + phb [0].vMin [Y]) / 2;
gameData.models.offsets [nModel][Z] = (phb [0].vMax [Z] + phb [0].vMin [Z]) / 2;
//phb [0].vOffset = gameData.models.offsets [nModel];
for (i = 0; i <= j; i++)
	ComputeHitbox (nModel, i);
return G3ModelRad (objP, nModel, bHires);
}

//------------------------------------------------------------------------------

int NearestGunPoint (CFixVector *vGunPoints, CFixVector *vGunPoint, int nGuns, int *nUsedGuns)
{
	fix			xDist, xMinDist = 0x7fffffff;
	int			h = 0, i;
	CFixVector	vi, v0 = *vGunPoint;

v0 [Z] = 0;
for (i = 0; i < nGuns; i++) {
	if (nUsedGuns [i])
		continue;
	vi = vGunPoints [i];
	vi [Z] = 0;
	xDist = CFixVector::Dist(vi, v0);
	if (xMinDist > xDist) {
		xMinDist = xDist;
		h = i;
		}
	}
nUsedGuns [h] = 1;
return h;
}

//------------------------------------------------------------------------------

void SetShipGunPoints (tOOFObject *po, CRenderModel *pm)
{
	static short nGunSubModels [] = {6, 7, 5, 4, 9, 10, 3, 3};

	CRenderSubModel		*psm;
	tOOF_point		*pp;
	int				i;

for (i = 0, pp = po->gunPoints.pPoints; i < (po->gunPoints.nPoints = N_PLAYER_GUNS); i++, pp++) {
	if (nGunSubModels [i] >= pm->nSubModels)
		continue;
	pm->nGunSubModels [i] = nGunSubModels [i];
	psm = pm->subModels + nGunSubModels [i];
	pp->vPos.x = (psm->vMax [X] + psm->vMin [X]) / 2;
	if (3 == (pp->nParent = nGunSubModels [i])) {
		pp->vPos.y = (psm->vMax [Z] + 3 * psm->vMin [Y]) / 4;
		pp->vPos.z = 7 * (psm->vMax [Z] + psm->vMin [Z]) / 8;
		}
	else {
		pp->vPos.y = (psm->vMax [Y] + psm->vMin [Y]) / 2;
		if (i < 4)
      	pp->vPos.z = psm->vMax [Z];
		else
			pp->vPos.z = (psm->vMax [Z] + psm->vMin [Z]) / 2;
		}
	}
}

//------------------------------------------------------------------------------

void SetRobotGunPoints (tOOFObject *po, CRenderModel *pm)
{
	CRenderSubModel		*psm;
	tOOF_point		*pp;
	int				i, j = po->gunPoints.nPoints;

for (i = 0, pp = po->gunPoints.pPoints; i < j; i++, pp++) {
	pm->nGunSubModels [i] = pp->nParent;
	psm = pm->subModels + pp->nParent;
	pp->vPos.x = (psm->vMax [X] + psm->vMin [X]) / 2;
	pp->vPos.y = (psm->vMax [Y] + psm->vMin [Y]) / 2;
  	pp->vPos.z = psm->vMax [Z];
	}
}

//------------------------------------------------------------------------------

void G3SetGunPoints (CObject *objP, CRenderModel *pm, int nModel, int bASE)
{
	CFixVector		*vGunPoints;
	int				nParent, h, i, j;

if (bASE) {
	CRenderSubModel	*psm = pm->subModels.Buffer ();

	vGunPoints = gameData.models.gunInfo [nModel].vGunPoints;
	for (i = 0, j = 0; i < pm->nSubModels; i++, psm++) {
		h = psm->nGunPoint;
		if ((h >= 0) && (h < MAX_GUNS)) {
			j++;
			pm->nGunSubModels [h] = i;
			vGunPoints [h] = psm->vCenter;
			}
		}
	gameData.models.gunInfo [nModel].nGuns = j;
	}
else {
		tOOF_point		*pp;
		tOOF_subObject	*pso;
		tOOFObject		*po = gameData.models.modelToOOF [1][nModel];

	if (!po)
		po = gameData.models.modelToOOF [0][nModel];
	pp = po->gunPoints.pPoints;
	if (objP->info.nType == OBJ_PLAYER)
		SetShipGunPoints (po, pm);
	else if (objP->info.nType == OBJ_ROBOT)
		SetRobotGunPoints (po, pm);
	else {
		gameData.models.gunInfo [nModel].nGuns = 0;
		return;
		}
	if ((j = po->gunPoints.nPoints)) {
		if (j > MAX_GUNS)
			j = MAX_GUNS;
		gameData.models.gunInfo [nModel].nGuns = j;
		vGunPoints = gameData.models.gunInfo [nModel].vGunPoints;
		for (i = 0; i < j; i++, pp++, vGunPoints++) {
			(*vGunPoints) [X] = F2X (pp->vPos.x);
			(*vGunPoints) [Y] = F2X (pp->vPos.y);
			(*vGunPoints) [Z] = F2X (pp->vPos.z);
			for (nParent = pp->nParent; nParent >= 0; nParent = pso->nParent) {
				pso = po->pSubObjects + nParent;
				(*vGunPoints) += pm->subModels [nParent].vOffset;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

int G3BuildModel (CObject *objP, int nModel, tPolyModel *pp, CBitmap **modelBitmaps, tRgbaColorf *pObjColor, int bHires)
{
	CRenderModel	*pm = gameData.models.g3Models [bHires] + nModel;

if (pm->bValid > 0)
	return 1;
if (pm->bValid < 0)
	return 0;
pm->bRendered = 0;
#if DBG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
memset (pm->nGunSubModels, 0xff, sizeof (pm->nGunSubModels));
return bHires ?
			ASEModel (nModel) ?
				G3BuildModelFromASE (objP, nModel) :
				G3BuildModelFromOOF (objP, nModel) :
			G3BuildModelFromPOF (objP, nModel, pp, modelBitmaps, pObjColor);
}

//------------------------------------------------------------------------------

int G3ModelMinMax (int nModel, tHitbox *phb)
{
	CRenderModel		*pm;
	CRenderSubModel	*psm;
	int			i;

if (!((pm = gameData.models.g3Models [1] + nModel) ||
	   (pm = gameData.models.g3Models [0] + nModel)))
	return 0;
for (i = pm->nSubModels, psm = pm->subModels.Buffer (); i; i--, psm++) {
	if (!psm->bThruster && (psm->nGunPoint < 0)) {
		phb->vMin [X] = F2X (psm->vMin [X]);
		phb->vMin [Y] = F2X (psm->vMin [Y]);
		phb->vMin [Z] = F2X (psm->vMin [Z]);
		phb->vMax [X] = F2X (psm->vMax [X]);
		phb->vMax [Y] = F2X (psm->vMax [Y]);
		phb->vMax [Z] = F2X (psm->vMax [Z]);
		phb++;
		}
	}
return pm->nSubModels;
}

//------------------------------------------------------------------------------
//eof
