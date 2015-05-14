#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "error.h"
#include "ogl_lib.h"
#include "rendermine.h"
#include "u_mem.h"
#include "segmath.h"
#include "cockpit.h"
#include "newdemo.h"

#define CAMERA_READPIXELS	0
#define TELEPORT_CAMERAS	1

#define CAM_IDX(_pc)	((_pc) - cameraManager.Cameras ())

CCameraManager cameraManager;

//------------------------------------------------------------------------------

int32_t CCamera::CreateBuffer (void)
{
if (!m_data.fbo.Create (CCanvas::Width (), CCanvas::Height (), m_data.bShadowMap ? 3 : 1, m_data.bShadowMap ? 1 : 2))
	return 0;
m_data.glTexId = m_data.bShadowMap ? m_data.fbo.DepthBuffer () : m_data.fbo.ColorBuffer ();
char szName [20];
sprintf (szName, m_data.bShadowMap ? "SHADOW#%04d" : "CAM#%04d", m_data.nId);
SetName (szName);
return 1;
}

//------------------------------------------------------------------------------

int32_t CCamera::HaveBuffer (int32_t bCheckTexture)
{
if (bCheckTexture)
	return Texture () ? Texture ()->FBO ().Available () : -1;
return m_data.fbo.Available ();
}

//------------------------------------------------------------------------------

int32_t CCamera::HaveTexture (void)
{
return Texture () && ((int32_t) Texture ()->Handle () > 0);
}

//------------------------------------------------------------------------------

int32_t CCamera::EnableBuffer (void)
{
ReleaseTexture ();
if (!m_data.fbo.Enable ())
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int32_t CCamera::DisableBuffer (bool bPrepare)
{
if (!m_data.fbo.Disable ())
	return 0;
if (!m_data.bShadowMap && bPrepare && !Prepared ()) {
	SetTranspType (-1);
	PrepareTexture (0, 0, &m_data.fbo);
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t CCamera::BindBuffer (void)
{
if ((HaveBuffer (0) != 1) && !CreateBuffer ())
	return 0;
ogl.BindTexture (m_data.glTexId);
return 1;
}

//------------------------------------------------------------------------------

int32_t CCamera::ReleaseBuffer (void)
{
if (HaveBuffer (0) != 1)
	return 0;
if (!m_data.bShadowMap)
	ReleaseTexture ();
return 1;
}

//------------------------------------------------------------------------------

int32_t CCamera::DestroyBuffer (void)
{
if (!HaveBuffer (0))
	return 0;
m_data.fbo.Destroy ();
return 1;
}

//------------------------------------------------------------------------------

void CCamera::Init (void)
{
memset (&m_data, 0, sizeof (m_data)); 
SetName ("Camera");
}

//------------------------------------------------------------------------------

void CCamera::Setup (int32_t nId, int16_t srcSeg, int16_t srcSide, int16_t tgtSeg, int16_t tgtSide, CObject *pObj, int32_t bTeleport)
{
	CAngleVector a;

if (pObj) {
	m_data.pObj = pObj;
	m_data.orient = pObj->info.position.mOrient;
	m_data.curAngle =
	m_data.curDelta = 0;
	m_data.t0 = 0;
	cameraManager.SetObjectCamera (pObj->Index (), nId);
	}
else {
	m_data.pObj = &m_data.obj;
	if (bTeleport) {
		CFixVector n = *SEGMENT (srcSeg)->m_sides [srcSide].m_normals;
		/*
		n.v.coord.x = -n.v.coord.x;
		n.v.coord.y = -n.v.coord.y;
		*/
		n.Neg ();
		a = n.ToAnglesVec ();
		}
	else
		a = SEGMENT (srcSeg)->m_sides [srcSide].m_normals [0].ToAnglesVec ();
	m_data.obj.info.position.mOrient = CFixMatrix::Create(a);
	if (bTeleport)
		m_data.obj.info.position.vPos = SEGMENT (srcSeg)->Center ();
	else
		m_data.obj.info.position.vPos = SEGMENT (srcSeg)->SideCenter (srcSide);
	m_data.obj.info.nSegment = srcSeg;
	m_data.bMirror = (tgtSeg == srcSeg) && (tgtSide == srcSide);
	}
//m_data.obj.nSide = srcSide;
m_data.nSegment = tgtSeg;
m_data.nSide = tgtSide;
m_data.bTeleport = (char) bTeleport;
}

//------------------------------------------------------------------------------

int32_t Pow2ize (int32_t v, int32_t max);//from ogl.c

int32_t CCamera::Create (int16_t nId, int16_t srcSeg, int16_t srcSide, int16_t tgtSeg, int16_t tgtSide, CObject *pObj, int32_t bShadowMap, int32_t bTeleport)
{
#if 0
	int16_t*		corners;
	CFixVector	*pv;
#endif

Init ();
m_data.nId = nId;
m_data.bShadowMap = bShadowMap;
#if 0
SetWidth (Pow2ize (gameData.render.screen.Width () / (2 - gameOpts->render.cameras.bHires)));
SetHeight (Pow2ize (gameData.render.screen.Height () / (2 - gameOpts->render.cameras.bHires)));
#else
#	if 1
if (bShadowMap) {
#if 0
	int32_t nSize = Max (gameData.render.screen.Width (), gameData.render.screen.Height ());
#else
	int32_t nSize = Pow2ize (int32_t (sqrt (double (gameData.render.screen.Width () * gameData.render.screen.Height ()))));
	while ((nSize > gameData.render.screen.Width ()) || (nSize > gameData.render.screen.Height ()))
		nSize /= 2;
	if (!gameOpts->render.cameras.bHires)
		nSize /= 2;
#endif
	CCanvas::SetWidth (nSize);
	CCanvas::SetHeight (nSize);
	}
else 
#	endif
	{
	int32_t nScreenScale = (!gameOpts->render.cameras.bHires) ? 2 : 1;
	CCanvas::SetWidth (gameData.render.screen.Width () / nScreenScale);
	CCanvas::SetHeight (gameData.render.screen.Height () / nScreenScale);
	}
#endif
SetBPP (4);
if (!CreateBuffer ()) 
	return 0;
Setup (nId, srcSeg, srcSide, tgtSeg, tgtSide, pObj, bTeleport);
return 1;
}

//------------------------------------------------------------------------------

void CCamera::Destroy (void)
{
//this->CBitmap::Destroy ();
//ReleaseTexture ();
if (m_data.glTexId)
	ogl.DeleteTextures (1, &m_data.glTexId);
if (m_data.screenBuf && (m_data.screenBuf != Buffer ())) {
	delete m_data.screenBuf;
	m_data.screenBuf = NULL;
	}
if (Buffer ()) {
	DestroyBuffer ();
	}
if (ogl.m_features.bRenderToTexture)
	DestroyBuffer ();
}

//------------------------------------------------------------------------------

#define EPS 0.01f

void CCamera::Align (CSegFace *pFace, tUVL *pUVL, tTexCoord2f *pTexCoord, CFloatVector3 *pVertex)
{
	int32_t	i2, i3, nType = 0, nScale = 2 - gameOpts->render.cameras.bHires;

if (gameStates.render.bTriangleMesh) {
	if ((nType = pFace->m_info.nType == SIDE_IS_TRI_13)) {
		i2 = 4;
		}
	else {
		i2 = 2;
		}
	i3 = 5;
	}
else {
	i2 = 2;
	i3 = 3;
	}

#if !DBG
if (m_data.bAligned) {
	if (pUVL)
		memcpy (pUVL, m_data.uvlList, sizeof (m_data.uvlList));
	}
else 
#endif
 {
		fix	i;
		float	duImage, dvImage, duFace, dvFace, du, dv,aFace, aImage;
		int32_t	xFlip, yFlip, rotLeft, rotRight;
		int32_t	bHaveBuffer = HaveBuffer (1) == 1;
		int32_t	bFitToWall = 1; //m_data.bTeleport || gameOpts->render.cameras.bFitToWall;
#if DBG
	if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
		BRP;
#endif
	if (pUVL) {
		fix delta = pUVL [1].u - pUVL [0].u;
		rotLeft = (delta > F2X (EPS));
		rotRight = (delta < -F2X (EPS));
		if (rotLeft) {
			yFlip = (pUVL [1].u - pUVL [2].u < -F2X (EPS));
			xFlip = (pUVL [3].v - pUVL [1].v > F2X (EPS));
			}
		else if (rotRight) {
			yFlip = (pUVL [3].u - pUVL [0].u < -F2X (EPS));
			xFlip = (pUVL [1].v - pUVL [3].v > F2X (EPS));
			}
		else {
			xFlip = (pUVL [2].u - pUVL [0].u < -F2X (EPS));
			yFlip = (pUVL [1].v - pUVL [0].v > F2X (EPS));
			}
		dvFace = X2F (pUVL [1].v - pUVL [0].v);
		duFace = X2F (pUVL [2].u - pUVL [0].u);
		}
	else {
		float delta = pTexCoord [1].v.u - pTexCoord [0].v.u;
		rotLeft = (delta > EPS);
		rotRight = (delta < -EPS);
		if (rotLeft) {
			yFlip = (pTexCoord [1].v.u - pTexCoord [i2].v.u > EPS);
			xFlip = (pTexCoord [i3].v.v - pTexCoord [1].v.v < -EPS);
			}
		else if (rotRight) {
			yFlip = (pTexCoord [i3].v.u - pTexCoord [0].v.u > EPS);
			xFlip = (pTexCoord [1].v.v - pTexCoord [i3].v.v < -EPS);
			}
		else {
			xFlip = (pTexCoord [i2].v.u - pTexCoord [0].v.u < -EPS);
			yFlip = (pTexCoord [1].v.v - pTexCoord [0].v.v > EPS);
			}
		dvFace = (float) fabs (pTexCoord [1].v.v - pTexCoord [0].v.v);
		duFace = (float) fabs (pTexCoord [i2].v.u - pTexCoord [0].v.u);
		}
	if (fabs (dvFace) <= EPS)
		dvFace = 0.0f;
	if (fabs (duFace) <= EPS)
		duFace = 0.0f;
	du = dv = 0;
	if (bHaveBuffer) {
		duImage = (float) CCanvas::Current ()->Width () / (float) Width () / nScale;
		dvImage = (float) CCanvas::Current ()->Height () / (float) Height () / nScale;
		if (!bFitToWall) {
			aImage = (float) CCanvas::Current ()->Height () / (float) CCanvas::Current ()->Width ();
			if (pVertex)
				aFace = CFloatVector::Dist(*reinterpret_cast<CFloatVector*> (pVertex), *reinterpret_cast<CFloatVector*> (pVertex + 1)) / 
				        CFloatVector::Dist(*reinterpret_cast<CFloatVector*> (pVertex + 1), *reinterpret_cast<CFloatVector*> (pVertex + i2));
			else
				aFace = dvFace / duFace;
			dv = (aImage - aFace) / (float) nScale;
			duImage -= du / nScale;
			dvImage -= dv;
			}
		}
	else {
		duImage = duFace;
		dvImage = dvFace;
		}
	if (m_data.bMirror)
		xFlip = !xFlip;
	if (pUVL) {
		pUVL [0].v = 
		pUVL [3].v = F2X (yFlip ? dvImage : dv / nScale);
		pUVL [1].v = 
		pUVL [2].v = F2X (yFlip ? dv / nScale : dvImage);
		pUVL [0].u = 
		pUVL [1].u = F2X (xFlip ? duImage : du / nScale);
		pUVL [2].u = 
		pUVL [3].u = F2X (xFlip ? du / nScale : duImage);
		for (i = 0; i < 4; i++)
			pUVL [i].l = I2X (1);
		if (rotRight) {
			for (i = 1; i < 5; i++) {
				m_data.uvlList [i - 1] = pUVL [i % 4];
				}
			}
		else if (rotRight) {
			for (i = 0; i < 4; i++) {
				m_data.uvlList [i] = pUVL [(i + 1) % 4];
				}
			}
		else
			memcpy (m_data.uvlList, pUVL, sizeof (m_data.uvlList));
		}
	else {
		tTexCoord2f texCoord [6];

		texCoord [0].v.v = 
		texCoord [3].v.v = yFlip ? dvImage : dv;
		texCoord [1].v.v = 
		texCoord [2].v.v = yFlip ? dv : dvImage;
		texCoord [0].v.u = 
		texCoord [1].v.u = xFlip ? duImage : du / nScale;
		texCoord [2].v.u = 
		texCoord [3].v.u = xFlip ? du / nScale : duImage;
		if (rotLeft) {
			tTexCoord2f h = texCoord [0];
			texCoord [0] = texCoord [1];
			texCoord [1] = texCoord [2];
			texCoord [2] = texCoord [3];
			texCoord [3] = h;
			}
		else if (rotRight) {
			tTexCoord2f h = texCoord [3];
			texCoord [3] = texCoord [2];
			texCoord [2] = texCoord [1];
			texCoord [1] = texCoord [0];
			texCoord [0] = h;
			}
		if (gameStates.render.bTriangleMesh) {
			if (nType) {
				texCoord [5] = texCoord [3];
				texCoord [4] = texCoord [2];
				texCoord [3] = texCoord [1];
				texCoord [2] = texCoord [5];
				}
			else {
				texCoord [5] = texCoord [3];
				texCoord [4] = texCoord [2];
				texCoord [3] = texCoord [0];
				}
			}
		memcpy (m_data.texCoord, texCoord, sizeof (m_data.texCoord));
		}
#if 0
	if (!gameOpts->render.cameras.bHires) {
		for (int32_t i = 0; i < 6; i++) {
#	if 0
			m_data.texCoord [i].v.u *= 2.0f;
			m_data.texCoord [i].v.v *= 2.0f;
#	else
			m_data.texCoord [i].v.u *= 0.5f;
			m_data.texCoord [i].v.v *= 0.5f;
			//m_data.texCoord [i].v.u += 0.5f;
			m_data.texCoord [i].v.v += 0.5f;
#	endif
			}
		}
#endif
	m_data.bAligned = 1;
	}
}

//------------------------------------------------------------------------------

int32_t CCamera::Ready (time_t t)
{
m_data.nWaitFrames++;
if (!m_data.bVisible)
	return 0;
if (m_data.bTeleport && !EGI_FLAG (bTeleporterCams, 0, 1, 0))
	return 0;
if (m_data.pObj && (m_data.pObj->info.nFlags & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	return 0;
if (gameOpts->render.cameras.nFPS && !m_data.bTimedOut) {
	if (t - m_data.nTimeout < 1000 / gameOpts->render.cameras.nFPS)
		return 0;
	m_data.bTimedOut = 1;
	}
return m_data.nWaitFrames;
}

//------------------------------------------------------------------------------

void CCamera::Reset (void)
{
m_data.nWaitFrames = 0;
m_data.bTimedOut = 0;
m_data.nTimeout = gameStates.app.nSDLTicks [0]; //SDL_GetTicks ();
m_data.bVisible = 0;
}

//------------------------------------------------------------------------------

int32_t CCamera::Render (void)
{
gameStates.render.cameras.bActive = 1;
if (gameStates.render.cockpit.nType != CM_FULL_SCREEN)
	cockpit->Activate (CM_FULL_SCREEN);
//gameOpts->render.nMaxFPS = 1;
if (ReleaseBuffer ()) {
	CObject* pViewerSave = gameData.SetViewer (m_data.pObj ? m_data.pObj : &m_data.obj);
	Activate ("CCamera::Render");
	RenderFrame (0, 0);
	Deactivate ();
	gameData.SetViewer (pViewerSave);
	m_data.bValid = 1;
	DisableBuffer ();
	}
gameStates.render.cameras.bActive = 0;
return m_data.bValid;
}

//--------------------------------------------------------------------

void CCamera::Rotate (void)
{

#define	DEG90		 (I2X (1) / 4)
#define	DEG45		 (I2X (1) / 8)
#define	DEG1		 (I2X (1) / (4 * 90))

if (!m_data.pObj)
	return;

	fixang	curAngle = m_data.curAngle;
	fixang	curDelta = m_data.curDelta;

#if 1
	time_t	t0 = m_data.t0;
	time_t	t = gameStates.app.nSDLTicks [0];

if ((t0 < 0) || (t - t0 >= 1000 / 90))
#endif
	if (m_data.pObj->cType.aiInfo.behavior == AIB_NORMAL) {
		CAngleVector	a;
		CFixMatrix	r;

		int32_t	h = abs (curDelta);
		int32_t	d = DEG1 / (gameOpts->render.cameras.nSpeed / 1000);
		int32_t	s = h ? curDelta / h : 1;

	if (h != d)
		curDelta = s * d;
#if 1
	m_data.pObj->mType.physInfo.brakes = (fix) t;
#endif
	if ((curAngle >= DEG45) || (curAngle <= -DEG45)) {
		if (curAngle * s - DEG45 >= curDelta * s)
			curAngle = s * DEG45 + curDelta - s;
		curDelta = -curDelta;
		}

	curAngle += curDelta;
	a.v.coord.h = curAngle;
	a.v.coord.b = a.v.coord.p = 0;
	r = CFixMatrix::Create (a);
	m_data.pObj->info.position.mOrient = m_data.orient * r;
	m_data.pObj->info.position.mOrient.CheckAndFix ();
	m_data.curAngle = curAngle;
	m_data.curDelta = curDelta;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CCameraManager::~CCameraManager ()
{
Destroy ();
}

//------------------------------------------------------------------------------

int32_t CCameraManager::Create (void)
{
	int32_t		i, j, k;
	uint8_t		t;
	CWall		*pWall;
	CObject	*pObj;
	CTrigger	*pTrigger;

if (!(/*gameStates.app.bD2XLevel &&*/ SEGMENTS.Buffer () && OBJECTS.Buffer ()))
	return 0;
#if 0
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return 0;
#endif
PrintLog (1, "creating cameras\n");
Destroy ();
if (!m_cameras.Create (MAX_CAMERAS)) {
	PrintLog (-1);
	return 0;
	}
if (!m_faceCameras.Create (gameData.segData.nSegments * 6)) {
	PrintLog (-1);
	return 0;
	}
if (!m_objectCameras.Create (LEVEL_OBJECTS)) {
	PrintLog (-1);
	return 0;
	}
m_faceCameras.Clear (0xFF);
m_objectCameras.Clear (0xFF);
#if MAX_SHADOWMAPS
m_shadowMaps.Clear (0xFF);
#endif
if (gameData.trigData.m_nTriggers [0]) {
	for (i = 0, pWall = WALLS.Buffer (); (i < gameData.wallData.nWalls) && (m_cameras.ToS () < MAX_CAMERAS); i++, pWall++) {
		t = pWall->nTrigger;
		if (t >= GEOTRIGGERS.Length ())
			continue;
		pTrigger = GEOTRIGGER (t);
		if (pTrigger && (pTrigger->m_info.nType == TT_CAMERA)) {
			for (j = 0; j < pTrigger->m_nLinks; j++)
				if (m_cameras.Grow () &&
					 m_cameras.Top ()->Create (m_cameras.ToS () - 1, (int16_t) pWall->nSegment, (int16_t) pWall->nSide, pTrigger->m_segments [j], pTrigger->m_sides [j], NULL, 0, 0))
					SetFaceCamera (pTrigger->m_segments [j] * 6 + pTrigger->m_sides [j], (char) m_cameras.ToS () - 1);
			}
#if TELEPORT_CAMERAS
		else if (EGI_FLAG (bTeleporterCams, 0, 1, 0) && (pTrigger->m_info.nType == TT_TELEPORT)) {
			if (m_cameras.Grow () &&
				 m_cameras.Top ()->Create (m_cameras.ToS () - 1, pTrigger->m_segments [0], pTrigger->m_sides [0], (int16_t) pWall->nSegment, (int16_t) pWall->nSide, NULL, 0, 1))
				SetFaceCamera (pWall->nSegment * 6 + pWall->nSide, (char) m_cameras.ToS () + 1);
			}
#endif
		}
	}

if (gameData.trigData.m_nTriggers [1]) {
	FORALL_OBJS (pObj) {
		if ((j = gameData.trigData.objTriggerRefs [pObj->Index ()].nFirst) >= int32_t (OBJTRIGGERS.Length ()))
			continue;
		if (!(pTrigger = OBJTRIGGER (j)))
			continue;
		j = gameData.trigData.objTriggerRefs [pObj->Index ()].nCount;
#if DBG
		if (j >= 0)
			j = j;
#endif
		for (; j && (m_cameras.ToS () < MAX_CAMERAS); j--, pTrigger++) {
			if (pTrigger->m_info.nType == TT_CAMERA) {
				for (k = 0; k < pTrigger->m_nLinks; k++)
					if (m_cameras.Grow () &&
						 m_cameras.Top ()->Create (m_cameras.ToS () - 1, -1, -1, pTrigger->m_segments [k], pTrigger->m_sides [k], pObj, 0, 0))
						SetFaceCamera (pTrigger->m_segments [k] * 6 + pTrigger->m_sides [k], (char) m_cameras.ToS () - 1);
				}
			}
		}
	}
PrintLog (-1);
return 1;
}

//------------------------------------------------------------------------------

void CCameraManager::Destroy (void)
{
if (m_cameras.Buffer ()) {
	PrintLog (1, "Destroying cameras\n");
	for (uint32_t i = 0; i < m_cameras.ToS (); i++)
		m_cameras [i].Destroy ();
	m_cameras.Destroy ();
	m_faceCameras.Destroy ();
	m_objectCameras.Destroy ();
#if MAX_SHADOWMAPS
	m_shadowMaps.Clear (0xFF);
#endif
	PrintLog (-1);
	}
}

//------------------------------------------------------------------------------

int32_t CCameraManager::Render (void)
{
	uint32_t		i;
	CObject		*pViewerSave = gameData.objData.pViewer;
	time_t		t;
	int32_t		nCamsRendered;
	int32_t		cm = gameStates.render.cockpit.nType;
//	int32_t		frameCap = gameOpts->render.nMaxFPS;
	int32_t		nWaitFrames, nMaxWaitFrames = -1;

if (!gameStates.app.bD2XLevel)
	return 0;
if (!extraGameInfo [0].bUseCameras)
	return 0;
if (IsMultiGame && !(gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bUseCameras))
	return 0;
nCamsRendered = 0;
t = SDL_GetTicks ();
m_current = NULL;
for (i = 0; i < m_cameras.ToS (); i++) {
	if (m_cameras [i].Type () != 3) { // 3 -> shadow map
		nWaitFrames = m_cameras [i].Ready (t);
		if (nWaitFrames && (nMaxWaitFrames < nWaitFrames)) {
			nMaxWaitFrames = nWaitFrames;
			m_current = m_cameras + i;
			}
		}
	}
if (m_current) {
	m_current->Reset ();
	nCamsRendered += m_current->Render ();
	}
gameData.SetViewer (pViewerSave);
//gameOpts->render.nMaxFPS = frameCap;
if (gameStates.render.cockpit.nType != cm) {
	cockpit->Activate (cm);
	return nCamsRendered;
	}
return 0;
}

//------------------------------------------------------------------------------

inline int32_t CCameraManager::GetObjectCamera (int32_t nObject) 
{
if (!(m_cameras.Buffer () && m_objectCameras.Buffer ()) || (nObject < 0) || (nObject >= LEVEL_OBJECTS))
	return -1;
return m_objectCameras [nObject];
}

//------------------------------------------------------------------------------

inline void CCameraManager::SetObjectCamera (int32_t nObject, int32_t i) 
{
if (m_objectCameras.Buffer () && (nObject >= 0) && (nObject < LEVEL_OBJECTS))
	m_objectCameras [nObject] = i;
}

//------------------------------------------------------------------------------

int32_t CCameraManager::GetFaceCamera (int32_t nFace) 
{
return (m_cameras.Buffer () && m_faceCameras.Buffer ()) ? m_faceCameras [nFace] : -1;
}

//------------------------------------------------------------------------------

void CCameraManager::SetFaceCamera (int32_t nFace, int32_t i) 
{
if (m_faceCameras.Buffer ())
	m_faceCameras [nFace] = i;
}

//--------------------------------------------------------------------

CCamera* CCameraManager::Camera (CObject *pObj)
{
	int16_t i = GetObjectCamera (pObj->Index ());

return (i < 0) ? NULL : m_cameras + i;
}

//--------------------------------------------------------------------

void CCameraManager::Rotate (CObject *pObj)
{
	int16_t i = GetObjectCamera (pObj->Index ());

if (i >= 0)
	m_cameras [i].Rotate ();
}

//------------------------------------------------------------------------------

void CCameraManager::ReAlign (void)
{
Destroy ();
Create ();
for (uint32_t i = 0; i < m_cameras.ToS (); i++)
	m_cameras [i].ReAlign ();
}

//------------------------------------------------------------------------------
