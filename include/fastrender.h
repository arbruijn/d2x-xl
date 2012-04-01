#ifndef _FASTRENDER_H
#define _FASTRENDER_H

#include "descent.h"
#include "endlevel.h"

//------------------------------------------------------------------------------

void QSortFaces (int left, int right);
int SetupDepthBuffer (int nType);
void RenderFaceList (int nType);
void ComputeDynamicQuadLight (int nStart, int nEnd, int nThread);
void ComputeDynamicTriangleLight (int nStart, int nEnd, int nThread);
void ComputeDynamicFaceLight (int nStart, int nEnd, int nThread);
void ComputeStaticFaceLight (int nStart, int nEnd, int nThread);
void ComputeMineLighting (short nStartSeg, fix xStereoSeparation, int nWindow);
void UpdateSlidingFaces (void);
int CountRenderFaces (void);
void GetRenderFaces (void);
void RenderMineObjects (int nType);
void RenderSkyBoxFaces (void);
int SetupCoronas (void);

//------------------------------------------------------------------------------

static inline void ComputeFaceLight (int nStart, int nEnd, int nThread)
{
if (gameStates.render.bApplyDynLight && (gameStates.app.bEndLevelSequence < EL_OUTSIDE)) {
	if (gameStates.render.bTriangleMesh)
		ComputeDynamicTriangleLight (nStart, nEnd, nThread);
	else if (gameData.render.mine.visibility [0].nSegments < gameData.segs.nSegments)
		ComputeDynamicQuadLight (nStart, nEnd, nThread);
	else
		ComputeDynamicFaceLight (nStart, nEnd, nThread);
	}
else
	ComputeStaticFaceLight (nStart, nEnd, nThread);
}

//------------------------------------------------------------------------------

#endif /* _FASTRENDER_H */
