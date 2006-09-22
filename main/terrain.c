/* $Id: terrain.c, v 1.4 2003/10/10 09:36:35 btb Exp $ */
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

/*
 *
 * Code to render cool external-scene terrain
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:31:29  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:31:27  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.12  1994/12/03  00:18:00  matt
 * Made endlevel sequence cut off early
 * Made exit model and bit explosion always plot last (after all terrain)
 *
 * Revision 1.11  1994/11/27  23:13:46  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.10  1994/11/21  18:04:36  matt
 * Fixed alloc/d_free problem with height array
 *
 * Revision 1.9  1994/11/21  17:30:42  matt
 * Properly d_free light array
 *
 * Revision 1.8  1994/11/19  12:40:55  matt
 * Added system to read endlevel data from file, and to make it work
 * with any exit tunnel.
 *
 * Revision 1.7  1994/11/16  11:49:44  matt
 * Added code to rotate terrain to match mine
 *
 * Revision 1.6  1994/11/02  16:22:59  matt
 * Killed con_printf
 *
 * Revision 1.5  1994/10/30  20:09:19  matt
 * For endlevel: added big explosion at tunnel exit; made lights in tunnel
 * go out; made more explosions on walls.
 *
 * Revision 1.4  1994/10/27  21:15:07  matt
 * Added better error handling
 *
 * Revision 1.3  1994/10/27  01:03:17  matt
 * Made terrain renderer use aribtary point in height array as origin
 *
 * Revision 1.2  1994/08/19  20:09:44  matt
 * Added end-of-level cut scene with external scene
 *
 * Revision 1.1  1994/08/17  20:20:49  matt
 * Initial revision
 *
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "3d.h"
#include "error.h"
#include "gr.h"
#include "texmap.h"
#include "iff.h"
#include "u_mem.h"
#include "mono.h"

#include "inferno.h"
#include "textures.h"
#include "object.h"
#include "endlevel.h"
#include "fireball.h"
#include "render.h"
#include "vecmat.h"

#define TERRAIN_GRID_MAX_SIZE	64
#define TERRAIN_GRID_SCALE    i2f (2*20)
#define TERRAIN_HEIGHT_SCALE  f1_0
#define f0_4						0x4000

#define GRID_OFFS(_i,_j)	((_i) * gameData.render.terrain.nGridW + (_j))
#define HEIGHT(_i,_j)		(gameData.render.terrain.pHeightMap [GRID_OFFS (_i,_j)])
#define LIGHT(_i,_j)			(gameData.render.terrain.pLightMap [GRID_OFFS (_i,_j)])

//!!#define HEIGHT (_i, _j)   gameData.render.terrain.pHeightMap [(gameData.render.terrain.nGridH-1-j)*gameData.render.terrain.nGridW+ (_i)]
//!!#define LIGHT (_i, _j)    gameData.render.terrain.pLightMap [(gameData.render.terrain.nGridH-1-j)*gameData.render.terrain.nGridW+ (_i)]

#define LIGHTVAL(_i,_j) (((fix) LIGHT (_i,_j))) // << 8)


// LINT: adding function prototypes
void BuildTerrainLightMap (void);
void _CDECL_ FreeTerrainLightMap (void);

// ------------------------------------------------------------------------

void DrawTerrainCell (int i, int j, g3s_point *p0, g3s_point *p1, g3s_point *p2, g3s_point *p3)
{
	g3s_point *pointList [3];

p1->p3_index =
p2->p3_index =
p3->p3_index = -1;
pointList [0] = p0;
pointList [1] = p1;
pointList [2] = p3;
gameData.render.terrain.uvlList [0][0].l = LIGHTVAL (i, j);
gameData.render.terrain.uvlList [0][1].l = LIGHTVAL (i, j+1);
gameData.render.terrain.uvlList [0][2].l = LIGHTVAL (i+1, j);

gameData.render.terrain.uvlList [0][0].u = 
gameData.render.terrain.uvlList [0][1].u = (i) * f0_4; 
gameData.render.terrain.uvlList [0][0].v = 
gameData.render.terrain.uvlList [0][2].v = (j) * f0_4;
gameData.render.terrain.uvlList [0][1].v = (j+1) * f0_4;
gameData.render.terrain.uvlList [0][2].u = (i+1) * f0_4;   

G3CheckAndDrawTMap (3, pointList, gameData.render.terrain.uvlList [0], 
						  gameData.render.terrain.bmP, NULL, NULL);
if (gameData.render.terrain.bOutline) {
	int lSave = gameStates.render.nLighting;
	gameStates.render.nLighting = 0;
	GrSetColorRGB (255, 0, 0, 255);
	G3DrawLine (pointList [0], pointList [1]);
	G3DrawLine (pointList [2], pointList [0]);
	gameStates.render.nLighting = lSave;
	}

pointList [0] = p1;
pointList [1] = p2;
gameData.render.terrain.uvlList [1][0].l = LIGHTVAL (i, j+1);
gameData.render.terrain.uvlList [1][1].l = LIGHTVAL (i+1, j+1);
gameData.render.terrain.uvlList [1][2].l = LIGHTVAL (i+1, j);

gameData.render.terrain.uvlList [1][0].u = (i) * f0_4; 
gameData.render.terrain.uvlList [1][1].u =
gameData.render.terrain.uvlList [1][2].u = (i+1) * f0_4;   
gameData.render.terrain.uvlList [1][0].v = 
gameData.render.terrain.uvlList [1][1].v = (j+1) * f0_4;
gameData.render.terrain.uvlList [1][2].v = (j) * f0_4;

G3CheckAndDrawTMap (3, pointList, gameData.render.terrain.uvlList [1], 
						  gameData.render.terrain.bmP, NULL, NULL);
if (gameData.render.terrain.bOutline) {
	int lSave = gameStates.render.nLighting;
	gameStates.render.nLighting=0;
	GrSetColorRGB (255, 128, 0, 255);
	G3DrawLine (pointList [0], pointList [1]);
	G3DrawLine (pointList [1], pointList [2]);
	G3DrawLine (pointList [2], pointList [0]);
	gameStates.render.nLighting = lSave;
	}

if ((i == gameData.render.terrain.orgI) && (j == gameData.render.terrain.orgJ))
	gameData.render.terrain.nMineTilesDrawn |= 1;
if ((i == gameData.render.terrain.orgI-1) && (j == gameData.render.terrain.orgJ))
	gameData.render.terrain.nMineTilesDrawn |= 2;
if ((i == gameData.render.terrain.orgI) && (j == gameData.render.terrain.orgJ-1))
	gameData.render.terrain.nMineTilesDrawn |= 4;
if ((i == gameData.render.terrain.orgI-1) && (j == gameData.render.terrain.orgJ-1))
	gameData.render.terrain.nMineTilesDrawn |= 8;

if (gameData.render.terrain.nMineTilesDrawn == 0xf) {
	RenderMine (gameData.endLevel.exit.nSegNum, 0, 0);
	gameData.render.terrain.nMineTilesDrawn=-1;
	}
}

//-----------------------------------------------------------------------------

vms_vector yCache [256];
ubyte ycFlags [256];

extern vms_matrix surface_orient;

vms_vector *get_dy_vec (int h)
{
	vms_vector *dyp;

dyp = yCache + h;
if (!ycFlags [h]) {
	vms_vector tv;
	VmVecCopyScale (&tv, &surface_orient.uvec, h * TERRAIN_HEIGHT_SCALE);
	G3RotateDeltaVec (dyp, &tv);
	ycFlags [h] = 1;
	}
return dyp;
}

//-----------------------------------------------------------------------------

int im=1;

void RenderTerrain (vms_vector *vOrgPoint, int org_2dx, int org_2dy)
{
	vms_vector	tv, delta_i, delta_j;		//delta_y;
	g3s_point	p, p2, pLast, p2Last, pLowSave, pHighSave;
	int			i, j, iLow, iHigh, jLow, jHigh, iViewer, jViewer;

#if 1
memset (&p, 0, sizeof (g3s_point));
memset (&p2, 0, sizeof (g3s_point));
memset (&pLast, 0, sizeof (g3s_point));
memset (&p2Last, 0, sizeof (g3s_point));
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
VmVecCopyScale (&tv, &surface_orient.rvec, TERRAIN_GRID_SCALE);
G3RotateDeltaVec (&delta_i, &tv);
VmVecCopyScale (&tv, &surface_orient.fvec, TERRAIN_GRID_SCALE);
G3RotateDeltaVec (&delta_j, &tv);
VmVecScaleAdd (&gameData.render.terrain.vStartPoint, vOrgPoint, &surface_orient.rvec,
						-(gameData.render.terrain.orgI - iLow) * TERRAIN_GRID_SCALE);
VmVecScaleInc (&gameData.render.terrain.vStartPoint, &surface_orient.fvec, 
						-(gameData.render.terrain.orgJ - jLow) * TERRAIN_GRID_SCALE);
VmVecSub (&tv, &gameData.objs.viewer->pos, &gameData.render.terrain.vStartPoint);
iViewer = VmVecDot (&tv, &surface_orient.rvec) / TERRAIN_GRID_SCALE;
jViewer = VmVecDot (&tv, &surface_orient.fvec) / TERRAIN_GRID_SCALE;
G3TransformAndEncodePoint (&pLast, &gameData.render.terrain.vStartPoint);
pLowSave = pLast;
for (j = jLow; j <= jHigh; j++) {
	G3AddDeltaVec (gameData.render.terrain.saveRow + j, &pLast, get_dy_vec (HEIGHT (iLow, j)));
	if (j == jHigh)
		pHighSave = pLast;
	else
		G3AddDeltaVec (&pLast, &pLast, &delta_j);
	}
for (i = iLow; i < iViewer; i++) {
	G3AddDeltaVec (&pLowSave, &pLowSave, &delta_i);
	pLast = pLowSave;
	G3AddDeltaVec (&p2Last, &pLast, get_dy_vec (HEIGHT (i+1, jLow)));
	for (j = jLow; j < jViewer; j++) {
		G3AddDeltaVec (&p, &pLast, &delta_j);
		G3AddDeltaVec (&p2, &p, get_dy_vec (HEIGHT (i+1, j+1)));
		DrawTerrainCell (i, j, gameData.render.terrain.saveRow + j, 
							  gameData.render.terrain.saveRow + j + 1, &p2, &p2Last);
		pLast = p;
		gameData.render.terrain.saveRow [j] = p2Last;
		p2Last = p2;
		}
	VmVecNegate (&delta_j);			//don't have a delta sub...
	G3AddDeltaVec (&pHighSave, &pHighSave, &delta_i);
	pLast = pHighSave;
	G3AddDeltaVec (&p2Last, &pLast, get_dy_vec (HEIGHT (i+1, jHigh)));
	for (j = jHigh - 1; j >= jViewer; j--) {
		G3AddDeltaVec (&p, &pLast, &delta_j);
		G3AddDeltaVec (&p2, &p, get_dy_vec (HEIGHT (i+1, j)));
		DrawTerrainCell (i, j, gameData.render.terrain.saveRow + j, 
							  gameData.render.terrain.saveRow + j + 1, &p2Last, &p2);
		pLast = p;
		gameData.render.terrain.saveRow [j+1] = p2Last;
		p2Last = p2;
		}
	gameData.render.terrain.saveRow [j+1] = p2Last;
	VmVecNegate (&delta_j);		//restore sign of j
	}
//now do i from other end
VmVecNegate (&delta_i);		//going the other way now...
VmVecScaleInc (&gameData.render.terrain.vStartPoint, &surface_orient.rvec, 
						(iHigh-iLow)*TERRAIN_GRID_SCALE);
G3TransformAndEncodePoint (&pLast, &gameData.render.terrain.vStartPoint);
pLowSave = pLast;
for (j = jLow; j <= jHigh; j++) {
	G3AddDeltaVec (gameData.render.terrain.saveRow + j, &pLast, get_dy_vec (HEIGHT (iHigh, j)));
	if (j == jHigh)
		pHighSave = pLast;
	else
		G3AddDeltaVec (&pLast, &pLast, &delta_j);
	}
for (i = iHigh - 1; i >= iViewer; i--) {
	G3AddDeltaVec (&pLowSave, &pLowSave, &delta_i);
	pLast = pLowSave;
	G3AddDeltaVec (&p2Last, &pLast, get_dy_vec (HEIGHT (i, jLow)));
	for (j = jLow; j < jViewer; j++) {
		G3AddDeltaVec (&p, &pLast, &delta_j);
		G3AddDeltaVec (&p2, &p, get_dy_vec (HEIGHT (i, j+1)));
		DrawTerrainCell (i, j, &p2Last, &p2, 
							  gameData.render.terrain.saveRow + j + 1, 
							  gameData.render.terrain.saveRow + j);
		pLast = p;
		gameData.render.terrain.saveRow [j] = p2Last;
		p2Last = p2;
		}
	VmVecNegate (&delta_j);			//don't have a delta sub...
	G3AddDeltaVec (&pHighSave, &pHighSave, &delta_i);
	pLast = pHighSave;
	G3AddDeltaVec (&p2Last, &pLast, get_dy_vec (HEIGHT (i, jHigh)));
	for (j = jHigh - 1; j >= jViewer; j--) {
		G3AddDeltaVec (&p, &pLast, &delta_j);
		G3AddDeltaVec (&p2, &p, get_dy_vec (HEIGHT (i, j)));
		DrawTerrainCell (i, j, &p2, &p2Last, 
							  gameData.render.terrain.saveRow + j + 1, 
							  gameData.render.terrain.saveRow + j);
		pLast = p;
		gameData.render.terrain.saveRow [j+1] = p2Last;
		p2Last = p2;
		}
	gameData.render.terrain.saveRow [j+1] = p2Last;
	VmVecNegate (&delta_j);		//restore sign of j
	}
}

//-----------------------------------------------------------------------------

void _CDECL_ FreeTerrainHeightMap (void)
{
if (gameData.render.terrain.pHeightMap) {
	LogErr ("unloading terrain height map\n");
	d_free (gameData.render.terrain.pHeightMap);
	}
}

//-----------------------------------------------------------------------------

void LoadTerrain (char *filename)
{
	grs_bitmap	bmHeight;
	int			iff_error;
	int			i, j;
	ubyte			h, hMin, hMax;

LogErr ("            loading terrain height map\n");
iff_error = iff_read_bitmap (filename, &bmHeight, BM_LINEAR);
if (iff_error != IFF_NO_ERROR) {
#if TRACE		
	con_printf (1, "File %s - IFF error: %s", filename, iff_errormsg (iff_error));
#endif		
	Error ("File %s - IFF error: %s", filename, iff_errormsg (iff_error));
}
if (gameData.render.terrain.pHeightMap)
	d_free (gameData.render.terrain.pHeightMap)
else
	atexit (FreeTerrainHeightMap);		//first time
gameData.render.terrain.nGridW = bmHeight.bm_props.w;
gameData.render.terrain.nGridH = bmHeight.bm_props.h;
Assert (gameData.render.terrain.nGridW <= TERRAIN_GRID_MAX_SIZE);
Assert (gameData.render.terrain.nGridH <= TERRAIN_GRID_MAX_SIZE);
gameData.render.terrain.pHeightMap = bmHeight.bm_texBuf;
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
//	d_free (bmHeight.bm_texBuf);
gameData.render.terrain.bmP = gameData.endLevel.terrain.bmP;
#if 0 //the following code turns the (palettized) terrain texture into a white TGA texture for testing
gameData.render.terrain.bmP->bm_props.rowsize *= 4;
gameData.render.terrain.bmP->bm_props.flags |= BM_FLAG_TGA;
d_free (gameData.render.terrain.bmP->bm_texBuf);
gameData.render.terrain.bmP->bm_texBuf = d_malloc (gameData.render.terrain.bmP->bm_props.h * gameData.render.terrain.bmP->bm_props.rowsize);
memset (gameData.render.terrain.bmP->bm_texBuf, 0xFF, gameData.render.terrain.bmP->bm_props.h * gameData.render.terrain.bmP->bm_props.rowsize);
#endif
LogErr ("            building terrain light map\n");
BuildTerrainLightMap ();
}


//-----------------------------------------------------------------------------

static void GetTerrainPoint (vms_vector *p, int i, int j)
{
if (i < 0)
	i = 0;
else if (i >= gameData.render.terrain.nGridW)
	i = gameData.render.terrain.nGridW;
if (j < 0)
	j = 0;
else if (j >= gameData.render.terrain.nGridH)
	j = gameData.render.terrain.nGridH;
p->x = TERRAIN_GRID_SCALE * i;
p->z = TERRAIN_GRID_SCALE * j;
p->y = (fix) HEIGHT (i, j) * TERRAIN_HEIGHT_SCALE;
}

//-----------------------------------------------------------------------------

vms_vector light = {0x2e14, 0xe8f5, 0x5eb8};

static fix GetTerrainFaceLight (vms_vector *p0, vms_vector *p1, vms_vector *p2)
{
	vms_vector norm;

VmVecNormal (&norm, p0, p1, p2);
return -VmVecDot (&norm, &light);
}

//-----------------------------------------------------------------------------

fix GetAvgTerrainLight (int i, int j)
{
	vms_vector pp, p [6];
	fix sum, light;
	int f;

GetTerrainPoint (&pp, i, j);
GetTerrainPoint (p, i-1, j);
GetTerrainPoint (p + 1, i, j-1);
GetTerrainPoint (p + 2, i+1, j-1);
GetTerrainPoint (p + 3, i+1, j);
GetTerrainPoint (p + 4, i, j+1);
GetTerrainPoint (p + 5, i-1, j+1);
for (f = 0, sum = 0; f < 6; f++) {
	light = GetTerrainFaceLight (&pp, p + f, p + (f + 1) % 5);
	if (light < 0)
		sum -= light;
	else
		sum += light;
	}
return sum / 6;
}

//-----------------------------------------------------------------------------

void _CDECL_ FreeTerrainLightMap ()
{
if (gameData.render.terrain.pLightMap) {
	LogErr ("unloading terrain light map\n");
	d_free (gameData.render.terrain.pLightMap);
	}
}

//-----------------------------------------------------------------------------

void BuildTerrainLightMap ()
{
	int i, j;
	fix l, l2, lMin = 0x7fffffff, lMax = 0;


if (gameData.render.terrain.pLightMap)
	d_free (gameData.render.terrain.pLightMap)
else
	atexit (FreeTerrainLightMap);		//first time

MALLOC (gameData.render.terrain.pLightMap, fix, 
		  gameData.render.terrain.nGridW * gameData.render.terrain.nGridH);
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
			l2 = fixdiv ((l - lMin), (lMax - lMin));
			if (l2 == f1_0)
				l2--;
			LIGHT (i, j) = l2;// >> 8;
			}
		}
	}
}

//-----------------------------------------------------------------------------
//eof
