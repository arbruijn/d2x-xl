#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif
//#include  "oof.h
#include "descent.h"
#include "args.h"
#include "u_mem.h"
#include "error.h"
#include "light.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "network.h"
#include "rendermine.h"
#include "strutil.h"

#define MAXGAP	0.01f

using namespace OOF;

//------------------------------------------------------------------------------

#define OOF_MEM_OPT	1

#ifdef __unix
#	ifndef stricmp
#		define	stricmp	strcasecmp
#	endif
#	ifndef strnicmp
#		define	strnicmp	strncasecmp
#	endif
#endif

//------------------------------------------------------------------------------

static int nIndent = 0;
static int bLogOOF = 0;
extern  FILE *fErr;

static void _CDECL_ OOF_PrintLog (const char *fmt, ...)
{
if (bLogOOF) {
	va_list arglist;
	static char szLog [1024];

	memset (szLog, ' ', nIndent);
	va_start (arglist, fmt);
	vsprintf (szLog + nIndent, fmt, arglist);
	va_end (arglist);
	fprintf (fErr, szLog);
	}
}

//------------------------------------------------------------------------------

static sbyte OOF_ReadByte (CFile& cf, const char *pszIdent)
{
sbyte b = cf.ReadByte ();
OOF_PrintLog ("      %s = %d\n", pszIdent, b);
return b;
}

//------------------------------------------------------------------------------

static int OOF_ReadInt (CFile& cf, const char *pszIdent)
{
int i = cf.ReadInt ();
OOF_PrintLog ("      %s = %d\n", pszIdent, i);
return i;
}

//------------------------------------------------------------------------------

static float OOF_ReadFloat (CFile& cf, const char *pszIdent)
{
float f = cf.ReadFloat ();
OOF_PrintLog ("      %s = %1.4f\n", pszIdent, f);
return f;
}

//------------------------------------------------------------------------------

static void OOF_ReadVector (CFile& cf, CFloatVector *pv, const char *pszIdent)
{
(*pv) [X] = cf.ReadFloat ();
(*pv) [Y] = cf.ReadFloat ();
(*pv) [Z] = cf.ReadFloat ();
OOF_PrintLog ("      %s = %1.4f,%1.4f,%1.4f\n", pszIdent, (*pv) [X], (*pv) [Y], (*pv) [Z]);
}

//------------------------------------------------------------------------------

static char *OOF_ReadString (CFile& cf, const char *pszIdent)
{
	char	*psz;
	int	l;

l = OOF_ReadInt (cf, "string length");
if (!(psz = new char [l + 1]))
	return NULL;
if (!l || cf.Read (psz, l, 1)) {
	psz [l] = '\0';
	OOF_PrintLog ("      %s = '%s'\n", pszIdent, psz);
	return psz;
	}
delete[] psz;
return NULL;
}

//------------------------------------------------------------------------------

static int OOF_ReadIntList (CFile& cf, CArray<int>& list)
{
	uint	i;
	char	szId [20] = "";

list.Destroy ();
if (!(i = OOF_ReadInt (cf, "nList"))) 
	return 0;
if (!list.Create (i))
	return -1;
for (i = 0; i < list.Length (); i++) {
	if (bLogOOF)
		sprintf (szId, "list [%d]", i);
	list [i] = OOF_ReadInt (cf, szId);
	}
return list.Length ();
}

//------------------------------------------------------------------------------

static int ListType (char *pListId)
{
if (!strncmp (pListId, "TXTR", 4))
	return 0;
if (!strncmp (pListId, "OHDR", 4))
	return 1;
if (!strncmp (pListId, "SOBJ", 4))
	return 2;
if (!strncmp (pListId, "GPNT", 4))
	return 3;
if (!strncmp (pListId, "SPCL", 4))
	return 4;
if (!strncmp (pListId, "ATCH", 4))
	return 5;
if (!strncmp (pListId, "PANI", 4))
	return 6;
if (!strncmp (pListId, "RANI", 4))
	return 7;
if (!strncmp (pListId, "ANIM", 4))
	return 8;
if (!strncmp (pListId, "WBAT", 4))
	return 9;
if (!strncmp (pListId, "NATH", 4))
	return 10;
return -1;
}

//------------------------------------------------------------------------------

static void OOF_InitMinMax (CFloatVector *pvMin, CFloatVector *pvMax)
{
if (pvMin && pvMax) {
	(*pvMin) [X] =
	(*pvMin) [Y] =
	(*pvMin) [Z] = 1000000;
	(*pvMax) [X] =
	(*pvMax) [Y] =
	(*pvMax) [Z] = -1000000;
	}
}

//------------------------------------------------------------------------------

static void OOF_GetMinMax (CFloatVector *pv, CFloatVector *pvMin, CFloatVector *pvMax)
{
if (pvMin && pvMax) {
	if ((*pvMin) [X] > (*pv) [X])
		(*pvMin) [X] = (*pv) [X];
	if ((*pvMax) [X] < (*pv) [X])
		(*pvMax) [X] = (*pv) [X];
	if ((*pvMin) [Y] > (*pv) [Y])
		(*pvMin) [Y] = (*pv) [Y];
	if ((*pvMax) [Y] < (*pv) [Y])
		(*pvMax) [Y] = (*pv) [Y];
	if ((*pvMin) [Z] > (*pv) [Z])
		(*pvMin) [Z] = (*pv) [Z];
	if ((*pvMax) [Z] < (*pv) [Z])
		(*pvMax) [Z] = (*pv) [Z];
	}
}

//------------------------------------------------------------------------------

static bool OOF_ReadVertList (CFile& cf, CArray<CFloatVector>& list, int nVerts, CFloatVector *pvMin, CFloatVector *pvMax)
{
	char	szId [20] = "";

OOF_InitMinMax (pvMin, pvMax);
if (!list.Create (nVerts))
	return false;

for (int i = 0; i < nVerts; i++) {
	if (bLogOOF)
		sprintf (szId, "vertList [%d]", i);
	OOF_ReadVector (cf, list + i, szId);
#if OOF_TEST_CUBE
	pv [i].x -= 10;
	pv [i].y += 15;
	//pv [i].z += 5;
	pv [i].x /= 2;
	pv [i].y /= 2;
	pv [i].z /= 2;
#endif
	OOF_GetMinMax (list + i, pvMin, pvMax);
	}
return true;
}

//------------------------------------------------------------------------------

void OOF::CFrameInfo::Init (void)
{
m_nFrames = 0;
m_nFirstFrame = 0;
m_nLastFrame = 0;
}

//------------------------------------------------------------------------------

int OOF::CFrameInfo::Read (CFile& cf, CModel* po, int bTimed)
{
nIndent += 2;
OOF_PrintLog ("reading frame info\n");
if (bTimed) {
	m_nFrames = OOF_ReadInt (cf, "nFrames");
	m_nFirstFrame = OOF_ReadInt (cf, "nFirstFrame");
	m_nLastFrame = OOF_ReadInt (cf, "nLastFrame");
	if (po->m_frameInfo.m_nFirstFrame > m_nFirstFrame)
		po->m_frameInfo.m_nFirstFrame = m_nFirstFrame;
	if (po->m_frameInfo.m_nLastFrame < m_nLastFrame)
		po->m_frameInfo.m_nLastFrame = m_nLastFrame;
	}
else
	m_nFrames = po->m_frameInfo.m_nFrames;
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int CRotFrame::Read (CFile& cf, int bTimed)
{
	float	fMag;

nIndent += 2;
OOF_PrintLog ("reading rot frame\n");
if (bTimed)
	m_nStartTime = OOF_ReadInt (cf, "nStartTime");
OOF_ReadVector (cf, &m_vAxis, "vAxis");
if (0 < (fMag = m_vAxis.Mag ()))
	m_vAxis /= fMag;
m_nAngle = OOF_ReadInt (cf, "nAngle");
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CAnim::Init (void)
{
m_nTicks = 0;
}

//------------------------------------------------------------------------------

void CAnim::Destroy (void)
{
m_remapTicks.Destroy ();
Init ();
}

//------------------------------------------------------------------------------

void CRotAnim::Init (void)
{
CAnim::Init ();
}

//------------------------------------------------------------------------------

void CRotAnim::Destroy (void)
{
m_frames.Destroy ();
CAnim::Destroy ();
}

//------------------------------------------------------------------------------

int CRotAnim::Read (CFile& cf, CModel* po, int bTimed)
{
if (!this->CFrameInfo::Read (cf, po, bTimed))
	return 0;
if (!m_frames.Create (m_nFrames))
	return 0;
m_frames.Clear (0);
if (bTimed &&
	 (m_nTicks = abs (m_nLastFrame - m_nFirstFrame) + 1) &&
	 !m_remapTicks.Create (m_nTicks)) {
	 Destroy ();
	return 0;
	}
if (m_nTicks)
	for (int i = 0; i < m_nFrames; i++)
		if (!m_frames [i].Read (cf, bTimed)) {
			Destroy ();
			return 0;
			}
return 1;
}

//------------------------------------------------------------------------------

void CRotAnim::BuildAngleMatrix (CFloatMatrix *pm, int a, CFloatVector *pAxis)
{
float x = (*pAxis) [X];
float y = (*pAxis) [Y];
float z = (*pAxis) [Z];
float s = (float) sin ((float) a);
float c = (float) cos ((float) a);
float t = 1.0f - c;
float i = t * x;
float j = s * z;
//pm->m_r.x = t * x * x + c;
(*pm).RVec()[X] = i * x + c;
i *= y;
//(*pm).RVec()[Y] = t * x * y + s * z;
//(*pm).UVec()[X] = t * x * y - s * z;
(*pm).RVec()[Y] = i + j;
(*pm).UVec()[X] = i - j;
i = t * z;
//(*pm).FVec()[Z] = t * z * z + c;
(*pm).FVec()[Z] = i * z + c;
i *= x;
j = s * y;
//(*pm).RVec()[Z] = t * x * z - s * y;
//(*pm).FVec()[X] = t * x * z + s * y;
(*pm).RVec()[Z] = i - j;
(*pm).FVec()[X] = i + j;
i = t * y;
//(*pm).UVec()[Y] = t * y * y + c;
(*pm).UVec()[Y] = i * y + c;
i *= z;
j = s * x;
//(*pm).UVec()[Z] = t * y * z + s * x;	
//(*pm).FVec()[Y] = t * y * z - s * x;
(*pm).UVec()[Z] = i + j;
(*pm).FVec()[Y] = i - j;
}

//------------------------------------------------------------------------------

void CRotAnim::BuildMatrices (void)
{
	CFloatMatrix	mBase, mTemp;
	CRotFrame*		pf;
	int				i;

mBase = CFloatMatrix::IDENTITY;
for (i = m_frames.Length (), pf = m_frames.Buffer (); i; i--, pf++) {
	BuildAngleMatrix (&mTemp, pf->m_nAngle, &pf->m_vAxis);
	pf->m_mMat = mTemp * mBase;
	mBase = pf->m_mMat;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int CPosFrame::Read (CFile& cf, int bTimed)
{
nIndent += 2;
OOF_PrintLog ("reading pos frame\n");
if (bTimed)
	m_nStartTime = OOF_ReadInt (cf, "nStartTime");
OOF_ReadVector (cf, &m_vPos, "vPos");
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

void CPosAnim::Destroy (void)
{
m_frames.Destroy ();
CAnim::Destroy ();
}

//------------------------------------------------------------------------------

int CPosAnim::Read (CFile& cf, CModel* po, int bTimed)
{
if (!this->CFrameInfo::Read (cf, po, bTimed))
	return 0;
if (bTimed &&
	 (m_nTicks = m_nLastFrame - m_nFirstFrame) &&
	 !m_remapTicks.Create (m_nTicks)) {
	 Destroy ();
	 return 0;
	}
if (!m_frames.Create (m_nFrames)) {
	Destroy ();
	return 0;
	}
m_frames.Clear (0);
for (int i = 0; i < m_nFrames; i++)
	if (!m_frames [i].Read (cf, bTimed)) {	
		Destroy ();
		return 0;
		}
return 1;
}

//------------------------------------------------------------------------------

void CSpecialPoint::Init (void)
{
memset (this, 0, sizeof (*this));
}

//------------------------------------------------------------------------------

void CSpecialPoint::Destroy (void)
{
delete[] m_pszName;
m_pszName = NULL;
delete[] m_pszProps;
m_pszProps = NULL;
}

//------------------------------------------------------------------------------

int CSpecialPoint::Read (CFile& cf)
{
Init ();
nIndent += 2;
OOF_PrintLog ("reading special point\n");
if (!(m_pszName = OOF_ReadString (cf, "pszName"))) {
	nIndent -= 2;
	return 0;
	}
if (!(m_pszProps = OOF_ReadString (cf, "pszProps"))) {
	nIndent -= 2;
	return 0;
	}
OOF_ReadVector (cf, &m_vPos, "vPos");
m_fRadius = OOF_ReadFloat (cf, "fRadius");
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int CSpecialList::Read (CFile& cf)
{
	uint	i;

i = OOF_ReadInt (cf, "nVerts");
if (!i)
	return 1;
if (!Create (i))
	return 0;
for (i = 0; i < Length (); i++)
	(*this) [i].Read (cf);
return 1;
}

//------------------------------------------------------------------------------

int CPoint::Read (CFile& cf, int bParent)
{
nIndent += 2;
OOF_PrintLog ("reading point\n");
m_nParent = bParent ? OOF_ReadInt (cf, "nParent") : 0;
OOF_ReadVector (cf, &m_vPos, "vPos");
OOF_ReadVector (cf, &m_vDir, "vDir");
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int CPointList::Read (CFile& cf, int bParent, int nSize)
{
	int	i;

nIndent += 2;
OOF_PrintLog ("reading point list\n");
i = OOF_ReadInt (cf, "nPoints");
if (nSize < i)
	nSize = i;
if (!Create (i)) {
	nIndent -= 2;
	return 0;
	}
for (i = 0; i < static_cast<int> (Length ()); i++)
	if (!(*this) [i].Read (cf, bParent)) {
		Destroy ();
		nIndent -= 2;
		return 0;
		}
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int CAttachList::Read (CFile& cf)
{
	int	i;

nIndent += 2;
OOF_PrintLog ("reading attach list\n");
i = OOF_ReadInt (cf, "nPoints");
if (!Create (i)) {
	nIndent -= 2;
	return 0;
	}
for (i = 0; i < static_cast<int> (Length ()); i++)
	if (!(*this) [i].CPoint::Read (cf, 1)) {
		Destroy ();
		nIndent -= 2;
		return 0;
		}
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------

int CAttachPoint::Read (CFile& cf)
{
OOF_ReadVector (cf, &m_vu, "vu");	//actually ignored
OOF_ReadVector (cf, &m_vu, "vu");
m_bu = 1;
return 1;
}

//------------------------------------------------------------------------------

int CAttachList::ReadNormals (CFile& cf)
{
	uint	i;

nIndent += 2;
OOF_PrintLog ("reading attach normals\n");
i = OOF_ReadInt (cf, "nPoints");
if (i != Length ()) {
	nIndent -= 2;
	return 0;
	}
for (i = 0; i < Length (); i++)
	(*this).Read (cf);
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CBattery::Init (void)
{
m_nVerts = 0;
m_nTurrets = 0;
}

//------------------------------------------------------------------------------

void CBattery::Destroy (void)
{
m_vertIndex.Destroy ();
m_turretIndex.Destroy ();
}

//------------------------------------------------------------------------------

int CBattery::Read (CFile& cf)
{
nIndent += 2;
OOF_PrintLog ("reading battery\n");
if (0 > (m_nVerts = OOF_ReadIntList (cf, m_vertIndex))) {
	nIndent -= 2;
	Destroy ();
	return 0;
	}
if (0 > (m_nTurrets = OOF_ReadIntList (cf, m_turretIndex))) {
	nIndent -= 2;
	Destroy ();
	return 0;
	}
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int CArmament::Read (CFile& cf)
{
	int	i;

nIndent += 2;
OOF_PrintLog ("reading armament\n");
if (!(i = OOF_ReadInt (cf, "nBatts"))) {
	nIndent -= 2;
	return 1;
	}
if (!Create (i)) {
	Destroy ();
	nIndent -= 2;
	return 0;
	}
for (i = 0; i < static_cast<int> (Length ()); i++)
	if (!(*this).Read (cf)) {
		Destroy ();
		nIndent -= 2;
		return 0;
		}
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int CFaceVert::Read (CFile& cf, int bFlipV)
{
nIndent += 2;
OOF_PrintLog ("reading face vertex\n");
m_nIndex = OOF_ReadInt (cf, "nIndex");
m_fu = OOF_ReadFloat (cf, "fu");
m_fv = OOF_ReadFloat (cf, "fv");
if (bFlipV)
	m_fv = -m_fv;
#if OOF_TEST_CUBE
/*!!!*/if (m_fu == 0.5) m_fu = 1.0;
/*!!!*/if (m_fv == 0.5) m_fv = 1.0;
/*!!!*/if (m_fu == -0.5) m_fu = 1.0;
/*!!!*/if (m_fv == -0.5) m_fv = 1.0;
#endif
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

inline CFloatVector* CFace::CalcCenter (CSubModel *pso)
{
	CFaceVert		*pfv = m_verts;
	CFloatVector	*pv = pso->m_verts.Buffer ();
	int				i;

m_vCenter.SetZero ();
for (i = m_nVerts; i; i--, pfv++)
	m_vCenter += pv [pfv->m_nIndex];
m_vCenter /= (float) m_nVerts;
return &m_vCenter;
}

//------------------------------------------------------------------------------

inline CFloatVector* CFace::CalcNormal (CSubModel *pso)
{
	CFloatVector	*pv = pso->m_rotVerts.Buffer ();
	CFaceVert		*pfv = m_verts;

m_vRotNormal = CFloatVector::Normal (pv [pfv [0].m_nIndex], pv [pfv [1].m_nIndex], pv [pfv [2].m_nIndex]);
return &m_vRotNormal;
}

//------------------------------------------------------------------------------

#if OOF_TEST_CUBE
/*!!!*/static int nTexId = 0;
#endif

int CFace::Read (CFile& cf, CSubModel *pso, CFaceVert *pfv, int bFlipV)
{
	int	i, v0 = 0;
	CEdge	e;

nIndent += 2;
OOF_PrintLog ("reading face\n");
OOF_ReadVector (cf, &m_vNormal, "vNormal");
#if 0
m_vNormal.x = -m_vNormal.x;
m_vNormal.y = -m_vNormal.y;
m_vNormal.z = -m_vNormal.z;
#endif
m_nVerts = OOF_ReadInt (cf, "nVerts");
m_bTextured = OOF_ReadInt (cf, "bTextured");
if (m_bTextured) {
	m_texProps.nTexId = OOF_ReadInt (cf, "texProps.nTexId");
#if OOF_TEST_CUBE
/*!!!*/	m_texProps.nTexId = nTexId % 6;
/*!!!*/	nTexId++;
#endif
	}
else {
	m_texProps.color.red = OOF_ReadByte (cf, "texProps.color.red");
	m_texProps.color.green = OOF_ReadByte (cf, "texProps.color.green");
	m_texProps.color.blue = OOF_ReadByte (cf, "texProps.color.blue");
	}
#if OOF_MEM_OPT
if (pfv) {
	m_verts = pfv;
#else
	if (!(m_verts= new CFaceVert [m_nVerts])) {
		nIndent -= 2;
		return OOF_FreeFace (&f);
		}
#endif
	OOF_InitMinMax (&m_vMin, &m_vMax);
	e.m_v1 [0] = -1;
	for (i = 0; i < m_nVerts; i++)
		if (!m_verts [i].Read (cf, bFlipV)) {
			nIndent -= 2;
			return 0;
			}
		else {
			e.m_v0 [0] = e.m_v1 [0];
			e.m_v1 [0] = m_verts [i].m_nIndex;
			OOF_GetMinMax (pso->m_verts + e.m_v1 [0], &m_vMin, &m_vMax);
			if (i)
				pso->AddEdge (this, e.m_v0 [0], e.m_v1 [0]);
			else
				v0 = e.m_v1 [0];
			}
	pso->AddEdge (this, e.m_v1 [0], v0);
	CalcCenter (pso);
#if OOF_MEM_OPT
	}
else
	cf.Seek (m_nVerts * sizeof (CFaceVert), SEEK_CUR);
#endif
m_fBoundingLength = OOF_ReadFloat (cf, "fBoundingLength");
m_fBoundingWidth = OOF_ReadFloat (cf, "fBoundingWidth");
nIndent -= 2;
return m_nVerts;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CFaceList::Init (void)
{
m_nFaces = 0;
}

//------------------------------------------------------------------------------

void CFaceList::Destroy (void)
{
m_list.Destroy ();
m_verts.Destroy ();
Init ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CEdgeList::Init (void)
{
m_nEdges = m_nContourEdges = 0;
}

//------------------------------------------------------------------------------

void CEdgeList::Destroy (void)
{
m_list.Destroy ();
Init ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CSubModel::Init (void)
{
m_nIndex = 0;
m_nParent = 0;
m_nFlags = 0;
m_nTreeOffset = 0;
m_nDataOffset = 0;
m_nMovementType = 0;
m_nMovementAxis = 0;
m_nFSLists = 0;
m_nVerts = 0;
m_nChildren = 0;
m_fd = 0;
m_fRadius = 0;
m_fFOV = 0;
m_fRPS = 0;
m_fUpdate = 0;
memset (m_children, 0, sizeof (m_children));
m_pszName = NULL;
m_pszProps = NULL;
m_faces.Init ();
m_edges.Init ();
m_glowInfo.Init ();
m_vNormal.SetZero ();
m_vPlaneVert.SetZero ();
m_vOffset.SetZero ();
m_vCenter.SetZero ();
m_vMin.SetZero ();
m_vMax.SetZero ();
m_vMod.SetZero ();
m_aMod.SetZero ();
m_mMod = CFloatMatrix::IDENTITY;	
}

//------------------------------------------------------------------------------

void CSubModel::Destroy (void)
{
#if !OOF_MEM_OPT
	int	i;
#endif

delete[] m_pszName;
m_pszName = NULL;
delete[] m_pszProps;
m_pszProps = NULL;
m_verts.Destroy ();
m_rotVerts.Destroy ();
m_vertColors.Destroy ();
m_normals.Destroy ();
m_pfAlpha.Destroy ();
m_posAnim.Destroy ();
m_rotAnim.Destroy ();
m_faces.Destroy ();
m_edges.Destroy ();
}

//------------------------------------------------------------------------------

int CSubModel::FindVertex (int i)
{
	CFloatVector	v, *pv;
	int				j;

pv = m_verts.Buffer ();
v = pv [i];
for (j = 0; j < i; j++, pv++)
	if ((v [X] == (*pv) [X]) && (v [Y] == (*pv) [Y]) && (v [Z] == (*pv) [Z]))
		return j;
return i;
}

//------------------------------------------------------------------------------

int CSubModel::FindEdge (int i0, int i1)
{
	int				i;
	CEdge				h;
	CFloatVector	v0, v1, hv0, hv1;

#if DBG
i0 = FindVertex (i0);
i1 = FindVertex (i1);
#endif
for (i = 0; i < m_edges.m_nEdges; i++) {
	h = m_edges.m_list [i];
	if (((h.m_v0 [0] == i0) && (h.m_v1 [0] == i1)) || ((h.m_v0 [0] == i1) && (h.m_v1 [0] == i0)))
		return i;
	}
v0 = m_verts [i0]; 
v1 = m_verts [i1]; 
for (i = 0; i < m_edges.m_nEdges; i++) {
	h = m_edges.m_list [i];
	hv0 = m_verts [h.m_v0 [0]]; 
	hv1 = m_verts [h.m_v1 [0]]; 
	if ((hv0 [X] == v0 [X]) && (hv0 [Y] == v0 [Y]) && (hv0 [Z] == v0 [Z]) &&
		 (hv1 [X] == v1 [X]) && (hv1 [Y] == v1 [Y]) && (hv1 [Z] == v1 [Z]))
		return i;
	if ((hv1 [X] == v0 [X]) && (hv1 [Y] == v0 [Y]) && (hv1 [Z] == v0 [Z]) &&
		 (hv0 [X] == v1 [X]) && (hv0 [Y] == v1 [Y]) && (hv0 [Z] == v1 [Z]))
		return i;
	}
for (i = 0; i < m_edges.m_nEdges; i++) {
	h = m_edges.m_list [i];
	hv0 = m_verts [h.m_v0 [0]] - v0;
	hv1 = m_verts [h.m_v1 [0]] - v1;
	if ((hv0.Mag () < MAXGAP) && (hv1.Mag () < MAXGAP))
		return i;
	hv0 = m_verts [h.m_v0 [0]] - v1;
	hv1 = m_verts [h.m_v1 [0]] - v0;
	if ((hv0.Mag () < MAXGAP) && (hv1.Mag () < MAXGAP))
		return i;
	}
return -1;
}

//------------------------------------------------------------------------------

int CSubModel::AddEdge (CFace *pf, int v0, int v1)
{
	int	i = FindEdge (v0, v1);
	CEdge	*pe;

if (m_nFlags & (OOF_SOF_GLOW | OOF_SOF_THRUSTER))
	return -1;
if (i < 0)
	i = m_edges.m_nEdges++;
pe = m_edges.m_list + i;
if (i < 0) {
	}
if (pe->m_faces [0]) {
	pe->m_faces [1] = pf;
	if (pf->m_bReverse) {
		pe->m_v0 [1] = v1;
		pe->m_v1 [1] = v0;
		}
	else {
		pe->m_v0 [1] = v0;
		pe->m_v1 [1] = v1;
		}
	}
else {
	pe->m_faces [0] = pf;
	if (pf->m_bReverse) {
		pe->m_v0 [0] = v1;
		pe->m_v1 [0] = v0;
		}
	else {
		pe->m_v0 [0] = v0;
		pe->m_v1 [0] = v1;
		}
	}
return i;
}

//------------------------------------------------------------------------------

void CSubModel::SetProps (char *pszProps)
{
	// first, extract the command

	int l;
	char command [200], data [200], *psz;

if (3 > (l = (int) strlen (pszProps)))
	return;
if ((psz = strchr (pszProps, '=')))
	l = (int) (psz - pszProps + 1);
memcpy (command, pszProps, l);
command [l] = '\0';
if (psz)
	strcpy (data, psz + 1);
else
	*data = '\0';

// now act on the command/data pair

if (!stricmp (command,"$rotate=")) { // constant rotation for a subobject
	float spinrate = (float) atof( data);
	if ((spinrate <= 0) || (spinrate > 20))
		return;		// bad data
	m_nFlags |= OOF_SOF_ROTATE;
	m_fRPS = 1.0f / spinrate;
	return;
	}

if (!strnicmp (command,"$jitter",7)) {	// this subobject is a jittery CObject
	m_nFlags |= OOF_SOF_JITTER;
	return;
	}

if (!strnicmp (command,"$shell",6)) { // this subobject is a door shell
	m_nFlags |= OOF_SOF_SHELL;
	return;
	}

if (!strnicmp (command,"$facing",7)) { // this subobject always faces you
	m_nFlags |= OOF_SOF_FACING;
	return;
	}

if (!strnicmp (command,"$frontface",10)) { // this subobject is a door front
	m_nFlags |= OOF_SOF_FRONTFACE;
	return;
	}

if (!stricmp (command,"$glow=")) {
	float r,g,b;
	float size;
	int nValues;

	Assert (!(m_nFlags & (OOF_SOF_GLOW | OOF_SOF_THRUSTER)));
	nValues = sscanf (data, " %f, %f, %f, %f", &r, &g, &b, &size);
	Assert (nValues == 4);
	m_nFlags |= OOF_SOF_GLOW;
	//m_glowInfo = new CGlowInfo;
	m_glowInfo.m_color.red = r;
	m_glowInfo.m_color.green = g;
	m_glowInfo.m_color.blue = b;
	m_glowInfo.m_fSize = size;
	return;
	}

if (!stricmp (command,"$thruster=")) {
	float r,g,b;
	float size;
	int nValues;

	Assert (!(m_nFlags & (OOF_SOF_GLOW | OOF_SOF_THRUSTER)));
	nValues = sscanf(data, " %f, %f, %f, %f", &r,&g,&b,&size);
	Assert(nValues == 4);
	m_nFlags |= OOF_SOF_THRUSTER;
	//m_glowInfo = new CGlowInfo;
	m_glowInfo.m_color.red = r;
	m_glowInfo.m_color.green = g;
	m_glowInfo.m_color.blue = b;
	m_glowInfo.m_fSize = size;
	return;
	}

if (!stricmp (command,"$fov=")) {
	float fov_angle;
	float turret_spr;
	float reactionTime;
	int nValues;

	nValues = sscanf(data, " %f, %f, %f", &fov_angle, &turret_spr, &reactionTime);
	Assert(nValues == 3);
	if (fov_angle < 0.0f || fov_angle > 360.0f) { // Bad data
		Assert(0);
		fov_angle = 1.0;
		}
	// .4 is really fast and really arbitrary
	if (turret_spr < 0.0f || turret_spr > 60.0f) { // Bad data
		Assert(0);
		turret_spr = 1.0f;
		}
	// 10 seconds is really slow and really arbitrary
	if (reactionTime < 0.0f || reactionTime > 10.0f) { // Bad data
		Assert(0);
		reactionTime = 10.0;
		}
	m_nFlags |= OOF_SOF_TURRET;
	m_fFOV = fov_angle/720.0f; // 720 = 360 * 2 and we want to make fov the amount we can move in either direction
	                             // it has a minimum value of (0.0) to [0.5]
	m_fRPS = 1.0f / turret_spr;  // convert spr to rps (rotations per second)
	m_fUpdate = reactionTime;
	return;
	}

if (!stricmp (command,"$monitor01")) { // this subobject is a monitor
	m_nFlags |= OOF_SOF_MONITOR1;
	return;
	}

if (!stricmp (command,"$monitor02")) { // this subobject is a 2nd monitor
	m_nFlags |= OOF_SOF_MONITOR2;
	return;
	}

if (!stricmp (command,"$monitor03")) { // this subobject is a 3rd monitor
	m_nFlags |= OOF_SOF_MONITOR3;
	return;
	}

if (!stricmp (command,"$monitor04")) { // this subobject is a 4th monitor
	m_nFlags |= OOF_SOF_MONITOR4;
	return;
	}

if (!stricmp (command,"$monitor05")) { // this subobject is a 4th monitor
	m_nFlags |= OOF_SOF_MONITOR5;
	return;
	}

if (!stricmp (command,"$monitor06")) { // this subobject is a 4th monitor
	m_nFlags |= OOF_SOF_MONITOR6;
	return;
	}

if (!stricmp (command,"$monitor07")) { // this subobject is a 4th monitor
	m_nFlags |= OOF_SOF_MONITOR7;
	return;
	}

if (!stricmp (command,"$monitor08")) { // this subobject is a 4th monitor
	m_nFlags |= OOF_SOF_MONITOR8;
	return;
	}

if (!stricmp (command,"$viewer")) { // this subobject is a viewer
	m_nFlags |= OOF_SOF_VIEWER;
	return;
	}

if (!stricmp (command,"$layer")) { // this subobject is a layer to be drawn after original CObject.
	m_nFlags |= OOF_SOF_LAYER;
	return;
	}

if (!stricmp (command,"$custom")) { // this subobject has custom textures/colors
	m_nFlags |= OOF_SOF_CUSTOM;
	return;
	}
}

//------------------------------------------------------------------------------

int CSubModel::Read (CFile& cf, CModel* po, int bFlipV)
{
	int				h, i;
#if OOF_MEM_OPT
	int				bReadData, nPos, nFaceVerts = 0;
#endif
	char				szId [20] = "";

nIndent += 2;
OOF_PrintLog ("reading submodel\n");
Init ();
m_nIndex = OOF_ReadInt (cf, "nIndex");
if (m_nIndex >= OOF_MAX_SUBOBJECTS) {
	nIndent -= 2;
	return 0;
	}
m_nParent = OOF_ReadInt (cf, "nParent");
OOF_ReadVector (cf, &m_vNormal, "vNormal");
m_fd = OOF_ReadFloat (cf, "fd");
OOF_ReadVector (cf, &m_vPlaneVert, "vPlaneVert");
OOF_ReadVector (cf, &m_vOffset, "vOffset");
m_fRadius = OOF_ReadFloat (cf, "fRadius");
m_nTreeOffset = OOF_ReadInt (cf, "nTreeOffset");
m_nDataOffset = OOF_ReadInt (cf, "nDataOffset");
if (po->m_nVersion > 1805)
	OOF_ReadVector (cf, &m_vCenter, "vCenter");
if (!(m_pszName = OOF_ReadString (cf, "pszName"))) {
	Destroy ();
	return 0;
	}
if (!(m_pszProps = OOF_ReadString (cf, "pszProps"))) {
	Destroy ();
	return 0;
	}
SetProps (m_pszProps);
m_nMovementType = OOF_ReadInt (cf, "nMovementType");
m_nMovementAxis = OOF_ReadInt (cf, "nMovementAxis");
m_fsLists = NULL;
if ((m_nFSLists = OOF_ReadInt (cf, "nFSLists")))
	cf.Seek (m_nFSLists * sizeof (int), SEEK_CUR);
m_nVerts = OOF_ReadInt (cf, "nVerts");
if (m_nVerts) {
	if (!OOF_ReadVertList (cf, m_verts, m_nVerts, &m_vMin, &m_vMax)) {
		Destroy ();
		nIndent -= 2;
		return 0;
		}
	m_vCenter = (m_vMin + m_vMax) / 2.0f;
	if (!m_rotVerts.Create (m_nVerts)) {
		Destroy ();
		nIndent -= 2;
		return 0;
		}
	if (!m_vertColors.Create (m_nVerts)) {
		Destroy ();
		nIndent -= 2;
		return 0;
		}
	m_vertColors.Clear (0);
	if (!(OOF_ReadVertList (cf, m_normals, m_nVerts, NULL, NULL))) {
		nIndent -= 2;
		Destroy ();
		return 0;
		}
	if (!m_pfAlpha.Create (m_nVerts)) {
		Destroy ();
		nIndent -= 2;
		return 0;
		}
	for (i = 0; i < m_nVerts; i++)
		if (po->m_nVersion < 2300) 
			m_pfAlpha [i] = 1.0f;
		else {
			if (bLogOOF)
				sprintf (szId, "pfAlpha [%d]", i);
			m_pfAlpha [i] = OOF_ReadFloat (cf, szId);
			if	(m_pfAlpha [i] < 0.99)
				po->m_nFlags |= OOF_PMF_ALPHA;
			}
	}
m_faces.m_nFaces = OOF_ReadInt (cf, "nFaces");
if (!m_faces.m_list.Create (m_faces.m_nFaces)) {
	Destroy ();
	nIndent -= 2;
	return 0;
	}
#if OOF_MEM_OPT
nPos = cf.Tell ();
m_edges.m_nEdges = 0;
for (bReadData = 0; bReadData < 2; bReadData++) {
	cf.Seek (nPos, SEEK_SET);
	if (bReadData) {
		if (!m_faces.m_verts.Create (nFaceVerts)) {
			Destroy ();
			nIndent -= 2;
			return 0;
			}
		if (!m_edges.m_list.Create (nFaceVerts)) {
			Destroy ();
			nIndent -= 2;
			return 0;
			}
		m_edges.m_list.Clear ();
		m_edges.m_nEdges = 0;
		}
	for (i = 0, nFaceVerts = 0; i < m_faces.m_nFaces; i++) {
		if (!(h = m_faces.m_list [i].Read (cf, this, bReadData ? m_faces.m_verts + nFaceVerts : NULL, bFlipV))) {
			Destroy ();
			nIndent -= 2;
			return 0;
			}
		nFaceVerts += h;
		}
	}
#else
for (i = 0; i < m_faces.m_nFaces; i++)
	if (!OOF_ReadFace (cf, &so, m_faces.faces + i, NULL, bFlipV)) {
		nIndent -= 2;
		Destroy ();
		return 0;
		}
#endif
nIndent -= 2;
return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int CModel::ReleaseTextures (void)
{
m_textures.Release ();
return 0;
}

//------------------------------------------------------------------------------

int CModel::ReloadTextures (int bCustom)
{
return m_textures.Bind (bCustom);
}

//------------------------------------------------------------------------------

int CModel::FreeTextures (void)
{
m_textures.Destroy ();
return 0;
}

//------------------------------------------------------------------------------

int CModel::ReadTextures (CFile& cf, int bCustom)
{
	int			i;
	char*			pszName;
#if DBG
	int			bOk = 1;
#endif

nIndent += 2;
OOF_PrintLog ("reading textures\n");
int nBitmaps = OOF_ReadInt (cf, "nBitmaps");
#if OOF_TEST_CUBE
/*!!!*/m_textures.nBitmaps = 6;
#endif
if (!(m_textures.Create (nBitmaps))) {
	nIndent -= 2;
	FreeTextures ();
	return 0;
	}
for (i = 0; i < m_textures.m_nBitmaps; i++) {
	if (!(pszName = OOF_ReadString (cf, "texture"))) {
		nIndent -= 2;
		FreeTextures ();
		return 0;
		}
	m_textures.m_names [i].SetBuffer (pszName, 0, strlen (pszName) + 1);
	if (!ReadModelTGA (m_textures.m_names [i].Buffer (), m_textures.m_bitmaps + i, bCustom)) {
#if DBG
		bOk = 0;
#else
		nIndent -= 2;
		FreeTextures ();
		return 0;
#endif
		}
	}
#if DBG
if (!bOk) {
	nIndent -= 2;
	FreeTextures ();
	return 0;
	}
#endif
return 1;
}

//------------------------------------------------------------------------------

void CModel::Init (void)
{
m_nModel = 0;
m_nVersion = 0;
m_nFlags = 0;
m_nDetailLevels = 0;
m_nSubModels = 0;
m_bCloaked = 0;
m_nCloakPulse = 0;
m_nCloakChangedTime = 0;
m_fMaxRadius = 0;
m_fAlpha = 0;
m_vMin.SetZero ();
m_vMax.SetZero ();
m_textures.Init ();
m_frameInfo.Init ();
}

//------------------------------------------------------------------------------

void CModel::Destroy (void)
{
FreeTextures ();
m_subModels.Destroy ();
m_gunPoints.Destroy ();
m_attachPoints.Destroy ();
m_specialPoints.Destroy ();
m_armament.Destroy ();
}

//------------------------------------------------------------------------------

int CModel::ReadInfo (CFile& cf)
{
nIndent += 2;
OOF_PrintLog ("reading OOF model\n");
m_nSubModels = OOF_ReadInt (cf, "nSubModels");
if (m_nSubModels >= OOF_MAX_SUBOBJECTS) {
	nIndent -= 2;
	return 0;
	}
m_fMaxRadius = OOF_ReadFloat (cf, "fMaxRadius");
OOF_ReadVector (cf, &m_vMin, "vMin");
OOF_ReadVector (cf, &m_vMax, "vMax");
m_nDetailLevels = OOF_ReadInt (cf, "nDetailLevels");
nIndent -= 2;
cf.Seek (m_nDetailLevels * sizeof (int), SEEK_CUR);
if (!m_subModels.Create (m_nSubModels))
	return 0;
return 1;
}

//------------------------------------------------------------------------------

void CModel::BuildAnimMatrices (void)
{
for (int i = 0; i < m_nSubModels; i++)
	m_subModels [i].m_rotAnim.BuildMatrices ();
}

//------------------------------------------------------------------------------

void CModel::AssignChildren (void)
{
	CSubModel *pso, *pParent;
	int					i;

for (i = 0, pso = m_subModels.Buffer (); i < m_nSubModels; i++, pso++) {
	int nParent = pso->m_nParent;
	if (nParent == i)
		pso->m_nParent = -1;
	else if (nParent != -1) {
		pParent = m_subModels + nParent;
		pParent->m_children [pParent->m_nChildren++] = i;
		}
	}
}

//------------------------------------------------------------------------------

inline void CModel::LinkSubModelBatteries (int iObject, int iBatt)
{
	CSubModel	*pso = m_subModels + iObject;
	int			i, nFlags = iBatt << OOF_WB_INDEX_SHIFT;

pso->m_nFlags |= nFlags | OOF_SOF_WB;	
for (i = 0; i < pso->m_nChildren; i++)
	LinkSubModelBatteries (pso->m_children [i], iBatt);
}

//------------------------------------------------------------------------------

void CModel::LinkBatteries (void)
{
	CSubModel*	pso;
	CBattery*	pb;
	int*			pti;
	int			i, j, k;

for (i = 0, pso = m_subModels.Buffer (); i < m_nSubModels; i++, pso++) {
	if (!(pso->m_nFlags & OOF_SOF_TURRET))
		continue;
	for (j = 0, pb = m_armament.Buffer (); j < static_cast<int> (m_armament.Length ()); j++, pb++)
		for (k = pb->m_nTurrets, pti = pb->m_turretIndex.Buffer (); k; k--, pti++)
			if (*pti == i) {
				LinkSubModelBatteries (i, j);
				j = m_armament.Length ();
				break;
				}
	}
}

//------------------------------------------------------------------------------

void CModel::BuildPosTickRemapList (void)
{
	int			i, j, k, t, nTicks;
	CSubModel	*pso;

for (i = m_nSubModels, pso = m_subModels.Buffer (); i; i--, pso++) {
	nTicks = pso->m_posAnim.m_nLastFrame - pso->m_posAnim.m_nFirstFrame;
	for (j = 0, t = pso->m_posAnim.m_nFirstFrame; j < nTicks; j++, t++)
		if ((k = pso->m_posAnim.m_nFrames - 1))
			for (; k >= 0; k--)
				if (t >= pso->m_posAnim.m_frames [k].m_nStartTime) {
					pso->m_posAnim.m_remapTicks [j] = k;
					break;
					}
	}
}

//------------------------------------------------------------------------------

void CModel::BuildRotTickRemapList (void)
{
	int				i, j, k, t, nTicks;
	CSubModel	*pso;

for (i = m_nSubModels, pso = m_subModels.Buffer (); i; i--, pso++) {
	nTicks = pso->m_rotAnim.m_nLastFrame - pso->m_rotAnim.m_nFirstFrame;
	for (j = 0, t = pso->m_rotAnim.m_nFirstFrame; j < nTicks; j++, t++)
		if ((k = pso->m_rotAnim.m_nFrames - 1))
			for (; k >= 0; k--)
				if (t >= pso->m_rotAnim.m_frames [k].m_nStartTime) {
					pso->m_rotAnim.m_remapTicks [j] = k;
					break;
					}
	}
}

//------------------------------------------------------------------------------

void CModel::ConfigureSubModels (void)
{
	int			i, j;
	CSubModel	*pso;

for (i = m_nSubModels, pso = m_subModels.Buffer (); i; i--, pso++) {
	if (!pso->m_rotAnim.m_nFrames)
		pso->m_nFlags &= ~(OOF_SOF_ROTATE | OOF_SOF_TURRET);

	if (pso->m_nFlags & OOF_SOF_FACING) {
		CFloatVector v [30], avg;
		CFace	*pf = pso->m_faces.m_list.Buffer ();

		for (j = 0; j < pf->m_nVerts; j++)
			v [j] = pso->m_verts [pf->m_verts [j].m_nIndex];
	
		pso->m_fRadius = (float) (sqrt (OOF_Centroid (&avg, v, pf->m_nVerts)) / 2);
		m_nFlags |= OOF_PMF_FACING;

		}

	if (pso->m_nFlags & (OOF_SOF_GLOW | OOF_SOF_THRUSTER)) {
		CFloatVector v [30];
		CFace	*pf = pso->m_faces.m_list.Buffer ();

		for (j = 0; j < pf->m_nVerts; j++)
			v [j] = pso->m_verts [pf->m_verts [j].m_nIndex];
		pso->m_glowInfo.m_vNormal = CFloatVector::Normal (v [0], v [1], v [2]);
		m_nFlags |= OOF_PMF_FACING;	// Set this so we know when to draw
		}
	}
}

//------------------------------------------------------------------------------

void CModel::GetSubModelBounds (CSubModel *pso, CFloatVector vo)
{
	int	i;
	float	h;

vo += pso->m_vOffset;
if (m_vMax [X] < (h = pso->m_vMax [X] + vo [X]))
	 m_vMax [X] = h;
if (m_vMax [Y] < (h = pso->m_vMax [Y] + vo [Y]))
	 m_vMax [Y] = h;
if (m_vMax [Z] < (h = pso->m_vMax [Z] + vo [Z]))
	 m_vMax [Z] = h;
if (m_vMin [X] > (h = pso->m_vMin [X] + vo [X]))
	 m_vMin [X] = h;
if (m_vMin [Y] > (h = pso->m_vMin [Y] + vo [Y]))
	 m_vMin [Y] = h;
if (m_vMin [Z] > (h = pso->m_vMin [Z] + vo [Z]))
	 m_vMin [Z] = h;
for (i = 0; i < pso->m_nChildren; i++)
	GetSubModelBounds (m_subModels + pso->m_children [i], vo);
}

//------------------------------------------------------------------------------

void CModel::GetBounds (void)
{
	CSubModel*		pso;
	CFloatVector	vo;
	int				i;

vo.SetZero ();
OOF_InitMinMax (&m_vMin, &m_vMax);
for (i = 0, pso = m_subModels.Buffer (); i < m_nSubModels; i++, pso++)
	if (pso->m_nParent == -1)
		GetSubModelBounds (pso, vo);
}

//------------------------------------------------------------------------------

int CModel::Read (char *filename, short nModel, int bFlipV, int bCustom)
{
	CFile			cf;
	char			fileId [4];
	int			i, nLength, nFrames, bTimed = 0;

bLogOOF = (fErr != NULL) && FindArg ("-printoof");
nIndent = 0;
OOF_PrintLog ("\nreading %s/%s\n", gameFolders.szModelDir [bCustom], filename);
if (!cf.Open (filename, gameFolders.szModelDir [bCustom], "rb", 0)) {
	OOF_PrintLog ("  file not found");
	return 0;
	}

if (!cf.Read (fileId, sizeof (fileId), 1)) {
	OOF_PrintLog ("  invalid file id\n");
	cf.Close ();
	return 0;
	}
if (strncmp (fileId, "PSPO", 4)) {
	OOF_PrintLog ("  invalid file id\n");
	cf.Close ();
	return 0;
	}
Init ();
m_nVersion = OOF_ReadInt (cf, "nVersion");
if (m_nVersion >= 2100)
	m_nFlags |= OOF_PMF_LIGHTMAP_RES;
if (m_nVersion >= 22) {
	bTimed = 1;
	m_nFlags |= OOF_PMF_TIMED;
	m_frameInfo.m_nFirstFrame = 0;
	m_frameInfo.m_nLastFrame = 0;
	}
m_nModel = nModel;

while (!cf.EoF ()) {
	char chunkId [4];

	if (!cf.Read (chunkId, sizeof (chunkId), 1)) {
		cf.Close ();
		return 0;
		}
	OOF_PrintLog ("  chunkId = '%c%c%c%c'\n", chunkId [0], chunkId [1], chunkId [2], chunkId [3]);
	nLength = OOF_ReadInt (cf, "nLength");
	switch (ListType (chunkId)) {
		case 0:
			if (!ReadTextures (cf, bCustom)) {
				Destroy ();
				return 0;
				}
			break;

		case 1:
			if (!ReadInfo (cf)) {
				Destroy ();
				return 0;
				}
			break;

		case 2:
			if (!m_subModels [m_nSubModels].Read (cf, this, bFlipV)) {
				Destroy ();
				return 0;
				}
			m_nSubModels++;
			break;

		case 3:
			if (!m_gunPoints.Read (cf, m_nVersion >= 1908, MAX_GUNS)) {
				Destroy ();
				return 0;
				}
			break;

		case 4:
			if (!m_specialPoints.Read (cf)) {
				Destroy ();
				return 0;
				}
			break;

		case 5:
			if (!m_attachPoints.Read (cf)) {
				Destroy ();
				return 0;
				}
			break;

		case 6:
			nFrames = m_frameInfo.m_nFrames;
			if (!bTimed)
				m_frameInfo.m_nFrames = OOF_ReadInt (cf, "nFrames");
			for (i = 0; i < m_nSubModels; i++)
				if (!m_subModels [i].m_posAnim.Read (cf, this, bTimed)) {
					Destroy ();
					return 0;
					}
			if (m_frameInfo.m_nFrames < nFrames)
				m_frameInfo.m_nFrames = nFrames;
			break;

		case 7:
		case 8:
			nFrames = m_frameInfo.m_nFrames;
			if (!bTimed)
				m_frameInfo.m_nFrames = OOF_ReadInt (cf, "nFrames");
			for (i = 0; i < m_nSubModels; i++)
				if (!m_subModels [i].m_rotAnim.Read (cf, this, bTimed)) {
					Destroy ();
					return 0;
					}
			if (m_frameInfo.m_nFrames < nFrames)
				m_frameInfo.m_nFrames = nFrames;
			break;

		case 9:
			if (!m_armament.Read (cf)) {
				Destroy ();
				return 0;
				}
			break;

		case 10:
			if (!m_attachPoints.ReadNormals (cf)) {
				Destroy ();
				return 0;
				}
			break;

		default:
			cf.Seek (nLength, SEEK_CUR);
			break;
		}
	}
cf.Close ();
ConfigureSubModels ();
BuildAnimMatrices ();
AssignChildren ();
LinkBatteries ();
BuildPosTickRemapList ();
BuildRotTickRemapList ();
gameData.models.bHaveHiresModel [this - gameData.models.oofModels [bCustom].Buffer ()] = 1;
return 1;
}

//------------------------------------------------------------------------------

int OOF_ReleaseTextures (void)
{
	CModel*	modelP;
	int		bCustom, i;

PrintLog ("releasing OOF model textures\n");
for (bCustom = 0; bCustom < 2; bCustom++)
	for (i = gameData.models.nHiresModels, modelP = gameData.models.oofModels [bCustom].Buffer (); i; i--, modelP++)
		modelP->ReleaseTextures ();
return 0;
}

//------------------------------------------------------------------------------

int OOF_ReloadTextures (void)
{
	CModel*	modelP;
	int		bCustom, i;

PrintLog ("reloading OOF model textures\n");
for (bCustom = 0; bCustom < 2; bCustom++)
	for (i = gameData.models.nHiresModels, modelP = gameData.models.oofModels [bCustom].Buffer (); i; i--, modelP++)
		if (!modelP->ReloadTextures (bCustom))
			return 0;
return 1;
}

//------------------------------------------------------------------------------

int OOF_FreeTextures (CModel* po)
{
po->ReleaseTextures ();
return 0;
}

//------------------------------------------------------------------------------
//eof

