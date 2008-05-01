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
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef __macosx__
# include <stdlib.h>
# include <SDL/SDL.h>
#else
# include <malloc.h>
# include <SDL.h>
#endif

#include "inferno.h"
#include "error.h"
#include "maths.h"
#include "light.h"
#include "dynlight.h"
#include "headlight.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texture.h"
#include "ogl_shader.h"
#include "ogl_render.h"
#include "ogl_fastrender.h"
#include "ogl_tmu.h"
#include "texmerge.h"
#include "transprender.h"
#include "gameseg.h"

#define MAX_PP_LIGHTS_PER_FACE 32
#define MAX_PP_LIGHTS_PER_PASS 1

//------------------------------------------------------------------------------

GLhandleARB gsShaderProg = 0;
GLhandleARB gsf = 0; 
GLhandleARB gsv = 0; 

char *grayScaleFS =
	"uniform sampler2D baseTex;\r\n" \
	"void main(void){" \
	"vec4 texColor = texture2D (baseTex, gl_TexCoord [0].xy);\r\n" \
	"float l = (texColor.r + texColor.g + texColor.b) / 4.0;\r\n" \
	"gl_FragColor = vec4 (l, l, l, texColor.a);}";

char *grayScaleVS =
	"void main(void){" \
	"gl_TexCoord [0]=gl_MultiTexCoord0;"\
	"gl_Position=ftransform();"\
	"gl_FrontColor=gl_Color;}";

//-------------------------------------------------------------------------

void InitGrayScaleShader (void)
{
if (!gameStates.ogl.bShadersOk)
	gameOpts->ogl.bGlTexMerge = 0;
else {
	PrintLog ("building grayscale shader programs\n");
	if (gsShaderProg)
		DeleteShaderProg (&gsShaderProg);
	gameStates.render.textures.bHaveGrayScaleShader = 
		CreateShaderProg (&gsShaderProg) &&
		CreateShaderFunc (&gsShaderProg, &gsf, &gsv, grayScaleFS, grayScaleVS, 1) &&
		LinkShaderProg (&gsShaderProg);
	}
}

//------------------------------------------------------------------------------

#define FACE_BUFFER_SIZE	100

typedef struct tFaceBuffer {
	grsBitmap	*bmP;
	short			nFaces;
	short			nElements;
	int			bTextured;
	ushort		index [FACE_BUFFER_SIZE * 4];
} tFaceBuffer;

static tFaceBuffer faceBuffer = {NULL, 0, 0, 0, {0}};

//------------------------------------------------------------------------------

void G3FlushFaceBuffer (int bForce)
{
#if G3_BUFFER_FACES
if ((faceBuffer.nFaces && bForce) || (faceBuffer.nFaces >= FACE_BUFFER_SIZE)) {
	glDrawElements (GL_TRIANGLE_FAN, faceBuffer.nElements, GL_UNSIGNED_SHORT, faceBuffer.index);
	faceBuffer.nFaces = 
	faceBuffer.nElements = 0;
	}
#endif
}

//------------------------------------------------------------------------------

void G3FillFaceBuffer (grsFace *faceP, int bTextured)
{
	int	i, j = faceP->nIndex;

faceBuffer.bmP = faceP->bmBot;
faceBuffer.bTextured = bTextured;
for (i = 4; i; i--)
	faceBuffer.index [faceBuffer.nElements++] = j++;
faceBuffer.nFaces++;
}

//------------------------------------------------------------------------------

int OglVertexLight (int nLights, int nPass, int iVertex)
{
	tActiveShaderLight	*activeLightsP;
	tShaderLight			*psl;
	int						iLight;
	tRgbaColorf				color = {1,1,1,1};
	GLenum					hLight;

if (nLights < 0)
	nLights = gameData.render.lights.dynamic.shader.nActiveLights [iVertex];
if (nLights) {
	if (nPass) {
		glActiveTexture (GL_TEXTURE1);
		glBlendFunc (GL_ONE, GL_ONE);
		glActiveTexture (GL_TEXTURE0);
		glBlendFunc (GL_ONE, GL_ONE);
		}
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
#if 0
else
	glColor4f (0,0,0,0);
#endif
activeLightsP = gameData.render.lights.dynamic.shader.activeLights [iVertex];
for (iLight = 0; (iLight < 8) && (nLights > 0); activeLightsP++) { 
	if (!(psl = GetActiveShaderLight (activeLightsP, iVertex)))
		continue;
#if 0
	if (psl->info.nType > 1) {
		iLight--;
		continue;
		}
#endif
	nLights--;
	hLight = GL_LIGHT0 + iLight++;
	glEnable (hLight);
	color.red = psl->info.color.red;
	color.green = psl->info.color.green;
	color.blue = psl->info.color.blue;
//			sprintf (szLightSources + strlen (szLightSources), "%d ", (psl->nObject >= 0) ? -psl->nObject : psl->nSegment);
	glLightfv (hLight, GL_POSITION, (GLfloat *) psl->vPosf);
	glLightfv (hLight, GL_DIFFUSE, (GLfloat *) &color);
	glLightfv (hLight, GL_SPECULAR, (GLfloat *) &color);
	if (psl->info.nType == 2) {
		glLightf (hLight, GL_CONSTANT_ATTENUATION,1.0f);
		glLightf (hLight, GL_LINEAR_ATTENUATION, 0.01f / psl->info.fBrightness);
		glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.001f / psl->info.fBrightness);
		}
	else {
		glLightf (hLight, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf (hLight, GL_LINEAR_ATTENUATION, 0.01f / psl->info.fBrightness);
		glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.001f / psl->info.fBrightness);
		}
	}
for (; iLight < 8; iLight++)
	glDisable (GL_LIGHT0 + iLight);
return nLights;
}

//------------------------------------------------------------------------------

int G3SetupFaceLight (grsFace *faceP, int bTextured)
{
	int	h, i, nLights = gameData.render.lights.dynamic.shader.nActiveLights [0];

for (h = i = 0; i < 4; i++) {
	if (i) {
		memcpy (gameData.render.lights.dynamic.shader.activeLights + i, 
					gameData.render.lights.dynamic.shader.activeLights,
					nLights * sizeof (void *));
		gameData.render.lights.dynamic.shader.nActiveLights [i] = nLights;
		}
	SetNearestVertexLights (faceP->index [i], NULL, 0, 0, 1, i);
	h += gameData.render.lights.dynamic.shader.nActiveLights [i];
	
	}
if (h)
   OglEnableLighting (0);
return h;
}

//------------------------------------------------------------------------------

extern GLhandleARB headlightShaderProgs [4];
extern GLhandleARB perPixelLightingShaderProgs [MAX_LIGHTS_PER_PIXEL][4];


//------------------------------------------------------------------------------

int G3SetupPerPixelLighting (grsFace *faceP, int bColorKey, int bMultiTexture, int bTextured)
{
	int						nLightRange, nLights;
	float						fBrightness;
	tRgbaColorf				ambient, diffuse;
	tRgbaColorf				black = {0,0,0,0};
	tRgbaColorf				specular = {0.5f,0.5f,0.5f,0.5f};
	fVector					vPos = {{0,0,0,1}};
	GLenum					hLight;
	tActiveShaderLight	*activeLightsP;
	tShaderLight			*psl;

#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if (!gameStates.ogl.iLight) {
	gameStates.ogl.nLights = SetNearestFaceLights (faceP, bTextured);
	gameStates.ogl.nFirstLight = gameData.render.lights.dynamic.shader.nFirstLight [0];
	}
OglEnableLighting (0);
glDisable (GL_LIGHTING);
activeLightsP = gameData.render.lights.dynamic.shader.activeLights [0] + gameStates.ogl.nFirstLight;
nLightRange = gameData.render.lights.dynamic.shader.nLastLight [0] - gameStates.ogl.nFirstLight + 1;
for (nLights = 0; 
	  (gameStates.ogl.iLight < gameStates.ogl.nLights) & (nLightRange > 0) && (nLights < gameStates.render.nMaxLightsPerPass); 
	  activeLightsP++, nLightRange--) { 
	if (!(psl = GetActiveShaderLight (activeLightsP, 0)))
		continue;
	hLight = GL_LIGHT0 + nLights++;
	glEnable (hLight);
	specular.alpha = (psl->info.nSegment >= 0) ? psl->info.fRad : 0; //krasser Missbrauch!
	fBrightness = psl->info.fBrightness;
	if (psl->info.nType == 2) {
		glLightf (hLight, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf (hLight, GL_LINEAR_ATTENUATION, 0.1f / fBrightness);
		glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.01f / fBrightness);
		ambient.red = psl->info.color.red * 0.05f;
		ambient.green = psl->info.color.green * 0.05f;
		ambient.blue = psl->info.color.blue * 0.05f;
		ambient.alpha = 1.0f;
		diffuse.red = psl->info.color.red * 0.95f;
		diffuse.green = psl->info.color.green * 0.95f;
		diffuse.blue = psl->info.color.blue * 0.95f;
		diffuse.alpha = 1.0f;
		}
	else {
		glLightf (hLight, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf (hLight, GL_LINEAR_ATTENUATION, 0.1f / fBrightness);
		glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.01f / fBrightness);
		ambient.red = psl->info.color.red * 0.025f;
		ambient.green = psl->info.color.green * 0.025f;
		ambient.blue = psl->info.color.blue * 0.025f;
		ambient.alpha = 1.0f;
		fBrightness *= 0.475f;
		diffuse.red = psl->info.color.red * fBrightness;
		diffuse.green = psl->info.color.green * fBrightness;
		diffuse.blue = psl->info.color.blue * fBrightness;
		diffuse.alpha = 1.0f;
		}
	glLightfv (hLight, GL_DIFFUSE, (GLfloat *) &diffuse);
	glLightfv (hLight, GL_SPECULAR, (GLfloat *) &specular);
	glLightfv (hLight, GL_AMBIENT, (GLfloat *) &ambient);
	glLightfv (hLight, GL_POSITION, (GLfloat *) (psl->vPosf));
	gameStates.ogl.iLight++;
	}
if (!nLightRange)
	gameStates.ogl.iLight = gameStates.ogl.nLights;
gameStates.ogl.nFirstLight = activeLightsP - gameData.render.lights.dynamic.shader.activeLights [0];
#if 0//def _DEBUG
for (int i = nLights; i < MAX_LIGHTS_PER_PIXEL; i++) {
#if 1
	glDisable (GL_LIGHT0 + i);
#else
	hLight = GL_LIGHT0 + i;
	glEnable (hLight);
	glLightfv (hLight, GL_POSITION, (GLfloat *) &vPos);
	glLightfv (hLight, GL_DIFFUSE, (GLfloat *) &black);
	glLightfv (hLight, GL_SPECULAR, (GLfloat *) &black);
	glLightfv (hLight, GL_AMBIENT, (GLfloat *) &black);
#endif
	}
#endif
if (InitPerPixelLightingShader (bColorKey ? 3 : bMultiTexture ? 2 : bTextured, nLights))
	return nLights;
OglDisableLighting ();
return 0;
}

//------------------------------------------------------------------------------

int G3SetupShader (grsFace *faceP, int bColorKey, int bMultiTexture, int bTextured, int bColored, tRgbaColorf *colorP)
{
	static grsBitmap	*nullBmP = NULL;

	int			oglRes, bLightMaps, bStaticColor, nLights, nType, nShader = gameStates.render.history.nShader;
	tRgbaColorf	color;

if (!gameStates.ogl.bShadersOk)
	return -1;
#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if (faceP && gameOpts->ogl.bPerPixelLighting) {
	//per pixel lighting
	bStaticColor = (gameStates.ogl.iLight == 0);
	if ((nLights = G3SetupPerPixelLighting (faceP, bColorKey, bMultiTexture, bTextured)))
		;//nLights = MAX_LIGHTS_PER_PIXEL; //better to process a few "black" lights than switch shader programs all the time?
	nType = bColorKey ? 3 : bMultiTexture ? 2 : bTextured;
	nShader = 20 + nLights * MAX_LIGHTS_PER_PIXEL + nType;
#ifdef _DEBUG
	if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	if (bLightMaps = HaveLightMaps ()) {
		int i = (faceP - gameData.segs.faces.faces) / LIGHTMAP_BUFSIZE;
#ifdef _DEBUG
		if (OglCreateLightMap (i))
#endif
			{INIT_TMU (InitTMU0, GL_TEXTURE0, nullBmP, lightMapData.buffers + i, 1, 1);}
		}
	if (nShader != gameStates.render.history.nShader) {
		glUseProgramObject (0);
		glUseProgramObject (tmProg = perPixelLightingShaderProgs [nLights][nType]);
		if (bLightMaps)
			glUniform1i (glGetUniformLocation (tmProg, "lMapTex"), 0);
		if (bTextured) {
			glUniform1i (glGetUniformLocation (tmProg, "baseTex"), bLightMaps);
			if (bColorKey || bMultiTexture) {
				glUniform1i (glGetUniformLocation (tmProg, "decalTex"), 1 + bLightMaps);
				if (bColorKey)
					glUniform1i (glGetUniformLocation (tmProg, "maskTex"), 2 + bLightMaps);
				}
			}
		}
	if (nLights)
		glUniform1f (glGetUniformLocation (tmProg, "bStaticColor"), bStaticColor ? 1.0f : 0.0f);
	//glUniform1fv (glGetUniformLocation (tmProg, "lightRad"), nLights, (GLfloat *) gameData.render.ogl.lightRads);
	}
else if (gameData.render.lights.dynamic.headLights.nLights && !gameStates.render.automap.bDisplay) {
	//headlights
	nLights = IsMultiGame ? /*gameData.multiplayer.nPlayers*/gameData.render.lights.dynamic.headLights.nLights : 1;
	InitHeadlightShaders (nLights);
	nType = bColorKey ? 3 : bMultiTexture ? 2 : bTextured;
	nShader = 10 + nType;
	if (nShader != gameStates.render.history.nShader) {
		glEnable (GL_TEXTURE_2D);
		glActiveTexture (GL_TEXTURE0);
		glUseProgramObject (0);
		glUseProgramObject (tmProg = headlightShaderProgs [nType]);
		if (bTextured) {
			glUniform1i (glGetUniformLocation (tmProg, "baseTex"), 0);
			if (bColorKey || bMultiTexture) {
				glUniform1i (glGetUniformLocation (tmProg, "decalTex"), 1);
				if (bColorKey)
					glUniform1i (glGetUniformLocation (tmProg, "maskTex"), 2);
				}
			}
#if 1
#	if 0
		glUniform1f (glGetUniformLocation (tmProg, "cutOff"), 0.5f);
		glUniform1f (glGetUniformLocation (tmProg, "spotExp"), 8.0f);
		glUniform1f (glGetUniformLocation (tmProg, "grAlpha"), 1.0f);
		glUniform1fv (glGetUniformLocation (tmProg, "brightness"), nLights, 
						  (GLfloat *) gameData.render.lights.dynamic.headLights.brightness);
#	endif
		//glUniform1f (glGetUniformLocation (tmProg, "aspect"), (float) grdCurScreen->scWidth / (float) grdCurScreen->scHeight);
		//glUniform1f (glGetUniformLocation (tmProg, "zoom"), 65536.0f / (float) gameStates.render.xZoom);
#if 1
		for (int i = 0; i < nLights; i++) {
			glEnable (GL_LIGHT0 + i);
			glLightfv (GL_LIGHT0 + i, GL_POSITION, (GLfloat *) (gameData.render.lights.dynamic.headLights.pos + i));
			glLightfv (GL_LIGHT0 + i, GL_SPOT_DIRECTION, (GLfloat *) (gameData.render.lights.dynamic.headLights.dir + i));
			}
#else
		glUniform3fv (glGetUniformLocation (tmProg, "lightPosWorld"), nLights, 
						  (GLfloat *) gameData.render.lights.dynamic.headLights.pos);
		glUniform3fv (glGetUniformLocation (tmProg, "lightDirWorld"), nLights, 
						  (GLfloat *) gameData.render.lights.dynamic.headLights.dir);
#endif
#endif
		if (colorP) {
			color.red = colorP->red * 1.1f;
			color.green = colorP->green * 1.1f;
			color.blue = colorP->blue * 1.1f;
			color.alpha = colorP->alpha;
			}
		else {
			color.red = color.green = color.blue = 2.0f;
			color.alpha = 1;
			}
		glUniform4fv (glGetUniformLocation (tmProg, "maxColor"), 1, (GLfloat *) &color);
		oglRes = glGetError ();
		}
	}
else if (bColorKey || bMultiTexture) {
	//texture mapping
	nShader = bColorKey ? 2 : 0;
	if (nShader != gameStates.render.history.nShader)
		glUseProgramObject (tmProg = tmShaderProgs [nShader + bColored * 3]);
	glUniform1i (glGetUniformLocation (tmProg, "baseTex"), 0);
	glUniform1i (glGetUniformLocation (tmProg, "decalTex"), 1);
	glUniform1i (glGetUniformLocation (tmProg, "maskTex"), 2);
	glUniform1f (glGetUniformLocation (tmProg, "grAlpha"), 1.0f);
	}
else if (!bColored && gameOpts->render.automap.bGrayOut) {
	//automap gray out
	nShader = 99;
	if (nShader != gameStates.render.history.nShader)
		glUseProgramObject (tmProg = gsShaderProg);
	glUniform1i (glGetUniformLocation (tmProg, "baseTex"), 0);
	}
else if (gameStates.render.history.nShader >= 0) {
	//no shaders
	glUseProgramObject (0);
	nShader = -1;
	}
gameStates.render.history.nShader = nShader;
return nShader;
}

//------------------------------------------------------------------------------

int G3DrawFaceSimple (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly)
{
	int			h, i, j, nTextures, nRemainingLights, nLights [4], nPass = 0;
	int			bOverlay, bTransparent, 
					bColorKey = 0, bMonitor = 0, 
					bLighting = HW_GEO_LIGHTING && !bDepthOnly, 
					bMultiTexture = 0;
	grsBitmap	*bmMask = NULL, *bmP [2];
	tTexCoord2f	*texCoordP, *ovlTexCoordP;

#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if (!faceP->bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = BmOverride (bmBot, -1);
bTransparent = faceP->bTransparent || (bmBot && ((bmBot->bmProps.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SEE_THRU)) == BM_FLAG_TRANSPARENT));

if (bDepthOnly) {
	if (bTransparent || faceP->bOverlay)
		return 0;
	bOverlay = 0;
	}
else {
	bMonitor = gameStates.render.bUseCameras && (faceP->nCamera >= 0);
#ifdef _DEBUG
	if (bMonitor)
		faceP = faceP;
#endif
	if (bTransparent && !(bMonitor || bmTop || faceP->bSplit || faceP->bOverlay)) {
#ifdef _DEBUG
		if (gameOpts->render.bDepthSort > 0) 
#endif
			{
#if 0//def _DEBUG
			if ((faceP->nSegment != nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide != nDbgSide)))
				return 1;
#endif
			RIAddFace (faceP);
			return 0;
			}
		}
	}
if (bTextured) {
	if (bmTop && !bMonitor) {
		if ((bmTop = BmOverride (bmTop, -1)) && BM_FRAMES (bmTop)) {
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
			bmTop = BM_CURFRAME (bmTop);
			}
		else
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
		bOverlay = (bColorKey && gameStates.ogl.bGlTexMerge) ? 1 : -1;
		bMultiTexture = HW_GEO_LIGHTING && (bOverlay > 0);
		}
	else
		bOverlay = 0;
	if ((bmBot != gameStates.render.history.bmBot) || 
		 (bmTop != gameStates.render.history.bmTop) || 
		 (bOverlay != gameStates.render.history.bOverlay)) {
		if (bOverlay > 0) {
			bmMask = gameStates.render.textures.bHaveMaskShader ? BM_MASK (bmTop) : NULL;
			// set base texture
			if (bmBot != gameStates.render.history.bmBot) {
				{INIT_TMU (InitTMU0, GL_TEXTURE0, bmBot, lightMapData.buffers, 0, 0);}
				gameStates.render.history.bmBot = bmBot;
				}
			// set overlay texture
			if (bmTop != gameStates.render.history.bmTop) {
				{INIT_TMU (InitTMU1, GL_TEXTURE1, bmTop, lightMapData.buffers, 0, 0);}
				gameStates.render.history.bmTop = bmTop;
				}
			if (bmMask) {
				{INIT_TMU (InitTMU2, GL_TEXTURE2, bmMask, lightMapData.buffers, 0, 0);}
				glUniform1i (glGetUniformLocation (tmProg, "maskTex"), 2);
				}
			gameStates.render.history.bmMask = bmMask;
			bmTop = NULL;
			G3SetupShader (faceP, bColorKey, 1, 1, !gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [faceP->nSegment], NULL);
			}
		else {
			if (gameStates.render.history.bOverlay > 0) {
				if (gameStates.ogl.bShadersOk)
					glUseProgramObject (0);
				if (gameStates.render.history.bmMask) {
					glActiveTexture (GL_TEXTURE2);
					OGL_BINDTEX (0);
					gameStates.render.history.bmMask = NULL;
					}
				}
			gameStates.render.history.bmTop = NULL;
			//if (bmBot != gameStates.render.history.bmBot) 
				{
				{INIT_TMU (InitTMU0, GL_TEXTURE0, bmBot, lightMapData.buffers, 0, 0);}
				}
			}
		G3SetupShader (faceP, 0, bMultiTexture, bmBot != NULL, 
							!gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [faceP->nSegment],
							bmBot ? NULL : &faceP->color);
		}
	gameStates.render.history.bOverlay = bOverlay;
	}
else {
	bOverlay = 0;
	glDisable (GL_TEXTURE_2D);
	}
if (!bBlend)
	glDisable (GL_BLEND);

#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nDbgSeg = nDbgSeg;
	else
		nDbgSeg = nDbgSeg;
#endif
bmP [0] = bmBot;
bmP [1] = bmTop;
nTextures = bmP [1] ? 2 : 1;
for (h = 0; h < nTextures; ) {
	gameStates.render.history.bmBot = bmP [h];
	nPass = 0;
	nLights [0] = nLights [1] = nLights [2] = nLights [3] = -1;
	nRemainingLights = bLighting;
	texCoordP = bMonitor ? faceP->pTexCoord : h ? gameData.segs.faces.ovlTexCoord : gameData.segs.faces.texCoord;
	if (bMultiTexture)
		ovlTexCoordP = bMonitor ? faceP->pTexCoord : gameData.segs.faces.ovlTexCoord;
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	do {
		j = faceP->nIndex;
		if (nPass)
			nRemainingLights = 0;
		glBegin (GL_TRIANGLE_FAN);
		for (i = 0; i < 4; i++, j++) {
	#ifdef _DEBUG
			if (faceP->index [i] == nDbgVertex)
				faceP = faceP;
	#endif
			if (bLighting && nPass) {
				if (!(nLights [i] = OglVertexLight (nLights [i], nPass, i)))
					glColor4f (0,0,0,0);
				else {
					nRemainingLights += nLights [i];
					if (bTextured)
						glColor4f (1,1,1,1);
					else
						glColor4fv ((GLfloat *) (gameData.segs.faces.color + faceP->nIndex));
					}
				}
			else
				glColor4fv ((GLfloat *) (gameData.segs.faces.color + j));
			if (bMultiTexture) {
				glMultiTexCoord2fv (GL_TEXTURE0, (GLfloat *) (texCoordP + j));
				glMultiTexCoord2fv (GL_TEXTURE1, (GLfloat *) (ovlTexCoordP + j));
				if (bmMask)
					glMultiTexCoord2fv (GL_TEXTURE2, (GLfloat *) (ovlTexCoordP + j));
				}
			else if (bmP [h])
				glTexCoord2fv ((GLfloat *) (texCoordP + j));
			glVertex3fv ((GLfloat *) (gameData.segs.faces.vertices + j));
			}
		glEnd ();
		if (bLighting && !(nPass || h)) {
			glBlendFunc (GL_ONE, GL_ONE);
			G3SetupFaceLight (faceP, bTextured);
			}
		nPass++;
		} while (nRemainingLights > 0);
	if (bLighting)
		OglDisableLighting ();
	if (++h >= nTextures)
		break;
	{INIT_TMU (InitTMU0, GL_TEXTURE0, bmP [h], lightMapData.buffers, 0, 0);}
	}
#if 0
if (bLighting) {
	for (i = 0; i < 4; i++)
		gameData.render.lights.dynamic.shader.nActiveLights [i] = gameData.render.lights.dynamic.shader.iVertexLights [i];
	}
#endif
if (!bBlend)
	glEnable (GL_BLEND);
return 0;
}

//------------------------------------------------------------------------------

int G3DrawFaceArrays (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly)
{
	int			bOverlay, bColored, bTransparent, bColorKey = 0, bMonitor = 0, bMultiTexture = 0;
	grsBitmap	*bmMask = NULL;
	tTexCoord2f	*ovlTexCoordP;

#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nDbgSeg = nDbgSeg;
	else
		nDbgSeg = nDbgSeg;
#if 0
else
	return 1;
#endif
if (bmBot && strstr (bmBot->szName, "door45#4"))
	bmBot = bmBot;
if (bmTop && strstr (bmTop->szName, "door35#4"))
	bmTop = bmTop;
#endif

if (!faceP->bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = BmOverride (bmBot, -1);
#if 0
if (bmBot && strstr (bmBot->szName, "door"))
	bTransparent = 0;
else
#endif
	bTransparent = faceP->bTransparent || (bmBot && (((bmBot->bmProps.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SEE_THRU)) == (BM_FLAG_TRANSPARENT))));

if (bDepthOnly) {
	if (bTransparent || faceP->bOverlay)
		return 0;
	bOverlay = 0;
	}
else {
	bMonitor = (faceP->nCamera >= 0);
#ifdef _DEBUG
	if (bMonitor)
		faceP = faceP;
#endif
	if (bTransparent && (gameStates.render.nType < 4) && !(bMonitor || bmTop || faceP->bSplit || faceP->bOverlay)) {
#ifdef _DEBUG
		if (gameOpts->render.bDepthSort > 0) 
#endif
			{
#if 0//def _DEBUG
			if ((faceP->nSegment != nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide != nDbgSide)))
				return 1;
#endif
			RIAddFace (faceP);
			return 0;
			}
		}
	}

#if G3_BUFFER_FACES
	G3FlushFaceBuffer (bMonitor || (bTextured != faceBuffer.bTextured) || faceP->bmTop || (faceP->bmBot != faceBuffer.bmP) || (faceP->nType != SIDE_IS_QUAD));
#endif
if (bTextured) {
	bColored = !gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [faceP->nSegment];
	if (bmTop && !bMonitor) {
		if ((bmTop = BmOverride (bmTop, -1)) && BM_FRAMES (bmTop)) {
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
			bmTop = BM_CURFRAME (bmTop);
			}
		else
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
		bOverlay = (bColorKey && gameStates.ogl.bGlTexMerge) ? 1 : -1;
		bMultiTexture = gameOpts->ogl.bPerPixelLighting && ((bOverlay > 0) || ((bOverlay < 0) && !bMonitor));
		}
	else
		bOverlay = 0;
	if ((bmBot != gameStates.render.history.bmBot) || 
		 (bmTop != gameStates.render.history.bmTop) || 
		 (bOverlay != gameStates.render.history.bOverlay) || 
		 (bColored != gameStates.render.history.bColored)) {
		if (bOverlay > 0) {
#ifdef _DEBUG
		if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
			if (bDepthOnly)
				nDbgSeg = nDbgSeg;
			else
				nDbgSeg = nDbgSeg;
		if (bmBot && strstr (bmBot->szName, "door"))
			bmBot = bmBot;
#endif
			// set base texture
			if (bmBot != gameStates.render.history.bmBot) {
				{INIT_TMU (InitTMU0, GL_TEXTURE0, bmBot, lightMapData.buffers, 1, 0);}
				gameStates.render.history.bmBot = bmBot;
				}
			// set overlay texture
			if (bmTop != gameStates.render.history.bmTop) {
				{INIT_TMU (InitTMU1, GL_TEXTURE1, bmTop, lightMapData.buffers, 1, 0);}
				gameStates.render.history.bmTop = bmTop;
				if (gameStates.render.history.bOverlay != 1) {	//enable multitexturing
					if (!G3EnableClientState (GL_TEXTURE_COORD_ARRAY, GL_TEXTURE1))
						return 1;
					}
				}
			if (!(bmMask = gameStates.render.textures.bHaveMaskShader ? BM_MASK (bmTop) : NULL))
				bColorKey = 0;
			else {
				{INIT_TMU (InitTMU2, GL_TEXTURE2, bmMask, lightMapData.buffers, 2, 0);}
				if (!G3EnableClientState (GL_TEXTURE_COORD_ARRAY, GL_TEXTURE2))
					return 1;
				}
			gameStates.render.history.bmMask = bmMask;
			G3SetupShader (faceP, bColorKey, 1, 1, bColored, NULL);
			}
		else {
			if (gameStates.render.history.bOverlay > 0) {
				if (gameStates.ogl.bShadersOk)
					glUseProgramObject (0);
				if (gameStates.render.history.bmMask) {
					glActiveTexture (GL_TEXTURE2);
					glClientActiveTexture (GL_TEXTURE2);
					glDisableClientState (GL_TEXTURE_COORD_ARRAY);
					OGL_BINDTEX (0);
					gameStates.render.history.bmMask = NULL;
					}
				}
			if (bMultiTexture) {
				if (bmTop != gameStates.render.history.bmTop) {
					glActiveTexture (GL_TEXTURE1);
					if (bmTop) {
						{INIT_TMU (InitTMU1, GL_TEXTURE1, bmTop, lightMapData.buffers, 1, 0);}
						}
					else {
						glClientActiveTexture (GL_TEXTURE1);
						OGL_BINDTEX (0);
						}
					gameStates.render.history.bmTop = bmTop;
					}
				}
			else {
				if (gameStates.render.history.bmTop) {
					glActiveTexture (GL_TEXTURE1);
					glClientActiveTexture (GL_TEXTURE1);
					OGL_BINDTEX (0);
					gameStates.render.history.bmTop = NULL;
					}
				}
			if (bmBot != gameStates.render.history.bmBot) {
				{INIT_TMU (InitTMU0, GL_TEXTURE0, bmBot, lightMapData.buffers, 1, 0);}
				gameStates.render.history.bmBot = bmBot;
				}
			G3SetupShader (faceP, 0, bMultiTexture, bmBot != NULL, bColored, bmBot ? NULL : &faceP->color);
			}
		}
	else
		G3SetupShader (faceP, 0, bMultiTexture, bmBot != NULL, bColored, bmBot ? NULL : &faceP->color);
	gameStates.render.history.bOverlay = bOverlay;
	gameStates.render.history.bColored = bColored;
	}
else {
	bOverlay = 0;
	glActiveTexture (GL_TEXTURE0);
	glClientActiveTexture (GL_TEXTURE0);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	}
#if G3_BUFFER_FACES
if (!(bMonitor || bOverlay)) {
	G3FillFaceBuffer (faceP, bTextured);
	return 0;
	}
#endif
#if 1
if (bBlend) {
	glEnable (GL_BLEND);
	if (FaceIsAdditive (faceP))
		glBlendFunc (GL_ONE, GL_ONE);
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
else
	glDisable (GL_BLEND);
#endif
if (gameStates.render.bTriangleMesh && !bMonitor) {
#ifdef _DEBUG
	if ((nDbgFace >= 0) && (faceP - gameData.segs.faces.faces != nDbgFace))
		return 0;
	if (!bDepthOnly && gameOpts->render.debug.bWireFrame) {
		if ((nDbgFace < 0) || (faceP - gameData.segs.faces.faces == nDbgFace)) {
			grsTriangle	*triP = gameData.segs.faces.tris + faceP->nTriIndex;
			glDisableClientState (GL_COLOR_ARRAY);
			if (bTextured)
				glDisable (GL_TEXTURE_2D);
#	if 1
			glColor3f (1.0f, 0.5f, 0.0f);
			glLineWidth (6);
			glBegin (GL_LINE_LOOP);
			for (int i = 0; i < 4; i++)
				glVertex3fv ((GLfloat *) (gameData.segs.fVertices + faceP->index [i]));
			glEnd ();
#	endif
			glLineWidth (2);
			glColor3f (1,1,1);
			for (int i = 0; i < faceP->nTris; i++, triP++)
				glDrawArrays (GL_LINE_LOOP, triP->nIndex, 3);
			glLineWidth (1);
			if (gameOpts->render.debug.bDynamicLight)
				glEnableClientState (GL_COLOR_ARRAY);
			if (bTextured)
				glEnable (GL_TEXTURE_2D);
			}
		}
	if (gameOpts->render.debug.bTextures && ((nDbgFace < 0) || (faceP - gameData.segs.faces.faces == nDbgFace)))
#endif
#if USE_RANGE_ELEMENTS
		{
		GLsizei nElements = faceP->nTris * 3;
		glDrawRangeElements (GL_TRIANGLES, faceP->vertIndex [0], faceP->vertIndex [nElements - 1], nElements, GL_UNSIGNED_INT, faceP->vertIndex);
		}	
#else
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, faceP->nTris * 3);
#endif
	}	
else if (gameOpts->ogl.bPerPixelLighting) {
	for (;;) {
		glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
		if (gameStates.ogl.iLight >= gameStates.ogl.nLights)
			break;
		G3SetupPerPixelLighting (faceP, bColorKey, bMultiTexture, bmBot != NULL);
		glUniform1f (glGetUniformLocation (tmProg, "bStaticColor"), 0.0f);
		glBlendFunc (GL_ONE, GL_ONE);
		glDepthFunc (GL_EQUAL);
		}
	glDepthFunc (GL_LEQUAL);
	}
else {
#ifdef _DEBUG
	if (!bDepthOnly && gameOpts->render.debug.bWireFrame) {
		if ((nDbgFace < 0) || (faceP - gameData.segs.faces.faces == nDbgFace)) {
			glColor3f (1.0f, 0.5f, 0.0f);
			glLineWidth (6);
			glBegin (GL_LINE_LOOP);
			for (int i = 0; i < 4; i++)
				glVertex3fv ((GLfloat *) (gameData.segs.fVertices + faceP->index [i]));
			glEnd ();
			glLineWidth (1);
			if (gameOpts->render.debug.bDynamicLight)
				glEnableClientState (GL_COLOR_ARRAY);
			if (bTextured)
				glEnable (GL_TEXTURE_2D);
			}
		}
	if (gameOpts->render.debug.bTextures) {
#endif
#if 1
	// this is a work around for OpenGL's per vertex light interpolation
	// rendering a quad is always started with the brightest vertex
	tRgbaColorf	*pc = gameData.segs.faces.color + faceP->nIndex;
	float l, lMax = 0;
	int i, j, iMax = 0, nIndex, index [4];

	for (i = 0; i < 4; i++, pc++) {
		l = pc->red + pc->green + pc->blue;
		if (lMax < l) {
			lMax = l;
			iMax = i;
			}
		}
	if (!iMax)
		glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
	else {
		nIndex = faceP->nIndex;
		for (i = 0, j = iMax; i < 4; i++, j %= 4)
			index [i] = nIndex + j++;
		glDrawElements (GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, index);
		}
#else
	glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
#endif
#ifdef _DEBUG
	}
#endif
	}

#ifdef _DEBUG
if (!gameOpts->render.debug.bTextures)
	return 0;
#endif
if (!bMultiTexture && (bOverlay || bMonitor)) {
#ifdef _DEBUG
	if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		if (bDepthOnly)
			nDbgSeg = nDbgSeg;
		else
			nDbgSeg = nDbgSeg;
#endif
	ovlTexCoordP = bMonitor ? faceP->pTexCoord - faceP->nIndex : gameData.segs.faces.ovlTexCoord;
	if (bTextured) {
		{INIT_TMU (InitTMU0, GL_TEXTURE0, bmTop, lightMapData.buffers, 1, 0);}
		if (gameData.render.lights.dynamic.headLights.nLights)
			glUniform1i (glGetUniformLocation (tmProg, "baseTex"), 0);
		glActiveTexture (GL_TEXTURE0);
		glClientActiveTexture (GL_TEXTURE0);
		glEnableClientState (GL_TEXTURE_COORD_ARRAY);
		}
	else {
		glActiveTexture (GL_TEXTURE0);
		glClientActiveTexture (GL_TEXTURE0);
		glDisable (GL_TEXTURE_2D);
		OGL_BINDTEX (0);
		}
	glTexCoordPointer (2, GL_FLOAT, 0, ovlTexCoordP);
	if (gameStates.render.bTriangleMesh)
		glDrawArrays (GL_TRIANGLES, faceP->nIndex, faceP->nTris * 3);
	else
		glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
	glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.texCoord);
	gameStates.render.history.bmBot = bmTop;
	}
#if 0
if (!bBlend)
	glEnable (GL_BLEND);
#endif
return 0;
}

//------------------------------------------------------------------------------

int G3DrawFaceArraysPPLM (grsFace *faceP, grsBitmap *bmBot, grsBitmap *bmTop, int bBlend, int bTextured, int bDepthOnly)
{
	int			bOverlay, bColored, bTransparent, bColorKey = 0, bMonitor = 0, bMultiTexture = 0;
	grsBitmap	*bmMask = NULL;
	tTexCoord2f	*ovlTexCoordP;

#ifdef _DEBUG
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nDbgSeg = nDbgSeg;
	else
		nDbgSeg = nDbgSeg;
#if 0
else
	return 1;
#endif
if (bmBot && strstr (bmBot->szName, "door45#4"))
	bmBot = bmBot;
if (bmTop && strstr (bmTop->szName, "door35#4"))
	bmTop = bmTop;
#endif

if (!faceP->bTextured)
	bmBot = NULL;
else if (bmBot)
	bmBot = BmOverride (bmBot, -1);
#if 0
if (bmBot && strstr (bmBot->szName, "door"))
	bTransparent = 0;
else
#endif
	bTransparent = faceP->bTransparent || (bmBot && (((bmBot->bmProps.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SEE_THRU)) == (BM_FLAG_TRANSPARENT))));

if (bDepthOnly) {
	if (bTransparent || faceP->bOverlay)
		return 0;
	bOverlay = 0;
	}
else {
	bMonitor = (faceP->nCamera >= 0);
#ifdef _DEBUG
	if (bMonitor)
		faceP = faceP;
#endif
	if (bTransparent && (gameStates.render.nType < 4) && !(bMonitor || bmTop || faceP->bSplit || faceP->bOverlay)) {
#ifdef _DEBUG
		if (gameOpts->render.bDepthSort > 0) 
#endif
			{
#if 0//def _DEBUG
			if ((faceP->nSegment != nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide != nDbgSide)))
				return 1;
#endif
			RIAddFace (faceP);
			return 0;
			}
		}
	}

#if G3_BUFFER_FACES
	G3FlushFaceBuffer (bMonitor || (bTextured != faceBuffer.bTextured) || faceP->bmTop || (faceP->bmBot != faceBuffer.bmP) || (faceP->nType != SIDE_IS_QUAD));
#endif
if (bTextured) {
	bColored = !gameStates.render.automap.bDisplay || gameData.render.mine.bAutomapVisited [faceP->nSegment];
	if (bmTop && !bMonitor) {
		if ((bmTop = BmOverride (bmTop, -1)) && BM_FRAMES (bmTop)) {
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
			bmTop = BM_CURFRAME (bmTop);
			}
		else
			bColorKey = (bmTop->bmProps.flags & BM_FLAG_SUPER_TRANSPARENT) != 0;
		bOverlay = bColorKey ? 1 : -1;
		bMultiTexture = (bOverlay != 0) && !bMonitor;
		}
	else
		bOverlay = 0;
	if ((bmBot != gameStates.render.history.bmBot) || 
		 (bmTop != gameStates.render.history.bmTop) || 
		 (bOverlay != gameStates.render.history.bOverlay) || 
		 (bColored != gameStates.render.history.bColored)) {
		if (bOverlay > 0) {
#ifdef _DEBUG
			if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
				if (bDepthOnly)
					nDbgSeg = nDbgSeg;
				else
					nDbgSeg = nDbgSeg;
			if (bmBot && strstr (bmBot->szName, "door"))
				bmBot = bmBot;
#endif
			// set base texture
			if (bmBot != gameStates.render.history.bmBot) {
				{INIT_TMU (InitTMU1, GL_TEXTURE1, bmBot, lightMapData.buffers, 1, 1);}
				gameStates.render.history.bmBot = bmBot;
				}
			// set overlay texture
			if (bmTop != gameStates.render.history.bmTop) {
				{INIT_TMU (InitTMU2, GL_TEXTURE2, bmTop, lightMapData.buffers, 1, 1);}
				gameStates.render.history.bmTop = bmTop;
				if (gameStates.render.history.bOverlay != 1) {	//enable multitexturing
					if (!G3EnableClientState (GL_TEXTURE_COORD_ARRAY, GL_TEXTURE2))
						return 1;
					}
				}
			if (!(bmMask = gameStates.render.textures.bHaveMaskShader ? BM_MASK (bmTop) : NULL))
				bColorKey = 0;
			else {
				{INIT_TMU (InitTMU3, GL_TEXTURE3, bmMask, lightMapData.buffers, 2, 1);}
				if (!G3EnableClientState (GL_TEXTURE_COORD_ARRAY, GL_TEXTURE3))
					return 1;
				}
			gameStates.render.history.bmMask = bmMask;
			}
		else {
			if (gameStates.render.history.bOverlay > 0) {
				if (gameStates.ogl.bShadersOk)
					glUseProgramObject (0);
				if (gameStates.render.history.bmMask) {
					glActiveTexture (GL_TEXTURE3);
					glClientActiveTexture (GL_TEXTURE3);
					glDisableClientState (GL_TEXTURE_COORD_ARRAY);
					OGL_BINDTEX (0);
					gameStates.render.history.bmMask = NULL;
					}
				}
			if (bMultiTexture) {
				if (bmTop != gameStates.render.history.bmTop) {
					if (bmTop) {
						{INIT_TMU (InitTMU2, GL_TEXTURE2, bmTop, lightMapData.buffers, 1, 1);}
						}
					else {
						glClientActiveTexture (GL_TEXTURE2);
						glActiveTexture (GL_TEXTURE2);
						OGL_BINDTEX (0);
						}
					gameStates.render.history.bmTop = bmTop;
					}
				}
			else {
				if (gameStates.render.history.bmTop) {
					glActiveTexture (GL_TEXTURE2);
					glClientActiveTexture (GL_TEXTURE2);
					OGL_BINDTEX (0);
					gameStates.render.history.bmTop = NULL;
					}
				}
			if (bmBot != gameStates.render.history.bmBot) {
				{INIT_TMU (InitTMU1, GL_TEXTURE1, bmBot, lightMapData.buffers, 1, 1);}
				gameStates.render.history.bmBot = bmBot;
				}
			}
		}
	gameStates.render.history.bOverlay = bOverlay;
	gameStates.render.history.bColored = bColored;
	}
else {
	bOverlay = 0;
	glActiveTexture (GL_TEXTURE1);
	glClientActiveTexture (GL_TEXTURE1);
	OGL_BINDTEX (0);
	glDisable (GL_TEXTURE_2D);
	}
#if G3_BUFFER_FACES
if (!(bMonitor || bOverlay)) {
	G3FillFaceBuffer (faceP, bTextured);
	return 0;
	}
#endif
#if 1
if (bBlend) {
	glEnable (GL_BLEND);
	if (FaceIsAdditive (faceP))
		glBlendFunc (GL_ONE, GL_ONE);
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
else
	glDisable (GL_BLEND);
#endif

#ifdef _DEBUG
if (!gameOpts->render.debug.bTextures)
	return 0;
if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	if (bDepthOnly)
		nDbgSeg = nDbgSeg;
	else
		nDbgSeg = nDbgSeg;
#endif
if (bDepthOnly) {
	glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
	return 1;
	}
gameStates.ogl.iLight = 0;
for (;;) {
	G3SetupShader (faceP, bColorKey, bMultiTexture, bmBot != NULL, bColored, NULL);
	glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
	if ((gameStates.ogl.iLight >= gameStates.ogl.nLights) || (gameStates.ogl.iLight >= MAX_PP_LIGHTS_PER_FACE))
		break;
	glBlendFunc (GL_ONE, GL_ONE);
	glDepthFunc (GL_EQUAL);
	}
glDepthFunc (GL_LEQUAL);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

if (bMonitor) {
#ifdef _DEBUG
	if (faceP && (faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		if (bDepthOnly)
			nDbgSeg = nDbgSeg;
		else
			nDbgSeg = nDbgSeg;
#endif
	ovlTexCoordP = bMonitor ? faceP->pTexCoord - faceP->nIndex : gameData.segs.faces.ovlTexCoord;
	if (bTextured) {
		{INIT_TMU (InitTMU1, GL_TEXTURE1, bmTop, lightMapData.buffers, 1, 0);}
		if (gameData.render.lights.dynamic.headLights.nLights)
			glUniform1i (glGetUniformLocation (tmProg, "baseTex"), 0);
		glActiveTexture (GL_TEXTURE1);
		glClientActiveTexture (GL_TEXTURE1);
		glEnableClientState (GL_TEXTURE_COORD_ARRAY);
		}
	else {
		glActiveTexture (GL_TEXTURE1);
		glClientActiveTexture (GL_TEXTURE1);
		glDisable (GL_TEXTURE_2D);
		OGL_BINDTEX (0);
		}
	glTexCoordPointer (2, GL_FLOAT, 0, ovlTexCoordP);
	glDrawArrays (GL_TRIANGLE_FAN, faceP->nIndex, 4);
	glTexCoordPointer (2, GL_FLOAT, 0, gameData.segs.faces.texCoord);
	gameStates.render.history.bmBot = bmTop;
	}
#if 0
if (!bBlend)
	glEnable (GL_BLEND);
#endif
return 0;
}

//------------------------------------------------------------------------------
//eof
