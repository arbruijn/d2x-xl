#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "u_mem.h"
#include "error.h"

//------------------------------------------------------------------------------

int32_t CSkyBox::CountSegments (void)
{
	CSegment*	segP;
	int32_t			i, nSegments;

for (i = gameData.segs.nSegments, nSegments = 0, segP = SEGMENTS.Buffer (); i; i--, segP++)
	if (segP->m_function == SEGMENT_FUNC_SKYBOX)
		nSegments++;
return nSegments;
}


//------------------------------------------------------------------------------

void CSkyBox::Destroy (void)
{
CStack<int16_t>::Destroy ();
gameStates.render.bHaveSkyBox = 0;
}

//------------------------------------------------------------------------------

int32_t BuildSkyBoxSegList (void)
{
gameData.segs.skybox.Destroy ();

int16_t nSegments = gameData.segs.skybox.CountSegments ();

if (!nSegments) 
	return 0;
if (!gameData.segs.skybox.Create (nSegments))
	return 0;
for (int32_t i = 0; i < gameData.segs.nSegments; i++)
	if (gameData.Segment (i)->m_function == SEGMENT_FUNC_SKYBOX)
		gameData.segs.skybox.Push (i);
gameStates.render.bHaveSkyBox = (gameData.segs.skybox.ToS () > 0);
return gameData.segs.skybox.ToS ();
}

//------------------------------------------------------------------------------
//eof
