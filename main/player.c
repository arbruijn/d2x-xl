/* $Id: player.c,v 1.3 2003/10/10 09:36:35 btb Exp $ */

/*
 *
 * Player Stuff
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "inferno.h"

#ifdef RCS
static char rcsid[] = "$Id: player.c,v 1.3 2003/10/10 09:36:35 btb Exp $";
#endif

/*
 * reads a tPlayerShip structure from a CFILE
 */
void PlayerShipRead(tPlayerShip *ps, CFILE *fp)
{
	int i;

	ps->nModel = CFReadInt(fp);
	ps->nExplVClip = CFReadInt(fp);
	ps->mass = CFReadFix(fp);
	ps->drag = CFReadFix(fp);
	ps->maxThrust = CFReadFix(fp);
	ps->reverseThrust = CFReadFix(fp);
	ps->brakes = CFReadFix(fp);
	ps->wiggle = CFReadFix(fp);
	ps->maxRotThrust = CFReadFix(fp);
	for (i = 0; i < N_PLAYER_GUNS; i++)
		CFReadVector(&(ps->gunPoints[i]), fp);
}
