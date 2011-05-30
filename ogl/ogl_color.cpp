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
#include "collision_math.h"
#include "palette.h"
#include "transformation.h"

#define TEST_AMBIENT			0
#define CHECK_LIGHT_VERT	1
#define BRIGHT_SHOTS			0
#if DBG
#	define USE_FACE_DIST		1
#else
#	define USE_FACE_DIST		1
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
tRgbaColorf shadowColor [2] = {{1.0f, 0.0f, 0.0f, 0.8f}, {0.0f, 0.0f, 1.0f, 0.8f}};
tRgbaColorf modelColor [2] = {{0.0f, 0.5f, 1.0f, 0.5f}, {0.0f, 1.0f, 0.5f, 0.5f}};

//------------------------------------------------------------------------------

void OglPalColor (CPalette *palette, int c, tRgbaColorf* colorP)
{
	tRgbaColorf	color;

if (c < 0) {
	if (colorP)
		colorP->red =
		colorP->green =
		colorP->blue =
		colorP->alpha = 1.0;
	else
		glColor3f (1.0, 1.0, 1.0);
	}
else {
	if (!palette)
		palette = paletteManager.Game ();
	palette->ToRgbaf (c, color);
	color.alpha = gameStates.render.grAlpha;
	if (colorP)
		*colorP = color;
	else
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

void OglCanvasColor (tCanvasColor* canvColorP, tRgbaColorf* colorP)
{
	tRgbaColorf	color;

if (!canvColorP) {
	if (colorP) {
		colorP->red =
		colorP->green =
		colorP->blue = 1.0f;
		colorP->alpha = gameStates.render.grAlpha;
		}
	else 
		glColor4f (1.0f, 1.0f, 1.0f, gameStates.render.grAlpha);
	}
else if (canvColorP->rgb) {
	color.red = float (canvColorP->color.red) / 255.0f;
	color.green = float (canvColorP->color.green) / 255.0f;
	color.blue = float (canvColorP->color.blue) / 255.0f;
	color.alpha = float (canvColorP->color.alpha) / 255.0f * gameStates.render.grAlpha;
	if (color.alpha < 1.0f)
		ogl.SetBlendMode (ogl.m_data.nSrcBlendMode, ogl.m_data.nDestBlendMode);
	if (colorP)
		*colorP = color;
	else
		glColor4fv (reinterpret_cast<GLfloat*> (&color));
	}
else
	OglPalColor (paletteManager.Game (), canvColorP->index, colorP);
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
		ogl.SetBlending (true);
		ogl.SetBlendMode (ogl.m_data.nSrcBlendMode, ogl.m_data.nDestBlendMode);
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
		double	vec, mat = tMapColor.color.red;
		double	h, l = 0;
		int		i;

	for (i = 0; i < nVertices; i++, uvlList++) {
		h = (bm->props.flags & BM_FLAG_NO_LIGHTING) ? 1.0 : X2F (uvlList->l);
		if (l < h)
			l = h;
		}

	// scale brightness with the average color to avoid darkening bright areas with the color
	vec = (color->color.red + color->color.green + color->color.blue) / 3;
	if (mat < color->color.green)
		mat = color->color.green;
	if (mat < color->color.blue)
		mat = color->color.blue;
	l = l / vec;
	// prevent any color component getting over 1.0
	if (l * mat > 1.0)
		l = 1.0 / mat;
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
if (ogl.m_states.bStandardContrast)
	glColor4f (r * s, g * s, b * s, ogl.m_states.fAlpha);
else {
	float c = (float) ogl.m_states.nContrast - 8.0f;

	if (c > 0.0f)
		c = 1.0f / (1.0f + c * (3.0f / 8.0f));
	else
		c = 1.0f - c * (3.0f / 8.0f);
	glColor4f (BC (r, c) * s, BC (g, c) * s, BC (b, c) * s, ogl.m_states.fAlpha);
	}
}

//------------------------------------------------------------------------------

/*inline*/
void SetTMapColor (tUVL *uvlList, int i, CBitmap *bmP, int bResetColor, tRgbaColorf* colorP)
{
	float l = (bmP->Flags () & BM_FLAG_NO_LIGHTING) ? 1.0f : X2F (uvlList->l);
	float s = 1.0f;

l *= gameData.render.fBrightness;
if (ogl.m_states.bScaleLight)
	s *= gameStates.render.bHeadlightOn ? 0.4f : 0.3f;
if (gameStates.app.bEndLevelSequence >= EL_OUTSIDE) {
	if (!colorP)
		OglColor4sf (l, l, l, s);
	else {
		colorP->red =
		colorP->green =
		colorP->blue = l;
		colorP->alpha = s;
		}
	}
else if (colorP) {
	if (tMapColor.index) {
		ScaleColor (&tMapColor, l);
		*colorP = tMapColor.color;
		colorP->red *= gameData.render.fBrightness;
		colorP->green *= gameData.render.fBrightness;
		colorP->blue *= gameData.render.fBrightness;
		if (l >= 0)
			tMapColor.color.red =
			tMapColor.color.green =
			tMapColor.color.blue = 1.0;
		}
	else if (i >= (int) (sizeof (vertColors) / sizeof (tFaceColor)))
		return;
	else if (vertColors [i].index) {
			tFaceColor *pvc = vertColors + i;

		*colorP = vertColors [i].color;
		colorP->red *= gameData.render.fBrightness;
		colorP->green *= gameData.render.fBrightness;
		colorP->blue *= gameData.render.fBrightness;
		if (bResetColor) {
			pvc->color.red =
			pvc->color.green =
			pvc->color.blue = 1.0;
			pvc->index = 0;
			}
		}
	else {
		colorP->red =
		colorP->green =
		colorP->blue = l;
		}
	colorP->alpha = s;
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

//#define G3_DOTF(_v0,_v1)	((_v0).v.c.x * (_v1).v.c.x + (_v0).v.c.y * (_v1).v.c.y + (_v0).v.c.z * (_v1).v.c.z)
/*
#define G3_REFLECT(_vr,_vl,_vn) \
 { \
	float	LdotN = 2 * G3_DOTF(_vl, _vn); \
	(_vr).v.c.x = (_vn).v.c.x * LdotN - (_vl).v.c.x; \
	(_vr).v.c.y = (_vn).v.c.y * LdotN - (_vl).v.c.y; \
	(_vr).v.c.z = (_vn).v.c.z * LdotN - (_vl).v.c.z; \
	}
*/
//------------------------------------------------------------------------------

inline int sqri (int i)
{
return i * i;
}

//------------------------------------------------------------------------------

#if CHECK_LIGHT_VERT

static inline int IsLightVert (int nVertex, CDynLight *lightP)
{
if ((nVertex >= 0) && lightP->info.faceP) {
	ushort *pv = gameStates.render.bTriangleMesh ? lightP->info.faceP->triIndex : lightP->info.faceP->m_info.index;
	for (int i = lightP->info.faceP->m_info.nVerts; i; i--, pv++)
		if (*pv == (ushort) nVertex)
			return 1;
	}
return 0;
}

#endif

//------------------------------------------------------------------------------

#define VECMAT_CALLS 0

float fLightRanges [5] = {0.5f, 0.7071f, 1.0f, 1.4142f, 2.0f};

int G3AccumVertColor (int nVertex, CFloatVector3 *pColorSum, CVertColorData *colorDataP, int nThread)
{
	CDynLight*			lightP;
	CDynLightIndex*	sliP = &lightManager.Index (0, nThread);
	CActiveDynLight*	activeLightsP = lightManager.Active (nThread) + sliP->nFirst;
	CVertColorData		colorData = *colorDataP;
	int					i, j, nLights, nType,
							bSkipHeadlight = gameOpts->ogl.bHeadlight && !gameStates.render.nState,
							bTransform = (gameStates.render.nState > 0) && !ogl.m_states.bUseTransform,
							nSaturation = gameOpts->render.color.nSaturation;
	int					nBrightness, nMaxBrightness = 0;
	int					bDiffuse, bSpecular = gameStates.render.bSpecularColor && (colorData.fMatShininess > 0.0f);
	float					fLightDist, fAttenuation, fLightAngle, spotEffect, NdotL, RdotE;
	CFloatVector3		lightDir, lightRayDir, lightPos, vertPos, vReflect;
	CFloatVector3		lightColor, colorSum, vertColor = CFloatVector3::Create (0.0f, 0.0f, 0.0f);

#if DBG
if (nThread == 0)
	nThread = nThread;
if (nThread == 1)
	nThread = nThread;
#endif
colorSum = *pColorSum;
vertPos = *transformation.m_info.posf [1].XYZ () - *colorData.vertPosP;
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
	if (!(lightP = activeLightsP->lightP))
#else
	if (!(lightP = GetActiveRenderLight (activeLightsP, nThread)))
#endif
		continue;
	nLights--;
#if DBG
	if ((nDbgVertex >= 0) && (nVertex == nDbgVertex))
		nDbgVertex = nDbgVertex;
#endif
#if DBG
	if ((nDbgSeg >= 0) && (lightP->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (lightP->info.nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#	if 0
	else
		continue;
#	endif
#endif
	if (!lightP->render.bState)
		continue;
#if 0
	if (i == colorData.nMatLight)
		continue;
#endif
	nType = lightP->render.nType;
	if (bSkipHeadlight && (nType == 3))
		continue;
#if ONLY_HEADLIGHT
	if (nType != 3)
		continue;
#endif
	if (lightP->info.bVariable && gameData.render.vertColor.bDarkness)
		continue;
	memcpy (&lightColor.v.color, &lightP->info.color, sizeof (lightColor.v.color));
	if (lightColor.IsZero ())
		continue;

	if (bTransform) 
		transformation.Transform (lightDir, *lightP->info.vDirf.XYZ (), 1);
	else
		lightDir = *lightP->info.vDirf.XYZ ();

	if (nType < 2)
		DistToFace (*colorData.vertPosP, lightP->info.nSegment, ubyte (lightP->info.nSide), &lightPos);
		//if (fabs (fLightDist) < 1.0f)
		//	fLightDist = 0.0f;
	else 
		lightPos = *lightP->render.vPosf [bTransform].XYZ ();
	lightRayDir = lightPos - *colorData.vertPosP;
	fLightDist = lightRayDir.Mag ();
	lightRayDir /= fLightDist; // normalize

#if DBG
	CFloatVector3 hDir = *lightP->render.vPosf [bTransform].XYZ () - *colorData.vertPosP;
	CFloatVector3::Normalize (hDir);
	float hDot = CFloatVector3::Dot (colorData.vertNorm, hDir);

	if ((nDbgSeg >= 0) && (lightP->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (lightP->info.nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif

	if (IsLightVert (nVertex, lightP)) { // inside the light emitting face
		fLightDist = 0.0f;
		NdotL = 1.0f;
		bDiffuse = 1;
		}
	else if (bDiffuse = lightP->info.bDiffuse [nThread]) {
		if (fLightDist < 1.e-6f) {
			fLightDist = 0.0f;
			lightRayDir = colorData.vertNorm;
			NdotL = 1.0f; // full light contribution for adjacent points
			}
		else {
			if (colorData.vertNorm.IsZero ())
				NdotL = 1.0f;
			else {
				NdotL = CFloatVector3::Dot (colorData.vertNorm, lightRayDir);
				if ((NdotL < 0.0f) && (NdotL > -0.01f)) // compensate for faces almost planar with the light emitting face
					NdotL = 0.0f;
				}
			if ((gameStates.render.nState || (nType < 2)) && (fLightDist > 0.1f)) {
				// check whether the vertex is behind the light or the light shines at the vertice's back
				// if any of these conditions apply, decrease the light radius, chosing the smaller negative angle
				fLightAngle = -CFloatVector3::Dot (lightRayDir, lightDir);
				fLightAngle = (fLightAngle < 0.99f) ? fLightAngle + 0.01f : 1.0f;
				}
			else
				fLightAngle = 1.0f;
			}
		}
	else {
		NdotL = 1.0f;
		fLightDist = X2F (lightP->render.xDistance [nThread]);
		fLightAngle = CFloatVector3::Dot (lightRayDir, lightDir);
		fLightAngle = (fLightAngle < 0.0f) ? 1.0f : 1.0f - fLightAngle; 
		}

#if DBG
	if ((nDbgSeg >= 0) && (lightP->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (lightP->info.nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif

	fLightDist *= ogl.m_states.fLightRange;
	if	(fLightDist <= 0.0f) {
		NdotL = 1.0f;
		fLightDist = 0.0f;
		fAttenuation = 1.0f / lightP->info.fBrightness;
		}
	else {	//make it decay faster
		float decay = min (fLightAngle, NdotL);
		if (decay < 0.0f) {
			decay += 1.0000001f;
			fLightDist /= decay * decay;
			}
		fAttenuation = 1.0f + GEO_LIN_ATT * fLightDist + GEO_QUAD_ATT * fLightDist * fLightDist;
		fAttenuation /= lightP->info.fBrightness;
		}

	vertColor = *gameData.render.vertColor.matAmbient.XYZ ();
	if (bDiffuse) {
		if (lightP->info.bSpot) {
			if (NdotL <= 0.0f)
				continue;
			CFloatVector3::Normalize (lightDir);
			lightRayDir.Neg ();
			spotEffect = CFloatVector3::Dot (lightDir, lightRayDir);

			if (spotEffect <= lightP->info.fSpotAngle)
				continue;
			if (lightP->info.fSpotExponent)
				spotEffect = (float) pow (spotEffect, lightP->info.fSpotExponent);
			fAttenuation /= spotEffect * ogl.m_states.fLightRange;
			vertColor += (*gameData.render.vertColor.matDiffuse.XYZ () * NdotL);
			}
		else {
#if 1
			if (NdotL > 0.0f)
				vertColor += (*gameData.render.vertColor.matDiffuse.XYZ () * NdotL);
#else
			if (NdotL > 0.1f)
				vertColor += (*gameData.render.vertColor.matDiffuse.XYZ () * NdotL);
			else if (NdotL >= 0.0f)
				vertColor += (*gameData.render.vertColor.matDiffuse.XYZ () * 0.1f);
#endif
			else
				NdotL = 0.0f;
			}
#if TEST_AMBIENT > 0
		vertColor.SetZero ();
#elif TEST_AMBIENT < 0
		vertColor.Set (0.0f, 0.0f, 3.0f);
#else
		vertColor *= lightColor;
#endif
		}
	else {
#if TEST_AMBIENT > 0
		vertColor.Set (1.0f, 1.0f, 1.0f);
#elif TEST_AMBIENT < 0
		vertColor.SetZero ();
#else
		// make ambient light behind the light source decay with angle and in front of it full strength 
		vertColor *= lightColor * fLightAngle /** fLightAngle*/;  // may use quadratic dampening to emphasize attenuation do to point being behind light source
#endif
	}

#if 1
	if (bSpecular && bDiffuse && (NdotL > 0.0f) && (fLightDist > 0.0f)) {
		if (!lightP->info.bSpot)	//need direction from light to vertex now
			lightRayDir.Neg ();
		vReflect = CFloatVector3::Reflect (lightRayDir, colorData.vertNorm);
		//CFloatVector3::Normalize (vReflect);
#if DBG
		if ((nDbgVertex >= 0) && (nVertex == nDbgVertex))
			nDbgVertex = nDbgVertex;
#endif
		RdotE = CFloatVector3::Dot (vReflect, vertPos);
		if (RdotE > 0.0f) {
			vertColor += (lightColor * (float) pow (RdotE, colorData.fMatShininess));
			vertColor.Set (0.0f, 0.0f, (float) pow (RdotE, colorData.fMatShininess)); //test
			}
		}
#endif

	vertColor /= fAttenuation;

	if ((nSaturation < 2) || gameStates.render.bHaveLightmaps) {//sum up color components
		colorSum += vertColor;
		}
	else {	//use max. color components
		nBrightness = sqri ((int) (vertColor.v.color.r * 1000)) + sqri ((int) (vertColor.v.color.g * 1000)) + sqri ((int) (vertColor.v.color.b * 1000));
		if (nMaxBrightness < nBrightness) {
			nMaxBrightness = nBrightness;
			colorSum = vertColor;
			}
		else if (nMaxBrightness == nBrightness) {
			if (colorSum.v.color.r < vertColor.v.color.r)
				colorSum.v.color.r = vertColor.v.color.r;
			if (colorSum.v.color.g < vertColor.v.color.g)
				colorSum.v.color.g = vertColor.v.color.g;
			if (colorSum.v.color.b < vertColor.v.color.b)
				colorSum.v.color.b = vertColor.v.color.b;
			}
		}
	j++;
	}
if (j) {
	if ((nSaturation == 1) || gameStates.render.bHaveLightmaps) { //if a color component is > 1, cap color components using highest component value
		float	cMax = colorSum.v.color.r;
		if (cMax < colorSum.v.color.g)
			cMax = colorSum.v.color.g;
		if (cMax < colorSum.v.color.b)
			cMax = colorSum.v.color.b;
		if (cMax > 1) {
			colorSum.v.color.r /= cMax;
			colorSum.v.color.g /= cMax;
			colorSum.v.color.b /= cMax;
			}
		}
	*pColorSum = colorSum;
	}
#if DBG
if (nLights)
	nLights = 0;
#endif
return j;
}

//------------------------------------------------------------------------------

void InitVertColorData (CVertColorData& colorData)
{
	static CFloatVector matSpecular;
	
matSpecular.Set (1.0f, 1.0f, 1.0f, 1.0f);

colorData.bExclusive = !FAST_SHADOWS && (gameStates.render.nShadowPass == 3),
colorData.fMatShininess = 0.0f;
colorData.bMatSpecular = 0;
colorData.bMatEmissive = 0;
colorData.nMatLight = -1;
if (lightManager.Material ().bValid) {
#if 0
	if (lightManager.Material ().emissive.dir.color.r ||
		 lightManager.Material ().emissive.dir.color.g ||
		 lightManager.Material ().emissive.dir.color.b) {
		colorData.bMatEmissive = 1;
		colorData.nMatLight = lightManager.Material ().nLight;
		colorSum = lightManager.Material ().emissive;
		}
#endif
	colorData.bMatSpecular =
		lightManager.Material ().specular.v.color.r ||
		lightManager.Material ().specular.v.color.g ||
		lightManager.Material ().specular.v.color.b;
	if (colorData.bMatSpecular) {
		colorData.matSpecular = lightManager.Material ().specular;
		colorData.fMatShininess = (float) lightManager.Material ().shininess;
		}
	else
		colorData.matSpecular = matSpecular;
	}
else {
	colorData.bMatSpecular = 1;
	colorData.matSpecular = matSpecular;
	colorData.fMatShininess = 64;
	}
}

//------------------------------------------------------------------------------

extern int nDbgVertex;


void G3VertexColor (int nSegment, int nSide, int nVertex,
						  CFloatVector3 *pvVertNorm, CFloatVector3 *pVertPos, 
						  tFaceColor *pVertColor, tFaceColor *pBaseColor,
						  float fScale, int bSetColor, int nThread)
{
PROF_START
	CFloatVector3	colorSum = CFloatVector3::Create(0.0f, 0.0f, 0.0f);
	CFloatVector3	vertPos;
	tFaceColor*		pc = NULL;
	int				bVertexLights;
	CVertColorData	colorData;

InitVertColorData (colorData);
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
#if 1//!DBG //cache light values per frame
if (!(gameStates.render.nState || colorData.bExclusive || colorData.bMatEmissive) && (nVertex >= 0)) {
	pc = gameData.render.color.vertices + nVertex;
	if (pc->index == gameStates.render.nFrameFlipFlop + 1) {
		if (pVertColor) {
			pVertColor->index = gameStates.render.nFrameFlipFlop + 1;
			pVertColor->color.red = pc->color.red * fScale;
			pVertColor->color.green = pc->color.green * fScale;
			pVertColor->color.blue = pc->color.blue * fScale;
			pVertColor->color.alpha = 1;
			}
		if (bSetColor > 0)
			OglColor4sf (pc->color.red * fScale, pc->color.green * fScale, pc->color.blue * fScale, 1.0);
		else if (bSetColor < 0) {
			pc->color.red *= gameData.render.fBrightness;
			pc->color.green *= gameData.render.fBrightness;
			pc->color.blue *= gameData.render.fBrightness;
			}
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
if (ogl.m_states.bUseTransform)
#if 1
	colorData.vertNorm = *pvVertNorm;
#else
	CFloatVector::Normalize (&colorData.vertNorm, pvVertNorm);
#endif
else {
	if (gameStates.render.nState)
		transformation.Rotate (colorData.vertNorm, *pvVertNorm, 0);
	else {
		colorData.vertNorm = *pvVertNorm;
		CFloatVector3::Normalize (colorData.vertNorm);
		}
	}
if ((bVertexLights = !(gameStates.render.nState || pVertColor))) {
	vertPos.Assign (gameData.segs.vertices [nVertex]);
	pVertPos = &vertPos;
	lightManager.SetNearestToVertex (nSegment, nSide, nVertex, NULL, 1, 0, 1, nThread);
	}
colorData.vertPosP = pVertPos;
//VmVecNegate (&vertNorm);
//if (nLights)
#if MULTI_THREADED_LIGHTS
if (gameStates.app.bMultiThreaded) {
	SDL_SemPost (gameData.threads.vertColor.info [0].exec);
	SDL_SemPost (gameData.threads.vertColor.info [1].exec);
	SDL_SemWait (gameData.threads.vertColor.info [0].done);
	SDL_SemWait (gameData.threads.vertColor.info [1].done);
	VmVecAdd (&colorSum, colorData.colorSum, colorData.colorSum + 1);
	}
else
#endif
#if 1
if (gameStates.app.bEndLevelSequence >= EL_OUTSIDE) {
	colorSum.v.color.r =
	colorSum.v.color.g =
	colorSum.v.color.b = 1;
	}
else
#endif
 {
	if (lightManager.Index (0, nThread).nActive) {
		if (pBaseColor)
			memcpy (&colorSum, &pBaseColor->color, sizeof (colorSum));
#if DBG
		if (!gameStates.render.nState && (nVertex == nDbgVertex))
			nVertex = nVertex;
#endif
		G3AccumVertColor (nVertex, &colorSum, &colorData, nThread);
		}
	if ((nVertex >= 0) && !(gameStates.render.nState || gameData.render.vertColor.bDarkness)) {
		tFaceColor *pfc = gameData.render.color.ambient + nVertex;
		colorSum.v.color.r += pfc->color.red * fScale;
		colorSum.v.color.g += pfc->color.green * fScale;
		colorSum.v.color.b += pfc->color.blue * fScale;
#if DBG
		if (!gameStates.render.nState && (nVertex == nDbgVertex) && (colorSum.v.color.r + colorSum.v.color.g + colorSum.v.color.b < 0.1f))
			nVertex = nVertex;
#endif
		}
	if (colorSum.v.color.r > 1.0)
		colorSum.v.color.r = 1.0;
	if (colorSum.v.color.g > 1.0)
		colorSum.v.color.g = 1.0;
	if (colorSum.v.color.b > 1.0)
		colorSum.v.color.b = 1.0;
	}
#if ONLY_HEADLIGHT
if (gameData.render.lights.dynamic.headlights.nLights)
	colorSum.dir.color.r = colorSum.dir.color.g = colorSum.dir.color.b = 0;
#endif
colorSum *= gameData.render.fBrightness;
if (bSetColor)
	OglColor4sf (colorSum.v.color.r * fScale, colorSum.v.color.g * fScale, colorSum.v.color.b * fScale, 1.0);
#if 1
if (!colorData.bMatEmissive && pc) {
	pc->index = gameStates.render.nFrameFlipFlop + 1;
	pc->color.red = colorSum.v.color.r;
	pc->color.green = colorSum.v.color.g;
	pc->color.blue = colorSum.v.color.b;
	}
if (pVertColor) {
	pVertColor->index = gameStates.render.nFrameFlipFlop + 1;
	pVertColor->color.red = colorSum.v.color.r * fScale;
	pVertColor->color.green = colorSum.v.color.g * fScale;
	pVertColor->color.blue = colorSum.v.color.b * fScale;
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
