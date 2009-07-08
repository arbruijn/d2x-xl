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

#include "descent.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "ogl_color.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "gameseg.h"
#include "wall.h"
#include "loadgamedata.h"
#include "fbuffer.h"
#include "error.h"
#include "u_mem.h"
#include "menu.h"
#include "menu.h"
#include "text.h"
#include "gamepal.h"
#include "gamemine.h"
#include "renderthreads.h"

CLightmapManager lightmapManager;

//------------------------------------------------------------------------------

#define LMAP_REND2TEX		0
#define TEXTURE_CHECK		1

#define LIGHTMAP_DATA_VERSION 22

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

tLightmapData lightmapData;

//------------------------------------------------------------------------------

int InitLightData (int bVariable);

//------------------------------------------------------------------------------

int CLightmapManager::Bind (int nLightmap)
{
	tLightmapBuffer	*lmP = &m_list.buffers [nLightmap];
#if DBG
	int					nError;
#endif

if (lmP->handle)
	return 1;
ogl.GenTextures (1, &lmP->handle);
if (!lmP->handle) {
#if 0//DBG
	nError = glGetError ();
#endif
	return 0;
	}
OGL_BINDTEX (lmP->handle); 
#if 0//DBG
if ((nError = glGetError ()))
	return 0;
#endif
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexImage2D (GL_TEXTURE_2D, 0, 3, LIGHTMAP_BUFWIDTH, LIGHTMAP_BUFWIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, lmP->bmP);
#if DBG
if ((nError = glGetError ()))
	return 0;
#endif
return 1;
}

//------------------------------------------------------------------------------

int CLightmapManager::BindAll (void)
{
for (int i = 0; i < m_list.nBuffers; i++)
	if (!Bind (i))
		return 0;
return 1;
}

//------------------------------------------------------------------------------

void CLightmapManager::Release (void)
{
if (m_list.buffers.Buffer ()) { 
	tLightmapBuffer *lmP = &m_list.buffers [0];
	for (int i = m_list.nBuffers; i; i--, lmP++)
		if (lmP->handle) {
			ogl.DeleteTextures (1, reinterpret_cast<GLuint*> (&lmP->handle));
			lmP->handle = 0;
			}
	} 
}

//------------------------------------------------------------------------------

void CLightmapManager::Destroy (void)
{
if (m_list.info.Buffer ()) { 
	Release ();
	m_list.info.Destroy ();
	m_list.buffers.Destroy ();
	m_list.nBuffers = 0;
	}
}

//------------------------------------------------------------------------------

inline void CLightmapManager::ComputePixelOffset (CFixVector& vOffs, CFixVector& v1, CFixVector& v2, int nOffset)
{
vOffs = (v2 - v1) * nOffset;
}

//------------------------------------------------------------------------------

void CLightmapManager::RestoreLights (int bVariable)
{
	CDynLight	*pl;
	int			i;

for (pl = lightManager.Lights (), i = lightManager.LightCount (0); i; i--, pl++)
	if (!(pl->info.nType || (pl->info.bVariable && !bVariable)))
		pl->info.bOn = 1;
}

//------------------------------------------------------------------------------

int CLightmapManager::CountLights (int bVariable)
{
	CDynLight		*pl;
	int				i, nLights = 0;

if (!gameStates.render.bPerPixelLighting)
	return 0;
for (pl = lightManager.Lights (), i = lightManager.LightCount (0); i; i--, pl++)
	if (!(pl->info.nType || (pl->info.bVariable && !bVariable)))
		nLights++;
return nLights; 
}

//------------------------------------------------------------------------------

double CLightmapManager::SideRad (int nSegment, int nSide)
{
	int			i;
	double		h, xMin, xMax, yMin, yMax, zMin, zMax;
	double		dx, dy, dz;
	short*		sideVerts;
	CFixVector	*v;

sideVerts = SEGMENTS [nSegment].Corners (nSide);
xMin = yMin = zMin = 1e300;
xMax = yMax = zMax = -1e300;
for (i = 0; i < 4; i++) {
	v = gameData.segs.vertices +sideVerts [i];
	h = (*v)[X];
	if (xMin > h)
		xMin = h;
	if (xMax < h)
		xMax = h;
	h = (*v)[Y];
	if (yMin > h)
		yMin = h;
	if (yMax < h)
		yMax = h;
	h = (*v)[Z];
	if (zMin > h)
		zMin = h;
	if (zMax < h)
		zMax = h;
	}
dx = xMax - xMin;
dy = yMax - yMin;
dz = zMax - zMin;
return sqrt (dx * dx + dy * dy + dz * dz) / (2 * (double) I2X (1));
}

//------------------------------------------------------------------------------

int CLightmapManager::Init (int bVariable)
{
	CDynLight		*pl;
	CSegFace			*faceP = NULL;
	int				bIsLight, nIndex, i; 
	short				t; 
	tLightmapInfo	*lmiP;  //temporary place to put light data.
	double			sideRad;

//first step find all the lights in the level.  By iterating through every surface in the level.
if (!(m_list.nLights = CountLights (bVariable))) {
	if (!m_list.buffers.Create (m_list.nBuffers = 1))
		return -1;
	m_list.buffers.Clear ();
	return 0;
	}
if (!m_list.info.Create (m_list.nLights)) {
	m_list.nLights = 0; 
	return -1;
	}
m_list.nBuffers = (gameData.segs.nFaces + LIGHTMAP_BUFSIZE - 1) / LIGHTMAP_BUFSIZE;
if (!m_list.buffers.Create (m_list.nBuffers)) {
	m_list.info.Destroy ();
	m_list.nLights = 0; 
	return -1;
	}
m_list.buffers.Clear (); 
m_list.info.Clear (); 
m_list.nLights = 0; 
//first lightmap is dummy lightmap for multi pass lighting
lmiP = m_list.info.Buffer (); 
for (pl = lightManager.Lights (), i = lightManager.LightCount (0); i; i--, pl++) {
	if (pl->info.nType || (pl->info.bVariable && !bVariable))
		continue;
	if (faceP == pl->info.faceP)
		continue;
	faceP = pl->info.faceP;
	bIsLight = 0; 
	t = IsLight (faceP->nBaseTex) ? faceP->nBaseTex : faceP->nOvlTex;
	sideRad = (double) faceP->fRads [1] / 10.0;
	nIndex = faceP->nSegment * 6 + faceP->nSide;
	//Process found light.
	lmiP->range += sideRad;
	//find where it is in the level.
	lmiP->vPos = SEGMENTS [faceP->nSegment].SideCenter (faceP->nSide);
	lmiP->nIndex = nIndex; 
	//find light direction, currently based on first 3 points of CSide, not always right.
	CFixVector *normalP = SEGMENTS [faceP->nSegment].m_sides [faceP->nSide].m_normals;
	lmiP->vDir = CFixVector::Avg(normalP[0], normalP[1]);
	lmiP++; 
	}
return m_list.nLights = (int) (lmiP - m_list.info.Buffer ()); 
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

void CLightmapManager::Copy (tRgbColorb *texColorP, ushort nLightmap)
{
tLightmapBuffer *bufP = &m_list.buffers [nLightmap / LIGHTMAP_BUFSIZE];
int i = nLightmap % LIGHTMAP_BUFSIZE;
int x = (i % LIGHTMAP_ROWSIZE) * LM_W;
int y = (i / LIGHTMAP_ROWSIZE) * LM_H;
for (i = 0; i < LM_H; i++, y++, texColorP += LM_W)
	memcpy (&bufP->bmP [y][x], texColorP, LM_W * sizeof (tRgbColorb));
}

//------------------------------------------------------------------------------

void CLightmapManager::CreateSpecial (tRgbColorb *texColorP, ushort nLightmap, ubyte nColor)
{
memset (texColorP, nColor, LM_W * LM_H * sizeof (tRgbColorb));
Copy (texColorP, nLightmap);
}

//------------------------------------------------------------------------------
// build one entire lightmap in single threaded mode or the upper and lower halves when multi threaded

void CLightmapManager::Build (int nThread)
{
	CFixVector		*pixelPosP;
	tRgbColorb		*texColorP;
	CFloatVector3	color;
	CFixVector		v0, v1, v2;
	struct {
		CFixVector	x, y;
	} offset;
	int				w, h, x, y, yMin, yMax;
	int				i0, i1, i2; 
	bool				bBlack, bWhite;
	CVertColorData	vcd = m_data.vcd;


#if 0// DBG
if ((m_data.faceP->nSegment != nDbgSeg) && ((nDbgSide < 0) || (m_data.faceP->nSide != nDbgSide))) {
	m_data.nColor |= 1;
	return;
	}
#endif

h = LM_H;
w = LM_W;

if (nThread < 0) {
	yMin = 0;
	yMax = h;
	nThread = 0;
	}
else if (nThread > 0) {
	yMin = h / 2;
	yMax = h;
	}
else {
	yMin = 0;
	yMax = h / 2;
	}

#if DBG
if ((m_data.faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (m_data.faceP->nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
vcd.vertPosP = &vcd.vertPos;
pixelPosP = m_data.pixelPos + yMin * w;
if (m_data.nType) {
	i1 = m_data.sideVerts [0]; 
	v1 = VERTICES [i1];
	i2 = m_data.sideVerts [2]; 
	v2 = VERTICES [i2];
	for (y = yMin; y < yMax; y++) {
		for (x = 0; x < w; x++, pixelPosP++) {
			if (y >= x) {
				i0 = m_data.sideVerts [1]; 
				v0 = VERTICES [i0];
				offset.x = (v0 - v1) * m_data.nOffset [y];
				offset.y = (v2 - v0) * m_data.nOffset [x];
				}
			else {
				i0 = m_data.sideVerts [3]; 
				v0 = VERTICES [i0];
				offset.y = (v0 - v1) * m_data.nOffset [x]; 
				offset.x = (v2 - v0) * m_data.nOffset [y]; 
				}
			*pixelPosP = v1 + offset.x + offset.y; 
			}
		}
	}
else {//SIDE_IS_TRI_02
	h--;
	i1 = m_data.sideVerts [1]; 
	v1 = VERTICES [i1];
	i2 = m_data.sideVerts [3]; 
	v2 = VERTICES [i2];
	for (y = yMin; y < yMax; y++) {
		for (x = 0; x < w; x++, pixelPosP++) {
			if (h - y >= x) {
				i0 = m_data.sideVerts [0]; 
				v0 = VERTICES [i0];
				offset.x = (v1 - v0) * m_data.nOffset [y];  
				offset.y = (v2 - v0) * m_data.nOffset [x];
				}
			else {
				i0 = m_data.sideVerts [2]; 
				v0 = VERTICES [i0];
				offset.y = (v1 - v0) * m_data.nOffset [h - x];  
				offset.x = (v2 - v0) * m_data.nOffset [h - y]; 
				}
			*pixelPosP = v0 + offset.x + offset.y; 
			}
		}
	}

bBlack = bWhite = true;
pixelPosP = m_data.pixelPos + yMin * w;
for (y = yMin; y < yMax; y++) {
	for (x = 0; x < w; x++, pixelPosP++) { 
#if DBG
		if ((m_data.faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (m_data.faceP->nSide == nDbgSide))) {
			nDbgSeg = nDbgSeg;
			if (((x == 0) || (x == w - 1)) || ((y == 0) || (y == w - 1)))
				nDbgSeg = nDbgSeg;
			if (x == 0)
				nDbgSeg = nDbgSeg;
			if (x == w - 1)
				nDbgSeg = nDbgSeg;
			if (y == 0)
				nDbgSeg = nDbgSeg;
			if (y == w - 1)
				nDbgSeg = nDbgSeg;
			}
#endif
		if (0 < lightManager.SetNearestToPixel (m_data.faceP->nSegment, m_data.faceP->nSide, &m_data.vNormal, 
															 pixelPosP, m_data.faceP->fRads [1] / 10.0f, nThread)) {
			vcd.vertPos.Assign (*pixelPosP);
			color.SetZero ();
			G3AccumVertColor (-1, &color, &vcd, nThread);
			if ((color [R] > 0.001f) || (color [G] > 0.001f) || (color [B] > 0.001f)) {
					bBlack = false;
				if (color [R] >= 0.999f)
					color [R] = 1;
				else
					bWhite = false;
				if (color [G] >= 0.999f)
					color [G] = 1;
				else
					bWhite = false;
				if (color [B] >= 0.999f)
					color [B] = 1;
				else
					bWhite = false;
				}
			texColorP = m_data.texColor + x * w + y;
			texColorP->red = (ubyte) (255 * color [R]);
			texColorP->green = (ubyte) (255 * color [G]);
			texColorP->blue = (ubyte) (255 * color [B]);
			}
		}
	}
if (bBlack)
	m_data.nColor |= 1;
else if (bWhite)
	m_data.nColor |= 2;
else 
	m_data.nColor |= 4;
}

//------------------------------------------------------------------------------

void CLightmapManager::BuildAll (int nFace)
{
	CSide*	sideP; 
	int		nLastFace; 
	int		i; 
	float		h;
	int		nBlackLightmaps = 0, nWhiteLightmaps = 0; 

gameStates.render.nState = 0;
h = 1.0f / float (LM_W - 1);
for (i = 0; i < LM_W; i++)
	m_data.nOffset [i] = F2X (i * h);
InitVertColorData (m_data.vcd);
m_data.vcd.vertPosP = &m_data.vcd.vertPos;
m_data.vcd.fMatShininess = 4;

#if 0
if (gameStates.app.bMultiThreaded)
	nLastFace = nFace ? gameData.segs.nFaces : gameData.segs.nFaces / 2;
else
#endif
	INIT_PROGRESS_LOOP (nFace, nLastFace, gameData.segs.nFaces);
if (nFace <= 0) {
	CreateSpecial (m_data.texColor, 0, 0);
	CreateSpecial (m_data.texColor, 1, 255);
	m_list.nLightmaps = 2;
	}
//Next Go through each surface and create a lightmap for it.
for (m_data.faceP = FACES.faces + nFace; nFace < nLastFace; nFace++, m_data.faceP++) {
#if DBG
	if ((m_data.faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (m_data.faceP->nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	if (SEGMENTS [m_data.faceP->nSegment].m_nType == SEGMENT_IS_SKYBOX)
		continue;
	sideP = SEGMENTS [m_data.faceP->nSegment].m_sides + m_data.faceP->nSide;
	memcpy (m_data.sideVerts, m_data.faceP->index, sizeof (m_data.sideVerts));
	m_data.nType = (sideP->m_nType == SIDE_IS_QUAD) || (sideP->m_nType == SIDE_IS_TRI_02);
	m_data.vNormal = CFixVector::Avg (sideP->m_normals [0], sideP->m_normals [1]);
	CFixVector::Normalize (m_data.vNormal);
	m_data.vcd.vertNorm.Assign (m_data.vNormal);
	CFloatVector3::Normalize (m_data.vcd.vertNorm);
	m_data.nColor = 0;
	memset (m_data.texColor, 0, LM_W * LM_H * sizeof (tRgbColorb));
	if (!RunRenderThreads (rtLightmap))
		Build (-1);
#if DBG
	if ((m_data.faceP->nSegment == nDbgSeg) && ((nDbgSide < 0) || (m_data.faceP->nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	if (m_data.nColor == 1) {
		m_data.faceP->nLightmap = 0;
		nBlackLightmaps++;
		}
	else if (m_data.nColor == 2) {
		m_data.faceP->nLightmap = 1;
		nWhiteLightmaps++;
		}
	else {
		Copy (m_data.texColor, m_list.nLightmaps);
		m_data.faceP->nLightmap = m_list.nLightmaps++;
		}
	}
}

//------------------------------------------------------------------------------

static int nFace = 0;

static int CreatePoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

paletteManager.LoadEffect ();
if (nFace < gameData.segs.nFaces) {
	lightmapManager.BuildAll (nFace);
	nFace += PROGRESS_INCR;
	}
else {
	key = -2;
	paletteManager.LoadEffect ();
	return nCurItem;
	}
menu [0].m_value++;
menu [0].m_bRebuild = 1;
key = 0;
paletteManager.LoadEffect ();
return nCurItem;
}

//------------------------------------------------------------------------------

char *CLightmapManager::Filename (char *pszFilename, int nLevel)
{
return GameDataFilename (pszFilename, "lmap", nLevel, gameOpts->render.nLightmapQuality);
}

//------------------------------------------------------------------------------

void CLightmapManager::Realloc (int nBuffers)
{
if (m_list.nBuffers > nBuffers) {
	m_list.buffers.Resize (nBuffers);
	m_list.nBuffers = nBuffers;
	}
}

//------------------------------------------------------------------------------

int CLightmapManager::Save (int nLevel)
{
	CFile				cf;
	tLightmapDataHeader ldh = {LIGHTMAP_DATA_VERSION, 
										gameData.segs.nSegments, 
										gameData.segs.nVertices, 
										gameData.segs.nFaces, 
										m_list.nLights, 
										MAX_LIGHT_RANGE,
										m_list.nBuffers};
	int				i, bOk;
	char				szFilename [FILENAME_LEN];
	CSegFace			*faceP;

if (!(gameStates.app.bCacheLightmaps && m_list.nLights && m_list.nBuffers))
	return 0;
if (!cf.Open (Filename (szFilename, nLevel), gameFolders.szCacheDir, "wb", 0))
	return 0;
bOk = (cf.Write (&ldh, sizeof (ldh), 1) == 1);
if (bOk) {
	for (i = gameData.segs.nFaces, faceP = FACES.faces.Buffer (); i; i--, faceP++) {
		bOk = cf.Write (&faceP->nLightmap, sizeof (faceP->nLightmap), 1) == 1;
		if (!bOk)
			break;
		}
	}
if (bOk) {
	for (i = 0; i < m_list.nBuffers; i++) {
		bOk = cf.Write (m_list.buffers [i].bmP, sizeof (m_list.buffers [i].bmP), 1) == 1;
		if (!bOk)
			break;
		}
	}
cf.Close ();
return bOk;
}

//------------------------------------------------------------------------------

int CLightmapManager::Load (int nLevel)
{
	CFile				cf;
	tLightmapDataHeader ldh;
	int				i, bOk;
	char				szFilename [FILENAME_LEN];
	CSegFace			*faceP;

if (!(gameStates.app.bCacheLightmaps))
	return 0;
if (!cf.Open (Filename (szFilename, nLevel), gameFolders.szCacheDir, "rb", 0))
	return 0;
bOk = (cf.Read (&ldh, sizeof (ldh), 1) == 1);
if (bOk)
	bOk = (ldh.nVersion == LIGHTMAP_DATA_VERSION) && 
			(ldh.nSegments == gameData.segs.nSegments) && 
			(ldh.nVertices == gameData.segs.nVertices) && 
			(ldh.nFaces == gameData.segs.nFaces) && 
			(ldh.nLights == m_list.nLights) && 
			(ldh.nMaxLightRange == MAX_LIGHT_RANGE);
if (bOk) {
	for (i = ldh.nFaces, faceP = FACES.faces.Buffer (); i; i--, faceP++) {
		bOk = cf.Read (&faceP->nLightmap, sizeof (faceP->nLightmap), 1) == 1;
		if (!bOk)
			break;
		}
	}
if (bOk) {
	for (i = 0; i < ldh.nBuffers; i++) {
		bOk = cf.Read (m_list.buffers [i].bmP, sizeof (m_list.buffers [i].bmP), 1) == 1;
		if (!bOk)
			break;
		}
	}
if (bOk)
	Realloc (ldh.nBuffers);
cf.Close ();
return bOk;
}

//------------------------------------------------------------------------------

void CLightmapManager::Init (void)
{
m_list.nBuffers = 
m_list.nLights = 0;
m_list.nLightmaps = 0;
memset (&m_data, 0, sizeof (m_data)); 
}

//------------------------------------------------------------------------------

int CLightmapManager::Create (int nLevel)
{
if (!(gameStates.render.bUsePerPixelLighting))
	return 0;
if ((gameStates.render.bUsePerPixelLighting == 1) && !CreateLightmapShader (0))
	return gameStates.render.bUsePerPixelLighting = 0;
#if !DBG
if (gameOpts->render.nLightmapQuality > 3)
	gameOpts->render.nLightmapQuality = 3;
#endif
Destroy ();
int nLights = Init (0);
if (nLights < 0)
	return 0;
if (Load (nLevel))
	return 1;
if (gameStates.render.bPerPixelLighting && gameData.segs.nFaces) {
	if (nLights) {
		lightManager.Transform (1, 0);
		int nSaturation = gameOpts->render.color.nSaturation;
		gameOpts->render.color.nSaturation = 1;
		gameStates.render.bHaveLightmaps = 1;
		//gameData.render.fAttScale [0] = 2.0f;
		lightManager.Index (0)[0].nFirst = MAX_SHADER_LIGHTS;
		lightManager.Index (0)[0].nLast = 0;
		if (gameStates.app.bProgressBars && gameOpts->menus.nStyle) {
			nFace = 0;
			ProgressBar (TXT_CALC_LIGHTMAPS, 0, PROGRESS_STEPS (gameData.segs.nFaces), CreatePoll);
			}
		else {
			messageBox.Show (TXT_CALC_LIGHTMAPS);
			BuildAll (-1);
			messageBox.Clear ();
			}
		//gameData.render.fAttScale [0] = (gameStates.render.bPerPixelLighting == 2) ? 1.0f : 2.0f;
		gameStates.render.bHaveLightmaps = 0;
		gameStates.render.nState = 0;
		gameOpts->render.color.nSaturation = nSaturation;
		Realloc ((m_list.nLightmaps + LIGHTMAP_BUFSIZE - 1) / LIGHTMAP_BUFSIZE);
		}
	else {
		CreateSpecial (m_data.texColor, 0, 0);
		m_list.nLightmaps = 1;
		for (int i = 0; i < gameData.segs.nFaces; i++)
			FACES.faces [i].nLightmap = 0;
		}
	BindAll ();
	Save (nLevel);
	}
return 1;
}

//------------------------------------------------------------------------------

