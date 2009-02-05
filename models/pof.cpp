
#include "descent.h"
#include "pof.h"

using namespace POF;

//------------------------------------------------------------------------------

void CFace::Init (void)
{
memset (this, 0, sizeof (*this));
}

//------------------------------------------------------------------------------

void CSubModel::Init (void)
{
memset (this, 0, sizeof (*this));
}

//------------------------------------------------------------------------------

void CModel::Init (void) 
{
m_nSubModels = 0;
m_nVerts = 0;
m_nFaces = 0;
m_nFaceVerts = 0;
m_nLitFaces = 0;
m_nAdjFaces = 0;
m_iSubObj = 0;
m_iVert = 0;
m_iFace = 0;
m_iFaceVert = 0;
m_nState = 0;
m_nVertFlag = 0;
}

//------------------------------------------------------------------------------

void CModel::Destroy (void) 
{
m_subModels.Destroy ();
m_verts.Destroy ();
m_vertsf.Destroy ();
m_fClipDist.Destroy ();
m_vertFlags.Destroy ();
m_vertNorms.Destroy ();
m_rotVerts.Destroy ();
m_faces.Destroy ();
m_litFaces.Destroy ();
m_adjFaces.Destroy ();
m_faceVerts.Destroy ();
m_vertMap.Destroy ();
Init ();
}

//------------------------------------------------------------------------------

//eof
