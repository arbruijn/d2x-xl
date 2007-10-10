/* $Id: render.c, v 1.18 2003/10/10 09:36:35 btb Exp $ */
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

#include "inferno.h"

//------------------------------------------------------------------------------

#define	PP_DELTAZ	-i2f(30)
#define	PP_DELTAY	i2f(10)

tFlightPath	externalView;

//------------------------------------------------------------------------------

void ResetFlightPath (tFlightPath *pPath, int nSize, int nFPS)
{
pPath->nSize = (nSize < 0) ? MAX_PATH_POINTS : nSize;
pPath->tRefresh = (time_t) (1000 / ((nFPS < 0) ? 40 : nFPS));
pPath->nStart =
pPath->nEnd = 0;
pPath->pPos = NULL;
pPath->tUpdate = -1;
}

//------------------------------------------------------------------------------

void SetPathPoint (tFlightPath *pPath, tObject *objP)
{
	time_t	t = SDL_GetTicks () - pPath->tUpdate;

if (pPath->nSize && ((pPath->tUpdate < 0) || (t >= pPath->tRefresh))) {
	pPath->tUpdate = t;
//	h = pPath->nEnd;
	pPath->nEnd = (pPath->nEnd + 1) % pPath->nSize;
	pPath->path [pPath->nEnd].vOrgPos = objP->position.vPos;
	pPath->path [pPath->nEnd].vPos = objP->position.vPos;
	pPath->path [pPath->nEnd].mOrient = objP->position.mOrient;
	VmVecScaleInc (&pPath->path [pPath->nEnd].vPos, &objP->position.mOrient.fVec, 0);
	VmVecScaleInc (&pPath->path [pPath->nEnd].vPos, &objP->position.mOrient.uVec, 0);
//	if (!memcmp (pPath->path + h, pPath->path + pPath->nEnd, sizeof (tMovementPath)))
//		pPath->nEnd = h;
//	else 
	if (pPath->nEnd == pPath->nStart)
		pPath->nStart = (pPath->nStart + 1) % pPath->nSize;
	}
}

//------------------------------------------------------------------------------

tPathPoint *GetPathPoint (tFlightPath *pPath)
{
	vmsVector		*p = &pPath->path [pPath->nEnd].vPos;
	int				i;

if (pPath->nStart == pPath->nEnd) {
	pPath->pPos = NULL;
	return NULL;
	}
i = pPath->nEnd;
do {
	if (!i)
		i = pPath->nSize;
	i--;
	if (VmVecDist (&pPath->path [i].vPos, p) >= i2f (15))
		break;
	}
while (i != pPath->nStart);
return pPath->pPos = pPath->path + i;
}

//------------------------------------------------------------------------------

void GetViewPoint (void)
{
	tPathPoint		*p = GetPathPoint (&externalView);

if (!p)
	VmVecScaleInc (&gameData.render.mine.viewerEye, &gameData.objs.viewer->position.mOrient.fVec, PP_DELTAZ);
else {
	gameData.render.mine.viewerEye = p->vPos;
	VmVecScaleInc (&gameData.render.mine.viewerEye, &p->mOrient.fVec, PP_DELTAZ * 2 / 3);
	VmVecScaleInc (&gameData.render.mine.viewerEye, &p->mOrient.uVec, PP_DELTAY * 2 / 3);
	}
}

//------------------------------------------------------------------------------
// eof