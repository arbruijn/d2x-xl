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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "descent.h"
#include "error.h"
#include "texmap.h"
#include "iff.h"
#include "u_mem.h"
#include "endlevel.h"
#include "rendermine.h"
#include "ogl_render.h"

#define GRID_SIZE				(gameData.render.terrain.nGridW * gameData.render.terrain.nGridH)
#define GRID_OFFS(_i,_j)	((_i) * gameData.render.terrain.nGridW + (_j))
#define HEIGHT(_i,_j)		(gameData.render.terrain.heightmap [GRID_OFFS (_i,_j)])
#define LIGHT(_i,_j)			(gameData.render.terrain.lightmap [GRID_OFFS (_i,_j)])

//!!#define HEIGHT (_i, _j)   gameData.render.terrain.heightmap [(gameData.render.terrain.nGridH - 1-j)*gameData.render.terrain.nGridW+ (_i)]
//!!#define LIGHT (_i, _j)    gameData.render.terrain.lightmap [(gameData.render.terrain.nGridH - 1-j)*gameData.render.terrain.nGridW+ (_i)]

#define LIGHTVAL(_i,_j) (((fix) LIGHT (_i,_j))) // << 8)


// LINT: adding function prototypes
void BuildTerrainLightmap (void);
void _CDECL_ FreeTerrainLightmap (void);

// ------------------------------------------------------------------------

void DrawTerrainCell (int32_t i, int32_t j, CRenderPoint *p0, CRenderPoint *p1, CRenderPoint *p2, CRenderPoint *p3)
{
	CRenderPoint *pointList [3];

p0->SetIndex (-1);
p1->SetIndex (-1);
p2->SetIndex (-1);
p3->SetIndex (-1);  
pointList [0] = p0;
pointList [1] = p1;
pointList [2] = p3;
gameData.render.terrain.uvlList [0][0].l = LIGHTVAL (i, j);
gameData.render.terrain.uvlList [0][1].l = LIGHTVAL (i, j + 1);
gameData.render.terrain.uvlList [0][2].l = LIGHTVAL (i + 1, j);

gameData.render.terrain.uvlList [0][0].u =
gameData.render.terrain.uvlList [0][1].u = (i) * (I2X (1) / 4);
gameData.render.terrain.uvlList [0][0].v =
gameData.render.terrain.uvlList [0][2].v = (j) * (I2X (1) / 4);
gameData.render.terrain.uvlList [0][1].v = (j + 1) * (I2X (1) / 4);
gameData.render.terrain.uvlList [0][2].u = (i + 1) * (I2X (1) / 4);
#if DBG
G3DrawTexPoly (3, pointList, gameData.render.terrain.uvlList [0], gameData.render.terrain.pBm, NULL, 1, 0, -1);
#else
G3CheckAndDrawTMap (3, pointList, gameData.render.terrain.uvlList [0], gameData.render.terrain.pBm, NULL, NULL);
#endif
if (gameData.render.terrain.bOutline) {
	int32_t lSave = gameStates.render.nLighting;
	gameStates.render.nLighting = 0;
	CCanvas::Current ()->SetColorRGB (255, 0, 0, 255);
	G3DrawLine (pointList [0], pointList [1]);
	G3DrawLine (pointList [2], pointList [0]);
	gameStates.render.nLighting = lSave;
	}

pointList [0] = p1;
pointList [1] = p2;
gameData.render.terrain.uvlList [1][0].l = LIGHTVAL (i, j + 1);
gameData.render.terrain.uvlList [1][1].l = LIGHTVAL (i + 1, j + 1);
gameData.render.terrain.uvlList [1][2].l = LIGHTVAL (i + 1, j);

gameData.render.terrain.uvlList [1][0].u = (i) * (I2X (1) / 4);
gameData.render.terrain.uvlList [1][1].u =
gameData.render.terrain.uvlList [1][2].u = (i + 1) * (I2X (1) / 4);
gameData.render.terrain.uvlList [1][0].v =
gameData.render.terrain.uvlList [1][1].v = (j + 1) * (I2X (1) / 4);
gameData.render.terrain.uvlList [1][2].v = (j) * (I2X (1) / 4);

#if DBG
G3DrawTexPoly (3, pointList, gameData.render.terrain.uvlList [1], gameData.render.terrain.pBm, NULL, 1, 0, -1);
#else
G3CheckAndDrawTMap (3, pointList, gameData.render.terrain.uvlList [1], gameData.render.terrain.pBm, NULL, NULL);
#endif
if (gameData.render.terrain.bOutline) {
	int32_t lSave = gameStates.render.nLighting;
	gameStates.render.nLighting=0;
	CCanvas::Current ()->SetColorRGB (255, 128, 0, 255);
	G3DrawLine (pointList [0], pointList [1]);
	G3DrawLine (pointList [1], pointList [2]);
	G3DrawLine (pointList [2], pointList [0]);
	gameStates.render.nLighting = lSave;
	}

if ((i == gameData.render.terrain.orgI) && (j == gameData.render.terrain.orgJ))
	gameData.render.terrain.nMineTilesDrawn |= 1;
if ((i == gameData.render.terrain.orgI - 1) && (j == gameData.render.terrain.orgJ))
	gameData.render.terrain.nMineTilesDrawn |= 2;
if ((i == gameData.render.terrain.orgI) && (j == gameData.render.terrain.orgJ - 1))
	gameData.render.terrain.nMineTilesDrawn |= 4;
if ((i == gameData.render.terrain.orgI - 1) && (j == gameData.render.terrain.orgJ - 1))
	gameData.render.terrain.nMineTilesDrawn |= 8;

if (gameData.render.terrain.nMineTilesDrawn == 0xf) {
	RenderMine (gameData.endLevel.exit.nSegNum, 0, 0);
	gameData.render.terrain.nMineTilesDrawn = -1;
	}
}

//-----------------------------------------------------------------------------

CFixVector yCache [256];
uint8_t ycFlags [256];

extern CFixMatrix mSurfaceOrient;

CFixVector *get_dy_vec (int32_t h)
{
	CFixVector *dyp;

dyp = yCache + h;
if (!ycFlags [h]) {
	CFixVector tv = mSurfaceOrient.m.dir.u * (h * TERRAIN_HEIGHT_SCALE);
	transformation.RotateScaled (*dyp, tv);
	ycFlags [h] = 1;
	}
return dyp;
}

//-----------------------------------------------------------------------------

int32_t im=1;

void RenderTerrain (CFixVector *vOrgPoint, int32_t org_2dx, int32_t org_2dy)
{
	CFixVector	tv, delta_i, delta_j;		//delta_y;
	CRenderPoint		p, p2, pLast, p2Last, pLowSave, pHighSave;
	int32_t			i, j, iLow, iHigh, jLow, jHigh, iViewer, jViewer;

#if 1
memset (&p, 0, sizeof (CRenderPoint));
memset (&p2, 0, sizeof (CRenderPoint));
memset (&pLast, 0, sizeof (CRenderPoint));
memset (&p2Last, 0, sizeof (CRenderPoint));
#endif
gameData.render.terrain.nMineTilesDrawn = 0;	//clear flags
gameData.render.terrain.orgI = org_2dy;
gameData.render.terrain.orgJ = org_2dx;
iLow = 0;
iHigh = gameData.render.terrain.nGridW - 1;
jLow = 0;
jHigh = gameData.render.terrain.nGridH - 1;
memset (ycFlags, 0, sizeof (ycFlags));
gameStates.render.nInterpolationMethod = im;
tv = mSurfaceOrient.m.dir.r * TERRAIN_GRID_SCALE;
transformation.RotateScaled (delta_i, tv);
tv = mSurfaceOrient.m.dir.f * TERRAIN_GRID_SCALE;
transformation.RotateScaled (delta_j, tv);
gameData.render.terrain.vStartPoint = *vOrgPoint - mSurfaceOrient.m.dir.r * ((gameData.render.terrain.orgI - iLow) * TERRAIN_GRID_SCALE);
gameData.render.terrain.vStartPoint -= mSurfaceOrient.m.dir.f * ((gameData.render.terrain.orgJ - jLow) * TERRAIN_GRID_SCALE);
tv = gameData.objData.pViewer->info.position.vPos - gameData.render.terrain.vStartPoint;
iViewer = CFixVector::Dot (tv, mSurfaceOrient.m.dir.r) / TERRAIN_GRID_SCALE;
if (iViewer > iHigh)
	iViewer = iHigh;
jViewer = CFixVector::Dot (tv, mSurfaceOrient.m.dir.f) / TERRAIN_GRID_SCALE;
if (jViewer > jHigh)
	jViewer = jHigh;
pLast.TransformAndEncode (gameData.render.terrain.vStartPoint);
pLowSave = pLast;
for (j = jLow; j <= jHigh; j++) {
	gameData.render.terrain.saveRow [j].Add (pLast, *get_dy_vec (HEIGHT (iLow, j)));
	if (j == jHigh)
		pHighSave = pLast;
	else
		pLast.Add (pLast, delta_j);
	}
for (i = iLow; i < iViewer; i++) {
	pLowSave.Add (pLowSave, delta_i);
	pLast = pLowSave;
	p2Last.Add (pLast, *get_dy_vec (HEIGHT (i + 1, jLow)));
	for (j = jLow; j < jViewer; j++) {
		p.Add (pLast, delta_j);
		p2.Add (p, *get_dy_vec (HEIGHT (i + 1, j + 1)));
		DrawTerrainCell (i, j, gameData.render.terrain.saveRow + j, gameData.render.terrain.saveRow + j + 1, &p2, &p2Last);
		pLast = p;
		gameData.render.terrain.saveRow [j] = p2Last;
		p2Last = p2;
		}
	delta_j = -delta_j;			//don't have a delta sub...
	pHighSave.Add (pHighSave, delta_i);
	pLast = pHighSave;
	p2Last.Add (pLast, *get_dy_vec (HEIGHT (i + 1, jHigh)));
	for (j = jHigh - 1; j >= jViewer; j--) {
		p.Add (pLast, delta_j);
		p2.Add (p, *get_dy_vec (HEIGHT (i + 1, j)));
		DrawTerrainCell (i, j, gameData.render.terrain.saveRow + j, gameData.render.terrain.saveRow + j + 1, &p2Last, &p2);
		pLast = p;
		gameData.render.terrain.saveRow [j + 1] = p2Last;
		p2Last = p2;
		}
	gameData.render.terrain.saveRow [j + 1] = p2Last;
	delta_j = -delta_j;		//restore sign of j
	}
//now do i from other end
delta_i = -delta_i;		//going the other way now...
gameData.render.terrain.vStartPoint += mSurfaceOrient.m.dir.r * ((iHigh - iLow) * TERRAIN_GRID_SCALE);
pLast.TransformAndEncode (gameData.render.terrain.vStartPoint);
pLowSave = pLast;
for (j = jLow; j <= jHigh; j++) {
	gameData.render.terrain.saveRow [j].Add (pLast, *get_dy_vec (HEIGHT (iHigh, j)));
	if (j == jHigh)
		pHighSave = pLast;
	else
		pLast.Add (pLast, delta_j);
	}
for (i = iHigh - 1; i >= iViewer; i--) {
	pLowSave.Add (pLowSave, delta_i);
	pLast = pLowSave;
	p2Last.Add (pLast, *get_dy_vec (HEIGHT (i, jLow)));
	for (j = jLow; j < jViewer; j++) {
		p.Add (pLast, delta_j);
		p2.Add (p, *get_dy_vec (HEIGHT (i, j + 1)));
		DrawTerrainCell (i, j, &p2Last, &p2,
							  gameData.render.terrain.saveRow + j + 1,
							  gameData.render.terrain.saveRow + j);
		pLast = p;
		gameData.render.terrain.saveRow [j] = p2Last;
		p2Last = p2;
		}
	delta_j = -delta_j;			//don't have a delta sub...
	pHighSave.Add (pHighSave, delta_i);
	pLast = pHighSave;
	p2Last.Add (pLast, *get_dy_vec (HEIGHT (i, jHigh)));
	for (j = jHigh - 1; j >= jViewer; j--) {
		p.Add (pLast, delta_j);
		p2.Add (p, *get_dy_vec (HEIGHT (i, j)));
		DrawTerrainCell (i, j, &p2, &p2Last,
							  gameData.render.terrain.saveRow + j + 1,
							  gameData.render.terrain.saveRow + j);
		pLast = p;
		gameData.render.terrain.saveRow [j + 1] = p2Last;
		p2Last = p2;
		}
	gameData.render.terrain.saveRow [j + 1] = p2Last;
	delta_j = -delta_j;		//restore sign of j
	}
}

//-----------------------------------------------------------------------------

void _CDECL_ FreeTerrainHeightmap (void)
{
if (gameData.render.terrain.heightmap.Buffer ()) {
	PrintLog (0, "unloading terrain height map\n");
	gameData.render.terrain.heightmap.Destroy ();
	}
}

//-----------------------------------------------------------------------------

void LoadTerrain (char *filename)
{
	CBitmap	bmHeight;
	int32_t		iffError;
	int32_t		i, j;
	uint8_t		h, hMin, hMax;
	CIFF		iff;

PrintLog (1, "loading terrain height map\n");
memset (&bmHeight, 0, sizeof (bmHeight));
iffError = iff.ReadBitmap (filename, &bmHeight, BM_LINEAR);
if (iffError != IFF_NO_ERROR) {
#if TRACE
	console.printf (1, "File %s - IFF error: %s", filename, iff.ErrorMsg (iffError));
#endif
 	PrintLog (-1);
	Error ("File %s - IFF error: %s", filename, iff.ErrorMsg (iffError));
}
gameData.render.terrain.heightmap.Destroy ();
gameData.render.terrain.nGridW = bmHeight.Width ();
gameData.render.terrain.nGridH = bmHeight.Height ();
Assert (gameData.render.terrain.nGridW <= TERRAIN_GRID_MAX_SIZE);
Assert (gameData.render.terrain.nGridH <= TERRAIN_GRID_MAX_SIZE);
PrintLog (0, "heightmap loaded, size=%dx%d\n", gameData.render.terrain.nGridW, gameData.render.terrain.nGridH);
gameData.render.terrain.heightmap = bmHeight;
hMax = 0;
hMin = 255;
for (i = 0; i < gameData.render.terrain.nGridW; i++)
	for (j = 0; j < gameData.render.terrain.nGridH; j++) {
		h = HEIGHT (i, j);
		if (h > hMax)
			hMax = h;
		if (h < hMin)
			hMin = h;
		}
for (i = 0; i < gameData.render.terrain.nGridW; i++) {
	for (j = 0; j < gameData.render.terrain.nGridH; j++) {
		HEIGHT (i, j) -= hMin;
		}
	}
gameData.render.terrain.pBm = gameData.endLevel.terrain.pBm;
#if 0 //the following code turns the (palettized) terrain texture into a white TGA texture for testing
gameData.render.terrain.pBm->props.rowSize *= 4;
gameData.render.terrain.pBm->props.flags |= BM_FLAG_TGA;
gameData.render.terrain.pBm->DestroyBuffer ();
gameData.render.terrain.pBm->CreateBuffer (gameData.render.terrain.pBm->Height () * gameData.render.terrain.pBm->props.rowSize);
gameData.render.terrain.pBm->Clear (0xFF);
#endif
PrintLog (1, "building terrain light map\n");
BuildTerrainLightmap ();
PrintLog (-1);
PrintLog (-1);
}


//-----------------------------------------------------------------------------

static void GetTerrainPoint (CFixVector *p, int32_t i, int32_t j)
{
if (!gameData.render.terrain.heightmap) {
	PrintLog (0, "no heightmap available\n");
	return;
	}
if (i < 0)
	i = 0;
else if (i >= gameData.render.terrain.nGridW)
	i = gameData.render.terrain.nGridW - 1;
if (j < 0)
	j = 0;
else if (j >= gameData.render.terrain.nGridH)
	j = gameData.render.terrain.nGridH - 1;
*p = gameData.render.terrain.points [GRID_OFFS (i, j)];
}

//-----------------------------------------------------------------------------

static fix GetTerrainFaceLight (CFixVector *p0, CFixVector *p1, CFixVector *p2)
{
	static CFixVector vLightDir = CFixVector::Create(0x2e14, 0xe8f5, 0x5eb8);
	CFixVector vNormal = CFixVector::Normal (*p0, *p1, *p2);

return -CFixVector::Dot (vNormal, vLightDir);
}

//-----------------------------------------------------------------------------

fix GetAvgTerrainLight (int32_t i, int32_t j)
{
	CFixVector	pp, p [6];
	fix			light, totalLight;
	int32_t			n;

pp.SetZero();
GetTerrainPoint (&pp, i, j);
GetTerrainPoint (p, i ? i - 1 : 5, j);
GetTerrainPoint (p + 1, i, j ? j - 1 : 5);
GetTerrainPoint (p + 2, i + 1, j ? j - 1 : 5);
GetTerrainPoint (p + 3, i + 1, j);
GetTerrainPoint (p + 4, i, j + 1);
GetTerrainPoint (p + 5, i ? i - 1 : 5, j + 1);
for (n = 0, totalLight = 0; n < 6; n++) {
	light = abs (GetTerrainFaceLight (&pp, p + n, p + (n + 1) % 6));
	if (light > 0)
		totalLight += light;
	}
return totalLight / 6;
}

//-----------------------------------------------------------------------------

void _CDECL_ FreeTerrainLightmap ()
{
if (gameData.render.terrain.lightmap.Buffer ()) {
	PrintLog (0, "unloading terrain light map\n");
	gameData.render.terrain.lightmap.Destroy ();
	}
}

//-----------------------------------------------------------------------------

void ComputeTerrainPoints (void)
{
	int32_t			i, j;
	CFixVector	*p = gameData.render.terrain.points.Buffer ();

for (i = 0; i < gameData.render.terrain.nGridW; i++) {
	for (j = 0; j < gameData.render.terrain.nGridH; j++) {
		p = gameData.render.terrain.points + GRID_OFFS (i, j);
		(*p).v.coord.x = TERRAIN_GRID_SCALE * i;
		(*p).v.coord.z = TERRAIN_GRID_SCALE * j;
		(*p).v.coord.y = (fix) HEIGHT (i, j) * TERRAIN_HEIGHT_SCALE;
		}
	}
}

//-----------------------------------------------------------------------------

void BuildTerrainLightmap ()
{
	int32_t i, j;
	fix l, l2, lMin = 0x7fffffff, lMax = 0;


if (gameData.render.terrain.lightmap.Buffer ())
	gameData.render.terrain.lightmap.Destroy ();
else
	atexit (FreeTerrainLightmap);		//first time

gameData.render.terrain.points.Create (GRID_SIZE);
gameData.render.terrain.lightmap.Create (GRID_SIZE);
ComputeTerrainPoints ();
for (i = 0; i < gameData.render.terrain.nGridW; i++) {
	for (j = 0; j < gameData.render.terrain.nGridH; j++) {
		l = GetAvgTerrainLight (i, j);
		if (l > lMax)
			lMax = l;
		if (l < lMin)
			lMin = l;
		if (lMin < 0)
			l = GetAvgTerrainLight (i, j);
		}
	}
for (i = 0; i < gameData.render.terrain.nGridW; i++) {
	for (j = 0; j < gameData.render.terrain.nGridH; j++) {
		l = GetAvgTerrainLight (i, j);
		if (lMin == lMax)
			LIGHT (i, j) = l;// >> 8;
		else {
			l2 = FixDiv ((l - lMin), (lMax - lMin));
			if (l2 == I2X (1))
				l2--;
			LIGHT (i, j) = l2;// >> 8;
			}
		}
	}
gameData.render.terrain.points.Destroy ();
}

//-----------------------------------------------------------------------------
//eof
