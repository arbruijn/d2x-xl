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

//------------------------------------------------------------------------------

#if RENDER2TEXTURE

int OglCreateCamBuf (tCamera *pc)
{
#if RENDER2TEXTURE == 1
if (!OglCreatePBuffer (&pc->pb, pc->texBuf.bm_props.w, pc->texBuf.bm_props.h, 0))
	return 0;
pc->glTexId = pc->pb.texId;
#elif RENDER2TEXTURE == 2
if (!OglCreateFBuffer (&pc->fb, pc->texBuf.bm_props.w, pc->texBuf.bm_props.h, pc->bShadowMap))
	return 0;
pc->glTexId = pc->fb.texId;
#endif
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

short nCameras = 0;
char nSideCameras [MAX_SEGMENTS][6];
tCamera cameras [MAX_CAMERAS];

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
	h = min (grdCurCanv->cv_bitmap.bm_props.w, grdCurCanv->cv_bitmap.bm_props.h);
	for (i = 1; i < h; i *= 2)
		;
	if (i > h)
		i /= 2;
	pc->texBuf.bm_props.w =
	pc->texBuf.bm_props.h = i;
	}
else 
#endif
	{
	h = grdCurCanv->cv_bitmap.bm_props.w;
	for (i = 1; i < h; i *= 2)
		;
	pc->texBuf.bm_props.w = i;
	h = grdCurCanv->cv_bitmap.bm_props.h;
	for (i = 1; i < h; i *= 2)
		;
	pc->texBuf.bm_props.h = i;
	}
pc->texBuf.bm_props.rowsize = grdCurCanv->cv_bitmap.bm_props.w;
#if RENDER2TEXTURE
if (!OglCreateCamBuf (pc)) 
#endif
{
#if CAMERA_READPIXELS
	if (!(pc->texBuf.bm_texBuf = (char *) d_malloc (pc->texBuf.bm_props.w * pc->texBuf.bm_props.h * 4)))
		return 0;
	if (gameOpts->render.cameras.bFitToWall || pc->bTeleport)
		pc->screenBuf = pc->texBuf.bm_texBuf;
	else {
		pc->screenBuf = d_malloc (grdCurCanv->cv_bitmap.bm_props.w * grdCurCanv->cv_bitmap.bm_props.h * 4);
		if (!pc->screenBuf) {
			gameOpts->render.cameras.bFitToWall = 1;
			pc->screenBuf = pc->texBuf.bm_texBuf;
			}
		}	
	memset (pc->texBuf.bm_texBuf, 0, pc->texBuf.bm_props.w * pc->texBuf.bm_props.h * 4);
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
	gameData.objs.cameraRef [OBJ_IDX (objP)] = nCameras;
	}
else {
	pc->objP = &pc->obj;
	if (bTeleport) {
		vmsVector n = *gameData.segs.segments [srcSeg].sides [srcSide].normals;
		n.x = -n.x;
		n.y = -n.y;
		n.z = -n.z;
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
	GetSideVerts (sideVerts, srcSeg, srcSide);
	for (i = 0; i < 4; i++) {
		pv = gameData.segs.vertices + sideVerts [i];
		pc->obj.position.vPos.x += pv->x;
		pc->obj.position.vPos.y += pv->y;
		pc->obj.position.vPos.z += pv->z;
		}
	pc->obj.position.vPos.x /= 4;
	pc->obj.position.vPos.y /= 4;
	pc->obj.position.vPos.z /= 4;
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
	int		h, i, j, k;
	ubyte		t;
	wall		*wallP;
	tObject	*objP;
	tTrigger	*triggerP;

if (!gameStates.app.bD2XLevel)
	return 0;
memset (nSideCameras, 0xFF, sizeof (nSideCameras));
for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++) {
	t = wallP->nTrigger;
	if (t >= gameData.trigs.nTriggers)
		continue;
	triggerP = gameData.trigs.triggers + t;
	if (triggerP->nType == TT_CAMERA) {
		for (j = 0; j < triggerP->nLinks; j++)
			if (CreateCamera (cameras + nCameras, (short) wallP->nSegment, (short) wallP->nSide, triggerP->nSegment [j], triggerP->nSide [j], NULL, 0, 0))
				nSideCameras [triggerP->nSegment [j]][triggerP->nSide [j]] = (char) nCameras++;
		}
#if TELEPORT_CAMERAS
	else if (/*EGI_FLAG (bTeleporterCams, 0, 0) &&*/ (triggerP->nType == TT_TELEPORT)) {
		if (CreateCamera (cameras + nCameras, triggerP->nSegment [0], triggerP->nSide [0], (short) wallP->nSegment, (short) wallP->nSide, NULL, 0, 1))
			nSideCameras [wallP->nSegment][wallP->nSide] = (char) nCameras++;
		}
#endif
	}
for (i = 0, objP = gameData.objs.objects; i <= gameData.objs.nLastObject; i++, objP++) {
	j = gameData.trigs.firstObjTrigger [i];
	for (h = sizeofa (gameData.trigs.objTriggerRefs); (j >= 0) && h; j = gameData.trigs.objTriggerRefs [j].next, h--) {
		triggerP = gameData.trigs.objTriggers + j;
		if (triggerP->nType == TT_CAMERA) {
			for (k = 0; k < triggerP->nLinks; k++)
				if (CreateCamera (cameras + nCameras, -1, -1, triggerP->nSegment [k], triggerP->nSide [k], objP, 0, 0))
					nSideCameras [triggerP->nSegment [k]][triggerP->nSide [k]] = (char) nCameras++;
			}
		}
	}
return nCameras;
}

//------------------------------------------------------------------------------

void DestroyCamera (tCamera *pc)
{
OglFreeBmTexture (&pc->texBuf);
glDeleteTextures (1, &pc->glTexId);
if (pc->screenBuf && (pc->screenBuf != (char *) pc->texBuf.bm_texBuf))
	d_free (pc->screenBuf);
if (pc->texBuf.bm_texBuf) {
	d_free (pc->texBuf.bm_texBuf);
	pc->texBuf.bm_texBuf = NULL;
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
	tCamera	*pc = cameras;

#if RENDER2TEXTURE == 1
if (bRender2TextureOk)
#	ifdef _WIN32
	wglMakeContextCurrentARB (hGlDC, hGlDC, hGlRC);
#	else
	glXMakeCurrent (hGlDC, hGlWindow, hGlRC);
#	endif
#endif
for (i = nCameras; i; i--, pc++)
	DestroyCamera (pc);
memset (cameras, 0, sizeof (cameras));
memset (nSideCameras, 0xFF, sizeof (nSideCameras));
nCameras = 0;
}

//------------------------------------------------------------------------------

void GetCameraUVL (tCamera *pc, uvl *uvlCopy)
{
#ifdef RELEASE
if (pc->bHaveUVL)
	memcpy (uvlCopy, pc->uvlList, sizeof (pc->uvlList));
else 
#endif
	{
		fix		h, i, dx, dy, dd = 0;
		double	ha, va, a, r, d;
		int		xFlip = (uvlCopy [2].u < uvlCopy [0].u);
		int		yFlip = (uvlCopy [1].v < uvlCopy [0].v);
		int		rotLeft = (uvlCopy [1].u < uvlCopy [0].u);
		int		rotRight = (uvlCopy [1].u > uvlCopy [0].u);
#if RENDER2TEXTURE
		int		bCamBufAvail = OglCamBufAvail (pc, 1) == 1;
		int		bFitToWall = pc->bTeleport || gameOpts->render.cameras.bFitToWall;
#endif

	h = F1_0; 
	dd = uvlCopy [1].v - uvlCopy [0].v;
	if (dd < 0)
		dd = -dd;
	dd = (dd % F1_0) ?  (F1_0 - dd % F1_0) / 2 : 0;
#if RENDER2TEXTURE == 1
	if (!bFitToWall && bCamBufAvail) {
		a = (double) grdCurCanv->cv_bitmap.bm_props.h / (double) pc->texBuf.bm_props.h;
		dy = ((fix) ((double) h * (1.0 - a))) / 2 * grdCurCanv->cv_bitmap.bm_props.h / (double) grdCurCanv->cv_bitmap.bm_props.w;
		dy -= dd;
		if (yFlip) {
			uvlCopy [0].v = -dy;
			uvlCopy [1].v = F1_0 + dy;
			}
		else {
			uvlCopy [0].v = F1_0 + dy; //-F1_0 - dy;
			uvlCopy [1].v = -dy; //F1_0;
			}
		if (yFlip) {
			uvlCopy [3].v = -dy;
			uvlCopy [2].v = F1_0 + dy;
			}
		else {
			uvlCopy [3].v = F1_0 + dy; //-F1_0 - dy;
			uvlCopy [2].v = -dy; //dy;
			}
		a = (double) (pc->texBuf.bm_props.w - grdCurCanv->cv_bitmap.bm_props.w) / 
			 (double) pc->texBuf.bm_props.w;
		dx = ((fix) ((double) h * a));
		for (i = 0; i < 2; i++) {
			if (xFlip) {
				uvlCopy [i].u = F1_0;
				uvlCopy [i + 2].u = 0;
				}
			else {
				uvlCopy [i + 2].u = F1_0;
				uvlCopy [i].u = 0;
				}
			}
		}
	else 
#elif RENDER2TEXTURE == 2
	if (!bFitToWall && bCamBufAvail) {
		r = (double) (uvlCopy [2].u - uvlCopy [0].u) / (double) (uvlCopy [1].v - uvlCopy [0].v);
		if (r < 0)
			r = -r;
		a = (double) grdCurCanv->cv_bitmap.bm_props.h / (double) pc->texBuf.bm_props.h;
		dy = ((fix) ((double) h * (1.0 - a))) / 2;// * grdCurCanv->cv_bitmap.bm_props.h / (double) grdCurCanv->cv_bitmap.bm_props.w;
		d = (double) grdCurCanv->cv_bitmap.bm_props.w / (double) grdCurCanv->cv_bitmap.bm_props.h;
		//dy = ((fix) ((double) h * (a))) / 2;// * grdCurCanv->cv_bitmap.bm_props.h / (double) grdCurCanv->cv_bitmap.bm_props.w;
		if (d - 1.0)
			dy = (int) ((double) dy * (d - r) / (d - 1.0));
		if (yFlip) {
			uvlCopy [0].v = (int) (-dy * a);
			uvlCopy [1].v = (int) ((F1_0 + dy) * a);
			}
		else {
			uvlCopy [0].v = (int) ((F1_0 + dy) * a); //-F1_0 - dy;
			uvlCopy [1].v = (int) (-dy * a); //F1_0;
			}
		if (yFlip) {
			uvlCopy [3].v = (int) (-dy * a);
			uvlCopy [2].v = (int) ((F1_0 + dy) * a);
			}
		else {
			uvlCopy [3].v = (int) ((F1_0 + dy) * a); //-F1_0 - dy;
			uvlCopy [2].v = (int) (-dy * a); //dy;
			}
		a = (double) grdCurCanv->cv_bitmap.bm_props.w / (double) pc->texBuf.bm_props.w;
		dx = ((fix) ((double) h * a)) / 2;
		dx = 0;
		for (i = 0; i < 2; i++) {
			if (xFlip) {
				uvlCopy [i].u = (int) (F1_0 * a);
				uvlCopy [i + 2].u = 0;
				}
			else {
				uvlCopy [i + 2].u = (int) (F1_0 * a);
				uvlCopy [i].u = 0;
				}
			}
		}
	else 
#endif // RENDER2TEXTURE
		{
		if (0 && bFitToWall)
			dx = dy = 0;
		else {
			ha = 1.0 - (double) grdCurCanv->cv_bitmap.bm_props.w / (double) pc->texBuf.bm_props.w;
			va = 1.0 - (double) grdCurCanv->cv_bitmap.bm_props.h / (double) pc->texBuf.bm_props.h;
			dx = (fix) ((double) F1_0 * ha);
			dy = (fix) ((double) F1_0 * va);
			dd = (pc->texBuf.bm_props.w == pc->texBuf.bm_props.h) ? (dy - dx) / 2 : dy;
			}
		if (yFlip) {
			uvlCopy [0].v =
			uvlCopy [3].v = dy;
			uvlCopy [1].v =
			uvlCopy [2].v = F1_0;
			}
		else {
			uvlCopy [0].v =
			uvlCopy [3].v = -dy;
			uvlCopy [1].v =
			uvlCopy [2].v = -F1_0;
			}
		if (xFlip) {
			uvlCopy [0].u =
			uvlCopy [1].u = -dx - dd;
			uvlCopy [2].u = 
			uvlCopy [3].u = -F1_0 + dd;
			}
		else {
			uvlCopy [0].u =
			uvlCopy [1].u = dx + dd;
			uvlCopy [2].u = 
			uvlCopy [3].u = F1_0 - dd;
			}
		if (rotLeft) {
			uvl	h = uvlCopy [0];
			uvlCopy [0] = uvlCopy [1];
			uvlCopy [1] = uvlCopy [2];
			uvlCopy [2] = uvlCopy [3];
			uvlCopy [3] = h;
			}
		else if (rotRight) {
			uvl	h = uvlCopy [0];
			uvlCopy [0] = uvlCopy [3];
			uvlCopy [3] = uvlCopy [2];
			uvlCopy [2] = uvlCopy [1];
			uvlCopy [1] = h;
			}
		}
	for (i = 0; i < 4; i++)
		uvlCopy [i].l = F1_0;
	memcpy (pc->uvlList, uvlCopy, sizeof (pc->uvlList));
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
if (pc->texBuf.bm_texBuf) 
#	endif
#endif
	{
	RenderFrame (0, 0);
	pc->bValid = 1;
	OglFreeBmTexture (&pc->texBuf);
#if CAMERA_READPIXELS
	memset (pc->texBuf.bm_texBuf, 0, pc->texBuf.bm_props.w * pc->texBuf.bm_props.h * 4);
	glDisable (GL_TEXTURE_2D);
#endif
	glReadBuffer (GL_BACK);
	if (gameOpts->render.cameras.bFitToWall || pc->bTeleport) {
#if CAMERA_READPIXELS == 0
		OglLoadBmTextureM (&pc->texBuf, 0, -1, 0, NULL);
		glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 
			(grdCurCanv->cv_bitmap.bm_props.w - pc->texBuf.bm_props.w) / 2, 
			(grdCurCanv->cv_bitmap.bm_props.h - pc->texBuf.bm_props.h) / 2, 
			pc->texBuf.bm_props.w, pc->texBuf.bm_props.h, 0);
#else
		glReadPixels (
			(grdCurCanv->cv_bitmap.bm_props.w - pc->texBuf.bm_props.w) / 2, 
			(grdCurCanv->cv_bitmap.bm_props.h - pc->texBuf.bm_props.h) / 2, 
			pc->texBuf.bm_props.w, pc->texBuf.bm_props.h, 
			GL_RGBA, GL_UNSIGNED_BYTE, pc->texBuf.bm_texBuf);
		OglLoadBmTextureM (&pc->texBuf, 0, -1, NULL);
#endif
		}
	else {
#if CAMERA_READPIXELS == 0
			OglLoadBmTextureM (&pc->texBuf, 0, -1, 0, NULL);
			glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 
				-(pc->texBuf.bm_props.w - grdCurCanv->cv_bitmap.bm_props.w) / 2,
				-(pc->texBuf.bm_props.h - grdCurCanv->cv_bitmap.bm_props.h) / 2, 
				pc->texBuf.bm_props.w, pc->texBuf.bm_props.h, 0);
#else
		char	*pSrc, *pDest;
		int	dxBuf = (pc->texBuf.bm_props.w - grdCurCanv->cv_bitmap.bm_props.w) / 2;
		int	dyBuf = (pc->texBuf.bm_props.h - grdCurCanv->cv_bitmap.bm_props.h) / 2;
		int	wSrc = grdCurCanv->cv_bitmap.bm_props.w * 4;
		int	wDest = pc->texBuf.bm_props.w * 4;

		if (grdCurCanv->cv_bitmap.bm_props.w == pc->texBuf.bm_props.w) {
			glReadPixels (
				0, 0, grdCurCanv->cv_bitmap.bm_props.w, grdCurCanv->cv_bitmap.bm_props.h,
				GL_RGBA, GL_UNSIGNED_BYTE, pc->texBuf.bm_texBuf + dyBuf * pc->texBuf.bm_props.w * 4);
			}
		else {
			glReadPixels (
				0, 0, grdCurCanv->cv_bitmap.bm_props.w, grdCurCanv->cv_bitmap.bm_props.h,
				GL_RGBA, GL_UNSIGNED_BYTE, pc->screenBuf);
			pSrc = pc->screenBuf;
			pDest = pc->texBuf.bm_texBuf + (dyBuf - 1) * wDest + dxBuf * 4;
#	ifndef _WIN32
			for (dyBuf = grdCurCanv->cv_bitmap.bm_props.h; dyBuf; dyBuf--, pSrc += wSrc, pDest += wDest)
				memcpy (pDest, pSrc, wSrc);
#	else
			dxBuf = pc->texBuf.bm_props.w - grdCurCanv->cv_bitmap.bm_props.w;
			dyBuf = grdCurCanv->cv_bitmap.bm_props.h;
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
	tCamera	*pc = cameras, *pCurCam = NULL;
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
for (i = 0; i < nCameras; i++, pc++) {
	pc->nWaitFrames++;
	if (!pc->bVisible)
		continue;
	if (pc->bTeleport && !EGI_FLAG (bTeleporterCams, 0, 0))
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
if (pc = pCurCam) {
	pc->nWaitFrames = 0;
	pc->bTimedOut = 0;
	pc->nTimeout = gameStates.app.nSDLTicks; //SDL_GetTicks ();
#else
t = gameStates.app.nSDLTicks; //SDL_GetTicks ();
for (i = 0; i < nCameras; i++, pc++) {
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
