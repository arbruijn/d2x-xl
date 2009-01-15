/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

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
#include "inferno.h"
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
#include "playsave.h"
#include "tracker.h"
#include "render.h"
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
Init ();
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
shadow.Init ();
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
xDistance = 0;
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
material.bValid = 0;
memset (nHeadlights, 0xff, sizeof (nHeadlights));
CLEAR (active);
CLEAR (index);
}

// ----------------------------------------------------------------------------

bool CDynLightData::Create (void)
{
CREATE (nearestSegLights, LEVEL_SEGMENTS * MAX_NEAREST_LIGHTS, 0xff);
CREATE (nearestVertLights, LEVEL_VERTICES * MAX_NEAREST_LIGHTS, 0xff);
CREATE (variableVertLights, LEVEL_VERTICES, 0xff);
CREATE (owners, LEVEL_OBJECTS, (char) 0xff);
return true;
}

// ----------------------------------------------------------------------------

void CDynLightData::Destroy (void)
{
DESTROY (nearestSegLights);
DESTROY (nearestVertLights);
DESTROY (variableVertLights);
DESTROY (owners);
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
if (!(gameStates.app.bNostalgia || lightManager.Create ()))
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
lights = NULL;
nShadowMaps = 0;
memset (&lightSource, 0, sizeof (lightSource));
memset (&vLightPos, 0, sizeof (vLightPos));
CLEAR (vLightDir);
nFrame = 0;
}

// ----------------------------------------------------------------------------

bool CShadowData::Create (void)
{
if (!gameStates.app.bNostalgia && gameStates.app.bEnableShadows)
	CREATE (objLights, LEVEL_OBJECTS * MAX_SHADOW_LIGHTS, 0);
Init ();
return true;
}

// ----------------------------------------------------------------------------

void CShadowData::Destroy (void)
{
DESTROY (objLights);
}

// ----------------------------------------------------------------------------

COglData::COglData ()
{
palette = NULL;
nSrcBlend = GL_SRC_ALPHA;
nDestBlend = GL_ONE_MINUS_SRC_ALPHA;
zNear = 1.0f;
zFar = 5000.0f;
depthScale.SetZero ();
screenScale.x = 
screenScale.y = 0;
CLEAR (nPerPixelLights);
CLEAR (lightRads);
CLEAR (lightPos);
bLightmaps = 0;
nHeadlights = 0;
};

//------------------------------------------------------------------------------

CThrusterData::CThrusterData ()
{
fSpeed = 0;
nPulse = 0;
tPulse = 0;
}

//------------------------------------------------------------------------------

CMineRenderData::CMineRenderData ()
{
}
 
//------------------------------------------------------------------------------

bool CMineRenderData::Create (void)
{
nVisited = 255;
nProcessed = 255;
nVisible = 255;
CREATE (nSegRenderList, gameData.segs.nSegments, 0);
CREATE (pFaceRenderList, gameData.segs.nFaces, 0);
CREATE (bVisited, gameData.segs.nSegments, 0);
CREATE (bVisible, gameData.segs.nSegments, 0);
CREATE (bProcessed, gameData.segs.nSegments, 0);
CREATE (nSegDepth, gameData.segs.nSegments, 0); 
CREATE (bObjectRendered, gameData.objs.nMaxObjects, 0); 
CREATE (bRenderSegment, gameData.segs.nSegments, 0); 
CREATE (nRenderObjList, gameData.objs.nMaxObjects, 0);
CREATE (nRenderPos, gameData.segs.nSegments, 0); 
CREATE (nRotatedLast, gameData.segs.nVertices, 0); 
CREATE (bCalcVertexColor, gameData.segs.nVertices, 0); 
CREATE (bAutomapVisited, gameData.segs.nSegments, 0); 
CREATE (bAutomapVisible, gameData.segs.nSegments, 0); 
CREATE (bRadarVisited, gameData.segs.nSegments, 0); 
return true;
}

//------------------------------------------------------------------------------

void CMineRenderData::Destroy (void)
{
DESTROY (nSegRenderList);
DESTROY (pFaceRenderList);
DESTROY (bVisited);
DESTROY (bVisible);
DESTROY (bProcessed);
DESTROY (nSegDepth); 
DESTROY (bObjectRendered); 
DESTROY (bRenderSegment); 
DESTROY (nRenderObjList);
DESTROY (nRenderPos); 
DESTROY (nRotatedLast); 
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
fAttScale = 0;
nPowerupFilter = 0;
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
CREATE (gameData.render.faceList, LEVEL_FACES, 0);
Init ();
return true;
}

// ----------------------------------------------------------------------------

void CRenderData::Destroy (void)
{
DESTROY (gameData.render.faceList);
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
CREATE (faces, LEVEL_FACES, 0);
CREATE ( tris, LEVEL_TRIANGLES, 0);
CREATE (vertices, LEVEL_TRIANGLES * 3, 0);
#if USE_RANGE_ELEMENTS
CREATE (vertIndex, LEVEL_TRIANGLES * 3, 0);
#endif
CREATE (faceVerts, LEVEL_FACES * 16, 0);
CREATE (normals, LEVEL_TRIANGLES * 3 * 2, 0);
CREATE (color, LEVEL_TRIANGLES * 3, 0);
CREATE (texCoord, LEVEL_TRIANGLES * 2 * 2, 0);
CREATE (ovlTexCoord, LEVEL_TRIANGLES * 2, 0);
CREATE (lMapTexCoord, LEVEL_TRIANGLES * 2, 0);
Init ();
return true;
}

// ----------------------------------------------------------------------------

void CFaceData::Destroy (void)
{
DESTROY (faces);
DESTROY ( tris);
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
nUsedFaces = 0;
nUsedKeys = 0; 
roots.Create ((MAX_WALL_TEXTURES  + MAX_WALL_TEXTURES / 10) * 3);
tails.Create ((MAX_WALL_TEXTURES  + MAX_WALL_TEXTURES / 10) * 3);
usedKeys.Create ((MAX_WALL_TEXTURES  + MAX_WALL_TEXTURES / 10) * 3);
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
nTris = 0;
bHaveSlideSegs = 0;
nSlideSegs = 0;
}

// ----------------------------------------------------------------------------

bool CSegmentData::Create (void)
{
CREATE (gameData.segs.vertices, LEVEL_VERTICES, 0);
CREATE (gameData.segs.fVertices, LEVEL_VERTICES, 0);
CREATE (SEGMENTS, LEVEL_SEGMENTS, 0);
CREATE (SEGMENTS, LEVEL_SEGMENTS, 0);
CREATE (SEGMENTS, LEVEL_SEGMENTS, 0);
CREATE (gameData.segs.points, LEVEL_VERTICES, 0);
#if CALC_SEGRADS
CREATE (gameData.segs.segRads [0], LEVEL_SEGMENTS, 0);
CREATE (gameData.segs.segRads [1], LEVEL_SEGMENTS, 0);
CREATE (gameData.segs.extent, LEVEL_SEGMENTS, 0);
#endif
CREATE (gameData.segs.segCenters [0], LEVEL_SEGMENTS, 0);
CREATE (gameData.segs.segCenters [1], LEVEL_SEGMENTS, 0);
CREATE (gameData.segs.sideCenters, LEVEL_SEGMENTS * 6, 0);
CREATE (gameData.segs.bSegVis, LEVEL_SEGMENTS * LEVEL_SEGVIS_FLAGS, 0);
CREATE (gameData.segs.slideSegs, LEVEL_SEGMENTS, 0);
CREATE (gameData.segs.segFaces, LEVEL_SEGMENTS, 0);
for (int i = 0; i < LEVEL_SEGMENTS; i++)
	SEGMENTS [i].m_objects = -1;
Init ();
return faces.Create ();
}

// ----------------------------------------------------------------------------

void CSegmentData::Destroy (void)
{
DESTROY (gameData.segs.vertices);
DESTROY (gameData.segs.fVertices);
DESTROY (SEGMENTS);
DESTROY (SEGMENTS);
DESTROY (SEGMENTS);
DESTROY (gameData.segs.points);
#if CALC_SEGRADS
DESTROY (gameData.segs.segRads [0]);
DESTROY (gameData.segs.segRads [1]);
DESTROY (gameData.segs.extent);
#endif
DESTROY (gameData.segs.segCenters [0]);
DESTROY (gameData.segs.segCenters [1]);
DESTROY (gameData.segs.sideCenters);
DESTROY (gameData.segs.bSegVis);
DESTROY (gameData.segs.slideSegs);
DESTROY (gameData.segs.segFaces);
faces.Destroy ();
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

void CWallData::Destroy (void)
{
walls.Destroy ();
}

//------------------------------------------------------------------------------

CTriggerData::CTriggerData ()
{
triggers.Create (MAX_TRIGGERS);
objTriggers.Create (MAX_TRIGGERS);
objTriggerRefs.Create (MAX_OBJ_TRIGGERS);
delay.Create (MAX_TRIGGERS);
}

// ----------------------------------------------------------------------------

bool CTriggerData::Create (void)
{
CREATE (firstObjTrigger, LEVEL_OBJECTS, 0xff);
nTriggers = 0;
nObjTriggers = 0;
return true;
}

// ----------------------------------------------------------------------------

void CTriggerData::Destroy (void)
{
firstObjTrigger.Destroy ();
}

// ----------------------------------------------------------------------------

CEffectData::CEffectData ()
{
for (int i = 0; i < 2; i++) {
	effects [i].Create (MAX_EFFECTS);
	vClips [i].Create (VCLIP_MAXNUM);
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
	bmIndex [i].Create (MAX_TEXTURES);
	textureIndex [i].Create (MAX_BITMAP_FILES);
	tMapInfo [i].Create (MAX_TEXTURES + MAX_TEXTURES / 10);	//add some room for extra textures like e.g. from the hoard data
	defaultBrightness [i].Create (MAX_WALL_TEXTURES);
	defaultBrightness [i].Clear ();
	}
objBmIndex.Create (MAX_OBJ_BITMAPS);
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

CMatCenData::CMatCenData ()
{
memset (this, 0, sizeof (*this));
xFuelRefillSpeed = I2X (1);
xFuelGiveAmount = I2X (25);
xFuelMaxAmount = I2X (100);
xEnergyToCreateOneRobot = I2X (1);
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

CBossData::CBossData ()
{
nCloakStartTime = 0;
nCloakEndTime = 0;
nLastTeleportTime = 0;
nTeleportInterval = I2X (8);
nCloakInterval = I2X (10);
nCloakDuration = BOSS_CLOAK_DURATION;
nLastGateTime = 0;
nGateInterval = I2X (6);
}

// ----------------------------------------------------------------------------

CEscortData::CEscortData ()
{
memset (this, 0, sizeof (*this));
nMaxLength = 200;
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
memset (this, 0, sizeof (*this));
xReInitTime = 0x3f000000;
for (int i = 0; i < NDL; i++)
	xWaitTimes [i] = I2X (30 - i * 5);
}

// ----------------------------------------------------------------------------

CMarkerData::CMarkerData ()
{
memset (this, 0, sizeof (*this));
}

// ----------------------------------------------------------------------------

void CMarkerData::Init (void)
{
gameData.marker.nHighlight = -1;
gameData.marker.viewers [0] =
gameData.marker.viewers [1] = -1;
gameData.marker.fScale = 2.0f;
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
}
 
// ----------------------------------------------------------------------------

bool CModelData::Create (void)
{
for (int i = 0; i < 2; i++) {
	aseModels [i].Create (MAX_POLYGON_MODELS);
	oofModels [i].Create (MAX_POLYGON_MODELS);
	for (int j = 0; j < 2; j++)
		pofData [i][j].Create (MAX_POLYGON_MODELS);
	CREATE (modelToOOF [i], MAX_POLYGON_MODELS, 0);
	CREATE (modelToASE [i], MAX_POLYGON_MODELS, 0);
	renderModels [i].Create (MAX_POLYGON_MODELS);
	}
CREATE (bHaveHiresModel, MAX_POLYGON_MODELS, 0);
for (int i = 0; i < 3; i++) {
	polyModels [i].Create (MAX_POLYGON_MODELS);
	for (int j = 0; j < MAX_POLYGON_MODELS; j++)
		polyModels [i][j].SetId (j);
	}
CREATE (modelToPOL, MAX_POLYGON_MODELS, 0);
CREATE (polyModelPoints, MAX_POLYGON_VERTS, 0);
CREATE (fPolyModelVerts, MAX_POLYGON_VERTS, 0);
textures.Create (MAX_POLYOBJ_TEXTURES);
CREATE (textureIndex, MAX_POLYOBJ_TEXTURES, 0xff);
CREATE (nDyingModels, MAX_POLYGON_MODELS, 0xff);
CREATE (nDeadModels, MAX_POLYGON_MODELS, 0xff);
CREATE (hitboxes, MAX_POLYGON_MODELS, 0);
CREATE (thrusters, MAX_POLYGON_MODELS, 0);
CREATE (offsets, MAX_POLYGON_MODELS, 0);
CREATE (gunInfo, MAX_POLYGON_MODELS, 0);
CREATE (spheres, MAX_POLYGON_MODELS, 0);
return true;
}

// ----------------------------------------------------------------------------

CMorphData::CMorphData ()
{
memset (this, 0, sizeof (*this));
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
gameData.objs.bIsMissile.Clear (0);
gameData.objs.bIsMissile [CONCUSSION_ID] =
gameData.objs.bIsMissile [HOMINGMSL_ID] =
gameData.objs.bIsMissile [SMARTMSL_ID] =
gameData.objs.bIsMissile [MEGAMSL_ID] =
gameData.objs.bIsMissile [FLASHMSL_ID] =
gameData.objs.bIsMissile [GUIDEDMSL_ID] =
gameData.objs.bIsMissile [MERCURYMSL_ID] =
gameData.objs.bIsMissile [EARTHSHAKER_ID] =
gameData.objs.bIsMissile [EARTHSHAKER_MEGA_ID] =
gameData.objs.bIsMissile [ROBOT_CONCUSSION_ID] =
gameData.objs.bIsMissile [ROBOT_HOMINGMSL_ID] =
gameData.objs.bIsMissile [ROBOT_FLASHMSL_ID] =
gameData.objs.bIsMissile [ROBOT_MERCURYMSL_ID] =
gameData.objs.bIsMissile [ROBOT_MEGA_FLASHMSL_ID] =
gameData.objs.bIsMissile [ROBOT_SMARTMSL_ID] =
gameData.objs.bIsMissile [ROBOT_MEGAMSL_ID] =
gameData.objs.bIsMissile [ROBOT_EARTHSHAKER_ID] =
gameData.objs.bIsMissile [ROBOT_SHAKER_MEGA_ID] = 1;

gameData.objs.bIsWeapon.Clear (0);
gameData.objs.bIsWeapon [VULCAN_ID] =
gameData.objs.bIsWeapon [GAUSS_ID] =
gameData.objs.bIsWeapon [ROBOT_VULCAN_ID] = 1;
gameData.objs.bIsWeapon [LASER_ID] =
gameData.objs.bIsWeapon [LASER_ID + 1] =
gameData.objs.bIsWeapon [LASER_ID + 2] =
gameData.objs.bIsWeapon [LASER_ID + 3] =
gameData.objs.bIsWeapon [REACTOR_BLOB_ID] =
gameData.objs.bIsWeapon [ROBOT_LIGHT_FIREBALL_ID] =
gameData.objs.bIsWeapon [SMARTMSL_BLOB_ID] =
gameData.objs.bIsWeapon [SMARTMINE_BLOB_ID] =
gameData.objs.bIsWeapon [ROBOT_SMARTMINE_BLOB_ID] =
gameData.objs.bIsWeapon [FLARE_ID] =
gameData.objs.bIsWeapon [SPREADFIRE_ID] =
gameData.objs.bIsWeapon [PLASMA_ID] =
gameData.objs.bIsWeapon [FUSION_ID] =
gameData.objs.bIsWeapon [SUPERLASER_ID] =
gameData.objs.bIsWeapon [SUPERLASER_ID + 1] =
gameData.objs.bIsWeapon [HELIX_ID] =
gameData.objs.bIsWeapon [PHOENIX_ID] =
gameData.objs.bIsWeapon [OMEGA_ID] =
gameData.objs.bIsWeapon [ROBOT_PLASMA_ID] =
gameData.objs.bIsWeapon [ROBOT_MEDIUM_FIREBALL_ID] =
gameData.objs.bIsWeapon [ROBOT_SMARTMSL_BLOB_ID] =
gameData.objs.bIsWeapon [ROBOT_TI_STREAM_ID] =
gameData.objs.bIsWeapon [ROBOT_PHOENIX_ID] =
gameData.objs.bIsWeapon [ROBOT_FAST_PHOENIX_ID] =
gameData.objs.bIsWeapon [ROBOT_PHASE_ENERGY_ID] =
gameData.objs.bIsWeapon [ROBOT_MEGA_FLASHMSL_ID] =
gameData.objs.bIsWeapon [ROBOT_MEGA_FLASHMSL_ID + 1] =
gameData.objs.bIsWeapon [ROBOT_VERTIGO_FIREBALL_ID] =
gameData.objs.bIsWeapon [ROBOT_VERTIGO_PHOENIX_ID] =
gameData.objs.bIsWeapon [ROBOT_HELIX_ID] =
gameData.objs.bIsWeapon [ROBOT_BLUE_ENERGY_ID] =
gameData.objs.bIsWeapon [ROBOT_WHITE_ENERGY_ID] =
gameData.objs.bIsWeapon [ROBOT_BLUE_LASER_ID] =
gameData.objs.bIsWeapon [ROBOT_RED_LASER_ID] =
gameData.objs.bIsWeapon [ROBOT_GREEN_LASER_ID] =
gameData.objs.bIsWeapon [ROBOT_WHITE_LASER_ID] = 1;

gameData.objs.bIsSlowWeapon.Clear (0);
gameData.objs.bIsSlowWeapon [VULCAN_ID] =
gameData.objs.bIsSlowWeapon [GAUSS_ID] =
gameData.objs.bIsSlowWeapon [ROBOT_VULCAN_ID] = 1;
gameData.objs.bIsSlowWeapon [REACTOR_BLOB_ID] =
gameData.objs.bIsSlowWeapon [ROBOT_TI_STREAM_ID] =
gameData.objs.bIsSlowWeapon [SMARTMINE_BLOB_ID] =
gameData.objs.bIsSlowWeapon [ROBOT_SMARTMINE_BLOB_ID] =
gameData.objs.bIsSlowWeapon [SMARTMSL_BLOB_ID] =
gameData.objs.bIsSlowWeapon [ROBOT_SMARTMSL_BLOB_ID] = 1;
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
nMaxObjects = max (gameFileInfo.objects.count + 1000, gameFileInfo.objects.count * 2);
nMaxUsedObjects = LEVEL_OBJECTS - 20;
}

// ----------------------------------------------------------------------------

bool CObjectData::Create (void)
{
Init ();
CREATE (gameData.objs.objects, LEVEL_OBJECTS, 0);
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
CREATE (gameData.objs.bWantEffect, LEVEL_OBJECTS, (char) 0xFF);
for (int i = 0; i < LEVEL_OBJECTS; i++) {
	gameData.objs.freeList [i] = i;
	OBJECTS [i].Init ();
	}
return lightClusterManager.Init () && shrapnelManager.Init ();
}

// ----------------------------------------------------------------------------

void CObjectData::Destroy (void)
{
DESTROY (gameData.objs.objects);
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
lightManager.Reset ();
shrapnelManager.Reset ();
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
xTime = 0;
xAfterburnerCharge = I2X (1);
xBossInvulDot = 0;
playerThrust.SetZero ();
}

// ----------------------------------------------------------------------------

bool CPhysicsData::Create (void)
{
CREATE (gameData.physics.ignoreObjs, LEVEL_OBJECTS, 0);
return true;
}

// ----------------------------------------------------------------------------

void CPhysicsData::Destroy (void)
{
DESTROY (gameData.physics.ignoreObjs);
}

// ----------------------------------------------------------------------------

bool CWeaponData::Create (void)
{
CREATE (color, LEVEL_OBJECTS, 0);
for (int i = 0; i < LEVEL_OBJECTS; i++)
	color [i].red =
	color [i].green =
	color [i].blue = 1.0;
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
CREATE (gameData.multigame.remoteToLocal, MAX_NUM_NET_PLAYERS * LEVEL_OBJECTS, 0);  // Remote CObject number for each local CObject
CREATE (gameData.multigame.localToRemote, LEVEL_OBJECTS, 0);
CREATE (gameData.multigame.nObjOwner, LEVEL_OBJECTS, 0);   // Who created each CObject in my universe, -1 = loaded at start
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
gameData.ai.nDistToLastPlayerPosFiredAt = 0;
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
strcpy (szEndingFilename, "endreg.txt");
strcpy (szBriefingFilename, "briefing.txt");
}

// ----------------------------------------------------------------------------

CScoreData::CScoreData ()
{
nKillsChanged = 0;
bNoMovieMessage = 0;
nPhallicLimit = 0;
bWaitingForOthers = 0;
nPhallicMan = -1;
}

// ----------------------------------------------------------------------------

CTimeData::CTimeData ()
{ 
memset (this, 0, sizeof (*this)); 
xFrame = 0x1000;
}

// ----------------------------------------------------------------------------

CTerrainRenderData::CTerrainRenderData ()
{
memset (this, 0, sizeof (*this));
uvlList [0][1].u = I2X (1);
uvlList [0][2].v = I2X (1);
uvlList [1][0].u = I2X (1);
uvlList [1][1].u = I2X (1);
uvlList [1][1].v = I2X (1);
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

bool CGameData::Create (void)
{
Destroy ();
if (!(gameData.segs.Create () &&
		gameData.objs.Create () &&
		gameData.trigs.Create () &&
		gameData.render.color.Create () &&
		gameData.render.lights.Create () &&
		gameData.render.shadows.Create () &&
		gameData.render.Create () &&
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
gameData.marker.Init ();
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
particleManager.Shutdown ();
lightningManager.Shutdown (1);
}

// ----------------------------------------------------------------------------

void FreeGameData (void)
{
PrintLog ("shutting down lightning manager\n");
lightningManager.Shutdown (1);
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
gameData.eff.effects [gameStates.app.bD1Data].ShareBuffer (gameData.eff.effectP);
gameData.eff.vClips [gameStates.app.bD1Data].ShareBuffer (gameData.eff.vClipP);
gameData.walls.anims [gameStates.app.bD1Data].ShareBuffer (gameData.walls.animP);
gameData.bots.info [gameStates.app.bD1Data].ShareBuffer (gameData.bots.infoP);
}

// ----------------------------------------------------------------------------
