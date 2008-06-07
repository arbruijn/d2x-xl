/*
Here is all the code for gameOpts->render.color.bUseLightmaps.  Some parts need 
to be optamized but the core functionality is there.
The thing you will need to add is the menu code to change
the value of int gameOpts->render.color.bUseLightmaps.  THIS CANNOT BE CHANGED IF
A GAME IS RUNNING.  It would likely cause it to crash.

Almost forgot there are a few lines that read exit (0) after 
checking certain capabilties.  This should really just 
disable the gameOpts->render.color.bUseLightmaps option, because the person's computer 
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

#define LIGHTMAP_DATA_VERSION 9

#define LM_W	LIGHTMAP_WIDTH
#define LM_H	LIGHTMAP_WIDTH

//------------------------------------------------------------------------------

typedef struct tLightmapDataHeader {
	int	nVersion;
	int	nSegments;
	int	nVertices;
	int	nFaces;
	int	nLights;
	int	nMaxLightRange;
	int	nBuffers;
	} tLightmapDataHeader;

//------------------------------------------------------------------------------

GLhandleARB lmShaderProgs [3] = {0,0,0}; 
GLhandleARB lmFS [3] = {0,0,0}; 
GLhandleARB lmVS [3] = {0,0,0}; 

int lightmapWidth [5] = {8, 16, 32, 64, 128};

tLightmap dummyLightmap;

tLightmapData lightmapData = {NULL, NULL, 0, 0};

//------------------------------------------------------------------------------

int InitLightData (int bVariable);

//------------------------------------------------------------------------------

int OglCreateLightmap (int nLightmap)
{
	tLightmapBuffer	*lmP = lightmapData.buffers + nLightmap;
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

int OglCreateLightmaps (void)
{
for (int i = 0; i < lightmapData.nBuffers; i++)
	if (!OglCreateLightmap (i))
		return 0;
return 1;
}

//------------------------------------------------------------------------------

void OglDestroyLightmaps (void)
{
if (lightmapData.buffers) { 
	tLightmapBuffer *lmP = lightmapData.buffers;
	for (int i = lightmapData.nBuffers; i; i--, lmP++)
		if (lmP->handle) {
			OglDeleteTextures (1, (GLuint *) &lmP->handle);
			lmP->handle = 0;
			}
	} 
}

//------------------------------------------------------------------------------

void DestroyLightmaps (void)
{
if (lightmapData.info) { 
	OglDestroyLightmaps ();
	D2_FREE (lightmapData.info);
	D2_FREE (lightmapData.buffers);
	lightmapData.nBuffers = 0;
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

if (!gameStates.render.bPerPixelLighting)
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

int InitLightData (int bVariable)
{
	tDynLight		*pl;
	grsFace			*faceP = NULL;
	int				bIsLight, nIndex, i; 
	short				t; 
	tLightmapInfo	*lmiP;  //temporary place to put light data.
	double			sideRad;

//first step find all the lights in the level.  By iterating through every surface in the level.
if (!(lightmapData.nLights = CountLights (bVariable)))
	return 0;
if (!(lightmapData.info = (tLightmapInfo *) D2_ALLOC (sizeof (tLightmapInfo) * lightmapData.nLights)))
	return lightmapData.nLights = 0; 
lightmapData.nBuffers = (gameData.segs.nFaces + LIGHTMAP_BUFSIZE - 1) / LIGHTMAP_BUFSIZE;
if (!(lightmapData.buffers = (tLightmapBuffer *) D2_ALLOC (lightmapData.nBuffers * sizeof (tLightmapBuffer)))) {
	D2_FREE (lightmapData.info);
	return lightmapData.nLights = 0; 
	}
memset (lightmapData.buffers, 0, lightmapData.nBuffers * sizeof (tLightmapBuffer)); 
memset (lightmapData.info, 0, sizeof (tLightmapInfo) * lightmapData.nLights); 
lightmapData.nLights = 0; 
//first lightmap is dummy lightmap for multi pass lighting
lmiP = lightmapData.info; 
for (pl = gameData.render.lights.dynamic.lights, i = gameData.render.lights.dynamic.nLights; i; i--, pl++) {
	if (pl->info.nType || (pl->info.bVariable && !bVariable))
		continue;
	if (faceP == pl->info.faceP)
		continue;
	faceP = pl->info.faceP;
	bIsLight = 0; 
	t = IsLight (faceP->nBaseTex) ? faceP->nBaseTex : faceP->nOvlTex;
	sideRad = (double) faceP->fRad / 10.0;
	nIndex = faceP->nSegment * 6 + faceP->nSide;
	//Process found light.
	lmiP->range += sideRad;
	//find where it is in the level.
	lmiP->vPos = SIDE_CENTER_V (faceP->nSegment, faceP->nSide);
	lmiP->nIndex = nIndex; 
	//find light direction, currently based on first 3 points of tSide, not always right.
	vmsVector *normalP = SEGMENTS [faceP->nSegment].sides [faceP->nSide].normals;
	VmVecAvg (&lmiP->vDir, normalP, normalP + 1);
	lmiP++; 
	}
return lightmapData.nLights = (int) (lmiP - lightmapData.info); 
}

//------------------------------------------------------------------------------

#if LMAP_REND2TEX

// create 512 brightness values using inverse square
void InitBrightmap (ubyte *brightmap)
{
	int		i;
	double	h;

for (i = 512; i; i--, brightmap++) {
	h = (double) i / 512.0;
	h *= h;
	*brightmap = (ubyte) ((255 * h) + 0.5);
	}
}

//------------------------------------------------------------------------------

// create a color/brightness table
void InitLightmapInfo (ubyte *lightmap, ubyte *brightmap, GLfloat *color)
{
	int		i, j;
	double	h;

for (i = 512; i; i--, brightmap++)
	for (j = 0; j < 3; j++, lightmap++)
		*lightmap = (ubyte) (*brightmap * color [j] + 0.5);
}

#endif //LMAP_REND2TEX

//------------------------------------------------------------------------------

void CopyLightmap (tRgbColorb *texColorP, ushort nLightmap)
{
tLightmapBuffer *bufP = lightmapData.buffers + nLightmap / LIGHTMAP_BUFSIZE;
int i = nLightmap % LIGHTMAP_BUFSIZE;
int x = (i % LIGHTMAP_ROWSIZE) * LM_W;
int y = (i / LIGHTMAP_ROWSIZE) * LM_H;
for (i = 0; i < LM_H; i++, y++, texColorP += LM_W)
	memcpy (&bufP->bmP [y][x], texColorP, LM_W * sizeof (tRgbColorb));
}

//------------------------------------------------------------------------------

void CreateSpecialLightmap (tRgbColorb *texColorP, ushort nLightmap, ubyte nColor)
{
memset (texColorP, nColor, LM_W * LM_H * sizeof (tRgbColorb));
CopyLightmap (texColorP, nLightmap);
}

//------------------------------------------------------------------------------

void ComputeLightmaps (int nFace, int nThread)
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

#if 0
if (gameStates.app.bMultiThreaded)
	nLastFace = nFace ? gameData.segs.nFaces : gameData.segs.nFaces / 2;
else
#endif
	INIT_PROGRESS_LOOP (nFace, nLastFace, gameData.segs.nFaces);
if (nFace <= 0) {
	CreateSpecialLightmap (texColor, 0, 0);
	CreateSpecialLightmap (texColor, 1, 255);
	lightmapData.nLightmaps = 2;
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
#ifdef _DEBUG
				if ((faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->nSide == nDbgSide)))
					nDbgSeg = nDbgSeg;
#endif
				if (0 < SetNearestPixelLights (faceP->nSegment, faceP->nSide, &vNormal, pPixelPos, faceP->fRad / 10.0f, nThread)) {
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
#ifdef _DEBUG
				else
					nDbgSeg = nDbgSeg;
#endif
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
			CopyLightmap (texColor, lightmapData.nLightmaps);
			faceP->nLightmap = lightmapData.nLightmaps++;
			}
		}
	}
}

//------------------------------------------------------------------------------

static tThreadInfo	ti [2];

int _CDECL_ LightmapThread (void *pThreadId)
{
	int		nId = *((int *) pThreadId);

gameData.render.lights.dynamic.shader.index [0][nId].nFirst = MAX_SHADER_LIGHTS;
gameData.render.lights.dynamic.shader.index [0][nId].nLast = 0;
ComputeLightmaps (nId ? gameData.segs.nFaces / 2 : 0, nId);
SDL_SemPost (ti [nId].done);
ti [nId].bDone = 1;
return 0;
}

//------------------------------------------------------------------------------

static void StartLightmapThreads (pThreadFunc pFunc)
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

static void CreateLightmapsPoll (int nItems, tMenuItem *m, int *key, int cItem)
{
GrPaletteStepLoad (NULL);
if (nFace < gameData.segs.nFaces) {
	ComputeLightmaps (nFace, 0);
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

char *LightmapDataFilename (char *pszFilename, int nLevel)
{
return GameDataFilename (pszFilename, "lmap", nLevel, gameOpts->render.nLightmapQuality);
}

//------------------------------------------------------------------------------

int SaveLightmapData (int nLevel)
{
	CFILE				cf;
	tLightmapDataHeader ldh = {LIGHTMAP_DATA_VERSION, 
										gameData.segs.nSegments, 
										gameData.segs.nVertices, 
										gameData.segs.nFaces, 
										lightmapData.nLights, 
										MAX_LIGHT_RANGE,
										lightmapData.nBuffers};
	int				i, bOk;
	char				szFilename [FILENAME_LEN];
	grsFace			*faceP;

if (!(gameStates.app.bCacheLightmaps && lightmapData.nLights && lightmapData.nBuffers))
	return 0;
if (!CFOpen (&cf, LightmapDataFilename (szFilename, nLevel), gameFolders.szTempDir, "wb", 0))
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
	for (i = 0; i < lightmapData.nBuffers; i++) {
		bOk = CFWrite (lightmapData.buffers [i].bmP, sizeof (lightmapData.buffers [i].bmP), 1, &cf) == 1;
		if (!bOk)
			break;
		}
	}
CFClose (&cf);
return bOk;
}

//------------------------------------------------------------------------------

void ReallocLightmaps (int nBuffers)
{
if (lightmapData.nBuffers > nBuffers) {
	lightmapData.buffers = (tLightmapBuffer *) D2_REALLOC (lightmapData.buffers, nBuffers * sizeof (tLightmapBuffer));
	lightmapData.nBuffers = nBuffers;
	}
}

//------------------------------------------------------------------------------

int LoadLightmapData (int nLevel)
{
	CFILE				cf;
	tLightmapDataHeader ldh;
	int				i, bOk;
	char				szFilename [FILENAME_LEN];
	grsFace			*faceP;

if (!gameStates.app.bCacheLightmaps)
	return 0;
if (!CFOpen (&cf, LightmapDataFilename (szFilename, nLevel), gameFolders.szTempDir, "rb", 0))
	return 0;
bOk = (CFRead (&ldh, sizeof (ldh), 1, &cf) == 1);
if (bOk)
	bOk = (ldh.nVersion == LIGHTMAP_DATA_VERSION) && 
			(ldh.nSegments == gameData.segs.nSegments) && 
			(ldh.nVertices == gameData.segs.nVertices) && 
			(ldh.nFaces == gameData.segs.nFaces) && 
			(ldh.nLights == lightmapData.nLights) && 
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
		bOk = CFRead (lightmapData.buffers [i].bmP, sizeof (lightmapData.buffers [i].bmP), 1, &cf) == 1;
		if (!bOk)
			break;
		}
	}
if (bOk)
	ReallocLightmaps (ldh.nBuffers);
CFClose (&cf);
return bOk;
}

//------------------------------------------------------------------------------

int CreateLightmaps (int nLevel)
{
if (!gameStates.render.bUsePerPixelLighting)
	return 0;
if ((gameStates.render.bUsePerPixelLighting == 1) && !InitLightmapShader (0))
	return gameStates.render.bUsePerPixelLighting = 0;
#ifdef RELEASE
if (gameOpts->render.nLightmapQuality > 2)
	gameOpts->render.nLightmapQuality = 2;
#endif
DestroyLightmaps ();
if (!InitLightData (0))
	return 0;
if (LoadLightmapData (nLevel))
	return 1;
TransformDynLights (1, 0);
if (gameStates.render.bPerPixelLighting && gameData.segs.nFaces) {
	int nSaturation = gameOpts->render.color.nSaturation;
	gameOpts->render.color.nSaturation = 1;
	gameStates.render.bLightmaps = 1;
#if 0
	if (gameStates.app.bMultiThreaded && (gameData.segs.nSegments > 8))
		StartLightmapThreads (LightmapThread);
	else 
#endif
		{
		gameData.render.lights.dynamic.shader.index [0][0].nFirst = MAX_SHADER_LIGHTS;
		gameData.render.lights.dynamic.shader.index [0][0].nLast = 0;
		if (gameStates.app.bProgressBars && gameOpts->menus.nStyle) {
			nFace = 0;
			NMProgressBar (TXT_CALC_LIGHTMAPS, 0, PROGRESS_STEPS (gameData.segs.nFaces), CreateLightmapsPoll);
			}
		else
			ComputeLightmaps (-1, 0);
		}
	gameStates.render.bLightmaps = 0;
	gameStates.render.nState = 0;
	gameOpts->render.color.nSaturation = nSaturation;
	ReallocLightmaps ((lightmapData.nLightmaps + LIGHTMAP_BUFSIZE - 1) / LIGHTMAP_BUFSIZE);
	OglCreateLightmaps ();
	SaveLightmapData (nLevel);
	}
return 1;
}

//------------------------------------------------------------------------------

