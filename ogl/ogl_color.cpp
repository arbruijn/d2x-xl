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
#define CHECK_LIGHT_VERT	0
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

#define GEO_LIN_ATT	(/*0.0f*/ gameData.renderData.fAttScale [0])
#define GEO_QUAD_ATT	(/*0.003333f*/ gameData.renderData.fAttScale [1])
#define OBJ_LIN_ATT	(/*0.0f*/ gameData.renderData.fAttScale [0])
#define OBJ_QUAD_ATT	(/*0.003333f*/ gameData.renderData.fAttScale [1])

//------------------------------------------------------------------------------

CFaceColor lightColor;
CFaceColor tMapColor;
CFaceColor vertColors [8];

CFloatVector shadowColor [2] = {{{{1.0f, 0.0f, 0.0f, 0.8f}}}, {{{0.0f, 0.0f, 1.0f, 0.8f}}}};
CFloatVector modelColor [2] = {{{{0.0f, 0.5f, 1.0f, 0.5f}}}, {{{0.0f, 1.0f, 0.5f, 0.5f}}}};

//------------------------------------------------------------------------------

void OglPalColor (CPalette *palette, int32_t c, CFloatVector* pColor)
{
	CFloatVector	color;

if (c < 0) {
	if (pColor)
		pColor->Red () =
		pColor->Green () =
		pColor->Blue () =
		pColor->Alpha () = 1.0;
	else
		glColor3f (1.0, 1.0, 1.0);
	}
else {
	if (!palette)
		palette = paletteManager.Game ();
	palette->ToRgbaf (c, color);
	color.Alpha () = gameStates.render.grAlpha;
	if (pColor)
		*pColor = color;
	else
		glColor4fv (reinterpret_cast<GLfloat*> (&color));
	}
}

//------------------------------------------------------------------------------

CFloatVector GetPalColor (CPalette *palette, int32_t c)
{
	CFloatVector color;

if (c < 0)
	color.Set (1.0f, 1.0f, 1.0f, 1.0f);
else {
	if (!palette)
		palette = paletteManager.Game ();
	palette->ToRgbaf (c, color);
	color.Alpha () = gameStates.render.grAlpha;
	}
return color;
}

//------------------------------------------------------------------------------

void OglCanvasColor (CCanvasColor* pCanvColor, CFloatVector* pColor)
{
	CFloatVector	color;

if (!pCanvColor) {
	if (pColor) {
		pColor->Red () =
		pColor->Green () =
		pColor->Blue () = 1.0f;
		pColor->Alpha () = gameStates.render.grAlpha;
		}
	else 
		glColor4f (1.0f, 1.0f, 1.0f, gameStates.render.grAlpha);
	}
else if (pCanvColor->rgb) {
	color.Set (pCanvColor->Red (), pCanvColor->Green (), pCanvColor->Blue ());
	color /= 255.0f;
	color.Alpha () = float (pCanvColor->Alpha ()) / 255.0f * gameStates.render.grAlpha;
	if (color.Alpha () < 1.0f)
		ogl.SetBlendMode (ogl.m_data.nSrcBlendMode, ogl.m_data.nDestBlendMode);
	if (pColor)
		*pColor = color;
	else
		glColor4fv (reinterpret_cast<GLfloat*> (&color));
	}
else
	OglPalColor (paletteManager.Game (), pCanvColor->index, pColor);
}

//------------------------------------------------------------------------------

CFloatVector GetCanvasColor (CCanvasColor *pColor)
{

if (!pColor) {
	CFloatVector	color = {{{1, 1, 1, gameStates.render.grAlpha}}};
	return color;
	}
else if (pColor->rgb) {
	CFloatVector color = {{{
		float (pColor->Red ()) / 255.0f,
		float (pColor->Green ()) / 255.0f,
		float (pColor->Blue ()) / 255.0f,
		float (pColor->Alpha ()) / 255.0f * gameStates.render.grAlpha
		}}};
	if (pColor->Alpha () < 1.0f) {
		ogl.SetBlending (true);
		ogl.SetBlendMode (ogl.m_data.nSrcBlendMode, ogl.m_data.nDestBlendMode);
		}
	return color;
	}
else
	return GetPalColor (paletteManager.Game (), pColor->index);
}

//------------------------------------------------------------------------------

// cap tMapColor scales the color values in tMapColor so that none of them exceeds
// 1.0 if multiplied with any of the current face's corners' brightness values.
// To do that, first the highest corner brightness is determined.
// In the next step, color values are increased to match the max. brightness.
// Then it is determined whether any color value multiplied with the max. brightness would
// exceed 1.0. If so, all three color values are scaled so that their maximum multiplied
// with the max. brightness does not exceed 1.0.

inline void CapTMapColor (tUVL *uvlList, int32_t nVertices, CBitmap *bm)
{
#if 0
	CFaceColor *color = tMapColor.index ? &tMapColor : lightColor.index ? &lightColor : NULL;

if (! (bm->props.flags & BM_FLAG_NO_LIGHTING) && color) {
		double	vec, mat = tMapColor.Red ();
		double	h, l = 0;
		int32_t		i;

	for (i = 0; i < nVertices; i++, uvlList++) {
		h = (bm->props.flags & BM_FLAG_NO_LIGHTING) ? 1.0 : X2F (uvlList->l);
		if (l < h)
			l = h;
		}

	// scale brightness with the average color to avoid darkening bright areas with the color
	vec = (color->Red () + color->Green () + color->Blue ()) / 3;
	if (mat < color->Green ())
		mat = color->Green ();
	if (mat < color->Blue ())
		mat = color->Blue ();
	l = l / vec;
	// prevent any color component getting over 1.0
	if (l * mat > 1.0)
		l = 1.0 / mat;
	color->Red () *= l;
	color->Green () *= l;
	color->Blue () *= l;
	}
#endif
}

//------------------------------------------------------------------------------

static inline void ScaleColor (CFaceColor *pColor, float l)
{
if (l >= 0) {
		float m = pColor->Red ();

	if (m < pColor->Green ())
		m = pColor->Green ();
	if (m < pColor->Blue ())
		m = pColor->Blue ();
	if (m > 0.0f)
		*pColor *= l / m;
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
void SetTMapColor (tUVL *uvlList, int32_t i, CBitmap *pBm, int32_t bResetColor, CFloatVector* pColor)
{
	float l = (pBm->Flags () & BM_FLAG_NO_LIGHTING) ? 1.0f : X2F (uvlList->l);
	float s = 1.0f;

l *= gameData.renderData.fBrightness;
if (ogl.m_states.bScaleLight)
	s *= gameStates.render.bHeadlightOn ? 0.4f : 0.3f;
if (gameStates.app.bEndLevelSequence >= EL_OUTSIDE) {
	if (!pColor)
		OglColor4sf (l, l, l, s);
	else {
		pColor->Red () =
		pColor->Green () =
		pColor->Blue () = l;
		pColor->Alpha () = s;
		}
	}
else if (pColor) {
	if (tMapColor.index) {
		ScaleColor (&tMapColor, l);
		pColor->Assign (tMapColor);
		*pColor *= gameData.renderData.fBrightness;
		if (l >= 0)
			tMapColor.Set (1.0f, 1.0f, 1.0f);
		}
	else if (i >= (int32_t) (sizeofa (vertColors)))
		return;
	else if (vertColors [i].index) {
			CFaceColor* pVertexColor = vertColors + i;

		pColor->Assign (vertColors [i]);
		*pColor *= gameData.renderData.fBrightness;
		if (bResetColor)
			pVertexColor->Set (1.0f, 1.0f, 1.0f);
		}
	else {
		pColor->Set (1.0f, 1.0f, 1.0f);
		}
	pColor->Alpha () = s;
	}
else {
	if (tMapColor.index) {
		ScaleColor (&tMapColor, l);
		OglColor4sf (tMapColor.Red (), tMapColor.Green (), tMapColor.Blue (), s);
		if (l >= 0)
			tMapColor.Set (1.0f, 1.0f, 1.0f);
		}
	else if (i >= (int32_t) (sizeofa (vertColors)))
		return;
	else if (vertColors [i].index) {
			CFaceColor* pVertexColor = vertColors + i;

		OglColor4sf (pVertexColor->Red (), pVertexColor->Green (), pVertexColor->Blue (), s);
		if (bResetColor) {
			pVertexColor->Set (1.0f, 1.0f, 1.0f);
			pVertexColor->index = 0;
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

inline int32_t sqri (int32_t i)
{
return i * i;
}

//------------------------------------------------------------------------------

#if CHECK_LIGHT_VERT

static inline int32_t IsLightVert (int32_t nVertex, CDynLight *pLight)
{
if ((nVertex >= 0) && pLight->info.pFace) {
	uint16_t *pv = gameStates.render.bTriangleMesh ? pLight->info.pFace->triIndex : pLight->info.pFace->m_info.index;
	for (int32_t i = pLight->info.pFace->m_info.nVerts; i; i--, pv++)
		if (*pv == (uint16_t) nVertex)
			return 1;
	}
return 0;
}

#endif

//------------------------------------------------------------------------------

#define VECMAT_CALLS 0

float fLightRanges [5] = {0.5f, 0.7071f, 1.0f, 1.4142f, 2.0f};

int32_t ComputeVertexColor (int32_t nSegment, int32_t nSide, int32_t nVertex, CFloatVector3 *colorSumP, CVertColorData *colorDataP, int32_t nThread)
{
	CDynLight*			pLight;
	CDynLightIndex*	pLightIndex = &lightManager.Index (0, nThread);
	CActiveDynLight*	pActiveLights = lightManager.Active (nThread) + pLightIndex->nFirst;
	CVertColorData		colorData = *colorDataP;
	int32_t				i, j, nType, nLights, 
							bSelf, nSelf,
							bSkipHeadlight = gameOpts->ogl.bHeadlight && !gameStates.render.nState,
							bTransform = (gameStates.render.nState > 0) && !ogl.m_states.bUseTransform;
	int32_t				bDiffuse, bSpecular = gameStates.render.bSpecularColor && (colorData.fMatShininess > 0.0f);
	float					fLightDist, fAttenuation, fLightAngle, spotEffect, NdotL, RdotE;
	CFloatVector3		lightDir, lightRayDir, lightPos, vertPos, vReflect;
	CFloatVector3		lightColor, colorSum [2], vertColor;
	
#if DBG
if (nThread == 0)
	BRP;
if (nThread == 1)
	BRP;
#endif
colorSum [0] = *colorSumP;
colorSum [1].SetZero ();
vertPos = *transformation.m_info.posf [1].XYZ () - *colorData.pVertPos;
CFloatVector3::Normalize (vertPos);
nLights = pLightIndex->nActive;
if (nLights > lightManager.LightCount (0))
	nLights = lightManager.LightCount (0);
i = pLightIndex->nLast - pLightIndex->nFirst + 1;
#if DBG
if ((nDbgVertex >= 0) && (nVertex == nDbgVertex))
	BRP;
#endif
nSelf = 0;
bSelf = 0;
for (j = 0; (i > 0) && (nLights > 0); pActiveLights++, i--) {
#if 1
	if (!(pLight = pActiveLights->pLight))
#else
	if (!(pLight = GetActiveRenderLight (pActiveLights, nThread)))
#endif
		continue;
	nLights--;
#if DBG
	if ((nDbgVertex >= 0) && (nVertex == nDbgVertex))
		BRP;
#endif
#if DBG
	if ((nDbgSeg >= 0) && (pLight->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pLight->info.nSide == nDbgSide)))
		BRP;
#	if 0
	else
		continue;
#	endif
#endif
	if (!pLight->render.bState)
		continue;
#if 0
	if (i == colorData.nMatLight)
		continue;
#endif
	nType = pLight->render.nType;
	if (bSkipHeadlight && (nType == 3))
		continue;
#if ONLY_HEADLIGHT
	if (nType != 3)
		continue;
#endif
	if (pLight->info.bVariable && gameData.renderData.vertColor.bDarkness)
		continue;
	memcpy (&lightColor.v.color, &pLight->info.color, sizeof (lightColor.v.color));
	if (lightColor.IsZero ())
		continue;

	if (!gameStates.render.bHaveLightmaps && (bSelf = pLight->info.bAmbient)) {
		if ((pLight->Segment () >= 0) && !SEGMENT (pLight->Segment ())->HasVertex ((uint8_t) pLight->Side (), nVertex))
			continue;
#if DBG
		if ((nDbgVertex >= 0) && (nVertex == nDbgVertex))
			BRP;
#endif
		nSelf++;
		}

	if (bTransform) 
		transformation.Transform (lightDir, *pLight->info.vDirf.XYZ (), 1);
	else
		lightDir = *pLight->info.vDirf.XYZ ();

	if (bSelf || (gameStates.render.bBuildLightmaps && (pLight->Segment () == nSegment) && (pLight->Side () == nSide))) {
		lightPos = *colorData.pVertPos;
		fLightDist = 0.0f;
		lightRayDir.SetZero ();
		}
	else {
		if (nType < 2)
			DistToFace (*colorData.pVertPos, pLight->info.nSegment, uint8_t (pLight->info.nSide), &lightPos); // returns 0 if a plane normal through the vertex position goes through the face
		else 
			lightPos = *pLight->render.vPosf [bTransform].XYZ ();
		lightRayDir = lightPos - *colorData.pVertPos;
		fLightDist = lightRayDir.Mag ();
		}

#if DBG
	if ((nDbgVertex >= 0) && (nVertex == nDbgVertex))
		BRP;
#endif
#if 0 //DBG
	CFloatVector3 hDir = *pLight->render.vPosf [bTransform].XYZ () - *colorData.pVertPos;
	CFloatVector3::Normalize (hDir);
	float hDot = CFloatVector3::Dot (colorData.vertNorm, hDir);

	if ((nDbgSeg >= 0) && (pLight->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pLight->info.nSide == nDbgSide)))
		BRP;
#endif

	if (fLightDist == 0.0f) {
		fLightAngle = 1.0f;
		NdotL = 1.0f;
		bDiffuse = 1;
		lightRayDir = colorData.vertNorm;
		}
	else {
		lightRayDir /= fLightDist; // normalize
		if ((bDiffuse = pLight->info.bDiffuse [nThread])) {
			if (colorData.vertNorm.IsZero ())
				NdotL = 1.0f;
			else {
				if (nType > 1)
					NdotL = CFloatVector3::Dot (colorData.vertNorm, lightRayDir);
				else {
					// for rectangular light sources, using the light source's closest point to the light receiving point
					// can yield a incident angle too low if the point lies in an adjacent face. In that case use the angle
					// from the light source's center if it is better
					CFloatVector3 d = *pLight->render.vPosf [bTransform].XYZ () - *colorData.pVertPos;
					CFloatVector3::Normalize (d);
					float dotCenter = CFloatVector3::Dot (colorData.vertNorm, d);
					float dotNearest = CFloatVector3::Dot (colorData.vertNorm, lightRayDir);
					NdotL = (dotCenter > dotNearest) ? dotCenter : dotNearest;
					}
				if ((NdotL < 0.0f) && (NdotL > -0.01f)) // compensate for faces almost planar with the light emitting face
					NdotL = 0.0f;
				}
			if ((gameStates.render.nState || (nType < 2)) && (fLightDist > 0.1f)) {
				fLightAngle = -CFloatVector3::Dot (lightRayDir, lightDir);
				if (fLightAngle >= -0.01f) // compensate for faces almost planar with the light emitting face
					fLightAngle += (1.0f - fLightAngle) / fLightDist; 
				}
			else
				fLightAngle = 1.0f;
			}
		else {
			NdotL = 1.0f;
			fLightAngle = (1.0f + CFloatVector3::Dot (colorData.vertNorm, lightDir)) / 2.0f;
			//fLightDist *= 2.0f;
			fLightDist = X2F (pLight->render.xDistance [nThread]);
			//fLightAngle = (fLightAngle < 0.0f) ? 1.0f : 1.0f - fLightAngle; 
			}
		}

#if DBG
	if ((nDbgSeg >= 0) && (pLight->info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pLight->info.nSide == nDbgSide)))
		BRP;
#endif

	fLightDist *= ogl.m_states.fLightRange;
	if	(fLightDist <= 0.0f) {
		NdotL = 1.0f;
		fLightDist = 0.0f;
		fAttenuation = 1.0f / pLight->info.fBrightness;
		}
	else {	//make it decay faster
		float decay = Min (fLightAngle, NdotL);
		if (decay < 0.0f) {
			decay += 1.0000001f;
			fLightDist /= decay * decay;
			}
		fAttenuation = 1.0f + GEO_LIN_ATT * fLightDist + GEO_QUAD_ATT * fLightDist * fLightDist;
		fAttenuation /= pLight->info.fBrightness;
		}

	vertColor = *gameData.renderData.vertColor.matAmbient.XYZ ();
	if (!bDiffuse) {
		// make ambient light behind the light source decay with angle and in front of it full strength 
		vertColor *= fLightAngle /** fLightAngle*/;  // may use quadratic dampening to emphasize attenuation due to point being behind light source
		}
	else {
		if (pLight->info.bSpot) {
			if (NdotL <= 0.0f)
				continue;
			CFloatVector3::Normalize (lightDir);
			lightRayDir.Neg ();
			spotEffect = CFloatVector3::Dot (lightDir, lightRayDir);

			if (spotEffect <= pLight->info.fSpotAngle)
				continue;
			if (pLight->info.fSpotExponent)
				spotEffect = (float) pow (spotEffect, pLight->info.fSpotExponent);
			fAttenuation /= spotEffect * ogl.m_states.fLightRange;
			vertColor += (*gameData.renderData.vertColor.matDiffuse.XYZ () * NdotL);
			}
		else {
			if (NdotL < -0.01f)
				NdotL = 0.0f;
			else {
#if 1
				// increase brightness for nearby points regardless of the angle between the light direction and the point's normal
				// to prevent big faces with small light textures on them being much brighter than their adjacent faces
				// because D2X-XL will in such cases consider the entire face to be emitting light
				if (fLightDist <= 1.0f)
					NdotL = 1.0f;
				else
					NdotL += (1.0f - NdotL) / fLightDist; 
#endif
				vertColor += (*gameData.renderData.vertColor.matDiffuse.XYZ () * NdotL); 
				}
			}
#if 1
		if (bSpecular && (NdotL > 0.0f) && (fLightDist > 0.0f)) {
			if (!pLight->info.bSpot)	//need direction from light to vertex now
				lightRayDir.Neg ();
			vReflect = CFloatVector3::Reflect (lightRayDir, colorData.vertNorm);
			//CFloatVector3::Normalize (vReflect);
#	if DBG
			if ((nDbgVertex >= 0) && (nVertex == nDbgVertex))
				BRP;
#	endif
			RdotE = CFloatVector3::Dot (vReflect, vertPos);
			if (RdotE > 0.0f) {
				vertColor += (lightColor * (float) pow (RdotE, colorData.fMatShininess));
				}
			}
#endif
		}

	vertColor *= lightColor;
	vertColor /= fAttenuation;

#if 1
	colorSum [bSelf] += vertColor;
#else
	if ((nSaturation < 2) || gameStates.render.bHaveLightmaps)//sum up color components
		colorSum += vertColor;
	else {	//use max. color components
		int32_t nBrightness = sqri ((int32_t) (vertColor.Red () * 1000)) + sqri ((int32_t) (vertColor.Green () * 1000)) + sqri ((int32_t) (vertColor.Blue () * 1000));
		if (nMaxBrightness < nBrightness) {
			nMaxBrightness = nBrightness;
			colorSum = vertColor;
			}
		else if (nMaxBrightness == nBrightness) {
			if (colorSum.Red () < vertColor.Red ())
				colorSum.Red () = vertColor.Red ();
			if (colorSum.Green () < vertColor.Green ())
				colorSum.Green () = vertColor.Green ();
			if (colorSum.Blue () < vertColor.Blue ())
				colorSum.Blue () = vertColor.Blue ();
			}
		}
#endif
	j++;
	}

#if DBG
if ((nDbgVertex >= 0) && (nVertex == nDbgVertex))
	BRP;
#endif
	
if (j) {
	if (nSelf)
		colorSum [0] += colorSum [1] / float (nSelf);
	//if ((nSaturation == 1) || gameStates.render.bHaveLightmaps) 
		{ //if a color component is > 1, cap color components using highest component value
		float	maxColor = colorSum [0].Max ();
		if (maxColor > 1.0f) {
			colorSum [0].Red () /= maxColor;
			colorSum [0].Green () /= maxColor;
			colorSum [0].Blue () /= maxColor;
			}
		}
	*colorSumP = colorSum [0];
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
		lightManager.Material ().specular.Red () ||
		lightManager.Material ().specular.Green () ||
		lightManager.Material ().specular.Blue ();
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

extern int32_t nDbgVertex;


void GetVertexColor (int32_t nSegment, int32_t nSide, int32_t nVertex,
						   CFloatVector3* vVertNormP, CFloatVector3* vVertPosP, 
						   CFaceColor* pVertexColor, CFaceColor* baseColorP,
						   float fScale, int32_t bSetColor, int32_t nThread)
{
PROF_START
	CFloatVector3	colorSum = CFloatVector3::Create (0.0f, 0.0f, 0.0f);
	CFloatVector3	vertPos;
	CFaceColor*		pColor = NULL;
	int32_t				bVertexLights;
	CVertColorData	colorData;

InitVertColorData (colorData);
#if DBG
if (!gameStates.render.nState && (nVertex == nDbgVertex))
	BRP;
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
	pColor = gameData.renderData.color.vertices + nVertex;
	if (pColor->index == gameStates.render.nFrameFlipFlop + 1) {
		if (pVertexColor)
#pragma omp critical (GetVertexColor1)
			{
			pVertexColor->index = gameStates.render.nFrameFlipFlop + 1;
			pVertexColor->Assign (*pColor);
			*pVertexColor *= fScale;
			pVertexColor->Alpha () = 1;
			}
		if (bSetColor > 0)
			OglColor4sf (pColor->Red () * fScale, pColor->Green () * fScale, pColor->Blue () * fScale, 1.0);
		else if (bSetColor < 0) {
			*pColor *= gameData.renderData.fBrightness;
			}
#if DBG
		if (!gameStates.render.nState && (nVertex == nDbgVertex))
			BRP;
#endif
PROF_END(ptVertexColor)
		return;
		}
	}
#endif
#if DBG
if (!gameStates.render.nState && (nVertex == nDbgVertex))
	BRP;
#endif
if (ogl.m_states.bUseTransform)
	colorData.vertNorm = *vVertNormP;
else {
	if (gameStates.render.nState)
		transformation.Rotate (colorData.vertNorm, *vVertNormP, 0);
	else {
		colorData.vertNorm = *vVertNormP;
		CFloatVector3::Normalize (colorData.vertNorm);
		}
	}
if ((bVertexLights = !(gameStates.render.nState || pVertexColor))) {
	vertPos.Assign (gameData.segData.vertices [nVertex]);
	vVertPosP = &vertPos;
	lightManager.SetNearestToVertex (nSegment, nSide, nVertex, NULL, 1, 0, 1, nThread);
	}
colorData.pVertPos = vVertPosP;

#if MULTI_THREADED_LIGHTS
if (gameStates.app.bMultiThreaded) {
	SDL_SemPost (gameData.threadData.vertColor.info [0].exec);
	SDL_SemPost (gameData.threadData.vertColor.info [1].exec);
	SDL_SemWait (gameData.threadData.vertColor.info [0].done);
	SDL_SemWait (gameData.threadData.vertColor.info [1].done);
	VmVecAdd (&colorSum, colorData.colorSum, colorData.colorSum + 1);
	}
else
#endif

if (gameStates.app.bEndLevelSequence >= EL_OUTSIDE) {
	colorSum.Red () =
	colorSum.Green () =
	colorSum.Blue () = 1.0f;
	}
else {
	if (lightManager.Index (0, nThread).nActive) {
		if (baseColorP)
			colorSum.Assign (*baseColorP);
#if DBG
		if (!gameStates.render.nState && (nVertex == nDbgVertex))
			BRP;
#endif
		ComputeVertexColor (nSegment, nSide, nVertex, &colorSum, &colorData, nThread);
		}
	if ((nVertex >= 0) && !(gameStates.render.nState || gameData.renderData.vertColor.bDarkness)) {
		colorSum += *gameData.renderData.color.ambient [nVertex].XYZ () * fScale;
#if DBG
		if (!gameStates.render.nState && (nVertex == nDbgVertex) && (colorSum.Sum () < 0.1f))
			BRP;
#endif
		}
	colorSum *= gameData.renderData.fBrightness;
	if (colorSum.Red () > 1.0)
		colorSum.Red () = 1.0;
	if (colorSum.Green () > 1.0)
		colorSum.Green () = 1.0;
	if (colorSum.Blue () > 1.0)
		colorSum.Blue () = 1.0;
	}
if (bSetColor)
	OglColor4sf (colorSum.Red () * fScale, colorSum.Green () * fScale, colorSum.Blue () * fScale, 1.0);

if (!colorData.bMatEmissive && pColor) 
#pragma omp critical (GetVertexColor2)
	{
	pColor->Assign (colorSum);
	pColor->index = gameStates.render.nFrameFlipFlop + 1;
	}
if (pVertexColor) 
#pragma omp critical (GetVertexColor3)
	{
	colorSum *= fScale;
	pVertexColor->Assign (colorSum);
	pVertexColor->Alpha () = 1;
	pVertexColor->index = gameStates.render.nFrameFlipFlop + 1;
	}

#if DBG
if (!gameStates.render.nState && (nVertex == nDbgVertex))
	BRP;
#endif
PROF_END(ptVertexColor)
}

//------------------------------------------------------------------------------
