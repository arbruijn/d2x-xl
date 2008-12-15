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
#include "renderlib.h"
#include "transprender.h"

#define SPARK_MIN_PROB		16
#define SPARK_FRAME_TIME	50

class CEnergySparks {
	private:
		short	m_nSegments;

	public:
		CEnergySparks () { m_nSegments = 0; };
		~CEnergySparks () {};

};

//-----------------------------------------------------------------------------

void AllocSegmentSparks (short nSegment)
{
	int				nMatCen = nSegment;
	tSegment2		*seg2P = gameData.segs.segment2s + (nSegment = gameData.matCens.sparkSegs [nSegment]);
	int				i, bFuel = (seg2P->special == SEGMENT_IS_FUELCEN);
	tSegmentSparks	*segP = gameData.matCens.sparks [bFuel] + nMatCen;
	tEnergySpark	*sparkP = segP->sparks;

segP->nMaxSparks = (ushort) (2 * AvgSegRadf (nSegment) + 0.5f);
if (!(sparkP = new tEnergySpark [segP->nMaxSparks]))
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
	delete[] segP->sparks;
	segP->nMaxSparks = 0;
	}
}

//-----------------------------------------------------------------------------

void CreateSegmentSparks (short nSegment)
{
	int				nMatCen = nSegment;
	tSegment2		*seg2P = gameData.segs.segment2s + (nSegment = gameData.matCens.sparkSegs [nSegment]);
	int				bFuel = (seg2P->special == SEGMENT_IS_FUELCEN);
	tSegmentSparks	*segP = gameData.matCens.sparks [bFuel] + nMatCen;
	tEnergySpark	*sparkP = segP->sparks;
	CFixVector		vOffs;
	CFloatVector			vMaxf, vMax2f;
	int				i;

vMaxf.Assign (gameData.segs.extent [nSegment].vMax);
vMax2f = vMaxf * 2;
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
		vOffs [X] = F2X (vMaxf [X] - f_rand () * vMax2f [X]);
		vOffs [Y] = F2X (vMaxf [Y] - f_rand () * vMax2f [Y]);
		vOffs [Z] = F2X (vMaxf [Z] - f_rand () * vMax2f [Z]);
		sparkP->vPos = *SEGMENT_CENTER_I (nSegment) + vOffs;
		if ((vOffs.Mag () > MinSegRad (nSegment)) && GetSegMasks (sparkP->vPos, nSegment, 0).centerMask)
			sparkP->nProb = 1;
		else {
			sparkP->xSize = F1_0 + 4 * d_rand ();
			sparkP->nFrame = 0;
			sparkP->tRender = -1;
			sparkP->bRendered = 0;
			sparkP->nProb = SPARK_MIN_PROB;
			if (gameOpts->render.effects.bMovingSparks) {
				sparkP->vDir [X] = (F1_0 / 4) - d_rand ();
				sparkP->vDir [Y] = (F1_0 / 4) - d_rand ();
				sparkP->vDir [Z] = (F1_0 / 4) - d_rand ();
				CFixVector::Normalize (sparkP->vDir);
				sparkP->vDir *= ((F1_0 / (16 + d_rand () % 16)));
				}
			else
				sparkP->vDir.SetZero ();
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
		int				i;

	for (i = segP->nMaxSparks; i; i--, sparkP++) {
		if (!sparkP->tRender)
			continue;
		if (gameStates.app.nSDLTicks - sparkP->tRender < SPARK_FRAME_TIME)
			continue;
		if (++sparkP->nFrame < 32) {
			sparkP->tRender = gameStates.app.nSDLTicks; //+= SPARK_FRAME_TIME;
			if (gameOpts->render.effects.bMovingSparks)
				sparkP->vPos += sparkP->vDir;
			}
		else {
			sparkP->tRender = 0;
			sparkP->tCreate = -1;
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

	segP->bUpdate = 1;
	for (i = segP->nMaxSparks; i; i--, sparkP++) {
		if (sparkP->tRender) {
			if (sparkP->nFrame > 31)
				sparkP->tRender = 0;
			else
				TIAddSpark (sparkP->vPos, (char) bFuel, sparkP->xSize, (char) sparkP->nFrame);
			}
		}
	}
}

//-----------------------------------------------------------------------------

void DoEnergySparkFrame (void)
{
SEM_ENTER (SEM_SPARKS)

	short	nSegment;

if (gameOpts->render.effects.bEnergySparks)
	for (nSegment = 0; nSegment < gameData.matCens.nSparkSegs; nSegment++)
		UpdateSegmentSparks (nSegment);
SEM_LEAVE (SEM_SPARKS)
}


//-----------------------------------------------------------------------------

void RenderEnergySparks (void)
{
if (gameOpts->render.effects.bEnergySparks) {
	for (short nSegment = 0; nSegment < gameData.matCens.nSparkSegs; nSegment++)
		RenderSegmentSparks (nSegment);
	}
}

//-----------------------------------------------------------------------------

int BuildSparkSegList (void)
{
	tSegment2	*seg2P = gameData.segs.segment2s.Buffer ();
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
SEM_ENTER (SEM_SPARKS)
	short	nSegment;

if (gameOpts->render.effects.bEnergySparks && BuildSparkSegList ()) {
	if (bmpSparks) {
		for (nSegment = 0; nSegment < gameData.matCens.nSparkSegs; nSegment++)
			AllocSegmentSparks (nSegment);
		d_srand ((uint) gameStates.app.nSDLTicks);
		}
	else
		gameOpts->render.effects.bEnergySparks = 0;
	}
SEM_LEAVE (SEM_SPARKS)
}

//-----------------------------------------------------------------------------

void FreeEnergySparks (void)
{
SEM_ENTER (SEM_SPARKS)
	short	nSegment;

if (gameOpts->render.effects.bEnergySparks)
	for (nSegment = 0; nSegment < gameData.matCens.nSparkSegs; nSegment++)
		FreeSegmentSparks (nSegment);
SEM_LEAVE (SEM_SPARKS)
}

//-----------------------------------------------------------------------------
