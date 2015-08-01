#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "u_mem.h"
#include "error.h"

//------------------------------------------------------------------------------

int32_t CSkyBox::CountSegments (void)
{
	CSegment*	pSeg;
	int32_t			i, nSegments;

for (i = gameData.segData.nSegments, nSegments = 0, pSeg = SEGMENTS.Buffer (); i; i--, pSeg++)
	if (pSeg->m_function == SEGMENT_FUNC_SKYBOX)
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
gameData.segData.skybox.Destroy ();

int16_t nSegments = gameData.segData.skybox.CountSegments ();

if (!nSegments) 
	return 0;
if (!gameData.segData.skybox.Create (nSegments))
	return 0;
for (int32_t i = 0; i < gameData.segData.nSegments; i++)
	if (SEGMENT (i)->m_function == SEGMENT_FUNC_SKYBOX)
		gameData.segData.skybox.Push (i);
gameStates.render.bHaveSkyBox = (gameData.segData.skybox.ToS () > 0);
return gameData.segData.skybox.ToS ();
}

//------------------------------------------------------------------------------
//eof
