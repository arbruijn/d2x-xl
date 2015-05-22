#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "descent.h"
#include "network.h"
#include "interp.h"
#include "ogl_lib.h"
#include "hiresmodels.h"
#include "renderlib.h"
#include "transprender.h"
#include "thrusterflames.h"
#include "addon_bitmaps.h"
#include "../effects/glow.h"

#ifndef fabsf
#	define fabsf(_f)	(float) fabs (_f)
#endif

CThrusterFlames thrusterFlames;

static tTexCoord2f tcCap [2][4] = {
	{{{0.0f,0.0f}},{{0.5f,0.0f}},{{0.5f,0.49609375f}},{{0.0f,0.49609375f}}},
	{{{0.5f,0.0f}},{{1.0f,0.0f}},{{1.0f,0.49609375f}},{{0.5f,0.49609375f}}}
	};

// -----------------------------------------------------------------------------

static CFloatVector	vRingVerts [RING_SEGS] = {
	CFloatVector::Create (-0.5f, -0.5f, 0.0f, 1.0f),
	CFloatVector::Create (-0.6533f, -0.2706f, 0.0f, 1.0f),
	CFloatVector::Create (-0.7071f, 0.0f, 0.0f, 1.0f),
	CFloatVector::Create (-0.6533f, 0.2706f, 0.0f, 1.0f),
	CFloatVector::Create (-0.5f, 0.5f, 0.0f, 1.0f),
	CFloatVector::Create (-0.2706f, 0.6533f, 0.0f, 1.0f),
	CFloatVector::Create (0.0f, 0.7071f, 0.0f, 1.0f),
	CFloatVector::Create (0.2706f, 0.6533f, 0.0f, 1.0f),
	CFloatVector::Create (0.5f, 0.5f, 0.0f, 1.0f),
	CFloatVector::Create (0.6533f, 0.2706f, 0.0f, 1.0f),
	CFloatVector::Create (0.7071f, 0.0f, 0.0f, 1.0f),
	CFloatVector::Create (0.6533f, -0.2706f, 0.0f, 1.0f),
	CFloatVector::Create (0.5f, -0.5f, 0.0f, 1.0f),
	CFloatVector::Create (0.2706f, -0.6533f, 0.0f, 1.0f),
	CFloatVector::Create (0.0f, -0.7071f, 0.0f, 1.0f),
	CFloatVector::Create (-0.2706f, -0.6533f, 0.0f, 1.0f)
};

//static int32_t nStripIdx [] = {0,15,1,14,2,13,3,12,4,11,5,10,6,9,7,8};

void CThrusterFlames::Create (void)
{
if (!m_bHaveFlame [m_bPlayer]) {
		CFloatVector*	pv;
		int32_t				i, j, m, n;
		double			phi, sinPhi;
		float				z = 0,
							fScale = 2.0f / 3.0f,
							fStep [2] = {1.0f / 4.0f, 1.0f / 3.0f};

	// first part with increasing diameter
	pv = &m_vFlame [0][0];
	for (i = 0, phi = 0; i < 5; i++, phi += PI / 10, z -= fStep [0]) {
		sinPhi = (1 + sin (phi) / 2) * fScale;
		for (j = 0; j < RING_SEGS; j++, pv++) {
			*pv = vRingVerts [j] * float (sinPhi);
			(*pv).v.coord.z = z;
			}
		}
	// second part with decreasing diameter
	m = n = THRUSTER_SEGS - i + 1;
	for (phi = PI / 2; i < THRUSTER_SEGS; i++, phi += PI / 12, z -= fStep [1], m--) {
		sinPhi = (1 + sin (phi) / 2) * fScale /** m / n*/;
		for (j = 0; j < RING_SEGS; j++, pv++) {
			*pv = vRingVerts [j] * float (sinPhi);
			(*pv).v.coord.z = z;
			}
		}

	tTexCoord2f	tTexCoord2fl, tTexCoord2flStep = {{0.5f / RING_SEGS, 0.45f / THRUSTER_SEGS}};
	tTexCoord2f* flameTexCoord = m_flameTexCoord [m_bPlayer];
	float uOffset = m_bPlayer ? 0.5f : 0.0f;

	int32_t nVerts = 0;
	for (i = 0; i < THRUSTER_SEGS - 1; i++) {
		for (j = 0; j <= RING_SEGS; j++) {
			m = j % RING_SEGS;
			tTexCoord2fl.v.u = uOffset + j * tTexCoord2flStep.v.u;
			for (n = 0; n < 2; n++) {
				tTexCoord2fl.v.v = 0.5f + tTexCoord2flStep.v.v * (i + n);
				flameTexCoord [nVerts] = tTexCoord2fl;
				m_flameVerts [nVerts++] = m_vFlame [i + n][m];
				}
			}
		}
	m_bHaveFlame [m_bPlayer] = true;
	}
}

// -----------------------------------------------------------------------------

void CThrusterFlames::CalcPosOnShip (CObject *pObj, CFixVector *vPos)
{
	tObjTransformation	*pPos = OBJPOS (pObj);

if (gameOpts->render.bHiresModels [0]) {
	vPos [0] = pPos->vPos + pPos->mOrient.m.dir.f * (-pObj->info.xSize);
	vPos [0] += pPos->mOrient.m.dir.r * (-(8 * pObj->info.xSize / 44));
	vPos [1] = vPos [0] + pPos->mOrient.m.dir.r * (8 * pObj->info.xSize / 22);
	}
else {
	vPos [0] = pPos->vPos + pPos->mOrient.m.dir.f * (-pObj->info.xSize / 10 * 9);
	if (gameStates.app.bFixModels)
		vPos [0] += pPos->mOrient.m.dir.u * (pObj->info.xSize / 40);
	else
		vPos [0] += pPos->mOrient.m.dir.u * (-pObj->info.xSize / 20);
	vPos [1] = vPos [0];
	vPos [0] += pPos->mOrient.m.dir.r * (-8 * pObj->info.xSize / 49);
	vPos [1] += pPos->mOrient.m.dir.r * (8 * pObj->info.xSize / 49);
	}
}

// -----------------------------------------------------------------------------

int32_t CThrusterFlames::CalcPos (CObject *pObj, tThrusterInfo* tiP, int32_t bAfterburnerBlob)
{
if (!tiP)
	tiP = &m_ti;

	tThrusterInfo	ti = *tiP;
	int32_t			i, bMissile = pObj->IsMissile ();

m_pData = NULL;
ti.pp = NULL;
ti.pThrusters = gameData.models.thrusters + pObj->ModelId ();
m_nThrusters = ti.pThrusters->nCount;
if (gameOpts->render.bHiresModels [0] && (pObj->info.nType == OBJ_PLAYER) && !GetASEModel (pObj->ModelId ())) {
	if (!m_bSpectate) {
		m_pData = gameData.render.thrusters + pObj->info.nId;
		ti.pp = m_pData->path.GetPoint ();
		}
	ti.fLength [0] =
	ti.fLength [1] = ti.fScale;
	ti.fSize [0] =
	ti.fSize [1] = (ti.fLength [0] + 1) / 2;
	ti.nType [0] = 
	ti.nType [1] = 1;
	m_nThrusters = 2;
	CalcPosOnShip (pObj, ti.vPos);
	ti.pThrusters = NULL;
	}
else if (bAfterburnerBlob || (bMissile && !m_nThrusters)) {
		tHitbox	*pHitbox = &gameData.models.hitboxes [pObj->ModelId ()].hitboxes [0];
		fix		nObjRad = (pHitbox->vMax.v.coord.z - pHitbox->vMin.v.coord.z) / 2;

	if (bAfterburnerBlob)
		nObjRad *= 2;
	if (pObj->info.nId == EARTHSHAKER_ID)
		ti.fSize [0] = 1.0f;
	else if ((pObj->info.nId == MEGAMSL_ID) || (pObj->info.nId == EARTHSHAKER_MEGA_ID))
		ti.fSize [0] = 0.8f;
	else if (pObj->info.nId == SMARTMSL_ID)
		ti.fSize [0] = 0.6f;
	else
		ti.fSize [0] = 0.5f;
	ti.fLength [0] = ti.fSize [0] * (1.0f + ti.fScale);
	m_nThrusters = 1;
	if (EGI_FLAG (bThrusterFlames, 1, 1, 0) == 2)
		ti.fLength [0] /= 2;
	if (!gameData.models.vScale.IsZero ())
		ti.vPos [0] *= gameData.models.vScale;
	*ti.vPos = pObj->info.position.vPos + pObj->info.position.mOrient.m.dir.f * (-nObjRad);
	ti.nType [0] = 
	ti.nType [1] = 1;
	ti.pThrusters = NULL;
	}
else if ((pObj->info.nType == OBJ_PLAYER) ||
			((pObj->info.nType == OBJ_ROBOT) && !pObj->cType.aiInfo.CLOAKED) ||
			bMissile) {
	CFixMatrix	mView, *pView;
	if (!m_bSpectate && (pObj->info.nType == OBJ_PLAYER)) {
		m_pData = gameData.render.thrusters + pObj->info.nId;
		ti.pp = m_pData->path.GetPoint ();
		}
	if (!m_nThrusters) {
		if (pObj->info.nType != OBJ_PLAYER)
			return 0;
		if (!m_bSpectate) {
			m_pData = gameData.render.thrusters + pObj->info.nId;
			ti.pp = m_pData->path.GetPoint ();
			}
		ti.fLength [0] =
		ti.fLength [1] = ti.fScale;
		ti.fSize [0] = 
		ti.fSize [1] = (ti.fLength [0] + 1) / 2;
		ti.nType [0] = 
		ti.nType [1] = 1;
		m_nThrusters = 2;
		CalcPosOnShip (pObj, ti.vPos);
		}
	else {
		tObjTransformation *pPos = OBJPOS (pObj);
		if (SPECTATOR (pObj)) {
			pView = &mView;
			mView = pPos->mOrient.Transpose ();
			}
		else
			pView = pObj->View (0);
		for (i = 0; i < m_nThrusters; i++) {
			ti.fSize [i] = ti.pThrusters->fSize [i];
			ti.nType [i] = ti.pThrusters->nType [i];
			float h = ((ti.nType [i] & (REAR_THRUSTER | FRONTAL_THRUSTER)) == (REAR_THRUSTER | FRONTAL_THRUSTER)) ? ti.fScale : 1.5f;
			if (m_nStyle == 1)
				h *= 4.0f;
			else
				h *= 0.75f;
			ti.fLength [i] = ti.fSize [i] * h;
			ti.vPos [i] = *pView * ti.pThrusters->vPos [i];
			if (!gameData.models.vScale.IsZero ())
				ti.vPos [i] *= gameData.models.vScale;
			ti.vPos [i] += pPos->vPos;
			ti.vDir [i] = *pView * ti.pThrusters->vDir [i];
			//CAngleVector a1 = pObj->info.position.mOrient.m.v.f.ToAnglesVec ();
			CFixVector v = ti.pThrusters->vDir [i];
			CAngleVector a = v.ToAnglesVec ();
			//CAngleVector a;
			//a [PA] = a1 [PA] - a2 [PA];
			//a [BA] = a1 [BA] - a2 [BA];
			//a [HA] = a1 [HA] - a2 [HA];
			ti.mRot [i] = CFixMatrix::Create (a);
			ti.nType [i] = ti.pThrusters->nType [i];
			}
		if (bMissile)
			m_nThrusters = 1;
		}
	}
else
	return 0;
*tiP = ti;
return m_nThrusters;
}

// -----------------------------------------------------------------------------

void CThrusterFlames::Render2D (CFixVector& vPos, CFixVector &vDir, float fSize, float fLength, CFloatVector *pColor)
{
if (ogl.StereoDevice () && (ogl.StereoSeparation () >= 0))
	return;

	static tTexCoord2f tcTrail [2][4] = {
		{{{0.25f,0.49609375f}},{{0.25f,0.0f}},{{0.5f,0.0f}},{{0.5f,0.49609375f}}},
		{{{0.75f,0.49609375f}},{{0.75f,0.0f}},{{1.0f,0.0f}},{{1.0f,0.49609375f}}}
		};
	static CFloatVector	vEye;

	CFloatVector	v, vPosf, vNormf, vTrail [4], vCap [4], vTrailTip, vDirf;
	float		/*c = 1.0f 0.7f + 0.03f * fPulse,*/ dotTrail, dotCap;

#if DBG
if (fSize > 5.0f)
	fSize = fSize;
#endif
vDirf.Assign (vDir);
vPosf.Assign (vPos);
vDirf *= fLength;
vTrailTip = vPosf - vDirf;
vEye.Assign (gameData.render.mine.viewer.vPos);
vNormf = CFloatVector::Normal (vTrailTip, vPosf, vEye) * fSize;
vCap [0] = vPosf + vNormf;
vCap [2] = vPosf - vNormf;
vNormf *= 0.7071f;
vTrail [0] = vPosf + vNormf;
vTrail [1] = vPosf - vNormf;
vTrail [2] = vTrail [1] - vDirf;
vTrail [3] = vTrail [0] - vDirf;
vNormf = CFloatVector::Normal (vTrail [0], vTrail [1], vTrailTip) * fSize;
vCap [1] = vPosf + vNormf;
vCap [3] = vPosf - vNormf;
vPosf -= vEye;
CFloatVector::Normalize (vPosf);
v = vTrailTip - vEye;
CFloatVector::Normalize (v);
dotTrail = CFloatVector::Dot (vPosf, v);
v = *vCap - vEye;
CFloatVector::Normalize (v);
dotCap = CFloatVector::Dot (vPosf, v);
transparencyRenderer.AddLightTrail (thruster.Bitmap (), vCap, tcCap [m_bPlayer], (dotTrail < dotCap) ? vTrail : NULL, tcTrail [m_bPlayer], pColor);
}

// -----------------------------------------------------------------------------

void CThrusterFlames::RenderCap (int32_t i)
{
if (thruster.Load ()) {
	CFloatVector	verts [4];
	float				z = (m_vFlame [0][0].v.coord.z + m_vFlame [1][0].v.coord.z) / 2.0f * m_ti.fLength [i];
	float				scale = m_ti.fSize [i] * 1.6666667f;

	// choose 4 vertices from the widest ring of the flame
	for (int32_t i = 0; i < 4; i++) {
		verts [i] = m_vFlame [5][4 * i];
		verts [i].v.coord.x *= scale;
		verts [i].v.coord.y *= scale;
		verts [i].v.coord.z = z;
		}
	if (glowRenderer.SetViewport (GLOW_THRUSTERS, verts, 4) > 0) {
		glColor3f (1,1,1);
		ogl.RenderQuad (thruster.Bitmap (), verts, 3, tcCap [m_bPlayer]);
		ogl.SetTexturing (true);
		thruster.Bitmap ()->Bind (1);
		}
	}
}

// -----------------------------------------------------------------------------

void CThrusterFlames::Render3D (int32_t i)
{
if (glowRenderer.SetViewport (GLOW_THRUSTERS, m_flameVerts, FLAME_VERT_COUNT) > 0) {
	glColor3f (1,1,1);
	ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
	OglTexCoordPointer (2, GL_FLOAT, 0, &m_flameTexCoord [m_bPlayer]);
	OglVertexPointer (3, GL_FLOAT, sizeof (CFloatVector), m_flameVerts);
	glPushMatrix ();
	glScalef (m_ti.fSize [i], m_ti.fSize [i], m_ti.fLength [i]);
	OglDrawArrays (GL_QUAD_STRIP, 0, FLAME_VERT_COUNT);
	glPopMatrix ();
	}
}

// -----------------------------------------------------------------------------

bool CThrusterFlames::Setup (CObject *pObj, int32_t nStages)
{
if (gameStates.app.bNostalgia)
	return false;
if (!gameOpts->render.effects.bEnabled)
	return false;
if (SHOW_SHADOWS && (gameStates.render.nShadowPass == 2))
	return false;
if (!(m_nStyle = EGI_FLAG (bThrusterFlames, 1, 1, 0)))
	return false;
if ((pObj->info.nType == OBJ_PLAYER) && (PLAYER (pObj->info.nId).flags & PLAYER_FLAGS_CLOAKED))
	return false;

bool bFallback;

m_bPlayer = (pObj->info.nType == OBJ_PLAYER);
m_bSpectate = SPECTATOR (pObj);

if (nStages & 2) {
	if (thruster.Load ()) 
		bFallback = false;
	else {
		m_nStyle ^= 3;	// fall back to other style
		if (!thruster.Load ()) {
			extraGameInfo [IsMultiGame].bThrusterFlames = 0;	// didn't work either
			return false;
			}
		bFallback = true;
		}

	if (m_nStyle == 2) {
		ogl.SetTexturing (true);
		ogl.SelectTMU (GL_TEXTURE0);
		thruster.Bitmap ()->SetTranspType (-1);
		if (thruster.Bitmap ()->Bind (1)) {
			extraGameInfo [IsMultiGame].bThrusterFlames = bFallback ? 0 : 1;
			return false;
			}
		thruster.Texture ()->Wrap (GL_CLAMP);
		}
	}

if (nStages & 1) {
	float fSpeed = X2F (pObj->mType.physInfo.velocity.Mag ());
	if (m_pData)
		m_pData->fSpeed = fSpeed;

	m_ti.pp = NULL;
	m_ti.fScale = fSpeed / float (pObj->MaxSpeedScaled ()) + 0.5f;
	//if (m_ti.fScale < m_ti.fSize / 2)
	//	m_ti.fScale = m_ti.fSize / 2;
	m_ti.fScale += float (Rand (100)) / 1000.0f;
	if (!CalcPos (pObj))
		return false;
	}

return true;
}

// -----------------------------------------------------------------------------

bool CThrusterFlames::IsFiring (CWeaponState* ws, int32_t i)
{
if (!ws)
	return true;
for (int32_t j = 0; j < (int32_t) sizeofa (ws->nThrusters); j++)
	if (ws->nThrusters [j] && ((m_ti.nType [i] & ws->nThrusters [j]) == m_ti.nType [i]))
		return true;
return false;
}

// -----------------------------------------------------------------------------

void CThrusterFlames::Render (CObject *pObj, tThrusterInfo* pInfo, int32_t nThruster)
{
int32_t bStencil = ogl.StencilOff ();

CWeaponState* ws = (pObj->info.nType == OBJ_PLAYER) ? gameData.multiplayer.weaponStates + pObj->info.nId : NULL;

if (m_nStyle == 1) {	//2D
	if (!Setup (pObj))
		return;

		static CFloatVector	tcColor = {{{0.75f, 0.75f, 0.75f, 1.0f}}};

	//m_ti.fLength *= 4 * m_ti.fSize;
	for (int32_t i = 0; i < m_nThrusters; i++) {
		if (IsFiring (ws, i)) {
			m_ti.fSize [i] *= ((pObj->info.nType == OBJ_PLAYER) && HaveHiresModel (pObj->ModelId ())) ? 1.2f : 1.5f;
			if (!gameData.models.vScale.IsZero ())
				m_ti.fSize [i] *= X2F (gameData.models.vScale.v.coord.z);
			Render2D (m_ti.vPos [i], m_ti.vDir [i], m_ti.fSize [i], m_ti.fLength [i], &tcColor);
			}
		}
	}
else { //3D
	//m_ti.fLength /= 2;
	if (gameStates.render.nType != RENDER_TYPE_TRANSPARENCY) {
		if (!Setup (pObj, 1))
			return;
		Create ();
		m_ti.mOrient = pObj->info.position.mOrient;
		for (int32_t i = 0; i < m_nThrusters; i++) {
			if (IsFiring (ws, i))
				transparencyRenderer.AddThruster (pObj, &m_ti, i);
			}
		}
	else {
		if (!Setup (pObj, 2))
			return;

		ogl.ResetClientStates (1);
		ogl.SetFaceCulling (false);
		ogl.SetBlendMode (OGL_BLEND_ADD);
		ogl.SetTransform (1);
		ogl.SetDepthWrite (false);

		glowRenderer.Begin (GLOW_THRUSTERS, 2, false, 0.75f);
		m_ti = *pInfo;
		transformation.Begin (m_ti.vPos [nThruster], (m_ti.pp && !m_bSpectate) ? m_ti.pp->mOrient : m_ti.mOrient, __FILE__, __LINE__);
		transformation.Begin (CFixVector::ZERO, m_ti.mRot [nThruster], __FILE__, __LINE__);
		// render a cap for the thruster flame at its base
		RenderCap (nThruster);
		Render3D (nThruster);
		transformation.End (__FILE__, __LINE__);
		transformation.End (__FILE__, __LINE__);
#if 1
		glowRenderer.Done (GLOW_THRUSTERS);
#else
		glowRenderer.End ();
#endif
		}
	ogl.SetTransform (0);
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
	ogl.SetFaceCulling (true);
	OglCullFace (0);
	ogl.SetDepthWrite (true);
	}
ogl.StencilOn (bStencil);
}

//------------------------------------------------------------------------------
//eof
