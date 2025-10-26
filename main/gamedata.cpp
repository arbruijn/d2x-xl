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
#include "trackobject.h"
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

//------------------------------------------------------------------------------

void CGameData::SetStereoSeparation (int32_t nFrame)
{
ogl.SetStereoSeparation (ogl.IsSideBySideDevice () ? gameOpts->render.stereo.xSeparation [ogl.IsOculusRift ()] * nFrame : 0);
}

//------------------------------------------------------------------------------

int32_t CGameData::StereoOffset2D (void)
{
#if 0
return 1;
#else
if (gameStates.render.nWindow [0])
	return 0;
if (ogl.IsOculusRift ())
	return (int32_t) DRound (X2D (ogl.StereoSeparation ()) * renderData.frame.Width () * double (WORLDSCALE) * 0.00125);
if (ogl.IsSideBySideDevice ())
	return (int32_t) DRound (X2D (ogl.StereoSeparation ()) * renderData.frame.Width () * 0.025);
return 0;
#endif
}

// ----------------------------------------------------------------------------

int32_t CGameData::FloatingStereoOffset2D (int32_t x, bool bForce)
{
#if 0
return 1;
#else
if (!bForce && (gameStates.render.nWindow [0] != 0))
	return 0;

	float scale [2];
	float w = (float) renderData.frame.Width ();
	float s = X2F (ogl.StereoSeparation ());

if (ogl.IsOculusRift ()) 
	scale [1] = s * w * float (WORLDSCALE) * 0.00125f;
else if (ogl.IsSideBySideDevice ())
	scale [1] = s * w * 0.025f;
else
	return 0;
	
scale [0] = (s < 0) ? float (w - x) / w : float (x) / w;
return (int32_t) FRound ((3.0f * scale [0] - 1.0f) * scale [1]);
#endif
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
for (int32_t i = 0; i < MAX_THREADS; i++)
	render.pActiveLights [i] = NULL;
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
nType = 0;
nTarget = 0;	//lit segment/face
nFrame = 0;
bShadow = 0;
bLightning = 0;
bExclusive = 0;
bState = 0;
CLEAR (bUsed);
CLEAR (pActiveLights);
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
int32_t i;
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
for (int32_t i = 0; i < MAX_THREADS; i++)
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
int32_t i;
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
Init ();
}

// ----------------------------------------------------------------------------

void CShadowData::Init (void)
{
nLight = 0;
nLights = 0;
pLight = NULL;
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
nSegments = 0;
nVisible = 0;
nVisited = 0;
nProcessed = 0;
}

//------------------------------------------------------------------------------

bool CVisibilityData::Create (int32_t nState)
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
	for (int32_t i = 0; i < 2; i++)
		CREATE (zRef [i], LEVEL_SEGMENTS, 0);
	CREATE (portals, LEVEL_SEGMENTS * 6, 0);
	CREATE (position, LEVEL_SEGMENTS, 0);
	}
return true;
}

//------------------------------------------------------------------------------

bool CVisibilityData::Resize (int32_t nLength)
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
for (int32_t i = 0; i < 2; i++)
	DESTROY (zRef [i]);
DESTROY (portals);
DESTROY (position);
DESTROY (points);
}

//------------------------------------------------------------------------------

CMineRenderData::CMineRenderData ()
{
bSetAutomapVisited = 0;
nObjRenderSegs = 0;
}

//------------------------------------------------------------------------------

bool CMineRenderData::Create (int32_t nState)
{
for (int32_t i = 0, j = gameStates.app.nThreads + 2; i < j; i++)
	visibility [i].Create (nState);
if (nState == 1) {
	CREATE (objRenderSegList, LEVEL_SEGMENTS, 0);
	CREATE (renderFaceListP, FACES.nFaces, 0);
	CREATE (bObjectRendered, gameData.objData.nMaxObjects, 0);
	CREATE (bRenderSegment, LEVEL_SEGMENTS, 0);
	CREATE (nRenderObjList, gameData.objData.nMaxObjects, 0);
	CREATE (bCalcVertexColor, LEVEL_VERTICES, 0);
	CREATE (bAutomapVisited, LEVEL_SEGMENTS, 0);
	CREATE (bAutomapVisible, LEVEL_SEGMENTS, 0);
	CREATE (bRadarVisited, LEVEL_SEGMENTS, 0);
	}
return true;
}

//------------------------------------------------------------------------------

bool CMineRenderData::Resize (int32_t nLength)
{
for (int32_t i = 0, j = gameStates.app.nThreads + 2; i < j; i++)
	if (!visibility [i].Resize (nLength))
		return false;
return true;
}

//------------------------------------------------------------------------------

void CMineRenderData::Destroy (void)
{
for (int32_t i = 0, j = gameStates.app.nThreads + 2; i < j; i++)
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

CVertColorData::CVertColorData ()
{
memset (this, 0, sizeof (*this)); 
SetAmbientLight (DEFAULT_AMBIENT_LIGHT);
SetSpecularLight (DEFAULT_SPECULAR_LIGHT);
InitLight ();
}

//------------------------------------------------------------------------------

void CVertColorData::InitLight (void)
{
matAmbient.Set (AmbientLight (), AmbientLight (), AmbientLight (), 1.0f);
matDiffuse.Set (DiffuseLight (), DiffuseLight (), DiffuseLight (), 1.0f);
matSpecular.Set (SpecularLight (), SpecularLight (), SpecularLight (), 1.0f);
}

//------------------------------------------------------------------------------

#if DBG

void CVertColorData::SetAmbientLight (int32_t nLight) 
{
nAmbientLight = Clamp (nLight, 0, 100);
nSpecularLight = Min (nSpecularLight, 100 - nAmbientLight); 
InitLight ();
}

//------------------------------------------------------------------------------

void CVertColorData::SetSpecularLight (int32_t nLight) 
{
nSpecularLight = Clamp (nLight, 0, 100);
nAmbientLight = Min (nAmbientLight, 100 - nSpecularLight); 
InitLight ();
}

#endif

//------------------------------------------------------------------------------

CRenderData::CRenderData ()
{
transpColor = DEFAULT_TRANSPARENCY_COLOR; //transparency color bitmap index
dAspect = 1.0;
fBrightness = 1.0f;
nTotalObjects = 0;
nTotalFaces = 0;
nUsedFaces = 0;
nTotalLights = 0;
nTotalSprites = 0;
nColoredFaces = 0;
nMaxLights = 32;
nPowerupFilter = 0;
nShaderChanges = 0;
nStateChanges = 0;
nStereoOffsetType = 0;
xFlashEffect = 0;
xTimeFlashLastPlayed = 0;
zMin = zMax = 0;
vertexList = NULL;
pVertex = NULL;
}

//------------------------------------------------------------------------------

void CRenderData::Init (void)
{
xFlashEffect = 0;
xTimeFlashLastPlayed = 0;
pVertex = NULL;
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
nStereoOffsetType = STEREO_OFFSET_NONE;
fAttScale [0] = 0.05f;
fAttScale [1] = 0.005f;
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

bool CRenderData::Create (void)
{
Destroy ();
CREATE (gameData.renderData.faceList, LEVEL_FACES, 0);
Init ();
return true;
}

// ----------------------------------------------------------------------------

void CRenderData::Destroy (void)
{
rift.Destroy ();
DESTROY (gameData.renderData.faceList);
faceIndex.Destroy ();
}

//------------------------------------------------------------------------------

CFaceData::CFaceData ()
{
iColor = 0;
nVertices = 0;
iVertices = 0;
iNormals = 0;
iLMapTexCoord = 0;
iTexCoord = 0;
iOvlTexCoord = 0;
nFaces = 0;
nTriangles = 0;
slidingFaces = NULL;
pIndex = NULL;
vboDataHandle = 0;
vboIndexHandle = 0;
pVertex = NULL;
}

//------------------------------------------------------------------------------

void CFaceData::Init (void)
{
slidingFaces = NULL;
vboDataHandle = 0;
vboIndexHandle = 0;
pVertex = NULL;
pIndex = NULL;
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

	int32_t nScale = 1 << gameStates.render.nMeshQuality;

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
CREATE (lMapTexCoord, gameData.segData.nFaces * 3 * 2, 0);
Init ();
return true;
}

// ----------------------------------------------------------------------------

bool CFaceData::Resize (void)
{
	int32_t nScale = 1 << gameStates.render.nMeshQuality;

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
RESIZE (lMapTexCoord, gameData.segData.nFaces * 3 * 2);
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
nUsedKeys = 0;
gameData.segData.nFaceKeys = -1;
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
roots.Create (gameData.segData.nFaceKeys, "CFaceListIndex::roots"); //((MAX_WALL_TEXTURES  + MAX_WALL_TEXTURES / 10) * 3);
tails.Create (gameData.segData.nFaceKeys, "CFaceListIndex::tails"); //((MAX_WALL_TEXTURES  + MAX_WALL_TEXTURES / 10) * 3);
usedKeys.Create (gameData.segData.nFaceKeys, "CFaceListIndex::usedKeys"); //((MAX_WALL_TEXTURES  + MAX_WALL_TEXTURES / 10) * 3);
Init ();
}

//------------------------------------------------------------------------------

void CFaceListIndex::Destroy (bool bRebuild)
{
if (!bRebuild)
	gameData.segData.nFaceKeys = -1;
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
nSegments = 0;
nLastSegment = 0;
nSlideSegs = 0;
nVertices = 0;
nLastVertex = 0;
nFaces = 0;
nFaceVerts = 0;
nFaceKeys = 0;
fRad = 20.0f;
xDistScale = 0;
vMin.SetZero ();
vMax.SetZero ();
CLEAR (szLevelFilename);
bHaveSlideSegs = 0;
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
nFogSegments [0] =
nFogSegments [1] =
nFogSegments [2] =
nFogSegments [3] = 0;
}

// ----------------------------------------------------------------------------

bool CSegmentData::Create (int32_t nSegments, int32_t nVertices)
{
	int32_t i;

this->nSegments = nSegments;
this->nVertices = nVertices;
CREATE (vertices, LEVEL_VERTICES, 0);
CREATE (fVertices, LEVEL_VERTICES, 0);
CREATE (vertexOwners, LEVEL_VERTICES, 0xFF);
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
dialHeaps [0].Create (gameData.segData.nSegments);
dialHeaps [1].Create (gameData.segData.nSegments);
#	else
dialHeap.Create (gameData.segData.nSegments);
#	endif
#endif
return true; //faces.Create ();
}

// ----------------------------------------------------------------------------

bool CSegmentData::Resize (void)
{
return gameData.renderData.mine.Resize () &&
		 gameData.segData.vertices.Resize (LEVEL_VERTICES) &&
		 gameData.segData.fVertices.Resize (LEVEL_VERTICES) &&
		 gameData.segData.vertexOwners.Resize (LEVEL_VERTICES);
}

// ----------------------------------------------------------------------------

void CSegmentData::Destroy (void)
{
DESTROY (gameData.segData.vertices);
DESTROY (gameData.segData.fVertices);
DESTROY (gameData.segData.vertexOwners);
DESTROY (SEGMENTS);
#if CALC_SEGRADS
DESTROY (gameData.segData.segRads [0]);
DESTROY (gameData.segData.segRads [1]);
DESTROY (gameData.segData.extent);
#endif
DESTROY (gameData.segData.segCenters [0]);
DESTROY (gameData.segData.segCenters [1]);
DESTROY (gameData.segData.sideCenters);
DESTROY (gameData.segData.bSegVis);
DESTROY (gameData.segData.bLightVis);
for (int32_t i = 0; i < LEVEL_SEGMENTS; i++)
	gameData.segData.segDistTable [i].Destroy ();
DESTROY (gameData.segData.segDistTable);
DESTROY (gameData.segData.slideSegs);
DESTROY (gameData.segData.segFaces);
DESTROY (gameData.segData.edges);
for (int32_t i = 0; i < 2; i++) {
	DESTROY (gameData.segData.edgeVertexData [i].m_vertices);
	DESTROY (gameData.segData.edgeVertexData [i].m_dists);
	}
gameData.segData.segmentGrids [0].Destroy ();
gameData.segData.segmentGrids [1].Destroy ();
gameData.segData.faceGrid.Destroy ();
nSegments = 0;
nFaces = 0;
nEdges = 0;
faceData.Destroy ();
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

inline int32_t CSegmentGrid::GridIndex (int32_t x, int32_t y, int32_t z)
{
return z * m_vDim.v.coord.y * m_vDim.v.coord.x + y * m_vDim.v.coord.x + x;
}

// ----------------------------------------------------------------------------

#define GRID_INDEX(_x,_y,_z)	(((_z) * m_vDim.v.coord.y + (_y)) * m_vDim.v.coord.x + (_x))

bool CSegmentGrid::Create (int32_t nGridSize, int32_t bSkyBox)
{
	CSegment*		pSeg;
	tSegGridIndex*	pIndex;
	CFixVector		v0, v1;
	int32_t			size, i, j, x, y, z;

Destroy ();
m_vMin.Set (0x7fffffff, 0x7fffffff, 0x7fffffff);
m_vMax.Set (-0x7fffffff, -0x7fffffff, -0x7fffffff);
pSeg = SEGMENTS.Buffer ();
for (i = gameData.segData.nSegments, j = 0; i; i--, pSeg++) {
	if ((pSeg->m_function == SEGMENT_FUNC_SKYBOX) == bSkyBox) {
		j++;
		v0 = pSeg->m_extents [0];
		v1 = pSeg->m_extents [1];
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

if (!m_index.Create (size, "CSegmentGrid::m_index"))
	return false;
m_index.Clear ();

pSeg = SEGMENTS.Buffer ();
for (i = gameData.segData.nSegments; i; i--, pSeg++) {
	if ((pSeg->m_function == SEGMENT_FUNC_SKYBOX) == bSkyBox) {
#if DBG
		if (pSeg - SEGMENTS.Buffer () == nDbgSeg)
			BRP;
#endif
		v0 = pSeg->m_extents [0] - m_vMin;
		v1 = pSeg->m_extents [1] - m_vMin;
		v0 /= I2X (m_nGridSize);
		v1 /= I2X (m_nGridSize);
		ToInt (Floor (v0));
		ToInt (Floor (v1));
		for (z = v0.v.coord.z; z <= v1.v.coord.z; z++) {
			for (y = v0.v.coord.y; y <= v1.v.coord.y; y++) {
				pIndex = &m_index [GridIndex (v0.v.coord.x, y, z)];
				for (x = v0.v.coord.x; x <= v1.v.coord.x; x++, pIndex++) {
					pIndex->nSegments++;
					}
				}
			}
		}
	}

#if DBG
int32_t h = 0, k = 0;
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

if (!m_segments.Create (j, "CSegmentGrid::m_segments")) {
	m_index.Destroy ();
	return false;
	}

pSeg = SEGMENTS.Buffer ();
for (i = 0; i < gameData.segData.nSegments; i++, pSeg++) {
	if ((pSeg->m_function == SEGMENT_FUNC_SKYBOX) == bSkyBox) {
#if DBG
		if (pSeg - SEGMENTS.Buffer () == nDbgSeg)
			BRP;
#endif
		v0 = pSeg->m_extents [0] - m_vMin;
		v1 = pSeg->m_extents [1] - m_vMin;
		v0 /= I2X (m_nGridSize);
		v1 /= I2X (m_nGridSize);
		ToInt (Floor (v0));
		ToInt (Floor (v1));
		for (z = v0.v.coord.z; z <= v1.v.coord.z; z++) {
			for (y = v0.v.coord.y; y <= v1.v.coord.y; y++) {
				pIndex = &m_index [GridIndex (v0.v.coord.x, y, z)];
				for (x = v0.v.coord.x; x <= v1.v.coord.x; x++, pIndex++)
					m_segments [pIndex->nIndex + pIndex->nSegments++] = i;
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

int32_t CSegmentGrid::GetSegList (CFixVector vPos, int16_t*& listP)
{
if (!Available ())
	return -1;
vPos -= m_vMin;
vPos /= I2X (m_nGridSize);
ToInt (Floor (vPos));
int32_t i = GRID_INDEX (vPos.v.coord.x, vPos.v.coord.y, vPos.v.coord.z);
if ((i < 0) || (i >= int32_t (m_index.Length ()))) {
	listP = NULL;
	return 0;
	}
listP = &m_segments [m_index [i].nIndex];
return m_index [i].nSegments;
}

// ----------------------------------------------------------------------------

CWallData::CWallData ()
{
exploding.Create (MAX_EXPLODING_WALLS, "CWallData::exploding");
activeDoors.Create (MAX_DOORS, "CWallData::activeDoors");
cloaking.Create (MAX_CLOAKING_WALLS, "CWallData::cloaking");
for (int32_t i = 0; i < 2; i++) {
	char szLabel [40];
	sprintf (szLabel, "CWallData::anims [%d]", i);
	anims [i].Create (MAX_WALL_ANIMS, szLabel);
	}
bitmaps.Create (MAX_WALL_ANIMS, "CWallData::bitmaps");
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

bool CTriggerData::Create (int32_t nTriggers, bool bObjTriggers)
{
if (bObjTriggers) {
	triggers [1].Create (nTriggers, "CTriggerData::triggers [1]");
#if 0
	objTriggerRefs.Create (MAX_OBJ_TRIGGERS, "CTriggerData::objTriggerRefs");
	objTriggerRefs.Clear (0xff);
#endif
	m_nTriggers [1] = nTriggers;
	}
else {
#if 0
	CREATE (firstObjTrigger, LEVEL_OBJECTS, 0xff);
#else
	CREATE (objTriggerRefs, LEVEL_OBJECTS, 0);
#endif
	triggers [0].Create (nTriggers, "CTriggerData::triggers [0]");
	delay.Create (nTriggers, "CTriggerData::delay");
	delay.Clear (0xff);
	m_nTriggers [0] = nTriggers;
	}
return true;
}

// ----------------------------------------------------------------------------

void CTriggerData::Destroy (void)
{
#if 0
firstObjTrigger.Destroy ();
#endif
triggers [0].Destroy ();
triggers [1].Destroy ();
objTriggerRefs.Destroy ();
delay.Destroy ();
m_nTriggers [0] = 0;
m_nTriggers [1] = 0;
}

// ----------------------------------------------------------------------------

CEffectData::CEffectData ()
{
for (int32_t i = 0; i < 2; i++) {
	char szLabel [40];
	sprintf (szLabel, "CEffectData::effects [%d]", i);
	effects [i].Create (MAX_EFFECTS, szLabel);
	sprintf (szLabel, "CEffectData::animations [%d]", i);
	animations [i].Create (MAX_ANIMATIONS_D2, szLabel);
	}
}

// ----------------------------------------------------------------------------

CSoundData::CSoundData ()
{
for (int32_t i = 0; i < 2; i++) {
	char szLabel [40];
	sprintf (szLabel, "CSoundData::sounds [%d]", i);
	sounds [i].Create (MAX_SOUND_FILES, szLabel); //[MAX_SOUND_FILES];
	}
sounds [0].ShareBuffer (pSound);
nType = 0;
}

// ----------------------------------------------------------------------------

CTextureData::CTextureData ()
{
for (int32_t i = 0; i < 2; i++) {
	char szLabel [40];
	sprintf (szLabel, "CTextureData::bitmapFiles [%d]", i);
	bitmapFiles [i].Create (MAX_BITMAP_FILES, szLabel);
	sprintf (szLabel, "CTextureData::bitmapFlags [%d]", i);
	bitmapFlags [i].Create (MAX_BITMAP_FILES, szLabel);
	sprintf (szLabel, "CTextureData::bitmaps [%d]", i);
	bitmaps [i].Create (MAX_BITMAP_FILES, szLabel);
	sprintf (szLabel, "CTextureData::altBitmaps [%d]", i);
	altBitmaps [i].Create (MAX_BITMAP_FILES, szLabel);
	sprintf (szLabel, "CTextureData::bmIndex [%d]", i);
	bmIndex [i].Create (MAX_TEXTURES + MAX_TEXTURES / 10, szLabel);
	bmIndex [i].Clear ();
	sprintf (szLabel, "CTextureData::textureIndex [%d]", i);
	textureIndex [i].Create (MAX_BITMAP_FILES, szLabel);
	textureIndex [i].Clear();
	sprintf (szLabel, "CTextureData::tMapInfo [%d]", i);
	tMapInfo [i].Create (MAX_TEXTURES + MAX_TEXTURES / 10, szLabel);	//add some room for extra textures like e.g. from the hoard data
	tMapInfo [i].Clear ();
	sprintf (szLabel, "CTextureData::defaultBrightness [%d]", i);
	defaultBrightness [i].Create (MAX_WALL_TEXTURES, szLabel);
	defaultBrightness [i].Clear ();
	}
objBmIndex.Create (MAX_OBJ_BITMAPS, "CTextureData::objBmIndex");
defaultObjBmIndex.Create (MAX_OBJ_BITMAPS, "CTextureData::defaultObjBmIndex");
addonBitmaps.Create (MAX_ADDON_BITMAP_FILES, "CTextureData::addonBitmaps");
bitmapXlat.Create (MAX_BITMAP_FILES, "CTextureData::bitmapXlat");
aliases.Create (MAX_ALIASES, "CTextureData::aliases");
pObjBmIndex.Create (MAX_OBJ_BITMAPS, "CTextureData::pObjBmIndex");
cockpitBmIndex.Create (N_COCKPIT_BITMAPS, "CTextureData::cockpitBmIndex");
bitmapColors.Create (MAX_BITMAP_FILES, "CTextureData::bitmapColors");
brightness.Create (MAX_WALL_TEXTURES, "CTextureData::brightness");
brightness.Clear ();

bitmaps [0].ShareBuffer (pBitmap);
altBitmaps [0].ShareBuffer (pAltBitmap);
bmIndex [0].ShareBuffer (pBmIndex);
bitmapFiles [0].ShareBuffer (pBitmapFile);
tMapInfo [0].ShareBuffer (pTexMapInfo);
gameData.pigData.tex.nFirstMultiBitmap = -1;
nObjBitmaps = 0;
bPageFlushed = 0;
nExtraBitmaps = 0;
nAliases = 0;
nHamFileVersion = 0;
nFirstMultiBitmap = 0;
CLEAR (nBitmaps);
CLEAR (nTextures);
}

// ----------------------------------------------------------------------------

CTextureData::~CTextureData ()
{
for (int32_t i = 0; i < 2; i++) {
	bitmapFiles [i].Destroy ();
	bitmapFlags [i].Destroy ();
	bitmaps [i].Destroy ();
	altBitmaps [i].Destroy ();
	bmIndex [i].Destroy ();
	textureIndex [i].Destroy ();
	tMapInfo [i].Destroy ();	//add some room for extra textures like e.g. from the hoard data
	defaultBrightness [i].Destroy ();
	}
objBmIndex.Destroy ();
defaultObjBmIndex.Destroy ();
addonBitmaps.Destroy ();
bitmapXlat.Destroy ();
aliases.Destroy ();
pObjBmIndex.Destroy ();
cockpitBmIndex.Destroy ();
bitmapColors.Destroy ();
brightness.Destroy ();

bitmaps [0].SetBuffer (NULL);
altBitmaps [0].SetBuffer (NULL);
bmIndex [0].SetBuffer (NULL);
bitmapFiles [0].SetBuffer (NULL);
tMapInfo [0].SetBuffer (NULL);
gameData.pigData.tex.nFirstMultiBitmap = -1;
nObjBitmaps = 0;
bPageFlushed = 0;
nExtraBitmaps = 0;
nAliases = 0;
nHamFileVersion = 0;
nFirstMultiBitmap = 0;
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
playerSegP = NULL;
}

//------------------------------------------------------------------------------

CRobotData::CRobotData ()
{
for (int32_t i = 0; i < 2; i++) {
	char szLabel [40];
	sprintf (szLabel, "CRobotData::info [%d]", i);
	info [i].Create (MAX_ROBOT_TYPES, szLabel);
	info [i].Clear (0xff);
	}
defaultInfo.Create (MAX_ROBOT_TYPES, "CRobotData::defaultInfo");
defaultInfo.Clear (0xff);
joints.Create (MAX_ROBOT_JOINTS, "CRobotData::joints");
joints.Clear (0xff);
defaultJoints.Create (MAX_ROBOT_JOINTS, "CRobotData::defaultJoints");
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
for (int32_t i = 0; i < MAX_BOSS_COUNT; i++)
	states [i].nDeadObj = -1;
nReactors = 0;
bDestroyed = 0;
bDisabled = 0;
bPresent = 0;
nStrength = 0;
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
for (int32_t i = 0; i < DIFFICULTY_LEVEL_COUNT; i++)
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
	int32_t i;

for (i = 0; i < 2; i++) {
	char szLabel [40];
	sprintf (szLabel, "CModelData::aseModels [%d]", i);
	aseModels [i].Create (MAX_POLYGON_MODELS, szLabel);
	sprintf (szLabel, "CModelData::oofModels [%d]", i);
	oofModels [i].Create (MAX_POLYGON_MODELS, szLabel);
	for (int32_t j = 0; j < 2; j++) {
		sprintf (szLabel, "CModelData::pofModels [%d][%d]", i, j);
		pofData [i][j].Create (MAX_POLYGON_MODELS, szLabel);
		}
	CREATE (modelToOOF [i], MAX_POLYGON_MODELS, 0);
	CREATE (modelToASE [i], MAX_POLYGON_MODELS, 0);
	sprintf (szLabel, "CModelData::renderModels [%d]", i);
	renderModels [i].Create (MAX_POLYGON_MODELS, szLabel);
	}
CREATE (bHaveHiresModel, MAX_POLYGON_MODELS, 0);
for (i = 0; i < 3; i++) {
	char szLabel [40];
	sprintf (szLabel, "CModelData::polyModels [%d]", i);
	polyModels [i].Create (MAX_POLYGON_MODELS, szLabel);
	for (int32_t j = 0; j < MAX_POLYGON_MODELS; j++)
		polyModels [i][j].SetKey (j);
	}
CREATE (modelToPOL, MAX_POLYGON_MODELS, 0);
CREATE (polyModelPoints, MAX_POLYGON_VERTS, 0);
for (i = 0; i < MAX_POLYGON_VERTS; i++)
	polyModelPoints [i].SetIndex (-1);
CREATE (fPolyModelVerts, MAX_POLYGON_VERTS, 0);
textures.Create (MAX_POLYOBJ_TEXTURES, "CModelData::textures");
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
PrintLog (1, "unloading polygon model data\n");
for (int32_t h = 0; h < 2; h++) {
	for (int32_t i = 0; i < MAX_POLYGON_MODELS; i++) {
#if DBG
		if ((nDbgModel >= 0) && (i == nDbgModel))
			BRP;
#endif
		renderModels [h][i].Destroy ();
		gameData.modelData.pofData [h][0][i].Destroy ();
		gameData.modelData.pofData [h][1][i].Destroy ();
		}
	}
PrintLog (-1);
}

// ----------------------------------------------------------------------------

CMorphData::CMorphData ()
{
gameData.renderData.morph.xRate = MORPH_RATE;
}

// ----------------------------------------------------------------------------

CCockpitData::CCockpitData ()
{
for (int32_t i = 0; i < 2; i++) {
	char szLabel [40];
	sprintf (szLabel, "CCockpitData::gauges [%d]", i);
	if (gauges [i].Create (MAX_GAUGE_BMS, szLabel))
		gauges [i].Clear (0xff);
	}
}

//------------------------------------------------------------------------------

void InitWeaponFlags (void)
{
}

//------------------------------------------------------------------------------

void InitIdToOOF (void)
{
gameData.objData.idToOOF.Clear (0);
gameData.objData.idToOOF [MEGAMSL_ID] = OOF_MEGA;
}

// ----------------------------------------------------------------------------

CObjectData::CObjectData ()
{
pConsole = NULL;
pViewer = NULL;
deadPlayerCamera = NULL;
endLevelCamera = NULL;
pMissileViewer = NULL;
nChildFreeList = 0;
nDeadControlCenter = -1;
nDebris = 0;
nFirstDropped = 0;
nLastDropped = 0;
nFreeDropped = 0;
nDropped = 0;
nDrops = 0;
nEffects = 0;
nFrameCount = 0;
nInitialRobots = 0;
nObjects = 0;
nMaxObjects = 0;
nMaxUsedObjects = 0;
nObjectLimit = 0;
nNextSignature = 0;
nVertigoBotFlags = 0;
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
pConsole = NULL;
pViewer = NULL;
pMissileViewer = NULL;
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
nMaxObjects = Max (gameFileInfo.objects.count + 1000, gameFileInfo.objects.count * 2);
nMaxUsedObjects = LEVEL_OBJECTS - 20;
}

// ----------------------------------------------------------------------------

void CObjectData::InitFreeList (void)
{
for (int32_t i = 0; i < LEVEL_OBJECTS; i++) {
	gameData.objData.freeList [i] = i;
	gameData.objData.freeListIndex.Clear (0xff);
	gameData.objData.objects [i].Init ();
	}
}

// ----------------------------------------------------------------------------

bool CObjectData::Create (void)
{
Init ();
CREATE (gameData.objData.objects, LEVEL_OBJECTS, 0);
CREATE (gameData.objData.update, LEVEL_OBJECTS, 0);
CREATE (gameData.objData.freeList, LEVEL_OBJECTS, 0);
CREATE (gameData.objData.freeListIndex, LEVEL_OBJECTS, 0xff);
CREATE (gameData.objData.parentObjs, LEVEL_OBJECTS, (char) 0xff);
CREATE (gameData.objData.childObjs, LEVEL_OBJECTS, 0);
CREATE (gameData.objData.firstChild, LEVEL_OBJECTS, (char) 0xff);
CREATE (gameData.objData.init, LEVEL_OBJECTS, 0);
CREATE (gameData.objData.dropInfo, LEVEL_OBJECTS, 0);
CREATE (gameData.objData.speedBoost, LEVEL_OBJECTS, 0);
CREATE (gameData.objData.vRobotGoals, LEVEL_OBJECTS, 0);
CREATE (gameData.objData.xLastAfterburnerTime, LEVEL_OBJECTS, 0);
CREATE (gameData.objData.xLight, LEVEL_OBJECTS, 0);
CREATE (gameData.objData.nLightSig, LEVEL_OBJECTS, 0);
CREATE (gameData.objData.nHitObjects, LEVEL_OBJECTS * MAX_HIT_OBJECTS, 0);
CREATE (gameData.objData.viewData, LEVEL_OBJECTS, (char) 0xFF);
CREATE (gameData.objData.bWantEffect, LEVEL_OBJECTS, (char) 0);
InitFreeList ();
memset (&gameData.objData.lists, 0, sizeof (gameData.objData.lists));
return gameData.renderData.mine.Create (0) && lightClusterManager.Init () && shrapnelManager.Init ();
}

// ----------------------------------------------------------------------------

void CObjectData::Destroy (void)
{
DESTROY (gameData.objData.objects);
DESTROY (gameData.objData.effects);
DESTROY (gameData.objData.freeListIndex);
DESTROY (gameData.objData.freeList);
DESTROY (gameData.objData.parentObjs);
DESTROY (gameData.objData.childObjs);
DESTROY (gameData.objData.firstChild);
DESTROY (gameData.objData.init);
DESTROY (gameData.objData.dropInfo);
DESTROY (gameData.objData.speedBoost);
DESTROY (gameData.objData.vRobotGoals);
DESTROY (gameData.objData.xLastAfterburnerTime);
DESTROY (gameData.objData.xLight);
DESTROY (gameData.objData.nLightSig);
DESTROY (gameData.objData.nHitObjects);
DESTROY (gameData.objData.viewData);
DESTROY (gameData.objData.bWantEffect);
memset (&gameData.objData.lists, 0, sizeof (gameData.objData.lists));
nObjects = 0;
lightManager.Reset ();
shrapnelManager.Reset ();
}

//------------------------------------------------------------------------------

void CObjectData::GatherEffects (void)
{
if (nEffects && effects.Create (nEffects)) {
	int32_t i, j;
	for (i = j = 0; i < gameFileInfo.objects.count; i++) {
		if (OBJECT (i)->info.nType == OBJ_EFFECT) {
			effects [j] = *OBJECT (i);
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

int32_t CObjectData::RebuildEffects (void)
{
	int32_t j = 0;

if (nEffects && effects.Buffer ()) {
	for (int32_t i = 0; i < nEffects; i++) {
		tBaseObject& bo = effects [i];
		int32_t nObject = CreateObject (bo.info.nType, bo.info.nId, -1,
											 -bo.info.nSegment - 2, bo.info.position.vPos, bo.info.position.mOrient,
											 bo.info.xSize, bo.info.controlType, bo.info.movementType, bo.info.renderType);
		if (nObject >= 0) {
			OBJECT (nObject)->info = bo.info;
			OBJECT (nObject)->mType = bo.mType;
			OBJECT (nObject)->cType = bo.cType;
			OBJECT (nObject)->rType = bo.rType;
			if (OBJECT (nObject)->info.nId == PARTICLE_ID)
				OBJECT (nObject)->SetupSmoke ();
			j++;
			}
		}
	}
return j;
}

// ----------------------------------------------------------------------------

void CObjectData::SetGuidedMissile (uint8_t nPlayer, CObject* pObj) 
{
guidedMissile [nPlayer].pObj = pObj;
guidedMissile [nPlayer].nSignature = pObj ? pObj->Signature () : -1;
}

// ----------------------------------------------------------------------------

bool CObjectData::IsGuidedMissile (CObject* pObj) 
{
return pObj && pObj->IsGuidedMissile ();
}

// ----------------------------------------------------------------------------

bool CObjectData::HasGuidedMissile (uint8_t nPlayer) 
{
return IsGuidedMissile (guidedMissile [nPlayer].pObj);
}

// ----------------------------------------------------------------------------

CObject* CObjectData::GetGuidedMissile (uint8_t nPlayer) 
{
return IsGuidedMissile (guidedMissile [nPlayer].pObj) ? guidedMissile [nPlayer].pObj : NULL;
}

// ----------------------------------------------------------------------------

CColorData::CColorData ()
{
if (textures.Create (MAX_WALL_TEXTURES, "CColorData::textures"))
	textures.Clear ();
for (int32_t i = 0; i < 2; i++) {
	char szLabel [40];
	sprintf (szLabel, "CColorData::defaultData [%d]", i);
	if (defaultTextures [i].Create (MAX_WALL_TEXTURES, szLabel))
		defaultTextures [i].Clear ();
	}
nVisibleLights = 0;
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
nHomingWeaponFPS [0] = 25;
nHomingWeaponFPS [1] = 33;
nHomingWeaponFPS [2] = 40;
nHomingWeaponFPS [3] = 60;
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
gameData.weaponData.xMinTrackableDot = MIN_TRACKABLE_DOT;
CLEAR (firing);
CLEAR (nTypes);
}

// ----------------------------------------------------------------------------

bool CWeaponData::Create (void)
{
CREATE (color, LEVEL_OBJECTS, 0);
for (int32_t i = 0; i < LEVEL_OBJECTS; i++)
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
for (int32_t i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	CREATE (gameData.multigame.remoteToLocal [i], LEVEL_OBJECTS, 0xff);  // Remote CObject number for each local CObject
CREATE (gameData.multigame.localToRemote, LEVEL_OBJECTS, 0xff);
CREATE (gameData.multigame.nObjOwner, LEVEL_OBJECTS, 0xff);   // Who created each CObject in my universe, -1 = loaded at start
return true;
}

// ----------------------------------------------------------------------------

void CMultiGameData::Destroy (void)
{
for (int32_t i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	DESTROY (gameData.multigame.remoteToLocal [i]);  // Remote CObject number for each local CObject
DESTROY (gameData.multigame.localToRemote);
DESTROY (gameData.multigame.nObjOwner);   // Who created each CObject in my universe, -1 = loaded at start
}

// ----------------------------------------------------------------------------

CAIData::CAIData ()
{
gameData.aiData.bInitialized = 0;
gameData.aiData.bEvaded = 0;
gameData.aiData.bEnableAnimation = 1;
gameData.aiData.bInfoEnabled = 0;
gameData.aiData.nAwarenessEvents = 0;
gameData.aiData.target.nDistToLastPosFiredAt = 0;
cloakInfo.Create (MAX_AI_CLOAK_INFO, "CAIData::cloakInfo");
awarenessEvents.Create (MAX_AWARENESS_EVENTS, "CAIData::awarenessEvents");
gameData.aiData.freePointSegs = gameData.aiData.routeSegs.Buffer ();
nHitSeg = -1;
nHitType = -1;
bObjAnimates = 0;
nLastMissileCamera = -1;
nMaxAwareness = 0;
nOverallAgitation = 0;
nTargetVisibility = 0;
}

// ----------------------------------------------------------------------------

bool CAIData::Create (void)
{
CREATE (gameData.aiData.localInfo, LEVEL_OBJECTS, 0);
CREATE (routeSegs, LEVEL_POINT_SEGS, 0);
return true;
}

// ----------------------------------------------------------------------------

void CAIData::Destroy (void)
{
DESTROY (gameData.aiData.localInfo);
DESTROY (routeSegs);
}

// ----------------------------------------------------------------------------

bool CDemoData::Create (void)
{
CREATE (gameData.demoData.bWasRecorded,  LEVEL_OBJECTS, 0);
CREATE (gameData.demoData.bViewWasRecorded, LEVEL_OBJECTS, 0);
return true;
}

// ----------------------------------------------------------------------------

void CDemoData::Destroy (void)
{
DESTROY (gameData.demoData.bWasRecorded);
DESTROY (gameData.demoData.bViewWasRecorded);
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
gameData.timeData.xMaxOnline = 180000;
gameData.timeData.xGameStart = -gameData.timeData.xMaxOnline;
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
pBm = NULL;
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
	(gameStates.app.bHaveExtraGameInfo [IsMultiGame] && !COMPETITION && !EGI_FLAG (bUnnerfD1Weapons, 0, 0, 0))
	? fix (float (xBaseDamage) * float (extraGameInfo [IsMultiGame].nFusionRamp) / 2)
	: xBaseDamage;
}

// ----------------------------------------------------------------------------

bool CGameData::Create (int32_t nSegments, int32_t nVertices)
{
ENTER (0, 0);
if (!(gameData.segData.Create (nSegments, nVertices) &&
		gameData.objData.Create () &&
		gameData.renderData.color.Create () &&
		gameData.renderData.lights.Create () &&
		gameData.renderData.shadows.Create () &&
		gameData.weaponData.Create () &&
		gameData.physicsData.Create () &&
		gameData.aiData.Create () &&
		gameData.multiplayer.Create () &&
		gameData.multigame.Create () &&
		gameData.demoData.Create ()))
	RETVAL (false)
particleManager.Init ();
particleManager.SetLastType (-1);
lightningManager.Init ();
markerManager.Init ();
gameData.physicsData.Init ();
gameData.bossData.Create ();
gameData.wallData.Reset ();
RETVAL (true)
}

// ----------------------------------------------------------------------------

void CGameData::Destroy (void)
{
ENTER (0, 0);
gameData.segData.Destroy ();
gameData.objData.Destroy ();
gameData.wallData.Destroy ();
gameData.trigData.Destroy ();
gameData.renderData.color.Destroy ();
gameData.renderData.lights.Destroy ();
gameData.renderData.shadows.Destroy ();
gameData.renderData.Destroy ();
gameData.weaponData.Destroy ();
gameData.physicsData.Destroy ();
gameData.aiData.Destroy ();
gameData.multiplayer.Destroy ();
gameData.multigame.Destroy ();
gameData.demoData.Destroy ();
gameData.bossData.Destroy ();
particleManager.Shutdown ();
lightningManager.Shutdown (1);
cameraManager.Destroy ();
RETURN
}

// ----------------------------------------------------------------------------

void FreeGameData (void)
{
PrintLog (1, "shutting down lightning manager\n");
lightningManager.Shutdown (1);
PrintLog (-1);
}

//------------------------------------------------------------------------------

void SetDataVersion (int32_t v)
{
gameStates.app.bD1Data = (v < 0) ? gameStates.app.bD1Mission && gameStates.app.bHaveD1Data : v;
PrintLog (0, "Setting data version %d (D%d) (D1 mission = %d, have D1 data = %d)\n", v, 2 - gameStates.app.bD1Data, gameStates.app.bD1Mission, gameStates.app.bHaveD1Data);
gameData.pigData.tex.bitmaps [gameStates.app.bD1Data].ShareBuffer (gameData.pigData.tex.pBitmap);
gameData.pigData.tex.altBitmaps [gameStates.app.bD1Data].ShareBuffer (gameData.pigData.tex.pAltBitmap);
gameData.pigData.tex.bmIndex [gameStates.app.bD1Data].ShareBuffer (gameData.pigData.tex.pBmIndex);
gameData.pigData.tex.bitmapFiles [gameStates.app.bD1Data].ShareBuffer (gameData.pigData.tex.pBitmapFile);
gameData.pigData.tex.tMapInfo [gameStates.app.bD1Data].ShareBuffer (gameData.pigData.tex.pTexMapInfo);
gameData.pigData.sound.sounds [gameStates.app.bD1Data].ShareBuffer (gameData.pigData.sound.pSound);
gameData.effectData.effects [gameStates.app.bD1Data].ShareBuffer (gameData.effectData.pEffect);
gameData.effectData.animations [gameStates.app.bD1Data].ShareBuffer (gameData.effectData.vClipP);
gameData.wallData.anims [gameStates.app.bD1Data].ShareBuffer (gameData.wallData.pAnim);
gameData.botData.info [gameStates.app.bD1Data].ShareBuffer (gameData.botData.pInfo);
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
if (!gameOptions [0].movies.nQuality)
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
	gameOptions [0].render.particles.bDisperse = 0;
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

void DefaultLightSettings (bool bSetup)
{
	static int32_t nMaxLightsPerObject [] = {8, 8, 16, 24};

//gameOptions [0].render.color.nLevel = 2;
gameOptions [0].render.color.bMix = 1;
gameOptions [0].render.color.nSaturation = 1;
if (bSetup || !gameOpts->app.bExpertMode) {
	gameOptions [0].render.color.nAmbientLight = DEFAULT_AMBIENT_LIGHT;
	gameOptions [0].render.color.nSpecularLight = DEFAULT_SPECULAR_LIGHT;
	}
gameData.SetAmbientLight (gameOpts->render.color.nAmbientLight);
gameData.SetSpecularLight (gameOpts->render.color.nSpecularLight);
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
gameOptions [0].render.effects.bMovingSparks = 1;
#if 0
if (gameOptions [0].render.nQuality < 2)
	gameOptions [0].render.effects.bGlow = 0;
#endif
extraGameInfo [0].nShieldEffect = gameOpts->render.effects.bShields;
gameOptions [0].render.effects.bOnlyShieldHits = 1;
extraGameInfo [0].bTracers = 1;
extraGameInfo [0].bShockwaves = 0; 
extraGameInfo [0].bDamageExplosions = 0;
if (bSetup || !gameOpts->app.bExpertMode) {
	gameOptions [0].render.effects.bEnergySparks = (gameOptions [0].render.nQuality > 1);
	gameOptions [0].render.effects.bSoftParticles = (gameOptions [0].render.nQuality == 3) ? 7 : 0;
	}
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
gameOptions [0].render.weaponIcons.bShowAmmo = 1;
gameOptions [0].render.weaponIcons.alpha = 4;

if (!SetSideBySideDisplayMode ())
	gameOpts->render.stereo.nGlasses = 0;
if (bSetup || !gameOpts->app.bExpertMode) {
	gameOptions [0].render.weaponIcons.nSort = 1;
	gameOpts->render.nLightmapPrecision = 1;
	if (gameOpts->render.stereo.nGlasses) {
		gameOpts->render.stereo.nMethod = 1;
		gameOpts->render.stereo.nScreenDist = 5;
		gameOpts->render.stereo.bColorGain = 1;
		gameOpts->render.stereo.bDeghost = 1;
		gameOpts->render.stereo.bEnhance = (gameOpts->render.bUseShaders && ogl.m_features.bShaders);
		gameOpts->render.stereo.bFlipFrames = 0;
		gameOpts->render.stereo.nRiftFOV = RIFT_DEFAULT_FOV;
		}
	}

DefaultPowerupSettings ();
DefaultShipSettings ();
DefaultMovieSettings ();
DefaultEffectSettings (bSetup);
DefaultLightSettings (bSetup);
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
	extraGameInfo [0].nWeaponTurnSpeed = 0;
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
if (gameOpts->app.bExpertMode != SUPERUSER) {
	extraGameInfo [0].nRechargeDelay = 4;
	extraGameInfo [0].nRechargeSpeed = 1;
	}
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

if (!gameOpts->app.bExpertMode) {
	gameOpts->render.cockpit.nCompactWidth = 0;
	gameOpts->render.cockpit.nCompactHeight = 0;
	}

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

void DefaultAllSettings (bool bSetup)
{
DefaultApplicationSettings (bSetup);
DefaultCockpitSettings (bSetup);
DefaultGameplaySettings (bSetup);
DefaultKeyboardSettings (bSetup);
DefaultMiscSettings (bSetup);
DefaultPhysicsSettings (bSetup);
DefaultRenderSettings (bSetup);
//DefaultSettings ();
}

// ----------------------------------------------------------------------------

#if DBG

void* GameDataError (const char* szData, const char* szType, int32_t bPrintLog, const char* pszFile, int32_t nLine) 
{
if (bPrintLog) {
	if (*pszFile)
		PrintLog (0, "Invalid %s reference (%s) in file %s (%d)\n", szData, szType, pszFile, nLine);
	else
		PrintLog (0, "Invalid %s reference (%s)\n", szData, szType);
	}
return NULL;
}

CObject* CGameData::Object (int32_t nObject, int32_t nChecks, const char* pszFile, int32_t nLine) 
{ 
if (!objData.objects.Buffer ())
	return (CObject*) GameDataError ("object", "buffer", nChecks & GAMEDATA_ERRLOG_BUFFER, pszFile, nLine);
if (nObject < 0)
	return (CObject*) GameDataError ("object", "underflow", nChecks & GAMEDATA_ERRLOG_UNDERFLOW, pszFile, nLine);
if (nObject > objData.nLastObject [0])
	return (CObject*) GameDataError ("object", "overflow", nChecks & GAMEDATA_ERRLOG_OVERFLOW, pszFile, nLine);
if ((uint32_t) nObject >= objData.objects.Length ())
	return (CObject*) GameDataError ("object", "overflow", nChecks & GAMEDATA_ERRLOG_BUFFER, pszFile, nLine);
return objData.objects + nObject; 
}


CSegment* CGameData::Segment (int32_t nSegment, int32_t nChecks, const char* pszFile, int32_t nLine) 
{ 
if (!segData.segments.Buffer ())	
	return (CSegment*) GameDataError ("segment", "buffer", nChecks & GAMEDATA_ERRLOG_BUFFER, pszFile, nLine);
if (nSegment < 0)
	return (CSegment*) GameDataError ("segment", "underflow", nChecks & GAMEDATA_ERRLOG_UNDERFLOW, pszFile, nLine);
if (nSegment >= segData.nSegments)
	return (CSegment*) GameDataError ("segment", "overflow", nChecks & GAMEDATA_ERRLOG_OVERFLOW, pszFile, nLine);
if ((uint32_t) nSegment >= segData.segments.Length ())
	return (CSegment*) GameDataError ("segment", "overflow", nChecks & GAMEDATA_ERRLOG_BUFFER, pszFile, nLine);
return segData.segments + nSegment; 
}


CWall* CGameData::Wall (int32_t nWall, int32_t nChecks, const char* pszFile, int32_t nLine) 
{ 
if (!wallData.walls.Buffer ())
	return (CWall*) GameDataError ("wall", "buffer", nChecks & GAMEDATA_ERRLOG_BUFFER, pszFile, nLine);
if (nWall < 0)
	return (CWall*) GameDataError ("wall", "underflow", nChecks & GAMEDATA_ERRLOG_UNDERFLOW, pszFile, nLine);
if (nWall == NO_WALL)
	return (CWall*) GameDataError ("wall", "null", nChecks & GAMEDATA_ERRLOG_UNDERFLOW, pszFile, nLine);
if (nWall >= wallData.nWalls)
	return (CWall*) GameDataError ("wall", "overflow", nChecks & GAMEDATA_ERRLOG_OVERFLOW, pszFile, nLine);
if ((uint32_t) nWall >= wallData.walls.Length ())
	return (CWall*) GameDataError ("wall", "overflow", nChecks & GAMEDATA_ERRLOG_BUFFER, pszFile, nLine);
return wallData.walls + nWall; 
}


CTrigger* CGameData::Trigger (int32_t nType, int32_t nTrigger, int32_t nChecks, const char* pszFile, int32_t nLine) 
{
if (nTrigger == NO_TRIGGER)
	return NULL;
if (!trigData.triggers [nType].Buffer ())
	return (CTrigger*) GameDataError ("trigger", "buffer", nChecks & GAMEDATA_ERRLOG_BUFFER, pszFile, nLine);
if (nTrigger < 0)
	return (CTrigger*) GameDataError ("trigger", "underflow", nChecks & GAMEDATA_ERRLOG_UNDERFLOW, pszFile, nLine);
if (nTrigger == NO_TRIGGER)
	return (CTrigger*) GameDataError ("trigger", "null", nChecks & GAMEDATA_ERRLOG_UNDERFLOW, pszFile, nLine);
if (nTrigger >= trigData.m_nTriggers [nType])
	return (CTrigger*) GameDataError ("trigger", "overflow", nChecks & GAMEDATA_ERRLOG_OVERFLOW, pszFile, nLine);
if ((uint32_t) nTrigger >= trigData.triggers [nType].Length ())
	return (CTrigger*) GameDataError ("trigger", "overflow", nChecks & GAMEDATA_ERRLOG_BUFFER, pszFile, nLine);
return trigData.triggers [nType] + nTrigger; 
}


CTrigger* CGameData::GeoTrigger (int32_t nTrigger, int32_t nChecks, const char* pszFile, int32_t nLine) 
{
return Trigger (0, nTrigger, nChecks, pszFile, nLine);
}


CTrigger* CGameData::ObjTrigger (int32_t nTrigger, int32_t nChecks, const char* pszFile, int32_t nLine) 
{
return Trigger (1, nTrigger, nChecks, pszFile, nLine);
}


tRobotInfo* CGameData::RobotInfo (int32_t nId, int32_t nChecks, const char* pszFile, int32_t nLine) 
{
int32_t bD1 = gameStates.app.bD1Mission && (nId < botData.nTypes [1]);
CArray<tRobotInfo>& a = botData.info [bD1];
if (!a.Buffer ())
	return (tRobotInfo*) GameDataError ("robot info", "buffer", nChecks & GAMEDATA_ERRLOG_BUFFER, pszFile, nLine);
if (nId < 0)
	return (tRobotInfo*) GameDataError ("robot info", "underflow", nChecks & GAMEDATA_ERRLOG_UNDERFLOW, pszFile, nLine);
if ((uint32_t) nId >= a.Length ())
	return (tRobotInfo*) GameDataError ("robot info", "overflow", nChecks & GAMEDATA_ERRLOG_OVERFLOW, pszFile, nLine);
if (nId >= botData.nTypes [bD1])
	return (tRobotInfo*) GameDataError ("robot info", "overflow", nChecks & GAMEDATA_ERRLOG_OVERFLOW, pszFile, nLine);
return a + nId; 
}


tRobotInfo* CGameData::RobotInfo (CObject* pObj, int32_t nChecks, const char* pszFile, int32_t nLine) 
{
if (!pObj || !Object (pObj->Index (), GAMEDATA_ERRLOG_ALL, pszFile, nLine))
	 return (tRobotInfo*) GameDataError ("robot info", "object buffer", nChecks, pszFile, nLine);
if ((nChecks & GAMEDATA_ERRLOG_TYPE) && !pObj->IsRobot () && !pObj->IsReactor ())
	return (tRobotInfo*) GameDataError ("robot info", "object type", nChecks, pszFile, nLine);
return RobotInfo (pObj->Id (), nChecks, pszFile, nLine);
}


CWeaponInfo* CGameData::WeaponInfo (int32_t nId, int32_t bD1, int32_t nChecks, const char* pszFile, int32_t nLine) 
{
CArray<CWeaponInfo>& a = weaponData.info [(bD1 < 0) ? gameStates.app.bD1Mission : bD1];
if (!a.Buffer ())
	return (CWeaponInfo*) GameDataError ("weapon info", "buffer", nChecks & GAMEDATA_ERRLOG_BUFFER, pszFile, nLine);
if (nId < 0)
	return (CWeaponInfo*) GameDataError ("weapon info", "underflow", nChecks & GAMEDATA_ERRLOG_UNDERFLOW, pszFile, nLine);
if (nId >= gameData.weaponData.nTypes [gameStates.app.bD1Mission])
	return (CWeaponInfo*) GameDataError ("weapon info", "overflow", nChecks & GAMEDATA_ERRLOG_OVERFLOW, pszFile, nLine);
return a + nId; 
}


CWeaponInfo* CGameData::WeaponInfo (CObject* pObj, int32_t bD1, int32_t nChecks, const char* pszFile, int32_t nLine) 
{
if (!pObj || !Object (pObj->Index (), GAMEDATA_ERRLOG_ALL, pszFile, nLine))
	 return (CWeaponInfo*) GameDataError ("weapon info", "object buffer", nChecks, pszFile, nLine);
if ((nChecks & GAMEDATA_ERRLOG_TYPE) && (pObj->Type () != OBJ_WEAPON))
	return (CWeaponInfo*) GameDataError ("weapon info", "object type", nChecks, pszFile, nLine);
return WeaponInfo (pObj->Id (), bD1, nChecks, pszFile, nLine);
}

#endif

static inline CWeaponInfo *WI_Unnerfed (int32_t nId) 
{
return gameData.WeaponInfo (nId, ((nId < gameData.weaponData.nTypes [1]) && EGI_FLAG (bUnnerfD1Weapons, 0, 0, 0)) ? 1 : -1);
}

fix WI_Strength (int32_t nId, int32_t nDifficulty) 
{
CWeaponInfo *pInfo = WI_Unnerfed (nId);
return pInfo ? pInfo->strength [Clamp (nDifficulty, 0, DIFFICULTY_LEVEL_COUNT - 1)] : 0;
}

fix WI_FlashSize (int32_t nId) 
{
CWeaponInfo *pInfo = WI_Unnerfed (nId);
return pInfo ? pInfo->xFlashSize : 0;
}

fix WI_ImpactSize (int32_t nId) 
{
CWeaponInfo *pInfo = WI_Unnerfed (nId);
return pInfo ? pInfo->xImpactSize : 0;
}

fix WI_EnergyUsage (int32_t nId) 
{
CWeaponInfo *pInfo = WI_Unnerfed (nId);
return pInfo ? pInfo->xEnergyUsage : 0;
}

fix WI_FireWait (int32_t nId) 
{
CWeaponInfo *pInfo = WI_Unnerfed (nId);
return pInfo ? pInfo->xFireWait : 0;
}

fix WI_Speed (int32_t nId, int32_t nDifficulty) 
{
CWeaponInfo *pInfo = WI_Unnerfed (nId);
return pInfo ? pInfo->speed [Clamp (nDifficulty, 0, DIFFICULTY_LEVEL_COUNT - 1)] : 0;
}

fix WI_DamageRadius (int32_t nId)	
{
CWeaponInfo *pInfo = WI_Unnerfed (nId);
return pInfo ? pInfo->xDamageRadius : 0;
}

// ----------------------------------------------------------------------------
