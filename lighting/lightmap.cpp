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
#include "key.h"
#include "netmisc.h"
#include "gamepal.h"
#include "loadgeometry.h"
#include "renderthreads.h"
#include "createmesh.h"

CLightmapManager lightmapManager;

//------------------------------------------------------------------------------

#define LMAP_REND2TEX		0
#define TEXTURE_CHECK		1

#define LIGHTMAP_DATA_VERSION 49
#define LM_W	LIGHTMAP_WIDTH
#define LM_H	LIGHTMAP_WIDTH

//------------------------------------------------------------------------------

typedef struct tLightmapDataHeader {
	int32_t	nVersion;
	int32_t	nCheckSum;
	int32_t	nSegments;
	int32_t	nVertices;
	int32_t	nFaces;
	int32_t	nLights;
	int32_t	nMaxLightRange;
	int32_t	nBuffers;
	int32_t	bCompressed;
	} tLightmapDataHeader;

//------------------------------------------------------------------------------

GLhandleARB lmShaderProgs [3] = {0,0,0}; 
GLhandleARB lmFS [3] = {0,0,0}; 
GLhandleARB lmVS [3] = {0,0,0}; 

#if DBG
int32_t lightmapWidth [5] = {8, 16, 32, 64, 128};
#else
int32_t lightmapWidth [5] = {16, 32, 64, 128, 256};
#endif

tLightmap dummyLightmap;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CLightmapProgress::Setup (void)
{
if (!gameStates.app.bPrecomputeLightmaps) 
	return;
if (!(m_pProgressMenu = CMenu::Active ()))
	return;
if ((m_pLevelProgress = (*m_pProgressMenu) ["subtitle 1"]))
		m_pLevelProgress->SetText ("level progress");
	if (!(m_pLevelProgress = (*m_pProgressMenu) ["progress bar 2"]))
		return;

if (!m_bActive) {
	if (!((m_pTotalProgress = (*m_pProgressMenu) ["progress bar 1"])))
		return;
	if (!(m_pTime = (*m_pProgressMenu) ["time"]))
		return;
	m_fTotal = float (m_pTotalProgress->MaxValue ());
	m_tStart = SDL_GetTicks ();
	m_pLevelCount = (*m_pProgressMenu) ["level count"];
	m_nSkipped = 0;
	m_bActive = true;
	}

if (m_pLevelCount) {
	char szLabel [50];
	sprintf (szLabel, "%d / %d", m_pTotalProgress->Value () / 100 + 1, m_pTotalProgress->MaxValue () / Scale ());
	m_pLevelCount->SetText (szLabel);
	}

m_pLevelProgress->Value () = 0;
m_pLevelProgress->MaxValue () = FACES.nFaces;
m_nLocalProgress = 0; 
}

//------------------------------------------------------------------------------

void CLightmapProgress::Update (void)
{
if (m_bActive) {
	m_pLevelProgress->Value ()++;
	m_pLevelProgress->Rebuild ();
	float fLevelProgress = float (m_pLevelProgress->Value ()) / float (FACES.nFaces);
	int32_t nLevelProgress = int32_t (float (Scale ()) * fLevelProgress + 0.5f);
	if (m_nLocalProgress < nLevelProgress) {
		m_nLocalProgress = nLevelProgress;
		m_pTotalProgress->Value ()++;
		m_pTotalProgress->Rebuild ();
		}
	if (m_pTime) {
		static CTimeout to (1000);
		if (to.Expired ()) {
			int32_t nTotalProgress = m_pTotalProgress->Value ();
			int32_t tLeft, tPassed = SDL_GetTicks () - m_tStart;
			if (nTotalProgress / Scale () - m_nSkipped > 0)
				tLeft = Max (int32_t (float (tPassed) * m_fTotal / float (nTotalProgress - m_nSkipped * Scale ())) - tPassed, 0);
			else { // for the first level, estimate the time needed for the entire level and scale with number of levels to be done
				int32_t tLevel = int32_t (float (tPassed) / fLevelProgress);
				tLeft = int32_t (float (tLevel) * m_fTotal / float (Scale ())) - tPassed;
				}
			tPassed = (tPassed + 500) / 1000;
			tLeft = (tLeft + 500) / 1000;
			char szTime [50];
			sprintf (szTime, "%d:%02d:%02d / %d:%02d:%02d", 
						tPassed / 3600, (tPassed / 60) % 60, tPassed % 60,
						tLeft / 3600, (tLeft / 60) % 60, tLeft % 60);
			m_pTime->SetText (szTime);
			}
		}
	m_pProgressMenu->Render (m_pProgressMenu->Title (), m_pProgressMenu->SubTitle ());
	}
}

//------------------------------------------------------------------------------

void CLightmapProgress::Skip (int32_t i)
{
++m_nSkipped;
if (m_pTotalProgress) {
	m_pTotalProgress->Value () += i * Scale ();
	m_fTotal = float (m_pTotalProgress->MaxValue () - m_nSkipped * Scale ());
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CLightmapFaceData::Setup (CSegFace* pFace)
{
CSide* pSide = SEGMENT (pFace->m_info.nSegment)->m_sides + pFace->m_info.nSide;
m_nType = pSide->m_nType;
m_vNormal = pSide->m_normals [2];
m_vCenter = pSide->Center ();
CFixVector::Normalize (m_vNormal);
m_vcd.vertNorm.Assign (m_vNormal);
CFloatVector3::Normalize (m_vcd.vertNorm);
InitVertColorData (m_vcd);
m_vcd.vertPosP = &m_vcd.vertPos;
m_vcd.fMatShininess = 4;
memcpy (m_sideVerts, pSide->m_corners, sizeof (m_sideVerts));
//memcpy (m_sideVerts, pFace->m_info.index, sizeof (m_sideVerts));
m_nColor = 0;
memset (m_texColor, 0, LM_W * LM_H * sizeof (CRGBColor));
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32_t CLightmapManager::Bind (int32_t nLightmap)
{
	tLightmapBuffer	*pLightmap = &m_list.buffers [nLightmap];
#if DBG
	int32_t				nError;
#endif

if (pLightmap->handle)
	return 1;
ogl.GenTextures (1, &pLightmap->handle);
if (!pLightmap->handle) {
#if 0//DBG
	nError = glGetError ();
#endif
	return 0;
	}
ogl.BindTexture (pLightmap->handle); 
#if 0//DBG
if ((nError = glGetError ()))
	return 0;
#endif
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexImage2D (GL_TEXTURE_2D, 0, 3, LIGHTMAP_BUFWIDTH, LIGHTMAP_BUFWIDTH, 0, GL_RGB, GL_UNSIGNED_BYTE, pLightmap->pBm);
#if DBG
if ((nError = glGetError ()))
	return 0;
#endif
return 1;
}

//------------------------------------------------------------------------------

int32_t CLightmapManager::BindAll (void)
{
for (int32_t i = 0; i < m_list.nBuffers; i++)
	if (!Bind (i))
		return 0;
return 1;
}

//------------------------------------------------------------------------------

void CLightmapManager::Release (void)
{
if (m_list.buffers.Buffer ()) { 
	tLightmapBuffer *pLightmap = &m_list.buffers [0];
	for (int32_t i = m_list.nBuffers; i; i--, pLightmap++)
		if (pLightmap->handle) {
			ogl.DeleteTextures (1, reinterpret_cast<GLuint*> (&pLightmap->handle));
			pLightmap->handle = 0;
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

inline void CLightmapManager::ComputePixelOffset (CFixVector& vOffs, CFixVector& v1, CFixVector& v2, int32_t nOffset)
{
vOffs = (v2 - v1) * nOffset;
}

//------------------------------------------------------------------------------

void CLightmapManager::RestoreLights (int32_t bVariable)
{
	CDynLight*	pLight = lightManager.Lights ();

for (int32_t i = lightManager.LightCount (0); i; i--, pLight++)
	if (!(pLight->info.nType || (pLight->info.bVariable && !bVariable)))
		pLight->info.bOn = 1;
}

//------------------------------------------------------------------------------

int32_t CLightmapManager::CountLights (int32_t bVariable)
{
	CDynLight*	pLight = lightManager.Lights ();
	int32_t			nLights = 0;

if (!gameStates.render.bPerPixelLighting)
	return 0;
for (int32_t i = lightManager.LightCount (0); i; i--, pLight++)
	if (!(pLight->info.nType || (pLight->info.bVariable && !bVariable)))
		nLights++;
return nLights; 
}

//------------------------------------------------------------------------------

double CLightmapManager::SideRad (int32_t nSegment, int32_t nSide)
{
	int32_t			i;
	double		h, xMin, xMax, yMin, yMax, zMin, zMax;
	double		dx, dy, dz;
	CFixVector	*v;
	CSide*		pSide = SEGMENT (nSegment)->Side (nSide);
	uint16_t*		sideVerts = pSide->Corners ();

xMin = yMin = zMin = 1e300;
xMax = yMax = zMax = -1e300;
for (i = 0; i < pSide->CornerCount (); i++) {
	v = gameData.segData.vertices +sideVerts [i];
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
return sqrt (dx * dx + dy * dy + dz * dz) / (double) I2X (2);
}

//------------------------------------------------------------------------------

int32_t CLightmapManager::Init (int32_t bVariable)
{
	CDynLight*		pLight;
	CSegFace*		pFace = NULL;
	int32_t			nIndex, i;
	tLightmapInfo*	pLightmapInfo;  //temporary place to put light data.
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
m_list.nBuffers = (FACES.nFaces + LIGHTMAP_BUFSIZE - 1) / LIGHTMAP_BUFSIZE;
if (!m_list.buffers.Create (m_list.nBuffers)) {
	m_list.info.Destroy ();
	m_list.nLights = 0; 
	return -1;
	}
m_list.buffers.Clear (); 
m_list.info.Clear (); 
m_list.nLights = 0; 
//first lightmap is dummy lightmap for multi pass lighting
pLightmapInfo = m_list.info.Buffer (); 
for (pLight = lightManager.Lights (), i = lightManager.LightCount (0); i; i--, pLight++) {
	if (pLight->info.nType || (pLight->info.bVariable && !bVariable))
		continue;
	if (pFace == pLight->info.pFace)
		continue;
	pFace = pLight->info.pFace;
	sideRad = (double) pFace->m_info.fRads [1] / 10.0;
	nIndex = pFace->m_info.nSegment * 6 + pFace->m_info.nSide;
	//Process found light.
	pLightmapInfo->range += sideRad;
	//find where it is in the level.
	pLightmapInfo->vPos = SEGMENT (pFace->m_info.nSegment)->SideCenter (pFace->m_info.nSide);
	pLightmapInfo->nIndex = nIndex; 
	//find light direction, currently based on first 3 points of CSide, not always right.
	CFixVector *pNormal = SEGMENT (pFace->m_info.nSegment)->m_sides [pFace->m_info.nSide].m_normals;
	pLightmapInfo->vDir = CFixVector::Avg(pNormal[0], pNormal[1]);
	pLightmapInfo++; 
	}
return m_list.nLights = (int32_t) (pLightmapInfo - m_list.info.Buffer ()); 
}

//------------------------------------------------------------------------------

#if LMAP_REND2TEX

// create 512 brightness values using inverse square
void InitBrightmap (uint8_t *brightmap)
{
	int32_t		i;
	double	h;

for (i = 512; i; i--, brightmap++) {
	h = (double) i / 512.0;
	h *= h;
	*brightmap = (uint8_t) FRound ((255 * h));
	}
}

//------------------------------------------------------------------------------

// create a color/brightness table
void InitLightmapInfo (uint8_t *lightmap, uint8_t *brightmap, GLfloat *color)
{
	int32_t		i, j;
	double	h;

for (i = 512; i; i--, brightmap++)
	for (j = 0; j < 3; j++, lightmap++)
		*lightmap = (uint8_t) FRound (*brightmap * color [j]);
}

#endif //LMAP_REND2TEX

//------------------------------------------------------------------------------

bool CLightmapManager::FaceIsInvisible (CSegFace* pFace)
{
#if DBG
if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
	BRP;
#endif
CSegment *pSeg = SEGMENT (pFace->m_info.nSegment);
if (pSeg->m_function == SEGMENT_FUNC_SKYBOX) {
	pFace->m_info.nLightmap = 1;
	return true;
	}
if (pSeg->m_children [pFace->m_info.nSide] < 0)
	return false;

CWall* pWall = pSeg->m_sides [pFace->m_info.nSide].Wall ();
if (!pWall || (pWall->nType != WALL_OPEN)) 
	return false;

int32_t nTrigger = 0; 
for (;;) {
	if (0 > (nTrigger = pWall->IsTriggerTarget (nTrigger)))
		return false;
	CTrigger* pTrigger = GEOTRIGGER (nTrigger);
	if (pTrigger && (pTrigger->m_info.nType == TT_CLOSE_WALL))
		return false;
	nTrigger++;
	}
pFace->m_info.nLightmap = 0;
return true;
}

//------------------------------------------------------------------------------

//static float offset [2] = {1.3846153846f, 3.2307692308f};
//static float weight [3] = {0.2270270270f, 0.3162162162f, 0.0702702703f};
static float weight [5] = {0.2270270270f, 0.1945945946f, 0.1216216216f, 0.0540540541f, 0.0162162162f};


static inline void Add (CLightmapFaceData& source, int32_t x, int32_t y, int32_t offset, CFloatVector3& color)
{
#if 0
if (x < 0)
	return;
if (x >= LM_W)
	return;
if (y < 0)
	return;
if (y >= LM_H)
	return;
#else
if (x < 0)
	x = 0;
else if (x >= LM_W)
	x = LM_W - 1;
if (y < 0)
	y = 0;
else if (y >= LM_H)
	y = LM_H - 1;
#endif
CRGBColor& sourceColor = source.m_texColor [y * LM_W + x];
float w = weight [offset];
color.Red () += sourceColor.r * w;
color.Green () += sourceColor.g * w;
color.Blue () += sourceColor.b * w;
}


void CLightmapManager::Blur (CSegFace* pFace, CLightmapFaceData& source, CLightmapFaceData& dest, int32_t direction)
{
	int32_t			w = LM_W, h = LM_H;
	int32_t			xScale = 1 - direction, yScale = direction, xo, yo;
	CRGBColor*		destColor = dest.m_texColor;
	CFloatVector3	color;

for (int32_t y = 0; y < h; y++) {
	for (int32_t x = 0; x < w; x++, destColor++) {
		color.Set (0.0f, 0.0f, 0.0f);
		Add (source, x, y, 0, color);
		for (int32_t offset = 1; offset < 5; offset++) {
			xo = offset * xScale;
			yo = offset * yScale;
			Add (source, x - xo, y - yo, offset, color);
			Add (source, x + xo, y + yo, offset, color);
			}
		destColor->Set ((uint8_t) FRound (color.Red ()), (uint8_t) FRound (color.Green ()), (uint8_t) FRound (color.Blue ()));
		}
	}
}



void CLightmapManager::Blur (CSegFace* pFace, CLightmapFaceData& source)
{
#if !DBG
	CLightmapFaceData	tempData;

Blur (pFace, source, tempData, 0);
Blur (pFace, tempData, source, 1);
#endif
}

//------------------------------------------------------------------------------

void CLightmapManager::Copy (CRGBColor *pTexColor, uint16_t nLightmap)
{
tLightmapBuffer *pBuffer = &m_list.buffers [nLightmap / LIGHTMAP_BUFSIZE];
int32_t i = nLightmap % LIGHTMAP_BUFSIZE;
int32_t x = (i % LIGHTMAP_ROWSIZE) * LM_W;
int32_t y = (i / LIGHTMAP_ROWSIZE) * LM_H;
for (i = 0; i < LM_H; i++, y++, pTexColor += LM_W)
	memcpy (&pBuffer->pBm [y][x], pTexColor, LM_W * sizeof (CRGBColor));
}

//------------------------------------------------------------------------------

void CLightmapManager::CreateSpecial (CRGBColor *pTexColor, uint16_t nLightmap, uint8_t nColor)
{
memset (pTexColor, nColor, LM_W * LM_H * sizeof (CRGBColor));
Copy (pTexColor, nLightmap);
}

//------------------------------------------------------------------------------
// build one entire lightmap in single threaded mode or in horizontal stripes when multi threaded

void CLightmapManager::Build (CSegFace* pFace, int32_t nThread)
{
	CFixVector*		pPixelPos;
	CRGBColor*		pTexColor;
	CFloatVector3	color;
	int32_t			w, h, x, y, yMin, yMax;
	uint8_t			nTriangles = pFace->m_info.nTriangles - 1;
	int16_t			nSegment = pFace->m_info.nSegment;
	int8_t			nSide = (int8_t) pFace->m_info.nSide;
	bool				bBlack, bWhite;

	CVertColorData	vcd = m_data.m_vcd; // need a local copy for each thread!

h = LM_H;
w = LM_W;

if (nThread < 0) {
	yMin = 0;
	yMax = h;
	nThread = 0;
	}
else {
	int32_t nStep = (h + gameStates.app.nThreads - 1) / gameStates.app.nThreads;
	yMin = nStep * nThread;
	yMax = yMin + nStep;
	if (yMax > h)
		yMax = h;
	}

#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
#endif

vcd.vertPosP = &vcd.vertPos;
pPixelPos = m_data.m_pixelPos + yMin * w;

#if 1 

// move edges for lightmap calculation slighty into the face to avoid lighting errors due to numerical errors when computing point to face visibility
// for light sources and lightmap vertices. Otherwise these may occur at the borders of a face when the face to be lit is adjacent to a light emitting face.
// The result of such lighting errors are dark spots at the borders of level geometry faces.

CFloatVector corners [4] = {
	FVERTICES [m_data.m_sideVerts [0]],
	FVERTICES [m_data.m_sideVerts [1]],
	FVERTICES [m_data.m_sideVerts [2]],
	FVERTICES [m_data.m_sideVerts [nTriangles ? 3 : 0]],
	};

CFloatVector o0 = corners [0];
o0 -= corners [1];
CFloatVector::Normalize (o0);
o0 *= 0.0015f;

CFloatVector o1 = corners [1];
o1 -= corners [2];
CFloatVector::Normalize (o1);
o1 *= 0.0015f;

CFloatVector o2 = corners [2];
o2 -= corners [3];
CFloatVector::Normalize (o2);
o2 *= 0.0015f;

if (!nTriangles) {
	corners [0] += o2;
	corners [0] -= o0;
	corners [1] += o0;
	corners [1] -= o1;
	corners [2] += o1;
	corners [2] -= o2;
	}
else {
	CFloatVector o3 = corners [3];
	o3	-= corners [0];
	CFloatVector::Normalize (o3);
	o3 *= 0.0015f;

	corners [0] += o3;
	corners [0] -= o0;
	corners [1] += o0;
	corners [1] -= o1;
	corners [2] += o1;
	corners [2] -= o2;
	corners [3] += o2;
	corners [3] -= o3;
	}

CFixVector v0, v1, v2, v3;

v0.Assign (corners [0]);
v1.Assign (corners [1]);
v2.Assign (corners [2]);
v3.Assign (corners [3]);

#else

CFixVector v0 = VERTICES [m_data.m_sideVerts [0]];
CFixVector v1 = VERTICES [m_data.m_sideVerts [1]];
CFixVector v2 = VERTICES [m_data.m_sideVerts [2]];
CFixVector v3 = VERTICES [m_data.m_sideVerts [3]];

#endif

CFixVector dx, dy;

if (!nTriangles) {
	CFixVector l0 = v2 - v1;
	CFixVector l1 = v1 - v0;
	for (y = yMin; y < yMax; y++) {
		for (x = 0; x <= y; x++, pPixelPos++) {
			dx = l0;
			dy = l1;
			dx *= m_data.nOffset [x];
			dy *= m_data.nOffset [y];
			*pPixelPos = v0;
			*pPixelPos += dx; 
			*pPixelPos += dy; 
			}
		}
	}
else if (m_data.m_nType != SIDE_IS_TRI_13) {
	CFixVector l0 = v2 - v1;
	CFixVector l1 = v1 - v0;
	CFixVector l2 = v3 - v0;
	CFixVector l3 = v2 - v3;
	for (y = yMin; y < yMax; y++) {
		for (x = 0; x < w; x++, pPixelPos++) {
			if (x <= y) {
				dx = l0;
				dy = l1;
				}
			else {
				dx = l2; 
				dy = l3; 
				}
			dx *= m_data.nOffset [x];
			dy *= m_data.nOffset [y];
			*pPixelPos = v0;
			*pPixelPos += dx; 
			*pPixelPos += dy; 
			}
		}
	}
else { 
	--h;
	CFixVector l0 = v3 - v0;
	CFixVector l1 = v1 - v0;
	CFixVector l2 = v1 - v2;
	CFixVector l3 = v3 - v2;
	for (y = yMin; y < yMax; y++) {
		for (x = 0; x < w; x++, pPixelPos++) {
			if (h - y >= x) {
				dx = l0;
				dx *= m_data.nOffset [x];
				dy = l1;
				dy *= m_data.nOffset [y];  
				*pPixelPos = v0;
				}
			else {
				dx = l2;
				dx *= m_data.nOffset [h - x];  
				dy = l3;
				dy *= m_data.nOffset [h - y]; 
				*pPixelPos = v2; 
				}
			*pPixelPos += dx; 
			*pPixelPos += dy; 
			}
		}
	}

bBlack = bWhite = true;
for (y = yMin; y < yMax; y++) {


#if DBG
	if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
		BRP;
#endif
	int32_t h = nTriangles ? w : y + 1;
	pPixelPos = m_data.m_pixelPos + y * w;
	for (x = 0; x < h; x++, pPixelPos++) { 

	if (!nThread) {
		static CTimeout to (33);
		if (to.Expired ()) {
			int32_t nKey = KeyInKey ();
			if (nKey == KEY_ESC)
				m_bSuccess = 0;
			if (nKey == KEY_ALTED + KEY_F4)
				exit (0);

			CMenu* m = CMenu::Active ();
			if (m)
				m->Render (m->Title (), m->SubTitle ());
			}
		}

	if (!m_bSuccess)
		return;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide))) {
			BRP;
			if ((x == w - 1) && (y == w - 1))
				BRP;
			if (((x == 0) || (x == w - 1)) || ((y == 0) || (y == w - 1)))
				BRP;
			if (x == 0)
				BRP;
			if (x == w - 1)
				BRP;
			if (y == 0)
				BRP;
			if (y == w - 1)
				BRP;
			}
		fix dist = x ? CFixVector::Dist (*pPixelPos, *(pPixelPos - 1)) : 0;
#endif
#if DBG
		int32_t nLights = lightManager.SetNearestToPixel (nSegment, nSide, &m_data.m_vNormal, pPixelPos, pFace->m_info.fRads [1] / 10.0f, nThread);
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			BRP;
		if (0 < nLights) {
#else
		if (0 < lightManager.SetNearestToPixel (nSegment, nSide, &m_data.m_vNormal, pPixelPos, pFace->m_info.fRads [1] / 10.0f, nThread)) {
#endif
			vcd.vertPos.Assign (*pPixelPos);
			color.SetZero ();
			ComputeVertexColor (nSegment, nSide, -1, &color, &vcd, nThread);
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				BRP;
#endif
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
			pTexColor = m_data.m_texColor + x * w + y;
			pTexColor->Red () = (uint8_t) (255 * color.Red ());
			pTexColor->Green () = (uint8_t) (255 * color.Green ());
			pTexColor->Blue () = (uint8_t) (255 * color.Blue ());
			}
		}
	}
#pragma omp critical (CLightmapManagerBuild)
{
if (bBlack)
	m_data.m_nColor |= 1;
else if (bWhite)
	m_data.m_nColor |= 2;
else 
	m_data.m_nColor |= 4;
}
#if DBG
if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
#endif
}

//------------------------------------------------------------------------------

int32_t CLightmapManager::CompareFaces (const tSegFacePtr* pf, const tSegFacePtr* pm)
{
	int32_t	k = (*pf)->m_info.nKey;
	int32_t	m = (*pm)->m_info.nKey;

return (k < m) ? -1 : (k > m) ? 1 : 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32_t CLightmapManager::BuildAll (int32_t nFace)
{
	int32_t		nLastFace;

INIT_PROGRESS_LOOP (nFace, nLastFace, FACES.nFaces);

if (nFace <= 0) {
	CreateSpecial (m_data.m_texColor, 0, 0);
	CreateSpecial (m_data.m_texColor, 1, 255);
	m_list.nLightmaps = 2;
	}
//Next Go through each surface and create a lightmap for it.
for (m_data.pFace = &FACES.faces [nFace]; nFace < nLastFace; nFace++, m_data.pFace++) {
#if DBG
	if ((m_data.pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (m_data.pFace->m_info.nSide == nDbgSide)))
		BRP;
#endif
	m_progress.Update ();
	if (SEGMENT (m_data.pFace->m_info.nSegment)->m_function == SEGMENT_FUNC_SKYBOX) {
		m_data.pFace->m_info.nLightmap = 1;
		continue;
		}
	if (FaceIsInvisible (m_data.pFace))
		continue;
	m_data.Setup (m_data.pFace);
	if (gameStates.app.nThreads < 2)
		Build (m_data.pFace, -1);
	else {
#if USE_OPENMP
#	pragma omp parallel for
		for (int32_t i = 0; i < gameStates.app.nThreads; i++)
			Build (m_data.pFace, i);
#else
		if (!RunRenderThreads (rtLightmap, gameStates.app.nThreads))
			Build (m_data.pFace, -1);
#endif
		}
	if (!m_bSuccess)
		return 0;
#if DBG
	if ((m_data.pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (m_data.pFace->m_info.nSide == nDbgSide)))
		BRP;
#endif
	if (m_data.m_nColor == 1) {
		m_data.pFace->m_info.nLightmap = 0;
		m_data.nBlackLightmaps++;
		}
	else if (m_data.m_nColor == 2) {
		m_data.pFace->m_info.nLightmap = 1;
		m_data.nWhiteLightmaps++;
		}
	else {
		Blur (m_data.pFace, m_data);
		Copy (m_data.m_texColor, m_list.nLightmaps);
		m_data.pFace->m_info.nLightmap = m_list.nLightmaps++;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

static int32_t nFace = 0;

static int32_t CreatePoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

//paletteManager.ResumeEffect ();
if (nFace < FACES.nFaces) {
	if (!lightmapManager.BuildAll (nFace)) {
		key = -2;
		return nCurItem;
		}
	nFace += PROGRESS_INCR;
	}
else {
	key = -2;
	//paletteManager.ResumeEffect ();
	return nCurItem;
	}
menu [0].Value ()++;
menu [0].Rebuild ();
key = 0;
//paletteManager.ResumeEffect ();
return nCurItem;
}

//------------------------------------------------------------------------------

char *CLightmapManager::Filename (char *pszFilename, int32_t nLevel)
{
#if 1
return GameDataFilename (pszFilename, "lmap", nLevel, (gameOpts->render.nLightmapQuality + 1) * (gameOpts->render.nLightmapPrecision + 1) - 1);
#else
if (gameOpts->render.color.nLevel == 2)
	return GameDataFilename (pszFilename, "lmap", nLevel, (gameOpts->render.nLightmapQuality + 1) * (gameOpts->render.nLightmapPrecision + 1) - 1);
else
	return GameDataFilename (pszFilename, "bw.lmap", nLevel, (gameOpts->render.nLightmapQuality + 1) * (gameOpts->render.nLightmapPrecision + 1) - 1);
#endif
}

//------------------------------------------------------------------------------

void CLightmapManager::Realloc (int32_t nBuffers)
{
if (m_list.nBuffers > nBuffers) {
	m_list.buffers.Resize (nBuffers);
	m_list.nBuffers = nBuffers;
	}
}

//------------------------------------------------------------------------------

void CLightmapManager::ToGrayScale (void)
{
for (int32_t i = 0; i < m_list.nBuffers; i++) {
	CRGBColor* pColor = &m_list.buffers [i].pBm [0][0];
	for (int32_t j = LIGHTMAP_BUFWIDTH * LIGHTMAP_BUFWIDTH; j; j--, pColor++)
		pColor->ToGrayScale (1);
	}
}

//------------------------------------------------------------------------------

void CLightmapManager::Posterize (void)
{
for (int32_t i = 0; i < m_list.nBuffers; i++) {
	CRGBColor* pColor = &m_list.buffers [i].pBm [0][0];
	for (int32_t j = LIGHTMAP_BUFWIDTH * LIGHTMAP_BUFWIDTH; j; j--, pColor++)
		pColor->Posterize ();
	}
}

//------------------------------------------------------------------------------

int32_t CLightmapManager::Save (int32_t nLevel)
{
	CFile				cf;
	tLightmapDataHeader ldh = {LIGHTMAP_DATA_VERSION, 
										CalcSegmentCheckSum (),
										gameData.segData.nSegments, 
										gameData.segData.nVertices, 
										FACES.nFaces, 
										m_list.nLights, 
										MAX_LIGHT_RANGE,
										m_list.nBuffers,
										gameStates.app.bCompressData
										};
	int32_t			i, bOk;
	char				szFilename [FILENAME_LEN];
	CSegFace			*pFace;

if (!(gameStates.app.bCacheLightmaps && m_list.nLights && m_list.nBuffers))
	return 0;
if (!cf.Open (Filename (szFilename, nLevel), gameFolders.var.szLightmaps, "wb", 0))
	return 0;
bOk = (cf.Write (&ldh, sizeof (ldh), 1) == 1);
if (bOk) {
	for (i = FACES.nFaces, pFace = FACES.faces.Buffer (); i; i--, pFace++) {
		bOk = cf.Write (&pFace->m_info.nLightmap, sizeof (pFace->m_info.nLightmap), 1) == 1;
		if (!bOk)
			break;
		}
	}
if (bOk) {
	for (i = 0; i < m_list.nBuffers; i++) {
		bOk = cf.Write (m_list.buffers [i].pBm, sizeof (m_list.buffers [i].pBm), 1, ldh.bCompressed) == 1;
		if (!bOk)
			break;
		}
	}
cf.Close ();
return bOk;
}

//------------------------------------------------------------------------------

int32_t CLightmapManager::Load (int32_t nLevel)
{
	CFile						cf;
	tLightmapDataHeader	ldh;
	int32_t					i, bOk;
	char						szFilename [FILENAME_LEN];
	CSegFace					*pFace;

if (!(gameStates.app.bCacheLightmaps)) {
	PrintLog (0, "lightmap caching is disabled\n");
	return 0;
	}
if (!cf.Open (Filename (szFilename, nLevel), gameFolders.var.szLightmaps, "rb", 0) &&
	 !cf.Open (Filename (szFilename, nLevel), gameFolders.var.szCache, "rb", 0)) {
	PrintLog (0, "Couldn't read lightmap file '%s'\n", Filename (szFilename, nLevel));
	if (*gameFolders.var.szLightmaps)
		PrintLog (0, "   Checked folder '%s'\n", gameFolders.var.szLightmaps);
	if (*gameFolders.var.szCache)
		PrintLog (0, "   Checked folder '%s'\n", gameFolders.var.szCache);
	return 0;
	}
bOk = (cf.Read (&ldh, sizeof (ldh), 1) == 1);
if (!bOk) 
	PrintLog (0, "Error reading lightmap header data\n");
else {
	bOk = false;
	if (ldh.nVersion != LIGHTMAP_DATA_VERSION) 
		PrintLog (0, "lightmap data outdated (version)\n");
	else if (ldh.nCheckSum != CalcSegmentCheckSum ())
		PrintLog (0, "lightmap data outdated (level data)\n");
	else if (ldh.nSegments != gameData.segData.nSegments) 
		PrintLog (0, "lightmap data outdated (segment count)\n");
	else if (ldh.nVertices != gameData.segData.nVertices) 
		PrintLog (0, "lightmap data outdated (vertex count)\n");
	else if (ldh.nFaces != FACES.nFaces) 
		PrintLog (0, "lightmap data outdated (face count)\n");
	else if (ldh.nLights != m_list.nLights) 
		PrintLog (0, "lightmap data outdated (light count)\n");
	else if (ldh.nBuffers > m_list.nBuffers)
		PrintLog (0, "lightmap data outdated (buffer count)\n");
	else if (ldh.nMaxLightRange != MAX_LIGHT_RANGE)
		PrintLog (0, "lightmap data outdated (light range)\n");
	else bOk = true;
	}
if (bOk) {
	for (i = ldh.nFaces, pFace = FACES.faces.Buffer (); i; i--, pFace++) {
		bOk = cf.Read (&pFace->m_info.nLightmap, sizeof (pFace->m_info.nLightmap), 1) == 1;
		if (!bOk) {
			PrintLog (0, "error reading lightmap count\n");
			break;
			}
		}
	}
if (bOk) {
	for (i = 0; i < ldh.nBuffers; i++) {
		bOk = cf.Read (m_list.buffers [i].pBm, sizeof (m_list.buffers [i].pBm), 1, ldh.bCompressed) == 1;
		if (!bOk) {
			PrintLog (0, "error reading lightmap data\n");
			break;
			}
		}
	}
cf.Close ();
if (bOk) {
	Realloc (ldh.nBuffers);
#if 1
	if (gameOpts->render.color.nLevel < 2)
		ToGrayScale ();
#endif
#if 1
	if (gameStates.render.CartoonStyle ())
		Posterize ();
#endif
	}
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

int32_t CLightmapManager::Create (int32_t nLevel)
{
if (!gameStates.render.bUsePerPixelLighting)
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
int32_t nLights = Init (0);
if (nLights < 0)
	return 0;

SetupProgress ();

if (Load (nLevel)) {
	m_progress.Skip (1);
	return 1;
	}

m_bSuccess = 1;

if ((gameStates.render.bPerPixelLighting || gameStates.app.bPrecomputeLightmaps) && FACES.nFaces) {
	if (nLights) {
		lightManager.Transform (1, 0);
		int32_t nSaturation = gameOpts->render.color.nSaturation;
		gameOpts->render.color.nSaturation = 1;
		gameStates.render.bHaveLightmaps = 1;
		//gameData.renderData.fAttScale [0] = 2.0f;
		lightManager.Index (0,0).nFirst = MAX_OGL_LIGHTS;
		lightManager.Index (0,0).nLast = 0;
		gameStates.render.bSpecularColor = 0;
		gameStates.render.nState = 0;
		float h = 1.0f / float (LM_W - 1);
		for (int32_t i = 0; i < LM_W; i++)
			m_data.nOffset [i] = i * h;
		m_data.nBlackLightmaps = 
		m_data.nWhiteLightmaps = 0; 
		//PLANE_DIST_TOLERANCE = fix (I2X (1) * 0.001f);
		//SetupSegments (); // set all faces up as triangles
		gameStates.render.bBuildLightmaps = 1;
		if (!gameStates.app.bPrecomputeLightmaps && gameStates.app.bProgressBars && gameOpts->menus.nStyle) {
			nFace = 0;
			ProgressBar (TXT_CALC_LIGHTMAPS, 1, 0, PROGRESS_STEPS (FACES.nFaces), CreatePoll);
			}
		else {
			if (!gameStates.app.bPrecomputeLightmaps)
				messageBox.Show (TXT_CALC_LIGHTMAPS);
			GrabMouse (0, 0);
			m_bSuccess = BuildAll (-1);
			GrabMouse (1, 0);
			if (!gameStates.app.bPrecomputeLightmaps)
				messageBox.Clear ();
			}
		if (!m_bSuccess)
			return 0;
		gameStates.render.bBuildLightmaps = 0;
		//PLANE_DIST_TOLERANCE = DEFAULT_PLANE_DIST_TOLERANCE;
		//SetupSegments (); // standard face setup (triangles or quads)
		//gameData.renderData.fAttScale [0] = (gameStates.render.bPerPixelLighting == 2) ? 1.0f : 2.0f;
		gameStates.render.bSpecularColor = 1;
		gameStates.render.bHaveLightmaps = 0;
		gameStates.render.nState = 0;
		gameOpts->render.color.nSaturation = nSaturation;
		Realloc ((m_list.nLightmaps + LIGHTMAP_BUFSIZE - 1) / LIGHTMAP_BUFSIZE);
		}
	else {
		CreateSpecial (m_data.m_texColor, 0, 0);
		m_list.nLightmaps = 1;
		for (int32_t i = 0; i < FACES.nFaces; i++)
			FACES.faces [i].m_info.nLightmap = 0;
		}
	BindAll ();
	Save (nLevel);
#if 1
	if (gameOpts->render.color.nLevel < 2)
		ToGrayScale ();
#	if 0
	if (gameStates.render.CartoonStyle ())
		Posterize ();
#	endif
#endif
	}
return m_bSuccess;
}

//------------------------------------------------------------------------------

int32_t CLightmapManager::Setup (int32_t nLevel)
{
if (gameStates.render.bPerPixelLighting || gameStates.app.bPrecomputeLightmaps) {
	if (!Create (nLevel))
		return 0;
	if (HaveLightmaps ())
		meshBuilder.RebuildLightmapTexCoord ();	//rebuild to create proper lightmap texture coordinates
	else
		gameOpts->render.bUseLightmaps = 0;
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t SetupLightmap (CSegFace* pFace)
{
#if DBG
if ((pFace->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (pFace->m_info.nSide == nDbgSide)))
	BRP;
#endif
int32_t i = pFace->m_info.nLightmap / LIGHTMAP_BUFSIZE;
if (!lightmapManager.Bind (i))
	return 0;
GLuint h = lightmapManager.Buffer (i)->handle;
#if 1 //!DBG
if (0 <= ogl.IsBound (h))
	return 1;
#endif
ogl.SelectTMU (GL_TEXTURE0, true);
ogl.SetTexturing (true);
//glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
ogl.BindTexture (h);
gameData.renderData.nStateChanges++;
return 1;
}

//------------------------------------------------------------------------------

