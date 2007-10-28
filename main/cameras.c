#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "inferno.h"
#include "segment.h"
#include "error.h"
#include "bm.h"
#include "render.h"
#include "game.h"
#include "object.h"
#include "textures.h"
#include "piggy.h"
#include "switch.h"
#include "wall.h"
#include "gamesave.h"
#include "hudmsg.h"
#include "network.h"
#include "cameras.h"
#include "u_mem.h"
#include "gameseg.h"

#define CAMERA_READPIXELS	0
#define TELEPORT_CAMERAS	1

#define CAM_IDX(_pc)	((_pc) - gameData.cameras.cameras)

//------------------------------------------------------------------------------

#if RENDER2TEXTURE

int OglCreateCamBuf (tCamera *pc)
{
#if RENDER2TEXTURE == 1
if (!OglCreatePBuffer (&pc->pb, pc->texBuf.bmProps.w, pc->texBuf.bmProps.h, 0))
	return 0;
pc->glTexId = pc->pb.texId;
#elif RENDER2TEXTURE == 2
if (!OglCreateFBuffer (&pc->fb, pc->texBuf.bmProps.w, pc->texBuf.bmProps.h, pc->bShadowMap))
	return 0;
pc->glTexId = pc->fb.texId;
#endif
sprintf (pc->texBuf.szName, "CAM#%04d", CAM_IDX (pc));
return 1;
}

//------------------------------------------------------------------------------

int OglCamBufAvail (tCamera *pc, int bCheckTexture)
{
if (bCheckTexture)
#if RENDER2TEXTURE == 1
	return pc->texBuf.glTexture ? OglPBufferAvail (&pc->texBuf.glTexture->pbuffer) : -1;
return OglPBufferAvail (&pc->pb);
#elif RENDER2TEXTURE == 2
	return pc->texBuf.glTexture ? OglFBufferAvail (&pc->texBuf.glTexture->fbuffer) : -1;
return OglFBufferAvail (&pc->fb);
#endif
}

//------------------------------------------------------------------------------

int OglEnableCamBuf (tCamera *pc)
{
#if RENDER2TEXTURE == 1
if (!OglEnablePBuffer (&pc->pb))
	return 0;
#elif RENDER2TEXTURE == 2
if (!OglEnableFBuffer (&pc->fb))
	return 0;
#endif
OglFreeBmTexture (&pc->texBuf);
return 1;
}

//------------------------------------------------------------------------------

int OglDisableCamBuf (tCamera *pc)
{
#if RENDER2TEXTURE == 1
if (!OglDisablePBuffer (&pc->pb))
	return 0;
if (!pc->texBuf.glTexture)
	OglLoadBmTextureM (&pc->texBuf, 0, -1, 0, &pc->pb);
#elif RENDER2TEXTURE == 2
if (!OglDisableFBuffer (&pc->fb))
	return 0;
if (!pc->texBuf.glTexture)
	OglLoadBmTextureM (&pc->texBuf, 0, -1, 0, &pc->fb);
#endif
return 1;
}

//------------------------------------------------------------------------------

int OglBindCamBuf (tCamera *pc)
{
if ((OglCamBufAvail (pc, 0) != 1) && !OglCreateCamBuf (pc))
	return 0;
OGL_BINDTEX (pc->glTexId);
#if RENDER2TEXTURE == 1
#	ifdef _WIN32
return pc->pb.bBound = wglBindTexImageARB (pc->pb.hBuf, WGL_FRONT_LEFT_ARB);
#	endif
#endif
return 1;
}

//------------------------------------------------------------------------------

int OglReleaseCamBuf (tCamera *pc)
{
if (OglCamBufAvail (pc, 0) != 1)
	return 0;
#if RENDER2TEXTURE == 1
if (!pc->pb.bBound)
	return 1;
#endif
OglFreeBmTexture (&pc->texBuf);
#if RENDER2TEXTURE == 1
pc->pb.bBound = 0;
#endif
return 1;
}

//------------------------------------------------------------------------------

int OglDestroyCamBuf (tCamera *pc)
{
if (!OglCamBufAvail (pc, 0))
	return 0;
#if RENDER2TEXTURE == 1
OglDestroyPBuffer (&pc->pb);
#elif RENDER2TEXTURE == 2
OglDestroyFBuffer (&pc->fb);
#endif
return 1;
}

#endif

//------------------------------------------------------------------------------

int CreateCamera (tCamera *pc, short srcSeg, short srcSide, short tgtSeg, short tgtSide, 
						tObject *objP, int bShadowMap, int bTeleport)
{
	vmsAngVec	a;
	int			h, i;
#if 0
	short			sideVerts [4];
	vmsVector	*pv;
#endif

memset (pc, 0, sizeof (tCamera));
pc->bShadowMap = bShadowMap;
#if 0
if (gameOpts->render.cameras.bFitToWall || bTeleport) {
	h = min (grdCurCanv->cvBitmap.bmProps.w, grdCurCanv->cvBitmap.bmProps.h);
	for (i = 1; i < h; i *= 2)
		;
	if (i > h)
		i /= 2;
	pc->texBuf.bmProps.w =
	pc->texBuf.bmProps.h = i;
	}
else 
#endif
	{
	h = grdCurCanv->cvBitmap.bmProps.w;
	for (i = 1; i < h; i *= 2)
		;
	pc->texBuf.bmProps.w = i;
	h = grdCurCanv->cvBitmap.bmProps.h;
	for (i = 1; i < h; i *= 2)
		;
	pc->texBuf.bmProps.h = i;
	}
pc->texBuf.bmProps.rowSize = grdCurCanv->cvBitmap.bmProps.w;
pc->texBuf.bmBPP = 4;
#if RENDER2TEXTURE
if (!OglCreateCamBuf (pc)) 
#endif
{
#if CAMERA_READPIXELS
	if (!(pc->texBuf.bmTexBuf = (char *) D2_ALLOC (pc->texBuf.bmProps.w * pc->texBuf.bmProps.h * 4)))
		return 0;
	if (gameOpts->render.cameras.bFitToWall || pc->bTeleport)
		pc->screenBuf = pc->texBuf.bmTexBuf;
	else {
		pc->screenBuf = D2_ALLOC (grdCurCanv->cvBitmap.bmProps.w * grdCurCanv->cvBitmap.bmProps.h * 4);
		if (!pc->screenBuf) {
			gameOpts->render.cameras.bFitToWall = 1;
			pc->screenBuf = pc->texBuf.bmTexBuf;
			}
		}	
	memset (pc->texBuf.bmTexBuf, 0, pc->texBuf.bmProps.w * pc->texBuf.bmProps.h * 4);
#else
	return 0;
#endif
	}
if (objP) {
	pc->objP = objP;
	pc->orient = objP->position.mOrient;
	pc->curAngle =
	pc->curDelta = 0;
	pc->t0 = 0;
	gameData.objs.cameraRef [OBJ_IDX (objP)] = gameData.cameras.nCameras;
	}
else {
	pc->objP = &pc->obj;
	if (bTeleport) {
		vmsVector n = *gameData.segs.segments [srcSeg].sides [srcSide].normals;
		n.p.x = -n.p.x;
		n.p.y = -n.p.y;
		n.p.z = -n.p.z;
		VmExtractAnglesVector (&a, &n);
		}
	else
		VmExtractAnglesVector (&a, gameData.segs.segments [srcSeg].sides [srcSide].normals);
	VmAngles2Matrix (&pc->obj.position.mOrient, &a);
#if 1
	if (bTeleport)
		COMPUTE_SEGMENT_CENTER_I (&pc->obj.position.vPos, srcSeg);
	else
		COMPUTE_SIDE_CENTER_I (&pc->obj.position.vPos, srcSeg, srcSide);
#else
	GetSideVertIndex (sideVerts, srcSeg, srcSide);
	for (i = 0; i < 4; i++) {
		pv = gameData.segs.vertices + sideVerts [i];
		pc->obj.position.p.vPos.x += pv->x;
		pc->obj.position.p.vPos.y += pv->y;
		pc->obj.position.p.vPos.z += pv->z;
		}
	pc->obj.position.p.vPos.x /= 4;
	pc->obj.position.p.vPos.y /= 4;
	pc->obj.position.p.vPos.z /= 4;
#endif
	pc->obj.nSegment = srcSeg;
	}
//pc->obj.nSide = srcSide;
pc->segNum = tgtSeg;
pc->sideNum = tgtSide;
pc->bTeleport = (char) bTeleport;
return 1;
}

//------------------------------------------------------------------------------

int CreateCameras (void)
{
	int		h, i, j, k, r;
	ubyte		t;
	tWall		*wallP;
	tObject	*objP;
	tTrigger	*triggerP;

if (!gameStates.app.bD2XLevel)
	return 0;

memset (gameData.cameras.nSides, 0xFF, MAX_SEGMENTS * 6 * sizeof (*gameData.cameras.nSides));
for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++) {
	t = wallP->nTrigger;
	if (t >= gameData.trigs.nTriggers)
		continue;
	triggerP = gameData.trigs.triggers + t;
	if (triggerP->nType == TT_CAMERA) {
		for (j = 0; j < triggerP->nLinks; j++)
			if (CreateCamera (gameData.cameras.cameras + gameData.cameras.nCameras, (short) wallP->nSegment, (short) wallP->nSide, triggerP->nSegment [j], triggerP->nSide [j], NULL, 0, 0))
				gameData.cameras.nSides [triggerP->nSegment [j] * 6 + triggerP->nSide [j]] = (char) gameData.cameras.nCameras++;
		}
#if TELEPORT_CAMERAS
	else if (/*EGI_FLAG (bTeleporterCams, 0, 0) &&*/ (triggerP->nType == TT_TELEPORT)) {
		if (CreateCamera (gameData.cameras.cameras + gameData.cameras.nCameras, triggerP->nSegment [0], triggerP->nSide [0], (short) wallP->nSegment, (short) wallP->nSide, NULL, 0, 1))
			gameData.cameras.nSides [wallP->nSegment * 6 + wallP->nSide] = (char) gameData.cameras.nCameras++;
		}
#endif
	}
for (i = 0, objP = gameData.objs.objects; i <= gameData.objs.nLastObject; i++, objP++) {
	r = j = gameData.trigs.firstObjTrigger [i];
	if (j >= 0)
		j = j;
	for (h = sizeofa (gameData.trigs.objTriggerRefs); (j >= 0) && h; h--) {
		triggerP = gameData.trigs.objTriggers + j;
		if (triggerP->nType == TT_CAMERA) {
			for (k = 0; k < triggerP->nLinks; k++)
				if (CreateCamera (gameData.cameras.cameras + gameData.cameras.nCameras, -1, -1, triggerP->nSegment [k], triggerP->nSide [k], objP, 0, 0))
					gameData.cameras.nSides [triggerP->nSegment [k] * 6 + triggerP->nSide [k]] = (char) gameData.cameras.nCameras++;
			}
		if (r == (j = gameData.trigs.objTriggerRefs [j].next))
			break;
		}
	}
return gameData.cameras.nCameras;
}

//------------------------------------------------------------------------------

void DestroyCamera (tCamera *pc)
{
OglFreeBmTexture (&pc->texBuf);
glDeleteTextures (1, &pc->glTexId);
if (pc->screenBuf && (pc->screenBuf != (char *) pc->texBuf.bmTexBuf))
	D2_FREE (pc->screenBuf);
if (pc->texBuf.bmTexBuf) {
	D2_FREE (pc->texBuf.bmTexBuf);
	pc->texBuf.bmTexBuf = NULL;
	}
#if RENDER2TEXTURE
if (bRender2TextureOk)
	OglDestroyCamBuf (pc);
#endif
}

//------------------------------------------------------------------------------

void DestroyCameras (void)
{
	int i;
	tCamera	*pc = gameData.cameras.cameras;

#if RENDER2TEXTURE == 1
if (bRender2TextureOk)
#	ifdef _WIN32
	wglMakeContextCurrentARB (hGlDC, hGlDC, hGlRC);
#	else
	glXMakeCurrent (hGlDC, hGlWindow, hGlRC);
#	endif
#endif
LogErr ("Destroying cameras\n");
for (i = gameData.cameras.nCameras; i; i--, pc++)
	DestroyCamera (pc);
if (gameData.cameras.nSides)
	memset (gameData.cameras.nSides, 0xFF, MAX_SEGMENTS * 6 * sizeof (*gameData.cameras.nSides));
gameData.cameras.nCameras = 0;
}

//------------------------------------------------------------------------------

void GetCameraUVL (tCamera *pc, tUVL *uvlP, tTexCoord2f *texCoordP, fVector3 *vertexP)
{
#ifndef _DEBUG
if (pc->bHaveUVL) {
	if (uvlP)
		memcpy (uvlP, pc->uvlList, sizeof (pc->uvlList));
	}
else 
#endif
	{
		fix	i;
		float	duImage, dvImage, duFace, dvFace, du, dv,aFace, aImage;
		int	xFlip, yFlip, rotLeft, rotRight;
#if RENDER2TEXTURE
		int	bCamBufAvail = OglCamBufAvail (pc, 1) == 1;
		int	bFitToWall = pc->bTeleport || gameOpts->render.cameras.bFitToWall;
#endif

	if (uvlP) {
		xFlip = (uvlP [2].u < uvlP [0].u);
		yFlip = (uvlP [1].v > uvlP [0].v);
		rotLeft = (uvlP [1].u < uvlP [0].u);
		rotRight = (uvlP [1].u > uvlP [0].u);
		dvFace = f2fl (uvlP [1].v - uvlP [0].v);
		duFace = f2fl (uvlP [2].u - uvlP [0].u);
		}
	else {
		xFlip = (texCoordP [2].v.u < texCoordP [0].v.u);
		yFlip = (texCoordP [1].v.v > texCoordP [0].v.v);
		rotLeft = (texCoordP [1].v.u < texCoordP [0].v.u);
		rotRight = (texCoordP [1].v.u > texCoordP [0].v.u);
		dvFace = (float) fabs (texCoordP [1].v.v - texCoordP [0].v.v);
		duFace = (float) fabs (texCoordP [2].v.u - texCoordP [0].v.u);
		}
	du = dv = 0;
	if (bCamBufAvail) {
		duImage = (float) grdCurCanv->cvBitmap.bmProps.w / (float) pc->texBuf.bmProps.w;
		dvImage = (float) grdCurCanv->cvBitmap.bmProps.h / (float) pc->texBuf.bmProps.h;
		if (!bFitToWall) {
			aImage = (float) grdCurCanv->cvBitmap.bmProps.h / (float) grdCurCanv->cvBitmap.bmProps.w;
			if (vertexP)
				aFace = VmVecDistf ((fVector *) vertexP, (fVector *) (vertexP + 1)) / 
						  VmVecDistf ((fVector *) (vertexP + 1), (fVector *) (vertexP + 2));
			else
				aFace = dvFace / duFace;
			dv = (aImage - aFace) / 2.0f;
			duImage -= du / 2;
			dvImage -= dv;
			}
		}
	else {
		duImage = duFace;
		dvImage = dvFace;
		}
	if (uvlP) {
		uvlP [0].v = 
		uvlP [3].v = fl2f (yFlip ? dvImage : dv / 2);
		uvlP [1].v = 
		uvlP [2].v = fl2f (yFlip ? dv / 2 : dvImage);
		uvlP [0].u = 
		uvlP [1].u = fl2f (xFlip ? duImage : du / 2);
		uvlP [2].u = 
		uvlP [3].u = fl2f (xFlip ? du / 2 : duImage);
		for (i = 0; i < 4; i++)
			uvlP [i].l = F1_0;
		memcpy (pc->uvlList, uvlP, sizeof (pc->uvlList));
		}
	else {
		pc->texCoord [0].v.v = 
		pc->texCoord [3].v.v = yFlip ? dvImage : dv;
		pc->texCoord [1].v.v = 
		pc->texCoord [2].v.v = yFlip ? dv : dvImage;
		pc->texCoord [0].v.u = 
		pc->texCoord [1].v.u = xFlip ? duImage : du / 2;
		pc->texCoord [2].v.u = 
		pc->texCoord [3].v.u = xFlip ? du / 2 : duImage;
		}
	pc->bHaveUVL = 1;
	}
}

//------------------------------------------------------------------------------

int RenderCamera (tCamera *pc)
{
gameStates.render.cameras.bActive = 1;
if (gameStates.render.cockpit.nMode != CM_FULL_SCREEN)
	SelectCockpit (CM_FULL_SCREEN);
gameData.objs.viewer = pc->objP;
gameOpts->render.nMaxFPS = 1;
#if RENDER2TEXTURE
if (OglReleaseCamBuf (pc) && OglEnableCamBuf (pc)) {
	RenderFrame (0, 0);
	pc->bValid = 1;
	OglDisableCamBuf (pc);
	}
else 
#	if CAMERA_READPIXELS
if (pc->texBuf.bmTexBuf) 
#	endif
#endif
	{
	RenderFrame (0, 0);
	pc->bValid = 1;
	OglFreeBmTexture (&pc->texBuf);
#if CAMERA_READPIXELS
	memset (pc->texBuf.bmTexBuf, 0, pc->texBuf.bmProps.w * pc->texBuf.bmProps.h * 4);
	glDisable (GL_TEXTURE_2D);
#endif
	glReadBuffer (GL_BACK);
	if (gameOpts->render.cameras.bFitToWall || pc->bTeleport) {
#if CAMERA_READPIXELS == 0
		OglLoadBmTextureM (&pc->texBuf, 0, -1, 0, NULL);
		glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 
			(grdCurCanv->cvBitmap.bmProps.w - pc->texBuf.bmProps.w) / 2, 
			(grdCurCanv->cvBitmap.bmProps.h - pc->texBuf.bmProps.h) / 2, 
			pc->texBuf.bmProps.w, pc->texBuf.bmProps.h, 0);
#else
		glReadPixels (
			(grdCurCanv->cvBitmap.bmProps.w - pc->texBuf.bmProps.w) / 2, 
			(grdCurCanv->cvBitmap.bmProps.h - pc->texBuf.bmProps.h) / 2, 
			pc->texBuf.bmProps.w, pc->texBuf.bmProps.h, 
			GL_RGBA, GL_UNSIGNED_BYTE, pc->texBuf.bmTexBuf);
		OglLoadBmTextureM (&pc->texBuf, 0, -1, NULL);
#endif
		}
	else {
#if CAMERA_READPIXELS == 0
			OglLoadBmTextureM (&pc->texBuf, 0, -1, 0, NULL);
			glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 
				-(pc->texBuf.bmProps.w - grdCurCanv->cvBitmap.bmProps.w) / 2,
				-(pc->texBuf.bmProps.h - grdCurCanv->cvBitmap.bmProps.h) / 2, 
				pc->texBuf.bmProps.w, pc->texBuf.bmProps.h, 0);
#else
		char	*pSrc, *pDest;
		int	dxBuf = (pc->texBuf.bmProps.w - grdCurCanv->cvBitmap.bmProps.w) / 2;
		int	dyBuf = (pc->texBuf.bmProps.h - grdCurCanv->cvBitmap.bmProps.h) / 2;
		int	wSrc = grdCurCanv->cvBitmap.bmProps.w * 4;
		int	wDest = pc->texBuf.bmProps.w * 4;

		if (grdCurCanv->cvBitmap.bmProps.w == pc->texBuf.bmProps.w) {
			glReadPixels (
				0, 0, grdCurCanv->cvBitmap.bmProps.w, grdCurCanv->cvBitmap.bmProps.h,
				GL_RGBA, GL_UNSIGNED_BYTE, pc->texBuf.bmTexBuf + dyBuf * pc->texBuf.bmProps.w * 4);
			}
		else {
			glReadPixels (
				0, 0, grdCurCanv->cvBitmap.bmProps.w, grdCurCanv->cvBitmap.bmProps.h,
				GL_RGBA, GL_UNSIGNED_BYTE, pc->screenBuf);
			pSrc = pc->screenBuf;
			pDest = pc->texBuf.bmTexBuf + (dyBuf - 1) * wDest + dxBuf * 4;
#	ifndef _WIN32
			for (dyBuf = grdCurCanv->cvBitmap.bmProps.h; dyBuf; dyBuf--, pSrc += wSrc, pDest += wDest)
				memcpy (pDest, pSrc, wSrc);
#	else
			dxBuf = pc->texBuf.bmProps.w - grdCurCanv->cvBitmap.bmProps.w;
			dyBuf = grdCurCanv->cvBitmap.bmProps.h;
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
			OglLoadBmTextureM (&pc->texBuf, 0, -1);
			}
#endif
		}
	}
gameStates.render.cameras.bActive = 0;
return pc->bValid;
}

//------------------------------------------------------------------------------

int RenderCameras (void)
{
	int		i;
	tCamera	*pc = gameData.cameras.cameras, *pCurCam = NULL;
	tObject	*viewerSave = gameData.objs.viewer;
	time_t	t;
	int		nCamsRendered;
	int		cm = gameStates.render.cockpit.nMode;
	int		frameCap = gameOpts->render.nMaxFPS;
	int		nMaxWaitFrames = -1;
	
if (!gameStates.app.bD2XLevel)
	return 0;
if (!extraGameInfo [0].bUseCameras)
	return 0;
if (IsMultiGame && !(gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bUseCameras))
	return 0;
nCamsRendered = 0;
#if 1 //render only one camera per frame
t = SDL_GetTicks ();
for (i = 0; i < gameData.cameras.nCameras; i++, pc++) {
	pc->nWaitFrames++;
	if (!pc->bVisible)
		continue;
	if (pc->bTeleport && !EGI_FLAG (bTeleporterCams, 0, 1, 0))
		continue;
	if (pc->objP && (pc->objP->flags & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED)))
		continue;
	if (gameOpts->render.cameras.nFPS && !pc->bTimedOut) {
		if (t - pc->nTimeout < 1000 / gameOpts->render.cameras.nFPS)
			continue;
		pc->bTimedOut = 1;
		}
	if (nMaxWaitFrames < pc->nWaitFrames) {
		nMaxWaitFrames = pc->nWaitFrames;
		pCurCam = pc;
		}
	}
if ((pc = pCurCam)) {
	pc->nWaitFrames = 0;
	pc->bTimedOut = 0;
	pc->nTimeout = gameStates.app.nSDLTicks; //SDL_GetTicks ();
#else
t = gameStates.app.nSDLTicks; //SDL_GetTicks ();
for (i = 0; i < gameData.cameras.nCameras; i++, pc++) {
	if (!pc->bVisible)
		continue;
	if (gameOpts->render.cameras.nFPS) {
		pc->bTimedOut = 0;
		if (t - pc->nTimeout < 1000 / gameOpts->render.cameras.nFPS)
			continue;
		pc->nTimeout = t;
		}
	pc->bTimedOut = 1;
#endif
	pc->bVisible = 0;
	nCamsRendered += RenderCamera (pc);
	}
gameData.objs.viewer = viewerSave;
gameOpts->render.nMaxFPS = frameCap;
if (gameStates.render.cockpit.nMode != cm) {
	SelectCockpit (cm);
	return nCamsRendered;
	}
return 0;
}

//------------------------------------------------------------------------------
