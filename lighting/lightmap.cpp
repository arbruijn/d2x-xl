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
#include "ogl_color.h"
#include "light.h"
#include "dynlight.h"
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
#define TEXTURE_CHECK	1

#define LIGHTMAP_DATA_VERSION 5

#define LM_W	LIGHTMAP_WIDTH
#define LM_H	LIGHTMAP_WIDTH

//------------------------------------------------------------------------------

typedef struct tLightMapDataHeader {
	int	nVersion;
	int	nSegments;
	int	nVertices;
	int	nFaces;
	int	nLights;
	int	nMaxLightRange;
	int	nBuffers;
	} tLightMapDataHeader;

//------------------------------------------------------------------------------

GLhandleARB lmShaderProgs [3] = {0,0,0}; 
GLhandleARB lmFS [3] = {0,0,0}; 
GLhandleARB lmVS [3] = {0,0,0}; 

int lightMapWidth [5] = {8, 16, 32, 64, 128};

tLightMap dummyLightMap;

tLightMapData lightMapData = {NULL, NULL, 0, 0};

//------------------------------------------------------------------------------

int InitLightData (int bVariable);

//------------------------------------------------------------------------------

int OglCreateLightMap (int nLightMap)
{
	tLightMapBuffer	*lmP = lightMapData.buffers + nLightMap;
#ifdef _DEBUG
	int					nError;
#endif

if (lmP->handle)
	return 1;
OglGenTextures (1, &lmP->handle);
if (!lmP->handle) {
#if 0//def _DEBUG
	nError = glGetError ();
#endif
	return 0;
	}
OGL_BINDTEX (lmP->handle); 
#if 0//def _DEBUG
if ((nError = glGetError ()))
	return 0;
#endif
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexImage2D (GL_TEXTURE_2D, 0, 3, LIGHTMAP_BUFWIDTH, LIGHTMAP_BUFWIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, lmP->bmP);
#ifdef _DEBUG
if ((nError = glGetError ()))
	return 0;
#endif
return 1;
}

//------------------------------------------------------------------------------

int OglCreateLightMaps (void)
{
for (int i = 0; i < lightMapData.nBuffers; i++)
	if (!OglCreateLightMap (i))
		return 0;
return 1;
}

//------------------------------------------------------------------------------

void OglDestroyLightMaps (void)
{
if (lightMapData.buffers) { 
	tLightMapBuffer *lmP = lightMapData.buffers;
	for (int i = lightMapData.nBuffers; i; i--, lmP++)
		if (lmP->handle) {
			OglDeleteTextures (1, (GLuint *) &lmP->handle);
			lmP->handle = 0;
			}
	} 
}

//------------------------------------------------------------------------------

void DestroyLightMaps (void)
{
if (lightMapData.info) { 
	OglDestroyLightMaps ();
	D2_FREE (lightMapData.info);
	D2_FREE (lightMapData.buffers);
	}
}

//------------------------------------------------------------------------------

inline void ComputePixelPos (vmsVector *vPos, vmsVector v1, vmsVector v2, double fOffset)
{
vPos->p.x = (fix) (fOffset * (v2.p.x - v1.p.x)); 
vPos->p.y = (fix) (fOffset * (v2.p.y - v1.p.y)); 
vPos->p.z = (fix) (fOffset * (v2.p.z - v1.p.z)); 
}

//------------------------------------------------------------------------------

void RestoreLights (int bVariable)
{
	tDynLight	*pl;
	int			i;

for (pl = gameData.render.lights.dynamic.lights, i = gameData.render.lights.dynamic.nLights; i; i--, pl++)
	if (!(pl->info.nType || (pl->info.bVariable && !bVariable)))
		pl->info.bOn = 1;
}

//------------------------------------------------------------------------------

int CountLights (int bVariable)
{
	tDynLight		*pl;
	int				i, nLights = 0;

if (!(gameStates.render.bPerPixelLighting))
	return 0;
for (pl = gameData.render.lights.dynamic.lights, i = gameData.render.lights.dynamic.nLights; i; i--, pl++)
	if (!(pl->info.nType || (pl->info.bVariable && !bVariable)))
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
if (!(lightMapData.nLights = CountLights (bVariable)))
	return 0;
if (!(lightMapData.info = (tLightMapInfo *) D2_ALLOC (sizeof (tLightMapInfo) * lightMapData.nLights)))
	return lightMapData.nLights = 0; 
lightMapData.nBuffers = (gameData.segs.nFaces + LIGHTMAP_BUFSIZE - 1) / LIGHTMAP_BUFSIZE;
if (!(lightMapData.buffers = (tLightMapBuffer *) D2_ALLOC (lightMapData.nBuffers * sizeof (tLightMapBuffer)))) {
	D2_FREE (lightMapData.info);
	return lightMapData.nLights = 0; 
	}
memset (lightMapData.buffers, 0, lightMapData.nBuffers * sizeof (tLightMapBuffer)); 
memset (lightMapData.info, 0, sizeof (tLightMapInfo) * lightMapData.nLights); 
lightMapData.nLights = 0; 
//first lightmap is dummy lightmap for multi pass lighting
lmiP = lightMapData.info; 
for (pl = gameData.render.lights.dynamic.lights, i = gameData.render.lights.dynamic.nLights; i; i--, pl++) {
	if (pl->info.nType || (pl->info.bVariable && !bVariable))
		continue;
	if (faceP == pl->info.faceP)
		continue;
	faceP = pl->info.faceP;
	bIsLight = 0; 
	t = IsLight (faceP->nBaseTex) ? faceP->nBaseTex : faceP->nOvlTex;
	if (0 == (lmiP->range = GetLightColor (t, &lmiP->color [0])))
		continue;
	bIsLight = 1;
	sideRad = (double) faceP->fRad / 10.0;
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
return lightMapData.nLights = (int) (lmiP - lightMapData.info); 
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

void CopyLightMap (tRgbColorb *texColorP, ushort nLightmap)
{
tLightMapBuffer *bufP = lightMapData.buffers + nLightmap / LIGHTMAP_BUFSIZE;
int i = nLightmap % LIGHTMAP_BUFSIZE;
int x = (i % LIGHTMAP_ROWSIZE) * LM_W;
int y = (i / LIGHTMAP_ROWSIZE) * LM_H;
for (i = 0; i < LM_H; i++, y++, texColorP += LM_W)
	memcpy (&bufP->bmP [y][x], texColorP, LM_W * sizeof (tRgbColorb));
}

//------------------------------------------------------------------------------

void CreateSpecialLightMap (tRgbColorb *texColorP, ushort nLightmap, ubyte nColor)
{
memset (texColorP, nColor, LM_W * LM_H * sizeof (tRgbColorb));
CopyLightMap (texColorP, nLightmap);
}

//------------------------------------------------------------------------------

void ComputeLightMaps (int nFace, int nThread)
{
	grsFace			*faceP;
	tSide				*sideP; 
	int				nLastFace; 
	ushort			sideVerts [4]; 

	int			x, y, i; 
	int			v0, v1, v2, v3; 
	fVector3		color;
	tRgbColorb	texColor [MAX_LIGHTMAP_WIDTH * MAX_LIGHTMAP_WIDTH], *texColorP;

#if 1
#	define		pixelOffset 0.0
#else
	double		pixelOffset = 0; //0.5
#endif
	int			nType, nBlackLightmaps = 0, nWhiteLightmaps = 0; 
	GLfloat		maxColor = 0; 
	vmsVector	offsetU, offsetV, pixelPos [MAX_LIGHTMAP_WIDTH * MAX_LIGHTMAP_WIDTH], *pPixelPos; 
	vmsVector	vNormal;
	double		fOffset [MAX_LIGHTMAP_WIDTH];
	bool			bBlack, bWhite;
	tVertColorData	vcd;

for (i = 0; i < LM_W; i++)
	fOffset [i] = (double) i / (double) (LM_W - 1);
InitVertColorData (vcd);
vcd.pVertPos = &vcd.vertPos;
vcd.fMatShininess = 4;

if (gameStates.app.bMultiThreaded)
	nLastFace = nFace ? gameData.segs.nFaces : gameData.segs.nFaces / 2;
else
	INIT_PROGRESS_LOOP (nFace, nLastFace, gameData.segs.nFaces);
if (nFace <= 0) {
	CreateSpecialLightMap (texColor, 0, 0);
	CreateSpecialLightMap (texColor, 1, 255);
	lightMapData.nLightmaps = 2;
	}
//Next Go through each surface and create a lightmap for it.
for (faceP = FACES + nFace; nFace < nLastFace; nFace++, faceP++) {
#ifdef _DEBUG
	if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	sideP = SEGMENTS [faceP->nSegment].sides + faceP->nSide;
	memcpy (sideVerts, faceP->index, sizeof (sideVerts));
	nType = (sideP->nType == SIDE_IS_QUAD) || (sideP->nType == SIDE_IS_TRI_02);
	pPixelPos = pixelPos;
	for (x = 0; x < LM_W; x++) {
		for (y = 0; y < LM_H; y++, pPixelPos++) {
			if (nType) {
				v0 = sideVerts [0]; 
				v2 = sideVerts [2]; 
				if (x >= y)	{
					v1 = sideVerts [1]; 
					//Next calculate this pixel's place in the world (tricky stuff)
					ComputePixelPos (&offsetU, gameData.segs.vertices [v0], gameData.segs.vertices [v1], fOffset [x]); //(((double) x) + pixelOffset) / (LM_W - 1)); //took me forever to figure out this should be an inverse thingy
					ComputePixelPos (&offsetV, gameData.segs.vertices [v1], gameData.segs.vertices [v2], fOffset [y]); //(((double) y) + pixelOffset) / (LM_H - 1)); 
					VmVecAdd (pPixelPos, &offsetU, &offsetV); 
					VmVecInc (pPixelPos, gameData.segs.vertices + v0);  //This should be the real world position of the pixel.
					//Find Normal
					//vNormalP = vFaceNorms;
					}
				else {
					//Next calculate this pixel's place in the world (tricky stuff)
					v3 = sideVerts [3]; 
					ComputePixelPos (&offsetV, gameData.segs.vertices [v0], gameData.segs.vertices [v3], fOffset [y]); //(((double) y) + pixelOffset) / (LM_W - 1)); //Notice y/x and offsetU/offsetV are swapped from above
					ComputePixelPos (&offsetU, gameData.segs.vertices [v3], gameData.segs.vertices [v2], fOffset [x]); //(((double) x) + pixelOffset) / (LM_H - 1)); 
					VmVecAdd (pPixelPos, &offsetU, &offsetV); 
					VmVecInc (pPixelPos, gameData.segs.vertices + v0);  //This should be the real world position of the pixel.
					//vNormalP = vFaceNorms + 1;
					}
				}
			else {//SIDE_IS_TRI_02
				v1 = sideVerts [1]; 
				v3 = sideVerts [3]; 
				if (LM_W - x >= y) {
					v0 = sideVerts [0]; 
					ComputePixelPos (&offsetU, gameData.segs.vertices [v0], gameData.segs.vertices [v1], fOffset [x]); //(((double) x) + pixelOffset) / (LM_W - 1)); 
					ComputePixelPos (&offsetV, gameData.segs.vertices [v0], gameData.segs.vertices [v3], fOffset [y]); //(((double) y) + pixelOffset) / (LM_W - 1)); 
					VmVecAdd (pPixelPos, &offsetU, &offsetV); 
					VmVecInc (pPixelPos, gameData.segs.vertices + v0);  //This should be the real world position of the pixel.
					//vNormalP = vFaceNorms + 2;
					}
				else {
					v2 = sideVerts [2]; 
					//Not certain this is correct, may need to subtract something
					ComputePixelPos (&offsetV, gameData.segs.vertices [v2], gameData.segs.vertices [v1], fOffset [LM_W - 1 - y]); //((double) ((LM_W - 1) - y) + pixelOffset) / (LM_W - 1)); 
					ComputePixelPos (&offsetU, gameData.segs.vertices [v2], gameData.segs.vertices [v3], fOffset [LM_W - 1 - x]); //((double) ((LM_W - 1) - x) + pixelOffset) / (LM_W - 1)); 
					VmVecAdd (pPixelPos, &offsetU, &offsetV); 
					VmVecInc (pPixelPos, gameData.segs.vertices + v2);  //This should be the real world position of the pixel.
					//vNormalP = vFaceNorms + 3;
					}
				}
			}
		}
#ifdef _DEBUG
	if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	if (SEGMENT2S [faceP->nSegment].special != SEGMENT_IS_SKYBOX) {
		VmVecAvg (&vNormal, sideP->normals, sideP->normals + 1);
		VmVecFixToFloat (&vcd.vertNorm, &vNormal);
		pPixelPos = pixelPos;
		texColorP = texColor;
		bBlack = 
		bWhite = true;
		memset (texColor, 0, LM_W * LM_H * sizeof (tRgbColorb));
		for (x = 0; x < LM_W; x++) { 
			for (y = 0; y < LM_H; y++, pPixelPos++) { 
				if (0 < SetNearestPixelLights (faceP->nSegment, pPixelPos, faceP->fRad / 10.0f, nThread)) {
					VmVecFixToFloat (&vcd.vertPos, pPixelPos);
					color.c.r = color.c.g = color.c.b = 0;
					G3AccumVertColor (-1, &color, &vcd, nThread);
					if ((color.c.r > 0.001f) || (color.c.g > 0.001f) || (color.c.b > 0.001f)) {
							bBlack = false;
						if (color.c.r >= 0.999f)
							color.c.r = 1;
						else
							bWhite = false;
						if (color.c.g >= 0.999f)
							color.c.g = 1;
						else
							bWhite = false;
						if (color.c.b >= 0.999f)
							color.c.b = 1;
						else
							bWhite = false;
						}
					//if (!(bBlack || bWhite)) 
						{
						texColorP = &texColor [y * LM_W + x];
						texColorP->red = (ubyte) (255 * color.c.r);
						texColorP->green = (ubyte) (255 * color.c.g);
						texColorP->blue = (ubyte) (255 * color.c.b);
						}
					}
				}
			}
#ifdef _DEBUG
		if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
			nDbgSeg = nDbgSeg;
#endif
		if (bBlack) {
			faceP->nLightmap = 0;
			nBlackLightmaps++;
			}
		else if (bWhite) {
			faceP->nLightmap = 1;
			nWhiteLightmaps++;
			}
		else {
			CopyLightMap (texColor, lightMapData.nLightmaps);
			faceP->nLightmap = lightMapData.nLightmaps++;
			}
		}
	}
}

//------------------------------------------------------------------------------

static tThreadInfo	ti [2];

int _CDECL_ LightMapThread (void *pThreadId)
{
	int		nId = *((int *) pThreadId);

gameData.render.lights.dynamic.shader.index [0][nId].nFirst = MAX_SHADER_LIGHTS;
gameData.render.lights.dynamic.shader.index [0][nId].nLast = 0;
ComputeLightMaps (nId ? gameData.segs.nFaces / 2 : 0, nId);
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
	ComputeLightMaps (nFace, 0);
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

char *LightMapDataFilename (char *pszFilename, int nLevel)
{
return GameDataFilename (pszFilename, "lmap", nLevel, gameOpts->render.nLightmapQuality);
}

//------------------------------------------------------------------------------

int SaveLightMapData (int nLevel)
{
	CFILE				cf;
	tLightMapDataHeader ldh = {LIGHTMAP_DATA_VERSION, 
										gameData.segs.nSegments, 
										gameData.segs.nVertices, 
										gameData.segs.nFaces, 
										lightMapData.nLights, 
										MAX_LIGHT_RANGE,
										lightMapData.nBuffers};
	int				i, bOk;
	char				szFilename [FILENAME_LEN];
	grsFace			*faceP;

if (!(gameStates.app.bCacheLightMaps && lightMapData.nLights && lightMapData.nBuffers))
	return 0;
if (!CFOpen (&cf, LightMapDataFilename (szFilename, nLevel), gameFolders.szTempDir, "wb", 0))
	return 0;
bOk = (CFWrite (&ldh, sizeof (ldh), 1, &cf) == 1);
if (bOk) {
	for (i = gameData.segs.nFaces, faceP = gameData.segs.faces.faces; i; i--, faceP++) {
		bOk = CFWrite (&faceP->nLightmap, sizeof (faceP->nLightmap), 1, &cf) == 1;
		if (!bOk)
			break;
		}
	}
if (bOk) {
	for (i = 0; i < lightMapData.nBuffers; i++) {
		bOk = CFWrite (lightMapData.buffers [i].bmP, sizeof (lightMapData.buffers [i].bmP), 1, &cf) == 1;
		if (!bOk)
			break;
		}
	}
CFClose (&cf);
return bOk;
}

//------------------------------------------------------------------------------

void ReallocLightMaps (int nBuffers)
{
if (lightMapData.nBuffers > nBuffers) {
	lightMapData.buffers = (tLightMapBuffer *) D2_REALLOC (lightMapData.buffers, nBuffers * sizeof (tLightMapBuffer));
	lightMapData.nBuffers = nBuffers;
	}
}

//------------------------------------------------------------------------------

int LoadLightMapData (int nLevel)
{
	CFILE				cf;
	tLightMapDataHeader ldh;
	int				i, bOk;
	char				szFilename [FILENAME_LEN];
	grsFace			*faceP;

if (!gameStates.app.bCacheLightMaps)
	return 0;
if (!CFOpen (&cf, LightMapDataFilename (szFilename, nLevel), gameFolders.szTempDir, "rb", 0))
	return 0;
bOk = (CFRead (&ldh, sizeof (ldh), 1, &cf) == 1);
if (bOk)
	bOk = (ldh.nVersion == LIGHTMAP_DATA_VERSION) && 
			(ldh.nSegments == gameData.segs.nSegments) && 
			(ldh.nVertices == gameData.segs.nVertices) && 
			(ldh.nFaces == gameData.segs.nFaces) && 
			(ldh.nLights == lightMapData.nLights) && 
			(ldh.nMaxLightRange == MAX_LIGHT_RANGE);
if (bOk) {
	for (i = ldh.nFaces, faceP = gameData.segs.faces.faces; i; i--, faceP++) {
		bOk = CFRead (&faceP->nLightmap, sizeof (faceP->nLightmap), 1, &cf) == 1;
		if (!bOk)
			break;
		}
	}
if (bOk) {
	for (i = 0; i < ldh.nBuffers; i++) {
		bOk = CFRead (lightMapData.buffers [i].bmP, sizeof (lightMapData.buffers [i].bmP), 1, &cf) == 1;
		if (!bOk)
			break;
		}
	}
if (bOk)
	ReallocLightMaps (ldh.nBuffers);
CFClose (&cf);
return bOk;
}

//------------------------------------------------------------------------------

void CreateLightMaps (int nLevel)
{
if (!gameStates.render.bUsePerPixelLighting)
	return;
#ifdef RELEASE
if (gameOpts->render.nLightmapQuality > 2)
	gameOpts->render.nLightmapQuality = 2;
#endif
DestroyLightMaps ();
if (!InitLightData (0))
	return;
if (LoadLightMapData (nLevel))
	return;
TransformDynLights (1, 0);
if (gameStates.render.bPerPixelLighting && gameData.segs.nFaces) {
	int nSaturation = gameOpts->render.color.nSaturation;
	gameOpts->render.color.nSaturation = 1;
	gameStates.render.bLightMaps = 1;
#if 0
	if (gameStates.app.bMultiThreaded && (gameData.segs.nSegments > 8))
		StartLightMapThreads (LightMapThread);
	else 
#endif
		{
		gameData.render.lights.dynamic.shader.index [0][0].nFirst = MAX_SHADER_LIGHTS;
		gameData.render.lights.dynamic.shader.index [0][0].nLast = 0;
		if (gameStates.app.bProgressBars && gameOpts->menus.nStyle) {
			nFace = 0;
			NMProgressBar (TXT_CALC_LIGHTMAPS, 0, PROGRESS_STEPS (gameData.segs.nFaces), CreateLightMapsPoll);
			}
		else
			ComputeLightMaps (-1, 0);
		}
	gameStates.render.bLightMaps = 0;
	gameStates.render.nState = 0;
	gameOpts->render.color.nSaturation = nSaturation;
	ReallocLightMaps ((lightMapData.nLightmaps + LIGHTMAP_BUFSIZE - 1) / LIGHTMAP_BUFSIZE);
	OglCreateLightMaps ();
	SaveLightMapData (nLevel);
	}
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
	"btmColor=texture2D(btmTex,gl_TexCoord[0].xy);" \
	"topColor=texture2D(topTex,gl_TexCoord[1].xy);" \
	"lMapColor=texture2D(lMapTex,gl_TexCoord[2].xy)+(gl_Color-0.5);" \
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
	"topColor=texture2D(topTex,gl_TexCoord[1].xy);" \
	"if(abs(topColor.a-1.0/255.0)<0.25)discard;" \
	"if((topColor.a==0.0)&&(abs(topColor.r-120.0/255.0)<8.0/255.0)&&(abs(topColor.g-88.0/255.0)<8.0/255.0)&&(abs(topColor.b-128.0/255.0)<8.0/255.0))discard;" \
	"else {btmColor=texture2D(btmTex,gl_TexCoord[0].xy);" \
	"lMapColor=texture2D(lMapTex,gl_TexCoord[2].xy)+(gl_Color-0.5);" \
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
	"bMask=texture2D(maskTex,gl_TexCoord[1].xy).a;" \
	"if(bMask<0.5)discard;" \
	"else {btmColor=texture2D(btmTex,gl_TexCoord[0].xy);" \
	"topColor=texture2D(topTex,gl_TexCoord[1].xy);" \
	"lMapColor=texture2D(lMapTex,gl_TexCoord[2].xy)+(gl_Color-0.5);" \
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
	gameStates.render.bLightMaps = 1;
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
	gameStates.render.bLightMaps = 0;
	}
}

//------------------------------------------------------------------------------

