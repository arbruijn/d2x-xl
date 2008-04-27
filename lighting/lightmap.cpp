/*
Here is all the code for gameOpts->render.color.bUseLightMaps.  Some parts need 
to be optamized but the core functionality is there.
The thing you will need to add is the menu code to change
the value of int gameOpts->render.color.bUseLightMaps.  THIS CANNOT BE CHANGED IF
A GAME IS RUNNING.  It would likely cause it to crash.

Almost forgot there are a few lines that read exit (0) after 
checking certain capabilties.  This should really just 
disable the gameOpts->render.color.bUseLightMaps option, because the person's computer 
doesn't support shaders.  But since the menu option isn't 
there I just had it exit instead.
*/

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#	include <io.h>
#elif !defined (__unix__)
#	include <conf.h>
#endif
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>

#include "inferno.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "light.h"
#include "lightmap.h"
#include "gameseg.h"
#include "wall.h"
#include "bm.h"
#include "fbuffer.h"
#include "error.h"
#include "u_mem.h"
#include "menu.h"
#include "newmenu.h"
#include "text.h"
#include "gamepal.h"
#include "gamemine.h"

//------------------------------------------------------------------------------

#define LMAP_REND2TEX	0

GLhandleARB lmShaderProgs [3] = {0,0,0}; 
GLhandleARB lmFS [3] = {0,0,0}; 
GLhandleARB lmVS [3] = {0,0,0}; 

int nLights; 
tLightMap *lightMaps = NULL;  //Level Lightmaps
tLightMapInfo *lightMapInfo = NULL;  //Level lights
tLightMap dummyLightMap;

#define TEXTURE_CHECK 1

int InitLightData (int bVariable);

//------------------------------------------------------------------------------

inline void FindOffset (vmsVector *outvec, vmsVector vec1, vmsVector vec2, double f_offset)
{
outvec->p.x = (fix) (f_offset * (vec2.p.x - vec1.p.x)); 
outvec->p.y = (fix) (f_offset * (vec2.p.y - vec1.p.y)); 
outvec->p.z = (fix) (f_offset * (vec2.p.z - vec1.p.z)); 
}

//------------------------------------------------------------------------------

void RestoreLights (int bVariable)
{
	tDynLight	*pl;
	int			i;

for (pl = gameData.render.lights.dynamic.lights, i = gameData.render.lights.dynamic.nLights; i; i--, pl++)
	if (!(pl->nType || (pl->bVariable && !bVariable)))
		pl->bOn = 1;
}

//------------------------------------------------------------------------------

int CountLights (int bVariable)
{
	tDynLight	*pl;
	int			i, nLights = 0;

if (!(gameOpts->ogl.bPerPixelLighting && gameStates.render.color.bLightMapsOk))
	return 0;
for (pl = gameData.render.lights.dynamic.lights, i = gameData.render.lights.dynamic.nLights; i; i--, pl++)
	if (!(pl->nType || (pl->bVariable && !bVariable)))
		nLights++;
return nLights; 
}

//------------------------------------------------------------------------------

double SideRad (int nSegment, int nSide)
{
	int			i;
	double		h, xMin, xMax, yMin, yMax, zMin, zMax;
	double		dx, dy, dz;
	short			sideVerts [4];
	vmsVector	*v;

GetSideVertIndex (sideVerts, nSegment, nSide); 
xMin = yMin = zMin = 1e300;
xMax = yMax = zMax = -1e300;
for (i = 0; i < 4; i++) {
	v = gameData.segs.vertices + sideVerts [i];
	h = v->p.x;
	if (xMin > h)
		xMin = h;
	if (xMax < h)
		xMax = h;
	h = v->p.y;
	if (yMin > h)
		yMin = h;
	if (yMax < h)
		yMax = h;
	h = v->p.z;
	if (zMin > h)
		zMin = h;
	if (zMax < h)
		zMax = h;
	}
dx = xMax - xMin;
dy = yMax - yMin;
dz = zMax - zMin;
return sqrt (dx * dx + dy * dy + dz * dz) / (2 * (double) F1_0);
}

//------------------------------------------------------------------------------

int OglCreateLightMaps (void)
{
	tLightMap	*lmP = lightMaps;

for (int i = HaveLightMaps () ? gameData.segs.nFaces : 1; i; i--, lmP++) {
	OglGenTextures (1, &lmP->handle);
	if (!lmP->handle)
		return 0;
	OGL_BINDTEX (lmP->handle); 
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D (GL_TEXTURE_2D, 0, 3, LIGHTMAP_WIDTH, LIGHTMAP_WIDTH, 0, gameStates.ogl.nRGBFormat, GL_FLOAT, lmP->bmP);
	}
return 1;
}

//------------------------------------------------------------------------------

void OglDestroyLightMaps (void)
{
if (lightMaps) { 
	tLightMap *lmP = lightMaps;
	for (int i = gameData.segs.nFaces + 1; i; i--, lmP++)
		if (lmP->handle) {
			OglDeleteTextures (1, (GLuint *) &lmP->handle);
			lmP->handle = 0;
			}
	} 
}

//------------------------------------------------------------------------------

void DestroyLightMaps (void)
{
if (lightMapInfo) { 
	OglDestroyLightMaps ();
	D2_FREE (lightMapInfo);
	if (lightMaps == &dummyLightMap)
		lightMaps = NULL;
	else
		D2_FREE (lightMaps);
	}
}

//------------------------------------------------------------------------------

int IsBigLight (int t)
{
if (gameStates.app.bD1Mission)
	t = ConvertD1Texture (t, 1);
switch (t) {
	case 302:
	case 378:
	case 405:
	case 406:
	case 407:
	case 408:
	case 409:
	case 420:
	case 426:
	case 432:
	case 433:
	case 434:
		return 1;
	default:
		break;
	}
return 0;
}

//------------------------------------------------------------------------------

#define		BASERANGE	32.5	//75.0

//------------------------------------------------------------------------------

inline double LightMapRange (void)
{
return BASERANGE + (double) gameOpts->render.color.nLightMapRange * 6.5;
}

//------------------------------------------------------------------------------

double GetLightColor (int t, GLfloat *colorP)
{
	GLfloat		color [3];
	int			bIsLight = 0;
	double		range, baseRange = LightMapRange ();

if (gameStates.app.bD1Mission) {
#ifdef _DEBUG
	if (t == 328)
		t = t;
	else if (t == 281)
		t = t;
#endif
	t = ConvertD1Texture (t, 1);
	}
switch (t) {
	case 275:
	case 276:
	case 278:
	case 288:
		bIsLight = 1; 
		color [0] = 
		color [1] = (GLfloat) 1.0; 
		color [2] = (GLfloat) 0.75; 
		range = baseRange; 
		break; 
	case 289:
	case 290:
	case 291:
	case 293:
	case 295:
	case 296:
	case 298:
	case 300:
	case 301:
	case 302:
	case 305:
	case 306:
	case 307:
	case 348:
	case 349:
		bIsLight = 1; 		
		color [0] = 
		color [1] = 
		color [2] = (GLfloat) 1; 
		range = baseRange; 
		break; 
	case 340:
	case 341:
	case 345:
	case 382:
		bIsLight = 1; 		
		color [0] = 
		color [1] = 
		color [2] = (GLfloat) 1; 
		range = baseRange / 2; 
		break; 
	case 343:
	case 344:
	case 377:
		bIsLight = 1; 
		color [0] = 
		color [1] = (GLfloat) 0.75; 
		color [2] = (GLfloat) 1; 
		range = baseRange / 2; 
		break; 
	case 346:
		bIsLight = 1; 		
		color [0] = (GLfloat) 0.5; 
		color [1] = (GLfloat) 1; 
		color [2] = (GLfloat) 0.5; 
		range = baseRange / 2; 
		break; 
	case 351:
		bIsLight = 1; 
		color [0] = (GLfloat) 0.95; 
		color [1] = (GLfloat) 0.84; 
		color [2] = (GLfloat) 0.56; 
		range = baseRange / 2; 
		break; 
	case 352:
		bIsLight = 1; 
		color [0] = (GLfloat) 1; 
		color [1] = 
		color [2] = (GLfloat) 0.25; 
		range = baseRange; 
		break; 
	case 364:
		bIsLight = 1; 
		color [0] = (GLfloat) 0.1; 
		color [1] = (GLfloat) 0.9; 
		color [2] = (GLfloat) 0.1; 
		range = baseRange / 2; 
		break; 
	case 366:
		bIsLight = 1; 
		color [0] = (GLfloat) 1; 
		color [1] = (GLfloat) 0.9; 
		color [2] = (GLfloat) 1; 
		range = baseRange / 2; 
		break; 
	case 368:
		bIsLight = 1; 
		color [0] = (GLfloat) 1; 
		color [1] = 
		color [2] = (GLfloat) 0.5; 
		range = baseRange / 2; 
		break; 
	case 370:
	case 372:
		bIsLight = 1; 
		color [0] = (GLfloat) 0.95; 
		color [1] = (GLfloat) 0.84; 
		color [2] = (GLfloat) 0.56; 
		range = baseRange / 2; 
		break; 
	case 380:
		bIsLight = 1; 
		color [0] = (GLfloat) 0.65; 
		color [1] = (GLfloat) 0.6; 
		color [2] = (GLfloat) 0.75; 
		range = baseRange / 2; 
		break; 
	case 410:
	case 427:
		bIsLight = 1; 
		color [0] = 
		color [1] = (GLfloat) 0.25; 
		color [2] = (GLfloat) 1; 
		range = baseRange / 2; 
		break; 
	case 374:
	case 375:
	case 391:
	case 392:
	case 393:
	case 394:
	case 395:
	case 396:
	case 397:
	case 398:
	case 412:
	case 429:
	case 430:
	case 431:
		bIsLight = 1; 
		color [0] = (GLfloat) 1; 
		color [1] = 
		color [2] = (GLfloat) 0.25; 
		range = baseRange / 2; 
		break; 
	case 411:
	case 428:
		bIsLight = 1; 
		color [0] = 
		color [1] = (GLfloat) 1; 
		color [2] = (GLfloat) 0.25; 
		range = baseRange / 2; 
		break; 
	case 414:
	case 416:
	case 418:
		bIsLight = 1; 
		color [0] = (GLfloat) 1.0; 
		color [1] = (GLfloat) 0.75; 
		color [2] = (GLfloat) 0.25; 
		range = baseRange / 3; 
		break; 
	case 423:
		bIsLight = 1; 
		color [0] = (GLfloat) 0.25; 
		color [1] = (GLfloat) 0.75; 
		color [2] = (GLfloat) 0.25; 
		range = baseRange; 
		break; 
	case 424:
		bIsLight = 1; 
		color [0] = (GLfloat) 0.85; 
		color [1] = (GLfloat) 0.65; 
		color [2] = (GLfloat) 0.25; 
		range = baseRange; 
		break; 
	case 235:
	case 236:
	case 237:
	case 243:
	case 244:
		bIsLight = 1; 
		color [0] = (GLfloat) 1.0; 
		color [1] = (GLfloat) 0.95; 
		color [2] = (GLfloat) 1.0; 
		range = baseRange; 
		break; 
	case 333:
		bIsLight = 1; 
		color [0] = (GLfloat) 0.5; 
		color [1] = (GLfloat) 0.33; 
		color [2] = (GLfloat) 0; 
		range = baseRange / 2; 
		break; 
	case 353:
		bIsLight = 1; 
		color [0] = (GLfloat) 0.75; 
		color [1] = (GLfloat) 0.375; 
		color [2] = (GLfloat) 0; 
		range = baseRange / 2; 
		break; 
	case 356:
	case 357:
	case 358:
		bIsLight = 1; 
		color [0] = (GLfloat) 1; 
		color [1] = 
		color [2] = (GLfloat) 0.25; 
		range = baseRange; 
		break; 
	case 359:
		bIsLight = 1; 
		color [0] = (GLfloat) 1; 
		color [1] = 
		color [2] = (GLfloat) 0; 
		range = baseRange / 2; 
		break; 
	//Lava gameData.pig.tex.bmIndex
	case 378:
	case 404:
	case 405:
	case 406:
	case 407:
	case 408:
		bIsLight = 1; 
		color [0] = (GLfloat) 1; 
		color [1] = (GLfloat) 0.5; 
		color [2] = (GLfloat) 0; 
		range = baseRange; 
		break; 
	case 409:
	case 426:
	case 434:
		bIsLight = 1; 
		color [0] = (GLfloat) 1; 
		color [1] = (GLfloat) 0.25; 
		color [2] = (GLfloat) 0; 
		range = baseRange; 
		break; 
	case 420:
	case 432:
	case 433:
		bIsLight = 1; 
		color [0] = (GLfloat) 0; 
		color [1] = (GLfloat) 0.25; 
		color [2] = (GLfloat) 1; 
		range = baseRange; 
		break; 
	default:
		if (gameData.pig.tex.pTMapInfo [t].lighting) {
			bIsLight = 1; 
			color [0] = 
			color [1] = 
			color [2] = (GLfloat) 1; 
			range = baseRange; 
			}
		break; 
	}
if (bIsLight) {
#if 1
#	if 0
	if (gameData.pig.tex.tMapInfo [t].lighting)
		range = baseRange * (double) gameData.pig.tex.tMapInfo [t].lighting / (double) (F1_0 / 2);
#	endif
	memcpy (colorP, color, sizeof (color));
	return range;
#else
	if (gameData.pig.tex.tMapInfo [t].lighting)
		*pTempLight = lmi;
	else
		bIsLight = 0;
#endif
	}
return 0;
}

//------------------------------------------------------------------------------

void InitLightMapTexture (tLightMap *lmP, float fColor)
{
float *colorP = (float *) (&lmP->bmP);
for (int i = LIGHTMAP_WIDTH * LIGHTMAP_WIDTH * 4; i; i--)
	*colorP++ = fColor;
}

//------------------------------------------------------------------------------

int InitLightData (int bVariable)
{
	tDynLight		*pl;
	grsFace			*faceP = NULL;
	int				bIsLight, nIndex, i; 
	short				t; 
	tLightMapInfo	*lmiP;  //temporary place to put light data.
	double			sideRad;
	double			baseRange = LightMapRange ();

//first step find all the lights in the level.  By iterating through every surface in the level.
if (!(nLights = CountLights (bVariable)))
	return 0;
if (!(lightMapInfo = (tLightMapInfo *) D2_ALLOC (sizeof (tLightMapInfo) * nLights)))
	return nLights = 0; 
if (!(lightMaps = (tLightMap *) D2_ALLOC ((gameData.segs.nFaces + 1) * sizeof (tLightMap)))) {
	D2_FREE (lightMapInfo);
	return nLights = 0; 
	}
memset (lightMaps, 0, sizeof (tLightMap) * gameData.segs.nFaces); 
memset (lightMapInfo, 0, sizeof (tLightMapInfo) * nLights); 
nLights = 0; 
//first lightmap is dummy lightmap for multi pass lighting
InitLightMapTexture (lightMaps, 0.0f);
lmiP = lightMapInfo; 
for (pl = gameData.render.lights.dynamic.lights, i = gameData.render.lights.dynamic.nLights; i; i--, pl++) {
	if (pl->nType || (pl->bVariable && !bVariable))
		continue;
	pl->bOn = 0; //replaced by lightmaps
	if (faceP == pl->faceP)
		continue;
	faceP = pl->faceP;
	bIsLight = 0; 
	t = IsLight (faceP->nBaseTex) ? faceP->nBaseTex : faceP->nOvlTex;
	if (0 == (lmiP->range = GetLightColor (t, &lmiP->color [0])))
		continue;
	bIsLight = 1;
	sideRad = (double) faceP->rad;
	nIndex = faceP->nSegment * 6 + faceP->nSide;
	if (gameStates.app.bD2XLevel && (gameData.render.color.lights [nIndex].index)) { 
		bIsLight = 1;
		lmiP->color [0] = (GLfloat) gameData.render.color.lights [nIndex].color.red; 
		lmiP->color [1] = (GLfloat) gameData.render.color.lights [nIndex].color.green; 
		lmiP->color [2] = (GLfloat) gameData.render.color.lights [nIndex].color.blue; 
		lmiP->range = baseRange; 
		}
	//Process found light.
	if (bIsLight) {
		lmiP->range += sideRad;
		//find where it is in the level.
		lmiP->vPos = SIDE_CENTER_V (faceP->nSegment, faceP->nSide);
		lmiP->nIndex = nIndex; 

		//find light direction, currently based on first 3 points of tSide, not always right.
		vmsVector *normalP = SEGMENTS [faceP->nSegment].sides [faceP->nSide].normals;
		VmVecAvg (&lmiP->vDir, normalP, normalP + 1);
		lmiP++; 
		}
	}
return nLights = (int) (lmiP - lightMapInfo); 
}

//------------------------------------------------------------------------------

#if LMAP_REND2TEX

// create 512 brightness values using inverse square
void InitBrightMap (ubyte *brightMap)
{
	int		i;
	double	h;

for (i = 512; i; i--, brightMap++) {
	h = (double) i / 512.0;
	h *= h;
	*brightMap = (ubyte) ((255 * h) + 0.5);
	}
}

//------------------------------------------------------------------------------

// create a color/brightness table
void InitLightMapInfo (ubyte *lightMap, ubyte *brightMap, GLfloat *color)
{
	int		i, j;
	double	h;

for (i = 512; i; i--, brightMap++)
	for (j = 0; j < 3; j++, lightMap++)
		*lightMap = (ubyte) (*brightMap * color [j] + 0.5);
}

#endif //LMAP_REND2TEX

//------------------------------------------------------------------------------

void ComputeLightMaps (int nFace)
{
	grsFace			*faceP;
	tSide				*sideP; 
	tLightMapInfo	*lmiP; 
	int				nLastFace; 
	ushort			sideVerts [4]; 

#if 1
#	define		LM_W LIGHTMAP_WIDTH
#	define		LM_H LIGHTMAP_WIDTH
#else
	int			LM_W = 8, LM_H = 8; 
#endif
	int			x, y, xy; 
	int			v0, v1, v2, v3; 
	GLfloat		*pTexColor;// = {0.0, 0.0, 0.0, 1.0}; 
	GLfloat		texColor [LM_W][LM_H][3];

#if 1
#	define		pixelOffset 0.0
#else
	double		pixelOffset = 0; //0.5
#endif
	int			l, s, nMethod; 
	GLfloat		tempBright = 0; 
	vmsVector	OffsetU, OffsetV, pixelPos [LM_W][LM_H], *pPixelPos, rayVec, vFaceNorms [4], vNormalP, sidePos; 
	double		brightPrct, sideRad, lightRange, pixelDist; 
	double		delta; 
	double		f_offset [8] = {
						0.0 / (LM_W - 1), 1.0 / (LM_W - 1), 2.0 / (LM_W - 1), 3.0 / (LM_W - 1),
						4.0 / (LM_W - 1), 5.0 / (LM_W - 1), 6.0 / (LM_W - 1), 7.0 / (LM_W - 1)
						};
#if LMAP_REND2TEX
	ubyte			brightMap [512];
	ubyte			lightMap [512*3];
	tUVL			lMapUVL [4];
	fix			nDist, nMinDist;
	GLuint		lightMapId;
	int			bStart;
#endif

if (gameStates.app.bMultiThreaded)
	nLastFace = nFace ? gameData.segs.nFaces : gameData.segs.nFaces / 2;
else
	INIT_PROGRESS_LOOP (nFace, nLastFace, gameData.segs.nFaces);
//Next Go through each surface and create a lightmap for it.
for (faceP = FACES + nFace; nFace < nLastFace; nFace++, faceP++) {
#ifdef _DEBUG
if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
	sideP = SEGMENTS [faceP->nSegment].sides + faceP->nSide;
	memcpy (sideVerts, faceP->index, sizeof (sideVerts));
#if LMAP_REND2TEX
	OglCreateFBuffer (&lightMaps [nFace].fbuffer, 64, 64);
	OglEnableFBuffer (&lightMaps [nFace].fbuffer);
#else
	nMethod = (sideP->nType == SIDE_IS_QUAD) || (sideP->nType == SIDE_IS_TRI_02);
	pPixelPos = &pixelPos [0][0];
#if 0
	VmVecNormal (vFaceNorms, gameData.segs.vertices + v0, gameData.segs.vertices + v2, gameData.segs.vertices + v1); 
	VmVecNormal (vFaceNorms + 1, gameData.segs.vertices + v0, gameData.segs.vertices + v3, gameData.segs.vertices + v2); 
	VmVecNormal (vFaceNorms + 2, gameData.segs.vertices + v0, gameData.segs.vertices + v1, gameData.segs.vertices + v3); 
	VmVecNormal (vFaceNorms + 3, gameData.segs.vertices + v2, gameData.segs.vertices + v1, gameData.segs.vertices + v3); 
#endif
	for (x = 0; x < LM_W; x++) {
		for (y = 0; y < LM_H; y++, pPixelPos++) {
			if (nMethod) {
				v0 = sideVerts [0]; 
				v2 = sideVerts [2]; 
				if (x >= y)	{
					v1 = sideVerts [1]; 
					//Next calculate this pixel's place in the world (tricky stuff)
					FindOffset (&OffsetU, gameData.segs.vertices [v0], gameData.segs.vertices [v1], f_offset [x]); //(((double) x) + pixelOffset) / (LM_W - 1)); //took me forever to figure out this should be an inverse thingy
					FindOffset (&OffsetV, gameData.segs.vertices [v1], gameData.segs.vertices [v2], f_offset [y]); //(((double) y) + pixelOffset) / (LM_H - 1)); 
					VmVecAdd (pPixelPos, &OffsetU, &OffsetV); 
					VmVecInc (pPixelPos, gameData.segs.vertices + v0);  //This should be the real world position of the pixel.
					//Find Normal
					//vNormalP = vFaceNorms;
					}
				else {
					//Next calculate this pixel's place in the world (tricky stuff)
					v3 = sideVerts [3]; 
					FindOffset (&OffsetV, gameData.segs.vertices [v0], gameData.segs.vertices [v3], f_offset [y]); //(((double) y) + pixelOffset) / (LM_W - 1)); //Notice y/x and OffsetU/OffsetV are swapped from above
					FindOffset (&OffsetU, gameData.segs.vertices [v3], gameData.segs.vertices [v2], f_offset [x]); //(((double) x) + pixelOffset) / (LM_H - 1)); 
					VmVecAdd (pPixelPos, &OffsetU, &OffsetV); 
					VmVecInc (pPixelPos, gameData.segs.vertices + v0);  //This should be the real world position of the pixel.
					//vNormalP = vFaceNorms + 1;
					}
				}
			else {//SIDE_IS_TRI_02
				v1 = sideVerts [1]; 
				v3 = sideVerts [3]; 
				if (LM_W - x >= y) {
					v0 = sideVerts [0]; 
					FindOffset (&OffsetU, gameData.segs.vertices [v0], gameData.segs.vertices [v1], f_offset [x]); //(((double) x) + pixelOffset) / (LM_W - 1)); 
					FindOffset (&OffsetV, gameData.segs.vertices [v0], gameData.segs.vertices [v3], f_offset [y]); //(((double) y) + pixelOffset) / (LM_W - 1)); 
					VmVecAdd (pPixelPos, &OffsetU, &OffsetV); 
					VmVecInc (pPixelPos, gameData.segs.vertices + v0);  //This should be the real world position of the pixel.
					//vNormalP = vFaceNorms + 2;
					}
				else {
					v2 = sideVerts [2]; 
					//Not certain this is correct, may need to subtract something
					FindOffset (&OffsetV, gameData.segs.vertices [v2], gameData.segs.vertices [v1], f_offset [LM_W - 1 - y]); //((double) ((LM_W - 1) - y) + pixelOffset) / (LM_W - 1)); 
					FindOffset (&OffsetU, gameData.segs.vertices [v2], gameData.segs.vertices [v3], f_offset [LM_W - 1 - x]); //((double) ((LM_W - 1) - x) + pixelOffset) / (LM_W - 1)); 
					VmVecAdd (pPixelPos, &OffsetU, &OffsetV); 
					VmVecInc (pPixelPos, gameData.segs.vertices + v2);  //This should be the real world position of the pixel.
					//vNormalP = vFaceNorms + 3;
					}
				}
			}
		}
#endif
	//Calculate LightVal
	//Next iterate through all the lights and add the light to the pixel every iteration.
	sideRad = (double) faceP->rad;
	VmVecAvg4 (
		&sidePos, 
		&pixelPos [0][0],
		&pixelPos [LM_W-1][0],
		&pixelPos [LM_W-1][LM_H-1],
		&pixelPos [0][LM_H-1]);
#if LMAP_REND2TEX
	bStart = 1;
#endif
	memset (texColor, 0, sizeof (texColor));
	for (l = 0, lmiP = lightMapInfo; l < nLights; l++, lmiP++) {
#if LMAP_REND2TEX
		nMinDist = 0x7FFFFFFF;
		// get the distances of all 4 tSide corners to the light source center 
		// scaled by the light source range
		for (i = 0; i < 4; i++) {
			int svi = sideVerts [i];
			sidePos.x = gameData.segs.vertices [svi].x;
			sidePos.y = gameData.segs.vertices [svi].y;
			sidePos.z = gameData.segs.vertices [svi].z;
			nDist = f2i (VmVecDist (&sidePos, &lmiP->vPos));	// calc distance
			if (nMinDist > nDist)
				nMinDist = nDist;
			lMapUVL [i].u = F1_0 * (double) nDist / (double) lmiP->range;	// scale distance
			}
		if ((lmiP->color [0] + lmiP->color [1] + lmiP->color [2] < 3) &&
			(nMinDist < lmiP->range + sideRad)) {
			// create and initialize an OpenGL texture for the lightmap
			InitLightMapInfo (lightMap, brightMap, lmiP->color);
			OglGenTextures (1, &lightMapId); 
			glTexImage1D (GL_TEXTURE_1D, 0, GL_RGB, 512, 1, GL_RGB, GL_UNSIGNED_BYTE, lightMap);
			OglActiveTexture (GL_TEXTURE0);
			glEnable (GL_TEXTURE_1D);
			glEnable (GL_BLEND);
			glBlendFunc (GL_ONE, bStart ? GL_ZERO : GL_ONE);
			// If processing first light, set the lightmap, else modify it
			glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, bStart ? GL_REPLACE : GL_ADD);
			glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GL_RGBA);
			glBindTexture (GL_TEXTURE_1D, lightMapId); 
			// extend the lightmap to the texture edges
			glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glBegin (GL_QUADS);
			glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
			for (i = 0; i < 4; i++) {
				glMultiTexCoord2f (GL_TEXTURE0, f2fl (lMapUVL [i].u), f2fl (lMapUVL [i].v));
				glVertex3f (f2fl (gameData.segs.vertices [sideVerts [i]].x), 
								f2fl (gameData.segs.vertices [sideVerts [i]].y), 
							   f2fl (gameData.segs.vertices [sideVerts [i]].z));
				}
			glEnd ();
			glDisable (GL_BLEND);
			glDisable (GL_TEXTURE_1D);
			OglDeleteTextures (1, &lightMapId);
			bStart = 0;
			}
#else
		lightRange = lmiP->range + sideRad;
		if (f2fl (VmVecDist (&sidePos, &lmiP->vPos)) <= lightRange) {
			pPixelPos = &pixelPos [0][0];
			pTexColor = texColor [0][0];
#if 1
			for (xy = LM_W * LM_H; xy; xy--, pPixelPos++, pTexColor += 3) { 
#else
			for (x = 0; x < LM_W; x++)
				for (y = 0; y < LM_H; y++, pPixelPos++, pTexColor += 3) {
#endif
					//Find angle to this light.
					pixelDist = f2fl (VmVecDist (pPixelPos, &lmiP->vPos)); 
					if (pixelDist > lightRange)
						continue;
					VmVecSub (&rayVec, &lmiP->vPos, pPixelPos); 
					delta = f2db (VmVecDeltaAng (&lmiP->vDir, &rayVec, NULL)); 
					if (delta < 0)
						delta = -delta; 
					if (pixelDist <= sideRad)
						brightPrct = 1;
					else {
						brightPrct = 1 - (pixelDist / lmiP->range); 
						brightPrct *= brightPrct; //square result
						if (delta < 0.245)
							brightPrct /= 4; 
						}
					pTexColor [0] += (GLfloat) (brightPrct * lmiP->color [0]); 
					pTexColor [1] += (GLfloat) (brightPrct * lmiP->color [1]); 
					pTexColor [2] += (GLfloat) (brightPrct * lmiP->color [2]); 
					}
			}
#endif
		}
#if LMAP_REND2TEX
	lightMaps [nFace].handle = lightMaps [nFace].fbuffer.texId;
	lightMaps [nFace].fbuffer.texId = 0;
	OglDestroyFBuffer (&lightMaps [nFace].fbuffer);
#else
	pPixelPos = &pixelPos [0][0];
	pTexColor = texColor [0][0];
	for (x = 0; x < LM_W; x++)
		for (y = 0; y < LM_H; y++, pPixelPos++, pTexColor += 3) {
			tempBright = pTexColor [0];
			for (s = 1; s < 3; s++)
				if (pTexColor [s] > tempBright)
					tempBright = pTexColor [s]; 
			if (tempBright > 1.0)
				for (s = 0; s < 3; s++)
					pTexColor [s] /= tempBright; 
			}
	memcpy (&lightMaps [nFace + 1].bmP, texColor, sizeof (texColor));
#endif
	}
}

//------------------------------------------------------------------------------

static tThreadInfo	ti [2];

int _CDECL_ LightMapThread (void *pThreadId)
{
	int		nId = *((int *) pThreadId);

ComputeLightMaps (nId ? gameData.segs.nFaces / 2 : 0);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
return 0;
}

//------------------------------------------------------------------------------

static void StartLightMapThreads (pThreadFunc pFunc)
{
	int	i;

for (i = 0; i < 2; i++) {
	ti [i].bDone = 0;
	ti [i].done = SDL_CreateSemaphore (0);
	ti [i].nId = i;
	ti [i].pThread = SDL_CreateThread (pFunc, &ti [i].nId);
	}
#if 1
SDL_SemWait (ti [0].done);
SDL_SemWait (ti [1].done);
#else
while (!(ti [0].bDone && ti [1].bDone))
	G3_SLEEP (0);
#endif
for (i = 0; i < 2; i++) {
	SDL_WaitThread (ti [i].pThread, NULL);
	SDL_DestroySemaphore (ti [i].done);
	}
}

//------------------------------------------------------------------------------

static int nFace = 0;

static void CreateLightMapsPoll (int nItems, tMenuItem *m, int *key, int cItem)
{
GrPaletteStepLoad (NULL);
if (nFace < gameData.segs.nFaces) {
	ComputeLightMaps (nFace);
	nFace += PROGRESS_INCR;
	}
else {
	*key = -2;
	GrPaletteStepLoad (NULL);
	return;
	}
m [0].value++;
m [0].rebuild = 1;
*key = 0;
GrPaletteStepLoad (NULL);
return;
}

//------------------------------------------------------------------------------

void CreateLightMaps (void)
{
DestroyLightMaps ();
if (!InitLightData (0))
	return;
#if LMAP_REND2TEX
InitBrightMap (brightMap);
memset (&lMapUVL, 0, sizeof (lMapUVL));
#endif
if (gameStates.render.color.bLightMapsOk && 
	 gameOpts->ogl.bPerPixelLighting && 
	 gameData.segs.nFaces) {
	if (gameStates.app.bMultiThreaded && (gameData.segs.nSegments > 8))
		StartLightMapThreads (LightMapThread);
	else if (gameStates.app.bProgressBars && gameOpts->menus.nStyle) {
		nFace = 0;
		NMProgressBar (TXT_CALC_LIGHTMAPS, 0, PROGRESS_STEPS (gameData.segs.nFaces), CreateLightMapsPoll);
		}
	else
		ComputeLightMaps (-1);
	}
if (!lightMaps) {
	InitLightMapTexture (&dummyLightMap, 0.0f);
	lightMaps = &dummyLightMap;
	}
OglCreateLightMaps ();
}

//------------------------------------------------------------------------------

#if DBG_SHADERS

char *lightMapFS [3] = {"lightmaps1.frag", "lightmaps2.frag", "lightmaps3.frag"};
char *lightMapVS [3] = {"lightmaps1.vert", "lightmaps2.vert", "lightmaps3.vert"};

#else

char *lightMapFS [3] = {
	"uniform sampler2D btmTex,topTex,lMapTex;" \
	"float maxC;" \
	"vec4 btmColor,topColor,lMapColor;" \
	"void main(void){" \
	"btmColor=texture2D(btmTex,vec2(gl_TexCoord[0]));" \
	"topColor=texture2D(topTex,vec2(gl_TexCoord[1]));" \
	"lMapColor=texture2D(lMapTex,vec2(gl_TexCoord[2]))+((gl_Color)-0.5);" \
	"maxC=lMapColor.r;" \
	"if(lMapColor.g>maxC)maxC=lMapColor.g;" \
	"if(lMapColor.b>maxC)maxC=lMapColor.b;" \
	"if(maxC>1.0){lMapColor.r/=maxC;lMapColor.g/=maxC;lMapColor.b/=maxC;}" \
	"lMapColor.a = gl_Color.a;" \
	"gl_FragColor = vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a))*lMapColor;}"
	,
	"uniform sampler2D btmTex,topTex,lMapTex;" \
	"float maxC;" \
	"vec4 btmColor,topColor,lMapColor;" \
	"void main(void){" \
	"topColor=texture2D(topTex,vec2(gl_TexCoord[1]));" \
	"if(abs(topColor.a-1.0/255.0)<0.25)discard;" \
	"if((topColor.a==0.0)&&(abs(topColor.r-120.0/255.0)<8.0/255.0)&&(abs(topColor.g-88.0/255.0)<8.0/255.0)&&(abs(topColor.b-128.0/255.0)<8.0/255.0))discard;" \
	"else {btmColor=texture2D(btmTex,vec2(gl_TexCoord[0]));" \
	"lMapColor=texture2D(lMapTex,vec2(gl_TexCoord[2]))+((gl_Color)-0.5);" \
	"maxC=lMapColor.r;" \
	"if(lMapColor.g>maxC)maxC=lMapColor.g;" \
	"if(lMapColor.b>maxC)maxC=lMapColor.b;" \
	"if(maxC>1.0){lMapColor.r/=maxC;lMapColor.g/=maxC;lMapColor.b/=maxC;}" \
	"if(topColor.a==0.0)gl_FragColor = btmColor*lMapColor;" \
	"else {lMapColor.a = gl_Color.a;" \
	"gl_FragColor = vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a))*lMapColor;}}}"
	,
	"uniform sampler2D btmTex,topTex,lMapTex,maskTex;" \
	"float maxC,r,g,b;" \
	"vec4 btmColor,topColor,lMapColor;" \
	"float bMask;" \
	"void main(void){" \
	"bMask=texture2D(maskTex,vec2(gl_TexCoord[1])).a;" \
	"if(bMask<0.5)discard;" \
	"else {btmColor=texture2D(btmTex,vec2(gl_TexCoord[0]));" \
	"topColor=texture2D(topTex,vec2(gl_TexCoord[1]));" \
	"lMapColor=texture2D(lMapTex,vec2(gl_TexCoord[2]))+((gl_Color)-0.5);" \
	"maxC=lMapColor.r;" \
	"if(lMapColor.g>maxC)maxC=lMapColor.g;" \
	"if(lMapColor.b>maxC)maxC=lMapColor.b;" \
	"if(maxC>1.0){lMapColor.r/=maxC;lMapColor.g/=maxC;lMapColor.b/=maxC;}" \
	"if(topColor.a==0.0)gl_FragColor = btmColor*lMapColor;" \
	"else {lMapColor.a = gl_Color.a;" \
	"gl_FragColor = vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a))*lMapColor;}}}"
	};

char *lightMapVS [3] = {
	"void main(void) {" \
	"gl_TexCoord[0]=gl_MultiTexCoord0;" \
	"gl_TexCoord[1]=gl_MultiTexCoord1;" \
	"gl_TexCoord[2]=gl_MultiTexCoord2;" \
	"gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;" \
	"gl_FrontColor=gl_Color;}"
	,
	"void main(void) {" \
	"gl_TexCoord[0]=gl_MultiTexCoord0;" \
	"gl_TexCoord[1]=gl_MultiTexCoord1;" \
	"gl_TexCoord[2]=gl_MultiTexCoord2;" \
	"gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;" \
	"gl_FrontColor=gl_Color;}"
	,
	"void main(void) {" \
	"gl_TexCoord[0]=gl_MultiTexCoord0;" \
	"gl_TexCoord[1]=gl_MultiTexCoord1;" \
	"gl_TexCoord[2]=gl_MultiTexCoord2;" \
	"gl_TexCoord[3]=gl_MultiTexCoord3;" \
	"gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;" \
	"gl_FrontColor=gl_Color;}"
	};

#endif

//------------------------------------------------------------------------------

void InitLightmapShaders (void)
{
	int	i;

if (!gameStates.ogl.bShadersOk)
	gameStates.render.color.bLightMapsOk = 0;
if (gameStates.render.color.bLightMapsOk) {
	PrintLog ("building lightmap shader programs\n");
	for (i = 0; i < 2; i++) {
		if (lmShaderProgs [i])
			DeleteShaderProg (lmShaderProgs + i);
		gameStates.render.color.bLightMapsOk = 
			CreateShaderProg (lmShaderProgs + i) &&
			CreateShaderFunc (lmShaderProgs + i, lmFS + i, lmVS + i, lightMapFS [i], lightMapVS [i], 1) &&
			LinkShaderProg (lmShaderProgs + i);
		if (!gameStates.render.color.bLightMapsOk) {
			while (i)
				DeleteShaderProg (lmShaderProgs + --i);
			break;
			}
		}
	}
}

//------------------------------------------------------------------------------

