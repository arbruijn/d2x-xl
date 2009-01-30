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
#include "gameseg.h"
#include "light.h"
#include "dynlight.h"
#include "lightning.h"
#include "renderthreads.h"
#include "hiresmodels.h"
#include "buildmodel.h"
#include "cquicksort.h"

using namespace RenderModel;

//------------------------------------------------------------------------------

void CModel::Init (void)
{
memset (m_teamTextures, 0, sizeof (m_teamTextures));
memset (m_nGunSubModels, 0xFF, sizeof (m_nGunSubModels));
m_nModel = -1;
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
m_vboDataHandle = 0;
m_vboIndexHandle = 0;
}

//------------------------------------------------------------------------------

bool CModel::Create (void)
{
m_verts.Create (m_nVerts);
m_color.Create (m_nVerts);
m_vertBuf [0].Create (m_nFaceVerts);
m_faceVerts.Create (m_nFaceVerts);
m_vertNorms.Create (m_nFaceVerts);
m_subModels.Create (m_nSubModels);
m_faces.Create (m_nFaces);
m_index [0].Create (m_nFaceVerts);
m_sortedVerts.Create (m_nFaceVerts);

m_verts.Clear (0);
m_color.Clear (0xff);
m_vertBuf [0].Clear (0);
m_faceVerts.Clear (0);
m_vertNorms.Clear (0);
m_subModels.Clear (0);
m_faces.Clear (0);
m_index [0].Clear (0);
m_sortedVerts.Clear (0);

for (short i = 0; i < m_nSubModels; i++)
	m_subModels [i].m_nSubModel = i;

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
			Destroy ();
			return false;
			}
		}
	glBindBufferARB (GL_ARRAY_BUFFER_ARB, m_vboDataHandle);
	if ((i = glGetError ())) {
#	if DBG
		HUDMessage (0, "glBindBufferARB failed (%d)", i);
#	endif
		gameStates.ogl.bHaveVBOs = 0;
		Destroy ();
		return false;
		}
	glBufferDataARB (GL_ARRAY_BUFFER, m_nFaceVerts * sizeof (CRenderVertex), NULL, GL_STATIC_DRAW_ARB);
	m_vertBuf [1].SetBuffer (reinterpret_cast<CRenderVertex*> (glMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB)), 1, m_nFaceVerts);
	m_vboIndexHandle = 0;
	glGenBuffersARB (1, &m_vboIndexHandle);
	if (m_vboIndexHandle) {
		glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, m_vboIndexHandle);
		glBufferDataARB (GL_ELEMENT_ARRAY_BUFFER_ARB, m_nFaceVerts * sizeof (short), NULL, GL_STATIC_DRAW_ARB);
		m_index [1].SetBuffer (reinterpret_cast<short*> (glMapBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB)), 1, m_nFaceVerts);
		}
	}
return true;
}

//------------------------------------------------------------------------------

void CModel::Destroy (void)
{
#if DBG
if (m_nModel > -1)
	m_nModel = m_nModel;
if (m_nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
if (gameStates.ogl.bHaveVBOs && m_vboDataHandle)
	glDeleteBuffersARB (1, &m_vboDataHandle);
if (gameStates.ogl.bHaveVBOs && m_vboIndexHandle)
	glDeleteBuffersARB (1, &m_vboIndexHandle);

m_textures.Destroy ();
m_sortedVerts.Destroy ();
m_index [0].Destroy ();
m_faces.Destroy ();
m_subModels.Destroy ();
m_vertNorms.Destroy ();
m_faceVerts.Destroy ();
m_vertBuf [0].Destroy ();
m_vertBuf [1].SetBuffer (0);	//avoid trying to delete memory allocated by the graphics driver
m_color.Destroy ();
m_verts.Destroy ();

Init ();
}

//------------------------------------------------------------------------------

int CFace::Compare (const CFace* pf, const CFace* pm)
{
if (pf == pm)
	return 0;
if (pf->m_textureP && (pf->m_nBitmap >= 0) && (pm->m_nBitmap >= 0)) {
	if (pf->m_textureP->BPP () < pm->m_textureP->BPP ())
		return -1;
	if (pf->m_textureP->BPP () > pm->m_textureP->BPP ())
		return -1;
	}
if (pf->m_nBitmap < pm->m_nBitmap)
	return -1;
if (pf->m_nBitmap > pm->m_nBitmap)
	return 1;
if (pf->m_nVerts < pm->m_nVerts)
	return -1;
if (pf->m_nVerts > pm->m_nVerts)
	return 1;
return 0;
}

//------------------------------------------------------------------------------

inline const bool CFace::operator!= (CFace& other)
{
if (m_textureP && (m_nBitmap >= 0) && (other.m_nBitmap >= 0)) {
	if (m_textureP->BPP () < m_textureP->BPP ())
		return true;
	if (m_textureP->BPP () > m_textureP->BPP ())
		return true;
	}
if (m_nBitmap < other.m_nBitmap)
	return true;
if (m_nBitmap > other.m_nBitmap)
	return true;
if (m_nVerts < other.m_nVerts)
	return true;
if (m_nVerts > other.m_nVerts)
	return true;
return false;
}

//------------------------------------------------------------------------------

void CFace::SetTexture (CBitmap* textureP)
{
m_textureP = (textureP && (m_nBitmap >= 0)) ? textureP + m_nBitmap : NULL;
}

//------------------------------------------------------------------------------

int CFace::GatherVertices (CArray<CVertex>& source, CArray<CVertex>& dest, int nIndex)
{
#if DBG
if (uint (m_nIndex + m_nVerts) > source.Length ())
	return 0;
if (uint (nIndex + m_nVerts) > dest.Length ())
	return 0;
#endif
memcpy (dest + nIndex, source + m_nIndex, m_nVerts * sizeof (CVertex));
m_nIndex = nIndex;	//set this face's index of its first vertex in the model's vertex buffer
return m_nVerts;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CSubModel::InitMinMax (void)
{
m_vMin = CFloatVector3::Create ((float) 1e30, (float) 1e30, (float) 1e30);
m_vMax = CFloatVector3::Create ((float) -1e30, (float) -1e30, (float) -1e30);
}

//------------------------------------------------------------------------------

void CSubModel::SetMinMax (CFloatVector3 *vertexP)
{
	CFloatVector3	v = *vertexP;

#if 0
v [X] += X2F (psm->m_vOffset [X]);
v [Y] += X2F (psm->m_vOffset [Y]);
v [Z] += X2F (psm->m_vOffset [Z]);
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

void CSubModel::SortFaces (CBitmap* textureP)
{
	CQuickSort<CFace>	qs;

for (int i = 0; i < m_nFaces; i++)
	m_faces [i].SetTexture (textureP);
qs.SortAscending (m_faces, 0, static_cast<uint> (m_nFaces - 1), &RenderModel::CFace::Compare);
}

//------------------------------------------------------------------------------

void CSubModel::GatherVertices (CArray<CVertex>& source, CArray<CVertex>& dest)
{
	int nIndex = m_nIndex;	//this submodels vertices start at m_nIndex in the models vertex buffer

// copy vertices face by face
for (int i = 0; i < m_nFaces; i++)
	nIndex += m_faces [i].GatherVertices (source, dest, nIndex);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CModel::Setup (int bHires, int bSort)
{
	CSubModel*		psm;
	CFace*			pfi, * pfj;
	CVertex*			pmv;
	CFloatVector3*	pv, * pn;
	tTexCoord2f*	pt;
	tRgbaColorf*	pc;
	CBitmap*			textureP = bHires ? m_textures.Buffer () : NULL;
	int				i, j;
	short				nId;

m_fScale = 1;
for (i = 0, j = m_nFaceVerts; i < j; i++)
	m_index [0][i] = i;
//sort each submodel's faces
for (i = 0; i < m_nSubModels; i++) {
	if (bSort) {
		psm = &m_subModels [i];
		psm->SortFaces (textureP);
		psm->GatherVertices (m_faceVerts, m_sortedVerts);
		}
	pfi = psm->m_faces;
	pfi->SetTexture (textureP);
	for (nId = 0, j = psm->m_nFaces - 1; j; j--) {
		pfi->m_nId = nId;
		pfj = pfi++;
		pfi->SetTexture (textureP);
		if (pfi != pfj)
			nId++;
#if G3_ALLOW_TRANSPARENCY
		if (textureP && (textureP [pfi->nBitmap].props.flags & BM_FLAG_TRANSPARENT))
			m_bHasTransparency = 1;
#endif
		}
	pfi->m_nId = nId;
	}
m_vbVerts.SetBuffer (reinterpret_cast<CFloatVector3*> (m_vertBuf [0].Buffer ()), 1, m_vertBuf [0].Length ());
m_vbNormals.SetBuffer (m_vbVerts.Buffer () + m_nFaceVerts, true, m_vertBuf [0].Length ());
m_vbColor.SetBuffer (reinterpret_cast<tRgbaColorf*> (m_vbNormals.Buffer () + m_nFaceVerts), 1, m_vertBuf [0].Length ());
m_vbTexCoord.SetBuffer (reinterpret_cast<tTexCoord2f*> (m_vbColor.Buffer () + m_nFaceVerts), 1, m_vertBuf [0].Length ());
pv = m_vbVerts.Buffer ();
pn = m_vbNormals.Buffer ();
pt = m_vbTexCoord.Buffer ();
pc = m_vbColor.Buffer ();
pmv = bSort ? m_sortedVerts.Buffer () : m_faceVerts.Buffer ();
for (i = 0, j = m_nFaceVerts; i < j; i++, pmv++) {
	pv [i] = pmv->m_vertex;
	pn [i] = pmv->m_normal;
	pc [i] = pmv->m_baseColor;
	pt [i] = pmv->m_texCoord;
	}
if (m_vertBuf [1].Buffer ()) // points to graphics driver buffer for VBO based rendering
	memcpy (m_vertBuf [1].Buffer (), m_vertBuf [0].Buffer (), m_vertBuf [1].Size ());
if (m_index [1].Buffer ()) // points to graphics driver buffer for VBO based rendering
	memcpy (m_index [1].Buffer (), m_index [0].Buffer (), m_index [1].Size ());
if (bSort)
	memcpy (m_faceVerts.Buffer (), m_sortedVerts.Buffer (), m_faceVerts.Size ());
else
	memcpy (m_sortedVerts.Buffer (), m_faceVerts.Buffer (), m_sortedVerts.Size ());
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

int CModel::Shift (CObject *objP, int bHires, CFloatVector3 *vOffsetfP)
{
#if 1
return 0;
#else
	CRenderSubModel*	psm;
	int					i;
	CFloatVector3		vOffsetf;
	CVertex*				pmv;

if (objP->info.nType == OBJ_PLAYER) {
	if (IsMultiGame && !IsCoopGame)
		return 0;
	}
else if ((objP->info.nType != OBJ_ROBOT) && (objP->info.nType != OBJ_HOSTAGE) && (objP->info.nType != OBJ_POWERUP))
	return 0;
if (vOffsetfP)
	vOffsetf = *vOffsetfP;
else
	vOffsetf.Assign (gameData.models.offsets [m_nModel]);
if (!(vOffsetf [X] || vOffsetf [Y] || vOffsetf [Z]))
	return 0;
if (vOffsetfP) {
	for (i = m_nFaceVerts, pmv = m_faceVerts; i; i--, pmv++) {
		pmv->m_vertex [X] += vOffsetf [X];
		pmv->m_vertex [Y] += vOffsetf [Y];
		pmv->m_vertex [Z] += vOffsetf [Z];
		}
	}
else {
	for (i = m_nSubModels, psm = m_subModels; i; i--, psm++) {
		psm->m_vMin [X] += vOffsetf [X];
		psm->m_vMin [Y] += vOffsetf [Y];
		psm->m_vMin [Z] += vOffsetf [Z];
		psm->m_vMax [X] += vOffsetf [X];
		psm->m_vMax [Y] += vOffsetf [Y];
		psm->m_vMax [Z] += vOffsetf [Z];
		}
	}
return 1;
#endif
}

//------------------------------------------------------------------------------

void CSubModel::Size (CModel* pm, CObject* objP, CFixVector* vOffset)
{
	tHitbox		*phb = (m_nHitbox < 0) ? NULL : gameData.models.hitboxes [pm->m_nModel].hitboxes + m_nHitbox;
	CFixVector	vMin, vMax, vOffs;
	int			i, j;
	CSubModel*	psm;

if (vOffset)
	vOffs = *vOffset + m_vOffset;	//compute absolute offset (i.e. including offsets of all parent submodels)
else
	vOffs = m_vOffset;
if (phb)
	phb->vOffset = vOffs;
vMin [X] = F2X (m_vMin [X]);
vMin [Y] = F2X (m_vMin [Y]);
vMin [Z] = F2X (m_vMin [Z]);
vMax [X] = F2X (m_vMax [X]);
vMax [Y] = F2X (m_vMax [Y]);
vMax [Z] = F2X (m_vMax [Z]);
m_vCenter = CFixVector::Avg (vMin, vMax);
if (m_bBullets) {
	pm->m_bBullets = 1;
	pm->m_vBullets = m_vCenter;
	}
m_nRad = CFixVector::Dist (vMin, vMax) / 2;
for (i = 0, j = pm->m_nSubModels, psm = pm->m_subModels.Buffer (); i < j; i++, psm++)
	if (m_nParent == m_nSubModel)
		psm->Size (pm, objP, &vOffs);
}

//------------------------------------------------------------------------------

int CModel::CmpVerts (const CFloatVector3* pv, const CFloatVector3* pm)
{
	float h;

h = (*pv) [X] - (*pm) [X];
if (h < 0)
	return -1;
if (h > 0)
	return 1;
h = (*pv) [Y] - (*pm) [Y];
if (h < 0)
	return -1;
if (h > 0)
	return 1;
h = (*pv) [Z] - (*pm) [Z];
if (h < 0)
	return -1;
if (h > 0)
	return 1;
return 0;
}

//------------------------------------------------------------------------------
// remove duplicates

short CModel::FilterVertices (CArray<CFloatVector3>& vertices, short nVertices)
{
	CFloatVector3	*pi, *pj;

for (pi = vertices.Buffer (), pj = pi + 1, --nVertices; nVertices; nVertices--, pj++)
	if (CmpVerts (pi, pj))
		*++pi = *pj;
return (short) (pi - vertices) + 1;
}

//------------------------------------------------------------------------------

fix CModel::Radius (CObject *objP)
{
	CSubModel*					psm;
	CFace*						pmf;
	CVertex*						pmv;
	CArray<CFloatVector3>	vertices;
	CFloatVector3				vCenter, vOffset, v, vMin, vMax;
	float							fRad = 0, r;
	short							h, i, j, k;

tModelSphere *sP = gameData.models.spheres + m_nModel;
if (m_nType >= 0) {
	if ((m_nSubModels == sP->nSubModels) && (m_nFaces == sP->nFaces) && (m_nFaceVerts == sP->nFaceVerts)) {
		gameData.models.offsets [m_nModel] = sP->vOffsets [m_nType];
		return sP->xRads [m_nType];
		}
	}
//first get the biggest distance between any two model vertices
if (vertices.Create (m_nFaceVerts)) {
		CFloatVector3	*pv, *pvi, *pvj;

	for (i = 0, h = m_nSubModels, psm = m_subModels.Buffer (), pv = vertices.Buffer (); i < h; i++, psm++) {
		if (psm->m_nHitbox > 0) {
			vOffset.Assign (gameData.models.hitboxes [m_nModel].hitboxes [psm->m_nHitbox].vOffset);
			for (j = psm->m_nFaces, pmf = psm->m_faces; j; j--, pmf++) {
				for (k = pmf->m_nVerts, pmv = m_faceVerts + pmf->m_nIndex; k; k--, pmv++, pv++)
					*pv = pmv->m_vertex + vOffset;
				}
			}
		}
	h = (short) (pv - vertices.Buffer ()) - 1;
	
	CQuickSort<CFloatVector3>	qs;
	qs.SortAscending (vertices.Buffer (), 0, h, &RenderModel::CModel::CmpVerts);
		
	//G3SortModelVerts (vertices, 0, h);
	h = FilterVertices (vertices, h);
	for (i = 0, pvi = vertices.Buffer (); i < h - 1; i++, pvi++)
		for (j = i + 1, pvj = vertices + j; j < h; j++, pvj++)
			if (fRad < (r = CFloatVector3::Dist (*pvi, *pvj))) {
				fRad = r;
				vMin = *pvi;
				vMax = *pvj;
				}
	fRad /= 2;
	// then move the tentatively computed model center around so that all vertices are enclosed in the sphere
	// around the center with the radius computed above
	vCenter.Assign (gameData.models.offsets [m_nModel]);
	for (i = h, pv = vertices.Buffer (); i; i--, pv++) {
		v = *pv - vCenter;
		r = v.Mag();
		if (fRad < r)
			vCenter += v * ((r - fRad) / r);
		}

	for (i = h, pv = vertices.Buffer (); i; i--, pv++)
		if (fRad < (r = CFloatVector3::Dist (*pv, vCenter)))
			fRad = r;

	vertices.Destroy ();

	gameData.models.offsets [m_nModel].Assign (vCenter);
	if (m_nType >= 0) {
		sP->nSubModels = m_nSubModels;
		sP->nFaces = m_nFaces;
		sP->nFaceVerts = m_nFaceVerts;
		sP->vOffsets [m_nType] = gameData.models.offsets [m_nModel];
		sP->xRads [m_nType] = F2X (fRad);
		}
	}
else {
	// then move the tentatively computed model center around so that all vertices are enclosed in the sphere
	// around the center with the radius computed above
	vCenter.Assign (gameData.models.offsets [m_nModel]);
	for (i = 0, h = m_nSubModels, psm = m_subModels.Buffer (); i < h; i++, psm++) {
		if (psm->m_nHitbox > 0) {
			vOffset.Assign (gameData.models.hitboxes [m_nModel].hitboxes [psm->m_nHitbox].vOffset);
			for (j = psm->m_nFaces, pmf = psm->m_faces; j; j--, pmf++) {
				for (k = pmf->m_nVerts, pmv = m_faceVerts + pmf->m_nIndex; k; k--, pmv++) {
					v = pmv->m_vertex + vOffset;
					if (fRad < (r = CFloatVector3::Dist (v, vCenter)))
						fRad = r;
					}
				}
			}
		}
	gameData.models.offsets [m_nModel].Assign (vCenter);
	}
return F2X (fRad);
}

//------------------------------------------------------------------------------

fix CModel::Size (CObject *objP, int bHires)
{
	CSubModel*		psm;
	int				i, j;
	tHitbox*			phb = &gameData.models.hitboxes [m_nModel].hitboxes [0];
	CFixVector		hv;
	CFloatVector3	vOffset;
	double			dx, dy, dz, r;

#if DBG
if (m_nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
psm = m_subModels.Buffer ();
vOffset = psm->m_vMin;

j = 1;
if (m_nSubModels == 1) {
	psm->m_nHitbox = 1;
	gameData.models.hitboxes [m_nModel].nHitboxes = 1;
	}
else {
	for (i = m_nSubModels; i; i--, psm++)
		psm->m_nHitbox = (psm->m_bGlow || psm->m_bThruster || psm->m_bWeapon || (psm->m_nGunPoint >= 0)) ? -1 : j++;
	gameData.models.hitboxes [m_nModel].nHitboxes = j - 1;
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
		for (i = 0; i < m_nSubModels; i++)
			if (m_subModels [i].m_nParent == -1)
				m_subModels [i].Size (this, objP, NULL);
		}
	else
		m_subModels [0].Size (this, objP, NULL);
	// determine min and max size
	for (i = 0, psm = m_subModels.Buffer (); i < m_nSubModels; i++, psm++) {
		if (0 < (j = psm->m_nHitbox)) {
			phb [j].vMin [X] = F2X (psm->m_vMin [X]);
			phb [j].vMin [Y] = F2X (psm->m_vMin [Y]);
			phb [j].vMin [Z] = F2X (psm->m_vMin [Z]);
			phb [j].vMax [X] = F2X (psm->m_vMax [X]);
			phb [j].vMax [Y] = F2X (psm->m_vMax [Y]);
			phb [j].vMax [Z] = F2X (psm->m_vMax [Z]);
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
		gameData.models.offsets [m_nModel][X] =
		gameData.models.offsets [m_nModel][Y] =
		gameData.models.offsets [m_nModel][Z] = 0;
	else {
		gameData.models.offsets [m_nModel][X] = (phb [0].vMin [X] + phb [0].vMax [X]) / -2;
		gameData.models.offsets [m_nModel][Y] = (phb [0].vMin [Y] + phb [0].vMax [Y]) / -2;
		gameData.models.offsets [m_nModel][Z] = (phb [0].vMin [Z] + phb [0].vMax [Z]) / -2;
		}
	} while (Shift (objP, bHires, NULL));

psm = m_subModels.Buffer ();
vOffset = psm->m_vMin - vOffset;
gameData.models.offsets [m_nModel].Assign (vOffset);
#if DBG
if (m_nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
#if 1
//VmVecInc (&psm [0].vOffset, gameData.models.offsets + m_nModel);
#else
G3ShiftModel (objP, m_nModel, bHires, &vOffset);
#endif
dx = (phb [0].vMax [X] - phb [0].vMin [X]);
dy = (phb [0].vMax [Y] - phb [0].vMin [Y]);
dz = (phb [0].vMax [Z] - phb [0].vMin [Z]);
phb [0].vSize [X] = (fix) dx;
phb [0].vSize [Y] = (fix) dy;
phb [0].vSize [Z] = (fix) dz;
gameData.models.offsets [m_nModel][X] = (phb [0].vMax [X] + phb [0].vMin [X]) / 2;
gameData.models.offsets [m_nModel][Y] = (phb [0].vMax [Y] + phb [0].vMin [Y]) / 2;
gameData.models.offsets [m_nModel][Z] = (phb [0].vMax [Z] + phb [0].vMin [Z]) / 2;
//phb [0].vOffset = gameData.models.offsets [m_nModel];
for (i = 0; i <= j; i++)
	ComputeHitbox (m_nModel, i);
return Radius (objP);
}

//------------------------------------------------------------------------------

int CModel::MinMax (tHitbox *phb)
{
	CSubModel*	psm;
	int			i;

for (i = m_nSubModels, psm = m_subModels.Buffer (); i; i--, psm++) {
	if (!psm->m_bThruster && (psm->m_nGunPoint < 0)) {
		phb->vMin [X] = F2X (psm->m_vMin [X]);
		phb->vMin [Y] = F2X (psm->m_vMin [Y]);
		phb->vMin [Z] = F2X (psm->m_vMin [Z]);
		phb->vMax [X] = F2X (psm->m_vMax [X]);
		phb->vMax [Y] = F2X (psm->m_vMax [Y]);
		phb->vMax [Z] = F2X (psm->m_vMax [Z]);
		phb++;
		}
	}
return m_nSubModels;
}

//------------------------------------------------------------------------------

void CModel::SetShipGunPoints (OOF::CModel *po)
{
	static short nGunSubModels [] = {6, 7, 5, 4, 9, 10, 3, 3};

	CSubModel*		psm;
	OOF::CPoint*	pp = po->m_gunPoints.Buffer ();

po->m_gunPoints.Resize (N_PLAYER_GUNS);
for (uint i = 0; i < po->m_gunPoints.Length (); i++, pp++) {
	if (nGunSubModels [i] >= m_nSubModels)
		continue;
	m_nGunSubModels [i] = nGunSubModels [i];
	psm = m_subModels + nGunSubModels [i];
	pp->m_vPos [X] = (psm->m_vMax [X] + psm->m_vMin [X]) / 2;
	if (3 == (pp->m_nParent = nGunSubModels [i])) {
		pp->m_vPos [X] = (psm->m_vMax [Z] + 3 * psm->m_vMin [Y]) / 4;
		pp->m_vPos [X] = 7 * (psm->m_vMax [Z] + psm->m_vMin [Z]) / 8;
		}
	else {
		pp->m_vPos [Y] = (psm->m_vMax [Y] + psm->m_vMin [Y]) / 2;
		if (i < 4)
      	pp->m_vPos [Z] = psm->m_vMax [Z];
		else
			pp->m_vPos [Z] = (psm->m_vMax [Z] + psm->m_vMin [Z]) / 2;
		}
	}
}

//------------------------------------------------------------------------------

void CModel::SetRobotGunPoints (OOF::CModel *po)
{
	CSubModel*	psm;
	OOF::CPoint*	pp;
	int			i, j = po->m_gunPoints.Length ();

for (i = 0, pp = po->m_gunPoints.Buffer (); i < j; i++, pp++) {
	m_nGunSubModels [i] = pp->m_nParent;
	psm = m_subModels + pp->m_nParent;
	pp->m_vPos [X] = (psm->m_vMax [X] + psm->m_vMin [X]) / 2;
	pp->m_vPos [Y] = (psm->m_vMax [Y] + psm->m_vMin [Y]) / 2;
  	pp->m_vPos [Z] = psm->m_vMax [Z];
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

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

fix G3ModelRad (CObject *objP, int m_nModel, int bHires)
{
return gameData.models.renderModels [bHires][m_nModel].Radius (objP);
}

//------------------------------------------------------------------------------

fix G3ModelSize (CObject *objP, int m_nModel, int bHires)
{
return gameData.models.renderModels [bHires][m_nModel].Size (objP, bHires);
}

//------------------------------------------------------------------------------

void G3SetShipGunPoints (OOF::CModel *po, CModel* pm)
{
pm->SetShipGunPoints (po);
}

//------------------------------------------------------------------------------

void G3SetRobotGunPoints (OOF::CModel *po, CModel* pm)
{
pm->SetRobotGunPoints (po);
}

//------------------------------------------------------------------------------

void CModel::SetGunPoints (CObject *objP, int bASE)
{
	CFixVector		*vGunPoints;
	int				nParent, h, i, j;

if (bASE) {
	CSubModel	*psm = m_subModels.Buffer ();

	vGunPoints = gameData.models.gunInfo [m_nModel].vGunPoints;
	for (i = 0, j = 0; i < m_nSubModels; i++, psm++) {
		h = psm->m_nGunPoint;
		if ((h >= 0) && (h < MAX_GUNS)) {
			j++;
			m_nGunSubModels [h] = i;
			vGunPoints [h] = psm->m_vCenter;
			}
		}
	gameData.models.gunInfo [m_nModel].nGuns = j;
	}
else {
		OOF::CPoint		*pp;
		OOF::CSubModel	*pso;
		OOF::CModel		*po = gameData.models.modelToOOF [1][m_nModel];

	if (!po)
		po = gameData.models.modelToOOF [0][m_nModel];
	pp = po->m_gunPoints.Buffer ();
	if (objP->info.nType == OBJ_PLAYER)
		SetShipGunPoints (po);
	else if (objP->info.nType == OBJ_ROBOT)
		SetRobotGunPoints (po);
	else {
		gameData.models.gunInfo [m_nModel].nGuns = 0;
		return;
		}
	if ((j = po->m_gunPoints.Length ())) {
		if (j > MAX_GUNS)
			j = MAX_GUNS;
		gameData.models.gunInfo [m_nModel].nGuns = j;
		vGunPoints = gameData.models.gunInfo [m_nModel].vGunPoints;
		for (i = 0; i < j; i++, pp++, vGunPoints++) {
			(*vGunPoints) [X] = F2X (pp->m_vPos [X]);
			(*vGunPoints) [Y] = F2X (pp->m_vPos [Y]);
			(*vGunPoints) [Z] = F2X (pp->m_vPos [Z]);
			for (nParent = pp->m_nParent; nParent >= 0; nParent = pso->m_nParent) {
				pso = po->m_subModels + nParent;
				(*vGunPoints) += m_subModels [nParent].m_vOffset;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

int G3BuildModel (CObject* objP, int nModel, CPolyModel* pp, CArray<CBitmap*>& modelBitmaps, tRgbaColorf* pObjColor, int bHires)
{
	RenderModel::CModel*	pm = gameData.models.renderModels [bHires] + nModel;

if (pm->m_bValid > 0)
	return 1;
if (pm->m_bValid < 0)
	return 0;
pm->m_bRendered = 0;
#if DBG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
pm->Init ();
return bHires 
		 ? GetASEModel (nModel) 
			? pm->BuildFromASE (objP, nModel) 
			: pm->BuildFromOOF (objP, nModel) 
		 : pm->BuildFromPOF (objP, nModel, pp, modelBitmaps, pObjColor);
}

//------------------------------------------------------------------------------

int G3ModelMinMax (int m_nModel, tHitbox *phb)
{
	RenderModel::CModel*		pm;

if (!((pm = gameData.models.renderModels [1] + m_nModel) ||
	   (pm = gameData.models.renderModels [0] + m_nModel)))
	return 0;
return pm->MinMax (phb);
}

//------------------------------------------------------------------------------
//eof
