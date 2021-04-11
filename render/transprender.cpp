#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "descent.h"
#include "u_mem.h"
#include "error.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "ogl_fastrender.h"
#include "rendermine.h"
#include "segmath.h"
#include "objrender.h"
#include "lightmap.h"
#include "lightning.h"
#include "sphere.h"
#include "../effects/glow.h"
#include "glare.h"
#include "thrusterflames.h"
#include "automap.h"
#include "transprender.h"
#include "renderthreads.h"
#include "addon_bitmaps.h"
#include "postprocessing.h"

#define RENDER_TRANSPARENCY	1
#define RENDER_TRANSP_DECALS	1

#define TRANSP_POLYS				1
#define TRANSP_FACES				1
#define TRANSP_OBJECTS			1
#define TRANSP_SPRITES			1
#define TRANSP_SPARKS			1
#define TRANSP_BULLETS			1
#define TRANSP_PARTICLES		1
#define TRANSP_SPHERES			1
#define TRANSP_LIGHTNING		1
#define TRANSP_LIGHTTRAILS		1
#define TRANSP_THRUSTER			1

#define TI_POLY_OFFSET			0
#define TI_POLY_CENTER			1

#if DBG
int32_t nDbgPoly = -1, nDbgItem = -1;
#endif

CTransparencyRenderer transparencyRenderer;

// Lazy reset controls when the entire list of transparent items is cleared.
// This is an optimization for stereoscopic rendering, where all transparent items
// need to be rendered twice. Now some of these items are passed to the 
// transparency renderer with their coordinates already transformed into view
// space, while others are passed with their world space coordinates, and 
// coordinate transformation is done by OpenGL when these items are rendered.
// The further kind of transparent items needs to be passed to the transparency
// renderer everytime the transformation changes, while the latter doesn't
// (since transformation changes are handled via OpenGL when rendering them).
// Smoke particles, which usually appear in greater numbers, belong to the latter
// category, so not having to process them twice for stereoscopic rendering is quite
// a performance improvement. Lazy reset makes sure such transparent items are
// only cleared from the buffer after the complete image (in the case of stereoscopic
// rendering left and right frame) have been rendered.
// To make handling this transparent to the various program parts passing stuff
// to the transparency renderer, the transparency renderer decides which items
// that are passed to it are accepted at what stage of the rendering process.
// LAZY_RESET controls this.

#define LAZY_RESET 1

//------------------------------------------------------------------------------

typedef struct tSparkVertex {
	CFloatVector3	vPos;
	tTexCoord2f		texCoord;
} tSparkVertex;

#define SPARK_BUF_SIZE	1000

typedef struct tSparkBuffer {
	int32_t			nSparks;
	tSparkVertex	vertices [SPARK_BUF_SIZE * 4];
	CParticle		particles [SPARK_BUF_SIZE];
} tSparkBuffer;

tSparkBuffer sparkBuffer;
CEffectArea sparkArea;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CEffectArea& CEffectArea::Add (CFloatVector& pos, float rad) 
{
ENTER (0, 0);
if (m_rad == 0.0f) {
	m_pos = pos;
	m_rad = rad;
	}
else {
	CFloatVector v = pos - m_pos;
	float d = v.Mag ();
	float h = rad + d - m_rad;
	if (h > 0.0f) {
		h /= 2.0f;
		v /= d;
		v *= h;
		m_pos += v;
		m_rad += h;
		}
	m_pos = CFloatVector::Avg (m_pos, pos);
	}
RETVAL (*this)
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CTranspPoly::Render (void)
{
ENTER (0, 0);
#if TRANSP_POLYS
if (pFace || pTriangle)
	RenderFace ();
else {
	PROF_START
		int32_t bSoftBlend = transparencyRenderer.SoftBlend (SOFT_BLEND_SPRITES);

	ogl.ResetClientStates (1);
	transparencyRenderer.Data ().pBm [1] = transparencyRenderer.Data ().pBm [2] = NULL;
	if (bAdditive & 3)
		glowRenderer.Begin (GLOW_POLYS, 2, false, 1.0f);
	ogl.EnableClientStates (pBm != NULL, nColors == nVertices, 0, GL_TEXTURE0);
	if (transparencyRenderer.LoadTexture (NULL, pBm, 0, 0, 0, 0, nWrap)) {
		if (pBm)
			OglTexCoordPointer (2, GL_FLOAT, 0, texCoord);
		if (nColors == nVertices)
			OglColorPointer (4, GL_FLOAT, 0, color);
		else
			glColor4fv (reinterpret_cast<GLfloat*> (color));
		OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), vertices);
		ogl.SetBlendMode (bAdditive);
		if (!(bSoftBlend && glareRenderer.LoadShader (5, bAdditive != 0)))
			shaderManager.Deploy (-1, true);
		//ogl.SetTexturing (false);
		//glLineWidth (5);
		ogl.SetFaceCulling (false);
		//ogl.SetupTransform (0);
		OglDrawArrays (nPrimitive, 0, nVertices);
		//ogl.ResetTransform (0);
		ogl.SetFaceCulling (true);
		//glLineWidth (1);
		}
	if (bAdditive & 3)
		glowRenderer.Done (GLOW_POLYS);
	#if DBG
	else
		HUDMessage (0, "Couldn't load '%s'", pBm->Name ());
	#endif
	#if TI_POLY_OFFSET
	if (!pBm) {
		glPolygonOffset (0,0);
		glDisable (GL_POLYGON_OFFSET_FILL);
		}
	#endif
	PROF_END(ptRenderFaces)
	}
#endif
RETURN
}

//------------------------------------------------------------------------------

void CTranspPoly::RenderFace (void)
{
ENTER (0, 0);
#if TRANSP_FACES
PROF_START
	//CSegFace*		pFace = this->pFace;
	//tFaceTriangle*	pTriangle = this->pTriangle;
	CBitmap*			bmBot = pBm, *bmTop, *bmMask;
	int32_t			bDecal, 
						bLightmaps = transparencyRenderer.Data ().bLightmaps && !gameStates.render.bFullBright,
						bTextured = (pFace->m_info.nSegColor == 0) && (bmBot != NULL), 
						bColored = (nColors == nVertices) && (bTextured || (gameStates.render.bPerPixelLighting != 2)) && !gameStates.render.bFullBright;

#if TI_POLY_OFFSET
if (pFace->m_info.nSegColor) {
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1.0, -1.0);
	//glPolygonMode (GL_FRONT, GL_FILL);
	}
#endif

#if DBG
if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide))) {
	BRP;
	}
#endif

if (bTextured) {
	if ((bmTop = pFace->bmTop))
		bmTop = bmTop->Override (BitmapFrame (bmTop, pFace->m_info.nBaseTex, pFace->m_info.nSegment));
	if (bmTop && !(bmTop->Flags () & (BM_FLAG_SUPER_TRANSPARENT | BM_FLAG_TRANSPARENT | BM_FLAG_SEE_THRU))) {
		bmBot = bmTop;
		bmTop = bmMask = NULL;
		bDecal = -1;
		pFace->m_info.nRenderType = gameStates.render.history.nType = 1;
		}
	else {
		bDecal = (bmTop != NULL);
		bmMask = (bDecal && ((bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT) != 0) && gameStates.render.textures.bHaveMaskShader) ? bmTop->Mask () : NULL;
		}
	}
else {
	bmBot = bmTop = bmMask = NULL;
	bDecal = 0;
	}

int32_t bAdditive = this->bAdditive, nIndex = pTriangle ? pTriangle->nIndex : pFace->m_info.nIndex;
if (bAdditive & 3)
	glowRenderer.Begin (GLOW_FACES, 2, false, 1.0f);
#if 0 //DBG
transparencyRenderer.Data ().pBm [0] = NULL;
ogl.ResetClientStates (bLightmaps);
#else
ogl.ResetClientStates (bTextured + bLightmaps + (bmTop != NULL) + (bmMask != NULL));
if (!bTextured)
	ogl.SetTexturing (false);
#endif

if (bmTop) {
	ogl.EnableClientStates (bTextured, 0, 0, GL_TEXTURE1 + bLightmaps);
#if 0 //DBG
	transparencyRenderer.Data ().pBm [1] = NULL;
#endif
	if (!transparencyRenderer.LoadTexture (pFace, bmTop, pFace->m_info.nOvlTex, 0, 1, bLightmaps, nWrap))
		RETURN
	if (bTextured)
		OglTexCoordPointer (2, GL_FLOAT, 0, FACES.ovlTexCoord + nIndex);
	OglVertexPointer (3, GL_FLOAT, 0, FACES.vertices + nIndex);
	if (bmMask) {
		ogl.EnableClientStates (bTextured, 0, 0, GL_TEXTURE2 + bLightmaps);
		if (!transparencyRenderer.LoadTexture (pFace, bmMask, pFace->m_info.nBaseTex, 0, 2, bLightmaps, nWrap))
			RETURN
		if (bTextured)
			OglTexCoordPointer (2, GL_FLOAT, 0, FACES.ovlTexCoord + nIndex);
		OglVertexPointer (3, GL_FLOAT, 0, FACES.vertices + nIndex);
		}
	else
		transparencyRenderer.Data ().pBm [2] = NULL;
	}
else {
	transparencyRenderer.Data ().pBm [1] = transparencyRenderer.Data ().pBm [2] = NULL;
	}

if (bLightmaps)
	ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0 + bTextured);
else
	ogl.EnableClientStates (bTextured, bColored, 1, GL_TEXTURE0);
if (!transparencyRenderer.LoadTexture (pFace, bmBot, pFace->m_info.nBaseTex, 0, 0, bLightmaps, nWrap)) {
	if (bAdditive & 3)
		glowRenderer.Done (GLOW_FACES);
	RETURN
	}	
if (bTextured)
	OglTexCoordPointer (2, GL_FLOAT, 0, (bDecal < 0) ? FACES.ovlTexCoord + nIndex : FACES.texCoord + nIndex);

if (!bLightmaps) {
	if (bColored)
		OglColorPointer (4, GL_FLOAT, 0, FACES.color + nIndex);
	OglNormalPointer (GL_FLOAT, 0, FACES.normals + nIndex);
	}
OglVertexPointer (3, GL_FLOAT, 0, FACES.vertices + nIndex);

if (bLightmaps) {
	ogl.EnableClientStates (1, bColored, 1, GL_TEXTURE0);
	OglTexCoordPointer (2, GL_FLOAT, 0, FACES.lMapTexCoord + nIndex);
	if (bColored)
		OglColorPointer (4, GL_FLOAT, 0, FACES.color + nIndex);
	OglNormalPointer (GL_FLOAT, 0, FACES.normals + nIndex);
	OglVertexPointer (3, GL_FLOAT, 0, FACES.vertices + nIndex);
	}
#if 0
if (!bTextured) {
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
	transparencyRenderer.Data ().pBm [0] = NULL;
	}
#endif
ogl.SetupTransform (1);
if (!(bColored || bTextured))
	glColor4fv (reinterpret_cast<GLfloat*> (&pFace->m_info.color));
else if (gameStates.render.bFullBright)
	glColor4f (1.0f, 1.0f, 1.0f, color [0].Alpha ());
else
	glColor4fv (reinterpret_cast<GLfloat*> (color));
ogl.SetBlendMode (bAdditive);

#if DBG
if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
	BRP;
//else
//	RETURN
//shaderManager.Deploy (-1);
#endif

if (gameStates.render.bPerPixelLighting && !gameStates.render.bFullBright) {
	if (!pFace->m_info.bColored) {
		SetupGrayScaleShader ((int32_t) pFace->m_info.nRenderType, &pFace->m_info.color);
		OglDrawArrays (nPrimitive, 0, nVertices);
		}
	else {
		if (gameStates.render.bPerPixelLighting == 1) {
			SetupLightmapShader (pFace, bTextured ? int32_t (pFace->m_info.nRenderType) : 0, false);
			OglDrawArrays (nPrimitive, 0, nVertices);
			}
		else {
			ogl.m_states.iLight = 0;
			lightManager.Index (0,0).nActive = -1;
			for (;;) {
				SetupPerPixelLightingShader (pFace, bTextured ? int32_t (pFace->m_info.nRenderType) : 0, false);
				OglDrawArrays (nPrimitive, 0, nVertices);
				if (ogl.m_states.iLight >= ogl.m_states.nLights) 
					break;
				bAdditive = 1;
				ogl.SetBlendMode (OGL_BLEND_ADD);
				ogl.SetDepthWrite (false);
				}
			}
		if (gameStates.render.bHeadlights) {
#	if DBG
			if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
				BRP;
			ogl.SetBlending (true);
#	endif
			lightManager.Headlights ().SetupShader (bTextured ? bmMask ? 3 : bDecal ? 2 : 1 : 0, 1, bTextured ? NULL : &pFace->m_info.color);
			if (bAdditive != 2) {
				bAdditive = 2;
				ogl.SetBlendMode (OGL_BLEND_ADD_WEAK);
				ogl.SetDepthWrite (false);
				}
			OglDrawArrays (nPrimitive, 0, nVertices);
			}
		}
	}
else {
	SetupShader (pFace, bmMask != NULL, bDecal > 0, bmBot != NULL,
				    (nSegment < 0) || !automap.Active () || automap.m_visited [nSegment],
					 bTextured ? NULL : pFace ? &pFace->m_info.color : color);
	OglDrawArrays (nPrimitive, 0, nVertices);
	}
if (bAdditive & 3)
	glowRenderer.Done (GLOW_FACES);
ogl.ResetTransform (pFace != NULL);
gameData.renderData.nTotalFaces++;

#if TI_POLY_OFFSET
if (pFace->m_info.nSegColor) {
	glPolygonOffset (0,0);
	glDisable (GL_POLYGON_OFFSET_FILL);
	}
#endif
PROF_END(ptRenderFaces)
#endif
RETURN
}

//------------------------------------------------------------------------------

void CTranspObject::Render (void)
{
ENTER (0, 0);
#if TRANSP_OBJECTS
shaderManager.Deploy (-1);
ogl.ResetClientStates ();
gameData.modelData.vScale = vScale;
DrawPolygonObject (pObj, 1);
gameData.modelData.vScale.SetZero ();
transparencyRenderer.ResetBitmaps ();
#endif
RETURN
}

//------------------------------------------------------------------------------

void CTranspSprite::Render (void)
{
ENTER (0, 0);
#if TRANSP_SPRITES
	int32_t bSoftBlend = (fSoftRad > 0) && transparencyRenderer.SoftBlend (SOFT_BLEND_SPRITES);
	int32_t bGlow = 0;

if (bAdditive < 0)
	bAdditive = -bAdditive;
else if (glowRenderer.Available (GLOW_SPRITES)) {
	bGlow = 1;
	if (bAdditive == 1) 
		glowRenderer.Begin (GLOW_SPRITES, 2, false, 0.85f);
	else if (bAdditive == 2)
		glowRenderer.Begin (GLOW_SPRITES, 1, false, 0.95f);
	else {
		bGlow = 0;
		if (glowRenderer.End ())
			transparencyRenderer.ResetBitmaps ();
		}
	}

ogl.ResetClientStates (1);
transparencyRenderer.Data ().pBm [1] = transparencyRenderer.Data ().pBm [2] = NULL;
transparencyRenderer.Data ().bUseLightmaps = 0;
ogl.SelectTMU (GL_TEXTURE0, true);
if (transparencyRenderer.LoadTexture (NULL, pBm, 0, 0, 0, 0, GL_CLAMP)) {
	ogl.SetTexturing (true);
	transparencyRenderer.ResetBitmaps ();
	if (bColor)
		glColor4fv (reinterpret_cast<GLfloat*> (&color));
	else
		glColor3f (1,1,1);
	ogl.SetBlendMode (bAdditive);
	if (!(bSoftBlend && glareRenderer.LoadShader (fSoftRad, bAdditive != 0)))
		shaderManager.Deploy (-1, true);
	pBm->SetColor ();
	CFloatVector vPosf;
	transformation.Transform (vPosf, position, 0);
#if DBG
	if (glowRenderer.SetViewport (GLOW_SPRITES, *vPosf.XYZ (), X2F (nWidth), X2F (nHeight), true) < 0) {
		transformation.Transform (vPosf, vPosf);
		tScreenPos s;
		ProjectPoint (*vPosf.XYZ (), s);
		BRP;
		}
#endif
	if (!bGlow || (glowRenderer.SetViewport (GLOW_SPRITES, *vPosf.XYZ (), X2F (nWidth), X2F (nHeight), true) > -1))
		ogl.RenderQuad (pBm, vPosf, X2F (nWidth), X2F (nHeight), 3);
	}
if (bGlow)
	glowRenderer.Done (GLOW_SPRITES);
#endif
RETURN
}

//------------------------------------------------------------------------------

void CTranspSpark::Render (void)
{
ENTER (0, 0);
#if TRANSP_SPARKS
	float	nCol = (float) (nFrame / 8);
	float	nRow = (float) (nFrame % 8);

if (!nType)
	nCol += 4;
if (USE_PARTICLE_SHADER) {
	if (sparkBuffer.nSparks >= SPARK_BUF_SIZE) {
		transparencyRenderer.FlushParticleBuffer (-1);
		sparkBuffer.nSparks = 0;
		}

	CParticle& p = sparkBuffer.particles [sparkBuffer.nSparks];

	p.m_nType =
	p.m_nRenderType = SMOKE_PARTICLES;
	p.m_nLife = p.m_nTTL = 1000;
	p.m_decay = 1.0f;
	p.m_nRad = 1.0f;
	p.m_renderColor.Red () = p.m_renderColor.Green () = p.m_renderColor.Blue () = p.m_renderColor.Alpha () = 1.0f;
	p.m_nWidth =
	p.m_nHeight = X2F (nSize);
	p.m_bEmissive = -1;
	p.m_bRotate = 1;
	p.m_bBlowUp = -1;
	p.m_nRotFrame = (nRotFrame + nFrame) % 64;
	p.m_vPosf = position;
	p.m_vPos.Assign (position);
	p.m_texCoord.v.v = nCol* 0.125f;
	p.m_texCoord.v.u = nRow* 0.125f; 
	particleManager.Add (&p, 1.0f);
	}
else {
	if (sparkBuffer.nSparks >= SPARK_BUF_SIZE)
		transparencyRenderer.FlushSparkBuffer ();

		tSparkVertex	*pVertex = sparkBuffer.vertices + 4 * sparkBuffer.nSparks;
		CFloatVector	vPos;
		float				nSize = X2F (this->nSize);

	sparkArea.Add (position, nSize);

	transformation.Transform (vPos, position, 0);
	pVertex->vPos.v.coord.x = vPos.v.coord.x - nSize;
	pVertex->vPos.v.coord.y = vPos.v.coord.y + nSize;
	pVertex->vPos.v.coord.z = vPos.v.coord.z;
	pVertex->texCoord.v.u = nCol* 0.125f;
	pVertex->texCoord.v.v = (nRow + 1)* 0.125f;
	pVertex++;
	pVertex->vPos.v.coord.x = vPos.v.coord.x + nSize;
	pVertex->vPos.v.coord.y = vPos.v.coord.y + nSize;
	pVertex->vPos.v.coord.z = vPos.v.coord.z;
	pVertex->texCoord.v.u = (nCol + 1)* 0.125f;
	pVertex->texCoord.v.v = (nRow + 1)* 0.125f;
	pVertex++;
	pVertex->vPos.v.coord.x = vPos.v.coord.x + nSize;
	pVertex->vPos.v.coord.y = vPos.v.coord.y - nSize;
	pVertex->vPos.v.coord.z = vPos.v.coord.z;
	pVertex->texCoord.v.u = (nCol + 1)* 0.125f;
	pVertex->texCoord.v.v = nRow* 0.125f;
	pVertex++;
	pVertex->vPos.v.coord.x = vPos.v.coord.x - nSize;
	pVertex->vPos.v.coord.y = vPos.v.coord.y - nSize;
	pVertex->vPos.v.coord.z = vPos.v.coord.z;
	pVertex->texCoord.v.u = nCol* 0.125f;
	pVertex->texCoord.v.v = nRow* 0.125f;
	}
sparkBuffer.nSparks++;
#endif
RETURN
}

//------------------------------------------------------------------------------

void CTranspSphere::Render (void)
{
ENTER (0, 0);
#if TRANSP_SPHERES
ogl.ResetClientStates ();
shaderManager.Deploy (-1, true);
if (nType == riSphereShield) {
	gameData.renderData.shield->SetupSurface (pPulse, pBitmap);
	DrawShieldSphere (pObj, color.Red (), color.Green (), color.Blue (), color.Alpha (), bAdditive, nSize);
	}
else if (nType == riMonsterball) {
	if (glowRenderer.End ())
		transparencyRenderer.ResetBitmaps ();
	DrawMonsterball (pObj, color.Red (), color.Green (), color.Blue (), color.Alpha ());
	}
//shaderManager.Deploy (-1);
ogl.SetDepthWrite (false);
transparencyRenderer.ResetBitmaps ();
#endif
RETURN
}

//------------------------------------------------------------------------------

void CTranspParticle::RenderBullet (CParticle *bullet)
{
ENTER (0, 0);
#if TRANSP_BULLETS
	CObject	o;

memset (&o, 0, sizeof (o));
o.info.nType = OBJ_POWERUP;
o.info.position.vPos = bullet->m_vPos;
o.info.position.mOrient = bullet->m_mOrient;
if (0 <= (o.info.nSegment = FindSegByPos (o.info.position.vPos, bullet->m_nSegment, 0, 0))) {
	lightManager.Index (0,0).nActive = 0;
	o.info.renderType = RT_POLYOBJ;
	o.rType.polyObjInfo.nModel = BULLET_MODEL;
	o.rType.polyObjInfo.nTexOverride = -1;
	DrawPolygonObject (&o, 1);
	gameData.modelData.vScale.SetZero ();
	transparencyRenderer.ResetBitmaps ();
	}
#endif
RETURN
}

//------------------------------------------------------------------------------

void CTranspParticle::Render (void)
{
ENTER (0, 0);
#if TRANSP_PARTICLES
if (particle->m_nType == BULLET_PARTICLES)
	RenderBullet (particle);
else {
	if (particle->Render (fBrightness) < 0)
		transparencyRenderer.ResetBitmaps ();
	}
#endif
RETURN
}

//------------------------------------------------------------------------------

void CTranspLightning::Render (void)
{
ENTER (0, 0);
#if TRANSP_LIGHTNING
if (transparencyRenderer.Data ().nPrevType != transparencyRenderer.Data ().nCurType) {
	ogl.ResetClientStates ();
	shaderManager.Deploy (-1, true);
	}
shaderManager.Deploy (-1, false);
lightning->Render (nDepth, 0);
transparencyRenderer.ResetBitmaps ();
#endif
RETURN
}

//------------------------------------------------------------------------------

void CTranspLightTrail::Render (void)
{
ENTER (0, 0);
#if TRANSP_LIGHTTRAILS
if (transparencyRenderer.Data ().nPrevType != transparencyRenderer.Data ().nCurType) {
	ogl.ResetClientStates (1);
	transparencyRenderer.Data ().pBm [1] = transparencyRenderer.Data ().pBm [2] = NULL;
	transparencyRenderer.Data ().bUseLightmaps = 0;
	shaderManager.Deploy (-1, true);
	}
glowRenderer.Begin (GLOW_LIGHTTRAILS, 2, false);
ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
if (transparencyRenderer.LoadTexture (NULL, pBm, 0, 0, 0, 0, GL_CLAMP)) {
	ogl.SetDepthWrite (false); //true);
	ogl.SetFaceCulling (false);
	ogl.SetBlendMode (OGL_BLEND_ADD);
	glColor4fv (reinterpret_cast<GLfloat*> (&color));
	OglTexCoordPointer (2, GL_FLOAT, 0, texCoord);
	OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), vertices);
	ogl.SetupTransform (1);
	OglDrawArrays (GL_QUADS, 0, bTrail ? 8 : 4);
	ogl.ResetTransform (1);
	ogl.SetFaceCulling (true);
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
	}
glowRenderer.Done (GLOW_LIGHTTRAILS);
#endif
RETURN
}

//------------------------------------------------------------------------------

void CTranspThruster::Render (void)
{
ENTER (0, 0);
#if TRANSP_THRUSTER
shaderManager.Deploy (-1, true);
ogl.ResetClientStates ();
thrusterFlames.Render (pObj, &info, nThruster);
gameData.modelData.vScale.SetZero ();
transparencyRenderer.ResetBitmaps ();
#endif
RETURN
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32_t CTranspItemBuffers::Create (void)
{
ENTER (0, 0);
	float scale = 1.0f / sqrt (float (gameStates.app.nThreads));

if (depthBuffer.Buffer ())
	RETVAL (1)
if (!depthBuffer.Create (uint32_t (ITEM_DEPTHBUFFER_SIZE * scale), "CTranspItemBuffers::depthBuffer"))
	RETVAL (0)
if (!itemHeap.Create (uint32_t (ITEM_BUFFER_SIZE * scale), "CTranspItemBuffers::itemHeap")) {
	depthBuffer.Destroy ();
	RETVAL (0)
	}
nHeapSize = 0;
Clear ();
ResetFreeList ();
RETVAL (1)
}

//------------------------------------------------------------------------------

void CTranspItemBuffers::Destroy (void)
{
ENTER (0, 0);
itemHeap.Destroy ();
depthBuffer.Destroy ();
RETURN
}

//------------------------------------------------------------------------------

void CTranspItemBuffers::Clear (void)
{
ENTER (0, 0);
if (depthBuffer.Buffer ())
	depthBuffer.Clear ();
#if DBG
if (itemHeap.Buffer ())
	memset (itemHeap.Buffer (), 0, nHeapSize);
#endif
RETURN
}

//------------------------------------------------------------------------------

void CTranspItemBuffers::ResetFreeList (void)
{
ENTER (0, 0);
memset (freeList, 0, sizeof (freeList));
RETURN
}

//------------------------------------------------------------------------------

inline CTranspItem* CTranspItemBuffers::AllocItem (int32_t nType, int32_t nSize)
{
ENTER (0, 0);
	CTranspItem* ti = freeList [nType];

if (ti) {
	freeList [nType] = ti->nextItemP;
	RETVAL (ti)
	}
if (nHeapSize + nSize >= (int32_t) itemHeap.Length ())
	RETVAL (NULL)
nHeapSize += nSize;
RETVAL ((CTranspItem*) itemHeap.Buffer (nHeapSize - nSize))
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static tTexCoord2f tcDefault [4] = {{{0,0}},{{1,0}},{{1,1}},{{0,1}}};

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::ItemCount (int32_t i) 
{ 
	int32_t nCount = 0;

for (int32_t j = 0; j < gameStates.app.nThreads; j++)
	nCount += m_data.buffers [j].nItems [i];
return nCount;
}

//------------------------------------------------------------------------------

inline int32_t CTransparencyRenderer::HeapSize (void)
{
	int32_t nHeapSize = 0;

for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
	nHeapSize += m_data.buffers [i].nHeapSize;
return nHeapSize;
}

//------------------------------------------------------------------------------

inline int32_t CTransparencyRenderer::DepthBuffer (void)
{
for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
	if (m_data.buffers [i].depthBuffer.Buffer ())
		return i;
return -1;
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::AllocBuffers (void)
{
ENTER (0, 0);
for (int32_t i = 0; i < gameStates.app.nThreads; i++)
	if (!m_data.buffers [i].Create ())
		RETVAL (0)
RETVAL (1)
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::FreeBuffers (void)
{
ENTER (0, 0);
for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
	m_data.buffers [i].Destroy ();
RETURN
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::ResetBuffers (void)
{
ENTER (0, 0);
for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
	m_data.buffers [i].Clear ();
RETURN
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::ResetFreeList (void)
{
ENTER (0, 0);
for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
	m_data.buffers [i].ResetFreeList ();
RETURN
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::Reset (void)
{
ENTER (0, 0);
for (int32_t i = 0; i < gameStates.app.nThreads; i++) 
	m_data.buffers [i].Reset ();
RETURN
}

//------------------------------------------------------------------------------

inline CTranspItem* CTransparencyRenderer::AllocItem (int32_t nType, int32_t nSize, int32_t nThread)
{
ENTER (0, 0);
RETVAL (m_data.buffers [nThread].AllocItem (nType, nSize))
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::InitBuffer (int32_t zMin, int32_t zMax, int32_t nWindow)
{
ENTER (0, 0);
#if LAZY_RESET
if (nWindow)
	m_data.bAllowAdd = 1;
else if (gameStates.render.cameras.bActive)
	m_data.bAllowAdd = 1;
else if (ogl.StereoDevice () && (ogl.StereoSeparation () >= 0))
	m_data.bAllowAdd = -1;
else
	m_data.bAllowAdd = 1;
if (m_data.bAllowAdd > 0) 
#endif
	{
	m_data.zMin = 0;
	m_data.zMax = zMax;
#if DBG
	if (zMax < 0)
		BRP;
#endif
	m_data.zScale = (double) (ITEM_DEPTHBUFFER_SIZE - 1) / (double) (m_data.zMax * sqrt (double (gameStates.app.nThreads)));
	if (m_data.zScale < 0.0)
		m_data.zScale = 1.0;
	else if (m_data.zScale > 1.0)
		m_data.zScale = 1.0;
	m_data.vViewer [0] = gameData.renderData.mine.viewer.vPos;
	transformation.Transform (m_data.vViewer [1], m_data.vViewer [0]);
	m_data.vViewerf [0].Assign (m_data.vViewer [0]);
	m_data.vViewerf [1].Assign (m_data.vViewer [1]);
	}
if (ogl.StereoSeparation () <= 0)
	m_data.bRenderGlow =
	m_data.bSoftBlend = 0;
m_data.bReady = 1;
RETURN
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::Add (CTranspItem* item, CFixVector vPos, int32_t nOffset, bool bClamp, int32_t bTransformed)
{
ENTER (0, 0);
//if (gameStates.render.nType == RENDER_TYPE_TRANSPARENCY)
//	RETVAL (0)
if (!Ready ())
	RETVAL (0)
#if LAZY_RESET
if (!m_data.bAllowAdd)
	RETVAL (0)
if ((bTransformed > 0) && (m_data.bAllowAdd < 0))	// already added and still in buffer
	RETVAL (0)
#endif

#if RENDER_TRANSPARENCY

int32_t nDepth = Depth (vPos, bTransformed > 0);
	
if (nDepth >= I2X (nOffset))
	nDepth -= I2X (nOffset);

if (nDepth < 0) {
	if (!bClamp)
		RETVAL (1)
	nDepth = m_data.zMin;
	}
else if (nDepth > m_data.zMax) {
	if (!bClamp)
		RETVAL (1)
	nDepth = m_data.zMax;
	}

// find the first particle to insert the new one *before* and place in pj; pi will be it's predecessor (NULL if to insert at list start)
nOffset = int32_t (double (nDepth) * m_data.zScale);

#if USE_OPENMP > 1
int32_t nThread = omp_get_thread_num ();
#	if DBG
if (nThread > gameStates.app.nThreads)
	RETVAL (0)
if (nThread == gameStates.app.nThreads)
	nThread = nThread;
#	endif
#else
int32_t nThread = 0;
#endif

CTranspItemBuffers& buffer = m_data.buffers [nThread];

if ((nOffset < 0) || (nOffset >= (int32_t) buffer.depthBuffer.Length ()))
	RETVAL (0)
CTranspItem* ph = buffer.AllocItem (item->Type (), item->Size ());
if (!ph) {
	RETVAL (0)
	}

memcpy (ph, item, item->Size ());
ph->nItem = buffer.nItems [0]++;
ph->bRendered = 0;
ph->bTransformed = bTransformed;
ph->z = nDepth;
ph->bValid = 1;
CTranspItem** pd = buffer.depthBuffer + nOffset;
#if 0 // sort by depth
CTranspItem* pi;
for (pi = pd->head; pi; pi = pi->nextItemP) {
	if ((pi->z < nDepth) || ((pi->z == nDepth) && (pi->nType < nType)))
		break;
	}
if (pi) {
	ph->nextItemP = pi->nextItemP;
	pi->nextItemP = ph;
	}
else 
#endif
	{
	ph->nextItemP = *pd;
	*pd = ph;
	}
if (buffer.nMinOffs > nOffset)
	buffer.nMinOffs = nOffset;
if (buffer.nMaxOffs < nOffset)
	buffer.nMaxOffs = nOffset;
RETVAL (1)
#else // RENDER_TRANSPARENCY
RETVAL (0)
#endif // RENDER_TRANSPARENCY
}

//------------------------------------------------------------------------------

#if TI_SPLIT_POLYS

int32_t CTransparencyRenderer::SplitPoly (tTranspPoly *item, int32_t nDepth)
{
	tTranspPoly		split [2];
	CFloatVector		vSplit;
	CFloatVector	color;
	int32_t			i, l, i0, i1, i2, i3, nMinLen = 0x7fff, nMaxLen = 0;
	float			z, zMin, zMax, *coord, *c0, *c1;

split [0] = split [1] = *item;
for (i = i0 = 0; i < split [0].nVertices; i++) {
	l = split [0].sideLength [i];
	if (nMaxLen < l) {
		nMaxLen = l;
		i0 = i;
		}
	if (nMinLen > l)
		nMinLen = l;
	}
if ((nDepth > 1) || !nMaxLen || (nMaxLen < 10) || ((nMaxLen <= 30) && ((split [0].nVertices == 3) || (nMaxLen <= nMinLen / 2 * 3)))) {
	for (i = 0, zMax = 0, zMin = 1e30f; i < split [0].nVertices; i++) {
		z = split [0].vertices [i].dir.coord.z;
		if (zMax < z)
			zMax = z;
		if (zMin > z)
			zMin = z;
		}
#if TI_POLY_CENTER
	return Add (item->pBm ? riTexPoly : riFlatPoly, item, sizeof (*item), F2X (zMax), F2X ((zMax + zMin) / 2));
#else
	return Add (item->pBm ? riTexPoly : riFlatPoly, item, sizeof (*item), F2X (zMax), F2X (zMin));
#endif
	}
if (split [0].nVertices == 3) {
	i1 = (i0 + 1) % 3;
	vSplit = CFloatVector::Avg(split [0].vertices [i0], split [0].vertices [i1]);
	split [0].vertices [i0] =
	split [1].vertices [i1] = vSplit;
	split [0].sideLength [i0] =
	split [1].sideLength [i0] = nMaxLen / 2;
	if (split [0].pBm) {
		split [0].texCoord [i0].dir.u =
		split [1].texCoord [i1].dir.u = (split [0].texCoord [i1].dir.u + split [0].texCoord [i0].dir.u) / 2;
		split [0].texCoord [i0].dir.dir =
		split [1].texCoord [i1].dir.dir = (split [0].texCoord [i1].dir.dir + split [0].texCoord [i0].dir.dir) / 2;
		}
	if (split [0].nColors == 3) {
		for (i = 4, coord = reinterpret_cast<float*> (&color, c0 = reinterpret_cast<float*> (split [0].color + i0), c1 = reinterpret_cast<float*> (split [0].color + i1); i; i--)
			*coord++ = (*c0++ + *c1++) / 2;
		split [0].color [i0] =
		split [1].color [i1] = color;
		}
	}
else {
	i1 = (i0 + 1) % 4;
	i2 = (i0 + 2) % 4;
	i3 = (i1 + 2) % 4;
	vSplit = CFloatVector::Avg(split [0].vertices [i0], split [0].vertices [i1]);
	split [0].vertices [i1] =
	split [1].vertices [i0] = vSplit;
	vSplit = CFloatVector::Avg(split [0].vertices [i2], split [0].vertices [i3]);
	split [0].vertices [i2] =
	split [1].vertices [i3] = vSplit;
	if (split [0].pBm) {
		split [0].texCoord [i1].dir.u =
		split [1].texCoord [i0].dir.u = (split [0].texCoord [i1].dir.u + split [0].texCoord [i0].dir.u) / 2;
		split [0].texCoord [i1].dir.dir =
		split [1].texCoord [i0].dir.dir = (split [0].texCoord [i1].dir.dir + split [0].texCoord [i0].dir.dir) / 2;
		split [0].texCoord [i2].dir.u =
		split [1].texCoord [i3].dir.u = (split [0].texCoord [i3].dir.u + split [0].texCoord [i2].dir.u) / 2;
		split [0].texCoord [i2].dir.dir =
		split [1].texCoord [i3].dir.dir = (split [0].texCoord [i3].dir.dir + split [0].texCoord [i2].dir.dir) / 2;
		}
	if (split [0].nColors == 4) {
		for (i = 4, coord = reinterpret_cast<float*> (&color, c0 = reinterpret_cast<float*> (split [0].color + i0), c1 = reinterpret_cast<float*> (split [0].color + i1); i; i--)
			*coord++ = (*c0++ + *c1++) / 2;
		split [0].color [i1] =
		split [1].color [i0] = color;
		for (i = 4, coord = reinterpret_cast<float*> (&color, c0 = reinterpret_cast<float*> (split [0].color + i2), c1 = reinterpret_cast<float*> (split [0].color + i3); i; i--)
			*coord++ = (*c0++ + *c1++) / 2;
		split [0].color [i2] =
		split [1].color [i3] = color;
		}
	split [0].sideLength [i0] =
	split [1].sideLength [i0] =
	split [0].sideLength [i2] =
	split [1].sideLength [i2] = nMaxLen / 2;
	}
return SplitPoly (split, nDepth + 1) && SplitPoly (split + 1, nDepth + 1);
}

#endif

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::AddObject (CObject *pObj)
{
ENTER (0, 0);
if (gameStates.render.nShadowMap)
	RETVAL (0)

	CTranspObject	item;
//	CFixVector		vPos;

if (pObj->info.nType == 255)
	RETVAL (0)
item.pObj = pObj;
item.vScale = gameData.modelData.vScale;
if (pObj->Cloaked ())
	m_data.bRenderGlow = 1;
//transformation.Transform (vPos, OBJPOS (pObj)->vPos, 0);
RETVAL (Add (&item, OBJPOS (pObj)->vPos, 0, 0, -1))
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::AddPoly (CSegFace *pFace, tFaceTriangle *pTriangle, CBitmap *pBm,
													 CFloatVector *vertices, char nVertices, tTexCoord2f *texCoord, CFloatVector *color,
													 CFaceColor* altColor, char nColors, char bDepthMask, int32_t nPrimitive, int32_t nWrap, int32_t bAdditive,
													 int16_t nSegment)
{
ENTER (0, 0);
if (gameStates.render.nShadowMap)
	RETVAL (0)

	CTranspPoly		item;
	int32_t			i;
	float				s = gameStates.render.grAlpha;
	//fix			zCenter;

item.pFace = pFace;
item.pTriangle = pTriangle;
item.pBm = pBm;
item.nVertices = nVertices;
item.nPrimitive = nPrimitive;
item.nWrap = nWrap;
item.bDepthMask = bDepthMask;
item.bAdditive = bAdditive;
item.nSegment = nSegment;
memcpy (item.texCoord, texCoord ? texCoord : tcDefault, nVertices * sizeof (tTexCoord2f));
if ((item.nColors = nColors)) {
	if (nColors < nVertices)
		nColors = 1;
	if (color) {
		memcpy (item.color, color, nColors * sizeof (CFloatVector));
		for (i = 0; i < nColors; i++)
			if (bAdditive)
				item.color [i].Alpha () = 1;
			else
				item.color [i].Alpha () *= s;
		}
	else if (altColor) {
		for (i = 0; i < nColors; i++) {
			item.color [i] = altColor [i];
			if (bAdditive)
				item.color [i].Alpha () = 1;
			else
				item.color [i].Alpha () *= s;
			}
		}
	else
		item.nColors = 0;
	}
memcpy (item.vertices, vertices, nVertices * sizeof (CFloatVector));
#if TI_SPLIT_POLYS
if (bDepthMask && m_data.bSplitPolys) {
	for (i = 0; i < nVertices; i++)
		item.sideLength [i] = (int16_t) FRound (CFloatVector::Dist(vertices [i], vertices [(i + 1) % nVertices]));
	return SplitPoly (&item, 0);
	}
else
#endif
	{
	if (pFace)
		RETVAL (Add (&item, SEGMENT (pFace->m_info.nSegment)->Side (pFace->m_info.nSide)->Center (), 0, true, -1))
	CFloatVector v = item.vertices [0];
	for (i = 1; i < item.nVertices; i++) 
		v += item.vertices [i];
	v /= item.nVertices;
	CFixVector vPos;
	vPos.Assign (v);
	if (bAdditive & 3)
		m_data.bRenderGlow = 1;
	if (gameOpts->SoftBlend (SOFT_BLEND_SPRITES) != 0)
		m_data.bSoftBlend = 1;
	RETVAL (Add (&item, vPos, 0, true))
	}
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::AddFaceTris (CSegFace *pFace)
{
ENTER (0, 0);
if (gameStates.render.nShadowMap)
	RETVAL (0)

	tFaceTriangle*	pTriangle;
	CFloatVector	vertices [3];
	int32_t			h, i, j, bAdditive = FaceIsAdditive (pFace);
	CBitmap*			pBm = pFace->m_info.bTextured ? /*pFace->bmTop ? pFace->bmTop :*/ pFace->bmBot : NULL;

if (pBm)
	pBm = pBm->Override (BitmapFrame (pBm, pFace->m_info.nBaseTex, pFace->m_info.nSegment));
#if DBG
if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
	BRP;
#endif
pTriangle = FACES.tris + pFace->m_info.nTriIndex;
for (h = pFace->m_info.nTris; h; h--, pTriangle++) {
	for (i = 0, j = pTriangle->nIndex; i < 3; i++, j++) {
#if 1
		transformation.Transform (vertices [i], *(reinterpret_cast<CFloatVector*> (FACES.vertices + j)), 0);
#else
		if (automap.Active ())
			transformation.Transform (vertices + i, gameData.segData.fVertices + pTriangle->index [i], 0);
		else
			vertices [i].Assign (RENDERPOINTS [pTriangle->index [i]].m_vertex [1]);
#endif
		}
	if (!AddPoly (pFace, pTriangle, pBm, vertices, 3, FACES.texCoord + pTriangle->nIndex,
					  FACES.color + pTriangle->nIndex,
					  NULL, 3, (pBm != NULL) && !bAdditive, GL_TRIANGLES, GL_REPEAT,
					  bAdditive, pFace->m_info.nSegment))
		RETVAL (0)
	}
RETVAL (1)
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::AddFaceQuads (CSegFace *pFace)
{
ENTER (0, 0);
if (gameStates.render.nShadowMap)
	RETVAL (0)

	CFloatVector	vertices [4];
	int32_t			i, j, bAdditive = FaceIsAdditive (pFace);
	CBitmap*			pBm = pFace->m_info.bTextured ? /*pFace->bmTop ? pFace->bmTop :*/ pFace->bmBot : NULL;

if (pBm)
	pBm = pBm->Override (BitmapFrame (pBm, pFace->m_info.nBaseTex, pFace->m_info.nSegment));
#if DBG
if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
	BRP;
#endif
for (i = 0, j = pFace->m_info.nIndex; i < 4; i++, j++) {
#if 1
	transformation.Transform (vertices [i], *(reinterpret_cast<CFloatVector*> (FACES.vertices + j)), 0);
#else
	if (automap.Active ())
		transformation.Transform(vertices [i], gameData.segData.fVertices [pFace->m_info.index [i]], 0);
	else
		vertices [i].Assign (RENDERPOINTS [pFace->m_info.index [i]].m_vertex [1]);
#endif
	}
RETVAL (AddPoly (pFace, NULL, pBm,
					  vertices, 4, FACES.texCoord + pFace->m_info.nIndex,
					  FACES.color + pFace->m_info.nIndex,
					  NULL, 4, (pBm != NULL) && !bAdditive, GL_TRIANGLE_FAN, GL_REPEAT,
					  bAdditive, pFace->m_info.nSegment) > 0)
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::AddSprite (CBitmap *pBm, const CFixVector& position, CFloatVector *color,
														int32_t nWidth, int32_t nHeight, char nFrame, char bAdditive, float fSoftRad)
{
ENTER (0, 0);
if (gameStates.render.nShadowMap)
	RETVAL (0)

	CTranspSprite	item;

item.pBm = pBm;
if ((item.bColor = (color != NULL)))
	item.color = *color;
item.nWidth = nWidth;
item.nHeight = nHeight;
item.nFrame = nFrame;
item.bAdditive = bAdditive;
item.fSoftRad = fSoftRad;
item.position.Assign (position);
if (bAdditive & 3)
	m_data.bRenderGlow = 1;
if (gameOpts->SoftBlend (SOFT_BLEND_SPRITES) != 0)
	m_data.bSoftBlend = 1;
RETVAL (Add (&item, position))
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::AddSpark (const CFixVector& position, char nType, int32_t nSize, char nFrame, char nRotFrame, char nOrient)
{
#if 1
ENTER (0, 0);
if (gameStates.render.nShadowMap)
	RETVAL (0)

	CTranspSpark	item;
//	CFixVector		vPos;

item.nSize = nSize;
item.nFrame = nFrame;
item.nRotFrame = nRotFrame;
item.nOrient = nOrient;
item.nType = nType;
item.position.Assign (position);
if (gameOpts->SoftBlend (SOFT_BLEND_PARTICLES))
	m_data.bSoftBlend = 1;
RETVAL (Add (&item, position))
#else
RETVAL (0)
#endif
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::AddSphere (tTranspSphereType nType, float red, float green, float blue, float alpha,  CObject *pObj, char bAdditive, fix nSize)
{
ENTER (0, 0);
if (gameStates.render.nShadowMap)
	RETVAL (0)

	CTranspSphere	item;
	//CFixVector		vPos;

item.nType = nType;
item.color.Red () = red;
item.color.Green () = green;
item.color.Blue () = blue;
item.color.Alpha () = alpha;
item.nSize = nSize;
item.bAdditive = bAdditive;
item.pObj = pObj;
item.pPulse = gameData.renderData.shield->GetPulse ();
item.pBitmap = gameData.renderData.shield->GetBitmap ();
if (nType != riMonsterball)
	m_data.bRenderGlow = 1;
//transformation.Transform (vPos, pObj->info.position.vPos, 0);
RETVAL (Add (&item, pObj->info.position.vPos))
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::AddParticle (CParticle *particle, float fBrightness, int32_t nThread)
{
ENTER (0, 0);
if (gameStates.render.nShadowMap)
	RETVAL (0)

	CTranspParticle	item;
//	fix					z;

if ((particle->m_nType < 0) || (particle->m_nType >= PARTICLE_TYPES))
	RETVAL (0)
item.particle = particle;
item.fBrightness = fBrightness;
//particle->Transform (gameStates.render.bPerPixelLighting == 2);
if (gameOpts->SoftBlend (SOFT_BLEND_PARTICLES))
	m_data.bSoftBlend = 1;
RETVAL (Add (&item, particle->m_vPos, 10))
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::AddLightning (CLightning *pLightning, int16_t nDepth)
{
ENTER (0, 0);
if (gameStates.render.nShadowMap)
	RETVAL (0)

	CTranspLightning	item;
	bool					bSwap;
	//CFixVector			vPos;

item.lightning = pLightning;
item.nDepth = nDepth;
#if 0
transformation.Transform (vPos, pLightning->m_vPos, 0);
z = vPos.dir.coord.z;
transformation.Transform (vPos, pLightning->m_vEnd, 0);
if (z < vPos.dir.coord.z)
	z = vPos.dir.coord.z;
#endif
fix d1 = Depth (pLightning->m_vPos, false);
fix d2 = Depth (pLightning->m_vEnd, false);
if ((bSwap = (d1 < d2)))
	::Swap (d1, d2);
if (d2 > m_data.zMax)
	RETVAL (0)
if (d1 < m_data.zMin)
	RETVAL (0)
m_data.bRenderGlow = 1;
if (!Add (&item, bSwap ? pLightning->m_vEnd : pLightning->m_vPos /*CFixVector::Avg (pLightning->m_vPos, pLightning->m_vEnd)*/, 0, true, 0)) // -1))
	RETVAL (0)
RETVAL (1)
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::AddLightTrail (CBitmap *pBm, CFloatVector *vThruster, tTexCoord2f *tcThruster, CFloatVector *vFlame, tTexCoord2f *tcFlame, CFloatVector *pColor)
{
ENTER (0, 0);
if (gameStates.render.nShadowMap)
	RETVAL (0)

	CTranspLightTrail	item;
	int32_t					i, j, iMin = 0;
	CFixVector			v;
	float					d, dMin = 1e30f;

item.pBm = pBm;
if ((item.bTrail = (vFlame != NULL))) {
	memcpy (item.vertices, vFlame, 4 * sizeof (CFloatVector));
	memcpy (item.texCoord, tcFlame, 4 * sizeof (tTexCoord2f));
	j = 4;
	}
else
	j = 0;
memcpy (item.vertices + j, vThruster, 4 * sizeof (CFloatVector));
memcpy (item.texCoord + j, tcThruster, 4 * sizeof (tTexCoord2f));
item.color = *pColor;
for (i = 0; i < j; i++) {
	d = Depth (item.vertices [i], true);
	if (dMin > d) {
		dMin = d;
		iMin = i;
		}
	}
v.Assign (item.vertices [iMin]);
m_data.bRenderGlow = 1;
RETVAL (Add (&item, v, 0, false, 0))
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::AddThruster (CObject* pObj, tThrusterInfo* pInfo, int32_t nThruster)
{
ENTER (0, 0);
if (gameStates.render.nShadowMap)
	RETVAL (0)

	CTranspThruster item;

item.pObj = pObj;
item.info = *pInfo;
item.nThruster = nThruster;
m_data.bRenderGlow = 1;
RETVAL (Add (&item, pInfo->vPos [nThruster], 0, false, 0))
}

//------------------------------------------------------------------------------

inline void CTransparencyRenderer::ResetBitmaps (void)
{
ENTER (0, 0);
m_data.pBm [0] =
m_data.pBm [1] =
m_data.pBm [2] = NULL;
m_data.bDecal = 0;
m_data.bTextured = 0;
m_data.nFrame = -1;
m_data.bUseLightmaps = 0;
RETURN
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::LoadTexture (CSegFace* pFace, CBitmap *pBm, int16_t nTexture, int32_t nFrame, int32_t bDecal, int32_t bLightmaps, int32_t nWrap)
{
ENTER (0, 0);
if (pBm) {
#if 0
	ogl.SelectTMU (GL_TEXTURE0 + bLightmaps, true);
	ogl.SetTexturing (true);
#endif
	if ((pBm != m_data.pBm [bDecal]) || ((nFrame = BitmapFrame (pBm, nTexture, pFace ? pFace->m_info.nSegment : -1, nFrame)) != m_data.nFrame) || (nWrap != m_data.nWrap)) {
		gameData.renderData.nStateChanges++;
		if (pBm) {
			if (pBm->Bind (1)) {
				ResetBitmaps ();
				RETVAL (0)
				}
			if (bDecal != 2)
				pBm = pBm->Override (nFrame);
			pBm->Texture ()->Wrap (nWrap);
			m_data.nWrap = nWrap;
			m_data.nFrame = nFrame;
			}
		else
			ogl.BindTexture (0);
		m_data.pBm [bDecal] = pBm;
		}
	}
else {
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
	ResetBitmaps ();
	}
RETVAL (1)
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::FlushSparkBuffer (void)
{
ENTER (0, 0);
if (DepthBuffer () < 0)
	RETURN

if (!sparkBuffer.nSparks)
	RETURN

sparkArea.Reset ();

	int32_t bSoftBlend = SoftBlend (SOFT_BLEND_PARTICLES);

ogl.ResetClientStates (1);
m_data.pBm [1] = m_data.pBm [2] = NULL;
m_data.bUseLightmaps = 0;
ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
if (LoadTexture (NULL, sparks.Bitmap (), 0, 0, 0, 0, GL_CLAMP)) {
	if (!(bSoftBlend && glareRenderer.LoadShader (3, 1)))
		shaderManager.Deploy (-1, true);
	ogl.SetBlendMode (OGL_BLEND_ADD);
	glColor3f (1,1,1);
	OglTexCoordPointer (2, GL_FLOAT, sizeof (tSparkVertex), &sparkBuffer.vertices [0].texCoord);
	OglVertexPointer (3, GL_FLOAT, sizeof (tSparkVertex), &sparkBuffer.vertices [0].vPos);
	OglDrawArrays (GL_QUADS, 0, 4 * sparkBuffer.nSparks);
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
	ogl.SetDepthTest (true);
	sparkBuffer.nSparks = 0;
	}
RETURN
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::FlushParticleBuffer (int32_t nType)
{
ENTER (0, 0);
if (DepthBuffer () < 0)
	RETURN
if (!HeapSize ())
	RETURN

if ((nType < 0) || ((nType != tiParticle) && (particleManager.LastType () >= 0))) {
	ResetBitmaps ();
	if (sparkBuffer.nSparks && !USE_PARTICLE_SHADER && particleManager.Overlap (sparkArea))
		FlushSparkBuffer ();
	if (particleManager.Flush (-1.0f, true)) {
		if (nType < 0)
			particleManager.CloseBuffer ();
		particleManager.SetLastType (-1);
		m_data.bClientColor = 1;
		sparkBuffer.nSparks = 0;
		ResetBitmaps ();
		}
	}
RETURN
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::FlushBuffers (int32_t nType, CTranspItem *item)
{
ENTER (0, 0);
if (DepthBuffer () < 0)
	RETURN
if (!HeapSize ())
	RETURN

if (glowRenderer.Available (GLOW_LIGHTNING | GLOW_SHIELDS | GLOW_SPRITES | GLOW_THRUSTERS) && 
	 (nType != tiLightning) && (nType != tiSphere) && (nType != tiSprite) && (nType != tiThruster)) {
	if (glowRenderer.End ())
		ResetBitmaps ();
	}
if (USE_PARTICLE_SHADER) {
	if ((nType != tiSpark) && (nType != tiParticle)) {
		//FlushSparkBuffer ();
		FlushParticleBuffer (nType);
		}
	}	
else {
	if (nType == tiSpark) {
		if (!item || particleManager.Overlap (((CTranspSpark*) item)->position, X2F (((CTranspSpark*) item)->nSize)))
			FlushParticleBuffer (nType);
		}
	else if (nType == tiParticle) {
		if (!item || sparkArea.Overlap (((CTranspParticle*) item)->particle->Posf (), ((CTranspParticle*) item)->particle->Rad ()))
			FlushSparkBuffer ();
		}	
	else {
		FlushSparkBuffer ();
		FlushParticleBuffer (nType);
		}
	}
RETURN
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::RenderItem (CTranspItem *pItem)
{
ENTER (0, 0);
if (!pItem->bRendered) {
	//pItem->bRendered = true;
	m_data.nPrevType = m_data.nCurType;
	m_data.nCurType = pItem->Type ();
#if DBG
	if (gameOpts->render.debug.bTextures && gameOpts->render.debug.bWalls)
#endif
	try {
		//if ((m_data.nCurType != tiSphere) && (m_data.nPrevType == tiSphere)) // spheres somehow mess up the glow renderer; I cannot determine why though
		//	glowRenderer.End ();
		FlushBuffers (m_data.nCurType, pItem);
		ogl.SetBlendMode (OGL_BLEND_ALPHA);
		ogl.SetDepthWrite (false);
		ogl.SetDepthMode (GL_LEQUAL);
		ogl.SetDepthTest (true);
		pItem->Render ();
		}
	catch(...) {
		PrintLog (0, "invalid transparent render item (type: %d)\n", m_data.nCurType);
		}
	}
RETVAL (m_data.nCurType)
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::NeedDepthBuffer (void)
{
if (!gameOpts->render.bUseShaders)
	return 0;
if (!gameOpts->render.effects.bEnabled)
	return 0;
if (!ogl.m_states.bGlowRendering)
	return true;
if (gameStates.render.cameras.bActive && !gameOpts->render.cameras.bHires)
	return 0;
return int32_t (m_data.bRenderGlow || m_data.bSoftBlend /*|| postProcessManager.HaveEffects ()*/);
}

//------------------------------------------------------------------------------

int32_t CTransparencyRenderer::SoftBlend (int32_t nFlag)
{
return m_data.bHaveDepthBuffer && !gameStates.render.cameras.bActive && (gameOpts->SoftBlend (nFlag) != 0);
}

//------------------------------------------------------------------------------

void CTransparencyRenderer::RenderBuffer (CTranspItemBuffers& buffer, bool bCleanup)
{
ENTER (0, 0);
CTranspItem* pCurrent = *buffer.pBuffer, * pNext, * pPrev;
if (bCleanup)
	*buffer.pBuffer = NULL;
pPrev = NULL;
do {
#if DBG
	if (pCurrent->nItem == nDbgItem)
		BRP;
#endif
	buffer.nItems [0]--;
#if 0
	if ((ogl.m_data.xStereoSeparation < 0) /*|| (pCurrent->Type () != tiPoly)*/)
#endif
	RenderItem (pCurrent);

	pNext = pCurrent->nextItemP;
	if (bCleanup)
		pCurrent->nextItemP = NULL;
	else if (pCurrent->bTransformed) {	// remove items that have transformed coordinates when stereo rendering since these items will be reentered with different coordinates
		int32_t nType = pCurrent->Type ();
		pCurrent->nextItemP = buffer.freeList [nType];
		buffer.freeList [nType] = pCurrent;
		if (pPrev)
			pPrev->nextItemP = pNext;
		else
			*buffer.pBuffer = pNext;
		}
	else
		pPrev = pCurrent;
	pCurrent = pNext;
	} while (pCurrent);
RETURN
}

//------------------------------------------------------------------------------

extern int32_t bLog;

void CTransparencyRenderer::Render (int32_t nWindow)
{
ENTER (0, 0);
#if RENDER_TRANSPARENCY
	int32_t			bStencil, bGlow;
	bool				bCleanup = !LAZY_RESET || (ogl.StereoSeparation () >= 0) || nWindow;

if (!AllocBuffers ())
	RETURN
if (!HeapSize ())
	RETURN
if (gameStates.render.cameras.bActive)
	nWindow = nWindow;
//HUDMessage (0, "transp. render heap size: %d.%03d.%03d", HeapSize () / 1000000, (HeapSize () % 1000000) / 1000, HeapSize () % 1000);

PROF_START
gameStates.render.nType = RENDER_TYPE_TRANSPARENCY;
ogl.ChooseDrawBuffer ();

shaderManager.Deploy (-1);
bStencil = ogl.StencilOff ();
bGlow = gameOpts->render.effects.bGlow;
if (nWindow)
	gameOpts->render.effects.bGlow = 0;
ResetBitmaps ();
m_data.bReady = 0;
m_data.bTextured = -1;
m_data.bUseLightmaps = 0;
m_data.bDecal = 0;
m_data.bLightmaps = lightmapManager.HaveLightmaps ();
m_data.bSplitPolys = (gameStates.render.bPerPixelLighting != 2) && (gameStates.render.bSplitPolys > 0);
m_data.nWrap = 0;
m_data.nFrame = -1;
m_data.pBm [0] =
m_data.pBm [1] = NULL;
if (glowRenderer.Available (0xFFFFFFFF))
	glowRenderer.End ();
else
	m_data.bRenderGlow = 0;
if (gameOptions [0].render.nQuality < 3)
	m_data.bSoftBlend = 0;
sparkBuffer.nSparks = 0;
ogl.DisableLighting ();
ogl.ResetClientStates ();
m_data.bHaveParticles = particleImageManager.LoadAll ();
ogl.SetBlendMode (OGL_BLEND_ALPHA);
ogl.SetDepthMode (GL_LEQUAL);
ogl.SetDepthWrite (false);
ogl.SetFaceCulling (true);
m_data.bHaveDepthBuffer = NeedDepthBuffer () && ogl.CopyDepthTexture (1/*, GL_TEXTURE1, 1*/);
particleManager.BeginRender (-1, 1);
m_data.nCurType = -1;

int32_t h = -1, nBuffers = 0;

for (int32_t i = 0; i < gameStates.app.nThreads; i++)
	if (m_data.buffers [i].nItems [0]) {
		if (h < 0)
			h = i;
		nBuffers++;
		}

if (nBuffers < 2) {
	CTranspItemBuffers& buffer = m_data.buffers [h];

	m_data.buffers [h].nItems [1] = m_data.buffers [h].nItems [0];
	for (buffer.pBuffer = &buffer.depthBuffer [buffer.nMaxOffs]; buffer.nItems [0] && (buffer.pBuffer >= buffer.depthBuffer.Buffer ()); buffer.pBuffer--)
		if (*buffer.pBuffer)
			RenderBuffer (buffer, bCleanup);
	}
else {
	CTranspItemBuffers* buffers [MAX_THREADS];
	
	nBuffers = 0;
	for (int32_t i = 0; i < gameStates.app.nThreads; i++)
		if (m_data.buffers [i].nItems [0]) {
			buffers [nBuffers] = &m_data.buffers [i];
			buffers [nBuffers]->pBuffer = &m_data.buffers [i].depthBuffer [buffers [nBuffers]->nMaxOffs];
			m_data.buffers [i].nItems [1] = m_data.buffers [i].nItems [0];
			nBuffers++;
			}

	while (nBuffers > 0) {
		for (int32_t i = 0; i < nBuffers; i++) {
			if (buffers [i]->pBuffer) {
				if (*buffers [i]->pBuffer) 
					RenderBuffer (*buffers [i], bCleanup);
				if (!buffers [i]->nItems [0] || (--buffers [i]->pBuffer < buffers [i]->depthBuffer.Buffer ())) {
					if (i < --nBuffers)
						buffers [i--] = buffers [nBuffers];
					}
				}
			}
		}
	}


shaderManager.Deploy (-1);
ogl.ResetClientStates ();
ogl.SetTexturing (false);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
ogl.SetDepthWrite (false);
ogl.SetDepthMode (GL_LEQUAL);
ogl.SetDepthTest (true);

glowRenderer.End ();
FlushBuffers (-1);
particleManager.EndRender ();

shaderManager.Deploy (-1);
ogl.ResetClientStates ();
ogl.SetTexturing (false);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
ogl.SetDepthMode (GL_LEQUAL);
ogl.SetDepthWrite (true);
ogl.StencilOn (bStencil);

if (bCleanup) {
	ResetFreeList ();
	for (int32_t i = 0; i < gameStates.app.nThreads; i++) {
#if 0
		for (int32_t j = 0, l = int32_t (m_data.buffers [i].depthBuffer.Length ()); j < l; j++) {
			if (m_data.buffers [i].depthBuffer [j]) {
				m_data.buffers [i].depthBuffer [j] = NULL;
				}
			}
#endif
		m_data.buffers [i].nItems [0] = 0;
		m_data.buffers [i].nMinOffs = ITEM_DEPTHBUFFER_SIZE;
		m_data.buffers [i].nMaxOffs = 0;
		m_data.buffers [i].nHeapSize = 0;
		}
	}
else {
	for (int32_t i = 0; i < gameStates.app.nThreads; i++)
		m_data.buffers [i].nItems [0] = m_data.buffers [i].nItems [1];
	}
gameOpts->render.effects.bGlow = bGlow;

PROF_END(ptTranspPolys)
#endif
RETURN
}

//------------------------------------------------------------------------------
//eof
