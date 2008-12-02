#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "inferno.h"
#include "error.h"
#include "ogl_lib.h"
#include "render.h"
#include "u_mem.h"
#include "gameseg.h"

#define CAMERA_READPIXELS	0
#define TELEPORT_CAMERAS	1

#define CAM_IDX(_pc)	((_pc) - cameraManager.Cameras ())

CCameraManager cameraManager;

//------------------------------------------------------------------------------

int CCamera::CreateBuffer (void)
{
#if RENDER2TEXTURE == 1
if (!OglCreatePBuffer (&m_info.pb, m_info.texture.Width (), m_info.texture.Height (), 0))
	return 0;
m_info.glTexId = m_info.pb.texId;
#elif RENDER2TEXTURE == 2
if (!m_info.fbo.Create (m_info.texture.Width (), m_info.texture.Height (), m_info.bShadowMap))
	return 0;
m_info.glTexId = m_info.fbo.RenderBuffer ();
#endif
char szName [20];
sprintf (szName, "CAM#%04d", m_info.nId);
m_info.texture.SetName (szName);
return 1;
}

//------------------------------------------------------------------------------

int CCamera::HaveBuffer (int bCheckTexture)
{
if (bCheckTexture)
#if RENDER2TEXTURE == 1
	return m_info.texture.glTexture ? OglPBufferAvail (&m_info.texture.glTexture->pbuffer) : -1;
return OglPHaveBuffer (&m_info.pb);
#elif RENDER2TEXTURE == 2
	return m_info.texture.TexInfo () ? m_info.texture.TexInfo ()->fbo.Available () : -1;
return m_info.fbo.Available ();
#endif
}

//------------------------------------------------------------------------------

int CCamera::HaveTexture (void)
{
return m_info.texture.TexInfo () && ((int) m_info.texture.TexInfo ()->handle > 0);
}

//------------------------------------------------------------------------------

int CCamera::EnableBuffer (void)
{
#if RENDER2TEXTURE == 1
if (!OglEnablePBuffer (&m_info.pb))
	return 0;
#elif RENDER2TEXTURE == 2
if (!m_info.fbo.Enable ())
	return 0;
#endif
OglFreeBmTexture (&m_info.texture);
return 1;
}

//------------------------------------------------------------------------------

int CCamera::DisableBuffer (void)
{
#if RENDER2TEXTURE == 1
if (!OglDisablePBuffer (&m_info.pb))
	return 0;
if (!m_info.texture.glTexture)
	LoadTexture (&m_info.texture, 0, -1, 0, &m_info.pb);
#elif RENDER2TEXTURE == 2
if (!m_info.fbo.Disable ())
	return 0;
if (!m_info.texture.TexInfo ())
	m_info.texture.LoadTexture (0, -1, 0, &m_info.fbo);
#endif
return 1;
}

//------------------------------------------------------------------------------

int CCamera::BindBuffer (void)
{
if ((HaveBuffer (0) != 1) && !CreateBuffer ())
	return 0;
OGL_BINDTEX (m_info.glTexId);
#if RENDER2TEXTURE == 1
#	ifdef _WIN32
return m_info.pbo.Bind ();
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
if (!m_info.pb.bBound)
	return 1;
#endif
OglFreeBmTexture (&m_info.texture);
#if RENDER2TEXTURE == 1
m_info.pb.bBound = 0;
#endif
return 1;
}

//------------------------------------------------------------------------------

int CCamera::DestroyBuffer (void)
{
if (!HaveBuffer (0))
	return 0;
#if RENDER2TEXTURE == 1
OglDestroyPBuffer (&m_info.pb);
#elif RENDER2TEXTURE == 2
m_info.fbo.Destroy ();
#endif
return 1;
}

//------------------------------------------------------------------------------

void CCamera::Init (void)
{
memset (&m_info, 0, sizeof (m_info)); 
}

//------------------------------------------------------------------------------

int CCamera::Create (short nId, short srcSeg, short srcSide, short tgtSeg, short tgtSide, 
							tObject *objP, int bShadowMap, int bTeleport)
{
	vmsAngVec	a;
	int			h, i;
#if 0
	short			sideVerts [4];
	vmsVector	*pv;
#endif

Init ();
m_info.nId = nId;
m_info.bShadowMap = bShadowMap;
h = CCanvas::Current ()->Width () / (2 - gameOpts->render.cameras.bHires);
for (i = 1; i < h; i <<= 1)
	;
m_info.texture.SetWidth (i);
h = CCanvas::Current ()->Bitmap ().Height () / (2 - gameOpts->render.cameras.bHires);
for (i = 1; i < h; i <<= 1)
	;
m_info.texture.SetHeight (i);
m_info.texture.SetRowSize (max (CCanvas::Current ()->Width (), m_info.texture.Width ()));
m_info.texture.SetBPP (4);
#if RENDER2TEXTURE
if (!CreateBuffer ()) 
#endif
{
#if CAMERA_READPIXELS
	if (!(m_info.texture.TexBuf () = (char *) D2_ALLOC (m_info.texture.Width () * m_info.texture.Height () * 4)))
		return 0;
	if (gameOpts->render.cameras.bFitToWall || m_info.bTeleport)
		m_info.screenBuf = m_info.texture.TexBuf ();
	else {
		m_info.screenBuf = D2_ALLOC (CCanvas::Current ()->Width () * CCanvas::Current ()->Bitmap ().Height () * 4);
		if (!m_info.screenBuf) {
			gameOpts->render.cameras.bFitToWall = 1;
			m_info.screenBuf = m_info.texture.TexBuf ();
			}
		}
	memset (m_info.texture.TexBuf (), 0, m_info.texture.Width () * m_info.texture.Height () * 4);
#else
	return 0;
#endif
	}
if (objP) {
	m_info.objP = objP;
	m_info.orient = objP->info.position.mOrient;
	m_info.curAngle =
	m_info.curDelta = 0;
	m_info.t0 = 0;
	cameraManager.SetObjectCamera (OBJ_IDX (objP), nId);
	}
else {
	m_info.objP = &m_info.obj;
	if (bTeleport) {
		vmsVector n = *gameData.segs.segments [srcSeg].sides [srcSide].normals;
		/*
		n[X] = -n[X];
		n[Y] = -n[Y];
		*/
		n.Neg();
		a = n.ToAnglesVec();
		}
	else
		a = gameData.segs.segments [srcSeg].sides [srcSide].normals [0].ToAnglesVec();
	m_info.obj.info.position.mOrient = vmsMatrix::Create(a);
#if 1
	if (bTeleport)
		COMPUTE_SEGMENT_CENTER_I (&m_info.obj.info.position.vPos, srcSeg);
	else
		COMPUTE_SIDE_CENTER_I (&m_info.obj.info.position.vPos, srcSeg, srcSide);
#else
	GetSideVertIndex (sideVerts, srcSeg, srcSide);
	for (i = 0; i < 4; i++) {
		pv = gameData.segs.vertices + sideVerts [i];
		m_info.obj.info.position.p.vPos.x += pv->x;
		m_info.obj.info.position.p.vPos.y += pv->y;
		m_info.obj.info.position.p.vPos.z += pv->z;
		}
	m_info.obj.info.position.p.vPos.x /= 4;
	m_info.obj.info.position.p.vPos.y /= 4;
	m_info.obj.info.position.p.vPos.z /= 4;
#endif
	m_info.obj.info.nSegment = srcSeg;
	m_info.bMirror = (tgtSeg == srcSeg) && (tgtSide == srcSide);
	}
//m_info.obj.nSide = srcSide;
m_info.nSegment = tgtSeg;
m_info.nSide = tgtSide;
m_info.bTeleport = (char) bTeleport;
return 1;
}

//------------------------------------------------------------------------------

void CCamera::Destroy (void)
{
OglFreeBmTexture (&m_info.texture);
OglDeleteTextures (1, &m_info.glTexId);
if (m_info.screenBuf && (m_info.screenBuf != (char *) m_info.texture.TexBuf ()))
	D2_FREE (m_info.screenBuf);
if (m_info.texture.TexBuf ()) {
	m_info.texture.DestroyTexBuf ();
	}
#if RENDER2TEXTURE
if (gameStates.ogl.bRender2TextureOk)
	DestroyBuffer ();
#endif
}

//------------------------------------------------------------------------------

void CCamera::GetUVL (grsFace *faceP, tUVL *uvlP, tTexCoord2f *texCoordP, fVector3 *vertexP)
{
	int i2, i3, nType = 0, nScale = 2 - gameOpts->render.cameras.bHires;

if (gameStates.render.bTriangleMesh) {
	if ((nType = faceP->nType == SIDE_IS_TRI_13)) {
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
if (m_info.bHaveUVL) {
	if (uvlP)
		memcpy (uvlP, m_info.uvlList, sizeof (m_info.uvlList));
	}
else 
#endif
	{
		fix	i;
		float	duImage, dvImage, duFace, dvFace, du, dv,aFace, aImage;
		int	xFlip, yFlip, rotLeft, rotRight;
#if RENDER2TEXTURE
		int	bHaveBuffer = HaveBuffer (1) == 1;
		int	bFitToWall = m_info.bTeleport || gameOpts->render.cameras.bFitToWall;
#endif

	if (uvlP) {
		rotLeft = (uvlP [1].u > uvlP [0].u);
		rotRight = (uvlP [1].u < uvlP [0].u);
		if (rotLeft) {
			yFlip = (uvlP [1].u < uvlP [2].u);
			xFlip = (uvlP [3].v > uvlP [1].v);
			}
		else if (rotRight) {
			yFlip = (uvlP [3].u < uvlP [0].u);
			xFlip = (uvlP [1].v > uvlP [3].v);
			}
		else {
			xFlip = (uvlP [2].u < uvlP [0].u);
			yFlip = (uvlP [1].v > uvlP [0].v);
			}
		dvFace = X2F (uvlP [1].v - uvlP [0].v);
		duFace = X2F (uvlP [2].u - uvlP [0].u);
		}
	else {
		rotLeft = (texCoordP [1].v.u < texCoordP [0].v.u);
		rotRight = (texCoordP [1].v.u > texCoordP [0].v.u);
		if (rotLeft) {
			yFlip = (texCoordP [1].v.u > texCoordP [i2].v.u);
			xFlip = (texCoordP [i3].v.v < texCoordP [1].v.v);
			}
		else if (rotRight) {
			yFlip = (texCoordP [i3].v.u > texCoordP [0].v.u);
			xFlip = (texCoordP [1].v.v < texCoordP [i3].v.v);
			}
		else {
			xFlip = (texCoordP [i2].v.u < texCoordP [0].v.u);
			yFlip = (texCoordP [1].v.v > texCoordP [0].v.v);
			}
		dvFace = (float) fabs (texCoordP [1].v.v - texCoordP [0].v.v);
		duFace = (float) fabs (texCoordP [i2].v.u - texCoordP [0].v.u);
		}
	du = dv = 0;
	if (bHaveBuffer) {
		duImage = (float) CCanvas::Current ()->Width () / (float) m_info.texture.Width () / nScale;
		dvImage = (float) CCanvas::Current ()->Bitmap ().Height () / (float) m_info.texture.Height () / nScale;
		if (!bFitToWall && RENDERPATH) {
			aImage = (float) CCanvas::Current ()->Bitmap ().Height () / (float) CCanvas::Current ()->Width ();
			if (vertexP)
				aFace = fVector::Dist(*(fVector *)vertexP, *(fVector *)(vertexP + 1)) / 
				        fVector::Dist(*(fVector *)(vertexP + 1), *(fVector *)(vertexP + i2));
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
	if (m_info.bMirror)
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
			uvlP [i].l = F1_0;
		if (rotRight) {
			for (i = 1; i < 5; i++) {
				m_info.uvlList [i - 1] = uvlP [i % 4];
				}
			}
		else if (rotRight) {
			for (i = 0; i < 4; i++) {
				m_info.uvlList [i] = uvlP [(i + 1) % 4];
				}
			}
		else
			memcpy (m_info.uvlList, uvlP, sizeof (m_info.uvlList));
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
		memcpy (m_info.texCoord, texCoord, sizeof (m_info.texCoord));
		}
	m_info.bHaveUVL = 1;
	}
}

//------------------------------------------------------------------------------

int CCamera::Ready (time_t t)
{
m_info.nWaitFrames++;
if (!m_info.bVisible)
	return 0;
if (m_info.bTeleport && !EGI_FLAG (bTeleporterCams, 0, 1, 0))
	return 0;
if (m_info.objP && (m_info.objP->info.nFlags & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED)))
	return 0;
if (gameOpts->render.cameras.nFPS && !m_info.bTimedOut) {
	if (t - m_info.nTimeout < 1000 / gameOpts->render.cameras.nFPS)
		return 0;
	m_info.bTimedOut = 1;
	}
return m_info.nWaitFrames;
}

//------------------------------------------------------------------------------

void CCamera::Reset (void)
{
m_info.nWaitFrames = 0;
m_info.bTimedOut = 0;
m_info.nTimeout = gameStates.app.nSDLTicks; //SDL_GetTicks ();
m_info.bVisible = 0;
}

//------------------------------------------------------------------------------

int CCamera::Render (void)
{
gameStates.render.cameras.bActive = 1;
if (gameStates.render.cockpit.nMode != CM_FULL_SCREEN)
	SelectCockpit (CM_FULL_SCREEN);
gameData.objs.viewerP = m_info.objP;
gameOpts->render.nMaxFPS = 1;
#if RENDER2TEXTURE
if (ReleaseBuffer () && EnableBuffer ()) {
	RenderFrame (0, 0);
	m_info.bValid = 1;
	DisableBuffer ();
	}
else 
#	if CAMERA_READPIXELS
if (m_info.texture.TexBuf ()) 
#	endif
#endif
	{
	RenderFrame (0, 0);
	m_info.bValid = 1;
	OglFreeBmTexture (&m_info.texture);
#if CAMERA_READPIXELS
	memset (m_info.texture.TexBuf (), 0, m_info.texture.Width () * m_info.texture.Height () * 4);
	glDisable (GL_TEXTURE_2D);
#endif
	glReadBuffer (GL_BACK);
	if (gameOpts->render.cameras.bFitToWall || m_info.bTeleport) {
#if CAMERA_READPIXELS == 0
		m_info.texture.LoadTexture (0, -1, 0, NULL);
		glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 
			(CCanvas::Current ()->Width () - m_info.texture.Width ()) / 2, 
			(CCanvas::Current ()->Bitmap ().Height () - m_info.texture.Height ()) / 2, 
			m_info.texture.Width (), m_info.texture.Height (), 0);
#else
		glReadPixels (
			(CCanvas::Current ()->Width () - m_info.texture.Width ()) / 2, 
			(CCanvas::Current ()->Bitmap ().Height () - m_info.texture.Height ()) / 2, 
			m_info.texture.Width (), m_info.texture.Height (), 
			GL_RGBA, GL_UNSIGNED_BYTE, m_info.texture.TexBuf ());
		LoadTexture (&m_info.texture, 0, -1, NULL);
#endif
		}
	else {
#if CAMERA_READPIXELS == 0
			m_info.texture.LoadTexture (0, -1, 0, NULL);
			glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 
				-(m_info.texture.Width () - CCanvas::Current ()->Width ()) / 2,
				-(m_info.texture.Height () - CCanvas::Current ()->Bitmap ().Height ()) / 2, 
				m_info.texture.Width (), m_info.texture.Height (), 0);
#else
		char	*pSrc, *pDest;
		int	dxBuf = (m_info.texture.Width () - CCanvas::Current ()->Width ()) / 2;
		int	dyBuf = (m_info.texture.Height () - CCanvas::Current ()->Bitmap ().Height ()) / 2;
		int	wSrc = CCanvas::Current ()->Width () * 4;
		int	wDest = m_info.texture.Width () * 4;

		if (CCanvas::Current ()->Width () == m_info.texture.Width ()) {
			glReadPixels (
				0, 0, CCanvas::Current ()->Width (), CCanvas::Current ()->Bitmap ().Height (),
				GL_RGBA, GL_UNSIGNED_BYTE, m_info.texture.TexBuf () + dyBuf * m_info.texture.Width () * 4);
			}
		else {
			glReadPixels (
				0, 0, CCanvas::Current ()->Width (), CCanvas::Current ()->Bitmap ().Height (),
				GL_RGBA, GL_UNSIGNED_BYTE, m_info.screenBuf);
			pSrc = m_info.screenBuf;
			pDest = m_info.texture.TexBuf () + (dyBuf - 1) * wDest + dxBuf * 4;
#	ifndef _WIN32
			for (dyBuf = CCanvas::Current ()->Bitmap ().Height (); dyBuf; dyBuf--, pSrc += wSrc, pDest += wDest)
				memcpy (pDest, pSrc, wSrc);
#	else
			dxBuf = m_info.texture.Width () - CCanvas::Current ()->Width ();
			dyBuf = CCanvas::Current ()->Bitmap ().Height ();
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
			LoadTexture (&m_info.texture, 0, -1);
			}
#endif
		}
	}
gameStates.render.cameras.bActive = 0;
return m_info.bValid;
}

//--------------------------------------------------------------------

void CCamera::Rotate (void)
{

#define	DEG90		 (F1_0 / 4)
#define	DEG45		 (F1_0 / 8)
#define	DEG1		 (F1_0 / (4 * 90))

if (!m_info.objP)
	return;

	fixang	curAngle = m_info.curAngle;
	fixang	curDelta = m_info.curDelta;

#if 1
	time_t	t0 = m_info.t0;
	time_t	t = gameStates.app.nSDLTicks;

if ((t0 < 0) || (t - t0 >= 1000 / 90))
#endif
	if (m_info.objP->cType.aiInfo.behavior == AIB_NORMAL) {
		vmsAngVec	a;
		vmsMatrix	r;

		int	h = abs (curDelta);
		int	d = DEG1 / (gameOpts->render.cameras.nSpeed / 1000);
		int	s = h ? curDelta / h : 1;

	if (h != d)
		curDelta = s * d;
#if 1
	m_info.objP->mType.physInfo.brakes = (fix) t;
#endif
	if ((curAngle >= DEG45) || (curAngle <= -DEG45)) {
		if (curAngle * s - DEG45 >= curDelta * s)
			curAngle = s * DEG45 + curDelta - s;
		curDelta = -curDelta;
		}

	curAngle += curDelta;
	a [HA] = curAngle;
	a [BA] = a [PA] = 0;
	r = vmsMatrix::Create (a);
	// TODO MM
	m_info.objP->info.position.mOrient = m_info.orient * r;
	m_info.curAngle = curAngle;
	m_info.curDelta = curDelta;
	}
}

//------------------------------------------------------------------------------

CCameraManager::~CCameraManager ()
{
D2_FREE (m_faceCameras);
D2_FREE (m_objectCameras);
Destroy ();
}

//------------------------------------------------------------------------------

int CCameraManager::Create (void)
{
	int		h, i, j, k, r;
	ubyte		t;
	tWall		*wallP;
	tObject	*objP;
	tTrigger	*triggerP;

if (!gameStates.app.bD2XLevel)
	return 0;
PrintLog ("   creating cameras\n");
if (!m_faceCameras) {
	GETMEM (char, m_faceCameras, 2 * MAX_SEGMENTS * 6, 0);
	if (!m_faceCameras)
		return 0;
	}
if (!m_objectCameras) {
	GETMEM (ushort, m_objectCameras, MAX_OBJECTS, 0);
	if (!m_objectCameras)
		return 0;
	}
memset (m_faceCameras, 0xFF, MAX_SEGMENTS * 6 * sizeof (*m_faceCameras));
memset (m_objectCameras, 0xFF, MAX_OBJECTS * sizeof (*m_objectCameras));
for (i = 0, wallP = gameData.walls.walls; (i < gameData.walls.nWalls) && (m_nCameras < MAX_CAMERAS); i++, wallP++) {
	t = wallP->nTrigger;
	if (t >= gameData.trigs.nTriggers)
		continue;
	triggerP = gameData.trigs.triggers + t;
	if (triggerP->nType == TT_CAMERA) {
		for (j = 0; j < triggerP->nLinks; j++)
			if (m_cameras [m_nCameras].Create (m_nCameras, (short) wallP->nSegment, (short) wallP->nSide, triggerP->nSegment [j], triggerP->nSide [j], NULL, 0, 0))
				SetFaceCamera (triggerP->nSegment [j] * 6 + triggerP->nSide [j], (char) m_nCameras++);
		}
#if TELEPORT_CAMERAS
	else if (/*EGI_FLAG (bTeleporterCams, 0, 0) &&*/ (triggerP->nType == TT_TELEPORT)) {
		if (m_cameras [m_nCameras].Create (m_nCameras, triggerP->nSegment [0], triggerP->nSide [0], (short) wallP->nSegment, (short) wallP->nSide, NULL, 0, 1))
			SetFaceCamera (wallP->nSegment * 6 + wallP->nSide, (char) m_nCameras++);
		}
#endif
	}
FORALL_OBJS (objP, i) {
	r = j = gameData.trigs.firstObjTrigger [OBJ_IDX (objP)];
#if DBG
	if (j >= 0)
		j = j;
#endif
	for (h = sizeofa (gameData.trigs.objTriggerRefs); (j >= 0) && h && (m_nCameras < MAX_CAMERAS); h--) {
		triggerP = gameData.trigs.objTriggers + j;
		if (triggerP->nType == TT_CAMERA) {
			for (k = 0; k < triggerP->nLinks; k++)
				if (m_cameras [m_nCameras].Create (m_nCameras, -1, -1, triggerP->nSegment [k], triggerP->nSide [k], objP, 0, 0))
					SetFaceCamera (triggerP->nSegment [k] * 6 + triggerP->nSide [k], (char) m_nCameras++);
			}
		if (r == (j = gameData.trigs.objTriggerRefs [j].next))
			break;
		}
	}
return m_nCameras;
}

//------------------------------------------------------------------------------

void CCameraManager::Destroy (void)
{
#if RENDER2TEXTURE == 1
if (gameStates.ogl.bRender2TextureOk)
#	ifdef _WIN32
	wglMakeContextCurrentARB (hGlDC, hGlDC, hGlRC);
#	else
	glXMakeCurrent (hGlDC, hGlWindow, hGlRC);
#	endif
#endif
PrintLog ("Destroying cameras\n");
for (int i = 0; i < m_nCameras; i++)
	m_cameras [i].Destroy ();
if (m_faceCameras)
	memset (m_faceCameras, 0xFF, MAX_SEGMENTS * 6 * sizeof (*m_faceCameras));
m_nCameras = 0;
}

//------------------------------------------------------------------------------

int CCameraManager::Render (void)
{
	int		i;
	CCamera	*cameraP = NULL;
	tObject	*viewerSave = gameData.objs.viewerP;
	time_t	t;
	int		nCamsRendered;
	int		cm = gameStates.render.cockpit.nMode;
	int		frameCap = gameOpts->render.nMaxFPS;
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
	if (nMaxWaitFrames < nWaitFrames) {
		nMaxWaitFrames = nWaitFrames;
		cameraP = m_cameras + i;
		}
	}
if (cameraP) {
	cameraP->Reset ();
	nCamsRendered += cameraP->Render ();
	}
gameData.objs.viewerP = viewerSave;
gameOpts->render.nMaxFPS = frameCap;
if (gameStates.render.cockpit.nMode != cm) {
	SelectCockpit (cm);
	return nCamsRendered;
	}
return 0;
}

//------------------------------------------------------------------------------

inline int CCameraManager::GetObjectCamera (int nObject) 
{
if (!(m_cameras && m_objectCameras) || (nObject < 0) || (nObject >= MAX_OBJECTS))
	return -1;
return m_objectCameras [nObject];
}

//------------------------------------------------------------------------------

inline void CCameraManager::SetObjectCamera (int nObject, int i) 
{
if (m_objectCameras && (nObject >= 0) && (nObject < MAX_OBJECTS))
	m_objectCameras [nObject] = i;
}

//------------------------------------------------------------------------------

int CCameraManager::GetFaceCamera (int nFace) 
{
return (m_cameras && m_faceCameras) ? m_faceCameras [nFace] : -1;
}

//------------------------------------------------------------------------------

void CCameraManager::SetFaceCamera (int nFace, int i) 
{
if (m_faceCameras)
	m_faceCameras [nFace] = i;
}

//--------------------------------------------------------------------

CCamera* CCameraManager::Camera (tObject *objP)
{
	short i = GetObjectCamera (OBJ_IDX (objP));

return (i < 0) ? NULL : m_cameras + i;
}

//--------------------------------------------------------------------

void CCameraManager::Rotate (tObject *objP)
{
	short i = GetObjectCamera (OBJ_IDX (objP));

if (i >= 0)
	m_cameras [i].Rotate ();
}

//------------------------------------------------------------------------------
