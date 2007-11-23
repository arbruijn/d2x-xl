/*
Here is all the code for gameOpts->render.color.bUseLightMaps.  Some parts need 
to be optamized but the core functionality is there.
The thing you will need to add is the menu code to change
the value of bool gameOpts->render.color.bUseLightMaps.  THIS CANNOT BE CHANGED IF
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
#include "ogl_shader.h"
#include "lighting.h"
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

int numLightMaps; 
tOglTexture *lightMaps = NULL;  //Level Lightmaps
tLightMap *lightData = NULL;  //Level lights

#define TEXTURE_CHECK 1

int InitLightData (void);

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
	LogErr ("building lightmap shader programs\n");
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

inline void FindOffset (vmsVector *outvec, vmsVector vec1, vmsVector vec2, double f_offset)
{
outvec->p.x = (fix) (f_offset * (vec2.p.x - vec1.p.x)); 
outvec->p.y = (fix) (f_offset * (vec2.p.y - vec1.p.y)); 
outvec->p.z = (fix) (f_offset * (vec2.p.z - vec1.p.z)); 
}

//------------------------------------------------------------------------------

#ifdef _DEBUG
char sideFlags [MAX_SEGMENTS_D2X * 6]; 
#endif

int CountLights (void)
{
	int		nLights = 0;
	short		segNum, sideNum; 
	tSegment	*segP;
	tSide		*sideP;

if (!(gameOpts->render.color.bUseLightMaps && gameStates.render.color.bLightMapsOk))
	return 0;
#ifdef _DEBUG
memset (sideFlags, 0, sizeof (sideFlags)); 
#endif
for (segNum = 0, segP = gameData.segs.segments; 
	  segNum <= gameData.segs.nLastSegment; 
	  segNum++, segP++) {
	for (sideNum = 0, sideP = segP->sides; sideNum < 6; sideNum++, sideP++) {
#if TEXTURE_CHECK
		if ((gameData.segs.segments [segNum].children [sideNum] >= 0) && 
			 !IS_WALL (WallNumI (segNum, sideNum)))
			continue; 	//skip open sides
#endif
		if (IsLight (sideP->nBaseTex) || IsLight (sideP->nOvlTex) ||
			 (gameStates.app.bD2XLevel && gameData.render.color.lights [segNum * 6 + sideNum].index)) {
			nLights++;
#ifdef _DEBUG
			sideFlags [segNum * 6 + sideNum] = 1; 
#endif
			}
		}
	}	
return nLights; 
}

//------------------------------------------------------------------------------

double SideRad (int segNum, int sideNum)
{
	int			i;
	double		h, xMin, xMax, yMin, yMax, zMin, zMax;
	double		dx, dy, dz;
	short			sideVerts [4];
	vmsVector	*v;

GetSideVertIndex (sideVerts, segNum, sideNum); 
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

void DestroyLightMaps (void)
{
if (lightData) { //! =  NULL, comparison not explicitly required 
		int	i;

	D2_FREE (lightData); 
	lightData = NULL; //init to a defined value 
	for (i = 0; i < MAX_SEGMENTS * 6; i++)
		if (lightMaps [i].handle)
			glDeleteTextures (1, (GLuint *) &lightMaps [i].handle);
	memset (lightMaps, 0, sizeof (lightMaps));
	} 
}

//------------------------------------------------------------------------------

int IsBigLight (int tMapNum)
{
if (gameStates.app.bD1Mission)
	tMapNum = ConvertD1Texture (tMapNum, 1);
switch (tMapNum) {
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

int GetLightColor (int tMapNum, tLightMap *pTempLight)
{
	tLightMap	tempLight;
	int			bIsLight = 0;
	double		baseRange = LightMapRange ();

if (gameStates.app.bD1Mission) {
#ifdef _DEBUG
	if (tMapNum == 328)
		tMapNum = tMapNum;
	else if (tMapNum == 281)
		tMapNum = tMapNum;
#endif
	tMapNum = ConvertD1Texture (tMapNum, 1);
	}
switch (tMapNum) {
	case 275:
	case 276:
	case 278:
	case 288:
		bIsLight = 1; 	
		tempLight.color [0]  = 
		tempLight.color [1] = (GLfloat) 1.0; 
		tempLight.color [2] = (GLfloat) 0.75; 
		tempLight.range = baseRange; 
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
		tempLight.color [0] = 
		tempLight.color [1] = 
		tempLight.color [2] = (GLfloat) 1; 
		tempLight.range = baseRange; 
		break; 
	case 340:
	case 341:
	case 345:
	case 382:
		bIsLight = 1; 			
		tempLight.color [0] = 
		tempLight.color [1] = 
		tempLight.color [2] = (GLfloat) 1; 
		tempLight.range = baseRange / 2; 
		break; 
	case 343:
	case 344:
	case 377:
		bIsLight = 1; 	
		tempLight.color [0] = 
		tempLight.color [1] = (GLfloat) 0.75; 
		tempLight.color [2] = (GLfloat) 1; 
		tempLight.range = baseRange / 2; 
		break; 
	case 346:
		bIsLight = 1; 			
		tempLight.color [0] = (GLfloat) 0.5; 
		tempLight.color [1] = (GLfloat) 1; 
		tempLight.color [2] = (GLfloat) 0.5; 
		tempLight.range = baseRange / 2; 
		break; 
	case 351:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 0.95; 
		tempLight.color [1] = (GLfloat) 0.84; 
		tempLight.color [2] = (GLfloat) 0.56; 
		tempLight.range = baseRange / 2; 
		break; 
	case 352:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 1; 
		tempLight.color [1] = 
		tempLight.color [2] = (GLfloat) 0.25; 
		tempLight.range = baseRange; 
		break; 
	case 364:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 0.1; 
		tempLight.color [1] = (GLfloat) 0.9; 
		tempLight.color [2] = (GLfloat) 0.1; 
		tempLight.range = baseRange / 2; 
		break; 
	case 366:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 1; 
		tempLight.color [1] = (GLfloat) 0.9; 
		tempLight.color [2] = (GLfloat) 1; 
		tempLight.range = baseRange / 2; 
		break; 
	case 368:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 1; 
		tempLight.color [1] = 
		tempLight.color [2] = (GLfloat) 0.5; 
		tempLight.range = baseRange / 2; 
		break; 
	case 370:
	case 372:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 0.95; 
		tempLight.color [1] = (GLfloat) 0.84; 
		tempLight.color [2] = (GLfloat) 0.56; 
		tempLight.range = baseRange / 2; 
		break; 
	case 380:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 0.65; 
		tempLight.color [1] = (GLfloat) 0.6; 
		tempLight.color [2] = (GLfloat) 0.75; 
		tempLight.range = baseRange / 2; 
		break; 
	case 410:
	case 427:
		bIsLight = 1; 	
		tempLight.color [0] = 
		tempLight.color [1] = (GLfloat) 0.25; 
		tempLight.color [2] = (GLfloat) 1; 
		tempLight.range = baseRange / 2; 
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
		tempLight.color [0] = (GLfloat) 1; 
		tempLight.color [1] = 
		tempLight.color [2] = (GLfloat) 0.25; 
		tempLight.range = baseRange / 2; 
		break; 
	case 411:
	case 428:
		bIsLight = 1; 	
		tempLight.color [0] = 
		tempLight.color [1] = (GLfloat) 1; 
		tempLight.color [2] = (GLfloat) 0.25; 
		tempLight.range = baseRange / 2; 
		break; 
	case 414:
	case 416:
	case 418:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 1.0; 
		tempLight.color [1] = (GLfloat) 0.75; 
		tempLight.color [2] = (GLfloat) 0.25; 
		tempLight.range = baseRange / 3; 
		break; 
	case 423:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 0.25; 
		tempLight.color [1] = (GLfloat) 0.75; 
		tempLight.color [2] = (GLfloat) 0.25; 
		tempLight.range = baseRange; 
		break; 
	case 424:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 0.85; 
		tempLight.color [1] = (GLfloat) 0.65; 
		tempLight.color [2] = (GLfloat) 0.25; 
		tempLight.range = baseRange; 
		break; 
	case 235:
	case 236:
	case 237:
	case 243:
	case 244:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 1.0; 
		tempLight.color [1] = (GLfloat) 0.95; 
		tempLight.color [2] = (GLfloat) 1.0; 
		tempLight.range = baseRange; 
		break; 
	case 333:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 0.5; 
		tempLight.color [1] = (GLfloat) 0.33; 
		tempLight.color [2] = (GLfloat) 0; 
		tempLight.range = baseRange / 2; 
		break; 
	case 353:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 0.75; 
		tempLight.color [1] = (GLfloat) 0.375; 
		tempLight.color [2] = (GLfloat) 0; 
		tempLight.range = baseRange / 2; 
		break; 
	case 356:
	case 357:
	case 358:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 1; 
		tempLight.color [1] = 
		tempLight.color [2] = (GLfloat) 0.25; 
		tempLight.range = baseRange; 
		break; 
	case 359:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 1; 
		tempLight.color [1] = 
		tempLight.color [2] = (GLfloat) 0; 
		tempLight.range = baseRange / 2; 
		break; 
	//Lava gameData.pig.tex.bmIndex
	case 378:
	case 404:
	case 405:
	case 406:
	case 407:
	case 408:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 1; 
		tempLight.color [1] = (GLfloat) 0.5; 
		tempLight.color [2] = (GLfloat) 0; 
		tempLight.range = baseRange; 
		break; 
	case 409:
	case 426:
	case 434:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 1; 
		tempLight.color [1] = (GLfloat) 0.25; 
		tempLight.color [2] = (GLfloat) 0; 
		tempLight.range = baseRange; 
		break; 
	case 420:
	case 432:
	case 433:
		bIsLight = 1; 	
		tempLight.color [0] = (GLfloat) 0; 
		tempLight.color [1] = (GLfloat) 0.25; 
		tempLight.color [2] = (GLfloat) 1; 
		tempLight.range = baseRange; 
		break; 
	default:
		if (gameData.pig.tex.pTMapInfo [tMapNum].lighting) {
			bIsLight = 1; 	
			tempLight.color [0] = 
			tempLight.color [1] = 
			tempLight.color [2] = (GLfloat) 1; 
			tempLight.range = baseRange; 
			}
		break; 
	}
if (bIsLight) {
#if 1
#	if 0
	if (gameData.pig.tex.tMapInfo [tMapNum].lighting)
		tempLight.range = baseRange * (double) gameData.pig.tex.tMapInfo [tMapNum].lighting / (double) (F1_0 / 2);
#	endif
	*pTempLight = tempLight;
#else
	if (gameData.pig.tex.tMapInfo [tMapNum].lighting)
		*pTempLight = tempLight;
	else
		bIsLight = 0;
#endif
	}
return bIsLight;
}

//------------------------------------------------------------------------------

int InitLightData (void)
{
	short			segNum, sideNum; 
	short			sideVerts [4]; 
	int			bIsLight; 
	short			tMapNum; 
	tLightMap	tempLight;  //temporary place to put light data.
	double		sideRad;
	double		baseRange = LightMapRange ();

//first step find all the lights in the level.  By iterating through every surface in the level.
if (!(numLightMaps = CountLights ()))
	return 0;
if (!(lightData = D2_ALLOC (sizeof (tLightMap) * numLightMaps)))
	return numLightMaps = 0; 
if (!(lightMaps = (tOglTexture *) D2_ALLOC (MAX_SEGMENTS * 6 * sizeof (*lightMaps)))) {
	D2_FREE (lightData);
	return numLightMaps = 0; 
	}
memset (lightMaps, 0, sizeof (*lightMaps) * MAX_SEGMENTS * 6); 
memset (lightData, 0, sizeof (tLightMap) * numLightMaps); 
numLightMaps = 0; 
for (segNum = 0; segNum <= gameData.segs.nLastSegment; segNum++) {
	for (sideNum = 0; sideNum < 6; sideNum++) {
		//check for point lights
#if TEXTURE_CHECK
		if ((gameData.segs.segments [segNum].children [sideNum] >= 0) && 
			 !IS_WALL (WallNumI (segNum, sideNum)))
			continue; 	//skip open sides
#endif
		bIsLight = 0; 
		sideRad = 0;
		tMapNum = gameData.segs.segments [segNum].sides [sideNum].nOvlTex;
		if (GetLightColor (tMapNum, &tempLight)) {
			bIsLight = 1;
			//if (IsBigLight (tMapNum))
				sideRad = SideRad (segNum, sideNum);
			}
		//then look at the base - will override an overlaying lightopTex.
		tMapNum = gameData.segs.segments [segNum].sides [sideNum].nBaseTex;
		if (GetLightColor (tMapNum, &tempLight)) {
			bIsLight = 1;
			//if (IsBigLight (tMapNum))
				sideRad = SideRad (segNum, sideNum);
			}
		if (gameStates.app.bD2XLevel && gameData.render.color.lights [segNum * 6 + sideNum].index) { 
			bIsLight = 1;
			tempLight.color [0] = (GLfloat) gameData.render.color.lights [segNum * 6 + sideNum].color.red; 
			tempLight.color [1] = (GLfloat) gameData.render.color.lights [segNum * 6 + sideNum].color.green; 
			tempLight.color [2] = (GLfloat) gameData.render.color.lights [segNum * 6 + sideNum].color.blue; 
			tempLight.range = baseRange; 
			}
		//Process found light.
		if (bIsLight) {
#ifdef _DEBUG
			if (!sideFlags [segNum * 6 + sideNum])
				continue; 
#endif
			tempLight.range += sideRad;
			//find where it is in the level.
			GetSideVertIndex (sideVerts, segNum, sideNum); 
			//this is just temporary need to find actual lights on surfaces
			VmVecAvg4 (
				&lightData [numLightMaps].pos, 
				gameData.segs.vertices + sideVerts [0], 
				gameData.segs.vertices + sideVerts [2], 
				gameData.segs.vertices + sideVerts [3], 
				gameData.segs.vertices + sideVerts [1]); 
#if 1
			memcpy (&lightData [numLightMaps].color, &tempLight.color, sizeof (tempLight.color)); 
#else
			lightData [numLightMaps].color [0] = tempLight.color [0]; 
			lightData [numLightMaps].color [1] = tempLight.color [1]; 
			lightData [numLightMaps].color [2] = tempLight.color [2]; 
#endif
			lightData [numLightMaps].range = tempLight.range; 
			lightData [numLightMaps].refside = segNum * 6 + sideNum; 

			//find light direction, currently based on first 3 points of tSide, not always right.
			VmVecNormal (
				&lightData [numLightMaps].dir, 
				gameData.segs.vertices + sideVerts [0], 
				gameData.segs.vertices + sideVerts [1], 
				gameData.segs.vertices + sideVerts [2]);
			numLightMaps++; 
			}
		}
	}
return numLightMaps; 
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
void InitLightMap (ubyte *lightMap, ubyte *brightMap, GLfloat *color)
{
	int		i, j;
	double	h;

for (i = 512; i; i--, brightMap++)
	for (j = 0; j < 3; j++, lightMap++)
		*lightMap = (ubyte) (*brightMap * color [j] + 0.5);
}

#endif //LMAP_REND2TEX

//------------------------------------------------------------------------------

void ComputeLightMaps (int segNum)
{
	tSegment		*segP; 
	tSide			*sideP; 
	tLightMap	*lmapP; 
	int			sideNum, lastSeg, mapNum; 
	short			sideVerts [4]; 

#if 1
#	define		Xs 8
#	define		Ys 8
#else
	int			Xs = 8, Ys = 8; 
#endif
	int			x, y, xy; 
	int			v0, v1, v2, v3; 
	GLfloat		*pTexColor;// = {0.0, 0.0, 0.0, 1.0}; 
	GLfloat		texColor [Xs][Ys][4];

#if 1
#	define		pixelOffset 0.0
#else
	double		pixelOffset = 0; //0.5
#endif
	int			l, s, nMethod, sideRad; 
	GLfloat		tempBright = 0; 
	vmsVector	OffsetU, OffsetV, pixelPos [Xs][Ys], *pPixelPos, rayVec, faceNorm, sidePos; 
	double		brightPrct, pixelDist; 
	double		delta; 
	double		f_offset [8] = {
						0.0 / (Xs - 1), 1.0 / (Xs - 1), 2.0 / (Xs - 1), 3.0 / (Xs - 1),
						4.0 / (Xs - 1), 5.0 / (Xs - 1), 6.0 / (Xs - 1), 7.0 / (Xs - 1)
						};
#if LMAP_REND2TEX
	ubyte			brightMap [512];
	ubyte			lightMap [512*3];
	tUVL			lMapUVL [4];
	fix			nDist, nMinDist;
	GLuint		lightMapId;
	int			bStart;
#endif

if (segNum <= 0) {
	DestroyLightMaps ();
	if (!InitLightData ())
		return;
#if LMAP_REND2TEX
	InitBrightMap (brightMap);
	memset (&lMapUVL, 0, sizeof (lMapUVL));
#endif
	}
INIT_PROGRESS_LOOP (segNum, lastSeg, gameData.segs.nSegments);
//Next Go through each surface and create a lightmap for it.
for (mapNum = 6 * segNum, segP = gameData.segs.segments + segNum; 
	  segNum < lastSeg; 
	  segNum++, segP++) {
	for (sideNum = 0, sideP = segP->sides; sideNum < 6; sideNum++, mapNum++, sideP++) {
#if TEXTURE_CHECK
		if ((segP->children [sideNum] >= 0) && !IS_WALL (WallNumS (sideP)))
			continue; 	//skip open sides
#endif			
		GetSideVertIndex (sideVerts, segNum, sideNum); 
#if LMAP_REND2TEX
		OglCreateFBuffer (&lightMaps [mapNum].fbuffer, 64, 64);
		OglEnableFBuffer (&lightMaps [mapNum].fbuffer);
#else
		lightMaps [mapNum].handle = EmptyTexture (Xs, Ys); 
		OGL_BINDTEX (lightMaps [mapNum].handle); 
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		nMethod = (sideP->nType == SIDE_IS_QUAD) || (sideP->nType == SIDE_IS_TRI_02);
		pPixelPos = &pixelPos [0][0];
		for (x = 0; x < Xs; x++) {
			for (y = 0; y < Ys; y++, pPixelPos++) {
				if (nMethod) {
					v0 = sideVerts [0]; 
					v2 = sideVerts [2]; 
					if (x >= y)	{
						v1 = sideVerts [1]; 
						//Next calculate this pixel's place in the world (tricky stuff)
						FindOffset (&OffsetU, gameData.segs.vertices [v0], gameData.segs.vertices [v1], f_offset [x]); //(((double) x) + pixelOffset) / (Xs - 1)); //took me forever to figure out this should be an inverse thingy
						FindOffset (&OffsetV, gameData.segs.vertices [v1], gameData.segs.vertices [v2], f_offset [y]); //(((double) y) + pixelOffset) / (Ys - 1)); 
						VmVecAdd (pPixelPos, &OffsetU, &OffsetV); 
						VmVecInc (pPixelPos, gameData.segs.vertices + v0);  //This should be the real world position of the pixel.
						//Find Normal
						VmVecNormal (&faceNorm, gameData.segs.vertices + v0, gameData.segs.vertices + v2, gameData.segs.vertices + v1); 
						}
					else {
						//Next calculate this pixel's place in the world (tricky stuff)
						v3 = sideVerts [3]; 
						FindOffset (&OffsetV, gameData.segs.vertices [v0], gameData.segs.vertices [v3], f_offset [y]); //(((double) y) + pixelOffset) / (Xs - 1)); //Notice y/x and OffsetU/OffsetV are swapped from above
						FindOffset (&OffsetU, gameData.segs.vertices [v3], gameData.segs.vertices [v2], f_offset [x]); //(((double) x) + pixelOffset) / (Ys - 1)); 
						VmVecAdd (pPixelPos, &OffsetU, &OffsetV); 
						VmVecInc (pPixelPos, gameData.segs.vertices + v0);  //This should be the real world position of the pixel.
						VmVecNormal (&faceNorm, gameData.segs.vertices + v0, gameData.segs.vertices + v3, gameData.segs.vertices + v2); 
						}
					}
				else {//SIDE_IS_TRI_02
					v1 = sideVerts [1]; 
					v3 = sideVerts [3]; 
					if (Xs - x >= y) {
						v0 = sideVerts [0]; 
						FindOffset (&OffsetU, gameData.segs.vertices [v0], gameData.segs.vertices [v1], f_offset [x]); //(((double) x) + pixelOffset) / (Xs - 1)); 
						FindOffset (&OffsetV, gameData.segs.vertices [v0], gameData.segs.vertices [v3], f_offset [y]); //(((double) y) + pixelOffset) / (Xs - 1)); 
						VmVecAdd (pPixelPos, &OffsetU, &OffsetV); 
						VmVecInc (pPixelPos, gameData.segs.vertices + v0);  //This should be the real world position of the pixel.
						}
					else {
						v2 = sideVerts [2]; 
						//Not certain this is correct, may need to subtract something
						FindOffset (&OffsetV, gameData.segs.vertices [v2], gameData.segs.vertices [v1], f_offset [Xs - 1 - y]); //((double) ((Xs - 1) - y) + pixelOffset) / (Xs - 1)); 
						FindOffset (&OffsetU, gameData.segs.vertices [v2], gameData.segs.vertices [v3], f_offset [Xs - 1 - x]); //((double) ((Xs - 1) - x) + pixelOffset) / (Xs - 1)); 
						VmVecAdd (pPixelPos, &OffsetU, &OffsetV); 
						VmVecInc (pPixelPos, gameData.segs.vertices + v2);  //This should be the real world position of the pixel.
						}
					}
				}
			}
#endif
		//Calculate LightVal
		//Next iterate through all the lights and add the light to the pixel every iteration.
		sideRad = (int) (SideRad (segNum, sideNum) + 0.5);
		VmVecAvg4 (
			&sidePos, 
			&pixelPos [0][0],
			&pixelPos [Xs-1][0],
			&pixelPos [Xs-1][Ys-1],
			&pixelPos [0][Ys-1]);
#if 1
		pTexColor = texColor [0][0] + 3;
		memset (texColor, 0, sizeof (texColor));
		for (xy = Xs * Ys; xy; xy--, pTexColor += 4)
			*pTexColor = 1;
#else
		pTexColor = texColor [0][0];
		for (x = 0; x < Xs; x++) {
			for (y = 0; y < Ys; y++, pTexColor += 4) {
				pTexColor [0] = 
				pTexColor [1] = 
				pTexColor [2] = 0; 
				pTexColor [3] = 1; 
				}
			}
#endif
#if LMAP_REND2TEX
		bStart = 1;
#endif
		for (l = 0, lmapP = lightData; l < numLightMaps; l++, lmapP++) {
#if LMAP_REND2TEX
			nMinDist = 0x7FFFFFFF;
			// get the distances of all 4 tSide corners to the light source center 
			// scaled by the light source range
			for (i = 0; i < 4; i++) {
				int svi = sideVerts [i];
				sidePos.x = gameData.segs.vertices [svi].x;
				sidePos.y = gameData.segs.vertices [svi].y;
				sidePos.z = gameData.segs.vertices [svi].z;
				nDist = f2i (VmVecDist (&sidePos, &lmapP->pos));	// calc distance
				if (nMinDist > nDist)
					nMinDist = nDist;
				lMapUVL [i].u = F1_0 * (double) nDist / (double) lmapP->range;	// scale distance
				}
			if ((lmapP->color [0] + lmapP->color [1] + lmapP->color [2] < 3) &&
				(nMinDist < lmapP->range + sideRad)) {
				// create and initialize an OpenGL texture for the lightmap
				InitLightMap (lightMap, brightMap, lmapP->color);
				glGenTextures (1, &lightMapId); 
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
				glDeleteTextures (1, &lightMapId);
				bStart = 0;
				}
#else
			if (f2i (VmVecDist (&sidePos, &lmapP->pos)) < lmapP->range + sideRad) {
				pPixelPos = &pixelPos [0][0];
				pTexColor = texColor [0][0];
#if 1
				for (xy = Xs * Ys; xy; xy--, pPixelPos++, pTexColor += 4) { 
#else
				for (x = 0; x < Xs; x++)
					for (y = 0; y < Ys; y++, pPixelPos++, pTexColor += 4) {
#endif
						//Find angle to this light.
						pixelDist = f2i (VmVecDist (pPixelPos, &lmapP->pos)); 
						if (pixelDist >= lmapP->range)
							continue;
						VmVecSub (&rayVec, &lmapP->pos, pPixelPos); 
						delta = f2db (VmVecDeltaAng (&lmapP->dir, &rayVec, NULL)); 
						if (delta < 0)
							delta = -delta; 
						brightPrct = 1 - (pixelDist / lmapP->range); 
						brightPrct *= brightPrct; //square result
						if (delta < 0.245)
							brightPrct /= 4; 
						pTexColor [0] += (GLfloat) (brightPrct * lmapP->color [0]); 
						pTexColor [1] += (GLfloat) (brightPrct * lmapP->color [1]); 
						pTexColor [2] += (GLfloat) (brightPrct * lmapP->color [2]); 
						}
				}
#endif
			}
#if LMAP_REND2TEX
		lightMaps [mapNum].handle = lightMaps [mapNum].fbuffer.texId;
		lightMaps [mapNum].fbuffer.texId = 0;
		OglDestroyFBuffer (&lightMaps [mapNum].fbuffer);
#else
		pPixelPos = &pixelPos [0][0];
		pTexColor = texColor [0][0];
		for (x = 0; x < Xs; x++)
			for (y = 0; y < Ys; y++, pPixelPos++, pTexColor += 4) {
				tempBright = 0;
				for (s = 0; s < 3; s++)
					if (pTexColor [s] > tempBright)
						tempBright = pTexColor [s]; 
				if (tempBright > 1.0)
					for (s = 0; s < 3; s++)
						pTexColor [s] /= tempBright; 
				glTexSubImage2D (GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, pTexColor); 
				}
#endif
		}
	}
}

//------------------------------------------------------------------------------

int HaveLightMaps (void)
{
return (lightData != NULL);
}

//------------------------------------------------------------------------------

static int segNum = 0;

static void CreateLightMapsPoll (int nItems, tMenuItem *m, int *key, int cItem)
{
GrPaletteStepLoad (NULL);
if (segNum < gameData.segs.nSegments) {
	ComputeLightMaps (segNum);
	segNum += PROGRESS_INCR;
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

void CreateLightMaps ()
{
DestroyLightMaps ();
if (gameStates.render.color.bLightMapsOk && 
	 gameOpts->render.color.bUseLightMaps && 
	 gameData.segs.nSegments) {
	if (gameStates.app.bProgressBars && gameOpts->menus.nStyle) {
		segNum = 0;
		NMProgressBar (TXT_CALC_LIGHTMAPS, 0, PROGRESS_STEPS (gameData.segs.nSegments), 
							CreateLightMapsPoll);
		}
	else
		ComputeLightMaps (-1);
	}
}

//------------------------------------------------------------------------------

