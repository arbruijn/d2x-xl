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
#include "segmath.h"
#include "wall.h"
#include "loadgamedata.h"
#include "fbuffer.h"
#include "error.h"
#include "u_mem.h"
#include "menu.h"
#include "menu.h"
#include "text.h"
#include "netmisc.h"
#include "gamepal.h"
#include "loadgeometry.h"
#include "renderthreads.h"
#include "createmesh.h"

CLightmapManager lightmapManager;

//------------------------------------------------------------------------------

#define LMAP_REND2TEX		0
#define TEXTURE_CHECK		1

#define LIGHTMAP_DATA_VERSION 31
#define LM_W	LIGHTMAP_WIDTH
#define LM_H	LIGHTMAP_WIDTH

//------------------------------------------------------------------------------

typedef struct tLightmapDataHeader {
	int	nVersion;
	int	nCheckSum;
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

#if DBG
int lightmapWidth [5] = {8, 16, 32, 64, 128};
#else
int lightmapWidth [5] = {16, 32, 64, 128, 256};
#endif

tLightmap dummyLightmap;

//------------------------------------------------------------------------------

int InitLightData (int bVariable);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CLightmapFaceData::Setup (CSegFace* faceP)
{
CSide* sideP = SEGMENTS [faceP->m_info.nSegment].m_sides + faceP->m_info.nSide;
m_nType = (sideP->m_nType == SIDE_IS_QUAD) || (sideP->m_nType == SIDE_IS_TRI_02);
m_vNormal = sideP->m_normals [2];
CFixVector::Normalize (m_vNormal);
m_vcd.vertNorm.Assign (m_vNormal);
CFloatVector3::Normalize (m_vcd.vertNorm);
InitVertColorData (m_vcd);
m_vcd.vertPosP = &m_vcd.vertPos;
m_vcd.fMatShininess = 4;
memcpy (m_sideVerts, faceP->m_info.index, sizeof (m_sideVerts));
m_nColor = 0;
memset (m_texColor, 0, LM_W * LM_H * sizeof (CRGBColor));
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
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
ogl.BindTexture (lmP->handle); 
#if 0//DBG
if ((nError = glGetError ()))
	return 0;
#endif
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
	CDynLight*	lightP = lightManager.Lights ();

for (int i = lightManager.LightCount (0); i; i--, lightP++)
	if (!(lightP->info.nType || (lightP->info.bVariable && !bVariable)))
		lightP->info.bOn = 1;
}

//------------------------------------------------------------------------------

int CLightmapManager::CountLights (int bVariable)
{
	CDynLight*	lightP = lightManager.Lights ();
	int			nLights = 0;

if (!gameStates.render.bPerPixelLighting)
	return 0;
for (int i = lightManager.LightCount (0); i; i--, lightP++)
	if (!(lightP->info.nType || (lightP->info.bVariable && !bVariable)))
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
	h = (*v).v.coord.x;
	if (xMin > h)
		xMin = h;
	if (xMax < h)
		xMax = h;
	h = (*v).v.coord.y;
	if (yMin > h)
		yMin = h;
	if (yMax < h)
		yMax = h;
	h = (*v).v.coord.z;
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
	CDynLight*		lightP;
	CSegFace*		faceP = NULL;
	int				bIsLight, nIndex, i; 
	short				t; 
	tLightmapInfo*	lightmapInfoP;  //temporary place to put light data.
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
lightmapInfoP = m_list.info.Buffer (); 
for (lightP = lightManager.Lights (), i = lightManager.LightCount (0); i; i--, lightP++) {
	if (lightP->info.nType || (lightP->info.bVariable && !bVariable))
		continue;
	if (faceP == lightP->info.faceP)
		continue;
	faceP = lightP->info.faceP;
	bIsLight = 0; 
	t = IsLight (faceP->m_info.nBaseTex) ? faceP->m_info.nBaseTex : faceP->m_info.nOvlTex;
	sideRad = (double) faceP->m_info.fRads [1] / 10.0;
	nIndex = faceP->m_info.nSegment * 6 + faceP->m_info.nSide;
	//Process found light.
	lightmapInfoP->range += sideRad;
	//find where it is in the level.
	lightmapInfoP->vPos = SEGMENTS [faceP->m_info.nSegment].SideCenter (faceP->m_info.nSide);
	lightmapInfoP->nIndex = nIndex; 
	//find light direction, currently based on first 3 points of CSide, not always right.
	CFixVector *normalP = SEGMENTS [faceP->m_info.nSegment].m_sides [faceP->m_info.nSide].m_normals;
	lightmapInfoP->vDir = CFixVector::Avg(normalP[0], normalP[1]);
	lightmapInfoP++; 
	}
return m_list.nLights = (int) (lightmapInfoP - m_list.info.Buffer ()); 
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

void CLightmapManager::Copy (CRGBColor *texColorP, ushort nLightmap)
{
tLightmapBuffer *bufP = &m_list.buffers [nLightmap / LIGHTMAP_BUFSIZE];
int i = nLightmap % LIGHTMAP_BUFSIZE;
int x = (i % LIGHTMAP_ROWSIZE) * LM_W;
int y = (i / LIGHTMAP_ROWSIZE) * LM_H;
for (i = 0; i < LM_H; i++, y++, texColorP += LM_W)
	memcpy (&bufP->bmP [y][x], texColorP, LM_W * sizeof (CRGBColor));
}

//------------------------------------------------------------------------------

void CLightmapManager::CreateSpecial (CRGBColor *texColorP, ushort nLightmap, ubyte nColor)
{
memset (texColorP, nColor, LM_W * LM_H * sizeof (CRGBColor));
Copy (texColorP, nLightmap);
}

//------------------------------------------------------------------------------

bool CLightmapManager::FaceIsInvisible (CSegFace* faceP)
{
if (SEGMENTS [faceP->m_info.nSegment].m_function == SEGMENT_FUNC_SKYBOX) {
	faceP->m_info.nLightmap = 1;
	return true;
	}
if (SEGMENTS [faceP->m_info.nSegment].m_children [faceP->m_info.nSide] < 0) {
	faceP->m_info.nLightmap = 0;
	return true;
	}
CWall* wallP = SEGMENTS [faceP->m_info.nSegment].m_sides [faceP->m_info.nSide].Wall ();
if (!wallP || (wallP->nType != WALL_OPEN)) 
	return false;

int nTrigger = 0; 
for (;;) {
	if (0 > (nTrigger = wallP->IsTriggerTarget (nTrigger)))
		return false;
	if (TRIGGERS [nTrigger].m_info.nType == TT_CLOSE_WALL)
		return false;
	nTrigger++;
	}
faceP->m_info.nLightmap = 0;
return true;
}

//------------------------------------------------------------------------------

static float offset [2] = {1.3846153846f, 3.2307692308f};
static float weight [3] = {0.2270270270f, 0.3162162162f, 0.0702702703f};


static inline void Scale (CRGBColor& color, int i)
{
color.r = ubyte (float (color.r) * weight [i] + 0.5f);
color.g = ubyte (float (color.g) * weight [i] + 0.5f);
color.b = ubyte (float (color.b) * weight [i] + 0.5f);
}

static inline void Add (CLightmapFaceData& source, float x0, float y0, int i, CRGBColor& color)
{
	float w = float (LM_W), h = float (LM_H);

int x = int (x0 * w + 0.5f);
int y = int (y0 * h + 0.5f);
if (x < 0)
	x = 0;
else if (x >= LM_W)
	x = LM_W - 1;
if (y < 0)
	y = 0;
else if (y >= LM_H)
	y = LM_H - 1;
CRGBColor c = source.m_texColor [y * LM_W + x];
Scale (c, i);
}


void CLightmapManager::Blur (CSegFace* faceP, CLightmapFaceData& source, CLightmapFaceData& dest, int direction)
{
	int			w = LM_W, h = LM_H;
	float			xScale = (1.0f - float (direction)), yScale = float (direction), xo, yo;
	CRGBColor	color, * destColor = dest.m_texColor;

for (int y = 0; y < h; y++) {
	float y0 = float (y) / float (h);
	for (int x = 0; x < w; x++) {
		float x0 = float (x) / float (w);
		color.r = color.g = color.b = 0;
		Add (source, x0, y0, 0, color);
		xo = offset [0] * xScale;
		yo = offset [0] * yScale;
		Add (source, x0 - xo, y0 - xo, 1, color);
		Add (source, x0 + xo, y0 + xo, 1, color);
		xo = offset [1] * xScale;
		yo = offset [1] * yScale;
		Add (source, x0 - xo, y0 - xo, 2, color);
		Add (source, x0 + xo, y0 + xo, 2, color);
		*destColor++ = color;
		}
	}
}



void CLightmapManager::Blur (CSegFace* faceP, CLightmapFaceData& source)
{
	CLightmapFaceData	tempData;

Blur (faceP, source, tempData, 0);
Blur (faceP, tempData, source, 1);
}

//------------------------------------------------------------------------------
// build one entire lightmap in single threaded mode or the upper and lower halves when multi threaded

void CLightmapManager::Build (CSegFace* faceP, int nThread)
{
if (FaceIsInvisible (faceP))
	return;

	CFixVector		*pixelPosP;
	CRGBColor		*texColorP;
	CFloatVector3	color;
	CFixVector		v0, v1, v2;
	struct {
		CFixVector	x, y;
	} offset;
	int				w = LM_W, h = LM_H, x, y;
	int				i0, i1, i2; 
	bool				bBlack, bWhite;

if (nThread < 0)
	nThread = omp_get_thread_num();

CLightmapFaceData	faceData;
faceData.Setup (faceP);

#if 0// DBG
if ((faceP->m_info.nSegment != nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide != nDbgSide))) {
	faceData.m_nColor |= 1;
	return;
	}
#endif

#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
faceData.m_vcd.vertPosP = &faceData.m_vcd.vertPos;
pixelPosP = faceData.m_pixelPos;
if (faceData.m_nType) {
	i1 = faceData.m_sideVerts [0]; 
	v1 = VERTICES [i1];
	i2 = faceData.m_sideVerts [2]; 
	v2 = VERTICES [i2];
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++, pixelPosP++) {
			if (y >= x) {
				i0 = faceData.m_sideVerts [1]; 
				v0 = VERTICES [i0];
				offset.x = (v0 - v1) * m_data.nOffset [y];
				offset.y = (v2 - v0) * m_data.nOffset [x];
				}
			else {
				i0 = faceData.m_sideVerts [3]; 
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
	i1 = faceData.m_sideVerts [1]; 
	v1 = VERTICES [i1];
	i2 = faceData.m_sideVerts [3]; 
	v2 = VERTICES [i2];
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++, pixelPosP++) {
			if (h - y >= x) {
				i0 = faceData.m_sideVerts [0]; 
				v0 = VERTICES [i0];
				offset.x = (v1 - v0) * m_data.nOffset [y];  
				offset.y = (v2 - v0) * m_data.nOffset [x];
				}
			else {
				i0 = faceData.m_sideVerts [2]; 
				v0 = VERTICES [i0];
				offset.y = (v1 - v0) * m_data.nOffset [h - x];  
				offset.x = (v2 - v0) * m_data.nOffset [h - y]; 
				}
			*pixelPosP = v0 + offset.x + offset.y; 
			}
		}
	}

bBlack = bWhite = true;
pixelPosP = faceData.m_pixelPos;
for (y = 0; y < h; y++) {
#if DBG
	if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	for (x = 0; x < w; x++, pixelPosP++) { 
#if DBG
		if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide))) {
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
		if (0 < lightManager.SetNearestToPixel (faceP->m_info.nSegment, faceP->m_info.nSide, &faceData.m_vNormal, 
															 pixelPosP, faceP->m_info.fRads [1] / 10.0f, nThread)) {
			faceData.m_vcd.vertPos.Assign (*pixelPosP);
			color.SetZero ();
			G3AccumVertColor (-1, &color, &faceData.m_vcd, nThread);
			if ((color.Red () >= 1.0f / 255.0f) || (color.Green () >= 1.0f / 255.0f) || (color.Blue () >= 1.0f / 255.0f)) {
					bBlack = false;
				if (color.Red () >= 254.0f / 255.0f)
					color.Red () = 1.0f;
				else
					bWhite = false;
				if (color.Green () >= 254.0f / 255.0f)
					color.Green () = 1.0f;
				else
					bWhite = false;
				if (color.Blue () >= 254.0f / 255.0f)
					color.Blue () = 1.0f;
				else
					bWhite = false;
				}
			texColorP = faceData.m_texColor + x * w + y;
			texColorP->Red () = (ubyte) (255 * color.Red ());
			texColorP->Green () = (ubyte) (255 * color.Green ());
			texColorP->Blue () = (ubyte) (255 * color.Blue ());
			}
		}
	}

if (bBlack) {
	faceP->m_info.nLightmap = 0;
#pragma omp atomic
	m_data.nBlackLightmaps++;
	return;
	}
if (bWhite) {
	faceP->m_info.nLightmap = 1;
#pragma omp atomic
	m_data.nWhiteLightmaps++;
	return;
	}

//Blur (faceP, faceData);
#pragma omp critical
{
faceP->m_info.nLightmap = m_list.nLightmaps++;
}
Copy (faceData.m_texColor, faceP->m_info.nLightmap);

#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
}

//------------------------------------------------------------------------------

int CLightmapManager::CompareFaces (const tSegFacePtr* pf, const tSegFacePtr* pm)
{
	int	k = (*pf)->m_info.nKey;
	int	m = (*pm)->m_info.nKey;

return (k < m) ? -1 : (k > m) ? 1 : 0;
}

//------------------------------------------------------------------------------

void CLightmapManager::BuildAll (int nFirstFace)
{
	int nLastFace; 

if (nFirstFace <= 0) {
	m_data.nBlackLightmaps = 0,
	m_data.nWhiteLightmaps = 0; 
	CLightmapFaceData faceData;
	CreateSpecial (faceData.m_texColor, 0, 0);
	CreateSpecial (faceData.m_texColor, 1, 255);
	m_list.nLightmaps = 2;
	float h = 1.0f / float (LM_W - 1);
	for (int i = 0; i < LM_W; i++)
		m_data.nOffset [i] = F2X (i * h);
	}

if (nFirstFace >= 0)
	Build (&FACES.faces [nFirstFace], 0);
else {
	INIT_PROGRESS_LOOP (nFirstFace, nLastFace, gameData.segs.nFaces);
#if USE_OPENMP
	if (gameStates.app.bMultiThreaded) {
		nLastFace -= nFirstFace;
#	pragma omp parallel
		#pragma omp for
		for (int nFace = 0; nFace < nLastFace; nFace++) 
			Build (&FACES.faces [nFirstFace + nFace], -1);
		}
	else 
#endif // USE_OPENMP
	for (int nFace = nFirstFace; nFace < nLastFace; nFace++) 
		Build (&FACES.faces [nFace], 0);
	}
}

//------------------------------------------------------------------------------

static int nFace = 0;

static int CreatePoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

//paletteManager.ResumeEffect ();
if (nFace < gameData.segs.nFaces) {
	lightmapManager.BuildAll (nFace);
	nFace += PROGRESS_INCR;
	}
else {
	key = -2;
	//paletteManager.ResumeEffect ();
	return nCurItem;
	}
menu [0].m_value++;
menu [0].m_bRebuild = 1;
key = 0;
//paletteManager.ResumeEffect ();
return nCurItem;
}

//------------------------------------------------------------------------------

char *CLightmapManager::Filename (char *pszFilename, int nLevel)
{
if (gameOpts->render.color.nLevel == 2)
	return GameDataFilename (pszFilename, "lmap", nLevel, (gameOpts->render.nLightmapQuality + 1) * (gameOpts->render.nLightmapPrecision + 1) - 1);
else
	return GameDataFilename (pszFilename, "bw.lmap", nLevel, (gameOpts->render.nLightmapQuality + 1) * (gameOpts->render.nLightmapPrecision + 1) - 1);
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
										CalcSegmentCheckSum (),
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
		bOk = cf.Write (&faceP->m_info.nLightmap, sizeof (faceP->m_info.nLightmap), 1) == 1;
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
	CFile						cf;
	tLightmapDataHeader	ldh;
	int						i, bOk;
	char						szFilename [FILENAME_LEN];
	CSegFace*				faceP;

if (!(gameStates.app.bCacheLightmaps))
	return 0;
if (!cf.Open (Filename (szFilename, nLevel), gameFolders.szCacheDir, "rb", 0))
	return 0;
bOk = (cf.Read (&ldh, sizeof (ldh), 1) == 1);
if (bOk)
	bOk = (ldh.nVersion == LIGHTMAP_DATA_VERSION) && 
			(ldh.nCheckSum == CalcSegmentCheckSum ()) &&
			(ldh.nSegments == gameData.segs.nSegments) && 
			(ldh.nVertices == gameData.segs.nVertices) && 
			(ldh.nFaces == gameData.segs.nFaces) && 
			(ldh.nLights == m_list.nLights) && 
			(ldh.nMaxLightRange == MAX_LIGHT_RANGE);
if (bOk) {
	for (i = ldh.nFaces, faceP = FACES.faces.Buffer (); i; i--, faceP++) {
		bOk = cf.Read (&faceP->m_info.nLightmap, sizeof (faceP->m_info.nLightmap), 1) == 1;
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
if (gameOpts->render.nLightmapPrecision > 2)
	gameOpts->render.nLightmapPrecision = 2;
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
		lightManager.Index (0,0).nFirst = MAX_OGL_LIGHTS;
		lightManager.Index (0,0).nLast = 0;
		gameStates.render.bSpecularColor = 0;
		gameStates.render.nState = 0;
		//PLANE_DIST_TOLERANCE = fix (I2X (1) * 0.001f);
		//SetupSegments (); // set all faces up as triangles
		if (gameStates.app.bProgressBars && gameOpts->menus.nStyle) {
			nFace = 0;
			ProgressBar (TXT_CALC_LIGHTMAPS, 0, PROGRESS_STEPS (gameData.segs.nFaces), CreatePoll);
			}
		else {
			messageBox.Show (TXT_CALC_LIGHTMAPS);
			BuildAll (-1);
			messageBox.Clear ();
			}
		//PLANE_DIST_TOLERANCE = DEFAULT_PLANE_DIST_TOLERANCE;
		//SetupSegments (); // standard face setup (triangles or quads)
		//gameData.render.fAttScale [0] = (gameStates.render.bPerPixelLighting == 2) ? 1.0f : 2.0f;
		gameStates.render.bSpecularColor = 1;
		gameStates.render.bHaveLightmaps = 0;
		gameStates.render.nState = 0;
		gameOpts->render.color.nSaturation = nSaturation;
		Realloc ((m_list.nLightmaps + LIGHTMAP_BUFSIZE - 1) / LIGHTMAP_BUFSIZE);
		}
	else {
		CLightmapFaceData faceData;
		CreateSpecial (faceData.m_texColor, 0, 0);
		m_list.nLightmaps = 1;
		for (int i = 0; i < gameData.segs.nFaces; i++)
			FACES.faces [i].m_info.nLightmap = 0;
		}
	BindAll ();
	Save (nLevel);
	}
return 1;
}

//------------------------------------------------------------------------------

void CLightmapManager::Setup (int nLevel)
{
if (gameStates.render.bPerPixelLighting) {
	Create (nLevel);
	if (HaveLightmaps ())
		meshBuilder.RebuildLightmapTexCoord ();	//rebuild to create proper lightmap texture coordinates
	else
		gameOpts->render.bUseLightmaps = 0;
	}
}

//------------------------------------------------------------------------------

int SetupLightmap (CSegFace* faceP)
{
#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
int i = faceP->m_info.nLightmap / LIGHTMAP_BUFSIZE;
if (!lightmapManager.Bind (i))
	return 0;
GLuint h = lightmapManager.Buffer (i)->handle;
#if 0 //!DBG
if (0 <= ogl.IsBound (h))
	return 1;
#endif
ogl.SelectTMU (GL_TEXTURE0, true);
ogl.SetTexturing (true);
glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
ogl.BindTexture (h);
gameData.render.nStateChanges++;
return 1;
}

//------------------------------------------------------------------------------

