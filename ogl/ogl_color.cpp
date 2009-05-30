/*
 *
 * Graphics support functions for OpenGL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#	include <io.h>
#endif
#include <math.h>

#include "descent.h"
#include "maths.h"
#include "error.h"
#include "network.h"
#include "light.h"
#include "dynlight.h"
#include "endlevel.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "fvi.h"
#include "palette.h"

#define CHECK_LIGHT_VERT	1
#define BRIGHT_SHOTS			0
#if DBG
#	define USE_FACE_DIST		1
#else
#	define USE_FACE_DIST		0
#endif

#if DBG
#	define ONLY_HEADLIGHT 0
#else
#	define ONLY_HEADLIGHT 0
#endif

#define GEO_LIN_ATT	(/*0.0f*/ gameData.render.fAttScale [0])
#define GEO_QUAD_ATT	(/*0.003333f*/ gameData.render.fAttScale [1])
#define OBJ_LIN_ATT	(/*0.0f*/ gameData.render.fAttScale [0])
#define OBJ_QUAD_ATT	(/*0.003333f*/ gameData.render.fAttScale [1])

//------------------------------------------------------------------------------

tFaceColor lightColor = {{1.0f, 1.0f, 1.0f, 1.0f}, 0};
tFaceColor tMapColor = {{1.0f, 1.0f, 1.0f, 1.0f}, 0};
tFaceColor vertColors [8] = {
 {{1.0f, 1.0f, 1.0f, 1.0f}, 0},
 {{1.0f, 1.0f, 1.0f, 1.0f}, 0},
 {{1.0f, 1.0f, 1.0f, 1.0f}, 0},
 {{1.0f, 1.0f, 1.0f, 1.0f}, 0},
 {{1.0f, 1.0f, 1.0f, 1.0f}, 0},
 {{1.0f, 1.0f, 1.0f, 1.0f}, 0},
 {{1.0f, 1.0f, 1.0f, 1.0f}, 0},
 {{1.0f, 1.0f, 1.0f, 1.0f}, 0}
	};
tRgbaColorf shadowColor [2] = {{1.0f, 0.0f, 0.0f, 80.0f}, {0.0f, 0.0f, 1.0f, 80.0f}};
tRgbaColorf modelColor [2] = {{0.0f, 0.5f, 1.0f, 0.5f}, {0.0f, 1.0f, 0.5f, 0.5f}};

//------------------------------------------------------------------------------

void OglPalColor (CPalette *palette, int c)
{
	tRgbaColorf	color;

if (c < 0)
	glColor3f (1.0, 1.0, 1.0);
else {
	if (!palette)
		palette = paletteManager.Game ();
	palette->ToRgbaf (c, color);
	color.alpha = gameStates.render.grAlpha;
	glColor4fv (reinterpret_cast<GLfloat*> (&color));
	}
}

//------------------------------------------------------------------------------

tRgbaColorf GetPalColor (CPalette *palette, int c)
{
	tRgbaColorf color;

if (c < 0) 
	color.red = color.green = color.blue = color.alpha = 1.0f;
else {
	if (!palette)
		palette = paletteManager.Game ();
	palette->ToRgbaf (c, color);
	color.alpha = gameStates.render.grAlpha;
	}
return color;
}

//------------------------------------------------------------------------------

void OglCanvasColor (tCanvasColor* colorP)
{
	GLfloat	fc [4];

if (!colorP)
	glColor4f (1.0, 1.0, 1.0, gameStates.render.grAlpha);
else if (colorP->rgb) {
	fc [0] = float (colorP->color.red) / 255.0f;
	fc [1] = float (colorP->color.green) / 255.0f;
	fc [2] = float (colorP->color.blue) / 255.0f;
	fc [3] = float (colorP->color.alpha) / 255.0f * gameStates.render.grAlpha;
	if (fc [3] < 1.0f) {
		glEnable (GL_BLEND);
		glBlendFunc (gameData.render.ogl.nSrcBlend, gameData.render.ogl.nDestBlend);
		}
	glColor4fv (fc);
	}
else
	OglPalColor (paletteManager.Game (), colorP->index);
}

//------------------------------------------------------------------------------

tRgbaColorf GetCanvasColor (tCanvasColor *colorP)
{

if (!colorP) {
	tRgbaColorf	color = {1, 1, 1, gameStates.render.grAlpha};
	return color;
	}
else if (colorP->rgb) {
	tRgbaColorf color = {
		float (colorP->color.red) / 255.0f,
		float (colorP->color.green) / 255.0f,
		float (colorP->color.blue) / 255.0f,
		float (colorP->color.alpha) / 255.0f * gameStates.render.grAlpha
		};
	if (colorP->color.alpha < 1.0f) {
		glEnable (GL_BLEND);
		glBlendFunc (gameData.render.ogl.nSrcBlend, gameData.render.ogl.nDestBlend);
		}
	return color;
	}
else
	return GetPalColor (paletteManager.Game (), colorP->index);
}

//------------------------------------------------------------------------------

// cap tMapColor scales the color values in tMapColor so that none of them exceeds
// 1.0 if multiplied with any of the current face's corners' brightness values.
// To do that, first the highest corner brightness is determined.
// In the next step, color values are increased to match the max. brightness.
// Then it is determined whether any color value multiplied with the max. brightness would
// exceed 1.0. If so, all three color values are scaled so that their maximum multiplied
// with the max. brightness does not exceed 1.0.

inline void CapTMapColor (tUVL *uvlList, int nVertices, CBitmap *bm)
{
#if 0
	tFaceColor *color = tMapColor.index ? &tMapColor : lightColor.index ? &lightColor : NULL;

if (! (bm->props.flags & BM_FLAG_NO_LIGHTING) && color) {
		double	a, m = tMapColor.color.red;
		double	h, l = 0;
		int		i;

	for (i = 0; i < nVertices; i++, uvlList++) {
		h = (bm->props.flags & BM_FLAG_NO_LIGHTING) ? 1.0 : X2F (uvlList->l);
		if (l < h)
			l = h;
		}

	// scale brightness with the average color to avoid darkening bright areas with the color
	a = (color->color.red + color->color.green + color->color.blue) / 3;
	if (m < color->color.green)
		m = color->color.green;
	if (m < color->color.blue)
		m = color->color.blue;
	l = l / a;
	// prevent any color component getting over 1.0
	if (l * m > 1.0)
		l = 1.0 / m;
	color->color.red *= l;
	color->color.green *= l;
	color->color.blue *= l;
	}
#endif
}

//------------------------------------------------------------------------------

static inline void ScaleColor (tFaceColor *color, float l)
{
if (l >= 0) {
		float m = color->color.red;

	if (m < color->color.green)
		m = color->color.green;
	if (m < color->color.blue)
		m = color->color.blue;
	if (m > 0.0f) {
		m = l / m;
		color->color.red *= m;
		color->color.green *= m;
		color->color.blue *= m;
		}
	}
}

//------------------------------------------------------------------------------

inline float BC (float c, float b)	//biased contrast
{
return (float) ((c < 0.5) ? pow (c, 1.0f / b) : pow (c, b));
}

void OglColor4sf (float r, float g, float b, float s)
{
if (gameStates.ogl.bStandardContrast)
	glColor4f (r * s, g * s, b * s, gameStates.ogl.fAlpha);
else {
	float c = (float) gameStates.ogl.nContrast - 8.0f;

	if (c > 0.0f)
		c = 1.0f / (1.0f + c * (3.0f / 8.0f));
	else
		c = 1.0f - c * (3.0f / 8.0f);
	glColor4f (BC (r, c) * s, BC (g, c) * s, BC (b, c) * s, gameStates.ogl.fAlpha);
	}
}

//------------------------------------------------------------------------------

/*inline*/
void SetTMapColor (tUVL *uvlList, int i, CBitmap *bmP, int bResetColor, tFaceColor *vertColor)
{
	float l = (bmP->Flags () & BM_FLAG_NO_LIGHTING) ? 1.0f : X2F (uvlList->l);
	float s = 1.0f;

#if SHADOWS
if (gameStates.ogl.bScaleLight)
	s *= gameStates.render.bHeadlightOn ? 0.4f : 0.3f;
#endif
if (gameStates.app.bEndLevelSequence >= EL_OUTSIDE)
	OglColor4sf (l, l, l, s);
else if (vertColor) {
	if (tMapColor.index) {
		ScaleColor (&tMapColor, l);
		vertColor->color = tMapColor.color;
		if (l >= 0)
			tMapColor.color.red =
			tMapColor.color.green =
			tMapColor.color.blue = 1.0;
		}
	else if (i >= (int) (sizeof (vertColors) / sizeof (tFaceColor)))
		return;
	else if (vertColors [i].index) {
			tFaceColor *pvc = vertColors + i;

		vertColor->color = vertColors [i].color;
		if (bResetColor) {
			pvc->color.red =
			pvc->color.green =
			pvc->color.blue = 1.0;
			pvc->index = 0;
			}
		}
	else {
		vertColor->color.red =
		vertColor->color.green =
		vertColor->color.blue = l;
		}
	vertColor->color.alpha = s;
	}
else {
	if (tMapColor.index) {
		ScaleColor (&tMapColor, l);
		OglColor4sf (tMapColor.color.red, tMapColor.color.green, tMapColor.color.blue, s);
		if (l >= 0)
			tMapColor.color.red =
			tMapColor.color.green =
			tMapColor.color.blue = 1.0;
		}
	else if (i >= (int) (sizeof (vertColors) / sizeof (tFaceColor)))
		return;
	else if (vertColors [i].index) {
			tFaceColor *pvc = vertColors + i;

		OglColor4sf (pvc->color.red, pvc->color.green, pvc->color.blue, s);
		if (bResetColor) {
			pvc->color.red =
			pvc->color.green =
			pvc->color.blue = 1.0;
			pvc->index = 0;
			}
		}
	else {
		OglColor4sf (l, l, l, s);
		}
	}
}

//------------------------------------------------------------------------------

//#define G3_DOTF(_v0,_v1)	((_v0) [X] * (_v1) [X] + (_v0) [Y] * (_v1) [Y] + (_v0) [Z] * (_v1) [Z])
/*
#define G3_REFLECT(_vr,_vl,_vn) \
 { \
	float	LdotN = 2 * G3_DOTF(_vl, _vn); \
	(_vr) [X] = (_vn) [X] * LdotN - (_vl) [X]; \
	(_vr) [Y] = (_vn) [Y] * LdotN - (_vl) [Y]; \
	(_vr) [Z] = (_vn) [Z] * LdotN - (_vl) [Z]; \
	}
*/
//------------------------------------------------------------------------------

inline int sqri (int i)
{
return i * i;
}

//------------------------------------------------------------------------------

#if CHECK_LIGHT_VERT

static inline int IsLightVert (int nVertex, CDynLight *prl)
{
if ((nVertex >= 0) && prl->info.faceP) {
	ushort *pv = gameStates.render.bTriangleMesh ? prl->info.faceP->triIndex : prl->info.faceP->index;
	for (int i = prl->info.faceP->nVerts; i; i--, pv++)
		if (*pv == (ushort) nVertex)
			return 1;
	}
return 0;
}

#endif

//------------------------------------------------------------------------------

#define VECMAT_CALLS 0

float fLightRanges [5] = {0.5f, 0.7071f, 1.0f, 1.4142f, 2.0f};

#if 1//def _DEBUG

int G3AccumVertColor (int nVertex, CFloatVector3 *pColorSum, CVertColorData *vcdP, int nThread)
{
	int					i, j, nLights, nType, 
							bSkipHeadlight = gameOpts->ogl.bHeadlight && !gameStates.render.nState,
							bTransform = gameStates.render.nState && !gameStates.ogl.bUseTransform,
							nSaturation = gameOpts->render.color.nSaturation;
	int					nBrightness, nMaxBrightness = 0;
	bool					bInRad;
	float					fLightDist, fAttenuation, spotEffect, NdotL, RdotE, nMinDot;
	CFloatVector3		spotDir, lightDir, lightPos, vertPos, vReflect;
	CFloatVector3		lightColor, colorSum, vertColor = CFloatVector3::Create (0.0f, 0.0f, 0.0f);
	CDynLight*			prl;
	CDynLightIndex*	sliP = &lightManager.Index (0) [nThread];
	CActiveDynLight*	activeLightsP = lightManager.Active (nThread) + sliP->nFirst;
	CVertColorData		vcd = *vcdP;

#if DBG
if (nThread == 0)
	nThread = nThread;
if (nThread == 1)
	nThread = nThread;
#endif
colorSum = *pColorSum;
vertPos = *vcd.vertPosP - *transformation.m_info.posf [1].XYZ ();
vertPos.Neg ();
CFloatVector3::Normalize (vertPos);
nLights = sliP->nActive;
if (nLights > lightManager.LightCount (0))
	nLights = lightManager.LightCount (0);
i = sliP->nLast - sliP->nFirst + 1;
#if DBG
if ((nDbgVertex >= 0) && (nVertex == nDbgVertex))
	nDbgVertex = nDbgVertex;
#endif
for (j = 0; (i > 0) && (nLights > 0); activeLightsP++, i--) {
#if 1
	if (!(prl = activeLightsP->pl))
#else
	if (!(prl = GetActiveRenderLight (activeLightsP, nThread)))
#endif
		continue;
	nLights--;
#if DBG
	if ((nDbgSeg >= 0) && (prl->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (prl->info.nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#	if 0
	else
		continue;
#	endif
#endif
	if (!prl->render.bState)
		continue;
#if 0
	if (i == vcd.nMatLight)
		continue;
#endif
	nType = prl->render.nType;
	if (bSkipHeadlight && (nType == 3))
		continue;
#if ONLY_HEADLIGHT
	if (nType != 3)
		continue;
#endif
	if (prl->info.bVariable && gameData.render.vertColor.bDarkness)
		continue;
	lightColor = *(reinterpret_cast<CFloatVector3*> (&prl->info.color));

#if USE_FACE_DIST
if ((nVertex < 0) && (nType < 2)) {
	bInRad = DistToFace (lightPos, *vcd.vertPosP, prl->info.nSegment, ubyte (prl->info.nSide)) == 0;
	lightDir = lightPos - *vcd.vertPosP;
	fLightDist = lightDir.Mag ();
	if (fLightDist < 0.1f)
		fLightDist = 0.0f;
	else {
		fLightDist *= gameStates.ogl.fLightRange;
		bInRad = false;
		}
	}
else 
#endif
	{
	lightPos = *prl->render.vPosf [bTransform].XYZ ();
	lightDir = lightPos - *vcd.vertPosP;
	fLightDist = lightDir.Mag () * gameStates.ogl.fLightRange;
	bInRad = false;
	}

	if (lightDir.IsZero ())
		lightDir = vcd.vertNorm;
	else
		CFloatVector3::Normalize (lightDir);
	if (bInRad)
		NdotL = 1.0f;
	else if (vcd.vertNorm.IsZero ())
		NdotL = 1.0f;
	else {
		NdotL = CFloatVector3::Dot (vcd.vertNorm, lightDir);
		if (fabs (NdotL) < 1.0e-3f)
			NdotL = 0.0f;
		}
	nMinDot = -0.1f;
	if (gameStates.render.nState || (nType < 2)) {
#if CHECK_LIGHT_VERT == 2
		if (IsLightVert (nVertex, prl))
			fLightDist = 0;
		else
#endif
		if (nType > 1) 
			fLightDist -= prl->info.fRad * gameStates.ogl.fLightRange; //make light brighter close to light source
#if USE_FACE_DIST
		else if (nVertex < 0) {
			if (NdotL < 0.0f) {
				// lights with angles < -15 deg towards this vertex have already been excluded
				// now dim the light if it's falling "backward" using the angle as a scale
				float dot = -6.0f * CFloatVector3::Dot (lightDir, *prl->info.vDirf.XYZ ());
				if (dot <= 0.0f) {
					if (dot < -1.0f)
						continue;
					nMinDot = 0.0f;
					dot += 1.0f;
					fLightDist /= dot;
					}
				}
			}
#endif
		else {
			float dot = CFloatVector3::Dot (lightDir, *prl->info.vDirf.XYZ ());
			float lightRad = prl->info.fRad;
			if (NdotL >= 0.0f) 
				lightRad *= 1.0f - 0.9f * sqrt (fabs (dot));
			else {
				// lights with angles < -15 deg towards this vertex have already been excluded
				// now dim the light if it's falling "backward" using the angle as a scale
				dot *= -6.0f;
				if (dot <= 0.0f) {
					if (dot < -1.0f)
						continue;
					nMinDot = 0.0f;
					dot += 1.0f;
					fLightDist /= dot;
					lightRad *= dot;
					}
				}
			fLightDist -= lightRad * gameStates.ogl.fLightRange; //make light darker if face behind light source
			}
		if (fLightDist < 0.0f)
			fLightDist = 0.0f;
		}
	if	(((NdotL >= nMinDot) && (fLightDist <= 0.0f)) || IsLightVert (nVertex, prl)) {
		bInRad = true;
		NdotL = 1.0f;
		fLightDist = 0.0f;
		fAttenuation = 1.0f / prl->info.fBrightness;
		}
	else {	//make it decay faster
#if BRIGHT_SHOTS
		if (nType == 2)
			fAttenuation = (1.0f + OBJ_LIN_ATT * fLightDist + OBJ_QUAD_ATT * fLightDist * fLightDist);
		else
#endif
			fAttenuation = (1.0f + GEO_LIN_ATT * fLightDist + GEO_QUAD_ATT * fLightDist * fLightDist);
#if 0
		NdotL = 1 - ((1 - NdotL) * 0.9f);
#endif
		if ((nVertex > -1) && (NdotL >= nMinDot) && (prl->info.fRad > 0.0f))
			NdotL += (1.0f - NdotL) / (0.5f + fAttenuation / 2.0f);
		fAttenuation /= prl->info.fBrightness;
		}
	if (prl->info.bSpot) {
		if (NdotL <= 0.0f)
			continue;
		spotDir = *prl->info.vDirf.XYZ (); 
		CFloatVector3::Normalize (spotDir);
		lightDir = -lightDir;
		/*
		lightDir [Y] = -lightDir [Y];
		lightDir [Z] = -lightDir [Z];
		*/
		//spotEffect = G3_DOTF (spotDir, lightDir);
		spotEffect = CFloatVector3::Dot (spotDir, lightDir);

		if (spotEffect <= prl->info.fSpotAngle)
			continue;
		if (prl->info.fSpotExponent)
			spotEffect = (float) pow (spotEffect, prl->info.fSpotExponent);
		fAttenuation /= spotEffect * gameStates.ogl.fLightRange;
		vertColor = *gameData.render.vertColor.matAmbient.XYZ () + (*gameData.render.vertColor.matDiffuse.XYZ () * NdotL);
		}
	else {
		vertColor = *gameData.render.vertColor.matAmbient.XYZ ();
		if (NdotL > 0.0f)
			vertColor += (*gameData.render.vertColor.matDiffuse.XYZ () * NdotL);
		else
			NdotL = 0.0f;
		}
	vertColor [R] *= lightColor [R];
	vertColor [G] *= lightColor [G];
	vertColor [B] *= lightColor [B];
	if ((NdotL > 0.0f) && (vcd.fMatShininess > 0.0f) /* && vcd.bMatSpecular */) {
		//RdotV = max (dot (Reflect (-Normalize (lightDir), Normal), Normalize (-vertPos)), 0.0);
		if (!prl->info.bSpot)	//need direction from light to vertex now
			lightDir.Neg ();
		vReflect = CFloatVector3::Reflect (lightDir, vcd.vertNorm);
		CFloatVector3::Normalize (vReflect);
#if DBG
		if ((nDbgVertex >= 0) && (nVertex == nDbgVertex))
			nDbgVertex = nDbgVertex;
#endif
		RdotE = CFloatVector3::Dot (vReflect, vertPos);
		if (RdotE > 0.0f) {
			//spec = pow (Reflect dot lightToEye, matShininess) * matSpecular * lightSpecular
			vertColor += (lightColor * (float) pow (RdotE, vcd.fMatShininess));
			}
		}
	if ((nSaturation < 2) || gameStates.render.bHaveLightmaps) {//sum up color components
		colorSum = colorSum + vertColor * (1.0f / fAttenuation);
		}
	else {	//use max. color components
		vertColor = vertColor * fAttenuation;
		nBrightness = sqri ((int) (vertColor [R] * 1000)) + sqri ((int) (vertColor [G] * 1000)) + sqri ((int) (vertColor [B] * 1000));
		if (nMaxBrightness < nBrightness) {
			nMaxBrightness = nBrightness;
			colorSum = vertColor;
			}
		else if (nMaxBrightness == nBrightness) {
			if (colorSum [R] < vertColor [R])
				colorSum [R] = vertColor [R];
			if (colorSum [G] < vertColor [G])
				colorSum [G] = vertColor [G];
			if (colorSum [B] < vertColor [B])
				colorSum [B] = vertColor [B];
			}
		}
	j++;
	}
if (j) {
	if ((nSaturation == 1) || gameStates.render.bHaveLightmaps) { //if a color component is > 1, cap color components using highest component value
		float	cMax = colorSum [R];
		if (cMax < colorSum [G])
			cMax = colorSum [G];
		if (cMax < colorSum [B])
			cMax = colorSum [B];
		if (cMax > 1) {
			colorSum [R] /= cMax;
			colorSum [G] /= cMax;
			colorSum [B] /= cMax;
			}
		}
	*pColorSum = colorSum;
	}
#if DBG
if (nLights)
	nLights = 0;
#endif
if (!RENDERPATH)
	lightManager.ResetNearestToVertex (nVertex, nThread);
return j;
}

#else //RELEASE

int G3AccumVertColor (int nVertex, CFloatVector3 *pColorSum, CVertColorData *vcdP, int nThread)
{
	int						i, j, nLights, nType, bInRad,
								bSkipHeadlight = gameOpts->ogl.bHeadlight && !gameStates.render.nState,
								nSaturation = gameOpts->render.color.nSaturation;
	int						nBrightness, nMaxBrightness = 0, nMeshQuality = gameOpts->render.nMeshQuality;
	float						fLightDist, fAttenuation, spotEffect, fMag, NdotL, RdotE;
	CFloatVector3					spotDir, lightDir, lightPos, vertPos, vReflect;
	CFloatVector3					lightColor, colorSum, vertColor = {{0.0f, 0.0f, 0.0f}};
	CDynLight			*prl;
	CDynLightIndex		*sliP = &lightManager.Index (0) [nThread];
	CActiveDynLight	*activeLightsP = lightManager.Active (nThread) + sliP->nFirst;
	CVertColorData			vcd = *vcdP;

colorSum = *pColorSum;
VmVecSub (&vertPos, vcd.vertPosP, reinterpret_cast<CFloatVector3*> (&transformation.m_info.glPosf));
CFixVector::Normalize (vertPos, VmVecNegate (&vertPos));
nLights = sliP->nActive;
if (nLights > lightManager.LightCount (0))
	nLights = lightManager.LightCount (0);
i = sliP->nLast - sliP->nFirst + 1;
for (j = 0; (i > 0) && (nLights > 0); activeLightsP++, i--) {
	if (!(prl = activeLightsP->prl))
		continue;
	nLights--;
	nType = prl->info.nType;
	if (bSkipHeadlight && (nType == 3))
		continue;
#if ONLY_HEADLIGHT
	if (nType != 3)
		continue;
#endif
	if (prl->info.bVariable && gameData.render.vertColor.bDarkness)
		continue;
	lightColor = *(reinterpret_cast<CFloatVector3*> (&prl->info.color));
	lightPos = prl->vPosf [gameStates.render.nState && !gameStates.ogl.bUseTransform].XYZ;
#if VECMAT_CALLS
	VmVecSub (&lightDir, &lightPos, vcd.vertPosP);
#else
	lightDir [X] = lightPos [X] - vcd.vertPosP->x();
	lightDir [Y] = lightPos [Y] - vcd.vertPosP->y();
	lightDir [Z] = lightPos [Z] - vcd.vertPosP->z();
#endif
	//scaled quadratic attenuation depending on brightness
	bInRad = 0;
	NdotL = 1;
#if VECMAT_CALLS
	CFixVector::Normalize (lightDir, &lightDir);
#else
	if ((fMag = VmVecMag (&lightDir))) {
		lightDir [X] /= fMag;
		lightDir [Y] /= fMag;
		lightDir [Z] /= fMag;
		}
#endif
#if 0
	if (prl->info.fBrightness < 0)
		fAttenuation = 0.01f;
	else
#endif
	 {
#if VECMAT_CALLS
		fLightDist = VmVecMag (&lightDir) / gameStates.ogl.fLightRange;
#else
		fLightDist = fMag / gameStates.ogl.fLightRange;
#endif
		if (gameStates.render.nState || (nType < 2)) {
#if CHECK_LIGHT_VERT
			if (IsLightVert (nVertex, prl))
				fLightDist = 0;
			else
#endif
				fLightDist -= prl->info.fRad / gameStates.ogl.fLightRange;	//make light brighter close to light source
			}
		if (fLightDist < 0.0f) {
			bInRad = 1;
			fLightDist = 0;
			fAttenuation = 1.0f / prl->info.fBrightness;
			}
		else if (IsLightVert (nVertex, prl)) {
			bInRad = 1;
			fLightDist = 0;
			fAttenuation = 1.0f / prl->info.fBrightness;
			}
		else {	//make it decay faster
#if BRIGHT_SHOTS
			if (nType == 2)
				fAttenuation = (1.0f + OBJ_LIN_ATT * fLightDist + OBJ_QUAD_ATT * fLightDist * fLightDist);
			else
#endif
				fAttenuation = (1.0f + GEO_LIN_ATT * fLightDist + GEO_QUAD_ATT * fLightDist * fLightDist);
			NdotL = CFixVector::Dot (vcd.vertNorm, &lightDir);
#if 0
			NdotL = 1 - ((1 - NdotL) * 0.9f);
#endif
			if (prl->info.fRad > 0)
				NdotL += (1.0f - NdotL) / (0.5f + fAttenuation / 2.0f);
			fAttenuation /= prl->info.fBrightness;
			}
		}
	if (prl->info.bSpot) {
		if (NdotL <= 0)
			continue;
#if VECMAT_CALLS
		CFloatVector::Normalize (&spotDir, &prl->vDirf);
#else
		fMag = VmVecMag (&prl->info.vDirf);
		spotDir.p.x = prl->info.vDirf.p.x / fMag;
		spotDir.p.y = prl->info.vDirf.p.y / fMag;
		spotDir.p.z = prl->info.vDirf.p.z / fMag;
#endif
		lightDir [X] = -lightDir [X];
		lightDir [Y] = -lightDir [Y];
		lightDir [Z] = -lightDir [Z];
		spotEffect = G3_DOTF (spotDir, lightDir);
#if 1
		if (spotEffect <= prl->info.fSpotAngle)
			continue;
#endif
		if (prl->info.fSpotExponent)
			spotEffect = (float) pow (spotEffect, prl->info.fSpotExponent);
		fAttenuation /= spotEffect * gameStates.ogl.fLightRange;
#if VECMAT_CALLS
		VmVecScaleAdd (&vertColor, &gameData.render.vertColor.matAmbient, &gameData.render.vertColor.matDiffuse, NdotL);
#else
		vertColor [X] = gameData.render.vertColor.matAmbient [X] + gameData.render.vertColor.matDiffuse [X] * NdotL;
		vertColor [Y] = gameData.render.vertColor.matAmbient [Y] + gameData.render.vertColor.matDiffuse [Y] * NdotL;
		vertColor [Z] = gameData.render.vertColor.matAmbient [Z] + gameData.render.vertColor.matDiffuse [Z] * NdotL;
#endif
		}
	else {
		vertColor [PA] = gameData.render.vertColor.matAmbient.XYZ.p;
		if (NdotL < 0)
			NdotL = 0;
		else {
		//vertColor = lightColor * (gl_FrontMaterial.diffuse * NdotL + matAmbient);
#if VECMAT_CALLS
			VmVecScaleInc (&vertColor, &gameData.render.vertColor.matDiffuse, NdotL);
#else
			vertColor [X] += gameData.render.vertColor.matDiffuse [X] * NdotL;
			vertColor [Y] += gameData.render.vertColor.matDiffuse [Y] * NdotL;
			vertColor [Z] += gameData.render.vertColor.matDiffuse [Z] * NdotL;
#endif
			}
		}
	vertColor [X] *= lightColor [X];
	vertColor [Y] *= lightColor [Y];
	vertColor [Z] *= lightColor [Z];
	if ((NdotL > 0) && (vcd.fMatShininess > 0)/* && vcd.bMatSpecular */) {
		//spec = pow (Reflect dot lightToEye, matShininess) * matSpecular * lightSpecular
		//RdotV = max (dot (Reflect (-Normalize (lightDir), Normal), Normalize (-vertPos)), 0.0);
		if (!prl->info.bSpot) {	//need direction from light to vertex now
			lightDir [X] = -lightDir [X];
			lightDir [Y] = -lightDir [Y];
			lightDir [Z] = -lightDir [Z];
			}
		G3_REFLECT (vReflect, lightDir, vcd.vertNorm);
#if VECMAT_CALLS
		CFloatVector::Normalize (&vReflect, &vReflect);
#else
		if ((fMag = VmVecMag (&vReflect))) {
			vReflect [X] /= fMag;
			vReflect [Y] /= fMag;
			vReflect [Z] /= fMag;
			}
#endif
		RdotE = G3_DOTF (vReflect, vertPos);
		if (RdotE > 0) {
#if VECMAT_CALLS
			VmVecScale (&lightColor, &lightColor, (float) pow (RdotE, vcd.fMatShininess));
#else
			fMag = (float) pow (RdotE, vcd.fMatShininess);
			lightColor [X] *= fMag;
			lightColor [Y] *= fMag;
			lightColor [Z] *= fMag;
#endif
			}
#if VECMAT_CALLS
		VmVecMul (&lightColor, &lightColor, &vcd.matSpecular);
		VmVecInc (&vertColor, &lightColor);
#else
		vertColor [X] += lightColor [X] * vcd.matSpecular [X];
		vertColor [Y] += lightColor [Y] * vcd.matSpecular [Y];
		vertColor [Z] += lightColor [Z] * vcd.matSpecular [Z];
#endif
		}
	if ((nSaturation < 2) || gameStates.render.bHaveLightmaps) {//sum up color components
#if VECMAT_CALLS
		VmVecScaleAdd (&colorSum, &colorSum, &vertColor, 1.0f / fAttenuation);
#else
		colorSum [X] += vertColor [X] / fAttenuation;
		colorSum [Y] += vertColor [Y] / fAttenuation;
		colorSum [Z] += vertColor [Z] / fAttenuation;
#endif
		}
	else {	//use max. color components
		vertColor [X] /= fAttenuation;
		vertColor [Y] /= fAttenuation;
		vertColor [Z] /= fAttenuation;
		nBrightness = sqri ((int) (vertColor [R] * 1000)) + sqri ((int) (vertColor [G] * 1000)) + sqri ((int) (vertColor [B] * 1000));
		if (nMaxBrightness < nBrightness) {
			nMaxBrightness = nBrightness;
			colorSum = vertColor;
			}
		else if (nMaxBrightness == nBrightness) {
			if (colorSum [R] < vertColor [R])
				colorSum [R] = vertColor [R];
			if (colorSum [G] < vertColor [G])
				colorSum [G] = vertColor [G];
			if (colorSum [B] < vertColor [B])
				colorSum [B] = vertColor [B];
			}
		}
	j++;
	}
if (j) {
	if ((nSaturation == 1) || gameStates.render.bHaveLightmaps) { //if a color component is > 1, cap color components using highest component value
		float	cMax = colorSum [R];
		if (cMax < colorSum [G])
			cMax = colorSum [G];
		if (cMax < colorSum [B])
			cMax = colorSum [B];
		if (cMax > 1) {
			colorSum [R] /= cMax;
			colorSum [G] /= cMax;
			colorSum [B] /= cMax;
			}
		}
	*pColorSum = colorSum;
	}
return j;
}

#endif

//------------------------------------------------------------------------------

void InitVertColorData (CVertColorData& vcd)
{
	static CFloatVector matSpecular = CFloatVector::Create(1.0f, 1.0f, 1.0f, 1.0f);

vcd.bExclusive = !FAST_SHADOWS && (gameStates.render.nShadowPass == 3),
vcd.fMatShininess = 0;
vcd.bMatSpecular = 0;
vcd.bMatEmissive = 0;
vcd.nMatLight = -1;
if (lightManager.Material ().bValid) {
#if 0
	if (lightManager.Material ().emissive [R] ||
		 lightManager.Material ().emissive [G] ||
		 lightManager.Material ().emissive [B]) {
		vcd.bMatEmissive = 1;
		vcd.nMatLight = lightManager.Material ().nLight;
		colorSum = lightManager.Material ().emissive;
		}
#endif
	vcd.bMatSpecular =
		lightManager.Material ().specular [R] ||
		lightManager.Material ().specular [G] ||
		lightManager.Material ().specular [B];
	if (vcd.bMatSpecular) {
		vcd.matSpecular = lightManager.Material ().specular;
		vcd.fMatShininess = (float) lightManager.Material ().shininess;
		}
	else
		vcd.matSpecular = matSpecular;
	}
else {
	vcd.bMatSpecular = 1;
	vcd.matSpecular = matSpecular;
	vcd.fMatShininess = 64;
	}
}

//------------------------------------------------------------------------------

extern int nDbgVertex;


void G3VertexColor (CFloatVector3 *pvVertNorm, CFloatVector3 *pVertPos, int nVertex,
						  tFaceColor *pVertColor, tFaceColor *pBaseColor,
						  float fScale, int bSetColor, int nThread)
{
PROF_START
	CFloatVector3	colorSum = CFloatVector3::Create(0.0f, 0.0f, 0.0f);
	CFloatVector3	vertPos;
	tFaceColor*		pc = NULL;
	int				bVertexLights;
	CVertColorData	vcd;

InitVertColorData (vcd);
#if DBG
if (!gameStates.render.nState && (nVertex == nDbgVertex))
	nVertex = nVertex;
#endif
#if 0
if (gameStates.render.nFlashScale)
	fScale *= X2F (gameStates.render.nFlashScale);
#endif
if (!FAST_SHADOWS && (gameStates.render.nShadowPass == 3))
	; //fScale = 1.0f;
else if (FAST_SHADOWS || (gameStates.render.nShadowPass != 1))
	; //fScale = 1.0f;
else
	fScale *= gameStates.render.bHeadlightOn ? 0.4f : 0.3f;
if (fScale > 1)
	fScale = 1;
#if 1//ndef _DEBUG //cache light values per frame
if (!(gameStates.render.nState || vcd.bExclusive || vcd.bMatEmissive) && (nVertex >= 0)) {
	pc = gameData.render.color.vertices + nVertex;
	if (pc->index == gameStates.render.nFrameFlipFlop + 1) {
		if (pVertColor) {
			pVertColor->index = gameStates.render.nFrameFlipFlop + 1;
			pVertColor->color.red = pc->color.red * fScale;
			pVertColor->color.green = pc->color.green * fScale;
			pVertColor->color.blue = pc->color.blue * fScale;
			pVertColor->color.alpha = 1;
			}
		if (bSetColor)
			OglColor4sf (pc->color.red * fScale, pc->color.green * fScale, pc->color.blue * fScale, 1.0);
#if DBG
		if (!gameStates.render.nState && (nVertex == nDbgVertex))
			nVertex = nVertex;
#endif
PROF_END(ptVertexColor)
		return;
		}
	}
#endif
#if DBG
if (!gameStates.render.nState && (nVertex == nDbgVertex))
	nVertex = nVertex;
#endif
if (gameStates.ogl.bUseTransform)
#if 1
	vcd.vertNorm = *pvVertNorm;
#else
	CFloatVector::Normalize (&vcd.vertNorm, pvVertNorm);
#endif
else {
	if (gameStates.render.nState)
		transformation.Rotate (vcd.vertNorm, *pvVertNorm, 0);
	else {
		vcd.vertNorm = *pvVertNorm; 
		CFloatVector3::Normalize (vcd.vertNorm);
		}
	}
if ((bVertexLights = !(gameStates.render.nState || pVertColor))) {
	vertPos.Assign (gameData.segs.vertices [nVertex]);
	pVertPos = &vertPos;
	lightManager.SetNearestToVertex (-1, nVertex, NULL, 1, 0, 1, nThread);
	}
vcd.vertPosP = pVertPos;
//VmVecNegate (&vertNorm);
//if (nLights)
#if MULTI_THREADED_LIGHTS
if (gameStates.app.bMultiThreaded) {
	SDL_SemPost (gameData.threads.vertColor.info [0].exec);
	SDL_SemPost (gameData.threads.vertColor.info [1].exec);
	SDL_SemWait (gameData.threads.vertColor.info [0].done);
	SDL_SemWait (gameData.threads.vertColor.info [1].done);
	VmVecAdd (&colorSum, vcd.colorSum, vcd.colorSum + 1);
	}
else
#endif
#if 1
if (gameStates.app.bEndLevelSequence >= EL_OUTSIDE) {
	colorSum [R] =
	colorSum [G] =
	colorSum [B] = 1;
	}
else
#endif
 {
	if (lightManager.Index (0) [nThread].nActive) {
		if (pBaseColor)
			memcpy (&colorSum, &pBaseColor->color, sizeof (colorSum));
#if DBG
		if (!gameStates.render.nState && (nVertex == nDbgVertex))
			nVertex = nVertex;
#endif
		G3AccumVertColor (nVertex, &colorSum, &vcd, nThread);
		}
	if ((nVertex >= 0) && !(gameStates.render.nState || gameData.render.vertColor.bDarkness)) {
		tFaceColor *pfc = gameData.render.color.ambient + nVertex;
		colorSum [R] += pfc->color.red * fScale;
		colorSum [G] += pfc->color.green * fScale;
		colorSum [B] += pfc->color.blue * fScale;
#if DBG
		if (!gameStates.render.nState && (nVertex == nDbgVertex) && (colorSum [R] + colorSum [G] + colorSum [B] < 0.1f))
			nVertex = nVertex;
#endif
		}
	if (colorSum [R] > 1.0)
		colorSum [R] = 1.0;
	if (colorSum [G] > 1.0)
		colorSum [G] = 1.0;
	if (colorSum [B] > 1.0)
		colorSum [B] = 1.0;
	}
#if ONLY_HEADLIGHT
if (gameData.render.lights.dynamic.headlights.nLights)
	colorSum [R] = colorSum [G] = colorSum [B] = 0;
#endif
if (bSetColor)
	OglColor4sf (colorSum [R] * fScale, colorSum [G] * fScale, colorSum [B] * fScale, 1.0);
#if 1
if (!vcd.bMatEmissive && pc) {
	pc->index = gameStates.render.nFrameFlipFlop + 1;
	pc->color.red = colorSum [R];
	pc->color.green = colorSum [G];
	pc->color.blue = colorSum [B];
	}
if (pVertColor) {
	pVertColor->index = gameStates.render.nFrameFlipFlop + 1;
	pVertColor->color.red = colorSum [R] * fScale;
	pVertColor->color.green = colorSum [G] * fScale;
	pVertColor->color.blue = colorSum [B] * fScale;
	pVertColor->color.alpha = 1;
	}
#endif
#if DBG
if (!gameStates.render.nState && (nVertex == nDbgVertex))
	nVertex = nVertex;
#endif
#if 0
if (bVertexLights)
	lightManager.Index (0) [nThread].nActive = lightManager.Index (0) [nThread].iVertex;
#endif
PROF_END(ptVertexColor)
}

//------------------------------------------------------------------------------
