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

int CCamera::CreateBuffer (void)
{
#if RENDER2TEXTURE == 1
if (!OglCreatePBuffer (&m_data.pb, Width (), Height (), 0))
	return 0;
m_data.glTexId = m_data.pb.texId;
#elif RENDER2TEXTURE == 2
if (!m_data.fbo.Create (Width (), Height (), 1/*m_data.bShadowMap*/))
	return 0;
m_data.glTexId = m_data.fbo.ColorBuffer ();
#endif
char szName [20];
sprintf (szName, "CAM#%04d", m_data.nId);
SetName (szName);
return 1;
}

//------------------------------------------------------------------------------

int CCamera::HaveBuffer (int bCheckTexture)
{
if (bCheckTexture)
#if RENDER2TEXTURE == 1
	return Texture () ? m_data.Texture ()->PBO ()->Available() : -1;
return OglPHaveBuffer (&m_data.pb);
#elif RENDER2TEXTURE == 2
	return Texture () ? Texture ()->FBO ().Available () : -1;
return m_data.fbo.Available ();
#endif
}

//------------------------------------------------------------------------------

int CCamera::HaveTexture (void)
{
return Texture () && ((int) Texture ()->Handle () > 0);
}

//------------------------------------------------------------------------------

int CCamera::EnableBuffer (void)
{
ReleaseTexture ();
#if RENDER2TEXTURE == 1
if (!OglEnablePBuffer (&m_data.pb))
	return 0;
#elif RENDER2TEXTURE == 2
if (!m_data.fbo.Enable ())
	return 0;
#endif
return 1;
}

//------------------------------------------------------------------------------

int CCamera::DisableBuffer (bool bPrepare)
{
#if RENDER2TEXTURE == 1
if (!OglDisablePBuffer (&m_data.pb))
	return 0;
if (!Prepared ())
	PrepareTexture (&m_data.buffer, 0, -1, 0, &m_data.pb);
#elif RENDER2TEXTURE == 2
if (!m_data.fbo.Disable ())
	return 0;
if (bPrepare && !Prepared ()) {
	SetTranspType (-1);
	PrepareTexture (0, 0, &m_data.fbo);
	}
#endif
return 1;
}

//------------------------------------------------------------------------------

int CCamera::BindBuffer (void)
{
if ((HaveBuffer (0) != 1) && !CreateBuffer ())
	return 0;
ogl.BindTexture (m_data.glTexId);
#if RENDER2TEXTURE == 1
#	ifdef _WIN32
return m_data.pbo.Bind ();
#	endif
#endif
return 1;
}

//------------------------------------------------------------------------------

int CCamera::ReleaseBuffer (void)
{
if (HaveBuffer (0) != 1)
	return 0;
#if RENDER2TEXTURE == 1
if (!m_data.pb.bBound)
	return 1;
#endif
ReleaseTexture ();
#if RENDER2TEXTURE == 1
m_data.pb.bBound = 0;
#endif
return 1;
}

//------------------------------------------------------------------------------

int CCamera::DestroyBuffer (void)
{
if (!HaveBuffer (0))
	return 0;
#if RENDER2TEXTURE == 1
OglDestroyPBuffer (&m_data.pb);
#elif RENDER2TEXTURE == 2
m_data.fbo.Destroy ();
#endif
return 1;
}

//------------------------------------------------------------------------------

void CCamera::Init (void)
{
memset (&m_data, 0, sizeof (m_data)); 
}

//------------------------------------------------------------------------------

int Pow2ize (int x);//from ogl.c

int CCamera::Create (short nId, short srcSeg, short srcSide, short tgtSeg, short tgtSide, 
							CObject *objP, int bShadowMap, int bTeleport)
{
	CAngleVector	a;
#if 0
	short*		corners;
	CFixVector	*pv;
#endif

Init ();
m_data.nId = nId;
m_data.bShadowMap = bShadowMap;
SetWidth (Pow2ize (screen.Width () / (2 - gameOpts->render.cameras.bHires)));
SetHeight (Pow2ize (screen.Height () / (2 - gameOpts->render.cameras.bHires)));
SetBPP (4);
//SetRowSize (max (CCanvas::Current ()->Width (), Width ()));
#if RENDER2TEXTURE
if (!CreateBuffer ()) 
#endif
{
#if CAMERA_READPIXELS
	if (!(Create (Width () * Height () * 4)))
		return 0;
	if (gameOpts->render.cameras.bFitToWall || m_data.bTeleport)
		m_data.screenBuf = Buffer ();
	else {
		m_data.screenBuf = new ubyte [CCanvas::Current ()->Width () * CCanvas::Current ()->Height () * 4];
		if (!m_data.screenBuf) {
			gameOpts->render.cameras.bFitToWall = 1;
			m_data.screenBuf = Buffer ();
			}
		}
	memset (Buffer (), 0, Width () * Height () * 4);
#else
	return 0;
#endif
	}
if (objP) {
	m_data.objP = objP;
	m_data.orient = objP->info.position.mOrient;
	m_data.curAngle =
	m_data.curDelta = 0;
	m_data.t0 = 0;
	cameraManager.SetObjectCamera (objP->Index (), nId);
	}
else {
	m_data.objP = &m_data.obj;
	if (bTeleport) {
		CFixVector n = *SEGMENTS [srcSeg].m_sides [srcSide].m_normals;
		/*
		n[X] = -n[X];
		n[Y] = -n[Y];
		*/
		n.Neg ();
		a = n.ToAnglesVec ();
		}
	else
		a = SEGMENTS [srcSeg].m_sides [srcSide].m_normals [0].ToAnglesVec ();
	m_data.obj.info.position.mOrient = CFixMatrix::Create(a);
#if 1
	if (bTeleport)
		m_data.obj.info.position.vPos = SEGMENTS [srcSeg].Center ();
	else
		m_data.obj.info.position.vPos = SEGMENTS [srcSeg].SideCenter (srcSide);
#else
	corners = SEGMENTS [srcSeg].Corners (srcSide);
	for (i = 0; i < 4; i++) {
		pv = gameData.segs.vertices + corners [i];
		m_data.obj.info.position.p.vPos.x += pv->x;
		m_data.obj.info.position.p.vPos.y += pv->y;
		m_data.obj.info.position.p.vPos.z += pv->z;
		}
	m_data.obj.info.position.p.vPos.x /= 4;
	m_data.obj.info.position.p.vPos.y /= 4;
	m_data.obj.info.position.p.vPos.z /= 4;
#endif
	m_data.obj.info.nSegment = srcSeg;
	m_data.bMirror = (tgtSeg == srcSeg) && (tgtSide == srcSide);
	}
//m_data.obj.nSide = srcSide;
m_data.nSegment = tgtSeg;
m_data.nSide = tgtSide;
m_data.bTeleport = (char) bTeleport;
return 1;
}

//------------------------------------------------------------------------------

void CCamera::Destroy (void)
{
ReleaseTexture ();
ogl.DeleteTextures (1, &m_data.glTexId);
if (m_data.screenBuf && (m_data.screenBuf != Buffer ())) {
	delete m_data.screenBuf;
	m_data.screenBuf = NULL;
	}
if (Buffer ()) {
	DestroyBuffer ();
	}
#if RENDER2TEXTURE
if (ogl.m_states.bRender2TextureOk)
	DestroyBuffer ();
#endif
}

//------------------------------------------------------------------------------

#define EPS 0.01f

void CCamera::Align (CSegFace *faceP, tUVL *uvlP, tTexCoord2f *texCoordP, CFloatVector3 *vertexP)
{
	int	i2, i3, nType = 0, nScale = 2 - gameOpts->render.cameras.bHires;

if (gameStates.render.bTriangleMesh) {
	if ((nType = faceP->m_info.nType == SIDE_IS_TRI_13)) {
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
	if (uvlP)
		memcpy (uvlP, m_data.uvlList, sizeof (m_data.uvlList));
	}
else 
#endif
 {
		fix	i;
		float	duImage, dvImage, duFace, dvFace, du, dv,aFace, aImage;
		int	xFlip, yFlip, rotLeft, rotRight;
#if RENDER2TEXTURE
		int	bHaveBuffer = HaveBuffer (1) == 1;
		int	bFitToWall = 1; //m_data.bTeleport || gameOpts->render.cameras.bFitToWall;
#endif
#if DBG
	if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	if (uvlP) {
		fix delta = uvlP [1].u - uvlP [0].u;
		rotLeft = (delta > F2X (EPS));
		rotRight = (delta < -F2X (EPS));
		if (rotLeft) {
			yFlip = (uvlP [1].u - uvlP [2].u < -F2X (EPS));
			xFlip = (uvlP [3].v - uvlP [1].v > F2X (EPS));
			}
		else if (rotRight) {
			yFlip = (uvlP [3].u - uvlP [0].u < -F2X (EPS));
			xFlip = (uvlP [1].v - uvlP [3].v > F2X (EPS));
			}
		else {
			xFlip = (uvlP [2].u - uvlP [0].u < -F2X (EPS));
			yFlip = (uvlP [1].v - uvlP [0].v > F2X (EPS));
			}
		dvFace = X2F (uvlP [1].v - uvlP [0].v);
		duFace = X2F (uvlP [2].u - uvlP [0].u);
		}
	else {
		float delta = texCoordP [1].v.u - texCoordP [0].v.u;
		rotLeft = (delta > EPS);
		rotRight = (delta < -EPS);
		if (rotLeft) {
			yFlip = (texCoordP [1].v.u - texCoordP [i2].v.u > EPS);
			xFlip = (texCoordP [i3].v.v - texCoordP [1].v.v < -EPS);
			}
		else if (rotRight) {
			yFlip = (texCoordP [i3].v.u - texCoordP [0].v.u > EPS);
			xFlip = (texCoordP [1].v.v - texCoordP [i3].v.v < -EPS);
			}
		else {
			xFlip = (texCoordP [i2].v.u - texCoordP [0].v.u < -EPS);
			yFlip = (texCoordP [1].v.v - texCoordP [0].v.v > EPS);
			}
		dvFace = (float) fabs (texCoordP [1].v.v - texCoordP [0].v.v);
		duFace = (float) fabs (texCoordP [i2].v.u - texCoordP [0].v.u);
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
			if (vertexP)
				aFace = CFloatVector::Dist(*reinterpret_cast<CFloatVector*> (vertexP), *reinterpret_cast<CFloatVector*> (vertexP + 1)) / 
				        CFloatVector::Dist(*reinterpret_cast<CFloatVector*> (vertexP + 1), *reinterpret_cast<CFloatVector*> (vertexP + i2));
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
	if (uvlP) {
		uvlP [0].v = 
		uvlP [3].v = F2X (yFlip ? dvImage : dv / nScale);
		uvlP [1].v = 
		uvlP [2].v = F2X (yFlip ? dv / nScale : dvImage);
		uvlP [0].u = 
		uvlP [1].u = F2X (xFlip ? duImage : du / nScale);
		uvlP [2].u = 
		uvlP [3].u = F2X (xFlip ? du / nScale : duImage);
		for (i = 0; i < 4; i++)
			uvlP [i].l = I2X (1);
		if (rotRight) {
			for (i = 1; i < 5; i++) {
				m_data.uvlList [i - 1] = uvlP [i % 4];
				}
			}
		else if (rotRight) {
			for (i = 0; i < 4; i++) {
				m_data.uvlList [i] = uvlP [(i + 1) % 4];
				}
			}
		else
			memcpy (m_data.uvlList, uvlP, sizeof (m_data.uvlList));
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
	m_data.bAligned = 1;
	}
}

//------------------------------------------------------------------------------

int CCamera::Ready (time_t t)
{
m_data.nWaitFrames++;
if (!m_data.bVisible)
	return 0;
if (m_data.bTeleport && !EGI_FLAG (bTeleporterCams, 0, 1, 0))
	return 0;
if (m_data.objP && (m_data.objP->info.nFlags & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED)))
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

int CCamera::Render (void)
{
gameStates.render.cameras.bActive = 1;
if (gameStates.render.cockpit.nType != CM_FULL_SCREEN)
	cockpit->Activate (CM_FULL_SCREEN);
gameData.objs.viewerP = m_data.objP;
//gameOpts->render.nMaxFPS = 1;
#if RENDER2TEXTURE
if (ReleaseBuffer () /*&& ogl.SelectDrawBuffer (-cameraManager.Index (this) - 1)*/) {
	//int h = gameOpts->render.stereo.nGlasses;
	//CCanvas::SetCurrent (this);
	RenderFrame (0, 0);
	m_data.bValid = 1;
	DisableBuffer ();
	}
else 
#	if CAMERA_READPIXELS
if (Buffer ()) 
#	endif
#endif
 {
	RenderFrame (0, 0);
	m_data.bValid = 1;
	ReleaseTexture ();
#if CAMERA_READPIXELS
	memset (Buffer (), 0, Width () * Height () * 4);
	ogl.SetTexturing (false);
#endif
	glReadBuffer (GL_BACK);
	if (gameOpts->render.cameras.bFitToWall || m_data.bTeleport) {
#if CAMERA_READPIXELS == 0
		SetTranspType (-1);
		PrepareTexture (0, 0, NULL);
		glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 
			(CCanvas::Current ()->Width () - Width ()) / 2, 
			(CCanvas::Current ()->Height () - Height ()) / 2, 
			Width (), Height (), 0);
#else
		ogl.SetReadBuffer (GL_FRONT, 0);
		glReadPixels (
			(CCanvas::Current ()->Width () - Width ()) / 2, 
			(CCanvas::Current ()->Height () - Height ()) / 2, 
			Width (), Height (), 
			GL_RGBA, GL_UNSIGNED_BYTE, Buffer ());
		PrepareTexture (&m_data.buffer, 0, -1, NULL);
#endif
		}
	else {
#if CAMERA_READPIXELS == 0
			SetTranspType (-1);
			PrepareTexture (0, 0, NULL);
			glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 
				-(Width () - CCanvas::Current ()->Width ()) / 2,
				-(Height () - CCanvas::Current ()->Height ()) / 2, 
				Width (), Height (), 0);
#else
		char	*pSrc, *pDest;
		int	dxBuf = (Width () - CCanvas::Current ()->Width ()) / 2;
		int	dyBuf = (Height () - CCanvas::Current ()->Height ()) / 2;
		int	wSrc = CCanvas::Current ()->Width () * 4;
		int	wDest = Width () * 4;

		if (CCanvas::Current ()->Width () == Width ()) {
			ogl.SetReadBuffer (GL_FRONT, 0);
			glReadPixels (
				0, 0, CCanvas::Current ()->Width (), CCanvas::Current ()->Height (),
				GL_RGBA, GL_UNSIGNED_BYTE, Buffer () + dyBuf * Width () * 4);
			}
		else {
			ogl.SetReadBuffer (GL_FRONT, 0);
			glReadPixels (
				0, 0, CCanvas::Current ()->Width (), CCanvas::Current ()->Height (),
				GL_RGBA, GL_UNSIGNED_BYTE, m_data.screenBuf);
			pSrc = m_data.screenBuf;
			pDest = Buffer () + (dyBuf - 1) * wDest + dxBuf * 4;
#	ifndef _WIN32
			for (dyBuf = CCanvas::Current ()->Height (); dyBuf; dyBuf--, pSrc += wSrc, pDest += wDest)
				memcpy (pDest, pSrc, wSrc);
#	else
			dxBuf = Width () - CCanvas::Current ()->Width ();
			dyBuf = CCanvas::Current ()->Height ();
			wSrc /= 4;
			__asm {
				push	edi
				push	esi
				push	ecx
				mov	edx,dyBuf
				mov	esi,pSrc
				mov	edi,pDest
				mov	eax,dxBuf
				shl	eax,2
				cld
		copy:
				mov	ecx,wSrc
				rep	movsd
				dec	edx
				jz		done
				add	edi,eax
				jmp	copy
		done:
				pop	ecx
				pop	esi
				pop	edi
				}
#	endif
			PrepareTexture (&m_data.buffer, 0, -1);
			}
#endif
		}
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

if (!m_data.objP)
	return;

	fixang	curAngle = m_data.curAngle;
	fixang	curDelta = m_data.curDelta;

#if 1
	time_t	t0 = m_data.t0;
	time_t	t = gameStates.app.nSDLTicks [0];

if ((t0 < 0) || (t - t0 >= 1000 / 90))
#endif
	if (m_data.objP->cType.aiInfo.behavior == AIB_NORMAL) {
		CAngleVector	a;
		CFixMatrix	r;

		int	h = abs (curDelta);
		int	d = DEG1 / (gameOpts->render.cameras.nSpeed / 1000);
		int	s = h ? curDelta / h : 1;

	if (h != d)
		curDelta = s * d;
#if 1
	m_data.objP->mType.physInfo.brakes = (fix) t;
#endif
	if ((curAngle >= DEG45) || (curAngle <= -DEG45)) {
		if (curAngle * s - DEG45 >= curDelta * s)
			curAngle = s * DEG45 + curDelta - s;
		curDelta = -curDelta;
		}

	curAngle += curDelta;
	a [HA] = curAngle;
	a [BA] = a [PA] = 0;
	r = CFixMatrix::Create (a);
	m_data.objP->info.position.mOrient = m_data.orient * r;
	m_data.objP->info.position.mOrient.CheckAndFix ();
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

int CCameraManager::Create (void)
{
	int		i, j, k;
	ubyte		t;
	CWall		*wallP;
	CObject	*objP;
	CTrigger	*triggerP;

if (!(gameStates.app.bD2XLevel && SEGMENTS.Buffer () && OBJECTS.Buffer ()))
	return 0;
#if 0
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	return 0;
#endif
PrintLog ("   creating cameras\n");
if (!m_faceCameras.Create (LEVEL_FACES))
	return 0;
if (!m_objectCameras.Create (LEVEL_OBJECTS))
	return 0;
m_faceCameras.Clear (0xFF);
m_objectCameras.Clear (0xFF);

if (gameData.trigs.m_nTriggers) {
	for (i = 0, wallP = WALLS.Buffer (); (i < gameData.walls.nWalls) && (m_nCameras < MAX_CAMERAS); i++, wallP++) {
		t = wallP->nTrigger;
		if (t >= TRIGGERS.Length ())
			continue;
		triggerP = TRIGGERS + t;
		if (triggerP->m_info.nType == TT_CAMERA) {
			for (j = 0; j < triggerP->m_info.nLinks; j++)
				if (m_cameras [m_nCameras].Create (m_nCameras, (short) wallP->nSegment, (short) wallP->nSide, triggerP->m_info.segments [j], triggerP->m_info.sides [j], NULL, 0, 0))
					SetFaceCamera (triggerP->m_info.segments [j] * 6 + triggerP->m_info.sides [j], (char) m_nCameras++);
			}
#if TELEPORT_CAMERAS
		else if (/*EGI_FLAG (bTeleporterCams, 0, 0) &&*/ (triggerP->m_info.nType == TT_TELEPORT)) {
			if (m_cameras [m_nCameras].Create (m_nCameras, triggerP->m_info.segments [0], triggerP->m_info.sides [0], (short) wallP->nSegment, (short) wallP->nSide, NULL, 0, 1))
				SetFaceCamera (wallP->nSegment * 6 + wallP->nSide, (char) m_nCameras++);
			}
#endif
		}
	}

if (gameData.trigs.m_nObjTriggers) {
	FORALL_OBJS (objP, i) {
		if ((j = gameData.trigs.objTriggerRefs [objP->Index ()].nFirst) >= int (OBJTRIGGERS.Length ()))
			continue;
		triggerP = OBJTRIGGERS + j;
		j = gameData.trigs.objTriggerRefs [objP->Index ()].nCount;
#if DBG
		if (j >= 0)
			j = j;
#endif
		for (; j && (m_nCameras < MAX_CAMERAS); j--, triggerP++) {
			if (triggerP->m_info.nType == TT_CAMERA) {
				for (k = 0; k < triggerP->m_info.nLinks; k++)
					if (m_cameras [m_nCameras].Create (m_nCameras, -1, -1, triggerP->m_info.segments [k], triggerP->m_info.sides [k], objP, 0, 0))
						SetFaceCamera (triggerP->m_info.segments [k] * 6 + triggerP->m_info.sides [k], (char) m_nCameras++);
				}
			}
		}
	}
return m_nCameras;
}

//------------------------------------------------------------------------------

void CCameraManager::Destroy (void)
{
#if RENDER2TEXTURE == 1
if (ogl.m_states.bRender2TextureOk)
#	ifdef _WIN32
	wglMakeContextCurrentARB (hGlDC, hGlDC, hGlRC);
#	else
	glXMakeCurrent (hGlDC, hGlWindow, hGlRC);
#	endif
#endif
PrintLog ("Destroying cameras\n");
for (int i = 0; i < m_nCameras; i++)
	m_cameras [i].Destroy ();
m_faceCameras.Destroy ();
m_objectCameras.Destroy ();
m_nCameras = 0;
}

//------------------------------------------------------------------------------

int CCameraManager::Render (void)
{
	int		i;
	CObject	*viewerSave = gameData.objs.viewerP;
	time_t	t;
	int		nCamsRendered;
	int		cm = gameStates.render.cockpit.nType;
//	int		frameCap = gameOpts->render.nMaxFPS;
	int		nWaitFrames, nMaxWaitFrames = -1;

if (!gameStates.app.bD2XLevel)
	return 0;
if (!extraGameInfo [0].bUseCameras)
	return 0;
if (IsMultiGame && !(gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bUseCameras))
	return 0;
nCamsRendered = 0;
t = SDL_GetTicks ();
for (i = 0; i < m_nCameras; i++) {
	nWaitFrames = m_cameras [i].Ready (t);
	if (nWaitFrames && (nMaxWaitFrames < nWaitFrames)) {
		nMaxWaitFrames = nWaitFrames;
		m_current = m_cameras + i;
		}
	}
if (m_current) {
	m_current->Reset ();
	nCamsRendered += m_current->Render ();
	}
gameData.objs.viewerP = viewerSave;
//gameOpts->render.nMaxFPS = frameCap;
if (gameStates.render.cockpit.nType != cm) {
	cockpit->Activate (cm);
	return nCamsRendered;
	}
return 0;
}

//------------------------------------------------------------------------------

inline int CCameraManager::GetObjectCamera (int nObject) 
{
if (!(m_cameras && m_objectCameras.Buffer ()) || (nObject < 0) || (nObject >= LEVEL_OBJECTS))
	return -1;
return m_objectCameras [nObject];
}

//------------------------------------------------------------------------------

inline void CCameraManager::SetObjectCamera (int nObject, int i) 
{
if (m_objectCameras.Buffer () && (nObject >= 0) && (nObject < LEVEL_OBJECTS))
	m_objectCameras [nObject] = i;
}

//------------------------------------------------------------------------------

int CCameraManager::GetFaceCamera (int nFace) 
{
return (m_cameras && m_faceCameras.Buffer ()) ? m_faceCameras [nFace] : -1;
}

//------------------------------------------------------------------------------

void CCameraManager::SetFaceCamera (int nFace, int i) 
{
if (m_faceCameras.Buffer ())
	m_faceCameras [nFace] = i;
}

//--------------------------------------------------------------------

CCamera* CCameraManager::Camera (CObject *objP)
{
	short i = GetObjectCamera (objP->Index ());

return (i < 0) ? NULL : m_cameras + i;
}

//--------------------------------------------------------------------

void CCameraManager::Rotate (CObject *objP)
{
	short i = GetObjectCamera (objP->Index ());

if (i >= 0)
	m_cameras [i].Rotate ();
}

//------------------------------------------------------------------------------

void CCameraManager::ReAlign (void)
{
Destroy ();
Create ();
for (int i = 0; i < m_nCameras; i++)
	m_cameras [i].ReAlign ();
}

//------------------------------------------------------------------------------
