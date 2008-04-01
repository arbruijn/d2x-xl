#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "inferno.h"
#include "cfile.h"
#include "u_mem.h"
#include "error.h"

#ifdef RCS
static char rcsid[] = "$Id: tSegment.c,v 1.3 2003/10/10 09:36:35 btb Exp $";
#endif

#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/

//------------------------------------------------------------------------------
// reads a tSegment2 structure from a CFILE
 
void ReadSegment2 (tSegment2 *s2, CFILE *fp)
{
	s2->special = CFReadByte (fp);
	s2->nMatCen = CFReadByte (fp);
	s2->value = CFReadByte (fp);
	s2->s2Flags = CFReadByte (fp);
	s2->xAvgSegLight = CFReadFix (fp);
}

//------------------------------------------------------------------------------
// reads a tLightDelta structure from a CFILE

void ReadLightDelta (tLightDelta *dlP, CFILE *fp)
{
dlP->nSegment = CFReadShort (fp);
dlP->nSide = CFReadByte (fp);
CFReadByte (fp);
if (!(dlP->bValid = (dlP->nSegment >= 0) && (dlP->nSegment < gameData.segs.nSegments) && (dlP->nSide >= 0) && (dlP->nSide < 6)))
	PrintLog ("Invalid delta light data %d (%d,%d)\n", dlP - gameData.render.lights.deltas, dlP->nSegment, dlP->nSide);
CFRead (dlP->vertLight, sizeof (dlP->vertLight [0]), sizeofa (dlP->vertLight), fp);
}


//------------------------------------------------------------------------------
// reads a tLightDeltaIndex structure from a CFILE

void ReadLightDeltaIndex (tLightDeltaIndex *di, CFILE *fp)
{
if (gameStates.render.bD2XLights) {
	short	i, j;
	di->d2x.nSegment = CFReadShort (fp);
	i = (short) CFReadByte (fp);
	j = (short) CFReadByte (fp);
	di->d2x.nSide = i;
	di->d2x.count = (j << 5) + ((i >> 3) & 63);
	di->d2x.index = CFReadShort (fp);
	}
else {
	di->d2.nSegment = CFReadShort (fp);
	di->d2.nSide = CFReadByte (fp);
	di->d2.count = CFReadByte (fp);
	di->d2.index = CFReadShort (fp);
	}
}
#endif

//------------------------------------------------------------------------------

int CountSkyBoxSegments (void)
{
	tSegment2	*seg2P;
	int			i, nSegments;

for (i = gameData.segs.nSegments, nSegments = 0, seg2P = SEGMENT2S; i; i--, seg2P++)
	if (seg2P->special == SEGMENT_IS_SKYBOX)
		nSegments++;
return nSegments;
}

//------------------------------------------------------------------------------

void FreeSkyBoxSegList (void)
{
D2_FREE (gameData.segs.skybox.segments);
}

//------------------------------------------------------------------------------

int BuildSkyBoxSegList (void)
{
FreeSkyBoxSegList ();
if ((gameData.segs.skybox.nSegments = CountSkyBoxSegments ())) {
	tSegment2	*seg2P;
	short			*segP;
	int			h, i;

if (!(gameData.segs.skybox.segments = (short *) D2_ALLOC (gameData.segs.nSegments * sizeof (short))))
	return 0;
segP = gameData.segs.skybox.segments;
for (h = gameData.segs.nSegments, i = 0, seg2P = SEGMENT2S; i < h; i++, seg2P++)
	if (seg2P->special == SEGMENT_IS_SKYBOX)
		*segP++ = i;
	}
gameStates.render.bHaveSkyBox = (gameData.segs.skybox.nSegments > 0);
return gameData.segs.skybox.nSegments;
}

//------------------------------------------------------------------------------
//eof
