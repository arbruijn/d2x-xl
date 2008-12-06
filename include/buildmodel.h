#ifndef _BUILDMODEL_H
#define _BUILDMODEL_H

int G3AllocModel (tG3Model *pm);
int G3FreeModelItems (tG3Model *pm);
void G3FreeAllPolyModelItems (void);
void G3InitSubModelMinMax (tG3SubModel *psm);
void G3SetSubModelMinMax (tG3SubModel *psm, fVector3 *vertexP);
void G3SortFaces (tG3SubModel *psm, int left, int right, CBitmap *pTextures);
void G3SortFaceVerts (tG3Model *pm, tG3SubModel *psm, tG3ModelVertex *psv);
void G3SetupModel (tG3Model *pm, int bHires, int bSort);
int G3ShiftModel (CObject *objP, int nModel, int bHires);
fix G3ModelSize (CObject *objP, tG3Model *pm, int nModel, int bHires);
void G3SetGunPoints (CObject *objP, tG3Model *pm, int nModel, int bASE);
int G3BuildModel (CObject *objP, int nModel, tPolyModel *pp, CBitmap **modelBitmaps, tRgbaColorf *pObjColor, int bHires);
int G3ModelMinMax (int nModel, tHitbox *phb);

int G3BuildModelFromASE (CObject *objP, int nModel);
int G3BuildModelFromOOF (CObject *objP, int nModel);
int G3BuildModelFromPOF (CObject *objP, int nModel, tPolyModel *pp, CBitmap **modelBitmaps, tRgbaColorf *pObjColor);

//------------------------------------------------------------------------------

static inline int IsDefaultModel (int nModel)
{
return gameData.models.polyModels [nModel].nDataSize == gameData.models.defPolyModels [nModel].nDataSize;
}

//------------------------------------------------------------------------------

#endif //_BUILDMODEL_H
//eof
