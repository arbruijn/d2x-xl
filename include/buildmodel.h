#ifndef _BUILDMODEL_H
#define _BUILDMODEL_H

int G3AllocModel (RenderModel::CModel *pm);
int G3FreeModelItems (RenderModel::CModel *pm);
void G3FreeAllPolyModelItems (void);
void G3InitSubModelMinMax (RenderModel::CSubModel *psm);
void G3SetSubModelMinMax (RenderModel::CSubModel *psm, CFloatVector3 *vertexP);
void G3SortFaces (RenderModel::CSubModel *psm, int left, int right, CBitmap *pTextures);
void G3SortFaceVerts (RenderModel::CModel *pm, RenderModel::CSubModel *psm, RenderModel::CVertex *psv);
void G3SetupModel (RenderModel::CModel *pm, int bHires, int bSort);
int G3ShiftModel (CObject *objP, int nModel, int bHires);
fix G3ModelSize (CObject *objP, RenderModel::CModel *pm, int nModel, int bHires);
void G3SetGunPoints (CObject *objP, RenderModel::CModel *pm, int nModel, int bASE);
int G3BuildModel (CObject *objP, int nModel, CPolyModel *pp, CArray<CBitmap*>& modelBitmaps, tRgbaColorf *pObjColor, int bHires);
int G3ModelMinMax (int nModel, tHitbox *phb);

int G3BuildModelFromASE (CObject *objP, int nModel);
int G3BuildModelFromOOF (CObject *objP, int nModel);
int G3BuildModelFromPOF (CObject *objP, int nModel, CPolyModel *pp, CArray<CBitmap*>& modelBitmaps, tRgbaColorf *pObjColor);

//------------------------------------------------------------------------------

static inline int IsDefaultModel (int nModel)
{
return (nModel >= gameData.models.nPolyModels) ||
		 (gameData.models.polyModels [0][nModel].DataSize () == gameData.models.polyModels [1][nModel].DataSize ());
}

//------------------------------------------------------------------------------

#endif //_BUILDMODEL_H
//eof
