/* $Id: segment.c,v 1.3 2003/10/10 09:36:35 btb Exp $ */

/*
 *
 * Segment Loading Stuff
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "inferno.h"
#include "cfile.h"

#ifdef RCS
static char rcsid[] = "$Id: segment.c,v 1.3 2003/10/10 09:36:35 btb Exp $";
#endif

#ifndef FAST_FILE_IO
/*
 * reads a segment2 structure from a CFILE
 */
void segment2_read(segment2 *s2, CFILE *fp)
{
	s2->special = CFReadByte (fp);
	s2->matcen_num = CFReadByte (fp);
	s2->value = CFReadByte (fp);
	s2->s2_flags = CFReadByte (fp);
	s2->static_light = CFReadFix (fp);
}

/*
 * reads a delta_light structure from a CFILE
 */
void delta_light_read(delta_light *dl, CFILE *fp)
{
	dl->segnum = CFReadShort (fp);
	dl->sidenum = CFReadByte (fp);
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
	di->d2x.segnum = CFReadShort (fp);
	i = (short) CFReadByte (fp);
	j = (short) CFReadByte (fp);
	di->d2x.sidenum = i;
	di->d2x.count = (j << 5) + ((i >> 3) & 63);
	di->d2x.index = CFReadShort (fp);
	}
else {
	di->d2.segnum = CFReadShort (fp);
	di->d2.sidenum = CFReadByte (fp);
	di->d2.count = CFReadByte (fp);
	di->d2.index = CFReadShort (fp);
	}
}
#endif
