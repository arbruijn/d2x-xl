#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "inferno.h"
#include "error.h"
#include "u_mem.h"
#include "text.h"
#include "cfile.h"
#include "bm.h"
#include "textures.h"
#include "ogl_defs.h"
#include "maths.h"
#include "vecmat.h"
#include "hudmsg.h"
#include "gameseg.h"
#include "piggy.h"

#define SPARK_MIN_PROB		16
#define SPARK_FRAME_TIME	25

//-----------------------------------------------------------------------------

void AllocSegmentSparks (short nSegment)
{
	int				nMatCen = nSegment;
	tSegment2		*seg2P = gameData.segs.segment2s + (nSegment = gameData.matCens.sparkSegs [nSegment]);
	int				i, bFuel = (seg2P->special == SEGMENT_IS_FUELCEN);
	tSegmentSparks	*segP = gameData.matCens.sparks [bFuel] + nMatCen;
	tEnergySpark	*sparkP = segP->sparks;

segP->nMaxSparks = (ushort) (2 * AvgSegRadf (nSegment) + 0.5f);
if (!(sparkP = (tEnergySpark *) D2_ALLOC (segP->nMaxSparks * sizeof (tEnergySpark))))
	segP->nMaxSparks = 0;
else {
	segP->sparks = sparkP;
	segP->bUpdate = 0;
	memset (sparkP, 0, segP->nMaxSparks * sizeof (tEnergySpark));
	for (i = segP->nMaxSparks; i; i--, sparkP++)
		sparkP->nProb = d_rand () % SPARK_MIN_PROB + 1;
	}
}

//-----------------------------------------------------------------------------

void FreeSegmentSparks (short nSegment)
{
	int				nMatCen = nSegment;
	tSegment2		*seg2P = gameData.segs.segment2s + (nSegment = gameData.matCens.sparkSegs [nSegment]);
	int				bFuel = (seg2P->special == SEGMENT_IS_FUELCEN);
	tSegmentSparks	*segP = gameData.matCens.sparks [bFuel]+ nMatCen;

if (segP->sparks) {
	D2_FREE (segP->sparks);
	segP->nMaxSparks = 0;
	}
}

//-----------------------------------------------------------------------------

void CreateSegmentSparks (short nSegment)
{
	int				nMatCen = nSegment;
	tSegment2		*seg2P = gameData.segs.segment2s + (nSegment = gameData.matCens.sparkSegs [nSegment]);
	int				bFuel = (seg2P->special == SEGMENT_IS_FUELCEN);
	tSegmentSparks	*segP = gameData.matCens.sparks [bFuel]+ nMatCen;
	tEnergySpark	*sparkP = segP->sparks;
	vmsVector		vOffs;
	fVector			vMaxf, vMax2f;
	int				i;

VmVecFixToFloat (&vMaxf, &gameData.segs.extent [nSegment].vMax);
VmVecScale (&vMax2f, &vMaxf, 2);
for (i = segP->nMaxSparks; i; i--, sparkP++) {
	if (sparkP->tRender)
		continue;
	if (!sparkP->nProb)
		sparkP->nProb = SPARK_MIN_PROB;
	if (gameStates.app.nSDLTicks - sparkP->tCreate < SPARK_FRAME_TIME) 
		continue;
	sparkP->tCreate = gameStates.app.nSDLTicks; 
	if (d_rand () % sparkP->nProb)
		sparkP->nProb--;
	else {
		vOffs.p.x = fl2f (vMaxf.p.x - f_rand () * vMax2f.p.x);
		vOffs.p.y = fl2f (vMaxf.p.y - f_rand () * vMax2f.p.y);
		vOffs.p.z = fl2f (vMaxf.p.z - f_rand () * vMax2f.p.z);
		VmVecAdd (&sparkP->vPos, SEGMENT_CENTER_I (nSegment), &vOffs);
		if ((VmVecMag (&vOffs) > MinSegRad (nSegment)) && GetSegMasks (&sparkP->vPos, nSegment, 0).centerMask)
			sparkP->nProb = 1;
		else {
			sparkP->xSize = F1_0 + 4 * d_rand ();
			sparkP->nFrame = 0;
			sparkP->tRender = -1;
			sparkP->nProb = SPARK_MIN_PROB;
			}
		}
	}
}

//-----------------------------------------------------------------------------

void UpdateSegmentSparks (short nSegment)
{
	int				nMatCen = nSegment;
	tSegment2		*seg2P = gameData.segs.segment2s + (nSegment = gameData.matCens.sparkSegs [nSegment]);
	int				bFuel = (seg2P->special == SEGMENT_IS_FUELCEN);
	tSegmentSparks	*segP = gameData.matCens.sparks [bFuel] + nMatCen;

if (segP->bUpdate) {
		tEnergySpark	*sparkP = segP->sparks;
		int				i, nLastRender [2];

	nLastRender [0] = BM_FRAMECOUNT (BM_ADDON (BM_ADDON_REPAIRSPARK)) - 1;
	nLastRender [1] = BM_FRAMECOUNT (BM_ADDON (BM_ADDON_FUELSPARK)) - 1;
	for (i = segP->nMaxSparks; i; i--, sparkP++) {
		if (!sparkP->tRender)
			continue;
		if (gameStates.app.nSDLTicks - sparkP->tRender < SPARK_FRAME_TIME)
			continue;
		if (sparkP->nFrame >= nLastRender [bFuel]) {
			sparkP->tRender = 0;
			sparkP->tCreate = -1;
			}
		else {
			sparkP->tRender = gameStates.app.nSDLTicks; //+= SPARK_FRAME_TIME;
			sparkP->nFrame++;
			}
		}
	CreateSegmentSparks (nMatCen);
	segP->bUpdate = 0;
	}
}

//-----------------------------------------------------------------------------

void RenderSegmentSparks (short nSegment)
{
	int			nMatCen = nSegment;
	tSegment2	*seg2P = gameData.segs.segment2s + (nSegment = gameData.matCens.sparkSegs [nSegment]);

if (gameData.render.mine.bVisible [nSegment] == gameData.render.mine.nVisible) {
	int				bFuel = (seg2P->special == SEGMENT_IS_FUELCEN);
	tSegmentSparks	*segP = gameData.matCens.sparks [bFuel] + nMatCen;
	tEnergySpark	*sparkP = segP->sparks;
	int				i;
	grsBitmap		*bmP = BM_ADDON (BM_ADDON_REPAIRSPARK + bFuel), *bmfP;

	segP->bUpdate = 1;
	for (i = segP->nMaxSparks; i; i--, sparkP++) {
		if (sparkP->tRender) {
			if (sparkP->nFrame >= BM_FRAMECOUNT (bmP))
				sparkP->tRender = 0;
			else {
				bmfP = BM_FRAMES (bmP) + sparkP->nFrame;
				BM_PARENT (bmfP) = NULL;
				G3DrawSprite (&sparkP->vPos, sparkP->xSize, sparkP->xSize, bmfP, NULL, 1.0, 1);
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------

void DoEnergySparkFrame (void)
{
	short	nSegment;

if (gameOpts->render.bEnergySparks)
	for (nSegment = 0; nSegment < gameData.matCens.nSparkSegs; nSegment++)
		UpdateSegmentSparks (nSegment);
}


//-----------------------------------------------------------------------------

void RenderEnergySparks (void)
{
	short	nSegment;

if (gameOpts->render.bEnergySparks)
	for (nSegment = 0; nSegment < gameData.matCens.nSparkSegs; nSegment++)
		RenderSegmentSparks (nSegment);
}

//-----------------------------------------------------------------------------

int BuildSparkSegList (void)
{
	tSegment2	*seg2P = gameData.segs.segment2s;
	short			nSegment;

gameData.matCens.nSparkSegs = 0;
for (nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++, seg2P++)
	if ((seg2P->special == SEGMENT_IS_FUELCEN) || (seg2P->special == SEGMENT_IS_REPAIRCEN))
		gameData.matCens.sparkSegs [gameData.matCens.nSparkSegs++] = nSegment;
return gameData.matCens.nSparkSegs;
}


//-----------------------------------------------------------------------------

void AllocEnergySparks (void)
{
	short	nSegment;

if (gameOpts->render.bEnergySparks && BuildSparkSegList ()) {
	if (BM_ADDON (BM_ADDON_FUELSPARK) && BM_ADDON (BM_ADDON_REPAIRSPARK)) {
		PageInAddonBitmap (-(BM_ADDON_FUELSPARK) - 1);
		PageInAddonBitmap (-(BM_ADDON_REPAIRSPARK) - 1);
		for (nSegment = 0; nSegment < gameData.matCens.nSparkSegs; nSegment++)
			AllocSegmentSparks (nSegment);
		OglLoadBmTexture (BM_ADDON (BM_ADDON_FUELSPARK), 1, 0, gameOpts->render.bDepthSort <= 0);
		OglLoadBmTexture (BM_ADDON (BM_ADDON_REPAIRSPARK), 1, 0, gameOpts->render.bDepthSort <= 0);
		d_srand ((uint) gameStates.app.nSDLTicks);
		}
	else
		gameOpts->render.bEnergySparks = 0;
	}
}

//-----------------------------------------------------------------------------

void FreeEnergySparks (void)
{
	short	nSegment;

if (gameOpts->render.bEnergySparks)
	for (nSegment = 0; nSegment < gameData.matCens.nSparkSegs; nSegment++)
		FreeSegmentSparks (nSegment);
}

//-----------------------------------------------------------------------------
