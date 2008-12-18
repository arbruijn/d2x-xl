#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "inferno.h"
#include "u_mem.h"
#include "error.h"

//------------------------------------------------------------------------------

int CSkyBox::CountSegments (void)
{
	CSegment*	segP;
	int			i, nSegments;

for (i = gameData.segs.nSegments, nSegments = 0, segP = SEGMENTS.Buffer (); i; i--, segP++)
	if (segP->m_nType == SEGMENT_IS_SKYBOX)
		nSegments++;
return nSegments;
}


//------------------------------------------------------------------------------

void CSkyBox::Destroy (void)
{
CStack::Destroy ();
gameStates.render.bHaveSkyBox = 0;
}

//------------------------------------------------------------------------------

int BuildSkyBoxSegList (void)
{
gameData.segs.skybox.Destroy ();

short nSegments = gameData.segs.skybox.CountSegments ();

if (!nSegments) {
	return 0;

	CSegment*	segP;
	int			h, i;

if (!(gameData.segs.skybox.Create (nSegments)))
	return 0;
for (h = gameData.segs.nSegments, i = 0, segP = SEGMENTS.Buffer (); i < h; i++, segP++)
	if (segP->m_nType == SEGMENT_IS_SKYBOX)
		gameData.segs.skybox.Push (i);
	}
gameStates.render.bHaveSkyBox = (gameData.segs.skybox.ToS () > 0);
return gameData.segs.skybox.ToS ();
}

//------------------------------------------------------------------------------
//eof
