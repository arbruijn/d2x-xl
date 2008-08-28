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

#include "inferno.h"
#include "maths.h"
#include "error.h"
#include "network.h"
#include "light.h"
#include "dynlight.h"
#include "endlevel.h"
#include "ogl_lib.h"
#include "ogl_color.h"

#define CHECK_LIGHT_VERT 1
#define BRIGHT_SHOTS 0

#ifdef _DEBUG
#	define ONLY_HEADLIGHT 0
#else
#	define ONLY_HEADLIGHT 0
#endif

#define GEO_LIN_ATT	(0.1f /** gameData.render.fAttScale*/)
#define GEO_QUAD_ATT	(0.01f /** gameData.render.fAttScale*/)
#define OBJ_LIN_ATT	(0.1f /** gameData.render.fAttScale*/)
#define OBJ_QUAD_ATT	(0.01f /** gameData.render.fAttScale*/)

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

void OglPalColor (ubyte *palette, int c)
{
	GLfloat	fc [4];

if (c < 0)
	glColor3f (1.0, 1.0, 1.0);
else {
	if (!palette)
		palette = gamePalette;
	if (!palette)
		palette = defaultPalette;
	c *= 3;
	fc [0] = (float) (palette [c]) / 63.0f;
	fc [1] = (float) (palette [c]) / 63.0f;
	fc [2] = (float) (palette [c]) / 63.0f;
	if (gameStates.render.grAlpha >= GR_ACTUAL_FADE_LEVELS)
		fc [3] = 1.0f;
	else {
		fc [3] = (float) gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS; //1.0f - (float) gameStates.render.grAlpha / ((float) GR_ACTUAL_FADE_LEVELS - 1.0f);
		glEnable (GL_BLEND);
		}
	glColor4fv (fc);
	}
}

//------------------------------------------------------------------------------

void OglGrsColor (grsColor *pc)
{
	GLfloat	fc [4];

if (!pc)
	glColor4f (1.0, 1.0, 1.0, gameStates.render.grAlpha);
else if (pc->rgb) {
	fc [0] = (float) (pc->color.red) / 255.0f;
	fc [1] = (float) (pc->color.green) / 255.0f;
	fc [2] = (float) (pc->color.blue) / 255.0f;
	fc [3] = (float) (pc->color.alpha) / 255.0f;
	if (fc [3] < 1.0f) {
		glEnable (GL_BLEND);
		glBlendFunc (gameData.render.ogl.nSrcBlend, gameData.render.ogl.nDestBlend);
		}
	glColor4fv (fc);
	}
else
	OglPalColor (gamePalette, pc->index);
}

//------------------------------------------------------------------------------

// cap tMapColor scales the color values in tMapColor so that none of them exceeds
// 1.0 if multiplied with any of the current face's corners' brightness values.
// To do that, first the highest corner brightness is determined.
// In the next step, color values are increased to match the max. brightness.
// Then it is determined whether any color value multiplied with the max. brightness would
// exceed 1.0. If so, all three color values are scaled so that their maximum multiplied
// with the max. brightness does not exceed 1.0.

inline void CapTMapColor (tUVL *uvlList, int nVertices, grsBitmap *bm)
{
#if 0
	tFaceColor *color = tMapColor.index ? &tMapColor : lightColor.index ? &lightColor : NULL;

if (! (bm->bmProps.flags & BM_FLAG_NO_LIGHTING) && color) {
		double	a, m = tMapColor.color.red;
		double	h, l = 0;
		int		i;

	for (i = 0; i < nVertices; i++, uvlList++) {
		h = (bm->bmProps.flags & BM_FLAG_NO_LIGHTING) ? 1.0 : X2F (uvlList->l);
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
void SetTMapColor (tUVL *uvlList, int i, grsBitmap *bmP, int bResetColor, tFaceColor *vertColor)
{
	float l = (bmP->bmProps.flags & BM_FLAG_NO_LIGHTING) ? 1.0f : X2F (uvlList->l);
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

//#define G3_DOTF(_v0,_v1)	((_v0)[X] * (_v1)[X] + (_v0)[Y] * (_v1)[Y] + (_v0)[Z] * (_v1)[Z])
/*
#define G3_REFLECT(_vr,_vl,_vn) \
	{ \
	float	LdotN = 2 * G3_DOTF(_vl, _vn); \
	(_vr)[X] = (_vn)[X] * LdotN - (_vl)[X]; \
	(_vr)[Y] = (_vn)[Y] * LdotN - (_vl)[Y]; \
	(_vr)[Z] = (_vn)[Z] * LdotN - (_vl)[Z]; \
	}
*/
//------------------------------------------------------------------------------

inline int sqri (int i)
{
return i * i;
}

//------------------------------------------------------------------------------

#if CHECK_LIGHT_VERT

static inline int IsLightVert (int nVertex, tShaderLight *psl)
{
if ((nVertex >= 0) && psl->info.faceP) {
	ushort *pv = gameStates.render.bTriangleMesh ? psl->info.faceP->triIndex : psl->info.faceP->index;
	for (int i = psl->info.faceP->nVerts; i; i--, pv++)
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

int G3AccumVertColor (int nVertex, fVector3 *pColorSum, tVertColorData *vcdP, int nThread)
{
	int						i, j, nLights, nType, bInRad,
								bSkipHeadlight = gameOpts->ogl.bHeadlight && !gameStates.render.nState,
								bTransform = gameStates.render.nState && !gameStates.ogl.bUseTransform,
								nSaturation = gameOpts->render.color.nSaturation;
	int						nBrightness, nMaxBrightness = 0;
	float						fLightDist, fAttenuation, spotEffect, NdotL, RdotE, nMinDot;
	fVector3					spotDir, lightDir, lightPos, vertPos, vReflect;
	fVector3					lightColor, colorSum, vertColor = fVector3::Create(0.0f, 0.0f, 0.0f);
	tShaderLight			*psl;
	tShaderLightIndex		*sliP = &gameData.render.lights.dynamic.shader.index [0][nThread];
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread] + sliP->nFirst;
	tVertColorData			vcd = *vcdP;

#ifdef _DEBUG
if (nThread == 0)
	nThread = nThread;
if (nThread == 1)
	nThread = nThread;
#endif
colorSum = *pColorSum;
vertPos = *vcd.pVertPos - *((fVector3 *) &viewInfo.glPosf);
vertPos.neg();
fVector3::normalize(vertPos);
nLights = sliP->nActive;
if (nLights > gameData.render.lights.dynamic.nLights)
	nLights = gameData.render.lights.dynamic.nLights;
i = sliP->nLast - sliP->nFirst + 1;
#ifdef _DEBUG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
for (j = 0; (i > 0) && (nLights > 0); activeLightsP++, i--) {
#if 1
	if (!(psl = activeLightsP->psl))
#else
	if (!(psl = GetActiveShaderLight (activeLightsP, nThread)))
#endif
		continue;
#ifdef _DEBUG
	if ((nDbgSeg >= 0) && (psl->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (psl->info.nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	nLights--;
#if 0
	if (i == vcd.nMatLight)
		continue;
#endif
	nType = psl->info.nType;
	if (bSkipHeadlight && (nType == 3))
		continue;
#if ONLY_HEADLIGHT
	if (nType != 3)
		continue;
#endif
	if (psl->info.bVariable && gameData.render.vertColor.bDarkness)
		continue;
	lightColor = *((fVector3 *) &psl->info.color);
	lightPos = *psl->vPosf [bTransform].v3();
	lightDir = lightPos - *vcd.pVertPos;
	bInRad = 0;
	fLightDist = lightDir.mag() * gameStates.ogl.fLightRange;
	fVector3::normalize(lightDir);
	if (vcd.vertNorm.isZero())
		NdotL = 1.0f;
	else
		NdotL = fVector3::dot(vcd.vertNorm, lightDir);
	nMinDot = -0.1f;
	if (gameStates.render.nState || (nType < 2)) {
#if CHECK_LIGHT_VERT == 2
		if (IsLightVert (nVertex, psl))
			fLightDist = 0;
		else
#endif
			fLightDist -= psl->info.fRad * gameStates.ogl.fLightRange; //make light brighter close to light source
		if (fLightDist < 0)
			fLightDist = 0;
#if 1 //don't directly light faces turning their back side towards the light source
		if ((NdotL < 0) && (fVector3::dot(lightDir, *psl->info.vDirf.v3()) <= 0))
			nMinDot = 0;
#endif
		}
	if	(((NdotL >= nMinDot) && (fLightDist <= 0.0f)) || IsLightVert (nVertex, psl)) {
		bInRad = 1;
		NdotL = 1;
		fLightDist = 0;
		fAttenuation = 1.0f / psl->info.fBrightness;
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
		if ((NdotL >= nMinDot) && (psl->info.fRad > 0))
			NdotL += (1.0f - NdotL) / (0.5f + fAttenuation / 2.0f);
		fAttenuation /= psl->info.fBrightness;
		}
	if (psl->info.bSpot) {
		if (NdotL <= 0)
			continue;
		spotDir = *psl->info.vDirf.v3(); fVector3::normalize(spotDir);
		lightDir = -lightDir;
		/*
		lightDir[Y] = -lightDir[Y];
		lightDir[Z] = -lightDir[Z];
		*/
		//spotEffect = G3_DOTF (spotDir, lightDir);
		spotEffect = fVector3::dot(spotDir, lightDir);

		if (spotEffect <= psl->info.fSpotAngle)
			continue;
		if (psl->info.fSpotExponent)
			spotEffect = (float) pow (spotEffect, psl->info.fSpotExponent);
		fAttenuation /= spotEffect * gameStates.ogl.fLightRange;
		vertColor = *gameData.render.vertColor.matAmbient.v3() + (*gameData.render.vertColor.matDiffuse.v3() * NdotL);
		}
	else {
		vertColor = *gameData.render.vertColor.matAmbient.v3();
		if (NdotL < 0)
			NdotL = 0;
		else
			vertColor += (*gameData.render.vertColor.matDiffuse.v3() * NdotL);
		}
	//TODO: Color Klasse
	vertColor[R] *= lightColor[R];
	vertColor[G] *= lightColor[G];
	vertColor[B] *= lightColor[B];
	if ((NdotL > 0.0) && (vcd.fMatShininess > 0) /* && vcd.bMatSpecular */) {
		//RdotV = max (dot (reflect (-normalize (lightDir), normal), normalize (-vertPos)), 0.0);
		if (!psl->info.bSpot)	//need direction from light to vertex now
			lightDir.neg();
		vReflect = fVector3::reflect(lightDir, vcd.vertNorm);
		fVector3::normalize(vReflect);
#ifdef _DEBUG
		if (nVertex == nDbgVertex)
			nDbgVertex = nDbgVertex;
#endif
		RdotE = fVector3::dot(vReflect, vertPos);
		if (RdotE > 0) {
			//spec = pow (reflect dot lightToEye, matShininess) * matSpecular * lightSpecular
			vertColor += (lightColor * (float) pow (RdotE, vcd.fMatShininess));
			}
		}
	if ((nSaturation < 2) || gameStates.render.bLightmaps)	{//sum up color components
		colorSum = colorSum + vertColor * (1.0f/fAttenuation);
		}
	else {	//use max. color components
		vertColor = vertColor * fAttenuation;
		nBrightness = sqri ((int) (vertColor[R] * 1000)) + sqri ((int) (vertColor[G] * 1000)) + sqri ((int) (vertColor[B] * 1000));
		if (nMaxBrightness < nBrightness) {
			nMaxBrightness = nBrightness;
			colorSum = vertColor;
			}
		else if (nMaxBrightness == nBrightness) {
			if (colorSum[R] < vertColor[R])
				colorSum[R] = vertColor[R];
			if (colorSum[G] < vertColor[G])
				colorSum[G] = vertColor[G];
			if (colorSum[B] < vertColor[B])
				colorSum[B] = vertColor[B];
			}
		}
	j++;
	}
if (j) {
	if ((nSaturation == 1) || gameStates.render.bLightmaps) { //if a color component is > 1, cap color components using highest component value
		float	cMax = colorSum[R];
		if (cMax < colorSum[G])
			cMax = colorSum[G];
		if (cMax < colorSum[B])
			cMax = colorSum[B];
		if (cMax > 1) {
			colorSum[R] /= cMax;
			colorSum[G] /= cMax;
			colorSum[B] /= cMax;
			}
		}
	*pColorSum = colorSum;
	}
#ifdef _DEBUG
if (nLights)
	nLights = 0;
#endif
if (!RENDERPATH)
	ResetNearestVertexLights (nVertex, nThread);
return j;
}

#else //RELEASE

int G3AccumVertColor (int nVertex, fVector3 *pColorSum, tVertColorData *vcdP, int nThread)
{
	int						i, j, nLights, nType, bInRad,
								bSkipHeadlight = gameOpts->ogl.bHeadlight && !gameStates.render.nState,
								nSaturation = gameOpts->render.color.nSaturation;
	int						nBrightness, nMaxBrightness = 0, nMeshQuality = gameOpts->render.nMeshQuality;
	float						fLightDist, fAttenuation, spotEffect, fMag, NdotL, RdotE;
	fVector3					spotDir, lightDir, lightPos, vertPos, vReflect;
	fVector3					lightColor, colorSum, vertColor = {{0.0f, 0.0f, 0.0f}};
	tShaderLight			*psl;
	tShaderLightIndex		*sliP = &gameData.render.lights.dynamic.shader.index [0][nThread];
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread] + sliP->nFirst;
	tVertColorData			vcd = *vcdP;

colorSum = *pColorSum;
VmVecSub (&vertPos, vcd.pVertPos, (fVector3 *) &viewInfo.glPosf);
vmsVector::normalize(vertPos, VmVecNegate (&vertPos));
nLights = sliP->nActive;
if (nLights > gameData.render.lights.dynamic.nLights)
	nLights = gameData.render.lights.dynamic.nLights;
i = sliP->nLast - sliP->nFirst + 1;
for (j = 0; (i > 0) && (nLights > 0); activeLightsP++, i--) {
	if (!(psl = activeLightsP->psl))
		continue;
	nLights--;
	nType = psl->info.nType;
	if (bSkipHeadlight && (nType == 3))
		continue;
#if ONLY_HEADLIGHT
	if (nType != 3)
		continue;
#endif
	if (psl->info.bVariable && gameData.render.vertColor.bDarkness)
		continue;
	lightColor = *((fVector3 *) &psl->info.color);
	lightPos = psl->vPosf [gameStates.render.nState && !gameStates.ogl.bUseTransform].v3;
#if VECMAT_CALLS
	VmVecSub (&lightDir, &lightPos, vcd.pVertPos);
#else
	lightDir[X] = lightPos[X] - vcd.pVertPos->x();
	lightDir[Y] = lightPos[Y] - vcd.pVertPos->y();
	lightDir[Z] = lightPos[Z] - vcd.pVertPos->z();
#endif
	//scaled quadratic attenuation depending on brightness
	bInRad = 0;
	NdotL = 1;
#if VECMAT_CALLS
	vmsVector::normalize(lightDir, &lightDir);
#else
	if ((fMag = VmVecMag (&lightDir))) {
		lightDir[X] /= fMag;
		lightDir[Y] /= fMag;
		lightDir[Z] /= fMag;
		}
#endif
#if 0
	if (psl->info.fBrightness < 0)
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
			if (IsLightVert (nVertex, psl))
				fLightDist = 0;
			else
#endif
				fLightDist -= psl->info.fRad / gameStates.ogl.fLightRange;	//make light brighter close to light source
			}
		if (fLightDist < 0.0f) {
			bInRad = 1;
			fLightDist = 0;
			fAttenuation = 1.0f / psl->info.fBrightness;
			}
		else if (IsLightVert (nVertex, psl)) {
			bInRad = 1;
			fLightDist = 0;
			fAttenuation = 1.0f / psl->info.fBrightness;
			}
		else {	//make it decay faster
#if BRIGHT_SHOTS
			if (nType == 2)
				fAttenuation = (1.0f + OBJ_LIN_ATT * fLightDist + OBJ_QUAD_ATT * fLightDist * fLightDist);
			else
#endif
				fAttenuation = (1.0f + GEO_LIN_ATT * fLightDist + GEO_QUAD_ATT * fLightDist * fLightDist);
			NdotL = vmsVector::dot(vcd.vertNorm, &lightDir);
#if 0
			NdotL = 1 - ((1 - NdotL) * 0.9f);
#endif
			if (psl->info.fRad > 0)
				NdotL += (1.0f - NdotL) / (0.5f + fAttenuation / 2.0f);
			fAttenuation /= psl->info.fBrightness;
			}
		}
	if (psl->info.bSpot) {
		if (NdotL <= 0)
			continue;
#if VECMAT_CALLS
		fVector::normalize(&spotDir, &psl->vDirf);
#else
		fMag = VmVecMag (&psl->info.vDirf);
		spotDir.p.x = psl->info.vDirf.p.x / fMag;
		spotDir.p.y = psl->info.vDirf.p.y / fMag;
		spotDir.p.z = psl->info.vDirf.p.z / fMag;
#endif
		lightDir[X] = -lightDir[X];
		lightDir[Y] = -lightDir[Y];
		lightDir[Z] = -lightDir[Z];
		spotEffect = G3_DOTF (spotDir, lightDir);
#if 1
		if (spotEffect <= psl->info.fSpotAngle)
			continue;
#endif
		if (psl->info.fSpotExponent)
			spotEffect = (float) pow (spotEffect, psl->info.fSpotExponent);
		fAttenuation /= spotEffect * gameStates.ogl.fLightRange;
#if VECMAT_CALLS
		VmVecScaleAdd (&vertColor, &gameData.render.vertColor.matAmbient, &gameData.render.vertColor.matDiffuse, NdotL);
#else
		vertColor[X] = gameData.render.vertColor.matAmbient[X] + gameData.render.vertColor.matDiffuse[X] * NdotL;
		vertColor[Y] = gameData.render.vertColor.matAmbient[Y] + gameData.render.vertColor.matDiffuse[Y] * NdotL;
		vertColor[Z] = gameData.render.vertColor.matAmbient[Z] + gameData.render.vertColor.matDiffuse[Z] * NdotL;
#endif
		}
	else {
		vertColor[PA] = gameData.render.vertColor.matAmbient.v3.p;
		if (NdotL < 0)
			NdotL = 0;
		else {
		//vertColor = lightColor * (gl_FrontMaterial.diffuse * NdotL + matAmbient);
#if VECMAT_CALLS
			VmVecScaleInc (&vertColor, &gameData.render.vertColor.matDiffuse, NdotL);
#else
			vertColor[X] += gameData.render.vertColor.matDiffuse[X] * NdotL;
			vertColor[Y] += gameData.render.vertColor.matDiffuse[Y] * NdotL;
			vertColor[Z] += gameData.render.vertColor.matDiffuse[Z] * NdotL;
#endif
			}
		}
	vertColor[X] *= lightColor[X];
	vertColor[Y] *= lightColor[Y];
	vertColor[Z] *= lightColor[Z];
	if ((NdotL > 0) && (vcd.fMatShininess > 0)/* && vcd.bMatSpecular */) {
		//spec = pow (reflect dot lightToEye, matShininess) * matSpecular * lightSpecular
		//RdotV = max (dot (reflect (-normalize (lightDir), normal), normalize (-vertPos)), 0.0);
		if (!psl->info.bSpot) {	//need direction from light to vertex now
			lightDir[X] = -lightDir[X];
			lightDir[Y] = -lightDir[Y];
			lightDir[Z] = -lightDir[Z];
			}
		G3_REFLECT (vReflect, lightDir, vcd.vertNorm);
#if VECMAT_CALLS
		fVector::normalize(&vReflect, &vReflect);
#else
		if ((fMag = VmVecMag (&vReflect))) {
			vReflect[X] /= fMag;
			vReflect[Y] /= fMag;
			vReflect[Z] /= fMag;
			}
#endif
		RdotE = G3_DOTF (vReflect, vertPos);
		if (RdotE > 0) {
#if VECMAT_CALLS
			VmVecScale (&lightColor, &lightColor, (float) pow (RdotE, vcd.fMatShininess));
#else
			fMag = (float) pow (RdotE, vcd.fMatShininess);
			lightColor[X] *= fMag;
			lightColor[Y] *= fMag;
			lightColor[Z] *= fMag;
#endif
			}
#if VECMAT_CALLS
		VmVecMul (&lightColor, &lightColor, &vcd.matSpecular);
		VmVecInc (&vertColor, &lightColor);
#else
		vertColor[X] += lightColor[X] * vcd.matSpecular[X];
		vertColor[Y] += lightColor[Y] * vcd.matSpecular[Y];
		vertColor[Z] += lightColor[Z] * vcd.matSpecular[Z];
#endif
		}
	if ((nSaturation < 2) || gameStates.render.bLightmaps)	{//sum up color components
#if VECMAT_CALLS
		VmVecScaleAdd (&colorSum, &colorSum, &vertColor, 1.0f / fAttenuation);
#else
		colorSum[X] += vertColor[X] / fAttenuation;
		colorSum[Y] += vertColor[Y] / fAttenuation;
		colorSum[Z] += vertColor[Z] / fAttenuation;
#endif
		}
	else {	//use max. color components
		vertColor[X] /= fAttenuation;
		vertColor[Y] /= fAttenuation;
		vertColor[Z] /= fAttenuation;
		nBrightness = sqri ((int) (vertColor[R] * 1000)) + sqri ((int) (vertColor[G] * 1000)) + sqri ((int) (vertColor[B] * 1000));
		if (nMaxBrightness < nBrightness) {
			nMaxBrightness = nBrightness;
			colorSum = vertColor;
			}
		else if (nMaxBrightness == nBrightness) {
			if (colorSum[R] < vertColor[R])
				colorSum[R] = vertColor[R];
			if (colorSum[G] < vertColor[G])
				colorSum[G] = vertColor[G];
			if (colorSum[B] < vertColor[B])
				colorSum[B] = vertColor[B];
			}
		}
	j++;
	}
if (j) {
	if ((nSaturation == 1) || gameStates.render.bLightmaps) { //if a color component is > 1, cap color components using highest component value
		float	cMax = colorSum[R];
		if (cMax < colorSum[G])
			cMax = colorSum[G];
		if (cMax < colorSum[B])
			cMax = colorSum[B];
		if (cMax > 1) {
			colorSum[R] /= cMax;
			colorSum[G] /= cMax;
			colorSum[B] /= cMax;
			}
		}
	*pColorSum = colorSum;
	}
return j;
}

#endif

//------------------------------------------------------------------------------

void InitVertColorData (tVertColorData& vcd)
{
	static fVector matSpecular = fVector::Create(1.0f, 1.0f, 1.0f, 1.0f);

vcd.bExclusive = !FAST_SHADOWS && (gameStates.render.nShadowPass == 3),
vcd.fMatShininess = 0;
vcd.bMatSpecular = 0;
vcd.bMatEmissive = 0;
vcd.nMatLight = -1;
if (gameData.render.lights.dynamic.material.bValid) {
#if 0
	if (gameData.render.lights.dynamic.material.emissive[R] ||
		 gameData.render.lights.dynamic.material.emissive[G] ||
		 gameData.render.lights.dynamic.material.emissive[B]) {
		vcd.bMatEmissive = 1;
		vcd.nMatLight = gameData.render.lights.dynamic.material.nLight;
		colorSum = gameData.render.lights.dynamic.material.emissive;
		}
#endif
	vcd.bMatSpecular =
		gameData.render.lights.dynamic.material.specular[R] ||
		gameData.render.lights.dynamic.material.specular[G] ||
		gameData.render.lights.dynamic.material.specular[B];
	if (vcd.bMatSpecular) {
		vcd.matSpecular = gameData.render.lights.dynamic.material.specular;
		vcd.fMatShininess = (float) gameData.render.lights.dynamic.material.shininess;
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


void G3VertexColor (fVector3 *pvVertNorm, fVector3 *pVertPos, int nVertex,
						  tFaceColor *pVertColor, tFaceColor *pBaseColor,
						  float fScale, int bSetColor, int nThread)
{
PROF_START
	fVector3			colorSum = fVector3::Create(0.0f, 0.0f, 0.0f);
	fVector3			vertPos;
	tFaceColor		*pc = NULL;
	int				bVertexLights;
	tVertColorData	vcd;

InitVertColorData (vcd);
#ifdef _DEBUG
if (!gameStates.render.nState && (nVertex == nDbgVertex))
	nVertex = nVertex;
#endif
if (gameStates.render.nFlashScale)
	fScale *= X2F (gameStates.render.nFlashScale);
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
#ifdef _DEBUG
		if (!gameStates.render.nState && (nVertex == nDbgVertex))
			nVertex = nVertex;
#endif
PROF_END(ptVertexColor)
		return;
		}
	}
#endif
#ifdef _DEBUG
if (!gameStates.render.nState && (nVertex == nDbgVertex))
	nVertex = nVertex;
#endif
if (gameStates.ogl.bUseTransform)
#if 1
	vcd.vertNorm = *pvVertNorm;
#else
	fVector::normalize(&vcd.vertNorm, pvVertNorm);
#endif
else {
	if (!gameStates.render.nState) {
		vcd.vertNorm = *pvVertNorm; fVector3::normalize(vcd.vertNorm);
	}
	else
		G3RotatePoint(vcd.vertNorm, *pvVertNorm, 0);
}
if ((bVertexLights = !(gameStates.render.nState || pVertColor))) {
	vertPos = gameData.segs.vertices[nVertex].toFloat3();
	pVertPos = &vertPos;
	SetNearestVertexLights (-1, nVertex, NULL, 1, 0, 1, nThread);
	}
vcd.pVertPos = pVertPos;
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
	colorSum[R] =
	colorSum[G] =
	colorSum[B] = 1;
	}
else
#endif
	{
	if (gameData.render.lights.dynamic.shader.index [0][nThread].nActive) {
		if (pBaseColor)
			memcpy (&colorSum, &pBaseColor->color, sizeof (colorSum));
#ifdef _DEBUG
		if (!gameStates.render.nState && (nVertex == nDbgVertex))
			nVertex = nVertex;
#endif
		G3AccumVertColor (nVertex, &colorSum, &vcd, nThread);
		}
	if ((nVertex >= 0) && !(gameStates.render.nState || gameData.render.vertColor.bDarkness)) {
		tFaceColor *pfc = gameData.render.color.ambient + nVertex;
		colorSum[R] += pfc->color.red;
		colorSum[G] += pfc->color.green;
		colorSum[B] += pfc->color.blue;
#ifdef _DEBUG
		if (!gameStates.render.nState && (nVertex == nDbgVertex) && (colorSum[R] + colorSum[G] + colorSum[B] < 0.1f))
			nVertex = nVertex;
#endif
		}
	if (colorSum[R] > 1.0)
		colorSum[R] = 1.0;
	if (colorSum[G] > 1.0)
		colorSum[G] = 1.0;
	if (colorSum[B] > 1.0)
		colorSum[B] = 1.0;
	}
#if ONLY_HEADLIGHT
if (gameData.render.lights.dynamic.headlights.nLights)
	colorSum[R] = colorSum[G] = colorSum[B] = 0;
#endif
if (bSetColor)
	OglColor4sf (colorSum[R] * fScale, colorSum[G] * fScale, colorSum[B] * fScale, 1.0);
#if 1
if (!vcd.bMatEmissive && pc) {
	pc->index = gameStates.render.nFrameFlipFlop + 1;
	pc->color.red = colorSum[R];
	pc->color.green = colorSum[G];
	pc->color.blue = colorSum[B];
	}
if (pVertColor) {
	pVertColor->index = gameStates.render.nFrameFlipFlop + 1;
	pVertColor->color.red = colorSum[R] * fScale;
	pVertColor->color.green = colorSum[G] * fScale;
	pVertColor->color.blue = colorSum[B] * fScale;
	pVertColor->color.alpha = 1;
	}
#endif
#ifdef _DEBUG
if (!gameStates.render.nState && (nVertex == nDbgVertex))
	nVertex = nVertex;
#endif
#if 0
if (bVertexLights)
	gameData.render.lights.dynamic.shader.index [0][nThread].nActive = gameData.render.lights.dynamic.shader.index [0][nThread].iVertex;
#endif
PROF_END(ptVertexColor)
}

//------------------------------------------------------------------------------
