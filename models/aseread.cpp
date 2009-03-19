#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif
//#include  "oof.h
#include "descent.h"
#include "u_mem.h"
#include "error.h"
#include "light.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "network.h"
#include "rendermine.h"
#include "strutil.h"
#include "interp.h"
#include "ase.h"

static char	szLine [1024];
static char	szLineBackup [1024];
static int nLine = 0;
static CFile *aseFile = NULL;
static char *pszToken = NULL;
static int bErrMsg = 0;

#define ASE_ROTATE_MODEL	1
#define ASE_FLIP_TEXCOORD	1

using namespace ASE;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int CModel::Error (const char *pszMsg)
{
if (!bErrMsg) {
	if (pszMsg)
		PrintLog ("   %s: error in line %d (%s)\n", aseFile->Name (), nLine, pszMsg);
	else
		PrintLog ("   %s: error in line %d\n", aseFile->Name (), nLine);
	bErrMsg = 1;
	}
return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static float FloatTok (const char *delims)
{
pszToken = strtok (NULL, delims);
if (!(pszToken && *pszToken))
	CModel::Error ("missing data");
return pszToken ? (float) atof (pszToken) : 0;
}

//------------------------------------------------------------------------------

static int IntTok (const char *delims)
{
pszToken = strtok (NULL, delims);
if (!(pszToken && *pszToken))
	CModel::Error ("missing data");
return pszToken ? atoi (pszToken) : 0;
}

//------------------------------------------------------------------------------

static char CharTok (const char *delims)
{
pszToken = strtok (NULL, delims);
if (!(pszToken && *pszToken))
	CModel::Error ("missing data");
return pszToken ? *pszToken : '\0';
}

//------------------------------------------------------------------------------

static char szEmpty [1] = "";

static char *StrTok (const char *delims)
{
pszToken = strtok (NULL, delims);
if (!(pszToken && *pszToken))
	CModel::Error ("missing data");
return pszToken ? pszToken : szEmpty;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static void ReadVector (CFile& cf, CFloatVector3 *pv)
{
#if ASE_ROTATE_MODEL
	float x = FloatTok (" \t");
	float z = -FloatTok (" \t");
	float y = FloatTok (" \t");
	*pv = CFloatVector3::Create(x, y, z);
#else	// need to rotate model for Descent
	int	i;

for (i = 0; i < 3; i++)
	pv [i] = FloatTok (" \t");
#endif
}

//------------------------------------------------------------------------------

static char* ReadLine (CFile& cf)
{
while (!cf.EoF ()) {
	cf.GetS (szLine, sizeof (szLine));
	nLine++;
	strcpy (szLineBackup, szLine);
	strupr (szLine);
	if ((pszToken = strtok (szLine, " \t")))
		return pszToken;
	}
return NULL;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int ASE_ReleaseTextures (void)
{
	CModel*	modelP;
	int		bCustom, i;

PrintLog ("releasing ASE model textures\n");
for (bCustom = 0; bCustom < 2; bCustom++)
	for (i = gameData.models.nHiresModels, modelP = gameData.models.aseModels [bCustom].Buffer (); i; i--, modelP++)
		modelP->ReleaseTextures ();
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReloadTextures (void)
{
	CModel*	modelP;
	int		bCustom, i;

PrintLog ("reloading ASE model textures\n");
for (bCustom = 0; bCustom < 2; bCustom++)
	for (i = gameData.models.nHiresModels, modelP = gameData.models.aseModels [bCustom].Buffer (); i; i--, modelP++)
		if (!modelP->ReloadTextures ()) {
			return 0;
			}
return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CSubModel::Init (void)
{
m_next = NULL;
memset (m_szName, 0, sizeof (m_szName));
memset (m_szParent, 0, sizeof (m_szParent));
m_nSubModel = 0;
m_nParent = 0;
m_nBitmap = 0;
m_nFaces = 0;
m_nVerts = 0;
m_nTexCoord = 0;
m_nIndex = 0;
m_bRender = 1;
m_bGlow = 0;
m_bThruster = 0;
m_bWeapon = 0;
m_nGun = -1;
m_nBomb = -1;
m_nMissile = -1;
m_nType = 0;
m_nWeaponPos = 0;
m_nGunPoint = -1;
m_nBullets = -1;
m_bBarrel = 0;
m_vOffset.SetZero ();
}

//------------------------------------------------------------------------------

void CSubModel::Destroy (void)
{
m_faces.Destroy ();
m_verts.Destroy ();
m_texCoord.Destroy ();
Init ();
}

//------------------------------------------------------------------------------

int CSubModel::ReadNode (CFile& cf)
{
	int	i;

if (CharTok (" \t") != '{')
	return CModel::Error ("syntax error");
while ((pszToken = ReadLine (cf))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*TM_POS")) {
		for (i = 0; i < 3; i++)
			m_vOffset [i] = 0; //FloatTok (" \t");
		}
	}
return CModel::Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

int CSubModel::ReadMeshVertexList (CFile& cf)
{
	CVertex	*pv;
	int					i;

if (CharTok (" \t") != '{')
	return CModel::Error ("syntax error");
while ((pszToken = ReadLine (cf))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_VERTEX")) {
		if (!m_verts)
			return CModel::Error ("no vertices found");
		i = IntTok (" \t");
		if ((i < 0) || (i >= m_nVerts))
			return CModel::Error ("invalid vertex number");
		pv = m_verts + i;
		ReadVector (cf, &pv->m_vertex);
		}	
	}
return CModel::Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

int CSubModel::ReadMeshFaceList (CFile& cf)
{
	CFace	*pf;
	int	i;

if (CharTok (" \t") != '{')
	return CModel::Error ("syntax error");
while ((pszToken = ReadLine (cf))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_FACE")) {
		if (!m_faces)
			return CModel::Error ("no faces found");
		i = IntTok (" \t");
		if ((i < 0) || (i >= m_nFaces))
			return CModel::Error ("invalid face number");
		pf = m_faces + i;
		for (i = 0; i < 3; i++) {
			strtok (NULL, " :\t");
			pf->m_nVerts [i] = IntTok (" :\t");
			}
		do {
			pszToken = StrTok (" :\t");
			if (!*pszToken)
				return CModel::Error ("unexpected end of file");
			} while (strcmp (pszToken, "*MESH_MTLID"));
		pf->m_nBitmap = IntTok (" ");
		}
	}
return CModel::Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

int CSubModel::ReadVertexTexCoord (CFile& cf)
{
	tTexCoord2f		*pt;
	int				i;

if (CharTok (" \t") != '{')
	return CModel::Error ("syntax error");
while ((pszToken = ReadLine (cf))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_TVERT")) {
		if (!m_texCoord)
			return CModel::Error ("no texture coordinates found");
		i = IntTok (" \t");
		if ((i < 0) || (i >= m_nTexCoord))
			return CModel::Error ("invalid texture coordinate number");
		pt = m_texCoord + i;
#if ASE_FLIP_TEXCOORD
		pt->v.u = FloatTok (" \t");
		pt->v.v = -FloatTok (" \t");
#else
		for (i = 0; i < 2; i++)
			pt->a [i] = FloatTok (" \t");
#endif
		}	
	}
return CModel::Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

int CSubModel::ReadFaceTexCoord (CFile& cf)
{
	CFace	*pf;
	int	i;

if (CharTok (" \t") != '{')
	return CModel::Error ("syntax error");
while ((pszToken = ReadLine (cf))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_TFACE")) {
		if (!m_faces)
			return CModel::Error ("no faces found");
		i = IntTok (" \t");
		if ((i < 0) || (i >= m_nFaces))
			return CModel::Error ("invalid face number");
		pf = m_faces + i;
		for (i = 0; i < 3; i++)
			pf->m_nTexCoord [i] = IntTok (" \t");
		}	
	}
return CModel::Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

int CSubModel::ReadMeshNormals (CFile& cf)
{
	CFace*	pf;
	CVertex*	pv;
	int		i;

if (CharTok (" \t") != '{')
	return CModel::Error ("syntax error");
while ((pszToken = ReadLine (cf))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_FACENORMAL")) {
		if (!m_faces)
			return CModel::Error ("no faces found");
		i = IntTok (" \t");
		if ((i < 0) || (i >= m_nFaces))
			return CModel::Error ("invalid face number");
		pf = m_faces + i;
		ReadVector (cf, &pf->m_vNormal);
		}
	else if (!strcmp (pszToken, "*MESH_VERTEXNORMAL")) {
		if (!m_verts)
			return CModel::Error ("no vertices found");
		i = IntTok (" \t");
		if ((i < 0) || (i >= m_nVerts))
			return CModel::Error ("invalid vertex number");
		pv = m_verts + i;
		ReadVector (cf, &pv->m_normal);
		}
	}
return CModel::Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

int CSubModel::ReadMesh (CFile& cf, int& nFaces, int& nVerts)
{
if (CharTok (" \t") != '{')
	return CModel::Error ("syntax error");
while ((pszToken = ReadLine (cf))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_NUMVERTEX")) {
		if (m_verts.Buffer ())
			return CModel::Error ("duplicate vertex list");
		m_nVerts = IntTok (" \t");
		if (!m_nVerts)
			return CModel::Error ("no vertices found");
		nVerts += m_nVerts;
		if (!(m_verts.Create (m_nVerts)))
			return CModel::Error ("out of memory");
		m_verts.Clear ();
		}
	else if (!strcmp (pszToken, "*MESH_NUMTVERTEX")) {
		if (m_texCoord.Buffer ())
			return CModel::Error ("no texture coordinates found");
		m_nTexCoord = IntTok (" \t");
		if (m_nTexCoord) {
			if (!(m_texCoord.Create (m_nTexCoord)))
				return CModel::Error ("out of memory");
			}
		}
	else if (!strcmp (pszToken, "*MESH_NUMFACES")) {
		if (m_faces.Buffer ())
			return CModel::Error ("no faces found");
		m_nFaces = IntTok (" \t");
		if (!m_nFaces)
			return CModel::Error ("no faces specified");
		nFaces += m_nFaces;
		if (!(m_faces.Create (m_nFaces)))
			return CModel::Error ("out of memory");
		m_faces.Clear ();
		}
	else if (!strcmp (pszToken, "*MESH_VERTEX_LIST")) {
		if (!ReadMeshVertexList (cf))
			return CModel::Error (NULL);
		}
	else if (!strcmp (pszToken, "*MESH_FACE_LIST")) {
		if (!ReadMeshFaceList (cf))
			return CModel::Error (NULL);
		}
	else if (!strcmp (pszToken, "*MESH_NORMALS")) {
		if (!ReadMeshNormals (cf))
			return CModel::Error (NULL);
		}
	else if (!strcmp (pszToken, "*MESH_TVERTLIST")) {
		if (!ReadVertexTexCoord (cf))
			return CModel::Error (NULL);
		}
	else if (!strcmp (pszToken, "*MESH_TFACELIST")) {
		if (!ReadFaceTexCoord (cf))
			return CModel::Error (NULL);
		}
	}
return CModel::Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

int CSubModel::Read (CFile& cf, int& nFaces, int& nVerts)
{
while ((pszToken = ReadLine (cf))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*NODE_NAME")) {
		strcpy (m_szName, StrTok (" \t\""));
		if (strstr (m_szName, "$GUNPNT"))
			m_nGunPoint = atoi (m_szName + 8);
		if (strstr (m_szName, "$BULLETS"))
			m_nBullets = 1;
		else if (strstr (m_szName, "GLOW") != NULL) 
			m_bGlow = 1;
		else if (strstr (m_szName, "$DUMMY") != NULL)
			m_bRender = 0;
		else if (strstr (m_szName, "$THRUSTER") != NULL)
			m_bThruster = 1;
		else if (strstr (m_szName, "$WINGTIP") != NULL) {
			m_bWeapon = 1;
			m_nGun = 0;
			m_nBomb =
			m_nMissile = -1;
			m_nType = atoi (m_szName + 8) + 1;
			}
		else if (strstr (m_szName, "$GUN") != NULL) {
			m_bWeapon = 1;
			m_nGun = atoi (m_szName + 4) + 1;
			m_nWeaponPos = atoi (m_szName + 6) + 1;
			m_nBomb =
			m_nMissile = -1;
			}
		else if (strstr (m_szName, "$BARREL") != NULL) {
			m_bWeapon = 1;
			m_nGun = atoi (m_szName + 7) + 1;
			m_nWeaponPos = atoi (m_szName + 9) + 1;
			m_nBomb =
			m_nMissile = -1;
			m_bBarrel = 1;
			}
		else if (strstr (m_szName, "$MISSILE") != NULL) {
			m_bWeapon = 1;
			m_nMissile = atoi (m_szName + 8) + 1;
			m_nWeaponPos = atoi (m_szName + 10) + 1;
			m_nGun =
			m_nBomb = -1;
			}
		else if (strstr (m_szName, "$BOMB") != NULL) {
			m_bWeapon = 1;
			m_nBomb = atoi (m_szName + 6) + 1;
			m_nGun =
			m_nMissile = -1;
			}
		}
	else if (!strcmp (pszToken, "*NODE_PARENT")) {
		strcpy (m_szParent, StrTok (" \t\""));
		}
	if (!strcmp (pszToken, "*NODE_TM")) {
		if (!ReadNode (cf))
			return CModel::Error (NULL);
		}
	else if (!strcmp (pszToken, "*MESH")) {
		if (!ReadMesh (cf, nFaces, nVerts))
			return CModel::Error (NULL);
		}
	else if (!strcmp (pszToken, "*MATERIAL_REF")) {
		m_nBitmap = IntTok (" \t");
		}
	}
return CModel::Error ("unexpected end of file");
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CModel::Init (void)
{
m_subModels = NULL;
m_nModel = -1;
m_nSubModels = 0;
m_nVerts = 0;
m_nFaces = 0;
}

//------------------------------------------------------------------------------

void CModel::Destroy (void)
{
	CSubModel	*psmi, *psmj;

for (psmi = m_subModels; psmi; ) {
	psmj = psmi;
	psmi = psmi->m_next;
	delete psmj;
	}
FreeTextures ();
Init ();
}

//------------------------------------------------------------------------------

int CModel::ReleaseTextures (void)
{
m_textures.Release ();
return 0;
}

//------------------------------------------------------------------------------

int CModel::ReloadTextures (void)
{
return m_textures.Bind (m_bCustom); 
}

//------------------------------------------------------------------------------

int CModel::FreeTextures (void)
{
m_textures.Destroy ();
return 0;
}
//------------------------------------------------------------------------------

int CModel::ReadTexture (CFile& cf, int nBitmap)
{
	CBitmap	*bmP = m_textures.m_bitmaps + nBitmap;
	char		fn [FILENAME_LEN], *ps;
	int		l;

if (CharTok (" \t") != '{')
	return CModel::Error ("syntax error");
bmP->SetFlat (0);
while ((pszToken = ReadLine (cf))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*BITMAP")) {
		if (bmP->Buffer ())	//duplicate
			return CModel::Error ("duplicate item");
		CFile::SplitPath (StrTok ("\""), NULL, fn, NULL);
		if (!ReadModelTGA (::strlwr (fn), bmP, m_bCustom))
			return CModel::Error ("texture not found");
		l = (int) strlen (fn) + 1;
		if (!m_textures.m_names [nBitmap].Create (l))
			return CModel::Error ("out of memory");
		memcpy (m_textures.m_names [nBitmap].Buffer (), fn, l);
		if ((ps = strstr (fn, "color")))
			m_textures.m_nTeam [nBitmap] = atoi (ps + 5) + 1;
		else
			m_textures.m_nTeam [nBitmap] = 0;
		bmP->SetTeam (m_textures.m_nTeam [nBitmap]);
		}
	}
return CModel::Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

int CModel::ReadMaterial (CFile& cf)
{
	int		i;
	CBitmap	*bmP;

i = IntTok (" \t");
if ((i < 0) || (i >= m_textures.m_nBitmaps))
	return CModel::Error ("invalid bitmap number");
if (CharTok (" \t") != '{')
	return CModel::Error ("syntax error");
bmP = m_textures.m_bitmaps + i;
bmP->SetFlat (1);
while ((pszToken = ReadLine (cf))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MATERAL_DIFFUSE")) {
		tRgbColorb	avgRGB;
		avgRGB.red = (ubyte) (FloatTok (" \t") * 255 + 0.5);
		avgRGB.green = (ubyte) (FloatTok (" \t") * 255 + 0.5);
		avgRGB.blue = (ubyte) (FloatTok (" \t") * 255 + 0.5);
		bmP->SetAvgColor (avgRGB);
		}
	else if (!strcmp (pszToken, "*MAP_DIFFUSE")) {
		if (!ReadTexture (cf, i))
			return CModel::Error (NULL);
		}
	}
return CModel::Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

int CModel::ReadMaterialList (CFile& cf)
{
if (CharTok (" \t") != '{')
	return CModel::Error ("syntax error");
if (!(pszToken = ReadLine (cf)))
	return CModel::Error ("unexpected end of file");
if (strcmp (pszToken, "*MATERIAL_COUNT"))
	return CModel::Error ("material count missing");
int nBitmaps = IntTok (" \t");
if (!nBitmaps)
	return CModel::Error ("no bitmaps specified");
if (!(m_textures.Create (nBitmaps)))
	return CModel::Error ("out of memory");
while ((pszToken = ReadLine (cf))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MATERIAL")) {
		if (!ReadMaterial (cf))
			return CModel::Error (NULL);
		}
	}
return CModel::Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

int CModel::ReadSubModel (CFile& cf)
{
	CSubModel	*psm;

if (CharTok (" \t") != '{')
	return CModel::Error ("syntax error");
if (!(psm = new CSubModel))
	return CModel::Error ("out of memory");
psm->m_nSubModel = m_nSubModels++;
psm->m_next = m_subModels;
m_subModels = psm;
return psm->Read (cf, m_nFaces, m_nVerts);
}

//------------------------------------------------------------------------------

int CModel::FindSubModel (const char* pszName)
{
	CSubModel *psm;

for (psm = m_subModels; psm; psm = psm->m_next)
	if (!strcmp (psm->m_szName, pszName))
		return psm->m_nSubModel;
return -1;
}

//------------------------------------------------------------------------------

void CModel::LinkSubModels (void)
{
	CSubModel	*psm;

for (psm = m_subModels; psm; psm = psm->m_next)
	psm->m_nParent = FindSubModel (psm->m_szParent);
}

//------------------------------------------------------------------------------

int CModel::Read (const char* filename, short nModel, int bCustom)
{
#if DBG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif

if (m_nModel >= 0)
	return 0;

if (gameStates.app.bCacheModelData && ReadBinary (nModel, bCustom))
	return 1;

	CFile		cf;
	int		nResult = 1;

if (!cf.Open (filename, gameFolders.szModelDir [bCustom], "rb", 0)) {
	return 0;
	}
bErrMsg = 0;
aseFile = &cf;
Init ();
m_nModel = nModel;
m_bCustom = bCustom;
#if DBG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
nLine = 0;
#endif
while ((pszToken = ReadLine (cf))) {
	if (!strcmp (pszToken, "*MATERIAL_LIST")) {
		if (!(nResult = ReadMaterialList (cf)))
			break;
		}
	else if (!strcmp (pszToken, "*GEOMOBJECT")) {
		if (!(nResult = ReadSubModel (cf)))
			break;
		}
	}
cf.Close ();
if (!nResult)
	Destroy ();
else {
	LinkSubModels ();
	gameData.models.bHaveHiresModel [this - gameData.models.aseModels [bCustom != 0].Buffer ()] = 1;
	if (gameStates.app.bCacheModelData)
		SaveBinary ();
	}
aseFile = NULL;
return nResult;
}

//------------------------------------------------------------------------------

int CSubModel::SaveBinary (CFile& cf)
{
cf.Write (m_szName, 1, sizeof (m_szName));
cf.Write (m_szParent, 1, sizeof (m_szParent));
cf.WriteShort (m_nSubModel);
cf.WriteShort (m_nParent);
cf.WriteShort (m_nBitmap);
cf.WriteShort (m_nFaces);
cf.WriteShort (m_nVerts);
cf.WriteShort (m_nTexCoord);
cf.WriteShort (m_nIndex);
cf.WriteByte (sbyte (m_bRender));
cf.WriteByte (sbyte (m_bGlow));
cf.WriteByte (sbyte (m_bThruster));
cf.WriteByte (sbyte (m_bWeapon));
cf.WriteByte (m_nGun);
cf.WriteByte (m_nBomb);
cf.WriteByte (m_nMissile);
cf.WriteByte (m_nType);
cf.WriteByte (m_nWeaponPos);
cf.WriteByte (m_nGunPoint);
cf.WriteByte (m_nBullets);
cf.WriteByte (m_bBarrel);
cf.WriteVector (m_vOffset);
m_faces.Write (cf);
m_verts.Write (cf);
m_texCoord.Write (cf);
return 1;
}

//------------------------------------------------------------------------------

int CModel::SaveBinary (void)
{
	CFile		cf;
	int		h, i, nResult = 1;
	char		szFilename [FILENAME_LEN];

sprintf (szFilename, "model%03d.bin", m_nModel);
if (!cf.Open (szFilename, gameFolders.szModelDir [m_bCustom], "wb", 0))
	return 0;
cf.WriteInt (m_nModel);
cf.WriteInt (m_nSubModels);
cf.WriteInt (m_nVerts);
cf.WriteInt (m_nFaces);
cf.WriteInt (m_bCustom);
cf.WriteInt (m_textures.m_nBitmaps);

for (i = 0; i < m_textures.m_nBitmaps; i++) {
	h = int (m_textures.m_names [i].Length ());
	cf.WriteInt (h);
	cf.Write (m_textures.m_names [i].Buffer (), 1, h);
	}
cf.Write (m_textures.m_nTeam.Buffer (), 1, m_textures.m_nBitmaps);

for (i = 0; i < m_nSubModels; i++)
	m_subModels [i].SaveBinary (cf);
cf.Close ();
return 1;
}

//------------------------------------------------------------------------------

int CSubModel::ReadBinary (CFile& cf)
{
m_next = NULL;
cf.Read (m_szName, 1, sizeof (m_szName));
cf.Read (m_szParent, 1, sizeof (m_szParent));
m_nSubModel = cf.ReadShort ();
m_nParent = cf.ReadShort ();
m_nBitmap = cf.ReadShort ();
m_nFaces = cf.ReadShort ();
m_nVerts = cf.ReadShort ();
m_nTexCoord = cf.ReadShort ();
m_nIndex = cf.ReadShort ();
m_bRender = ubyte (cf.ReadByte ());
m_bGlow = ubyte (cf.ReadByte ());
m_bThruster = ubyte (cf.ReadByte ());
m_bWeapon = ubyte (cf.ReadByte ());
m_nGun = cf.ReadByte ();
m_nBomb = cf.ReadByte ();
m_nMissile = cf.ReadByte ();
m_nType = cf.ReadByte ();
m_nWeaponPos = cf.ReadByte ();
m_nGunPoint = cf.ReadByte ();
m_nBullets = cf.ReadByte ();
m_bBarrel = cf.ReadByte ();
cf.ReadVector (m_vOffset);
if (!(m_faces.Create (m_nFaces) && m_verts.Create (m_nVerts) && m_texCoord.Create (m_nTexCoord)))
	return 0;
m_faces.Read (cf);
m_verts.Read (cf);
m_texCoord.Read (cf);
return 1;
}

//------------------------------------------------------------------------------

int CModel::ReadBinary (short nModel, int bCustom)
{
	CFile		cf;
	int		h, i, nResult = 1;
	char		szFilename [FILENAME_LEN];

sprintf (szFilename, "model%03d.bin", nModel);
if (!cf.Open (szFilename, gameFolders.szModelDir [bCustom], "rb", 0))
	return 0;
m_nModel = cf.ReadInt ();
m_nSubModels = cf.ReadInt ();
m_nVerts = cf.ReadInt ();
m_nFaces = cf.ReadInt ();
m_bCustom = cf.ReadInt ();

m_subModels = NULL;
m_textures.m_nBitmaps = cf.ReadInt ();
if (!m_textures.m_bitmaps.Create (m_textures.m_nBitmaps)) {
	cf.Close ();
	Destroy ();
	return 0;
	}

for (i = 0; i < m_textures.m_nBitmaps; i++) {
	h = cf.ReadInt ();
	if (!m_textures.m_names [i].Create (h)) {
		cf.Close ();
		Destroy ();
		return 0;
		}
	m_textures.m_names [i].Read (cf);
	if (!ReadModelTGA (m_textures.m_names [i].Buffer (), m_textures.m_bitmaps + i, m_bCustom)) {
		cf.Close ();
		Destroy ();
		return 0;
		}
	}
if (!m_textures.m_nTeam.Create (m_textures.m_nBitmaps)) {
	cf.Close ();
	Destroy ();
	return 0;
	}
m_textures.m_nTeam.Read (cf);

CSubModel*	smP, * tailP = NULL;

m_subModels = NULL;
for (i = 0; i < m_nSubModels; i++) {
	if (!(smP = new CSubModel)) {
		cf.Close ();
		Destroy ();
		return NULL;
		}
	if (m_subModels)
		tailP->m_next = smP;
	else
		m_subModels = smP;
	tailP = smP;
	smP->ReadBinary (cf);
	}

return 1;
}

//------------------------------------------------------------------------------
//eof

