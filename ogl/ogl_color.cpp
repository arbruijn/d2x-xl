/* $Id: ogl.c, v 1.14 204/05/11 23:15:55 btb Exp $ */
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

#ifdef _DEBUG
#	define ONLY_HEADLIGHT 0
#else
#	define ONLY_HEADLIGHT 0
#endif

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
tRgbaColorf shadowColor [2] = {{1.0f, 0.0f, 0.0f, 0.25f}, {0.0f, 0.0f, 1.0f, 0.25f}};
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
		h = (bm->bmProps.flags & BM_FLAG_NO_LIGHTING) ? 1.0 : f2fl (uvlList->l);
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
	float l = (bmP->bmProps.flags & BM_FLAG_NO_LIGHTING) ? 1.0f : f2fl (uvlList->l);
	float s = 1.0f;

#if SHADOWS
if (gameStates.ogl.bScaleLight)
	s *= gameStates.render.bHeadLightOn ? 0.4f : 0.3f;
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
	else if (i >= sizeof (vertColors) / sizeof (tFaceColor))
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
	else if (i >= sizeof (vertColors) / sizeof (tFaceColor))
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

#define G3_DOTF(_v0,_v1)	((_v0).p.x * (_v1).p.x + (_v0).p.y * (_v1).p.y + (_v0).p.z * (_v1).p.z)

#define G3_REFLECT(_vr,_vl,_vn) \
	{ \
	float	LdotN = 2 * G3_DOTF(_vl, _vn); \
	(_vr).p.x = (_vn).p.x * LdotN - (_vl).p.x; \
	(_vr).p.y = (_vn).p.y * LdotN - (_vl).p.y; \
	(_vr).p.z = (_vn).p.z * LdotN - (_vl).p.z; \
	} 

//------------------------------------------------------------------------------

inline int sqri (int i)
{
return i * i;
}

//------------------------------------------------------------------------------

#if CHECK_LIGHT_VERT

static inline int IsLightVert (int nVertex, tShaderLight *psl)
{
if (psl->nType < 2) {
		short	*pv = psl->nVerts;

	if (*pv == nVertex)
		return 1;
	if (*++pv == nVertex)
		return 1;
	if (*++pv == nVertex)
		return 1;
	if (*++pv == nVertex)
		return 1;
	}
return 0;
}

#endif

//------------------------------------------------------------------------------

#define VECMAT_CALLS 0

float fLightRanges [5] = {5, 7.071f, 10, 14.142f, 20};

#ifdef _DEBUG

int G3AccumVertColor (int nVertex, fVector *pColorSum, tVertColorData *vcdP, int nThread)
{
	int						i, j, nLights, nType, bInRad, 
								bSkipHeadLight = gameStates.ogl.bHeadLight && (gameData.render.lights.dynamic.headLights.nLights > 0) && !gameStates.render.nState, 
								nSaturation = gameOpts->render.color.nSaturation;
	int						nBrightness, nMaxBrightness = 0;
	float						fLightDist, fAttenuation, spotEffect, NdotL, RdotE;
	fVector					spotDir, lightDir, lightColor, lightPos, vReflect, colorSum, 
								vertColor = {{0.0f, 0.0f, 0.0f, 1.0f}};
	tShaderLight			*psl;
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread] + gameData.render.lights.dynamic.shader.nFirstLight [nThread];
	tVertColorData			vcd = *vcdP;

r_tvertexc++;
colorSum = *pColorSum;
nLights = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
if (nLights > gameData.render.lights.dynamic.nLights)
	nLights = gameData.render.lights.dynamic.nLights;
i = gameData.render.lights.dynamic.shader.nLastLight [nThread] - gameData.render.lights.dynamic.shader.nFirstLight [nThread] + 1;
for (j = 0; (i > 0); activeLightsP++, i--) {
	if (!(psl = GetActiveShaderLight (activeLightsP, nThread)))
		continue;
	if (!nLights--)
		continue;
#if 0
	if (i == vcd.nMatLight)
		continue;
#endif
	nType = psl->nType;
	if (bSkipHeadLight && (nType == 3))
		continue;
#if ONLY_HEADLIGHT
	if (nType != 3)
		continue;
#endif
	if (psl->bVariable && gameData.render.vertColor.bDarkness)
		continue;
	lightColor = *((fVector *) &psl->color);
	lightPos = psl->pos [gameStates.render.nState && !gameStates.ogl.bUseTransform];
	VmVecSub (&lightDir, &lightPos, vcd.pVertPos);
	//scaled quadratic attenuation depending on brightness
	bInRad = 0;
	fLightDist = VmVecMag (&lightDir) / gameStates.ogl.fLightRange;
	VmVecNormalize (&lightDir, &lightDir);
	if (gameStates.render.nState || (nType < 2)) {
#if CHECK_LIGHT_VERT == 2
		if (IsLightVert (nVertex, psl))
			fLightDist = 0;
		else
#endif
			fLightDist -= psl->rad;	//make light brighter close to light source
		}
	if (fLightDist < 1.0f) {
		bInRad = 1;
		fLightDist = 1.4142f;
		NdotL = 1;
		}
#if CHECK_LIGHT_VERT == 1
	else if (IsLightVert (nVertex, psl)) {
		bInRad = 1;
		NdotL = 1;
		}
#endif
	else {	//make it decay faster
		fLightDist *= fLightDist;
		if (nType < 2)
			fLightDist *= 2.0f;
		if (vcd.vertNorm.p.x != 0 || vcd.vertNorm.p.y != 0 || vcd.vertNorm.p.z != 0)
			NdotL = sqrt (VmVecDot (&vcd.vertNorm, &lightDir));
		else
			NdotL = 0;
		}
	fAttenuation = fLightDist / psl->brightness;
	if (psl->bSpot) {
		if (NdotL <= 0)
			continue;
		VmVecNormalize (&spotDir, &psl->dir);
		lightDir.p.x = -lightDir.p.x;
		lightDir.p.y = -lightDir.p.y;
		lightDir.p.z = -lightDir.p.z;
		spotEffect = G3_DOTF (spotDir, lightDir);
		if (spotEffect <= psl->spotAngle)
			continue;
		if (psl->spotExponent)
			spotEffect = (float) pow (spotEffect, psl->spotExponent);
		fAttenuation /= spotEffect * 10;
		VmVecScaleAdd (&vertColor, &gameData.render.vertColor.matAmbient, &gameData.render.vertColor.matDiffuse, NdotL);
		}
	else if (NdotL < 0) {
		NdotL = 0;
		VmVecInc (&vertColor, &gameData.render.vertColor.matAmbient);
		}
	else {
		//vertColor = lightColor * (gl_FrontMaterial.diffuse * NdotL + matAmbient);
		VmVecScaleAdd (&vertColor, &gameData.render.vertColor.matAmbient, &gameData.render.vertColor.matDiffuse, NdotL);
		}
	VmVecMul (&vertColor, &vertColor, &lightColor);
	if ((NdotL > 0.0) /* && vcd.bMatSpecular */) {
		//spec = pow (reflect dot lightToEye, matShininess) * matSpecular * lightSpecular
		//RdotV = max (dot (reflect (-normalize (lightDir), normal), normalize (-vertPos)), 0.0);
		VmVecNegate (&lightPos);	//create vector from light to eye
		VmVecNormalize (&lightPos, &lightPos);
		if (!psl->bSpot)	//need direction from light to vertex now
			VmVecNegate (&lightDir);
		VmVecReflect (&vReflect, &lightDir, &vcd.vertNorm);
		VmVecNormalize (&vReflect, &vReflect);
		RdotE = VmVecDot (&vReflect, &lightPos);
		if (RdotE > 0) {
			VmVecScale (&lightColor, &lightColor, (float) pow (RdotE, vcd.fMatShininess));
			VmVecInc (&vertColor, &lightColor);
			}
		}
	if (nSaturation < 2)	{//sum up color components
		VmVecScaleAdd (&colorSum, &colorSum, &vertColor, 1.0f / fAttenuation);
		}
	else {	//use max. color components
		VmVecScale (&vertColor, &vertColor, fAttenuation);
		nBrightness = sqri ((int) (vertColor.c.r * 1000)) + sqri ((int) (vertColor.c.g * 1000)) + sqri ((int) (vertColor.c.b * 1000));
		if (nMaxBrightness < nBrightness) {
			nMaxBrightness = nBrightness;
			colorSum = vertColor;
			}
		else if (nMaxBrightness == nBrightness) {
			if (colorSum.c.r < vertColor.c.r)
				colorSum.c.r = vertColor.c.r;
			if (colorSum.c.g < vertColor.c.g)
				colorSum.c.g = vertColor.c.g;
			if (colorSum.c.b < vertColor.c.b)
				colorSum.c.b = vertColor.c.b;
			}
		}
	j++;
	}
if (j) {
	if (nSaturation == 1) {	//if a color component is > 1, cap color components using highest component value
		float	cMax = colorSum.c.r;
		if (cMax < colorSum.c.g)
			cMax = colorSum.c.g;
		if (cMax < colorSum.c.b)
			cMax = colorSum.c.b;
		if (cMax > 1) {
			colorSum.c.r /= cMax;
			colorSum.c.g /= cMax;
			colorSum.c.b /= cMax;
			}
		}
	*pColorSum = colorSum;
	}
if (nLights)
	nLights = 0;
return j;
}

#else //RELEASE

int G3AccumVertColor (int nVertex, fVector *pColorSum, tVertColorData *vcdP, int nThread)
{
	int						h, i, j, nLights, nType, bInRad, 
								bSkipHeadLight = gameStates.ogl.bHeadLight && (gameData.render.lights.dynamic.headLights.nLights > 0) && !gameStates.render.nState, 
								nSaturation = gameOpts->render.color.nSaturation;
	int						nBrightness, nMaxBrightness = 0;
	float						fLightDist, fAttenuation, spotEffect, fMag, NdotL, RdotE;
	fVector					spotDir, lightDir, lightColor, lightPos, vReflect, colorSum, 
								vertColor = {{0.0f, 0.0f, 0.0f, 1.0f}};
	tShaderLight			*psl;
	tActiveShaderLight	*activeLightsP = gameData.render.lights.dynamic.shader.activeLights [nThread] + gameData.render.lights.dynamic.shader.nFirstLight [nThread];
	tVertColorData			vcd = *vcdP;

r_tvertexc++;
colorSum = *pColorSum;
nLights = gameData.render.lights.dynamic.shader.nActiveLights [nThread];
if (nLights > gameData.render.lights.dynamic.nLights)
	nLights = gameData.render.lights.dynamic.nLights;
i = gameData.render.lights.dynamic.shader.nLastLight [nThread] - gameData.render.lights.dynamic.shader.nFirstLight [nThread] + 1;
for (j = 0; (i > 0); activeLightsP++, i--) {
	if (!(psl = GetActiveShaderLight (activeLightsP, nThread)))
		continue;
	if (!nLights--)
		continue;
#if 0
	if (i == vcd.nMatLight)
		continue;
#endif
	nType = psl->nType;
	if (bSkipHeadLight && (nType == 3))
		continue;
#if ONLY_HEADLIGHT
	if (nType != 3)
		continue;
#endif
	if (psl->bVariable && gameData.render.vertColor.bDarkness)
		continue;
	lightColor = *((fVector *) &psl->color);
lightPos = psl->pos [gameStates.render.nState && !gameStates.ogl.bUseTransform];
#if VECMAT_CALLS
	VmVecSub (&lightDir, &lightPos, vcd.pVertPos);
#else
	lightDir.p.x = lightPos.p.x - vcd.pVertPos->p.x;
	lightDir.p.y = lightPos.p.y - vcd.pVertPos->p.y;
	lightDir.p.z = lightPos.p.z - vcd.pVertPos->p.z;
#endif
	//scaled quadratic attenuation depending on brightness
	bInRad = 0;
	if (psl->brightness < 0)
		fAttenuation = 0.01f;
	else {
		fLightDist = VmVecMag (&lightDir) / gameStates.ogl.fLightRange;
		if (gameStates.render.nState || (nType < 2)) {
#if CHECK_LIGHT_VERT
			if (IsLightVert (nVertex, psl))
				fLightDist = 0;
			else
#endif
				fLightDist -= psl->rad;	//make light brighter close to light source
			}
		if (fLightDist < 1.0f) {
			bInRad = 1;
			fLightDist = 1.4142f;
			NdotL = 1;
			}
		else if (IsLightVert (nVertex, psl)) {
			bInRad = 1;
			NdotL = 1;
			}
		else {	//make it decay faster
			fLightDist *= fLightDist;
			if (nType < 2)
				fLightDist *= 2.0f;
			NdotL = VmVecDot (&vcd.vertNorm, &lightDir);
			}
		fAttenuation = fLightDist / psl->brightness;
		}
#if VECMAT_CALLS
	VmVecNormalize (&lightDir, &lightDir);
#else
	if ((fMag = VmVecMag (&lightDir))) {
		lightDir.p.x /= fMag;
		lightDir.p.y /= fMag;
		lightDir.p.z /= fMag;
		}
#endif
	NdotL = bInRad ? 1 : G3_DOTF (vcd.vertNorm, lightDir);
	if (psl->bSpot) {
		if (NdotL <= 0)
			continue;
#if VECMAT_CALLS
		VmVecNormalize (&spotDir, &psl->dir);
#else
		fMag = VmVecMag (&psl->dir);
		spotDir.p.x = psl->dir.p.x / fMag;
		spotDir.p.y = psl->dir.p.y / fMag;
		spotDir.p.z = psl->dir.p.z / fMag;
#endif
		lightDir.p.x = -lightDir.p.x;
		lightDir.p.y = -lightDir.p.y;
		lightDir.p.z = -lightDir.p.z;
		spotEffect = G3_DOTF (spotDir, lightDir);
#if 1
		if (spotEffect <= psl->spotAngle)
			continue;
#endif
		if (psl->spotExponent)
			spotEffect = (float) pow (spotEffect, psl->spotExponent);
		fAttenuation /= spotEffect * 10;
#if VECMAT_CALLS
		VmVecScaleAdd (&vertColor, &gameData.render.vertColor.matAmbient, &gameData.render.vertColor.matDiffuse, NdotL);
#else
		vertColor.p.x = gameData.render.vertColor.matAmbient.p.x + /* gameData.render.vertColor.matDiffuse.p.x * */ NdotL;
		vertColor.p.y = gameData.render.vertColor.matAmbient.p.y + /* gameData.render.vertColor.matDiffuse.p.y * */ NdotL;
		vertColor.p.z = gameData.render.vertColor.matAmbient.p.z + /* gameData.render.vertColor.matDiffuse.p.z * */ NdotL;
#endif
		}
	else if (NdotL < 0) {
		NdotL = 0;
#if VECMAT_CALLS
		VmVecInc (&vertColor, &gameData.render.vertColor.matAmbient);
#else
		vertColor.p.x += gameData.render.vertColor.matAmbient.p.x;
		vertColor.p.y += gameData.render.vertColor.matAmbient.p.y;
		vertColor.p.z += gameData.render.vertColor.matAmbient.p.z;
#endif
		}
	else {
		//vertColor = lightColor * (gl_FrontMaterial.diffuse * NdotL + matAmbient);
#if VECMAT_CALLS
		VmVecScaleAdd (&vertColor, &gameData.render.vertColor.matAmbient, &gameData.render.vertColor.matDiffuse, NdotL);
#else
		vertColor.p.x = gameData.render.vertColor.matAmbient.p.x + /* gameData.render.vertColor.matDiffuse.p.x * */ NdotL;
		vertColor.p.y = gameData.render.vertColor.matAmbient.p.y + /* gameData.render.vertColor.matDiffuse.p.y * */ NdotL;
		vertColor.p.z = gameData.render.vertColor.matAmbient.p.z + /* gameData.render.vertColor.matDiffuse.p.z * */ NdotL;
#endif
		}
	vertColor.p.x *= lightColor.p.x;
	vertColor.p.y *= lightColor.p.y;
	vertColor.p.z *= lightColor.p.z;
	if ((NdotL > 0.0) /* && vcd.bMatSpecular */) {
		//spec = pow (reflect dot lightToEye, matShininess) * matSpecular * lightSpecular
		//RdotV = max (dot (reflect (-normalize (lightDir), normal), normalize (-vertPos)), 0.0);
#if VECMAT_CALLS
		VmVecNegate (&lightPos);	//create vector from light to eye
		VmVecNormalize (&lightPos, &lightPos);
#else
		if ((fMag = -VmVecMag (&lightPos))) {
			lightPos.p.x /= fMag;
			lightPos.p.y /= fMag;
			lightPos.p.z /= fMag;
			}
#endif
		if (!psl->bSpot) {	//need direction from light to vertex now
			lightDir.p.x = -lightDir.p.x;
			lightDir.p.y = -lightDir.p.y;
			lightDir.p.z = -lightDir.p.z;
			}
		G3_REFLECT (vReflect, lightDir, vcd.vertNorm);
#if VECMAT_CALLS
		VmVecNormalize (&vReflect, &vReflect);
#else
		if ((fMag = VmVecMag (&vReflect))) {
			vReflect.p.x /= fMag;
			vReflect.p.y /= fMag;
			vReflect.p.z /= fMag;
			}
#endif
		RdotE = G3_DOTF (vReflect, lightPos);
		//if (RdotE > 0.0)
		//	vertColor += matSpecular * lightColor * pow (RdotE, fMatShininess);
		if ((RdotE > 0) && (vcd.fMatShininess > 0)) {
#if VECMAT_CALLS
			VmVecScale (&lightColor, &lightColor, (float) pow (RdotE, vcd.fMatShininess));
#else
			fMag = (float) pow (RdotE, vcd.fMatShininess);
			lightColor.p.x *= fMag;
			lightColor.p.y *= fMag;
			lightColor.p.z *= fMag;
#endif
			}
#if VECMAT_CALLS
		VmVecMul (&lightColor, &lightColor, &vcd.matSpecular);
		VmVecInc (&vertColor, &lightColor);
#else
		vertColor.p.x += lightColor.p.x /* * vcd.matSpecular.p.x */;
		vertColor.p.y += lightColor.p.y /* * vcd.matSpecular.p.y */;
		vertColor.p.z += lightColor.p.z /* * vcd.matSpecular.p.z */;
#endif
		}
	if (nSaturation < 2)	{//sum up color components
#if VECMAT_CALLS
		VmVecScaleAdd (&colorSum, &colorSum, &vertColor, 1.0f / fAttenuation);
#else
		colorSum.p.x += vertColor.p.x / fAttenuation;
		colorSum.p.y += vertColor.p.y / fAttenuation;
		colorSum.p.z += vertColor.p.z / fAttenuation;
#endif
		}
	else {	//use max. color components
		vertColor.p.x /= fAttenuation;
		vertColor.p.y /= fAttenuation;
		vertColor.p.z /= fAttenuation;
		nBrightness = sqri ((int) (vertColor.c.r * 1000)) + sqri ((int) (vertColor.c.g * 1000)) + sqri ((int) (vertColor.c.b * 1000));
		if (nMaxBrightness < nBrightness) {
			nMaxBrightness = nBrightness;
			colorSum = vertColor;
			}
		else if (nMaxBrightness == nBrightness) {
			if (colorSum.c.r < vertColor.c.r)
				colorSum.c.r = vertColor.c.r;
			if (colorSum.c.g < vertColor.c.g)
				colorSum.c.g = vertColor.c.g;
			if (colorSum.c.b < vertColor.c.b)
				colorSum.c.b = vertColor.c.b;
			}
		}
	j++;
	}
if (j) {
	if (nSaturation == 1) {	//if a color component is > 1, cap color components using highest component value
		float	cMax = colorSum.c.r;
		if (cMax < colorSum.c.g)
			cMax = colorSum.c.g;
		if (cMax < colorSum.c.b)
			cMax = colorSum.c.b;
		if (cMax > 1) {
			colorSum.c.r /= cMax;
			colorSum.c.g /= cMax;
			colorSum.c.b /= cMax;
			}
		}
	*pColorSum = colorSum;
	}
return j;
}

#endif

//------------------------------------------------------------------------------

extern int nDbgVertex;

#if PROFILING
time_t tG3VertexColor = 0;
#endif

void G3VertexColor (fVector *pvVertNorm, fVector *pVertPos, int nVertex, 
						  tFaceColor *pVertColor, tFaceColor *pBaseColor, 
						  float fScale, int bSetColor, int nThread)
{
	fVector			matSpecular = {{1.0f, 1.0f, 1.0f, 1.0f}},
						colorSum = {{0.0f, 0.0f, 0.0f, 1.0f}};
	fVector			vertPos;
	tFaceColor		*pc = NULL;
	int				bVertexLights;
#if PROFILING
	time_t			t = clock ();
#endif
	tVertColorData	vcd;

#ifdef _DEBUG
if (!gameStates.render.nState && (nVertex == nDbgVertex))
	nVertex = nVertex;
#endif
if (gameStates.render.nFlashScale)
	fScale *= f2fl (gameStates.render.nFlashScale);
vcd.bExclusive = !FAST_SHADOWS && (gameStates.render.nShadowPass == 3),
vcd.fMatShininess = 0;
vcd.bMatSpecular = 0;
vcd.bMatEmissive = 0; 
vcd.nMatLight = -1;
if (!FAST_SHADOWS && (gameStates.render.nShadowPass == 3))
	; //fScale = 1.0f;
else if (FAST_SHADOWS || (gameStates.render.nShadowPass != 1))
	; //fScale = 1.0f;
else
	fScale *= gameStates.render.bHeadLightOn ? 0.4f : 0.3f;
if (fScale > 1)
	fScale = 1;
if (gameData.render.lights.dynamic.material.bValid) {
#if 0
	if (gameData.render.lights.dynamic.material.emissive.c.r ||
		 gameData.render.lights.dynamic.material.emissive.c.g ||
		 gameData.render.lights.dynamic.material.emissive.c.b) {
		vcd.bMatEmissive = 1;
		vcd.nMatLight = gameData.render.lights.dynamic.material.nLight;
		colorSum = gameData.render.lights.dynamic.material.emissive;
		}
#endif
	vcd.bMatSpecular = 
		gameData.render.lights.dynamic.material.specular.c.r ||
		gameData.render.lights.dynamic.material.specular.c.g ||
		gameData.render.lights.dynamic.material.specular.c.b;
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
#if PROFILING
		tG3VertexColor += clock () - t;
#endif
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
	VmVecNormalize (&vcd.vertNorm, pvVertNorm);
#endif
else {
	if (!gameStates.render.nState)
		VmVecNormalize (&vcd.vertNorm, pvVertNorm);
	else 
		G3RotatePoint (&vcd.vertNorm, pvVertNorm, 0);
	}
if ((bVertexLights = !(gameStates.render.nState || pVertColor))) {
	VmVecFixToFloat (&vertPos, gameData.segs.vertices + nVertex);
	pVertPos = &vertPos;
	SetNearestVertexLights (nVertex, NULL, 1, 0, 1, nThread);
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
	colorSum.c.r = 
	colorSum.c.g = 
	colorSum.c.b = 
	colorSum.c.a = 1;
	}
else 
#endif
	{
	if (gameData.render.lights.dynamic.shader.nActiveLights [nThread]) {
		if (pBaseColor)
			memcpy (&colorSum, &pBaseColor->color, sizeof (colorSum));
#ifdef _DEBUG
		if (!gameStates.render.nState && (nVertex == nDbgVertex))
			nVertex = nVertex;
#endif
		G3AccumVertColor (nVertex, &colorSum, &vcd, nThread);
		}
	if ((nVertex >= 0) && !(gameStates.render.nState || gameData.render.vertColor.bDarkness)) {
		tFaceColor *pc = gameData.render.color.ambient + nVertex;
		colorSum.c.r += pc->color.red;
		colorSum.c.g += pc->color.green;
		colorSum.c.b += pc->color.blue;
		}
	if (colorSum.c.r > 1.0)
		colorSum.c.r = 1.0;
	if (colorSum.c.g > 1.0)
		colorSum.c.g = 1.0;
	if (colorSum.c.b > 1.0)
		colorSum.c.b = 1.0;
	}
#if ONLY_HEADLIGHT
if (gameData.render.lights.dynamic.headLights.nLights)
	colorSum.c.r = colorSum.c.g = colorSum.c.b = 0;
#endif
if (bSetColor)
	OglColor4sf (colorSum.c.r * fScale, colorSum.c.g * fScale, colorSum.c.b * fScale, 1.0);
#if 1
if (!vcd.bMatEmissive && pc) {
	pc->index = gameStates.render.nFrameFlipFlop + 1;
	pc->color.red = colorSum.c.r;
	pc->color.green = colorSum.c.g;
	pc->color.blue = colorSum.c.b;
	}
if (pVertColor) {
	pVertColor->index = gameStates.render.nFrameFlipFlop + 1;
	pVertColor->color.red = colorSum.c.r * fScale;
	pVertColor->color.green = colorSum.c.g * fScale;
	pVertColor->color.blue = colorSum.c.b * fScale;
	pVertColor->color.alpha = colorSum.c.a;
	}
#endif
#ifdef _DEBUG
if (!gameStates.render.nState && (nVertex == nDbgVertex))
	nVertex = nVertex;
#endif
#if 0
if (bVertexLights)
	gameData.render.lights.dynamic.shader.nActiveLights [nThread] = gameData.render.lights.dynamic.shader.iVertexLights [nThread];
#endif
#if PROFILING
tG3VertexColor += clock () - t;
#endif
#ifdef _DEBUG
for (int k = 0; k < MAX_SHADER_LIGHTS; k++)
	if (gameData.render.lights.dynamic.shader.activeLights [0][k].nType > 1) {
		gameData.render.lights.dynamic.shader.activeLights [0][k].nType = 0;
		gameData.render.lights.dynamic.shader.activeLights [0][k].psl = NULL;
		}
	else if (gameData.render.lights.dynamic.shader.activeLights [0][k].nType == 1)
		gameData.render.lights.dynamic.shader.activeLights [0][k].nType = 1;
#endif
} 

//------------------------------------------------------------------------------
