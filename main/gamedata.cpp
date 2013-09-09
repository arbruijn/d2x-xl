#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(__unix__) || defined(__macosx__)
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __macosx__
#	include "SDL/SDL_main.h"
#	include "SDL/SDL_keyboard.h"
#	include "FolderDetector.h"
#else
#	include "SDL_main.h"
#	include "SDL_keyboard.h"
#endif
#include "descent.h"
#include "u_mem.h"
#include "strutil.h"
#include "key.h"
#include "timer.h"
#include "error.h"
#include "segpoint.h"
#include "screens.h"
#include "texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "iff.h"
#include "pcx.h"
#include "args.h"
#include "hogfile.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "sdlgl.h"
#include "text.h"
#include "newdemo.h"
#include "objrender.h"
#include "renderthreads.h"
#include "network.h"
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "joy.h"
#include "desc_id.h"
#include "joydefs.h"
#include "gamepal.h"
#include "movie.h"
#include "compbit.h"
#include "playerprofile.h"
#include "tracker.h"
#include "rendermine.h"
#include "sphere.h"
#include "endlevel.h"
#include "interp.h"
#include "autodl.h"
#include "hiresmodels.h"
#include "soundthreads.h"
#include "gameargs.h"
#include "shrapnel.h"
#include "collide.h"
#include "lightcluster.h"
#include "multi.h"
#include "marker.h"
#if USE_DACS
#	include "dialheap.h"
#endif

// ----------------------------------------------------------------------------

void DefaultApplicationSettings (bool bSetup = false);
void DefaultCockpitSettings (bool bSetup = false);
void DefaultGameplaySettings (bool bSetup = false);
void DefaultKeyboardSettings (bool bSetup = false);
void DefaultMiscSettings (bool bSetup = false);
void DefaultPhysicsSettings (bool bSetup = false);
void DefaultRenderSettings (bool bSetup = false);
void DefaultSmokeSettings (bool bSetup = false);

// ----------------------------------------------------------------------------

void CGameData::Init (void)
{
#if USE_IRRLICHT
memset (&irrData, 0, sizeof (irrData));
#endif
InitEndLevelData ();
InitStringPool ();
SetDataVersion (-1);
}

// ----------------------------------------------------------------------------

CDynLight::CDynLight ()
{
info.visibleVertices = NULL;
Init ();
}

// ----------------------------------------------------------------------------

CDynLight::~CDynLight ()
{
Destroy ();
}

// ----------------------------------------------------------------------------

void CDynLight::Destroy (void)
{
if (info.visibleVertices) {
	delete info.visibleVertices;
	info.visibleVertices = NULL;
	}
for (int i = 0; i < MAX_THREADS; i++)
	render.activeLightsP [i] = NULL;
}

// ----------------------------------------------------------------------------

void CDynLight::Init ()
{
vDir.SetZero ();
color.SetZero ();
#if USE_OGL_LIGHTS
handle = 0;
fAttenuation.SetZero ();	// constant, linear quadratic
fAmbient.SetZero ();
fDiffuse.SetZero ();
#endif
fSpecular.SetZero ();
fEmissive.SetZero ();
bTransform = 0;
info.Init ();
//shadow.Init ();
}

// ----------------------------------------------------------------------------

void COglMaterial::Init (void)
{
#if 0 //using global default values instead
diffuse.SetZero ();
ambient.SetZero ();
#endif
specular.SetZero ();
emissive.SetZero ();
shininess = 0;
bValid = 0;
nLight = -1;
}

// ----------------------------------------------------------------------------

CLightRenderData::CLightRenderData ()
{
CLEAR (vPosf);
memset (xDistance, 0, sizeof (xDistance));
CLEAR (nVerts);
nTarget = 0;	//lit segment/face
nFrame = 0;
bShadow = 0;
bLightning = 0;
bExclusive = 0;
CLEAR (bUsed);
CLEAR (activeLightsP);
}

// ----------------------------------------------------------------------------

CDynLightData::CDynLightData ()
{
Init ();
}

// ----------------------------------------------------------------------------

void CDynLightData::Init (void)
{
nLights [0] =
nLights [1] = 0;
nVariable = 0;
nDynLights = 0;
nVertLights = 0;
nSegment = -1;
nTexHandle = 0;
nVariable = 0;
nThread = -1;
material.bValid = 0;
memset (nHeadlights, 0xff, sizeof (nHeadlights));
int i;
for (i = 0; i < MAX_THREADS; i++)
	active [i].Clear ();
for (i = 0; i < 2; i++)
	index [i].Clear ();
}

// ----------------------------------------------------------------------------

bool CDynLightData::Create (void)
{
CREATE (nearestSegLights, LEVEL_SEGMENTS * MAX_NEAREST_LIGHTS, 0xff);
CREATE (nearestVertLights, LEVEL_VERTICES * MAX_NEAREST_LIGHTS, 0xff);
CREATE (variableVertLights, LEVEL_VERTICES, 0xff);
CREATE (owners, LEVEL_OBJECTS, (char) 0xff);
for (int i = 0; i < MAX_THREADS; i++)
	CREATE (active [i], MAX_OGL_LIGHTS, 0);
return true;
}

// ----------------------------------------------------------------------------

void CDynLightData::Destroy (void)
{
DESTROY (nearestSegLights);
DESTROY (nearestVertLights);
DESTROY (variableVertLights);
DESTROY (owners);
int i;
for (i = 0; i < MAX_THREADS; i++)
	DESTROY (active [i]);
for (i = 0; i < 2; i++)
	index [i].Clear ();
for (i = 0; i < nLights [0]; i++)
	lights [i].Destroy ();
nLights [0] =
nLights [1] =
nLights [2] = 0;
}

// ----------------------------------------------------------------------------

CLightData::CLightData ()
{
Init ();
}

// ----------------------------------------------------------------------------

void CLightData::Init (void)
{
nStatic = 0;
nCoronas = 0;
bGotGlobalDynColor = 0;
bStartDynColoring = 0;
bInitDynColoring = 1;
memset (&globalDynColor, 0, sizeof (globalDynColor));
}

// ----------------------------------------------------------------------------

bool CLightData::Create (void)
{
if (!(/*gameStates.app.bNostalgia ||*/ lightManager.Create ()))
	return false;
CREATE (segDeltas, LEVEL_SEGMENTS * 6, 0);
CREATE (subtracted, LEVEL_SEGMENTS, 0);
CREATE (dynamicLight, LEVEL_VERTICES, 0);
CREATE (dynamicColor, LEVEL_VERTICES, 0);
CREATE (bGotDynColor, LEVEL_VERTICES, 0);
CREATE (vertices, LEVEL_VERTICES, 0);
CREATE (vertexFlags, LEVEL_VERTICES, 0);
CREATE (newObjects, LEVEL_OBJECTS, 0);
CREATE (objects, LEVEL_OBJECTS, 0);
CREATE (coronaQueries, MAX_OGL_LIGHTS, 0);
CREATE (coronaSamples, MAX_OGL_LIGHTS, 0);
Init ();
return true;
}

// ----------------------------------------------------------------------------

bool CLightData::Resize (void)
{
if (!(/*gameStates.app.bNostalgia ||*/ lightManager.Create ()))
	return false;
return dynamicLight.Resize (LEVEL_VERTICES) &&
		 dynamicColor.Resize (LEVEL_VERTICES) &&
		 bGotDynColor.Resize (LEVEL_VERTICES) &&
		 vertices.Resize (LEVEL_VERTICES) &&
		 vertexFlags.Resize (LEVEL_VERTICES);
}

// ----------------------------------------------------------------------------

void CLightData::Destroy (void)
{
DESTROY (segDeltas);
DESTROY (deltaIndices);
DESTROY (deltas);
DESTROY (subtracted);
DESTROY (dynamicLight);
DESTROY (dynamicColor);
DESTROY (bGotDynColor);
DESTROY (vertices);
DESTROY (vertexFlags);
DESTROY (newObjects);
DESTROY (objects);
DESTROY (coronaQueries);
DESTROY (coronaSamples);
flicker.Destroy ();
lightManager.Destroy ();
}

//------------------------------------------------------------------------------

CShadowData::CShadowData ()
{
}

// ----------------------------------------------------------------------------

void CShadowData::Init (void)
{
nLight = 0;
nLights = 0;
lightP = NULL;
nShadowMaps = 0;
memset (&lightSource, 0, sizeof (lightSource));
memset (&vLightPos, 0, sizeof (vLightPos));
CLEAR (vLightDir);
nFrame = 0;
}

// ----------------------------------------------------------------------------

bool CShadowData::Create (void)
{
if (!gameStates.app.bNostalgia) {
	CREATE (objLights, LEVEL_OBJECTS * MAX_SHADOW_LIGHTS, 0);
	}
Init ();
return true;
}

// ----------------------------------------------------------------------------

void CShadowData::Destroy (void)
{
DESTROY (objLights);
}

//------------------------------------------------------------------------------

CThrusterData::CThrusterData ()
{
fSpeed = 0;
nPulse = 0;
tPulse = 0;
}

//------------------------------------------------------------------------------

CVisibilityData::CVisibilityData ()
{
}

//------------------------------------------------------------------------------

bool CVisibilityData::Create (int nState)
{
if (nState == 0) {
	CREATE (points, LEVEL_VERTICES, 0);
	}
else {
	nVisited = 255;
	nProcessed = 255;
	nVisible = 255;
	CREATE (segments, LEVEL_SEGMENTS, 0);
	CREATE (bVisited, LEVEL_SEGMENTS, 0);
	CREATE (bVisible, LEVEL_SEGMENTS, 0);
	CREATE (bProcessed, LEVEL_SEGMENTS, 0);
	CREATE (nDepth, LEVEL_SEGMENTS, 0);
	for (int i = 0; i < 2; i++)
		CREATE (zRef [i], LEVEL_SEGMENTS, 0);
	CREATE (portals, LEVEL_SEGMENTS * 6, 0);
	CREATE (position, LEVEL_SEGMENTS, 0);
	}
return true;
}

//------------------------------------------------------------------------------

bool CVisibilityData::Resize (int nLength)
{
return points.Resize ((nLength < 0) ? LEVEL_VERTICES : nLength) != NULL;
}

//------------------------------------------------------------------------------

void CVisibilityData::Destroy (void)
{
DESTROY (segments);
DESTROY (bVisited);
DESTROY (bVisible);
DESTROY (bProcessed);
DESTROY (nDepth);
for (int i = 0; i < 2; i++)
	DESTROY (zRef [i]);
DESTROY (portals);
DESTROY (position);
DESTROY (points);
}

//------------------------------------------------------------------------------

CMineRenderData::CMineRenderData ()
{
}

//------------------------------------------------------------------------------

bool CMineRenderData::Create (int nState)
{
for (int i = 0, j = gameStates.app.nThreads + 2; i < j; i++)
	visibility [i].Create (nState);
if (nState == 1) {
	CREATE (objRenderSegList, LEVEL_SEGMENTS, 0);
	CREATE (renderFaceListP, FACES.nFaces, 0);
	CREATE (bObjectRendered, gameData.objs.nMaxObjects, 0);
	CREATE (bRenderSegment, LEVEL_SEGMENTS, 0);
	CREATE (nRenderObjList, gameData.objs.nMaxObjects, 0);
	CREATE (bCalcVertexColor, LEVEL_VERTICES, 0);
	CREATE (bAutomapVisited, LEVEL_SEGMENTS, 0);
	CREATE (bAutomapVisible, LEVEL_SEGMENTS, 0);
	CREATE (bRadarVisited, LEVEL_SEGMENTS, 0);
	}
return true;
}

//------------------------------------------------------------------------------

bool CMineRenderData::Resize (int nLength)
{
for (int i = 0, j = gameStates.app.nThreads + 2; i < j; i++)
	if (!visibility [i].Resize (nLength))
		return false;
return true;
}

//------------------------------------------------------------------------------

void CMineRenderData::Destroy (void)
{
for (int i = 0, j = gameStates.app.nThreads + 2; i < j; i++)
	visibility [i].Destroy ();
DESTROY (objRenderSegList);
DESTROY (renderFaceListP);
DESTROY (bObjectRendered);
DESTROY (bRenderSegment);
DESTROY (nRenderObjList);
DESTROY (bCalcVertexColor);
DESTROY (bAutomapVisited);
DESTROY (bAutomapVisible);
DESTROY (bRadarVisited);
}

//------------------------------------------------------------------------------

CRenderData::CRenderData ()
{
transpColor = DEFAULT_TRANSPARENCY_COLOR; //transparency color bitmap index
}

//------------------------------------------------------------------------------

void CRenderData::Init (void)
{
xFlashEffect = 0;
xTimeFlashLastPlayed = 0;
vertP = NULL;
zMin = 0;
zMax = 0;
dAspect = 0;
nTotalFaces = 0;
nTotalObjects = 0;
nTotalSprites = 0;
nTotalLights = 0;
nMaxLights = 0;
nColoredFaces = 0;
nStateChanges = 0;
nShaderChanges = 0;
nPowerupFilter = 0;
#if 1
fAttScale [0] = 0.05f;
fAttScale [1] = 0.005f;
#else
fAttScale [0] = 0.625f;
fAttScale [1] = 0.0625f;
#endif
faceList.Clear ();
}

// ----------------------------------------------------------------------------

CHUDMessage::CHUDMessage ()
{
memset (this, 0, sizeof (*this));
nColor = -1;
}

// ----------------------------------------------------------------------------

CHUDData::CHUDData ()
{
bPlayerMessage = 1;
}

// ----------------------------------------------------------------------------

bool CRiftData::Create (void)
{
#if OCULUS_RIFT
OVR::System::Init (OVR::Log::ConfigureDefaultLog (OVR::LogMask_All));
gameData.render.rift.m_bAvailable = 0;
m_managerP = *OVR::DeviceManager::Create();
if (m_managerP) {
	//m_managerP->SetMessageHandler(this);

	// Release Sensor/HMD in case this is a retry.
	m_sensorP.Clear ();
	m_hmdP.Clear ();
	// RenderParams.MonitorName.Clear();
	m_hmdP = *m_managerP->EnumerateDevices<OVR::HMDDevice> ().CreateDevice ();
	if (m_hmdP) {
		m_sensorP = *m_hmdP->GetSensor ();

		// This will initialize m_hmdInfo with information about configured IPD,
		// screen size and other variables needed for correct projection.
		// We pass HMD DisplayDeviceName into the renderer to select the
		// correct monitor in full-screen mode.
		if (m_hmdP->GetDeviceInfo (&m_hmdInfo))	{            
			// RenderParams.MonitorName = m_hmdInfo.DisplayDeviceName;
			// RenderParams.DisplayId = m_hmdInfo.DisplayId;
			m_stereoConfig.SetHMDInfo (m_hmdInfo);
			}
		}
	else {            
		// If we didn't detect an HMD, try to create the sensor directly.
		// This is useful for debugging sensor interaction; it is not needed in
		// a shipping app.
		m_sensorP = *m_managerP->EnumerateDevices<OVR::SensorDevice> ().CreateDevice ();
		}
	}
// If there was a problem detecting the Rift, display appropriate message.

const char* detectionMessage = NULL;

if (!m_managerP)
	detectionMessage = "Cannot initialize Oculus Rift system.";
if (!m_hmdP && !m_sensorP)
	detectionMessage = "Oculus Rift not detected.";
else if (!m_hmdP)
	detectionMessage = "Oculus Sensor detected; HMD Display not detected.\n";
else if (m_hmdInfo.DisplayDeviceName [0] == '\0')
	detectionMessage = "Oculus Sensor detected; HMD display EDID not detected.\n";
else if (!m_sensorP) {
	gameData.render.rift.m_bAvailable = 1;
	detectionMessage = "Oculus HMD Display detected; Sensor not detected.\n";
	}
else 
	gameData.render.rift.m_bAvailable = 2;
if (detectionMessage) 
	PrintLog (0, detectionMessage);
#endif
return gameData.render.rift.m_bAvailable;
}

// ----------------------------------------------------------------------------

void CRiftData::Destroy (void)
{
}

// ----------------------------------------------------------------------------

bool CRenderData::Create (void)
{
Destroy ();
CREATE (gameData.render.faceList, LEVEL_FACES, 0);
Init ();
return true;
}

// ----------------------------------------------------------------------------

void CRenderData::Destroy (void)
{
rift.Destroy ();
DESTROY (gameData.render.faceList);
faceIndex.Destroy ();
}

//------------------------------------------------------------------------------

CFaceData::CFaceData ()
{
}

//------------------------------------------------------------------------------

void CFaceData::Init (void)
{
slidingFaces = NULL;
vboDataHandle = 0;
vboIndexHandle = 0;
vertexP = NULL;
indexP = NULL;
nTriangles = 0;
nVertices = 0;
iVertices = 0;
iNormals = 0;
iColor = 0;
iTexCoord = 0;
iOvlTexCoord = 0;
iLMapTexCoord = 0;
}

// ----------------------------------------------------------------------------

bool CFaceData::Create (void)
{
Destroy ();

	int nScale = 1 << gameStates.render.nMeshQuality;

CREATE (faces, LEVEL_FACES, 0);
CREATE (tris, LEVEL_TRIANGLES, 0);
CREATE (vertices, LEVEL_TRIANGLES * 3, 0);
#if USE_RANGE_ELEMENTS
CREATE (vertIndex, LEVEL_TRIANGLES * 3, 0);
#endif
CREATE (faceVerts, LEVEL_FACES * 2 * nScale, 0);
CREATE (normals, LEVEL_TRIANGLES * 3 * 2, 0);
CREATE (color, LEVEL_TRIANGLES * 3, 0);
CREATE (texCoord, LEVEL_TRIANGLES * 2 * 2, 0);
CREATE (ovlTexCoord, LEVEL_TRIANGLES * 2 * 2, 0);
CREATE (lMapTexCoord, gameData.segs.nFaces * 3 * 2, 0);
Init ();
return true;
}

// ----------------------------------------------------------------------------

bool CFaceData::Resize (void)
{
	int nScale = 1 << gameStates.render.nMeshQuality;

RESIZE (faces, LEVEL_FACES);
RESIZE (tris, LEVEL_TRIANGLES);
RESIZE (vertices, LEVEL_TRIANGLES * 3);
#if USE_RANGE_ELEMENTS
RESIZE (vertIndex, LEVEL_TRIANGLES * 3);
#endif
RESIZE (faceVerts, LEVEL_FACES * 2 * nScale);
RESIZE (normals, LEVEL_TRIANGLES * 3 * 2);
RESIZE (color, LEVEL_TRIANGLES * 3);
RESIZE (texCoord, LEVEL_TRIANGLES * 2 * 2);
RESIZE (ovlTexCoord, LEVEL_TRIANGLES * 2 * 2);
RESIZE (lMapTexCoord, gameData.segs.nFaces * 3 * 2);
return true;
}

// ----------------------------------------------------------------------------

void CFaceData::Destroy (void)
{
DESTROY (faces);
DESTROY (tris);
DESTROY (vertices);
#if USE_RANGE_ELEMENTS
DESTROY (vertIndex);
#endif
DESTROY (faceVerts);
DESTROY (normals);
DESTROY (color);
DESTROY (texCoord);
DESTROY (ovlTexCoord);
DESTROY (lMapTexCoord);
}

//------------------------------------------------------------------------------

CFaceListIndex::CFaceListIndex ()
{
gameData.segs.nFaceKeys = -1;
}

//------------------------------------------------------------------------------

CFaceListIndex::~CFaceListIndex ()
{
Destroy ();
}

//------------------------------------------------------------------------------

void CFaceListIndex::Create (void)
{
Destroy (true);
roots.Create (gameData.segs.nFaceKeys); //((MAX_WALL_TEXTURES  + MAX_WALL_TEXTURES / 10) * 3);
tails.Create (gameData.segs.nFaceKeys); //((MAX_WALL_TEXTURES  + MAX_WALL_TEXTURES / 10) * 3);
usedKeys.Create (gameData.segs.nFaceKeys); //((MAX_WALL_TEXTURES  + MAX_WALL_TEXTURES / 10) * 3);
Init ();
}

//------------------------------------------------------------------------------

void CFaceListIndex::Destroy (bool bRebuild)
{
if (!bRebuild)
	gameData.segs.nFaceKeys = -1;
nUsedKeys = 0;
roots.Destroy ();
tails.Destroy ();
usedKeys.Destroy ();
}

//------------------------------------------------------------------------------

void CFaceListIndex::Init (void)
{
#if 1
roots.Clear (0xff);
tails.Clear (0xff);
usedKeys.Clear (0);
#endif
}

//------------------------------------------------------------------------------

CSegmentData::CSegmentData ()
{
nMaxSegments = MAX_SEGMENTS_D2X;
nLevelVersion = 0;
nVertices = 0;
nSegments = 0;
vMin.SetZero ();
vMax.SetZero ();
CLEAR (szLevelFilename);
}

//------------------------------------------------------------------------------

void CSegmentData::Init (void)
{
fRad = 0;
nFaceVerts = 0;
nLastVertex = 0;
nLastSegment = 0;
nFaces = 0;
bHaveSlideSegs = 0;
nSlideSegs = 0;
}

// ----------------------------------------------------------------------------

bool CSegmentData::Create (int nSegments, int nVertices)
{
	int i;

this->nSegments = nSegments;
this->nVertices = nVertices;
CREATE (vertices, LEVEL_VERTICES, 0);
CREATE (fVertices, LEVEL_VERTICES, 0);
CREATE (segments, LEVEL_SEGMENTS, 0);
#if CALC_SEGRADS
CREATE (segRads [0], LEVEL_SEGMENTS, 0);
CREATE (segRads [1], LEVEL_SEGMENTS, 0);
CREATE (extent, LEVEL_SEGMENTS, 0);
#endif
CREATE (segCenters [0], LEVEL_SEGMENTS, 0);
CREATE (segCenters [1], LEVEL_SEGMENTS, 0);
CREATE (sideCenters, LEVEL_SEGMENTS * 6, 0);
// the following two arrays are bit arrays; (X + 7) / 8 ensures enough bytes are allocated for all bits
// bSegVis [0] is a triangular matrix, x <= y always
CREATE (bSegVis, SegVisSize (), 0);
CREATE (segDistTable, LEVEL_SEGMENTS, 0);
CREATE (slideSegs, LEVEL_SEGMENTS, 0);
CREATE (segFaces, LEVEL_SEGMENTS, 0);
for (i = 0; i < LEVEL_SEGMENTS; i++)
	segments [i].m_objects = -1;
Init ();
#if 0
#	if BIDIRECTIONAL_DACS
dialHeaps [0].Create (gameData.segs.nSegments);
dialHeaps [1].Create (gameData.segs.nSegments);
#	else
dialHeap.Create (gameData.segs.nSegments);
#	endif
#endif
return true; //faces.Create ();
}

// ----------------------------------------------------------------------------

bool CSegmentData::Resize (void)
{
return gameData.render.mine.Resize () &&
		 gameData.segs.vertices.Resize (LEVEL_VERTICES) &&
		 gameData.segs.fVertices.Resize (LEVEL_VERTICES);
}

// ----------------------------------------------------------------------------

void CSegmentData::Destroy (void)
{
DESTROY (gameData.segs.vertices);
DESTROY (gameData.segs.fVertices);
DESTROY (SEGMENTS);
#if CALC_SEGRADS
DESTROY (gameData.segs.segRads [0]);
DESTROY (gameData.segs.segRads [1]);
DESTROY (gameData.segs.extent);
#endif
DESTROY (gameData.segs.segCenters [0]);
DESTROY (gameData.segs.segCenters [1]);
DESTROY (gameData.segs.sideCenters);
DESTROY (gameData.segs.bSegVis);
DESTROY (gameData.segs.bLightVis);
for (int i = 0; i < LEVEL_SEGMENTS; i++)
	gameData.segs.segDistTable [i].Destroy ();
DESTROY (gameData.segs.segDistTable);
DESTROY (gameData.segs.slideSegs);
DESTROY (gameData.segs.segFaces);
gameData.segs.grids [0].Destroy ();
gameData.segs.grids [1].Destroy ();
nSegments = 0;
nFaces = 0;
faces.Destroy ();
#if 0
#	if BIDIRECTIONAL_DACS
dialHeaps [0].Destroy ();
dialHeaps [1].Destroy ();
#	else
dialHeap.Destroy ();
#	endif
#endif
}

// ----------------------------------------------------------------------------

inline fix Ceil (fix v);

inline fix Floor (fix v)
{
return (v >= 0) ? v & 0xFFFF0000 : -Ceil (-v);
}

// ----------------------------------------------------------------------------

inline fix Ceil (fix v)
{
return (v >= 0) ? Floor (v + 0xFFFF) : -Floor (-v);
}

// ----------------------------------------------------------------------------

inline CFixVector& Floor (CFixVector& v)
{
v.v.coord.x = Floor (v.v.coord.x);
v.v.coord.y = Floor (v.v.coord.y);
v.v.coord.z = Floor (v.v.coord.z);
return v;
}

// ----------------------------------------------------------------------------

inline CFixVector& Ceil (CFixVector& v)
{
v.v.coord.x = Ceil (v.v.coord.x);
v.v.coord.y = Ceil (v.v.coord.y);
v.v.coord.z = Ceil (v.v.coord.z);
return v;
}

// ----------------------------------------------------------------------------

inline CFixVector& ToInt (CFixVector& v)
{
v.v.coord.x = X2I (v.v.coord.x);
v.v.coord.y = X2I (v.v.coord.y);
v.v.coord.z = X2I (v.v.coord.z);
return v;
}

// ----------------------------------------------------------------------------

inline int CSegmentGrid::GridIndex (int x, int y, int z)
{
return z * m_vDim.v.coord.y * m_vDim.v.coord.x + y * m_vDim.v.coord.x + x;
}

// ----------------------------------------------------------------------------

#define GRID_INDEX(_x,_y,_z)	(((_z) * m_vDim.v.coord.y + (_y)) * m_vDim.v.coord.x + (_x))

bool CSegmentGrid::Create (int nGridSize, int bSkyBox)
{
	CSegment*		segP;
	tSegGridIndex*	indexP;
	CFixVector		v0, v1;
	int				size, i, j, x, y, z;

Destroy ();
m_vMin.Set (0x7fffffff, 0x7fffffff, 0x7fffffff);
m_vMax.Set (-0x7fffffff, -0x7fffffff, -0x7fffffff);
segP = SEGMENTS.Buffer ();
for (i = gameData.segs.nSegments, j = 0; i; i--, segP++) {
	if ((segP->m_function == SEGMENT_FUNC_SKYBOX) == bSkyBox) {
		j++;
		v0 = segP->m_extents [0];
		v1 = segP->m_extents [1];
		if (m_vMin.v.coord.x > v0.v.coord.x)
			m_vMin.v.coord.x = v0.v.coord.x;
		if (m_vMin.v.coord.y > v0.v.coord.y)
			m_vMin.v.coord.y = v0.v.coord.y;
		if (m_vMin.v.coord.z > v0.v.coord.z)
			m_vMin.v.coord.z = v0.v.coord.z;
		if (m_vMax.v.coord.x < v1.v.coord.x)
			m_vMax.v.coord.x = v1.v.coord.x;
		if (m_vMax.v.coord.y < v1.v.coord.y)
			m_vMax.v.coord.y = v1.v.coord.y;
		if (m_vMax.v.coord.z < v1.v.coord.z)
			m_vMax.v.coord.z = v1.v.coord.z;
		}
	}

if (!j)
	return true;

m_nGridSize = nGridSize;
m_vDim = m_vMax - m_vMin;
m_vDim /= I2X (m_nGridSize);
ToInt (Ceil (m_vDim));
size = ++m_vDim.v.coord.x * ++m_vDim.v.coord.y * ++m_vDim.v.coord.z;

if (!m_index.Create (size))
	return false;
m_index.Clear ();

segP = SEGMENTS.Buffer ();
for (i = gameData.segs.nSegments; i; i--, segP++) {
	if ((segP->m_function == SEGMENT_FUNC_SKYBOX) == bSkyBox) {
#if DBG
		if (segP - SEGMENTS.Buffer () == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		v0 = segP->m_extents [0] - m_vMin;
		v1 = segP->m_extents [1] - m_vMin;
		v0 /= I2X (m_nGridSize);
		v1 /= I2X (m_nGridSize);
		ToInt (Floor (v0));
		ToInt (Floor (v1));
		for (z = v0.v.coord.z; z <= v1.v.coord.z; z++) {
			for (y = v0.v.coord.y; y <= v1.v.coord.y; y++) {
				indexP = &m_index [GridIndex (v0.v.coord.x, y, z)];
				for (x = v0.v.coord.x; x <= v1.v.coord.x; x++, indexP++) {
					indexP->nSegments++;
					}
				}
			}
		}
	}

#if DBG
int h = 0, k = 0;
#endif
for (i = j = 0; i < size; i++) {
	m_index [i].nIndex = j;
#if DBG
	if (h < m_index [i].nSegments)
		h = m_index [k = i].nSegments;
#endif
	j += m_index [i].nSegments;
	m_index [i].nSegments = 0;
	}

if (!m_segments.Create (j)) {
	m_index.Destroy ();
	return false;
	}

segP = SEGMENTS.Buffer ();
for (i = 0; i < gameData.segs.nSegments; i++, segP++) {
	if ((segP->m_function == SEGMENT_FUNC_SKYBOX) == bSkyBox) {
#if DBG
		if (segP - SEGMENTS.Buffer () == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		v0 = segP->m_extents [0] - m_vMin;
		v1 = segP->m_extents [1] - m_vMin;
		v0 /= I2X (m_nGridSize);
		v1 /= I2X (m_nGridSize);
		ToInt (Floor (v0));
		ToInt (Floor (v1));
		for (z = v0.v.coord.z; z <= v1.v.coord.z; z++) {
			for (y = v0.v.coord.y; y <= v1.v.coord.y; y++) {
				indexP = &m_index [GridIndex (v0.v.coord.x, y, z)];
				for (x = v0.v.coord.x; x <= v1.v.coord.x; x++, indexP++)
					m_segments [indexP->nIndex + indexP->nSegments++] = i;
				}
			}
		}
	}

return true;
}

// ----------------------------------------------------------------------------

void CSegmentGrid::Destroy (void)
{
m_index.Destroy ();
m_segments.Destroy ();
}

// ----------------------------------------------------------------------------

int CSegmentGrid::GetSegList (CFixVector vPos, short*& listP)
{
if (!Available ())
	return -1;
vPos -= m_vMin;
vPos /= I2X (m_nGridSize);
ToInt (Floor (vPos));
int i = GRID_INDEX (vPos.v.coord.x, vPos.v.coord.y, vPos.v.coord.z);
if ((i < 0) || (i >= int (m_index.Length ()))) {
	listP = NULL;
	return 0;
	}
listP = &m_segments [m_index [i].nIndex];
return m_index [i].nSegments;
}

// ----------------------------------------------------------------------------

CWallData::CWallData ()
{
exploding.Create (MAX_EXPLODING_WALLS);
activeDoors.Create (MAX_DOORS);
cloaking.Create (MAX_CLOAKING_WALLS);
for (int i = 0; i < 2; i++)
	anims [i].Create (MAX_WALL_ANIMS);
bitmaps.Create (MAX_WALL_ANIMS);
nWalls = 0;
CLEAR (nAnims);
}

//------------------------------------------------------------------------------

void CWallData::Reset (void)
{
exploding.Reset ();
cloaking.Reset ();
activeDoors.Reset ();
}

//------------------------------------------------------------------------------

void CWallData::Destroy (void)
{
walls.Destroy ();
nWalls = 0;
}

//------------------------------------------------------------------------------

CTriggerData::CTriggerData ()
{
}

// ----------------------------------------------------------------------------

bool CTriggerData::Create (int nTriggers, bool bObjTriggers)
{
if (bObjTriggers) {
	objTriggers.Create (nTriggers);
#if 0
	objTriggerRefs.Create (MAX_OBJ_TRIGGERS);
	objTriggerRefs.Clear (0xff);
#endif
	m_nObjTriggers = nTriggers;
	}
else {
#if 0
	CREATE (firstObjTrigger, LEVEL_OBJECTS, 0xff);
#else
	CREATE (objTriggerRefs, LEVEL_OBJECTS, 0);
#endif
	triggers.Create (nTriggers);
	delay.Create (nTriggers);
	delay.Clear (0xff);
	m_nTriggers = nTriggers;
	}
return true;
}

// ----------------------------------------------------------------------------

void CTriggerData::Destroy (void)
{
#if 0
firstObjTrigger.Destroy ();
#endif
triggers.Destroy ();
objTriggers.Destroy ();
objTriggerRefs.Destroy ();
delay.Destroy ();
m_nTriggers = 0;
m_nObjTriggers = 0;
}

// ----------------------------------------------------------------------------

CEffectData::CEffectData ()
{
for (int i = 0; i < 2; i++) {
	effects [i].Create (MAX_EFFECTS);
	vClips [i].Create (MAX_VCLIPS);
	}
}

// ----------------------------------------------------------------------------

CSoundData::CSoundData ()
{
for (int i = 0; i < 2; i++)
	sounds [i].Create (MAX_SOUND_FILES); //[MAX_SOUND_FILES];
sounds [0].ShareBuffer (soundP);
}

// ----------------------------------------------------------------------------

CTextureData::CTextureData ()
{
for (int i = 0; i < 2; i++) {
	bitmapFiles [i].Create (MAX_BITMAP_FILES);
	bitmapFlags [i].Create (MAX_BITMAP_FILES);
	bitmaps [i].Create (MAX_BITMAP_FILES);
	altBitmaps [i].Create (MAX_BITMAP_FILES);
	bmIndex [i].Create (MAX_TEXTURES + MAX_TEXTURES / 10);
	bmIndex [i].Clear ();
	textureIndex [i].Create (MAX_BITMAP_FILES);
	tMapInfo [i].Create (MAX_TEXTURES + MAX_TEXTURES / 10);	//add some room for extra textures like e.g. from the hoard data
	tMapInfo [i].Clear ();
	defaultBrightness [i].Create (MAX_WALL_TEXTURES);
	defaultBrightness [i].Clear ();
	}
objBmIndex.Create (MAX_OBJ_BITMAPS);
defaultObjBmIndex.Create (MAX_OBJ_BITMAPS);
addonBitmaps.Create (MAX_ADDON_BITMAP_FILES);
bitmapXlat.Create (MAX_BITMAP_FILES);
aliases.Create (MAX_ALIASES);
objBmIndexP.Create (MAX_OBJ_BITMAPS);
cockpitBmIndex.Create (N_COCKPIT_BITMAPS);
bitmapColors.Create (MAX_BITMAP_FILES);
brightness.Create (MAX_WALL_TEXTURES);
brightness.Clear ();

bitmaps [0].ShareBuffer (bitmapP);
altBitmaps [0].ShareBuffer (altBitmapP);
bmIndex [0].ShareBuffer (bmIndexP);
bitmapFiles [0].ShareBuffer (bitmapFileP);
tMapInfo [0].ShareBuffer (tMapInfoP);
gameData.pig.tex.nFirstMultiBitmap = -1;
nObjBitmaps = 0;
bPageFlushed = 0;
nExtraBitmaps = 0;
nAliases = 0;
nHamFileVersion = 0;
nFirstMultiBitmap = 0;
CLEAR (nBitmaps);
CLEAR (nTextures);
}

//------------------------------------------------------------------------------

CShipData::CShipData ()
{
player = &only;
}

//------------------------------------------------------------------------------

CProducerData::CProducerData ()
{
xFuelRefillSpeed = I2X (1);
xFuelGiveAmount = I2X (25);
xFuelMaxAmount = I2X (100);
xEnergyToCreateOneRobot = I2X (1);
nProducers = 0;
nRobotMakers = 0;
nEquipmentMakers = 0;
nRepairCenters = 0;
}

//------------------------------------------------------------------------------

CRobotData::CRobotData ()
{
for (int i = 0; i < 2; i++) {
	info [i].Create (MAX_ROBOT_TYPES);
	info [i].Clear (0xff);
	}
defaultInfo.Create (MAX_ROBOT_TYPES);
defaultInfo.Clear (0xff);
joints.Create (MAX_ROBOT_JOINTS);
joints.Clear (0xff);
defaultJoints.Create (MAX_ROBOT_JOINTS);
defaultJoints.Clear (0xff);
CLEAR (robotNames);
nJoints = 0;
nDefaultJoints = 0;
nCamBotId = -1;
nCamBotModel = -1;
nDefaultTypes = 0;
bReplacementsLoaded = 0;
CLEAR (nTypes);
}

//------------------------------------------------------------------------------

CReactorData::CReactorData ()
{
for (int i = 0; i < MAX_BOSS_COUNT; i++)
	states [i].nDeadObj = -1;
}

// ----------------------------------------------------------------------------

CEscortData::CEscortData ()
{
memset (this, 0, sizeof (*this));
nMaxLength = 1000;
nKillObject = -1;
nGoalObject = ESCORT_GOAL_UNSPECIFIED;
nSpecialGoal = -1;
nGoalIndex = -1;
strcpy (szName, "GUIDE-BOT");
strcpy (szRealName, "GUIDE-BOT");
}

// ----------------------------------------------------------------------------

CThiefData::CThiefData ()
{
nStolenItem = 0;
xReInitTime = 0x3f000000;
for (int i = 0; i < NDL; i++)
	xWaitTimes [i] = I2X (30 - i * 5);
}

 // ----------------------------------------------------------------------------

CModelData::CModelData ()
{
nLoresModels = 0;
nHiresModels = 0;
nPolyModels = 0;
nDefPolyModels = 0;
nCockpits = 0;
nLightScale = 0;
nSimpleModelThresholdScale = 5;
nMarkerModel = -1;
vScale.SetZero ();
Create ();
strcpy (szShipModels [0], "pyrogl.ase");
strcpy (szShipModels [1], "phantomxl.ase");
strcpy (szShipModels [2], "pyrogl.ase");
}

// ----------------------------------------------------------------------------

bool CModelData::Create (void)
{
	int i;

for (i = 0; i < 2; i++) {
	aseModels [i].Create (MAX_POLYGON_MODELS);
	oofModels [i].Create (MAX_POLYGON_MODELS);
	for (int j = 0; j < 2; j++)
		pofData [i][j].Create (MAX_POLYGON_MODELS);
	CREATE (modelToOOF [i], MAX_POLYGON_MODELS, 0);
	CREATE (modelToASE [i], MAX_POLYGON_MODELS, 0);
	renderModels [i].Create (MAX_POLYGON_MODELS);
	}
CREATE (bHaveHiresModel, MAX_POLYGON_MODELS, 0);
for (i = 0; i < 3; i++) {
	polyModels [i].Create (MAX_POLYGON_MODELS);
	for (int j = 0; j < MAX_POLYGON_MODELS; j++)
		polyModels [i][j].SetKey (j);
	}
CREATE (modelToPOL, MAX_POLYGON_MODELS, 0);
CREATE (polyModelPoints, MAX_POLYGON_VERTS, 0);
for (i = 0; i < MAX_POLYGON_VERTS; i++)
	polyModelPoints [i].SetIndex (-1);
CREATE (fPolyModelVerts, MAX_POLYGON_VERTS, 0);
textures.Create (MAX_POLYOBJ_TEXTURES);
CREATE (textureIndex, MAX_POLYOBJ_TEXTURES, 0xff);
CREATE (nDyingModels, MAX_POLYGON_MODELS, 0xff);
CREATE (nDeadModels, MAX_POLYGON_MODELS, 0xff);
CREATE (offsets, MAX_POLYGON_MODELS, 0);
CREATE (gunInfo, MAX_POLYGON_MODELS, 0);
CREATE (spheres, MAX_POLYGON_MODELS, 0);
return true;
}

//------------------------------------------------------------------------------

void CModelData::Destroy (void)
{
	int	h, i;

PrintLog (1, "unloading polygon model data\n");
for (h = 0; h < 2; h++) {
	for (i = 0; i < MAX_POLYGON_MODELS; i++) {
#if DBG
		if ((nDbgModel >= 0) && (i == nDbgModel))
			nDbgModel = nDbgModel;
#endif
		renderModels [h][i].Destroy ();
		gameData.models.pofData [h][0][i].Destroy ();
		gameData.models.pofData [h][1][i].Destroy ();
		}
	}
PrintLog (-1);
}

// ----------------------------------------------------------------------------

CMorphData::CMorphData ()
{
gameData.render.morph.xRate = MORPH_RATE;
}

// ----------------------------------------------------------------------------

CCockpitData::CCockpitData ()
{
for (int i = 0; i < 2; i++)
	if (gauges [i].Create (MAX_GAUGE_BMS))
		gauges [i].Clear (0xff);
}

//------------------------------------------------------------------------------

void InitWeaponFlags (void)
{
}

//------------------------------------------------------------------------------

void InitIdToOOF (void)
{
gameData.objs.idToOOF.Clear (0);
gameData.objs.idToOOF [MEGAMSL_ID] = OOF_MEGA;
}

// ----------------------------------------------------------------------------

CObjectData::CObjectData ()
{
InitWeaponFlags ();
CollideInit ();
InitIdToOOF ();
}

// ----------------------------------------------------------------------------

void CObjectData::Init (void)
{
memset (&color, 0, sizeof (color));
CLEAR (nLastObject);
CLEAR (guidedMissile);
CLEAR (trackGoals);
bWantEffect.Clear ();
consoleP = NULL;
viewerP = NULL;
missileViewerP = NULL;
deadPlayerCamera = NULL;
endLevelCamera = NULL;
nFirstDropped = 0;
nLastDropped = 0;
nFreeDropped = 0;
nDropped = 0;
nObjects = 0;
nObjectLimit = 0;
nNextSignature = 1;
nChildFreeList = 0;
nDrops = 0;
nDeadControlCenter = 0;
nVertigoBotFlags = 0;
nFrameCount = 0;
nEffects = 0;
nMaxObjects = max (gameFileInfo.objects.count + 1000, gameFileInfo.objects.count * 2);
nMaxUsedObjects = LEVEL_OBJECTS - 20;
}

// ----------------------------------------------------------------------------

void CObjectData::InitFreeList (void)
{
for (int i = 0; i < LEVEL_OBJECTS; i++) {
	gameData.objs.freeList [i] = i;
	OBJECTS [i].Init ();
	}
}

// ----------------------------------------------------------------------------

bool CObjectData::Create (void)
{
Init ();
CREATE (gameData.objs.objects, LEVEL_OBJECTS, 0);
CREATE (gameData.objs.update, LEVEL_OBJECTS, 0);
CREATE (gameData.objs.freeList, LEVEL_OBJECTS, 0);
CREATE (gameData.objs.parentObjs, LEVEL_OBJECTS, (char) 0xff);
CREATE (gameData.objs.childObjs, LEVEL_OBJECTS, 0);
CREATE (gameData.objs.firstChild, LEVEL_OBJECTS, (char) 0xff);
CREATE (gameData.objs.init, LEVEL_OBJECTS, 0);
CREATE (gameData.objs.dropInfo, LEVEL_OBJECTS, 0);
CREATE (gameData.objs.speedBoost, LEVEL_OBJECTS, 0);
CREATE (gameData.objs.vRobotGoals, LEVEL_OBJECTS, 0);
CREATE (gameData.objs.xLastAfterburnerTime, LEVEL_OBJECTS, 0);
CREATE (gameData.objs.xLight, LEVEL_OBJECTS, 0);
CREATE (gameData.objs.nLightSig, LEVEL_OBJECTS, 0);
CREATE (gameData.objs.nHitObjects, LEVEL_OBJECTS * MAX_HIT_OBJECTS, 0);
CREATE (gameData.objs.viewData, LEVEL_OBJECTS, (char) 0xFF);
CREATE (gameData.objs.bWantEffect, LEVEL_OBJECTS, (char) 0);
InitFreeList ();
memset (&gameData.objs.lists, 0, sizeof (gameData.objs.lists));
return gameData.render.mine.Create (0) && lightClusterManager.Init () && shrapnelManager.Init ();
}

// ----------------------------------------------------------------------------

void CObjectData::Destroy (void)
{
DESTROY (gameData.objs.objects);
DESTROY (gameData.objs.effects);
DESTROY (gameData.objs.freeList);
DESTROY (gameData.objs.parentObjs);
DESTROY (gameData.objs.childObjs);
DESTROY (gameData.objs.firstChild);
DESTROY (gameData.objs.init);
DESTROY (gameData.objs.dropInfo);
DESTROY (gameData.objs.speedBoost);
DESTROY (gameData.objs.vRobotGoals);
DESTROY (gameData.objs.xLastAfterburnerTime);
DESTROY (gameData.objs.xLight);
DESTROY (gameData.objs.nLightSig);
DESTROY (gameData.objs.nHitObjects);
DESTROY (gameData.objs.viewData);
DESTROY (gameData.objs.bWantEffect);
memset (&gameData.objs.lists, 0, sizeof (gameData.objs.lists));
nObjects = 0;
lightManager.Reset ();
shrapnelManager.Reset ();
}

//------------------------------------------------------------------------------

void CObjectData::GatherEffects (void)
{
if (nEffects && effects.Create (nEffects)) {
	int i, j;
	for (i = j = 0; i < gameFileInfo.objects.count; i++) {
		if (OBJECTS [i].info.nType == OBJ_EFFECT) {
			effects [j] = OBJECTS [i];
			effects [j].info.nPrevInSeg = effects [j].info.nNextInSeg = -1;
			j++;
			}
		}
	nEffects = j;
	}
else
	nEffects = 0;
}

//------------------------------------------------------------------------------

int CObjectData::RebuildEffects (void)
{
	int j = 0;

if (nEffects && effects.Buffer ()) {
	for (int i = 0; i < nEffects; i++) {
		tBaseObject& bo = effects [i];
		int nObject = CreateObject (bo.info.nType, bo.info.nId, -1,
											 -bo.info.nSegment - 2, bo.info.position.vPos, bo.info.position.mOrient,
											 bo.info.xSize, bo.info.controlType, bo.info.movementType, bo.info.renderType);
		if (nObject >= 0) {
			OBJECTS [nObject].info = bo.info;
			OBJECTS [nObject].mType = bo.mType;
			OBJECTS [nObject].cType = bo.cType;
			OBJECTS [nObject].rType = bo.rType;
			if (OBJECTS [nObject].info.nId == SMOKE_ID)
				OBJECTS [nObject].SetupSmoke ();
			j++;
			}
		}
	}
return j;
}

// ----------------------------------------------------------------------------

CColorData::CColorData ()
{
if (textures.Create (MAX_WALL_TEXTURES))
	textures.Clear ();
for (int i = 0; i < 2; i++)
	if (defaultTextures [i].Create (MAX_WALL_TEXTURES))
		defaultTextures [i].Clear ();
}

// ----------------------------------------------------------------------------

bool CColorData::Create (void)
{
CREATE (lights, LEVEL_SEGMENTS * 6, 0);
CREATE (sides, LEVEL_SEGMENTS * 6, 0);
CREATE (segments, LEVEL_SEGMENTS, 0);
CREATE (vertices, LEVEL_VERTICES, 0);
CREATE (vertBright, LEVEL_VERTICES, 0);
CREATE (ambient, LEVEL_VERTICES, 0);	//static light values
CREATE (visibleLights, LEVEL_SEGMENTS * 6, 0);
return true;
}

// ----------------------------------------------------------------------------

bool CColorData::Resize (void)
{
return vertices.Resize (LEVEL_VERTICES) &&
		 vertBright.Resize (LEVEL_VERTICES) &&
		 ambient.Resize (LEVEL_VERTICES);
}

// ----------------------------------------------------------------------------

void CColorData::Destroy (void)
{
DESTROY (lights);
DESTROY (sides);
DESTROY (segments);
DESTROY (vertices);
DESTROY (vertBright);
DESTROY (ambient);	//static light values
DESTROY (visibleLights);
}

 // ----------------------------------------------------------------------------

CFVISideData::CFVISideData ()
{
memset (this, 0, sizeof (*this));
bCache = 1;
}

 // ----------------------------------------------------------------------------

CPhysicsData::CPhysicsData ()
{
Init ();
}

// ----------------------------------------------------------------------------

void CPhysicsData::Init (void)
{
xTime = 0;
xAfterburnerCharge = I2X (1);
xBossInvulDot = 0;
playerThrust.SetZero ();
segments.Clear (0xff);
nSegments = 0;
bIgnoreObjFlag = 0;
}

// ----------------------------------------------------------------------------

bool CPhysicsData::Create (void)
{
return true;
}

// ----------------------------------------------------------------------------

void CPhysicsData::Destroy (void)
{
}

// ----------------------------------------------------------------------------

CWeaponData::CWeaponData ()
{
nPrimary = 0;
nSecondary = 0;
nOverridden = 0;
bTripleFusion = 0;
CLEAR (firing);
CLEAR (nTypes);
}

// ----------------------------------------------------------------------------

bool CWeaponData::Create (void)
{
CREATE (color, LEVEL_OBJECTS, 0);
for (int i = 0; i < LEVEL_OBJECTS; i++)
	color [i].Set (1.0f, 1.0f, 1.0f, 1.0f);
return true;
}

// ----------------------------------------------------------------------------

void CWeaponData::Destroy (void)
{
color.Destroy ();
}

// ----------------------------------------------------------------------------

COmegaData::COmegaData ()
{
memset (this, 0, sizeof (*this));
xCharge [0] =
xCharge [1] =
xMaxCharge = DEFAULT_MAX_OMEGA_CHARGE;
}

// ----------------------------------------------------------------------------

CMultiplayerData::CMultiplayerData ()
{
nPlayers = 1;
nMaxPlayers = -1;
nLocalPlayer = 0;
nPlayerPositions = -1;
bMoving = 0;
xStartAbortMenuTime = 0;
CLEAR (playerInit);
CLEAR (nVirusCapacity);
CLEAR (nLastHitTime);
CLEAR (bWasHit);
CLEAR (bulletEmitters);
CLEAR (gatlingSmoke);
powerupsInMine.Clear ();
powerupsOnShip.Clear ();
maxPowerupsAllowed.Clear ();
}

// ----------------------------------------------------------------------------

bool CMultiplayerData::Create (void)
{
CREATE (gameData.multiplayer.leftoverPowerups, LEVEL_OBJECTS, 0);
return true;
}

// ----------------------------------------------------------------------------

void CMultiplayerData::Destroy (void)
{
DESTROY (gameData.multiplayer.leftoverPowerups);
}

// ----------------------------------------------------------------------------

CMultiGameData::CMultiGameData ()
{
nWhoKilledCtrlcen = 0;
bShowReticleName = 0;
bIsGuided = 0;
bQuitGame = 0;
bGotoSecret = 0;
nTypingTimeout = 0;
}

// ----------------------------------------------------------------------------

bool CMultiGameData::Create (void)
{
CREATE (gameData.multigame.remoteToLocal, MAX_NUM_NET_PLAYERS * LEVEL_OBJECTS, 0xff);  // Remote CObject number for each local CObject
CREATE (gameData.multigame.localToRemote, LEVEL_OBJECTS, 0xff);
CREATE (gameData.multigame.nObjOwner, LEVEL_OBJECTS, 0xff);   // Who created each CObject in my universe, -1 = loaded at start
return true;
}

// ----------------------------------------------------------------------------

void CMultiGameData::Destroy (void)
{
DESTROY (gameData.multigame.remoteToLocal);  // Remote CObject number for each local CObject
DESTROY (gameData.multigame.localToRemote);
DESTROY (gameData.multigame.nObjOwner);   // Who created each CObject in my universe, -1 = loaded at start
}

// ----------------------------------------------------------------------------

CAIData::CAIData ()
{
gameData.ai.bInitialized = 0;
gameData.ai.bEvaded = 0;
gameData.ai.bEnableAnimation = 1;
gameData.ai.bInfoEnabled = 0;
gameData.ai.nAwarenessEvents = 0;
gameData.ai.target.nDistToLastPosFiredAt = 0;
cloakInfo.Create (MAX_AI_CLOAK_INFO);
awarenessEvents.Create (MAX_AWARENESS_EVENTS);
gameData.ai.freePointSegs = gameData.ai.routeSegs.Buffer ();
}

// ----------------------------------------------------------------------------

bool CAIData::Create (void)
{
CREATE (gameData.ai.localInfo, LEVEL_OBJECTS, 0);
CREATE (routeSegs, LEVEL_POINT_SEGS, 0);
return true;
}

// ----------------------------------------------------------------------------

void CAIData::Destroy (void)
{
DESTROY (gameData.ai.localInfo);
DESTROY (routeSegs);
}

// ----------------------------------------------------------------------------

bool CDemoData::Create (void)
{
CREATE (gameData.demo.bWasRecorded,  LEVEL_OBJECTS, 0);
CREATE (gameData.demo.bViewWasRecorded, LEVEL_OBJECTS, 0);
return true;
}

// ----------------------------------------------------------------------------

void CDemoData::Destroy (void)
{
DESTROY (gameData.demo.bWasRecorded);
DESTROY (gameData.demo.bViewWasRecorded);
}

// ----------------------------------------------------------------------------

CMissionData::CMissionData ()
{
memset (this, 0, sizeof (*this));
nCurrentLevel = 0;
nLastMission = -1;
nLastLevel = -1;
nLastSecretLevel = -1;
nEntryLevel = -1;
strcpy (szBriefingFilename [1], "endreg.txt");
strcpy (szBriefingFilename [0], "briefing.txt");
}

// ----------------------------------------------------------------------------

CScoreData::CScoreData ()
{
nKillsChanged = 0;
bNoMovieMessage = 0;
nHighscore = 0;
bWaitingForOthers = 0;
nChampion = -1;
}

// ----------------------------------------------------------------------------

CTimeData::CTimeData ()
{
memset (this, 0, sizeof (*this));
gameData.time.xMaxOnline = 180000;
gameData.time.xGameStart = -gameData.time.xMaxOnline;
xFrame = 0x1000;
xLastThiefHitTime = 0;
}

// ----------------------------------------------------------------------------

CTerrainRenderData::CTerrainRenderData ()
{
bOutline = 0;
nGridW = nGridH = 0;
orgI = orgJ = 0;
nMineTilesDrawn = 0;
bmP = NULL;
CLEAR (uvlList);
uvlList [0][1].u =
uvlList [0][2].v =
uvlList [1][0].u =
uvlList [1][1].u =
uvlList [1][1].v =
uvlList [1][2].v = I2X (1);
}

// ----------------------------------------------------------------------------

CMenuData::CMenuData ()
{
memset (this, 0, sizeof (*this));
alpha = 5 * 16 - 1;	//~ 0.3
nLineWidth = 1;
}

// ----------------------------------------------------------------------------

CApplicationData::CApplicationData ()
{
memset (this, 0, sizeof (*this));
nGameMode = GM_GAME_OVER;
}

// ----------------------------------------------------------------------------

void CGameData::SetFusionCharge (fix xCharge, bool bLocal)
{
gameData.multiplayer.weaponStates [N_LOCALPLAYER].xFusionCharge = xCharge;
if (!bLocal)
	MultiSendFusionCharge ();
}

// ----------------------------------------------------------------------------

fix CGameData::FusionCharge (fix nId)
{
return gameData.multiplayer.weaponStates [(nId < 0) ? N_LOCALPLAYER : nId].xFusionCharge;
}

// ----------------------------------------------------------------------------

fix CGameData::FusionDamage (fix xBaseDamage)
{
return 
	(gameStates.app.bHaveExtraGameInfo [IsMultiGame] && !COMPETITION)
	? fix (float (xBaseDamage) * float (extraGameInfo [IsMultiGame].nFusionRamp) / 2)
	: xBaseDamage;
}

// ----------------------------------------------------------------------------

bool CGameData::Create (int nSegments, int nVertices)
{
if (!(gameData.segs.Create (nSegments, nVertices) &&
		gameData.objs.Create () &&
		gameData.render.color.Create () &&
		gameData.render.lights.Create () &&
		gameData.render.shadows.Create () &&
		gameData.weapons.Create () &&
		gameData.physics.Create () &&
		gameData.ai.Create () &&
		gameData.multiplayer.Create () &&
		gameData.multigame.Create () &&
		gameData.demo.Create ()))
	return false;
particleManager.Init ();
particleManager.SetLastType (-1);
lightningManager.Init ();
markerManager.Init ();
gameData.physics.Init ();
gameData.bosses.Create ();
gameData.walls.Reset ();
return true;
}

// ----------------------------------------------------------------------------

void CGameData::Destroy (void)
{
gameData.segs.Destroy ();
gameData.objs.Destroy ();
gameData.walls.Destroy ();
gameData.trigs.Destroy ();
gameData.render.color.Destroy ();
gameData.render.lights.Destroy ();
gameData.render.shadows.Destroy ();
gameData.render.Destroy ();
gameData.weapons.Destroy ();
gameData.physics.Destroy ();
gameData.ai.Destroy ();
gameData.multiplayer.Destroy ();
gameData.multigame.Destroy ();
gameData.demo.Destroy ();
gameData.bosses.Destroy ();
particleManager.Shutdown ();
lightningManager.Shutdown (1);
cameraManager.Destroy ();

}

// ----------------------------------------------------------------------------

void FreeGameData (void)
{
PrintLog (1, "shutting down lightning manager\n");
lightningManager.Shutdown (1);
PrintLog (-1);
}

//------------------------------------------------------------------------------

void SetDataVersion (int v)
{
gameStates.app.bD1Data = (v < 0) ? gameStates.app.bD1Mission && gameStates.app.bHaveD1Data : v;
gameData.pig.tex.bitmaps [gameStates.app.bD1Data].ShareBuffer (gameData.pig.tex.bitmapP);
gameData.pig.tex.altBitmaps [gameStates.app.bD1Data].ShareBuffer (gameData.pig.tex.altBitmapP);
gameData.pig.tex.bmIndex [gameStates.app.bD1Data].ShareBuffer (gameData.pig.tex.bmIndexP);
gameData.pig.tex.bitmapFiles [gameStates.app.bD1Data].ShareBuffer (gameData.pig.tex.bitmapFileP);
gameData.pig.tex.tMapInfo [gameStates.app.bD1Data].ShareBuffer (gameData.pig.tex.tMapInfoP);
gameData.pig.sound.sounds [gameStates.app.bD1Data].ShareBuffer (gameData.pig.sound.soundP);
gameData.effects.effects [gameStates.app.bD1Data].ShareBuffer (gameData.effects.effectP);
gameData.effects.vClips [gameStates.app.bD1Data].ShareBuffer (gameData.effects.vClipP);
gameData.walls.anims [gameStates.app.bD1Data].ShareBuffer (gameData.walls.animP);
gameData.bots.info [gameStates.app.bD1Data].ShareBuffer (gameData.bots.infoP);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void DefaultPowerupSettings (void)
{
// powerup render option defaults
gameOptions [0].render.powerups.nSpin = 1;
}

// ----------------------------------------------------------------------------

void DefaultShipSettings (void)
{
// ship render option defaults
extraGameInfo [0].bShowWeapons = 1;
gameOptions [0].render.ship.bBullets = (gameOptions [0].render.nQuality > 1);
gameOptions [0].render.ship.nWingtip = 1;
gameOptions [0].render.ship.nColor = 0;
}

// ----------------------------------------------------------------------------

void DefaultMovieSettings (void)
{
// movie render option defaults
//gameOptions [0].movies.bSubTitles = 1;
gameOptions [0].movies.nQuality = gameOptions [0].render.nQuality > 2;
gameOptions [0].movies.bResize = 1;
}

// ----------------------------------------------------------------------------

void DefaultShadowSettings (void)
{
// shadow render option defaults
gameOptions [0].render.shadows.nLights = (gameOptions [0].render.nQuality > 1) ? 4 : 2;
#if 0
gameOptions [0].render.shadows.nReach = (gameOptions [0].render.nQuality > 1) ? 2 : 1;
gameOptions [0].render.shadows.nClip = (gameOptions [0].render.nQuality > 1) ? 2 : 1;
#endif
gameOptions [0].render.shadows.bPlayers = 1;
gameOptions [0].render.shadows.bRobots = 1;
gameOptions [0].render.shadows.bMissiles = (gameOptions [0].render.nQuality > 1);
gameOptions [0].render.shadows.bPowerups = 0;
gameOptions [0].render.shadows.bReactors = 0;
}

// ----------------------------------------------------------------------------

void DefaultCoronaSettings (void)
{
// corona render option defaults
gameOptions [0].render.coronas.bShots = 1;
gameOptions [0].render.coronas.bPowerups = 1;
gameOptions [0].render.coronas.bWeapons = 0;
gameOptions [0].render.coronas.bAdditive = 1;
gameOptions [0].render.coronas.bAdditiveObjs = 1;
#if 1
gameOptions [0].render.coronas.nIntensity = 1;
#else
if (gameOptions [0].render.coronas.nStyle == 2) 
	gameOptions [0].render.coronas.nIntensity = 1;
else if (gameOptions [0].render.coronas.nStyle == 1) 
	gameOptions [0].render.coronas.nIntensity = 1;
else
	gameOptions [0].render.coronas.nIntensity = 1;
#endif
gameOptions [0].render.coronas.nObjIntensity = 1;
}

// ----------------------------------------------------------------------------

void DefaultSmokeSettings (bool bSetup)
{
gameOptions [0].render.particles.bWiggleBubbles = 1;
gameOptions [0].render.particles.bWobbleBubbles = 1;
if (bSetup || !gameOpts->app.bExpertMode) {
	gameOptions [0].render.particles.bPlayers = (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.bRobots = 1;
	gameOptions [0].render.particles.bMissiles = 1;
	gameOptions [0].render.particles.bDebris = 1;
	gameOptions [0].render.particles.bCollisions = 0;
	gameOptions [0].render.particles.bDisperse = 1;
	gameOptions [0].render.particles.bRotate = 1;
	gameOptions [0].render.particles.bDecreaseLag = (gameOptions [0].render.nQuality < 2);
	gameOptions [0].render.particles.bAuxViews = (gameOptions [0].render.nQuality > 2);
	gameOptions [0].render.particles.bMonitors = (gameOptions [0].render.nQuality > 1);
	// player ships
	gameOptions [0].render.particles.nSize [1] = 2 + (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.nDens [1] = (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.nLife [1] = (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.nAlpha [1] = 2;
	// robots
	gameOptions [0].render.particles.nSize [2] = 2 + (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.nDens [2] = (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.nLife [2] = (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.nAlpha [2] = 2;
	// missiles
	gameOptions [0].render.particles.nSize [3] = 2 + (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.nDens [3] = 1 + (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.nLife [3] = 1 + (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.nAlpha [3] = 2;
	// debris
	gameOptions [0].render.particles.nSize [4] = 2 + (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.nDens [4] = 1 + (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.nLife [4] = 1 + (gameOpts->render.particles.nQuality > 1);
	gameOptions [0].render.particles.nAlpha [4] = 2;
	// static smoke
	#if 0
	gameOptions [0].render.particles.bStatic = 
	gameOptions [0].render.particles.bBubbles = (gameOptions [0].render.particles.nQuality > 2);
	#endif
	}
}

// ----------------------------------------------------------------------------

void DefaultLightningSettings (void)
{
gameOptions [0].render.lightning.nQuality = 0;
gameOptions [0].render.lightning.nStyle = extraGameInfo [0].bUseLightning;
gameOptions [0].render.lightning.bGlow = (gameOptions [0].render.nQuality > 1);
gameOptions [0].render.lightning.bDamage = 1;
gameOptions [0].render.lightning.bExplosions = (extraGameInfo [0].bUseLightning > 1);
gameOptions [0].render.lightning.bPlayers = (extraGameInfo [0].bUseLightning > 1);
gameOptions [0].render.lightning.bRobots = (extraGameInfo [0].bUseLightning > 1);
gameOptions [0].render.lightning.bStatic = 1;
gameOptions [0].render.lightning.bOmega = 1;
gameOptions [0].render.lightning.bRobotOmega = 1;
gameOptions [0].render.lightning.bAuxViews = (gameOptions [0].render.nQuality > 2);
gameOptions [0].render.lightning.bMonitors = 1;
}

// ----------------------------------------------------------------------------

void DefaultCameraSettings (void)
{
extraGameInfo [0].bTeleporterCams = 0;
gameOptions [0].render.cameras.bFitToWall = 0;
gameOptions [0].render.cameras.nSpeed = 5000;
gameOptions [0].render.cameras.nFPS = 0;
}

// ----------------------------------------------------------------------------

void DefaultLightSettings (void)
{
	static int nMaxLightsPerObject [] = {8, 8, 16, 24};

//gameOptions [0].render.color.nLevel = 2;
gameOptions [0].render.color.bMix = 1;
gameOptions [0].render.color.nSaturation = 1;
extraGameInfo [0].bPowerupLights = 0;
extraGameInfo [0].bBrightObjects = 0;
gameOptions [0].ogl.nMaxLightsPerObject = nMaxLightsPerObject [gameOptions [0].render.nQuality];
gameOptions [0].ogl.bHeadlight = ogl.m_features.bShaders;
gameOptions [0].ogl.bObjLighting = ogl.m_features.bShaders;
gameOptions [0].ogl.bLightObjects = ogl.m_features.bShaders;
extraGameInfo [0].bFlickerLights = !gameOptions [0].app.bEpilepticFriendly;
extraGameInfo [0].bBrightObjects = 0;
}

// ----------------------------------------------------------------------------

void DefaultAutomapSettings (void)
{
extraGameInfo [0].bPowerupsOnRadar = 1;
extraGameInfo [0].bRobotsOnRadar = 1;
gameOptions [0].render.automap.nColor = 1;
//gameOptions [0].render.automap.bBright = 0;
gameOptions [0].render.automap.bGrayOut = 1;
gameOptions [0].render.automap.bCoronas = 0;
gameOptions [0].render.automap.bSparks = 0; 
gameOptions [0].render.automap.bParticles = 0;
gameOptions [0].render.automap.bLightning = 0;
gameOptions [0].render.automap.bSkybox = 0;
}

// ----------------------------------------------------------------------------

void DefaultEffectSettings (bool bSetup)
{
gameOptions [0].render.effects.bAutoTransparency = 1;
gameOptions [0].render.effects.bTransparent = 1;
//gameOptions [0].render.effects.nShockwaves = 1;
gameOptions [0].render.effects.bEnergySparks = (gameOptions [0].render.nQuality > 0);
gameOptions [0].render.effects.bMovingSparks = 1;
if (gameOptions [0].render.nQuality < 2)
	gameOptions [0].render.effects.bGlow = 0;
extraGameInfo [0].bPlayerShield = 1;
gameOptions [0].render.effects.bRobotShields = 1;
gameOptions [0].render.effects.bOnlyShieldHits = 1;
extraGameInfo [0].bTracers = 1;
extraGameInfo [0].bShockwaves = 0; 
extraGameInfo [0].bDamageExplosions = 0;
if (bSetup || !gameOpts->app.bExpertMode)
	gameOptions [0].render.effects.bSoftParticles = (gameOptions [0].render.nQuality == 3) ? 7 : 0;
else if (gameOptions [0].render.nQuality < 3)
	gameOptions [0].render.effects.bSoftParticles = 0;
#if 1
	gameOptions [0].render.particles.bBubbles = gameOptions [0].render.particles.bStatic;
#endif
DefaultSmokeSettings ();
DefaultShadowSettings ();
DefaultCoronaSettings ();
DefaultLightningSettings ();
}

// ----------------------------------------------------------------------------

void DefaultRenderSettings (bool bSetup)
{
extraGameInfo [0].grWallTransparency = (5 * FADE_LEVELS + 5) / 10;
gameOptions [0].render.color.bWalls = 1;
ogl.m_states.nContrast = 8;
gameOptions [0].render.textures.nQuality = gameOptions [0].render.nQuality;

gameOptions [0].render.weaponIcons.bEquipment = 1;
gameOptions [0].render.weaponIcons.bSmall = 1;
gameOptions [0].render.weaponIcons.nSort = 1;
gameOptions [0].render.weaponIcons.bShowAmmo = 1;
gameOptions [0].render.weaponIcons.alpha = 4;

if (!SetSideBySideDisplayMode ())
	gameOpts->render.stereo.nGlasses = 0;
if (bSetup || !gameOpts->app.bExpertMode) {
	gameOpts->render.nLightmapPrecision = 1;
	if (gameOpts->render.stereo.nGlasses) {
		gameOpts->render.stereo.nMethod = 1;
		gameOpts->render.stereo.nScreenDist = 5;
		gameOpts->render.stereo.bColorGain = 1;
		gameOpts->render.stereo.bDeghost = 1;
		gameOpts->render.stereo.bEnhance = (gameOpts->render.bUseShaders && ogl.m_features.bShaders);
		gameOpts->render.stereo.bFlipFrames = 0;
		gameOpts->render.stereo.nFOV = STEREO_DEFAULT_FOV;
		}
	}

DefaultPowerupSettings ();
DefaultShipSettings ();
DefaultMovieSettings ();
DefaultEffectSettings (bSetup);
DefaultLightSettings ();
DefaultCameraSettings ();
DefaultAutomapSettings ();
}

// ----------------------------------------------------------------------------

void DefaultPhysicsSettings (bool bSetup)
{
extraGameInfo [0].nSpeedBoost = 10;
extraGameInfo [0].bRobotsHitRobots = 1;
extraGameInfo [0].bKillMissiles = 1;
extraGameInfo [0].bFluidPhysics = 1;
gameOpts->render.nDebrisLife = 0;
if (bSetup || (gameOpts->app.bExpertMode != SUPERUSER)) {
	extraGameInfo [0].bWiggle = 1;
	extraGameInfo [0].nOmegaRamp = (IsMultiGame && !IsCoopGame) ? 1 : 3;
	extraGameInfo [0].nMslTurnSpeed = 2;
	extraGameInfo [0].nMslStartSpeed = 0;
	gameOpts->gameplay.nSlowMotionSpeedup = 6;
	//extraGameInfo [0].nDrag = 10;
	}
}

// ----------------------------------------------------------------------------

void DefaultGameplaySettings (bool bSetup)
{
extraGameInfo [0].headlight.bAvailable = 1;
gameOptions [0].gameplay.bHeadlightOnWhenPickedUp = 0;
//gameOptions [0].gameplay.bInventory = 1;
extraGameInfo [0].bRotateMarkers = 1;
extraGameInfo [0].bMultiBosses = 1;
gameOptions [0].gameplay.bIdleAnims = (gameOpts->gameplay.nAIAggressivity > 0);
extraGameInfo [0].bImmortalPowerups = 0;
//extraGameInfo [0].nSpawnDelay = -1;
//extraGameInfo [0].bFixedRespawns = 0;
extraGameInfo [0].bDropAllMissiles = 1;
extraGameInfo [0].nWeaponDropMode = 1;
extraGameInfo [0].bDualMissileLaunch = 0;
extraGameInfo [0].bTripleFusion = 1;
extraGameInfo [0].bEnhancedShakers = 1;
gameOptions [0].gameplay.bUseD1AI = 1;
//if (!gameOpts->app.bExpertMode)
//	extraGameInfo [0].nZoomMode = 1;
gameData.multiplayer.weaponStates [N_LOCALPLAYER].nShip = gameOpts->gameplay.nShip [0];
MultiSendWeaponStates ();
}

// ----------------------------------------------------------------------------

void DefaultSettings (void)
{
}

// ----------------------------------------------------------------------------

void DefaultMiscSettings (bool bSetup)
{
gameOptions [0].gameplay.bEscortHotKeys = 1;
gameOptions [0].multi.bUseMacros = 1;
gameOptions [0].menus.bSmartFileSearch = 1;
gameOptions [0].menus.bShowLevelVersion = 1;
gameOptions [0].gameplay.bFastRespawn = 0;
gameOptions [0].demo.bOldFormat = gameStates.app.bNostalgia != 0;
}

// ----------------------------------------------------------------------------

void DefaultCockpitSettings (bool bSetup)
{
//gameOptions [0].render.cockpit.bReticle = 1;
//gameOptions [0].render.cockpit.bMissileView = 1;
gameOptions [0].render.cockpit.bGuidedInMainView = 1;
gameOptions [0].render.cockpit.bMouseIndicator = 1;
gameOptions [0].render.cockpit.bHUDMsgs = 1;
gameOptions [0].render.cockpit.bSplitHUDMsgs = 1;
gameOptions [0].render.cockpit.bWideDisplays = 1;

extraGameInfo [0].bDamageIndicators = extraGameInfo [0].bTargetIndicators;
extraGameInfo [0].bTagOnlyHitObjs = 1;
//extraGameInfo [0].bMslLockIndicators = 1;
gameOptions [0].render.cockpit.bRotateMslLockInd = 1;
extraGameInfo [0].bCloakedIndicators = 0;

gameOptions [0].render.cockpit.bScaleGauges = 1;
gameOptions [0].render.cockpit.bFlashGauges = 1;
//gameOptions [0].gameplay.bShieldWarning = 0;
//gameOptions [0].render.cockpit.bObjectTally = 1;
gameOptions [0].render.cockpit.bPlayerStats = 0;

extraGameInfo [0].nRadar = (gameOpts->render.cockpit.nRadarRange > 0);
}

// ----------------------------------------------------------------------------

void DefaultKeyboardSettings (bool bSetup)
{
gameOptions [0].input.bUseHotKeys = 1;
}

// ----------------------------------------------------------------------------

void DefaultApplicationSettings (bool bSetup)
{
gameOptions [0].app.nVersionFilter = 3;
}

// ----------------------------------------------------------------------------

void DefaultAllSettings (void)
{
DefaultApplicationSettings (true);
DefaultCockpitSettings (true);
DefaultGameplaySettings (true);
DefaultKeyboardSettings (true);
DefaultMiscSettings (true);
DefaultPhysicsSettings (true);
DefaultRenderSettings (true);
//DefaultSettings ();
}

// ----------------------------------------------------------------------------
