#ifndef _FASTRENDER_H
#define _FASTRENDER_H

#include "descent.h"
#include "endlevel.h"

//------------------------------------------------------------------------------

void QSortFaces (int32_t left, int32_t right);
int32_t SetupDepthBuffer (int32_t nType);
void RenderFaceList (int32_t nType);
void ComputeDynamicQuadLight (int32_t nStart, int32_t nEnd, int32_t nThread);
void ComputeDynamicTriangleLight (int32_t nStart, int32_t nEnd, int32_t nThread);
void ComputeDynamicFaceLight (int32_t nStart, int32_t nEnd, int32_t nThread);
void ComputeStaticFaceLight (int32_t nStart, int32_t nEnd, int32_t nThread);
void ComputeMineLighting (int16_t nStartSeg, fix xStereoSeparation, int32_t nWindow);
void UpdateSlidingFaces (void);
int32_t CountRenderFaces (void);
void GetRenderFaces (void);
void RenderMineObjects (int32_t nType);
void RenderSkyBoxFaces (void);
int32_t SetupCoronas (void);

//------------------------------------------------------------------------------

static inline void ComputeFaceLight (int32_t nStart, int32_t nEnd, int32_t nThread)
{
if (gameStates.render.bApplyDynLight && (gameStates.app.bEndLevelSequence < EL_OUTSIDE)) {
	if (gameStates.render.bTriangleMesh)
		ComputeDynamicTriangleLight (nStart, nEnd, nThread);
	else if (gameData.renderData.mine.visibility [0].nSegments < gameData.segData.nSegments)
		ComputeDynamicQuadLight (nStart, nEnd, nThread);
	else
		ComputeDynamicFaceLight (nStart, nEnd, nThread);
	}
else
	ComputeStaticFaceLight (nStart, nEnd, nThread);
}

//------------------------------------------------------------------------------

#endif /* _FASTRENDER_H */
