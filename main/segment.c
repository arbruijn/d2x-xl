#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "inferno.h"
#include "cfile.h"

#ifdef RCS
static char rcsid[] = "$Id: tSegment.c,v 1.3 2003/10/10 09:36:35 btb Exp $";
#endif

#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads a tSegment2 structure from a CFILE
 */
void segment2_read(tSegment2 *s2, CFILE *fp)
{
	s2->special = CFReadByte (fp);
	s2->nMatCen = CFReadByte (fp);
	s2->value = CFReadByte (fp);
	s2->s2Flags = CFReadByte (fp);
	s2->xAvgSegLight = CFReadFix (fp);
}

/*
 * reads a tLightDelta structure from a CFILE
 */
void delta_light_read(tLightDelta *dl, CFILE *fp)
{
	dl->nSegment = CFReadShort (fp);
	dl->nSide = CFReadByte (fp);
	dl->dummy = CFReadByte (fp);
	dl->vert_light[0] = CFReadByte (fp);
	dl->vert_light[1] = CFReadByte (fp);
	dl->vert_light[2] = CFReadByte (fp);
	dl->vert_light[3] = CFReadByte (fp);
}


/*
 * reads a dl_index structure from a CFILE
 */
void dl_index_read(dl_index *di, CFILE *fp)
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
